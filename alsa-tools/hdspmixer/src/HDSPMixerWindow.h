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
#ifndef HDSPMixerWindow_H
#define HDSPMixerWindow_H

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Preferences.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Menu.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/filename.H>
#include <FL/fl_ask.H>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <alsa/asoundlib.h>
#include <alsa/sound/hdsp.h>
#include "HDSPMixerCard.h"
#include "HDSPMixerInputs.h"
#include "HDSPMixerOutputs.h"
#include "HDSPMixerPresetData.h"
#include "HDSPMixerPlaybacks.h"
#include "HDSPMixerSetup.h"
#include "HDSPMixerAbout.h"
#include "defines.h"

class HDSPMixerInputs;
class HDSPMixerOutputs;
class HDSPMixerPlaybacks;
class HDSPMixerPresetData;
class HDSPMixerSetup;
class HDSPMixerAbout;
class HDSPMixerCard;

class HDSPMixerWindow:public Fl_Double_Window 
{
private:
    int buttons_removed;
public:
    int current_card;
    int current_preset;
    int dirty;
    char file_name_buffer[FL_PATH_MAX];
    char window_title[FL_PATH_MAX];
    char *file_name;
    Fl_Preferences *prefs;
    Fl_Menu_Bar *menubar;
    Fl_Scroll *scroll;
    HDSPMixerSetup *setup;
    HDSPMixerAbout *about;
    HDSPMixerPresetData *data[MAX_CARDS][3][NUM_PRESETS]; /* data[card number][mode(ss/ds/qs)][preset number] */
    HDSPMixerCard *cards[MAX_CARDS];
    HDSPMixerInputs *inputs;
    HDSPMixerPlaybacks *playbacks;
    HDSPMixerOutputs *outputs;
    HDSPMixerWindow(int x, int y, int w, int h, const char *label, class HDSPMixerCard *hdsp_card1, class HDSPMixerCard *hdsp_card2, class HDSPMixerCard *hdsp_card3);
    void reorder();
    int handle(int e);
    void resize(int x, int y, int w, int h);
    void checkState();
    void setSubmix(int submix_value);
    void unsetSubmix();
    void setMixer(int idx, int src, int dest);
    void refreshMixer();
    void setGain(int in, int out, int value);
    void resetMixer();
    void restoreDefaults(int card);
    void refreshMixerStrip(int idx, int src);
    void save();
    void load();
    void setTitle(std::string suffix);
    void setTitleWithFilename();
    void stashPreset();
    void unstashPreset();
};

#endif

