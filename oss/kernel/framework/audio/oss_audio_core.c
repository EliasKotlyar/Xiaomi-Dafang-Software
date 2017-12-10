/*
 * Purpose: Audio core functionality of OSS
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

#undef  NO_COOKED_MODE
#define AUDIO_C

#define GRC	3

/* Some debugging macros (for future use) */
#define UP_STATUS(x)
#define DOWN_STATUS(x)

#include <oss_config.h>

extern int src_quality;
extern int vmix_disabled;
extern int excl_policy;

oss_mutex_t audio_global_mutex;

/*
 * Resizeable audio device tables
 */
static oss_memblk_t *audio_global_memblk=NULL;
int oss_max_audio_devfiles=4;
int oss_max_audio_engines=8;
#define AUDIO_MALLOC(osdev, size) oss_memblk_malloc(&audio_global_memblk, size)
#define AUDIO_FREE(osdev, addr) oss_memblk_free(&audio_global_memblk, addr)
#define AUDIO_ENGINE_INCREMENT	(oss_max_audio_engines/2)
#define AUDIO_DEVFILE_INCREMENT	(oss_max_audio_devfiles/2)

/*
 * List of audio devices in the system
 */
adev_t **audio_engines = NULL;
int num_audio_engines = 0;

adev_t **audio_devfiles = NULL;
int num_audio_devfiles = 0;
extern int flat_device_model;

#if 0
/*
 * Device lists (input, output, duplex) for /dev/dsp.
 */

oss_devlist_t dspinlist = { 0 };
oss_devlist_t dspoutlist = { 0 };
oss_devlist_t dspinoutlist = { 0 };
oss_devlist_t dspoutlist2 = { 0 };	/* 2nd priority output devices */
#endif

/*
 * Applications known to require special handling (mmap, etc.). These 
 * applications are listed here because they use the OSS API incorrectly and
 * some workarounds by OSS are required to make them to work.
 */
typedef struct
{
  char *name;
  int open_flags;
} oss_specialapp_t;

oss_specialapp_t special_apps[] = {
  {"quake", OF_MMAP},
  {"quake2", OF_MMAP},
  {"q3demo.x86", OF_MMAP},
  {"wolfsp.x86", OF_MMAP},
  {"wolfmp.x86", OF_MMAP},
  {"quake3", OF_MMAP},
  {"wolf3d", OF_MMAP},
  {"rtcw", OF_MMAP},
  {"artsd", OF_BLOCK | OF_NOCONV},
  {"realplay.bin", OF_SMALLFRAGS},
  {"hxplay.bin", OF_SMALLFRAGS},
#if 1
  {"auserver", OF_NOCONV},	/* Win4lin */
#endif
  {"quake3.x86", OF_MMAP},
  {"wolf.x86", OF_MMAP},
  {"et.x86", OF_MMAP},
  {"doom.x86", OF_MMAP},
  // {"mozilla-bin", OF_MEDIUMBUF}, /* For the flash plugin */
  {NULL, 0}
};

#ifdef APPLIST_SUPPORT
/*
 * Lookup tables for applications. Use these lists to assign
 * fixed target devices for given applications.
 */

app_routing_t oss_applist[APPLIST_SIZE];
int oss_applist_size = 0;

static int
app_lookup (int mode, const char *appname, int *open_flags)
{
  int i, dev;

  if (appname == NULL)
    return -2;

  for (i = 0; i < oss_applist_size; i++)
    {
      if (oss_applist[i].mode == mode)
	if (strncmp (appname, oss_applist[i].name, 32) == 0)
	  {
	    dev = oss_applist[i].dev;

	    if (dev < -1 || dev >= num_audio_devfiles)
	      return -1;
	    *open_flags |= oss_applist[i].open_flags;
	    return dev;
	  }
    }

  return -2;
}
#endif

/*ARGSUSED*/
static int
get_open_flags (int mode, int open_flags, struct fileinfo *file)
{
  char *name;
  int i;

  if ((name = GET_PROCESS_NAME (file)) != NULL)
    {
#ifdef APPLIST_SUPPORT
      if (app_lookup (mode, name, &open_flags) >= -1)
	return open_flags;
#endif

      for (i = 0; special_apps[i].name != NULL; i++)
	if (strcmp (special_apps[i].name, name) == 0)
	  {
	    open_flags |= special_apps[i].open_flags;
	    return open_flags;
	  }
    }

  return open_flags;
}

#define BIGWRITE_SIZE	1024

#define NEUTRAL8	0x80
#define NEUTRAL16	0x00
#define OSS_PLUGIN_VERSION		(0x100|oss_sample_bits)

int always_cooked = 0;
static unsigned long sync_seed = 0;

#define COOKED_BLOCK_SIZE 4096

extern oss_uint64_t oss_tmp_timer;

/*
 * Structure used by oss_audio_register_client()
 */
typedef struct
{
  oss_audio_startup_func func;
  void *devc;
  oss_device_t *osdev;
} audio_startup_t;

#define MAX_STARTUPS	5
static audio_startup_t audio_startups[MAX_STARTUPS];
static int num_audio_startups = 0;

int
dmap_get_qlen (dmap_p dmap)
{
  int len;

  if (dmap->dma_mode == PCM_ENABLE_OUTPUT)
    {
      if (dmap->fragment_size == 0)
	{
	  cmn_err (CE_WARN, "dmap (out) fragment_size=0, dev=%s\n",
		   dmap->adev->name);
	  return 0;
	}

      len =
	(int) ((dmap->user_counter -
		dmap->byte_counter) / dmap->fragment_size);
      if (len < 0)
	len = 0;
      return len;
    }

  if (dmap->dma_mode == PCM_ENABLE_INPUT)
    {
      if (dmap->fragment_size == 0)
	{
	  cmn_err (CE_WARN, "dmap (in) fragment_size=0, dev=%s\n",
		   dmap->adev->name);
	  return 0;
	}

      len =
	(int) ((dmap->byte_counter -
		dmap->user_counter) / dmap->fragment_size);
      if (len < 0)
	len = 0;
      return len;
    }

  return 0;
}

int
dmap_get_qhead (dmap_p dmap)
{
  unsigned int ptr;

  if (dmap->fragment_size == 0)
    return 0;

  if (dmap->dma_mode == PCM_ENABLE_OUTPUT)
    {
      ptr = (int) (dmap->byte_counter % dmap->bytes_in_use);
      return ptr / dmap->fragment_size;
    }

  if (dmap->dma_mode == PCM_ENABLE_INPUT)
    {
      ptr = (int) (dmap->user_counter % dmap->bytes_in_use);
      return ptr / dmap->fragment_size;
    }

  return 0;
}

int
dmap_get_qtail (dmap_p dmap)
{
  unsigned int ptr;

  if (dmap->fragment_size == 0)
    return 0;

  if (dmap->dma_mode == PCM_ENABLE_INPUT)
    {
      ptr = (int) (dmap->byte_counter % dmap->bytes_in_use);
      return ptr / dmap->fragment_size;
    }

  if (dmap->dma_mode == PCM_ENABLE_OUTPUT)
    {
      ptr = (int) (dmap->user_counter % dmap->bytes_in_use);
      return ptr / dmap->fragment_size;
    }

  return 0;
}

int
oss_audio_set_format (int dev, int fmt, int format_mask)
{
  adev_p adev;
  int ret, newfmt;
  unsigned char neutral_byte;
  audio_format_info_p fmt_info;

  sync_seed++;

  if (dev < 0 || dev >= num_audio_engines)
    return OSS_ENXIO;

  adev = audio_engines[dev];
  if (!adev->enabled)
    return OSS_ENXIO;

  if (fmt == 0)
    return adev->user_parms.fmt;

  if (fmt == AFMT_AC3)
    {
      /*
       * AC3 cannot tolerate any format/rate conversions so disable them.
       */
      adev->cooked_enable = 0;

      /*
       * Report EIO error if the application tries to do something stupid. 
       * Otherwise buggy applications may produce loud helicopter sound.
       */

      if (adev->flags & ADEV_VMIX)
	{
	  oss_audio_set_error (adev->engine_num, E_REC,
			       OSSERR (1018,
				       "AFMT_AC3 used with virtual mixing device."),
			       0);
	  /*
	   * Errordesc:
	   * Devices that support virtual or hardware mixing are
	   * probably not capable to play AC3 streams properly.
	   *
	   * Virtual mixing is enabled on the audio device. The virtual
	   * mixer driver will do voolume control that corrupts the AC3
	   * bitstream. Applications using AC3 should
	   * open the audio device with O_EXCL to make sure that virtual
	   * mixing is properly bypassed.
	   */
	   return OSS_EIO;
	}

      if (adev->flags & ADEV_HWMIX)
	{
	  oss_audio_set_error (adev->engine_num, E_REC,
			       OSSERR (1019,
				       "AFMT_AC3 used with hardware mixing device."),
			       0);
	  /*
	   * Errordesc:
	   * Devices that support virtual or hardware mixing are
	   * probably not capable to play AC3 streams properly.
	   *
	   * This error may be caused by user who selected a wrong audio
	   * device. 
	   */
	   return OSS_EIO;
	}

      if (adev->dmask & DMASK_OUT)
	if (adev->dmap_out->flags & DMAP_COOKED)
	  {
	    oss_audio_set_error (adev->engine_num, E_REC,
				 OSSERR (1020,
					 "AFMT_AC3 used with format conversions enabled."),
				 0);
	    /*
	     * Errordesc:
	     * AC3 audio format (AFMT_AC3) bitstreams don't tolerate any
	     * kind of sample rate or format conversions. However it looks 
	     * like conversions would be needed. The reason may be that the
	     * application had earlier requested some sample rate that is
	     * not supported by the device.
	     *
	     * Applications using AC3 should call SNDCTL_DSP_COOKEDMODE to
	     * disable format conversions.
	     */
	   return OSS_EIO;
	  }
    }

  ret = adev->d->adrv_set_format (dev, fmt);

  adev->user_parms.fmt = ret;
  adev->hw_parms.fmt = ret;

  if ((fmt_info = oss_find_format (ret)) == NULL)
    neutral_byte = 0;
  else
    {
      neutral_byte = fmt_info->neutral_byte;
    }

  if (adev->dmask & DMASK_OUT)
    adev->dmap_out->neutral_byte = neutral_byte;
  if (adev->dmask & DMASK_IN)
    adev->dmap_in->neutral_byte = neutral_byte;

  /* Disable format conversions if mmap() */
  if ((adev->dmap_out->mapping_flags & DMA_MAP_MAPPED) ||
      (adev->dmap_in->mapping_flags & DMA_MAP_MAPPED) ||
      (adev->open_flags & OF_MMAP))
    return ret;

#ifdef NO_COOKED_MODE
  return ret;
#else

  if (ret == fmt)
    return ret;

  if (!adev->cooked_enable)
    return ret;

/*
 * We need to perform format conversions because the device
 * doesn't support the requested format.
 */

  /* Convertable format? */
  if (!(fmt & CONVERTABLE_FORMATS))
    {
      return ret;
    }

  adev->user_parms.fmt = fmt;
  if (adev->dmask & DMASK_OUT)
    {
      adev->dmap_out->flags |= DMAP_COOKED;
    }

  if (adev->dmask & DMASK_IN)
    {
      adev->dmap_in->flags |= DMAP_COOKED;
    }

  /* If the device is in 16 bit format then just return */
  if (ret & (AFMT_S16_LE | AFMT_S16_BE))
    return fmt;

  /* Try to find a suitable format */

  if (fmt == AFMT_MU_LAW && ret == AFMT_U8)	/* mu-Law <-> 8 bit linear */
    return fmt;

  if (format_mask & AFMT_S16_LE)
    {
      newfmt = AFMT_S16_LE;
      goto got_it;
    }

  if (format_mask & AFMT_S16_BE)
    {
      newfmt = AFMT_S16_BE;
      goto got_it;
    }

  return fmt;			/* Nothing better than the one suggested by the device */
got_it:

  ret = adev->d->adrv_set_format (dev, newfmt);

  adev->hw_parms.fmt = ret;

  return fmt;
#endif
}

int
oss_audio_set_channels (int dev, int ch)
{
  adev_p adev;
  int ret;

  sync_seed++;

  if (dev < 0 || dev >= num_audio_engines)
    return OSS_ENXIO;

  adev = audio_engines[dev];
  if (!adev->enabled)
    return OSS_ENXIO;

  if (ch == 0)
    return adev->user_parms.channels;

  ret = adev->d->adrv_set_channels (dev, ch);

  if (ret < 1)
    {
      cmn_err (CE_WARN,
	       "Audio engine %d: Internal error in channel setup, err=%d, ch=%d\n",
	       dev, ret, ch);
      return OSS_EIO;
    }

  if (ch > 2 && ch > ret)	/* Requested multi channel mode not possible */
    ch = ret;

  adev->user_parms.channels = ret;
  adev->hw_parms.channels = ret;

  /* Disable format conversions if mmap() */
  if ((adev->dmap_out->mapping_flags & DMA_MAP_MAPPED) ||
      (adev->dmap_in->mapping_flags & DMA_MAP_MAPPED) ||
      (adev->open_flags & OF_MMAP))
    return ret;

  if (ret > 1 && (adev->flags & ADEV_NONINTERLEAVED))
  {
	  if (adev->dmask & DMASK_OUT)
	      adev->dmap_out->flags |= DMAP_COOKED;
	
	  if (adev->dmask & DMASK_IN)
	      adev->dmap_in->flags |= DMAP_COOKED;
  }
     

#ifdef NO_COOKED_MODE
  return ret;
#else

  if (!adev->cooked_enable)
    return ret;

  if (ret == ch)
    return ret;

  /* For the time being only stereo <->mono is possible */
  if (ch != ret)
    if (!((ch == 1 && ret == 2) || (ch == 2 && ret == 1)))
      return ret;

/*
 * Needs to perform format conversions
 */

  adev->user_parms.channels = ch;
  if (adev->dmask & DMASK_OUT)
    {
      adev->dmap_out->flags |= DMAP_COOKED;
    }

  if (adev->dmask & DMASK_IN)
    {
      adev->dmap_in->flags |= DMAP_COOKED;
    }

  return ch;
#endif
}

int
oss_audio_set_rate (int dev, int rate)
{
  adev_p adev;
  int ret;

  sync_seed++;

  if (dev < 0 || dev >= num_audio_engines)
    return OSS_ENXIO;

  adev = audio_engines[dev];
  if (!adev->enabled)
    return OSS_ENXIO;

  if (rate == 0)
    return adev->user_parms.rate;

  ret = adev->d->adrv_set_rate (dev, rate);
  if (ret < 0)
    return ret;

  adev->user_parms.rate = ret;
  adev->hw_parms.rate = ret;

  /* Disable format conversions if mmap() */
  if ((adev->dmap_out->mapping_flags & DMA_MAP_MAPPED) ||
      (adev->dmap_in->mapping_flags & DMA_MAP_MAPPED) ||
      (adev->flags & ADEV_NOSRC) || (adev->open_flags & OF_MMAP))
    return ret;

  if (!adev->cooked_enable)
    return ret;

#if defined(NO_COOKED_MODE) || GRC == 0
  return ret;
#else

  if (ret == rate)		/* No SRC needed */
    return ret;

/*
 * Needs to perform format conversions
 */

  adev->user_parms.rate = rate;
  if (adev->dmask & DMASK_OUT)
    {
      adev->dmap_out->flags |= DMAP_COOKED;
    }

  if (adev->dmask & DMASK_IN)
    {
      adev->dmap_in->flags |= DMAP_COOKED;
    }

  return rate;
#endif
}

static void
reset_dmap (dmap_p dmap)
{
#ifdef DO_TIMINGS
  oss_do_timing ("Reset dmap");
#endif
  dmap->dma_mode = 0;
  dmap->flags &= (DMAP_FRAGFIXED | DMAP_COOKED);
  dmap->byte_counter = dmap->user_counter = 0;
  dmap->fragment_counter = 0;
  dmap->interrupt_count = 0;
  dmap->expand_factor = UNIT_EXPAND;	/* 1:1 */
  dmap->play_underruns = dmap->rec_overruns = 0;
  dmap->num_errors = 0;
  dmap->leftover_bytes = 0;
  dmap->leftover_buf = NULL;
}

int
oss_alloc_dmabuf (int dev, dmap_p dmap, int direction)
{
  adev_t *adev = dmap->adev;

  if (adev == NULL)
    {
      cmn_err (CE_WARN, "oss_alloc_dmabuf: adev==NULL\n");
      return OSS_EIO;
    }

  return __oss_alloc_dmabuf (dev, dmap, adev->dmabuf_alloc_flags,
			     adev->dmabuf_maxaddr, direction);
}

static int
default_alloc_buffer (int dev, dmap_t * dmap, int direction)
{
  int err;

  if (dmap->dmabuf != NULL)
    return 0;

  if ((err = oss_alloc_dmabuf (dev, dmap, direction)) < 0)
    return err;

  return 0;
}

/*ARGSUSED*/
static int
default_free_buffer (int dev, dmap_t * dmap, int direction)
{
  if (dmap->dmabuf == NULL)
    return 0;

  oss_free_dmabuf (dev, dmap);

  dmap->dmabuf = NULL;
  dmap->dmabuf_phys = 0;
  return 0;
}

static int
init_dmap (adev_p adev, dmap_p dmap, int direction)
{
  int retval;
  int l, static_len;
  char *p;
  oss_native_word flags;

  if (dmap->osdev == NULL)
    {
      cmn_err (CE_WARN, "dmap->osdev==NULL\n");
      return OSS_EIO;
    }

  if (dmap->dmabuf == NULL)
    {
      if (adev->d->adrv_alloc_buffer != NULL)
	{
	  retval =
	    adev->d->adrv_alloc_buffer (adev->engine_num, dmap, direction);
	}
      else
	{
	  retval = default_alloc_buffer (adev->engine_num, dmap, direction);
	}
      if (retval < 0)
	{
	  cmn_err (CE_WARN,
		   "Failed to allocate DMA buffer for audio engine %d/%s\n",
		   adev->engine_num, adev->name);
	  return retval;
	}

      if (dmap->dmabuf_phys == 0)	/* Cannot do mmap */
	adev->flags |= ADEV_NOMMAP;
    }

#ifdef linux
  if (dmap->dmabuf_phys == 0)
    adev->flags |= ADEV_NOMMAP;
#endif
  dmap->flags &= ~DMAP_FRAGFIXED;

  l = sizeof (dmap_t);
  static_len = (oss_native_word) & dmap->flags - (oss_native_word) dmap;

  p = (char *) &dmap->flags;
  l -= static_len;
  if (p != NULL)
    memset (p, 0, l);		/* Clear all */

  if (!dmap->buffer_protected && dmap->dmabuf != NULL)
    memset (dmap->dmabuf, dmap->neutral_byte, dmap->buffsize);

  MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
  reset_dmap (dmap);
  dmap->bytes_in_use = dmap->buffsize;
  dmap->frame_size = 1;
  dmap->user_frame_size = 1;
  dmap->flags = 0;
  dmap->audio_callback = NULL;

  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);

  return 0;
}

void
audio_reset_adev (adev_p adev)
{
  oss_native_word flags, dflags;
#ifdef DO_TIMINGS
  oss_do_timing ("Reset input & output\n");
#endif
  sync_seed++;
  if (!adev->enabled)
    return;

  dflags = 0;
  MUTEX_ENTER_IRQDISABLE (adev->mutex, flags);
  adev->go = 1;
  adev->enable_bits = adev->open_mode;
  if (adev->open_mode & OPEN_WRITE)
    {
      dmap_p dmap = adev->dmap_out;
      dmap->flags &= ~DMAP_PREPARED;
      memset (dmap->dmabuf, 0, dmap->buffsize);
    }

  if (adev->d->adrv_halt_input && adev->d->adrv_halt_output)
    {
      if (adev->open_mode & OPEN_READ)
	{
	  dmap_p dmap = adev->dmap_in;
	  MUTEX_ENTER (dmap->mutex, dflags);
	  dmap->flags &= ~DMAP_PREPARED;
	  if ((dmap->dma_mode == PCM_ENABLE_INPUT)
	      && (dmap->flags & DMAP_STARTED))
	    {
#ifdef CONFIG_OSSD
	      ossd_event (adev->engine_num, OSSD_EV_RESET_INPUT);
#endif
	      MUTEX_EXIT (dmap->mutex, dflags);
	      MUTEX_EXIT_IRQRESTORE (adev->mutex, flags);
	      adev->d->adrv_halt_input (adev->engine_num);
	      MUTEX_ENTER_IRQDISABLE (adev->mutex, flags);
	      MUTEX_ENTER (dmap->mutex, dflags);
	      reset_dmap (dmap);
	    }
	  MUTEX_EXIT (dmap->mutex, dflags);
	}
      if (adev->open_mode & OPEN_WRITE)
	{
	  dmap_p dmap = adev->dmap_out;
	  MUTEX_ENTER (dmap->mutex, dflags);
	  if ((dmap->dma_mode == PCM_ENABLE_OUTPUT) &&
	      (dmap->flags & DMAP_STARTED))
	    {
#ifdef CONFIG_OSSD
	      ossd_event (adev->engine_num, OSSD_EV_RESET_OUTPUT);
#endif
	      MUTEX_EXIT (dmap->mutex, dflags);
	      MUTEX_EXIT_IRQRESTORE (adev->mutex, flags);
	      adev->d->adrv_halt_output (adev->engine_num);
	      MUTEX_ENTER_IRQDISABLE (adev->mutex, flags);
	      MUTEX_ENTER (dmap->mutex, dflags);
	      reset_dmap (dmap);
	    }
	  MUTEX_EXIT (dmap->mutex, dflags);
	}

      MUTEX_EXIT_IRQRESTORE (adev->mutex, flags);
      return;
    }
  MUTEX_EXIT_IRQRESTORE (adev->mutex, flags);

#ifdef CONFIG_OSSD
  ossd_event (adev->engine_num, OSSD_EV_RESET_OUTPUT);
  ossd_event (adev->engine_num, OSSD_EV_RESET_INPUT);
#endif
  adev->d->adrv_halt_io (adev->engine_num);

  MUTEX_ENTER_IRQDISABLE (adev->mutex, flags);
  reset_dmap (adev->dmap_out);
  reset_dmap (adev->dmap_in);
  MUTEX_EXIT_IRQRESTORE (adev->mutex, flags);
}

static void
audio_reset_input (adev_p adev)
{
  dmap_p dmap = adev->dmap_in;
  oss_native_word flags, dflags;

  dflags = 0;
  if (!(adev->open_mode & OPEN_READ))
    return;

  if (dmap->dma_mode != PCM_ENABLE_INPUT)
    return;

  if (!(dmap->flags & DMAP_STARTED))
    return;

#ifdef DO_TIMINGS
  oss_do_timing ("Reset input\n");
#endif
  dmap->flags &= ~DMAP_PREPARED;

  if (adev->d->adrv_halt_input)
    {
#ifdef CONFIG_OSSD
      ossd_event (adev->engine_num, OSSD_EV_RESET_INPUT);
#endif
      adev->d->adrv_halt_input (adev->engine_num);
      MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
      reset_dmap (dmap);
      MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
      return;
    }

  adev->d->adrv_halt_io (adev->engine_num);
  MUTEX_ENTER_IRQDISABLE (adev->mutex, flags);
  MUTEX_ENTER (dmap->mutex, dflags);
  reset_dmap (dmap);
  MUTEX_EXIT (dmap->mutex, dflags);
  MUTEX_EXIT_IRQRESTORE (adev->mutex, flags);
}

static void
audio_reset_output (adev_p adev)
{
  dmap_p dmap = adev->dmap_out;
  oss_native_word flags, dflags;
  dflags = 0;

  sync_seed++;

  if (!(adev->open_mode & OPEN_WRITE))
    return;

  if (dmap->dma_mode != PCM_ENABLE_OUTPUT)
    return;

  if (!(dmap->flags & DMAP_STARTED))
    return;

  dmap->flags &= ~DMAP_PREPARED;
#ifdef DO_TIMINGS
  oss_do_timing ("Reset output\n");
#endif

  if (adev->d->adrv_halt_output)
    {
#ifdef CONFIG_OSSD
      ossd_event (adev->engine_num, OSSD_EV_RESET_OUTPUT);
#endif
      adev->d->adrv_halt_output (adev->engine_num);
      MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
      reset_dmap (dmap);
      MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
      return;
    }

  adev->d->adrv_halt_io (adev->engine_num);
  MUTEX_ENTER_IRQDISABLE (adev->mutex, flags);
  MUTEX_ENTER (dmap->mutex, dflags);
  reset_dmap (dmap);
  MUTEX_EXIT (dmap->mutex, dflags);
  MUTEX_EXIT_IRQRESTORE (adev->mutex, flags);
}

static int launch_output (adev_p adev, dmap_p dmap);
static int launch_input (adev_p adev, dmap_p dmap);

static void
oss_audio_post (adev_p adev)
{
  int i, n, p;
  oss_uint64_t limit;
  dmap_p dmap = adev->dmap_out;

  if (!(adev->open_mode & OPEN_WRITE))
    return;
  if (dmap->user_counter == 0)
    return;

  if (dmap->flags & DMAP_STARTED)
    return;

#ifdef DO_TIMINGS
  oss_do_timing ("Post output\n");
#endif

  /* Clean the unused buffer space (in slow way) */
  limit = dmap->byte_counter + dmap->bytes_in_use;
  if (limit > dmap->user_counter + dmap->bytes_in_use)
    limit = dmap->user_counter + dmap->bytes_in_use;
  n = (int) (limit - dmap->user_counter) + 1;
  p = (int) (dmap->user_counter % dmap->bytes_in_use);	/* Current ptr */
  for (i = 0; i < n; i++)
    {
      dmap->dmabuf[p] = dmap->neutral_byte;
      p = (p + 1) % dmap->bytes_in_use;
    }

  launch_output (adev, dmap);
}

static void
oss_audio_sync (adev_p adev)
{
  dmap_p dmap = adev->dmap_out;
  oss_uint64_t uc, limit;
  oss_native_word flags;
  unsigned int status;

  int n = 0, loops = 0, tmout = OSS_HZ, i;
  int prev_underruns;

  if (dmap->dma_mode != PCM_ENABLE_OUTPUT)
    return;
  if (adev->dmap_out->mapping_flags & DMA_MAP_MAPPED)
    return;

  if (!(dmap->flags & DMAP_STARTED))
    oss_audio_post (adev);

  if (!(dmap->flags & DMAP_STARTED))
    return;

  /* Don't wait if output is untriggered */
  if (adev->go == 0 || !(adev->enable_bits & PCM_ENABLE_OUTPUT))
    return;

#ifdef DO_TIMINGS
  oss_do_timing ("Sync output\n");
#endif

  MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
  dmap->flags |= DMAP_POST;

  uc = dmap->user_counter;
  prev_underruns = dmap->play_underruns;
  dmap->play_underruns = 0;

  while (dmap->play_underruns == 0 && loops++ < dmap->nfrags
	 && dmap->byte_counter < uc)
    {
      int p;
      adev->dmap_out->error = 0;
      /* Clean the unused buffer space (in a slow but precise way) */
      limit = dmap->byte_counter + dmap->bytes_in_use;
      if (limit > dmap->user_counter + dmap->bytes_in_use)
	limit = dmap->user_counter + dmap->bytes_in_use;

      p = (int) (dmap->user_counter % dmap->bytes_in_use);	/* Current ptr */
      n = (int) (limit - dmap->user_counter) + 1;
      for (i = 0; i < n; i++)
	{
	  dmap->dmabuf[p] = dmap->neutral_byte;
	  p = (p + 1) % dmap->bytes_in_use;
	}

      tmout = (dmap->fragment_size * OSS_HZ) / dmap->data_rate;
      tmout += OSS_HZ / 10;

      if (!oss_sleep (adev->out_wq, &dmap->mutex, tmout, &flags, &status))
	{
#ifndef __FreeBSD__
	  cmn_err (CE_WARN, "Output timed out (sync) on audio engine %d\n",
		   adev->engine_num);
#endif
	  adev->timeout_count++;
	  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
	  FMA_EREPORT(adev->osdev, DDI_FM_DEVICE_STALL, NULL, NULL, NULL);
	  FMA_IMPACT(adev->osdev, DDI_SERVICE_LOST);
	  return;
	}
    }
  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);

  n = 0;
  if (adev->d->adrv_local_qlen)	/* Drain the internal queue too */
    while (n++ <=
	   adev->d->adrv_local_qlen (adev->engine_num) / dmap->fragment_size)
      {
	MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
	audio_engines[adev->engine_num]->dmap_out->error = 0;
	tmout = (dmap->fragment_size * OSS_HZ) / dmap->data_rate;
	tmout += OSS_HZ / 10;
	oss_sleep (adev->out_wq, &dmap->mutex, tmout, &flags, &status);
	MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
      }
  dmap->play_underruns = prev_underruns;
}

static int prepare_output (adev_p adev, dmap_p dmap);
static int prepare_input (adev_p adev, dmap_p dmap);

void
oss_audio_set_error (int dev, int mode, int err, int parm)
{
  int i, n;

  dmap_t *dmap;

#ifdef DO_TIMINGS
  {
    if (mode == E_REC)
      oss_timing_printf ("Audio rec error code %05d/%d, engine=%d", err, parm,
	       mode);
    else
      oss_timing_printf ("Audio play error code %05d/%d, engine=%d", err, parm,
	       mode);
  }
#endif

#if 0
  if (mode == E_REC)
    cmn_err (CE_CONT, "Audio rec error code %05d/%d, engine=%d\n", err, parm,
	     mode);
  else
    cmn_err (CE_CONT, "Audio play error code %05d/%d, engine=%d\n", err, parm,
	     mode);
#endif

  switch (mode)
    {
    case E_PLAY:
      dmap = audio_engines[dev]->dmap_out;
      break;
    case E_REC:
      dmap = audio_engines[dev]->dmap_in;
      break;

    default:
      dmap = audio_engines[dev]->dmap_out;	/* Actually this would be an error */
    }

  n = dmap->num_errors++;
  if (n > MAX_AUDIO_ERRORS)
    n = MAX_AUDIO_ERRORS;

  if (n > 0)
    {

/*
 * Move previous errors (if any) upwards in the list.
 */
      for (i = n; i > 0; i--)
	{
	  dmap->errors[i] = dmap->errors[i - 1];
	  dmap->error_parms[i] = dmap->error_parms[i - 1];
	}
    }

  dmap->errors[0] = err;
  dmap->error_parms[0] = parm;

}

/*ARGSUSED*/
static void
report_errors (char *s, const char *hdr, adev_t * adev, dmap_t * dmap,
	       int bufsize)
{
  int i, l, j;

  if (bufsize < 30)		/* No space left */
    return;

  strcpy (s, hdr);
  s += l = strlen (s);
  bufsize -= l;

  for (i = 0; i < dmap->num_errors && i < MAX_AUDIO_ERRORS; i++)
    {
      int dupe = 0;

      if (bufsize < 30)
	return;
      for (j = 0; !dupe && j < i; j++)
	if (dmap->errors[i] == dmap->errors[j])
	  dupe = 1;

      if (dupe)
	continue;

      sprintf (s, " %05d:%d", dmap->errors[i], dmap->error_parms[i]);
      s += l = strlen (s);
      bufsize -= l;
    }
}

static void
store_history (int dev)
{
  int p;
  char *tmp, *s;

  adev_p adev = audio_engines[dev];

  if (adev->pid == 0)		/* Virtual mixer */
    return;

  if (*adev->cmd && strcmp (adev->cmd, "sndconf") == 0)
    return;

  p = oss_history_p;
  oss_history_p = (oss_history_p + 1) % OSS_HISTORY_SIZE;

  tmp = oss_history[p];
  memset (tmp, 0, OSS_HISTORY_STRSIZE);
  s = tmp;

  sprintf (s, "%s.%02d: ", adev->devnode, adev->engine_num);
  s += strlen (s);

  if (adev->pid != -1)
    {
      sprintf (s, "pid %d ", adev->pid);
      s += strlen (s);
    }

  if (*adev->cmd != 0)
    {
      sprintf (s, "cmd '%s' ", adev->cmd);
      s += strlen (s);
    }
  else
    {
      strcpy (s, "cmd: Unknown ");
      s += strlen (s);
    }

  if (adev->open_mode & OPEN_READ)
    {
      sprintf (s, "IN ");
      s += strlen (s);
    }

  if (adev->open_mode & OPEN_WRITE)
    {
      sprintf (s, "OUT ");
      s += strlen (s);
    }

  if (adev->timeout_count > 0)
    {
      sprintf (s, "%d timeouts ", adev->timeout_count);
      s += strlen (s);
    }

  if (adev->dmap_out->play_underruns > 0)
    {
      sprintf (s, "%d underruns ", adev->dmap_out->play_underruns);
      s += strlen (s);
    }

  if (adev->dmap_in->rec_overruns > 0)
    {
      sprintf (s, "%d overruns ", adev->dmap_in->rec_overruns);
      s += strlen (s);
    }

  if (adev->dmap_out->num_errors > 0)
    {
      report_errors (s, "Play events:", adev, adev->dmap_out,
		     OSS_HISTORY_STRSIZE - strlen (tmp) - 1);
      s += strlen (s);
      *s++ = ' ';
    }

  if (adev->dmap_in->num_errors > 0)
    {
      report_errors (s, "Rec events:", adev, adev->dmap_in,
		     OSS_HISTORY_STRSIZE - strlen (tmp) - 1);
      s += strlen (s);
      *s++ = ' ';
    }
}

char *
audio_show_latency (int dev)
{
  static char str[32];
  adev_p adev = audio_engines[dev];
  dmap_p dmap;
  int dr, fs;

  *str = 0;

  if (dev < 0 || dev >= num_audio_engines)
    return "Bad device";

  if (adev->open_mode == 0)
    return "Not opened";

  if (adev->open_mode == OPEN_READ)
    dmap = adev->dmap_in;
  else
    dmap = adev->dmap_out;

  if (dmap == NULL)
    return "Unknown";

  if (!(dmap->flags & DMAP_STARTED))
    return "Not started";

  dr = dmap->data_rate;
  fs = dmap->fragment_size;

  if (dr <= 0)
    sprintf (str, "%d", dmap->fragment_size);
  else
    {
      int r = (fs * 10000) / dr;

      sprintf (str, "%d/%d (%d.%d msec)", dmap->fragment_size, dr, r / 10,
	       r % 10);
    }


  return str;
}

void
oss_audio_release (int dev, struct fileinfo *file)
{
  adev_p adev;
  int mode;
  oss_native_word flags;

  sync_seed++;

  if (dev < 0 || dev >= num_audio_engines)
    return;
  if (file)
    mode = file->mode & O_ACCMODE;
  else
    mode = OPEN_READ | OPEN_WRITE;
#ifdef DO_TIMINGS
    oss_timing_printf ("=-=-=- Closing audio engine %d -=-=-=", dev);
#endif

  adev = audio_engines[dev];

  if (adev->unloaded || !adev->enabled)
    {
      cmn_err (CE_CONT, "Audio device %s not available - close aborted\n",
	       adev->name);
      return;
    }

  oss_audio_post (adev);
  oss_audio_sync (adev);

#ifdef CONFIG_OSSD
  ossd_event (adev->engine_num, OSSD_EV_CLOSE);
#endif
  adev->d->adrv_halt_io (dev);
  adev->d->adrv_close (dev, mode);
  MUTEX_ENTER_IRQDISABLE (adev->mutex, flags);
  if (adev->dmask & DMASK_OUT)
    {
      adev->dmap_out->audio_callback = NULL;
      if (!adev->dmap_out->buffer_protected)
	if (adev->dmap_out->dmabuf != NULL)
	  memset (adev->dmap_out->dmabuf, adev->dmap_out->neutral_byte,
		  adev->dmap_out->buffsize);
    }

  if (adev->dmask & DMASK_IN)
    {
      adev->dmap_in->audio_callback = NULL;
    }

  store_history (dev);

  memset (audio_engines[dev]->cmd, 0, sizeof (audio_engines[dev]->cmd));
  memset (audio_engines[dev]->label, 0, sizeof (audio_engines[dev]->label));
  memset (audio_engines[dev]->song_name, 0,
	  sizeof (audio_engines[dev]->song_name));
  audio_engines[dev]->pid = -1;
  MUTEX_EXIT_IRQRESTORE (adev->mutex, flags);

  /*
   * Free the DMA buffer(s)
   */
  if (adev->dmask & DMASK_OUT)
    if (adev->dmap_out->dmabuf != NULL)
      {
	if (adev->d->adrv_free_buffer != NULL)
	  {
	    adev->d->adrv_free_buffer (dev, adev->dmap_out, OPEN_WRITE);
	  }
	else
	  default_free_buffer (dev, adev->dmap_out, OPEN_WRITE);
	adev->dmap_out->dmabuf = NULL;
      }

  if (adev->dmask & DMASK_IN)
    if (adev->dmap_in != adev->dmap_out && adev->dmap_in->dmabuf != NULL)
      {
	if (adev->d->adrv_free_buffer != NULL)
	  {
	    adev->d->adrv_free_buffer (dev, adev->dmap_in, OPEN_READ);
	  }
	else
	  default_free_buffer (dev, adev->dmap_in, OPEN_READ);
	adev->dmap_in->dmabuf = NULL;
      }

  MUTEX_ENTER_IRQDISABLE (adev->mutex, flags);
  adev->open_mode = 0;
  adev->flags &= ~ADEV_OPENED;
  MUTEX_EXIT_IRQRESTORE (adev->mutex, flags);
}

static void audio_outputintr (int dev, int intr_flags);
static void audio_inputintr (int dev, int intr_flags);

/*ARGSUSED*/
int
oss_audio_open_engine (int dev, int no_worries, struct fileinfo *file,
		       int recursive, int open_flags, int *newdev)
{
/*
 * Open audio engine
 */
  adev_p adev;
  int retval = 0, fmt;
  int mode;
  int saved_cooked;
  int channels, rate;
  oss_native_word flags;
  extern int cooked_enable;	/* Config option */

  DOWN_STATUS (0xff);

  if (file)
    {
      mode = file->mode & O_ACCMODE;
    }
  else
    mode = OPEN_READ | OPEN_WRITE;

  sync_seed++;

  DDB (cmn_err
       (CE_CONT, "oss_audio_open_engine(%d, mode=0x%x)\n", dev, mode));

#ifdef DO_TIMINGS
    oss_timing_printf ("-----> oss_audio_open_engine(%d, mode=0x%x)", dev, mode);
#endif

  if (dev < 0 || dev >= num_audio_engines || audio_engines == NULL)
    return OSS_ENXIO;

  adev = audio_engines[dev];
  if (adev->unloaded)
    return OSS_ENODEV;
  if (!adev->enabled)
    return OSS_ENXIO;

  if (adev->osdev == NULL)
    {
      cmn_err (CE_WARN, "adev->osdev==NULL\n");
      return OSS_EIO;
    }

  open_flags = get_open_flags (mode, open_flags, file);

  MUTEX_ENTER_IRQDISABLE (adev->mutex, flags);
  if (adev->open_mode != 0)
    {
      MUTEX_EXIT_IRQRESTORE (adev->mutex, flags);
      return OSS_EBUSY;
    }
  adev->open_mode = mode;
  MUTEX_EXIT_IRQRESTORE (adev->mutex, flags);

  adev->cooked_enable = cooked_enable;	/* This must be done before drv->open() */
#ifdef DO_TIMINGS
  if (adev->cooked_enable)
    oss_do_timing ("Initial cooked mode ON");
  else
    oss_do_timing ("Initial cooked mode OFF");
#endif

  adev->src_quality = src_quality;
  if ((retval = adev->d->adrv_open (dev, mode, open_flags)) < 0)
    {
      adev->open_mode = 0;

      return retval;
    }

  MUTEX_ENTER_IRQDISABLE (adev->mutex, flags);
  adev->outputintr = audio_outputintr;
  adev->inputintr = audio_inputintr;
  adev->forced_nonblock = 0;
  adev->setfragment_warned = 0;
  adev->getispace_error_count = 0;
  adev->sync_next = NULL;
  adev->sync_group = 0;
  adev->sync_flags = 0;
  adev->open_flags = open_flags;
  adev->flags |= ADEV_OPENED;
  adev->timeout_count = 0;
  adev->policy = -1;
  adev->song_name[0] = 0;
  adev->label[0] = 0;

  if (open_flags & (OF_NOCONV | OF_MMAP))
    {
      adev->cooked_enable = 0;
#ifdef DO_TIMINGS
      oss_do_timing ("Forcing cooked mode OFF");
#endif
    }

  memset (audio_engines[dev]->cmd, 0, sizeof (audio_engines[dev]->cmd));

  if (recursive)
    {
      strcpy (audio_engines[dev]->cmd, "OSS internal");
      audio_engines[dev]->pid = 0;
    }
  else
    {
      char *cmd;

      if ((cmd = GET_PROCESS_NAME (file)) != NULL)
	{
	  strncpy (audio_engines[dev]->cmd, cmd, 16);
	  strncpy (audio_engines[dev]->label, cmd, 16);
	  audio_engines[dev]->cmd[15] = '\0';
	  audio_engines[dev]->label[15] = '\0';
	}
      audio_engines[dev]->pid = GET_PROCESS_PID (file);
    }

  adev->dmask = 0;
/*
 * Find out which dmap structures are required.
 */

  if (mode == OPEN_WRITE)
    adev->dmask = DMASK_OUT;
  else if (mode == OPEN_READ)
    adev->dmask = DMASK_IN;
  else
    {
      if (mode != (OPEN_WRITE | OPEN_READ))
	cmn_err (CE_NOTE, "Unrecognized open mode %x\n", mode);
      adev->dmask = DMASK_OUT;

      if ((adev->flags & ADEV_DUPLEX) && adev->dmap_out != adev->dmap_in)
	adev->dmask = DMASK_OUT | DMASK_IN;
    }

#if 0
/*
 * Handling of non-blocking doesn't belong here. It will be handled
 * by read/write.
 */
  if (file != NULL && !(open_flags & OF_BLOCK))
    if (ISSET_FILE_FLAG (file, O_NONBLOCK) || adev->forced_nonblock)
      {
#ifdef DO_TIMINGS
	oss_do_timing ("*** Non-blocking open ***");
#endif
	adev->nonblock = 1;
      }
#endif
  MUTEX_EXIT_IRQRESTORE (adev->mutex, flags);

  if (adev->dmask & DMASK_OUT)
    {

      /*
       * Use small DMA buffer if requested and if the device supports it.
       */
      if (SMALL_DMABUF_SIZE >= (adev->min_block * 2) &&
	  (open_flags & OF_SMALLBUF))
	adev->dmap_out->flags |= DMAP_SMALLBUF;
      else
	if (MEDIUM_DMABUF_SIZE >= (adev->min_block * 2) &&
	    (open_flags & OF_MEDIUMBUF))
	adev->dmap_out->flags |= DMAP_MEDIUMBUF;
      else
	adev->dmap_out->flags &= ~(DMAP_SMALLBUF | DMAP_MEDIUMBUF);

      if ((retval = init_dmap (adev, adev->dmap_out, OPEN_WRITE)) < 0)
	{
	  oss_audio_release (dev, file);
	  return retval;
	}
    }

  if (adev->dmask & DMASK_IN)
    {
      /*
       * Use small DMA buffer if requested and if the device supports it.
       */
      if (SMALL_DMABUF_SIZE >= (adev->min_block * 2) &&
	  (open_flags & OF_SMALLBUF))
	adev->dmap_in->flags |= DMAP_SMALLBUF;
      else
	adev->dmap_in->flags &= ~DMAP_SMALLBUF;

      if ((retval = init_dmap (adev, adev->dmap_in, OPEN_READ)) < 0)
	{
	  oss_audio_release (dev, file);
	  return retval;
	}
    }

  adev->dmap_out->num_errors = 0;
  adev->dmap_in->num_errors = 0;

  if ((mode & OPEN_WRITE) && !(adev->flags & ADEV_NOOUTPUT))
    {
      oss_reset_wait_queue (adev->out_wq);
    }
  if ((mode & OPEN_READ) && !(adev->flags & ADEV_NOINPUT))
    {
      oss_reset_wait_queue (adev->in_wq);
    }

  adev->xformat_mask = 0;

  switch (adev->dmask)
    {
    case DMASK_OUT:
      adev->xformat_mask = adev->oformat_mask;
      break;

    case DMASK_IN:
      adev->xformat_mask = adev->iformat_mask;
      break;

    case DMASK_OUT | DMASK_IN:
      adev->xformat_mask = adev->oformat_mask & adev->iformat_mask;
      break;

    default:
      cmn_err (CE_WARN, "Internal error A during open\n");
    }

  fmt = DSP_DEFAULT_FMT;
  rate = DSP_DEFAULT_SPEED;

  if (adev->flags & ADEV_FIXEDRATE && adev->fixed_rate != 0)
    rate = adev->fixed_rate;

  if (adev->flags & ADEV_16BITONLY)
    fmt = AFMT_S16_LE;

  adev->go = 1;
  adev->enable_bits = adev->open_mode;

  channels = 1;
  if (adev->flags & ADEV_STEREOONLY)
    channels = 2;

  saved_cooked = adev->cooked_enable;
  adev->cooked_enable = 0;
  oss_audio_set_format (dev, fmt, adev->xformat_mask);
  oss_audio_set_channels (dev, channels);
  oss_audio_set_rate (dev, rate);
  adev->cooked_enable = saved_cooked;

/* Clear the Cooked mode to permit mmap() */
  if (adev->dmask & DMASK_OUT)
    adev->dmap_out->flags &= ~DMAP_COOKED;
  if (adev->dmask & DMASK_IN)
    adev->dmap_in->flags &= ~DMAP_COOKED;

#ifdef DO_TIMINGS
  {
    char *p_name;
    pid_t proc_pid;
    oss_timing_printf ("=-=-=- Audio device %d (%s) opened, mode %x -=-=-=",
	     adev->engine_num, adev->name, mode);

    if ((proc_pid = GET_PROCESS_PID (file)) != -1)
	oss_timing_printf ("Opened by PID: %d", proc_pid);

    if ((p_name = GET_PROCESS_NAME (file)) != NULL)
	oss_timing_printf ("Opening process name: %s", p_name);
  }
#endif

#ifdef CONFIG_OSSD
  ossd_event (adev->engine_num, OSSD_EV_OPEN);
#endif

  return adev->engine_num;
}

#ifdef VDEV_SUPPORT
int
oss_audio_open_devfile (int dev, int dev_class, struct fileinfo *file,
			int recursive, int open_flags, int *newdev)
{
/*
 * Open audio device file (by calling oss_audio_open_engine).
 */
  int err, d, n;
  int open_excl = 0;
  adev_p adev;
  int mode = OPEN_READ | OPEN_WRITE;

#ifdef DO_TIMINGS
    oss_timing_printf ("-----> oss_audio_open_devfile(%d, mode=0x%x)", dev, mode);
#endif

  if (file)
    {
      mode = file->mode & O_ACCMODE;
      open_flags = get_open_flags (mode, open_flags, file);
      if (file->acc_flags & O_EXCL)
	{
	  if (excl_policy == 0) open_excl = 1;
#ifdef GET_PROCESS_UID
	  else if ((excl_policy == 1) && (GET_PROCESS_UID () == 0)) open_excl = 1;
#endif
	}
    }

  DDB (cmn_err
       (CE_CONT, "oss_audio_open_devfile(%d, mode=0x%x, excl=%d)\n", dev,
	mode, open_excl));

  if (dev < 0 || dev >= num_audio_devfiles)
    {
      return OSS_ENODEV;
    }
  adev = audio_devfiles[dev];
  dev = adev->engine_num;

  n=0;
  while (adev->d->adrv_redirect != NULL)
     {
	     int next_dev = dev;

	     if (n++ > num_audio_engines)
		{
			cmn_err(CE_CONT, "Recursive audio device redirection\n");
			return OSS_ELOOP;
		}

  	     next_dev = adev->d->adrv_redirect (dev, mode, open_flags);
#ifdef DO_TIMINGS
	     oss_timing_printf ("Engine redirect %d -> %d\n", dev, next_dev);
#endif

	     if (next_dev == dev) /* No change */
		break;

	     if (next_dev < 0 || next_dev >= num_audio_engines)
		return OSS_ENXIO;

	     dev = next_dev;
	     adev = audio_engines[dev];
     }

/*
 * Check Exclusive mode open - do not do any kind of device
 * sharing. For the time being this mode is not supported with devices
 * that do hardware mixing.
 */
  if (open_excl && !(adev->flags & ADEV_HWMIX))
    {
      DDB (cmn_err
	   (CE_CONT, "Exclusive open, engine=%d\n", adev->engine_num));
      if ((dev = oss_audio_open_engine (adev->engine_num, dev_class, file,
					recursive, open_flags, newdev)) < 0)
	{
	  return dev;
	}

      goto done;
    }
  
#ifdef CONFIG_OSS_VMIX
/*
 * Get a vmix engine if the device has vmix support enabled.
 */
  if (!vmix_disabled)
  if (adev->vmix_mixer != NULL)	// Virtual mixer attached
     {
	     int vmix_dev;

	     /*
	      * Create a new vmix client instance.
	      */

	     if ((vmix_dev=vmix_create_client(adev->vmix_mixer))>=0)
	        {
#ifdef DO_TIMINGS
	     oss_timing_printf ("Vmix redirect %d -> %d\n", dev, vmix_dev);
#endif
		      if ((dev = oss_audio_open_engine (vmix_dev, dev_class, file,
							recursive, open_flags, newdev)) < 0)
			{
			  //cmn_err(CE_WARN, "Failed to open vmix engine %d, err=%d\n", vmix_dev, dev);
			  return dev;
			}
#ifdef DO_TIMINGS
	     oss_timing_printf ("Vmix engine opened %d -> %d\n", vmix_dev, dev);
#endif
		
		      goto done;
		}
     }
#endif

#if 0
/*
 * Follow redirection chain for the device.
 *
 * (TODO: This feature was for earlier versions of vmix/softoss and not in use for the time being. However
 * it is possible that some other driver finds use for it in the future).
 */

  if (!open_excl)
     {
	  if (mode == OPEN_WRITE)
	    {
	      redirect = adev->redirect_out;
	    }
	  else if (mode == OPEN_READ)
	    {
	      redirect = adev->redirect_in;
	    }
	  else
	    {
	      /*
	       * Read-write open. Check that redirection for both directions are
	       * identical.
	       */
	      if (adev->redirect_out == adev->redirect_in)
		redirect = adev->redirect_out;
	    }
     }
  DDB (cmn_err
       (CE_CONT, "Redirect=%d (%d, %d)\n", redirect, adev->redirect_out,
	adev->redirect_in));

  if (redirect >= 0 && redirect < num_audio_engines)
    {
      adev = audio_engines[redirect];
      DDB (cmn_err
	   (CE_CONT, "Redirect devfile %d -> engine=%d\n", dev,
	    adev->engine_num));
      dev = redirect;
    }
#endif

  while (adev != NULL)
    {
      DDB (cmn_err (CE_CONT, "Trying engine=%d\n", adev->engine_num));
      if (!adev->enabled)
	return OSS_EAGAIN;

      if (adev->unloaded)
	return OSS_EAGAIN;

      *newdev = adev->engine_num;
      if ((err =
	   oss_audio_open_engine (adev->engine_num, dev_class, file,
				  recursive, open_flags, newdev)) < 0)
	{
	  /* 
	   * Check if the device was busy and if it has a
	   * shadow device.
	   */
	  if (err != OSS_EBUSY || adev->next_out == NULL)
	    return err;
	  adev = adev->next_out;
	}
      else
	{
	  dev = adev->engine_num;
	  DDB (cmn_err (CE_CONT, "Engine %d opened\n", adev->engine_num));
	  goto done;
	}
    }

  return OSS_EBUSY;

done:
/*
 * Try to find which minor number matches this /dev/dsp# device.
 */

  if ((d = oss_find_minor (OSS_DEV_DSP_ENGINE, dev)) < 0)
    {
      oss_audio_release (dev, file);
      return d;
    }

  *newdev = d;
  return dev;
}
#endif

static struct
{
  int sz, nfrags;
} policies[] =
{
  {
  8, 2},			/* 0 */
  {
  16, 2},			/* 1 */
  {
  128, 2},			/* 2 */
  {
  256, 4},			/* 3 */
  {
  512, 4},			/* 4 */
  {
  1024, 0},			/* 5 */
  {
  2048, 0},			/* 6 */
  {
  4096, 0},			/* 7 */
  {
  16384, 0},			/* 8 */
  {
  32768, 0},			/* 9 */
  {
  256 *1024, 0},		/* 10 */
};

static void
setup_fragments (adev_p adev, dmap_p dmap, int direction)
{
  int sz, maxfrags = 100000;
  int min;
  extern int max_intrate;

  if (adev->max_intrate == 0 || adev->max_intrate > max_intrate)
    adev->max_intrate = max_intrate;

  if (dmap->flags & DMAP_FRAGFIXED)
    return;
  dmap->flags |= DMAP_FRAGFIXED;

#ifdef DO_TIMINGS
    oss_timing_printf ("setup_fragments(%d, dir=%d)", adev->engine_num, direction);
#endif

  sz = 1024;

  dmap->low_water_rq = 0;
  dmap->low_water = -1;

  if (dmap->fragsize_rq != 0)	/* SNDCTL_DSP_SETFRAGMENT was called */
    {
      int v;
      v = dmap->fragsize_rq & 0xffff;

      sz = (1 << v);

      if (dmap->expand_factor != UNIT_EXPAND)
	{
	  int sz2;

	  sz2 = (sz * dmap->expand_factor) / UNIT_EXPAND;

	  if (sz < sz2)
	    {
	      while (sz < sz2)
		sz *= 2;
	    }
	  else if (sz > sz2)
	    {
	      while (sz > sz2)
		sz /= 2;
	    }

	}

      if (sz < 8)
	sz = 8;
      if (sz > dmap->buffsize / 2)
	sz = dmap->buffsize / 2;

      maxfrags = (dmap->fragsize_rq >> 16) & 0x7fff;

      if (maxfrags == 0)
	maxfrags = 0x7fff;
      else if (maxfrags < 2)
	maxfrags = 2;

      if (maxfrags < adev->min_fragments)
	maxfrags = adev->min_fragments;
      else if (adev->max_fragments >= 2 && maxfrags > adev->max_fragments)
	maxfrags = adev->max_fragments;

#if 1
      /*
       * Workaround for realplay
       */
      if (adev->open_flags & OF_SMALLFRAGS)
	while (sz > dmap->buffsize / 8)
	  {
	    maxfrags *= 2;
	    sz /= 2;
	  }
#endif
    }

  if (adev->policy >= 0 && adev->policy <= 10)
    {
      sz = policies[adev->policy].sz;
      maxfrags = policies[adev->policy].nfrags;
      if (maxfrags == 0)	/* Unlimited */
	maxfrags = 0xffff;
    }

#if 1
  /* Use short fragments if ossd has registered this device */
  if (adev->ossd_registered)
    sz = 256;
#endif

  /* Sanity check */
  if (adev->min_block && adev->max_block)
    if (adev->min_block > adev->max_block)
      adev->min_block = adev->max_block = 0;

  if (adev->max_block > 0 && sz > adev->max_block)
    sz = adev->max_block;
  if (adev->min_block > 0 && sz < adev->min_block)
    sz = adev->min_block;

  if (sz < 8)
    sz = 8;

  /* Ensure that the interrupt rate is within the limits */

  min = 0;

  if (adev->max_intrate > 0)
    {
      int data_rate;

      data_rate = dmap->data_rate;
      if (data_rate < 8000)
	data_rate = adev->hw_parms.rate * 4;

      min = data_rate / adev->max_intrate;

      if (min > dmap->buffsize / 2)
	min = dmap->buffsize / 2;
#ifdef DO_TIMINGS
	oss_timing_printf ("Max intrate %d -> min buffer %d", adev->max_intrate,
		 min);
#endif
    }

#if 1
  if (dmap->flags & DMAP_COOKED)
    if (min < 256)
      min = 256;
#endif

  if (adev->max_block && min > adev->max_block)
    min = adev->max_block;

  if (sz < min)
    {
      while (sz < min)
	sz *= 2;
      if (adev->max_block > 0 && sz > adev->max_block)
	sz = adev->max_block;
    }

  dmap->fragment_size = sz;
  dmap->nfrags = dmap->buffsize / sz;

  if (dmap == adev->dmap_out)
    {
      if (direction == OPEN_WRITE)
	{
	  if (dmap->nfrags > maxfrags)
	    dmap->nfrags = maxfrags;
	}
    }

  if (adev->d->adrv_setup_fragments)
    {
      adev->d->adrv_setup_fragments (adev->engine_num, dmap, direction);
    }

  dmap->bytes_in_use = dmap->nfrags * dmap->fragment_size;

  if (dmap->nfrags < 2)
    {
      dmap->nfrags = 2;
      dmap->fragment_size = dmap->bytes_in_use / 2;
    }

  while (dmap->nfrags < adev->min_fragments)
    {
      dmap->nfrags *= 2;
      dmap->fragment_size /= 2;
    }

  if (adev->max_fragments >= 2)
    while (dmap->nfrags > adev->max_fragments)
      {
	dmap->nfrags /= 2;
	dmap->fragment_size *= 2;
      }
  while (dmap->nfrags < adev->min_fragments)
    {
      dmap->nfrags *= 2;
      dmap->fragment_size /= 2;
    }

  dmap->bytes_in_use = dmap->nfrags * dmap->fragment_size;

  dmap->bytes_in_use = dmap->nfrags * dmap->fragment_size;
  dmap->low_water_rq = dmap->fragment_size;

#if 1
  if (dmap->nfrags < 4)
#endif
    if (dmap->low_water_rq < dmap->bytes_in_use / 4)
      dmap->low_water_rq = dmap->bytes_in_use / 4;

#ifdef DO_TIMINGS
    oss_timing_printf ("fragsz=%d, nfrags=%d", dmap->fragment_size, dmap->nfrags);
#endif
}

static int
getblksize (adev_p adev)
{
  int size = 4096;
  dmap_p dmap;
  oss_native_word flags, dflags;
  dflags = 0;

  MUTEX_ENTER_IRQDISABLE (adev->mutex, flags);
  if (adev->dmask & DMASK_IN)
    {
      dmap = adev->dmap_in;

      if (!(dmap->flags & DMAP_FRAGFIXED))
	{
	  MUTEX_ENTER (dmap->mutex, dflags);
	  setup_fragments (adev, dmap, OPEN_READ);
	  size = dmap->fragment_size;
	  MUTEX_EXIT (dmap->mutex, dflags);
	}
    }

  if (adev->dmask & DMASK_OUT)
    {
      dmap = adev->dmap_out;

      if (!(dmap->flags & DMAP_FRAGFIXED))
	{
	  MUTEX_ENTER (dmap->mutex, dflags);
	  setup_fragments (adev, dmap, OPEN_WRITE);
	  size = dmap->fragment_size;
	  MUTEX_EXIT (dmap->mutex, dflags);
	}
    }
  MUTEX_EXIT_IRQRESTORE (adev->mutex, flags);

  return size;
}

/*ARGSUSED*/
static oss_uint64_t
get_output_pointer (adev_p adev, dmap_p dmap, int do_adjust)
{
  oss_uint64_t ptr;

  if (adev->d->adrv_get_output_pointer == NULL)
    {
      return dmap->fragment_counter * dmap->fragment_size;
    }

  UP_STATUS (STS_PTR);
  ptr =
    adev->d->adrv_get_output_pointer (adev->engine_num, dmap,
				      PCM_ENABLE_OUTPUT) & ~7;
  DOWN_STATUS (STS_PTR);

  return ptr;
}

static oss_uint64_t
get_input_pointer (adev_p adev, dmap_p dmap, int do_adjust)
{
  oss_uint64_t ptr;

  if (adev->d->adrv_get_input_pointer == NULL)
    return dmap->fragment_counter * dmap->fragment_size;

  ptr =
    adev->d->adrv_get_input_pointer (adev->engine_num, dmap,
				     PCM_ENABLE_INPUT) & ~7;
  if (ptr < dmap->byte_counter)
    ptr += dmap->bytes_in_use;
  ptr %= dmap->bytes_in_use;

  if (do_adjust)
    {
      oss_uint64_t tmp = dmap->byte_counter;

      tmp = (tmp / dmap->bytes_in_use) * dmap->bytes_in_use;
      dmap->byte_counter = tmp + ptr;
    }

  return ptr;
}

static int
get_optr (adev_p adev, dmap_p dmap, ioctl_arg arg)
{
  count_info *info = (count_info *) arg;
  oss_native_word flags;
  oss_uint64_t bytes;

  memset ((char *) info, 0, sizeof (count_info));

  if (dmap->dma_mode != PCM_ENABLE_OUTPUT || !(dmap->flags & DMAP_STARTED))
    return 0;

  MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);

  info->ptr = get_output_pointer (adev, dmap, 1);
  info->blocks = dmap->interrupt_count;
  dmap->interrupt_count = 0;

  bytes = (dmap->byte_counter / dmap->bytes_in_use) * dmap->bytes_in_use;
  bytes += info->ptr;
  if (bytes < dmap->byte_counter)
    bytes += dmap->bytes_in_use;
  info->bytes = (unsigned int) bytes;
  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);

  /* Adjust for format conversions */
  info->ptr = (info->ptr * UNIT_EXPAND) / dmap->expand_factor;
  info->bytes = (info->bytes / dmap->expand_factor) * UNIT_EXPAND;
  info->bytes &= 0x3fffffff;

#ifdef DO_TIMINGS
    oss_timing_printf ("GETOPTR(%d,%d,%d)", info->bytes, info->ptr, info->blocks);
#endif
  return 0;
}

static int
get_iptr (adev_p adev, dmap_p dmap, ioctl_arg arg)
{
  count_info *info = (count_info *) arg;
  oss_native_word flags;
  oss_uint64_t bytes;

  memset ((char *) info, 0, sizeof (count_info));

  if (dmap->dma_mode != PCM_ENABLE_INPUT || !(dmap->flags & DMAP_STARTED))
    return 0;

  MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
  info->ptr = get_input_pointer (adev, dmap, 1);
  info->blocks = dmap->interrupt_count;
  dmap->interrupt_count = 0;

  bytes = (dmap->byte_counter / dmap->bytes_in_use) * dmap->bytes_in_use;
  bytes += info->ptr;
  if (bytes < dmap->byte_counter)
    bytes += dmap->bytes_in_use;
  info->bytes = (unsigned int) bytes;
  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);

  /* Adjust for format conversions */
  info->ptr = (info->ptr * UNIT_EXPAND) / dmap->expand_factor;
  info->bytes = (info->bytes / dmap->expand_factor) * UNIT_EXPAND;
  info->bytes &= 0x3fffffff;

#ifdef DO_TIMINGS
    oss_timing_printf ("GETIPTR(%d,%d,%d)", info->bytes, info->ptr, info->blocks);
#endif
  return 0;
}

#ifndef OSS_NO_LONG_LONG
static int
get_long_optr (adev_p adev, dmap_p dmap, ioctl_arg arg)
{
  oss_count_t *ptr = (oss_count_t *) arg;
  oss_native_word flags;
  oss_uint64_t pos, bytes;

  memset ((char *) ptr, 0, sizeof (*ptr));

  if (dmap->dma_mode != PCM_ENABLE_OUTPUT || !(dmap->flags & DMAP_STARTED))
    return 0;

  ptr->fifo_samples = 0;
  MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
  if (dmap->frame_size < 1)
    dmap->frame_size = 1;
  if (dmap->user_frame_size < 1)
    dmap->user_frame_size = 1;
  pos = get_output_pointer (adev, dmap, 1);
  bytes = (dmap->byte_counter / dmap->bytes_in_use) * dmap->bytes_in_use;
  bytes += pos;
  ptr->samples = (unsigned int) (bytes / dmap->frame_size);

  /* Adjust for format conversions */
  ptr->samples = (ptr->samples * UNIT_EXPAND) / dmap->expand_factor;

  if (adev->d->adrv_local_qlen)
    {
      ptr->fifo_samples =
	adev->d->adrv_local_qlen (adev->engine_num) / dmap->frame_size;
      ptr->fifo_samples =
	(ptr->fifo_samples * UNIT_EXPAND) / dmap->expand_factor;
    }
  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);

  return 0;
}

static int
get_long_iptr (adev_p adev, dmap_p dmap, ioctl_arg arg)
{
  oss_count_t *ptr = (oss_count_t *) arg;
  oss_native_word flags;
  oss_uint64_t pos, bytes;

  memset ((char *) ptr, 0, sizeof (*ptr));

  if (dmap->dma_mode != PCM_ENABLE_INPUT || !(dmap->flags & DMAP_STARTED))
    return 0;

  ptr->fifo_samples = 0;
  MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
  if (dmap->frame_size < 1)
    dmap->frame_size = 1;
  if (dmap->user_frame_size < 1)
    dmap->user_frame_size = 1;
  pos = get_input_pointer (adev, dmap, 1);
  bytes = (dmap->byte_counter / dmap->bytes_in_use) * dmap->bytes_in_use;
  bytes += pos;
  ptr->samples = (unsigned int) (bytes / dmap->frame_size);

  /* Adjust for format conversions */
  ptr->samples = (ptr->samples / dmap->expand_factor) * UNIT_EXPAND;

  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);

  return 0;
}
#endif

static int
get_odelay (adev_p adev, dmap_p dmap, ioctl_arg arg)
{
  oss_int64_t val, pos;
  oss_native_word flags;
  if (dmap->dma_mode != PCM_ENABLE_OUTPUT ||
      (dmap->mapping_flags & DMA_MAP_MAPPED))
    return *arg = (0);

  MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
  pos = get_output_pointer (adev, dmap, 1);
  val = (dmap->byte_counter / dmap->bytes_in_use) * dmap->bytes_in_use;
  val += pos;
  val = dmap->user_counter - val;
  if (val < 0)
    val = 0;
  if (val > dmap->bytes_in_use)
    val = dmap->bytes_in_use;

  if (adev->d->adrv_local_qlen)
    {
      val += adev->d->adrv_local_qlen (adev->engine_num);
    }

  val += dmap->leftover_bytes;
  val = (val * UNIT_EXPAND) / dmap->expand_factor;

  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
#ifdef DO_TIMINGS
    oss_timing_printf ("GETODELAY(%d, %d)", adev->engine_num, val);
#endif
  return *arg = (val);
}

static int audio_space_in_queue (adev_p adev, dmap_p dmap, int count);

static int
get_ospace (adev_p adev, dmap_p dmap, ioctl_arg arg)
{
  audio_buf_info *info = (audio_buf_info *) arg;
  oss_native_word flags;

  memset ((char *) info, 0, sizeof (audio_buf_info));

  if (!(dmap->flags & DMAP_PREPARED))
    {
      setup_fragments (adev, dmap, OPEN_WRITE);
      prepare_output (adev, dmap);
    }
  MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
  info->fragstotal = dmap->nfrags;

  info->fragsize = (dmap->fragment_size * UNIT_EXPAND) / dmap->expand_factor;

  /* Make sure we report full samples */
  info->fragsize =
    ((info->fragsize + dmap->user_frame_size -
      1) / dmap->user_frame_size) * dmap->user_frame_size;

  if (!(adev->open_mode & OPEN_WRITE))
    {
      MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
      cmn_err (CE_WARN,
	       "SNDCTL_DSP_GETOSPACE cannot be called in read-only mode.\n");
#ifdef DO_TIMINGS
      oss_do_timing ("GETOSPACE: Bad access mode - return EACCES");
#endif
      return OSS_EACCES;
    }

  if (dmap->mapping_flags & DMA_MAP_MAPPED)
    {
      MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
      cmn_err (CE_WARN,
	       "SNDCTL_DSP_GETOSPACE cannot be called in mmap mode.\n");
#ifdef DO_TIMINGS
      oss_do_timing ("GETOSPACE: mmap access mode - return EPERM");
#endif
      oss_audio_set_error (adev->engine_num, E_PLAY,
			   OSSERR (1000, "GETOSPACE called in mmap mode"), 0);
      /* Errordesc:
       * Do not call SNDCTL_DSP_GETOSPACE in mmap mode. Use SNDCTL_DSP_GETOPTR
       * instead. SNDCTL_DSP_GETOSPACE is defined only for applications that
       * use the normal write() method.
       * Applications that use mmap can call SNDCTL_DSP_GETOSPACE before calling
       * mmap to get the actual buffer size.
       */
      return OSS_EPERM;
    }

  if (!(dmap->flags & DMAP_STARTED))
    {
      int bytes;
      bytes = (int) ((long long) dmap->bytes_in_use - dmap->user_counter);
#ifdef DO_TIMINGS
      {
	oss_do_timing ("GETOSPACE: Not started - ignore device count");
	oss_timing_printf ("bytes_in_use=%d", dmap->bytes_in_use);
	oss_timing_printf ("user_counter=%lld", dmap->user_counter);
	oss_timing_printf ("raw bytes=%d", bytes);
      }
#endif
      bytes = (bytes / dmap->expand_factor) * UNIT_EXPAND;	/* Round downwards */
      bytes = (bytes / dmap->user_frame_size) * dmap->user_frame_size;	/* Truncate to frame size */
      info->bytes = bytes;
      info->fragments = info->fragstotal = dmap->nfrags;
      if (info->bytes > info->fragsize * info->fragstotal)
	info->bytes = info->fragsize * info->fragstotal;

      MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
      return 0;
    }

  info->bytes = audio_space_in_queue (adev, dmap, 0);
#ifdef DO_TIMINGS
    oss_timing_printf ("audio_space_in_queue returned %d", info->bytes);
#endif

  if (info->bytes < dmap->low_water)
    info->bytes = 0;

  if (adev->dmap_out->flags & DMAP_COOKED)
    {

      if (dmap->tmpbuf_ptr > 0)
	info->bytes -= dmap->tmpbuf_ptr;
    }

  if ((adev->dmap_out->flags & DMAP_COOKED) && info->bytes <
      dmap->fragment_size / 2)
    {
#ifdef DO_TIMINGS
      oss_do_timing ("GETOSPACE: Buffer full");
#endif
      info->bytes = 0;
    }

  if (info->bytes < 0)
    info->bytes = 0;

  info->bytes = (info->bytes * UNIT_EXPAND) / dmap->expand_factor;
  info->bytes = (info->bytes / dmap->user_frame_size) * dmap->user_frame_size;	/* Truncate to frame size */
  if (dmap->flags & DMAP_COOKED)
    {
      /*
       * Reserve some space for format conversions. Sample rate conversions
       * may not always be able to take the "last" sample with fractional conversion
       * ratios.
       */
      if (info->bytes >= dmap->frame_size)
	info->bytes -= dmap->frame_size;	/* Substract one sample. */
    }
  info->fragments = info->bytes / info->fragsize;

  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);

  if (info->bytes > info->fragsize * info->fragstotal)
    info->bytes = info->fragsize * info->fragstotal;

  return 0;
}

static int
get_ispace (adev_p adev, dmap_p dmap, ioctl_arg arg)
{
  audio_buf_info *info = (audio_buf_info *) arg;
  oss_native_word flags;

  memset ((char *) info, 0, sizeof (audio_buf_info));

  setup_fragments (adev, dmap, OPEN_READ);
  prepare_input (adev, dmap);
  MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
  info->fragstotal = dmap->nfrags;
  info->fragsize = (dmap->fragment_size * UNIT_EXPAND) / dmap->expand_factor;

  /* Make sure we report full samples */
  info->fragsize =
    ((info->fragsize + dmap->user_frame_size -
      1) / dmap->user_frame_size) * dmap->user_frame_size;

  if (!(adev->open_mode & OPEN_READ))
    {
      MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
      cmn_err (CE_WARN,
	       "SNDCTL_DSP_GETISPACE cannot be called in write-only mode.\n");
      return OSS_EACCES;
    }

  if (dmap->mapping_flags & DMA_MAP_MAPPED)
    {
      MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
      cmn_err (CE_WARN,
	       "SNDCTL_DSP_GETISPACE cannot be called in mmap mode.\n");
      oss_audio_set_error (adev->engine_num, E_REC,
			   OSSERR (1003, "GETISPACE called in mmap mode"), 0);
      /* Errordesc:
       * Do not call SNDCTL_DSP_GETISPACE in mmap mode.
       * SNDCTL_DSP_GETISPACE is defined only for applications that
       * use the normal write() method.
       * Applications that use mmap can call SNDCTL_DSP_GETISPACE before calling
       * mmap to get the actual buffer size.
       */
      return OSS_EPERM;
    }

  if (!(dmap->flags & DMAP_STARTED))
    {
/*
 * A stupid application has called GETISPACE before recording has started.
 * The right behaviour would definitely be returning bytes=0. However
 * reporting one ready fragment will keep poor programmers happy.
 * After all this is a minor error because no properly written application
 * should ever call GETISPACE before starting recording.
 */
      info->bytes = info->fragsize;
      info->fragments = 1;

      MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
      if (info->bytes > info->fragsize * info->fragstotal)
	info->bytes = info->fragsize * info->fragstotal;
      if (adev->getispace_error_count++ == 1)	/* Second time this happens */
	{
	  oss_audio_set_error (adev->engine_num, E_REC,
			       OSSERR (1004,
				       "GETISPACE called before recording has been started"),
			       0);
	  /*
	   * Errordesc:
	   * There is no recoded data available before recording has been started.
	   * This would result in situation where the application waits infinitely
	   * for recorded data that never arrives. As a workaround 
	   * SNDCTL_DSP_GETISPACE will fake that one fragment is already available
	   * to read. This in turn makes the application to block incorrectly.
	   *
	   * Applications must start recording before calling SNDCTL_DSP_GETISPACE
	   * if they are trying to avoid blocking. This can be done by calling
	   * SNDCTL_DSP_SETTRIGGER.
	   *
	   * However applications going to use mmap can/should call
	   * SNDCTL_DSP_GETISPACE in the beginning to find out how large buffer to
	   * map. In such case this event is a false alarm.
	   */
	}
      return 0;
    }

  info->bytes = (unsigned int) (dmap->byte_counter - dmap->user_counter);

  if (dmap->flags & DMAP_COOKED)
    {
      /* Count the already converted bytes in the tmp buffer */
      int nn = dmap->tmpbuf_len - dmap->tmpbuf_ptr;

      if (nn > 0)
	info->bytes += nn;
    }

  if (info->bytes > dmap->bytes_in_use)
    info->bytes = dmap->bytes_in_use;

  info->bytes = (info->bytes * UNIT_EXPAND) / dmap->expand_factor;
  info->bytes = (info->bytes / dmap->user_frame_size) * dmap->user_frame_size;
  info->fragments = info->bytes / info->fragsize;
  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);

  if (info->bytes > info->fragsize * info->fragstotal)
    info->bytes = info->fragsize * info->fragstotal;

#ifdef DO_TIMINGS
    oss_timing_printf ("GETISPACE(%d,%d,%d,%d)", info->bytes, info->fragments,
	     info->fragsize, info->fragstotal);
#endif
  return 0;
}

#define xrand(seed) (seed=1664525*seed+1013904223)

static void
setfragment_error (int dev)
{
  if (audio_engines[dev]->setfragment_warned)
    return;

  oss_audio_set_error (dev, E_PLAY,
		       OSSERR (1011,
			       "SNDCTL_DSP_SETFRAGMENT was called too late."),
		       0);
  /*
   * Errordesc: The SNDCTL_DSP_SETFRAGMENT call is only valid immediately after
   * opening the device. It can only be called once without reopening
   * the audio device. 
   *
   * Calling read/write or certain ioctl calls will lock the fragment size/count
   * to some values which makes changing it impossible.
   */

#ifdef DO_TIMINGS
  oss_do_timing ("Setfragment called twice");
#endif

  audio_engines[dev]->setfragment_warned = 1;
}

static int
handle_syncgroup (adev_p adev, oss_syncgroup * group)
{
  int id, sync_dev;
  adev_p sync_adev;

  if (adev->sync_group != 0)
    {
      return OSS_EBUSY;
    }

  if (group->id == 0)
    {
      if (adev->engine_num > SYNC_DEVICE_MASK)
	{
	  cmn_err (CE_WARN, "Bad device number %d\n", adev->engine_num);
	  return OSS_EIO;
	}

      id = xrand (sync_seed) & ~SYNC_DEVICE_MASK; /* Clear the engine number field */
      id |= adev->engine_num;

      group->id = id;
      sync_dev = adev->engine_num;
      adev->sync_next = NULL;
    }
  else
    {
      id = group->id;
      sync_dev = id & SYNC_DEVICE_MASK;
    }

  if (sync_dev < 0 || sync_dev >= num_audio_engines)
    {
      group->id = 0;
      return OSS_EINVAL;
    }

  sync_adev = audio_engines[sync_dev];

  if (sync_dev == adev->engine_num)
    adev->sync_flags |= SYNC_MASTER;
  else
    {
      if (!(sync_adev->sync_flags & SYNC_MASTER))
	{
	  return OSS_EINVAL;
	}

      if (sync_adev->sync_group != id)
	{
	  return OSS_EINVAL;
	}

      adev->sync_flags |= SYNC_SLAVE;
    }

  group->mode &= (PCM_ENABLE_INPUT | PCM_ENABLE_OUTPUT);
  group->mode &= adev->open_mode;
  if (group->mode == 0)
    {
      adev->sync_flags = 0;
      return OSS_EINVAL;
    }

  adev->sync_mode = group->mode;
  adev->sync_group = id;
  if (adev->d->adrv_sync_control != NULL)
    adev->d->adrv_sync_control (adev->engine_num, SYNC_ATTACH,
				adev->sync_mode);

  if (adev != sync_adev)
    {
      adev->sync_next = sync_adev->sync_next;
      sync_adev->sync_next = adev;
    }

  adev->go = 0;

  return 0;
}

static int
handle_syncstart (int orig_dev, int group)
{
  int master_dev, arg;
  adev_p adev, next;

  master_dev = group & SYNC_DEVICE_MASK;
  if (master_dev < 0 || master_dev >= num_audio_engines)
    return OSS_EINVAL;

  adev = audio_engines[master_dev];

  if (!(adev->sync_flags & SYNC_MASTER) ||
     master_dev != orig_dev)
     {
	/*
	 * Potential attack. SYNCSTART was called on wrong file descriptor.
	 */
	return OSS_EPERM;
     }

  if (adev->sync_group != group)
    return OSS_EINVAL;

/*
 * Pass 1: Inform all devices about the actual start command to come soon.
 */

  while (adev != NULL)
    {

      if (adev->sync_group == group)
	{
	  adev->go = 0;

	  if (adev->d->adrv_sync_control)
	    {
	      if (adev->sync_mode & PCM_ENABLE_INPUT)
		{
		  if (!(adev->dmap_in->flags & DMAP_PREPARED))
		    {
		      adev->dmap_in->dma_mode = PCM_ENABLE_INPUT;
		      prepare_input (adev, adev->dmap_in);
		    }
		  launch_input (adev, adev->dmap_in);
		}
	      if (adev->sync_mode & PCM_ENABLE_OUTPUT)
		{
		  dmap_p dmap = adev->dmap_out;
		  int err;

		  if (adev->dmap_out->mapping_flags & DMA_MAP_MAPPED)
		    dmap->dma_mode = PCM_ENABLE_OUTPUT;
		  if ((err = prepare_output (adev, adev->dmap_out)) < 0)
		    return err;
		  launch_output (adev, adev->dmap_out);
		}
	      adev->d->adrv_sync_control (adev->engine_num, SYNC_PREPARE,
					  adev->sync_mode);
	    }
	  else
	    {
	      arg = 0;
	      oss_audio_ioctl (adev->engine_num, NULL, SNDCTL_DSP_SETTRIGGER,
			       (ioctl_arg) & arg);
	    }
	}
      else
	{
	  cmn_err (CE_NOTE, "Broken sync chain\n");
	}

      adev = adev->sync_next;
    }

/*
 * Pass 2: Deliver the actual start commands.
 */

  adev = audio_engines[master_dev];

  while (adev != NULL)
    {

      if (adev->sync_group == group)
	{
	  adev->go = 1;

	  if (adev->d->adrv_sync_control)
	    adev->d->adrv_sync_control (adev->engine_num, SYNC_TRIGGER,
					adev->sync_mode);
	  else
	    {
	      arg = adev->sync_mode;
	      oss_audio_ioctl (adev->engine_num, NULL, SNDCTL_DSP_SETTRIGGER,
			       (ioctl_arg) & arg);
	    }
	}
      else
	{
	  /* Skip this one */
	  adev = adev->sync_next;
	  continue;
	}

      next = adev->sync_next;
      adev->sync_next = NULL;
      adev->sync_flags = 0;
      adev->sync_group = 0;

      adev = next;
    }
  return 0;
}

#ifdef DO_TIMINGS
static char *
find_ioctl_name (unsigned int cmd, ioctl_arg val)
{
  static char tmp[32];

  typedef struct
  {
    unsigned int code;
    char *name;
    int flags;
#define IOF_DEC		0x00000001
#define IOF_HEX		0x00000002
  } ioctl_def_t;

  static ioctl_def_t call_names[] = {
    {SNDCTL_DSP_HALT, "SNDCTL_DSP_HALT"},
    {SNDCTL_DSP_SYNC, "SNDCTL_DSP_SYNC"},
    {SNDCTL_DSP_SPEED, "SNDCTL_DSP_SPEED", IOF_DEC},
    {SNDCTL_DSP_STEREO, "SNDCTL_DSP_STEREO", IOF_DEC},
    {SNDCTL_DSP_GETBLKSIZE, "SNDCTL_DSP_GETBLKSIZE"},
    {SNDCTL_DSP_CHANNELS, "SNDCTL_DSP_CHANNELS", IOF_DEC},
    {SNDCTL_DSP_POST, "SNDCTL_DSP_POST"},
    {SNDCTL_DSP_SUBDIVIDE, "SNDCTL_DSP_SUBDIVIDE"},
    {SNDCTL_DSP_SETFRAGMENT, "SNDCTL_DSP_SETFRAGMENT", IOF_HEX},
    {SNDCTL_DSP_GETFMTS, "SNDCTL_DSP_GETFMTS"},
    {SNDCTL_DSP_SETFMT, "SNDCTL_DSP_SETFMT", IOF_HEX},
    {SNDCTL_DSP_GETOSPACE, "SNDCTL_DSP_GETOSPACE"},
    {SNDCTL_DSP_GETISPACE, "SNDCTL_DSP_GETISPACE"},
    {SNDCTL_DSP_GETCAPS, "SNDCTL_DSP_GETCAPS"},
    {SNDCTL_DSP_GETTRIGGER, "SNDCTL_DSP_GETTRIGGER"},
    {SNDCTL_DSP_SETTRIGGER, "SNDCTL_DSP_SETTRIGGER", IOF_HEX},
    {SNDCTL_DSP_GETIPTR, "SNDCTL_DSP_GETIPTR"},
    {SNDCTL_DSP_GETOPTR, "SNDCTL_DSP_GETOPTR"},
    {SNDCTL_DSP_SETSYNCRO, "SNDCTL_DSP_SETSYNCRO", IOF_HEX},
    {SNDCTL_DSP_SETDUPLEX, "SNDCTL_DSP_SETDUPLEX"},
    {SNDCTL_DSP_GETODELAY, "SNDCTL_DSP_GETODELAY"},
    {SNDCTL_DSP_GETPLAYVOL, "SNDCTL_DSP_GETPLAYVOL"},
    {SNDCTL_DSP_SETPLAYVOL, "SNDCTL_DSP_SETPLAYVOL", IOF_HEX},
    {SNDCTL_DSP_GETRECVOL, "SNDCTL_DSP_GETRECVOL"},
    {SNDCTL_DSP_SETRECVOL, "SNDCTL_DSP_SETRECVOL", IOF_HEX},
    {SNDCTL_DSP_GETERROR, "SNDCTL_DSP_GETERROR"},
    {SNDCTL_DSP_READCTL, "SNDCTL_DSP_READCTL"},
    {SNDCTL_DSP_WRITECTL, "SNDCTL_DSP_WRITECTL"},
    {SNDCTL_DSP_SYNCGROUP, "SNDCTL_DSP_SYNCGROUP"},
    {SNDCTL_DSP_SYNCSTART, "SNDCTL_DSP_SYNCSTART"},
    {SNDCTL_DSP_COOKEDMODE, "SNDCTL_DSP_COOKEDMODE", IOF_DEC},
    {SNDCTL_DSP_SILENCE, "SNDCTL_DSP_SILENCE"},
    {SNDCTL_DSP_SKIP, "SNDCTL_DSP_SKIP"},
    {SNDCTL_DSP_GETCHANNELMASK, "SNDCTL_DSP_GETCHANNELMASK"},
    {SNDCTL_DSP_BIND_CHANNEL, "SNDCTL_DSP_BIND_CHANNEL"},
    {SNDCTL_DSP_HALT_INPUT, "SNDCTL_DSP_HALT_INPUT"},
    {SNDCTL_DSP_HALT_OUTPUT, "SNDCTL_DSP_HALT_OUTPUT"},
    {SNDCTL_DSP_LOW_WATER, "SNDCTL_DSP_LOW_WATER"},
#ifndef OSS_NO_LONG_LONG
    {SNDCTL_DSP_CURRENT_IPTR, "SNDCTL_DSP_CURRENT_IPTR"},
    {SNDCTL_DSP_CURRENT_OPTR, "SNDCTL_DSP_CURRENT_OPTR"},
#endif
    {SNDCTL_DSP_GET_RECSRC, "SNDCTL_DSP_GET_RECSRC"},
    {SNDCTL_DSP_SET_RECSRC, "SNDCTL_DSP_SET_RECSRC"},
    {SNDCTL_DSP_GET_RECSRC_NAMES, "SNDCTL_DSP_GET_RECSRC_NAMES"},
    {SNDCTL_DSP_GET_PLAYTGT, "SNDCTL_DSP_GET_PLAYTGT"},
    {SNDCTL_DSP_SET_PLAYTGT, "SNDCTL_DSP_SET_PLAYTGT"},
    {SNDCTL_DSP_GET_PLAYTGT_NAMES, "SNDCTL_DSP_GET_PLAYTGT_NAMES"},
    {0, NULL}
  };

  int i;
  for (i = 0; call_names[i].code != 0; i++)
    if (call_names[i].code == cmd)
      {
	int flags = call_names[i].flags;

	if (flags & IOF_DEC)
	  {
	    sprintf (tmp, "%s,  *%d", call_names[i].name, *val);
	    return tmp;
	  }

	if (flags & IOF_HEX)
	  {
	    sprintf (tmp, "%s, *0x%08x", call_names[i].name, *val);
	    return tmp;
	  }

	return call_names[i].name;
      }

  sprintf (tmp, "Unknown %08x", cmd);
  return tmp;
}
#endif

/*ARGSUSED*/
int
oss_encode_enum (oss_mixer_enuminfo * ei, const char *s, int version)
{
  int n = 1, l;
  int i;

  memset (ei, 0, sizeof (*ei));	/* Wipe out everything */

  strncpy (ei->strings, s, sizeof (ei->strings) - 1);
  ei->strings[sizeof (ei->strings) - 1] = 0;

  ei->strindex[0] = 0;

  l = strlen (ei->strings);
  for (i = 0; i < l; i++)
    {
      if (ei->strings[i] == ' ')
	{
	  ei->strindex[n++] = i + 1;
	  ei->strings[i] = 0;
	}
    }

  ei->nvalues = n;

  return 0;
}

static int
get_legacy_recsrc_names (int dev, oss_mixer_enuminfo * ei)
{
  static const char *labels[] = SOUND_DEVICE_NAMES;

  int i, mixer_dev, recmask, devmask, caps, n;
  char *s;

  if (audio_engines[dev]->mixer_dev < 0)	/* No mixer */
    return 0;

  mixer_dev = audio_engines[dev]->mixer_dev;


  if (oss_legacy_mixer_ioctl
      (mixer_dev, -1, SOUND_MIXER_READ_CAPS, (ioctl_arg) & caps) < 0)
    caps = 0;			/* Error */

  if (caps & SOUND_CAP_NORECSRC)
    return 0;

  if (oss_legacy_mixer_ioctl
      (mixer_dev, -1, SOUND_MIXER_READ_DEVMASK, (ioctl_arg) & devmask) < 0)
    return 0;			/* Error */

  if (oss_legacy_mixer_ioctl
      (mixer_dev, -1, SOUND_MIXER_READ_RECMASK, (ioctl_arg) & recmask) < 0)
    return 0;			/* Error */

  recmask &= devmask;
  if (recmask == 0)
    return 0;

  n = 0;
  s = ei->strings;

  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
    if (recmask & (1 << i))	/* This control is also recording device */
      {
	 /*LINTED*/ ei->strindex[n] = s - ei->strings;

	strcpy (s, labels[i]);
	s += strlen (s);
	*s++ = 0;
	n++;
      }

  ei->nvalues = n;

  return (n > 0);
}

static int
get_legacy_recsrc (int dev)
{
  int i, mixer_dev, recmask, recsrc, caps, n;

  if (audio_engines[dev]->mixer_dev < 0)	/* No mixer */
    return 0;

  mixer_dev = audio_engines[dev]->mixer_dev;


  if (oss_legacy_mixer_ioctl
      (mixer_dev, -1, SOUND_MIXER_READ_CAPS, (ioctl_arg) & caps) < 0)
    caps = 0;			/* Error */

  if (caps & SOUND_CAP_NORECSRC)
    return 0;

  if (oss_legacy_mixer_ioctl
      (mixer_dev, -1, SOUND_MIXER_READ_RECSRC, (ioctl_arg) & recsrc) < 0)
    return 0;			/* Error */

  if (oss_legacy_mixer_ioctl
      (mixer_dev, -1, SOUND_MIXER_READ_RECMASK, (ioctl_arg) & recmask) < 0)
    return 0;			/* Error */

  if (recmask == 0 || recsrc == 0)
    return 0;

  n = 0;

  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
    if (recmask & (1 << i))	/* This control is also recording device */
      {
	if (recsrc & (1 << i))	/* It was this one */
	  return n;
	n++;
      }


  return 0;
}

static int
set_legacy_recsrc (int dev, int val)
{
  int i, mixer_dev, recmask, recsrc, caps, n;

  if (audio_engines[dev]->mixer_dev < 0)	/* No mixer */
    return 0;

  mixer_dev = audio_engines[dev]->mixer_dev;


  if (oss_legacy_mixer_ioctl
      (mixer_dev, -1, SOUND_MIXER_READ_CAPS, (ioctl_arg) & caps) < 0)
    caps = 0;			/* Error */

  if (caps & SOUND_CAP_NORECSRC)
    return 0;

  if (oss_legacy_mixer_ioctl
      (mixer_dev, -1, SOUND_MIXER_READ_RECMASK, (ioctl_arg) & recmask) < 0)
    return 0;			/* Error */

  if (recmask == 0)
    return 0;

  n = 0;

  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
    if (recmask & (1 << i))	/* This control is also recording device */
      {
	if (n == val)
	  {
	    recsrc = (1 << i);

	    if (oss_legacy_mixer_ioctl
		(mixer_dev, -1, SOUND_MIXER_WRITE_RECSRC,
		 (ioctl_arg) & recsrc) < 0)
	      return 0;		/* Error */

	    return 1;
	  }
	n++;
      }


  return 0;
}

/*ARGSUSED*/
int
oss_audio_ioctl (int dev, struct fileinfo *bogus,
		 unsigned int cmd, ioctl_arg arg)
{
  int val, err;
  int mixdev;
  int ret;
  adev_p adev;
  dmap_p dmapin, dmapout;
  oss_native_word flags;
#ifdef DO_TIMINGS
  oss_timing_printf ("oss_audio_ioctl(%d, %s)", dev, find_ioctl_name (cmd, arg));
#endif

  if (cmd == OSS_GETVERSION)
    return *arg = OSS_VERSION;

  UP_STATUS (STS_IOCTL);
  DOWN_STATUS (STS_IOCTL);
  sync_seed++;

  if (dev < 0 || dev >= num_audio_engines)
    return OSS_ENXIO;

  adev = audio_engines[dev];
  if (adev->unloaded)
    return OSS_ENODEV;
  if (!adev->enabled)
    return OSS_ENXIO;

  if (adev->d->adrv_ioctl_override != NULL)
  {
  /*
   * Use the ioctl override function if available. However process the request
   * in the usual way if the override function returned OSS_EAGAIN. It may
   * be possible that the override function has modified the parameters
   * before returning.
   */
	  if ((err = adev->d->adrv_ioctl_override(dev, cmd, arg)) != OSS_EAGAIN)
	     return err;
  }

  dmapout = adev->dmap_out;
  dmapin = adev->dmap_in;

/*
 * Handle mixer ioctl calls on audio fd.
 */

  if (cmd == SOUND_MIXER_WRITE_PCM) cmd = SNDCTL_DSP_SETPLAYVOL;
  if (cmd == SOUND_MIXER_WRITE_RECLEV) cmd = SNDCTL_DSP_SETRECVOL;
  if (cmd == SOUND_MIXER_READ_PCM) cmd = SNDCTL_DSP_GETPLAYVOL;
  if (cmd == SOUND_MIXER_READ_RECLEV) cmd = SNDCTL_DSP_GETRECVOL;

  if ((mixdev = adev->mixer_dev) != -1)
    {
      if (((cmd >> 8) & 0xff) == 'M' && num_mixers > 0)	/* Mixer ioctl */
	if ((ret = oss_legacy_mixer_ioctl (mixdev, dev, cmd, arg)) != OSS_EINVAL)
	  return ret;
    }

  if (((cmd >> 8) & 0xff) == 'X')	/* Mixer extension API */
     return oss_mixer_ext (adev->engine_num, OSS_DEV_DSP, cmd, arg);

  switch (cmd)
    {
    case SNDCTL_DSP_SYNC:
      oss_audio_sync (adev);
      return 0;
      break;

    case SNDCTL_DSP_POST:
      oss_audio_post (adev);
      return 0;
      break;

    case SNDCTL_DSP_HALT:
      audio_reset_adev (adev);
      return 0;
      break;

    case SNDCTL_DSP_HALT_INPUT:
      audio_reset_input (adev);
      return 0;
      break;

    case SNDCTL_DSP_LOW_WATER:
      val = *arg;
      if (adev->open_mode & OPEN_READ)
	adev->dmap_in->low_water = val;
      if (adev->open_mode & OPEN_WRITE)
	adev->dmap_out->low_water = val;
      return 0;
      break;

    case SNDCTL_DSP_HALT_OUTPUT:
      audio_reset_output (adev);
      return 0;
      break;

    case SNDCTL_DSP_GETFMTS:
      switch (adev->open_mode & (OPEN_READ | OPEN_WRITE))
	{
	case OPEN_WRITE:
	  return *arg = (adev->oformat_mask);
	  break;

	case OPEN_READ:
	  return *arg = (adev->oformat_mask);
	  break;

	default:
	  return *arg = (adev->xformat_mask);
	  break;
	}
      break;

    case SNDCTL_DSP_SETFMT:
      val = *arg;
      switch (adev->open_mode & (OPEN_READ | OPEN_WRITE))
	{
	case OPEN_WRITE:
	  return *arg = (oss_audio_set_format (dev, val,
					       audio_engines
					       [dev]->oformat_mask));
	  break;

	case OPEN_READ:
	  return *arg = (oss_audio_set_format (dev, val,
					       audio_engines
					       [dev]->iformat_mask));
	  break;

	default:
	  return *arg = (oss_audio_set_format (dev, val,
					       audio_engines
					       [dev]->oformat_mask &
					       audio_engines
					       [dev]->iformat_mask));
	  break;
	}

    case SNDCTL_DSP_GETOSPACE:
      if (!(adev->dmask & DMASK_OUT))
	{
	  oss_audio_set_error (adev->engine_num, E_PLAY,
			       OSSERR (1001,
				       "GETOSPACE called in read-only mode"),
			       0);
	  /* Errordesc: SNDCTL_DSP_GETOSPACE is not defined in read-only access mode */
	  return OSS_ENOTSUP;
	}
      ret = get_ospace (adev, dmapout, arg);
#ifdef DO_TIMINGS
      {
	audio_buf_info *info = (audio_buf_info *) arg;
	oss_timing_printf ("GETOSPACE(b=%d,f=%d,fsz=%d,ft=%d)=%d", info->bytes,
		 info->fragments, info->fragsize, info->fragstotal, ret);
	oss_timing_printf ("Low water %d, ap flags=%x, tmpbuf=%d/%d",
		 dmapout->low_water, adev->open_flags, dmapout->tmpbuf_ptr,
		 dmapout->tmpbuf_len);
      }
#endif
      return ret;
      break;

    case SNDCTL_DSP_GETISPACE:
      if (!(adev->dmask & DMASK_IN))
	{
	  oss_audio_set_error (adev->engine_num, E_REC,
			       OSSERR (1002,
				       "GETISPACE called in write-only mode"),
			       0);
	  /*
	   * Errordesc: SNDCTL_DSP_GETISPACE has no defined meaning when the audio
	   * device is opened in write-only mode.
	   */
	  return OSS_ENOTSUP;
	}

      return get_ispace (adev, dmapin, arg);
      break;

    case SNDCTL_DSP_GETODELAY:
      if (!(adev->dmask & DMASK_OUT))
	{
	  oss_audio_set_error (adev->engine_num, E_PLAY,
			       OSSERR (1005,
				       "GETODELAY called in read-only mode"),
			       0);
	  return OSS_ENOTSUP;
	}
      return get_odelay (adev, dmapout, arg);
      break;

    case SNDCTL_DSP_SETDUPLEX:
      /*
       * Note! SNDCTL_DSP_SETDUPLEX has not been implemented by any driver for years.
       *       The call is still implemented in audio core but it may get removed in the
       *       future.
       */
      if (adev->open_mode != OPEN_READWRITE)
	{
	  oss_audio_set_error (adev->engine_num, E_PLAY,
			       OSSERR (1006,
				       "SETDUPLEX called in non-read/write mode"),
			       0);
	  return OSS_ENOTSUP;
	}
      if (adev->flags & ADEV_DUPLEX)
	{
	  if (adev->d->adrv_ioctl == NULL)
	    return 0;
	  val = adev->d->adrv_ioctl (dev, cmd, arg);
	  if (val == OSS_EINVAL)
	    return 0;
	  else
	    return val;
	}
      else
	{
	  return OSS_ENOTSUP;
	}
      break;

    case SNDCTL_DSP_COOKEDMODE:
      val = *arg;

      if (adev->flags & ADEV_NONINTERLEAVED)
	 val=1;

      adev->cooked_enable = !!val;
      if (adev->d->adrv_ioctl != NULL)
	adev->d->adrv_ioctl (dev, cmd, arg);
#ifdef DO_TIMINGS
      if (adev->cooked_enable)
	oss_do_timing ("Setting cooked mode ON");
      else
	oss_do_timing ("Setting cooked mode OFF");
#endif
      return 0;
      break;

    case SNDCTL_DSP_GETCAPS:
      {
	int info;
	info = audio_engines[dev]->caps;
	info |= 2;		/* Revision level of this ioctl() */

#if 0
	if (!(adev->flags & ADEV_VIRTUAL) && !adev->d->adrv_local_qlen)
#endif
	  info |= PCM_CAP_REALTIME;


	if (!(adev->flags & ADEV_NOINPUT))
	  info |= PCM_CAP_INPUT;

	if (!(adev->flags & ADEV_NOOUTPUT))
	  info |= PCM_CAP_OUTPUT;

	if ((adev->flags & ADEV_VIRTUAL))
	  info |= PCM_CAP_VIRTUAL;

	if (!(adev->flags & ADEV_NOINPUT) && !(adev->flags & ADEV_NOOUTPUT))
	  if (adev->flags & ADEV_DUPLEX && adev->open_mode == OPEN_READWRITE)
	    info |= PCM_CAP_DUPLEX;

	if (dev > 0)
	  if (adev->flags & ADEV_SPECIAL)
	    info |= PCM_CAP_SPECIAL;

	if (dev < num_audio_engines - 1)
	  {
	    if (audio_engines[dev + 1]->flags & ADEV_SHADOW)
	      info |= PCM_CAP_MULTI;
	  }

	if (adev->d->adrv_local_qlen)	/* Device has hidden buffers */
	  info |= PCM_CAP_BATCH;

	if (adev->d->adrv_trigger)	/* Supports SETTRIGGER */
	  info |= PCM_CAP_TRIGGER;

#ifdef ALLOW_BUFFER_MAPPING
	info |= PCM_CAP_MMAP;
#endif
	if (!(adev->flags & ADEV_NOINPUT))
	  info |= PCM_CAP_INPUT;

	if (!(adev->flags & ADEV_NOOUTPUT))
	  info |= PCM_CAP_OUTPUT;

	if (adev->d->adrv_bind != NULL)
	  info |= PCM_CAP_BIND;
	/*
	 * TODO: ADEV_DEFAULT is not the right way to find out 
	 * PCM_CAP_DEFAULT devices. A new ADEV_ flag should be defined
	 * for this purpose.
	 */
	if (adev->flags & ADEV_DEFAULT)
	  info |= PCM_CAP_DEFAULT;

	return *arg = (info);
      }
      break;

    case SNDCTL_DSP_NONBLOCK:
      adev->forced_nonblock = 1;
      return 0;
      break;

    case SNDCTL_DSP_GETCHANNELMASK:
    case SNDCTL_DSP_BIND_CHANNEL:
      if (adev->d->adrv_bind == NULL)
	return OSS_EINVAL;
      return adev->d->adrv_bind (adev->engine_num, cmd, arg);
      break;

    case SNDCTL_DSP_SETTRIGGER:
      {
	int val;

	if (!adev->d->adrv_trigger)
	  {
	    cmn_err (CE_NOTE, "Device %d doesn't have trigger capability\n",
		     adev->engine_num);
	    return OSS_EIO;
	  }

	val = *arg;
	val &= PCM_ENABLE_INPUT | PCM_ENABLE_OUTPUT;
	val &= adev->open_mode;

	if ((val & adev->open_mode) != (adev->enable_bits & adev->open_mode))
	  {
	    if ((val & PCM_ENABLE_OUTPUT)
		&& !(adev->enable_bits & PCM_ENABLE_OUTPUT))
	      {
		dmap_p dmap = adev->dmap_out;
		oss_native_word flags;
		if (adev->dmap_out->mapping_flags & DMA_MAP_MAPPED)
		  dmap->dma_mode = PCM_ENABLE_OUTPUT;
		if ((err = prepare_output (adev, adev->dmap_out)) < 0)
		  return err;
		launch_output (adev, adev->dmap_out);
		MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
		adev->enable_bits &= PCM_ENABLE_OUTPUT;
		adev->enable_bits |= val & PCM_ENABLE_OUTPUT;
		MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
	      }

	    if ((val & PCM_ENABLE_INPUT)
		&& !(adev->enable_bits & PCM_ENABLE_INPUT))
	      {
		dmap_p dmap = adev->dmap_in;
		oss_native_word flags;
		MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
		dmap->dma_mode = PCM_ENABLE_INPUT;
		MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
		if ((err = prepare_input (adev, adev->dmap_in)) < 0)
		  return err;
		MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
		launch_input (adev, adev->dmap_in);
		adev->enable_bits &= PCM_ENABLE_INPUT;
		adev->enable_bits |= val & PCM_ENABLE_INPUT;
		MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
	      }

	    adev->enable_bits = val;
	    if (adev->enable_bits == 0 || adev->go)	/* Device is enabled */
	      {
		adev->d->adrv_trigger (adev->engine_num, adev->enable_bits);
	      }
	  }
	*arg = val;
	return 0;
      }
      break;

    case SNDCTL_DSP_GETTRIGGER:
      if (!adev->d->adrv_trigger)
	{
	  cmn_err (CE_NOTE, "Device %d doesn't have trigger capability\n",
		   adev->engine_num);
	  return OSS_EIO;
	}
      return *arg = (adev->enable_bits & adev->open_mode);
      break;

    case SNDCTL_DSP_SETSYNCRO:
      adev->go = 0;
      break;

    case SNDCTL_DSP_SYNCGROUP:
      return handle_syncgroup (adev, (oss_syncgroup *) arg);
      break;

    case SNDCTL_DSP_SYNCSTART:
      val = *arg;

      MUTEX_ENTER_IRQDISABLE (audio_global_mutex, flags);
      ret = handle_syncstart (adev->engine_num, val);
      MUTEX_EXIT_IRQRESTORE (audio_global_mutex, flags);

      return ret;
      break;

    case SNDCTL_DSP_GETERROR:
      {
	audio_errinfo *info = (audio_errinfo *) arg;

	memset ((void *) info, 0, sizeof (*info));

	//if (audio_engines[dev]->open_mode & OPEN_READ)
	{
	  dmap_t *dmap = audio_engines[dev]->dmap_in;

	  info->rec_overruns = dmap->rec_overruns;
	  dmap->rec_overruns = 0;
	  info->rec_ptradjust = 0;
	  info->rec_errorcount = dmap->num_errors;
	  info->rec_lasterror = dmap->errors[0];
	  info->rec_errorparm = dmap->error_parms[0];
	}

	//if (audio_engines[dev]->open_mode & OPEN_WRITE)
	{
	  dmap_t *dmap = audio_engines[dev]->dmap_out;

	  info->play_underruns = dmap->play_underruns;
	  dmap->play_underruns = 0;
	  info->play_ptradjust = 0;
	  info->play_errorcount = dmap->num_errors;
	  info->play_lasterror = dmap->errors[0];
	  info->play_errorparm = dmap->error_parms[0];
	}

	return 0;
      }
      break;

    case SNDCTL_DSP_GETPLAYVOL:
    case SNDCTL_DSP_SETPLAYVOL:
      {
	int err, mixdev;

	mixdev = adev->mixer_dev;
	if (cmd == SNDCTL_DSP_SETPLAYVOL && mixdev >= 0
	    && mixdev < num_mixers)
	  mixer_devs[mixdev]->modify_counter++;

	if (adev->d->adrv_ioctl == NULL
	    || (err = adev->d->adrv_ioctl (dev, cmd, arg)) == OSS_EINVAL)
	  {
	    /* Emulate these calls using mixer API */

	    if (mixdev < 0 || mixdev >= num_mixers)
	      return OSS_EINVAL;

	    if (cmd == SNDCTL_DSP_GETPLAYVOL)
	      cmd = SOUND_MIXER_READ_PCM;
	    else
	      cmd = SOUND_MIXER_WRITE_PCM;
	    return mixer_devs[mixdev]->d->ioctl (mixdev, dev, cmd, arg);
	  }
	return err;
      }
      break;

    case SNDCTL_DSP_GETRECVOL:
    case SNDCTL_DSP_SETRECVOL:
      {
	int err, mixdev;

	mixdev = adev->mixer_dev;
	if (cmd == SNDCTL_DSP_SETRECVOL && mixdev >= 0 && mixdev < num_mixers)
	  mixer_devs[mixdev]->modify_counter++;

	if (adev->d->adrv_ioctl == NULL
	    || (err = adev->d->adrv_ioctl (dev, cmd, arg)) == OSS_EINVAL)
	  {
	    /* Emulate these calls using mixer API */

	    if (mixdev < 0 || mixdev >= num_mixers)
	      return OSS_EINVAL;

	    /* Try with RECGAIN */
	    if (cmd == SNDCTL_DSP_GETRECVOL)
	      cmd = SOUND_MIXER_READ_RECGAIN;
	    else
	      cmd = SOUND_MIXER_WRITE_RECGAIN;
	    err = mixer_devs[mixdev]->d->ioctl (mixdev, dev, cmd, arg);

	    /* Try with RECLEV */
	    if (cmd == SNDCTL_DSP_GETRECVOL)
	      cmd = SOUND_MIXER_READ_RECLEV;
	    else
	      cmd = SOUND_MIXER_WRITE_RECLEV;
	    err = mixer_devs[mixdev]->d->ioctl (mixdev, dev, cmd, arg);

	    if (err >= 0)	/* Was OK */
	      return err;

	    /* Try with IGAIN */
	    if (cmd == SNDCTL_DSP_GETRECVOL)
	      cmd = SOUND_MIXER_READ_IGAIN;
	    else
	      cmd = SOUND_MIXER_WRITE_IGAIN;
	    return mixer_devs[mixdev]->d->ioctl (mixdev, dev, cmd, arg);
	  }
	return err;
      }
      break;

    case SNDCTL_DSP_GETOPTR:
      if (!(adev->dmask & DMASK_OUT))
	{
	  oss_audio_set_error (adev->engine_num, E_PLAY,
			       OSSERR (1007,
				       "GETOPTR called in read-only mode"),
			       0);
	  return OSS_ENOTSUP;
	}
      return get_optr (adev, dmapout, arg);
      break;

    case SNDCTL_DSP_GETIPTR:
      if (!(adev->dmask & DMASK_IN))
	{
	  oss_audio_set_error (adev->engine_num, E_REC,
			       OSSERR (1008,
				       "GETIPTR called in write-only mode"),
			       0);
	  return OSS_ENOTSUP;
	}
      return get_iptr (adev, dmapin, arg);
      break;

#ifndef OSS_NO_LONG_LONG
    case SNDCTL_DSP_CURRENT_OPTR:
      if (!(adev->dmask & DMASK_OUT))
	return OSS_ENOTSUP;
      return get_long_optr (adev, dmapout, arg);
      break;

    case SNDCTL_DSP_CURRENT_IPTR:
      if (!(adev->dmask & DMASK_IN))
	return OSS_ENOTSUP;
      return get_long_iptr (adev, dmapin, arg);
      break;
#endif

    case SNDCTL_DSP_POLICY:
      val = *arg;
      if (val < 0 || val > 10)
	return OSS_EIO;
      adev->policy = val;
      return 0;
      break;

    case SNDCTL_DSP_SETFRAGMENT:
    case SNDCTL_DSP_SUBDIVIDE:
      if (cmd == SNDCTL_DSP_SUBDIVIDE)
	{
	  oss_audio_set_error (adev->engine_num, E_PLAY,
			       OSSERR (1010,
				       "SNDCTL_DSP_SUBDIVIDE is obsolete"),
			       0);
	  /*
	   * Errordesc:
	   * SNDCTL_DSP_SUBDIVIDE is obsolete. It's still emulated by OSS but
	   * the result is not precise. You need to use SNDCTL_DSP_SETFRAGMENT
	   * instead.
	   */
	  *arg = 0x00040008;	/* Default to 4 fragments of 256 bytes */
	}

      val = *arg;
      if (adev->dmask & DMASK_OUT)
	{
	  if (dmapout->flags & DMAP_FRAGFIXED)
	    setfragment_error (dev);
	  dmapout->fragsize_rq = val;
	}
      if (adev->dmask & DMASK_IN)
	{
	  if (dmapin->flags & DMAP_FRAGFIXED)
	    setfragment_error (dev);
	  dmapin->fragsize_rq = val;
	}
      return 0;

#ifdef __FreeBSD__
    case FREEBSD_GETBLKSIZE:
#endif
    case SNDCTL_DSP_GETBLKSIZE:
      return *arg = getblksize (adev);

    case SNDCTL_DSP_SPEED:
      val = *arg;
      if (val<0)
	 return OSS_EINVAL;
      return *arg = (oss_audio_set_rate (dev, val));

    case SNDCTL_DSP_STEREO:
      {
	int n, v;

	n = *arg;
	if (n > 1)
	  {
	    oss_audio_set_error (adev->engine_num, E_PLAY,
				 OSSERR (1009,
					 "SNDCTL_DSP_STEREO called with bad agrument value"),
				 n);
	    /*
	     * Errordesc: SNDCTL_DSP_STEREO is an obsolete ioctl call that
	     * supports only mono (0) or stereo (1). For larger number of channels
	     * you need to use SNDCTL_DSP_CHANNELS instead.
	     */
	    return OSS_EINVAL;
	  }

	if (n < 0)
	  return OSS_EINVAL;

	v = oss_audio_set_channels (dev, n + 1);
	return *arg = (v - 1);
      }

    case SNDCTL_DSP_CHANNELS:
      {
	int v;
	val = *arg;
#ifdef DO_TIMINGS
	{
	  char tmp[128];

	  sprintf (tmp, "Set channels %d", (int) val);
	  oss_do_timing2 (DFLAG_PROFILE, tmp);
	}
#endif
	if (val<0)
	{
		return OSS_EINVAL;
	}
	v = oss_audio_set_channels (dev, val);
	return *arg = v;
      }

    case SNDCTL_DSP_PROFILE:	/* Obsolete */
      return 0;
      break;

    case SNDCTL_DSP_SILENCE:
      memset (dmapout->dmabuf, dmapout->neutral_byte, dmapout->buffsize);
      return 0;
      break;

    case SNDCTL_DSP_SKIP:
      return 0;
      break;

    case SNDCTL_DSP_GET_RECSRC_NAMES:
      if (adev->d->adrv_ioctl == NULL || (err = adev->d->adrv_ioctl (dev, cmd, arg)) == OSS_EINVAL)	/* Not handled */
	{
	  oss_mixer_enuminfo *ei = (oss_mixer_enuminfo *) arg;

	  memset (ei, 0, sizeof (*ei));	/* Wipe out everything */

	  if (get_legacy_recsrc_names (dev, ei))
	    return 0;
	  ei->nvalues = 1;
	  strcpy (ei->strings, "default");
	  return 0;
	}
      return err;
      break;

    case SNDCTL_DSP_GET_RECSRC:
      if (adev->d->adrv_ioctl == NULL || (err = adev->d->adrv_ioctl (dev, cmd, arg)) == OSS_EINVAL)	/* Not handled */
	{
	  return *arg = (get_legacy_recsrc (dev));
	}
      return err;
      break;

    case SNDCTL_DSP_SET_RECSRC:
      if (adev->d->adrv_ioctl == NULL || (err = adev->d->adrv_ioctl (dev, cmd, arg)) == OSS_EINVAL)	/* Not handled */
	{
	  val = *arg;
	  set_legacy_recsrc (dev, val);
	  return *arg = (get_legacy_recsrc (dev));
	}
      return err;
      break;

    case SNDCTL_DSP_GET_PLAYTGT_NAMES:
      if (adev->d->adrv_ioctl == NULL || (err = adev->d->adrv_ioctl (dev, cmd, arg)) == OSS_EINVAL)	/* Not handled */
	{
	  oss_mixer_enuminfo *ei = (oss_mixer_enuminfo *) arg;
	  memset (ei, 0, sizeof (*ei));
	  ei->nvalues = 1;
	  strcpy (ei->strings, "default");
	  return 0;
	}
      return err;
      break;

    case SNDCTL_DSP_GET_PLAYTGT:
      if (adev->d->adrv_ioctl == NULL || (err = adev->d->adrv_ioctl (dev, cmd, arg)) == OSS_EINVAL)	/* Not handled */
	{
	  return *arg = (0);
	}
      return err;
      break;

    case SNDCTL_DSP_SET_PLAYTGT:
      if (adev->d->adrv_ioctl == NULL || (err = adev->d->adrv_ioctl (dev, cmd, arg)) == OSS_EINVAL)	/* Not handled */
	{
	  return *arg = (0);
	}
      return err;
      break;

    case SNDCTL_SETSONG:
      if (adev->d->adrv_ioctl != NULL
	  && adev->d->adrv_ioctl (dev, cmd, arg) >= 0)
	return 0;
      strncpy (adev->song_name, (char *) arg, sizeof (adev->song_name));
      adev->song_name[sizeof (adev->song_name) - 1] = 0;
      return 0;
      break;

    case SNDCTL_SETLABEL:
      if (adev->d->adrv_ioctl != NULL
	  && adev->d->adrv_ioctl (dev, cmd, arg) >= 0)
	return 0;
      strncpy (adev->label, (char *) arg, sizeof (adev->label));
      adev->label[sizeof (adev->label) - 1] = 0;
      if (*adev->cmd == 0)	/* Command name not known */
	strcpy (adev->cmd, adev->label);
      return 0;
      break;
    default:
      if (adev->d->adrv_ioctl == NULL)
	return OSS_EINVAL;
      return adev->d->adrv_ioctl (dev, cmd, arg);
    }
  return OSS_EINVAL;
}


static int
prepare_output (adev_p adev, dmap_p dmap)
{
  int ret, data_rate = 0;
  audio_format_info_p fmt_info;

  if (dmap->flags & DMAP_PREPARED)
    return 0;

  data_rate = 8;

  dmap->flags &= ~DMAP_COOKED;
  adev->user_parms.convert = 0;
  adev->hw_parms.convert = 0;
  dmap->convert_func = NULL;
  dmap->convert_mode = 0;
  dmap->expand_factor = UNIT_EXPAND;

  if (adev->user_parms.rate != adev->hw_parms.rate)
    dmap->flags |= DMAP_COOKED;
  if (adev->user_parms.fmt != adev->hw_parms.fmt)
    dmap->flags |= DMAP_COOKED;
  if (adev->user_parms.channels != adev->hw_parms.channels)
    dmap->flags |= DMAP_COOKED;
  if ((adev->flags & ADEV_NONINTERLEAVED) && adev->hw_parms.channels > 1)
    dmap->flags |= DMAP_COOKED;

#if 1
  if (always_cooked && !(dmap->mapping_flags & DMA_MAP_MAPPED))
    dmap->flags |= DMAP_COOKED;
#endif

  if ((dmap->mapping_flags & DMA_MAP_MAPPED) && (dmap->flags & DMAP_COOKED))
    {
      cmn_err (CE_WARN, "Internal error in mmap() support\n");
      dmap->flags &= ~DMAP_COOKED;
    }

  if (dmap->flags & DMAP_COOKED)
    {
#ifdef DO_TIMINGS
      oss_do_timing ("Cooked mode - Setting up conversions");
#endif
      if ((ret =
	   setup_format_conversions (adev, dmap, &adev->user_parms,
				     &adev->hw_parms,
				     &adev->user_parms,
				     &adev->hw_parms,
				     adev->oformat_mask)) < 0)
	{
	  return ret;
	}
    }
  else
    {
      DDB (cmn_err (CE_CONT, "No format conversions needed\n"));
#ifdef DO_TIMINGS
      oss_do_timing ("No format conversions needed");
#endif
    }

/*
 * Compute device data rate and frame size
 */
  if ((fmt_info = oss_find_format (adev->hw_parms.fmt)) != NULL)
    data_rate = fmt_info->bits;

  data_rate /= 8;
  if (data_rate < 1)
    data_rate = 1;
  data_rate *= adev->hw_parms.channels;
  dmap->frame_size = data_rate;
  data_rate *= adev->hw_parms.rate;
  if (data_rate < 1)
    data_rate = 8000;
  dmap->data_rate = data_rate;

/*
 * Compute application/user frame_size
 */
  data_rate = 8;
  if ((fmt_info = oss_find_format (adev->user_parms.fmt)) != NULL)
    data_rate = fmt_info->bits;

  data_rate /= 8;
  if (data_rate < 1)
    data_rate = 1;
  data_rate *= adev->user_parms.channels;
  dmap->user_frame_size = data_rate;

  setup_fragments (adev, dmap, OPEN_WRITE);
  if (dmap->expand_factor == 0)
    {
      dmap->expand_factor = UNIT_EXPAND;
      cmn_err (CE_WARN, "Bad expand factor for device %d\n",
	       adev->engine_num);
    }
  if (dmap->low_water == -1)
    {
      dmap->low_water =
	(dmap->low_water_rq * UNIT_EXPAND) / dmap->expand_factor;
    }

#ifdef DO_TIMINGS
     oss_timing_printf ("Prepare output dev=%d, fragsize=%d, nfrags=%d, bytes_in_use=%d/%d",
	     adev->engine_num, dmap->fragment_size, dmap->nfrags,
	     dmap->bytes_in_use, dmap->buffsize);
#endif

  if ((ret =
       adev->d->adrv_prepare_for_output (adev->engine_num,
					 dmap->fragment_size,
					 dmap->nfrags)) < 0)
    {
      return ret;
    }

  dmap->flags |= DMAP_PREPARED;
#ifdef CONFIG_OSSD
  ossd_event (adev->engine_num, OSSD_EV_PREPARE_OUTPUT);
#endif

  return 0;
}

static int
prepare_input (adev_p adev, dmap_p dmap)
{
  int ret, data_rate = 0;
  audio_format_info_p fmt_info;

  if (dmap->flags & DMAP_PREPARED)
    return 0;

  data_rate = 0;

  dmap->flags &= ~DMAP_COOKED;
  adev->user_parms.convert = 0;
  adev->hw_parms.convert = 0;
  dmap->convert_func = NULL;
  dmap->convert_mode = 0;
  dmap->expand_factor = UNIT_EXPAND;

  if (adev->user_parms.rate != adev->hw_parms.rate)
    dmap->flags |= DMAP_COOKED;
  if (adev->user_parms.fmt != adev->hw_parms.fmt)
    dmap->flags |= DMAP_COOKED;
  if (adev->user_parms.channels != adev->hw_parms.channels)
    dmap->flags |= DMAP_COOKED;
  if ((adev->flags & ADEV_NONINTERLEAVED) && adev->hw_parms.channels > 1)
    dmap->flags |= DMAP_COOKED;

#if 1
  if (always_cooked && !(dmap->mapping_flags & DMA_MAP_MAPPED))
    dmap->flags |= DMAP_COOKED;
#endif

  if ((dmap->mapping_flags & DMA_MAP_MAPPED) && (dmap->flags & DMAP_COOKED))
    {
      cmn_err (CE_WARN, "Internal error in mmap() support\n");
      dmap->flags &= ~DMAP_COOKED;
    }

  if (dmap->flags & DMAP_COOKED)
    {
      if ((ret =
	   setup_format_conversions (adev, dmap, &adev->hw_parms,
				     &adev->user_parms,
				     &adev->user_parms,
				     &adev->hw_parms,
				     adev->iformat_mask)) < 0)
	{
	  DDB (cmn_err
	       (CE_CONT, "setup_format_conversions failed, err=%d\n", ret));
	  return ret;
	}
    }
  else
    dmap->expand_factor = UNIT_EXPAND;

/*
 * Compute device data rate and frame size
 */
  if ((fmt_info = oss_find_format (adev->hw_parms.fmt)) != NULL)
    data_rate = fmt_info->bits;

  data_rate /= 8;
  if (data_rate < 1)
    data_rate = 1;
  data_rate *= adev->hw_parms.channels;
  dmap->frame_size = data_rate;
  data_rate *= adev->hw_parms.rate;
  if (data_rate < 1)
    data_rate = 8000;
  dmap->data_rate = data_rate;

/*
 * Compute user/application frame size
 */
  data_rate = 8;
  if ((fmt_info = oss_find_format (adev->user_parms.fmt)) != NULL)
    data_rate = fmt_info->bits;

  data_rate /= 8;
  if (data_rate < 1)
    data_rate = 1;
  data_rate *= adev->user_parms.channels;
  dmap->user_frame_size = data_rate;

  setup_fragments (adev, dmap, OPEN_READ);

#if 1
  /* Compute 1/expand_factor */

  if (dmap->expand_factor == 0)
    {
      cmn_err (CE_NOTE, "Internal error (expand_factor==0)\n");
      dmap->expand_factor = UNIT_EXPAND;
    }

  {
    int expand = dmap->expand_factor;
    int expand2 = expand * 100 / UNIT_EXPAND;
    DDB (cmn_err (CE_CONT, "Expand factor was = %d (%d.%02d)\n", expand,
		  expand2 / 100, expand2 % 100));
  }

  dmap->expand_factor = (UNIT_EXPAND * UNIT_EXPAND) / dmap->expand_factor;

  {
    int expand = dmap->expand_factor;
    int expand2 = expand * 100 / UNIT_EXPAND;
    DDB (cmn_err
	 (CE_CONT, "Expand factor inverted to = %d (%d.%02d)\n", expand,
	  expand2 / 100, expand2 % 100));
  }
#endif

  if (dmap->low_water == -1)
    {
      dmap->low_water =
	(dmap->low_water_rq * UNIT_EXPAND) / dmap->expand_factor;
    }

#ifdef DO_TIMINGS
     oss_timing_printf ("Prepare input dev=%d, fragsize=%d, nfrags=%d, bytes_in_use=%d",
	     adev->engine_num, dmap->fragment_size, dmap->nfrags,
	     dmap->bytes_in_use);
#endif

  if ((ret =
       adev->d->adrv_prepare_for_input (adev->engine_num, dmap->fragment_size,
					dmap->nfrags)) < 0)
    {
      DDB (cmn_err
	   (CE_CONT, "/dev/dsp%d: prepare_for_input failed, err=%d\n",
	    adev->engine_num, ret));
      return ret;
    }

  dmap->flags |= DMAP_PREPARED;
#ifdef CONFIG_OSSD
  ossd_event (adev->engine_num, OSSD_EV_PREPARE_INPUT);
#endif

  return 0;
}

static int
launch_input (adev_p adev, dmap_p dmap)
{
#ifdef DO_TIMINGS
  oss_do_timing ("Launch input called");
#endif
  if (dmap->flags & DMAP_STARTED)
    return 0;

  if (!(dmap->flags & DMAP_PREPARED))
    {
      cmn_err (CE_WARN, "launch_input while not prepared.\n");
      return OSS_EIO;
    }

#ifdef DO_TIMINGS
  oss_do_timing ("Launch_input calling d->start_input");
#endif

  if (adev->d->adrv_start_input != NULL)
    {
      if (adev->flags & ADEV_AUTOMODE)
	adev->d->adrv_start_input (adev->engine_num, dmap->dmabuf_phys,
				   dmap->bytes_in_use, dmap->fragment_size,
				   0);
      else
	adev->d->adrv_start_input (adev->engine_num, dmap->dmabuf_phys,
				   dmap->fragment_size, dmap->fragment_size,
				   0);
    }

#ifdef DO_TIMINGS
  oss_do_timing ("Launch_input calling trigger");
#endif
  dmap->flags |= DMAP_STARTED;
  if (adev->d->adrv_trigger
      && ((adev->enable_bits * adev->go) & PCM_ENABLE_INPUT))
    {
      adev->d->adrv_trigger (adev->engine_num, adev->enable_bits * adev->go);
    }

  return 0;
}

static int
find_raw_input_space (adev_p adev, dmap_p dmap, int *dmapos)
{
  int count;
  int tmout, n = 0;
  oss_uint64_t offs, doffs;
  int lim = 1;
  unsigned int status;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
  if (adev->nonblock)
    get_input_pointer (adev, dmap, 1);
  count = (int) (dmap->byte_counter - dmap->user_counter);
  *dmapos=0;

  if (dmap->flags & DMAP_COOKED)
    {
      lim = dmap->fragment_size;
    }

  while (count < lim)
    {
      if (adev->nonblock)
	{
	  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
#ifdef DO_TIMINGS
	  oss_do_timing ("*** EAGAIN ***");
#endif
	  return OSS_EAGAIN;
	}

      if (n++ > 100)
	{
	  cmn_err (CE_WARN, "Audio input %d doesn't get filled.\n",
		   adev->engine_num);
	  cmn_err (CE_CONT, "Counters %d / %d\n", count, lim);
	  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
	  return OSS_EIO;
	}

      tmout = (dmap->fragment_size * OSS_HZ) / dmap->data_rate;
      tmout += OSS_HZ / 2;

      if (adev->go == 0)
	tmout = 0;

#ifdef DO_TIMINGS
      oss_do_timing ("Sleep (in)");
      oss_timing_enter (DF_SLEEPREAD);
#endif
      audio_engines[adev->engine_num]->dmap_in->error = 0;
      if (!oss_sleep (adev->in_wq, &dmap->mutex, tmout, &flags, &status))
	{
#ifdef DO_TIMINGS
	  oss_do_timing ("Sleep (in) timed out");
#endif
	  adev->timeout_count++;
	  if (adev->d->adrv_check_input)
	    {
	      int err = adev->d->adrv_check_input (adev->engine_num);
	      if (err == 0)
		continue;	/* Retry */
	      MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
	      return err;
	    }
	  cmn_err (CE_NOTE,
		   "Input timed out on audio engine %d (count=%lld)\n",
		   adev->engine_num, dmap->byte_counter);
	  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
	  FMA_EREPORT(adev->osdev, DDI_FM_DEVICE_STALL, NULL, NULL, NULL);
	  FMA_IMPACT(adev->osdev, DDI_SERVICE_LOST);
	  return OSS_EIO;
	}			/* Timed out */

#ifdef DO_TIMINGS
      oss_timing_leave (DF_SLEEPREAD);
      oss_do_timing ("Sleep (in) done");
#endif

      if (status & WK_SIGNAL)
	{
	  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
	  return OSS_EINTR;
	}
      count = (int) (dmap->byte_counter - dmap->user_counter);
#ifdef DO_TIMINGS
	oss_timing_printf ("User counter %d, byte_counter %d", dmap->user_counter,
		 dmap->byte_counter);
#endif
    }

/* Now we should have some data */

  if (adev->nonblock)
    get_input_pointer (adev, dmap, 1);

  offs = dmap->user_counter % dmap->bytes_in_use;
  doffs = dmap->byte_counter % dmap->bytes_in_use;

  count = (int) (dmap->bytes_in_use - offs);
  if (offs <= doffs)
    count = (int) (doffs - offs);
  if (count == 0)
    count = dmap->bytes_in_use;

  *dmapos = offs;
  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);

  return count;
}

/*ARGSUSED*/
static int
move_raw_rdpointer (adev_p adev, dmap_p dmap, int len)
{
  int ret = 0;
  oss_native_word flags;
#ifdef DO_TIMINGS
    oss_timing_printf ("Move rdpointer, offs=%d, incr %d", dmap->user_counter,
	     len);
#endif
  MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
  dmap->user_counter += len;
  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);

  return ret;
}

static void
copy_read_noninterleaved(adev_t *adev, dmap_t *dmap, int dma_offs, unsigned char *localbuf, int local_offs, int l)
{
/*
 * Copy audio data from non-interleaved device buffer to interleaved
 * local buffer.
 */
// TODO: This function assumes 32 bit audio DATA
	
  int ch, i, nc = adev->hw_parms.channels;
  int *outbuf, *inbuf;

  l		/= sizeof(*outbuf)*nc;
  dma_offs	/= sizeof(*outbuf);
  local_offs	/= sizeof(*outbuf);

  for (ch=0;ch<nc;ch++)
  {
  	outbuf = (int*)(localbuf+local_offs);
	outbuf += ch;

	inbuf = (int *)(dmap->dmabuf + dmap->buffsize*ch / nc);
	inbuf += dma_offs / nc;

	for (i=0;i<l;i++)
	{
		*outbuf = *inbuf++;
		outbuf += nc;
	}
  }

}

static int
find_input_space (adev_p adev, dmap_p dmap, unsigned char **dbuf)
{
  unsigned char *p, *p1 = dmap->tmpbuf1, *p2 = dmap->tmpbuf2;
  int err, l, l2, max, dmapos;

  if (!(dmap->flags & DMAP_COOKED))
    {
      err=find_raw_input_space (adev, dmap, &dmapos);
      *dbuf=dmap->dmabuf+dmapos;
      return err;
    }

  if (dmap->tmpbuf_len > dmap->tmpbuf_ptr)
    {
      *dbuf = dmap->tmpbuf1 + dmap->tmpbuf_ptr;
      return dmap->tmpbuf_len - dmap->tmpbuf_ptr;
    }

  dmap->tmpbuf_len = dmap->tmpbuf_ptr = 0;

  if ((l = find_raw_input_space (adev, dmap, &dmapos)) < 0)
    {
      return l;
    }
  p=dmap->dmabuf;

  if (dmap->expand_factor > UNIT_EXPAND)
    max = (TMP_CONVERT_MAX * UNIT_EXPAND) / dmap->expand_factor;
  else
    max = TMP_CONVERT_MAX;

  if (max > TMP_CONVERT_MAX)
    max = TMP_CONVERT_MAX;

  if (l > max)
    l = max;
  l = (l / dmap->frame_size) * dmap->frame_size;	/* Truncate to nearest frame size */
  l2 = l;
  VMEM_CHECK (p1, l);
  VMEM_CHECK (p+dmapos, l);

  if ((adev->flags & ADEV_NONINTERLEAVED) && adev->hw_parms.channels > 1)
     copy_read_noninterleaved(adev, dmap, dmapos, p1, 0, l);
  else
     memcpy (p1, p+dmapos, l);

  move_raw_rdpointer (adev, dmap, l);

  if ((err =
       dmap->convert_func (adev, dmap, &p1, &l2,
			   &p2, &adev->hw_parms,
			   &adev->user_parms)) < 0)
    return err;

  dmap->tmpbuf1 = p1;
  dmap->tmpbuf2 = p2;
  dmap->tmpbuf_len = l2;

  *dbuf = dmap->tmpbuf1;
  return dmap->tmpbuf_len;
}

static int
move_rdpointer (adev_p adev, dmap_p dmap, int len)
{
  if (!(dmap->flags & DMAP_COOKED) || dmap->tmpbuf_ptr >= dmap->tmpbuf_len)
    {
      dmap->tmpbuf_ptr = 0;
      dmap->tmpbuf_len = 0;
      return move_raw_rdpointer (adev, dmap, len);
    }

  if (dmap->tmpbuf_ptr < dmap->tmpbuf_len)
    {
      dmap->tmpbuf_ptr += len;

      if (dmap->tmpbuf_ptr >= dmap->tmpbuf_len)
	dmap->tmpbuf_len = dmap->tmpbuf_ptr = 0;

      return 0;
    }

  cmn_err (CE_NOTE, "Why here?\n");
  return OSS_EIO;
}

int
oss_audio_read (int dev, struct fileinfo *file, uio_t * buf, int count)
{
  adev_p adev;
  dmap_p dmap;
  int c, l, p, ret, n;
  unsigned char *dmabuf;
#ifdef DO_TIMINGS
    oss_timing_printf ("--- audio_read(%d, %d) ---", dev, count);
#endif

  sync_seed++;

  if (dev < 0 || dev >= num_audio_engines)
    return OSS_ENXIO;

  adev = audio_engines[dev];
  if (!adev->enabled)
    return OSS_ENXIO;
  if (adev->flags & ADEV_NOINPUT)
    return OSS_EACCES;
  dmap = adev->dmap_in;

  if (!(adev->open_mode & OPEN_READ))
    return OSS_ENOTSUP;
  if (dmap->dma_mode == PCM_ENABLE_OUTPUT)
    {
      audio_reset_output (adev);
      reset_dmap (dmap);
    }

/*
 * Initial setup 
 */
  oss_reset_wait_queue (adev->in_wq);

  if (file != NULL)
    {
      if ((ISSET_FILE_FLAG (file, O_NONBLOCK)
	   && !(adev->open_flags & OF_BLOCK)) || adev->forced_nonblock)
	adev->nonblock = 1;
      else
	adev->nonblock = 0;
#ifdef DO_TIMINGS
      if (adev->nonblock)
	oss_do_timing ("*** NON BLOCKING READ ***");
#endif
    }

  if (dmap->dma_mode != PCM_ENABLE_INPUT)
    {
      if ((ret = prepare_input (adev, dmap)) < 0)
	{
	  DDB (cmn_err (CE_CONT, "Prepare input failed, err=%d\n", ret));
	  return ret;
	}
      dmap->dma_mode = PCM_ENABLE_INPUT;
      launch_input (adev, dmap);
    }

  if (!(dmap->flags & DMAP_PREPARED))
    {				/* Not prepared. Why??? */
      cmn_err (CE_WARN, "Intenal error (not prepared)\n");
      return OSS_EIO;
    }

  c = count;
  p = 0;
  n = 0;

  while (c > 0 && n++ < 1000)
    {
      if ((l = find_input_space (adev, dmap, &dmabuf)) < 0)
	{
	  if (l == OSS_EINTR)
	    {
	      if (c == count)	/* Nothing read yet */
		return OSS_EINTR;
	      return count - c;
	    }
	  if (l == OSS_EAGAIN)
	    {
	      if (c == count)	/* Nothing read yet */
		return OSS_EAGAIN;
	      return count - c;
	    }
	  return l;
	}

      if (l > c)
	l = c;

      if (uiomove (dmabuf, l, UIO_READ, buf) != 0)
	{
	  cmn_err (CE_WARN, "audio: uiomove(UIO_READ) failed\n");
	  return OSS_EFAULT;
	}
      if ((ret = move_rdpointer (adev, dmap, l)) < 0)
	{
	  return ret;
	}

      c -= l;
      p += l;
    }

#ifdef DO_TIMINGS
    oss_timing_printf ("-------- Audio read done (%d)", count - c);
#endif

  return count - c;
}

/*ARGSUSED*/
static int
audio_space_in_queue (adev_p adev, dmap_p dmap, int count)
{
  int cnt;

  cnt = (int) (dmap->byte_counter + dmap->bytes_in_use - dmap->user_counter);

  if (cnt < 0)
    {
      cmn_err (CE_CONT, "Buffer %d overfilled (%d)\n", adev->engine_num, cnt);
    }

  if (!(dmap->mapping_flags & DMA_MAP_MAPPED))
    if (cnt > dmap->bytes_in_use)	/* Output underrun */
      {
#ifdef DO_TIMINGS
	oss_timing_printf ("adev %d: Play underrun B", adev->engine_num);
	oss_timing_printf ("  User=%lld", dmap->user_counter);
	oss_timing_printf ("  Dev=%lld", dmap->byte_counter);
	oss_timing_printf ("  Tmp=%d", dmap->tmpbuf_ptr);
#endif
	cnt = dmap->bytes_in_use;
	dmap->user_counter = dmap->byte_counter;
	dmap->play_underruns++;
	if (!dmap->underrun_flag)
	  {
#ifdef DO_TIMINGS
	    oss_do_timing ("Clearing the buffer");
#endif
	    memset (dmap->dmabuf, dmap->neutral_byte, dmap->bytes_in_use);
	  }
	dmap->underrun_flag = 1;
      }

  if ((dmap->user_counter + cnt) > (dmap->byte_counter + dmap->bytes_in_use))
    cmn_err (CE_CONT, "Overflow %lld+%d, %lld\n", dmap->user_counter, cnt,
	     dmap->byte_counter);

  return cnt;
}

static int
find_output_space (adev_p adev, dmap_p dmap, int *size, int count)
{
  int offs;
  int len, l2, n = 0, tmout;
  oss_native_word flags;
  int tout;
  unsigned int status;

  if (dmap == NULL)
    {
      cmn_err (CE_WARN, "Internal error - dmap==NULL\n");
      return OSS_EIO;
    }

  MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
  len = audio_space_in_queue (adev, dmap, count);

  tout = 0;
  while (len < dmap->user_frame_size)
    {				/* Wait for some space */
      if (tout++ > 10000)
	{
	  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
	  cmn_err (CE_WARN, "Internal timeout error B\n");
	  return OSS_EIO;
	}

      if (adev->nonblock || !(adev->enable_bits & PCM_ENABLE_OUTPUT))
	{
	  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
#ifdef DO_TIMINGS
	    oss_timing_printf ("Space=%d\n", len);
	    oss_do_timing ("*** EAGAIN ***");
#endif
	  if (!(adev->enable_bits & PCM_ENABLE_OUTPUT))
	    {
	      launch_output (adev, dmap);
	      oss_audio_set_error (adev->engine_num, E_PLAY,
				   OSSERR (1021,
					   "Play buffer full when playback is triggered off."),
				   0);
	      /*
	       * Errordesc:
	       * An application had written too many samples of data between
	       * turning off the PCM_ENABLE_OUTPUT trigger bit and turning it back
	       * again to trigger playback.
	       *
	       * Applications using SNDCTL_DSP_SETTRIGGER should avoid filling the 
	       * available playback buffer before triggering output.
	       *
	       * One possible error causing this is that the application has
	       * triggered only recording on a duplex device.
	       */
	    }
	  return OSS_EAGAIN;
	}

      if (n++ > dmap->nfrags * 2)
	{
	  cmn_err (CE_WARN, "Audio output %d doesn't drain (%lld/%lld %d).\n",
		   adev->engine_num, dmap->user_counter, dmap->byte_counter,
		   len);
	  cmn_err (CE_CONT, "len=%d/%d, total=%d\n", len, dmap->fragment_size,
		   dmap->bytes_in_use);
#ifdef DO_TIMINGS
	  oss_do_timing ("Audio output doesn't drain");
#endif
	  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
	  audio_reset_output (adev);
	  return OSS_EIO;
	}

      tmout = (dmap->fragment_size * OSS_HZ) / dmap->data_rate;
      tmout += OSS_HZ;

      if (adev->go == 0)
	tmout = 0;

      audio_engines[adev->engine_num]->dmap_out->error = 0;
      if (adev->go == 0)
	{
	  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
	  return OSS_EAGAIN;
	}
#ifdef DO_TIMINGS
      oss_timing_printf ("Sleep(%d)", adev->engine_num);
      oss_timing_enter (DF_SLEEPWRITE);
#endif
      UP_STATUS (STS_SLEEP);
      if (!oss_sleep (adev->out_wq, &dmap->mutex, tmout, &flags, &status))
	{
#ifdef DO_TIMINGS
	  oss_timing_printf ("Sleep(%d) (out) timed out", adev->engine_num);
#endif
	  adev->timeout_count++;

	  if (adev->d->adrv_check_output)
	    {
	      int err = adev->d->adrv_check_output (adev->engine_num);
	      if (err == 0)
		continue;	/* Retry */
	      MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
	      return err;
	    }
	  else
	    {
	      cmn_err (CE_NOTE,
		       "Output timed out on audio engine %d/'%s' (count=%lld)\n",
		       adev->engine_num, adev->name, dmap->byte_counter);
	    }
	  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
	  FMA_EREPORT(adev->osdev, DDI_FM_DEVICE_STALL, NULL, NULL, NULL);
	  FMA_IMPACT(adev->osdev, DDI_SERVICE_LOST);
	  return OSS_EIO;
	}			/* Timed out */
      DOWN_STATUS (STS_SLEEP);

#ifdef DO_TIMINGS
      oss_timing_leave (DF_SLEEPWRITE);
      oss_timing_printf ("Sleep(%d) (out) done", adev->engine_num);
#endif

      if (status & WK_SIGNAL)
	{
#ifdef DO_TIMINGS
	  oss_do_timing ("Signal caught");
#endif
	  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
	  return OSS_EINTR;
	}

      len = audio_space_in_queue (adev, dmap, count);
#ifdef DO_TIMINGS
      oss_timing_printf ("Free output space now %d bytes", len);
#endif
    }				/* Wait for space */

/*
 * Now we hopefully have some free space in the buffer.
 */

  offs = (int) (dmap->user_counter % dmap->bytes_in_use);

  l2 = dmap->bytes_in_use - offs;
  if (len > l2)
    len = l2;
  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
  if (len < 0)
    len = 0;

  *size = len;
#ifdef DO_TIMINGS
  oss_timing_printf ("Got output buffer, offs %d/%d, len %d", offs,
	     dmap->bytes_in_use, len);
#endif
  if (offs < 0 || (offs + len) > dmap->bytes_in_use)
    {
      cmn_err (CE_WARN, "Bad audio output buffer %d/%d\n", offs, len);
      return OSS_EIO;
    }

  return offs;
}

static int
launch_output (adev_p adev, dmap_p dmap)
{
  oss_native_word flags;

#ifdef DO_TIMINGS
  oss_do_timing ("Launch output called");
#endif
  if (dmap->flags & DMAP_STARTED)
    {
      return 0;
    }

  if (dmap->user_counter == 0 && dmap->audio_callback == NULL
      && dmap->mapping_flags == 0)
    {
      return 0;
    }

  if (!(dmap->flags & DMAP_PREPARED))
    {
      cmn_err (CE_WARN, "launch_output while not prepared. Engine=%d\n", adev->engine_num);
      return OSS_EIO;
    }

#ifdef DO_TIMINGS
  oss_do_timing ("Launch_output calling output_block");
#endif

  if (adev->d->adrv_output_block != NULL)
    {
      if (adev->flags & ADEV_AUTOMODE)
	adev->d->adrv_output_block (adev->engine_num, dmap->dmabuf_phys,
				    dmap->bytes_in_use, dmap->fragment_size,
				    0);
      else
	adev->d->adrv_output_block (adev->engine_num, dmap->dmabuf_phys,
				    dmap->fragment_size, dmap->fragment_size,
				    0);
    }

#ifdef DO_TIMINGS
  oss_do_timing ("Launch_output calling trigger");
#endif
  MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
  dmap->flags |= DMAP_STARTED;
  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
  if (adev->d->adrv_trigger
      && ((adev->enable_bits * adev->go) & PCM_ENABLE_OUTPUT))
    {
      adev->d->adrv_trigger (adev->engine_num, adev->enable_bits * adev->go);
    }

  return 0;
}

static int
move_wrpointer (adev_p adev, dmap_p dmap, int len)
{
  int ret = 0;
  oss_native_word flags;
#ifdef DO_TIMINGS
  oss_timing_printf ("Move wrpointer, len %d", len);
#endif
  MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
  dmap->underrun_flag = 0;

  dmap->user_counter += len;
#ifdef DO_TIMINGS
  oss_timing_printf ("  User=%lld", dmap->user_counter);
  oss_timing_printf ("  Byte=%lld", dmap->byte_counter);
  oss_timing_printf ("  Fill=%lld", dmap->user_counter - dmap->byte_counter);
#endif

#ifdef CONFIG_OSSD
  ossd_event (adev->engine_num, OSSD_EV_UPDATE_OUTPUT);
#endif

  if (ret < 0 || dmap->flags & DMAP_STARTED)
    {
      MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
      return ret;
    }

  if (!(adev->enable_bits & PCM_ENABLE_OUTPUT))
    {
#ifdef DO_TIMINGS
      oss_do_timing ("Output not triggered - skipping launch_output");
#endif
      MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
      return ret;
    }

  if ((dmap->user_counter < dmap->fragment_size * 2)
      && (dmap->user_counter < dmap->bytes_in_use / 2))
    {
#ifdef DO_TIMINGS
      oss_timing_printf ("dmap->user_counter=%lld, dmap->fragment_size*2=%ld",
	       dmap->user_counter, dmap->fragment_size * 2);
      oss_do_timing
	("Not enough data in the buffer yet - skipping launch_output");
#endif
      MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
      return ret;
    }
  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
  ret = launch_output (adev, dmap);
  return ret;
}

/*ARGSUSED*/
static void
store_tmp_data (adev_p adev, dmap_p dmap, unsigned char *buf, int count)
{
  dmap->leftover_buf = buf;
  dmap->leftover_bytes = count;
}

static void
copy_write_noninterleaved(adev_t *adev, dmap_t *dmap, int dma_offs, unsigned char *localbuf, int local_offs, int l)
{
/*
 * Copy interleaved N channel data to non-interleaved device buffer.
 */
// TODO: This function assumes 32 bit audio DATA
	
  int ch, i, nc = adev->hw_parms.channels;
  int *inbuf, *outbuf;

  l		/= sizeof(*inbuf)*nc;
  dma_offs	/= sizeof(*inbuf);
  local_offs	/= sizeof(*inbuf);

  for (ch=0;ch<nc;ch++)
  {
  	inbuf = (int*)(localbuf+local_offs);
	inbuf += ch;

	outbuf = (int *)(dmap->dmabuf + dmap->buffsize*ch / nc);
	outbuf += dma_offs / nc;

	for (i=0;i<l;i++)
	{
		*outbuf++ = *inbuf;
		inbuf += nc;
	}
  }

}

static int
write_copy (adev_p adev, dmap_p dmap, unsigned char *buf, int count)
{
  int err, offs, spc, l, p = 0;

  while (count)
    {
      l = count;

      if ((offs = find_output_space (adev, dmap, &spc, l)) < 0)
	{
	  if (offs == OSS_EAGAIN)
	    {
	      store_tmp_data (adev, dmap, buf + p, count);
	      launch_output (adev, dmap);
	      return OSS_EAGAIN;
	    }
	  return offs;
	}

      if (l > spc)
	l = spc;

      VMEM_CHECK (&dmap->dmabuf[offs], l);
      VMEM_CHECK (buf + p, l);

      if ((adev->flags & ADEV_NONINTERLEAVED) && adev->hw_parms.channels > 1)
	 {
	    copy_write_noninterleaved(adev, dmap, offs, buf + p, 0, l);
	 }
      else
         memcpy (&dmap->dmabuf[offs], buf + p, l);

      if ((err = move_wrpointer (adev, dmap, l)) < 0)
	return err;

      count -= l;
      p += l;
    }
  return 0;
}

int
oss_audio_write (int dev, struct fileinfo *file, uio_t * buf, int count)
{
  adev_p adev;
  oss_native_word flags;
  dmap_p dmap;
  int ret;
  int l, c, spc, offs, p, err;
  int tmout;

#ifdef DO_TIMINGS
  oss_timing_printf ("--- audio_write(%d, %d) ---", dev, count);
#endif

  sync_seed++;

  if (dev < 0 || dev >= num_audio_engines)
    return OSS_ENXIO;

  adev = audio_engines[dev];
  if (!adev->enabled)
    return OSS_ENXIO;
  if (adev->flags & ADEV_NOOUTPUT)
    return OSS_EACCES;
  dmap = adev->dmap_out;

  if (!(adev->open_mode & OPEN_WRITE))
    return OSS_ENOTSUP;

  UP_STATUS (STS_WRITE);

  if (dmap->dma_mode == PCM_ENABLE_INPUT)
    {
      audio_reset_input (adev);
      reset_dmap (dmap);
    }

/*
 * Initial setup 
 */
  oss_reset_wait_queue (adev->out_wq);

  if (file != NULL)
    {
      if ((ISSET_FILE_FLAG (file, O_NONBLOCK)
	   && !(adev->open_flags & OF_BLOCK)) || adev->forced_nonblock)
	adev->nonblock = 1;
      else
	adev->nonblock = 0;
#ifdef DO_TIMINGS
      if (adev->nonblock)
	oss_do_timing ("*** NON BLOCKING WRITE ***");
#endif
    }

  MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);

  if (dmap->dma_mode != PCM_ENABLE_OUTPUT)
    {
      MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
      if ((ret = prepare_output (adev, dmap)) < 0)
	{
	  DOWN_STATUS (STS_WRITE);
	  return ret;
	}
      MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
      dmap->dma_mode = PCM_ENABLE_OUTPUT;
    }

  if (!(dmap->flags & DMAP_PREPARED))
    {				/* Not prepared. Why??? */
      MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
      DOWN_STATUS (STS_WRITE);
      cmn_err (CE_WARN, "Internal error (not prepared)\n");
      return OSS_EIO;
    }
#if 1
  if (dmap->leftover_bytes > 0)
    {
      unsigned char *b;
      int l;

      b = dmap->leftover_buf;
      l = dmap->leftover_bytes;
      dmap->leftover_bytes = 0;
      dmap->leftover_buf = NULL;

      MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
      if ((err = write_copy (adev, dmap, b, l)) < 0)
	{
	  DOWN_STATUS (STS_WRITE);
	  return err;
	}

      dmap->leftover_bytes = 0;
      dmap->leftover_buf = NULL;
      MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
    }
#endif
  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);

  if (count <= 0)
    {
      DOWN_STATUS (STS_WRITE);
      return 0;
    }

  c = count;
  p = 0;

  tmout = 0;

  while (c > 0)
    {
#ifdef DO_TIMINGS
	oss_timing_printf ("%d/%d bytes to go", c, count);
#endif

      if (tmout++ > 1000)
	{
	  cmn_err (CE_WARN, "Internal timeout error A (%d/%d)\n", c, count);
	  return OSS_EIO;
	}

      l = c;
      if (l > 8)
	l &= ~7;		/* Align it */

      if ((offs = find_output_space (adev, dmap, &spc, l)) < 0)
	{
	  if (offs == OSS_EINTR)
	    {
	      DOWN_STATUS (STS_WRITE);
	      if (c == count)	/* Nothing written yet */
		return OSS_EINTR;
	      return count - c;
	    }
	  if (offs == OSS_EAGAIN)
	    {
	      DOWN_STATUS (STS_WRITE);
	      if (c == count)	/* Nothing written yet */
		{
		  return OSS_EAGAIN;
		}
	      return count - c;
	    }
	  DOWN_STATUS (STS_WRITE);
	  return offs;
	}

      if (dmap->convert_func == NULL)
	{
	  if (dmap->device_write != NULL)
	    {
	      unsigned char *tmpbuf;
	      int l2;

	      if (dmap->tmpbuf1 == NULL)
		{
		  dmap->tmpbuf1 = AUDIO_MALLOC (dmap->osdev, TMP_CONVERT_BUF_SIZE+512);
		}

/*
 * Leave some room for data expansion so use just half of the available
 * space.
 */
	      tmpbuf = dmap->tmpbuf1;
	      if (l > spc / 2)
		l = spc / 2;
	      if (l > TMP_CONVERT_BUF_SIZE / 2)
		l = TMP_CONVERT_BUF_SIZE / 2;

	      l2 = l;
	      if (uiomove (tmpbuf, l, UIO_WRITE, buf) != 0)
		{
		  cmn_err (CE_WARN,
			   "audio: uiomove(UIO_WRITE) failed (noconv)\n");
		  DOWN_STATUS (STS_WRITE);
		  return OSS_EFAULT;
		}
	      if ((err =
		   dmap->device_write (adev, dmap, tmpbuf,
				       &dmap->dmabuf[offs], spc, &l,
				       &l2)) < 0)
		{
		  DOWN_STATUS (STS_WRITE);
		  return err;
		}
	      if ((err = move_wrpointer (adev, dmap, l2)) < 0)
		{
		  DOWN_STATUS (STS_WRITE);
		  return err;
		}
	    }
	  else
	    {
	      if (l > spc)
		l = spc;
	      if (uiomove (&dmap->dmabuf[offs], l, UIO_WRITE, buf) != 0)
		{
		  cmn_err (CE_WARN,
			   "audio: uiomove(UIO_WRITE) (noconv2) failed\n");
		  return OSS_EFAULT;
		}
	      if ((err = move_wrpointer (adev, dmap, l)) < 0)
		{
		  DOWN_STATUS (STS_WRITE);
		  return err;
		}
	    }
	}
      else
	{
	  /*
	   * Perform format conversions.
	   */
	  unsigned char *p1 = dmap->tmpbuf1, *p2 = dmap->tmpbuf2;
	  int l2, max, out_max;

	  if (spc > TMP_CONVERT_MAX)
	    spc = TMP_CONVERT_MAX / 2;

	  if (dmap->expand_factor > UNIT_EXPAND)
	     {
		max = spc;

	    	out_max = (spc * dmap->expand_factor) / UNIT_EXPAND; /* Output size */
		if (out_max > TMP_CONVERT_BUF_SIZE) /* Potential overflow */
		   {
			   max = (TMP_CONVERT_BUF_SIZE * UNIT_EXPAND) / dmap->expand_factor;
		   }
	     }
	  else
	    max = spc;
	  if (max < dmap->frame_size)
	    max = dmap->frame_size;

	  if (max > TMP_CONVERT_MAX)
	    max = TMP_CONVERT_MAX / 2;

	  if (l > max)
	    l = max;
	  /* Avoid leaving too short "tails" */
	  if (c - l < 64)
	    l = c;

	  /* Round to integer number of samples */
	  l =
	    (((l + dmap->frame_size -
	       1) / dmap->frame_size)) * dmap->frame_size;
	  if (l > c)
	    l = c;

	  l2 = l;

	  VMEM_CHECK (p1, l);
	  if (uiomove (p1, l, UIO_WRITE, buf) != 0)
	    cmn_err (CE_WARN, "audio: uiomove(UIO_WRITE) (conv) failed\n");
	  UP_STATUS (STS_CONVERT);
	  if ((err =
	       dmap->convert_func (adev, dmap, &p1, &l2,
				   &p2, &adev->user_parms,
				   &adev->hw_parms)) < 0)
	    {
	      cmn_err (CE_WARN, "Format conversion failed (%d)\n", err);
	      DOWN_STATUS (STS_WRITE | STS_CONVERT);
	      return err;
	    }
	  DOWN_STATUS (STS_CONVERT);

	  if ((err = write_copy (adev, dmap, p1, l2)) < 0)
	    {
	      if (err != OSS_EAGAIN)
		{
		  DOWN_STATUS (STS_WRITE);
		  return err;
		}

	      /* Handle non blocking I/O */
	      if (c == count)	/* Nothing written yet */
		{
		  DOWN_STATUS (STS_WRITE);
		  return OSS_EAGAIN;
		}

	      DOWN_STATUS (STS_WRITE);
	      return count - c;

	    }
	}

      c -= l;
      p += l;

      if (l > 0)
	tmout = 0;
    }

#ifdef DO_TIMINGS
  oss_do_timing ("--- Audio write done");
#endif
  DOWN_STATUS (STS_WRITE);
  return count - c;
}

#ifdef MANAGE_DEV_DSP
#ifdef VDEV_SUPPORT
void
oss_combine_write_lists (void)
{
  int i;

  for (i = 0; i < dspoutlist2.ndevs; i++)
    dspoutlist.devices[dspoutlist.ndevs++] = dspoutlist2.devices[i];

  dspoutlist2.ndevs = 0;
}

/*ARGSUSED*/
int
oss_open_vdsp (int dev, int dev_type, struct fileinfo *file, int recursive,
	       int open_flags, int *newdev)
{
  int ret, i, d;
  int mode = file->mode & O_ACCMODE;

  DDB (cmn_err (CE_CONT, "oss_open_vdsp(%d, mode=%d)\n", dev, mode));

  switch (dev)
    {
    case 1:
      mode = OPEN_READ;
      break;
    case 2:
      mode = OPEN_WRITE;
      break;

      /* default: Use the mode defined by O_ACCMODE */
    }

  dev = -1;
  open_flags = get_open_flags (mode, open_flags, file);

  switch (dev_type)
    {
    case OSS_DEV_VDSP:
      dev_type = OSS_DEV_DSP;
      break;
#if 0
    case OSS_DEV_VAUDIO:
      dev_type = OSS_DEV_DEVAUDIO;
      break;
#endif
    default:
      cmn_err (CE_NOTE, "Unknown dev class %d\n", dev_type);
      break;
    }

  if (audio_devfiles == NULL)
    {
      cmn_err (CE_NOTE, "No audio device files available\n");
      return OSS_ENXIO;
    }
#ifdef MANAGE_DEV_DSP
#ifdef VDEV_SUPPORT
  oss_combine_write_lists ();
#endif
#endif

#ifdef APPLIST_SUPPORT
  {
    char *appname;
    appname = GET_PROCESS_NAME (file);
    if (open_flags & OF_DEVAUDIO)
      appname = "Devaudio_Support";

    if ((dev = app_lookup (mode, appname, &open_flags)) >= 0)
      if (audio_devfiles[dev]->enabled && !audio_devfiles[dev]->unloaded)
	if ((ret =
	     oss_audio_open_devfile (dev, dev_type, file, 0, open_flags,
				     newdev)) >= 0)
	  {
	    dev = ret;
	    DDB (cmn_err
		 (CE_CONT, "Using dsp%d configured for this application\n",
		  dev));
	    goto done;
	  }
  }
#endif
  DDB (cmn_err (CE_CONT, "\n"));
  DDB (cmn_err (CE_CONT, "Out devs: "));
  for (i = 0; i < dspoutlist.ndevs; i++)
    DDB (cmn_err (CE_CONT, "%d ", dspoutlist.devices[i]));
  for (i = 0; i < dspoutlist2.ndevs; i++)
    DDB (cmn_err (CE_CONT, "(%d) ", dspoutlist2.devices[i]));
  DDB (cmn_err (CE_CONT, "\n"));
  DDB (cmn_err (CE_CONT, "In devs: "));
  for (i = 0; i < dspinlist.ndevs; i++)
    DDB (cmn_err (CE_CONT, "%d ", dspinlist.devices[i]));
  DDB (cmn_err (CE_CONT, "\n"));
  DDB (cmn_err (CE_CONT, "In/out devs: "));
  for (i = 0; i < dspinoutlist.ndevs; i++)
    DDB (cmn_err (CE_CONT, "%d ", dspinoutlist.devices[i]));
  DDB (cmn_err (CE_CONT, "\n"));

  switch (mode & (OPEN_READ | OPEN_WRITE))
    {
    case OPEN_WRITE:
      DDB (cmn_err (CE_CONT, "Selecting output device: "));

      for (i = 0; i < dspoutlist.ndevs; i++)
	{
	  dev = dspoutlist.devices[i];
	  if (!audio_devfiles[dev]->enabled || audio_devfiles[dev]->unloaded)
	    {
	      dev = -1;
	      continue;
	    }

	  DDB (cmn_err (CE_CONT, "%d ", dev));
	  if ((ret =
	       oss_audio_open_devfile (dev, dev_type, file, 0, open_flags,
				       newdev)) >= 0)
	    {
	      dev = ret;
	      DDB (cmn_err (CE_CONT, "->%d ", dev));
	      break;
	    }
	  dev = -1;
	}
      break;

    case OPEN_READ:
      DDB (cmn_err (CE_CONT, "Selecting input device: "));
      for (i = 0; i < dspinlist.ndevs; i++)
	{
	  dev = dspinlist.devices[i];
	  if (!audio_devfiles[dev]->enabled || audio_devfiles[dev]->unloaded)
	    {
	      dev = -1;
	      continue;
	    }
	  DDB (cmn_err (CE_CONT, "%d ", dev));
	  if ((ret =
	       oss_audio_open_devfile (dev, dev_type, file, 0, open_flags,
				       newdev)) >= 0)
	    {
	      dev = ret;
	      DDB (cmn_err (CE_CONT, "->%d ", dev));
	      break;
	    }

	  dev = -1;
	}
      break;

    case OPEN_WRITE | OPEN_READ:
      DDB (cmn_err (CE_CONT, "Selecting input/output device: "));
      for (i = 0; i < dspinoutlist.ndevs; i++)
	{
	  dev = dspinoutlist.devices[i];
	  if (!audio_devfiles[dev]->enabled || audio_devfiles[dev]->unloaded)
	    {
	      dev = -1;
	      continue;
	    }
	  DDB (cmn_err (CE_CONT, "%d ", dev));
	  if ((ret =
	       oss_audio_open_devfile (dev, dev_type, file, 0, open_flags,
				       newdev)) >= 0)
	    {
	      dev = ret;
	      DDB (cmn_err (CE_CONT, "->%d ", dev));
	      break;
	    }
	  dev = -1;
	}
      break;
    }

  DDB (cmn_err (CE_CONT, " - got vdsp -> %d\n", dev));
  if (dev == -1)
    return OSS_EBUSY;

done:
/*
 * Try to find which minor number matches this /dev/dsp# device. Note that the actual
 * device type doesn't matter after this point so we can use OSS_DEV_DSP.
 */
  if ((d = oss_find_minor (OSS_DEV_DSP_ENGINE, dev)) < 0)
    {
      oss_audio_release (dev, file);
      return d;
    }

  *newdev = d;
  return dev;
}
#endif
#endif

static int
audio_init_device (int dev)
{
  adev_p adev;

  sync_seed = GET_JIFFIES ();
  adev = audio_engines[dev];

  MUTEX_INIT (adev->master_osdev, adev->mutex, MH_FRAMEW);

  if (audio_engines[dev]->dmap_out == NULL
      || audio_engines[dev]->dmap_in == NULL)
    {
      dmap_p dmap;

      dmap = AUDIO_MALLOC (adev->osdev, sizeof (dmap_t));
      if (dmap == NULL)
	{
	  cmn_err (CE_WARN, "Failed to allocate dmap, dev=%d\n", dev);
	  return OSS_ENOMEM;
	}

      memset ((char *) dmap, 0, sizeof (dmap_t));

      if (!(adev->flags & ADEV_NOOUTPUT) && adev->out_wq == NULL)
	{
	  if ((adev->out_wq =
	       oss_create_wait_queue (adev->osdev, "audio_out")) == NULL)
	    {
	      cmn_err (CE_WARN, "Cannot create audio output wait queue\n");
	      return OSS_ENOMEM;
	    }
	}

      if (!(adev->flags & ADEV_NOINPUT) && adev->in_wq == NULL)
	{
	  if ((adev->in_wq =
	       oss_create_wait_queue (adev->osdev, "audio_in")) == NULL)
	    {
	      cmn_err (CE_WARN, "Cannot create audio input wait queue\n");
	      return OSS_ENOMEM;
	    }
	}

      memset ((char *) dmap, 0, sizeof (dmap_t));
      dmap->osdev = adev->osdev;
      dmap->adev = adev;
      dmap->master_osdev = adev->master_osdev;
      MUTEX_INIT (dmap->master_osdev, dmap->mutex, MH_FRAMEW + 1);
      adev->dmap_out = adev->dmap_in = dmap;

      if (adev->flags & ADEV_DUPLEX)
	{

	  dmap = AUDIO_MALLOC (adev->osdev, sizeof (dmap_t));
	  if (dmap == NULL)
	    {
	      cmn_err (CE_WARN, "Failed to allocate dmap, dev=%d\n", dev);
	      return OSS_ENOMEM;
	    }

	  memset ((char *) dmap, 0, sizeof (dmap_t));
	  dmap->osdev = adev->osdev;
	  dmap->adev = adev;
	  dmap->master_osdev = adev->master_osdev;
	  MUTEX_INIT (dmap->master_osdev, dmap->mutex, MH_FRAMEW + 1);
	  adev->dmap_in = dmap;
	}
    }

  return 0;
}

void
audio_uninit_device (int dev)
{
  adev_p adev;
  /* oss_native_word flags; */

  adev = audio_engines[dev];

  if (adev->unloaded)
    return;

  if (adev->dmap_out != NULL && adev->dmap_out->dmabuf != NULL)
    {
      if (adev->d->adrv_free_buffer != NULL)
	{
	  adev->d->adrv_free_buffer (dev, adev->dmap_out, OPEN_WRITE);
	}
      else
	default_free_buffer (dev, adev->dmap_out, OPEN_WRITE);
      adev->dmap_out->dmabuf = NULL;
    }

  if (adev->dmap_in != NULL
      && (adev->dmap_in != adev->dmap_out && adev->dmap_in->dmabuf != NULL))
    {
      if (adev->d->adrv_free_buffer != NULL)
	{
	  adev->d->adrv_free_buffer (dev, adev->dmap_in, OPEN_READ);
	}
      else
	default_free_buffer (dev, adev->dmap_in, OPEN_READ);
      adev->dmap_in->dmabuf = NULL;
    }

  if (adev->in_wq != NULL)
    {
      oss_remove_wait_queue (adev->in_wq);
      adev->in_wq = NULL;
    }
  if (adev->out_wq != NULL)
    {
      oss_remove_wait_queue (adev->out_wq);
      adev->out_wq = NULL;
    }

#ifdef CONFIG_OSS_VMIX
  if (adev->vmix_mixer != NULL)
     {
	     adev->vmix_mixer = NULL;
     }
#endif

  MUTEX_CLEANUP (adev->mutex);

  if (adev->dmap_out != NULL)
    MUTEX_CLEANUP (adev->dmap_out->mutex);

  if (adev->flags & ADEV_DUPLEX && adev->dmap_in != NULL
      && adev->dmap_out != adev->dmap_in)
    {
      MUTEX_CLEANUP (adev->dmap_in->mutex);
    }
  adev->unloaded = 1;
}

void
oss_audio_init (oss_device_t *osdev)
{
	MUTEX_INIT (osdev, audio_global_mutex, MH_DRV);
}

void
oss_audio_uninit (void)
{
/*
 * Release all memory/resources allocated by the audio core.
 */
  oss_memblk_unalloc(&audio_global_memblk);

  MUTEX_CLEANUP (audio_global_mutex);
}

void
oss_audio_inc_byte_counter (dmap_t * dmap, int increment)
{
  oss_uint64_t p1, p2;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
  p1 = dmap->byte_counter / dmap->fragment_size;
  dmap->byte_counter += increment;
  p2 = dmap->byte_counter / dmap->fragment_size;

  dmap->interrupt_count += (int) (p2 - p1);
  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
}

static void
do_inputintr (int dev, int intr_flags)
{
  adev_p adev;
  dmap_p dmap;
  int cptr;
  oss_native_word flags;

  adev = audio_engines[dev];
  dmap = adev->dmap_in;

  MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);

  if (!(intr_flags & AINTR_NO_POINTER_UPDATES))
    {
      dmap->byte_counter += dmap->fragment_size;
      dmap->interrupt_count++;
    }

  while (dmap->byte_counter > dmap->user_counter &&
	 (int) (dmap->byte_counter - dmap->user_counter) > dmap->bytes_in_use)
    {
      dmap->user_counter += dmap->fragment_size;
      dmap->rec_overruns++;
    }
  dmap->fragment_counter = (dmap->fragment_counter + 1) % dmap->nfrags;

  if (dmap->dmabuf_dma_handle != NULL) /* Some drivers don't use DMA */
     OSS_DMA_SYNC(dmap->dmabuf_dma_handle, 0, dmap->bytes_in_use, OSS_DMA_SYNC_INBOUND);

#ifdef DO_TIMINGS
  oss_do_timing ("Wake up (in)");
#endif
  oss_wakeup (adev->in_wq, &dmap->mutex, &flags, POLLIN | POLLRDNORM);

  if (adev->flags & ADEV_AUTOMODE)
    {
      goto finish;
    }

  cptr = dmap_get_qtail (dmap) * dmap->fragment_size;
  if (adev->d->adrv_start_input != NULL)
    adev->d->adrv_start_input (adev->engine_num, dmap->dmabuf_phys + cptr,
			       dmap->fragment_size, dmap->fragment_size, 1);
finish:
  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);

  if (dmap->audio_callback != NULL)
    dmap->audio_callback (adev->engine_num, dmap->callback_parm);
}

static void
audio_inputintr (int dev, int intr_flags)
{
  adev_p adev;
  dmap_p dmap;
  int n;
  oss_uint64_t pos;

  if (dev < 0 || dev >= num_audio_engines)
    return;

  adev = audio_engines[dev];
  dmap = adev->dmap_in;

  if (dmap->dma_mode != PCM_ENABLE_INPUT)
    {
      return;
    }

  if (!(dmap->flags & DMAP_STARTED))
    {
      return;
    }
#ifdef DO_TIMINGS
  oss_timing_printf ("Inputintr(%d)", dev);
#endif

  if (adev->d->adrv_get_input_pointer == NULL
      || (intr_flags & AINTR_NO_POINTER_UPDATES))
    {
      do_inputintr (dev, intr_flags);
      return;
    }

  pos =
    adev->d->adrv_get_input_pointer (adev->engine_num, dmap,
				     PCM_ENABLE_INPUT) / dmap->fragment_size;
  n = 0;

  while (dmap_get_qtail (dmap) != pos && n++ < dmap->nfrags)
    {
      do_inputintr (dev, intr_flags);
    }
}

static void
finish_output_interrupt (adev_p adev, dmap_p dmap)
{
  if (dmap->dmabuf_dma_handle != NULL) /* Some drivers don't use DMA */
     OSS_DMA_SYNC(dmap->dmabuf_dma_handle, 0, dmap->bytes_in_use, OSS_DMA_SYNC_OUTBOUND);

#ifdef CONFIG_OSSD
  ossd_event (adev->engine_num, OSSD_EV_UPDATE_OUTPUT);
#endif
  if (dmap->audio_callback != NULL)
    {
      dmap->audio_callback (adev->engine_num, dmap->callback_parm);
    }
}

static void
do_outputintr (int dev, int intr_flags)
{
  adev_p adev;
  dmap_p dmap;
  int cptr;
  oss_native_word flags;

  adev = audio_engines[dev];
  dmap = adev->dmap_out;

  MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);

  if (dmap->dmabuf == NULL)
    {
      MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
      cmn_err (CE_WARN, "Output interrupt when no buffer is allocated\n");
      return;
    }

  if (!(intr_flags & AINTR_NO_POINTER_UPDATES))
    {
      dmap->byte_counter += dmap->fragment_size;
      dmap->interrupt_count++;
    }

  dmap->fragment_counter = (dmap->fragment_counter + 1) % dmap->nfrags;

  if (dmap->user_counter <= dmap->byte_counter)	/* Underrun */
    {
      if (!(dmap->mapping_flags & DMA_MAP_MAPPED))
	{
#ifdef DO_TIMINGS
	  oss_timing_printf ("adev %d: Play underrun A", dev);
	  oss_timing_printf ("  User=%lld", dmap->user_counter);
	  oss_timing_printf ("  Dev=%lld", dmap->byte_counter);
	  oss_timing_printf ("  Tmp=%d", dmap->tmpbuf_ptr);
#endif
	  dmap->play_underruns++;

	  if (!dmap->underrun_flag)
	    {
#ifdef DO_TIMINGS
	      oss_do_timing ("Clearing the buffer");
#endif
	      memset (dmap->dmabuf, dmap->neutral_byte, dmap->buffsize);
	    }
	  dmap->underrun_flag = 1;
#if 1
	  dmap->user_counter = dmap->byte_counter + dmap->fragment_size;
#endif
	}
    }

  if (audio_space_in_queue (adev, dmap, 0) >= 0 || adev->nonblock)
    {
#ifdef DO_TIMINGS
      oss_do_timing ("Wake up (out)");
#endif
      oss_wakeup (adev->out_wq, &dmap->mutex, &flags, POLLOUT | POLLWRNORM);
    }

  if (adev->flags & ADEV_AUTOMODE)
    {
      MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
      finish_output_interrupt (adev, dmap);
      return;
    }

  cptr = dmap_get_qhead (dmap) * dmap->fragment_size;
  MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);
  if (adev->d->adrv_output_block != NULL)
    {
      adev->d->adrv_output_block (adev->engine_num, dmap->dmabuf_phys + cptr,
				  dmap->fragment_size, dmap->fragment_size,
				  1);
    }
  finish_output_interrupt (adev, dmap);
}

static void
audio_outputintr (int dev, int intr_flags)
{
  adev_p adev;
  dmap_p dmap;
  int n;
  oss_uint64_t pos;

  if (dev < 0 || dev >= num_audio_engines)
    return;

  adev = audio_engines[dev];
  dmap = adev->dmap_out;

  if (dmap->dma_mode != PCM_ENABLE_OUTPUT)
    {
      return;
    }

  if (!(dmap->flags & DMAP_STARTED))
    {
      return;
    }

#ifdef DO_TIMINGS
  oss_timing_printf ("Outputintr(%d)", dev);
#endif
  UP_STATUS (STS_INTR);

  if (adev->d->adrv_get_output_pointer == NULL
      || (intr_flags & AINTR_NO_POINTER_UPDATES))
    {
      UP_STATUS (STS_DOINTR);
      do_outputintr (dev, intr_flags);
      DOWN_STATUS (STS_DOINTR | STS_INTR);
      return;
    }

  pos = get_output_pointer (adev, dmap, 0) / dmap->fragment_size;
  n = 0;

  while (dmap_get_qhead (dmap) != pos && n++ < dmap->nfrags)
    {
      UP_STATUS (STS_DOINTR);
      do_outputintr (dev, intr_flags);
      DOWN_STATUS (STS_DOINTR);
    }
  DOWN_STATUS (STS_INTR);
}

void
oss_audio_reset (int dev)
{
  adev_p adev;
  if (dev < 0 || dev >= num_audio_engines)
    return;
  adev = audio_engines[dev];

  audio_reset_adev (adev);

  if (adev->dmask & DMASK_OUT)
    reset_dmap (adev->dmap_out);
  if (adev->dmask & DMASK_IN)
    reset_dmap (adev->dmap_in);
}

void
oss_audio_start_syncgroup (unsigned int syncgroup)
{
/*
 * This routine is to be called by the /dev/midi driver to start a sync group
 * at the right time.
 */
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (audio_global_mutex, flags);
  handle_syncstart (syncgroup & SYNC_DEVICE_MASK, syncgroup);
  MUTEX_EXIT_IRQRESTORE (audio_global_mutex, flags);
}

#ifdef ALLOW_SELECT
/*ARGSUSED*/
static int
chpoll_output (int dev, struct fileinfo *file, oss_poll_event_t * ev)
{
  short events = ev->events;
  adev_p adev;
  dmap_p dmap;
  oss_native_word flags;
  audio_buf_info bi;
  int limit;

#ifdef DO_TIMINGS
  oss_timing_printf ("--- audio_chpoll_output(%d, %08x) ---", dev, events);
#endif

  if (dev < 0 || dev >= num_audio_engines)
    return OSS_ENXIO;
  adev = audio_engines[dev];

      dmap = adev->dmap_out;
      if (dmap->mapping_flags & DMA_MAP_MAPPED)
	{
#if 1
      /*
       * It might actually be better to permit pollling in mmapped
       * mode rather than returning an error.
       */
      if (dmap->interrupt_count > 0)
	 ev->revents |= (POLLOUT | POLLWRNORM) & events;
      return 0;
#else
	  oss_audio_set_error (adev->engine_num, E_PLAY,
			       OSSERR (1014,
				       "select/poll called for an OSS device in mmap mode."),
			       0);
	  /*
	   * Errordesc:
	   * The select() and poll() system calls are not defined for OSS devices
	   * when the device is in mmap mode.
	   */
	  return OSS_EPERM;
#endif
	}

      if (dmap->dma_mode == PCM_ENABLE_INPUT)
	{
	  oss_audio_set_error (adev->engine_num, E_PLAY,
			       OSSERR (1015,
				       "select/poll called for wrong direction."),
			       0);
	  /*
	   * Errordesc: The application has opened the device in read-only
	   * mode but it tried to use select/poll to check if the device is 
	   * ready for write. This is pointless because that will never
	   * happen.
	   */
	  return 0;
	}

      get_ospace (audio_engines[dev], dmap, (ioctl_arg) & bi);
      limit = bi.fragsize * bi.fragstotal / 4;
      if (dmap->low_water > limit)
	dmap->low_water = limit;
      MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
      if (bi.bytes < dmap->low_water)
	{			/* No space yet */
#ifdef DO_TIMINGS
	  oss_do_timing ("Output not ready yet");
#endif
	  oss_register_poll (adev->out_wq, &dmap->mutex, &flags, ev);
	}
      else
	{
#ifdef DO_TIMINGS
	  oss_do_timing ("Reporting output ready");
#endif
	  ev->revents |= (POLLOUT | POLLWRNORM) & events;
	}
      MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);

  return 0;
}

/*ARGSUSED*/
static int
chpoll_input (int dev, struct fileinfo *file, oss_poll_event_t * ev)
{
  short events = ev->events;
  adev_p adev;
  dmap_p dmap;
  oss_native_word flags;
  audio_buf_info bi;
  int limit;

#ifdef DO_TIMINGS
  oss_timing_printf ("--- audio_chpoll_input(%d, %08x) ---", dev, events);
#endif

  if (dev < 0 || dev >= num_audio_engines)
    return OSS_ENXIO;
  adev = audio_engines[dev];

  dmap = adev->dmap_in;
      if (dmap->mapping_flags & DMA_MAP_MAPPED)
	{
#if 1
      /*
       * It might actually be better to permit pollling in mmapped
       * mode rather than returning an error.
       */
      if (dmap->interrupt_count > 0)
	 ev->revents |= (POLLOUT | POLLWRNORM) & events;
      return 0;
#else
	  oss_audio_set_error (adev->engine_num, E_REC,
			       OSSERR (1013,
				       "select/poll called for an OSS device in mmap mode."),
			       0);
	  /*
	   * Errordesc:
	   * The select() and poll() system calls are not defined for OSS devices
	   * when the device is in mmap mode.
	   */
	  return OSS_EIO;
#endif
	}

      if (dmap->dma_mode != PCM_ENABLE_INPUT)
	{
	  oss_audio_set_error (adev->engine_num, E_REC,
			       OSSERR (1016,
				       "select/poll called for wrong direction."),
			       0);
	  /*
	   * Errordesc: The application has opened the device in write-only
	   * mode but it tried to use select/poll to check if the device has any
	   * data available to read. This is pointless because that will never
	   * happen.
	   */
	  return 0;
	}

      if (!(dmap->flags & DMAP_STARTED))
	{
	  oss_audio_set_error (adev->engine_num, E_REC,
			       OSSERR (1017,
				       "select/poll called before recording is started."),
			       0);
	  /*
	   * Errordesc:
	   * The application should start recording (using SNDCTL_DSP_SETTRIGGER)
	   * before calling select/poll to wait for recorded data. If recording is
	   * not active then recorded data will never arrive and the application
	   * will just sit and wait without doing anything.
	   */
	}

      get_ispace (audio_engines[dev], dmap, (ioctl_arg) & bi);
      limit = bi.fragsize * bi.fragstotal / 4;
      if (dmap->low_water > limit)
	dmap->low_water = limit;
      MUTEX_ENTER_IRQDISABLE (dmap->mutex, flags);
      if (bi.bytes < dmap->low_water)
	{			/* No space yet */
#ifdef DO_TIMINGS
	  oss_timing_printf ("Input not ready yet (%d/%d)\n", bi.bytes,
		   dmap->low_water);
#endif
	  oss_register_poll (adev->in_wq, &dmap->mutex, &flags, ev);
	}
      else
	{
#ifdef DO_TIMINGS
	  oss_do_timing ("Reporting input ready");
#endif
	  ev->revents |= (POLLIN | POLLRDNORM) & events;
	}
      MUTEX_EXIT_IRQRESTORE (dmap->mutex, flags);

  return 0;
}

/*ARGSUSED*/
int
oss_audio_chpoll (int dev, struct fileinfo *file, oss_poll_event_t * ev)
{
  short events = ev->events;
  adev_p adev;

#ifdef DO_TIMINGS
  oss_timing_printf ("--- audio_chpoll(%d, %08x) ---", dev, events);
#endif

  if (dev < 0 || dev >= num_audio_engines)
    return OSS_ENXIO;
  adev = audio_engines[dev];

  if ((events & (POLLOUT | POLLWRNORM)) && (adev->open_mode & OPEN_WRITE))
    {
	    int err;

	    if ((err=chpoll_output (dev, file, ev))<0)
	       return err;
    }

  if ((events & (POLLIN | POLLRDNORM)) && (adev->open_mode & OPEN_READ))
    {
	    int err;

	    if ((err=chpoll_input (dev, file, ev))<0)
	       return err;
    }

  return 0;
}
#endif

/*ARGSUSED*/
static void
dummy_inputintr (int dev, int intr_flags)
{
  /* Dummy input interrupt handler */
}

/*ARGSUSED*/
static void
dummy_outputintr (int dev, int x)
{
  /* Dummy output interrupt handler */
}

static oss_cdev_drv_t audio_cdev_drv = {
#ifdef VDEV_SUPPORT
  oss_audio_open_devfile,
#else
  oss_audio_open_engine,
#endif
  oss_audio_release,
  oss_audio_read,
  oss_audio_write,
  oss_audio_ioctl,
#ifdef ALLOW_SELECT
  oss_audio_chpoll
#else
  NULL
#endif
};

#ifdef MANAGE_DEV_DSP
#ifdef VDEV_SUPPORT
static oss_cdev_drv_t vdsp_cdev_drv = {
  oss_open_vdsp,
  oss_audio_release,
  oss_audio_read,
  oss_audio_write,
  oss_audio_ioctl,
#ifdef ALLOW_SELECT
  oss_audio_chpoll
#else
  NULL
#endif
};
#endif
#endif

static oss_cdev_drv_t audio_engine_cdev_drv = {
  oss_audio_open_engine,
  oss_audio_release,
  oss_audio_read,
  oss_audio_write,
  oss_audio_ioctl,
#ifdef ALLOW_SELECT
  oss_audio_chpoll
#else
  NULL
#endif
};

void
oss_audio_register_client (oss_audio_startup_func func, void *devc,
			   oss_device_t * osdev)
{
/*
 * Register a callback for a driver that wants to be initialized after a new audio device
 * is installed.
 */

  if (num_audio_startups >= MAX_STARTUPS)
    return;

  audio_startups[num_audio_startups].func = func;
  audio_startups[num_audio_startups].devc = devc;
  audio_startups[num_audio_startups].osdev = osdev;
  num_audio_startups++;
}

void
oss_add_audio_devlist (int list_id, int devfile)
{
#if 0
/*
 * Device list support is not in use at this moment
 */
  oss_devlist_t *list;
  int i;

  switch (list_id)
    {
    case OPEN_READ:
      list = &dspinlist;
      break;

    case OPEN_WRITE:
      list = &dspoutlist;
      break;

    case OPEN_READ | OPEN_WRITE:
      list = &dspinoutlist;
      break;

    default:
      cmn_err (CE_CONT, "Bad device list ID %d\n", list_id);
      return;
    }

  for (i = 0; i < list->ndevs; i++)
    if (list->devices[i] == devfile)	/* Already there */
      return;

  /*
   * Add the device to the list
   */
  list->devices[list->ndevs++] = devfile;
#endif
}

#if 0
static void
add_to_devlists (adev_p adev)
{
/*
 * Add the device to the default input, output and/or input/output lists
 * for /dev/dsp.
 */

  if (!(adev->flags & ADEV_SPECIAL))
    {
      if (!(adev->flags & ADEV_NOOUTPUT))
	{			/* Output capable */

	  if (adev->flags & ADEV_VIRTUAL)
	    {
	      int n = dspoutlist.ndevs++;

	      if (adev->flags & ADEV_DEFAULT)
		{
		  int i;

		  for (i = n; i > 0; i--)
		    dspoutlist.devices[i] = dspoutlist.devices[i - 1];
		  dspoutlist.devices[0] = adev->audio_devfile;
		}
	      else
		dspoutlist.devices[n] = adev->audio_devfile;
	    }
	  else
	    {
	      /* Put non-virtual devices to a secondary list */
	      int n = dspoutlist2.ndevs++;
	      dspoutlist2.devices[n] = adev->audio_devfile;
	    }

	}

      if (!(adev->flags & (ADEV_NOINPUT)))
	{			/* Input capable */

	  int n = dspinlist.ndevs++;

	  if (adev->flags & ADEV_DEFAULT)
	    {
	      int i;

	      for (i = n; i > 0; i--)
		dspinlist.devices[i] = dspinlist.devices[i - 1];
	      dspinlist.devices[0] = adev->audio_devfile;
	    }
	  else
	    dspinlist.devices[n] = adev->audio_devfile;
	}

      if (!(adev->flags & (ADEV_NOINPUT | ADEV_NOOUTPUT))
	  || (adev->flags & ADEV_DUPLEX))
	{			/* Input/output capable */

	  int n = dspinoutlist.ndevs++;

	  if (adev->flags & ADEV_DEFAULT)
	    {
	      int i;

	      for (i = n; i > 0; i--)
		dspinoutlist.devices[i] = dspinoutlist.devices[i - 1];
	      dspinoutlist.devices[0] = adev->audio_devfile;
	    }
	  else
	    dspinoutlist.devices[n] = adev->audio_devfile;
	}
    }
}
#endif

static int
resize_array(oss_device_t *osdev, adev_t ***arr, int *size, int increment)
{
	adev_t **old=*arr, **new = *arr;
	int old_size = *size;
	int new_size = *size;
		
	if (new_size >= HARD_MAX_AUDIO_DEVFILES) /* Too many device files */
	   return 0;

	new_size += increment;
	if (new_size > HARD_MAX_AUDIO_DEVFILES)
           new_size = HARD_MAX_AUDIO_DEVFILES;

	if ((new=AUDIO_MALLOC(osdev, new_size * sizeof (adev_t *)))==NULL)
	   return 0;

	memset(new, 0, new_size * sizeof(adev_t *));
	if (old != NULL)
	   memcpy(new, old, old_size * sizeof(adev_t *));

	*size = new_size;
	*arr = new;

	if (old != NULL)
	   AUDIO_FREE(osdev, old);

	return 1;
}

int
oss_install_audiodev_with_devname (int vers,
		      oss_device_t * osdev,
		      oss_device_t * master_osdev,
		      char *name,
		      const audiodrv_t * driver,
		      int driver_size,
		      int flags,
		      unsigned int format_mask, void *devc, int parent,
		      const char * devfile_name)
{
  audiodrv_t *d;
  adev_t *op;
  int i, num = -1, hidden_device = 0;
  int update_devlists = 0;
  int reinsterted_device = 0;
  int chdev_flags = 0;
  int devfile_num = 0;

  if (devc == NULL)
     {
	     cmn_err(CE_WARN, "devc==NULL for %s. Cannot install audio device\n", name);
	     return OSS_EINVAL;
     }

  if (name == NULL)
    cmn_err (CE_CONT, "Name is really NULL\n");
  if (master_osdev == NULL)
    master_osdev = osdev;

#ifdef VDEV_SUPPORT
  if (flags & (ADEV_SHADOW | ADEV_HIDDEN))
    hidden_device = 1;
#else
  flags &= ~ADEV_HIDDEN;
  hidden_device = 0;
#endif

  if (audio_engines == NULL)
    {
      audio_engines = AUDIO_MALLOC (NULL, sizeof (adev_t *) * MAX_AUDIO_ENGINES);
      memset (audio_engines, 0, sizeof (adev_t *) * MAX_AUDIO_ENGINES);
    }

  if (audio_devfiles == NULL)
    {
      audio_devfiles = AUDIO_MALLOC (NULL, sizeof (adev_t *) * MAX_AUDIO_DEVFILES);
      memset (audio_devfiles, 0, sizeof (adev_t *) * MAX_AUDIO_DEVFILES);
    }

  if (vers != OSS_AUDIO_DRIVER_VERSION)
    {
      cmn_err (CE_WARN, "Incompatible audio driver for %s\n", name);
      return OSS_EIO;
    }

  if (driver_size > sizeof (audiodrv_t))
    driver_size = sizeof (audiodrv_t);
/*
 * Try to figure out if this device has earlier been installed. Sometimes it may happen that
 * the low level driver gets unloaded while the osscore module still
 * remains loaded. Try to re-use the same audio device number if possible.
 */

  num = -1;

  for (i = 0; i < num_audio_engines; i++)
    {

      if ((audio_engines[i]->flags & (ADEV_SHADOW|ADEV_HIDDEN)) !=
		      (flags & (ADEV_SHADOW|ADEV_HIDDEN))) /* Different visibility */
	 continue;

      if (audio_engines[i]->unloaded
	  && audio_engines[i]->os_id == oss_get_osid (osdev))
	{
	  /* This audio device was previously connected to the same physical device */

	  num = i;
	  reinsterted_device = 1;
	  DDB (cmn_err
	       (CE_NOTE, "Audio device %d got re-installed again.\n", i));
	  break;
	}
    }

  if (num == -1)
    {

      if (num_audio_engines >= MAX_AUDIO_ENGINES)
	{
		if (!resize_array(osdev, &audio_engines, &oss_max_audio_engines, AUDIO_ENGINE_INCREMENT))
		   {
		   	cmn_err (CE_CONT, "Cannot grow audio_engines[]\n");
			return OSS_EIO;
		   }
	}

      d = AUDIO_MALLOC (osdev, sizeof (audiodrv_t));
      op = AUDIO_MALLOC (osdev, sizeof (adev_t));
      memset ((char *) op, 0, sizeof (adev_t));
      if (driver_size < sizeof (audiodrv_t))
	memset ((char *) d, 0, sizeof (audiodrv_t));
      memcpy ((char *) d, (char *) driver, driver_size);
      num = num_audio_engines;
      audio_engines[num] = op;
      num_audio_engines++;

      update_devlists = 1;

      sprintf (op->handle, "%s-au%02d", osdev->handle,
	       osdev->num_audio_engines + 1);
      op->port_number = osdev->num_audio_engines;
    }
  else
    {
      op = audio_engines[num];
      d = audio_engines[num]->d;
      if (driver_size < sizeof (audiodrv_t))
	memset ((char *) d, 0, sizeof (audiodrv_t));
      memcpy ((char *) d, (char *) driver, driver_size);
      update_devlists = 0;
    }

  if (d == NULL || op == NULL)
    {
      cmn_err (CE_WARN, "Can't allocate driver for %s (adev=%p, d=%p)\n",
	       name, op, d);
      return OSS_ENOSPC;
    }

  if (d->adrv_get_input_pointer == NULL)
    d->adrv_get_input_pointer = d->adrv_get_buffer_pointer;
  if (d->adrv_get_output_pointer == NULL)
    d->adrv_get_output_pointer = d->adrv_get_buffer_pointer;

  op->d = d;
  op->os_id = oss_get_osid (osdev);
  op->enabled = 0;
  op->outputintr = dummy_outputintr;
  op->inputintr = dummy_inputintr;
  op->vmix_mixer = NULL;

  strncpy (op->name, name, sizeof (op->name));
  op->name[sizeof (op->name) - 1] = 0;
  op->caps = 0;

  op->flags = flags;
  op->latency = -1;		/* Unknown */

  op->oformat_mask = format_mask;
  op->iformat_mask = format_mask;
  op->mixer_dev = -1;
  op->min_channels = 1;
  op->max_channels = 2;
  op->min_rate = 8000;
  op->max_rate = 44100;
  op->devc = devc;
  op->parent_dev = parent + 1;
  op->dmap_in = NULL;
  op->dmap_out = NULL;
  memset (op->cmd, 0, sizeof (op->cmd));
  op->pid = -1;
  op->osdev = osdev;
  op->master_osdev = master_osdev;
  op->card_number = osdev->cardnum;
  op->engine_num = op->rate_source = num;
  op->real_dev = num;
  op->redirect_in = -1;
  op->redirect_out = -1;
  op->dmabuf_alloc_flags = 0;
  op->dmabuf_maxaddr = MEMLIMIT_32BITS;

  audio_engines[num] = op;
  op->next_out = NULL;
  op->next_in = NULL;

  op->song_name[0] = 0;
  op->label[0] = 0;
  op->nrates=0;

  if ((flags & ADEV_SHADOW) && num > 0)
    {
      adev_p prev = audio_engines[num - 1];	/* Previous device */

      prev->next_out = op;
      prev->next_in = op;
    }

  if (audio_init_device (num) < 0)
    return OSS_ENOMEM;

/*
 * Create the device node.
 */
#ifndef VDEV_SUPPORT
  flat_device_model = 1;
  hidden_device = 0;
#endif

  if (reinsterted_device)
    {
      /*
       * A hotpluggable device has been reinserted in the system.
       * Update the internal tables and re-create the device file entry.
       * However don't create new audio engine and device file numbers.
       */
      devfile_num = op->audio_devfile;
      chdev_flags |= CHDEV_REPLUG;
    }
  else
    {
      if (!hidden_device)
	{
	  if (num_audio_devfiles >= MAX_AUDIO_DEVFILES)
	     {
		if (!resize_array(osdev, &audio_devfiles, &oss_max_audio_devfiles, AUDIO_DEVFILE_INCREMENT))
		   {
		   	cmn_err (CE_CONT, "Cannot grow audio_devfiles[]\n");
			return num;
		   }
	     }

      	     audio_devfiles[num_audio_devfiles] = op;
	     devfile_num = num_audio_devfiles++;
	}
    }

  if (flat_device_model || !hidden_device)
    {
#ifdef NEW_DEVICE_NAMING
      oss_devnode_t name;
      char tmpl[32];

      if (*devfile_name != 0)
	{
	  /*
	   * A name was suggested by the low level driver
	   */
	  strcpy (tmpl, devfile_name);
	}
      else if (flags & ADEV_NOOUTPUT)
	sprintf (tmpl, "pcmin%d", osdev->num_audiorec++);
      else
	sprintf (tmpl, "pcm%d", osdev->num_audioduplex++);

# ifdef USE_DEVICE_SUBDIRS
      sprintf (name, "oss/%s/%s", osdev->nick, tmpl);
# else
      sprintf (name, "%s_%s", osdev->nick, tmpl);
# endif
#else
      sprintf (name, "dsp%d", num);
#endif

      op->real_dev = devfile_num;
      op->audio_devfile = devfile_num;
      audio_devfiles[devfile_num] = op;
      sprintf (op->devnode, "/dev/%s", name);
      oss_install_chrdev (master_osdev, name, OSS_DEV_DSP, devfile_num,
			  &audio_cdev_drv, chdev_flags);
      osdev->num_audio_engines++;

      op->enabled = 1;
      op->unloaded = 0;

    }
  else
    {
      op->real_dev = -1;
      if (devfile_num > 0 && (flags & ADEV_SHADOW))
	{
	  /* Use the device node of the parent device file */
	  if (audio_devfiles[devfile_num - 1] != NULL)
	    strcpy (op->devnode, audio_devfiles[devfile_num - 1]->devnode);
	  else
	    strcpy (op->devnode, "Unknown");
	  audio_devfiles[devfile_num] = op;
	}
      else
	{
	  strcpy (op->devnode, "HiddenAudioDevice");
	}
      op->enabled = 1;
      op->unloaded = 0;
    }

//#ifdef VDEV_SUPPORT
  oss_install_chrdev (osdev, NULL, OSS_DEV_DSP_ENGINE, num,
		      &audio_engine_cdev_drv, chdev_flags);
//#endif

#if 0
  if (!reinsterted_device && update_devlists && !hidden_device)
    add_to_devlists (op);
#endif

  {
    int i;
    for (i = 0; i < num_audio_engines; i++)
      if (audio_engines[i] == NULL)
	cmn_err (CE_WARN, "Audio engines %d==NULL\n", i);
    for (i = 0; i < num_audio_devfiles; i++)
      if (audio_devfiles[i] == NULL)
	cmn_err (CE_WARN, "Audio devfiles %d==NULL\n", i);
  }

  return num;
}

int
oss_install_audiodev (int vers,
		      oss_device_t * osdev,
		      oss_device_t * master_osdev,
		      char *name,
		      const audiodrv_t * driver,
		      int driver_size,
		      unsigned long long flags,
		      unsigned int format_mask, void *devc, int parent)
{
 return oss_install_audiodev_with_devname (vers,
		      osdev,
		      master_osdev,
		      name,
		      driver,
		      driver_size,
		      flags,
		      format_mask, devc, parent,
		      ""); /* Use default device file naming */
}

void
oss_audio_delayed_attach (void)
{
  int i;

  /*
   * Serve possible other drivers waiting for new friends.
   */
  for (i = 0; i < num_audio_startups; i++)
    {
      if (audio_startups[i].osdev != NULL
	  && audio_startups[i].osdev->available)
	{
	  if (audio_startups[i].func (audio_startups[i].devc))
	    audio_startups[i].osdev = NULL;	/* Inactivate it */
	}
    }
}

/*ARGSUSED*/
void
install_vdsp (oss_device_t * osdev)
{
#ifdef MANAGE_DEV_DSP
/*
 * Install the driver for /dev/dsp (the multiplexer device)
 *
 * NOTE! Not used at this moment because /dev/dsp assignment is managed in user
 * 	 space (by ossdevlinks).
 */
#ifdef VDEV_SUPPORT
  oss_install_chrdev (osdev, "dsp", OSS_DEV_VDSP, 0, &vdsp_cdev_drv,
		      CHDEV_VIRTUAL);
  oss_install_chrdev (osdev, "dsp_in", OSS_DEV_VDSP, 1, &vdsp_cdev_drv,
		      CHDEV_VIRTUAL);
  oss_install_chrdev (osdev, "dsp_out", OSS_DEV_VDSP, 2, &vdsp_cdev_drv,
		      CHDEV_VIRTUAL);
#endif
#endif
}
