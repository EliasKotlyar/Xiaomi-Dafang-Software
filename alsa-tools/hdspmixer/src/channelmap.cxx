/*
 *   HDSPMixer
 *
 *   Copyright (C) 2011 Adrian Knoth (adi@drcomp.erfurt.thur.de)
 *                      Fredrik Lingvall (fredrik.lingvall@gmail.com)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 */

#include "channelmap.h"


/***
 *
 * hdsp cards
 *
 ***/

// Digiface


char dest_map_df_ss[14] = {
	0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26
};

char channel_map_df_ss[26] = {
	0, 1, 2, 3, 4, 5, 6, 7,		/* ADAT 1 */
	8, 9, 10, 11, 12, 13, 14, 15,	/* ADAT 2 */
	16, 17, 18, 19, 20, 21, 22, 23, /* ADAT 3 */
	24, 25				/* SPDIF */
};

// Multiface

char dest_map_mf_ss[10] = {
	0, 2, 4, 6, 16, 18, 20, 22, 24, 26
};

char channel_map_mf_ss[26] = {
	0, 1, 2, 3, 4, 5, 6, 7,		/* Line in */
	16, 17, 18, 19, 20, 21, 22, 23, /* ADAT */
	24, 25,				/* SPDIF */
	26, 27,             /* Phones L+R, only a destination channel */
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1
};

// Digiface/Multiface

char meter_map_ds[26] = {
	0, 1, 2, 3, 8, 9, 10, 11, /* analog 1-8 on Multiface, ADAT1+2 on Digiface*/
	16, 17, 18, 19, /* ADAT on Multiface, ADAT3 on Digiface */
	24, 25, /* SPDIF */
	26, 27, /* Headphones */
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1
};

char channel_map_ds[26] = {
	1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23,
	24, 25,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1
};

char dest_map_ds[8] = {
	0, 2, 8, 10, 16, 18, 24, 26
};

/* RPM */
char dest_map_rpm[3] = {
    0, 2, 4
};

char channel_map_rpm[26] = {
     0,  1,  2,  3,  4,  5, (char)-1, (char)-1,
    (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
    (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
    (char)-1, (char)-1
};

// HDSP 9652

char dest_map_h9652_ss[13] = {
	0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24
};

char dest_map_h9652_ds[7] = {
	0, 2, 8, 10, 16, 18, 24
};

// HDSP 9632

char dest_map_h9632_ss[8] = {
	0, 2, 4, 6, 8, 10, 12, 14
};

char dest_map_h9632_ds[6] = {
	0, 2, 8, 10, 12, 14
};

char dest_map_h9632_qs[4] = {
	8, 10, 12, 14
};

char channel_map_h9632_ss[16] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

char channel_map_h9632_ds[12] = {
	0, 1, 2, 3, 8, 9, 10, 11, 12, 13, 14, 15
};

char channel_map_h9632_qs[8] = {
	8, 9, 10, 11, 12, 13, 14, 15
};


/***
 *
 * hdspm cards
 *
 ***/

// HDSPe MADI and MADIface

char dest_map_unity[32] = {
	0,  2,  4,  6,  8, 10, 12, 14,
	16, 18, 20, 22, 24, 26, 28, 30,
	32, 34, 36, 38, 40, 42, 44, 46,
	48, 50, 52, 54, 56, 58, 60, 62
};

char channel_map_unity_ss[HDSPM_MAX_CHANNELS] = {
	0, 1, 2, 3, 4, 5, 6, 7,
	8, 9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23,
	24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39,
	40, 41, 42, 43, 44, 45, 46, 47,
	48, 49, 50, 51, 52, 53, 54, 55,
	56, 57, 58, 59, 60, 61, 62, 63
};

char channel_map_unity_ds[HDSPM_MAX_CHANNELS] = {
	0, 2, 4, 6, 8, 10, 12, 14,
	16, 18, 20, 22, 24, 26, 28, 30,
	32, 34, 36, 38, 40, 42, 44, 46,
	48, 50, 52, 54, 56, 58, 60, 62,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
};

char channel_map_unity_qs[HDSPM_MAX_CHANNELS] = {
	0, 4, 8, 12, 16, 20, 24, 28,
	32, 36, 40, 44, 48, 52, 56, 60,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
};

// HDSPe RayDAT

char dest_map_raydat_ss[18] = {
	4,  6,  8, 10,
	12, 14, 16, 18,
	20, 22, 24, 26,
	28, 30, 32, 34,
	0,  2
};

char dest_map_raydat_ds[10] = {
	4,  6,
	8, 10,
	12, 14,
	16, 18,
	0,  2
};

char dest_map_raydat_qs[6] = {
	4,
	6,
	8,
	10,
	0,  2
};

char channel_map_raydat_ss[HDSPM_MAX_CHANNELS] = {
	4, 5, 6, 7, 8, 9, 10, 11,	/* ADAT 1 */
	12, 13, 14, 15, 16, 17, 18, 19,	/* ADAT 2 */
	20, 21, 22, 23, 24, 25, 26, 27,	/* ADAT 3 */
	28, 29, 30, 31, 32, 33, 34, 35,	/* ADAT 4 */
	0, 1,			/* AES */
	2, 3,			/* SPDIF */
	(char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
};

char channel_map_raydat_ds[HDSPM_MAX_CHANNELS] = {
	4, 5, 6, 7,		/* ADAT 1 */
	8, 9, 10, 11,		/* ADAT 2 */
	12, 13, 14, 15,		/* ADAT 3 */
	16, 17, 18, 19,		/* ADAT 4 */
	0, 1,			/* AES */
	2, 3,			/* SPDIF */
	(char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
};

char channel_map_raydat_qs[HDSPM_MAX_CHANNELS] = {
	4, 5,			/* ADAT 1 */
	6, 7,			/* ADAT 2 */
	8, 9,			/* ADAT 3 */
	10, 11,			/* ADAT 4 */
	0, 1,			/* AES */
	2, 3,			/* SPDIF */
	(char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
};

// HDSPe AIO

char dest_map_aio_ss[10] = {
   0, // Analogue
   8, // AES
  10, // SPDIF
  12, 14, 16, 18, // ADAT
   6,  // Phones
   2,  // AEB 1+2
   4   // AEB 3+4
};


char dest_map_aio_ds[8] = {
   0, // Analogue
   8, // AES
  10, // SPDIF
  12, 16, // ADAT
   6,  // Phones
   2,  // AEB 1+2
   4   // AEB 3+4
};

char dest_map_aio_qs[7] = {
   0, // Analogue
   8, // AES
  10, // SPDIF
  12, // ADAT
   6, // Phone
   2, // AEB 1+2
   4  // AEB 3+4
};

char channel_map_aio_in_ss[HDSPM_MAX_CHANNELS] = {
	0, 1,			/* line in */
	8, 9,			/* aes in, */
	10, 11,			/* spdif in */
	12, 13, 14, 15, 16, 17, 18, 19,	/* ADAT in */
	2, 3, 4, 5,		/* AEB */
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
};

char channel_map_aio_out_ss[HDSPM_MAX_CHANNELS] = {
	0, 1,			/* line out */
	8, 9,			/* aes out */
	10, 11,			/* spdif out */
	12, 13, 14, 15, 16, 17, 18, 19,	/* ADAT out */
	6, 7,			/* phone out */
	2, 3, 4, 5,		/* AEB */
	(char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
};

char channel_map_aio_in_ds[HDSPM_MAX_CHANNELS] = {
	0, 1,			/* line in */
	8, 9,			/* aes in */
	10, 11,			/* spdif in */
	12, 14, 16, 18,		/* adat in */
	2, 3, 4, 5,		/* AEB */
	(char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1
};

char channel_map_aio_out_ds[HDSPM_MAX_CHANNELS] = {
	0, 1,			/* line out */
	8, 9,			/* aes out */
	10, 11,			/* spdif out */
	12, 14, 16, 18,		/* adat out */
	6, 7,			/* phone out */
	2, 3, 4, 5,		/* AEB */
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1
};

char channel_map_aio_in_qs[HDSPM_MAX_CHANNELS] = {
	0, 1,			/* line in */
	8, 9,			/* aes in */
	10, 11,			/* spdif in */
	12, 16,			/* adat in */
	2, 3, 4, 5,		/* AEB */
	(char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1
};

char channel_map_aio_out_qs[HDSPM_MAX_CHANNELS] = {
	0, 1,			/* line out */
	8, 9,			/* aes out */
	10, 11,			/* spdif out */
	12, 16,			/* adat out */
	6, 7,			/* phone out */
	2, 3, 4, 5,		/* AEB */
	(char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1
};

// HDSP AES32 and HDSPe AES

char dest_map_aes32[8] = {
  0,  2,  4,  6,  8, 10, 12, 14
};

char channel_map_aes32[HDSPM_MAX_CHANNELS] = {
	0, 1,			/* AES 1 */
	2, 3,			/* AES 2 */
	4, 5,			/* AES 3 */
	6, 7,			/* AES 4 */
	8, 9,			/* AES 5 */
	10, 11,			/* AES 6 */
	12, 13,			/* AES 7 */
	14, 15,			/* AES 8 */
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1,
	(char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1, (char)-1
};

