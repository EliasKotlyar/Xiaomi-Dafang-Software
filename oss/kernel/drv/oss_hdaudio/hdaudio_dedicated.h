/*
 * Purpose: Definitions for dedicated HD audio codec drivers
 */
/*
 *
 * This file is part of Open Sound System.
 *
 * Copyright (C) 4Front Technologies 1996-2008.
 *
 * This this source file is released under GPL v2 license (no other versions).
 * See the COPYING file included in the main directory of this source
 * distribution for the license terms and conditions.
 *
 */
#ifndef HDAUDIO_DEDICATED_H
#define HDAUDIO_DEDICATED_H

extern void hda_codec_add_group(int dev, hdaudio_mixer_t * mixer, int cad, int *group, int parent_group, const char *name);
extern int hda_codec_add_pingroup(int dev, hdaudio_mixer_t * mixer, int cad, int wid, int *group, int top_group, int *parent_group, const char *name, int *n, const char *parent_name, int group_size);
extern int hda_codec_add_adcgroup(int dev, hdaudio_mixer_t * mixer, int cad, int wid, int *group, int top_group, int *parent_group, const char *name, int *n, const char *parent_name, int group_size);
extern int hda_codec_add_miscgroup(int dev, hdaudio_mixer_t * mixer, int cad, int wid, int *group, int top_group, int *parent_group, const char *name, int *n, const char *parent_name, int group_size);

#define HDA_GROUP(group, top_group, name) hda_codec_add_group(dev, mixer, cad, &group, top_group, name)
#define HDA_PIN_GROUP(wid, group, pin_group, name, n, parent_name, group_size) \
	hda_codec_add_pingroup(dev, mixer, cad, wid, &group, top_group, &pin_group, name, &n, parent_name, group_size)
#define HDA_ADC_GROUP(wid, group, rec_group, name, n, parent_name, group_size) \
	hda_codec_add_adcgroup(dev, mixer, cad, wid, &group, top_group, &rec_group, name, &n, parent_name, group_size)
#define HDA_MISC_GROUP(wid, group, misc_group, name, n, parent_name, group_size) \
	hda_codec_add_miscgroup(dev, mixer, cad, wid, &group, top_group, &misc_group, name, &n, parent_name, group_size)

#define UNMUTE	0
#define MUTE	1
#define RECORD	1
#define NOREC	0

extern int hda_codec_add_outamp(int dev, hdaudio_mixer_t * mixer, int cad, int wid, int group, const char *name, int percent, unsigned int flags);
extern int hda_codec_add_outmute(int dev, hdaudio_mixer_t * mixer, int cad, int wid, int group, const char *name, int state);
#define HDA_OUTAMP(wid, group, name, percent) ctl=hda_codec_add_outamp(dev, mixer, cad, wid, group, name, percent, 0)
#define HDA_OUTAMP_F(wid, group, name, percent, flags) hda_codec_add_outamp(dev, mixer, cad, wid, group, name, percent, flags)
#define HDA_OUTMUTE(wid, group, name, muted) ctl=hda_codec_add_outmute(dev, mixer, cad, wid, group, name, muted)

extern int hda_codec_add_inamp(int dev, hdaudio_mixer_t * mixer, int cad, int wid, int ix, int group, const char *name, int percent, unsigned int flags);
extern int hda_codec_add_inmute(int dev, hdaudio_mixer_t * mixer, int cad, int wid, int ix, int group, const char *name, int muted, int flags);
extern int hda_codec_set_inmute(int dev, hdaudio_mixer_t * mixer, int cad, int wid, int ix, int muted);
extern int hda_codec_add_insrc(int dev, hdaudio_mixer_t * mixer, int cad, int wid, int ix, int group, const char *name, int muted);
extern int hda_codec_add_insrcselect(int dev, hdaudio_mixer_t * mixer, int cad, int wid, int group, int *ctl, const char *name, int initial_selection);

#define HDA_INAMP(wid, ix, group, name, percent) ctl=hda_codec_add_inamp(dev, mixer, cad, wid, ix, group, name, percent, 0)
#define HDA_INAMP_F(wid, ix, group, name, percent, flags) ctl=hda_codec_add_inamp(dev, mixer, cad, wid, ix, group, name, percent, flags)

#define HDA_INMUTE(wid, ix, group, name, muted) ctl=hda_codec_add_inmute(dev, mixer, cad, wid, ix, group, name, muted, 0)
#define HDA_INMUTE_F(wid, ix, group, name, muted, flags) ctl=hda_codec_add_inmute(dev, mixer, cad, wid, ix, group, name, muted, flags)
#define HDA_SET_INMUTE(wid, ix, muted) hda_codec_set_inmute(dev, mixer, cad, wid, ix, muted)
#define HDA_INSRC(wid, ix, group, name, muted) hda_codec_add_insrc(dev, mixer, cad, wid, ix, group, name, muted)
#define HDA_INSRC_SELECT(wid, group, ctl, name, initial_selection) \
	hda_codec_add_insrcselect(dev, mixer, cad, wid, group, &ctl, name, initial_selection)

extern int hda_codec_add_select(int dev, hdaudio_mixer_t *mixer, int cad, int wid, int group, const char *name, int *ctl, int initial_select);
extern void hda_codec_set_select(int dev, hdaudio_mixer_t *mixer, int cad, int wid, int value);
extern void hda_codec_set_pinselect(int dev, hdaudio_mixer_t *mixer, int cad, int wid, int value);
extern int hda_codec_add_pinselect(int dev, hdaudio_mixer_t *mixer, int cad, int wid, int group, const char *name, int *ctl, int initial_select);
#define HDA_SELECT(wid, name, ctl, group, sel) hda_codec_add_select(dev, mixer, cad, wid, group, name, &ctl, sel)
#define HDA_PINSELECT(wid, ctl, group, name, sel) hda_codec_add_pinselect(dev, mixer, cad, wid, group, name, &ctl, sel)
#define HDA_SETSELECT(wid, value) hda_codec_set_select(dev, mixer, cad, wid, value)
#define HDA_SET_PINSELECT(wid, value) hda_codec_set_pinselect(dev, mixer, cad, wid, value)

extern int hda_codec_add_choices(int dev, hdaudio_mixer_t *mixer, int ctl, const char *choiselist);
#define HDA_CHOICES(ctl, choicelist) hda_codec_add_choices(dev, mixer, ctl, choicelist)

extern void hda_codec_set_color(int dev, hdaudio_mixer_t *mixer, int ctl, int color);
#define HDA_COLOR(ctl, color) hda_codec_set_color(dev, mixer, ctl, color)

#endif

