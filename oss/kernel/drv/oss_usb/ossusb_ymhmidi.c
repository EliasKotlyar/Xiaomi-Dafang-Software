/*
 * Purpose: Dedicated driver for Yamaha USB MIDI devices
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

#include "oss_config.h"
#include "midi_core.h"
#include "ossusb.h"
#include "midiparser.h"

typedef struct ymhusb_devc ymhusb_devc;

#define MAX_INDEVS	1
#define MAX_OUTDEVS	1
#define TMPBUF_SIZE 	64
#define TMPBUF_NSLOTS	24

typedef unsigned char tmpbuf_slot_t[TMPBUF_SIZE];
#define OUTBUF_SIZE (TMPBUF_NSLOTS*TMPBUF_SIZE)

typedef struct
{
  ymhusb_devc *devc;
  oss_device_t *osdev;
  udi_usb_devc *usbdev;
  oss_mutex_t mutex;

  int portnum;
  int midi_dev;
  int open_mode;

  void *endpoint_desc;
  udi_endpoint_handle_t *endpoint_handle;
  udi_usb_request_t *datapipe;

  oss_midi_inputbyte_t midi_input_intr;
  unsigned char *tmpbuf;
  oss_dma_handle_t tmpbuf_dma_handle;
  int tmp_len;
  midiparser_common_p parser;

  volatile int output_busy;

  /* Output buffer */
  tmpbuf_slot_t *outbuf;
  oss_dma_handle_t outbuf_dma_handle;
  int buf_t, buf_h;
} ymhusb_midic;

struct ymhusb_devc
{
  special_unload_t unload_func;

  oss_device_t *osdev;
  udi_usb_devc *usbdev;

  oss_mutex_t mutex;

  int n_inputs, n_outputs;
  ymhusb_midic indevs[MAX_INDEVS], outdevs[MAX_OUTDEVS];
};

static void
ymhusb_unload (void *d)
{
  ymhusb_devc *devc = (ymhusb_devc *) d;
  int i;

  for (i = 0; i < devc->n_inputs; i++)
    {
      MUTEX_CLEANUP (devc->indevs[i].mutex);
    }
  for (i = 0; i < devc->n_outputs; i++)
    {
      MUTEX_CLEANUP (devc->outdevs[i].mutex);
    }
  MUTEX_CLEANUP (devc->mutex);
}

static int ymhmidi_start_input (ymhusb_devc * devc, ymhusb_midic * midic);

void
ymhmidi_record_callback (udi_usb_request_t * request, void *arg)
{
  ymhusb_midic *midic = arg;
  ymhusb_devc *devc = midic->devc;
  unsigned char *data;
  int l;

  l = udi_usb_request_actlen (request);
  data = udi_usb_request_actdata (request);

  if (l == 0)
    goto restart;

  if (midi_devs[midic->midi_dev]->event_input != NULL)
    midi_devs[midic->midi_dev]->event_input (midic->midi_dev, data, l);

restart:
  if (midic->open_mode & OPEN_READ)
    ymhmidi_start_input (devc, midic);
}

static int ymhmidi_submit_output (ymhusb_devc * devc, ymhusb_midic * midic);

 /*ARGSUSED*/ void
ymhmidi_play_callback (udi_usb_request_t * request, void *arg)
{
  ymhusb_midic *midic = arg;
  ymhusb_devc *devc = midic->devc;

  midic->output_busy = 0;
  ymhmidi_submit_output (devc, midic);
}

 /*ARGSUSED*/ static int
ymhmidi_start_input (ymhusb_devc * devc, ymhusb_midic * midic)
{
  int err;

  if ((err =
       udi_usb_submit_request (midic->datapipe, ymhmidi_record_callback,
			       midic, midic->endpoint_handle,
			       UDI_USBXFER_BULK_READ, midic->tmpbuf,
			       TMPBUF_SIZE)) < 0)
    {
      return err;
    }

  return 0;
}

 /*ARGSUSED*/ static int
ymhmidi_submit_output (ymhusb_devc * devc, ymhusb_midic * midic)
{
  int err;
  int n = midic->buf_t;

  if (midic->buf_h == midic->buf_t)
    return 0;

  if ((err =
       udi_usb_submit_request (midic->datapipe, ymhmidi_play_callback, midic,
			       midic->endpoint_handle, UDI_USBXFER_BULK_WRITE,
			       midic->outbuf[n], TMPBUF_SIZE)) < 0)
    {
      cmn_err (CE_WARN, "Submit USB MIDI request failed\n");
      return err;
    }

  midic->buf_t = (n + 1) % TMPBUF_NSLOTS;;
  midic->output_busy = 1;

  return 0;
}

static int
ymhmidi_put_output (ymhusb_devc * devc, ymhusb_midic * midic)
{
  int n = midic->buf_h;

  midic->buf_h = (n + 1) % TMPBUF_NSLOTS;

  memcpy (midic->outbuf[n], midic->tmpbuf, TMPBUF_SIZE);
  midic->tmp_len = 0;
  memset (midic->tmpbuf, 0, TMPBUF_SIZE);

  if (midic->output_busy)
    return 0;

  ymhmidi_submit_output (devc, midic);

  return 0;
}

 /*ARGSUSED*/ static void
ymhusb_close_input (int dev, int mode)
{
  oss_native_word flags;
  ymhusb_midic *midic;

  midic = midi_devs[dev]->devc;

  MUTEX_ENTER_IRQDISABLE (midic->mutex, flags);
  midic->open_mode = 0;
  midic->midi_input_intr = NULL;
  udi_usb_free_request (midic->datapipe);
  udi_close_endpoint (midic->endpoint_handle);
  if (midic->tmpbuf != NULL)
    CONTIG_FREE (midic->osdev, midic->tmpbuf, TMPBUF_SIZE, midic->tmpbuf_dma_handle);
  MUTEX_EXIT_IRQRESTORE (midic->mutex, flags);
}

 /*ARGSUSED*/ static int
ymhusb_open_input (int dev, int mode, oss_midi_inputbyte_t inputbyte,
		   oss_midi_inputbuf_t inputbuf,
		   oss_midi_outputintr_t outputintr)
{
  oss_native_word flags, phaddr;
  ymhusb_midic *midic;
  ymhusb_devc *devc;

  midic = midi_devs[dev]->devc;
  devc = midic->devc;

  MUTEX_ENTER_IRQDISABLE (midic->mutex, flags);
  if (midic->open_mode)
    {
      MUTEX_EXIT_IRQRESTORE (midic->mutex, flags);
      return OSS_EBUSY;
    }

  midic->open_mode = mode;
  midic->midi_input_intr = inputbyte;
  MUTEX_EXIT_IRQRESTORE (midic->mutex, flags);

  midic->tmpbuf =
    CONTIG_MALLOC (midic->osdev, TMPBUF_SIZE, MEMLIMIT_32BITS, &phaddr, midic->tmpbuf_dma_handle);
  memset (midic->tmpbuf, 0, TMPBUF_SIZE);

  if ((midic->endpoint_handle =
       udi_open_endpoint (midic->usbdev, midic->endpoint_desc)) == NULL)
    {
      midic->open_mode = 0;
      midic->midi_input_intr = NULL;
      cmn_err (CE_WARN, "Cannot open audio pipe\n");
      return OSS_ENOMEM;
    }
  if ((midic->datapipe =
       udi_usb_alloc_request (midic->usbdev, midic->endpoint_handle, 1,
			      UDI_USBXFER_BULK_READ)) == NULL)
    {
      return OSS_EIO;
    }

  return ymhmidi_start_input (devc, midic);
}

static void
ymhusb_flush_output (int dev)
{
  oss_native_word flags;
  ymhusb_midic *midic;
  ymhusb_devc *devc;
  int next;

  midic = midi_devs[dev]->devc;
  devc = midic->devc;

  if (midic->tmp_len == 0)
    return;
  next = (midic->buf_t + 1) % TMPBUF_NSLOTS;
  if (next == midic->buf_h)	/* Buffer full */
    return;

  MUTEX_ENTER_IRQDISABLE (midic->mutex, flags);
  ymhmidi_put_output (devc, midic);
  MUTEX_EXIT_IRQRESTORE (midic->mutex, flags);
}

static int
ymhusb_wait_output (int dev)
{
  ymhusb_midic *midic;

  ymhusb_flush_output (dev);

  midic = midi_devs[dev]->devc;

  if (midic->output_busy)
    return 1;

  if (midic->buf_t == midic->buf_h)
    return 0;			/* Not busy */

  return 1;
}

 /*ARGSUSED*/ static void
ymhusb_close_output (int dev, int mode)
{
  oss_native_word flags;
  ymhusb_midic *midic;

  midic = midi_devs[dev]->devc;

  ymhusb_flush_output (dev);

  MUTEX_ENTER_IRQDISABLE (midic->mutex, flags);
  midic->open_mode = 0;
  midic->midi_input_intr = NULL;
  udi_usb_free_request (midic->datapipe);
  udi_close_endpoint (midic->endpoint_handle);
  if (midic->outbuf != NULL)
    CONTIG_FREE (midic->osdev, midic->outbuf, OUTBUF_SIZE, midic->outbuf_dma_handle);
  if (midic->tmpbuf != NULL)
    KERNEL_FREE (midic->tmpbuf);
  midiparser_unalloc (midic->parser);
  midic->parser = NULL;
  MUTEX_EXIT_IRQRESTORE (midic->mutex, flags);
}

static void
out_event (ymhusb_midic * midic, unsigned char a, unsigned char b,
	   unsigned char c, unsigned char d)
{
  unsigned char *e;
  ymhusb_devc *devc = midic->devc;

  if (midic->tmp_len > TMPBUF_SIZE - 4)
    ymhmidi_put_output (devc, midic);

  e = midic->tmpbuf + midic->tmp_len;
  *e++ = a;
  *e++ = b;
  *e++ = c;
  *e++ = d;

  midic->tmp_len += 4;

  if (midic->tmp_len >= TMPBUF_SIZE)
    ymhmidi_put_output (devc, midic);
}

void
parser_cb (void *context, int category, unsigned char msg, unsigned char ch,
	   unsigned char *parms, int len)
{
  ymhusb_midic *midic = context;

  if (category == CAT_VOICE)
    {
      switch (msg)
	{
	case MIDI_NOTEON:
	  out_event (midic, 0x9, msg | ch, parms[0], parms[1]);
	  break;

	case MIDI_NOTEOFF:
	  out_event (midic, 0x8, msg | ch, parms[0], parms[1]);
	  break;

	case MIDI_KEY_PRESSURE:
	  out_event (midic, 0xa, msg | ch, parms[0], parms[1]);
	  break;
	}

      return;
    }

  if (category == CAT_CHN)
    {
      switch (msg)
	{
	case MIDI_CTL_CHANGE:
	  out_event (midic, 0xb, msg | ch, parms[0], parms[1]);
	  break;

	case MIDI_PGM_CHANGE:
	  out_event (midic, 0xc, msg | ch, parms[0], 0);
	  break;

	case MIDI_CHN_PRESSURE:
	  out_event (midic, 0xd, msg | ch, parms[0], 0);
	  break;

	case MIDI_PITCH_BEND:
	  out_event (midic, 0xe, msg | ch, parms[0], parms[1]);
	  break;
	}
      return;
    }

  if (category == CAT_REALTIME)
    {
      out_event (midic, 0xf, msg | ch, 0, 0);
      return;
    }

  if (category == CAT_MTC)
    {
      out_event (midic, 0x2, 0xf1, parms[0], 0);
      return;
    }

  if (category == CAT_SYSEX)
    {
      int l = len, n;
      unsigned char *d = parms;

      while (l > 0)
	{
	  n = l;
	  if (n > 3)
	    n = 3;

	  switch (n)
	    {
	    case 3:
	      out_event (midic, 0x4, d[0], d[1], d[2]);
	      break;
	    case 2:
	      out_event (midic, 0x6, d[0], d[1], 0);
	      break;
	    case 1:
	      out_event (midic, 0x5, d[0], 0, 0);
	      break;
	    }

	  l -= n;
	  d += n;
	}
      return;
    }
}

 /*ARGSUSED*/ static int
ymhusb_open_output (int dev, int mode, oss_midi_inputbyte_t inputbyte,
		    oss_midi_inputbuf_t inputbuf,
		    oss_midi_outputintr_t outputintr)
{
  oss_native_word flags, phaddr;
  ymhusb_midic *midic;

  midic = midi_devs[dev]->devc;

  MUTEX_ENTER_IRQDISABLE (midic->mutex, flags);
  if (midic->open_mode)
    {
      MUTEX_EXIT_IRQRESTORE (midic->mutex, flags);
      return OSS_EBUSY;
    }

  midic->open_mode = mode;
  midic->output_busy = 0;
  MUTEX_EXIT_IRQRESTORE (midic->mutex, flags);

  midic->tmpbuf = KERNEL_MALLOC (TMPBUF_SIZE);
  memset (midic->tmpbuf, 0, TMPBUF_SIZE);
  midic->outbuf =
    CONTIG_MALLOC (midic->osdev, OUTBUF_SIZE, MEMLIMIT_32BITS, &phaddr, midic->outbuf_dma_handle);
  midic->tmp_len = 0;
  midic->buf_h = midic->buf_t = 0;	/* Empty buffer */
  memset (midic->outbuf, 0, OUTBUF_SIZE);

  if ((midic->parser = midiparser_create (parser_cb, midic)) == NULL)
    {
      midic->open_mode = 0;
      midic->midi_input_intr = NULL;
      cmn_err (CE_WARN, "Cannot create MIDI parser\n");
      return OSS_ENOMEM;
    }

  if ((midic->endpoint_handle =
       udi_open_endpoint (midic->usbdev, midic->endpoint_desc)) == NULL)
    {
      midic->open_mode = 0;
      midic->midi_input_intr = NULL;
      cmn_err (CE_WARN, "Cannot open audio pipe\n");
      return OSS_ENOMEM;
    }
  if ((midic->datapipe =
       udi_usb_alloc_request (midic->usbdev, midic->endpoint_handle, 1,
			      UDI_USBXFER_BULK_WRITE)) == NULL)
    {
      return OSS_EIO;
    }

  return 0;
}

static int
ymhusb_out (int dev, unsigned char midi_byte)
{
  ymhusb_midic *midic = midi_devs[dev]->devc;
  oss_native_word flags;
  int next;

  next = (midic->buf_t + 1) % TMPBUF_NSLOTS;
  if (next == midic->buf_h)	/* Buffer full */
    return 0;			/* Try again later */

  MUTEX_ENTER_IRQDISABLE (midic->mutex, flags);
  midiparser_input (midic->parser, midi_byte);
  MUTEX_EXIT_IRQRESTORE (midic->mutex, flags);

  return 1;
}

 /*ARGSUSED*/ static int
ymhusb_ioctl (int dev, unsigned cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static midi_driver_t ymhusb_input_driver = {
  ymhusb_open_input,
  ymhusb_close_input,
  ymhusb_ioctl,
  ymhusb_out,
};

static void
add_input_device (ymhusb_devc * devc, char *name, void *desc, int caps)
{
  int n;
  ymhusb_midic *midic;
  char tmp[128];

  if (devc->n_inputs >= MAX_INDEVS)
    {
      cmn_err (CE_WARN, "Yamaha MIDI: Too many inputs\n");
      return;
    }

  n = devc->n_inputs++;

  midic = &devc->indevs[n];

  midic->devc = devc;
  midic->osdev = devc->osdev;
  MUTEX_INIT (midic->osdev, midic->mutex, MH_DRV + 1);
  midic->endpoint_desc = desc;
  midic->portnum = n;
  midic->usbdev = devc->usbdev;

  sprintf (tmp, "%s input %d", name, n);
  if ((midic->midi_dev =
       oss_install_mididev (OSS_MIDI_DRIVER_VERSION, "YMHMIDI", tmp,
			    &ymhusb_input_driver,
			    sizeof (ymhusb_input_driver),
			    MFLAG_INPUT, midic, midic->osdev)) < 0)
    {
      cmn_err (CE_CONT, "Failed to install MIDI device\n");
      return;
    }
  midi_devs[midic->midi_dev]->caps |= caps;
}

static midi_driver_t ymhusb_output_driver = {
  ymhusb_open_output,
  ymhusb_close_output,
  ymhusb_ioctl,
  ymhusb_out,
  NULL,
  0,
  ymhusb_flush_output,
  ymhusb_wait_output
};

static void
add_output_device (ymhusb_devc * devc, char *name, void *desc, int caps)
{
  int n;
  ymhusb_midic *midic;
  char tmp[128];

  if (devc->n_outputs >= MAX_OUTDEVS)
    {
      cmn_err (CE_WARN, "Yamaha MIDI: Too many outputs\n");
      return;
    }

  n = devc->n_outputs++;

  midic = &devc->outdevs[n];

  midic->devc = devc;
  midic->osdev = devc->osdev;
  MUTEX_INIT (midic->osdev, midic->mutex, MH_DRV + 1);
  midic->endpoint_desc = desc;
  midic->portnum = n;
  midic->usbdev = devc->usbdev;

  sprintf (tmp, "%s output %d", name, n);
  if ((midic->midi_dev =
       oss_install_mididev (OSS_MIDI_DRIVER_VERSION, "YMHMIDI", tmp,
			    &ymhusb_output_driver,
			    sizeof (ymhusb_output_driver),
			    MFLAG_OUTPUT, midic, midic->osdev)) < 0)
    {
      cmn_err (CE_CONT, "Failed to install MIDI device\n");
      return;
    }

  midi_devs[midic->midi_dev]->caps |= caps;
}

void *
yamaha_usb_midi_driver (ossusb_devc * usb_devc)
{
  ymhusb_devc *devc;
  int i;
  unsigned char *desc;
  int desc_len;
  int caps = MIDI_CAP_EXTERNAL;
  unsigned int devid;

  char *name = "Yamaha USB MIDI";

  if (usb_devc->dev_name != NULL)
    name = usb_devc->dev_name;

  if ((devc = PMALLOC (usb_devc->osdev, sizeof (*devc))) == NULL)
    {
      cmn_err (CE_WARN, "Yamaha MIDI: Out of memory\n");
      return NULL;
    }

  memset (devc, 0, sizeof (*devc));

  devc->unload_func = ymhusb_unload;
  devc->osdev = usb_devc->osdev;
  devc->usbdev = usb_devc->last_usbdev;
  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  devid = (usb_devc->vendor << 16) | usb_devc->product;

  DDB (cmn_err
       (CE_CONT, "Attaching Yamaha MIDI device %08x (%s)\n", devid, name));

  switch (devid)
    {
    case 0x0499101e:		/* PSR-K1 keyboard */
      caps |= MIDI_CAP_PTOP;
      break;
    }

  for (i = 0; i < 32; i++)
    if ((desc =
	 udi_usbdev_get_endpoint (devc->usbdev, 0, i, &desc_len)) != NULL)
      {
	if (desc[2] & 0x80)
	  {
	    add_input_device (devc, name, desc, caps);
	  }
	else
	  {
	    add_output_device (devc, name, desc, caps);
	  }
      }

  return devc;
}
