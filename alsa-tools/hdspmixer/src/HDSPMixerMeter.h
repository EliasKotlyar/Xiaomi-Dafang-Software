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
#ifndef HDSPMixerMeter_H
#define HDSPMixerMeter_H

#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include "HDSPMixerWindow.h"
#include "HDSPMixerPeak.h"
#include "pixmaps.h"
#include "defines.h"

class HDSPMixerWindow;
class HDSPMixerPeak;

class HDSPMixerMeter:public Fl_Widget
{
private:
    HDSPMixerWindow *basew;
    HDSPMixerPeak *peaktext;
    int logToHeight(double db);
    double fast_peak_level, max_level, slow_peak_level; 
    bool peak_rms;
    int peak_height, rms_height, count, new_peak_height, new_rms_height;
public:
    int fine_draw;
    void draw();
    void update(int peak, int overs, int64 rms);
    HDSPMixerMeter(int x, int y, bool not_output, HDSPMixerPeak *p);
};

#endif

