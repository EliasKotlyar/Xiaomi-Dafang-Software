/*
 * Purpose: Driver for ESS Solo PCI audio controller.
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

#include "oss_solo_cfg.h"
#include "oss_pci.h"

#define ESS_VENDOR_ID	0x125d
#define ESS_SOLO1	0x1969

#define DSP_RESET	(devc->sb_base + 0x6)
#define DSP_READ	(devc->sb_base + 0xA)
#define DSP_WRITE	(devc->sb_base + 0xC)
#define DSP_WRSTATUS	(devc->sb_base + 0xC)
#define DSP_STATUS	(devc->sb_base + 0xE)
#define DSP_STATUS16	(devc->sb_base + 0xF)
#define MIXER_ADDR	(devc->sb_base + 0x4)
#define MIXER_DATA	(devc->sb_base + 0x5)
#define OPL3_LEFT	(devc->sb_base + 0x0)
#define OPL3_RIGHT	(devc->sb_base + 0x2)
#define OPL3_BOTH	(devc->sb_base + 0x8)

#define DSP_CMD_SPKON		0xD1
#define DSP_CMD_SPKOFF		0xD3

#define SBPRO_RECORDING_DEVICES	(SOUND_MASK_LINE | SOUND_MASK_MIC | SOUND_MASK_CD)
#define SBPRO_MIXER_DEVICES		(SOUND_MASK_SYNTH | SOUND_MASK_PCM | \
					 SOUND_MASK_LINE | SOUND_MASK_MIC | \
					 SOUND_MASK_CD | SOUND_MASK_VOLUME)
#define SOLO_RECORDING_DEVICES (SBPRO_RECORDING_DEVICES)
#define SOLO_MIXER_DEVICES (SBPRO_MIXER_DEVICES|SOUND_MASK_LINE2|SOUND_MASK_SPEAKER|SOUND_MASK_RECLEV|SOUND_MASK_LINE1)

#define LEFT_CHN	0
#define RIGHT_CHN	1

#define VOC_VOL		0x04
#define MIC_VOL		0x0A
#define MIC_MIX		0x0A
#define RECORD_SRC	0x0C
#define IN_FILTER	0x0C
#define OUT_FILTER	0x0E
#define MASTER_VOL	0x22
#define FM_VOL		0x26
#define CD_VOL		0x28
#define LINE_VOL	0x2E
#define IRQ_NR		0x80
#define DMA_NR		0x81
#define IRQ_STAT	0x82
#define OPSW		0x3c

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

typedef struct solo_portc
{
  int speed, bits, channels;
  int open_mode;
  int audiodev;
  int trigger_bits;
  int audio_enabled;
}
solo_portc;

typedef struct solo_devc
{
  oss_device_t *osdev;
  oss_native_word base, ddma_base, sb_base, mpu_base;

  int mpu_attached, fm_attached;
  int irq;
  char *chip_name;
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;
  oss_native_word last_capture_addr;
  /* Audio parameters */
  solo_portc portc[MAX_PORTC];

  /* Mixer parameters */
  int *levels;
  int recmask;
}
solo_devc;


static int
solo_command (solo_devc * devc, unsigned char val)
{
  int i;

  for (i = 0; i < 0x10000; i++)
    {
      if ((INB (devc->osdev, DSP_WRSTATUS) & 0x80) == 0)
	{
	  OUTB (devc->osdev, val, DSP_WRITE);
	  return 1;
	}
    }

  cmn_err (CE_WARN, "Command(%x) Timeout.\n", val);
  return 0;
}

static int
solo_get_byte (solo_devc * devc)
{
  int i;

  for (i=0; i < 0x10000; i++)
    if (INB (devc->osdev, DSP_STATUS) & 0x80)
      {
	return INB (devc->osdev, DSP_READ);
      }

  return 0xffff;
}

static int
ext_read (solo_devc * devc, unsigned char reg)
{

  if (!solo_command (devc, 0xc0))	/* Read register command */
    return OSS_EIO;

  if (!solo_command (devc, reg))
    return OSS_EIO;

  return solo_get_byte (devc);
}

static int
ext_write (solo_devc * devc, unsigned char reg, unsigned char data)
{
  if (!solo_command (devc, reg))
    return 0;
  return solo_command (devc, data);
}

static unsigned int
solo_getmixer (solo_devc * devc, unsigned int port)
{
  unsigned int val;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  OUTB (devc->osdev, (unsigned char) (port & 0xff), MIXER_ADDR);
  oss_udelay (50);
  val = INB (devc->osdev, MIXER_DATA);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return val;
}

static void
solo_setmixer (solo_devc * devc, unsigned int port, unsigned int value)
{
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  OUTB (devc->osdev, (unsigned char) (port & 0xff), MIXER_ADDR);
  oss_udelay (50);
  OUTB (devc->osdev, (unsigned char) (value & 0xff), MIXER_DATA);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
}

static int
solo_reset (solo_devc * devc)
{
  int loopc;

  DDB (cmn_err (CE_WARN, "Entered solo_reset()\n"));

  OUTB (devc->osdev, 3, DSP_RESET);	/* Reset FIFO too */
  INB (devc->osdev, DSP_RESET);	/* Reset FIFO too */
  OUTB (devc->osdev, 0, DSP_RESET);
  oss_udelay (10);

  for (loopc = 0; loopc < 0x10000; loopc++)
    if (INB (devc->osdev, DSP_STATUS) & 0x80)
      if (INB (devc->osdev, DSP_READ) != 0xAA)
        {
          DDB (cmn_err (CE_WARN, "No response to RESET\n"));
          return 0;			/* Sorry */
        }
  solo_command (devc, 0xc6);	/* Enable extended mode */

  ext_write (devc, 0xb9, 3);    /* Demand mode - set to reserve mode */
  solo_setmixer (devc, 0x71, 0x32); /* 4x sampling + DAC2 asynch */

  /* enable DMA/IRQ */
  ext_write (devc, 0xb1, (ext_read (devc, 0xb1) & 0x0F) | 0x50);
  ext_write (devc, 0xb2, (ext_read (devc, 0xb2) & 0x0F) | 0x50);

  DDB (cmn_err (CE_WARN, "solo_reset() OK\n"));
  return 1;
}

struct mixer_def
{
  unsigned int regno:8;
  unsigned int bitoffs:4;
  unsigned int nbits:4;
};

typedef struct mixer_def mixer_tab[32][2];
typedef struct mixer_def mixer_ent;

#define MIX_ENT(name, reg_l, bit_l, len_l, reg_r, bit_r, len_r)	\
	{{reg_l, bit_l, len_l}, {reg_r, bit_r, len_r}}

static mixer_tab solo_mix = {
  MIX_ENT (SOUND_MIXER_VOLUME, 0x32, 7, 4, 0x32, 3, 4),
  MIX_ENT (SOUND_MIXER_BASS, 0x00, 0, 0, 0x00, 0, 0),
  MIX_ENT (SOUND_MIXER_TREBLE, 0x00, 0, 0, 0x00, 0, 0),
  MIX_ENT (SOUND_MIXER_SYNTH, 0x36, 7, 4, 0x36, 3, 4),
  MIX_ENT (SOUND_MIXER_PCM, 0x7c, 7, 4, 0x7c, 3, 4),
  MIX_ENT (SOUND_MIXER_SPEAKER, 0x3c, 2, 3, 0x00, 0, 0),
  MIX_ENT (SOUND_MIXER_LINE, 0x3e, 7, 4, 0x3e, 3, 4),
  MIX_ENT (SOUND_MIXER_MIC, 0x1a, 7, 4, 0x1a, 3, 4),
  MIX_ENT (SOUND_MIXER_CD, 0x38, 7, 4, 0x38, 3, 4),
  MIX_ENT (SOUND_MIXER_IMIX, 0x00, 0, 0, 0x00, 0, 0),
  MIX_ENT (SOUND_MIXER_ALTPCM, 0x7c, 7, 4, 0x7c, 3, 4),
  MIX_ENT (SOUND_MIXER_RECLEV, 0xb4, 7, 4, 0xb4, 3, 4),
  MIX_ENT (SOUND_MIXER_IGAIN, 0x68, 7, 4, 0x68, 3, 4),
  MIX_ENT (SOUND_MIXER_OGAIN, 0x00, 0, 0, 0x00, 0, 0),
  MIX_ENT (SOUND_MIXER_LINE1, 0x6d, 7, 4, 0x6d, 3, 4),
  MIX_ENT (SOUND_MIXER_LINE2, 0x3a, 7, 4, 0x3a, 3, 4),
  MIX_ENT (SOUND_MIXER_LINE3, 0x00, 0, 0, 0x00, 0, 0)
};

/*ARGSUSED*/
static void
change_bits (solo_devc * devc, unsigned char *regval, int dev, int chn,
	     int newval)
{
  unsigned char mask;
  int shift;

  mask = (1 << solo_mix[dev][chn].nbits) - 1;
  newval = (int) ((newval * mask) + 50) / 100;	/* Scale */

  shift = solo_mix[dev][chn].bitoffs - solo_mix[dev][LEFT_CHN].nbits + 1;

  *regval &= ~(mask << shift);	/* Mask out previous value */
  *regval |= (newval & mask) << shift;	/* Set the new value */
}

#if 0
static int
ess_set_reclev (solo_devc * devc, int dev, int left, int right)
{
  unsigned char b;

  b = (((15 * right) / 100) << 4) | (((15 * left) / 100));

  ext_write (devc, 0xb4, b);	/* Change input volume control */
  devc->levels[dev] = left | (right << 8);
  return left | (right << 8);
}

static int
ess_set_altpcm (solo_devc * devc, int dev, int left, int right)
{
  unsigned char b;

  b = (((15 * right) / 100) << 4) | (((15 * left) / 100));
  solo_setmixer (devc, 0x7C, b);	/* Change dac2 volume control */
  devc->levels[dev] = left | (right << 8);
  return left | (right << 8);
}
#endif

static int set_recmask (solo_devc * devc, int mask);

static int
solo_mixer_set (solo_devc * devc, int dev, int value)
{
  int left = value & 0x000000ff;
  int right = (value & 0x0000ff00) >> 8;

  int regoffs;
  unsigned char val;

  if (left > 100)
    left = 100;
  if (right > 100)
    right = 100;

#if 0
  if (dev == SOUND_MIXER_RECLEV)
    return ess_set_reclev (devc, dev, left, right);
  if (dev == SOUND_MIXER_ALTPCM)
    return ess_set_altpcm (devc, dev, left, right);
#endif

  if (dev > 31)
    return OSS_EINVAL;

  if (!(SOLO_MIXER_DEVICES & (1 << dev)))	/*
						 * Not supported
						 */
    return OSS_EINVAL;

  regoffs = solo_mix[dev][LEFT_CHN].regno;

  if (regoffs == 0)
    return OSS_EINVAL;

  val = solo_getmixer (devc, regoffs);
  change_bits (devc, &val, dev, LEFT_CHN, left);

  devc->levels[dev] = left | (left << 8);

  if (solo_mix[dev][RIGHT_CHN].regno != regoffs)	/*
							 * Change register
							 */
    {
      solo_setmixer (devc, regoffs, val);	/*
						 * Save the old one
						 */
      regoffs = solo_mix[dev][RIGHT_CHN].regno;

      if (regoffs == 0)
	return left | (left << 8);	/*
					 * Just left channel present
					 */

      val = solo_getmixer (devc, regoffs);	/*
						 * Read the new one
						 */
    }

  change_bits (devc, &val, dev, RIGHT_CHN, right);

  solo_setmixer (devc, regoffs, val);

  devc->levels[dev] = left | (right << 8);
  return left | (right << 8);
}

/*ARGSUSED*/
static int
solo_mixer_ioctl (int dev, int audiodev, unsigned int cmd, ioctl_arg arg)
{
  solo_devc *devc = mixer_devs[dev]->devc;
  int val;

  if (cmd == SOUND_MIXER_PRIVATE1)
    {
      val = *arg;
      if (val != 0 && val != 1)
	return (OSS_EINVAL);

      if (val)
	{
	  cmn_err (CE_WARN, "turning on 26db mic boost\n");
	  ext_write (devc, 0xa9, ext_read (devc, 0xa9) | 0xC);
	}
      else
	{
	  cmn_err (CE_WARN, "turning off 26db mic boost\n");
	  ext_write (devc, 0xa9, ext_read (devc, 0xa9) & ~0x4);
	}
      return *arg = val;
    }


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
	    return *arg = solo_mixer_set (devc, cmd & 0xff, val);
	  }
      else
	switch (cmd & 0xff)
	  {

	  case SOUND_MIXER_RECSRC:
	    return *arg = devc->recmask;
	    break;

	  case SOUND_MIXER_DEVMASK:
	    return *arg = SOLO_MIXER_DEVICES;
	    break;

	  case SOUND_MIXER_STEREODEVS:
	    return *arg = SOLO_MIXER_DEVICES &
	      ~(SOUND_MASK_MIC | SOUND_MASK_SPEAKER | SOUND_MASK_IMIX);
	    break;

	  case SOUND_MIXER_RECMASK:
	    return *arg = SOLO_RECORDING_DEVICES;
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
set_recsrc (solo_devc * devc, int src)
{
  solo_setmixer (devc, RECORD_SRC,
		 (solo_getmixer (devc, RECORD_SRC) & ~7) | (src & 0x7));
}

static int
set_recmask (solo_devc * devc, int mask)
{
  int devmask = mask & SOLO_RECORDING_DEVICES;

  if (devmask != SOUND_MASK_MIC &&
      devmask != SOUND_MASK_LINE && devmask != SOUND_MASK_CD)
    {				/*
				 * More than one devices selected. 
				 * Drop the previous selection
				 */
      devmask &= ~devc->recmask;
    }

  if (devmask != SOUND_MASK_MIC &&
      devmask != SOUND_MASK_LINE && devmask != SOUND_MASK_CD)
    {				/*
				 * More than one devices selected.
				 * Default to mic
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
solo_mixer_reset (solo_devc * devc)
{
  char name[32];
  int i;

  sprintf (name, "SOLO");

  devc->levels = load_mixer_volumes (name, default_levels, 1);
  devc->recmask = 0;

  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
    solo_mixer_set (devc, i, devc->levels[i]);

  set_recmask (devc, SOUND_MASK_MIC);
}

static mixer_driver_t solo_mixer_driver = {
  solo_mixer_ioctl
};

static int
solointr (oss_device_t * osdev)
{
  solo_devc *devc = (solo_devc *) osdev->devc;
  int status;
  int serviced = 0;
  int i;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  status = INB (devc->osdev, devc->base + 7);

  for (i = 0; i < MAX_PORTC; i++)
    {
      if (status & 0x10)	/* ESS Native Mode */
	{
          int instat;
	  solo_portc *portc = &devc->portc[i];
	  serviced = 1;

	  instat = INB (devc->osdev, DSP_STATUS);	/* Ack the interrupt */
	  if (portc->trigger_bits & PCM_ENABLE_INPUT)
	    oss_audio_inputintr (portc->audiodev, 1);
	}

      if (status & 0x20)	/* ESS DAC2 Mode */
	{
	  solo_portc *portc = &devc->portc[i];
	  serviced = 1;

	  if (portc->trigger_bits & PCM_ENABLE_OUTPUT)
	    oss_audio_outputintr (portc->audiodev, 1);
	  solo_setmixer (devc, 0x7A, solo_getmixer (devc, 0x7A) & 0x47);
	}
    }

  if (status & 0x80)		/* MPU interrupt */
    {
      serviced = 1;
      /* uart401intr (INT_HANDLER_CALL (devc->irq)); *//* UART401 interrupt */
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return serviced;
}

static int
solo_audio_set_rate (int dev, int arg)
{
  solo_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->speed;

  if (arg > 48000)
    arg = 48000;
  if (arg < 8000)
    arg = 8000;
  portc->speed = arg;
  return portc->speed;
}

static short
solo_audio_set_channels (int dev, short arg)
{
  solo_portc *portc = audio_engines[dev]->portc;

  if ((arg != 1) && (arg != 2))
    return portc->channels;
  portc->channels = arg;

  return portc->channels;
}

static unsigned int
solo_audio_set_format (int dev, unsigned int arg)
{
  solo_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->bits;

  if (!(arg & (AFMT_U8 | AFMT_S16_LE)))
    return portc->bits;
  portc->bits = arg;

  return portc->bits;
}

/*ARGSUSED*/
static int
solo_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void solo_audio_trigger (int dev, int state);

static void
solo_audio_reset (int dev)
{
  solo_audio_trigger (dev, 0);
}

static void
solo_audio_reset_input (int dev)
{
  solo_portc *portc = audio_engines[dev]->portc;
  solo_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
solo_audio_reset_output (int dev)
{
  solo_portc *portc = audio_engines[dev]->portc;
  solo_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

/*ARGSUSED*/
static int
solo_audio_open (int dev, int mode, int open_flags)
{
  oss_native_word flags;
  solo_portc *portc = audio_engines[dev]->portc;
  solo_devc *devc = audio_engines[dev]->devc;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }
  portc->open_mode = mode;
  portc->audio_enabled = ~mode;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

static void
solo_audio_close (int dev, int mode)
{
  solo_portc *portc = audio_engines[dev]->portc;

  solo_audio_reset (dev);
  portc->open_mode = 0;
  portc->audio_enabled &= ~mode;
}

/*ARGSUSED*/
static void
solo_audio_output_block (int dev, oss_native_word buf, int count,
			 int fragsize, int intrflag)
{
  solo_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;

}

/*ARGSUSED*/
static void
solo_audio_start_input (int dev, oss_native_word buf, int count,
			int fragsize, int intrflag)
{
  solo_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
}

static void
solo_audio_trigger (int dev, int state)
{
  oss_native_word flags;
  solo_portc *portc = audio_engines[dev]->portc;
  solo_devc *devc = audio_engines[dev]->devc;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode & OPEN_WRITE)
    {
      if (state & PCM_ENABLE_OUTPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      solo_setmixer (devc, 0x78, 0x92);	/* stablilze fifos */
	      oss_udelay(100);
	      solo_setmixer (devc, 0x78, 0x93);	/* Go */
	      OUTB (devc->osdev, 0x0A, devc->base + 6);     /*unmask dac2 intr */
	      portc->trigger_bits |= PCM_ENABLE_OUTPUT;
	    }
	}
      else
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      (portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
	      portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
	      solo_setmixer (devc, 0x78, 0);	/* stop the audio dac2 dma */
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
	      ext_write (devc, 0xb8, 0x0f);	/* Go */
	      OUTB (devc->osdev, 0x00, devc->ddma_base + 0x0f); /*start dma*/
	      portc->trigger_bits |= PCM_ENABLE_INPUT;
	    }
	}
      else
	{
	  if ((portc->audio_enabled & PCM_ENABLE_INPUT) &&
	      (portc->trigger_bits & PCM_ENABLE_INPUT))
	    {
	      portc->trigger_bits &= ~PCM_ENABLE_INPUT;
	      portc->audio_enabled &= ~PCM_ENABLE_INPUT;
	      ext_write (devc, 0xb8, 0x00);	/* stop */
	    }
	}
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static void
ext_speed (int dev, int direction)
{
  solo_devc *devc = audio_engines[dev]->devc;
  solo_portc *portc = audio_engines[dev]->portc;
  int divider, div, filter;
  unsigned int rate;
  int speed, s0, s1, use0;
  int dif0, dif1;
  unsigned char t0, t1;

  /* rate = source / (256 - divisor) */
  /* divisor = 256 - (source / rate) */
  speed = portc->speed;

  t0 = 128 - (793800 / speed);
  s0 = 793800 / (128 - t0);

  t1 = 128 - (768000 / speed);
  s1 = 768000 / (128 - t1);
  t1 |= 0x80;

  dif0 = speed - s0;
  if (dif0 < 0)
    dif0 *= -1;
  dif1 = speed - s1;
  if (dif1 < 0)
    dif1 *= -1;

  use0 = (dif0 < dif1) ? 1 : 0;

  if (use0)
    {
      rate = s0;
      div = t0;
    }
  else
    {
      rate = s1;
      div = t1;
    }
  portc->speed = rate;
/*
 * Set filter divider register
 */
  filter = (rate * 8 * 82) / 20;	/* 80% of the rate */
  divider = 256 - 7160000 / (filter);
  if (direction)
    {
      ext_write (devc, 0xa1, div);
      ext_write (devc, 0xa2, divider);
    }
  else
    {
      solo_setmixer (devc, 0x70, div);
      solo_setmixer (devc, 0x72, divider);
    }
}

/*ARGSUSED*/
static int
solo_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  solo_devc *devc = audio_engines[dev]->devc;
  solo_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;
  unsigned int left, right, reclev;
  dmap_t *dmap = audio_engines[dev]->dmap_in;
  int c;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  OUTB (devc->osdev, 2, DSP_RESET);     /* Reset FIFO too */
  INB (devc->osdev, DSP_RESET); /* Reset FIFO too */
  OUTB (devc->osdev, 0, DSP_RESET);
  oss_udelay (10);


  ext_speed (dev, 1);

  ext_write (devc, 0xa8, (ext_read (devc, 0xa8) & ~0x3) | (3 - portc->channels));	/* Mono/stereo */

  solo_command (devc, DSP_CMD_SPKOFF);

  if (portc->channels == 1)
    {
      if (portc->bits == AFMT_U8)
        {                       /* 8 bit mono */
          ext_write (devc, 0xb7, 0x51);
          ext_write (devc, 0xb7, 0xd0);
        }
      else
        {                       /* 16 bit mono */
          ext_write (devc, 0xb7, 0x71);
          ext_write (devc, 0xb7, 0xf4);
        }
    }
  else
    {                           /* Stereo */
      if (portc->bits == AFMT_U8)
        {                       /* 8 bit stereo */
          ext_write (devc, 0xb7, 0x51);
          ext_write (devc, 0xb7, 0x98);
        }
      else
        {                       /* 16 bit stereo */
          ext_write (devc, 0xb7, 0x71);
          ext_write (devc, 0xb7, 0xbc);
        }
    }

  /* 
   * reset the 0xb4 register to the stored value of RECLEV - for some 
   * reason it gets reset when you enter ESS Extension mode.
   */
  left = devc->levels[SOUND_MIXER_RECLEV] & 0xff;
  right = (devc->levels[SOUND_MIXER_RECLEV] >> 8) & 0xff;
  reclev = (((15 * right) / 100) << 4) | (((15 * left) / 100));
  ext_write (devc, 0xb4, reclev);

  OUTB (devc->osdev, 0xc4, devc->ddma_base + 0x08);
  OUTB (devc->osdev, 0xff, devc->ddma_base + 0x0d);   /* clear DMA */
  OUTB (devc->osdev, 0x01, devc->ddma_base + 0x0f);  /* stop DMA */
  OUTB (devc->osdev, 0x14, devc->ddma_base + 0x0b);  /*Demand/Single Mode */

  OUTL (devc->osdev, dmap->dmabuf_phys, devc->ddma_base + 0x00);
  OUTW (devc->osdev, dmap->bytes_in_use, devc->ddma_base + 0x04);

  c = -(dmap->fragment_size);
  /* Reload DMA Count */
  ext_write (devc, 0xa4, (unsigned char) ((unsigned short) c & 0xff));
  ext_write (devc, 0xa5, (unsigned char) (((unsigned short) c >> 8) & 0xff));

  portc->audio_enabled &= ~PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/
static int
solo_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  solo_devc *devc = audio_engines[dev]->devc;
  solo_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;
  dmap_t *dmap = audio_engines[dev]->dmap_out;
  int c;


  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  OUTB (devc->osdev, 2, DSP_RESET);     /* Reset FIFO too */
  INB (devc->osdev, DSP_RESET); 
  OUTB (devc->osdev, 0, DSP_RESET);
  ext_speed (dev, 0);
  if (portc->channels == 1)
    {
      if (portc->bits == AFMT_U8)
	solo_setmixer (devc, 0x7A, 0x40 | 0x00);	/*8bit mono unsigned */
      else
	solo_setmixer (devc, 0x7A, 0x40 | 0x05);	/*16bit mono signed */
    }
  else
    {
      if (portc->bits == AFMT_U8)
	solo_setmixer (devc, 0x7A, 0x40 | 0x02);	/*8bit stereo unsigned */
      else
	solo_setmixer (devc, 0x7A, 0x40 | 0x07);	/*16bit stereo signed */
    }

  OUTL (devc->osdev, dmap->dmabuf_phys, devc->base + 0);
  OUTW (devc->osdev, dmap->bytes_in_use, devc->base + 4);

  OUTB (devc->osdev, 0x0, devc->base + 6);	/* disable the DMA mask */

  c = -(dmap->fragment_size>>1);
  solo_setmixer (devc, 0x74, (unsigned char) ((unsigned short) c & 0xff));
  solo_setmixer (devc, 0x76, (unsigned char) (((unsigned short) c >> 8) & 0xff));
  portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

static int
solo_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  solo_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;
  oss_native_word ptr=0;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  if (direction == PCM_ENABLE_OUTPUT)
  {
      ptr = dmap->bytes_in_use - INW(devc->osdev, devc->base + 4);
  }

  if (direction == PCM_ENABLE_INPUT)
  {
      int count;
      unsigned int diff;

      ptr = INL(devc->osdev, devc->ddma_base + 0x00);
      count = INL(devc->osdev, devc->ddma_base + 0x04);

      diff = dmap->dmabuf_phys + dmap->bytes_in_use - ptr - count;

      if (ptr < dmap->dmabuf_phys || 
          ptr >= dmap->dmabuf_phys + dmap->bytes_in_use)

           ptr = devc->last_capture_addr;            /* use prev value */
      else
      	   devc->last_capture_addr = ptr;            /* save it */

      ptr -= dmap->dmabuf_phys;
  }

  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return ptr;
}


static audiodrv_t solo_audio_driver = {
  solo_audio_open,
  solo_audio_close,
  solo_audio_output_block,
  solo_audio_start_input,
  solo_audio_ioctl,
  solo_audio_prepare_for_input,
  solo_audio_prepare_for_output,
  solo_audio_reset,
  NULL,
  NULL,
  solo_audio_reset_input,
  solo_audio_reset_output,
  solo_audio_trigger,
  solo_audio_set_rate,
  solo_audio_set_format,
  solo_audio_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,				/* solo_alloc_buffer, */
  NULL,				/* solo_free_buffer, */
  NULL,
  NULL,
  solo_get_buffer_pointer 
};

static int
init_solo (solo_devc * devc)
{
  int my_mixer;
  int i, adev;
  int first_dev = 0;

  devc->mpu_attached = devc->fm_attached = 0;

/*
 * Initialize and attach the legacy devices
 */

  if (!solo_reset (devc))
    {
      cmn_err (CE_WARN, "Reset command failed\n");
      return 0;
    }

/* setup mixer regs */
  solo_setmixer (devc, 0x7d, 0x0c);
  OUTB (devc->osdev, 0xf0, devc->base + 7); 
  OUTB (devc->osdev, 0x00, devc->ddma_base + 0x0f);

  if ((my_mixer = oss_install_mixer (OSS_MIXER_DRIVER_VERSION,
				     devc->osdev,
				     devc->osdev,
				     "ESS Solo",
				     &solo_mixer_driver,
				     sizeof (mixer_driver_t), devc)) < 0)
    {
      return 0;
    }
  solo_mixer_reset (devc);

  for (i = 0; i < MAX_PORTC; i++)
    {
      char tmp_name[100];
      solo_portc *portc = &devc->portc[i];
      int caps = ADEV_AUTOMODE;
      strcpy (tmp_name, devc->chip_name);

      if (i == 0)
	{
	  strcpy (tmp_name, devc->chip_name);
	  caps |= ADEV_DUPLEX;
	}
      else
	{
	  caps |= ADEV_DUPLEX | ADEV_SHADOW;
	}

      if ((adev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
					devc->osdev,
					devc->osdev,
					tmp_name,
					&solo_audio_driver,
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
	  audio_engines[adev]->min_rate = 8000;
	  audio_engines[adev]->max_rate = 48000;
	  audio_engines[adev]->dmabuf_maxaddr = MEMLIMIT_ISA;
	  audio_engines[adev]->dmabuf_alloc_flags |= DMABUF_SIZE_16BITS;
	  audio_engines[adev]->caps |= PCM_CAP_FREERATE;
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
oss_solo_attach (oss_device_t * osdev)
{

  unsigned char pci_irq_line, pci_revision;
  unsigned short pci_command, vendor, device;
  unsigned int pci_ioaddr0, pci_ioaddr1, pci_ioaddr2, pci_ioaddr3;
  solo_devc *devc;

  DDB (cmn_err (CE_WARN, "Entered ESS Solo probe routine\n"));

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  if (vendor != ESS_VENDOR_ID || device != ESS_SOLO1)
    return 0;

  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_0, &pci_ioaddr0);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_1, &pci_ioaddr1);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_2, &pci_ioaddr2);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_3, &pci_ioaddr3);

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
  devc->chip_name = "ESS Solo-1";

  devc->base = MAP_PCI_IOADDR (devc->osdev, 0, pci_ioaddr0);
  /* Remove I/O space marker in bit 0. */
  devc->base &= ~0x3;

  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_IO;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);

  oss_register_device (osdev, devc->chip_name);

  if (oss_register_interrupts (devc->osdev, 0, solointr, NULL) < 0)
    {
      cmn_err (CE_WARN, "Can't allocate IRQ%d\n", pci_irq_line);
      return 0;
    }


  /* Read the VCBase register */
  if (pci_ioaddr2 == 0)
    {
      cmn_err (CE_WARN, "DMA I/O base not set\n");
      /*return 0; */
    }
  /* Copy it's contents to the DDMA register */
  pci_write_config_dword (osdev, 0x60, pci_ioaddr2 | 0x1); /* enabled DDMA */
  devc->ddma_base = MAP_PCI_IOADDR (devc->osdev, 2, pci_ioaddr2);
  devc->ddma_base &= ~0x3;

  /* Init other SB base registers */
  if (pci_ioaddr1 == 0)
    {
      cmn_err (CE_WARN, "SB I/O base not set\n");
      return 0;
    }
  devc->sb_base = MAP_PCI_IOADDR (devc->osdev, 1, pci_ioaddr1);
  devc->sb_base &= ~0x3;


  /* Init MPU base register */
  if (pci_ioaddr3 == 0)
    {
      cmn_err (CE_WARN, "MPU I/O base not set\n");
      return 0;
    }
  devc->mpu_base = MAP_PCI_IOADDR (devc->osdev, 3, pci_ioaddr3);
  devc->mpu_base &= ~0x3;

  /* Setup Legacy audio register - disable legacy audio */
  pci_write_config_word (osdev, 0x40, 0x805f);

  /* Select DDMA and Disable IRQ emulation */
  pci_write_config_dword (osdev, 0x50, 0);
  pci_read_config_dword (osdev, 0x50, &pci_ioaddr0);
  pci_ioaddr0 &= (~(0x0700 | 0x6000));
  pci_write_config_dword (osdev, 0x50, pci_ioaddr0);

  return init_solo (devc);	/* Detected */
}


int
oss_solo_detach (oss_device_t * osdev)
{
  solo_devc *devc = (solo_devc *) osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  /* disable all interrupts */
  /*OUTB (devc->osdev, 0, devc->base + 7); */

#ifdef OBSOLETED_STUFF
  if (devc->mpu_attached)
    unload_mpu (devc);
#endif

  oss_unregister_interrupts (devc->osdev);

  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);
  UNMAP_PCI_IOADDR (devc->osdev, 0);
  UNMAP_PCI_IOADDR (devc->osdev, 1);
  UNMAP_PCI_IOADDR (devc->osdev, 2);
  UNMAP_PCI_IOADDR (devc->osdev, 3);

  oss_unregister_device (devc->osdev);
  return 1;
}
