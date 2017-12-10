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
#include "HDSPMixerIOMixer.h"

HDSPMixerIOMixer::HDSPMixerIOMixer(int x, int y, int w, int h, int ch, int type):Fl_Group(x, y, w, h)
{
    mixer_type = type;
    if (type) {
	channel_name << "Out " << ch;
    } else {
	channel_name << "In " << ch;
    }
    channel_num = ch;
    if (channel_num%2) {
	relative_num = channel_num+1;
	p_iomixer_xpm = iomixer_xpm;
    } else {
	relative_num = channel_num-1;
	p_iomixer_xpm = iomixer_r_xpm;
    }
    for (int j = 0; j < MAX_CARDS; ++j) {
	for (int i = 0; i < NUM_PRESETS; ++i) {
	    data[j][0][i] = new HDSPMixerStripData();
	    data[j][1][i] = new HDSPMixerStripData();
	    data[j][2][i] = new HDSPMixerStripData();
	}
    }
    mutesolo = new HDSPMixerMuteSolo(x+3, y+3, 0, 0, channel_num, type);
    gain = new HDSPMixerGain(x+3, y+207, 1);
    peak = new HDSPMixerPeak(x+3, y+36, 1);
    fader = new HDSPMixerFader(x+4, y+51, 32768.0, channel_num, type);
    pan = new HDSPMixerPan(x+3, y+19, channel_num, type) ;
    targets = new HDSPMixerSelector(x+3, y+240, 29, 10);
    meter = new HDSPMixerMeter(x+20, y+59, true, peak);
    end();
}

void HDSPMixerIOMixer::draw_background() 
{
    draw_background(x(), y(), w(), h());
}

void HDSPMixerIOMixer::draw_background(int xpos, int ypos, int w, int h)
{
    fl_push_clip(xpos, ypos, w, h);
    fl_draw_pixmap(p_iomixer_xpm, x(), y());
    fl_pop_clip();
}

void HDSPMixerIOMixer::draw()
{
    Fl_Widget *const* a = array();
    if (damage() & ~FL_DAMAGE_CHILD) {
	draw_background();
	fl_color(FL_FOREGROUND_COLOR);
	fl_font(FL_HELVETICA, 8);
	fl_draw(channel_name.str().c_str(), x()+4, y()+225, 27, 9, FL_ALIGN_CENTER);
	for (int i=children(); i--;) {
	    Fl_Widget& o = **a++;
	    draw_child(o);
	}
    } else {
	for (int i=children(); i--;) update_child(**a++);
    }
}

void HDSPMixerIOMixer::update_child(Fl_Widget& widget) 
{
    if (widget.damage() && widget.visible() && widget.type() < FL_WINDOW && fl_not_clipped(widget.x(), widget.y(), widget.w(), widget.h())) {
	if ((HDSPMixerMeter*)&widget == meter) {
	    ((HDSPMixerMeter *)&widget)->fine_draw = 1;
	} else {
	    draw_background(widget.x(), widget.y(), widget.w(), widget.h());
	}
	widget.draw();
	widget.clear_damage();
    }
}

