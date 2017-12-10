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
#include "HDSPMixerButtons.h"

HDSPMixerButtons::HDSPMixerButtons(int x, int y, int w, int h):Fl_Group(x, y, w, h)
{
    cardselector = new HDSPMixerCardSelector(x+6, y+227, 61, 13, 1);
    master = new HDSPMixerMaster(x+6 , y+18 , 62, 12);
    view = new HDSPMixerView(x+6, y+53, 13, 76);
    presets = new HDSPMixerPresets(x+6, y+153, 61, 52);
    end();
    preset = 0;
    input = 0;
    playback = 0;
    output = 0;
    submix = 0;
    save = 0;
}

void HDSPMixerButtons::draw_background()
{
    draw_background(x(), y(), w(), h());
}

void HDSPMixerButtons::draw_background(int xpos, int ypos, int w, int h)
{
    fl_push_clip(xpos, ypos, w, h);
    fl_draw_pixmap(buttons_xpm, x(), y());
    fl_pop_clip();
}

void HDSPMixerButtons::draw()
{
    Fl_Widget *const* a = array();
    if (damage() & ~FL_DAMAGE_CHILD) {
	draw_background();
	for (int i=children(); i--;) {
	    Fl_Widget& o = **a++;
	    draw_child(o);
	}
    } else  {
	for (int i=children(); i--;)  update_child(**a++);
    }	
}

void HDSPMixerButtons::update_child(Fl_Widget& widget)
{
    if (widget.damage() && widget.visible() && widget.type() < FL_WINDOW && fl_not_clipped(widget.x(), widget.y(), widget.w(), widget.h())) {
	draw_background(widget.x(), widget.y(), widget.w(), widget.h());
	widget.draw();
	widget.clear_damage();
    }
}

