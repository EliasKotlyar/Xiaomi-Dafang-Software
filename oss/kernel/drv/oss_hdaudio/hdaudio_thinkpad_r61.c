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
/* Codec vendor 11d4:1984 */
/* HD codec revision 1.0 (4.0) (0x00100400) */
/* Subsystem ID 17aa20bb */
/* Default amplifier caps: in=80000000, out=00052727 */
#include "oss_hdaudio_cfg.h"
#include "hdaudio.h"
#include "hdaudio_codec.h"
#include "hdaudio_dedicated.h"

int
hdaudio_thinkpad_r61_mixer_init (int dev, hdaudio_mixer_t * mixer, int cad, int top_group)
{
  int ctl=0;

  DDB(cmn_err(CE_CONT, "hdaudio_thinkpad_r61_mixer_init got called.\n"));

  /* Handle PIN widgets */
  {
	int n, group, pin_group;

	n=0;

	HDA_GROUP(pin_group, top_group, "jack");

	if (HDA_PIN_GROUP(0x11, group, pin_group, "green", n, "jack", 4))	/* Pin widget 0x11 */
	   {
		/* Src 0x7=headphone-mix */
		if (HDA_PINSELECT(0x11, ctl, group, "mode", -1))
			HDA_CHOICES(ctl, "headphone-mix-out input");
		HDA_OUTMUTE(0x11, group, "inmute", UNMUTE);
	   }

	if (HDA_PIN_GROUP(0x12, group, pin_group, "int-speaker", n, "jack", 4))	/* Pin widget 0x12 */
	   {
		/* Src 0xa=lineout-mix */
		if (HDA_PINSELECT(0x12, ctl, group, "mode", -1))
			HDA_CHOICES(ctl, "lineout-mix-out input");
		HDA_OUTMUTE(0x12, group, "inmute", UNMUTE);
	   }

	if (HDA_PIN_GROUP(0x13, group, pin_group, "int-speaker", n, "jack", 4))	/* Pin widget 0x13 */
	   {
		/* Src 0x1f=mono-mix */
		if (HDA_PINSELECT(0x13, ctl, group, "mode", -1))
			HDA_CHOICES(ctl, "mono-mix-out input");
		HDA_OUTAMP(0x13, group, "invol", 90);
	   }

	if (HDA_PIN_GROUP(0x14, group, pin_group, "red", n, "jack", 4))	/* Pin widget 0x14 */
	   {
		if (HDA_PINSELECT(0x14, ctl, group, "mode", -1))
			HDA_CHOICES(ctl, "input");
		HDA_INAMP(0x14, 0, group, "out", 0);	/* From widget 0x00 */
	   }

	if (HDA_PIN_GROUP(0x15, group, pin_group, "int-mic", n, "jack", 4))	/* Pin widget 0x15 */
	   {
		if (HDA_PINSELECT(0x15, ctl, group, "mode", -1))
			HDA_CHOICES(ctl, "input");
		HDA_INAMP(0x15, 0, group, "out", 0);	/* From widget 0x00 */
	   }

	if (HDA_PIN_GROUP(0x16, group, pin_group, "int-cd", n, "jack", 4))	/* Pin widget 0x16 */
	   {
		/* Src 0xb=aux-mix */
		if (HDA_PINSELECT(0x16, ctl, group, "mode", -1))
			HDA_CHOICES(ctl, "aux-mix-out input");
		HDA_OUTMUTE(0x16, group, "inmute", UNMUTE);
	   }

	if (HDA_PIN_GROUP(0x17, group, pin_group, "int-mic", n, "jack", 4))	/* Pin widget 0x17 */
	   {
		if (HDA_PINSELECT(0x17, ctl, group, "mode", -1))
			HDA_CHOICES(ctl, "input");
	   }

	if (HDA_PIN_GROUP(0x18, group, pin_group, "int-mic", n, "jack", 4))	/* Pin widget 0x18 */
	   {
		if (HDA_PINSELECT(0x18, ctl, group, "mode", -1))
			HDA_CHOICES(ctl, "input");
	   }

	if (HDA_PIN_GROUP(0x1a, group, pin_group, "int-internal", n, "jack", 4))	/* Pin widget 0x1a */
	   {
		if (HDA_PINSELECT(0x1a, ctl, group, "mode", -1))
			HDA_CHOICES(ctl, "input");
	   }

	if (HDA_PIN_GROUP(0x1b, group, pin_group, "black", n, "jack", 4))	/* Pin widget 0x1b */
	   {
		/* Src 0x2=spdif */
		if (HDA_PINSELECT(0x1b, ctl, group, "mode", -1))
			HDA_CHOICES(ctl, "spdif-out input");
		HDA_OUTAMP(0x1b, group, "invol", 90);

		/* Widget 0x02 (spdif) */
		/* Src 0x1= */
		/* Src 0x8=rec1-sel */
		/* Src 0x9=rec2-sel */
	   }

	if (HDA_PIN_GROUP(0x1c, group, pin_group, "red", n, "jack", 4))	/* Pin widget 0x1c */
	   {
		/* Src 0x24=dock-mix */
		if (HDA_PINSELECT(0x1c, ctl, group, "mode", -1))
			HDA_CHOICES(ctl, "dock-mix-out input");
		HDA_OUTMUTE(0x1c, group, "inmute", UNMUTE);
	   }
  }
  /* Handle ADC widgets */
  {
	int n, group, rec_group;

	n=0;

	HDA_GROUP(rec_group, top_group, "record");

	if (HDA_ADC_GROUP(0x05, group, rec_group, "int-mic", n, "record", 4))	/* ADC widget 0x05 */
	   {
		/* Src 0x17=int-mic */
		HDA_INAMP(0x05, 0, group, "int-mic", 90);	/* From widget 0x17 */
	   }

	if (HDA_ADC_GROUP(0x06, group, rec_group, "int-mic", n, "record", 4))	/* ADC widget 0x06 */
	   {
		/* Src 0x18=int-mic */
		HDA_INAMP(0x06, 0, group, "int-mic", 90);	/* From widget 0x18 */
	   }

	if (HDA_ADC_GROUP(0x08, group, rec_group, "rec1-sel", n, "record", 4))	/* ADC widget 0x08 */
	   {
		/* Src 0xc=rec1-sel */
	   }

	if (HDA_ADC_GROUP(0x09, group, rec_group, "rec2-sel", n, "record", 4))	/* ADC widget 0x09 */
	   {
		/* Src 0xd=rec2-sel */
	   }
  }
  /* Handle misc widgets */
  {
	int n, group, misc_group;

	n=0;

	HDA_GROUP(misc_group, top_group, "misc");

	if (HDA_MISC_GROUP(0x03, group, misc_group, "headphone", n, "misc", 8))	/* Misc widget 0x03 */
	   {
		HDA_OUTAMP(0x03, group, "-", 90);
	   }

	if (HDA_MISC_GROUP(0x04, group, misc_group, "front", n, "misc", 8))	/* Misc widget 0x04 */
	   {
		HDA_OUTAMP(0x04, group, "-", 90);
	   }

	if (HDA_MISC_GROUP(0x07, group, misc_group, "headphone-mix", n, "misc", 8))	/* Misc widget 0x07 */
	   {
		/* Src 0x22=headphone-sel */
		/* Src 0x21=input-mix */
		{
			int amp_group;

			HDA_GROUP(amp_group, group, "mute");
			HDA_INMUTE(0x07, 0, amp_group, "headphone-sel", UNMUTE);	/* From widget 0x22 */
			HDA_INMUTE(0x07, 1, amp_group, "input-mix", UNMUTE);	/* From widget 0x21 */
		}

		/* Widget 0x22 (headphone-sel) */
		/* Src 0x3=headphone */
		/* Src 0x4=front */
		if (HDA_SELECT(0x22, "src", ctl, group, 1 /* Select front */))
		   {
			HDA_CHOICES(ctl, "headphone front");
		   }

		/* Widget 0x21 (input-mix) */
		/* Src 0x20=input-mix */
		HDA_OUTAMP(0x21, group, "-", 90);

		/* Widget 0x20 (input-mix) */
		/* Src 0x14=red */
		/* Src 0x96= */
		/* Src 0x1a=int-internal */
		/* Src 0x25=dock-mix */
		HDA_INAMP(0x20, 0, group, "red", 90);	/* From widget 0x14 */
		HDA_INAMP(0x20, 1, group, "", 90);	/* From widget 0x96 */
		HDA_INAMP(0x20, 2, group, "int-internal", 90);	/* From widget 0x1a */
		HDA_INAMP(0x20, 3, group, "dock-mix", 90);	/* From widget 0x25 */
	   }

	if (HDA_MISC_GROUP(0x0a, group, misc_group, "lineout-mix", n, "misc", 8))	/* Misc widget 0x0a */
	   {
		/* Src 0x4=front */
		/* Src 0x21=input-mix */
		{
			int amp_group;

			HDA_GROUP(amp_group, group, "mute");
			HDA_INMUTE(0x0a, 0, amp_group, "front", UNMUTE);	/* From widget 0x04 */
			HDA_INMUTE(0x0a, 1, amp_group, "input-mix", UNMUTE);	/* From widget 0x21 */
		}
	   }

	if (HDA_MISC_GROUP(0x0b, group, misc_group, "aux-mix", n, "misc", 8))	/* Misc widget 0x0b */
	   {
		/* Src 0xf=aux-sel */
		/* Src 0x21=input-mix */
		{
			int amp_group;

			HDA_GROUP(amp_group, group, "mute");
			HDA_INMUTE(0x0b, 0, amp_group, "aux-sel", UNMUTE);	/* From widget 0x0f */
			HDA_INMUTE(0x0b, 1, amp_group, "input-mix", UNMUTE);	/* From widget 0x21 */
		}

		/* Widget 0x0f (aux-sel) */
		/* Src 0x3=headphone */
		/* Src 0x4=front */
		if (HDA_SELECT(0x0f, "src", ctl, group, -1))
		   {
			HDA_CHOICES(ctl, "headphone front");
		   }
	   }

	if (HDA_MISC_GROUP(0x0c, group, misc_group, "rec1-sel", n, "misc", 8))	/* Misc widget 0x0c */
	   {
		/* Src 0x14=mic */
		/* Src 0x96= */
		/* Src 0x20=input-mix */
		/* Src 0x25=dock-mix */
		if (HDA_SELECT(0x0c, "src", ctl, group, -1))
		   {
			HDA_CHOICES(ctl, "mic unknown input-mix dock-mix");
		   }
		HDA_OUTAMP(0x0c, group, "-", 90);

		/* Widget 0x25 (dock-mix) */
		/* Src 0x1c=red */
		HDA_OUTAMP(0x25, group, "-", 90);
	   }

	if (HDA_MISC_GROUP(0x0d, group, misc_group, "rec2-sel", n, "misc", 8))	/* Misc widget 0x0d */
	   {
		/* Src 0x14=mic */
		/* Src 0x96= */
		/* Src 0x20=input-mix */
		/* Src 0x25=dock-mix */
		if (HDA_SELECT(0x0d, "src", ctl, group, -1))
		   {
			HDA_CHOICES(ctl, "mic unknown input-mix dock-mix");
		   }
		HDA_OUTAMP(0x0d, group, "-", 90);
	   }

	if (HDA_MISC_GROUP(0x10, group, misc_group, "beep", n, "misc", 8))	/* Misc widget 0x10 */
	   {
		HDA_OUTAMP(0x10, group, "-", 90);
	   }

	if (HDA_MISC_GROUP(0x1e, group, misc_group, "mono-mix", n, "misc", 8))	/* Misc widget 0x1e */
	   {
		/* Src 0xe=mono-sel */
		/* Src 0x21=input-mix */
		{
			int amp_group;

			HDA_GROUP(amp_group, group, "mute");
			HDA_INMUTE(0x1e, 0, amp_group, "mono-sel", UNMUTE);	/* From widget 0x0e */
			HDA_INMUTE(0x1e, 1, amp_group, "input-mix", UNMUTE);	/* From widget 0x21 */
		}
	   }

	if (HDA_MISC_GROUP(0x24, group, misc_group, "dock-mix", n, "misc", 8))	/* Misc widget 0x24 */
	   {
		/* Src 0x23=dock-sel */
		/* Src 0x21=input-mix */
		{
			int amp_group;

			HDA_GROUP(amp_group, group, "mute");
			HDA_INMUTE(0x24, 0, amp_group, "dock-sel", UNMUTE);	/* From widget 0x23 */
			HDA_INMUTE(0x24, 1, amp_group, "input-mix", UNMUTE);	/* From widget 0x21 */
		}
	   }
  }
  return 0;
}
