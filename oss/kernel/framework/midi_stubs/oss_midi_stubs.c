/*
 * Purpose: Placeholder functions for MIDI support for systems that have MIDI support excluded from OSS.
 *
 * This source file contains empty stubs for the MIDI related functions
 * referenced by other parts of OSS.
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

#include "oss_config.h"
#ifndef CONFIG_OSS_MIDI
#include "midi_core.h"
#include "midiparser.h"

#define MDB(x)

oss_mutex_t midi_mutex;
tdev_t *oss_timer_devs[MAX_TIMER_DEV] = { NULL };
int oss_num_timers = 0;

/*
 * List of MIDI devices.
 */
mididev_t *__midi_devs[1], **midi_devs = __midi_devs;
int num_mididevs = 0;

/*
 * List of MIDI clients (/dev/midi* device files).
 */
int oss_num_midi_clients = 0;
oss_midi_client_t *oss_midi_clients[MAX_MIDI_CLIENTS] = { NULL };

/*ARGSUSED*/
void
oss_midi_init (oss_device_t * osdev)
{
}

/*ARGSUSED*/
int
oss_install_mididev (int version,
		     char *id, char *name,
		     midi_driver_t * d, int driver_size,
		     unsigned int flags, void *devc, oss_device_t * osdev)
{
  return 0;
}

/*ARGSUSED*/
void
install_vmidi (oss_device_t * osdev)
{
}

/*ARGSUSED*/
midiparser_common_p
midiparser_create (midiparser_callback_t callback, void *context)
{
  return NULL;
}

/*ARGSUSED*/
void
midiparser_unalloc (midiparser_common_p common)
{
}

/*ARGSUSED*/
void
midiparser_input (midiparser_common_p synth, unsigned char data)
{
}
#endif
