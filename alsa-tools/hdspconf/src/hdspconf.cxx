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

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <alsa/asoundlib.h>
#include <alsa/sound/hdsp.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Tabs.H>
#include "HC_CardPane.h"
#include "HC_XpmRenderer.h"
#include "pixmaps.h"
#include "HC_AboutText.h"
#include "defines.h"

//#define GUI_TEST

class HC_CardPane;
class HC_XpmRenderer;
class HC_AboutText;

static void refresh_cb(void *arg)
{
    Fl_Tabs *tabs = (Fl_Tabs *)arg;
    int err;
    snd_hwdep_t *hw;
    char card_name[6];
    hdsp_config_info_t config_info;
    
    for (int i = 0; i < tabs->children()-1 ; ++i) {
	HC_CardPane *pane = (HC_CardPane *)tabs->child(i);
	if (!pane->visible()) {
	    break;
	}
	snprintf(card_name, 6, "hw:%i", pane->alsa_index);

	if ((err = snd_hwdep_open(&hw, card_name, SND_HWDEP_OPEN_READ)) != 0) {
	    fprintf(stderr, "Error opening hwdep device on card %s.\n", card_name);
	    break;
	}
    
	if ((err = snd_hwdep_ioctl(hw, SNDRV_HDSP_IOCTL_GET_CONFIG_INFO, (void *)&config_info)) < 0) {
	    fprintf(stderr, "Hwdep ioctl error on card %s.\n", card_name);
	    snd_hwdep_close(hw);
	    break;
	}

	snd_hwdep_close(hw);

	pane->sync_ref->setRef(config_info.pref_sync_ref);
	pane->spdif_freq->setFreq(config_info.spdif_sample_rate);
	pane->sync_check->setAdat1Status(config_info.adat_sync_check[0]);
	pane->sync_check->setSpdifStatus(config_info.spdif_sync_check);
	pane->sync_check->setWCStatus(config_info.wordclock_sync_check);
	if (pane->type != H9632) {
	    pane->sync_check->setAdatSyncStatus(config_info.adatsync_sync_check);
	}
	if (pane->type == Digiface || pane->type == H9652) {
	    pane->sync_check->setAdat2Status(config_info.adat_sync_check[1]);
	    pane->sync_check->setAdat3Status(config_info.adat_sync_check[2]);
	}
	pane->spdif_in->setInput(config_info.spdif_in);
	if (pane->spdif_out->lock == 0) {
	    pane->spdif_out->setOut(config_info.spdif_out);
	    pane->spdif_out->setProfessional(config_info.spdif_professional);
	    pane->spdif_out->setEmphasis(config_info.spdif_emphasis);
	    pane->spdif_out->setNonaudio(config_info.spdif_nonaudio);
	}
	pane->clock_source->setSource(config_info.clock_source);	
	pane->autosync_ref->setRef(config_info.autosync_ref);
	pane->autosync_ref->setFreq(config_info.autosync_sample_rate);
	pane->system_clock->setMode(config_info.system_clock_mode);
	pane->system_clock->setFreq(config_info.system_sample_rate);
	if (pane->type == H9632) {
	    pane->input_level->setInputLevel(config_info.ad_gain);
	    pane->output_level->setOutputLevel(config_info.da_gain);
	    pane->phones->setPhones(config_info.phone_gain);
	    pane->breakout_cable->setXlr(config_info.xlr_breakout_cable);
	}
	if (pane->type == H9632 || pane->type == H9652) {
	    pane->aeb->setAdatInternal(config_info.analog_extension_board);
	}
    }

    Fl::add_timeout(0.3, refresh_cb, arg);
    return;
}

int main(int argc, char **argv)
{
    Fl_Window *window;
    Fl_Tabs   *tabs;
    HC_CardPane  *card_panes[4];
    HC_XpmRenderer *lad_banner;
    HC_XpmRenderer *alsa_logo;
    HC_XpmRenderer *rme_logo;
    HC_AboutText *about_text;
    Fl_Group  *about_pane;
    char *name;
    int card;
    HDSP_IO_Type hdsp_cards[4];
    int alsa_index[4];
    snd_ctl_card_info_t *info;
    snd_pcm_info_t *pcminfo;
    int cards = 0;
    
    snd_ctl_card_info_alloca(&info);
    snd_pcm_info_alloca(&pcminfo);    
    card = -1;
    printf("\nHDSPConf %s - Copyright (C) 2003 Thomas Charbonnel <thomas@undata.org>\n", VERSION);
    printf("This program comes WITH ABSOLUTELY NO WARRANTY\n");
    printf("HDSPConf is free software, see the file copying for details\n\n");
    printf("Looking for HDSP cards :\n");

#ifdef GUI_TEST
    hdsp_cards[0] = Digiface;
    alsa_index[0] = 0;
    hdsp_cards[1] = H9652;
    alsa_index[1] = 1;
    hdsp_cards[2] = Multiface;
    alsa_index[2] = 2;
    hdsp_cards[3] = H9632;
    alsa_index[3] = 3;
    cards = 4;
#else
    while (snd_card_next(&card) >= 0 && cards < 4) {
	if (card < 0) {
	    break;
	} else {
	    snd_card_get_longname(card, &name);
	    printf("Card %d : %s\n", card, name);
	    if (!strncmp(name, "RME Hammerfall DSP + Multiface", 30)) {
		printf("Multiface found !\n");
		hdsp_cards[cards] = Multiface;
		alsa_index[cards] = card;
		cards++;
	    } else if (!strncmp(name, "RME Hammerfall DSP + Digiface", 29)) {
		printf("Digiface found !\n");
		hdsp_cards[cards] = Digiface;
		alsa_index[cards] = card;
		cards++;
	    } else if (!strncmp(name, "RME Hammerfall HDSP 9652", 24)) {
		printf("HDSP 9652 found !\n");
		hdsp_cards[cards] = H9652;
		alsa_index[cards] = card;
		cards++;
	    } else if (!strncmp(name, "RME Hammerfall HDSP 9632", 24)) {
		printf("HDSP 9632 found !\n");
		hdsp_cards[cards] = H9632;
		alsa_index[cards] = card;
		cards++;	
	    } else if (!strncmp(name, "RME Hammerfall DSP", 18)) {
		printf("Uninitialized HDSP card found.\nUse hdsploader to upload configuration data to the card.\n");
	    }
            free(name);
	}
    }
#endif
    if (!cards) {
	printf("No Hammerfall DSP card found.\n");
	exit(1);
    }
    printf("%d Hammerfall DSP %s found.\n", cards, (cards > 1) ? "cards" : "card");
    
    window = new Fl_Window(WINDOW_WIDTH, WINDOW_HEIGHT, "Hammerfall DSP Alsa Settings");
    tabs = new Fl_Tabs(TABS_X, TABS_Y, TABS_W, TABS_H);
    window->end();
    for (int i = 0; i < cards; ++i) {
	    card_panes[i] = new HC_CardPane(alsa_index[i], i, hdsp_cards[i]);
	    tabs->add((Fl_Group *)card_panes[i]);
    }
    about_pane = new Fl_Group(10, 30, 480, 360, "About");
    about_pane->labelsize(10);
    about_text = new HC_AboutText(80, 70, 440, 210); 
    rme_logo = new HC_XpmRenderer(60, 328, 113, 35, rme_xpm);
    alsa_logo = new HC_XpmRenderer(230, 320, 50, 50, alsalogo_xpm);
    lad_banner = new HC_XpmRenderer(325, 325, 113, 39, lad_banner_xpm);
    about_pane->end();
    tabs->add(about_pane);
#ifndef GUI_TEST
    refresh_cb((void *)tabs);
#endif
    window->show(argc, argv);
    return Fl::run();    
}

