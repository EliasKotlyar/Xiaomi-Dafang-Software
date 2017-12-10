/*
 * Purpose: Driver for Crystal cs461x and cs461x PCI audio controllers
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

#include "oss_cs461x_cfg.h"
#include "midi_core.h"
#include "ac97.h"
#include "oss_pci.h"
#include "cs461x.h"

extern int cs461x_clkrun_fix;

#define CRYSTAL_VENDOR_ID	0x1013
#define CRYSTAL_CS4610_ID	0x6001
#define CRYSTAL_CS461x_ID	0x6003
#define CRYSTAL_CS4615_ID	0x6004

#define USE_SG

#define WRITEB(a,d) devc->bRegister0[a] = d
#define READB(a) devc->bRegister0[a]
#define WRITEW(a,d) devc->wRegister0[a>>1] = d
#define READW(a) devc->wRegister0[a>>1]

#define READ0L(a) (devc->dwRegister0[a>>2])
#define WRITE0L(a, d) (devc->dwRegister0[a>>2] = d)
#define READ1L(a) (devc->dwRegister1[a>>2])
#define WRITE1L(a,d) (devc->dwRegister1[a>>2] = d)

#define READ1L(a) (devc->dwRegister1[a>>2])

#ifdef OSS_BIG_ENDIAN
static unsigned int
ymf_swap (unsigned int x)
{
  return ((x & 0x000000ff) << 24) |
    ((x & 0x0000ff00) << 8) |
    ((x & 0x00ff0000) >> 8) | ((x & 0xff000000) >> 24);
}

#define LSWAP(x) ymf_swap(x)
#else
#define LSWAP(x) 	x
#endif

#define MAX_PORTC 2

typedef struct cs461x_portc
{
  int speed, bits, channels;
  int open_mode;
  int audio_enabled;
  int trigger_bits;
  int audiodev;
}
cs461x_portc;

typedef struct cs461x_devc
{
  oss_device_t *osdev;
  char *chip_name;
  unsigned short subsysid;

  oss_native_word bar0addr, bar1addr;
  unsigned int *bar0virt, *bar1virt;
  volatile unsigned int *dwRegister0, *dwRegister1;
  volatile unsigned short *wRegister0, *wRegister1;
  volatile unsigned char *bRegister0, *bRegister1;
  int irq;
  int dual_codec;
  unsigned int *play_sgbuf;
  unsigned int play_sgbuf_phys;
  oss_dma_handle_t play_sgbuf_dma_handle;

  /* Mutex */
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;

  /* MIDI */
  int midi_opened;
  int midi_dev;
  oss_midi_inputbyte_t midi_input_intr;

  /* Mixer parameters */
  ac97_devc ac97devc, ac97devc2;
  int mixer_dev, mixer2_dev;

  /* Audio parameters */
  cs461x_portc portc[MAX_PORTC];
  int open_mode;
  oss_native_word PCTL;		/*Play Control Register */
  oss_native_word CCTL;		/*Record Control Register */
  int processor_started;
}
cs461x_devc;

#define MAX_CS461x 6

static int ac97_write (void *devc, int addr, int data);
static int ac97_read (void *devc, int addr);

static void
PokeBA0 (cs461x_devc * devc, unsigned int offset, unsigned int value)
{
  WRITE0L (offset, value);
}

static unsigned int
PeekBA0 (cs461x_devc * devc, unsigned int offset)
{
  unsigned int value;

  value = READ0L (offset);
  return (value);

}

static int
ac97_read (void *devc_, int offset)
{
  cs461x_devc *devc = devc_;
  int count;
  unsigned int status, value;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  /*
   * Make sure that there is not data sitting around from a previous
   * uncompleted access. ACSDA = Status Data Register = 47Ch
   */
  status = PeekBA0 (devc, BA0_ACSDA);
  /* Get the actual AC97 register from the offset */
  PokeBA0 (devc, BA0_ACCAD, offset - BA0_AC97_RESET);
  PokeBA0 (devc, BA0_ACCDA, 0);
  PokeBA0 (devc, BA0_ACCTL, ACCTL_DCV | ACCTL_CRW | ACCTL_VFRM |
	   ACCTL_ESYN | ACCTL_RSTN);

  /* Wait for the read to occur. */
  for (count = 0; count < 10; count++)
    {
      /* First, we want to wait for a short time. */
      oss_udelay (100);
      /*
       * Now, check to see if the read has completed.
       * ACCTL = 460h, DCV should be reset by now and 460h = 17h
       */
      status = PeekBA0 (devc, BA0_ACCTL);
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

  for (count = 0; count < 10; count++)
    {
      /*
       * Read the AC97 status register.
       * ACSTS = Status Register = 464h
       */
      status = PeekBA0 (devc, BA0_ACSTS);
      /*
       * See if we have valid status.
       * VSTS - Valid Status
       */
      if (status & ACSTS_VSTS)
	break;

      /*
       * Wait for a short while.
       */
      oss_udelay (100);
    }
  /* Make sure we got valid status. */
  if (!(status & ACSTS_VSTS))
    {
      cmn_err (CE_WARN, "AC97 Read Timedout(2)\n");
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return (-1);
    }

  /*
   * Read the data returned from the AC97 register.
   * ACSDA = Status Data Register = 474h
   */
  value = PeekBA0 (devc, BA0_ACSDA);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return (value);
}

static int
ac97_write (void *devc_, int offset, int data)
{
  cs461x_devc *devc = devc_;
  int count;
  unsigned int status = 0;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  PokeBA0 (devc, BA0_ACCAD, offset);
  PokeBA0 (devc, BA0_ACCDA, data);
  PokeBA0 (devc, BA0_ACCTL, ACCTL_DCV | ACCTL_VFRM | ACCTL_ESYN | ACCTL_RSTN);
  for (count = 0; count < 10; count++)
    {
      /* First, we want to wait for a short time. */
      oss_udelay (100);
      /* Now, check to see if the write has completed. */
      /* ACCTL = 460h, DCV should be reset by now and 460h = 07h */
      status = PeekBA0 (devc, BA0_ACCTL);
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
ac97_read2 (void *devc_, int offset)
{
  cs461x_devc *devc = devc_;
  int count;
  unsigned int status, value;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  /*
   * Make sure that there is not data sitting around from a previous
   * uncompleted access. ACSDA = Status Data Register = 47Ch
   */
  status = PeekBA0 (devc, BA0_ACSDA2);

  /* Get the actual AC97 register from the offset */
  PokeBA0 (devc, BA0_ACCAD, offset);
  PokeBA0 (devc, BA0_ACCDA, 0);
  PokeBA0 (devc, BA0_ACCTL,
	   ACCTL_DCV | ACCTL_TC | ACCTL_CRW | ACCTL_VFRM | ACCTL_ESYN |
	   ACCTL_RSTN);


  /* Wait for the read to occur. */
  for (count = 0; count < 10; count++)
    {
      /* First, we want to wait for a short time. */
      oss_udelay (1000);
      /*
       * Now, check to see if the read has completed.
       * ACCTL = 460h, DCV should be reset by now and 460h = 17h
       */
      status = PeekBA0 (devc, BA0_ACCTL);
      if (!(status & ACCTL_DCV))
	break;
    }

  /* Make sure the read completed. */
  if (PeekBA0 (devc, BA0_ACCTL) & ACCTL_DCV)
    {
      cmn_err (CE_WARN, "Secondary AC97 Read Timedout\n");
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return (-1);
    }

  /* Wait for the valid status bit to go active. */

  for (count = 0; count < 10; count++)
    {
      /*
       * Read the AC97 status register.
       * ACSTS = Status Register = 464h
       */
      status = PeekBA0 (devc, BA0_ACSTS2);
      /*
       * See if we have valid status.
       * VSTS - Valid Status
       */
      if (status & ACSTS_VSTS)
	break;

      /*
       * Wait for a short while.
       */
      oss_udelay (1000);
    }
  /* Make sure we got valid status */
  if (!(status & ACSTS_VSTS))
    {
      cmn_err (CE_WARN, "Secondary AC97 Read Timedout(2)\n");
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return (-1);
    }

  /*
   * Read the data returned from the AC97 register.
   * ACSDA = Status Data Register = 474h
   */
  value = PeekBA0 (devc, BA0_ACSDA2);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return (value);
}

static int
ac97_write2 (void *devc_, int offset, int data)
{
  cs461x_devc *devc = devc_;
  int count;
  unsigned int status = 0;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  PokeBA0 (devc, BA0_ACCAD, offset);
  PokeBA0 (devc, BA0_ACCDA, data);
  PokeBA0 (devc, BA0_ACCTL,
	   ACCTL_DCV | ACCTL_TC | ACCTL_VFRM | ACCTL_ESYN | ACCTL_RSTN);
  for (count = 0; count < 10; count++)
    {
      /* First, we want to wait for a short time. */
      oss_udelay (1000);
      /* Now, check to see if the write has completed. */
      /* ACCTL = 460h, DCV should be reset by now and 460h = 07h */
      status = PeekBA0 (devc, BA0_ACCTL);
      if (!(status & ACCTL_DCV))
	break;
    }

  /* write didn't completed. */
  if (status & ACCTL_DCV)
    {
      cmn_err (CE_WARN, "Secondary AC97 Write timeout\n");
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return (-1);
    }
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return 0;
}

/*  
 * this is 3*1024 for parameter, 3.5*1024 for sample and 2*3.5*1024 
 * for code since each instruction is 40 bits and takes two dwords
 */
#define INKY_BA1_DWORD_SIZE (13 * 1024 + 512)
/* this is parameter, sample, and code */
#define INKY_MEMORY_COUNT 3
struct cs461x_firmware_struct
{
  struct
  {
    unsigned int ulDestAddr;
    unsigned int ulSourceSize;
  }
  MemoryStat[INKY_MEMORY_COUNT];
  unsigned int BA1Array[INKY_BA1_DWORD_SIZE];
};
#include "cs461x_dsp.h"

static void
DoTransfer (cs461x_devc * devc, unsigned int *fpulSrc,
	    unsigned int ulByteDestOffset, unsigned int ulByteLength)
{
  int dwByteCounter;

  if (ulByteDestOffset & 0x3)
    {
      cmn_err (CE_WARN, "invalid DMA address\n");
      return;
    }

  for (dwByteCounter = 0; dwByteCounter < ulByteLength; dwByteCounter += 4)
    {
      WRITE1L ((ulByteDestOffset + dwByteCounter),
	       fpulSrc[dwByteCounter / 4]);
    }
}

static void
install_ucode (cs461x_devc * devc)
{
  unsigned int i, count;

  count = 0;
  for (i = 0; i < INKY_MEMORY_COUNT; i++)
    {

      DoTransfer (devc, (unsigned int *) (cs461x_firmware.BA1Array + count),
		  cs461x_firmware.MemoryStat[i].ulDestAddr,
		  cs461x_firmware.MemoryStat[i].ulSourceSize);
      count += cs461x_firmware.MemoryStat[i].ulSourceSize / 4;
    }
}

void
clear_serial_fifo (cs461x_devc * devc)
{
  unsigned int ulIdx, ulLoop;
  unsigned int ulStatus = 0;
  unsigned int ulCLKCR1;


  ulCLKCR1 = PeekBA0 (devc, BA0_CLKCR1);

  if (!(ulCLKCR1 & CLKCR1_SWCE))
    {
      PokeBA0 (devc, BA0_CLKCR1, ulCLKCR1 | CLKCR1_SWCE);
    }
  PokeBA0 (devc, BA0_SERBWP, 0);

  for (ulIdx = 0; ulIdx < 256; ulIdx++)
    {
      /*
       * Make sure the previous FIFO write operation has completed.
       */
      for (ulLoop = 0; ulLoop < 5; ulLoop++)
	{
	  oss_udelay (100);
	  ulStatus = PeekBA0 (devc, BA0_SERBST);
	  if (!(ulStatus & SERBST_WBSY))
	    {
	      break;
	    }
	}

      if (ulStatus & SERBST_WBSY)
	{
	  if (!(ulCLKCR1 & CLKCR1_SWCE))
	    {
	      PokeBA0 (devc, BA0_CLKCR1, ulCLKCR1);
	    }
	}

      /* Write the serial port FIFO index. */

      PokeBA0 (devc, BA0_SERBAD, ulIdx);

      /*
       * Tell the serial port to load the new value into the FIFO location.
       */
      PokeBA0 (devc, BA0_SERBCM, SERBCM_WRC);
    }

  /*
   * Now, if we powered up the devices, then power them back down again.
   * This is kinda ugly, but should never happen.
   */
  if (!(ulCLKCR1 & CLKCR1_SWCE))
    {
      PokeBA0 (devc, BA0_CLKCR1, ulCLKCR1);
    }

}

static int
cs461x_reset_processor (cs461x_devc * devc)
{
  unsigned int ulIdx;

  /* Write the reset bit of the SP control register. */
  WRITE1L (BA1_SPCR, SPCR_RSTSP);
  WRITE1L (BA1_SPCR, SPCR_DRQEN);

  /* Clear the trap registers. */
  for (ulIdx = 0; ulIdx < 8; ulIdx++)
    {
      WRITE1L (BA1_DREG, DREG_REGID_TRAP_SELECT + ulIdx);
      WRITE1L (BA1_TWPR, 0xFFFF);
    }

  WRITE1L (BA1_DREG, 0);
  WRITE1L (BA1_FRMT, 0xadf);
  return (0);
}

static int
cs461x_start_processor (cs461x_devc * devc)
{

  unsigned int ulCount;
  int ulTemp = 0;

  /* reset the processor first */
  cs461x_reset_processor (devc);

  /* reload the ucode */
  install_ucode (devc);

  /* Set the frame timer to reflect the number of cycles per frame. */
  WRITE1L (BA1_FRMT, 0xADF);

  /*
   * Turn on the run, run at frame, and DMA enable bits in the local copy of
   * the SP control register.
   */
  WRITE1L (BA1_SPCR, SPCR_RUN | SPCR_RUNFR | SPCR_DRQEN);

  /*
   * Wait until the run at frame bit resets itself in the SP control
   * register.
   */
  for (ulCount = 0; ulCount < 25; ulCount++)
    {
      /* Wait a little bit, so we don't issue PCI reads too frequently. */
      oss_udelay (100);

      /* Fetch the current value of the SP status register. */
      ulTemp = READ1L (BA1_SPCR);

      /* If the run at frame bit has reset, then stop waiting. */
      if (!(ulTemp & SPCR_RUNFR))
	break;
    }
  /* If the run at frame bit never reset, then return an error. */
  if (ulTemp & SPCR_RUNFR)
    {
      cmn_err (CE_WARN, "Start(): SPCR_RUNFR never reset.\n");
    }
  devc->processor_started = 1;
  return 0;
}

static int
cs461xintr (oss_device_t * osdev)
{
  cs461x_devc *devc = (cs461x_devc *) osdev->devc;
  cs461x_portc *portc;
  unsigned int status;
  int i;
  unsigned int uart_stat;
  int serviced = 0;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

/* Read the Interrupt Status Register */
  status = PeekBA0 (devc, BA0_HISR);

/*
 * This is the MIDI read interrupt service. First check to see
 * if the MIDI interrupt flag is set in the HISR register. Next
 * read the MIDI status register. See if Receive Buffer Empty 
 * is empty (0=FIFO Not empty, 1=FIFO is empty
 */
  if ((devc->midi_opened & OPEN_READ) && (status & HISR_MIDI))
    {
      uart_stat = PeekBA0 (devc, BA0_MIDSR);
/*
 * read one byte of MIDI data and hand it off the the sequencer module
 * to decode this. Keep checking to see if the data is available. Stop
 * when no more data is there in the FIFO.
 */
      while (!(uart_stat & MIDSR_RBE))
	{
	  unsigned char d;
	  d = PeekBA0 (devc, BA0_MIDRP);

	  if (devc->midi_opened & OPEN_READ && devc->midi_input_intr)
	    devc->midi_input_intr (devc->midi_dev, d);
	  uart_stat = PeekBA0 (devc, BA0_MIDSR);
	}
    }

/* Audio interrupt handling */
  if (status & 0x3)
    {
      for (i = 0; i < MAX_PORTC; i++)
	{
	  portc = &devc->portc[i];
	  if (status & 0x1)
	    if (portc->trigger_bits & PCM_ENABLE_OUTPUT)
	      {
		dmap_t *dmap = audio_engines[portc->audiodev]->dmap_out;
		int ptr, n;

		ptr = READ1L (BA1_PBA) - dmap->dmabuf_phys;
		ptr = ptr / dmap->fragment_size;
		if (ptr < 0 || ptr >= dmap->nfrags)
		  ptr = 0;
		n = 0;
		while (dmap_get_qhead (dmap) != ptr && n++ < dmap->nfrags)
		  oss_audio_outputintr (portc->audiodev, 0);
	      }

	  if (status & 0x2)
	    if (portc->trigger_bits & PCM_ENABLE_INPUT)
	      {

		dmap_t *dmap = audio_engines[portc->audiodev]->dmap_in;
		int ptr, n;

		ptr = READ1L (BA1_CBA) - dmap->dmabuf_phys;
		ptr = ptr / (dmap->fragment_size);

		if (ptr < 0 || ptr >= dmap->nfrags)
		  ptr = 0;
		n = 0;
		while (dmap_get_qtail (dmap) != ptr && n++ < dmap->nfrags)
		  oss_audio_inputintr (portc->audiodev, 0);
	      }
	}
    }

  serviced = 1;
  PokeBA0 (devc, BA0_HICR, 0x03);

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return serviced;
}

static int
cs461x_audio_set_rate (int dev, int arg)
{
  cs461x_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->speed;

  if (audio_engines[dev]->flags & ADEV_FIXEDRATE)
    {
      audio_engines[dev]->fixed_rate = 48000;
      arg = 48000;
    }
  else
    audio_engines[dev]->fixed_rate = 0;

  if (arg > 48000)
    arg = 48000;
  if (arg < 5000)
    arg = 5000;
  portc->speed = arg;
  return portc->speed;
}

static short
cs461x_audio_set_channels (int dev, short arg)
{
  cs461x_portc *portc = audio_engines[dev]->portc;

  if (audio_engines[dev]->flags & ADEV_STEREOONLY)
    arg = 2;

  if ((arg != 1) && (arg != 2))
    return portc->channels;
  portc->channels = arg;

  return portc->channels;
}

static unsigned int
cs461x_audio_set_format (int dev, unsigned int arg)
{
  cs461x_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->bits;

  if (audio_engines[dev]->flags & ADEV_16BITONLY)
    arg = 16;

  if (!(arg & (AFMT_U8 | AFMT_S16_LE)))
    return portc->bits;
  portc->bits = arg;

  return portc->bits;
}

/*ARGSUSED*/
static int
cs461x_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void cs461x_audio_trigger (int dev, int state);

static void
cs461x_audio_reset (int dev)
{
  cs461x_audio_trigger (dev, 0);
}

static void
cs461x_audio_reset_input (int dev)
{
  cs461x_portc *portc = audio_engines[dev]->portc;
  cs461x_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
cs461x_audio_reset_output (int dev)
{
  cs461x_portc *portc = audio_engines[dev]->portc;
  cs461x_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

/*ARGSUSED*/
static int
cs461x_audio_open (int dev, int mode, int open_flags)
{
  cs461x_portc *portc = audio_engines[dev]->portc;
  cs461x_devc *devc = audio_engines[dev]->devc;
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
#if 0
  if (mode & OPEN_READ)
    {
      dmapin->buffsize = 4096;
      audio_engines[dev]->fixed_rate = 48000;
      audio_engines[dev]->min_rate = 48000;
      audio_engines[dev]->max_rate = 48000;
      audio_engines[dev]->flags |=
	ADEV_FIXEDRATE | ADEV_16BITONLY | ADEV_STEREOONLY;
    }
  if (mode & OPEN_WRITE)
    {
      audio_engines[dev]->min_rate = 5000;
      audio_engines[dev]->max_rate = 48000;
      audio_engines[dev]->flags &=
	~(ADEV_FIXEDRATE | ADEV_16BITONLY | ADEV_STEREOONLY);
      audio_engines[dev]->fixed_rate = 0;
    }
#endif
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

static void
cs461x_audio_close (int dev, int mode)
{
  cs461x_portc *portc = audio_engines[dev]->portc;
  cs461x_devc *devc = audio_engines[dev]->devc;

  cs461x_audio_reset (dev);
  portc->audio_enabled &= ~mode;
  devc->open_mode &= ~mode;
  portc->open_mode = 0;
}

/*ARGSUSED*/
static void
cs461x_audio_output_block (int dev, oss_native_word buf, int count,
			   int fragsize, int intrflag)
{
  cs461x_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
}

/*ARGSUSED*/
static void
cs461x_audio_start_input (int dev, oss_native_word buf, int count,
			  int fragsize, int intrflag)
{
  cs461x_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
}

static void
cs461x_audio_trigger (int dev, int state)
{
  oss_native_word flags, tmp;
  cs461x_devc *devc = audio_engines[dev]->devc;
  cs461x_portc *portc = audio_engines[dev]->portc;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode & OPEN_WRITE)
    {
      if (state & PCM_ENABLE_OUTPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      tmp = READ1L (BA1_PCTL);
	      tmp &= 0xFFFF;
	      tmp |= devc->PCTL;
	      WRITE1L (BA1_PCTL, tmp);
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
	      tmp = READ1L (BA1_PCTL);
	      tmp &= 0xFFFF;
	      WRITE1L (BA1_PCTL, tmp);
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
	      tmp = READ1L (BA1_CCTL);
	      tmp &= 0xFFFF0000;
	      tmp |= devc->CCTL;
	      WRITE1L (BA1_CCTL, tmp);
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
	      tmp = READ1L (BA1_CCTL);
	      tmp &= 0xFFFF0000;
	      WRITE1L (BA1_CCTL, tmp);
	    }
	}
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static int
cs461x_play_rate (cs461x_devc * devc, unsigned int ulInRate)
{
/*define GOF_PER_SEC 200*/
  unsigned int ulTemp1, ulTemp2;
  unsigned int ulPhiIncr;
  unsigned int ulCorrectionPerGOF, ulCorrectionPerSec;
  unsigned int ulOutRate = 48000;

  ulTemp1 = ulInRate << 16;
  ulPhiIncr = ulTemp1 / ulOutRate;
  ulTemp1 -= ulPhiIncr * ulOutRate;
  ulTemp1 <<= 10;
  ulPhiIncr <<= 10;
  ulTemp2 = ulTemp1 / ulOutRate;
  ulPhiIncr += ulTemp2;
  ulTemp1 -= ulTemp2 * ulOutRate;
  ulCorrectionPerGOF = ulTemp1 / 200;
  ulTemp1 -= ulCorrectionPerGOF * 200;
  ulCorrectionPerSec = ulTemp1;

  /* Fill in the SampleRateConverter control block. */
  WRITE1L (BA1_PSRC, ((ulCorrectionPerSec << 16) & 0xFFFF0000) |
	   (ulCorrectionPerGOF & 0xFFFF));
  WRITE1L (BA1_PPI, ulPhiIncr);
  return 0;
}

static int
cs461x_record_rate (cs461x_devc * devc, int ulOutRate)
{
  unsigned int ulPhiIncr, ulCoeffIncr, ulTemp1, ulTemp2;
  unsigned int ulCorrectionPerGOF, ulCorrectionPerSec, ulInitialDelay;
  unsigned int ulInRate = 48000;

  /*
   * We can only decimate by up to a factor of 1/9th the hardware rate.
   * Return an error if an attempt is made to stray outside that limit.
   */
  if ((ulOutRate * 9) < ulInRate)
    {
      cmn_err (CE_WARN,
	       "SetCaptureSampleRate(): Requested output rate is below 1/9th of the input rate.\n");
      return (-1);
    }

  /*
   * We can not capture at at rate greater than the Input Rate (48000).
   * Return an error if an attempt is made to stray outside that limit.
   */
  if (ulOutRate > ulInRate)
    {
      cmn_err (CE_WARN,
	       "SetCaptureSampleRate(): Requested output rate is greater than the input rate.");
      return (-1);
    }
  ulTemp1 = ulOutRate << 16;
  ulCoeffIncr = ulTemp1 / ulInRate;
  ulTemp1 -= ulCoeffIncr * ulInRate;
  ulTemp1 <<= 7;
  ulCoeffIncr <<= 7;
  ulCoeffIncr += ulTemp1 / ulInRate;
  ulCoeffIncr ^= 0xFFFFFFFF;
  ulCoeffIncr++;
  ulTemp1 = ulInRate << 16;
  ulPhiIncr = ulTemp1 / ulOutRate;
  ulTemp1 -= ulPhiIncr * ulOutRate;
  ulTemp1 <<= 10;
  ulPhiIncr <<= 10;
  ulTemp2 = ulTemp1 / ulOutRate;
  ulPhiIncr += ulTemp2;
  ulTemp1 -= ulTemp2 * ulOutRate;
  ulCorrectionPerGOF = ulTemp1 / 200;
  ulTemp1 -= ulCorrectionPerGOF * 200;
  ulCorrectionPerSec = ulTemp1;
  ulInitialDelay = ((ulInRate * 24) + ulOutRate - 1) / ulOutRate;
  /* Fill in the VariDecimate control block. */
  WRITE1L (BA1_CSRC, ((ulCorrectionPerSec << 16) & 0xFFFF0000) |
	   (ulCorrectionPerGOF & 0xFFFF));
  WRITE1L (BA1_CCI, ulCoeffIncr);
  WRITE1L (BA1_CD,
	   (((BA1_VARIDEC_BUF_1 + (ulInitialDelay << 2)) << 16) & 0xFFFF0000)
	   | 0x80);
  WRITE1L (BA1_CPI, ulPhiIncr);
#if 0
  {
    unsigned int frameGroupLength, cnt;
    int rate = ulOutRate;

    frameGroupLength = 1;
    for (cnt = 2; cnt <= 64; cnt *= 2)
      {
	if (((rate / cnt) * cnt) != rate)
	  frameGroupLength *= 2;
      }
    if (((rate / 3) * 3) != rate)
      {
	frameGroupLength *= 3;
      }
    for (cnt = 5; cnt <= 125; cnt *= 5)
      {
	if (((rate / cnt) * cnt) != rate)
	  frameGroupLength *= 5;
      }

    WRITE1L (BA1_CFG1, frameGroupLength);
    WRITE1L (BA1_CFG2, (0x00800000 | frameGroupLength));
    WRITE1L (BA1_CCST, 0x0000FFFF);
    WRITE1L (BA1_CSPB, ((65536 * rate) / 24000));
    WRITE1L ((BA1_CSPB + 4), 0x0000FFFF);
  }
#endif
  return (0);
}

struct InitStruct
{
  unsigned long off;
  unsigned long val;
}
InitArray[] =
{
  {
  0x00000040, 0x3fc0000f}
  ,
  {
  0x0000004c, 0x04800000}
  ,
  {
  0x000000b3, 0x00000780}
  ,
  {
  0x000000b7, 0x00000000}
  ,
  {
  0x000000bc, 0x07800000}
  ,
  {
  0x000000cd, 0x00800000}
,};


/*
 * "SetCaptureSPValues()" -- Initialize record task values before each
 *      capture startup.
 */
void
SetCaptureSPValues (cs461x_devc * devc)
{
  unsigned i, offset;
  for (i = 0; i < sizeof (InitArray) / sizeof (struct InitStruct); i++)
    {
      offset = InitArray[i].off * 4;	/* 8bit to 32bit offset value */
      WRITE1L (offset, InitArray[i].val);
    }
}

/*ARGSUSED*/
static int
cs461x_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  cs461x_devc *devc = audio_engines[dev]->devc;
  cs461x_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_in;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  /* Start the processor */
  if (!devc->processor_started)
    cs461x_start_processor (devc);

  /* Set Capture S/P values */
  SetCaptureSPValues (devc);
  /* set the record rate */
  cs461x_record_rate (devc, portc->speed);

  /* write the buffer address */
  WRITE1L (BA1_CBA, dmap->dmabuf_phys);

  portc->audio_enabled &= ~PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/
static int
cs461x_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  cs461x_devc *devc = audio_engines[dev]->devc;
  cs461x_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_out;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  /* Start the processor */
  if (!devc->processor_started)
    cs461x_start_processor (devc);

#ifdef USE_SG
  {
    unsigned int sg_temp[9], sg_npages = 0;
    int i;
    oss_native_word Count, playFormat, tmp, tmp2;

    sg_npages = (dmap->bytes_in_use) / 4096;

    devc->play_sgbuf[0] = dmap->dmabuf_phys;
    devc->play_sgbuf[1] = 0x00000008;

    for (i = 0; i < sg_npages; i++)
      {
	devc->play_sgbuf[2 * i] = dmap->dmabuf_phys + 4096 * i;
	if (i == sg_npages - 1)
	  tmp2 = 0xbfff0000;
	else
	  tmp2 = 0x80000000 + 8 * (i + 1);

	devc->play_sgbuf[2 * i + 1] = tmp2;
      }
    sg_temp[0] = 0x82c0200d;
    sg_temp[1] = 0xffff0000;
    sg_temp[2] = devc->play_sgbuf[0];
    sg_temp[3] = 0x00010600;
    sg_temp[4] = devc->play_sgbuf[2];
    sg_temp[5] = 0x80000010;
    sg_temp[6] = devc->play_sgbuf[0];
    sg_temp[7] = devc->play_sgbuf[2];
    sg_temp[8] = (devc->play_sgbuf_phys & 0xffff000) | 0x10;

    for (i = 0; i < sizeof (sg_temp) / 4; i++)
      WRITE1L ((BA1_PDTC + i * 4), sg_temp[i]);

    WRITE1L (BA1_PBA, dmap->dmabuf_phys);

    cs461x_play_rate (devc, (unsigned int) portc->speed);

    Count = 4;
    playFormat = READ1L (BA1_PFIE);
    playFormat &= ~0x0000f03f;

    if ((portc->channels == 2))
      {
	playFormat &= ~DMA_RQ_C2_AC_MONO_TO_STEREO;
	Count *= 2;
      }
    else
      playFormat |= DMA_RQ_C2_AC_MONO_TO_STEREO;

    if ((portc->bits == 16))
      {
	playFormat &=
	  ~(DMA_RQ_C2_AC_8_TO_16_BIT | DMA_RQ_C2_AC_SIGNED_CONVERT);
	Count *= 2;
      }
    else
      playFormat |= (DMA_RQ_C2_AC_8_TO_16_BIT | DMA_RQ_C2_AC_SIGNED_CONVERT);

    WRITE1L (BA1_PFIE, playFormat);

    tmp = READ1L (BA1_PDTC);
    tmp &= 0xfffffe00;
    WRITE1L (BA1_PDTC, tmp | --Count);
  }
#else
  {
    unsigned int pdtc_value, pfie_value;
    /* Set the sample rate converter */
    cs461x_play_rate (devc, (unsigned int) portc->speed);

    pfie_value = READ1L (BA1_PFIE);
    pfie_value &= ~0x0000f03f;

    pdtc_value = READ1L (BA1_PDTC);
    pdtc_value &= ~0x000003ff;

    /* Now set the sample size/stereo/mono */
    if ((portc->bits == 8) && (portc->channels == 1))
      {
	pdtc_value |= 0x03;
	pfie_value |= 0xB000;	/*8bit mono */
      }
    if ((portc->bits == 8) && (portc->channels == 2))
      {
	pdtc_value |= 0x07;
	pfie_value |= 0xA000;	/*8bit stereo */
      }
    if ((portc->bits == 16) && (portc->channels == 1))
      {
	pdtc_value |= 0x07;
	pfie_value |= 0x2000;	/*16bit mono */
      }
    if ((portc->bits == 16) && (portc->channels == 2))
      {
	pdtc_value |= 0x0F;
	pfie_value |= 0x0000;	/*16bit stereo */
      }

    WRITE1L (BA1_PDTC, pdtc_value);
    WRITE1L (BA1_PFIE, pfie_value);
    WRITE1L (BA1_PBA, dmap->dmabuf_phys);
  }
#endif

  portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

static int
cs461x_alloc_buffer (int dev, dmap_t * dmap, int direction)
{

#ifdef USE_SG
  int err;
#else
  oss_native_word phaddr;
  cs461x_devc *devc = audio_engines[dev]->devc;
#endif

  if (dmap->dmabuf != NULL)
    return 0;

#ifdef USE_SG
  if ((err = oss_alloc_dmabuf (dev, dmap, direction)) < 0)
    {
      cmn_err (CE_WARN, "Failed to allocate a DMA buffer\n");
      return err;
    }
#else
  dmap->dmabuf =
    (void *) CONTIG_MALLOC (devc->osdev, 4096, MEMLIMIT_32BITS, &phaddr, dmap->dmabuf_dma_handle);
  dmap->dmabuf_phys = phaddr;
# ifdef linux
  oss_reserve_pages (dmap->dmabuf, dmap->dmabuf + 4096 - 1);
# endif
  dmap->buffsize = 4096;
#endif
  return 0;
}

/*ARGSUSED*/
static int
cs461x_free_buffer (int dev, dmap_t * dmap, int direction)
{
#ifndef USE_SG
  cs461x_devc *devc = audio_engines[dev]->devc;
#endif
#ifdef USE_SG
  oss_free_dmabuf (dev, dmap);
  dmap->dmabuf = 0;
#else
  if (dmap->dmabuf == NULL)
    return 0;
  CONTIG_FREE (devc->osdev, dmap->dmabuf, 4096, dmap->dmabuf_dma_handle);
# ifdef linux
  oss_unreserve_pages (dmap->dmabuf, dmap->dmabuf + 4096 - 1);
# endif
  dmap->dmabuf = 0;
#endif
  return 0;

}

static int
cs461x_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  cs461x_devc *devc = audio_engines[dev]->devc;
  unsigned int ptr = 0;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  if (direction == PCM_ENABLE_OUTPUT)
    {
      ptr = READ1L (BA1_PBA) - dmap->dmabuf_phys;
    }
  if (direction == PCM_ENABLE_INPUT)
    {
      ptr = READ1L (BA1_CBA) - dmap->dmabuf_phys;
    }
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return ptr;
}

static audiodrv_t cs461x_audio_driver = {
  cs461x_audio_open,
  cs461x_audio_close,
  cs461x_audio_output_block,
  cs461x_audio_start_input,
  cs461x_audio_ioctl,
  cs461x_audio_prepare_for_input,
  cs461x_audio_prepare_for_output,
  cs461x_audio_reset,
  NULL,
  NULL,
  cs461x_audio_reset_input,
  cs461x_audio_reset_output,
  cs461x_audio_trigger,
  cs461x_audio_set_rate,
  cs461x_audio_set_format,
  cs461x_audio_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  cs461x_alloc_buffer,
  cs461x_free_buffer,
  NULL,
  NULL,
  cs461x_get_buffer_pointer
};

/***********************MIDI PORT ROUTINES ****************/

/*ARGSUSED*/
static int
cs461x_midi_open (int dev, int mode, oss_midi_inputbyte_t inputbyte,
		  oss_midi_inputbuf_t inputbuf,
		  oss_midi_outputintr_t outputintr)
{
  cs461x_devc *devc = (cs461x_devc *) midi_devs[dev]->devc;
  oss_native_word flags;

  if (devc->midi_opened)
    {
      return OSS_EBUSY;
    }
  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  devc->midi_input_intr = inputbyte;
  devc->midi_opened = mode;

  /* first reset the MIDI port */
  PokeBA0 (devc, BA0_MIDCR, 0x10);
  PokeBA0 (devc, BA0_MIDCR, 0x00);

  /* Now check if we're in Read or Write mode */
  if (mode & OPEN_READ)
    {
      /* enable MIDI Input intr and receive enable */
      PokeBA0 (devc, BA0_MIDCR, MIDCR_RXE | MIDCR_RIE);
    }

  if (mode & OPEN_WRITE)
    {
      /* enable MIDI transmit enable without interrupt mode */
      PokeBA0 (devc, BA0_MIDCR, MIDCR_TXE);
    }
  PokeBA0 (devc, BA0_HICR, HICR_IEV | HICR_CHGM);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/
static void
cs461x_midi_close (int dev, int mode)
{
  cs461x_devc *devc = (cs461x_devc *) midi_devs[dev]->devc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
/* Reset the device*/
  PokeBA0 (devc, BA0_MIDCR, 0x10);
  PokeBA0 (devc, BA0_MIDCR, 0x00);
  devc->midi_opened = 0;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static int
cs461x_midi_out (int dev, unsigned char midi_byte)
{
  cs461x_devc *devc = (cs461x_devc *) midi_devs[dev]->devc;
  unsigned char uart_stat;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  uart_stat = PeekBA0 (devc, BA0_MIDSR);

/* Check if Transmit buffer full flag is set - if so return */
  if ((uart_stat & MIDSR_TBF))
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return 0;
    }
/* Now put the MIDI databyte in the write port */
  PokeBA0 (devc, BA0_MIDWP, midi_byte);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 1;
}

/*ARGSUSED*/
static int
cs461x_midi_ioctl (int dev, unsigned cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static midi_driver_t cs461x_midi_driver = {
  cs461x_midi_open,
  cs461x_midi_close,
  cs461x_midi_ioctl,
  cs461x_midi_out
};

static int
init_cs461x (cs461x_devc * devc)
{

  int my_mixer, my_mixer2, i;
  unsigned int ulCount = 0;
  unsigned int ulStatus = 0;
  unsigned int tmp;
  int first_dev = 0;
  int adev;
  oss_native_word phaddr;

/****************BEGIN HARDWARE INIT*****************/
  /*
   * First, blast the clock control register to zero so that the PLL starts
   * out in a known state, and blast the master serial port control register
   * to zero so that the serial ports also start out in a known state.
   */
  PokeBA0 (devc, BA0_CLKCR1, 0x0);
  PokeBA0 (devc, BA0_SERMC1, 0x0);
  oss_udelay (1000);
  /*
   * If we are in AC97 mode, then we must set the part to a host controlled
   * AC-link.  Otherwise, we won't be able to bring up the link.
   */
  if (devc->dual_codec)
    PokeBA0 (devc, BA0_SERACC,
	     SERACC_HSP | SERACC_TWO_CODECS | SERACC_CODEC_TYPE_2_0);
  else
    PokeBA0 (devc, BA0_SERACC, SERACC_HSP | SERACC_CODEC_TYPE_1_03);	/* AC97 1.03 */

  /*
   * Drive the ARST# pin low for a minimum of 1uS (as defined in the AC97
   * spec) and then drive it high.  This is done for non AC97 modes since
   * there might be logic external to the CS461x that uses the ARST# line
   * for a reset.
   */
  PokeBA0 (devc, BA0_ACCTL, 1);
  oss_udelay (10000);
  PokeBA0 (devc, BA0_ACCTL, 0);
  oss_udelay (10000);
  PokeBA0 (devc, BA0_ACCTL, ACCTL_RSTN);
  /*
   * The first thing we do here is to enable sync generation.  As soon
   * as we start receiving bit clock, we'll start producing the SYNC
   * signal.
   */
  PokeBA0 (devc, BA0_ACCTL, ACCTL_ESYN | ACCTL_RSTN);

  /*
   * Now wait for a short while to allow the AC97 part to start
   * generating bit clock (so we don't try to start the PLL without an
   * input clock).
   */
  oss_udelay (500000);
  /*
   * Set the serial port timing configuration, so that
   * the clock control circuit gets its clock from the correct place.
   */
  PokeBA0 (devc, BA0_SERMC1, SERMC1_PTC_AC97);
  oss_udelay (500000);
  /*
   * Write the selected clock control setup to the hardware.  Do not turn on
   * SWCE yet (if requested), so that the devices clocked by the output of
   * PLL are not clocked until the PLL is stable.
   */
  PokeBA0 (devc, BA0_PLLCC, PLLCC_LPF_1050_2780_KHZ | PLLCC_CDR_73_104_MHZ);
  PokeBA0 (devc, BA0_PLLM, 0x3a);
  PokeBA0 (devc, BA0_CLKCR2, CLKCR2_PDIVS_8);

  /*
   * Power up the PLL.
   */
  PokeBA0 (devc, BA0_CLKCR1, CLKCR1_PLLP);

  /*
   * Wait until the PLL has stabilized.
   */
  oss_udelay (100000);
  /*
   * Turn on clocking of the core so that we can setup the serial ports.
   */
  tmp = PeekBA0 (devc, BA0_CLKCR1) | CLKCR1_SWCE;
  PokeBA0 (devc, BA0_CLKCR1, tmp);

  /* Clear the FIFOs */
  clear_serial_fifo (devc);
  /* PokeBA0 (devc, BA0_SERBSP, 0); */

  /*
   * Write the serial port configuration to the part.  The master
   * enable bit is not set until all other values have been written.
   */
  PokeBA0 (devc, BA0_SERC1, SERC1_SO1F_AC97 | SERC1_SO1EN);
  PokeBA0 (devc, BA0_SERC2, SERC2_SI1F_AC97 | SERC1_SO1EN);
  PokeBA0 (devc, BA0_SERMC1, SERMC1_PTC_AC97 | SERMC1_MSPE);

  PokeBA0 (devc, BA0_SERC7, SERC7_ASDI2EN);
  PokeBA0 (devc, BA0_SERC3, 0);
  PokeBA0 (devc, BA0_SERC4, 0);
  PokeBA0 (devc, BA0_SERC5, 0);
  PokeBA0 (devc, BA0_SERC6, 0);

  oss_udelay (100000);


  /* Wait for the codec ready signal from the AC97 codec. */
  for (ulCount = 0; ulCount < 1000; ulCount++)
    {
      /*
       * First, lets wait a short while to let things settle out a bit,
       * and to prevent retrying the read too quickly.
       */
      oss_udelay (10000);
      /*
       * Read the AC97 status register to see if we've seen a CODEC READY
       * signal from the AC97 codec.
       */
      ulStatus = PeekBA0 (devc, BA0_ACSTS);
      if (ulStatus & ACSTS_CRDY)
	break;
    }

  if (!(ulStatus & ACSTS_CRDY))
    {
      cmn_err (CE_WARN, "Initialize() Never read Codec Ready from AC97.\n");
      return 0;
    }

  if (devc->dual_codec)
    {
      for (ulCount = 0; ulCount < 1000; ulCount++)
	{
	  /*
	   *            
	   * First, lets wait a short while to let things settle out a bit,
	   * and to prevent retrying the read too quickly.
	   *
	   */
	  oss_udelay (10000);
	  /*
	   * Read the AC97 status register to see if we've seen a CODEC READY
	   * signal from the AC97 codec.
	   */
	  ulStatus = PeekBA0 (devc, BA0_ACSTS2);
	  if (ulStatus & ACSTS_CRDY)
	    break;
	}

      if (!(ulStatus & ACSTS_CRDY))
	{
	  cmn_err (CE_WARN,
		   "Initialize() Never read Codec Ready from second AC97.\n");
	}
    }
  /*
   * Assert the vaid frame signal so that we can start sending commands
   * to the AC97 codec.
   */
  PokeBA0 (devc, BA0_ACCTL, ACCTL_VFRM | ACCTL_ESYN | ACCTL_RSTN);
  /*
   * Wait until we've sampled input slots 3 and 4 as valid, meaning that
   * the codec is pumping ADC data across the AC-link.
   */
  for (ulCount = 0; ulCount < 1000; ulCount++)
    {
      /*
       * First, lets wait a short while to let things settle out a bit,
       * and to prevent retrying the read too quickly.
       */
      oss_udelay (10000);
      /*
       * Read the input slot valid register and see if input slots 3 and
       * 4 are valid yet.
       */
      ulStatus = PeekBA0 (devc, BA0_ACISV);
      if ((ulStatus & (ACISV_ISV3 | ACISV_ISV4)) == (ACISV_ISV3 | ACISV_ISV4))
	break;
    }
  /*
   * Make sure we sampled valid input slots 3 and 4.  If not, then return
   * an error.
   */
  if ((ulStatus & (ACISV_ISV3 | ACISV_ISV4)) != (ACISV_ISV3 | ACISV_ISV4))
    cmn_err (CE_WARN, "Initialize(%x) Never sampled ISV3 & ISV4 from AC97.\n",
	     (int) ulStatus);
  /*
   * Now, assert valid frame and the slot 3 and 4 valid bits.  This will
   * commense the transfer of digital audio data to the AC97 codec.
   */
  PokeBA0 (devc, BA0_ACOSV, ACOSV_SLV3 | ACOSV_SLV4);

  /* Reset the Processor */
  cs461x_reset_processor (devc);
  /* Now download the image */
  install_ucode (devc);

  devc->processor_started = 0;

  /* Save the play and capture parameters and write 0 to stop DMA */
  devc->PCTL = READ1L (BA1_PCTL) & 0xFFFF0000;
  WRITE1L (BA1_PCTL, devc->PCTL & 0x0000FFFF);
  devc->CCTL = READ1L (BA1_CCTL) & 0x0000FFFF;
  WRITE1L (BA1_CCTL, devc->CCTL & 0xFFFF0000);

  /*
   *  Enable interrupts on the part.
   */
  PokeBA0 (devc, BA0_HICR, HICR_IEV | HICR_CHGM);

  ulStatus = READ1L (BA1_PFIE);
  ulStatus &= ~0x0000f03f;
  WRITE1L (BA1_PFIE, ulStatus);	/* playback interrupt enable */

  ulStatus = READ1L (BA1_CIE);
  ulStatus &= ~0x0000003f;
  ulStatus |= 0x00000001;
  WRITE1L (BA1_CIE, ulStatus);	/* capture interrupt enable */

/****** END OF HARDWARE INIT *****/

  my_mixer =
    ac97_install (&devc->ac97devc, "CS461x AC97 Mixer", ac97_read, ac97_write,
		  devc, devc->osdev);

  if (my_mixer >= 0)
    {
      devc->mixer_dev = my_mixer;
      if (devc->subsysid == 0x5053)	/* SantaCruz has dual codec */
	{
	  my_mixer2 =
	    ac97_install (&devc->ac97devc2, "CS4630 AC97 Secondary",
			  ac97_read2, ac97_write2, devc, devc->osdev);
	  if (my_mixer2 >= 0)
	    {
	      devc->mixer2_dev = my_mixer2;
	      ac97_write2 (devc, BA0_AC97_6CH_VOL_C_LFE, 0x7FFF);
	      ac97_write2 (devc, BA0_AC97_6CH_VOL_SURROUND, 0x7FFF);
	    }
	}
    }
  else
    return 0;

  /* Turn on amp on the CS4297 on TB/Videologic */
  if (devc->subsysid == 0x5053)
    {
      unsigned short u16PinConfig, u16LogicType, u16ValidSlots, u16Idx;
      unsigned short u16Status = 0;
      int i;


      ac97_write (devc, 0x26, ac97_read (devc, 0x26) | 0x8000);
#if 0
      /* Set SPDIF */
      PokeBA0 (devc, BA0_ASER_MASTER, 1);	/*ASER_MASTER_ME */
      WRITE1L (0x8049, 0x00000001);
#endif
      /*
       *  Set GPIO pin's 7 and 8 so that they are configured for output.
       */
      u16PinConfig = ac97_read2 (devc, BA0_AC97_GPIO_PIN_CONFIG);
      u16PinConfig &= 0x27F;
      ac97_write2 (devc, BA0_AC97_GPIO_PIN_CONFIG, u16PinConfig);

      /*
       * Set GPIO pin's 7 and 8 so that they are compatible with CMOS logic.
       */
      u16LogicType = ac97_read2 (devc, BA0_AC97_GPIO_PIN_TYPE);
      u16LogicType &= 0x27F;
      ac97_write2 (devc, BA0_AC97_GPIO_PIN_TYPE, u16LogicType);

      u16ValidSlots = PeekBA0 (devc, BA0_ACOSV);
      u16ValidSlots |= 0x200;
      PokeBA0 (devc, BA0_ACOSV, u16ValidSlots);

      /*
       * Fill slots 12 with the correct value for the GPIO pins. 
       */
      for (u16Idx = 0x90; u16Idx <= 0x9F; u16Idx++)
	{

	  /*
	   * Initialize the fifo so that bits 7 and 8 are on.
	   *
	   * Remember that the GPIO pins in bonzo are shifted by 4 bits to
	   * the left.  0x1800 corresponds to bits 7 and 8.
	   */
	  PokeBA0 (devc, BA0_SERBWP, 0x1800);

	  /*
	   * Make sure the previous FIFO write operation has completed.
	   */
	  for (i = 0; i < 5; i++)
	    {
	      u16Status = PeekBA0 (devc, BA0_SERBST);
	      if (!(u16Status & SERBST_WBSY))
		{
		  break;
		}
	      oss_udelay (10000);
	    }

	  /*
	   * Write the serial port FIFO index.
	   */
	  PokeBA0 (devc, BA0_SERBAD, u16Idx);

	  /*
	   * Tell the serial port to load the new value into the FIFO location.
	   */
	  PokeBA0 (devc, BA0_SERBCM, SERBCM_WRC);
	}
    }
  /* Hercules Game Theater Amp */
  if (devc->subsysid == 0x1681)
    {
      PokeBA0 (devc, BA0_EGPIODR, EGPIODR_GPOE2);	/* enable EGPIO2 output */
      PokeBA0 (devc, BA0_EGPIOPTR, EGPIOPTR_GPPT2);	/* open-drain on output */

    }

#ifdef USE_SG
  devc->play_sgbuf =
    (unsigned int *) CONTIG_MALLOC (devc->osdev, 4096, MEMLIMIT_32BITS,
				    &phaddr, devc->play_sgbuf_dma_handle);
  if (devc->play_sgbuf == NULL)
    {
      cmn_err (CE_WARN,
	       "cs461x: Failed to allocate play scatter/gather buffer.\n");
      return 0;
    }
  devc->play_sgbuf_phys = phaddr;
#endif

  for (i = 0; i < MAX_PORTC; i++)
    {
      char tmp_name[100];
      cs461x_portc *portc = &devc->portc[i];
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
					&cs461x_audio_driver,
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
	  audio_engines[adev]->mixer_dev = my_mixer;
	  audio_engines[adev]->rate_source = first_dev;
	  audio_engines[adev]->min_rate = 5000;
	  audio_engines[adev]->max_rate = 48000;
	  audio_engines[adev]->min_block = 4096;
	  audio_engines[adev]->max_block = 4096;
	  audio_engines[adev]->caps |= PCM_CAP_FREERATE;
	  portc->open_mode = 0;
	  portc->audiodev = adev;
	  portc->audio_enabled = 0;
	  portc->trigger_bits = 0;
#ifdef CONFIG_OSS_VMIX
	  if (i == 0)
	     vmix_attach_audiodev(devc->osdev, adev, -1, 0);
#endif
	}

    }

#ifndef sparc
  devc->midi_dev =
    oss_install_mididev (OSS_MIDI_DRIVER_VERSION, "CS461x",
			 "CS461x MIDI Port", &cs461x_midi_driver,
			 sizeof (midi_driver_t),
			 0, devc, devc->osdev);
#endif
  devc->midi_opened = 0;
  return 1;
}

int
oss_cs461x_attach (oss_device_t * osdev)
{
  unsigned char pci_irq_line, pci_revision, pci_irq_inta;
  unsigned short pci_command, vendor, device;
  unsigned int ioaddr;
  int err;
  cs461x_devc *devc;

  DDB (cmn_err (CE_WARN, "Entered CS461x probe routine\n"));
#if 0
  if (cs461x_clkrun_fix)
    while ((osdev = (pci_find_class (0x680 << 8, osdev))))

      {
	unsigned char pp;
	unsigned int port, control;
	unsigned short vendor, device;

	pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
	pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

	if (vendor != 0x8086 || device != 0x7113)
	  continue;

	pci_read_config_byte (osdev, 0x41, &pp);
	port = pp << 8;
	control = INW (devc->osdev, port + 0x10);
	OUTW (devc->osdev, control | 0x2000, port + 0x10);
	oss_udelay (100);
	OUTW (devc->osdev, control & ~0x2000, port + 0x10);
      }
#endif

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  if (vendor != CRYSTAL_VENDOR_ID || device != CRYSTAL_CS461x_ID)
    return 0;

  if ((devc = PMALLOC (osdev, sizeof (*devc))) == NULL)
    {
      cmn_err (CE_WARN, "Out of memory\n");
      return 0;
    }

  devc->osdev = osdev;
  osdev->devc = devc;
  devc->open_mode = 0;

  oss_pci_byteswap (osdev, 1);

  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_byte (osdev, PCI_INTERRUPT_LINE + 1, &pci_irq_inta);
  pci_read_config_dword (osdev, PCI_MEM_BASE_ADDRESS_0, &ioaddr);
  devc->bar0addr = ioaddr;
  pci_read_config_dword (osdev, PCI_MEM_BASE_ADDRESS_1, &ioaddr);
  devc->bar1addr = ioaddr;
  pci_read_config_word (osdev, PCI_SUBSYSTEM_VENDOR_ID, &devc->subsysid);

  switch (device)
    {
    case CRYSTAL_CS461x_ID:
      devc->chip_name = "Crystal CS461x";
      break;

    case CRYSTAL_CS4610_ID:
      devc->chip_name = "Crystal CS4610";
      break;

    case CRYSTAL_CS4615_ID:
      devc->chip_name = "Crystal CS4615";
      break;

    default:
      devc->chip_name = "CrystalPCI";
    }

  if (devc->subsysid == 0x5053)
    {
      devc->chip_name = "Crystal CS4630";
      devc->dual_codec = 1;
    }

  /* activate the device enable bus master/memory space */
  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  if ((devc->bar0addr == 0) || (devc->bar1addr == 0))
    {
      cmn_err (CE_WARN, "Undefined MEMORY I/O address.\n");
      return 0;
    }

  if (pci_irq_line == 0)
    {
      cmn_err (CE_WARN, "IRQ not assigned by BIOS.\n");
      return 0;
    }

  /* Map the shared memory area */
  devc->bar0virt =
    (unsigned int *) MAP_PCI_MEM (devc->osdev, 0, devc->bar0addr, 0x2000);
  devc->bar1virt =
    (unsigned int *) MAP_PCI_MEM (devc->osdev, 1, devc->bar1addr, 0x40000);
  devc->dwRegister0 = (unsigned int *) devc->bar0virt;
  devc->wRegister0 = (unsigned short *) devc->bar0virt;
  devc->bRegister0 = (unsigned char *) devc->bar0virt;
  devc->dwRegister1 = (unsigned int *) devc->bar1virt;
  devc->wRegister1 = (unsigned short *) devc->bar1virt;
  devc->bRegister1 = (unsigned char *) devc->bar1virt;

  devc->irq = pci_irq_line;

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);

  oss_register_device (osdev, devc->chip_name);

  if ((err = oss_register_interrupts (devc->osdev, 0, cs461xintr, NULL)) < 0)
    {
      cmn_err (CE_WARN, "Can't register interrupt handler, err=%d\n", err);
      return 0;
    }

  return init_cs461x (devc);	/*Detected */
}


int
oss_cs461x_detach (oss_device_t * osdev)
{
  cs461x_devc *devc = (cs461x_devc *) osdev->devc;


  if (oss_disable_device (osdev) < 0)
    return 0;

  PokeBA0 (devc, BA0_HICR, 0x02);	/*enable intena */
  cs461x_reset_processor (devc);
#ifdef USE_SG
  CONTIG_FREE (devc->osdev, devc->play_sgbuf, 4096, devc->play_sgbuf_dma_handle);
#endif

  oss_unregister_interrupts (devc->osdev);

  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);

  UNMAP_PCI_MEM (devc->osdev, 0, devc->bar0addr, devc->bar0virt, 0x2000);
  UNMAP_PCI_MEM (devc->osdev, 1, devc->bar1addr, devc->bar1virt, 0x40000);

  devc->bar0addr = 0;
  devc->bar1addr = 0;

  oss_unregister_device (devc->osdev);
  return 1;
}
