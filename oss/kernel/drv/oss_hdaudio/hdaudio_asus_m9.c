/*
 * Purpose: init handler for Asus m9.
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

/* Codec index is 0 */
/* Codec vendor 10ec:0260 */
/* HD codec revision 1.0 (4.0) (0x00100400) */
/* Subsystem ID 1025160d */
#include "oss_hdaudio_cfg.h"
#include "hdaudio.h"
#include "hdaudio_codec.h"
#include "hdaudio_dedicated.h"
#include "hdaudio_mixers.h"

int
hdaudio_asus_m9_mixer_init (int dev, hdaudio_mixer_t * mixer, int cad, int top_group)
{
	DDB(cmn_err(CE_CONT, "hdaudio_asus_m9_mixer_init got called.\n"));

	corb_write (mixer, cad, 0x1b, 0, SET_EAPD, 0);

	return hdaudio_generic_mixer_init(dev, mixer, cad, top_group);
}

