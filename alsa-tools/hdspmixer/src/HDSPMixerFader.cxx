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
#include "HDSPMixerFader.h"

HDSPMixerFader::HDSPMixerFader(int x, int y, double r, int id, int src):Fl_Widget(x, y, 13, 153)
{
    index = id;
    source = src;
    ndb = (int)(CF*(double)(log(0.5*(exp(3.0)-1)+1)*(double)(FADER_HEIGHT)/3.0));
    non_submix_dest = 0;
    dest = 0;
    ref = r;
    basew = (HDSPMixerWindow *)window();
    anchor = lastpos = lasty = drag = shift_orig = y_orig = 0;
    for (int i = 0; i < HDSP_MAX_DEST; i++) {
	pos[i] = 0;
    }
}

void HDSPMixerFader::draw()
{
    fl_draw_pixmap(Slider1_xpm, x(), y()+139-(int)(pos[dest]/CF));
}

int HDSPMixerFader::handle(int e)
{
    int button3 = Fl::event_button3();
    int shift = Fl::event_shift();
    int ctrl = Fl::event_ctrl();
    int ypos = Fl::event_y()-y();
    switch (e) {
	case FL_PUSH:
	    if (onSlider(ypos)) {
		anchor = 144-ypos-(int)(pos[dest]/CF);
		if (button3) relative->set(pos[dest]);
		return 1;
	    }
	    anchor = 0;
	    if (ctrl) {
		pos[dest] = ndb;
	    } else if (ypos < 7) {
		pos[dest] = 137*CF;
	    } else if (ypos > 6 && ypos < 146) {
		pos[dest] = (144-ypos)*CF;
	    } else if (ypos > 145) {
		pos[dest] = 0;
	    } 
	    if (lastpos != pos[dest]) {
		basew->setMixer(index, source, dest);
		sendGain();
		lastpos = pos[dest];
	    }
	    if (lasty != (int)(pos[dest]/CF)) {
		redraw();
		lasty = (int)(pos[dest]/CF);
	    }
	    if (button3) relative->set(pos[dest]);
	    shift_orig = pos[dest];
	    y_orig = ypos;
	    basew->checkState();
	    return 1;
	case FL_DRAG:
	    ypos += anchor;
	    if (ctrl) {
		pos[dest] = ndb;
		shift_orig = pos[dest];
		y_orig = ypos;
	    } else if ((ypos > 6 && ypos < 146) || drag) {
		drag = 1;
		if (shift) {
		    pos[dest] = (y_orig-ypos)+shift_orig;
		    if (pos[dest] < 0) pos[dest] = 0;
		    if (pos[dest] > 137*CF) pos[dest] = 137*CF;
		} else {
		    if (ypos < 7) {
			pos[dest] = 137*CF;
		    } else if (ypos > 144) {
			pos[dest] = 0;
		    } else {
			pos[dest] = (144-ypos)*CF;
		    }
		    shift_orig = pos[dest];   
		    y_orig = ypos;
		}
	    }
	    if (lastpos != pos[dest]) {
		basew->setMixer(index, source, dest);
		sendGain();
		lastpos = pos[dest];
	    }
	    if (lasty != (int)(pos[dest]/CF)) {
		redraw();
		lasty = (int)(pos[dest]/CF);
	    }
	    if (button3) relative->set(pos[dest]);
	    basew->checkState();
	    return 1;
	case FL_RELEASE:
	    drag = 0;
	    anchor = 0;
	    return 1;   
	default :
	    return Fl_Widget::handle(e);
    }
}

int HDSPMixerFader::onSlider(int ypos)
{
    if (ypos > (139-(int)(pos[dest]/CF)) && ypos < (151-(int)(pos[dest]/CF))) {
	return 1;
    }
    return 0;
}

void HDSPMixerFader::set(int p)
{
    if (p != pos[dest]) {
	pos[dest] = lastpos = p;
	basew->setMixer(index, source, dest);
	sendGain();
    }
    if (lasty != (int)(pos[dest]/CF)) {
	lasty = (int)(pos[dest]/CF);
	redraw();
    }
}

int HDSPMixerFader::posToInt(int p) {
    double x, y;
    
    if (p == ndb) return 32768;
    if (p == 137*CF) return 65535;
    if (p == 0) return 0;
    
    x = ((double)(p)) / (double)(137*CF);
    y = 65535.0 * (exp(3.0 * x) - 1.0) / (exp(3.0) - 1.0);
    if (y > 65535.0) y = 65535.0;
    if (y < 0.0) y = 0.0;
    return (int)y;
}

void HDSPMixerFader::posToLog(char *s)
{
    double db, fpos;
    
    if (posToInt(pos[dest]) == 0) {
	snprintf(s, 10, "-oo");
	return;
    }    
    fpos = (double)posToInt(pos[dest]) / ref;
    db = 20.0 * log10(fpos);
    snprintf(s, 10, "%.1f", db);
}

void HDSPMixerFader::sendGain()
{
    char buf[10];
    posToLog(buf);
    gain->setText(buf);
}

