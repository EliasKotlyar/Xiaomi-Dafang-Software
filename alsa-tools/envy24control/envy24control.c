/*****************************************************************************
   envy24control.c - Env24 chipset (ICE1712) control utility
   Copyright (C) 2000 by Jaroslav Kysela <perex@perex.cz>

   (2003/03/22) Changed to hbox/vbox layout.
   Copyright (C) 2003 by Søren Wedel Nielsen
   
   (16.12.2005)  Re-worked user interface -digital mixer display permanently
   visible; pcms split to another page; controls re-arranged and all pages
   scrollable for min window size and greater flexibility; pop-up menu enabled.
   Changes to levelmeters.c to prevent invalid redraws.
   New options added: 'w' to set initial window pixel width and 't' for tall equal mixer height style.
   Copyright (C) 2005 by Alan Horstmann

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
******************************************************************************/

#include "envy24control.h"
#include "midi.h"
#include "config.h"
#define _GNU_SOURCE
#include <getopt.h>

int input_channels, output_channels, pcm_output_channels, spdif_channels, view_spdif_playback, card_number;
int card_is_dmx6fire = FALSE, tall_equal_mixer_ht = 0;
char *profiles_file_name, *default_profile;

ice1712_eeprom_t card_eeprom;
snd_ctl_t *ctl;

GtkWidget *window;

GtkWidget *mixer_mix_drawing;
GtkWidget *mixer_clear_peaks_button;
GtkWidget *mixer_drawing[20];
GtkObject *mixer_adj[20][2];
GtkWidget *mixer_vscale[20][2];
GtkWidget *mixer_mute_toggle[20][2];
GtkWidget *mixer_stereo_toggle[20];

GtkWidget *router_radio[10][12];

//GtkWidget *hw_master_clock_xtal_radio;
GtkWidget *hw_master_clock_xtal_22050;
GtkWidget *hw_master_clock_xtal_32000;
GtkWidget *hw_master_clock_xtal_44100;
GtkWidget *hw_master_clock_xtal_48000;
GtkWidget *hw_master_clock_xtal_88200;
GtkWidget *hw_master_clock_xtal_96000;
GtkWidget *hw_master_clock_spdif_radio;
GtkWidget *hw_master_clock_word_radio;
GtkWidget *hw_master_clock_status_label;
GtkWidget *hw_master_clock_actual_rate_label;

GtkWidget *hw_clock_state_label;
GtkWidget *hw_clock_state_locked;
GtkWidget *hw_clock_state_reset;

GtkWidget *hw_rate_locking_check;
GtkWidget *hw_rate_reset_check;

GtkObject *hw_volume_change_adj;
GtkWidget *hw_volume_change_spin;

GtkWidget *hw_spdif_profi_nonaudio_radio;
GtkWidget *hw_spdif_profi_audio_radio;

GtkWidget *hw_profi_stream_stereo_radio;
GtkWidget *hw_profi_stream_notid_radio;

GtkWidget *hw_profi_emphasis_none_radio;
GtkWidget *hw_profi_emphasis_5015_radio;
GtkWidget *hw_profi_emphasis_ccitt_radio;
GtkWidget *hw_profi_emphasis_notid_radio;

GtkWidget *hw_consumer_copyright_on_radio;
GtkWidget *hw_consumer_copyright_off_radio;

GtkWidget *hw_consumer_copy_1st_radio;
GtkWidget *hw_consumer_copy_original_radio;

GtkWidget *hw_consumer_emphasis_none_radio;
GtkWidget *hw_consumer_emphasis_5015_radio;

GtkWidget *hw_consumer_category_dat_radio;
GtkWidget *hw_consumer_category_pcm_radio;
GtkWidget *hw_consumer_category_cd_radio;
GtkWidget *hw_consumer_category_general_radio;

GtkWidget *hw_spdif_professional_radio;
GtkWidget *hw_spdif_consumer_radio;
GtkWidget *hw_spdif_output_notebook;

GtkWidget *hw_spdif_input_coaxial_radio;
GtkWidget *hw_spdif_input_optical_radio;
GtkWidget *hw_spdif_switch_off_radio;

GtkWidget *input_interface_internal;
GtkWidget *input_interface_front_input;
GtkWidget *input_interface_rear_input;
GtkWidget *input_interface_wavetable;

GtkWidget *hw_phono_input_on_radio;
GtkWidget *hw_phono_input_off_radio;

GtkObject *av_dac_volume_adj[10];
GtkObject *av_adc_volume_adj[10];
GtkObject *av_ipga_volume_adj[10];
GtkLabel *av_dac_volume_label[10];
GtkLabel *av_adc_volume_label[10];
GtkLabel *av_ipga_volume_label[10];
GtkWidget *av_dac_sense_radio[10][4];
GtkWidget *av_adc_sense_radio[10][4];

struct profile_button {
	GtkWidget *toggle_button;
	GtkWidget *entry;
} profiles_toggle_buttons[MAX_PROFILES];

GtkWidget *active_button = NULL;
GtkObject *card_number_adj;


static void create_mixer_frame(GtkWidget *box, int stream)
{
	GtkWidget *vbox;
	GtkWidget *vbox1;
	GtkWidget *hbox;
	GtkWidget *frame;
	GtkObject *adj;
	GtkWidget *vscale;
	GtkWidget *drawing;
	GtkWidget *label;
	GtkWidget *toggle;
	char str[64], drawname[32];

	if (stream <= MAX_PCM_OUTPUT_CHANNELS) {
		sprintf(str, "PCM Out %i", stream);
	} else if (stream <= (MAX_PCM_OUTPUT_CHANNELS + MAX_SPDIF_CHANNELS)) {
		sprintf(str, "SPDIF Out %s", stream & 1 ? "L": "R");
	} else if (card_is_dmx6fire) {
		switch (stream) {
		case 11: sprintf(str, "CD In L");break;
		case 12: sprintf(str, "CD In R");break;
		case 13: sprintf(str, "Line In L");break;
		case 14: sprintf(str, "Line In R");break;
		case 15: sprintf(str, "Phono/Mic L");break;
		case 16: sprintf(str, "Phono/Mic R");break;
		case 19: sprintf(str, "Digital In L");break;
		case 20: sprintf(str, "Digital In R");break;
		default : sprintf(str, "????");break;
		}
	} else if (stream <= (MAX_PCM_OUTPUT_CHANNELS + MAX_SPDIF_CHANNELS + MAX_INPUT_CHANNELS)) {
		sprintf(str, "H/W In %i", stream - (MAX_PCM_OUTPUT_CHANNELS + MAX_SPDIF_CHANNELS));
	} else if (stream <= (MAX_PCM_OUTPUT_CHANNELS + MAX_SPDIF_CHANNELS + MAX_INPUT_CHANNELS + MAX_SPDIF_CHANNELS)) {
		sprintf(str, "SPDIF In %s", stream & 1 ? "L": "R");
	} else {
		strcpy(str, "???");
	}

	frame = gtk_frame_new(str);
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 2);

	vbox = gtk_vbox_new(FALSE, 6);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 2);

	hbox = gtk_hbox_new(FALSE, 2);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

	adj = gtk_adjustment_new(96, 0, 96, 1, 16, 0);
	mixer_adj[stream-1][0] = adj;
	vscale = gtk_vscale_new(GTK_ADJUSTMENT(adj));
	mixer_vscale[stream-1][0] = vscale;
        gtk_widget_show(vscale);
	gtk_box_pack_start(GTK_BOX(hbox), vscale, TRUE, FALSE, 0);
	gtk_scale_set_value_pos(GTK_SCALE(vscale), GTK_POS_BOTTOM);
	gtk_scale_set_digits(GTK_SCALE(vscale), 0);
	gtk_signal_connect(GTK_OBJECT(adj), "value_changed",
			   GTK_SIGNAL_FUNC(mixer_adjust),
			   (gpointer)(long)((stream << 16) + 0));

	vbox1 = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox1);
	gtk_box_pack_start(GTK_BOX(hbox), vbox1, FALSE, FALSE, 0);

	drawing = gtk_drawing_area_new();
	mixer_drawing[stream-1] = drawing;
	sprintf(drawname, "Mixer%i", stream);
	gtk_widget_set_name(drawing, drawname);
	gtk_widget_show(drawing);
	gtk_signal_connect(GTK_OBJECT(drawing), "expose_event",
			   GTK_SIGNAL_FUNC(level_meters_expose_event), NULL);
	gtk_signal_connect(GTK_OBJECT(drawing), "configure_event",
			   GTK_SIGNAL_FUNC(level_meters_configure_event), NULL);
	gtk_widget_set_events(drawing, GDK_EXPOSURE_MASK);
	gtk_widget_set_usize(drawing, 36, (60 * tall_equal_mixer_ht + 204));
	gtk_box_pack_start(GTK_BOX(vbox1), drawing, FALSE, FALSE, 1);

	label = gtk_label_new("");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox1), label, TRUE, TRUE, 2);

	adj = gtk_adjustment_new(96, 0, 96, 1, 16, 0);
	mixer_adj[stream-1][1] = adj;
	vscale = gtk_vscale_new(GTK_ADJUSTMENT(adj));
	mixer_vscale[stream-1][1] = vscale;
        gtk_widget_show(vscale);
	gtk_box_pack_start(GTK_BOX(hbox), vscale, TRUE, FALSE, 0);
	gtk_scale_set_value_pos(GTK_SCALE(vscale), GTK_POS_BOTTOM);
	gtk_scale_set_digits(GTK_SCALE(vscale), 0);
	gtk_signal_connect(GTK_OBJECT(adj), "value_changed",
			   GTK_SIGNAL_FUNC(mixer_adjust),
			   (gpointer)(long)((stream << 16) + 1));
	
	hbox = gtk_hbox_new(TRUE, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, FALSE, 0);

	label = gtk_label_new("Left");
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
	
	label = gtk_label_new("Right");
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

	toggle = gtk_toggle_button_new_with_label("L/R Gang");
	mixer_stereo_toggle[stream-1] = toggle;
	gtk_widget_show(toggle);
	gtk_box_pack_end(GTK_BOX(vbox), toggle, FALSE, FALSE, 0);
	/* gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), TRUE); */
	gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
			   GTK_SIGNAL_FUNC(config_set_stereo), (gpointer)stream-1);

	hbox = gtk_hbox_new(TRUE, 6);
	gtk_widget_show(hbox);
	gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	toggle = gtk_toggle_button_new_with_label("Mute");
	mixer_mute_toggle[stream-1][0] = toggle;
	gtk_widget_show(toggle);
	gtk_box_pack_start(GTK_BOX(hbox), toggle, FALSE, TRUE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), TRUE);
	gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
			   GTK_SIGNAL_FUNC(mixer_toggled_mute),
			   (gpointer)(long)((stream << 16) + 0));

	toggle = gtk_toggle_button_new_with_label("Mute");
	mixer_mute_toggle[stream-1][1] = toggle;
	gtk_widget_show(toggle);
	gtk_box_pack_start(GTK_BOX(hbox), toggle, FALSE, TRUE, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), TRUE);
	gtk_signal_connect(GTK_OBJECT(toggle), "toggled",
			   GTK_SIGNAL_FUNC(mixer_toggled_mute),
			   (gpointer)(long)((stream << 16) + 1));
}


static void create_inputs_mixer(GtkWidget *main, GtkWidget *notebook, int page)
{
        GtkWidget *hbox;
        GtkWidget *vbox;

	GtkWidget *label;
	GtkWidget *scrolledwindow;
	GtkWidget *viewport;
	int stream;


	hbox = gtk_hbox_new(FALSE, 3);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(notebook), hbox);

        label = gtk_label_new("Monitor Inputs");
        gtk_widget_show(label);
	gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), 
				   gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),page), 
				   label);

	/* build scrolling area */
	scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwindow);
	gtk_box_pack_start(GTK_BOX(hbox), scrolledwindow, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);

	viewport = gtk_viewport_new(NULL, NULL);
	gtk_widget_show(viewport);
	gtk_container_add(GTK_CONTAINER(scrolledwindow), viewport);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(viewport), vbox);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	for(stream = (MAX_PCM_OUTPUT_CHANNELS + MAX_SPDIF_CHANNELS + 1); \
		stream <= input_channels + (MAX_PCM_OUTPUT_CHANNELS + MAX_SPDIF_CHANNELS); stream ++) {
		if (mixer_stream_is_active(stream))
			create_mixer_frame(hbox, stream);
	}
	for(stream = (MAX_PCM_OUTPUT_CHANNELS + MAX_SPDIF_CHANNELS + MAX_INPUT_CHANNELS + 1); \
		stream <= spdif_channels + (MAX_PCM_OUTPUT_CHANNELS + MAX_SPDIF_CHANNELS + MAX_INPUT_CHANNELS); stream ++) {
		if (mixer_stream_is_active(stream))
			create_mixer_frame(hbox, stream);
	}
}

static void create_pcms_mixer(GtkWidget *main, GtkWidget *notebook, int page)
{
        GtkWidget *hbox;
        GtkWidget *vbox;

	GtkWidget *label;
	GtkWidget *scrolledwindow;
	GtkWidget *viewport;
	int stream;

	hbox = gtk_hbox_new(FALSE, 3);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(notebook), hbox);

        label = gtk_label_new("Monitor PCMs");
        gtk_widget_show(label);
	gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook),
				   gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook),page),
				   label);

	/* build scrolling area */
	scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwindow);
	gtk_box_pack_start(GTK_BOX(hbox), scrolledwindow, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow), 
					GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);

	viewport = gtk_viewport_new(NULL, NULL);
	gtk_widget_show(viewport);
	gtk_container_add(GTK_CONTAINER(scrolledwindow), viewport);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(viewport), vbox);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	for(stream = 1; stream <= pcm_output_channels; stream ++) {
		if (mixer_stream_is_active(stream))
			create_mixer_frame(hbox, stream);
	}
	for(stream = (MAX_PCM_OUTPUT_CHANNELS + 1); \
		stream <= spdif_channels + MAX_PCM_OUTPUT_CHANNELS; stream ++) {
		if (mixer_stream_is_active(stream) && view_spdif_playback)
			create_mixer_frame(hbox, stream);
	}

}

static void create_router_frame(GtkWidget *box, int stream, int pos)
{
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkWidget *radiobutton;
	GtkWidget *label;
	GtkWidget *hseparator;
	GSList *group = NULL;
	char str[64], str1[64];
	int idx;
	static char *table[10] = {
		"S/PDIF In L",
		"S/PDIF In R",
		"H/W In 1",
		"H/W In 2",
		"H/W In 3",
		"H/W In 4",
		"H/W In 5",
		"H/W In 6",
		"H/W In 7",
		"H/W In 8"
	};

	if (card_is_dmx6fire)
	{
                table[0] = "Digital In L";
                table[1] = "Digital In R";
                table[2] = "CD In L";
                table[3] = "CD In R";
                table[4] = "Line In L";
                table[5] = "Line In R";
                table[6] = "Phono/Mic L";
                table[7] = "Phono/Mic R";
	}

	if (stream <= MAX_OUTPUT_CHANNELS) {
		sprintf(str, "H/W Out %i (%s)", stream, stream & 1 ? "L" : "R");
	} else if (stream == (MAX_OUTPUT_CHANNELS + 1)) {
		if (card_is_dmx6fire) {
				strcpy(str, "Digital Out (L)");
			} else {
				strcpy(str, "S/PDIF Out (L)");
				}
	} else if (stream == (MAX_OUTPUT_CHANNELS + 2)) {
		if (card_is_dmx6fire) {
				strcpy(str, "Digital Out (R)");
			} else {
				strcpy(str, "S/PDIF Out (R)");
				}
	} else {
		strcpy(str, "???");
		}
	if ((stream == MAX_PCM_OUTPUT_CHANNELS + 1) || (stream == MAX_PCM_OUTPUT_CHANNELS + 2)) {
		sprintf(str1, "S/PDIF Out (%s)", stream & 1 ? "L" : "R");
	} else { 
		sprintf(str1, "PCM Out %i", stream);
	}

	frame = gtk_frame_new(str);
	gtk_widget_show(frame);
	gtk_box_pack_start (GTK_BOX(box), frame, FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 6);


	vbox = gtk_vbox_new(TRUE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

	radiobutton = gtk_radio_button_new_with_label(group, str1);
	router_radio[stream-1][0] = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)patchbay_toggled, 
			   (gpointer)(long)((stream << 16) + 0));


	hseparator = gtk_hseparator_new();
	gtk_widget_show(hseparator);
	gtk_box_pack_start(GTK_BOX(vbox), hseparator, FALSE, TRUE, 0);

	label = gtk_label_new("");
	gtk_widget_show(label);

	/* the digital mixer can only be routed to HW1/2 or SPDIF1/2 */
	if( (stream <= 2) /* hw1/2 */ ||
	    ((stream > MAX_OUTPUT_CHANNELS) && (stream <= MAX_OUTPUT_CHANNELS + 2)) /* spdif1/2 */
	    ) {
		radiobutton = gtk_radio_button_new_with_label(group, stream & 1 ? "Digital Mix L" : "Digital Mix R");
		router_radio[stream-1][1] = radiobutton;
		group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
		gtk_widget_show(radiobutton);
		gtk_box_pack_start(GTK_BOX(vbox), 
				    radiobutton, FALSE, FALSE, 0);
		gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
				  (GtkSignalFunc)patchbay_toggled, 
				   (gpointer)(long)((stream << 16) + 1));
	}
	else {
	  label = gtk_label_new("");
	  gtk_widget_show(label);
	  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
	}


	hseparator = gtk_hseparator_new();
	gtk_widget_show(hseparator);
	gtk_box_pack_start(GTK_BOX(vbox), hseparator, FALSE, TRUE, 0);


	for(idx = 2 - spdif_channels; idx < input_channels + 2; idx++) {
		radiobutton = gtk_radio_button_new_with_label(group, table[idx]);
		router_radio[stream-1][2+idx] = radiobutton;
		group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
		gtk_widget_show(radiobutton);
		gtk_box_pack_start(GTK_BOX(vbox), 
				    radiobutton, FALSE, FALSE, 0);
		gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
				  (GtkSignalFunc)patchbay_toggled, 
				   (gpointer)(long)((stream << 16) + 2 + idx));
	}
}

static void create_router(GtkWidget *main, GtkWidget *notebook, int page)
{
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *scrolledwindow;
	GtkWidget *viewport;
	int stream, pos;

	scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwindow);
	gtk_container_add(GTK_CONTAINER(notebook), scrolledwindow);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow), 
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);

        label = gtk_label_new("Patchbay / Router");
        gtk_widget_show(label);
	gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), 
				   gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page), 
				   label);

	viewport = gtk_viewport_new(NULL, NULL);
	gtk_widget_show(viewport);
	gtk_container_add(GTK_CONTAINER(scrolledwindow), viewport);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(viewport), hbox);

	pos = 0;
	for (stream = 1; stream <= output_channels; stream++) {
		if (patchbay_stream_is_active(stream))
			create_router_frame(hbox, stream, pos++);
	}
	for (stream = MAX_OUTPUT_CHANNELS + 1; stream <= MAX_OUTPUT_CHANNELS + spdif_channels; stream++) {
		if (patchbay_stream_is_active(stream))
			create_router_frame(hbox, stream, pos++);
	}
}

static void create_master_clock(GtkWidget *box)
{
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *radiobutton;
	GtkWidget *label;
	GSList *group = NULL;

	frame = gtk_frame_new("Master Clock");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, FALSE, 4);


	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);


	radiobutton = gtk_radio_button_new_with_label(group, "Int 22050");
	hw_master_clock_xtal_22050 = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)internal_clock_toggled, 
			  (gpointer)"22050");


	radiobutton = gtk_radio_button_new_with_label(group, "Int 32000");
	hw_master_clock_xtal_32000 = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)internal_clock_toggled, 
			  (gpointer)"32000");


	radiobutton = gtk_radio_button_new_with_label(group, "Int 44100");
	hw_master_clock_xtal_44100 = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)internal_clock_toggled, 
			  (gpointer)"44100");


	radiobutton = gtk_radio_button_new_with_label(group, "Int 48000");
	hw_master_clock_xtal_48000 = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)internal_clock_toggled, 
			  (gpointer)"48000");


	radiobutton = gtk_radio_button_new_with_label(group, "Int 88200");
	hw_master_clock_xtal_88200 = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)internal_clock_toggled, 
			  (gpointer)"88200");


	radiobutton = gtk_radio_button_new_with_label(group, "Int 96000");
	hw_master_clock_xtal_96000 = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)internal_clock_toggled, 
			  (gpointer)"96000");



	radiobutton = gtk_radio_button_new_with_label(group, "S/PDIF In");
	hw_master_clock_spdif_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)internal_clock_toggled, 
			  (gpointer)"SPDIF");



	if (card_eeprom.subvendor != ICE1712_SUBDEVICE_DELTA1010 &&
	    card_eeprom.subvendor != ICE1712_SUBDEVICE_DELTA1010LT)
		return;

	radiobutton = gtk_radio_button_new_with_label(group, "Word Clock");
	hw_master_clock_word_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)internal_clock_toggled, 
			  (gpointer)"WordClock");
	
        label = gtk_label_new("Locked");
        hw_master_clock_status_label = label;
        gtk_widget_show(label);
        gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 0);
}

static void create_rate_state(GtkWidget *box)
{
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkWidget *check;

	frame = gtk_frame_new("Rate State");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(box), frame, TRUE, TRUE, 0);

	hbox = gtk_hbox_new(TRUE, 0);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(frame), hbox);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);

	check = gtk_check_button_new_with_label("Locked");
	hw_rate_locking_check = check;
	gtk_widget_show(check);
	gtk_box_pack_start(GTK_BOX(hbox), check, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(check), "toggled",
			  (GtkSignalFunc)rate_locking_toggled, 
			  (gpointer)"locked");


	check = gtk_check_button_new_with_label("Reset");
	hw_rate_reset_check = check;
	gtk_widget_show(check);
	gtk_box_pack_start(GTK_BOX(hbox), check, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(check), "toggled",
			  (GtkSignalFunc)rate_reset_toggled, 
			  (gpointer)"reset");

}

static void create_actual_rate(GtkWidget *box)
{
	GtkWidget *frame;
	GtkWidget *label;

	frame = gtk_frame_new("Actual Rate");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(box), frame, TRUE, TRUE, 0);

	label = gtk_label_new("");
	hw_master_clock_actual_rate_label = label;
	gtk_widget_show(label);
	gtk_container_add(GTK_CONTAINER(frame), label);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_padding(GTK_MISC(label), 6, 6);
}

static void create_volume_change(GtkWidget *box)
{
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkObject *spinbutton_adj;
	GtkWidget *spinbutton;
	GtkWidget *label;

	frame = gtk_frame_new("Volume Change");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(box), frame, TRUE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(frame), hbox);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);

	label = gtk_label_new("Rate");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);
	gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

	spinbutton_adj = gtk_adjustment_new(16, 0, 255, 1, 10, 10);
	hw_volume_change_adj = spinbutton_adj;
	spinbutton = gtk_spin_button_new(GTK_ADJUSTMENT(spinbutton_adj), 1, 0);
	gtk_widget_show(spinbutton);
	gtk_box_pack_start(GTK_BOX(hbox), spinbutton, TRUE, FALSE, 0);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spinbutton), TRUE);
	gtk_signal_connect(GTK_OBJECT(spinbutton_adj), "value_changed",
			   GTK_SIGNAL_FUNC(volume_change_rate_adj), NULL);
	
}

static void create_spdif_output_settings_profi_data(GtkWidget *box)
{
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *radiobutton;
	GSList *group = NULL;

	frame = gtk_frame_new("Data Mode");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, TRUE, 0);


	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);


	radiobutton = gtk_radio_button_new_with_label(group, "Non-audio");
	hw_spdif_profi_nonaudio_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)profi_data_toggled, 
			  (gpointer)"Non-audio");

	radiobutton = gtk_radio_button_new_with_label(group, "Audio");
	hw_spdif_profi_audio_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)profi_data_toggled, 
			  (gpointer)"Audio");
}

static void create_spdif_output_settings_profi_stream(GtkWidget *box)
{
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *radiobutton;
	GSList *group = NULL;

	frame = gtk_frame_new("Stream");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, TRUE, 0);


	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

	radiobutton = gtk_radio_button_new_with_label(group, "Stereophonic");
	hw_profi_stream_stereo_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)profi_stream_toggled, 
			  (gpointer)"Stereo");

	radiobutton = gtk_radio_button_new_with_label(group, "Not indicated");
	hw_profi_stream_notid_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)profi_stream_toggled, 
			  (gpointer)"NOTID");
}

static void create_spdif_output_settings_profi_emphasis(GtkWidget *box)
{
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *radiobutton;
	GSList *group = NULL;

	frame = gtk_frame_new("Emphasis");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, TRUE, 0);


	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);


	radiobutton = gtk_radio_button_new_with_label(group, "No emphasis");
	hw_profi_emphasis_none_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)profi_emphasis_toggled, 
			  (gpointer)"No");

	radiobutton = gtk_radio_button_new_with_label(group, "50/15us");
	hw_profi_emphasis_5015_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)profi_emphasis_toggled, 
			  (gpointer)"5015");

	radiobutton = gtk_radio_button_new_with_label(group, "CCITT J.17");
	hw_profi_emphasis_ccitt_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)profi_emphasis_toggled, 
			  (gpointer)"CCITT");

	radiobutton = gtk_radio_button_new_with_label(group, "Not indicated");
	hw_profi_emphasis_notid_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)profi_emphasis_toggled, 
			  (gpointer)"NOTID");
}

static void create_spdif_output_settings_profi(GtkWidget *notebook, int page)
{
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *label;

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(notebook), hbox);

        label = gtk_label_new("Professional");
        gtk_widget_show(label);
	gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), 
				   gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page), 
				   label);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

	create_spdif_output_settings_profi_data(vbox);
	create_spdif_output_settings_profi_stream(vbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

	create_spdif_output_settings_profi_emphasis(vbox);
}

static void create_spdif_output_settings_consumer_copyright(GtkWidget *box)
{
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *radiobutton;
	GSList *group = NULL;

	frame = gtk_frame_new("Copyright");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, TRUE, 0);
	
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);


	radiobutton = gtk_radio_button_new_with_label(group, "Copyrighted");
	hw_consumer_copyright_on_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)consumer_copyright_toggled, 
			  (gpointer)"Copyright");

	radiobutton = gtk_radio_button_new_with_label(group, "Copy permitted");
	hw_consumer_copyright_off_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)consumer_copyright_toggled,
			  (gpointer)"Permitted");
}

static void create_spdif_output_settings_consumer_copy(GtkWidget *box)
{
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *radiobutton;
	GSList *group = NULL;

	frame = gtk_frame_new("Copy");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, TRUE, 0);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

	radiobutton = gtk_radio_button_new_with_label(group,
						      "1-st generation");
	hw_consumer_copy_1st_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)consumer_copy_toggled, 
			  (gpointer)"1st");

	radiobutton = gtk_radio_button_new_with_label(group, "Original");
	hw_consumer_copy_original_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)consumer_copy_toggled, 
			  (gpointer)"Original");
}

static void create_spdif_output_settings_consumer_emphasis(GtkWidget *box)
{
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *radiobutton;
	GSList *group = NULL;

	frame = gtk_frame_new("Emphasis");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, TRUE, 0);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

	radiobutton = gtk_radio_button_new_with_label(group, "No emphasis");
	hw_consumer_emphasis_none_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)consumer_emphasis_toggled, 
			  (gpointer)"No");

	radiobutton = gtk_radio_button_new_with_label(group, "50/15us");
	hw_consumer_emphasis_5015_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)consumer_emphasis_toggled, 
			  (gpointer)"5015");
}

static void create_spdif_output_settings_consumer_category(GtkWidget *box)
{
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *radiobutton;
	GSList *group = NULL;

	frame = gtk_frame_new("Category");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, TRUE, 0);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

	radiobutton = gtk_radio_button_new_with_label(group, "DAT");
	hw_consumer_category_dat_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)consumer_category_toggled, 
			  (gpointer)"DAT");

	radiobutton = gtk_radio_button_new_with_label(group, "PCM encoder");
	hw_consumer_category_pcm_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)consumer_category_toggled, 
			  (gpointer)"PCM");

	radiobutton = gtk_radio_button_new_with_label(group, "CD (ICE-908)");
	hw_consumer_category_cd_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)consumer_category_toggled, 
			  (gpointer)"CD");

	radiobutton = gtk_radio_button_new_with_label(group, "General");
	hw_consumer_category_general_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)consumer_category_toggled, 
			  (gpointer)"General");
}

static void create_spdif_output_settings_consumer(GtkWidget *notebook, int page)
{
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *label;

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(notebook), hbox);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 6);

	label = gtk_label_new("Consumer");
        gtk_widget_show(label);
	gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), 
				   gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page), 
				   label);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

	create_spdif_output_settings_consumer_copyright(vbox);
	create_spdif_output_settings_consumer_copy(vbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

	create_spdif_output_settings_consumer_emphasis(vbox);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

	create_spdif_output_settings_consumer_category(vbox);
}

static void create_spdif_output_settings(GtkWidget *box)
{
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *radiobutton;
	GtkWidget *notebook;
	GSList *group = NULL;

	frame = gtk_frame_new("S/PDIF Output Settings");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(box), frame, TRUE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 6);


	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	radiobutton = gtk_radio_button_new_with_label(NULL, "Professional");
	hw_spdif_professional_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(hbox), radiobutton, FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(radiobutton), 6);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)spdif_output_toggled, 
			  (gpointer)"Professional");

	radiobutton = gtk_radio_button_new_with_label(group, "Consumer");
	hw_spdif_consumer_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(hbox), radiobutton, FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(radiobutton), 6);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)spdif_output_toggled, 
			  (gpointer)"Consumer");


	notebook = gtk_notebook_new();
	hw_spdif_output_notebook = notebook;
	gtk_widget_show(notebook);
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);


	create_spdif_output_settings_profi(notebook, 0);
 	create_spdif_output_settings_consumer(notebook, 1); 
}

static void create_spdif_input_select(GtkWidget *box)
{
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *radiobutton;
	GSList *group = NULL;
	int hide = 1;

	if((card_eeprom.subvendor == ICE1712_SUBDEVICE_DELTADIO2496) || (card_is_dmx6fire))
		hide = 0;

	frame = gtk_frame_new("Digital Input");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(box), frame, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 6);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

	radiobutton = gtk_radio_button_new_with_label(group, "Coaxial");
	hw_spdif_input_coaxial_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)spdif_input_toggled, 
			  (gpointer)"Coaxial");

	radiobutton = gtk_radio_button_new_with_label(group, "Optical");
	hw_spdif_input_optical_radio = radiobutton;
	group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	gtk_widget_show(radiobutton);
	gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
	gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
			  (GtkSignalFunc)spdif_input_toggled, 
			  (gpointer)"Optical");

        radiobutton = gtk_radio_button_new_with_label(group, "Internal CD");
        hw_spdif_switch_off_radio = radiobutton;
        group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
	if(card_is_dmx6fire)
	        gtk_widget_show(radiobutton);
        gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
        gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
                          (GtkSignalFunc)spdif_input_toggled,
                          (gpointer)"Off");

        if(hide)
                gtk_widget_hide_all(frame);
}


static void create_phono_input(GtkWidget *box)
{
        GtkWidget *frame;
        GtkWidget *vbox;
        GtkWidget *radiobutton;
        GSList *group = NULL;
        int hide = 1;

        if(card_is_dmx6fire)
                hide = 0;

        frame = gtk_frame_new("Phono Input Switch");
        gtk_widget_show(frame);
        gtk_box_pack_start(GTK_BOX(box), frame, FALSE, TRUE, 7);
        gtk_container_set_border_width(GTK_CONTAINER(frame), 6);

        vbox = gtk_vbox_new(FALSE, 0);
        gtk_widget_show(vbox);
        gtk_container_add(GTK_CONTAINER(frame), vbox);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

        radiobutton = gtk_radio_button_new_with_label(group, "Phono");
        hw_phono_input_on_radio = radiobutton;
        group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
        gtk_widget_show(radiobutton);
        gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
        gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
                          (GtkSignalFunc)phono_input_toggled,
                          (gpointer)"Phono");

        radiobutton = gtk_radio_button_new_with_label(group, "Mic");
        hw_phono_input_off_radio = radiobutton;
        group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
        gtk_widget_show(radiobutton);
        gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
        gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
                          (GtkSignalFunc)phono_input_toggled,
                          (gpointer)"Mic");

        if(hide)
                gtk_widget_hide_all(frame);
}

static void create_input_interface(GtkWidget *box)
{
        GtkWidget *frame;
        GtkWidget *vbox;
        GtkWidget *radiobutton;
        GSList *group = NULL;
        int hide = 1;

        if (card_is_dmx6fire)
                hide = 0;

        frame = gtk_frame_new("Line In Selector");
        gtk_widget_show(frame);
        gtk_box_pack_start(GTK_BOX(box), frame, FALSE, TRUE, 4);
        //gtk_container_set_border_width(GTK_CONTAINER(frame), 6);

        vbox = gtk_vbox_new(FALSE, 0);
        gtk_widget_show(vbox);
        gtk_container_add(GTK_CONTAINER(frame), vbox);
        gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

        radiobutton = gtk_radio_button_new_with_label(group, "Internal");
        input_interface_internal = radiobutton;
        group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
        gtk_widget_show(radiobutton);
        gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
        gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
                          (GtkSignalFunc)analog_input_select_toggled,
                          (gpointer)"Internal");

        radiobutton = gtk_radio_button_new_with_label(group, "Front Input");
        input_interface_front_input = radiobutton;
        group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
        gtk_widget_show(radiobutton);
        gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
        gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
                          (GtkSignalFunc)analog_input_select_toggled,
                          (gpointer)"Front Input");

        radiobutton = gtk_radio_button_new_with_label(group, "Rear Input");
        input_interface_rear_input = radiobutton;
        group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
        gtk_widget_show(radiobutton);
        gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
        gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
                          (GtkSignalFunc)analog_input_select_toggled,
                          (gpointer)"Rear Input");

        radiobutton = gtk_radio_button_new_with_label(group, "Wavetable");
        input_interface_wavetable = radiobutton;
        group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
        gtk_widget_show(radiobutton);
        gtk_box_pack_start(GTK_BOX(vbox), radiobutton, FALSE, FALSE, 0);
        gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
                          (GtkSignalFunc)analog_input_select_toggled,
                          (gpointer)"Wave Table");

        if(hide)
                gtk_widget_hide_all(frame);
}

static void create_hardware(GtkWidget *main, GtkWidget *notebook, int page)
{
	GtkWidget *label;
	GtkWidget *hbox;
	GtkWidget *hbox1;
	GtkWidget *hbox2;
	GtkWidget *vbox;
	GtkWidget *vbox1;
	GtkWidget *scrolledwindow;
	GtkWidget *viewport;
	GtkWidget *hseparator;

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(notebook), hbox);

	label = gtk_label_new("Hardware Settings");
	gtk_widget_show(label);
	gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), 
				   gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page), 
				   label);

	/* Build scrolling area */
	scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwindow);
	gtk_box_pack_start(GTK_BOX(hbox), scrolledwindow, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);

	viewport = gtk_viewport_new(NULL, NULL);
	gtk_widget_show(viewport);
	gtk_container_add(GTK_CONTAINER(scrolledwindow), viewport);

	/* Outer box */
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(viewport), hbox);

	/* Create boxes for controls */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 6);

	hbox1 = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox1);
	gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 0);

	hseparator = gtk_hseparator_new();
	gtk_widget_show(hseparator);
	gtk_box_pack_start(GTK_BOX(vbox), hseparator, FALSE, FALSE, 2);

	hbox2 = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox2);
	gtk_box_pack_start(GTK_BOX(vbox), hbox2, FALSE, FALSE, 0);

	create_master_clock(hbox1);

	vbox1 = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox1);
	gtk_box_pack_start(GTK_BOX(hbox1), vbox1, FALSE, FALSE, 20);

	create_rate_state(vbox1);
	create_actual_rate(vbox1);
	create_volume_change(vbox1);
	create_input_interface(hbox2);
	create_phono_input(hbox2);
	create_spdif_input_select(hbox2);
	create_spdif_output_settings(hbox);
}

static void create_about(GtkWidget *main, GtkWidget *notebook, int page)
{
	GtkWidget *label;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *scrolledwindow;
	GtkWidget *viewport;

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(notebook), hbox);

        label = gtk_label_new("About");
        gtk_widget_show(label);
	gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), 
				   gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page), 
				   label);

	/* build scrolling area */
	scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwindow);
	gtk_box_pack_start(GTK_BOX(hbox), scrolledwindow, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);

	viewport = gtk_viewport_new(NULL, NULL);
	gtk_widget_show(viewport);
	gtk_container_add(GTK_CONTAINER(scrolledwindow), viewport);


	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(viewport), vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);


	/* Create text as labels */
	label = gtk_label_new("");
        gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 6);

	/* create first line */
	label = gtk_label_new("Envy24 Control Utility " VERSION);
        gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 6);

	/* create second line */
	label = gtk_label_new("A GTK Tool for Envy24 PCI Audio Chip");
        gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 6);


	/* create third line */
	label = gtk_label_new("Copyright(c) 2000 by Jaroslav Kysela <perex@perex.cz>");
        gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 6);

	label = gtk_label_new("");
        gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 6);
}

static void create_analog_volume(GtkWidget *main, GtkWidget *notebook, int page)
{
	GtkWidget *label;
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *frame;
	GtkObject *adj;
	GtkWidget *vscale;
	GtkWidget *radiobutton;
	GSList *group;
	GtkWidget *scrolledwindow;
	GtkWidget *viewport;
	int i, j;
	static char* dmx6fire_inputs[6] = {
		"CD In (L)",
		"CD In (R)",
		"Line  (L)",
		"Line  (R)",
		"Phono (L)",
		"Phono (R)"
	};
	static char* dmx6fire_outputs[6] = {
		"Front (L)",
		"Front (R)",
		"Rear (L)",
		"Rear (R)",
		"Centre",
		"LFE"
	};


	scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwindow);
	gtk_container_add(GTK_CONTAINER(notebook), scrolledwindow);

        label = gtk_label_new("Analog Volume");
        gtk_widget_show(label);
	gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), 
				   gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page), 
				   label);

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow), 
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
	viewport = gtk_viewport_new(NULL, NULL);
	gtk_widget_show(viewport);
	gtk_container_add(GTK_CONTAINER(scrolledwindow), viewport);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(viewport), hbox);

	/* create DAC */
	for(i = 0; i < envy_dac_volumes(); i++) {
		char name[32];
		sprintf(name, "DAC %d", i);
		frame = gtk_frame_new(name);
		gtk_widget_show(frame);
		//gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
		gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, TRUE, 0);
		gtk_container_set_border_width(GTK_CONTAINER(frame), 6);

		vbox = gtk_vbox_new(FALSE, 0);
		gtk_widget_show(vbox);
		gtk_container_add(GTK_CONTAINER(frame), vbox);
		gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

		/* Add friendly labels for DMX 6Fires */
		if(card_is_dmx6fire && (i < 6)){
			label = gtk_label_new(dmx6fire_outputs[i]);
			gtk_widget_show(label);
			gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 6);
		}

		adj = gtk_adjustment_new(0, -(envy_dac_max()), 0, 1, 16, 0);
		av_dac_volume_adj[i] = adj;
		vscale = gtk_vscale_new(GTK_ADJUSTMENT(adj));
		gtk_scale_set_draw_value(GTK_SCALE(vscale), FALSE);
		gtk_widget_show(vscale);
		gtk_box_pack_start(GTK_BOX(vbox), vscale, TRUE, TRUE, 6);
		gtk_scale_set_digits(GTK_SCALE(vscale), 0);
		gtk_signal_connect(GTK_OBJECT(adj), "value_changed",
				   GTK_SIGNAL_FUNC(dac_volume_adjust), 
				   (gpointer)(long)(i));

	        label = gtk_label_new("000");
	        av_dac_volume_label[i] =(GtkLabel *)label;
	        gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 6);


		if (i >= envy_dac_senses())
			continue;
		group = NULL;
		for (j = 0; j < envy_dac_sense_items(); j++) {
		  radiobutton = gtk_radio_button_new_with_label(group, 
								envy_dac_sense_enum_name(j));
			av_dac_sense_radio[i][j] = radiobutton;
			gtk_widget_show(radiobutton);
			gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
					  (GtkSignalFunc)dac_sense_toggled, 
					   (gpointer)(long)((i << 8) + j));
			gtk_box_pack_start(GTK_BOX(vbox), 
					    radiobutton, FALSE, TRUE, 0);
			group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
		}
	}

	/* create ADC */
	for (i = 0; i < envy_adc_volumes(); i++) {
		char name[32];
		sprintf(name, "ADC %d", i);
		frame = gtk_frame_new(name);
		gtk_widget_show(frame);
		//gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
		gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, TRUE, 0);
		gtk_container_set_border_width(GTK_CONTAINER(frame), 6);

		vbox = gtk_vbox_new(FALSE, 0);
		gtk_widget_show(vbox);
		gtk_container_add(GTK_CONTAINER(frame), vbox);
		gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

		/* Add friendly labels for DMX 6Fires */
		if(card_is_dmx6fire && (i < 6)){
			label = gtk_label_new(dmx6fire_inputs[i]);
			gtk_widget_show(label);
			gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 6);
		}

		adj = gtk_adjustment_new(0, -(envy_adc_max()), 0, 1, 16, 0);
		av_adc_volume_adj[i] = adj;
		vscale = gtk_vscale_new(GTK_ADJUSTMENT(adj));
		gtk_scale_set_draw_value(GTK_SCALE(vscale), FALSE);
		gtk_widget_show(vscale);
		gtk_box_pack_start(GTK_BOX(vbox), vscale, TRUE, TRUE, 6);
		gtk_scale_set_value_pos(GTK_SCALE(vscale), GTK_POS_BOTTOM);
		gtk_scale_set_digits(GTK_SCALE(vscale), 0);
		gtk_signal_connect(GTK_OBJECT(adj), "value_changed",
				   GTK_SIGNAL_FUNC(adc_volume_adjust), 
				   (gpointer)(long)(i));

	        label = gtk_label_new("000");
	        av_adc_volume_label[i] =(GtkLabel *)label;
	        gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 6);

		if (i >= envy_adc_senses())
			continue;
		group = NULL;
		for (j = 0; j < envy_adc_sense_items(); j++) {
			radiobutton = gtk_radio_button_new_with_label(group, 
								      envy_adc_sense_enum_name(j));
			av_adc_sense_radio[i][j] = radiobutton;
			gtk_widget_show(radiobutton);
			gtk_signal_connect(GTK_OBJECT(radiobutton), "toggled",
					  (GtkSignalFunc)adc_sense_toggled, 
					   (gpointer)(long)((i << 8) + j));
			gtk_box_pack_start(GTK_BOX(vbox), 
					    radiobutton, FALSE, TRUE, 0);
			group = gtk_radio_button_group(GTK_RADIO_BUTTON(radiobutton));
		}
	}

	/* create IPGA */
	for (i = 0; i < envy_ipga_volumes(); i++) {
		char name[32];
		sprintf(name, "IPGA %d", i);
		frame = gtk_frame_new(name);
		gtk_widget_show(frame);
		//gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
		gtk_box_pack_start(GTK_BOX(hbox), frame, FALSE, TRUE, 0);
		gtk_container_set_border_width(GTK_CONTAINER(frame), 6);

		vbox = gtk_vbox_new(FALSE, 0);
		gtk_widget_show(vbox);
		gtk_container_add(GTK_CONTAINER(frame), vbox);
		gtk_container_set_border_width(GTK_CONTAINER(vbox), 6);

		/* Add friendly labels for DMX 6Fires */
		if(card_is_dmx6fire && (i < 6)){
			label = gtk_label_new(dmx6fire_inputs[i]);
			gtk_widget_show(label);
			gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 6);
		}

		adj = gtk_adjustment_new(0, -36, 0, 1, 16, 0);
		av_ipga_volume_adj[i] = adj;
		vscale = gtk_vscale_new(GTK_ADJUSTMENT(adj));
		gtk_scale_set_draw_value(GTK_SCALE(vscale), FALSE);
		gtk_widget_show(vscale);
		gtk_box_pack_start(GTK_BOX(vbox), vscale, TRUE, TRUE, 6);
		gtk_scale_set_value_pos(GTK_SCALE(vscale), GTK_POS_BOTTOM);
		gtk_scale_set_digits(GTK_SCALE(vscale), 0);
		gtk_signal_connect(GTK_OBJECT(adj), "value_changed",
				   GTK_SIGNAL_FUNC(ipga_volume_adjust), 
				   (gpointer)(long)(i));

	        label = gtk_label_new("000");
	        av_ipga_volume_label[i] = (GtkLabel *)label;
	        gtk_widget_show(label);
		gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, TRUE, 6);
	}
}

int index_active_profile()
{
	gint index;
	gboolean found;

	found = FALSE;
	for (index = 0; index < MAX_PROFILES; index++)
	{
		if (active_button == profiles_toggle_buttons[index].toggle_button) {
			found = TRUE;
			break;
		}
	}

	if (found)
		return index;

	return NOTFOUND;
}

int delete_card_number(GtkWidget *delete_button)
{
	gint res;
	gint card_nr;
	gint index;

	if (!(GTK_TOGGLE_BUTTON (delete_button)->active))
		return EXIT_SUCCESS;

	card_nr = GTK_ADJUSTMENT (card_number_adj)->value;
	if ((card_nr < 0) || (card_nr >= MAX_CARD_NUMBERS)) {
		fprintf(stderr, "card number not in [0 ... %d]\n", MAX_CARD_NUMBERS - 1);
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON (delete_button), FALSE);
		return -EINVAL;
	}

	res = delete_card(card_number, profiles_file_name);
	if (res < 0) {
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON (delete_button), FALSE);
		return res;
	}
	if (card_nr == card_number) {
		for (index = 0; index < MAX_PROFILES; index++)
		{
			gtk_entry_set_text(GTK_ENTRY (profiles_toggle_buttons[index].entry), get_profile_name(index + 1, card_number, profiles_file_name));
		}
	}

	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON (delete_button), FALSE);

	return EXIT_SUCCESS;
}

int restore_active_profile(const gint profile_number)
{
	gint res;

	res = save_restore(ALSACTL_OP_RESTORE, profile_number, card_number, profiles_file_name, NULL);

	return res;
}

int save_active_profile(GtkWidget *save_button)
{
	gint res;
	gint index;

	if (!(GTK_TOGGLE_BUTTON (save_button)->active))
		return EXIT_SUCCESS;
	if ((index = index_active_profile()) >= 0) {
		res = save_restore(ALSACTL_OP_STORE, index + 1, card_number, profiles_file_name, \
			gtk_entry_get_text(GTK_ENTRY (profiles_toggle_buttons[index].entry)));
	} else {
		fprintf(stderr, "No active profile found.\n");
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON (save_button), FALSE);
		return -EXIT_FAILURE;
	}

	gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON (save_button), FALSE);

	return res;
}

void entry_toggle_editable(GtkWidget *toggle_button, GtkWidget *entry)
{
	gint index;
	gint profile_number;

	if (active_button == toggle_button) {
		gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON (toggle_button), TRUE);
		gtk_editable_set_editable(GTK_EDITABLE (entry), TRUE);
		gtk_widget_grab_focus(entry);
		return;
	} else if (GTK_TOGGLE_BUTTON (toggle_button)->active) {
		active_button = toggle_button;
	}
	gtk_editable_set_editable(GTK_EDITABLE (entry), GTK_TOGGLE_BUTTON (toggle_button)->active);
	if (GTK_TOGGLE_BUTTON (toggle_button)->active) {
		gtk_widget_grab_focus(entry);
		profile_number = NOTFOUND;
		for (index = 0; index < MAX_PROFILES; index++)
		{
			if (profiles_toggle_buttons[index].toggle_button != toggle_button) {
				gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON (profiles_toggle_buttons[index].toggle_button), FALSE);
			} else {
				profile_number = index + 1;
			}
		}
		if (profile_number >= 0)
			restore_active_profile(profile_number);
	}
}

void enter_callback( const GtkWidget *widget, const GtkWidget *entry )
{
	const gchar *entry_text;
	entry_text = gtk_entry_get_text (GTK_ENTRY (entry));
	printf("Inhalt : %s\n", entry_text);
}

static GtkWidget *toggle_button_entry(const GtkWidget *parent, const gchar *profile_name, const gint index)
{
	GtkWidget *box;
	GtkWidget *entry_label;
	GtkWidget *toggle_button;

	box = gtk_hbox_new(FALSE, 0);

	toggle_button = gtk_toggle_button_new();
	gtk_container_border_width(GTK_CONTAINER(toggle_button), 3);

	profiles_toggle_buttons[index].entry = entry_label = gtk_entry_new();
	gtk_entry_set_max_length(GTK_ENTRY (entry_label), MAX_PROFILE_NAME_LENGTH);
	gtk_entry_set_text(GTK_ENTRY (entry_label), profile_name);
	/* only the active profile can be modified */
	gtk_editable_set_editable(GTK_EDITABLE (entry_label), FALSE);
	gtk_signal_connect(GTK_OBJECT (entry_label), "activate",
			 GTK_SIGNAL_FUNC (enter_callback),
			 (gpointer) entry_label);
	gtk_signal_connect(GTK_OBJECT (toggle_button), "toggled",
			 GTK_SIGNAL_FUNC (entry_toggle_editable),
			 (gpointer) entry_label);

	gtk_box_pack_start(GTK_BOX (box), entry_label, FALSE, FALSE, 20);
	gtk_widget_show(entry_label);
	gtk_widget_show(box);
	gtk_container_add(GTK_CONTAINER (toggle_button), box);
	gtk_widget_show(toggle_button);
	return (toggle_button);
}

static void create_profiles(GtkWidget *main, GtkWidget *notebook, int page)
{
	GtkWidget *label;
	GtkWidget *label_card_nr;
	GtkWidget *vbox1;
	GtkWidget *vbox2;
	GtkWidget *hbox;
	GtkWidget *hbox1;
	GtkWidget *save_button;
	GtkWidget *delete_button;
	GtkObject *card_button_adj;
	GtkWidget *card_button;
	GtkWidget *scrolledwindow;
	GtkWidget *viewport;
	gint index;
	gint profile_number;
	gchar *profile_name;
	gint max_profiles;
	gint max_digits;

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(notebook), hbox);


        label = gtk_label_new("Profiles");
        gtk_widget_show(label);
	gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook), 
				   gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page), 
				   label);

	/* build scrolling area */
	scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwindow);
	gtk_box_pack_start(GTK_BOX(hbox), scrolledwindow, TRUE, TRUE, 0);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);

	viewport = gtk_viewport_new(NULL, NULL);
	gtk_widget_show(viewport);
	gtk_container_add(GTK_CONTAINER(scrolledwindow), viewport);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(viewport), hbox);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 0);


	/* Create button boxes */
	vbox1 = gtk_vbutton_box_new();

	gtk_vbutton_box_set_spacing_default(0);
	for (index = 0; index < MAX_PROFILES; index++)	{
		profile_name = get_profile_name(index + 1, card_number, profiles_file_name);
		profiles_toggle_buttons[index].toggle_button = toggle_button_entry(window, profile_name, index);
		gtk_box_pack_start(GTK_BOX (vbox1), profiles_toggle_buttons[index].toggle_button, FALSE, FALSE, 0);
	}
	gtk_widget_show(vbox1);
	gtk_container_border_width(GTK_CONTAINER(vbox1), 6);

	vbox2 = gtk_vbutton_box_new();
	gtk_widget_show(vbox2);
	gtk_container_border_width(GTK_CONTAINER(vbox2), 50);

	hbox1 = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox1);
	gtk_box_pack_start(GTK_BOX(vbox2), hbox1, FALSE, FALSE, 20);

        label_card_nr = gtk_label_new("Card Number:");
        gtk_widget_show(label_card_nr);
	gtk_box_pack_start(GTK_BOX(hbox1), label_card_nr, FALSE, FALSE, 20);
	gtk_label_set_justify(GTK_LABEL(label_card_nr), GTK_JUSTIFY_LEFT);

	card_button_adj = gtk_adjustment_new(16, 0, MAX_CARD_NUMBERS - 1, 1, 10, 10);
	card_number_adj = card_button_adj;
	card_button = gtk_spin_button_new(GTK_ADJUSTMENT (card_button_adj), 1, 0);
	gtk_widget_show(card_button);
	gtk_box_pack_start(GTK_BOX (hbox1), card_button, TRUE, FALSE, 0);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON (card_button), TRUE);
	gtk_adjustment_set_value(GTK_ADJUSTMENT (card_button_adj), card_number);

	delete_button = gtk_toggle_button_new_with_label("Delete card from profiles");
	gtk_widget_show(delete_button);
	gtk_box_pack_start(GTK_BOX (vbox2), delete_button, FALSE, FALSE, 20);
	gtk_signal_connect(GTK_OBJECT (delete_button), "toggled",
			 GTK_SIGNAL_FUNC (delete_card_number),
			 NULL);

	save_button = gtk_toggle_button_new_with_label("Save active profile");
	gtk_widget_show(save_button);
	gtk_box_pack_end(GTK_BOX (vbox2), save_button, FALSE, FALSE, 20);
	gtk_signal_connect(GTK_OBJECT (save_button), "toggled",
			 GTK_SIGNAL_FUNC (save_active_profile),
			 NULL);

	gtk_container_add(GTK_CONTAINER(hbox), vbox1);
	gtk_container_add(GTK_CONTAINER(hbox), vbox2);

	if (default_profile != NULL)
	{
		/*
		 * only if default_profile is numerical and lower or equal than MAX_PROFILES it will be a profile_number
		 * otherwise it will be a profile name
		 */
		if (strcspn(default_profile, "0123456789") == 0) {
			for (max_profiles = MAX_PROFILES, max_digits = 0; max_profiles > 9; max_digits++, max_profiles /= 10)
				;
			max_digits++;
			if (strlen(default_profile) <= max_digits) {
				profile_number = atoi(default_profile);
				if (profile_number < 1 || profile_number > MAX_PROFILES)
					profile_number = get_profile_number(default_profile, card_number, profiles_file_name);
			} else {
				profile_number = get_profile_number(default_profile, card_number, profiles_file_name);
			}
		} else {
			profile_number = get_profile_number(default_profile, card_number, profiles_file_name);
		}
		if ((profile_number > 0) && (profile_number <= MAX_PROFILES)) {
			gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON (profiles_toggle_buttons[profile_number - 1].toggle_button), TRUE);
		} else {
			fprintf(stderr, "Cannot find profile '%s' for card '%d'.\n", default_profile, card_number);
		}
	}
}

static void create_outer(GtkWidget *main)
{
        GtkWidget *hbox1;
        GtkWidget *vbox;

	GtkWidget *label;
	GtkWidget *frame;
	GtkWidget *drawing;

	/* Create digital mixer frame */
	vbox = gtk_vbox_new(FALSE, 1);
	gtk_widget_show(vbox);
	gtk_box_pack_start(GTK_BOX(main), vbox, FALSE, FALSE, 0);

	label = gtk_label_new(" Rt-clk Menu >>");
	//gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 3);
	frame = gtk_frame_new("Digital Mixer");
	gtk_widget_show(frame);
	gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 6);

	/* Create controls in the digital mixer frame */

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(frame), vbox);	

	hbox1 = gtk_hbox_new(FALSE, 0);
	gtk_widget_show(hbox1);
	gtk_box_pack_start(GTK_BOX(vbox), hbox1, FALSE, FALSE, 6);

	drawing = gtk_drawing_area_new();
	mixer_mix_drawing = drawing;
	gtk_widget_set_name(drawing, "DigitalMixer");
	gtk_box_pack_start(GTK_BOX(hbox1), drawing, TRUE, FALSE, 6);
	if (tall_equal_mixer_ht > 1 ) {
		gtk_widget_set_usize(drawing, 60, 264 + 60 * (tall_equal_mixer_ht - 1));
	} else {
		gtk_widget_set_usize(drawing, 60, 264);
	}
	gtk_signal_connect(GTK_OBJECT(drawing), "expose_event",
			   (GtkSignalFunc)level_meters_expose_event, NULL);
	gtk_signal_connect(GTK_OBJECT(drawing), "configure_event",
			   (GtkSignalFunc)level_meters_configure_event, NULL);
	gtk_widget_set_events(drawing, GDK_EXPOSURE_MASK);
	gtk_widget_show(drawing);

	hbox1 = gtk_hbox_new(TRUE, 0);
	gtk_widget_show(hbox1);
	gtk_box_pack_start(GTK_BOX(vbox), hbox1, TRUE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox1), 6);

	label = gtk_label_new("Left");
	gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox1), label, FALSE, TRUE, 0);

	label = gtk_label_new("Right");
	gtk_misc_set_alignment(GTK_MISC(label), 1, 0.5);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox1), label, FALSE, TRUE, 0);


	mixer_clear_peaks_button = gtk_button_new_with_label("Reset Peaks");
	gtk_widget_show(mixer_clear_peaks_button);
	gtk_box_pack_start(GTK_BOX(vbox), mixer_clear_peaks_button, TRUE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(mixer_clear_peaks_button), 4);
	gtk_signal_connect(GTK_OBJECT(mixer_clear_peaks_button), "clicked",
			   GTK_SIGNAL_FUNC(level_meters_reset_peaks), NULL);
}/* End create_outer  */

static void create_blank(GtkWidget *main, GtkWidget *notebook, int page)
{
/*	This is a little workaround for a problem with the pop-up menu.
	For some reason the label of the last page is not accessed by the menu
	so all it shows is 'page 7'.  Here a blank extra page is created, unseen,
	which seems to satisfy gtk, and we see the menu last page label correct. AH 12.7.2005 */

	GtkWidget *label;
	GtkWidget *hbox;

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(notebook), hbox);

        label = gtk_label_new("Blank");
	gtk_notebook_set_tab_label(GTK_NOTEBOOK(notebook),
				   gtk_notebook_get_nth_page(GTK_NOTEBOOK(notebook), page),
				   label);
}

static void usage(void)
{
	fprintf(stderr, "usage: envy24control [-c card#] [-D control-name] [-o num-outputs] [-i num-inputs] [-p num-pcm-outputs] [-s num-spdif-in/outs] [-v] [-f profiles-file] [profile name|profile id] [-m channel-num] [-w initial-window-width] [-t height-num]\n");
	fprintf(stderr, "\t-c, --card\tAlsa card number to control\n");
	fprintf(stderr, "\t-D, --device\tcontrol-name\n");
	fprintf(stderr, "\t-o, --outputs\tLimit number of analog line outputs to display\n");
	fprintf(stderr, "\t-i, --input\tLimit number of analog line inputs to display\n");
	fprintf(stderr, "\t-p, --pcm_output\tLimit number of PCM outputs to display\n");
	fprintf(stderr, "\t-s, --spdif\tLimit number of spdif inputs/outputs to display\n");
	fprintf(stderr, "\t-v, --view_spdif_playback\tshows the spdif playback channels in the mixer\n");
	fprintf(stderr, "\t-f, --profiles_file\tuse file as profiles file\n");
	fprintf(stderr, "\t-m, --midichannel\tmidi channel number for controller control\n");
	fprintf(stderr, "\t-M, --midienhanced\tUse an enhanced mapping from midi controller to db slider\n");
	fprintf(stderr, "\t-w, --window_width\tSet initial window width (try 2,6 or 8; 280,626, or 968)\n");
	fprintf(stderr, "\t-t, --tall_eq_mixer_heights\tSet taller height mixer displays (1-9)\n");
}

int main(int argc, char **argv)
{
        GtkWidget *notebook;
	GtkWidget *outerbox;
        char *name, tmpname[8], title[128];
	int i, c, err;
	snd_ctl_card_info_t *hw_info;
	snd_ctl_elem_value_t *val;
	int npfds;
	struct pollfd *pfds;
	int midi_fd = -1, midi_channel = -1, midi_enhanced = 0;
	int page;
	int input_channels_set = 0;
	int output_channels_set = 0;
	int pcm_output_channels_set = 0;
	int width_val;
	int wwidth_set =FALSE;
	int wwidth = 796;
	const int chanwidth = 86;
	const int fixwidth = 108;

	static struct option long_options[] = {
		{"device", 1, 0, 'D'},
		{"card", 1, 0, 'c'},
		{"profiles_file", 1, 0, 'f'},
		{"inputs", 1, 0, 'i'},
		{"midichannel", 1, 0, 'm'},
		{"midienhanced", 0, 0, 'M'},
		{"outputs", 1, 0, 'o'},
		{"pcm_outputs", 1, 0, 'p'},
		{"spdif", 1, 0, 's'},
		{"window_width", 1, 0, 'w'},
		{"view_spdif_playback", 0, 0, 'v'},
		{"tall_eq_mixer_heights", 1, 0, 't'},
		{ NULL }
	};


	snd_ctl_card_info_alloca(&hw_info);
	snd_ctl_elem_value_alloca(&val);

	/* Go through gtk initialization */
        gtk_init(&argc, &argv);

	name = NULL; /* probe */
	card_number = 0;
	input_channels = MAX_INPUT_CHANNELS;
	output_channels = MAX_OUTPUT_CHANNELS;
	pcm_output_channels = MAX_PCM_OUTPUT_CHANNELS;
	spdif_channels = MAX_SPDIF_CHANNELS;
	view_spdif_playback = 0;
	profiles_file_name = DEFAULT_PROFILERC;
	default_profile = NULL;
	while ((c = getopt_long(argc, argv, "D:c:f:i:m:Mo:p:s:w:vt:", long_options, NULL)) != -1) {
		switch (c) {
		case 'D':
			name = optarg;
			card_number = atoi(strchr(name, ':') + sizeof(char));
			if (card_number < 0 || card_number >= MAX_CARD_NUMBERS) {
				fprintf(stderr, "envy24control: invalid card number %d\n", card_number);
				exit(1);
			}
			break;
		case 'c':
			i = atoi(optarg);
			if (i < 0 || i >= MAX_CARD_NUMBERS) {
				fprintf(stderr, "envy24control: invalid card number %d\n", i);
				exit(1);
			}
			card_number = i;
			sprintf(tmpname, "hw:%d", i);
			name = tmpname;
			break;
		case 'f':
			profiles_file_name = optarg;
			break;
		case 'i':
			input_channels = atoi(optarg);
			if (input_channels < 0 || input_channels > MAX_INPUT_CHANNELS) {
				fprintf(stderr, "envy24control: must have 0-%i inputs\n", MAX_INPUT_CHANNELS);
				exit(1);
			}
			input_channels_set = 1;
			break;
		case 'm':
			midi_channel = atoi(optarg);
			if (midi_channel < 1 || midi_channel > 16) {
				fprintf(stderr, "envy24control: invalid midi channel number %i\n", midi_channel);
				exit(1);
			}
			--midi_channel;
			break;
		case 'M': midi_enhanced = 1; break;
		case 'o':
			output_channels = atoi(optarg);
			if (output_channels < 0 || output_channels > MAX_OUTPUT_CHANNELS) {
				fprintf(stderr, "envy24control: must have 0-%i outputs\n", MAX_OUTPUT_CHANNELS);
				exit(1);
			}
			output_channels_set = 1;
			break;
		case 'p':
			pcm_output_channels = atoi(optarg);
			if (pcm_output_channels < 0 || pcm_output_channels > MAX_PCM_OUTPUT_CHANNELS) {
				fprintf(stderr, "envy24control: must have 0-%i pcm outputs\n", MAX_PCM_OUTPUT_CHANNELS);
				exit(1);
			}
			pcm_output_channels_set = 1;
			break;
		case 's':
			spdif_channels = atoi(optarg);
			if (spdif_channels < 0 || spdif_channels > MAX_SPDIF_CHANNELS) {
				fprintf(stderr, "envy24control: must have 0-%i spdifs\n", MAX_SPDIF_CHANNELS);
				exit(1);
			}
			break;
		case 'w':
			width_val = atoi(optarg);
			if ((width_val >= 1) && (width_val <= 20)) {
				wwidth = (width_val * chanwidth + fixwidth);
			} else {
				wwidth = width_val;
			}
			wwidth_set = TRUE;
			break;
		case 'v':
			view_spdif_playback = 1;
			break;
		case 't':
			tall_equal_mixer_ht = atoi(optarg);
			if ((tall_equal_mixer_ht < 0) || (tall_equal_mixer_ht >= 10))
				tall_equal_mixer_ht = 0;
			break;
		default:
			usage();
			exit(1);
			break;
		}
	}
	if (optind < argc) {
		default_profile = argv[optind];
	}

	if (! name) {
		/* probe cards */
		static char cardname[8];
		/* FIXME: hardcoded max number of cards */
		for (card_number = 0; card_number < 8; card_number++) {
			sprintf(cardname, "hw:%d", card_number);
			if (snd_ctl_open(&ctl, cardname, 0) < 0)
				continue;
			if (snd_ctl_card_info(ctl, hw_info) < 0 ||
			    strcmp(snd_ctl_card_info_get_driver(hw_info), "ICE1712")) {
				snd_ctl_close(ctl);
				continue;
			}
			/* found */
			name = cardname;
			break;
		}
		if (! name) {
			fprintf(stderr, "No ICE1712 cards found\n");
			exit(EXIT_FAILURE);
		}
	} else {
		if ((err = snd_ctl_open(&ctl, name, 0)) < 0) {
			fprintf(stderr, "snd_ctl_open: %s\n", snd_strerror(err));
			exit(EXIT_FAILURE);
		}
		if ((err = snd_ctl_card_info(ctl, hw_info)) < 0) {
			fprintf(stderr, "snd_ctl_card_info: %s\n", snd_strerror(err));
			exit(EXIT_FAILURE);
		}
		if (strcmp(snd_ctl_card_info_get_driver(hw_info), "ICE1712")) {
			fprintf(stderr, "invalid card type (driver is %s)\n", snd_ctl_card_info_get_driver(hw_info));
			exit(EXIT_FAILURE);
		}
	}

	snd_ctl_elem_value_set_interface(val, SND_CTL_ELEM_IFACE_CARD);
	snd_ctl_elem_value_set_name(val, "ICE1712 EEPROM");
	if ((err = snd_ctl_elem_read(ctl, val)) < 0) {
		fprintf(stderr, "Unable to read EEPROM contents: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	memcpy(&card_eeprom, snd_ctl_elem_value_get_bytes(val), 32);

	if(card_eeprom.subvendor == ICE1712_SUBDEVICE_DMX6FIRE)
		card_is_dmx6fire = TRUE;

	/* Set a better default for input_channels and output_channels */
	if(!input_channels_set)
		if(card_is_dmx6fire)
			input_channels = 6;

	if(!output_channels_set)
		if(card_is_dmx6fire)
			output_channels = 6;

	if(!pcm_output_channels_set)
		if(card_is_dmx6fire)
			pcm_output_channels = 6; /* PCMs 7&8 can be used -set using option -p8 */

	if (!wwidth_set)
		if (card_is_dmx6fire)
			wwidth = 626;


	/* Initialize code */
	config_open();
	level_meters_init();
	mixer_init();
	patchbay_init();
	hardware_init();
	analog_volume_init();
	if (midi_channel >= 0)
		midi_fd = midi_init(argv[0], midi_channel, midi_enhanced);

	fprintf(stderr, "using\t --- input_channels: %i\n\t --- output_channels: %i\n\t --- pcm_output_channels: %i\n\t --- spdif in/out channels: %i\n", \
		input_channels, output_channels, pcm_output_channels, spdif_channels);

        /* Make the title */
        sprintf(title, "Envy24 Control Utility %s (%s)", VERSION, snd_ctl_card_info_get_longname(hw_info));

        /* Create the main window */
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), title);
        gtk_signal_connect(GTK_OBJECT (window), "delete_event", 
                           (GtkSignalFunc) gtk_main_quit, NULL);
        signal(SIGINT, (void *)gtk_main_quit);

	gtk_window_set_default_size(GTK_WINDOW(window), wwidth, 300);

	outerbox = gtk_hbox_new(FALSE, 3);
	gtk_widget_show(outerbox);
	gtk_container_add(GTK_CONTAINER(window), outerbox);

	create_outer(outerbox);

        /* Create the notebook */
        notebook = gtk_notebook_new();
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
	gtk_notebook_popup_enable(GTK_NOTEBOOK(notebook));
        gtk_widget_show(notebook);
	gtk_container_add(GTK_CONTAINER(outerbox), notebook);

	page = 0;

	create_inputs_mixer(outerbox, notebook, page++);
	create_pcms_mixer(outerbox, notebook, page++);
	create_router(outerbox, notebook, page++);
	create_hardware(outerbox, notebook, page++);
	if (envy_analog_volume_available())
		create_analog_volume(outerbox, notebook, page++);
	create_profiles(outerbox, notebook, page++);
	create_about(outerbox, notebook, page++);
	create_blank(outerbox, notebook, page++);

	npfds = snd_ctl_poll_descriptors_count(ctl);
	if (npfds > 0) {
		pfds = alloca(sizeof(*pfds) * npfds);
		npfds = snd_ctl_poll_descriptors(ctl, pfds, npfds);
		for (i = 0; i < npfds; i++)
			gdk_input_add(pfds[i].fd,
				      GDK_INPUT_READ,
				      control_input_callback,
				      ctl);
		snd_ctl_subscribe_events(ctl, 1);
	}
	if (midi_fd >= 0) {
		gdk_input_add(midi_fd, GDK_INPUT_READ, midi_process, NULL);
	}
	gtk_timeout_add(40, level_meters_timeout_callback, NULL);
	gtk_timeout_add(100, master_clock_status_timeout_callback, NULL);
	gtk_timeout_add(100, internal_clock_status_timeout_callback, NULL);
	gtk_timeout_add(100, rate_locking_status_timeout_callback, NULL);
	gtk_timeout_add(100, rate_reset_status_timeout_callback, NULL);


	gtk_widget_show(window);

	level_meters_postinit();
	mixer_postinit();
	patchbay_postinit();	
	hardware_postinit();
	analog_volume_postinit();

	gtk_main();

	snd_ctl_close(ctl);
	midi_close();
	config_close();

	return EXIT_SUCCESS;
}
