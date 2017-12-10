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
#include "HDSPMixerMaster.h"

HDSPMixerMaster::HDSPMixerMaster(int x, int y, int w, int h):Fl_Widget(x, y, 62, 12)
{
    basew = (HDSPMixerWindow *)window();
    solo = mute = solo_active = mute_active = 0;
}


void HDSPMixerMaster::draw() 
{
    if (mute && mute_active) {
	fl_push_clip(x(), y(), 30, 11);
	fl_draw_pixmap(b_mute_xpm, x(), y());
	fl_pop_clip();
    } else if (mute) {
	fl_push_clip(x(), y(), 30, 11);
	fl_draw_pixmap(b_mute_xpm, x(), y()-11);
	fl_pop_clip();
    }	
    if (solo && solo_active) {
	fl_push_clip(x()+32, y(), 30, 11);
	fl_draw_pixmap(b_solo_xpm, x()+32, y());
	fl_pop_clip();
    } else if (solo) {
	fl_push_clip(x()+32, y(), 30, 11);
	fl_draw_pixmap(b_solo_xpm, x()+32, y()-11);
	fl_pop_clip();
    }	
}

int HDSPMixerMaster::handle(int e)
{
    int xpos = Fl::event_x()-x();
    switch (e) {
	case FL_PUSH:
	    if (xpos >= 0 && xpos <= 30) {
		if (mute) {
		    mute = 0;
		} else {
		    mute = 1;
		}
		for (int i = 0; i < basew->cards[basew->current_card]->channels_input; i++) {
		    basew->inputs->strips[i]->mutesolo->redraw();
		}
		for (int i = 0; i < basew->cards[basew->current_card]->channels_playback; i++) {
		    basew->playbacks->strips[i]->mutesolo->redraw();
		}
		basew->refreshMixer();
		redraw();
		basew->checkState();
		return 1;
	    }
	    if (xpos >= 32) {
		if (solo) {
		    solo = 0;
		} else {
		    solo = 1;
		}
		for (int i = 0; i < basew->cards[basew->current_card]->channels_input; i++) {
		    basew->inputs->strips[i]->mutesolo->redraw();
		}
		for (int i = 0; i < basew->cards[basew->current_card]->channels_playback; i++) {
		    basew->playbacks->strips[i]->mutesolo->redraw();
		}
		basew->refreshMixer();
		redraw();
		basew->checkState();
		return 1;
	    }
	default:
	    return Fl_Widget::handle(e);
    }    
}

