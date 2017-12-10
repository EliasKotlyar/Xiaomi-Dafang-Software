/*
 * sound/lynxone.c
 *
 * Driver for LynxONE Studio Interface.
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

#include "lynxone_cfg.h"
#include "oss_pci.h"

#include "Environ.h"
#include "HalLynx.h"
#include "HalAudio.h"
#include "HalDwnld.h"
#include "DosCmn.h"
#include "DrvDebug.h"
#include "lynx_hal.h"


#define PLX_VENDOR_ID		0x10b5
#define PLX_LYNXONE		0x1142


USHORT EEPromReadWord (PADAPTER pA, USHORT usAddress);
USHORT HalOpenAdapter (lynx_devc * devc);
ULONG GetDigitalInStatus (PADAPTER pA);

PADAPTER
DrvGetAdapter (PVOID pDriverContext, int nAdapter)
/*/////////////////////////////////////////////////////////////////////////*/
{
  lynx_devc *devc = (lynx_devc *) pDriverContext;
  return (&devc->adapter);
}


static void
handle_play_interrupt (lynx_devc * devc, int ch)
{
  ULONG ulBytesRequested, ulCircularBufferSize;
  PADAPTER pA;
  PDEVICE pD;
  int dev;

  lynx_portc *portc;
  dmap_t *dmap;

  portc = &devc->portc[ch];
  dev = portc->audiodev;

  dmap = audio_engines[dev]->dmap_out;
  pA = &devc->adapter;
  pD = &pA->Device[portc->hw_device];

  /* Get the amount of free space on device */
  HalDeviceGetTransferSize (pD, &ulBytesRequested, &ulCircularBufferSize);
  if (portc->local_qlen > 0)
    portc->local_qlen--;

  if (devc->clock_source == 1)	/* Using external sync */
    if ((GetDigitalInStatus (pA) & MIXVAL_DIERR_MASK) == MIXVAL_DIERR_NOLOCK)
      {
	cmn_err (CE_WARN, "LynxONE: External sync dropped.\n");
	return;
      }

  if (ulBytesRequested >= dmap->fragment_size)
    {
      oss_audio_outputintr (dev, 1);
    }
}

static void
handle_record_interrupt (lynx_devc * devc, int ch)
{
  ULONG ulBytesTransfered, ulBytesRequested, ulCircularBufferSize, n, l;
  PADAPTER pA;
  PDEVICE pD;
  int dev;
  unsigned char *p;

  lynx_portc *portc;
  dmap_t *dmap;

  portc = &devc->portc[ch];
  dev = portc->audiodev;

  dmap = audio_engines[dev]->dmap_in;
  pA = &devc->adapter;
  pD = &pA->Device[portc->hw_device];

  /* Get the amount of available data on device */
  HalDeviceGetTransferSize (pD, &ulBytesRequested, &ulCircularBufferSize);
  n = 0;
  while (n++ < dmap->nfrags && ulBytesRequested >= dmap->fragment_size)
    {

      l = ulBytesRequested;
      if (l > dmap->fragment_size)
	l = dmap->fragment_size;

      p = dmap->dmabuf + dmap_get_qtail (dmap) * dmap->fragment_size;
      HalDeviceTransferAudio (pD, p, l, &ulBytesTransfered);
      HalDeviceTransferComplete (pD, ulBytesTransfered);

      HalDeviceGetTransferSize (pD, &ulBytesRequested, &ulCircularBufferSize);

      if (devc->clock_source == 1)	/* Using external sync */
	{
	  static int n = 0;

	  if (!(n++ & 0xf))
	    if ((GetDigitalInStatus (pA) & MIXVAL_DIERR_MASK) ==
		MIXVAL_DIERR_NOLOCK)
	      {
		cmn_err (CE_WARN, "LynxONE: External sync dropped.\n");
		return;
	      }
	}

      oss_audio_inputintr (portc->audiodev, 0);
    }

}

static void
handle_midi_record_interrupt (lynx_devc * devc, int ch)
{
  lynx_midic *midic = &devc->midic[ch];
  PADAPTER pA;
  PMIDI pREC;
  unsigned char buf[32];
  ULONG n, i;

  if (!midic->midi_opened)
    return;

  pA = &devc->adapter;
  pREC = &pA->Midi[midic->hw_indev];
  HalMidiRead (pREC, buf, sizeof (buf), &n);

  for (i = 0; i < n; i++)
    {
      midic->midi_input_intr (midic->midi_dev, buf[i]);
    }
}

static int
lynxintr (oss_device_t * osdev)
{
  lynx_devc *devc = (lynx_devc *) osdev->devc;
  PADAPTER pA;
  ULONG ulDeviceInterrupt;
  int serviced = 0;


  pA = &devc->adapter;

  HalFindInterrupt (pA, &ulDeviceInterrupt);
  if (!ulDeviceInterrupt)
    return 0;

  serviced = 1;

  if (ulDeviceInterrupt & WAVE_PLAY0_INTERRUPT)
    handle_play_interrupt (devc, 0);

  if (ulDeviceInterrupt & WAVE_PLAY1_INTERRUPT)
    handle_play_interrupt (devc, 2);

  if (ulDeviceInterrupt & WAVE_RECORD0_INTERRUPT)
    handle_record_interrupt (devc, 1);

  if (ulDeviceInterrupt & WAVE_RECORD1_INTERRUPT)
    handle_record_interrupt (devc, 3);

  if (ulDeviceInterrupt & MIDI_RECORD0_INTERRUPT)
    handle_midi_record_interrupt (devc, 0);

  if (ulDeviceInterrupt & MIDI_RECORD1_INTERRUPT)
    handle_midi_record_interrupt (devc, 1);

  return serviced;
}

/***********************************
 * Audio routines 
 ***********************************/

static int
lynx_set_rate (int dev, int arg)
{
  lynx_devc *devc = audio_engines[dev]->devc;

  if (arg == 0)
    return devc->speed;

  if (devc->qsrc)
    audio_engines[dev]->flags |= ADEV_FIXEDRATE;
  else
    audio_engines[dev]->flags &= ~ADEV_FIXEDRATE;

  if (devc->ratelock)
    {
      return devc->locked_speed;
    }

  if (devc->rate_fixed || dev != devc->master_dev)
    {
      return audio_engines[dev]->fixed_rate;
    }

  if (arg < 8000)
    arg = 8000;
  if (arg > 96000)
    arg = 96000;
  devc->speed = arg;
  if (devc->qsrc)
    {
      if (arg == 8000)
	audio_engines[dev]->fixed_rate = devc->locked_speed;
      else
	audio_engines[dev]->fixed_rate = arg;
    }

  return audio_engines[dev]->fixed_rate = devc->speed;
}

static short
lynx_set_channels (int dev, short arg)
{
  lynx_portc *portc = audio_engines[dev]->portc;
  if (arg == 0)
    return portc->channels;

#if 1
  /* TODO: Check why mono doesn't work */
  arg = 2;
#else
  if (arg != 1 && arg != 2)
    arg = 2;
#endif

  return portc->channels = arg;
}

static unsigned int
lynx_set_format (int dev, unsigned int arg)
{
  lynx_portc *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->bits;

  switch (arg)
    {
    case AFMT_S16_NE:
      portc->bits = arg;
      portc->samplewidth = 16;
      break;

    case AFMT_S32_NE:
      portc->bits = arg;
      portc->samplewidth = 32;
      break;
    }
  return portc->bits;
}

static int
lynx_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static void lynx_trigger (int dev, int state);

static void
lynx_reset (int dev)
{
  lynx_trigger (dev, 0);
}

static int
lynx_open (int dev, int mode, int open_flags)
{
  lynx_devc *devc = audio_engines[dev]->devc;
  lynx_portc *portc = audio_engines[dev]->portc;
  oss_native_word flags;

  PADAPTER pA;
  PDEVICE pD;

  pA = &devc->adapter;
  pD = &pA->Device[portc->hw_device];

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (portc->open_mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  portc->open_mode = mode;
  portc->device_prepared = portc->device_triggered = 0;
  devc->open_devices++;

  if (devc->open_devices == 1)
    {

      devc->master_dev = dev;
      devc->rate_fixed = 0;
      audio_engines[dev]->flags &= ~ADEV_FIXEDRATE;
    }
  else
    {
      audio_engines[dev]->flags |= ADEV_FIXEDRATE;
      audio_engines[dev]->fixed_rate = devc->speed;
    }
  if (devc->clock_source == 1)	/* Using external sync */
    {
      if ((GetDigitalInStatus (pA) & MIXVAL_DIERR_MASK) ==
	  MIXVAL_DIERR_NOLOCK)
	{
	  cmn_err (CE_WARN, "LynxONE: No digital input signal detected\n");
	  devc->open_devices = 0;
	  portc->open_mode = 0;
	  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

	  return OSS_EIO;
	}
    }

  if (portc->hw_device == WAVE_RECORD0_DEVICE
      || portc->hw_device == WAVE_RECORD1_DEVICE)
    {
      HalDeviceSetMode (pD, MODE_RECORD_OPEN);
    }

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

static void
lynx_close (int dev, int mode)
{
  lynx_portc *portc = audio_engines[dev]->portc;
  lynx_devc *devc = audio_engines[dev]->devc;
  PADAPTER pA;
  PDEVICE pD;

  pA = &devc->adapter;
  pD = &pA->Device[portc->hw_device];

  if (portc->hw_device == WAVE_RECORD0_DEVICE
      || portc->hw_device == WAVE_RECORD1_DEVICE)
    {
      HalDeviceSetMode (pD, MODE_RECORD_CLOSE);
    }
  devc->open_devices--;
  portc->open_mode = 0;
}

static void
lynx_output_block (int dev, oss_native_word ptr, int count, int fragsize,
		   int intrflag)
{
  lynx_portc *portc = audio_engines[dev]->portc;
  lynx_devc *devc = audio_engines[dev]->devc;
  ULONG ulCircularBufferSize, ulBytesRequested, ulBytesTransfered;
  PADAPTER pA;
  PDEVICE pD;
  int p;
  dmap_t *dmap = audio_engines[dev]->dmap_out;

  p = ptr - dmap->dmabuf_phys;

  pA = &devc->adapter;
  pD = &pA->Device[portc->hw_device];

  /* Get the amount of free space on device */
  HalDeviceGetTransferSize (pD, &ulBytesRequested, &ulCircularBufferSize);

  if (count > ulBytesRequested)
    {
      /* cmn_err (CE_WARN, "LynxONE: count > ulBytesRequested (%d/%d)\n", count, */
      /*      ulBytesRequested); */
      return;
    }

  HalDeviceTransferAudio (pD, dmap->dmabuf + p, count, &ulBytesTransfered);
  HalDeviceTransferComplete (pD, ulBytesTransfered);

  if (portc->local_qlen < (8188 / dmap->fragment_size))
    {
      portc->local_qlen++;

      HalDeviceGetTransferSize (pD, &ulBytesRequested, &ulCircularBufferSize);
      if (dmap_get_qlen (dmap) > 1 && ulBytesRequested >= dmap->fragment_size)
	oss_audio_outputintr (dev, 1);
    }
}

static void
lynx_start_input (int dev, oss_native_word ptr, int count, int fragsize,
		  int intrflag)
{
}

static void
lynx_trigger (int dev, int state)
{

  lynx_devc *devc = audio_engines[dev]->devc;
  lynx_portc *portc = audio_engines[dev]->portc;
  int changed;
  PADAPTER pA;
  PDEVICE pD;

  changed = portc->device_triggered ^ state;

  pA = &devc->adapter;
  pD = &pA->Device[portc->hw_device];

  if (portc->type == TY_AOUT || portc->type == TY_DOUT)
    if ((portc->open_mode & OPEN_WRITE) && (changed & PCM_ENABLE_OUTPUT))
      {
	if ((state & PCM_ENABLE_OUTPUT)
	    && (portc->device_prepared & PCM_ENABLE_OUTPUT))
	  {
	    HalDeviceSetMode (pD, MODE_PLAY);
	    portc->device_triggered = PCM_ENABLE_OUTPUT;
	    portc->local_qlen = 0;
	  }
	else if (!(state & PCM_ENABLE_OUTPUT)
		 && (portc->device_triggered & PCM_ENABLE_OUTPUT))
	  {
	    portc->device_prepared = portc->device_triggered = 0;
	    HalDeviceSetMode (pD, MODE_IDLE);
	    portc->local_qlen = 65536;
	  }
      }

  if (portc->type == TY_AIN || portc->type == TY_DIN)
    if ((portc->open_mode & OPEN_READ) && (changed & PCM_ENABLE_INPUT))
      {
	if ((state & PCM_ENABLE_INPUT)
	    && (portc->device_prepared & PCM_ENABLE_INPUT))
	  {
	    HalDeviceSetMode (pD, MODE_RECORD);
	    portc->device_triggered = PCM_ENABLE_INPUT;
	    portc->local_qlen = 0;
	  }
	else if (!(state & PCM_ENABLE_INPUT)
		 && (portc->device_triggered & PCM_ENABLE_INPUT))
	  {
	    portc->device_prepared = portc->device_triggered = 0;
	    HalDeviceSetMode (pD, MODE_IDLE);
	    portc->local_qlen = 0;
	  }
      }
}

static void
fix_rate (lynx_devc * devc)
{
  int i, d;
  PADAPTER pA = &devc->adapter;

  if (devc->ratelock && !devc->rate_fixed)
    devc->speed = devc->locked_speed;
  devc->rate_fixed = 1;

  if (devc->qsrc)
    for (i = 0; i < 4; i++)
      {
	d = devc->portc[i].audiodev;

	audio_engines[d]->fixed_rate = devc->speed;
      }

  switch (devc->clock_source)
    {
    case 0:			/* Internal */
      HalMixerSetControl (&pA->Mixer, LINE_ADAPTER, LINE_NO_SOURCE,
			  CONTROL_CLOCKSOURCE, 0, MIXVAL_CLKSRC_INTERNAL);
      break;

    case 1:			/* Digital in */
      HalMixerSetControl (&pA->Mixer, LINE_ADAPTER, LINE_NO_SOURCE,
			  CONTROL_CLOCKSOURCE, 0, MIXVAL_CLKSRC_DIGITAL);
      break;

    case 2:			/* External, Worldclock */
      HalMixerSetControl (&pA->Mixer, LINE_ADAPTER, LINE_NO_SOURCE,
			  CONTROL_CLOCKSOURCE, 0, MIXVAL_CLKSRC_EXTERNAL);
      HalMixerSetControl (&pA->Mixer, LINE_ADAPTER, LINE_NO_SOURCE,
			  CONTROL_CLOCKREFERENCE, 0, MIXVAL_CLKREF_WORD);
      break;

    case 3:			/* External 27 MHz */
      HalMixerSetControl (&pA->Mixer, LINE_ADAPTER, LINE_NO_SOURCE,
			  CONTROL_CLOCKSOURCE, 0, MIXVAL_CLKSRC_EXTERNAL);
      HalMixerSetControl (&pA->Mixer, LINE_ADAPTER, LINE_NO_SOURCE,
			  CONTROL_CLOCKREFERENCE, 0, MIXVAL_CLKREF_27MHZ);
      break;

    case 4:			/* External 13 MHz */
      HalMixerSetControl (&pA->Mixer, LINE_ADAPTER, LINE_NO_SOURCE,
			  CONTROL_CLOCKSOURCE, 0, MIXVAL_CLKSRC_EXTERNAL);
      HalMixerSetControl (&pA->Mixer, LINE_ADAPTER, LINE_NO_SOURCE,
			  CONTROL_CLOCKREFERENCE, 0, MIXVAL_CLKREF_13p5MHZ);
      break;

    case 5:			/* Header, World clock */
      HalMixerSetControl (&pA->Mixer, LINE_ADAPTER, LINE_NO_SOURCE,
			  CONTROL_CLOCKSOURCE, 0, MIXVAL_CLKSRC_HEADER);
      HalMixerSetControl (&pA->Mixer, LINE_ADAPTER, LINE_NO_SOURCE,
			  CONTROL_CLOCKREFERENCE, 0, MIXVAL_CLKREF_WORD);
      break;

    case 6:			/* Header, 27 MHz */
      HalMixerSetControl (&pA->Mixer, LINE_ADAPTER, LINE_NO_SOURCE,
			  CONTROL_CLOCKSOURCE, 0, MIXVAL_CLKSRC_HEADER);
      HalMixerSetControl (&pA->Mixer, LINE_ADAPTER, LINE_NO_SOURCE,
			  CONTROL_CLOCKREFERENCE, 0, MIXVAL_CLKREF_27MHZ);
      break;

    case 7:			/* Header, 13 MHz */
      HalMixerSetControl (&pA->Mixer, LINE_ADAPTER, LINE_NO_SOURCE,
			  CONTROL_CLOCKSOURCE, 0, MIXVAL_CLKSRC_HEADER);
      HalMixerSetControl (&pA->Mixer, LINE_ADAPTER, LINE_NO_SOURCE,
			  CONTROL_CLOCKREFERENCE, 0, MIXVAL_CLKREF_13p5MHZ);
      break;

    default:
      cmn_err (CE_WARN, "LynxONE: Bad clock source %d\n", devc->clock_source);
    }

  switch (devc->digital_format)
    {
    case 0:
      HalMixerSetControl (&pA->Mixer, LINE_ADAPTER, LINE_NO_SOURCE,
			  CONTROL_DIGITALFORMAT, 0, MIXVAL_DF_AESEBU);
      break;

    case 1:
      HalMixerSetControl (&pA->Mixer, LINE_ADAPTER, LINE_NO_SOURCE,
			  CONTROL_DIGITALFORMAT, 0, MIXVAL_DF_SPDIF);
      break;

    default:
      cmn_err (CE_WARN, "LynxONE: Bad digital format %d\n",
	       devc->digital_format);
    }
}

static int
lynx_prepare_for_output (int dev, int bsize, int bcount)
{
  lynx_devc *devc = audio_engines[dev]->devc;
  lynx_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_out;

  PADAPTER pA;
  PDEVICE pD;

  WAVEFORMATEX WaveFormatEx;

  if (portc->type != TY_AOUT && portc->type != TY_DOUT)
    {
      cmn_err (CE_WARN, "LynxONE: Audio device %d cannot do output.\n", dev);
      return OSS_EIO;
    }

  pA = &devc->adapter;
  pD = &pA->Device[portc->hw_device];
  fix_rate (devc);

  WaveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
  WaveFormatEx.wBitsPerSample = portc->samplewidth;
  WaveFormatEx.nChannels = portc->channels;
  WaveFormatEx.nSamplesPerSec = devc->speed;
  WaveFormatEx.cbSize = dmap->fragment_size;
  WaveFormatEx.nAvgBytesPerSec =
    (WaveFormatEx.nSamplesPerSec * WaveFormatEx.nChannels *
     WaveFormatEx.wBitsPerSample) / 8;
  WaveFormatEx.nBlockAlign =
    (WaveFormatEx.wBitsPerSample * WaveFormatEx.nChannels) / 8;

  HalDeviceSetFormat (pD, &WaveFormatEx);

  /* Set play volume L & R */
  HalMixerSetControl (&pA->Mixer, LINE_ANALOG_OUT, LINE_PLAY_0,
		      CONTROL_VOLUME, 0, devc->left);
  HalMixerSetControl (&pA->Mixer, LINE_ANALOG_OUT, LINE_PLAY_0,
		      CONTROL_VOLUME, 1, devc->right);

  /* preload the buffer to the card */
  HalDeviceSetMode (pD, MODE_PRELOAD);
  /* HalDeviceSetInterruptSamples (pD, dmap->fragment_size/4); */

  portc->device_prepared = PCM_ENABLE_OUTPUT;
  switch (devc->trim)
    {
    case 1:			/* Consumer */
      HalMixerSetControl (&pA->Mixer, LINE_ADAPTER, LINE_NO_SOURCE,
			  CONTROL_TRIM, 0, MIXVAL_TRIM_CONSUMER);
      break;
    case 0:			/* Professional */
    default:
      HalMixerSetControl (&pA->Mixer, LINE_ADAPTER, LINE_NO_SOURCE,
			  CONTROL_TRIM, 0, MIXVAL_TRIM_PROFESSIONAL);
      break;
    }
  return 0;
}

static int
lynx_prepare_for_input (int dev, int bsize, int bcount)
{
  lynx_devc *devc = audio_engines[dev]->devc;
  lynx_portc *portc = audio_engines[dev]->portc;
  dmap_t *dmap = audio_engines[dev]->dmap_in;

  PADAPTER pA;
  PDEVICE pD;

  WAVEFORMATEX WaveFormatEx;

  if (portc->type != TY_AIN && portc->type != TY_DIN)
    {
      cmn_err (CE_WARN, "LynxONE: Audio device %d cannot do input.\n", dev);
      return OSS_EIO;
    }

  pA = &devc->adapter;
  pD = &pA->Device[portc->hw_device];

  fix_rate (devc);
  WaveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
  WaveFormatEx.wBitsPerSample = portc->samplewidth;
  WaveFormatEx.nChannels = portc->channels;
  WaveFormatEx.nSamplesPerSec = devc->speed;
  WaveFormatEx.cbSize = dmap->fragment_size;
  WaveFormatEx.nAvgBytesPerSec =
    (WaveFormatEx.nSamplesPerSec * WaveFormatEx.nChannels *
     WaveFormatEx.wBitsPerSample) / 8;
  WaveFormatEx.nBlockAlign =
    (WaveFormatEx.wBitsPerSample * WaveFormatEx.nChannels) / 8;

  HalDeviceSetFormat (pD, &WaveFormatEx);

  portc->device_prepared = PCM_ENABLE_INPUT;
  return 0;
}

static int
lynx_local_qlen (int dev)
{
  int count;

  lynx_portc *portc = audio_engines[dev]->portc;

  if (portc->device_triggered == 0)
    return 0;

  count = portc->local_qlen;
  if (count < 0)
    count = 0;
  return count;
}

#if 0
static int
lynx_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
}
#endif

static audiodrv_t lynx_driver = {
  lynx_open,
  lynx_close,
  lynx_output_block,
  lynx_start_input,
  lynx_ioctl,
  lynx_prepare_for_input,
  lynx_prepare_for_output,
  lynx_reset,
  lynx_local_qlen,
  NULL,
  NULL,
  NULL,
  lynx_trigger,
  lynx_set_rate,
  lynx_set_format,
  lynx_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,				/* lynx_alloc_buffer */
  NULL,				/* lynx_free_buffer */
  NULL,
  NULL,
  NULL				/* lynx_get_buffer_pointer */
};

static int
install_audiodev (lynx_devc * devc, char *name, int type)
{
  int adev, flags;
  lynx_portc *portc;

  flags = ADEV_16BITONLY | ADEV_NOVIRTUAL | ADEV_DISABLE_VIRTUAL;
  portc = &devc->portc[devc->nrdevs++];

  portc->devc = devc;

  devc->clock_source = 0;	/* Internal */
  devc->speed = devc->locked_speed = 48000;
  devc->ratelock = 1;

  switch (type)
    {
    case TY_AOUT:
      flags |= ADEV_NOINPUT;
      portc->hw_device = WAVE_PLAY0_DEVICE;
      break;

    case TY_AIN:
      flags |= ADEV_NOOUTPUT;
      portc->hw_device = WAVE_RECORD0_DEVICE;
      break;

    case TY_DOUT:
      flags |= ADEV_NOINPUT;
      portc->hw_device = WAVE_PLAY1_DEVICE;
      break;

    case TY_DIN:
      flags |= ADEV_NOOUTPUT;
      portc->hw_device = WAVE_RECORD1_DEVICE;
      break;
    }

  if ((adev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
				    devc->osdev,
				    devc->osdev,
				    name, &lynx_driver,
				    sizeof (audiodrv_t), flags,
				    AFMT_S16_NE | AFMT_S32_NE, devc, -1)) < 0)
    {
      return 0;
    }

  if (devc->first_dev == -1)
    devc->first_dev = adev;
  audio_engines[adev]->mixer_dev = devc->mixer_dev;
  audio_engines[adev]->portc = portc;
  audio_engines[adev]->rate_source = devc->first_dev;
  audio_engines[adev]->max_block = 2048;
  audio_engines[adev]->min_rate = 8000;
  audio_engines[adev]->max_rate = 96000;
  audio_engines[adev]->caps |= PCM_CAP_FREERATE;
  portc->open_mode = 0;

  portc->type = type;
  portc->audiodev = adev;

  return 1;
}

static int
lynx_midi_open (int dev, int mode, oss_midi_inputbyte_t inputbyte,
		oss_midi_inputbuf_t inputbuf,
		oss_midi_outputintr_t outputintr)
{
  lynx_midic *midic = (lynx_midic *) midi_devs[dev]->devc;
  lynx_devc *devc = midic->devc;
  oss_native_word flags;

  PADAPTER pA;
  PMIDI pREC, pPLAY;

  pA = &devc->adapter;
  pPLAY = &pA->Midi[midic->hw_outdev];
  pREC = &pA->Midi[midic->hw_indev];

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (midic->midi_opened)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  midic->midi_input_intr = inputbyte;
  midic->midi_opened = mode;

  if (mode & OPEN_READ)
    HalMidiSetMode (pREC, MODE_RECORD);

  if (mode & OPEN_WRITE)
    HalMidiSetMode (pPLAY, MODE_PLAY);

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

static void
lynx_midi_close (int dev, int mode)
{
  lynx_midic *midic = (lynx_midic *) midi_devs[dev]->devc;
  lynx_devc *devc = midic->devc;

  PADAPTER pA;
  PMIDI pREC, pPLAY;

  pA = &devc->adapter;
  pPLAY = &pA->Midi[midic->hw_outdev];
  pREC = &pA->Midi[midic->hw_indev];

  HalMidiSetMode (pPLAY, MODE_IDLE);
  HalMidiSetMode (pREC, MODE_IDLE);

  midic->midi_opened = 0;
}

static int
lynx_midi_out (int dev, unsigned char midi_byte)
{
  lynx_midic *midic = (lynx_midic *) midi_devs[dev]->devc;
  lynx_devc *devc = midic->devc;
  PADAPTER pA;
  PMIDI pPLAY;
  ULONG i, n;

  pA = &devc->adapter;
  pPLAY = &pA->Midi[midic->hw_outdev];

  for (i = 0; i < 1000; i++)
    {
      HalMidiWrite (pPLAY, &midi_byte, 1, &n);
      if (n == 1)
	return 1;
    }

  return 0;			/* Timeout */
}

static int
lynx_midi_ioctl (int dev, unsigned cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static midi_driver_t lynx_midi_driver = {
  lynx_midi_open,
  lynx_midi_close,
  lynx_midi_ioctl,
  lynx_midi_out
};

static int
install_mididev (lynx_devc * devc, int ch)
{
  lynx_midic *midic;

  midic = &devc->midic[ch];
  midic->devc = devc;

  if (ch == 0)
    {
      midic->hw_indev = MIDI_RECORD0_DEVICE;
      midic->hw_outdev = MIDI_PLAY0_DEVICE;
    }
  else
    {
      midic->hw_indev = MIDI_RECORD1_DEVICE;
      midic->hw_outdev = MIDI_PLAY1_DEVICE;
    }

  midic->midi_dev =
    oss_install_mididev (OSS_MIDI_DRIVER_VERSION, "LYNXONE", "LynxONE",
			 &lynx_midi_driver, sizeof (midi_driver_t),
			 0, midic, devc->osdev);
  return 1;
}

static int
lynx_set_control (int dev, int ctrl, unsigned int cmd, int value)
{
  lynx_devc *devc = mixer_devs[dev]->devc;
  PADAPTER pA;

  pA = &devc->adapter;

  if (cmd == SNDCTL_MIX_READ)
    switch (ctrl)
      {
      case 1:
	return devc->locked_speed;
	break;

      case 2:
	return devc->ratelock;
	break;

      case 3:
	return devc->qsrc;
	break;

      case 4:
	return devc->clock_source;
	break;

      case 5:
	return devc->digital_format;
	break;

      case 6:
	return devc->trim;
	break;

      case 7:
	return devc->monitorsrc;
	break;

      case 8:
	return devc->analogmon;
	break;

      case 9:
	return devc->digitalmon;
	break;

      default:
	return OSS_EIO;
      }

  if (cmd == SNDCTL_MIX_WRITE)
    switch (ctrl)
      {
      case 1:
	if (value < 8000 || value > 96000)
	  return OSS_EINVAL;
	if (!devc->rate_fixed)
	  devc->speed = value;
	return devc->locked_speed = value;
	break;

      case 2:
	devc->ratelock = !!value;
	if (!devc->rate_fixed)
	  devc->speed = devc->locked_speed;
	return value;
	break;

      case 3:
	{
	  int i;
	  value = !!value;

	  for (i = 0; i < 4; i++)
	    {
	      int d = devc->portc[i].audiodev;

	      if (value)
		audio_engines[d]->flags |= ADEV_FIXEDRATE;
	      else
		audio_engines[d]->flags &= ~ADEV_FIXEDRATE;
	      audio_engines[dev]->fixed_rate = devc->speed;
	    }
	}
	return devc->qsrc = value;
	break;

      case 4:
	if (value < 0 || value > 7)
	  {
	    cmn_err (CE_WARN, "LynxONE: Bad sync source %d\n", value);
	    return OSS_EINVAL;
	  }
	return devc->clock_source = value;
	break;

      case 5:
	if (value < 0 || value > 1)
	  {
	    cmn_err (CE_WARN, "LynxONE: Bad digital format %d\n", value);
	    return OSS_EINVAL;
	  }
	return devc->digital_format = value;
	break;

      case 6:
	switch (value)
	  {
	  case 1:		/* Consumer */
	    HalMixerSetControl (&pA->Mixer, LINE_ADAPTER, LINE_NO_SOURCE,
				CONTROL_TRIM, 0, MIXVAL_TRIM_CONSUMER);
	    return devc->trim = 1;
	    break;

	  case 0:		/* Professional */
	  default:
	    HalMixerSetControl (&pA->Mixer, LINE_ADAPTER, LINE_NO_SOURCE,
				CONTROL_TRIM, 0, MIXVAL_TRIM_PROFESSIONAL);
	    return devc->trim = 0;
	    break;
	  }

      case 7:
	/* Monitoring source */
	if (value < 0)
	  value = 0;
	if (value > 3)
	  value = 3;
	if (HalMixerSetControl (&pA->Mixer, LINE_ADAPTER, LINE_NO_SOURCE,
				CONTROL_MONITOR_SOURCE, 0,
				value) != HSTATUS_OK)
	  {
	    cmn_err (CE_WARN, "CONTROL_MONITOR_SOURCE failed\n");
	    return devc->monitorsrc;
	  }
	return devc->monitorsrc = value;
	break;

      case 8:
	/* Analog monitoring */
	value = !!value;
	if (HalMixerSetControl (&pA->Mixer, LINE_ANALOG_OUT, LINE_PLAY_0,
				CONTROL_MONITOR, 0, value) != HSTATUS_OK)
	  cmn_err (CE_WARN, "Setting analog monitor failed\n");
	return devc->analogmon = value;
	break;

      case 9:
	/* Digital monitoring */
	value = !!value;
	if (HalMixerSetControl (&pA->Mixer, LINE_DIGITAL_OUT, LINE_PLAY_1,
				CONTROL_MONITOR, 0, value) != HSTATUS_OK)
	  cmn_err (CE_CONT, "Setting digital monitor failed\n");
	return devc->digitalmon = value;
	break;

      default:
	return OSS_EIO;
      }

  return OSS_EINVAL;
}

static int
lynx_mix_init (int dev)
{
  int group, ctl;

  if ((group = mixer_ext_create_group (dev, 0, "LYNXONE")) < 0)
    return group;

  if ((ctl = mixer_ext_create_control (dev, group,
				       1, lynx_set_control,
				       MIXT_VALUE,
				       "LYNX_RATE", 96000,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if ((ctl = mixer_ext_create_control (dev, group,
				       2, lynx_set_control,
				       MIXT_ONOFF,
				       "LYNX_RATELOCK", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if ((ctl = mixer_ext_create_control (dev, group,
				       3, lynx_set_control,
				       MIXT_ONOFF,
				       "LYNX_QSRC", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if ((ctl = mixer_ext_create_control (dev, group,
				       4, lynx_set_control,
				       MIXT_ENUM,
				       "LYNX_SYNC", 8,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if ((ctl = mixer_ext_create_control (dev, group,
				       5, lynx_set_control,
				       MIXT_ENUM,
				       "LYNX_FORMAT", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if ((ctl = mixer_ext_create_control (dev, group,
				       6, lynx_set_control,
				       MIXT_ENUM,
				       "LYNX_TRIM", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if ((group = mixer_ext_create_group (dev, 0, "MONITOR")) < 0)
    return group;

  if ((ctl = mixer_ext_create_control (dev, group,
				       7, lynx_set_control,
				       MIXT_ENUM,
				       "SOURCE", 4,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;
  mixer_ext_set_strings (dev, ctl, "ANALOGIN DIGITALIN ANALOGOUT DIGITALOUT",
			 0);

  if ((ctl = mixer_ext_create_control (dev, group,
				       8, lynx_set_control,
				       MIXT_ONOFF,
				       "ANALOGOUT", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  if ((ctl = mixer_ext_create_control (dev, group,
				       9, lynx_set_control,
				       MIXT_ONOFF,
				       "DIGITALOUT", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  return 0;
}

static unsigned int db2lin_101[101] = { 0,
  0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 5, 6, 6, 7, 8, 9,
  10, 12, 13, 15, 17, 19, 21, 24, 27, 30, 34, 38, 43, 48, 54, 61, 68, 76,
  86, 96, 108, 121, 136, 152, 171, 191, 215, 241, 270, 303, 339, 381,
  427, 479, 537, 602, 675, 757, 848, 951, 1067, 1196, 1341, 1503, 1686,
  1890, 2119, 2376, 2664, 2987, 3349, 3754, 4209, 4720, 5292, 5933, 6652,
  7458, 8362, 9375, 10511, 11785, 13213, 14814, 16610, 18622, 20879,
  23409, 26246, 29427, 32993, 36991, 41473, 46499, 52134, 58451,
  65535
};

static int
set_volume (lynx_devc * devc, int val)
{
  PADAPTER pA = &devc->adapter;
  int left, right;

  left = val & 0xff;
  right = (val >> 8) & 0xff;

  if (left < 0)
    left = 0;
  if (left > 100)
    left = 100;
  if (right < 0)
    right = 0;
  if (right > 100)
    right = 100;

  val = left | (right << 8);
  devc->leftvol = left;
  devc->rightvol = right;

  devc->left = db2lin_101[left];
  devc->right = db2lin_101[right];

  /* Set play volume L & R */
  HalMixerSetControl (&pA->Mixer, LINE_ANALOG_OUT, LINE_PLAY_0,
		      CONTROL_VOLUME, 0, devc->left);
  HalMixerSetControl (&pA->Mixer, LINE_ANALOG_OUT, LINE_PLAY_0,
		      CONTROL_VOLUME, 1, devc->right);


  return val;
}

static int
lynx_mixer_ioctl (int dev, int audiodev, unsigned int cmd, ioctl_arg arg)
{
  int val;
  lynx_devc *devc = mixer_devs[dev]->devc;

  switch (cmd)
    {
    case SOUND_MIXER_READ_DEVMASK:
    case SOUND_MIXER_READ_STEREODEVS:
      return *arg = SOUND_MASK_PCM | SOUND_MASK_LINE;
      break;

    case SOUND_MIXER_READ_RECMASK:
    case SOUND_MIXER_READ_RECSRC:
    case SOUND_MIXER_WRITE_RECSRC:
      return *arg = SOUND_MASK_LINE;
      break;

    case SOUND_MIXER_READ_CAPS:
      return *arg = 0 /* SOUND_CAP_NOLEGACY */ ;
      break;

    case SOUND_MIXER_READ_LINE:
    case SOUND_MIXER_WRITE_LINE:
      return *arg = 100 | (100 << 8);
      break;

    case SOUND_MIXER_READ_PCM:
      return *arg = devc->leftvol | (devc->rightvol << 8);
      break;

    case SOUND_MIXER_WRITE_PCM:
      val = *arg;
      return *arg = set_volume (devc, val);
      break;

    default:
      return OSS_EINVAL;
    }
}

static mixer_driver_t lynx_mixer_driver = {
  lynx_mixer_ioctl
};

static int
init_lynxone (lynx_devc * devc)
{
  PADAPTER pA = &devc->adapter;
  if (HalOpenAdapter (devc) != HSTATUS_OK)
    {
      cmn_err (CE_WARN, "LynxONE: HalOpenAdapter failed\n");
      return 0;
    }
  DDB (cmn_err (CE_WARN, "\nLynxONE Device found...\n"));
  DDB (cmn_err (CE_WARN, "  Serial number: %d\n", EEPromReadWord (pA, 0x34)));
  DDB (cmn_err
       (CE_WARN, "  Revision: %c\n", (0x40 + EEPromReadWord (pA, 0x35))));
  DDB (cmn_err (CE_WARN, "  Config date: %02d-%02d-%04d\n\n", (EEPromReadWord (pA, 0x37) >> 8),	/* mon */
		(EEPromReadWord (pA, 0x37) & 0xff),	/* day */
		EEPromReadWord (pA, 0x36)));

  devc->leftvol = 100;
  devc->rightvol = 100;
  devc->left = devc->right = 65535;

  if ((devc->mixer_dev = oss_install_mixer (OSS_MIXER_DRIVER_VERSION,
					    devc->osdev,
					    devc->osdev,
					    "LynxONE Control Panel",
					    &lynx_mixer_driver,
					    sizeof (mixer_driver_t),
					    devc)) >= 0)
    {
      mixer_ext_set_init_fn (devc->mixer_dev, lynx_mix_init, 20);
      mixer_devs[devc->mixer_dev]->priority = -1;	/* Don't use as the default mixer */
    }
  else
    return 0;

  devc->first_dev = -1;
  install_audiodev (devc, "LynxONE analog output", TY_AOUT);
  install_audiodev (devc, "LynxONE analog input", TY_AIN);
  install_audiodev (devc, "LynxONE digital output", TY_DOUT);
  install_audiodev (devc, "LynxONE digital input", TY_DIN);
  devc->master_dev = -1;
  devc->rate_fixed = 0;
  devc->qsrc = 0;

  install_mididev (devc, 0);
  install_mididev (devc, 1);

  return 1;
}

int
lynxone_attach (oss_device_t * osdev)
{
  lynx_devc *devc;
  unsigned char pci_irq_line, pci_revision /*, pci_latency */ ;
  unsigned short pci_command, vendor, device;
  unsigned int pci_ioaddr0, pci_ioaddr2, pci_ioaddr3;
  int err;

  DDB (cmn_err (CE_WARN, "Entered LynxONE detect routine\n"));

  pci_read_config_word (osdev, PCI_VENDOR_ID, &vendor);
  pci_read_config_word (osdev, PCI_DEVICE_ID, &device);

  if (vendor != PLX_VENDOR_ID || device != PLX_LYNXONE)
    return 0;

  pci_read_config_byte (osdev, PCI_REVISION_ID, &pci_revision);
  pci_read_config_word (osdev, PCI_COMMAND, &pci_command);
  pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line);
  pci_read_config_dword (osdev, PCI_MEM_BASE_ADDRESS_0, &pci_ioaddr0);
  pci_read_config_dword (osdev, PCI_MEM_BASE_ADDRESS_2, &pci_ioaddr2);
  pci_read_config_dword (osdev, PCI_MEM_BASE_ADDRESS_3, &pci_ioaddr3);

  if ((pci_ioaddr0 == 0) || (pci_ioaddr2 == 0) || (pci_ioaddr3 == 0))
    {
      cmn_err (CE_WARN, "LYNXONE: BAR(s) not initialized by BIOS\n");
      return 0;
    }


  if (pci_irq_line == 0)
    {
      cmn_err (CE_WARN, "LYNXONE: IRQ not initialized by BIOS\n");
      return 0;
    }

  if ((devc = PMALLOC (osdev, sizeof (*devc))) == NULL)
    {
      cmn_err (CE_WARN, "Out of memory\n");
      return 0;
    }


  devc->irq = pci_irq_line;
  devc->osdev = osdev;
  osdev->devc = devc;

  pci_command |= PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY;
  pci_write_config_word (osdev, PCI_COMMAND, pci_command);

  oss_register_device (osdev, "LynxONE");

  devc->plx_addr = pci_ioaddr0;
  devc->plx_ptr =
    (unsigned char *) MAP_PCI_MEM (devc->osdev, 0, devc->plx_addr, 128);
  if (devc->plx_ptr == NULL)
    {
      cmn_err (CE_WARN, "LynxONE: Can't map PCI registers (0)\n");
      return 0;
    }

  devc->base_addr = pci_ioaddr2;
  devc->base_ptr =
    (unsigned char *) MAP_PCI_MEM (devc->osdev, 2, devc->base_addr,
				   64 * 1024);
  if (devc->base_ptr == NULL)
    {
      cmn_err (CE_WARN, "LynxONE: Can't ioremap PCI registers (2)\n");
      return 0;
    }

  devc->fpga_addr = pci_ioaddr3;
  devc->fpga_ptr =
    (unsigned char *) MAP_PCI_MEM (devc->osdev, 3, devc->fpga_addr, 16);
  if (devc->fpga_ptr == NULL)
    {
      cmn_err (CE_WARN, "LynxONE: Can't ioremap PCI registers (3)\n");
      return 0;
    }

  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);

  if ((err = oss_register_interrupts (devc->osdev, 0, lynxintr, NULL)) < 0)
    {
      cmn_err (CE_WARN, "Can't register interrupt handler, err=%d\n", err);
      return 0;
    }

  return init_lynxone (devc);
}

int
lynxone_detach (oss_device_t * osdev)
{
  lynx_devc *devc = (lynx_devc *) osdev->devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  HalCloseAdapter (&devc->adapter);

  oss_unregister_interrupts (devc->osdev);

  MUTEX_CLEANUP (devc->mutex);
  UNMAP_PCI_MEM (devc->osdev, 0, devc->plx_addr, devc->plx_ptr, 128);
  UNMAP_PCI_MEM (devc->osdev, 2, devc->base_addr, devc->base_ptr, 64 * 1024);
  UNMAP_PCI_MEM (devc->osdev, 3, devc->fpga_addr, devc->fpga_ptr, 16);
  oss_unregister_device (devc->osdev);

  return 1;
}
