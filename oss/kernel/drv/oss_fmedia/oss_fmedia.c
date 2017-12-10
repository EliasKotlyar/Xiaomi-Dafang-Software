/*
 * Purpose: Driver for FM801 FM801 PCI audio controller.
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

#include "oss_fmedia_cfg.h"
#include "ac97.h"
#include "oss_pci.h"
#include "uart401.h"


#define FORTEMEDIA_VENDOR_ID	0x1319
#define FORTEMEDIA_FM801	0x0801

/* Register Definitions */
#define AC97_CMD		0x2A
#define AC97_DATA		0x2C
#define CODEC_CONTROL		0x22
#define PCM_VOL			0x00
#define FM_VOL			0x02
#define I2S_VOL			0x04
#define DIG_REC_SRC		0x06
#define PLAY_CONTROL		0x08
#define PLAY_SIZE		0x0A
#define PLAY_BUF1_ADDR		0x0C
#define PLAY_BUF2_ADDR		0x10
#define REC_CONTROL		0x14
#define REC_SIZE		0x16
#define REC_BUF1_ADDR		0x18
#define REC_BUF2_ADDR		0x1C
#define I2S_MODE		0x24
#define HW_VOL_CONTROL		0x26
#define I2S_CONTROL		0x29
#define MPU_DATA		0x30
#define MPU_COMMAND		0x31
#define GPIO_CONTROL		0x52
#define GENERAL_CONTROL		0x54
#define IRQ_MASK		0x56
#define IRQ_STATUS		0x5B

/* playback and record control register bits */
#define FM801_BUF1_LAST         (1<<1)
#define FM801_BUF2_LAST         (1<<2)
#define FM801_START             (1<<5)
#define FM801_PAUSE             (1<<6)
#define FM801_IMMED_STOP        (1<<7)

/* IRQ status bits */
#define IRQ_PLAY      		(1<<8)
#define IRQ_REC			(1<<9)
#define IRQ_HWVOL		(1<<14)
#define IRQ_MPU           	(1<<15)

#define MAX_PORTC 2

typedef struct fm801_portc
{
  int speed, bits, channels;
  int open_mode;
  int audio_enabled;
  int trigger_bits;
  int audiodev;
} fm801_portc;

typedef struct fm801_devc
{
  oss_device_t *osdev;
  oss_native_word base, mpu_base;
  int mpu_attached, fm_attached;
  int irq, mpu_irq;
  int irq_allocated;
  volatile unsigned char intr_mask;
  int model;
#define MDL_FM801AS		1
#define MDL_FM801AU		2
  char *chip_name;
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;
  /* Audio parameters */
  int audio_initialized;
  int open_mode;
  fm801_portc portc[MAX_PORTC];
  int play_flag, play_count;
  int rec_flag, rec_count;
  /* Mixer parameters */
  ac97_devc ac97devc, ac97devc2;
  int mixer_dev, mixer2_dev;
} fm801_devc;

extern int fmedia_mpu_irq;

static struct
{
  unsigned int rate;
  unsigned int lower;
  unsigned int upper;
  unsigned char freq;
} rate_lookup[] =
{
  {
  5512, (0 + 5512) / 2, (5512 + 8000) / 2, 0x0},
  {
  8000, (5512 + 8000) / 2, (8000 + 9600) / 2, 0x1},
  {
  9600, (8000 + 9600) / 2, (9600 + 11025) / 2, 0x2},
  {
  11025, (9600 + 11025) / 2, (11025 + 16000) / 2, 0x3},
  {
  16000, (11025 + 16000) / 2, (16000 + 19200) / 2, 0x4},
  {
  19200, (16000 + 19200) / 2, (19200 + 22050) / 2, 0x5},
  {
  22050, (19200 + 22050) / 2, (22050 + 32000) / 2, 0x6},
  {
  32000, (22050 + 32000) / 2, (32000 + 38400) / 2, 0x7},
  {
  38400, (32000 + 38400) / 2, (38400 + 44100) / 2, 0x8},
  {
  44100, (38400 + 44100) / 2, (44100 + 48000) / 2, 0x9},
  {
  48000, (48000 + 44100) / 2, (48000 + 96000) / 2, 0xA}
};

static int
ac97_write (void *devc_, int index, int data)
{
  fm801_devc *devc = devc_;
  int idx;
  oss_native_word flags;
  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  /*
   * Wait until the codec interface is ready..
   */
  for (idx = 0; idx < 10000; idx++)
    {
      if (!(INW (devc->osdev, devc->base + AC97_CMD) & (1 << 9)))
	break;
      oss_udelay (10);
    }
  if (INW (devc->osdev, devc->base + AC97_CMD) & (1 << 9))
    {
      DDB (cmn_err (CE_WARN, "AC97 busy\n"));
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return OSS_EIO;
    }
  /* write data and address */
  OUTW (devc->osdev, data, devc->base + AC97_DATA);
  OUTW (devc->osdev, index | (0 << 10), devc->base + AC97_CMD);
  /*
   * Wait until the write command is completed..
   */
  for (idx = 0; idx < 1000; idx++)
    {
      if (!(INW (devc->osdev, devc->base + AC97_CMD) & (1 << 9)))
	break;
      oss_udelay (10);
    }
  if (INW (devc->osdev, devc->base + AC97_CMD) & (1 << 9))
    {
      DDB (cmn_err (CE_WARN, "AC97 busy (1)\n"));
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return OSS_EIO;
    }
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return 0;
}

static int
ac97_read (void *devc_, int index)
{
  fm801_devc *devc = devc_;
  int idx;
  oss_native_word flags;
  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  /*
   * Wait until the codec interface is not ready..
   */
  for (idx = 0; idx < 10000; idx++)
    {
      if (!(INW (devc->osdev, devc->base + AC97_CMD) & (1 << 9)))
	break;
      oss_udelay (10);
    }
  if (INW (devc->osdev, devc->base + AC97_CMD) & (1 << 9))
    {
      DDB (cmn_err (CE_WARN, "AC97 (read) not ready (1)\n"));
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return 0;
    }
  /* read command */
  OUTW (devc->osdev, index | (0 << 10) | (1 << 7), devc->base + AC97_CMD);
  for (idx = 0; idx < 10000; idx++)
    {
      if (!(INW (devc->osdev, devc->base + AC97_CMD) & (1 << 9)))
	break;
      oss_udelay (10);
    }
  /*
   * Wait until the codec interface is not ready..
   */
  if (INW (devc->osdev, devc->base + AC97_CMD) & (1 << 9))
    {
      DDB (cmn_err (CE_WARN, "AC97 (read) not ready(2)\n"));
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return 0;
    }
  for (idx = 0; idx < 10000; idx++)
    {
      if (INW (devc->osdev, devc->base + AC97_CMD) & (1 << 8))
	break;
      oss_udelay (10);
    }
  if (!(INW (devc->osdev, devc->base + AC97_CMD) & (1 << 8)))
    {
      cmn_err (CE_WARN, "AC97 (read) data not valid (2)\n");
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return 0;
    }
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return INW (devc->osdev, devc->base + AC97_DATA);
}

static int
ac97_write2 (void *devc_, int index, int data)
{
  fm801_devc *devc = devc_;
  int idx;
  oss_native_word flags;
  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  /*
   * Wait until the codec interface is ready..
   */
  for (idx = 0; idx < 10000; idx++)
    {
      if (!(INW (devc->osdev, devc->base + AC97_CMD) & (1 << 9)))
	break;
      oss_udelay (10);
    }
  if (INW (devc->osdev, devc->base + AC97_CMD) & (1 << 9))
    {
      DDB (cmn_err (CE_WARN, "Secondary AC97 busy\n"));
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return OSS_EIO;
    }
  /* write data and address */
  OUTW (devc->osdev, data, devc->base + AC97_DATA);
  OUTW (devc->osdev, index | (0x1 << 10), devc->base + AC97_CMD);
  /*
   * Wait until the write command is completed..
   */
  for (idx = 0; idx < 1000; idx++)
    {
      if (!(INW (devc->osdev, devc->base + AC97_CMD) & (1 << 9)))
	break;
      oss_udelay (10);
    }
  if (INW (devc->osdev, devc->base + AC97_CMD) & (1 << 9))
    {
      DDB (cmn_err (CE_WARN, "Secondary AC97 busy (1)\n"));
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return OSS_EIO;
    }
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return 0;
}

static int
ac97_read2 (void *devc_, int index)
{
  fm801_devc *devc = devc_;
  int idx;
  oss_native_word flags;
  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  /*
   * Wait until the codec interface is not ready..
   */
  for (idx = 0; idx < 10000; idx++)
    {
      if (!(INW (devc->osdev, devc->base + AC97_CMD) & (1 << 9)))
	break;
      oss_udelay (10);
    }
  if (INW (devc->osdev, devc->base + AC97_CMD) & (1 << 9))
    {
      DDB (cmn_err (CE_WARN, "Secondary AC97 (read) not ready (1)\n"));
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return 0;
    }
  /* read command */
  OUTW (devc->osdev, index | (0x1 << 10) | (1 << 7), devc->base + AC97_CMD);
  for (idx = 0; idx < 10000; idx++)
    {
      if (!(INW (devc->osdev, devc->base + AC97_CMD) & (1 << 9)))
	break;
      oss_udelay (10);
    }
  /*
   * Wait until the codec interface is not ready..
   */
  if (INW (devc->osdev, devc->base + AC97_CMD) & (1 << 9))
    {
      DDB (cmn_err (CE_WARN, "Secondary AC97 (read) not ready(2)\n"));
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return 0;
    }
  for (idx = 0; idx < 10000; idx++)
    {
      if (INW (devc->osdev, devc->base + AC97_CMD) & (1 << 8))
	break;
      oss_udelay (10);
    }
  if (!(INW (devc->osdev, devc->base + AC97_CMD) & (1 << 8)))
    {
      cmn_err (CE_WARN, "Secondary AC97 (read) data not valid (2)\n");
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return 0;
    }
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return INW (devc->osdev, devc->base + AC97_DATA);
}

static int
fm801intr (oss_device_t * osdev)
{
  fm801_devc *devc = (fm801_devc *) osdev->devc;
  unsigned int status;
  int i;
  int serviced = 0;
  status = INB (devc->osdev, devc->base + IRQ_STATUS);
  /* Playback interrupt */
  if (status & 0x01)
    {
      serviced = 1;
      for (i = 0; i < MAX_PORTC; i++)
	{
	  fm801_portc *portc = &devc->portc[i];
	  if (portc->trigger_bits & PCM_ENABLE_OUTPUT)
	    {
	      dmap_t *dmapout = audio_engines[portc->audiodev]->dmap_out;
	      devc->play_count++;
	      if (devc->play_count == dmapout->nfrags)
		devc->play_count = 0;
	      if (devc->play_flag)
		{
		  OUTL (devc->osdev, dmapout->dmabuf_phys +
			devc->play_count * dmapout->fragment_size,
			devc->base + PLAY_BUF1_ADDR);
		}
	      else
		{
		  OUTL (devc->osdev, dmapout->dmabuf_phys +
			devc->play_count * dmapout->fragment_size,
			devc->base + PLAY_BUF2_ADDR);
		}
	      devc->play_flag = !devc->play_flag;
	      oss_audio_outputintr (portc->audiodev, 1);
	    }
	}
      OUTB (devc->osdev, status | 0x01, devc->base + IRQ_STATUS);
    }
  /* Record Interrupt */
  if (status & 0x02)
    {
      serviced = 1;
      for (i = 0; i < MAX_PORTC; i++)
	{
	  fm801_portc *portc = &devc->portc[i];
	  if (portc->trigger_bits & PCM_ENABLE_INPUT)
	    {
	      dmap_t *dmapin = audio_engines[portc->audiodev]->dmap_in;
	      devc->rec_count++;
	      if (devc->rec_count == dmapin->nfrags)
		devc->rec_count = 0;
	      if (devc->rec_flag)
		{
		  OUTL (devc->osdev, dmapin->dmabuf_phys +
			devc->rec_count * dmapin->fragment_size,
			devc->base + REC_BUF1_ADDR);
		}
	      else
		OUTL (devc->osdev, dmapin->dmabuf_phys +
		      devc->rec_count * dmapin->fragment_size,
		      devc->base + REC_BUF2_ADDR);
	      devc->rec_flag = !devc->rec_flag;
	      oss_audio_inputintr (portc->audiodev, 0);
	    }
	}
      OUTB (devc->osdev, status | 0x02, devc->base + IRQ_STATUS);
    }
  /* MIDI Interrupt */
  if (status & 0x80)
    {
      serviced = 1;
      /* uart401intr (INT_HANDLER_CALL (devc->mpu_irq)); */
      OUTB (devc->osdev, status | 0x80, devc->base + IRQ_STATUS);
    }
  return serviced;
}

static int
fm801_audio_set_rate (int dev, int arg)
{
  fm801_portc *portc = audio_engines[dev]->portc;
  if (arg == 0)
    return portc->speed;
  if (arg > 48000)
    arg = 48000;
  if (arg < 5000)
    arg = 5000;
  portc->speed = arg;
  return portc->speed;
}

static short
fm801_audio_set_channels (int dev, short arg)
{
  fm801_portc *portc = audio_engines[dev]->portc;
  if (arg>6)
     arg=6;
  if ((arg != 1) && (arg != 2) && (arg != 4) && (arg != 6))
    return portc->channels;
  portc->channels = arg;
  return portc->channels;
}

static unsigned int
fm801_audio_set_format (int dev, unsigned int arg)
{
  fm801_portc *portc = audio_engines[dev]->portc;
  if (arg == 0)
    return portc->bits;
  if (!(arg & (AFMT_U8 | AFMT_S16_LE)))
    return portc->bits;
  portc->bits = arg;
  return portc->bits;
}

/*ARGSUSED*/
static int
fm801_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void fm801_audio_trigger (int dev, int state);

static void
fm801_audio_reset (int dev)
{
  fm801_audio_trigger (dev, 0);
}

static void
fm801_audio_reset_input (int dev)
{
  fm801_portc *portc = audio_engines[dev]->portc;
  fm801_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
fm801_audio_reset_output (int dev)
{
  fm801_portc *portc = audio_engines[dev]->portc;
  fm801_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

/*ARGSUSED*/
static int
fm801_audio_open (int dev, int mode, int open_flags)
{
  fm801_portc *portc = audio_engines[dev]->portc;
  fm801_devc *devc = audio_engines[dev]->devc;
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
fm801_audio_close (int dev, int mode)
{
  fm801_portc *portc = audio_engines[dev]->portc;
  fm801_devc *devc = audio_engines[dev]->devc;
  fm801_audio_reset (dev);
  portc->open_mode = 0;
  devc->open_mode &= ~mode;
  portc->audio_enabled &= ~mode;
}

/*ARGSUSED*/
static void
fm801_audio_output_block (int dev, oss_native_word buf, int count,
			  int fragsize, int intrflag)
{
  fm801_portc *portc = audio_engines[dev]->portc;
  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
}

/*ARGSUSED*/
static void
fm801_audio_start_input (int dev, oss_native_word buf, int count,
			 int fragsize, int intrflag)
{
  fm801_portc *portc = audio_engines[dev]->portc;
  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
}

static void
fm801_audio_trigger (int dev, int state)
{
  fm801_portc *portc = audio_engines[dev]->portc;
  fm801_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;
  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode & OPEN_WRITE)
    {
      if (state & PCM_ENABLE_OUTPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      OUTW (devc->osdev,
		    INW (devc->osdev,
			 devc->base +
			 PLAY_CONTROL) | FM801_START | FM801_IMMED_STOP,
		    devc->base + PLAY_CONTROL);
	      portc->trigger_bits |= PCM_ENABLE_OUTPUT;
	    }
	}
      else
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      (portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
	      portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
	      OUTW (devc->osdev,
		    (INW (devc->osdev, devc->base + PLAY_CONTROL) &
		     (~FM801_START | FM801_IMMED_STOP)) | (FM801_BUF1_LAST |
							   FM801_BUF2_LAST),
		    devc->base + PLAY_CONTROL);
	    }
	}
    }
  if (portc->open_mode & OPEN_READ)
    {
      if (state & PCM_ENABLE_INPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_INPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_INPUT))
	    {
	      OUTW (devc->osdev, INW (devc->osdev, devc->base + REC_CONTROL) |
		    FM801_START | FM801_IMMED_STOP, devc->base + REC_CONTROL);
	      portc->trigger_bits |= PCM_ENABLE_INPUT;
	    }
	}
      else
	{
	  if ((portc->audio_enabled & PCM_ENABLE_INPUT) &&
	      (portc->trigger_bits & PCM_ENABLE_INPUT))
	    {
	      portc->audio_enabled &= ~PCM_ENABLE_INPUT;
	      portc->trigger_bits &= ~PCM_ENABLE_INPUT;
	      OUTW (devc->osdev,
		    (INW (devc->osdev, devc->base + REC_CONTROL) &
		     (~FM801_START | FM801_IMMED_STOP)) | (FM801_BUF1_LAST |
							   FM801_BUF2_LAST),
		    devc->base + REC_CONTROL);
	    }
	}
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static unsigned char
sampling_rate (unsigned int rate)
{
  unsigned char freq = 1;
  int i;
  if (rate > 48000)
    rate = 48000;
  if (rate < 5512)
    rate = 5512;
  for (i = 0; i < sizeof (rate_lookup) / sizeof (rate_lookup[0]); i++)
    {
      if (rate > rate_lookup[i].lower && rate <= rate_lookup[i].upper)
	{
	  rate = rate_lookup[i].rate;
	  freq = rate_lookup[i].freq;
	  break;
	}
    }
  return (freq);
}

/*ARGSUSED*/
static int
fm801_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  fm801_devc *devc = audio_engines[dev]->devc;
  fm801_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_in;
  unsigned short value;
  unsigned char frequency;
  oss_native_word flags;
  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  value = 0x0000;
  if (portc->channels == 2)
    value |= 0x8000;
  if (portc->bits == 16)
    value |= 0x4000;
  frequency = sampling_rate (portc->speed);
  value |= (frequency << 8);
  OUTW (devc->osdev, value, devc->base + REC_CONTROL);
  OUTW (devc->osdev, dmap->fragment_size - 1, devc->base + REC_SIZE);
  OUTL (devc->osdev, dmap->dmabuf_phys, devc->base + REC_BUF1_ADDR);
  OUTL (devc->osdev, dmap->dmabuf_phys + dmap->fragment_size,
	devc->base + REC_BUF2_ADDR);
  devc->rec_flag = 1;
  devc->rec_count = 1;
  portc->audio_enabled &= ~PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/
static int
fm801_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  fm801_devc *devc = audio_engines[dev]->devc;
  fm801_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_out;
  unsigned short value;
  unsigned char frequency;
  oss_native_word flags;
  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  value = 0x0000;
  if (portc->channels > 1)
    value |= 0x8000;
  if (portc->bits == 16)
    value |= 0x4000;
  frequency = sampling_rate (portc->speed);
  value |= (frequency << 8);
  if (portc->channels == 4)
    value |= (1 << 12);		/* 4channel output */
  if (portc->channels == 6)
    value |= (1 << 13);		/* 6channel output */
  OUTW (devc->osdev, value, devc->base + PLAY_CONTROL);
  OUTW (devc->osdev, dmap->fragment_size - 1, devc->base + PLAY_SIZE);
  OUTL (devc->osdev, dmap->dmabuf_phys, devc->base + PLAY_BUF1_ADDR);
  OUTL (devc->osdev, dmap->dmabuf_phys + dmap->fragment_size,
	devc->base + PLAY_BUF2_ADDR);
  devc->play_flag = 1;
  devc->play_count = 1;
  portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

static int
fm801_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  fm801_devc *devc = audio_engines[dev]->devc;
  int ptr = 0;
  oss_native_word flags;
  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  if (direction == PCM_ENABLE_OUTPUT)
    {
      if (devc->play_flag)
	ptr = INL (devc->osdev, devc->base + PLAY_BUF1_ADDR);
      else
	ptr = INL (devc->osdev, devc->base + PLAY_BUF2_ADDR);
    }
  if (direction == PCM_ENABLE_INPUT)
    {
      if (devc->rec_flag)
	ptr = INL (devc->osdev, devc->base + REC_BUF1_ADDR);
      else
	ptr = INL (devc->osdev, devc->base + REC_BUF2_ADDR);
    }
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return ptr % dmap->bytes_in_use;
}

static audiodrv_t fm801_audio_driver = {
  fm801_audio_open,
  fm801_audio_close,
  fm801_audio_output_block,
  fm801_audio_start_input,
  fm801_audio_ioctl,
  fm801_audio_prepare_for_input,
  fm801_audio_prepare_for_output,
  fm801_audio_reset,
  NULL,
  NULL,
  fm801_audio_reset_input,
  fm801_audio_reset_output,
  fm801_audio_trigger,
  fm801_audio_set_rate,
  fm801_audio_set_format,
  fm801_audio_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,				/* fm801_alloc_buffer */
  NULL,				/* fm801_free_buffer */
  NULL,
  NULL,
  fm801_get_buffer_pointer
};

static void
init_audio (fm801_devc * devc)
{
  devc->audio_initialized = 1;
}

static void
uninit_audio (fm801_devc * devc)
{
  unsigned int irqmask;
  devc->audio_initialized = 0;
#ifndef _TRU64_UNIX
  /* interrupt setup - mask MPU, PLAYBACK & CAPTURE */
  irqmask = INW (devc->osdev, devc->base + IRQ_MASK);
  irqmask |= 0x00C3;
  OUTW (devc->osdev, irqmask, devc->base + IRQ_MASK);
  pci_write_config_word (devc->osdev, 0x40, 0x807F);
#endif
}

#ifdef OBSOLETED_STUFF
/*
 * This device has "ISA style" MIDI and FM subsystems. Such devices don't
 * use PCI config space for the I/O ports and interrupts. Instead the driver
 * needs to allocate proper resources itself. This functionality is no longer
 * possible. For this reason the MIDI and FM parts are not accessible.
 */
static void
attach_fm (fm801_devc * devc)
{
  if (!opl3_detect (devc->base + 0x68, devc->osdev))
    return;
  opl3_init (devc->base + 0x68, devc->osdev);
  devc->fm_attached = 1;
}

static void
attach_mpu (fm801_devc * devc)
{
  struct address_info hw_config;
  hw_config.io_base = devc->mpu_base;
  hw_config.irq = devc->mpu_irq;
  hw_config.dma = -1;
  hw_config.dma2 = -1;
  hw_config.always_detect = 0;
  hw_config.name = "FM801 MPU-401";
  hw_config.driver_use_1 = 0;
  hw_config.driver_use_2 = 0;
  hw_config.osdev = devc->osdev;
#ifdef CREATE_OSP
  CREATE_OSP (hw_config.osdev);
#endif
  hw_config.card_subtype = 0;
  if (!probe_uart401 (&hw_config))
    {
      cmn_err (CE_WARN, "MPU-401 was not detected\n");
      return;
    }
  devc->mpu_attached = 1;
  attach_uart401 (&hw_config);
}

static void
unload_mpu (fm801_devc * devc)
{
  struct address_info hw_config;
  hw_config.io_base = -devc->mpu_base;
  hw_config.irq = devc->mpu_irq;
  hw_config.dma = -1;
  hw_config.dma2 = -1;
  hw_config.always_detect = 0;
  hw_config.name = "FM801 MPU-401";
  hw_config.driver_use_1 = 0;
  hw_config.driver_use_2 = 0;
  hw_config.osdev = devc->osdev;
#ifdef CREATE_OSP
  CREATE_OSP (hw_config.osdev);
#endif
  hw_config.card_subtype = 0;
  devc->mpu_attached = 0;
  unload_uart401 (&hw_config);
}
#endif

static int
init_fm801 (fm801_devc * devc)
{
  int my_mixer, my_mixer2;
  int legacy;
  int irqmask;
  int adev;
  int first_dev = 0;
  int i;

  devc->mpu_attached = devc->fm_attached = 0;

  legacy = 0;

#if !defined(__hpux) && !defined(sparc) && !defined(_TRU64)
  /* Enable Legacy FM, MPU and Joystick ports */
  legacy = 0x001E;
  switch (fmedia_mpu_irq)
    {
    case 5:
      legacy |= 0x0000;
      break;
    case 7:
      legacy |= 0x0800;
      break;
    case 9:
      legacy |= 0x1000;
      break;
    case 10:
      legacy |= 0x1800;
      break;
    case 11:
      legacy |= 0x2000;
      break;
    }
#endif

  pci_write_config_word (devc->osdev, 0x40, legacy);

  /* codec cold reset + AC'97 warm reset */
  OUTW (devc->osdev, (1 << 5) | (1 << 6), devc->base + CODEC_CONTROL);
  oss_udelay (10);
  OUTW (devc->osdev, 0, devc->base + CODEC_CONTROL);

  if (devc->model == MDL_FM801AU)
    {
      OUTW (devc->osdev, (1 << 7), devc->base + CODEC_CONTROL);
      oss_udelay (10);
    }

  /* init volume */
  OUTW (devc->osdev, 0x0808, devc->base + PCM_VOL);
  OUTW (devc->osdev, 0x0808, devc->base + FM_VOL);
  OUTW (devc->osdev, 0x0808, devc->base + I2S_VOL);
  /* interrupt setup - unmask MPU, PLAYBACK & CAPTURE */
  irqmask = INW (devc->osdev, devc->base + IRQ_MASK);
  irqmask &= ~0x0083;
  OUTW (devc->osdev, irqmask, devc->base + IRQ_MASK);
  OUTW (devc->osdev, 0x280C, devc->base + GENERAL_CONTROL);
  OUTW (devc->osdev, 0x0, devc->base + I2S_MODE);
#if !defined(__hpux) && !defined(sparc) && !defined(_TRU64_UNIX)
  /* interrupt clear */
  /*
   * TODO: Check this. Unaligned I/O access causes a crash onder non-x86
   */
  OUTW (devc->osdev, IRQ_PLAY | IRQ_REC | IRQ_MPU, devc->base + IRQ_STATUS);
#endif
  /*
   * Enable BusMasterMode and IOSpace Access
   */
#ifdef OBSOLETED_STUFF
  attach_fm (devc);
  attach_mpu (devc);
#endif
  my_mixer = ac97_install (&devc->ac97devc, "FM801 AC97 Mixer",
			   ac97_read, ac97_write, devc, devc->osdev);

  if (my_mixer >= 0)
    {
      devc->mixer_dev = my_mixer;
      if (devc->model == MDL_FM801AU)
	{
	  my_mixer2 = ac97_install (&devc->ac97devc2, "FM801 AC97 Secondary",
				    ac97_read2, ac97_write2, devc,
				    devc->osdev);
	  if (my_mixer2 >= 0)
	    devc->mixer2_dev = my_mixer2;
	}
    }
  else
    return 0;

  for (i = 0; i < MAX_PORTC; i++)
    {
      char tmp_name[100];
      fm801_portc *portc = &devc->portc[i];
      int caps = ADEV_AUTOMODE;
      if (i == 0)
	{
	  strcpy (tmp_name, devc->chip_name);
	  caps |= ADEV_DUPLEX;
	}
      else
	{
	  strcpy (tmp_name, devc->chip_name);
	  caps |= ADEV_DUPLEX | ADEV_SHADOW;
	}
      if ((adev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
					devc->osdev,
					devc->osdev,
					tmp_name,
					&fm801_audio_driver,
					sizeof (audiodrv_t),
					caps,
					AFMT_U8 | AFMT_S16_LE, devc, -1)) < 0)
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
	  audio_engines[adev]->min_rate = 5000;
	  audio_engines[adev]->max_rate = 48000;
	  audio_engines[adev]->caps |= PCM_CAP_FREERATE;
	  audio_engines[adev]->min_channels = 2;
	  audio_engines[adev]->max_channels = 6;
	  portc->open_mode = 0;
	  portc->audiodev = adev;
	  portc->audio_enabled = 0;
#ifdef CONFIG_OSS_VMIX
	  if (i == 0)
	     vmix_attach_audiodev(devc->osdev, adev, -1, 0);
#endif
	}
      init_audio (devc);
    }
  return 1;
}

int
oss_fmedia_attach (oss_device_t * osdev)
{
  unsigned char pci_irq_line, pci_revision;
  unsigned short pci_command, vendor, device;
  unsigned int pci_ioaddr;
  int err;
  fm801_devc *devc;

  DDB (cmn_err (CE_WARN, "Entered FM801 FM801 probe routine\n"));

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  if (vendor != FORTEMEDIA_VENDOR_ID || device != FORTEMEDIA_FM801)
    return 0;

  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_0, &pci_ioaddr);

  DDB (cmn_err (CE_WARN, "FM801 I/O base %04x\n", pci_ioaddr));

  if (pci_irq_line == 0)
    {
      cmn_err (CE_WARN, "IRQ not assigned by BIOS (%d). Can't continue\n",
	       pci_irq_line);
      return 0;
    }

  if (pci_ioaddr == 0)
    {
      cmn_err (CE_WARN, "I/O address not assigned by BIOS.\n");
      return 0;
    }

  if ((devc = PMALLOC (osdev, sizeof (*devc))) == NULL)
    {
      cmn_err (CE_WARN, "Out of memory\n");
      return 0;
    }
  devc->osdev = osdev;
  osdev->devc = devc;

  devc->base = MAP_PCI_IOADDR (devc->osdev, 0, pci_ioaddr);

  /* Remove I/O space marker in bit 0. */
  devc->base &= ~3;
  devc->mpu_irq = fmedia_mpu_irq;
  devc->mpu_base = devc->base + 0x30;
  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_IO;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  switch (pci_revision)
    {
    case 0xb1:
      devc->model = MDL_FM801AS;
      devc->chip_name = "ForteMedia FM801-AS";
      break;
    case 0xb2:
      devc->model = MDL_FM801AU;
      devc->chip_name = "ForteMedia FM801-AU";
      break;
    }

  oss_register_device (osdev, devc->chip_name);
  if ((err = oss_register_interrupts (osdev, 0, fm801intr, NULL)) < 0)
    {
      cmn_err (CE_WARN, "Error installing interrupt handler: %x\n", err);
      return 0;
    }

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);

  return init_fm801 (devc);	/* Detected */
}

int
oss_fmedia_detach (oss_device_t * osdev)
{
  fm801_devc *devc = (fm801_devc *) osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  if (devc->audio_initialized)
    {
      uninit_audio (devc);
    }

  devc->audio_initialized = 0;

#ifdef OBSOLETED_STUFF
  if (devc->mpu_attached)
    {
      unload_mpu (devc);
      devc->mpu_attached = 0;
    }
#endif

  oss_unregister_interrupts (devc->osdev);
  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);
  UNMAP_PCI_IOADDR (devc->osdev, 0);
  oss_unregister_device (devc->osdev);
  return 1;
}
