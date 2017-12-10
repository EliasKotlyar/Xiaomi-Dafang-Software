/*
 * Purpose: Driver for the VIA8233/8235 AC97 audio controller
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

#include "oss_via823x_cfg.h"
#include <oss_pci.h>
#include <ac97.h>
#include "via8233.h"


static void feed_sgd (via8233_devc * devc, dmap_t * dmap, engine_desc * eng);

static int
ac97_read (void *devc_, int wIndex)
{
  oss_native_word flags;
  unsigned int dwWriteValue = 0, dwTmpValue, i = 0;
  via8233_devc *devc = devc_;


  /* Index has only 7 bit */
  if (wIndex > 0x7F)
    return 0;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  dwWriteValue = ((unsigned int) wIndex << 16) + CODEC_RD;
  OUTL (devc->osdev, dwWriteValue, devc->base + AC97CODEC);
  oss_udelay (100);
  /* Check AC CODEC access time out */
  for (i = 0; i < CODEC_TIMEOUT_COUNT; i++)
    {
      /* if send command over, break */
      if (INL (devc->osdev, devc->base + AC97CODEC) & STA_VALID)
	break;
      oss_udelay (50);
    }
  if (i == CODEC_TIMEOUT_COUNT)
    {
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return OSS_EIO;
    }

  /* Check if Index still ours? If yes, return data, else return FAIL */
  dwTmpValue = INL (devc->osdev, devc->base + AC97CODEC);
  OUTB (devc->osdev, 0x02, devc->base + AC97CODEC + 3);
  if (((dwTmpValue & CODEC_INDEX) >> 16) == wIndex)
    {
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return ((int) dwTmpValue & CODEC_DATA);
    }
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return OSS_EIO;

}

static int
ac97_write (void *devc_, int wIndex, int wData)
{
  oss_native_word flags;
  unsigned int dwWriteValue = 0, i = 0;
  via8233_devc *devc = devc_;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  dwWriteValue = ((unsigned int) wIndex << 16) + wData;
  OUTL (devc->osdev, dwWriteValue, devc->base + AC97CODEC);
  oss_udelay (100);

  /* Check AC CODEC access time out */
  for (i = 0; i < CODEC_TIMEOUT_COUNT; i++)
    {
      /* if send command over, break */
      if (!(INL (devc->osdev, devc->base + AC97CODEC) & IN_CMD))
	break;
      oss_udelay (50);
    }
  if (i == CODEC_TIMEOUT_COUNT)
    {
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return 0;
    }
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return 1;
}

static int
via8233intr (oss_device_t * osdev)
{
  int serviced = 0, i;
  via8233_devc *devc = osdev->devc;
  via8233_portc *portc;
  engine_desc *eng;
  unsigned int engine_stat;
  unsigned int status = 0;
  oss_native_word flags;

  MUTEX_ENTER (devc->mutex, flags);
  status = INL (devc->osdev, devc->base + 0x84);
#if 0
  // This is reported to cause hang because some status register bits
  // may be turned on even ehen the device is not interrupting.
  if (status == 0)
    {
      /*
       * No interrupts are pending so we can stop without
       * polling all the individual status registers.
       */
      MUTEX_EXIT (devc->mutex, flags);
      return 0;
    }
#endif

  for (i = 0; i < MAX_PORTC; i++)
    {
      portc = &devc->portc[i];
      eng = portc->play_engine;

      if (eng != NULL)
	if ((eng->mode & OPEN_WRITE)
	    && (portc->trigger_bits & PCM_ENABLE_OUTPUT))
	  {
	    engine_stat = INB (devc->osdev, eng->base + 0x00);
	    if (engine_stat & 0x01)
	      {
		oss_audio_outputintr (portc->audiodev, 1);
		feed_sgd (devc, audio_engines[portc->audiodev]->dmap_out,
			  eng);
		serviced = 1;
	      }
	    OUTB (devc->osdev, engine_stat, eng->base + 0x00);
	  }

      eng = portc->rec_engine;
      if (eng != NULL)
	if ((eng->mode & OPEN_READ)
	    && (portc->trigger_bits & PCM_ENABLE_INPUT))
	  {
	    engine_stat = INB (devc->osdev, eng->base + 0x00);
	    if (engine_stat & 0x01)
	      {
		oss_audio_inputintr (portc->audiodev, 0);
		feed_sgd (devc, audio_engines[portc->audiodev]->dmap_in, eng);
		serviced = 1;
	      }
	    OUTB (devc->osdev, engine_stat, eng->base + 0x00);
	  }
    }
  MUTEX_EXIT (devc->mutex, flags);

  return serviced;
}

/*
 * Audio routines
 */

static int
via8233_audio_set_rate (int dev, int arg)
{
  via8233_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->speed;

  if (audio_engines[dev]->flags & ADEV_FIXEDRATE)
    arg = 48000;

  if (arg > 48000)
    arg = 48000;
  if (arg < 5000)
    arg = 5000;
  portc->speed = arg;
  return portc->speed;
}

static short
via8233_audio_set_channels (int dev, short arg)
{
  via8233_portc *portc = audio_engines[dev]->portc;

  if (arg > 6)
     arg=6;

  if ((arg != 1) && (arg != 2) && (arg != 4) && (arg != 6))
    return portc->channels;
  portc->channels = arg;

  return portc->channels;
}

static unsigned int
via8233_audio_set_format (int dev, unsigned int arg)
{
  via8233_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->bits;

  if (!(arg & (AFMT_U8 | AFMT_S16_LE | AFMT_AC3)))
    return portc->bits;
  portc->bits = arg;

  return portc->bits;
}

/*ARGSUSED*/
static int
via8233_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void via8233_audio_trigger (int dev, int state);

static void
via8233_audio_reset (int dev)
{
  via8233_audio_trigger (dev, 0);
}

static void
via8233_audio_reset_input (int dev)
{
  via8233_portc *portc = audio_engines[dev]->portc;
  via8233_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
via8233_audio_reset_output (int dev)
{
  via8233_portc *portc = audio_engines[dev]->portc;
  via8233_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

/*ARGSUSED*/
static int
via8233_audio_open (int dev, int mode, int open_flags)
{
  via8233_portc *portc = audio_engines[dev]->portc;
  via8233_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  if (devc->open_mode & mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  devc->open_mode |= mode;

  portc->open_mode = mode;
  portc->audio_enabled &= ~mode;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

static void
via8233_audio_close (int dev, int mode)
{
  via8233_portc *portc = audio_engines[dev]->portc;
  via8233_devc *devc = audio_engines[dev]->devc;

  via8233_audio_reset (dev);
  portc->open_mode = 0;
  devc->open_mode &= ~mode;
  portc->audio_enabled &= ~mode;
}

/*ARGSUSED*/
static void
via8233_audio_output_block (int dev, oss_native_word buf, int count,
			    int fragsize, int intrflag)
{
  via8233_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;


}

/*ARGSUSED*/
static void
via8233_audio_start_input (int dev, oss_native_word buf, int count,
			   int fragsize, int intrflag)
{
  via8233_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;

}

static void
via8233_audio_trigger (int dev, int state)
{
  via8233_portc *portc = audio_engines[dev]->portc;
  via8233_devc *devc = audio_engines[dev]->devc;
  engine_desc *eng;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if ((portc->open_mode & OPEN_WRITE) && portc->play_engine != NULL)
    {
      eng = portc->play_engine;

      if (state & PCM_ENABLE_OUTPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      /* Start with autoinit and SGD flag interrupts enabled */
	      OUTB (devc->osdev, 0xa1, eng->base + 0x01);
	      portc->trigger_bits |= PCM_ENABLE_OUTPUT;
	    }
	}
      else
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      (portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      OUTB (devc->osdev, 0x40, eng->base + 0x01);	/* Stop */
	      portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
	    }
	}
    }

  if ((portc->open_mode & OPEN_READ) && portc->rec_engine != NULL)
    {
      eng = portc->rec_engine;

      if (state & PCM_ENABLE_INPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_INPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_INPUT))
	    {
	      /* Start with autoinit and SGD flag interrupts enabled */
	      OUTB (devc->osdev, 0xa1, eng->base + 0x01);
	      portc->trigger_bits |= PCM_ENABLE_INPUT;
	    }
	}
      else
	{
	  if ((portc->audio_enabled & PCM_ENABLE_INPUT) &&
	      (portc->trigger_bits & PCM_ENABLE_INPUT))
	    {
	      portc->trigger_bits &= ~PCM_ENABLE_INPUT;
	      OUTB (devc->osdev, 0x40, eng->base + 0x01);	/* Stop */
	    }
	}
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static void
feed_sgd (via8233_devc * devc, dmap_t * dmap, engine_desc * eng)
{
  unsigned int addr, p, tmp;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  addr = dmap->dmabuf_phys + eng->cfrag * dmap->fragment_size;
  p = eng->sgd_ptr;

  eng->sgd[p].phaddr = addr;
  eng->sgd[p].flags = dmap->fragment_size | SGD_FLAG;
  if (p == (SGD_SIZE - 1))
    eng->sgd[p].flags |= SGD_EOL;

  /* Update the last entry ptr */
  tmp = INL (devc->osdev, eng->base + 0x08);
  tmp &= 0x00ffffff;
  tmp |= (p << 24);
  OUTL (devc->osdev, tmp, eng->base + 0x08);

  eng->prev_sgd = p;
  eng->frags[p] = eng->cfrag;
  eng->cfrag = (eng->cfrag + 1) % dmap->nfrags;
  eng->sgd_ptr = (eng->sgd_ptr + 1) % SGD_SIZE;
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
}

/*ARGSUSED*/
static int
via8233_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  via8233_devc *devc = audio_engines[dev]->devc;
  via8233_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_in;
  engine_desc *eng;
  int i;
  unsigned int fmt;
  oss_native_word flags;

  if (portc->rec_engine == NULL)
    {
      cmn_err (CE_WARN, "No rec engine (dev=%d)\n", dev);
      return OSS_EIO;
    }

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  eng = portc->rec_engine;

  OUTB (devc->osdev, 0x40, eng->base + 0x01);	/* Stop */

  eng->cfrag = 0;
  eng->sgd_ptr = 0;
  eng->prevpos = 0xffffffff;

  for (i = 0; i < 2; i++)
    feed_sgd (devc, dmap, eng);

  OUTL (devc->osdev, eng->sgd_phys, eng->base + 0x04);

  fmt = 0;
  if (portc->bits == AFMT_S16_LE)
    fmt |= (1 << 21);
  if (portc->channels == 2)
    fmt |= (1 << 20);

  if (devc->chip_type != CHIP_8233A)
    {
      if (portc->speed == 48000)
	fmt |= (1 << 20) - 1;
      else
	fmt |= ((1024 * portc->speed + 24000) / 48000) * 1024;
    }

  fmt |= (eng->prev_sgd << 24);
  OUTL (devc->osdev, fmt, eng->base + 0x08);
  ac97_recrate (&devc->ac97devc, portc->speed);

  portc->audio_enabled &= ~PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/
static int
via8233_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  via8233_devc *devc = audio_engines[dev]->devc;
  via8233_portc *portc = audio_engines[dev]->portc;
  dmap_p dmap = audio_engines[dev]->dmap_out;

  engine_desc *eng;
  int i, tmp;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  eng = portc->play_engine;

  OUTB (devc->osdev, 0x40, eng->base + 0x01);	/* Stop */

  eng->cfrag = 0;
  eng->sgd_ptr = 0;
  eng->prevpos = 0xffffffff;

  for (i = 0; i < 2; i++)
    feed_sgd (devc, dmap, eng);

  OUTL (devc->osdev, eng->sgd_phys, eng->base + 0x4);

  ac97_spdifout_ctl (devc->mixer_dev, SPDIFOUT_AUDIO, SNDCTL_MIX_WRITE, 0);

  if (portc->bits == AFMT_AC3)
    {
      portc->channels = 2;
      portc->bits = 16;
    }
  ac97_spdif_setup (devc->mixer_dev, portc->speed, portc->bits);

  tmp = (portc->bits == AFMT_U8) ? 0 : 0x80;
  tmp |= portc->channels << 4;
  OUTB (devc->osdev, tmp, eng->base + 0x02);

  /* Select channel assignment - not valid for 8233A */
  tmp = 0;
  if (devc->chip_type != CHIP_8233A)
    {
      switch (portc->channels)
	{
	case 1:
	  tmp = (1 << 0) | (1 << 4);
	  break;
	case 2:
	  tmp = (1 << 0) | (2 << 4);
	  break;
	case 4:
	  tmp = (1 << 0) | (2 << 4) | (3 << 8) | (4 << 12);
	  break;
	case 6:
	  tmp =
	    (1 << 0) | (2 << 4) | (5 << 8) | (6 << 12) | (3 << 16) | (4 <<
								      20);
	  break;
	default:
	  tmp = 0;
	  break;
	}
    }
  tmp |= 0xFF000000;
  OUTL (devc->osdev, tmp, eng->base + 0x08);
  /* need to set the speed twice - for some odd reason */
  ac97_playrate (&devc->ac97devc, portc->speed);
  ac97_playrate (&devc->ac97devc, portc->speed);

  portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

/*ARGSUSED*/
static int
via8233_alloc_buffer (int dev, dmap_t * dmap, int direction)
{
  int err;
  via8233_devc *devc = audio_engines[dev]->devc;
  via8233_portc *portc = audio_engines[dev]->portc;
  engine_desc *eng;

  if (dmap->dmabuf != NULL)
    return 0;

  if ((err = oss_alloc_dmabuf (dev, dmap, direction)) < 0)
    return err;


  if (direction == PCM_ENABLE_INPUT)
    {
      eng = &devc->engines[REC_SGD_NUM];
      eng->mode = OPEN_READ;
      portc->rec_engine = eng;
    }
  else
    {
      eng = &devc->engines[PLAY_SGD_NUM];
      eng->mode = OPEN_WRITE;
      portc->play_engine = eng;
    }

  return 0;
}


/*ARGSUSED*/
static int
via8233_free_buffer (int dev, dmap_t * dmap, int direction)
{
  via8233_portc *portc = audio_engines[dev]->portc;

  if (dmap->dmabuf == NULL)
    return 0;
  oss_free_dmabuf (dev, dmap);

  dmap->dmabuf = NULL;
  dmap->dmabuf_phys = 0;

  if (direction == PCM_ENABLE_OUTPUT)
    portc->play_engine = NULL;

  if (direction == PCM_ENABLE_INPUT)
    portc->rec_engine = NULL;

  return 0;
}

static int
via8233_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  via8233_portc *portc = audio_engines[dev]->portc;
  via8233_devc *devc = audio_engines[dev]->devc;
  unsigned int ptr, pos, tmp;
  engine_desc *eng;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  if (direction == PCM_ENABLE_OUTPUT)
    {
      int status;
      /* int this_sgd, prev_sgd */ ;

      if (portc->play_engine == NULL
	  || !(portc->trigger_bits & PCM_ENABLE_OUTPUT))
	{
	  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
	  return 0;
	}

      eng = portc->play_engine;

      ptr = INL (devc->osdev, eng->base + 0x0c);
      status = INB (devc->osdev, eng->base + 0x00);
      if (!(status & 0x80))	/* SGD not triggered */
	{
	  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
	  return 0;
	}

#ifdef DO_TIMINGS
      oss_timing_printf ("rawpos=%d", ptr);
#endif
#if 0
      this_sgd = ptr >> 24;
      prev_sgd = eng->prevpos >> 24;
      prev_sgd = (prev_sgd + 1) % SGD_SIZE;	/* Increment */
      /* Chip bug catcher */
      if (((ptr & 0xffffff) == 0) &&
	  ((eng->prevpos & 0xffffff) == 0) && (this_sgd == prev_sgd))
	ptr = eng->prevpos;
      else
	eng->prevpos = ptr;
#endif

      tmp = ptr & 0xffffff;
      ptr >>= 24;

      pos = eng->frags[ptr] * dmap->fragment_size;
      pos += (dmap->fragment_size - tmp) & ~3;

#ifdef DO_TIMINGS
      oss_timing_printf ("Playpos=%d", pos);
#endif
      pos = pos % dmap->bytes_in_use;
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return pos;
    }

  if (direction == PCM_ENABLE_INPUT)
    {
      int status;
      /* int this_sgd, prev_sgd; */

      if (portc->rec_engine == NULL
	  || !(portc->trigger_bits & PCM_ENABLE_INPUT))
	{
	  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
	  return 0;
	}

      eng = portc->rec_engine;

      ptr = INL (devc->osdev, eng->base + 0x0c);
      status = INB (devc->osdev, eng->base + 0x00);
      if (!(status & 0x80))	/* SGD not triggered */
	{
	  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
	  return 0;
	}

#if 0
      this_sgd = ptr >> 24;
      prev_sgd = eng->prevpos >> 24;
      prev_sgd = (prev_sgd + 1) % SGD_SIZE;	/* Increment */

      /* Chip bug catcher */
      if (((ptr & 0xffffff) == 0) &&
	  ((eng->prevpos & 0xffffff) == 0) && (this_sgd == prev_sgd))
	ptr = eng->prevpos;
      else
	eng->prevpos = ptr;
#endif

      tmp = ptr & 0xffffff;
      ptr >>= 24;

      pos = eng->frags[ptr] * dmap->fragment_size;
      pos += (dmap->fragment_size - tmp) & ~3;

#ifdef DO_TIMINGS
      oss_timing_printf ("Recpos=%d", pos);
#endif
      pos = pos % dmap->bytes_in_use;
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return pos;
    }
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return 0;
}

static const audiodrv_t via8233_audio_driver = {
  via8233_audio_open,
  via8233_audio_close,
  via8233_audio_output_block,
  via8233_audio_start_input,
  via8233_audio_ioctl,
  via8233_audio_prepare_for_input,
  via8233_audio_prepare_for_output,
  via8233_audio_reset,
  NULL,
  NULL,
  via8233_audio_reset_input,
  via8233_audio_reset_output,
  via8233_audio_trigger,
  via8233_audio_set_rate,
  via8233_audio_set_format,
  via8233_audio_set_channels,
  NULL,
  NULL,
  NULL,				/* via8233_check_input, */
  NULL,				/* via8233_check_output, */
  via8233_alloc_buffer,
  via8233_free_buffer,
  NULL,
  NULL,
  via8233_get_buffer_pointer
};

static int
via8233_alloc_engines (via8233_devc * devc)
{
  engine_desc *eng;
  oss_native_word phaddr;
  int i;

  for (i = 0; i < MAX_ENGINES; i++)
    {
      if (i)
	{
	  eng = &devc->engines[REC_SGD_NUM];
	  eng->base = devc->base + 0x60;
	}
      else
	{
	  eng = &devc->engines[PLAY_SGD_NUM];
	  eng->base = devc->base + 0x40;
	}

      if (eng->sgd == NULL)
	{
	  eng->sgd =
	    CONTIG_MALLOC (devc->osdev, SGD_ALLOC, MEMLIMIT_32BITS, &phaddr, eng->sgd_dma_handle);

	  if (eng->sgd == NULL)
	    {
	      cmn_err (CE_WARN, "can't allocate SGD table\n");
	      return OSS_ENOSPC;
	    }
	  eng->sgd_phys = phaddr;
	}
    }
  return 0;
}

static int
via8233_init (via8233_devc * devc)
{
  int my_mixer, adev, opts;
  via8233_portc *portc;
  int i;
  char tmp_name[50];
  int first_dev = 0;

  /* allocate the Scatter Gather Engine buffers */
  if (via8233_alloc_engines (devc) < 0)
    {
      cmn_err (CE_WARN, "Unable to allocate engines\n");
      return OSS_ENOSPC;
    }

  /*
   * Init mixer
   */
  my_mixer =
    ac97_install (&devc->ac97devc, "VIA823x AC97 Mixer", ac97_read,
		  ac97_write, devc, devc->osdev);

  if (my_mixer == -1)
    {
      cmn_err (CE_WARN, "AC97 mixer installation failed\n");
      return 0;			/* No mixer */
    }

  devc->mixer_dev = my_mixer;
  mixer_devs[my_mixer]->priority = 10;	/* Known motherboard device */

  /* enable S/PDIF */
  devc->ac97devc.spdif_slot = SPDIF_SLOT34;
  ac97_spdifout_ctl (devc->mixer_dev, SPDIFOUT_ENABLE, SNDCTL_MIX_WRITE, 1);

  for (i = 0; i < MAX_PORTC; i++)
    {
      opts = ADEV_AUTOMODE;

      if (!ac97_varrate (&devc->ac97devc))
	{
	  opts |= ADEV_FIXEDRATE;
	}

      portc = &devc->portc[i];
      if (i == 0)
	{
	  opts |= ADEV_DUPLEX;
	  strcpy (tmp_name, devc->chip_name);
	}
      else
	{
	  opts |= ADEV_DUPLEX | ADEV_SHADOW;
	  strcpy (tmp_name, devc->chip_name);
	}

      if ((adev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
					devc->osdev,
					devc->osdev,
					tmp_name,
					&via8233_audio_driver,
					sizeof (audiodrv_t),
					opts,
					AFMT_U8 | AFMT_S16_LE | AFMT_AC3,
					devc, -1)) < 0)
	{
	  adev = -1;
	  return 1;
	}
      else
	{
	  if (i == 0)
	    first_dev = adev;
	  audio_engines[adev]->portc = portc;
	  audio_engines[adev]->rate_source = first_dev;
	  audio_engines[adev]->mixer_dev = my_mixer;
	  audio_engines[adev]->min_rate = 8000;
	  audio_engines[adev]->max_rate = 48000;
	  audio_engines[adev]->caps |= PCM_CAP_FREERATE;
	  audio_engines[adev]->min_channels = 2;
	  audio_engines[adev]->max_channels = 6;
	  if (opts & ADEV_FIXEDRATE)
	    {
	      audio_engines[adev]->fixed_rate = 48000;
	      audio_engines[adev]->min_rate = 48000;
	      audio_engines[adev]->max_rate = 48000;
	    }

	  portc->open_mode = 0;
	  portc->audio_enabled = 0;
	  portc->audiodev = adev;
#ifdef CONFIG_OSS_VMIX
	  if (i == 0)
	     vmix_attach_audiodev(devc->osdev, adev, -1, 0);
#endif
	}
    }

  return 1;
}

int
oss_via823x_attach (oss_device_t * osdev)
{
  unsigned char pci_irq_line, pci_revision, bTmp /*, pci_latency */ ;
  unsigned short pci_command, vendor, device;
  unsigned int pci_ioaddr;
  via8233_devc *devc;

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  DDB (cmn_err
       (CE_CONT, "oss_via823x_attach(Vendor %x, device %x)\n", vendor, device));
  if ((vendor != VIA_VENDOR_ID)
      || (device != VIA_8233_ID && device != VIA_8233A_ID))
    {

      cmn_err (CE_WARN, "Hardware not recognized (vendor=%x, dev=%x)\n",
	       vendor, device);
      return 0;
    }

  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_0, &pci_ioaddr);

  if (pci_ioaddr == 0)
    {
      cmn_err (CE_WARN, "I/O address not assigned by BIOS.\n");
      return 0;
    }

  if (pci_irq_line == 0)
    {
      cmn_err (CE_WARN, "IRQ not assigned by BIOS (%d).\n", pci_irq_line);
      return 0;
    }

  if ((devc = PMALLOC (osdev, sizeof (*devc))) == NULL)
    {
      cmn_err (CE_WARN, "Out of memory\n");
      return 0;
    }

  devc->osdev = osdev;
  osdev->devc = devc;
  devc->open_mode = 0;

  devc->chip_type = CHIP_8233;
  devc->chip_name = "VIA VT8233";

  if (pci_revision == 0x50)
    devc->chip_name = "VIA VT8235";

  if (pci_revision == 0x60)
    devc->chip_name = "VIA VT8237";

  if ((device == VIA_8233A_ID) ||
      (device == VIA_8233_ID && pci_revision == 0x40))
    {
      devc->chip_type = CHIP_8233A;
      devc->chip_name = "VIA VT8233A";
    }

  pci_write_config_byte (osdev, 0x41, 0xc0);	/*ENAC97 & deassert RESET */

  oss_udelay (10);
  pci_read_config_byte (osdev, 0x41, &bTmp);
  oss_udelay (10);
  if (devc->chip_type == CHIP_8233A)
    bTmp |= 0x0C;		/* Enable var rate support */
  else
    bTmp |= 0x0f;		/* enable VRA,SB,DX */
  pci_write_config_byte (osdev, 0x41, bTmp);
  oss_udelay (10);

  if (devc->chip_type == CHIP_8233A)
    {
      pci_read_config_byte (osdev, 0x49, &bTmp);
      oss_udelay (10);
      pci_write_config_byte (osdev, 0x49, 0x0);
    }
  else
    {
      /* set slot 3,4 as SPDIF on VIA8235 - AC3 passthrough magic! */
      pci_write_config_byte (osdev, 0x49, 0x1);
    }

  devc->base = MAP_PCI_IOADDR (devc->osdev, 0, pci_ioaddr);
  /* Remove I/O space marker in bit 0. */
  devc->base &= ~0x3;

  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_IO;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  devc->irq = pci_irq_line;

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);

  oss_register_device (osdev, devc->chip_name);

  if (oss_register_interrupts (devc->osdev, 0, via8233intr, NULL) < 0)
    {
      cmn_err (CE_WARN, "Unable to register interrupts\n");
      return 0;
    }

  return via8233_init (devc);	/* Detected */
}

int
oss_via823x_detach (oss_device_t * osdev)
{
  via8233_devc *devc = (via8233_devc *) osdev->devc;
  engine_desc *eng;
  int i;

  if (oss_disable_device (devc->osdev) < 0)
    return 0;


  /* disable S/PDIF */
  if (devc->mixer_dev > 0)
    ac97_spdifout_ctl (devc->mixer_dev, SPDIFOUT_ENABLE, SNDCTL_MIX_WRITE, 0);

  oss_unregister_interrupts (devc->osdev);

  for (i = 0; i < MAX_ENGINES; i++)
    {
      eng = &devc->engines[i];
      if (eng->sgd != NULL)
	{
	  CONTIG_FREE (devc->osdev, eng->sgd, SGD_ALLOC, eng->sgd_dma_handle);
	  eng->sgd = NULL;
	}
    }

  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);
  UNMAP_PCI_IOADDR (devc->osdev, 0);

  oss_unregister_device (devc->osdev);
  return 1;
}
