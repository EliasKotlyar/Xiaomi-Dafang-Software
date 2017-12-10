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
#include "HC_BreakoutCable.h"

static void setXlrStatus(const char *ctl_name, int val, int card_index)
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

void xlr_cb(Fl_Widget *w, void *arg)
{
    setXlrStatus("XLR Breakout Cable", ((Fl_Check_Button *)w)->value(), ((HC_CardPane *)arg)->alsa_index);
}

HC_BreakoutCable::HC_BreakoutCable(int x, int y, int w, int h):Fl_Group(x, y, w, h, "Breakout Cable")
{
	lock = 0;
	box(FL_ENGRAVED_FRAME);
	label("Breakout Cable");
	labelsize(10);
	align(FL_ALIGN_TOP|FL_ALIGN_LEFT);
	xlr = new Fl_Check_Button(x+15, y, w-30, 20, "XLR");
	xlr->labelsize(10);
	xlr->callback(xlr_cb, (void *)parent());
	end();
}

void HC_BreakoutCable::setXlr(unsigned char val)
{
    if (val != xlr->value()) {
	xlr->value(val);
    }
}

int HC_BreakoutCable::handle(int e)
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

