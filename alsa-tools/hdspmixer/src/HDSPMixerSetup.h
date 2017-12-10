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
#ifndef HDSPMixerSetup_H
#define HDSPMixerSetup_H

#include <string.h>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Round_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Return_Button.H>
#include "HDSPMixerWindow.h"

class HDSPMixerWindow;

class HDSPMixerSetup:public Fl_Double_Window
{
public:
    Fl_Group *plm;
    Fl_Group *numbers;
    Fl_Group *level;
    Fl_Group *rmsplus3grp;
    Fl_Counter *over;
    Fl_Choice *rate;
    Fl_Round_Button *rms;
    Fl_Round_Button *peak;
    Fl_Round_Button *fourty;
    Fl_Round_Button *sixty;
    Fl_Check_Button *rmsplus3;
    Fl_Return_Button *ok;
    int rate_val;
    int over_val;
    int level_val;
    int numbers_val;
    int rmsplus3_val;
    HDSPMixerSetup(int w, int h, char const *label, HDSPMixerWindow *win);
    HDSPMixerWindow *basew;
    void updateValues();
};

#endif

