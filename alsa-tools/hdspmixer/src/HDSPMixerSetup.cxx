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
#include "HDSPMixerSetup.h"

static void rate_cb(Fl_Widget *widget, void *arg)
{
    HDSPMixerWindow *w = (HDSPMixerWindow *)arg;
    const Fl_Menu_Item *i = ((Fl_Menu_ *)widget)->mvalue();
    if (!strncmp("s", i->label(), 1)) {
	w->setup->rate_val = 0;
    } else if (!strncmp("m", i->label(), 1)) {
	w->setup->rate_val = 1;
    } else {
	w->setup->rate_val = 2;
    }
    w->checkState();
}

static void ok_cb(Fl_Widget *widget, void *arg)
{
    HDSPMixerSetup *s = (HDSPMixerSetup *)arg;
    s->hide();
}


static void peak_cb(Fl_Widget *widget, void *arg)
{
    HDSPMixerWindow *w = (HDSPMixerWindow *)arg;
    w->setup->numbers_val = 1;
    w->checkState();
}

static void rms_cb(Fl_Widget *widget, void *arg)
{
    HDSPMixerWindow *w = (HDSPMixerWindow *)arg;
    w->setup->numbers_val = 0;
    w->checkState();
}


static void fourty_cb(Fl_Widget *widget, void *arg)
{
    HDSPMixerWindow *w = (HDSPMixerWindow *)arg;
    w->setup->level_val = 0;
    w->checkState();
}

static void sixty_cb(Fl_Widget *widget, void *arg)
{
    HDSPMixerWindow *w = (HDSPMixerWindow *)arg;
    w->setup->level_val = 1;
    w->checkState();
}

static void over_cb(Fl_Widget *widget, void *arg)
{
    HDSPMixerWindow *w = (HDSPMixerWindow *)arg;
    w->setup->over_val = (int)w->setup->over->value();
    w->checkState();
}


static void rmsplus3_cb(Fl_Widget *widget, void *arg)
{
    HDSPMixerWindow *w = (HDSPMixerWindow *)arg;
    if (w->setup->rmsplus3->value()) {
	w->setup->rmsplus3_val = 1;
    } else {
	w->setup->rmsplus3_val = 0;
    }
    w->checkState();
}


HDSPMixerSetup::HDSPMixerSetup(int w, int h, char const *label, HDSPMixerWindow *win):Fl_Double_Window(w, h, label)
{
    basew = win;
    plm = new Fl_Group(10, 25, 380, 60, "Peak Level Meters");    
    plm->labelfont(FL_HELVETICA);
    plm->labelsize(12);
    plm->align(FL_ALIGN_TOP_LEFT);
    plm->box(FL_ENGRAVED_FRAME);
    over = new Fl_Counter(30, 50, 50, 20, "FS samples for OVR  ");
    over->callback((Fl_Callback *)over_cb, (void *)basew);
    over->type(FL_SIMPLE_COUNTER);
    over->bounds(1, 15);
    over->step(1);
    over->value(3);
    over_val = 3;
    over->labelfont(FL_HELVETICA);
    over->labelsize(12);
    over->align(FL_ALIGN_TOP_LEFT);
    rate = new Fl_Choice(230, 50, 80, 20, "Release Rate  ");
    rate->align(FL_ALIGN_TOP_LEFT);
    rate->labelfont(FL_HELVETICA);
    rate->labelsize(12);
    rate->add("slow", 0, (Fl_Callback *)rate_cb, (void *)basew);
    rate->add("medium", 0, (Fl_Callback *)rate_cb, (void *)basew);
    rate->add("high", 0, (Fl_Callback *)rate_cb, (void *)basew);
    rate->value(1);
    rate_val = 1;
    plm->end();
    numbers = new Fl_Group(10, 110, 180, 60, "Numbers");
    numbers->box(FL_ENGRAVED_FRAME);
    numbers->labelfont(FL_HELVETICA);
    numbers->labelsize(12);
    numbers->align(FL_ALIGN_TOP_LEFT);    
    rms = new Fl_Round_Button(30, 120, 60, 20, "RMS");
    rms->labelfont(FL_HELVETICA);
    rms->labelsize(12);
    rms->callback((Fl_Callback *)rms_cb, (void *)basew);
    rms->set();
    numbers_val = 0;
    rms->type(FL_RADIO_BUTTON);
    peak = new Fl_Round_Button(30, 140, 60, 20, "Peak");
    peak->labelfont(FL_HELVETICA);
    peak->labelsize(12);
    peak->type(FL_RADIO_BUTTON);
    peak->callback((Fl_Callback *)peak_cb, (void *)basew);
    numbers->end();
    level = new Fl_Group(210, 110, 180, 60, "Minimum Level");
    level->box(FL_ENGRAVED_FRAME);
    level->labelfont(FL_HELVETICA);
    level->labelsize(12);
    level->align(FL_ALIGN_TOP_LEFT);
    fourty = new Fl_Round_Button(230, 120, 60, 20, "-40 dB");
    fourty->labelfont(FL_HELVETICA);
    fourty->labelsize(12);
    fourty->type(FL_RADIO_BUTTON);
    fourty->set();
    level_val = 0;
    fourty->callback((Fl_Callback *)fourty_cb, (void *)basew);
    sixty = new Fl_Round_Button(230, 140, 60, 20, "-60 dB");
    sixty->labelfont(FL_HELVETICA);
    sixty->labelsize(12);
    sixty->type(FL_RADIO_BUTTON);
    sixty->callback((Fl_Callback *)sixty_cb, (void *)basew);
    level->end();
    rmsplus3grp = new Fl_Group(10, 190, 180, 60);
    rmsplus3grp->box(FL_ENGRAVED_FRAME);
    rmsplus3 = new Fl_Check_Button(30, 210, 100, 20, "RMS +3dB");
    rmsplus3->labelfont(FL_HELVETICA);
    rmsplus3->labelsize(12);
    rmsplus3->callback((Fl_Callback *)rmsplus3_cb, (void *)basew);
    rmsplus3_val = 0;
    rmsplus3grp->end();
    ok = new Fl_Return_Button(270, 210, 60, 20, "OK");
    ok->callback((Fl_Callback *)ok_cb, (void *)this);
    end();
#ifndef NON_MODAL_SETUP
    set_modal();
#endif
}

void HDSPMixerSetup::updateValues()
{
    rmsplus3->value(rmsplus3_val);
    if (level_val) {
	sixty->setonly();
    } else {
	fourty->setonly();
    }
    if (numbers_val) {
	peak->setonly();
    } else {
	rms->setonly();
    }
    over->value(over_val);
    rate->value(rate_val);    
#ifdef NON_MODAL_SETUP
    if (shown()) {
	redraw();
    }
#endif
}

