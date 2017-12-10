#ifndef OSSDDK_H
#define OSSDDK_H

/*
 * ossddk.h - Open Sound System Driver Devlopment Kit (DDK) exported defs.
 *
 * Copyright (C) 4Front Technologies 2005 - All rights reserved.
 */

#define OSSDDK_VERSION		0x02	/* 0.2 */

#include "oss_exports.h"

/*
 * Audio interface
 */
#define OSS_AUDIO_DRIVER_VERSION	2

typedef struct _dmap_t dmap_t;

/*
 * Audio device flags (adev_t->flags)
 */
#define ADEV_DEFAULT		0x0000000000000001LL	/* Preferred default audio device candidate */
#define ADEV_AUTOMODE		0x0000000000000002LL	/* Playbak pointer loops automatically */
#define ADEV_DUPLEX		0x0000000000000004LL	/* Device permits full duplex (O_RDWR method) */
#define ADEV_NOINPUT		0x0000000000000008LL	/* Device cannot do input */
#define ADEV_NOOUTPUT		0x0000000000000010LL	/* Device cannot do output */
#define ADEV_VMIX		0x0000000000000020LL	/* Device is virtual mixer one */

/* NOTE! 0x0000000000000040 is free */

#define ADEV_VIRTUAL		0x0000000000000080LL	/* Virtual audio device */
#define ADEV_OPENED		0x0000000000000100LL	/* Will be set when the device is open */
#define ADEV_NOCONVERT		0x0000000000000200LL	/* No automatic format conversions are permitted */

/*
 * ADEV_HIDDEN is reserved for internal use by the audio core and it must
 * not be set by low level drivers. Instead low level drivers should set
 * adev->caps |= PCM_CAP_HIDDEN if they like to hide the device file from
 * ordinary audio applications.
 */
#define ADEV_HIDDEN		0x0000000000001000LL

#define ADEV_FIXEDRATE		0x0000000000002000LL	/* Fixed sampling rate (obsolete) */
#define ADEV_16BITONLY		0x0000000000004000LL	/* Only 16 bit audio support */
#define ADEV_STEREOONLY		0x0000000000008000LL	/* Only stereo (requires 16BITONLY) */
#define ADEV_SHADOW		0x0000000000010000LL	/* "shadow" device */
#define ADEV_8BITONLY		0x0000000000020000LL	/* Only 8 bits */
#define ADEV_32BITONLY		0x0000000000040000LL	/* Only 24 or 32 bits */
#define ADEV_NOVIRTUAL		0x0000000000080000LL	/* Don't install SoftOSS automatically for this device */
#define ADEV_NOSRC		0x0000000000100000LL	/* Don't do any kind of SRC */
#define ADEV_SPECIAL		0x0000000000200000LL	/* Multich or otherwise special dev */
#define ADEV_NOMMAP		0x0000000000400000LL	/* No MMAP capability */
#define ADEV_DISABLE_VIRTUAL	0x0000000000800000LL	/* Not compatible with virtual drivers  */
#define ADEV_COLD		0x0000000001000000LL	/* Reserved for a future feature - DO NOT USE */
#define ADEV_HWMIX		0x0000000002000000LL	/* Device supports "hardware mixing" */
#define ADEV_LOOP		0x0000000004000000LL	/* Loopback device */
#define ADEV_NONINTERLEAVED	0x0000000008000000LL	/* DMA buffer _NOT_ interleaved  */

#ifdef _KERNEL
typedef struct _audiodrv_t
{
  int (*adrv_open) (int dev, int mode, int open_flags);
  void (*adrv_close) (int dev, int mode);
  void (*adrv_output_block) (int dev, oss_native_word buf,
			int count, int fragsize, int intrflag);
  void (*adrv_start_input) (int dev, oss_native_word buf,
		       int count, int fragsize, int intrflag);
  int (*adrv_ioctl) (int dev, unsigned int cmd, int *arg);
  int (*adrv_prepare_for_input) (int dev, int fragsize, int nfrags);
  int (*adrv_prepare_for_output) (int dev, int fragsize, int nfrags);
  void (*adrv_halt_io) (int dev);
  int (*adrv_local_qlen) (int dev);
  void *not_used; /* Out of order */
  void (*adrv_halt_input) (int dev);
  void (*adrv_halt_output) (int dev);
  void (*adrv_trigger) (int dev, int bits);
  int (*adrv_set_rate) (int dev, int speed);
  unsigned int (*adrv_set_format) (int dev, unsigned int bits);
  short (*adrv_set_channels) (int dev, short channels);
  void (*adrv_postprocess_write) (int dev);	/* Device spesific postprocessing for written data */
  void (*adrv_preprocess_read) (int dev);	/* Device spesific preprocessing for read data */
  /* Timeout handlers for input and output */
  int (*adrv_check_input) (int dev);
  int (*adrv_check_output) (int dev);
  int (*adrv_alloc_buffer) (int dev, dmap_t * dmap, int direction);
  int (*adrv_free_buffer) (int dev, dmap_t * dmap, int direction);
  void (*adrv_lock_buffer) (int dev, int direction);
  void *dummy; /* Not used any more */
  int (*adrv_get_buffer_pointer) (int dev, dmap_t * dmap, int direction);
  int (*adrv_calibrate_speed) (int dev, int nominal_rate, int true_rate);
#define SYNC_ATTACH	0
#define SYNC_PREPARE	1
#define SYNC_TRIGGER	2
  int (*adrv_sync_control) (int dev, int event, int mode);
  void (*adrv_prepare_to_stop) (int dev);
  int (*adrv_get_input_pointer) (int dev, dmap_t * dmap, int direction);
  int (*adrv_get_output_pointer) (int dev, dmap_t * dmap, int direction);
  int (*adrv_bind) (int dev, unsigned int cmd, int *arg);
  void (*adrv_setup_fragments) (int dev, dmap_t * dmap, int direction);
  int (*adrv_redirect) (int dev, int mode, int open_flags);
  int (*adrv_ioctl_override) (int dev, unsigned int cmd, int *arg);
} audiodrv_t;
#endif

/*
 * Mixer interface
 */
#define OSS_MIXER_DRIVER_VERSION	2
#define OSSDDK_MIXER_ROOT		0

struct _mixer_driver_t
{
  int (*ioctl) (int dev, int audiodev, unsigned int cmd, int *arg);
};
#define AINTR_LOCALQUEUE			0x00000001
#define AINTR_NO_POINTER_UPDATES		0x00000002

#if 0
/*
 * The ossddk_* functions are not included in OSS for the time being
 */
#ifdef _KERNEL
extern oss_device_t *ossddk_register_device (int ddkvers,
					     dev_info_t * dip,
					     int drvtype,
					     int instance, const char *nick,
#ifdef sun
					     ddi_iblock_cookie_t
					     iblock_cookie,
#endif
					     void *devc,
					     const char *longname);
extern int ossddk_disable_device (oss_device_t * osdev);
extern void *ossddk_osdev_get_devc (oss_device_t * osdev);
extern void ossddk_unregister_device (oss_device_t * osdev);

#ifdef sun
/*
 * OSS driver entry point routines
 */
extern int oss_open (dev_t * dev_p, int open_flags, int otyp,
		     cred_t * cred_p);
extern int oss_close (dev_t dev, int flag, int otyp, cred_t * cred_p);
extern int oss_ioctl (dev_t dev, int cmd, intptr_t arg, int mode,
		      cred_t * cred_p, int *rval_p);
extern int oss_read (dev_t dev, struct uio *uiop, cred_t * credp);
extern int oss_write (dev_t dev, struct uio *uiop, cred_t * cred_p);
extern int oss_chpoll (dev_t dev, short events, int anyyet, short *reventsp,
		       struct pollhead **phpp);
extern int oss_mmap (dev_t dev, off_t offset, int nprot);
#endif

#if defined(amd64) || defined(sparc) || defined(x86_64)
typedef unsigned long long coreaddr_t;
#else
typedef unsigned long coreaddr_t;
#endif

int ossddk_install_audiodev (int vers,
			     oss_device_t * osdev,
			     oss_device_t * master_osdev,
			     char *name,
			     const audiodrv_t * driver,
			     int driver_size,
			     int flags,
			     unsigned int format_mask, void *devc,
			     int parent);

extern void ossddk_adev_set_devc (int dev, void *devc);
extern void ossddk_adev_set_portc (int dev, void *portc);
extern void ossddk_adev_set_portc_play (int dev, void *portc);
extern void ossddk_adev_set_portc_record (int dev, void *portc);
extern void ossddk_adev_set_portnum (int dev, int portnum);
extern void ossddk_adev_set_mixer (int dev, int mixer_dev);
extern void ossddk_adev_set_rates (int dev, int min_rate, int max_rate,
				   int nrates, int rates[20]);
extern void ossddk_adev_set_formats (int dev, unsigned int oformats,
				     unsigned int iformats);
extern void ossddk_adev_set_caps (int dev, unsigned int caps);
extern void ossddk_adev_set_flags (int dev, unsigned int caps);
extern void ossddk_adev_set_channels (int dev, int min_channels,
				      int max_channels);
extern void ossddk_adev_set_buflimits (int dev, int min_fragment,
				       int max_fragment);
extern void ossddk_adev_set_enable_flag (int dev, int state);
extern void ossddk_adev_set_unloaded_flag (int dev, int state);
extern void ossddk_adev_set_magic (int dev, int magic);
extern void ossddk_adev_set_ratesource (int dev, int rate_source);
extern void ossddk_adev_set_songname (int dev, char *song);
extern void ossddk_adev_set_label (int dev, char *label);

extern void *ossddk_adev_get_devc (int dev);
extern void *ossddk_adev_get_portc (int dev);
extern void *ossddk_adev_get_portc_play (int dev);
extern void *ossddk_adev_get_portc_record (int dev);
extern dmap_t *ossddk_adev_get_dmapout (int dev);
extern dmap_t *ossddk_adev_get_dmapin (int dev);
extern char *ossddk_adev_get_songname (int dev);
extern char *ossddk_adev_get_label (int dev);
extern int ossddk_adev_get_flags (int dev);

extern void ossddk_audio_outputintr (int dev, int intr_flags);
extern void ossddk_audio_inputintr (int dev, int intr_flags);

typedef void (*oss_audio_callback_t) (int dev, int parm);
extern void ossddk_dmap_set_dmabuf (dmap_t * dmap, unsigned char *buf);
extern void ossddk_dmap_set_phys (dmap_t * dmap, unsigned long addr);
extern void ossddk_dmap_set_buffsize (dmap_t * dmap, int size);
extern void ossddk_dmap_set_private (dmap_t * dmap, void *ptr);
extern void ossddk_dmap_set_callback (dmap_t * dmap,
				      oss_audio_callback_t cb, int arg);
extern void ossddk_dmap_set_fragsize (dmap_t * dmap, int size);
extern void ossddk_dmap_set_numfrags (dmap_t * dmap, int nfrags);
extern void ossddk_dmap_set_playerror (dmap_t * dmap, int err, int parm);
extern void ossddk_dmap_set_recerror (dmap_t * dmap, int err, int parm);

extern void *ossddk_dmap_get_private (dmap_t * dmap);
extern int ossddk_dmap_get_fragsize (dmap_t * dmap);
extern int ossddk_dmap_get_numfrags (dmap_t * dmap);
extern int ossddk_dmap_get_buffused (dmap_t * dmap);
extern unsigned char *ossddk_dmap_get_dmabuf (dmap_t * dmap);
extern unsigned long ossddk_dmap_get_phys (dmap_t * dmap);
extern int ossddk_dmap_get_buffsize (dmap_t * dmap);
extern int ossddk_dmap_get_qhead (dmap_t * dmap);
extern int ossddk_dmap_get_qtail (dmap_t * dmap);

int ossddk_install_mixer (int vers,
			  oss_device_t * osdev,
			  oss_device_t * master_osdev,
			  const char *name,
			  mixer_driver_t * driver,
			  int driver_size, void *devc);
extern void *ossddk_mixer_get_devc (int dev);
extern void ossddk_mixer_touch (int dev);
extern int ossddk_mixer_create_group (int dev, int parent, const char *id);
extern int ossddk_mixer_create_control (int dev, int parent, int ctrl,
					mixer_ext_fn func, int type,
					const char *id, int maxvalue,
					int flags);
extern int ossddk_mixer_truncate (int dev, int index);
extern int ossddk_mixer_set_strings (int dev, int ctl, const char *s,
				     int version);

/*
 * AC97 interface
 */

extern int ossddk_ac97_install (ac97_handle_t ** handle, char *name,
				ac97_readfunc_t readfn,
				ac97_writefunc_t writefn, void *hostparms,
				oss_device_t * osdev);
extern void ossddk_ac97_remove (ac97_handle_t * handle);
extern int ossddk_ac97_set_ext_init (ac97_handle_t * handle,
				     mixer_create_controls_t func,
				     int nextra);
extern int ossddk_ac97_is_varrate (ac97_handle_t * handle);
extern int ossddk_ac97_set_recrate (ac97_handle_t * handle, int srate);
extern int ossddk_ac97_set_playrate (ac97_handle_t * handle, int srate);

#endif /* _KERNEL */
#endif

#define OSSDDK_OK	0

#endif
