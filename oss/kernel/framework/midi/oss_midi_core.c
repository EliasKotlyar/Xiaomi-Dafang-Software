/*
 * Purpose: MIDI support.
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

#define MDB(x)

oss_mutex_t midi_mutex;
static oss_device_t *osscore_osdev = NULL;

/*
 * List of MIDI devices.
 */
mididev_t **midi_devs = NULL;
int num_mididevs = 0;

/*
 * List of MIDI clients (/dev/midi* device files).
 */
int oss_num_midi_clients = 0;
oss_midi_client_t *oss_midi_clients[MAX_MIDI_CLIENTS] = { NULL };

static int
queue_midi_input (mididev_t * mididev, unsigned char *data, int len,
		  oss_native_word * wait_flags)
{
  int ret;

  midi_packet_header_t hdr, *hdrp = NULL;

  if (mididev->working_mode != MIDI_MODE_TRADITIONAL)
    {
      int use_abs = 0;

      hdrp = &hdr;

      memset (&hdr, 0, sizeof (hdr));
      hdr.magic = MIDI_HDR_MAGIC;
      hdr.event_type = MIDI_EV_WRITE;
      hdr.options |= MIDI_OPT_TIMED;

      if (mididev->working_mode == MIDI_MODE_TIMED_ABS)
	{
	  use_abs = 1;
	  hdr.options |= MIDI_OPT_USECTIME;
	}

      hdr.time =
	oss_timer_get_current_time (mididev->timer_dev, mididev->latency,
				    use_abs);
    }

  if ((ret = midi_queue_put (mididev->in_queue, data, len, hdrp)) < 0)
    {
      if (len == 1)
	cmn_err (CE_CONT, "/dev/midi%02d: Input buffer overflow\n",
		 mididev->dev);
      ret = 0;
    }

  oss_wakeup (mididev->in_wq, &mididev->mutex, wait_flags, POLLIN);

  return ret;
}

static void
mtc_generator (void *d)
{
#if 1
  oss_native_word dev = (oss_native_word) d;
  oss_native_word flags;
  unsigned char data, buf[2];
  mididev_p mididev = midi_devs[dev];
  int t;
  int step;
  int h, m, s, f;

  t = (int) (GET_JIFFIES () - mididev->mtc_t0);

  if (mididev->mtc_timebase < 1)
    return;
  mididev->mtc_timeout_id =
    timeout (mtc_generator, (void *) dev, OSS_HZ / 100);

  step = OSS_HZ / (mididev->mtc_timebase * 4);	/* System ticks per quarter frame */
  if (step < 1)
    {
      /* System timer has too low resolution  so do nothing */
      return;
    }

  t /= step;
  if (t <= mididev->mtc_prev_t)	/* Not enough time elapsed yet (for some reason) */
    {
      return;
    }

  mididev->mtc_prev_t = t;

  if (mididev->mtc_phase == 0)
    mididev->mtc_current = t;
  else
    t = mididev->mtc_current;

  t /= 4;			/* Ignore quarter frames */
  f = t % mididev->mtc_timebase;
  t /= mididev->mtc_timebase;

  s = t % 60;
  t /= 60;

  m = t % 60;
  t /= 60;

  h = t % 0x24;

  data = 0x00;

  switch (mididev->mtc_phase)
    {
    case 0:
      data = 0x00 | (f % 0x0f);
      break;
    case 1:
      data = 0x10 | ((f >> 4) % 0x0f);
      break;
    case 2:
      data = 0x20 | (s % 0x0f);
      break;
    case 3:
      data = 0x30 | ((s >> 4) % 0x0f);
      break;
    case 4:
      data = 0x40 | (m % 0x0f);
      break;
    case 5:
      data = 0x50 | ((m >> 4) % 0x0f);
      break;
    case 6:
      data = 0x60 | (h % 0x0f);
      break;
    case 7:
      data = 0x70 | ((h >> 4) % 0x03) | (mididev->mtc_codetype << 1);
      break;
    }

  mididev->mtc_phase = (mididev->mtc_phase + 1) % 8;

#if 0
  cmn_err (CE_CONT, "MTC = %02d:%02d:%02d %02d.x -> %02x\n",
	   h, m, s, f, data);
#endif

  MUTEX_ENTER_IRQDISABLE (mididev->mutex, flags);
  buf[0] = 0xf1;
  buf[1] = data;
  queue_midi_input (mididev, buf, 2, &flags);
  MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
#endif
}

int
oss_midi_input_byte (int dev, unsigned char data)
{
  oss_native_word flags;
  mididev_t *mididev;
  int ret;

  if (dev < 0 || dev >= num_mididevs)
    cmn_err (CE_PANIC, "MIDI driver bug - bad MIDI device %d\n", dev);

  mididev = midi_devs[dev];

  if (data == 0xf1)
    {
      mididev->mtc_timebase = -1;
    }
  MUTEX_ENTER_IRQDISABLE (mididev->mutex, flags);
  ret = queue_midi_input (mididev, &data, 1, &flags);
  MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);

  return ret;
}

int
oss_midi_input_buf (int dev, unsigned char *data, int len)
{
  oss_native_word flags;
  mididev_t *mididev;
  int ret, n = 0;

  if (dev < 0 || dev >= num_mididevs)
    cmn_err (CE_PANIC, "MIDI driver bug - bad MIDI device %d\n", dev);

  mididev = midi_devs[dev];

  MUTEX_ENTER_IRQDISABLE (mididev->mutex, flags);

  while (len > 0)
    {
      int i, l = len;

      if (l > MIDI_PAYLOAD_SIZE)
	l = MIDI_PAYLOAD_SIZE;


      for (i = 0; i < l; i++)
	{
	  /* Turn off MTC generation if MTC data is received */
	  if (data[i] == 0xf1)
	    {
	      mididev->mtc_timebase = -1;
	    }
	}
      ret = queue_midi_input (mididev, data, l, &flags);
      if (ret < 0)
	{
	  ret = 0;
	  cmn_err (CE_WARN, "MIDI input bytes dropped\n");
	}

      if (ret == 0)		/* Cannot queue more bytes */
	break;

      len -= ret;
      data += ret;
      n += ret;
    }

  MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);

  return n;
}

static void
oss_midi_input_event (int dev, unsigned char *data, int len)
{
  oss_native_word flags;
  mididev_t *mididev;
  int ret, n = 0, i;

  if (dev < 0 || dev >= num_mididevs)
    cmn_err (CE_PANIC, "MIDI driver bug - bad MIDI device %d\n", dev);

  mididev = midi_devs[dev];

  MUTEX_ENTER_IRQDISABLE (mididev->mutex, flags);

  for (i = 0; i < len; i += 4)
    {
      int l;
      unsigned char *d = data + i;

      switch (d[0] & 0x0f)
	{
	case 0x0:		/* End marker */
	  MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
	  return;
	  break;

	case 0x2:		/* Two byte real time messages TODO? */
	  l = 2;
	  break;

	case 0x4:		/* Sysex start/continuation */
	  l = 3;
	  break;

	case 0x5:		/* Sysex termination records */
	case 0x6:
	case 0x7:
	  l = (d[0] & 0x0f) - 4;
	  break;

	case 0x8:
	case 0x9:
	case 0xa:
	case 0xb:
	case 0xe:
	  l = 3;
	  break;

	case 0x0c:
	case 0x0d:
	  l = 2;
	  break;

	case 0xf:		/* System real time messages */
	  l = 1;
	  break;

	default:
	  l = 3;
	}

      if (l == 0)		/* Nothing to send */
	continue;

      if (l > MIDI_PAYLOAD_SIZE)
	l = MIDI_PAYLOAD_SIZE;

      /*
       * Ignore active sensing
       */
      if (d[1] == 0xfe)
	continue;

      ret = queue_midi_input (mididev, d + 1, l, &flags);
      if (ret < 0)
	{
	  ret = 0;
	  cmn_err (CE_WARN, "MIDI input bytes dropped\n");
	}

      if (ret == 0)		/* Cannot queue more bytes */
	break;

      len -= ret;
      data += ret;
      n += ret;
    }

  MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);

  return;
}

static void oss_midi_callback (void *arg);
static int setup_tempo (mididev_t * mididev);

void
oss_midi_copy_timer (int dev, int source_dev)
{
  mididev_t *mididev;
  int err;

  if (dev < 0 || dev >= num_mididevs)
    return;
  mididev = midi_devs[dev];

  if (mididev->timer_dev >= 0)	/* Detach the old client */
    oss_timer_detach_client (mididev->timer_dev, mididev);

  mididev->timer_dev = -1;

  if (source_dev < 0 || source_dev >= num_mididevs)
    return;

  if (midi_devs[source_dev]->timer_dev < 0)
    return;

  if ((err =
       oss_timer_attach_client (midi_devs[source_dev]->timer_dev,
				mididev)) < 0)
    {
      cmn_err (CE_CONT, "Cannot attach timer %d to MIDI dev %d, err=%d\n",
	       midi_devs[source_dev]->timer_dev, mididev->dev, err);
      return;
    }

  mididev->timer_dev = midi_devs[source_dev]->timer_dev;
  mididev->timer_driver = midi_devs[source_dev]->timer_driver;
}

int
oss_midi_output_intr_handler (int dev, int callback_flag)
{
  mididev_t *mididev;
  oss_native_word flags;

  unsigned char *buf;
  int len;
  midi_packet_header_t *hdr;
  MDB (cmn_err (CE_CONT, "oss_midi_output_intr_handler()\n"));

  if (dev < 0 || dev >= num_mididevs)
    cmn_err (CE_PANIC, "oss_midi_output_intr: Bad device %d\n", dev);

  mididev = midi_devs[dev];

  if (callback_flag)
    {
      mididev->out_timeout = 0;
    }

  if (!(mididev->open_mode & OPEN_WRITE))	/* Device may have been closed */
    {
      return 0;
    }

  MUTEX_ENTER_IRQDISABLE (mididev->mutex, flags);

  while ((len = midi_queue_find_buffer (mididev->out_queue, &buf, &hdr)) >= 0)
/* TODO: Check time */
    {
      int i, n = 0;

      if (!callback_flag)	/* Called from write() handler */
	if (hdr != NULL && (hdr->options & MIDI_OPT_BUSY))
	  {
	    /*
	     * This buffer is already playing
	     */
	    MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
	    return 0;
	  }

      MDB (cmn_err (CE_CONT, "Bytes in buffer=%d\n", len));
#if 0
      if (buf != NULL)
	MDB (cmn_err (CE_CONT, "First=%02x\n", buf[0]));
#endif

      if (hdr == NULL)
	{
	  MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
	  return 0;
	}

      hdr->options |= MIDI_OPT_BUSY;
      if (mididev->timer_dev >= 0)
	if (hdr->options & MIDI_OPT_TIMED)
	  {
	    if (mididev->d->flush_output)
	      mididev->d->flush_output (mididev->dev);
	    MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
	    /* TODO: Does releasing the mutex here cause any race conditions? */
	    if (oss_timer_wait_time
		(mididev->timer_dev, mididev->dev, mididev->latency,
		 hdr->time, oss_midi_callback, hdr->options) <= 0)
	      {
		/*
		 * Not the right time yet. Timer is armed
		 * and the callback function will be called
		 * at the right time. We don't remove the event
		 * so it can be retried later.
		 */
		return 0;
	      }
	    MUTEX_ENTER_IRQDISABLE (mididev->mutex, flags);
	  }

      switch (hdr->event_type)
	{
	case MIDI_EV_TEMPO:
	  mididev->tempo = hdr->parm;
	  setup_tempo (mididev);
	  break;

	case MIDI_EV_START:
	  oss_timer_start (mididev->timer_dev);
	  if (mididev->d->timer_setup)
	    mididev->d->timer_setup (mididev->dev);
	  break;

	case MIDI_EV_WRITE:
	  break;
	}

      if (mididev->d->outputc == NULL
	  || (len > 1 && mididev->d->bulk_write != NULL))
	{
	  n = mididev->d->bulk_write (mididev->dev, buf, len);
	  if (n <= 0)		/* Error */
	    {
	      MDB (cmn_err
		   (CE_NOTE, "MIDI device %d - output stalled (%d).\n",
		    mididev->dev, n));
	      n = 0;
	    }
	}
      else
	{
	  for (i = 0; i < len; i++)
	    {
	      MDB (cmn_err (CE_CONT, "Out %d %02x\n", mididev->dev, buf[i]));
	      if (!mididev->d->outputc (mididev->dev, buf[i]))
		{
		  MDB (cmn_err
		       (CE_NOTE, "MIDI device %d - output stalled (%d).\n",
			mididev->dev, n));
		  break;	/* device full */
		}
	      n++;
	    }
	}

      /* Remove the bytes sent out */
      if (n > 0 || len == 0)
	{
	  MDB (cmn_err (CE_CONT, "Remove %d/%d\n", n, len));
	  midi_queue_remove_chars (mididev->out_queue, n);
	  oss_wakeup (mididev->out_wq, &mididev->mutex, &flags, POLLOUT);
	}

      if (n == len)
	{
	  if (mididev->d->flush_output)
	    mididev->d->flush_output (mididev->dev);
	}

      if (n < len)
	{
	  /* Not everything consumed - arm a timeout */

	  MDB (cmn_err (CE_CONT, "Not all consumed %d/%d\n", n, len));
	  if (mididev->out_timeout == 0)	/* Not armed yet */
	    {
	      oss_native_word d = dev;
	      mididev->out_timeout =
		timeout (oss_midi_callback, (void *) d, 1);
	    }
	  MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);

	  return 0;		/* Not all buffered bytes were sent */
	}
      MDB (cmn_err (CE_CONT, "Everything consumed\n"));
    }

  MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);

  return 1;			/* Everything was done */
}

int
oss_midi_output_intr (int dev)
{
  MDB (cmn_err (CE_CONT, "oss_midi_output_intr\n"));
  return oss_midi_output_intr_handler (dev, 0);
}

static void
oss_midi_callback (void *arg)
{
  int dev;

  dev = (oss_native_word) arg;

  MDB (cmn_err (CE_CONT, "MIDI timer callback\n"));
  oss_midi_output_intr_handler (dev, 1);
}

void
oss_midi_set_defaults (mididev_t * mididev)
{
  mididev->prech_timeout = 0;	/* Infinite wait */

  mididev->event_input = oss_midi_input_event;
  mididev->tempo = 120;
  mididev->timebase = OSS_HZ;
}

static void put_output (mididev_t * mididev, unsigned char *buf, int len);

int
oss_midi_open (int dev, int no_worries, struct fileinfo *file, int recursive,
	       int open_flags, int *newdev)
{
/*
 * oss_midi_open opens a MIDI client (/dev/midi##) and binds it directly to
 * the MIDI port with the same number (dev).
 */
  oss_native_word flags;
  oss_midi_client_t *client;
  int ok, err = OSS_EBUSY;
  int mode = file->mode & O_ACCMODE;

  char *cmd;

  if (dev < 0 || dev >= num_mididevs)
    return OSS_ENXIO;

  /* 
   * Don't allow read only access on playback only devices.
   * However R/W access should be allowed so that the application can be
   * informed about (possible) status changes.
   */
  if ((mode == OPEN_READ) && !(midi_devs[dev]->flags & MFLAG_INPUT))
    {
      return OSS_EACCES;
    }

  /*
   * Input only devices cannot do output so don't allow it.
   */
  if ((mode & OPEN_WRITE) && !(midi_devs[dev]->flags & MFLAG_OUTPUT))
    {
      return OSS_EACCES;
    }

  if (!midi_devs[dev]->enabled)
    return OSS_ENXIO;

  if (midi_devs[dev]->unloaded)
    return OSS_ENXIO;

  client = oss_midi_clients[dev];
  if (client == NULL)
     return OSS_ENXIO;

  MUTEX_ENTER_IRQDISABLE (midi_mutex, flags);

  ok = 1;
  if (client->open_mode != 0)	/* Client busy */
    ok = 0;

  if (midi_devs[dev]->open_mode != 0)	/* Device busy */
    ok = 0;

  if (ok)			/* Still OK */
    {
      client->mididev = midi_devs[dev];
      if (client->mididev == NULL)
      {
	 cmn_err(CE_WARN, "client->mididev == NULL\n");
         MUTEX_EXIT_IRQRESTORE (midi_mutex, flags);
	 return OSS_EIO;
      }

      client->open_mode = mode;
      midi_devs[dev]->open_mode = mode;

      if ((cmd = GET_PROCESS_NAME (file)) != NULL)
    	 strncpy (client->mididev->cmd, cmd, 15);
      client->mididev->pid = GET_PROCESS_PID (file);

      /* Release locks before calling device open method */
      MUTEX_EXIT_IRQRESTORE (midi_mutex, flags);
      if ((err = midi_devs[dev]->d->open (dev, mode,
					  oss_midi_input_byte,
					  oss_midi_input_buf,
					  oss_midi_output_intr)) < 0)
	ok = 0;
      /* Reacquire locks */
      MUTEX_ENTER_IRQDISABLE (midi_mutex, flags);
      if (!ok)
	{
	  client->open_mode = 0;
	  midi_devs[dev]->open_mode = 0;
	}
    }

  if (!ok)
    {
      MUTEX_EXIT_IRQRESTORE (midi_mutex, flags);
      return err;
    }

  oss_midi_set_defaults (client->mididev);

  client->mididev->is_timing_master = 0;

  client->num = dev;

  MUTEX_EXIT_IRQRESTORE (midi_mutex, flags);

  if (mode & OPEN_READ)
    {
      char name[16];
      sprintf (name, "read%d", dev);
      client->mididev->in_queue =
	midi_queue_alloc (client->mididev->osdev, name);
      if (client->mididev->in_queue == NULL)
	{
	  cmn_err (CE_WARN, "Failed to allocate MIDI input queue\n");
	  midi_devs[dev]->open_mode = 0;
	  client->open_mode = 0;
	  client->mididev = NULL;
	  return OSS_ENOMEM;
	}
    }

  if (mode & OPEN_WRITE)
    {
      char name[16];
      int i;
      unsigned char tmpbuf[4];
      sprintf (name, "write%d", dev);
      client->mididev->out_queue =
	midi_queue_alloc (client->mididev->osdev, name);
/* if (dev==0)midi_queue_debugging(client->mididev->out_queue); */
      if (client->mididev->out_queue == NULL)
	{
	  cmn_err (CE_WARN, "Failed to allocate MIDI output queue\n");
	  midi_devs[dev]->open_mode = 0;
	  client->open_mode = 0;
	  client->mididev = NULL;
	  return OSS_ENOMEM;
	}

/*
 * Reset all MIDI controllers to their default values
 */
      if (!(client->mididev->flags & MFLAG_QUIET))
	for (i = 0; i < 16; i++)
	  {
	    tmpbuf[0] = 0xb0 | i;	/* Control change */
	    tmpbuf[1] = 121;	/* Reset all controllers */
	    tmpbuf[2] = 127;	/* Reset vol/exp/pan too */
	    put_output (client->mididev, tmpbuf, 3);
	  }
      if (client->mididev->d->flush_output)
	client->mididev->d->flush_output (client->mididev->dev);
    }

  if (client->mididev->d->init_device)
    client->mididev->d->init_device (dev);

  return 0;
}

int
oss_vmidi_open (int dev, int dev_type, struct fileinfo *file, int recursive,
		int open_flags, int *newdev);

static void
sync_output (mididev_t * mididev)
{
  int n = 0;
  unsigned int status;
  oss_native_word flags;

  if (mididev->is_killed)
    {
      midi_queue_removeall (mididev->out_queue);
      return;
    }

  if (mididev->timer_dev < 0 || !oss_timer_is_running (mididev->timer_dev))
    {
      midi_queue_removeall (mididev->out_queue);
      return;
    }

  MUTEX_ENTER_IRQDISABLE (mididev->mutex, flags);
  if (mididev->d->flush_output)
    mididev->d->flush_output (mididev->dev);

  while (!midi_queue_isempty (mididev->out_queue))
    {
      int tmout;

      if (n++ > 100)
	{
	  cmn_err (CE_NOTE, "MIDI output didn't get drained (sync).\n");
	  MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
	  return;
	}
      tmout = oss_timer_get_timeout (mididev->timer_dev, mididev->dev);
      if (oss_sleep (mididev->out_wq, &mididev->mutex,
		     tmout, &flags, &status))
	n = 0;			/* Not timeout */
      if (status & WK_SIGNAL)
	{
	  mididev->is_killed = 1;
	  MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
	  return;
	}
    }

  n = 0;
  if (mididev->d->wait_output)
    while (mididev->d->wait_output (mididev->dev))
      {
	MDB (cmn_err (CE_CONT, "Wait output\n"));
	if (n++ > 10)
	  {
	    cmn_err (CE_NOTE, "MIDI output doesn't get emptied (sync).\n");
	    MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
	    return;
	  }
	oss_sleep (mididev->out_wq, &mididev->mutex, OSS_HZ, &flags, &status);
	if (status & WK_SIGNAL)
	  {
	    mididev->is_killed = 1;
	    MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
	    return;
	  }
      }
/*
 * Give the device few moments of extra time to flush all buffers.
 */
  oss_sleep (mididev->out_wq, &mididev->mutex, OSS_HZ / 2, &flags, &status);
  MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);

}

static void
put_output (mididev_t * mididev, unsigned char *buf, int len)
{
  int i, n;
  int tmout = 1000;		/* Spend here at most 1000 msec */

  if (mididev->d->outputc == NULL
      || (len > 1 && mididev->d->bulk_write != NULL))
    {
      int p = 0;

      while (p < len && tmout-- > 0)
	{
	  n = mididev->d->bulk_write (mididev->dev, buf + p, len - p);
	  if (n < 0)		/* Error */
	    {
	      return;
	    }

	  if (n < (len - p))	/* Not all consumed yet */
	    oss_udelay (1000);

	  p += n;
	}

      return;
    }

  if (mididev->d->outputc)
    for (i = 0; i < len; i++)
      {
	while (!mididev->d->outputc (mididev->dev, buf[i]) && tmout-- > 0)
	  {
	    oss_udelay (1000);
	  }
      }
}

void
oss_midi_release (int dev, struct fileinfo *file)
{
  oss_native_word flags;
  oss_midi_client_t *client;
  int midi_dev;
  mididev_t *mididev;
  int mode;

  MUTEX_ENTER_IRQDISABLE (midi_mutex, flags);
  mode = file->mode & O_ACCMODE;
  client = oss_midi_clients[dev];

  if (client->mididev == NULL)	/* Not bound to a port device */
    {
      MUTEX_EXIT_IRQRESTORE (midi_mutex, flags);
      return;
    }

  mididev = client->mididev;
  midi_dev = mididev->dev;

  untimeout (mididev->mtc_timeout_id);

  client->open_mode = 0;

  if (mididev->open_mode & OPEN_WRITE)	/* Needs to sync */
    {
      MUTEX_EXIT_IRQRESTORE (midi_mutex, flags);
      sync_output (mididev);

      if (mididev->is_timing_master)
	oss_timer_stop (mididev->timer_dev);
      mididev->is_killed = 0;	/* Too young to die */

      /* Shut up possible hanging notes (if any) */
      if (!(mididev->flags & MFLAG_QUIET))
	{
	  unsigned char tmpbuf[4];
	  int i;

	  /* Sending one active sensing message should stop most devices */
	  tmpbuf[0] = 0xfe;
	  put_output (mididev, tmpbuf, 1);

#if 1
	  /*
	   * To make sure we will also send a "all notes off" message to
	   * all MIDI channels.
	   */
	  for (i = 0; i < 16; i++)
	    {
	      tmpbuf[0] = 0xb0 | i;	/* Control change */
	      tmpbuf[1] = 123;	/* All notes off */
	      tmpbuf[2] = 0;
	      put_output (mididev, tmpbuf, 3);
	    }
#endif
	}

      sync_output (mididev);

      MUTEX_ENTER_IRQDISABLE (midi_mutex, flags);
    }
  MUTEX_EXIT_IRQRESTORE (midi_mutex, flags);

  mididev->d->ioctl (midi_dev, SNDCTL_SETSONG, (ioctl_arg) "");
  mididev->d->close (midi_dev, mode);
  MUTEX_ENTER_IRQDISABLE (midi_mutex, flags);
  mididev->open_mode &= ~mode;
  MUTEX_EXIT_IRQRESTORE (midi_mutex, flags);

  if (mididev->timer_dev >= 0)
    oss_timer_detach_client (mididev->timer_dev, mididev);
  mididev->timer_dev = -1;
  mididev->timer_driver = 0;

  if (mididev->in_queue != NULL)
    midi_queue_free (mididev->in_queue);
  mididev->in_queue = NULL;
  mididev->pid = -1;
  mididev->cmd[0] = 0;

  if (mididev->out_queue != NULL)
    midi_queue_free (mididev->out_queue);
  mididev->out_queue = NULL;

  client->mididev = NULL;

  if (mididev->out_timeout != 0)
    untimeout (mididev->out_timeout);
  mididev->out_timeout = 0;
}

int
oss_midi_write (int dev, struct fileinfo *file, uio_t * buf, int count)
{
  oss_midi_client_t *client = oss_midi_clients[dev];
  mididev_t *mididev;
  int ret;
  unsigned int status;
  int c = 0, l, loops;
  oss_native_word flags;
  midi_packet_header_t hdr;
  int save_header = 0;
  int tmout = OSS_HZ;

  unsigned char *targetbuf;

  MDB (cmn_err (CE_CONT, "MIDI write %d\n", count));
  if (client->mididev == NULL)
    if ((ret = midi_mapper_autobind (dev, client->open_mode)) < 0)
      return ret;
  mididev = client->mididev;

  if (mididev == NULL)
    return OSS_EBUSY;

  MUTEX_ENTER_IRQDISABLE (mididev->mutex, flags);

  /*
   * Since the application called write again we can ignore possible
   * earlier received kill signals.
   */
  mididev->is_killed = 0;

  memset (&hdr, 0, sizeof (hdr));
  if (mididev->working_mode != MIDI_MODE_TRADITIONAL)
    {
      if (count < sizeof (midi_packet_header_t))
	{
	  MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
	  cmn_err (CE_WARN, "Too short MIDI write (no header)\n");
	  return OSS_EINVAL;
	}

      MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
      if (uiomove (&hdr, sizeof (midi_packet_header_t), UIO_WRITE, buf) != 0)
	{
	  cmn_err (CE_WARN, "uiomove (header) failed\n");
	  return OSS_EFAULT;
	}

      if (hdr.magic != MIDI_HDR_MAGIC)
	{
	  cmn_err (CE_WARN, "Bad MIDI write packet header (%04x)\n",
		   hdr.magic);
	  return OSS_EINVAL;
	}
      count -= sizeof (midi_packet_header_t);
      c += sizeof (midi_packet_header_t);

      /* Force save of the header if no other data was written */
      if (count <= 0)
	save_header = 1;

      MUTEX_ENTER_IRQDISABLE (mididev->mutex, flags);
    }

  loops = 0;
  while (save_header || count > 0)
    {
      MDB (cmn_err (CE_CONT, "Write bytes left %d\n", count));
      l = count;
      save_header = 0;

      if (loops++ > 10)
	{
	  MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
	  cmn_err (CE_CONT, "MIDI Output buffer %d doesn't drain %d/%d\n",
		   mididev->dev, c, count);
	  return OSS_EIO;
	}

      if (l > MIDI_PAYLOAD_SIZE)
	l = MIDI_PAYLOAD_SIZE;

      if ((ret =
	   midi_queue_alloc_record (mididev->out_queue, &targetbuf, l,
				    &hdr)) < 0)
	{
	  if (ret == OSS_ENOSPC)	/* Buffers full */
	    {
	      MDB (cmn_err (CE_CONT, "*** Buffers full ***\n"));
	      tmout =
		oss_timer_get_timeout (mididev->timer_dev, mididev->dev);
	      if (!oss_sleep
		  (mididev->out_wq, &mididev->mutex, tmout, &flags, &status))
		{
		  MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
		  if (c > 0)
		    return c;
		  return OSS_EIO;	/* Timeout - why */
		}
	      else
		loops = 0;	/* Restart loop limiter */

	      if (status & WK_SIGNAL)
		{
		  mididev->is_killed = 1;
		  MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
		  return OSS_EINTR;
		}

	      if (loops > 95)
		cmn_err (CE_NOTE, "MIDI Output buffers full\n");
	      continue;		/* Try again later */
	    }

	  MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
	  return ret;
	}

      if (l > ret)
	l = ret;

      MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
/* cmn_err(CE_CONT, "Copy %08x (%d)\n", targetbuf, l); */

      loops = 0;

      if (l > 0)
	{
	  MDB (cmn_err (CE_CONT, "Store %d bytes\n", l));
	  if (uiomove (targetbuf, l, UIO_WRITE, buf) != 0)
	    {
	      cmn_err (CE_WARN, "uiomove (write) failed\n");
	      return OSS_EFAULT;
	    }

	  count -= l;
	  c += l;
	}

      MDB (cmn_err (CE_CONT, "From oss_midi_write\n"));
      oss_midi_output_intr_handler (mididev->dev, 0);
      MUTEX_ENTER_IRQDISABLE (mididev->mutex, flags);
    }

  MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);

  /* midi_queue_trace(mididev->out_queue); */
  MDB (cmn_err (CE_CONT, "MIDI write done %d\n\n", c));

  return c;
}

static int
do_read (mididev_t * mididev, uio_t * buf, unsigned char *data, int c,
	 int max_bytes, midi_packet_header_t * hdr)
{
  int l = 0;

  if (mididev->working_mode != MIDI_MODE_TRADITIONAL)
    {
      if (max_bytes <= sizeof (*hdr))
	{
	  cmn_err (CE_WARN, "MIDI read too small for packet header\n");
	  return OSS_E2BIG;
	}

      hdr->magic = MIDI_HDR_MAGIC;
      if (uiomove (hdr, sizeof (*hdr), UIO_READ, buf) != 0)
	{
	  cmn_err (CE_WARN, "uiomove (read) failed\n");
	  return OSS_EFAULT;
	}

      l += sizeof (*hdr);
    }

  if (l + c > max_bytes)
    c = max_bytes - l;

  if (c <= 0)
    return OSS_E2BIG;

  if (uiomove (data, c, UIO_READ, buf) != 0)
    {
      cmn_err (CE_WARN, "uiomove (read) failed\n");
      return OSS_EFAULT;
    }

  midi_queue_remove_chars (mididev->in_queue, c);
  l += c;

  return l;
}

int
oss_midi_read (int dev, struct fileinfo *file, uio_t * buf, int count)
{
  oss_midi_client_t *client = oss_midi_clients[dev];
  mididev_t *mididev;
  oss_native_word flags;
  unsigned char *data;

  unsigned int status;
  int c = 0;
  midi_packet_header_t *hdr;

  int err;

  if (client->mididev == NULL)
    if ((err = midi_mapper_autobind (dev, client->open_mode)) < 0)
      return err;
  mididev = client->mididev;

  if (mididev == NULL)
    return OSS_EBUSY;

  MUTEX_ENTER_IRQDISABLE (mididev->mutex, flags);

  if ((c = midi_queue_find_buffer (mididev->in_queue, &data, &hdr)) < 0)
    {
      MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
      return c;
    }

  if (c > 0
      || (mididev->working_mode != MIDI_MODE_TRADITIONAL && c >= 0
	  && hdr != NULL))
    {
      MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
      return do_read (mididev, buf, data, c, count, hdr);
    }

  if (mididev->prech_timeout < 0)	/* Non blocking mode */
    {
      MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
      return OSS_EWOULDBLOCK;
    }

  if (!oss_sleep
      (mididev->in_wq, &mididev->mutex, mididev->prech_timeout, &flags,
       &status))
    {
      /* Timeout */

      MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
      return 0;
    }

  if (status & WK_SIGNAL)
    {
      mididev->is_killed = 1;
      MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
      return OSS_EINTR;
    }

  if ((c = midi_queue_get (mididev->in_queue, &data, count, &hdr)) < 0
      || (c == 0 && hdr == NULL))
    {
      MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
      return c;
    }

  MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
  if (c > 0)
    return do_read (mididev, buf, data, c, count, hdr);

  return c;
}

static int
setup_working_mode (mididev_t * mididev)
{
  int timer_dev;
  int err;

  if (mididev->timer_dev < 0)	/* No timer yet */
    {
      /*
       * Setup a timer
       */

      if (mididev->timer_driver < 0
	  || mididev->timer_driver >= oss_num_timer_drivers)
	{
	  cmn_err (CE_WARN, "Invalid MIDI timer %d selected\n",
		   mididev->timer_driver);
	  return OSS_ENXIO;
	}

      if ((timer_dev =
	   oss_timer_create_instance (mididev->timer_driver, mididev)) < 0)
	return timer_dev;
      mididev->is_timing_master = 1;

      if ((err = oss_timer_attach_client (timer_dev, mididev)) < 0)
	{
	  oss_timer_detach_client (timer_dev, mididev);
	  return err;
	}

      mididev->timer_dev = timer_dev;
      if (mididev->d->timer_setup)
	mididev->d->timer_setup (mididev->dev);
    }
  else
    timer_dev = mididev->timer_dev;

  if (timer_dev < 0 || timer_dev >= oss_num_timers ||
      oss_timer_devs[timer_dev] == NULL)
    {
      cmn_err (CE_WARN, "Failed to allocate a timer instance\n");
      return OSS_ENXIO;
    }


  return 0;
}

static int
setup_timebase (mididev_t * mididev)
{
  if (mididev->timer_dev < 0)	/* No timer yet */
    return 0;

  return oss_timer_set_timebase (mididev->timer_dev, mididev->timebase);
}

static int
setup_tempo (mididev_t * mididev)
{
  if (mididev->timer_dev < 0)	/* No timer yet */
    return 0;

  return oss_timer_set_tempo (mididev->timer_dev, mididev->tempo);
}

int
oss_midi_ioctl (int dev, struct fileinfo *file,
		unsigned int cmd, ioctl_arg arg)
{
  oss_midi_client_t *client = oss_midi_clients[dev];
  mididev_t *mididev;
  int err, val;
  oss_native_word d;

  if (client->mididev == NULL)
    if ((err = midi_mapper_autobind (dev, client->open_mode)) < 0)
      return err;
  mididev = client->mididev;
  if (mididev == NULL)
    return OSS_EBUSY;

  dev = mididev->dev;

  switch (cmd)
    {
    case SNDCTL_MIDI_PRETIME:
      val = *arg;
      if (val < -1)
	return OSS_EINVAL;

      if (val != -1)
	val = (OSS_HZ * val) / 100;
      mididev->prech_timeout = val;
      return 0;
      break;

    case SNDCTL_MIDI_MTCINPUT:
      val = *arg;
      switch (val)
	{
	case 0:
	case -1:
	  break;
	case 24:
	  mididev->mtc_codetype = 0;
	  break;
	case 25:
	  mididev->mtc_codetype = 1;
	  break;
	case 29:
	  mididev->mtc_codetype = 2;
	  break;
	case 30:
	  mididev->mtc_codetype = 3;
	  break;
	  break;
	default:
	  return OSS_EINVAL;
	}
      mididev->mtc_timebase = val;
      mididev->mtc_t0 = GET_JIFFIES ();
      mididev->mtc_current = 0;
      mididev->mtc_prev_t = -1;
      mididev->mtc_phase = 0;
      d = dev;
      if (mididev->mtc_timebase != -1)
	mididev->mtc_timeout_id =
	  timeout (mtc_generator, (void *) d, OSS_HZ / 100);
      return *arg = val;
      break;

    case SNDCTL_MIDI_SETMODE:
      val = *arg;
      if (val != MIDI_MODE_TRADITIONAL && val != MIDI_MODE_TIMED
	  && val != MIDI_MODE_TIMED_ABS)
	return OSS_EINVAL;
      mididev->working_mode = val;
      *arg = val;
      if ((err = setup_working_mode (mididev)) < 0)
	return err;
      if ((err = setup_tempo (mididev)) < 0)
	return err;
      if ((err = setup_timebase (mididev)) < 0)
	return err;
      return 0;
      break;

    case SNDCTL_MIDI_TIMEBASE:
      val = *arg;
      if (val < 1 || val > 1000)
	return OSS_EINVAL;
      mididev->timebase = val;
      return setup_timebase (mididev);
      break;

    case SNDCTL_MIDI_TEMPO:
      val = *arg;
      if (val < 1 || val > 200)
	return OSS_EINVAL;
      mididev->tempo = val;
      return setup_tempo (mididev);
      break;

    default:
      return mididev->d->ioctl (mididev->dev, cmd, arg);
    }
}

#ifdef ALLOW_SELECT
int
oss_midi_chpoll (int dev, struct fileinfo *file, oss_poll_event_t * ev)
{
  short events = ev->events;
  oss_midi_client_t *client;
  mididev_t *mididev;
  oss_native_word flags;
  int err;

  if (dev < 0 || dev >= oss_num_midi_clients)
    return OSS_ENXIO;
  client = oss_midi_clients[dev];
  if (client->mididev == NULL)
    if ((err = midi_mapper_autobind (dev, client->open_mode)) < 0)
      return err;
  mididev = client->mididev;
  if (mididev == NULL)
    return OSS_EIO;

  if ((events & (POLLOUT | POLLWRNORM)) && (mididev->open_mode & OPEN_WRITE))
    {
      MUTEX_ENTER_IRQDISABLE (mididev->mutex, flags);
      if (mididev->out_queue == NULL
	  || midi_queue_spaceleft (mididev->out_queue) < 10)
	{			/* No space yet */
	  oss_register_poll (mididev->out_wq, &mididev->mutex, &flags, ev);
	}
      else
	{
	  ev->revents |= (POLLOUT | POLLWRNORM) & events;
	}
      MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
    }

  if ((events & (POLLIN | POLLRDNORM)) && (mididev->open_mode & OPEN_READ))
    {
      MUTEX_ENTER_IRQDISABLE (mididev->mutex, flags);
      if (mididev->in_queue == NULL || midi_queue_isempty (mididev->in_queue))
	{			/* No space yet */
	  oss_register_poll (mididev->in_wq, &mididev->mutex, &flags, ev);
	}
      else
	{
	  ev->revents |= (POLLIN | POLLRDNORM) & events;
	}
      MUTEX_EXIT_IRQRESTORE (mididev->mutex, flags);
    }

  return 0;
}
#endif

#ifdef VDEV_SUPPORT
static oss_cdev_drv_t vmidi_cdev_drv = {
  oss_vmidi_open,
  oss_midi_release,
  oss_midi_read,
  oss_midi_write,
  oss_midi_ioctl,
#ifdef ALLOW_SELECT
  oss_midi_chpoll
#else
  NULL
#endif
};
#endif

static oss_cdev_drv_t midi_cdev_drv = {
  oss_midi_open,
  oss_midi_release,
  oss_midi_read,
  oss_midi_write,
  oss_midi_ioctl,
#ifdef ALLOW_SELECT
  oss_midi_chpoll
#else
  NULL
#endif
};

int
oss_vmidi_open (int dev, int dev_type, struct fileinfo *file, int recursive,
		int open_flags, int *newdev)
{
/*
 * oss_vmidi_open is another version of oss_midi_open. Instead of binding
 * the client directly with the device port this routine creates just a
 * client (device file). The binding operation is left to be done later
 * using SNDCTL_MIDI_BIND. If no binding is made then the first
 * read/write/ioctl/etc operation will automatically bind the client
 * with the first available (lowest number) MIDI port.
 *
 * oss_vmidi_open() drives the /dev/midi device file.
 */

  oss_native_word flags;
  oss_midi_client_t *client = NULL;
  int d;
  int mode = file->mode & O_ACCMODE;

  MUTEX_ENTER_IRQDISABLE (midi_mutex, flags);

/*
 * Find the first midi client number that is free. Start from the client 
 * numbers after the last existing MIDI port.
 */
  for (dev = num_mididevs; dev < oss_num_midi_clients; dev++)
    {
      if (oss_midi_clients[dev] != NULL &&
	  oss_midi_clients[dev]->open_mode == 0)
	{
	  client = oss_midi_clients[dev];
	  break;
	}
    }

  if (client == NULL)
    {
      /*
       * No earlier allocated clients were free so create a new entry.
       * Also create an anonymous character device.
       */
      MUTEX_EXIT_IRQRESTORE (midi_mutex, flags);
      client = PMALLOC (osdev, sizeof (*client));
      MUTEX_ENTER_IRQDISABLE (midi_mutex, flags);

      if (oss_num_midi_clients >= MAX_MIDI_CLIENTS)
	{
	  cmn_err (CE_WARN, "Too many MIDI clients\n");
	  MUTEX_EXIT_IRQRESTORE (midi_mutex, flags);
	  return OSS_ENXIO;
	}

      if (client == NULL)
	{
	  cmn_err (CE_WARN, "Out of memory\n");
	  MUTEX_EXIT_IRQRESTORE (midi_mutex, flags);
	  return OSS_ENOMEM;
	}

      memset (client, 0, sizeof (*client));
      oss_midi_clients[dev = oss_num_midi_clients++] = client;

#ifdef CONFIG_OSS_MIDI
      MDB (cmn_err (CE_CONT, "Installing /dev/midi%02d\n", dev));
      oss_install_chrdev (osscore_osdev, NULL, OSS_DEV_MIDI, dev,
			  &midi_cdev_drv, 0);
#endif
    }

  client->open_mode = mode;
  client->num = dev;
  client->mididev = NULL;	/* Not bound yet */
  MUTEX_EXIT_IRQRESTORE (midi_mutex, flags);

  if ((d = oss_find_minor (OSS_DEV_MIDI, dev)) < 0)
    {
      oss_midi_release (dev, file);
      return d;
    }
  *newdev = d;

  return dev;
}

void
oss_midi_init (oss_device_t * osdev)
{
  static int already_done = 0;

  if (already_done)
    return;
  already_done = 1;

  if (sizeof (midi_packet_header_t) != 32)
    {
      cmn_err (CE_WARN, "sizeof(midi_packet_header_t) != 32 (%d)\n",
	       sizeof (midi_packet_header_t));
      cmn_err (CE_CONT, "MIDI subsystem not activated\n");
      return;
    }

  osscore_osdev = osdev;

  MUTEX_INIT (osdev, midi_mutex, MH_FRAMEW);

  midi_mapper_init (osdev);
  oss_init_timers (osdev);
  attach_oss_default_timer (osdev);
}

void
oss_midi_uninit (void)
{
  int dev;

  midi_mapper_uninit ();
  detach_oss_default_timer ();
  oss_uninit_timers ();

  for (dev = 0; dev < num_mididevs; dev++)
    {
      oss_remove_wait_queue (midi_devs[dev]->out_wq);
      oss_remove_wait_queue (midi_devs[dev]->in_wq);
      MUTEX_CLEANUP (midi_devs[dev]->mutex);
    }
  MUTEX_CLEANUP (midi_mutex);
}

void
install_vmidi (oss_device_t * osdev)
{
#ifdef CONFIG_OSS_MIDI
#ifdef VDEV_SUPPORT
  oss_install_chrdev (osdev, "midi", OSS_DEV_VMIDI, 0, &vmidi_cdev_drv,
		      CHDEV_VIRTUAL);
#endif
#endif
}

int
oss_install_mididev (int version,
		     char *id, char *name,
		     midi_driver_t * d, int driver_size,
		     unsigned int flags, void *devc, oss_device_t * osdev)
{
  int curr_midi_dev;
  mididev_t *op;
  oss_midi_client_t *client;
  int i;
  int old = 0;

/* 
 * Old style drivers are always input&output even they don't report that.
 * Fix the permissions automatically.
 */
  if (!(flags & (MFLAG_INPUT | MFLAG_OUTPUT)))
    flags |= MFLAG_INPUT | MFLAG_OUTPUT;

  if (driver_size > sizeof (midi_driver_t))
    driver_size = sizeof (midi_driver_t);

  if (midi_devs == NULL)
    {
      midi_devs = PMALLOC (osdev, sizeof (mididev_p) * MAX_MIDI_DEV);
    }

  if (midi_devs == NULL)
    {
      cmn_err (CE_WARN, "oss_install_mididev: Out of memory\n");
      return OSS_ENOMEM;
    }

  for (i = 0; i < num_mididevs; i++)
    if (midi_devs[i]->unloaded && midi_devs[i]->os_id == oss_get_osid (osdev))
      {
	old = 1;
	op = midi_devs[i];
	curr_midi_dev = i;
	MDB (cmn_err (CE_CONT, "Reincarnation of MIDI device %d\n", i));
	break;
      }

  if (!old)
    {
      if (num_mididevs >= MAX_MIDI_DEV - 1)
	{
	  cmn_err (CE_WARN, "Too many MIDI devices in the system\n");
	  return OSS_EIO;
	}

      op = midi_devs[num_mididevs] = PMALLOC (osdev, sizeof (mididev_t));
      if (op == NULL)
	{
	  cmn_err (CE_WARN,
		   "oss_install_mididev: Failed to allocate memory\n");
	  return OSS_ENOMEM;
	}
      memset (op, 0, sizeof (*op));
      curr_midi_dev = num_mididevs++;
    }

  op->devc = devc;
  op->dev = op->real_dev = curr_midi_dev;
  op->pid = -1;
  op->osdev = osdev;
  op->os_id = oss_get_osid (osdev);
  op->latency = -1;		/* Not known */

  if (!old)
    {
      MUTEX_INIT (op->osdev, op->mutex, MH_FRAMEW + 1);

      if ((op->out_wq =
	   oss_create_wait_queue (op->osdev, "midi_out")) == NULL)
	cmn_err (CE_PANIC, "Cannot create MIDI output wait queue\n");

      if ((op->in_wq = oss_create_wait_queue (op->osdev, "midi_in")) == NULL)
	cmn_err (CE_PANIC, "Cannot create MIDI input wait queue\n");

      op->d = PMALLOC (osdev, sizeof (midi_driver_t));
      memset (op->d, 0, sizeof (op->d));
      memcpy (op->d, d, driver_size);
      sprintf (op->handle, "%s-md%02d", osdev->handle, ++osdev->num_mididevs);
      op->port_number = osdev->num_mididevs;
    }

  op->flags = flags;
  op->enabled = 1;
  op->unloaded = 0;
  op->mtc_timebase = -1;
  op->card_number = osdev->cardnum;
  op->timer_driver = 0;		/* Point to the default timer */
  op->timer_dev = -1;		/* No timer instance created yet */

  strncpy (op->name, name, sizeof (op->name) - 1);
  op->name[sizeof (op->name) - 1] = 0;

  op->caps = 0;
  if (flags & MFLAG_INPUT)
    op->caps |= MIDI_CAP_INPUT;
  if (flags & MFLAG_OUTPUT)
    op->caps |= MIDI_CAP_OUTPUT;


  if (oss_midi_clients[curr_midi_dev] == NULL)
    {
      client = PMALLOC (osdev, sizeof (*client));
      if (client == NULL)
	cmn_err (CE_PANIC, "OSS install MIDI: Out of memory.\n");
      memset (client, 0, sizeof (*client));
      oss_midi_clients[curr_midi_dev] = client;
      oss_num_midi_clients = num_mididevs;
    }


#ifdef CONFIG_OSS_MIDI
/*
 * Create the device node.
 */

  {
    char name[32];
#ifdef NEW_DEVICE_NAMING
# ifdef USE_DEVICE_SUBDIRS
    sprintf (name, "oss/%s/mid%d", osdev->nick, osdev->num_mididevs - 1);
# else
    sprintf (name, "%s_mid%d", osdev->nick, osdev->num_mididevs - 1);
# endif
#else
    sprintf (name, "midi%02d", curr_midi_dev);
#endif
    oss_install_chrdev (osdev, name, OSS_DEV_MIDI, curr_midi_dev,
			&midi_cdev_drv, 0);
    sprintf (op->devnode, "/dev/%s", name);
  }
#endif

  return curr_midi_dev;
}
