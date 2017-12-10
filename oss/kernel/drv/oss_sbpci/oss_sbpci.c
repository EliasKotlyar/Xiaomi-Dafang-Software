/*
 * Purpose: Creative/Ensoniq AudioPCI97  driver (ES1371/ES1373)
 *
 * This driver is used with the original Ensoniq AudioPCI97 card and many
 * PCI based Sound Blaster cards by Creative Technologies. For example
 * Sound Blaster PCI128 and Creative/Ectiva EV1938.
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

#include "oss_sbpci_cfg.h"
#include "midi_core.h"
#include "sbpci.h"
#include "ac97.h"
#include "oss_pci.h"


extern int apci_latency;
extern int apci_spdif;

#define ENSONIQ_VENDOR_ID	0x1274
#define ECTIVA_VENDOR_ID	0x1102
#define ENSONIQ_ES1371		0x1371
#define ENSONIQ_ES5880      	0x8001
#define ENSONIQ_ES5880A     	0x8002
#define ENSONIQ_ES5880B     	0x5880
#define ECTIVA_ES1938		0x8938

#define MAX_PORTC 2

typedef struct apci97_portc
{

  /* Audio parameters */
  int audiodev;
  int open_mode;
  int trigger_bits;
  int audio_enabled;
  int speed, bits, channels;
  int atype;			/* 0=DAC/ADC, 1=Synth */
}
apci97_portc;

typedef struct apci97_devc
{
  oss_device_t *osdev;
  oss_mutex_t mutex, low_mutex;
  oss_native_word base;
  int irq;
  char *chip_name;
  int revision;

  apci97_portc portc[MAX_PORTC];
/*
 * Mixer
 */
  ac97_devc ac97devc;

/*
 * MIDI
 */
  int midi_opened;
  int midi_dev;
  oss_midi_inputbyte_t midi_input_intr;
}
apci97_devc;


void SRCRegWrite (apci97_devc * devc, unsigned short reg, unsigned short val);
void SRCSetRate (apci97_devc * devc, unsigned char base, unsigned short rate);


static int
ac97_read (void *devc_, int wAddr)
{
  apci97_devc *devc = devc_;
  int i, dtemp, dinit;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  dtemp = INL (devc->osdev, devc->base + CONC_dCODECCTL_OFF);
  /* wait for WIP to go away saving the current state for later */
  for (i = 0; i < 0x100UL; ++i)
    if (!(INL (devc->osdev, devc->base + CONC_dCODECCTL_OFF) & (1UL << 30)))
      break;

  /* write addr w/data=0 and assert read request ... */

  /* save the current state for later */
  dinit = INL (devc->osdev, devc->base + CONC_dSRCIO_OFF);

  /* enable SRC state data in SRC mux */
  for (i = 0; i < 0x100UL; ++i)
    if (!
	((dtemp =
	  INL (devc->osdev, devc->base + CONC_dSRCIO_OFF)) & SRC_BUSY))
      break;
  OUTL (devc->osdev, (dtemp & SRC_CTLMASK) | 0x00010000UL,
	devc->base + CONC_dSRCIO_OFF);

  /* wait for a SAFE time to write a read request and then do it, dammit */

  for (i = 0; i < 0x100UL; ++i)
    {
      if ((INL (devc->osdev, devc->base + CONC_dSRCIO_OFF) & 0x00070000UL) ==
	  0x00010000UL)
	break;
    }

  OUTL (devc->osdev,
	((int) wAddr << 16) | (1UL << 23), devc->base + CONC_dCODECCTL_OFF);

  /* restore SRC reg */
  for (i = 0; i < 0x100UL; ++i)
    if (!
	((dtemp =
	  INL (devc->osdev, devc->base + CONC_dSRCIO_OFF)) & SRC_BUSY))
      break;
  OUTL (devc->osdev, dinit, devc->base + CONC_dSRCIO_OFF);

  /* now wait for the stinkin' data (RDY) */
  for (i = 0; i < 0x100UL; ++i)
    if (INL (devc->osdev, devc->base + CONC_dCODECCTL_OFF) & (1UL << 31))
      break;
  dtemp = INL (devc->osdev, devc->base + CONC_dCODECCTL_OFF);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);

  return dtemp & 0xffff;
}

static int
ac97_write (void *devc_, int wAddr, int wData)
{
  apci97_devc *devc = devc_;
  int i, dtemp, dinit;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  /* wait for WIP to go away */
  for (i = 0; i < 0x100UL; ++i)
    if (!(INL (devc->osdev, devc->base + CONC_dCODECCTL_OFF) & (1UL << 30)))
      break;

  /* save the current state for later */
  dinit = INL (devc->osdev, devc->base + CONC_dSRCIO_OFF);

  dtemp = INL (devc->osdev, devc->base + CONC_dSRCIO_OFF);
  /* enable SRC state data in SRC mux */
  for (i = 0; i < 0x100UL; ++i)
    if (!
	((dtemp =
	  INL (devc->osdev, devc->base + CONC_dSRCIO_OFF)) & SRC_BUSY))
      break;
  OUTL (devc->osdev, (dtemp & SRC_CTLMASK) | 0x00010000UL,
	devc->base + CONC_dSRCIO_OFF);

  /* wait for a SAFE time to write addr/data and then do it, dammit */
  for (i = 0; i < 0x100UL; ++i)
    {
      if ((INL (devc->osdev, devc->base + CONC_dSRCIO_OFF) & 0x00070000UL) ==
	  0x00010000UL)
	break;
    }

  OUTL (devc->osdev, ((int) wAddr << 16) | wData,
	devc->base + CONC_dCODECCTL_OFF);

  /* restore SRC reg */
  for (i = 0; i < 0x100UL; ++i)
    if (!(INL (devc->osdev, devc->base + CONC_dSRCIO_OFF) & SRC_BUSY))
      break;
  OUTL (devc->osdev, dinit, devc->base + CONC_dSRCIO_OFF);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);

  return 0;
}


void
SRCInit (apci97_devc * devc)
{
  int i;

  /* Clear all SRC RAM then init - keep SRC disabled until done */
  for (i = 0; i < SRC_IOPOLL_COUNT; ++i)
    if (!(INL (devc->osdev, devc->base + CONC_dSRCIO_OFF) & SRC_BUSY))
      break;
  OUTL (devc->osdev, SRC_DISABLE, devc->base + CONC_dSRCIO_OFF);

  for (i = 0; i < 0x80; ++i)
    SRCRegWrite (devc, (unsigned short) i, 0U);

  SRCRegWrite (devc, SRC_SYNTH_BASE + SRC_TRUNC_N_OFF, 16 << 4);
  SRCRegWrite (devc, SRC_SYNTH_BASE + SRC_INT_REGS_OFF, 16 << 10);
  SRCRegWrite (devc, SRC_DAC_BASE + SRC_TRUNC_N_OFF, 16 << 4);
  SRCRegWrite (devc, SRC_DAC_BASE + SRC_INT_REGS_OFF, 16 << 10);
  SRCRegWrite (devc, SRC_SYNTH_VOL_L, 1 << 12);
  SRCRegWrite (devc, SRC_SYNTH_VOL_R, 1 << 12);
  SRCRegWrite (devc, SRC_DAC_VOL_L, 1 << 12);
  SRCRegWrite (devc, SRC_DAC_VOL_R, 1 << 12);
  SRCRegWrite (devc, SRC_ADC_VOL_L, 1 << 12);
  SRCRegWrite (devc, SRC_ADC_VOL_R, 1 << 12);

  /* default some rates */
  SRCSetRate (devc, SRC_SYNTH_BASE, 8000);
  SRCSetRate (devc, SRC_DAC_BASE, 8000);
  SRCSetRate (devc, SRC_ADC_BASE, 8000);

  /* now enable the whole deal */
  for (i = 0; i < SRC_IOPOLL_COUNT; ++i)
    if (!(INL (devc->osdev, devc->base + CONC_dSRCIO_OFF) & SRC_BUSY))
      break;
  OUTL (devc->osdev, 0, devc->base + CONC_dSRCIO_OFF);

  return;
}

unsigned short
SRCRegRead (apci97_devc * devc, unsigned short reg)
{
  int i, dtemp;

  dtemp = INL (devc->osdev, devc->base + CONC_dSRCIO_OFF);
  /* wait for ready */
  for (i = 0; i < SRC_IOPOLL_COUNT; ++i)
    if (!
	((dtemp =
	  INL (devc->osdev, devc->base + CONC_dSRCIO_OFF)) & SRC_BUSY))
      break;

  /* assert a read request */
  OUTL (devc->osdev,
	(dtemp & SRC_CTLMASK) | ((int) reg << 25),
	devc->base + CONC_dSRCIO_OFF);

  /* now wait for the data */
  for (i = 0; i < SRC_IOPOLL_COUNT; ++i)
    if (!
	((dtemp =
	  INL (devc->osdev, devc->base + CONC_dSRCIO_OFF)) & SRC_BUSY))
      break;

  return (unsigned short) dtemp;
}


void
SRCRegWrite (apci97_devc * devc, unsigned short reg, unsigned short val)
{
  int i, dtemp;
  int writeval;

  dtemp = INL (devc->osdev, devc->base + CONC_dSRCIO_OFF);
  /* wait for ready */
  for (i = 0; i < SRC_IOPOLL_COUNT; ++i)
    if (!
	((dtemp =
	  INL (devc->osdev, devc->base + CONC_dSRCIO_OFF)) & SRC_BUSY))
      break;

  /* assert the write request */
  writeval = (dtemp & SRC_CTLMASK) | SRC_WENABLE | ((int) reg << 25) | val;
  OUTL (devc->osdev, writeval, devc->base + CONC_dSRCIO_OFF);

  return;
}

typedef struct
{
  unsigned char base;
  unsigned short rate;
}
SRC_RATE_REC;

#define SRC_RATE_RECS (3)
SRC_RATE_REC theSRCRates[SRC_RATE_RECS] = {
  {SRC_SYNTH_BASE, 0},
  {SRC_DAC_BASE, 0},
  {SRC_ADC_BASE, 0}
};

/*ARGSUSED*/
unsigned short
SRCGetRate (apci97_devc * devc, unsigned char base)
{
  unsigned short i;

  for (i = 0; i < SRC_RATE_RECS; i++)
    if (theSRCRates[i].base == base)
      return theSRCRates[i].rate;

  return 0;
}

void
SRCSetRate (apci97_devc * devc, unsigned char base, unsigned short rate)
{
  int i, freq, dtemp;
  unsigned short N, truncM, truncStart;


  for (i = 0; i < SRC_RATE_RECS; i++)
    if (theSRCRates[i].base == base)
      {
	theSRCRates[i].rate = rate;
	break;
      }

  if (base != SRC_ADC_BASE)
    {
      /* freeze the channel */
      dtemp = base == SRC_SYNTH_BASE ? SRC_SYNTHFREEZE : SRC_DACFREEZE;
      for (i = 0; i < SRC_IOPOLL_COUNT; ++i)
	if (!(INL (devc->osdev, devc->base + CONC_dSRCIO_OFF) & SRC_BUSY))
	  break;
      OUTL (devc->osdev,
	    (INL (devc->osdev, devc->base + CONC_dSRCIO_OFF) & SRC_CTLMASK) |
	    dtemp, devc->base + CONC_dSRCIO_OFF);

      /* calculate new frequency and write it - preserve accum */
      freq = ((int) rate << 16) / 3000U;
      SRCRegWrite (devc, (unsigned short) base + SRC_INT_REGS_OFF,
		   (SRCRegRead
		    (devc,
		     (unsigned short) base +
		     SRC_INT_REGS_OFF) & 0x00ffU) | ((unsigned short) (freq >>
								       6) &
						     0xfc00));
      SRCRegWrite (devc, (unsigned short) base + SRC_VFREQ_FRAC_OFF,
		   (unsigned short) freq >> 1);

      /* un-freeze the channel */
      for (i = 0; i < SRC_IOPOLL_COUNT; ++i)
	if (!(INL (devc->osdev, devc->base + CONC_dSRCIO_OFF) & SRC_BUSY))
	  break;
      OUTL (devc->osdev,
	    (INL (devc->osdev, devc->base + CONC_dSRCIO_OFF) & SRC_CTLMASK) &
	    ~dtemp, devc->base + CONC_dSRCIO_OFF);
    }
  else
    {
      /* derive oversample ratio */
      N = rate / 3000U;
      if (N == 15 || N == 13 || N == 11 || N == 9)
	--N;

      /* truncate the filter and write n/trunc_start */
      truncM = (21 * N - 1) | 1;
      if (rate >= 24000U)
	{
	  if (truncM > 239)
	    truncM = 239;
	  truncStart = (239 - truncM) >> 1;

	  SRCRegWrite (devc, base + SRC_TRUNC_N_OFF,
		       (truncStart << 9) | (N << 4));
	}
      else
	{
	  if (truncM > 119)
	    truncM = 119;
	  truncStart = (119 - truncM) >> 1;

	  SRCRegWrite (devc, base + SRC_TRUNC_N_OFF,
		       0x8000U | (truncStart << 9) | (N << 4));
	}

      /* calculate new frequency and write it - preserve accum */
      freq = ((48000UL << 16) / rate) * N;
      SRCRegWrite (devc, base + SRC_INT_REGS_OFF,
		   (SRCRegRead
		    (devc,
		     (unsigned short) base +
		     SRC_INT_REGS_OFF) & 0x00ff) | ((unsigned short) (freq >>
								      6) &
						    0xfc00));
      SRCRegWrite (devc, base + SRC_VFREQ_FRAC_OFF,
		   (unsigned short) freq >> 1);

      SRCRegWrite (devc, SRC_ADC_VOL_L, N << 8);
      SRCRegWrite (devc, SRC_ADC_VOL_R, N << 8);

    }

  return;
}

static void
apci97_writemem (apci97_devc * devc, int page, int offs, int data)
{
  int tmp;

  tmp = INL (devc->osdev, devc->base + 0xc);
  OUTL (devc->osdev, page, devc->base + 0xc);	/* Select memory page */
  OUTL (devc->osdev, data, devc->base + offs);
  OUTL (devc->osdev, tmp, devc->base + 0xc);	/* Select the original memory page */
}

static unsigned int
apci97_readmem (apci97_devc * devc, int page, int offs)
{
  unsigned int val;

  OUTL (devc->osdev, page, devc->base + 0xc);	/* Select memory page */
  val = INL (devc->osdev, devc->base + offs);
  return val;
}

static int
apci97intr (oss_device_t * osdev)
{
  int stats, i;
  int tmp;
  unsigned char ackbits = 0;
  unsigned char uart_stat;
  apci97_devc *devc = (apci97_devc *) osdev->devc;
  apci97_portc *portc;
  int served = 0;

  stats = INL (devc->osdev, devc->base + 0x04);
  /*cmn_err (CE_WARN, "AudioPCI97 intr status %08x\n", stats); */

  if (!(stats & 0x80000000))	/* No interrupt pending */
    return served;

  served = 1;

  for (i = 0; i < MAX_PORTC; i++)
    {
      portc = &devc->portc[i];

      if (stats & 0x00000010)	/* CCB interrupt */
	{
	  cmn_err (CE_WARN, "CCB interrupt\n");
	}

      if ((stats & 0x00000004) && (portc->atype))	/* DAC1 (synth) interrupt */
	{
	  ackbits |= CONC_SERCTL_DAC1IE;
	  if (portc->trigger_bits & PCM_ENABLE_OUTPUT)
	    oss_audio_outputintr (portc->audiodev, 0);

	}

      if ((stats & 0x00000002) && (!portc->atype))	/* DAC2 interrupt */
	{
	  ackbits |= CONC_SERCTL_DAC2IE;
	  if (portc->trigger_bits & PCM_ENABLE_OUTPUT)
	    oss_audio_outputintr (portc->audiodev, 0);
	}

      if ((stats & 0x00000001) && (!portc->atype))	/* ADC interrupt */
	{
	  ackbits |= CONC_SERCTL_ADCIE;
	  if (portc->trigger_bits & PCM_ENABLE_INPUT)
	    {
	      oss_audio_inputintr (portc->audiodev, 0);
	    }
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
      /* Ack the interrupt */
      tmp = INB (devc->osdev, devc->base + CONC_bSERCTL_OFF);
      OUTB (devc->osdev, (tmp & ~ackbits), devc->base + CONC_bSERCTL_OFF);	/* Clear bits */
      OUTB (devc->osdev, tmp | ackbits, devc->base + CONC_bSERCTL_OFF);	/* Return them back on */
    }

  return served;
}

/*
 * Audio routines
 */

static int
apci97_audio_set_rate (int dev, int arg)
{
  apci97_portc *portc = audio_engines[dev]->portc;

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
apci97_audio_set_channels (int dev, short arg)
{
  apci97_portc *portc = audio_engines[dev]->portc;

  if ((arg != 1) && (arg != 2))
    return portc->channels;
  portc->channels = arg;

  return portc->channels;
}

static unsigned int
apci97_audio_set_format (int dev, unsigned int arg)
{
  apci97_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->bits;


  if (!(arg & (AFMT_U8 | AFMT_S16_LE | AFMT_AC3)))
    return portc->bits;
  portc->bits = arg;

  return portc->bits;
}

/*ARGSUSED*/
static int
apci97_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void apci97_audio_trigger (int dev, int state);

static void
apci97_audio_reset (int dev)
{
  apci97_audio_trigger (dev, 0);
}

static void
apci97_audio_reset_input (int dev)
{
  apci97_portc *portc = audio_engines[dev]->portc;
  apci97_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
apci97_audio_reset_output (int dev)
{
  apci97_portc *portc = audio_engines[dev]->portc;
  apci97_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

/*ARGSUSED*/
static int
apci97_audio_open (int dev, int mode, int open_flags)
{
  apci97_portc *portc = audio_engines[dev]->portc;
  apci97_devc *devc = audio_engines[dev]->devc;
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
apci97_audio_close (int dev, int mode)
{
  apci97_portc *portc = audio_engines[dev]->portc;

  apci97_audio_reset (dev);
  portc->open_mode = 0;
  portc->audio_enabled &= ~mode;
}

/*ARGSUSED*/
static void
apci97_audio_output_block (int dev, oss_native_word buf, int count,
			   int fragsize, int intrflag)
{
  apci97_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;

}

/*ARGSUSED*/
static void
apci97_audio_start_input (int dev, oss_native_word buf, int count,
			  int fragsize, int intrflag)
{
  apci97_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
}

static void
apci97_audio_trigger (int dev, int state)
{
  apci97_devc *devc = audio_engines[dev]->devc;
  apci97_portc *portc = audio_engines[dev]->portc;
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
		  tmp &= ~CONC_SERCTL_DAC1IE;
		  OUTB (devc->osdev, tmp, devc->base + CONC_bSERCTL_OFF);
		}
	      else
		{
		  tmp = INB (devc->osdev, devc->base + CONC_bDEVCTL_OFF);
		  tmp &= ~CONC_DEVCTL_DAC2_EN;
		  OUTB (devc->osdev, tmp, devc->base + CONC_bDEVCTL_OFF);

		  tmp = INB (devc->osdev, devc->base + CONC_bSERCTL_OFF);
		  tmp &= ~CONC_SERCTL_DAC2IE;
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
apci97_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  dmap_t *dmap = audio_engines[dev]->dmap_in;
  apci97_devc *devc = audio_engines[dev]->devc;
  apci97_portc *portc = audio_engines[dev]->portc;
  int tmp = 0x00;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  /* Set physical address of the DMA buffer */

  apci97_writemem (devc, CONC_ADCCTL_PAGE, CONC_dADCPADDR_OFF,
		   dmap->dmabuf_phys);

  /* Set ADC rate */
  SRCSetRate (devc, SRC_ADC_BASE, portc->speed);

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
  apci97_writemem (devc, CONC_ADCCTL_PAGE, CONC_wADCFC_OFF,
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

  portc->audio_enabled &= ~PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

/*ARGSUSED*/
static int
apci97_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  dmap_t *dmap = audio_engines[dev]->dmap_out;
  apci97_devc *devc = audio_engines[dev]->devc;
  apci97_portc *portc = audio_engines[dev]->portc;
  int tmp;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (devc->revision >= 4)
    {
      /* set SPDIF to PCM mode */
      OUTL (devc->osdev, INL (devc->osdev, devc->base + 0x1c) & ~0x2,
	    devc->base + 0x1c);
      if (portc->bits & AFMT_AC3)
	{
	  portc->channels = 2;
	  portc->bits = 16;
	  portc->speed = 48000;
	  /* set S/PDIF to AC3 Mode */
	  OUTL (devc->osdev, INL (devc->osdev, devc->base + 0x1c) | 0x2,
		devc->base + 0x1c);
	}
    }

  if (portc->atype)
    {
      /* Set physical address of the DMA buffer */
      apci97_writemem (devc, CONC_SYNCTL_PAGE, CONC_dSYNPADDR_OFF,
		       dmap->dmabuf_phys);

      /* Set DAC1 rate */
      SRCSetRate (devc, SRC_SYNTH_BASE, portc->speed);

      /* Set format */
      tmp = INB (devc->osdev, devc->base + CONC_bSERFMT_OFF);
      tmp &= ~((CONC_PCM_DAC_STEREO | CONC_PCM_DAC_16BIT) >> 2);
      if (portc->channels == 2)
	tmp |= (CONC_PCM_DAC_STEREO >> 2);
      if (portc->bits == 16)
	{
	  tmp |= (CONC_PCM_DAC_16BIT >> 2);
	}
      OUTB (devc->osdev, tmp, devc->base + CONC_bSERFMT_OFF);

      /* Set the frame count */
      apci97_writemem (devc, CONC_SYNCTL_PAGE, CONC_wSYNFC_OFF,
		       (dmap->bytes_in_use / 4) - 1);

      /* Set # of samples between interrupts */
      OUTW (devc->osdev,
	    (dmap->fragment_size / ((portc->channels * portc->bits) / 8)) - 1,
	    devc->base + CONC_wSYNIC_OFF);

      /* Enable the wave interrupt */
      tmp =
	INB (devc->osdev,
	     devc->base + CONC_bSERCTL_OFF) & ~CONC_SERCTL_DAC1IE;
      OUTB (devc->osdev, tmp, devc->base + CONC_bSERCTL_OFF);
      tmp |= CONC_SERCTL_DAC1IE;
      OUTB (devc->osdev, tmp, devc->base + CONC_bSERCTL_OFF);

    }
  else
    {
      /* Set physical address of the DMA buffer */
      apci97_writemem (devc, CONC_DACCTL_PAGE, CONC_dDACPADDR_OFF,
		       dmap->dmabuf_phys);

      /* Set DAC rate */
      SRCSetRate (devc, SRC_DAC_BASE, portc->speed);

      /* Set format */
      tmp = INB (devc->osdev, devc->base + CONC_bSERFMT_OFF);
      tmp &= ~(CONC_PCM_DAC_STEREO | CONC_PCM_DAC_16BIT);
      if (portc->channels == 2)
	tmp |= CONC_PCM_DAC_STEREO;
      if (portc->bits == 16)
	{
	  tmp |= CONC_PCM_DAC_16BIT;
	  OUTB (devc->osdev, 0x10, devc->base + CONC_bSKIPC_OFF);	/* Skip count register */
	}
      else
	{
	  OUTB (devc->osdev, 0x08, devc->base + CONC_bSKIPC_OFF);	/* Skip count register */
	}
      OUTB (devc->osdev, tmp, devc->base + CONC_bSERFMT_OFF);


      /* Set the frame count */
      apci97_writemem (devc, CONC_DACCTL_PAGE, CONC_wDACFC_OFF,
		       (dmap->bytes_in_use / 4) - 1);

      /* Set # of samples between interrupts */
      OUTW (devc->osdev,
	    (dmap->fragment_size / ((portc->channels * portc->bits) / 8)) - 1,
	    devc->base + CONC_wDACIC_OFF);

      /* Enable the wave interrupt */
      tmp =
	INB (devc->osdev,
	     devc->base + CONC_bSERCTL_OFF) & ~CONC_SERCTL_DAC2IE;
      OUTB (devc->osdev, tmp, devc->base + CONC_bSERCTL_OFF);
      tmp |= CONC_SERCTL_DAC2IE;
      OUTB (devc->osdev, tmp, devc->base + CONC_bSERCTL_OFF);
    }

  portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/
static int
apci97_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  apci97_devc *devc = audio_engines[dev]->devc;
  apci97_portc *portc = audio_engines[dev]->portc;
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

  ptr = apci97_readmem (devc, page, port);
  ptr >>= 16;
  ptr <<= 2;			/* count is in dwords */
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);

  return ptr;
}

audiodrv_t apci97_audio_driver = {
  apci97_audio_open,
  apci97_audio_close,
  apci97_audio_output_block,
  apci97_audio_start_input,
  apci97_audio_ioctl,
  apci97_audio_prepare_for_input,
  apci97_audio_prepare_for_output,
  apci97_audio_reset,
  NULL,
  NULL,
  apci97_audio_reset_input,
  apci97_audio_reset_output,
  apci97_audio_trigger,
  apci97_audio_set_rate,
  apci97_audio_set_format,
  apci97_audio_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,				/* apci97_alloc_buffer */
  NULL,				/* apci97_free_buffer */
  NULL,
  NULL,
  apci97_get_buffer_pointer
};

/*ARGSUSED*/
static int
apci97_midi_open (int dev, int mode, oss_midi_inputbyte_t inputbyte,
		  oss_midi_inputbuf_t inputbuf,
		  oss_midi_outputintr_t outputintr)
{
  apci97_devc *devc = (apci97_devc *) midi_devs[dev]->devc;

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
apci97_midi_close (int dev, int mode)
{
  apci97_devc *devc = (apci97_devc *) midi_devs[dev]->devc;

  OUTB (devc->osdev, 0x00, devc->base + CONC_bUARTCSTAT_OFF);
  devc->midi_opened = 0;
}

static int
apci97_midi_out (int dev, unsigned char midi_byte)
{
  apci97_devc *devc = (apci97_devc *) midi_devs[dev]->devc;
  int i;

  unsigned char uart_stat =
    INB (devc->osdev, devc->base + CONC_bUARTCSTAT_OFF);

  i = 0;
  while (i < 1000000 && !(uart_stat & CONC_UART_TXRDY))
    {
      uart_stat = INB (devc->osdev, devc->base + CONC_bUARTCSTAT_OFF);
      i++;
    }

  if (!(uart_stat & CONC_UART_TXRDY))
    return 0;


  OUTB (devc->osdev, midi_byte, devc->base + CONC_bUARTDATA_OFF);

  return 1;
}

/*ARGSUSED*/
static int
apci97_midi_ioctl (int dev, unsigned cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static midi_driver_t apci97_midi_driver = {
  apci97_midi_open,
  apci97_midi_close,
  apci97_midi_ioctl,
  apci97_midi_out
};

static int
apci97_control (int dev, int ctrl, unsigned int cmd, int value)
{
  apci97_devc *devc = mixer_devs[dev]->hw_devc;

  if (cmd == SNDCTL_MIX_READ)
    {
      value = 0;
      switch (ctrl)
	{
	case 1:		/* Speaker Mode */
	  value = (INL (devc->osdev, devc->base + 4) & (1 << 26) ? 1 : 0);
	  break;

	case 2:		/* Dual Dac Mode */
	  value = INL (devc->osdev, devc->base + 4) & (1 << 27) ? 1 : 0;
	  break;
	}
    }
  if (cmd == SNDCTL_MIX_WRITE)
    {
      switch (ctrl)
	{
	case 1:		/* Front/Rear Mirror */
	  if (value)
	    {
	      OUTL (devc->osdev,
		    INL (devc->osdev, devc->base + 4) | (1 << 26),
		    devc->base + 4);
	    }
	  else
	    {
	      OUTL (devc->osdev,
		    INL (devc->osdev, devc->base + 4) & ~(1 << 26),
		    devc->base + 4);
	    }
	  break;

	case 2:		/* DAC1->Front DAC2->REAR */
	  if (value)
	    {
	      /* disable front/rear mirroring */
	      OUTL (devc->osdev,
		    INL (devc->osdev, devc->base + 4) & ~(1 << 26),
		    devc->base + 4);
	      /* Enable Dual Dac mode */
	      OUTL (devc->osdev,
		    INL (devc->osdev, devc->base + 4) | (1 << 27) | (1 << 24),
		    devc->base + 4);
	    }
	  else
	    {
	      /* enable mirror */
	      OUTL (devc->osdev,
		    INL (devc->osdev, devc->base + 4) | (1 << 26),
		    devc->base + 4);
	      /* disable dual dac */
	      OUTL (devc->osdev,
		    INL (devc->osdev,
			 devc->base + 4) & ~((1 << 27) | (1 << 24)),
		    devc->base + 4);
	    }
	  break;
	}
    }
  return value;
}

static int
apci97_mix_init (int dev)
{
  int group, err;

  if ((group = mixer_ext_create_group (dev, 0, "MIXEXT")) < 0)
    return group;

  if ((err = mixer_ext_create_control (dev, group, 1, apci97_control,
				       MIXT_ENUM, "SPKMODE", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err = mixer_ext_create_control (dev, group, 2, apci97_control,
				       MIXT_ONOFF, "DUALDAC", 1,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;
  return 0;

}

static int
init_apci97 (apci97_devc * devc, int device_id)
{
  int my_mixer;
  int tmp, i;
  int first_dev = 0;

  if ((device_id == ENSONIQ_ES5880) || (device_id == ENSONIQ_ES5880A) ||
      (device_id == ENSONIQ_ES5880B) ||
      (device_id == 0x1371 && devc->revision == 7) ||
      (device_id == 0x1371 && devc->revision >= 9))
    {
      int i;

      /* Have a ES5880 so enable the codec manually */
      tmp = INB (devc->osdev, devc->base + CONC_bINTSUMM_OFF) & 0xff;
      tmp |= 0x20;
      OUTB (devc->osdev, tmp, devc->base + CONC_bINTSUMM_OFF);	/* OUTB? */
      for (i = 0; i < 2000; i++)
	oss_udelay (10);
    }

  SRCInit (devc);
#if 0
  OUTB (devc->osdev, 0x00, devc->base + CONC_bSERCTL_OFF);
  OUTB (devc->osdev, 0x00, devc->base + CONC_bNMIENA_OFF);	/* NMI off */
  OUTB (devc->osdev, 0x00, devc->base + CONC_wNMISTAT_OFF);	/* OUTB? */
#endif
/*
 * Turn on UART and CODEC
 */
  tmp = INL (devc->osdev, devc->base + CONC_bDEVCTL_OFF) & 0xff;
  tmp &= ~(CONC_DEVCTL_PCICLK_DS | CONC_DEVCTL_XTALCLK_DS);
  OUTB (devc->osdev, tmp | CONC_DEVCTL_UART_EN | CONC_DEVCTL_JSTICK_EN,
	devc->base + CONC_bDEVCTL_OFF);
  OUTB (devc->osdev, 0x00, devc->base + CONC_bUARTCSTAT_OFF);

  /* Perform AC97 codec warm reset */
  tmp = INB (devc->osdev, devc->base + CONC_bMISCCTL_OFF) & 0xff;
  OUTB (devc->osdev, tmp | CONC_MISCCTL_SYNC_RES,
	devc->base + CONC_bMISCCTL_OFF);
  oss_udelay (200);
  OUTB (devc->osdev, tmp, devc->base + CONC_bMISCCTL_OFF);
  oss_udelay (200);

/*
 * Enable S/PDIF
 */
  if (devc->revision >= 4)
    {
      if (apci_spdif)
	{
	  /* enable SPDIF */
	  OUTL (devc->osdev, INL (devc->osdev, devc->base + 0x04) | (1 << 18),
		devc->base + 0x04);
	  /* SPDIF out = data from DAC */
	  OUTL (devc->osdev, INL (devc->osdev, devc->base + 0x00) | (1 << 26),
		devc->base + 0x00);
	}
      else
	{
	  /* disable spdif out */
	  OUTL (devc->osdev,
		INL (devc->osdev, devc->base + 0x04) & ~(1 << 18),
		devc->base + 0x04);
	  OUTL (devc->osdev,
		INL (devc->osdev, devc->base + 0x00) & ~(1 << 26),
		devc->base + 0x00);
	}
    }

/*
 * Init mixer
 */
  my_mixer =
    ac97_install (&devc->ac97devc, "AC97 Mixer", ac97_read, ac97_write, devc,
		  devc->osdev);

  if (my_mixer < 0)
    return 0;

  if (devc->revision >= 4)
    {
      /* enable 4 speaker mode */
      OUTL (devc->osdev, INL (devc->osdev, devc->base + 4) | (1 << 26),
	    devc->base + 4);
      mixer_ext_set_init_fn (my_mixer, apci97_mix_init, 5);
    }

  for (i = 0; i < MAX_PORTC; i++)
    {

      int adev;
      char tmp_name[100];
      apci97_portc *portc = &devc->portc[i];
      int caps = ADEV_AUTOMODE;
      int fmts = AFMT_U8 | AFMT_S16_LE;

      if (devc->revision >= 4)
	fmts |= AFMT_AC3;

      if (i == 0)
	{
	  sprintf (tmp_name, "%s (rev %d)", devc->chip_name, devc->revision);
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
					&apci97_audio_driver,
					sizeof (audiodrv_t),
					caps, fmts, devc, -1)) < 0)
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
	  audio_engines[adev]->min_rate = 5000;
	  audio_engines[adev]->max_rate = 48000;
	  audio_engines[adev]->caps |= PCM_CAP_FREERATE;
	  portc->open_mode = 0;
	  portc->audiodev = adev;
	  portc->atype = i;
#ifdef CONFIG_OSS_VMIX
	  if (i == 0)
	     vmix_attach_audiodev(devc->osdev, adev, -1, 0);
#endif
	}

      audio_engines[adev]->mixer_dev = my_mixer;
    }

  if ((devc->midi_dev = oss_install_mididev (OSS_MIDI_DRIVER_VERSION, "APCI97", "APCI97 UART", &apci97_midi_driver, sizeof (midi_driver_t),
					     0, devc, devc->osdev)) < 0)
    {
      cmn_err (CE_WARN, "Couldn't install MIDI device\n");
      return 0;
    }

  devc->midi_opened = 0;
  return 1;
}

int
oss_sbpci_attach (oss_device_t * osdev)
{
  unsigned char pci_irq_line, pci_revision;
  unsigned short pci_command, vendor, device;
  unsigned int pci_ioaddr;
  apci97_devc *devc;
  int err;

  DDB (cmn_err (CE_WARN, "Entered AudioPCI97 probe routine\n"));

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  if ((vendor != ENSONIQ_VENDOR_ID && vendor != ECTIVA_VENDOR_ID) ||
      (device != ENSONIQ_ES1371 && device != ENSONIQ_ES5880 &&
       device != ENSONIQ_ES5880A && device != ECTIVA_ES1938 &&
       device != ENSONIQ_ES5880B))

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

  switch (device)
    {
    case ENSONIQ_ES1371:
      devc->chip_name = "Creative AudioPCI97 (ES1371)";
      break;
    case ECTIVA_ES1938:
      devc->chip_name = "Ectiva AudioPCI";
      break;
    case ENSONIQ_ES5880:
    case ENSONIQ_ES5880A:
    case ENSONIQ_ES5880B:
      devc->chip_name = "Sound Blaster PCI128";
      break;
    default:
      devc->chip_name = "AudioPCI97";
    }

  devc->base = MAP_PCI_IOADDR (devc->osdev, 0, pci_ioaddr);
  /* Remove I/O space marker in bit 0. */
  devc->base &= ~3;

  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_IO;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  /* set the PCI latency to 32 */
  if ((apci_latency == 32) || (apci_latency == 64) || (apci_latency == 96) ||
      (apci_latency == 128))
    pci_write_config_byte (osdev, 0x0d, apci_latency);



  oss_register_device (osdev, devc->chip_name);

  if ((err = oss_register_interrupts (osdev, 0, apci97intr, NULL)) < 0)
    {
      cmn_err (CE_WARN, "Can't allocate IRQ%d, err=%d\n", pci_irq_line, err);
      return 0;
    }


  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);


  devc->revision = pci_revision;
  return init_apci97 (devc, device);	/* Detected */
}

int
oss_sbpci_detach (oss_device_t * osdev)
{
  apci97_devc *devc = (apci97_devc *) osdev->devc;
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
