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
#ifndef HC_SPDIFIN_H
#define HC_SPDIFIN_H

#include <FL/Fl_Group.H>
#include <FL/Fl_Round_Button.H>
#include <alsa/asoundlib.h>
#include "HC_CardPane.h"

class HC_CardPane;

class HC_SpdifIn:public Fl_Group
{
public:
    HC_SpdifIn(int x, int y, int w, int h);
    int source;
    Fl_Round_Button *adat1;
    Fl_Round_Button *coaxial;
    Fl_Round_Button *internal;
    Fl_Round_Button *aes;
    void setInput(unsigned char i);
};

#endif

