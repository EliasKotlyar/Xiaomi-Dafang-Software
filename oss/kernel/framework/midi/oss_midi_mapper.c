/*
 * Purpose: MIDI device mapper
 *
 * This code is used to bind applications using /dev/mixer to the actual
 * MIDI input and output ports.
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
#include "midi_core.h"

extern oss_mutex_t midi_mutex;

void
midi_mapper_init (oss_device_t * osdev)
{
}

void
midi_mapper_uninit (void)
{
}

int
midi_mapper_autobind (int client_dev, int mode)
{
  int dev;
  oss_midi_client_t *client = oss_midi_clients[client_dev];

  cmn_err (CE_CONT, "MIDI autobind %d\n", client_dev);

  for (dev = 0; dev < num_mididevs; dev++)
    {
      int err;

      if (midi_devs[dev]->open_mode)	/* Busy */
	continue;

      if ((mode & OPEN_WRITE) && !(midi_devs[dev]->flags & MFLAG_OUTPUT))
	continue;

      if ((mode == OPEN_READ) && !(midi_devs[dev]->flags & MFLAG_INPUT))
	continue;

      if ((err = midi_devs[dev]->d->open (dev, client->open_mode,
					  oss_midi_input_byte,
					  oss_midi_input_buf,
					  oss_midi_output_intr)) < 0)
	{
	  /* The device seems to think it's busy */
	  continue;
	}

      if (midi_devs[dev]->open_mode == 0)	/* Not busy */
	{

	  midi_devs[dev]->open_mode = mode;
	  client->mididev = midi_devs[dev];

	  if (mode & OPEN_READ)
	    {
	      client->mididev->in_queue =
		midi_queue_alloc (client->mididev->osdev, "qread");
	      if (client->mididev->in_queue == NULL)
		{
		  cmn_err (CE_WARN,
			   "Failed to allocate MIDI input queue(2)\n");
		  midi_devs[dev]->d->close (dev, mode);
		  midi_devs[dev]->open_mode = 0;
		  return OSS_ENOMEM;

		}
	    }

	  if (mode & OPEN_WRITE)
	    {
	      client->mididev->out_queue =
		midi_queue_alloc (client->mididev->osdev, "qwrite");
	      if (client->mididev->out_queue == NULL)
		{
		  cmn_err (CE_WARN,
			   "Failed to allocate MIDI output queue(2)\n");
		  midi_devs[dev]->d->close (dev, mode);
		  midi_devs[dev]->open_mode = 0;
		  return OSS_ENOMEM;

		}
	    }

	  oss_midi_set_defaults (client->mididev);

	  cmn_err (CE_CONT, "Bound to %d/%s\n", dev, midi_devs[dev]->name);
	  return dev;

	}
    }

  return OSS_EBUSY;
}
