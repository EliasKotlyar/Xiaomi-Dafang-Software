/*****************************************************************************
   patchbay.c - patchbay/router code
   Copyright (C) 2000 by Jaroslav Kysela <perex@perex.cz>
   
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

#define SPDIF_PLAYBACK_ROUTE_NAME	"IEC958 Playback Route"
#define ANALOG_PLAYBACK_ROUTE_NAME	"H/W Playback Route"

#define toggle_set(widget, state) \
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), state);

static int stream_active[MAX_OUTPUT_CHANNELS + MAX_SPDIF_CHANNELS];
extern int output_channels, input_channels, pcm_output_channels, spdif_channels;

static int is_active(GtkWidget *widget)
{
	return GTK_TOGGLE_BUTTON(widget)->active ? 1 : 0;
}

static int get_toggle_index(int stream)
{
	int err, out;
	snd_ctl_elem_value_t *val;

	stream--;
	if (stream < 0 || stream > 9) {
		g_print("get_toggle_index (1)\n");
		return 0;
	}
	snd_ctl_elem_value_alloca(&val);
	snd_ctl_elem_value_set_interface(val, SND_CTL_ELEM_IFACE_MIXER);
	if (stream >= MAX_OUTPUT_CHANNELS) {
		snd_ctl_elem_value_set_name(val, SPDIF_PLAYBACK_ROUTE_NAME);
		snd_ctl_elem_value_set_index(val, stream - MAX_OUTPUT_CHANNELS);
	} else {
		snd_ctl_elem_value_set_name(val, ANALOG_PLAYBACK_ROUTE_NAME);
		snd_ctl_elem_value_set_index(val, stream);
	}
	if ((err = snd_ctl_elem_read(ctl, val)) < 0)
		return 0;
	out = snd_ctl_elem_value_get_enumerated(val, 0);
	if (out >= MAX_INPUT_CHANNELS + MAX_SPDIF_CHANNELS + 1) {
		if (stream >= MAX_PCM_OUTPUT_CHANNELS || stream < MAX_SPDIF_CHANNELS)
			return 1; /* digital mixer */
	} else if (out >= MAX_INPUT_CHANNELS + 1)
		return out - (MAX_INPUT_CHANNELS + 1) + 2; /* spdif left (=2) / right (=3) */
	else if (out >= 1)
		return out + spdif_channels + 1; /* analog (4-) */

	return 0; /* pcm */
}

void patchbay_update(void)
{
	int stream, tidx;

	for (stream = 1; stream <= (MAX_OUTPUT_CHANNELS + MAX_SPDIF_CHANNELS); stream++) {
		if (stream_active[stream - 1]) {
			tidx = get_toggle_index(stream);
			toggle_set(router_radio[stream - 1][tidx], TRUE);
		}
	}
}

static void set_routes(int stream, int idx)
{
	int err;
	unsigned int out;
	snd_ctl_elem_value_t *val;

	stream--;
	if (stream < 0 || stream > 9) {
		g_print("set_routes (1)\n");
		return;
	}
	if (! stream_active[stream])
		return;
	out = 0;
	if (idx == 1)
		out = MAX_INPUT_CHANNELS + MAX_SPDIF_CHANNELS + 1;
	else if (idx == 2 || idx == 3)	/* S/PDIF left & right */
		out = idx + 7; /* 9-10 */
	else if (idx >= 4) /* analog */
		out = idx - 3; /* 1-8 */

	snd_ctl_elem_value_alloca(&val);
	snd_ctl_elem_value_set_interface(val, SND_CTL_ELEM_IFACE_MIXER);
	if (stream >= MAX_OUTPUT_CHANNELS) {
		snd_ctl_elem_value_set_name(val, SPDIF_PLAYBACK_ROUTE_NAME);
		snd_ctl_elem_value_set_index(val, stream - MAX_OUTPUT_CHANNELS);
	} else {
		snd_ctl_elem_value_set_name(val, ANALOG_PLAYBACK_ROUTE_NAME);
		snd_ctl_elem_value_set_index(val, stream);
	}

	snd_ctl_elem_value_set_enumerated(val, 0, out);
	if ((err = snd_ctl_elem_write(ctl, val)) < 0)
		g_print("Multi track route write error: %s\n", snd_strerror(err));
}

void patchbay_toggled(GtkWidget *togglebutton, gpointer data)
{
	int stream = (long)data >> 16;
	int what = (long)data & 0xffff;

	if (is_active(togglebutton))
		set_routes(stream, what);
}

int patchbay_stream_is_active(int stream)
{
	return stream_active[stream - 1];
}

void patchbay_init(void)
{
	int i;
	int nb_active_channels;
	snd_ctl_elem_value_t *val;

	snd_ctl_elem_value_alloca(&val);
	snd_ctl_elem_value_set_interface(val, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_value_set_name(val, ANALOG_PLAYBACK_ROUTE_NAME);
	memset (stream_active, 0, (MAX_OUTPUT_CHANNELS + MAX_SPDIF_CHANNELS) * sizeof(int));
	nb_active_channels = 0;
	for (i = 0; i < output_channels; i++) {
		snd_ctl_elem_value_set_numid(val, 0);
		snd_ctl_elem_value_set_index(val, i);
		if (snd_ctl_elem_read(ctl, val) < 0)
			continue;

		stream_active[i] = 1;
		nb_active_channels++;
	}
	output_channels = nb_active_channels;
	snd_ctl_elem_value_set_name(val, SPDIF_PLAYBACK_ROUTE_NAME);
	nb_active_channels = 0;
	for (i = 0; i < spdif_channels; i++) {
		snd_ctl_elem_value_set_numid(val, 0);
		snd_ctl_elem_value_set_index(val, i);
 		if (snd_ctl_elem_read(ctl, val) < 0)
			continue;
		stream_active[i + MAX_OUTPUT_CHANNELS] = 1;
		nb_active_channels++;
	}
	spdif_channels = nb_active_channels;
}

void patchbay_postinit(void)
{
	patchbay_update();
}
