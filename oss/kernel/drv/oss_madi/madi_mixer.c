/*
 * Purpose: Mixer/control panel  for RME MADI and AES32 audio interfaces
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

#include "oss_madi_cfg.h"
#include "oss_pci.h"
#include "madi.h"

extern int madi_maxchannels;

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

void
madi_write_gain (madi_devc_t * devc, unsigned int chn, unsigned int src,
		 unsigned short value)
{
/*
 * Write gain value to a mixer register. Parameter chn selects the output
 * channel. Parameter src is the signal source. If (src & SRC_PLAY) then set
 * the audio (DMA) playback volume (pcm[src]->out[chn]). Otherwise set
 * the input[src]->out[chn] volume
 */

  if (chn >= MAX_CHANNELS || src >= (SRC_PLAY + MAX_CHANNELS))
    return;

  madi_write (devc, MADI_mixerStart + (src + 128 * chn) * 4, value);
  devc->mixer_values[chn][src] = value;
}

int
madi_read_gain (madi_devc_t * devc, unsigned int chn, unsigned int src)
{
  if (chn >= MAX_CHANNELS || src >= (SRC_PLAY + MAX_CHANNELS))
    return 0;

  return devc->mixer_values[chn][src];
}

static int
madi_mixer_ioctl (int dev, int audiodev, unsigned int cmd, ioctl_arg arg)
{
#if 0
/*
 * Dummy mixer. Some broken applications require this.
 */

  if (cmd == SOUND_MIXER_READ_CAPS)
    return *arg = SOUND_CAP_NOLEGACY;

  if (cmd == SOUND_MIXER_READ_DEVMASK ||
      cmd == SOUND_MIXER_READ_RECMASK || cmd == SOUND_MIXER_READ_RECSRC)
    return *arg = 0;

  if (cmd == SOUND_MIXER_READ_VOLUME || cmd == SOUND_MIXER_READ_PCM)
    return *arg = 100 | (100 << 8);
  if (cmd == SOUND_MIXER_WRITE_VOLUME || cmd == SOUND_MIXER_WRITE_PCM)
    return *arg = 100 | (100 << 8);
#endif
  return OSS_EINVAL;
}

 /*ARGSUSED*/ static int
get_peak (int dev, int ctrl, unsigned int cmd, int value)
{
  madi_devc_t *devc = mixer_devs[dev]->devc;
  unsigned int v;

  if (cmd != SNDCTL_MIX_READ)
    return OSS_EINVAL;

  v = madi_read (devc, ctrl) >> 24;	// Left channel
  value = peak_cnv[v & 0xff];

  v = madi_read (devc, ctrl + 4) >> 24;	// Right channel
  value |= peak_cnv[v & 0xff] << 8;

  return value;
}

 /*ARGSUSED*/ static int
vol_slider (int dev, int ctrl, unsigned int cmd, int value)
{
  madi_devc_t *devc = mixer_devs[dev]->devc;
  unsigned int v;
  int chn, src;

  chn = (ctrl >> 8) & 0xff;
  src = ctrl & 0xff;

  if (chn >= MAX_CHANNELS || src >= 2 * MAX_CHANNELS)
    return OSS_EINVAL;

  if (cmd == SNDCTL_MIX_READ)
    {
      // Left channel
      v = devc->mixer_values[chn][src];
      if (v > 0)
	v--;
      value = v & 0xffff;

      // Right channel
      v = devc->mixer_values[chn + 1][src + 1];
      if (v > 0)
	v--;
      value |= (v & 0xffff) << 16;

      return value;
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
      // Left channel
      v = value & 0xffff;
      if (v > UNITY_GAIN)
	v = UNITY_GAIN;
      if (v > 0)
	v++;
      madi_write_gain (devc, chn, src, v);

      // Right channel
      v = (value >> 16) & 0xffff;
      if (v > UNITY_GAIN)
	v = UNITY_GAIN;
      if (v > 0)
	v++;
      madi_write_gain (devc, chn + 1, src + 1, v);

      return value;
    }

  return OSS_EINVAL;
}

 /*ARGSUSED*/ static int
madi_set_ctl (int dev, int ctrl, unsigned int cmd, int value)
{
  madi_devc_t *devc = mixer_devs[dev]->devc;
  unsigned int status;

  if (cmd == SNDCTL_MIX_READ)
    switch (ctrl)
      {
      case 0:			/* Buffer size */
	return (devc->cmd & MADI_LatencyMask) >> 1;
	break;

      case 1:			/* Sync locked status */
	status = madi_read (devc, MADI_status);
	return !!(status & MADI_LockStatus);
	break;

      case 2:			/* Input source (Optical/Coax) */
	return !!(devc->cmd & MADI_InputSrc0);
	break;

      case 3:			/* Clock source */
	return !(devc->cmd & MADI_ClockModeMaster);
	break;

      case 4:			/* Preferred sync reference */
	return !!(devc->cmd & MADI_SyncRef_MADI);
	break;

      case 5:			/* Safe Mode (auto input) */
	return !!(devc->cmd & MADI_AutoInput);
	break;
      }

  if (cmd == SNDCTL_MIX_WRITE)
    switch (ctrl)
      {
      case 0:			/* Buffer size */
	if (devc->busy_play_channels || devc->busy_rec_channels) /* Device busy */
	   return (devc->cmd & MADI_LatencyMask) >> 1;

	if (value < 0 || value > 7)
	  return OSS_EINVAL;

	devc->cmd &= ~MADI_LatencyMask;
	devc->cmd |= value << 1;
	madi_control (devc, devc->cmd);
	return value;
	break;

      case 2:			/* Input source (Optical/Coax) */
	value = !!value;
	if (value)
	   madi_control (devc, devc->cmd | MADI_InputSrc0);
	else
	   madi_control (devc, devc->cmd & ~MADI_InputSrc0);
	return value;
	break;

      case 3:			/* Clock source */
	value = !!value;
	if (!value)
	   madi_control (devc, devc->cmd | MADI_ClockModeMaster);
	else
	   madi_control (devc, devc->cmd & ~MADI_ClockModeMaster);
	return value;
	break;

      case 4:			/* Preferred sync reference */
	value = !!value;
	if (value)
	   madi_control (devc, devc->cmd | MADI_SyncRef_MADI);
	else
	   madi_control (devc, devc->cmd & ~MADI_SyncRef_MADI);
	return value;
	break;

      case 5:			/* Safe Mode (auto input) */
	if (value)
	   madi_control (devc, devc->cmd | MADI_AutoInput);
	else
	   madi_control (devc, devc->cmd & ~MADI_AutoInput);
	return value;
	break;
      }

  return OSS_EINVAL;
}

static int
madi_mix_init (int dev)
{
  int i;
  madi_devc_t *devc = mixer_devs[dev]->devc;
  char tmp[16];
  int err, ctl;
  int group = 0;

/*
 * Common controls
 */

  if ((ctl = mixer_ext_create_control (dev, 0,
				       3,
				       madi_set_ctl,
				       MIXT_ENUM, "clock", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;
  mixer_ext_set_strings (dev, ctl, "master autosync", 0);

  if (devc->model == MDL_MADI)
  {
  if ((ctl = mixer_ext_create_control (dev, 0,
				       4,
				       madi_set_ctl,
				       MIXT_ENUM, "syncref", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;
  mixer_ext_set_strings (dev, ctl, "WordClock MADI-in", 0);
  }

  if (devc->model == MDL_MADI)
  {
  if ((ctl = mixer_ext_create_control (dev, 0,
				       2,
				       madi_set_ctl,
				       MIXT_ENUM, "input", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;
  mixer_ext_set_strings (dev, ctl, "optical coax", 0);
  }

  if (devc->model == MDL_MADI)
  {
  if ((ctl = mixer_ext_create_control (dev, 0,
				       5,
				       madi_set_ctl,
				       MIXT_ONOFF, "safe-input", 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;
  }

  if ((ctl = mixer_ext_create_control (dev, 0,
				       0,
				       madi_set_ctl,
				       MIXT_ENUM, "buffer", 8,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;
  mixer_ext_set_strings (dev, ctl, "64 128 256 512 1k 2k 4k 8k", 0);

  if ((ctl = mixer_ext_create_control (dev, 0,
				       1,
				       madi_set_ctl,
				       MIXT_VALUE, "lock", 2,
				       MIXF_OKFAIL | MIXF_READABLE |
				       MIXF_POLL)) < 0)
    return ctl;

  /*
   * Input mixer
   */

  for (i = 0; i < madi_maxchannels; i += 2)
    {
      int offs = MADI_inpeaks + i * 4;
      int ctl;

      if (!(i % 32))
	if ((group = mixer_ext_create_group (dev, 0, "in")) < 0)
	  return group;

      ctl = (i << 8) | (SRC_IN | i);

      sprintf (tmp, "%d-%d", i + 1, i + 2);
      if ((err =
	   mixer_ext_create_control (dev, group, ctl, vol_slider,
				     MIXT_STEREOSLIDER16, tmp, UNITY_GAIN - 1,
				     MIXF_READABLE | MIXF_WRITEABLE)) < 0)
	return err;
      if ((err =
	   mixer_ext_create_control (dev, group, offs, get_peak,
				     MIXT_STEREOPEAK, "-", 144,
				     MIXF_READABLE | MIXF_DECIBEL)) < 0)
	return err;
    }

  /*
   * Playback (pcm) mixer
   */
  for (i = 0; i < devc->num_outputs; i++)
    {
      int offs = MADI_playpeaks;
      int ctl, ch;
      madi_portc_t *portc = devc->out_portc[i];

      ch = portc->channel;
      offs += ch * 4;

      if (!(i % 16))
	if ((group = mixer_ext_create_group (dev, 0, "play")) < 0)
	  return group;

      ctl = (ch << 8) | (SRC_PLAY | ch);

      if ((err =
	   mixer_ext_create_control (dev, group, offs, get_peak,
				     MIXT_STEREOPEAK, "-", 144,
				     MIXF_READABLE | MIXF_DECIBEL)) < 0)
	return err;

      sprintf (tmp, "@pcm%d", portc->audio_dev);
      if ((err =
	   mixer_ext_create_control (dev, group, ctl, vol_slider,
				     MIXT_STEREOSLIDER16, tmp, UNITY_GAIN - 1,
				     MIXF_READABLE | MIXF_WRITEABLE)) < 0)
	return err;
    }

  /*
   * Output mixer
   */
  for (i = 0; i < madi_maxchannels; i += 2)
    {
      int offs = MADI_outpeaks + i * 4;

      if (!(i % 64))
	if ((group = mixer_ext_create_group (dev, 0, "out")) < 0)
	  return group;

      sprintf (tmp, "%d-%d", i + 1, i + 2);
      if ((err =
	   mixer_ext_create_control (dev, group, offs, get_peak,
				     MIXT_STEREOPEAK, tmp, 144,
				     MIXF_READABLE | MIXF_DECIBEL)) < 0)
	return err;
    }

#if 0
  /*
   * Analog monitor
   */
  {
    int offs = MADI_outpeaks + MONITOR_CH * 4;

    if ((err =
	 mixer_ext_create_control (dev, group, offs, get_peak,
				   MIXT_STEREOPEAK, "mon", 144,
				   MIXF_READABLE | MIXF_DECIBEL)) < 0)
      return err;
  }
#endif

  return 0;
}

static mixer_driver_t madi_mixer_driver = {
  madi_mixer_ioctl
};

int
madi_install_mixer (madi_devc_t * devc)
{
  int my_mixer;

  if ((my_mixer = oss_install_mixer (OSS_MIXER_DRIVER_VERSION,
				     devc->osdev,
				     devc->osdev,
				     devc->name,
				     &madi_mixer_driver,
				     sizeof (mixer_driver_t), devc)) < 0)
    return my_mixer;

  mixer_devs[my_mixer]->priority = -1;	/* Don't use as the default mixer */

  return my_mixer;
}

void
madi_activate_mixer (madi_devc_t * devc)
{
  mixer_ext_set_init_fn (devc->mixer_dev, madi_mix_init, madi_maxchannels*5 + 30);
  touch_mixer (devc->mixer_dev);
}
