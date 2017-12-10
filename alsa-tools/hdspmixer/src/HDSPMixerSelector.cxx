/*
 *   HDSPMixer
 *    
 *   Copyright (C) 2003 Thomas Charbonnel (thomas@undata.org)
 *    
 *   Copyright (C) 2011 Adrian Knoth (adi@drcomp.erfurt.thur.de)
 *                      Fredrik Lingvall (fredrik.lingvall@gmail.com)
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
#include "HDSPMixerSelector.h"

static char const *destinations_madi_ss[32] = {
  "1+2", "3+4", "5+6", "7+8",
  "9+10", "11+12", "13+14", "15+16",
  "17+18", "19+20", "21+22", "23+24",
  "25+26", "27+28", "29+30", "31+32",
  "33+34", "35+36", "37+38", "39+40",
  "41+42", "43+44", "45+46", "47+48",
  "49+50", "51+52", "53+54", "55+56",
  "57+58", "59+60", "61+62", "63+64"
};

static char const *destinations_madi_ds[16] = {
  "1+2", "3+4", "5+6", "7+8",
  "9+10", "11+12", "13+14", "15+16",
  "17+18", "19+20", "21+22", "23+24",
  "25+26", "27+28", "29+30", "31+32"
};

static char const *destinations_madi_qs[8] = {
  "1+2", "3+4", "5+6", "7+8",
  "9+10", "11+12", "13+14", "15+16"
};

static char const *destinations_aes32[8] = {
  "AES 1+2", "AES 3+4", "AES 5+6", "AES 7+8",
  "AES 9+10", "AES 11+12", "AES 13+14", "AES 15+16",
};

static char const *destinations_raydat_ss[18] = {

  "A1 1+2", "A1 3+4", "A1 5+6", "A1 7+8",
  "A2 1+2", "A2 3+4", "A2 5+6", "A2 7+8",
  "A3 1+2", "A3 3+4", "A3 5+6", "A3 7+8",
  "A4 1+2", "A4 3+4", "A4 5+6", "A4 7+8",
  "AES",
  "SPDIF"
};

static char const *destinations_raydat_ds[10] = {
  "A1 1+2", "A1 3+4",
  "A2 1+2", "A2 3+4",
  "A3 1+2", "A3 3+4",
  "A4 1+2", "A4 3+4",
  "AES",
  "SPDIF"
};

static char const *destinations_raydat_qs[6] = {
  "A1 1+2",
  "A2 1+2",
  "A3 1+2",
  "A4 1+2",
  "AES",
  "SPDIF"
};


static char const *destinations_aio_ss[10] = {
  "AN 1+2",
  "AES",
  "SPDIF",
  "A 1+2", "A 3+4", "A 5+6", "A 7+8",
  "Phones",
  "AEB 1+2",
  "AEB 3+4"
};

static char const *destinations_aio_ds[8] = {
  "AN 1+2", 
  "AES",
  "SPDIF",
  "A 1+2", "A 3+4",
  "Phones",
  "AEB 1+2",
  "AEB 3+4"
};

static char const *destinations_aio_qs[7] = {
  "AN 1+2",
  "AES",
  "SPDIF",
  "A 1+2",
  "Phones",
  "AEB 1+2",
  "AEB 3+4"
};

static char const *destinations_mf_ss[10] = {
  "AN 1+2", "AN 3+4", "AN 5+6", "AN 7+8",
  "A 1+2", "A 3+4", "A 5+6", "A 7+8",
  "SPDIF", "Analog"
};

static char const *destinations_mf_ds[8] = {
  "AN 1+2", "AN 3+4", "AN 5+6", "AN 7+8",
  "A 1+2", "A 3+4",
  "SPDIF", "Analog"
};

static char const *destinations_df_ss[14] = {
  "A1 1+2", "A1 3+4", "A1 5+6", "A1 7+8",
  "A2 1+2", "A2 3+4", "A2 5+6", "A2 7+8",
  "A3 1+2", "A3 3+4", "A3 5+6", "A3 7+8",
  "SPDIF", "Analog"
};

static char const *destinations_df_ds[8] = {
  "A1 1+2", "A1 3+4",
  "A2 1+2", "A2 3+4",
  "A3 1+2", "A3 3+4",
  "SPDIF", "Analog"
};

static char const *destinations_rpm[3] = {
  "Main", "Mon", "Phones"
};

static char const *destinations_h9652_ss[13] = {
  "A1 1+2", "A1 3+4", "A1 5+6", "A1 7+8",
  "A2 1+2", "A2 3+4", "A2 5+6", "A2 7+8",
  "A3 1+2", "A3 3+4", "A3 5+6", "A3 7+8",
  "SPDIF"
};

static char const *destinations_h9652_ds[7] = {
  "A1 1+2", "A1 3+4",
  "A2 1+2", "A2 3+4",
  "A3 1+2", "A3 3+4",
  "SPDIF"
};

static char const *destinations_h9632_ss[8] = {
  "A 1+2", "A 3+4", "A 5+6", "A 7+8",
  "SPDIF", "AN 1+2", "AN 3+4", "AN 5+6"
};

static char const *destinations_h9632_ds[6] = {
  "A 1+2", "A 3+4",
  "SPDIF", "AN 1+2", "AN 3+4", "AN 5+6"    
};

static char const *destinations_h9632_qs[4] = {
  "SPDIF", "AN 1+2", "AN 3+4", "AN 5+6"    
};

HDSPMixerSelector::HDSPMixerSelector(int x, int y, int w, int h):Fl_Menu_(x, y, w, h)
{
    max_dest = 0;
    selected = 0;
    basew = (HDSPMixerWindow *)window();
    textfont(FL_HELVETICA);
    textsize(8);
    textcolor(FL_FOREGROUND_COLOR);
    setLabels();
}

void HDSPMixerSelector::draw() {
    fl_color(FL_WHITE);
    fl_font(FL_HELVETICA, 8);
    fl_draw((char *)mvalue()->label(), x(), y(), w(), h(), FL_ALIGN_CENTER);
}

int HDSPMixerSelector::handle(int e) {
    const Fl_Menu_Item *item;
    switch(e) {
	case FL_PUSH:
	    for (int i = 0; i < max_dest; i++) {
		if (((HDSPMixerIOMixer *)parent())->fader->pos[i] != 0) {
		    mode(i, FL_MENU_TOGGLE|FL_MENU_VALUE);
		} else {
		    mode(i, FL_MENU_TOGGLE);
		}
	    }    
	    if ((item = (menu()->popup(x(), y()+h(), 0, 0, this))) != NULL) {
		value(item);
		selected = value();
		if (basew->inputs->buttons->view->submix) {
		    basew->inputs->buttons->view->submix_value = value();
		    for (int i = 0; i < HDSP_MAX_CHANNELS; i++) {
			basew->inputs->strips[i]->targets->value(value());
			basew->inputs->strips[i]->targets->redraw();
			basew->playbacks->strips[i]->targets->value(value());
			basew->playbacks->strips[i]->targets->redraw();
			basew->inputs->strips[i]->fader->dest = value();
			basew->inputs->strips[i]->fader->redraw();
			basew->inputs->strips[i]->fader->sendGain();
			basew->playbacks->strips[i]->fader->dest = value();
			basew->playbacks->strips[i]->fader->redraw();
			basew->playbacks->strips[i]->fader->sendGain();
			basew->inputs->strips[i]->pan->dest = value();
			basew->inputs->strips[i]->pan->redraw();
			basew->playbacks->strips[i]->pan->dest = value();
			basew->playbacks->strips[i]->pan->redraw();
		    }
		} else {
		    ((HDSPMixerIOMixer *)parent())->fader->dest = value();
		    ((HDSPMixerIOMixer *)parent())->fader->redraw();
		    ((HDSPMixerIOMixer *)parent())->pan->dest = value();
		    ((HDSPMixerIOMixer *)parent())->pan->redraw();
		    ((HDSPMixerIOMixer *)parent())->fader->sendGain();
		}
		redraw();
	    }
	    basew->checkState();
	    return 1;
	default:
	    return Fl_Menu_::handle(e);
    }
}

void HDSPMixerSelector::setLabels()
{
    int type;
    hdsp_9632_aeb_t *aeb;
    int sm;
    clear();
    type = basew->cards[basew->current_card]->type;
    aeb = &basew->cards[basew->current_card]->h9632_aeb;
    sm = basew->cards[basew->current_card]->speed_mode;
    if (type == Multiface) {
	switch (sm) {
	case 0:
	    max_dest = 10;
	    destinations = destinations_mf_ss;
	    break;
	case 1:
	    max_dest = 8;
	    destinations = destinations_mf_ds;
	    break;
	case 2:
	    /* should never happen */
	    break;
	}
    } else if (type == Digiface) {
	switch (sm) {
	case 0:
	    max_dest = 14;
	    destinations = destinations_df_ss;
	    break;
	case 1:
	    max_dest = 8;
	    destinations = destinations_df_ds;
	    break;
	case 2:
	    /* should never happen */
	    break;
	}
    } else if (type == RPM) {
        max_dest = 3;
        destinations = destinations_rpm;
    } else if (type == H9652) {
	switch (sm) {
	case 0:
	    max_dest = 13;
	    destinations = destinations_h9652_ss;
	    break;
	case 1:
	    max_dest = 7;
	    destinations = destinations_h9652_ds;
	    break;
	case 2:
	    /* should never happen */
	    break;
	}
    } else if (type == H9632) {
	switch (sm) {
	case 0:
	    max_dest = 6 + (aeb->aebo ? 2 : 0);
	    destinations = destinations_h9632_ss;
	    break;
	case 1:
	    max_dest = 4 + (aeb->aebo ? 2 : 0);
	    destinations = destinations_h9632_ds;
	    break;
	case 2:
	    max_dest = 2 + (aeb->aebo ? 2 : 0);
	    destinations = destinations_h9632_qs;
	    break;
	}
    } else if (HDSPeMADI == type) {
	switch (sm) {
	case 0:
	  max_dest = 32;
	  destinations = destinations_madi_ss;
	  break;
	case 1:
	  max_dest = 16;
	  destinations = destinations_madi_ds;
	  break;
	case 2:
	  max_dest = 8;
	  destinations = destinations_madi_qs;
	  break;
	}
    } else if (HDSP_AES == type) {
      max_dest = 8;
      destinations = destinations_aes32;
    } else if (HDSPeAIO == type) {
	switch (sm) {
	case 0:
	  max_dest = 10;
	  destinations = destinations_aio_ss;
	  break;
	case 1:
	  max_dest = 8;
	  destinations = destinations_aio_ds;
	  break;
	case 2:
	  max_dest = 7;
	  destinations = destinations_aio_qs;
	  break;
	}
    } else if (HDSPeRayDAT == type) {
	switch (sm) {
	case 0:
	  max_dest = 18;
	  destinations = destinations_raydat_ss;
	  break;
	case 1:
	  max_dest = 10;
	  destinations = destinations_raydat_ds;
	  break;
	case 2:
	  max_dest = 6;
	  destinations = destinations_raydat_qs;
	  break;
	}
     
    }

    
    for (int i = 0; i < max_dest; ++i) {
	add(destinations[i], 0, 0, 0, FL_MENU_TOGGLE);
    }
    value(0);
}

