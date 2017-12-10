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
#include "HDSPMixerPan.h"

HDSPMixerPan::HDSPMixerPan(int x, int y, int id, int src):Fl_Widget(x, y, 30, 13)
{
    source = src;
    index = id;
    basew = (HDSPMixerWindow *)window();
    dest =  x_orig = shift_orig = lastpos = lastx = drag = 0;
    for (int i = 0; i < HDSP_MAX_DEST; i++) {
	pos[i] = 0;
    }
}

void HDSPMixerPan::draw()
{ 
    fl_draw_pixmap(Slider2_xpm, x()+(int)(pos[dest]/CF), y());
}

int HDSPMixerPan::handle(int e)
{
    int button3 = Fl::event_button3();
    int shift = Fl::event_shift();
    int ctrl = Fl::event_ctrl();
    int xpos = Fl::event_x()-x();
    switch (e) {
	case FL_PUSH:
	    if (xpos > 0 && xpos < 30) {
		if (ctrl) {
		    pos[dest] = 14*CF;
		} else {
		    pos[dest] = (xpos-1)*CF;
		}
		if (lastx != (int)(pos[dest]/CF)) {
		    redraw();
		    lastx = (int)(pos[dest]/CF);
		}
		if (lastpos != pos[dest]) {
		    basew->setMixer(index, source, dest);
		    sendText();
		    lastpos = pos[dest];
		}
		if (button3) relative->set(28*CF-pos[dest]);
		shift_orig = pos[dest];
		x_orig = xpos;
		basew->checkState();
	    }
	    return 1;
	case FL_DRAG:
	    if (ctrl) {
		pos[dest] = 14*CF;
		shift_orig = pos[dest];
		x_orig = xpos;
	    } else if ((xpos > 0 && xpos < 30) || drag) {
		drag = 1;
		if (shift) {
		    pos[dest] = ((xpos-1)-x_orig)+shift_orig;
		    if (pos[dest] < 0) pos[dest] = 0;
		    if (pos[dest] > 28*CF) pos[dest] = 28*CF;
		} else {
		    if (xpos < 1) {
			pos[dest] = 0;
		    } else if (xpos > 29) {
			pos[dest] = 28*CF;
		    } else {
			pos[dest] = (xpos-1)*CF;
		    }
		    shift_orig = pos[dest];
		    x_orig = xpos;
		}
	    }
	    if (lastpos != pos[dest]) {
		basew->setMixer(index, source, dest);
		sendText();
		lastpos = pos[dest];
	    }
	    if (lastx != (int)(pos[dest]/CF)) {
		redraw();
		lastx = (int)(pos[dest]/CF);
	    }
	    if (button3) relative->set(28*CF-pos[dest]);
	    basew->checkState();
	    return 1;
	case FL_RELEASE:
	    drag = 0;
	    return 1;   
	default :
	    return Fl_Widget::handle(e);
    }
}

void HDSPMixerPan::panToText(char *s) 
{
    double x;
    
    x = (double)pos[dest] / (double)(28*CF);
    
    if (pos[dest] < 28*CF/2) {
	x = 1.0 - x;
	snprintf(s, 10, "L %.2f", x);
    } else if (pos[dest] > 28*CF/2) {
	snprintf(s, 10, "R %.2f", x);
    } else {
	snprintf(s, 10, "<C>");
    } 
}
void HDSPMixerPan::sendText() {
    char buf[10];
    panToText(buf);
    ((HDSPMixerIOMixer *)parent())->gain->setText(buf);
}

void HDSPMixerPan::set(int p)
{
    if (pos[dest] != p) {
	pos[dest] = lastpos = p;
	basew->setMixer(index, source, dest);
	sendText();
    }
    if (lastx != (int)(pos[dest]/CF)) {
	lastx = (int)(pos[dest]/CF);
	redraw();
    }
}

