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
#ifndef HDSPMixerFader_H
#define HDSPMixerFader_H

#include <math.h>
#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include "HDSPMixerWindow.h"
#include "HDSPMixerIOMixer.h"
#include "HDSPMixerOutput.h"
#include "pixmaps.h"
#include "defines.h"

class HDSPMixerIOMixer;
class HDSPMixerOutput;
class HDSPMixerGain;
class HDSPMixerWindow;

class HDSPMixerFader:public Fl_Widget
{
private:
    double ref;
    int lastpos, lasty, drag, shift_orig, y_orig, anchor, source, index;
    int onSlider(int ypos);
    void posToLog(char *s);
public:
    int ndb;
    int posToInt(int p);
    int non_submix_dest;
    int dest;
    int pos[HDSP_MAX_DEST];
    HDSPMixerWindow *basew;
    HDSPMixerFader *relative;
    HDSPMixerGain *gain;
    void set(int pos);
    HDSPMixerFader(int x, int y, double r, int id, int src);
    int handle(int e);
    void draw();
    void sendGain();
};

#endif

