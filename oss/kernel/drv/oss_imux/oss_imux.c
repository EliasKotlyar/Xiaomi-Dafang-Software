/*
 * Purpose: Pseudo driver for sharing one input device between multiple apps.
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

#define MAX_IMUX_INSTANCES 16
#define MAX_IMUX_DEV	48
#define SUPPORTED_FORMATS (AFMT_S16_NE|AFMT_S16_OE)

#include "oss_imux_cfg.h"

static unsigned long long used_masterdevs = 0LL;

typedef struct
{
  int audio_dev;
  int port_number;
  int is_opened, is_prepared, is_triggered;
  int speed, channels, fmt;
  int ch_index;
  int left_igain, right_igain;
  oss_peaks_t peaks;
}
imux_portc;

typedef struct
{
  oss_device_t *osdev;
  oss_device_t *master_osdev;
  oss_mutex_t mutex;
  int installed_ok;
  int hw_dev;
  int hw_speed, hw_channels, hw_fmt, sw_fmt;
  int hw_fragsize;

  int device_started;
  struct fileinfo finfo;

  int nr_devices, open_count;
  imux_portc portc[MAX_IMUX_DEV];
  int fragsize;
  int prev_fragment;

/*
 * Mixer
 */
  int mixer_dev;
  int autoreset;		/* Autoreset igain sliders to 100 during open */

/*
 * Startup info
 */
  int imux_devices;
  int imux_rate;
  int imux_masterdev;
  int instance_no;
}
imux_devc;

static imux_devc imux_info[MAX_IMUX_INSTANCES] = { {0} };
static int nimuxdevs = 0;

/*ARGSUSED*/
static int
imux_set_rate (int dev, int arg)
{
  imux_devc *devc = audio_engines[dev]->devc;
  return devc->hw_speed;
}

/*ARGSUSED*/
static short
imux_set_channels (int dev, short arg)
{
  imux_portc *portc = audio_engines[dev]->portc;
  return portc->channels = 2;
}

/*ARGSUSED*/
static unsigned int
imux_set_format (int dev, unsigned int arg)
{
  imux_devc *devc = audio_engines[dev]->devc;

  return devc->sw_fmt;
}


static int
imux_igain (int dev, int ctrl, unsigned int cmd, int value)
{
/*
 * Access function for IMUX input gain
 */
  int left, right;
  imux_devc *devc = mixer_devs[dev]->devc;
  imux_portc *portc;

  if (ctrl != 100 && (ctrl < 0 || ctrl >= devc->imux_devices))
    return OSS_EINVAL;

  portc = &devc->portc[ctrl];

  if (cmd == SNDCTL_MIX_READ)
    {
      if (ctrl == 100)
	return devc->autoreset;
      return (portc->left_igain & 0x00ff) |
	((portc->right_igain & 0x00ff) << 8);
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
      if (ctrl == 100)
	return devc->autoreset = !!value;

      left = value & 0x00ff;
      right = (value >> 8) & 0x00ff;
      if (left < 0)
	left = 0;
      if (left > 100)
	left = 100;
      if (right < 0)
	right = 0;
      if (right > 100)
	right = 100;

      portc->left_igain = left;
      portc->right_igain = right;
      return left | (right << 8);
    }

  return OSS_EINVAL;
}

static int
imux_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  imux_devc *devc = audio_engines[dev]->devc;
  imux_portc *portc = audio_engines[dev]->portc;
  int value, i, n, p;

  switch (cmd)
    {
    case SNDCTL_DSP_SET_RECSRC:
      {
	value = *arg;
	if (value <= 0 || value >= devc->hw_channels - 1)
	  return OSS_EINVAL;
	portc->ch_index = value;
      }

    case SNDCTL_DSP_GET_RECSRC:
      return *arg = portc->ch_index;
      break;

    case SNDCTL_DSP_GET_RECSRC_NAMES:
      {
	oss_mixer_enuminfo *ei = (oss_mixer_enuminfo *) arg;

	memset (ei, 0, sizeof (*ei));

	n = ei->nvalues = devc->hw_channels - 1;
	p = 0;

	if (n <= 1)		/* Only one alternative */
	  {
	    ei->nvalues = 1;
	    ei->strindex[0] = 0;
	    sprintf (ei->strings, "default");

	  }
	else
	  /* Multiple alternatives */
	  for (i = 0; i < n; i++)
	    {
	      ei->strindex[i] = p;

	      sprintf (&ei->strings[p], "CH%d/%d", i + 1, i + 2);

	      p += strlen (&ei->strings[p]) + 1;
	    }

	return 0;
      }
      break;

    case SNDCTL_DSP_SETRECVOL:
      value = *arg;
      return *arg = imux_igain (audio_engines[dev]->mixer_dev,
				portc->ch_index, SNDCTL_MIX_WRITE, value);
      break;

    case SNDCTL_DSP_GETRECVOL:
      return *arg = imux_igain (audio_engines[dev]->mixer_dev,
				portc->ch_index, SNDCTL_MIX_READ, 0);
      break;

    case SNDCTL_DSP_GETIPEAKS:
      memset (arg, 0, sizeof (oss_peaks_t));
      memcpy (arg, portc->peaks, sizeof (portc->peaks));
      memset (portc->peaks, 0, sizeof (portc->peaks));
      return 0;
      break;
    }
  return OSS_EINVAL;
}

static void imux_trigger (int dev, int state);

static void
imux_reset (int dev)
{
  imux_trigger (dev, 0);
}

/*ARGSUSED*/
static void
bufcpy16ne (imux_devc * devc, imux_portc * portc, unsigned char *tbuf,
	    unsigned char *buf, int nbytes, int frame_size, int hw_framesize)
{
  int ns, i, hw_channels;
  short *p1, *p2;

  ns = nbytes / frame_size;

  hw_channels = hw_framesize / sizeof (*p2);

  p1 = (short *) tbuf;
  p2 = (short *) (buf + portc->ch_index * frame_size / 2);

  for (i = 0; i < ns; i++)
    {
      int v;

      /* Left channel */
      v = *p2++;
      v = v * portc->left_igain / 100;
      *p1++ = v;
      if (v < 0)
	v = -v;
      if (v > portc->peaks[0])
	portc->peaks[0] = v;

      /* Right channel */
      v = *p2++;
      v = v * portc->right_igain / 100;
      *p1++ = v;
      if (v < 0)
	v = -v;
      if (v > portc->peaks[1])
	portc->peaks[1] = v;

      if (hw_channels > 2)
	p2 += hw_channels - 2;
    }
}

/*ARGSUSED*/
static void
bufcpy16oe (imux_devc * devc, imux_portc * portc, unsigned char *tbuf,
	    unsigned char *buf, int nbytes, int frame_size, int hw_framesize)
{
  int ns, i, hw_channels;
  short *p1, *p2;

  ns = nbytes / frame_size;

  hw_channels = hw_framesize / sizeof (*p2);

  p1 = (short *) tbuf;
  p2 = (short *) (buf + portc->ch_index * frame_size / 2);

  for (i = 0; i < ns; i++)
    {
      int v;

      /* Left channel */
      v = *p2++;
      v = ((v >> 8) & 0xff) | ((v & 0xff) << 8);
      v = v * portc->left_igain / 100;
      *p1++ = v;
      if (v < 0)
	v = -v;
      if (v > portc->peaks[0])
	portc->peaks[0] = v;

      /* Right channel */
      v = *p2++;
      v = ((v >> 8) & 0xff) | ((v & 0xff) << 8);
      v = v * portc->right_igain / 100;
      *p1++ = v;
      if (v < 0)
	v = -v;
      if (v > portc->peaks[1])
	portc->peaks[1] = v;

      if (hw_channels > 2)
	p2 += hw_channels - 2;
    }
}

/*ARGSUSED*/
static void
imux_callback (int dev, int parm)
{
  dmap_t *dmap;
  unsigned char *buf, *tbuf;
  int i, len, frag, tail, next, n;
  imux_devc *devc = NULL;

  for (i = 0; i < nimuxdevs && devc == NULL; i++)
    {
      if (imux_info[i].hw_dev == dev)
	devc = &imux_info[i];
    }

  if (devc == NULL)
    {
      cmn_err (CE_WARN, "IMUX error\n");
      return;
    }

  dmap = audio_engines[dev]->dmap_in;

  frag = dmap_get_qhead (dmap);
  tail = dmap_get_qtail (dmap);
  next = (frag + 1) % dmap->nfrags;

  n = 0;
  while (n++ < 2 && frag != tail && next != tail
	 && devc->prev_fragment != frag)
    {
      devc->prev_fragment = frag;
      buf = &dmap->dmabuf[frag * dmap->fragment_size];
      len = dmap->fragment_size;

      dmap->user_counter += dmap->fragment_size;

      for (i = 0; i < devc->nr_devices; i++)
	{
	  dmap_t *client_dmap;

	  imux_portc *portc = &devc->portc[i];
	  client_dmap = audio_engines[portc->audio_dev]->dmap_in;

	  if (!portc->is_opened || !portc->is_triggered)
	    continue;

	  if (client_dmap->fragment_size != (2 * len) / devc->hw_channels)
	    {
	      DDB (cmn_err (CE_WARN, "Fragment warning (%d, %d, %d-%d)\n",
			    client_dmap->fragment_size, len,
			    audio_engines[portc->audio_dev]->min_block,
			    audio_engines[portc->audio_dev]->max_block));
#if 1
	      /* Make automatic adjustments */
	      client_dmap->fragment_size = (2 * len) / devc->hw_channels;
	      client_dmap->nfrags =
		client_dmap->buffsize / client_dmap->fragment_size;
	      client_dmap->bytes_in_use =
		client_dmap->nfrags * client_dmap->fragment_size;
#else
	      continue;
#endif
	    }

	  tbuf =
	    &client_dmap->dmabuf[dmap_get_qtail (client_dmap) *
				 client_dmap->fragment_size];

	  if (devc->hw_fmt == AFMT_S16_NE)	/* Native 16 bits */
	    {
	      bufcpy16ne (devc, portc, tbuf, buf,
			  client_dmap->fragment_size,
			  client_dmap->frame_size, dmap->frame_size);
	    }
	  else if (devc->hw_fmt == AFMT_S16_OE)	/* Native 16 bits */
	    {
	      bufcpy16oe (devc, portc, tbuf, buf,
			  client_dmap->fragment_size,
			  client_dmap->frame_size, dmap->frame_size);
	    }
	  else
	    memcpy (tbuf, buf, len);
#ifdef DO_TIMINGS
	  oss_timing_printf ("Imux: copy f=%d/%d to ch=%d t=%d/%d", frag, tail,
		     i, dmap_get_qtail (client_dmap),
		     dmap_get_qhead (client_dmap));
#endif
	  oss_audio_inputintr (portc->audio_dev, 0);

	}
      frag = dmap_get_qhead (dmap);
      tail = dmap_get_qtail (dmap);
      next = (frag + 1) % dmap->nfrags;
    }


}

static int
start_device (imux_devc * devc)
{
  int i, err, dev, trig, nc;
  /* int frags = 0x7fff0008;    fragment size of 256 bytes */

  if (devc->hw_dev < 0 || devc->hw_dev >= num_audio_engines)
    {
      devc->hw_dev = 0;
    }

  if (devc->hw_dev < 0 || devc->hw_dev >= num_audio_engines)
    {
      cmn_err (CE_WARN, "No audio hardware available\n");
      return OSS_ENXIO;
    }

  if (devc->device_started)
    {
      return 1;
    }

  devc->finfo.mode = OPEN_READ;
  devc->finfo.acc_flags = 0;
  devc->prev_fragment = -1;

  DDB (cmn_err (CE_NOTE,
		"Instance %d: Will use audio device %d as the master\n",
		devc->instance_no, devc->hw_dev));

  if (devc->hw_dev >= devc->portc[0].audio_dev
      && devc->hw_dev <= devc->portc[devc->imux_devices - 1].audio_dev)
    {
      cmn_err (CE_WARN, "Bad master device %d\n", devc->hw_dev);
      return OSS_ENXIO;
    }

  if (!(audio_engines[devc->hw_dev]->iformat_mask & SUPPORTED_FORMATS))
    {
      cmn_err (CE_CONT,
	       "Audio device %d doesn't support compatible sample formats.\n",
	       devc->hw_dev);
      return OSS_EIO;
    }
  if ((err =
       oss_audio_open_engine (devc->hw_dev, OSS_DEV_DSP, &devc->finfo, 1, 0,
			      NULL)) < 0)
    {
      return err;
    }
  audio_engines[devc->hw_dev]->cooked_enable = 0;
  strcpy (audio_engines[devc->hw_dev]->cmd, "IMUX");
  audio_engines[devc->hw_dev]->pid = 0;

  devc->hw_fmt =
    oss_audio_set_format (devc->hw_dev, devc->hw_fmt,
			  audio_engines[devc->hw_dev]->iformat_mask);
  devc->device_started = 1;
  devc->hw_channels = 2;

  devc->hw_channels =
    oss_audio_set_channels (devc->hw_dev, devc->hw_channels);

/*
 * TODO: Turn off OPT_SHADOW flag from the virtual devices if the master device 
 *      works in multi channel mode.
 */
  devc->hw_speed = oss_audio_set_rate (devc->hw_dev, devc->hw_speed);

#if 0
  oss_audio_ioctl (devc->hw_dev, NULL, SNDCTL_DSP_SETFRAGMENT,
		   (ioctl_arg) & frags);
#endif
  oss_audio_ioctl (devc->hw_dev, NULL, SNDCTL_DSP_GETBLKSIZE,
		   (ioctl_arg) & devc->fragsize);

  if (!(devc->hw_fmt & SUPPORTED_FORMATS))
    {
      oss_audio_release (devc->hw_dev, &devc->finfo);
      cmn_err (CE_WARN,
	       "This device doesn't support any known sample formats (%x)\n",
	       devc->hw_fmt);
      return OSS_EIO;
    }

  switch (devc->hw_fmt)
    {
    case AFMT_S16_NE:
      devc->sw_fmt = AFMT_S16_NE;
      break;

    case AFMT_S16_OE:
      devc->sw_fmt = AFMT_S16_NE;
      break;

    default:
      cmn_err (CE_WARN, "Bad sample format %x\n", devc->hw_fmt);
    }

  nc = devc->hw_channels;
  if (nc < 2)
    {
      oss_audio_release (devc->hw_dev, &devc->finfo);
      cmn_err (CE_WARN, "A 2 channel soundcard (or better) is required\n");
      return OSS_EIO;
    }

  DDB (cmn_err (CE_CONT, "Started audio device %d, s=%d, c=%d, bits=%d\n",
		devc->hw_dev, devc->hw_speed, devc->hw_channels,
		devc->hw_fmt));

  trig = 0;
  oss_audio_ioctl (devc->hw_dev, NULL, SNDCTL_DSP_SETTRIGGER,
		   (ioctl_arg) & trig);
  trig = PCM_ENABLE_INPUT;
  oss_audio_ioctl (devc->hw_dev, NULL, SNDCTL_DSP_SETTRIGGER,
		   (ioctl_arg) & trig);

  devc->hw_fragsize = audio_engines[devc->hw_dev]->dmap_out->fragment_size;

  for (i = 0; i < devc->nr_devices; i++)
    {
      int bz = (2 * devc->hw_fragsize) / devc->hw_channels;
      dev = devc->portc[i].audio_dev;
      audio_engines[dev]->fixed_rate = devc->hw_speed;
      audio_engines[dev]->min_block = bz;
      audio_engines[dev]->max_block = bz;
    }
  audio_engines[devc->hw_dev]->dmap_in->audio_callback = imux_callback;

  return 1;
}

static void
stop_device (imux_devc * devc)
{
  if (!devc->device_started)
    return;

  oss_audio_ioctl (devc->hw_dev, NULL, SNDCTL_DSP_HALT, 0);
  oss_audio_release (devc->hw_dev, &devc->finfo);
  audio_engines[devc->hw_dev]->pid = -1;
  memset (audio_engines[devc->hw_dev]->cmd, 0,
	  sizeof (audio_engines[devc->hw_dev]->cmd));
  audio_engines[devc->hw_dev]->flags &= ~ADEV_NOSRC;

  devc->device_started = 0;
}

/*ARGSUSED*/
static int
imux_open (int dev, int mode, int open_flags)
{
  imux_portc *portc = audio_engines[dev]->portc;
  imux_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;
  int bz, err;

  if (devc->hw_dev < 0)
    {
      cmn_err (CE_NOTE, "No master device allocated\n");
      return OSS_EIO;
    }

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (portc->is_opened)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  portc->is_opened = 1;
  portc->is_prepared = 0;
  portc->is_triggered = 0;
  portc->channels = 2;
  portc->ch_index = 0;
  memset (portc->peaks, 0, sizeof (portc->peaks));
  portc->fmt = AFMT_S16_NE;
  audio_engines[dev]->rate_source = audio_engines[devc->hw_dev]->rate_source;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  if (!devc->device_started)
    if ((err = start_device (devc)) < 0)
      {
	portc->is_opened = 0;
	return err;
      }

  bz = (2 * devc->hw_fragsize) / devc->hw_channels;
  audio_engines[dev]->min_block = audio_engines[dev]->max_block = bz;
  portc->ch_index = portc->port_number % (devc->hw_channels / 2);
  portc->speed = devc->hw_speed;

  if (devc->autoreset)
    {
      portc->left_igain = 100;
      portc->right_igain = 100;
      mixer_devs[devc->mixer_dev]->modify_counter++;
    }

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  devc->open_count++;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/
static void
imux_close (int dev, int mode)
{
  imux_portc *portc = audio_engines[dev]->portc;
  imux_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;
  int count;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  portc->is_triggered = 0;
  count = --devc->open_count;
  /* MUTEX_EXIT_IRQRESTORE (devc->mutex, flags); */

  if (count == 0)		/* Last one? */
    stop_device (devc);

  /* MUTEX_ENTER_IRQDISABLE (devc->mutex, flags); */
  portc->is_opened = 0;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
static void
imux_output_block (int dev, oss_native_word buf, int count, int fragsize,
		   int intrflag)
{
}

/*ARGSUSED*/
static void
imux_start_input (int dev, oss_native_word buf, int count, int fragsize,
		  int intrflag)
{
}

static void
imux_trigger (int dev, int state)
{
  imux_portc *portc = audio_engines[dev]->portc;

  if (state & PCM_ENABLE_INPUT)
    portc->is_triggered = 1;
  else
    portc->is_triggered = 0;
}

/*ARGSUSED*/
static int
imux_prepare_for_input (int dev, int bsize, int bcount)
{
  return 0;
}

/*ARGSUSED*/
static int
imux_prepare_for_output (int dev, int bsize, int bcount)
{
  return OSS_EIO;
}

/*ARGSUSED*/
static int
imux_alloc_buffer (int dev, dmap_t * dmap, int direction)
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
imux_free_buffer (int dev, dmap_t * dmap, int direction)
{
  if (dmap->dmabuf == NULL)
    return 0;
  KERNEL_FREE (dmap->dmabuf);

  dmap->dmabuf = NULL;
  return 0;
}

#if 0
static int
imux_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
}
#endif

static audiodrv_t imux_driver = {
  imux_open,
  imux_close,
  imux_output_block,
  imux_start_input,
  imux_ioctl,
  imux_prepare_for_input,
  imux_prepare_for_output,
  imux_reset,
  NULL,
  NULL,
  NULL,
  NULL,
  imux_trigger,
  imux_set_rate,
  imux_set_format,
  imux_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  imux_alloc_buffer,
  imux_free_buffer,
  NULL,
  NULL,
  NULL				/* imux_get_buffer_pointer */
};

/*ARGSUSED*/
static int
imux_mixer_ioctl (int dev, int audiodev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static mixer_driver_t imux_mixer_driver = {
  imux_mixer_ioctl
};

static const unsigned char peak_cnv[256] = {
  0, 18, 29, 36, 42, 47, 51, 54, 57, 60, 62, 65, 67, 69, 71, 72,
  74, 75, 77, 78, 79, 81, 82, 83, 84, 85, 86, 87, 88, 89, 89, 90,
  91, 92, 93, 93, 94, 95, 95, 96, 97, 97, 98, 99, 99, 100, 100, 101,
  101, 102, 102, 103, 103, 104, 104, 105, 105, 106, 106, 107, 107, 108, 108,
  108,
  109, 109, 110, 110, 110, 111, 111, 111, 112, 112, 113, 113, 113, 114, 114,
  114,
  115, 115, 115, 115, 116, 116, 116, 117, 117, 117, 118, 118, 118, 118, 119,
  119,
  119, 119, 120, 120, 120, 121, 121, 121, 121, 122, 122, 122, 122, 122, 123,
  123,
  123, 123, 124, 124, 124, 124, 125, 125, 125, 125, 125, 126, 126, 126, 126,
  126,
  127, 127, 127, 127, 127, 128, 128, 128, 128, 128, 129, 129, 129, 129, 129,
  130,
  130, 130, 130, 130, 130, 131, 131, 131, 131, 131, 131, 132, 132, 132, 132,
  132,
  132, 133, 133, 133, 133, 133, 133, 134, 134, 134, 134, 134, 134, 134, 135,
  135,
  135, 135, 135, 135, 135, 136, 136, 136, 136, 136, 136, 136, 137, 137, 137,
  137,
  137, 137, 137, 138, 138, 138, 138, 138, 138, 138, 138, 139, 139, 139, 139,
  139,
  139, 139, 139, 140, 140, 140, 140, 140, 140, 140, 140, 141, 141, 141, 141,
  141,
  141, 141, 141, 141, 142, 142, 142, 142, 142, 142, 142, 142, 142, 143, 143,
  143,
  143, 143, 143, 143, 143, 143, 144, 144, 144, 144, 144, 144, 144, 144, 144,
  144,
};

/*ARGSUSED*/
static int
imux_vu (int dev, int ctrl, unsigned int cmd, int value)
{
/*
 * Access function for PCM VU meters
 */
  int left, right;
  imux_devc *devc = mixer_devs[dev]->devc;

  if (ctrl < 0 || ctrl >= devc->imux_devices)
    return OSS_EINVAL;

  if (cmd == SNDCTL_MIX_READ)
    {
      imux_portc *portc = &devc->portc[ctrl];
      left = peak_cnv[(portc->peaks[0] * 144) / 32768];
      right = peak_cnv[(portc->peaks[1] * 144) / 32768];
      memset (portc->peaks, 0, sizeof (portc->peaks));
      return left | (right << 8);
    }

  return OSS_EINVAL;
}

static int
imux_mix_init (int dev)
{
  int group;
  imux_devc *devc = mixer_devs[dev]->devc;
  int i;
  int err;

  if ((group = mixer_ext_create_group (dev, 0, "CLIENT")) < 0)
    return group;

  for (i = 0; i < devc->imux_devices; i++)
    {
      char tmp[32];

      sprintf (tmp, "@pcm%d", devc->portc[i].audio_dev);

      if ((err = mixer_ext_create_control (dev, group, i, imux_igain,
					   MIXT_STEREOSLIDER,
					   tmp, 100,
					   MIXF_READABLE | MIXF_WRITEABLE)) <
	  0)
	return err;

      if ((err = mixer_ext_create_control (dev, group, i, imux_vu,
					   MIXT_STEREOPEAK,
					   "-", 144, MIXF_READABLE)) < 0)
	return err;
    }

  if ((err = mixer_ext_create_control (dev, group, 100, imux_igain,
				       MIXT_ONOFF,
				       "autoreset", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;
  return 0;
}

static int
find_master_device (imux_devc * devc)
{
/*
 * Find a suitable master device.
 *
 * Return 1 if found and 0 if not.
 */
  int dev;
  adev_p adev;

  if (num_audio_devfiles < 1)
    return 0;

  if (devc->imux_masterdev >= 0)
    {
/*
 * imux_masterdev property was given. Use the given /dev/dsp# number
 * as the master device.
 */
      if (devc->imux_masterdev >= num_audio_devfiles)
	return 0;		/* Device not attached yet */
      devc->hw_dev = audio_devfiles[devc->imux_masterdev]->engine_num;
      adev = audio_engines[devc->hw_dev];

      if (adev->flags & (ADEV_NOINPUT))
	{
	  cmn_err (CE_NOTE,
		   "Audio device %d cannot be used as an IMUX master device\n",
		   devc->imux_masterdev);
	  devc->hw_dev = -1;
	  return 0;
	}

      devc->hw_dev = adev->engine_num;
      /* TODO: Prevent the other virtual drivers from picking this one */
    }
  else
    {
/*
 * Try to find if some of the devices currently available is suitable
 * master device.
 */
      devc->hw_dev = -1;

      for (dev = 0; dev < num_audio_engines; dev++)
	{
	  adev = audio_engines[dev];

	  if (adev->flags & ADEV_NOINPUT)
	    continue;

	  if (used_masterdevs & (1LL << dev))
	    continue;

	  devc->hw_dev = adev->engine_num;
	  break;
	}
    }

  return (devc->hw_dev >= 0);
}

static int
try_to_start (void *dc)
{
  int n, adev, opts;
  char tmp[32];
  imux_devc *devc = dc;
  int my_mixer;

  if (!find_master_device (devc))
    return 0;

/*
 * If the situation is hopeless then just tell the caller to stop
 * trying this operation.
 */
  if (devc->hw_dev < 0)
    return 1;

  if (used_masterdevs & (1LL << devc->hw_dev))
    {
      devc->hw_dev = -1;
      cmn_err (CE_WARN,
	       "Selected master device % for IMUX instance %d already used for some other instance\n",
	       devc->osdev->instance, devc->hw_dev);
      return 1;
    }

  used_masterdevs |= (1LL << devc->hw_dev);

  opts =
    ADEV_STEREOONLY | ADEV_16BITONLY | ADEV_VIRTUAL | ADEV_NOOUTPUT |
    ADEV_FIXEDRATE | ADEV_NOMMAP;

  devc->nr_devices = 0;
  devc->open_count = 0;

  devc->hw_speed = devc->imux_rate;
  devc->master_osdev = audio_engines[devc->hw_dev]->osdev;
  devc->installed_ok = 1;
  devc->autoreset = 1;
  MUTEX_INIT (devc->master_osdev, devc->mutex, MH_DRV + 4);

  if ((my_mixer = oss_install_mixer (OSS_MIXER_DRIVER_VERSION,
				     devc->osdev,
				     devc->master_osdev,
				     "IMUX Control panel",
				     &imux_mixer_driver,
				     sizeof (mixer_driver_t), devc)) >= 0)
    {
      mixer_ext_set_init_fn (my_mixer, imux_mix_init, 32);
    }
  else
    my_mixer = -1;

  devc->mixer_dev = my_mixer;

  for (n = 0; n < devc->imux_devices; n++)
    {
      imux_portc *portc;

      if (n > 0)
	opts |= ADEV_SHADOW;

      sprintf (tmp, "IMux%d audio record", devc->instance_no);

      if ((adev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
					devc->osdev,
					devc->master_osdev,
					tmp,
					&imux_driver,
					sizeof (audiodrv_t),
					opts, AFMT_S16_NE, devc, -1)) < 0)
	{
	  return 1;
	}

      portc = &devc->portc[n];

      audio_engines[adev]->portc = portc;
      audio_engines[adev]->mixer_dev = my_mixer;
      audio_engines[adev]->min_rate = 5000;
      audio_engines[adev]->max_rate = 192000;
      portc->left_igain = 100;
      portc->right_igain = 100;

      portc->audio_dev = adev;
      portc->is_opened = 0;
      portc->is_prepared = 0;
      portc->is_triggered = 0;
      portc->speed = 48000;
      portc->channels = 2;
      portc->fmt = AFMT_S16_NE;
      portc->port_number = n;

      devc->nr_devices = n + 1;
    }

  return 1;
}


int
oss_imux_attach (oss_device_t * osdev)
{
  extern int imux_devices;
  extern int imux_masterdev;
  extern int imux_rate;
  imux_devc *devc;

  if (imux_devices < 2)
    imux_devices = 2;
  if (imux_devices > MAX_IMUX_DEV)
    imux_devices = MAX_IMUX_DEV;

  if (nimuxdevs >= MAX_IMUX_INSTANCES)
    {
      cmn_err (CE_NOTE, "Only %d instances permitted\n", MAX_IMUX_INSTANCES);
      return 0;
    }

  devc = &imux_info[nimuxdevs++];

  devc->osdev = osdev;
  osdev->devc = devc;

  devc->master_osdev = NULL;
  devc->instance_no = osdev->instance;
  devc->imux_devices = imux_devices;
  devc->imux_rate = imux_rate;
  devc->imux_masterdev = imux_masterdev;
  imux_masterdev = -1;
  devc->hw_dev = -1;
  devc->sw_fmt = AFMT_S16_NE;

  oss_register_device (osdev, "OSS IMUX driver");

  if (!try_to_start (devc))
    {
      oss_audio_register_client (try_to_start, devc, devc->osdev);
    }

  return 1;
}

int
oss_imux_detach (oss_device_t * osdev)
{
  imux_devc *devc = osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  if (devc->installed_ok)
    {
      MUTEX_CLEANUP (devc->mutex);
    }

  oss_unregister_device (devc->osdev);

  return 1;
}
