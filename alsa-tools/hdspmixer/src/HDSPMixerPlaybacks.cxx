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

#pragma implementation
#include "HDSPMixerPlaybacks.h"

HDSPMixerPlaybacks::HDSPMixerPlaybacks(int x, int y, int w, int h, int nchans):Fl_Group(x, y, w, h)
{
    int i;
    for (i = 0; i < HDSP_MAX_CHANNELS; i += 2) {
	strips[i] = new HDSPMixerIOMixer((i*STRIP_WIDTH), y, STRIP_WIDTH, FULLSTRIP_HEIGHT, i+1, 1); 
	strips[i+1] = new HDSPMixerIOMixer(((i+1)*STRIP_WIDTH), y, STRIP_WIDTH, FULLSTRIP_HEIGHT, i+2, 1); 
	/* Setup linked stereo channels */
	strips[i]->pan->relative = strips[i+1]->pan;
	strips[i+1]->pan->relative = strips[i]->pan;
	strips[i]->mutesolo->relative = strips[i+1]->mutesolo;
	strips[i+1]->mutesolo->relative = strips[i]->mutesolo;
	strips[i]->fader->relative = strips[i+1]->fader;
	strips[i+1]->fader->relative = strips[i]->fader;
	strips[i]->fader->gain = strips[i]->gain;
	strips[i+1]->fader->gain = strips[i+1]->gain;
	strips[i]->gain->relative = strips[i+1]->gain;
	strips[i+1]->gain->relative = strips[i]->gain;
    }
    empty_aebo[0] = new HDSPMixerEmpty((nchans-6)*STRIP_WIDTH, y, 2*STRIP_WIDTH, FULLSTRIP_HEIGHT, 0);
    empty_aebo[1] = new HDSPMixerEmpty((nchans-4)*STRIP_WIDTH, y, 2*STRIP_WIDTH, FULLSTRIP_HEIGHT, 0);
    empty = new HDSPMixerEmpty(nchans*STRIP_WIDTH, y, 2*STRIP_WIDTH, FULLSTRIP_HEIGHT, 1);
    end();
    resizable(NULL);
}

