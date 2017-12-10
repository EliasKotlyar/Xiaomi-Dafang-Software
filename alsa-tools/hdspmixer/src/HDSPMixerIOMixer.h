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
#ifndef HDSPMixerIOMixer_H
#define HDSPMixerIOMixer_H

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/fl_draw.H>
#include <stdio.h>
#include "HDSPMixerWindow.h"
#include "HDSPMixerSelector.h"
#include "HDSPMixerPan.h"
#include "HDSPMixerFader.h"
#include "HDSPMixerGain.h"
#include "HDSPMixerPeak.h"
#include "HDSPMixerMuteSolo.h"
#include "HDSPMixerStripData.h"
#include "HDSPMixerMeter.h"
#include "pixmaps.h"
#include <sstream>

class HDSPMixerWindow;
class HDSPMixerSelector;
class HDSPMixerPan;
class HDSPMixerFader;
class HDSPMixerGain;
class HDSPMixerPeak;
class HDSPMixerMuteSolo;
class HDSPMixerStripData;
class HDSPMixerMeter;

class HDSPMixerIOMixer:public Fl_Group
{
private:
    char const **p_iomixer_xpm;
    int channel_num, relative_num, mixer_type;
	std::stringstream channel_name;
    void update_child(Fl_Widget &widget);
public:
    HDSPMixerStripData *data[MAX_CARDS][3][NUM_PRESETS]; /* data[card][mode(ss/ds/qs)][preset number] */
    HDSPMixerPan *pan;
    HDSPMixerFader *fader;
    HDSPMixerPeak *peak;
    HDSPMixerGain *gain;
    HDSPMixerMuteSolo *mutesolo;
    HDSPMixerSelector *targets;
    HDSPMixerMeter *meter;
    HDSPMixerIOMixer(int x, int y, int w, int h, int channelnum, int type);
    void draw();
    void draw_background();
    void draw_background(int x, int y, int w, int h);
    void register_relatives();
};

#endif

