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
#include <string.h>
#include <math.h>

const char *device = "/dev/jz_adc_aux_0";
const char *nightModeCmd = "/system/sdcard/scripts/nightmode.sh";
int readAverageCount = 25;
int delayBetweenReads = 10;
int windowSize = 5;
double thresholdOn = 40.0;
double thresholdOff = 45.0;
int verbose = 0;
bool nightModeEnabled = false;

int software_method(void);
// Configuration parameters for software method
#if 1
int jitter_percent = 3;
int eq1_user_exposure = 1200000;
int eq2_user_exposure = 930000;
int eq2_user_iridix = 14;
int eq3_user_wb = 3000;
int eq3_user_iridix = 17;
int eq2_count = 10;
int eq3_count = 8;
int sec_wait = 3;

int mode_switch_wait_count=0;
#else
int max_night_ex_wait = 6;
int max_faill_count = 25;
int max_iridx_wait = 20;
int exposure_tolerance = 1;
int wait_iridix_diff = 2;
int day_color_temp = 200;
int day_exposure_drop = 5;
#endif


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
        mode_switch_wait_count = sec_wait;
        return;
}

void usage() {
        printf("Usage: autonight [options]\n\n");
        printf("Options:\n");
        printf("-v              Increase verbosity\n");
        printf("-S              uses software to determine day/night\n");
        printf("                    following options don't mean anything for software method\n");
        printf("                    use sw_night_configure.html for more software mode options\n");
        printf("-D <str>        Sets jz_adc_aux device (default: %s)\n", device);
        printf("-c <str>        Sets the command to call to set night mode (default: %s)\n", nightModeCmd);
        printf("-a <int>        Sets the number of ADC reads to average into a single sample (default: %d)\n", readAverageCount);
        printf("-d <int>        Delay (in seconds) between averaged reads (default: %d)\n", delayBetweenReads);
        printf("-n <int>        Number of averaged samples to window average for thresholding (default: %d)\n", windowSize);
        printf("-O <float>      Turn on night mode when window average value drops below this threshold (default: %.2lf)\n", thresholdOn);
        printf("-F <floag>      Turn off night mode when window average value goes above this threshold (default: %.2lf)\n", thresholdOff);
        printf("-h              Print this usage statement and exit\n");
        return;
}

int main(int argc, char *argv[]) {
        int opt;
        int windowIndex = 0;
        bool windowFull = false;
        double *window = (double *)malloc(sizeof(double) * windowSize);
        bool use_software_method = false;

        while((opt = getopt(argc, argv, "D:c:a:d:n:O:F:vShj:w:1:2:3:")) != -1) {
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
                                  verbose++;
                                  break;

                        case 'S':
                                  use_software_method = true;
                                  break;
                        case 'j':
                                  jitter_percent = atoi(optarg);
                                  break;
                        case 'w':
                                  sec_wait = atoi(optarg);
                                  break;
                        case '1':
                                  eq1_user_exposure = atoi(optarg);
                                  break;
                        case '2':
                                  if(sscanf(optarg, "%d,%d,%d", &eq2_user_exposure, &eq2_user_iridix, &eq2_count) != 3){
                                    fprintf(stderr, "invalid argument: %s\n", optarg);
                                    usage();
                                  }
                                  break;
                        case '3':
                                  if(sscanf(optarg, "%d,%d,%d", &eq3_user_wb, &eq3_user_iridix, &eq3_count) != 3){
                                    fprintf(stderr, "invalid argument: %s\n", optarg);
                                    usage();
                                  }
                                  break;
                        /*case 'N':
                                  max_night_ex_wait = atoi(optarg);
                                  break;

                        case 'M':
                                  max_faill_count = atoi(optarg);
                                  break;

                        case 'W':
                                  max_iridx_wait = atoi(optarg);
                                  break;

                        case 'T':
                                  exposure_tolerance = atoi(optarg);
                                  break;

                        case 'I':
                                  wait_iridix_diff = atoi(optarg);
                                  break;

                        case 'B':
                                  day_color_temp = atoi(optarg);
                                  break;

                        case 'X':
                                  day_exposure_drop = atoi(optarg);
                                  break;
                        */
                        case 'h':
                                  usage();
                                  return 0;

                        default:
                                  fprintf(stderr, "Unknown argument: '%c'\n", opt);
                                  usage();
                                  return 1;
                }
        }
        if(use_software_method){
          return software_method();
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
#define ISP_INFO  "/proc/jz/isp/isp_info"
#define EXPOSURE  "ISP exposure log2 id: "
#define IRIDIX    "ISP Iridix strength : "
#define COLORTEMP "ISP WB Temperature : "

int readIspInfo(unsigned int *exposure, unsigned int *iridix, unsigned int *colortemp)
{
  int fd, bytes_to_read, bytes_in_buf, i, bytes_read;
  char buf[100];
  int ret=0;

  buf[99] = 0;
  if(!exposure || !iridix || !colortemp){
    fprintf(stderr, "ERROR: %s: Invalid param\n", __func__);
    return -1;
  }

  if( (fd = open(ISP_INFO, O_RDONLY)) <= 0){
    fprintf(stderr, "ERROR: %s: open failed\n", __func__);
    return -1;
  }

  bytes_to_read = sizeof(buf);
  bytes_in_buf = 0;
  while( ( bytes_read = read(fd, &buf[bytes_in_buf], bytes_to_read) ) > 0 || bytes_in_buf > 0) {
    if(bytes_read > 0) bytes_in_buf += bytes_read;
    if(strncmp(buf, EXPOSURE, sizeof(EXPOSURE)-1) == 0){
      *exposure = atoi(&buf[sizeof(EXPOSURE)-1]);
      ret++;
    }else if(strncmp(buf, IRIDIX, sizeof(IRIDIX)-1) == 0){
      *iridix = atoi(&buf[sizeof(IRIDIX)-1]);
      ret++;
    }else if(strncmp(buf, COLORTEMP, sizeof(COLORTEMP)-1) == 0){
      *colortemp = atoi(&buf[sizeof(COLORTEMP)-1]);
      ret++;
    }
    if(ret == 3) break;

    i = 0;
    while(i < bytes_in_buf && buf[i] != 0xA){
      i++;
    }
    if(i >= bytes_in_buf){
      fprintf(stderr, "ERROR: %s: line is longer than %d chars buf\n", __func__, bytes_in_buf);
      close(fd);
      return ret;
    }
    memcpy(buf, &buf[i+1], bytes_in_buf - i - 1);
    bytes_to_read = i+1;
    bytes_in_buf = bytes_in_buf - bytes_to_read;
  }
  close(fd);
  return ret;
}

typedef enum {
  DAY_MODE,
  WAIT_IRIDX,
  NIGHT_MODE_START,
  NIGHT_MODE_WAIT,
} States;


/*
int jitter_percent = 3;
int eq1_user_exposure = 1200000;
int eq2_user_exposure = 930000;
int eq2_user_iridix = 14;
int eq3_user_wb = 3000;
int eq3_user_iridix = 17;
int eq2_count = 10;
int eq3_count = 8;
int sec_wait = 3;
*/
#if 1
int software_method(void)
{
  int last_exposure = 0;
  int eq2_wait_count = 0;
  int eq3_wait_count = 0; 
  int exposure, iridix, colortemp;
  
  if(verbose){
    printf("Starting software method using \n");
    printf("jitter_percent = %d\n", jitter_percent);
    printf("eq1_user_exposure = %d\n", eq1_user_exposure);
    printf("eq2_user_exposure = %d\n", eq2_user_exposure);
    printf("eq2_user_iridix = %d\n", eq2_user_iridix);
    printf("eq2_count = %d\n", eq2_count);
    printf("eq3_user_wb = %d\n", eq3_user_wb);
    printf("eq3_user_iridix = %d\n", eq3_user_iridix);
    printf("eq3_count = %d\n", eq3_count);
    printf("sec_wait = %d\n", sec_wait);
  }
  // Start with day
  nightModeEnabled = false;
  updateNightMode();

  while(1){
    double exp_diff;
    sleep(1);
    if(readIspInfo(&exposure, &iridix, &colortemp) == 3){
      if(verbose >= 2) printf("(%d, %d, %d)\n", exposure, iridix, colortemp);
      // Jitter equation
      if( (int)((fabs((double)exposure - (double)last_exposure)/(double)exposure)*100.0 - (double) jitter_percent) > 0  ){
        eq2_wait_count = 0;
        eq3_wait_count = 0;
        last_exposure = exposure;
        if(verbose >= 1) printf("jitter active\n");
        continue;
      }
      last_exposure = exposure;

      // Mode switch wait
      if(mode_switch_wait_count){
        mode_switch_wait_count--;
        if(verbose >= 1) printf("mode switch wait active\n");
        continue;
      }

      // Eq1
      if(nightModeEnabled == false && exposure > eq1_user_exposure){
        if(verbose >= 1) printf("Switching to night new exp=%d\n", exposure);
        nightModeEnabled = true;
        eq2_wait_count = 0;
        eq3_wait_count = 0;
        updateNightMode();
        continue;
      }

      // Eq2
      if(nightModeEnabled == true && exposure < eq2_user_exposure && iridix < eq2_user_iridix){
        eq2_wait_count++;
        if(eq2_wait_count > eq2_count){
          if(verbose >= 1) printf("Eq2 switching to day mode exp=%d iridix=%d\n", exposure, iridix);
          nightModeEnabled = false;
          updateNightMode();
          continue;
        }else{
          if(verbose >= 1) printf("Eq2 wait_count active exp=%d iridix=%d\n", exposure, iridix);
        }
      }else{
        eq2_wait_count = 0;
      }

      // Eq3
      if(nightModeEnabled == true && colortemp < eq3_user_wb && iridix > eq3_user_iridix){
        eq3_wait_count++;
        if(eq3_wait_count > eq3_count){
          if(verbose >= 1) printf("Eq3 switching to day mode wb=%d iridix=%d\n", colortemp, iridix);
          nightModeEnabled = false;
          updateNightMode();
          continue;
        }else{
          if(verbose >= 1) printf("Eq3 wait_count active wb=%d iridix=%d\n", colortemp, iridix);
        }
      }else{
        eq3_wait_count = 0;
      }
    }else{
      // Unable to read isp_info
      fprintf(stderr, "Unable to read isp_info\n");
    }
  }
}
#else
int software_method(void)
{
  int exposure, iridix, colortemp;
  int last_exposure=-1;
  int night_exposure, night_iridix, night_temp;
  int night_mode_wait_count, iridx_wait_count;
  int exp_fall_count;
  double day_exposure_drop_f = 1.0 - (double)day_exposure_drop/100.0;
  States state;

  // Start with day
  nightModeEnabled = false;
  updateNightMode();
  state = DAY_MODE;

  while(1){
    if(readIspInfo(&exposure, &iridix, &colortemp) == 3){
      if(last_exposure == -1) last_exposure = exposure;
      if( (exposure < last_exposure && ((last_exposure - exposure)*100)/last_exposure >= exposure_tolerance) ||
          (exposure > last_exposure && ((exposure - last_exposure)*100)/exposure >= exposure_tolerance) ){
        //Exposure chaging rapidly. Wait for it to get stablized
        if(verbose >= 2) printf("Exposure falling by %d%% to %d count %d\n", ((last_exposure - exposure)*100)/last_exposure, exposure, exp_fall_count);
        last_exposure = exposure;
        exp_fall_count++;
        if(exp_fall_count < max_faill_count){
          // If it has been changing for while, just take it
          night_mode_wait_count = 0;
          continue;
        }
      }
      last_exposure = exposure;
      exp_fall_count = 0;
      /*if(colortemp+day_color_temp <= night_temp){
        if(state != DAY_MODE){
          // Check color temp for any state in night mode
          if(verbose >= 1) printf("Colortemp %d\n", colortemp);
            nightModeEnabled = false;
            updateNightMode();
            state = DAY_MODE;
          }
      }*/
      switch(state){
        case DAY_MODE:
          //Look for very high exposure setting (max is 1283554)
          if(exposure > 1200000 && iridix <= 30){
              if(verbose >= 1) printf("Exposure %d, %d>\n", exposure, iridix);
              nightModeEnabled = true;
              updateNightMode();
              state = NIGHT_MODE_START;
              sleep(3); // Extra sleep
          }
          break;
        case NIGHT_MODE_START:
          if(iridix >= 30){
            // Something is likely covering us
            if(verbose >= 1) printf("Iridx high %d, %d>\n", exposure, iridix);
            night_iridix = iridix;
            iridx_wait_count = 0;
            state = WAIT_IRIDX;
            break;
          }
          // save readings as initial night values
          night_exposure = exposure;
          night_iridix = iridix;
          night_temp = colortemp;
          if(verbose >= 1) printf("night mode start %d, %d, %d\n", exposure, iridix, colortemp);
          state = NIGHT_MODE_WAIT;
          night_mode_wait_count = 0;
          iridx_wait_count = 0;
          break;
        case WAIT_IRIDX:
            // Wait for Iridx to drop to below 30
            if(verbose >= 2) printf("WAIT_IRIDX %d, %d %d\n", iridix, night_iridix, iridx_wait_count);
            if(iridix <= (night_iridix-wait_iridix_diff)){
              if(verbose >= 1) printf("Iridx dropped %d, %d>\n", exposure, iridix);
              nightModeEnabled = false;
              updateNightMode();
              state = DAY_MODE;
            }else{
              iridx_wait_count++;
              if(iridx_wait_count >= max_iridx_wait){
                // Give up and switch
                if(verbose >= 1) printf("Iridx wait too long %d, %d>\n", exposure, iridix);
                nightModeEnabled = false;
                updateNightMode();
                state = DAY_MODE;
              }
            }
            break;
        case NIGHT_MODE_WAIT:
          if(colortemp+day_color_temp <= night_temp){
            if(verbose >= 1) printf("Colortemp %d\n", colortemp);
              nightModeEnabled = false;
              updateNightMode();
              state = DAY_MODE;
          }
          if(verbose >= 2) printf("Exposure changed by %d%% to %d,%d (%d) %d %d %6f\n", ((last_exposure - exposure)*100)/last_exposure, exposure, iridix, night_exposure, night_iridix, colortemp, (day_exposure_drop_f*(double)night_exposure));
          if( (double)exposure < (day_exposure_drop_f*(double)night_exposure) ){
            if( iridix < (night_iridix + 4) ){
              // Exposure droped by 5% without increasing iridix by much
              iridx_wait_count = 0;
              if(night_mode_wait_count < max_night_ex_wait){
                night_mode_wait_count++;
                if(verbose >= 2) printf("Exposure count %d <%d to %d> Iridix <%d to %d>\n", night_mode_wait_count, night_exposure, exposure, night_iridix, iridix);
                sleep(1);
              }else{
                // Day mode
                if(verbose >= 1) printf("Exposure <%d to %d> Iridix <%d to %d>\n", night_exposure, exposure, night_iridix, iridix);
                nightModeEnabled = false;
                updateNightMode();
                state = DAY_MODE;
              }
            }else{
              // Exposure droped by 2K but iridix is up by 2 or more
              iridx_wait_count++;
              night_mode_wait_count = 0;
              sleep(1);
              if(verbose) printf("Iridx count %d <%d to %d> Iridix <%d to %d>\n", iridx_wait_count, night_exposure, exposure, night_iridix, iridix);
              if(iridx_wait_count > max_iridx_wait){
                // Whatever is causing iridx is not moving. Lets assume day mode
                if(verbose) printf("Iridix <%d to %d>\n", night_iridix, iridix);
                nightModeEnabled = false;
                updateNightMode();
                state = DAY_MODE;
              }
            }
          }else if(exposure > night_exposure){
            if(verbose >= 2) printf("Exposure up to %d from %d\n", exposure, night_exposure);
            // Reset night numbers
            night_exposure = exposure;
            last_exposure = exposure;
            state = NIGHT_MODE_WAIT;
            night_mode_wait_count = 0;
            exp_fall_count = 0;
          }else{
            night_mode_wait_count = 0;
          }
          break;
        default:
          fprintf(stderr, "Invalid state %d\n", state);
      }
    }else{
      // Unable to read isp_info
      fprintf(stderr, "Unable to read isp_info\n");
    }
    sleep(1);
  }
  return -1;
}
#endif
