/*
 * Purpose: Driver for RME Digi96 family
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

#include "oss_digi96_cfg.h"
#include "oss_pci.h"

#define RME_VENDOR_ID2		0x10ee
#define RME_DIGI96		0x3fc0
#define RME_DIGI96_8		0x3fc1
#define RME_DIGI96_PRO		0x3fc2
#define RME_DIGI96_PAD		0x3fc3

#define MAX_AUDIO_CHANNEL	2

/*
 * Control register pCTRL1
 */
#define CTRL1_STARTPLAY		0x00000001
#define CTRL1_STARTREC		0x00000002
#define CTRL1_GAIN		0x0000000c
#define CTRL1_MODE24_PLAY	0x00000010
#define CTRL1_MODE24_REC 	0x00000020
#define CTRL1_PLAYBM		0x00000040
#define CTRL1_RECBM		0x00000080
#define CTRL1_ADAT		0x00000100
#define CTRL1_FREQ		0x00000600
#define	CTRL1_FREQ32		0x00000200
#define	CTRL1_FREQ44		0x00000400
#define	CTRL1_FREQ48		0x00000600
#define CTRL1_DS		0x00000800
#define CTRL1_PRO		0x00001000
#define CTRL1_EMP		0x00002000
#define CTRL1_SEL		0x00004000
#define CTRL1_MASTER		0x00008000
#define CTRL1_PD		0x00010000
#define CTRL1_INPUTSEL		0x00060000
#define CTRL1_INPUTSHIFT		17
#define CTRL1_THRU		0x07f80000
#define CTRL1_AC3		0x08000000
#define CTRL1_MONITOR		0x30000000
#define CTRL1_ISEL		0x40000000
#define CTRL1_IDIS		0x80000000

/*
 * Control register pCTRL2
 */
#define CTRL2_WSEL		0x00000001
#define CTRL2_ANALOG		0x00000002	/* PAD only */
#define CTRL2_FREQAD		0x0000001c	/* PAD */
#define CTRL2_PD2		0x00000020
#
/* Next ones are only for new Digi96/8Pro (blue board) and PAD */
#define CTRL2_DAC_EN		0x00000040
#define CTRL2_CLATCH		0x00000080
#define CTRL2_CCLK		0x00000100
#define CTRL2_CDATA		0x00000200
/*
 * For reads from the pPLAYPOS and pRECPOS registers
 */

#define POS_PLAYIRQ		0x80000000
#define POS_AUTOSYNC		0x40000000
#define POS_FBITS		0x38000000
#define POS_FSHIFT			27
#define POS_ERF			0x04000000
#define POS_CONSTANT11		0x03000000
#define POS_LOCK		0x00800000
#define POS_DEVID		0x00600000
#define POS_CONSTANT000		0x008c0000
#define POS_TOUT		0x00020000
#define POS_RECIRQ		0x00010000
#define POS_ADDR		0x000fffff

typedef struct digi96_portc
{
  int open_mode;
  int audio_dev;
  int channels, bits;
  int speed, speedsel;
  int trigger_bits;
}
digi96_portc;

typedef struct digi96_devc
{
  oss_device_t *osdev;
  oss_mutex_t mutex;

  char *chip_name;
  int have_adat;
  int have_analog;
  int mode;
#define MD_SPDIF	0
#define MD_AES		1
#define MD_ADAT		2

  oss_native_word physaddr;
  char *linaddr;
  unsigned int *pPLAYBUF;
  unsigned int *pRECBUF;
  unsigned int *pCTRL1, ctrl1_bits;
  unsigned int *pCTRL2, ctrl2_bits;
  unsigned int *pPLAYACK;
  unsigned int *pRECACK;
  unsigned int *pPLAYPOS;
  unsigned int *pRECPOS;
  unsigned int *pRESETPLAY;
  unsigned int *pRESETREC;

  int irq;

  digi96_portc portc[MAX_AUDIO_CHANNEL];
  int open_mode;
#define TY_READ	0
#define TY_WRITE 1
#define TY_BOTH 2
  int mixer_dev;

  int master;
  int external_locked;
  int input_source;
  int doublespeed;
  int adatrate;
}
digi96_devc;

static int fbit_tab[8] = {
  0, 0, 0, 96000, 88200, 48000, 44100, 32000
};


static void
write_ctrl1 (digi96_devc * devc, unsigned int ctrl)
{
  devc->ctrl1_bits = ctrl;

  PCI_WRITEL (devc->osdev, devc->pCTRL1, ctrl);
}

static void
write_ctrl2 (digi96_devc * devc, unsigned int ctrl)
{
  devc->ctrl2_bits = ctrl;

  PCI_WRITEL (devc->osdev, devc->pCTRL2, ctrl);
}

static int
digi96intr (oss_device_t * osdev)
{
  int i, serviced = 0;
  unsigned int playstat, recstat;
  digi96_devc *devc = (digi96_devc *) osdev->devc;
  digi96_portc *portc;

  playstat = PCI_READL (devc->osdev, devc->pPLAYPOS);
  recstat = PCI_READL (devc->osdev, devc->pRECPOS);

  for (i = 0; i < 2; i++)
    {
      portc = &devc->portc[i];

      if (playstat & POS_PLAYIRQ)
	{
	  serviced = 1;
	  PCI_WRITEL (devc->osdev, devc->pPLAYACK, 0);
	  oss_audio_outputintr (portc->audio_dev, 0);
	}

      if (recstat & POS_RECIRQ)
	{
	  serviced = 1;
	  PCI_WRITEL (devc->osdev, devc->pRECACK, 0);
	  oss_audio_inputintr (portc->audio_dev, 0);
	}
    }
  return serviced;
}


/***********************************
 * Audio routines 
 ***********************************/

static int
digi96_set_rate (int dev, int arg)
{
  digi96_devc *devc = audio_engines[dev]->devc;
  digi96_portc *portc = audio_engines[dev]->portc;

  static int speed_table[6] = { 32000, 44100, 48000, 64000, 88200, 96000 };
  int i, best = 2, dif, bestdif = 0x7fffffff;

  if (arg)
    {
      if (devc->external_locked || (portc->open_mode & OPEN_READ))
	arg = portc->speed;	/* Speed locked to input */

      for (i = 0; i < 6; i++)
	{
	  if (arg == speed_table[i])	/* Exact match */
	    {
	      portc->speed = arg;
	      portc->speedsel = i;
	      return portc->speed;
	    }

	  dif = arg - speed_table[i];
	  if (dif < 0)
	    dif *= -1;
	  if (dif <= bestdif)
	    {
	      best = i;
	      bestdif = dif;
	    }

	}

      portc->speed = speed_table[best];
      portc->speedsel = best;
    }

  return portc->speed;
}

static short
digi96_set_channels (int dev, short arg)
{
  digi96_portc *portc = audio_engines[dev]->portc;
  digi96_devc *devc = audio_engines[dev]->devc;

  if (arg == 0)
    return portc->channels;

  if (devc->mode == MD_ADAT)
    arg = 8;
  else
    arg = 2;
  return portc->channels = arg;
}

static unsigned int
digi96_set_format (int dev, unsigned int arg)
{
  digi96_portc *portc = audio_engines[dev]->portc;

  if (arg != AFMT_S16_LE && arg != AFMT_AC3 && arg != AFMT_S32_LE)
    return portc->bits;

  return portc->bits = arg;
}

/*ARGSUSED*/
static int
digi96_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void digi96_trigger (int dev, int state);

static void
digi96_reset (int dev)
{
  digi96_trigger (dev, 0);
}

static void
digi96_reset_input (int dev)
{
  digi96_portc *portc = audio_engines[dev]->portc;
  digi96_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
digi96_reset_output (int dev)
{
  digi96_portc *portc = audio_engines[dev]->portc;
  digi96_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

static int
verify_input (digi96_devc * devc, digi96_portc * portc)
{
  int i, status, savedstatus;
  int tmp;
  oss_native_word flags;

  tmp = devc->ctrl1_bits & ~CTRL1_INPUTSEL;
  tmp |= (devc->input_source & 0x3) << CTRL1_INPUTSHIFT;
  write_ctrl1 (devc, tmp);

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  i = 0;
  status = PCI_READL (devc->osdev, devc->pRECPOS);

  if (status & POS_LOCK)	/* ADAT input */
    {
      devc->mode = MD_ADAT;
      portc->channels = 8;

      switch (devc->adatrate)
	{
	case 0:		/* Autodetect */
	  if (status & POS_AUTOSYNC)
	    portc->speed = 48000;
	  else
	    portc->speed = 44100;
	  DDB (cmn_err
	       (CE_WARN, "ADAT input detected, sr=%d Hz\n", portc->speed));
	  break;

	case 1:
	  portc->speed = 44100;
	  break;

	case 2:
	  portc->speed = 48000;
	  break;
	}
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return 1;
    }

  while (i++ < 100 && status & POS_ERF)
    {
      oss_udelay (10);
      status = PCI_READL (devc->osdev, devc->pRECPOS);
    }

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  savedstatus = status;

  status &= POS_ERF;

  if (status)
    cmn_err (CE_WARN, "Cannot sync with the input signal\n");
  else
    {
      int fbits = (savedstatus & POS_FBITS) >> POS_FSHIFT;
      portc->speed = fbit_tab[fbits];
      DDB (cmn_err
	   (CE_WARN, "digi96: Measured input sampling rate is %d\n",
	    portc->speed));
    }
  return devc->external_locked = !status;
}

/*ARGSUSED*/
static int
digi96_open (int dev, int mode, int open_flags)
{
  unsigned int tmp;
  digi96_portc *portc = audio_engines[dev]->portc;
  digi96_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode != 0 || (devc->open_mode & mode))	/* Busy? */
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  portc->open_mode = mode;
  devc->open_mode |= mode;
  portc->audio_dev = dev;
  portc->trigger_bits = 0;

  tmp = devc->ctrl1_bits;
  devc->external_locked = 0;
  if (devc->master)
    tmp |= CTRL1_MASTER;
  else
    {
      tmp &= ~CTRL1_MASTER;
      devc->external_locked = 1;
    }
  write_ctrl1 (devc, tmp);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  if (devc->external_locked || (portc->open_mode & OPEN_READ))
    verify_input (devc, portc);

  if (devc->mode == MD_ADAT)
    {
      audio_engines[dev]->min_block = 8 * 1024;
      audio_engines[dev]->max_block = 8 * 1024;
      audio_engines[dev]->caps &= ~DSP_CH_MASK;
      audio_engines[dev]->caps |= DSP_CH_MULTI;
      audio_engines[dev]->min_channels = 8;
      audio_engines[dev]->max_channels = 8;
      write_ctrl1 (devc, devc->ctrl1_bits & ~CTRL1_ISEL);
    }
  else
    {
      audio_engines[dev]->min_block = 2 * 1024;
      audio_engines[dev]->max_block = 2 * 1024;
      audio_engines[dev]->caps &= ~DSP_CH_MASK;
      audio_engines[dev]->caps |= DSP_CH_STEREO;
      audio_engines[dev]->min_channels = 2;
      audio_engines[dev]->max_channels = 2;
      write_ctrl1 (devc, devc->ctrl1_bits | CTRL1_ISEL);
    }

  return 0;
}

static void
digi96_close (int dev, int mode)
{
  digi96_devc *devc = audio_engines[dev]->devc;
  digi96_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;

  digi96_reset (dev);
  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  portc->open_mode = 0;
  devc->open_mode &= ~mode;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
static void
digi96_output_block (int dev, oss_native_word ptr, int count, int fragsize,
		     int intrflag)
{
}

/*ARGSUSED*/
static void
digi96_start_input (int dev, oss_native_word ptr, int count, int fragsize,
		    int intrflag)
{
}

static void
digi96_trigger (int dev, int state)
{
  digi96_devc *devc = audio_engines[dev]->devc;
  digi96_portc *portc = audio_engines[dev]->portc;
  unsigned int tmp = devc->ctrl1_bits;

  state &= portc->open_mode;

  if (portc->open_mode & OPEN_WRITE)
    {
      if (state & PCM_ENABLE_OUTPUT)
	tmp |= CTRL1_STARTPLAY;
      else
	tmp &= ~CTRL1_STARTPLAY;
    }

  if (portc->open_mode & OPEN_READ)
    {
      if (state & PCM_ENABLE_INPUT)
	tmp |= CTRL1_STARTREC;
      else
	tmp &= ~CTRL1_STARTREC;
    }

  portc->trigger_bits = state;
  write_ctrl1 (devc, tmp);
}

/*ARGSUSED*/
static int
digi96_prepare_for_output (int dev, int bsize, int bcount)
{
  digi96_devc *devc = audio_engines[dev]->devc;
  digi96_portc *portc = audio_engines[dev]->portc;

  int cmd = devc->ctrl1_bits;
  int doublespeed;

  PCI_WRITEL (devc->osdev, devc->pRESETPLAY, 0);
  if (devc->master)
    cmd |= CTRL1_MASTER;
  else
    cmd &= ~CTRL1_MASTER;
  if (portc->channels == 8)
    cmd |= CTRL1_ADAT;
  else
    cmd &= ~CTRL1_ADAT;
  write_ctrl1 (devc, cmd);

  if (!devc->master)
    {
      if (!verify_input (devc, portc))
	return OSS_EIO;
    }

  cmd = devc->ctrl1_bits;
  doublespeed = (portc->speedsel > 3);

  cmd &= ~(CTRL1_DS | CTRL1_FREQ);
  if (doublespeed)
    cmd |= CTRL1_DS;

  cmd |= ((portc->speedsel % 3) + 1) << 9;
  write_ctrl1 (devc, cmd);

  if (doublespeed != devc->doublespeed)
    {
      devc->doublespeed = doublespeed;
      write_ctrl1 (devc, cmd | CTRL1_PD);	/* Reset the DAC */
    }

  if (portc->bits == AFMT_AC3)
    write_ctrl1 (devc, devc->ctrl1_bits | CTRL1_AC3 | CTRL1_PD);
  else
    {
      if (portc->bits == AFMT_S32_LE)
	{
	  write_ctrl1 (devc, devc->ctrl1_bits | CTRL1_MODE24_PLAY);
	}
      else
	write_ctrl1 (devc, devc->ctrl1_bits & ~(CTRL1_MODE24_PLAY));

      write_ctrl1 (devc, devc->ctrl1_bits & ~(CTRL1_AC3 | CTRL1_PD));	/* Unmute DAC */
    }
  return 0;
}

/*ARGSUSED*/
static int
digi96_prepare_for_input (int dev, int bsize, int bcount)
{
  digi96_devc *devc = audio_engines[dev]->devc;
  digi96_portc *portc = audio_engines[dev]->portc;

  int cmd = devc->ctrl1_bits;

  if (portc->channels == 8)
    cmd |= CTRL1_ADAT;
  else
    cmd &= ~CTRL1_ADAT;

  if (portc->bits == AFMT_S32_LE)
    cmd |= CTRL1_MODE24_REC;
  else
    cmd &= ~CTRL1_MODE24_REC;
  write_ctrl1 (devc, cmd);

  if (!verify_input (devc, portc))
    return OSS_EIO;
  PCI_WRITEL (devc->osdev, devc->pRESETREC, 0);

  return 0;
}

static int
digi96_alloc_buffer (int dev, dmap_t * dmap, int direction)
{
  digi96_devc *devc = audio_engines[dev]->devc;

  if (direction == OPEN_WRITE)
    {
      dmap->dmabuf = (void *) devc->pPLAYBUF;
      dmap->dmabuf_phys = (oss_native_word) devc->pPLAYBUF;
    }
  else
    {
      dmap->dmabuf = (void *) devc->pRECBUF;
      dmap->dmabuf_phys = (oss_native_word) devc->pRECBUF;
    }
  dmap->buffsize = 64 * 1024;

  return 0;
}

/*ARGSUSED*/
static int
digi96_free_buffer (int dev, dmap_t * dmap, int direction)
{
  dmap->dmabuf = NULL;
  dmap->dmabuf_phys = 0;
  return 0;
}

/*ARGSUSED*/
static void
digi96_setup_fragments (int dev, dmap_p dmap, int direction)
{
  /* Make sure the whole sample RAM is covered by the buffer */
  dmap->nfrags = dmap->buffsize / dmap->fragment_size;
}

static audiodrv_t digi96_driver = {
  digi96_open,
  digi96_close,
  digi96_output_block,
  digi96_start_input,
  digi96_ioctl,
  digi96_prepare_for_input,
  digi96_prepare_for_output,
  digi96_reset,
  NULL,
  NULL,
  digi96_reset_input,
  digi96_reset_output,
  digi96_trigger,
  digi96_set_rate,
  digi96_set_format,
  digi96_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  digi96_alloc_buffer,
  digi96_free_buffer,
  NULL,
  NULL,
  NULL,				/* digi96_get_buffer_pointer */
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  digi96_setup_fragments
};

/*ARGSUSED*/
static int
digi96_mixer_ioctl (int dev, int audiodev, unsigned int cmd, ioctl_arg arg)
{
  if (cmd == SOUND_MIXER_PRIVATE1)	/* Bogus testing */
    {
      int val;

      val = *arg;
      return *arg = val;
    }
  if (cmd == SOUND_MIXER_READ_DEVMASK ||
      cmd == SOUND_MIXER_READ_RECMASK || cmd == SOUND_MIXER_READ_RECSRC)
    return *arg = 0;

  if (cmd == SOUND_MIXER_READ_VOLUME || cmd == SOUND_MIXER_READ_PCM)
    return *arg = 100 | (100 << 8);
  if (cmd == SOUND_MIXER_WRITE_VOLUME || cmd == SOUND_MIXER_WRITE_PCM)
    return *arg = 100 | (100 << 8);
  return OSS_EINVAL;
}

static mixer_driver_t digi96_mixer_driver = {
  digi96_mixer_ioctl
};

static int
digi96_set_control (int dev, int ctrl, unsigned int cmd, int value)
{
  digi96_devc *devc = mixer_devs[dev]->devc;
  unsigned int tmp;

  if (ctrl < 0)
    return OSS_EINVAL;

  if (cmd == SNDCTL_MIX_READ)
    {
      switch (ctrl)
	{
	case 1:
	  return !!devc->master;
	  break;

	case 2:
	  return devc->input_source;
	  break;

	case 3:
	  return !!(devc->ctrl1_bits & CTRL1_SEL);
	  break;

	case 4:
	  return devc->mode;
	  break;

	case 5:
	  return !!(devc->ctrl2_bits & CTRL2_WSEL);
	  break;

	case 6:
	  return !!(devc->ctrl1_bits & CTRL1_EMP);
	  break;

	case 7:
	  return !!(devc->ctrl1_bits & CTRL1_AC3);
	  break;

	case 8:
	  return devc->adatrate;
	  break;
	}

      return OSS_EINVAL;
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
      switch (ctrl)
	{
	case 1:
	  devc->master = !!value;
	  return !!value;
	  break;

	case 2:
	  if (value < 0 || value > 3)
	    return OSS_EINVAL;

	  devc->input_source = value;
	  return value;
	  break;

	case 3:
	  tmp = devc->ctrl1_bits;
	  if (value)
	    tmp |= CTRL1_SEL;
	  else
	    {
	      tmp &= ~(CTRL1_SEL | CTRL1_MASTER);
	      devc->master = 0;
	    }
	  write_ctrl1 (devc, tmp);
	  return value;
	  break;

	case 4:
	  if (value > 2 || (!devc->have_adat && value > 1))
	    return OSS_EINVAL;

	  devc->mode = value;
	  return value;
	  break;

	case 5:
	  if (value)
	    {
	      write_ctrl2 (devc, devc->ctrl2_bits | CTRL2_WSEL);
	      return 1;
	    }

	  write_ctrl2 (devc, devc->ctrl2_bits & ~CTRL2_WSEL);
	  return 0;
	  break;

	case 6:
	  if (value)
	    {
	      write_ctrl1 (devc, devc->ctrl1_bits | CTRL1_EMP);
	      return 1;
	    }

	  write_ctrl1 (devc, devc->ctrl1_bits & ~CTRL1_EMP);
	  return 0;
	  break;

	case 7:
	  if (value)
	    {
	      write_ctrl1 (devc, devc->ctrl1_bits | CTRL1_AC3);
	      return 1;
	    }

	  write_ctrl1 (devc, devc->ctrl1_bits & ~CTRL1_AC3);
	  return 0;
	  break;

	case 8:
	  if (value > 2)
	    value = 2;
	  if (value < 0)
	    value = 0;

	  return devc->adatrate = value;
	  break;

	}

      return OSS_EINVAL;
    }

  return OSS_EINVAL;
}

static int
digi96_mix_init (int dev)
{
  /* digi96_devc *devc = mixer_devs[dev]->devc; */
  int group, err;

  if ((group = mixer_ext_create_group (dev, 0, "DIGI96")) < 0)
    return group;

  if ((err = mixer_ext_create_control (dev, group,
				       4, digi96_set_control,
				       MIXT_ENUM,
				       "DIGI96_MODE", 3,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       1, digi96_set_control,
				       MIXT_ENUM,
				       "DIGI96_SYNC", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       2, digi96_set_control,
				       MIXT_ENUM,
				       "DIGI96_INPUT", 4,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       3, digi96_set_control,
				       MIXT_ENUM,
				       "DIGI96_SEL", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       5, digi96_set_control,
				       MIXT_ONOFF,
				       "DIGI96_WORLDCLK", 1,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       6, digi96_set_control,
				       MIXT_ONOFF,
				       "DIGI96_EMPH", 1,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

#if 0
  if ((err = mixer_ext_create_control (dev, group,
				       7, digi96_set_control,
				       MIXT_ENUM,
				       "DIGI96_DATA", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;
#endif

  if ((err = mixer_ext_create_control (dev, group,
				       8, digi96_set_control,
				       MIXT_ENUM,
				       "DIGI96_ADATRATE", 3,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;
  mixer_ext_set_strings (dev, err, "AUTO 44100 48000", 0);

  return 0;
}

static int
attach_channel (digi96_devc * devc, int chnum, char *name,
		audiodrv_t * drv, int type)
{
  int adev;
  digi96_portc *portc;
  int opts = ADEV_DUPLEX | ADEV_COLD | ADEV_AUTOMODE | ADEV_NOVIRTUAL;

  if (chnum < 0 || chnum >= MAX_AUDIO_CHANNEL)
    return 0;

  if (type == TY_BOTH)
    opts = ADEV_SHADOW;

  portc = &devc->portc[chnum];

  devc->master = 1;
  devc->input_source = 0;
  devc->doublespeed = 0;
  devc->external_locked = 0;

  if ((adev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
				    devc->osdev,
				    devc->osdev,
				    name,
				    drv,
				    sizeof (audiodrv_t),
				    opts,
				    AFMT_AC3 | AFMT_S16_LE | AFMT_S32_LE,
				    devc, -1)) < 0)
    {
      return 0;
    }

  audio_engines[adev]->mixer_dev = devc->mixer_dev;
  audio_engines[adev]->portc = portc;
  audio_engines[adev]->caps |= DSP_CH_STEREO;
  audio_engines[adev]->min_block = 2 * 1024;
  audio_engines[adev]->max_block = 2 * 1024;
  audio_engines[adev]->min_rate = 32000;
  audio_engines[adev]->max_rate = 96000;
  portc->open_mode = 0;
  devc->open_mode = 0;
  portc->audio_dev = adev;

  portc->speed = 48000;
  portc->speedsel = 3;
  portc->bits = AFMT_S16_LE;
  portc->channels = 2;
  return 1;
}

int
init_digi96 (digi96_devc * devc)
{
  int my_mixer;
  int ret;

  if ((my_mixer = oss_install_mixer (OSS_MIXER_DRIVER_VERSION,
				     devc->osdev,
				     devc->osdev,
				     "RME Digi96 Control panel",
				     &digi96_mixer_driver,
				     sizeof (mixer_driver_t), devc)) < 0)
    {

      devc->mixer_dev = -1;
      return 0;
    }
  else

    {
      mixer_devs[my_mixer]->priority = -1;	/* Don't use as the default mixer */
      devc->mixer_dev = my_mixer;
      mixer_ext_set_init_fn (my_mixer, digi96_mix_init, 20);
    }

  ret = attach_channel (devc, 0, devc->chip_name, &digi96_driver, 0);
  if (ret > 0)
    {
      ret =
	attach_channel (devc, 1, devc->chip_name, &digi96_driver, TY_BOTH);
    }

/*
 * Set some defaults
 */
  write_ctrl2 (devc, 0);
  write_ctrl1 (devc, CTRL1_ISEL | CTRL1_FREQ48 | CTRL1_PD);	/* Reset DAC */
  write_ctrl1 (devc, CTRL1_ISEL | CTRL1_FREQ48 | CTRL1_MASTER | CTRL1_SEL);
  write_ctrl2 (devc, CTRL2_DAC_EN);	/* Soft unmute the DAC (blue PRO boards) */
  return ret;
}


int
oss_digi96_attach (oss_device_t * osdev)
{
  digi96_devc *devc;
  unsigned char pci_irq_line, pci_revision /*, pci_latency */ ;
  unsigned short pci_command, vendor, device;
  unsigned int pci_ioaddr;
  int err;

  DDB (cmn_err (CE_WARN, "Entered Digi96 detect routine\n"));

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);
  if (vendor != RME_VENDOR_ID2 ||
      (device != RME_DIGI96_PRO &&
       device != RME_DIGI96_PAD &&
       device != RME_DIGI96 && device != RME_DIGI96_8))

    return 0;

  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, PCI_MEM_BASE_ADDRESS_0, &pci_ioaddr);

  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  if (pci_ioaddr == 0)
    {
      cmn_err (CE_WARN, "BAR0 not initialized by BIOS\n");
      return 0;
    }

  if ((devc = PMALLOC (osdev, sizeof (*devc))) == NULL)
    {
      cmn_err (CE_WARN, "Out of memory\n");
      return 0;
    }

  devc->osdev = osdev;
  osdev->devc = devc;
  devc->physaddr = pci_ioaddr;

  devc->have_adat = 0;
  devc->have_analog = 0;
  devc->mode = MD_SPDIF;

  switch (device)
    {
    case RME_DIGI96:
      devc->chip_name = "RME Digi96";
      break;

    case RME_DIGI96_8:
      devc->chip_name = "RME Digi96/8";
      devc->have_adat = 1;
      break;

    case RME_DIGI96_PRO:
      devc->have_adat = 1;
      if (pci_revision < 2)
	{
	  devc->chip_name = "RME Digi96/8 PRO (green)";
	}
      else
	{
	  devc->chip_name = "RME Digi96/8 PRO (blue)";
	}
      break;

    case RME_DIGI96_PAD:
      devc->have_adat = 1;
      devc->have_analog = 1;
      devc->chip_name = "RME Digi96/8 PAD";
      break;
    }

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);

  oss_register_device (osdev, devc->chip_name);

  devc->linaddr =
    (char *) MAP_PCI_MEM (devc->osdev, 0, devc->physaddr, 0x60000);
  if (devc->linaddr == NULL)
    {
      cmn_err (CE_WARN, "Can't map PCI registers (0x%08lx)\n",
	       devc->physaddr);
      return 0;
    }

  if ((err = oss_register_interrupts (devc->osdev, 0, digi96intr, NULL)) < 0)
    {
      cmn_err (CE_WARN, "Can't register interrupt handler, err=%d\n", err);
      return 0;
    }

  devc->pPLAYBUF = (unsigned int *) devc->linaddr;
  devc->pRECBUF = (unsigned int *) (devc->linaddr + 0x10000);
  devc->pCTRL1 = (unsigned int *) (devc->linaddr + 0x20000);
  devc->pCTRL2 = (unsigned int *) (devc->linaddr + 0x20004);
  devc->pPLAYACK = (unsigned int *) (devc->linaddr + 0x20008);
  devc->pRECACK = (unsigned int *) (devc->linaddr + 0x2000c);
  devc->pPLAYPOS = (unsigned int *) (devc->linaddr + 0x20000);
  devc->pRECPOS = (unsigned int *) (devc->linaddr + 0x30000);
  devc->pRESETPLAY = (unsigned int *) (devc->linaddr + 0x4fffc);
  devc->pRESETREC = (unsigned int *) (devc->linaddr + 0x5fffc);

  return init_digi96 (devc);	/* Detected */
}

int
oss_digi96_detach (oss_device_t * osdev)
{
  digi96_devc *devc = (digi96_devc *) osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  write_ctrl2 (devc, 0);	/* Soft mute */

  oss_unregister_interrupts (devc->osdev);

  MUTEX_CLEANUP (devc->mutex);

  UNMAP_PCI_MEM (devc->osdev, 0, devc->physaddr, devc->linaddr, 0x60000);

  oss_unregister_device (devc->osdev);

  return 1;

}
