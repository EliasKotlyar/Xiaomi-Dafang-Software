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

#pragma implementation
#include "HDSPMixerPeak.h"

HDSPMixerPeak::HDSPMixerPeak(int x, int y, int parenttype):Fl_Widget(x, y, 30, 13)
{
    parent_iomixer = parenttype;
    over = 0;
    snprintf(text, 10, "-oo");
}

void HDSPMixerPeak::draw()
{
    if (over) {
        fl_draw_pixmap(over_xpm, x(), y());
    }
    fl_color(FL_GREEN);
    fl_font(FL_HELVETICA, 8);
    fl_draw(text, x(), y(), w(), h(), FL_ALIGN_CENTER);
}

void HDSPMixerPeak::update(double maxlevel, int ovr) {
    if (ovr) {
	snprintf(text, 10, "Ovr");
	over = 1;
    } else {
	over = 0;
	if (maxlevel <= 0.001) {
	    snprintf(text, 10, "0.00");
	} else if (maxlevel == 1000.0) {
	    snprintf(text, 10, "-oo");
	} else if (maxlevel >= 100.0) {
	    snprintf(text, 10, "-%.1f", maxlevel);
	} else {
	    snprintf(text, 10, "-%.2f", maxlevel);
	}
    }
    redraw();    
}

