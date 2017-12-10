/*
 * uPurpose: Driver for the VIA VT82C686A AC97 audio controller
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

#include "oss_via97_cfg.h"
#include "oss_pci.h"
#include "ac97.h"

#define VIA_VENDOR_ID		0x1106
#define VIA_82C686			0x3058

#define CODEC_TIMEOUT_COUNT     500
#define AC97CODEC       0x80	/*Access AC97 Codec */
#define IN_CMD        0x01000000	/*busy in sending */
#define STA_VALID     0x02000000	/*1:status data is valid */
#define CODEC_RD      0x00800000	/*Read CODEC status */
#define CODEC_INDEX   0x007F0000	/*Index of command register to access */
#define CODEC_DATA    0x0000FFFF	/*AC97 status register data */

typedef struct
{
  unsigned int phaddr;
  unsigned int flags;
}
SGD_entry;

typedef struct
{
  int open_mode;
  int speed, bits, channels;
  int audiodev;
  int audio_enabled;
  int trigger_bits;
}
via97_portc;

typedef struct via97_devc
{
  oss_device_t *osdev;
  oss_native_word base;
  int irq;
  int open_mode;
  char *chip_name;
/*
 * Mixer
 */
  ac97_devc ac97devc;

/*
 *  MIDI
 */

  int mpu_base;
  int mpu_attached;

  /* Audio parameters */
  via97_portc portc[2];
  oss_mutex_t mutex;		/* For normal locking */
  oss_mutex_t low_mutex;	/* For low level routines */

  /* Memory allocation and scatter/gather info */
#define BUFFER_SIZE		(128*1024)

/* NOTE! Check SGD_ALLOC if changing SGD_SIZE */
#define MIN_BLOCK		64
#define SGD_SIZE		(BUFFER_SIZE/MIN_BLOCK)
#define SGD_TOTAL_SIZE		(2*SGD_SIZE)
#define SGD_ALLOC		(SGD_TOTAL_SIZE*8)

  SGD_entry *SGD_table;
  oss_native_word SGD_table_phys;
  int play_sgd_ptr, rec_sgd_ptr;
  oss_dma_handle_t sgd_dma_handle;
  unsigned int play_sgd_phys, rec_sgd_phys;
}
via97_devc;

static int
ac97_read (void *devc_, int wIndex)
{
  oss_native_word dwWriteValue = 0, dwTmpValue;
  unsigned int i = 0;
  oss_native_word flags;
  via97_devc *devc = devc_;

  /* Index has only 7 bit */
  if (wIndex > 0x7F)
    return 0;
  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  dwWriteValue = ((oss_native_word) wIndex << 16) + CODEC_RD;
  OUTL (devc->osdev, dwWriteValue, devc->base + AC97CODEC);
  oss_udelay (100);
  /* Check AC CODEC access time out */
  for (i = 0; i < CODEC_TIMEOUT_COUNT; i++)
    {
      /* if send command over, break */
      if (INL (devc->osdev, devc->base + AC97CODEC) & STA_VALID)
	break;
      oss_udelay (50);
    }
  if (i == CODEC_TIMEOUT_COUNT)
    {
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return 0;
    }

  /* Check if Index still ours? If yes, return data, else return FAIL */
  dwTmpValue = INL (devc->osdev, devc->base + AC97CODEC);
  OUTB (devc->osdev, 0x02, devc->base + AC97CODEC + 3);
  if (((dwTmpValue & CODEC_INDEX) >> 16) == wIndex)
    {
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return ((int) dwTmpValue & CODEC_DATA);
    }
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return 0;

}

static int
ac97_write (void *devc_, int wIndex, int wData)
{
  oss_native_word dwWriteValue = 0;
  unsigned int i = 0;
  oss_native_word flags;
  via97_devc *devc = devc_;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  dwWriteValue = ((oss_native_word) wIndex << 16) + wData;
  OUTL (devc->osdev, dwWriteValue, devc->base + AC97CODEC);
  oss_udelay (100);

  /* Check AC CODEC access time out */
  for (i = 0; i < CODEC_TIMEOUT_COUNT; i++)
    {
      /* if send command over, break */
      if (!(INL (devc->osdev, devc->base + AC97CODEC) & IN_CMD))
	break;
      oss_udelay (50);
    }
  if (i == CODEC_TIMEOUT_COUNT)
    {
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return (-1);
    }
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return 1;
}

static int
via97intr (oss_device_t * osdev)
{
  via97_devc *devc = (via97_devc *) osdev->devc;
  via97_portc *portc;
  unsigned char status;
  int serviced = 0;
  int i;

#ifdef OBSOLETED_STUFF
  if (devc->mpu_attached)
    {
      if (uart401intr (INT_HANDLER_CALL (devc->irq)))
	serviced = 1;
    }
#endif

  /* Handle playback interrupt */
  status = INB (devc->osdev, devc->base + 0x00);

  if (status & 0x01)
    {
      serviced = 1;
      for (i = 0; i < 2; i++)
	{
	  portc = &devc->portc[i];
	  /* IOC Interrupt */
	  if ((portc->trigger_bits & PCM_ENABLE_OUTPUT) && (status & 0x01))
	    oss_audio_outputintr (portc->audiodev, 0);
	}
    }
  OUTB (devc->osdev, status | 0x01, devc->base + 0x00);


  /* Handle record interrupt */
  status = INB (devc->osdev, devc->base + 0x10);

  if (status & 0x01)
    {
      serviced = 1;
      for (i = 0; i < 2; i++)
	{
	  portc = &devc->portc[i];
	  /* IOC Interrupt */
	  if ((portc->trigger_bits & PCM_ENABLE_INPUT) && (status & 0x01))
	    oss_audio_inputintr (portc->audiodev, 0);
	}
    }

  OUTB (devc->osdev, status | 0x01, devc->base + 0x10);
  return serviced;
}

/*
 * Audio routines
 */

static int
via97_audio_set_rate (int dev, int arg)
{
  via97_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->speed;

  if (audio_engines[dev]->flags & ADEV_FIXEDRATE)
    arg = 48000;

  if (arg > 48000)
    arg = 48000;
  if (arg < 5000)
    arg = 5000;
  portc->speed = arg;
  return portc->speed;
}

static short
via97_audio_set_channels (int dev, short arg)
{
  via97_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->channels;


  if ((arg != 1) && (arg != 2))
    return portc->channels = 2;
  portc->channels = arg;

  return portc->channels;
}

static unsigned int
via97_audio_set_format (int dev, unsigned int arg)
{
  via97_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->bits;

  if (!(arg & (AFMT_U8 | AFMT_S16_LE)))
    return portc->bits = AFMT_S16_LE;
  portc->bits = arg;

  return portc->bits;
}

/*ARGSUSED*/
static int
via97_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void via97_audio_trigger (int dev, int state);

static void
via97_audio_reset (int dev)
{
  via97_audio_trigger (dev, 0);
}

static void
via97_audio_reset_input (int dev)
{
  via97_portc *portc = audio_engines[dev]->portc;
  via97_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
via97_audio_reset_output (int dev)
{
  via97_portc *portc = audio_engines[dev]->portc;
  via97_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

/*ARGSUSED*/
static int
via97_audio_open (int dev, int mode, int open_flags)
{
  via97_portc *portc = audio_engines[dev]->portc;
  via97_devc *devc = audio_engines[dev]->devc;
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
via97_audio_close (int dev, int mode)
{
  via97_portc *portc = audio_engines[dev]->portc;
  via97_devc *devc = audio_engines[dev]->devc;

  via97_audio_reset (dev);
  portc->open_mode = 0;
  devc->open_mode &= ~mode;
  portc->audio_enabled &= ~mode;
}

/*ARGSUSED*/
static void
via97_audio_output_block (int dev, oss_native_word buf, int count,
			  int fragsize, int intrflag)
{
  via97_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;

}

/*ARGSUSED*/
static void
via97_audio_start_input (int dev, oss_native_word buf, int count,
			 int fragsize, int intrflag)
{
  via97_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;

}

static void
via97_audio_trigger (int dev, int state)
{
  via97_devc *devc = audio_engines[dev]->devc;
  via97_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode & OPEN_WRITE)
    {
      if (state & PCM_ENABLE_OUTPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      OUTB (devc->osdev, 0x80, devc->base + 0x01);
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
	      OUTB (devc->osdev, 0x40, devc->base + 0x01);
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

	      OUTB (devc->osdev, 0x80, devc->base + 0x11);
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
	      OUTB (devc->osdev, 0x40, devc->base + 0x11);
	    }
	}
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
static int
via97_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  via97_devc *devc = audio_engines[dev]->devc;
  via97_portc *portc = audio_engines[dev]->portc;
  int i, sgd_ptr;
  dmap_t *dmap = audio_engines[dev]->dmap_in;
  unsigned char tmp;
  oss_native_word flags;

  if (audio_engines[dev]->dmap_in->dmabuf_phys == 0)
    return OSS_ENOSPC;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  tmp = 0x81;			/* Auto start at EOL, interrupt on FLAG */

  if (portc->bits != AFMT_U8)
    tmp |= 0x20;
  if (portc->channels != 1)
    tmp |= 0x10;
  OUTB (devc->osdev, tmp, devc->base + 0x12);

  ac97_recrate (&devc->ac97devc, portc->speed);

  sgd_ptr = devc->rec_sgd_ptr;
  for (i = 0; i < dmap->nfrags; i++)
    {
      if (sgd_ptr >= SGD_TOTAL_SIZE)
	{
	  cmn_err (CE_WARN, "Out of Record SGD entries\n");
	  return OSS_ENOSPC;
	}

      devc->SGD_table[sgd_ptr].phaddr =
	dmap->dmabuf_phys + (i * dmap->fragment_size);
      devc->SGD_table[sgd_ptr].flags = 0x40000000 | dmap->fragment_size;

      sgd_ptr++;
    }

  devc->SGD_table[sgd_ptr - 1].flags |= 0x80000000;	/* EOL */
  devc->rec_sgd_phys =
    devc->SGD_table_phys + devc->rec_sgd_ptr * sizeof (SGD_entry);
  OUTL (devc->osdev, devc->rec_sgd_phys, devc->base + 0x14);
  portc->audio_enabled &= ~PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/
static int
via97_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  via97_devc *devc = audio_engines[dev]->devc;
  via97_portc *portc = audio_engines[dev]->portc;
  int i, sgd_ptr;
  dmap_t *dmap = audio_engines[dev]->dmap_out;
  unsigned char tmp;
  oss_native_word flags;

  if (audio_engines[dev]->dmap_out->dmabuf_phys == 0)
    return OSS_ENOSPC;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  tmp = 0x81;			/* Auto start at EOL, interrupt on FLAG */

  if (portc->bits != AFMT_U8)
    tmp |= 0x20;
  if (portc->channels != 1)
    tmp |= 0x10;
  OUTB (devc->osdev, tmp, devc->base + 0x02);

  ac97_playrate (&devc->ac97devc, portc->speed);

  sgd_ptr = devc->play_sgd_ptr;

  for (i = 0; i < dmap->nfrags; i++)
    {
      if (sgd_ptr >= SGD_TOTAL_SIZE)
	{
	  cmn_err (CE_WARN, "Out of Playback SGD entries\n");
	  return OSS_ENOSPC;
	}

      devc->SGD_table[sgd_ptr].phaddr =
	dmap->dmabuf_phys + (i * dmap->fragment_size);
      devc->SGD_table[sgd_ptr].flags = 0x40000000 | dmap->fragment_size;

      sgd_ptr++;
    }

  devc->SGD_table[sgd_ptr - 1].flags |= 0x80000000;	/* EOL */
  devc->play_sgd_phys =
    devc->SGD_table_phys + devc->play_sgd_ptr * sizeof (SGD_entry);
  OUTL (devc->osdev, devc->play_sgd_phys, devc->base + 0x04);

  portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

static int
via97_alloc_buffer (int dev, dmap_t * dmap, int direction)
{
  int err;

  if (dmap->dmabuf != NULL)
    return 0;
  if ((err = oss_alloc_dmabuf (dev, dmap, direction)) < 0)
    return err;

  if (dmap->buffsize > BUFFER_SIZE)
    {
      cmn_err (CE_WARN, "Too large DMA buffer (%d/%d) - truncated\n",
	       dmap->buffsize, BUFFER_SIZE);
      dmap->buffsize = BUFFER_SIZE;
    }

  return 0;
}

static int
via97_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
/*
 * Unfortunately the VIA chip seems to raise interrupt about 32 bytes before
 * the DMA pointer moves to a new fragment. This means that the bytes value
 * returned by SNDCTL_DSP_GET[]PTR will be bogus during few samples before
 * the pointer wraps back to the beginning of buffer.
 *
 * If mmap() is not being used this routine will return 0 during this period.
 */
  via97_devc *devc = audio_engines[dev]->devc;
  unsigned int ptr, sgd;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);

  if (direction == PCM_ENABLE_OUTPUT)
    {
      ptr = INL (devc->osdev, devc->base + 0x0c) & 0xffffff;
      sgd =
	((INL (devc->osdev, devc->base + 0x04) - devc->play_sgd_phys) / 8) -
	1;

      ptr = dmap->fragment_size - ptr;
      ptr = ptr + (sgd * dmap->fragment_size);
//cmn_err(CE_CONT, "%d/%d\n", sgd, ptr);
    }
  else
    {
      ptr = INL (devc->osdev, devc->base + 0x1c) & 0xffffff;
      sgd =
	((INL (devc->osdev, devc->base + 0x14) - devc->rec_sgd_phys) / 8) - 1;

      ptr = dmap->fragment_size - ptr;
      ptr = ptr + (sgd * dmap->fragment_size);
    }

  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return ptr;
}

static const audiodrv_t via97_audio_driver = {
  via97_audio_open,
  via97_audio_close,
  via97_audio_output_block,
  via97_audio_start_input,
  via97_audio_ioctl,
  via97_audio_prepare_for_input,
  via97_audio_prepare_for_output,
  via97_audio_reset,
  NULL,
  NULL,
  via97_audio_reset_input,
  via97_audio_reset_output,
  via97_audio_trigger,
  via97_audio_set_rate,
  via97_audio_set_format,
  via97_audio_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  via97_alloc_buffer,
  NULL,				/* via97_free_buffer, */
  NULL,
  NULL,
  via97_get_buffer_pointer
};

#ifdef OBSOLETED_STUFF
/*
 * This device has "ISA style" MIDI and FM subsystems. Such devices don't
 * use PCI config space for the I/O ports and interrupts. Instead the driver
 * needs to allocate proper resources itself. This functionality is no longer
 * possible. For this reason the MIDI and FM parts are not accessible.
 */
static void
attach_mpu (via97_devc * devc)
{
  struct address_info hw_config;

  hw_config.io_base = devc->mpu_base;
  hw_config.irq = -devc->irq;
  hw_config.dma = -1;
  hw_config.dma2 = -1;
  hw_config.always_detect = 0;
  hw_config.name = "VIA97 external MIDI";
  hw_config.driver_use_1 = 0;
  hw_config.driver_use_2 = 0;
  hw_config.osdev = devc->osdev;
#ifdef CREATE_OSP
  CREATE_OSP (hw_config.osdev);
#endif
  hw_config.card_subtype = 0;
#if 1
  if (!probe_uart401 (&hw_config))
    {
      cmn_err (CE_WARN, "MPU-401 was not detected\n");
      return;
    }
#endif
  DDB (cmn_err (CE_WARN, "MPU-401 detected - Good\n"));
  attach_uart401 (&hw_config);
  devc->mpu_attached = 1;
}

static void
unload_mpu (via97_devc * devc)
{
  struct address_info hw_config;

  if (devc == NULL || !devc->mpu_attached)
    return;

  devc->mpu_attached = 0;
  hw_config.io_base = devc->mpu_base;
  hw_config.irq = devc->irq;
  hw_config.dma = -1;
  hw_config.dma2 = -1;
  hw_config.always_detect = 0;
  hw_config.name = "VIA97";
  hw_config.driver_use_1 = 0;
  hw_config.driver_use_2 = 0;
  hw_config.osdev = devc->osdev;
#ifdef CREATE_OSP
  CREATE_OSP (hw_config.osdev);
#endif
  hw_config.card_subtype = 0;

  unload_uart401 (&hw_config);

}
#endif

static int
init_via97 (via97_devc * devc)
{
  int my_mixer, adev, opts, i;
  int first_dev = 0;
  oss_native_word phaddr;


/*
 * Allocate the SGD buffers
 */

  if (devc->SGD_table == NULL)
    {
      devc->SGD_table =
	CONTIG_MALLOC (devc->osdev, SGD_ALLOC, MEMLIMIT_32BITS, &phaddr, devc->sgd_dma_handle);

      if (devc->SGD_table == NULL)
	return OSS_ENOSPC;
      devc->SGD_table_phys = phaddr;
    }

  /*
   * Allocate SGD entries for recording and playback.
   */
  devc->rec_sgd_ptr = 0;
  devc->play_sgd_ptr = SGD_SIZE;
/*
 * Init mixer
 */
  my_mixer =
    ac97_install (&devc->ac97devc, "VIA82C686 AC97 Mixer", ac97_read,
		  ac97_write, devc, devc->osdev);
  if (my_mixer == -1)
    return 0;			/* No mixer */

  mixer_devs[my_mixer]->priority = 10;	/* Known motherboard device */

  /* enable variable rate */
  ac97_write (devc, 0x2a, 0x01);

#ifdef OBSOLETED_STUFF
  DDB (cmn_err (CE_WARN, "Probing UART401 at 0x%x\n", devc->mpu_base));
  attach_mpu (devc);
#endif

  for (i = 0; i < 2; i++)
    {
      via97_portc *portc = &devc->portc[i];
      char tmp_name[100];

      opts = ADEV_AUTOMODE;

      if (!ac97_varrate (&devc->ac97devc))
	{
	  opts |= ADEV_FIXEDRATE;
	}

      if (i == 0)
	{
	  opts |= ADEV_DUPLEX;
	  strcpy (tmp_name, "VIA 82C686 AC97 Controller");
	}
      else
	{
	  opts |= ADEV_DUPLEX | ADEV_SHADOW;
	  strcpy (tmp_name, "VIA 82C686 AC97 Controller");
	}

      if ((adev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
					devc->osdev,
					devc->osdev,
					tmp_name,
					&via97_audio_driver,
					sizeof (audiodrv_t),
					opts,
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
	  audio_engines[adev]->min_rate = 8000;
	  audio_engines[adev]->max_rate = 48000;
	  audio_engines[adev]->caps |= PCM_CAP_FREERATE;
	  portc->open_mode = 0;
	  portc->audio_enabled = 0;
	  portc->audiodev = adev;
	  audio_engines[adev]->min_block = MIN_BLOCK;
	  audio_engines[adev]->max_block = 4 * 1024;
	  audio_engines[adev]->max_fragments = SGD_SIZE;
	  if (opts & ADEV_FIXEDRATE)
	    {
	      audio_engines[adev]->fixed_rate = 48000;
	      audio_engines[adev]->min_rate = 48000;
	      audio_engines[adev]->max_rate = 48000;
	    }
#ifdef CONFIG_OSS_VMIX
	  if (i == 0)
	     vmix_attach_audiodev(devc->osdev, adev, -1, 0);
#endif
	}
    }
  return 1;
}

int
oss_via97_attach (oss_device_t * osdev)
{
  unsigned char tmp, pci_irq_line, pci_revision /*, pci_latency */ ;
  unsigned short pci_command, vendor, device;
  unsigned int pci_ioaddr;
  int mpu_base;
  via97_devc *devc;

  DDB (cmn_err (CE_WARN, "Entered VT82C686 probe routine\n"));

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  if (vendor != VIA_VENDOR_ID || device != VIA_82C686)
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
  devc->chip_name = "VIA VT82C686";

  /* Enable codec, etc */

  pci_write_config_byte (osdev, 0x41, 0xc0);
  oss_udelay (10);
  pci_read_config_byte (osdev, 0x41, &tmp);
  pci_write_config_byte (osdev, 0x41, tmp | 0x0c);
  oss_udelay (10);

  /* setup game port/MIDI */
  pci_write_config_byte (osdev, 0x42, 0x2a);
  /* disable FM io */
  pci_write_config_byte (osdev, 0x48, 0x00);

  /* Enable interrupt on FLAG and on EOL */
  tmp = INB (devc->osdev, devc->base + 0x22);
  OUTB (devc->osdev, tmp | 0x83, devc->base + 0x22);


  /* Enable MPU401 */
  pci_read_config_byte (osdev, 0x8, &tmp);
  if ((tmp & 0xff) >= 0x20)
    {
      pci_read_config_byte (osdev, 0x42, &tmp);
      pci_write_config_byte (osdev, 0x42, tmp & 0x3f);
    }

  pci_read_config_byte (osdev, 0x43, &tmp);
  switch ((tmp & 0x0c) >> 2)
    {
    case 0:
      mpu_base = 0x300;
      break;
    case 1:
      mpu_base = 0x310;
      break;
    case 2:
      mpu_base = 0x320;
      break;
    default:
      mpu_base = 0x330;
      break;
    }

  /* map PCI IO address space */
  devc->base = MAP_PCI_IOADDR (devc->osdev, 0, pci_ioaddr);
  /* Remove I/O space marker in bit 0. */
  devc->base &= ~0x3;

  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_IO;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  devc->irq = pci_irq_line;
  devc->mpu_base = mpu_base;
  devc->SGD_table = NULL;
  devc->mpu_attached = 0;
  devc->open_mode = 0;

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);

  oss_register_device (osdev, devc->chip_name);

  if (oss_register_interrupts (devc->osdev, 0, via97intr, NULL) < 0)
    {
      cmn_err (CE_WARN, "Unable to register interrupts\n");
      return 0;
    }

  return init_via97 (devc);	/* Detected */
}


int
oss_via97_detach (oss_device_t * osdev)
{
  via97_devc *devc = (via97_devc *) osdev->devc;


  if (oss_disable_device (devc->osdev) < 0)
    return 0;

  OUTB (devc->osdev, 0x40, devc->base + 0x01);
  OUTB (devc->osdev, 0x40, devc->base + 0x11);
  OUTB (devc->osdev, 0, devc->base + 0x02);
  OUTB (devc->osdev, 0, devc->base + 0x12);
  OUTL (devc->osdev, 0, devc->base + 0x04);
  OUTL (devc->osdev, 0, devc->base + 0x14);
  OUTL (devc->osdev, 0, devc->base + 0x22);
  oss_udelay (30);

#ifdef OBSOLETED_STUFF
  if (devc->mpu_attached)
    unload_mpu (devc);
#endif

  oss_unregister_interrupts (devc->osdev);

  if (devc->SGD_table != NULL)
    {
      CONTIG_FREE (devc->osdev, devc->SGD_table, SGD_ALLOC, devc->sgd_dma_handle);
      devc->SGD_table = NULL;
    }

  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);
  UNMAP_PCI_IOADDR (devc->osdev, 0);

  oss_unregister_device (devc->osdev);
  return 1;
}
