/*
 * Purpose: Default MIDI timer (using system clock)
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

typedef struct
{
  unsigned long long start_time;
  timeout_id_t timeout_id;
  oss_midi_wait_callback_t timer_callback;
  void *arg;
} deftmr_timerc_t;

static unsigned long long tick_scale = 1;

static int
deftmr_create_instance (int timer_dev, int driver_dev)
{
  deftmr_timerc_t *timerc;

  if ((timerc = KERNEL_MALLOC (sizeof (*timerc))) == NULL)
    return OSS_ENOMEM;

  memset (timerc, 0, sizeof (*timerc));

  oss_timer_devs[timer_dev]->timerc = timerc;

  return 0;
}

static void
deftmr_free_instance (int timer_dev, int driver_dev)
{
  deftmr_timerc_t *timerc;

  timerc = oss_timer_devs[timer_dev]->timerc;
  untimeout (timerc->timeout_id);

  if (timerc != NULL)
    KERNEL_FREE (timerc);
  oss_timer_devs[timer_dev]->timerc = NULL;
}

static int
deftmr_attach_client (int timer_dev, int mididev)
{
  return 0;
}

static void
deftmr_detach_client (int timer_dev, int mididev)
{
}

static int
deftmr_ioctl (int timer_dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static int
deftmr_set_source (int timer_dev, int source)
{
  return source;
}

static int
deftmr_set_tempo (int timer_dev, int tempo)
{
  return tempo;
}

static int
deftmr_set_timebase (int timer_dev, int timebase)
{
  return timebase;
}
static int
deftmr_start (int timer_dev, oss_uint64_t tick)
{
  deftmr_timerc_t *timerc;

  timerc = oss_timer_devs[timer_dev]->timerc;

  if (timerc == NULL)
    {
      cmn_err (CE_WARN, "deftmr_start: timerc==NULL\n");
      return OSS_EIO;
    }

  timerc->start_time = GET_JIFFIES ();
  return 0;
}

static int
deftmr_stop (int timer_dev)
{
  deftmr_timerc_t *timerc;

  timerc = oss_timer_devs[timer_dev]->timerc;
  untimeout (timerc->timeout_id);

  return 0;
}

static int
deftmr_cont (int timer_dev)
{
  return 0;
}

static int
deftmr_wait (int timer_dev, unsigned long long time,
	     oss_midi_wait_callback_t callback, void *arg)
{
  deftmr_timerc_t *timerc;
  unsigned long long t;

  timerc = oss_timer_devs[timer_dev]->timerc;
  untimeout (timerc->timeout_id);

  t = ((oss_uint64_t) time + (tick_scale / 2)) / tick_scale;

  t += timerc->start_time;

  t -= GET_JIFFIES ();

  if (t < 0)
    {
      return 1;
    }

  timerc->timeout_id = timeout (callback, arg, t);

  return 0;
}

static unsigned long long
deftmr_get_current_time (int timer_dev)
{
  unsigned long long t;
  deftmr_timerc_t *timerc;

  timerc = oss_timer_devs[timer_dev]->timerc;
  /*
   * Compute time in system ticks since start of the timer
   */
  t = GET_JIFFIES ();
  if (t < timerc->start_time)
    return 0;
  t -= timerc->start_time;

  return t * tick_scale;	/* In microseconds */
}

oss_timer_driver_t default_midi_driver = {
  deftmr_create_instance,
  deftmr_free_instance,
  deftmr_attach_client,
  deftmr_detach_client,
  deftmr_ioctl,
  deftmr_wait,
  deftmr_get_current_time,
  deftmr_set_source,
  deftmr_set_tempo,
  deftmr_set_timebase,
  deftmr_start,
  deftmr_stop,
  deftmr_cont
};

void
attach_oss_default_timer (oss_device_t * osdev)
{
  int timer_dev;

  tick_scale = 1000000 / OSS_HZ;
  if ((timer_dev = oss_install_timer (OSS_TIMER_DRIVER_VERSION, "System timer", &default_midi_driver, sizeof (oss_timer_driver_t), 0,	/* Flags */
				      16,	/* max_instances */
				      1000000 / OSS_HZ,	/* Resolution */
				      NULL, osdev)) < 0)
    {
      cmn_err (CE_WARN, "Failed to install default MIDI timer\n");
      return;
    }
}

void
detach_oss_default_timer ()
{
}
