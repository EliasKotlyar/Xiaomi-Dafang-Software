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
#include "HDSPMixerPresets.h"

static void saving_cb(void *arg)
{
    HDSPMixerWindow *w = (HDSPMixerWindow *)arg;
    if (w->inputs->buttons->presets->presetmask == 0 && w->dirty) {
	w->inputs->buttons->presets->presetmask = (int)pow(2, w->inputs->buttons->presets->preset-1);
    }
    if (w->inputs->buttons->presets->saving) {
	w->inputs->buttons->presets->presetmask = ~(w->inputs->buttons->presets->presetmask); 
	if (w->inputs->buttons->presets->save) {
	    w->inputs->buttons->presets->save = 0;
	 } else {
	    w->inputs->buttons->presets->save = 1;
	}	 
	Fl::add_timeout(0.3, saving_cb, arg);
    } else {
	w->inputs->buttons->presets->save = 0;
	w->inputs->buttons->presets->presetmask = (int)pow(2, w->inputs->buttons->presets->preset-1);
    }
    w->inputs->buttons->presets->redraw();
}

HDSPMixerPresets::HDSPMixerPresets(int x, int y, int w, int h):Fl_Widget(x, y, 61, 52)
{
    basew = (HDSPMixerWindow *)window();
    preset = 1;
    presetmask = PRE1;
    save = 0;
    saving = 0;
}

void HDSPMixerPresets::draw() 
{
    if (presetmask & PRE1) {
	fl_draw_pixmap(b_pre1_xpm, x(), y());	
    }
    if (presetmask & PRE2) {
	fl_draw_pixmap(b_pre2_xpm, x()+16, y());
    }
    if (presetmask & PRE3) {
	fl_draw_pixmap(b_pre3_xpm, x()+32, y());
    }
    if (presetmask & PRE4) {
	fl_draw_pixmap(b_pre4_xpm, x()+48, y());
    }
    if (presetmask & PRE5) {
        fl_draw_pixmap(b_pre5_xpm, x(), y()+20);
    }
    if (presetmask & PRE6) {
	fl_draw_pixmap(b_pre6_xpm, x()+16, y()+20);
    }
    if (presetmask & PRE7) {
	fl_draw_pixmap(b_pre7_xpm, x()+32, y()+20);
    }
    if (presetmask & PRE8) {
        fl_draw_pixmap(b_pre8_xpm, x()+48, y()+20);
    }
    if (save) {
	fl_draw_pixmap(b_save_xpm, x(), y()+39);
    }
}

int HDSPMixerPresets::handle(int e)
{
    int xpos = Fl::event_x()-x();
    int ypos = Fl::event_y()-y();
    switch (e) {
	case FL_PUSH:
	    if (ypos < 13 && xpos < 13) {
		preset_change(1);
	    } else if (xpos > 15 && xpos < 29 && ypos < 13) {
		preset_change(2);
	    } else if (xpos > 31 && xpos < 45 && ypos < 13) {
		preset_change(3);
	    } else if (xpos > 47 && ypos < 13) {
		preset_change(4);
	    } else if (ypos > 19 && ypos < 33 && xpos < 13) {
		preset_change(5);
	    } else if (ypos > 19 && ypos < 33 && xpos > 15 && xpos < 29) {
		preset_change(6);
	    } else if (ypos > 19 && ypos < 33 && xpos > 31 && xpos < 45) {
		preset_change(7);
	    } else if (ypos > 19 && ypos < 33 && xpos > 47) {
		preset_change(8);
	    } else if (xpos < 12 && ypos > 38) {
		if (saving) {
		    saving = 0;
		    save = 0;
		} else {
		    saving = 1;
		    save = 1;
		    Fl::add_timeout(0.3, saving_cb, (void *)basew);
		}
		redraw();
	    }
	    return 1;
	default:
	    return Fl_Widget::handle(e);
    }    
}

void HDSPMixerPresets::save_preset(int prst) {
    int speed = basew->cards[basew->current_card]->speed_mode;
    int card = basew->current_card;
    int p = prst-1;
    basew->dirty = 0;
    for (int i = 0; i < HDSP_MAX_CHANNELS; i++) {
	for (int z = 0; z < HDSP_MAX_DEST; z++) {
	    basew->inputs->strips[i]->data[card][speed][p]->pan_pos[z] = basew->inputs->strips[i]->pan->pos[z];
	    basew->inputs->strips[i]->data[card][speed][p]->fader_pos[z] = basew->inputs->strips[i]->fader->pos[z];
	    basew->playbacks->strips[i]->data[card][speed][p]->pan_pos[z] = basew->playbacks->strips[i]->pan->pos[z];
	    basew->playbacks->strips[i]->data[card][speed][p]->fader_pos[z] = basew->playbacks->strips[i]->fader->pos[z];
	}

	basew->inputs->strips[i]->data[card][speed][p]->mute = basew->inputs->strips[i]->mutesolo->mute;
	basew->inputs->strips[i]->data[card][speed][p]->solo = basew->inputs->strips[i]->mutesolo->solo;
	basew->inputs->strips[i]->data[card][speed][p]->dest = basew->inputs->strips[i]->targets->selected;

	basew->playbacks->strips[i]->data[card][speed][p]->mute = basew->playbacks->strips[i]->mutesolo->mute;
	basew->playbacks->strips[i]->data[card][speed][p]->solo = basew->playbacks->strips[i]->mutesolo->solo;
	basew->playbacks->strips[i]->data[card][speed][p]->dest = basew->playbacks->strips[i]->targets->selected;
	
	basew->outputs->strips[i]->data[card][speed][p]->fader_pos = basew->outputs->strips[i]->fader->pos[0];
    }
    /* Line outs */
    basew->outputs->strips[HDSP_MAX_CHANNELS]->data[card][speed][p]->fader_pos = basew->outputs->strips[HDSP_MAX_CHANNELS]->fader->pos[0];
    basew->outputs->strips[HDSP_MAX_CHANNELS+1]->data[card][speed][p]->fader_pos = basew->outputs->strips[HDSP_MAX_CHANNELS+1]->fader->pos[0];
    
    /* Global settings */    
    basew->data[card][speed][p]->input = basew->inputs->buttons->view->input;
    basew->data[card][speed][p]->output = basew->inputs->buttons->view->output;
    basew->data[card][speed][p]->playback = basew->inputs->buttons->view->playback;
    basew->data[card][speed][p]->submix = basew->inputs->buttons->view->submix;
    basew->data[card][speed][p]->submix_value = basew->inputs->buttons->view->submix_value;
    basew->data[card][speed][p]->solo = basew->inputs->buttons->master->solo;
    basew->data[card][speed][p]->mute = basew->inputs->buttons->master->mute;
    basew->data[card][speed][p]->over = basew->setup->over_val;
    basew->data[card][speed][p]->rate = basew->setup->rate_val;
    basew->data[card][speed][p]->level = basew->setup->level_val;
    basew->data[card][speed][p]->numbers = basew->setup->numbers_val;
    basew->data[card][speed][p]->rmsplus3 = basew->setup->rmsplus3_val;
}

void HDSPMixerPresets::restore_preset(int prst) {
    int speed = basew->cards[basew->current_card]->speed_mode;
    int card = basew->current_card;
    int p = prst-1;
    basew->dirty = 0;
    basew->inputs->buttons->master->solo_active = 0;
    basew->inputs->buttons->master->mute_active = 0;
    
    for (int i = 0; i < HDSP_MAX_CHANNELS; i++) {
	for (int z = 0; z < HDSP_MAX_DEST; z++) {
	    basew->inputs->strips[i]->pan->pos[z] = basew->inputs->strips[i]->data[card][speed][p]->pan_pos[z];
	    basew->inputs->strips[i]->fader->pos[z] = basew->inputs->strips[i]->data[card][speed][p]->fader_pos[z];
	    basew->playbacks->strips[i]->pan->pos[z] = basew->playbacks->strips[i]->data[card][speed][p]->pan_pos[z];
	    basew->playbacks->strips[i]->fader->pos[z] = basew->playbacks->strips[i]->data[card][speed][p]->fader_pos[z];
	}
	
	basew->inputs->buttons->master->mute_active += (basew->inputs->strips[i]->mutesolo->mute = basew->inputs->strips[i]->data[card][speed][p]->mute);
	basew->inputs->buttons->master->solo_active += (basew->inputs->strips[i]->mutesolo->solo = basew->inputs->strips[i]->data[card][speed][p]->solo);
	basew->inputs->strips[i]->targets->selected = basew->inputs->strips[i]->data[card][speed][p]->dest;

	basew->inputs->buttons->master->mute_active += (basew->playbacks->strips[i]->mutesolo->mute = basew->playbacks->strips[i]->data[card][speed][p]->mute);
	basew->inputs->buttons->master->solo_active += (basew->playbacks->strips[i]->mutesolo->solo = basew->playbacks->strips[i]->data[card][speed][p]->solo);
	basew->playbacks->strips[i]->targets->selected = basew->playbacks->strips[i]->data[card][speed][p]->dest;
	
	basew->outputs->strips[i]->fader->pos[0] = basew->outputs->strips[i]->data[card][speed][p]->fader_pos;
    }
    /* Line outs */
    basew->outputs->strips[HDSP_MAX_CHANNELS]->fader->pos[0] = basew->outputs->strips[HDSP_MAX_CHANNELS+1]->data[card][speed][p]->fader_pos;
    basew->outputs->strips[HDSP_MAX_CHANNELS+1]->fader->pos[0] = basew->outputs->strips[HDSP_MAX_CHANNELS+1]->data[card][speed][p]->fader_pos;

    for (int i = 0; i < basew->cards[card]->channels_input; ++i) {
	basew->inputs->strips[i]->fader->sendGain();
	basew->inputs->strips[i]->redraw();
    }

    for (int i = 0; i < basew->cards[card]->channels_playback; ++i) {
	basew->playbacks->strips[i]->fader->sendGain();
	basew->playbacks->strips[i]->redraw();
	basew->outputs->strips[i]->fader->sendGain();
	basew->outputs->strips[i]->redraw();
    }
    basew->outputs->strips[basew->cards[card]->channels_playback]->fader->sendGain();
    basew->outputs->strips[basew->cards[card]->channels_playback]->redraw();
    basew->outputs->strips[basew->cards[card]->channels_playback+1]->fader->sendGain();
    basew->outputs->strips[basew->cards[card]->channels_playback+1]->redraw();

    /* Global settings */
    basew->inputs->buttons->view->input = basew->data[card][speed][p]->input;
    basew->inputs->buttons->view->output = basew->data[card][speed][p]->output;
    basew->inputs->buttons->view->playback = basew->data[card][speed][p]->playback; 
    basew->inputs->buttons->view->submix = basew->data[card][speed][p]->submix;
    basew->inputs->buttons->view->submix_value = basew->data[card][speed][p]->submix_value;
    if (basew->inputs->buttons->view->submix) {
	basew->setSubmix(basew->inputs->buttons->view->submix_value);
	basew->menubar->mode(12, FL_MENU_TOGGLE|FL_MENU_VALUE);
    } else {
	basew->menubar->mode(12, FL_MENU_TOGGLE);
    }
    basew->inputs->buttons->master->solo = basew->data[card][speed][p]->solo;
    basew->inputs->buttons->master->mute = basew->data[card][speed][p]->mute;
    basew->inputs->buttons->redraw();
    basew->reorder();
    
    basew->setup->over_val = basew->data[card][speed][p]->over;
    basew->setup->rate_val = basew->data[card][speed][p]->rate;
    basew->setup->numbers_val = basew->data[card][speed][p]->numbers;
    basew->setup->rmsplus3_val = basew->data[card][speed][p]->rmsplus3;
    basew->setup->level_val = basew->data[card][speed][p]->level;
    basew->setup->updateValues();

    if (basew->inputs->buttons->view->submix) {
	basew->setSubmix(basew->inputs->buttons->view->submix_value);
    } else {
	basew->unsetSubmix();
    }
    basew->refreshMixer();

#ifdef NON_MODAL_SETUP
    if (basew->setup->shown()) {
	basew->setup->redraw();
    }
#endif
}

void HDSPMixerPresets::preset_change(int p) {
    preset = p;
    basew->current_preset = preset-1;
    presetmask = (int)pow(2, preset-1);
    if (saving) {
	saving = 0;
	save_preset(p);
    } else {
	restore_preset(p);
    }
    redraw();
}

