/*
 *   HDSPConf
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

#pragma implementation
#include "HC_CardPane.h"

extern const char *card_names[5];

HC_CardPane::HC_CardPane(int alsa_idx, int idx, HDSP_IO_Type t):Fl_Group(PANE_X, PANE_Y, PANE_W, PANE_H)
{
    alsa_index = alsa_idx;
    index = idx;
    type = t;
    snprintf(name, 19, "Card %d (%s)", index+1, card_names[t]);
    label(name);
    labelsize(10);

    if (type == Multiface) {
	clock_source = new HC_ClockSource(x()+9, y()+20, 148, V_STEP*7);
	sync_check = new HC_SyncCheck(x()+9, y()+40+V_STEP*7, 148, V_STEP*4);

	spdif_in = new HC_SpdifIn(x()+166, y()+20, 148, V_STEP*3);
	spdif_out = new HC_SpdifOut(x()+166, y()+40+V_STEP*3, 148, V_STEP*4);
	spdif_freq = new HC_SpdifFreq(x()+166, y()+60+V_STEP*7, 148, V_STEP);

	sync_ref = new HC_PrefSyncRef(x()+323, y()+20, 148, V_STEP*4);
	autosync_ref = new HC_AutoSyncRef(x()+323, y()+40+V_STEP*4, 148, V_STEP*2);
	system_clock = new HC_SystemClock(x()+323, y()+60+V_STEP*6, 148, V_STEP*2);

    } else if (type == Digiface) {

	clock_source = new HC_ClockSource(x()+9, y()+20, 148, V_STEP*7);
	sync_check = new HC_SyncCheck(x()+9, y()+40+V_STEP*7, 148, V_STEP*6);

	spdif_in = new HC_SpdifIn(x()+166, y()+20, 148, V_STEP*3);
	spdif_out = new HC_SpdifOut(x()+166, y()+40+V_STEP*3, 148, V_STEP*4);
	spdif_freq = new HC_SpdifFreq(x()+166, y()+60+V_STEP*7, 148, V_STEP);

	sync_ref = new HC_PrefSyncRef(x()+323, y()+20, 148, V_STEP*6);
	autosync_ref = new HC_AutoSyncRef(x()+323, y()+40+V_STEP*6, 148, V_STEP*2);
	system_clock = new HC_SystemClock(x()+323, y()+60+V_STEP*8, 148, V_STEP*2);

    } else if (type == H9652) {

	clock_source = new HC_ClockSource(x()+9, y()+20, 148, V_STEP*7);
	sync_check = new HC_SyncCheck(x()+9, y()+40+V_STEP*7, 148, V_STEP*6);

	spdif_in = new HC_SpdifIn(x()+166, y()+20, 148, V_STEP*3);
	spdif_out = new HC_SpdifOut(x()+166, y()+40+V_STEP*3, 148, V_STEP*4);
	spdif_freq = new HC_SpdifFreq(x()+166, y()+60+V_STEP*7, 148, V_STEP);

	aeb = new HC_Aeb(x()+323, y()+20, 148, V_STEP);
	sync_ref = new HC_PrefSyncRef(x()+323, y()+40+V_STEP, 148, V_STEP*6);
	autosync_ref = new HC_AutoSyncRef(x()+323, y()+60+V_STEP*7, 148, V_STEP*2);
	system_clock = new HC_SystemClock(x()+323, y()+80+V_STEP*9, 148, V_STEP*2);

    } else if (type == H9632) {
	clock_source = new HC_ClockSource(x()+8, y()+20, 110, V_STEP*10);
	sync_check = new HC_SyncCheck(x()+8, y()+40+V_STEP*10, 110, V_STEP*3);

	spdif_in = new HC_SpdifIn(x()+126, y()+20, 110, V_STEP*4);
	spdif_out = new HC_SpdifOut(x()+126, y()+40+V_STEP*4, 110, V_STEP*4);
	spdif_freq = new HC_SpdifFreq(x()+126, y()+60+V_STEP*8, 110, V_STEP);

	aeb = new HC_Aeb(x()+244, y()+20, 110, V_STEP);
	sync_ref = new HC_PrefSyncRef(x()+244, y()+40+V_STEP, 110, V_STEP*3);
	autosync_ref = new HC_AutoSyncRef(x()+244, y()+60+V_STEP*4, 110, V_STEP*2);
	system_clock = new HC_SystemClock(x()+244, y()+80+V_STEP*6, 110, V_STEP*2);

	breakout_cable = new HC_BreakoutCable(x()+362, y()+20, 110, V_STEP);
	input_level = new HC_InputLevel(x()+362, y()+40+V_STEP, 110, V_STEP*3);
	output_level = new HC_OutputLevel(x()+362, y()+60+V_STEP*4, 110, V_STEP*3);
	phones = new HC_Phones(x()+362, y()+80+V_STEP*7, 110, V_STEP*3);

    }
    end();
}

