/*
 * Purpose: Driver for Trident 4DWAVE, ALI 5451 and SiS 7918 audio chips
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

#include "oss_trident_cfg.h"
#include "oss_pci.h"
#include "ac97.h"

#if defined(sparc)
#define MEM_MAPPED_REGISTERS
#else
#undef MEM_MAPPED_REGISTERS
#endif

#define TRIDENT_VENDOR_ID	0x1023
#define TRIDENT_4DWAVEDX_ID	0x2000
#define TRIDENT_4DWAVENX_ID	0x2001
#define TRIDENT_4DWAVECX_ID	0x2002

#define ALI_VENDOR_ID		0x10b9
#define ALI_5451_ID		0x5451

#define SIS_VENDOR_ID		0x1039
#define SIS_7018_ID		0x7018

/* DX/NX/CX IO Registers */
#define SP_CSO      0x24
#define SP_LBA      0x28
#define SP_ESO      0x2C
#define ACR0        0x40
#define SP_STAT     0x64
#define TLBC        0x6C
#define START_A     0x80
#define STOP_A      0x84
#define INT_A       0x98
#define INTEN_A     0xA4
#define VOL 	    0xA8
#define DELTA       0xAC
#define MISCINT	    0xB0
#define START_B	    0xB4
#define STOP_B      0xB8
#define INT_B       0xD8
#define INTEN_B     0xDC
#define CIR         0xA0
#define CSO	    0xE0
#define LBA         0xE4
#define ESO	    0xE8
#define FMC         0xEC
#define TCTRL       0xF0
#define EBUF1       0xF4
#define EBUF2       0xF8

#define DMAR0	    0x00
#define DMAR4	    0x04
#define DMAR6	    0x06
#define DMAR11	    0x0b
#define DMAR15	    0x0f
#define SBDELTA	    0xac
#define SBBL	    0xc0
#define SBCTL	    0xc4

/* SIS 7018 Define */
#define SECONDARY_ID 	0x00004000
#define PCMOUT 		0x00010000
#define SURROUT 	0x00020000
#define CENTEROUT 	0x00040000
#define LFEOUT 		0x00080000
#define LINE1OUT 	0x00100000
#define LINE2OUT 	0x00200000
#define GPIOOUT  	0x00400000
#define CHANNEL_PB  	0x0000
#define CHANNEL_SPC_PB 	0x4000
#define CHANNEL_REC 	0x8000
#define CHANNEL_REC_PB  0xc000
#define MODEM_LINE1 	0x0000
#define MODEM_LINE2 	0x0400
#define PCM_LR 		0x0800
#define HSET 		0x0c00
#define I2S_LR 		0x1000
#define CENTER_LFE 	0x1400
#define SURR_LR 	0x1800
#define SPDIF_LR 	0x1c00
#define MIC 		0x1400
#define MONO_LEFT 	0x0000
#define MONO_RIGHT 	0x0100
#define MONO_MIX  	0x0200
#define SRC_ENABLE 	0x0080

#define INTR(bank)  ((bank==1)?INT_A:INT_B)
#define INTEN(bank) ((bank==1)?INTEN_A:INTEN_B)
#define STOP(bank)  ((bank==1)?STOP_A:STOP_B)
#define START(bank) ((bank==1)?START_A:START_B)

/* ALI5451 definitions */
#define ALI_5451_V02 0x2
#define SIS_7018_V02 0x2


extern int trident_mpu_ioaddr;

#define MAX_PORTC 8

typedef struct trident_portc
{
  int speed, bits, channels;
  int open_mode;
  int audio_enabled;
  int trigger_bits;
  int audiodev;
  int port_type;
#define DF_PCM   0
#define DF_SPDIF 1
  unsigned char play_chan, play_intr_chan;
  unsigned char rec_chan, rec_intr_chan;
  int pbank, rbank;
#define BANK_A	1
#define BANK_B	0
} trident_portc;


typedef struct trident_devc
{
  oss_device_t *osdev;
  char *chip_name;
  int chip_type;
  int revision;
  int irq;

#ifdef MEM_MAPPED_REGISTERS
  unsigned int bar1addr;
  char *bar1virt;
#else				/*  */
  oss_native_word base;
#endif				/*  */
  volatile unsigned char intr_mask;
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;

  /* Legacy */
  int mpu_base, mpu_irq, mpu_attached;

  /* Mixer parameters */
  ac97_devc ac97devc;
  int mixer_dev;

  /* Audio parameters */
  trident_portc portc[MAX_PORTC];
  int audio_opened;
  unsigned char sb_dma_flags;
} trident_devc;

#ifndef MEM_MAPPED_REGISTERS
/* I/O mapped register access */
#define READL(o,a)	INL(o, devc->base+(a))
#define READW(o,a)	INW(o, devc->base+(a))
#define READB(o,a)	INB(o, devc->base+(a))
#define WRITEL(o,d,a)	OUTL(o, d, devc->base+(a))
#define WRITEW(o,d,a)	OUTW(o, d, devc->base+(a))
#define WRITEB(o,d,a)	OUTB(o, d, devc->base+(a))
#else
/* Mem mapped I/O registers */
#define READL(o,a)	*(volatile unsigned int*)(devc->bar1virt+(a))
#define READW(o,a)	*(volatile unsigned short*)(devc->bar1virt+(a))
#define READB(o,a)	*(volatile unsigned char*)(devc->bar1virt+(a))
#define WRITEL(o,d,a)	*(volatile unsigned int*)(devc->bar1virt+(a))=d
#define WRITEW(o,d,a)	*(volatile unsigned short*)(devc->bar1virt+(a))=d
#define WRITEB(o,d,a)	*(volatile unsigned char*)(devc->bar1virt+(a))=d
#endif

static int
ac97_read (void *devc_, int addr)
{
  trident_devc *devc = devc_;
  int t, ret;
  oss_native_word data, reg = 0;
  oss_native_word rmask = 0;
  oss_native_word dmask = 0;
  oss_native_word wport = 0;
  oss_native_word rport = 0;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);

  t = 20000;
  switch (devc->chip_type)
    {
    case TRIDENT_4DWAVEDX_ID:
    case SIS_7018_ID:
      wport = 0x40;
      rport = 0x44;
      if (devc->revision == SIS_7018_V02)
	{
	  rport = 0x40;
	}
      rmask = 0x00008000;
      dmask = 0x00008000;
      break;
    case TRIDENT_4DWAVENX_ID:
      wport = 0x44;
      rport = 0x48;
      rmask = 0x00000800;
      dmask = 0x00000400;
      break;
    case ALI_5451_ID:
      wport = 0x40;
      rport = 0x44;
      rmask = 0x00008000;
      dmask = 0x00008000;
      if (devc->revision == ALI_5451_V02)
	{
	  rport = 0x40;
	}
      break;
    default:
      {
	static int already_done = 0;
	if (!already_done)
	  cmn_err (CE_WARN, "Unknown codec interface\n");
	already_done = 1;
	MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
	return OSS_EIO;
      }
    }

  /* Check to make sure No Writes are pending */
  reg = READL (devc->osdev, wport);
  while ((reg & dmask) && --t)
    reg = READL (devc->osdev, wport);	/* re-read status/data */

  /* Check to make sure No reads are pending */
  reg = READL (devc->osdev, rport);
  while ((reg & dmask) && --t)
    reg = READL (devc->osdev, rport);	/* re-read status/data */
  data = addr | rmask;
  WRITEL (devc->osdev, data, rport);	/* select register for reading */
  reg = READL (devc->osdev, rport);	/* read status/data */

  t = 20000;

  while ((reg & dmask) && --t)
    {				/* busy reading */
      reg = READL (devc->osdev, rport);	/* re-read status/data */
    }
  if (t)
    ret = (reg >> 16);
  else
    {
      DDB (cmn_err (CE_WARN, "AC97 mixer read timed out\n"));
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return OSS_EIO;
    }
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return ret;
}

static int
ac97_write (void *devc_, int addr, int data)
{
  trident_devc *devc = devc_;
  int t;
  oss_native_word wport = 0;
  oss_native_word reg = 0;
  unsigned int wmask = 0;
  unsigned int dmask = 0;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);

  t = 2000;

  switch (devc->chip_type)
    {
    case TRIDENT_4DWAVEDX_ID:
    case SIS_7018_ID:
      wport = 0x40;
      wmask = 0x00008000;
      dmask = 0x00008000;
      break;
    case TRIDENT_4DWAVENX_ID:
      wport = 0x44;
      wmask = 0x00000800;
      dmask = 0x00000800;
      break;
    case ALI_5451_ID:
      wport = 0x40;
      wmask = 0x00008000;
      dmask = 0x00008000;
      if (devc->revision == 2)
	dmask = 0x100;
      break;
    default:
      {
	static int already_done = 0;
	if (!already_done)
	  cmn_err (CE_WARN, "Unknown codec interface\n");
	already_done = 1;
	MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
	return OSS_EIO;
      }
    }

  /* Check to make sure No Writes are pending */
  reg = READL (devc->osdev, wport);
  while ((reg & wmask) && --t)
    {
      reg = READL (devc->osdev, wport);	/* re-read status/data */
    }
  if (t)
    {
      reg = data << 16;
      reg |= (addr & 0xff);
      reg |= wmask | dmask;
      WRITEL (devc->osdev, reg, wport);
    }
  else
    DDB (cmn_err (CE_WARN, "AC97 mixer write timed out\n"));
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return 0;
}

static int
tridentintr (oss_device_t * osdev)
{
  trident_devc *devc = (trident_devc *) osdev->devc;
  int i;
  oss_native_word status, intstat;
  int serviced = 0;
  dmap_t *dmapin;
  oss_native_word flag;


  /* check the interrupt status register MISCINT */
  status = READL (devc->osdev, MISCINT);
  serviced = 1;

  /* If it is a not 4DWave Playback  or MIDI interrupt then return */
  if (!(status & (0x00000020 | 0x00000008 | 0x4)))
    {
      WRITEL (devc->osdev, status, MISCINT);
      return 1;
    }

#ifdef OBSOLETED_STUFF
/*
 * This device has "ISA style" MIDI and FM subsystems. Such devices don't
 * use PCI config space for the I/O ports and interrupts. Instead the driver
 * needs to allocate proper resources itself. This functionality is no longer
 * possible. For this reason the MIDI and FM parts are not accessible.
 */
  /* Check if this is a MIDI interrupt */
  if (status & 0x00000008)
    {
      uart401intr (INT_HANDLER_CALL (devc->irq));
      serviced = 1;
    }
#endif
  MUTEX_ENTER_IRQDISABLE (devc->mutex, flag);

  /* Check to see if this is our channel */
  if (status & 0x20)
    for (i = 0; i < MAX_PORTC; i++)
      {
	trident_portc *portc = &devc->portc[i];
	serviced = 1;

	/* Handle Playback Interrupts */
	intstat = READL (devc->osdev, INTR (portc->pbank));

	if (intstat & 1L << portc->play_intr_chan)
	  {
	    WRITEL (devc->osdev, READL (devc->osdev, INTR (portc->pbank))
		    | 1L << portc->play_intr_chan, INTR (portc->pbank));
	    oss_audio_outputintr (portc->audiodev, 1);
	  }

	/* Handle Record Interrupts */
	intstat = READL (devc->osdev, INTR (portc->rbank));

	if (intstat & 1L << portc->rec_intr_chan)
	  {
	    WRITEL (devc->osdev, READL (devc->osdev, INTR (portc->rbank))
		    | 1L << portc->rec_intr_chan, INTR (portc->rbank));
	    if ((devc->chip_type == ALI_5451_ID)
		|| (devc->chip_type == SIS_7018_ID))
	      {
		unsigned int ptr;
		int i;
		int chan;
		dmapin = audio_engines[portc->audiodev]->dmap_in;

		if (portc->rbank == BANK_A)
		  chan = portc->rec_chan;

		else
		  chan = portc->rec_chan + 32;
		WRITEL (devc->osdev,
			(READL (devc->osdev, CIR) & ~0x3F) | chan, CIR);
		ptr = READW (devc->osdev, CSO + 2);
		ptr++;
		ptr *= portc->channels * (portc->bits / 8);

		if (dmapin->bytes_in_use == 0 || dmapin->fragment_size == 0)
		   {
			   cmn_err(CE_WARN, "bytes_in_use=%d, fragment_size=%d\n", dmapin->bytes_in_use, dmapin->fragment_size);
			   continue;
		   }
		ptr %= dmapin->bytes_in_use;
		ptr /= dmapin->fragment_size;
		i = 0;
		while (dmap_get_qtail (dmapin) != ptr && i++ < dmapin->nfrags)
		  oss_audio_inputintr (portc->audiodev, 0);
	      }
	    else
	      oss_audio_inputintr (portc->audiodev, 0);
	  }
      }

  if (status & 0x4)
    {
      serviced = 1;
      READB (devc->osdev, 0x1E);	/*SB ESP ack */
      READB (devc->osdev, 0x1F);	/*SB IRQ ack */
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flag);
  return serviced;
}


/***************************************************************************/
static int
trident_audio_set_rate (int dev, int arg)
{
  trident_portc *portc = audio_engines[dev]->portc;
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
trident_audio_set_channels (int dev, short arg)
{
  trident_portc *portc = audio_engines[dev]->portc;
  if ((arg != 1) && (arg != 2))
    return portc->channels;
  portc->channels = arg;
  return portc->channels;
}

static unsigned int
trident_audio_set_format (int dev, unsigned int arg)
{
  trident_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->bits;
  if (!(arg & (AFMT_U8 | AFMT_S16_LE | AFMT_AC3)))
    return portc->bits;
  portc->bits = arg;
  return portc->bits;
}

/*ARGSUSED*/
static int
trident_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void trident_audio_trigger (int dev, int state);

static void
trident_audio_reset (int dev)
{
  trident_audio_trigger (dev, 0);
}

static void
trident_audio_reset_input (int dev)
{
  trident_portc *portc = audio_engines[dev]->portc;
  trident_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
trident_audio_reset_output (int dev)
{
  trident_portc *portc = audio_engines[dev]->portc;
  trident_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

/*ARGSUSED*/
static int
trident_audio_open (int dev, int mode, int open_flags)
{
  trident_portc *portc = audio_engines[dev]->portc;
  trident_devc *devc = audio_engines[dev]->devc;
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
trident_audio_close (int dev, int mode)
{
  trident_portc *portc = audio_engines[dev]->portc;
  trident_audio_reset (dev);
  portc->open_mode = 0;
  portc->audio_enabled &= ~mode;
}

/*ARGSUSED*/
static void
trident_audio_output_block (int dev, oss_native_word buf, int count,
			    int fragsize, int intrflag)
{
  trident_portc *portc = audio_engines[dev]->portc;
  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
}

/*ARGSUSED*/
static void
trident_audio_start_input (int dev, oss_native_word buf, int count,
			   int fragsize, int intrflag)
{
  trident_portc *portc = audio_engines[dev]->portc;
  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
}

static void
trident_audio_trigger (int dev, int state)
{
  trident_devc *devc = audio_engines[dev]->devc;
  trident_portc *portc = audio_engines[dev]->portc;
  oss_native_word ainten = 0, playstop = 0, recstop = 0;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode & OPEN_WRITE)
    {
      if (state & PCM_ENABLE_OUTPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT)
	      && !(portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {

	      /* Enable the channel interrupt */
	      WRITEL (devc->osdev, READL (devc->osdev, INTEN (portc->pbank))
		      | (1L << portc->play_intr_chan), INTEN (portc->pbank));

	      /* start playback and playback interrupt channels */
	      WRITEL (devc->osdev, READL (devc->osdev, START (portc->pbank))
		      | (1L << portc->play_chan)
		      | (1L << portc->play_intr_chan), START (portc->pbank));
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

	      /* disable playback interrupt channel */
	      ainten = READL (devc->osdev, INTEN (portc->pbank));
	      ainten = ainten & ~(1L << portc->play_intr_chan);
	      WRITEL (devc->osdev, ainten, INTEN (portc->pbank));

	      /* stop playback and playback interrupt channels */
	      playstop = ((1L << portc->play_chan) |
			  (1L << portc->play_intr_chan));
	      WRITEL (devc->osdev, playstop, STOP (portc->pbank));
	    }
	}
    }

  if (portc->open_mode & OPEN_READ)
    {
      if (state & PCM_ENABLE_INPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_INPUT)
	      && !(portc->trigger_bits & PCM_ENABLE_INPUT))
	    {
	      /* Enable the channel interrupts */
	      WRITEL (devc->osdev, READL (devc->osdev, INTEN (portc->rbank))
		      | (1L << portc->rec_intr_chan), INTEN (portc->rbank));

	      if ((devc->chip_type != ALI_5451_ID)
		  && (devc->chip_type != SIS_7018_ID))
		{
		  /* set recording flags */
		  WRITEB (devc->osdev, devc->sb_dma_flags, SBCTL);

		  /* start record interrupt channel */
		  WRITEL (devc->osdev, 1L << portc->rec_intr_chan,
			  START (portc->rbank));
		}
	      else
		{
		  WRITEL (devc->osdev,
			  READL (devc->osdev,
				 START (portc->rbank)) | (1L << portc->
							  rec_chan) | (1L <<
								       portc->
								       rec_intr_chan),
			  START (portc->rbank));
		}
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
	      if ((devc->chip_type != ALI_5451_ID)
		  && (devc->chip_type != SIS_7018_ID))
		{
		  /* disable the record interrrupt enable channel */
		  ainten = READL (devc->osdev, INTEN (portc->rbank));
		  ainten = ainten & ~(1L << portc->rec_intr_chan);
		  WRITEL (devc->osdev, ainten, INTEN (portc->rbank));

		  /* stop DMA controller */
		  WRITEB (devc->osdev, 0x00, SBCTL);

		  /* stop record interrupt channel */
		  recstop |= 1L << portc->rec_intr_chan;
		  WRITEL (devc->osdev, recstop, STOP (portc->rbank));
		}
	      else
		{
		  /* disable rec interrupt channel */
		  ainten = READL (devc->osdev, INTEN (portc->rbank));
		  ainten = ainten & ~(1L << portc->rec_intr_chan);
		  WRITEL (devc->osdev, ainten, INTEN (portc->rbank));

		  /* stop playback and playback interrupt channels */
		  recstop = ((1L << portc->rec_chan) |
			     (1L << portc->rec_intr_chan));
		  WRITEL (devc->osdev, recstop, STOP (portc->rbank));
		}

	      /* ack any interrupts on INT_A */
	      recstop = READL (devc->osdev, INTR (portc->rbank));
	      recstop |= 1L << portc->rec_intr_chan;
	      WRITEL (devc->osdev, recstop, INTR (portc->rbank));
	    }
	}
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
static int
trident_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  trident_devc *devc = audio_engines[dev]->devc;
  trident_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_in;
  oss_native_word bufsize, delta, eso;
  oss_native_word temp;
  unsigned int attribute = 0;
  unsigned char bValue;
  unsigned short wValue;
  oss_native_word dwValue;
  unsigned short wRecCodecSamples;
  int chan;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (devc->chip_type == TRIDENT_4DWAVEDX_ID)
    {
      bValue = READB (devc->osdev, 0x48);	/*enable AC97 ADC */
      WRITEB (devc->osdev, bValue | 0x48, 0x48);	/*disable rec intr */
    }

  if (devc->chip_type == TRIDENT_4DWAVENX_ID)
    {
      wValue = READW (devc->osdev, MISCINT);
      WRITEW (devc->osdev, wValue | 0x1000, MISCINT);
    }

  if ((devc->chip_type != ALI_5451_ID) && (devc->chip_type != SIS_7018_ID))
    {
      /* Initialize legacy recording */
      WRITEB (devc->osdev, 0, DMAR15);
      bValue = READB (devc->osdev, DMAR11) & 0x03;
      WRITEB (devc->osdev, bValue | 0x54, DMAR11);

      /* Set base address in DMAR0-DMAR3 */
      WRITEL (devc->osdev, dmap->dmabuf_phys, DMAR0);

      /* Set count in DMAR4/DMAR5 */
      eso = dmap->bytes_in_use;
      WRITEW (devc->osdev, eso - 1, DMAR4);

      /* Set speed in SBDelta */
      dwValue = (48000 << 12) / portc->speed;
      WRITEW (devc->osdev, (unsigned short) dwValue, SBDELTA);

      /*Set channel interrupt blk length */
      if ((portc->bits == 16) || (portc->channels == 2))
	wRecCodecSamples = (unsigned short) ((eso >> 1) - 1);
      else
	wRecCodecSamples = (unsigned short) (eso - 1);

      dwValue = wRecCodecSamples << 16;
      dwValue |= wRecCodecSamples & 0x0000ffff;
      WRITEL (devc->osdev, dwValue, SBBL);

      /*set format */
      devc->sb_dma_flags |= 0x19;
      if (portc->bits == 16)
	devc->sb_dma_flags |= 0xA0;
      if (portc->channels == 2)
	devc->sb_dma_flags |= 0x40;
    }
  else
    {
#if !defined(sparc)
      if (devc->chip_type == ALI_5451_ID)
	WRITEL (devc->osdev,
		READL (devc->osdev, 0xd4) | ((unsigned int) 1 << 31), 0xd4);
#endif

      /* for SiS 7018: Rec with PCM_LR, MONO_MIX, SRC_EN */
      if (devc->chip_type == SIS_7018_ID)
	attribute = 0x8880;	/* PCM->IN */

      delta = (48000 << 12) / portc->speed;
      bufsize = dmap->bytes_in_use;

      if (portc->bits == 16)
	bufsize /= 2;
      if (portc->channels == 2)
	bufsize /= 2;
      bufsize--;

      if (portc->rbank == BANK_A)
	chan = portc->rec_chan;
      else
	chan = portc->rec_chan + 32;

      WRITEL (devc->osdev, (READL (devc->osdev, CIR) & ~0x3F) | chan, CIR);	/*select current chan */

      /* Now set the buffer address pointer */
      WRITEL (devc->osdev, dmap->dmabuf_phys, LBA);

      /* Set the Size and Sampling Rate */
      eso = (bufsize << 16) | (delta & 0x0000FFFF);
      WRITEL (devc->osdev, eso, ESO);
      temp = 0x80000000;	/*enable gvsel */
      if (portc->channels == 2)
	temp |= 0x00004000;	/*stereo/mono */
      if (portc->bits == 16)
	temp |= 0x0000A000;	/*unsigned 8/signed 16bit */
      temp |= 0x00001000;	/*enable loop */
      temp |= 0x003F0000;	/*set pan to off */
      temp |= 0x00000FFF;	/*set vol to off */
      WRITEL (devc->osdev, temp, TCTRL);	/*set format */
      WRITEL (devc->osdev, attribute << 16, FMC);
      WRITEL (devc->osdev, 0, EBUF1);
      WRITEL (devc->osdev, 0, EBUF2);
      /* WRITEL (devc->osdev, 0xFFFF, VOL); *//*Music/WaveVol set to max */
      WRITEL (devc->osdev, 0, CSO);	/*set CSO and Alpha to 0 */
    }

/* Now prepare the Record Interrupt Channel */
  delta = (48000 << 12) / portc->speed;
  bufsize = dmap->fragment_size;
  if (portc->bits == 16)
    bufsize /= 2;
  if (portc->channels == 2)
    bufsize /= 2;
  bufsize--;

  if (portc->rbank == BANK_A)
    chan = portc->rec_intr_chan;
  else
    chan = portc->rec_intr_chan + 32;

  WRITEL (devc->osdev, (READL (devc->osdev, CIR) & ~0x3F) | chan, CIR);

  /* Now set the buffer address pointer */
  WRITEL (devc->osdev, dmap->dmabuf_phys, LBA);

  /* Set the Size and Sampling Rate */
  if ((devc->chip_type == TRIDENT_4DWAVEDX_ID)
      || (devc->chip_type == SIS_7018_ID) || (devc->chip_type == ALI_5451_ID))
    {
      eso = (bufsize << 16) | (delta & 0x0000FFFF);
      WRITEL (devc->osdev, eso, ESO);
    }
  else
    {
      eso = ((delta << 16) & 0xff000000) | (bufsize & 0x00ffffff);
      WRITEL (devc->osdev, eso, ESO);
    }

  temp = 0x80000000;		/*enable gvsel */
  if (portc->channels == 2)
    temp |= 0x00004000;		/*stereo/mono */
  if (portc->bits == 16)
    temp |= 0x0000A000;		/*8 unsigned/16bit signed data */
  temp |= 0x00001000;		/*enable loop */
  temp |= 0x003F0000;		/*set pan to off */
  temp |= 0x00000FFF;		/*set vol to off */
  WRITEL (devc->osdev, temp, TCTRL);	/*set format */

  /* for the record interrupt channel note that no attribute should be
   * set for SIS7018 
   */
  WRITEL (devc->osdev, 0, FMC);
  WRITEL (devc->osdev, 0, EBUF1);
  WRITEL (devc->osdev, 0, EBUF2);
  /* WRITEL (devc->osdev, 0xFFFF, VOL); *//*Music/WaveVol set to min */
  if ((devc->chip_type == TRIDENT_4DWAVEDX_ID)
      || (devc->chip_type == SIS_7018_ID) || (devc->chip_type == ALI_5451_ID))
    {
      WRITEL (devc->osdev, 0, CSO);	/*set CSO and Alpha to 0 */
    }
  else
    {
      WRITEL (devc->osdev, (delta << 24) | (0 & 0x00ffffff), CSO);
    }

  /* Now set the mode on portc to record */
  portc->audio_enabled &= ~PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/
static int
trident_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  trident_devc *devc = audio_engines[dev]->devc;
  trident_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_out;
  oss_native_word bufsize, delta, eso;
  oss_native_word temp;
  int chan, spdif_rate, spdif_chan;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  /* prepare the main playback channel */
  delta = (portc->speed * 4096) / 48000;
  bufsize = dmap->bytes_in_use;

  if (portc->bits == 16)
    bufsize /= 2;
  if (portc->channels == 2)
    bufsize /= 2;
  bufsize--;

  if ((devc->chip_type == ALI_5451_ID) && (devc->revision == ALI_5451_V02)
      && (portc->port_type == DF_SPDIF))

    {
      ac97_spdifout_ctl (devc->mixer_dev, SPDIFOUT_AUDIO, SNDCTL_MIX_WRITE,
			 0);
      portc->play_chan = 15;
      portc->play_intr_chan = 14;

      if (portc->bits == AFMT_AC3)

	{
	  ac97_spdif_setup (devc->mixer_dev, portc->speed, portc->bits);
	  portc->bits = 16;
	  portc->channels = 2;
	  portc->speed = 48000;
	  /* set non-pcm (AC3) mode on SPDIF control */
	  WRITEW (devc->osdev, (READW (devc->osdev, 0x70) & ~0x2) | 0x2,
		  0x70);
	}
      else
	/* set pcm mode on SPDIF control */
	WRITEW (devc->osdev, READW (devc->osdev, 0x70) & ~0x2, 0x70);

      /*set up the sampling rate */
      switch (portc->speed)
	{
	case 44100:
	  spdif_rate = 0;
	  break;
	case 32000:
	  spdif_rate = 0x300;
	  break;
	case 48000:
	default:
	  spdif_rate = 0x200;
	  break;
	}
      spdif_chan = READB (devc->osdev, 0x74) & 0xbf;	/*select spdif_out */
      spdif_chan |= 0x80;	/*select right */
      WRITEB (devc->osdev, spdif_chan, 0x74);
      WRITEB (devc->osdev, spdif_rate | 0x20, 0x72);
      spdif_chan &= (~0x80);	/*select left */
      WRITEB (devc->osdev, spdif_chan, 0x74);
      WRITEB (devc->osdev, spdif_rate | 0x10, 0x72);
    }

  if (portc->pbank == BANK_A)
    chan = portc->play_chan;
  else
    chan = portc->play_chan + 32;

  WRITEL (devc->osdev, (READL (devc->osdev, CIR) & ~0x3F) | chan, CIR);	/*select current chan */

  /* Now set the buffer address pointer */
  WRITEL (devc->osdev, dmap->dmabuf_phys, LBA);

  /* Set the Size and Sampling Rate */
  if ((devc->chip_type == TRIDENT_4DWAVEDX_ID)
      || (devc->chip_type == SIS_7018_ID) || (devc->chip_type == ALI_5451_ID))
    {
      eso = (bufsize << 16) | (delta & 0x0000FFFF);
      WRITEL (devc->osdev, eso, ESO);
    }
  else
    {
      eso = ((delta << 16) & 0xff000000) | (bufsize & 0x00ffffff);
      WRITEL (devc->osdev, eso, ESO);
    }
  temp = 0x80000000;		/*enable gvsel */
  if (portc->channels == 2)
    temp |= 0x00004000;		/*stereo/mono */
  if (portc->bits == 16)
    temp |= 0x0000A000;		/*8 unsigned/16bit signed data */
  temp |= 0x00001000;		/*enable loop */
  WRITEL (devc->osdev, temp, TCTRL);	/*set format */
  WRITEL (devc->osdev, 0, FMC);
  WRITEL (devc->osdev, 0, EBUF1);
  WRITEL (devc->osdev, 0, EBUF2);
  WRITEL (devc->osdev, 0, VOL);	/*Music/WaveVol set to max */
  if ((devc->chip_type == TRIDENT_4DWAVEDX_ID)
      || (devc->chip_type == SIS_7018_ID) || (devc->chip_type == ALI_5451_ID))
    {
      WRITEL (devc->osdev, 0, CSO);	/*set CSO and Alpha to 0 */
    }
  else
    {
      WRITEL (devc->osdev, (delta << 24) | (0 & 0x00ffffff), CSO);
    }

  /* Now prepare the playback interrupt channel */
  delta = (portc->speed * 4096) / 48000;
  bufsize = dmap->fragment_size;
  if (portc->bits == 16)
    bufsize /= 2;
  if (portc->channels == 2)
    bufsize /= 2;
  bufsize--;

  if (portc->pbank == BANK_A)
    chan = portc->play_intr_chan;
  else
    chan = portc->play_intr_chan + 32;

  WRITEL (devc->osdev, (READL (devc->osdev, CIR) & ~0x3F) | chan, CIR);	/*select current chan */

  /* Now set the buffer address pointer */
  WRITEL (devc->osdev, dmap->dmabuf_phys, LBA);

  /* Set the Size and Sampling Rate */
  if ((devc->chip_type == TRIDENT_4DWAVEDX_ID)
      || (devc->chip_type == SIS_7018_ID) || (devc->chip_type == ALI_5451_ID))
    {
      eso = (bufsize << 16) | (delta & 0x0000FFFF);
      WRITEL (devc->osdev, eso, ESO);
    }
  else
    {
      eso = ((delta << 16) & 0xff000000) | (bufsize & 0x00ffffff);
      WRITEL (devc->osdev, eso, ESO);
    }

  temp = 0x80000000;		/*enable gvsel */
  if (portc->channels == 2)
    temp |= 0x00004000;		/*stereo/mono */
  if (portc->bits == 16)
    temp |= 0x0000A000;		/*8 unsigned/16bit signed data */
  temp |= 0x00001000;		/*enable loop */
  temp |= 0xFFF;		/* no vol */
  temp |= 0x003F0000;		/*set volume to off */
  WRITEL (devc->osdev, temp, TCTRL);	/*set format */
  WRITEL (devc->osdev, 0, FMC);
  WRITEL (devc->osdev, 0, EBUF1);
  WRITEL (devc->osdev, 0, EBUF2);
  WRITEL (devc->osdev, 0, VOL);	/*Music/WaveVol set to max */
  if ((devc->chip_type == TRIDENT_4DWAVEDX_ID)
      || (devc->chip_type == SIS_7018_ID) || (devc->chip_type == ALI_5451_ID))
    {
      WRITEL (devc->osdev, 0, CSO);	/*set CSO and Alpha to 0 */
    }
  else
    {
      WRITEL (devc->osdev, (delta << 24) | (0 & 0x00ffffff), CSO);
    }

  /* Now set the mode on portc to playback */
  portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

#if 0
static int
trident_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  trident_devc *devc = audio_engines[dev]->devc;
  trident_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;
  int ptr = 0;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (direction == PCM_ENABLE_INPUT)
    {
      WRITEB (devc->osdev,
	      (READB (devc->osdev, CIR) & ~0x7F) | portc->rec_chan, CIR);
      if ((devc->chip_type == TRIDENT_4DWAVEDX_ID)
	  || (devc->chip_type == SIS_7018_ID)
	  || (devc->chip_type == ALI_5451_ID))
	ptr = READW (devc->osdev, CSO + 2);
      else			/* ID_4DWAVE_NX */
	{
	  ptr = READW (devc->osdev, SBBL) & 0xFFFF;
	  if (portc->channels > 1)
	    ptr >>= 1;
	  if (ptr > 0)
	    ptr = dmap->bytes_in_use - ptr;
	}
    }

  if (direction == PCM_ENABLE_OUTPUT)
    {
      WRITEB (devc->osdev,
	      (READB (devc->osdev, CIR) & ~0x7F) | portc->play_chan, CIR);
      if ((devc->chip_type == TRIDENT_4DWAVEDX_ID)
	  || (devc->chip_type == SIS_7018_ID)
	  || (devc->chip_type == ALI_5451_ID))
	{
	  ptr = READW (devc->osdev, CSO + 2);
	}
      else
	{
	  ptr = READL (devc->osdev, CSO) & 0x00ffffff;
	}
      if (ptr > dmap->bytes_in_use)
	ptr = 0;
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return ptr;
}
#endif

#if !defined(sparc)
static int
trident_alloc_buffer (int dev, dmap_t * dmap, int direction)
{
  int err;

  if (dmap->dmabuf != NULL)
    return 0;

  if ((err = oss_alloc_dmabuf (dev, dmap, direction)) < 0)
    {
      cmn_err (CE_WARN, "Failed to allocate a DMA buffer.\n");
      return err;
    }
  if (dmap->dmabuf_phys & 0x80000000)

    {
      cmn_err (CE_WARN, "Got DMA buffer address beyond 2G limit.\n");
      oss_free_dmabuf (dev, dmap);
      dmap->dmabuf = NULL;
      return OSS_ENOSPC;
    }
  return 0;
}
#endif

static audiodrv_t trident_audio_driver = {
  trident_audio_open,
  trident_audio_close,
  trident_audio_output_block,
  trident_audio_start_input,
  trident_audio_ioctl,
  trident_audio_prepare_for_input,
  trident_audio_prepare_for_output,
  trident_audio_reset,
  NULL,
  NULL,
  trident_audio_reset_input,
  trident_audio_reset_output,
  trident_audio_trigger,
  trident_audio_set_rate,
  trident_audio_set_format,
  trident_audio_set_channels,
  NULL,
  NULL,
  NULL,				/* trident_check_input, */
  NULL,				/* trident_check_output, */
#if !defined(sparc)
  trident_alloc_buffer,
#else
  NULL,
#endif
  NULL,				/* trident_free_buffer, */
  NULL,
  NULL,
  NULL				/* trident_get_buffer_pointer */
};


#ifdef OBSOLETED_STUFF
static void
attach_mpu (trident_devc * devc)
{
  struct address_info hw_config;
  hw_config.io_base = devc->mpu_base;
  hw_config.irq = -devc->irq;
  hw_config.dma = -1;
  hw_config.dma2 = -1;
  hw_config.always_detect = 0;
  hw_config.name = devc->chip_name;
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
unload_mpu (trident_devc * devc)
{
  struct address_info hw_config;
  hw_config.io_base = devc->mpu_base;
  hw_config.irq = devc->mpu_irq;
  hw_config.dma = -1;
  hw_config.dma2 = -1;
  hw_config.always_detect = 0;
  hw_config.name = devc->chip_name;
  hw_config.driver_use_1 = 0;
  hw_config.driver_use_2 = 0;
  hw_config.osdev = devc->osdev;
#ifdef CREATE_OSP
  CREATE_OSP (hw_config.osdev);
#endif /*  */
  hw_config.card_subtype = 0;
  devc->mpu_attached = 0;
  unload_uart401 (&hw_config);
}
#endif

static int
init_trident (trident_devc * devc)
{
  int my_mixer, i;
  oss_device_t *osdev = devc->osdev;
  unsigned char legacy;
  oss_native_word global_control;
  int adev;
  unsigned int dwVal;
  int first_dev = 0;

/* 
 * Legacy I/O setup
 */
  devc->mpu_attached = 0;
  legacy = 0xA0;		/* Enable MPU, GP, FM */
  switch (devc->mpu_base)
    {
    case 0x330:
      legacy |= 0x00;
      break;
    case 0x300:
      legacy |= 0x40;
      break;
    default:
      devc->mpu_base = 0;
    }
  pci_write_config_byte (osdev, 0x44, legacy);	/*setup legacy devs */

  /* pci_write_config_byte (osdev, 0x45, 0x12); *//*setup DDMA */
  /* pci_write_config_byte (osdev, 0x46, 0x02); *//*setup device r/w */

  /* Now reset AC97 and unmute the codec channels */
  if (devc->chip_type == TRIDENT_4DWAVEDX_ID)
    {
      WRITEB (devc->osdev, 0x09, 0x40);	/*set ddma */
      WRITEL (devc->osdev, 0x2, 0x48);	/*enable the AC97 */
    }

  if (devc->chip_type == SIS_7018_ID)
    {
      WRITEL (devc->osdev, 0, 0x4c);
      WRITEL (devc->osdev, 0xF0000, 0x48);	/* enable ac97 link */
    }

  if (devc->chip_type == TRIDENT_4DWAVENX_ID)
    {
      WRITEL (devc->osdev, 0x2, 0x40);
      /* Setup 48KHz S/PDIF output on 4DWave NX */
      WRITEL (devc->osdev, INL (devc->osdev, SP_STAT) | 4, SP_STAT);
      WRITEB (devc->osdev, 0x38, SP_CSO + 3);
    }

  if (devc->chip_type == ALI_5451_ID)
    {
#ifndef sparc
      pci_read_config_dword (osdev, 0x7c, &dwVal);
      pci_write_config_dword (osdev, 0x7c, dwVal | 0x08000000);
      oss_udelay (5000);
      pci_read_config_dword (osdev, 0x7c, &dwVal);
      pci_write_config_dword (osdev, 0x7c, dwVal & 0xf7ffffff);
      oss_udelay (5000);
      pci_read_config_dword (osdev, 0x44, &dwVal);
      pci_write_config_dword (osdev, 0x44, dwVal | 0x000c0000);
      oss_udelay (500);
      pci_read_config_dword (osdev, 0x44, &dwVal);
      pci_write_config_dword (osdev, 0x44, dwVal & 0xfffbffff);
#endif
      /* enable full 32bit and disable DDMA */
      pci_write_config_dword (osdev, 0x40, dwVal & 0x00000008);
      oss_udelay (5000);
    }

  my_mixer = ac97_install (&devc->ac97devc, devc->chip_name, ac97_read,
			   ac97_write, devc, devc->osdev);
  if (my_mixer >= 0)
    {
      devc->mixer_dev = my_mixer;
      mixer_devs[my_mixer]->priority = 9;
    }
  else
    return 0;

#ifdef OBSOLETED_STUFF
  if (devc->mpu_base > 0)
    attach_mpu (devc);
#endif

  for (i = 0; i < 2; i++)
    {
      char tmp_name[1024];
      trident_portc *portc = &devc->portc[i];
      int caps = ADEV_AUTOMODE /* | ADEV_HWMIX */ ;
      int porttype = DF_PCM;

      sprintf (tmp_name, "%s (rev %d)", devc->chip_name, devc->revision);
      if (i == 0)
	{
	  caps |= ADEV_DUPLEX;
	}
      else
	{
	  caps |= ADEV_NOINPUT;
	  if (i > 1)
	    caps |= ADEV_SHADOW;
	}

#if 0
      /* This will not work any more */
      if ((devc->chip_type == ALI_5451_ID)
	  && (devc->revision == ALI_5451_V02) && (i == 7))
	{
	  /* need to set SPDIF output in PCI southbridge chip 0x1533 */
	  while ((osdev = (pci_find_class (0x601 << 8, osdev))))
	    {
	      unsigned short vendorid, deviceid;
	      unsigned char temp;
	      pci_read_config_word (osdev, PCI_VENDOR_ID, &vendorid);
	      pci_read_config_word (osdev, PCI_DEVICE_ID, &deviceid);
	      if (vendorid != 0x10b9 || deviceid != 0x1533)
		continue;
	      pci_read_config_byte (osdev, 0x61, &temp);
	      temp |= 0x40;
	      pci_write_config_byte (osdev, 0x61, temp);
	      pci_read_config_byte (osdev, 0x7d, &temp);
	      temp |= 0x01;
	      pci_write_config_byte (osdev, 0x7d, temp);
	      pci_read_config_byte (osdev, 0x7e, &temp);
	      temp &= (~0x20);
	      temp |= 0x10;
	      pci_write_config_byte (osdev, 0x7e, temp);

	      /* SPDIF Magic!!! */
	      pci_read_config_byte (osdev, 0x63, &temp);
	      temp |= 0x3;	/* enable SPDIF OUT bit0=out bit1=in */
	      pci_write_config_byte (osdev, 0x63, temp);

	      /* SPDIF Output in SERIAL Config reg */
	      temp = READL (devc->osdev, 0x48);
	      WRITEB (devc->osdev, temp | 0x20, 0x48);
	      temp = READL (devc->osdev, 0x74);
	      WRITEB (devc->osdev, temp & 0xbf, 0x74);

	      /* Enable SPDIF on play_channel 15 and setup as SPDIFOUT */
	      WRITEW (devc->osdev, READW (devc->osdev, 0xd4) | 0x8000, 0xd4);
	      WRITEW (devc->osdev, READW (devc->osdev, 0xd4) & ~0x0400, 0xd4);

	      porttype = DF_SPDIF;
	      caps |= ADEV_SPECIAL | ADEV_FIXEDRATE;
	      sprintf (tmp_name, "%s (S/PDIF Output)", devc->chip_name);
	    }
	}
#endif
      if ((adev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
					devc->osdev,
					devc->osdev,
					tmp_name,
					&trident_audio_driver,
					sizeof (audiodrv_t),
					caps,
					AFMT_U8 | AFMT_S16_LE | AFMT_AC3,
					devc, -1)) < 0)

	{
	  adev = -1;
	  return 0;
	}
      else
	{
	  if (i == 0)
	    first_dev = adev;
	  audio_engines[adev]->portc = portc;
	  audio_engines[adev]->mixer_dev = my_mixer;
	  audio_engines[adev]->rate_source = first_dev;
	  audio_engines[adev]->min_rate = 8000;
	  audio_engines[adev]->max_rate = 48000;
	  audio_engines[adev]->caps |= PCM_CAP_FREERATE;

#if !defined(sparc)
	  /*
	   * Only 31 bit memory addresses are supported except under Sparc
	   * where the on-board audio chip supports the top half of 32 bit PCI
	   * address space.
	   */
	  audio_engines[adev]->dmabuf_maxaddr = MEMLIMIT_31BITS;
#endif
	  portc->audiodev = adev;
	  portc->open_mode = 0;
	  portc->audio_enabled = 0;

	  if (audio_engines[adev]->flags & ADEV_FIXEDRATE)
	    {
	      audio_engines[adev]->fixed_rate = 48000;
	      audio_engines[adev]->min_rate = 48000;
	      audio_engines[adev]->max_rate = 48000;
	    }

	  portc->port_type = porttype;
	  portc->play_chan = 3 + (2 * i);
	  portc->play_intr_chan = 2 + (2 * i);
	  portc->pbank = BANK_A;
	  portc->rec_chan = 1;
	  portc->rec_intr_chan = 0;
	  portc->rbank = BANK_A;

	  if (devc->chip_type == ALI_5451_ID)
	    {
	      portc->rec_chan = 31;
	      portc->rec_intr_chan = 30;
	      portc->rbank = BANK_A;
	    }
	  if (devc->chip_type == SIS_7018_ID)
	    {
	      portc->pbank = BANK_B;
	      portc->rec_chan = 1;
	      portc->rec_intr_chan = 0;
	      portc->rbank = BANK_B;
	      WRITEL (devc->osdev, READL (devc->osdev, CIR) | 0x10000, CIR);
	    }
#ifdef CONFIG_OSS_VMIX
	  if (i == 0)
	     vmix_attach_audiodev(devc->osdev, adev, -1, 0);
#endif
	}

      devc->audio_opened = 0;
      /* set the global control  and enable end interrupt condition */
      global_control = READL (devc->osdev, CIR);
      global_control |= (1 << 12);	/* control end intr */
      WRITEL (devc->osdev, global_control, CIR);
    }
  return 1;
}

int
oss_trident_attach (oss_device_t * osdev)
{
  unsigned char pci_irq_line, pci_revision /*, pci_latency */ ;
  unsigned short pci_command, vendor, device;
  unsigned int pci_ioaddr;
  trident_devc *devc;

  DDB (cmn_err (CE_WARN, "Entered Trident 4DWAVE probe routine\n"));

  oss_pci_byteswap (osdev, 1);

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  if ((vendor != TRIDENT_VENDOR_ID && vendor != ALI_VENDOR_ID
       && vendor != SIS_VENDOR_ID) ||
      (device != TRIDENT_4DWAVEDX_ID && device != TRIDENT_4DWAVENX_ID &&
       device != TRIDENT_4DWAVECX_ID && device != ALI_5451_ID &&
       device != SIS_7018_ID))

    return 0;

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
  devc->revision = pci_revision;
  devc->irq = pci_irq_line;

  switch (device)
    {
    case TRIDENT_4DWAVEDX_ID:
      devc->chip_name = "Trident 4DWAVEDX";
      devc->chip_type = TRIDENT_4DWAVEDX_ID;
      break;
    case TRIDENT_4DWAVENX_ID:
      devc->chip_name = "Trident 4DWAVENX";
      devc->chip_type = TRIDENT_4DWAVENX_ID;
      break;
    case TRIDENT_4DWAVECX_ID:
      devc->chip_name = "Trident 4DWAVECX";
      devc->chip_type = TRIDENT_4DWAVECX_ID;
      break;
    case ALI_5451_ID:
      devc->chip_name = "ALI M5451";
      devc->chip_type = ALI_5451_ID;
      break;
    case SIS_7018_ID:
      devc->chip_name = "SiS 7018";
      devc->chip_type = SIS_7018_ID;
      break;
    default:
      devc->chip_name = "Trident 4DWAVE";
      devc->chip_type = TRIDENT_4DWAVEDX_ID;
    }

#ifdef MEM_MAPPED_REGISTERS
  pci_read_config_dword (osdev, PCI_MEM_BASE_ADDRESS_1, &devc->bar1addr);
  devc->bar1virt =
    (char *) MAP_PCI_MEM (devc->osdev, 1, devc->bar1addr, 1024 * 1024);
  pci_command |= PCI_COMMAND_MEMORY;
#else
  devc->base = MAP_PCI_IOADDR (devc->osdev, 0, pci_ioaddr);

  /* Remove I/O space marker in bit 0. */
  devc->base &= ~0x03;
  pci_command |= PCI_COMMAND_IO;
#endif
  devc->mpu_base = trident_mpu_ioaddr;
  devc->mpu_irq = devc->irq;

  /* activate the device */
  pci_command |= PCI_COMMAND_MASTER;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);

  oss_register_device (osdev, devc->chip_name);

  if (oss_register_interrupts (devc->osdev, 0, tridentintr, NULL) < 0)
    {
      cmn_err (CE_WARN, "Can't allocate IRQ%d\n", pci_irq_line);
      return 0;
    }

  return init_trident (devc);	/*Detected */
}



int
oss_trident_detach (oss_device_t * osdev)
{
  trident_devc *devc = (trident_devc *) osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  WRITEL (devc->osdev, 0, CIR);
  if (devc->chip_type == TRIDENT_4DWAVENX_ID)
    WRITEL (devc->osdev, 0x0, SP_CSO);	/*disable S/PDIF on NX */

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

#ifdef MEM_MAPPED_REGISTERS
  UNMAP_PCI_MEM(osdev, 1, devc->bar1addr, devc->bar1virt, 1024x1024);
#else
  UNMAP_PCI_IOADDR (devc->osdev, 0);
#endif

  oss_unregister_device (devc->osdev);
  return 1;
}
