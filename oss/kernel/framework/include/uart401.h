#ifndef UART401_H
#define UART401_H
/*
 * Purpose: Definitions for the MPU-401 (UART) support library
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

#ifndef MIDI_CORE_H
#include "midi_core.h"
#endif

/*
 * Purpose: Definitions for the MPU-401 UART driver
 */

typedef struct uart401_devc
{
  oss_native_word base;
  int irq;
  oss_device_t *osdev;
  int running;
  oss_mutex_t mutex;
  oss_midi_inputbuf_t save_input_buffer;
  int opened, disabled;
  volatile unsigned char input_byte;
  int my_dev;
  int share_irq;
}
uart401_devc;

extern int uart401_init (uart401_devc * devc, oss_device_t * osdev, int base,
			 char *name);
extern void uart401_irq (uart401_devc * devc);
extern void uart401_disable (uart401_devc * devc);
#endif
