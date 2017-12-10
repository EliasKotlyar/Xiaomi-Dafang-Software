/*
 * Purpose: Common definitions for the Envy24 driver
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
#include "uart401.h"

#define DEV_BUFSIZE		(64*1024)	/* Buffer size per channel */

#define HW_PLAYBUFFSIZE	(10*2048)
#define HW_RECBUFFSIZE	(12*2048)
#define HW_ALLOCSIZE	(24*1024)	/* Must be bigger or equal than HW_PLAYBUFFSIZE and HW_RECBUFFSIZE */

typedef struct envy24_auxdrv envy24_auxdrv_t;

typedef struct
{
  unsigned int svid;
  char *product;
  int nr_outs, nr_ins;		/* # of analog channels */
  int flags;
#define MF_MAUDIO		0x00000001	/* Made by M Audio */
#define MF_MIDI1		0x00000002	/* Has MIDI 1 port */
#define MF_SPDIF		0x00000004	/* Has S/PDIF */
#define MF_AKMCODEC		0x00000008	/* Has AKM codecs */
#define MF_WCLOCK		0x00000010	/* Has World clock input */
#define MF_SPDSELECT		0x00000020	/* Optical+coax S/PDIF */
#define MF_EWS88		0x00000040	/* Terratec EWS88MT */
#define MF_AP			0x00000080	/* M Audio Audiophile family */
#define MF_MIDI2		0x00000100	/* Has MIDI 2 port */
#define MF_EWX2496		0x00000200	/* Terratec EWX 24/96 */
#define MF_CONSUMER		0x00000400	/* Force consumer AC97 codec detection */
#define MF_HOONTECH		0x00000800
#define MF_D410			0x00001000	/* Delta 410 */
#define MF_MEEPROM		0x00002000	/* M Audio EEPROM interface */
#define MF_AC97			0x00004000	/* Consumer AC97 codec */
  envy24_auxdrv_t *auxdrv;
  unsigned char *eeprom_data;
}
card_spec;

#define ICENSEMBLE_VENDOR_ID	0x1412
#define ICENSEMBLE_ENVY24_ID	0x1712

#define AKM_A 0x40
#define AKM_B 0x80

#ifdef USE_LICENSING
extern int options_data;
#endif

#define MAX_ODEV 10
#define MAX_IDEV 12

#ifdef DO_RIAA
typedef struct
{
  int32_t x1, x2, x3;
  int32_t y1, y2, y3;
}
riaa_t;
#endif

typedef struct
{
  int flags;
#define PORTC_SPDOUT	0x00000001
#define PORTC_SPDIN	0x00000002
  int dev;
  int chnum;
  int direction;
#define DIR_INPUT	ADEV_NOOUTPUT
#define DIR_OUTPUT	ADEV_NOINPUT
  int open_mode;

  char name[16];

  int bits, channels;
  volatile int is_active;
  volatile int trigger_bits;
  int pcm_qlen;
  int riaa_filter;
#ifdef DO_RIAA
  riaa_t riaa_parms[12];
#endif
}
envy24_portc;

/*
 * Hoontech specific structure
 */
typedef union
{
  struct
  {
    unsigned int box:2;
    unsigned int darear:1;
    unsigned int id0:2;
    unsigned int clock0:1;
    unsigned int res0:2;

    unsigned int ch1:1;
    unsigned int ch2:1;
    unsigned int ch3:1;
    unsigned int id1:2;
    unsigned int clock1:1;
    unsigned int res1:2;

    unsigned int ch4:1;
    unsigned int midiin:1;
    unsigned int midi1:1;
    unsigned int id2:2;
    unsigned int clock2:1;
    unsigned int res2:2;

    unsigned int midi2:1;
    unsigned int mute:1;
    unsigned int insel:1;
    unsigned int id3:2;
    unsigned int clock3:1;
    unsigned int res3:2;
  }
  b;

  struct
  {
    unsigned int b0:8;
    unsigned int b1:8;
    unsigned int b2:8;
    unsigned int b3:8;
  }
  w;

  unsigned int dwVal;

}
ADSP;

#define _adsp devc->adsp
/*****************/

typedef struct envy24d_portc
{
  int audio_dev;
  int open_mode;
  int channels;
  int direction;
  int trigger_bits;
}
envy24d_portc;

typedef struct
{
  oss_device_t *osdev;
  oss_mutex_t mutex;
  int mpu1_attached, mpu2_attached;
  oss_native_word ccs_base, mt_base;
  int irq;
  card_spec *model_data;
  envy24_auxdrv_t *auxdrv;
  int skipdevs;			/* envy24_skipdevs option */

  unsigned char eeprom[32];

/*
 * MT mixer
 */

  int mixer_dev;

/*
 * Consumer (AC97) mixer
 */

  ac97_devc ac97devc;
  int consumer_mixer_dev;
  int consumer_ac97_present;

  int nr_outdevs, nr_indevs;
  int curr_outch, curr_inch;
  int inportmask, outportmask;
  envy24_portc play_portc[MAX_ODEV];
  envy24_portc rec_portc[MAX_IDEV];
  envy24d_portc direct_portc_in, direct_portc_out;
  int direct_audio_opened;

  int rec_bufsize, play_bufsize;

  unsigned char *playbuf, *recbuf;
  int playbuffsize, recbuffsize;
  oss_native_word playbuf_phys, recbuf_phys;
  oss_dma_handle_t playbuf_dma_handle, recbuf_dma_handle;
  int hw_rfragsize, hw_pfragsize, hw_fragsamples, hw_nfrags;
  volatile int hw_playfrag, hw_recfrag;
  volatile int active_inputs, active_outputs;
  volatile int open_inputs, open_outputs;
  volatile int playback_started, recording_started;
  volatile int playback_prepared, recording_prepared;
  int speed, pending_speed_sel, speedbits;
  int syncsource;
  int gpio_tmp;
#define SYNC_INTERNAL 	0
#define SYNC_SPDIF		1
#define SYNC_WCLOCK		2

  int play_channel_mask, rec_channel_mask;
  int nr_play_channels, nr_rec_channels;
  int writeahead;

  int use_src;
  int ratelock;

/* Spdif parameters */
  int spdif_pro_mode;
  unsigned char spdif_cbits[24];
  int sync_locked;
  int ac3_mode;

  short akm_gains[0xff];
  ADSP adsp;			/* Hoontech only */
  uart401_devc uart401devc1;
  uart401_devc uart401devc2;

  int first_dev;
}
envy24_devc;

struct envy24_auxdrv
{
  void (*card_init) (envy24_devc * devc);
  int (*mixer_init) (envy24_devc * devc, int dev, int group);
  void (*spdif_init) (envy24_devc * devc);
  void (*spdif_set) (envy24_devc * devc, int ctrl0);
  int (*spdif_ioctl) (envy24_devc * devc, int dev, unsigned int cmd,
		      ioctl_arg arg);
  int (*spdif_read_reg) (envy24_devc * devc, int reg);
  void (*spdif_write_reg) (envy24_devc * devc, int reg, int val);
  void (*set_rate) (envy24_devc * devc);
  int (*get_locked_status) (envy24_devc * devc);
  int (*spdif_mixer_init) (envy24_devc * devc, int dev, int group);
  void (*card_uninit) (envy24_devc * devc);
};

struct speed_sel
{
  int speed, speedbits;
};

/* extern struct speed_sel speed_tab[]; */

void envy24d_install (envy24_devc * devc);
void envy24d_playintr (envy24_devc * devc);
void envy24d_recintr (envy24_devc * devc);
void envy24_prepare_play_engine (envy24_devc * devc);
void envy24_launch_play_engine (envy24_devc * devc);
void envy24_stop_playback (envy24_devc * devc);
void envy24_start_recording (envy24_devc * devc);
void envy24_launch_recording (envy24_devc * devc);
void envy24_stop_recording (envy24_devc * devc);

void envy24_write_cci (envy24_devc * devc, int pos, int data);
int envy24_read_cci (envy24_devc * devc, int pos);
extern int cs8427_spdif_ioctl (envy24_devc * devc, int dev, unsigned int cmd,
			       ioctl_arg arg);
extern void init_cs8427_spdif (envy24_devc * devc);
extern int cs8427_spdif_mixer_init (envy24_devc * devc, int dev, int group);
void WriteGPIObit (envy24_devc * devc, int sel, int what);
int ReadGPIObit (envy24_devc * devc, int sel);
void write_cs8427_spdif (envy24_devc * devc, int d);
extern void envy24_set_enum_mask (int dev, int ctl, oss_native_word mask);
