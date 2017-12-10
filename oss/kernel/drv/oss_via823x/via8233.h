/*
 * Purpose: Definitions for the via8233 driver
 */
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
#define VIA_VENDOR_ID		0x1106
#define VIA_8233_ID		0x3059
#define VIA_8233A_ID		0x7059

#define PLAY_SGD_NUM		1
#define REC_SGD_NUM		0
#define MAX_ENGINES		2
#define MAX_PORTC		2

#define SGD_SIZE		256
#define SGD_ALLOC		4096
#define SGD_EOL			0x80000000
#define SGD_FLAG		0x40000000

#define CODEC_TIMEOUT_COUNT     500
#define AC97CODEC     0x80	/*Access AC97 Codec */
#define IN_CMD        0x01000000	/*busy in sending */
#define STA_VALID     0x02000000	/*1:status data is valid */
#define CODEC_RD      0x00800000	/*Read CODEC status */
#define CODEC_INDEX   0x007F0000	/*Index of command register to access */
#define CODEC_DATA    0x0000FFFF	/*AC97 status register data */

typedef struct
{
  unsigned int phaddr;
  unsigned int flags;
}
SGD_entry;

struct via8233_portc;

typedef struct
{
  int sgd_num, base;
  oss_dma_handle_t sgd_dma_handle;
  int mode;
  SGD_entry *sgd;
  oss_native_word sgd_phys;
  int sgd_ptr;
  int prev_sgd;
  int cfrag;
  unsigned int prevpos;
  short frags[SGD_SIZE];
  struct via8233_portc *portc;
}
engine_desc;

typedef struct via8233_portc
{
  int audiodev;
  int open_mode;
  int mode_mask;
  int speed, bits, channels;
  engine_desc *play_engine, *rec_engine;
  int trigger_bits;
  int audio_enabled;
}
via8233_portc;


typedef struct via8233_devc
{
  oss_device_t *osdev;
  int chip_type;
#define CHIP_8233	0
#define CHIP_8233A	1
  char *chip_name;
  int multi_channel_active;
  oss_native_word base;
  int irq;
  int open_mode;
  oss_mutex_t mutex;		/* For normal locking */
  oss_mutex_t low_mutex;	/* For low level routines */
/*
 * Mixer
 */
  ac97_devc ac97devc;
  int mixer_dev;

  via8233_portc portc[MAX_PORTC];
  engine_desc engines[MAX_ENGINES];
}
via8233_devc;
