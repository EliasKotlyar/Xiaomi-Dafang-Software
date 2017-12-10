/*
 * Purpose: OSS audio loopback (virtual) driver
 *
 * Description:
 * OSS audio loopback driver is a virtual/pseudo driver that can be used
 * for example to user land based virtual audio devices.
 *
 * Each audioloop instance has two sides or endpoints. The server side is
 * typically used by the application that implements the virtual audio device.
 * Client side in turn is used by any audio application that wants to record or
 * play audio. Server side device must be open before the client side can be
 * opened.
 *
 *
 *
 * CAUTION! Certain portc fields (mutex, rate/format) are only available
 *          on the server side portc structure. Care must be taken.
 *
 *
 *
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

#include "oss_audioloop_cfg.h"

#define MAX_RATE 	192000
#define MAX_CHANNELS	64
#define SUPPORTED_FORMATS	(AFMT_S16_NE|AFMT_S32_NE)

extern int audioloop_instances;	/* Config option */

#define MAX_INSTANCES	16

typedef struct _audioloop_devc_t audioloop_devc_t;
typedef struct _audioloop_portc_t audioloop_portc_t;

struct _audioloop_portc_t
{
  audioloop_devc_t *devc;
  audioloop_portc_t *peer;
  int audio_dev;
  int open_mode;
  int port_type;
#define PT_CLIENT	1
#define PT_SERVER	2
  int instance;

  /* State variables */
  int input_triggered, output_triggered;
  oss_wait_queue_t *wq;

  /* Server side (only) fields */
  int rate;
  int channels;
  unsigned int fmt, fmt_bytes;
  oss_mutex_t mutex;
  timeout_id_t timeout_id;
};

struct _audioloop_devc_t
{
  oss_device_t *osdev;
  oss_mutex_t mutex;

  int num_instances;

  audioloop_portc_t *client_portc[MAX_INSTANCES];
  audioloop_portc_t *server_portc[MAX_INSTANCES];
};

#define MAX_ATTACH_COUNT	1
static audioloop_devc_t audioloop_devices[MAX_ATTACH_COUNT];
static int attach_count;

static void
transfer_audio (audioloop_portc_t * server_portc, dmap_t * dmap_from,
		dmap_t * dmap_to)
{
  int l = dmap_from->fragment_size;
  unsigned char *fromp, *top;

  if (dmap_to->fragment_size != l)
    {
      cmn_err (CE_WARN, "Fragment size mismatch (%d != %d)\n",
	       dmap_to->fragment_size, l);

      /* Perform emergency stop */
      server_portc->input_triggered = 0;
      server_portc->output_triggered = 0;
      server_portc->peer->input_triggered = 0;
      server_portc->peer->output_triggered = 0;
      return;
    }

  fromp =
    dmap_from->dmabuf + (dmap_from->byte_counter % dmap_from->bytes_in_use);
  top = dmap_to->dmabuf + (dmap_to->byte_counter % dmap_to->bytes_in_use);

  memcpy (top, fromp, l);

}

static void
handle_input (audioloop_portc_t * server_portc)
{
  audioloop_portc_t *client_portc = server_portc->peer;

  if (client_portc->output_triggered)
    {
      transfer_audio (server_portc,
		      audio_engines[client_portc->audio_dev]->dmap_out,
		      audio_engines[server_portc->audio_dev]->dmap_in);
      oss_audio_outputintr (client_portc->audio_dev, 0);
    }

  oss_audio_inputintr (server_portc->audio_dev, 0);
}

static void
handle_output (audioloop_portc_t * server_portc)
{
  audioloop_portc_t *client_portc = server_portc->peer;

  if (client_portc->input_triggered)
    {
      transfer_audio (server_portc,
		      audio_engines[server_portc->audio_dev]->dmap_out,
		      audio_engines[client_portc->audio_dev]->dmap_in);
      oss_audio_inputintr (client_portc->audio_dev, 0);
    }

  oss_audio_outputintr (server_portc->audio_dev, 0);
}

static void
audioloop_cb (void *pc)
{
/*
 * This timer callback routine will get called 100 times/second. It handles
 * movement of audio data between the client and server sides.
 */
  audioloop_portc_t *server_portc = pc;
  int tmout = OSS_HZ / 100;

  if (tmout < 1)
    tmout = 1;

  server_portc->timeout_id = 0;	/* No longer valid */

  if (server_portc->input_triggered)
    handle_input (server_portc);

  if (server_portc->output_triggered)
    handle_output (server_portc);

  /* Retrigger timer callback */
  if (server_portc->input_triggered || server_portc->output_triggered)
    server_portc->timeout_id = timeout (audioloop_cb, server_portc, tmout);
}

static int
audioloop_check_input (int dev)
{
  audioloop_portc_t *portc = audio_engines[dev]->portc;
  if (!portc->peer->output_triggered)
    {
      return OSS_ECONNRESET;
    }
  return 0;
}

static int
audioloop_check_output (int dev)
{
  audioloop_portc_t *portc = audio_engines[dev]->portc;

  if (!portc->peer->input_triggered)
    {
      return OSS_ECONNRESET;
    }

  if (portc->peer->open_mode == 0)
    return OSS_EIO;
  return 0;
}

static void
setup_sample_format (audioloop_portc_t * portc)
{
  adev_t *adev;
  int fragsize, frame_size;

  frame_size = portc->channels * portc->fmt_bytes;
  if (frame_size == 0)
    frame_size = 4;

  fragsize = (portc->rate * frame_size) / 100;	/* Number of bytes/fragment (100Hz) */
  portc->rate = fragsize * 100 / frame_size;

/* Setup the server side */
  adev = audio_engines[portc->audio_dev];
  adev->min_block = adev->max_block = fragsize;

/* Setup the client side */
  adev = audio_engines[portc->peer->audio_dev];
  adev->min_block = adev->max_block = fragsize;

  adev->max_rate = adev->min_rate = portc->rate;
  adev->iformat_mask = portc->fmt;
  adev->oformat_mask = portc->fmt;
  adev->xformat_mask = portc->fmt;
  adev->min_channels = adev->max_channels = portc->channels;
}

static int
audioloop_server_set_rate (int dev, int arg)
{
  audioloop_portc_t *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->rate;

  if (portc->peer->input_triggered || portc->peer->output_triggered)
    return portc->rate;

  if (arg < 5000)
    arg = 5000;
  if (arg > MAX_RATE)
    arg = MAX_RATE;

  /* Force the sample rate to be multiple of 100 */
  arg = (arg / 100) * 100;

  portc->rate = arg;

  setup_sample_format (portc);

  return portc->rate = arg;
}

/*ARGSUSED*/
static int
audioloop_client_set_rate (int dev, int arg)
{
  audioloop_portc_t *portc = audio_engines[dev]->portc;

  return portc->peer->rate;
}

static short
audioloop_server_set_channels (int dev, short arg)
{
  audioloop_portc_t *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->channels;

  if (portc->peer->input_triggered || portc->peer->output_triggered)
    return portc->channels;

  if (arg < 1)
    arg = 1;
  if (arg > MAX_CHANNELS)
    arg = MAX_CHANNELS;

  portc->channels = arg;

  setup_sample_format (portc);

  return portc->channels;
}

/*ARGSUSED*/
static short
audioloop_client_set_channels (int dev, short arg)
{
  audioloop_portc_t *portc = audio_engines[dev]->portc;

  return portc->peer->channels;	/* Server side channels */
}

static unsigned int
audioloop_server_set_format (int dev, unsigned int arg)
{
  audioloop_portc_t *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->fmt;

  if (portc->peer->input_triggered || portc->peer->output_triggered)
    return portc->fmt;

  switch (arg)
    {
    case AFMT_S16_NE:
      portc->fmt_bytes = 2;
      break;

    case AFMT_S32_NE:
      portc->fmt_bytes = 4;
      break;

    default:			/* Unsupported format */
      arg = AFMT_S16_NE;
      portc->fmt_bytes = 2;

    }

  portc->fmt = arg;

  setup_sample_format (portc);

  return portc->fmt;
}

/*ARGSUSED*/
static unsigned int
audioloop_client_set_format (int dev, unsigned int arg)
{
  audioloop_portc_t *portc = audio_engines[dev]->portc;

  return portc->peer->fmt;	/* Server side sample format */
}

static void audioloop_trigger (int dev, int state);

static void
audioloop_reset (int dev)
{
  audioloop_trigger (dev, 0);
}

/*ARGSUSED*/
static int
audioloop_server_open (int dev, int mode, int open_flags)
{
  audioloop_portc_t *portc = audio_engines[dev]->portc;
  audioloop_devc_t *devc = audio_engines[dev]->devc;
  oss_native_word flags;
  adev_t *adev;

  if ((mode & OPEN_READ) && (mode & OPEN_WRITE))
    return OSS_EACCES;

  if (portc == NULL || portc->peer == NULL)
    return OSS_ENXIO;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (portc->open_mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  portc->open_mode = mode;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

/*
 * Update client device flags
 */
  adev = audio_engines[portc->peer->audio_dev];
  adev->flags &= ~(ADEV_NOINPUT | ADEV_NOOUTPUT);
  if (!(mode & OPEN_READ))
    adev->flags |= ADEV_NOOUTPUT;
  if (!(mode & OPEN_WRITE))
    adev->flags |= ADEV_NOINPUT;
  adev->enabled = 1;		/* Enable client side */

  return 0;
}

/*ARGSUSED*/
static int
audioloop_client_open (int dev, int mode, int open_flags)
{
  audioloop_portc_t *portc = audio_engines[dev]->portc;
  audioloop_devc_t *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  if (portc == NULL || portc->peer == NULL)
    return OSS_ENXIO;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (portc->open_mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  portc->open_mode = mode;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/
static void
audioloop_server_close (int dev, int mode)
{
  audioloop_portc_t *portc = audio_engines[dev]->portc;
  audioloop_devc_t *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  audio_engines[portc->peer->audio_dev]->enabled = 0;	/* Disable client side */
  portc->open_mode = 0;

  /* Stop the client side */
  portc->peer->input_triggered = 0;
  portc->peer->output_triggered = 0;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
static void
audioloop_client_close (int dev, int mode)
{
  audioloop_portc_t *portc = audio_engines[dev]->portc;
  audioloop_devc_t *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  portc->open_mode = 0;

  /* Stop the server side */
  portc->peer->input_triggered = 0;
  portc->peer->output_triggered = 0;


  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
static int
audioloop_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  switch (cmd)
    {
    case SNDCTL_GETLABEL:
      {
	/*
	 * Return an empty string so that this feature can be tested.
	 * Complete functionality is to be implemented later.
	 */
	oss_label_t *s = (oss_label_t *) arg;
	memset (s, 0, sizeof (oss_label_t));
	return 0;
      }
      break;

    case SNDCTL_GETSONG:
      {
	/*
	 * Return an empty string so that this feature can be tested.
	 * Complete functionality is to be implemented later.
	 */
	oss_longname_t *s = (oss_longname_t *) arg;
	memset (s, 0, sizeof (oss_longname_t));
	return 0;
      }
      break;
    }

  return OSS_EINVAL;
}

/*ARGSUSED*/
static void
audioloop_output_block (int dev, oss_native_word buf, int count, int fragsize,
			int intrflag)
{
}

/*ARGSUSED*/
static void
audioloop_start_input (int dev, oss_native_word buf, int count, int fragsize,
		       int intrflag)
{
}

static void
audioloop_trigger (int dev, int state)
{
  audioloop_portc_t *portc = audio_engines[dev]->portc;

  if (portc->open_mode & OPEN_READ)	/* Handle input */
    {
      portc->input_triggered = !!(state & OPEN_READ);
      if (!portc->input_triggered)
	portc->peer->output_triggered = 0;
    }

  if (portc->open_mode & OPEN_WRITE)	/* Handle output */
    {
      portc->output_triggered = !!(state & OPEN_WRITE);
      if (!portc->output_triggered)
	portc->peer->input_triggered = 0;
    }

  if (portc->output_triggered || portc->input_triggered)	/* Something is going on */
    {
      int tmout = OSS_HZ / 100;

      if (tmout < 1)
	tmout = 1;

      if (portc->port_type != PT_SERVER)
	portc = portc->peer;	/* Switch to the server side */

      if (portc->output_triggered || portc->input_triggered)	/* Something is going on */
	if (portc->timeout_id == 0)
	  portc->timeout_id = timeout (audioloop_cb, portc, tmout);
    }
  else
    {
      if (portc->port_type == PT_SERVER)
	if (portc->timeout_id != 0)
	  {
	    untimeout (portc->timeout_id);
	    portc->timeout_id = 0;
	  }
    }
}

/*ARGSUSED*/
static int
audioloop_server_prepare_for_input (int dev, int bsize, int bcount)
{
  oss_native_word flags;
  unsigned int status;

  audioloop_portc_t *portc = audio_engines[dev]->portc;

  MUTEX_ENTER_IRQDISABLE (portc->mutex, flags);
  portc->input_triggered = 0;

  /*
   * Wake the client which may be in waiting in close() 
   */
  oss_wakeup (portc->peer->wq, &portc->mutex, &flags, POLLOUT | POLLWRNORM);

  if (!(portc->peer->open_mode & OPEN_WRITE))
    {
      /* Sleep until the client side becomes ready */
      oss_sleep (portc->wq, &portc->mutex, 0, &flags, &status);
      if (status & WK_SIGNAL)
	{
	  MUTEX_EXIT_IRQRESTORE (portc->mutex, flags);
	  return OSS_EINTR;
	}
    }
  MUTEX_EXIT_IRQRESTORE (portc->mutex, flags);

  return 0;
}

/*ARGSUSED*/
static int
audioloop_server_prepare_for_output (int dev, int bsize, int bcount)
{
  oss_native_word flags;
  unsigned int status;

  audioloop_portc_t *portc = audio_engines[dev]->portc;

  MUTEX_ENTER_IRQDISABLE (portc->mutex, flags);
  portc->output_triggered = 0;

  /*
   * Wake the client which may be in waiting in close() 
   */
  oss_wakeup (portc->peer->wq, &portc->mutex, &flags, POLLIN | POLLRDNORM);

  if (!(portc->peer->open_mode & OPEN_READ))
    {
      /* Sleep until the client side becomes ready */
      oss_sleep (portc->wq, &portc->mutex, 0, &flags, &status);
      if (status & WK_SIGNAL)
	{
	  MUTEX_EXIT_IRQRESTORE (portc->mutex, flags);
	  return OSS_EINTR;
	}
    }
  MUTEX_EXIT_IRQRESTORE (portc->mutex, flags);

  return 0;
}

/*ARGSUSED*/
static int
audioloop_client_prepare_for_input (int dev, int bsize, int bcount)
{
  oss_native_word flags;
  audioloop_portc_t *portc = audio_engines[dev]->portc;
  unsigned int status;

  MUTEX_ENTER_IRQDISABLE (portc->peer->mutex, flags);
  portc->input_triggered = 0;
  /* Wake the server side */
  oss_wakeup (portc->peer->wq, &portc->peer->mutex, &flags,
	      POLLIN | POLLRDNORM);

  /*
   * Delay a moment so that the server side gets chance to reinit itself
   * for next file/stream.
   */
  oss_sleep (portc->wq, &portc->peer->mutex, OSS_HZ, &flags, &status);
  MUTEX_EXIT_IRQRESTORE (portc->peer->mutex, flags);

  return 0;
}

/*ARGSUSED*/
static int
audioloop_client_prepare_for_output (int dev, int bsize, int bcount)
{
  oss_native_word flags;
  audioloop_portc_t *portc = audio_engines[dev]->portc;
  unsigned int status;

  MUTEX_ENTER_IRQDISABLE (portc->peer->mutex, flags);
  portc->output_triggered = 0;
  /* Wake the server side */
  oss_wakeup (portc->peer->wq, &portc->peer->mutex, &flags,
	      POLLIN | POLLRDNORM);

  /*
   * Delay a moment so that the server side gets chance to reinit itself
   * for next file/stream.
   */
  oss_sleep (portc->wq, &portc->peer->mutex, OSS_HZ, &flags, &status);
  MUTEX_EXIT_IRQRESTORE (portc->peer->mutex, flags);

  return 0;
}

/*ARGSUSED*/
static int
audioloop_alloc_buffer (int dev, dmap_t * dmap, int direction)
{
#define MY_BUFFSIZE (64*1024)
  if (dmap->dmabuf != NULL)
    return 0;
  dmap->dmabuf_phys = 0;	/* Not mmap() capable */
  dmap->dmabuf = KERNEL_MALLOC (MY_BUFFSIZE);
  if (dmap->dmabuf == NULL)
    return OSS_ENOSPC;
  dmap->buffsize = MY_BUFFSIZE;

  return 0;
}

/*ARGSUSED*/
static int
audioloop_free_buffer (int dev, dmap_t * dmap, int direction)
{
  if (dmap->dmabuf == NULL)
    return 0;
  KERNEL_FREE (dmap->dmabuf);

  dmap->dmabuf = NULL;
  return 0;
}

#if 0
static int
audioloop_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
}
#endif

static audiodrv_t audioloop_server_driver = {
  audioloop_server_open,
  audioloop_server_close,
  audioloop_output_block,
  audioloop_start_input,
  audioloop_ioctl,
  audioloop_server_prepare_for_input,
  audioloop_server_prepare_for_output,
  audioloop_reset,
  NULL,
  NULL,
  NULL,
  NULL,
  audioloop_trigger,
  audioloop_server_set_rate,
  audioloop_server_set_format,
  audioloop_server_set_channels,
  NULL,
  NULL,
  audioloop_check_input,
  audioloop_check_output,
  audioloop_alloc_buffer,
  audioloop_free_buffer,
  NULL,
  NULL,
  NULL				/* audioloop_get_buffer_pointer */
};

static audiodrv_t audioloop_client_driver = {
  audioloop_client_open,
  audioloop_client_close,
  audioloop_output_block,
  audioloop_start_input,
  audioloop_ioctl,
  audioloop_client_prepare_for_input,
  audioloop_client_prepare_for_output,
  audioloop_reset,
  NULL,
  NULL,
  NULL,
  NULL,
  audioloop_trigger,
  audioloop_client_set_rate,
  audioloop_client_set_format,
  audioloop_client_set_channels,
  NULL,
  NULL,
  audioloop_check_input,
  audioloop_check_output,
  audioloop_alloc_buffer,
  audioloop_free_buffer,
  NULL,
  NULL,
  NULL				/* audioloop_get_buffer_pointer */
};


static int
install_server (audioloop_devc_t * devc, int num)
{
  audioloop_portc_t *portc;
  char tmp[64], devname[16];
  int adev;

  int opts =
    ADEV_STEREOONLY | ADEV_16BITONLY | ADEV_VIRTUAL |
    ADEV_FIXEDRATE | ADEV_SPECIAL;

  if ((portc = PMALLOC (devc->osdev, sizeof (*portc))) == NULL)
    return OSS_ENOMEM;
  memset (portc, 0, sizeof (*portc));

  portc->devc = devc;
  MUTEX_INIT (devc->osdev, portc->mutex, MH_DRV + 1);
  if ((portc->wq = oss_create_wait_queue (devc->osdev, "audioloop")) == NULL)
    {
      cmn_err (CE_WARN, "Cannot create audioloop wait queue\n");
      return OSS_EIO;
    }

  portc->instance = num;
  portc->port_type = PT_SERVER;

  devc->server_portc[num] = portc;

  sprintf (devname, "server%d", num);

  sprintf (tmp, "Audio loopback %d server side", num);

  if ((adev = oss_install_audiodev_with_devname (OSS_AUDIO_DRIVER_VERSION,
				    devc->osdev,
				    devc->osdev,
				    tmp,
				    &audioloop_server_driver,
				    sizeof (audiodrv_t),
				    opts, SUPPORTED_FORMATS, devc, -1,
				    devname)) < 0)
    {
      return adev;
    }

  audio_engines[adev]->portc = portc;
  audio_engines[adev]->min_rate = 5000;
  audio_engines[adev]->max_rate = MAX_RATE;
  audio_engines[adev]->min_channels = 1;
  audio_engines[adev]->caps |= PCM_CAP_HIDDEN;
  audio_engines[adev]->max_channels = MAX_CHANNELS;

  portc->audio_dev = adev;
  portc->rate = 48000;
  portc->fmt = AFMT_S16_NE;
  portc->fmt_bytes = 2;
  portc->channels = 2;

  return 0;
}


static int
install_client (audioloop_devc_t * devc, int num)
{
  audioloop_portc_t *portc;
  char tmp[64];
  int adev;

  int opts =
    ADEV_STEREOONLY | ADEV_16BITONLY | ADEV_VIRTUAL |
    ADEV_FIXEDRATE | ADEV_SPECIAL | ADEV_LOOP;

  if ((portc = PMALLOC (devc->osdev, sizeof (*portc))) == NULL)
    return OSS_ENOMEM;
  memset (portc, 0, sizeof (*portc));

  portc->devc = devc;
  if ((portc->wq = oss_create_wait_queue (devc->osdev, "audioloop")) == NULL)
    {
      cmn_err (CE_WARN, "Cannot create audioloop wait queue\n");
      return OSS_EIO;
    }

  portc->instance = num;
  portc->port_type = PT_CLIENT;

  devc->client_portc[num] = portc;

  sprintf (tmp, "Audio loopback %d", num);

  if ((adev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
				    devc->osdev,
				    devc->osdev,
				    tmp,
				    &audioloop_client_driver,
				    sizeof (audiodrv_t),
				    opts, SUPPORTED_FORMATS, devc, -1)) < 0)
    {
      return adev;
    }

  audio_engines[adev]->portc = portc;
  audio_engines[adev]->min_rate = 5000;
  audio_engines[adev]->max_rate = MAX_RATE;
  audio_engines[adev]->min_channels = 1;
  audio_engines[adev]->max_channels = MAX_CHANNELS;;
  audio_engines[adev]->enabled = 0;	/* Not enabled until server side is opened */

  portc->audio_dev = adev;


  return 0;
}

int
oss_audioloop_attach (oss_device_t * osdev)
{
  audioloop_devc_t *devc;
  int i;

  if (attach_count >= MAX_ATTACH_COUNT)
    {
      cmn_err (CE_WARN, "Attach limit reached (%d)\n", MAX_ATTACH_COUNT);
      return 0;
    }

  if (audioloop_instances < 1)
    audioloop_instances = 1;
  if (audioloop_instances > MAX_INSTANCES)
    audioloop_instances = MAX_INSTANCES;

  DDB (cmn_err
       (CE_CONT, "Initailzing audioloop %d instances\n",
	audioloop_instances));
  devc = &audioloop_devices[0];

  osdev->devc = devc;
  devc->osdev = osdev;
  MUTEX_INIT (osdev, devc->mutex, MH_DRV);

  oss_register_device (osdev, "audioloop");

  for (i = 0; i < audioloop_instances; i++)
    {
      if (install_server (devc, i) < 0)
	break;

      if (install_client (devc, i) < 0)
	break;

      devc->client_portc[i]->peer = devc->server_portc[i];
      devc->server_portc[i]->peer = devc->client_portc[i];
      devc->num_instances = i + 1;
    }

  return 1;
}

int
oss_audioloop_detach (oss_device_t * osdev)
{
  audioloop_devc_t *devc = osdev->devc;
  int i;

  if (oss_disable_device (osdev) < 0)
    return 0;

  MUTEX_CLEANUP (devc->mutex);

  for (i = 0; i < devc->num_instances; i++)
    {
      MUTEX_CLEANUP (devc->server_portc[i]->mutex);
    }

  oss_unregister_device (osdev);

  return 1;
}
