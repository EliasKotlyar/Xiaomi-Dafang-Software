/*
 * Purpose: Definitions of HD audio codec functions, structures and macros
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
#ifndef HDAUDIO_CODEC_H
#define HDAUDIO_CODEC_H

#define HDA_MAX_OUTSTREAMS		12
#define HDA_MAX_INSTREAMS		8

#define MAX_CODECS	16
#define MAX_WIDGETS	150
#define MAX_CONN	24

#define CT_INMONO	0x0001
#define CT_INSTEREO	0x0002
#define CT_INMUTE	0x0003
#define CT_INSRC	0x0004	/* Negation of a mute control */
#define CT_SELECT	0x0005

#define CT_OUTMONO	0x0006
#define CT_OUTSTEREO	0x0007
#define CT_OUTMUTE	0x0008

#define CT_OUTGAINSEL	0x0009
#define CT_INGAINSEL	0x000a
#define CT_INSRCSELECT	0x000b

#define corb_read(m, cad, wid, d, verb, parm, p1, p2) \
	m->read(m->devc, cad, wid, d, verb, parm, p1, p2)
#define corb_write(m, cad, wid, d, verb, parm) \
	m->write(m->devc, cad, wid, d, verb, parm)

#define MIXNUM(widget, ty, ix) \
	((ix) | (ty<<8) | (widget->wid<<16) | (widget->cad << 24))
extern int hdaudio_set_control (int dev, int ctrl, unsigned int cmd,
				int value);

typedef struct _hdaudio_mixer_struct hdaudio_mixer_t;

/*
 * Reserved (predefined) stream numbers
 */
#define IDDLE_STREAM	0	/* Reserved for silence */
#define FRONT_STREAM	1	/* Reserved for front channel stereo pair */

typedef struct
{
  char *name;
  int is_output;
  unsigned char ix, cad, base_wid;
  unsigned char recsrc_wid;
  unsigned char volume_wid;
  int borrow_count;		/* Number of other endpoints grouped with this one (for multich) */
  int binding;
  int afg;
  int min_rate, max_rate;
  int channels;
  int fmt_mask;
  volatile int busy;
  int nrates, rates[20];
  unsigned int sizemask;
  int stream_number, default_stream_number, iddle_stream;
  int is_digital;
  int is_modem;
  int auto_muted;
  int skip; /* Not connected to anywhere */
  int already_used;
} hdaudio_endpointinfo_t;

/*
 * Codec requests
 */

#define GET_GAIN(inout, leftright)		(0xb00|(inout<<7)|(leftright<<5))
#define SET_GAIN(out,inp,lft, rght, ix)	(0x300|(out<<7)|(inp<<6)|(lft<<5)|(rght<<4)|ix)
#define GET_CONVERTER_FORMAT			0xa00
#define SET_CONVERTER_FORMAT			0x200
#define GET_PARAMETER				0xf00
#define GET_SELECTOR				0xf01
#define SET_SELECTOR				0x701
#define GET_CONNECTION_LIST_ENTRY		0xf02
#define GET_PROCESSING_STATE			0xf03
#define SET_PROCESSING_STATE			0x703
#define GET_SDI_SELECT				0xf04
#define SET_SDI_SELECT				0x704
#define GET_CONVERTER				0xf06
#define SET_CONVERTER				0x706
#define GET_PINCTL				0xf07
#define SET_PINCTL				0x707
#define GET_UNSOLICITED				0xf08
#define SET_UNSOLICITED				0x708
#define GET_CONFIG_DEFAULT			0xf1c
#define SET_POWER_STATE				0x705
#define GET_POWER_STATE				0xf05
#define GET_PIN_SENSE				0xf09
#define TRIGGER_PIN_SENSE			0x709
#define GET_BEEP				0xf0a
#define SET_BEEP				0x70a
#define GET_EAPD				0xf0c
#define SET_EAPD				0x70c
#define GET_SPDIF_CONTROL			0xf0d
#define SET_SPDIF_CONTROL1			0x70d
#define SET_SPDIF_CONTROL2			0x70e
#define GET_VOLUME_KNOB_CONTROL			0xf0f
#define SET_VOLUME_KNOB_CONTROL			0x70f
#define GET_GPI_DATA				0xf10
#define SET_GPI_DATA				0x710
#define GET_GPI_WAKE				0xf11
#define SET_GPI_WAKE				0x711
#define GET_GPI_UNSOL				0xf12
#define SET_GPI_UNSOL				0x712
#define GET_GPI_STICKY				0xf13
#define SET_GPI_STICKY				0x713
#define GET_GPO_DATA				0xf14
#define SET_GPO_DATA				0x714
#define GET_GPIO_DATA				0xf15
#define SET_GPIO_DATA				0x715
#define GET_GPIO_ENABLE				0xf16
#define SET_GPIO_ENABLE				0x716
#define GET_GPIO_DIR				0xf17
#define SET_GPIO_DIR				0x717
#define GET_GPIO_WKEN				0xf18
#define SET_GPIO_WKEN				0x718
#define GET_GPIO_UNSOL				0xf19
#define SET_GPIO_UNSOL				0x719
#define GET_GPIO_STICKY				0xf1a
#define SET_GPIO_STICKY				0x71a
#define GET_SUBSYSTEM_ID			0xf20
#define GET_STRIPE_CONTROL			0xf24
#define SET_CODEC_RESET				0x7ff

/*
 * Parameters
 */

#define	HDA_VENDOR			0x00
#define HDA_REVISION			0x02
#define HDA_NODE_COUNT			0x04
#define HDA_GROUP_TYPE			0x05
#define HDA_AUDIO_GROUP_CAPS		0x08
#define HDA_WIDGET_CAPS			0x09
#define HDA_PCM_SIZES			0x0a
#define HDA_STREAM_FMTS			0x0b
#define HDA_PIN_CAPS			0x0c
#define HDA_INPUTAMP_CAPS		0x0d
#define HDA_CONNLIST_LEN		0x0e
#define HDA_SUPPORTED_POWER_STATES	0x0f
#define HDA_PROCESSING_CAPS		0x10
#define HDA_GPIO_COUNT			0x11
#define HDA_OUTPUTAMP_CAPS		0x12
#define HDA_VOLUMEKNOB_CAPS		0x13

typedef int (*hdmixer_write_t) (void *devc, unsigned int cad,
				unsigned int nid, unsigned int d,
				unsigned int verb, unsigned int parm);
typedef int (*hdmixer_read_t) (void *devc, unsigned int cad, unsigned int nid,
			       unsigned int d, unsigned int verb,
			       unsigned int parm, unsigned int *upper,
			       unsigned int *lower);

#ifdef _KERNEL
extern hdaudio_mixer_t *hdaudio_mixer_create (char *name, void *devc,
					      oss_device_t * osdev,
					      hdmixer_write_t writefunc,
					      hdmixer_read_t readfunc,
					      unsigned int codecmask,
					      unsigned int vendor_id,
					      unsigned int subvendor_id);
extern void hdaudio_mixer_set_initfunc (hdaudio_mixer_t * mixer,
					mixer_create_controls_t func);
#endif

extern int hdaudio_mixer_get_mixdev (hdaudio_mixer_t * mixer);

extern int hdaudio_mixer_get_outendpoints (hdaudio_mixer_t * mixer,
					   hdaudio_endpointinfo_t **
					   endpoints, int size);
extern int hdaudio_mixer_get_inendpoints (hdaudio_mixer_t * mixer,
					  hdaudio_endpointinfo_t ** endpoints,
					  int size);

extern int hdaudio_codec_setup_endpoint (hdaudio_mixer_t * mixer,
					 hdaudio_endpointinfo_t * endpoint,
					 int rate, int channels, int fmt,
					 int stream_number,
					 unsigned int *setupbits);
extern int hdaudio_codec_reset_endpoint (hdaudio_mixer_t * mixer,
					 hdaudio_endpointinfo_t * endpoint,
					 int channels);
extern int hdaudio_codec_audio_ioctl (hdaudio_mixer_t * mixer,
				      hdaudio_endpointinfo_t * endpoint,
				      unsigned int cmd, ioctl_arg arg);

extern int hda_codec_getname (hdaudio_mixer_t * mixer, hda_name_t *name);
extern int hda_codec_getwidget (hdaudio_mixer_t * mixer, hda_widget_info_t *info);
extern void hda_codec_unsol (hdaudio_mixer_t * mixer, unsigned int upper,
			     unsigned int lower);
extern int hdaudio_amp_maxval (unsigned int ampcaps);

/*
 * Audio widget types 
 */

#define NT_DAC		0
#define NT_ADC		1
#define NT_MIXER	2
#define NT_SELECT	3
#define NT_PIN		4
#define NT_POWER	5
#define NT_KNOB		6
#define NT_BEEP		7
#define NT_VENDOR	15

#endif

/*
 * Struct for sample rate table
 */
struct hdaudio_rate_def
{
  unsigned int rate, mask;
};

extern const struct hdaudio_rate_def *hdaudio_rates;

typedef struct
{
  unsigned char cad, wid;
  char name[32];
  char color[32];
  unsigned char wid_type, group_type;
  char used;			/* Already bound to mixer */
  char skip;			/* Ignore this widget as input source */
  char skip_output;		/* Ignore this widget as output target */

  unsigned int widget_caps;
#define WCAP_STEREO		 (1 << 0)
#define WCAP_INPUT_AMP_PRESENT	 (1 << 1)
#define WCAP_OUTPUT_AMP_PRESENT	 (1 << 2)
#define WCAP_AMP_CAP_OVERRIDE	 (1 << 3)
#define WCAP_FORMAT_OVERRIDE	 (1 << 4)
#define WCAP_STRIPE		 (1 << 5)
#define WCAP_PROC_WIDGET	 (1 << 6)
#define WCAP_UNSOL_CAPABLE	 (1 << 7)
#define WCAP_CONN_LIST		 (1 << 8)
#define WCAP_DIGITAL		 (1 << 9)
#define WCAP_POWER_CTL		 (1 << 10)
#define WCAP_LR_SWAP		 (1 << 11)

  unsigned int inamp_caps;
  unsigned int outamp_caps;
#define AMPCAP_MUTE			(1UL<<31)
#define AMPCAP_STEPSIZE_SHIFT		16
#define AMPCAP_STEPSIZE_MASK		0x7f
#define AMPCAP_NUMSTEPS_SHIFT		8
#define AMPCAP_NUMSTEPS_MASK		0x7f
#define AMPCAP_OFFSET_SHIFT		0
#define AMPCAP_OFFSET_MASK		0x7f

  unsigned int pincaps;
#define PINCAP_EAPD			(1<<16)
#define PINCAP_VREF_MASK		0x7f
#define PINCAP_VREF_SHIFT		8
#define PINCAP_RESERVED			(1<<7)
#define PINCAP_BALANCED_PINS		(1<<6)
#define PINCAP_INPUT_CAPABLE		(1<<5)
#define PINCAP_OUTPUT_CAPABLE		(1<<4)
#define PINCAP_HP_DRIVE_CAPABLE		(1<<3)
#define PINCAP_JACKSENSE_CAPABLE	(1<<2)
#define PINCAP_TRIGGERR_RQRD		(1<<1)
#define PINCAP_IMPSENSE_CAPABLE		(1<<0)

  unsigned int sizes;

  int nconn;
  short connections[MAX_CONN];
  short references[MAX_CONN];
  short refcount;
  char plugged_in;
  int impsense;			/* Impedanse sensing result. -1=no info */
  char pin_type, sensed_pin;
#define PIN_UNKNOWN	0
#define PIN_IN		1
#define PIN_MIC		2
#define PIN_OUT		3
#define PIN_HEADPHONE	4
  hdaudio_endpointinfo_t *endpoint;
  int current_selector;
  int rgbcolor;	/* RGB value of the jack color */
  int association, sequence;
} widget_t;

typedef int (*hda_mixer_init_func) (int dev, hdaudio_mixer_t * mixer, int cad,
				    int group);

typedef struct codec_desc codec_desc_t;

typedef struct
{
  const codec_desc_t *codec_desc;
  int active;	/* Codec has audio or modem functionality */
  unsigned long vendor_flags;

  int nwidgets;
  widget_t widgets[MAX_WIDGETS];
  int afg;
  int jack_number;
  int first_node;
  int num_jacks_plugged;
  unsigned int sizes;
  hda_mixer_init_func mixer_init;
  unsigned int default_inamp_caps;
  unsigned int default_outamp_caps;
  unsigned int vendor_id, subvendor_id;

  int speaker_mode;
#define SPKMODE_DEFAULT		0
#define SPKMODE_SPEAKER		1
#define SPKMODE_STEREO		2
#define SPKMODE_51		3
#define SPKMODE_71		4

  int num_inendpoints;
  hdaudio_endpointinfo_t inendpoints[HDA_MAX_INSTREAMS];
  int num_outendpoints;
  hdaudio_endpointinfo_t outendpoints[HDA_MAX_OUTSTREAMS];

  unsigned int multich_map;	/* See codec_desc_t.multich_map */

  int mixer_mode;	/* For use by dedicated drivers. Initially set to 0 */
  char **remap;
} codec_t;

struct _hdaudio_mixer_struct
{
  void *devc;
  oss_device_t *osdev;

  char name[64];
  char *chip_name;
  int mixer_dev;

  hdmixer_write_t write;
  hdmixer_read_t read;

  unsigned int codecmask;

  int ncodecs;
  codec_t *codecs[MAX_CODECS];

  int ncontrols;		/* Total number of widgets (all codecs) */

  int num_inendpoints, copied_inendpoints;
  hdaudio_endpointinfo_t inendpoints[HDA_MAX_INSTREAMS];
  int num_outendpoints, copied_outendpoints;
  hdaudio_endpointinfo_t outendpoints[HDA_MAX_OUTSTREAMS];

  int npins;			/* Used for managing the width of the connectors group */
  int split_connectors;		/* Fearless flag used for splitting the connectors geroup */
  mixer_create_controls_t client_mixer_init;

  int remap_avail;
};

extern int create_outgain_selector (hdaudio_mixer_t * mixer, widget_t * widget, int group, const char *name);
extern int create_ingain_selector (hdaudio_mixer_t * mixer, codec_t * codec, widget_t * widget, int group, int ix, const char *name);

#include "hdaudio_mixers.h"
