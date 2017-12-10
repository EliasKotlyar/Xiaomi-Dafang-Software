/*
 * Purpose: Driver for the ALI 5455 (AC97) audio controller
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

#include "oss_ali5455_cfg.h"
#include <oss_pci.h>
#include <ac97.h>

#define ALI_VENDOR_ID           0x10b9
#define ALI_DEVICE_5455         0x5455

#define MAX_ALI5455 1
#define MAX_PORTC 3
#define BDL_SIZE	32

#ifdef OSS_BIG_ENDIAN
static __inline__ unsigned int
swap32 (unsigned int x)
{
  return ((x & 0x000000ff) << 24) |
    ((x & 0x0000ff00) << 8) |
    ((x & 0x00ff0000) >> 8) | ((x & 0xff000000) >> 24);
}

static __inline__ unsigned short
swap16 (unsigned short x)
{
  return ((x >> 8) & 0xff) | ((x & 0xff) << 8);
}

#define SWAP32(x) swap32(x)
#define SWAP16(x) swap16(x)
#else
#define SWAP32(x) 	x
#define SWAP16(x) 	x
#endif

typedef struct
{
  int open_mode;
  int speed, bits, channels;
  int audio_enabled;
  int trigger_bits;
  int audiodev;
  int port_type;
#define DF_PCM 0
#define DF_SPDIF 1
}
ALI_portc;

typedef struct
{
  unsigned int addr;
  unsigned short size;
  unsigned short flags;
}
bdl_t;

typedef struct ALI_devc
{
  oss_device_t *osdev;
  oss_native_word base;
  int irq;
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;

  /* Mixer */
  ac97_devc ac97devc;
  int mixer_dev;

  /* Audio parameters */
  int open_mode;

  /* Buffer Descriptor List */
  char *bdlBuffer;
  bdl_t *playBDL, *recBDL, *spdifBDL;
  oss_native_word playBDL_phys, recBDL_phys, spdifBDL_phys;
  oss_dma_handle_t bdl_dma_handle;

  int play_currbuf, play_currfrag;
  int spdif_currbuf, spdif_currfrag;
  int rec_currbuf, rec_currfrag;
  char *chip_name;
  ALI_portc portc[MAX_PORTC];
  int play_frag_index[BDL_SIZE];
  int rec_frag_index[BDL_SIZE];
  int spdif_frag_index[BDL_SIZE];
}
ALI_devc;

static int
ac97_read (void *devc_, int reg)
{
  ALI_devc *devc = devc_;
  int i = 100;
  unsigned int status;
  unsigned int data = 0;
  unsigned short read_reg = 0;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  status = INB (devc->osdev, devc->base + 0x34);

  /* wait for the Codec Access Semaphore bit to be set */
  while (i-- && (INL (devc->osdev, devc->base + 0x3c) & 0x80000000))
    oss_udelay (1);

  for (i = 0; i < 100; i++)
    {
      status = INB (devc->osdev, devc->base + 0x38);
      if (status & 0x08)
	break;
    }
  if (i == 100)
    {
      cmn_err (CE_WARN, "AC97 not ready for read\n");
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return 0;
    }

  OUTW (devc->osdev, reg | 0x80, devc->base + 0x22);

  for (i = 0; i < 100; i++)
    {
      status = INB (devc->osdev, devc->base + 0x38);

      if (status & 0x02)
	{

	  data = INW (devc->osdev, devc->base + 0x24);
	  read_reg = INW (devc->osdev, devc->base + 0x26);
	  break;
	}
    }

  if (i == 100)
    {
      cmn_err (CE_WARN, "AC97 read timed out \n");
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return 0;
    }

  if (read_reg != reg)
    {
      cmn_err (CE_WARN, "AC97 invalid reg read %x (%x)\n", read_reg, reg);
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return 0;
    }

  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return data;
}

static int
ac97_write (void *devc_, int reg, int data)
{
  ALI_devc *devc = devc_;
  int i = 100, status;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  /* wait for the Codec Access Semaphore bit to be set */
  while (i-- && (INL (devc->osdev, devc->base + 0x3c) & 0x80000000))
    oss_udelay (1);

  /* wait until command port is ready for write */
  for (i = 0; i < 100; i++)
    {
      status = INB (devc->osdev, devc->base + 0x38);
      if (status & 0x01)
	break;
    }

  if (i == 100)
    {
      cmn_err (CE_WARN, "AC97 timed out for write\n");
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return 0;
    }

  OUTL (devc->osdev, reg << 16 | data, devc->base + 0x20);

  for (i = 0; i < 100; i++)
    {
      status = INB (devc->osdev, devc->base + 0x38);
      if (status & 0x01)
	break;
    }

  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return 1;
}

static int
ALIintr (oss_device_t * osdev)
{
  int status, global_status, p, f, i;
  int serviced = 0;
  ALI_devc *devc = (ALI_devc *) osdev->devc;
  ALI_portc *portc;
  /* oss_native_word flags; */

  /* Handle playback */
  /*
   * TODO: Fix mutexes and move the inputintr/outputintr calls outside the
   * mutex block.
   */

  /* MUTEX_ENTER (devc->mutex, flags); */
  /* Handle Global Interrupts */
  global_status = INL (devc->osdev, devc->base + 0x18);
  OUTL (devc->osdev, global_status, devc->base + 0x18);

  if (!(global_status & (0x10000 | 0x20000 | 0x80000 | 0x100000 | 0x200000)))
    {
      /* MUTEX_EXIT (devc->mutex, flags); */
      return serviced;
    }

  /* Handle Playback Interrupts */

  status = INB (devc->osdev, devc->base + 0x56);

  if ((status & 0x08) && (global_status & 0x20000))
    for (i = 0; i < MAX_PORTC - 1; i++)
      {
	portc = &devc->portc[i];
	serviced = 1;
	if ((portc->trigger_bits & PCM_ENABLE_OUTPUT))	/* IOC interrupt */
	  {
	    dmap_t *dmap = audio_engines[portc->audiodev]->dmap_out;
	    p = INB (devc->osdev, devc->base + 0x54);

	    if (p != devc->play_currbuf)
	      {
		p = devc->play_currbuf;
		f = devc->play_currfrag;
		devc->playBDL[p].addr =
		  SWAP32 (dmap->dmabuf_phys + (f * dmap->fragment_size));

		devc->playBDL[p].size = SWAP16 (dmap->fragment_size / 2);
		devc->playBDL[p].flags = SWAP16 (0xc000);	/* IOC interrupts */

		OUTB (devc->osdev, p, devc->base + 0x55);	/* Set LVD */
		devc->play_frag_index[p] = f;
		devc->play_currbuf = (p + 1) % BDL_SIZE;
		devc->play_currfrag = (f + 1) % dmap->nfrags;
	      }
	    oss_audio_outputintr (portc->audiodev, 1);
	  }
      }
  OUTB (devc->osdev, status, devc->base + 0x56);	/* Clear interrupts */

/*--------------------------------------------------------------------------*/
  /* handle SPDIF interrupts */

  status = INB (devc->osdev, devc->base + 0x76);
  if ((status & 0x08) && (global_status & 0x80000))
    {
      portc = &devc->portc[2];
      serviced = 1;
      if ((portc->trigger_bits & PCM_ENABLE_OUTPUT))	/* IOC interrupt */
	{
	  dmap_t *dmap = audio_engines[portc->audiodev]->dmap_out;
	  p = INB (devc->osdev, devc->base + 0x74);

	  if (p != devc->spdif_currbuf)
	    {
	      p = devc->spdif_currbuf;
	      f = devc->spdif_currfrag;
	      devc->spdifBDL[p].addr =
		SWAP32 (dmap->dmabuf_phys + (f * dmap->fragment_size));

	      devc->spdifBDL[p].size = SWAP16 (dmap->fragment_size / 2);
	      devc->spdifBDL[p].flags = SWAP16 (0xc000);	/* IOC interrupts */

	      OUTB (devc->osdev, p, devc->base + 0x75);	/* Set LVD */
	      devc->spdif_frag_index[p] = f;
	      devc->spdif_currbuf = (p + 1) % BDL_SIZE;
	      devc->spdif_currfrag = (f + 1) % dmap->nfrags;
	    }
	  oss_audio_outputintr (portc->audiodev, 1);
	}
    }
  OUTB (devc->osdev, status, devc->base + 0x76);	/* Clear interrupts */
/*---------------------------------------------------------------------------*/

  /* Handle Recording Interrupts */
  status = INB (devc->osdev, devc->base + 0x46);

  if ((status & 0x08) && (global_status & 0x10000))
    for (i = 0; i < MAX_PORTC - 1; i++)
      {
	portc = &devc->portc[i];
	serviced = 1;
	if ((portc->trigger_bits & PCM_ENABLE_INPUT))	/* IOC interrupt */
	  {
	    dmap_t *dmap = audio_engines[portc->audiodev]->dmap_in;
	    p = INB (devc->osdev, devc->base + 0x44);

	    if (p != devc->rec_currbuf)
	      {
		p = devc->rec_currbuf;
		f = devc->rec_currfrag;
		devc->recBDL[p].addr =
		  SWAP32 (dmap->dmabuf_phys + (f * dmap->fragment_size));

		/* SIS uses bytes, ali5455 uses samples */
		devc->recBDL[p].size = SWAP16 (dmap->fragment_size / 2);

		devc->recBDL[p].flags = SWAP16 (0xc000);	/* IOC interrupts */

		OUTB (devc->osdev, p, devc->base + 0x45);	/* Set LVD */
		devc->rec_frag_index[p] = f;
		devc->rec_currbuf = (p + 1) % BDL_SIZE;
		devc->rec_currfrag = (f + 1) % dmap->nfrags;
	      }
	    oss_audio_inputintr (portc->audiodev, 0);
	  }
      }
  OUTB (devc->osdev, status, devc->base + 0x46);	/* Clear int */

  /* MUTEX_EXIT (devc->mutex, flags); */
  return serviced;
}

/*
 * Audio routines
 */

static int
ALI_audio_set_rate (int dev, int arg)
{
  ALI_portc *portc = audio_engines[dev]->portc;

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
ALI_audio_set_channels (int dev, short arg)
{
  ALI_portc *portc = audio_engines[dev]->portc;

  if ((arg == 1) || (arg == 2))
    {
      audio_engines[dev]->flags |= ADEV_STEREOONLY;
      arg = 2;
    }
  else
    audio_engines[dev]->flags &= ~ADEV_STEREOONLY;

  if (arg>6)
     arg=6;

  if ((arg != 1) && (arg != 2) && (arg != 4) && (arg != 6))
    return portc->channels;
  portc->channels = arg;

  return portc->channels;
}

static unsigned int
ALI_audio_set_format (int dev, unsigned int arg)
{
  ALI_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->bits;

  if (!(arg & (AFMT_U8 | AFMT_S16_LE | AFMT_AC3)))
    return portc->bits;
  portc->bits = arg;

  return portc->bits;
}

/*ARGSUSED*/
static int
ALI_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void ALI_audio_trigger (int dev, int state);

static void
ALI_audio_reset (int dev)
{
  ALI_audio_trigger (dev, 0);
}

static void
ALI_audio_reset_input (int dev)
{
  ALI_portc *portc = audio_engines[dev]->portc;
  ALI_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
ALI_audio_reset_output (int dev)
{
  ALI_portc *portc = audio_engines[dev]->portc;
  ALI_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

/*ARGSUSED*/
static int
ALI_audio_open (int dev, int mode, int openflags)
{
  ALI_portc *portc = audio_engines[dev]->portc;
  ALI_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  if (portc->port_type == DF_SPDIF)
    {
      if (mode & OPEN_READ)
	{
	  cmn_err (CE_WARN,
		   "ICH: The S/PDIF device supports only playback\n");
	  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
	  return OSS_EIO;
	}
    }
  else
    {
      if (devc->open_mode & mode)
	{
	  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
	  return OSS_EBUSY;
	}

      devc->open_mode |= mode;
    }


  portc->open_mode = mode;
  portc->audio_enabled &= ~mode;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

static void
ALI_audio_close (int dev, int mode)
{
  ALI_portc *portc = audio_engines[dev]->portc;
  ALI_devc *devc = audio_engines[dev]->devc;

  ALI_audio_reset (dev);
  portc->open_mode = 0;

  if (portc->port_type != DF_SPDIF)
    devc->open_mode &= ~mode;


  portc->audio_enabled &= ~mode;
}

/*ARGSUSED*/
static void
ALI_audio_output_block (int dev, oss_native_word buf, int count,
			int fragsize, int intrflag)
{
  ALI_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
}

/*ARGSUSED*/
static void
ALI_audio_start_input (int dev, oss_native_word buf, int count,
		       int fragsize, int intrflag)
{
  ALI_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
}

static void
ALI_audio_trigger (int dev, int state)
{
  ALI_devc *devc = audio_engines[dev]->devc;
  ALI_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (portc->open_mode & OPEN_WRITE)
    {
      if (state & PCM_ENABLE_OUTPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      if (portc->port_type == DF_SPDIF)
		{
		  OUTB (devc->osdev, 0x1d, devc->base + 0x7b);	/* Setup intr */
		  OUTW (devc->osdev, INW (devc->osdev, devc->base + 0x08) | 0x08, devc->base + 0x08);	/* start DMA */
		}

	      if (portc->port_type == DF_PCM)
		{
		  OUTB (devc->osdev, 0x1d, devc->base + 0x5b);	/* setup intr */
		  OUTW (devc->osdev, INW (devc->osdev, devc->base + 0x08) | 0x02, devc->base + 0x08);	/* start DMA */
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
	      if (portc->port_type == DF_SPDIF)
		{
		  OUTB (devc->osdev, 0x00, devc->base + 0x7b);	/* reset */
		  OUTW (devc->osdev, INW (devc->osdev, devc->base + 0x08) & ~0x08, devc->base + 0x08);	/* stop DMA */
		}

	      if (portc->port_type == DF_PCM)
		{
		  OUTB (devc->osdev, 0x00, devc->base + 0x5b);	/* reset */
		  OUTW (devc->osdev, INW (devc->osdev, devc->base + 0x08) & ~0x02, devc->base + 0x08);	/* stop DMA */
		}
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
	      OUTB (devc->osdev, 0x1d, devc->base + 0x4b);	/* Kickstart */
	      OUTW (devc->osdev, INW (devc->osdev, devc->base + 0x08) | 0x01, devc->base + 0x08);	/* stop DMA */
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
	      OUTB (devc->osdev, 0x00, devc->base + 0x4b);	/* reset */
	      OUTW (devc->osdev, INW (devc->osdev, devc->base + 0x08) & ~0x01, devc->base + 0x08);	/* stop DMA */
	    }
	}
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
static int
ALI_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  ALI_devc *devc = audio_engines[dev]->devc;
  ALI_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_in;
  int i, n;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  OUTB (devc->osdev, 0x02, devc->base + 0x4b);	/* Reset */
  OUTL (devc->osdev, devc->recBDL_phys, devc->base + 0x40);	/* BDL base */

  ac97_recrate (&devc->ac97devc, portc->speed);

  n = bcount;
  if (n > BDL_SIZE)
    n = BDL_SIZE;

  for (i = 0; i < n; i++)
    {
      devc->recBDL[i].addr =
	SWAP32 (dmap->dmabuf_phys + (i * dmap->fragment_size));
      devc->recBDL[i].size = SWAP16 (dmap->fragment_size / 2);
      devc->recBDL[i].flags = SWAP16 (0xc000);	/* IOC interrupts */
      devc->rec_frag_index[i] = i;
    }
  OUTB (devc->osdev, n - 1, devc->base + 0x45);	/* Set last valid descriptor */

  devc->rec_currbuf = n % BDL_SIZE;
  devc->rec_currfrag = n;
  if (devc->rec_currfrag >= dmap->nfrags)
    devc->rec_currfrag = 0;

  portc->audio_enabled &= ~PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/
static int
ALI_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  ALI_devc *devc = audio_engines[dev]->devc;
  ALI_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_out;
  int i, n;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  ac97_spdifout_ctl (devc->mixer_dev, SPDIFOUT_AUDIO, SNDCTL_MIX_WRITE, 0);
  ac97_spdif_setup (devc->mixer_dev, portc->speed, portc->bits);

  if (portc->bits == AFMT_AC3)
    {
      portc->channels = 2;
      portc->bits = 16;
    }

  /* do SPDIF out */
  if (portc->port_type == DF_SPDIF)
    {
      ac97_playrate (&devc->ac97devc, portc->speed);
      OUTB (devc->osdev, 0x02, devc->base + 0x7b);	/* Reset */
      OUTL (devc->osdev, devc->spdifBDL_phys, devc->base + 0x70);

      n = bcount;
      if (n > BDL_SIZE)
	n = BDL_SIZE;

      for (i = 0; i < n; i++)
	{
	  devc->spdifBDL[i].addr =
	    SWAP32 (dmap->dmabuf_phys + (i * dmap->fragment_size));
	  devc->spdifBDL[i].size = SWAP16 (dmap->fragment_size / 2);
	  devc->spdifBDL[i].flags = SWAP16 (0xc000);	/* IOC interrupts */
	  devc->spdif_frag_index[i] = i;
	}
      OUTB (devc->osdev, n - 1, devc->base + 0x75);	/* Set LVI descriptor */
      devc->spdif_currbuf = n % BDL_SIZE;
      devc->spdif_currfrag = n;
      if (devc->spdif_currfrag >= dmap->nfrags)
	devc->spdif_currfrag = 0;

      portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
      portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;

      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return 0;
    }

  /* else do PCM */
  OUTB (devc->osdev, 0x02, devc->base + 0x5b);	/* Reset */
  OUTL (devc->osdev, devc->playBDL_phys, devc->base + 0x50);

  ac97_playrate (&devc->ac97devc, portc->speed);

  /* set default to 2 channel mode */
  if (portc->channels == 2)
    OUTW (devc->osdev, INW (devc->osdev, devc->base + 0x00) & ~0x300,
	  devc->base + 0x00);
  if (portc->channels == 4)
    OUTW (devc->osdev,
	  (INW (devc->osdev, devc->base + 0x00) & ~0x300) | 0x100,
	  devc->base + 0x00);
  if (portc->channels == 6)
    OUTW (devc->osdev,
	  (INW (devc->osdev, devc->base + 0x00) & ~0x300) | 0x200,
	  devc->base + 0x00);


  n = bcount;
  if (n > BDL_SIZE)
    n = BDL_SIZE;

  for (i = 0; i < n; i++)
    {
      devc->playBDL[i].addr =
	SWAP32 (dmap->dmabuf_phys + (i * dmap->fragment_size));
      devc->playBDL[i].size = SWAP16 (dmap->fragment_size / 2);
      devc->playBDL[i].flags = SWAP16 (0xc000);	/* IOC interrupts */
      devc->play_frag_index[i] = i;
    }
  OUTB (devc->osdev, n - 1, devc->base + 0x55);	/* Set last valid descriptor */

  devc->play_currbuf = n % BDL_SIZE;
  devc->play_currfrag = n;
  if (devc->play_currfrag >= dmap->nfrags)
    devc->play_currfrag = 0;

  portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

static const audiodrv_t ALI_audio_driver = {
  ALI_audio_open,
  ALI_audio_close,
  ALI_audio_output_block,
  ALI_audio_start_input,
  ALI_audio_ioctl,
  ALI_audio_prepare_for_input,
  ALI_audio_prepare_for_output,
  ALI_audio_reset,
  NULL,
  NULL,
  ALI_audio_reset_input,
  ALI_audio_reset_output,
  ALI_audio_trigger,
  ALI_audio_set_rate,
  ALI_audio_set_format,
  ALI_audio_set_channels
};

static int
init_ALI (ALI_devc * devc)
{
  int my_mixer, my_dev, opts;
  oss_native_word phaddr;
  int i;

  /* ACLink on, warm reset */
  OUTL (devc->osdev, 0x80008003, devc->base + 0x00);	/*reset SCR */
  OUTL (devc->osdev, 0x83838383, devc->base + 0x0c);	/*reset pcm in/out FIFO */
  OUTL (devc->osdev, 0x83838383, devc->base + 0x1c);	/*reset SPDIF/LFEr FIFO */
  OUTL (devc->osdev, 0x0028000a, devc->base + 0x10);	/*set spdif/pcm in/out */
  OUTL (devc->osdev, INL (devc->osdev, devc->base + 0xFC) | 0x3,
	devc->base + 0xFC);

  /* set up Codec SPDIFOUT slot to 10/11 */
  OUTL (devc->osdev, INL (devc->osdev, devc->base + 0x00) | 0x300000,
	devc->base + 0x00);
  /* disable interrupts */
  OUTL (devc->osdev, 0x00, devc->base + 0x14);
  OUTL (devc->osdev, 0x00, devc->base + 0x18);

  devc->bdlBuffer =
    CONTIG_MALLOC (devc->osdev, 4 * 32 * 32, MEMLIMIT_32BITS, &phaddr, devc->bdl_dma_handle);
  if (devc->bdlBuffer == NULL)
    {
      cmn_err (CE_WARN, "Failed to allocate BDL\n");
      return 0;
    }

  devc->playBDL = (bdl_t *) devc->bdlBuffer;
  devc->playBDL_phys = phaddr;
  devc->recBDL = (bdl_t *) (devc->bdlBuffer + (1 * 32 * 32));
  devc->recBDL_phys = phaddr + (1 * 32 * 32);
  devc->spdifBDL = (bdl_t *) (devc->bdlBuffer + (2 * 32 * 32));
  devc->spdifBDL_phys = phaddr + (2 * 32 * 32);

/*
 * Init mixer
 */
  my_mixer =
    ac97_install (&devc->ac97devc, "AC97 Mixer", ac97_read, ac97_write, devc,
		  devc->osdev);

  if (my_mixer == -1)
    return 0;			/* No mixer */

  devc->mixer_dev = my_mixer;

  /* enable S/PDIF */
  devc->ac97devc.spdif_slot = SPDIF_SLOT1011;
  ac97_spdifout_ctl (devc->mixer_dev, SPDIFOUT_ENABLE, SNDCTL_MIX_WRITE, 1);

#if 0
  /* enable variable rate mode */
  ac97_write (devc, 0x2a, ac97_read (devc, 0x2a) | 9);
  if (!(ac97_read (devc, 0x2a) & 1))
    DDB (cmn_err (CE_WARN, "VRA not supported...using GRC\n"));
#endif

  for (i = 0; i < MAX_PORTC; i++)
    {
      ALI_portc *portc = &devc->portc[i];
      char tmp_name[100];
      int port_fmt = DF_PCM;
      int formats = AFMT_S16_LE | AFMT_AC3;
      strcpy (tmp_name, devc->chip_name);
      opts = ADEV_AUTOMODE | ADEV_16BITONLY | ADEV_STEREOONLY;
      portc->port_type = DF_PCM;

      if (!ac97_varrate (&devc->ac97devc))
	{
	  opts |= ADEV_FIXEDRATE;
	}

      if (i == 0)
	{
	  opts |= ADEV_DUPLEX;
	  strcpy (tmp_name, devc->chip_name);
	}
      if (i == 1)
	{
	  opts |= ADEV_DUPLEX | ADEV_SHADOW;
	  strcpy (tmp_name, devc->chip_name);
	}

      if (i == 2)
	{
	  sprintf (tmp_name, "%s (S/PDIF)", devc->chip_name);
	  opts |= ADEV_NOINPUT | ADEV_SPECIAL | ADEV_FIXEDRATE;
	  port_fmt = DF_SPDIF;
	}

      if ((my_dev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
					  devc->osdev,
					  devc->osdev,
					  tmp_name,
					  &ALI_audio_driver,
					  sizeof (audiodrv_t), opts,
					  formats, devc, -1)) < 0)
	{
	  my_dev = -1;
	  return 0;
	}
      else
	{
	  audio_engines[my_dev]->portc = portc;
	  audio_engines[my_dev]->mixer_dev = my_mixer;
	  audio_engines[my_dev]->min_rate =
	    (opts & ADEV_FIXEDRATE) ? 48000 : 5000;
	  audio_engines[my_dev]->max_rate = 48000;
	  audio_engines[my_dev]->caps |= PCM_CAP_FREERATE;
	  /*audio_engines[my_dev]->min_block = 4096; */
	  /*audio_engines[my_dev]->max_block = 4096; */
	  audio_engines[my_dev]->min_channels = 2;
	  audio_engines[my_dev]->max_channels = 6;
	  portc->open_mode = 0;
	  portc->audio_enabled = 0;
	  portc->audiodev = my_dev;
	  portc->port_type = port_fmt;
	  if (audio_engines[my_dev]->flags & ADEV_FIXEDRATE)
	    audio_engines[my_dev]->fixed_rate = 48000;
#ifdef CONFIG_OSS_VMIX
	  if (i == 0)
	     vmix_attach_audiodev(devc->osdev, my_dev, -1, 0);
#endif
	}
    }
  return 1;
}


int
oss_ali5455_attach (oss_device_t * osdev)
{
  unsigned char pci_irq_line, pci_revision /*, pci_latency */ ;
  unsigned short pci_command, vendor, device;
  unsigned int pci_ioaddr0;
  ALI_devc *devc;

  DDB (cmn_err (CE_WARN, "Entered ALI AC97 probe routine\n"));

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  if ((vendor != ALI_VENDOR_ID) || (device != ALI_DEVICE_5455))
    return 0;

  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_0, &pci_ioaddr0);

  if (pci_ioaddr0 == 0)
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

  /* Remove I/O space marker in bit 0. */
  devc->base = MAP_PCI_IOADDR (devc->osdev, 0, pci_ioaddr0);
  devc->base &= ~0xF;

  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_IO;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  devc->chip_name = "ALI M5455";
  devc->irq = pci_irq_line;

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);

  oss_register_device (osdev, devc->chip_name);

  if (oss_register_interrupts (devc->osdev, 0, ALIintr, NULL) < 0)
    {
      cmn_err (CE_WARN, "Unable to install interrupt handler\n");
      return 0;
    }

  return init_ALI (devc);	/* Detected */
}


int
oss_ali5455_detach (oss_device_t * osdev)
{
  ALI_devc *devc = (ALI_devc *) osdev->devc;


  if (oss_disable_device (osdev) < 0)
    return 0;

  /* disable S/PDIF */
  if (devc->mixer_dev)
    ac97_spdifout_ctl (devc->mixer_dev, SPDIFOUT_ENABLE, SNDCTL_MIX_WRITE, 0);
  /* disable interrupts */
  OUTL (devc->osdev, 0x00, devc->base + 0x14);
  OUTL (devc->osdev, 0x00, devc->base + 0x18);

  oss_unregister_interrupts (devc->osdev);

  if (devc->bdlBuffer)
    {
      CONTIG_FREE (devc->osdev, devc->bdlBuffer, 4 * 32 * 32, devc->bdl_dma_handle);
      devc->bdlBuffer = NULL;
    }

  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);
  UNMAP_PCI_IOADDR (devc->osdev, 0);

  oss_unregister_device (osdev);
  return 1;
}
