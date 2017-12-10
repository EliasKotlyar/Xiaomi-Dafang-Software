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
#include "HDSPMixerCardSelector.h"

HDSPMixerCardSelector::HDSPMixerCardSelector(int x, int y, int w, int h, int cardnum):Fl_Widget(x, y, 61, 13)
{
    basew = (HDSPMixerWindow *)window();
    card = cardnum;
}


void HDSPMixerCardSelector::draw() 
{
    switch (card) {
	case 1:
	    fl_draw_pixmap(b_card1_xpm, x(), y());
	    return;
	case 2:
	    fl_draw_pixmap(b_card2_xpm, x()+24, y());
	    return;
	case 3:
	    fl_draw_pixmap(b_card3_xpm, x()+48, y());
	    return;
	default:
	    return;
    }    
}

void HDSPMixerCardSelector::ActivateCard (int i)
{
  card = i + 1;
  basew->stashPreset(); /* save current mixer state */
  basew->current_card = i;
  basew->cards[i]->setMode (basew->cards[i]->getSpeed ());
  basew->setTitleWithFilename();
  basew->unstashPreset(); /* restore previous mixer state */
  redraw ();
}

int HDSPMixerCardSelector::handle(int e)
{
    int xpos = Fl::event_x()-x();
    switch (e) {
	case FL_PUSH:
	    if (xpos < 13 && card != 1)
	      ActivateCard (0);
	    else if (xpos >= 24 && xpos < 37 && card != 2 && basew->cards[1] != NULL)
	      ActivateCard (1);
	    else if (xpos >= 48 && card != 3 && basew->cards[2] != NULL)
	      ActivateCard (2);
	    return 1;
	default:
	    return Fl_Widget::handle(e);
    }    
}
