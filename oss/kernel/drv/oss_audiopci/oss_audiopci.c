/* 
 * Purpose: Creative/Ensoniq AudioPCI driver (ES1370 "CONCERT" ASIC and AKM4531 codec/mixer)
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

#include "oss_audiopci_cfg.h"
#include "audiopci.h"
#include "midi_core.h"
#include "oss_pci.h"

#define ENSONIQ_VENDOR_ID	0x1274
#define ENSONIQ_VENDOR_ID2	0x1275
#define ENSONIQ_AUDIOPCI	0x5000

#define MAX_PORTC 2

typedef struct apci_portc
{

  /* Audio parameters */
  int audiodev;
  int open_mode;
  int trigger_bits;
  int audio_enabled;
  int speed, bits, channels;
  int atype;			/* 0=DAC/ADC, 1=Synth */
  int speedsel;
} apci_portc;

typedef struct apci_devc
{
  oss_device_t *osdev;
  oss_mutex_t mutex, low_mutex;
  oss_native_word base;
  int irq;
  char *chip_name;

  /* Mixer parameters */
  int *levels;
  unsigned char ak_regs[0x20];	/* Current mixer register values */
  int recdevs;
  int micbias, micboost;
  unsigned char outsw1, outsw2;

  /* Audio parameters */
  int irq_allocated;
  apci_portc portc[MAX_PORTC];

/*
 * MIDI
 */
  int midi_opened;
  int midi_dev;
  oss_midi_inputbyte_t midi_input_intr;
} apci_devc;


/*
 * Initial values to be written into the mixer registers of AK4531 codec.
 */
static const unsigned char ak_reg_init[0x20] = {
  /* Mute all inputs/outputs initially */
  0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,	/* 00 to 07 */
  0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,	/* 08 to 0f */
  0x7f, 0x3d, 0x55, 0x26, 0xf7, 0xef, 0x03, 0x00,	/* 10 to 17 */
  0x00, 0x01			/* 18 to 19 */
};

static void
ak_write (apci_devc * devc, int reg, int value)
{
  int i;

  if (reg < 0 || reg > 0x19)
    return;

  value &= 0xff;
  devc->ak_regs[reg] = (unsigned char) value;

  /* Wait until the codec is ready */
  for (i = 0; i < 0x40000; i++)
    if (!(INB (devc->osdev, devc->base + CONC_bCODECSTAT_OFF) & 0x01))
      break;
  oss_udelay (10);
  OUTW (devc->osdev, (reg << 8) | value, devc->base + CONC_wCODECCTL_OFF);
  oss_udelay (10);
}

static void
apci_writemem (apci_devc * devc, int page, int offs, int data)
{
  int tmp;

  tmp = INL (devc->osdev, devc->base + 0xc);
  OUTL (devc->osdev, page, devc->base + 0xc);	/* Select memory page */
  OUTL (devc->osdev, data, devc->base + offs);
  OUTL (devc->osdev, tmp, devc->base + 0xc);	/* Select the original memory page */
}

static unsigned int
apci_readmem (apci_devc * devc, int page, int offs)
{
  unsigned int val;

  OUTL (devc->osdev, page, devc->base + 0xc);	/* Select memory page */
  val = INL (devc->osdev, devc->base + offs);
  return val;
}

#define bmast_off(x)
#define bmast_on(x)

static int
apci_set_recmask (apci_devc * devc, int mask)
{
  unsigned char tmp;

  mask &= REC_DEVS;

/*
 * Set lch input mixer SW 1 register
 */
  tmp = 0;
  if (mask & SOUND_MASK_ALTPCM)
    tmp |= 0x40;
  if (mask & SOUND_MASK_LINE)
    tmp |= 0x10;
  if (mask & SOUND_MASK_CD)
    tmp |= 0x04;
  if (mask & SOUND_MASK_MIC)
    tmp |= 0x01;
  ak_write (devc, 0x12, tmp);

/*
 * Set rch input mixer SW 1 register
 */
  tmp = 0;
  if (mask & SOUND_MASK_ALTPCM)
    tmp |= 0x20;
  if (mask & SOUND_MASK_LINE)
    tmp |= 0x08;
  if (mask & SOUND_MASK_CD)
    tmp |= 0x02;
  if (mask & SOUND_MASK_MIC)
    tmp |= 0x01;
  ak_write (devc, 0x13, tmp);

/*
 * Set lch input mixer SW 2 register
 */
  tmp = 0;
  if (mask & SOUND_MASK_LINE2)
    tmp |= 0x40;
  if (mask & SOUND_MASK_LINE3)
    tmp |= 0x20;
  if (mask & SOUND_MASK_LINE1)
    tmp |= 0x10;
  if (mask & SOUND_MASK_MIC)
    tmp |= 0x80;
  ak_write (devc, 0x14, tmp);

/*
 * Set rch input mixer SW 2 register
 */
  tmp = 0;
  if (mask & SOUND_MASK_LINE2)
    tmp |= 0x40;
  if (mask & SOUND_MASK_LINE3)
    tmp |= 0x20;
  if (mask & SOUND_MASK_LINE1)
    tmp |= 0x08;
  if (mask & SOUND_MASK_MIC)
    tmp |= 0x80;
  ak_write (devc, 0x15, tmp);

  return devc->recdevs = mask;
}

/*ARGSUSED*/
static void
change_bits (apci_devc * devc, unsigned char *regval, int dev, int chn,
	     int newval)
{
  unsigned char mask;
  int shift;
  int mute;
  int mutemask;
  int set_mute_bit;

  set_mute_bit = (newval == 0);

  if (ak_mix_devices[dev][chn].polarity == 1)	/* Reverse */
    newval = 100 - newval;

  mask = (1 << ak_mix_devices[dev][chn].nbits) - 1;
  shift = ak_mix_devices[dev][chn].bitpos;

#if 0
  newval = (int) ((newval * mask) + 50) / 100;	/* Scale it */
  *regval &= ~(mask << shift);	/* Clear bits */
  *regval |= (newval & mask) << shift;	/* Set new value */
#else
  if (ak_mix_devices[dev][chn].mutepos == 8)
    {				/* if there is no mute bit */
      mute = 0;			/* No mute bit; do nothing special */
      mutemask = ~0;		/* No mute bit; do nothing special */
    }
  else
    {
      mute = (set_mute_bit << ak_mix_devices[dev][chn].mutepos);
      mutemask = ~(1 << ak_mix_devices[dev][chn].mutepos);
    }

  newval = (int) ((newval * mask) + 50) / 100;	/* Scale it */
  *regval &= (~(mask << shift)) & (mutemask);	/* Clear bits */
  *regval |= ((newval & mask) << shift) | mute;	/* Set new value */
#endif
}

static int
apci_mixer_get (apci_devc * devc, int dev)
{
  if (!((1 << dev) & MIXER_DEVS))
    return OSS_EINVAL;

  return devc->levels[dev];
}

static int
apci_mixer_set (apci_devc * devc, int dev, int value)
{
  int left = value & 0x000000ff;
  int right = (value & 0x0000ff00) >> 8;
  int retvol;

  int regoffs;
  unsigned char val;

  if (dev > 31)
    return OSS_EINVAL;

  if (!(MIXER_DEVS & (1 << dev)))
    return OSS_EINVAL;

  if (left > 100)
    left = 100;
  if (right > 100)
    right = 100;

  if (ak_mix_devices[dev][RIGHT_CHN].regno == 0xff)	/* Mono control */
    right = left;

  retvol = left | (right << 8);

#if 1
  /* Scale volumes */
  left = mix_cvt[left];
  right = mix_cvt[right];

  /* Scale it again */
  left = mix_cvt[left];
  right = mix_cvt[right];
#endif

  if (ak_mix_devices[dev][LEFT_CHN].regno == 0xff)
    return OSS_EINVAL;

  devc->levels[dev] = retvol;

  /*
   * Set the left channel
   */

  regoffs = ak_mix_devices[dev][LEFT_CHN].regno;
  val = 0;
  change_bits (devc, &val, dev, LEFT_CHN, left);
  ak_write (devc, regoffs, val);

  /*
   * Set the right channel
   */

  if (ak_mix_devices[dev][RIGHT_CHN].regno == 0xff)
    return retvol;		/* Was just a mono channel */

  regoffs = ak_mix_devices[dev][RIGHT_CHN].regno;
  val = 0;
  change_bits (devc, &val, dev, RIGHT_CHN, right);
  ak_write (devc, regoffs, val);

  return retvol;
}

static void
apci_mixer_reset (apci_devc * devc)
{
  int i;

  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
    if (MIXER_DEVS & (1 << i))
      apci_mixer_set (devc, i, devc->levels[i]);
  apci_set_recmask (devc, SOUND_MASK_MIC);
  devc->outsw1 = ak_reg_init[0x10];
  devc->outsw2 = ak_reg_init[0x11];
}

/*ARGSUSED*/
static int
apci_mixer_ioctl (int dev, int audiodev, unsigned int cmd, ioctl_arg arg)
{
  apci_devc *devc = mixer_devs[dev]->devc;

  if (((cmd >> 8) & 0xff) == 'M')
    {
      int val;

      if (IOC_IS_OUTPUT (cmd))
	switch (cmd & 0xff)
	  {
	  case SOUND_MIXER_RECSRC:
	    val = *arg;
	    return *arg = apci_set_recmask (devc, val);
	    break;

	  default:
	    val = *arg;
	    return *arg = apci_mixer_set (devc, cmd & 0xff, val);
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
	    return *arg = apci_mixer_get (devc, cmd & 0xff);
	  }
    }
  else
    return OSS_EINVAL;
}

/*ARGSUSED*/
static int
getmute (apci_devc * devc, int offs, unsigned char bits, int value)
{
  unsigned char tmp;

  tmp = (offs == 0x10) ? devc->outsw1 : devc->outsw2;

  tmp &= bits;
  return (tmp == 0);		/* Note! inverted polarity */
}

static int
setmute (apci_devc * devc, int offs, unsigned char bits, int value)
{
  unsigned char tmp;

  value = !value;		/* Inverted polarity (now 0=mute) */

  tmp = (offs == 0x10) ? devc->outsw1 : devc->outsw2;

  tmp &= ~bits;			/* Mask old bits */
  if (value)
    tmp |= bits;

  ak_write (devc, offs, tmp);
  if (offs == 0x10)
    devc->outsw1 = tmp;
  else
    devc->outsw2 = tmp;

  return !value;
}

static int
apci_outsw (int dev, int ctrl, unsigned int cmd, int value)
{
/*
 * Access function for AudioPCI mixer extension bits
 */
  apci_devc *devc = mixer_devs[dev]->devc;

  if (cmd == SNDCTL_MIX_READ)
    {
      value = 0;
      switch (ctrl)
	{
	case 1:		/* 20 dB microphone boost */
	  value = devc->micboost;
	  break;

	case 2:		/* Microphone phantom power */
	  value = devc->micbias;
	  break;

	case 10:		/* Pcm mute */
	  value = getmute (devc, 0x11, 0x0c, value);
	  break;

	case 11:		/* Pcm2 mute */
	  value = getmute (devc, 0x10, 0x60, value);
	  break;

	case 12:		/* Mic mute */
	  value = getmute (devc, 0x10, 0x01, value);
	  break;

	case 13:		/* CD mute */
	  value = getmute (devc, 0x10, 0x06, value);
	  break;

	case 14:		/* Line mute */
	  value = getmute (devc, 0x10, 0x18, value);
	  break;

	case 15:		/* Line1 mute */
	  value = getmute (devc, 0x11, 0x30, value);
	  break;

	case 16:		/* Line2 mute */
	  value = getmute (devc, 0x11, 0x01, value);
	  break;

	case 17:		/* Line3 mute */
	  value = getmute (devc, 0x11, 0x02, value);
	  break;

	case 18:		/*Separate output enable for the synth device (XCTL0) */
	  value =
	    ((INB (devc->osdev, devc->base + CONC_bMISCCTL_OFF) & 0x01) != 0);
	  break;

	default:
	  return OSS_EINVAL;
	}

      return value;
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
      int tmp;

      if (value)
	value = 1;

      switch (ctrl)
	{
	case 1:		/* 20 dB microphone boost */
	  devc->micboost = value;
	  ak_write (devc, 0x19, value);
	  break;

	case 2:		/* Microphone phantom power */
	  devc->micbias = value;
	  /* Delay the actual change until next recording */
	  break;

	case 10:		/* Pcm mute */
	  value = setmute (devc, 0x11, 0x0c, value);
	  break;

	case 11:		/* Pcm2 mute */
	  value = setmute (devc, 0x10, 0x60, value);
	  break;

	case 12:		/* Mic mute */
	  value = setmute (devc, 0x10, 0x01, value);
	  break;

	case 13:		/* CD mute */
	  value = setmute (devc, 0x10, 0x06, value);
	  break;

	case 14:		/* Line mute */
	  value = setmute (devc, 0x10, 0x18, value);
	  break;

	case 15:		/* Line1 mute */
	  value = setmute (devc, 0x11, 0x30, value);
	  break;

	case 16:		/* Line2 mute */
	  value = setmute (devc, 0x11, 0x01, value);
	  break;

	case 17:		/* Line3 mute */
	  value = setmute (devc, 0x11, 0x02, value);
	  break;

	case 18:		/*Separate output enable for the synth device (XCTL0) */
	  tmp = INB (devc->osdev, devc->base + CONC_bMISCCTL_OFF);
	  if (value)
	    {
	      OUTB (devc->osdev, tmp | 0x01, devc->base + CONC_bMISCCTL_OFF);
	    }
	  else
	    {
	      OUTB (devc->osdev, tmp & ~0x01, devc->base + CONC_bMISCCTL_OFF);
	    }
	  break;

	default:
	  return OSS_EINVAL;
	}

      return value;
    }

  return OSS_EINVAL;
}

static int
apci_mix_init (int dev)
{
  int group, err;

  if ((group = mixer_ext_create_group (dev, 0, "APCI_EXTMIC")) < 0)
    return group;

  if ((err = mixer_ext_create_control (dev, group, 1, apci_outsw, MIXT_ONOFF,
				       "APCI_MICBOOST", 1,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group, 2, apci_outsw, MIXT_ONOFF,
				       "APCI_MICBIAS", 1,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((group = mixer_ext_create_group (dev, 0, "APCI_MUTE")) < 0)
    return group;

  if ((err = mixer_ext_create_control (dev, group, 10, apci_outsw,
				       MIXT_ONOFF,
				       "APCI_PCMMUTE", 1,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group, 11, apci_outsw,
				       MIXT_ONOFF,
				       "APCI_PCM2MUTE", 1,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group, 12, apci_outsw,
				       MIXT_ONOFF,
				       "APCI_MICMUTE", 1,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group, 13, apci_outsw,
				       MIXT_ONOFF,
				       "APCI_CDMUTE", 1,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group, 14, apci_outsw,
				       MIXT_ONOFF,
				       "APCI_LINEMUTE", 1,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group, 15, apci_outsw,
				       MIXT_ONOFF,
				       "APCI_LINE1MUTE", 1,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group, 16, apci_outsw,
				       MIXT_ONOFF,
				       "APCI_LINE2MUTE", 1,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group, 17, apci_outsw,
				       MIXT_ONOFF,
				       "APCI_LINE3MUTE", 1,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((group = mixer_ext_create_group (dev, 0, "APCI_4CHAN")) < 0)
    return group;

  if ((err = mixer_ext_create_control (dev, group, 18, apci_outsw,
				       MIXT_ONOFF,
				       "APCI4CH_ENABLE", 1,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  return 0;
}

static mixer_driver_t apci_mixer_driver = {
  apci_mixer_ioctl
};

static int
apciintr (oss_device_t * osdev)
{
  int stats, i;
  unsigned char ackbits = 0, tmp;
  unsigned char uart_stat;
  apci_devc *devc = (apci_devc *) osdev->devc;
  apci_portc *portc;
  int serviced = 0;

  stats = INL (devc->osdev, devc->base + 0x04);

  if (!(stats & 0x80000000))	/* No interrupt pending */
    return 0;

  serviced = 1;
  for (i = 0; i < MAX_PORTC; i++)
    {
      portc = &devc->portc[i];

      if (stats & 0x00000010)	/* CCB interrupt */
	{
	  cmn_err (CE_WARN, "CCB interrupt\n");
	}

      if ((stats & 0x00000004) && (portc->atype))	/* DAC1 (synth) interrupt */
	{
	  ackbits |= CONC_SERCTL_SYNIE;
	  if (portc->trigger_bits & PCM_ENABLE_OUTPUT)
	    oss_audio_outputintr (portc->audiodev, 0);

	}

      if ((stats & 0x00000002) && (!portc->atype))	/* DAC2 interrupt */
	{
	  ackbits |= CONC_SERCTL_DACIE;
	  if (portc->trigger_bits & PCM_ENABLE_OUTPUT)
	    oss_audio_outputintr (portc->audiodev, 0);
	}

      if ((stats & 0x00000001) && (!portc->atype))	/* ADC interrupt */
	{
	  ackbits |= CONC_SERCTL_ADCIE;
	  if (portc->trigger_bits & PCM_ENABLE_INPUT)
	    oss_audio_inputintr (portc->audiodev, 0);
	}

      if (stats & 0x00000008)	/* UART interrupt */
	{
	  uart_stat = INB (devc->osdev, devc->base + CONC_bUARTCSTAT_OFF);

	  while (uart_stat & CONC_UART_RXRDY)
	    {
	      unsigned char d;

	      d = INB (devc->osdev, devc->base + CONC_bUARTDATA_OFF);

	      if (devc->midi_opened & OPEN_READ && devc->midi_input_intr)
		devc->midi_input_intr (devc->midi_dev, d);
	      uart_stat = INB (devc->osdev, devc->base + CONC_bUARTCSTAT_OFF);
	    }
	}

      tmp = INB (devc->osdev, devc->base + CONC_bSERCTL_OFF);
      OUTB (devc->osdev, (tmp & ~ackbits), devc->base + CONC_bSERCTL_OFF);	/* Clear bits */
      OUTB (devc->osdev, tmp | ackbits, devc->base + CONC_bSERCTL_OFF);	/* Return them back on */

    }
  return serviced;
}

/*
 * Audio routines
 */

static unsigned short
compute_dac2_rate (int samPerSec)
{

  unsigned short usTemp;

  /* samPerSec /= 2; */

  usTemp = (unsigned short) ((DAC_CLOCK_DIVIDE / 8) / samPerSec);

  if (usTemp & 0x00000001)
    {
      usTemp >>= 1;
      usTemp -= 1;
    }
  else
    {
      usTemp >>= 1;
      usTemp -= 2;
    }

  return usTemp;

}

static int
apci_audio_set_rate (int dev, int arg)
{
  apci_portc *portc = audio_engines[dev]->portc;

  int speeds[] = { 5512, 11025, 22050, 44100 };
  int i, n = 0, best = 1000000;


  if (arg == 0)
    return portc->speed;

  if (portc->atype)
    {
      if (arg > 44100)
	arg = 44100;
      if (arg < 5512)
	arg = 5512;

      for (i = 0; i < 4; i++)
	{
	  int diff = arg - speeds[i];

	  if (diff < 0)
	    diff *= -1;

	  if (diff < best)
	    {
	      n = i;
	      best = diff;
	    }
	}
      portc->speed = speeds[n];
      portc->speedsel = n;
    }
  else
    {
      if (arg > 48000)
	arg = 48000;
      if (arg < 5000)
	arg = 5000;
      portc->speed = arg;
    }
  return portc->speed;
}

static short
apci_audio_set_channels (int dev, short arg)
{
  apci_portc *portc = audio_engines[dev]->portc;

  if ((arg != 1) && (arg != 2))
    return portc->channels;
  portc->channels = arg;

  return portc->channels;
}

static unsigned int
apci_audio_set_format (int dev, unsigned int arg)
{
  apci_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->bits;

  if (!(arg & (AFMT_U8 | AFMT_S16_LE)))
    return portc->bits;
  portc->bits = arg;

  return portc->bits;
}

/*ARGSUSED*/
static int
apci_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void apci_audio_trigger (int dev, int state);

static void
apci_audio_reset (int dev)
{
  apci_audio_trigger (dev, 0);
}

static void
apci_audio_reset_input (int dev)
{
  apci_portc *portc = audio_engines[dev]->portc;
  apci_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
apci_audio_reset_output (int dev)
{
  apci_portc *portc = audio_engines[dev]->portc;
  apci_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

/*ARGSUSED*/
static int
apci_audio_open (int dev, int mode, int open_flags)
{
  apci_portc *portc = audio_engines[dev]->portc;
  apci_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;

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
apci_audio_close (int dev, int mode)
{
  apci_portc *portc = audio_engines[dev]->portc;

  apci_audio_reset (dev);
  portc->open_mode = 0;
  portc->audio_enabled &= ~mode;
}

/*ARGSUSED*/
static void
apci_audio_output_block (int dev, oss_native_word buf, int count,
			 int fragsize, int intrflag)
{
  apci_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;

}

/*ARGSUSED*/
static void
apci_audio_start_input (int dev, oss_native_word buf, int count,
			int fragsize, int intrflag)
{
  apci_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;

}

static void
apci_audio_trigger (int dev, int state)
{
  apci_devc *devc = audio_engines[dev]->devc;
  apci_portc *portc = audio_engines[dev]->portc;
  int tmp;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (portc->open_mode & OPEN_WRITE)
    {
      if (state & PCM_ENABLE_OUTPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      if (portc->atype)
		{
		  tmp = INB (devc->osdev, devc->base + CONC_bDEVCTL_OFF);
		  tmp |= CONC_DEVCTL_DAC1_EN;
		  OUTB (devc->osdev, tmp, devc->base + CONC_bDEVCTL_OFF);
		}
	      else
		{
		  tmp = INB (devc->osdev, devc->base + CONC_bDEVCTL_OFF);
		  tmp |= CONC_DEVCTL_DAC2_EN;
		  OUTB (devc->osdev, tmp, devc->base + CONC_bDEVCTL_OFF);
		}
	      portc->trigger_bits |= PCM_ENABLE_OUTPUT;
	      oss_udelay (50);
	    }
	}
      else
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      (portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
	      portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
	      if (portc->atype)
		{
		  tmp = INB (devc->osdev, devc->base + CONC_bDEVCTL_OFF);
		  tmp &= ~CONC_DEVCTL_DAC1_EN;
		  OUTB (devc->osdev, tmp, devc->base + CONC_bDEVCTL_OFF);

		  tmp = INB (devc->osdev, devc->base + CONC_bSERCTL_OFF);
		  tmp &= ~CONC_SERCTL_SYNIE;
		  OUTB (devc->osdev, tmp, devc->base + CONC_bSERCTL_OFF);
		}
	      else
		{
		  tmp = INB (devc->osdev, devc->base + CONC_bDEVCTL_OFF);
		  tmp &= ~CONC_DEVCTL_DAC2_EN;
		  OUTB (devc->osdev, tmp, devc->base + CONC_bDEVCTL_OFF);

		  tmp = INB (devc->osdev, devc->base + CONC_bSERCTL_OFF);
		  tmp &= ~CONC_SERCTL_DACIE;
		  OUTB (devc->osdev, tmp, devc->base + CONC_bSERCTL_OFF);
		}
	    }
	}
    }

  if ((portc->open_mode & OPEN_READ)
      && !(audio_engines[dev]->flags & ADEV_NOINPUT))
    {
      if (state & PCM_ENABLE_INPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_INPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_INPUT))
	    {
	      tmp = INB (devc->osdev, devc->base + CONC_bDEVCTL_OFF);
	      tmp |= CONC_DEVCTL_ADC_EN;
	      OUTB (devc->osdev, tmp, devc->base + CONC_bDEVCTL_OFF);
	      portc->trigger_bits |= PCM_ENABLE_INPUT;
	      oss_udelay (50);
	    }
	}
      else
	{
	  if ((portc->audio_enabled & PCM_ENABLE_INPUT) &&
	      (portc->trigger_bits & PCM_ENABLE_INPUT))
	    {
	      portc->audio_enabled &= ~PCM_ENABLE_INPUT;
	      portc->trigger_bits &= ~PCM_ENABLE_INPUT;

	      tmp = INB (devc->osdev, devc->base + CONC_bDEVCTL_OFF);
	      tmp &= ~CONC_DEVCTL_ADC_EN;
	      OUTB (devc->osdev, tmp, devc->base + CONC_bDEVCTL_OFF);

	      tmp = INB (devc->osdev, devc->base + CONC_bSERCTL_OFF);
	      tmp &= ~CONC_SERCTL_ADCIE;
	      OUTB (devc->osdev, tmp, devc->base + CONC_bSERCTL_OFF);
	    }
	}
    }

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
static int
apci_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  dmap_t *dmap = audio_engines[dev]->dmap_in;
  apci_devc *devc = audio_engines[dev]->devc;
  apci_portc *portc = audio_engines[dev]->portc;
  unsigned short tmp = 0x00;
  oss_native_word flags;

  /* Set physical address of the DMA buffer */

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  apci_writemem (devc, CONC_ADCCTL_PAGE, CONC_dADCPADDR_OFF,
		 dmap->dmabuf_phys);

  /* Set DAC (ADC) rate */
  OUTW (devc->osdev, compute_dac2_rate (portc->speed),
	devc->base + CONC_wDACRATE_OFF);

  /* Set format */
  tmp = INB (devc->osdev, devc->base + CONC_bSERFMT_OFF);
  tmp &= ~(CONC_PCM_ADC_STEREO | CONC_PCM_ADC_16BIT);
  if (portc->channels == 2)
    tmp |= CONC_PCM_ADC_STEREO;
  if (portc->bits == 16)
    {
      tmp |= CONC_PCM_ADC_16BIT;
      OUTB (devc->osdev, 0x10, devc->base + CONC_bSKIPC_OFF);	/* Skip count register */
    }
  else
    {
      OUTB (devc->osdev, 0x08, devc->base + CONC_bSKIPC_OFF);	/* Skip count register */
    }
  OUTB (devc->osdev, tmp, devc->base + CONC_bSERFMT_OFF);

  /* Set the frame count */
  apci_writemem (devc, CONC_ADCCTL_PAGE, CONC_wADCFC_OFF,
		 (dmap->bytes_in_use / 4) - 1);

  /* Set # of samples between interrupts */
  OUTW (devc->osdev,
	(dmap->fragment_size / ((portc->channels * portc->bits) / 8)) - 1,
	devc->base + CONC_wADCIC_OFF);

  /* Enable the wave interrupt */
  tmp = INB (devc->osdev, devc->base + CONC_bSERCTL_OFF) & ~CONC_SERCTL_ADCIE;
  OUTB (devc->osdev, tmp, devc->base + CONC_bSERCTL_OFF);
  tmp |= CONC_SERCTL_ADCIE;
  OUTB (devc->osdev, tmp, devc->base + CONC_bSERCTL_OFF);
  OUTB (devc->osdev, tmp, devc->base + CONC_bSERCTL_OFF);

  /* Enable microphone phantom power */
  tmp = INW (devc->osdev, devc->base + 2) & ~CONC_DEVCTL_MICBIAS;
  if (devc->micbias)
    tmp |= CONC_DEVCTL_MICBIAS;
  OUTW (devc->osdev, tmp, devc->base + 2);

  portc->audio_enabled &= ~PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/
static int
apci_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  dmap_t *dmap = audio_engines[dev]->dmap_out;
  unsigned char tmp = 0x00;
  apci_devc *devc = audio_engines[dev]->devc;
  apci_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->atype)
    {
      /* Set physical address of the DMA buffer */
      apci_writemem (devc, CONC_SYNCTL_PAGE, CONC_dSYNPADDR_OFF,
		     dmap->dmabuf_phys);

      /* Set DAC1 rate */
      tmp = INB (devc->osdev, devc->base + CONC_bMISCCTL_OFF) & ~0x30;
      tmp |= portc->speedsel << 4;
      OUTB (devc->osdev, tmp, devc->base + CONC_bMISCCTL_OFF);

      /* Set format */
      tmp = INB (devc->osdev, devc->base + CONC_bSERFMT_OFF);
      tmp &= ~(CONC_PCM_DAC1_STEREO | CONC_PCM_DAC1_16BIT);
      if (portc->channels == 2)
	tmp |= CONC_PCM_DAC1_STEREO;
      if (portc->bits == 16)
	{
	  tmp |= CONC_PCM_DAC1_16BIT;
	}
      OUTB (devc->osdev, tmp, devc->base + CONC_bSERFMT_OFF);

      /* Set the frame count */
      apci_writemem (devc, CONC_SYNCTL_PAGE, CONC_wSYNFC_OFF,
		     (dmap->bytes_in_use / 4) - 1);

      /* Set # of samples between interrupts */
      OUTW (devc->osdev,
	    (dmap->fragment_size / ((portc->channels * portc->bits) / 8)) - 1,
	    devc->base + CONC_wSYNIC_OFF);

      /* Enable the wave interrupt */
      tmp =
	INB (devc->osdev, devc->base + CONC_bSERCTL_OFF) & ~CONC_SERCTL_SYNIE;
      OUTB (devc->osdev, tmp, devc->base + CONC_bSERCTL_OFF);
      tmp |= CONC_SERCTL_SYNIE;
      OUTB (devc->osdev, tmp, devc->base + CONC_bSERCTL_OFF);
    }
  else
    {
      /* Set physical address of the DMA buffer */
      apci_writemem (devc, CONC_DACCTL_PAGE, CONC_dDACPADDR_OFF,
		     dmap->dmabuf_phys);

      /* Set DAC rate */
      OUTW (devc->osdev, compute_dac2_rate (portc->speed),
	    devc->base + CONC_wDACRATE_OFF);

      /* Set format */
      tmp = INB (devc->osdev, devc->base + CONC_bSERFMT_OFF);
      tmp &= ~(CONC_PCM_DAC2_STEREO | CONC_PCM_DAC2_16BIT);
      if (portc->channels == 2)
	tmp |= CONC_PCM_DAC2_STEREO;
      if (portc->bits == 16)
	{
	  tmp |= CONC_PCM_DAC2_16BIT;
	  OUTB (devc->osdev, 0x10, devc->base + CONC_bSKIPC_OFF);	/* Skip count register */
	}
      else
	{
	  OUTB (devc->osdev, 0x08, devc->base + CONC_bSKIPC_OFF);	/* Skip count register */
	}
      OUTB (devc->osdev, tmp, devc->base + CONC_bSERFMT_OFF);

      /* Set the frame count */
      apci_writemem (devc, CONC_DACCTL_PAGE, CONC_wDACFC_OFF,
		     (dmap->bytes_in_use / 4) - 1);
      apci_writemem (devc, CONC_DACCTL_PAGE, CONC_wDACFC_OFF,
		     (dmap->bytes_in_use / 4) - 1);

      /* Set # of samples between interrupts */
      OUTW (devc->osdev,
	    (dmap->fragment_size / ((portc->channels * portc->bits) / 8)) - 1,
	    devc->base + CONC_wDACIC_OFF);

      /* Enable the wave interrupt */
      tmp =
	INB (devc->osdev, devc->base + CONC_bSERCTL_OFF) & ~CONC_SERCTL_DACIE;
      OUTB (devc->osdev, tmp, devc->base + CONC_bSERCTL_OFF);
      tmp |= CONC_SERCTL_DACIE;
      OUTB (devc->osdev, tmp, devc->base + CONC_bSERCTL_OFF);
    }

  portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/
static int
apci_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  apci_devc *devc = audio_engines[dev]->devc;
  apci_portc *portc = audio_engines[dev]->portc;
  int ptr = 0, port = 0, page = 0;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  if (direction == PCM_ENABLE_OUTPUT)
    {
      if (portc->atype)
	{
	  port = CONC_wSYNFC_OFF;
	  page = CONC_SYNCTL_PAGE;
	}
      else
	{
	  port = CONC_wDACFC_OFF;
	  page = CONC_DACCTL_PAGE;
	}
    }

  if (direction == PCM_ENABLE_INPUT)
    {
      port = CONC_wADCFC_OFF;
      page = CONC_ADCCTL_PAGE;
    }

  ptr = apci_readmem (devc, page, port);
  ptr >>= 16;
  ptr <<= 2;			/* count is in dwords */
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return ptr;
}


static const audiodrv_t apci_audio_driver = {
  apci_audio_open,
  apci_audio_close,
  apci_audio_output_block,
  apci_audio_start_input,
  apci_audio_ioctl,
  apci_audio_prepare_for_input,
  apci_audio_prepare_for_output,
  apci_audio_reset,
  NULL,
  NULL,
  apci_audio_reset_input,
  apci_audio_reset_output,
  apci_audio_trigger,
  apci_audio_set_rate,
  apci_audio_set_format,
  apci_audio_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,				/* apci_alloc_buffer */
  NULL,				/* apci_free_buffer */
  NULL,
  NULL,
  apci_get_buffer_pointer
};

/*ARGSUSED*/
static int
apci_midi_open (int dev, int mode, oss_midi_inputbyte_t inputbyte,
		oss_midi_inputbuf_t inputbuf,
		oss_midi_outputintr_t outputintr)
{
  apci_devc *devc = (apci_devc *) midi_devs[dev]->devc;

  if (devc->midi_opened)
    {
      return OSS_EBUSY;
    }

  devc->midi_input_intr = inputbyte;
  devc->midi_opened = mode;

  if (mode & OPEN_READ)
    {
      OUTB (devc->osdev, CONC_UART_RXINTEN, devc->base + CONC_bUARTCSTAT_OFF);
    }

  return 0;
}

/*ARGSUSED*/
static void
apci_midi_close (int dev, int mode)
{
  apci_devc *devc = (apci_devc *) midi_devs[dev]->devc;

  OUTB (devc->osdev, 0x00, devc->base + CONC_bUARTCSTAT_OFF);
  devc->midi_opened = 0;
}

static int
apci_midi_out (int dev, unsigned char midi_byte)
{
  apci_devc *devc = (apci_devc *) midi_devs[dev]->devc;
  int i;

  unsigned char uart_stat =
    INB (devc->osdev, devc->base + CONC_bUARTCSTAT_OFF);

  bmast_off (devc);
  for (i = 0; i < 30000; i++)
    {
      uart_stat = INB (devc->osdev, devc->base + CONC_bUARTCSTAT_OFF);
      if (uart_stat & CONC_UART_TXRDY)
	break;
    }

  if (!(uart_stat & CONC_UART_TXRDY))
    {
      bmast_on (devc);
      return 0;
    }


  OUTB (devc->osdev, midi_byte, devc->base + CONC_bUARTDATA_OFF);
  bmast_on (devc);

  return 1;
}

/*ARGSUSED*/
static int
apci_midi_ioctl (int dev, unsigned cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static midi_driver_t apci_midi_driver = {
  apci_midi_open,
  apci_midi_close,
  apci_midi_ioctl,
  apci_midi_out,
};

static int
init_apci (apci_devc * devc)
{
  int i, my_mixer, tmp;

  devc->micbias = 1;
  devc->micboost = 1;

  OUTW (devc->osdev, compute_dac2_rate (8000),
	devc->base + CONC_wDACRATE_OFF);

  tmp =
    INB (devc->osdev,
	 devc->base + CONC_bMISCCTL_OFF) & ~CONC_MISCTL_CCB_INTRM;
  tmp |= CONC_MISCTL_MUTE | CONC_MISCTL_DAC1FREQ_2205;
  OUTB (devc->osdev, tmp, devc->base + CONC_bMISCCTL_OFF);

  /* Turn on UART, CODEC and joystick. Disable SERR. */
  tmp = INB (devc->osdev, devc->base + CONC_bDEVCTL_OFF) & ~CONC_DEVCTL_SERR_DISABLE;	/* Yes */
  tmp |= CONC_DEVCTL_UART_EN | CONC_DEVCTL_CODEC_EN | CONC_DEVCTL_JSTICK_EN;
  OUTB (devc->osdev, tmp, devc->base + CONC_bDEVCTL_OFF);

  /* Disable NMI */
  OUTB (devc->osdev, 0x00, devc->base + CONC_bNMIENA_OFF);
  OUTW (devc->osdev, 0x0000, devc->base + CONC_wNMISTAT_OFF);

  /* Init serial interface */
  OUTB (devc->osdev, 0x00, devc->base + CONC_bSERCTL_OFF);
  OUTB (devc->osdev, CONC_PCM_DAC1_STEREO | CONC_PCM_DAC1_16BIT,
	devc->base + CONC_bSERFMT_OFF);

  /* Unmute the codec */
  tmp = INB (devc->osdev, devc->base + CONC_bMISCCTL_OFF) & ~CONC_MISCTL_MUTE;
  OUTB (devc->osdev, tmp, devc->base + CONC_bMISCCTL_OFF);

  /* Reset the UART */
  OUTB (devc->osdev, 0x03, devc->base + CONC_bUARTCSTAT_OFF);
  OUTB (devc->osdev, 0x00, devc->base + CONC_bUARTCSTAT_OFF);

  /*
   ******** Mixer initialization *********
   */
  oss_udelay (30);
  ak_write (devc, 0x16, 0x03);	/* Release reset */
  oss_udelay (50);
  ak_write (devc, 0x18, 0x00);	/* Select ADC from input mixer */

  for (i = 0; i <= 0x19; i++)
    ak_write (devc, i, ak_reg_init[i]);

  /* Enable microphone phantom power */
  tmp = INW (devc->osdev, devc->base + 2) & ~CONC_DEVCTL_MICBIAS;
  if (devc->micbias)
    tmp |= CONC_DEVCTL_MICBIAS;
  OUTW (devc->osdev, tmp, devc->base + 2);

  if ((my_mixer = oss_install_mixer (OSS_MIXER_DRIVER_VERSION,
				     devc->osdev,
				     devc->osdev,
				     "Creative AudioPCI",
				     &apci_mixer_driver,
				     sizeof (mixer_driver_t), devc)) >= 0)
    {
      char mxname[20];

      sprintf (mxname, "AudioPCI");
      devc->recdevs = 0;
      apci_set_recmask (devc, SOUND_MASK_MIC);
      devc->levels = load_mixer_volumes (mxname, default_mixer_levels, 1);
      mixer_ext_set_init_fn (my_mixer, apci_mix_init, 30);
      apci_mixer_reset (devc);
    }

  for (i = 0; i < MAX_PORTC; i++)
    {

      int adev;
      char tmp_name[100];
      apci_portc *portc = &devc->portc[i];
      int caps = ADEV_AUTOMODE;

      if (i == 0)
	{
	  strcpy (tmp_name, devc->chip_name);
	  caps |= ADEV_DUPLEX;
	}
      else
	{
	  sprintf (tmp_name, "%s (playback only)", devc->chip_name);
	  caps |= ADEV_NOINPUT;
	}

      if ((adev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
					devc->osdev,
					devc->osdev,
					tmp_name,
					&apci_audio_driver,
					sizeof (audiodrv_t),
					caps,
					AFMT_U8 | AFMT_S16_LE, devc, -1)) < 0)
	{
	  adev = -1;
	  return 0;
	}
      else
	{
	  audio_engines[adev]->portc = portc;
	  if (i == 0)
	    {
	      audio_engines[adev]->min_rate = 5000;
	      audio_engines[adev]->max_rate = 48000;
	      audio_engines[adev]->caps |= PCM_CAP_FREERATE;
	    }
	  else
	    {
	      audio_engines[adev]->min_rate = 5012;
	      audio_engines[adev]->max_rate = 44100;
	    }
/*        audio_engines[adev]->min_block = 1024; */
	  portc->open_mode = 0;
	  portc->audiodev = adev;
	  portc->atype = i;
          audio_engines[adev]->mixer_dev = my_mixer;
#ifdef CONFIG_OSS_VMIX
	  if (i == 0)
	     vmix_attach_audiodev(devc->osdev, adev, -1, 0);
#endif
	}
    }

  if ((devc->midi_dev = oss_install_mididev (OSS_MIDI_DRIVER_VERSION, "AUDIOPCI", "AudioPCI UART", &apci_midi_driver, sizeof (midi_driver_t),
					     0, devc, devc->osdev)) < 0)
    {
      cmn_err (CE_WARN, "Couldn't install MIDI device\n");
      return 0;
    }

  devc->midi_opened = 0;
  return 1;
}

int
oss_audiopci_attach (oss_device_t * osdev)
{
  unsigned char pci_irq_line, pci_revision;
  unsigned short pci_command, vendor, device;
  unsigned int pci_ioaddr;
  int err;
  apci_devc *devc;

  DDB (cmn_err (CE_WARN, "Entered AudioPCI probe routine\n"));

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  if ((vendor != ENSONIQ_VENDOR_ID && vendor != ENSONIQ_VENDOR_ID2) ||
      device != ENSONIQ_AUDIOPCI)

    return 0;

  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_0, &pci_ioaddr);

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

  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_IO;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  devc->chip_name = "Creative AudioPCI (ES1370)";

  oss_register_device (osdev, devc->chip_name);


  if ((err = oss_register_interrupts (osdev, 0, apciintr, NULL)) < 0)
    {
      cmn_err (CE_WARN, "Can't allocate IRQ%d, err=%d\n", pci_irq_line, err);
      return 0;
    }

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);

  return init_apci (devc);	/* Detected */
}


int
oss_audiopci_detach (oss_device_t * osdev)
{
  apci_devc *devc = (apci_devc *) osdev->devc;
  int tmp;

  if (oss_disable_device (osdev) < 0)
    return 0;

  tmp = INB (devc->osdev, devc->base + CONC_bDEVCTL_OFF) &
    ~(CONC_DEVCTL_DAC2_EN | CONC_DEVCTL_ADC_EN | CONC_DEVCTL_DAC1_EN);
  OUTB (devc->osdev, tmp, devc->base + CONC_bDEVCTL_OFF);
  OUTB (devc->osdev, tmp, devc->base + CONC_bDEVCTL_OFF);
  OUTB (devc->osdev, tmp, devc->base + CONC_bDEVCTL_OFF);
  OUTB (devc->osdev, tmp, devc->base + CONC_bDEVCTL_OFF);

  oss_unregister_interrupts (devc->osdev);

  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);
  UNMAP_PCI_IOADDR (devc->osdev, 0);

  oss_unregister_device (devc->osdev);
  return 1;
}
