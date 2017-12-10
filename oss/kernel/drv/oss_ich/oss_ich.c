/*
 * Purpose: Driver for the Intel ICH AC97 audio controller
 *
 * The same design is also used in many PC chipsets by nVidia, AMD and SiS for
 * audio functionality.
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

#include "oss_ich_cfg.h"
#include <oss_pci.h>

#include <ac97.h>
extern int intelpci_force_mmio;

#define INTEL_VENDOR_ID         0x8086
#define SIS_VENDOR_ID           0x1039
#define NVIDIA_VENDOR_ID        0x10de
#define AMD_VENDOR_ID           0x1022
#define SIS_DEVICE_7012         0x7012
#define INTEL_DEVICE_ICH1       0x2415
#define INTEL_DEVICE_ICH1R1     0x2425
#define INTEL_DEVICE_ICH1R2     0x7195
#define INTEL_DEVICE_ICH2       0x2445
#define INTEL_DEVICE_ICH3       0x2485
#define INTEL_DEVICE_ICH4       0x24c5
#define INTEL_DEVICE_ICH5       0x24d5
#define INTEL_DEVICE_ESB        0x25a6
#define INTEL_DEVICE_ICH6       0x266e
#define INTEL_DEVICE_ICH7       0x27de
#define NVIDIA_DEVICE_NFORCE    0x01b1
#define NVIDIA_DEVICE_MCP4	0x003a
#define NVIDIA_DEVICE_NFORCE2   0x006a
#define	NVIDIA_DEVICE_CK8	0x008a
#define NVIDIA_DEVICE_NFORCE3   0x00da
#define NVIDIA_DEVICE_CK8S      0x00ea
#define	NVIDIA_DEVICE_NFORCE4	0x0059
#define	NVIDIA_DEVICE_MCP51	0x026b
#define AMD_DEVICE_768          0x7445
#define AMD_DEVICE_8111         0x746d

extern int intelpci_rate_tuning;

#define MAX_PORTC 3
#define BDL_SIZE	32

extern int ich_jacksense;

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
ich_portc;

typedef struct
{
  unsigned int addr;
  unsigned short size;
  unsigned short flags;
}
bdl_t;

typedef struct ich_devc
{
  oss_device_t *osdev;
  oss_native_word base, ac97_base;
  oss_native_word membar_addr, ac97_membar_addr;
  char *membar_virt, *ac97_membar_virt;
#define CTL_BASE   0		/* addressing controller regs */
#define MIXER_BASE 1		/* addressing mixer regs */
  int mem_mode;
#define MMAP_MODE  0		/* ICH4/ICH5 uses MEM BARS */
#define IO_MODE    1		/* ICH1/2/3/4/5 uses IO BARS */

  int irq;
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;

  /* Mixer */
  ac97_devc ac97devc;
  int mixer_dev;
  int inverted_amplifier;

  /* Audio parameters */
  int open_mode;
  int fifo_errors;

  /* Buffer Descriptor List */
  char *bdlBuffer;
  bdl_t *playBDL, *recBDL, *spdifBDL;
  oss_native_word playBDL_phys, recBDL_phys, spdifBDL_phys;
  oss_dma_handle_t bldbuf_dma_handle;

  int play_currbuf, play_currfrag;
  int spdif_currbuf, spdif_currfrag;
  int rec_currbuf, rec_currfrag;
#define INTEL_ICH1 0
#define INTEL_ICH3 1
#define INTEL_ICH4 2
#define SIS_7012   3
#define AMD_768    4
#define AMD_8111   5
#define NVIDIA_NFORCE  6
#define NVIDIA_NFORCE2 7
  int model;
  char *chip_name;
  ich_portc portc[MAX_PORTC];
  int play_frag_index[BDL_SIZE];
  int rec_frag_index[BDL_SIZE];
  int spdif_frag_index[BDL_SIZE];
}
ich_devc;

static unsigned int
ich_INL (ich_devc * devc, int base, unsigned int a)
{
  if (devc->mem_mode == MMAP_MODE)
    {
      if (base == CTL_BASE)
	return *(volatile unsigned int *) (devc->membar_virt + (a));
      else
	return *(volatile unsigned int *) (devc->ac97_membar_virt + (a));
    }
  else
    {
      if (base == CTL_BASE)
	return INL (devc->osdev, devc->base + a);
      else
	return INL (devc->osdev, devc->ac97_base + a);
    }
}

static unsigned short
ich_INW (ich_devc * devc, int base, unsigned short a)
{
  if (devc->mem_mode == MMAP_MODE)
    {
      if (base == CTL_BASE)
	return *(volatile unsigned short *) (devc->membar_virt + (a));
      else
	return *(volatile unsigned short *) (devc->ac97_membar_virt + (a));
    }
  else
    {
      if (base == CTL_BASE)
	return INW (devc->osdev, devc->base + a);
      else
	return INW (devc->osdev, devc->ac97_base + a);
    }
}

static unsigned char
ich_INB (ich_devc * devc, int base, unsigned char a)
{
  if (devc->mem_mode == MMAP_MODE)
    {
      if (base == CTL_BASE)
	return *(volatile unsigned char *) (devc->membar_virt + (a));
      else
	return *(volatile unsigned char *) (devc->ac97_membar_virt + (a));
    }
  else
    {
      if (base == CTL_BASE)
	return INB (devc->osdev, devc->base + a);
      else
	return INB (devc->osdev, devc->ac97_base + a);
    }
}

static void
ich_OUTL (ich_devc * devc, unsigned int d, int base, unsigned int a)
{
  if (devc->mem_mode == MMAP_MODE)
    {
      if (base == CTL_BASE)
	*(volatile unsigned int *) (devc->membar_virt + (a)) = d;
      else
	*(volatile unsigned int *) (devc->ac97_membar_virt + (a)) = d;
    }
  else
    {
      if (base == CTL_BASE)
	OUTL (devc->osdev, d, devc->base + a);
      else
	OUTL (devc->osdev, d, devc->ac97_base + a);
    }
}

static void
ich_OUTW (ich_devc * devc, unsigned short d, int base, unsigned short a)
{
  if (devc->mem_mode == MMAP_MODE)
    {
      if (base == CTL_BASE)
	*(volatile unsigned short *) (devc->membar_virt + (a)) = d;
      else
	*(volatile unsigned short *) (devc->ac97_membar_virt + (a)) = d;
    }
  else
    {
      if (base == CTL_BASE)
	OUTW (devc->osdev, d, devc->base + a);
      else
	OUTW (devc->osdev, d, devc->ac97_base + a);
    }
}

static void
ich_OUTB (ich_devc * devc, unsigned char d, int base, unsigned char a)
{
  if (devc->mem_mode == MMAP_MODE)
    {
      if (base == CTL_BASE)
	*(volatile unsigned char *) (devc->membar_virt + (a)) = d;
      else
	*(volatile unsigned char *) (devc->ac97_membar_virt + (a)) = d;
    }
  else
    {
      if (base == CTL_BASE)
	OUTB (devc->osdev, d, devc->base + a);
      else
	OUTB (devc->osdev, d, devc->ac97_base + a);
    }
}

static int
ac97_read (void *devc_, int reg)
{
  ich_devc *devc = devc_;
  int i = 100, status;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  status = ich_INB (devc, CTL_BASE, 0x34);

  while (status & 0x01 && i-- > 0)
    {
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      oss_udelay (10);
      MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
      status = ich_INB (devc, CTL_BASE, 0x34);
    }

  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return ich_INW (devc, MIXER_BASE, reg);
}

static int
ac97_write (void *devc_, int reg, int data)
{
  ich_devc *devc = devc_;
  int i = 100, status;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  status = ich_INB (devc, CTL_BASE, 0x34);

  while (status & 0x01 && i-- > 0)
    {
      MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
      oss_udelay (10);
      MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
      status = ich_INB (devc, CTL_BASE, 0x34);
    }

  ich_OUTW (devc, data, MIXER_BASE, reg);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return 1;
}

/*
 * The top half interrupt handler
 */
static int
ichintr (oss_device_t * osdev)
{
  int serviced = 0;
  ich_devc *devc = osdev->devc;
  ich_portc *portc;
  unsigned int glob_status, status, p, f;
  oss_native_word flags;
  int i;

  flags = 0;			/* To prevent compiler warnings */
  MUTEX_ENTER (devc->mutex, flags);
  /* Get pending interrupts and acknowledge them */
  glob_status = ich_INL (devc, CTL_BASE, 0x30);
  ich_OUTL (devc, glob_status, CTL_BASE, 0x30);

  /*
   * Check the interrupt bits of the status register
   */
  if (!(glob_status & 0x0cf7))
    {
      /* Not for me */
      MUTEX_EXIT (devc->mutex, flags);
      return 0;
    }

  /*-------------------- Handle PCM -------------------*/
  if (devc->model == SIS_7012)
    status = ich_INB (devc, CTL_BASE, 0x18);
  else
    status = ich_INB (devc, CTL_BASE, 0x16);

  if (status & 0x10)		/* FIFO error */
    devc->fifo_errors++;

  if (status & 0x08)
    {
      for (i = 0; i < MAX_PORTC - 1; i++)
	{
	  portc = &devc->portc[i];
	  serviced = 1;
	  if ((portc->trigger_bits & PCM_ENABLE_OUTPUT))	/* IOC interrupt */
	    {
	      dmap_t *dmap = audio_engines[portc->audiodev]->dmap_out;
	      p = ich_INB (devc, CTL_BASE, 0x14);

	      if (p != devc->play_currbuf)
		{
		  p = devc->play_currbuf;
		  f = devc->play_currfrag;
		  devc->playBDL[p].addr =
		    dmap->dmabuf_phys + (f * dmap->fragment_size);

		  /* SIS uses bytes, intelpci uses samples */
		  if (devc->model == SIS_7012)
		    devc->playBDL[p].size = (dmap->fragment_size);
		  else
		    devc->playBDL[p].size = (dmap->fragment_size / 2);
		  devc->playBDL[p].flags = 0xc000;	/* IOC interrupts */

		  ich_OUTB (devc, p, CTL_BASE, 0x15);	/* Set LVD */
		  devc->play_frag_index[p] = f;
		  devc->play_currbuf = (p + 1) % BDL_SIZE;
		  devc->play_currfrag = (f + 1) % dmap->nfrags;
		}
	      oss_audio_outputintr (portc->audiodev, 1);
	    }
	}
      if (devc->model == SIS_7012)
	ich_OUTB (devc, status, CTL_BASE, 0x18);	/* Clear interrupts */
      else
	ich_OUTB (devc, status, CTL_BASE, 0x16);	/* Clear interrupts */
    }

  /*------------------- handle SPDIF interrupts -------------------------*/

  if (devc->model == NVIDIA_NFORCE2)
    {
      status = ich_INB (devc, CTL_BASE, 0x76);
      if (status & 0x08)
	{
	  portc = &devc->portc[2];
	  serviced = 1;
	  if ((portc->trigger_bits & PCM_ENABLE_OUTPUT))	/* IOC interrupt */
	    {
	      dmap_t *dmap = audio_engines[portc->audiodev]->dmap_out;
	      p = ich_INB (devc, CTL_BASE, 0x74);

	      if (p != devc->spdif_currbuf)
		{
		  p = devc->spdif_currbuf;
		  f = devc->spdif_currfrag;
		  devc->spdifBDL[p].addr =
		    dmap->dmabuf_phys + (f * dmap->fragment_size);
		  /* SIS uses bytes, intelpci uses samples */
		  devc->spdifBDL[p].size = (dmap->fragment_size / 2);
		  devc->spdifBDL[p].flags = 0xc000;	/* IOC interrupts */

		  ich_OUTB (devc, p, CTL_BASE, 0x75);	/* Set LVD */
		  devc->spdif_frag_index[p] = f;
		  devc->spdif_currbuf = (p + 1) % BDL_SIZE;
		  devc->spdif_currfrag = (f + 1) % dmap->nfrags;
		}
	      oss_audio_outputintr (portc->audiodev, 1);
	    }
	  ich_OUTB (devc, status, CTL_BASE, 0x76);	/* Clear interrupts */
	}
    }

  /*----------------------- Handle Recording Interrupts --------------------*/

  if (devc->model == SIS_7012)
    status = ich_INB (devc, CTL_BASE, 0x08);
  else
    status = ich_INB (devc, CTL_BASE, 0x06);

  if (status & 0x08)
    {
      for (i = 0; i < MAX_PORTC - 1; i++)
	{
	  portc = &devc->portc[i];
	  serviced = 1;
	  if ((portc->trigger_bits & PCM_ENABLE_INPUT))	/* IOC interrupt */
	    {
	      dmap_t *dmap = audio_engines[portc->audiodev]->dmap_in;
	      p = ich_INB (devc, CTL_BASE, 0x04);

	      if (p != devc->rec_currbuf)
		{
		  p = devc->rec_currbuf;
		  f = devc->rec_currfrag;
		  devc->recBDL[p].addr =
		    dmap->dmabuf_phys + (f * dmap->fragment_size);

		  /* SIS uses bytes, intelpci uses samples */
		  if (devc->model == SIS_7012)
		    devc->recBDL[p].size = (dmap->fragment_size);
		  else
		    devc->recBDL[p].size = (dmap->fragment_size / 2);

		  devc->recBDL[p].flags = 0xc000;	/* IOC interrupts */

		  ich_OUTB (devc, p, CTL_BASE, 0x05);	/* Set LVD */
		  devc->rec_frag_index[p] = f;
		  devc->rec_currbuf = (p + 1) % BDL_SIZE;
		  devc->rec_currfrag = (f + 1) % dmap->nfrags;
		}
	      oss_audio_inputintr (portc->audiodev, 0);
	    }
	}
      if (devc->model == SIS_7012)
	ich_OUTB (devc, status, CTL_BASE, 0x08);	/* Clear int */
      else
	ich_OUTB (devc, status, CTL_BASE, 0x06);	/* Clear int */
    }
  MUTEX_EXIT (devc->mutex, flags);
  return serviced;
}


/*
 * Audio routines
 */

static int
ich_audio_set_rate (int dev, int arg)
{
  ich_portc *portc = audio_engines[dev]->portc;

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
ich_audio_set_channels (int dev, short arg)
{
  ich_portc *portc = audio_engines[dev]->portc;

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
ich_audio_set_format (int dev, unsigned int arg)
{
  ich_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->bits;

#if 1
  if (portc->open_mode & OPEN_READ)
    return portc->bits = AFMT_S16_LE;
#endif
  if (!(arg & (AFMT_S16_LE | AFMT_AC3)))
    return portc->bits;
  portc->bits = arg;

  return portc->bits;
}

/*ARGSUSED*/
static int
ich_audio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void ich_audio_trigger (int dev, int state);

static void
ich_audio_reset (int dev)
{
  ich_audio_trigger (dev, 0);
}

static void
ich_audio_reset_input (int dev)
{
  ich_portc *portc = audio_engines[dev]->portc;
  ich_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
ich_audio_reset_output (int dev)
{
  ich_portc *portc = audio_engines[dev]->portc;
  ich_audio_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

/*ARGSUSED*/
static int
ich_audio_open (int dev, int mode, int openflags)
{
  ich_portc *portc = audio_engines[dev]->portc;
  ich_devc *devc = audio_engines[dev]->devc;
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
	  cmn_err (CE_NOTE, "The S/PDIF device supports only playback\n");
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
  devc->fifo_errors = 0;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

static void
ich_audio_close (int dev, int mode)
{
  ich_portc *portc = audio_engines[dev]->portc;
  ich_devc *devc = audio_engines[dev]->devc;

  ich_audio_reset (dev);
  portc->open_mode = 0;

  if (devc->fifo_errors > 0)
    cmn_err (CE_CONT, "%d fifo errors were detected\n", devc->fifo_errors);

  if (portc->port_type != DF_SPDIF)
    devc->open_mode &= ~mode;
  portc->audio_enabled &= ~mode;
}

/*ARGSUSED*/
static void
ich_audio_output_block (int dev, oss_native_word buf, int count,
			int fragsize, int intrflag)
{
  ich_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
}

/*ARGSUSED*/
static void
ich_audio_start_input (int dev, oss_native_word buf, int count,
		       int fragsize, int intrflag)
{
  ich_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
}

static void
ich_audio_trigger (int dev, int state)
{
  ich_devc *devc = audio_engines[dev]->devc;
  ich_portc *portc = audio_engines[dev]->portc;
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
		ich_OUTB (devc, 0x1d, CTL_BASE, 0x7b);	/* Kickstart */
	      else
		ich_OUTB (devc, 0x1d, CTL_BASE, 0x1b);	/* Kickstart */
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
		ich_OUTB (devc, 0x00, CTL_BASE, 0x7b);	/* reset */
	      else
		ich_OUTB (devc, 0x00, CTL_BASE, 0x1b);	/* reset */
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
	      ich_OUTB (devc, 0x1d, CTL_BASE, 0x0b);	/* Kickstart */
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
	      ich_OUTB (devc, 0x00, CTL_BASE, 0x0b);	/* reset */
	    }
	}
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
static int
ich_audio_prepare_for_input (int dev, int bsize, int bcount)
{
  ich_devc *devc = audio_engines[dev]->devc;
  ich_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_in;
  int i, n, speed;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  ich_OUTB (devc, 0x02, CTL_BASE, 0x0b);	/* Reset */
  ich_OUTL (devc, devc->recBDL_phys, CTL_BASE, 0x00);	/* BDL base */

  speed = portc->speed;
  speed = (speed * 240) / intelpci_rate_tuning;
  ac97_recrate (&devc->ac97devc, speed);

  n = bcount;
  if (n > BDL_SIZE)
    n = BDL_SIZE;

  for (i = 0; i < n; i++)
    {
      devc->recBDL[i].addr = dmap->dmabuf_phys + (i * dmap->fragment_size);

      /* SiS7012 uses bytes, ICH uses samples */
      if (devc->model == SIS_7012)
	devc->recBDL[i].size = (dmap->fragment_size);
      else
	devc->recBDL[i].size = (dmap->fragment_size / 2);

      devc->recBDL[i].flags = 0xc000;	/* IOC interrupts */
      devc->rec_frag_index[i] = i;
    }
  ich_OUTB (devc, n - 1, CTL_BASE, 0x05);	/* Set last valid descriptor */

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
ich_audio_prepare_for_output (int dev, int bsize, int bcount)
{
  ich_devc *devc = audio_engines[dev]->devc;
  ich_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_out;
  int i, n, speed;
  unsigned int tmp;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  /* We need to add ac3 pass through support */
  if (devc->model == SIS_7012)
    {
      if (portc->bits == AFMT_AC3)
	ich_OUTB (devc, 0, CTL_BASE, 0x4c);
      else
	ich_OUTB (devc, 1, CTL_BASE, 0x4c);
    }

  ac97_spdif_setup (devc->mixer_dev, portc->speed, portc->bits);

  if (portc->bits == AFMT_AC3)
    {
      portc->channels = 2;
      portc->bits = 16;
    }

  /* do SPDIF out */
  if ((portc->port_type == DF_SPDIF) && (devc->model == NVIDIA_NFORCE2))
    {
      ich_OUTB (devc, 0x02, CTL_BASE, 0x7b);	/* Reset */
      ich_OUTL (devc, devc->spdifBDL_phys, CTL_BASE, 0x70);	/* BDL base */
      n = bcount;
      if (n > BDL_SIZE)
	n = BDL_SIZE;

      for (i = 0; i < n; i++)
	{
	  devc->spdifBDL[i].addr =
	    dmap->dmabuf_phys + (i * dmap->fragment_size);
	  devc->spdifBDL[i].size = (dmap->fragment_size / 2);
	  devc->spdifBDL[i].flags = 0xc000;	/* IOC interrupts */
	  devc->spdif_frag_index[i] = i;
	}
      ich_OUTB (devc, n - 1, CTL_BASE, 0x75);	/* Set last valid
						 * descriptor */
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
  ich_OUTB (devc, 0x02, CTL_BASE, 0x1b);	/* Reset */
  ich_OUTL (devc, devc->playBDL_phys, CTL_BASE, 0x10);	/* BDL base */

  speed = portc->speed;
  speed = (speed * 240) / intelpci_rate_tuning;
  ac97_playrate (&devc->ac97devc, speed);

  /* Handle 4/6 channel output on 7012 */
  if (devc->model == SIS_7012)
    {
      tmp = ich_INL (devc, CTL_BASE, 0x30);

      /* set default to 2 channel mode */
      ich_OUTB (devc, ich_INB (devc, CTL_BASE, 0x2c) & 0x3f, CTL_BASE, 0x2c);

      if ((portc->channels == 4) && (tmp & (1 << 20)))
	ich_OUTB (devc, (ich_INB (devc, CTL_BASE, 0x2c) & 0x3f) | 0x40,
		  CTL_BASE, 0x2c);

      if ((portc->channels == 6) && (tmp & (1 << 21)))
	ich_OUTB (devc, (ich_INB (devc, CTL_BASE, 0x2c) & 0x3f) | 0x80,
		  CTL_BASE, 0x2c);
    }
  /* Handle 4/6 channel output on evrything other than ICH1 and SIS7012 */
  if ((devc->model != INTEL_ICH1) && (devc->model != SIS_7012))
    {
      tmp = ich_INL (devc, CTL_BASE, 0x30);

      /* set default to 2 channel mode */
      ich_OUTL (devc, ich_INL (devc, CTL_BASE, 0x2c) & 0x0cfffff, CTL_BASE,
		0x2c);

      if ((portc->channels == 4) && (tmp & (1 << 20)))
	ich_OUTL (devc,
		  (ich_INL (devc, CTL_BASE, 0x2c) & 0x00fffff) | 0x0100000,
		  CTL_BASE, 0x2c);

      if ((portc->channels == 6) && (tmp & (1 << 21)))
	ich_OUTL (devc,
		  (ich_INL (devc, CTL_BASE, 0x2c) & 0x00fffff) | 0x0200000,
		  CTL_BASE, 0x2c);
    }
  n = bcount;
  if (n > BDL_SIZE)
    n = BDL_SIZE;

  for (i = 0; i < n; i++)
    {
      devc->playBDL[i].addr = dmap->dmabuf_phys + (i * dmap->fragment_size);

      /* SiS7012 uses bytes, ICH uses samples */
      if (devc->model == SIS_7012)
	devc->playBDL[i].size = (dmap->fragment_size);
      else
	devc->playBDL[i].size = (dmap->fragment_size / 2);

      devc->playBDL[i].flags = 0xc000;	/* IOC interrupts */
      devc->play_frag_index[i] = i;
    }
  ich_OUTB (devc, n - 1, CTL_BASE, 0x15);	/* Set last valid descriptor */


  devc->play_currbuf = n % BDL_SIZE;
  devc->play_currfrag = n;
  if (devc->play_currfrag >= dmap->nfrags)
    devc->play_currfrag = 0;

  portc->audio_enabled &= ~PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

static int
ich_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  ich_devc *devc = audio_engines[dev]->devc;
  ich_portc *portc = audio_engines[dev]->portc;
  int p = 0, f = 0, c = 0;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  if (direction == PCM_ENABLE_OUTPUT)
    {
      if (portc->port_type == DF_PCM)
	{
	  f = ich_INB (devc, CTL_BASE, 0x14);	/* Current buffer */

	  if (devc->model == SIS_7012)
	    p = ich_INW (devc, CTL_BASE, 0x16);	/* Position in current
						 * buffer */
	  else
	    p = ich_INW (devc, CTL_BASE, 0x18);	/* Position in current
						 * buffer */
	  c = devc->play_frag_index[f];	/* Current fragment */

	  if (devc->model == SIS_7012)
	    p = dmap->fragment_size - (p);	/* Remaining bytes */
	  else
	    p = dmap->fragment_size - (p * 2);	/* Remaining bytes */
	}
      if ((portc->port_type == DF_SPDIF) && (devc->model == NVIDIA_NFORCE2))
	{
	  f = ich_INB (devc, CTL_BASE, 0x74);	/* Current buffer */
	  p = ich_INW (devc, CTL_BASE, 0x78);	/* Position in current
						 * buffer */
	  c = devc->play_frag_index[f];	/* Current fragment */
	  p = dmap->fragment_size - (p * 2);	/* Remaining bytes */
	}
    }
  /*
   * Handle input
   */
  if (direction == PCM_ENABLE_INPUT)
    {

      f = ich_INB (devc, CTL_BASE, 0x04);	/* Current buffer */

      if (devc->model == SIS_7012)
	p = ich_INW (devc, CTL_BASE, 0x06);	/* Position in current
						 * buffer */
      else
	p = ich_INW (devc, CTL_BASE, 0x08);	/* Position in current
						 * buffer */
      c = devc->rec_frag_index[f];	/* Current fragment */

      if (devc->model == SIS_7012)
	p = dmap->fragment_size - (p);	/* Remaining bytes */
      else
	p = dmap->fragment_size - (p * 2);	/* Remaining bytes */
    }
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
  return p + c * dmap->fragment_size;
}

#if 0
static int
ich_calibrate_speed (int dev, int nominal_speed, int true_speed)
{
  unsigned int fix;
  DDB (smn_err (CE_CONT,
		"ich_calibrate_speed(%d, %d, %d)\n", dev, nominal_speed,
		true_speed));

  fix = ((240 * true_speed) + nominal_speed / 2) / nominal_speed;
  DDB (cmn_err (CE_NOTE, "intelpci_rate_tuning = %d\n", fix));
  if (fix > 1)
    intelpci_rate_tuning = fix;

  return 0;
}
#endif

static const audiodrv_t ich_audio_driver = {
  ich_audio_open,
  ich_audio_close,
  ich_audio_output_block,
  ich_audio_start_input,
  ich_audio_ioctl,
  ich_audio_prepare_for_input,
  ich_audio_prepare_for_output,
  ich_audio_reset,
  NULL,
  NULL,
  ich_audio_reset_input,
  ich_audio_reset_output,
  ich_audio_trigger,
  ich_audio_set_rate,
  ich_audio_set_format,
  ich_audio_set_channels,
  NULL,
  NULL,
  NULL,				/* ich_check_input, */
  NULL,				/* ich_check_output, */
  NULL,				/* ich_alloc_buffer, */
  NULL,				/* ich_free_buffer, */
  NULL,
  NULL,
  ich_get_buffer_pointer,
  NULL				/* ich_calibrate_speed */
};

static int
ich_init (ich_devc * devc)
{
  int my_mixer, adev, opts;
  int i, max_port;
  unsigned int reg;
  int first_dev = 0;
  oss_native_word phaddr;

  /* ACLink on, warm reset */
  reg = ich_INL (devc, CTL_BASE, 0x2c);
  if ((reg & 0x02) == 0)
    reg |= 2;
  else
    reg |= 4;
  reg &= ~8;
  ich_OUTL (devc, reg, CTL_BASE, 0x2c);
  oss_udelay (500);
  if (devc->model == SIS_7012)
    {
      reg |= 0x10;
      ich_OUTL (devc, reg, CTL_BASE, 0x2c);
      oss_udelay (500);
    }
  /* disable interrupts */
  ich_OUTB (devc, 0x00, CTL_BASE, 0x0b);
  ich_OUTB (devc, 0x00, CTL_BASE, 0x1b);
  ich_OUTB (devc, 0x00, CTL_BASE, 0x2b);
  ich_OUTB (devc, 0x00, CTL_BASE, 0x7b);

  devc->bdlBuffer =
    CONTIG_MALLOC (devc->osdev, 4 * 32 * 32, MEMLIMIT_32BITS, &phaddr, devc->bldbuf_dma_handle);
  if (devc->bdlBuffer == NULL)
    {
      cmn_err (CE_WARN, "Failed to allocate BDL\n");
      return 0;
    }
  devc->playBDL = (bdl_t *) devc->bdlBuffer;
  devc->playBDL_phys = phaddr;
  devc->recBDL = (bdl_t *) (devc->bdlBuffer + (32 * 32));
  devc->recBDL_phys = phaddr + 32 * 32;
  devc->spdifBDL = (bdl_t *) (devc->bdlBuffer + (2 * 32 * 32));
  devc->spdifBDL_phys = phaddr + 2 * 32 * 32;

  /*
   * Init mixer
   */
  my_mixer =
    ac97_install_full (&devc->ac97devc, "ICH AC97 Mixer", ac97_read, ac97_write,
		       devc, devc->osdev, devc->inverted_amplifier |
		       (ich_jacksense?AC97_FORCE_SENSE:0));
  if (my_mixer == -1)
    {
      cmn_err (CE_WARN, "AC97 mixer installation failed\n");
      return 0;			/* No mixer */
    }

  devc->mixer_dev = my_mixer;
  mixer_devs[my_mixer]->priority = 10;	/* Known motherboard device */

  /* enable S/PDIF */
  devc->ac97devc.spdif_slot = SPDIF_SLOT34;
  ac97_spdifout_ctl (devc->mixer_dev, SPDIFOUT_ENABLE, SNDCTL_MIX_WRITE, 1);

  /* enable variable rate mode */
  ac97_write (devc, 0x2a, ac97_read (devc, 0x2a) | 9);
  if (!(ac97_read (devc, 0x2a) & 1))
    DDB (cmn_err (CE_NOTE, "VRA not supported...using GRC\n"));

  /* Enable SPDIF for SiS 7012 */
  if (devc->model == SIS_7012)
    {
      ich_OUTL (devc, ich_INL (devc, CTL_BASE, 0x2c) | (1 << 10), CTL_BASE,
		0x2c);
      ich_OUTL (devc, ich_INL (devc, CTL_BASE, 0x4c) | 1, CTL_BASE, 0x4c);
    }
  if (devc->model == NVIDIA_NFORCE2)
    max_port = 3;
  else
    max_port = 2;

  for (i = 0; i < max_port; i++)
    {
      ich_portc *portc = &devc->portc[i];
      char tmp_name[100];
      int port_fmt = DF_PCM;
      int formats = AFMT_S16_LE | AFMT_AC3;
      char *devfile_name = "";

      strcpy (tmp_name, devc->chip_name);
      opts = ADEV_AUTOMODE | ADEV_16BITONLY | ADEV_STEREOONLY | ADEV_COLD;

      if (!ac97_varrate (&devc->ac97devc))
	{
	  opts |= ADEV_FIXEDRATE;
	}
      if (i == 0)
	{
	  strcpy (tmp_name, devc->chip_name);
	  opts |= ADEV_DUPLEX;
	}
      if (i == 1)
	{
	  strcpy (tmp_name, devc->chip_name);
	  opts |= ADEV_DUPLEX | ADEV_SHADOW;
	}
      if (i == 2)
	{
	  sprintf (tmp_name, "%s S/PDIF out", devc->chip_name);
	  opts |= ADEV_NOINPUT | ADEV_SPECIAL | ADEV_FIXEDRATE;
	  port_fmt = DF_SPDIF;
	  devfile_name = "spdout";
	}
      if ((adev = oss_install_audiodev_with_devname (OSS_AUDIO_DRIVER_VERSION,
					devc->osdev,
					devc->osdev,
					tmp_name,
					&ich_audio_driver,
					sizeof (audiodrv_t),
					opts, formats, devc, -1,
					devfile_name)) < 0)
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
	  audio_engines[adev]->max_block = 64 * 1024;

	  /* fix a timeout bug with Nforce2 */
	  if ((devc->model == NVIDIA_NFORCE) ||
	      (devc->model == NVIDIA_NFORCE2))
	    {
	      audio_engines[adev]->min_block = 4096;
	      audio_engines[adev]->max_block = 4096;
	    }

	  audio_engines[adev]->min_rate =
	    (opts & ADEV_FIXEDRATE) ? 48000 : 5000;
	  audio_engines[adev]->max_rate = 48000;
	  audio_engines[adev]->caps |= PCM_CAP_FREERATE;
	  audio_engines[adev]->min_channels = 2;
	  audio_engines[adev]->max_channels = 6;
	  portc->open_mode = 0;
	  portc->audio_enabled = 0;
	  portc->audiodev = adev;
	  portc->port_type = port_fmt;
	  if (audio_engines[adev]->flags & ADEV_FIXEDRATE)
	    audio_engines[adev]->fixed_rate = 48000;
#ifdef CONFIG_OSS_VMIX
	  if (i == 0)
	     vmix_attach_audiodev(devc->osdev, adev, -1, 0);
#endif
	}
    }
  return 1;
}

int
oss_ich_attach (oss_device_t * osdev)
{
  unsigned char pci_irq_line, pci_revision /* , pci_latency */ ;
  unsigned short pci_command, vendor, device, sub_vendor, sub_id;
  unsigned int pci_ioaddr0, pci_ioaddr1;
  unsigned int dw;

  ich_devc *devc;

  if ((devc = PMALLOC (osdev, sizeof (*devc))) == NULL)
    {
      cmn_err (CE_WARN, "Out of memory\n");
      return 0;
    }

  devc->osdev = osdev;
  osdev->devc = devc;
  devc->open_mode = 0;

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  DDB (cmn_err
       (CE_CONT, "oss_ich_attach(Vendor %x, device %x)\n", vendor, device));

#if 0
  // This check is not necessary because the kernel has already checked
  // the vendor&device ID

  if ((vendor != INTEL_VENDOR_ID && vendor != SIS_VENDOR_ID &&
       vendor != NVIDIA_VENDOR_ID && vendor != AMD_VENDOR_ID) ||
      (device != INTEL_DEVICE_ICH1 && device != INTEL_DEVICE_ICH1R1 &&
       device != INTEL_DEVICE_ICH1R2 && device != INTEL_DEVICE_ICH2 &&
       device != INTEL_DEVICE_ICH3 && device != INTEL_DEVICE_ICH4 &&
       device != INTEL_DEVICE_ICH5 && device != INTEL_DEVICE_ESB &&
       device != INTEL_DEVICE_ICH6 && device != INTEL_DEVICE_ICH7 &&
       device != SIS_DEVICE_7012 &&
       device != AMD_DEVICE_768 && device != AMD_DEVICE_8111 &&
       device != NVIDIA_DEVICE_NFORCE && device != NVIDIA_DEVICE_NFORCE2 &&
       device != NVIDIA_DEVICE_NFORCE3 && device != NVIDIA_DEVICE_CK8S &&
       device != NVIDIA_DEVICE_NFORCE4 && device != NVIDIA_DEVICE_CK8 &&
       device != NVIDIA_DEVICE_MCP51 && device != NVIDIA_DEVICE_MCP4
       ))
    {
      cmn_err (CE_WARN, "Hardware not recognized (vendor=%x, dev=%x)\n",
	       vendor, device);
      return 0;
    }
#endif

  pci_read_config_word (osdev, PCI_SUBSYSTEM_VENDOR_ID, &sub_vendor);
  pci_read_config_word (osdev, PCI_SUBSYSTEM_ID, &sub_id);
  dw = (sub_id << 16) | sub_vendor;

  switch (dw)
    {
       case 0x202f161f:	/* Gateway 7326GZ */
       case 0x203a161f:	/* Gateway 4028GZ or 4542GZ */
       case 0x203e161f:	/* Gateway 3520GZ/M210 */
       case 0x204c161f:	/* Kvazar-Micro Senator 3592XT */
       case 0x8144104d:	/* Sony VAIO PCG-TR* */
       case 0x8197104d:	/* Sony S1XP */
       case 0x81c0104d:	/* Sony VAIO type T */
       case 0x81c5104d:	/* Sony VAIO VGN B1VP/B1XP */
       case 0x3089103c:	/* Compaq Presario B3800 */
       case 0x309a103c:	/* HP Compaq nx4300 */
       case 0x82131033:	/* NEC VersaPro VJ10F/BH */
       case 0x82be1033:	/* NEC VersaPro VJ12F/CH */
         devc->inverted_amplifier = AC97_INVERTED;
         cmn_err (CE_CONT, "An inverted amplifier has been autodetected\n");
         break;
       default:
         devc->inverted_amplifier = 0;
         break;
    }

  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_0, &pci_ioaddr0);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_1, &pci_ioaddr1);

  switch (device)
    {
    case INTEL_DEVICE_ICH1:
      devc->model = INTEL_ICH1;
      devc->chip_name = "Intel ICH (2415)";
      break;
    case INTEL_DEVICE_ICH1R1:
      devc->model = INTEL_ICH1;
      devc->chip_name = "Intel ICHR1(2425)";
      break;
    case INTEL_DEVICE_ICH1R2:
      devc->model = INTEL_ICH1;
      devc->chip_name = "Intel ICHR2 (7195)";
      break;
    case INTEL_DEVICE_ICH2:
      devc->model = INTEL_ICH1;
      devc->chip_name = "Intel ICH2 (2445)";
      break;
    case INTEL_DEVICE_ICH3:
      devc->model = INTEL_ICH3;
      devc->chip_name = "Intel ICH3 (2485)";
      break;
    case INTEL_DEVICE_ICH4:
      devc->model = INTEL_ICH4;
      devc->chip_name = "Intel ICH4 (24C5)";
      break;
    case INTEL_DEVICE_ICH5:
      devc->model = INTEL_ICH4;
      devc->chip_name = "Intel ICH5 (24D5)";
      break;
    case INTEL_DEVICE_ICH6:
      devc->model = INTEL_ICH4;
      devc->chip_name = "Intel ICH6 (266E)";
      break;
    case INTEL_DEVICE_ICH7:
      devc->model = INTEL_ICH4;
      devc->chip_name = "Intel ICH7 (27DE)";
      break;
    case INTEL_DEVICE_ESB:
      devc->model = INTEL_ICH4;
      devc->chip_name = "Intel ICH5 (25a6)";
      break;
    case SIS_DEVICE_7012:
      devc->model = SIS_7012;
      devc->chip_name = "SiS 7012";
      break;
    case NVIDIA_DEVICE_NFORCE:
      devc->model = NVIDIA_NFORCE;
      devc->chip_name = "Nvidia nForce";
      break;
    case NVIDIA_DEVICE_NFORCE2:
      devc->model = NVIDIA_NFORCE2;
      devc->chip_name = "Nvidia nForce2";
      pci_read_config_dword (osdev, 0x4c, &dw);
      dw |= 0x1000000;
      pci_write_config_dword (osdev, 0x4c, dw);
      break;
    case NVIDIA_DEVICE_NFORCE3:
      devc->model = NVIDIA_NFORCE2;
      devc->chip_name = "Nvidia nForce3";
      pci_read_config_dword (osdev, 0x4c, &dw);
      dw |= 0x1000000;
      pci_write_config_dword (osdev, 0x4c, dw);
      break;
    case NVIDIA_DEVICE_CK8S:
      devc->model = NVIDIA_NFORCE2;
      devc->chip_name = "Nvidia CK8S";
      pci_read_config_dword (osdev, 0x4c, &dw);
      dw |= 0x1000000;
      pci_write_config_dword (osdev, 0x4c, dw);
      break;
    case NVIDIA_DEVICE_NFORCE4:
      devc->model = NVIDIA_NFORCE2;
      devc->chip_name = "Nvidia nForce4";
      pci_read_config_dword (osdev, 0x4c, &dw);
      dw |= 0x1000000;
      pci_write_config_dword (osdev, 0x4c, dw);
      break;
    case NVIDIA_DEVICE_CK8:
      devc->model = NVIDIA_NFORCE2;
      devc->chip_name = "Nvidia CK8";
      pci_read_config_dword (osdev, 0x4c, &dw);
      dw |= 0x1000000;
      pci_write_config_dword (osdev, 0x4c, dw);
      break;
    case NVIDIA_DEVICE_MCP51:
      devc->model = NVIDIA_NFORCE2;
      devc->chip_name = "Nvidia MCP51";
      pci_read_config_dword (osdev, 0x4c, &dw);
      dw |= 0x1000000;
      pci_write_config_dword (osdev, 0x4c, dw);
      break;
    case NVIDIA_DEVICE_MCP4:
      devc->model = NVIDIA_NFORCE2;
      devc->chip_name = "Nvidia MCP4";
      pci_read_config_dword (osdev, 0x4c, &dw);
      dw |= 0x1000000;
      pci_write_config_dword (osdev, 0x4c, dw);
      break;
    case AMD_DEVICE_768:
      devc->model = AMD_768;
      devc->chip_name = "AMD 768";
      break;
    case AMD_DEVICE_8111:
      devc->model = AMD_8111;
      devc->chip_name = "AMD 8111";
      break;
    default:
      devc->chip_name = "Unknown ICH chip";
    }

  if (((pci_ioaddr1 == 0) || (intelpci_force_mmio)) &&
      (devc->model == INTEL_ICH4))
    {
      unsigned int ioaddr;

      /* read bar2 and bar3 for getting mmap address */
      pci_read_config_dword (osdev, PCI_MEM_BASE_ADDRESS_2, &ioaddr);
      devc->ac97_membar_addr = ioaddr;
      pci_read_config_dword (osdev, PCI_MEM_BASE_ADDRESS_3, &ioaddr);
      devc->membar_addr = ioaddr;

      /* get virtual address */
      devc->ac97_membar_virt =
	(char *) MAP_PCI_MEM (devc->osdev, 2, devc->ac97_membar_addr, 512);
      devc->membar_virt =
	(char *) MAP_PCI_MEM (devc->osdev, 3, devc->membar_addr, 256);
      devc->mem_mode = MMAP_MODE;
    }
  else
    devc->mem_mode = IO_MODE;

  if (devc->mem_mode == IO_MODE)
    {
      if (devc->model == INTEL_ICH4)
	{
	  /*
	   * enable the IOSE bit in 0x41 for legacy
	   * mode for ICH4/ICH5
	   */
	  pci_write_config_byte (osdev, 0x41, 1);

	  /* Set the secondary codec ID */
	  pci_write_config_byte (osdev, 0x40, 0x39);
	}
      /* Remove I/O space marker in bit 0. */
      devc->ac97_base = MAP_PCI_IOADDR (devc->osdev, 0, pci_ioaddr0);
      devc->base = MAP_PCI_IOADDR (devc->osdev, 1, pci_ioaddr1);
      devc->ac97_base &= ~0xF;
      devc->base &= ~0xF;
    }
  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_IO | PCI_COMMAND_MEMORY;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  devc->irq = pci_irq_line;

  if (devc->mem_mode != IO_MODE)
    {
      devc->base = devc->membar_addr;
    }

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);

  oss_register_device (osdev, devc->chip_name);

  if (oss_register_interrupts (devc->osdev, 0, ichintr, NULL) < 0)
    {
      cmn_err (CE_WARN, "Unable to install interrupt handler\n");
      return 0;
    }

  return ich_init (devc);	/* Detected */
}

int
oss_ich_detach (oss_device_t * osdev)
{
  ich_devc *devc = (ich_devc *) osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  /* disable interrupts */
  ich_OUTB (devc, 0x00, CTL_BASE, 0x0b);
  ich_OUTB (devc, 0x00, CTL_BASE, 0x1b);
  ich_OUTB (devc, 0x00, CTL_BASE, 0x2b);
  ich_OUTB (devc, 0x00, CTL_BASE, 0x7b);
  oss_unregister_interrupts (devc->osdev);

  /* disable S/PDIF */
  if (devc->mixer_dev)
    ac97_spdifout_ctl (devc->mixer_dev, SPDIFOUT_ENABLE, SNDCTL_MIX_WRITE, 0);

  if (devc->bdlBuffer)
    CONTIG_FREE (devc->osdev, devc->bdlBuffer, 4 * 32 * 32, devc->bldbuf_dma_handle);
  devc->bdlBuffer = NULL;

  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);

  if ((devc->mem_mode == MMAP_MODE) && (devc->membar_addr != 0))
    {
      UNMAP_PCI_MEM (devc->osdev, 2, devc->ac97_membar_addr,
		     devc->ac97_membar_virt, 512);
      UNMAP_PCI_MEM (devc->osdev, 3, devc->membar_addr, devc->membar_virt,
		     256);
      devc->membar_addr = 0;
      devc->ac97_membar_addr = 0;
    }
  else
    {
      UNMAP_PCI_IOADDR (devc->osdev, 0);
      UNMAP_PCI_IOADDR (devc->osdev, 1);
    }
  oss_unregister_device (osdev);
  return 1;
}

#ifdef OSS_POWER_MANAGE
/* Not activated in .config at this moment */
int
oss_ich_power (oss_device_t *osdev, int component, int level)
{
// cmn_err(CE_CONT, "oss_ich_power(%d, %d)\n", component, level);

	return 0; /* Failed */
}
#endif

#ifdef OSS_SUSPEND_RESUME
int
oss_ich_suspend(oss_device_t *osdev)
{
//cmn_err(CE_CONT, "oss_ich_suspend()\n");
	return 0; /* Failed */
}

int
oss_ich_resume(oss_device_t *osdev)
{
//cmn_err(CE_CONT, "oss_ich_resume()\n");
	return 0; /* Failed */
}
#endif
