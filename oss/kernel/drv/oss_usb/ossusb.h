/*
 * Purpose: Definitions for the USB audio driver files
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

#ifndef __OSUSB_H_
#define __OSUSB_H_

#include "udi.h"
#include "midi_core.h"

#define CS_DEVICE                0x21
#define CS_CONFIG                0x22
#define CS_STRING                0x23
#define CS_INTERFACE             0x24
#define CS_ENDPOINT              0x25

#define AC_HEADER		0x01
#define AC_INPUT_TERMINAL	0x02
#define AC_OUTPUT_TERMINAL	0x03
#define AC_MIXER_UNIT		0x04
#define AC_SELECTOR_UNIT	0x05
#define AC_FEATURE_UNIT		0x06
#define AC_PROCESSING_UNIT	0x07
#define AC_EXTENSION_UNIT	0x08

#define AS_GENERAL		0x01
#define AS_FORMAT_TYPE		0x02
#define AS_FORMAT_SPECIFIC	0x03

#define SET_CUR    0x01
#define SET_MEM    0x05
#define GET_CUR    0x81
#define GET_MIN    0x82
#define GET_MAX    0x83
#define GET_RES    0x84
#define GET_MEM    0x85
#define GET_STAT   0xff

#define USB_RECIP_MASK                  0x1f
#define USB_RECIP_DEVICE                0x00
#define USB_RECIP_INTERFACE             0x01
#define USB_RECIP_ENDPOINT              0x02
#define USB_RECIP_OTHER                 0x03
#define USB_VENDOR_REQUEST		0x40

#define USB_TYPE_CLASS                  0x20

/*
 * Unit types
 *
 * Note that TY_OUTPUT is actually PCM input and TY_INPUT is actually
 * PCM output. Unit types are defined as seen as the USB control interface.
 * Input terminals listen to the output streaming interfases and output
 * terminals feed the input streaming interfaces. Confusing it is.
 */
#define TY_INPUT		0
#define TY_OUTPUT		1
#define TY_MIXER		2
#define TY_SELECT		3
#define TY_FEAT			4
#define TY_PROC			5
#define TY_EXT			6
// Remember to inclease size of devc->unit_counters[] if adding more unit types

#define USBCLASS_AUDIO			1
#define USBCLASS_HID			3

#define MAX_IFACE 10

typedef struct
{
  int num;
  int typ;			// TY_*
  int subtyp;
  char name[16];
  unsigned char *desc;
  int desclen;
  int mixnum;
  int target;
  int source;
  int ctl_avail;
  int control_count;
  int channels;
  int chmask;
} usb_audio_unit_t;

#define MAX_UNIT 40
#define MAX_CONTROLS	100
#define MAX_PORTC	8	/* Max audio streaming interfaces per device */
#define MAX_MIDIC	16	/* Max MIDI streaming interfaces per device */

typedef struct
{
  usb_audio_unit_t *unit;
  int index;
  int flags;
  int min_ch, max_ch;
  int global;
  int chmask;
  int value;
  int min, max, scale;
  int exttype;
} usb_control_t;

typedef struct ossusb_devc ossusb_devc;

typedef struct
{
  ossusb_devc *devc;
  int disabled;
  int if_number;
  void *endpoint_desc, *orig_endpoint_desc;
  udi_endpoint_handle_t *endpoint_handle;
  int audio_dev;
  int open_mode;
  int permitted_modes;
  int prepared_modes;
  int stopping;

  int speed;
  int bytes_per_sample;
  int terminal_link;
  udi_usb_devc *usbdev;
  int num_settings;
  int act_setting;

#define NR_DATAPIPES	2
  udi_usb_request_t *datapipe[NR_DATAPIPES];
  int curr_datapipe;
  int fragment_size;		/* Bytes per (1ms) USB tick */
  int overflow_samples, overflow_size;
  int convert_3byte;
  int pipeline_delay;
  unsigned char *tmp_buf[2];
  int use_tmpbuf;
  oss_dma_handle_t tmpbuf_dma_handle[2];
} ossusb_portc;

typedef struct
{
  struct ossusb_devc *devc;
  oss_device_t *osdev;
  udi_usb_devc *usbdev;

  void *in_endpoint_desc, *out_endpoint_desc;
  udi_endpoint_handle_t *in_endpoint_handle, *out_endpoint_handle;
  int portnum;
  int midi_dev;
  int open_mode;
  // int output_endpoint;
  oss_midi_inputbyte_t midi_input_intr;
  oss_midi_outputintr_t midi_output_intr;
} ossusb_midic;

typedef void *(*special_driver_t) (ossusb_devc * devc);
typedef void (*special_unload_t) (void *devc);

struct ossusb_devc
{
  special_unload_t unload_func;
  oss_device_t *osdev;
  oss_device_t *main_osdev;
  oss_mutex_t mutex;
  int num_interfaces;
  udi_usb_devc *usbdev[MAX_IFACE], *last_usbdev;
  int inum[MAX_IFACE];
  int vendor, product;
  char *dev_name, devpath[32];
  int nports;
  int disabled;
  int usb_version; // 1 or 2

  /*
   * Units
   */

  usb_audio_unit_t units[MAX_UNIT];
  int nunits;
  char unit_counters[7];	// For TY_* identifiers

  /*
   * Mixer stuff
   */
  int mixer_dev;
  void *mixer_usbdev;
  int devmask, recmask, stereodevs, recsrc;

  int rec_unit;
  int num_recdevs;
  int recdevs[32];

  int *levels;

  usb_control_t controls[MAX_CONTROLS];
  int ncontrols;

/* Audio stuff */
  int num_audio_engines, num_inputs, num_outputs;
  ossusb_portc portc[MAX_PORTC];

/* MIDI stuff */
  int num_mididevs;
  ossusb_midic midic[MAX_MIDIC];

  unsigned char *playbuf;
  oss_dma_handle_t playbuf_dma_handle;
  udi_usb_request_t *output_pipe;
  int output_busy;
#define Q_MAX 4096
  unsigned char queue[Q_MAX];
  int q_nbytes;

  unsigned char *recbuf;
  oss_dma_handle_t recbuf_dma_handle;
  udi_usb_request_t *input_pipe;
  udi_endpoint_handle_t *input_endpoint_handle;
};

extern int usb_trace, usb_quiet;

#define UDB(x) {if (usb_trace) x;}

extern ossusb_devc *ossusb_init_audiostream (ossusb_devc * devc,
					     udi_usb_devc * usbdev, int inum,
					     int reinit);
extern ossusb_devc *ossusb_init_midistream (ossusb_devc * devc,
					    udi_usb_devc * usbdev, int inum,
					    int reinit);
extern void ossusb_disconnect_midistream (ossusb_devc * devc);
extern void ossusb_dump_desc (unsigned char *desc, int cfg_len);
extern unsigned int ossusb_get_int (unsigned char *p, int nbytes);
extern int ossusb_change_altsetting (int dev, int ctrl, unsigned int cmd,
				     int value);
extern void ossusb_create_altset_control(int dev, int portc_num, int nalt, char *name);
#endif
