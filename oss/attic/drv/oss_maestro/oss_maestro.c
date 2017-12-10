/*
 * Purpose: Driver for ESS Maestro1/2 (PCI) audio controller
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

#include "oss_maestro_cfg.h"
#include "oss_pci.h"
#include "uart401.h"
#include "ac97.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define BYTE unsigned char
#define WORD unsigned short
#define DWORD unsigned int
#define HIWORD(dw) ((dw >> 16) & 0xffff)
#define LOWORD(dw) (dw & 0xffff)
#define HIBYTE(w) ((w >> 8) & 0xff)
#define LOBYTE(w) (w & 0xff)

#define gwPTBaseIO devc->base
#define outpw(p,d) OUTW(devc->osdev, d, p)

#define DISABLE 0
#define ENABLE  1

#define CHANNEL0    0x0
#define bAPURPlay   0x0
#define bAPULPlay   0x1

#define bAPULSrc  	0x2
#define bAPURSrc  	0x3
#define bAPULMix  	0x4
#define bAPURMix  	0x5


#define ESS_VENDOR_ID	0x1285
#define ESS_MAESTRO		0x0100
#define ESS2_VENDOR_ID	0x125d
#define ESS_MAESTRO2	0x1968
#define ESS_MAESTRO2E 	0x1978

#define WSETBIT(w, b) (w | (1 << b))
#define WMSKBIT(w, b) (w & ~(1 << b))
#define DWSETBIT(w, b) (w | (1 << b))
#define DWMSKBIT(w, b) (w & ~(1 << b))

static int SYSCLK;
#define ESSM_CFMT_STEREO     0x01
#define ESSM_CFMT_16BIT      0x02
#define ESSM_CFMT_MASK       0x03
#define ESSM_CFMT_ASHIFT     0
#define ESSM_CFMT_CSHIFT     4

#define MAXCOUNTDOWN 31
DWORD dwFrequencyTable[] = {
  512, 1024, 2048, 4096, 8192, 16384, 32768, 65536
};

#define IO_PT_CODEC_CMD     ( devc->base + 0x30 )
#define IO_PT_CODEC_STATUS  ( devc->base + 0x30 )
#define IO_PT_CODEC_DATA    ( devc->base + 0x32 )
#define IO_PT_CODEC_FORMATA  ( devc->base + 0x34 )
#define IO_PT_CODEC_FORMATB  ( devc->base + 0x36 )

#define _DST_MONO           0x00
#define _DST_STEREO         0x08

#define _DST_NONE           0x00
#define _DST_DAC            0x01
#define _DST_MODEM          0x02
#define _DST_RESERVED1      0x03
#define _DST_DIRECTSOUND    0x04
#define _DST_ASSP           0x05
#define _DST_RESERVED2      0x06
#define _DST_RESERVED3      0x07

#define PT101_MIXER_DEVS         (SOUND_MASK_LINE1 | SOUND_MASK_LINE2 | \
                                 SOUND_MASK_MIC | SOUND_MASK_VOLUME | \
                                 SOUND_MASK_LINE3 | \
				 SOUND_MASK_SYNTH | SOUND_MASK_CD | \
			         SOUND_MASK_LINE | SOUND_MASK_PCM | \
			         SOUND_MASK_IGAIN)

#define PT101_MIXER_RECDEVS      (SOUND_MASK_LINE | SOUND_MASK_MIC | \
				 SOUND_MASK_CD | SOUND_MASK_LINE1 | \
				 SOUND_MASK_LINE2 | SOUND_MASK_LINE3)

#define PT101_MIXER_STEREODEVS   (SOUND_MASK_LINE | SOUND_MASK_IGAIN | \
                                 SOUND_MASK_VOLUME | SOUND_MASK_SYNTH | \
                                 SOUND_MASK_CD |  \
                                 SOUND_MASK_PCM | SOUND_MASK_LINE1 | \
				 SOUND_MASK_LINE2 | SOUND_MASK_LINE3)

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

typedef struct maestro_portc
{
  int speed, bits, channels;
  int open_mode;
  int audiodev;
  int trigger_bits;
  int audio_enabled;
}
maestro_portc;

#define MAX_PORTC 2

typedef struct maestro_devc
{
  oss_device_t *osdev;
  oss_native_word base;
  int irq;
  int model;
  char *chip_name;
  int open_mode;
  /* Data table for virtualizing write-only registers */
  WORD wIDRRegDataTable[0x1F];
  WORD gwWCRegTable[0x200];
#define MD_MAESTRO1		1
#define MD_MAESTRO2		2
#define MD_MAESTRO2E		3
  unsigned short gpio;
  oss_mutex_t lock;
  oss_mutex_t low_lock;

  struct dmabuf
  {
    unsigned char *rawbuf;
    unsigned dmasize;
    oss_native_word base;	/* Offset for ptr */
  }
  dma_adc, dma_dac, dma_mix;
  int wApuBufferSize;
  unsigned char *dmapages;
  oss_native_word dmapages_phys;
  int dmalen;

  /* Mixer parameters */
  ac97_devc ac97devc;
  /* PT101 Mixer parameters */
  int my_mixer;
  int *levels;
  int recdevs;
  int recmask;

  maestro_portc portc[MAX_PORTC];
}
maestro_devc;

extern WORD wRdAPUReg (maestro_devc * devc, BYTE _bChannel, BYTE _bRegIndex);

static int
maestrointr (oss_device_t * osdev)
{
  maestro_devc *devc = (maestro_devc *) osdev->devc;
  maestro_portc *portc;
  int i;
  int serviced = 0;

  if (!(INW (devc->osdev, devc->base + 0x1A)))
    return 0;

  OUTW (devc->osdev, INW (devc->osdev, devc->base + 0x04) | 0x1,
	devc->base + 0x04);
  serviced = 1;

  /* ack all interrupts */
  OUTB (devc->osdev, 0xFF, devc->base + 0x1A);
  for (i = 0; i < MAX_PORTC; i++)
    {
      portc = &devc->portc[i];

      /* Handle Playback */
      if (portc->trigger_bits & PCM_ENABLE_OUTPUT)
	{
	  dmap_t *dmapout = audio_engines[portc->audiodev]->dmap_out;
	  WORD wApuCurrentPos;
	  int n;

	  wApuCurrentPos = wRdAPUReg (devc, bAPURPlay, 0x05);
	  wApuCurrentPos = (wApuCurrentPos - devc->dma_dac.base) & 0xFFFE;
	  wApuCurrentPos = (wApuCurrentPos % devc->wApuBufferSize) << 1;
	  wApuCurrentPos /= dmapout->fragment_size;	/*Actual qhead */
	  if (wApuCurrentPos == 0 || wApuCurrentPos > dmapout->nfrags)
	    wApuCurrentPos = 0;
	  n = 0;
	  while (dmap_get_qhead (dmapout) != wApuCurrentPos
		 && n++ < dmapout->nfrags)
	    oss_audio_outputintr (portc->audiodev, 0);
	}

      /* Handle recording */
      if (portc->trigger_bits & PCM_ENABLE_INPUT)
	{
	  dmap_t *dmapin = audio_engines[portc->audiodev]->dmap_in;
	  WORD wApuCurrentPos;
	  int n;

	  wApuCurrentPos = wRdAPUReg (devc, bAPULSrc, 0x05);
	  wApuCurrentPos = (wApuCurrentPos - devc->dma_adc.base) & 0xFFFE;
	  wApuCurrentPos = (wApuCurrentPos % devc->wApuBufferSize) << 1;
	  wApuCurrentPos /= dmapin->fragment_size;	/* Actual qtail */
	  if (wApuCurrentPos == 0 || wApuCurrentPos > dmapin->nfrags)
	    wApuCurrentPos = 0;
	  n = 0;
	  while (dmap_get_qtail (dmapin) != wApuCurrentPos
		 && n++ < dmapin->nfrags)
	    oss_audio_inputintr (portc->audiodev, 0);
	}
    }
  return serviced;
}

static int
ac97_read (void *devc_, int addr)
{
  maestro_devc *devc = devc_;
  int data, i;
  oss_native_word flags;
  int sanity = 10000;

  MUTEX_ENTER_IRQDISABLE (devc->low_lock, flags);

  for (i = 0; i < 100000; i++)
    if (!(INB (devc->osdev, devc->base + 0x30) & 0x01))
      break;
  OUTW (devc->osdev, addr | 0x80, devc->base + 0x30);

  while (INB (devc->osdev, devc->base + 0x30) & 1)
    {
      sanity--;
      if (!sanity)
	{
	  cmn_err (CE_WARN, "ac97 codec timeout - 0x%x.\n", addr);
	  MUTEX_EXIT_IRQRESTORE (devc->low_lock, flags);
	  return 0;
	}
    }

  data = INW (devc->osdev, devc->base + 0x32);
  oss_udelay (100);

  MUTEX_EXIT_IRQRESTORE (devc->low_lock, flags);
  return data & 0xffff;
}

static int
ac97_write (void *devc_, int addr, int data)
{
  maestro_devc *devc = devc_;
  int i;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_lock, flags);

  for (i = 0; i < 10000; i++)
    if (!(INB (devc->osdev, devc->base + 0x30) & 0x01))
      break;
  OUTW (devc->osdev, data & 0xffff, devc->base + 0x32);
  oss_udelay (100);
  OUTW (devc->osdev, (addr & 0x7f) & ~0x80, devc->base + 0x30);
  oss_udelay (100);

  MUTEX_EXIT_IRQRESTORE (devc->low_lock, flags);

  return 0;
}

/****************************************************
 *		PT101 CODEC Routines						*
 ****************************************************/

/************************************************************************/
/*                     PT101 CODEC Read									*/
/************************************************************************/

static int
pt101_set_recmask (maestro_devc * devc, int mask)
{
  int bits = 0;
  mask &= devc->recmask;

  mask = mask & (devc->recdevs ^ mask);	/* Pick the one recently turned on */

  if (!mask)
    return devc->recdevs;

  devc->recdevs = mask;

  switch (mask)
    {
    case SOUND_MASK_LINE:
      bits = 0x0000;
      break;
    case SOUND_MASK_MIC:
      bits = 0x0020;
      break;
    case SOUND_MASK_CD:
    case SOUND_MASK_LINE1:
      bits = 0x0040;
      break;
     /*CD*/ case SOUND_MASK_LINE2:
      bits = 0x0060;
      break;			/*Video */
    case SOUND_MASK_LINE3:
      bits = 0x0080;
      break;			/*Modem */
    default:			/* Unknown bit (combination) */
      mask = SOUND_MASK_MIC;
      bits = 0x0020;
    }

  ac97_write (devc, 0x02, bits);

  return devc->levels[31] = devc->recdevs;
}

static int
pt101_mixer_get (maestro_devc * devc, int dev)
{
  if (!((1 << dev) & PT101_MIXER_DEVS))
    return OSS_EINVAL;

  return devc->levels[dev];
}

static int
pt101_mixer_set (maestro_devc * devc, int dev, int value)
{

  int left, right, lvl;
  int lch, rch;
  static const char pt101_mix_map[101] = {
    15, 15, 15, 15, 15, 15, 14, 14, 14, 14, 14, 14, 13, 13, 13, 13, 13, 13,
    12, 12,
    12, 12, 12, 12, 11, 11, 11, 11, 11, 11, 10, 10, 10, 10, 10, 10, 9, 9, 9,
    9,
    9, 9, 8, 8, 8, 8, 8, 8, 7, 7, 7, 7, 7, 7, 6, 6, 6, 6, 6, 6,
    5, 5, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 2, 2,
    2, 2, 2, 2, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };


  if (!((1 << dev) & PT101_MIXER_DEVS))
    return OSS_EINVAL;

  if (!((1 << dev) & PT101_MIXER_STEREODEVS))
    {
      lvl = value & 0xff;
      if (lvl > 100)
	lvl = 100;
      lch = rch = lvl;
      value = lvl | (lvl << 8);
    }
  else
    {
      lch = value & 0xff;
      rch = (value >> 8) & 0xff;
      if (lch > 100)
	lch = 100;
      if (rch > 100)
	rch = 100;
      value = lch | (rch << 8);
    }

/* Now adjust the left and right channel to the mixer map*/
  left = pt101_mix_map[lch];
  right = pt101_mix_map[rch];

  switch (dev)
    {
    case SOUND_MIXER_CD:
    case SOUND_MIXER_LINE1:
      if (rch == 0)
	ac97_write (devc, 0x04, (ac97_read (devc, 0x04) & 0xFF7F) | 0x0080);
      else
	ac97_write (devc, 0x04, (ac97_read (devc, 0x04) & 0xFF7F));

      if (lch == 0)
	ac97_write (devc, 0x04, (ac97_read (devc, 0x04) & 0x7FFF) | 0x8000);
      else
	ac97_write (devc, 0x04, (ac97_read (devc, 0x04) & 0x7FFF));

      ac97_write (devc, 0x04, ((ac97_read (devc, 0x04) & 0xFFE1) | (right << 1)));	/*Right */
      ac97_write (devc, 0x04, ((ac97_read (devc, 0x04) & 0xE1FF) | (left << 9)));	/*Left */
      break;

    case SOUND_MIXER_LINE2:
      if (rch == 0)
	ac97_write (devc, 0x05, (ac97_read (devc, 0x05) & 0xFF7F) | 0x0080);
      else
	ac97_write (devc, 0x05, (ac97_read (devc, 0x05) & 0xFF7F));

      if (lch == 0)
	ac97_write (devc, 0x05, (ac97_read (devc, 0x05) & 0x7FFF) | 0x8000);
      else
	ac97_write (devc, 0x05, (ac97_read (devc, 0x05) & 0x7FFF));

      ac97_write (devc, 0x05, (WORD) ((ac97_read (devc, 0x05) & 0xFFE1) | ((WORD) right << 1)));	/*Right */
      ac97_write (devc, 0x05, (WORD) ((ac97_read (devc, 0x05) & 0xE1FF) | ((WORD) left << 9)));	/*Left */
      break;

    case SOUND_MIXER_LINE3:
      if (rch == 0)
	ac97_write (devc, 0x06, (ac97_read (devc, 0x06) & 0xFF7F) | 0x0080);
      else
	ac97_write (devc, 0x06, (ac97_read (devc, 0x06) & 0xFF7F));

      if (lch == 0)
	ac97_write (devc, 0x06, (ac97_read (devc, 0x06) & 0x7FFF) | 0x8000);
      else
	ac97_write (devc, 0x06, (ac97_read (devc, 0x06) & 0x7FFF));

      ac97_write (devc, 0x06, (WORD) ((ac97_read (devc, 0x06) & 0xFFE1) | ((WORD) right << 1)));	/*Right */
      ac97_write (devc, 0x06, (WORD) ((ac97_read (devc, 0x06) & 0xE1FF) | ((WORD) left << 9)));	/*Left */
      break;

    case SOUND_MIXER_LINE:
      if (rch == 0)
	ac97_write (devc, 0x1D, (ac97_read (devc, 0x1D) & 0xFF7F) | 0x0080);
      else
	ac97_write (devc, 0x1D, (ac97_read (devc, 0x1D) & 0xFF7F));

      if (lch == 0)
	ac97_write (devc, 0x1D, (ac97_read (devc, 0x1D) & 0x7FFF) | 0x8000);
      else
	ac97_write (devc, 0x1D, (ac97_read (devc, 0x1D) & 0x7FFF));

      ac97_write (devc, 0x1D, (WORD) ((ac97_read (devc, 0x1D) & 0xFFE1) | ((WORD) right << 1)));	/*Right */
      ac97_write (devc, 0x1D, (WORD) ((ac97_read (devc, 0x1D) & 0xE1FF) | ((WORD) left << 9)));	/*Left */
      break;

    case SOUND_MIXER_MIC:
      if (rch == 0)
	ac97_write (devc, 0x07, (ac97_read (devc, 0x07) & 0xFF7F) | 0x0080);
      else
	ac97_write (devc, 0x07, (ac97_read (devc, 0x07) & 0xFF7F));

      if (lch == 0)
	ac97_write (devc, 0x07, (ac97_read (devc, 0x07) & 0x7FFF) | 0x8000);
      else
	ac97_write (devc, 0x07, (ac97_read (devc, 0x07) & 0x7FFF));

      left = right;
      ac97_write (devc, 0x07,
		  (WORD) ((ac97_read (devc, 0x07) & 0xFFE1) |
			  ((WORD) right << 1)));
      ac97_write (devc, 0x07,
		  (WORD) ((ac97_read (devc, 0x07) & 0xE1FF) |
			  ((WORD) right << 9)));
      break;

    case SOUND_MIXER_SYNTH:
    case SOUND_MIXER_VOLUME:
    case SOUND_MIXER_PCM:
      if (rch == 0)
	ac97_write (devc, 0x09, (ac97_read (devc, 0x09) & 0xFF7F) | 0x0080);
      else
	ac97_write (devc, 0x09, (ac97_read (devc, 0x09) & 0xFF7F));

      if (lch == 0)
	ac97_write (devc, 0x09, (ac97_read (devc, 0x09) & 0x7FFF) | 0x8000);
      else
	ac97_write (devc, 0x09, (ac97_read (devc, 0x09) & 0x7FFF));

      ac97_write (devc, 0x09, (WORD) ((ac97_read (devc, 0x09) & 0xFFE1) | ((WORD) right << 1)));	/*Right */
      ac97_write (devc, 0x09, (WORD) ((ac97_read (devc, 0x09) & 0xE1FF) | ((WORD) left << 9)));	/*Left */
      break;

    case SOUND_MIXER_IGAIN:
      ac97_write (devc, 0x02, (WORD) ((ac97_read (devc, 0x02) & 0xFFF0) | ((WORD) right)));	/*Right */
      ac97_write (devc, 0x02, (WORD) ((ac97_read (devc, 0x02) & 0xF0FF) | ((WORD) left << 8)));	/*Left */
      break;
    }

  return devc->levels[dev] = value;
}

static void
pt101_mixer_reset (maestro_devc * devc)
{
  int i;

  devc->levels = load_mixer_volumes ("ESS PT101", default_mixer_levels, 1);
  devc->recmask = PT101_MIXER_RECDEVS & PT101_MIXER_DEVS;
  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
    pt101_mixer_set (devc, i, devc->levels[i]);
  pt101_set_recmask (devc, SOUND_MASK_MIC);
}

/*ARGSUSED*/
static int
pt101_mixer_ioctl (int dev, int audiodev, unsigned int cmd, ioctl_arg arg)
{
  maestro_devc *devc = mixer_devs[dev]->devc;
  int val;

  if (((cmd >> 8) & 0xff) == 'M')
    {
      if (IOC_IS_OUTPUT (cmd))
	switch (cmd & 0xff)
	  {
	  case SOUND_MIXER_RECSRC:
	    val = *arg;
	    return *arg = pt101_set_recmask (devc, val);
	    break;

	  default:
	    val = *arg;
	    return *arg = pt101_mixer_set (devc, cmd & 0xff, val);
	  }
      else
	switch (cmd & 0xff)
	  {

	  case SOUND_MIXER_RECSRC:
	    return *arg = devc->recdevs;
	    break;

	  case SOUND_MIXER_DEVMASK:
	    return *arg = PT101_MIXER_DEVS;
	    break;

	  case SOUND_MIXER_STEREODEVS:
	    return *arg = PT101_MIXER_STEREODEVS;
	    break;

	  case SOUND_MIXER_RECMASK:
	    return *arg = devc->recmask;
	    break;

	  case SOUND_MIXER_CAPS:
	    return *arg = SOUND_CAP_EXCL_INPUT;
	    break;

	  default:
	    return *arg = pt101_mixer_get (devc, cmd & 0xff);
	  }
    }
  else
    return OSS_EINVAL;
}

/*
 * Low level I/O routines
 */
#define WAVE_CACHE_INDEX    (devc->base + 0x10)
#define WAVE_CACHE_DATA         (devc->base + 0x12)
#define WAVE_CACHE_CONTROL  (devc->base + 0x14)

/*****************************************************************************/
/* Write Wave Cache Address I/O Port                                         */
/*                                                                           */
/*****************************************************************************/
static void
vWrWCRegIndex (maestro_devc * devc, WORD _wRegIndex)
{
  OUTW (devc->osdev, LOWORD (_wRegIndex), WAVE_CACHE_INDEX);
}

/*****************************************************************************/
/* Write Wave Cache Data I/O Port                                            */
/*                                                                           */
/*****************************************************************************/
static void
vWrWCRegData (maestro_devc * devc, WORD _wRegData)
{
  OUTW (devc->osdev, _wRegData, WAVE_CACHE_DATA);
}

/*****************************************************************************/
/* Read Wave Cache Addr I/O Port                                             */
/*                                                                           */
/*****************************************************************************/
WORD
wRdWCRegIndex (maestro_devc * devc)
{
  return ((WORD) INW (devc->osdev, WAVE_CACHE_INDEX));
}

/*****************************************************************************/
/* Read Wave Cache Data I/O Port                                             */
/*                                                                           */
/*****************************************************************************/
WORD
wRdWCRegData (maestro_devc * devc)
{
  return ((WORD) (INW (devc->osdev, WAVE_CACHE_DATA)));
}

/*****************************************************************************/
/* Write Wave Cache Memory                                                   */
/*                                                                           */
/*****************************************************************************/
void
vWrWCReg (maestro_devc * devc, WORD _wRegIndex, WORD _wRegData)
{
  vWrWCRegIndex (devc, _wRegIndex);
  vWrWCRegData (devc, _wRegData);
}

/*****************************************************************************/
/* Read Wave Cache Memory                                                    */
/*                                                                           */
/*****************************************************************************/
WORD
wRdWCReg (maestro_devc * devc, WORD _wRegIndex)
{
  WORD wRetVal;

  vWrWCRegIndex (devc, _wRegIndex);
  wRetVal = wRdWCRegData (devc);

  return (wRetVal);
}

/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*****************************************************************************/
void
vSetWCChannelControlBit (maestro_devc * devc, BYTE _bChannel, BYTE _bBit)
{
  vWrWCReg (devc, (WORD) (_bChannel << 3),
	    WSETBIT (wRdWCReg (devc, (WORD) (_bChannel << 3)), _bBit));
}

/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*****************************************************************************/
void
vMskWCChannelControlBit (maestro_devc * devc, BYTE _bChannel, BYTE _bBit)
{

  vWrWCReg (devc, (WORD) (_bChannel << 3),
	    WMSKBIT (wRdWCReg (devc, (WORD) (_bChannel << 3)), _bBit));
}


/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*****************************************************************************/
void
vSetWCChannelStereo (maestro_devc * devc, BYTE _bChannel, BYTE _bState)
{
  switch (_bState)
    {
    case 0:
       /*MONO*/ vMskWCChannelControlBit (devc, _bChannel, 1);
      break;

    case 1:
       /*STEREO*/ vSetWCChannelControlBit (devc, _bChannel, 1);
      break;
    }
}

/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*****************************************************************************/
void
vSetWCChannelWordSize (maestro_devc * devc, BYTE _bChannel, BYTE _bState)
{
  switch (_bState)
    {
    case 0:			/* 8BIT */
      vSetWCChannelControlBit (devc, _bChannel, 2);
      break;

    case 1:			/* 16BIT */
      vMskWCChannelControlBit (devc, _bChannel, 2);
      break;
    }
}

/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*****************************************************************************/
void
vSetWCChannelTagAddr (maestro_devc * devc, BYTE _bChannel, WORD _wTagAddr)
{
  vWrWCReg (devc, (WORD) (_bChannel << 3),
	    (WORD) ((wRdWCReg (devc, (WORD) (_bChannel << 3)) & 0x0007) |
		    (WORD) (_wTagAddr << 3)));
}

#define WAVERAM_START   0x400000

#define IDR0_DATA_PORT          0x00
#define IDR1_CRAM_POINTER       0x01
#define IDR2_CRAM_INCREMENT     0x02
#define IDR3_WAVE_DATA          0x03
#define IDR4_WAVE_PTR_LO        0x04
#define IDR5_WAVE_PTR_HI        0x05
#define IDR6_TIMER_CTRL         0x06
#define IDR7_WAVE_ROMRAM        0x07

/* Read/Write access for Indexed Data Registers */
#define _RW             0	/* Read/Write */
#define _WO             1	/* Write Only */
#define _RO             2	/* Read Only */

static BYTE bIDRAccessTable[] = {
  _RW, _RW, _RW, _RW,
  _RW, _RW, _WO, _WO,
  _WO, _WO, _WO, _WO,
  _WO, _WO, _WO, _WO,
  _WO, _WO, _RW, _WO,
  _RO, _RW, _RW, _WO
};

#define PT_DATA_PORT            devc->base
#define PT_INDEX_PORT           (devc->base + 0x02)

/*****************************************************************************/
/* Write IDR Register Index                                                  */
/*                                                                           */
/*****************************************************************************/
static void
vWrIDRIndex (maestro_devc * devc, WORD _wRegIndex)
{
  OUTW (devc->osdev, _wRegIndex, PT_INDEX_PORT);
}

/*****************************************************************************/
/* Write IDR Register Data                                                   */
/*                                                                           */
/*****************************************************************************/
static void
vWrIDRData (maestro_devc * devc, WORD _wRegData)
{
  OUTW (devc->osdev, _wRegData, PT_DATA_PORT);
}

/*****************************************************************************/
/* Read IDR Register Data                                                    */
/*                                                                           */
/*****************************************************************************/
static WORD
wRdIDRData (maestro_devc * devc)
{
  return ((WORD) INW (devc->osdev, PT_DATA_PORT));
}

/*****************************************************************************/
/* Write IDR Register                                                        */
/*                                                                           */
/*****************************************************************************/
static void
vWrIDR (maestro_devc * devc, WORD _wRegIndex, WORD _wRegData)
{
  vWrIDRIndex (devc, _wRegIndex);

  vWrIDRData (devc, _wRegData);

  devc->wIDRRegDataTable[_wRegIndex] = _wRegData;
}

/*****************************************************************************/
/* Read IDR Register                                                         */
/*                                                                           */
/*****************************************************************************/
static WORD
wRdIDR (maestro_devc * devc, WORD _wRegIndex)
{
  WORD wRetVal = 0;

  vWrIDRIndex (devc, _wRegIndex);

  switch (bIDRAccessTable[_wRegIndex])
    {
    case _RW:
    case _RO:
      wRetVal = wRdIDRData (devc);
      break;

    case _WO:
      wRetVal = devc->wIDRRegDataTable[_wRegIndex];
      break;
    }

  return (wRetVal);
}

/*****************************************************************************/
/* Set IDR Register Bit                                                      */
/*                                                                           */
/*****************************************************************************/
static void
vSetIDRBit (maestro_devc * devc, WORD _wRegIndex, BYTE _bRegBit)
{
  vWrIDR (devc, _wRegIndex, WSETBIT (wRdIDR (devc, _wRegIndex), _bRegBit));
}

/*****************************************************************************/
/* Mask IDR Register Bit                                                     */
/*                                                                           */
/*****************************************************************************/
static void
vMskIDRBit (maestro_devc * devc, WORD _wRegIndex, BYTE _bRegBit)
{
  vWrIDR (devc, _wRegIndex, WMSKBIT (wRdIDR (devc, _wRegIndex), _bRegBit));
}

/**************************/
#define _APU_MIXER  0x06
#define _APU_SYNR   0x3C
#define _APU_SYNL   0x3D
#define _APU_DIGL   0x3E
#define _APU_DIGR   0x3F

/* APU Modes */
#define	_APU_OFF			0x00
#define	_APU_16BITLINEAR	0x01	/* 16-Bit Linear Sample Player */
#define	_APU_16BITSTEREO	0x02	/* 16-Bit Stereo Sample Player */
#define	_APU_8BITLINEAR		0x03	/* 8-Bit Linear Sample Player */
#define	_APU_8BITSTEREO		0x04	/* 8-Bit Stereo Sample Player */
#define	_APU_8BITDIFF		0x05	/* 8-Bit Differential Sample Playrer */
#define	_APU_DIGITALDELAY	0x06	/* Digital Delay Line */
#define	_APU_DUALTAP		0x07	/* Dual Tap Reader */
#define	_APU_CORRELATOR		0x08	/* Correlator */
#define	_APU_INPUTMIXER		0x09	/* Input Mixer */
#define	_APU_WAVETABLE		0x0A	/* Wave Table Mode */
#define	_APU_SRCONVERTOR	0x0B	/* Sample Rate Convertor */
#define	_APU_16BITPINGPONG	0x0C	/* 16-Bit Ping-Pong Sample Player */

#define	_APU_RESERVED1		0x0D	/* Reserved 1 */
#define	_APU_RESERVED2		0x0E	/* Reserved 2 */
#define	_APU_RESERVED3		0x0F	/* Reserved 3 */

/* APU Filtey Q Control */
#define _APU_FILTER_LESSQ           0x00
#define _APU_FILTER_MOREQ           0x03

/* APU Filter Control */
#define	_APU_FILTER_2POLE_LOPASS	0x00
#define	_APU_FILTER_2POLE_BANDPASS	0x01
#define	_APU_FILTER_2POLE_HIPASS	0x02
#define	_APU_FILTER_1POLE_LOPASS	0x03
#define	_APU_FILTER_1POLE_HIPASS	0x04
#define	_APU_FILTER_OFF				0x05

/* Polar Pan Control */
#define	_APU_PAN_CENTER_CIRCLE		0x00
#define	_APU_PAN_MIDDLE_RADIUS		0x01
#define	_APU_PAN_OUTSIDE_RADIUS		0x02
#define	_APU_PAN_

/* APU ATFP Type */
#define	_APU_ATFP_AMPLITUDE			0x00
#define	_APU_ATFP_TREMELO			0x01
#define	_APU_ATFP_FILTER			0x02
#define	_APU_ATFP_PAN				0x03

/* APU ATFP Flags */
#define	_APU_ATFP_FLG_OFF			0x00
#define	_APU_ATFP_FLG_WAIT			0x01
#define	_APU_ATFP_FLG_DONE			0x02
#define	_APU_ATFP_FLG_INPROCESS		0x03

static WORD
wRdAPURegIndex (maestro_devc * devc)
{
  return (wRdIDR (devc, IDR1_CRAM_POINTER));
}

static void
vWrAPURegIndex (maestro_devc * devc, WORD _wRegIndex)
{
  vWrIDR (devc, IDR1_CRAM_POINTER, _wRegIndex);

  while (wRdAPURegIndex (devc) != _wRegIndex)
    {
    }
}

static WORD
wRdAPURegData (maestro_devc * devc)
{
  return (wRdIDR (devc, IDR0_DATA_PORT));
}

static void
vWrAPURegData (maestro_devc * devc, WORD _wRegData)
{
  while (wRdAPURegData (devc) != _wRegData)
    {
      vWrIDR (devc, IDR0_DATA_PORT, _wRegData);
    }
}

static void
vWrAPUReg (maestro_devc * devc, BYTE _bChannel, BYTE _bRegIndex,
	   WORD _wRegData)
{
  vWrAPURegIndex (devc, (WORD) ((_bChannel << 4) + _bRegIndex));
  vWrAPURegData (devc, _wRegData);
}

/* Should be static */ WORD
wRdAPUReg (maestro_devc * devc, BYTE _bChannel, BYTE _bRegIndex)
{
  vWrAPURegIndex (devc, (WORD) ((_bChannel << 4) + _bRegIndex));
  return (wRdAPURegData (devc));
}

/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*****************************************************************************/
DWORD
wCalculateAPUFrequency (DWORD freq)
{
  if (freq == 48000)
    return 0x10000;

  return ((freq / (SYSCLK / 1024L)) << 16) +
    (((freq % (SYSCLK / 1024L)) << 16) / (SYSCLK / 1024L));
}


static void
vSetAPUFrequency (maestro_devc * devc, BYTE _bChannel, DWORD _dwFrequency)
{
  vWrAPUReg (devc, _bChannel, 0x02,
	     (WORD) ((wRdAPUReg (devc, _bChannel, 0x02) & 0x00FF) |
		     (WORD) (LOBYTE (_dwFrequency) << 8)));
  vWrAPUReg (devc, _bChannel, 0x03, (WORD) (_dwFrequency >> 8));
}

static void
vSetAPURegBit (maestro_devc * devc, BYTE _bChannel, BYTE _bRegIndex,
	       BYTE _bRegBit)
{
  vWrAPUReg (devc, _bChannel, _bRegIndex,
	     WSETBIT (wRdAPUReg (devc, _bChannel, _bRegIndex), _bRegBit));
}

static void
vMskAPURegBit (maestro_devc * devc, BYTE _bChannel, BYTE _bRegIndex,
	       BYTE _bRegBit)
{
  vWrAPUReg (devc, _bChannel, _bRegIndex,
	     WMSKBIT (wRdAPUReg (devc, _bChannel, _bRegIndex), _bRegBit));
}

static void
vSetAPUSubmixMode (maestro_devc * devc, BYTE _bChannel, BYTE _bState)
{
  switch (_bState)
    {
    case DISABLE:
      vMskAPURegBit (devc, _bChannel, 0x02, 3);
      break;

    case ENABLE:
      vSetAPURegBit (devc, _bChannel, 0x02, 3);
      break;
    }
}

static void
vSetAPUSubmixGroup (maestro_devc * devc, BYTE _bChannel, BYTE _bSubmixGroup)
{
  vWrAPUReg (devc, _bChannel, 0x02,
	     (WORD) ((wRdAPUReg (devc, _bChannel, 0x02) & 0xFFF8) |
		     _bSubmixGroup));
}

static void
vSetAPU6dB (maestro_devc * devc, BYTE _bChannel, BYTE _bState)
{
  switch (_bState)
    {
    case DISABLE:
      vMskAPURegBit (devc, _bChannel, 0x02, 4);
      break;

    case ENABLE:
      vSetAPURegBit (devc, _bChannel, 0x02, 4);
      break;
    }
}

static void
vSetAPUType (maestro_devc * devc, BYTE _bChannel, BYTE _bType)
{
  vWrAPUReg (devc, _bChannel, 0x00,
	     (wRdAPUReg (devc, _bChannel, 0x00) & 0xFF0F) | (_bType << 4));
}

static void
vSetAPUDMA (maestro_devc * devc, BYTE _bChannel, BYTE _bState)
{
  switch (_bState)
    {
    case DISABLE:
      vMskAPURegBit (devc, _bChannel, 0x00, 14);
      break;

    case ENABLE:
      vSetAPURegBit (devc, _bChannel, 0x00, 14);
      break;
    }
}

static void
vSetAPUFilterType (maestro_devc * devc, BYTE _bChannel, BYTE _bFilterType)
{
  vWrAPUReg (devc, _bChannel, 0x00,
	     (WORD) ((wRdAPUReg (devc, _bChannel, 0x00) & 0xFFF3) |
		     (_bFilterType << 2)));
}

static void
vSetAPUFilterQ (maestro_devc * devc, BYTE _bChannel, BYTE _bFilterQ)
{
  vWrAPUReg (devc, _bChannel, 0x00,
	     (WORD) ((wRdAPUReg (devc, _bChannel, 0x00) & 0xFFFC) |
		     _bFilterQ));
}

static void
vSetAPUFilterTuning (maestro_devc * devc, BYTE _bChannel, BYTE _bFilterTuning)
{
  vWrAPUReg (devc, _bChannel, 0x0A,
	     (WORD) ((wRdAPUReg (devc, _bChannel, 0x0A) & 0x00FF) |
		     ((WORD) _bFilterTuning << 8)));
}

static void
vSetAPUPolarPan (maestro_devc * devc, BYTE _bChannel, BYTE _bPolarPan)
{
  vWrAPUReg (devc, _bChannel, 0x0A,
	     (WORD) ((wRdAPUReg (devc, _bChannel, 0x0A) & 0xFFC0) |
		     _bPolarPan));
}

static void
vSetAPUDataSourceA (maestro_devc * devc, BYTE _bChannel, BYTE _bDataSourceA)
{
  vWrAPUReg (devc, _bChannel, 0x0B,
	     (WORD) ((wRdAPUReg (devc, _bChannel, 0x0B) & 0xFF80) |
		     _bDataSourceA));
}

static void
vSetAPUAmplitudeNow (maestro_devc * devc, BYTE _bChannel, BYTE _bAmplitude)
{
  vWrAPUReg (devc, _bChannel, 0x09,
	     (WORD) ((wRdAPUReg (devc, _bChannel, 0x09) & 0x00FF) |
		     (((WORD) _bAmplitude << 8))));
}

static void
vSetAPUWave64kPage (maestro_devc * devc, BYTE _bChannel, DWORD _wWaveStart)
{
  vWrAPUReg (devc, _bChannel, 0x04, ((_wWaveStart >> 16) & 0xFF) << 8);
}

static void
vSetAPUWaveStart (maestro_devc * devc, BYTE _bChannel, WORD _wWaveStart)
{
  vWrAPUReg (devc, _bChannel, 0x05, _wWaveStart);
}

static void
vSetAPUWaveEnd (maestro_devc * devc, BYTE _bChannel, WORD _wWaveEnd)
{
  vWrAPUReg (devc, _bChannel, 0x06, _wWaveEnd);
}

static void
vSetAPUWaveLoop (maestro_devc * devc, BYTE _bChannel, WORD _wWaveLoop)
{
  vWrAPUReg (devc, _bChannel, 0x07, _wWaveLoop);
}

static void
vSetAPUWavePtr (maestro_devc * devc, BYTE _bChannel, DWORD _dwWaveStart,
		WORD _wWaveLength)
{
  /* start of sample */
  vSetAPUWave64kPage (devc, _bChannel, _dwWaveStart);
  vSetAPUWaveStart (devc, _bChannel, LOWORD (_dwWaveStart));
  vSetAPUWaveEnd (devc, _bChannel, LOWORD ((_dwWaveStart + _wWaveLength)));
  vSetAPUWaveLoop (devc, _bChannel, _wWaveLength);
}

/*****************************************************************************/
/* Write Wave Cache Control I/O Port                                         */
/*                                                                           */
/*****************************************************************************/
static void
vWrWCControlReg (maestro_devc * devc, WORD _wWCControlRegFlags)
{
  OUTW (devc->osdev, _wWCControlRegFlags, WAVE_CACHE_CONTROL);
}

/*****************************************************************************/
/* Read Wave Cache Control I/O Port                                          */
/*                                                                           */
/*****************************************************************************/
static WORD
wRdWCControlReg (maestro_devc * devc)
{
  return ((WORD) (INW (devc->osdev, WAVE_CACHE_CONTROL)));
}

/*****************************************************************************/
/* Set Wave Cache Control Bit Flag                                           */
/*                                                                           */
/*****************************************************************************/
static void
vSetWCControlRegBit (maestro_devc * devc, BYTE _bWCControlRegBit)
{
  vWrWCControlReg (devc, WSETBIT (wRdWCControlReg (devc), _bWCControlRegBit));
}

/*****************************************************************************/
/* Mask Wave Cache Control Bit Flag                                          */
/*                                                                           */
/*****************************************************************************/
static void
vMskWCControlRegBit (maestro_devc * devc, BYTE _bWCControlRegBit)
{
  vWrWCControlReg (devc, WMSKBIT (wRdWCControlReg (devc), _bWCControlRegBit));
}

/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*****************************************************************************/
static void
vSetWCEnable (maestro_devc * devc, BYTE _bState)
{
  switch (_bState)
    {
    case DISABLE:
      vMskWCControlRegBit (devc, 8);
      break;

    case ENABLE:
      vSetWCControlRegBit (devc, 8);
      break;
    }
}

/*****************************************************************************/
/*                                                                           */
/*                                                                           */
/*****************************************************************************/
/*ARGSUSED*/
void
vSetTimer (maestro_devc * devc, WORD _wInterruptFreq)
{

  int prescale;
  int divide;

  /* XXX make freq selector much smarter, see calc_bob_rate */
  int freq = 200;

  /* compute ideal interrupt frequency for buffer size & play rate */
  /* first, find best prescaler value to match freq */
  for (prescale = 5; prescale < 12; prescale++)
    if (freq > (SYSCLK >> (prescale + 9)))
      break;

  /* next, back off prescaler whilst getting divider into optimum range */
  divide = 1;
  while ((prescale > 5) && (divide < 32))
    {
      prescale--;
      divide <<= 1;
    }
  divide >>= 1;

  /* now fine-tune the divider for best match */
  for (; divide < 31; divide++)
    if (freq >= ((SYSCLK >> (prescale + 9)) / (divide + 1)))
      break;

  /* divide = 0 is illegal, but don't let prescale = 4! */
  if (divide == 0)
    {
      divide++;
      if (prescale > 5)
	prescale--;
    }
  vWrIDR (devc, 0x06, (WORD) (0x9000 | (prescale << 5) | divide));
}

/*
 *****************************************************************************
 */

static int
maestro_audio_set_rate (int dev, int arg)
{
  maestro_portc *portc = audio_engines[dev]->portc;

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
maestro_audio_set_channels (int dev, short arg)
{
  maestro_portc *portc = audio_engines[dev]->portc;

  if ((arg != 1) && (arg != 2))
    return portc->channels;
  portc->channels = arg;

  return portc->channels;
}

static unsigned int
maestro_audio_set_format (int dev, unsigned int arg)
{
  maestro_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->bits;

  if (!(arg & (AFMT_U8 | AFMT_S16_LE)))
    return portc->bits;
  portc->bits = arg;

  return portc->bits;
}

/*ARGSUSED*/
static int
maestro_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void maestro_audio_trigger (int dev, int state);

static void
maestro_audio_reset (int dev)
{
  maestro_audio_trigger (dev, 0);
}

static void
maestro_audio_reset_input (int dev)
{
  maestro_portc *portc = audio_engines[dev]->portc;
  maestro_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
maestro_audio_reset_output (int dev)
{
  maestro_portc *portc = audio_engines[dev]->portc;
  maestro_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

/*ARGSUSED*/
static int
maestro_audio_open (int dev, int mode, int open_flags)
{
  oss_native_word flags;
  maestro_portc *portc = audio_engines[dev]->portc;
  maestro_devc *devc = audio_engines[dev]->devc;

  MUTEX_ENTER_IRQDISABLE (devc->lock, flags);
  if (portc->open_mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->lock, flags);
      return OSS_EBUSY;
    }
#if 1
  if ((mode & OPEN_READ))
    {
      audio_engines[dev]->flags |= ADEV_16BITONLY;
    }
  else
    {
      audio_engines[dev]->flags &= ~(ADEV_16BITONLY);
    }
#endif
  if (devc->open_mode & mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->lock, flags);
      return OSS_EBUSY;
    }

  devc->open_mode |= mode;

  portc->open_mode = mode;
  portc->audio_enabled &= ~mode;
  MUTEX_EXIT_IRQRESTORE (devc->lock, flags);

  return 0;
}

static void
maestro_audio_close (int dev, int mode)
{
  maestro_portc *portc = audio_engines[dev]->portc;
  maestro_devc *devc = audio_engines[dev]->devc;

  maestro_audio_reset (dev);
  portc->open_mode = 0;
  devc->open_mode &= ~mode;
  portc->audio_enabled &= ~mode;
}

/*ARGSUSED*/
static void
maestro_audio_output_block (int dev, oss_native_word buf, int count,
			    int fragsize, int intrflag)
{
  maestro_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
}

/*ARGSUSED*/
static void
maestro_audio_start_input (int dev, oss_native_word buf, int count,
			   int fragsize, int intrflag)
{
  maestro_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;

}

static void
maestro_audio_trigger (int dev, int state)
{
  oss_native_word flags;
  maestro_portc *portc = audio_engines[dev]->portc;
  maestro_devc *devc = audio_engines[dev]->devc;

  MUTEX_ENTER_IRQDISABLE (devc->lock, flags);

  if (portc->open_mode & OPEN_WRITE)
    {
      if (state & PCM_ENABLE_OUTPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      if (portc->bits == AFMT_U8)
		{
		  vSetAPUType (devc, bAPURPlay, 0x03);
		  if (portc->channels == 2)
		    {
		      vSetAPUType (devc, bAPURPlay, 0x04);
		      vSetAPUType (devc, bAPULPlay, 0x04);
		    }
		}
	      if (portc->bits == AFMT_S16_LE)
		{
		  vSetAPUType (devc, bAPURPlay, 0x01);
		  if (portc->channels == 2)
		    {
		      vSetAPUType (devc, bAPURPlay, 0x02);
		      vSetAPUType (devc, bAPULPlay, 0x02);
		    }
		}
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
	      vSetAPUType (devc, bAPURPlay, 0x0000);
	      vSetAPUType (devc, bAPULPlay, 0x0000);
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
	      vSetAPUType (devc, bAPULMix, 0x09);
	      vSetAPUType (devc, bAPULSrc, 0x0B);
	      if (portc->channels == 2)
		{
		  vSetAPUType (devc, bAPURMix, 0x09);
		  vSetAPUType (devc, bAPURSrc, 0x0B);
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
	      vSetAPUType (devc, bAPURMix, 0x00);
	      vSetAPUType (devc, bAPURSrc, 0x00);
	      vSetAPUType (devc, bAPULMix, 0x00);
	      vSetAPUType (devc, bAPULSrc, 0x00);
	    }
	}
    }
  MUTEX_EXIT_IRQRESTORE (devc->lock, flags);
}

/*ARGSUSED*/
static int
maestro_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  maestro_devc *devc = audio_engines[dev]->devc;
  maestro_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_in;

  oss_native_word flags;

  WORD wIndex, channel;
  unsigned int wSampleRate = portc->speed;
  int skip = 2;

#if 0
  if (portc->channels == 2)
    {
      cmn_err (CE_WARN, "Stereo recording not supported\n");
      return OSS_EIO;
    }
#endif

  MUTEX_ENTER_IRQDISABLE (devc->lock, flags);

  devc->wApuBufferSize = dmap->bytes_in_use >> 1;

  if (wSampleRate > 47999)
    wSampleRate = 47999;
  if (wSampleRate < 5000)
    wSampleRate = 5000;


  if (portc->channels == 2)
    {
      devc->wApuBufferSize >>= 1;
      if (portc->bits == 16)
	wSampleRate <<= 1;
      skip = 1;
    }

  for (channel = 2; channel < 6; channel += skip)
    {
      int bsize, route;
      unsigned int physaddr;
      unsigned int rate;

      /* Clear out the APU */
      for (wIndex = 0; wIndex < 16; wIndex++)
	vWrAPUReg (devc, channel, (WORD) wIndex, 0x0000);

      /* data seems to flow from the codec, through an apu into
         the 'mixbuf' bit of page, then through the SRC apu
         and out to the real 'buffer'. */

      if (channel & 0x04)
	{
	  /* codec to mixbuf left */
	  if (!(channel & 0x01))
	    physaddr = devc->dma_mix.base;
	  else
	    /* codec to mixbuf right */
	    physaddr = devc->dma_mix.base + (PAGE_SIZE >> 4);


	  bsize = PAGE_SIZE >> 5;	/* 256 bytes needed for Mixbuf */
	  route = 0x14 + (channel - 4);	/* input routed from parallelin base */

	  /* The Mixer always runs at 48Khz. */
	  rate = 0x10000;
	  vSetAPUFrequency (devc, channel, rate);

	}
      else
	{
	  /* sample rate converter takes input from the mixer apu and 
	     outputs it to system memory */

	  if (!(channel & 0x01))
	    /* left channel records its half */
	    physaddr = dmap->dmabuf_phys;

	  else
	    /* right channel records its half */
	    physaddr = dmap->dmabuf_phys + devc->wApuBufferSize * 2;

	  bsize = devc->wApuBufferSize;
	  /* get input from inputing apu */
	  route = channel + 2;

	  /* SRC takes 48Khz data from mixer and converts it to requested rate */
	  rate = (DWORD) wCalculateAPUFrequency (wSampleRate);
	  vSetAPUFrequency (devc, channel, rate);


	}

      /* set the wavecache control reg */
      vSetWCChannelTagAddr (devc, channel,
			    (LOWORD (physaddr) - 0x10) & 0xFFF8);
      vSetWCChannelStereo (devc, channel, 0);
      vSetWCChannelWordSize (devc, channel, 1);

      physaddr -= devc->dmapages_phys;
      physaddr >>= 1;		/* words */

      /* base offset of dma calcs when reading the pointer
         on this left one */
#if 0
      if (channel == 2)
	devc->dma_adc.base = physaddr & 0xFFFF;
#endif
      physaddr |= 0x00400000;	/* bit 22 -> System RAM */

      /* Load the buffer into the wave engine */
      vSetAPUWavePtr (devc, channel, physaddr, bsize);

      /* Now set the rest of the APU params */
      vSetAPUSubmixMode (devc, channel, ENABLE);
      vSetAPUSubmixGroup (devc, channel, 0x0);
      vSetAPU6dB (devc, channel, DISABLE);
      vSetAPUAmplitudeNow (devc, channel, 0xF0);
      vSetAPUFilterTuning (devc, channel, 0x8F);
      vSetAPUPolarPan (devc, channel, 0x08);
      vSetAPUFilterType (devc, channel, 0x03);
      vSetAPUFilterQ (devc, channel, 0x03);
      vSetAPUDMA (devc, channel, ENABLE);

      /* route input */
      vSetAPUDataSourceA (devc, channel, route);
    }

  vMskIDRBit (devc, 0x11, 0);
  vMskIDRBit (devc, 0x17, 0);	/* Disable Bob Timer Interrupts */
  vSetTimer (devc, SYSCLK / (devc->wApuBufferSize));
  vSetIDRBit (devc, 0x11, 0);
  vSetIDRBit (devc, 0x17, 0);	/* Enable Bob Timer Interrupts */
  portc->audio_enabled &= ~PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;

  MUTEX_EXIT_IRQRESTORE (devc->lock, flags);
  return 0;
}

/*ARGSUSED*/
static int
maestro_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  maestro_devc *devc = audio_engines[dev]->devc;
  maestro_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_out;

  oss_native_word flags;

  WORD wIndex, i;
  DWORD dwPhysAddr;
  unsigned int wSampleRate = portc->speed;
  int numchans = 0;

  MUTEX_ENTER_IRQDISABLE (devc->lock, flags);

  devc->wApuBufferSize = dmap->bytes_in_use >> 1;

  if ((portc->bits == 8) && (portc->channels == 1))
    {
      wSampleRate >>= 1;
    }

  if (portc->channels == 2)
    {
      numchans++;
      if (portc->bits == 16)
	devc->wApuBufferSize >>= 1;
    }

  for (i = 0; i <= numchans; i++)
    {

      dwPhysAddr = dmap->dmabuf_phys;

      vSetWCChannelTagAddr (devc, i,
			    (WORD) ((LOWORD (dwPhysAddr) - 0x10) & 0xFFF8));

      if (portc->bits == 16)
	vSetWCChannelWordSize (devc, i, 1);
      else
	vSetWCChannelWordSize (devc, i, 0);

      if (portc->channels == 2)
	vSetWCChannelStereo (devc, i, 1);
      else
	vSetWCChannelStereo (devc, i, 0);

      /* Calculate WP base address */
      dwPhysAddr -= devc->dmapages_phys;
      dwPhysAddr >>= 1;		/* adjust for word size */
#if 0
      if (i)
	devc->dma_dac.base = dwPhysAddr & 0xFFFF;
#endif
      dwPhysAddr |= 0x00400000L;	/* Enable bit 22 for system ram access */


      if (portc->channels == 2)
	{
	  if (!i)
	    dwPhysAddr |= 0x00800000;
	  if (portc->bits == 16)
	    {
	      dwPhysAddr >>= 1;
	    }
	}
      for (wIndex = 0; wIndex < 16; wIndex++)
	vWrAPUReg (devc, i, (BYTE) wIndex, 0x0000);

      vSetAPUFrequency (devc, i,
			(DWORD) wCalculateAPUFrequency (wSampleRate));
      vSetAPUWavePtr (devc, i, dwPhysAddr, devc->wApuBufferSize);
      vSetAPU6dB (devc, i, DISABLE);
      vSetAPUAmplitudeNow (devc, i, 0xF0);
      vSetAPUFilterTuning (devc, i, 0x8F);
      if (portc->channels == 2)
	vSetAPUPolarPan (devc, i, (i ? 0x10 : 0x00));
      else
	vSetAPUPolarPan (devc, i, 0x08);
      vSetAPUFilterType (devc, i, 0x03);
      vSetAPUFilterQ (devc, i, 0x03);
      vSetAPUDMA (devc, i, ENABLE);
    }

  vMskIDRBit (devc, 0x11, 0);
  vMskIDRBit (devc, 0x17, 0);	/* Disable Bob Timer Interrupts */
  vSetTimer (devc, SYSCLK / devc->wApuBufferSize);
  vSetIDRBit (devc, 0x11, 0);
  vSetIDRBit (devc, 0x17, 0);	/* Enable Bob Timer Interrupts */
  portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
  MUTEX_EXIT_IRQRESTORE (devc->lock, flags);
  return 0;
}

static int
maestro_alloc_buffer (int dev, dmap_t * dmap, int direction)
{
  maestro_devc *devc = audio_engines[dev]->devc;

  if (dmap->dmabuf != NULL)
    return 0;

  if (direction == PCM_ENABLE_OUTPUT)
    {
      dmap->dmabuf = devc->dma_dac.rawbuf;
      dmap->dmabuf_phys = devc->dma_dac.base;
      dmap->buffsize = devc->dma_dac.dmasize;
    }

  if (direction == PCM_ENABLE_INPUT)
    {
      dmap->dmabuf = devc->dma_adc.rawbuf;
      dmap->dmabuf_phys = devc->dma_adc.base;
      dmap->buffsize = devc->dma_adc.dmasize;
    }
  return 0;
}

/*ARGSUSED*/
static int
maestro_free_buffer (int dev, dmap_t * dmap, int direction)
{
  if (dmap->dmabuf == NULL)
    return 0;

  dmap->dmabuf = NULL;
  dmap->dmabuf_phys = 0;
  dmap->buffsize = 0;

  return 0;
}

/*ARGSUSED*/
static int
maestro_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  maestro_devc *devc = audio_engines[dev]->devc;
  maestro_portc *portc = audio_engines[dev]->portc;
  int ptr = 0;
  oss_native_word flags;

  if (!(portc->open_mode & direction))
    return 0;

  MUTEX_ENTER_IRQDISABLE (devc->low_lock, flags);
  if (direction == PCM_ENABLE_INPUT)
    {
      ptr = wRdAPUReg (devc, bAPULSrc, 0x05);
      ptr = (ptr - devc->dma_adc.base) & 0xFFFE;
    }

  if (direction == PCM_ENABLE_OUTPUT)
    {
      ptr = wRdAPUReg (devc, bAPURPlay, 0x05);
      ptr = (ptr - devc->dma_dac.base) & 0xFFFE;
    }
  ptr = (ptr % devc->wApuBufferSize) << 1;
  MUTEX_EXIT_IRQRESTORE (devc->low_lock, flags);
  return (ptr);
}

static void
set_maestro_base (maestro_devc * devc)
{
  unsigned long packed_phys = devc->dmapages_phys >> 12;
  vWrWCReg (devc, 0x01FC, packed_phys);
  vWrWCReg (devc, 0x01FD, packed_phys);
  vWrWCReg (devc, 0x01FE, packed_phys);
  vWrWCReg (devc, 0x01FF, packed_phys);
}

static int
allocate_maestro_bufs (maestro_devc * devc)
{
  unsigned char *rawbuf = NULL;
  int size, extra, start;
  oss_native_word phaddr;

  /* size = size of rec/play buffers 
   * start = offset from beginning for rec/play buffers
   * extra=size of mix buf + start
   */
#if defined(sun) || defined(linux)
  size = 32 * 1024;
  start = 16 * 1024;
  extra = 16 * 1024;
#else
#if defined (__FreeBSD__)
  size = 64 * 1024;
  start = 8 * 1024;
  extra = 16 * 1024;
#else
  size = 96 * 1024;
  start = 16 * 1024;
  extra = 16 * 1024;
#endif
#endif
  devc->dmalen = size + extra;

  rawbuf =
    (void *) CONTIG_MALLOC (devc->osdev, devc->dmalen, MEMLIMIT_28BITS,
			    &phaddr, TODO);

  if (!rawbuf)
    return 1;

  DDB (cmn_err (CE_WARN, "addr=%p, size=%d\n", (void *) rawbuf, size));

  if ((phaddr + size - 1) & ~((1 << 28) - 1))
    {
      cmn_err (CE_WARN, "DMA buffer beyond 256MB\n");
      CONTIG_FREE (devc->osdev, rawbuf, size + extra, TODO);
      return 1;
    }

  devc->dmapages = rawbuf;
  devc->dmapages_phys = phaddr;

  devc->dma_dac.rawbuf = rawbuf + start;
  devc->dma_dac.dmasize = size / 2;
  devc->dma_dac.base = phaddr + start;

  devc->dma_adc.rawbuf = devc->dma_dac.rawbuf + size / 2;
  devc->dma_adc.dmasize = size / 2;
  devc->dma_adc.base = devc->dma_dac.base + size / 2;

  devc->dma_mix.rawbuf = rawbuf + 4096;
  devc->dma_mix.base = phaddr + 4096;
  devc->dma_mix.dmasize = 4096;
#ifdef linux
  /* reserve the pages for MMAP */
  oss_reserve_pages ((oss_native_word) devc->dma_dac.rawbuf,
		     (oss_native_word) devc->dma_dac.rawbuf +
		     devc->dma_dac.dmasize - 1);
#endif
  return 0;
}

static audiodrv_t maestro_audio_driver = {
  maestro_audio_open,
  maestro_audio_close,
  maestro_audio_output_block,
  maestro_audio_start_input,
  maestro_audio_ioctl,
  maestro_audio_prepare_for_input,
  maestro_audio_prepare_for_output,
  maestro_audio_reset,
  NULL,
  NULL,
  maestro_audio_reset_input,
  maestro_audio_reset_output,
  maestro_audio_trigger,
  maestro_audio_set_rate,
  maestro_audio_set_format,
  maestro_audio_set_channels,
  NULL,
  NULL,
  NULL,				/* maestro_check_input, */
  NULL,				/* maestro_check_output, */
  maestro_alloc_buffer,
  maestro_free_buffer,
  NULL,
  NULL,
  maestro_get_buffer_pointer
};

static mixer_driver_t pt101_mixer_driver = {
  pt101_mixer_ioctl
};


static void
maestro_ac97_reset (maestro_devc * devc)
{
  int save_68;

/* Reset the Codec */
  OUTW (devc->osdev, INW (devc->osdev, devc->base + 0x38) & 0xfffc,
	devc->base + 0x38);
  OUTW (devc->osdev, INW (devc->osdev, devc->base + 0x3a) & 0xfffc,
	devc->base + 0x3a);
  OUTW (devc->osdev, INW (devc->osdev, devc->base + 0x3c) & 0xfffc,
	devc->base + 0x3c);
  /* reset the first codec */
  OUTW (devc->osdev, 0x0000, devc->base + 0x36);
  save_68 = INW (devc->osdev, devc->base + 0x68);
  if (devc->gpio & 0x1)
    save_68 |= 0x10;
  OUTW (devc->osdev, 0xfffe, devc->base + 0x64);	/* tickly gpio 0.. */
  OUTW (devc->osdev, 0x0001, devc->base + 0x68);
  OUTW (devc->osdev, 0x0000, devc->base + 0x60);
  oss_udelay (10);
  OUTW (devc->osdev, 0x0001, devc->base + 0x60);
  oss_udelay (100);
  OUTW (devc->osdev, save_68 | 0x1, devc->base + 0x68);	/* now restore .. */
  OUTW (devc->osdev, (INW (devc->osdev, devc->base + 0x38) & 0xfffc) | 0x1,
	devc->base + 0x38);
  OUTW (devc->osdev, (INW (devc->osdev, devc->base + 0x3a) & 0xfffc) | 0x1,
	devc->base + 0x3a);
  OUTW (devc->osdev, (INW (devc->osdev, devc->base + 0x3c) & 0xfffc) | 0x1,
	devc->base + 0x3c);

  /* now the second codec */
  OUTW (devc->osdev, 0x0000, devc->base + 0x36);
  OUTW (devc->osdev, 0xfff7, devc->base + 0x64);
  save_68 = INW (devc->osdev, devc->base + 0x68);
  OUTW (devc->osdev, 0x0009, devc->base + 0x68);
  OUTW (devc->osdev, 0x0001, devc->base + 0x60);
  oss_udelay (10);
  OUTW (devc->osdev, 0x0009, devc->base + 0x60);
  oss_udelay (100);
  OUTW (devc->osdev, INW (devc->osdev, devc->base + 0x38) & 0xfffc,
	devc->base + 0x38);
  OUTW (devc->osdev, INW (devc->osdev, devc->base + 0x3a) & 0xfffc,
	devc->base + 0x3a);
  OUTW (devc->osdev, INW (devc->osdev, devc->base + 0x3c) & 0xfffc,
	devc->base + 0x3c);
}

static int
init_maestro (maestro_devc * devc)
{
  int my_mixer;
  int i;
  oss_native_word n;
  unsigned short w;
  int wIndex;
  int first_dev = 0;
  int codec_id;
  int bRow, bAPU;

/* Sound reset */
  OUTW (devc->osdev, 0x2000, 0x18 + devc->base);
  oss_udelay (10);
  OUTW (devc->osdev, 0x0000, 0x18 + devc->base);
  oss_udelay (10);

/* Setup 0x34 and 0x36 regs */
  OUTW (devc->osdev, 0xc090, devc->base + 0x34);
  oss_udelay (1000);
  OUTW (devc->osdev, 0x3000, devc->base + 0x36);
  oss_udelay (1000);

/* reset ac97 link */
  maestro_ac97_reset (devc);

/* DirectSound*/
  n = INL (devc->osdev, devc->base + 0x34);
  n &= ~0xF000;
  n |= 12 << 12;		/* Direct Sound, Stereo */
  OUTL (devc->osdev, n, devc->base + 0x34);

  n = INL (devc->osdev, devc->base + 0x34);
  n &= ~0x0F00;			/* Modem off */
  OUTL (devc->osdev, n, devc->base + 0x34);

  n = INL (devc->osdev, devc->base + 0x34);
  n &= ~0x00F0;
  n |= 9 << 4;			/* DAC, Stereo */
  OUTL (devc->osdev, n, devc->base + 0x34);

  n = INL (devc->osdev, devc->base + 0x34);
  n &= ~0x000F;			/* ASSP off */
  OUTL (devc->osdev, n, devc->base + 0x34);

  n = INL (devc->osdev, devc->base + 0x34);
  n |= (1 << 29);		/* Enable ring bus */
  OUTL (devc->osdev, n, devc->base + 0x34);

  n = INL (devc->osdev, devc->base + 0x34);
  n |= (1 << 28);		/* Enable serial bus */
  OUTL (devc->osdev, n, devc->base + 0x34);

  n = INL (devc->osdev, devc->base + 0x34);
  n &= ~0x00F00000;		/* MIC off */
  OUTL (devc->osdev, n, devc->base + 0x34);

  n = INL (devc->osdev, devc->base + 0x34);
  n &= ~0x000F0000;		/* I2S off */
  OUTL (devc->osdev, n, devc->base + 0x34);

  w = INW (devc->osdev, devc->base + 0x18);
  w &= ~(1 << 7);		/* ClkRun off */
  OUTW (devc->osdev, w, devc->base + 0x18);

  w = INW (devc->osdev, devc->base + 0x18);
  w &= ~(1 << 6);		/* Harpo off */
  OUTW (devc->osdev, w, devc->base + 0x18);

  w = INW (devc->osdev, devc->base + 0x18);
  w &= ~(1 << 4);		/* ASSP irq off */
  OUTW (devc->osdev, w, devc->base + 0x18);

  w = INW (devc->osdev, devc->base + 0x18);
  w &= ~(1 << 3);		/* ISDN irq off */
  OUTW (devc->osdev, w, devc->base + 0x18);

  w = INW (devc->osdev, devc->base + 0x18);
  w |= (1 << 2);		/* Direct Sound IRQ on */
  OUTW (devc->osdev, w, devc->base + 0x18);

  w = INW (devc->osdev, devc->base + 0x18);
  w &= ~(1 << 1);		/* MPU401 IRQ off */
  OUTW (devc->osdev, w, devc->base + 0x18);

  w = INW (devc->osdev, devc->base + 0x18);
  w &= ~(1 << 0);		/* SB IRQ on */
  OUTW (devc->osdev, w, devc->base + 0x18);

  OUTB (devc->osdev, 0x00, gwPTBaseIO + 0xA4);
  OUTB (devc->osdev, 0x03, gwPTBaseIO + 0xA2);
  OUTB (devc->osdev, 0x00, gwPTBaseIO + 0xA6);

  if (devc->model == ESS_MAESTRO)
    {
/* Clear out wavecache */
      for (wIndex = 0; wIndex < 0x0200; wIndex++)
	vWrWCReg (devc, wIndex, 0x0000);
/* Clear out APU */
      for (wIndex = 0; wIndex < 0x40; wIndex++)
	vWrAPUReg (devc, wIndex, 0x00, 0x0000);
/* Clear out WP */
      for (wIndex = 0; wIndex < 0x1F; wIndex++)
	devc->wIDRRegDataTable[wIndex] = 0x0000;
    }
  else
    {
/* Clear out Wave Cache */
      for (bRow = 0; bRow < 0x10; bRow++)
	{
	  vWrWCReg (devc, (WORD) (0x01E0 + bRow), 0x0000);
	}

      /* Clear Control RAM */
      for (bAPU = 0x00; bAPU < 0x40; bAPU++)
	{
	  for (bRow = 0; bRow < 0x0E; bRow++)
	    {
	      vWrAPUReg (devc, bAPU, bRow, 0x0000);
	    }
	}
    }

  vWrIDR (devc, 0x02, 0x0000);	/* CRam Increment */
  vWrIDR (devc, 0x08, 0xB004);	/* Audio Serial Configuration */
  vWrIDR (devc, 0x09, 0x001B);	/* Audio Serial Configuration */
  vWrIDR (devc, 0x0A, 0x8000);	/* Audio Serial Configuration */
  vWrIDR (devc, 0x0B, 0x3F37);	/* Audio Serial Configuration */
  vWrIDR (devc, 0x0C, 0x0098);

  /* parallel out */
  vWrIDR (devc, 0x0C, (wRdIDR (devc, 0x0C) & ~0xF000) | 0x8000);
  /* parallel in */
  vWrIDR (devc, 0x0C, (wRdIDR (devc, 0x0C) & ~0x0F00) | 0x0500);

  vWrIDR (devc, 0x0D, 0x7632);	/*Audio Serial Configuration */
  OUTW (devc->osdev, INW (devc->osdev, 0x14 + devc->base) | (1 << 8),
	0x14 + devc->base);
  OUTW (devc->osdev, INW (devc->osdev, 0x14 + devc->base) & 0xFE03,
	0x14 + devc->base);
  OUTW (devc->osdev, (INW (devc->osdev, 0x14 + devc->base) & 0xFFFC),
	0x14 + devc->base);
  OUTW (devc->osdev, INW (devc->osdev, 0x14 + devc->base) | (1 << 7),
	0x14 + devc->base);

  /* enable the Wavecache */
  vSetWCEnable (devc, ENABLE);
  /* Unmask WP interrupts */
  OUTW (devc->osdev, (WORD) (INW (devc->osdev, devc->base + 0x18)
			     | 0x0004), devc->base + 0x18);

  if (devc->model == ESS_MAESTRO)
    {
      OUTW (devc->osdev, 0xA1A0, gwPTBaseIO + 0x14);
    }
  else
    {
      OUTW (devc->osdev, 0xA1A0, gwPTBaseIO + 0x14);
    }

/* First get the Codec ID type - check if it is PT101 or AC97*/
  codec_id = ac97_read (devc, 0x00);

/* initialize the PT101 Codec */
  if (codec_id == 0x80)
    {
      ac97_write (devc, 0x09, 0x0000);	/*DAC Mute */
      ac97_write (devc, 0x0F, 0x0000);
      ac97_write (devc, 0x11, 0x0000);
      ac97_write (devc, 0x14, 0x0000);
      ac97_write (devc, 0x1A, 0x0105);
      ac97_write (devc, 0x1E, 0x0101);	/*set PT101 reg 0x1F */
      ac97_write (devc, 0x1F, 0x8080);	/*set PT101 reg 0x1F */
    }

  if ((devc->model == ESS_MAESTRO2E) || (devc->model == ESS_MAESTRO2))
    {
      outpw (gwPTBaseIO + 0x02, (WORD) 0x0001);
      outpw (gwPTBaseIO + 0x00, (WORD) 0x03C0);
      outpw (gwPTBaseIO + 0x02, (WORD) 0x1);
      outpw (gwPTBaseIO + 0x02, (WORD) 0x1);
      outpw (gwPTBaseIO + 0x00, (WORD) 0x3c0);
      outpw (gwPTBaseIO + 0x02, (WORD) 0x0000);
      outpw (gwPTBaseIO + 0x00, (WORD) 0x4010);
      outpw (gwPTBaseIO + 0x02, (WORD) 0x0001);
      outpw (gwPTBaseIO + 0x00, (WORD) 0x03D0);
      outpw (gwPTBaseIO + 0x02, (WORD) 0x1);
      outpw (gwPTBaseIO + 0x02, (WORD) 0x1);
      outpw (gwPTBaseIO + 0x00, (WORD) 0x3d0);
      outpw (gwPTBaseIO + 0x02, (WORD) 0x0000);
      outpw (gwPTBaseIO + 0x00, (WORD) 0x4010);
    }
  else
    {
      outpw (gwPTBaseIO + 0x02, (WORD) 0x0001);
      outpw (gwPTBaseIO + 0x00, (WORD) 0x03C0);
      outpw (gwPTBaseIO + 0x02, (WORD) 0x00);
      outpw (gwPTBaseIO + 0x00, (WORD) 0x401F);

      outpw (gwPTBaseIO + 0x02, (WORD) 0x0001);
      outpw (gwPTBaseIO + 0x00, (WORD) 0x03D0);
      outpw (gwPTBaseIO + 0x02, (WORD) 0x00);
      outpw (gwPTBaseIO + 0x00, (WORD) 0x401F);
    }

/***********************ALLOCATE MEMORY************************/
  {
    int ret;

    ret = allocate_maestro_bufs (devc);
    if (ret != 0)
      {
	cmn_err (CE_WARN, "Couldn't allocate Maestro memory\n");
	return 1;
      }
    set_maestro_base (devc);
  }
/******************INIT MIXERS*********************************/
  if (codec_id == 0x80)
    {
      my_mixer = oss_install_mixer (OSS_MIXER_DRIVER_VERSION,
				    devc->osdev,
				    devc->osdev,
				    "ESS PT101",
				    &pt101_mixer_driver,
				    sizeof (mixer_driver_t), devc);
      if (my_mixer < 0)
	return 0;
      else
	pt101_mixer_reset (devc);
    }
  else
    {
      /* Reset the codec */
      my_mixer = ac97_install (&devc->ac97devc, "Maestro2 AC97 Mixer",
			       ac97_read, ac97_write, devc, devc->osdev);
      if (my_mixer < 0)
	return 0;
    }
  for (i = 0; i < MAX_PORTC; i++)
    {
      int adev;
      int caps = ADEV_AUTOMODE;
      maestro_portc *portc = &devc->portc[i];
      char tmp_name[100];
      strcpy (tmp_name, devc->chip_name);

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
					&maestro_audio_driver,
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
#if 0
	  audio_engines[adev]->min_block = 4096;
	  audio_engines[adev]->max_block = 4096;
#endif
	  audio_engines[adev]->min_rate = 5000;
	  audio_engines[adev]->max_rate = 48000;
	  audio_engines[adev]->caps |= PCM_CAP_FREERATE;
	  portc->audiodev = adev;
	  portc->open_mode = 0;
	  portc->trigger_bits = 0;
	  portc->speed = 0;
	  portc->bits = 0;
	  portc->channels = 0;
	  audio_engines[adev]->mixer_dev = my_mixer;
	  audio_engines[adev]->vmix_flags = VMIX_MULTIFRAG;
#ifdef CONFIG_OSS_VMIX
	  if (i == 0)
	     vmix_attach_audiodev(devc->osdev, adev, -1, 0);
#endif
	}
    }
  return 1;
}

/* NEC Versas ? */
#define NEC_VERSA_SUBID1        0x80581033
#define NEC_VERSA_SUBID2        0x803c1033

int
oss_maestro_attach (oss_device_t * osdev)
{
  unsigned short sdata;
  unsigned char pci_irq_line, pci_revision;
  unsigned short pci_command, vendor, device;
  unsigned int pci_ioaddr, subid;
  maestro_devc *devc;

  DDB (cmn_err (CE_WARN, "Entered ESS Maestro probe routine\n"));

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_dword (osdev, PCI_SUBSYSTEM_VENDOR_ID, &subid);
  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_0, &pci_ioaddr);
  if ((vendor != ESS_VENDOR_ID && vendor != ESS2_VENDOR_ID) ||
      (device != ESS_MAESTRO && device != ESS_MAESTRO2 &&
       device != ESS_MAESTRO2E))

    return 0;

  DDB (cmn_err (CE_WARN, "Maestro I/O base %04x\n", pci_ioaddr));


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

  devc->base = MAP_PCI_IOADDR (devc->osdev, 0, pci_ioaddr);
  /* Remove I/O space marker in bit 0. */
  devc->base &= ~0x3;

  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_IO;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  switch (device)
    {
    case ESS_MAESTRO2E:
      devc->model = MD_MAESTRO2E;
      devc->chip_name = "Maestro-2E";
      SYSCLK = 50000000L;
      break;

    case ESS_MAESTRO2:
      devc->model = MD_MAESTRO2;
      devc->chip_name = "Maestro-2";
      SYSCLK = 50000000L;
      break;

    case ESS_MAESTRO:
    default:
      devc->model = MD_MAESTRO1;
      devc->chip_name = "Maestro-1";
      SYSCLK = 49152000L;
      break;
    }

  if (subid == NEC_VERSA_SUBID1 || subid == NEC_VERSA_SUBID2)
    {
      /* turn on external amp? */
      OUTW (devc->osdev, 0xf9ff, devc->base + 0x64);
      OUTW (devc->osdev, INW (devc->osdev, devc->base + 0x68) | 0x600,
	    devc->base + 0x68);
      OUTW (devc->osdev, 0x0209, devc->base + 0x60);
    }


  /* Legacy Audio Control */
  pci_read_config_word (osdev, 0x40, &sdata);
  sdata |= (1 << 15);		/* legacy decode off */
  sdata &= ~(1 << 14);		/* Disable SIRQ */
  sdata &= ~(0x1f);		/* disable mpu irq/io, game port, fm, SB */

  pci_write_config_word (osdev, 0x40, sdata);

  /* MIDI/SB I/O, IRQ Control */
  pci_read_config_word (osdev, 0x50, &sdata);
  sdata &= ~(1 << 5);
  pci_write_config_word (osdev, 0x50, sdata);

  /* GPIO Control */
  pci_read_config_word (osdev, 0x52, &sdata);
  sdata &= ~(1 << 15);		/* Turn off internal clock multiplier */
  /* XXX how do we know which to use? */
  sdata &= ~(1 << 14);		/* External clock */
  sdata &= ~(1 << 7);		/* HWV off */
  sdata &= ~(1 << 6);		/* Debounce off */
  sdata &= ~(1 << 5);		/* GPIO 4:5 */
  sdata |= (1 << 4);		/* Disconnect from the CHI. */
  /* Enabling this made dell 7500 work. */
  sdata &= ~(1 << 2);		/* MIDI fix off (undoc) */
  sdata &= ~(1 << 1);		/* reserved, always write 0 */
  pci_write_config_word (osdev, 0x52, sdata);
  if (devc->model == ESS_MAESTRO2E)
    {
      pci_write_config_word (osdev, 0x54, 0x0000);
      pci_write_config_word (osdev, 0x56, 0x0000);
      pci_write_config_word (osdev, 0x58, 0x0000);
      pci_write_config_word (osdev, 0xC4, 0x8000);
      pci_read_config_word (osdev, 0x68, &devc->gpio);
    }

  MUTEX_INIT (devc->osdev, devc->lock, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_lock, MH_DRV + 1);

  oss_register_device (osdev, devc->chip_name);

  if (oss_register_interrupts (devc->osdev, 0, maestrointr, NULL) < 0)
    {
      cmn_err (CE_WARN, "Can't allocate IRQ%d\n", pci_irq_line);
      return 0;
    }

  return init_maestro (devc);	/* Detected */
}


int
oss_maestro_detach (oss_device_t * osdev)
{
  maestro_devc *devc = (maestro_devc *) osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  /* Stop WP interrutps */
  OUTW (devc->osdev, (WORD) (INW (devc->osdev, devc->base + 0x18) & 0xFFF8),
	devc->base + 0x18);
  OUTW (devc->osdev, 0x0001, devc->base + 0x04);
  vSetWCEnable (devc, DISABLE);
  vMskIDRBit (devc, 0x17, 0);	/* Disable Bob Timer Interrupts */

  oss_unregister_interrupts (devc->osdev);
  if (devc->dmapages != NULL)
    CONTIG_FREE (devc->osdev, devc->dmapages, devc->dmalen, TODO);

  MUTEX_CLEANUP (devc->lock);
  MUTEX_CLEANUP (devc->low_lock);
  UNMAP_PCI_IOADDR (devc->osdev, 0);

  oss_unregister_device (osdev);
  return 1;

}
