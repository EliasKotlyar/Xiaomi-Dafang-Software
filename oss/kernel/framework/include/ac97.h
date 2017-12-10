/*
 * Purpose: Definitions for the AC97 codec support library
 *
 * This header file must be included by all low level drivers that support
 * AC97 based devices.
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

typedef int (*ac97_ext_ioctl) (int dev, int audiodev, unsigned int cmd,
			       int arg);
typedef struct
{
  int is_ok;
  char name[30];
  void *host_parms;
  oss_device_t *osdev;
  ac97_readfunc_t read;
  ac97_writefunc_t write;
  unsigned short ac97_id;	/* Register 0x00 */
  int mixer_dev;

  /* Detected mixer capabilities */
  int mastervol_bits;
  int pcmvol_bits;
  int rearvol_bits;
  int centervol_bits;
  int sidevol_bits;
  int auxvol_bits;
  int enh_bits;			/* number of bits for the 3D enhance */
  int micboost;			/* 20db Mic boost on/off */
  int *levels;
  int recdevs;
  ac97_ext_ioctl overrides[SOUND_MIXER_NRDEVICES];

  int devmask;
  int recmask;
  int var_rate_support;
  int fixed_rate;
  int playrate_support;		/* ac97 2.4 codec */
#define PLAY_2CHAN		0
#define PLAY_4CHAN		1
#define PLAY_6CHAN		2
#define PLAY_8CHAN		3
  int spdifout_support;
#define CS_SPDIFOUT		1	/* Cirrus Logic */
#define AD_SPDIFOUT		2	/* Analog Devices */
#define STAC_SPDIFOUT		3	/* Sigmatel */
#define ALC_SPDIFOUT		4	/* Avance Logic */
#define VIA_SPDIFOUT		5	/* VIA Technologies */
#define CMI_SPDIFOUT		6	/* CMedia */
#define YMF_SPDIFOUT		7	/* Yamaha */
#define CX_SPDIFOUT		8	/* Conexant */
  int spdif_slot;
#define SPDIF_SLOT34		0x00	/*slot3/4 = bits 5/6 = 0 */
#define SPDIF_SLOT78		0x10
#define SPDIF_SLOT69		0x20
#define SPDIF_SLOT1011		0x30
  int spdifin_support;
#define ALC_SPDIFIN		1	/* Avance Logic SPDIF Input */
#define CMI_SPDIFIN		2	/* CMedia SPDIF Input */

  int mixer_ext;
#define ALC650_MIXER_EXT	1
#define AD1980_MIXER_EXT 	2
#define VIA1616_MIXER_EXT	3
#define CMI9739_MIXER_EXT	4
#define CMI9780_MIXER_EXT	5
#define STAC9758_MIXER_EXT	6
#define WM9704_MIXER_EXT	7

  int enh_3d;
/*
 * From AC97 register 0x00:
 *
 *	0x00=No enhancement
 *	0x01=Analog Devices Phat Stereo
 *	0x02=Creative Stereo Enhancement
 *	0x03=National Semiconductor 3D Stereo Enhancement
 *	0x04=Yamaha Ymersion
 *	0x06=Crystal 3D Stereo Enhancement
 *	0x07=Qsound QXpander
 *	0x08=Spatializer 3D Stereo Enhancement
 *	0x09=SRS 3D Stereo Enhancement
 *	0x0b=AKM 3D Audio
 *	0x0c=Aureal Stereo Enhancement
 *	0x0d=Aztech 3D Audio Enhancement
 *	0x0e=Binaura 3D Audio Enhancement
 *	0x0f=ESS Technology
 *	0x10=Harman International VMAx
 *	0x11=NVidea 3D Stereo Enhancement
 *	0x12=Philips Incredible Sound
 *	0x13=Texas Instruments 3D Stereo Enhancement
 *	0x14=VLSI Technology 3D Stereo Enhancement
 *	0x18=Wolfson Analoque 3D stereo enhancement
 *  0x1a=Sigmatel 3D Stereo enhancement (SS3D)
 */
  int extmixlevels[10];		/* volume controls for Rear/Center/additional chans */
#define CENTER_VOL              0
#define REAR_VOL            	1
#define	SIDE_VOL		2
  mixer_create_controls_t client_mixer_init;
}
ac97_devc;

extern int ac97_install (ac97_devc * devc, char *name, ac97_readfunc_t readfn,
			 ac97_writefunc_t writefn, void *hostparms,
			 oss_device_t * osdev);
#define AC97_INVERTED		0x1
#define AC97_FORCE_SENSE	0x2
extern int ac97_install_full (ac97_devc * devc, char *name,
			      ac97_readfunc_t readfn, ac97_writefunc_t writefn,
			      void *hostparms, oss_device_t * osdev, int flags);
extern int ac97_init_ext (int dev, ac97_devc * devc,
			  mixer_create_controls_t func, int nextra);
extern int ac97_varrate (ac97_devc * devc);
extern int ac97_recrate (ac97_devc * devc, int srate);
extern int ac97_playrate (ac97_devc * devc, int srate);
extern void ac97_remove_control (ac97_devc * devc, int mask, int level);
extern void ac97_override_control (ac97_devc * devc, int ctrl,
				   ac97_ext_ioctl func, int level);

/* AC97 V2.2 mixer functions */
extern int ac97_spdifin_ctl (int dev, int ctrl, unsigned int cmd, int value);
extern int ac97_spdifout_ctl (int dev, int ctrl, unsigned int cmd, int value);
extern void ac97_spdif_setup (int dev, int speed, int bits);
extern int ac97_mixext_ctl (int dev, int ctrl, unsigned int cmd, int value);
extern int ac97_mixer_set (ac97_devc * devc, int dev, int value);

/* AC97 V2.2 Mixer extensions */
#define VOL_CENTER		1
#define VOL_REAR		2
#define VOL_SIDE		3
#define CENTER2MIC		4
#define REAR2LINE		5
#define SPREAD			6
#define MICBOOST		7
#define JACKSENSE		8
#define DOWNMIX_LFE		9
#define DOWNMIX_REAR		10

/* SPDIF OUT Mixer Register Definitions */
#define SPDIFOUT_ENABLE 	1
#define SPDIFOUT_PRO    	2
#define SPDIFOUT_AUDIO		3
#define SPDIFOUT_COPY		4
#define SPDIFOUT_PREEMPH	5
#define SPDIFOUT_CATEGORY     	6
#define SPDIFOUT_GENERATION	7
#define SPDIFOUT_RATE		8
#define SPDIFOUT_VBIT		9
#define SPDIFOUT_ADC   		10

/* SPDIF IN Mixer register Definitions */
#define SPDIFIN_ENABLE 		1
#define SPDIFIN_PRO    		2
#define SPDIFIN_AUDIO  		3
#define SPDIFIN_COPY   		4
#define SPDIFIN_PREEMPH		5
#define SPDIFIN_MODE   		6
#define SPDIFIN_CATEGORY	7
#define SPDIFIN_GENERATION	8
#define SPDIFIN_SOURCE 		9
#define SPDIFIN_CHAN   		10
#define SPDIFIN_RATE   		11
#define SPDIFIN_CLOCK  		12
#define SPDIFIN_SIGNAL 		13
#define SPDIFIN_VBIT   		14
#define SPDIFIN_MON    		15
#define SPDIFIN_LOOP   		16
