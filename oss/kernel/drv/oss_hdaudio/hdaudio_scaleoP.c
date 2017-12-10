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
/* Codec vendor 0804:7d10 */
/* HD codec revision 1.0 (0.1) (0x00100001) */
/* Subsystem ID 1043e601 */
/* Default amplifier caps: in=00000000, out=00000000 */
#include "oss_hdaudio_cfg.h"
#include "hdaudio.h"
#include "hdaudio_codec.h"
#include "hdaudio_dedicated.h"

int
hdaudio_scaleoP_mixer_init (int dev, hdaudio_mixer_t * mixer, int cad,
			    int top_group)
{
  int ctl = 0;

  DDB (cmn_err (CE_CONT, "hdaudio_scaleoP_mixer_init got called.\n"));

  HDA_OUTAMP_F (0x0c, top_group, "front", 90, MIXF_PCMVOL);
  HDA_OUTAMP_F (0x0d, top_group, "rear", 90, MIXF_PCMVOL);
  HDA_OUTAMP_F (0x0e, top_group, "center/LFE", 90, MIXF_PCMVOL);
  HDA_OUTAMP_F (0x0f, top_group, "side", 90, MIXF_PCMVOL);
  HDA_OUTAMP_F (0x26, top_group, "pcm4", 90, MIXF_PCMVOL);

  /*
   * Unmute all inputs for the above sliders.
   */
  HDA_SET_INMUTE (0x0c, 0, UNMUTE);	/* From widget 0x02 */
  HDA_SET_INMUTE (0x0c, 1, UNMUTE);	/* From widget 0x0b */
  HDA_SET_INMUTE (0x0d, 0, UNMUTE);	/* From widget 0x03 */
  HDA_SET_INMUTE (0x0d, 1, UNMUTE);	/* From widget 0x0b */
  HDA_SET_INMUTE (0x0e, 0, UNMUTE);	/* From widget 0x04 */
  HDA_SET_INMUTE (0x0e, 1, UNMUTE);	/* From widget 0x0b */
  HDA_SET_INMUTE (0x0f, 0, UNMUTE);	/* From widget 0x05 */
  HDA_SET_INMUTE (0x0f, 1, UNMUTE);	/* From widget 0x0b */
  HDA_SET_INMUTE (0x26, 0, UNMUTE);	/* From widget 0x25 */
  HDA_SET_INMUTE (0x26, 1, UNMUTE);	/* From widget 0x0b */

  /* Handle ADC widgets */
  {
    int n, group, rec_group;

    n = 0;

    HDA_GROUP (rec_group, top_group, "record");

    if (HDA_ADC_GROUP (0x08, group, rec_group, "rec1", n, "record", 2))	/* ADC widget 0x08 */
      {
	/* Src 0x23=mix */
	HDA_INAMP (0x08, 0, group, "-", 90);	/* From widget 0x23 */

	/* Widget 0x23 (mix) */
	/* Src 0x18=pink */
	/* Src 0x19=fp-pink */
	/* Src 0x1a=blue */
	/* Src 0x1b=fp-green */
	/* Src 0x1c=int-aux */
	/* Src 0x1d=black */
	/* Src 0x14=green */
	/* Src 0x15=black */
	/* Src 0x16=orange */
	/* Src 0x17=grey */
	/* Src 0x0b=input-mix */
	if (HDA_INSRC_SELECT (0x23, group, ctl, "recsrc", 10))
	  HDA_CHOICES (ctl,
		       "pink fp-pink blue fp-green int-aux black green black orange grey input-mix");
      }

    if (HDA_ADC_GROUP (0x09, group, rec_group, "rec2", n, "record", 2))	/* ADC widget 0x09 */
      {
	/* Src 0x22=mix */
	HDA_INAMP (0x09, 0, group, "-", 90);	/* From widget 0x22 */

	/* Widget 0x22 (mix) */
	/* Src 0x18=pink */
	/* Src 0x19=fp-pink */
	/* Src 0x1a=blue */
	/* Src 0x1b=fp-green */
	/* Src 0x1c=int-aux */
	/* Src 0x1d=black */
	/* Src 0x14=green */
	/* Src 0x15=black */
	/* Src 0x16=orange */
	/* Src 0x17=grey */
	/* Src 0xb=input-mix */
	if (HDA_INSRC_SELECT (0x23, group, ctl, "recsrc", 10))
	  HDA_CHOICES (ctl,
		       "pink fp-pink blue fp-green int-aux black green black orange grey input-mix");
      }

#if 0
    if (HDA_ADC_GROUP (0x0a, group, rec_group, "spdif-in", n, "record", 4))	/* ADC widget 0x0a */
      {
	/* Src 0x1f=speaker */
      }
#endif
  }
  /* Handle misc widgets */
  {
    int n, group;

    n = 0;

    HDA_GROUP (group, top_group, "input-mix");

    /* Src 0x18=mic */
    /* Src 0x19=fp-mic */
    /* Src 0x1a=linein */
    /* Src 0x1b=fp-headphone */
    /* Src 0x1c=int-aux */
    /* Src 0x1d=speaker */
    /* Src 0x14=lineout */
    /* Src 0x15=lineout */
    /* Src 0x16=lineout */
    /* Src 0x17=lineout */
    HDA_INAMP (0x0b, 0, group, "pink", 30);	/* From widget 0x18 */
    HDA_INAMP (0x0b, 2, group, "blue", 90);	/* From widget 0x1a */
    HDA_INAMP (0x0b, 4, group, "int-aux", 90);	/* From widget 0x1c */
    HDA_INAMP (0x0b, 5, group, "black", 90);	/* From widget 0x1d */
    HDA_INAMP (0x0b, 6, group, "green", 90);	/* From widget 0x14 */
    HDA_INAMP (0x0b, 7, group, "black", 90);	/* From widget 0x15 */
    HDA_INAMP (0x0b, 8, group, "orange", 90);	/* From widget 0x16 */
    HDA_INAMP (0x0b, 9, group, "grey", 90);	/* From widget 0x17 */
    HDA_INAMP (0x0b, 3, group, "fp-green", 90);	/* From widget 0x1b */
    HDA_INAMP (0x0b, 1, group, "fp-pink", 30);	/* From widget 0x19 */
  }
  /* Handle PIN widgets */
  {
    int n, group, pin_group;

    n = 0;

    HDA_GROUP (pin_group, top_group, "jack");

    if (HDA_PIN_GROUP (0x14, group, pin_group, "green", n, "jack", 4))	/* Pin widget 0x14 */
      {
	/* Src 0xc=front */
	/* Src 0xd=rear */
	/* Src 0xe=center/LFE */
	/* Src 0xf=side */
	/* Src 0x26=pcm4 */
	if (HDA_PINSELECT (0x14, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl,
		       "front-out rear-out center/LFE-out side-out pcm4-out input");
	HDA_OUTMUTE (0x14, group, "inmute", UNMUTE);
	HDA_INAMP (0x14, 0, group, "in", 90);	/* From widget 0x0c */
      }

    if (HDA_PIN_GROUP (0x15, group, pin_group, "black", n, "jack", 4))	/* Pin widget 0x15 */
      {
	/* Src 0xc=front */
	/* Src 0xd=rear */
	/* Src 0xe=center/LFE */
	/* Src 0xf=side */
	/* Src 0x26=pcm4 */
	if (HDA_PINSELECT (0x15, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl,
		       "front-out rear-out center/LFE-out side-out pcm4-out input");
	HDA_OUTMUTE (0x15, group, "inmute", UNMUTE);
	HDA_INAMP (0x15, 0, group, "in", 90);	/* From widget 0x0c */
      }

    if (HDA_PIN_GROUP (0x16, group, pin_group, "orange", n, "jack", 4))	/* Pin widget 0x16 */
      {
	/* Src 0xc=front */
	/* Src 0xd=rear */
	/* Src 0xe=center/LFE */
	/* Src 0xf=side */
	/* Src 0x26=pcm4 */
	if (HDA_PINSELECT (0x16, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl,
		       "front-out rear-out center/LFE-out side-out pcm4-out input");
	HDA_OUTMUTE (0x16, group, "inmute", UNMUTE);
	HDA_INAMP (0x16, 0, group, "in", 90);	/* From widget 0x0c */
      }

    if (HDA_PIN_GROUP (0x17, group, pin_group, "grey", n, "jack", 4))	/* Pin widget 0x17 */
      {
	/* Src 0xc=front */
	/* Src 0xd=rear */
	/* Src 0xe=center/LFE */
	/* Src 0xf=side */
	/* Src 0x26=pcm4 */
	if (HDA_PINSELECT (0x17, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl,
		       "front-out rear-out center/LFE-out side-out pcm4-out input");
	HDA_OUTMUTE (0x17, group, "inmute", UNMUTE);
	HDA_INAMP (0x17, 0, group, "in", 90);	/* From widget 0x0c */
      }

    if (HDA_PIN_GROUP (0x18, group, pin_group, "pink", n, "jack", 4))	/* Pin widget 0x18 */
      {
	/* Src 0xc=front */
	/* Src 0xd=rear */
	/* Src 0xe=center/LFE */
	/* Src 0xf=side */
	/* Src 0x26=pcm4 */
	if (HDA_PINSELECT (0x18, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl,
		       "front-out rear-out center/LFE-out side-out pcm4-out input");
	HDA_OUTMUTE (0x18, group, "inmute", UNMUTE);
	HDA_INAMP (0x18, 0, group, "in", 90);	/* From widget 0x0c */
      }

    if (HDA_PIN_GROUP (0x1a, group, pin_group, "blue", n, "jack", 4))	/* Pin widget 0x1a */
      {
	/* Src 0xc=front */
	/* Src 0xd=rear */
	/* Src 0xe=center/LFE */
	/* Src 0xf=side */
	/* Src 0x26=pcm4 */
	if (HDA_PINSELECT (0x1a, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl,
		       "front-out rear-out center/LFE-out side-out pcm4-out input");
	HDA_OUTMUTE (0x1a, group, "inmute", UNMUTE);
	HDA_INAMP (0x1a, 0, group, "in", 90);	/* From widget 0x0c */
      }

    if (HDA_PIN_GROUP (0x1b, group, pin_group, "fp-green", n, "jack", 4))	/* Pin widget 0x1b */
      {
	/* Src 0xc=front */
	/* Src 0xd=rear */
	/* Src 0xe=center/LFE */
	/* Src 0xf=side */
	/* Src 0x26=pcm4 */
	if (HDA_PINSELECT (0x1b, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl,
		       "front-out rear-out center/LFE-out side-out pcm4-out input");
	HDA_OUTMUTE (0x1b, group, "inmute", UNMUTE);
	HDA_INAMP (0x1b, 0, group, "in", 90);	/* From widget 0x0c */
      }

    if (HDA_PIN_GROUP (0x19, group, pin_group, "fp-pink", n, "jack", 4))	/* Pin widget 0x19 */
      {
	/* Src 0xc=front */
	/* Src 0xd=rear */
	/* Src 0xe=center/LFE */
	/* Src 0xf=side */
	/* Src 0x26=pcm4 */
	if (HDA_PINSELECT (0x19, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl,
		       "front-out rear-out center/LFE-out side-out pcm4-out input");
	HDA_OUTMUTE (0x19, group, "inmute", UNMUTE);
	HDA_INAMP (0x19, 0, group, "in", 90);	/* From widget 0x0c */
      }

#if 0
    /*
     * Non-used pins
     */
    if (HDA_PIN_GROUP (0x1c, group, pin_group, "int-aux", n, "jack", 4))	/* Pin widget 0x1c */
      {
	if (HDA_PINSELECT (0x1c, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl, "input");
      }

    if (HDA_PIN_GROUP (0x1d, group, pin_group, "black", n, "jack", 4))	/* Pin widget 0x1d */
      {
	if (HDA_PINSELECT (0x1d, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl, "input");
      }
#endif
  }

  return 0;
}
