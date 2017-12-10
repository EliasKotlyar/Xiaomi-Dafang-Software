/*
 * Purpose: Driver for Creative Audigy LS audio controller
 *
 * This sound card has been sold under many different names.
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

#include "oss_audigyls_cfg.h"
#include "oss_pci.h"
#include "ac97.h"
#include "midi_core.h"
#include "remux.h"

#define DEFAULT_RATE 48000

#undef  USE_ITIMER

#define PCI_VENDOR_ID_CREATIVE 		0x1102
#define PCI_DEVICE_ID_CREATIVE_AUDIGYLS 0x0007

/*
 * Indirect registers
 */

#define PTBA	0x000
#define PTBS	0x001
#define PTCA	0x002
#define PFBA	0x004
#define PFBS	0x005
#define CPFA	0x006
#define PFEA	0x007
#define CPCAV	0x008
#define RFBA	0x010
#define RFBS	0x011
#define CRFA	0x012
#define CRCAV	0x013
#define CDL	0x020
#define SA	0x040
#define SCS3	0x041
#define SCS0	0x042
#define SCS1	0x043
#define SCS2	0x044
#define SPC	0x045
#define WMARK	0x046
#define SPSC	0x049
#define RCD	0x050		/* 0x50-0z5f */
#define P17RECSEL       0x060
#define P17RECVOLL      0x061
#define P17RECVOLH      0x062

#define HMIXMAP_SPDIF 0x63
#define SMIXMAP_SPDIF 0x64
#define MIXCTL_SPDIF 0x65
#define MIXVOL_SPDIF 0x66
#define HMIXMAP_I2S 0x67
#define SMIXMAP_I2S 0x68
#define MIXCTL_I2S 0x69
#define MIXVOL_I2S 0x6a

/* MIDI UART */
#define MUDATA	0x06c
#define MUCMDA	0x06d
#define MUDATB	0x06e
#define MUCMDB	0x06f

#define SRT	0x070
#define SRCTL	0x071
#define AUDCTL	0x072
#define CHIP_ID	0x074
#define AINT_ENABLE	0x075
#define AINT_STATUS	0x076
#define Wall192		0x077
#define Wall441		0x078
#define IT		0x079
#define SPI		0x07a
#define I2C_A		0x07b
#define I2C_0		0x07c
#define I2C_1		0x07d

/*
 * Global Interrupt bits
 */

#define IE 0x0c
#define INTR_PCI	(1<<  0)
#define INTR_TXA	(1<<  1)
#define INTR_RXA	(1<<  2)
#define INTR_IT1	(1<<  3)
#define INTR_IT2	(1<<  4)
#define INTR_SS_	(1<<  5)
#define INTR_SRT	(1<<  6)
#define INTR_GP 	(1<<  7)
#define INTR_AI 	(1<<  8)
#define INTR_I2Cdac 	(1<<  9)
#define INTR_I2CEE 	(1<< 10)
#define INTR_SPI 	(1<< 11)
#define INTR_SPF 	(1<< 12)
#define INTR_SUO 	(1<< 13)
#define INTR_SUI 	(1<< 14)
#define INTR_TXB 	(1<< 16)
#define INTR_RXB 	(1<< 17)

/*
 * Audio interrupt bits
 */

#define AI_PFH		(1<<  0)
#define AI_PFF		(1<<  4)
#define AI_TFH		(1<<  8)
#define AI_TFF		(1<< 12)
#define AI_RFH		(1<< 16)
#define AI_RFF		(1<< 20)
#define AI_EAI		(1<< 24)


#define MAX_PORTC	3

extern int audigyls_spdif_enable;

typedef struct
{
  int audio_dev;
  int play_port;
  int rec_port;
  int open_mode;
  int trigger_bits;
  int audio_enabled;
  int channels;
  int bits;
  int speed;
#ifdef USE_ITIMER
  int frag_192khz;
#endif
  int port_type;
  int play_ptr, rec_ptr;
}
audigyls_portc;

typedef struct
{
  oss_device_t *osdev;
  oss_native_word base;
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;
  char *card_name;
  unsigned int subvendor;
  int rec_src;			/* record channel src spdif/i2s/ac97/src */
#define RECSEL_SPDIFOUT   0
#define RECSEL_I2SOUT     1
#define RECSEL_SPDIFIN    2
#define RECSEL_I2SIN      3
#define RECSEL_AC97       4
#define RECSEL_SRC        5

  int mixer_dev;
  ac97_devc ac97devc;
  int has_ac97;
  int spread;			/* copy front to surr/center channels */
  int loopback;			/* record channel input from /dev/dspXX */
  int input_source;		/* input from mic/line/aux/etc */
  int captmon;         /* hear what you record*/ 
  int fbvol;         /* recording monitor volume */ 
/*
 * UART
 */
  oss_midi_inputbyte_t midi_input_intr;
  int midi_opened, midi_disabled;
  volatile unsigned char input_byte;
  int midi_dev;
  int mpu_attached;
  int playvol[4];
  int recvol;
  audigyls_portc portc[MAX_PORTC];

}
audigyls_devc;


static void audigylsuartintr (audigyls_devc * devc);

static unsigned int
read_reg (audigyls_devc * devc, int reg, int chn)
{
  oss_native_word flags;
  unsigned int val;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  OUTL (devc->osdev, (reg << 16) | (chn & 0xffff), devc->base + 0x00);	/* Pointer */
  val = INL (devc->osdev, devc->base + 0x04);	/* Data */
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);

/*printk("Read reg %03x (ch %d) = %08x\n", reg, chn, val);*/
  return val;
}

static void
write_reg (audigyls_devc * devc, int reg, int chn, unsigned int value)
{
  oss_native_word flags;

  /*printk("Write reg %03x (ch %d) = %08x\n", reg, chn, value); */
  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  OUTL (devc->osdev, (reg << 16) | (chn & 0xffff), devc->base + 0x00);	/* Pointer */
  OUTL (devc->osdev, value, devc->base + 0x04);	/* Data */
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
#if 0
  {
    char tmp[100];
    int ok = 1;
    sprintf (tmp, "@w%d %04x/%s %x", chn, reg, emu_regname (reg, &ok), value);
    if (ok)
      oss_do_timing (tmp);
  }
#endif
}


static int
audigyls_ac97_read (void *devc_, int wAddr)
{
  audigyls_devc *devc = devc_;
  int dtemp = 0, i;

  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  OUTB (devc->osdev, wAddr, devc->base + 0x1e);
  for (i = 0; i < 10000; i++)
    if (INB (devc->osdev, devc->base + 0x1e) & 0x80)
      break;
  if (i == 10000)
    {
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return OSS_EIO;
    }
  dtemp = INW (devc->osdev, devc->base + 0x1c);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);

  return dtemp & 0xffff;
}

static int
audigyls_ac97_write (void *devc_, int wAddr, int wData)
{
  audigyls_devc *devc = devc_;
  oss_native_word flags;
  int i;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  OUTB (devc->osdev, wAddr, devc->base + 0x1e);
  for (i = 0; i < 10000; i++)
    if (INB (devc->osdev, devc->base + 0x1e) & 0x80)
      break;
  if (i == 10000)
    {
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return OSS_EIO;
    }
  OUTW (devc->osdev, wData, devc->base + 0x1c);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);

  return 0;
}

static void
check_recording_intr (audigyls_devc * devc, audigyls_portc * portc)
{
#if 1
  int pos, n = 0;
  dmap_p dmap;

  dmap = audio_engines[portc->audio_dev]->dmap_in;

  pos = read_reg (devc, CRFA, portc->rec_port);
  pos /= dmap->fragment_size;
  while (dmap_get_qtail (dmap) != pos && n++ < dmap->nfrags)
#endif
    oss_audio_inputintr (portc->audio_dev, 0);
}

static void
check_playback_intr (audigyls_devc * devc, audigyls_portc * portc)
{
#if 1
  int pos, n = 0;
  dmap_p dmap;

  dmap = audio_engines[portc->audio_dev]->dmap_out;

  pos = read_reg (devc, CPFA, portc->play_port);
  pos /= dmap->fragment_size;
  while (dmap_get_qhead (dmap) != pos && n++ < dmap->nfrags)
#endif
    oss_audio_outputintr (portc->audio_dev, 0);
}

static int
audigylsintr (oss_device_t * osdev)
{
  int serviced = 0;
  unsigned int status;
  unsigned int astatus = 0;
  int i;
  audigyls_devc *devc = osdev->devc;
  audigyls_portc *portc;
  oss_native_word flags;

  flags = 0;			/* To fix compiler warnings about unused variable */
  MUTEX_ENTER (devc->mutex, flags);

  status = INL (devc->osdev, devc->base + 0x08);

  if (status & INTR_RXA)	/* MIDI RX interrupt */
    {
      audigylsuartintr (devc);
    }

  if (status & INTR_AI)
    {
      astatus = read_reg (devc, AINT_STATUS, 0);
      for (i = 0; i < MAX_PORTC; i++)
	{
	  portc = &devc->portc[i];

	  if ((portc->trigger_bits & PCM_ENABLE_OUTPUT) &&
	      (astatus & ((AI_PFF | AI_PFH) << portc->play_port)))
	    {
	      dmap_t *dmap = audio_engines[portc->audio_dev]->dmap_out;

	      if (astatus & (AI_PFF << portc->play_port))
		portc->play_ptr = 0;
	      if (astatus & (AI_PFH << portc->play_port))
		portc->play_ptr = dmap->bytes_in_use / 2;

	      oss_audio_outputintr (portc->audio_dev, 0);
	    }
	  if ((portc->trigger_bits & PCM_ENABLE_INPUT) &&
	      (astatus & ((AI_RFF | AI_RFH) << portc->rec_port)))
	    {
	      dmap_t *dmap = audio_engines[portc->audio_dev]->dmap_in;

	      if (astatus & (AI_RFF << portc->rec_port))
		portc->rec_ptr = 0;
	      if (astatus & (AI_RFH << portc->rec_port))
		portc->rec_ptr = dmap->bytes_in_use / 2;

	      oss_audio_inputintr (portc->audio_dev, 0);
	    }
	}
      write_reg (devc, AINT_STATUS, 0, astatus);
    }

  if (status & INTR_IT1)
    {
      for (i = 0; i < MAX_PORTC; i++)
	{
	  portc = &devc->portc[i];

	  if ((portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    check_playback_intr (devc, portc);

	  if ((portc->trigger_bits & PCM_ENABLE_INPUT))
	    check_recording_intr (devc, portc);
	}
    }

  serviced = 1;
  OUTL (devc->osdev, status, devc->base + 0x08);	/* Acknowledge */
  MUTEX_EXIT (devc->mutex, flags);
  return serviced;
}

static int
audigyls_set_rate (int dev, int arg)
{
  audigyls_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->speed;

  if (audio_engines[dev]->flags & ADEV_FIXEDRATE)
    arg = DEFAULT_RATE;

  if (arg != 44100 && arg != 48000 && arg != 96000 && arg != 192000)
    arg = 48000;

  portc->speed = arg;
  return portc->speed;
}

static short
audigyls_set_channels (int dev, short arg)
{
  audigyls_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->channels;

  if ((arg == 1))
    arg = 2;

  if (portc->open_mode & OPEN_READ)
    return portc->channels = 2;

  if (arg != 1 && arg != 2)
    return portc->channels = 2;
  return portc->channels = arg;
}

static unsigned int
audigyls_set_format (int dev, unsigned int arg)
{
  audigyls_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->bits;

  if (arg != AFMT_AC3 && arg != AFMT_S16_LE)
    return portc->bits = AFMT_S16_LE;

  return portc->bits = arg;
}

/*ARGSUSED*/
static int
audigyls_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void audigyls_trigger (int dev, int state);

static void
audigyls_reset (int dev)
{
  audigyls_trigger (dev, 0);
}

/*ARGSUSED*/
static int
audigyls_open (int dev, int mode, int open_flags)
{
  audigyls_portc *portc = audio_engines[dev]->portc;
  audigyls_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }
  portc->open_mode = mode;
  portc->audio_enabled = ~mode;
  portc->play_ptr = portc->rec_ptr = 0;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

static void
audigyls_close (int dev, int mode)
{
  audigyls_portc *portc = audio_engines[dev]->portc;

  audigyls_reset (dev);
  portc->open_mode = 0;
  portc->audio_enabled &= ~mode;
}

/*ARGSUSED*/
static void
audigyls_output_block (int dev, oss_native_word buf, int count, int fragsize,
		       int intrflag)
{
}

/*ARGSUSED*/
static void
audigyls_start_input (int dev, oss_native_word buf, int count, int fragsize,
		      int intrflag)
{
  audigyls_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
}

#ifdef USE_ITIMER
static void
check_itimer (audigyls_devc * devc)
{
  int i;
  unsigned int t = 0x1fffffff;
  audigyls_portc *portc;
  int tmp;

  for (i = 0; i < MAX_PORTC; i++)
    {
      portc = &devc->portc[i];
      if (portc->frag_192khz != 0 && portc->frag_192khz < t)
	t = portc->frag_192khz;
    }
  if (t == 0x1fffffff)		/* No audio devices active */
    {
      tmp = INL (devc->osdev, devc->base + IE);
      tmp &= ~INTR_IT1;
      OUTL (devc->osdev, tmp, devc->base + IE);
    }
  else
    {
      t /= 16;
      if (t < 1)
	t = 1;
      write_reg (devc, IT, 0, t);
      tmp = INL (devc->osdev, devc->base + IE);
      tmp |= INTR_IT1;
      OUTL (devc->osdev, tmp, devc->base + IE);
    }
}

static void
adjust_itimer (audigyls_devc * devc, audigyls_portc * portc, dmap_p dmap)
{
  unsigned int t;

  /* Compute byte rate */

  t = portc->speed * portc->channels;

  switch (portc->bits)
    {
    case AFMT_S16_LE:
    case AFMT_S16_BE:
    case AFMT_AC3:
      t *= 2;
      break;

    case AFMT_S32_LE:
    case AFMT_S32_BE:
    case AFMT_S24_LE:
    case AFMT_S24_BE:
      t *= 4;
      break;
    }

/* Compute the number of 192kHz ticks per fragment */

  t = (dmap->fragment_size * 192000) / t;	/* msecs / fragment */
  if (t < 1)
    t = 1;

  portc->frag_192khz = t;
  check_itimer (devc);
}
#endif

static void
audigyls_trigger (int dev, int state)
{
  audigyls_devc *devc = audio_engines[dev]->devc;
  audigyls_portc *portc = audio_engines[dev]->portc;
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
	      tmp = read_reg (devc, SA, 0);
	      tmp |= 1 << portc->play_port;
	      write_reg (devc, SA, 0, tmp);
#ifdef USE_ITIMER
	      check_itimer (devc);
#else
	      tmp = read_reg (devc, AINT_ENABLE, 0);
	      tmp |= ((AI_PFH | AI_PFF) << portc->play_port);
	      write_reg (devc, AINT_ENABLE, 0, tmp);
#endif
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
#ifdef USE_ITIMER
	      portc->frag_192khz = 0;
	      check_itimer (devc);
#endif
	      /* Disable Play channel */
	      tmp = read_reg (devc, SA, 0);
	      tmp &= ~(1 << portc->play_port);
	      write_reg (devc, SA, 0, tmp);
#ifndef USE_ITIMER
	      tmp = read_reg (devc, AINT_ENABLE, 0);
	      tmp &= ~((AI_PFH | AI_PFF) << portc->play_port);
	      write_reg (devc, AINT_ENABLE, 0, tmp);
#endif
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
	      /* Enable Rec Channel */
	      tmp = read_reg (devc, SA, 0);
	      tmp |= 0x100 << portc->rec_port;	/* enable record */
	      write_reg (devc, SA, 0, tmp);
#ifdef USE_ITIMER
	      check_itimer (devc);
#else
	      tmp = read_reg (devc, AINT_ENABLE, 0);
	      tmp |= ((AI_RFF | AI_RFH) << portc->rec_port);
	      write_reg (devc, AINT_ENABLE, 0, tmp);
#endif
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
#ifdef USE_ITIMER
	      portc->frag_192khz = 0;
	      check_itimer (devc);
#endif
	      /* disable channel */
	      tmp = read_reg (devc, SA, 0);
	      tmp &= ~(0x100 << portc->rec_port);
	      write_reg (devc, SA, 0, tmp);
#ifndef USE_ITIMER
	      tmp = read_reg (devc, AINT_ENABLE, 0);
	      tmp &= ~((AI_RFF | AI_RFH) << portc->rec_port);
	      write_reg (devc, AINT_ENABLE, 0, tmp);
#endif
	    }
	}
    }

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
static int
audigyls_prepare_for_input (int dev, int bsize, int bcount)
{
  unsigned int tmp, recmap, reg;
   /*LINTED*/ unsigned int oversample;
  audigyls_devc *devc = audio_engines[dev]->devc;
  audigyls_portc *portc = audio_engines[dev]->portc;
  dmap_p dmap = audio_engines[dev]->dmap_in;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  write_reg (devc, CRFA, portc->rec_port, 0);
  write_reg (devc, CRCAV, portc->rec_port, 0);

  write_reg (devc, RFBA, portc->rec_port, dmap->dmabuf_phys);
  write_reg (devc, RFBS, portc->rec_port, (dmap->bytes_in_use) << 16);

  /* set 16/24 bits */
  tmp = INL (devc->osdev, devc->base + 0x14);
  if (portc->bits == AFMT_S16_LE)
    tmp &= ~0x400;		/*16 bit */
  else
    tmp |= 0x400;		/*24 bit */
  OUTL (devc->osdev, tmp, devc->base + 0x14);

  /* set recording speed */
  reg = read_reg (devc, SRCTL, 0) & ~0xc000;

  switch (portc->speed)
    {
    case 48000:
      reg |= 0x0;
      oversample = 0x2;
      break;

    case 96000:
      reg |= 0x8000;
      oversample = 0xa;
      break;

    case 192000:
      reg |= 0xc000;
      oversample = 0xa;
      break;

    default:
      reg |= 0;
      oversample = 0x2;
      break;
    }

  write_reg (devc, SRCTL, 0, reg);
/*  audigyls_i2c_write(devc, 0xc, oversample);*/

/* setup record input */
  if (devc->loopback)
    {
      devc->rec_src = RECSEL_I2SOUT;
      recmap = 0;
    }
  else
    {
      if (devc->has_ac97)
	{
	  devc->rec_src = RECSEL_AC97;	/* audigy LS */
	}
      else
	{
	  devc->rec_src = RECSEL_I2SIN;	/* sb 7.1 value */
	}
      recmap = 0x00; 
    }
  tmp = recmap;			/* default record input map */
  tmp |= devc->rec_src << 28 | devc->rec_src << 24 | devc->rec_src << 20 | devc->rec_src << 16; 
//write_reg (devc, SMIXMAP_SPDIF, 0, 0x76767676); 
  write_reg (devc, P17RECSEL, 0, tmp);
  portc->audio_enabled &= ~PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;

#ifdef USE_ITIMER
  adjust_itimer (devc, portc, dmap);
#endif

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

/*ARGSUSED*/
static int
audigyls_prepare_for_output (int dev, int bsize, int bcount)
{
  audigyls_devc *devc = audio_engines[dev]->devc;
  audigyls_portc *portc = audio_engines[dev]->portc;
  dmap_p dmap = audio_engines[dev]->dmap_out;
  unsigned int tmp, reg;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->bits == AFMT_AC3)
    portc->channels = 2;

  write_reg (devc, PTBA, portc->play_port, 0);
  write_reg (devc, PTBS, portc->play_port, 0);
  write_reg (devc, PTCA, portc->play_port, 0);

  write_reg (devc, CPFA, portc->play_port, 0);
  write_reg (devc, PFEA, portc->play_port, 0);
  write_reg (devc, CPCAV, portc->play_port, 0);


  /* set 16/24 bits */
  tmp = INL (devc->osdev, devc->base + 0x14);
  if (portc->bits == AFMT_S16_LE)
    tmp &= ~0x800;		/*16 bit */
  else
    tmp |= 0x800;		/*24 bit */
  OUTL (devc->osdev, tmp, devc->base + 0x14);

  /* set playback rate */
  tmp = read_reg (devc, SA, 0) & ~0xff0000;
  reg = read_reg (devc, SRCTL, 0) & ~0x0303000f;
#if 0
  switch (portc->speed)
    {
    case 48000:
      tmp |= 0;
      reg |= 0;
      break;

    case 44100:
      tmp |= 0x10000;
      reg |= 0x01010005;
      break;

    case 96000:
      tmp |= 0x20000;
      reg |= 0x0202000a;
      break;

    case 192000:
      tmp |= 0x30000;
      reg |= 0x0303000f;
      break;

    default:
      tmp |= 0;			/* default is 48000 */
      reg |= 0;
      break;
    }
#endif
  write_reg (devc, SA, 0, tmp << (portc->play_port * 2));
  write_reg (devc, SRCTL, 0, reg);

  /* Single buffering mode */
  write_reg (devc, PFBA, portc->play_port, dmap->dmabuf_phys);
  write_reg (devc, PFBS, portc->play_port, (dmap->bytes_in_use) << 16);

  if (audigyls_spdif_enable)
    {
      if (portc->bits == AFMT_AC3)
	{
	  audigyls_ac97_write (devc, 0x1c, 0x8000);
	  write_reg (devc, SCS3, 0, 0x02108006);	/* Non Audio */
#if 0
	  write_reg (devc, SCS0, 0, 0x02108006);	/* Non Audio */
	  write_reg (devc, SCS1, 0, 0x02108006);	/* Non Audio */
	  write_reg (devc, SCS2, 0, 0x02108006);	/* Non Audio */
#endif
	}
      else
	{
	  write_reg (devc, SCS3, 0, 0x02108004);	/* Audio */
#if 0
	  write_reg (devc, SCS0, 0, 0x02108004);	/* Audio */
	  write_reg (devc, SCS1, 0, 0x02108004);	/* Audio */
	  write_reg (devc, SCS2, 0, 0x02108004);	/* Audio */
#endif
	}
    }
  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;

#ifdef USE_ITIMER
  adjust_itimer (devc, portc, dmap);
#endif

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

static int
audigyls_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  unsigned int p = 0;
  audigyls_portc *portc = audio_engines[dev]->portc;

#if 1
  audigyls_devc *devc = audio_engines[dev]->devc;

  if (direction == PCM_ENABLE_OUTPUT)
    {
      p = read_reg (devc, CPFA, portc->play_port);
    }

  if (direction == PCM_ENABLE_INPUT)
    {
      p = read_reg (devc, CRFA, portc->rec_port);
    }

  /*
   * Round to the nearest fragment boundary.
   */
  p = (p + dmap->fragment_size / 2);
  p = (p / dmap->fragment_size) * dmap->fragment_size;
#else
  if (direction == PCM_ENABLE_OUTPUT)
    {
      return portc->play_ptr;
    }

  if (direction == PCM_ENABLE_INPUT)
    {
      return portc->rec_ptr;
    }
#endif

  return p % dmap->bytes_in_use;
}

static audiodrv_t audigyls_audio_driver = {
  audigyls_open,
  audigyls_close,
  audigyls_output_block,
  audigyls_start_input,
  audigyls_ioctl,
  audigyls_prepare_for_input,
  audigyls_prepare_for_output,
  audigyls_reset,
  NULL,
  NULL,
  NULL,
  NULL,
  audigyls_trigger,
  audigyls_set_rate,
  audigyls_set_format,
  audigyls_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,				/* audigyls_alloc_buffer, */
  NULL,				/* audigyls_free_buffer */
  NULL,
  NULL,
  audigyls_get_buffer_pointer
};


static __inline__ int
audigylsuart_status (audigyls_devc * devc)
{
  return read_reg (devc, MUCMDA, 0);
}

#define input_avail(devc) (!(audigylsuart_status(devc)&INPUT_AVAIL))
#define output_ready(devc)      (!(audigylsuart_status(devc)&OUTPUT_READY))
static void
audigylsuart_cmd (audigyls_devc * devc, unsigned char cmd)
{
  write_reg (devc, MUCMDA, 0, cmd);
}

static __inline__ int
audigylsuart_read (audigyls_devc * devc)
{
  return read_reg (devc, MUDATA, 0);
}

static __inline__ void
audigylsuart_write (audigyls_devc * devc, unsigned char byte)
{
  write_reg (devc, MUDATA, 0, byte);
}

#define OUTPUT_READY    0x40
#define INPUT_AVAIL     0x80
#define MPU_ACK         0xFE
#define MPU_RESET       0xFF
#define UART_MODE_ON    0x3F

static int reset_audigylsuart (audigyls_devc * devc);
static void enter_uart_mode (audigyls_devc * devc);

static void
audigylsuart_input_loop (audigyls_devc * devc)
{
  while (input_avail (devc))
    {
      unsigned char c = audigylsuart_read (devc);

      if (c == MPU_ACK)
	devc->input_byte = c;
      else if (devc->midi_opened & OPEN_READ && devc->midi_input_intr)
	devc->midi_input_intr (devc->midi_dev, c);
    }
}

static void
audigylsuartintr (audigyls_devc * devc)
{
  audigylsuart_input_loop (devc);
}

/*ARGSUSED*/
static int
audigylsuart_open (int dev, int mode, oss_midi_inputbyte_t inputbyte,
		   oss_midi_inputbuf_t inputbuf,
		   oss_midi_outputintr_t outputintr)
{
  audigyls_devc *devc = (audigyls_devc *) midi_devs[dev]->devc;

  if (devc->midi_opened)
    {
      return OSS_EBUSY;
    }

  while (input_avail (devc))
    audigylsuart_read (devc);

  devc->midi_input_intr = inputbyte;
  devc->midi_opened = mode;
  enter_uart_mode (devc);
  devc->midi_disabled = 0;

  return 0;
}

/*ARGSUSED*/
static void
audigylsuart_close (int dev, int mode)
{
  audigyls_devc *devc = (audigyls_devc *) midi_devs[dev]->devc;
  reset_audigylsuart (devc);
  oss_udelay (10);
  enter_uart_mode (devc);
  reset_audigylsuart (devc);
  devc->midi_opened = 0;
}


static int
audigylsuart_out (int dev, unsigned char midi_byte)
{
  int timeout;
  audigyls_devc *devc = (audigyls_devc *) midi_devs[dev]->devc;
  oss_native_word flags;

  /*
   * Test for input since pending input seems to block the output.
   */

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (input_avail (devc))
    audigylsuart_input_loop (devc);

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  /*
   * Sometimes it takes about 130000 loops before the output becomes ready
   * (After reset). Normally it takes just about 10 loops.
   */

  for (timeout = 130000; timeout > 0 && !output_ready (devc); timeout--);

  if (!output_ready (devc))
    {
      cmn_err (CE_WARN, "UART timeout - Device not responding\n");
      devc->midi_disabled = 1;
      reset_audigylsuart (devc);
      enter_uart_mode (devc);
      return 1;
    }

  audigylsuart_write (devc, midi_byte);
  return 1;
}

/*ARGSUSED*/
static int
audigylsuart_ioctl (int dev, unsigned cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static midi_driver_t audigyls_midi_driver = {
  audigylsuart_open,
  audigylsuart_close,
  audigylsuart_ioctl,
  audigylsuart_out,
};

static void
enter_uart_mode (audigyls_devc * devc)
{
  int ok, timeout;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  for (timeout = 30000; timeout > 0 && !output_ready (devc); timeout--);

  devc->input_byte = 0;
  audigylsuart_cmd (devc, UART_MODE_ON);

  ok = 0;
  for (timeout = 50000; timeout > 0 && !ok; timeout--)
    if (devc->input_byte == MPU_ACK)
      ok = 1;
    else if (input_avail (devc))
      if (audigylsuart_read (devc) == MPU_ACK)
	ok = 1;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}


void
attach_audigylsuart (audigyls_devc * devc)
{
  enter_uart_mode (devc);
  devc->midi_dev = oss_install_mididev (OSS_MIDI_DRIVER_VERSION, "AUDIGYLS", "AudigyLS UART", &audigyls_midi_driver, sizeof (midi_driver_t),
					0, devc, devc->osdev);
  devc->midi_opened = 0;
}

static int
reset_audigylsuart (audigyls_devc * devc)
{
  int ok, timeout, n;

  /*
   * Send the RESET command. Try again if no success at the first time.
   */

  ok = 0;

  for (n = 0; n < 2 && !ok; n++)
    {
      for (timeout = 30000; timeout > 0 && !output_ready (devc); timeout--);

      devc->input_byte = 0;
      audigylsuart_cmd (devc, MPU_RESET);

      /*
       * Wait at least 25 msec. This method is not accurate so let's make the
       * loop bit longer. Cannot sleep since this is called during boot.
       */

      for (timeout = 50000; timeout > 0 && !ok; timeout--)
	if (devc->input_byte == MPU_ACK)	/* Interrupt */
	  ok = 1;
	else if (input_avail (devc))
	  if (audigylsuart_read (devc) == MPU_ACK)
	    ok = 1;

    }



  if (ok)
    audigylsuart_input_loop (devc);	/*
					 * Flush input before enabling interrupts
					 */

  return ok;
}


int
probe_audigylsuart (audigyls_devc * devc)
{
  int ok = 0;
  oss_native_word flags;

  DDB (cmn_err (CE_CONT, "Entered probe_audigylsuart\n"));

  devc->midi_input_intr = NULL;
  devc->midi_opened = 0;
  devc->input_byte = 0;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  ok = reset_audigylsuart (devc);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  if (ok)
    {
      DDB (cmn_err (CE_CONT, "Reset UART401 OK\n"));
    }
  else
    {
      DDB (cmn_err
	   (CE_CONT, "Reset UART401 failed (no hardware present?).\n"));
      DDB (cmn_err
	   (CE_CONT, "mpu401 status %02x\n", audigylsuart_status (devc)));
    }

  DDB (cmn_err (CE_CONT, "audigylsuart detected OK\n"));
  return ok;
}

void
unload_audigylsuart (audigyls_devc * devc)
{
  reset_audigylsuart (devc);
}


static void
attach_mpu (audigyls_devc * devc)
{
  devc->mpu_attached = 1;
  attach_audigylsuart (devc);
}

/* only for SBLive 7.1 */
int
audigyls_i2c_write (audigyls_devc * devc, int reg, int data)
{
  int i, timeout, tmp;


  tmp = (reg << 9 | data) << 16;	/* set the upper 16 bits */
  /* first write the command to the data reg */
  write_reg (devc, I2C_1, 0, tmp);
  for (i = 0; i < 20; i++)
    {
      tmp = read_reg (devc, I2C_A, 0) & ~0x6fe;
      /* see audigyls.pdf for bits */
      tmp |= 0x400 | 0x100 | 0x34;
      write_reg (devc, I2C_A, 0, tmp);
      /* now wait till controller sets valid bit (0x100) to 0 */
      timeout = 0;
       /*LINTED*/ while (1)
	{
	  tmp = read_reg (devc, I2C_A, 0);
	  if ((tmp & 0x100) == 0)
	    break;

	  if (timeout > 100)
	    break;

	  timeout++;
	}

      /* transaction aborted */
      if (tmp & 0x200)
	return 0;
    }
  return 1;
}

int
audigyls_spi_write (audigyls_devc * devc, int data)
{
  unsigned int orig;
  unsigned int tmp;
  int i, valid;

  tmp = read_reg (devc, SPI, 0);
  orig = (tmp & ~0x3ffff) | 0x30000;
  write_reg (devc, SPI, 0, orig | data);
  valid = 0;
  /* Wait for status bit to return to 0 */
  for (i = 0; i < 1000; i++)
    {
      oss_udelay (100);
      tmp = read_reg (devc, SPI, 0);
      if (!(tmp & 0x10000))
	{
	  valid = 1;
	  break;
	}
    }
  if (!valid)			/* Timed out */
    return 0;

  return 1;
}

static unsigned int
mix_scale (int left, int right, int bits)
{
  left = mix_cvt[left];
  right = mix_cvt[right];

  return ((left * ((1 << bits) - 1) / 100) << 8) | (right *
						    ((1 << bits) - 1) / 100);
}

static int
audigyls_set_volume (audigyls_devc * devc, int codecid, int value)
{
  audigyls_portc *portc = NULL;
  int left, right, i2s_vol;

  portc = &devc->portc[codecid];

  left = value & 0xff;
  right = (value >> 8) & 0xff;
  if (left > 100)
    left = 100;
  if (right > 100)
    right = 100;
  devc->playvol[codecid] = left | (right << 8);

  i2s_vol = 65535 - mix_scale (left, right, 8);
  write_reg (devc, MIXVOL_I2S, portc->play_port, (i2s_vol << 16));
  return devc->playvol[codecid];
}

int
audigyls_mix_control (int dev, int ctrl, unsigned int cmd, int value)
{
  audigyls_devc *devc = mixer_devs[dev]->hw_devc;
  int val;

  if (cmd == SNDCTL_MIX_READ)
    {
      value = 0;
      switch (ctrl)
	{
	case 1:		/* spread */
	  value = devc->spread;
	  break;

	case 2:		/* record what you hear */
	  value = devc->loopback;
	  break;

	case 3:
	  {
	    value = devc->recvol;
	  }
	  break;

	case 4: 
	  value = devc->input_source;
	  break;
	case 5: 
	  value = devc->captmon; 
	  break; 
	case 7: 
	  value = devc->fbvol; 
	  break; 
	}
    }
  if (cmd == SNDCTL_MIX_WRITE)
    {
      switch (ctrl)
	{
	case 1:		/* recording source */
	  devc->spread = value;
	  if (value)
	    write_reg (devc, HMIXMAP_I2S, 0, 0x10101010);
	  else
	    write_reg (devc, HMIXMAP_I2S, 0, 0x76543210);
	  break;

	case 2:		/* record what you hear */
	  devc->loopback = value;
	  break;

	case 3:
	  {
	    val = (255 - value) & 0xff;
	    write_reg (devc, P17RECVOLL, 0,
		       val << 24 | val << 16 | val << 8 | val);
	    write_reg (devc, P17RECVOLH, 0,
		       val << 24 | val << 16 | val << 8 | val);
      /* write_reg (devc, SRCTL, 1, 
            0xff << 24 | 0xff << 16 | val << 8 | val); */ 
	    devc->recvol = value & 0xff;
	  }
	  break;

	case 4:
	  { 
	  switch (value) 
	    {
          case 0:   /* for mic input remove GPIO */ 
	    {
	      OUTL (devc->osdev, INL (devc->osdev, devc->base + 0x18) | 0x400,
		    devc->base + 0x18);
	      audigyls_i2c_write (devc, 0x15, 0x2);	/* Mic */
	    }
            break; 
          case 1: 
	    {
	      OUTL (devc->osdev,
		    INL (devc->osdev, devc->base + 0x18) & ~0x400,
		    devc->base + 0x18);
	      audigyls_i2c_write (devc, 0x15, 0x4);	/* Line */
	    }
            break; 
          case 2: 
            { 
              OUTL (devc->osdev, 
              INL (devc->osdev, devc->base + 0x18) & ~0x400, 
              devc->base + 0x18); 
              audigyls_i2c_write (devc, 0x15, 0x8);   /* Aux */ 
            } 
          break; 
	    } 
	  devc->input_source = value;
	} 
	break; 
	case 5: 
	  { 
	    devc->captmon = value; 
	    /* Send analog capture to front speakers */ 
	    if (value) 
	      write_reg (devc, SMIXMAP_I2S, 0, 0x76767676); 
	    else 
	     write_reg (devc, SMIXMAP_I2S, 0, 0x10101010); 
	  } 
	break; 
	case 7: 
	  { 
	    /*Set recording monitor volume */ 
	    val = (255 - value) & 0xff; 
	    write_reg (devc, SRCTL, 1, val << 8 | val); 
	    devc->fbvol = value & 0xff; 
	  } 
	break;
	}
    }
  return value;
}

static int
audigyls_mix_init (int dev)
{
  int group, err;
  audigyls_devc *devc = mixer_devs[dev]->hw_devc;

  if ((group = mixer_ext_create_group (dev, 0, "EXT")) < 0)
    return group;

  if ((err = mixer_ext_create_control (dev, group, 1, audigyls_mix_control,
				       MIXT_ONOFF, "Spread", 1,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group, 2, audigyls_mix_control,
				       MIXT_ONOFF, "LOOPBACK", 1,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;


  if ((err = mixer_ext_create_control (dev, group, 3, audigyls_mix_control,
				       MIXT_MONOSLIDER, "RECORDVOL", 255,
				       MIXF_READABLE | MIXF_WRITEABLE |
				       MIXF_RECVOL)) < 0)
    return err;

  if (!devc->has_ac97)
    {
      if ((err =
	   mixer_ext_create_control (dev, group, 4, audigyls_mix_control,
				     MIXT_ENUM, "RECORDSRC", 3, 
				     MIXF_READABLE | MIXF_WRITEABLE)) < 0)
	return err;
      mixer_ext_set_strings (dev, err, "MIC LINE AUX", 0); 
    }
  if ((err = mixer_ext_create_control (dev, group, 7, audigyls_mix_control, 
				       MIXT_MONOSLIDER, "monitorvol", 255,
				       MIXF_READABLE | MIXF_WRITEABLE |
				       MIXF_RECVOL)) < 0)
    return err;
  if ((err = mixer_ext_create_control (dev, group, 5, audigyls_mix_control, 
				       MIXT_ONOFF, "RecMon", 1, 
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0) 
    return err; 
  return 0;
}

static const int bindings[MAX_PORTC] = {
  DSP_BIND_FRONT,
  DSP_BIND_CENTER_LFE,
  DSP_BIND_SURR
};

static int
install_audio_devices (audigyls_devc * devc)
{
  int i;
  int frontdev = -1;
  int adev, flags;
  int fmts = AFMT_S16_LE | AFMT_AC3;
  static char *names[] = {
    "AudigyLS front",
    "AudigyLS center/lfe",
    "AudigyLS surround"
  };

#if 0
  if (audigyls_spdif_enable == 1)
    n = 2;
#endif

  for (i = 0; i < MAX_PORTC; i++)
    {
      audigyls_portc *portc = &devc->portc[i];

      flags =
	ADEV_AUTOMODE | ADEV_16BITONLY | ADEV_STEREOONLY | ADEV_FIXEDRATE;

      switch (i)
	{
	case 0:
	  portc->play_port = 0;
	  portc->rec_port = 2;
	  flags |= ADEV_DUPLEX;
	  break;
	case 1:
	  portc->play_port = 1;
	  portc->rec_port = 2;
	  flags |= ADEV_NOINPUT;
	  break;
	case 2:
	  portc->play_port = 3;
	  portc->rec_port = 2;
	  flags |= ADEV_NOINPUT;
	  break;
	}

      if ((adev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
					devc->osdev,
					devc->osdev,
					names[i],
					&audigyls_audio_driver,
					sizeof (audiodrv_t),
					flags, fmts, devc, -1)) < 0)
	{
	  return 0;
	}

      if (i == 0)
	frontdev = adev;

      audio_engines[adev]->portc = portc;
      audio_engines[adev]->max_fragments = 2;
      audio_engines[adev]->dmabuf_alloc_flags |= DMABUF_SIZE_16BITS;
      audio_engines[adev]->rate_source = frontdev;
      audio_engines[adev]->mixer_dev = devc->mixer_dev;
      audio_engines[adev]->binding = bindings[i];
      if (audio_engines[adev]->flags & ADEV_FIXEDRATE)
	{
	  audio_engines[adev]->fixed_rate = DEFAULT_RATE;
	  audio_engines[adev]->min_rate = DEFAULT_RATE;
	  audio_engines[adev]->max_rate = DEFAULT_RATE;
	}
      else
	{
	  audio_engines[adev]->min_rate = 44100;
	  audio_engines[adev]->max_rate = 192000;
	}
      portc->audio_dev = adev;
      portc->open_mode = 0;
      devc->playvol[i] = 0x3030;
      devc->recvol = 128;
      portc->bits = AFMT_S16_LE;
    }

#ifdef USE_REMUX
  if (frontdev >= 0)
    {
      if (audigyls_spdif_enable && devc->has_ac97)
	remux_install ("AudigyLS 4.0 output", devc->osdev, frontdev,
		       frontdev + 2, -1, -1);
      else
	remux_install ("AudigyLS 5.1 output", devc->osdev, frontdev,
		       frontdev + 2, frontdev + 1, -1);
    }
#endif

#ifdef CONFIG_OSS_VMIX
	  if (frontdev >= 0)
	     vmix_attach_audiodev(devc->osdev, frontdev, -1, 0);
#endif
  return 1;
}

static void
select_out3_mode (audigyls_devc * devc, int mode)
{
  /*
   * Set the out3/spdif combo jack format.
   * mode0=analog rear/center, 1=spdif
   */

  if (mode == 0)
    {
      write_reg (devc, SPC, 0, 0x00000f00);
    }
  else
    {
      write_reg (devc, SPC, 0, 0x0000000f);
    }
}

/*ARGSUSED*/
static int
audigyls_mixer_ioctl (int dev, int audiodev, unsigned int cmd, ioctl_arg arg)
{
  audigyls_devc *devc = mixer_devs[dev]->devc;

  if (((cmd >> 8) & 0xff) == 'M')
    {
      int val;

      if (IOC_IS_OUTPUT (cmd))
	switch (cmd & 0xff)
	  {
	  case SOUND_MIXER_RECSRC:
	    return *arg = 0;
	    break;

	  case SOUND_MIXER_PCM:
	    val = *arg;
	    return *arg = audigyls_set_volume (devc, 0, val);

	  case SOUND_MIXER_CENTERVOL:
	    val = *arg;
	    return *arg = audigyls_set_volume (devc, 1, val);

	  case SOUND_MIXER_REARVOL:
	    val = *arg;
	    return *arg = audigyls_set_volume (devc, 2, val);
	  }
      else
	switch (cmd & 0xff)	/* Return Parameter */
	  {
	  case SOUND_MIXER_RECSRC:
	  case SOUND_MIXER_RECMASK:
	    return *arg = 0;
	    break;

	  case SOUND_MIXER_DEVMASK:
	    return *arg =
	      SOUND_MASK_PCM | SOUND_MASK_REARVOL | SOUND_MASK_CENTERVOL;
	    break;

	  case SOUND_MIXER_STEREODEVS:
	    return *arg =
	      SOUND_MASK_PCM | SOUND_MASK_REARVOL | SOUND_MASK_CENTERVOL;
	    break;

	  case SOUND_MIXER_CAPS:
	    return *arg = SOUND_CAP_EXCL_INPUT;
	    break;

	  case SOUND_MIXER_PCM:
	    return *arg = devc->playvol[0];
	    break;

	  case SOUND_MIXER_CENTERVOL:
	    return *arg = devc->playvol[2];
	    break;

	  case SOUND_MIXER_REARVOL:
	    return *arg = devc->playvol[3];
	    break;
	  }
    }
  else
    return *arg = 0;

  return OSS_EINVAL;
}

static mixer_driver_t audigyls_mixer_driver = {
  audigyls_mixer_ioctl
};

int
oss_audigyls_attach (oss_device_t * osdev)
{
  int tmp, err, i;
  unsigned char pci_irq_line, pci_revision;
  unsigned short pci_command, vendor, device;
  unsigned int pci_ioaddr;
  unsigned int subvendor;
  audigyls_devc *devc;

  static unsigned int spi_dac[] = {
    0x00ff, 0x02ff, 0x0400, 0x530, 0x0622, 0x08ff, 0x0aff, 0x0cff,
    0x0eff, 0x10ff, 0x1200, 0x1400, 0x1800, 0x1aff, 0x1cff,
    0x1e00,
  };

  DDB (cmn_err (CE_WARN, "Entered AUDIGYLS probe routine\n"));

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  if (vendor != PCI_VENDOR_ID_CREATIVE ||
      device != PCI_DEVICE_ID_CREATIVE_AUDIGYLS)
    return 0;

  pci_read_config_dword (osdev, 0x2c, &subvendor);
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
      cmn_err (CE_WARN, "IRQ not assigned by BIOS.\n");
      return 0;
    }

  if ((devc = PMALLOC (osdev, sizeof (*devc))) == NULL)
    {
      cmn_err (CE_WARN, "Out of memory\n");
      return 0;
    }

  devc->osdev = osdev;
  osdev->devc = devc;
  devc->card_name = "AudigyLS";
  devc->subvendor = subvendor;

  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_IO;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  devc->base = MAP_PCI_IOADDR (devc->osdev, 0, pci_ioaddr);
  devc->base &= ~0x3;

  MUTEX_INIT (osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (osdev, devc->low_mutex, MH_DRV + 1);

  oss_register_device (osdev, devc->card_name);

  if ((err =
       oss_register_interrupts (devc->osdev, 0, audigylsintr, NULL)) < 0)
    {
      cmn_err (CE_WARN, "Can't register interrupt handler, err=%d\n", err);
      return 0;
    }


/*
 * Init mixer
 */
  if (subvendor == 0x10021102)	/* original audigyls */
    {
      devc->mixer_dev = ac97_install (&devc->ac97devc, devc->card_name,
				      audigyls_ac97_read, audigyls_ac97_write,
				      devc, devc->osdev);
      devc->has_ac97 = 1;
      audigyls_ac97_write (devc, 0x1c, 0x8000);
    }
  else
    {
      devc->mixer_dev = oss_install_mixer (OSS_MIXER_DRIVER_VERSION,
					   devc->osdev,
					   devc->osdev,
					   "AudigyLS Mixer",
					   &audigyls_mixer_driver,
					   sizeof (mixer_driver_t), devc);
      devc->has_ac97 = 0;	/* no ac97 */
      mixer_devs[devc->mixer_dev]->hw_devc = devc;
      mixer_devs[devc->mixer_dev]->priority = 1;	/* Possible default mixer candidate */
    }

  mixer_ext_set_init_fn (devc->mixer_dev, audigyls_mix_init, 10);

#if 0
  write_reg (devc, SCS0, 0, 0x02108504);
  write_reg (devc, SCS1, 0, 0x02108504);
  write_reg (devc, SCS2, 0, 0x02108504);
#endif
  write_reg (devc, SCS3, 0, 0x02108504);

  write_reg (devc, AUDCTL, 0, 0x0f0f003f);	/* enable all outputs */

  select_out3_mode (devc, audigyls_spdif_enable);

/**********************************************************************
 * In P17, there's 8 GPIO pins.
 * GPIO register: 0x00XXYYZZ
 * XX: Configure GPIO to be either GPI (0) or GPO (1).
 * YY: GPO values, applicable if the pin is configure to be GPO.
 * ZZ: GPI values, applicable if the pin is configure to be GPI.
 * 
 * in SB570, pin 0-4 and 6 is used as GPO and pin 5 and 7 is used as GPI.
 * 
 * GPO0:
 * 1 ==> Analog output
 * 0 ==> Digital output
 * GPO1:
 * 1 ==> Enable output on card
 * 0 ==> Diable output on card
 * GPO2:
 * 1 ==> Enable Mic Bias and Mic Path
 * 0 ==> Disable Mic Bias and Mic Path
 * GPO3:
 * 1 ==> Disable SPDIF-IO output
 * 0 ==> Enable SPDIF-IO output
 * GPO4 and GPO6:
 * DAC sampling rate selection:
 * Not applicable to SB570 since DAC is controlled through SPI
 * GPI5:
 * 1 ==> Front Panel is not connected
 * 0 ==> Front Panel is connected
 * GPI7:
 * 1 ==> Front Panel Headphone is not connected
 * 0 ==> Front Panel Headphone is connected
 ***********************************************************/

  OUTL (devc->osdev, 0, devc->base + 0x18);	/* GPIO */
  if (devc->has_ac97)
    OUTL (devc->osdev, 0x005f03a3, devc->base + 0x18);
  else
    {
      /* for SBLive 7.1 */
      OUTL (devc->osdev, 0x005f4301, devc->base + 0x18);
      audigyls_i2c_write (devc, 0x15, 0x4);
      for (i = 0; i < (sizeof (spi_dac)/sizeof (spi_dac[0])); i++)
	{
	  audigyls_spi_write (devc, spi_dac[i]);
	}
    }

  OUTL (devc->osdev, INTR_PCI | INTR_RXA | INTR_AI, devc->base + IE);
  OUTL (devc->osdev, 0x00000009, devc->base + 0x14);	/* Enable audio */

  tmp = read_reg (devc, SRCTL, 0);
  if (devc->has_ac97)
    tmp |= 0xf0c81000;		/* record src0/src1 from ac97 */
  else
    tmp |= 0x50c81000;		/* record src0/src1 from I2SIN */
  write_reg (devc, SRCTL, 0, tmp);
  write_reg (devc, HMIXMAP_I2S, 0, 0x76543210);	/* default out route */
  install_audio_devices (devc);

  if (devc->has_ac97)		/* only attach midi for AudigyLS */
    attach_mpu (devc);

  return 1;
}

int
oss_audigyls_detach (oss_device_t * osdev)
{
  unsigned int status;
  audigyls_devc *devc = (audigyls_devc *) osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;


  write_reg (devc, SA, 0, 0);
  OUTL (devc->osdev, 0x00000000, devc->base + IE);	/* Interrupt disable */
  write_reg (devc, AINT_ENABLE, 0, 0);	/* Disable audio interrupts */
  status = INL (devc->osdev, devc->base + 0x08);
  OUTL (devc->osdev, status, devc->base + 0x08);	/* Acknowledge */
  oss_udelay (1000);
  if (devc->mpu_attached)
    unload_audigylsuart (devc);

  oss_unregister_interrupts (devc->osdev);

  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);
  UNMAP_PCI_IOADDR (devc->osdev, 0);

  oss_unregister_device (osdev);
  return 1;
}
