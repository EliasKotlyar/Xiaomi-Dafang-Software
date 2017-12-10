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
/* Codec index is 0 */
/* Codec vendor 10ec:0880 */
/* HD codec revision 0.9 (5.0) (0x00090500) */
/* Subsystem ID 08800000 */
/* Default amplifier caps: in=00000000, out=00000000 */
#include "oss_hdaudio_cfg.h"
#include "hdaudio.h"
#include "hdaudio_codec.h"
#include "hdaudio_dedicated.h"

int
hdaudio_abit_AA8_mixer_init (int dev, hdaudio_mixer_t * mixer, int cad,
			     int top_group)
{
  int ctl = 0;

  DDB (cmn_err (CE_CONT, "hdaudio_abit_AA8_mixer_init got called.\n"));

  /* Handle PIN widgets */
  {
    int n, group, pin_group;

    n = 0;

    HDA_GROUP (pin_group, top_group, "jack");

    if (HDA_PIN_GROUP (0x14, group, pin_group, "green", n, "jack", 4))	/* Pin widget 0x14 */
      {
	/* Src 0xc=front */
	if (HDA_PINSELECT (0x14, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl, "front input");
	HDA_OUTMUTE (0x14, group, "mute", UNMUTE);
      }

    if (HDA_PIN_GROUP (0x15, group, pin_group, "black", n, "jack", 4))	/* Pin widget 0x15 */
      {
	/* Src 0xd=rear */
	if (HDA_PINSELECT (0x15, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl, "rear input");
	HDA_OUTMUTE (0x15, group, "mute", UNMUTE);
      }

    if (HDA_PIN_GROUP (0x16, group, pin_group, "C-L", n, "jack", 4))	/* Pin widget 0x16 */
      {
	/* Src 0xe=center/LFE */
	if (HDA_PINSELECT (0x16, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl, "center/LFE input");
	HDA_OUTMUTE (0x16, group, "mute", UNMUTE);
      }

    if (HDA_PIN_GROUP (0x17, group, pin_group, "surr", n, "jack", 4))	/* Pin widget 0x17 */
      {
	/* Src 0xf=side */
	if (HDA_PINSELECT (0x17, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl, "side input");
	HDA_OUTMUTE (0x17, group, "mute", UNMUTE);
      }

    if (HDA_PIN_GROUP (0x18, group, pin_group, "pink1", n, "jack", 4))	/* Pin widget 0x18 */
      {
	/* Src 0x10=out-source */
	if (HDA_PINSELECT (0x18, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl, "out-source input");
	HDA_OUTMUTE (0x18, group, "mute", UNMUTE);

	/* Widget 0x10 (out-source) */
	/* Src 0xc=front */
	/* Src 0xd=rear */
	/* Src 0xe=center/LFE */
	/* Src 0xf=side */
	if (HDA_SELECT (0x10, "out-source", ctl, group, -1))
	  {
	    HDA_CHOICES (ctl, "front rear center/LFE side");
	  }
      }

    if (HDA_PIN_GROUP (0x19, group, pin_group, "pink2", n, "jack", 4))	/* Pin widget 0x19 */
      {
	/* Src 0x11=out-source */
	if (HDA_PINSELECT (0x19, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl, "out-source input");
	HDA_OUTMUTE (0x19, group, "mute", UNMUTE);

	/* Widget 0x11 (out-source) */
	/* Src 0xc=front */
	/* Src 0xd=rear */
	/* Src 0xe=center/LFE */
	/* Src 0xf=side */
	if (HDA_SELECT (0x11, "out-source", ctl, group, -1))
	  {
	    HDA_CHOICES (ctl, "front rear center/LFE side");
	  }
      }

    if (HDA_PIN_GROUP (0x1a, group, pin_group, "blue1", n, "jack", 4))	/* Pin widget 0x1a */
      {
	/* Src 0x12=out-source */
	if (HDA_PINSELECT (0x1a, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl, "out-source input");
	HDA_OUTMUTE (0x1a, group, "mute", UNMUTE);

	/* Widget 0x12 (out-source) */
	/* Src 0xc=front */
	/* Src 0xd=rear */
	/* Src 0xe=center/LFE */
	/* Src 0xf=side */
	if (HDA_SELECT (0x12, "out-source", ctl, group, -1))
	  {
	    HDA_CHOICES (ctl, "front rear center/LFE side");
	  }
      }

    if (HDA_PIN_GROUP (0x1b, group, pin_group, "blue2", n, "jack", 4))	/* Pin widget 0x1b */
      {
	/* Src 0x13=out-source */
	if (HDA_PINSELECT (0x1b, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl, "out-source input");
	HDA_OUTMUTE (0x1b, group, "mute", UNMUTE);

	/* Widget 0x13 (out-source) */
	/* Src 0xc=front */
	/* Src 0xd=rear */
	/* Src 0xe=center/LFE */
	/* Src 0xf=side */
	if (HDA_SELECT (0x13, "out-source", ctl, group, -1))
	  {
	    HDA_CHOICES (ctl, "front rear center/LFE side");
	  }
      }

    if (HDA_PIN_GROUP (0x1c, group, pin_group, "cd", n, "jack", 4))	/* Pin widget 0x1c */
      {
	if (HDA_PINSELECT (0x1c, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl, "input");
      }

    if (HDA_PIN_GROUP (0x1d, group, pin_group, "beep", n, "jack", 4))	/* Pin widget 0x1d */
      {
	if (HDA_PINSELECT (0x1d, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl, "input");
      }
  }
  /* Handle ADC widgets */
  {
    int n, group, rec_group;

    n = 0;

    HDA_GROUP (rec_group, top_group, "record");

    if (HDA_ADC_GROUP (0x07, group, rec_group, "rec1", n, "record", 2))	/* ADC widget 0x07 */
      {
	/* Src 0x18=pink1 */
	/* Src 0x19=pink2 */
	/* Src 0x1a=blue1 */
	/* Src 0x1b=blue2 */
	/* Src 0x1c=cd */
	/* Src 0x14=green */
	/* Src 0x15=black */
	if (HDA_SELECT (0x07, "src", ctl, group, -1))
	  {
	    HDA_CHOICES (ctl, "pink1 pink2 blue1 blue2 cd green black");
	  }
	{
	  int amp_group;

	  HDA_GROUP (amp_group, group, "vol");
	  HDA_INAMP (0x07, 0, amp_group, "pink1", 90);	/* From widget 0x18 */
	  HDA_INAMP (0x07, 1, amp_group, "pink2", 90);	/* From widget 0x19 */
	  HDA_INAMP (0x07, 2, amp_group, "blue1", 90);	/* From widget 0x1a */
	  HDA_INAMP (0x07, 3, amp_group, "blue2", 90);	/* From widget 0x1b */
	  HDA_INAMP (0x07, 4, amp_group, "cd", 90);	/* From widget 0x1c */
	  HDA_INAMP (0x07, 5, amp_group, "green", 90);	/* From widget 0x14 */
	  HDA_INAMP (0x07, 6, amp_group, "black", 90);	/* From widget 0x15 */
	}
      }

    if (HDA_ADC_GROUP (0x08, group, rec_group, "rec2", n, "record", 2))	/* ADC widget 0x08 */
      {
	/* Src 0x18=pink1 */
	/* Src 0x19=pink2 */
	/* Src 0x1a=blue1 */
	/* Src 0x1b=blue2 */
	/* Src 0x1c=cd */
	/* Src 0x14=green */
	/* Src 0x15=black */
	if (HDA_SELECT (0x08, "src", ctl, group, -1))
	  {
	    HDA_CHOICES (ctl, "pink1 pink2 blue1 blue2 cd green black");
	  }
	{
	  int amp_group;

	  HDA_GROUP (amp_group, group, "vol");
	  HDA_INAMP (0x08, 0, amp_group, "pink1", 90);	/* From widget 0x18 */
	  HDA_INAMP (0x08, 1, amp_group, "pink2", 90);	/* From widget 0x19 */
	  HDA_INAMP (0x08, 2, amp_group, "blue1", 90);	/* From widget 0x1a */
	  HDA_INAMP (0x08, 3, amp_group, "blue2", 90);	/* From widget 0x1b */
	  HDA_INAMP (0x08, 4, amp_group, "cd", 90);	/* From widget 0x1c */
	  HDA_INAMP (0x08, 5, amp_group, "green", 90);	/* From widget 0x14 */
	  HDA_INAMP (0x08, 6, amp_group, "black", 90);	/* From widget 0x15 */
	}
      }

    if (HDA_ADC_GROUP (0x09, group, rec_group, "rec3", n, "record", 2))	/* ADC widget 0x09 */
      {
	/* Src 0x18=pink1 */
	/* Src 0x19=pink2 */
	/* Src 0x1a=blue1 */
	/* Src 0x1b=blue2 */
	/* Src 0x1c=cd */
	/* Src 0xb=inputmix */
	/* Src 0x14=green */
	/* Src 0x15=black */
	/* Src 0x16=C-L */
	/* Src 0x17=surr */
	if (HDA_SELECT (0x09, "src", ctl, group, -1))
	  {
	    HDA_CHOICES (ctl,
			 "pink1 pink2 blue1 blue2 cd inputmix green black C-L surr");
	  }
	{
	  int amp_group;

	  HDA_GROUP (amp_group, group, "vol");
	  HDA_INAMP (0x09, 0, amp_group, "pink1", 90);	/* From widget 0x18 */
	  HDA_INAMP (0x09, 1, amp_group, "pink2", 90);	/* From widget 0x19 */
	  HDA_INAMP (0x09, 2, amp_group, "blue1", 90);	/* From widget 0x1a */
	  HDA_INAMP (0x09, 3, amp_group, "blue2", 90);	/* From widget 0x1b */
	  HDA_INAMP (0x09, 4, amp_group, "cd", 90);	/* From widget 0x1c */
	  HDA_INAMP (0x09, 5, amp_group, "inputmix", 90);	/* From widget 0x0b */
	  HDA_INAMP (0x09, 6, amp_group, "green", 90);	/* From widget 0x14 */
	  HDA_INAMP (0x09, 7, amp_group, "black", 90);	/* From widget 0x15 */
	  HDA_INAMP (0x09, 8, amp_group, "C-L", 90);	/* From widget 0x16 */
	  HDA_INAMP (0x09, 9, amp_group, "surr", 90);	/* From widget 0x17 */
	}
      }

#if 0
    if (HDA_ADC_GROUP (0x0a, group, rec_group, "spdif-in", n, "record", 2))	/* ADC widget 0x0a */
      {
	/* Src 0x1f=spdin */
      }
#endif
  }
  /* Handle misc widgets */
  {
    int n, group, misc_group;

    n = 0;

    HDA_GROUP (misc_group, top_group, "misc");

    if (HDA_MISC_GROUP (0x0c, group, misc_group, "front", n, "misc", 8))	/* Misc widget 0x0c */
      {
	/* Src 0x2=front */
	/* Src 0xb=inputmix */
	HDA_OUTAMP (0x0c, group, "-", 90);
	{
	  int amp_group;

	  HDA_GROUP (amp_group, group, "mute");
	  HDA_INMUTE (0x0c, 0, amp_group, "front", UNMUTE);	/* From widget 0x02 */
	  HDA_INMUTE (0x0c, 1, amp_group, "inputmix", UNMUTE);	/* From widget 0x0b */
	}
      }

    if (HDA_MISC_GROUP (0x0d, group, misc_group, "rear", n, "misc", 8))	/* Misc widget 0x0d */
      {
	/* Src 0x3=rear */
	/* Src 0xb=inputmix */
	HDA_OUTAMP (0x0d, group, "-", 90);
	{
	  int amp_group;

	  HDA_GROUP (amp_group, group, "mute");
	  HDA_INMUTE (0x0d, 0, amp_group, "rear", UNMUTE);	/* From widget 0x03 */
	  HDA_INMUTE (0x0d, 1, amp_group, "inputmix", UNMUTE);	/* From widget 0x0b */
	}
      }

    if (HDA_MISC_GROUP (0x0e, group, misc_group, "center/LFE", n, "misc", 8))	/* Misc widget 0x0e */
      {
	/* Src 0x4=center/LFE */
	/* Src 0xb=inputmix */
	HDA_OUTAMP (0x0e, group, "-", 90);
	{
	  int amp_group;

	  HDA_GROUP (amp_group, group, "mute");
	  HDA_INMUTE (0x0e, 0, amp_group, "center/LFE", UNMUTE);	/* From widget 0x04 */
	  HDA_INMUTE (0x0e, 1, amp_group, "inputmix", UNMUTE);	/* From widget 0x0b */
	}
      }

    if (HDA_MISC_GROUP (0x0f, group, misc_group, "side", n, "misc", 8))	/* Misc widget 0x0f */
      {
	/* Src 0x5=side */
	/* Src 0xb=inputmix */
	HDA_OUTAMP (0x0f, group, "-", 90);
	{
	  int amp_group;

	  HDA_GROUP (amp_group, group, "mute");
	  HDA_INMUTE (0x0f, 0, amp_group, "side", UNMUTE);	/* From widget 0x05 */
	  HDA_INMUTE (0x0f, 1, amp_group, "inputmix", UNMUTE);	/* From widget 0x0b */
	}
      }

    if (HDA_MISC_GROUP (0x0b, group, misc_group, "inputmix", n, "misc", 0))	/* Misc widget 0x0b */
      {
	/* Src 0x18=pink1 */
	/* Src 0x19=pink2 */
	/* Src 0x1a=blue1 */
	/* Src 0x1b=blue2 */
	/* Src 0x1c=cd */
	/* Src 0x1d=beep */
	/* Src 0x14=green */
	/* Src 0x15=black */
	{
	  int amp_group;

	  HDA_GROUP (amp_group, group, "vol");
	  HDA_INAMP (0x0b, 0, amp_group, "pink1", 90);	/* From widget 0x18 */
	  HDA_INAMP (0x0b, 1, amp_group, "pink2", 90);	/* From widget 0x19 */
	  HDA_INAMP (0x0b, 2, amp_group, "blue1", 90);	/* From widget 0x1a */
	  HDA_INAMP (0x0b, 3, amp_group, "blue2", 90);	/* From widget 0x1b */
	  HDA_INAMP (0x0b, 4, amp_group, "cd", 90);	/* From widget 0x1c */
	  HDA_INAMP (0x0b, 5, amp_group, "beep", 90);	/* From widget 0x1d */
	  HDA_INAMP (0x0b, 6, amp_group, "green", 90);	/* From widget 0x14 */
	  HDA_INAMP (0x0b, 7, amp_group, "black", 90);	/* From widget 0x15 */
	}
      }
  }
  return 0;
}
