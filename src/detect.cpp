/*
 * yafdb - Yet Another Face Detection and Bluring
 *
 * Copyright (c) 2014 FOXEL SA - http://foxel.ch
 * Please read <http://foxel.ch/license> for more information.
 *
 *
 * Author(s):
 *
 * Antony Ducommun <nitro@tmsrv.org>
 *
 *
 * This file is part of the FOXEL project <http://foxel.ch>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Additional Terms:
 *
 * You are required to preserve legal notices and author attributions in
 * that material or in the Appropriate Legal Notices displayed by works
 * containing it.
 *
 * You are required to attribute the work as explained in the "Usage and
 * Attribution" section of <http://foxel.ch/license>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>

#include "detectors/gnomonic.hpp"
#include "detectors/haar.hpp"


/*
 * List of supported algorithms.
 *
 */

#define ALGORITHM_NONE      0
#define ALGORITHM_HAAR      1


/*
 * Program arguments.
 *
 */

#define OPTION_ALGORITHM              0
#define OPTION_GNOMONIC               1
#define OPTION_GNOMONIC_WIDTH         2
#define OPTION_GNOMONIC_APERTURE_X    3
#define OPTION_GNOMONIC_APERTURE_Y    4


static int algorithm = ALGORITHM_HAAR;
static int gnomonic_enabled = 0;
static int gnomonic_width = 256;
static double gnomonic_aperture_x = 30;
static double gnomonic_aperture_y = 30;
static const char *source_file = NULL;
static const char *objects_file = NULL;


static struct option options[] = {
    {"algorithm", required_argument, 0, 'a'},
    {"gnomonic", no_argument, &gnomonic_enabled, 1},
    {"gnomonic-width", required_argument, 0, 0},
    {"gnomonic-aperture-x", required_argument, 0, 0},
    {"gnomonic-aperture-y", required_argument, 0, 0},
    {0, 0, 0, 0}
};


/**
 * Display program usage.
 *
 */
void usage() {
    printf("yafdb-detect --algorithm algo input-panorama.tiff output-objects.txt\n\n");

    printf("Detects objects within input panorama (eqr). Detected objects are\n");
    printf("written to a text file.\n\n");

    printf("--algorithm algo: algorithm to use for object detection ('haar')\n");
    printf("--gnomonic: activate gnomonic reprojection task\n");
    printf("--gnomonic-width 256: window width for reprojection task\n");
    printf("--gnomonic-aperture-x 30: horizontal angle increment for reprojection task\n");
    printf("--gnomonic-aperture-y 30: vertical angle increment for reprojection task\n");
}


/**
 * Program entry-point.
 *
 */
int main(int argc, char **argv) {
    // parse arguments
    while (true) {
        int index = -1;

        getopt_long(argc, argv, "", options, &index);
        if (index == -1) {
            if (argc != optind + 2) {
                usage();
                return 1;
            }
            source_file = argv[optind++];
            if (access(source_file, R_OK)) {
                fprintf(stderr, "Error: source file not readable: %s\n", source_file);
                return 2;
            }

            objects_file = argv[optind++];
            if (access(objects_file, W_OK) && errno == EACCES) {
                fprintf(stderr, "Error: detected objects file not writable: %s\n", objects_file);
                return 2;
            }
            break;
        }

        switch (index) {
        case OPTION_ALGORITHM:
            if (strcmp(optarg, "none") == 0) {
                algorithm = ALGORITHM_NONE;
            } else if (strcmp(optarg, "haar") == 0) {
                algorithm = ALGORITHM_HAAR;
            } else {
                fprintf(stderr, "Error: unsupported algorithm: %s\n", optarg);
            }
            break;

        case OPTION_GNOMONIC:
            break;

        case OPTION_GNOMONIC_WIDTH:
            gnomonic_width = atoi(optarg);
            break;

        case OPTION_GNOMONIC_APERTURE_X:
            gnomonic_aperture_x = atof(optarg);
            break;

        case OPTION_GNOMONIC_APERTURE_Y:
            gnomonic_aperture_y = atof(optarg);
            break;

        default:
            usage();
            return 1;
        }
    }

    // read source file
    cv::Mat source = cv::imread(source_file);

    if (source.rows <= 0 || source.cols <= 0) {
        fprintf(stderr, "Error: cannot read image in source file: %s\n", source_file);
        return 2;
    }

    // instantiate detector
    ObjectDetector *detector;

    switch (algorithm) {
    case ALGORITHM_NONE:
        detector = new ObjectDetector();
        break;

    case ALGORITHM_HAAR:
        detector = (new MultiDetector())->addDetector(
            new HaarDetector("frontface", "haarcascades/haarcascade_frontalface_default.xml")
        )->addDetector(
            new HaarDetector("frontface", "haarcascades/haarcascade_frontalface_alt.xml")
        )->addDetector(
            new HaarDetector("frontface", "haarcascades/haarcascade_frontalface_alt2.xml")
        )->addDetector(
            new HaarDetector("frontface", "haarcascades/haarcascade_frontalface_alt_tree.xml")
        )->addDetector(
            new HaarDetector("profileface", "haarcascades/haarcascade_profileface.xml")
        );
        break;

    default:
        fprintf(stderr, "Error: no detector instantiated!\n");
        return 3;
    }

    // setup gnomonic reprojection task
    if (gnomonic_enabled) {
        detector = new GnomonicProjectionDetector(detector, gnomonic_width, gnomonic_aperture_x, gnomonic_aperture_y);
    }

    // detect objects in source image
    bool success = false;

    if (detector != NULL) {
        // display source file
        // detector->preview(source);

        // run detection algorithm
        std::list<DetectedObject> objects;

        if (detector->detect(source, objects)) {
            success = true;

            // TODO: save detected objects

            // display detected objects
            detector->preview(source, objects);
        }

        delete detector;
        detector = NULL;
    }
    return success ? 0 : 4;
}