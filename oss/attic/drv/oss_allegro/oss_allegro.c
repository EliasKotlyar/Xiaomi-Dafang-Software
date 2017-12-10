/*
 * Driver for ALLEGRO PCI audio controller.
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


#include "oss_allegro_cfg.h"
#include "oss_pci.h"
#include "ac97.h"
#include "uart401.h"

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define NT_MODEL

#define ESSM3_VENDOR_ID	  0x125D
#define ESS_1988       0x1988	/* allegro */
#define ESS_1989       0x1989	/* allegro */
#define ESS_1990       0x1990	/* Canyon 3D */
#define ESS_1992       0x1992	/* Canyon 3D-2 */
#define ESS_1998       0x1998	/* m3 */
#define ESS_1999       0x1999	/* m3 */
#define ESS_199A       0x199a	/* m3 */
#define ESS_199B       0x199b	/* m3 */

#define NT_MODEL
#define MAX_PORTC 2

typedef struct allegro_portc
{
  int speed, bits, channels;
  int open_mode;
  int audio_enabled;
  int trigger_bits;
  int audiodev;
}
allegro_portc;

typedef struct allegro_devc
{
  oss_device_t *osdev;
  oss_native_word base, mpu_base;
  int mpu_attached, fm_attached;
  uart401_devc uart401devc;
  int irq;
  int model;
#define MDL_ESS1988  1
#define MDL_ESS1989  2
#define MDL_ESS1990  3
#define MDL_ESS1992  4
#define MDL_ESS1998  5
#define MDL_ESS1999  6
#define MDL_ESS199A  7
#define MDL_ESS199B  8
  char *chip_name;

  /* Audio parameters */
  int open_mode;
  int audio_initialized;
  allegro_portc portc[MAX_PORTC];
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;

  /* buffer and device control */
  unsigned char fmt, ctrl;
  struct dmabuf
  {
    void *rawbuf;
    unsigned dmasize;
    oss_native_word base;	/* Offset for ptr */
  }
  dma_adc, dma_dac;

  oss_dma_handle_t dma_handle;

  /* Mixer parameters */
  ac97_devc ac97devc;
}
allegro_devc;

#include "port.h"
#include "hardware.h"
#include "kernel.h"
#include "srcmgr.h"

PALLEGRO_WAVE *WaveStream = NULL;
extern int allegro_mpu_ioaddr;
extern int allegro_amp;
int debug = 0;

#define ESS_CFMT_STEREO     0x01
#define ESS_CFMT_16BIT      0x02
#define ESS_CFMT_MASK       0x03
#define ESS_CFMT_ASHIFT     0
#define ESS_CFMT_CSHIFT     4

#define CTRL_DAC_EN    0x40	/* enable DAC */
#define CTRL_ADC_EN     0x10	/* enable ADC */

WORD gwDSPConnectIn = KCONNECT_ADC1;	/* Default ADC1 */

#include "allegro_util.inc"
#include "srcmgr.inc"
#include "kernel.inc"
#include "kernelbn.inc"


unsigned int
GetPosition (allegro_devc * devc, unsigned char Direction)
{
  oss_native_word flags;
  unsigned long pos = 0;

  if (!Direction)
    WaveStream = &CaptureStream;
  else
    WaveStream = &PlaybackStream;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (*WaveStream)
    pos = SRCMGR_GetPosition (devc, *WaveStream);

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return pos;
}

static int
ac97_write (void *devc_, int addr, int data)
{
  oss_native_word flags;
  allegro_devc *devc = devc_;
#if 0
  int i;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  for (i = 0; i < 1000; i++)
    if (!(INB (devc->osdev, devc->base + 0x30) & 0x01))
      break;
  OUTW (devc->osdev, data & 0xffff, devc->base + 0x32);
  oss_udelay (100);
  OUTW (devc->osdev, (addr & 0x7f) & ~0x80, devc->base + 0x30);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
#else
  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  HWMGR_WriteCodecData (devc, (UCHAR) addr, data);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
#endif
  return 0;
}

static int
ac97_read (void *devc_, int addr)
{
  allegro_devc *devc = devc_;
  oss_native_word flags;
  unsigned short data;
#if 0
  int i;
  int sanity = 10000;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
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
	  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
	  return 0;
	}
    }

  data = INW (devc->osdev, devc->base + 0x32);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
#else
  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  HWMGR_ReadCodecData (devc, (UCHAR) addr, &data);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
#endif
  return data;
}

void
HwStartDMA (allegro_devc * devc, unsigned char Direction)
{
  if (!Direction)
    WaveStream = &CaptureStream;
  else
    WaveStream = &PlaybackStream;

  SetState (devc, *WaveStream, KSSTATE_RUN);

  DDB (cmn_err (CE_WARN, "Wave%s DMA started\n", !Direction ? "In" : "Out"));
}


void
HwStopDMA (allegro_devc * devc, unsigned char Direction)
{
  if (!Direction)
    WaveStream = &CaptureStream;
  else
    WaveStream = &PlaybackStream;

  SetState (devc, *WaveStream, KSSTATE_STOP);

  DDB (cmn_err (CE_WARN, "Wave%s DMA stopped\n", !Direction ? "In" : "Out"));
}


void
HwSetWaveFormat (allegro_devc * devc, PWAVE_INFO WaveInfo,
		 unsigned char Direction)
{

  if (!Direction)
    WaveStream = &CaptureStream;
  else
    WaveStream = &PlaybackStream;

  SetFormat (devc, *WaveStream, WaveInfo);
}


static int
allegrointr (oss_device_t * osdev)
{
  allegro_devc *devc = (allegro_devc *) osdev->devc;
  unsigned char bIntStatus;
  unsigned char bIntTimer = FALSE;
  unsigned int currdac, curradc, n, i;
  int serviced = 0;

  bIntStatus = INB (devc->osdev, devc->base + 0x1A);

  if (bIntStatus == 0xff)
    return 0;
  OUTW (devc->osdev, bIntStatus, devc->base + 0x1A);
  if (bIntStatus & ASSP_INT_PENDING)
    {
      unsigned char status;
      serviced = 1;

      status = INB (devc->osdev, (devc->base + ASSP_CONTROL_B));
      if ((status & STOP_ASSP_CLOCK) == 0)
	{
	  status = INB (devc->osdev, (devc->base + ASSP_HOST_INT_STATUS));

	  /* acknowledge other interrupts */
	  if (status & DSP2HOST_REQ_TIMER)
	    {
	      OUTB (devc->osdev, DSP2HOST_REQ_TIMER,
		    (devc->base + ASSP_HOST_INT_STATUS));
	      bIntTimer = TRUE;
	    }
	}
    }

  if (bIntStatus & 0x40)
    {
      serviced = 1;
      OUTB (devc->osdev, 0x40, devc->base + 0x1A);
    }

  if (bIntStatus & MPU401_INT_PENDING)
    {
      serviced = 1;
      uart401_irq (&devc->uart401devc);
    }

  if (!bIntTimer)
    return serviced;

  for (i = 0; i < MAX_PORTC; i++)
    {
      allegro_portc *portc = &devc->portc[i];
      if (portc->trigger_bits & PCM_ENABLE_OUTPUT)
	{
	  dmap_t *dmapout = audio_engines[portc->audiodev]->dmap_out;
	  currdac = GetPosition (devc, 1);
	  currdac /= dmapout->fragment_size;
	  n = 0;
	  while (dmap_get_qhead (dmapout) != currdac && n++ < dmapout->nfrags)
	    oss_audio_outputintr (portc->audiodev, 1);
	}
      if (portc->trigger_bits & PCM_ENABLE_INPUT)
	{
	  dmap_t *dmapin = audio_engines[portc->audiodev]->dmap_in;
	  curradc = GetPosition (devc, 0);
	  curradc /= dmapin->fragment_size;

	  n = 0;
	  while (dmap_get_qtail (dmapin) != curradc && n++ < dmapin->nfrags)
	    oss_audio_inputintr (portc->audiodev, 0);
	}
    }

  return serviced;
}

static int
allegro_audio_set_rate (int dev, int arg)
{
  allegro_portc *portc = audio_engines[dev]->portc;

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
allegro_audio_set_channels (int dev, short arg)
{
  allegro_portc *portc = audio_engines[dev]->portc;

  if ((arg != 1) && (arg != 2))
    return portc->channels;
  portc->channels = arg;

  return portc->channels;
}

static unsigned int
allegro_audio_set_format (int dev, unsigned int arg)
{
  allegro_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->bits;

  if (!(arg & (AFMT_U8 | AFMT_S16_LE)))
    return portc->bits;
  portc->bits = arg;

  return portc->bits;
}

/*ARGSUSED*/
static int
allegro_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void allegro_audio_trigger (int dev, int state);

static void
allegro_audio_reset (int dev)
{
  allegro_audio_trigger (dev, 0);
}

static void
allegro_audio_reset_input (int dev)
{
  allegro_portc *portc = audio_engines[dev]->portc;
  allegro_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
allegro_audio_reset_output (int dev)
{
  allegro_portc *portc = audio_engines[dev]->portc;
  allegro_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

/*ARGSUSED*/
static int
allegro_audio_open (int dev, int mode, int open_flags)
{
  oss_native_word flags;
  allegro_portc *portc = audio_engines[dev]->portc;
  allegro_devc *devc = audio_engines[dev]->devc;

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
allegro_audio_close (int dev, int mode)
{
  allegro_portc *portc = audio_engines[dev]->portc;
  allegro_devc *devc = audio_engines[dev]->devc;

  allegro_audio_reset (dev);
  portc->open_mode = 0;
  devc->open_mode &= ~mode;
  portc->audio_enabled &= ~mode;
}

/*ARGSUSED*/
static void
allegro_audio_output_block (int dev, oss_native_word buf, int count,
			    int fragsize, int intrflag)
{
  allegro_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;

}

/*ARGSUSED*/
static void
allegro_audio_start_input (int dev, oss_native_word buf, int count,
			   int fragsize, int intrflag)
{
  allegro_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;

}

static void
allegro_audio_trigger (int dev, int state)
{
  /*oss_native_word flags; */
  allegro_portc *portc = audio_engines[dev]->portc;
  allegro_devc *devc = audio_engines[dev]->devc;

  /* MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags); */
  if (portc->open_mode & OPEN_WRITE)
    {
      if (state & PCM_ENABLE_OUTPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_OUTPUT))

	    {
	      devc->ctrl |= CTRL_DAC_EN;
	      HwStartDMA (devc, TRUE);
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
	      devc->ctrl &= ~CTRL_DAC_EN;
	      HwStopDMA (devc, TRUE);
	      FreeStream (devc, PlaybackStream);
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
	      devc->ctrl |= CTRL_ADC_EN;
	      HwStartDMA (devc, FALSE);
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
	      devc->ctrl &= ~CTRL_ADC_EN;
	      HwStopDMA (devc, FALSE);
	      FreeStream (devc, CaptureStream);
	    }
	}
    }
  /* MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags); */
}

/*ARGSUSED*/
static int
allegro_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  allegro_devc *devc = audio_engines[dev]->devc;
  allegro_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;
  WAVE_INFO WaveInfo;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  WaveInfo.BitsPerSample = portc->bits;
  WaveInfo.Channels = portc->channels;
  WaveInfo.SamplesPerSec = portc->speed;

  HwSetWaveFormat (devc, &WaveInfo, FALSE);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  portc->audio_enabled &= ~PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
  return 0;
}

/*ARGSUSED*/
static int
allegro_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  allegro_devc *devc = audio_engines[dev]->devc;
  allegro_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;
  WAVE_INFO WaveInfo;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  WaveInfo.BitsPerSample = portc->bits;
  WaveInfo.Channels = portc->channels;
  WaveInfo.SamplesPerSec = portc->speed;
  SRCMGR_SetVolume (devc, PlaybackStream->DspClientInstance, 0x7FFF, 0x7FFF);

  HwSetWaveFormat (devc, &WaveInfo, TRUE);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
  return 0;
}

static int
allegro_alloc_buffer (int dev, dmap_t * dmap, int direction)
{

  allegro_devc *devc = audio_engines[dev]->devc;

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
allegro_free_buffer (int dev, dmap_t * dmap, int direction)
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
allegro_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  allegro_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;
  oss_native_word ptr = 0;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  if (direction == PCM_ENABLE_OUTPUT)
    {
      ptr = GetPosition (devc, 1);
    }
  if (direction == PCM_ENABLE_INPUT)
    {
      ptr = GetPosition (devc, 0);
    }
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return ptr;
}


static int
alloc_dmabuf (allegro_devc * devc, int direction)
{
  struct dmabuf *db = direction ? &devc->dma_adc : &devc->dma_dac;
  unsigned char fmt = 0;
  oss_native_word phaddr;

  if (direction)
    {
      fmt &= ~((ESS_CFMT_STEREO | ESS_CFMT_16BIT) << ESS_CFMT_CSHIFT);
      fmt >>= ESS_CFMT_CSHIFT;

    }
  else
    {
      fmt &= ~((ESS_CFMT_STEREO | ESS_CFMT_16BIT) << ESS_CFMT_ASHIFT);
      fmt >>= ESS_CFMT_ASHIFT;
    }

  if (!db->rawbuf)
    {
#define MAX_REJECTED 8
      int rejected = 0;
      oss_native_word rejectedPA[MAX_REJECTED];
      oss_native_word PhysicalAddressConstraint = 0xFFFF;

      /* alloc as big a chunk as we can */
      db->dmasize = 16384;

      while (!db->rawbuf)
	{
	  oss_native_word LogicalAddress;
	  void *rawbuf;
	  rawbuf =
	    (void *) CONTIG_MALLOC (devc->osdev, db->dmasize,
				    MEMLIMIT_32BITS, &phaddr, devc->dma_handle);

	  if (!rawbuf)
	    break;
	  LogicalAddress = phaddr;
	  if (((LogicalAddress & ~PhysicalAddressConstraint) ==
	       ((LogicalAddress + 0x4000 - 1) & ~PhysicalAddressConstraint)))
	    {
	      db->rawbuf = rawbuf;
	    }
	  else
	    {
	      if (rejected == MAX_REJECTED)
		{
		  CONTIG_FREE (devc->osdev, rawbuf, db->dmasize, devc->dma_handle);
		  break;
		}
	      rejectedPA[rejected] = (oss_native_word) rawbuf;
	      rejected++;
	    }
	}

      while (rejected--)
	{
	  CONTIG_FREE (devc->osdev, (char *) rejectedPA[rejected],
		       db->dmasize, TODO);
	}

      if (!db->rawbuf)
	return OSS_ENOMEM;

#ifdef linux
      /* now mark the pages as reserved; otherwise remap_page_range doesn't do what we want */
      oss_reserve_pages ((oss_native_word) db->rawbuf,
			 (oss_native_word) db->rawbuf + (PAGE_SIZE << 2) - 1);
#endif
    }
  memset (db->rawbuf, (fmt & ESS_CFMT_16BIT) ? 0 : 0x80, db->dmasize);
  db->base = phaddr;
  return 0;
}

static int
free_dmabuf (allegro_devc * devc, unsigned direction)
{
  struct dmabuf *db;

  if (direction)
    db = &devc->dma_dac;
  else
    db = &devc->dma_adc;

  if (db->rawbuf)
    {
#ifdef linux
      /* undo marking the pages as reserved */
      oss_unreserve_pages ((oss_native_word) db->rawbuf,
			   (oss_native_word) db->rawbuf + (PAGE_SIZE << 2) -
			   1);
#endif
      CONTIG_FREE (devc->osdev, db->rawbuf, db->dmasize, TODO);
      db->rawbuf = NULL;
      return 0;
    }
  return OSS_EIO;
}

static audiodrv_t allegro_audio_driver = {
  allegro_audio_open,
  allegro_audio_close,
  allegro_audio_output_block,
  allegro_audio_start_input,
  allegro_audio_ioctl,
  allegro_audio_prepare_for_input,
  allegro_audio_prepare_for_output,
  allegro_audio_reset,
  NULL,
  NULL,
  allegro_audio_reset_input,
  allegro_audio_reset_output,
  allegro_audio_trigger,
  allegro_audio_set_rate,
  allegro_audio_set_format,
  allegro_audio_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  allegro_alloc_buffer,
  allegro_free_buffer,
  NULL,
  NULL,
  allegro_get_buffer_pointer
};

#if 0
static void
attach_fm (allegro_devc * devc)
{
  if (!opl3_detect (0x388, devc->osdev))
    return;
  opl3_init (0x388, devc->osdev);
  devc->fm_attached = 1;
}
#endif

static void
attach_mpu (allegro_devc * devc)
{
  devc->mpu_attached = 1;
  uart401_init (&devc->uart401devc, devc->osdev, devc->mpu_base,
		"Allegro MIDI UART");
}

static int
init_allegro (allegro_devc * devc)
{
  int my_mixer, adev;
  int first_dev = 0;
  int ret, i;
  unsigned short val;

  devc->mpu_attached = devc->fm_attached = 0;

  ret = alloc_dmabuf (devc, 0);
  if (ret != 0)
    {
      cmn_err (CE_WARN, "Couldn't allocate playback memory\n");
      return 1;
    }

  ret = alloc_dmabuf (devc, 1);
  if (ret != 0)
    {
      cmn_err (CE_WARN, "Couldn't allocate recording memory\n");
      return 1;
    }

  AllocateStream (devc, WAVE_CAPTURE);
  AllocateStream (devc, WAVE_PLAYBACK);

#if 0
  for (i = devc->base + 0x1c; i <= devc->base + 0x1f; i++)
    OUTB (devc->osdev, 0x00, i);
  OUTB (devc->osdev, 0x40, devc->base + 0x38);
  OUTB (devc->osdev, 0x10, devc->base + 0x3e);
  OUTB (devc->osdev, 0x44, devc->base + 0x3f);
#endif

  HWMGR_InitSystem (devc);
  OUTW (devc->osdev, 0x0012, devc->base + 0x18);

  /* Once enable it, never touch again */
  OUTB (devc->osdev, (INB (devc->osdev, devc->base + 0xA6) | 0x01),
	devc->base + 0xA6);

  pci_read_config_word (devc->osdev, PCI_ALLEGRO_CONFIG, &val);
  switch (devc->mpu_base)
    {
    case 0x330:
      val |= 0;
      break;
    case 0x300:
      val |= (1 << 3);
      break;
    case 0x320:
      val |= (2 << 3);
      break;
    case 0x340:
      val |= (3 << 3);
      break;
    }
  pci_write_config_word (devc->osdev, PCI_ALLEGRO_CONFIG, val);

#if 0
  attach_fm (devc);
#endif

  if (devc->mpu_base > 0)
    attach_mpu (devc);


  InitStream ();

  if (allegro_amp)
    {
      /*initialize the GPIOs....this is hacked using Windows settings */
      OUTB (devc->osdev, 0xff, devc->base + 0x61);
      oss_udelay (100);

      OUTB (devc->osdev, 0xff, devc->base + 0x63);
      oss_udelay (100);

      OUTB (devc->osdev, 0xff, devc->base + 0x67);
      oss_udelay (100);
      OUTB (devc->osdev, 0xff, devc->base + 0x69);
    }

  my_mixer = ac97_install (&devc->ac97devc, "AC97 Mixer",
			   ac97_read, ac97_write, devc, devc->osdev);
  if (my_mixer < 0)
    return 0;

  for (i = 0; i < MAX_PORTC; i++)
    {
      int caps = ADEV_AUTOMODE;
      allegro_portc *portc = &devc->portc[i];
      char tmp_name[256];

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
					&allegro_audio_driver,
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
	  audio_engines[adev]->min_rate = 5000;
	  audio_engines[adev]->max_rate = 48000;
	  audio_engines[adev]->caps |= PCM_CAP_FREERATE;
	  audio_engines[adev]->vmix_flags = VMIX_MULTIFRAG;
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
oss_allegro_attach (oss_device_t * osdev)
{
  unsigned char pci_irq_line, pci_revision;
  unsigned short pci_command, vendor, device;
  unsigned int pci_ioaddr;
  int err;
  allegro_devc *devc;
  DDB (cmn_err (CE_WARN, "Entered ALLEGRO ALLEGRO probe routine\n"));

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  if (vendor != ESSM3_VENDOR_ID ||
      (device != ESS_1988 && device != ESS_1989 && device != ESS_1990 &&
       device != ESS_1992 && device != ESS_1998 && device != ESS_1999 &&
       device != ESS_199A && device != ESS_199B))

    return 0;

  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_0, &pci_ioaddr);

  DDB (cmn_err (CE_WARN, "ALLEGRO I/O base %04x\n", pci_ioaddr));

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
  devc->mpu_base = allegro_mpu_ioaddr;

  devc->base = MAP_PCI_IOADDR (devc->osdev, 0, pci_ioaddr);
  /* Remove I/O space marker in bit 0. */
  devc->base &= ~3;

  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_IO;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  /* Enable Game port/MPU401 */
  pci_read_config_word (osdev, PCI_LEGACY_AUDIO_CTRL, &pci_command);
  pci_command = GAME_PORT_ENABLE | MPU401_IO_ENABLE | MPU401_IRQ_ENABLE;
  pci_write_config_word (osdev, PCI_LEGACY_AUDIO_CTRL, pci_command);

  switch (device)
    {
    case ESS_1988:
      devc->model = MDL_ESS1998;
      devc->chip_name = "ESS Allegro (ESS1998)";
      bChipType = A1_1988;
      break;
    case ESS_1989:
      devc->model = MDL_ESS1989;
      devc->chip_name = "ESS Allegro (ESS1989)";
      bChipType = A1_1989;
      break;
    case ESS_1990:
      devc->model = MDL_ESS1990;
      devc->chip_name = "ESS Canyon 3D (ESS1990)";
      break;
    case ESS_1992:
      devc->model = MDL_ESS1992;
      devc->chip_name = "ESS Canyon 3D-II (ESS1992)";
      break;
    case ESS_1998:
      devc->model = MDL_ESS1998;
      devc->chip_name = "ESS Maestro3 (ESS1998)";
      bChipType = M3_1998;
      break;
    case ESS_1999:
      devc->model = MDL_ESS1999;
      devc->chip_name = "ESS Maestro3 (ESS1999)";
      bChipType = M3_1999;
      break;
    case ESS_199A:
      devc->model = MDL_ESS199A;
      devc->chip_name = "ESS Maestro3 (ESS199A)";
      bChipType = M3_199A;
      break;
    case ESS_199B:
      devc->model = MDL_ESS199B;
      devc->chip_name = "ESS Maestro3 (ESS199B)";
      bChipType = M3_199B;
      break;

    }

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);

  oss_register_device (osdev, devc->chip_name);

  if ((err = oss_register_interrupts (devc->osdev, 0, allegrointr, NULL)) < 0)
    {
      cmn_err (CE_WARN, "Can't allocate IRQ%d, err=%d\n", pci_irq_line, err);
      return 0;
    }

  return init_allegro (devc);	/* Detected */
}


int
oss_allegro_detach (oss_device_t * osdev)
{
  allegro_devc *devc = (allegro_devc *) osdev->devc;
  int ret;

  if (oss_disable_device (osdev) < 0)
    return 0;


  OUTW (devc->osdev, 0x00, devc->base + 0x18);

  if (devc->mpu_attached)
    {
      uart401_disable (&devc->uart401devc);
      devc->mpu_attached = 0;
    }

  oss_unregister_interrupts (devc->osdev);

  ret = free_dmabuf (devc, 0);	/* free playback mem */
  if (ret != 0)
    {
      cmn_err (CE_WARN, "Couldn't free playback memory\n");
      return 0;
    }

  ret = free_dmabuf (devc, 1);	/* free record mem */
  if (ret != 0)
    {
      cmn_err (CE_WARN, "Couldn't free record memory\n");
      return 0;
    }

  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);
  UNMAP_PCI_IOADDR (devc->osdev, 0);

  oss_unregister_device (osdev);
  return 1;
}
