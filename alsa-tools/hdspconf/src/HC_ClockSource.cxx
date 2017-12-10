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
#include "HC_ClockSource.h"

extern const char *freqs[10];

void clock_source_cb(Fl_Widget *w, void *arg)
{
	int err;
	char card_name[6];
	snd_ctl_elem_value_t *ctl;
	snd_ctl_elem_id_t *id;
	snd_ctl_t *handle;
	int src = 0;
	HC_ClockSource *cs = (HC_ClockSource *)arg;
	HC_CardPane *pane = (HC_CardPane *)(cs->parent());
	Fl_Round_Button *source = (Fl_Round_Button *)w;
	if (source == cs->autosync) {
		src = 0;
	} else if (source == cs->khz32) {
		src = 1;
	} else if (source == cs->khz44_1) {
		src = 2;
	} else if (source == cs->khz48) {
		src = 3;
	} else if (source == cs->khz64) {
		src = 4;
	} else if (source == cs->khz88_2) {
		src = 5;
	} else if (source == cs->khz96) {
		src = 6;
	} else if (source == cs->khz128) {
		src = 7;
	} else if (source == cs->khz176_4) {
		src = 8;
	} else if (source == cs->khz192) {
		src = 9;
	}

	snprintf(card_name, 6, "hw:%i", pane->alsa_index);
	snd_ctl_elem_value_alloca(&ctl);
	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_id_set_name(id, "Sample Clock Source");
	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_index(id, 0);
	snd_ctl_elem_value_set_id(ctl, id);
	snd_ctl_elem_value_set_enumerated(ctl, 0, src);
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

HC_ClockSource::HC_ClockSource(int x, int y, int w, int h):Fl_Group(x, y, w, h, "Sample Clock Source")
{
	int i = 0;
	box(FL_ENGRAVED_FRAME);
	label("Sample Clock Source");
	labelsize(10);
	align(FL_ALIGN_TOP|FL_ALIGN_LEFT);
	autosync = new Fl_Round_Button(x+10, y+V_STEP*i++, w-20, V_STEP, "AutoSync");
	autosync->callback(clock_source_cb, (void *)this);
	khz32 = new Fl_Round_Button(x+10, y+V_STEP*i++, w-20, V_STEP, freqs[0]);
	khz32->callback(clock_source_cb, (void *)this);
	khz44_1 = new Fl_Round_Button(x+10, y+V_STEP*i++, w-20, V_STEP, freqs[1]);
	khz44_1->callback(clock_source_cb, (void *)this);
	khz48 = new Fl_Round_Button(x+10, y+V_STEP*i++, w-20, V_STEP, freqs[2]);
	khz48->callback(clock_source_cb, (void *)this);
	khz64 = new Fl_Round_Button(x+10, y+V_STEP*i++, w-20, V_STEP, freqs[3]);
	khz64->callback(clock_source_cb, (void *)this);
	khz88_2 = new Fl_Round_Button(x+10, y+V_STEP*i++, w-20, V_STEP, freqs[4]);
	khz88_2->callback(clock_source_cb, (void *)this);
	khz96 = new Fl_Round_Button(x+10, y+V_STEP*i++, w-20, V_STEP, freqs[5]);
	khz96->callback(clock_source_cb, (void *)this);
	if (((HC_CardPane *)parent())->type == H9632) {
	    khz128 = new Fl_Round_Button(x+10, y+V_STEP*i++, w-20, V_STEP, freqs[7]);
	    khz128->callback(clock_source_cb, (void *)this);
	    khz128->labelsize(10);
	    khz128->type(FL_RADIO_BUTTON);
	    khz176_4 = new Fl_Round_Button(x+10, y+V_STEP*i++, w-20, V_STEP, freqs[8]);
	    khz176_4->callback(clock_source_cb, (void *)this);
	    khz176_4->labelsize(10);
	    khz176_4->type(FL_RADIO_BUTTON);
	    khz192 = new Fl_Round_Button(x+10, y+V_STEP*i++, w-20, V_STEP, freqs[9]);
	    khz192->callback(clock_source_cb, (void *)this);
	    khz192->labelsize(10);
	    khz192->type(FL_RADIO_BUTTON);
	}
	autosync->labelsize(10);
	autosync->type(FL_RADIO_BUTTON);
	khz32->labelsize(10);
	khz32->type(FL_RADIO_BUTTON);
	khz44_1->labelsize(10);
	khz44_1->type(FL_RADIO_BUTTON);
	khz48->labelsize(10);
	khz48->type(FL_RADIO_BUTTON);
	khz64->labelsize(10);
	khz64->type(FL_RADIO_BUTTON);
	khz88_2->labelsize(10);
	khz88_2->type(FL_RADIO_BUTTON);
	khz96->labelsize(10);
	khz96->type(FL_RADIO_BUTTON);
	end();
}

void HC_ClockSource::setSource(unsigned char s)
{
    if (s < children()) {
	if (((Fl_Round_Button *)child(s))->value() != 1)
	    ((Fl_Round_Button *)child(s))->setonly();
    }
}

