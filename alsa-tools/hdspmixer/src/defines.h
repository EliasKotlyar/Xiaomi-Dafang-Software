/*
 *   HDSPMixer
 *    
 *   Copyright (C) 2003 Thomas Charbonnel (thomas@undata.org)
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
 */

#ifndef DEFINES_H
#define DEFINES_H

/* Uncomment this to make the setup window non-modal */
//#define NON_MODAL_SETUP 1

#define HDSPeMADI 10
#define HDSPeRayDAT 11
#define HDSPeAIO 12
#define HDSP_AES 13 /* both HDSP AES32 and HDSPe AES */

#define HDSP_MAX_CHANNELS 64
#define HDSP_MAX_DEST	  32

#define STRIP_WIDTH 	  36
#define FULLSTRIP_HEIGHT  253
#define SMALLSTRIP_HEIGHT 208
#define MENU_HEIGHT       20

#define MIN_WIDTH  2*STRIP_WIDTH
#define MIN_HEIGHT MENU_HEIGHT

#define CF	     8
#define FADER_HEIGHT 137
#define METER_HEIGHT 139
#define PEAK_HEIGHT  4

#define PAN_WIDTH 28

#define MAX_CARDS	3

/* Number of presets. 8 presets visible to the user, the 9th is used for
 * holding temporary mixer data when switching cards, so it's not a real
 * preset but more like a scratch pad.
 */
#define NUM_PRESETS	9

typedef unsigned long long int int64;

#endif

