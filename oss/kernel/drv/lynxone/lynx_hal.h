
#include "midi_core.h"
/*
 *
 * This file is part of Open Sound System.
 *
 * Copyright (C) 4Front Technologies 1996-2008.
 *
 * This this source file is released under GPL v2 license (no other versions).
 * See the COPYING file included in the main directory of this source
 * distribution for the license terms and conditions.
 *
 */

struct lynx_devc;

typedef struct
{
  int type;
#define TY_AOUT			1
#define TY_AIN			2
#define TY_DOUT			3
#define TY_DIN			4

  int open_mode;
  int device_prepared, device_triggered;
  int audiodev;
  int local_qlen;
  int channels, bits;
  int samplewidth;

  int hw_device;

  struct lynx_devc *devc;
}
lynx_portc;

typedef struct
{
  struct lynx_devc *devc;
  int hw_indev, hw_outdev;
  int midi_opened;
  int midi_dev;
  oss_midi_inputbyte_t midi_input_intr;
}
lynx_midic;

typedef struct lynx_devc
{
  oss_device_t *osdev;
  ADAPTER adapter;
  oss_mutex_t mutex;

  unsigned int plx_addr;
  unsigned int base_addr;
  unsigned int fpga_addr;

  unsigned char *plx_ptr;
  unsigned char *base_ptr;
  unsigned char *fpga_ptr;
  int irq;

  int mixer_dev;

  int speed;
  int locked_speed;
  int ratelock;
  int first_dev;
  int clock_source;
  int digital_format;
  lynx_portc portc[4];
  int nrdevs;
  int open_devices;
  int rate_fixed;
  int qsrc;
  int master_dev;

  int leftvol, rightvol;
  int left, right;
  int trim;

/*
 * MIDI
 */

  lynx_midic midic[2];

/*
 * Monitoring
 */
  int monitorsrc;		/* Analogin, digitalin, analogout, digitalout */
  int analogmon, digitalmon;	/* ON/OFF */
}
lynx_devc;
