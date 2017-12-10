/*
 * Purpose: Driver for the ATI IXP (AC97) audio controller
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

#include "oss_atiaudio_cfg.h"
#include "oss_pci.h"
#include "ac97.h"

#define ATI_VENDOR_ID           0x1002
#define ATI_DEVICE_IXP         	0x4341
#define ATI_DEVICE_IXP300       0x4361
#define ATI_DEVICE_IXP400       0x4370

#define MAX_PORTC 3
#define BDL_SIZE	64

typedef struct
{
  int open_mode;
  int speed, bits, channels;
  int audio_enabled;
  int trigger_bits;
  int audiodev;
  int port_type;
#define DF_ANALOG 0
#define DF_SPDIF 1
}
ATI_portc;

typedef struct
{
  unsigned int addr;
  unsigned short status;
  unsigned short size;
  unsigned int next;
}
bdl_t;

typedef struct
{
  oss_device_t *osdev;
  oss_native_word membar_addr;
  char *membar_virt;
  int irq;
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;

  /* Mixer */
  ac97_devc ac97devc;
  int mixer_dev;
  int inverted;

  /* Audio parameters */
  int open_mode;

  /* Buffer Descriptor List */
  char *bdlBuffer;
  bdl_t *playBDL, *recBDL, *spdifBDL;
  oss_native_word playBDL_phys, recBDL_phys, spdifBDL_phys;
  oss_dma_handle_t bldbuf_dma_handle;

  int play_currbuf, play_currfrag;
  int rec_currbuf, rec_currfrag;
  char *chip_name;
  ATI_portc portc[MAX_PORTC];
  int play_frag_index[BDL_SIZE];
  int rec_frag_index[BDL_SIZE];
}
ATI_devc;


/* Mem mapped I/O registers */
#define READL(o,a)       *(volatile unsigned int*)(devc->membar_virt+(a))
#define READW(o,a)       *(volatile unsigned short*)(devc->membar_virt+(a))
#define READB(o,a)       *(volatile unsigned char*)(devc->membar_virt+(a))
#define WRITEL(o,d,a)    *(volatile unsigned int*)(devc->membar_virt+(a))=d
#define WRITEW(o,d,a)    *(volatile unsigned short*)(devc->membar_virt+(a))=d
#define WRITEB(o,d,a)    *(volatile unsigned char*)(devc->membar_virt+(a))=d


static int
ac97_ready (void *devc_)
{
  ATI_devc *devc = devc_;
  int timeout = 10000;

  while (READL (devc->osdev, 0x0c) & 0x100)
    {
      if (!timeout--)
	{
	  cmn_err (CE_WARN, "codec ready timed out\n");
	  return OSS_EIO;
	}
      oss_udelay (10);
    }
  return 0;
}

static int
ac97_read (void *devc_, int reg)
{
  ATI_devc *devc = devc_;
  unsigned int data = 0;
  unsigned int addr = 0;
  int timeout;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);

again:
  if (ac97_ready (devc) < 0)
    {
      cmn_err (CE_WARN, "ac97 not ready\n");
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return OSS_EIO;
    }

  addr = (unsigned int) (reg << 9) | (1 << 8) | (1 << 2);
  WRITEL (devc->osdev, addr, 0x0c);	/* Set read commmand */

  if (ac97_ready (devc) < 0)
    {
      cmn_err (CE_WARN, "ac97 not ready\n");
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return OSS_EIO;
    }

  timeout = 1000;

  for (;;)
    {
      data = READL (devc->osdev, 0x10);	/* ac97 data port */
      if (data & 0x100)
	{
	  /* if the register returned isn't the reg sent then resend command */
	  if ((data & 0xFE00) != (reg << 9))
	    goto again;
	  else
	    break;
	}

      if (!--timeout)
	{
	  cmn_err (CE_WARN, "AC97 read timed out\n");
	  break;
	}
    }
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return (data >> 16);
}


static int
ac97_write (void *devc_, int reg, int data)
{
  ATI_devc *devc = devc_;
  oss_native_word flags;
  unsigned int val;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  if (ac97_ready (devc) < 0)
    {
      cmn_err (CE_WARN, "AC97 not ready\n");
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return 0;
    }

  val = (data << 16) | (reg << 9) | 0x100;
  WRITEL (devc->osdev, val, 0x0c);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return 1;
}

static int
ATIIXPintr (oss_device_t * osdev)
{
  int status, i;
  int serviced = 0;
  ATI_devc *devc = (ATI_devc *) osdev->devc;
  ATI_portc *portc;
  /* oss_native_word flags; */

/*
 * TODO: Enable the mutexes and move inputintr/outputintr calls outside the
 * mutex region.
 */

  /* MUTEX_ENTER (devc->mutex, flags); */
  /* Handle Global Interrupts */
  status = READL (devc->osdev, 0x00);

  /* handle PCM output */
  if (status & 0x08)
    for (i = 0; i < MAX_PORTC; i++)
      {
	portc = &devc->portc[i];
	if ((portc->trigger_bits & PCM_ENABLE_OUTPUT)
	    && (portc->port_type == DF_ANALOG))
	  {
	    dmap_t *dmap = audio_engines[portc->audiodev]->dmap_out;
	    int ptr=0, n = 0;
	    int timeout = 1000;
	    serviced = 1;
	    while (timeout--)
	      {
		ptr = READL (devc->osdev, 0x44);

		if (ptr < dmap->dmabuf_phys)
		  continue;

		ptr -= dmap->dmabuf_phys;

		if (ptr >= dmap->bytes_in_use)
		  continue;

		break;
	      }

	    if (dmap->fragment_size == 0)
	      {
		cmn_err (CE_WARN, "dmap->fragment_size == 0\n");
		continue;
	      }

	    ptr /= dmap->fragment_size;

	    if ((ptr < 0) || (ptr > dmap->nfrags))
	      ptr = 0;

	    n = 0;
	    while ((dmap_get_qhead (dmap) != ptr) && (n++ < dmap->nfrags))
	      oss_audio_outputintr (portc->audiodev, 1);
	  }
      }

  /* handle PCM input */
  if (status & 0x02)
    for (i = 0; i < MAX_PORTC; i++)
      {
	portc = &devc->portc[i];
	if ((portc->trigger_bits & PCM_ENABLE_INPUT)
	    && (portc->port_type == DF_ANALOG))
	  {
	    dmap_t *dmap = audio_engines[portc->audiodev]->dmap_in;
	    int ptr=0, n = 0;
	    int timeout = 1000;
	    serviced = 1;

	    while (timeout--)
	      {
		ptr = READL (devc->osdev, 0x2c);

		if (ptr < dmap->dmabuf_phys)
		  continue;

		ptr -= dmap->dmabuf_phys;

		if (ptr >= dmap->bytes_in_use)
		  continue;

		break;
	      }

	    if (dmap->fragment_size == 0)
	      {
		cmn_err (CE_WARN, "dmap->fragment_size == 0\n");
		continue;
	      }

	    ptr /= dmap->fragment_size;

	    if ((ptr < 0) || (ptr > dmap->nfrags))
	      ptr = 0;

	    n = 0;
	    while ((dmap_get_qtail (dmap) != ptr) && (n++ < dmap->nfrags))
	      oss_audio_inputintr (portc->audiodev, 0);
	  }
      }

  /* handle SPDIF output */
  if (status & 0x20)
    {
      portc = &devc->portc[2];
      if ((portc->trigger_bits & PCM_ENABLE_OUTPUT)
	  && (portc->port_type == DF_SPDIF))
	{
	  dmap_t *dmap = audio_engines[portc->audiodev]->dmap_out;
	  int ptr=0, n = 0;
	  int timeout = 1000;
	  serviced = 1;

	  while (timeout--)
	    {
	      ptr = READL (devc->osdev, 0x5c);
	      if (ptr < dmap->dmabuf_phys)
		continue;
	      ptr -= dmap->dmabuf_phys;
	      if (ptr >= dmap->bytes_in_use)
		continue;
	      break;
	    }

	  if (dmap->fragment_size == 0)
	    cmn_err (CE_WARN, "dmap->fragment_size == 0\n");
	  else
	    {
	      ptr /= dmap->fragment_size;

	      if ((ptr < 0) || (ptr > dmap->nfrags))
		ptr = 0;

	      n = 0;
	      while ((dmap_get_qhead (dmap) != ptr) && (n++ < dmap->nfrags))
		oss_audio_outputintr (portc->audiodev, 1);
	    }
	}
    }

  /* Acknowledge Interrupts */
  WRITEL (devc->osdev, status, 0x00);
  /* MUTEX_EXIT (devc->mutex, flags); */
  return serviced;
}

/*
 * Audio routines
 */

static int
ATI_audio_set_rate (int dev, int arg)
{
  ATI_portc *portc = audio_engines[dev]->portc;

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
ATI_audio_set_channels (int dev, short arg)
{
  ATI_portc *portc = audio_engines[dev]->portc;

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
ATI_audio_set_format (int dev, unsigned int arg)
{
  ATI_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->bits;

  if (!(arg & (AFMT_U8 | AFMT_S16_LE | AFMT_AC3)))
    return portc->bits;
  portc->bits = arg;

  return portc->bits;
}

/*ARGSUSED*/
static int
ATI_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void ATI_audio_trigger (int dev, int state);

static void
ATI_audio_reset (int dev)
{
  ATI_audio_trigger (dev, 0);
}

static void
ATI_audio_reset_input (int dev)
{
  ATI_portc *portc = audio_engines[dev]->portc;
  ATI_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
ATI_audio_reset_output (int dev)
{
  ATI_portc *portc = audio_engines[dev]->portc;
  ATI_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

/*ARGSUSED*/
static int
ATI_audio_open (int dev, int mode, int openflags)
{
  ATI_portc *portc = audio_engines[dev]->portc;
  ATI_devc *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode || (devc->open_mode & mode))
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  portc->open_mode = mode;
  portc->audio_enabled &= ~mode;
  devc->open_mode |= mode;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

static void
ATI_audio_close (int dev, int mode)
{
  ATI_portc *portc = audio_engines[dev]->portc;
  ATI_devc *devc = audio_engines[dev]->devc;

  ATI_audio_reset (dev);
  portc->open_mode = 0;
  devc->open_mode &= ~mode;
  portc->audio_enabled &= ~mode;
}

/*ARGSUSED*/
static void
ATI_audio_output_block (int dev, oss_native_word buf, int count,
			int fragsize, int intrflag)
{
  ATI_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
}

/*ARGSUSED*/
static void
ATI_audio_start_input (int dev, oss_native_word buf, int count,
		       int fragsize, int intrflag)
{
  ATI_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
}

static void
ATI_enable_dma (int dev, int direction, int enable)
{
  ATI_devc *devc = audio_engines[dev]->devc;
  ATI_portc *portc = audio_engines[dev]->portc;

  if (enable)
    {
      if (direction)
	{
	  /* flush fifo and enable analog or spdif DMA */
	  WRITEL (devc->osdev, READL (devc->osdev, 0x88) | 1, 0x88);

	  if (portc->port_type == DF_ANALOG)
	    WRITEL (devc->osp, READL (devc->osp, 0x08) | 1 << 9, 0x08);
	  else
	    WRITEL (devc->osp, READL (devc->osp, 0x08) | 1 << 10, 0x08);

	}
      else
	{
	  /* flush fifo */
	  WRITEL (devc->osdev, READL (devc->osdev, 0x88) | 2, 0x88);
	  WRITEL (devc->osdev, READL (devc->osdev, 0x08) | 1 << 8, 0x08);
	}
    }
  else
    {
      if (direction)
	{
	  if (portc->port_type == DF_ANALOG)
	    WRITEL (devc->osp, READL (devc->osp, 0x08) & ~(1 << 9), 0x08);
	  else
	    WRITEL (devc->osp, READL (devc->osp, 0x08) & ~(1 << 10), 0x08);
	}
      else
	WRITEL (devc->osdev, READL (devc->osdev, 0x08) & ~(1 << 8), 0x08);
    }
}

static void
ATI_audio_trigger (int dev, int state)
{
  ATI_devc *devc = audio_engines[dev]->devc;
  ATI_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (portc->open_mode & OPEN_WRITE)
    {
      if (state & PCM_ENABLE_OUTPUT)
	{
	  if ((portc->audio_enabled & PCM_ENABLE_OUTPUT) &&
	      !(portc->trigger_bits & PCM_ENABLE_OUTPUT))
	    {
	      ATI_enable_dma (dev, 1, 1);

	      /* set BDL & enable analog or SPDIF DMA */
	      if (portc->port_type == DF_ANALOG)
		{
		  WRITEL (devc->osp, devc->playBDL_phys | 1, 0x38);
		  WRITEL (devc->osp, READL (devc->osp, 0x08) | 0x04, 0x08);
		}
	      else		/* SPDIF */
		{
		  WRITEL (devc->osp, devc->spdifBDL_phys | 1, 0x50);
		  WRITEL (devc->osp, READL (devc->osp, 0x08) | 0x10, 0x08);
		}

	      /* set bus busy */
	      WRITEL (devc->osdev, READL (devc->osdev, 0x04) | 1 << 14, 0x04);

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

	      ATI_enable_dma (dev, 1, 0);

	      /* Stop Anlog or SPDIF DMA */
	      if (portc->port_type == DF_ANALOG)
		WRITEL (devc->osp, READL (devc->osp, 0x08) & ~0x04, 0x08);
	      else
		WRITEL (devc->osp, READL (devc->osp, 0x08) & ~0x10, 0x08);

	      /* set bus clear */
	      WRITEL (devc->osdev, READL (devc->osdev, 0x04) & ~(1 << 14),
		      0x04);

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
	      portc->trigger_bits |= PCM_ENABLE_INPUT;
	      ATI_enable_dma (dev, 0, 1);
	      WRITEL (devc->osdev, devc->recBDL_phys | 1, 0x20);

	      /* enable audio intput and enable input DMA */
	      WRITEL (devc->osdev, READL (devc->osdev, 0x08) | 0x02, 0x08);
	      /* set bus busy */
	      WRITEL (devc->osdev, READL (devc->osdev, 0x04) | (1 << 14),
		      0x04);
	    }
	}
      else
	{
	  if ((portc->audio_enabled & PCM_ENABLE_INPUT) &&
	      (portc->trigger_bits & PCM_ENABLE_INPUT))
	    {
	      portc->audio_enabled &= ~PCM_ENABLE_INPUT;
	      portc->trigger_bits &= ~PCM_ENABLE_INPUT;
	      ATI_enable_dma (dev, 0, 0);
	      /* stop audio input */
	      WRITEL (devc->osdev, READL (devc->osdev, 0x08) & ~0x02, 0x08);
	      /* clear bus busy */
	      WRITEL (devc->osdev, READL (devc->osdev, 0x04) & ~(1 << 14),
		      0x04);
	    }
	}
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
static int
ATI_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  ATI_devc *devc = audio_engines[dev]->devc;
  ATI_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_in;
  int i, n;
  unsigned int data;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  /* set the bits */
  data = READL (devc->osdev, 0x08) & ~(1 << 21);
  if (portc->bits == 16)
    data |= (1 << 21);
  WRITEL (devc->osdev, data, 0x08);	/*set 8/16 bits */

  n = dmap->nfrags;
  if (n > BDL_SIZE)
    {
      cmn_err (CE_WARN, "Internal error - BDL too small\n");
      return OSS_EIO;
    }

  for (i = 0; i < n; i++)
    {
      int next = i + 1;
      if (i == n - 1)
	next = 0;

      devc->recBDL[i].addr = dmap->dmabuf_phys + (i * dmap->fragment_size);
      devc->recBDL[i].status = 0;
      devc->recBDL[i].size = dmap->fragment_size >> 2;
      devc->recBDL[i].next = devc->recBDL_phys + next * sizeof (bdl_t);
    }

  portc->audio_enabled &= ~PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

/*ARGSUSED*/
static int
ATI_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  ATI_devc *devc = audio_engines[dev]->devc;
  ATI_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_out;
  int i, n;
  unsigned int data;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);


  if (portc->port_type == DF_SPDIF)
    {
      ac97_spdif_setup (devc->mixer_dev, portc->speed, portc->bits);

      if (portc->bits == AFMT_AC3)
	{
	  portc->channels = 2;
	  portc->bits = 16;
	}

      n = dmap->nfrags;
      if (n > BDL_SIZE)
	n = BDL_SIZE;

      for (i = 0; i < n; i++)
	{
	  int next = i + 1;
	  if (i == n - 1)
	    next = 0;

	  devc->spdifBDL[i].addr =
	    dmap->dmabuf_phys + (i * dmap->fragment_size);
	  devc->spdifBDL[i].status = 0;
	  devc->spdifBDL[i].size = dmap->fragment_size >> 2;
	  devc->spdifBDL[i].next =
	    devc->spdifBDL_phys + next * sizeof (bdl_t);
	}

    }
  else
    {
      data = READL (devc->osdev, 0x34) & ~0x3ff;
      switch (portc->channels)
	{
	case 6:
	  data |= 0x30;		/* slots 7, 8 */
	  break;
	case 4:
	  data |= 0x48;		/* slts 6, 9 */
	  break;
	default:
	  data |= 0x03;		/* slots 3, 4 */
	  break;
	}
      data |= 4 << 11;
      WRITEL (devc->osdev, data, 0x34);

      /* set the bits */
      data = READL (devc->osdev, 0x08) & ~(1 << 22);
      if (portc->bits == 16)
	data |= (1 << 22);
      WRITEL (devc->osdev, data, 0x08);	/*set 8/16 bits */
#if 0
      /* set 6 channel reorder */
      if (portc->channels >= 6)
	WRITEL (devc->osdev, 0x1, 0x84);
      else
	WRITEL (devc->osdev, 0x00, 0x84);
#endif
      n = dmap->nfrags;
      if (n > BDL_SIZE)
	{
	  cmn_err (CE_WARN, "Internal error - BDL too small\n");
	  return OSS_EIO;
	}

      for (i = 0; i < n; i++)
	{
	  int next = i + 1;
	  if (i == n - 1)
	    next = 0;

	  devc->playBDL[i].addr =
	    dmap->dmabuf_phys + (i * dmap->fragment_size);
	  devc->playBDL[i].status = 0;
	  devc->playBDL[i].size = dmap->fragment_size >> 2;
	  devc->playBDL[i].next = devc->playBDL_phys + next * sizeof (bdl_t);
	}
    }
  portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

static int
ATI_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  ATI_devc *devc = audio_engines[dev]->devc;
  ATI_portc *portc = audio_engines[dev]->portc;
  unsigned int f = 0;
  oss_native_word flags;
  int loop = 100;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  /* we need to read the regs a couple of times because of possible h/w bugs */
  while (loop--)
    {
      if (direction == PCM_ENABLE_OUTPUT)
	{
	  if (portc->port_type == DF_ANALOG)
	    f = READL (devc->osdev, 0x44);
	  else
	    f = READL (devc->osdev, 0x5c);
	}

      if (direction == PCM_ENABLE_INPUT)
	{
	  f = READL (devc->osdev, 0x2c);
	}

      if (f < dmap->dmabuf_phys)
	continue;

      f -= dmap->dmabuf_phys;

      if (f >= dmap->bytes_in_use)
	continue;

      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      return f;
    }

  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return 0;
}

static const audiodrv_t ATI_audio_driver = {
  ATI_audio_open,
  ATI_audio_close,
  ATI_audio_output_block,
  ATI_audio_start_input,
  ATI_audio_ioctl,
  ATI_audio_prepare_for_input,
  ATI_audio_prepare_for_output,
  ATI_audio_reset,
  NULL,
  NULL,
  ATI_audio_reset_input,
  ATI_audio_reset_output,
  ATI_audio_trigger,
  ATI_audio_set_rate,
  ATI_audio_set_format,
  ATI_audio_set_channels,
  NULL,
  NULL,
  NULL,				/* ATI_check_input, */
  NULL,				/* ATI_check_output, */
  NULL,				/* ATI_alloc_buffer, */
  NULL,				/* ATI_free_buffer, */
  NULL,
  NULL,
  ATI_get_buffer_pointer
};

static int
init_ATI (ATI_devc * devc)
{
  int my_mixer, my_dev, opts;
  int i, timeout;
  oss_native_word phaddr;

  /* Power up */
  WRITEL (devc->osdev, READL (devc->osdev, 0x08) & ~1, 0x08);
  oss_udelay (20);
  /* check if the ACLink is Active */
  timeout = 10;
  while (!(READL (devc->osdev, 0x08) & (1 << 28)))
    {
      WRITEL (devc->osdev, READL (devc->osdev, 0x08) | (1 << 30), 0x08);	/* assert sync */
      oss_udelay (10);
      if (READL (devc->osdev, 0x08) & (1 << 28))
	break;

      /* set AC97 reset field to 0 */
      WRITEL (devc->osdev, READL (devc->osdev, 0x08) & ~(0x80000000), 0x08);
      oss_udelay (10);

      /* set AC97 reset field to 1 */
      WRITEL (devc->osdev, READL (devc->osdev, 0x08) | (0x80000000), 0x08);
      oss_udelay (10);

      if (READL (devc->osdev, 0x08) & (1 << 28))
	break;
      if (!--timeout)
	{
	  cmn_err (CE_WARN, "Timed out waiting for AC97 Link ready\n");
	  break;
	}
    }

  /* set ac97 reset field to 1: deassert */
  WRITEL (devc->osdev, READL (devc->osdev, 0x08) | (0x80000000), 0x08);

  /* do a soft reset */
  WRITEL (devc->osdev, READL (devc->osdev, 0x08) | (1 << 29), 0x08);
  oss_udelay (10);

  /* deassert softreset */
  WRITEL (devc->osdev, READL (devc->osdev, 0x08) & ~(1 << 29), 0x08);
  oss_udelay (10);

  /* do a sync */
  WRITEL (devc->osdev, READL (devc->osdev, 0x08) | (1 << 30), 0x08);
  oss_udelay (10);

  /* now continue with initializing the rest of the chip */
  WRITEL (devc->osdev, READL (devc->osdev, 0x08) | (1 << 25), 0x08);	/*enable burst */

  WRITEL (devc->osdev, 0x22, 0x04);	/* enable audio+spdif interrupts */
  devc->bdlBuffer =
    CONTIG_MALLOC (devc->osdev, MAX_PORTC * BDL_SIZE * sizeof (bdl_t),
		   MEMLIMIT_32BITS, &phaddr, devc->bldbuf_dma_handle);
  if (devc->bdlBuffer == NULL)
    {
      cmn_err (CE_WARN, "OSS Failed to allocate BDL\n");
      return 0;
    }

  devc->playBDL = (bdl_t *) devc->bdlBuffer;
  devc->playBDL_phys = phaddr;
  devc->recBDL =
    (bdl_t *) (devc->bdlBuffer + (1 * BDL_SIZE * sizeof (bdl_t)));
  devc->recBDL_phys = phaddr + (1 * BDL_SIZE * sizeof (bdl_t));
  devc->spdifBDL =
    (bdl_t *) (devc->bdlBuffer + (2 * BDL_SIZE * sizeof (bdl_t)));
  devc->spdifBDL_phys = phaddr + (2 * BDL_SIZE * sizeof (bdl_t));

/*
 * Init mixer
 */
  my_mixer =
    ac97_install_full (&devc->ac97devc, "AC97 Mixer", ac97_read, ac97_write,
		       devc, devc->osdev, devc->inverted);

  if (my_mixer == -1)
    return 0;			/* No mixer */

  devc->mixer_dev = my_mixer;


  /* enable S/PDIF */
  devc->ac97devc.spdif_slot = SPDIF_SLOT1011;
  ac97_spdifout_ctl (devc->mixer_dev, SPDIFOUT_ENABLE, SNDCTL_MIX_WRITE, 1);

  for (i = 0; i < MAX_PORTC; i++)
    {
      ATI_portc *portc = &devc->portc[i];
      char tmp_name[100];
      int formats = AFMT_S16_LE | AFMT_AC3;
      char *devfile_name = "";
      strcpy (tmp_name, devc->chip_name);
      opts = ADEV_AUTOMODE | ADEV_STEREOONLY | ADEV_FIXEDRATE;
      portc->port_type = DF_ANALOG;

      if (i == 0)
	{
	  opts |= ADEV_DUPLEX;
	  strcpy (tmp_name, devc->chip_name);
	}
      if (i == 1)
	{
	  strcpy (tmp_name, devc->chip_name);
	  opts |= ADEV_DUPLEX | ADEV_SHADOW;
	}
      if (i == 2)
	{
	  sprintf (tmp_name, "%s (SPDIF out)", devc->chip_name);
	  opts |= ADEV_NOINPUT | ADEV_SPECIAL;
	  portc->port_type = DF_SPDIF;
	  devfile_name = "spdout";
	}

      if ((my_dev = oss_install_audiodev_with_devname (OSS_AUDIO_DRIVER_VERSION,
					  devc->osdev,
					  devc->osdev,
					  tmp_name,
					  &ATI_audio_driver,
					  sizeof (audiodrv_t),
					  opts, formats, devc, -1,
					  devfile_name)) < 0)
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
	  audio_engines[my_dev]->min_channels = 2;
	  audio_engines[my_dev]->max_channels = 6;

	  /*
	   * We can have at most BDL_SIZE fragments and they can
	   * be at most 128k each.
	   */
	  audio_engines[my_dev]->max_block = 128 * 1024;
	  audio_engines[my_dev]->max_fragments = BDL_SIZE;
	  portc->open_mode = 0;
	  portc->audio_enabled = 0;
	  portc->audiodev = my_dev;
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
oss_atiaudio_attach (oss_device_t * osdev)
{
  unsigned char pci_irq_line, pci_revision /*, pci_latency */ ;
  unsigned short pci_command, vendor, device, sub_vendor, sub_id;
  unsigned int pci_ioaddr;
  ATI_devc *devc;

  DDB (cmn_err (CE_WARN, "Entered ATI AC97 probe routine\n"));

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  if ((vendor != ATI_VENDOR_ID) || (device != ATI_DEVICE_IXP &&
				    device != ATI_DEVICE_IXP300
				    && device != ATI_DEVICE_IXP400))
    return 0;

  oss_pci_byteswap (osdev, 1);

  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, PCI_MEM_BASE_ADDRESS_0, &pci_ioaddr);

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

  pci_read_config_word (osdev, PCI_SUBSYSTEM_VENDOR_ID, &sub_vendor);
  pci_read_config_word (osdev, PCI_SUBSYSTEM_ID, &sub_id);
  switch ((sub_id << 16) | sub_vendor)
    {
      case 0x11831043:  /* ASUS A6R */
      case 0x2043161f:  /* Maxselect x710s */
      case 0x0506107b:  /* Gateway 7510GX */
	devc->inverted = AC97_INVERTED;
	cmn_err (CE_CONT, "An inverted amplifier has been autodetected\n");
	break;
      default:
	devc->inverted = 0;
	break;
    }

  devc->membar_addr = pci_ioaddr;
  devc->membar_virt =
    (char *) MAP_PCI_MEM (devc->osdev, 0, devc->membar_addr, 256);

  if (devc->membar_virt == NULL)
    {
      cmn_err (CE_WARN, "ATIIXP: Cannot map pci mem\n");
      return 0;
    }

  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

#if 0
  latency = pci_read_config_dword (osdev, 0x50, &latency);
  pci_write_config_dword (osdev, 0x50, latency | 1);
#endif

  switch (device)
    {
    case ATI_DEVICE_IXP:
      devc->chip_name = "ATI IXP200";
      break;

    case ATI_DEVICE_IXP300:
      devc->chip_name = "ATI IXP300";
      break;

    case ATI_DEVICE_IXP400:
      devc->chip_name = "ATI IXP400";
      break;

    default:
      devc->chip_name = "ATI IXP";
      break;
    }

  devc->irq = pci_irq_line;
  devc->open_mode = 0;

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);

  oss_register_device (osdev, devc->chip_name);

  if (oss_register_interrupts (devc->osdev, 0, ATIIXPintr, NULL) < 0)
    {
      cmn_err (CE_WARN, "Unable to register interrupts\n");
      return 0;
    }

  return init_ATI (devc);	/* Detected */
}


int
oss_atiaudio_detach (oss_device_t * osdev)
{
  ATI_devc *devc = (ATI_devc *) osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  WRITEL (devc->osdev, 0x3f, 0x00);	/* ack all interrupts */
  WRITEL (devc->osdev, 0, 0x04);	/* disable interrupts */

  /* disable S/PDIF */
  if (devc->mixer_dev)
    ac97_spdifout_ctl (devc->mixer_dev, SPDIFOUT_ENABLE, SNDCTL_MIX_WRITE, 0);

  oss_unregister_interrupts (devc->osdev);

  if (devc->bdlBuffer)
    CONTIG_FREE (devc->osdev, devc->bdlBuffer,
		 MAX_PORTC * BDL_SIZE * sizeof (bdl_t), devc->bldbuf_dma_handle);

  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);

  if (devc->membar_addr != 0)
    {
      UNMAP_PCI_MEM (devc->osdev, 0, devc->membar_addr, devc->membar_virt,
		     256);
      devc->membar_addr = 0;
    }

  oss_unregister_device (devc->osdev);
  return 1;

}
