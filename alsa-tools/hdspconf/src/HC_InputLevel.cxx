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
#include "HC_InputLevel.h"

void input_level_cb(Fl_Widget *w, void *arg)
{
    int err;
    char card_name[6];
    snd_ctl_elem_value_t *ctl;
    snd_ctl_elem_id_t *id;
    snd_ctl_t *handle;
    int gain = 0;
    Fl_Round_Button *source = (Fl_Round_Button *)w;
    HC_InputLevel *il = (HC_InputLevel *)arg;
    HC_CardPane *pane = (HC_CardPane *)il->parent();
    if (source == il->lo_gain) {
	gain = 2;
    } else if (source == il->plus_four_dbu) {
	gain = 1;
    } else if (source == il->minus_ten_dbv) {
	gain = 0;
    }
    snprintf(card_name, 6, "hw:%i", pane->alsa_index);
    snd_ctl_elem_value_alloca(&ctl);
    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_id_set_name(id, "AD Gain");
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

HC_InputLevel::HC_InputLevel(int x, int y, int w, int h):Fl_Group(x, y, w, h, "Input Level")
{
	int i = 0;
	int v_step = (int)(h/3.0f);
	box(FL_ENGRAVED_FRAME);;
	label("Input Level");
	labelsize(10);
	align(FL_ALIGN_TOP|FL_ALIGN_LEFT);
	lo_gain = new Fl_Round_Button(x+15, y+v_step*i++, w-30, v_step, "Lo Gain");
	plus_four_dbu = new Fl_Round_Button(x+15, y+v_step*i++, w-30, v_step, "+4 dBu");
	minus_ten_dbv = new Fl_Round_Button(x+15, y+v_step*i++, w-30, v_step, "-10 dBV");
	lo_gain->labelsize(10);
	lo_gain->type(FL_RADIO_BUTTON);
	lo_gain->callback(input_level_cb, (void *)this);
	plus_four_dbu->labelsize(10);
	plus_four_dbu->type(FL_RADIO_BUTTON);
	plus_four_dbu->callback(input_level_cb, (void *)this);
	minus_ten_dbv->labelsize(10);
	minus_ten_dbv->type(FL_RADIO_BUTTON);
	minus_ten_dbv->callback(input_level_cb, (void *)this);
	end();	
}

void HC_InputLevel::setInputLevel(unsigned char i)
{
	switch (i) {
	case 0:
	    if (minus_ten_dbv->value() != 1)
		minus_ten_dbv->setonly();
	    break;
	case 1:
	    if (plus_four_dbu->value() != 1)
		plus_four_dbu->setonly();
	    break;
	case 2:
	    if (lo_gain->value() != 1)
		lo_gain->setonly();
	    break;
	}
}

