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
/* Codec index is 1 */
/* Codec vendor 10ec:0883 */
/* HD codec revision 1.0 (0.2) (0x00100002) */
/* Subsystem ID 10250000 */
/* Default amplifier caps: in=00000000, out=00000000 */
#include "oss_hdaudio_cfg.h"
#include "hdaudio.h"
#include "hdaudio_codec.h"
#include "hdaudio_dedicated.h"

int
hdaudio_ferrari5k_mixer_init (int dev, hdaudio_mixer_t * mixer, int cad, int top_group)
{
  int ctl=0;

  DDB(cmn_err(CE_CONT, "hdaudio_ferrari5k_mixer_init got called.\n"));

  // Main volume controls for PCM channels. Moved from the misc group
  HDA_OUTAMP_F(0x0c, top_group, "front", 90, MIXF_MAINVOL);
  HDA_OUTAMP_F(0x0d, top_group, "rear", 90, MIXF_MAINVOL);
  HDA_OUTAMP_F(0x0e, top_group, "center/lfe", 90, MIXF_MAINVOL);
  HDA_OUTAMP_F(0x0f, top_group, "side", 90, MIXF_MAINVOL);
  HDA_OUTAMP_F(0x26, top_group, "pcm4", 90, MIXF_MAINVOL);

  // Mute controls for the output pins
  HDA_OUTMUTE(0x15, top_group, "speaker-mute", UNMUTE);
  HDA_OUTMUTE(0x14, top_group, "headph-mute", UNMUTE);
  HDA_OUTMUTE(0x1a, top_group, "lineout-mute", UNMUTE);
  HDA_OUTMUTE(0x18, top_group, "mic-jack-mute", UNMUTE);

  /* Handle PIN widgets */
  {
	int n, group, pin_group;

	n=0;

	HDA_GROUP(pin_group, top_group, "jack");

	if (HDA_PIN_GROUP(0x15, group, pin_group, "int-speaker", n, "jack", 8))	/* Pin widget 0x15 */
	   {
		/* Src 0xc=front */
		/* Src 0xd=rear */
		/* Src 0xe=center/LFE */
		/* Src 0xf=side */
		/* Src 0x26=pcm4 */
		if (HDA_PINSELECT(0x15, ctl, group, "mode", -1))
			HDA_CHOICES(ctl, "front-out rear-out center/LFE-out side-out pcm4-out unused");
		//HDA_INAMP(0x15, 0, group, "inlevel", 90);	/* From widget 0x0c */
	   }

	if (HDA_PIN_GROUP(0x14, group, pin_group, "headphone", n, "jack", 8))	/* Pin widget 0x14 */
	   {
		/* Src 0xc=front */
		/* Src 0xd=rear */
		/* Src 0xe=center/LFE */
		/* Src 0xf=side */
		/* Src 0x26=pcm4 */
		if (HDA_PINSELECT(0x14, ctl, group, "mode", -1))
			HDA_CHOICES(ctl, "front-out rear-out center/LFE-out side-out pcm4-out input");
		HDA_INAMP(0x14, 0, group, "inlevel", 90);	/* From widget 0x0c */
	   }

	if (HDA_PIN_GROUP(0x18, group, pin_group, "ext-mic", n, "jack", 8))	/* Pin widget 0x18 */
	   {
		/* Src 0xc=front */
		/* Src 0xd=rear */
		/* Src 0xe=center/LFE */
		/* Src 0xf=side */
		/* Src 0x26=pcm4 */
		if (HDA_PINSELECT(0x18, ctl, group, "mode", -1))
			HDA_CHOICES(ctl, "front-out rear-out center/LFE-out side-out pcm4-out input");
		HDA_INAMP(0x18, 0, group, "inlevel", 90);	/* From widget 0x0c */
	   }

	if (HDA_PIN_GROUP(0x19, group, pin_group, "int-mic", n, "jack", 8))	/* Pin widget 0x19 */
	   {
		/* Src 0xc=front */
		/* Src 0xd=rear */
		/* Src 0xe=center/LFE */
		/* Src 0xf=side */
		/* Src 0x26=pcm4 */
#if 1
		HDA_SETSELECT(0x19, 5); // Hardwired to mic-input
#else
		if (HDA_PINSELECT(0x19, ctl, group, "mode", -1))
			HDA_CHOICES(ctl, "front-out rear-out center/LFE-out side-out pcm4-out input");
#endif
		//HDA_OUTMUTE(0x19, group, "outmute", UNMUTE);
		HDA_INAMP(0x19, 0, group, "inlevel", 90);	/* From widget 0x0c */
	   }

	if (HDA_PIN_GROUP(0x1a, group, pin_group, "line-out", n, "jack", 8))	/* Pin widget 0x1a */
	   {
		/* Src 0xc=front */
		/* Src 0xd=rear */
		/* Src 0xe=center/LFE */
		/* Src 0xf=side */
		/* Src 0x26=pcm4 */
		if (HDA_PINSELECT(0x1a, ctl, group, "mode", 0))
			HDA_CHOICES(ctl, "front-out rear-out center/LFE-out side-out pcm4-out input");
		HDA_INAMP(0x1a, 0, group, "inlevel", 90);	/* From widget 0x0c */
	   }
  }

  /* Handle ADC widgets */
  {
	int n, group, rec_group;

	n=0;

	HDA_GROUP(rec_group, top_group, "record");

	if (HDA_ADC_GROUP(0x08, group, rec_group, "rec1", n, "record", 4))	/* ADC widget 0x08 */
	   {
		/* Src 0x23=rec1 */
		HDA_INAMP_F(0x08, 0, group, "rec1", 80, MIXF_RECVOL);	/* From widget 0x23 */

		/* Widget 0x23 (rec1) */
		/* Src 0x18=ext-mic */
		/* Src 0x19=int-mic */
		/* Src 0x1a=line-out */
		/* Src 0x1b=black */
		/* Src 0x1c=black */
		/* Src 0x1d=black */
		/* Src 0x14=black */
		/* Src 0x15=int-speaker */
		/* Src 0x16=black */
		/* Src 0x17=black */
		/* Src 0xb=input-mix */
		{
#if 1
			if (HDA_INSRC_SELECT(0x23, group, ctl, "recsrc", 1))
			    HDA_CHOICES(ctl, "ext-mic int-mic line-out-jack unused unused unused unused unused unused unused input-mix");
#else
			HDA_GROUP(amp_group, group, "mute");
			HDA_INMUTE(0x23, 0, amp_group, "ext-mic", UNMUTE);	/* From widget 0x18 */
			HDA_INMUTE(0x23, 1, amp_group, "int-mic", UNMUTE);	/* From widget 0x19 */
			HDA_INMUTE(0x23, 2, amp_group, "line-out", UNMUTE);	/* From widget 0x1a */
			HDA_INMUTE(0x23, 3, amp_group, "black", UNMUTE);	/* From widget 0x1b */
			HDA_INMUTE(0x23, 4, amp_group, "black", UNMUTE);	/* From widget 0x1c */
			HDA_INMUTE(0x23, 5, amp_group, "black", UNMUTE);	/* From widget 0x1d */
			HDA_INMUTE(0x23, 6, amp_group, "headph-jack", UNMUTE);	/* From widget 0x14 */
			HDA_INMUTE(0x23, 7, amp_group, "int-speaker", UNMUTE);	/* From widget 0x15 */
			HDA_INMUTE(0x23, 8, amp_group, "black", UNMUTE);	/* From widget 0x16 */
			HDA_INMUTE(0x23, 9, amp_group, "black", UNMUTE);	/* From widget 0x17 */
			HDA_INMUTE(0x23, 10, amp_group, "input-mix", MUTE);	/* From widget 0x0b */
#endif
		}
	   }

	if (HDA_ADC_GROUP(0x09, group, rec_group, "rec2", n, "record", 4))	/* ADC widget 0x09 */
	   {
		/* Src 0x22=rec2 */
		HDA_INAMP_F(0x09, 0, group, "rec2", 80, MIXF_RECVOL);	/* From widget 0x22 */

		/* Widget 0x22 (rec2) */
		/* Src 0x18=ext-mic */
		/* Src 0x19=int-mic */
		/* Src 0x1a=line-out */
		/* Src 0x1b=black */
		/* Src 0x1c=black */
		/* Src 0x1d=black */
		/* Src 0x14=black */
		/* Src 0x15=int-speaker */
		/* Src 0x16=black */
		/* Src 0x17=black */
		/* Src 0xb=input-mix */
		{
#if 1
			if (HDA_INSRC_SELECT(0x22, group, ctl, "recsrc", 1))
			    HDA_CHOICES(ctl, "ext-mic int-mic line-out-jack unused unused unused unused unused unused unused input-mix");
#else
			HDA_GROUP(amp_group, group, "mute");
			HDA_INMUTE(0x22, 0, amp_group, "ext-mic", UNMUTE);	/* From widget 0x18 */
			HDA_INMUTE(0x22, 1, amp_group, "int-mic", UNMUTE);	/* From widget 0x19 */
			HDA_INMUTE(0x22, 2, amp_group, "line-out", UNMUTE);	/* From widget 0x1a */
			HDA_INMUTE(0x22, 3, amp_group, "black", UNMUTE);	/* From widget 0x1b */
			HDA_INMUTE(0x22, 4, amp_group, "black", UNMUTE);	/* From widget 0x1c */
			HDA_INMUTE(0x22, 5, amp_group, "black", UNMUTE);	/* From widget 0x1d */
			HDA_INMUTE(0x22, 6, amp_group, "headph-jack", UNMUTE);	/* From widget 0x14 */
			HDA_INMUTE(0x22, 7, amp_group, "int-speaker", UNMUTE);	/* From widget 0x15 */
			HDA_INMUTE(0x22, 8, amp_group, "black", UNMUTE);	/* From widget 0x16 */
			HDA_INMUTE(0x22, 9, amp_group, "black", UNMUTE);	/* From widget 0x17 */
			HDA_INMUTE(0x22, 10, amp_group, "input-mix", MUTE);	/* From widget 0x0b */
#endif
		}
	   }

	if (HDA_ADC_GROUP(0x0a, group, rec_group, "spdif-in", n, "record", 4))	/* ADC widget 0x0a */
	   {
		/* Src 0x1f=speaker */
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
		/* Src 0xb=input-mix */
		{
			int amp_group;

			HDA_GROUP(amp_group, group, "mute");
			HDA_INMUTE(0x0c, 0, amp_group, "front", UNMUTE);	/* From widget 0x02 */
			HDA_INMUTE(0x0c, 1, amp_group, "input-mix", MUTE);	/* From widget 0x0b */
		}
	   }

	if (HDA_MISC_GROUP(0x0d, group, misc_group, "rear", n, "misc", 8))	/* Misc widget 0x0d */
	   {
		/* Src 0x3=rear */
		/* Src 0xb=input-mix */
		{
			int amp_group;

			HDA_GROUP(amp_group, group, "mute");
			HDA_INMUTE(0x0d, 0, amp_group, "rear", UNMUTE);	/* From widget 0x03 */
			HDA_INMUTE(0x0d, 1, amp_group, "input-mix", MUTE);	/* From widget 0x0b */
		}
	   }

	if (HDA_MISC_GROUP(0x0e, group, misc_group, "center/LFE", n, "misc", 8))	/* Misc widget 0x0e */
	   {
		/* Src 0x4=center/LFE */
		/* Src 0xb=input-mix */
		{
			int amp_group;

			HDA_GROUP(amp_group, group, "mute");
			HDA_INMUTE(0x0e, 0, amp_group, "center/LFE", UNMUTE);	/* From widget 0x04 */
			HDA_INMUTE(0x0e, 1, amp_group, "input-mix", MUTE);	/* From widget 0x0b */
		}
	   }

	if (HDA_MISC_GROUP(0x0f, group, misc_group, "side", n, "misc", 8))	/* Misc widget 0x0f */
	   {
		/* Src 0x5=side */
		/* Src 0xb=input-mix */
		{
			int amp_group;

			HDA_GROUP(amp_group, group, "mute");
			HDA_INMUTE(0x0f, 0, amp_group, "side", UNMUTE);	/* From widget 0x05 */
			HDA_INMUTE(0x0f, 1, amp_group, "input-mix", MUTE);	/* From widget 0x0b */
		}
	   }

	if (HDA_MISC_GROUP(0x26, group, misc_group, "pcm4", n, "misc", 8))	/* Misc widget 0x26 */
	   {
		/* Src 0x25=pcm4 */
		/* Src 0xb=input-mix */
		{
			int amp_group;

			HDA_GROUP(amp_group, group, "mute");
			HDA_INMUTE(0x26, 0, amp_group, "pcm4", UNMUTE);	/* From widget 0x25 */
			HDA_INMUTE(0x26, 1, amp_group, "input-mix", MUTE);	/* From widget 0x0b */
		}
	   }

	if (HDA_MISC_GROUP(0x0b, group, misc_group, "input-mix", n, "misc", 8))	/* Misc widget 0x0b */
	   {
		/* Src 0x18=mic */
		/* Src 0x19=int-mic */
		/* Src 0x1a=linein */
		/* Src 0x1b=speaker */
		/* Src 0x1c=speaker */
		/* Src 0x1d=speaker */
		/* Src 0x14=headphone */
		/* Src 0x15=int-speaker */
		/* Src 0x16=speaker */
		/* Src 0x17=speaker */
		HDA_INAMP(0x0b, 0, group, "ext-mic", 10);	/* From widget 0x18 */
		HDA_INAMP(0x0b, 1, group, "int-mic", 10);	/* From widget 0x19 */
		HDA_INAMP(0x0b, 2, group, "line-out", 0);	/* From widget 0x1a */
		HDA_INAMP(0x0b, 3, group, "black", 0);	/* From widget 0x1b */
		HDA_INAMP(0x0b, 4, group, "black", 0);	/* From widget 0x1c */
		HDA_INAMP(0x0b, 5, group, "black", 0);	/* From widget 0x1d */
		//HDA_INAMP(0x0b, 6, group, "headph-jack", 90);	/* From widget 0x14 */
		//HDA_INAMP(0x0b, 7, group, "int-speaker", 0);	/* From widget 0x15 */
		HDA_INAMP(0x0b, 8, group, "black", 0);	/* From widget 0x16 */
		HDA_INAMP(0x0b, 9, group, "black", 0);	/* From widget 0x17 */
	   }
  }
  return 0;
}
