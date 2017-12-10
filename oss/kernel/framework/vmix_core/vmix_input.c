/*
 * Purpose: Virtual mixing audio driver recording routines
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

#define SWAP_SUPPORT
#include <oss_config.h>
#include "vmix.h"

#if 0
/* Debugging macros */
extern unsigned char tmp_status;
# define UP_STATUS(v) OUTB(NULL, (tmp_status=tmp_status|(v)), 0x378)
# define DOWN_STATUS(v) OUTB(NULL, (tmp_status=tmp_status&~(v)), 0x378)
#else
# define UP_STATUS(v)
# define DOWN_STATUS(v)
#endif

#undef SINE_DEBUG

#ifndef CONFIG_OSS_VMIX_FLOAT
#undef SINE_DEBUG
#endif

#ifdef SINE_DEBUG
#define SINE_SIZE	48
static const float sine_table[SINE_SIZE] = {
  0.000000, 0.130526, 0.258819, 0.382683,
  0.500000, 0.608761, 0.707107, 0.793353,
  0.866025, 0.923880, 0.965926, 0.991445,
  1.000000, 0.991445, 0.965926, 0.923880,
  0.866025, 0.793353, 0.707107, 0.608761,
  0.500000, 0.382683, 0.258819, 0.130526,
  0.000000, -0.130526, -0.258819, -0.382683,
  -0.500000, -0.608761, -0.707107, -0.793353,
  -0.866025, -0.923880, -0.965926, -0.991445,
  -1.000000, -0.991445, -0.965926, -0.923880,
  -0.866025, -0.793353, -0.707107, -0.608761,
  -0.500000, -0.382683, -0.258819, -0.130526
};
static int sine_phase[2] = { 0 };
#endif

/*
 * Recording import functions (from the physical devices)
 */
#undef  INT_IMPORT
#define INT_IMPORT(x) (x * 256)

static void
import16ne (vmix_engine_t * eng, void *inbuf, vmix_sample_t * chbufs[],
	    int channels, int samples)
{
  short *op;
#define SAMPLE_TYPE	short
#define SAMPLE_RANGE	32768.0
#undef VMIX_BYTESWAP
#define VMIX_BYTESWAP(x) x

#include "vmix_import.inc"
}

static void
import16oe (vmix_engine_t * eng, void *inbuf, vmix_sample_t * chbufs[],
	    int channels, int samples)
{
  short *op;
#undef  SAMPLE_TYPE
#undef  SAMPLE_RANGE
#define SAMPLE_TYPE	short
#define SAMPLE_RANGE	32768.0
#undef VMIX_BYTESWAP
#define VMIX_BYTESWAP(x) bswap16(x)

#include "vmix_import.inc"
}

#undef  INT_IMPORT
#define INT_IMPORT(x) (x / 256)

static void
import32ne (vmix_engine_t * eng, void *inbuf, vmix_sample_t * chbufs[],
	    int channels, int samples)
{
  int *op;
#undef  SAMPLE_TYPE
#undef  SAMPLE_RANGE
#define SAMPLE_TYPE	int
#define SAMPLE_RANGE	2147483648.0
#undef VMIX_BYTESWAP
#define VMIX_BYTESWAP(x) x

#include "vmix_import.inc"
}

static void
import32oe (vmix_engine_t * eng, void *inbuf, vmix_sample_t * chbufs[],
	    int channels, int samples)
{
  int *op;
#define SAMPLE_TYPE	int
#define SAMPLE_RANGE	2147483648.0
#undef VMIX_BYTESWAP
#define VMIX_BYTESWAP(x) bswap32(x)

#include "vmix_import.inc"
}

/*
 * recording export functions to virtual devices
 */
#undef  BUFFER_TYPE
#define BUFFER_TYPE short *

#undef  INT_EXPORT
#define INT_EXPORT(x) (x / 256)

void
vmix_rec_export_16ne (vmix_portc_t * portc, int nsamples)
{
  short *outp;
#undef VMIX_BYTESWAP
#define VMIX_BYTESWAP(x) x
#ifdef CONFIG_OSS_VMIX_FLOAT
  double range = 32767.0;
#endif
#include "rec_export.inc"
}

void
vmix_rec_export_16oe (vmix_portc_t * portc, int nsamples)
{
  short *outp;
#undef VMIX_BYTESWAP
#define VMIX_BYTESWAP(x) bswap16(x)
#ifdef CONFIG_OSS_VMIX_FLOAT
  double range = 32767.0;
#endif
#include "rec_export.inc"
}

#undef BUFFER_TYPE
#define BUFFER_TYPE int *
#undef  INT_EXPORT
#define INT_EXPORT(x) (x * 256)

void
vmix_rec_export_32ne (vmix_portc_t * portc, int nsamples)
{
  int *outp;
#undef VMIX_BYTESWAP
#define VMIX_BYTESWAP(x) x
#ifdef CONFIG_OSS_VMIX_FLOAT
  double range = 2147483647.0;
#endif
#include "rec_export.inc"
}

void
vmix_rec_export_32oe (vmix_portc_t * portc, int nsamples)
{
  int *outp;
#undef VMIX_BYTESWAP
#define VMIX_BYTESWAP(x) bswap32(x)
#ifdef CONFIG_OSS_VMIX_FLOAT
  double range = 2147483647.0;
#endif
#include "rec_export.inc"
}

#ifdef CONFIG_OSS_VMIX_FLOAT
void
vmix_rec_export_float (vmix_portc_t * portc, int nsamples)
{
  float *outp;
#undef BUFFER_TYPE
#define BUFFER_TYPE float *
#undef VMIX_BYTESWAP
#define VMIX_BYTESWAP(x) x
  double range = 1.0;
#include "rec_export.inc"
}
#endif

static void
vmix_record_callback (int dev, int parm)
{
  int i, n;
  int do_input = 0;

  adev_t *adev = audio_engines[dev];
  dmap_t *dmap = adev->dmap_in;
  oss_native_word flags;

  vmix_mixer_t *mixer = adev->vmix_mixer;
  vmix_engine_t *eng = &mixer->record_engine;

#ifdef CONFIG_OSS_VMIX_FLOAT
  fp_env_t fp_buf;
  short *fp_env = fp_buf;
  fp_flags_t fp_flags;
#endif

  if (mixer == NULL) /* Houston, we have a problem. */
     return;

  /*
   * Check if any input applications are active. Skip input processing
   * if it's not needed (to save CPU cycles).
   */

  for (i = 0; i < mixer->num_clientdevs; i++)
    if (mixer->client_portc[i]->trigger_bits & PCM_ENABLE_INPUT)
      do_input = 1;

  if (!do_input) /* Skip all input processing */
     {
	  n = 0;
	  while (n++ < dmap->nfrags
		 && (int) (dmap->byte_counter - dmap->user_counter) >=
		 dmap->fragment_size)
	    {
		  dmap->user_counter += dmap->fragment_size;
	    }
	  return;
     }

  UP_STATUS (0x02);
  MUTEX_ENTER_IRQDISABLE (mixer->mutex, flags);
#ifdef CONFIG_OSS_VMIX_FLOAT
  {
    /*
     * Align the FP save buffer to 16 byte boundary
     */
    oss_native_word p;
    p = (oss_native_word) fp_buf;

    p = ((p + 15ULL) / 16) * 16;
    fp_env = (short *) p;
  }

  FP_SAVE (fp_env, fp_flags);
#endif

  n = 0;
  while (n++ < dmap->nfrags
	 && (int) (dmap->byte_counter - dmap->user_counter) >=
	 dmap->fragment_size)
    {
      int i, p;
      unsigned char *inbuf;

      if (!do_input)
	{
	  /*
	   * Just skip the recorded data becaus nobody needs it.
	   */
	  dmap->user_counter += dmap->fragment_size;
	  continue;
	}

      for (i = 0; i < eng->channels; i++)
	{
	  memset (eng->chbufs[i], 0, CHBUF_SAMPLES * sizeof (vmix_sample_t));
	}

      p = (int) (dmap->user_counter % dmap->bytes_in_use);
      inbuf = dmap->dmabuf + p;

      eng->converter (eng, inbuf, eng->chbufs, eng->channels,
		      eng->samples_per_frag);

      for (i = 0; i < mixer->num_clientdevs; i++)
	{
	  vmix_portc_t *portc = mixer->client_portc[i];

	  if (portc->trigger_bits & PCM_ENABLE_INPUT)
	    {
	      if (portc->rec_mixing_func == NULL)
		continue;
	      if (portc->rec_choffs + portc->channels >
		  mixer->record_engine.channels)
		portc->rec_choffs = 0;
	      portc->rec_mixing_func (portc,
				      mixer->record_engine.samples_per_frag);
	    }
	}

      dmap->user_counter += dmap->fragment_size;
    }

#ifdef CONFIG_OSS_VMIX_FLOAT
  FP_RESTORE (fp_env, fp_flags);
#endif
  MUTEX_EXIT_IRQRESTORE (mixer->mutex, flags);

/*
 * Call oss_audio_inputintr outside FP mode because it may 
 * cause a task switch (under Solaris). Task switch may turn on CR0.TS under
 * x86 which in turn will cause #nm exception.
 */
  for (i = 0; i < mixer->num_clientdevs; i++)
    if (mixer->client_portc[i]->trigger_bits & PCM_ENABLE_INPUT)
      {
	vmix_portc_t *portc = mixer->client_portc[i];
	oss_audio_inputintr (portc->audio_dev, 0);
      }
  DOWN_STATUS (0x02);
}

void
finalize_record_engine (vmix_mixer_t * mixer, int fmt, adev_t * adev,
			dmap_p dmap)
{
  int i;

  switch (fmt)
    {
    case AFMT_S16_NE:
      mixer->record_engine.bits = 16;
      mixer->record_engine.converter = import16ne;
      break;

    case AFMT_S16_OE:
      mixer->record_engine.bits = 16;
      mixer->record_engine.converter = import16oe;
      break;

    case AFMT_S32_NE:
      mixer->record_engine.bits = 32;
      mixer->record_engine.converter = import32ne;
      break;

    case AFMT_S32_OE:
      mixer->record_engine.bits = 32;
      mixer->record_engine.converter = import32oe;
      break;

    default:
      cmn_err (CE_CONT, "Unrecognized recording sample format %x\n", fmt);
      return;
    }

  mixer->record_engine.fragsize = dmap->fragment_size;

  mixer->record_engine.samples_per_frag =
    mixer->record_engine.fragsize / mixer->record_engine.channels /
    (mixer->record_engine.bits / 8);

  if (mixer->record_engine.samples_per_frag > CHBUF_SAMPLES)
    {
      cmn_err (CE_WARN, "Too many samples per fragment (%d,%d)\n",
	       mixer->record_engine.samples_per_frag, CHBUF_SAMPLES);
      return;
    }

  for (i = 0; i < mixer->record_engine.channels; i++)
    if (mixer->record_engine.chbufs[i] == NULL)	/* Not allocated yet */
      {
	mixer->record_engine.chbufs[i] =
	  PMALLOC (mixer->master_osdev,
		   CHBUF_SAMPLES * sizeof (vmix_sample_t));
	if (mixer->record_engine.chbufs[i] == NULL)
	  {
	    cmn_err (CE_WARN, "Out of memory\n");
	    return;
	  }
      }

  dmap->audio_callback = vmix_record_callback;	/* Enable conversions */
  dmap->callback_parm = mixer->instance_num;
  dmap->dma_mode = PCM_ENABLE_INPUT;

  if (mixer->num_clientdevs > 1)
  {
    adev->redirect_out = mixer->client_portc[0]->audio_dev;
    adev->vmix_mixer = mixer;
  }
  vmix_record_callback (mixer->inputdev, mixer->instance_num);
}

void
vmix_setup_record_engine (vmix_mixer_t * mixer, adev_t * adev, dmap_t * dmap)
{
  int fmt;
  int old_min;
  int frags = 0x7fff0007;	/* fragment size of 128 bytes */

/*
 * Sample format (and endianess) setup 
 *
 */

  // First make sure a sane format is selected before starting to probe
  fmt = adev->d->adrv_set_format (mixer->inputdev, AFMT_S16_LE);
  fmt = adev->d->adrv_set_format (mixer->inputdev, AFMT_S16_NE);

  // Find out the "best" sample format supported by the device

  if (adev->iformat_mask & AFMT_S16_OE)
    fmt = AFMT_S16_OE;
  if (adev->iformat_mask & AFMT_S16_NE)
    fmt = AFMT_S16_NE;
  if (mixer->multich_enable)
    {
      if (adev->iformat_mask & AFMT_S32_OE)
	fmt = AFMT_S32_OE;
      if (adev->iformat_mask & AFMT_S32_NE)
	fmt = AFMT_S32_NE;
    }

  fmt = adev->d->adrv_set_format (mixer->inputdev, fmt);
  mixer->record_engine.fmt = fmt;

/*
 * Number of channels
 */
  mixer->record_engine.channels = mixer->max_channels;

  if (mixer->record_engine.channels > MAX_REC_CHANNELS)
     mixer->record_engine.channels = MAX_REC_CHANNELS;

  if (!mixer->multich_enable)
    mixer->record_engine.channels = 2;

  /* Force the device to stereo before trying with (possibly) imultiple channels */
  adev->d->adrv_set_channels (mixer->inputdev, 2);

  mixer->record_engine.channels = 
    adev->d->adrv_set_channels (mixer->inputdev,
				mixer->record_engine.channels);

  if (mixer->record_engine.channels > MAX_REC_CHANNELS)
    {
      cmn_err (CE_WARN,
	       "Number of channels (%d) is larger than maximum (%d)\n",
	       mixer->record_engine.channels, MAX_REC_CHANNELS);
      return;
    }

  /*
   * Try to set the same rate than for playback.
   */
  mixer->record_engine.rate =
    oss_audio_set_rate (mixer->inputdev, mixer->play_engine.rate);

  if (mixer->record_engine.rate <= 22050)
    frags = 0x7fff0004;		/* Use smaller fragments */

  audio_engines[mixer->inputdev]->hw_parms.channels =
    mixer->record_engine.channels;
  audio_engines[mixer->inputdev]->hw_parms.rate = mixer->record_engine.rate;
  audio_engines[mixer->inputdev]->dmap_in->data_rate =
    mixer->record_engine.rate * mixer->record_engine.channels *
    mixer->record_engine.bits / 8;
  audio_engines[mixer->inputdev]->dmap_in->frame_size =
    mixer->record_engine.channels * mixer->record_engine.bits / 8;

  old_min = adev->min_fragments;

#if 0
  if ((adev->max_fragments == 0 || adev->max_fragments >= 4)
      && adev->min_block == 0)
    adev->min_fragments = 4;
#endif

  oss_audio_ioctl (mixer->inputdev, NULL, SNDCTL_DSP_SETFRAGMENT,
		   (ioctl_arg) & frags);
  oss_audio_ioctl (mixer->inputdev, NULL, SNDCTL_DSP_GETBLKSIZE,
		   (ioctl_arg) & mixer->record_engine.fragsize);

  dmap->bytes_in_use = dmap->fragment_size * dmap->nfrags;

  oss_audio_ioctl (mixer->inputdev, NULL, SNDCTL_DSP_GETBLKSIZE,
		   (ioctl_arg) & mixer->record_engine.fragsize);
  mixer->record_engine.fragsize = dmap->fragment_size;
  adev->min_fragments = old_min;

  if (mixer->record_engine.channels > 2)
    {
      DDB (cmn_err
	   (CE_CONT, "Enabling multi channel rec mode, %d hw channels\n",
	    mixer->record_engine.channels));
    }
  else if (mixer->record_engine.channels != 2)
    {
      cmn_err (CE_WARN,
	       "Master device doesn't support suitable channel configuration\n");

      return;
    }

  finalize_record_engine (mixer, fmt, adev, dmap);
}
