/*
 * Purpose: Driver for RME Digi32 family
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

#include "oss_digi32_cfg.h"
#include "oss_pci.h"

#define RME_VENDOR_ID		0xea60
#define RME_DIGI32		0x9896
#define RME_DIGI32_PRO		0x9897
#define RME_DIGI32_8		0x9898

#define RME_VENDOR_ID2		0x10ee
#define RME_DIGI96_8_PRO	0x3fc2

#define MAX_AUDIO_CHANNEL	4

/*
 * Control register bits
 */
#define C_EMP		0x8000
#define C_PD		0x4000
#define C_AUTOSYNC	0x2000
#define C_ADAT		0x1000
#define C_DS		0x0800
#define C_BM		0x0800
#define C_PRO		0x0400
#define C_MUTE		0x0200
#define C_RESET		0x0100
#define C_INP_1		0x0080
#define C_INP_0		0x0040
#define C_FREQ_1	0x0020
#define C_FREQ_0	0x0010
#define C_SEL		0x0008
#define C_MODE24	0x0004
#define C_MONO		0x0002
#define C_START		0x0001

/*
 * Status register bits 
 */
#define S_INTR		0x80000000
#define S_KMODE		0x40000000	/* 0=Quartz mode, 1=PLL mode */
#define S_FBITS		0x38000000	/* F bits of CS8412/14 */
#define S_FSHIFT	27
#define S_ERF		0x04000000	/* ERF of the CS8412. 1=error */
#define S_REV		0x0x300000	/* Revision bits */
#define S_ADATLOCK	0x00080000	/* ADAT PLL locked */

typedef struct digi32_portc
{
  int open_mode;
  int audio_dev;
  int voice_chn;
  int type;
#define TY_IN	1
#define TY_OUT	2
#define TY_BOTH 3

  int active;
  int channels;
  int speed;
  int speedsel;
  int bits;
  int inptr, outptr;
  int tmp_autosync;
  int bytes_per_sample;
  int trigger_bits;
}
digi32_portc;

typedef struct digi32_devc
{
  oss_device_t *osdev;
  oss_mutex_t mutex;
  char *chip_name;
  int model;
#define MDL_BASIC		0
#define MDL_8			1
#define MDL_PRO			2
#define MDL_96_8_PRO		3

  int cs841x_part;
#define CS8412			0
#define CS8414			1

  unsigned int cmd;

  oss_native_word physaddr;
  char *linaddr;
  unsigned int *pFIFO;
  unsigned int *pCTRL;

  int irq;

  digi32_portc portc[MAX_AUDIO_CHANNEL];
  int open_count;
  int mixer_dev;

  int speed_locked;
  int active_device_count;
}
digi32_devc;

static int fbit_tab[2][8] = {
  {0, 48000, 44100, 32000, 48000, 44100, 44056, 32000},
  {0, 0, 0, 96000, 88200, 48000, 44100, 32000}
};


static void
write_command (digi32_devc * devc, unsigned int ctrl)
{
  devc->cmd = ctrl;

  PCI_WRITEL (devc->osdev, devc->pCTRL, ctrl);
}

/*ARGSUSED*/
static void
handle_recording (int dev)
{
  digi32_portc *portc = audio_engines[dev]->portc;
  digi32_devc *devc = audio_engines[dev]->devc;

  int i, p;
  unsigned int *buf;

  p = portc->inptr / 4;		/* Dword addressing */
  buf =
    (unsigned int *) &audio_engines[dev]->dmap_in->
    dmabuf[dmap_get_qtail (audio_engines[dev]->dmap_in) *
	   audio_engines[dev]->dmap_in->fragment_size];

  /* Perform copy with channel swapping */
  if (portc->bytes_per_sample == 4)
    {
      /* 32 bit mode */
      for (i = 0; i < (8 * 1024) / 4; i += 2)
	{
	  buf[i] = PCI_READL (devc->osdev, devc->pFIFO + i + 1 + p);
	  buf[i + 1] = PCI_READL (devc->osdev, devc->pFIFO + i + p);
	}
    }
  else
    {
      /* 16 bit mode */
      unsigned int tmp;

      for (i = 0; i < (8 * 1024) / 4; i++)
	{
	  tmp = PCI_READL (devc->osdev, devc->pFIFO + i + p);
	  buf[i] = ((tmp >> 16) & 0xffff) | ((tmp & 0xffff) << 16);
	}
    }

  if (!(portc->trigger_bits & PCM_ENABLE_OUTPUT))	/* Clean the samples */
    for (i = 0; i < (8 * 1024) / 4; i++)
      {
	PCI_WRITEL (devc->osdev, devc->pFIFO + i + p, 0);
      }

  portc->inptr = (portc->inptr + (8 * 1024)) % (128 * 1024);	/* Increment the pointer */
}

static int
digi32intr (oss_device_t * osdev)
{
  unsigned int status;
  digi32_devc *devc = (digi32_devc *) osdev->devc;
  digi32_portc *portc;
  int i;

  status = PCI_READL (devc->osdev, devc->pCTRL);
  if (!(status & S_INTR))
    return 0;

  for (i = 0; i < 2; i++)
    {
      portc = &devc->portc[i];

      if (portc->trigger_bits & PCM_ENABLE_INPUT)
	{
	  handle_recording (portc->audio_dev);
	  if (status & S_ERF && !(devc->cmd & C_AUTOSYNC))
	    {
	      cmn_err (CE_WARN, "External sync dropped\n");
#if 0
	      devc->cmd |= C_AUTOSYNC;
	      write_command (devc, devc->cmd);
#endif
	    }
	  else
	    oss_audio_inputintr (portc->audio_dev, 0);
	}

      if (portc->trigger_bits & PCM_ENABLE_OUTPUT)
	{
	  oss_audio_outputintr (portc->audio_dev, 1);
	}
    }
  write_command (devc, devc->cmd);	/* Interrupt acknowledge */

  return 1;
}


/***********************************
 * Audio routines 
 ***********************************/

static int
digi32_set_rate (int dev, int arg)
{
  digi32_devc *devc = audio_engines[dev]->devc;
  digi32_portc *portc = audio_engines[dev]->portc;

  static int speed_table[6] = { 32000, 44100, 48000, 64000, 88200, 96000 };
  int n, i, best = 2, dif, bestdif = 0x7fffffff;

  if (arg == 0 || devc->active_device_count > 0 || devc->speed_locked)
    arg = portc->speed;

  n = (devc->model == MDL_PRO) ? 6 : 3;

  for (i = 0; i < n; i++)
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
  return portc->speed;
}

static short
digi32_set_channels (int dev, short arg)
{
  digi32_portc *portc = audio_engines[dev]->portc;

  if ((arg == 0))
    return portc->channels;
  arg = 2;
  portc->channels = arg;

  return portc->channels;
}

static unsigned int
digi32_set_format (int dev, unsigned int arg)
{
  digi32_devc *devc = audio_engines[dev]->devc;
  digi32_portc *portc = audio_engines[dev]->portc;

  if (arg == 0 || devc->active_device_count > 0)
    return portc->bits;

  if (arg != AFMT_S16_NE && arg != AFMT_S32_NE && arg != AFMT_AC3)
    arg = AFMT_S16_NE;

  portc->bytes_per_sample = (arg & (AFMT_S32_LE | AFMT_S32_BE)) ? 4 : 2;
  portc->bits = arg;

  return portc->bits;
}

/*ARGSUSED*/
static int
digi32_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void digi32_trigger (int dev, int state);

static void
digi32_reset (int dev)
{
  digi32_trigger (dev, 0);
}

static void
digi32_reset_input (int dev)
{
  digi32_portc *portc = audio_engines[dev]->portc;
  digi32_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
digi32_reset_output (int dev)
{
  digi32_portc *portc = audio_engines[dev]->portc;
  digi32_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

static void digi32_close (int dev, int mode);

/*ARGSUSED*/
static int
digi32_open (int dev, int mode, int open_flags)
{
  digi32_portc *portc = audio_engines[dev]->portc;
  digi32_devc *devc = audio_engines[dev]->devc;
  int i;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode != 0)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  portc->open_mode = mode;
  portc->audio_dev = dev;
  devc->speed_locked = 0;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  for (i = 0; i < 32 * 1024; i++)
    PCI_WRITEL (devc->osdev, devc->pFIFO + i, 0);	/* Clean the buffer */
  return 0;
}

/*ARGSUSED*/
static void
digi32_close (int dev, int mode)
{
  digi32_portc *portc = audio_engines[dev]->portc;
  digi32_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  digi32_reset (dev);
  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  portc->open_mode = 0;
  portc->active = 0;
  if (portc->tmp_autosync)
    {
      portc->tmp_autosync = 0;
      devc->cmd |= C_AUTOSYNC;
      write_command (devc, devc->cmd);
    }

/*
 * Inactivate all auxiliary channels allocated for this device
 */

  if (--devc->open_count <= 0)
    {
      devc->open_count = 0;
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
static void
digi32_output_block (int dev, oss_native_word ptr, int count, int fragsize,
		     int intrflag)
{
  digi32_portc *portc = audio_engines[dev]->portc;
  digi32_devc *devc = audio_engines[dev]->devc;

  int i, p;
  unsigned int *buf;

  p = portc->outptr / 4;	/* Dword addressing */
  buf = (unsigned int *) &audio_engines[dev]->dmap_out->dmabuf[ptr];


  /* Perform copy with channel swapping */
  if (portc->bytes_per_sample == 4)
    {
      /* 32 bit mode */
      for (i = 0; i < (8 * 1024) / 4; i += 2)
	{
	  PCI_WRITEL (devc->osdev, devc->pFIFO + i + p, buf[i + 1]);
	  PCI_WRITEL (devc->osdev, devc->pFIFO + i + 1 + p, buf[i]);
	}
    }
  else
    {
      /* 16 bit mode */
      for (i = 0; i < (8 * 1024) / 4; i++)
	{
	  unsigned int tmp = buf[i];
	  tmp = ((tmp >> 16) & 0xffff) | ((tmp & 0xffff) << 16);
	  PCI_WRITEL (devc->osdev, devc->pFIFO + i + p, tmp);
	}
    }

  PCI_READL (devc->osdev, devc->pFIFO);	/* Dummy read to flust cache on some archs */

  portc->outptr = (portc->outptr + (8 * 1024)) % (128 * 1024);	/* Increment the pointer */

#if 1
  if (!intrflag)
    oss_audio_outputintr (dev, 0);
#endif
}

/*ARGSUSED*/
static void
digi32_start_input (int dev, oss_native_word ptr, int count, int fragsize,
		    int intrflag)
{
}

static void
digi32_trigger (int dev, int state)
{
  digi32_portc *portc = audio_engines[dev]->portc;
  digi32_devc *devc = audio_engines[dev]->devc;
  int cmd = devc->cmd;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  state &= portc->open_mode;

  if (portc->active != state)
    {
      if (state)
	{
	  if (devc->active_device_count == 0)
	    {
	      cmd |= C_START;
	      cmd &= ~C_RESET;
	      write_command (devc, cmd | C_SEL);
	      if ((state & OPEN_WRITE) && (portc->open_mode & OPEN_WRITE))
		portc->trigger_bits |= PCM_ENABLE_OUTPUT;
	      if ((state & OPEN_READ) && (portc->open_mode & OPEN_READ))
		portc->trigger_bits |= PCM_ENABLE_INPUT;
	      devc->active_device_count = 1;
	    }
	}
      else
	{
	  if (devc->active_device_count != 0)
	    {
	      cmd &= ~C_START;
	      write_command (devc, cmd);
	      if (!(state & OPEN_WRITE) && (portc->open_mode & OPEN_WRITE))
		portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
	      if (!(state & OPEN_READ) && (portc->open_mode & OPEN_READ))
		portc->trigger_bits &= ~PCM_ENABLE_INPUT;
	      devc->active_device_count = 0;
	    }
	}
    }

  portc->active = state;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
static int
digi32_prepare_for_output (int dev, int bsize, int bcount)
{
/* NOTE! This routine is used for input too */

  digi32_devc *devc = audio_engines[dev]->devc;
  digi32_portc *portc = audio_engines[dev]->portc;

  int cmd = devc->cmd;
  int dblbit = 0, prev_dblbit, speedsel;

  if (devc->active_device_count > 0)
    return 0;

  if (bsize != 8 * 1024)
    {
      cmn_err (CE_CONT, "Illegal fragment size %d\n", bsize);
      return OSS_EIO;
    }

  prev_dblbit = cmd & C_DS;
  cmd &= C_EMP | C_PRO | C_SEL | C_MUTE | C_INP_0 | C_INP_1 | C_AUTOSYNC;
  cmd |= C_RESET;

  if (portc->bits & (AFMT_S32_BE | AFMT_S32_LE))
    cmd |= C_MODE24;

  if (portc->speedsel > 2)
    dblbit = C_DS;

  speedsel = (portc->speedsel % 3) + 1;
  cmd |= (speedsel << 4) | dblbit;

  if (dblbit != prev_dblbit)
    {
      write_command (devc, cmd | C_PD);
      oss_udelay (10);
    }
  write_command (devc, cmd);
  portc->outptr = 0;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
  return 0;
}

/*ARGSUSED*/
static int
digi32_prepare_for_input (int dev, int bsize, int bcount)
{
  int i, status, fbits;
  digi32_devc *devc = audio_engines[dev]->devc;
  digi32_portc *portc = audio_engines[dev]->portc;
  int cmd = devc->cmd;
  int dblbit = 0, prev_dblbit, speedsel;

  if (devc->cmd & C_AUTOSYNC)
    {
      devc->cmd &= ~C_AUTOSYNC;	/* Autosync must be used for input */
      portc->tmp_autosync = 1;
    }

  write_command (devc, devc->cmd);
  oss_udelay (100);

  for (i = 0; i < 1000; i++)
    {
      oss_udelay (10);
      status = PCI_READL (devc->osdev, devc->pCTRL);

      if (!(status & S_ERF))
	break;
    }

  if (i >= 1000)
    {
      cmn_err (CE_WARN, "No input signal detected\n");
      return OSS_EIO;
    }

  devc->speed_locked = 1;
  fbits = (status & S_FBITS) >> S_FSHIFT;
  portc->speed = fbit_tab[devc->cs841x_part][fbits];
  DDB (cmn_err
       (CE_WARN, "digi32: Measured input sampling rate is %d\n",
	portc->speed));

  if (devc->active_device_count > 0)
    return 0;

  if (bsize != 8 * 1024)
    {
      cmn_err (CE_CONT, "Illegal fragment size %d\n", bsize);
      return OSS_EIO;
    }

  prev_dblbit = cmd & C_DS;
  cmd &= C_EMP | C_PRO | C_SEL | C_MUTE | C_INP_0 | C_INP_1 | C_AUTOSYNC;
  cmd |= C_RESET;

  if (portc->bits & (AFMT_S32_BE | AFMT_S32_LE))
    cmd |= C_MODE24;

  if (portc->speedsel > 2)
    dblbit = C_DS;

  speedsel = (portc->speedsel % 3) + 1;
  cmd |= (speedsel << 4) | dblbit;

  if (dblbit != prev_dblbit)
    {
      write_command (devc, cmd | C_PD);
      oss_udelay (10);
    }
  write_command (devc, cmd);
  portc->inptr = 0;

  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
  return 0;
}

/*ARGSUSED*/
static int
digi32_alloc_buffer (int dev, dmap_t * dmap, int direction)
{
  extern int digi32_buffsize;

  if (digi32_buffsize < 512)	/* Given in kilobytes */
    digi32_buffsize *= 1024;

  if (digi32_buffsize < 4096)	/* Smaller ones not acceptable */
    digi32_buffsize = 64 * 1024;

  if (dmap->dmabuf != NULL)
    return 0;
  dmap->dmabuf_phys = 0;	/* Not mmap() capable */
  dmap->dmabuf = KERNEL_MALLOC (digi32_buffsize);
  if (dmap->dmabuf == NULL)
    return OSS_ENOSPC;
  dmap->buffsize = digi32_buffsize;

  return 0;
}

/*ARGSUSED*/
static int
digi32_free_buffer (int dev, dmap_t * dmap, int direction)
{
  if (dmap->dmabuf == NULL)
    return 0;
  KERNEL_FREE (dmap->dmabuf);

  dmap->dmabuf = NULL;
  return 0;
}

#if 0
static int
digi32_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
}
#endif

/*ARGSUSED*/
static void
digi32_setup_fragments (int dev, dmap_p dmap, int direction)
{
  /* Make sure the whole sample RAM is covered by the buffer */
  dmap->nfrags = dmap->buffsize / dmap->fragment_size;
}

static audiodrv_t digi32_output_driver = {
  digi32_open,
  digi32_close,
  digi32_output_block,
  digi32_start_input,
  digi32_ioctl,
  digi32_prepare_for_input,
  digi32_prepare_for_output,
  digi32_reset,
  NULL,
  NULL,
  digi32_reset_input,
  digi32_reset_output,
  digi32_trigger,
  digi32_set_rate,
  digi32_set_format,
  digi32_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  digi32_alloc_buffer,
  digi32_free_buffer,
  NULL,
  NULL,
  NULL,				/* digi32_get_buffer_pointer */
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  digi32_setup_fragments
};

static int
attach_channel (digi32_devc * devc, int chnum, char *name,
		audiodrv_t * drv, int type)
{
  int adev, opts;
  digi32_portc *portc;

  if (chnum < 0 || chnum >= MAX_AUDIO_CHANNEL)
    return 0;

  portc = &devc->portc[chnum];
  opts =
    ADEV_DUPLEX | ADEV_STEREOONLY | ADEV_COLD | ADEV_NOMMAP | ADEV_NOVIRTUAL;

  if (chnum != 0)
     opts |= ADEV_SHADOW;

  if ((adev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
				    devc->osdev,
				    devc->osdev,
				    name, drv, sizeof (audiodrv_t),
				    opts,
				    AFMT_S16_NE | AFMT_S32_NE | AFMT_AC3,
				    devc, -1)) < 0)
    {
      return 0;
    }

  audio_engines[adev]->mixer_dev = devc->mixer_dev;
  audio_engines[adev]->portc = portc;
  audio_engines[adev]->caps |= DSP_CH_STEREO;
  audio_engines[adev]->min_block = 8 * 1024;
  audio_engines[adev]->max_block = 8 * 1024;
  audio_engines[adev]->min_rate = 32000;
  audio_engines[adev]->max_rate = (devc->model == MDL_PRO) ? 96000 : 48000;
  portc->open_mode = 0;
  portc->voice_chn = chnum;
  portc->speed = 48000;
  portc->bits = AFMT_S16_NE;
  portc->bytes_per_sample = 2;
  portc->type = type;
  return 1;
}

/*ARGSUSED*/
static int
digi32_mixer_ioctl (int dev, int audiodev, unsigned int cmd, ioctl_arg arg)
{
  if (cmd == SOUND_MIXER_READ_DEVMASK ||
      cmd == SOUND_MIXER_READ_RECMASK || cmd == SOUND_MIXER_READ_RECSRC)
    return *arg = 0;

  if (cmd == SOUND_MIXER_READ_VOLUME || cmd == SOUND_MIXER_READ_PCM)
    return *arg = 100 | (100 << 8);
  if (cmd == SOUND_MIXER_WRITE_VOLUME || cmd == SOUND_MIXER_WRITE_PCM)
    return *arg = 100 | (100 << 8);
  return OSS_EINVAL;
}

static mixer_driver_t digi32_mixer_driver = {
  digi32_mixer_ioctl
};

static int
digi32_set_control (int dev, int ctrl, unsigned int cmd, int value)
{
  digi32_devc *devc = mixer_devs[dev]->devc;
  int offs, nbits;
  unsigned int data;

  if (ctrl < 0)
    return OSS_EINVAL;

  offs = (ctrl >> 4) & 0x0f;	/* # of bits in the field */
  nbits = ctrl & 0x0f;		/* Shift amount */

  if (cmd == SNDCTL_MIX_READ)
    {
      data = devc->cmd;
      return (data >> offs) & ((1 << nbits) - 1);
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
      if (value < 0 || value >= (1 << nbits))
	return OSS_EINVAL;
      data = devc->cmd & ~(((1 << nbits) - 1) << offs);
      data |= (value & ((1 << nbits) - 1)) << offs;
      devc->cmd = data;
      return value;
    }

  return OSS_EINVAL;
}

static int
digi32_mix_init (int dev)
{
  digi32_devc *devc = mixer_devs[dev]->devc;
  int group, err;

  if ((group = mixer_ext_create_group (dev, 0, "DIGI32")) < 0)
    return group;

  if (devc->model == MDL_PRO)
    if ((err = mixer_ext_create_control (dev, group,
					 0xf1, digi32_set_control,
					 MIXT_ONOFF,
					 "DIGI32_DEEMPH", 1,
					 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
      return err;

  if ((err = mixer_ext_create_control (dev, group,
				       0xd1, digi32_set_control,
				       MIXT_ENUM,
				       "DIGI32_SYNC", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if (devc->model != MDL_8)
    if ((err = mixer_ext_create_control (dev, group,
					 0xa1, digi32_set_control,
					 MIXT_ENUM,
					 "DIGI32_AESMODE", 2,
					 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
      return err;

  if ((err = mixer_ext_create_control (dev, group,
				       0x91, digi32_set_control,
				       MIXT_ONOFF,
				       "DIGI32_MUTE", 1,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group,
				       0x62, digi32_set_control,
				       MIXT_ENUM,
				       "DIGI32_INPUT", 3,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;


  return 0;
}

int
init_digi32 (digi32_devc * devc)
{
  int my_mixer;
  char tmp[128];
  int ret;

  devc->mixer_dev = 0;

  if ((my_mixer = oss_install_mixer (OSS_MIXER_DRIVER_VERSION,
				     devc->osdev,
				     devc->osdev,
				     "RME Digi32 Control panel",
				     &digi32_mixer_driver,
				     sizeof (mixer_driver_t), devc)) < 0)
    {

      devc->mixer_dev = -1;
      return 0;
    }
  else
    {
      devc->mixer_dev = my_mixer;
      mixer_devs[my_mixer]->priority = -1;	/* Don't use as the default mixer */
      mixer_ext_set_init_fn (my_mixer, digi32_mix_init, 20);
    }

  ret =
    attach_channel (devc, 0, devc->chip_name, &digi32_output_driver, TY_BOTH);
  if (ret > 0)
    {
      sprintf (tmp, "%s (shadow)", devc->chip_name);
      ret = attach_channel (devc, 1, tmp, &digi32_output_driver, TY_BOTH);
    }
  /*setup the command */
  if (devc->model == MDL_PRO)
    write_command (devc, C_PD);	/* Reset DAC */

  write_command (devc, C_AUTOSYNC);
  return ret;
}


int
oss_digi32_attach (oss_device_t * osdev)
{
  digi32_devc *devc;
  unsigned char pci_irq_line, pci_revision;
  unsigned short pci_command, vendor, device;
  unsigned int pci_ioaddr;
  int err;

  DDB (cmn_err (CE_WARN, "Entered Digi32 detect routine\n"));

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  if ((vendor != RME_VENDOR_ID && vendor != RME_VENDOR_ID2) ||
      (device != RME_DIGI32_PRO &&
       device != RME_DIGI32 && device != RME_DIGI32_8))

    return 0;

  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, PCI_MEM_BASE_ADDRESS_0, &pci_ioaddr);

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

  switch (device)
    {
    case RME_DIGI32_PRO:
      devc->model = MDL_PRO;
      if (pci_revision == 150)
	{
	  devc->chip_name = "RME Digi 32 Pro (CS8414)";
	  devc->cs841x_part = CS8414;
	}
      else
	{
	  devc->chip_name = "RME Digi 32 Pro (CS8412)";
	  devc->cs841x_part = CS8412;
	}
      break;

    case RME_DIGI32_8:
      devc->chip_name = "RME Digi 32/8";
      devc->model = MDL_8;
      devc->cs841x_part = CS8412;
      break;

    case RME_DIGI32:
      devc->chip_name = "RME Digi32";
      devc->model = MDL_BASIC;
      devc->cs841x_part = CS8412;
      break;

    case RME_DIGI96_8_PRO:
      devc->chip_name = "RME Digi 96/8 Pro";
      devc->model = MDL_96_8_PRO;
      devc->cs841x_part = CS8414;
      break;
    }

  DDB (cmn_err (CE_WARN, "Found Digi32 at 0x%x\n", pci_ioaddr));

  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);

  oss_register_device (osdev, devc->chip_name);

  if ((err = oss_register_interrupts (devc->osdev, 0, digi32intr, NULL)) < 0)
    {
      cmn_err (CE_WARN, "Can't register interrupt handler, err=%d\n", err);
      return 0;
    }

  devc->linaddr = (char *) MAP_PCI_MEM (devc->osdev, 0, devc->physaddr,
					128 * 1024 + 4);
  if (devc->linaddr == NULL)
    {
      cmn_err (CE_WARN, "Can't ioremap PCI registers\n");
      return 0;
    }

  devc->pFIFO = (unsigned int *) devc->linaddr;
  devc->pCTRL = (unsigned int *) (devc->linaddr + 0x20000);

  return init_digi32 (devc);	/* Detected */
}

int
oss_digi32_detach (oss_device_t * osdev)
{
  digi32_devc *devc = (digi32_devc *) osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  oss_unregister_interrupts (devc->osdev);

  MUTEX_CLEANUP (devc->mutex);

  UNMAP_PCI_MEM (devc->osdev, 0, devc->physaddr, devc->linaddr,
		 128 * 1024 + 4);

  oss_unregister_device (devc->osdev);

  return 1;
}
