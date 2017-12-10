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
#include "HC_SpdifOut.h"

static void setSpdifBit(char *ctl_name, int val, int card_index)
{
    int err;
    char card_name[6];
    snd_ctl_elem_value_t *ctl;
    snd_ctl_elem_id_t *id;
    snd_ctl_t *handle;
    snprintf(card_name, 6, "hw:%i", card_index);
    snd_ctl_elem_value_alloca(&ctl);
    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_id_set_name(id, ctl_name);
    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_index(id, 0);
    snd_ctl_elem_value_set_id(ctl, id);
    snd_ctl_elem_value_set_integer(ctl, 0, val);
    if ((err = snd_ctl_open(&handle, card_name, SND_CTL_NONBLOCK)) < 0) {
	fprintf(stderr, "Error opening ctl interface on card %s\n", card_name);
	return;
    }
    if ((err = snd_ctl_elem_write(handle, ctl)) < 0) {
	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_HWDEP);
	snd_ctl_elem_value_set_id(ctl, id);
	if ((err = snd_ctl_elem_write(handle, ctl)) < 0) {
	    fprintf(stderr, "Error accessing ctl interface on card %s\n", card_name);
	    snd_ctl_close(handle);
	    return;
	}
    }
    snd_ctl_close(handle);
}

void spdif_on_adat_cb(Fl_Widget *w, void *arg)
{
    setSpdifBit("IEC958 Output also on ADAT1", ((Fl_Check_Button *)w)->value(), ((HC_CardPane *)arg)->alsa_index);
}

void spdif_professional_cb(Fl_Widget *w, void *arg)
{
    setSpdifBit("IEC958 Professional Bit", ((Fl_Check_Button *)w)->value(), ((HC_CardPane *)arg)->alsa_index);
}

void spdif_emphasis_cb(Fl_Widget *w, void *arg)
{
    setSpdifBit("IEC958 Emphasis Bit", ((Fl_Check_Button *)w)->value(), ((HC_CardPane *)arg)->alsa_index);
}

void spdif_nonaudio_cb(Fl_Widget *w, void *arg)
{
    setSpdifBit("IEC958 Non-audio Bit", ((Fl_Check_Button *)w)->value(), ((HC_CardPane *)arg)->alsa_index);
}

HC_SpdifOut::HC_SpdifOut(int x, int y, int w, int h):Fl_Group(x, y, w, h, "SPDIF Out")
{
	int i = 0;
	lock = 0;
	int v_step = (int)(h/4.0f);
	box(FL_ENGRAVED_FRAME);
	label("SPDIF Out");
	labelsize(10);
	align(FL_ALIGN_TOP|FL_ALIGN_LEFT);
	adat1 = new Fl_Check_Button(x+15, y+v_step*i++, w-30, v_step, "ADAT1");
	professional = new Fl_Check_Button(x+15, y+v_step*i++, w-30, v_step, "Professional");
	emphasis = new Fl_Check_Button(x+15, y+v_step*i++, w-30, v_step, "Emphasis");
	non_audio = new Fl_Check_Button(x+15, y+v_step*i++, w-30, v_step, "Non-Audio");
	adat1->labelsize(10);
	adat1->callback(spdif_on_adat_cb, (void *)parent());
	professional->labelsize(10);
	professional->callback(spdif_professional_cb, (void *)parent());
	emphasis->labelsize(10);
	emphasis->callback(spdif_emphasis_cb, (void *)parent());
	non_audio->labelsize(10);
	non_audio->callback(spdif_nonaudio_cb, (void *)parent());
	end();	
}

void HC_SpdifOut::setOut(unsigned char val)
{
    if (val != adat1->value()) {
	adat1->value(val);
    }
}

void HC_SpdifOut::setProfessional(unsigned char val)
{
    
    if (val != professional->value()) {
	professional->value(val);
    }
}

void HC_SpdifOut::setEmphasis(unsigned char val)
{
    if (val != emphasis->value()) {
	emphasis->value(val);
    }
}

void HC_SpdifOut::setNonaudio(unsigned char val)
{
    if (val != non_audio->value()) {
	non_audio->value(val);
    }
}

int HC_SpdifOut::handle(int e)
{
    switch (e) {
	case FL_PUSH:
	    lock = 1;
	    break;
	case FL_RELEASE:
	    lock = 0;
	    break;
	default:
	    return Fl_Group::handle(e);
    }
    return Fl_Group::handle(e);
}

