/*
 * Purpose: Viartual audio mixing framework
 *
 * This subsystem makes it possible to share one physical audio between
 * multiple applications at the same time.
 *
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

#define VMIX_MAIN
#include <oss_config.h>
#include "vmix.h"

extern int vmix_disabled;	/* Configuration option (osscore.conf) */
extern int vmix_loopdevs;	/* Configuration option (osscore.conf) */
extern int flat_device_model;
extern int vmix_no_autoattach;
static vmix_mixer_t *mixer_list = NULL; /* List of all currently installed mixer instances */
static int num_instances = 0;

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

/*
 * Mixer/control panel interface
 */
static int
vmix_outvol (int dev, int ctrl, unsigned int cmd, int value)
{
  vmix_mixer_t *mixer = mixer_devs[dev]->vmix_devc;
  int vol;

  if (mixer == NULL)
    return OSS_ENXIO;

  if (cmd == SNDCTL_MIX_READ)
    {
      switch (ctrl)
	{
	case 0:		/* Main output volume */
	  return mixer->play_engine.outvol | (mixer->play_engine.
					      outvol << 16);
	  break;

	case 1:		/* Peak meter */
	  vol =
	    peak_cnv[mixer->play_engine.
		     vu[0]] | (peak_cnv[mixer->play_engine.vu[1]] << 8);
	  mixer->play_engine.vu[0] = 0;
	  mixer->play_engine.vu[1] = 0;
	  return vol;
	  break;

	case 509:		/* Curren sample rate */
	  return mixer->rate;
	  break;

	case 510:		/* Enable/disable */
	  return !mixer->disabled;
	  break;

	case 511:		/* Multi channel enable */
	  return mixer->multich_enable;
	  break;

	case 512:		/* grc3<->interpolation selector */
	  return mixer->src_quality;
	  break;

	default:
	  return OSS_EINVAL;
	}
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
      vol = value & 0xffff;	/* Left/mono channel volume */
      if (vol > DB_SIZE * 5)
	vol = DB_SIZE * 5;

      switch (ctrl)
	{
	case 0:
	  mixer->play_engine.outvol = vol;

	  mixer_devs[dev]->modify_counter++;
	  return vol | (vol << 16);
	  break;

	case 510:		/* Enable/disable */
	  mixer->disabled = !value;
	  return !!value;
	  break;

	case 511:		/* Multich enable */
	  mixer->multich_enable = !!value;
	  mixer_devs[dev]->modify_counter++;
	  return mixer->multich_enable;
	  break;

	case 512:		/* grc3<->interpolation selector */
	  mixer->src_quality = value & 0xff;
	  mixer_devs[dev]->modify_counter++;
	  return mixer->src_quality;
	  break;

	default:
	  return OSS_EINVAL;
	}
    }

  return OSS_EINVAL;
}

static int
vmix_invol (int dev, int ctrl, unsigned int cmd, int value)
{
  vmix_mixer_t *mixer = mixer_devs[dev]->vmix_devc;
  int vol;

  if (mixer == NULL)
    return OSS_ENXIO;

  if (cmd == SNDCTL_MIX_READ)
    {
      switch (ctrl)
	{
	case 0:		/* Main input volume */
	  return mixer->record_engine.outvol | (mixer->record_engine.
						outvol << 16);
	  break;

	case 1:		/* recording peak meter */
	  vol =
	    peak_cnv[mixer->record_engine.
		     vu[0]] | (peak_cnv[mixer->record_engine.vu[1]] << 8);
	  mixer->record_engine.vu[0] = 0;
	  mixer->record_engine.vu[1] = 0;
	  return vol & 0x7fffffff;
	  break;

	default:
	  return OSS_EINVAL;
	}
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
      vol = value & 0xffff;	/* Left/mono channel volume */
      if (vol > DB_SIZE * 5)
	vol = DB_SIZE * 5;

      switch (ctrl)
	{
	case 0:
	  mixer->record_engine.outvol = vol;
	  mixer_devs[mixer->input_mixer_dev]->modify_counter++;
	  return vol | (vol << 16);
	  break;

	default:
	  return OSS_EINVAL;
	}
    }

  return OSS_EINVAL;
}

static int
vmix_outportc_vol (int dev, int ctrl, unsigned int cmd, int value)
{
  vmix_mixer_t *mixer = mixer_devs[dev]->vmix_devc;
  vmix_portc_t *portc;
  int vol, rvol;

  if (mixer == NULL)
    return OSS_ENXIO;

  if (ctrl < 0)
    return OSS_ENXIO;

  if (ctrl >= mixer->num_clientdevs)			/* Client engine not created yet */
     return (DB_SIZE * 5) | ((DB_SIZE * 5) << 16);	/* Force to maximum level */

  portc = mixer->client_portc[ctrl];

  if (cmd == SNDCTL_MIX_READ)
    {
      return portc->volume[0] | (portc->volume[1] << 16);
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
      vol = (value) & 0xffff;	/* Left/mono channel volume */
      rvol = (value >> 16) & 0xffff;	/* Right channel volume */
      if (vol > DB_SIZE * 5)
	vol = DB_SIZE * 5;
      if (rvol > DB_SIZE * 5)
	rvol = DB_SIZE * 5;
      portc->volume[0] = vol;
      portc->volume[1] = rvol;
      return vol | (rvol << 16);
    }

  return OSS_EINVAL;
}

/*ARGSUSED*/
static int
vmix_outportc_vu (int dev, int ctrl, unsigned int cmd, int value)
{
  vmix_mixer_t *mixer = mixer_devs[dev]->vmix_devc;
  vmix_portc_t *portc;
  int val;

  if (mixer == NULL)
    return OSS_ENXIO;

  if (cmd != SNDCTL_MIX_READ)
    return OSS_EINVAL;

  if (ctrl < 0 || ctrl >= mixer->num_clientdevs)
    return OSS_ENXIO;

  portc = mixer->client_portc[ctrl];

  val = peak_cnv[portc->vu[0]] | (peak_cnv[portc->vu[1]] << 8);
  portc->vu[0] = portc->vu[1] = 0;

  return val;
}

static void
create_client_controls (void *vmix_mixer, int client_num)
{
  /*
   * Create the pcmN sliders
   */
  int group, err;
  vmix_mixer_t *mixer = vmix_mixer;
  int mixer_dev;
  char name[32];

  mixer_dev = mixer->output_mixer_dev;
  group = mixer->client_mixer_group;

  if (mixer_dev < 0 || mixer_dev >= num_mixers)
     return;

  sprintf (name, "@pcm%d", mixer->client_portc[client_num]->audio_dev);
  if ((err =
       mixer_ext_create_control (mixer_dev, group, client_num, vmix_outportc_vol,
				     MIXT_STEREOSLIDER16, name, DB_SIZE * 5,
				     MIXF_READABLE | MIXF_WRITEABLE |
				     MIXF_CENTIBEL | MIXF_PCMVOL)) < 0)
	return;

 if ((err =
      mixer_ext_create_control (mixer_dev, group, client_num, vmix_outportc_vu,
				     MIXT_STEREOPEAK, "", 144,
				     MIXF_READABLE | MIXF_DECIBEL)) < 0)
	return;
}

static int
create_output_controls (int mixer_dev)
{
  int  ctl;
  oss_mixext *ext;
  vmix_mixer_t *mixer = mixer_devs[mixer_dev]->vmix_devc;
  char tmp[32];

  /*
   * Misc vmix related mixer settings.
   */
      sprintf (tmp, "vmix%d-enable", mixer->instance_num);
      if ((ctl = mixer_ext_create_control (mixer_dev, 0, 510, vmix_outvol,
					   MIXT_ONOFF,
					   tmp, 2,
					   MIXF_READABLE | MIXF_WRITEABLE)) <
	  0)
	return ctl;

      sprintf (tmp, "vmix%d-rate", mixer->instance_num);
      if ((ctl = mixer_ext_create_control (mixer_dev, 0, 509, vmix_outvol,
					   MIXT_VALUE,
					   tmp, 500000,
					   MIXF_READABLE | MIXF_HZ)) <
	  0)
	return ctl;
      mixer_ext_set_description(mixer_dev, ctl, "Sample rate currently used by virtual mixer on this device.\n"
		      "Use vmixctl(1) command to change the rate.");

      if (mixer->max_channels>2)
	 {
	      sprintf (tmp, "vmix%d-channels", mixer->instance_num);
	      if ((ctl = mixer_ext_create_control (mixer_dev, 0, 511, vmix_outvol,
						   MIXT_ENUM,
						   tmp, 2,
						   MIXF_READABLE | MIXF_WRITEABLE)) <
		  0)
		return ctl;
	
	      mixer_ext_set_strings (mixer_dev, ctl,
				     "Stereo Multich", 0);
	 }

      sprintf (tmp, "vmix%d-src", mixer->instance_num);
      if ((ctl = mixer_ext_create_control (mixer_dev, 0, 512, vmix_outvol,
					   MIXT_ENUM,
					   tmp, 7,
					   MIXF_READABLE | MIXF_WRITEABLE)) <
	  0)
	return ctl;

      mixer_ext_set_strings (mixer_dev, ctl,
			     "Fast Low Medium High High+ Production OFF", 0);
      mixer_ext_set_description(mixer_dev, ctl, "Sample rate conversion quality used by the virtual mixer.\n"
         "\n"
         "Virtual mixer uses internally a fixed sampling rate that can be set\n"
         "using the 'vmixctl rate' command (usually 48 kHz by default). Applications\n"
         "that want to use different rates will be handled by performing automatic\n"
         "sample rate conversions (SRC) in software. This operation will consume\n"
         "some additional CPU time depending on the quality. The following\n"
         "alternatives are availabe:\n"
         "\n"
         "Fast:	Use fast linear interpolation algorithm (low quality).\n"
         "Low: Use slightly better linear interpolation\n"
         "Medium: Use an algorithm that provides good quality with moderate CPU load.\n"
         "High/High+/Production: Higher quality algorithms that consume more CPU resources.\n"
         "OFF: No sample rate conversions. Sample rate locked to the master rate.\n"
         "\n"
         "'Fast' will work best in most cases. Only users with high end audio\n"
         "cards and speakers should use the other settings.\n"
		      );
  	ext = mixer_find_ext (mixer_dev, ctl);
  	if (ext != NULL)
	{
		int i;
		
  		memset (ext->enum_present, 0, sizeof (ext->enum_present));
#ifdef CONFIG_OSS_VMIX_FLOAT
		ext->enum_present[0] |= 0x01; // "Fast" is always present
#endif
		ext->enum_present[0] |= 0x040; // As well as "OFF"
#if CONFIG_OSS_GRC_MAX_QUALITY > 7
#error CONFIG_OSS_GRC_MAX_QUALITY is out of range
#endif

		for (i=CONFIG_OSS_GRC_MIN_QUALITY; i <= CONFIG_OSS_GRC_MAX_QUALITY; i++)
			ext->enum_present[0] |= (1 << i);
	}

      /*
       * Create the vmix volume slider and peak meter to the top panel.
       */
  if (!(mixer->vmix_flags & VMIX_NOMAINVOL))
    {
      sprintf (tmp, "vmix%d-outvol", mixer->instance_num);
      if ((ctl = mixer_ext_create_control (mixer_dev, 0, 0, vmix_outvol,
					   MIXT_MONOSLIDER16,
					   tmp, DB_SIZE * 5,
					   MIXF_READABLE | MIXF_WRITEABLE |
					   MIXF_CENTIBEL | MIXF_PCMVOL)) < 0)
	return ctl;

      if (mixer->first_output_mixext == -1)
	mixer->first_output_mixext = ctl;
    }
  else
    mixer->play_engine.outvol = DB_SIZE * 5;

      sprintf (tmp, "vmix%d-outvu", mixer->instance_num);
      if ((ctl = mixer_ext_create_control (mixer_dev, 0, 1, vmix_outvol,
					   MIXT_STEREOPEAK,
					   tmp, 144,
					   MIXF_READABLE | MIXF_DECIBEL)) < 0)
	return ctl;

  if (mixer->first_output_mixext == -1)
     mixer->first_output_mixext = ctl;

  sprintf (tmp, "vmix%d", mixer->instance_num);
  if ((mixer->client_mixer_group = mixer_ext_create_group (mixer_dev, 0, tmp)) < 0)
    return mixer->client_mixer_group;

  return 0;
}

static int
create_input_controls (int mixer_dev)
{
  int err;
  vmix_mixer_t *mixer = mixer_devs[mixer_dev]->vmix_devc;
  char name[32];

  if (!(mixer->vmix_flags & VMIX_NOMAINVOL))
    {
      sprintf (name, "vmix%d-invol", mixer->instance_num);

      if ((err = mixer_ext_create_control (mixer_dev, 0, 0, vmix_invol,
					   MIXT_MONOSLIDER16,
					   name, DB_SIZE * 5,
					   MIXF_READABLE | MIXF_WRITEABLE |
					   MIXF_CENTIBEL | MIXF_PCMVOL)) < 0)
	return err;

      sprintf (name, "vmix%d-invu", mixer->instance_num);
      if ((err = mixer_ext_create_control (mixer_dev, 0, 1, vmix_invol,
					   MIXT_STEREOPEAK,
					   name, 144,
					   MIXF_READABLE | MIXF_DECIBEL)) < 0)
	return err;

      if (mixer->first_input_mixext == -1)
	mixer->first_input_mixext = err;
    }

  return 0;
}

static int
create_duplex_controls (int mixer_dev)
{
	int err;

	if ((err=create_output_controls (mixer_dev)) < 0)
	   return err;

	return create_input_controls (mixer_dev);
}

static int
vmix_process_chninfo (vmix_channel_map_t * output, oss_chninfo * input,
                      int maxchan)
{
  int i, val;

  for (i=0; i < sizeof (vmix_channel_map_t); i++)
    {
      val = (*input)[i];
      if ((val <= 0) || (val > maxchan) || (val == i+1))
       {
         (*input)[i] = 0;
         (*output)[i] = i;
       }
      else
       {
         (*output)[i] = val-1;
       }
    }
  return 0;
}

/*
 * Audio virtual device routines
 */

static int
vmix_set_rate (int dev, int arg)
{
  vmix_mixer_t *mixer = audio_engines[dev]->devc;
  vmix_portc_t *portc = audio_engines[dev]->portc;

  if (arg == 0)
    return portc->rate;

#ifdef CONFIG_OSS_VMIX_FLOAT
  if (portc->do_src)
    {
      /*
       * Stick with the master device rate if the requested rate is
       * not inside the range supported by the simple interpolation algorithm.
       * In that way the rate conversion will be handled by the audio core
       * with better quality.
       */
      if (arg > (mixer->play_engine.rate * 5) / 4)	/* At most 1.25*master_rate */
	arg = mixer->play_engine.rate;

/*
 * The simple linear interpolation algorithm cannot handle more than 2x rate
 * boosts reliably so don't permit them.
 */
      if (arg < mixer->play_engine.rate / 2)
	arg = mixer->play_engine.rate;

      return portc->rate = arg;
    }
#endif

  return portc->rate = mixer->play_engine.rate;
}

static short
vmix_set_channels (int dev, short arg)
{
  vmix_portc_t *portc = audio_engines[dev]->portc;

  if (portc->dev_type == DT_LOOP)
    {
      return portc->mixer->play_engine.channels;
    }

  if (arg == 0)
    {
      return portc->channels;
    }

  if (portc->open_mode & OPEN_WRITE)
    {
      if (arg > portc->mixer->play_engine.channels)
	arg = portc->mixer->play_engine.channels;
    }

  if (portc->open_mode & OPEN_READ)
    {
      if (arg > portc->mixer->record_engine.channels)
	arg = portc->mixer->record_engine.channels;
    }
  return portc->channels = arg;
}

/*ARGSUSED*/
static unsigned int
vmix_set_format (int dev, unsigned int arg)
{
  vmix_portc_t *portc = audio_engines[dev]->portc;

  if (portc->dev_type == DT_LOOP)
    return portc->mixer->play_engine.fmt;

  return portc->fmt = AFMT_S16_NE;
}

static int
export_names (oss_mixer_enuminfo * ei, char *names)
{
  int n = 1, p = 0;

  strcpy (ei->strings, names);
  names = ei->strings;

  ei->strindex[0] = 0;

  while (names[p] != 0)
    {
      if (names[p] == ' ')
	{
	  names[p] = 0;
	  ei->strindex[n++] = p + 1;
	}
      p++;
    }

  ei->nvalues = n;

  return 0;
}

static int
vmix_ioctl_override (int dev, unsigned int cmd, ioctl_arg arg)
{
/*
 * Redirect all mixer (also legacy) ioctl calls to the master device
 * if the master device driver has exported its ioctl override method.
 *
 * All other ioctl calls will be processed in normal way (by oss_audio_core.c)
 * because we return OSS_EAGAIN.
 */

  if ((((cmd >> 8) & 0xff) == 'M') || (((cmd >> 8) & 0xff) == 'X'))
  {
  	vmix_mixer_t *mixer = audio_engines[dev]->devc;
	adev_t *adev;

	adev=audio_engines[mixer->masterdev];

	if (adev->d->adrv_ioctl_override == NULL)
	   return OSS_EAGAIN;

	return adev->d->adrv_ioctl_override(adev->engine_num, cmd, arg);
  }

  return OSS_EAGAIN;
}

static int
vmix_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  vmix_portc_t *portc = audio_engines[dev]->portc;
  vmix_mixer_t *mixer = audio_engines[dev]->devc;
  int val, left, right;

  switch (cmd)
    {
    case SNDCTL_DSP_SETPLAYVOL:
      left = (*arg) & 0xff;
      if (left > 100)
	left = 100;
      left = (left * DB_SIZE * 5) / 100;
      portc->volume[0] = left;

      right = (*arg >> 8) & 0xff;
      if (right > 100)
	right = 100;
      right = (right * DB_SIZE * 5) / 100;
      portc->volume[1] = right;

      if (mixer->output_mixer_dev >= 0
	  && mixer->output_mixer_dev < num_mixers)
	mixer_devs[mixer->output_mixer_dev]->modify_counter++;
      return 0;
      break;

    case SNDCTL_DSP_GETPLAYVOL:
      left = (portc->volume[0] * 100) / (DB_SIZE * 5);
      right = (portc->volume[1] * 100) / (DB_SIZE * 5);
      *arg = left | (right << 8);
      return 0;
      break;

    case SNDCTL_DSP_GET_PLAYTGT:
      return *arg = portc->play_choffs / 2;
      break;

    case SNDCTL_DSP_SET_PLAYTGT:
      val = (*arg) * 2;
      if (val < 0)
	return OSS_EIO;
      if (val >= mixer->play_engine.channels)
	return OSS_EIO;
      portc->play_choffs = val;
      return *arg = val / 2;
      break;

    case SNDCTL_DSP_GET_PLAYTGT_NAMES:
      {
	oss_mixer_enuminfo *ei = (oss_mixer_enuminfo *) arg;
	int n, p, i;
	char *names = NULL;

	names = audio_engines[mixer->masterdev]->outch_names;

	memset (ei, 0, sizeof (*ei));

	n = mixer->play_engine.channels / 2;
	if (n < 1)
	  n = 1;
	ei->nvalues = n;
	p = 0;

	if (n <= 1)		/* Only one alternative */
	  {
	    ei->nvalues = 1;
	    ei->strindex[0] = 0;
	    sprintf (ei->strings, "default");

	  }
	else
	  {
	    /* Multiple alternatives */

	    if (names != NULL)
	      return export_names (ei, names);

	    for (i = 0; i < n; i++)
	      {
		ei->strindex[i] = p;

		sprintf (&ei->strings[p], "CH%d/%d", i * 2 + 1, i * 2 + 2);

		p += strlen (&ei->strings[p]) + 1;
	      }
	  }

	return 0;
      }
      break;

/*
 * Bypass the recording source and level calls to the master device if only one
 * recording client is active and if the master device is not a multi channel
 * one (probably professional device).
 */
    case SNDCTL_DSP_GETRECVOL:
    case SNDCTL_DSP_SETRECVOL:

      if (mixer->inputdev == -1)	/* No input device */
	return OSS_EINVAL;
      if (mixer->open_inputs < 2 && mixer->record_engine.channels <= 2)
	return oss_audio_ioctl (mixer->inputdev, NULL, cmd, arg);

      return OSS_EINVAL;
      break;

/*
 * Recording source selection. This can be done in two different ways depending
 * on the hardware capabilities: 
 *
 * 1) If the input master device has multiple channels then select one of
 *    the stereo pairs. It is likely that such device is a professional
 *    audio card that uses fixed inputs anyway.
 * 2) For stereo input only devices bypass the RECSRC ioctl calls to the
 *    actual hardware driver. However this cannot be done when multiple
 *    client applications have the same device opened for recording. In
 *    such situation switching the recording source would disturb other
 *    applications that already have recording going on.
 */

    case SNDCTL_DSP_GET_RECSRC:

      if (mixer->inputdev == -1)	/* No input device */
	return OSS_EINVAL;
      if (mixer->open_inputs < 2 && mixer->record_engine.channels <= 2)
	return oss_audio_ioctl (mixer->inputdev, NULL, cmd, arg);

      return *arg = portc->rec_choffs / 2;
      break;

    case SNDCTL_DSP_SET_RECSRC:

      if (mixer->inputdev == -1)	/* No input device */
	return OSS_EINVAL;

      if (mixer->open_inputs < 2 && mixer->record_engine.channels <= 2)
	return oss_audio_ioctl (mixer->inputdev, NULL, cmd, arg);

      val = (*arg) * 2;
      if (val < 0)
	return OSS_EIO;
      if (val >= mixer->record_engine.channels)
	return OSS_EIO;
      portc->rec_choffs = val;
      return *arg = val / 2;
      break;

    case SNDCTL_DSP_GET_RECSRC_NAMES:
      {
	oss_mixer_enuminfo *ei = (oss_mixer_enuminfo *) arg;
	int n, p, i;
	char *names = NULL;

	if (mixer->inputdev == -1)
	  return OSS_EINVAL;

	if (mixer->open_inputs < 2 && mixer->record_engine.channels <= 2)
	  return oss_audio_ioctl (mixer->inputdev, NULL, cmd, arg);

	names = audio_engines[mixer->inputdev]->inch_names;

	memset (ei, 0, sizeof (*ei));

	n = mixer->record_engine.channels / 2;
	if (n < 1)
	  n = 1;
	ei->nvalues = n;
	p = 0;

	if (n <= 1)		/* Only one alternative */
	  {
	    /* 
	     * It might be better to get the name of the current recording
	     * source from the master device. However for the time being
	     * we will return just "default".
	     */
	    ei->nvalues = 1;
	    ei->strindex[0] = 0;
	    sprintf (ei->strings, "default");
	  }
	else
	  {
	    /* Multiple alternatives */

	    if (names != NULL)
	      return export_names (ei, names);

	    for (i = 0; i < n; i++)
	      {
		ei->strindex[i] = p;

		sprintf (&ei->strings[p], "CH%d/%d", i * 2 + 1, i * 2 + 2);

		p += strlen (&ei->strings[p]) + 1;
	      }
	  }

	return 0;
      }
      break;
    }
  return OSS_EINVAL;
}

static void
vmix_halt_input (int dev)
{
  vmix_portc_t *portc = audio_engines[dev]->portc;
  vmix_mixer_t *mixer = audio_engines[dev]->devc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (mixer->mutex, flags);
  portc->trigger_bits &= PCM_ENABLE_INPUT;
  MUTEX_EXIT_IRQRESTORE (mixer->mutex, flags);
}

static void
vmix_halt_output (int dev)
{
  vmix_portc_t *portc = audio_engines[dev]->portc;
  vmix_mixer_t *mixer = audio_engines[dev]->devc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (mixer->mutex, flags);
  portc->trigger_bits &= PCM_ENABLE_OUTPUT;
  MUTEX_EXIT_IRQRESTORE (mixer->mutex, flags);
}

static void
vmix_reset (int dev)
{
  vmix_halt_input (dev);
  vmix_halt_output (dev);
}

static int
start_engines (vmix_mixer_t * mixer)
{
  int err;
  int trig;

  adev_t *adev_in, *adev_out;
  dmap_t *dmap_in, *dmap_out;

  if (mixer->masterdev_opened)
    {
      return 0;
    }

  mixer->master_finfo.mode = OPEN_WRITE;
  if (mixer->inputdev == mixer->masterdev)
    mixer->master_finfo.mode |= OPEN_READ;
  mixer->master_finfo.acc_flags = 0;

  adev_out = adev_in = audio_engines[mixer->masterdev];

  if (mixer->inputdev > -1)
    adev_in = audio_engines[mixer->inputdev];

  adev_out->cooked_enable = 0;

  if ((err =
       oss_audio_open_engine (mixer->masterdev, OSS_DEV_DSP,
			      &mixer->master_finfo, 1, OF_SMALLBUF,
			      NULL)) < 0)
    {
      return err;
    }

  dmap_out = adev_out->dmap_out;
  dmap_in = adev_in->dmap_in;
  adev_out->cooked_enable = 0;

  if (mixer->inputdev > -1 && mixer->inputdev != mixer->masterdev)
    {
      /*
       * Open input device
       */
      adev_in->cooked_enable = 0;
      mixer->input_finfo.mode = OPEN_READ;
      mixer->input_finfo.acc_flags = 0;
      if ((err =
	   oss_audio_open_engine (mixer->inputdev, OSS_DEV_DSP,
				  &mixer->input_finfo, 1, OF_SMALLBUF,
				  NULL)) < 0)
	{
	  oss_audio_release (mixer->masterdev, &mixer->master_finfo);
	  return err;
	}
      strcpy (adev_in->cmd, "VMIX_IN");
      strcpy (adev_in->label, "VMIX_IN");
      dmap_in = adev_in->dmap_in;
      adev_out->pid = 0;
      adev_out->cooked_enable = 0;

      vmix_setup_record_engine (mixer, adev_in, dmap_in);

      dmap_in->dma_mode = PCM_ENABLE_INPUT;
      trig = 0;
      oss_audio_ioctl (mixer->inputdev, NULL, SNDCTL_DSP_SETTRIGGER,
		       (ioctl_arg) & trig);
    }

  mixer->masterdev_opened = 1;
  strcpy (adev_out->cmd, "VMIX");
  strcpy (adev_out->label, "VMIX");
  adev_out->pid = 0;
  adev_out->cooked_enable = 0;

  vmix_setup_play_engine (mixer, adev_out, dmap_out);

  trig = 0;
  dmap_out->dma_mode = PCM_ENABLE_OUTPUT;

  oss_audio_ioctl (mixer->masterdev, NULL, SNDCTL_DSP_SETTRIGGER,
		   (ioctl_arg) & trig);
  trig = PCM_ENABLE_OUTPUT;
  if (mixer->masterdev == mixer->inputdev)
    trig |= PCM_ENABLE_INPUT;
  else
    {
      int trig2 = PCM_ENABLE_INPUT;
      if (mixer->inputdev > -1)
	if (oss_audio_ioctl (mixer->inputdev, NULL, SNDCTL_DSP_SETTRIGGER,
			     (ioctl_arg) & trig2) < 0)
	  {
	    cmn_err (CE_WARN, "Trigger (input) failed\n");
	  }
    }

  if (oss_audio_ioctl (mixer->masterdev, NULL, SNDCTL_DSP_SETTRIGGER,
		       (ioctl_arg) & trig) < 0)
    {
      cmn_err (CE_WARN, "Trigger failed\n");
    }

  return 0;
}

static void
stop_engines (vmix_mixer_t * mixer)
{
  oss_native_word flags;

  if (mixer->masterdev_opened)
    {
      adev_t *adev;
      dmap_t *dmap;

      adev = audio_engines[mixer->masterdev];
      dmap = adev->dmap_out;

      oss_audio_ioctl (mixer->masterdev, NULL, SNDCTL_DSP_HALT, 0);

      MUTEX_ENTER_IRQDISABLE (mixer->mutex, flags);
      dmap->audio_callback = NULL;

      dmap = adev->dmap_in;
      if (dmap != NULL)
	{
	  dmap->audio_callback = NULL;
	}
      MUTEX_EXIT_IRQRESTORE (mixer->mutex, flags);

      oss_audio_release (mixer->masterdev, &mixer->master_finfo);

      if (mixer->inputdev > -1 && mixer->inputdev != mixer->masterdev)
	{
	  adev = audio_engines[mixer->inputdev];
	  dmap = adev->dmap_in;

	  oss_audio_ioctl (mixer->inputdev, NULL, SNDCTL_DSP_HALT, 0);
	  MUTEX_ENTER_IRQDISABLE (mixer->mutex, flags);
	  dmap->audio_callback = NULL;
	  MUTEX_EXIT_IRQRESTORE (mixer->mutex, flags);
	  oss_audio_release (mixer->inputdev, &mixer->input_finfo);
	}

      mixer->masterdev_opened = 0;
    }
}

/*ARGSUSED*/
static int
vmix_open (int dev, int mode, int open_flags)
{
  adev_t *adev = audio_engines[dev];
  vmix_portc_t *portc = audio_engines[dev]->portc;
  vmix_mixer_t *mixer = portc->mixer;
  oss_native_word flags;
  int start = 0;

  if (mode & portc->disabled_modes)
    return OSS_EACCES;

  MUTEX_ENTER_IRQDISABLE (mixer->mutex, flags);

  portc->open_pending = 0;	/* Was set to 1 by vmix_create_client */

  if (portc->open_mode != 0)
    {
      MUTEX_EXIT_IRQRESTORE (mixer->mutex, flags);
      return OSS_EBUSY;
    }

  portc->open_mode = mode;
  portc->trigger_bits = 0;
  portc->play_mixing_func = NULL;
  portc->rec_mixing_func = NULL;
  portc->do_src = 0;
  portc->play_choffs = 0;	/* Left align */
  portc->rec_choffs = 0;	/* Left align */

#ifdef CONFIG_OSS_VMIX_FLOAT
  /*
   * For the time being always enable local linear interpolation to make
   * vmix devices to work faster.
   */
  if (mixer->src_quality == 0)
    portc->do_src = 1;
  else
#endif
    {
      adev->src_quality = mixer->src_quality;
      if (mixer->src_quality == 6)	/* SRC=OFF */
	adev->cooked_enable = 0;
      else
	{
	  if (adev->src_quality < 1)
	    adev->src_quality = 1;
	  else if (adev->src_quality > 5)
	    adev->src_quality = 5;
	}
    }
#ifdef CONFIG_OSS_VMIX_FLOAT
/*
 * Enable local src (linear interpolation) for mmap applications and SADA
 * support (sadasupport.c).
 */
  if (open_flags & (OF_DEVAUDIO | OF_MMAP))
    portc->do_src = 1;
#endif

  /*
   * However SRC is not supported for input
   */
  if (mode == PCM_ENABLE_INPUT)
    portc->do_src = 0;

  if (mixer->open_devices++ == 0)
    start = 1;

  if (mode & PCM_ENABLE_INPUT)
     mixer->open_inputs++;

  MUTEX_EXIT_IRQRESTORE (mixer->mutex, flags);

  if (start)
    {
      int err;

      if ((err = start_engines (mixer)) < 0)
	{
	  MUTEX_ENTER_IRQDISABLE (mixer->mutex, flags);
	  mixer->open_devices = 0;
	  mixer->open_inputs=0;
	  portc->open_mode = 0;
	  MUTEX_EXIT_IRQRESTORE (mixer->mutex, flags);
	  return err;
	}
    }

  return 0;
}

/*ARGSUSED*/
static void
vmix_close (int dev, int mode)
{
  vmix_portc_t *portc = audio_engines[dev]->portc;
  vmix_mixer_t *mixer = portc->mixer;
  oss_native_word flags;
  int stop = 0;

  MUTEX_ENTER_IRQDISABLE (mixer->mutex, flags);
  if (mixer->open_devices-- == 1)
    stop = 1;

  if (mode & PCM_ENABLE_INPUT)
     mixer->open_inputs--;

  portc->open_mode = 0;
  portc->trigger_bits = 0;
  portc->play_mixing_func = NULL;
  portc->rec_mixing_func = NULL;
  MUTEX_EXIT_IRQRESTORE (mixer->mutex, flags);

  if (stop)
    {
      stop_engines (mixer);
    }

}

/*ARGSUSED*/
static void
vmix_output_block (int dev, oss_native_word buf, int count,
		   int fragsize, int intrflag)
{
}

/*ARGSUSED*/
static void
vmix_start_input (int dev, oss_native_word buf, int count, int fragsize,
		  int intrflag)
{
}

static void
vmix_trigger (int dev, int state)
{
  vmix_portc_t *portc = audio_engines[dev]->portc;
  vmix_mixer_t *mixer = audio_engines[dev]->devc;

  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (mixer->mutex, flags);
  if (portc->open_mode & OPEN_WRITE)
    {
      portc->trigger_bits &= ~PCM_ENABLE_OUTPUT;
      portc->trigger_bits |= state & PCM_ENABLE_OUTPUT;
    }

  if (portc->open_mode & OPEN_READ)
    {
      portc->trigger_bits &= ~PCM_ENABLE_INPUT;
      portc->trigger_bits |= state & PCM_ENABLE_INPUT;
    }
  MUTEX_EXIT_IRQRESTORE (mixer->mutex, flags);
}

/*ARGSUSED*/
static int
vmix_prepare_for_input (int dev, int bsize, int bcount)
{
  vmix_portc_t *portc = audio_engines[dev]->portc;
  /* int bytes; */

  switch (portc->fmt)
    {
    case AFMT_S16_NE:
      portc->rec_mixing_func = vmix_rec_export_16ne;
      /* bytes = 2; */
      break;

    case AFMT_S16_OE:
      portc->rec_mixing_func = vmix_rec_export_16oe;
      /* bytes = 2; */
      break;

    case AFMT_S32_NE:
      portc->rec_mixing_func = vmix_rec_export_32ne;
      /* bytes = 4; */
      break;

    case AFMT_S32_OE:
      portc->rec_mixing_func = vmix_rec_export_32oe;
      /* bytes = 4; */
      break;

#ifdef CONFIG_OSS_VMIX_FLOAT
    case AFMT_FLOAT:
      portc->rec_mixing_func = vmix_rec_export_float;
      /* bytes = 4; */
      break;
#endif
    }

  portc->rec_dma_pointer = 0;
  return 0;
}

/*ARGSUSED*/
static int
vmix_prepare_for_output (int dev, int bsize, int bcount)
{
  vmix_portc_t *portc = audio_engines[dev]->portc;
  /* int bytes; */
#ifdef CONFIG_OSS_VMIX_FLOAT
  vmix_mixer_t *mixer = audio_engines[dev]->devc;

  memset (&portc->play_dma_pointer_src, 0, sizeof (portc->play_dma_pointer_src));	/* 0.0 */

  if (portc->rate != mixer->play_engine.rate)	/* Sample rate conversions needed */
    {
      switch (portc->fmt)
	{
	case AFMT_S16_NE:
	  portc->play_mixing_func = vmix_outmix_16ne_src;
	  /* bytes = 2; */
	  break;

	case AFMT_S16_OE:
	  portc->play_mixing_func = vmix_outmix_16oe_src;
	  /* bytes = 2; */
	  break;

	case AFMT_S32_NE:
	  portc->play_mixing_func = vmix_outmix_32ne_src;
	  /* bytes = 4; */
	  break;

	case AFMT_S32_OE:
	  portc->play_mixing_func = vmix_outmix_32oe_src;
	  /* bytes = 4; */
	  break;

	case AFMT_FLOAT:
	  portc->play_mixing_func = vmix_outmix_float_src;
	  /* bytes = 4; */
	  break;
	}
    }
  else
#endif
    {
      switch (portc->fmt)
	{
	case AFMT_S16_NE:
	  portc->play_mixing_func = vmix_outmix_16ne;
	  /* bytes = 2; */
	  break;

	case AFMT_S16_OE:
	  portc->play_mixing_func = vmix_outmix_16oe;
	  /* bytes = 2; */
	  break;

	case AFMT_S32_NE:
	  portc->play_mixing_func = vmix_outmix_32ne;
	  /* bytes = 4; */
	  break;

	case AFMT_S32_OE:
	  portc->play_mixing_func = vmix_outmix_32oe;
	  /* bytes = 4; */
	  break;

#ifdef CONFIG_OSS_VMIX_FLOAT
	case AFMT_FLOAT:
	  portc->play_mixing_func = vmix_outmix_float;
	  /* bytes = 4; */
	  break;
#endif
	}
    }

  portc->play_dma_pointer = 0;
  return 0;
}

/*ARGSUSED*/
static int
vmix_alloc_buffer (int dev, dmap_t * dmap, int direction)
{
  vmix_portc_t *portc = audio_engines[dev]->portc;

  /* Loopback devices share the DMA buffer of the master device */
  if (portc->dev_type == DT_LOOP)
    {
      adev_t *adev;
      adev = audio_engines[portc->mixer->masterdev];

      dmap->dmabuf_phys = 0;
      dmap->dmabuf = adev->dmap_out->dmabuf;
      dmap->buffsize = adev->dmap_out->buffsize;
      dmap->buffer_protected = 1;	/* Write protect flag for audio core */

      return 0;
    }

  if (dmap->dmabuf != NULL)
    return 0;

#ifdef ALLOW_BUFFER_MAPPING
  /*
   * Apps may use mmap() so allocate a buffer that is
   * suitable for it.
   */
  {
    int err;

    if ((err = oss_alloc_dmabuf (dev, dmap, direction)) < 0)
      return err;
  }
#else
#define MY_BUFFSIZE (64*1024)
  dmap->dmabuf_phys = 0;
  dmap->dmabuf = KERNEL_MALLOC (MY_BUFFSIZE);
  if (dmap->dmabuf == NULL)
    return OSS_ENOSPC;
  dmap->buffsize = MY_BUFFSIZE;
#endif

  return 0;
}

/*ARGSUSED*/
static int
vmix_free_buffer (int dev, dmap_t * dmap, int direction)
{
  vmix_portc_t *portc = audio_engines[dev]->portc;

  /* Loopback devices share the DMA buffer of the master device so don't free it */
  if (portc->dev_type == DT_LOOP)
    {
      dmap->dmabuf_phys = 0;
      dmap->dmabuf = NULL;
      dmap->buffsize = 0;
      return 0;
    }

  if (dmap->dmabuf == NULL)
    return 0;
#ifdef ALLOW_BUFFER_MAPPING
  oss_free_dmabuf (dev, dmap);
#else
  KERNEL_FREE (dmap->dmabuf);
#endif

  dmap->dmabuf = NULL;
  dmap->dmabuf_phys = 0;
  return 0;
}

/*ARGSUSED*/
static int
vmix_get_output_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  vmix_portc_t *portc = audio_engines[dev]->portc;

  return portc->play_dma_pointer;
}

/*ARGSUSED*/
static int
vmix_get_input_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
  vmix_portc_t *portc = audio_engines[dev]->portc;

  if (portc->dev_type == DT_LOOP)
    {
      int p;
      /* Return the write pointer of the master side */
      adev_t *adev;
      adev = audio_engines[portc->mixer->masterdev];

      p = (int) (adev->dmap_out->user_counter % adev->dmap_out->bytes_in_use);

      return p;
    }

  return portc->rec_dma_pointer;
}

static int
vmix_local_qlen (int dev)
{
  vmix_portc_t *portc;
  vmix_mixer_t *mixer;
  int samplesize;
  int len = 0;

  portc = audio_engines[dev]->portc;
  mixer = audio_engines[dev]->devc;

  samplesize = 1;
  if (portc->bits == AFMT_S16_NE)
    samplesize *= 2;
  samplesize *= portc->channels;

  /* Compute the number of samples in the physical device */
  oss_audio_ioctl (mixer->masterdev, NULL, SNDCTL_DSP_GETODELAY,
		   (ioctl_arg) & len);

  if (mixer->play_engine.bits == 16)
    len /= 2;			/* 16 bit samples */
  else
    len /= 4;			/* 32 bit samples */
  len /= mixer->play_engine.channels;

  /* Convert # of samples to local bytes */

  len *= portc->channels;

  if (portc->bits == AFMT_S16_NE)
    len *= 2;

  return len;
}

/*ARGSUSED*/
static void
vmix_setup_fragments (int dev, dmap_t * dmap, int direction)
{
/*
 * vmix_setup_fragments is used to force fragment/buffer parameters of
 * loopback devices to match the DMA buffer of the physical device.
 */
  vmix_portc_t *portc = audio_engines[dev]->portc;
  adev_t *adev;

  if (portc->dev_type != DT_LOOP)
    return;

  adev = audio_engines[portc->mixer->masterdev];

  /* Copy the buffering parameters */
  dmap->fragment_size = adev->dmap_out->fragment_size;
  dmap->nfrags = adev->dmap_out->nfrags;
  dmap->bytes_in_use = adev->dmap_out->bytes_in_use;
}

static audiodrv_t vmix_driver = {
  vmix_open,
  vmix_close,
  vmix_output_block,
  vmix_start_input,
  vmix_ioctl,
  vmix_prepare_for_input,
  vmix_prepare_for_output,
  vmix_reset,
  vmix_local_qlen,
  NULL,
  vmix_halt_input,
  vmix_halt_output,
  vmix_trigger,
  vmix_set_rate,
  vmix_set_format,
  vmix_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  vmix_alloc_buffer,
  vmix_free_buffer,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  vmix_get_input_buffer_pointer,
  vmix_get_output_buffer_pointer,
  NULL,
  vmix_setup_fragments
};

/*
 * Initialization support
 */

static void
relink_masterdev (vmix_mixer_t * mixer)
{
/*
 * Insert the VMIX devices to the engine search list of the master device.
 */
  adev_t *first_adev, *last_adev, *master_adev;
  int i, n = mixer->num_clientdevs;

  n = n - 1;
  if (n < 1)
    return;

  first_adev = audio_engines[mixer->client_portc[0]->audio_dev];
  last_adev = audio_engines[mixer->client_portc[n]->audio_dev];
  master_adev = audio_engines[mixer->masterdev];

/*
 * Relink client devices in the proper way.
 */

  for (i=0;i<mixer->num_clientdevs;i++)
  {
	  adev_t *adev = audio_engines[mixer->client_portc[i]->audio_dev];

	  if (i==mixer->num_clientdevs-1)
	     adev->next_out = NULL;
	  else
	     adev->next_out = audio_engines[mixer->client_portc[i+1]->audio_dev];
  }

  if (master_adev == NULL || master_adev->unloaded || !master_adev->enabled)
    {
      cmn_err (CE_WARN, "vmix: master_adev %d is not available\n", mixer->masterdev);
      cmn_err (CE_CONT, "master_adev=%p, unloaded=%d, enabled=%d\n", master_adev, master_adev->unloaded, master_adev->enabled);
      return;
    }

  last_adev->next_out = NULL;
  master_adev->next_out = first_adev;
}

static void
unlink_masterdev (vmix_mixer_t * mixer)
{
/*
 * Remove the VMIX devices from the engine search list of the master device.
 */
  adev_t *last_adev, *master_adev;
  int i, n = mixer->num_clientdevs;

  if (n < 1)
    return;

  if (n > MAX_CLIENTS)
     n = MAX_CLIENTS;

  for (i=0;i<n;i++)
  {
	  /*
	   * Mark all client engines as unloaded
	   */
	  adev_t *adev = audio_engines[mixer->client_portc[i]->audio_dev];

	  adev->unloaded = 1;
  }

  last_adev = audio_engines[mixer->client_portc[n-1]->audio_dev];
  master_adev = audio_engines[mixer->masterdev];

  if (master_adev == NULL)
    {
      cmn_err (CE_WARN, "master_adev == NULL\n");
      return;
    }

  master_adev->vmix_mixer=NULL;

  master_adev->next_out = last_adev->next_out;
  last_adev->next_out = NULL;
}

static void
relink_inputdev (vmix_mixer_t * mixer)
{
/*
 * Insert the VMIX devices to the engine search list of the input device.
 */
  adev_t *first_adev, *last_adev, *input_adev;
  int i, n = mixer->num_clientdevs;

/*
 * Relink client devices in the proper way.
 */

  for (i=0;i<mixer->num_clientdevs;i++)
  {
	  adev_t *adev = audio_engines[mixer->client_portc[i]->audio_dev];

	  if (i==mixer->num_clientdevs-1)
	     adev->next_in = NULL;
	  else
	     adev->next_in = audio_engines[mixer->client_portc[i+1]->audio_dev];
  }

  if (n < 1)
    return;

  n = n - 1;

  first_adev = audio_engines[mixer->client_portc[0]->audio_dev];
  last_adev = audio_engines[mixer->client_portc[n]->audio_dev];
  input_adev = audio_engines[mixer->inputdev];

  last_adev->next_in = NULL;
  input_adev->next_in = first_adev;
}

static void
unlink_inputdev (vmix_mixer_t * mixer)
{
/*
 * Remove the VMIX devices from the engine search list of the input device.
 */
  adev_t *last_adev, *input_adev;
  int n = mixer->num_clientdevs;

  if (n < 1)
    return;

  if (n > MAX_CLIENTS)
     n = MAX_CLIENTS;

  n = n - 1;

  last_adev = audio_engines[mixer->client_portc[n]->audio_dev];
  input_adev = audio_engines[mixer->inputdev];
  input_adev->vmix_mixer=NULL;

  input_adev->next_in = last_adev->next_in;
  last_adev->next_in = NULL;
}

static int
create_vmix_engine (vmix_mixer_t * mixer)
{
  vmix_portc_t *portc;
  int n;
  char tmp[128];
  adev_t *adev, *master_adev;
  int opts = ADEV_VIRTUAL | ADEV_DEFAULT | ADEV_VMIX;

  n = mixer->num_clientdevs;

  /*
   * ADEV_HIDDEN is used for the VMIX devices because they should not be
   * made visible to applications. The audio core will automatically
   * redirect applications opening the master device to use the
   * VMIX devices.
   *
   * However make the client devices visible if requested by vmixctl
   */ 
  if (!(mixer->attach_flags & VMIX_INSTALL_VISIBLE))
     {
     	opts |= ADEV_HIDDEN;

  	if (n > 0)
    	   opts |= ADEV_SHADOW;
     }

  if (mixer->masterdev == -1)
    return OSS_ENXIO;

  if (n + 1 >= MAX_CLIENTS) /* Cannot create more client engines */
     return OSS_EBUSY;

  /*
   * Other than the first instance are unlikely to be default the default
   * audio device of the system.
   */
  if (mixer->instance_num > 0)
    opts |= ADEV_SPECIAL;

  if ((portc = PMALLOC (mixer->osdev, sizeof (*portc))) == NULL)
    {
      cmn_err (CE_WARN, "Cannot allocate portc structure\n");
      return OSS_ENOMEM;
    }
  memset (portc, 0, sizeof (*portc));
  portc->open_pending = 1; /* Reserve this engine to the client it was created for */

  mixer->num_clientdevs++;

  portc->num = n;

  mixer->client_portc[n] = portc;

  if (mixer->inputdev == -1)
    opts |= ADEV_NOINPUT;
  else
    opts |= ADEV_DUPLEX;

  master_adev = audio_engines[mixer->masterdev];

  sprintf (tmp, "%s (vmix)", master_adev->name);

  if ((portc->audio_dev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
						mixer->osdev,
						mixer->master_osdev,
						tmp,
						&vmix_driver,
						sizeof (audiodrv_t),
						opts,
#ifdef CONFIG_OSS_VMIX_FLOAT
						AFMT_FLOAT |
#endif
						AFMT_S16_NE | AFMT_S32_NE,
						mixer, -1)) < 0)
    {
      return portc->audio_dev;
    }

  adev = audio_engines[portc->audio_dev];

  if (master_adev->d->adrv_ioctl_override != NULL)
     adev->d->adrv_ioctl_override = vmix_ioctl_override;

  adev->mixer_dev = mixer->output_mixer_dev;
  adev->portc = portc;
  adev->min_rate = mixer->play_engine.rate;
  adev->max_rate = mixer->play_engine.rate;
  adev->caps |= PCM_CAP_FREERATE | PCM_CAP_BATCH | PCM_CAP_MULTI;
  adev->min_channels = 2;
  adev->max_channels = mixer->play_engine.channels;

  adev->rate_source = audio_engines[mixer->masterdev]->rate_source;
  portc->dev_type = DT_OUT;
  portc->mixer = mixer;
  portc->volume[0] = DB_SIZE * 5;
  portc->volume[1] = DB_SIZE * 5;
  if (mixer->inputdev == -1)
    portc->disabled_modes = OPEN_READ;

  portc->fmt = AFMT_S16_NE;
  portc->bits = 16;
  portc->channels = 2;

#ifdef VDEV_SUPPORT
  /* Report device node of the master device */
  if (opts & ADEV_HIDDEN)
     strcpy (adev->devnode, master_adev->devnode);
  oss_add_audio_devlist (OPEN_READ | OPEN_WRITE, master_adev->audio_devfile);
#endif

  if (n == 0 && mixer->masterdev < num_audio_engines
      && audio_engines[mixer->masterdev] != NULL)
    {
      audio_engines[mixer->masterdev]->redirect_out = portc->audio_dev;

      if (mixer->inputdev > -1)
	{
	  audio_engines[mixer->inputdev]->redirect_in = portc->audio_dev;
	  audio_engines[mixer->masterdev]->redirect_in = portc->audio_dev;
	}
    }

  relink_masterdev (mixer);

  if (mixer->inputdev > -1)
    relink_inputdev (mixer);


  return portc->audio_dev;
}

static void
create_loopdev (vmix_mixer_t * mixer)
{
  vmix_portc_t *portc;
  int n;
  char tmp[128], nick[16];
  adev_t *adev;
  int opts = ADEV_VIRTUAL | ADEV_NOOUTPUT;

  if (mixer->masterdev == -1)
    return;

  if ((portc = PMALLOC (mixer->osdev, sizeof (*portc))) == NULL)
    {
      cmn_err (CE_WARN, "Cannot allocate portc structure\n");
      return;
    }
  memset (portc, 0, sizeof (*portc));

  n = mixer->num_loopdevs++;
  portc->num = n;
  mixer->loop_portc[n] = portc;

  if (n > 0)
    opts |= ADEV_SHADOW;

  adev = audio_engines[mixer->masterdev];

  sprintf (tmp, "%s (vmix) loopback record", adev->name);
  sprintf (nick, "loop%d", n);

  if ((portc->audio_dev = oss_install_audiodev_with_devname (OSS_AUDIO_DRIVER_VERSION,
						mixer->osdev,
						mixer->master_osdev,
						tmp,
						&vmix_driver,
						sizeof (audiodrv_t),
						opts,
						mixer->play_engine.fmt,
						mixer, -1,
						nick)) < 0)
    {
      return;
    }

  adev = audio_engines[portc->audio_dev];

  adev->mixer_dev = mixer->output_mixer_dev;
  adev->portc = portc;
  adev->min_rate = mixer->play_engine.rate;
  adev->max_rate = mixer->play_engine.rate;
  adev->min_channels = mixer->play_engine.channels;
  adev->max_channels = mixer->play_engine.channels;
  adev->caps |= PCM_CAP_FREERATE | PCM_CAP_HIDDEN  | PCM_CAP_BATCH | PCM_CAP_MULTI;
  portc->mixer = mixer;
  portc->dev_type = DT_LOOP;
  portc->disabled_modes = OPEN_WRITE;
  portc->fmt = AFMT_S16_NE;
  portc->bits = 16;
  portc->channels = 2;
  portc->volume[0] = DB_SIZE * 5;
  portc->volume[1] = DB_SIZE * 5;
}

static int
uninit_vmix_instance(vmix_mixer_t *mixer)
{
      if (mixer->master_osdev != NULL)
	{
	  MUTEX_CLEANUP (mixer->mutex);
	}

      if (mixer->masterdev < 0 || mixer->masterdev >= num_audio_engines)
	return OSS_ENXIO;

      if (!mixer->installed_ok)
	{
	  return OSS_EIO;
	}

      if (audio_engines[mixer->masterdev] == NULL)
	return OSS_ENXIO;

/*
 * Cleanup the engine redirection links
 */

      audio_engines[mixer->masterdev]->redirect_out = -1;

      if (mixer->inputdev > -1)
	{
	  audio_engines[mixer->inputdev]->redirect_in = -1;
	  audio_engines[mixer->masterdev]->redirect_in = -1;
	}
      unlink_masterdev (mixer);

      if (mixer->inputdev > -1 && mixer->inputdev != mixer->masterdev)
	{
	  unlink_inputdev (mixer);
	}

      return 0;
}

static int
check_masterdev (void *mx, int reattach)
{
  vmix_mixer_t *mixer = mx;
  adev_t *adev;

  if (mixer->masterdev < 0 || mixer->masterdev >= num_audio_engines)
    return OSS_ENXIO;

  adev = audio_engines[mixer->masterdev];
  DDB (cmn_err
       (CE_CONT, "Check masterdev eng=%d/%s\n", adev->engine_num,
	adev->name));

  /* Don't accept virtual devices other than loopback ones */
  if (adev->flags & ADEV_VIRTUAL && !(adev->flags & ADEV_LOOP))
    return OSS_EIO;

  if (adev->vmix_mixer != NULL) /* Already attached */
  {
     cmn_err(CE_CONT, "Vmix already attached to %s\n", adev->devnode);
     return OSS_EBUSY;
  }

  if (adev->flags & ADEV_NOOUTPUT)
    return OSS_EIO;

  if (adev->flags & ADEV_DISABLE_VIRTUAL)	/* Not compatible */
    return OSS_EIO;

  if (adev->vmix_flags & VMIX_DISABLED)	/* Not compatible */
    return OSS_EIO;

  if (adev->max_channels < 2)
    return OSS_EIO;

  if (!(adev->oformat_mask & SUPPORTED_FORMATS))
    return OSS_EIO;

  if (mixer->inputdev != -1 && mixer->inputdev != mixer->masterdev)
     if (audio_engines[mixer->inputdev]->vmix_mixer != NULL) /* Input master already driven */
     {
        cmn_err(CE_CONT, "Vmix already attached to %s\n", audio_engines[mixer->inputdev]->devnode);
	return OSS_EBUSY;
     }

/*
 * Good. Initialize all per-mixer variables
 */
  mixer->master_osdev = adev->master_osdev;
  adev->vmix_mixer = mixer;
  adev->prev_vmix_mixer = mixer;
//cmn_err(CE_CONT, "Calling MUTEX_INIT mast=%p, mixer=%p, mutex=%p\n", mixer->master_osdev, mixer,mixer->mutex);
  MUTEX_INIT (mixer->master_osdev, mixer->mutex, MH_DRV + 4);
//cmn_err(CE_CONT, "OK\n");

  DDB (cmn_err (CE_CONT, "Vmix masterdev=%d\n", mixer->masterdev));

/*
 * The device is OK. Next check for the input/duplex capability.
 */

  mixer->vmix_flags = adev->vmix_flags;
  mixer->max_channels = adev->max_channels;

  if (mixer->attach_flags & VMIX_INSTALL_NOINPUT)
     mixer->vmix_flags |= VMIX_NOINPUT;

  if (mixer->vmix_flags & VMIX_NOINPUT)
    mixer->inputdev = -1;
  else if (!(adev->flags & ADEV_NOINPUT) && (adev->flags & ADEV_DUPLEX))
    {
      mixer->inputdev = mixer->masterdev;
      DDB (cmn_err
	   (CE_CONT, "Vmix masterdev=%d shared for input\n",
	    mixer->masterdev));
    }

/*
 * At this point we know the input and output master devices so it's the time
 * to create the virtual audio devices.
 */

  adev = audio_engines[mixer->masterdev];

  if (mixer->osdev->first_mixer >= 0)
    {
      mixer->output_mixer_dev = mixer->osdev->first_mixer;
      mixer->input_mixer_dev = mixer->osdev->first_mixer;
    }

  if (mixer->inputdev != -1 && mixer->inputdev != mixer->masterdev)
    {
      adev = audio_engines[mixer->inputdev];

      adev->vmix_mixer = mixer;
      adev->prev_vmix_mixer = mixer;

      if (adev->mixer_dev != -1)
	{
	  mixer->input_mixer_dev = adev->mixer_dev;
	}
    }

  /*
   * Warm up the engines so that client device creation knows the defaults
   */
  start_engines (mixer);
  stop_engines (mixer);

  mixer->installed_ok = 1;


      if (!reattach)
      {
	  if (mixer->output_mixer_dev > -1)
	    {
		if (mixer->output_mixer_dev == mixer->input_mixer_dev)
		   {
	              mixer_ext_set_vmix_init_fn (mixer->output_mixer_dev,
					  create_duplex_controls, 20, mixer);
		   }
		else
		   {
	              mixer_ext_set_vmix_init_fn (mixer->output_mixer_dev,
					  create_output_controls, 20, mixer);
		   }
	    }
	
	  if (mixer->output_mixer_dev != mixer->input_mixer_dev)
	  if (mixer->inputdev >= 0 &&
	      mixer->input_mixer_dev > -1)
	    {
	      mixer_ext_set_vmix_init_fn (mixer->input_mixer_dev,
					  create_input_controls, 10, mixer);
	    }
      }
/*
 * Crate one client in advance so that that SNDCTL_AUDIOINFO can provide proper info.
 */
  if (!(mixer->attach_flags & VMIX_INSTALL_NOPREALLOC))
     {
	int cl, n=4;

	if ((mixer->attach_flags & 0xff) != 0) /* Number of clients given */
	   {
		   n = mixer->attach_flags & 0xff;
		   if (n<1) n=1;
		   if (n>=MAX_CLIENTS) n=MAX_CLIENTS-1; /* TODO: Why n=MAX_CLIENTS doesn't work? */
	   }
	
	for (cl=0;cl<n;cl++)
    	    vmix_create_client (mixer);

	/*
	 * Mark the engines to be free for use
	 */
	for (cl=0;cl<mixer->num_clientdevs;cl++)
  	    mixer->client_portc[cl]->open_pending = 0;
     }

  if (vmix_loopdevs>0)
  {
     int i;

     for (i=0;i<vmix_loopdevs;i++)
         create_loopdev (mixer);
  }

  DDB (cmn_err (CE_CONT, "Master dev %d is OK\n", adev->engine_num));

  return 0;
}

int
vmix_attach_audiodev(oss_device_t *osdev, int masterdev, int inputdev, unsigned int attach_flags)
{
/*
 * Purpose: Create a vmix instance for an audio device.
 *
 * This function will be called by OSS drivers after installing the audio devices. It will create
 * an vmix instance for the device.
 *
 * Parameters:
 *
 * osdev:		The osdev structure of the actual hardware device.
 * masterdev:		The audio engine number of the master (playback or duplex) device.
 * inputdev:		Input master device (if different than masterdev). Value of -1 means that the
 * 			masterdev device should also be used as the input master device (if it supports
 * 			input).
 * attach_flags:	Flags like VMIX_INSTALL_NOPREALLOC.
 */

  vmix_mixer_t *mixer=NULL;
  int reattach=0;
  int err;
  int i;

  if (vmix_disabled) /* Vmix not available in the system */
     return OSS_EIO;

  /*
   * If the vmix_no_autoattach option is set in osscore.conf then attach
   * vmix only when 'vmixctl attach' is executed manually (VMIX_INSTALL_MANUAL).
   */
  if (vmix_no_autoattach && !(attach_flags & VMIX_INSTALL_MANUAL))
     return 0;

  if (flat_device_model)
  {
     attach_flags |= VMIX_INSTALL_VISIBLE;
     attach_flags = (attach_flags & ~0xff) | 8; /* Precreate 8 client engines */
  }

  if (audio_engines[masterdev]->prev_vmix_mixer != NULL)
     {
	     mixer = audio_engines[masterdev]->prev_vmix_mixer;

	     if (mixer->attach_flags != attach_flags) /* Not compatible */
		mixer=NULL;
     }

  if (mixer == NULL)
     {
	  if ((mixer = PMALLOC (osdev, sizeof (*mixer))) == NULL)
	    {
	      cmn_err (CE_CONT, "Cannot allocate memory for instance descriptor\n");
	      return OSS_ENOMEM;
	    }
	
	  memset (mixer, 0, sizeof (*mixer));
  	  mixer->instance_num = num_instances++;
  	  if (mixer->instance_num > 0)
    	     mixer->osdev = osdev_clone (osdev, mixer->instance_num);
     }
  else
     {
	     relink_masterdev (mixer);
	
	     if (mixer->inputdev > -1)
	        relink_inputdev (mixer);
	
	     reattach=1;
     }

  mixer->osdev = osdev;
  mixer->first_input_mixext = -1;
  mixer->first_output_mixext = -1;
  mixer->src_quality = 0;

  mixer->output_mixer_dev = -1;
  mixer->input_mixer_dev = -1;

  /*
   * Mixer default levels
   */
  mixer->play_engine.outvol = DB_SIZE * 5;
  mixer->record_engine.outvol = DB_SIZE * 5;
#ifndef CONFIG_OSS_VMIX_FLOAT
  mixer->play_engine.outvol -= 3;	/* For overflow protection */
#endif
  for (i = 0; i < sizeof (vmix_channel_map_t); i++)
    mixer->play_engine.channel_order[i] = i;

  mixer->masterdev = masterdev;
  mixer->inputdev = inputdev;
  mixer->rate = 48000;
  mixer->attach_flags = attach_flags;

  DDB (cmn_err (CE_CONT, "Create instance %d\n", num_instances));
  DDB (cmn_err (CE_CONT, "vmix_masterdev=%d\n", masterdev));
  DDB (cmn_err (CE_CONT, "vmix_inputdev=%d\n", inputdev));
  DDB (cmn_err (CE_CONT, "vmix_rate=%d\n", mixer->rate));
  DDB (cmn_err (CE_CONT, "\n"));

  /*
   * Insert the newly created mixer to the mixer list.
   */
  mixer->next = mixer_list;
  mixer_list = mixer;

  if (masterdev >= 0)
    {
      if (masterdev >= num_audio_engines)
	{
	  return OSS_ENXIO;
	}

      masterdev = mixer->masterdev;

      if (mixer->inputdev >= 0)
	{
	  if (inputdev >= num_audio_engines)
	    inputdev = mixer->inputdev = -1;
	}

      if ((err=check_masterdev (mixer, reattach))<0)
	{
	  cmn_err (CE_CONT, "Vmix instance %d: Master device %d is not compatible with vmix (error %d)\n",
		   mixer->instance_num + 1, mixer->masterdev, err);
	  return err;
	}

      return 0;
    }

  return OSS_EIO;
}

int
vmix_detach_audiodev(int masterdev)
{
/*
 * Purpose: Detach the vmix subsystem from the audio device.
 *
 * Most drivers don't call this since vmix instances will be automatically detached when the 
 * master device is removed. However drivers that support dynamically created/deleted devices must
 * call this when a device is deleted.
 *
 * Paramaters:
 *
 * masterdev:		The audio engine number of the master device (same as in vmix_attach_audiodev).
 */

	vmix_mixer_t *mixer;

	if (masterdev<0 || masterdev>=num_audio_engines)
	   return OSS_ENXIO;

	mixer = audio_engines[masterdev]->vmix_mixer;

	if (mixer==NULL) /* Not attached */
	   return 0;

        return uninit_vmix_instance(mixer);
}

int
vmix_set_master_rate(int masterdev, int rate)
{
/*
 * Purpose: Set the master sampling rate of given vmix instance.
 *
 * Paramaters:
 *
 * masterdev:		The audio engine number of the master device (same as in vmix_attach_audiodev).
 * rate:		The requested new sampling rate.
 */

	vmix_mixer_t *mixer;

	if (rate < 4000 || rate > 200000)
	   return OSS_EDOM;

	if (masterdev<0 || masterdev>=num_audio_engines)
	   return OSS_ENXIO;

	mixer = audio_engines[masterdev]->vmix_mixer;

	if (mixer==NULL)
	   return OSS_ENXIO;

	mixer->rate = rate;

	return 0;
}

int
vmix_set_channel_map (int masterdev, void * map)
{
  vmix_mixer_t *mixer;

  if (masterdev < 0 || masterdev >= num_audio_engines)
    return OSS_ENXIO;

  mixer = audio_engines[masterdev]->vmix_mixer;

  if (mixer == NULL)
    return OSS_EPERM;

  return vmix_process_chninfo (&mixer->play_engine.channel_order, map,
                               mixer->play_engine.channels);
}

int
vmix_create_client(void *mixer_)
{
  int engine_num=-1, i;
  oss_native_word flags;
  vmix_portc_t *portc;
  vmix_mixer_t *mixer = mixer_;

  if (mixer->disabled) /* Vmix is disabled for the time being */
     return OSS_ENXIO;

/*
 * First check if any of the already created engines is free and available for use.
 */

  MUTEX_ENTER_IRQDISABLE (mixer->mutex, flags);
  for (i=0;i<mixer->num_clientdevs;i++)
  {
	  portc = mixer->client_portc[i];

	  if (portc->open_mode != 0 || portc->open_pending)
	     continue;

	  portc->open_pending = 1;
	  engine_num = portc->audio_dev;
	  break;
  }
  MUTEX_EXIT_IRQRESTORE (mixer->mutex, flags);
 
/*
 * Create a new engine and use it
 */

  if (engine_num < 0) /* Engine not allocated yet */
     {
	  if ((engine_num = create_vmix_engine (mixer))<0)
	    {
	        cmn_err(CE_WARN, "Failed to create a vmix engine, error=%d\n", engine_num);
	        return engine_num;
	     }

  	   portc = audio_engines[engine_num]->portc;
	   create_client_controls (mixer, portc->num);
     }

  portc = audio_engines[engine_num]->portc;

  /* portc->open_pending = 1; // This was done already by create_vmix_engine() */
  return engine_num;
}

void
vmix_core_init (oss_device_t *osdev)
{
#ifdef CONFIG_OSS_VMIX_FLOAT
  int check;
/*
 * Check that the processor is compatible with vmix (has proper FP support).
 */

  if ((check = oss_fp_check ()) <= 0)
    {
      vmix_disabled = 1;
      cmn_err (CE_WARN,
	       "This processor architecture is not compatible with vmix (info=%d) - Not enabled.\n",
	       check);
      return;
    }
#endif
}

void
vmix_core_uninit (void)
{
	vmix_mixer_t *mixer = mixer_list;
	int n = 0;

  while (mixer != NULL && n++ < num_instances)
    {
      uninit_vmix_instance(mixer);
      mixer = mixer->next;
    }

  mixer_list = NULL; /* Everything removed */
}

void
vmix_change_devnames(void *vmix_mixer, const char *name)
{
/*
 * Change audio device names of all client engines.
 */
  vmix_mixer_t *mixer = vmix_mixer;
  int i;

  for (i=0;i<mixer->num_clientdevs;i++)
  {
	  adev_t *adev = audio_engines[mixer->client_portc[i]->audio_dev];

	  sprintf(adev->name, "%s (vmix)", name);
  }
}
