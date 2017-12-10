/*
 * Purpose: USB audio streaming interface support
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

#define SAMPLING_FREQ_CONTROL      0x01
#define PITCH_CONTROL              0x02

#define FORMAT_II_UNDEFINED		0x1000
#define FORMAT_II_MPEG			0x1001
#define FORMAT_II_AC3			0x1002

#define TMPBUF_SIZE	4096

#if 0
static int
read_control_value (ossusb_devc * devc, int endpoint, int ctl, int l)
{
  unsigned char buf[4];
  int len, i, v;

  memset (buf, 0, sizeof (buf));

  len = udi_usb_rcv_control_msg (devc->mixer_usbdev, 0,	// endpoint
				 GET_CUR, USB_RECIP_ENDPOINT | USB_TYPE_CLASS,	// rqtype
				 ctl << 8,	// value
				 endpoint,	// index
				 buf,	// buffer
				 l,	// buflen
				 OSS_HZ);
  if (len < 0)
    {
      cmn_err (CE_WARN, "Endpoint read error %d\n", len);
      return 0;
    }

  cmn_err (CE_CONT, "Read len %d (%d): ", len, l);
  for (i = 0; i < len; i++)
    cmn_err (CE_CONT, "%02x ", buf[i]);

  switch (len)
    {
    case 3:
      v = buf[0] | (buf[1] << 8) | (buf[2] << 16);
      break;

    default:
      cmn_err (CE_CONT, "oss usbaudio: Bad control read (%d)\n", l);
    }

  cmn_err (CE_CONT, "= %d\n", v);

  return v;
}
#endif

static int
write_control_value (ossusb_devc * devc, udi_endpoint_handle_t * endpoint,
		     int ctl, int l, unsigned int v)
{
  unsigned char buf[4];
  int len;

  memset (buf, 0, sizeof (buf));

  switch (l)
    {
    case 3:
      buf[0] = (v) & 0xff;
      buf[1] = (v >> 8) & 0xff;
      buf[2] = (v >> 16) & 0xff;
      break;

    default:
      cmn_err (CE_CONT, "oss usbaudio: Bad control size %d\n", l);
      return OSS_EIO;
    }

  len = udi_usb_snd_control_msg (devc->mixer_usbdev, 0,	// endpoint
				 SET_CUR, USB_RECIP_ENDPOINT | USB_TYPE_CLASS,	// rqtype
				 ctl << 8,	// value
				 udi_endpoint_get_num (endpoint),	// index
				 buf,	// buffer
				 l,	// buflen
				 OSS_HZ);
  if (len < 0)
    {
      cmn_err (CE_WARN, "Endpoint control write error %d\n", len);
      return OSS_EIO;
    }

  return len;
}

static void
set_fraglimits (adev_t * adev, ossusb_portc * portc)
{
  int l, m;

  if (portc->bytes_per_sample < 1)
    portc->bytes_per_sample = 1;

  l = portc->bytes_per_sample * adev->min_channels * portc->speed / 1000;
  l = (l / portc->bytes_per_sample) * portc->bytes_per_sample;

  m = 2;

  while (m < l)
    m *= 2;

  adev->min_block = m;
  adev->max_block = TMPBUF_SIZE / 2;
  portc->fragment_size = l;
}

static int
usbaudio_set_rate (int dev, int arg)
{
  adev_p adev = audio_engines[dev];
  ossusb_portc *portc = adev->portc;
  ossusb_devc *devc = adev->devc;

  int i, x, diff, bestdiff;

  if (devc->disabled)
    return OSS_EPIPE;

  if (arg == 0)
    return portc->speed;

  if (arg < adev->min_rate)
    arg = adev->min_rate;
  if (arg > adev->max_rate)
    arg = adev->max_rate;

  if (!(adev->caps & PCM_CAP_FREERATE))
    {
      /* Search for the nearest supported rate */
      bestdiff = 0x7fffffff;
      x = -1;

      for (i = 0; i < adev->nrates; i++)
	{
	  diff = arg - adev->rates[i];

	  if (diff < 0)
	    diff = -diff;	/* ABS */
	  if (diff < bestdiff)
	    {
	      x = i;
	      bestdiff = diff;
	    }
	}

      if (x > -1)
	arg = adev->rates[x];
    }

  portc->speed = arg;
  set_fraglimits (adev, portc);

  return portc->speed;
}

/*ARGSUSED*/
static short
usbaudio_set_channels (int dev, short arg)
{
  adev_p adev = audio_engines[dev];
  ossusb_devc *devc = adev->devc;

  if (devc->disabled)
    return OSS_EPIPE;

  return adev->min_channels;	/* max_channels should be the same too */
}

/*ARGSUSED*/
static unsigned int
usbaudio_set_format (int dev, unsigned int arg)
{
  adev_p adev = audio_engines[dev];
  ossusb_devc *devc = adev->devc;

  if (devc->disabled)
    return adev->oformat_mask;

  return adev->oformat_mask;	/* iformat_mask should be the same  too */
}

/*ARGSUSED*/
static int
usbaudio_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  ossusb_devc *devc = audio_engines[dev]->devc;
  if (devc->disabled)
    return OSS_EPIPE;

  return OSS_EINVAL;
}

/*ARGSUSED*/
static int
setup_format_I (ossusb_devc * devc, ossusb_portc * portc, adev_p adev,
		unsigned char *d, int l)
{
  int min_rate = 0, max_rate = 0;
  int i, n;
  int frame_size, bits, channels;
  unsigned int fmt = 0;

  if (usb_trace > 1)
    {
      cmn_err (CE_CONT,
	       "AS_FORMAT_TYPE: FORMAT_TYPE_I, #ch %d, framesize %d, bits %d, freq_type %02x, freq=",
	       d[4], d[5], d[6], d[7]);
      if (d[7] == 0)
	cmn_err (CE_CONT, "%d-%d ", ossusb_get_int (&d[8], 3),
		 ossusb_get_int (&d[11], 3));
      {
	for (i = 0; i < d[7]; i++)
	  cmn_err (CE_CONT, "%d ", ossusb_get_int (&d[8 + i * 3], 3));
	cmn_err (CE_CONT, " ");
      }

      switch (d[3])
	{
	case 0x00:
	  cmn_err (CE_CONT, "Undefined ");
	  break;
	case 0x01:
	  cmn_err (CE_CONT, "PCM ");
	  break;
	case 0x02:
	  cmn_err (CE_CONT, "PCM8 ");
	  break;
	case 0x03:
	  cmn_err (CE_CONT, "float ");
	  break;
	case 0x04:
	  cmn_err (CE_CONT, "A-Law ");
	  break;
	case 0x05:
	  cmn_err (CE_CONT, "u-Law ");
	  break;
	}
      cmn_err (CE_CONT, "\n");
    }

  channels = d[4];
  frame_size = d[5];
  bits = d[6];
  UDB (cmn_err (CE_CONT, "Channels %d, bits %d (%d bytes)\n", channels, bits,
		frame_size));

  adev->min_channels = adev->max_channels = channels;
  portc->convert_3byte = 0;

  switch (d[3])
    {
    case 0x01:
      fmt = AFMT_S16_LE;
      break;
    case 0x02:
      fmt = AFMT_U8;
      break;
    case 0x03:
      fmt = AFMT_FLOAT;
      break;
    case 0x04:
      fmt = AFMT_A_LAW;
      break;
    case 0x05:
      fmt = AFMT_MU_LAW;
      break;
    }

  if (fmt == AFMT_S16_LE)	/* Have to check the frame size too */
    {
      switch (frame_size)
	{
	case 1:
	  fmt = AFMT_S8;
	  break;
	case 2:
	  fmt = AFMT_S16_LE;
	  break;
	case 3:
	  fmt = AFMT_S32_LE;
	  frame_size = 4;
	  portc->convert_3byte = 1;
	  break;
	case 4:
	  fmt = AFMT_S32_LE;
	  break;
	}
    }

  portc->bytes_per_sample = frame_size;
  adev->oformat_mask = adev->iformat_mask = fmt;
  UDB (cmn_err (CE_CONT, "Format mask %08x\n", fmt));

  adev->caps &= ~PCM_CAP_FREERATE;
  n = d[7];
  if (n < 1)			/* Free rate selection between min (0) and max (1) */
    {
      n = 2;
      adev->caps |= PCM_CAP_FREERATE;
    }

  min_rate = 0x7fffffff;
  max_rate = 0;

  if (n > 20)
    {
      cmn_err (CE_WARN, "The device supports too many sample rates\n");
      n = 20;
    }

  adev->nrates = 0;

  for (i = 0; i < n; i++)
    {
      int rate = ossusb_get_int (&d[8 + i * 3], 3);

#if 0
      /* Skip rates that are not multiples of 1000 Hz */
      if (rate % 1000)
	continue;
#endif

      if (rate < min_rate)
	min_rate = rate;
      if (rate > max_rate)
	max_rate = rate;
      adev->rates[adev->nrates++] = rate;
    }

  adev->min_rate = min_rate;
  adev->max_rate = max_rate;
  UDB (cmn_err (CE_CONT, "Min rate %d, max rate %d\n", min_rate, max_rate));

  adev->caps &= ~DSP_CH_MASK;

  switch (channels)
    {
    case 1:
      adev->caps |= DSP_CH_MONO;
      break;
    case 2:
      adev->caps |= DSP_CH_STEREO;
      break;
    default:
      adev->caps |= DSP_CH_MULTI;
      break;
    }
  return 0;
}

/*ARGSUSED*/
static int
setup_format_II (ossusb_devc * devc, ossusb_portc * portc, adev_p adev,
		 unsigned char *d, int l)
{
  int min_rate = 0, max_rate = 0;
  int i;

  if (usb_trace > 1)
    {
      cmn_err (CE_CONT, "MaxBitRate %d ", d[4]);
      cmn_err (CE_CONT, "SamplesPerFrame %d ", d[5]);

      if (d[8] == 0)
	cmn_err (CE_CONT, "Sample Rates %d-%d ", ossusb_get_int (&d[9], 3),
		 ossusb_get_int (&d[12], 3));
      else
	{
	  int n;
	  int min_rate = 0x7fffffff;
	  int max_rate = 0;

	  n = d[8];

	  if (n > 20)
	    {
	      cmn_err (CE_CONT, "oss usbaudio: Too many sample rates (%d)\n",
		       n);
	      n = 20;
	    }

	  adev->nrates = 0;
	  cmn_err (CE_CONT, "Possible sample rates: ");
	  for (i = 0; i < d[8]; i++)
	    {
	      int rate = ossusb_get_int (&d[9 + i * 3], 3);

#if 0
	      /* Skip rates that are not multiples of 1000 Hz */
	      if (rate % 1000)
		continue;
#endif

	      if (rate < min_rate)
		min_rate = rate;
	      if (rate > max_rate)
		max_rate = rate;
	      cmn_err (CE_CONT, "%d ", rate);
	      adev->rates[adev->nrates++] = rate;
	    }
	  adev->min_rate = min_rate;
	  adev->max_rate = max_rate;
	}

      cmn_err (CE_CONT, "\n");
    }

  adev->caps &= ~PCM_CAP_FREERATE;
  if (d[8] == 0)
    {
      min_rate = ossusb_get_int (&d[9], 3);
      max_rate = ossusb_get_int (&d[12], 3);
      adev->caps |= PCM_CAP_FREERATE;
    }
  else
    {
      min_rate = 1 << 30;
      max_rate = 0;

      for (i = 0; i < d[8]; i++)
	{
	  int r = ossusb_get_int (&d[9 + i * 3], 3);

	  if (r < min_rate)
	    min_rate = r;
	  if (r > max_rate)
	    max_rate = r;
	}
    }

  adev->min_channels = adev->max_channels = 2;
  adev->oformat_mask = adev->iformat_mask = AFMT_AC3;
  adev->min_rate = min_rate;
  adev->max_rate = max_rate;

  return 0;
}

/*ARGSUSED*/
static int
setup_format_specific (ossusb_devc * devc, ossusb_portc * portc, adev_p adev,
		       unsigned char *d, int l)
{
  int fmt;

  fmt = ossusb_get_int (&d[3], 2);

  if (usb_trace > 1)
    cmn_err (CE_CONT, "Format specific: fmt=%04x\n", fmt);

  switch (fmt)
    {
    case FORMAT_II_MPEG:
      if (usb_trace > 1)
	cmn_err (CE_CONT, "MPEG format\n");
      adev->oformat_mask = adev->iformat_mask = AFMT_MPEG;
      break;

    case FORMAT_II_AC3:
      if (usb_trace > 1)
	cmn_err (CE_CONT, "AC3 format\n");
      adev->oformat_mask = adev->iformat_mask = AFMT_AC3;
#if 0
      cmn_err (CE_CONT, "BSID=%08x\n", ossusb_get_int (&d[5], 4));
      cmn_err (CE_CONT, "AC3Features %02x\n", d[9]);
#endif
      break;

    default:
      cmn_err (CE_CONT, "oss usbaudio: Unsupported FORMAT II tag %04x\n",
	       fmt);
      adev->enabled = 0;
      return OSS_ENXIO;
    }
  return 0;
}

static int
prepare_altsetting (ossusb_devc * devc, ossusb_portc * portc, int new_setting)
{
  int desc_len;
  unsigned char *desc, *d;
  int l, p;
  int err;
  adev_p adev;

  adev = audio_engines[portc->audio_dev];
  adev->enabled = 1;
  portc->disabled = 0;

  if (portc->act_setting == new_setting)	/* No need to change */
    return 0;

  if (new_setting < portc->num_settings)
    desc = udi_usbdev_get_altsetting (portc->usbdev, new_setting, &desc_len);
  else
    desc = NULL;

  if (desc == NULL || desc_len < 3)
    {
      cmn_err (CE_CONT,
	       "Audio device %d not available when altsetting=%d\n",
	       audio_engines[portc->audio_dev]->real_dev, new_setting);
      portc->disabled = 1;
      adev->enabled = 0;
      portc->act_setting = new_setting;
      return OSS_ENXIO;
    }

  UDB (cmn_err
       (CE_CONT, "Select active setting %d on interface %d (dsp%d)\n",
	new_setting, portc->if_number, adev->engine_num));
  portc->act_setting = new_setting;

  p = 0;
  while (p < desc_len)
    {
      int i;

      d = desc + p;
      l = *d;

      if (usb_trace > 1)
	{
	  cmn_err (CE_CONT, "Streaming desc: ");
	  for (i = 0; i < l; i++)
	    cmn_err (CE_CONT, "%02x ", d[i]);
	  cmn_err (CE_CONT, "\n");

	}

      if (d[1] != CS_INTERFACE)
	{
	UDB (cmn_err (CE_CONT, "Unknown descriptor type %02x\n", d[1]))}
      else
	switch (d[2])
	  {
	  case AS_GENERAL:
	    portc->terminal_link = d[3];
	    portc->pipeline_delay = d[4];

	    if (usb_trace > 1)
	      {
		cmn_err (CE_CONT, "AS_GENERAL ");
		cmn_err (CE_CONT, "Terminal link %d/%s ", d[3],
			 devc->units[d[3]].name);
		cmn_err (CE_CONT, "Delay %d ", d[4]);
		cmn_err (CE_CONT, "Format tag %02x%02x ", d[5], d[6]);
		cmn_err (CE_CONT, "\n");
	      }
	    break;

	  case AS_FORMAT_TYPE:
	    if (usb_trace > 1)
	      {
		cmn_err (CE_CONT, "AS_FORMAT_TYPE: FORMAT_TYPE_%d: ", d[3]);
	      }

	    switch (d[3])
	      {
	      case 1:		/* FORMAT_TYPE_I */
		if ((err = setup_format_I (devc, portc, adev, d, l)) < 0)
		  return err;
		break;

	      case 2:		/* FORMAT_TYPE_II */
		if ((err = setup_format_II (devc, portc, adev, d, l)) < 0)
		  return err;
		break;

	      default:
		cmn_err (CE_CONT,
			 "\noss usbaudio: Unsupported format type %d\n",
			 d[3]);
		adev->enabled = 0;
		return OSS_ENXIO;
	      }
	    break;

	  case AS_FORMAT_SPECIFIC:
	    if ((err = setup_format_specific (devc, portc, adev, d, l)) < 0)
	      return err;
	    break;

	  default:
	    UDB (cmn_err
		 (CE_CONT, "Unknown descriptor subtype %02x\n", d[2]));
	  }

      p += l;
    }

  desc = udi_usbdev_get_endpoint (portc->usbdev, portc->act_setting, 0, &l);
  if (desc == NULL)
    {
      cmn_err (CE_CONT, "oss usbaudio: Bad endpoint\n");
      return OSS_EIO;
    }

  portc->endpoint_desc = desc;

  desc = udi_usbdev_get_endpoint (portc->usbdev, portc->act_setting, 1, &l);
#if 0
  if (desc != NULL)
    {
      /* TODO: Handle sync endpoints */
      cmn_err (CE_CONT, "Sync Endpoint: ");
      ossusb_dump_desc (desc, l);
    }
#endif

  return 0;
}

static void usbaudio_close (int dev, int mode);

/*ARGSUSED*/
static int
usbaudio_open (int dev, int mode, int open_flags)
{
  ossusb_devc *devc = audio_engines[dev]->devc;
  ossusb_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags, phaddr;
  int err;
  int i;

  if (devc->disabled)
    return OSS_EPIPE;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (portc->open_mode != 0)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }
  portc->open_mode = mode;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  if ((err = prepare_altsetting (devc, portc, portc->act_setting)) < 0)
    {
      portc->open_mode = 0;
      return err;
    }

  {
    int i;

    for (i = 0; i < 2; i++)
      portc->tmp_buf[i] =
	CONTIG_MALLOC (devc->osdev, TMPBUF_SIZE, MEMLIMIT_32BITS, &phaddr, portc->tmpbuf_dma_handle[i]);
  }

  if ((err =
       udi_usbdev_set_interface (portc->usbdev, portc->if_number,
				 portc->act_setting)) < 0)
    {
      cmn_err (CE_NOTE,
	       "oss usbaudio: Failed to set interface mode, error %d - ignored\n",
	       err);
      // portc->open_mode = 0;
      //return err;
    }

  portc->curr_datapipe = 0;

  if ((portc->endpoint_handle =
       udi_open_endpoint (portc->usbdev, portc->endpoint_desc)) == NULL)
    {
      usbaudio_close (dev, mode);
      cmn_err (CE_WARN, "Cannot open audio pipe\n");
      return OSS_ENOMEM;
    }

  for (i = 0; i < 2; i++)
    {
      if ((portc->datapipe[i] =
	   udi_usb_alloc_request (portc->usbdev, portc->endpoint_handle, 1,
				  UDI_USBXFER_ISO_WRITE)) == NULL)
	{
	  usbaudio_close (dev, mode);
	  cmn_err (CE_WARN, "Cannot alloc isoc request\n");
	  return OSS_ENOMEM;
	}
    }

  set_fraglimits (audio_engines[dev], portc);
  return 0;
}

static void usbaudio_reset (int dev);

/*ARGSUSED*/
static void
usbaudio_close (int dev, int mode)
{
  ossusb_devc *devc = audio_engines[dev]->devc;
  ossusb_portc *portc = audio_engines[dev]->portc;
  int i;

  usbaudio_reset (dev);

  for (i = 0; i < 2; i++)
    {
      if (portc->datapipe[i] != NULL)
	{
	  udi_usb_free_request (portc->datapipe[i]);
	  portc->datapipe[i] = NULL;
	}

      if (portc->tmp_buf[i] != NULL)
	{
	  CONTIG_FREE (devc->osdev, portc->tmp_buf[i], TMPBUF_SIZE, portc->tmpbuf_dma_handle[i]);
	  portc->tmp_buf[i] = NULL;
	}
    }

  if (portc->endpoint_handle != NULL)
    udi_close_endpoint (portc->endpoint_handle);

  udi_usbdev_set_interface (portc->usbdev, portc->if_number, 0);
  portc->open_mode = 0;
}

/*ARGSUSED*/
static void
usbaudio_output_block (int dev, oss_native_word buf, int count,
		       int fragsize, int intrflag)
{
  ossusb_devc *devc = audio_engines[dev]->devc;
  if (devc->disabled)
    return;
}

/*ARGSUSED*/
static void
usbaudio_start_input (int dev, oss_native_word buf, int count, int fragsize,
		      int intrflag)
{
  ossusb_devc *devc = audio_engines[dev]->devc;
  if (devc->disabled)
    return;
}

static int feed_output (int dev, ossusb_devc * devc, ossusb_portc * portc);
static void start_input (int dev, ossusb_devc * devc, ossusb_portc * portc);

static int
copy_input (ossusb_portc * portc, dmap_t * dmap, unsigned char *buf, int len)
{
  int outlen = 0;
  int offs;

  offs = (int) (dmap->byte_counter % dmap->bytes_in_use);

  while (len > 0)
    {
      int l;
      l = len;

      if (portc->convert_3byte)
	{
	  int i, n;
	  int *dmabuf;

	  l = (l * 4) / 3;
	  /* Check for buffer wraparound */
	  if (offs + l > dmap->bytes_in_use)
	    l = dmap->bytes_in_use - offs;

	  n = (l / 4);

	  if (dmap == NULL || dmap->dmabuf == NULL)
	    return outlen;

	  dmabuf = (int *) (dmap->dmabuf + offs);

	  for (i = 0; i < n; i++)
	    {
	      int v;

	      v = buf[2] | (buf[1] << 8) | (buf[0] << 16);
	      *dmabuf++ = v;
	      buf += 3;
	    }

	  outlen += l;
	}
      else
	{
	  unsigned char *dmabuf;
	  dmabuf = dmap->dmabuf;

	  if (dmap == NULL || dmap->dmabuf == NULL)
	    return outlen;

	  /* Check for buffer wraparound */
	  if (offs + l > dmap->bytes_in_use)
	    l = dmap->bytes_in_use - offs;

	  memcpy (dmabuf + offs, buf, l);
	  outlen += l;
	  buf += l;
	}

      len -= l;
      offs = 0;
    }

  return outlen;
}

/*ARGSUSED*/
static void
play_callback (udi_usb_request_t * request, void *arg)
{
  ossusb_portc *portc = arg;

  feed_output (portc->audio_dev, portc->devc, portc);
  oss_audio_outputintr (portc->audio_dev,
			AINTR_NO_POINTER_UPDATES | AINTR_LOCALQUEUE);
}

static void
rec_callback (udi_usb_request_t * request, void *arg)
{
  ossusb_portc *portc = arg;
  dmap_t *dmap;
  int len;

  if (portc == NULL || (unsigned long) arg < 4096)
    {
      cmn_err (CE_WARN, "Bad portc\n");
      return;
    }

  dmap = audio_engines[portc->audio_dev]->dmap_in;
  len = udi_usb_request_actlen (request);

  if (len == 0)
    return;			/* No data so it looks like we are closing down */

  if ((len =
       copy_input (portc, dmap, udi_usb_request_actdata (request), len)) < 1)
    {
      cmn_err (CE_WARN, "Saving recorded data failed (%d)\n", len);
      return;
    }

  oss_audio_inc_byte_counter (dmap, len);
  oss_audio_inputintr (portc->audio_dev, AINTR_NO_POINTER_UPDATES);
#ifdef linux
  start_input (portc->audio_dev, portc->devc, portc);
#endif
}

#if 0
/*
 * Testing stuff only
 */
static int
sin_gen (void)
{

  static int phase = 0, v;

  static short sinebuf[48] = {
    0, 4276, 8480, 12539, 16383, 19947, 23169, 25995,
    28377, 30272, 31650, 32486, 32767, 32486, 31650, 30272,
    28377, 25995, 23169, 19947, 16383, 12539, 8480, 4276,
    0, -4276, -8480, -12539, -16383, -19947, -23169, -25995,
    -28377, -30272, -31650, -32486, -32767, -32486, -31650, -30272,
    -28377, -25995, -23169, -19947, -16383, -12539, -8480, -4276
  };
  v = sinebuf[phase] * 256;
  phase = (phase + 1) % 48;

  return v;
}
#endif

/*ARGSUSED*/
static int
output_convert (ossusb_devc * devc, ossusb_portc * portc, dmap_p dmap,
		unsigned char *dmabuf, int pos, int pn, int total_len)
{
  unsigned char *tmpbuf = portc->tmp_buf[pn], *b;
  int i, n, len, out_size = 0;
  int err;

  while (total_len > 0)
    {
      int l = total_len;

      /* Check for buffer wraparound */
      if (pos + l > dmap->bytes_in_use)
	l = dmap->bytes_in_use - pos;

      total_len -= l;

      b = dmabuf + pos;

      if (portc->convert_3byte)
	{
	  int *buf;

	  n = l / sizeof (*buf);
	  buf = (int *) b;

	  len = n * 3;

	  for (i = 0; i < n; i++)
	    {
	      int val = (*buf++);

	      val /= 256;
	      // val=sin_gen();
	      *tmpbuf++ = (val) & 0xff;
	      *tmpbuf++ = (val >> 8) & 0xff;
	      *tmpbuf++ = (val >> 16) & 0xff;
	    }
	}
      else
	{
	  len = l;
	  memcpy (tmpbuf, b, l);
	  tmpbuf += l;
	}

      pos = 0;
      out_size += len;
    }

  if ((err =
       udi_usb_submit_request (portc->datapipe[pn], play_callback, portc,
			       portc->endpoint_handle, UDI_USBXFER_ISO_WRITE,
			       portc->tmp_buf[pn], out_size)) < 0)
    {
      //cmn_err(CE_CONT, "oss usbaudio: Write transfer eror %d\n", err);
      return err;
    }

  return 0;
}

static int
feed_output (int dev, ossusb_devc * devc, ossusb_portc * portc)
{
  int pn, pos, len;
  oss_native_word flags;
  adev_p adev = audio_engines[dev];
  dmap_p dmap = adev->dmap_out;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (portc->stopping)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return 0;
    }

  pn = portc->curr_datapipe;
  portc->curr_datapipe = (portc->curr_datapipe + 1) % NR_DATAPIPES;

  pos = (int) (dmap->byte_counter % dmap->bytes_in_use);
  len = portc->fragment_size;

  portc->overflow_samples += portc->overflow_size;
  if (portc->overflow_samples > 1000)
    {
      len += dmap->frame_size * (portc->overflow_samples / 1000);
      portc->overflow_samples = portc->overflow_samples % 1000;
    }

  output_convert (devc, portc, dmap, dmap->dmabuf, pos, pn, len);
  oss_audio_inc_byte_counter (dmap, len);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

static void
start_input (int dev, ossusb_devc * devc, ossusb_portc * portc)
{
  int frag, err;
  oss_native_word flags;
  adev_p adev = audio_engines[dev];
  dmap_p dmap = adev->dmap_in;

  if (portc->stopping)
    return;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  frag = 0;

  if ((err =
       udi_usb_submit_request (portc->datapipe[0], rec_callback, portc,
			       portc->endpoint_handle, UDI_USBXFER_ISO_READ,
			       dmap->dmabuf + frag * portc->fragment_size,
			       portc->fragment_size)) < 0)
    {
      cmn_err (CE_WARN, "oss usbaudio: Read transfer error %d\n", err);
      cmn_err (CE_CONT, "Endpoint %02x\n",
	       udi_endpoint_get_num (portc->endpoint_handle));
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static void
usbaudio_trigger (int dev, int state)
{
  ossusb_devc *devc = audio_engines[dev]->devc;
  ossusb_portc *portc = audio_engines[dev]->portc;

  if (devc->disabled)
    return;

  if (portc->open_mode & PCM_ENABLE_OUTPUT)
    {
      if ((portc->prepared_modes & PCM_ENABLE_OUTPUT)
	  && (state & PCM_ENABLE_OUTPUT))
	{
	  portc->prepared_modes &= ~PCM_ENABLE_OUTPUT;
	  portc->curr_datapipe = 0;
	  portc->stopping = 0;

	  feed_output (dev, devc, portc);
	  feed_output (dev, devc, portc);
	}
      else if (!(state & PCM_ENABLE_OUTPUT))
	{
	  portc->stopping = 1;
#if 1
	  udi_usb_cancel_request (portc->datapipe[0]);
	  udi_usb_cancel_request (portc->datapipe[1]);
#endif
	  portc->curr_datapipe = 0;
	}
    }

  if (portc->open_mode & PCM_ENABLE_INPUT)
    {
      if ((portc->prepared_modes & PCM_ENABLE_INPUT)
	  && (state & PCM_ENABLE_INPUT))
	{
	  portc->prepared_modes &= ~PCM_ENABLE_INPUT;
	  portc->stopping = 0;
	  start_input (dev, devc, portc);
	}
      else if (!(state & PCM_ENABLE_INPUT))
	{
	  portc->stopping = 1;
#if 0
	  udi_usb_cancel_request (portc->datapipe[0]);
	  udi_usb_cancel_request (portc->datapipe[1]);
#endif
	  portc->curr_datapipe = 0;
	}
    }
}

static void
usbaudio_reset (int dev)
{
  usbaudio_trigger (dev, 0);
}

/*ARGSUSED*/
static int
usbaudio_prepare_for_input (int dev, int bsize, int bcount)
{
  ossusb_devc *devc = audio_engines[dev]->devc;
  ossusb_portc *portc = audio_engines[dev]->portc;
  adev_p adev = audio_engines[dev];

  if (devc->disabled)
    return OSS_EPIPE;


  if (adev->flags & ADEV_NOINPUT)
    return OSS_ENOTSUP;

  portc->stopping = 0;

  if (write_control_value
      (devc, portc->endpoint_handle, SAMPLING_FREQ_CONTROL, 3,
       portc->speed) < 0)
    {
      cmn_err (CE_CONT, "Failed to set %d Hz sampling rate\n", portc->speed);
      return OSS_EIO;
    }

  /*
   * Handle fractional samples that don't fit in the 1ms period.
   */
  portc->overflow_size = portc->speed % 1000;
  portc->overflow_samples = 0;

  portc->prepared_modes |= PCM_ENABLE_INPUT;
  return 0;
}

/*ARGSUSED*/
static int
usbaudio_prepare_for_output (int dev, int bsize, int bcount)
{
  ossusb_devc *devc = audio_engines[dev]->devc;
  ossusb_portc *portc = audio_engines[dev]->portc;
  adev_p adev = audio_engines[dev];

  if (devc->disabled)
    return OSS_EPIPE;

  if (adev->flags & ADEV_NOOUTPUT)
    return OSS_ENOTSUP;

  portc->stopping = 0;

  if (write_control_value
      (devc, portc->endpoint_handle, SAMPLING_FREQ_CONTROL, 3,
       portc->speed) < 0)
    {
      cmn_err (CE_CONT, "Failed to set %d Hz sampling rate\n", portc->speed);
      return OSS_EIO;
    }

  /*
   * Handle fractional samples that don't fit in the 1ms period.
   */
  portc->overflow_size = portc->speed % 1000;
  portc->overflow_samples = 0;

  portc->prepared_modes |= PCM_ENABLE_OUTPUT;
  return 0;
}

static int
usbaudio_check_input (int dev)
{
  ossusb_devc *devc = audio_engines[dev]->devc;
  if (devc->disabled)
    {
      cmn_err (CE_CONT,
	       "oss usbaudio: Audio device %d removed from the system.\n",
	       dev);
      return OSS_EPIPE;
    }

  cmn_err (CE_CONT, "oss usbaudio: Audio input timed out on device %d.\n",
	   dev);
  return OSS_EIO;
}

static int
usbaudio_check_output (int dev)
{
  ossusb_devc *devc = audio_engines[dev]->devc;
  if (devc->disabled)
    {
      cmn_err (CE_CONT,
	       "oss usbaudio: Audio device %d removed from the system.\n",
	       dev);
      return OSS_EPIPE;
    }

  cmn_err (CE_CONT, "oss usbaudio: Audio output timed out on device %d.\n",
	   dev);
  return OSS_EIO;
}

static int
usbaudio_local_qlen (int dev)
{
  ossusb_portc *portc = audio_engines[dev]->portc;
  dmap_p dmap = audio_engines[dev]->dmap_out;
  int delay = portc->pipeline_delay + 1;	/* Pipeline delay in 1 msec ticks */

  delay = delay * dmap->data_rate / 1000;	/* Bytes/msec */

  return delay;
}

static audiodrv_t usbaudio_driver = {
  usbaudio_open,
  usbaudio_close,
  usbaudio_output_block,
  usbaudio_start_input,
  usbaudio_ioctl,
  usbaudio_prepare_for_input,
  usbaudio_prepare_for_output,
  usbaudio_reset,
  usbaudio_local_qlen,
  NULL,
  NULL,
  NULL,
  usbaudio_trigger,
  usbaudio_set_rate,
  usbaudio_set_format,
  usbaudio_set_channels,
  NULL,
  NULL,
  usbaudio_check_input,
  usbaudio_check_output,
  NULL,				/* usbaudio_alloc_buffer */
  NULL,				/* usbaudio_free_buffer */
  NULL,
  NULL,
  NULL				/* usbaudio_get_buffer_pointer */
};

ossusb_devc *
ossusb_init_audiostream (ossusb_devc * devc, udi_usb_devc * usbdev, int inum,
			 int reinit)
{
  int nsettings, actsetting = 0, desc_len;
  unsigned char *desc, *d;
  adev_p adev;
  int i, p, l;
  int portc_num;
  void *endpoint_desc;

  char dev_name[128];

  int opts = ADEV_AUTOMODE;

  ossusb_portc *portc;

  if (devc->num_audio_engines >= MAX_PORTC)
    {
      cmn_err (CE_CONT, "usbaudio: Too many audio streaming interfaces\n");
      return devc;
    }

  if (usbdev == NULL)
    {
      cmn_err (CE_CONT, "usbaudio: usbdev==NULL\n");
      return devc;
    }

  nsettings = udi_usbdev_get_num_altsettings (usbdev);
  desc = udi_usbdev_get_endpoint (usbdev, 1, 0, &l);
  if (desc != NULL)
    {
      /* cmn_err(CE_CONT, "Endpoint: ");ossusb_dump_desc(desc, l); */
      endpoint_desc = desc;
    }
  else
    endpoint_desc = NULL;

  if (reinit)
    for (i = 0; i < devc->num_audio_engines; i++)
      if (devc->portc[i].orig_endpoint_desc == endpoint_desc)	/* Already registered this */
	{
	  prepare_altsetting (devc, &devc->portc[i],
			      devc->portc[i].act_setting);
	  return devc;
	}

  portc = &devc->portc[devc->num_audio_engines];
  portc_num = devc->num_audio_engines;
  portc->if_number = inum;
  portc->endpoint_desc = portc->orig_endpoint_desc = endpoint_desc;
  portc->usbdev = usbdev;
  portc->act_setting = -1;	/* Set to an impossible value */
  devc->num_audio_engines++;

  memset (dev_name, 0, sizeof (dev_name));
  strncpy (dev_name, devc->dev_name, sizeof (dev_name) - 1);
  portc->num_settings = nsettings;

#if 1
  if (usb_trace > 2)
    for (i = 0; i < nsettings; i++)
      {
	desc = udi_usbdev_get_altsetting (usbdev, i, &desc_len);
	cmn_err (CE_CONT, "\nDumping altsetting %d (l=%d)\n", i, desc_len);
	if (usb_trace)
	  ossusb_dump_desc (desc, desc_len);
      }
#endif

  desc = udi_usbdev_get_altsetting (usbdev, actsetting, &desc_len);

  if (desc == NULL || desc_len < 1)
    for (i = 0; i < nsettings && (desc == NULL || desc_len < 1); i++)
      {
	UDB (cmn_err (CE_CONT, "Trying to read altsetting %d\n", i));
	desc = udi_usbdev_get_altsetting (usbdev, i, &desc_len);
	if (desc != NULL)
	  actsetting = i;
      }

  UDB (cmn_err (CE_CONT, "Altsetting %d, len %d\n", actsetting, desc_len));

  if (desc == NULL)
    {
      cmn_err (CE_WARN, "Can't read interface descriptors\n");
      return NULL;
    }

  if (usb_trace > 2)
    ossusb_dump_desc (desc, desc_len);

  p = 0;
  while (p < desc_len)
    {
      d = desc + p;
      l = *d;

      if (d[1] != CS_INTERFACE)
	{
	UDB (cmn_err (CE_CONT, "Unknown descriptor type %02x\n", d[1]))}
      else
	switch (d[2])
	  {
	  case AS_GENERAL:
	    portc->terminal_link = d[3];
	    break;
	  }

      p += l;
    }

  if (portc->terminal_link > 0 && portc->terminal_link <= devc->nunits)
    {
      char *s = dev_name + strlen (dev_name);

      sprintf (s, " %s", devc->units[portc->terminal_link].name);
      s += strlen (s);

      if (devc->units[portc->terminal_link].typ == TY_OUTPUT)	/* USB terminal type */
	{
	  opts |= ADEV_NOOUTPUT;
	}
      else
	{
	  opts |= ADEV_NOINPUT;
	}
    }

  if ((portc->audio_dev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
						devc->osdev,
						devc->osdev,
						dev_name,
						&usbaudio_driver,
						sizeof (audiodrv_t),
						opts,
						AFMT_S16_NE, devc, -1)) < 0)
    {
      return devc;
    }

  portc->devc = devc;
  adev = audio_engines[portc->audio_dev];
  adev->portc = portc;
  adev->mixer_dev = devc->mixer_dev;
  adev->rate_source = devc->portc[0].audio_dev;
  adev->max_block = 256;
  adev->min_fragments = 4;	/* vmix needs this */

  prepare_altsetting (devc, portc, 1);

  if (portc->num_settings > 2)
    {
      char name[128];

      sprintf (name, "%s-altset", devc->units[portc->terminal_link].name);
      ossusb_create_altset_control (devc->mixer_dev, portc_num,
				    portc->num_settings, name);
    }

#if 0
  // TODO: This needs to be checked before vmix is enabled
#ifdef CONFIG_OSS_VMIX
  if (devc->units[portc->terminal_link].typ != TY_OUTPUT)
     vmix_attach_audiodev(devc->osdev, portc->audio_dev, -1, 0);
#endif
#endif
  return devc;
}

/*ARGSUSED*/
int
ossusb_change_altsetting (int dev, int ctrl, unsigned int cmd, int value)
{
  ossusb_devc *devc = mixer_devs[dev]->devc;
  ossusb_portc *portc;

  if (ctrl < 0 || ctrl >= devc->num_audio_engines)
    return OSS_ENXIO;

  portc = &devc->portc[ctrl];

  if (cmd == SNDCTL_MIX_READ)
    return portc->act_setting;

  if (value >= portc->num_settings)
    value = portc->num_settings - 1;

  if (portc->act_setting != value)
    {
      prepare_altsetting (devc, portc, value);
    }
  return value;
}
