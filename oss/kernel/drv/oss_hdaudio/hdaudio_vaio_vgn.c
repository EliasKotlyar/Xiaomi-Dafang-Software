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
/* Codec vendor 0804:73dc */
/* HD codec revision 1.0 (2.1) (0x00100201) */
/* Subsystem ID 104d2200 */
/* Default amplifier caps: in=80050f00, out=80027f7f */
#include "oss_hdaudio_cfg.h"
#include "hdaudio.h"
#include "hdaudio_codec.h"
#include "hdaudio_dedicated.h"

int
hdaudio_vaio_vgn_mixer_init (int dev, hdaudio_mixer_t * mixer, int cad,
			     int top_group)
{
  int ctl = 0;

  DDB (cmn_err (CE_CONT, "hdaudio_vaio_vgn_mixer_init got called.\n"));

  HDA_OUTAMP_F (0x05, top_group, "speaker", 90, MIXF_MAINVOL);
  /* We sync the volume of the headphone DAC to the speaker DAC */
#if 1
  HDA_OUTAMP_F (0x02, top_group, "headphone", 90, MIXF_MAINVOL);
#endif


  HDA_SETSELECT (0x0f, 0);	/* Int speaker mode */
  HDA_SETSELECT (0x14, 1);	/* Int mic mode */

  /* Handle PIN widgets */
  {
    int n, group, pin_group;

    n = 0;

    HDA_GROUP (pin_group, top_group, "jack");

    if (HDA_PIN_GROUP (0x0a, group, pin_group, "headphone", n, "jack", 4))	/* Pin widget 0x0a */
      {
	/* Src 0x2=pcm */
	if (HDA_PINSELECT (0x0a, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl, "pcm-out input");
	HDA_OUTMUTE (0x0a, group, "mute", UNMUTE);
      }

    if (HDA_PIN_GROUP (0x0b, group, pin_group, "black", n, "jack", 4))	/* Pin widget 0x0b */
      {
	/* Src 0x4=pcm */
	if (HDA_PINSELECT (0x0b, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl, "pcm-out input");
	HDA_OUTMUTE (0x0b, group, "mute", UNMUTE);

	/* Widget 0x04 (pcm) */
	HDA_OUTAMP (0x04, group, "-", 90);
      }

    if (HDA_PIN_GROUP (0x0c, group, pin_group, "black", n, "jack", 4))	/* Pin widget 0x0c */
      {
	/* Src 0x3=pcm */
	if (HDA_PINSELECT (0x0c, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl, "pcm-out input");
	HDA_OUTMUTE (0x0c, group, "mute", UNMUTE);

	/* Widget 0x03 (pcm) */
	HDA_OUTAMP (0x03, group, "-", 90);
      }

    if (HDA_PIN_GROUP (0x0d, group, pin_group, "red", n, "jack", 4))	/* Pin widget 0x0d */
      {
	/* Src 0x2=pcm */
	if (HDA_PINSELECT (0x0d, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl, "pcm-out input");
	HDA_OUTMUTE (0x0d, group, "mute", UNMUTE);
      }

    if (HDA_PIN_GROUP (0x0e, group, pin_group, "black", n, "jack", 4))	/* Pin widget 0x0e */
      {
	if (HDA_PINSELECT (0x0e, ctl, group, "mode", -1))
	  HDA_CHOICES (ctl, "input");
      }
  }
  /* Handle ADC widgets */
  {
    int n, group, rec_group;

    n = 0;

    HDA_GROUP (rec_group, top_group, "record");

    if (HDA_ADC_GROUP (0x06, group, rec_group, "rec1", n, "record", 4))	/* ADC widget 0x06 */
      {
	/* Src 0x7=rec */

	/* Widget 0x07 (rec) */
	/* Src 0xe=black */
	HDA_INAMP_F (0x07, 0, group, "black", 80, MIXF_RECVOL);	/* From widget 0x0e */
      }

    if (HDA_ADC_GROUP (0x08, group, rec_group, "rec", n, "record", 8))	/* ADC widget 0x08 */
      {
	/* Src 0x9=rec */

	/* Widget 0x09 (rec) */
	/* Src 0x15=rec */
	HDA_INAMP_F (0x09, 0, group, "rec", 80, MIXF_RECVOL);	/* From widget 0x15 */

	/* Widget 0x15 (rec) */
	/* Src 0xa=black */
	/* Src 0xd=red */
	/* Src 0x14=int-mic */
	/* Src 0x2=pcm */
	if (HDA_SELECT (0x15, "src", ctl, group, -1))
	  {
	    HDA_CHOICES (ctl, "headphone mic int-mic pcm");
	  }
	HDA_OUTAMP (0x15, group, "micboost", 0);
      }

    if (HDA_ADC_GROUP (0x12, group, rec_group, "spdifin", n, "record", 4))	/* ADC widget 0x12 */
      {
	/* Src 0x13=speaker */
      }
  }
  /* Handle misc widgets */
  {
#if 0
    if (HDA_MISC_GROUP (0x16, group, misc_group, "beep", n, "misc", 8))	/* Misc widget 0x16 */
      {
	HDA_OUTAMP (0x16, group, "-", 90);
      }
#endif
  }
  return 0;
}
