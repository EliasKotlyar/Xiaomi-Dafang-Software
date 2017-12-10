/*
 * ddksample_audio.c - OSS DDK sample driver - Audio functionality
 *
 * Description:
 *
 * This simple audio driver implements an OSS audio device similar to
 * /dev/zero. The output will be just discarded. Recording will return
 * silence. In addition the sample parameters (rate, channels and format)
 * is emulated. Timeout callbacks are used to keep the device running
 * at the right rate (approximately).
 *
 * To demonstrate mixer functionality (together with ddksample_mixer.c) this
 * driver also computes the peak levels from the output data. Also output
 * volume controls are implemented.
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


/*
 * Solaris DDI includes
 */
#include <sys/types.h>
#include <sys/modctl.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>

/*
 * OSS specific includes
 */
#include <sys/soundcard.h>
#include <sys/ossddk/ossddk.h>

/***************************************************
 * Private types and variables
 */

#include "ddksample.h"

/**************************************************/

/*
 * This driver emulates periodic interrupts on fragment boundaries using
 * the timeout(9f) mechanism. Ordinary drivers use device interrupts
 * for this.
 */

static void
ddksample_intr (void *arg)
{
  ddksample_portc *portc = arg;

  if (portc->audio_active & PCM_ENABLE_OUTPUT)
    {
#if 1
/*
 * We will first do some computations on the output data. This is not
 * necessary in normal drivers. It's here just to make the mixer section
 * to behave like the mixer of some real device.
 */
      int pos;
      unsigned char *buf;
      dmap_t *dmap;

      dmap = ossddk_adev_get_dmapout (portc->dev);
      buf = ossddk_dmap_get_dmabuf (dmap);

      /*
       * Compute the current playback position (beginning of the
       * fragment we have just finished).
       */
      pos = ossddk_dmap_get_qhead (dmap) * ossddk_dmap_get_fragsize (dmap);

      buf = buf + pos;
      ddksample_do_math (portc, buf, ossddk_dmap_get_fragsize (dmap));
#endif




      ossddk_audio_outputintr (portc->dev, 0);	/* Acknowledge one fragment */
    }

  if (portc->audio_active & PCM_ENABLE_INPUT)
    {
      ossddk_audio_inputintr (portc->dev);	/* Acknowledge one fragment */
    }

  portc->timeout_id = timeout (ddksample_intr, portc, portc->timeout_value);
}

static int
ddksample_set_rate (int dev, int arg)
{
  ddksample_portc *portc = ossddk_adev_get_portc (dev);

  if (arg == 0)			/* Query - return the current rate */
    return portc->speed;

/*
 * If the requested rate is not supported the driver may just return the
 * current rate. Or preferably it may select the rate that is closest to the
 * requested one. In this sample driver we use the easy way.
 */

  if (arg != 16000 && arg != 24000 && arg != 48000)
    return portc->speed;

/*
 * The rate was OK so write it down and report it back.
 */

  return portc->speed = arg;
}

static short
ddksample_set_channels (int dev, short arg)
{
  ddksample_portc *portc = ossddk_adev_get_portc (dev);

  if (arg == 0)			/* Query - return the current setting */
    return portc->channels;

/*
 * Assume 2 channels (stereo) if something incorrect is
 * requested.
 */

  if (arg != 1 && arg != 2)
    return 2;

  return portc->channels = arg;
}

static unsigned int
ddksample_set_format (int dev, unsigned int arg)
{
  ddksample_portc *portc = ossddk_adev_get_portc (dev);

  if (arg == 0)			/* Query */
    return portc->format;

/*
 * Fall back to 16 bits (native endianess) if some unsupported
 * rate is requested.
 */

  if (arg != AFMT_S16_NE && arg != AFMT_S32_NE)
    return AFMT_S16_NE;

  if (arg == AFMT_S32_NE)
    portc->bits = 32;
  else
    portc->bits = 16;

  return portc->format = arg;
}

static int
ddksample_ioctl (int dev, unsigned int cmd, int *arg)
{
// TODO 
  return -EINVAL;
}

static void ddksample_trigger (int dev, int state);

static void
ddksample_reset (int dev)
{
  ddksample_trigger (dev, 0);
}

static int
ddksample_open (int dev, int mode, int open_flags)
{
  ddksample_portc *portc = ossddk_adev_get_portc (dev);
  ddksample_devc *devc = ossddk_adev_get_devc (dev);

  mutex_enter (&devc->mutex);

  if (portc->open_mode != 0)	/* Device busy */
    {
      mutex_exit (&devc->mutex);
      return -EBUSY;
    }

  portc->open_mode = mode;
  devc->audio_open_count++;
  mutex_exit (&devc->mutex);

  return 0;
}

static void
ddksample_close (int dev, int mode)
{
  ddksample_portc *portc = ossddk_adev_get_portc (dev);
  ddksample_devc *devc = ossddk_adev_get_devc (dev);

  ddksample_reset (dev);

  mutex_enter (&devc->mutex);
  portc->open_mode = 0;
  devc->audio_open_count--;
  mutex_exit (&devc->mutex);
}

static void
ddksample_trigger (int dev, int state)
{
  ddksample_portc *portc = ossddk_adev_get_portc (dev);

  if (portc->open_mode & OPEN_WRITE)
    {
      if ((state & PCM_ENABLE_OUTPUT)
	  && (portc->prepare_flags & PCM_ENABLE_OUTPUT))
	{
	  portc->prepare_flags &= ~PCM_ENABLE_OUTPUT;

	  /* 
	   * Arm the timeout if it was not started yet.
	   */
	  if (portc->audio_active == 0)
	    portc->timeout_id =
	      timeout (ddksample_intr, portc, portc->timeout_value);
	  portc->audio_active |= PCM_ENABLE_OUTPUT;
	}
      else
	if (!(state & PCM_ENABLE_OUTPUT)
	    && (portc->audio_active & PCM_ENABLE_OUTPUT))
	{
	  portc->audio_active &= ~PCM_ENABLE_OUTPUT;
	  if (portc->audio_active == 0)
	    untimeout (portc->timeout_id);
	  portc->timeout_id = 0;
	}
    }

  if (portc->open_mode & OPEN_READ)
    {
      if ((state & PCM_ENABLE_INPUT)
	  && (portc->prepare_flags & PCM_ENABLE_INPUT))
	{
	  portc->prepare_flags &= ~PCM_ENABLE_INPUT;

	  /* 
	   * Arm the timeout if it was not started yet.
	   */
	  if (portc->audio_active == 0)
	    portc->timeout_id =
	      timeout (ddksample_intr, portc, portc->timeout_value);
	  portc->audio_active |= PCM_ENABLE_INPUT;
	}
      else
	if (!(state & PCM_ENABLE_INPUT)
	    && (portc->audio_active & PCM_ENABLE_INPUT))
	{
	  portc->audio_active &= ~PCM_ENABLE_INPUT;
	  if (portc->audio_active == 0)
	    untimeout (portc->timeout_id);
	  portc->timeout_id = 0;
	}
    }
}

static int
ddksample_prepare_for_input (int dev, int fragsize, int numfrags)
{
  ddksample_portc *portc = ossddk_adev_get_portc (dev);
  int datarate, intrate, tmout;

  cmn_err (CE_CONT, "Input, rate=%d, channels=%d, format=0x%x, bits=%d\n",
	   portc->speed, portc->channels, portc->format, portc->bits);

/*
 * Compute the number of lbolt ticks between interrupts. This value is needed
 * for timeout(9f).
 */

  datarate = portc->speed * portc->channels * portc->bits / 8;	/* Bytes per second */
  intrate = datarate * 10 / fragsize;
  tmout = hz * 1000 / intrate;

  cmn_err (CE_CONT, "Datarate %d, fragsize %d\n", datarate, fragsize);
  cmn_err (CE_CONT, "Intrate %d.%d interrupts / sec.\n", intrate / 10,
	   intrate % 10);
  cmn_err (CE_CONT, "Timeout %d.%02d ticks\n", tmout / 100, tmout % 100);

  tmout = (tmout + 50) / 100;	/* Round to the nearest integer value */
  if (tmout < 1)
    tmout = 1;

  portc->timeout_value = tmout;
  portc->prepare_flags |= PCM_ENABLE_INPUT;
  return 0;
}

static int
ddksample_prepare_for_output (int dev, int fragsize, int numfrags)
{
  ddksample_portc *portc = ossddk_adev_get_portc (dev);
  int datarate, intrate, tmout;

  cmn_err (CE_CONT, "Output, rate=%d, channels=%d, format=0x%x, bits=%d\n",
	   portc->speed, portc->channels, portc->format, portc->bits);

/*
 * Compute the number of lbolt ticks between interrupts. This value is needed
 * for timeout(9f).
 */

  datarate = portc->speed * portc->channels * portc->bits / 8;	/* Bytes per second */
  intrate = datarate * 10 / fragsize;
  tmout = hz * 1000 / intrate;

  cmn_err (CE_CONT, "Datarate %d, fragsize %d\n", datarate, fragsize);
  cmn_err (CE_CONT, "Intrate %d.%d interrupts / sec.\n", intrate / 10,
	   intrate % 10);
  cmn_err (CE_CONT, "Timeout %d.%02d ticks\n", tmout / 100, tmout % 100);

  tmout = (tmout + 50) / 100;	/* Round to the nearest integer value */
  if (tmout < 1)
    tmout = 1;

  portc->timeout_value = tmout;
  portc->prepare_flags |= PCM_ENABLE_OUTPUT;
  return 0;
}

static int
ddksample_alloc_buffer (int dev, dmap_t * dmap, int direction)
{
#define MY_BUFFSIZE (64*1024)
  if (ossddk_dmap_get_dmabuf (dmap) != NULL)
    return 0;
  ossddk_dmap_set_phys (dmap, 0);	/* Not mmap() capable */
  ossddk_dmap_set_dmabuf (dmap, kmem_zalloc (MY_BUFFSIZE, KM_SLEEP));
  if (ossddk_dmap_get_dmabuf (dmap) == NULL)	/* Alloc failed */
    return -ENOSPC;
  ossddk_dmap_set_buffsize (dmap, MY_BUFFSIZE);

  return 0;
}

static int
ddksample_free_buffer (int dev, dmap_t * dmap, int direction)
{
  if (ossddk_dmap_get_dmabuf (dmap) == NULL)
    return 0;
  kmem_free (ossddk_dmap_get_dmabuf (dmap), MY_BUFFSIZE);

  ossddk_dmap_set_dmabuf (dmap, NULL);
  return 0;
}

#if 0
static int
ddksample_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  return 0;
}
#endif

static audiodrv_t ddksample_audio_driver = {
  ddksample_open,
  ddksample_close,
  NULL,				// output_block
  NULL,				// start_input
  ddksample_ioctl,
  ddksample_prepare_for_input,
  ddksample_prepare_for_output,
  ddksample_reset,
  NULL,
  NULL,
  NULL,
  NULL,
  ddksample_trigger,
  ddksample_set_rate,
  ddksample_set_format,
  ddksample_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  ddksample_alloc_buffer,
  ddksample_free_buffer,
  NULL,
  NULL,
  NULL				/* ddksample_get_buffer_pointer */
};

int
ddksample_audio_init (ddksample_devc * devc)
{
  int dev;
  int i;
  static int rates[] = { 16000, 24000, 48000 };

  for (i = 0; i < MAX_SUBDEVICE; i++)
    {
      ddksample_portc *portc;
      int flags;

      flags = ADEV_VIRTUAL | ADEV_DUPLEX;

      if (i > 0)
	flags |= ADEV_SHADOW;

      if ((dev = ossddk_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
					  devc->osdev,
					  devc->osdev,
					  "OSS DDK sample device",
					  &ddksample_audio_driver,
					  sizeof (audiodrv_t),
					  flags,
					  AFMT_S16_NE | AFMT_S32_NE,
					  devc, -1)) < 0)
	return dev;

      portc = &devc->portc[devc->num_portc++];

      portc->dev = dev;
      portc->speed = 48000;
      portc->channels = 2;
      portc->format = AFMT_S16_NE;
      portc->bits = 16;
      portc->open_mode = 0;
      portc->timeout_id = 0;
      portc->left_volume = DDKSAMPLE_MAX_VOL;
      portc->right_volume = DDKSAMPLE_MAX_VOL;
      portc->left_peak = 0;
      portc->right_peak = 0;
      portc->mute = 0;

      ossddk_adev_set_portc (dev, portc);
      ossddk_adev_set_portnum (dev, i);
      ossddk_adev_set_mixer (dev, devc->mixer_dev);

      ossddk_adev_set_rates (dev, 16000, 48000, 3, rates);
      ossddk_adev_set_channels (dev, 1, 2);	/* Mono and stereo are supported */

      /*
       * Usually the list of supported audio formats is set when calling
       * ossddk_install_audiodev(). However in some cases it may be
       * necessary to change the format list later. Changing it is
       * also necessary if recording formats are fdifferent than the
       * playback formats.
       *
       * The next lines demonstrates how to do that. Normal drivers should
       * never do that.
       */
      ossddk_adev_set_formats (dev, AFMT_S32_NE | AFMT_S16_NE,	// 16 and 32 bit playback
			       AFMT_S16_NE);	// Only 16 bit recording


      /*
       * Report stereo as the preferred channel configuration.
       */
      ossddk_adev_set_caps (dev, DSP_CH_STEREO);

#if 0
      /*
       * Many devices have some limits for the fragment size. For
       * example it's common that the fragment must fit in a single
       * memory page. In such cases setting the fragment size limits is
       * necessary. Otherwise the driver should not care about them.
       *
       * Both the upper and lower limits are set using the same call.
       * Value of 0 means no limit.
       *
       * Here we set the upper limit to PAGE_SIZE and no lower limit.
       */
      ossddk_adev_set_buflimits (dev, 0, PAGE_SIZE);
#endif

#if 0
      /*
       * In some cases the device may be rate clocked with some other
       * device that provides the sample clock. For example there may
       * be some special cable connected between the devices. When the 
       * driver knows this it can report the device which controls
       * the sampling rate. Otherwise this information must be left 
       * untouched.
       *
       * Knowing that two or more devices have their rates locked will
       * make work of some future OSS subsystems more reliable. However
       * false information will probably break things in the hard way.
       */
      ossddk_adev_set_ratesource (dev, devc->some_known_device_number);
#endif

#if 0
      /*
       * If the device requires some kind of proprietary control program
       * it's necessary to assign some magic number for the device.
       * The control program can find the right device by looking at
       * the magic field returned by SNDCTL_AUDIOINFO.
       *
       * Normal devices must not report any kind of magic numbers. In
       * particular this feature must not be used to help "client"
       * programs to detect what kind of device is being used.
       *
       * CAUTION! To avoid conflicts between two drivers using the same
       * magic it's necessary to request the magic number from 
       * 4Front Technologies (support@opensound.com).
       */
      ossddk_adev_set_magic (dev, magic_number_assigned_for_this_driver);
#endif
    }

  return 0;
}
