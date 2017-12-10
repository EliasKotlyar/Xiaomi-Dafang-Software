/*
 * Purpose: Driver for the UltraSparc workstations using CS4231 codec for audio
 *
 * This driver is for Solaris/Sparc only. It uses Solaris specific kernel 
 * services which are not portable to other operating systems.
 *
 * Originally this driver supported various AD1848 based ISA codecs. For
 * this reason devc->model is used to find out which features the codec
 * supports. The latest version of the driver supports only models 2 (CS4231)
 * and 3 (CS4231A).
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

#include "oss_audiocs_cfg.h"
#include <oss_pci.h>

#include "cs4231_mixer.h"

struct cs4231_pioregs
{
 	uint8_t iar;		/* index address register */
	uint8_t pad1[3];		/* pad */
	uint8_t idr;		/* indexed data register */
	uint8_t pad2[3];		/* pad */
	uint8_t statr;		/* status register */
	uint8_t pad3[3];		/* pad */
	uint8_t piodr;		/* PIO data regsiter */
	uint8_t pad4[3];
};

/*
 * These are the registers for the EBUS2 DMA channel interface to the
 * 4231. One struct per channel for playback and record, therefore there
 * individual handles for the CODEC and the two DMA engines.
 */

struct cs4231_eb2regs {
	uint32_t 	eb2csr;		/* Ebus 2 csr */
	uint32_t 	eb2acr;		/* ebus 2 Addrs */
	uint32_t 	eb2bcr;		/* ebus 2 counts */
};
typedef struct cs4231_eb2regs cs4231_eb2regs_t;

#define	PLAY_CSR	devc->play_regs->eb2csr
#define	PLAY_ACR	devc->play_regs->eb2acr
#define	PLAY_BCR	devc->play_regs->eb2bcr
#define	REC_CSR		devc->rec_regs->eb2csr
#define	REC_ACR		devc->rec_regs->eb2acr
#define	REC_BCR		devc->rec_regs->eb2bcr

typedef struct
{
  oss_device_t *osdev;
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;
  struct cs4231_pioregs *codec_base;
  uint_t *auxio_base;

  cs4231_eb2regs_t *play_regs;
  cs4231_eb2regs_t *rec_regs;

  ddi_acc_handle_t codec_acc_handle;
  ddi_acc_handle_t auxio_acc_handle;
  ddi_acc_handle_t play_acc_handle;
  ddi_acc_handle_t rec_acc_handle;
#define CODEC_HNDL devc->codec_acc_handle
#define PLAY_HNDL  devc->play_acc_handle
#define REC_HNDL   devc->rec_acc_handle
#define AUXIO_HNDL devc->auxio_acc_handle

  unsigned char MCE_bit;
  unsigned char saved_regs[32];

  int audio_flags;
  int record_dev, playback_dev;
  int speed;
  unsigned char speed_bits;
  int channels;
  int audio_format;
  unsigned char format_bits;

  int xfer_count;
  int audio_mode;
  int open_mode;
  char *chip_name;
  int model;
#define MD_1848		1
#define MD_4231		2
#define MD_4231A	3

  /* Mixer parameters */
  int is_muted;
  int recmask;
  int supported_devices, orig_devices;
  int supported_rec_devices, orig_rec_devices;
  int *levels;
  short mixer_reroute[32];
  int dev_no;
  volatile unsigned long timer_ticks;
  int timer_running;
  int irq_ok;
  mixer_ents *mix_devices;
  int mixer_output_port;
}
cs4231_devc_t;

typedef struct cs4231_portc_t
{
  int open_mode;
}
cs4231_portc_t;

static int ad_format_mask[9 /*devc->model */ ] =
{
  0,
  AFMT_U8 | AFMT_S16_LE | AFMT_MU_LAW | AFMT_A_LAW,
  AFMT_U8 | AFMT_S16_LE | AFMT_MU_LAW | AFMT_A_LAW | AFMT_S16_BE,
  AFMT_U8 | AFMT_S16_LE | AFMT_MU_LAW | AFMT_A_LAW | AFMT_S16_BE,
  AFMT_U8 | AFMT_S16_LE | AFMT_MU_LAW | AFMT_A_LAW,	/* AD1845 */
  AFMT_U8 | AFMT_S16_LE | AFMT_MU_LAW | AFMT_A_LAW | AFMT_S16_BE,
  AFMT_U8 | AFMT_S16_LE | AFMT_S16_BE,
  AFMT_U8 | AFMT_S16_LE | AFMT_MU_LAW | AFMT_A_LAW | AFMT_S16_BE,
  AFMT_U8 | AFMT_S16_LE		/* CMI8330 */
};

#define CODEC_INB(devc, addr)		ddi_get8(CODEC_HNDL, addr)
#define CODEC_OUTB(devc, data, addr)	ddi_put8(CODEC_HNDL, addr, data)

/*
 * CS4231 codec I/O registers
 */
#define io_Index_Addr(d)	&d->codec_base->iar
#define io_Indexed_Data(d)	&d->codec_base->idr
#define io_Status(d)		&d->codec_base->statr
#define io_Polled_IO(d)		&d->codec_base->piodr
#define CS4231_IO_RETRIES	10

/*
 * EB2 audio registers
 */
#define		EB2_AUXIO_COD_PDWN	0x00000001u	/* power down Codec */

static int cs4231_open (int dev, int mode, int open_flags);
static void cs4231_close (int dev, int mode);
static int cs4231_ioctl (int dev, unsigned int cmd, ioctl_arg arg);
static void cs4231_output_block (int dev, oss_native_word buf, int count,
				 int fragsize, int intrflag);
static void cs4231_start_input (int dev, oss_native_word buf, int count,
				int fragsize, int intrflag);
static int cs4231_prepare_for_output (int dev, int bsize, int bcount);
static int cs4231_prepare_for_input (int dev, int bsize, int bcount);
static void cs4231_halt (int dev);
static void cs4231_halt_input (int dev);
static void cs4231_halt_output (int dev);
static void cs4231_trigger (int dev, int bits);

static void
eb2_power(cs4231_devc_t * devc, int level)
{
	unsigned int tmp;
	oss_native_word flags;

  	MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
	tmp = ddi_get32(AUXIO_HNDL, devc->auxio_base);

	tmp &= ~EB2_AUXIO_COD_PDWN;

	if (!level)
	   tmp |= EB2_AUXIO_COD_PDWN;
	ddi_put32(AUXIO_HNDL, devc->auxio_base, tmp);

	oss_udelay(10000);

  	MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
}

static int
ad_read (cs4231_devc_t * devc, int reg)
{
  oss_native_word flags;
  int x;

  reg = reg & 0xff;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);

  for (x=0;x<CS4231_IO_RETRIES;x++)
  {
     CODEC_OUTB (devc, (unsigned char) reg | devc->MCE_bit,
	io_Index_Addr (devc));
     oss_udelay(1000);

     if (CODEC_INB (devc, io_Index_Addr (devc)) == (reg | devc->MCE_bit))
	break;
  }

  if (x==CS4231_IO_RETRIES)
  {
     cmn_err(CE_NOTE, "Indirect register selection failed (read %d)\n", reg);
     cmn_err(CE_CONT, "Reg=%02x (%02x)\n", CODEC_INB (devc, io_Index_Addr (devc)), reg | devc->MCE_bit);
  }

  x = CODEC_INB (devc, io_Indexed_Data (devc));
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);

  return x;
}

static void
ad_write (cs4231_devc_t * devc, int reg, int data)
{
  oss_native_word flags;
  int x;

  reg &= 0xff;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);

  for (x=0;x<CS4231_IO_RETRIES;x++)
  {
     CODEC_OUTB (devc, (unsigned char) reg | devc->MCE_bit,
	io_Index_Addr (devc));

     oss_udelay(1000);

     if (CODEC_INB (devc, io_Index_Addr (devc)) == (reg | devc->MCE_bit))
	break;
  }

  if (x==CS4231_IO_RETRIES)
  {
     cmn_err(CE_NOTE, "Indirect register selection failed (write %d)\n", reg);
     cmn_err(CE_CONT, "Reg=%02x (%02x)\n", CODEC_INB (devc, io_Index_Addr (devc)), reg | devc->MCE_bit);
  }

  CODEC_OUTB (devc, (unsigned char) (data & 0xff), io_Indexed_Data (devc));
  oss_udelay(1000);

  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
}

static void
ad_mute (cs4231_devc_t * devc)
{
  int i;
  unsigned char prev;

  /*
   * Save old register settings and mute output channels
   */
  for (i = 6; i < 8; i++)
    {
      prev = devc->saved_regs[i] = ad_read (devc, i);

      devc->is_muted = 1;
      ad_write (devc, i, prev | 0x80);
    }
}

static void
ad_unmute (cs4231_devc_t * devc)
{
  int i, dummy;

  /*
   * Restore back old volume registers (unmute)
   */
  for (i = 6; i < 8; i++)
    {
      ad_write (devc, i, devc->saved_regs[i] & ~0x80);
    }
  devc->is_muted = 0;
}

static void
ad_enter_MCE (cs4231_devc_t * devc)
{
  unsigned short prev;

  int timeout = 1000;
  while (timeout > 0 && CODEC_INB (devc, io_Index_Addr(devc)) == 0x80)	/*Are we initializing */
    timeout--;

  devc->MCE_bit = 0x40;
  prev = CODEC_INB (devc, io_Index_Addr (devc));
  if (prev & 0x40)
    {
      return;
    }

  CODEC_OUTB (devc, devc->MCE_bit, io_Index_Addr (devc));
}

static void
ad_leave_MCE (cs4231_devc_t * devc)
{
  unsigned char prev;
  int timeout = 1000;

  while (timeout > 0 && CODEC_INB (devc, io_Index_Addr(devc)) == 0x80)	/*Are we initializing */
    timeout--;

  devc->MCE_bit = 0x00;
  prev = CODEC_INB (devc, io_Index_Addr (devc));
  CODEC_OUTB (devc, 0x00, io_Index_Addr (devc));	/* Clear the MCE bit */

  if ((prev & 0x40) == 0)	/* Not in MCE mode */
    {
      return;
    }

  CODEC_OUTB (devc, 0x00, io_Index_Addr (devc));	/* Clear the MCE bit */
}

static int
cs4231_set_recmask (cs4231_devc_t * devc, int mask)
{
  unsigned char recdev;
  int i, n;

  mask &= devc->supported_rec_devices;

  /* Rename the mixer bits if necessary */
  for (i = 0; i < 32; i++)
    if (devc->mixer_reroute[i] != i)
      if (mask & (1 << i))
	{
	  mask &= ~(1 << i);
	  mask |= (1 << devc->mixer_reroute[i]);
	}

  n = 0;
  for (i = 0; i < 32; i++)	/* Count selected device bits */
    if (mask & (1 << i))
      n++;

  if (n == 0)
    mask = SOUND_MASK_MIC;
  else if (n != 1)		/* Too many devices selected */
    {
      mask &= ~devc->recmask;	/* Filter out active settings */

      n = 0;
      for (i = 0; i < 32; i++)	/* Count selected device bits */
	if (mask & (1 << i))
	  n++;

      if (n != 1)
	mask = SOUND_MASK_MIC;
    }

  switch (mask)
    {
    case SOUND_MASK_MIC:
      recdev = 2;
      break;

    case SOUND_MASK_LINE:
      recdev = 0;
      break;

    case SOUND_MASK_CD:
    case SOUND_MASK_LINE1:
      recdev = 1;
      break;

    case SOUND_MASK_IMIX:
      recdev = 3;
      break;

    default:
      mask = SOUND_MASK_MIC;
      recdev = 2;
    }

  recdev <<= 6;
  ad_write (devc, 0, (ad_read (devc, 0) & 0x3f) | recdev);
  ad_write (devc, 1, (ad_read (devc, 1) & 0x3f) | recdev);

  /* Rename the mixer bits back if necessary */
  for (i = 0; i < 32; i++)
    if (devc->mixer_reroute[i] != i)
      if (mask & (1 << devc->mixer_reroute[i]))
	{
	  mask &= ~(1 << devc->mixer_reroute[i]);
	  mask |= (1 << i);
	}

  devc->recmask = mask;
  return mask;
}

static void
change_bits (cs4231_devc_t * devc, unsigned char *regval, int dev, int chn,
	     int newval, int regoffs)
{
  unsigned char mask;
  int shift;
  int mute;
  int mutemask;
  int set_mute_bit;

  set_mute_bit = (newval == 0) || (devc->is_muted && dev == SOUND_MIXER_PCM);

  if (devc->mix_devices[dev][chn].polarity == 1)	/* Reverse */
    {
      newval = 100 - newval;
    }

  mask = (1 << devc->mix_devices[dev][chn].nbits) - 1;
  shift = devc->mix_devices[dev][chn].bitpos;

#if 0
  newval = (int) ((newval * mask) + 50) / 100;	/* Scale it */
  *regval &= ~(mask << shift);	/* Clear bits */
  *regval |= (newval & mask) << shift;	/* Set new value */
#else
  if (devc->mix_devices[dev][RIGHT_CHN].mutepos == 8)
    {				/* if there is no mute bit */
      mute = 0;			/* No mute bit; do nothing special */
      mutemask = ~0;		/* No mute bit; do nothing special */
    }
  else
    {
      mute = (set_mute_bit << devc->mix_devices[dev][RIGHT_CHN].mutepos);
      mutemask = ~(1 << devc->mix_devices[dev][RIGHT_CHN].mutepos);
    }

  newval = (int) ((newval * mask) + 50) / 100;	/* Scale it */
  *regval &= (~(mask << shift)) & (mutemask);	/* Clear bits */
  *regval |= ((newval & mask) << shift) | mute;	/* Set new value */
#endif
}

static int
cs4231_mixer_get (cs4231_devc_t * devc, int dev)
{
  if (!((1 << dev) & devc->supported_devices))
    return OSS_EINVAL;

  dev = devc->mixer_reroute[dev];

  return devc->levels[dev];
}

static int
cs4231_mixer_set (cs4231_devc_t * devc, int dev, int value)
{
  int left = value & 0x000000ff;
  int right = (value & 0x0000ff00) >> 8;
  int retvol;

  int regoffs, regoffs1;
  unsigned char val;

  if (dev > 31)
    return OSS_EINVAL;

  if (!(devc->supported_devices & (1 << dev)))
    return OSS_EINVAL;

  dev = devc->mixer_reroute[dev];

  if (left > 100)
    left = 100;
  if (right > 100)
    right = 100;

  if (devc->mix_devices[dev][RIGHT_CHN].nbits == 0)	/* Mono control */
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

  if (devc->mix_devices[dev][LEFT_CHN].nbits == 0)
    return OSS_EINVAL;

  devc->levels[dev] = retvol;

  /*
   * Set the left channel
   */

  regoffs1 = regoffs = devc->mix_devices[dev][LEFT_CHN].regno;
  val = ad_read (devc, regoffs);
  change_bits (devc, &val, dev, LEFT_CHN, left, regoffs);
  devc->saved_regs[regoffs] = val;

  /*
   * Set the right channel
   */

  if (devc->mix_devices[dev][RIGHT_CHN].nbits == 0)
    {
      ad_write (devc, regoffs, val);
      return retvol;		/* Was just a mono channel */
    }

  regoffs = devc->mix_devices[dev][RIGHT_CHN].regno;
  if (regoffs != regoffs1)
    {
      ad_write (devc, regoffs1, val);
      val = ad_read (devc, regoffs);
    }

  change_bits (devc, &val, dev, RIGHT_CHN, right, regoffs);
  ad_write (devc, regoffs, val);
  devc->saved_regs[regoffs] = val;

  return retvol;
}

static void
cs4231_mixer_reset (cs4231_devc_t * devc)
{
  int i;

  devc->mix_devices = &(cs4231_mix_devices[0]);
  devc->supported_devices = MODE2_MIXER_DEVICES;
  devc->supported_rec_devices = MODE2_REC_DEVICES;

  for (i = 0; i < 32; i++)
    devc->mixer_reroute[i] = i;

  devc->orig_devices = devc->supported_devices;
  devc->orig_rec_devices = devc->supported_rec_devices;

  devc->levels = default_mixer_levels;

  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
    if (devc->supported_devices & (1 << i))
      cs4231_mixer_set (devc, i, devc->levels[i]);
  cs4231_set_recmask (devc, SOUND_MASK_MIC);
}

static int
cs4231_mixer_ioctl (int dev, int audiodev, unsigned int cmd, ioctl_arg arg)
{
  cs4231_devc_t *devc = mixer_devs[dev]->devc;

  if (cmd == SOUND_MIXER_PRIVATE1) /* SADA compatible play target selection */
    {
      int val;

      val = *arg;

      if (val == 0xffff)
	return *arg = devc->mixer_output_port;

      val &= (AUDIO_SPEAKER | AUDIO_HEADPHONE | AUDIO_LINE_OUT);

      devc->mixer_output_port = val;

      if (val & AUDIO_SPEAKER)
	ad_write (devc, 26, ad_read (devc, 26) & ~0x40);	/* Unmute mono out */
      else
	ad_write (devc, 26, ad_read (devc, 26) | 0x40);	/* Mute mono out */

      return *arg = devc->mixer_output_port;
    }

  if (((cmd >> 8) & 0xff) == 'M')
    {
      int val;

      if (IOC_IS_OUTPUT (cmd))
	switch (cmd & 0xff)
	  {
	  case SOUND_MIXER_RECSRC:
	    val = *arg;
	    return *arg = cs4231_set_recmask (devc, val);
	    break;

	  default:
	    val = *arg;
	    return *arg = cs4231_mixer_set (devc, cmd & 0xff, val);
	  }
      else
	switch (cmd & 0xff)	/*
				 * Return parameters
				 */
	  {

	  case SOUND_MIXER_RECSRC:
	    return *arg = devc->recmask;
	    break;

	  case SOUND_MIXER_DEVMASK:
	    return *arg = devc->supported_devices;
	    break;

	  case SOUND_MIXER_STEREODEVS:
	    return *arg = devc->supported_devices &
	      ~(SOUND_MASK_SPEAKER | SOUND_MASK_IMIX);
	    break;

	  case SOUND_MIXER_RECMASK:
	    return *arg = devc->supported_rec_devices;
	    break;

	  case SOUND_MIXER_CAPS:
	    return *arg = SOUND_CAP_EXCL_INPUT;
	    break;

	  default:
	    return *arg = cs4231_mixer_get (devc, cmd & 0xff);
	  }
    }
  else
    return OSS_EINVAL;
}

static int
cs4231_set_rate (int dev, int arg)
{
  cs4231_devc_t *devc = (cs4231_devc_t *) audio_engines[dev]->devc;

  /*
   * The sampling speed is encoded in the least significant nibble of I8. The
   * LSB selects the clock source (0=24.576 MHz, 1=16.9344 MHz) and other
   * three bits select the divisor (indirectly):
   *
   * The available speeds are in the following table. Keep the speeds in
   * the increasing order.
   */
  typedef struct
  {
    int speed;
    unsigned char bits;
  }
  speed_struct;

  static speed_struct speed_table[] = {
    {5510, (0 << 1) | 1},
    {5510, (0 << 1) | 1},
    {6620, (7 << 1) | 1},
    {8000, (0 << 1) | 0},
    {9600, (7 << 1) | 0},
    {11025, (1 << 1) | 1},
    {16000, (1 << 1) | 0},
    {18900, (2 << 1) | 1},
    {22050, (3 << 1) | 1},
    {27420, (2 << 1) | 0},
    {32000, (3 << 1) | 0},
    {33075, (6 << 1) | 1},
    {37800, (4 << 1) | 1},
    {44100, (5 << 1) | 1},
    {48000, (6 << 1) | 0}
  };

  int i, n, selected = -1;

  n = sizeof (speed_table) / sizeof (speed_struct);

  if (arg <= 0)
    return devc->speed;

#if 1
  if (ad_read (devc, 9) & 0x03)
    return devc->speed;
#endif

  if (arg < speed_table[0].speed)
    selected = 0;
  if (arg > speed_table[n - 1].speed)
    selected = n - 1;

  for (i = 1 /*really */ ; selected == -1 && i < n; i++)
    if (speed_table[i].speed == arg)
      selected = i;
    else if (speed_table[i].speed > arg)
      {
	int diff1, diff2;

	diff1 = arg - speed_table[i - 1].speed;
	diff2 = speed_table[i].speed - arg;

	if (diff1 < diff2)
	  selected = i - 1;
	else
	  selected = i;
      }

  if (selected == -1)
    {
      cmn_err (CE_WARN, "Can't find supported sample rate?\n");
      selected = 3;
    }

  devc->speed = speed_table[selected].speed;
  devc->speed_bits = speed_table[selected].bits;
  return devc->speed;
}

static short
cs4231_set_channels (int dev, short arg)
{
  cs4231_devc_t *devc = (cs4231_devc_t *) audio_engines[dev]->devc;

  if (arg != 1 && arg != 2)
    {
      return devc->channels;
    }

#if 1
  if (ad_read (devc, 9) & 0x03)
    {
      return devc->channels;
    }
#endif

  devc->channels = arg;
  return arg;
}

static unsigned int
cs4231_set_bits (int dev, unsigned int arg)
{
  cs4231_devc_t *devc = (cs4231_devc_t *) audio_engines[dev]->devc;

  static struct format_tbl
  {
    int format;
    unsigned char bits;
  }
  format2bits[] =
  {
    {0, 0},
    {AFMT_MU_LAW, 1},
    {AFMT_A_LAW, 3},
    {AFMT_U8, 0},
    {AFMT_S16_LE, 2},
    {AFMT_S16_BE, 6},
    {AFMT_S8, 0},
    {AFMT_U16_LE, 0},
    {AFMT_U16_BE, 0}
  };
  int i, n = sizeof (format2bits) / sizeof (struct format_tbl);

  if (arg == 0)
    return devc->audio_format;

#if 1
  if (ad_read (devc, 9) & 0x03)
    return devc->audio_format;
#endif

  if (!(arg & ad_format_mask[devc->model]))
    arg = AFMT_U8;

  devc->audio_format = arg;

  for (i = 0; i < n; i++)
    if (format2bits[i].format == arg)
      {
	if ((devc->format_bits = format2bits[i].bits) == 0)
	  return devc->audio_format = AFMT_U8;	/* Was not supported */

	return arg;
      }

  /* Still hanging here. Something must be terribly wrong */
  devc->format_bits = 0;
  return devc->audio_format = AFMT_U8;
}

static const audiodrv_t cs4231_audio_driver = {
  cs4231_open,
  cs4231_close,
  cs4231_output_block,
  cs4231_start_input,
  cs4231_ioctl,
  cs4231_prepare_for_input,
  cs4231_prepare_for_output,
  cs4231_halt,
  NULL,
  NULL,
  cs4231_halt_input,
  cs4231_halt_output,
  cs4231_trigger,
  cs4231_set_rate,
  cs4231_set_bits,
  cs4231_set_channels
};

static mixer_driver_t cs4231_mixer_driver = {
  cs4231_mixer_ioctl
};

static int
cs4231_open (int dev, int mode, int open_flags)
{
  cs4231_devc_t *devc = NULL;
  cs4231_portc_t *portc;
  oss_native_word flags;

  if (dev < 0 || dev >= num_audio_engines)
    return OSS_ENXIO;

  devc = (cs4231_devc_t *) audio_engines[dev]->devc;
  portc = (cs4231_portc_t *) audio_engines[dev]->portc;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode || (devc->open_mode & mode))
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  devc->open_mode |= mode;
  portc->open_mode = mode;
  devc->audio_mode &= ~mode;

  if (mode & OPEN_READ)
    devc->record_dev = dev;
  if (mode & OPEN_WRITE)
    devc->playback_dev = dev;
/*
 * Mute output until the playback really starts. This decreases clicking (hope so).
 */
  ad_mute (devc);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

static void
cs4231_close (int dev, int mode)
{
  oss_native_word flags;
  cs4231_devc_t *devc = (cs4231_devc_t *) audio_engines[dev]->devc;
  cs4231_portc_t *portc = (cs4231_portc_t *) audio_engines[dev]->portc;

  DDB (cmn_err (CE_CONT, "cs4231_close(void)\n"));

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  cs4231_halt (dev);

  devc->open_mode &= ~portc->open_mode;
  devc->audio_mode &= ~portc->open_mode;
  portc->open_mode = 0;

  ad_mute (devc);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static int
cs4231_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void
cs4231_output_block (int dev, oss_native_word buf, int count, int fragsize,
		     int intrflag)
{
  oss_native_word flags;
  unsigned int cnt;
  cs4231_devc_t *devc = (cs4231_devc_t *) audio_engines[dev]->devc;

  cnt = fragsize;
  /* cnt = count; */

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (devc->audio_format & (AFMT_S16_LE | AFMT_S16_BE))	/* 16 bit data */
     cnt >>= 1;
  if (devc->channels > 1)
     cnt >>= 1;
  cnt--;

  if (devc->audio_mode & PCM_ENABLE_OUTPUT
      && audio_engines[dev]->flags & ADEV_AUTOMODE && intrflag
      && cnt == devc->xfer_count)
    {
      devc->audio_mode |= PCM_ENABLE_OUTPUT;
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return;
    }

  ad_write (devc, 15, (unsigned char) (cnt & 0xff));
  ad_write (devc, 14, (unsigned char) ((cnt >> 8) & 0xff));

  devc->xfer_count = cnt;
  devc->audio_mode |= PCM_ENABLE_OUTPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static void
cs4231_start_input (int dev, oss_native_word buf, int count, int fragsize,
		    int intrflag)
{
  oss_native_word flags;
  unsigned int cnt;
  cs4231_devc_t *devc = (cs4231_devc_t *) audio_engines[dev]->devc;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  cnt = fragsize;
  /* cnt = count; */

  if (devc->audio_format & (AFMT_S16_LE | AFMT_S16_BE))	/* 16 bit data */
     cnt >>= 1;

  if (devc->channels > 1)
    cnt >>= 1;
  cnt--;

  if (devc->audio_mode & PCM_ENABLE_INPUT
      && audio_engines[dev]->flags & ADEV_AUTOMODE && intrflag
      && cnt == devc->xfer_count)
    {
      devc->audio_mode |= PCM_ENABLE_INPUT;
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return;			/*
				 * Auto DMA mode on. No need to react
				 */
    }

  ad_write (devc, 31, (unsigned char) (cnt & 0xff));
  ad_write (devc, 30, (unsigned char) ((cnt >> 8) & 0xff));

  ad_unmute (devc);

  devc->xfer_count = cnt;
  devc->audio_mode |= PCM_ENABLE_INPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static void
set_output_format (int dev)
{
  int timeout;
  unsigned char fs, old_fs, tmp = 0;
  oss_native_word flags;
  cs4231_devc_t *devc = (cs4231_devc_t *) audio_engines[dev]->devc;

  if (ad_read (devc, 9) & 0x03)
    return;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  fs = devc->speed_bits | (devc->format_bits << 5);

  if (devc->channels > 1)
    fs |= 0x10;

  ad_enter_MCE (devc);		/* Enables changes to the format select reg */

  old_fs = ad_read (devc, 8);

  ad_write (devc, 8, fs);
  /*
   * Write to I8 starts resynchronization. Wait until it completes.
   */
  timeout = 0;
  while (timeout < 100 && CODEC_INB (devc, io_Index_Addr(devc)) != 0x80)
    timeout++;
  timeout = 0;
  while (timeout < 10000 && CODEC_INB (devc, io_Index_Addr(devc)) == 0x80)
    timeout++;

  ad_leave_MCE (devc);

  devc->xfer_count = 0;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static void
set_input_format (int dev)
{
  int timeout;
  unsigned char fs, old_fs, tmp = 0;
  oss_native_word flags;
  cs4231_devc_t *devc = (cs4231_devc_t *) audio_engines[dev]->devc;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  fs = devc->speed_bits | (devc->format_bits << 5);

  if (devc->channels > 1)
    fs |= 0x10;

  ad_enter_MCE (devc);		/* Enables changes to the format select reg */

  /*
   * If mode >= 2 (CS4231), set I28. It's the capture format register.
   */
  if (devc->model != MD_1848)
    {
      old_fs = ad_read (devc, 28);
      ad_write (devc, 28, fs);

      /*
       * Write to I28 starts resynchronization. Wait until it completes.
       */
      timeout = 0;
      while (timeout < 100 && CODEC_INB (devc, io_Index_Addr(devc)) != 0x80)
	timeout++;

      timeout = 0;
      while (timeout < 10000 && CODEC_INB (devc, io_Index_Addr(devc)) == 0x80)
	timeout++;

      if (devc->model != MD_1848)
	{
	  /*
	   * CS4231 compatible devices don't have separate sampling rate selection
	   * register for recording an playback. The I8 register is shared so we have to
	   * set the speed encoding bits of it too.
	   */
	  unsigned char tmp = devc->speed_bits | (ad_read (devc, 8) & 0xf0);
	  ad_write (devc, 8, tmp);
	  /*
	   * Write to I8 starts resynchronization. Wait until it completes.
	   */
	  timeout = 0;
	  while (timeout < 100 && CODEC_INB (devc, io_Index_Addr(devc)) != 0x80)
	    timeout++;

	  timeout = 0;
	  while (timeout < 10000 && CODEC_INB (devc, io_Index_Addr(devc)) == 0x80)
	    timeout++;
	}
    }
  else
    {				/* For CS4231 set I8. */

      old_fs = ad_read (devc, 8);
      ad_write (devc, 8, fs);
      /*
       * Write to I8 starts resynchronization. Wait until it completes.
       */
      timeout = 0;
      while (timeout < 100 && CODEC_INB (devc, io_Index_Addr(devc)) != 0x80)
	timeout++;
      timeout = 0;
      while (timeout < 10000 && CODEC_INB (devc, io_Index_Addr(devc)) == 0x80)
	timeout++;
    }

  ad_leave_MCE (devc);
  devc->xfer_count = 0;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

}

static void
set_sample_format (int dev)
{
  cs4231_devc_t *devc = (cs4231_devc_t *) audio_engines[dev]->devc;

  if (ad_read (devc, 9) & 0x03)	/* Playback or recording active */
    return;

  set_input_format (dev);
  set_output_format (dev);
}

static int
cs4231_prepare_for_output (int dev, int bsize, int bcount)
{
  cs4231_devc_t *devc = (cs4231_devc_t *) audio_engines[dev]->devc;

  ad_mute (devc);

  cs4231_halt_output (dev);
  set_sample_format (dev);

/*
 * Setup the EB2 DMA engine
 */

  return 0;
}

static int
cs4231_prepare_for_input (int dev, int bsize, int bcount)
{
  cs4231_devc_t *devc = (cs4231_devc_t *) audio_engines[dev]->devc;

  if (devc->audio_mode)
    return 0;

  cs4231_halt_input (dev);
  set_sample_format (dev);
  //TODO: Prepare the DMA engine
  return 0;
}

static void
cs4231_halt (int dev)
{
  cs4231_devc_t *devc = (cs4231_devc_t *) audio_engines[dev]->devc;
  cs4231_portc_t *portc = (cs4231_portc_t *) audio_engines[dev]->portc;

  unsigned char bits = ad_read (devc, 9);

  if (bits & 0x01 && portc->open_mode & OPEN_WRITE)
    cs4231_halt_output (dev);

  if (bits & 0x02 && portc->open_mode & OPEN_READ)
    cs4231_halt_input (dev);
}

static void
cs4231_halt_input (int dev)
{
  cs4231_devc_t *devc = (cs4231_devc_t *) audio_engines[dev]->devc;
  cs4231_portc_t *portc = (cs4231_portc_t *) audio_engines[dev]->portc;
  oss_native_word flags;

  if (!(portc->open_mode & OPEN_READ))
    return;
  if (!(ad_read (devc, 9) & 0x02))
    return;			/* Capture not enabled */

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  ad_write (devc, 9, ad_read (devc, 9) & ~0x02);	/* Stop capture */
  // TODO: Stop DMA

  CODEC_OUTB (devc, 0, io_Status (devc));	/* Clear interrupt status */
  CODEC_OUTB (devc, 0, io_Status (devc));	/* Clear interrupt status */

  devc->audio_mode &= ~PCM_ENABLE_INPUT;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static void
cs4231_halt_output (int dev)
{
  cs4231_devc_t *devc = (cs4231_devc_t *) audio_engines[dev]->devc;
  cs4231_portc_t *portc = (cs4231_portc_t *) audio_engines[dev]->portc;
  oss_native_word flags;

  if (!(portc->open_mode & OPEN_WRITE))
    return;
  if (!(ad_read (devc, 9) & 0x01))
    return;			/* Playback not enabled */

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  ad_mute (devc);
  oss_udelay (10);

  ad_write (devc, 9, ad_read (devc, 9) & ~0x01);	/* Stop playback */
  //TODO: Disable DMA

  CODEC_OUTB (devc, 0, io_Status (devc));	/* Clear interrupt status */
  CODEC_OUTB (devc, 0, io_Status (devc));	/* Clear interrupt status */

  devc->audio_mode &= ~PCM_ENABLE_OUTPUT;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static void
cs4231_trigger (int dev, int state)
{
  cs4231_devc_t *devc = (cs4231_devc_t *) audio_engines[dev]->devc;
  cs4231_portc_t *portc = (cs4231_portc_t *) audio_engines[dev]->portc;
  oss_native_word flags;
  unsigned char tmp, old, oldstate;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  oldstate = state;
  state &= devc->audio_mode;

  tmp = old = ad_read (devc, 9);

  if (portc->open_mode & OPEN_READ)
    {
      if (state & PCM_ENABLE_INPUT)
	tmp |= 0x02;
      else
	tmp &= ~0x02;
    }

  if (portc->open_mode & OPEN_WRITE)
    {
      if (state & PCM_ENABLE_OUTPUT)
	tmp |= 0x01;
      else
	tmp &= ~0x01;
    }

  /* ad_mute(devc); */
  if (tmp != old)
    {
      ad_write (devc, 9, tmp);
      if (state & PCM_ENABLE_OUTPUT)
	{
	  oss_udelay (10);
	  oss_udelay (10);
	  oss_udelay (10);
	  ad_unmute (devc);
	}
    }

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static void
cs4231_init_hw (cs4231_devc_t * devc)
{
  int i;
  oss_native_word flags;
  /*
   * Initial values for the indirect registers of CS4248/CS4231.
   */
  static int init_values[] = {
    0xa8, 0xa8, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88,
    0x00, 0x0c, 0x02, 0x00, 0x8a, 0x01, 0x00, 0x00,

    /* Positions 16 to 31 just for CS4231/2 and ad1845 */
    0x80, 0x00, 0x10, 0x10, 0x00, 0x00, 0x1f, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };

  for (i = 0; i < 16; i++)
    ad_write (devc, i, init_values[i]);

/*
 * The XCTL0 (0x40) and XCTL1 (0x80) bits of I10 are used in Sparcs to
 * control codec's output pins which mute the line out and speaker out
 * connectors (respectively).
 *
 * Set them both to 0 (not muted). Better control is required in future.
 */

  ad_write (devc, 10, ad_read (devc, 10) & ~0xc0);

  ad_mute (devc);		/* Mute PCM until next use and initialize some variables */

  if (devc->model > MD_1848)
    {
      ad_write (devc, 12, ad_read (devc, 12) | 0x40);	/* Mode2 = enabled */

      for (i = 16; i < 32; i++)
	ad_write (devc, i, init_values[i]);

    }

  if (devc->model > MD_1848)
    {
      if (devc->audio_flags & ADEV_DUPLEX)
	ad_write (devc, 9, ad_read (devc, 9) & ~0x04);	/* Dual DMA mode */
      else
	ad_write (devc, 9, ad_read (devc, 9) | 0x04);	/* Single DMA mode */

    }
  else
    {
      devc->audio_flags &= ~ADEV_DUPLEX;
      ad_write (devc, 9, ad_read (devc, 9) | 0x04);	/* Single DMA mode */
    }

  CODEC_OUTB (devc, 0, io_Status (devc));	/* Clear pending interrupts */

  /*
   * Toggle the MCE bit. It completes the initialization phase.
   */

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  ad_enter_MCE (devc);		/* In case the bit was off */
  ad_leave_MCE (devc);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);

/*
 * Perform full calibration
 */

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  ad_enter_MCE (devc);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);

  ad_write (devc, 9, ad_read (devc, 9) | 0x18);	/* Enable autocalibration */

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  ad_leave_MCE (devc);		/* This will trigger autocalibration */
  ad_enter_MCE (devc);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);

  ad_write (devc, 9, ad_read (devc, 9) & ~0x18);	/* Disable autocalibration */

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  ad_leave_MCE (devc);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
}

int
cs4231_detect (cs4231_devc_t * devc)
{

  unsigned char tmp;
  unsigned char tmp1 = 0xff, tmp2 = 0xff;
  oss_native_word flags;

  int i;

  devc->MCE_bit = 0x40;
  devc->chip_name = "CS4231";
  devc->model = MD_4231;	/* CS4231 or CS4248 */
  devc->levels = NULL;

  /*
   * Check that the I/O address is in use.
   *
   * The bit 0x80 of the base I/O port is known to be 0 after the
   * chip has performed its power on initialization. Just assume
   * this has happened before the OS is starting.
   *
   * If the I/O address is unused, it typically returns 0xff.
   */

  if (CODEC_INB (devc, io_Index_Addr(devc)) == 0xff)
    {
      DDB (cmn_err
	   (CE_CONT,
	    "cs4231_detect: The base I/O address appears to be dead\n"));
    }

#if 1
/*
 * Wait for the device to stop initialization
 */
  DDB (cmn_err (CE_CONT, "cs4231_detect() - step 0\n"));

  for (i = 0; i < 10000000; i++)
    {
      unsigned char x = CODEC_INB (devc, io_Index_Addr(devc));
      if (x == 0xff || !(x & 0x80))
	break;
    }

#endif

  DDB (cmn_err (CE_CONT, "cs4231_detect() - step A\n"));

  if (CODEC_INB (devc, io_Index_Addr(devc)) == 0x80)	/* Not ready. Let's wait */
  {
    MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
    ad_leave_MCE (devc);
    MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  }

  if ((CODEC_INB (devc, io_Index_Addr(devc)) & 0x80) != 0x00)	/* Not a CS4231 */
    {
      DDB (cmn_err (CE_WARN, "cs4231 detect error - step A (%02x)\n",
		    (int) CODEC_INB (devc, io_Index_Addr(devc))));
      return 0;
    }

  /*
   * Test if it's possible to change contents of the indirect registers.
   * Registers 0 and 1 are ADC volume registers. The bit 0x10 is read only
   * so try to avoid using it.
   */

  DDB (cmn_err (CE_CONT, "cs4231_detect() - step B\n"));
  ad_write (devc, 0, 0xaa);
  ad_write (devc, 1, 0x45);	/* 0x55 with bit 0x10 clear */
  oss_udelay (10);

  if ((tmp1 = ad_read (devc, 0)) != 0xaa ||
      (tmp2 = ad_read (devc, 1)) != 0x45)
    {
      DDB (cmn_err
	   (CE_WARN, "cs4231 detect error - step B (%x/%x)\n", tmp1, tmp2));
#if 1
      if (tmp1 == 0x8a && tmp2 == 0xff)	/* AZT2320 ????? */
	{
	  DDB (cmn_err (CE_CONT, "Ignoring error\n"));
	}
      else
#endif
	return 0;
    }

  DDB (cmn_err (CE_CONT, "cs4231_detect() - step C\n"));
  ad_write (devc, 0, 0x45);
  ad_write (devc, 1, 0xaa);
  oss_udelay (10);

  if ((tmp1 = ad_read (devc, 0)) != 0x45
      || (tmp2 = ad_read (devc, 1)) != 0xaa)
    {
      DDB (cmn_err
	   (CE_WARN, "cs4231 detect error - step C (%x/%x)\n", tmp1, tmp2));
#if 1
      if (tmp1 == 0x65 && tmp2 == 0xff)	/* AZT2320 ????? */
	{
	  DDB (cmn_err (CE_CONT, "Ignoring error\n"));
	}
      else
#endif
	return 0;
    }

  /*
   * The indirect register I12 has some read only bits. Lets
   * try to change them.
   */
  DDB (cmn_err (CE_CONT, "cs4231_detect() - step D\n"));
  tmp = ad_read (devc, 12);
  ad_write (devc, 12, (~tmp) & 0x0f);

  if ((tmp & 0x0f) != ((tmp1 = ad_read (devc, 12)) & 0x0f))
    {
      DDB (cmn_err (CE_WARN, "cs4231 detect error - step D (%x)\n", tmp1));
      return 0;
    }

  /*
   * NOTE! Last 4 bits of the reg I12 tell the chip revision.
   *   0x01=RevB and 0x0A=RevC.
   */

  DDB (cmn_err (CE_CONT, "cs4231_detect() - step G\n"));

  ad_write (devc, 12, 0x40);	/* Set mode2, clear 0x80 */

  tmp1 = ad_read (devc, 12);
  if (tmp1 & 0x80)
    {
      devc->chip_name = "CS4248";	/* Our best knowledge just now */
    }

  if ((tmp1 & 0xc0) == (0x80 | 0x40))
    {
      /*
       *      CS4231 detected - is it?
       *
       *      Verify that setting I0 doesn't change I16.
       */
      DDB (cmn_err (CE_CONT, "cs4231_detect() - step H\n"));
      ad_write (devc, 16, 0);	/* Set I16 to known value */

      ad_write (devc, 0, 0x45);
      if ((tmp1 = ad_read (devc, 16)) != 0x45)	/* No change -> CS4231? */
	{

	  ad_write (devc, 0, 0xaa);
	  if ((tmp1 = ad_read (devc, 16)) == 0xaa)	/* Rotten bits? */
	    {
	      DDB (cmn_err
		   (CE_WARN, "cs4231 detect error - step H(%x)\n", tmp1));
	      return 0;
	    }

	  /*
	   * Verify that some bits of I25 are read only.
	   */

	  DDB (cmn_err (CE_CONT, "cs4231_detect() - step I\n"));
	  tmp1 = ad_read (devc, 25);	/* Original bits */
	  ad_write (devc, 25, ~tmp1);	/* Invert all bits */
	  if ((ad_read (devc, 25) & 0xe7) == (tmp1 & 0xe7))
	    {
	      int id, full_id;

	      /*
	       *      It's at least CS4231
	       */
	      devc->chip_name = "CS4231";

	      devc->model = MD_4231;

	      /*
	       * It could be an AD1845 or CS4231A as well.
	       * CS4231 and AD1845 report the same revision info in I25
	       * while the CS4231A reports different.
	       */

	      id = ad_read (devc, 25) & 0xe7;
	      full_id = ad_read (devc, 25);
	      if (id == 0x80)	/* Device busy??? */
		id = ad_read (devc, 25) & 0xe7;
	      if (id == 0x80)	/* Device still busy??? */
		id = ad_read (devc, 25) & 0xe7;
	      DDB (cmn_err
		   (CE_CONT, "cs4231_detect() - step J (%02x/%02x)\n", id,
		    ad_read (devc, 25)));

	      switch (id)
		{

		case 0xa0:
		  devc->chip_name = "CS4231A";
		  devc->model = MD_4231A;
		  break;

		default:	/* Assume CS4231 or OPTi 82C930 */
		  DDB (cmn_err (CE_CONT, "I25 = %02x/%02x\n",
				ad_read (devc, 25),
				ad_read (devc, 25) & 0xe7));
		  devc->model = MD_4231;

		}
	    }
	  ad_write (devc, 25, tmp1);	/* Restore bits */

	  DDB (cmn_err (CE_CONT, "cs4231_detect() - step K\n"));
	}
    }

  DDB (cmn_err (CE_CONT, "cs4231_detect() - Detected OK\n"));

  return 1;
}

void
cs4231_init (cs4231_devc_t * devc)
{
  int i, my_dev, my_mixer;
  char dev_name[100];

  cs4231_portc_t *portc = NULL;

  devc->open_mode = 0;
  devc->timer_ticks = 0;
  devc->audio_flags = ADEV_AUTOMODE;
  devc->playback_dev = devc->record_dev = 0;

  sprintf (dev_name, "Sparc builtin audio (%s)", devc->chip_name);

  if ((my_mixer = oss_install_mixer (OSS_MIXER_DRIVER_VERSION,
				     devc->osdev,
				     devc->osdev,
				     dev_name,
				     &cs4231_mixer_driver,
				     sizeof (mixer_driver_t), devc)) >= 0)
    {
      cs4231_mixer_reset (devc);
    }

  if (devc->model > MD_1848)
    {
      devc->audio_flags |= ADEV_DUPLEX;
    }

  if ((my_dev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
				      devc->osdev,
				      devc->osdev,
				      dev_name,
				      &cs4231_audio_driver,
				      sizeof (audiodrv_t),
				      devc->audio_flags,
				      ad_format_mask[devc->model],
				      devc, -1)) < 0)
    {
      return;
    }

  portc = PMALLOC (devc->osdev, sizeof (*portc));
  memset ((char *) portc, 0, sizeof (*portc));

  audio_engines[my_dev]->portc = portc;
  audio_engines[my_dev]->min_block = 512;
  audio_engines[my_dev]->mixer_dev = my_mixer;

  cs4231_init_hw (devc);

#ifdef CONFIG_OSS_VMIX
  if (i == 0)
     vmix_attach_audiodev(devc->osdev, my_dev, -1, 0);
#endif
#if 0
  test_it (devc);
#endif
}

void
cs4231_unload (cs4231_devc_t * devc)
{
#if 0
  int i, dev = 0;

  for (i = 0; devc == NULL && i < nr_cs4231_devs; i++)
    if (adev_info[i].base == io_base)
      {
	devc = &adev_info[i];
	dev = devc->dev_no;
      }

  if (devc != NULL)
    {

      if (!share_dma)
	{
	  if (irq > 0)
	    snd_release_irq (devc->irq, NULL);

	  FREE_DMA_CHN (audio_engines[dev]->dmap_out->dma);

	  if (audio_engines[dev]->dmap_in->dma !=
	      audio_engines[dev]->dmap_out->dma)
	    FREE_DMA_CHN (audio_engines[dev]->dmap_in->dma);
	}
    }
  else
    cmn_err (CE_WARN, "Can't find device to be unloaded. Base=%x\n", io_base);
#endif
}

int
cs4231intr (oss_device_t * osdev)
{
  unsigned char status;
  cs4231_devc_t *devc = osdev->devc;
  int alt_stat = 0xff;
  unsigned char c930_stat = 0;
  int cnt = 0;
  int serviced = 0;

  devc->irq_ok = 1;

interrupt_again:		/* Jump back here if int status doesn't reset */

  status = CODEC_INB (devc, io_Status (devc));

  if (status == 0x80)
    cmn_err (CE_CONT, "cs4231intr: Why?\n");
  else
    serviced = 1;
  if (devc->model == MD_1848)
    CODEC_OUTB (devc, 0, io_Status (devc));	/* Clear interrupt status */

  if (status & 0x01)
    {
      if (devc->model != MD_1848)
	{
	  alt_stat = ad_read (devc, 24);
	}

      /* Acknowledge the intr before proceeding */
      if (devc->model != MD_1848)
	ad_write (devc, 24, ad_read (devc, 24) & ~alt_stat);	/* Selective ack */

      if (devc->open_mode & OPEN_READ && devc->audio_mode & PCM_ENABLE_INPUT
	  && alt_stat & 0x20)
	{
	  oss_audio_inputintr (devc->record_dev, 0);
	}

      if (devc->open_mode & OPEN_WRITE && devc->audio_mode & PCM_ENABLE_OUTPUT
	  && alt_stat & 0x10)
	{
	  oss_audio_outputintr (devc->playback_dev, 1);
	}

      if (devc->model != MD_1848 && alt_stat & 0x40)	/* Timer interrupt */
	{
	  devc->timer_ticks++;
	}
    }

#if 1
/*
 * Sometimes playback or capture interrupts occur while a timer interrupt
 * is being handled. The interrupt will not be retriggered if we don't
 * handle it now. Check if an interrupt is still pending and restart
 * the handler in this case.
 */
  if (CODEC_INB (devc, io_Status (devc)) & 0x01 && cnt++ < 4)
    {
      goto interrupt_again;
    }
#endif
  return serviced;
}

int
oss_audiocs_attach (oss_device_t * osdev)
{
  unsigned int dw;
  int err;

static ddi_device_acc_attr_t acc_attr = {
  DDI_DEVICE_ATTR_V0,
  DDI_STRUCTURE_LE_ACC,
  DDI_STRICTORDER_ACC
};
  caddr_t addr;

  cs4231_devc_t *devc = osdev->devc;

  DDB(cmn_err(CE_CONT, "Entered oss_audiocs_attach()\n"));

  if ((devc = PMALLOC (osdev, sizeof (*devc))) == NULL)
    {
      cmn_err (CE_WARN, "Out of memory\n");
      return 0;
    }

  devc->osdev = osdev;
  osdev->devc = devc;
  devc->open_mode = 0;

  devc->chip_name = "Generic CS4231";

/*
 * Map I/O registers. This is done in Solaris specific way so the code
 * is not portable.
 */

  // Codec registers
  if ((err = ddi_regs_map_setup
       (osdev->dip, 0, &addr, 0, 16, &acc_attr,
	&CODEC_HNDL)) != DDI_SUCCESS)
     {
	     cmn_err(CE_WARN, "Cannot map codec registers, error=%d\n", err);
	     return 0;
     }
  devc->codec_base = (struct cs4231_pioregs*)addr;

  // Play registers
  if ((err = ddi_regs_map_setup
       (osdev->dip, 1, (caddr_t *)&devc->play_regs, 0, sizeof(cs4231_eb2regs_t), &acc_attr,
	&PLAY_HNDL)) != DDI_SUCCESS)
     {
	     cmn_err(CE_WARN, "Cannot map codec registers, error=%d\n", err);
	     return 0;
     }

  // Capture registers
  if ((err = ddi_regs_map_setup
       (osdev->dip, 2, (caddr_t *)&devc->rec_regs, 0, sizeof(cs4231_eb2regs_t), &acc_attr,
	&REC_HNDL)) != DDI_SUCCESS)
     {
	     cmn_err(CE_WARN, "Cannot map codec registers, error=%d\n", err);
	     return 0;
     }

  // Auxio register
  if ((err = ddi_regs_map_setup
       (osdev->dip, 3, &addr, 0, 4, &acc_attr,
	&AUXIO_HNDL)) != DDI_SUCCESS)
     {
	     cmn_err(CE_WARN, "Cannot map aixio register, error=%d\n", err);
	     return 0;
     }
  devc->auxio_base = (uint_t *)addr;

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);

  eb2_power(devc, 1);

  if (oss_register_interrupts (devc->osdev, 0, cs4231intr, NULL) < 0)
    {
      cmn_err (CE_WARN, "Unable to install interrupt handler\n");
      return 0;
    }

  if (!cs4231_detect(devc))
     return 0;

  oss_register_device (osdev, devc->chip_name);

  cs4231_init (devc);

  return 1;
}

int
oss_audiocs_detach (oss_device_t * osdev)
{
  cs4231_devc_t *devc = (cs4231_devc_t *) osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  oss_unregister_interrupts (devc->osdev);

  eb2_power(devc, 0);

  ddi_regs_map_free(&CODEC_HNDL);
  ddi_regs_map_free(&PLAY_HNDL);
  ddi_regs_map_free(&REC_HNDL);
  ddi_regs_map_free(&AUXIO_HNDL);

  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);

  oss_unregister_device (osdev);

  return 1;
}
