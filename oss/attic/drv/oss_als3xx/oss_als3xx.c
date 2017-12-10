/*
 * Purpose: Driver for ALS ALS300 PCI audio controller.
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

#include "oss_als3xx_cfg.h"
#include "oss_pci.h"
#include "ac97.h"
#include "als300.h"

#define ALS_VENDOR_ID	0x4005
#define ALS_300		0x0300
#define ALS_300P	0x0308

extern int als300_mpu_base;

#define MAX_PORTC 2

typedef struct als300_portc
{
  int speed, bits, channels;
  int open_mode;
  int audio_enabled;
  int trigger_bits;
  int audiodev;
}
als300_portc;

typedef struct als300_devc
{
  oss_device_t *osdev;
  oss_native_word base, mpu_base;
  int mpu_attached, fm_attached;
  int irq;

  volatile unsigned char intr_mask;
#define MDL_ALS300		0
#define MDL_ALS300PLUS          1
  int model;
  char *chip_name;
  unsigned char chip_rev;
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;

  /* Audio parameters */
  int open_mode;
  als300_portc portc[2];
  oss_native_word srcode;

  /* Mixer parameters */
  ac97_devc ac97devc;
  int mixer_dev;
}
als300_devc;


void
gcr_writel (als300_devc * devc, unsigned char index, oss_native_word data)
{
  OUTB (devc->osdev, index, devc->base + 0x0c);
  OUTL (devc->osdev, data, devc->base + 0x08);
}


oss_native_word
gcr_readl (als300_devc * devc, unsigned char index)
{
  oss_native_word bufl;
  OUTB (devc->osdev, index, devc->base + 0x0c);
  bufl = INL (devc->osdev, devc->base + 0x08);
  return (bufl);
}

static int
ac97_write (void *devc_, int index, int data)
{
  oss_native_word access;
  unsigned i, N;
  unsigned char byte;
  als300_devc *devc = devc_;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  i = 0;
  N = 1000;
  do
    {
      byte = INB (devc->osdev, devc->base + 6) & 0x80;
      if (byte == 0x00)
	goto go;
      oss_udelay (10);
      i++;
    }
  while (i < N);
  if (i >= N)
    cmn_err (CE_WARN, "\n Write AC97 mixer index time out !!");
go:
  access = index;
  access <<= 24;		/*index */
  access &= 0x7fffffff;		/*write */
  access |= data;		/*data */
  OUTL (devc->osdev, access, devc->base);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return 0;
}

static int
ac97_read (void *devc_, int index)
{
  oss_native_word access;
  unsigned int data;
  unsigned i, N;
  unsigned char byte;
  oss_native_word flags;
  als300_devc *devc = devc_;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  i = 0;
  N = 1000;
  do
    {
      byte = INB (devc->osdev, devc->base + 6) & 0x80;
      if (byte == 0x00)
	goto next;
      oss_udelay (10);
      i++;
    }
  while (i < N);
  if (i >= N)
    cmn_err (CE_WARN, "\n Write AC97 mixer index time out !!");
next:
  access = index;
  access <<= 24;		/*index */
  access |= 0x80000000;
  OUTL (devc->osdev, access, devc->base);

  i = 0;
  N = 1000;
  do
    {
      byte = INB (devc->osdev, devc->base + 6);
      if ((byte & 0x40) != 0)
	goto next1;
      oss_udelay (10);
      i++;
    }
  while (i < N);
  if (i >= N)
    cmn_err (CE_WARN, "Read AC97 mixer data time out !!");
next1:
  data = INW (devc->osdev, devc->base + 0x04);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return data;
}

static int
als300intr (oss_device_t * osdev)
{
  als300_devc *devc = (als300_devc *) osdev->devc;
  int i;
  unsigned int status, mpustatus, serviced = 0;

  /* Get the status */
  if (devc->model == MDL_ALS300)
    status = INB (devc->osdev, devc->base + 0x07);
  else
    status = INB (devc->osdev, devc->base + 0xF);

  /* if playback - do output */
  for (i = 0; i < MAX_PORTC; i++)
    {
      als300_portc *portc = &devc->portc[i];

      if (status & 0x08)
	{
	  serviced = 1;
	  if (portc->trigger_bits & PCM_ENABLE_OUTPUT)
	    oss_audio_outputintr (portc->audiodev, 1);
	}

      /* if record - do input */
      if (status & 0x04)
	{
	  serviced = 1;
	  if (portc->trigger_bits & PCM_ENABLE_INPUT)
	    oss_audio_inputintr (portc->audiodev, 0);
	}
    }

  /* Handle the UART401 Interrupt */
  if (devc->model == MDL_ALS300)
    {
      mpustatus = INB (devc->osdev, devc->base + 0x0E);
      if ((mpustatus & 0x10) || (status & 0x80))
	{
	  serviced = 1;
/*	      uart401intr (INT_HANDLER_CALL (devc->irq)); */
	}
    }
  else
    {
      if (status & 0x40)
	{
	  serviced = 1;
/*	      uart401intr (INT_HANDLER_CALL (devc->irq)); */
	}
    }

  /* Acknowledge the interrupt */
  if (status)
    {
      if (devc->model == MDL_ALS300)
	{
	  OUTB (devc->osdev, status, devc->base + 0x07);	/* acknowledge interrupt */
	}
      else
	{
	  OUTB (devc->osdev, status, devc->base + 0xF);	/* acknowledge interrupt */
	}
    }
  return serviced;
}

static int
als300_audio_set_rate (int dev, int arg)
{
  als300_portc *portc = audio_engines[dev]->portc;

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
als300_audio_set_channels (int dev, short arg)
{
  als300_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->channels;

  if (audio_engines[dev]->flags & ADEV_STEREOONLY)
    arg = 2;

  if ((arg != 1) && (arg != 2))
    return portc->channels;
  portc->channels = arg;

  return portc->channels;
}

static unsigned int
als300_audio_set_format (int dev, unsigned int arg)
{
  als300_portc *portc = audio_engines[dev]->portc;

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
als300_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void als300_audio_trigger (int dev, int state);

static void
als300_audio_reset (int dev)
{
  als300_audio_trigger (dev, 0);
}

static void
als300_audio_reset_input (int dev)
{
  als300_portc *portc = audio_engines[dev]->portc;
  als300_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
als300_audio_reset_output (int dev)
{
  als300_portc *portc = audio_engines[dev]->portc;
  als300_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

/*ARGSUSED*/
static int
als300_audio_open (int dev, int mode, int open_flags)
{
  als300_portc *portc = audio_engines[dev]->portc;
  als300_devc *devc = audio_engines[dev]->devc;
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

  if ((mode & OPEN_READ) && !(audio_engines[dev]->flags & ADEV_NOINPUT))
    {
      audio_engines[dev]->fixed_rate = 48000;
      audio_engines[dev]->flags |=
	ADEV_FIXEDRATE | ADEV_STEREOONLY | ADEV_16BITONLY;
      audio_engines[dev]->min_rate = 48000;
      audio_engines[dev]->max_rate = 48000;
    }
  else
    {
      audio_engines[dev]->min_rate = 5000;
      audio_engines[dev]->max_rate = 48000;
      audio_engines[dev]->fixed_rate = 0;
      audio_engines[dev]->flags &=
	~(ADEV_FIXEDRATE | ADEV_STEREOONLY | ADEV_16BITONLY);
    }

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

static void
als300_audio_close (int dev, int mode)
{
  als300_portc *portc = audio_engines[dev]->portc;
  als300_devc *devc = audio_engines[dev]->devc;

  als300_audio_reset (dev);
  portc->open_mode = 0;
  devc->open_mode &= ~mode;
  portc->audio_enabled &= ~mode;
}

/*ARGSUSED*/
static void
als300_audio_output_block (int dev, oss_native_word buf, int count,
			   int fragsize, int intrflag)
{
  als300_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
}

/*ARGSUSED*/
static void
als300_audio_start_input (int dev, oss_native_word buf, int count,
			  int fragsize, int intrflag)
{
  als300_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
}

static void
als300_audio_trigger (int dev, int state)
{
  als300_portc *portc = audio_engines[dev]->portc;
  als300_devc *devc = audio_engines[dev]->devc;
  oss_native_word old82 = 0, new82 = 0, old85 = 0, new85 = 0, mode = 0;
  oss_native_word flags;
  int i;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode & OPEN_WRITE)
    {
      if (state & PCM_ENABLE_OUTPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      if (portc->bits == 8)
		mode = 0x00140000L;
	      else if (portc->bits == 16)
		mode = 0x00000000L;
	      if (portc->channels == 2)
		mode |= 0x00000000L;
	      else if (portc->channels == 1)
		mode |= 0x00080000L;

	      old82 = gcr_readl (devc, 0x82);
	      new82 = (old82 & 0x0000ffff) | 0x00010000;
	      new82 = new82 | mode | devc->srcode << 22;
#if 1
	      for (i = 0; i < 5000; i++)
		{
		  if ((INB (devc->osdev, devc->base + 6) & 0x08) == 0x00)
		    goto PlayStart;
		  oss_udelay (10);
		}
	    PlayStart:
#endif
	      gcr_writel (devc, 0x82, new82);
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
	      gcr_writel (devc, 0x82, 0x00);
	      OUTB (devc->osdev, 0x8, devc->base + 7);	/* acknowledge interrupt */
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
	      old85 = gcr_readl (devc, 0x85);
	      new85 = (old85 & 0x0000ffff) | 0x00010000;
#if 1
	      for (i = 0; i < 5000; i++)
		{
		  if ((INB (devc->osdev, devc->base + 6) & 0x08) == 0x00)
		    goto RecStart;
		  oss_udelay (10);
		}
	    RecStart:
#endif
	      gcr_writel (devc, 0x85, new85);
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
	      gcr_writel (devc, 0x85, 0x00);
	      OUTB (devc->osdev, 0x04, devc->base + 7);	/* acknowledge interrupt */
	    }
	}
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
unsigned int
FindClosestFreq (als300_devc * devc, unsigned int sr)
{
  unsigned int i, error = 50000, dif, code = 0;

  for (i = 0; i < FNO1; i += 2)
    {
      if (sr < FreqTable1[i])
	dif = FreqTable1[i] - sr;
      else
	dif = sr - FreqTable1[i];
      if (dif < error)
	{
	  error = dif;
	  code = FreqTable1[i + 1];
	}
    }
  return code;
}

/*ARGSUSED*/
static int
als300_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  als300_devc *devc = audio_engines[dev]->devc;
  als300_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_in;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  gcr_writel (devc, 0x85, dmap->fragment_size - 1);
  gcr_writel (devc, 0x83, dmap->dmabuf_phys);
  gcr_writel (devc, 0x84, dmap->dmabuf_phys + dmap->bytes_in_use - 1);
  portc->audio_enabled &= ~PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/
static int
als300_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  als300_devc *devc = audio_engines[dev]->devc;
  als300_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_out;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  devc->srcode = FindClosestFreq (devc, portc->speed);
  gcr_writel (devc, 0x82, (dmap->fragment_size - 1));
  gcr_writel (devc, 0x80, dmap->dmabuf_phys);
  gcr_writel (devc, 0x81, dmap->dmabuf_phys + dmap->bytes_in_use - 1);
  portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

#if 0
static int
als300_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  als300_portc *portc = audio_engines[dev]->portc;
  als300_devc *devc = audio_engines[dev]->devc;

  unsigned int ptr = 0;

  if (direction == DMODE_OUTPUT)
    ptr = gcr_readl (devc, 0x9A);
  if (direction == DMODE_INPUT)
    ptr = gcr_readl (devc, 0x9B);
  return ptr;
}
#endif

static audiodrv_t als300_audio_driver = {
  als300_audio_open,
  als300_audio_close,
  als300_audio_output_block,
  als300_audio_start_input,
  als300_audio_ioctl,
  als300_audio_prepare_for_input,
  als300_audio_prepare_for_output,
  als300_audio_reset,
  NULL,
  NULL,
  als300_audio_reset_input,
  als300_audio_reset_output,
  als300_audio_trigger,
  als300_audio_set_rate,
  als300_audio_set_format,
  als300_audio_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,				/* als300_alloc_buffer */
  NULL,				/* als300_free_buffer */
  NULL,
  NULL,
  NULL				/* als300_get_buffer_pointer */
};

#ifdef OBSOLETED_STUFF
/*
 * This device has "ISA style" MIDI and FM subsystems. Such devices don't
 * use PCI config space for the I/O ports and interrupts. Instead the driver
 * needs to allocate proper resources itself. This functionality is no longer
 * possible. For this reason the MIDI and FM parts are not accessible.
 */
static void
attach_fm (als300_devc * devc)
{
  if (!opl3_detect (0x388, devc->osdev))
    return;
  opl3_init (0x388, devc->osdev);
  devc->fm_attached = 1;
}

static void
attach_mpu (als300_devc * devc)
{
  struct address_info hw_config;

  hw_config.io_base = devc->mpu_base;
  hw_config.irq = -devc->irq;
  hw_config.dma = -1;
  hw_config.dma2 = -1;
  hw_config.always_detect = 0;
  hw_config.name = "ALS300 MPU";
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
unload_mpu (als300_devc * devc)
{
  struct address_info hw_config;

  hw_config.io_base = devc->mpu_base;
  hw_config.irq = -devc->irq;
  hw_config.dma = -1;
  hw_config.dma2 = -1;
  hw_config.always_detect = 0;
  hw_config.name = "ALS300 MPU";
  hw_config.driver_use_1 = 0;
  hw_config.driver_use_2 = 0;
  hw_config.osdev = devc->osdev;
#ifdef CREATE_OSP
  CREATE_OSP (hw_config.osdev);
#endif
  hw_config.card_subtype = 0;

  devc->mpu_attached = 0;
  unload_uart401 (&hw_config);
}
#endif

static int
init_als300 (als300_devc * devc)
{
  int my_mixer;
  int i;
  int adev;
  oss_native_word dwTemp;
  int first_dev = 0;

  devc->mpu_attached = devc->fm_attached = 0;

/*
 * Enable BusMasterMode and IOSpace Access 
 */
  switch (devc->model)
    {
    case MDL_ALS300:
      dwTemp = gcr_readl (devc, 0x98);
      dwTemp |= devc->mpu_base >> 4;
      dwTemp = gcr_readl (devc, 0x98);
      dwTemp |= (1 << 19 | 1 << 18 | 1 << 17 | 1 << 16);
      gcr_writel (devc, 0x98, dwTemp);
      gcr_writel (devc, 0x99, 0);
      dwTemp = gcr_readl (devc, 0x8c);
      dwTemp |= 0x50;
      if (devc->chip_rev < 4)
	dwTemp &= ~0x8000;
      else
	dwTemp |= 0x8000;
      gcr_writel (devc, 0x8c, (dwTemp | 0x50));	/*enable INTA */
      break;

    case MDL_ALS300PLUS:
      dwTemp = ((oss_native_word) 0x200 << 16) | (0x388);
      dwTemp |= 0x00010000;	/* enable Game */
      dwTemp |= 0x00000001;	/* enable FM */
      gcr_writel (devc, 0xa8, dwTemp);
      dwTemp = (oss_native_word) (devc->mpu_base << 16);
      dwTemp |= 0x00010000;	/* enable MPU */
      gcr_writel (devc, 0xa9, dwTemp);

      dwTemp = gcr_readl (devc, 0x8c);

      gcr_writel (devc, 0x8c, dwTemp | 0x308050);	/*enable INTA */
      gcr_writel (devc, 0x99, 0);	/* no DDMA */
      break;
    }

#ifdef OBSOLETED_STUFF
  attach_fm (devc);

  if (devc->mpu_base > 0)
    attach_mpu (devc);
#endif

  if ((my_mixer =
       ac97_install (&devc->ac97devc, "ALS300 AC97 Mixer", ac97_read,
		     ac97_write, devc, devc->osdev)) >= 0)
    {
      devc->mixer_dev = my_mixer;
    }
  else
    return 0;

  for (i = 0; i < MAX_PORTC; i++)
    {
      char tmp_name[100];
      als300_portc *portc = &devc->portc[i];
      int caps = ADEV_AUTOMODE;

      if (i == 0)
	{
	  sprintf (tmp_name, "%s (Rev %c)", devc->chip_name,
		   'A' + devc->chip_rev);
	  caps |= ADEV_DUPLEX;
	}
      else
	{
	  sprintf (tmp_name, "%s (shadow)", devc->chip_name);
	  caps |= ADEV_DUPLEX | ADEV_SHADOW;
	}

      if ((adev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
					devc->osdev,
					devc->osdev,
					tmp_name,
					&als300_audio_driver,
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
	  audio_engines[adev]->mixer_dev = my_mixer;
	  audio_engines[adev]->rate_source = first_dev;
	  audio_engines[adev]->mixer_dev = my_mixer;
	  audio_engines[adev]->min_block = 4096;
	  audio_engines[adev]->max_block = 4096;
	  audio_engines[adev]->min_rate = 5000;
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

  return 1;
}

int
oss_als3xx_attach (oss_device_t * osdev)
{
  unsigned char pci_irq_line, pci_revision;
  unsigned short pci_command, vendor, device;
  unsigned int pci_ioaddr;
  int err;
  als300_devc *devc;

  DDB (cmn_err (CE_WARN, "Entered ALS ALS300 probe routine\n"));


  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_0, &pci_ioaddr);

  if (vendor != ALS_VENDOR_ID || (device != ALS_300 && device != ALS_300P))
    return 0;

  DDB (cmn_err (CE_WARN, "rev %x I/O base %04x\n", pci_revision, pci_ioaddr));

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
  devc->open_mode = 0;

  devc->base = MAP_PCI_IOADDR (devc->osdev, 0, pci_ioaddr);
  /* Remove I/O space marker in bit 0. */
  devc->base &= ~3;

  devc->irq = pci_irq_line;
  devc->mpu_base = als300_mpu_base;

  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_IO;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);


  switch (device)
    {
    case ALS_300:
      devc->model = MDL_ALS300;
      devc->chip_name = "Avance Logic ALS300";
      devc->chip_rev = pci_revision;
      break;

    case ALS_300P:
      devc->model = MDL_ALS300PLUS;
      devc->chip_name = "Avance Logic ALS300+";
      devc->chip_rev = pci_revision;
      break;
    }

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);

  oss_register_device (osdev, devc->chip_name);

  if ((err = oss_register_interrupts (devc->osdev, 0, als300intr, NULL)) < 0)
    {
      cmn_err (CE_WARN, "Can't allocate IRQ%d, err=%d\n", pci_irq_line, err);
      return 0;
    }

  return init_als300 (devc);	/* Detected */
}

int
oss_als3xx_detach (oss_device_t * osdev)
{
  als300_devc *devc = (als300_devc *) osdev->devc;
  unsigned int dwTemp;

  if (oss_disable_device (osdev) < 0)
    return 0;

  if (devc->model == MDL_ALS300PLUS)
    {
      dwTemp = gcr_readl (devc, 0x8c);
      gcr_writel (devc, 0x8c, dwTemp &= ~0x8000);
    }
#ifdef OBSOLETED_STUFF
  if (devc->mpu_attached)
    unload_mpu (devc);
#endif
  oss_unregister_interrupts (devc->osdev);

  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);
  UNMAP_PCI_IOADDR (devc->osdev, 0);

  oss_unregister_device (devc->osdev);
  return 1;
}
