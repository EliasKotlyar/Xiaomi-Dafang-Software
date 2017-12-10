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
#include "HC_Phones.h"

void phones_cb(Fl_Widget *w, void *arg)
{
    int err;
    char card_name[6];
    snd_ctl_elem_value_t *ctl;
    snd_ctl_elem_id_t *id;
    snd_ctl_t *handle;
    int gain = 0;
    Fl_Round_Button *source = (Fl_Round_Button *)w;
    HC_Phones *ph = (HC_Phones *)arg;
    HC_CardPane *pane = (HC_CardPane *)ph->parent();
    if (source == ph->zero_db) {
	gain = 0;
    } else if (source == ph->minus_six_db) {
	gain = 1;
    } else if (source == ph->minus_twelve_db) {
	gain = 2;
    }
    snprintf(card_name, 6, "hw:%i", pane->alsa_index);
    snd_ctl_elem_value_alloca(&ctl);
    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_id_set_name(id, "Phones Gain");
    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_index(id, 0);
    snd_ctl_elem_value_set_id(ctl, id);
    snd_ctl_elem_value_set_enumerated(ctl, 0, gain);
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

HC_Phones::HC_Phones(int x, int y, int w, int h):Fl_Group(x, y, w, h, "Phones")
{
	int i = 0;
	int v_step = (int)(h/3.0f);
	box(FL_ENGRAVED_FRAME);;
	label("Phones");
	labelsize(10);
	align(FL_ALIGN_TOP|FL_ALIGN_LEFT);
	zero_db = new Fl_Round_Button(x+15, y+v_step*i++, w-30, v_step, "Hi Gain");
	minus_six_db = new Fl_Round_Button(x+15, y+v_step*i++, w-30, v_step, "+4 dBu");
	minus_twelve_db = new Fl_Round_Button(x+15, y+v_step*i++, w-30, v_step, "-10 dBV");
	zero_db->labelsize(10);
	zero_db->type(FL_RADIO_BUTTON);
	zero_db->callback(phones_cb, (void *)this);
	minus_six_db->labelsize(10);
	minus_six_db->type(FL_RADIO_BUTTON);
	minus_six_db->callback(phones_cb, (void *)this);
	minus_twelve_db->labelsize(10);
	minus_twelve_db->type(FL_RADIO_BUTTON);
	minus_twelve_db->callback(phones_cb, (void *)this);
	end();	
}

void HC_Phones::setPhones(unsigned char i)
{
	if (i < children()) {
	    if (((Fl_Round_Button *)child(i))->value() != 1)
		((Fl_Round_Button *)child(i))->setonly();
	}
}

