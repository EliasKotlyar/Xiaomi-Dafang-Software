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
#include "HDSPMixerEmpty.h"

HDSPMixerEmpty::HDSPMixerEmpty(int x, int y, int w, int h, int type):Fl_Widget(x, y, w, h)
{
    linux_advertising = type;
}

void HDSPMixerEmpty::draw()
{
    if (fl_not_clipped(x(), y(), w(), h()) != 0) {
        if (linux_advertising) {
	    fl_draw_pixmap(empty_linux_xpm, x(), y());
	} else {
    	    fl_draw_pixmap(empty_xpm, x(), y());
	}
    }  
}

