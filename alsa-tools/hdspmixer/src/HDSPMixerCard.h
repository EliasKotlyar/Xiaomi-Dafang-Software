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
#ifndef HDSPMixerCard_H
#define HDSPMixerCard_H

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <alsa/asoundlib.h>
#include <alsa/sound/hdsp.h>
#include <alsa/sound/hdspm.h>
#include "defines.h"
#include "channelmap.h"
#include "HDSPMixerWindow.h"

/* temporary workaround until hdsp.h (HDSP_IO_Type gets fixed */
#ifndef RPM
# define RPM	4
#endif

class HDSPMixerWindow;

class HDSPMixerCard
{
private:
    snd_ctl_t *cb_handle;
    snd_async_handler_t *cb_handler;

public:
    HDSPMixerWindow *basew;
    char name[6];
    std::string cardname;
    HDSPMixerCard(int cardtype, int id, char *shortname);
    int channels_input, channels_playback, window_width, window_height, card_id;
    int channels_output;
    int type;
    int last_preset; /* Last activated preset before switching to another card */
    int last_dirty; /* Last dirty flag before switching to another card */
    char *channel_map_input, *channel_map_playback;
    char *dest_map;
    char *meter_map_input, *meter_map_playback;
    int speed_mode;
    int playbacks_offset;
    void setMode(int mode);
    int initializeCard(HDSPMixerWindow *w);
    int getSpeed();
    int getAutosyncSpeed();
    void actualizeStrips();
    void adjustSettings();
    void getAeb();
    hdsp_9632_aeb_t h9632_aeb;
};

#endif

