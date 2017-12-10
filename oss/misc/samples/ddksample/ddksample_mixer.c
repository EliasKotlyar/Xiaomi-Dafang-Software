/*
 * ddksample_audio.c: OSS DDK sample driver - Mixer functionality
 *
 * Description:
 * This source module implements the mixer functionality of the sample
 * driver. The devc and portc fields used by the mixer are actually used
 * in the ddksample_misc.c module which emulates some audio hardware.
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
static const unsigned char peak_cnv[256] = {
  0, 18, 29, 36, 42, 47, 51, 54, 57, 60, 62, 65, 67, 69, 71, 72,
  74, 75, 77, 78, 79, 81, 82, 83, 84, 85, 86, 87, 88, 89, 89, 90,
  91, 92, 93, 93, 94, 95, 95, 96, 97, 97, 98, 99, 99, 100, 100, 101,
  101, 102, 102, 103, 103, 104, 104, 105, 105, 106, 106, 107, 107, 108, 108,
  108,
  109, 109, 110, 110, 110, 111, 111, 111, 112, 112, 113, 113, 113, 114, 114,
  114,
  115, 115, 115, 115, 116, 116, 116, 117, 117, 117, 118, 118, 118, 118, 119,
  119,
  119, 119, 120, 120, 120, 121, 121, 121, 121, 122, 122, 122, 122, 122, 123,
  123,
  123, 123, 124, 124, 124, 124, 125, 125, 125, 125, 125, 126, 126, 126, 126,
  126,
  127, 127, 127, 127, 127, 128, 128, 128, 128, 128, 129, 129, 129, 129, 129,
  130,
  130, 130, 130, 130, 130, 131, 131, 131, 131, 131, 131, 132, 132, 132, 132,
  132,
  132, 133, 133, 133, 133, 133, 133, 134, 134, 134, 134, 134, 134, 134, 135,
  135,
  135, 135, 135, 135, 135, 136, 136, 136, 136, 136, 136, 136, 137, 137, 137,
  137,
  137, 137, 137, 138, 138, 138, 138, 138, 138, 138, 138, 139, 139, 139, 139,
  139,
  139, 139, 139, 140, 140, 140, 140, 140, 140, 140, 140, 141, 141, 141, 141,
  141,
  141, 141, 141, 141, 142, 142, 142, 142, 142, 142, 142, 142, 142, 143, 143,
  143,
  143, 143, 143, 143, 143, 143, 144, 144, 144, 144, 144, 144, 144, 144, 144,
  144,
};

static int
peak_cb (int dev, int ctl_id, unsigned int cmd, int value)
{
  ddksample_devc *devc = ossddk_mixer_get_devc (dev);

  if (ctl_id < 0 || ctl_id >= devc->num_portc)
    return -EINVAL;

  if (cmd == SNDCTL_MIX_READ)
    {
      ddksample_portc *portc = &devc->portc[ctl_id];
      int val, left, right;

      /*
       * Truncate the peak counters to 8 bits (signed range).
       */
      left = portc->left_peak >> 15;
      right = portc->right_peak >> 15;

      if (left > 255)
	left = 255;
      if (right > 255)
	right = 255;

      /*
       * Convert to logarithmic (dB) scale 
       */
      left = peak_cnv[left];
      right = peak_cnv[right];

      val = left | (right << 8);

      portc->left_peak = portc->right_peak = 0;	/* Reset */

      return val;
    }


  return -EINVAL;
}

static int
mute_cb (int dev, int ctl_id, unsigned int cmd, int value)
{
  ddksample_devc *devc = ossddk_mixer_get_devc (dev);

  if (ctl_id < 0 || ctl_id >= devc->num_portc)
    return -EINVAL;

  if (cmd == SNDCTL_MIX_WRITE)
    {
      ddksample_portc *portc = &devc->portc[ctl_id];

      return portc->mute = (value != 0);
    }

  if (cmd == SNDCTL_MIX_READ)
    {
      ddksample_portc *portc = &devc->portc[ctl_id];

      return portc->mute;
    }

  return -EINVAL;
}

static int
gain_cb (int dev, int ctl_id, unsigned int cmd, int value)
{
  ddksample_devc *devc = ossddk_mixer_get_devc (dev);

  if (ctl_id < 0 || ctl_id >= devc->num_portc)
    return -EINVAL;

  if (cmd == SNDCTL_MIX_WRITE)
    {
      int left, right;
      ddksample_portc *portc = &devc->portc[ctl_id];

      left = value & 0xff;
      right = (value >> 8) & 0xff;

      if (left < 0)
	left = 0;
      if (left > DDKSAMPLE_MAX_VOL)
	left = DDKSAMPLE_MAX_VOL;
      if (right < 0)
	right = 0;
      if (right > DDKSAMPLE_MAX_VOL)
	right = DDKSAMPLE_MAX_VOL;

      portc->left_volume = left;
      portc->right_volume = right;

      return left | (right << 8);
    }

  if (cmd == SNDCTL_MIX_READ)
    {
      ddksample_portc *portc = &devc->portc[ctl_id];
      return portc->left_volume | (portc->right_volume << 8);
    }

  return -EINVAL;
}

static int
ddksample_create_controls (int dev)
{
  ddksample_devc *devc = ossddk_mixer_get_devc (dev);
  int group, top, i;
  int ctl;

  /*
   * Create the top level group
   *
   * By convention all controls specific to /dev/dsp# device files
   * are grouped under a group called "/dev".
   */

  if ((top = ossddk_mixer_create_group (dev,	// Mixer device number
					OSSDDK_MIXER_ROOT,	// Parent group
					"/dev")) < 0)	// Name
    return top;

  for (i = 0; i < devc->num_portc; i++)
    {
      char name[16];

      ddksample_portc *portc = &devc->portc[i];

      /*
       * Create a special group for each /dev/dsp# device.
       * Control/group names staring with "@pcm" have special function.
       * GUI mixers will replace them with the application name using
       * the corresponding /dev/dsp# device. In this way the mixer
       * interface is more intuitive.
       *
       * In our case we create a group for each audio device and
       * place the controls under them. 
       *
       * We have both a volume slider and a peak meter for each device
       * file. The convention is that the volume slider will be shown first
       * if it affects the peak meter (such as in case). Otherwise the
       * peak meter will be first.
       */
      sprintf (name, "@pcm%d", portc->dev);

      if ((group = ossddk_mixer_create_group (dev,	// Mixer device number
					      top,	// Parent group
					      name)) < 0)	// Name
	return group;

      if ((ctl = ossddk_mixer_create_control (dev,	// Mixer device number
					      group,	// Parent group
					      i,	// Controller id
					      gain_cb,	// Callback function
					      MIXT_STEREOSLIDER,	// Control type
					      "-",	// Name (no name)
					      DDKSAMPLE_MAX_VOL,	// Maximum value
					      MIXF_READABLE | MIXF_WRITEABLE)) < 0)	// Protection
	return ctl;

      if ((ctl = ossddk_mixer_create_control (dev,	// Mixer device number
					      group,	// Parent group
					      i,	// Controller id
					      peak_cb,	// Callback function
					      MIXT_STEREOPEAK,	// Control type
					      "-",	// Name (no name)
					      144,	// Maximum value
					      MIXF_READABLE)) < 0)	// Protection
	return ctl;

      if ((ctl = ossddk_mixer_create_control (dev,	// Mixer device number
					      group,	// Parent group
					      i,	// Controller id
					      mute_cb,	// Callback function
					      MIXT_ONOFF,	// Control type
					      "MUTE",	// Name
					      2,	// ONOFF controls should have 2 here
					      MIXF_READABLE | MIXF_WRITEABLE)) < 0)	// Protection
	return ctl;
    }

  return OSSDDK_OK;
}

/**************************************************/

static int
ddksample_legacy_mixer_ioctl (int dev, int audiodev, unsigned int cmd,
			      int *arg)
{
  int vol, left, right;
  ddksample_devc *devc = ossddk_mixer_get_devc (dev);

  switch (cmd)
    {
    case SOUND_MIXER_READ_DEVMASK:
    case SOUND_MIXER_READ_STEREODEVS:
      *arg = SOUND_MASK_VOLUME | SOUND_MASK_MIC | SOUND_MASK_RECLEV;
      return OSSDDK_OK;
      break;

    case SOUND_MIXER_READ_RECMASK:
    case SOUND_MIXER_READ_RECSRC:
    case SOUND_MIXER_WRITE_RECSRC:
      *arg = SOUND_MASK_MIC;
      return OSSDDK_OK;
      break;

    case SOUND_MIXER_READ_VOLUME:
      *arg = devc->mainvol_left | (devc->mainvol_right << 8);
      return OSSDDK_OK;
      break;

    case SOUND_MIXER_WRITE_VOLUME:
      vol = *arg;
      left = vol & 0xff;
      right = (vol >> 8) & 0xff;

      if (left < 0)
	left = 0;
      if (left > 100)
	left = 100;
      if (right < 0)
	right = 0;
      if (right > 100)
	right = 100;

      devc->mainvol_left = left;
      devc->mainvol_right = right;

      *arg = devc->mainvol_left | (devc->mainvol_right << 8);
      return OSSDDK_OK;
      break;

    case SOUND_MIXER_READ_MIC:
    case SOUND_MIXER_WRITE_MIC:
      *arg = 100 | (100 << 8);	/* Always full volume */
      return OSSDDK_OK;
      break;

    case SOUND_MIXER_READ_RECLEV:
    case SOUND_MIXER_WRITE_RECLEV:
      *arg = 100 | (100 << 8);	/* Always full volume */
      return OSSDDK_OK;
      break;

    default:
      return -EINVAL;
    }
}

static mixer_driver_t ddksample_mixer_driver = {
  ddksample_legacy_mixer_ioctl,
  MAX_SUBDEVICE * 4,		/* Number of mixer elements we need */
  ddksample_create_controls
};

int
ddksample_mixer_init (ddksample_devc * devc)
{
  devc->mainvol_left = 100;
  devc->mainvol_right = 100;

  if ((devc->mixer_dev = ossddk_install_mixer (OSS_MIXER_DRIVER_VERSION,
					       devc->osdev,
					       devc->osdev,
					       "OSS DDK sample mixer",
					       &ddksample_mixer_driver,
					       sizeof (mixer_driver_t),
					       devc)) >= 0)
    return devc->mixer_dev;	/* Error code */

#if 0
  /*
   * By default OSS will delay creation of the mixer controls
   * until the mixer is actually used by some application. This means
   * that in some cases the mixer doesn't get created at all before
   * the driver gets unloaded again.
   *
   * The driver can force the mixer to be created and the creation
   * callback (ddksample_create_controls in our case) to be called
   * immediately by calling ossddk_mixer_touch(). This is necessary only
   * if the creation function performs initializations that must be
   * done immediately. Otherwise the touch_mixer() routine must not be
   * called.
   *
   * In this driver ossddk_mixer_touch() must not be called. Mixer
   * initialization is designed to be invoked after audio devices are
   * initialized. Otherwise devc->num_portc will not have right value
   * and mixer initialization will fail.
   */
  ossddk_mixer_touch (devc->mixer_dev);
#endif

  return OSSDDK_OK;
}
