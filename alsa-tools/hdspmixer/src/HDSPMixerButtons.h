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

#pragma interface
#ifndef HDSPMixerButtons_H
#define HDSPMixerButtons_H

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/fl_draw.H>
#include "HDSPMixerCardSelector.h"
#include "HDSPMixerMaster.h"
#include "HDSPMixerView.h"
#include "HDSPMixerPresets.h"
#include "pixmaps.h"

class HDSPMixerCardSelector;
class HDSPMixerMaster;
class HDSPMixerView;
class HDSPMixerPresets;

class HDSPMixerButtons:public Fl_Group
{
private:
    void update_child(Fl_Widget& widget);
    void draw_background();
    void draw_background(int x, int y, int w, int h);
public:
    HDSPMixerPresets *presets;
    HDSPMixerCardSelector *cardselector;
    HDSPMixerMaster *master;
    HDSPMixerView *view;
    int input, output, playback, submix, preset, save;
    HDSPMixerButtons(int x, int y, int w, int h);
    void draw();   
};

#endif

