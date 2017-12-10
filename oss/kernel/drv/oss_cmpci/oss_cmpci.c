/*
 * Purpose: Driver for CMEDIA CM8738 PCI audio controller.
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

#include "oss_cmpci_cfg.h"
#include "oss_pci.h"
#include "uart401.h"

#define CMEDIA_VENDOR_ID	0x13F6
#define CMEDIA_CM8738		0x0111
#define CMEDIA_CM8338A		0x0100
#define CMEDIA_CM8338B		0x0101
/*
 * CM8338 registers definition
 */

#define FUNCTRL0      (devc->base+0x00)
#define FUNCTRL1      (devc->base+0x04)
#define CHFORMAT      (devc->base+0x08)
#define INT_HLDCLR    (devc->base+0x0C)
#define INT_STATUS    (devc->base+0x10)
#define LEGACY_CTRL   (devc->base+0x14)
#define MISC_CTRL     (devc->base+0x18)
#define TDMA_POS      (devc->base+0x1C)
#define MIXER         (devc->base+0x20)
#define MIXER_DATA    (devc->base+0x22)
#define MIXER_ADDR    (devc->base+0x23)
#define MIXER1        (devc->base+0x24)
#define MIXER2        (devc->base+0x25)
#define AUX_VOL       (devc->base+0x26)
#define MIXER3        (devc->base+0x27)
#define AC97          (devc->base+0x28)

#define CH0_FRAME1    (devc->base+0x80)
#define CH0_FRAME2    (devc->base+0x84)
#define CH1_FRAME1    (devc->base+0x88)
#define CH1_FRAME2    (devc->base+0x8C)

#define SPDIF_STAT    (devc->base+0x90)
#define MISC2_CTRL    (devc->base+0x92)

#define MPU_MIRROR    (devc->base+0x40)
#define FM_MIRROR     (devc->base+0x50)
#define JOY_MIRROR    (devc->base+0x60)

#define DSP_MIX_DATARESETIDX    (0x00)
#define DSP_MIX_OUTMIXIDX	(0x3c)
#define CM_CH0_ENABLE     0x01
#define CM_CH1_ENABLE     0x02
#define CM_CH0_RESET      0x04
#define CM_CH1_RESET      0x08
#define CM_CH0_RECORD	  0x01
#define CM_CH1_RECORD	  0x02
#define CM_CH0_PLAY	  ~0x01
#define CM_CH1_PLAY	  ~0x02
#define CM_CH0_INT        1
#define CM_CH1_INT        2
#define CM_EXTENT_CODEC   0x100
#define CM_EXTENT_MIDI    0x2
#define CM_EXTENT_SYNTH   0x4
#define CM_CFMT_STEREO    0x01
#define CM_CFMT_16BIT     0x02
#define CM_CFMT_MASK      0x03
#define CM_CFMT_DACSHIFT  0
#define CM_CFMT_ADCSHIFT  2

#define MUTE_LINE	1
#define MUTE_CD		2
#define MUTE_MIC	3
#define MUTE_AUX	4
#define REAR2LINE	5
#define CEN2LINE	6
#define BASS2LINE	7
#define CEN2MIC		8
#define MODE_4SPK	9
#define DUALDAC		10
#define MICBOOST	11
#define SPDIF_PLAY	12
#define SPDIF_LOOP	13
#define SPDIF_REC	14
#define SPDIF_IMON	15
#define SPDIF_POL	16
#define SPDIF_COPY	17
#define SPDIF_OPT	18
#define SPDIF_AC3	19
#define AUX_REC		20
#define AUX_LEVEL	21

#define CHAN0		   0x1
#define CHAN1		   0x2

static struct
{
  unsigned int rate;
  unsigned int lower;
  unsigned int upper;
  unsigned char freq;
}
rate_lookup[] =
{
  {
  5512, (0 + 5512) / 2, (5512 + 8000) / 2, 0}
  ,
  {
  8000, (5512 + 8000) / 2, (8000 + 11025) / 2, 4}
  ,
  {
  11025, (8000 + 11025) / 2, (11025 + 16000) / 2, 1}
  ,
  {
  16000, (11025 + 16000) / 2, (16000 + 22050) / 2, 5}
  ,
  {
  22050, (16000 + 22050) / 2, (22050 + 32000) / 2, 2}
  ,
  {
  32000, (22050 + 32000) / 2, (32000 + 44100) / 2, 6}
  ,
  {
  44100, (32000 + 44100) / 2, (44100 + 48000) / 2, 3}
  ,
  {
  48000, (48000 + 44100) / 2, (48000 + 96000) / 2, 7}
};

static unsigned char cmpci_recmasks_L[SOUND_MIXER_NRDEVICES] = {
  0x00,				/* SOUND_MIXER_VOLUME   */
  0x00,				/* SOUND_MIXER_BASS     */
  0x00,				/* SOUND_MIXER_TREBLE   */
  0x40,				/* SOUND_MIXER_SYNTH    */
  0x00,				/* SOUND_MIXER_PCM      */
  0x00,				/* SOUND_MIXER_SPEAKER  */
  0x10,				/* SOUND_MIXER_LINE     */
  0x01,				/* SOUND_MIXER_MIC      */
  0x04,				/* SOUND_MIXER_CD       */
  0x00,				/* SOUND_MIXER_IMIX     */
  0x00,				/* SOUND_MIXER_LINE1    */
  0x00,				/* SOUND_MIXER_RECLEV   */
  0x00,				/* SOUND_MIXER_IGAIN    */
  0x00				/* SOUND_MIXER_OGAIN    */
};

static unsigned char cmpci_recmasks_R[SOUND_MIXER_NRDEVICES] = {
  0x00,				/* SOUND_MIXER_VOLUME   */
  0x00,				/* SOUND_MIXER_BASS     */
  0x00,				/* SOUND_MIXER_TREBLE   */
  0x20,				/* SOUND_MIXER_SYNTH    */
  0x00,				/* SOUND_MIXER_PCM      */
  0x00,				/* SOUND_MIXER_SPEAKER  */
  0x08,				/* SOUND_MIXER_LINE     */
  0x01,				/* SOUND_MIXER_MIC      */
  0x02,				/* SOUND_MIXER_CD       */
  0x00,				/* SOUND_MIXER_IMIX     */
  0x00,				/* SOUND_MIXER_LINE1    */
  0x00,				/* SOUND_MIXER_RECLEV   */
  0x00,				/* SOUND_MIXER_IGAIN    */
  0x00				/* SOUND_MIXER_OGAIN    */
};

#define CMEDIA_RECORDING_DEVICES (SOUND_MASK_SYNTH | SOUND_MASK_LINE | \
				  SOUND_MASK_MIC | SOUND_MASK_CD | SOUND_MASK_LINE1)

#define CMEDIA_MIXER_DEVICES (SOUND_MASK_SYNTH | SOUND_MASK_PCM | \
			     SOUND_MASK_LINE | SOUND_MASK_MIC | \
			     SOUND_MASK_IGAIN | SOUND_MASK_CD | \
			     SOUND_MASK_VOLUME | SOUND_MASK_SPEAKER |\
			     SOUND_MASK_LINE1|SOUND_MASK_RECLEV)

#define LEFT_CHN        0
#define RIGHT_CHN       1
/*
 * Mixer registers of CMPCI
 */
#define CMPCI_IMASK_L    0x3d
#define CMPCI_IMASK_R    0x3e


int default_levels[32] = {
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

typedef struct cmpci_portc
{
  int speed, bits, channels;
  int open_mode;
  int trigger_bits;
  int audio_enabled;
  int audiodev;
  int dacfmt, adcfmt;
  int chan0_play, chan0_rec;
  int chan1_play, chan1_rec;
}
cmpci_portc;

#define MAX_PORTC 2

typedef struct cmpci_devc
{
  oss_device_t *osdev;
  oss_native_word base;
  int fm_attached;
  int irq;
  int max_channels;
  volatile unsigned char intr_mask;
  int model;
#define MDL_CM8738		1
#define MDL_CM8338A		2
#define MDL_CM8338B		3
#define MDL_CM8768		4
  char *chip_name;
  int chiprev;
  int mode_4spk;
  int dev_mode;
#define DEFAULT_MODE		1
#define DUALDAC_MODE		2
#define SPDIFIN_MODE		4
  unsigned char spdif_control_bits[24];
  /* Audio parameters */
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;
  int open_mode;
  int audio_initialized;
  cmpci_portc portc[MAX_PORTC];

  /* spdif/ac3 stuff */
  int spdif_enabled;
  int can_ac3;

  /* Mixer parameters */
  int mixer_dev;
  int *levels;
  int recmask;

  /* uart401 */
  int uart401_attached;
  uart401_devc uart401devc;
}
cmpci_devc;


static void
set_spdif_rate (cmpci_devc * devc, cmpci_portc * portc)
{
  if (portc->speed == 48000)
    {
      /* setup MISC control reg */
      OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) | (1 << 24) | (1 << 15),
	    MISC_CTRL);
    }
  else
    {
      OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) & ~(1 << 15),
	    MISC_CTRL);
      OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) & ~(1 << 24),
	    MISC_CTRL);
    }
}

static void
setup_ac3 (cmpci_devc * devc, cmpci_portc * portc, int value)
{
  if (value && (portc->speed == 48000 || portc->speed == 44100))
    {
      if (devc->chiprev == 37)
	{
	  OUTL (devc->osdev, INL (devc->osdev, CHFORMAT) | (1 << 20),
		CHFORMAT);
	}
      else
	{
	  /* Enable AC3 */
	  OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) | (1 << 18),
		MISC_CTRL);
	}
    }
  else
    {
      if (devc->chiprev == 37)
	{
	  OUTL (devc->osdev, INL (devc->osdev, CHFORMAT) & ~(1 << 20),
		CHFORMAT);
	}
      else
	{
	  /* Disable AC3 */
	  OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) & ~(1 << 18),
		MISC_CTRL);
	}
    }
}


static void
cmpci_setmixer (cmpci_devc * devc, unsigned int port, unsigned int value)
{
  oss_native_word flags;
  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  OUTB (devc->osdev, (unsigned char) (port & 0xff), MIXER_ADDR);
  oss_udelay (20);
  OUTB (devc->osdev, (unsigned char) (value & 0xff), MIXER_DATA);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  oss_udelay (20);
}

static unsigned int
cmpci_getmixer (cmpci_devc * devc, unsigned int port)
{
  unsigned int val;

  oss_native_word flags;
  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  OUTB (devc->osdev, (unsigned char) (port & 0xff), MIXER_ADDR);


  oss_udelay (20);
  val = INB (devc->osdev, MIXER_DATA);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  oss_udelay (20);
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

static mixer_tab cmpci_mix = {
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
  MIX_ENT (SOUND_MIXER_LINE1, 0, 0, 0, 0, 0, 0),
  MIX_ENT (SOUND_MIXER_RECLEV, 0x3f, 7, 2, 0x40, 7, 2),
  MIX_ENT (SOUND_MIXER_IGAIN, 0x3f, 7, 2, 0x40, 7, 2),
  MIX_ENT (SOUND_MIXER_OGAIN, 0x41, 7, 2, 0x42, 7, 2)
};

/*ARGSUSED*/
static void
change_bits (cmpci_devc * devc, unsigned char *regval, int dev, int chn,
	     int newval)
{
  unsigned char mask;
  int shift;

  mask = (1 << cmpci_mix[dev][chn].nbits) - 1;
  newval = (int) ((newval * mask) + 50) / 100;	/* Scale */

  shift = cmpci_mix[dev][chn].bitoffs - cmpci_mix[dev][LEFT_CHN].nbits + 1;

  *regval &= ~(mask << shift);	/* Mask out previous value */
  *regval |= (newval & mask) << shift;	/* Set the new value */
}

static int set_recmask (int dev, int mask);

static int
cmpci_mixer_set (cmpci_devc * devc, int chan, int value)
{
  int left = value & 0x000000ff;
  int right = (value & 0x0000ff00) >> 8;

  int regoffs;
  unsigned char val;

  if (left > 100)
    left = 100;
  if (right > 100)
    right = 100;

  if (chan > 31)
    return OSS_EINVAL;

  if (!(CMEDIA_MIXER_DEVICES & (1 << chan)))	/*
						 * Not supported
						 */
    return OSS_EINVAL;

  regoffs = cmpci_mix[chan][LEFT_CHN].regno;

  if (regoffs == 0)
    return OSS_EINVAL;

  val = cmpci_getmixer (devc, regoffs);
  change_bits (devc, &val, chan, LEFT_CHN, left);

  devc->levels[chan] = left | (left << 8);

  if (cmpci_mix[chan][RIGHT_CHN].regno != regoffs)	/*
							 * Change register
							 */
    {
      cmpci_setmixer (devc, regoffs, val);	/*
						 * Save the old one
						 */
      regoffs = cmpci_mix[chan][RIGHT_CHN].regno;

      if (regoffs == 0)
	return left | (left << 8);	/*
					 * Just left channel present
					 */

      val = cmpci_getmixer (devc, regoffs);	/*
						 * Read the new one
						 */
    }

  change_bits (devc, &val, chan, RIGHT_CHN, right);

  cmpci_setmixer (devc, regoffs, val);

  devc->levels[chan] = left | (right << 8);
  return left | (right << 8);
}

static int cmpci_outsw (int dev, int ctrl, unsigned int cmd, int value);

/*ARGSUSED*/
static int
cmpci_mixer_ioctl (int dev, int audiodev, unsigned int cmd, ioctl_arg arg)
{
  cmpci_devc *devc = mixer_devs[dev]->devc;
  int val;

  if (((cmd >> 8) & 0xff) == 'M')
    {
      if (IOC_IS_OUTPUT (cmd))
	switch (cmd & 0xff)
	  {
	  case SOUND_MIXER_RECSRC:
	    val = *arg;
	    if (val == SOUND_MASK_LINE1)
	      *arg = cmpci_outsw (dev, AUX_REC, SNDCTL_MIX_WRITE, 1);
	    else
	      *arg = cmpci_outsw (dev, AUX_REC, SNDCTL_MIX_WRITE, 0);
	    return *arg = set_recmask (dev, val);
	    break;

	  case SOUND_MIXER_LINE1:
	    val = *arg;
	    return *arg = cmpci_outsw (dev, AUX_LEVEL, SNDCTL_MIX_WRITE, val);
	    break;

	  default:
	    val = *arg;
	    return *arg = cmpci_mixer_set (devc, cmd & 0xff, val);
	  }
      else
	switch (cmd & 0xff)
	  {

	  case SOUND_MIXER_RECSRC:
	    return *arg = devc->recmask;
	    break;

	  case SOUND_MIXER_DEVMASK:
	    return *arg = CMEDIA_MIXER_DEVICES;
	    break;

	  case SOUND_MIXER_STEREODEVS:
	    return *arg = CMEDIA_MIXER_DEVICES &
	      ~(SOUND_MASK_MIC | SOUND_MASK_SPEAKER | SOUND_MASK_IMIX);
	    break;

	  case SOUND_MIXER_RECMASK:
	    return *arg = CMEDIA_RECORDING_DEVICES;
	    break;

	  case SOUND_MIXER_LINE1:
	    val = cmpci_outsw (dev, AUX_LEVEL, SNDCTL_MIX_READ, 0);
	    return *arg = val;
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

static int
set_recmask (int dev, int mask)
{
  cmpci_devc *devc = mixer_devs[dev]->devc;
  int devmask = mask & CMEDIA_RECORDING_DEVICES;
  int i;
  unsigned char regimageL, regimageR;

  if (!devmask)
    devmask = SOUND_MASK_MIC;

  regimageL = regimageR = 0;
  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
    if ((1 << i) & devmask)
      {
	regimageL |= cmpci_recmasks_L[i];
	regimageR |= cmpci_recmasks_R[i];
      }
  cmpci_setmixer (devc, CMPCI_IMASK_L, regimageL);
  cmpci_setmixer (devc, CMPCI_IMASK_R, regimageR);
  devc->recmask = devmask;
  return devc->recmask;
}

static int
cmpci_outsw (int dev, int ctrl, unsigned int cmd, int value)
{
/*
 * Access function for CMPCI mixer extension bits
 */
  cmpci_devc *devc = mixer_devs[dev]->devc;
  int left, right, tmp;

  if (cmd == SNDCTL_MIX_READ)
    {
      value = 0;
      switch (ctrl)
	{
	case MUTE_LINE:	/* Left line in to output connection */
	  value = (cmpci_getmixer (devc, 0x3c) & 0x18) ? 0 : 1;
	  break;

	case MUTE_CD:		/* Cd in to output connection */
	  value = (cmpci_getmixer (devc, 0x3c) & 0x06) ? 0 : 1;
	  break;

	case MUTE_MIC:		/* Mic in to output connection */
	  value = (cmpci_getmixer (devc, 0x3c) & 0x01) ? 0 : 1;
	  break;

	case MODE_4SPK:	/* 4Speaker out */
	  value = INL (devc->osdev, MISC_CTRL) & (1 << 26) ? 1 : 0;
	  break;

	case DUALDAC:		/* dual dac */
	  value = devc->dev_mode & DUALDAC_MODE ? 1 : 0;
	  break;

	case REAR2LINE:	/* rear to line in */
	  value = INB (devc->osdev, MIXER1) & (1 << 5) ? 1 : 0;
	  break;

	case CEN2LINE:		/* center to line in */
	  value = INL (devc->osdev, LEGACY_CTRL) & (1 << 14) ? 1 : 0;
	  break;

	case BASS2LINE:	/* basss to line in */
	  value = INL (devc->osdev, LEGACY_CTRL) & (1 << 13) ? 1 : 0;
	  break;

	case SPDIF_PLAY:	/* spdif out */
	  value = INL (devc->osdev, LEGACY_CTRL) & (1 << 23) ? 1 : 0;
	  break;

	case SPDIF_LOOP:	/* S/PDIF I/O Loop */
	  value = (INL (devc->osdev, FUNCTRL1) & (1 << 7)) ? 1 : 0;
	  break;

	case SPDIF_REC:	/* spdif record mode */
	  value = devc->dev_mode & SPDIFIN_MODE ? 1 : 0;
	  break;

	case SPDIF_IMON:	/* spdif input monitor */
	  value = INB (devc->osdev, MIXER1) & 0x1 ? 1 : 0;
	  break;

	case SPDIF_POL:	/* spdif input reverse */
	  if (devc->chiprev < 39)
	    value = INB (devc->osdev, MIXER3) & 0x06 ? 1 : 0;
	  else
	    value = INL (devc->osdev, CHFORMAT) & 0x80 ? 1 : 0;
	  break;

	case SPDIF_AC3:	/* ac3 */
	  value = INL (devc->osdev, MISC_CTRL) & (1 << 18) ? 1 : 0;
	  break;

	case SPDIF_COPY:	/* copy protect (indirect) */
	  value = INL (devc->osdev, LEGACY_CTRL) & (1 << 22) ? 1 : 0;
	  break;

	case SPDIF_OPT:	/* Coax/Optical Select */
	  value = INL (devc->osdev, MISC_CTRL) & (1 << 25) ? 1 : 0;
	  break;

	case CEN2MIC:		/* Center2MIC */
	  if (devc->chiprev >= 39)
	    value = INB (devc->osdev, MIXER3) & 0x4 ? 1 : 0;
	  break;

	case MICBOOST:		/* MIC Boost */
	  value = INB (devc->osdev, MIXER2) & 0x1 ? 0 : 1;
	  break;

	case AUX_LEVEL:
	  value = devc->levels[SOUND_MIXER_LINE1];
	  break;

	case AUX_REC:		/* set LINE1 as rec source - handled by set_recmask */
	  break;

	case MUTE_AUX:		/* AUX mute */
	  value = INB (devc->osdev, MIXER2) & 0x30 ? 0 : 1;
	  break;

	default:
	  return OSS_EINVAL;
	}

      return value;
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
      switch (ctrl)
	{
	case MUTE_LINE:	/* L/R line in to output connection */
	  if (!value)
	    cmpci_setmixer (devc, 0x3c, cmpci_getmixer (devc, 0x3c) | 0x18);
	  else
	    cmpci_setmixer (devc, 0x3c, cmpci_getmixer (devc, 0x3c) & ~0x18);
	  break;

	case MUTE_CD:		/* Cd in to output connection */
	  if (!value)
	    cmpci_setmixer (devc, 0x3c, cmpci_getmixer (devc, 0x3c) | 0x06);
	  else
	    cmpci_setmixer (devc, 0x3c, cmpci_getmixer (devc, 0x3c) & ~0x06);
	  break;

	case MUTE_MIC:		/* Mic in to output connection */
	  if (!value)
	    cmpci_setmixer (devc, 0x3c, cmpci_getmixer (devc, 0x3c) | 0x01);
	  else
	    cmpci_setmixer (devc, 0x3c, cmpci_getmixer (devc, 0x3c) & ~0x01);
	  break;

	case MODE_4SPK:	/* 4Speaker out */
	  if (value)
	    {
	      OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) | (1 << 26),
		    MISC_CTRL);
	      devc->mode_4spk = 1;
	    }
	  else
	    OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) & ~(1 << 26),
		  MISC_CTRL);
	  devc->mode_4spk = 0;
	  break;

	case DUALDAC:		/* DUAL DAC mode */
	  if (value)
	    {
	      OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) | (1 << 23),
		    MISC_CTRL);
	      /* Disable 4Speaker mode */
	      OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) & ~(1 << 26),
		    MISC_CTRL);
	      devc->dev_mode = DUALDAC_MODE;
	    }
	  else
	    {
	      OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) & ~(1 << 23),
		    MISC_CTRL);

	      /* enable back the 4Speaker mode */
	      OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) | 1 << 26,
		    MISC_CTRL);

	      devc->dev_mode = DEFAULT_MODE;
	    }
	  break;

	case REAR2LINE:	/* REAR TO LINEIN */
	  if (value)
	    {
	      OUTB (devc->osdev, (INB (devc->osdev, MIXER1) | (1 << 5)),
		    MIXER1);
	    }
	  else
	    OUTB (devc->osdev, (INB (devc->osdev, MIXER1) & ~(1 << 5)),
		  MIXER1);
	  break;

	case CEN2LINE:		/* CENTER TO LINEIN */
	  if (value)
	    {
	      OUTL (devc->osdev, INL (devc->osdev, LEGACY_CTRL) | (1 << 14),
		    LEGACY_CTRL);
	    }
	  else
	    OUTL (devc->osdev, INL (devc->osdev, LEGACY_CTRL) & ~(1 << 14),
		  LEGACY_CTRL);
	  break;

	case BASS2LINE:	/* BASS TO LINEIN */
	  if (value)
	    {
	      OUTL (devc->osdev, INL (devc->osdev, LEGACY_CTRL) | (1 << 13),
		    LEGACY_CTRL);
	    }
	  else
	    OUTL (devc->osdev, INL (devc->osdev, LEGACY_CTRL) & ~(1 << 13),
		  LEGACY_CTRL);
	  break;

	case SPDIF_PLAY:	/* SPDIF ENABLE */
	  if (value)
	    {
	      OUTL (devc->osdev, INL (devc->osdev, LEGACY_CTRL) | (1 << 23),
		    LEGACY_CTRL);

	      /* enable wave/fm/midi to spdif OUT DAC2SPDO on rev 33/37 */
	      if (devc->chiprev < 39)
		OUTL (devc->osdev, INL (devc->osdev, LEGACY_CTRL) | (1 << 21),
		      LEGACY_CTRL);
	      devc->spdif_enabled = 1;
	    }
	  else
	    {
	      OUTL (devc->osdev, INL (devc->osdev, LEGACY_CTRL) & ~(1 << 23),
		    LEGACY_CTRL);
	      /* Disable wave/fm/midi to spdif OUT (DAC2SPDO) */
	      if (devc->chiprev < 39)
		OUTL (devc->osdev,
		      INL (devc->osdev, LEGACY_CTRL) & ~(1 << 21),
		      LEGACY_CTRL);
	      devc->spdif_enabled = 0;
	    }
	  break;

	case SPDIF_LOOP:	/* S/PDIF I/O Loop */
	  if (value)
	    {
	      OUTL (devc->osdev, INL (devc->osdev, FUNCTRL1) | (1 << 7),
		    FUNCTRL1);
	    }
	  else
	    OUTL (devc->osdev, INL (devc->osdev, FUNCTRL1) & ~(1 << 7),
		  FUNCTRL1);
	  break;

	case SPDIF_REC:	/* SPDIF Record Mode */
	  if (value)
	    {
	      devc->dev_mode = SPDIFIN_MODE;
	    }
	  else
	    devc->dev_mode = DEFAULT_MODE;
	  break;

	case SPDIF_IMON:	/*  spdif monitor */
	  if (value)
	    {
	      OUTB (devc->osdev, INB (devc->osdev, MIXER1) | 0x0D, MIXER1);
	    }
	  else
	    OUTB (devc->osdev, INB (devc->osdev, MIXER1) & ~0xD, MIXER1);
	  break;

	case SPDIF_POL:	/* spdif reverse */
	  if (value)
	    {
	      if (devc->chiprev < 39)
		{
		  OUTB (devc->osdev, INB (devc->osdev, MIXER3) | 0x06,
			MIXER3);
		}
	      else
		OUTL (devc->osdev, INL (devc->osdev, CHFORMAT) | 0x80,
		      CHFORMAT);
	    }
	  else
	    {
	      if (devc->chiprev < 39)
		{
		  OUTB (devc->osdev, INB (devc->osdev, MIXER3) & ~0x06,
			MIXER3);
		}
	      else
		OUTL (devc->osdev, INL (devc->osdev, CHFORMAT) & ~0x80,
		      CHFORMAT);
	    }
	  break;

	case SPDIF_AC3:	/* AC3 enabled on S/PDIF */
	  if (value)
	    {
	      OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) | (1 << 18),
		    MISC_CTRL);
	    }
	  else
	    OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) & ~(1 << 18),
		  MISC_CTRL);
	  break;

	case SPDIF_COPY:	/* Copy protect */
	  if (value)
	    {
	      OUTL (devc->osdev, INL (devc->osdev, LEGACY_CTRL) | (1 << 22),
		    LEGACY_CTRL);
	    }
	  else
	    OUTL (devc->osdev, INL (devc->osdev, LEGACY_CTRL) & ~(1 << 22),
		  LEGACY_CTRL);
	  break;

	case SPDIF_OPT:	/* Coax/Optical */
	  if (value)
	    {
	      OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) | (1 << 25),
		    MISC_CTRL);
	    }
	  else
	    OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) & ~(1 << 25),
		  MISC_CTRL);
	  break;

	case CEN2MIC:		/* Center -> Mic OUT */
	  if (value)
	    {
	      if (devc->chiprev >= 39)
		{
		  OUTB (devc->osdev, INB (devc->osdev, MIXER3) | 0x4, MIXER3);
		}
	    }
	  else if (devc->chiprev >= 39)
	    {
	      OUTB (devc->osdev, INB (devc->osdev, MIXER3) & ~0x4, MIXER3);
	    }
	  break;

	case MICBOOST:		/* Mic Boost */
	  if (!value)
	    {
	      OUTB (devc->osdev, INB (devc->osdev, MIXER2) | 0x1, MIXER2);
	    }
	  else
	    OUTB (devc->osdev, INB (devc->osdev, MIXER2) & ~0x1, MIXER2);
	  break;

	case AUX_LEVEL:	/* Aux levels */
	  left = value & 0xff;
	  right = (value >> 8) & 0xff;
	  if (left > 100)
	    left = 100;
	  if (right > 100)
	    right = 100;
	  value = left | (right << 8);
	  left = mix_cvt[left];
	  right = mix_cvt[right];

	  tmp = ((right * ((1 << 4) - 1) / 100) << 4) |
	    (left * ((1 << 4) - 1) / 100);

	  OUTB (devc->osdev, tmp, AUX_VOL);
	  devc->levels[SOUND_MIXER_LINE1] = value;
	  break;

	case AUX_REC:		/* line1 record select */
	  if (value)
	    {
	      OUTB (devc->osdev, INB (devc->osdev, MIXER2) | 0xc0, MIXER2);
	    }
	  else
	    OUTB (devc->osdev, INB (devc->osdev, MIXER2) & ~0xc0, MIXER2);
	  break;

	case MUTE_AUX:		/* line1 mute control */
	  if (!value)
	    {
	      OUTB (devc->osdev, INB (devc->osdev, MIXER2) | 0x30, MIXER2);
	    }
	  else
	    OUTB (devc->osdev, INB (devc->osdev, MIXER2) & ~0x30, MIXER2);
	  break;

	default:
	  return OSS_EINVAL;
	}

      return (value);
    }
  return OSS_EINVAL;
}

static int
cmpci_mix_init (int dev)
{
  int group, err;

  if ((group = mixer_ext_create_group (dev, 0, "CMPCI_MUTECTL")) < 0)
    return group;

  if ((err =
       mixer_ext_create_control (dev, group, MUTE_LINE, cmpci_outsw,
				 MIXT_ONOFF, "CMPCI_LINEMUTE", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, MUTE_CD, cmpci_outsw, MIXT_ONOFF,
				 "CMPCI_CDMUTE", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, MUTE_MIC, cmpci_outsw,
				 MIXT_ONOFF, "CMPCI_MICMUTE", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, MUTE_AUX, cmpci_outsw,
				 MIXT_ONOFF, "CMPCI_LINE1MUTE", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((group = mixer_ext_create_group (dev, 0, "CMPCI_JACKCTL")) < 0)
    return group;
  if ((err =
       mixer_ext_create_control (dev, group, REAR2LINE, cmpci_outsw,
				 MIXT_ONOFF, "CMPCI_REAR2LINE", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;
  if ((err =
       mixer_ext_create_control (dev, group, CEN2LINE, cmpci_outsw,
				 MIXT_ONOFF, "CMPCI_CEN2LINE", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;
  if ((err =
       mixer_ext_create_control (dev, group, BASS2LINE, cmpci_outsw,
				 MIXT_ONOFF, "CMPCI_BASS2LINE", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, CEN2MIC, cmpci_outsw, MIXT_ONOFF,
				 "CMPCI_CEN2MIC", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;


  if ((group = mixer_ext_create_group (dev, 0, "CMPCI_MIXEXT")) < 0)
    return group;

  if ((err =
       mixer_ext_create_control (dev, group, MODE_4SPK, cmpci_outsw,
				 MIXT_ENUM, "CMPCI_SPKMODE", 2,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, DUALDAC, cmpci_outsw, MIXT_ONOFF,
				 "CMPCI_DUALDAC", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, MICBOOST, cmpci_outsw,
				 MIXT_ONOFF, "CMPCI_MICBOOST", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  /* Create a new SPDIF group */
  if ((group = mixer_ext_create_group (dev, 0, "CMPCI_SPDIF")) < 0)
    return group;

  if ((err =
       mixer_ext_create_control (dev, group, SPDIF_PLAY, cmpci_outsw,
				 MIXT_ONOFF, "CMPCI_Play", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, SPDIF_LOOP, cmpci_outsw,
				 MIXT_ONOFF, "CMPCI_LOOP", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  /* Having this in mixer doesn't make any sense */
  if ((err =
       mixer_ext_create_control (dev, group, SPDIF_REC, cmpci_outsw,
				 MIXT_ONOFF, "CMPCI_RECORD", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, SPDIF_IMON, cmpci_outsw,
				 MIXT_ONOFF, "CMPCI_IMON", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, SPDIF_POL, cmpci_outsw,
				 MIXT_ONOFF, "CMPCI_POLREV", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

#if 0
  /* Having this in mixer doesn't make any sense */
  if ((err =
       mixer_ext_create_control (dev, group, SPDIF_AC3, cmpci_outsw,
				 MIXT_ONOFF, "CMPCI_AC3", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;
#endif

  if ((err =
       mixer_ext_create_control (dev, group, SPDIF_COPY, cmpci_outsw,
				 MIXT_ONOFF, "CMPCI_COPYPROT", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, SPDIF_OPT, cmpci_outsw,
				 MIXT_ONOFF, "CMPCI_OPTICAL", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  return 0;
}

static void
cmpci_mixer_reset (int dev)
{
  int i;
  cmpci_devc *devc = mixer_devs[dev]->devc;

  devc->levels = load_mixer_volumes ("CM8738_Mixer", default_levels, 1);
  devc->recmask = 0;

  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
    cmpci_mixer_set (devc, i, devc->levels[i]);

  set_recmask (dev, SOUND_MASK_MIC);
}


static mixer_driver_t cmpci_mixer_driver = {
  cmpci_mixer_ioctl
};


static int
cmpciintr (oss_device_t * osdev)
{
  cmpci_devc *devc = (cmpci_devc *) osdev->devc;
  unsigned int intstat, intsrc;
  int i;
  int serviced = 0;

  /* see if this is our interrupt */
  intsrc = INL (devc->osdev, INT_STATUS);
  if (intsrc & (CM_CH0_INT | CM_CH1_INT))
    {
      /* Handle playback */
      serviced = 1;

      intstat = INB (devc->osdev, INT_HLDCLR + 2);

      for (i = 0; i < MAX_PORTC; i++)
	{
	  cmpci_portc *portc = &devc->portc[i];

	  if (intstat & CM_CH1_INT)
	    {
	      /* do chan1 playback */
	      if ((portc->chan1_play) &&
		  (portc->trigger_bits & PCM_ENABLE_OUTPUT))
		{
		  OUTB (devc->osdev, intstat & ~CM_CH1_INT, INT_HLDCLR + 2);
		  OUTB (devc->osdev, intstat | CM_CH1_INT, INT_HLDCLR + 2);
		  oss_audio_outputintr (portc->audiodev, 0);
		}

	      /* do chan1 record */
	      if ((portc->chan1_rec) &&
		  (portc->trigger_bits & PCM_ENABLE_INPUT))
		{
		  OUTB (devc->osdev, intstat & ~CM_CH1_INT, INT_HLDCLR + 2);
		  OUTB (devc->osdev, intstat | CM_CH1_INT, INT_HLDCLR + 2);
		  oss_audio_inputintr (portc->audiodev, 0);
		}
	    }

	  if (intstat & CM_CH0_INT)
	    {
	      /* do chan0 playback */
	      if ((portc->chan0_play) &&
		  (portc->trigger_bits & PCM_ENABLE_OUTPUT))
		{
		  OUTB (devc->osdev, intstat & ~CM_CH0_INT, INT_HLDCLR + 2);
		  OUTB (devc->osdev, intstat | CM_CH0_INT, INT_HLDCLR + 2);
		  oss_audio_outputintr (portc->audiodev, 0);
		}

	      /* do chan0 record */
	      if ((portc->chan0_rec) &&
		  (portc->trigger_bits & PCM_ENABLE_INPUT))
		{
		  OUTB (devc->osdev, intstat & ~CM_CH0_INT, INT_HLDCLR + 2);
		  OUTB (devc->osdev, intstat | CM_CH0_INT, INT_HLDCLR + 2);
		  oss_audio_inputintr (portc->audiodev, 0);
		}
	    }
	}
    }

  if (intsrc & 0x10000)
    {
      serviced = 1;
      uart401_irq (&devc->uart401devc);
    }

  return serviced;
}

static int
cmpci_audio_set_rate (int dev, int arg)
{
  cmpci_portc *portc = audio_engines[dev]->portc;

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
cmpci_audio_set_channels (int dev, short arg)
{
  cmpci_devc *devc = audio_engines[dev]->devc;
  cmpci_portc *portc = audio_engines[dev]->portc;

  if (devc->model == MDL_CM8768)
    {
      if (arg>8)
	 arg=8;

      if ((arg != 1) && (arg != 2) && (arg != 4) && (arg != 6) && (arg != 8))
	return portc->channels;
    }
  else
    {
      if (arg>6)
	 arg=6;

      if ((arg != 1) && (arg != 2) && (arg != 4) && (arg != 6))
	return portc->channels;
    }
  portc->channels = arg;

  return portc->channels;
}

static unsigned int
cmpci_audio_set_format (int dev, unsigned int arg)
{
  cmpci_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->bits;

  if (!(arg & (AFMT_U8 | AFMT_S16_LE | AFMT_AC3)))
    return portc->bits;
  portc->bits = arg;

  return portc->bits;
}

/*ARGSUSED*/
static int
cmpci_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void cmpci_audio_trigger (int dev, int state);

static void
cmpci_audio_reset (int dev)
{
  cmpci_audio_trigger (dev, 0);
}

static void
cmpci_audio_reset_input (int dev)
{
  cmpci_portc *portc = audio_engines[dev]->portc;
  cmpci_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
cmpci_audio_reset_output (int dev)
{
  cmpci_portc *portc = audio_engines[dev]->portc;
  cmpci_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

/*ARGSUSED*/
static int
cmpci_audio_open (int dev, int mode, int open_flags)
{
  cmpci_portc *portc = audio_engines[dev]->portc;
  cmpci_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  if (!(devc->dev_mode & DUALDAC_MODE))
    {
      if (devc->open_mode & mode)
	{
	  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
	  return OSS_EBUSY;
	}

      devc->open_mode |= mode;
    }
  portc->open_mode = mode;
  portc->audio_enabled &= ~mode;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

static void
cmpci_audio_close (int dev, int mode)
{
  cmpci_portc *portc = audio_engines[dev]->portc;
  cmpci_devc *devc = audio_engines[dev]->devc;

  cmpci_audio_reset (dev);
  portc->open_mode = 0;
  devc->open_mode &= ~mode;
  portc->audio_enabled &= ~mode;

  if ((devc->spdif_enabled) || (devc->dev_mode & SPDIFIN_MODE))
    OUTL (devc->osdev, INL (devc->osdev, FUNCTRL1) & ~(1 << 9), FUNCTRL1);

  if (portc->chan0_play)
    portc->chan0_play = 0;

  if (portc->chan1_play)
    portc->chan1_play = 0;

  if (portc->chan0_rec)
    portc->chan0_rec = 0;

  if (portc->chan1_rec)
    portc->chan1_rec = 0;
}

/*ARGSUSED*/
static void
cmpci_audio_output_block (int dev, oss_native_word buf, int count,
			  int fragsize, int intrflag)
{
  cmpci_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
}

/*ARGSUSED*/
static void
cmpci_audio_start_input (int dev, oss_native_word buf, int count,
			 int fragsize, int intrflag)
{
  cmpci_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
}

static void
cmpci_audio_trigger (int dev, int state)
{
  cmpci_portc *portc = audio_engines[dev]->portc;
  cmpci_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode & OPEN_WRITE)
    {
      if (state & PCM_ENABLE_OUTPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      if (portc->chan0_play)
		{
		  /* enable the channel0 */
		  OUTB (devc->osdev,
			INB (devc->osdev, FUNCTRL0 + 2) | CM_CH0_ENABLE,
			FUNCTRL0 + 2);
		  OUTB (devc->osdev,
			INB (devc->osdev, INT_HLDCLR + 2) | CM_CH0_INT,
			INT_HLDCLR + 2);
		}

	      if (portc->chan1_play)
		{
		  /* enable the channel1 */
		  OUTB (devc->osdev,
			INB (devc->osdev, FUNCTRL0 + 2) | CM_CH1_ENABLE,
			FUNCTRL0 + 2);
		  OUTB (devc->osdev,
			INB (devc->osdev, INT_HLDCLR + 2) | CM_CH1_INT,
			INT_HLDCLR + 2);
		}
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

	      if (portc->chan0_play)
		{
		  /* disable interrupt */
		  OUTB (devc->osdev,
			INB (devc->osdev, INT_HLDCLR + 2) & ~CM_CH0_INT,
			INT_HLDCLR + 2);

		  /* disable channel0 */
		  OUTB (devc->osdev,
			INB (devc->osdev, FUNCTRL0 + 2) & ~CM_CH0_ENABLE,
			FUNCTRL0 + 2);
		}
	      if (portc->chan1_play)
		{
		  /* disable interrupt */
		  OUTB (devc->osdev,
			INB (devc->osdev, INT_HLDCLR + 2) & ~CM_CH1_INT,
			INT_HLDCLR + 2);

		  /* disable channel */
		  OUTB (devc->osdev,
			INB (devc->osdev, FUNCTRL0 + 2) & ~CM_CH1_ENABLE,
			FUNCTRL0 + 2);
		}
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
	      if (portc->chan1_rec)
		{
		  /* enable the channel1 */
		  OUTB (devc->osdev,
			INB (devc->osdev, FUNCTRL0 + 2) | CM_CH1_ENABLE,
			FUNCTRL0 + 2);
		  OUTB (devc->osdev,
			INB (devc->osdev, INT_HLDCLR + 2) | CM_CH1_INT,
			INT_HLDCLR + 2);
		}
	      if (portc->chan0_rec)
		{
		  /* enable the channel0 */
		  OUTB (devc->osdev,
			INB (devc->osdev, FUNCTRL0 + 2) | CM_CH0_ENABLE,
			FUNCTRL0 + 2);
		  OUTB (devc->osdev,
			INB (devc->osdev, INT_HLDCLR + 2) | CM_CH0_INT,
			INT_HLDCLR + 2);
		}
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
	      if (portc->chan1_rec)
		{
		  /* disable interrupt */
		  OUTB (devc->osdev,
			INB (devc->osdev, INT_HLDCLR + 2) & ~CM_CH1_INT,
			INT_HLDCLR + 2);

		  /* disable channel 1 */
		  OUTB (devc->osdev,
			INB (devc->osdev, FUNCTRL0 + 2) & ~CM_CH1_ENABLE,
			FUNCTRL0 + 2);
		}
	      if (portc->chan0_rec)
		{
		  /* disable interrupt */
		  OUTB (devc->osdev,
			INB (devc->osdev, INT_HLDCLR + 2) & ~CM_CH0_INT,
			INT_HLDCLR + 2);

		  /* disable channel 0 */
		  OUTB (devc->osdev,
			INB (devc->osdev, FUNCTRL0 + 2) & ~CM_CH0_ENABLE,
			FUNCTRL0 + 2);

		}
	    }
	}
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static void
set_dac_rate (int dev, int chan_type)
{
  cmpci_devc *devc = audio_engines[dev]->devc;
  cmpci_portc *portc = audio_engines[dev]->portc;
  unsigned char freq = 4, val;
  int i;
  int rate = portc->speed;

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
  if (chan_type == CHAN0)
    {
      val = INB (devc->osdev, FUNCTRL1 + 1) & ~0x1c;
      OUTB (devc->osdev, val | freq << 2, FUNCTRL1 + 1);
    }
  else
    {
      val = INB (devc->osdev, FUNCTRL1 + 1) & ~0xe0;
      OUTB (devc->osdev, val | freq << 5, FUNCTRL1 + 1);
    }
  if (devc->spdif_enabled)
    set_spdif_rate (devc, portc);
}

static void
set_dac_fmt (int dev, int chan_type)
{
  unsigned char val;
  cmpci_devc *devc = audio_engines[dev]->devc;
  cmpci_portc *portc = audio_engines[dev]->portc;
  int channels = portc->channels;
  int bits = portc->bits;

  if (chan_type == CHAN0)
    {
      /* Set the format on Channl 0 */
      val = INB (devc->osdev, CHFORMAT) & ~0x3;

      if ((channels == 1) && (bits == 8))
	{
	  OUTB (devc->osdev, 0x00 | val, CHFORMAT);
	  portc->dacfmt = 0;
	}

      if ((channels == 2) && (bits == 8))
	{
	  OUTB (devc->osdev, 0x01 | val, CHFORMAT);
	  portc->dacfmt = 1;
	}

      if ((channels == 1) && (bits == 16))
	{
	  OUTB (devc->osdev, 0x02 | val, CHFORMAT);
	  portc->dacfmt = 1;
	}

      if ((channels > 1) && (bits == 16))
	{
	  OUTB (devc->osdev, 0x03 | val, CHFORMAT);
	  portc->dacfmt = 2;
	}
    }
  else
    {
      /* Set the format on Channel 1 */
      val = INB (devc->osdev, CHFORMAT) & ~0xC;

      if ((channels == 1) && (bits == 8))
	{
	  OUTB (devc->osdev, 0x00 | val, CHFORMAT);
	  portc->dacfmt = 0;
	}

      if ((channels == 2) && (bits == 8))
	{
	  OUTB (devc->osdev, 0x04 | val, CHFORMAT);
	  portc->dacfmt = 1;
	}

      if ((channels == 1) && (bits == 16))
	{
	  OUTB (devc->osdev, 0x08 | val, CHFORMAT);
	  portc->dacfmt = 1;
	}

      if ((channels > 1) && (bits == 16))
	{
	  OUTB (devc->osdev, 0x0C | val, CHFORMAT);
	  portc->dacfmt = 2;
	}
    }
}


static void
set_adc_rate (int dev, int chan_type)
{
  cmpci_devc *devc = audio_engines[dev]->devc;
  cmpci_portc *portc = audio_engines[dev]->portc;
  unsigned char freq = 4, val;
  int i;
  int rate = portc->speed;

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
  if (chan_type == CHAN1)
    {
      val = INB (devc->osdev, FUNCTRL1 + 1) & ~0xe0;
      OUTB (devc->osdev, val | freq << 5, FUNCTRL1 + 1);
    }
  else
    {
      val = INB (devc->osdev, FUNCTRL1 + 1) & ~0x1c;
      OUTB (devc->osdev, val | freq << 2, FUNCTRL1 + 1);
    }

  if (devc->dev_mode & SPDIFIN_MODE)
    set_spdif_rate (devc, portc);
}

static void
set_adc_fmt (int dev, int chan_type)
{
  unsigned char val;
  cmpci_devc *devc = audio_engines[dev]->devc;
  cmpci_portc *portc = audio_engines[dev]->portc;
  int channels = portc->channels;
  int bits = portc->bits;

  if (chan_type == CHAN1)
    {
      /* Set the format on Channel 1 */
      val = INB (devc->osdev, CHFORMAT) & ~0xC;

      if ((channels == 1) && (bits == 8))
	{
	  OUTB (devc->osdev, 0x00 | val, CHFORMAT);
	  portc->adcfmt = 0;
	}

      if ((channels == 2) && (bits == 8))
	{
	  OUTB (devc->osdev, 0x04 | val, CHFORMAT);
	  portc->adcfmt = 1;
	}

      if ((channels == 1) && (bits == 16))
	{
	  OUTB (devc->osdev, 0x08 | val, CHFORMAT);
	  portc->adcfmt = 1;
	}

      if ((channels > 1) && (bits == 16))
	{
	  OUTB (devc->osdev, 0x0C | val, CHFORMAT);
	  portc->adcfmt = 2;
	}
    }
  else
    {
      /* Set the format on Channl 0 */
      val = INB (devc->osdev, CHFORMAT) & ~0x3;

      if ((channels == 1) && (bits == 8))
	{
	  OUTB (devc->osdev, 0x00 | val, CHFORMAT);
	  portc->adcfmt = 0;
	}

      if ((channels == 2) && (bits == 8))
	{
	  OUTB (devc->osdev, 0x01 | val, CHFORMAT);
	  portc->adcfmt = 1;
	}

      if ((channels == 1) && (bits == 16))
	{
	  OUTB (devc->osdev, 0x02 | val, CHFORMAT);
	  portc->adcfmt = 1;
	}

      if ((channels == 2) && (bits == 16))
	{
	  OUTB (devc->osdev, 0x03 | val, CHFORMAT);
	  portc->adcfmt = 2;
	}
    }
}

static void
setup_record (int dev, int chan_type)
{
  cmpci_devc *devc = audio_engines[dev]->devc;
  cmpci_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_in;

  if (chan_type == CHAN1)	/* SPDIF Record can only occur on CHAN1 */
    {
      /* reset and disable channel */
      OUTB (devc->osdev,
	    INB (devc->osdev, FUNCTRL0 + 2) | CM_CH1_RESET, FUNCTRL0 + 2);
      oss_udelay (10);
      OUTB (devc->osdev,
	    INB (devc->osdev, FUNCTRL0 + 2) & ~CM_CH1_RESET, FUNCTRL0 + 2);

      cmpci_outsw (devc->mixer_dev, SPDIF_PLAY, SNDCTL_MIX_WRITE, 0);

      OUTL (devc->osdev, dmap->dmabuf_phys, CH1_FRAME1);
      OUTW (devc->osdev, (dmap->bytes_in_use >> portc->adcfmt) - 1,
	    CH1_FRAME2);
      OUTW (devc->osdev, (dmap->fragment_size >> portc->adcfmt),
	    CH1_FRAME2 + 2);

      /* set channel 1 to record mode */
      OUTB (devc->osdev, INB (devc->osdev, FUNCTRL0) | CM_CH1_RECORD,
	    FUNCTRL0);
      portc->chan1_rec = 1;

      /* setup SPDIF in on CHAN A */
      OUTL (devc->osdev, INL (devc->osdev, FUNCTRL1) | (1 << 9), FUNCTRL1);
    }
  else				/* Normal PCM record on Channel 0 */
    {
      /* reset and disable channel */
      OUTB (devc->osdev,
	    INB (devc->osdev, FUNCTRL0 + 2) | CM_CH0_RESET, FUNCTRL0 + 2);
      oss_udelay (10);
      OUTB (devc->osdev,
	    INB (devc->osdev, FUNCTRL0 + 2) & ~CM_CH0_RESET, FUNCTRL0 + 2);

      OUTL (devc->osdev, dmap->dmabuf_phys, CH0_FRAME1);
      OUTW (devc->osdev, (dmap->bytes_in_use >> portc->adcfmt) - 1,
	    CH0_FRAME2);
      OUTW (devc->osdev, (dmap->fragment_size >> portc->adcfmt),
	    CH0_FRAME2 + 2);

      /* set channel 0 to record mode */
      OUTB (devc->osdev, INB (devc->osdev, FUNCTRL0) | CM_CH0_RECORD,
	    FUNCTRL0);
      portc->chan0_rec = 1;
    }
}

/*ARGSUSED*/
static int
cmpci_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  cmpci_devc *devc = audio_engines[dev]->devc;
  cmpci_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  switch (devc->dev_mode)
    {
    case DEFAULT_MODE:
      set_adc_rate (dev, CHAN0);
      set_adc_fmt (dev, CHAN0);
      setup_record (dev, CHAN0);
      break;

    case DUALDAC_MODE:
      cmn_err (CE_WARN, "Cannot record because DUALDAC mode is ON.\n");
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EIO;

    case SPDIFIN_MODE:
      if (portc->speed < 44100)
	{
	  cmn_err (CE_WARN,
		   "Cannot record spdif at sampling rate less than 44.1Khz.\n");
	  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
	  return OSS_EIO;
	}
      set_adc_rate (dev, CHAN1);
      set_adc_fmt (dev, CHAN1);
      setup_record (dev, CHAN1);
      break;
    }
  portc->audio_enabled &= ~PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

static void
setup_play (int dev, int chan_type)
{
  cmpci_devc *devc = audio_engines[dev]->devc;
  cmpci_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_out;

  if (chan_type == CHAN0)
    {
      /* reset channel */
      OUTB (devc->osdev,
	    INB (devc->osdev, FUNCTRL0 + 2) | CM_CH0_RESET, FUNCTRL0 + 2);
      oss_udelay (10);
      OUTB (devc->osdev,
	    INB (devc->osdev, FUNCTRL0 + 2) & ~CM_CH0_RESET, FUNCTRL0 + 2);
      oss_udelay (10);

      /* Now set the buffer address/sizes */
      OUTL (devc->osdev, dmap->dmabuf_phys, CH0_FRAME1);
      OUTW (devc->osdev, (dmap->bytes_in_use >> portc->dacfmt) - 1,
	    CH0_FRAME2);
      OUTW (devc->osdev, (dmap->fragment_size >> portc->dacfmt),
	    CH0_FRAME2 + 2);

      /* set channel 0 to play mode */
      OUTB (devc->osdev, INB (devc->osdev, FUNCTRL0) & CM_CH0_PLAY, FUNCTRL0);
      portc->chan0_play = 1;

      /* setup spdif output on CHAN A , disable CHAN B spdif */
      if (devc->spdif_enabled)
	{
	  OUTL (devc->osdev, INL (devc->osdev, FUNCTRL1) | (1 << 8),
		FUNCTRL1);
	  OUTL (devc->osdev, INL (devc->osdev, FUNCTRL1) & ~(1 << 9),
		FUNCTRL1);
	}
    }
  else
    {
      /* reset and disable channel */
      OUTB (devc->osdev,
	    INB (devc->osdev, FUNCTRL0 + 2) | CM_CH1_RESET, FUNCTRL0 + 2);
      oss_udelay (10);
      OUTB (devc->osdev,
	    INB (devc->osdev, FUNCTRL0 + 2) & ~CM_CH1_RESET, FUNCTRL0 + 2);
      oss_udelay (10);

      /* Handle 4/5/6 channel mode */
      if (portc->channels < 4)
	{
	  /* check if 4speaker mode is enabled from mixer or not */
	  if (devc->mode_4spk)
	    OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) & ~(1 << 26),
		  MISC_CTRL);

	  /* disable 4channel mode on CHAN B */
	  OUTL (devc->osdev, INL (devc->osdev, CHFORMAT) & ~(1 << 29),
		CHFORMAT);
	  /* disable 5 channel mode on CHAN B */
	  OUTL (devc->osdev, INL (devc->osdev, CHFORMAT) & ~(0x80000000),
		CHFORMAT);
	  /* disable 6channel mode out CHAN B */
	  OUTL (devc->osdev, INL (devc->osdev, LEGACY_CTRL) & ~(1 << 15),
		LEGACY_CTRL);
	  /* disable 8 channel decode on CHAN B - only for CMI8768 */
	  if (devc->model == MDL_CM8768)
	    OUTB (devc->osdev, INB (devc->osdev, MISC2_CTRL) & ~0x20,
		  MISC2_CTRL);
	  /* Set NXCNG */
	  OUTL (devc->osdev, INL (devc->osdev, LEGACY_CTRL) & ~(0x80000000),
		LEGACY_CTRL);
	}

      if ((portc->channels == 4) && (devc->chiprev > 37))
	{
	  /* disable 4 speaker mode */
	  OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) & ~(1 << 26),
		MISC_CTRL);
	  /* enable 4channel mode on CHAN B */
	  OUTL (devc->osdev, INL (devc->osdev, CHFORMAT) | (1 << 29),
		CHFORMAT);

	  /* disable 5 channel mode on CHAN B */
	  OUTL (devc->osdev, INL (devc->osdev, CHFORMAT) & ~(0x80000000),
		CHFORMAT);
	  /* disable 6channel mode out CHAN B */
	  OUTL (devc->osdev, INL (devc->osdev, LEGACY_CTRL) & ~(1 << 15),
		LEGACY_CTRL);
	  /* disable center/bass channel */
	  OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) & ~(1 << 7),
		MISC_CTRL);
	  /* disable bass */
	  OUTL (devc->osdev, INL (devc->osdev, LEGACY_CTRL) & ~(1 << 12),
		LEGACY_CTRL);

	  /* disable 8 channel decode on CHAN B - only for CMI8768 */
	  if (devc->model == MDL_CM8768)
	    OUTB (devc->osdev, INB (devc->osdev, MISC2_CTRL) & ~0x20,
		  MISC2_CTRL);
	  /* Set NXCNG */
	  OUTL (devc->osdev, INL (devc->osdev, LEGACY_CTRL) & ~(0x80000000),
		LEGACY_CTRL);
	}

      if ((portc->channels == 6) && (devc->chiprev > 37))
	{
	  /* disable 4 speaker mode */
	  OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) & ~(1 << 26),
		MISC_CTRL);
	  /* disable 4channel mode on CHAN B */
	  OUTL (devc->osdev, INL (devc->osdev, CHFORMAT) & ~(1 << 29),
		CHFORMAT);

	  /* enable center channel */
	  OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) | (1 << 7),
		MISC_CTRL);
	  /* enable bass */
	  OUTL (devc->osdev, INL (devc->osdev, LEGACY_CTRL) | (1 << 12),
		LEGACY_CTRL);
	  /* enable 5 channel mode on CHAN B */
	  OUTL (devc->osdev, INL (devc->osdev, CHFORMAT) | (0x80000000),
		CHFORMAT);
	  /* enable 6 channel decode on CHAN B */
	  OUTL (devc->osdev, INL (devc->osdev, LEGACY_CTRL) | (1 << 15),
		LEGACY_CTRL);

	  /* disable 8 channel decode on CHAN B - only for CMI8768 */
	  if (devc->model == MDL_CM8768)
	    OUTB (devc->osdev, INB (devc->osdev, MISC2_CTRL) & ~0x20,
		  MISC2_CTRL);

	  /* Set NXCNG */
	  OUTL (devc->osdev, INL (devc->osdev, LEGACY_CTRL) | (0x80000000),
		LEGACY_CTRL);
	}

      if ((portc->channels == 8) && (devc->model == MDL_CM8768))
	{
	  /* disable 4 speaker mode */
	  OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) & ~(1 << 26),
		MISC_CTRL);
	  /* disable 4channel mode on CHAN B */
	  OUTL (devc->osdev, INL (devc->osdev, CHFORMAT) & ~(1 << 29),
		CHFORMAT);

	  /* enable center channel */
	  OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) | (1 << 7),
		MISC_CTRL);
	  /* enable bass channel */
	  OUTL (devc->osdev, INL (devc->osdev, LEGACY_CTRL) | (1 << 12),
		LEGACY_CTRL);
	  /* disable 5 channel mode on CHAN B */
	  OUTL (devc->osdev, INL (devc->osdev, CHFORMAT) & ~(0x80000000),
		CHFORMAT);
	  /* disable 6 channel decode on CHAN B */
	  OUTL (devc->osdev, INL (devc->osdev, LEGACY_CTRL) & ~(1 << 15),
		LEGACY_CTRL);

	  /* enable 8 channel decode on CHAN B */
	  OUTB (devc->osdev, INB (devc->osdev, MISC2_CTRL) | 0x20,
		MISC2_CTRL);
	  /* Set NXCNG */
	  OUTL (devc->osdev, INL (devc->osdev, LEGACY_CTRL) | (0x80000000),
		LEGACY_CTRL);
	}

      /* Now set the buffer address/sizes */
      OUTL (devc->osdev, dmap->dmabuf_phys, CH1_FRAME1);
      OUTW (devc->osdev, (dmap->bytes_in_use >> portc->dacfmt) - 1,
	    CH1_FRAME2);
      OUTW (devc->osdev, (dmap->fragment_size >> portc->dacfmt),
	    CH1_FRAME2 + 2);


      /* set channel 1 to play mode */
      OUTB (devc->osdev, INB (devc->osdev, FUNCTRL0) & CM_CH1_PLAY, FUNCTRL0);
      portc->chan1_play = 1;

      /* setup spdif output on CHAN B , disable CHAN A spdif */
      if (devc->spdif_enabled)
	{
	  OUTL (devc->osdev, INL (devc->osdev, FUNCTRL1) | (1 << 9),
		FUNCTRL1);
	  OUTL (devc->osdev, INL (devc->osdev, FUNCTRL1) & ~(1 << 8),
		FUNCTRL1);
	}
    }
}

/*ARGSUSED*/
static int
cmpci_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  cmpci_devc *devc = audio_engines[dev]->devc;
  cmpci_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if ((portc->bits == AFMT_AC3) && devc->can_ac3)
    {
      portc->bits = 16;
      portc->channels = 2;
      cmpci_outsw (devc->mixer_dev, SPDIF_PLAY, SNDCTL_MIX_WRITE, 1);
      setup_ac3 (devc, portc, 1);
    }
  else
    setup_ac3 (devc, portc, 0);

  switch (devc->dev_mode)
    {
    case DEFAULT_MODE:
      /* set speed */
      set_dac_rate (dev, CHAN1);
      /* set format */
      set_dac_fmt (dev, CHAN1);
      /* set buffer address/size and other setups */
      setup_play (dev, CHAN1);
      break;

    case DUALDAC_MODE:
      if (dev == devc->portc[0].audiodev)
	{
	  set_dac_rate (dev, CHAN1);
	  set_dac_fmt (dev, CHAN1);
	  setup_play (dev, CHAN1);
	  setup_ac3 (devc, portc, 0);
	}
      if (dev == devc->portc[1].audiodev)
	{
	  set_dac_rate (dev, CHAN0);
	  set_dac_fmt (dev, CHAN0);
	  setup_play (dev, CHAN0);
	  setup_ac3 (devc, portc, 0);
	}
      break;

    case SPDIFIN_MODE:
      set_dac_rate (dev, CHAN0);
      set_dac_fmt (dev, CHAN0);
      setup_play (dev, CHAN0);
      setup_ac3 (devc, portc, 0);
      break;
    }

  portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

static int
cmpci_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  cmpci_devc *devc = audio_engines[dev]->devc;
  cmpci_portc *portc = audio_engines[dev]->portc;
  unsigned int ptr = 0;
  oss_native_word flags;

  if (!(portc->open_mode & direction))
    return 0;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  if (direction == PCM_ENABLE_INPUT)
    {
      if (portc->chan0_rec)
	ptr = INW (devc->osdev, CH0_FRAME1);
      if (portc->chan1_rec)
	ptr = INW (devc->osdev, CH1_FRAME1);
    }

  if (direction == PCM_ENABLE_OUTPUT)
    {
      if (portc->chan0_play)
	ptr = INW (devc->osdev, CH0_FRAME1);
      if (portc->chan1_play)
	ptr = INW (devc->osdev, CH1_FRAME1);
    }
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return ptr % dmap->bytes_in_use;
}

static audiodrv_t cmpci_audio_driver = {
  cmpci_audio_open,
  cmpci_audio_close,
  cmpci_audio_output_block,
  cmpci_audio_start_input,
  cmpci_audio_ioctl,
  cmpci_audio_prepare_for_input,
  cmpci_audio_prepare_for_output,
  cmpci_audio_reset,
  NULL,
  NULL,
  cmpci_audio_reset_input,
  cmpci_audio_reset_output,
  cmpci_audio_trigger,
  cmpci_audio_set_rate,
  cmpci_audio_set_format,
  cmpci_audio_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,				/* cmpci_alloc_buffer, */
  NULL,				/* cmpci_free_buffer, */
  NULL,
  NULL,
  cmpci_get_buffer_pointer
};

#ifdef OBSOLETED_STUFF
static void
attach_mpu (cmpci_devc * devc)
{
  unsigned int base;

  if (devc->chiprev == 33)
    base = 0x330;		/* Chiprev033 doen't have bas+0x40 */
  else
    base = MPU_MIRROR;		/* base+0x40 is MPU PCI mirror */

  uart401_init (&devc->uart401devc, devc->osdev, base, "Cmedia MIDI UART");
  devc->uart401_attached = 1;
}
#endif

static int
init_cmpci (cmpci_devc * devc)
{
  oss_native_word val;
  int first_dev = 0;
  int i;

  devc->fm_attached = 0;

/*
 * Enable BusMasterMode and IOSpace Access
 */
  /* Check the model number of the chip */
  val = INL (devc->osdev, INT_HLDCLR) & 0xff000000;

  if (!val)
    {
      val = INL (devc->osdev, CHFORMAT) & 0x1f000000;
      if (!val)
	{
	  devc->chiprev = 33;
	  devc->can_ac3 = 0;
	  devc->max_channels = 6;
	}
      else
	{
	  devc->chiprev = 37;
	  devc->can_ac3 = 1;
	  devc->max_channels = 6;
	}
    }
  else
    {
      if (val & 0x04000000)
	{
	  devc->chiprev = 39;
	  devc->can_ac3 = 1;
	  devc->max_channels = 6;
	}
      if (val & 0x08000000)
	{
	  devc->chiprev = 55;
	  devc->can_ac3 = 1;
	  devc->max_channels = 6;
	}
      if (val & 0x28000000)
	{
	  devc->chiprev = 68;
	  devc->can_ac3 = 1;
	  devc->model = MDL_CM8768;
	  devc->max_channels = 8;
	  devc->chip_name = "CMedia CM8768";
	}
    }

  /* enable uart, joystick in Function Control Reg1 */
  OUTB (devc->osdev, INB (devc->osdev, FUNCTRL1) | 0x06, FUNCTRL1);
  /* enable FM */
  OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) | (1 << 19), MISC_CTRL);
  OUTB (devc->osdev, 0, INT_HLDCLR + 2);	/* disable ints */
  OUTB (devc->osdev, 0, FUNCTRL0 + 2);	/* reset channels */

#ifdef OBSOLETED_STUFF
  attach_mpu (devc);
#endif

  /* install the CMPCI mixer */
  if ((devc->mixer_dev = oss_install_mixer (OSS_MIXER_DRIVER_VERSION,
					    devc->osdev,
					    devc->osdev,
					    "CMedia CMPCI",
					    &cmpci_mixer_driver,
					    sizeof (mixer_driver_t),
					    devc)) < 0)
    {
      return 0;
    }

  mixer_devs[devc->mixer_dev]->hw_devc = devc;
  mixer_devs[devc->mixer_dev]->priority = 1;	/* Possible default mixer candidate */

  cmpci_mixer_reset (devc->mixer_dev);
  mixer_ext_set_init_fn (devc->mixer_dev, cmpci_mix_init, 25);
  OUTB (devc->osdev, 0xF, MIXER2);

  /* setup 4speaker output */
  OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) | (1 << 26), MISC_CTRL);
  /* enable subwoofer/center channel */
  OUTL (devc->osdev, INL (devc->osdev, LEGACY_CTRL) | (1 << 12), LEGACY_CTRL);
  OUTL (devc->osdev, INL (devc->osdev, MISC_CTRL) | (1 << 7), MISC_CTRL);

  for (i = 0; i < MAX_PORTC; i++)
    {
      char tmp_name[100];
      cmpci_portc *portc = &devc->portc[i];
      int caps = ADEV_AUTOMODE;

      if (i == 0)
	{
	  sprintf (tmp_name, "%s (rev %0d)", devc->chip_name, devc->chiprev);
	  caps |= ADEV_DUPLEX;
	}
      else
	{
	  sprintf (tmp_name, "%s (playback only)", devc->chip_name);
	  caps |= ADEV_NOINPUT;
	}
      if ((portc->audiodev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
						   devc->osdev,
						   devc->osdev,
						   tmp_name,
						   &cmpci_audio_driver,
						   sizeof (audiodrv_t),
						   caps,
						   AFMT_U8 | AFMT_S16_LE |
						   AFMT_AC3, devc, -1)) < 0)
	{
	  return 0;
	}
      else
	{
	  if (i == 0)
	    first_dev = portc->audiodev;
	  audio_engines[portc->audiodev]->portc = portc;
	  audio_engines[portc->audiodev]->rate_source = first_dev;
	  audio_engines[portc->audiodev]->caps =
	    PCM_CAP_ANALOGOUT | PCM_CAP_ANALOGIN | PCM_CAP_DIGITALOUT |
	    PCM_CAP_DIGITALIN;
	  audio_engines[portc->audiodev]->min_rate = 5000;
	  audio_engines[portc->audiodev]->max_rate = 48000;
	  audio_engines[portc->audiodev]->caps |= PCM_CAP_FREERATE;
	  audio_engines[portc->audiodev]->min_channels = 2;
	  audio_engines[portc->audiodev]->max_channels = devc->max_channels;
	  audio_engines[portc->audiodev]->vmix_flags = VMIX_MULTIFRAG;
	  audio_engines[portc->audiodev]->dmabuf_alloc_flags |=
	    DMABUF_SIZE_16BITS;
	  portc->open_mode = 0;
	  portc->audio_enabled = 0;
          audio_engines[portc->audiodev]->mixer_dev = devc->mixer_dev;
          devc->dev_mode = DEFAULT_MODE;
          devc->spdif_enabled = 0;
#ifdef CONFIG_OSS_VMIX
	  if (i == 0)
	     vmix_attach_audiodev(devc->osdev, portc->audiodev, -1, 0);
#endif
	}
    }
  return 1;
}

int
oss_cmpci_attach (oss_device_t * osdev)
{
  unsigned char pci_irq_line, pci_revision;
  unsigned short pci_command, vendor, device;
  unsigned int pci_ioaddr;
  int err;
  cmpci_devc *devc;

  DDB (cmn_err (CE_CONT, "Entered CMEDIA CMPCI attach routine\n"));
  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  if (vendor != CMEDIA_VENDOR_ID
      || ((device != CMEDIA_CM8738) && (device != CMEDIA_CM8338A)
	  && (device != CMEDIA_CM8338B)))
    return 0;

  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_0, &pci_ioaddr);

  DDB (cmn_err (CE_WARN, "CMPCI I/O base %04x\n", pci_ioaddr));

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

  /* Map the IO Base address */
  devc->base = MAP_PCI_IOADDR (devc->osdev, 0, pci_ioaddr);

  /* Remove I/O space marker in bit 0. */
  devc->base &= ~3;

  /* set the PCI_COMMAND register to master mode */
  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_IO;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  switch (device)
    {
    case CMEDIA_CM8738:
      devc->model = MDL_CM8738;
      devc->chip_name = "CMedia CM8738";
      devc->max_channels = 6;
      break;

    case CMEDIA_CM8338A:
      devc->model = MDL_CM8338A;
      devc->chip_name = "CMedia CM8338A";
      devc->max_channels = 6;
      break;

    case CMEDIA_CM8338B:
      devc->model = MDL_CM8338B;
      devc->chip_name = "CMedia CM8338B";
      devc->max_channels = 6;
      break;
    }

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);

  oss_register_device (osdev, devc->chip_name);

  if ((err = oss_register_interrupts (devc->osdev, 0, cmpciintr, NULL)) < 0)
    {
      cmn_err (CE_WARN, "Can't allocate IRQ%d, err=%d\n", pci_irq_line, err);
      return 0;
    }

  return init_cmpci (devc);	/* Detected */
}

int
oss_cmpci_detach (oss_device_t * osdev)
{
  cmpci_devc *devc = (cmpci_devc *) osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  /* disable Interrupts */
  OUTB (devc->osdev, 0, INT_HLDCLR + 2);

  /* disable channels */
  OUTB (devc->osdev, 0, FUNCTRL0 + 2);

  /* uninstall UART401 */
  if (devc->uart401_attached)
    uart401_disable (&devc->uart401devc);

  oss_unregister_interrupts (devc->osdev);

  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);
  UNMAP_PCI_IOADDR (devc->osdev, 0);

  oss_unregister_device (osdev);
  return 1;
}
