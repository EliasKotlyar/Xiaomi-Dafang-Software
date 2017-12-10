/*
 * Purpose: Declarations of some functions and structures for HD audio mixers
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
/*
 * Prototype definitions for dedicated HDAudio codec/mixer drivers.
 */

extern int hdaudio_generic_mixer_init (int dev, hdaudio_mixer_t * mixer,
				       int cad, int group);
