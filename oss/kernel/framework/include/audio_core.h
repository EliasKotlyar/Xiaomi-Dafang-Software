#ifndef AUDIO_CORE_H
#define AUDIO_CORE_H
/*
 * Purpose: Internal definitions for the OS audio core
 *
 * IMPORTANT NOTICE!
 *
 * This file contains internal structures used by Open Sound Systems.
 * They will change without any notice between OSS versions. Care must be taken
 * to make sure any software using this header gets properly re-compiled before
 * use.
 *
 * 4Front Technologies (or anybody else) takes no responsibility of damages
 * caused by use of this file.
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
 * Max number of audio channels currently supported by the sample format converter.
 */
#define OSS_MAX_CONVERT_CHANNELS	64

/*
 * Size of the temporary buffer used for audio conversions.
 *
 * TMP_CONVERT_MAX defines how many bytes can be fed to the converter in
 * one call.
 *
 * TMP_CONVERT_BUF_SIZE defines how many bytes of buffer will be allocated.
 * This is larger than TMP_CONVERT_MAX because amount of data may get expanded
 * during the conversion (for example if the output sampling rate or #channels
 * is larger than in the input side).
 */
#define TMP_CONVERT_MAX		(16*1024)
#define TMP_CONVERT_BUF_SIZE	(8*TMP_CONVERT_MAX)

/*
 * open_flags (for opening audio devices)
 */
#define OF_MMAP		0x00000001	/* Opening application is known to use mmap() */
#define OF_BLOCK	0x00000002	/* Disable O_NONBLOCK */
#define OF_NOCONV	0x00000010	/* Disable all fmt/src conversions */
#define OF_DEVAUDIO	0x00000020	/* Solaris /dev/audio emulator */
#define OF_SMALLFRAGS	0x00000040	/* Use smaller fragments than requested */
#define OF_SMALLBUF	0x00000080	/* Allocate small (4k) buffer this time */
#define OF_MEDIUMBUF	0x00000100	/* Allocate moderate (16k) buffer */

#define CONVERTABLE_FORMATS \
	(AFMT_U8|AFMT_S8|AFMT_MU_LAW|AFMT_S16_LE|AFMT_S16_BE|\
	 AFMT_S24_LE|AFMT_S24_BE|AFMT_S32_LE|AFMT_S32_BE|AFMT_S24_PACKED)

#ifndef _KERNEL
typedef struct _audiodrv_t audiodrv_t;
#endif

typedef struct
{
  char *name;
  int fmt;
  unsigned char neutral_byte;
  char bits;
  char is_linear;
  char endianess;
#define ENDIAN_NONE		0
#define ENDIAN_LITTLE 	1
#define ENDIAN_BIG	  	2
#ifdef OSS_BIG_ENDIAN
# define ENDIAN_NATIVE ENDIAN_BIG
#else
# define ENDIAN_NATIVE ENDIAN_LITTLE
#endif

  char is_signed;
  char alignment;
#define ALIGN_MSB		0
#define ALIGN_LSB		1
  int no_convert;
}
audio_format_info_t, *audio_format_info_p;

typedef struct
{
  int fmt, rate, channels;
  int convert;
}
sample_parms;

typedef struct _adev_t adev_t, *adev_p;
typedef struct _dmap_t *dmap_p;
typedef int (*cnv_func_t) (adev_p adev, dmap_p dmap, unsigned char **srcp, int *srcl,
			   unsigned char **tgtp, sample_parms * source,
			   sample_parms * target);

struct _dmap_t
{
/*
 * Static fields (not to be cleared during open)
 */
#ifdef _KERNEL
  oss_mutex_t mutex;
#endif
  oss_device_t *osdev;
  oss_device_t *master_osdev;	/* The osdev pointer of the master (for virtual drivers) */
  adev_t *adev;
  unsigned char *dmabuf;
  oss_native_word dmabuf_phys;
  oss_dma_handle_t dmabuf_dma_handle;
  int buffsize;
  int buffer_protected;		/* Buffer is shared - don't modify/clear */
  unsigned char *tmpbuf1, *tmpbuf2;
  void *driver_use_ptr;
  long driver_use_value;
  /* Interrupt callback stuff */
  void (*audio_callback) (int dev, int parm);
  int callback_parm;

#ifdef OS_DMA_PARMS
    OS_DMA_PARMS
#endif
/*
 * Dynamic fields (will be zeroed during open)
 * Don't add anything before flags.
 */
  void *srcstate[OSS_MAX_CONVERT_CHANNELS];
  oss_native_word flags;
#define DMAP_NOTIMEOUT	0x00000001
#define DMAP_POST		0x00000002
#define DMAP_PREPARED	0x00000004
#define DMAP_FRAGFIXED	0x00000008	/* Fragment size fixed */
#define DMAP_STARTED	0x00000010
#define DMAP_COOKED		0x00000020
#define DMAP_SMALLBUF     0x00000040	/* Allocate small buffers */
#define DMAP_MEDIUMBUF     0x00000040	/* Allocate 16k buffers */
  int dma_mode;			/* PCM_ENABLE_INPUT, PCM_ENABLE_OUTPUT or 0 */

  /*
   * Queue parameters.
   */
  int nfrags;
  int fragment_size;
  int bytes_in_use;
  int data_rate;		/* Bytes/second */
  int frame_size;		/* Device frame size */
  int user_frame_size;		/* Application frame size */
  int fragsize_rq;
  int low_water, low_water_rq;
  volatile oss_uint64_t byte_counter;
  volatile oss_uint64_t user_counter;
  int interrupt_count;
  int fragment_counter;
  int expand_factor;

  int mapping_flags;
#define			DMA_MAP_MAPPED		0x00000001
  char neutral_byte;

  int error;
  int play_underruns, rec_overruns;
  int underrun_flag;
  int num_errors;
#define MAX_AUDIO_ERRORS	5
  int errors[MAX_AUDIO_ERRORS];
  int error_parms[MAX_AUDIO_ERRORS];

  unsigned char *leftover_buf;
  int leftover_bytes;
  int tmpbuf_len, tmpbuf_ptr;
  cnv_func_t convert_func;
  unsigned int convert_mode;
  struct audio_buffer *(*user_import) (adev_t * adev,
				       dmap_t * dmap,
				       sample_parms * parms,
				       unsigned char *cbuf, int len);
  int (*user_export) (adev_t * adev,
		      dmap_t * dmap, sample_parms * parms,
		      struct audio_buffer * buf, unsigned char *cbuf,
		      int maxbytes);
  struct audio_buffer *(*device_read) (adev_t * adev,
				       dmap_t * dmap,
				       sample_parms * parms,
				       unsigned char *cbuf, int len);
  int (*device_write) (adev_t * adev,
		       dmap_t * dmap,
		       void *frombuf, void *tobuf,
		       int maxspace, int *fromlen, int *tolen);
};
extern int dmap_get_qlen (dmap_t * dmap);
extern int dmap_get_qhead (dmap_t * dmap);
extern int dmap_get_qtail (dmap_t * dmap);

struct _adev_t
{
  char name[64];
  char handle[32];
  int engine_num;		/* Audio engine number */
  int audio_devfile;		/* Audio device file number */
  int enabled;
  int unloaded;
  struct _adev_t *next_in, *next_out;	/* Links to the next "shadow" devices */
  long long flags;
  int open_flags;
  int src_quality;
  int caps;
  int magic;			/* Secret low level driver ID */
  int latency;			/* In usecs, -1=unknown */


  /*
   * Sampling parameters
   */

  sample_parms user_parms, hw_parms;
  int iformat_mask, oformat_mask;	/* Bitmasks for supported audio formats */
  int min_rate, max_rate;	/* Sampling rate limits */
  int min_channels, max_channels;
  char *inch_names, *outch_names;
  int xformat_mask;		/* Format mask for current open mode */
  int binding;
  void *devc;			/* Driver specific info */
  audiodrv_t *d;
  void *portc, *portc_play, *portc_record;	/* Driver specific info */
  dmap_t *dmap_in, *dmap_out;
  int mixer_dev;
  int open_mode;
  int go;
  int enable_bits;
  int parent_dev;		/* 0 -> no parent, 1 to n -> parent=parent_dev+1 */
  int max_block;		/* Maximum fragment size to be accepted */
  int min_block;		/* Minimum fragment size */
  int min_fragments;		/* Minimum number of fragments */
  int max_fragments;		/* Maximum number of fragments */
  int max_intrate;		/* Another form of min_block */
  int dmabuf_alloc_flags;
  oss_uint64_t dmabuf_maxaddr;
  int fixed_rate;
  int vmix_flags;		/* special flags sent to virtual mixer */
#define VMIX_MULTIFRAG	0x00000001	/* More than 2 fragments required (causes longer latencies) */
#define VMIX_DISABLED	0x00000002	/* Not compatible with vmix */
#define VMIX_NOINPUT	0x00000004	/* Disable input capability */
#define VMIX_NOMAINVOL	0x00000008	/* No main volume sliders/meters please */
  pid_t pid;
  char cmd[16];
  oss_device_t *osdev;
  oss_device_t *master_osdev;	/* The osdev pointer of the master (for virtual drivers) */
  int setfragment_warned;
  int getispace_error_count;
  int redirect_in, redirect_out;
  int dmask;			/* Open dmaps */
#define DMASK_OUT		0x01
#define DMASK_IN		0x02
  int nonblock;
  int forced_nonblock;
  int ossd_registered;
  int sync_flags;
#define SYNC_MASTER		0x01
#define SYNC_SLAVE		0x02
  int sync_group;
  int sync_mode;
  adev_t *sync_next;		/* Next device in sync group */

  int rate_source;
  unsigned int nrates, rates[OSS_MAX_SAMPLE_RATES + 1];

#ifdef _KERNEL
  oss_mutex_t mutex;
  oss_wait_queue_t *out_wq, *in_wq;
#endif

  int card_number;
  int port_number;
  int real_dev;

/*
 * By default OSS will let applications to use sampling rates and formats
 * that are not supported by the hardware. Instead OSS performs the necessary
 * format conversions in software. Applications that don't tolerate this kind
 * of conversions usually disable them by using features of the OSS API
 * (SNDCTL_DSP_COOKEDMODE). If this option is set to 0 then the format
 * conversions will be disabled for all applications and devices unless the
 * application explicitly enables them.
 *
 * cooked_enable is a global variable (int) defined in oss_core_options.c. The current
 * value of this global variable will be copied to adev->cooked_enable when
 * an audio engine is opened.
 */
  int cooked_enable;
  int timeout_count;

  void (*outputintr) (int dev, int intr_flags);
  void (*inputintr) (int dev, int intr_flags);

  int policy;
  void *os_id;			/* The device ID (dip) given by the system. */
  oss_longname_t song_name;
  oss_label_t label;
  oss_devnode_t devnode;
  void *vmix_mixer;		/* Pointer to the vmix_mixer_t structure for this device */
  void *prev_vmix_mixer;	/* Reserved for vmix_core */
};

#define UNIT_EXPAND		1024

/*
 * The audio_devfiles and audio_engines tables contain pointers to
 * the (same) adev_t structures. The difference is that
 * audio_engines contains an entry for all audio devices/engines in the system
 * (including hidden and shadow devices). The 'dev' parameter of most audio
 * core routines and audio driver methods are indexes to this array.
 *
 * The audio_devfiles array is a "parallel" structure that contains only
 * the audio engines that have a device file in /dev/oss (and usually also
 * an legacy /dev/dsp# device file). This audio_devfiles array doesn't contain
 * "hidden" audio engines.
 *
 * Each audio operations structure in audio_devfiles will also be in
 * audio_engines but the indexes are different. Both arrays contain pointer to
 * the same structure in memory (not a copy).
 *
 * For example the 6th audio device file (usually but not always /dev/dsp5) is
 * audio_devfiles[5]. However it may be (say) audio_engines[11] if there are
 * hidden devices created before it.
 *
 *  /dev/dsp5 -> audio_devfile[5] == audio_engines[11]
 *
 * The next field of the adev_t structure contains a pointer
 * to the next "identical" device. Most OSS implementations will
 * try to open one of the subsequent devices in the next chain if
 * the original device was busy. "Identical" means that the device suports
 * multiple identical (hw mixing) engines or the vmix driver is used to
 * add software mixing capability to the device.
 */

extern adev_t **audio_engines;
extern int num_audio_engines;
extern adev_t **audio_devfiles;
extern int num_audio_devfiles;

#if 0
typedef struct
{
  int ndevs;
  unsigned short devices[MAX_AUDIO_DEVFILES];
}
oss_devlist_t;

extern oss_devlist_t dspoutlist, dspoutlist2, dspinlist, dspinoutlist;
#endif

int oss_install_audiodev (int vers,
			  oss_device_t * osdev,
			  oss_device_t * master_osdev,
			  char *name,
			  const audiodrv_t * driver,
			  int driver_size,
			  unsigned long long flags,
			  unsigned int format_mask, void *devc, int parent);

int oss_install_audiodev_with_devname (int vers,
			  oss_device_t * osdev,
			  oss_device_t * master_osdev,
			  char *name,
			  const audiodrv_t * driver,
			  int driver_size,
			  int flags,
			  unsigned int format_mask, void *devc, int parent,
			  const char *devfile_name);
extern void install_vdsp (oss_device_t * osdev);
extern int *load_mixer_volumes (char *name, int *levels, int present);

#ifdef _KERNEL
int oss_audio_read (int dev, struct fileinfo *file, uio_t * buf, int count);
int oss_audio_write (int dev, struct fileinfo *file, uio_t * buf, int count);
int oss_audio_open_engine (int dev, int dev_class, struct fileinfo *file,
			   int recursive, int open_flags, int *newdev);
int oss_audio_open_devfile (int dev, int dev_class, struct fileinfo *file,
			    int recursive, int open_flags, int *newdev);
int oss_open_vdsp (int dev, int dev_type, struct fileinfo *file,
		   int recursive, int open_flags, int *newdev);
void oss_audio_release (int dev, struct fileinfo *file);
int oss_audio_ioctl (int dev, struct fileinfo *file,
		     unsigned int cmd, ioctl_arg arg);
int oss_audio_set_format (int dev, int fmt, int format_mask);
int oss_audio_set_channels (int dev, int ch);
int oss_audio_set_rate (int dev, int val);
void audio_uninit_device (int dev);
int oss_audio_mmap (int dev, int direction);
#endif

/* From audiofmt.c */
int setup_format_conversions (adev_p adev, dmap_p dmap, sample_parms * source,
			      sample_parms * target,
			      sample_parms * user,
			      sample_parms * device, int format_mask);
audio_format_info_p oss_find_format (unsigned int fmt);


#define oss_audio_outputintr(dev, flags) audio_engines[dev]->outputintr(dev, flags)
#define oss_audio_inputintr(dev, flags) audio_engines[dev]->inputintr(dev, flags)
void oss_audio_reset (int dev);
void oss_audio_start_syncgroup (unsigned int syncgroup);
typedef int (*oss_audio_startup_func) (void *devc);
extern void oss_audio_register_client (oss_audio_startup_func func,
				       void *devc, oss_device_t * osdev);

extern int oss_encode_enum (oss_mixer_enuminfo * ei, const char *s,
			    int version);
extern char *audio_show_latency (int dev);
extern void oss_audio_inc_byte_counter (dmap_t * dmap, int increment);
extern void oss_add_audio_devlist (int list, int devfile);

#ifndef SMALL_DMABUF_SIZE
#define SMALL_DMABUF_SIZE (4*1024)
#endif

#define MEDIUM_DMABUF_SIZE (16*1024)
#endif
