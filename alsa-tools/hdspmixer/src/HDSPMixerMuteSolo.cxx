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
#include "HDSPMixerMuteSolo.h"

HDSPMixerMuteSolo::HDSPMixerMuteSolo(int x, int y, int muteinit, int soloinit, int idx, int src):Fl_Widget(x, y, 30, 13)
{
    basew = (HDSPMixerWindow *)window();
    source = src;
    index = idx;
    mute = muteinit;
    solo = soloinit;
}

void HDSPMixerMuteSolo::draw()
{
    int gmute = basew->inputs->buttons->master->mute;
    int gsolo = basew->inputs->buttons->master->solo;
    int gsolo_active = basew->inputs->buttons->master->solo_active;
    if (solo && gsolo) {
	fl_push_clip(x()+17, y(), 13, 13);
	fl_draw_pixmap(solo_xpm, x()+17, y());
	fl_pop_clip();
    } else if (solo) {
	fl_push_clip(x()+17, y(), 13, 13);
	fl_draw_pixmap(solo_xpm, x()+17, y()-13);
	fl_pop_clip();
    }
    if (((mute && gmute) || (gsolo && gsolo_active)) && !(solo && gsolo)) {
	fl_push_clip(x(), y(), 13, 13);
	fl_draw_pixmap(mute_xpm, x(), y());
	fl_pop_clip();
    } else if (mute) {
	fl_push_clip(x(), y(), 13, 13);
	fl_draw_pixmap(mute_xpm, x(), y()-13);
	fl_pop_clip();
    }
}

void HDSPMixerMuteSolo::setMute(int m)
{
    if (m != mute) {
	mute = m;
	if (mute) {
	    basew->inputs->buttons->master->mute_active++;
	} else {
	    basew->inputs->buttons->master->mute_active--;
	}
	basew->refreshMixerStrip(index, source);
	redraw();
    }
}

void HDSPMixerMuteSolo::setSolo(int s)
{
    if (s != solo) {
	solo = s;
	if (solo) {
	    basew->inputs->buttons->master->solo_active++;
	} else {
	    basew->inputs->buttons->master->solo_active--;
	}
	basew->refreshMixer();
	redraw_all();
    }
}

int HDSPMixerMuteSolo::handle(int e)
{
    int button3 = Fl::event_button3();
    int xpos = Fl::event_x()-x();
    switch (e) {
	case FL_PUSH:
	    if (xpos < 13) {
		if (mute) {
		    mute = 0;
		    basew->inputs->buttons->master->mute_active--;
		} else {
		    mute = 1;
		    basew->inputs->buttons->master->mute_active++;
		}
		basew->inputs->buttons->master->redraw();
		basew->refreshMixerStrip(index, source);
		redraw();
		if (button3)
		    relative->setMute(mute);
	    } else if (xpos > 16) {
		if (solo) {
		    solo = 0;
		    basew->inputs->buttons->master->solo_active--;
		} else {
		    solo = 1;
		    basew->inputs->buttons->master->solo_active++;
		}
		basew->inputs->buttons->master->redraw();
		redraw();
		basew->refreshMixerStrip(index, source);
		if (button3) {
		    relative->setSolo(solo);
		} else {
		    basew->refreshMixer();
		    redraw_all();
		}	
	    }
	    basew->checkState();
	    return 1;
	default:
	    return Fl_Widget::handle(e);
    }	 
}

void HDSPMixerMuteSolo::redraw_all()
{
    for (int i = 0; i < (basew->cards[basew->current_card]->channels_input); ++i) {
	basew->inputs->strips[i]->mutesolo->redraw();
    }

    for (int i = 0; i < (basew->cards[basew->current_card]->channels_playback); ++i) {
	basew->playbacks->strips[i]->mutesolo->redraw();	
    }
}

