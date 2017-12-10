/*
 * Purpose: Driver for Creative emu10k1x audio controller
 *
 * This device is usually called as SB Live! 5.1 and it has been used in 
 * some Dell machines. However it has nothing to do with the original
 * SB Live! design.
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

#include "oss_emu10k1x_cfg.h"
#include "oss_pci.h"
#include "ac97.h"
#include "midi_core.h"
#include "remux.h"

#define PCI_VENDOR_ID_CREATIVE 		0x1102
#define PCI_DEVICE_ID_CREATIVE_EMU10K1X 0x0006

#define USE_DUALBUF

/*
 * Indirect registers
 */

#define PTBA	0x000
#define PTBS	0x001
#define PTCA	0x002
#define PFBA	0x004
#define PFBS	0x005
#define CPFA	0x006
#define PFEA	0x007
#define CPCAV	0x008
#define RFBA	0x010
#define RFBS	0x011
#define CRFA	0x012
#define CRCAV	0x013
#define CDL	0x020
#define CDR	0x030
#define SA	0x040
#define EA_aux	0x041
#define SCS0	0x042
#define SCS1	0x043
#define SCS2	0x044
#define SPC	0x045
#define WMARK	0x046
#define MUDAT	0x047
#define MUCMD	0x048
#define RCD	0x050

/* Interrupt bits
 */

#define INTR_RFF	(1<<19)
#define INTR_RFH	(1<<16)
#define INTR_PFF	(1<<11)
#define INTR_PFH	(1<<8)
#define INTR_EAI	(1<<29)
#define INTR_PCI	1
#define INTR_UART_RX	2
#define INTR_UART_TX	4
#define INTR_AC97	0x10
#define INTR_GPIO	0x40

#define PLAY_INTR_ENABLE (INTR_PFF|INTR_PFH)
#define RECORD_INTR_ENABLE (INTR_RFF|INTR_RFH)

#define EMU_BUFSIZE 32*1024
#define MAX_PORTC	3

extern int emu10k1x_spdif_enable;

typedef struct
{
  int audio_dev;
  int port_number;
  int open_mode;
  int trigger_bits;
  int audio_enabled;
  int channels;
  int fmt;
  int speed;
  unsigned char *playbuf, *recbuf;
  oss_native_word playbuf_phys, recbuf_phys;

  oss_dma_handle_t playbuf_dma_handle, recbuf_dma_handle;

  int play_cfrag, play_chalf, rec_cfrag, rec_chalf;
}
emu10k1x_portc;

typedef struct
{
  oss_device_t *osdev;
  int loaded;
  oss_native_word base;
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;
  int irq;
  char *card_name;
  unsigned int subvendor;

  int mixer_dev;
  ac97_devc ac97devc;

/*
 * UART
 */
  oss_midi_inputbyte_t midi_input_intr;
  int midi_opened, midi_disabled;
  volatile unsigned char input_byte;
  int midi_dev;
  int mpu_attached;

  emu10k1x_portc portc[MAX_PORTC];

}
emu10k1x_devc;


static void emu10k1xuartintr (emu10k1x_devc * devc);

static unsigned int
read_reg (emu10k1x_devc * devc, int reg, int chn)
{
  oss_native_word flags;
  unsigned int val;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  OUTL (devc->osdev, (reg << 16) | (chn & 0xffff), devc->base + 0x00);	/* Pointer */
  val = INL (devc->osdev, devc->base + 0x04);	/* Data */
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);

/* printk("Read reg %03x (ch %d) = %08x\n", reg, chn, val); */
  return val;
}

static void
write_reg (emu10k1x_devc * devc, int reg, int chn, unsigned int value)
{
  oss_native_word flags;

  /* printk("Write reg %03x (ch %d) = %08x\n", reg, chn, value); */
  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  OUTL (devc->osdev, (reg << 16) | (chn & 0xffff), devc->base + 0x00);	/* Pointer */
  OUTL (devc->osdev, value, devc->base + 0x04);	/* Data */
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
#if 0
  {
    char tmp[100];
    int ok = 1;
    sprintf (tmp, "@w%d %04x/%s %x", chn, reg, emu_regname (reg, &ok), value);
    if (ok)
      oss_do_timing (tmp);
  }
#endif
}

static void
recording_intr (emu10k1x_devc * devc, emu10k1x_portc * portc, int status)
{
#ifdef USE_DUALBUF
  unsigned char *frombuf, *tobuf;
  dmap_p dmap = audio_engines[portc->audio_dev]->dmap_in;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  /* "Auto sync" the play half counters with the device */
  if (status & INTR_RFH)	/* 1st half completed */
    portc->rec_chalf = 0;	/* Reuse the first half */
  else
    portc->rec_chalf = 1;	/* Reuse the second half */

  tobuf = dmap->dmabuf + (portc->rec_cfrag * dmap->fragment_size);
  frombuf = portc->recbuf + (portc->rec_chalf * dmap->fragment_size);

  memcpy (tobuf, frombuf, dmap->fragment_size);

/* printk("rec %d/%d\n", portc->rec_cfrag, portc->rec_chalf); */
  portc->rec_cfrag = (portc->rec_cfrag + 1) % dmap->nfrags;
  portc->rec_chalf = !portc->rec_chalf;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
#endif
  oss_audio_inputintr (portc->audio_dev, 0);
}

static void
playback_intr (emu10k1x_devc * devc, emu10k1x_portc * portc,
	       unsigned int status)
{
#ifdef USE_DUALBUF
  dmap_p dmap = audio_engines[portc->audio_dev]->dmap_out;
  unsigned char *frombuf, *tobuf;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  /* "Auto sync" the play half counters with the device */
  if (status & (INTR_PFH << portc->port_number))	/* 1st half completed */
    {
      portc->play_chalf = 0;	/* Reuse the first half */
    }
  else
    {
      portc->play_chalf = 1;	/* Reuse the second half */
    }


  frombuf = dmap->dmabuf + (portc->play_cfrag * dmap->fragment_size);
  tobuf = portc->playbuf + (portc->play_chalf * dmap->fragment_size);

  memcpy (tobuf, frombuf, dmap->fragment_size);

/* printk("play %d/%d\n", portc->play_cfrag, portc->play_chalf); */
  portc->play_cfrag = (portc->play_cfrag + 1) % dmap->nfrags;
  portc->play_chalf = !portc->play_chalf;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
#endif
  oss_audio_outputintr (portc->audio_dev, 0);
}

static int
emu10k1xintr (oss_device_t * osdev)
{
  int serviced = 0;
  unsigned int status;
  emu10k1x_devc *devc = (emu10k1x_devc *) osdev->devc;
  int portnum;

  status = INL (devc->osdev, devc->base + 0x08);

  if (status & 0x2)		/* MIDI RX interrupt */
    {
      emu10k1xuartintr (devc);
      serviced = 1;
    }

  if (status & (INTR_PFF | INTR_PFH | INTR_RFF | INTR_RFH))
    {
      for (portnum = 0; portnum < 3; portnum++)
	{
	  emu10k1x_portc *portc = &devc->portc[portnum];

	  if ((portc->trigger_bits & PCM_ENABLE_OUTPUT) &&
	      (status & ((INTR_PFF | INTR_PFH) << portc->port_number)))
	    playback_intr (devc, portc, status);
	  if ((portc->trigger_bits & PCM_ENABLE_INPUT) &&
	      (status & (INTR_RFF | INTR_RFH)))
	    recording_intr (devc, portc, status);

	}
      serviced = 1;
      OUTL (devc->osdev, status, devc->base + 0x08);	/* Acknowledge */
    }
  return serviced;
}

/*ARGSUSED*/
static int
emu10k1x_set_rate (int dev, int arg)
{
  emu10k1x_portc *portc = audio_engines[dev]->portc;

  return portc->speed = 48000;
}

static short
emu10k1x_set_channels (int dev, short arg)
{
  emu10k1x_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->channels;

  if (portc->open_mode & OPEN_READ)
    return portc->channels = 2;

  if (arg != 1 && arg != 2)
    return portc->channels = 2;
  return portc->channels = arg;
}

static unsigned int
emu10k1x_set_format (int dev, unsigned int arg)
{
  emu10k1x_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->fmt;

  if (arg == AFMT_AC3)
    if ((portc->open_mode & OPEN_READ) || !emu10k1x_spdif_enable)
      arg = AFMT_S16_LE;

  if (arg != AFMT_AC3 && arg != AFMT_S16_LE)
    return portc->fmt = AFMT_S16_LE;

  return portc->fmt = arg;
}

/*ARGSUSED*/
static int
emu10k1x_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void emu10k1x_trigger (int dev, int state);

static void
emu10k1x_reset (int dev)
{
  emu10k1x_trigger (dev, 0);
}

static void
emu10k1x_reset_input (int dev)
{
  emu10k1x_portc *portc = audio_engines[dev]->portc;
  emu10k1x_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_INPUT);
}

static void
emu10k1x_reset_output (int dev)
{
  emu10k1x_portc *portc = audio_engines[dev]->portc;
  emu10k1x_trigger (dev, portc->trigger_bits & ~PCM_ENABLE_OUTPUT);
}

/*ARGSUSED*/
static int
emu10k1x_open (int dev, int mode, int open_flags)
{
  emu10k1x_portc *portc = audio_engines[dev]->portc;
  emu10k1x_devc *devc = audio_engines[dev]->devc;
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
emu10k1x_close (int dev, int mode)
{
  emu10k1x_portc *portc = audio_engines[dev]->portc;

  emu10k1x_trigger (dev, 0);
  portc->open_mode = 0;
  portc->audio_enabled &= ~mode;
}

/*ARGSUSED*/
static void
emu10k1x_output_block (int dev, oss_native_word buf, int count, int fragsize,
		       int intrflag)
{
}

/*ARGSUSED*/
static void
emu10k1x_start_input (int dev, oss_native_word buf, int count, int fragsize,
		      int intrflag)
{
  emu10k1x_portc *portc = audio_engines[dev]->portc;

  portc->audio_enabled |= PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
}


static void
emu10k1x_trigger (int dev, int state)
{
  emu10k1x_devc *devc = audio_engines[dev]->devc;
  emu10k1x_portc *portc = audio_engines[dev]->portc;
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
	      /* Enable play channel and set mono/stereo mode */
	      tmp = read_reg (devc, SA, 0);
	      tmp &= ~(0x10000 << portc->port_number);
	      if (portc->channels == 1)
		tmp |= (0x10000 << portc->port_number);
	      tmp |= 1 << portc->port_number;
	      write_reg (devc, SA, 0, tmp);

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

	      /* Disable Play channel */
	      tmp = read_reg (devc, SA, 0);
	      tmp &= ~(1 << portc->port_number);
	      write_reg (devc, SA, 0, tmp);
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
	      /* Enable Rec Channel */
	      tmp = read_reg (devc, SA, 0);
	      tmp |= 0x100;
	      write_reg (devc, SA, 0, tmp);
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

	      /* disable channel */
	      tmp = read_reg (devc, SA, 0);
	      tmp &= ~0x100;
	      write_reg (devc, SA, 0, tmp);
	    }
	}
    }

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
static int
emu10k1x_prepare_for_input (int dev, int bsize, int bcount)
{
  emu10k1x_devc *devc = audio_engines[dev]->devc;
  emu10k1x_portc *portc = audio_engines[dev]->portc;
  dmap_p dmap = audio_engines[dev]->dmap_in;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
#ifndef USE_DUALBUF
  /* Single buffering mode */
  dmap->nfrags = 2;
  dmap->bytes_in_use = dmap->nfrags * dmap->fragment_size;
  write_reg (devc, RFBA, 0, dmap->dmabuf_phys);
  write_reg (devc, RFBS, 0, (dmap->bytes_in_use - 4) << 16);
#else
  write_reg (devc, RFBA, 0, portc->recbuf_phys);
  write_reg (devc, RFBS, 0, (dmap->fragment_size * 2) << 16);
#endif
  memset (portc->recbuf, 0, EMU_BUFSIZE);
  portc->rec_cfrag = portc->rec_chalf = 0;
  portc->audio_enabled &= ~PCM_ENABLE_INPUT;
  portc->trigger_bits &= ~PCM_ENABLE_INPUT;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

/*ARGSUSED*/
static int
emu10k1x_prepare_for_output (int dev, int bsize, int bcount)
{
  emu10k1x_devc *devc = audio_engines[dev]->devc;
  emu10k1x_portc *portc = audio_engines[dev]->portc;
  dmap_p dmap = audio_engines[dev]->dmap_out;
  unsigned int tmp;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->fmt == AFMT_AC3)
    portc->channels = 2;

  write_reg (devc, PTBA, portc->port_number, 0);
  write_reg (devc, PTBS, portc->port_number, 0);
  write_reg (devc, PTCA, portc->port_number, 0);

  write_reg (devc, CPFA, portc->port_number, 0);
  write_reg (devc, PFEA, portc->port_number, 0);
  write_reg (devc, CPCAV, portc->port_number, 0);

#ifndef USE_DUALBUF
  /* Single buffering mode */
  dmap->nfrags = 2;
  dmap->bytes_in_use = dmap->nfrags * dmap->fragment_size;
  write_reg (devc, PFBA, portc->port_number, dmap->dmabuf_phys);
  write_reg (devc, PFBS, portc->port_number, (dmap->bytes_in_use - 4) << 16);
#else
  /* Dual buffering mode */
  write_reg (devc, PFBA, portc->port_number, portc->playbuf_phys);
  write_reg (devc, PFBS, portc->port_number, (dmap->fragment_size * 2) << 16);
#endif
  memset (portc->playbuf, 0, EMU_BUFSIZE);
  portc->play_cfrag = portc->play_chalf = 0;

  if (portc->fmt == AFMT_AC3)
    {
      tmp = read_reg (devc, EA_aux, 0);
      tmp &= ~(0x03 << (portc->port_number * 2));
      if (portc->port_number == 2)
	tmp &= ~0x10000;
      write_reg (devc, EA_aux, 0, tmp);
      write_reg (devc, SCS0 + portc->port_number, 0, 0x02108506);	/* Data */
    }
  else
    {
      tmp = read_reg (devc, EA_aux, 0);
      tmp |= (0x03 << (portc->port_number * 2));

      if (emu10k1x_spdif_enable == 0)
	if (portc->port_number == 2)
	  tmp |= 0x10000;

      write_reg (devc, EA_aux, 0, tmp);
      write_reg (devc, SCS0 + portc->port_number, 0, 0x02108504);	/* Audio */
    }

  portc->audio_enabled |= PCM_ENABLE_OUTPUT;
  portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

static int
emu10k1x_alloc_buffer (int dev, dmap_t * dmap, int direction)
{
  int err;
  emu10k1x_devc *devc = audio_engines[dev]->devc;
  emu10k1x_portc *portc = audio_engines[dev]->portc;
  oss_native_word phaddr;

  if ((err = oss_alloc_dmabuf (dev, dmap, direction)) < 0)
    return err;

  if (direction == OPEN_READ)
    {
      if (portc->port_number == 0)
	{
	  portc->recbuf =
	    CONTIG_MALLOC (devc->osdev, EMU_BUFSIZE, MEMLIMIT_32BITS, &phaddr, portc->recbuf_dma_handle);
	  if (portc->recbuf == NULL)
	    return OSS_ENOMEM;
	  portc->recbuf_phys = phaddr;
	}
    }
  else
    {
      portc->playbuf =
	CONTIG_MALLOC (devc->osdev, EMU_BUFSIZE, MEMLIMIT_32BITS, &phaddr, portc->playbuf_dma_handle);
      if (portc->playbuf == NULL)
	return OSS_ENOMEM;
      portc->playbuf_phys = phaddr;
    }
  return 0;
}

/*ARGSUSED*/
static int
emu10k1x_free_buffer (int dev, dmap_t * dmap, int direction)
{
  emu10k1x_devc *devc = audio_engines[dev]->devc;
  emu10k1x_portc *portc = audio_engines[dev]->portc;

  oss_free_dmabuf (dev, dmap);

  if (portc->playbuf != NULL)
    {
      CONTIG_FREE (devc->osdev, portc->playbuf, EMU_BUFSIZE, portc->playbuf_dma_handle);
      portc->playbuf = NULL;
    }

  if (portc->recbuf != NULL)
    {
      CONTIG_FREE (devc->osdev, portc->recbuf, EMU_BUFSIZE, portc->recbuf_dma_handle);
      portc->recbuf = NULL;
    }

  return 0;
}

#if 0
static int
emu10k1x_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  unsigned int p = 0;

  emu10k1x_devc *devc = audio_engines[dev]->devc;
  emu10k1x_portc *portc = audio_engines[dev]->portc;

  dmap = audio_engines[dev]->dmap_out;
  if (direction == PCM_ENABLE_OUTPUT)
    p = read_reg (devc, CPFA, portc->port_number);

  if (direction == PCM_ENABLE_INPUT)
    p = read_reg (devc, CRFA, portc->port_number);

  p %= (dmap->bytes_in_use - 4);

  return p;
}
#endif

static audiodrv_t emu10k1x_audio_driver = {
  emu10k1x_open,
  emu10k1x_close,
  emu10k1x_output_block,
  emu10k1x_start_input,
  emu10k1x_ioctl,
  emu10k1x_prepare_for_input,
  emu10k1x_prepare_for_output,
  emu10k1x_reset,
  NULL,
  NULL,
  emu10k1x_reset_input,
  emu10k1x_reset_output,
  emu10k1x_trigger,
  emu10k1x_set_rate,
  emu10k1x_set_format,
  emu10k1x_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  emu10k1x_alloc_buffer,
  emu10k1x_free_buffer,
  NULL,
  NULL,
  NULL				/*emu10k1x_get_buffer_pointer */
};


#define MUADAT   0x47
#define MUACMD   0x48
#define MUASTAT   0x48

static __inline__ int
emu10k1xuart_status (emu10k1x_devc * devc)
{
  return read_reg (devc, MUASTAT, 0);
}

#define input_avail(devc) (!(emu10k1xuart_status(devc)&INPUT_AVAIL))
#define output_ready(devc)      (!(emu10k1xuart_status(devc)&OUTPUT_READY))
static void
emu10k1xuart_cmd (emu10k1x_devc * devc, unsigned char cmd)
{
  write_reg (devc, MUACMD, 0, cmd);
}

static __inline__ int
emu10k1xuart_read (emu10k1x_devc * devc)
{
  return read_reg (devc, MUADAT, 0);
}

static __inline__ void
emu10k1xuart_write (emu10k1x_devc * devc, unsigned char byte)
{
  write_reg (devc, MUADAT, 0, byte);
}

#define OUTPUT_READY    0x40
#define INPUT_AVAIL     0x80
#define MPU_ACK         0xFE
#define MPU_RESET       0xFF
#define UART_MODE_ON    0x3F

static int reset_emu10k1xuart (emu10k1x_devc * devc);
static void enter_uart_mode (emu10k1x_devc * devc);

static void
emu10k1xuart_input_loop (emu10k1x_devc * devc)
{
  while (input_avail (devc))
    {
      unsigned char c = emu10k1xuart_read (devc);

      if (c == MPU_ACK)
	devc->input_byte = c;
      else if (devc->midi_opened & OPEN_READ && devc->midi_input_intr)
	devc->midi_input_intr (devc->midi_dev, c);
    }
}

static void
emu10k1xuartintr (emu10k1x_devc * devc)
{
  emu10k1xuart_input_loop (devc);
}

/*ARGSUSED*/
static int
emu10k1xuart_open (int dev, int mode, oss_midi_inputbyte_t inputbyte,
		   oss_midi_inputbuf_t inputbuf,
		   oss_midi_outputintr_t outputintr)
{
  emu10k1x_devc *devc = (emu10k1x_devc *) midi_devs[dev]->devc;

  if (devc->midi_opened)
    {
      return OSS_EBUSY;
    }

  while (input_avail (devc))
    emu10k1xuart_read (devc);

  devc->midi_input_intr = inputbyte;
  devc->midi_opened = mode;
  enter_uart_mode (devc);
  devc->midi_disabled = 0;

  return 0;
}

/*ARGSUSED*/
static void
emu10k1xuart_close (int dev, int mode)
{
  emu10k1x_devc *devc = (emu10k1x_devc *) midi_devs[dev]->devc;

  reset_emu10k1xuart (devc);
  oss_udelay (10);
  enter_uart_mode (devc);
  reset_emu10k1xuart (devc);
  devc->midi_opened = 0;
}


static int
emu10k1xuart_out (int dev, unsigned char midi_byte)
{
  int timeout;
  emu10k1x_devc *devc = (emu10k1x_devc *) midi_devs[dev]->devc;
  oss_native_word flags;

  /*
   * Test for input since pending input seems to block the output.
   */

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (input_avail (devc))
    emu10k1xuart_input_loop (devc);

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  /*
   * Sometimes it takes about 130000 loops before the output becomes ready
   * (After reset). Normally it takes just about 10 loops.
   */

  for (timeout = 130000; timeout > 0 && !output_ready (devc); timeout--);

  if (!output_ready (devc))
    {
      cmn_err (CE_WARN, "UART timeout - Device not responding\n");
      devc->midi_disabled = 1;
      reset_emu10k1xuart (devc);
      enter_uart_mode (devc);
      return 1;
    }

  emu10k1xuart_write (devc, midi_byte);
  return 1;
}

/*ARGSUSED*/
static int
emu10k1xuart_ioctl (int dev, unsigned cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static midi_driver_t emu10k1x_midi_driver = {
  emu10k1xuart_open,
  emu10k1xuart_close,
  emu10k1xuart_ioctl,
  emu10k1xuart_out
};


static void
enter_uart_mode (emu10k1x_devc * devc)
{
  int ok, timeout;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  for (timeout = 30000; timeout > 0 && !output_ready (devc); timeout--);

  devc->input_byte = 0;
  emu10k1xuart_cmd (devc, UART_MODE_ON);

  ok = 0;
  for (timeout = 50000; timeout > 0 && !ok; timeout--)
    if (devc->input_byte == MPU_ACK)
      ok = 1;
    else if (input_avail (devc))
      if (emu10k1xuart_read (devc) == MPU_ACK)
	ok = 1;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}


void
attach_emu10k1xuart (emu10k1x_devc * devc)
{
  enter_uart_mode (devc);

  devc->midi_dev =
    oss_install_mididev (OSS_MIDI_DRIVER_VERSION, "EMU10K1X", "SB P16X UART",
			 &emu10k1x_midi_driver, sizeof (midi_driver_t),
			 0, devc, devc->osdev);
  devc->midi_opened = 0;
}

static int
reset_emu10k1xuart (emu10k1x_devc * devc)
{
  int ok, timeout, n;

  /*
   * Send the RESET command. Try again if no success at the first time.
   */

  ok = 0;

  for (n = 0; n < 2 && !ok; n++)
    {
      for (timeout = 30000; timeout > 0 && !output_ready (devc); timeout--);

      devc->input_byte = 0;
      emu10k1xuart_cmd (devc, MPU_RESET);

      /*
       * Wait at least 25 msec. This method is not accurate so let's make the
       * loop bit longer. Cannot sleep since this is called during boot.
       */

      for (timeout = 50000; timeout > 0 && !ok; timeout--)
	if (devc->input_byte == MPU_ACK)	/* Interrupt */
	  ok = 1;
	else if (input_avail (devc))
	  if (emu10k1xuart_read (devc) == MPU_ACK)
	    ok = 1;

    }



  if (ok)
    emu10k1xuart_input_loop (devc);	/*
					 * Flush input before enabling interrupts
					 */

  return ok;
}


int
probe_emu10k1xuart (emu10k1x_devc * devc)
{
  int ok = 0;
  oss_native_word flags;

  DDB (cmn_err (CE_CONT, "Entered probe_emu10k1xuart\n"));

  devc->midi_input_intr = NULL;
  devc->midi_opened = 0;
  devc->input_byte = 0;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  ok = reset_emu10k1xuart (devc);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  if (ok)
    {
      DDB (cmn_err (CE_CONT, "Reset UART401 OK\n"));
    }
  else
    {
      DDB (cmn_err
	   (CE_WARN, "Reset UART401 failed (no hardware present?).\n"));
      DDB (cmn_err
	   (CE_WARN, "mpu401 status %02x\n", emu10k1xuart_status (devc)));
    }

  DDB (cmn_err (CE_WARN, "emu10k1xuart detected OK\n"));
  return ok;
}

void
unload_emu10k1xuart (emu10k1x_devc * devc)
{
  reset_emu10k1xuart (devc);
}


static void
attach_mpu (emu10k1x_devc * devc)
{
  devc->mpu_attached = 1;
  attach_emu10k1xuart (devc);
}


static int
emu10k1x_ac97_read (void *devc_, int wAddr)
{
  emu10k1x_devc *devc = devc_;
  int dtemp = 0, i;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  OUTB (devc->osdev, wAddr, devc->base + 0x1e);
  for (i = 0; i < 10000; i++)
    if (INB (devc->osdev, devc->base + 0x1e) & 0x80)
      break;
  dtemp = INW (devc->osdev, devc->base + 0x1c);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);

  return dtemp & 0xffff;
}

static int
emu10k1x_ac97_write (void *devc_, int wAddr, int wData)
{
  emu10k1x_devc *devc = devc_;
  int i;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  OUTB (devc->osdev, wAddr, devc->base + 0x1e);
  for (i = 0; i < 10000; i++)
    if (INB (devc->osdev, devc->base + 0x1e) & 0x80)
      break;
  OUTW (devc->osdev, wData, devc->base + 0x1c);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);

  return 0;
}

static const int bindings[MAX_PORTC] = {
  DSP_BIND_FRONT,
  DSP_BIND_SURR,
  DSP_BIND_CENTER_LFE
};

static void
install_audio_devices (emu10k1x_devc * devc)
{
  int i;
  unsigned int tmp;
  int firstdev = -1;
  char name[64];

#if 0
  if (emu10k1x_spdif_enable == 1)
    n = 2;
#endif

  /* Enable play interrupts for all 3 channels */
  for (i = 0; i < MAX_PORTC; i++)
    {
      tmp = INL (devc->osdev, devc->base + 0x0c);
      tmp |= PLAY_INTR_ENABLE << i;
      OUTL (devc->osdev, tmp, devc->base + 0x0c);
    }

  /* Enable record interrupts */
  tmp = INL (devc->osdev, devc->base + 0x0c);
  tmp |= RECORD_INTR_ENABLE;
  OUTL (devc->osdev, tmp, devc->base + 0x0c);

  for (i = 0; i < MAX_PORTC; i++)
    {
      int adev, flags;
      emu10k1x_portc *portc = &devc->portc[i];

      flags = ADEV_AUTOMODE | ADEV_FIXEDRATE | ADEV_16BITONLY | ADEV_COLD;

      switch (i)
	{
	case 0:
	  sprintf (name, "%s (front)", devc->card_name);
	  break;
	case 1:
	  sprintf (name, "%s (surround)", devc->card_name);
	  break;
	case 2:
	  if (emu10k1x_spdif_enable == 1)
	    sprintf (name, "%s (SPDIF)", devc->card_name);
	  else
	    sprintf (name, "%s (center/LFE)", devc->card_name);
	  break;
	}

      if (i == 0)
	flags |= ADEV_DUPLEX;
      else
	flags |= ADEV_NOINPUT;

      if ((adev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
					devc->osdev,
					devc->osdev,
					name,
					&emu10k1x_audio_driver,
					sizeof (audiodrv_t),
					flags, AFMT_S16_LE | AFMT_AC3, devc,
					-1)) < 0)
	{
	  return;
	}

      if (i == 0)
	firstdev = adev;
      audio_engines[adev]->portc = portc;
      audio_engines[adev]->mixer_dev = devc->mixer_dev;
      audio_engines[adev]->rate_source = firstdev;
      audio_engines[adev]->min_rate = 48000;
      audio_engines[adev]->max_rate = 48000;
      audio_engines[adev]->caps |= PCM_CAP_FREERATE;
      /*audio_engines[adev]->max_block = EMU_BUFSIZE / 2; *//*  Never change this */
      audio_engines[adev]->fixed_rate = 48000;
      audio_engines[adev]->binding = bindings[i];
      audio_engines[adev]->vmix_flags = VMIX_MULTIFRAG;
      portc->audio_dev = adev;
      portc->open_mode = 0;
      portc->port_number = i;
      portc->channels = 2;
      portc->fmt = AFMT_S16_LE;
#ifdef CONFIG_OSS_VMIX
      if (i == 0)
         vmix_attach_audiodev(devc->osdev, adev, -1, 0);
#endif
    }

#ifdef USE_REMUX
  if (firstdev >= 0)
    {
      if (emu10k1x_spdif_enable == 1)
	{
	  sprintf (name, "%s 4.0 output", devc->card_name);
	  remux_install (name, devc->osdev, firstdev, firstdev + 1, -1, -1);
	}
      else
	{
	  sprintf (name, "%s 5.1 output", devc->card_name);
	  remux_install (name, devc->osdev, firstdev, firstdev + 1,
			 firstdev + 2, -1);
	}
    }
#endif
}

static void
select_out3_mode (emu10k1x_devc * devc, int mode)
{
  /*
   * Set the out3/spdif combo jack format.
   * mode0=analog rear/center, 1=spdif
   */

  if (mode == 0)
    {
      write_reg (devc, SPC, 0, 0x00000700);
      write_reg (devc, EA_aux, 0, 0x0001000f);
    }
  else
    {
      write_reg (devc, SPC, 0, 0x00000000);
      write_reg (devc, EA_aux, 0, 0x0000070f);
    }
}

int
oss_emu10k1x_attach (oss_device_t * osdev)
{
  unsigned char pci_irq_line, pci_revision;
  unsigned short pci_command, vendor, device;
  unsigned int pci_ioaddr;
  unsigned int subvendor;
  int err;
  emu10k1x_devc *devc = NULL;

  DDB (cmn_err (CE_WARN, "Entered EMU10K1X probe routine\n"));

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  if (vendor != PCI_VENDOR_ID_CREATIVE ||
      device != PCI_DEVICE_ID_CREATIVE_EMU10K1X)

    return 0;

  pci_read_config_dword (osdev, 0x2c, &subvendor);
  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, PCI_BASE_ADDRESS_0, &pci_ioaddr);

  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_IO;
  pci_command &= ~(PCI_COMMAND_SERR | PCI_COMMAND_PARITY);
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  if (pci_ioaddr == 0)
    {
      cmn_err (CE_WARN, "I/O address not assigned by BIOS.\n");
      return 0;
    }

  if (pci_irq_line == 0)
    {
      cmn_err (CE_WARN, "IRQ not assigned by BIOS.\n");
      return 0;
    }

  if ((devc = PMALLOC (osdev, sizeof (*devc))) == NULL)
    {
      cmn_err (CE_WARN, "Out of memory\n");
      return 0;
    }

  devc->osdev = osdev;
  osdev->devc = devc;
  devc->card_name = "Sound Blaster Live (P16X)";
  devc->subvendor = subvendor;

  devc->base = MAP_PCI_IOADDR (devc->osdev, 0, pci_ioaddr);
  devc->base &= ~0x3;

  devc->irq = pci_irq_line;

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  MUTEX_INIT (devc->osdev, devc->low_mutex, MH_DRV + 1);

  oss_register_device (osdev, devc->card_name);

  if ((err =
       oss_register_interrupts (devc->osdev, 0, emu10k1xintr, NULL)) < 0)
    {
      cmn_err (CE_WARN, "Can't register interrupt handler, err=%d\n", err);
      return 0;
    }

/*
 * Init mixer
 */
  devc->mixer_dev = ac97_install (&devc->ac97devc, devc->card_name,
				  emu10k1x_ac97_read, emu10k1x_ac97_write,
				  devc, devc->osdev);
  if (devc->mixer_dev < 0)
    {
      cmn_err (CE_WARN, "Mixer install failed - cannot continue\n");
      return 0;
    }

  write_reg (devc, SCS0, 0, 0x02108504);
  write_reg (devc, SCS1, 0, 0x02108504);
  write_reg (devc, SCS2, 0, 0x02108504);
  select_out3_mode (devc, emu10k1x_spdif_enable);
  OUTL (devc->osdev, 0x00000000, devc->base + 0x18);	/* GPIO */
  OUTL (devc->osdev, INTR_PCI | INTR_UART_RX, devc->base + 0x0c);
  OUTL (devc->osdev, 0x00000009, devc->base + 0x14);	/* Enable audio */
  install_audio_devices (devc);
  attach_mpu (devc);
  return 1;
}

static void
unload_mpu (emu10k1x_devc * devc)
{
  if (devc->mpu_attached)
    {
      unload_emu10k1xuart (devc);
      devc->mpu_attached = 0;
    }
}

int
oss_emu10k1x_detach (oss_device_t * osdev)
{
  emu10k1x_devc *devc = (emu10k1x_devc *) osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  write_reg (devc, SA, 0, 0);
  OUTL (devc->osdev, 0x00000000, devc->base + 0x0c);	/* Interrupt disable */
  OUTL (devc->osdev, 0x00000001, devc->base + 0x14);

  unload_mpu (devc);

  oss_unregister_interrupts (devc->osdev);

  MUTEX_CLEANUP (devc->mutex);
  MUTEX_CLEANUP (devc->low_mutex);
  UNMAP_PCI_IOADDR (devc->osdev, 0);

  oss_unregister_device (osdev);
  return 1;
}
