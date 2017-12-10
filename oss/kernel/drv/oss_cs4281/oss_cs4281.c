/*
 * Purpose: Driver for Crystal PCI audio controller.
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

#include "oss_cs4281_cfg.h"
#include "midi_core.h"
#include "ac97.h"
#include "oss_pci.h"
#include "cs4281.h"

#define CRYSTAL_VENDOR_ID	0x1013
#define CRYSTAL_CS4281_ID	0x6005

#if 1
#define WRITEB(a,d) devc->bRegister0[a] = d
#define READB(a) devc->bRegister0[a]
#define WRITEW(a,d) devc->wRegister0[a>>1] = d
#define READW(a) devc->wRegister0[a>>1]
#define READL(a) (devc->dwRegister0[a>>2])
#define WRITEL(a, d) devc->dwRegister0[a>>2] = d
#else
#define WRITEB(a,d) PCI_WRITEB(devc->osdev, d, devc->bRegister0[a])
#define READB(a) PCI_READB(devc->osdev, devc->bRegister0[a])
#define WRITEW(a,d) PCI_WRITEW(devc->osdev, d, devc->wRegister0[a>>1])
#define READW(a) PCI_READW(devc->osdev, devc->wRegister0[a>>1])
#define READL(a) PCI_READL(devc->osdev, devc->dwRegister0[a>>2])
#define WRITEL(a, d) PCI_WRITEL(devc->osdev, d, devc->dwRegister0[a>>2])
#endif

#ifdef OSS_BIG_ENDIAN
static unsigned int
be_swap (unsigned int x)
{
  return ((x & 0x000000ff) << 24) |
    ((x & 0x0000ff00) << 8) |
    ((x & 0x00ff0000) >> 8) | ((x & 0xff000000) >> 24);
}

#define LSWAP(x) be_swap(x)
#else
#define LSWAP(x) 	x
#endif

#define MAX_PORTC 2

typedef struct cs4281_portc
{
  int speed, bits, channels;
  int open_mode;
  int trigger_bits;
  int audio_enabled;
  int audiodev;
}
cs4281_portc;

typedef struct cs4281_devc
{
  oss_device_t *osdev;
  char *chip_name;
  oss_native_word bar0addr, bar1addr;
  unsigned int *bar0virt, *bar1virt;
  volatile unsigned int *dwRegister0, *dwRegister1;
  volatile unsigned short *wRegister0, *wRegister1;
  volatile unsigned char *bRegister0, *bRegister1;
  int irq;
  volatile unsigned char intr_mask;
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;

  /* MIDI */
  int midi_opened;
  int midi_dev;
  oss_midi_inputbyte_t midi_input_intr;

  /* Mixer parameters */
  ac97_devc ac97devc;
  int mixer_dev;

  /* Audio parameters */
  cs4281_portc portc[MAX_PORTC];
  int open_mode;
  int fm_attached, mpu_attached;
}
cs4281_devc;



static int
ac97_read (void *devc_, int reg)
{
  cs4281_devc *devc = devc_;
  int count;
  oss_native_word status, value;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  /*
   * Make sure that there is not data sitting around from a previous
   * uncompleted access. ACSDA = Status Data Register = 47Ch
   */
  status = READL (BA0_ACSDA);
  /* Get the actual AC97 register from the offset */
  WRITEL (BA0_ACCAD, reg);
  WRITEL (BA0_ACCDA, 0);
  WRITEL (BA0_ACCTL, ACCTL_DCV | ACCTL_CRW | ACCTL_VFRM | ACCTL_ESYN);

  /* Wait for the read to occur. */
  for (count = 0; count < 500; count++)
    {
      /* First, we want to wait for a short time. */
      oss_udelay (10);
      /*
       * Now, check to see if the read has completed.
       * ACCTL = 460h, DCV should be reset by now and 460h = 17h
       */
      status = READL (BA0_ACCTL);
      if (!(status & ACCTL_DCV))
	{
	  break;
	}
    }

  /* Make sure the read completed. */
  if (status & ACCTL_DCV)
    {
      cmn_err (CE_WARN, "AC97 Read Timedout\n");
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return (-1);
    }

  /* Wait for the valid status bit to go active. */

  for (count = 0; count < 500; count++)
    {
      /*
       * Read the AC97 status register.
       * ACSTS = Status Register = 464h
       */
      status = READL (BA0_ACSTS);
      /*
       * See if we have valid status.
       * VSTS - Valid Status
       */
      if (status & ACSTS_VSTS)
	break;

      /*
       * Wait for a short while.
       */
      oss_udelay (10);
    }
  /* Make sure we got valid status. */
  if (!(status & ACSTS_VSTS))
    {
      cmn_err (CE_WARN, "AC97 Read Timedout\n");
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return (-1);
    }

  /*
   * Read the data returned from the AC97 register.
   * ACSDA = Status Data Register = 474h
   */
  value = READL (BA0_ACSDA);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return (value);
}

static int
ac97_write (void *devc_, int reg, int data)
{
  cs4281_devc *devc = devc_;
  int count;
  oss_native_word status, flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  WRITEL (BA0_ACCAD, reg);
  WRITEL (BA0_ACCDA, data);
  WRITEL (BA0_ACCTL, ACCTL_DCV | ACCTL_VFRM | ACCTL_ESYN);
  for (count = 0; count < 500; count++)
    {
      /* First, we want to wait for a short time. */
      oss_udelay (10);
      /* Now, check to see if the write has completed. */
      /* ACCTL = 460h, DCV should be reset by now and 460h = 07h */
      status = READL (BA0_ACCTL);
      if (!(status & ACCTL_DCV))
	break;
    }

  /* write didn't completed. */
  if (status & ACCTL_DCV)
    {
      cmn_err (CE_WARN, "AC97 Write timeout\n");
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return (-1);
    }
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return 0;
}


static int
cs4281intr (oss_device_t * osdev)
{
  cs4281_devc *devc = (cs4281_devc *) osdev->devc;
  cs4281_portc *portc;
  oss_native_word status, uart_stat;
  int i;
  int serviced = 0;

/* Read the Interrupt Status Register */
  status = READL (BA0_HISR);

/*
 * This is the MIDI read interrupt service. First check to see
 * if the MIDI interrupt flag is set in the HISR register. Next
 * read the MIDI status register. See if Receive Buffer Empty 
 * is empty (0=FIFO Not empty, 1=FIFO is empty
 */
  if ((devc->midi_opened & OPEN_READ) && (status & HISR_MIDI))
    {
      serviced = 1;
      uart_stat = READL (BA0_MIDSR);
/*
 * read one byte of MIDI data and hand it off the the sequencer module
 * to decode this. Keep checking to see if the data is available. Stop
 * when no more data is there in the FIFO.
 */
      while (!(uart_stat & MIDSR_RBE))
	{
	  unsigned char d;
	  d = READL (BA0_MIDRP);

	  if (devc->midi_opened & OPEN_READ && devc->midi_input_intr)
	    devc->midi_input_intr (devc->midi_dev, d);
	  uart_stat = READL (BA0_MIDSR);
	}
    }
/* Audio interrupt handling */
  if (status & (HISR_INTENA | HISR_DMAI))
    for (i = 0; i < MAX_PORTC; i++)
      {
	portc = &devc->portc[i];
	if ((status & HISR_DMA0) && (portc->trigger_bits & PCM_ENABLE_OUTPUT))
	  {
	    dmap_t *dmapout = audio_engines[portc->audiodev]->dmap_out;
	    unsigned int currdac;
	    int n;

	    serviced = 1;
	    READL (BA0_HDSR0);	/* ack the DMA interrupt */
	    currdac = READL (BA0_DCA0) - dmapout->dmabuf_phys;
	    currdac /= dmapout->fragment_size;
	    n = 0;
	    while (dmap_get_qhead (dmapout) != currdac
		   && n++ < dmapout->nfrags)
	      oss_audio_outputintr (portc->audiodev, 1);
	  }

	if ((status & HISR_DMA1) && (portc->trigger_bits & PCM_ENABLE_INPUT))
	  {
	    dmap_t *dmapin = audio_engines[portc->audiodev]->dmap_in;
	    unsigned int curradc;
	    int n;

	    serviced = 1;
	    READL (BA0_HDSR1);	/* ack the DMA interrupt */
	    curradc = READL (BA0_DCA1) - dmapin->dmabuf_phys;
	    curradc /= dmapin->fragment_size;
	    n = 0;
	    while (dmap_get_qtail (dmapin) != curradc && n++ < dmapin->nfrags)
	      oss_audio_inputintr (portc->audiodev, 0);
	  }
	WRITEL (BA0_HICR, HICR_IEV | HICR_CHGM);
      }
  return serviced;
}

static int
cs4281_audio_set_rate (int dev, int arg)
{
  cs4281_portc *portc = audio_engines[dev]->portc;
  if (arg == 0)
    return portc->speed;
  if (arg > 48000)
    arg = 48000;
  if (arg < 6023)
    arg = 6023;
  portc->speed = arg;
  return portc->speed;
}

static short
cs4281_audio_set_channels (int dev, short arg)
{
  cs4281_portc *portc = audio_engines[dev]->portc;
  if ((arg != 1) && (arg != 2))
    return portc->channels;
  portc->channels = arg;
  return portc->channels;
}

static unsigned int
cs4281_audio_set_format (int dev, unsigned int arg)
{
  cs4281_portc *portc = audio_engines[dev]->portc;
  if (arg == 0)
    return portc->bits;
  if (!(arg & (AFMT_U8 | AFMT_S16_LE)))
    return portc->bits;
  portc->bits = arg;
  return portc->bits;
}

/*ARGSUSED*/
static int
cs4281_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void cs4281_audio_trigger (int dev, int state);
static void
cs4281_audio_reset (int dev)
{
  cs4281_audio_trigger (dev, 0);
}

static void
cs4281_audio_reset_input (int dev)
{
  cs4281_portc *portc = audio_engines[dev]->portc;
  cs4281_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
cs4281_audio_reset_output (int dev)
{
  cs4281_portc *portc = audio_engines[dev]->portc;
  cs4281_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

/*ARGSUSED*/
static int
cs4281_audio_open (int dev, int mode, int open_flags)
{
  cs4281_portc *portc = audio_engines[dev]->portc;
  cs4281_devc *devc = audio_engines[dev]->devc;
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
cs4281_audio_close (int dev, int mode)
{
  cs4281_portc *portc = audio_engines[dev]->portc;
  cs4281_devc *devc = audio_engines[dev]->devc;
  cs4281_audio_reset (dev);
  portc->open_mode = 0;
  devc->open_mode &= ~mode;
  portc->audio_enabled &= ~mode;
}

/*ARGSUSED*/
static void
cs4281_audio_output_block (int dev, oss_native_word buf, int count,
			   int fragsize, int intrflag)
{
  cs4281_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
}

/*ARGSUSED*/
static void
cs4281_audio_start_input (int dev, oss_native_word buf, int count,
			  int fragsize, int intrflag)
{
  cs4281_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
}

static void
cs4281_audio_trigger (int dev, int state)
{
  cs4281_devc *devc = audio_engines[dev]->devc;
  cs4281_portc *portc = audio_engines[dev]->portc;
  oss_native_word tmp1;
  oss_native_word flags;
  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode & OPEN_WRITE)
    {
      if (state & PCM_ENABLE_OUTPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      /* Clear DMA0 channel Mask bit. Start Playing. */
	      tmp1 = READL (BA0_DCR0) & ~DCRn_MSK;	/*enable DMA */
	      WRITEL (BA0_DCR0, tmp1);	/* (154h) */
	      WRITEL (BA0_HICR, HICR_IEV | HICR_CHGM);	/*enable intr */
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
	      tmp1 = READL (BA0_DCR0) & ~DCRn_MSK;
	      WRITEL (BA0_DCR0, tmp1 | DCRn_MSK);
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
	      /* Clear DMA1 channel Mask bit. Start recording. */
	      tmp1 = READL (BA0_DCR1) & ~DCRn_MSK;
	      WRITEL (BA0_DCR1, tmp1);	/* (15ch) */
	      WRITEL (BA0_HICR, HICR_IEV | HICR_CHGM);	/*Set INTENA=1. */
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
	      tmp1 = READL (BA0_DCR1) & ~DCRn_MSK;
	      WRITEL (BA0_DCR1, tmp1 | DCRn_MSK);
	    }
	}
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static int
cs4281_rate (oss_native_word rate)
{
  int val = 0;

  switch (rate)
    {
    case 8000:
      val = 5;
      break;
    case 11025:
      val = 4;
      break;
    case 16000:
      val = 3;
      break;
    case 22050:
      val = 2;
      break;
    case 44100:
      val = 1;
      break;
    case 48000:
      val = 0;
      break;
    default:
      val = 1536000 / rate;
      break;
    }
  return val;
}

/*ARGSUSED*/
static int
cs4281_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  cs4281_devc *devc = audio_engines[dev]->devc;
  cs4281_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_in;
  int count = dmap->bytes_in_use;
  oss_native_word recordFormat;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  /* set the record rate */
  WRITEL (BA0_ADCSR, cs4281_rate (portc->speed));	/* (748h) */
  /* Start with defaults for the record format */
  /* reg & modify them for the current case. */
  recordFormat = DMRn_DMA | DMRn_AUTO | DMRn_TR_WRITE;
  if (portc->channels == 1)	/* If mono, */
    recordFormat |= DMRn_MONO;	/* Turn on mono bit. */
  if (portc->bits == 8)		/* If 8-bit, */
    recordFormat |= (DMRn_SIZE8 | DMRn_USIGN);	/* turn on 8bit/unsigned. */
  WRITEL (BA0_DMR1, recordFormat);
  /* set input gain to 0db */
  /* ac97_write(devc, BA0_AC97_RECORD_GAIN, 0x0808);  */
  if (portc->channels == 2)	/* If stereo, */
    count /= 2;			/* halve DMA count(stereo); */
  if (portc->bits == 16)	/* If 16-bit, */
    count /= 2;
  /* Set the physical play buffer address DMA1 Base & Current. */
  WRITEL (BA0_DBA1, dmap->dmabuf_phys);	/* (118h) */
  /* Set the sample count(-1) in DMA Base Count register 1. */
  WRITEL (BA0_DBC1, count - 1);
  portc->audio_enabled &= ~PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/
static int
cs4281_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  cs4281_devc *devc = audio_engines[dev]->devc;
  cs4281_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_out;
  int count = dmap->bytes_in_use;
  oss_native_word playFormat;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  /* Set the sample rate converter */
  WRITEL (BA0_DACSR, cs4281_rate (portc->speed));
  playFormat = DMRn_DMA | DMRn_AUTO | DMRn_TR_READ | (1 << 6);
  if (portc->channels == 1)	/* If stereo, */
    playFormat |= DMRn_MONO;	/* Turn on mono bit. */
  if (portc->bits == 8)		/* If 16-bit, */
    playFormat |= (DMRn_SIZE8 | DMRn_USIGN);	/* turn on 8-bit/unsigned. */
  WRITEL (BA0_DMR0, playFormat);	/* (150h) */
  if (portc->channels == 2)	/* If stereo, */
    count /= 2;			/* halve DMA count(stereo); */
  if (portc->bits == 16)	/* If 16-bit, */
    count /= 2;
  /* Set the physical play buffer address DMA0 Base & Current. */
  WRITEL (BA0_DBA0, dmap->dmabuf_phys & ~0x3);	/* (118h) */
  /* Set the sample count(-1) in DMA Base Count register 0. */
  WRITEL (BA0_DBC0, count - 1);
  portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

static int
cs4281_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  cs4281_devc *devc = audio_engines[dev]->devc;
  unsigned int ptr = 0;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);

  if (direction == PCM_ENABLE_OUTPUT)
    {
      ptr = READL (BA0_DCA0);
    }
  if (direction == PCM_ENABLE_INPUT)
    {
      ptr = READL (BA0_DCA1);
    }
  ptr -= dmap->dmabuf_phys;
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return ptr;
}

static audiodrv_t cs4281_audio_driver = {
  cs4281_audio_open,
  cs4281_audio_close,
  cs4281_audio_output_block,
  cs4281_audio_start_input,
  cs4281_audio_ioctl,
  cs4281_audio_prepare_for_input,
  cs4281_audio_prepare_for_output,
  cs4281_audio_reset,
  NULL,
  NULL,
  cs4281_audio_reset_input,
  cs4281_audio_reset_output,
  cs4281_audio_trigger,
  cs4281_audio_set_rate,
  cs4281_audio_set_format,
  cs4281_audio_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,				/* cs4281_alloc_buffer, */
  NULL,				/* cs4281_free_buffer, */
  NULL,
  NULL,
  cs4281_get_buffer_pointer
};

/***********************MIDI PORT ROUTINES ****************/
/*ARGSUSED*/
static int
cs4281_midi_open (int dev, int mode, oss_midi_inputbyte_t inputbyte,
		  oss_midi_inputbuf_t inputbuf,
		  oss_midi_outputintr_t outputintr)
{
  cs4281_devc *devc = (cs4281_devc *) midi_devs[dev]->devc;
  if (devc->midi_opened)
    {
      return OSS_EBUSY;
    }

  devc->midi_input_intr = inputbyte;
  devc->midi_opened = mode;
  /* first reset the MIDI port */
  WRITEL (BA0_MIDCR, 0x10);
  WRITEL (BA0_MIDCR, 0x00);
  /* Now check if we're in Read or Write mode */
  if (mode & OPEN_READ)
    {
      /* enable MIDI Input intr and receive enable */
      WRITEL (BA0_MIDCR, MIDCR_RXE | MIDCR_RIE);
    }

  if (mode & OPEN_WRITE)
    {
      /* enable MIDI transmit enable */
      WRITEL (BA0_MIDCR, MIDCR_TXE);
    }
  return 0;
}

/*ARGSUSED*/
static void
cs4281_midi_close (int dev, int mode)
{
  cs4281_devc *devc = (cs4281_devc *) midi_devs[dev]->devc;
/* Reset the device*/
  WRITEL (BA0_MIDCR, 0x10);
  WRITEL (BA0_MIDCR, 0x00);
  devc->midi_opened = 0;
}

static int
cs4281_midi_out (int dev, unsigned char midi_byte)
{
  cs4281_devc *devc = (cs4281_devc *) midi_devs[dev]->devc;
  unsigned char uart_stat = READL (BA0_MIDSR);
/* Check if Transmit buffer full flag is set - if so return */
  if ((uart_stat & MIDSR_TBF))
    return 0;
/* Now put the MIDI databyte in the write port */
  WRITEL (BA0_MIDWP, midi_byte);
  return 1;
}

/*ARGSUSED*/
static int
cs4281_midi_ioctl (int dev, unsigned cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static midi_driver_t cs4281_midi_driver = {
  cs4281_midi_open,
  cs4281_midi_close,
  cs4281_midi_ioctl,
  cs4281_midi_out
};

static int
init_cs4281 (cs4281_devc * devc)
{

  int my_mixer, i;
  oss_native_word dwAC97SlotID, tmp1;
  int first_dev = 0;
/****************BEGIN HARDWARE INIT*****************/
  /* Setup CFLR */
  tmp1 = READL (BA0_CFLR);
  if (tmp1 != 0x01)
    {
      WRITEL (BA0_CFLR, 0x01);	/*set up AC97 mode */
      tmp1 = READL (BA0_CFLR);
      if (tmp1 != 0x01)
	{
	  tmp1 = READL (BA0_CWPR);
	  if (tmp1 != 0x4281)
	    WRITEL (BA0_CWPR, 0x4281);
	  tmp1 = READL (BA0_CWPR);
	  if (tmp1 != 0x4281)
	    {
	      cmn_err (CE_WARN, "Resetting AC97 failed\n");
	      return OSS_EIO;
	    }
	  WRITEL (BA0_CFLR, 0x1);
	  tmp1 = READL (BA0_CFLR);
	  if (tmp1 != 0x1)
	    {
	      cmn_err (CE_WARN, "Resetting AC97 still fails\n");
	      return OSS_EIO;
	    }
	}
    }
  /* Setup the FM and Joystick trap address for Legacy emulation */
  WRITEL (BA0_IOTCR, 0x1);
  WRITEL (BA0_IOTFM, 0xc0030388);
  WRITEL (BA0_IOTGP, 0xc0070200);
  /**************************************** */
  /*  Set up the Sound System Configuration */
  /**************************************** */
  /* Set the 'Configuration Write Protect' register */
  /* to 4281h.  Allows vendor-defined configuration */
  /* space between 0e4h and 0ffh to be written. */
  WRITEL (BA0_CWPR, 0x4281);	/* (3e0h) */
  if ((tmp1 = READL (BA0_SERC1)) != (SERC1_SO1EN | SERC1_SO1F_AC97))
    {
      cmn_err (CE_WARN, "SERC1: AC97 check failed\n");
      return OSS_EIO;
    }
  /* setup power management to full power */
  WRITEL (BA0_SSPM, 0x7E);
  /* First, blast the clock control register to zero so that the */
  /* PLL starts out in a known state, and blast the master serial */
  /* port control register to zero so that the serial ports also */
  /* start out in a known state. */
  WRITEL (BA0_CLKCR1, 0);	/* (400h) */
  WRITEL (BA0_SERMC, 0);	/* (420h) */
  /* (1) Drive the ARST# pin low for a minimum of 1uS (as defined in */
  /* the AC97 spec) and then drive it high.  This is done for non */
  /* AC97 modes since there might be logic external to the CS461x */
  /* that uses the ARST# line for a reset. */
  WRITEL (BA0_ACCTL, 0);
  oss_udelay (50);
  WRITEL (BA0_SPMC, 0);		/* (3ech) */
  oss_udelay (50);
  WRITEL (BA0_SPMC, SPMC_RSTN);
  oss_udelay (50000);
  WRITEL (BA0_SERMC, SERMC_PTC_AC97 | SERMC_MSPE | 0x10000);
  /* (3) Turn on the Sound System Clocks. */
  WRITEL (BA0_CLKCR1, CLKCR1_DLLP);	/* (400h) */
  /* Wait for the PLL to stabilize. */
  oss_udelay (50000);
  /* Turn on clocking of the core (CLKCR1(400h) = 0x00000030) */
  WRITEL (BA0_CLKCR1, CLKCR1_DLLP | CLKCR1_SWCE);
  /* (5) Wait for clock stabilization. */
  for (tmp1 = 0; tmp1 < 100; tmp1++)
    {
      if (READL (BA0_CLKCR1) & CLKCR1_DLLRDY)
	break;
      oss_udelay (50000);
    }
  if (!(READL (BA0_CLKCR1) & CLKCR1_DLLRDY))
    {
      cmn_err (CE_WARN, "DLLRDY Clock not ready\n");
      return OSS_EIO;
    }
  /* (6) Enable ASYNC generation. */
  WRITEL (BA0_ACCTL, ACCTL_ESYN);	/* (460h) */
  /* Now wait 'for a short while' to allow the  AC97 */
  /* part to start generating bit clock. (so we don't */
  /* Try to start the PLL without an input clock.) */
  /* (7) Wait for the codec ready signal from the AC97 codec. */
  for (tmp1 = 0; tmp1 < 100; tmp1++)
    {
      /* Delay a mil to let things settle out and */
      /* to prevent retrying the read too quickly. */
      if (READL (BA0_ACSTS) & ACSTS_CRDY)	/* If ready,  (464h) */
	break;			/*   exit the 'for' loop. */
      oss_udelay (50000);
    }
  if (!(READL (BA0_ACSTS) & ACSTS_CRDY))	/* If never came ready, */
    {
      cmn_err (CE_WARN, "AC97 not ready\n");
      return OSS_EIO;		/*   exit initialization. */
    }
  /* (8) Assert the 'valid frame' signal so we can */
  /* begin sending commands to the AC97 codec. */
  WRITEL (BA0_ACCTL, ACCTL_VFRM | ACCTL_ESYN);	/* (460h) */

  /* (11) Wait until we've sampled input slots 3 & 4 as valid, meaning */
  /* that the codec is pumping ADC data across the AC link. */
  for (tmp1 = 0; tmp1 < 100; tmp1++)
    {
      /* Read the input slot valid register;  See */
      /* if input slots 3 and 4 are valid yet. */
      if ((READL (BA0_ACISV) & (ACISV_ISV3 | ACISV_ISV4)) ==	/* (474h) */
	  (ACISV_ISV3 | ACISV_ISV4))
	break;			/* Exit the 'for' if slots are valid. */
      oss_udelay (50000);
    }
  /* If we never got valid data, exit initialization. */
  if ((READL (BA0_ACISV) & (ACISV_ISV3 | ACISV_ISV4)) !=
      (ACISV_ISV3 | ACISV_ISV4))
    {
      cmn_err (CE_WARN, "AC97 Slot not valid\n");
      return OSS_EIO;		/* If no valid data, exit initialization. */
    }
  /* (12) Start digital data transfer of audio data to the codec. */
  WRITEL (BA0_ACOSV, ACOSV_SLV3 | ACOSV_SLV4);	/* (468h) */

  /* For playback, we map AC97 slot 3 and 4(Left */
  /* & Right PCM playback) to DMA Channel 0. */
  /* Set the fifo to be 31 bytes at offset zero. */
  dwAC97SlotID = 0x01001F00;	/* FCR0.RS[4:0]=1(=>slot4, right PCM playback). */
  /* FCR0.LS[4:0]=0(=>slot3, left PCM playback). */
  /* FCR0.SZ[6-0]=15; FCR0.OF[6-0]=0. */
  WRITEL (BA0_FCR0, dwAC97SlotID);	/* (180h) */
  WRITEL (BA0_FCR0, dwAC97SlotID | FCRn_FEN);	/* Turn on FIFO Enable. */
  /* For capture, we map AC97 slot 10 and 11(Left */
  /* and Right PCM Record) to DMA Channel 1. */
  /* Set the fifo to be 31 bytes at offset 32. */
  dwAC97SlotID = 0x0B0A1F20;	/* FCR1.RS[4:0]=11(=>slot11, right PCM record). */
  /* FCR1.LS[4:0]=10(=>slot10, left PCM record). */
  /* FCR1.SZ[6-0]=15; FCR1.OF[6-0]=16. */
  WRITEL (BA0_FCR1, dwAC97SlotID);	/* (184h) */
  WRITEL (BA0_FCR1, dwAC97SlotID | FCRn_FEN);	/* Turn on FIFO Enable. */
  /* Map the Playback SRC to the same AC97 slots(3 & 4-- */
  /* --Playback left & right)as DMA channel 0. */
  /* Map the record SRC to the same AC97 slots(10 & 11-- */
  /* -- Record left & right) as DMA channel 1. */
  dwAC97SlotID = 0x0b0a0100;	/*SCRSA.PRSS[4:0]=1(=>slot4, right PCM playback). */
  /*SCRSA.PLSS[4:0]=0(=>slot3, left PCM playback). */
  /*SCRSA.CRSS[4:0]=11(=>slot11, right PCM record) */
  /*SCRSA.CLSS[4:0]=10(=>slot10, left PCM record). */
  WRITEL (BA0_SRCSA, dwAC97SlotID);	/* (75ch) */
  /* Set 'Half Terminal Count Interrupt Enable' and 'Terminal */
  /* Count Interrupt Enable' in DMA Control Registers 0 & 1. */
  /* Set 'MSK' flag to 1 to keep the DMA engines paused. */
  tmp1 = (DCRn_HTCIE | DCRn_TCIE | DCRn_MSK);	/* (00030001h) */
  WRITEL (BA0_DCR0, tmp1);	/* (154h) */
  WRITEL (BA0_DCR1, tmp1);	/* (15ch) */
  /* Set 'Auto-Initialize Control' to 'enabled'; For playback, */
  /* set 'Transfer Type Control'(TR[1:0]) to 'read transfer', */
  /* for record, set Transfer Type Control to 'write transfer'. */
  /* All other bits set to zero;  Some will be changed @ transfer start. */
  tmp1 = (DMRn_DMA | DMRn_AUTO | DMRn_TR_READ);	/* (20000018h) */
  WRITEL (BA0_DMR0, tmp1);	/* (150h) */
  tmp1 = (DMRn_DMA | DMRn_AUTO | DMRn_TR_WRITE);	/* (20000014h) */
  WRITEL (BA0_DMR1, tmp1);	/* (158h) */
  /* Enable DMA interrupts generally, and */
  /* DMA0 & DMA1 interrupts specifically. */
  tmp1 = READL (BA0_HIMR) & 0x7fbbfcff;
  WRITEL (BA0_HIMR, tmp1);
  /* set up some volume defaults */
  WRITEL (BA0_PPLVC, 0x0808);
  WRITEL (BA0_PPRVC, 0x0808);
  WRITEL (BA0_FMLVC, 0x0);
  WRITEL (BA0_FMRVC, 0x0);
/****** END OF HARDWARE INIT *****/


  my_mixer =
    ac97_install (&devc->ac97devc, "CS4281 AC97 Mixer", ac97_read, ac97_write,
		  devc, devc->osdev);
  if (my_mixer < 0)
    {
      return 0;
    }

  devc->mixer_dev = my_mixer;

  /* ac97_write(devc,  0x26, ac97_read(devc, 0x26) | 0x8000); */
  for (i = 0; i < MAX_PORTC; i++)
    {
      int adev;
      int caps = 0;
      cs4281_portc *portc = &devc->portc[i];
      char tmp_name[100];
      strcpy (tmp_name, devc->chip_name);

      if (i == 0)
	{
          caps = ADEV_AUTOMODE | ADEV_DUPLEX;
	  strcpy (tmp_name, devc->chip_name);
	}
      else
	{
          caps = ADEV_AUTOMODE | ADEV_DUPLEX | ADEV_SHADOW;
	  strcpy (tmp_name, devc->chip_name);
	}

      if ((adev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
					devc->osdev,
					devc->osdev,
					tmp_name,
					&cs4281_audio_driver,
					sizeof (audiodrv_t),
					caps,
					AFMT_S16_LE | AFMT_U8, devc, -1)) < 0)
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
	  audio_engines[adev]->min_rate = 6023;
	  audio_engines[adev]->max_rate = 48000;
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

  devc->midi_dev = oss_install_mididev (OSS_MIDI_DRIVER_VERSION, "CS4281", "CS4281 MIDI Port", &cs4281_midi_driver, sizeof (midi_driver_t),
					0, devc, devc->osdev);
  devc->midi_opened = 0;
  return 1;
}

int
oss_cs4281_attach (oss_device_t * osdev)
{
  unsigned char pci_irq_line, pci_revision, pci_irq_inta;
  unsigned short pci_command, vendor, device;
  unsigned int ioaddr;
  int err;
  cs4281_devc *devc;

  DDB (cmn_err (CE_WARN, "Entered CS4281 probe routine\n"));

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  if (vendor != CRYSTAL_VENDOR_ID || device != CRYSTAL_CS4281_ID)
    return 0;

  if ((devc = PMALLOC (osdev, sizeof (*devc))) == NULL)
    {
      cmn_err (CE_WARN, "Out of memory\n");
      return 0;
    }

  devc->osdev = osdev;
  osdev->devc = devc;

  oss_pci_byteswap (osdev, 1);

  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_byte (osdev, PCI_INTERRUPT_LINE + 1, &pci_irq_inta);
  pci_read_config_dword (osdev, PCI_MEM_BASE_ADDRESS_0, &ioaddr);
  devc->bar0addr = ioaddr;
  pci_read_config_dword (osdev, PCI_MEM_BASE_ADDRESS_1, &ioaddr);
  devc->bar1addr = ioaddr;

  /* activate the device enable bus master/memory space */
  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  if ((devc->bar0addr == 0) || (devc->bar1addr == 0))
    {
      cmn_err (CE_WARN, "undefined MEMORY I/O address.\n");
      return 0;
    }

  if (pci_irq_line == 0)
    {
      cmn_err (CE_WARN, "IRQ not assigned by BIOS.\n");
      return 0;
    }


  /* Map the shared memory area */
  devc->bar0virt =
    (unsigned int *) MAP_PCI_MEM (devc->osdev, 0, devc->bar0addr, 4 * 1024);
  devc->bar1virt =
    (unsigned int *) MAP_PCI_MEM (devc->osdev, 1, devc->bar1addr,
				  1024 * 1024);
  devc->dwRegister0 = devc->bar0virt;
  devc->wRegister0 = (unsigned short *) devc->bar0virt;
  devc->bRegister0 = (unsigned char *) devc->bar0virt;
  devc->dwRegister1 = devc->bar1virt;
  devc->wRegister1 = (unsigned short *) devc->bar1virt;
  devc->bRegister1 = (unsigned char *) devc->bar1virt;

  devc->chip_name = "CS4281";
  devc->irq = pci_irq_line;
  devc->open_mode = 0;

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);

  oss_register_device (osdev, devc->chip_name);

  if ((err = oss_register_interrupts (devc->osdev, 0, cs4281intr, NULL)) < 0)
    {
      cmn_err (CE_WARN, "Can't register interrupt handler, err=%d\n", err);
      return 0;
    }
  return init_cs4281 (devc);	/*Detected */
}


int
oss_cs4281_detach (oss_device_t * osdev)
{
  cs4281_devc *devc = (cs4281_devc *) osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  WRITEL (BA0_HICR, 0x02);	/*enable intena */

  oss_unregister_interrupts (devc->osdev);

  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);

  UNMAP_PCI_MEM (devc->osdev, 0, devc->bar0addr, devc->bar0virt, 4 * 1024);
  UNMAP_PCI_MEM (devc->osdev, 1, devc->bar1addr, devc->bar1virt, 1024 * 1024);

  devc->bar0addr = 0;
  devc->bar1addr = 0;

  oss_unregister_device (devc->osdev);
  return 1;
}
