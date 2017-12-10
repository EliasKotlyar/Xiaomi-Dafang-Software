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

#pragma implementation
#include "HC_SpdifIn.h"

void spdif_in_cb(Fl_Widget *w, void *arg)
{
    int err;
    char card_name[6];
    snd_ctl_elem_value_t *ctl;
    snd_ctl_elem_id_t *id;
    snd_ctl_t *handle;
    int in = 0;
    Fl_Round_Button *source = (Fl_Round_Button *)w;
    HC_SpdifIn *si = (HC_SpdifIn *)arg;
    HC_CardPane *pane = (HC_CardPane *)si->parent();
    if (source == si->adat1) {
	in = 0;
    } else if (source == si->coaxial) {
	in = 1;
    } else if (source == si->internal) {
	in = 2;
    } else if (source == si->aes) {
	in = 3;
    }
    snprintf(card_name, 6, "hw:%i", pane->alsa_index);
    snd_ctl_elem_value_alloca(&ctl);
    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_id_set_name(id, "IEC958 Input Connector");
    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_index(id, 0);
    snd_ctl_elem_value_set_id(ctl, id);
    snd_ctl_elem_value_set_enumerated(ctl, 0, in);
    if ((err = snd_ctl_open(&handle, card_name, SND_CTL_NONBLOCK)) < 0) {
	fprintf(stderr, "Error opening ctl interface on card %s\n", card_name);
	return;
    }
    if ((err = snd_ctl_elem_write(handle, ctl)) < 0) {
	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_PCM);
	snd_ctl_elem_value_set_id(ctl, id);
	if ((err = snd_ctl_elem_write(handle, ctl)) < 0) {
	    fprintf(stderr, "Error accessing ctl interface on card %s\n", card_name);
	    snd_ctl_close(handle);
	    return;
	}
    }
    snd_ctl_close(handle);
}

HC_SpdifIn::HC_SpdifIn(int x, int y, int w, int h):Fl_Group(x, y, w, h, "SPDIF In")
{
	int i = 0;
	box(FL_ENGRAVED_FRAME);;
	label("SPDIF In");
	labelsize(10);
	align(FL_ALIGN_TOP|FL_ALIGN_LEFT);
	adat1 = new Fl_Round_Button(x+10, y+V_STEP*i++, w-20, V_STEP, "Optical");
	coaxial = new Fl_Round_Button(x+10, y+V_STEP*i++, w-20, V_STEP, "Coaxial");
	internal = new Fl_Round_Button(x+10, y+V_STEP*i++, w-20, V_STEP, "Internal");
	if (((HC_CardPane *)parent())->type == H9632) {
	    aes = new Fl_Round_Button(x+10, y+V_STEP*i++, w-20, V_STEP, "AES");
	    aes->labelsize(10);
	    aes->type(FL_RADIO_BUTTON);
	    aes->callback(spdif_in_cb, (void *)this);
	}
	adat1->labelsize(10);
	adat1->type(FL_RADIO_BUTTON);
	adat1->callback(spdif_in_cb, (void *)this);
	coaxial->labelsize(10);
	coaxial->type(FL_RADIO_BUTTON);
	coaxial->callback(spdif_in_cb, (void *)this);
	internal->labelsize(10);
	internal->type(FL_RADIO_BUTTON);
	internal->callback(spdif_in_cb, (void *)this);
	end();	
}

void HC_SpdifIn::setInput(unsigned char i)
{
	if (i < children()) {
	    if (((Fl_Round_Button *)child(i))->value() != 1)
		((Fl_Round_Button *)child(i))->setonly();
	}
}

