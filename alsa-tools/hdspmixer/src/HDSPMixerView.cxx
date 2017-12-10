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
#include "HDSPMixerView.h"

HDSPMixerView::HDSPMixerView(int x, int y, int w, int h):Fl_Widget(x, y, 13, 76)
{
    basew = (HDSPMixerWindow *)window();
    submix = input = playback = output = 1;
    submix_value = 0;
}


void HDSPMixerView::draw() 
{
    if (input) {
	fl_draw_pixmap(b_blank_xpm, x(), y());
    }
    if (playback) {
	fl_draw_pixmap(b_blank_xpm, x(), y()+20);
    }
    if (output) {
	fl_draw_pixmap(b_blank_xpm, x(), y()+40);
    }
    if (submix) {
	fl_draw_pixmap(b_blank_xpm, x(), y()+63);
    }
}

int HDSPMixerView::handle(int e)
{
    int ypos = Fl::event_y()-y();
    switch (e) {
	case FL_PUSH:
	    if (ypos < 13) {
		if (input) {
		    input = 0;
		    basew->menubar->mode(9, FL_MENU_TOGGLE);
		 } else {
		    basew->menubar->mode(9, FL_MENU_TOGGLE|FL_MENU_VALUE);
		    input = 1;
		}
		redraw();
		basew->reorder();
	    } else if (ypos >= 20 && ypos < 33) {
		if (playback) {
		    basew->menubar->mode(10, FL_MENU_TOGGLE);
		    playback = 0;
		} else {
		    basew->menubar->mode(10, FL_MENU_TOGGLE|FL_MENU_VALUE);
		    playback = 1;
		}
		redraw();
		basew->reorder();
	    } else if (ypos >= 40 && ypos < 53) {
		if (output) {
		    basew->menubar->mode(11, FL_MENU_TOGGLE|FL_MENU_DIVIDER);
		    output = 0;
		} else {
		    basew->menubar->mode(11, FL_MENU_TOGGLE|FL_MENU_VALUE|FL_MENU_DIVIDER);
		    output = 1;
		}
		redraw();
		basew->reorder();
	    } else if (ypos >= 63) {
		if (submix) {
		    basew->menubar->mode(12, FL_MENU_TOGGLE);
		    submix = 0;
		    basew->unsetSubmix();
		} else {
		    basew->menubar->mode(12, FL_MENU_TOGGLE|FL_MENU_VALUE);
		    basew->setSubmix(submix_value);
		    submix = 1;
		}
		redraw();
		basew->reorder();
	    }
	    basew->checkState();
	    return 1;
	default:
	    return Fl_Widget::handle(e);
    }    
}

