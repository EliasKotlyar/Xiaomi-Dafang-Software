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
#include "HDSPMixerCard.h"

static void alsactl_cb(snd_async_handler_t *handler)
{
    int err, clock_value;
    snd_ctl_t *ctl;
    snd_ctl_event_t *event;
    snd_ctl_elem_value_t *elemval;
    snd_ctl_elem_id_t *elemid;
    HDSPMixerCard *card;
    
    card = (HDSPMixerCard *)snd_async_handler_get_callback_private(handler);
    
    snd_ctl_elem_value_alloca(&elemval);
    snd_ctl_elem_id_alloca(&elemid);
    
    ctl = snd_async_handler_get_ctl(handler);
    
    if ((err = snd_ctl_nonblock(ctl, 1))) {
	printf("Error setting non blocking mode for card %s\n", card->name);
	return;
    }
    
    snd_ctl_event_malloc(&event);    
    
    while ((err = snd_ctl_read(ctl, event)) > 0) {
	if (snd_ctl_event_elem_get_numid(event) == 11 && (card == card->basew->cards[card->basew->current_card])) {
	    /* We have a clock change and are the focused card */
	    snd_ctl_event_elem_get_id(event, elemid);
	    snd_ctl_elem_value_set_id(elemval, elemid);
	    if ((err = snd_ctl_elem_read(ctl, elemval)) < 0) {
		fprintf(stderr, "Error reading snd_ctl_elem_t\n");
		snd_ctl_event_free(event);
		return;
	    }
	    clock_value = snd_ctl_elem_value_get_enumerated(elemval, 0);
	    if (clock_value == 0) {
		int new_speed = card->getAutosyncSpeed();
		if (new_speed >= 0 && new_speed != card->speed_mode) card->setMode(new_speed);
	    }
	    if (clock_value > 3 && clock_value < 7 && card->speed_mode != 1) {
		card->setMode(1);
	    } else if (clock_value < 4 && card->speed_mode != 0) {
		card->setMode(0);
	    } else if (clock_value > 6 && card->speed_mode != 2) {
		card->setMode(2);
	    }
	}
	snd_ctl_event_clear(event);
    }
    
    snd_ctl_event_free(event);
}

int HDSPMixerCard::getAutosyncSpeed()
{
    int err, rate;
    snd_ctl_elem_value_t *elemval;
    snd_ctl_elem_id_t * elemid;
    snd_ctl_t *handle;
    snd_ctl_elem_value_alloca(&elemval);
    snd_ctl_elem_id_alloca(&elemid);
    if ((err = snd_ctl_open(&handle, name, SND_CTL_NONBLOCK)) < 0) {
	fprintf(stderr, "Error accessing ctl interface on card %s\n.", name);
	return -1; 
    }
    
    snd_ctl_elem_id_set_name(elemid, "System Sample Rate");
    snd_ctl_elem_id_set_interface(elemid, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_index(elemid, 0);
    snd_ctl_elem_value_set_id(elemval, elemid);
    if (snd_ctl_elem_read(handle, elemval) < 0) {
	snd_ctl_elem_id_set_interface(elemid, SND_CTL_ELEM_IFACE_HWDEP);
	snd_ctl_elem_value_set_id(elemval, elemid);
	snd_ctl_elem_read(handle, elemval);
    }
    rate = snd_ctl_elem_value_get_integer(elemval, 0);

    snd_ctl_close(handle);

    if (rate > 96000) {
	return 2;
    } else if (rate > 48000) {
	return 1;
    }
    return 0;
}

int HDSPMixerCard::getSpeed()
{
    int err, val;
    snd_ctl_elem_value_t *elemval;
    snd_ctl_elem_id_t * elemid;
    snd_ctl_t *handle;
    
    snd_ctl_elem_value_alloca(&elemval);
    snd_ctl_elem_id_alloca(&elemid);

    if ((err = snd_ctl_open(&handle, name, SND_CTL_NONBLOCK)) < 0) {
	fprintf(stderr, "Error accessing ctl interface on card %s\n.", name);
	return -1; 
    }
    snd_ctl_elem_id_set_name(elemid, "Sample Clock Source");
    snd_ctl_elem_id_set_interface(elemid, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_index(elemid, 0);
    snd_ctl_elem_value_set_id(elemval, elemid);
    if (snd_ctl_elem_read(handle, elemval) < 0) {
	snd_ctl_elem_id_set_interface(elemid, SND_CTL_ELEM_IFACE_PCM);
	snd_ctl_elem_value_set_id(elemval, elemid);
	snd_ctl_elem_read(handle, elemval);
    }
    val = snd_ctl_elem_value_get_enumerated(elemval, 0);
    snd_ctl_close(handle);

    switch (val) {
    case 0:
	/* Autosync mode : We need to determine sample rate */
	return getAutosyncSpeed();
	break;
    case 1:
    case 2:
    case 3:
	/* SR <= 48000 - normal speed */
	return 0;
    case 4:
    case 5:
    case 6:
	/* SR > 48000 Hz - double speed */
	return 1;
    case 7:
    case 8:
    case 9:
	/* SR > 96000 Hz - quad speed */
	return 2;    
    default:
	/* Should never happen */
	return 0;
    }
    return 0;    
}

HDSPMixerCard::HDSPMixerCard(int cardtype, int id, char *shortname)
{
    type = cardtype;
    card_id = id;
    snprintf(name, 6, "hw:%i", card_id);
    cardname = shortname;
    h9632_aeb.aebi = 0;
    h9632_aeb.aebo = 0;
    if (type == H9632) {
	getAeb();
	playbacks_offset = 16;
    } else {
	playbacks_offset = 26;
    }

    speed_mode = getSpeed();
    if (speed_mode < 0) {
	fprintf(stderr, "Error trying to determine speed mode for card %s, exiting.\n", name);
	exit(EXIT_FAILURE);
    }
    
    /* Set channels and mappings */
    adjustSettings();
    last_preset = last_dirty = 0;
        
    basew = NULL;
}

void HDSPMixerCard::getAeb() {
    int err;
    snd_hwdep_t *hw;
    snd_hwdep_info_t *info;
    snd_hwdep_info_alloca(&info);
    if ((err = snd_hwdep_open(&hw, name, SND_HWDEP_OPEN_DUPLEX)) != 0) {
	fprintf(stderr, "Error opening hwdep device on card %s.\n", name);
	return; 
    }
    if ((err = snd_hwdep_ioctl(hw, SNDRV_HDSP_IOCTL_GET_9632_AEB, &h9632_aeb)) < 0) {
	fprintf(stderr, "Hwdep ioctl error on card %s : %s.\n", name, snd_strerror(err));
	snd_hwdep_close(hw);
	return; 
    }
    snd_hwdep_close(hw);
}

void HDSPMixerCard::adjustSettings() {
    if (type == Multiface) {
        switch (speed_mode) {
        case 0:
            channels_input = 18;
            channels_playback = 18;
            channels_output = 20; /* SS 8xAnalog+8xADAT+2xSPDIF+2xHeadphone */
            channel_map_input = channel_map_playback = channel_map_mf_ss;
            dest_map = dest_map_mf_ss;
            meter_map_input = meter_map_playback = channel_map_mf_ss;
            break;
        case 1:
            channels_input = 14;
            channels_playback = 14;
            channels_output = 16; /* DS 8xAnalog+4xADAT(SMUX)+2xSPDIF+2xHeadphone */
            channel_map_input = channel_map_playback = meter_map_ds;
            dest_map = dest_map_ds;
            meter_map_input = meter_map_playback = meter_map_ds;
            break;
        case 2:
            /* should never happen */
            break;
        }
    }
    
    if (type == Digiface) {
        switch (speed_mode) {
        case 0:
            channels_input = channels_playback = 26;
            channels_output = 28; /* SS 3x8xADAT+2xSPDIF+2xHeadphone */
            channel_map_input = channel_map_playback = channel_map_df_ss;
            dest_map = dest_map_df_ss;
            meter_map_input = meter_map_playback = channel_map_df_ss;
            break;
        case 1:
            channels_input = channels_playback = 14;
            channels_output = 16; /* DS 3x4xADAT(SMUX)+2xSPDIF+2xHeadphone */
            channel_map_input = channel_map_playback = meter_map_ds;
            dest_map = dest_map_ds;
            meter_map_input = meter_map_playback = meter_map_ds;
            break;
        case 2:
            /* should never happen */
            break;
        }
    }

    if (type == RPM) {
        /* RPM has no digital audio connectors, hence channel mappings don't
         * depend on speedmode */
        channels_input = 5;
        channels_playback = channels_output = 6; /* 2xMain,2xMon,2xPH */
        channel_map_input = channel_map_playback = channel_map_rpm;
        dest_map = dest_map_rpm;
        meter_map_input = meter_map_playback = channel_map_rpm;
    }


    if (type == H9652) {
        switch (speed_mode) {
        case 0:
            channels_input = channels_playback = 26;
            channels_output = 26; /* SS like Digiface, but no Headphones */
            channel_map_input = channel_map_playback = channel_map_df_ss;
            dest_map = dest_map_h9652_ss;
            meter_map_input = meter_map_playback = channel_map_df_ss;
            break;
        case 1:
            channels_input = channels_playback = 14;
            channels_output = 14; /* DS like Digiface, but no Headphones */
            channel_map_input = channel_map_playback = channel_map_ds;
            dest_map = dest_map_h9652_ds;
            meter_map_input = meter_map_playback = meter_map_ds;
            break;
        case 2:
            /* should never happen */
            break;
        }
    }

    if (type == H9632) {
        switch (speed_mode) {
        case 0:
            channels_input = channels_playback = 12 + ((h9632_aeb.aebi || h9632_aeb.aebo) ? 4 : 0);
            channels_output = channels_playback; /* untested, no idea about this card */
            channel_map_input = channel_map_playback = channel_map_h9632_ss;
            dest_map = dest_map_h9632_ss;
            meter_map_input = meter_map_playback = channel_map_h9632_ss;
            break;
        case 1:
            channels_input = channels_playback = 8 + ((h9632_aeb.aebi || h9632_aeb.aebo) ? 4 : 0);
            channels_output = channels_playback; /* untested, no idea about this card */
            channel_map_input = channel_map_playback = channel_map_h9632_ds;
            dest_map = dest_map_h9632_ds;
            meter_map_input = meter_map_playback = channel_map_h9632_ds;
            break;
        case 2:
            channels_input = channels_playback = 4 + ((h9632_aeb.aebi || h9632_aeb.aebo) ? 4 : 0);
            channels_output = channels_playback; /* untested, no idea about this card */
            channel_map_input = channel_map_playback = channel_map_h9632_qs;
            dest_map = dest_map_h9632_qs;
            meter_map_input = meter_map_playback = channel_map_h9632_qs;
            break;
        }
    }

    if (HDSPeMADI == type) {
        playbacks_offset = 64;

        switch (speed_mode) {
        case 0: // SS
            channels_input = channels_playback = 64;
            channels_output = channels_input; /* SS headphones missing, at least HDSPe MADI has some, MADIface hasn't */
            channel_map_input = channel_map_playback = channel_map_unity_ss;
            dest_map = dest_map_unity;
            meter_map_input = meter_map_playback = channel_map_unity_ss;
            break;
        case 1: // DS
            channels_input = channels_playback = 32;
            channels_output = channels_input; /* DS headphones missing, at least HDSPe MADI has some, MADIface hasn't */
            channel_map_input = channel_map_playback = channel_map_unity_ss;
            dest_map = dest_map_unity;
            meter_map_input = meter_map_playback = channel_map_unity_ss;
            break;
        case 2: // QS
            channels_input = channels_playback = 16;
            channels_output = channels_input; /* QS headphones missing, at least HDSPe MADI has some, MADIface hasn't */
            channel_map_input = channel_map_playback = channel_map_unity_ss;
            dest_map = dest_map_unity;
            meter_map_input = meter_map_playback = channel_map_unity_ss;
            break;
        }

    }

    if (HDSPeAIO == type) {
        playbacks_offset = 64;

        switch (speed_mode) {
        case 0: // SS
            channels_input = 18;
            channels_playback = 20;
            channels_output = 20; /* SS 2xAnalog+2xAES+2xSPDIF+8xADAT+2xHeadphones+4xAEB */
            channel_map_input = channel_map_aio_in_ss;
            channel_map_playback = channel_map_aio_out_ss;
            dest_map = dest_map_aio_ss;
            meter_map_input = channel_map_aio_in_ss;
            meter_map_playback = channel_map_aio_out_ss;
            break;
        case 1: // DS
            channels_input = 14;
            channels_playback = 16;
            channels_output = 16; /* DS 2xAnalog+2xAES+2xSPDIF+4xADAT(SMUX)+2xHeadphones+4xAEB */
            channel_map_input = channel_map_aio_in_ds;
            channel_map_playback = channel_map_aio_out_ds;
            dest_map = dest_map_aio_ds;
            meter_map_input = channel_map_aio_in_ds;
            meter_map_playback = channel_map_aio_out_ds;
            break;
        case 2: // QS
            channels_input = 12;
            channels_playback = 14;
            channels_output = 14; /* QS 2xAnalog+2xAES+2xSPDIF+2xADAT(SMUX)+2xHeadphones+4xAEB */
            channel_map_input = channel_map_aio_in_qs;
            channel_map_playback = channel_map_aio_out_qs;
            dest_map = dest_map_aio_qs;
            meter_map_input = channel_map_aio_in_qs;
            meter_map_playback = channel_map_aio_out_qs;
            break;
        }

    }

    if (HDSP_AES == type) {
        playbacks_offset = 64; /* not sure about this one? */

        /* 16 channels for all modes */
        channels_input = 16;
        channels_playback = 16;
        channels_output = 16;
        channel_map_input = channel_map_aes32;
        channel_map_playback = channel_map_aes32;
        dest_map = dest_map_aes32;
        meter_map_input = channel_map_aes32;
        meter_map_playback = channel_map_aes32;

    }

    if (HDSPeRayDAT == type) {
        playbacks_offset = 64;

        switch (speed_mode) {
        case 0: // SS
            channels_input = 36;
            channels_playback = 36;
            channels_output = 36; /* SS 4x8xADAT+2xAES/EBU+2xSPDIF */
            channel_map_input = channel_map_playback = channel_map_raydat_ss;
            dest_map = dest_map_raydat_ss;
            meter_map_input = meter_map_playback = channel_map_raydat_ss;
            break;
        case 1: // DS
            channels_input = 20;
            channels_playback = 20;
            channels_output = 20; /* DS 4x4xADAT(SMUX)+2xAES/EBU+2xSPDIF */
            channel_map_input = channel_map_playback = channel_map_raydat_ds;
            dest_map = dest_map_raydat_ds;
            meter_map_input = meter_map_playback = channel_map_raydat_ds;
            break;
        case 2: // QS
            channels_input = 12;
            channels_playback = 12;
            channels_output = 12; /* QS 4x2xADAT(SMUX)+2xAES/EBU+2xSPDIF */
            channel_map_input = channel_map_playback = channel_map_raydat_qs;
            dest_map = dest_map_raydat_qs;
            meter_map_input = meter_map_playback = channel_map_raydat_qs;
            break;
        }

    }

    window_width = (channels_playback+2)*STRIP_WIDTH;
    window_height = FULLSTRIP_HEIGHT*2+SMALLSTRIP_HEIGHT+MENU_HEIGHT;
}

void HDSPMixerCard::setMode(int mode)
{
    speed_mode = mode;
    adjustSettings();
    actualizeStrips();

    for (int i = 0; i < channels_input; ++i) {
      basew->inputs->strips[i]->targets->setLabels();
    }
    for (int i = 0; i < channels_playback; ++i) {
      basew->playbacks->strips[i]->targets->setLabels();
    }
    for (int i = 0; i < channels_output; ++i) {
      basew->outputs->strips[i]->setLabels();
    }

    if (h9632_aeb.aebo && !h9632_aeb.aebi) {
	basew->inputs->empty_aebi[0]->position(STRIP_WIDTH*(channels_input-4), basew->inputs->empty_aebi[0]->y());
	basew->inputs->empty_aebi[1]->position(STRIP_WIDTH*(channels_input-2), basew->inputs->empty_aebi[1]->y());
    } else if (h9632_aeb.aebi && !h9632_aeb.aebo) {
	basew->playbacks->empty_aebo[0]->position(STRIP_WIDTH*(channels_playback-4), basew->playbacks->empty_aebo[0]->y());
	basew->playbacks->empty_aebo[1]->position(STRIP_WIDTH*(channels_playback-2), basew->playbacks->empty_aebo[1]->y());
	basew->outputs->empty_aebo[0]->position(STRIP_WIDTH*(channels_playback-4), basew->outputs->empty_aebo[0]->y());
	basew->outputs->empty_aebo[1]->position(STRIP_WIDTH*(channels_playback-2), basew->outputs->empty_aebo[1]->y());
    }
    basew->inputs->buttons->position(STRIP_WIDTH*channels_input, basew->inputs->buttons->y());
    basew->inputs->init_sizes();
    basew->playbacks->empty->position(STRIP_WIDTH*channels_playback, basew->playbacks->empty->y());
    basew->playbacks->init_sizes();
    basew->outputs->empty->position(STRIP_WIDTH*(channels_playback), basew->outputs->empty->y());    
    basew->outputs->init_sizes();
    basew->inputs->size(window_width, basew->inputs->h());
    basew->playbacks->size(window_width, basew->playbacks->h());
    basew->outputs->size(window_width, basew->outputs->h());
    basew->scroll->init_sizes();
    ((Fl_Widget *)(basew->menubar))->size(window_width, basew->menubar->h());
    basew->size_range(MIN_WIDTH, MIN_HEIGHT, window_width, window_height);
    basew->resize(basew->x(), basew->y(), window_width, basew->h());
    basew->reorder();
    basew->resetMixer();
    basew->inputs->buttons->presets->preset_change(1);
}

void HDSPMixerCard::actualizeStrips()
{
    for (int i = 0; i < HDSP_MAX_CHANNELS; ++i) {
	if (i < channels_input) {
	    basew->inputs->strips[i]->show();
	} else {
	    basew->inputs->strips[i]->hide();
	}

	if (i < channels_playback) {
	    basew->playbacks->strips[i]->show();
	} else {
	    basew->playbacks->strips[i]->hide();
	}

	if (i < channels_output) {
	    basew->outputs->strips[i]->show();
	} else {
	    basew->outputs->strips[i]->hide();
	}
    }
    if (h9632_aeb.aebi && !h9632_aeb.aebo) {
	for (int i = 0; i < 2; ++i) {
	    basew->inputs->empty_aebi[i]->hide();
	    basew->playbacks->empty_aebo[i]->show();
	    basew->outputs->empty_aebo[i]->show();
	}
	for (int i = channels_playback-4; i < channels_playback; ++i) {
	    basew->playbacks->strips[i]->hide();
	    basew->outputs->strips[i]->hide();
	}
    } else if (h9632_aeb.aebo && !h9632_aeb.aebi) { 
	for (int i = 0; i < 2; ++i) {
	    basew->inputs->empty_aebi[i]->show();
	    basew->playbacks->empty_aebo[i]->hide();
	    basew->outputs->empty_aebo[i]->hide();
	}        
	for (int i = channels_input-4; i < channels_input; ++i) {
	    basew->inputs->strips[i]->hide();
	}
    } else {
	for (int i = 0; i < 2; ++i) {
	    basew->inputs->empty_aebi[i]->hide();
	    basew->playbacks->empty_aebo[i]->hide();
	    basew->outputs->empty_aebo[i]->hide();
	}
    }
    if (type != H9652 && type != H9632) basew->outputs->empty->hide();
}

int HDSPMixerCard::initializeCard(HDSPMixerWindow *w)
{
    int err;
    if ((err = snd_ctl_open(&cb_handle, name, SND_CTL_NONBLOCK)) < 0) {
	fprintf(stderr, "Error opening ctl interface for card %s - exiting\n", name);
	exit(EXIT_FAILURE);
    }
    if ((err = snd_async_add_ctl_handler(&cb_handler, cb_handle, alsactl_cb, this)) < 0) {
	fprintf(stderr, "Error registering async ctl callback for card %s - exiting\n", name);
	exit(EXIT_FAILURE);
    }
    if ((err = snd_ctl_subscribe_events(cb_handle, 1)) < 0) {
	fprintf(stderr, "Error subscribing to ctl events for card %s - exiting\n", name);
	exit(EXIT_FAILURE);
    }
    basew = w;
    actualizeStrips();
    return 0;
}

