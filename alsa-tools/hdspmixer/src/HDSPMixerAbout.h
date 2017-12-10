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
#ifndef HDSPMixerAbout_H
#define HDSPMixerAbout_H

#include <string.h>
#include <FL/Fl_Double_Window.H>
#include "HDSPMixerWindow.h"
#include "HDSPMixerAboutText.h"

class HDSPMixerWindow;
class HDSPMixerAboutText;

class HDSPMixerAbout:public Fl_Double_Window
{
private:
    HDSPMixerAboutText *text;
    HDSPMixerWindow *basew;
public:
    HDSPMixerAbout(int w, int h, char const *label, HDSPMixerWindow *win);
    int handle(int e);
};

#endif

