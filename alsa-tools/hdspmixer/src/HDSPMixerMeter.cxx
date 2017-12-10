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
#include "HDSPMixerMeter.h"

HDSPMixerMeter::HDSPMixerMeter(int x, int y, bool not_output, HDSPMixerPeak *p):Fl_Widget(x, y, 8, METER_HEIGHT)
{
    basew = (HDSPMixerWindow *)window();
    count = 0;
    fine_draw = 0;
    peaktext = p;
    new_peak_height = peak_height = 0;
    new_rms_height = rms_height = 0;
    fast_peak_level = 1000.0;
    slow_peak_level = 1000.0;
    max_level = 1000.0;
    /* this is no more as simple :
       H9652 cards do have both peak and rms data for outputs
    */
    peak_rms = not_output;
}

void HDSPMixerMeter::draw()
{
    if (!fine_draw) {
	fl_push_clip(x(), y()+(METER_HEIGHT-new_rms_height), w(), new_rms_height);
	fl_draw_pixmap(level_xpm, x(), y());
	fl_pop_clip();
    } else {
	if (new_rms_height > rms_height) {
	    fl_push_clip(x(), y()+(METER_HEIGHT-new_rms_height), w(), new_rms_height-rms_height);
	    fl_draw_pixmap(level_xpm, x(), y());
	    fl_pop_clip();
	} else if (new_rms_height < rms_height) {
	    fl_push_clip(x(), y()+(METER_HEIGHT-rms_height), w(), rms_height-new_rms_height);
	    if (peak_rms) {
		fl_draw_pixmap(iomixer_xpm, x()-20, y()-59);
	    } else {
		fl_draw_pixmap(output_xpm, x()-20, y()-27);
	    }
	    fl_pop_clip();    
	}
    }
    rms_height = new_rms_height;
    
    if ((new_peak_height != peak_height || !fine_draw) && (peak_rms || basew->cards[basew->current_card]->type == H9652)) {
	if ((rms_height <= (peak_height - PEAK_HEIGHT)) || rms_height == 0) { 
	    fl_push_clip(x(), y()+(METER_HEIGHT - peak_height), w(), PEAK_HEIGHT+1);
	    if (peak_rms) {
		fl_draw_pixmap(iomixer_xpm, x()-20, y()-59);
	    } else {
		fl_draw_pixmap(output_xpm, x()-20, y()-27);
	    }
	    fl_pop_clip();
	} else if (rms_height >= peak_height) {
	    fl_push_clip(x(), y()+(METER_HEIGHT - peak_height), w(), PEAK_HEIGHT+1);
	    fl_draw_pixmap(level_xpm, x(), y());
	    fl_pop_clip();
	} else {
	    fl_push_clip(x(), y()+(METER_HEIGHT - peak_height), w(), peak_height - rms_height);
	    if (peak_rms) {
		fl_draw_pixmap(iomixer_xpm, x()-20, y()-59);
	    } else {
		fl_draw_pixmap(output_xpm, x()-20, y()-27);
	    }
	    fl_pop_clip();
	    fl_push_clip(x(), y()+(METER_HEIGHT - peak_height)+(peak_height - rms_height), w(), PEAK_HEIGHT - (peak_height - rms_height));
	    fl_draw_pixmap(level_xpm, x(), y());
	    fl_pop_clip();	    
	}
	
	if (new_peak_height > 0) {
	    fl_push_clip(x(), y()+(METER_HEIGHT-new_peak_height), w(), ((new_peak_height > METER_HEIGHT) ? METER_HEIGHT : new_peak_height));
	    fl_draw_pixmap(peak_xpm, x(), y()+(METER_HEIGHT-new_peak_height));
	    fl_pop_clip();	    
	}	
	peak_height = new_peak_height;
    }
    fine_draw = 0;
}

int HDSPMixerMeter::logToHeight(double db)
{
    double x;
    int h;
    double max_db;
    
    max_db = basew->setup->level_val ? 60.0 : 40.0;
    
    if (db < max_db) {
	x = (db /max_db) * double(METER_HEIGHT);
	h = METER_HEIGHT - (int)x;
    } else {
	h = 0;
    }
    if (h < 0) h = 0;
    return h;
}

void HDSPMixerMeter::update(int peak, int overs, int64 rms)
{
    double fp, fr, db;
    int over = 0;
    
    if (!visible()) return;
    

    count++;
    
    if (overs >= basew->setup->over_val) {
	over = 1;
    }
    
    peak >>= 8;
    peak &= 0x7FFFFF;

    if (peak != 0) {
	fp = (double)peak / (double)(0x7FFFFF);
	db = -20 * log10(fp);
    } else {
	db = 1000.0;
    }

    switch (basew->setup->rate_val) {
	case 0:
	    fast_peak_level += 0.030 * 8.3;
	    break;
	case 1:
	    fast_peak_level += 0.030 * 15;
	    break;
	case 2:
	    fast_peak_level += 0.030 * 23.7;
	    break;
    }
	
    if (fast_peak_level > 138.47) fast_peak_level = 1000.0;
    
    if (db > fast_peak_level)
	db = fast_peak_level;
    else 
	fast_peak_level = db;

    new_peak_height = logToHeight(db);
    
    fr = (double)rms;
    fr /= ((double)(1125899638407184.0)*(double)(8191.0));
    fr = sqrt(fr);
    
    if (!peak_rms && (basew->cards[basew->current_card]->type != H9652)) {
	new_rms_height = new_peak_height;
    } else {
	fr = -20 * log10(fr);
	if (basew->setup->rmsplus3_val) {
	    fr -= 3.010299957;
	    if (fr < 0.0) fr = 0.0;
	}
	if (basew->setup->numbers_val == 0) db = fr;
	new_rms_height = logToHeight(fr);
    }


    if (new_rms_height != rms_height || (new_peak_height != peak_height && (peak_rms || basew->cards[basew->current_card]->type == H9652))) {
	/* FIXME: may not be SMP safe */
	redraw();
    }    
    
    if (db < max_level) max_level = db;

    if (count > 15 || over) {
	count = 0;
	if (max_level != slow_peak_level) {
	    peaktext->update(max_level, over);
	    slow_peak_level = max_level;
	}
	max_level = 1000.0;
    }
}
