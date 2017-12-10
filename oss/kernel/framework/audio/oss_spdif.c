/*
 * Purpose: S/PDIF (IEC958) control bit and mixer extension management
 *
 * This code is used by some drivers to handle S/PDIF specific control
 * functions (see {!xlink spdif_control} for more info.
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
#include "spdif.h"

#define MAX_DEV 10
static int ndevs = 0;
static spdif_devc *devc_list[MAX_DEV] = { 0 };

static void
init_ctl (spdif_devc * devc, oss_digital_control * ctl)
{
  memset (ctl, 0, sizeof (*ctl));

  ctl->caps = devc->caps;
}

static void
block_mixer (spdif_devc * devc)
{
  devc->mixer_blocked = 1;
  memcpy (&devc->cold_ctl, &devc->hot_ctl, sizeof (devc->hot_ctl));
  devc->ctl = &devc->cold_ctl;

}

static void
unblock_mixer (spdif_devc * devc)
{
  memcpy (&devc->hot_ctl, &devc->cold_ctl, sizeof (devc->hot_ctl));
  devc->ctl = &devc->hot_ctl;
  devc->mixer_blocked = 0;
}

int
oss_spdif_install (spdif_devc * devc, oss_device_t * osdev,
		   spdif_driver_t * d, int driver_size, void *host_devc,
		   void *host_portc, int mixer_dev, int flags,
		   unsigned int caps)
{
  int i, pos;

  if (devc->is_ok)
    {
      /* cmn_err (CE_WARN, "spdif: Bad usage - double init\n"); */
      return 0;
    }

  if (d == NULL || driver_size != sizeof (spdif_driver_t))
    {
      cmn_err (CE_WARN, "spdif: Bad driver\n");
      return OSS_EIO;
    }

  memset (devc, 0, sizeof (*devc));

  devc->open_mode = 0;
  devc->osdev = osdev;
  MUTEX_INIT (devc->osdev, devc->mutex, MH_FRAMEW + 2);
  devc->d = d;
  devc->flags = flags;
  devc->host_devc = host_devc;
  devc->host_portc = host_portc;
  devc->mixer_dev = mixer_dev;
  devc->ctl = &devc->hot_ctl;
  devc->caps = caps;
  devc->is_ok = 1;

  pos=-1;

  for (i=0;pos==-1 && i<ndevs;i++)
  if (devc_list[i]==NULL)
  {
	  pos = i;
  }

  if (pos==-1)
  {
     if (ndevs >= MAX_DEV)
     {
	     cmn_err(CE_WARN, "Too many S/PDIF devices\n");
	     return OSS_EBUSY;
     }

     pos=ndevs++;
  }

  devc_list[pos] = devc;

  init_ctl (devc, &devc->hot_ctl);
  init_ctl (devc, &devc->cold_ctl);

  return 0;
}

void
oss_spdif_uninstall (spdif_devc * devc)
{
	int i;

	for (i=0;i<ndevs;i++)
	    if (devc_list[i]==devc)
	       devc_list[i]=NULL;
}

int
oss_spdif_open (spdif_devc * devc, int open_mode)
{
  oss_native_word flags;
  flags = 0;
  if (!devc->is_ok)
    {
      cmn_err (CE_WARN, "spdif: Bad usage - open\n");
      return OSS_EIO;
    }
  MUTEX_ENTER (devc->mutex, flags);
  devc->open_mode |= open_mode;
  MUTEX_EXIT (devc->mutex, flags);
  return 0;
}

void
oss_spdif_close (spdif_devc * devc, int open_mode)
{
  oss_native_word flags;
  flags = 0;
  if (!devc->is_ok)
    {
      cmn_err (CE_WARN, "spdif: Bad usage - close\n");
      return;
    }
  MUTEX_ENTER (devc->mutex, flags);
  devc->open_mode &= ~open_mode;

  if (devc->mixer_blocked && (open_mode & OPEN_WRITE))
    {
      unblock_mixer (devc);
    }
  MUTEX_EXIT (devc->mutex, flags);
}

static void
update_rate_bits (spdif_devc * devc, int rate)
{
  unsigned char spd;
/*
 * Set S/PDIF rate control (TODO: Pro mode)
 */

  devc->hot_ctl.srate_out = rate;

  switch (rate)
    {
    case 48000:
      spd = 2;
      break;
    case 32000:
      spd = 3;
      break;
    case 88200:
      spd = 4;
      break;
    case 96000:
      spd = 5;
      break;
    case 176400:
      spd = 7;
      break;
    case 192000:
      spd = 6;
      break;
    default:
      spd = 0;
      rate = 44100;
    }

  devc->hot_ctl.rate_bits = spd;

  devc->hot_ctl.cbitout[3] &= ~0x0f;
  devc->hot_ctl.cbitout[3] |= spd & 0x0f;
  devc->hot_ctl.srate_out = rate;
}

void
oss_spdif_setrate (spdif_devc * devc, int rate)
{
  oss_native_word flags;
  flags = 0;
  if (!devc->is_ok)
    {
      cmn_err (CE_WARN, "spdif: Bad usage - setrate\n");
      return;
    }
  MUTEX_ENTER (devc->mutex, flags);

  update_rate_bits (devc, rate);

  if (devc->d->reprogram_device)
    devc->d->reprogram_device (devc->host_devc, devc->host_portc,
			       &devc->hot_ctl, REPGM_RATE | REPGM_CBIT3);
  MUTEX_EXIT (devc->mutex, flags);
}

static int spdif_mix_init (spdif_devc * devc);

/*ARGSUSED*/
static void
switch_mode (spdif_devc * devc, oss_digital_control * ctl, int smurf)
{
  ctl->pro_mode = ctl->cbitout[0] & 0x01;

  if (ctl == devc->ctl)		/* Visible to mixer */
    spdif_mix_init (devc);	/* Re-create mixer extensions */
}

static void
update_hardware (spdif_devc * devc, int repgm)
{
  if (repgm & REPGM_RATE)
    update_rate_bits (devc, devc->hot_ctl.srate_out);
  if (devc->d->reprogram_device)
    devc->d->reprogram_device (devc->host_devc, devc->host_portc,
			       &devc->hot_ctl, repgm);
}

static int
readctl (spdif_devc * devc, oss_digital_control * c, int open_mode)
{
  int valid = c->valid & ~(VAL_OUTMASK);
  int ret = 0;
  oss_native_word flags;
  flags = 0;

  MUTEX_ENTER (devc->mutex, flags);

  memcpy (c, &devc->hot_ctl, sizeof (devc->hot_ctl));
  memset (c->filler, 0, sizeof (c->filler));	/* Hide internal variables */
  c->valid &= (VAL_OUTMASK);

  if (open_mode & OPEN_READ)
    {
      if (devc->d->get_status)
	ret = devc->d->get_status (devc->host_devc, devc->host_portc,
				   c, valid);
      else
	{
	  if (valid != 0)	/* Can't get input status */
	    ret = OSS_EIO;
	}
    }

  MUTEX_EXIT (devc->mutex, flags);
  return ret;
}

/*ARGSUSED*/
static int
handle_request (spdif_devc * devc, oss_digital_control * c)
{
  return OSS_EIO;
}

static int
writectl (spdif_devc * devc, oss_digital_control * c)
{
  int valid = c->valid;
  int repgm = 0;
  int err = 0;
  oss_native_word flags;
  flags = 0;

  MUTEX_ENTER (devc->mutex, flags);
  if (!devc->mixer_blocked)
    block_mixer (devc);
  c->valid = 0;
  c->caps = devc->caps;

  if ((valid & VAL_CBITOUT) &&
      ((devc->caps & DIG_CBITOUT_MASK) != DIG_CBITOUT_NONE))
    {
      int smurf;

      smurf = (devc->hot_ctl.cbitout[0] ^ c->cbitout[0]) & 1;
      c->valid |= VAL_CBITOUT;
      repgm |= REPGM_CBITALL;

      if (smurf)		/* Mode changet CONSUMER<->PRO */
	{
	  repgm |= REPGM_ALL;
	  switch_mode (devc, &devc->hot_ctl, 0);
	}

      memcpy (&devc->hot_ctl.cbitout, &c->cbitout, sizeof (c->cbitout));
    }

  if ((valid & VAL_UBITOUT) && (devc->caps & DIG_UBITOUT))
    {
      c->valid |= VAL_UBITOUT;
      repgm |= REPGM_UBITALL;
      memcpy (&devc->hot_ctl.ubitout, &c->ubitout, sizeof (c->ubitout));
    }

  if ((c->out_vbit != VBIT_NOT_INDICATED) && (devc->caps & DIG_VBITOUT))
    {
      devc->hot_ctl.out_vbit = c->out_vbit;
      repgm |= REPGM_VBIT;
    }
  else
    c->out_vbit = VBIT_NOT_INDICATED;
  c->in_vbit = VBIT_NOT_INDICATED;

  devc->hot_ctl.valid = valid;

  if ((valid & VAL_ORATE))
    {
      /* TODO: Check the rate */
      update_rate_bits (devc, c->srate_out);
      c->valid |= VAL_ORATE;
      c->srate_out = devc->hot_ctl.srate_out;
    }

  if (valid & VAL_REQUEST)
    {
      err = handle_request (devc, c);
      if (err >= 0)
	c->valid |= VAL_REQUEST;
    }

  MUTEX_EXIT (devc->mutex, flags);
  return err;
}

int
oss_spdif_ioctl (spdif_devc * devc, int open_mode, unsigned int cmd,
		 ioctl_arg arg)
{
  if (!devc->is_ok)
    {
      cmn_err (CE_WARN, "spdif: Bad usage - ioctl\n");
      return SPDIF_NOIOCTL;	/* Not for me */
    }

  switch (cmd)
    {
    case SNDCTL_DSP_WRITECTL:
      if (!(open_mode & OPEN_WRITE))	/* Not opened for output */
	return OSS_ENOTSUP;
      if (!(devc->flags & SPDF_OUT))
	return OSS_ENOTSUP;
      return writectl (devc, (oss_digital_control *) arg);
      break;

    case SNDCTL_DSP_READCTL:
      if (!(devc->flags & (SPDF_OUT | SPDF_IN)))
	return OSS_ENOTSUP;
      return readctl (devc, (oss_digital_control *) arg, open_mode);
      break;
    }

  return SPDIF_NOIOCTL;		/* Not for me too */
}

static int
spdif_set_control (spdif_devc * devc, int ctrl, unsigned int cmd, int value)
{
  int val;

  if (cmd == SNDCTL_MIX_READ)
    {
      switch (ctrl)
	{
	case 1:		/* spdif.enable (OFF/ON) */
	  return !!(devc->ctl->outsel & OUTSEL_DIGITAL);
	  break;

	case 2:		/* spdif.mode (CONSUMER|PRO) */
	  return !!(devc->ctl->cbitout[0] & 1);
	  break;

	case 3:		/* spdif.audio (AUDIO/DATA) */
	  return !!(devc->ctl->cbitout[0] & 2);
	  break;

	case 4:		/* spdif.vbit (OFF/ON) */
	  return (devc->ctl->out_vbit == VBIT_ON);
	  break;

	case 5:		/* spdif.copyright (YES/NO) */
	  return get_cbit (devc->ctl->cbitout, 0, 2);
	  break;

	case 6:		/* spdif.generation (COPY/ORIGINAL) */
	  return get_cbit (devc->ctl->cbitout, 1, 0);
	  break;

	case 7:		/* spdif.preemph (OFF 50/16usec) */
	  return devc->ctl->emphasis_type;
	}

      return OSS_EINVAL;
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
      switch (ctrl)
	{
	case 1:		/* spdif.enable */
	  if (value)
	    devc->ctl->outsel |= OUTSEL_DIGITAL;
	  else
	    devc->ctl->outsel &= ~OUTSEL_DIGITAL;
	  update_hardware (devc, REPGM_OUTSEL);
	  return !!(devc->ctl->outsel & OUTSEL_DIGITAL);
	  break;

	case 2:		/* spdif.mode (CONSUMER|PRO) */
	  val = devc->ctl->cbitout[0] & 1;
	  value = !!value;

	  set_cbit (devc->ctl->cbitout, 0, 0, value);
	  if (val != value)	/* Mode change */
	    switch_mode (devc, devc->ctl, 1);
	  update_hardware (devc, REPGM_ALL);
	  return get_cbit (devc->ctl->cbitout, 0, 0);
	  break;

	case 3:		/* spdif.audio (AUDIO|DATA) */
	  value = !!value;

	  set_cbit (devc->ctl->cbitout, 0, 1, value);
	  update_hardware (devc, REPGM_CBIT0);
	  return get_cbit (devc->ctl->cbitout, 0, 1);
	  break;

	case 4:		/* spdif.vbit (OFF/ON) */
	  devc->ctl->out_vbit = (value) ? VBIT_ON : VBIT_OFF;
	  update_hardware (devc, REPGM_VBIT);
	  return (devc->ctl->out_vbit == VBIT_ON);
	  break;

	case 5:		/* spdif.copyright (YES|NO) */
	  value = !!value;

	  set_cbit (devc->ctl->cbitout, 0, 2, value);
	  update_hardware (devc, REPGM_CBIT0);
	  return get_cbit (devc->ctl->cbitout, 0, 2);
	  break;

	case 6:		/* spdif.generation (COPY|ORIGINAL) */
	  value = !!value;

	  set_cbit (devc->ctl->cbitout, 1, 0, value);
	  update_hardware (devc, REPGM_CBIT1);
	  return get_cbit (devc->ctl->cbitout, 1, 0);
	  break;

	case 7:		/* spdif.preemph (OFF 50/15usec) */
	  value = !!value;
	  devc->ctl->emphasis_type = value;
	  if (devc->ctl->pro_mode)
	    {
	      devc->ctl->cbitout[0] &= ~0x1c;
	      if (value)
		devc->ctl->cbitout[0] |= 0x0c;
	    }
	  else
	    {
	      devc->ctl->cbitout[0] &= ~0x28;
	      if (value)
		devc->ctl->cbitout[0] |= 0x08;
	    }
	  update_hardware (devc, REPGM_CBIT0);
	  return value;

	  break;
	}

      return OSS_EINVAL;
    }
  return OSS_EINVAL;
}

static int
spdif_set_mixer_control (int dev, int ctrl, unsigned int cmd, int value)
{
  int i, ret;
  spdif_devc *devc = NULL;
  oss_native_word flags;
  flags = 0;

  for (i = 0; i < ndevs && devc == NULL; i++)
    if (devc_list[i]->mixer_dev == dev)
      devc = devc_list[i];

  if (devc == NULL)
    return OSS_EINVAL;
  MUTEX_ENTER (devc->mutex, flags);
  ret = spdif_set_control (devc, ctrl, cmd, value);
  MUTEX_EXIT (devc->mutex, flags);
  return ret;
}

static int
spdif_mix_init (spdif_devc * devc)
{
  int group, err, dev;

  dev = devc->mixer_dev;

  if (!devc->is_ok)
    {
      cmn_err (CE_WARN, "spdif: Bad usage - mix_init\n");
      return OSS_EIO;
    }

  if (devc->group > 0)
    mixer_ext_truncate (devc->mixer_dev, devc->group);
  devc->group = 0;

  if ((group = mixer_ext_create_group_flags (dev, 0, "SPDIF", MIXF_FLAT)) < 0)
    return group;

  devc->group = group;

  if (devc->flags & SPDF_ENABLE)
    if ((err = mixer_ext_create_control (dev, group,
					 1, spdif_set_mixer_control,
					 MIXT_ONOFF,
					 "SPDIF_ENABLE", 2,
					 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
      return err;

  if ((devc->caps & DIG_CBITOUT_MASK))
    {
      if ((err = mixer_ext_create_control (dev, group,
					   3, spdif_set_mixer_control,
					   MIXT_ENUM,
					   "SPDIF_AUDIO", 2,
					   MIXF_READABLE | MIXF_WRITEABLE)) <
	  0)
	return err;
    }

  if (devc->caps & DIG_VBITOUT)
    {
      if ((err = mixer_ext_create_control (dev, group,
					   4, spdif_set_mixer_control,
					   MIXT_ONOFF,
					   "SPDIF_VBIT", 2,
					   MIXF_READABLE | MIXF_WRITEABLE)) <
	  0)
	return err;
    }

  if ((devc->caps & DIG_CBITOUT_MASK))
    {
      if ((err = mixer_ext_create_control (dev, group,
					   7, spdif_set_mixer_control,
					   MIXT_ENUM,
					   "SPDIF_PREEMPH", 2,
					   MIXF_READABLE | MIXF_WRITEABLE)) <
	  0)
	return err;
    }

  if ((devc->caps & DIG_PRO) && (devc->caps & DIG_CONSUMER))
    {
      /* Device supports both consumer and pro */
      if ((err = mixer_ext_create_control (dev, group,
					   2, spdif_set_mixer_control,
					   MIXT_ENUM,
					   "SPDIF_MODE", 2,
					   MIXF_READABLE | MIXF_WRITEABLE)) <
	  0)
	return err;
    }

  /* Copyright management */
  if ((devc->caps & DIG_CBITOUT_MASK) && !devc->ctl->pro_mode)
    {
      if ((err = mixer_ext_create_control (dev, group,
					   5, spdif_set_mixer_control,
					   MIXT_ENUM,
					   "SPDIF_COPYRIGHT", 2,
					   MIXF_READABLE | MIXF_WRITEABLE)) <
	  0)
	return err;

      if ((err = mixer_ext_create_control (dev, group,
					   6, spdif_set_mixer_control,
					   MIXT_ENUM,
					   "SPDIF_GENERAT", 2,
					   MIXF_READABLE | MIXF_WRITEABLE)) <
	  0)
	return err;
    }

  return 0;
}

int
oss_spdif_mix_init (spdif_devc * devc)
{
  int ret;
  oss_native_word flags;
  flags = 0;
  MUTEX_ENTER (devc->mutex, flags);
  ret = spdif_mix_init (devc);
  MUTEX_EXIT (devc->mutex, flags);
  return ret;
}
