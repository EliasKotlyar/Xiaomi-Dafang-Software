
/*
 *
 * Purpose: AC97 chip specific Mixer extension handlers
 *
 * This file is included from ac97.c and it implements codec specific
 * extensions such as S/PDIF control.
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

extern int ac97_recselect;

void
ac97_enable_spdif (ac97_devc * devc)
{
  switch (devc->spdifout_support)
    {
    case CS_SPDIFOUT:
      codec_write (devc, 0x68, codec_read (devc, 0x68) | 0x8000);
      break;
    case CX_SPDIFOUT:
      codec_write (devc, 0x5c, codec_read (devc, 0x5c) | 0x08);
      break;
    case AD_SPDIFOUT:
    case ALC_SPDIFOUT:
    case VIA_SPDIFOUT:
    case STAC_SPDIFOUT:
    case CMI_SPDIFOUT:
    case YMF_SPDIFOUT:
      codec_write (devc, 0x2a,
		   (codec_read (devc, 0x2a) & 0xffc3) | 0x4 | devc->
		   spdif_slot);
      break;
    default:
      break;
    }
}

void
ac97_disable_spdif (ac97_devc * devc)
{
  switch (devc->spdifout_support)
    {
    case CS_SPDIFOUT:
      codec_write (devc, 0x68, codec_read (devc, 0x68) & ~0x8000);
      break;
    case AD_SPDIFOUT:
    case ALC_SPDIFOUT:
    case VIA_SPDIFOUT:
    case STAC_SPDIFOUT:
    case CMI_SPDIFOUT:
    case YMF_SPDIFOUT:
      codec_write (devc, 0x2a, (codec_read (devc, 0x2a) & 0xffc3) & ~0x4);
      break;
    case CX_SPDIFOUT:
      codec_write (devc, 0x5c, codec_read (devc, 0x5c) & ~0x8);
      break;
    default:
      break;
    }
}

/********************AC97 SPDIFIN Control *************************/
int
ac97_spdifin_ctl (int dev, int ctrl, unsigned int cmd, int value)
{
  ac97_devc *devc = mixer_devs[dev]->devc;


  if (cmd == SNDCTL_MIX_READ)
    {
      value = 0;
      switch (ctrl)
	{
	case SPDIFIN_ENABLE:	/* Enable/disable SPDIF IN */
	  if (devc->spdifin_support == ALC_SPDIFIN)
	    value = (codec_read (devc, 0x7A) & 0x8000) ? 1 : 0;
	  if (devc->spdifin_support == CMI_SPDIFIN)
	    value = (codec_read (devc, 0x6c) & 0x1) ? 1 : 0;
	  break;
	case SPDIFIN_PRO:	/* Pro */
	  if (devc->spdifin_support == ALC_SPDIFIN)
	    value = (codec_read (devc, 0x60) & 0x1) ? 1 : 0;
	  if (devc->spdifin_support == CMI_SPDIFIN)
	    value = (codec_read (devc, 0x68) & 0x1) ? 1 : 0;
	  break;
	case SPDIFIN_AUDIO:	/* /Audio */
	  if (devc->spdifin_support == ALC_SPDIFIN)
	    value = (codec_read (devc, 0x60) & 0x2) ? 1 : 0;
	  if (devc->spdifin_support == CMI_SPDIFIN)
	    value = (codec_read (devc, 0x68) & 0x1) ? 1 : 0;
	  break;
	case SPDIFIN_COPY:	/* Copy */
	  if (devc->spdifin_support == ALC_SPDIFIN)
	    value = (codec_read (devc, 0x60) & 0x4) ? 1 : 0;
	  if (devc->spdifin_support == CMI_SPDIFIN)
	    value = (codec_read (devc, 0x68) & 0x4) ? 1 : 0;
	  break;
	case SPDIFIN_PREEMPH:	/* Pre */
	  if (devc->spdifin_support == ALC_SPDIFIN)
	    value = (codec_read (devc, 0x60) & 0x38) >> 3;
	  if (devc->spdifin_support == CMI_SPDIFIN)
	    value = (codec_read (devc, 0x68) & 0x38) >> 3;
	  break;
	case SPDIFIN_MODE:	/* Mode */
	  if (devc->spdifin_support == ALC_SPDIFIN)
	    value = (codec_read (devc, 0x60) & 0xc0) ? 1 : 0;
	  if (devc->spdifin_support == CMI_SPDIFIN)
	    value = (codec_read (devc, 0x68) & 0xc0) ? 1 : 0;
	  break;
	case SPDIFIN_CATEGORY:	/* Category */
	  if (devc->spdifin_support == ALC_SPDIFIN)
	    value = (codec_read (devc, 0x60) & 0x7f00) >> 8;
	  if (devc->spdifin_support == CMI_SPDIFIN)
	    value = (codec_read (devc, 0x68) & 0x7f00) >> 8;
	  break;
	case SPDIFIN_GENERATION:	/* Level */
	  if (devc->spdifin_support == ALC_SPDIFIN)
	    value = (codec_read (devc, 0x60) & 0x8000) ? 1 : 0;
	  if (devc->spdifin_support == CMI_SPDIFIN)
	    value = (codec_read (devc, 0x68) & 0x8000) ? 1 : 0;
	  break;
	case SPDIFIN_SOURCE:	/* Source */
	  if (devc->spdifin_support == ALC_SPDIFIN)
	    value = codec_read (devc, 0x62) & 0xf;
	  if (devc->spdifin_support == CMI_SPDIFIN)
	    value = codec_read (devc, 0x6a) & 0xf;
	  break;
	case SPDIFIN_CHAN:	/* Channel */
	  if (devc->spdifin_support == ALC_SPDIFIN)
	    value = (codec_read (devc, 0x62) & 0xf0) >> 4;
	  if (devc->spdifin_support == CMI_SPDIFIN)
	    value = (codec_read (devc, 0x6a) & 0xf0) >> 4;
	  break;
	case SPDIFIN_RATE:	/* Sampling Rate */
	  if (devc->spdifin_support == ALC_SPDIFIN)
	    value = (codec_read (devc, 0x62) & 0xf00) >> 8;
	  if (devc->spdifin_support == CMI_SPDIFIN)
	    value = (codec_read (devc, 0x6a) & 0xf00) >> 8;
	  switch (value)
	    {
	    case 0:
	      value = 44100;
	      break;
	    case 2:
	      value = 48000;
	      break;
	    case 3:
	      value = 32000;
	      break;
	    }
	  break;
	case SPDIFIN_CLOCK:	/* Clock Accuracy */
	  if (devc->spdifin_support == ALC_SPDIFIN)
	    value = (codec_read (devc, 0x62) & 0x3000) >> 12;
	  if (devc->spdifin_support == CMI_SPDIFIN)
	    value = (codec_read (devc, 0x6a) & 0x3000) >> 12;
	  break;
	case SPDIFIN_SIGNAL:	/* Lock */
	  if (devc->spdifin_support == ALC_SPDIFIN)
	    value = (codec_read (devc, 0x62) & 0x4000) ? 1 : 0;
	  break;
	case SPDIFIN_VBIT:	/* VBit */
	  if (devc->spdifin_support == ALC_SPDIFIN)
	    value = (codec_read (devc, 0x62) & 0x8000) ? 1 : 0;
	  break;

	case SPDIFIN_MON:	/* SPDIF Input Mon */
	  if (devc->spdifin_support == ALC_SPDIFIN)
	    value = codec_read (devc, 0x6A) & (1 << 13) ? 1 : 0;
	  if (devc->spdifin_support == CMI_SPDIFIN)
	    value = codec_read (devc, 0x6c) & 0x8 ? 1 : 0;
	  break;

	default:
	  break;
	}
    }
  if (cmd == SNDCTL_MIX_WRITE)
    {
      switch (ctrl)
	{
	case SPDIFIN_ENABLE:	/* SPDIF Record Enable */
	  if (devc->spdifin_support == ALC_SPDIFIN)
	    {
	      if (value)
		{
		  /* Set 0x7a enable SPDIF Input                      */
		  /* set 0x6a: PCM Input from SPDIFIN                 */
		  codec_write (devc, 0x7a, codec_read (devc, 0x7a) | 0x2);
		  codec_write (devc, 0x6a, codec_read (devc, 0x6a) | 0x8000);
		}
	      else
		{
		  codec_write (devc, 0x7a, codec_read (devc, 0x7a) & ~0x2);
		  codec_write (devc, 0x6a, codec_read (devc, 0x6a) & ~0x8000);
		}
	    }

	  if (devc->spdifin_support == CMI_SPDIFIN)
	    {
	      if (value)
		{
		  codec_write (devc, 0x6c, codec_read (devc, 0x6c) | 1);
		  codec_write (devc, 0x6c, codec_read (devc, 0x6c) | 0x10);
		}
	      else
		{
		  codec_write (devc, 0x6c, codec_read (devc, 0x6c) & ~1);
		  codec_write (devc, 0x6c, codec_read (devc, 0x6c) & ~0x10);
		}
	    }
	  break;

	case SPDIFIN_MON:	/* SPDIF input monitor on analog DAC */
	  if (devc->spdifin_support == ALC_SPDIFIN)
	    {
	      if (value)
		codec_write (devc, 0x6a, codec_read (devc, 0x6a) | (1 << 13));
	      else
		codec_write (devc, 0x6a,
			     codec_read (devc, 0x6a) & ~(1 << 13));
	    }

	  if (devc->spdifin_support == CMI_SPDIFIN)
	    {
	      if (value)
		codec_write (devc, 0x6c, codec_read (devc, 0x6c) | 0x8);
	      else
		codec_write (devc, 0x6c, codec_read (devc, 0x6c) & ~0x8);
	    }
	  break;

	}
    }
  return value;
}

static int
spdifin_ext_init (int dev)
{
  int group, err;

  if ((group = mixer_ext_create_group_flags (dev, 0, "SPDIN", MIXF_FLAT)) < 0)
    return group;

  if ((err =
       mixer_ext_create_control (dev, group, SPDIFIN_ENABLE, ac97_spdifin_ctl,
				 MIXT_ONOFF, "SPDIN_Enable", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, SPDIFIN_MON, ac97_spdifin_ctl,
				 MIXT_ONOFF, "SPDIN_Monitor", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

#if 1
  if ((err =
       mixer_ext_create_control (dev, group, SPDIFIN_PRO, ac97_spdifin_ctl,
				 MIXT_VALUE, "SPDIN_Pro", 1,
				 MIXF_READABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, SPDIFIN_AUDIO, ac97_spdifin_ctl,
				 MIXT_VALUE, "SPDIN_Audio", 1,
				 MIXF_READABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, SPDIFIN_COPY, ac97_spdifin_ctl,
				 MIXT_VALUE, "SPDIN_Copy", 1,
				 MIXF_READABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, SPDIFIN_PREEMPH,
				 ac97_spdifin_ctl, MIXT_VALUE,
				 "SPDIN_Pre-emph", 1, MIXF_READABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, SPDIFIN_MODE, ac97_spdifin_ctl,
				 MIXT_VALUE, "SPDIN_Mode", 1,
				 MIXF_READABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, SPDIFIN_CATEGORY,
				 ac97_spdifin_ctl, MIXT_VALUE,
				 "SPDIN_CATEGORY", 1, MIXF_READABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, SPDIFIN_GENERATION,
				 ac97_spdifin_ctl, MIXT_VALUE,
				 "SPDIN_GenLevel", 1, MIXF_READABLE)) < 0)
    return err;
  if ((err =
       mixer_ext_create_control (dev, group, SPDIFIN_SOURCE, ac97_spdifin_ctl,
				 MIXT_VALUE, "SPDIN_Source", 1,
				 MIXF_READABLE)) < 0)
    return err;
  if ((err =
       mixer_ext_create_control (dev, group, SPDIFIN_CHAN, ac97_spdifin_ctl,
				 MIXT_VALUE, "SPDIN_Channel", 1,
				 MIXF_READABLE)) < 0)
    return err;
  if ((err =
       mixer_ext_create_control (dev, group, SPDIFIN_RATE, ac97_spdifin_ctl,
				 MIXT_VALUE, "SPDIN_Rate", 1,
				 MIXF_READABLE)) < 0)
    return err;
  if ((err =
       mixer_ext_create_control (dev, group, SPDIFIN_CLOCK, ac97_spdifin_ctl,
				 MIXT_VALUE, "SPDIN_Clock", 1,
				 MIXF_READABLE)) < 0)
    return err;
  if ((err =
       mixer_ext_create_control (dev, group, SPDIFIN_SIGNAL, ac97_spdifin_ctl,
				 MIXT_VALUE, "SPDIN_Signal", 1,
				 MIXF_READABLE)) < 0)
    return err;
  if ((err =
       mixer_ext_create_control (dev, group, SPDIFIN_VBIT, ac97_spdifin_ctl,
				 MIXT_VALUE, "SPDIN_VBit", 1,
				 MIXF_READABLE)) < 0)
    return err;
#endif
  return 0;
}

/********************AC97 SPDIFOUT Control *************************/
int
ac97_spdifout_ctl (int dev, int ctrl, unsigned int cmd, int value)
{
  int tmp = 0;
  static int flag;
  ac97_devc *devc = mixer_devs[dev]->devc;
  if (cmd == SNDCTL_MIX_READ)
    {
      value = 0;
      switch (ctrl)
	{
	case SPDIFOUT_ENABLE:	/* SPDIF OUT */
	  switch (devc->spdifout_support)
	    {
	    case CS_SPDIFOUT:
	      value = (codec_read (devc, 0x68) & 0x8000) ? 1 : 0;
	      break;
	    case CX_SPDIFOUT:
	      value = (codec_read (devc, 0x5c) & 0x08) ? 1 : 0;
	      break;
	    case AD_SPDIFOUT:
	    case ALC_SPDIFOUT:
	    case VIA_SPDIFOUT:
	    case STAC_SPDIFOUT:
	    case CMI_SPDIFOUT:
	    case YMF_SPDIFOUT:
	      value = (codec_read (devc, 0x2a) & 0x4) ? 1 : 0;
	      break;
	    }
	  break;

	case SPDIFOUT_PRO:	/* Consumer/PRO */
	  switch (devc->spdifout_support)
	    {
	    case CS_SPDIFOUT:
	      value = (codec_read (devc, 0x68) & 0x1) ? 1 : 0;
	      break;
	    case CX_SPDIFOUT:
	      value = (codec_read (devc, 0x5c) & 0x1) ? 1 : 0;
	      break;
	    case AD_SPDIFOUT:
	    case ALC_SPDIFOUT:
	    case VIA_SPDIFOUT:
	    case STAC_SPDIFOUT:
	    case CMI_SPDIFOUT:
	    case YMF_SPDIFOUT:
	      value = (codec_read (devc, 0x3a) & 0x1) ? 1 : 0;
	      break;
	    }
	  break;

	case SPDIFOUT_AUDIO:	/* PCM/AC3 */
	  switch (devc->spdifout_support)
	    {
	    case CS_SPDIFOUT:
	      value = (codec_read (devc, 0x68) & 0x2) ? 1 : 0;
	      break;
	    case CX_SPDIFOUT:
	      value = (codec_read (devc, 0x5c) & 0x2) ? 1 : 0;
	      break;
	    case AD_SPDIFOUT:
	    case ALC_SPDIFOUT:
	    case VIA_SPDIFOUT:
	    case STAC_SPDIFOUT:
	    case CMI_SPDIFOUT:
	    case YMF_SPDIFOUT:
	      value = (codec_read (devc, 0x3a) & 0x2) ? 1 : 0;
	      break;
	    }
	  break;

	case SPDIFOUT_COPY:	/* Copy Prot */
	  switch (devc->spdifout_support)
	    {
	    case CS_SPDIFOUT:
	      value = (codec_read (devc, 0x68) & 0x4) ? 1 : 0;
	      break;
	    case CX_SPDIFOUT:
	      value = (codec_read (devc, 0x5c) & 0x4) ? 1 : 0;
	      break;
	    case AD_SPDIFOUT:
	    case ALC_SPDIFOUT:
	    case VIA_SPDIFOUT:
	    case STAC_SPDIFOUT:
	    case CMI_SPDIFOUT:
	    case YMF_SPDIFOUT:
	      value = (codec_read (devc, 0x3a) & 0x4) ? 1 : 0;
	      break;
	    }
	  break;

	case SPDIFOUT_PREEMPH:	/* Pre emphasis */
	  switch (devc->spdifout_support)
	    {
	    case CS_SPDIFOUT:
	      value = codec_read (devc, 0x68) & 0x8 ? 1 : 0;
	      break;
	    case AD_SPDIFOUT:
	    case ALC_SPDIFOUT:
	    case VIA_SPDIFOUT:
	    case STAC_SPDIFOUT:
	    case CMI_SPDIFOUT:
	    case YMF_SPDIFOUT:
	      value = (codec_read (devc, 0x3a) & 0x8) ? 1 : 0;
	      break;
	    }
	  break;

	case SPDIFOUT_RATE:	/* Sampling Rate */
	  switch (devc->spdifout_support)
	    {
	    case CS_SPDIFOUT:
	      {
		tmp = codec_read (devc, 0x68) & 0x1000;
		switch (tmp)
		  {
		  case 0x1000:
		    value = 0;
		    break;
		  case 0x0000:
		    value = 1;
		    break;
		  default:
		    cmn_err (CE_NOTE, "unsupported SPDIF F/S rate\n");
		    break;
		  }

	      }
	      break;
	    case AD_SPDIFOUT:
	    case ALC_SPDIFOUT:
	    case VIA_SPDIFOUT:
	    case STAC_SPDIFOUT:
	    case CMI_SPDIFOUT:
	    case YMF_SPDIFOUT:
	      {
		tmp = codec_read (devc, 0x3a) & 0x3000;
		switch (tmp)
		  {
		  case 0x2000:
		    value = 0;
		    break;
		  case 0x0000:
		    value = 1;
		    break;
		  case 0x3000:
		    value = 2;
		    break;
		  default:
		    cmn_err (CE_NOTE, "unsupported SPDIF F/S rate\n");
		    value = 0;
		    break;
		  }
	      }
	      break;
	    }
	  break;

	case SPDIFOUT_VBIT:	/* V Bit */
	  switch (devc->spdifout_support)
	    {
	    case CS_SPDIFOUT:
	      value = codec_read (devc, 0x68) & 0x4000 ? 1 : 0;
	      break;
	    case AD_SPDIFOUT:
	    case ALC_SPDIFOUT:
	    case VIA_SPDIFOUT:
	    case STAC_SPDIFOUT:
	    case CMI_SPDIFOUT:
	    case YMF_SPDIFOUT:
	      value = codec_read (devc, 0x3a) & 0x8000 ? 1 : 0;
	      break;
	    }
	  break;

	case SPDIFOUT_ADC:	/* Analog In to SPDIF Out */
	  switch (devc->spdifout_support)
	    {
	    case ALC_SPDIFOUT:
	      value = codec_read (devc, 0x6a) & (1 << 12) ? 1 : 0;
	      break;

	    case AD_SPDIFOUT:
	      value = codec_read (devc, 0x74) & 0x4 ? 1 : 0;
	      break;

	    case CS_SPDIFOUT:
	      value = codec_read (devc, 0x5e) & (1 << 11) ? 1 : 0;
	      break;

	    case STAC_SPDIFOUT:
	      value = codec_read (devc, 0x6a) & 0x2 ? 1 : 0;
	      break;

	    case YMF_SPDIFOUT:
	      value = codec_read (devc, 0x66) & 0x2 ? 1 : 0;
	      break;

	    default:
	      break;
	    }
	}
    }
  if (cmd == SNDCTL_MIX_WRITE)
    {
      ac97_disable_spdif (devc);
      switch (ctrl)
	{
	case SPDIFOUT_ENABLE:	/* Enable SPDIF OUT */
	  if (value)
	    {
	      ac97_enable_spdif (devc);
	      flag = 1;
	    }
	  else
	    {
	      ac97_disable_spdif (devc);
	      flag = 0;
	    }
	  break;

	case SPDIFOUT_PRO:	/* consumer/pro audio */
	  switch (devc->spdifout_support)
	    {
	    case CS_SPDIFOUT:
	      if (value)
		codec_write (devc, 0x68, codec_read (devc, 0x68) | 1);
	      else
		codec_write (devc, 0x68, codec_read (devc, 0x68) & ~1);
	      break;
	    case CX_SPDIFOUT:
	      if (value)
		codec_write (devc, 0x5c, codec_read (devc, 0x5c) | 1);
	      else
		codec_write (devc, 0x5c, codec_read (devc, 0x5c) & ~1);
	      break;
	    case AD_SPDIFOUT:
	    case ALC_SPDIFOUT:
	    case VIA_SPDIFOUT:
	    case STAC_SPDIFOUT:
	    case CMI_SPDIFOUT:
	    case YMF_SPDIFOUT:
	      if (value)
		codec_write (devc, 0x3a, codec_read (devc, 0x3a) | 1);
	      else
		codec_write (devc, 0x3a, codec_read (devc, 0x3a) & ~1);
	      break;
	    }
	  break;

	case SPDIFOUT_AUDIO:	/* PCM/AC3 */
	  switch (devc->spdifout_support)
	    {
	    case CS_SPDIFOUT:
	      if (value)
		codec_write (devc, 0x68, codec_read (devc, 0x68) | 0x2);
	      else
		codec_write (devc, 0x68, codec_read (devc, 0x68) & ~2);
	      break;
	    case CX_SPDIFOUT:
	      if (value)
		codec_write (devc, 0x5c, codec_read (devc, 0x5c) | 0x2);
	      else
		codec_write (devc, 0x5c, codec_read (devc, 0x5c) & ~2);
	      break;
	    case AD_SPDIFOUT:
	    case ALC_SPDIFOUT:
	    case VIA_SPDIFOUT:
	    case STAC_SPDIFOUT:
	    case CMI_SPDIFOUT:
	    case YMF_SPDIFOUT:
	      if (value)
		codec_write (devc, 0x3a, codec_read (devc, 0x3a) | 0x2);
	      else
		codec_write (devc, 0x3a, codec_read (devc, 0x3a) & ~2);
	      break;
	    }
	  break;

	case SPDIFOUT_COPY:	/* copy prot */
	  switch (devc->spdifout_support)
	    {
	    case CS_SPDIFOUT:
	      if (value)
		codec_write (devc, 0x68, codec_read (devc, 0x68) | 4);
	      else
		codec_write (devc, 0x68, codec_read (devc, 0x68) & ~4);
	      break;
	    case CX_SPDIFOUT:
	      if (value)
		codec_write (devc, 0x5c, codec_read (devc, 0x5c) | 4);
	      else
		codec_write (devc, 0x5c, codec_read (devc, 0x5c) & ~4);
	      break;
	    case AD_SPDIFOUT:
	    case ALC_SPDIFOUT:
	    case VIA_SPDIFOUT:
	    case STAC_SPDIFOUT:
	    case CMI_SPDIFOUT:
	    case YMF_SPDIFOUT:
	      if (value)
		codec_write (devc, 0x3a, codec_read (devc, 0x3a) | 4);
	      else
		codec_write (devc, 0x3a, codec_read (devc, 0x3a) & ~4);
	      break;
	    }
	  break;

	case SPDIFOUT_PREEMPH:	/* preemphasis */
	  switch (devc->spdifout_support)
	    {
	    case CS_SPDIFOUT:
	      if (value)
		codec_write (devc, 0x68, codec_read (devc, 0x68) | 8);
	      else
		codec_write (devc, 0x68, codec_read (devc, 0x68) & ~8);
	      break;
	    case AD_SPDIFOUT:
	    case ALC_SPDIFOUT:
	    case VIA_SPDIFOUT:
	    case STAC_SPDIFOUT:
	    case CMI_SPDIFOUT:
	    case YMF_SPDIFOUT:
	      if (value)
		codec_write (devc, 0x3a, codec_read (devc, 0x3a) | 8);
	      else
		codec_write (devc, 0x3a, codec_read (devc, 0x3a) & ~8);
	      break;
	    }
	  break;

	case SPDIFOUT_RATE:	/* Frequency */
	  switch (devc->spdifout_support)
	    {
	    case CS_SPDIFOUT:
	      switch (value)
		{
		case 0:
		  codec_write (devc, 0x68,
			       (codec_read (devc, 0x68) & 0xEFFF) | 0x1000);
		  break;
		case 1:
		  codec_write (devc, 0x68,
			       (codec_read (devc, 0x68) & 0xEFFF) | 0);
		  break;
		default:
		  break;
		}
	      break;

	    case AD_SPDIFOUT:
	    case ALC_SPDIFOUT:
	    case VIA_SPDIFOUT:
	    case STAC_SPDIFOUT:
	    case CMI_SPDIFOUT:
	    case YMF_SPDIFOUT:
	      switch (value)
		{
		case 0:	/* 48000 */
		  codec_write (devc, 0x3a,
			       (codec_read (devc, 0x3a) & 0xcfff) | 0x2000);
		  break;
		case 1:	/* 44100 */
		  codec_write (devc, 0x3a,
			       (codec_read (devc, 0x3a) & 0xcfff) | 0x0);
		  break;
		case 2:	/* 32000 */
		  codec_write (devc, 0x3a,
			       (codec_read (devc, 0x3a) & 0xcfff) | 0x3000);
		  break;

		default:
		  break;
		}
	      break;
	    }
	  break;

	case SPDIFOUT_VBIT:	/* V Bit */
	  switch (devc->spdifout_support)
	    {
	    case CS_SPDIFOUT:
	      if (value)
		codec_write (devc, 0x68, codec_read (devc, 0x68) | 0x4000);
	      else
		codec_write (devc, 0x68, codec_read (devc, 0x68) & ~0x4000);
	      break;
	    case AD_SPDIFOUT:
	    case ALC_SPDIFOUT:
	    case VIA_SPDIFOUT:
	    case STAC_SPDIFOUT:
	    case CMI_SPDIFOUT:
	    case YMF_SPDIFOUT:
	      if (value)
		codec_write (devc, 0x3a, codec_read (devc, 0x3a) | 0x8000);
	      else
		codec_write (devc, 0x3a, codec_read (devc, 0x3a) & ~0x8000);
	      break;
	    }
	  break;

	case SPDIFOUT_ADC:	/* Analog In to SPDIF Out */
	  switch (devc->spdifout_support)
	    {
	    case ALC_SPDIFOUT:
	      if (value)
		codec_write (devc, 0x6a, codec_read (devc, 0x6a) | (1 << 12));
	      else
		codec_write (devc, 0x6a,
			     codec_read (devc, 0x6a) & ~(1 << 12));
	      break;

	    case AD_SPDIFOUT:
	      if (value)
		codec_write (devc, 0x74, codec_read (devc, 0x74) | 0x4);
	      else
		codec_write (devc, 0x74, codec_read (devc, 0x74) & ~0x4);
	      break;

	    case CS_SPDIFOUT:
	      if (value)
		codec_write (devc, 0x5e, codec_read (devc, 0x5e) | (1 << 11));
	      else
		codec_write (devc, 0x5e,
			     codec_read (devc, 0x5e) & ~(1 << 11));
	      break;

	    case STAC_SPDIFOUT:
	      if (value)
		codec_write (devc, 0x6a, codec_read (devc, 0x6a) | 0x2);
	      else
		codec_write (devc, 0x6a, codec_read (devc, 0x6a) & ~0x2);
	      break;

	    case CMI_SPDIFOUT:
	      if (value)
		codec_write (devc, 0x6c, codec_read (devc, 0x6c) | 0x2);
	      else
		codec_write (devc, 0x6c, codec_read (devc, 0x6c) & ~0x2);
	      break;

	    case YMF_SPDIFOUT:
	      if (value)
		codec_write (devc, 0x66, codec_read (devc, 0x66) | 0x2);
	      else
		codec_write (devc, 0x66, codec_read (devc, 0x66) & ~0x2);
	      break;
	    }
	  break;

	default:
	  break;
	}
      if (flag)
	ac97_enable_spdif (devc);
    }

  return value;
}

static int
spdifout_ext_init (int dev)
{
  int group, err;
  if ((group =
       mixer_ext_create_group_flags (dev, 0, "SPDOUT", MIXF_FLAT)) < 0)
    return group;
  if ((err =
       mixer_ext_create_control (dev, group, SPDIFOUT_ENABLE,
				 ac97_spdifout_ctl, MIXT_ONOFF,
				 "SPDOUT_ENABLE", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, SPDIFOUT_ADC,
				 ac97_spdifout_ctl, MIXT_ONOFF,
				 "SPDOUT_ADC/DAC", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, SPDIFOUT_PRO,
				 ac97_spdifout_ctl, MIXT_ENUM,
				 "SPDOUT_Pro", 2,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, SPDIFOUT_AUDIO,
				 ac97_spdifout_ctl, MIXT_ENUM,
				 "SPDOUT_Audio", 2,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, SPDIFOUT_COPY,
				 ac97_spdifout_ctl, MIXT_ONOFF,
				 "SPDOUT_Copy", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, SPDIFOUT_PREEMPH,
				 ac97_spdifout_ctl, MIXT_ONOFF,
				 "SPDOUT_Pre-emph", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, SPDIFOUT_RATE,
				 ac97_spdifout_ctl, MIXT_ENUM,
				 "SPDOUT_Rate", 3,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  if ((err =
       mixer_ext_create_control (dev, group, SPDIFOUT_VBIT,
				 ac97_spdifout_ctl, MIXT_ONOFF,
				 "SPDOUT_VBit", 1,
				 MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return err;

  return 0;
}

/**********************Mixer Extensions ********************/
int
ac97_mixext_ctl (int dev, int ctrl, unsigned int cmd, int value)
{
  int left, right;
  ac97_devc *devc = mixer_devs[dev]->devc;

  if (cmd == SNDCTL_MIX_READ)
    {
      value = 0;
      switch (ctrl)
	{
	case VOL_CENTER:	/* Left line in to output connection */
	  value = devc->extmixlevels[CENTER_VOL];	/* LFE/Center */
	  break;

	case VOL_REAR:
	  value = devc->extmixlevels[REAR_VOL];	/* Rear/Surround */
	  break;

	case VOL_SIDE:
	  value = devc->extmixlevels[SIDE_VOL];	/* Side Surround */
	  break;

	case REAR2LINE:	/* Rear to Line In */
	  if (devc->mixer_ext == ALC650_MIXER_EXT)
	    value = (codec_read (devc, 0x6A) & 0x200) ? 1 : 0;
	  if (devc->mixer_ext == CMI9739_MIXER_EXT)
	    value = (codec_read (devc, 0x64) & 0x400) ? 1 : 0;
	  if (devc->mixer_ext == STAC9758_MIXER_EXT)
	    value = (codec_read (devc, 0x64) & 0x50) ? 1 : 0;
	  if (devc->mixer_ext == AD1980_MIXER_EXT)
	    value = (codec_read (devc, 0x76) & (1 << 11)) ? 0 : 1;
	  break;

	case CENTER2MIC:	/* Center to Mic */
	  if (devc->mixer_ext == ALC650_MIXER_EXT)
	    value = (codec_read (devc, 0x6A) & 0x400) ? 1 : 0;
	  if (devc->mixer_ext == CMI9739_MIXER_EXT)
	    value = (codec_read (devc, 0x64) & 0x1000) ? 1 : 0;
	  if (devc->mixer_ext == STAC9758_MIXER_EXT)
	    value = (codec_read (devc, 0x64) & 0x0C) ? 1 : 0;
	  if (devc->mixer_ext == AD1980_MIXER_EXT)
	    value = (codec_read (devc, 0x76) & (1 << 12)) ? 0 : 1;
	  break;

	case SPREAD:		/* Duplicate front on lfe/rear */
	  if (devc->mixer_ext == ALC650_MIXER_EXT)
	    value = (codec_read (devc, 0x6A) & 0x1) ? 1 : 0;
	  if (devc->mixer_ext == AD1980_MIXER_EXT)
	    value = (codec_read (devc, 0x76) & 0x80) ? 1 : 0;
	  if (devc->mixer_ext == VIA1616_MIXER_EXT)
	    value = (codec_read (devc, 0x5a) & 0x8000) ? 1 : 0;
	  break;

	case MICBOOST:		/* 30db Mic boost */
	  if (devc->mixer_ext == AD1980_MIXER_EXT)
	    value = (codec_read (devc, 0x76) & 0x2) ? 1 : 0;
	  if (devc->mixer_ext == CMI9739_MIXER_EXT)
	    value = (codec_read (devc, 0x64) & 0x1) ? 1 : 0;
	  break;

	case DOWNMIX_LFE:	/* Downmix LFE to Front */
	  if (devc->mixer_ext == ALC650_MIXER_EXT)
	    value = (codec_read (devc, 0x6A) & 0x4) ? 1 : 0;
	  if (devc->mixer_ext == AD1980_MIXER_EXT)
	    value = (codec_read (devc, 0x76) & 0x100) ? 1 : 0;
	  if (devc->mixer_ext == VIA1616_MIXER_EXT)
	    value = (codec_read (devc, 0x5a) & 0x1000) ? 1 : 0;
	  break;

	case DOWNMIX_REAR:	/* Downmix Rear to Front */
	  if (devc->mixer_ext == ALC650_MIXER_EXT)
	    value = (codec_read (devc, 0x6A) & 0x2) ? 1 : 0;
	  if (devc->mixer_ext == VIA1616_MIXER_EXT)
	    value = (codec_read (devc, 0x5a) & 0x800) ? 1 : 0;
	  break;
	}
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
      switch (ctrl)
	{
	case VOL_CENTER:	/* Center/LFE volume */
	  left = value & 0xff;
	  right = (value >> 8) & 0xff;
	  if (left > 100)
	    left = 100;
	  if (right > 100)
	    right = 100;
	  value = left | (right << 8);
	  if (devc->mixer_ext == ALC650_MIXER_EXT)
	    codec_write (devc, 0x66,
			 mix_scale (left, right, devc->centervol_bits));
	  devc->extmixlevels[CENTER_VOL] = value;
	  break;

	case VOL_REAR:		/* Rear/Surround volume */
	  left = value & 0xff;
	  right = (value >> 8) & 0xff;
	  if (left > 100)
	    left = 100;
	  if (right > 100)
	    right = 100;
	  value = left | (right << 8);
	  if (devc->mixer_ext == ALC650_MIXER_EXT)
	    codec_write (devc, 0x64,
			 mix_scale (left, right, devc->rearvol_bits));
	  devc->extmixlevels[REAR_VOL] = value;
	  break;

	case VOL_SIDE:		/* Side Surround volume */
	  left = value & 0xff;
	  right = (value >> 8) & 0xff;
	  if (left > 100)
	    left = 100;
	  if (right > 100)
	    right = 100;
	  value = left | (right << 8);
	  if (devc->mixer_ext == CMI9780_MIXER_EXT)
	    codec_write (devc, 0x60,
			 mix_scale (left, right, devc->sidevol_bits));
	  devc->extmixlevels[SIDE_VOL] = value;
	  break;

	case REAR2LINE:	/* Surround to Line In */
	  if (devc->mixer_ext == ALC650_MIXER_EXT)
	    {
	      if (value)
		codec_write (devc, 0x6a, codec_read (devc, 0x6a) | 0x200);
	      else
		codec_write (devc, 0x6a, codec_read (devc, 0x6a) & ~0x200);
	    }

	  if ((devc->mixer_ext == CMI9739_MIXER_EXT)
	      || (devc->mixer_ext == CMI9780_MIXER_EXT))
	    {
	      if (value)
		codec_write (devc, 0x64, codec_read (devc, 0x64) | 0x400);
	      else
		codec_write (devc, 0x64, codec_read (devc, 0x64) & ~0x400);
	    }

	  if (devc->mixer_ext == STAC9758_MIXER_EXT)
	    {
	      if (value)
		codec_write (devc, 0x64, codec_read (devc, 0x64) | 0x50);
	      else
		codec_write (devc, 0x64, codec_read (devc, 0x64) & ~0x50);
	    }

	  if (devc->mixer_ext == AD1980_MIXER_EXT)
	    {
	      if (value)
		codec_write (devc, 0x76,
			     codec_read (devc, 0x76) & ~(1 << 11));
	      else
		codec_write (devc, 0x76, codec_read (devc, 0x76) | (1 << 11));
	    }

	  break;

	case CENTER2MIC:	/* Center/LFE to Mic In */
	  if (devc->mixer_ext == ALC650_MIXER_EXT)
	    {
	      if (value)
		codec_write (devc, 0x6a, codec_read (devc, 0x6a) | 0x400);
	      else
		codec_write (devc, 0x6a, codec_read (devc, 0x6a) & ~0x400);
	    }

	  if ((devc->mixer_ext == CMI9739_MIXER_EXT)
	      || (devc->mixer_ext == CMI9780_MIXER_EXT))
	    {
	      if (value)
		codec_write (devc, 0x64, codec_read (devc, 0x64) | 0x1000);
	      else
		codec_write (devc, 0x64, codec_read (devc, 0x64) & ~0x1000);
	    }

	  if (devc->mixer_ext == STAC9758_MIXER_EXT)
	    {
	      if (value)
		codec_write (devc, 0x64, codec_read (devc, 0x64) | 0xC);
	      else
		codec_write (devc, 0x64, codec_read (devc, 0x64) & ~0xC);
	    }

	  if (devc->mixer_ext == AD1980_MIXER_EXT)
	    {
	      if (value)
		codec_write (devc, 0x76,
			     codec_read (devc, 0x76) & ~(1 << 12));
	      else
		codec_write (devc, 0x76, codec_read (devc, 0x76) | (1 << 12));
	    }
	  break;

	case SPREAD:		/* 4 Speaker output - LF/RF duplicate on Rear */
	  if (devc->mixer_ext == ALC650_MIXER_EXT)
	    {
	      if (value)
		{
		  /* enable Line-In jack to play Surround out */
		  codec_write (devc, 0x6a, codec_read (devc, 0x6a) | 0x200);
		  codec_write (devc, 0x6a, codec_read (devc, 0x6a) | 1);
		}
	      else
		{
		  /* disable Line-In jack to play Surround out */
		  codec_write (devc, 0x6a, codec_read (devc, 0x6a) & ~0x200);
		  codec_write (devc, 0x6a, codec_read (devc, 0x6a) & ~1);
		}
	    }

	  if (devc->mixer_ext == AD1980_MIXER_EXT)
	    {
	      if (value)
		codec_write (devc, 0x76, codec_read (devc, 0x76) | 0x80);
	      else
		codec_write (devc, 0x76, codec_read (devc, 0x76) & ~0x80);
	    }

	  if (devc->mixer_ext == VIA1616_MIXER_EXT)
	    {
	      if (value)
		codec_write (devc, 0x5a, codec_read (devc, 0x5a) | 0x8000);
	      else
		codec_write (devc, 0x5a, codec_read (devc, 0x5a) & ~0x8000);
	    }
	  break;

	case MICBOOST:
	  if (devc->mixer_ext == AD1980_MIXER_EXT)
	    {
	      if (value)
		codec_write (devc, 0x76, codec_read (devc, 0x76) | 0x2);
	      else
		codec_write (devc, 0x76, codec_read (devc, 0x76) & ~0x2);
	    }

	  if ((devc->mixer_ext == CMI9739_MIXER_EXT)
	      || (devc->mixer_ext == CMI9780_MIXER_EXT))
	    {
	      if (value)
		codec_write (devc, 0x64, codec_read (devc, 0x64) | 0x1);
	      else
		codec_write (devc, 0x64, codec_read (devc, 0x64) & ~0x1);
	    }
	  break;

	case DOWNMIX_LFE:	/* Downmix LFE to Front */
	  if (devc->mixer_ext == ALC650_MIXER_EXT)
	    {
	      if (value)
		codec_write (devc, 0x6a, codec_read (devc, 0x6a) | 0x4);
	      else
		codec_write (devc, 0x6a, codec_read (devc, 0x6a) & ~0x4);
	    }

	  if (devc->mixer_ext == AD1980_MIXER_EXT)
	    {
	      if (value)
		codec_write (devc, 0x76, codec_read (devc, 0x76) | 0x300);
	      else
		codec_write (devc, 0x76, codec_read (devc, 0x76) & ~0x300);
	    }

	  if (devc->mixer_ext == VIA1616_MIXER_EXT)
	    {
	      if (value)
		codec_write (devc, 0x5a, codec_read (devc, 0x5a) | 0x1000);
	      else
		codec_write (devc, 0x5a, codec_read (devc, 0x5a) & ~0x1000);
	    }
	  break;

	case DOWNMIX_REAR:	/* Downmix Rear to Front */
	  if (devc->mixer_ext == ALC650_MIXER_EXT)
	    {
	      if (value)
		codec_write (devc, 0x6a, codec_read (devc, 0x6a) | 0x2);
	      else
		codec_write (devc, 0x6a, codec_read (devc, 0x6a) & ~0x2);
	    }

	  if (devc->mixer_ext == VIA1616_MIXER_EXT)
	    {
	      if (value)
		codec_write (devc, 0x5a, codec_read (devc, 0x5a) | 0x800);
	      else
		codec_write (devc, 0x5a, codec_read (devc, 0x5a) & ~0x800);
	    }
	  break;
	}
    }
  return value;
}

static int
ac97_micboost_ctl (int dev, int ctrl, unsigned int cmd, int value)
{
  ac97_devc *devc = mixer_devs[dev]->devc;
  int tmp;

  if (cmd == SNDCTL_MIX_READ)
    {
	return !!devc->micboost;
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
	value = !!value;

	tmp = codec_read (devc, 0x0e);

	if (value)
	  devc->micboost = 0x40;
	else
	  devc->micboost = 0x00;

	codec_write (devc, 0x0e, (tmp & ~0x40) | devc->micboost);

	return value;
    }

  return OSS_EINVAL;
}

static int
ac97_recselect_ctl (int dev, int ctrl, unsigned int cmd, int value)
{
  int tmp;
  ac97_devc *devc = mixer_devs[dev]->devc;

  if (ctrl < 0 || ctrl > 1)
    return OSS_EINVAL;

  tmp = codec_read (devc, 0x1a);

  if (cmd == SNDCTL_MIX_READ)
    {
      return (tmp >> (ctrl * 8)) & 0x07;
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
      if (value < 0 || value > 7)
	return OSS_EINVAL;

      tmp &= ~(0x7 << (ctrl * 8));
      tmp |= (value << (ctrl * 8));
      codec_write (devc, 0x1a, tmp);
      return value;
    }

  return OSS_EINVAL;
}

static int
ac97_mixext_init (int dev)
{
  ac97_devc *devc = mixer_devs[dev]->devc;
  unsigned int mixflags = 0;

  int group, err, ctl;

  if ((ctl =
     mixer_ext_create_control (dev, 0, 0,
			     ac97_micboost_ctl, MIXT_ONOFF,
			     "micboost", 1,
			     MIXF_READABLE | MIXF_WRITEABLE)) < 0)
     return ctl;

  if (ac97_recselect)
    {
      if ((group =
	   mixer_ext_create_group_flags (dev, 0, "AC97_RECSEL",
					 MIXF_FLAT)) < 0)
	return group;

      if ((ctl =
	   mixer_ext_create_control (dev, group, 1,
				     ac97_recselect_ctl, MIXT_ENUM,
				     "LEFT", 8,
				     MIXF_READABLE | MIXF_WRITEABLE)) < 0)
	return ctl;
      mixer_ext_set_strings (dev, ctl,
			     "MIC CD VIDEO AUX LINE STEREOMIX MONOMIX PHONE",
			     0);

      if ((ctl =
	   mixer_ext_create_control (dev, group, 0,
				     ac97_recselect_ctl, MIXT_ENUM,
				     "RIGHT", 8,
				     MIXF_READABLE | MIXF_WRITEABLE)) < 0)
	return ctl;
      mixer_ext_set_strings (dev, ctl,
			     "MIC CD VIDEO AUX LINE STEREOMIX MONOMIX PHONE",
			     0);
    }

/*
 * Don't use flat groups with AC97 chips that have slider controls (ALC650 at this moment)
 */
  if (devc->mixer_ext != ALC650_MIXER_EXT)
    mixflags = MIXF_FLAT;

  if ((group =
       mixer_ext_create_group_flags (dev, 0, "AC97_MIXEXT", mixflags)) < 0)
    return group;

  if ((devc->mixer_ext == ALC650_MIXER_EXT)
      || (devc->mixer_ext == CMI9780_MIXER_EXT))
    {
      if ((err =
	   mixer_ext_create_control (dev, group, VOL_CENTER,
				     ac97_mixext_ctl, MIXT_STEREOSLIDER,
				     "CENTERVOL", 100,
				     MIXF_READABLE | MIXF_WRITEABLE)) < 0)
	return err;
      ac97_mixext_ctl (dev, VOL_CENTER, SNDCTL_MIX_WRITE, 100 | (100 << 8));

      if ((err =
	   mixer_ext_create_control (dev, group, VOL_REAR, ac97_mixext_ctl,
				     MIXT_STEREOSLIDER, "REARVOL", 100,
				     MIXF_READABLE | MIXF_WRITEABLE)) < 0)
	return err;
      ac97_mixext_ctl (dev, VOL_REAR, SNDCTL_MIX_WRITE, 100 | (100 << 8));

      if ((err =
	   mixer_ext_create_control (dev, group, VOL_SIDE, ac97_mixext_ctl,
				     MIXT_STEREOSLIDER, "SIDEVOL", 100,
				     MIXF_READABLE | MIXF_WRITEABLE)) < 0)
	return err;
      ac97_mixext_ctl (dev, VOL_SIDE, SNDCTL_MIX_WRITE, 100 | (100 << 8));
    }


  if ((devc->mixer_ext == CMI9739_MIXER_EXT)
      || (devc->mixer_ext == CMI9780_MIXER_EXT)
      || (devc->mixer_ext == ALC650_MIXER_EXT)
      || (devc->mixer_ext == AD1980_MIXER_EXT)
      || (devc->mixer_ext == STAC9758_MIXER_EXT))
    {
      if ((err =
	   mixer_ext_create_control (dev, group, REAR2LINE,
				     ac97_mixext_ctl, MIXT_ONOFF,
				     "REAR2LINEJACK", 1,
				     MIXF_READABLE | MIXF_WRITEABLE)) < 0)
	return err;
    }

  if ((devc->mixer_ext == CMI9739_MIXER_EXT)
      || (devc->mixer_ext == CMI9780_MIXER_EXT)
      || (devc->mixer_ext == ALC650_MIXER_EXT)
      || (devc->mixer_ext == AD1980_MIXER_EXT)
      || (devc->mixer_ext == STAC9758_MIXER_EXT))
    {

      if ((err =
	   mixer_ext_create_control (dev, group, CENTER2MIC,
				     ac97_mixext_ctl, MIXT_ONOFF,
				     "CENTER2MICJACK", 1,
				     MIXF_READABLE | MIXF_WRITEABLE)) < 0)
	return err;
    }

  if ((devc->mixer_ext == AD1980_MIXER_EXT)
      || (devc->mixer_ext == VIA1616_MIXER_EXT)
      || (devc->mixer_ext == ALC650_MIXER_EXT))
    {

      if ((err =
	   mixer_ext_create_control (dev, group, SPREAD,
				     ac97_mixext_ctl, MIXT_ENUM,
				     "SPKMODE", 2,
				     MIXF_READABLE | MIXF_WRITEABLE)) < 0)
	return err;
    }

  if ((devc->mixer_ext == AD1980_MIXER_EXT)
      || (devc->mixer_ext == VIA1616_MIXER_EXT)
      || (devc->mixer_ext == ALC650_MIXER_EXT))
    {
      if ((err =
	   mixer_ext_create_control (dev, group, DOWNMIX_LFE,
				     ac97_mixext_ctl, MIXT_ONOFF,
				     "MIX-LFE2FRONT", 1,
				     MIXF_READABLE | MIXF_WRITEABLE)) < 0)
	return err;
    }

  if ((devc->mixer_ext == VIA1616_MIXER_EXT)
      || (devc->mixer_ext == ALC650_MIXER_EXT))
    {
      if ((err =
	   mixer_ext_create_control (dev, group, DOWNMIX_REAR,
				     ac97_mixext_ctl, MIXT_ONOFF,
				     "MIX-REAR2FRONT", 1,
				     MIXF_READABLE | MIXF_WRITEABLE)) < 0)
	return err;
    }

  if ((devc->mixer_ext == AD1980_MIXER_EXT)
      || (devc->mixer_ext == CMI9739_MIXER_EXT)
      || (devc->mixer_ext == CMI9780_MIXER_EXT))
    {
      if ((err =
	   mixer_ext_create_control (dev, group, MICBOOST,
				     ac97_mixext_ctl, MIXT_ONOFF,
				     "MICBOOST", 1,
				     MIXF_READABLE | MIXF_WRITEABLE)) < 0)
	return err;
    }
  return 0;
}

/********************END of Mixer Extensions **************/

/* AC97 Mixer Extensions */

static int
ac97mixer_ext_init (int dev)
{
  ac97_devc *devc = mixer_devs[dev]->devc;
  int err;

  if (devc->client_mixer_init != NULL)
    if ((err = devc->client_mixer_init (dev)) < 0)
      return err;

  if ((devc->mixer_ext > 0) || ac97_recselect)
    if ((err = ac97_mixext_init (dev)) < 0)
      return err;

  if (devc->spdifout_support)
    if ((err = spdifout_ext_init (dev)) < 0)
      return err;

  if (devc->spdifin_support)
    if ((err = spdifin_ext_init (dev)) < 0)
      return err;
  return 0;
}

void
ac97_spdif_setup (int dev, int speed, int bits)
{
  int val;

  val = ac97_spdifout_ctl (dev, SPDIFOUT_RATE, SNDCTL_MIX_READ, 0);

  mixer_devs[dev]->modify_counter++;

  switch (speed)
    {
    case 48000:
      if (val != 0)
	ac97_spdifout_ctl (dev, SPDIFOUT_RATE, SNDCTL_MIX_WRITE, 0);
      break;

    case 44100:
      if (val != 1)
	ac97_spdifout_ctl (dev, SPDIFOUT_RATE, SNDCTL_MIX_WRITE, 1);
      break;

    case 32000:
      if (val != 2)
	ac97_spdifout_ctl (dev, SPDIFOUT_RATE, SNDCTL_MIX_WRITE, 2);
      break;

    default:
      break;
    }

  if (bits == AFMT_AC3)
     ac97_spdifout_ctl (dev, SPDIFOUT_AUDIO, SNDCTL_MIX_WRITE, 1);
  else
     ac97_spdifout_ctl (dev, SPDIFOUT_AUDIO, SNDCTL_MIX_WRITE, 0);
}
