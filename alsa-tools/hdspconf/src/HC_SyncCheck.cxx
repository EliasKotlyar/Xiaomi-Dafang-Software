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
#include "HC_SyncCheck.h"

extern const char *lock_status[3];

HC_SyncCheck::HC_SyncCheck(int x, int y, int w, int h):Fl_Widget(x, y, w, h, "SyncCheck")
{
	adat1_lock_status = -1;
	adat2_lock_status = -1;
	adat3_lock_status = -1;
	wordclock_lock_status = -1;
	adatsync_lock_status = -1;
	spdif_lock_status = -1;
	if (((HC_CardPane *)parent())->type == Multiface || ((HC_CardPane *)parent())->type == H9632) {
		adat_name = "ADAT In";
	} else {
		adat_name = "ADAT1 In";
	}
	h_step = (int)(w/2.0f);
	draw_box = Fl::get_boxtype(FL_ENGRAVED_FRAME);
	label("SyncCheck");
	labelsize(10);
	align(FL_ALIGN_TOP|FL_ALIGN_LEFT);
}

void HC_SyncCheck::draw()
{
	int h_pos = 4;
	int i = 0;
	fl_color(FL_BACKGROUND_COLOR);
	fl_rectf(x(), y(), w(), h());
	draw_box(x(), y(), w(), h(), FL_WHITE);
	fl_color(FL_FOREGROUND_COLOR);
	fl_font(FL_HELVETICA, 10);
	fl_draw(adat_name, x()+h_pos, y()+V_STEP*i, h_step, V_STEP, FL_ALIGN_LEFT);
	fl_draw(lock_status[adat1_lock_status], x()+h_pos+h_step, y()+V_STEP*i++, h_step-h_pos, V_STEP, FL_ALIGN_CENTER);
	if (((HC_CardPane *)parent())->type == Digiface || ((HC_CardPane *)parent())->type == H9652) {
		fl_draw("ADAT2 In", x()+h_pos, y()+V_STEP*i, h_step, V_STEP, FL_ALIGN_LEFT);
		fl_draw(lock_status[adat2_lock_status], x()+h_pos+h_step, y()+V_STEP*i++, h_step-h_pos, V_STEP, FL_ALIGN_CENTER);
		fl_draw("ADAT3 In", x()+h_pos, y()+V_STEP*i, h_step, V_STEP, FL_ALIGN_LEFT);
		fl_draw(lock_status[adat3_lock_status], x()+h_pos+h_step, y()+V_STEP*i++, h_step-h_pos, V_STEP, FL_ALIGN_CENTER);
	}
	fl_draw("SPDIF In", x()+h_pos, y()+V_STEP*i, h_step, V_STEP, FL_ALIGN_LEFT);
	fl_draw(lock_status[spdif_lock_status], x()+h_pos+h_step, y()+V_STEP*i++, h_step-h_pos, V_STEP, FL_ALIGN_CENTER);
	fl_draw("WordClock", x()+h_pos, y()+V_STEP*i, h_step, V_STEP, FL_ALIGN_LEFT);
	fl_draw(lock_status[wordclock_lock_status], x()+h_pos+h_step, y()+V_STEP*i++, h_step-h_pos, V_STEP, FL_ALIGN_CENTER);
	if (((HC_CardPane *)parent())->type != H9632) {
	    fl_draw("ADAT Sync", x()+h_pos, y()+V_STEP*i, h_step, V_STEP, FL_ALIGN_LEFT);
	    fl_draw(lock_status[adatsync_lock_status], x()+h_pos+h_step, y()+V_STEP*i, h_step-h_pos, V_STEP, FL_ALIGN_CENTER);
	}
}

void HC_SyncCheck::setSpdifStatus(unsigned char s)
{
	if (s == spdif_lock_status) return;
	if (s > 2) spdif_lock_status = 2;
	else spdif_lock_status = s;
	redraw();
	return;
}

void HC_SyncCheck::setWCStatus(unsigned char s)
{
	if (s == wordclock_lock_status) return;
	if (s > 2) wordclock_lock_status = 2;
	else wordclock_lock_status = s;
	redraw();
	return;
}

void HC_SyncCheck::setAdat1Status(unsigned char s)
{
	if (s == adat1_lock_status) return;
	if (s > 2) adat1_lock_status = 2;
	else adat1_lock_status = s;
	redraw();
	return;
}

void HC_SyncCheck::setAdat2Status(unsigned char s)
{
	if (s == adat2_lock_status) return;
	if (s > 2) adat2_lock_status = 2;
	else adat2_lock_status = s;
	redraw();
	return;
}

void HC_SyncCheck::setAdat3Status(unsigned char s)
{
	if (s == adat3_lock_status) return;
	if (s > 2) adat3_lock_status = 2;
	else adat3_lock_status = s;
	redraw();
	return;
}

void HC_SyncCheck::setAdatSyncStatus(unsigned char s)
{
	if (s == adatsync_lock_status) return;
	if (s > 2) adatsync_lock_status = 2;
	else adatsync_lock_status = s;
	redraw();
	return;
}

