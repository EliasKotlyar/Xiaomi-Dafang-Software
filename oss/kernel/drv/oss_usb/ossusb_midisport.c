/*
 * Purpose: Dedicated driver for M-Audio/Midiman MIDISPORT USB MIDI family
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
#include "midisport2x2_fw.h"
#include "midisport1x1_fw.h"
#include "oxygen8_fw.h"

/*
 * Audiosport Quattro also use the same packet protocol (no FW load).
 * 		  Out EP 2 is the output and in EP 1 is the input (check this).
 */
#define RECBUF_SIZE	(10*4)
#define DLBUF_SIZE	63
#define MAX_PACK	32
#define PLAYBUF_SIZE	(MAX_PACK*4)
#define QUEUE_ENTRIES	17
#define QUEUE_SIZE	(QUEUE_ENTRIES*PLAYBUF_SIZE)
#define MAX_PORTS	9

static int instance_num = 0;

typedef struct midisport_devc midisport_devc;

typedef unsigned char playbuf_t[PLAYBUF_SIZE];

typedef struct
{
  oss_mutex_t mutex;
  midisport_devc *devc;
  int open_count;
  int busy;
  int max_out_blocks;		/* Num of 4(3) byte blocks allowed in single message */

  playbuf_t *playbuf;		/* QUEUE_SIZE */
  oss_dma_handle_t playbuf_dma_handle;
  int playbuf_h, playbuf_t;

  unsigned char *out_ep_desc;
  udi_usb_request_t *output_pipe;
  udi_endpoint_handle_t *output_endpoint_handle;
} midisport_outqueue_t;

typedef struct
{
  midisport_devc *devc;
  oss_device_t *osdev;
  oss_mutex_t mutex;

  int portnum;
  int midi_dev;
  int open_mode;
  oss_midi_inputbyte_t midi_input_intr;
  playbuf_t output_buf;
  int outbuf_p;

  int outqueue_ix;		/* Output queue 0 or 1 */
  oss_midi_outputintr_t outputintr;
} midisport_midic;

static int alphabethic_numbering = 1;

struct midisport_devc
{
  special_unload_t unload_func;
  int is_dummy;
  oss_device_t *osdev;
  oss_mutex_t mutex;
  ossusb_devc *usb_devc;
  udi_usb_devc *usbdev;

  int instance_num;

  unsigned char *in_ep_desc;

  int num_inputs, num_outputs;
  int open_inputs;
  midisport_midic in_midic[MAX_PORTS];
  midisport_midic out_midic[MAX_PORTS];

  unsigned char *recbuf;
  oss_dma_handle_t recbuf_dma_handle;
  udi_usb_request_t *input_pipe;
  udi_endpoint_handle_t *input_endpoint_handle;

  int num_queues;
  midisport_outqueue_t out_queues[2];
};

static void
load_firmware (midisport_devc * devc, const struct setup_request *setup,
	       char *name)
{
  int err;

  if (setup == NULL)
    {
      cmn_err (CE_WARN, "midisport: No firmware available\n");
      return;
    }

  while (setup->pData != NULL)
    {
#if 0
      cmn_err (CE_CONT, "Load(%x, Rq=%x, Rqt=%x, Val=%x, Ix=%x, data=%x, len=%d\n", devc->usbdev, setup->bRequest,	/* Request */
	       setup->bmRequestType,	/* Rqtype */
	       setup->wValue,	/* Value */
	       setup->wIndex,	/* Index */
	       setup->pData,	/* Data */
	       setup->wLength);	/* Len */
#endif

      err = udi_usb_snd_control_msg (devc->usbdev, 0,	/* Endpoint */
				     setup->bRequest,	/* Request */
				     setup->bmRequestType,	/* Rqtype */
				     setup->wValue,	/* Value */
				     setup->wIndex,	/* Index */
				     (void *) setup->pData,	/* Data */
				     setup->wLength,	/* Len */
				     OSS_HZ);
      if (err < 0)
	{
	  cmn_err (CE_WARN, "%s: Firmware download failed (%d)\n", name, err);
	  return;
	}

      setup++;
    }
}

static void
midisport_unload (void *d)
{
  midisport_devc *devc = d;

  if (devc->is_dummy)
    {
      return;
    }

  MUTEX_CLEANUP (devc->mutex);
}

void *
midisport_init (ossusb_devc * usb_devc)
{
  midisport_devc *devc;
  unsigned int devid;
  char *name = "Unknown device";
  const struct setup_request *setup = NULL;

  devid = (usb_devc->vendor << 16) | usb_devc->product;

  switch (devid)
    {
    case 0x07631001:
      name = "MIDISPORT 2x2";
      setup = midisport2x2_setupRequest;
      break;
    case 0x07631014:
      name = "Oxygen8";
      setup = oxygen8_setupRequest;
      break;
    case 0x07631010:
      name = "MIDISPORT 1x1";
      setup = midisport1x1_setupRequest;
      break;
    }

  cmn_err (CE_CONT, "%s firmware load started\n", name);

  if ((devc = PMALLOC (usb_devc->osdev, sizeof (*devc))) == NULL)
    {
      cmn_err (CE_WARN, "midisport: Out of memory\n");
      return NULL;
    }

  memset (devc, 0, sizeof (*devc));

  devc->unload_func = midisport_unload;
  devc->is_dummy = 1;
  devc->osdev = usb_devc->osdev;
  devc->usb_devc = usb_devc;
  devc->usbdev = usb_devc->last_usbdev;

  load_firmware (devc, setup, name);

  cmn_err (CE_CONT, "%s firmware load completed\n", name);
  return devc;
}

static int midisport_start_input (midisport_devc * devc);

static void
record_callback (udi_usb_request_t * request, void *arg)
{
  midisport_devc *devc = arg;
  int i, l, p;
  unsigned char *data;
  data = udi_usb_request_actdata (request);

  l = udi_usb_request_actlen (request);
  if (l == 0)
    goto restart;

  for (p = 0; p < l - 3; p += 4)
    {
      unsigned char *buf, cmd;
      int nbytes, src;
      midisport_midic *midic;

      buf = data + p;
      cmd = buf[3];
      nbytes = cmd & 0x0f;
      src = (cmd >> 4) & 0x0f;

      if (nbytes == 0)		/* End of data */
	break;

      if (nbytes > 3 || src >= devc->num_inputs)
	continue;		/* No data or error */

      midic = &devc->in_midic[src];

      if (!(midic->open_mode & OPEN_READ) || midic->midi_input_intr == NULL)
	continue;		/* This device is not recording */

      for (i = 0; i < nbytes; i++)
	{
	  midic->midi_input_intr (midic->midi_dev, buf[i]);
	}
    }

restart:
  midisport_start_input (devc);
}

static int
midisport_start_input (midisport_devc * devc)
{
  oss_native_word flags;
  int err = 0;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if ((err =
       udi_usb_submit_request (devc->input_pipe, record_callback, devc,
			       devc->input_endpoint_handle,
			       UDI_USBXFER_INTR_READ, devc->recbuf,
			       RECBUF_SIZE)) < 0)
    {
      cmn_err (CE_WARN, "udi_usb_submit_request failed, err=%d\n", err);
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return err;
}

static void submit_output (midisport_devc * devc, midisport_outqueue_t * q);

 /*ARGSUSED*/ static void
play_callback (udi_usb_request_t * request, void *arg)
{
  midisport_outqueue_t *q = arg;
  midisport_devc *devc = q->devc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (q->mutex, flags);
  q->busy = 0;
  submit_output (devc, q);
  MUTEX_EXIT_IRQRESTORE (q->mutex, flags);
}

 /*ARGSUSED*/ static void
submit_output (midisport_devc * devc, midisport_outqueue_t * q)
{
  int err;
  int max_bytes;
  unsigned char *qbuf;

  if (q->busy)
    return;

  if (q->playbuf_h == q->playbuf_t)	/* Queue empty */
    return;

  qbuf = q->playbuf[q->playbuf_h];
  max_bytes = q->max_out_blocks * 4;

  q->playbuf_h = (q->playbuf_h + 1) % QUEUE_ENTRIES;
  q->busy = 1;

  if ((err =
       udi_usb_submit_request (q->output_pipe, play_callback, q,
			       q->output_endpoint_handle,
			       UDI_USBXFER_BULK_WRITE, qbuf, max_bytes)) < 0)
    {
      cmn_err (CE_WARN, "udi_usb_submit_request (play) failed, err=%d\n",
	       err);
    }
}

 /*ARGSUSED*/ static void
midisport_close_input (int dev, int mode)
{
  oss_native_word flags;
  midisport_midic *midic;
  midisport_devc *devc;
  int do_stop = 0;

  midic = midi_devs[dev]->devc;
  devc = midic->devc;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  devc->open_inputs--;
  if (devc->open_inputs == 0)
    do_stop = 1;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  if (do_stop)
    {
      udi_usb_free_request (devc->input_pipe);
      udi_close_endpoint (devc->input_endpoint_handle);
      if (devc->recbuf != NULL)
	CONTIG_FREE (midic->osdev, devc->recbuf, RECBUF_SIZE, devc->recbuf_dma_handle);
    }

  MUTEX_ENTER_IRQDISABLE (midic->mutex, flags);
  midic->open_mode = 0;
  midic->midi_input_intr = NULL;
  MUTEX_EXIT_IRQRESTORE (midic->mutex, flags);
}

 /*ARGSUSED*/ static int
midisport_open_input (int dev, int mode, oss_midi_inputbyte_t inputbyte,
		      oss_midi_inputbuf_t inputbuf,
		      oss_midi_outputintr_t outputintr)
{
  oss_native_word flags;
  midisport_midic *midic;
  midisport_devc *devc;
  oss_native_word phaddr;
  int do_start = 0;

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


  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  devc->open_inputs++;
  if (devc->open_inputs == 1)
    do_start = 1;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  if (do_start)
    {
      int err;

      devc->recbuf =
	CONTIG_MALLOC (devc->osdev, RECBUF_SIZE, MEMLIMIT_32BITS, &phaddr, devc->recbuf_dma_handle);
      if (devc->recbuf == NULL)
	{
	  cmn_err (CE_CONT, "Failed to allocate the recording buffer\n");
	  return OSS_ENOMEM;
	}

      if ((devc->input_endpoint_handle =
	   udi_open_endpoint (devc->usbdev, devc->in_ep_desc)) == NULL)
	{
	  midic->open_mode = 0;
	  midic->midi_input_intr = NULL;
	  cmn_err (CE_WARN, "Cannot open audio pipe\n");
	  return OSS_ENOMEM;
	}
      if ((devc->input_pipe =
	   udi_usb_alloc_request (devc->usbdev, devc->input_endpoint_handle,
				  1, UDI_USBXFER_INTR_READ)) == NULL)
	{
	  return OSS_EIO;
	}

      if ((err = midisport_start_input (devc)) < 0)
	{
	  cmn_err (CE_WARN, "midisport: Input error %d\n", err);
	  midisport_close_input (dev, mode);
	  return OSS_EIO;
	}
    }

  return 0;
}

static int
open_output_queue (midisport_devc * devc, int queue_ix)
{
  oss_native_word flags;
  midisport_outqueue_t *q;
  int open_count;
  oss_native_word phaddr;

  if (queue_ix < 0 || queue_ix >= devc->num_queues)
    {
      cmn_err (CE_WARN, "Bad output queue index %d\n", queue_ix);
      return OSS_EIO;
    }

  q = &devc->out_queues[queue_ix];

  MUTEX_ENTER_IRQDISABLE (q->mutex, flags);
  open_count = q->open_count++;

  if (open_count == 0)		/* First open */
    {
      q->playbuf_h = q->playbuf_t = 0;
      q->busy = 0;
    }
  MUTEX_EXIT_IRQRESTORE (q->mutex, flags);

  if (open_count == 0)		/* First open */
    {
      if ((q->playbuf =
	   CONTIG_MALLOC (devc->osdev, QUEUE_SIZE, MEMLIMIT_32BITS,
			  &phaddr, q->playbuf_dma_handle)) == NULL)
	{
	  cmn_err (CE_WARN, "Failed to allocate output buffer (%d bytes)\n",
		   QUEUE_SIZE);
	  q->open_count--;
	  return OSS_ENOMEM;
	}

      if ((q->output_endpoint_handle =
	   udi_open_endpoint (devc->usbdev, q->out_ep_desc)) == NULL)
	{
	  cmn_err (CE_WARN, "Failed to open output endpoint\n");
	  q->open_count--;
	  return OSS_EIO;
	}

      if ((q->output_pipe =
	   udi_usb_alloc_request (devc->usbdev, q->output_endpoint_handle, 1,
				  UDI_USBXFER_BULK_WRITE)) == NULL)
	{
	  cmn_err (CE_WARN, "Failed to allocate output request\n");
	  q->open_count--;
	  return OSS_EIO;
	}

    }
  return 0;
}

static void
close_output_queue (midisport_devc * devc, int queue_ix)
{
  oss_native_word flags;
  midisport_outqueue_t *q;
  int open_count;

  if (queue_ix < 0 || queue_ix >= devc->num_queues)
    {
      cmn_err (CE_WARN, "Bad output queue index %d\n", queue_ix);
      return;
    }

  q = &devc->out_queues[queue_ix];

  if (q->open_count <= 0)	/* Was not opened at all */
    return;

  MUTEX_ENTER_IRQDISABLE (q->mutex, flags);
  open_count = q->open_count--;
  MUTEX_EXIT_IRQRESTORE (q->mutex, flags);

  if (open_count <= 1)		/* Queue not needed any more */
    {
      udi_usb_free_request (q->output_pipe);
      udi_close_endpoint (q->output_endpoint_handle);
      if (q->playbuf != NULL)
	CONTIG_FREE (devc->osdev, q->playbuf, QUEUE_SIZE, q->playbuf_dma_handle);
    }
}

 /*ARGSUSED*/ static void
midisport_close_output (int dev, int mode)
{
  oss_native_word flags;
  midisport_midic *midic;
  midisport_devc *devc;

  midic = midi_devs[dev]->devc;
  devc = midic->devc;

  close_output_queue (devc, midic->outqueue_ix);

  MUTEX_ENTER_IRQDISABLE (midic->mutex, flags);
  midic->open_mode = 0;
  midic->outputintr = NULL;
  MUTEX_EXIT_IRQRESTORE (midic->mutex, flags);
}

 /*ARGSUSED*/ static int
midisport_open_output (int dev, int mode, oss_midi_inputbyte_t inputbyte,
		       oss_midi_inputbuf_t inputbuf,
		       oss_midi_outputintr_t outputintr)
{
  oss_native_word flags;
  midisport_midic *midic;
  midisport_devc *devc;
  int err;

  midic = midi_devs[dev]->devc;
  devc = midic->devc;

  MUTEX_ENTER_IRQDISABLE (midic->mutex, flags);
  if (midic->open_mode)
    {
      MUTEX_EXIT_IRQRESTORE (midic->mutex, flags);
      return OSS_EBUSY;
    }

  midic->open_mode = mode;
  midic->midi_input_intr = NULL, midic->outputintr = outputintr;
  midic->outbuf_p = 0;
  memset (midic->output_buf, 0, sizeof (midic->output_buf));
  MUTEX_EXIT_IRQRESTORE (midic->mutex, flags);

  if ((err = open_output_queue (devc, midic->outqueue_ix)) < 0)
    {
      cmn_err (CE_WARN, "Failed to open the output queue (%d)\n", err);
      midisport_close_output (dev, mode);
      return err;
    }

  return 0;
}

static int
do_flush (midisport_devc * devc, midisport_midic * midic,
	  midisport_outqueue_t * q)
{
  int next;
  unsigned char *qbuf;
  int max_bytes;

  /*
   * Move stuff from the intermediate buffer to the EP queue
   */
  max_bytes = q->max_out_blocks * 4;
  next = (q->playbuf_t + 1) % QUEUE_ENTRIES;
  if (next == q->playbuf_h)	/* No more space in any of the buffers */
    {
      return 0;
    }

  qbuf = q->playbuf[q->playbuf_t];
  memcpy (qbuf, midic->output_buf, max_bytes);
  memset (midic->output_buf, 0, max_bytes);
  midic->outbuf_p = 0;
  q->playbuf_t = next;

  submit_output (devc, q);

  return 1;
}

#if 0
static void
midisport_flush_output (int dev)
{
  midisport_midic *midic = midi_devs[dev]->devc;
  midisport_devc *devc = midic->devc;
  midisport_outqueue_t *q;
  oss_native_word flags;
  oss_native_word qflags;

  q = &devc->out_queues[midic->outqueue_ix];

  MUTEX_ENTER_IRQDISABLE (midic->mutex, flags);
  MUTEX_ENTER_IRQDISABLE (q->mutex, qflags);
  do_flush (devc, midic, q);
  MUTEX_EXIT_IRQRESTORE (q->mutex, qflags);
  MUTEX_EXIT_IRQRESTORE (midic->mutex, flags);
}
#endif

static int
midisport_bulk_write (int dev, unsigned char *buf, int len)
{
  midisport_midic *midic = midi_devs[dev]->devc;
  midisport_devc *devc = midic->devc;
  oss_native_word flags;
  oss_native_word qflags;
  int i, l = 0, n = 0, p;
  //int max_bytes;
  midisport_outqueue_t *q;
  unsigned char *outbuf;

  if (midic->outqueue_ix < 0 || midic->outqueue_ix >= devc->num_queues)
    {
      cmn_err (CE_WARN, "Bad output queue index %d\n", midic->outqueue_ix);
      return OSS_EIO;
    }

  q = &devc->out_queues[midic->outqueue_ix];

  MUTEX_ENTER_IRQDISABLE (midic->mutex, flags);

  //max_bytes = q->max_out_blocks * 4;

  for (i = 0; i < len; i += 3)
    {
      l = len - i;
      if (l > 3)
	l = 3;
      p = midic->outbuf_p;

      if ((p + 4) >= q->max_out_blocks)
	{
	  int next;
	  MUTEX_ENTER_IRQDISABLE (q->mutex, qflags);

	  next = (q->playbuf_t + 1) % QUEUE_ENTRIES;
	  if (next == q->playbuf_h)	/* No more space in any of the buffers */
	    {
	      MUTEX_EXIT_IRQRESTORE (q->mutex, qflags);
	      MUTEX_EXIT_IRQRESTORE (midic->mutex, flags);
	      return n;
	    }

	  if (!do_flush (devc, midic, q))	/* Output FIFO full */
	    {
	      MUTEX_EXIT_IRQRESTORE (q->mutex, qflags);
	      return n;
	    }
	  p = midic->outbuf_p;
	  MUTEX_EXIT_IRQRESTORE (q->mutex, qflags);
	}

      outbuf = midic->output_buf + p;
      memcpy (outbuf, buf, l);
      outbuf[3] = (midic->portnum << 4) | l;
      midic->outbuf_p += 4;
      if (midic->outbuf_p >= q->max_out_blocks)
	do_flush (devc, midic, q);
      n += l;
    }

  MUTEX_EXIT_IRQRESTORE (midic->mutex, flags);

  return n;
}

 /*ARGSUSED*/ static int
midisport_ioctl (int dev, unsigned cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static midi_driver_t midisport_midi_input_driver = {
  midisport_open_input,
  midisport_close_input,
  midisport_ioctl
};

static midi_driver_t midisport_midi_output_driver = {
  midisport_open_output,
  midisport_close_output,
  midisport_ioctl,
  NULL,
  midisport_bulk_write,
  8 * 3				/* 8 packets of 3 bytes */
};

static void
create_inputs (midisport_devc * devc, char *name, int ep, int ninputs)
{
  int desc_len;
  int i, ix = ep;
  char portid = 'A';
  unsigned char *desc;
  int flags = MFLAG_INPUT;

  for (i = 0; i < 32; i++)
    if ((desc =
	 udi_usbdev_get_endpoint (devc->usbdev, 0, i, &desc_len)) != NULL)
      {
	if (desc[2] == (ep | 0x80))
	  ix = i;
      }

  if (!alphabethic_numbering)
    portid = '1';

  if (ninputs > MAX_PORTS)
    {
      cmn_err (CE_WARN, "Too many input ports %d\n", ninputs);
      return;
    }

  if ((devc->in_ep_desc =
       udi_usbdev_get_endpoint (devc->usbdev, 0, ix, &desc_len)) == NULL)
    {
      cmn_err (CE_WARN, "Bad endpoint %d\n", ep);
      return;
    }

  if (!(devc->in_ep_desc[2] & 0x80))
    {
      cmn_err (CE_WARN, "Bad endpoint %d - not input\n", ep);
    }

  for (i = 0; i < ninputs; i++)
    {
      midisport_midic *midic = midic = &devc->in_midic[i];
      char tmp[128];

      midic->devc = devc;
      midic->osdev = devc->osdev;
      MUTEX_INIT (devc->osdev, midic->mutex, MH_DRV + 1);
      midic->portnum = i;
      devc->num_inputs++;

      if (i == 8)
	{
	  sprintf (tmp, "%s SMPTE status", name);
	  flags = MFLAG_MTC;
	}
      else
	sprintf (tmp, "%s input %c", name, portid + i);

      midic->midi_dev =
	oss_install_mididev (OSS_MIDI_DRIVER_VERSION, "MIDISPORT", tmp,
			     &midisport_midi_input_driver,
			     sizeof (midi_driver_t),
			     flags, midic, midic->osdev);
    }
}

static void
create_output (midisport_devc * devc, char *name, int queue_ix)
{
  int n;
  midisport_midic *midic = NULL;
  char tmp[128];
  char portid = 'A';
  int flags = MFLAG_OUTPUT;

  if (!alphabethic_numbering)
    portid = '1';

  n = devc->num_outputs++;
  midic = &devc->out_midic[n];
  midic->devc = devc;
  MUTEX_INIT (devc->osdev, midic->mutex, MH_DRV + 1);
  midic->osdev = devc->osdev;
  midic->portnum = n;
  midic->outqueue_ix = queue_ix;

  if (n == 8)
    {
      sprintf (tmp, "%s SMPTE control", name);
      flags = MFLAG_MTC;
    }
  else
    sprintf (tmp, "%s output %c", name, portid + n);

  midic->midi_dev =
    oss_install_mididev (OSS_MIDI_DRIVER_VERSION, "MIDISPORT", tmp,
			 &midisport_midi_output_driver,
			 sizeof (midi_driver_t),
			 flags, midic, midic->osdev);
}

static void
init_outqueue (midisport_devc * devc, int ix, int ep, int max_blocks)
{
  int desc_len;
  midisport_outqueue_t *q;
  int i, epix = ep;

  unsigned char *desc;

  for (i = 0; i < 32; i++)
    if ((desc =
	 udi_usbdev_get_endpoint (devc->usbdev, 0, i, &desc_len)) != NULL)
      {
	if (desc[2] == ep)
	  epix = i;
      }

  if (ix < 0 || ix > devc->num_queues)
    {
      cmn_err (CE_WARN, "Endpoint index outside bounds\n");
      return;
    }

  q = &devc->out_queues[ix];
  MUTEX_INIT (devc->osdev, q->mutex, MH_DRV + 2);
  q->max_out_blocks = max_blocks;
  q->devc = devc;

  if ((q->out_ep_desc =
       udi_usbdev_get_endpoint (devc->usbdev, 0, epix, &desc_len)) == NULL)
    {
      cmn_err (CE_WARN, "Bad endpoint %d\n", ep);
      return;
    }

  if (q->out_ep_desc[2] & 0x80)
    {
      cmn_err (CE_WARN, "Bad endpoint %d - not output\n", ep);
    }
  cmn_err (CE_CONT, "Attaching output endpoint %d=%02x\n", ep,
	   q->out_ep_desc[2]);
}

void *
midisport_driver (ossusb_devc * usb_devc)
{
  midisport_devc *devc;
  char *name;
  unsigned int devid;
  int i;
  unsigned char *desc;
  int desc_len;

  devid = (usb_devc->vendor << 16) | usb_devc->product;

  if ((devc = PMALLOC (usb_devc->osdev, sizeof (*devc))) == NULL)
    {
      cmn_err (CE_WARN, "midisport: Out of memory\n");
      return NULL;
    }

  memset (devc, 0, sizeof (*devc));

  devc->unload_func = midisport_unload;
  devc->osdev = usb_devc->osdev;
  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);

  devc->usb_devc = usb_devc;
  devc->usbdev = usb_devc->last_usbdev;
  devc->instance_num = instance_num++;

  for (i = 0; i < 32; i++)
    if ((desc =
	 udi_usbdev_get_endpoint (devc->usbdev, 0, i, &desc_len)) != NULL)
      {
	if (desc[2] & 0x80)
	  {
	    cmn_err (CE_CONT, "Input endpoint %02x\n", desc[2]);
	  }
	else
	  {
	    cmn_err (CE_CONT, "Output endpoint %02x\n", desc[2]);
	  }
      }

  alphabethic_numbering = 1;

  devc->num_queues = 2;

  switch (devid)
    {
    case 0x07631002:
      name = "Midisport 2x2";
      create_inputs (devc, name, 1, 2);
      init_outqueue (devc, 0, 2, 8);
      init_outqueue (devc, 1, 4, 8);
      create_output (devc, name, 0);
      create_output (devc, name, 1);
      break;

    case 0x07631011:
      name = "Midisport 1x1";
      devc->num_queues = 1;
      init_outqueue (devc, 0, 2, 8);
      create_inputs (devc, name, 1, 1);
      create_output (devc, name, 0);
      break;

    case 0x07631015:
      name = "Oxygen8";
      devc->num_queues = 1;
      create_inputs (devc, name, 1, 1);
      init_outqueue (devc, 0, 2, 8);
      create_output (devc, name, 0);
      break;

    case 0x07632001:
      name = "Quattro";
      devc->num_queues = 1;
      create_inputs (devc, name, 1, 1);
      init_outqueue (devc, 0, 2, 8);
      create_output (devc, name, 0);
      break;

    case 0x07631031:
      name = "Midisport 8x8";
      alphabethic_numbering = 0;
      create_inputs (devc, name, 2, 9);
      init_outqueue (devc, 0, 2, 10);
      init_outqueue (devc, 1, 4, 8);
      create_output (devc, name, 0);
      create_output (devc, name, 1);
      create_output (devc, name, 0);
      create_output (devc, name, 1);
      create_output (devc, name, 0);
      create_output (devc, name, 1);
      create_output (devc, name, 0);
      create_output (devc, name, 1);
      create_output (devc, name, 0);	/* SMPTE control */
      break;

    case 0x07631021:
      name = "Midisport 4x4";
      create_inputs (devc, name, 2, 4);
      init_outqueue (devc, 0, 2, 16);
      init_outqueue (devc, 1, 4, 16);
      create_output (devc, name, 0);
      create_output (devc, name, 1);
      create_output (devc, name, 0);
      create_output (devc, name, 1);
      break;

    default:
      cmn_err (CE_WARN, "Unrecognized MIDI device %08x\n", devid);
    }

  return devc;
}
