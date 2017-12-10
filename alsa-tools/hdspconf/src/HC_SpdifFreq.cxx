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
#include "HC_SpdifFreq.h"

extern const char *freqs[10];

HC_SpdifFreq::HC_SpdifFreq(int x, int y, int w, int h):Fl_Widget(x, y, w, h, "SPDIF Freq.")
{
	spdif_freq = 0;
	draw_box = Fl::get_boxtype(FL_ENGRAVED_FRAME);
	label("SPDIF Freq.");
	labelsize(10);
	align(FL_ALIGN_TOP|FL_ALIGN_LEFT);
}

void HC_SpdifFreq::draw()
{
	fl_color(FL_BACKGROUND_COLOR);
	fl_rectf(x(), y(), w(), h());
	draw_box(x(), y(), w(), h(), FL_WHITE);
	fl_color(FL_FOREGROUND_COLOR);
	fl_font(FL_HELVETICA, 10);
	fl_draw(freqs[spdif_freq], x(), y(), w(), h(), FL_ALIGN_CENTER);
}

void HC_SpdifFreq::setFreq(int f)
{
	int freq;
	switch (f) {
	case 32000:
	    freq = 0;
	    break;
	case 44100:
	    freq = 1;
	    break;
	case 48000:
	    freq = 2;
	    break;
	case 64000:
	    freq = 3;
	    break;
	case 88200:
	    freq = 4;
	    break;
	case 96000:
	    freq = 5;
	    break;
	case 128000:
	    freq = 7;
	    break;
	case 176400:
	    freq = 8;
	    break;
	case 192000:
	    freq = 9;
	    break;
	default:
	    freq = 6;
	}
	if (freq != spdif_freq) {
	    spdif_freq = freq;
	    redraw();
	}
}
