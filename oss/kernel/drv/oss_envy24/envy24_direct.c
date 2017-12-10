/*
 * Purpose: Direct 24 bit multich driver for Envy24.
 *
 * This driver implements the all inputs and all outputs audio devices for
 * envy24. Unlike the ordinary driver this one doesn't use additional buffering.
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
#include "ac97.h"

#ifdef USE_LICENSING
#include "private/licensing/licensing.h"
#endif

#include "envy24.h"

#define OUTCH_NAMES "CH1/2 CH3/4 CH5/6 CH7/8 digital"
#define INCH_NAMES "CH1/2 CH3/4 CH5/6 CH7/8 digital monitor_mix"

/*
 * Audio routines
 */
extern int envy24_audio_set_rate (int dev, int arg);
int envy24d_get_buffer_pointer (int dev, dmap_t * dmap, int direction);

void
envy24d_playintr (envy24_devc * devc)
{
  oss_audio_outputintr (devc->direct_portc_out.audio_dev, 0);
}

void
envy24d_recintr (envy24_devc * devc)
{
  oss_audio_inputintr (devc->direct_portc_in.audio_dev, 0);
}

/*ARGSUSED*/ 
static short
envy24d_audio_set_channels (int dev, short arg)
{
  envy24d_portc *portc = audio_engines[dev]->portc;
  return portc->channels;
}

/*ARGSUSED*/ 
static unsigned int
envy24d_audio_set_format (int dev, unsigned int arg)
{
  return AFMT_S32_LE;
}

/*ARGSUSED*/ 
static int
envy24d_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void envy24d_audio_trigger (int dev, int state);

static void
envy24d_audio_reset (int dev)
{
  envy24d_audio_trigger (dev, 0);
}

/*ARGSUSED*/ 
static int
envy24d_audio_open (int dev, int mode, int open_flags)
{
  envy24_devc *devc = audio_engines[dev]->devc;
  envy24d_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;

  if (!(mode & portc->direction))
    return OSS_EINVAL;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (portc->open_mode != 0 || (devc->direct_audio_opened & mode))
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  if (mode == OPEN_WRITE && (devc->play_channel_mask != 0))
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  if (mode == OPEN_READ && (devc->rec_channel_mask != 0))
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  portc->open_mode = mode;
  devc->direct_audio_opened |= mode;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

static void
envy24d_audio_close (int dev, int mode)
{
  envy24_devc *devc = audio_engines[dev]->devc;
  envy24d_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  portc->open_mode = 0;
  devc->direct_audio_opened &= ~mode;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/ 
static void
envy24d_audio_output_block (int dev, oss_native_word buf, int count,
			    int fragsize, int intrflag)
{
}

/*ARGSUSED*/ 
static void
envy24d_audio_start_input (int dev, oss_native_word buf, int count,
			   int fragsize, int intrflag)
{
}

static void
envy24d_audio_trigger (int dev, int state)
{
  envy24_devc *devc = audio_engines[dev]->devc;
  envy24d_portc *portc = audio_engines[dev]->portc;
  int changed;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  changed = state ^ portc->trigger_bits;

  if (portc->direction == OPEN_WRITE && (changed & PCM_ENABLE_OUTPUT))
    {
      if (state & PCM_ENABLE_OUTPUT)
	{
#ifdef DO_TIMINGS
	  oss_do_timing ("Envy24d: Trigger start output");
#endif
	  portc->trigger_bits = state;
	  envy24_launch_play_engine (devc);
	}
      else
	{
#ifdef DO_TIMINGS
	  oss_do_timing ("Envy24d: Trigger stop output");
#endif
	  portc->trigger_bits = state;
	  envy24_stop_playback (devc);
	}
    }

  if (portc->direction == OPEN_READ && (changed & PCM_ENABLE_INPUT))
    {
      if (state & PCM_ENABLE_INPUT)
	{
#ifdef DO_TIMINGS
	  oss_do_timing ("Envy24d: Trigger start input");
#endif
	  portc->trigger_bits = state;
	  envy24_launch_recording (devc);
	}
      else
	{
#ifdef DO_TIMINGS
	  oss_do_timing ("Envy24d: Trigger stop input");
#endif
	  portc->trigger_bits = state;
	  envy24_stop_recording (devc);
	}
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/ 
static int
envy24d_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  envy24_devc *devc = audio_engines[dev]->devc;
  envy24_start_recording (devc);
  return 0;
}

/*ARGSUSED*/ 
static int
envy24d_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  envy24_devc *devc = audio_engines[dev]->devc;

  envy24_prepare_play_engine (devc);
  return 0;
}

static int
envy24d_alloc_buffer (int dev, dmap_t * dmap, int direction)
{
  envy24_devc *devc = audio_engines[dev]->devc;

  if (direction == OPEN_WRITE)
    {
      if (devc->playbuf == NULL)
	{
	  cmn_err (CE_CONT, "Envy24: No playback DMA buffer available\n");
	  return OSS_ENOSPC;
	}

      dmap->dmabuf = devc->playbuf;
      dmap->buffsize = devc->playbuffsize;
      dmap->dmabuf_phys = devc->playbuf_phys;
      return 0;
    }

  if (direction == OPEN_READ)
    {
      if (devc->recbuf == NULL)
	{
	  cmn_err (CE_CONT, "Envy24: No recording DMA buffer available\n");
	  return OSS_ENOSPC;
	}

      dmap->dmabuf = devc->recbuf;
      dmap->buffsize = devc->recbuffsize;
      dmap->dmabuf_phys = devc->recbuf_phys;
      return 0;
    }

  return OSS_EIO;
}

/*ARGSUSED*/ 
static int
envy24d_free_buffer (int dev, dmap_t * dmap, int direction)
{
  dmap->dmabuf = NULL;
  dmap->dmabuf_phys = 0;
  dmap->buffsize = 0;
  return 0;
}

int
envy24d_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{

  envy24_devc *devc = audio_engines[dev]->devc;
  int pos;

  if (direction == PCM_ENABLE_OUTPUT)
    {
      pos = INW (devc->osdev, devc->mt_base + 0x14);
      pos = (pos + 1) * 4;
      pos = dmap->bytes_in_use - pos;

      return pos;
    }

  if (direction == PCM_ENABLE_INPUT)
    {
      pos = INW (devc->osdev, devc->mt_base + 0x24);
      pos = (pos + 1) * 4;
      pos = dmap->bytes_in_use - pos;

      return pos;
    }

  return OSS_EIO;
}

/*ARGSUSED*/ 
static int
envy24d_check_input (int dev)
{
  cmn_err (CE_WARN, "Envy24d: Input timed out.\n");
  return OSS_EIO;
}

/*ARGSUSED*/ 
static int
envy24d_check_output (int dev)
{
  cmn_err (CE_WARN, "Envy24d: Output timed out (%d)\n", GET_JIFFIES ());
  return OSS_EIO;
}

static const audiodrv_t envy24d_audio_driver = {
  envy24d_audio_open,
  envy24d_audio_close,
  envy24d_audio_output_block,
  envy24d_audio_start_input,
  envy24d_audio_ioctl,
  envy24d_audio_prepare_for_input,
  envy24d_audio_prepare_for_output,
  envy24d_audio_reset,
  NULL,
  NULL,
  NULL,
  NULL,
  envy24d_audio_trigger,
  envy24_audio_set_rate,
  envy24d_audio_set_format,
  envy24d_audio_set_channels,
  NULL,
  NULL,
  envy24d_check_input,
  envy24d_check_output,
  envy24d_alloc_buffer,
  envy24d_free_buffer,
  NULL,
  NULL,
  envy24d_get_buffer_pointer
};

void
envy24d_install (envy24_devc * devc)
{

  int adev, out_dev, in_dev;
  char tmpname[128];
  envy24d_portc *portc;

  /*
   * Output device
   */
  sprintf (tmpname, "%s (all outputs)", devc->model_data->product);

  if ((out_dev = adev = oss_install_audiodev_with_devname (OSS_AUDIO_DRIVER_VERSION,
				    devc->osdev,
				    devc->osdev,
				    tmpname,
				    &envy24d_audio_driver,
				    sizeof (audiodrv_t),
				    ADEV_NOVIRTUAL | ADEV_NOINPUT | ADEV_COLD
				    | ADEV_SPECIAL | ADEV_32BITONLY,
				    AFMT_S32_LE, devc, -1,
				    "10ch_out")) >= 0)
    {
      portc = &devc->direct_portc_out;
      audio_engines[adev]->portc = portc;
      audio_engines[adev]->rate_source = devc->first_dev;
      audio_engines[adev]->caps |= DSP_CH_MULTI;
      audio_engines[adev]->min_channels = 10;
      audio_engines[adev]->max_channels = 10;
      audio_engines[adev]->outch_names = OUTCH_NAMES;
      audio_engines[adev]->min_block = devc->hw_pfragsize;
      audio_engines[adev]->max_block = devc->hw_pfragsize;
      audio_engines[adev]->min_rate = 8000;
      audio_engines[adev]->max_rate = 96000;

      audio_engines[adev]->mixer_dev = devc->mixer_dev;
      devc->direct_audio_opened = 0;
      portc->open_mode = 0;
      portc->audio_dev = adev;
      portc->channels = 10;
      portc->direction = OPEN_WRITE;
      audio_engines[adev]->port_number = 0;	/* First output channel */
    }

  /*
   * Input device
   */
  sprintf (tmpname, "%s (all inputs)", devc->model_data->product);

  if ((adev = in_dev = oss_install_audiodev_with_devname (OSS_AUDIO_DRIVER_VERSION,
				    devc->osdev,
				    devc->osdev,
				    tmpname,
				    &envy24d_audio_driver,
				    sizeof (audiodrv_t),
				    ADEV_NOVIRTUAL | ADEV_NOOUTPUT | ADEV_COLD
				    | ADEV_SPECIAL | ADEV_32BITONLY,
				    AFMT_S32_LE, devc, -1,
				    "12ch_in")) >= 0)
    {
      portc = &devc->direct_portc_in;
      audio_engines[adev]->portc = portc;
      audio_engines[adev]->rate_source = devc->first_dev;
      audio_engines[adev]->caps |= DSP_CH_MULTI;
      audio_engines[adev]->min_channels = 12;
      audio_engines[adev]->max_channels = 12;
      audio_engines[adev]->inch_names = INCH_NAMES;
      audio_engines[adev]->min_rate = 8000;
      audio_engines[adev]->max_rate = 96000;

      audio_engines[adev]->mixer_dev = devc->mixer_dev;
      devc->direct_audio_opened = 0;
      portc->open_mode = 0;
      portc->audio_dev = adev;
      audio_engines[adev]->min_block = devc->hw_pfragsize;
      audio_engines[adev]->max_block = devc->hw_pfragsize;
      portc->channels = 12;
      portc->direction = OPEN_READ;
      audio_engines[adev]->port_number = 10;	/* First input channel */
    }
}
