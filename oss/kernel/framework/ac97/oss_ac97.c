/*
 * Purpose: AC97 codec support library
 *
 * This source file contains common AC97 codec/mixer related code that is
 * used by all drivers for AC97 based devices.
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

#include "ac97.h"

extern int ac97_amplifier;	/* From oss_core_options.c */
extern int ac97_recselect;	/* From oss_core_options.c */

/*
 * LINE1 == AUX
 */

#define MIXER_DEVS		(SOUND_MASK_LINE1 | SOUND_MASK_VIDEO | \
				 SOUND_MASK_MIC | SOUND_MASK_VOLUME | \
				 SOUND_MASK_LINE | SOUND_MASK_SPEAKER | \
				 SOUND_MASK_CD | SOUND_MASK_LINE | \
				 SOUND_MASK_PHONE | SOUND_MASK_MONO | \
				 SOUND_MASK_PCM | SOUND_MASK_IGAIN)

#define EXTRA_DEVS		(SOUND_MASK_REARVOL | SOUND_MASK_CENTERVOL | \
				 SOUND_MASK_SIDEVOL)

#define REC_DEVS		(SOUND_MASK_LINE1 | SOUND_MASK_MIC | \
				 SOUND_MASK_PHONE | SOUND_MASK_CD | \
				 SOUND_MASK_LINE | SOUND_MASK_VOLUME | \
				 SOUND_MASK_MONO | SOUND_MASK_VIDEO )

#define STEREO_DEVS		((MIXER_DEVS|EXTRA_DEVS) & ~(SOUND_MASK_MIC| \
				     SOUND_MASK_SPEAKER|SOUND_MASK_BASS| \
				     SOUND_MASK_TREBLE | SOUND_MASK_MONO))


static int default_mixer_levels[32] = {
  0x4b4b,			/* Master Volume */
  0x3232,			/* Bass */
  0x3232,			/* Treble */
  0x4b4b,			/* FM */
  0x4b4b,			/* PCM */
  0x8000,			/* PC Speaker */
  0x2020,			/* Ext Line */
  0x0000,			/* Mic */
  0x4b4b,			/* CD */
  0x0000,			/* Recording monitor */
  0x4b4b,			/* Second PCM */
  0x4b4b,			/* Recording level */
  0x4b4b,			/* Input gain */
  0x4b4b,			/* Output gain */
  0x2020,			/* Line1 */
  0x2020,			/* Line2 */
  0x1515,			/* Line3 (usually line in) */
  0x0101,			/* Digital1 */
  0x0000,			/* Digital2 */
  0x0000,			/* Digital3 */
  0x0000,			/* Phone In */
  0x4b4b,			/* Mono/Phone out */
  0x0000,			/* Video */
  0x0000,			/* Radio */
  0x0000,			/* Depth */
  0x4b4b,			/* Rear */
  0x4b4b,			/* Center */
  0x4b4b			/* Surround */
};

static int nr_ac97 = 0;
extern int ac97_amplifier, ac97_recselect;

static unsigned short
codec_read (ac97_devc * devc, int reg)
{
  return devc->read (devc->host_parms, reg) & 0xffff;
}

static int
codec_write (ac97_devc * devc, int reg, unsigned short data)
{
  return devc->write (devc->host_parms, reg, data);
}


static int
ac97_set_recmask (ac97_devc * devc, int pmask)
{
  int mask = pmask & devc->recmask;
  int sel = 0;

  mask = mask & (mask ^ devc->recdevs);	/* Find out the changed bits */

  if (!mask)			/* No change */
    return devc->recdevs;

  devc->recdevs = mask;

  switch (mask)
    {
    case SOUND_MASK_MIC:
      sel = 0;
      break;
    case SOUND_MASK_CD:
      sel = 1;
      break;
    case SOUND_MASK_VIDEO:
      sel = 2;
      break;
    case SOUND_MASK_LINE1:
      sel = 3;
      break;			/* AUX */
    case SOUND_MASK_LINE:
      sel = 4;
      break;
    case SOUND_MASK_VOLUME:
      sel = 5;
      break;
    case SOUND_MASK_MONO:
      sel = 6;
      break;
    case SOUND_MASK_PHONE:
      sel = 7;
      break;
    default:			/* Unknown choise. Default to mic */
      devc->recdevs = SOUND_MASK_MIC;
      sel = 0;
    }
  codec_write (devc, 0x1a, sel | (sel << 8));

  return devc->levels[31] = devc->recdevs;
}

static int
ac97_mixer_get (ac97_devc * devc, int dev)
{
  if (dev < 0 || dev >= SOUND_MIXER_NRDEVICES)
    return OSS_EINVAL;
  if (!((1 << dev) & devc->devmask))
    return OSS_EINVAL;
  return devc->levels[dev];
}

static unsigned int
mix_scale (int left, int right, int bits)
{
  int reverse = 0, mute = 0;

  if (bits < 0)
    {
      bits = -bits;
      reverse = 1;
    }

  if ((left | right) == 0)	/* Mute */
    mute = 0x8000;

  if (reverse)
    {
      left = 100 - left;
      right = 100 - right;
    }

  left = 100 - mix_cvt[left];
  right = 100 - mix_cvt[right];

  return mute | ((left * ((1 << bits) - 1) / 100) << 8) | (right *
							   ((1 << bits) -
							    1) / 100);
}

int
ac97_mixer_set (ac97_devc * devc, int dev, int value)
{
  int left, right, lvl;

  if (dev < 0 || dev >= SOUND_MIXER_NRDEVICES)
    return OSS_EINVAL;
  if (!((1 << dev) & devc->devmask))
    return OSS_EINVAL;

  if (!((1 << dev) & STEREO_DEVS))
    {
      lvl = value & 0xff;
      if (lvl > 100)
	lvl = 100;
      left = right = lvl;
      value = lvl | (lvl << 8);
    }
  else
    {
      left = value & 0xff;
      right = (value >> 8) & 0xff;
      if (left > 100)
	left = 100;
      if (right > 100)
	right = 100;
      value = left | (right << 8);
    }

  switch (dev)
    {
    case SOUND_MIXER_VOLUME:
      if (!(devc->devmask & SOUND_MASK_ALTPCM))
	codec_write (devc, 0x04,
		     mix_scale (left, right, devc->mastervol_bits));
      codec_write (devc, 0x02, mix_scale (left, right, devc->mastervol_bits));
      break;

    case SOUND_MIXER_TREBLE:
      codec_write (devc, 0x08,
		   mix_scale (left, devc->levels[SOUND_MIXER_TREBLE] & 0xff,
			      4));
      break;

    case SOUND_MIXER_BASS:
      codec_write (devc, 0x08,
		   mix_scale (devc->levels[SOUND_MIXER_BASS] & 0xff, left,
			      4));
      break;

    case SOUND_MIXER_DEPTH:
      codec_write (devc, 0x22,
		   (mix_cvt[left] * ((1 << devc->enh_bits) - 1)) / 100);
      break;

    case SOUND_MIXER_SPEAKER:
      codec_write (devc, 0x0a, mix_scale (0, left, 5) & 0x801F);
      break;

    case SOUND_MIXER_PHONE:
      codec_write (devc, 0x0c, mix_scale (0, left, devc->mastervol_bits));
      break;

    case SOUND_MIXER_MONO:
      codec_write (devc, 0x06, mix_scale (0, left, devc->mastervol_bits));
      break;

    case SOUND_MIXER_MIC:
      codec_write (devc, 0x0e, (mix_scale (0, left, devc->mastervol_bits)
				& ~0x40) | devc->micboost);
      break;

    case SOUND_MIXER_LINE:
      codec_write (devc, 0x10, mix_scale (left, right, 5));
      break;

    case SOUND_MIXER_CD:
      codec_write (devc, 0x12, mix_scale (left, right, 5));
      break;

    case SOUND_MIXER_VIDEO:
      codec_write (devc, 0x14, mix_scale (left, right, 5));
      break;

    case SOUND_MIXER_LINE1:
      codec_write (devc, 0x16, mix_scale (left, right, 5));
      break;

    case SOUND_MIXER_PCM:
      codec_write (devc, 0x18, mix_scale (left, right, devc->pcmvol_bits));
      break;

    case SOUND_MIXER_IGAIN:
      codec_write (devc, 0x1c, mix_scale (left, right, -4));
      break;

    case SOUND_MIXER_ALTPCM:
      codec_write (devc, 0x04, mix_scale (left, right, devc->mastervol_bits));
#if 0
      codec_write (devc, 0x36, mix_scale (left, right, devc->mastervol_bits));
      codec_write (devc, 0x38, mix_scale (left, right, devc->mastervol_bits));
#endif
      break;

    case SOUND_MIXER_REARVOL:
      codec_write (devc, 0x38, mix_scale (left, right, devc->rearvol_bits));
      break;

    case SOUND_MIXER_CENTERVOL:
      codec_write (devc, 0x36, mix_scale (left, right, devc->centervol_bits));
      break;

#if 1				/* AC97 2.4 specified mid-surround channel */
    case SOUND_MIXER_SIDEVOL:
      codec_write (devc, 0x1c, mix_scale (left, right, devc->sidevol_bits));
      break;
#endif
    default:
      return OSS_EINVAL;
    }

  return devc->levels[dev] = value;
}

#include "ac97_ext.inc"

static void
ac97_mixer_reset (ac97_devc * devc)
{
  int i;

  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
    if ((1 << i) & devc->devmask)
      ac97_mixer_set (devc, i, devc->levels[i]);

  devc->recmask = REC_DEVS & devc->devmask;
  if (devc->levels[31] == 0)
    devc->levels[31] = SOUND_MASK_LINE;
  ac97_set_recmask (devc, devc->levels[31]);
}

/*
 * Remove a set of unused controls (mask) from the list of supported
 * controls.
 */
void
ac97_remove_control (ac97_devc * devc, int mask, int level)
{
  int i;

  if (!devc->is_ok)
    return;

  devc->devmask &= ~(mask);
  devc->recmask = REC_DEVS & devc->devmask;
  devc->recdevs &= devc->recmask;
  if (devc->recmask == 0)
    {
      ac97_set_recmask (devc, SOUND_MASK_LINE);
    }

  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
    if (mask & (1 << i))
      ac97_mixer_set (devc, i, level);
}

/*
 * Override a volume control with application defined handler. Before that
 * set the AC97 volume to the given level. 
 */
void
ac97_override_control (ac97_devc * devc, int ctrl, ac97_ext_ioctl func,
		       int level)
{
  int dev = devc->mixer_dev;
  int oldlevel;

  if (!devc->is_ok)
    return;

  if (ctrl < 0 || ctrl >= SOUND_MIXER_NRDEVICES)
    {
      cmn_err (CE_WARN, "ac97: Invalid override for %d\n", ctrl);
      return;
    }
  oldlevel = ac97_mixer_get (devc, ctrl);
  ac97_mixer_set (devc, ctrl, level);

  devc->overrides[ctrl] = func;

  mixer_devs[dev]->ignore_mask |= (1 << ctrl);
  func (devc->mixer_dev, -1, MIXER_WRITE (ctrl), oldlevel);
}

/*ARGSUSED*/
static int
find_current_recsrc (ac97_devc * devc)
{
  return SOUND_MIXER_IGAIN;
}

static int
find_current_monsrc (ac97_devc * devc)
{
  int i;

  for (i = 0; i < 32; i++)
    {
      if (devc->recdevs & (1 << i))
	return i;
    }

  return SOUND_MIXER_MIC;
}

static int
call_override_func (ac97_devc * devc, int audiodev, unsigned int cmd, int val)
{
  int ret, ctrl;

  if (!devc->is_ok)
    return OSS_EIO;

  ctrl = cmd & 0xff;

  if (ctrl < 0 || ctrl >= SOUND_MIXER_NRDEVICES
      || devc->overrides[ctrl] == NULL)
    return OSS_EINVAL;

  ret = devc->overrides[ctrl] (devc->mixer_dev, audiodev, cmd, val);
  if (ret < 0)
    return ret;

  return devc->levels[ctrl] = ret;
}

static int
ac97_mixer_ioctl (int dev, int audiodev, unsigned int cmd, ioctl_arg arg)
{
  ac97_devc *devc = mixer_devs[dev]->devc;
  int ctrl;

/*
 * Handle some special cases first
 */

  switch (cmd)
    {
    case SOUND_MIXER_WRITE_RECGAIN:
      {
	int chn, val;
	chn = find_current_recsrc (devc);
	val = *arg;
	return *arg = (ac97_mixer_set (devc, chn, val));
      }
      break;

    case SOUND_MIXER_READ_RECGAIN:
      {
	int chn;
	chn = find_current_recsrc (devc);
	return *arg = (ac97_mixer_get (devc, chn));
      }
      break;

    case SOUND_MIXER_WRITE_MONGAIN:
      {
	int chn, val;
	chn = find_current_monsrc (devc);
	val = *arg;
	return *arg = (ac97_mixer_set (devc, chn, val));
      }
      break;

    case SOUND_MIXER_READ_MONGAIN:
      {
	int chn;
	chn = find_current_monsrc (devc);
	return *arg = (ac97_mixer_get (devc, chn));
      }
      break;

      /* enable/disable 20db Mic Boost */
    case SOUND_MIXER_PRIVATE1:
      {
	int value, tmp;

	value = *arg;
	if (value != 0 && value != 1)
	  return OSS_EINVAL;

	tmp = codec_read (devc, 0x0e);

	if (value)
	  devc->micboost = 0x40;
	else
	  devc->micboost = 0x00;

	codec_write (devc, 0x0e, (tmp & ~0x40) | devc->micboost);
	return *arg = (value);
      }
      break;
    }

  if (((cmd >> 8) & 0xff) == 'M')	/* Set control */
    {
      int val;

      if (IOC_IS_OUTPUT (cmd))
	switch (cmd & 0xff)
	  {
	  case SOUND_MIXER_RECSRC:
	    if (ac97_recselect)
	      return *arg = (SOUND_MASK_MIC);
	    val = *arg;
	    return *arg = (ac97_set_recmask (devc, val));
	    break;

	  default:
	    if ((cmd & 0xff) > SOUND_MIXER_NRDEVICES)
	      return OSS_EINVAL;
	    val = *arg;
	    ctrl = cmd & 0xff;
	    if (ctrl >= 0 && ctrl < SOUND_MIXER_NRDEVICES)
	      if (devc->overrides[ctrl] != NULL)
		return *arg = call_override_func (devc, audiodev, cmd, val);

	    return *arg = (ac97_mixer_set (devc, cmd & 0xff, val));
	  }
      else
	switch (cmd & 0xff)	/*
				 * Return parameters
				 */
	  {

	  case SOUND_MIXER_RECSRC:
	    if (ac97_recselect)
	      return *arg = (SOUND_MASK_MIC);
	    return *arg = (devc->recdevs);
	    break;

	  case SOUND_MIXER_DEVMASK:
	    return *arg = (devc->devmask);
	    break;

	  case SOUND_MIXER_STEREODEVS:
	    return *arg = (STEREO_DEVS);
	    break;

	  case SOUND_MIXER_RECMASK:
	    if (ac97_recselect)
	      return *arg = (0);

	    return *arg = (devc->recmask);
	    break;

	  case SOUND_MIXER_CAPS:
	    return *arg = (SOUND_CAP_EXCL_INPUT);
	    break;

	  default:
	    ctrl = cmd & 0xff;
	    if (ctrl >= 0 && ctrl < SOUND_MIXER_NRDEVICES)
	      if (devc->overrides[cmd & 0xff] != NULL)
		return *arg = call_override_func (devc, audiodev, cmd, 0);
	    return *arg = (ac97_mixer_get (devc, cmd & 0xff));
	  }
    }
  else
    {
      return OSS_EINVAL;
    }
}

static mixer_driver_t ac97_mixer_driver = {
  ac97_mixer_ioctl
};

int
ac97_install (ac97_devc * devc, char *host_name, ac97_readfunc_t readfn,
	      ac97_writefunc_t writefn, void *hostparms, oss_device_t * osdev)
{
  return
    ac97_install_full (devc, host_name, readfn, writefn, hostparms, osdev, 0);
}

int
ac97_install_full (ac97_devc * devc, char *host_name, ac97_readfunc_t readfn,
		   ac97_writefunc_t writefn, void *hostparms,
		   oss_device_t * osdev, int flags)
{
  int my_mixer;
  int tmp, tmp2;
  unsigned int id, mask;
  char tmp_name[100];

  memset (devc, 0, sizeof (ac97_devc));
  devc->osdev = osdev;
  devc->host_parms = hostparms;
  devc->read = readfn;
  devc->is_ok = 0;
  devc->write = writefn;
  strcpy (devc->name, "AC97");
  if (codec_write (devc, 0x00, 0x00) < 0)	/* reset */
    return OSS_EIO;
  codec_write (devc, 0x26, 0x00);	/* Power up */
  oss_udelay (1000);
  if (ac97_amplifier != -1) tmp = ac97_amplifier;
  else tmp = !(flags & AC97_INVERTED);

  if (tmp)
    codec_write (devc, 0x26, codec_read (devc, 0x26) & ~0x8000);	/* Power up (external amplifier powered up) */
  else
    codec_write (devc, 0x26, codec_read (devc, 0x26) | 0x8000);	/* Power up (external amplifier powered down) */
  codec_write (devc, 0x20, 0x00);	/* General Purpose */

  tmp = codec_read (devc, 0x7c);
  if (tmp >= 0xffff)
    {
      cmn_err (CE_WARN, "AC97 Codec/mixer chip doesn't seem to be alive.\n");
      return OSS_EIO;
    }

  tmp2 = codec_read (devc, 0x7e);
  id = ((tmp & 0xffff) << 16) | (tmp2 & 0xffff);

  DDB (cmn_err (CE_CONT, "AC97 codec ID=0x%08x ('%c%c%c%x')\n",
		id,
		(tmp >> 8) & 0xff,
		tmp & 0xff, (tmp2 >> 8) & 0xff, tmp2 & 0xff));
  devc->ac97_id = codec_read (devc, 0x00);
  devc->enh_3d = (devc->ac97_id >> 10) & 0x1f;

  devc->devmask = MIXER_DEVS;
  devc->fixed_rate = 0;
  devc->var_rate_support = (codec_read (devc, 0x28) & 0x1) ? 1 : 0;
  devc->spdifout_support = (codec_read (devc, 0x28) & 0x4) ? 1 : 0;
  devc->mixer_ext = 0;
  devc->playrate_support = PLAY_2CHAN;

  if (codec_read (devc, 0x28) & 0x1C0)
    devc->playrate_support = PLAY_6CHAN;
  else if (codec_read (devc, 0x28) & 0x80)
    devc->playrate_support = PLAY_4CHAN;

  devc->enh_bits = 4;
  devc->micboost = 0x40;
  devc->pcmvol_bits = 5;
  devc->rearvol_bits = 5;
  devc->sidevol_bits = 5;
  devc->centervol_bits = 5;
  devc->auxvol_bits = 5;
  devc->extmixlevels[CENTER_VOL] = 0x0404;
  devc->extmixlevels[REAR_VOL] = 0x0404;
  devc->extmixlevels[SIDE_VOL] = 0x0404;

  /* need to detect all the Cirrus Logic variations! */
  if ((id & 0xffff0000) == 0x43520000)
    mask = 0xFFFFFFF0;
  else
    mask = 0xFFFFFFFF;

  switch (id & mask)
    {
    case 0:
      strcpy (devc->name, "Unknown");
      break;

    case 0x414b4d00:
      strcpy (devc->name, "AK4540");
      break;

    case 0x83847600:
      strcpy (devc->name, "STAC9700");
      break;

    case 0xc250c250:
    case 0x83847601:
      strcpy (devc->name, "STAC9701");
      break;

    case 0x83847609:
      strcpy (devc->name, "STAC9721");
      break;

    case 0x83847604:
      strcpy (devc->name, "STAC9704");
      break;

    case 0x83847605:
      strcpy (devc->name, "STAC9705");
      break;

    case 0x83847608:
      strcpy (devc->name, "STAC9708");
      devc->devmask |= SOUND_MASK_ALTPCM;
      codec_write (devc, 0x6E, 8);	/* codec_read (devc, 0x6E) & ~0x8); non-inverted pphase */
      devc->enh_bits = 2;
      break;

    case 0x83847644:
      strcpy (devc->name, "STAC9744");
      break;

    case 0x83847650:
      strcpy (devc->name, "STAC9750");
      devc->spdifout_support = STAC_SPDIFOUT;	/* SPDIF output on ACR */
      devc->enh_bits = 3;
      break;

    case 0x83847652:
      strcpy (devc->name, "STAC9752");
      devc->spdifout_support = STAC_SPDIFOUT;	/* SPDIF output on ACR */
      devc->enh_bits = 3;
      break;

    case 0x83847656:
      strcpy (devc->name, "STAC9756");
      devc->spdifout_support = STAC_SPDIFOUT;	/* SPDIF output on ACR */
      devc->enh_bits = 3;
      break;

    case 0x83847666:
      strcpy (devc->name, "STAC9766");
      devc->enh_bits = 3;
      devc->spdifout_support = STAC_SPDIFOUT;	/* SPDIF output on ACR */
      break;

    case 0x83847658:
      strcpy (devc->name, "STAC9758");
      devc->enh_bits = 3;
      devc->spdifout_support = STAC_SPDIFOUT;	/* SPDIF output on ACR */
      devc->mixer_ext = STAC9758_MIXER_EXT;
      break;

    case 0x54524108:
    case 0x54524128:
      strcpy (devc->name, "TR28028");
      devc->devmask |= SOUND_MASK_ALTPCM;
      break;

    case 0x54524103:
    case 0x54524123:
      strcpy (devc->name, "TR28023");
      break;

    case 0x454d4328:
      strcpy (devc->name, "EM28028");
      codec_write (devc, 0x2a, (codec_read (devc, 0x2a) & ~3800) | 0xE0);
      devc->devmask |= SOUND_MASK_ALTPCM;
      break;

    case 0x43585428:
    case 0x43585429:
      strcpy (devc->name, "CX20468");
      devc->spdifout_support = CX_SPDIFOUT;
      break;

    case 0x43525900:
      strcpy (devc->name, "CS4297");
      break;

    case 0x43525910:
      strcpy (devc->name, "CS4297A");
      devc->spdifout_support = CS_SPDIFOUT;
      break;

    case 0x43525920:
      strcpy (devc->name, "CS4294");
      break;

    case 0x43525930:
      strcpy (devc->name, "CS4299");
      devc->spdifout_support = CS_SPDIFOUT;
      break;

    case 0x43525970:
      strcpy (devc->name, "CS4202");
      devc->spdifout_support = CS_SPDIFOUT;
      break;

    case 0x43525950:
      strcpy (devc->name, "CS4205");
      devc->spdifout_support = CS_SPDIFOUT;
      break;

    case 0x4144303:		/* ADS3 */
      strcpy (devc->name, "AD1819B");
      break;

    case 0x41445340:		/* ADS40 */
      strcpy (devc->name, "AD1881");
      break;

    case 0x41445348:		/* ADS48 */
      strcpy (devc->name, "AD1881A");
      break;

    case 0x41445360:		/* ADS60 */
      strcpy (devc->name, "AD1885");
      break;

    case 0x41445361:		/* ADS61 */
      strcpy (devc->name, "AD1886");
      /* Jack sense */
      codec_write (devc, 0x72, (codec_read (devc, 0x72) & (~0xEF)) | 0x10);
      devc->spdifout_support = AD_SPDIFOUT;
      break;

    case 0x41445362:		/* ADS62 */
      strcpy (devc->name, "AD1887");
      break;

    case 0x41445368:		/* ADS62 */
      strcpy (devc->name, "AD1888");
      devc->spdifout_support = AD_SPDIFOUT;
      devc->mixer_ext = AD1980_MIXER_EXT;
      codec_write (devc, 0x76, 0xC420);
      devc->centervol_bits = 6;
      devc->rearvol_bits = 6;
      codec_write (devc, 0x04, 0x0808);
      break;

    case 0x41445370:		/* ADS70 */
      strcpy (devc->name, "AD1980");
      devc->spdifout_support = AD_SPDIFOUT;
      devc->mixer_ext = AD1980_MIXER_EXT;
      codec_write (devc, 0x76, 0xC420);
      break;

    case 0x41445372:		/* ADS72 */
      strcpy (devc->name, "AD1981");
      devc->spdifout_support = AD_SPDIFOUT;
      /* set jacksense to mute line if headphone is plugged */
      if (flags & AC97_FORCE_SENSE)
	/* XXX */
        codec_write (devc, 0x72, (codec_read (devc, 0x72) & (~0xe00)) | 0x400);
      break;

    case 0x41445374:		/* ADS74 */
      strcpy (devc->name, "AD1981B");
      devc->spdifout_support = AD_SPDIFOUT;
      /* set jacksense to mute line if headphone is plugged */
      if (flags & AC97_FORCE_SENSE)
	codec_write (devc, 0x72, (codec_read (devc, 0x72) | 0x0800));
      break;

    case 0x41445375:		/* ADS74 */
      strcpy (devc->name, "AD1985");
      devc->spdifout_support = AD_SPDIFOUT;
      devc->mixer_ext = AD1980_MIXER_EXT;
      if (flags & AC97_FORCE_SENSE)
	/* XXX */
        codec_write (devc, 0x72, (codec_read (devc, 0x72) & (~0xe00)) | 0x400);
      codec_write (devc, 0x76, 0xC420);
      break;

    case 0x574d4c00:
      strcpy (devc->name, "WM9701A");	/* www.wolfson.co.uk */
      break;

    case 0x574d4c03:
      strcpy (devc->name, "WM9703");
      break;

    case 0x574d4c04:
      strcpy (devc->name, "WM9704");
      devc->mixer_ext = WM9704_MIXER_EXT;
      codec_write (devc, 0x5A, codec_read (devc, 0x5A) | 0x80);	/*enable I2S */
      break;

    case 0x45838308:
      strcpy (devc->name, "ES19XX");
      break;

    case 0x49434511:		/* IcEnsemble ICE1232 (VIA1611A) */
      strcpy (devc->name, "ICE1232/VT1611A");
      break;

    case 0x56494161:		/* VIA1612A */
    case 0x56494170:		/* VIA1612A */
      strcpy (devc->name, "VT1612A");
      if (codec_read (devc, 0x28) & 0x04)
	devc->spdifout_support = VIA_SPDIFOUT;
      devc->mixer_ext = VIA1616_MIXER_EXT;
      codec_write (devc, 0x2a, codec_read (devc, 0x2a) & ~0x3800);
      codec_write (devc, 0x5a, 0x0230);
      break;

    case 0x49434551:		/* IcEnsemble ICE1232 (VIA1616) */
      strcpy (devc->name, "VT1616");
      /* Enable S/PDIF mixer extensions only if S/PDIF is physically present */
      if (codec_read (devc, 0x28) & 0x04)
	devc->spdifout_support = VIA_SPDIFOUT;
      devc->mixer_ext = VIA1616_MIXER_EXT;
      devc->mixer_ext = VIA1616_MIXER_EXT;
      codec_write (devc, 0x2a, codec_read (devc, 0x2a) & ~0x3800);
      codec_write (devc, 0x5a, 0x0230);
      break;

    case 0x49434552:		/* IcEnsemble ICE1232 (VIA1616A) */
      strcpy (devc->name, "VT1616A");
      devc->spdifout_support = VIA_SPDIFOUT;
      devc->mixer_ext = VIA1616_MIXER_EXT;
      break;

    case 0x56494182:		/* VIA1618 */
      strcpy (devc->name, "VT1618");
      devc->spdifout_support = VIA_SPDIFOUT;
      devc->mixer_ext = VIA1616_MIXER_EXT;
      break;

    case 0x414c4326:
      strcpy (devc->name, "ALC100");
      devc->enh_bits = 2;
      break;

    case 0x414c4710:
      strcpy (devc->name, "ALC200P");
      devc->enh_bits = 2;
      break;

    case 0x414c4740:
      strcpy (devc->name, "ALC202");	/* www.realtek.com.tw */
      devc->spdifout_support = ALC_SPDIFOUT;
      devc->enh_bits = 2;
      break;

    case 0x414c4770:
      strcpy (devc->name, "ALC203");	/* www.realtek.com.tw */
      devc->spdifout_support = ALC_SPDIFOUT;
      devc->enh_bits = 2;
      break;

    case 0x000f8384:
      strcpy (devc->name, "EV1938");	/* Creative/Ectiva */
      break;

    case 0x414c4750:
    case 0x414c4752:
      strcpy (devc->name, "ALC250");	/* www.realtek.com.tw */
      devc->spdifout_support = ALC_SPDIFOUT;
      devc->spdifin_support = ALC_SPDIFIN;
      devc->enh_bits = 2;
      break;

    case 0x414c4720:
      strcpy (devc->name, "ALC650");	/* www.realtek.com.tw */
      devc->spdifout_support = ALC_SPDIFOUT;
/*      devc->spdifin_support = ALC_SPDIFIN; */
      devc->mixer_ext = ALC650_MIXER_EXT;
      devc->enh_bits = 2;
      break;

    case 0x414c4760:
    case 0x414c4761:
      strcpy (devc->name, "ALC655");	/* www.realtek.com.tw */
      devc->spdifout_support = ALC_SPDIFOUT;
      devc->spdifin_support = ALC_SPDIFIN;
      devc->mixer_ext = ALC650_MIXER_EXT;
      devc->enh_bits = 2;
      break;

    case 0x414c4780:
      strcpy (devc->name, "ALC658");	/* www.realtek.com.tw */
      devc->spdifout_support = ALC_SPDIFOUT;
      devc->spdifin_support = ALC_SPDIFIN;
      devc->mixer_ext = ALC650_MIXER_EXT;
      devc->enh_bits = 2;
      break;

    case 0x414c4790:
      strcpy (devc->name, "ALC850");	/* www.realtek.com.tw */
      devc->spdifout_support = ALC_SPDIFOUT;
      devc->spdifin_support = ALC_SPDIFIN;
      devc->mixer_ext = ALC650_MIXER_EXT;
      devc->enh_bits = 2;
      break;

    case 0x434d4941:
      strcpy (devc->name, "CMI9738");
      devc->devmask |= SOUND_MASK_ALTPCM;
      break;

    case 0x434d4961:
      strcpy (devc->name, "CMI9739");
      devc->spdifout_support = CMI_SPDIFOUT;
      devc->spdifin_support = CMI_SPDIFIN;
      devc->mixer_ext = CMI9739_MIXER_EXT;
      break;

    case 0x434d4983:
      strcpy (devc->name, "CMI9761A");
      devc->spdifout_support = CMI_SPDIFOUT;
      devc->spdifin_support = CMI_SPDIFIN;
      devc->mixer_ext = CMI9739_MIXER_EXT;
      break;

    case 0x434d4969:
      strcpy (devc->name, "CMI9780");
#if 0
      devc->spdifout_support = CMI_SPDIFOUT;
      devc->spdifin_support = CMI_SPDIFIN;
      devc->mixer_ext = CMI9780_MIXER_EXT;
      devc->playrate_support == PLAY_8CHAN;
#endif
      break;

    case 0x594d4800:
      strcpy (devc->name, "YMF743");
      break;


    case 0x594d4803:
      strcpy (devc->name, "YMF753");
      devc->spdifout_support = YMF_SPDIFOUT;
      codec_write (devc, 0x66, codec_read (devc, 0x66) | 0x9);	/* set TX8 + 3AWE */
      break;

    default:
      sprintf (devc->name, "0x%04x%04x", tmp, tmp2);
    }

  DDB (cmn_err (CE_CONT, "Detected AC97 codec: %s\n", devc->name));
  DDB (cmn_err (CE_CONT, "AC97 codec capabilities %x\n", devc->ac97_id));
  DDB (cmn_err
       (CE_CONT, "Dedicated Mic PCM in channel %d\n",
	!!(devc->ac97_id & 0x0001)));
  DDB (cmn_err
       (CE_CONT, "Modem Line Codec support %d\n",
	!!(devc->ac97_id & 0x0002)));
  DDB (cmn_err
       (CE_CONT, "Bass&Treble control %d\n", !!(devc->ac97_id & 0x0004)));
  DDB (cmn_err
       (CE_CONT, "Simulated stereo %d\n", !!(devc->ac97_id & 0x0008)));
  DDB (cmn_err
       (CE_CONT, "Headphone out support %d\n", !!(devc->ac97_id & 0x0010)));
  DDB (cmn_err
       (CE_CONT, "Loudness support %d\n", !!(devc->ac97_id & 0x0020)));
  DDB (cmn_err
       (CE_CONT, "18bit DAC resolution %d\n", !!(devc->ac97_id & 0x0040)));
  DDB (cmn_err
       (CE_CONT, "20bit DAC resolution %d\n", !!(devc->ac97_id & 0x0080)));
  DDB (cmn_err
       (CE_CONT, "18bit ADC resolution %d\n", !!(devc->ac97_id & 0x0100)));
  DDB (cmn_err
       (CE_CONT, "20bit ADC resolution %d\n", !!(devc->ac97_id & 0x0200)));
  DDB (cmn_err (CE_CONT, "3D enhancement technique: %x\n", devc->enh_3d));
#if 1
  {
    int ext_status = codec_read (devc, 0x28);
    DDB (cmn_err
	 (CE_CONT, "AC97 v2.1 multi slot support %d\n",
	  !!(ext_status & 0x0200)));
    DDB (cmn_err
	 (CE_CONT, "AC97 v2.1 surround DAC support 0x%x\n",
	  (ext_status >> 6) & 0x07));
  }
#endif

  if (devc->fixed_rate)
    {
      int tmp3;

      /* Turn off variable samling rate support */
      tmp3 = codec_read (devc, 0x2a) & ~0x0001;
      codec_write (devc, 0x2a, tmp3);
    }

  if (devc->ac97_id & 0x0004)
    devc->devmask |= SOUND_MASK_BASS | SOUND_MASK_TREBLE;

  if (devc->enh_3d != 0)
    {
      devc->devmask |= SOUND_MASK_DEPTH;
      codec_write (devc, 0x20, codec_read (devc, 0x20) | 0x2000);	/* Turn on 3D */
    }


/*
 * Detect if the codec supports 5 or 6 bits in the master control register.
 */

  codec_write (devc, 0x02, 0x20);
  if ((codec_read (devc, 0x02) & 0x1f) == 0x1f)
    devc->mastervol_bits = 5;
  else
    devc->mastervol_bits = 6;

#if 0
  codec_write (devc, 0x18, 0x20);
  if ((codec_read (devc, 0x18) & 0x1f) == 0x1f)
    devc->pcmvol_bits = 5;
  else
    devc->pcmvol_bits = 6;
#endif

  if (devc->playrate_support == PLAY_8CHAN)
    devc->devmask |=
      SOUND_MASK_REARVOL | SOUND_MASK_CENTERVOL | SOUND_MASK_SIDEVOL;

  if (devc->playrate_support == PLAY_6CHAN)
    devc->devmask |= SOUND_MASK_REARVOL | SOUND_MASK_CENTERVOL;

  if (devc->playrate_support == PLAY_4CHAN)
    devc->devmask |= SOUND_MASK_REARVOL;

  sprintf (tmp_name, "%s (%s)", host_name, devc->name);

  if ((my_mixer = oss_install_mixer (OSS_MIXER_DRIVER_VERSION,
				     osdev,
				     osdev,
				     tmp_name,
				     &ac97_mixer_driver,
				     sizeof (mixer_driver_t), devc)) >= 0)
    {
      mixer_devs[my_mixer]->hw_devc = hostparms;
      mixer_devs[my_mixer]->priority = 2;	/* Possible default mixer candidate */
      devc->recdevs = 0;
      devc->mixer_dev = my_mixer;
      devc->is_ok = 1;
      sprintf (tmp_name, "%s_%d", devc->name, nr_ac97++);

      devc->levels = load_mixer_volumes (tmp_name, default_mixer_levels, 1);

      ac97_mixer_reset (devc);
      /* AD1888 seems to keep muting the headphone out */
      if ((id & mask) == 0x41445368)
	{
	  codec_write (devc, 0x04, codec_read (devc, 0x04) & ~0x8000);
	  codec_write (devc, 0x1c, codec_read (devc, 0x1c) & ~0x8000);
	}
      /* set PC speaker to mute causes humming on STAC97xx AC97s */
      codec_write (devc, 0x0a, 0x8000);
    }

  if ((devc->mixer_ext) || (devc->spdifout_support)
      || (devc->spdifin_support) || ac97_recselect)
    {
      mixer_ext_set_init_fn (devc->mixer_dev, ac97mixer_ext_init, 60);
    }
  return my_mixer;
}

int
ac97_init_ext (int dev, ac97_devc * devc, mixer_create_controls_t func,
	       int nextra)
{
  if (dev < 0)
    return OSS_EIO;
  if ((devc->mixer_ext) || (devc->spdifout_support)
      || (devc->spdifin_support) || ac97_recselect)
    nextra += 50;
  devc->client_mixer_init = func;
  return mixer_ext_set_init_fn (dev, ac97mixer_ext_init, nextra);
}

int
ac97_varrate (ac97_devc * devc)
{
  int ext_status;

  if ((devc->fixed_rate) || !devc->is_ok)
    return 0;

  ext_status = codec_read (devc, 0x28);
  if (ext_status & 0x0001)
    {
      devc->var_rate_support = 1;
    }
  return devc->var_rate_support;
}

int
ac97_playrate (ac97_devc * devc, int srate)
{
  if (!devc->is_ok)
    return 0;

  if ((codec_read (devc, 0x7c) == 0x4144)
      && (codec_read (devc, 0x7e) == 0x5303))
    {				/* AD 18191B */
      codec_write (devc, 0x74, 0x1901);
      codec_write (devc, 0x76, 0x0404);
      codec_write (devc, 0x7A, srate);	/*use sr1 for play */
      return 1;
    }

  if ((codec_read (devc, 0x7c) == 0x4144)
      && ((codec_read (devc, 0x7e) == 0x5348) ||
	  (codec_read (devc, 0x7e) == 0x5360) ||
	  (codec_read (devc, 0x7e) == 0x5361) ||
	  (codec_read (devc, 0x7e) == 0x5362)))

    {				/* AD1881/AD1886/AD1887/AD1891 */
      codec_write (devc, 0x76, 0x0404);
      codec_write (devc, 0x2a, codec_read (devc, 0x2a) | 0x01);
      codec_write (devc, 0x2c, srate);	/* use sr1 for play */
      return 1;
    }

  if (devc->playrate_support == PLAY_6CHAN)	/* set front/rear/lfe/c/s rate */
    {
      codec_write (devc, 0x2c, srate);
      codec_write (devc, 0x2e, srate);
      codec_write (devc, 0x30, srate);
      codec_write (devc, 0x2a, codec_read (devc, 0x2a) | 0x01);
      return 1;
    }

  codec_write (devc, 0x2c, srate);
  codec_write (devc, 0x2a, codec_read (devc, 0x2a) | 0x01);
  return 1;
}

int
ac97_recrate (ac97_devc * devc, int srate)
{
  if (!devc->is_ok)
    return 0;

  if ((codec_read (devc, 0x7c) == 0x4144)
      && (codec_read (devc, 0x7e) == 0x5303))
    {				/* AD 1819B */
      codec_write (devc, 0x74, 0x1901);
      codec_write (devc, 0x76, 0x0404);
      codec_write (devc, 0x78, srate);	/*use sr0 for rec */
      return 1;
    }

  if ((codec_read (devc, 0x7c) == 0x4144)
      && ((codec_read (devc, 0x7e) == 0x5348) ||
	  (codec_read (devc, 0x7e) == 0x5360) ||
	  (codec_read (devc, 0x7e) == 0x5361) ||
	  (codec_read (devc, 0x7e) == 0x5362)))
    {				/* AD1881/AD886/AD1887/AD1891 */
      codec_write (devc, 0x76, 0x0404);
      codec_write (devc, 0x2a, codec_read (devc, 0x2a) | 0x01);
      codec_write (devc, 0x32, srate);	/* use sr0 for rec */
      return 1;
    }

  codec_write (devc, 0x32, srate);
  codec_write (devc, 0x2a, codec_read (devc, 0x2a) | 0x01);
  return 1;
}
