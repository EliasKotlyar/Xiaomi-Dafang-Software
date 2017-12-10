#ifndef _DDKSAMPLE_H
#define _DDKSAMPLE_H

// Audio engine (port) control structure.

#define MAX_SUBDEVICE	4

typedef struct
{
  int dev;
  int open_mode;		/* OPEN_READ | OPEN_WRITE */

  int speed;
  int channels;
  int format, bits;

  int prepare_flags;
  int audio_active;

  int timeout_value;
  timeout_id_t timeout_id;

  int left_volume, right_volume;
  int mute;
#define DDKSAMPLE_MAX_VOL	100

  int left_peak, right_peak;
} ddksample_portc;

// Device control structure
typedef struct
{
  oss_device_t *osdev;
  kmutex_t mutex;

  int mixer_dev;
  int mainvol_left, mainvol_right;

  int num_portc;
  ddksample_portc portc[MAX_SUBDEVICE];
  int audio_open_count;		/* Number of channels currently opened */

} ddksample_devc;

extern int ddksample_mixer_init (ddksample_devc * devc);
extern int ddksample_audio_init (ddksample_devc * devc);
extern void ddksample_do_math (ddksample_portc * portc, void *buf, int len);

#endif
