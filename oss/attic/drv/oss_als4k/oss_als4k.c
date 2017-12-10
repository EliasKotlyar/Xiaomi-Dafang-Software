/*
 * Purpose: Driver for ALS ALS4000 PCI audio controller.
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

#include "oss_als4k_cfg.h"
#include "oss_pci.h"

#define ALS_VENDOR_ID	0x4005
#define ALS_4000	0x4000

#define DSP_RESET       (devc->base + 0x16)
#define DSP_READ        (devc->base + 0x1A)
#define DSP_WRITE       (devc->base + 0x1C)
#define DSP_COMMAND     (devc->base + 0x1C)
#define DSP_STATUS      (devc->base + 0x1C)
#define DSP_DATA_AVAIL  (devc->base + 0x1E)
#define DSP_DATA_AVL16  (devc->base + 0x1F)
#define MIXER_ADDR      (devc->base + 0x14)
#define MIXER_DATA      (devc->base + 0x15)

#define DSP_CMD_SPKON           0xD1
#define DSP_CMD_SPKOFF          0xD3

#define ALS_RECORDING_DEVICES (SOUND_MASK_LINE | SOUND_MASK_MIC | SOUND_MASK_CD)
#define ALS_MIXER_DEVICES (SOUND_MASK_SYNTH | SOUND_MASK_PCM | \
			     SOUND_MASK_LINE | SOUND_MASK_MIC | \
			     SOUND_MASK_BASS | SOUND_MASK_TREBLE | \
			     SOUND_MASK_IGAIN | SOUND_MASK_CD | \
			     SOUND_MASK_VOLUME)
#define ALS4000_RECORDING_DEVICES (ALS_RECORDING_DEVICES)
#define ALS4000_MIXER_DEVICES (ALS_MIXER_DEVICES|SOUND_MASK_LINE2|SOUND_MASK_SPEAKER|SOUND_MASK_RECLEV)

#define LEFT_CHN        0
#define RIGHT_CHN       1

#define VOC_VOL         0x04
#define MIC_VOL         0x0A
#define MIC_MIX         0x0A
#define RECORD_SRC      0x0C
#define IN_FILTER       0x0C
#define OUT_FILTER      0x0E
#define MASTER_VOL      0x22
#define FM_VOL          0x26
#define CD_VOL          0x28
#define LINE_VOL        0x2E
#define IRQ_NR          0x80
#define DMA_NR          0x81
#define IRQ_STAT        0x82
#define OPSW            0x3c

static int default_levels[32] = {
  0x5a5a,			/* Master Volume */
  0x4b4b,			/* Bass */
  0x4b4b,			/* Treble */
  0x4b4b,			/* FM */
  0x4b4b,			/* PCM */
  0x4b4b,			/* PC Speaker */
  0x4b4b,			/* Ext Line */
  0x2020,			/* Mic */
  0x4b4b,			/* CD */
  0x0000,			/* Recording monitor */
  0x4b4b,			/* SB PCM */
  0x4b4b,			/* Recording level */
  0x4b4b,			/* Input gain */
  0x4b4b,			/* Output gain */
  0x4040,			/* Line1 */
  0x4040,			/* Line2 */
  0x1515			/* Line3 */
};

#define MAX_PORTC 2

typedef struct als4000_portc
{
  int speed, bits, channels;
  int open_mode;
  int audio_enabled;
  int trigger_bits;
  int audiodev;
}
als4000_portc;

typedef struct als4000_devc
{
  oss_device_t *osdev;
  oss_native_word base, mpu_base;
  int sb_attached, mpu_attached, fm_attached;
  int irq, mpu_irq;
  int play_rate_lock, rec_rate_lock;

  char *chip_name;
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;

  /* Audio parameters */
  int open_mode;
  als4000_portc portc[MAX_PORTC];
  oss_native_word srcode;

  /* Mixer parameters */
  int mixer_dev;
  int *levels;
  int recmask;
}
als4000_devc;

extern int als4000_mpu_ioaddr;
extern int als4000_mpu_irq;

void
als4000_gcr_writel (als4000_devc * devc, unsigned char index,
		    oss_native_word data)
{
  OUTB (devc->osdev, index, devc->base + 0x0c);
  OUTL (devc->osdev, data, devc->base + 0x08);
}


oss_native_word
als4000_gcr_readl (als4000_devc * devc, unsigned char index)
{
  oss_native_word bufl;

  OUTB (devc->osdev, index, devc->base + 0x0c);
  bufl = INL (devc->osdev, devc->base + 0x08);
  return (bufl);
}

static void
als4000_write (als4000_devc * devc, unsigned char val)
{
  int i;

  for (i = 0; i < 100000; i++)
    if ((INB (devc->osdev, DSP_STATUS) & 0x80) == 0)
      {
	OUTB (devc->osdev, val, DSP_COMMAND);
	return;
      }

  cmn_err (CE_WARN, "Write Command(%x) timed out.\n", val);
  return;
}

static unsigned char
als4000_read (als4000_devc * devc)
{
  int i;

  for (i = 0; i < 100000; i++)
    if (INB (devc->osdev, DSP_DATA_AVAIL) & 0x80)
      {
	return INB (devc->osdev, DSP_READ);
      }

  cmn_err (CE_WARN, "Read Command timed out.\n");
  return 0xff;
}

static void
als4000_setmixer (als4000_devc * devc, unsigned char port,
		  unsigned char value)
{
  OUTB (devc->osdev, port, MIXER_ADDR);
  oss_udelay (20);
  OUTB (devc->osdev, value, MIXER_DATA);
  //oss_udelay (20);
}

static unsigned int
als4000_getmixer (als4000_devc * devc, unsigned char port)
{
  unsigned char val;

  OUTB (devc->osdev, port, MIXER_ADDR);
  oss_udelay (20);
  val = INB (devc->osdev, MIXER_DATA);
  //oss_udelay (20);
  return val;
}

struct mixer_def
{
  unsigned int regno:8;
  unsigned int bitoffs:4;
  unsigned int nbits:4;
};

typedef struct mixer_def mixer_tab[32][2];
typedef struct mixer_def mixer_ent;

#define MIX_ENT(name, reg_l, bit_l, len_l, reg_r, bit_r, len_r) \
        {{reg_l, bit_l, len_l}, {reg_r, bit_r, len_r}}

static mixer_tab als4000_mix = {
  MIX_ENT (SOUND_MIXER_VOLUME, 0x30, 7, 5, 0x31, 7, 5),
  MIX_ENT (SOUND_MIXER_BASS, 0x46, 7, 4, 0x47, 7, 4),
  MIX_ENT (SOUND_MIXER_TREBLE, 0x44, 7, 4, 0x45, 7, 4),
  MIX_ENT (SOUND_MIXER_SYNTH, 0x34, 7, 5, 0x35, 7, 5),
  MIX_ENT (SOUND_MIXER_PCM, 0x32, 7, 5, 0x33, 7, 5),
  MIX_ENT (SOUND_MIXER_SPEAKER, 0x3b, 7, 2, 0x00, 0, 0),
  MIX_ENT (SOUND_MIXER_LINE, 0x38, 7, 5, 0x39, 7, 5),
  MIX_ENT (SOUND_MIXER_MIC, 0x3a, 7, 5, 0x00, 0, 0),
  MIX_ENT (SOUND_MIXER_CD, 0x36, 7, 5, 0x37, 7, 5),
  MIX_ENT (SOUND_MIXER_IMIX, 0x3c, 0, 1, 0x00, 0, 0),
  MIX_ENT (SOUND_MIXER_ALTPCM, 0x00, 0, 0, 0x00, 0, 0),
  MIX_ENT (SOUND_MIXER_RECLEV, 0x3f, 7, 2, 0x40, 7, 2),
  MIX_ENT (SOUND_MIXER_IGAIN, 0x3f, 7, 2, 0x40, 7, 2),
  MIX_ENT (SOUND_MIXER_OGAIN, 0x41, 7, 2, 0x42, 7, 2)
};

/*ARGSUSED*/
static void
change_bits (als4000_devc * devc, unsigned char *regval, int dev, int chn,
	     int newval)
{
  unsigned char mask;
  int shift;

  mask = (1 << als4000_mix[dev][chn].nbits) - 1;
  newval = (int) ((newval * mask) + 50) / 100;	/* Scale */

  shift =
    als4000_mix[dev][chn].bitoffs - als4000_mix[dev][LEFT_CHN].nbits + 1;

  *regval &= ~(mask << shift);	/* Mask out previous value */
  *regval |= (newval & mask) << shift;	/* Set the new value */
}

static int set_recmask (als4000_devc * devc, int mask);

static int
als4000_mixer_set (als4000_devc * devc, int dev, int value)
{
  int left = value & 0x000000ff;
  int right = (value & 0x0000ff00) >> 8;

  int regoffs;
  unsigned char val;

  if (left > 100)
    left = 100;
  if (right > 100)
    right = 100;

  if (dev > 31)
    return OSS_EINVAL;

  if (!(ALS4000_MIXER_DEVICES & (1 << dev)))	/*
						 * Not supported
						 */
    return OSS_EINVAL;

  regoffs = als4000_mix[dev][LEFT_CHN].regno;

  if (regoffs == 0)
    return OSS_EINVAL;

  val = als4000_getmixer (devc, regoffs);
  change_bits (devc, &val, dev, LEFT_CHN, left);

  devc->levels[dev] = left | (left << 8);

  if (als4000_mix[dev][RIGHT_CHN].regno != regoffs)	/*
							 * Change register
							 */
    {
      als4000_setmixer (devc, regoffs, val);	/*
						 * Save the old one
						 */
      regoffs = als4000_mix[dev][RIGHT_CHN].regno;

      if (regoffs == 0)
	return left | (left << 8);	/*
					 * Just left channel present
					 */

      val = als4000_getmixer (devc, regoffs);	/*
						 * Read the new one
						 */
    }

  change_bits (devc, &val, dev, RIGHT_CHN, right);

  als4000_setmixer (devc, regoffs, val);

  devc->levels[dev] = left | (right << 8);
  return left | (right << 8);
}

/*ARGSUSED*/
static int
als4000_mixer_ioctl (int dev, int audiodev, unsigned int cmd, ioctl_arg arg)
{
  als4000_devc *devc = mixer_devs[dev]->devc;
  int val;

  if (((cmd >> 8) & 0xff) == 'M')
    {
      if (IOC_IS_OUTPUT (cmd))
	switch (cmd & 0xff)
	  {
	  case SOUND_MIXER_RECSRC:
	    val = *arg;
	    return *arg = set_recmask (devc, val);
	    break;

	  default:

	    val = *arg;
	    return *arg = als4000_mixer_set (devc, cmd & 0xff, val);
	  }
      else
	switch (cmd & 0xff)
	  {

	  case SOUND_MIXER_RECSRC:
	    return *arg = devc->recmask;
	    break;

	  case SOUND_MIXER_DEVMASK:
	    return *arg = ALS4000_MIXER_DEVICES;
	    break;

	  case SOUND_MIXER_STEREODEVS:
	    return *arg = ALS4000_MIXER_DEVICES &
	      ~(SOUND_MASK_MIC | SOUND_MASK_SPEAKER | SOUND_MASK_IMIX);
	    break;

	  case SOUND_MIXER_RECMASK:
	    return *arg = ALS4000_RECORDING_DEVICES;
	    break;

	  case SOUND_MIXER_CAPS:
	    return *arg = SOUND_CAP_EXCL_INPUT;
	    break;


	  default:
	    return *arg = devc->levels[cmd & 0x1f];
	  }
    }
  else
    return OSS_EINVAL;
}


static void
set_recsrc (als4000_devc * devc, int src)
{
  als4000_setmixer (devc, RECORD_SRC,
		    (als4000_getmixer (devc, RECORD_SRC) & ~7) | (src & 0x7));
}

static int
set_recmask (als4000_devc * devc, int mask)
{
  int devmask = mask & ALS4000_RECORDING_DEVICES;

  if (devmask != SOUND_MASK_MIC &&
      devmask != SOUND_MASK_LINE && devmask != SOUND_MASK_CD)
    {				/*
				 * More than one devices selected. Drop the *
				 * previous selection
				 */
      devmask &= ~devc->recmask;
    }

  if (devmask != SOUND_MASK_MIC &&
      devmask != SOUND_MASK_LINE && devmask != SOUND_MASK_CD)
    {				/*
				 * More than one devices selected. Default to
				 * * mic
				 */
      devmask = SOUND_MASK_MIC;
    }


  if (devmask ^ devc->recmask)	/*
				 * Input source changed
				 */
    {
      switch (devmask)
	{

	case SOUND_MASK_MIC:
	  set_recsrc (devc, 0);
	  break;

	case SOUND_MASK_LINE:
	  set_recsrc (devc, 6);
	  break;

	case SOUND_MASK_CD:
	  set_recsrc (devc, 2);
	  break;


	default:
	  set_recsrc (devc, 0);
	}
    }

  devc->recmask = devmask;
  return devc->recmask;
}

static void
als4000_mixer_reset (als4000_devc * devc)
{
  char name[32];
  int i;

  sprintf (name, "ALS4000");

  devc->levels = load_mixer_volumes (name, default_levels, 1);
  devc->recmask = 0;

  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
    als4000_mixer_set (devc, i, devc->levels[i]);

  set_recmask (devc, SOUND_MASK_MIC);
}


static mixer_driver_t als4000_mixer_driver = {
  als4000_mixer_ioctl
};

static int
als4000intr (oss_device_t * osdev)
{
  als4000_devc *devc = (als4000_devc *) osdev->devc;
  unsigned int status, i;
  unsigned char mxstat;
  int serviced = 0;


  status = INB (devc->osdev, devc->base + 0x0e);

  for (i = 0; i < MAX_PORTC; i++)
    {
      als4000_portc *portc = &devc->portc[i];

      if ((status & 0x80) && (portc->trigger_bits & PCM_ENABLE_OUTPUT))
	{
	  serviced = 1;
	  oss_audio_outputintr (portc->audiodev, 1);
	}
      if ((status & 0x40) && (portc->trigger_bits & PCM_ENABLE_INPUT))
	{
	  serviced = 1;
	  oss_audio_inputintr (portc->audiodev, 0);
	}
    }

  OUTB (devc->osdev, status, devc->base + 0x0e);	/* acknowledge interrupt */

#if 0
  if (status & 0x10)
    {
      serviced = 1;
      uart401intr (INT_HANDLER_CALL (devc->mpu_irq));
    }
#endif
  mxstat = als4000_getmixer (devc, 0x82);
  switch (mxstat)
    {
    case 0x1:
      INB (devc->osdev, devc->base + 0x1e);
      break;
    case 0x2:
      INB (devc->osdev, devc->base + 0x1f);
      break;
    case 0x4:
      INB (devc->osdev, devc->base + 0x30);
      break;
    case 0x20:
      INB (devc->osdev, devc->base + 0x16);
      break;
    }
  return serviced;
}

static int
als4000_audio_set_rate (int dev, int arg)
{
  als4000_portc *portc = audio_engines[dev]->portc;

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
als4000_audio_set_channels (int dev, short arg)
{
  als4000_portc *portc = audio_engines[dev]->portc;

  if ((arg != 1) && (arg != 2))
    return portc->channels;
  portc->channels = arg;

  return portc->channels;
}

static unsigned int
als4000_audio_set_format (int dev, unsigned int arg)
{
  als4000_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->bits;

  if (!(arg & (AFMT_U8 | AFMT_S16_LE)))
    return portc->bits;
  portc->bits = arg;

  return portc->bits;
}

static void
als4000_speed (als4000_devc * devc, int speed)
{
  if (!(devc->play_rate_lock | devc->rec_rate_lock))
    {
      als4000_write (devc, 0x41);
      als4000_write (devc, (speed >> 8) & 0xFF);
      als4000_write (devc, speed & 0xFF);
    }
}

/*ARGSUSED*/
static int
als4000_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static int
als4000_reset (als4000_devc * devc)
{
  int i;
  unsigned char byte;
  DDB (cmn_err (CE_WARN, "Entered als4000_reset()\n"));

  OUTB (devc->osdev, 1, DSP_RESET);
  oss_udelay (20);
  OUTB (devc->osdev, 0, DSP_RESET);

  byte = als4000_read (devc);
  for (i = 0; i < 10000; i++)
    if (byte != 0xAA)
      {
	DDB (cmn_err (CE_WARN, "No response to RESET\n"));
	return 0;		/* Sorry */
      }

  DDB (cmn_err (CE_WARN, "als4000_reset() OK\n"));
  return 1;
}

static void als4000_audio_trigger (int dev, int state);

static void
als4000_audio_reset (int dev)
{
  als4000_audio_trigger (dev, 0);
}

static void
als4000_audio_reset_input (int dev)
{
  als4000_portc *portc = audio_engines[dev]->portc;
  als4000_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
als4000_audio_reset_output (int dev)
{
  als4000_portc *portc = audio_engines[dev]->portc;
  als4000_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

/*ARGSUSED*/
static int
als4000_audio_open (int dev, int mode, int open_flags)
{
  als4000_portc *portc = audio_engines[dev]->portc;
  als4000_devc *devc = audio_engines[dev]->devc;
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
als4000_audio_close (int dev, int mode)
{
  als4000_portc *portc = audio_engines[dev]->portc;
  als4000_devc *devc = audio_engines[dev]->devc;

  als4000_audio_reset (dev);
  als4000_reset (devc);
  portc->open_mode = 0;
  devc->open_mode &= ~mode;
  portc->audio_enabled &= ~mode;
}

/*ARGSUSED*/
static void
als4000_audio_output_block (int dev, oss_native_word buf, int count,
			    int fragsize, int intrflag)
{
  als4000_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
}

/*ARGSUSED*/
static void
als4000_audio_start_input (int dev, oss_native_word buf, int count,
			   int fragsize, int intrflag)
{
  als4000_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
}

static void
als4000_audio_trigger (int dev, int state)
{
  als4000_portc *portc = audio_engines[dev]->portc;
  als4000_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode & OPEN_WRITE)
    {
      if (state & PCM_ENABLE_OUTPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      if (portc->bits == AFMT_U8)
		als4000_write (devc, 0xd6);	/*start 8bit dma */
	      else
		als4000_write (devc, 0xd4);	/*start 16bit dma */
	      devc->play_rate_lock = 1;
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
	      if (portc->bits == AFMT_U8)
		als4000_write (devc, 0xd0);	/*exit 8bit dma */
	      else
		als4000_write (devc, 0xd5);	/*exit 16bit dma */
	      devc->play_rate_lock = 0;
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
	      /* start recording */
	      als4000_setmixer (devc, 0xde,
				als4000_getmixer (devc, 0xde) | 0x80);
	      devc->rec_rate_lock = 1;
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
	      /* stop recording */
	      als4000_setmixer (devc, 0xde,
				als4000_getmixer (devc, 0xde) & ~0x80);
	      devc->rec_rate_lock = 0;
	    }
	}
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
static int
als4000_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  als4000_devc *devc = audio_engines[dev]->devc;
  als4000_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_in;
  oss_native_word flags;
  int cnt, bits;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  /* set speed */
  als4000_speed (devc, portc->speed);

  /* set mode */
  if (portc->bits == AFMT_S16_LE)
    bits = 0x10;		/*signed 16bit */
  else
    bits = 0x04;		/*unsigned 8bit */
  if (portc->channels == 2)
    bits |= 0x20;		/*stereo */
  als4000_setmixer (devc, 0xde, bits);

  /* set size and buffer address */
  als4000_gcr_writel (devc, 0xa2, dmap->dmabuf_phys);
  als4000_gcr_writel (devc, 0xa3, dmap->bytes_in_use - 1);

  /* set tranfersize */
  cnt = dmap->fragment_size;
  if (portc->bits == AFMT_S16_LE)
    cnt >>= 1;
  cnt--;

  als4000_setmixer (devc, 0xdc, cnt & 0xFF);
  als4000_setmixer (devc, 0xdd, (cnt >> 8) & 0xFF);

  portc->audio_enabled &= ~PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/
static int
als4000_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  als4000_devc *devc = audio_engines[dev]->devc;
  als4000_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_out;
  oss_native_word flags;
  int cnt;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  /* set speed */
  als4000_speed (devc, portc->speed);

  /* set mode */
  als4000_write (devc, (portc->bits == AFMT_S16_LE ? 0xb6 : 0xc6));
  als4000_write (devc, ((portc->channels == 2 ? 0x20 : 0) +
			(portc->bits == AFMT_S16_LE ? 0x10 : 0)));


  /* set size and buffer address */
  als4000_gcr_writel (devc, 0x91, dmap->dmabuf_phys);
  als4000_gcr_writel (devc, 0x92, (dmap->bytes_in_use - 1) | 0x00180000);

  /* set transfer size */
  cnt = dmap->fragment_size;
  if (portc->bits == AFMT_S16_LE)
    cnt >>= 1;
  cnt--;

  als4000_write (devc, cnt & 0xff);	/*length low */
  als4000_write (devc, (cnt >> 8));	/*length high */

  als4000_write (devc, 0xd1);	/* turn on speaker */

  portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

static int
als4000_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  als4000_devc *devc = audio_engines[dev]->devc;
  unsigned int ptr;

  if (direction == PCM_ENABLE_OUTPUT)
    ptr = als4000_gcr_readl (devc, 0xa0);
  else
    ptr = als4000_gcr_readl (devc, 0xa4);

  return (ptr - dmap->dmabuf_phys) % dmap->bytes_in_use;
}


static audiodrv_t als4000_audio_driver = {
  als4000_audio_open,
  als4000_audio_close,
  als4000_audio_output_block,
  als4000_audio_start_input,
  als4000_audio_ioctl,
  als4000_audio_prepare_for_input,
  als4000_audio_prepare_for_output,
  als4000_audio_reset,
  NULL,
  NULL,
  als4000_audio_reset_input,
  als4000_audio_reset_output,
  als4000_audio_trigger,
  als4000_audio_set_rate,
  als4000_audio_set_format,
  als4000_audio_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,				/* als4000_alloc_buffer, */
  NULL,				/* als4000_free_buffer, */
  NULL,
  NULL,
  als4000_get_buffer_pointer
};

#ifdef OBSOLETED_STUFF
/*
 * This device has "ISA style" MIDI and FM subsystems. Such devices don't
 * use PCI config space for the I/O ports and interrupts. Instead the driver
 * needs to allocate proper resources itself. This functionality is no longer
 * possible. For this reason the MIDI and FM parts are not accessible.
 */
static void
attach_fm (als4000_devc * devc)
{
  if (!opl3_detect (0x388, devc->osdev))
    return;
  opl3_init (0x388, devc->osdev);
  devc->fm_attached = 1;
}

static void
attach_mpu (als4000_devc * devc)
{
  struct address_info hw_config;

  hw_config.io_base = devc->mpu_base;
  hw_config.irq = devc->mpu_irq;
  hw_config.dma = -1;
  hw_config.dma2 = -1;
  hw_config.always_detect = 0;
  hw_config.name = "Avance Logic ALS4000 MPU";
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
unload_mpu (als4000_devc * devc)
{
  struct address_info hw_config;

  hw_config.io_base = devc->mpu_base;
  hw_config.irq = devc->mpu_irq;
  hw_config.dma = -1;
  hw_config.dma2 = -1;
  hw_config.always_detect = 0;
  hw_config.name = "Avance Logic ALS4000 MPU";
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

void
setup_als4000 (als4000_devc * devc)
{
  unsigned char byte, i;
  oss_native_word dwTemp;

  dwTemp = ((oss_native_word) 0x200 << 16) | (0x388);
  dwTemp |= 0x00010000;
  dwTemp |= 0x00000001;
  als4000_gcr_writel (devc, 0xa8, dwTemp);
  dwTemp = ((oss_native_word) devc->mpu_base << 16);
  dwTemp |= 0x00010000;
  als4000_gcr_writel (devc, 0xa9, dwTemp);

  byte = als4000_getmixer (devc, 0xc0);
  als4000_setmixer (devc, 0xc0, byte | 0x80);	/*disable M80/M81 write protect */
  als4000_setmixer (devc, 0x81, 0x01);
  als4000_setmixer (devc, 0xc0, (byte & 0x7F));	/*set M80/M81 write protect */

  dwTemp = als4000_gcr_readl (devc, 0x8c);
  als4000_gcr_writel (devc, 0x8c, dwTemp | 0x00028000);	/*enable INTA */

  for (i = 0x91; i < 0xa7; i++)
    als4000_gcr_writel (devc, i, 0x00);
  dwTemp = als4000_gcr_readl (devc, 0x99);
  als4000_gcr_writel (devc, 0x99, dwTemp | 0x100);
}


static int
init_als4000 (als4000_devc * devc)
{
  int my_mixer;
  int first_dev = 0;
  unsigned char byte;
  int i;
  int adev;

  devc->mpu_attached = devc->fm_attached = 0;

  setup_als4000 (devc);

#ifdef OBSOLETED_STUFF
  attach_fm (devc);
  if (devc->mpu_base > 0)
    attach_mpu (devc);
#endif

  /* check ESP */
  als4000_write (devc, 0xE4);
  als4000_write (devc, 0xaa);
  als4000_write (devc, 0xE8);
  byte = als4000_read (devc);
  if (byte != 0xaa)
    {
      DDB (cmn_err (CE_WARN, "ESP Not OK\n"));
      return 0;
    }

  /* reset the DSP */
  als4000_reset (devc);

  if ((my_mixer = oss_install_mixer (OSS_MIXER_DRIVER_VERSION,
				     devc->osdev,
				     devc->osdev,
				     "Avance Logic ALS4000",
				     &als4000_mixer_driver,
				     sizeof (mixer_driver_t), devc)) < 0)
    {
      return 0;
    }
  als4000_mixer_reset (devc);

  for (i = 0; i < MAX_PORTC; i++)
    {
      char tmp_name[100];
      als4000_portc *portc = &devc->portc[i];
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
					&als4000_audio_driver,
					sizeof (audiodrv_t),
					caps,
					AFMT_U8 | AFMT_S16_LE, devc, -1)) < 0)
	{
	  adev = -1;
	  return 0;
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
	  audio_engines[adev]->dmabuf_maxaddr = MEMLIMIT_ISA;
	  portc->open_mode = 0;
	  portc->audiodev = adev;
	  portc->audio_enabled = 0;
#ifdef CONFIG_OSS_VMIX
	  if (i == 0)
	     vmix_attach_audiodev(devc->osdev, adev, -1, 0);
#endif
	}
    }
  return 1;
}

int
oss_als4k_attach (oss_device_t * osdev)
{

  unsigned char pci_irq_line, pci_revision;
  unsigned short pci_command, vendor, device;
  unsigned int pci_ioaddr;
  int err;
  als4000_devc *devc;

  DDB (cmn_err (CE_WARN, "Entered ALS ALS4000 probe routine\n"));

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_0, &pci_ioaddr);

  if (vendor != ALS_VENDOR_ID || device != ALS_4000)
    return 0;

  DDB (cmn_err (CE_WARN, "ALS4000 I/O base %04x\n", pci_ioaddr));

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
  devc->irq = pci_irq_line;
  devc->chip_name = "Avance Logic ALS4000";
  devc->mpu_base = als4000_mpu_ioaddr;
  devc->mpu_irq = als4000_mpu_irq;


  devc->base = MAP_PCI_IOADDR (devc->osdev, 0, pci_ioaddr);
  /* Remove I/O space marker in bit 0. */
  devc->base &= ~3;

  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_IO;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);

  oss_register_device (osdev, devc->chip_name);

  if ((err = oss_register_interrupts (devc->osdev, 0, als4000intr, NULL)) < 0)
    {
      cmn_err (CE_WARN, "Error installing interrupt handler: %x\n", err);
      return 0;
    }

  return init_als4000 (devc);	/* Detected */
}


int
oss_als4k_detach (oss_device_t * osdev)
{
  als4000_devc *devc = (als4000_devc *) osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  als4000_gcr_writel (devc, 0x8c, 0x0);

#ifdef OBSOLETED_STUFF
  if (devc->mpu_attached)
    unload_mpu (devc);
#endif

  oss_unregister_interrupts (devc->osdev);

  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);
  UNMAP_PCI_IOADDR (devc->osdev, 0);

  oss_unregister_device (devc->osdev);
  return 1;
}
