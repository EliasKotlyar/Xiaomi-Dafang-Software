
/*
 * Purpose: Driver for S3 SonicVibes PCI audio controller.
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

#include "oss_s3vibes_cfg.h"
#include "oss_pci.h"

#ifdef USE_LICENSING
#include "private/licensing/licensing.h"
#endif

#define S3_VENDOR_ID	0x5333
#define S3_SONICVIBES	0xca00

#define MIXER_DEVS		(SOUND_MASK_LINE1 | SOUND_MASK_LINE2 | \
				 SOUND_MASK_MIC | SOUND_MASK_VOLUME | \
				 SOUND_MASK_CD | SOUND_MASK_LINE | \
				 SOUND_MASK_PCM | SOUND_MASK_SYNTH | \
				 SOUND_MASK_RECLEV|SOUND_MASK_DEPTH )

#define REC_DEVS		(SOUND_MASK_LINE1 | SOUND_MASK_LINE2 | \
				 SOUND_MASK_MIC | \
				 SOUND_MASK_CD | SOUND_MASK_LINE)

#define STEREO_DEVS		(SOUND_MASK_LINE1 | SOUND_MASK_LINE | \
				 SOUND_MASK_VOLUME | \
				 SOUND_MASK_CD | SOUND_MASK_LINE | \
				 SOUND_MASK_PCM | SOUND_MASK_RECLEV)

#define LEFT_CHN	0
#define RIGHT_CHN	1

static int default_mixer_levels[32] = {
  0x3232,			/* Master Volume */
  0x3232,			/* Bass */
  0x3232,			/* Treble */
  0x4b4b,			/* FM */
  0x3232,			/* PCM */
  0x1515,			/* PC Speaker */
  0x2020,			/* Ext Line */
  0x1010,			/* Mic */
  0x4b4b,			/* CD */
  0x0000,			/* Recording monitor */
  0x4b4b,			/* Second PCM */
  0x4b4b,			/* Recording level */
  0x4b4b,			/* Input gain */
  0x4b4b,			/* Output gain */
  0x2020,			/* Line1 */
  0x2020,			/* Line2 */
  0x1515			/* Line3 (usually line in) */
};

#define MAX_PORTC 2

typedef struct vibes_portc
{
  int audio_dev;
  int open_mode;
  int channels;
  int bits;
  int speed;
  int trigger_bits;
  int audio_enabled;
}
vibes_portc;

typedef struct vibes_devc
{
  oss_device_t *osdev;
  int sb_base;
  int enh_base;
  int fm_base;
  int mpu_base;
  int game_base;
  int dmaa_base;
  int dmac_base;
  int irq;
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;

  /* Mixer parameters */
  int my_mixer;
  int *levels;
  int recdevs;

  /* Audio parameters */
  int audio_open_mode;
  vibes_portc portc[MAX_PORTC];
}
vibes_devc;


static int
vibesintr (oss_device_t * osdev)
{
  vibes_devc *devc = (vibes_devc *) osdev->devc;
  vibes_portc *portc;
  unsigned char stat;
  int serviced = 0;
  int i;

  stat = INB (devc->osdev, devc->enh_base + 0x02);

  for (i = 0; i < MAX_PORTC; i++)
    {
      portc = &devc->portc[i];
      if ((stat & 0x01) && (portc->trigger_bits & PCM_ENABLE_OUTPUT))
	{
	  serviced = 1;
	  oss_audio_outputintr (portc->audio_dev, 1);
	}

      if ((stat & 0x04) && (portc->trigger_bits & PCM_ENABLE_INPUT))
	{
	  serviced = 1;
	  oss_audio_inputintr (portc->audio_dev, 0);
	}
    }
  return serviced;
}

static unsigned char
vibes_mixer_read (vibes_devc * devc, int reg)
{
  oss_native_word flags;
  unsigned char data, tmp;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  /* Select register index */
  tmp = INB (devc->osdev, devc->enh_base + 4) & 0xc0;
  OUTB (devc->osdev, tmp | (reg & 0x3f), devc->enh_base + 4);
  oss_udelay (10);
  /* Read the register */
  data = INB (devc->osdev, devc->enh_base + 5);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return data;
}

static void
vibes_mixer_write (vibes_devc * devc, int reg, unsigned char data)
{
  oss_native_word flags;
  unsigned char tmp;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  /* Select register index */
  tmp = INB (devc->osdev, devc->enh_base + 0x04) & 0xc0;
  OUTB (devc->osdev, tmp | (reg & 0x3f), devc->enh_base + 4);
  oss_udelay (10);
  /* Write the register */
  OUTB (devc->osdev, data, devc->enh_base + 0x05);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
}

static int
vibes_set_recmask (vibes_devc * devc, int mask)
{
  unsigned char bits = 0, tmp;
  oss_native_word flags;
  mask &= REC_DEVS;

  if (mask & (mask - 1))	/* More than one bits selected */
    mask = mask & (devc->recdevs ^ mask);	/* Pick the one recently turned on */

  switch (mask)
    {
    case SOUND_MASK_CD:
      bits = 0;
      break;
    case SOUND_MASK_LINE2:
      bits = 3;
      break;
    case SOUND_MASK_LINE:
      bits = 4;
      break;
    case SOUND_MASK_LINE1:
      bits = 5;
      break;
    case SOUND_MASK_MIC:
      bits = 6;
      break;

    default:			/* Unknown bit (combination) */
      mask = SOUND_MASK_MIC;
      bits = 6;
    }

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  tmp = vibes_mixer_read (devc, 0x00) & 0x1f;	/* Left ADC source */
  vibes_mixer_write (devc, 0x00, tmp | (bits << 5));
  tmp = vibes_mixer_read (devc, 0x01) & 0x1f;	/* Right ADC source */
  vibes_mixer_write (devc, 0x01, tmp | (bits << 5));
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return devc->recdevs = mask;
}

static int
vibes_mixer_get (vibes_devc * devc, int dev)
{
  if (!((1 << dev) & MIXER_DEVS))
    return OSS_EINVAL;

  return devc->levels[dev];
}

static void
set_volume (vibes_devc * devc, int reg, int level, int width)
{
  unsigned char tmp, bits;
  int scale = (1 << width) - 1;

  tmp = vibes_mixer_read (devc, reg) & ~(0x80 | scale);

  if (level == 0)
    bits = 0x80;
  else
    {
      bits = (scale * level) / 100;
      bits = scale - bits;	/* Reverse */
    }

  vibes_mixer_write (devc, reg, tmp | bits);
}

static int
vibes_mixer_set (vibes_devc * devc, int dev, int value)
{
  int left = value & 0x000000ff;
  int right = (value & 0x0000ff00) >> 8;

  if (left > 100)
    left = 100;
  if (right > 100)
    right = 100;

  switch (dev)
    {
    case SOUND_MIXER_LINE1:
      set_volume (devc, 0x02, left, 5);
      set_volume (devc, 0x03, right, 5);
      break;

    case SOUND_MIXER_CD:
      set_volume (devc, 0x04, left, 5);
      set_volume (devc, 0x05, right, 5);
      break;

    case SOUND_MIXER_LINE:
      set_volume (devc, 0x06, left, 5);
      set_volume (devc, 0x07, right, 5);
      break;

    case SOUND_MIXER_MIC:
      set_volume (devc, 0x08, left, 4);	/* Mono control with just 4 bits */
      right = left;
      break;

    case SOUND_MIXER_SYNTH:
      set_volume (devc, 0x0a, left, 5);
      set_volume (devc, 0x0b, right, 5);
      break;

    case SOUND_MIXER_LINE2:
      set_volume (devc, 0x0c, left, 5);
      set_volume (devc, 0x0d, right, 5);
      break;

    case SOUND_MIXER_VOLUME:
      set_volume (devc, 0x0e, left, 6);
      set_volume (devc, 0x0f, right, 6);
      break;

    case SOUND_MIXER_PCM:
      set_volume (devc, 0x10, left, 5);
      set_volume (devc, 0x11, right, 5);
      break;

    case SOUND_MIXER_RECLEV:
      {
	int tmp, bits;

	bits = (15 * left) / 100;
	bits = 15 - bits;	/* Reverse */
	tmp = vibes_mixer_read (devc, 0x00) & ~15;
	vibes_mixer_write (devc, 0x00, tmp | bits);

	bits = (15 * right) / 100;
	bits = 15 - bits;	/* Reverse */
	tmp = vibes_mixer_read (devc, 0x01) & ~15;
	vibes_mixer_write (devc, 0x01, tmp | bits);
      }
      break;

    case SOUND_MIXER_DEPTH:
      set_volume (devc, 0x2c, left, 3);
      right = left;
      break;
#if 0
    case SOUND_MIXER_CENTER:
      set_volume (devc, 0x2d, left, 3);
      right = left;
      break;
#endif
    }

  value = left | (right << 8);

  return devc->levels[dev] = value;
}

static void
vibes_mixer_reset (vibes_devc * devc)
{
  int i;

  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
    if (MIXER_DEVS & (1 << i))
      vibes_mixer_set (devc, i, devc->levels[i]);
  vibes_set_recmask (devc, SOUND_MASK_MIC);
}

/*ARGSUSED*/
static int
vibes_mixer_ioctl (int dev, int audiodev, unsigned int cmd, ioctl_arg arg)
{
  vibes_devc *devc = mixer_devs[dev]->devc;

  if (((cmd >> 8) & 0xff) == 'M')
    {
      int val;

      if (IOC_IS_OUTPUT (cmd))
	switch (cmd & 0xff)
	  {
	  case SOUND_MIXER_RECSRC:
	    val = *arg;
	    return *arg = vibes_set_recmask (devc, val);
	    break;

	  default:
	    val = *arg;
	    return *arg = vibes_mixer_set (devc, cmd & 0xff, val);
	  }
      else
	switch (cmd & 0xff)	/*
				 * Return parameters
				 */
	  {

	  case SOUND_MIXER_RECSRC:
	    return *arg = devc->recdevs;
	    break;

	  case SOUND_MIXER_DEVMASK:
	    return *arg = MIXER_DEVS;
	    break;

	  case SOUND_MIXER_STEREODEVS:
	    return *arg = STEREO_DEVS;
	    break;

	  case SOUND_MIXER_RECMASK:
	    return *arg = REC_DEVS;
	    break;

	  case SOUND_MIXER_CAPS:
	    return *arg = SOUND_CAP_EXCL_INPUT;
	    break;

	  default:
	    return *arg = vibes_mixer_get (devc, cmd & 0xff);
	  }
    }
  else
    return OSS_EINVAL;
}

static mixer_driver_t vibes_mixer_driver = {
  vibes_mixer_ioctl
};

static int
vibes_audio_set_rate (int dev, int arg)
{
  vibes_portc *portc = audio_engines[dev]->portc;

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
vibes_audio_set_channels (int dev, short arg)
{
  vibes_portc *portc = audio_engines[dev]->portc;

  if ((arg != 1) && (arg != 2))
    return portc->channels;
  portc->channels = arg;

  return portc->channels;
}

static unsigned int
vibes_audio_set_format (int dev, unsigned int arg)
{
  vibes_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->bits;

  if (!(arg & (AFMT_U8 | AFMT_S16_LE)))
    return portc->bits;
  portc->bits = arg;

  return portc->bits;
}

/*ARGSUSED*/
static int
vibes_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void
vibes_halt_output (int dev)
{
  vibes_devc *devc = audio_engines[dev]->devc;
  vibes_portc *portc = audio_engines[dev]->portc;

  unsigned char tmp;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  tmp = vibes_mixer_read (devc, 0x13) & ~0x01;
  vibes_mixer_write (devc, 0x13, tmp);
  OUTB (devc->osdev, 0xff, devc->dmaa_base + 0x0d);
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static void
vibes_halt_input (int dev)
{
  vibes_devc *devc = audio_engines[dev]->devc;
  vibes_portc *portc = audio_engines[dev]->portc;

  unsigned char tmp;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  tmp = vibes_mixer_read (devc, 0x13) & ~0x02;
  vibes_mixer_write (devc, 0x13, tmp);
  OUTB (devc->osdev, 0xff, devc->dmac_base + 0x0d);
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static void
vibes_audio_reset (int dev)
{
  vibes_portc *portc = audio_engines[dev]->portc;

  if (portc->open_mode & OPEN_WRITE)
    vibes_halt_output (dev);
  if (portc->open_mode & OPEN_READ)
    vibes_halt_input (dev);
}

/*ARGSUSED*/
static int
vibes_audio_open (int dev, int mode, int open_flags)
{
  oss_native_word flags;
  vibes_devc *devc = audio_engines[dev]->devc;
  vibes_portc *portc = audio_engines[dev]->portc;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (devc->audio_open_mode & mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  devc->audio_open_mode |= mode;
  portc->open_mode = mode;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

static void
vibes_audio_close (int dev, int mode)
{
  vibes_devc *devc = audio_engines[dev]->devc;
  vibes_portc *portc = audio_engines[dev]->portc;

  vibes_audio_reset (dev);
  devc->audio_open_mode &= ~mode;
  portc->open_mode = 0;
}

/*ARGSUSED*/
static void
vibes_audio_output_block (int dev, oss_native_word buf, int count,
			  int fragsize, int intrflag)
{
  vibes_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;
  int c = fragsize - 1;
  int dma_base = devc->dmaa_base;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  vibes_mixer_write (devc, 0x18, (c >> 8) & 0xff);
  vibes_mixer_write (devc, 0x19, c & 0xff);

  /* Setup DMA_A registers */
  count = count - 1;
  OUTB (devc->osdev, 0xff, dma_base + 0x0d);
  OUTB (devc->osdev, 0x01, dma_base + 0x0f);
  OUTB (devc->osdev, (buf) & 0xff, dma_base + 0x00);
  OUTB (devc->osdev, (buf >> 8) & 0xff, dma_base + 0x01);
  OUTB (devc->osdev, (buf >> 16) & 0xff, dma_base + 0x02);
  OUTB (devc->osdev, (buf >> 24) & 0xff, dma_base + 0x03);
  OUTB (devc->osdev, (count) & 0xff, dma_base + 0x04);
  OUTB (devc->osdev, (count >> 8) & 0xff, dma_base + 0x05);
  OUTB (devc->osdev, (count >> 16) & 0xff, dma_base + 0x06);
  OUTB (devc->osdev, 0x18, dma_base + 0x0b);
  OUTB (devc->osdev, 0x00, dma_base + 0x0f);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
static void
vibes_audio_start_input (int dev, oss_native_word buf, int count,
			 int fragsize, int intrflag)
{
  vibes_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;
  int c = (fragsize >> 1) - 1;
  int dma_base = devc->dmac_base;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  vibes_mixer_write (devc, 0x1c, (c >> 8) & 0xff);
  vibes_mixer_write (devc, 0x1d, c & 0xff);

  /* Setup DMA_C registers */
  count = count / 2 - 1;	/* Word count */
  OUTB (devc->osdev, 0xff, dma_base + 0x0d);
  OUTB (devc->osdev, (buf) & 0xff, dma_base + 0x00);
  OUTB (devc->osdev, (buf >> 8) & 0xff, dma_base + 0x01);
  OUTB (devc->osdev, (buf >> 16) & 0xff, dma_base + 0x02);
  OUTB (devc->osdev, (buf >> 24) & 0xff, dma_base + 0x03);
  OUTB (devc->osdev, (count) & 0xff, dma_base + 0x04);
  OUTB (devc->osdev, (count >> 8) & 0xff, dma_base + 0x05);
  OUTB (devc->osdev, (count >> 16) & 0xff, dma_base + 0x06);
  OUTB (devc->osdev, 0x14, dma_base + 0x0b);
  OUTB (devc->osdev, 0x00, dma_base + 0x0f);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
static void
vibes_audio_trigger (int dev, int state)
{
  vibes_portc *portc = audio_engines[dev]->portc;
  vibes_devc *devc = audio_engines[dev]->devc;

  unsigned char tmp;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  tmp = vibes_mixer_read (devc, 0x13) & ~0x07;
  if (portc->open_mode & OPEN_WRITE)
    {
      tmp |= 0x01;
      portc->trigger_bits |= PCM_ENABLE_OUTPUT;
    }
  if (portc->open_mode & OPEN_READ)
    {
      tmp |= 0x02;
      portc->trigger_bits |= PCM_ENABLE_INPUT;
    }
  vibes_mixer_write (devc, 0x13, tmp);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static void
setup_output_rate (vibes_devc * devc, vibes_portc * portc)
{
  unsigned int p = portc->speed;

  p = (p * 65536) / 48000;
  p = (p - 1) & 0xffff;

  if (p <= 1)
    p = 1;
  if (p > 0xffff)
    p = 0xffff;

  vibes_mixer_write (devc, 0x1e, (p) & 0xff);
  vibes_mixer_write (devc, 0x1f, (p >> 8) & 0xff);
}

static void
setup_input_rate (vibes_devc * devc, vibes_portc * portc)
{
  unsigned char tmp;

  /* Use separate sampling rate for ADC and DAC */

  tmp = vibes_mixer_read (devc, 0x22) | 0x10;
  vibes_mixer_write (devc, 0x22, tmp);	/* Use alternate sampling rate */
  vibes_mixer_write (devc, 0x23, ((48000 / portc->speed) - 1) << 4);
}

/*ARGSUSED*/
static int
vibes_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  vibes_devc *devc = audio_engines[dev]->devc;
  vibes_portc *portc = audio_engines[dev]->portc;
  unsigned char tmp;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  OUTB (devc->osdev, 0x40, devc->enh_base + 0x04);	/* Set MCE */
  tmp = vibes_mixer_read (devc, 0x12) & ~0x30;
  if (portc->channels != 1)
    tmp |= 0x10;
  if (portc->bits != 8)
    tmp |= 0x20;
  vibes_mixer_write (devc, 0x12, tmp);
  setup_input_rate (devc, portc);
  OUTB (devc->osdev, 0x00, devc->enh_base + 0x04);	/* Clear MCE */
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

/*ARGSUSED*/
static int
vibes_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  vibes_devc *devc = audio_engines[dev]->devc;
  vibes_portc *portc = audio_engines[dev]->portc;
  unsigned char tmp;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  tmp = vibes_mixer_read (devc, 0x12) & ~0x03;
  if (portc->channels != 1)
    tmp |= 0x01;
  if (portc->bits != 8)
    tmp |= 0x02;

  OUTB (devc->osdev, 0x40, devc->enh_base + 0x04);	/* Set MCE */
  vibes_mixer_write (devc, 0x12, tmp);
  setup_output_rate (devc, portc);
  OUTB (devc->osdev, 0x00, devc->enh_base + 0x04);	/* Clear MCE */
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

#if 0
static int
vibes_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  vibes_devc *devc = audio_engines[dev]->devc;
  /* vibes_portc *portc = audio_engines[dev]->portc; */
  unsigned int tmp = 0;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  if (direction == DMODE_OUTPUT)
    {
      tmp = INB (devc->osdev, devc->dmaa_base + 0x04);
      tmp |= INB (devc->osdev, devc->dmaa_base + 0x05) << 8;
      tmp |= INB (devc->osdev, devc->dmaa_base + 0x06) << 16;

      tmp = dmap->bytes_in_use % tmp;
    }
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return tmp;
}
#endif

static audiodrv_t vibes_audio_driver = {
  vibes_audio_open,
  vibes_audio_close,
  vibes_audio_output_block,
  vibes_audio_start_input,
  vibes_audio_ioctl,
  vibes_audio_prepare_for_input,
  vibes_audio_prepare_for_output,
  vibes_audio_reset,
  NULL,
  NULL,
  vibes_halt_input,
  vibes_halt_output,
  vibes_audio_trigger,
  vibes_audio_set_rate,
  vibes_audio_set_format,
  vibes_audio_set_channels,
  NULL,
  NULL,
  NULL,				/* vibes_check_input, */
  NULL,				/* vibes_check_output, */
  NULL,				/*vibes_alloc_buffer, */
  NULL,				/* vibes_free_buffer, */
  NULL,
  NULL,
  NULL				/* vibes_get_buffer_pointer */
};

static int
init_vibes (vibes_devc * devc)
{
  int my_mixer, adev;
  int i;
  unsigned char tmp;
  vibes_portc *portc;


  /* Setup CM00 */
  tmp = INB (devc->osdev, devc->enh_base + 0x00);
  OUTB (devc->osdev, tmp | 0x01, devc->enh_base + 0x00);	/* Enhanced mode on */

  my_mixer = 0;
  if ((my_mixer = oss_install_mixer (OSS_MIXER_DRIVER_VERSION,
				     devc->osdev,
				     devc->osdev,
				     "S3 SonicVibes",
				     &vibes_mixer_driver,
				     sizeof (mixer_driver_t), devc)) < 0)
    {
      my_mixer = -1;
      return 0;
    }
  else
    {
      devc->my_mixer = my_mixer;
      devc->recdevs = 0;
      vibes_set_recmask (devc, SOUND_MASK_MIC);
      devc->levels = load_mixer_volumes ("S3VIBES", default_mixer_levels, 1);
      vibes_mixer_reset (devc);
    }

  for (i = 0; i < MAX_PORTC; i++)
    {
      int flags = ADEV_AUTOMODE | ADEV_DUPLEX;

      portc = &devc->portc[i];

      if (i > 0)
	flags |= ADEV_SHADOW;

      if ((adev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
					devc->osdev,
					devc->osdev,
					"S3 SonicVibes",
					&vibes_audio_driver,
					sizeof (audiodrv_t),
					flags,
					AFMT_U8 | AFMT_S16_LE, devc, -1)) < 0)
	{
	  return 0;
	}

      audio_engines[adev]->mixer_dev = devc->my_mixer;
      audio_engines[adev]->portc = portc;
      audio_engines[adev]->dmabuf_maxaddr = MEMLIMIT_ISA;
      portc->open_mode = 0;
      portc->channels = 1;
      portc->bits = 8;
      portc->audio_dev = adev;
      portc->open_mode = 0;
      portc->trigger_bits = 0;
      portc->audio_enabled = 0;
#ifdef CONFIG_OSS_VMIX
      if (i == 0)
	 vmix_attach_audiodev(devc->osdev, adev, -1, 0);
#endif
    }
  return 1;
}

int
oss_s3vibes_attach (oss_device_t * osdev)
{
  unsigned int dmaa_base, dmac_base;
  unsigned char pci_irq_line, pci_revision;
  unsigned short pci_command, vendor, device;
  unsigned int pci_ioaddr0, pci_ioaddr1, pci_ioaddr2, pci_ioaddr3,
    pci_ioaddr4;
  vibes_devc *devc;

  DDB (cmn_err (CE_WARN, "Entered S3 SonicVibes probe routine\n"));

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_0, &pci_ioaddr0);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_1, &pci_ioaddr1);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_2, &pci_ioaddr2);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_3, &pci_ioaddr3);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_4, &pci_ioaddr4);
  pci_read_config_dword (osdev, 0x40, &dmaa_base);
  pci_read_config_dword (osdev, 0x48, &dmac_base);

  if (vendor != S3_VENDOR_ID || device != S3_SONICVIBES)
    return 0;

  if (pci_ioaddr0 == 0)
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
  devc->irq = pci_irq_line;

  devc->sb_base = MAP_PCI_IOADDR (devc->osdev, 0, pci_ioaddr0);
  devc->enh_base = MAP_PCI_IOADDR (devc->osdev, 1, pci_ioaddr1);
  devc->fm_base = MAP_PCI_IOADDR (devc->osdev, 2, pci_ioaddr2);
  devc->mpu_base = MAP_PCI_IOADDR (devc->osdev, 3, pci_ioaddr3);
  devc->game_base = MAP_PCI_IOADDR (devc->osdev, 4, pci_ioaddr4);

  /* Remove I/O space marker in bit 0. */
  devc->sb_base &= ~3;
  devc->enh_base &= ~3;
  devc->fm_base &= ~3;
  devc->mpu_base &= ~3;
  devc->game_base &= ~3;

  DDB (cmn_err (CE_WARN, "SB compatibility base %04x\n", devc->sb_base));
  DDB (cmn_err (CE_WARN, "Enhanced base %04x\n", devc->enh_base));
  DDB (cmn_err (CE_WARN, "FM base %04x\n", devc->fm_base));
  DDB (cmn_err (CE_WARN, "MIDI base %04x\n", devc->mpu_base));
  DDB (cmn_err (CE_WARN, "Game port base %04x\n", devc->game_base));
  DDB (cmn_err (CE_WARN, "DMA_A port base %04x\n", dmaa_base));
  DDB (cmn_err (CE_WARN, "DMA_C port base %04x\n", dmac_base));

  dmaa_base = (dmaa_base & 0x000f) + devc->sb_base;
  dmac_base = (dmac_base & 0x000f) + devc->sb_base + 16;

  dmaa_base |= 0x09;		/* Enable. Allow 32 bit addressing */
  dmac_base |= 0x01;		/* Enable */

  pci_write_config_dword (osdev, 0x40, dmaa_base);
  pci_write_config_dword (osdev, 0x48, dmac_base);
  DDB (cmn_err (CE_WARN, "DMA_A port base relocated to %04x\n", dmaa_base));
  DDB (cmn_err (CE_WARN, "DMA_C port base relocated to %04x\n", dmac_base));

  devc->dmaa_base = dmaa_base & ~0x000f;
  devc->dmac_base = dmac_base & ~0x000f;

  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_IO;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);

  oss_register_device (osdev, "S3 Sonic Vibes");

  if (oss_register_interrupts (devc->osdev, 0, vibesintr, NULL) < 0)
    {
      cmn_err (CE_WARN, "Can't allocate IRQ%d\n", pci_irq_line);
      return 0;
    }

  return init_vibes (devc);	/* Detected */
}

int
oss_s3vibes_detach (oss_device_t * osdev)
{
  vibes_devc *devc = (vibes_devc *) osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  oss_unregister_interrupts (devc->osdev);

  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);
  UNMAP_PCI_IOADDR (devc->osdev, 0);
  UNMAP_PCI_IOADDR (devc->osdev, 1);
  UNMAP_PCI_IOADDR (devc->osdev, 2);
  UNMAP_PCI_IOADDR (devc->osdev, 3);
  UNMAP_PCI_IOADDR (devc->osdev, 4);

  oss_unregister_device (devc->osdev);
  return 1;

}
