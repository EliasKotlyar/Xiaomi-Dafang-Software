/*
 * Purpose: Common definitions for envy24ht driver files
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
#include "ac97.h"
#include "midi_core.h"

/* 
 * Various subvendor device ID's
 */
#define SSID_AUREON_SPACE    0x1145153b
#define SSID_AUREON_SKY      0x1147153b
#define SSID_AUREON_UNIVERSE 0x1153153b
#define SSID_PHASE28	     0x1149153b
#define SSID_PRODIGY71	     0x45534933
#define SSID_PRODIGYHD2      0x41543137
#define SSID_PRODIGYHD2_ADE  0x24011412
#define SSID_JULIA	     0x45533031
#define SSID_AP192	     0x36321412

typedef struct
{
  unsigned int dwSubVendorID;	/* PCI[2C-2F], in BIG ENDIAN format */
  unsigned char bSize;		/* size of EEPROM image in bytes */
  unsigned char bVersion;	/* version equal 1 for this structure. */
  unsigned char bCodecConfig;	/* PCI[60] */
  unsigned char bACLinkConfig;	/* PCI[61] */
  unsigned char bI2SID;		/* PCI[62] */
  unsigned char bSpdifConfig;	/* PCI[63] */
  unsigned char bGPIOInitMask0;	/* Corresponding bits need to be inited */
  /* 0 means write bit and 1 means cannot write */
  unsigned char bGPIOInitState0;	/* Initial state of GPIO pins */
  unsigned char bGPIODirection0;	/* GPIO Direction State */
  unsigned char bGPIOInitMask1;
  unsigned char bGPIOInitState1;
  unsigned char bGPIODirection1;
  unsigned char bGPIOInitMask2;
  unsigned char bGPIOInitState2;
  unsigned char bGPIODirection2;
  unsigned char bAC97RecSrc;
  unsigned char bDACID[4];	/* I2S IDs for DACs [0 ~ 3] */
  unsigned char bADCID[4];	/* I2S IDs for ADCs [0 ~ 3] */
  unsigned char bDACID4;	/* I2S ID  for DAC#4 */
  unsigned char Reserved[32];
} eeprom_data_t;

typedef struct envy24ht_auxdrv envy24ht_auxdrv_t;

typedef struct
{
  unsigned int svid;
  char *product;
  int nr_outs, nr_ins;		/* # of analog channels */
  int flags;
#define MF_MIDI			0x00000001	/* Has MIDI port */
#define MF_192K			0x00000002	/* Supports 192kHz */
#define MF_SPDIFIN		0x00000004	/* Has S/PDIF input */
#define MF_SPDIFOUT		0x00000008	/* Has S/PDIF output */
#define MF_ENVY24PT		0x00000010	/* Envy24PT chip (no EEPROM) */
#define MF_NOAC97		0x00000020	/* Ignore AC97 codec */
  const envy24ht_auxdrv_t *auxdrv;
  unsigned char *eeprom_data;
}
card_spec;

#define ICENSEMBLE_VENDOR_ID	0x1412
#define ICENSEMBLE_ENVY24HT_ID	0x1724
#define MAX_ENVY24HT_CARD 8

#ifdef USE_LICENSING
extern int options_data;
#endif

#define MAX_ODEV 5
#define MAX_IDEV 2

typedef struct
{
  int dev;
  int open_mode;
  int direction;
  int fmt;
  char *name;

  int channels;
  volatile int is_active;
  volatile int trigger_bits;
  oss_native_word base;
  unsigned char mask;
  int dev_flags;
#define DF_MULTICH		0x00000001
#define DF_SPDIF		0x00000002
#define DF_AC3			0x00000004
#define DF_DUPLEX		0x00000008
  int chmask;
  int used_chmask;

  int state_bits;
}
envy24ht_portc;

typedef struct
{
  int dta, clk;
} oss_i2c_t;

/*****************/

typedef struct
{
  oss_device_t *osdev;
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;
  const envy24ht_auxdrv_t *auxdrv;
  int codec_type;
#define CODEC_I2S	0
#define CODEC_AC97	1
  int mpu1_attached, mpu2_attached;
  oss_native_word ccs_base, mt_base;
  int irq;
  card_spec *model_data;
  unsigned int subvendor;
  eeprom_data_t eeprom;
  char channel_names[4][10];
  unsigned short gpio_shadow_L;
  unsigned char gpio_shadow_H;

  oss_i2c_t i2c;
/*
 * MT mixer
 */

  int mixer_dev;
  ac97_devc ac97devc;

  int nr_outdevs, nr_indevs;
  envy24ht_portc play_portc[MAX_ODEV];
  envy24ht_portc rec_portc[MAX_IDEV];

  int outportmask, inportmask;
  int nr_play_channels, nr_rec_channels;
  int first_dev;

  int open_count;
  int speed, pending_speed, pending_speed_sel, speedbits, configured_rate_sel;
  int prev_speed;		/* Strictly for use by low level modules */
  int max_ratesel;
  int syncsource;

  int use_src;
  int ratelock;
  int external_sync;
  int busy_play_channels;
  int busy_rec_channels;

  spdif_devc spdc;
  int gains[6];
  int monitor[5];
  int recsrc;

/*
 * MIDI
 */
  int midi_opened;
  int midi_attached;
  oss_midi_inputbyte_t midi_input_intr;
  oss_midi_outputintr_t midi_output_intr;
  volatile unsigned char input_byte;
  int midi_dev;

/*
 * Low level stuff
 */
  unsigned char dac1val[5], dac2val[11];	/* M Audio Revo 7.1 */
  unsigned short m_AC97Volume[6];	/* Terratec Aureon */
  unsigned char m_fAC97Mute[6];	/* Terratec Aureon */
  int m_DigInSource;		/* Aureon */
  int m_LineInSource;		/* Aureon */
  int m_SPDIFSource;		/* Aureon */
  int m_ADCIndex;		/* Aureon */
  int m_f1724SPDIF;		/* Aureon */
  int m_SPDIFConfig;		/* Aureon */
  int m_Frontjack;		/* Aureon */
  unsigned char m_fDACMute[12];	/* Aureon */
  unsigned char m_DACVolume[13];	/* Aureon & Juli@ */
  unsigned short m_ADCVolume[8];	/* Aureon */
  unsigned char m_ADCMux;	/* Aureon */
  unsigned char m_fSPDIFRecord;	/* Aureon */
  unsigned char m_AuxMux;	/* Aureon */
  unsigned int m_ClockSource;	/* Aureon */
  unsigned int m_Out0Source;	/* Aureon */

  int mute;
  int reclevel;
  timeout_id_t timeout_id;	/* Juli@ */

  unsigned char syncstart_mask;
}
envy24ht_devc;

struct envy24ht_auxdrv
{
  void (*card_init) (envy24ht_devc * devc);
  int (*mixer_init) (envy24ht_devc * devc, int dev, int group);
  void (*set_rate) (envy24ht_devc * devc);
  int (*get_locked_status) (envy24ht_devc * devc);
  int (*spdif_mixer_init) (envy24ht_devc * devc, int dev, int group);
  int (*private1) (envy24ht_devc * devc, int value);
  int (*audio_ioctl) (envy24ht_devc * devc, envy24ht_portc * portc, unsigned int cmd,
		      ioctl_arg arg);
  void (*card_uninit) (envy24ht_devc * devc);
};

struct speed_sel
{
  int speed, speedbits;
};

void envy24ht_write_cci (envy24ht_devc * devc, int pos, int data);
int envy24ht_read_cci (envy24ht_devc * devc, int pos);
