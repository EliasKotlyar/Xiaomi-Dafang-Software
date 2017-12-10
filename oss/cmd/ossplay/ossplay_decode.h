#ifndef OSSPLAY_DECODE_H
#define OSSPLAY_DECODE_H

#include "ossplay.h"

#define MAC_IMA_BLKLEN		34
/*
 * ima4 block length in AIFC files. Qt has "stsd" chunk which can change this,
 * but I know of no AIFC equivalent.
 */

typedef struct verbose_values {
  char tstring[20];
  double secs;
  double next_sec;
  double secs_timer2;
  double next_sec_timer2;
  double tsecs;
  double constant;
  int format;
}
verbose_values_t;

errors_t decode_sound (dspdev_t *, int, big_t, int, int, int, void *);
errors_t encode_sound (dspdev_t *, fctypes_t, const char *, int, int, int,
                       double);
int get_db_level (const unsigned char *, ssize_t, int);
verbose_values_t * setup_verbose (int, double, double);

#endif
