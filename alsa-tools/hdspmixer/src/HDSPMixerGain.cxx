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
#include "HDSPMixerGain.h"

HDSPMixerGain::HDSPMixerGain(int x, int y, int parenttype):Fl_Widget(x, y, 30, 13)
{
    setText("-oo");
}

void HDSPMixerGain::draw()
{
    fl_color(FL_GREEN);
    fl_font(FL_HELVETICA, 8);
    fl_draw(text, x(), y(), w(), h(), FL_ALIGN_CENTER);
}

void HDSPMixerGain::setText(char const *txt)
{
    strncpy(text, txt, 10);
    redraw();
}
