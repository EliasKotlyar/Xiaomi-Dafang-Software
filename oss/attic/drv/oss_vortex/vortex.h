/*
 * Purpose: Definitions for the Vortex driver
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
#include "oss_vortex_cfg.h"
#include "midi_core.h"
#include <ac97.h>
#include <oss_pci.h>

#define MAX_PORTC 2

typedef struct
{
  int open_mode;
  int speed, bits, channels;
  int voice_chn;
  int audio_enabled;
  int trigger_bits;
  int audiodev;
}
vortex_portc;


typedef struct vortex_devc
{
  oss_device_t *osdev;
  unsigned int bar0addr;
  unsigned int *bar0virt;
  volatile unsigned int *dwRegister;
  unsigned int bar0_size;
  int irq;
  char *name;
  int id;			/* Vortex1 or Vortex2 */
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;

  /* Block pointers */
  oss_native_word global_base;
  oss_native_word dma_base;
  oss_native_word midi_base;
  oss_native_word fifo_base;
  oss_native_word adbarb_block_base;
  oss_native_word serial_block_base;
  oss_native_word parallel_base;
  oss_native_word src_base;

  /* Mixer parameters */
  ac97_devc ac97devc;
  int mixer_dev;

  /* Audio parameters */
  vortex_portc portc[MAX_PORTC];
  int open_mode;
  int origbufsize;

  oss_native_word sr_active;
  unsigned int tail_index[32];
  unsigned int dst_index[32];
  unsigned char sr_list[256];
  unsigned int dst_routed[256];

  /* MIDI */
  int midi_opened;
  int midi_dev;
  oss_midi_inputbyte_t midi_input_intr;
}
vortex_devc;

#define READL(a)	(devc->dwRegister[a>>2])
#define WRITEL(a, d)	(devc->dwRegister[a>>2]=d)
