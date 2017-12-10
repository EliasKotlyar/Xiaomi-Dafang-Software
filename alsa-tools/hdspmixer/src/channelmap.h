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


#ifndef channelmap_H
#define channelmap_H

#include <alsa/sound/hdsp.h>
#include <alsa/sound/hdspm.h>

/***
 *
 * hdsp cards
 *
 ***/

// Digiface

extern char dest_map_df_ss[14];

extern char channel_map_df_ss[26];

// Multiface

extern char dest_map_mf_ss[10];

extern char channel_map_mf_ss[26];

// Digiface/Multiface

extern char meter_map_ds[26];

extern char channel_map_ds[26];

extern char dest_map_ds[8];

// RPM

extern char dest_map_rpm[3];
extern char channel_map_rpm[26];

// HDSP 9652

extern char dest_map_h9652_ss[13];

extern char dest_map_h9652_ds[7];

// HDSP 9632

extern char dest_map_h9632_ss[8];

extern char dest_map_h9632_ds[6];

extern char dest_map_h9632_qs[4];

extern char channel_map_h9632_ss[16];

extern char channel_map_h9632_ds[12];

extern char channel_map_h9632_qs[8];


/***
 *
 * hdspm cards
 *
 ***/

// HDSPe MADI and MADIface

extern char dest_map_unity[32];

extern char channel_map_unity_ss[HDSPM_MAX_CHANNELS];

extern char channel_map_unity_ds[HDSPM_MAX_CHANNELS];

extern char channel_map_unity_qs[HDSPM_MAX_CHANNELS];

// HDSPe RayDAT

extern char dest_map_raydat_ss[18];

extern char dest_map_raydat_ds[10];

extern char dest_map_raydat_qs[6];

extern char channel_map_raydat_ss[HDSPM_MAX_CHANNELS];

extern char channel_map_raydat_ds[HDSPM_MAX_CHANNELS];

extern char channel_map_raydat_qs[HDSPM_MAX_CHANNELS];

// HDSPe AIO

extern char dest_map_aio_ss[10];


extern char dest_map_aio_ds[8];

extern char dest_map_aio_qs[7];

extern char channel_map_aio_in_ss[HDSPM_MAX_CHANNELS];

extern char channel_map_aio_out_ss[HDSPM_MAX_CHANNELS];

extern char channel_map_aio_in_ds[HDSPM_MAX_CHANNELS];

extern char channel_map_aio_out_ds[HDSPM_MAX_CHANNELS];

extern char channel_map_aio_in_qs[HDSPM_MAX_CHANNELS];

extern char channel_map_aio_out_qs[HDSPM_MAX_CHANNELS];

// HDSP AES32 and HDSPe AES

extern char dest_map_aes32[8];

extern char channel_map_aes32[HDSPM_MAX_CHANNELS];

#endif /* channelmap_H */
