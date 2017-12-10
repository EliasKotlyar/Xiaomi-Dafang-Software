/*
 * Purpose: Multi channel playback support for devices with multiple stereo engines.
 *
 * Some sound cards don't provide single multi channel output engine. Instead
 * they have multiple stereo output pairs connected to the front, side, 
 * center/LFE and rear speakers. The remux driver is used by such drivers to
 * redistribute multi channel streams to the individual stereo engines.
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

#ifndef USE_REMUX
#error remux.c is not compatible with this architecture (endianess)
#endif

#ifdef OSS_BIG_ENDIAN
void
remux_install (char *name, oss_device_t * osdev, int frontdev, int reardev,
	       int center_lfe_dev, int surrounddev)
{
  /* Not compatible with big endian yet */
}

#else

#include "remux.h"

static char *chnames[MAX_SUBDEVS] = {
  "Front",
  "Surr",
  "C&L",
  "Rear"
};

static const int bindings[MAX_SUBDEVS] = {
  DSP_BIND_FRONT,
  DSP_BIND_SURR,
  DSP_BIND_CENTER_LFE,
  DSP_BIND_REAR
};

static remux_devc dev_info = { 0 }, *devc = &dev_info;

/*
 * Audio routines
 */
/*ARGSUSED*/
static void
remux_callback (int dev, int parm)
{
  oss_audio_outputintr (devc->audio_dev, 1);
}

static int
remux_set_rate (int dev, int arg)
{
  remux_devc *devc = audio_engines[dev]->devc;

  return devc->speed =
    audio_engines[devc->physdev[0]]->d->adrv_set_rate (devc->physdev[0], arg);
}

static short
remux_set_channels (int dev, short arg)
{
  remux_devc *devc = audio_engines[dev]->devc;

  if (arg == 0)
    return devc->channels;

  arg &= ~1;			/* Make sure we have even number of channels */

  if (arg < 2)
    return devc->channels = 2;
  if (arg > devc->maxchannels)
    return devc->channels = devc->maxchannels;

  return devc->channels = arg;
}

/*ARGSUSED*/
static unsigned int
remux_set_format (int dev, unsigned int arg)
{
  return AFMT_S16_NE;
}

/*ARGSUSED*/
static int
remux_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void remux_trigger (int dev, int state);

static void
remux_reset (int dev)
{
  remux_trigger (dev, 0);
}

/*ARGSUSED*/
static int
remux_open (int dev, int mode, int open_flags)
{
  remux_devc *devc = audio_engines[dev]->devc;
  adev_p adev = audio_engines[devc->audio_dev];
  oss_native_word flags;
  int i, j, err;

  if (mode & OPEN_READ)
    {
      cmn_err (CE_WARN, "Audio device %d cannot do recording\n", dev);
      /* return OSS_ENOTSUP; */
    }

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (devc->open_mode != 0)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

      return OSS_EBUSY;
    }
  devc->open_mode = mode;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  for (i = 0; i < devc->n_physdevs; i++)
    {
      adev_p pdev = audio_engines[devc->physdev[i]];

      devc->finfo[i].mode = OPEN_WRITE;
      devc->finfo[i].acc_flags = 0;
      if ((err =
	   oss_audio_open_engine (devc->physdev[i], OSS_DEV_DSP,
				  &devc->finfo[i], 1, OF_SMALLFRAGS,
				  NULL)) < 0)
	{
	  for (j = 0; j < i; j++)
	  {
	    oss_audio_release (devc->physdev[j], &devc->finfo[j]);
	  }

	  devc->open_mode = 0;
	  return err;
	}


      strcpy (pdev->cmd, chnames[i]);
      pdev->pid = 0;
      pdev->cooked_enable = 0;

      if (pdev->d->adrv_bind != NULL)
	{
	  int b = bindings[i];
	  pdev->d->adrv_bind (pdev->engine_num, SNDCTL_DSP_BIND_CHANNEL,
			      (ioctl_arg) & b);
	}

      if (pdev->flags & ADEV_FIXEDRATE)
	{
	  adev->flags |= ADEV_FIXEDRATE;
	  adev->fixed_rate = pdev->fixed_rate;
	}
      else
	adev->flags &= ~(ADEV_FIXEDRATE);

    }

  devc->speed = 48000;
  devc->channels = 2;
  adev->cooked_enable = 0;

  return 0;
}

/*ARGSUSED*/
static void
remux_close (int dev, int mode)
{
  int i;
  remux_devc *devc = audio_engines[dev]->devc;

  for (i = 0; i < devc->n_physdevs; i++)
    oss_audio_release (devc->physdev[i], &devc->finfo[i]);
  devc->open_mode = 0;
}

#if 0
static int sinebuf[48] = {

  0, 4276, 8480, 12539, 16383, 19947, 23169, 25995,
  28377, 30272, 31650, 32486, 32767, 32486, 31650, 30272,
  28377, 25995, 23169, 19947, 16383, 12539, 8480, 4276,
  0, -4276, -8480, -12539, -16383, -19947, -23169, -25995,
  -28377, -30272, -31650, -32486, -32767, -32486, -31650, -30272,
  -28377, -25995, -23169, -19947, -16383, -12539, -8480, -4276
};
#endif

/*ARGSUSED*/
static void
remux_output_block (int dev, oss_native_word buf, int count,
		    int fragsize, int intrflag)
{
/*
 * This routine does the actual de-interleaving of stereo pairs to the
 * individual devices. All data movement is done based on sample pairs
 * 2*16=32 bit (2*short=int). Native endianess (AFMT_S16_NE) is assumed.
 */
  adev_p adev = audio_engines[dev], pdev = audio_engines[devc->physdev[0]];
  remux_devc *devc = adev->devc;
  dmap_p dmap = adev->dmap_out, pdmap = pdev->dmap_out;

  int ch, nc, nf = pdmap->nfrags;

  int *inbuf;
  int ptr, pos = 0;

  ptr = ((unsigned long) dmap->byte_counter) % dmap->bytes_in_use;

  inbuf = (int *) (dmap->dmabuf + ptr);

  nc = devc->channels / 2;

  for (ch = 0; ch < nc; ch++)
    {
      int *outbuf;
      int i, ns;

      pdev = audio_engines[devc->physdev[ch]];
      pdmap = pdev->dmap_out;
      if (ch == 0)
	pos =
	  (pdmap->user_counter + pdmap->fragment_size) % pdmap->bytes_in_use;

      outbuf = (int *) (pdmap->dmabuf + pos);

      ns = dmap->fragment_size / (4 * nc);

      for (i = 0; i < ns; i++)
	{
#if 0
	  static int p[MAX_SUBDEVS] = { 0 };

	  short *s = (short *) &outbuf[i];
	  s[0] = s[1] = sinebuf[p[ch]];
	  p[ch] = (p[ch] + 1) % 48;
#else
	  outbuf[i] = inbuf[(i * nc) + ch];
#endif
	}

      pdmap->user_counter += ns * 4;
    }

  if (pdmap->user_counter - pdmap->byte_counter <
      pdmap->fragment_size * (nf / 2))
    {
      oss_audio_outputintr (devc->audio_dev, 1);
    }
}

/*ARGSUSED*/
static void
remux_start_input (int dev, oss_native_word buf, int count,
		   int fragsize, int intrflag)
{
}

static void
remux_trigger (int dev, int state)
{
  remux_devc *devc = audio_engines[dev]->devc;

  int i;

  for (i = 0; i < devc->n_physdevs; i++)
    {
      int pd = devc->physdev[i];
      adev_p pdev = audio_engines[pd];

      pdev->d->adrv_trigger (pd, state);
      if (state & PCM_ENABLE_OUTPUT)
	pdev->dmap_out->flags |= DMAP_STARTED;
      else
	pdev->dmap_out->flags &= ~DMAP_STARTED;
    }
}

/*ARGSUSED*/
static int
remux_prepare_for_input (int dev, int bsize, int bcount)
{
  return OSS_EIO;
}

static int
remux_prepare_for_output (int dev, int bsize, int bcount)
{
  remux_devc *devc = audio_engines[dev]->devc;
  adev_p adev = audio_engines[dev];
  int i, err, tmp, nd;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  nd = devc->channels / 2;
  bsize = 240;			/* Must be divisible by 3*4 and 4*4 */

  for (i = 0; i < devc->n_physdevs; i++)
    {
      int pd = devc->physdev[i];
      adev_p pdev = audio_engines[pd];
      dmap_p dmap = pdev->dmap_out;

      tmp = pdev->d->adrv_set_format (pd, AFMT_S16_NE);
      if (tmp != AFMT_S16_NE)
	{
	  cmn_err (CE_NOTE, "remux: Bad sample format %x\n", tmp);
	  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
	  return OSS_EIO;
	}

      tmp = pdev->d->adrv_set_channels (pd, 2);
      if (tmp != 2)
	{
	  cmn_err (CE_NOTE, "remux: Bad number of channels %d\n", tmp);
	  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
	  return OSS_EIO;
	}

      tmp = pdev->d->adrv_set_rate (pd, devc->speed);
      if (tmp != devc->speed)
	{
	  cmn_err (CE_NOTE, "remux: Bad sample rate %d\n", tmp);
	  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
	  return OSS_EIO;
	}

      if (pdev->min_block > 0 && bsize < pdev->min_block)
	bsize = pdev->min_block;
      if (pdev->max_block > 0 && bsize > pdev->max_block)
	bsize = pdev->max_block;

      dmap->fragment_size = bsize;

      bcount = dmap->buffsize / bsize;

      dmap->bytes_in_use = bcount * bsize;
      dmap->nfrags = bcount;

      if ((err = pdev->d->adrv_prepare_for_output (pd, bsize, bcount)) < 0)
	{
	  cmn_err (CE_WARN,
		   "remux: Preparing device #%d failed, error %d\n", pd, err);
	  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
	  return err;
	}

      if (dmap->dmabuf == NULL)
	{
	  cmn_err (CE_WARN, "dmabuf==NULL\n");
	  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
	  return OSS_ENOMEM;
	}

      memset (dmap->dmabuf, 0, dmap->buffsize);

      if (i == 0)
	dmap->audio_callback = remux_callback;
      dmap->dma_mode = PCM_ENABLE_OUTPUT;
      dmap->flags |= DMAP_PREPARED;
      dmap->data_rate = devc->speed * 4;
    }

  adev->dmap_out->fragment_size = bsize * nd;
  adev->dmap_out->nfrags =
    adev->dmap_out->bytes_in_use / adev->dmap_out->fragment_size;
  adev->dmap_out->nfrags &= ~1;

  if (adev->dmap_out->nfrags < 2)
    adev->dmap_out->nfrags = 2;
  adev->dmap_out->bytes_in_use =
    adev->dmap_out->fragment_size * adev->dmap_out->nfrags;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

static audiodrv_t remux_driver = {
  remux_open,
  remux_close,
  remux_output_block,
  remux_start_input,
  remux_ioctl,
  remux_prepare_for_input,
  remux_prepare_for_output,
  remux_reset,
  NULL,
  NULL,
  NULL,
  NULL,
  remux_trigger,
  remux_set_rate,
  remux_set_format,
  remux_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,				/* remux_alloc_buffer */
  NULL,				/* remux_free_buffer */
  NULL,
  NULL,
  NULL
};

static int
check_physdev (int dev)
{
  adev_p adev = audio_engines[dev];

  if (!(adev->oformat_mask & AFMT_S16_NE))
    {
      cmn_err (CE_NOTE,
	       "remux: Audio device %d doesn't support the 16 bit format\n",
	       dev);
      return 0;
    }

  if (adev->flags & (ADEV_NOOUTPUT))
    {
      cmn_err (CE_NOTE, "remux: Audio device %d doesn't support output\n",
	       dev);
      return 0;
    }

#if 0
  if (!(adev->flags & ADEV_AUTOMODE))
    {
      cmn_err (CE_NOTE, "remux: Audio device %d doesn't support auto-mode\n",
	       dev);
      return 0;
    }
#endif

  return 1;
}

static int
countdevs (int frontdev, int reardev, int center_lfe_dev, int surrounddev)
{
  if (frontdev < 0 || frontdev >= num_audio_engines)
    return 0;
  if (reardev < 0 || reardev >= num_audio_engines)
    return 2;
  if (center_lfe_dev < 0 || center_lfe_dev >= num_audio_engines)
    return 4;
  if (surrounddev < 0 || surrounddev >= num_audio_engines)
    return 6;

  return 8;
}

void
remux_install (char *name, oss_device_t * osdev, int frontdev, int reardev,
	       int center_lfe_dev, int surrounddev)
{
  adev_p adev, pdev;
  int n, i;

  DDB (cmn_err (CE_CONT, "remux install %s: %d, %d, %d, %d\n",
		name, frontdev, reardev, center_lfe_dev, surrounddev));

  devc->osdev = osdev;
  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV + 2);

  n = countdevs (frontdev, reardev, center_lfe_dev, surrounddev);

  if (n < 4)
    {
      cmn_err (CE_WARN, "remux: Bad devices (%d, %d, %d, %d)\n",
		      frontdev, reardev, center_lfe_dev, surrounddev);
      return;
    }

  devc->maxchannels = n;
  devc->n_physdevs = n / 2;
  devc->physdev[0] = frontdev;
  devc->physdev[1] = reardev;
  devc->physdev[2] = center_lfe_dev;
  devc->physdev[3] = surrounddev;

  for (i = 0; i < devc->n_physdevs; i++)
    if (!check_physdev (devc->physdev[i]))
      return;

  pdev = audio_engines[frontdev];

  if ((devc->audio_dev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
					       devc->osdev,
					       devc->osdev,
					       name,
					       &remux_driver,
					       sizeof (audiodrv_t),
					       ADEV_NOINPUT | ADEV_SPECIAL |
					       ADEV_DISABLE_VIRTUAL |
					       ADEV_NOSRC, AFMT_S16_NE, devc,
					       -1)) < 0)
    {
      devc->audio_dev = -1;
      return;
    }

  adev = audio_engines[devc->audio_dev];

  adev->devc = devc;
  adev->min_channels = 2;
  adev->max_channels = devc->maxchannels;
  adev->min_rate = pdev->min_rate;
  adev->max_rate = pdev->max_rate;
  adev->rate_source = frontdev;
  adev->caps |= DSP_CH_MULTI;
  if (pdev->caps & PCM_CAP_FREERATE)
    adev->caps |= PCM_CAP_FREERATE;

  devc->open_mode = 0;
}

#endif
