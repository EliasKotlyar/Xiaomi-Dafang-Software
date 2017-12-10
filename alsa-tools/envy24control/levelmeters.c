/*****************************************************************************
   levelmeters.c - Stereo level meters
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

static GdkGC *penGreenShadow[21] = { NULL, };
static GdkGC *penGreenLight[21] = { NULL, };
static GdkGC *penOrangeShadow[21] = { NULL, };
static GdkGC *penOrangeLight[21] = { NULL, };
static GdkGC *penRedShadow[21] = { NULL, };
static GdkGC *penRedLight[21] = { NULL, };
static GdkPixmap *pixmap[21] = { NULL, };
static snd_ctl_elem_value_t *peaks;

extern int input_channels, output_channels, pcm_output_channels, spdif_channels, view_spdif_playback;

static void update_peak_switch(void)
{
	int err;

	if ((err = snd_ctl_elem_read(ctl, peaks)) < 0)
		g_print("Unable to read peaks: %s\n", snd_strerror(err));
}

static void get_levels(int idx, int *l1, int *l2)
{
	*l1 = *l2 = 0;
	
	if (idx == 0) {
		*l1 = snd_ctl_elem_value_get_integer(peaks, 20);
		*l2 = snd_ctl_elem_value_get_integer(peaks, 21);
	} else {
		*l1 = *l2 = snd_ctl_elem_value_get_integer(peaks, idx - 1);
	}
}

static GdkGC *get_pen(int idx, int nRed, int nGreen, int nBlue)
{
	GdkColor *c;
	GdkGC *gc;
	
	c = (GdkColor *)g_malloc(sizeof(GdkColor));
	c->red = nRed;
	c->green = nGreen;
	c->blue = nBlue;
	gdk_color_alloc(gdk_colormap_get_system(), c);
	gc = gdk_gc_new(pixmap[idx]);
	gdk_gc_set_foreground(gc, c);
	return gc;
}

static int get_index(gchar *name)
{
	int result;

	if (!strcmp(name, "DigitalMixer"))
		return 0;
	result = atoi(name + 5);
	if (result < 1 || result > 20) {
		g_print("Wrong drawing area ID: %s\n", name);
		gtk_main_quit();
	}
	return result;
}

static void redraw_meters(int idx, int width, int height, int level1, int level2)
{
	int stereo = idx == 0;
	int segment_width = stereo ? (width / 2) - 8 : width - 12;
	int segments = (height - 6) / 4;
	int green_segments = (segments / 4) * 3;
	int red_segments = 2;
	int orange_segments = segments - green_segments - red_segments;
	int seg;
	int segs_on1 = ((segments * level1) + 128) / 255;
	int segs_on2 = ((segments * level2) + 128) / 255;

	// g_print("segs_on1 = %i (%i), segs_on2 = %i (%i)\n", segs_on1, level1, segs_on2, level2);
	for (seg = 0; seg < green_segments; seg++) {
		gdk_draw_rectangle(pixmap[idx],
				   segs_on1 > 0 ? penGreenLight[idx] : penGreenShadow[idx],
				   TRUE,
				   6, 3 + ((segments - seg - 1) * 4),
				   segment_width,
				   3);
		if (stereo)
			gdk_draw_rectangle(pixmap[idx],
					   segs_on2 > 0 ? penGreenLight[idx] : penGreenShadow[idx],
					   TRUE,
					   2 + (width / 2),
					   3 + ((segments - seg - 1) * 4),
					   segment_width,
					   3);
		segs_on1--;
		segs_on2--;
	}
	for (seg = green_segments; seg < green_segments + orange_segments; seg++) {
		gdk_draw_rectangle(pixmap[idx],
				   segs_on1 > 0 ? penOrangeLight[idx] : penOrangeShadow[idx],
				   TRUE,
				   6, 3 + ((segments - seg - 1) * 4),
				   segment_width,
				   3);
		if (stereo)
			gdk_draw_rectangle(pixmap[idx],
					   segs_on2 > 0 ? penOrangeLight[idx] : penOrangeShadow[idx],
					   TRUE,
					   2 + (width / 2),
					   3 + ((segments - seg - 1) * 4),
					   segment_width,
					   3);
		segs_on1--;
		segs_on2--;
	}
	for (seg = green_segments + orange_segments; seg < segments; seg++) {
		gdk_draw_rectangle(pixmap[idx],
				   segs_on1 > 0 ? penRedLight[idx] : penRedShadow[idx],
				   TRUE,
				   6, 3 + ((segments - seg - 1) * 4),
				   segment_width,
				   3);
		if (stereo)
			gdk_draw_rectangle(pixmap[idx],
					   segs_on2 > 0 ? penRedLight[idx] : penRedShadow[idx],
					   TRUE,
					   2 + (width / 2),
					   3 + ((segments - seg - 1) * 4),
					   segment_width,
					   3);
		segs_on1--;
		segs_on2--;
	}
}

gint level_meters_configure_event(GtkWidget *widget, GdkEventConfigure *event)
{
	int idx = get_index(gtk_widget_get_name(widget));

	if (pixmap[idx] != NULL)
		gdk_pixmap_unref(pixmap[idx]);
	pixmap[idx] = gdk_pixmap_new(widget->window,
				     widget->allocation.width,
				     widget->allocation.height,
				     -1);
	penGreenShadow[idx] = get_pen(idx, 0, 0x77ff, 0);
	penGreenLight[idx] = get_pen(idx, 0, 0xffff, 0);
	penOrangeShadow[idx] = get_pen(idx, 0xddff, 0x55ff, 0);
	penOrangeLight[idx] = get_pen(idx, 0xffff, 0x99ff, 0);
	penRedShadow[idx] = get_pen(idx, 0xaaff, 0, 0);
	penRedLight[idx] = get_pen(idx, 0xffff, 0, 0);
	gdk_draw_rectangle(pixmap[idx],
			   widget->style->black_gc,
			   TRUE,
			   0, 0,
			   widget->allocation.width,
			   widget->allocation.height);
	// g_print("configure: %i:%i\n", widget->allocation.width, widget->allocation.height);
	redraw_meters(idx, widget->allocation.width, widget->allocation.height, 0, 0);
	return TRUE;
}

gint level_meters_expose_event(GtkWidget *widget, GdkEventExpose *event)
{
	int idx = get_index(gtk_widget_get_name(widget));
	int l1, l2;
	
	get_levels(idx, &l1, &l2);
	redraw_meters(idx, widget->allocation.width, widget->allocation.height, l1, l2);
	gdk_draw_pixmap(widget->window,
			widget->style->black_gc,
			pixmap[idx],
			event->area.x, event->area.y,
			event->area.x, event->area.y,
			event->area.width, event->area.height);
	return FALSE;
}

gint level_meters_timeout_callback(gpointer data)
{
	GtkWidget *widget;
	int idx, l1, l2;

	update_peak_switch();
	for (idx = 0; idx <= pcm_output_channels; idx++) {
		get_levels(idx, &l1, &l2);
		widget = idx == 0 ? mixer_mix_drawing : mixer_drawing[idx-1];
		if (GTK_WIDGET_VISIBLE(widget) && (pixmap[idx] != NULL)) {
			redraw_meters(idx, widget->allocation.width, widget->allocation.height, l1, l2);
			gdk_draw_pixmap(widget->window,
					widget->style->black_gc,
					pixmap[idx],
					0, 0,
					0, 0,
					widget->allocation.width, widget->allocation.height);
		}
	}
	if (view_spdif_playback) {
		for (idx = MAX_PCM_OUTPUT_CHANNELS + 1; idx <= MAX_OUTPUT_CHANNELS + spdif_channels; idx++) {
			get_levels(idx, &l1, &l2);
			widget = idx == 0 ? mixer_mix_drawing : mixer_drawing[idx-1];
			if (GTK_WIDGET_VISIBLE(widget) && (pixmap[idx] != NULL)) {
				redraw_meters(idx, widget->allocation.width, widget->allocation.height, l1, l2);
				gdk_draw_pixmap(widget->window,
						widget->style->black_gc,
						pixmap[idx],
						0, 0,
						0, 0,
						widget->allocation.width, widget->allocation.height);
			}
		}
	}
	for (idx = MAX_PCM_OUTPUT_CHANNELS + MAX_SPDIF_CHANNELS + 1; idx <= input_channels + MAX_PCM_OUTPUT_CHANNELS + MAX_SPDIF_CHANNELS; idx++) {
		get_levels(idx, &l1, &l2);
		widget = idx == 0 ? mixer_mix_drawing : mixer_drawing[idx-1];
		if (GTK_WIDGET_VISIBLE(widget) && (pixmap[idx] != NULL)) {
			redraw_meters(idx, widget->allocation.width, widget->allocation.height, l1, l2);
			gdk_draw_pixmap(widget->window,
					widget->style->black_gc,
					pixmap[idx],
					0, 0,
					0, 0,
					widget->allocation.width, widget->allocation.height);
		}
	}
	for (idx = MAX_PCM_OUTPUT_CHANNELS + MAX_SPDIF_CHANNELS + MAX_INPUT_CHANNELS + 1; \
		    idx <= spdif_channels + MAX_PCM_OUTPUT_CHANNELS + MAX_SPDIF_CHANNELS + MAX_INPUT_CHANNELS; idx++) {
		get_levels(idx, &l1, &l2);
		widget = idx == 0 ? mixer_mix_drawing : mixer_drawing[idx-1];
		if (GTK_WIDGET_VISIBLE(widget) && (pixmap[idx] != NULL)) {
			redraw_meters(idx, widget->allocation.width, widget->allocation.height, l1, l2);
			gdk_draw_pixmap(widget->window,
					widget->style->black_gc,
					pixmap[idx],
					0, 0,
					0, 0,
					widget->allocation.width, widget->allocation.height);
		}
	}
	return TRUE;
}

void level_meters_reset_peaks(GtkButton *button, gpointer data)
{
}

void level_meters_init(void)
{
	int err;

	snd_ctl_elem_value_malloc(&peaks);
	snd_ctl_elem_value_set_interface(peaks, SND_CTL_ELEM_IFACE_PCM);
	snd_ctl_elem_value_set_name(peaks, "Multi Track Peak");
	if ((err = snd_ctl_elem_read(ctl, peaks)) < 0)
		/* older ALSA driver, using MIXER type */
		snd_ctl_elem_value_set_interface(peaks,
			SND_CTL_ELEM_IFACE_MIXER);
}

void level_meters_postinit(void)
{
	level_meters_timeout_callback(NULL);
}
