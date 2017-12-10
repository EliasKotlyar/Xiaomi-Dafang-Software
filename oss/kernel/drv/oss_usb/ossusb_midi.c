/*
 * Purpose: USB MIDI streaming interface support
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
#include "ossusb.h"

#define RECBUF_SIZE	32
#define PLAYBUF_SIZE	32

/*
 * MS Class-Specific Interface Descriptor Subtypes
 */
#define MS_HEADER		0x01
#define MIDI_IN_JACK		0x02
#define MIDI_OUT_JACK		0x03
#define ELEMENT			0x04

/* Jack types */
#define JT_EMBEDDED		0x01
#define JT_EXTERNAL		0x02

static int usb_midi_start_input (ossusb_devc * devc, ossusb_midic * midic);

static void
record_callback (udi_usb_request_t * request, void *arg)
{
  ossusb_midic *midic = arg;
  ossusb_devc *devc = midic->devc;
  int i, l;

  l = udi_usb_request_actlen (request);
  if (l == 0)
    goto restart;

  for (i = 0; i < l; i++)
    cmn_err (CE_CONT, "%02x ", devc->recbuf[i]);
  cmn_err (CE_CONT, "\n");

restart:
  usb_midi_start_input (devc, midic);
}

static int
usb_midi_start_input (ossusb_devc * devc, ossusb_midic * midic)
{
  oss_native_word flags;
  int err = 0;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if ((err =
       udi_usb_submit_request (devc->input_pipe, record_callback, midic,
			       midic->in_endpoint_handle,
			       UDI_USBXFER_BULK_READ, devc->recbuf,
			       RECBUF_SIZE)) < 0)
    {
      cmn_err (CE_WARN, "usbmidi: udi_usb_submit_request failed, err=%d\n",
	       err);
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return err;
}

static int usb_midi_start_output (ossusb_devc * devc, ossusb_midic * midic);

 /*ARGSUSED*/ static void
play_callback (udi_usb_request_t * request, void *arg)
{
  ossusb_midic *midic = arg;
  ossusb_devc *devc = midic->devc;

  devc->output_busy = 0;
  usb_midi_start_output (devc, midic);
}

static int
usb_midi_start_output (ossusb_devc * devc, ossusb_midic * midic)
{
  oss_native_word flags;
  int err = 0, l = 0;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  l = devc->q_nbytes;
  if (l > PLAYBUF_SIZE)
    l = PLAYBUF_SIZE;

  if (l == 0 || devc->output_busy)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return 0;
    }

  memcpy (devc->playbuf, devc->queue, l);

  if ((err =
       udi_usb_submit_request (devc->output_pipe, play_callback, midic,
			       midic->out_endpoint_handle,
			       UDI_USBXFER_BULK_WRITE, devc->playbuf, l)) < 0)
    {
      cmn_err (CE_WARN, "usbmidi: udi_usb_submit_request() failed, err=%d\n",
	       err);
    }
  else
    devc->output_busy = 1;

  /* Remove the vbytes from the queue */
  if (l >= devc->q_nbytes)
    devc->q_nbytes = 0;
  else
    {
      int n = devc->q_nbytes - l;
      memcpy (devc->queue, devc->queue + l, n);
      devc->q_nbytes -= l;
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return err;
}

static void
usb_midi_close (int dev, int mode)
{
  oss_native_word flags;
  ossusb_midic *midic;
  ossusb_devc *devc;

  midic = midi_devs[dev]->devc;
  devc = midic->devc;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  midic->open_mode = 0;
  midic->midi_input_intr = NULL;
  midic->midi_output_intr = NULL;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  if (mode & OPEN_READ)
    {
      if (devc->input_pipe != NULL)
	udi_usb_cancel_request (devc->input_pipe);
      devc->input_pipe = NULL;

      if (devc->recbuf != NULL)
	CONTIG_FREE (devc->osdev, devc->recbuf, RECBUF_SIZE, devc->recbuf_dma_handle);
      devc->recbuf = NULL;
      if (devc->playbuf != NULL)
	CONTIG_FREE (devc->osdev, devc->playbuf, PLAYBUF_SIZE, devc->playbuf_dma_handle);
      devc->playbuf = NULL;
      udi_close_endpoint (midic->in_endpoint_handle);
    }

  if (mode & OPEN_WRITE)
    {
      udi_usb_cancel_request (devc->output_pipe);
      udi_close_endpoint (midic->out_endpoint_handle);
    }
}

 /*ARGSUSED*/ static int
usb_midi_open (int dev, int mode, oss_midi_inputbyte_t inputbyte,
	       oss_midi_inputbuf_t inputbuf, oss_midi_outputintr_t outputintr)
{
  oss_native_word flags, phaddr;
  ossusb_midic *midic;
  ossusb_devc *devc;

  midic = midi_devs[dev]->devc;
  devc = midic->devc;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (midic->open_mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  midic->open_mode = mode;
  midic->midi_input_intr = inputbyte;
  midic->midi_output_intr = outputintr;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  if (mode & OPEN_WRITE)
    {
      if ((midic->out_endpoint_handle =
	   udi_open_endpoint (midic->usbdev,
			      midic->out_endpoint_desc)) == NULL)
	{
	  usb_midi_close (dev, mode);
	  cmn_err (CE_WARN, "Cannot open midi output pipe\n");
	  return OSS_ENOMEM;
	}

      if ((devc->output_pipe =
	   udi_usb_alloc_request (midic->usbdev, midic->out_endpoint_handle,
				  1, UDI_USBXFER_BULK_WRITE)) == NULL)
	{
	  cmn_err (CE_WARN, "usbmidi: Failed to allocate output pipe\n");
	}

      devc->playbuf =
	CONTIG_MALLOC (devc->osdev, PLAYBUF_SIZE, MEMLIMIT_32BITS, &phaddr, devc->playbuf_dma_handle);

      devc->output_busy = 0;
      devc->q_nbytes = 0;
    }

  if (mode & OPEN_READ)
    {
      int err;

      if ((midic->in_endpoint_handle =
	   udi_open_endpoint (midic->usbdev,
			      midic->in_endpoint_desc)) == NULL)
	{
	  usb_midi_close (dev, mode);
	  cmn_err (CE_WARN, "Cannot open midi input pipe\n");
	  return OSS_ENOMEM;
	}
      if ((devc->input_pipe =
	   udi_usb_alloc_request (midic->usbdev, midic->in_endpoint_handle, 1,
				  UDI_USBXFER_BULK_READ)) == NULL)
	{
	  cmn_err (CE_WARN, "usbmidi: Failed to allocate input pipe\n");
	}
      devc->recbuf =
	CONTIG_MALLOC (devc->osdev, RECBUF_SIZE, MEMLIMIT_32BITS, &phaddr, devc->recbuf_dma_handle);

      if ((err = usb_midi_start_input (devc, midic)) < 0)
	{
	  cmn_err (CE_WARN, "usbmidi: Input error %d\n", err);
	  usb_midi_close (dev, mode);
	  return OSS_EIO;
	}
    }

  return 0;
}

static int
usb_midi_out (int dev, unsigned char data)
{
  ossusb_midic *midic = midi_devs[dev]->devc;
  ossusb_devc *devc;
  oss_native_word flags;
  unsigned char *buf;

  devc = midic->devc;

  cmn_err (CE_CONT, "Send %02x\n", data);
  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if ((devc->q_nbytes + 4) >= Q_MAX)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return 0;
    }

  buf = devc->queue + devc->q_nbytes;

  memset (buf, 0, 4);
  buf[0] = 0x0f;
  buf[1] = data;

  devc->q_nbytes += 4;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  usb_midi_start_output (devc, midic);
  return 1;
}

 /*ARGSUSED*/ static int
usb_midi_ioctl (int dev, unsigned cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static midi_driver_t usb_midi_driver = {
  usb_midi_open,
  usb_midi_close,
  usb_midi_ioctl,
  usb_midi_out,
};

 /*ARGSUSED*/
  ossusb_devc *
ossusb_init_midistream (ossusb_devc * devc, udi_usb_devc * usbdev, int inum,
			int reinit)
{
  int i, n;
  int p, l;
  char tmp[64];
  unsigned char *in_endpoints[16], *out_endpoints[16];
  void *in_endpoint_desc = NULL, *out_endpoint_desc = NULL;
  int num_in_endpoints = 0, num_out_endpoints = 0;
  int cin = 0, cout = 0;
  ossusb_midic *midic;
  unsigned char *desc, *d;
  int desc_len;

  for (i = 0; i < 32; i++)
    if ((desc = udi_usbdev_get_endpoint (usbdev, 0, i, &desc_len)) != NULL)
      {
	unsigned char *ep;

	if (desc_len > 100)
	  desc_len = 100;

	cmn_err (CE_CONT, "Endpoint %d (%d)", i, desc_len);
	ossusb_dump_desc (desc, desc_len);
	ep = desc;

	if (desc[2] & 0x80)
	  {
	    in_endpoints[num_in_endpoints++] = ep;
	  }
	else
	  {
	    out_endpoints[num_out_endpoints++] = ep;
	  }
      }

  cmn_err (CE_CONT, "%d input endpoints: ", num_in_endpoints);
  for (i = 0; i < num_in_endpoints; i++)
    cmn_err (CE_CONT, "%02x ", in_endpoints[i][2]);
  cmn_err (CE_CONT, "\n");
  cmn_err (CE_CONT, "%d input endpoints: ", num_out_endpoints);
  for (i = 0; i < num_out_endpoints; i++)
    cmn_err (CE_CONT, "%02x ", out_endpoints[i][2]);
  cmn_err (CE_CONT, "\n");

  cmn_err (CE_CONT, "USB MIDI stream\n");
  desc = udi_usbdev_get_altsetting (usbdev, 0, &desc_len);
  if (desc == NULL || desc_len < 3)
    {
      cmn_err (CE_WARN, "usbmidi: bad altsetting\n");
      return NULL;
    }
  // ossusb_dump_desc (desc, desc_len);
  p = 0;
  while (p < desc_len)
    {
      d = desc + p;

      l = *d;
      //if (usb_trace > 1)
      {
	cmn_err (CE_CONT, "MIDI streaming desc: ");
	for (i = 0; i < l; i++)
	  cmn_err (CE_CONT, "%02x ", d[i]);
	cmn_err (CE_CONT, "\n");
      }

      if (d[1] != CS_INTERFACE)
	{
	  cmn_err (CE_WARN, "usbmidi: Unrecognized descriptor: \n");
	  ossusb_dump_desc (d, l);
	  p += l;
	  continue;
	}

      switch (d[2])
	{
	case MS_HEADER:
	  cmn_err (CE_CONT, "MS_HEADER: ");
	  cmn_err (CE_CONT, "v%x.%02x ", d[3], d[4]);
	  break;

	case MIDI_IN_JACK:
	  cmn_err (CE_CONT, "MIDI_IN_JACK: ");
	  cmn_err (CE_CONT, "Type %d, ID %02x, iJack %d\n", d[3], d[4], d[5]);
	  in_endpoint_desc = in_endpoints[cin];
	  if (cin < num_in_endpoints - 1)
	    cin++;
	  break;

	case MIDI_OUT_JACK:
	  cmn_err (CE_CONT, "MIDI_OUT_JACK: ");
	  cmn_err (CE_CONT, "Type %d, ID %02x, iJack %d\n", d[3], d[4], d[5]);
	  n = d[5];
	  cmn_err (CE_CONT, "\t%d inputs: ", n);
	  for (i = 0; i < n; i++)
	    cmn_err (CE_CONT, "%02x/%02x ", d[6 + i * 2], d[6 + i * 2 + 1]);
	  cmn_err (CE_CONT, "\n");
	  out_endpoint_desc = out_endpoints[cout];
	  if (cout < num_out_endpoints - 1)
	    cout++;
	  break;

	case ELEMENT:
	  cmn_err (CE_CONT, "ELEMENT\n");
	  break;
	}

      p += l;
    }


#if 1
  if (reinit)
    for (i = 0; i < devc->num_mididevs; i++)
      if (devc->midic[i].in_endpoint_desc == in_endpoint_desc)	/* Already registered this */
	if (devc->midic[i].out_endpoint_desc == out_endpoint_desc)	/* Already registered this */
	  {
	    return devc;
	  }
#endif

  midic = &devc->midic[devc->num_mididevs];

  devc->osdev = devc->osdev;
  midic->devc = devc;
  midic->usbdev = usbdev;
  midic->in_endpoint_desc = in_endpoint_desc;
  midic->out_endpoint_desc = out_endpoint_desc;

  strcpy (tmp, devc->dev_name);

  midic->midi_dev =
    oss_install_mididev (OSS_MIDI_DRIVER_VERSION, "USBMIDI", tmp,
			 &usb_midi_driver, sizeof (midi_driver_t),
			 0, midic, devc->osdev);
  devc->num_mididevs++;

  return devc;
}

 /*ARGSUSED*/ void
ossusb_disconnect_midistream (ossusb_devc * devc)
{
}
