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
 * Purpose: Dedicated HDaudio mixer driver for Asus Eee PC
 */

/* Codec index is 0 */
/* Codec vendor 10ec:0662 */
/* HD codec revision 1.0 (1.1) (0x00100101) */
/* Subsystem ID 10438337 */
/* Default amplifier caps: in=00000000, out=00000000 */
#include "oss_hdaudio_cfg.h"
#include "hdaudio.h"
#include "hdaudio_codec.h"
#include "hdaudio_dedicated.h"

int
hdaudio_eeepc_mixer_init (int dev, hdaudio_mixer_t * mixer, int cad, int top_group)
{
  int ctl=0;

  DDB(cmn_err(CE_CONT, "hdaudio_eeepc_mixer_init got called.\n"));

  HDA_OUTAMP(0x02, top_group, "pcm-front", 100);
  HDA_OUTAMP(0x04, top_group, "pcm-rear", 100);

  /* Handle PIN widgets */
  {
	int n, group, pin_group;

	n=0;

	HDA_GROUP(pin_group, top_group, "jack");

	if (HDA_PIN_GROUP(0x14, group, pin_group, "speaker", n, "jack", 4))	/* Pin widget 0x14 */
	   {
		/* Src 0xc=front */
#if 1
		HDA_SET_PINSELECT(0x14, 0); // Hardwire as output
#else
		if (HDA_PINSELECT(0x14, ctl, group, "mode", -1))
			HDA_CHOICES(ctl, "front-out input");
#endif
		HDA_OUTMUTE(0x14, group, "mute", UNMUTE);
	   }

#if 0
	if (HDA_PIN_GROUP(0x15, group, pin_group, "black", n, "jack", 4))	/* Pin widget 0x15 */
	   {
		/* Src 0xd=rear */
		if (HDA_PINSELECT(0x15, ctl, group, "mode", -1))
			HDA_CHOICES(ctl, "rear-out input");
		HDA_OUTMUTE(0x15, group, "mute", UNMUTE);
	   }

	if (HDA_PIN_GROUP(0x16, group, pin_group, "black", n, "jack", 4))	/* Pin widget 0x16 */
	   {
		/* Src 0xe=rear */
		if (HDA_PINSELECT(0x16, ctl, group, "mode", -1))
			HDA_CHOICES(ctl, "rear-out input");
		HDA_OUTMUTE(0x16, group, "mute", UNMUTE);
	   }
#endif

	if (HDA_PIN_GROUP(0x19, group, pin_group, "mic", n, "jack", 4))	/* Pin widget 0x19 */
	   {
		/* Src 0xc=front */
		/* Src 0xe=rear */
#if 0
		if (HDA_PINSELECT(0x19, ctl, group, "mode", -1))
			HDA_CHOICES(ctl, "front-out rear-out input");
		// HDA_OUTMUTE(0x19, group, "inmute", UNMUTE);
#else
		HDA_SET_PINSELECT(0x19, 2); // Hardwire to input
#endif 
		HDA_INAMP(0x19, 0, group, "in", 90);	/* From widget 0x0c */
	   }

#if 0
	if (HDA_PIN_GROUP(0x1a, group, pin_group, "black", n, "jack", 4))	/* Pin widget 0x1a */
	   {
		/* Src 0xd=rear */
		if (HDA_PINSELECT(0x1a, ctl, group, "mode", -1))
			HDA_CHOICES(ctl, "rear-out input");
		HDA_OUTMUTE(0x1a, group, "inmute", UNMUTE);
	   }
#endif

	if (HDA_PIN_GROUP(0x1b, group, pin_group, "green", n, "jack", 4))	/* Pin widget 0x1b */
	   {
		/* Src 0xc=front */
		/* Src 0xe=rear */
		if (HDA_PINSELECT(0x1b, ctl, group, "mode", -1))
			HDA_CHOICES(ctl, "front-out rear-out input");
		HDA_OUTMUTE(0x1b, group, "inmute", UNMUTE);
		HDA_INAMP(0x1b, 0, group, "out", 90);	/* From widget 0x0c */
	   }

	if (HDA_PIN_GROUP(0x18, group, pin_group, "pink", n, "jack", 4))	/* Pin widget 0x18 */
	   {
		/* Src 0xe=rear */
		if (HDA_PINSELECT(0x18, ctl, group, "mode", 1))
			HDA_CHOICES(ctl, "rear-out input");
		HDA_OUTMUTE(0x18, group, "mute", UNMUTE);
		HDA_INAMP(0x18, 0, group, "in", 90);	/* From widget 0x0e */
	   }

#if 0
	if (HDA_PIN_GROUP(0x1c, group, pin_group, "black", n, "jack", 4))	/* Pin widget 0x1c */
	   {
		if (HDA_PINSELECT(0x1c, ctl, group, "mode", -1))
			HDA_CHOICES(ctl, "input");
	   }

	if (HDA_PIN_GROUP(0x1d, group, pin_group, "purple", n, "jack", 4))	/* Pin widget 0x1d */
	   {
		if (HDA_PINSELECT(0x1d, ctl, group, "mode", -1))
			HDA_CHOICES(ctl, "input");
	   }
#endif
  }
  /* Handle ADC widgets */
  {
	int n, group, rec_group;

	n=0;

	HDA_GROUP(rec_group, top_group, "record");

	if (HDA_ADC_GROUP(0x08, group, rec_group, "pcmin0", n, "record", 4))	/* ADC widget 0x08 */
	   {
		/* Src 0x23=mix */
		HDA_INAMP(0x08, 0, group, "-", 90);	/* From widget 0x23 */

		/* Widget 0x23 (mix) */
		/* Src 0x18=pink */
		/* Src 0x19=mic */
		/* Src 0x1a=black */
		/* Src 0x1b=green */
		/* Src 0x1c=black */
		/* Src 0x1d=purple */
		/* Src 0x14=speaker */
		/* Src 0x15=black */
		/* Src 0x16=black */
		/* Src 0xb=mix */
		{
			int amp_group;

			HDA_GROUP(amp_group, group, "rec-mute");
			HDA_INMUTE(0x23, 0, amp_group, "pink", MUTE);	/* From widget 0x18 */
			HDA_INMUTE(0x23, 1, amp_group, "mic", UNMUTE);	/* From widget 0x19 */
			HDA_INMUTE(0x23, 2, amp_group, "black", MUTE);	/* From widget 0x1a */
			HDA_INMUTE(0x23, 3, amp_group, "green", MUTE);	/* From widget 0x1b */
			HDA_INMUTE(0x23, 4, amp_group, "black", MUTE);	/* From widget 0x1c */
			HDA_INMUTE(0x23, 5, amp_group, "purple", MUTE);	/* From widget 0x1d */
			HDA_INMUTE(0x23, 6, amp_group, "speaker", MUTE);	/* From widget 0x14 */
			HDA_INMUTE(0x23, 7, amp_group, "black", MUTE);	/* From widget 0x15 */
			HDA_INMUTE(0x23, 8, amp_group, "black", MUTE);	/* From widget 0x16 */
			HDA_INMUTE(0x23, 9, amp_group, "input-mixer", MUTE);	/* From widget 0x0b */
		}
	   }

	if (HDA_ADC_GROUP(0x09, group, rec_group, "pcmin1", n, "record", 4))	/* ADC widget 0x09 */
	   {
		/* Src 0x22=mix */
		HDA_INAMP(0x09, 0, group, "-", 90);	/* From widget 0x22 */

		/* Widget 0x22 (mix) */
		/* Src 0x18=pink */
		/* Src 0x19=mic */
		/* Src 0x1a=black */
		/* Src 0x1b=green */
		/* Src 0x1c=black */
		/* Src 0x1d=purple */
		/* Src 0x14=speaker */
		/* Src 0x15=black */
		/* Src 0x16=black */
		/* Src 0xb=mix */
		{
			int amp_group;

			HDA_GROUP(amp_group, group, "rec-mute");
			HDA_INMUTE(0x22, 0, amp_group, "pink", MUTE);	/* From widget 0x18 */
			HDA_INMUTE(0x22, 1, amp_group, "mic", MUTE);	/* From widget 0x19 */
			HDA_INMUTE(0x22, 2, amp_group, "black", MUTE);	/* From widget 0x1a */
			HDA_INMUTE(0x22, 3, amp_group, "green", MUTE);	/* From widget 0x1b */
			HDA_INMUTE(0x22, 4, amp_group, "black", MUTE);	/* From widget 0x1c */
			HDA_INMUTE(0x22, 5, amp_group, "purple", MUTE);	/* From widget 0x1d */
			HDA_INMUTE(0x22, 6, amp_group, "speaker", MUTE);	/* From widget 0x14 */
			HDA_INMUTE(0x22, 7, amp_group, "black", MUTE);	/* From widget 0x15 */
			HDA_INMUTE(0x22, 8, amp_group, "black", MUTE);	/* From widget 0x16 */
			HDA_INMUTE(0x22, 9, amp_group, "input-mixer", UNMUTE);	/* From widget 0x0b */
		}
	   }
  }
  /* Handle misc widgets */
  {
	int n, group, misc_group;

	n=0;

	HDA_GROUP(misc_group, top_group, "misc");

	if (HDA_MISC_GROUP(0x0c, group, misc_group, "front", n, "misc", 8))	/* Misc widget 0x0c */
	   {
		/* Src 0x2=front */
		/* Src 0xb=mix */
		{
			int amp_group;

			HDA_GROUP(amp_group, group, "mute");
			HDA_INMUTE(0x0c, 0, amp_group, "pcm-front", UNMUTE);	/* From widget 0x02 */
			HDA_INMUTE(0x0c, 1, amp_group, "input-mixer", UNMUTE);	/* From widget 0x0b */
		}
	   }

	if (HDA_MISC_GROUP(0x0d, group, misc_group, "rear", n, "misc", 8))	/* Misc widget 0x0d */
	   {
		/* Src 0x3=rear */
		/* Src 0xb=mix */
		{
			int amp_group;

			HDA_GROUP(amp_group, group, "mute");
			HDA_INMUTE(0x0d, 0, amp_group, "pcm-rear", UNMUTE);	/* From widget 0x03 */
			HDA_INMUTE(0x0d, 1, amp_group, "input-mixer", UNMUTE);	/* From widget 0x0b */
		}
	   }

	if (HDA_MISC_GROUP(0x0e, group, misc_group, "rear", n, "misc", 8))	/* Misc widget 0x0e */
	   {
		/* Src 0x4=rear */
		/* Src 0xb=mix */
		{
			int amp_group;

			HDA_GROUP(amp_group, group, "mute");
			HDA_INMUTE(0x0e, 0, amp_group, "rear", UNMUTE);	/* From widget 0x04 */
			HDA_INMUTE(0x0e, 1, amp_group, "input-mixer", UNMUTE);	/* From widget 0x0b */
		}
	   }

#if 0
	if (HDA_MISC_GROUP(0x02, group, misc_group, "pcm-front", n, "misc", 8))	/* Misc widget 0x02 */
	   {
		HDA_OUTAMP(0x02, group, "-", 90);
	   }

	if (HDA_MISC_GROUP(0x03, group, misc_group, "pcm", n, "misc", 8))	/* Misc widget 0x03 */
	   {
		HDA_OUTAMP(0x03, group, "-", 0);
	   }

	if (HDA_MISC_GROUP(0x04, group, misc_group, "pcm-rear", n, "misc", 8))	/* Misc widget 0x04 */
	   {
		HDA_OUTAMP(0x04, group, "-", 90);
	   }
#endif

	if (HDA_MISC_GROUP(0x0b, group, misc_group, "input-mixer", n, "misc", 8))	/* Misc widget 0x0b */
	   {
		/* Src 0x18=rear */
		/* Src 0x19=mic */
		/* Src 0x1a=rear */
		/* Src 0x1b=headphone */
		/* Src 0x1c=speaker */
		/* Src 0x1d=speaker */
		/* Src 0x14=front */
		/* Src 0x15=rear */
		/* Src 0x16=rear */
		HDA_INAMP(0x0b, 0, group, "pink", 20);	/* From widget 0x18 */
		HDA_INAMP(0x0b, 1, group, "mic", 20);	/* From widget 0x19 */
		HDA_INAMP(0x0b, 2, group, "black", 90);	/* From widget 0x1a */
		HDA_INAMP(0x0b, 3, group, "green", 90);	/* From widget 0x1b */
		HDA_INAMP(0x0b, 4, group, "black", 90);	/* From widget 0x1c */
		HDA_INAMP(0x0b, 5, group, "purple", 90);	/* From widget 0x1d */
		// HDA_INAMP(0x0b, 6, group, "speaker", 90);	/* From widget 0x14 */
		HDA_INAMP(0x0b, 7, group, "black", 90);	/* From widget 0x15 */
		HDA_INAMP(0x0b, 8, group, "black", 90);	/* From widget 0x16 */
	   }
  }
  return 0;
}
