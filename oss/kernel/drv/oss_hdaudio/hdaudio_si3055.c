/*
 * Purpose: Driver for Si3055 and compatible modems.
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


/*
 * Documentation Note
 * 
 * There is no publicly available documentation for Si3055. However,
 * there is a very similar modem (Si3038) for which a datasheet is
 * publicly available:
 * 
 * https://www.silabs.com/Support%20Documents/TechnicalDocs/si3038.pdf
 * 
 * This driver was written by reading the ALSA code, looking for a
 * similar modem (Si3038), and figuring out the corresponding Si3055
 * register IDs for Si3038 registers, again by reading the ALSA code,
 * and by checking the default after-reset values.
 * 
 */

#include "oss_hdaudio_cfg.h"
#include "hdaudio.h"
#include "hdaudio_codec.h"
#include "hdaudio_dedicated.h"
#include "hdaudio_mixers.h"

/*
 * Si3055 register IDs.
 */
#define SI3055_EXT_MODEM_STATUS 2
#define SI3055_LINE_RATE  3
#define SI3055_HDA_STREAMS      4
#define SI3055_GPIO_CONFIG      5
#define SI3055_GPIO_PIN_STATUS  10
#define SI3055_LINE_CONFIG      13


/* Corresponding registers in Si3038 (for reference):
 * 
 * SI3055_EXT_MODEM_STATUS      3Eh (Extended Modem Status & Control)
 * SI3055_LINE_RATE             40h (Line 1 DAC/ADC Rate)
 * SI3055_GPIO_PIN_STATUS       54h (GPIO Pin Status)
 * SI3055_LINE_CONFIG           56h (Line Side Configuration 1)
 */

/*
 * The SI3055_HDA_STREAMS register has no corresponding in Si3038.
 * It contains the playback and recording stream descriptors in the
 * following format:
 * 
 *    ((playback_stream_num << 4) << 8) | (recording_stream_num << 4)
 */

/* Si3055 verbs. */
#define SI3055_REG_GET_VERB     0x900
#define SI3055_REG_SET_VERB     0x100

/* Convenience macros for reading from and writing to Si3055 registers. */
#define SI3055_REG_GET(mixer, cad, reg, a, b) corb_read(mixer, cad, reg, 0, SI3055_REG_GET_VERB, 0, a, b)
#define SI3055_REG_SET(mixer, cad, reg, val)  corb_write(mixer, cad, reg, 0, SI3055_REG_SET_VERB, val)


void hdaudio_si3055_set_rate(hdaudio_mixer_t *mixer, int cad, int rate)
{
  SI3055_REG_SET(mixer, cad, SI3055_LINE_RATE, rate);
}

int hdaudio_si3055_set_offhook(hdaudio_mixer_t *mixer, int cad, int offhook)
{
  unsigned int a, b;
  SI3055_REG_GET(mixer, cad, SI3055_GPIO_PIN_STATUS, &a, &b);

  if (offhook)
    {
      a |= 0x1;    /* Set Off-Hook bit */
    }
  else
    {
      a &= ~0x1;   /* Unset Off-Hook bit */
    }

  SI3055_REG_SET(mixer, cad, SI3055_GPIO_PIN_STATUS, a);
  SI3055_REG_GET(mixer, cad, SI3055_GPIO_PIN_STATUS, &a, &b);
  return ((a & 0x1) == 0x1);
}

void hdaudio_si3055_endpoint_init(hdaudio_mixer_t *mixer, int cad)
{
  codec_t *codec = mixer->codecs[cad];  /* Modem codec */
  widget_t *widget;                     /* MFG widget */
  hdaudio_endpointinfo_t *endpoint;
  unsigned int a, b;      /* Used for reading data. */
  int tmout;              /* Timeout counter. */

  /* Output and input stream numbers. */
  int playback_stream_num, recording_stream_num;

  
  DDB(cmn_err(CE_CONT, "hdaudio_si3055_endpoint_init got called.\n"));

  /* Reset the modem codec. */
  corb_write(mixer, cad, 0x00, 0, SET_CODEC_RESET, 0);
  corb_write(mixer, cad, codec->afg, 0, SET_CONVERTER, IDDLE_STREAM << 4);
  corb_write(mixer, cad, codec->afg, 0, SET_CONVERTER_FORMAT, 0);
  
  /* Set 9600Hz as the initial line sampling rate.
   * It can be changed later when desired.
   */
  SI3055_REG_SET(mixer, cad, SI3055_LINE_RATE, 9600);
  
  /* Assign the "unused" value to the playback and recording
   * stream descriptors (ref. HDAudio_03.pdf, page 40).
   */
  SI3055_REG_SET(mixer, cad, SI3055_HDA_STREAMS, 0x0000);
  
  /* Write 0x0000 to the Extended Modem Status & Control register
   * to power up the modem (ref. si3038.pdf, page 22).
   */
  SI3055_REG_SET(mixer, cad, SI3055_EXT_MODEM_STATUS, 0x0000);
  
  /* Wait for the modem to complete power up. The lower 8 bits
   * indicate that it is ready (ref. si3038.pdf, page 22).
   */
  tmout = 10;
  do
    {
      SI3055_REG_GET(mixer, cad, SI3055_EXT_MODEM_STATUS, &a, &b);
      DDB(cmn_err(CE_CONT, "si3055: ext modem status: %04x.\n", a));
      oss_udelay(1000);
    }
    while(((a & 0xf) == 0) && --tmout);
  
  if ((a & 0xf) == 0)
    {
      cmn_err(CE_WARN, "si3055: power up timeout (status: %04x).\n", a);
    }
  
  /* This register contains 0x1fff after reset. We need to set it
   * to zero to get the modem working. No corresponding register
   * could be found in the Si3038 datasheet.
   */
  SI3055_REG_SET(mixer, cad, SI3055_GPIO_CONFIG, 0x0000);

  /* Program line interface parameters. The register value after
   * a reset is 0xF010. Set it to 0x0010 to unmute the analog
   * receive and transmit paths.
   */
  SI3055_REG_SET(mixer, cad, SI3055_LINE_CONFIG, 0x0010);
      
  /* Setup the widget info. */
  widget = &codec->widgets[codec->afg];
  widget->endpoint = &codec->inendpoints[0];
  widget->sizes = 0x20000; /* 16 bits */
  strcpy(widget->name, "modem");

  /* Setup the output endpoint. */
  codec->num_outendpoints = 1;
  endpoint = &codec->outendpoints[0];
  endpoint->ix = 0;
  endpoint->iddle_stream = 0;
  endpoint->cad = cad;
  endpoint->base_wid = codec->afg;
  endpoint->recsrc_wid = endpoint->volume_wid = -1;
  endpoint->nrates = 3;
  endpoint->rates[0] = 8000;
  endpoint->rates[1] = 9600;
  endpoint->rates[2] = 16000;
  endpoint->name = widget->name;
  endpoint->channels = 1;
  endpoint->is_digital = 0;
  endpoint->is_modem = 1;
  endpoint->sizemask = widget->sizes;
  endpoint->fmt_mask = AFMT_S16_LE;
  endpoint->afg = codec->afg;
  endpoint->min_rate = 8000;
  endpoint->max_rate = 16000;
  
  /* Setup the input endpoint. */
  codec->num_inendpoints = 1;
  memcpy(&codec->inendpoints[0], endpoint, sizeof(*endpoint));

  /* Choose stream numbers for output and input streams. */
  playback_stream_num = ++mixer->num_outendpoints;
  endpoint->stream_number = endpoint->default_stream_number = playback_stream_num;

  endpoint = &codec->inendpoints[0];
  recording_stream_num = ++mixer->num_inendpoints;
  endpoint->stream_number = endpoint->default_stream_number = recording_stream_num;

  /* Setup the stream numbers. */
  SI3055_REG_SET(mixer, cad, SI3055_HDA_STREAMS, ((playback_stream_num  << 4) << 8) |
                                                  (recording_stream_num << 4));
}

