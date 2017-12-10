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
#ifndef HDSPMixerOutput_H
#define HDSPMixerOutput_H

#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/fl_draw.H>
#include <alsa/sound/hdsp.h>
#include "HDSPMixerFader.h"
#include "HDSPMixerPeak.h"
#include "HDSPMixerGain.h"
#include "HDSPMixerMeter.h"
#include "HDSPMixerOutputData.h"
#include "HDSPMixerWindow.h"
#include "pixmaps.h"

class HDSPMixerFader;
class HDSPMixerGain;
class HDSPMixerPeak;
class HDSPMixerMeter;
class HDSPMixerOutputData;
class HDSPMixerWindow;


class HDSPMixerOutput:public Fl_Group
{
private:
    int out_num;
    char const **labels_input, **labels_playback;
    char const **p_output_xpm;
    HDSPMixerPeak *peak;
    HDSPMixerWindow *basew;    
    void update_child(Fl_Widget& widget);
public:
    HDSPMixerOutputData *data[MAX_CARDS][3][NUM_PRESETS]; /* data[card][mode(ss/ds/qs)][preset number] */
    HDSPMixerFader *fader;
    HDSPMixerGain *gain;
    HDSPMixerMeter *meter;
    HDSPMixerOutput(int x, int y, int w, int h, int out);
    void draw();
    void draw_background();
    void draw_background(int x, int y, int w, int h);
    void setLabels();
};

#endif

