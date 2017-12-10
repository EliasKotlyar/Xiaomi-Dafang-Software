/*
 *   HDSPConf
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
#ifndef HC_AUTOSYNCREF_H
#define HC_AUTOSYNCREF_H

#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include "HC_CardPane.h"
#include "labels.h"

class HC_CardPane;

class HC_AutoSyncRef:public Fl_Widget
{
public:
    HC_AutoSyncRef(int x, int y, int w, int h);
    void draw();
    int external_freq, external_ref;
    void setFreq(int f);
    void setRef(unsigned char r);
private:
    Fl_Box_Draw_F *draw_box;
};

#endif

