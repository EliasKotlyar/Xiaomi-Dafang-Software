/*
 * Purpose: MIDI timer support.
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

tdev_t *oss_timer_devs[MAX_TIMER_DEV] = { NULL };
int oss_num_timers = 0;

oss_timer_driver_t *oss_timer_drivers[MAX_TIMER_DEV] = { NULL };
int oss_num_timer_drivers = 0;

int
oss_install_timer (int driver_version,
		   char *name,
		   oss_timer_driver_t * d, int driver_size,
		   unsigned int flags,
		   int max_instances,
		   int resolution, void *devc, oss_device_t * osdev)
{
  oss_timer_driver_t *op;
  int num;

  if (driver_version != OSS_TIMER_DRIVER_VERSION)
    {
      cmn_err (CE_WARN, "Incompatible timer driver version %d\n",
	       driver_version);
      return OSS_EINVAL;
    }

  if (driver_size != sizeof (*op))
    {
      cmn_err (CE_WARN, "Incompatible timer driver size %d\n", driver_size);
      return OSS_EINVAL;
    }

  if (oss_num_timer_drivers >= MAX_TIMER_DEV)
    {
      cmn_err (CE_WARN, "Too many timer drivers\n");
      return OSS_ENOMEM;
    }

  if ((op = PMALLOC (osdev, sizeof (*op))) == NULL)
    {
      cmn_err (CE_WARN, "Out of memory (oss_install_timer)\n");
      return OSS_ENOMEM;
    }

  memcpy (op, d, driver_size);
  op->flags = flags;
  op->devc = devc;
  op->osdev = osdev;
  MUTEX_INIT (op->osdev, op->mutex, MH_FRAMEW + 2);

  op->max_instances = max_instances;
  op->num_instances = 0;
  strncpy (op->name, name, sizeof (op->name));
  op->name[sizeof (op->name) - 1] = 0;
  num = oss_num_timer_drivers++;
  op->driver_dev = num;
  op->resolution = resolution;
  oss_timer_drivers[num] = op;

  return 0;
}

int
oss_timer_create_instance (int driver_dev, mididev_t * mididev)
{
  oss_timer_driver_t *drv;
  tdev_t *tmr;
  oss_native_word flags;
  int num, i, err;

/* TODO: Create a mutex for all timers */

  if (driver_dev < 0 || driver_dev >= oss_num_timer_drivers)
    return OSS_ENXIO;

  drv = oss_timer_drivers[driver_dev];

  MUTEX_ENTER_IRQDISABLE (drv->mutex, flags);
  if (drv->num_instances >= drv->max_instances)
    {
      cmn_err (CE_CONT, "All instances of timer %d are in use (%d)\n",
	       driver_dev, drv->max_instances);

      MUTEX_EXIT_IRQRESTORE (drv->mutex, flags);
      return OSS_EBUSY;
    }
  drv->num_instances++;
  MUTEX_EXIT_IRQRESTORE (drv->mutex, flags);

  if ((tmr = KERNEL_MALLOC (sizeof (*tmr))) == NULL)
    {
      MUTEX_ENTER_IRQDISABLE (drv->mutex, flags);
      drv->num_instances--;
      MUTEX_EXIT_IRQRESTORE (drv->mutex, flags);
      return OSS_ENOMEM;
    }
  memset (tmr, 0, sizeof (*tmr));

  num = -1;

  for (i = 0; i < oss_num_timers; i++)
    if (oss_timer_devs[i] == NULL)
      {
	num = i;
	break;
      }

  if (num == -1)
    {
      if (oss_num_timers >= MAX_TIMER_DEV)	/* No timers available */
	{
	  cmn_err (CE_WARN, "No free timer instances\n");
	  KERNEL_FREE (tmr);
	  MUTEX_ENTER_IRQDISABLE (drv->mutex, flags);
	  drv->num_instances--;
	  MUTEX_EXIT_IRQRESTORE (drv->mutex, flags);
	  return OSS_EBUSY;
	}
      num = oss_num_timers++;
    }

/*
 * Init the timer instance
 */
  tmr->osdev = drv->osdev;
  MUTEX_INIT (tmr->osdev, tmr->mutex, MH_FRAMEW + 3);
  tmr->refcount = 0;
  tmr->d = drv;
  tmr->driver_dev = driver_dev;
  tmr->timer_dev = num;
  tmr->devc = drv->devc;
  tmr->name = drv->name;

  oss_timer_devs[num] = tmr;

  if (drv->create_instance != NULL)
    if ((err = drv->create_instance (num, driver_dev)) < 0)
      {
	oss_timer_devs[num] = NULL;
	MUTEX_CLEANUP (tmr->mutex);
	KERNEL_FREE (tmr);
	MUTEX_ENTER_IRQDISABLE (drv->mutex, flags);
	drv->num_instances--;
	MUTEX_EXIT_IRQRESTORE (drv->mutex, flags);
	return err;
      }

  return num;
}

#define BITS_PER_ENTRY (sizeof(unsigned int)*8)

int
oss_timer_attach_client (int timer_dev, mididev_t * mididev)
{
  tdev_t *tmr;
  int err;
  int mdev = mididev->dev;
  oss_native_word flags;

  if (timer_dev < 0 || timer_dev >= oss_num_timers)
    return OSS_ENXIO;

  tmr = oss_timer_devs[timer_dev];
  if (tmr == NULL)
    {
      cmn_err (CE_WARN, "tmr==NULL\n");
      return OSS_EIO;
    }
  if (tmr->d == NULL)
    {
      cmn_err (CE_WARN, "tmr->d==NULL\n");
      return OSS_EIO;
    }

  if (tmr->d->attach_client != NULL)
    if ((err = tmr->d->attach_client (timer_dev, mididev->dev)) < 0)
      {
	return err;
      }

  MUTEX_ENTER_IRQDISABLE (tmr->mutex, flags);
  tmr->refcount++;
  tmr->midimap[mdev / BITS_PER_ENTRY] |= (1 << (mdev % BITS_PER_ENTRY));
  tmr->midi_times[mdev] = 0;
  MUTEX_EXIT_IRQRESTORE (tmr->mutex, flags);

  return 0;
}

void
oss_timer_detach_client (int timer_dev, mididev_t * mididev)
{
  oss_native_word flags;
  tdev_t *tmr;
  oss_timer_driver_t *drv;
  int refcount;
  int mdev;

  mdev = mididev->dev;

  if (timer_dev < 0 || timer_dev >= oss_num_timers)
    return;

  tmr = oss_timer_devs[timer_dev];
  if (tmr == NULL)
    return;

  if (tmr->d->detach_client != NULL)
    tmr->d->detach_client (timer_dev, mididev->dev);

  MUTEX_ENTER_IRQDISABLE (tmr->mutex, flags);
  refcount = --tmr->refcount;
  tmr->midimap[mdev / BITS_PER_ENTRY] &= ~(1 << (mdev % BITS_PER_ENTRY));
  tmr->midi_times[mdev] = 0;
  MUTEX_EXIT_IRQRESTORE (tmr->mutex, flags);

  if (refcount <= 0)
    {
      drv = tmr->d;

      oss_timer_stop (timer_dev);
      if (tmr->d->free_instance != NULL)
	tmr->d->free_instance (timer_dev, tmr->driver_dev);

      oss_timer_devs[timer_dev] = NULL;
      MUTEX_CLEANUP (tmr->mutex);
      KERNEL_FREE (tmr);

      MUTEX_ENTER_IRQDISABLE (drv->mutex, flags);
      drv->num_instances--;
      if (drv->num_instances < 0)
	{
	  cmn_err (CE_WARN, "Driver instances < 0\n");
	  drv->num_instances = 0;
	}
      MUTEX_EXIT_IRQRESTORE (drv->mutex, flags);
    }
}

static int update_timer_state (tdev_t * tmr);

static void
timer_callback (void *arg)
{
  int i, x;
  oss_uint64_t t;

  tdev_t *tmr = arg;

  t = tmr->next_time;
  tmr->prev_time = t;
  tmr->prev_tick = tmr->next_tick;

  for (x = 0; x < sizeof (tmr->midimap) / sizeof (unsigned int); x++)
    if (tmr->midimap[x] != 0)
      for (i = x * BITS_PER_ENTRY;
	   i < (x + 1) * BITS_PER_ENTRY && x < num_mididevs; i++)
	if (tmr->midimap[i / BITS_PER_ENTRY] & (1 << (i % BITS_PER_ENTRY)))
	  if (tmr->midi_times[i] > 0 && tmr->midi_times[i] <= t)
	    {
	      /*
	       * Timer for this MIDI device has expired. Wake up the device.
	       */
	      tmr->midi_times[i] = 0;
	      if (tmr->callback != NULL)
		{
		  oss_native_word d = i;
		  tmr->callback ((void *) d);
		}
	    }

  update_timer_state (tmr);
}

static int
update_timer_state (tdev_t * tmr)
{
  int i, x;
  int state;
  oss_uint64_t t;
  oss_midi_time_t tick = 0;

  if (tmr == NULL)
    return 0;

  if (tmr->tempo < 1)
    tmr->tempo = 60;
  if (tmr->timebase < 1)
    tmr->timebase = OSS_HZ;

  t = 0xffffffffffffffffULL;	/* Largest possible time */
  for (x = 0; x < sizeof (tmr->midimap) / sizeof (unsigned int); x++)
    if (tmr->midimap[x] != 0)
      for (i = x * BITS_PER_ENTRY;
	   i < (x + 1) * BITS_PER_ENTRY && x < num_mididevs; i++)
	if (tmr->midimap[i / BITS_PER_ENTRY] & (1 << (i % BITS_PER_ENTRY)))
	  if (tmr->midi_times[i] != 0)
	    {
	      if (tmr->midi_times[i] < t)
		{
		  t = tmr->midi_times[i];
		  tick = tmr->midi_ticks[i];
		}
	    }

  if (t == 0xffffffffffffffffULL)	/* No events queued */
    return 1;

  tmr->next_time = t;
  tmr->next_tick = tick;

  if (tmr->d->wait == NULL)
    {
      cmn_err (CE_WARN, "No wait method for timer\n");
      return 1;
    }

  state = tmr->d->wait (tmr->timer_dev, t, timer_callback, tmr);

  if (state == 1)
    {
      timer_callback (tmr);
      return 1;
    }

  return 0;
}

int
oss_timer_set_tempo (int timer_dev, int tempo)
{
  tdev_t *tmr;
  int err;

  if (timer_dev < 0 || timer_dev >= oss_num_timers)
    return OSS_ENXIO;

  tmr = oss_timer_devs[timer_dev];
  if (tmr == NULL)
    return OSS_ENXIO;
  tmr->pending_tempo = tempo;

  {
    tmr->tempo = tmr->pending_tempo;

    tmr->bookmark_time = tmr->prev_time;
    tmr->bookmark_tick = tmr->prev_tick;
  }

  if (tmr->d->set_tempo != NULL)
    if ((err = tmr->d->set_tempo (timer_dev, tempo)) < 0)
      return err;

  return 0;
}

int
oss_timer_set_timebase (int timer_dev, int timebase)
{
  tdev_t *tmr;
  int err;

  if (timer_dev < 0 || timer_dev >= oss_num_timers)
    return OSS_ENXIO;

  tmr = oss_timer_devs[timer_dev];
  if (tmr == NULL || tmr->d->set_timebase == NULL)
    return OSS_ENXIO;
  tmr->timebase = timebase;
  if ((err = tmr->d->set_timebase (timer_dev, timebase)) < 0)
    return err;

  return 0;
}

int
oss_timer_wait_time (int timer_dev, int midi_dev, int latency,
		     oss_midi_time_t time, oss_midi_wait_callback_t callback,
		     unsigned short options)
{
  tdev_t *tmr;
  oss_uint64_t t;
  unsigned int tick;

  if (time == 0)
    return 1;

  if (latency < 0)
    latency = 0;

  if (timer_dev < 0 || timer_dev >= oss_num_timers)
    return 0;

  if (midi_dev < 0 || midi_dev >= num_mididevs)
    return 0;

  tmr = oss_timer_devs[timer_dev];
  if (tmr == NULL)
    return 0;

  tmr->callback = callback;

  if (!tmr->run_state)
    oss_timer_start (timer_dev);

  t = tick = time;

  if (!(options & MIDI_OPT_USECTIME))
    {
      /*
       * Convert MIDI time to absolute (usecs)
       */
      time -= tmr->bookmark_tick;

      t =
	60000000LL * (oss_uint64_t) time / (oss_uint64_t) (tmr->tempo *
							   tmr->timebase);

      t += tmr->bookmark_time;
    }

  if (t >= latency)
    t -= latency;

  if (t <= tmr->prev_time)	/* Already expired */
    {
      return 1;
    }

  tmr->midi_times[midi_dev] = t;
  tmr->midi_ticks[midi_dev] = tick;

  return update_timer_state (tmr);
}

oss_midi_time_t
oss_timer_get_current_time (int timer_dev, int latency, int use_abs)
{
  tdev_t *tmr;
  oss_uint64_t t;

  if (latency < 0)
    latency = 0;

  if (timer_dev < 0 || timer_dev >= oss_num_timers)
    return 0;

  tmr = oss_timer_devs[timer_dev];
  if (tmr == NULL || tmr->d->get_current_time == NULL)
    return 0;

  if (tmr->run_state == 0)
    return 0;

  t = tmr->d->get_current_time (timer_dev);

  if (!use_abs)
    {
      /* 
       * Convert from absolute (usec) time to relative (MIDI).
       */

      if (t < tmr->bookmark_time)
	return 0;

      t -= tmr->bookmark_time;
      t = 60000000LL * t / (oss_uint64_t) (tmr->tempo * tmr->timebase);
      t = t * (oss_uint64_t) (tmr->tempo * tmr->timebase);

      t = (t + 30000000LL) / 60000000LL;

      t += tmr->bookmark_tick;
    }

  return t;
}

int
oss_timer_is_running (int timer_dev)
{
  tdev_t *tmr;

  if (timer_dev < 0 || timer_dev >= oss_num_timers)
    return 0;

  tmr = oss_timer_devs[timer_dev];
  if (tmr == NULL)
    return 0;
  return tmr->run_state;
}

void
oss_timer_start (int timer_dev)
{
  tdev_t *tmr;

  if (timer_dev < 0 || timer_dev >= oss_num_timers)
    return;

  tmr = oss_timer_devs[timer_dev];
  if (tmr == NULL)
    return;
  tmr->pending_tempo = tmr->tempo;
  tmr->next_tick = 0;
  tmr->prev_tick = 0;
  tmr->next_time = 0;
  tmr->prev_time = 0;

  tmr->bookmark_tick = 0;
  tmr->bookmark_time = 0LL;

  tmr->run_state = 1;
  if (tmr->d->start != NULL)
    tmr->d->start (timer_dev, 0LL);
}

void
oss_timer_continue (int timer_dev)
{
  tdev_t *tmr;

  if (timer_dev < 0 || timer_dev >= oss_num_timers)
    return;

  tmr = oss_timer_devs[timer_dev];
  if (tmr == NULL)
    return;
  tmr->run_state = 1;
}

void
oss_timer_stop (int timer_dev)
{
  tdev_t *tmr;

  if (timer_dev < 0 || timer_dev >= oss_num_timers)
    return;

  tmr = oss_timer_devs[timer_dev];
  if (tmr == NULL)
    return;
  tmr->run_state = 0;
  if (tmr->d->stop != NULL)
    tmr->d->stop (timer_dev);
  tmr->callback = NULL;
}

int
oss_timer_get_timeout (int timer_dev, int midi_dev)
{
  tdev_t *tmr;
  oss_uint64_t t, scale;

  if (timer_dev < 0 || timer_dev >= oss_num_timers)
    return OSS_HZ;

  tmr = oss_timer_devs[timer_dev];
  if (tmr == NULL)
    return OSS_HZ;

  if (tmr->prev_time >= tmr->next_time)
    return OSS_HZ;

  scale = 1000000 / OSS_HZ;
  if (scale < 1)
    return OSS_HZ;

  t = tmr->next_time - tmr->prev_time;
  t /= scale;			/* Usecs to system clock ticks */

  t += 1;

  if (t > 1)
    return t;

  return OSS_HZ;
}

int
oss_init_timers (oss_device_t * osdev)
{
  return 0;
}

int
oss_uninit_timers (void)
{
  /* TODO: Cleanup all mutexes */
  return 0;
}
