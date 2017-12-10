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
#ifndef HDSPMixerPan_H
#define HDSPMixerPan_H

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include "HDSPMixerIOMixer.h"
#include "HDSPMixerWindow.h"
#include "pixmaps.h"
#include "defines.h"

class HDSPMixerIOMixer;
class HDSPMixerWindow;

class HDSPMixerPan:public Fl_Widget
{
private:
    int drag, shift_orig,  x_orig, lastpos, lastx;
    void sendText();
    void panToText(char *s);
public:
    int pos[HDSP_MAX_DEST];
    int dest;
    int index;
    int source;
    HDSPMixerWindow *basew;
    HDSPMixerPan *relative;
    HDSPMixerPan(int x, int y, int id, int src);
    int handle(int e);
    void draw();
    void set(int pos);
};

#endif

