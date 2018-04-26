/*****************************************************************************
 * autonight.c
 * April 26, 2018
 *
 * Copyright 2018 - Howard Logic
 * https://howardlogic.com
 * All Rights Reserved
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 *****************************************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

const char *device = "/dev/jz_adc_aux_0";
const char *nightModeCmd = "/system/sdcard/scripts/nightmode.sh";
int readAverageCount = 25;
int delayBetweenReads = 10;
int windowSize = 5;
double thresholdOn = 40.0;
double thresholdOff = 45.0;
bool verbose = false;
bool nightModeEnabled = false;

int jzAuxReadSample(const char *device, unsigned int *buffer) {
        int fd = open(device, O_RDONLY);
        if(fd == -1) return -1;
        int size = sizeof(*buffer);
        int ct = read(fd, (void *)buffer, size);
        close(fd);
        return ct == size;
}

int jzAuxReadAverage(const char *device, double *value, int count) {
        if(count < 1) return 0;
        double valret = 0.0;
        for(int i = 0; i < count; i++) {
                unsigned int sample;
                if(!jzAuxReadSample(device, &sample)) return 0;
                valret += sample;
        }
        valret /= count;
        if(value) *value = valret;
        return 1;
}

void updateNightMode() {
        char buf[256];
        if(verbose) printf("Night Mode %s\n", nightModeEnabled ? "Enabled" : "Disabled");
        snprintf(buf, sizeof(buf), "%s %s\n", nightModeCmd, nightModeEnabled ? "on" : "off");
        int ret = system(buf);
        if(ret) fprintf(stderr, "WARNING: %s returned %d\n", buf, ret);
        return;
}

void usage() {
        printf("Usage: autonight [options]\n\n");
        printf("Options:\n");
        printf("-D <str>        Sets jz_adc_aux device (default: %s)\n", device);
        printf("-c <str>        Sets the command to call to set night mode (default: %s)\n", nightModeCmd);
        printf("-a <int>        Sets the number of ADC reads to average into a single sample (default: %d)\n", readAverageCount);
        printf("-d <int>        Delay (in seconds) between averaged reads (default: %d)\n", delayBetweenReads);
        printf("-n <int>        Number of averaged samples to window average for thresholding (default: %d)\n", windowSize);
        printf("-O <float>      Turn on night mode when window average value drops below this threshold (default: %.2lf)\n", thresholdOn);
        printf("-F <floag>      Turn off night mode when window average value goes above this threshold (default: %.2lf)\n", thresholdOff);
        printf("-v              Enable verbose output\n");
        printf("-h              Print this usage statement and exit\n");
        return;
}

int main(int argc, char *argv[]) {
        int opt;
        int windowIndex = 0;
        bool windowFull = false;
        double *window = (double *)malloc(sizeof(double) * windowSize);

        while((opt = getopt(argc, argv, "D:c:a:d:n:O:F:vh")) != -1) {
                switch (opt) {
                        case 'D': device = optarg; 
                                  break;

                        case 'c': nightModeCmd = optarg;
                                  break;

                        case 'a': readAverageCount = atoi(optarg);
                                  break;

                        case 'd': delayBetweenReads = atoi(optarg);
                                  break;

                        case 'n': windowSize = atoi(optarg);
                                  break;

                        case 'O': thresholdOn = atof(optarg);
                                  break;

                        case 'F': thresholdOff = atof(optarg);
                                  break;

                        case 'v':
                                  verbose = true;
                                  break;

                        case 'h':
                                  usage();
                                  return 0;

                        default:
                                  fprintf(stderr, "Unknown argument: '%c'\n", opt);
                                  usage();
                                  return 1;
                }
        }

        while(true) {
                double value;
                int ret = jzAuxReadAverage(device, &value, readAverageCount);
                if(!ret) {
                        fprintf(stderr, "ERROR: jzAuxReadAverage(%d) failed: %m\n", readAverageCount);
                        sleep(delayBetweenReads);
                        continue;
                }
                if(verbose) printf("Current value: %.2lf\n", value);
                window[windowIndex] = value;
                windowIndex++;
                if(windowIndex >= windowSize) {
                        windowFull = true;
                        windowIndex = 0;
                }
                if(windowFull) {
                        double windowAvg = 0.0;
                        for(int i = 0; i < windowSize; i++) windowAvg += window[i];
                        windowAvg /= windowSize;
                        if(verbose) printf("Window (%d) Avg: %.2lf\n", windowSize, windowAvg);
                        if(!nightModeEnabled && windowAvg <= thresholdOn) {
                                nightModeEnabled = true;
                                updateNightMode();

                        } else if(nightModeEnabled && windowAvg >= thresholdOff) {
                                nightModeEnabled = false;
                                updateNightMode();
                        }
                }
                sleep(delayBetweenReads);
        }

        return 0;
}




