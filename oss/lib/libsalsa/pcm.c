/*
 *  Copyright (c) 2004 by Hannu Savolainen < hannu@opensound.com>
 *
 *   Parts of the code is derived from the alsa-lib package that is
 *   copyrighted by Jaroslav Kysela and the other ALSA team members.
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License version 2.1 as
 *   published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
#define USE_VERSIONED_SYMBOLS
#define DO_VERSIONING
#define HAVE_ELF
#define PERIOD_SIZE	128

#include "local.h"

#define PCMTYPE(v) [SND_PCM_TYPE_##v] = #v
#define STATE(v) [SND_PCM_STATE_##v] = #v
#define STREAM(v) [SND_PCM_STREAM_##v] = #v
#define READY(v) [SND_PCM_READY_##v] = #v
#define XRUN(v) [SND_PCM_XRUN_##v] = #v
#define SILENCE(v) [SND_PCM_SILENCE_##v] = #v
#define TSTAMP(v) [SND_PCM_TSTAMP_##v] = #v
#define ACCESS(v) [SND_PCM_ACCESS_##v] = #v
#define START(v) [SND_PCM_START_##v] = #v
#define HW_PARAM(v) [SND_PCM_HW_PARAM_##v] = #v
#define SW_PARAM(v) [SND_PCM_SW_PARAM_##v] = #v
#define FORMAT(v) [SND_PCM_FORMAT_##v] = #v
#define SUBFORMAT(v) [SND_PCM_SUBFORMAT_##v] = #v

#define FORMATD(v, d) [SND_PCM_FORMAT_##v] = d
#define SUBFORMATD(v, d) [SND_PCM_SUBFORMAT_##v] = d

static const char *snd_pcm_stream_names[] = {
  STREAM (PLAYBACK),
  STREAM (CAPTURE),
};

static const char *snd_pcm_state_names[] = {
  STATE (OPEN),
  STATE (SETUP),
  STATE (PREPARED),
  STATE (RUNNING),
  STATE (XRUN),
  STATE (DRAINING),
  STATE (PAUSED),
  STATE (SUSPENDED),
  STATE (DISCONNECTED),
};

static const char *snd_pcm_access_names[] = {
  ACCESS (MMAP_INTERLEAVED),
  ACCESS (MMAP_NONINTERLEAVED),
  ACCESS (MMAP_COMPLEX),
  ACCESS (RW_INTERLEAVED),
  ACCESS (RW_NONINTERLEAVED),
};

static const char *snd_pcm_format_descriptions[] = {
  FORMATD (S8, "Signed 8 bit"),
  FORMATD (U8, "Unsigned 8 bit"),
  FORMATD (S16_LE, "Signed 16 bit Little Endian"),
  FORMATD (S16_BE, "Signed 16 bit Big Endian"),
  FORMATD (U16_LE, "Unsigned 16 bit Little Endian"),
  FORMATD (U16_BE, "Unsigned 16 bit Big Endian"),
  FORMATD (S24_LE, "Signed 24 bit Little Endian"),
  FORMATD (S24_BE, "Signed 24 bit Big Endian"),
  FORMATD (U24_LE, "Unsigned 24 bit Little Endian"),
  FORMATD (U24_BE, "Unsigned 24 bit Big Endian"),
  FORMATD (S32_LE, "Signed 32 bit Little Endian"),
  FORMATD (S32_BE, "Signed 32 bit Big Endian"),
  FORMATD (U32_LE, "Unsigned 32 bit Little Endian"),
  FORMATD (U32_BE, "Unsigned 32 bit Big Endian"),
  FORMATD (FLOAT_LE, "Float 32 bit Little Endian"),
  FORMATD (FLOAT_BE, "Float 32 bit Big Endian"),
  FORMATD (FLOAT64_LE, "Float 64 bit Little Endian"),
  FORMATD (FLOAT64_BE, "Float 64 bit Big Endian"),
  FORMATD (IEC958_SUBFRAME_LE, "IEC-958 Little Endian"),
  FORMATD (IEC958_SUBFRAME_BE, "IEC-958 Big Endian"),
  FORMATD (MU_LAW, "Mu-Law"),
  FORMATD (A_LAW, "A-Law"),
  FORMATD (IMA_ADPCM, "Ima-ADPCM"),
  FORMATD (MPEG, "MPEG"),
  FORMATD (GSM, "GSM"),
  FORMATD (SPECIAL, "Special"),
  FORMATD (S24_3LE, "Signed 24 bit Little Endian in 3bytes"),
  FORMATD (S24_3BE, "Signed 24 bit Big Endian in 3bytes"),
  FORMATD (U24_3LE, "Unsigned 24 bit Little Endian in 3bytes"),
  FORMATD (U24_3BE, "Unsigned 24 bit Big Endian in 3bytes"),
  FORMATD (S20_3LE, "Signed 20 bit Little Endian in 3bytes"),
  FORMATD (S20_3BE, "Signed 20 bit Big Endian in 3bytes"),
  FORMATD (U20_3LE, "Unsigned 20 bit Little Endian in 3bytes"),
  FORMATD (U20_3BE, "Unsigned 20 bit Big Endian in 3bytes"),
  FORMATD (S18_3LE, "Signed 18 bit Little Endian in 3bytes"),
  FORMATD (S18_3BE, "Signed 18 bit Big Endian in 3bytes"),
  FORMATD (U18_3LE, "Unsigned 18 bit Little Endian in 3bytes"),
  FORMATD (U18_3BE, "Unsigned 18 bit Big Endian in 3bytes"),
};

struct _snd_pcm
{
  int fd;
  int state;
  oss_audioinfo info;
  int stream, oss_mode;
  int frame_size, sample_size, period_size;
  int channels;
  int periods;
  snd_pcm_channel_area_t *area;
};

struct _snd_pcm_format_mask
{
  unsigned int mask;
};

struct _snd_pcm_status
{
  int dummy;
};

struct _snd_pcm_sw_params
{
  int dummy;
};

struct _snd_pcm_hw_params
{
  int speed;
  int fmt, channels;
  int oss_fmt;

  oss_audioinfo *info;
  snd_pcm_t *pcm;

  int frame_size;
  int sample_bits;
  int access;
};

static int
parse_device (const char *nme, char *dspname)
{
  char *s;
  oss_audioinfo ai;
  int card = 0, port = 0, i;
  char tmp[256], *name = tmp;

  strcpy (name, nme);

  if (name[0] != 'h')
    return 0;
  if (name[1] != 'w')
    return 0;
  if (name[2] != ':')
    return 0;

  name += 3;
  s = name;

  while (*s && *s != ',')
    s++;

  if (*s == ',')
    {
      int n = 0;

      *s++ = 0;
      if (sscanf (name, "%d", &card) != 1)
	return 0;
      if (sscanf (s, "%d", &port) != 1)
	return 0;

      n = -1;
      for (i = 0; i < sysinfo.numaudios; i++)
	{
	  ai.dev = i;
	  if (ioctl (mixer_fd, SNDCTL_AUDIOINFO, &ai) == -1)
	    continue;

	  if (ai.card_number != card)
	    continue;

	  n++;
	  if (port < n)
	    continue;

	  sprintf (dspname, "/dev/dsp%d", i);
	  return 1;
	}
      return 0;
    }
  if (sscanf (name, "%d", &card) != 1)
    return 0;

  for (i = 0; i < sysinfo.numaudios; i++)
    {
      ai.dev = i;
      if (ioctl (mixer_fd, SNDCTL_AUDIOINFO, &ai) == -1)
	continue;

      if (ai.card_number == card)
	{
	  port = i;
	  sprintf (dspname, "/dev/dsp%d", port);
	  return 1;
	}
    }

  return 0;
}

/**
 * \brief Opens a PCM
 * \param pcmp Returned PCM handle
 * \param name ASCII identifier of the PCM handle
 * \param stream Wanted stream
 * \param mode Open mode (see #SND_PCM_NONBLOCK, #SND_PCM_ASYNC)
 * \return 0 on success otherwise a negative error code
 */
int
snd_pcm_open (snd_pcm_t ** pcmp, const char *name,
	      snd_pcm_stream_t stream, int mode)
{
  snd_pcm_t *pcm;
  int oss_mode = O_WRONLY;
  char dspname[32];
  const char *devdsp;

  if ((devdsp = getenv("OSS_AUDIODEV")) == NULL)
     devdsp = "/dev/dsp";

  ALIB_INIT ();

  if (!alib_appcheck ())
    return -ENODEV;

  *pcmp = NULL;

  dbg_printf ("snd_pcm_open(%s, %x)\n", name, stream);

  if (mode != 0)
    dbg_printf2 ("snd_pcm_open: mode=0x%x - not emulated.\n", mode);

  if (strcmp (name, "default") == 0)
    name = devdsp;
  else
    {
      if (!parse_device (name, dspname))
	{
#if 0
	  fprintf (stderr, "OSS asound - Bad PCM device '%s'\n", name);
	  return -ENOENT;
#else
	  strcpy (dspname, devdsp);
#endif
	}

      name = dspname;
    }

  if ((pcm = malloc (sizeof (*pcm))) == NULL)
    return -ENOMEM;

  switch (stream)
    {
    case SND_PCM_STREAM_CAPTURE:
      oss_mode = O_RDONLY;
      break;
    case SND_PCM_STREAM_PLAYBACK:
      oss_mode = O_WRONLY;
      break;
    default:
      fprintf (stderr, "snd_pcm_open: Bad stream %d\n", stream);
      return -EINVAL;
    }

  memset (pcm, 0, sizeof (*pcm));
  pcm->stream = stream;
  pcm->frame_size = 2;
  pcm->sample_size = 1;
  pcm->channels = 2;
  pcm->oss_mode = oss_mode;
  pcm->state = SND_PCM_STATE_OPEN;
  pcm->period_size = PERIOD_SIZE;
  pcm->periods = 4;

  if ((pcm->fd = open (name, oss_mode, 0)) == -1)
    {
      int err = errno;
      free (pcm);
      return -err;
    }

  pcm->info.dev = -1;
  if (ioctl (pcm->fd, SNDCTL_ENGINEINFO, &pcm->info) == -1)
    {
      int err = errno;
      close (pcm->fd);
      free (pcm);
      return -err;
    }

  dbg_printf ("Opened %s='%s' = %d\n", name, pcm->info.name, pcm->fd);
  *pcmp = pcm;
  return 0;
}

/**
 * \brief close PCM handle
 * \param pcm PCM handle
 * \return 0 on success otherwise a negative error code
 *
 * Closes the specified PCM handle and frees all associated
 * resources.
 */
int
snd_pcm_close (snd_pcm_t * pcm)
{
  if (pcm == NULL)
    return -1;
  close (pcm->fd);
  if (pcm->area != NULL)
    free (pcm->area);
  free (pcm);
  return 0;
}

/**
 * \brief Convert frames in bytes for a PCM
 * \param pcm PCM handle
 * \param frames quantity in frames
 * \return quantity expressed in bytes
 */
ssize_t
snd_pcm_frames_to_bytes (snd_pcm_t * pcm, snd_pcm_sframes_t frames)
{
// printf("snd_pcm_frames_to_bytes(%d)\n", frames);
  return frames * pcm->frame_size;
}

/**
 * \brief Convert bytes in frames for a PCM
 * \param pcm PCM handle
 * \param bytes quantity in bytes
 * \return quantity expressed in frames
 */
snd_pcm_sframes_t
snd_pcm_bytes_to_frames (snd_pcm_t * pcm, ssize_t bytes)
{
  dbg_printf3 ("snd_pcm_bytes_to_frames(%d)=%d\n", bytes,
	       bytes / pcm->frame_size);
  return bytes / pcm->frame_size;
}

/**
 * \brief Stop a PCM preserving pending frames
 * \param pcm PCM handle
 * \return 0 on success otherwise a negative error code
 * \retval -ESTRPIPE a suspend event occurred
 *
 * For playback wait for all pending frames to be played and then stop
 * the PCM.
 * For capture stop PCM permitting to retrieve residual frames.
 *
 * For stopping the PCM stream immediately, use \link ::snd_pcm_drop() \endlink
 * instead.
 */
int
snd_pcm_drain (snd_pcm_t * pcm)
{
  dbg_printf2 ("snd_pcm_drain()\n");
  ioctl (pcm->fd, SNDCTL_DSP_SYNC, NULL);
  return 0;
}

/**
 * \brief Stop a PCM dropping pending frames
 * \param pcm PCM handle
 * \return 0 on success otherwise a negative error code
 *
 * This function stops the PCM <i>immediately</i>.
 * The pending samples on the buffer are ignored.
 *
 * For processing all pending samples, use \link ::snd_pcm_drain() \endlink
 * instead.
 */
int
snd_pcm_drop (snd_pcm_t * pcm)
{
  dbg_printf2 ("snd_pcm_drop()\n");
  ioctl (pcm->fd, SNDCTL_DSP_HALT, NULL);
  return 0;
}

/** \brief Install one PCM hardware configuration chosen from a configuration sp
ace and #snd_pcm_prepare it
 * \param pcm PCM handle
 * \param params Configuration space definition container
 * \return 0 on success otherwise a negative error code
 *
 * The configuration is chosen fixing single parameters in this order:
 * first access, first format, first subformat, min channels, min rate,
 * min period time, max buffer size, min tick time
 *
 * After this call, #snd_pcm_prepare() is called automatically and
 * the stream is brought to \c #SND_PCM_STATE_PREPARED state.
 */
int
snd_pcm_hw_params (snd_pcm_t * pcm, snd_pcm_hw_params_t * params)
{
  int tmp;

  dbg_printf ("snd_pcm_hw_params(%d, %d ch, %x)\n",
	      params->speed, params->channels, params->oss_fmt);

  tmp = params->oss_fmt;
  if (ioctl (pcm->fd, SNDCTL_DSP_SETFMT, &tmp) == -1)
    return -errno;
  if (tmp != params->oss_fmt)
    return -EINVAL;

  tmp = params->channels;
  if (ioctl (pcm->fd, SNDCTL_DSP_CHANNELS, &tmp) == -1)
    return -errno;
  if (tmp != params->channels)
    return -EINVAL;
  pcm->channels = tmp;

  tmp = params->speed;
  if (ioctl (pcm->fd, SNDCTL_DSP_SPEED, &tmp) == -1)
    return -errno;
  if (tmp != params->speed)
    return -EINVAL;

  tmp = params->channels * params->sample_bits;
  pcm->frame_size = params->frame_size = tmp / 8;
  pcm->sample_size = pcm->frame_size / pcm->channels;
  dbg_printf ("Frame size = %d\n", params->frame_size);
  return 0;
}


/**
 * \brief Fill params with a full configuration space for a PCM
 * \param pcm PCM handle
 * \param params Configuration space
 */
int
snd_pcm_hw_params_any (snd_pcm_t * pcm, snd_pcm_hw_params_t * params)
{
  dbg_printf ("snd_pcm_hw_params_any()\n");

  memset (params, 0, sizeof (params));
  if (ioctl (pcm->fd, SNDCTL_DSP_CHANNELS, &params->channels) == -1)
    return -errno;
  pcm->channels = params->channels;
  if (ioctl (pcm->fd, SNDCTL_DSP_SETFMT, &params->oss_fmt) == -1)
    return -errno;
  if (ioctl (pcm->fd, SNDCTL_DSP_SPEED, &params->speed) == -1)
    return -errno;
  dbg_printf ("Rate %d, channels %d, fmt %x\n", params->speed,
	      params->channels, params->oss_fmt);

// TODO
  params->frame_size = 2 * params->channels;
  params->sample_bits = 16;

  params->info = &pcm->info;
  params->pcm = pcm;
  return 0;
}

/**
 * \brief Restrict a configuration space to contain only one access type
 * \param pcm PCM handle
 * \param params Configuration space
 * \param access access type
 * \return 0 otherwise a negative error code if configuration space would become empty
 */
int
snd_pcm_hw_params_set_access (snd_pcm_t * pcm, snd_pcm_hw_params_t * params,
			      snd_pcm_access_t access)
{
  dbg_printf2 ("snd_pcm_hw_params_set_access(%x/%s)\n", access,
	       snd_pcm_access_names[access]);
  params->access = access;
  if (access == SND_PCM_ACCESS_RW_INTERLEAVED)
    return 0;
  if (access == SND_PCM_ACCESS_MMAP_INTERLEAVED)
    return 0;
  return -EINVAL;
}

/**
 * \brief Restrict a configuration space to contain only one buffer size
 * \param pcm PCM handle
 * \param params Configuration space
 * \param val buffer size in frames
 * \return 0 otherwise a negative error code if configuration space would become empty
 *
 * Wanted exact value is <,=,> val following dir (-1,0,1)
 */
int
snd_pcm_hw_params_set_buffer_size (snd_pcm_t * pcm,
				   snd_pcm_hw_params_t * params,
				   snd_pcm_uframes_t val)
{
  dbg_printf ("snd_pcm_hw_params_set_buffer_size()\n");

  return 0;
}

int
snd_pcm_hw_params_set_buffer_size_min (snd_pcm_t * pcm,
				       snd_pcm_hw_params_t * params,
				       snd_pcm_uframes_t * val)
{
  dbg_printf ("snd_pcm_hw_params_set_buffer_size_min()\n");

  return 0;
}

/**
 * \brief Restrict a configuration space to have buffer size nearest to a target
 * \param pcm PCM handle
 * \param params Configuration space
 * \param val approximate target buffer size in frames / returned chosen approxi
mate target buffer size in frames
 * \return 0 otherwise a negative error code if configuration space is empty
 *
 * target/chosen exact value is <,=,> val following dir (-1,0,1)
 */
int
__snd_pcm_hw_params_set_buffer_size_near (snd_pcm_t * pcm,
					  snd_pcm_hw_params_t * params,
					  snd_pcm_uframes_t * val)
{
  dbg_printf ("snd_pcm_hw_params_set_buffer_size_near(%d)\n", *val);
  return 0;
}

/**
 * \brief Restrict a configuration space to contain only one channels count
 * \param pcm PCM handle
 * \param params Configuration space
 * \param val channels count
 * \return 0 otherwise a negative error code if configuration space would become empty
 */
int
snd_pcm_hw_params_set_channels (snd_pcm_t * pcm, snd_pcm_hw_params_t * params,
				unsigned int val)
{
  dbg_printf2 ("snd_pcm_hw_params_set_channels(%d)\n", val);

  params->channels = val;

  if (ioctl (pcm->fd, SNDCTL_DSP_CHANNELS, &params->channels) < 0)
    return -errno;

  if (params->channels != val)
    return -EINVAL;
  pcm->channels = params->channels;
  return 0;
}

int
__snd_pcm_hw_params_set_channels_near (snd_pcm_t * pcm,
				       snd_pcm_hw_params_t * params,
				       unsigned int *val)
{
  dbg_printf2 ("snd_pcm_hw_params_set_channels_near(%d)\n", *val);

  params->channels = *val;

  if (ioctl (pcm->fd, SNDCTL_DSP_CHANNELS, &params->channels) < 0)
    return -errno;

  *val = params->channels;
  pcm->channels = params->channels;
  return 0;
}

/**
 * \brief Restrict a configuration space to contain only one format
 * \param pcm PCM handle
 * \param params Configuration space
 * \param format format
 * \return 0 otherwise a negative error code
 */
int
snd_pcm_hw_params_set_format (snd_pcm_t * pcm, snd_pcm_hw_params_t * params,
			      snd_pcm_format_t format)
{
  int err = -EINVAL;
  int tmp;

  dbg_printf2 ("snd_pcm_hw_params_set_format(%d)\n", format);
  if (format == SND_PCM_FORMAT_U8)
    {
      params->fmt = format;
      params->oss_fmt = AFMT_U8;
      params->sample_bits = 8;
      err = 0;
    }

  if (format == SND_PCM_FORMAT_S16_LE)
    {
      params->fmt = format;
      params->oss_fmt = AFMT_S16_LE;
      params->sample_bits = 16;
      err = 0;
    }

  if (err < 0)
    return err;

  tmp = params->oss_fmt;
  if (ioctl (pcm->fd, SNDCTL_DSP_SETFMT, &tmp) < 0)
    return -errno;

  if (tmp != params->oss_fmt)
    return -EINVAL;

  return 0;
}

/**
 * \brief Extract format from a configuration space
 * \param params Configuration space
 * \param format returned format
 * \return format otherwise a negative error code if not exactly one is present
 */
int INTERNAL (snd_pcm_hw_params_get_format) (const snd_pcm_hw_params_t *
					     params,
					     snd_pcm_format_t * format)
{
  dbg_printf ("snd_pcm_hw_params_get_format(params=%x)\n", params);

  *format = params->fmt;
}

/**
 * \brief Verify if a format is available inside a configuration space for a PCM * \param pcm PCM handle
 * \param params Configuration space
 * \param format format
 * \return 0 if available a negative error code otherwise
 */
int
snd_pcm_hw_params_test_format (snd_pcm_t * pcm, snd_pcm_hw_params_t * params,
			       snd_pcm_format_t format)
{
  dbg_printf2 ("snd_pcm_hw_params_test_format()\n");

  if (format == SND_PCM_FORMAT_U8)
    return 0;
  if (format == SND_PCM_FORMAT_S16_LE)
    return 0;

  return -EINVAL;
}

/**
 * \brief Restrict a configuration space to contain only one periods count
 * \param pcm PCM handle
 * \param params Configuration space
 * \param val approximate periods per buffer
 * \param dir Sub unit direction
 * \return 0 otherwise a negative error code if configuration space would become empty
 *
 * Wanted exact value is <,=,> val following dir (-1,0,1)
 */
int
snd_pcm_hw_params_set_periods (snd_pcm_t * pcm, snd_pcm_hw_params_t * params,
			       unsigned int val, int dir)
{
  dbg_printf
    ("snd_pcm_hw_params_set_periods(pcm=%x, params=%x, val=%d, dir=%d)\n",
     pcm, params, val, dir);
  pcm->periods = val;

  return 0;
}

/**
 * \brief Restrict a configuration space to contain only integer periods counts
 * \param pcm PCM handle
 * \param params Configuration space
 * \return 0 otherwise a negative error code if configuration space would become empty
 */
int
snd_pcm_hw_params_set_periods_integer (snd_pcm_t * pcm,
				       snd_pcm_hw_params_t * params)
{
  dbg_printf ("snd_pcm_hw_params_set_periods_integer()\n");
  return 0;
}

/**                                                                              * \brief Restrict a configuration space with a minimum periods count
 * \param pcm PCM handle                                                         * \param params Configuration space
 * \param val approximate minimum periods per buffer (on return filled with actual minimum)
 * \param dir Sub unit direction (on return filled with actual direction)        * \return 0 otherwise a negative error code if configuration space would become
 empty                                                                           *
 * Wanted/actual exact minimum is <,=,> val following dir (-1,0,1)               */
int
snd_pcm_hw_params_set_periods_min (snd_pcm_t * pcm,
				   snd_pcm_hw_params_t * params,
				   unsigned int *val, int *dir)
{
  dbg_printf ("snd_pcm_hw_params_set_periods_min(%x, %x)\n", val, dir);
  return 0;
}

/**
 * \brief Restrict a configuration space with a maximum periods count
 * \param pcm PCM handle
 * \param params Configuration space
 * \param val approximate maximum periods per buffer (on return filled with actual maximum)
 * \param dir Sub unit direction (on return filled with actual direction)
 * \return 0 otherwise a negative error code if configuration space would become empty
 *
 * Wanted/actual exact maximum is <,=,> val following dir (-1,0,1)
 */
int
snd_pcm_hw_params_set_periods_max (snd_pcm_t * pcm,
				   snd_pcm_hw_params_t * params,
				   unsigned int *val, int *dir)
{
  dbg_printf ("snd_pcm_hw_params_set_periods_max(%x, %x)\n", val, dir);
  return 0;
  return -EINVAL;
}

/**
 * \brief Restrict a configuration space to contain only one rate
 * \param pcm PCM handle
 * \param params Configuration space
 * \param val approximate rate
 * \param dir Sub unit direction
 * \return 0 otherwise a negative error code if configuration space would become empty
 *
 * Wanted exact value is <,=,> val following dir (-1,0,1)
 */
int
snd_pcm_hw_params_set_rate (snd_pcm_t * pcm, snd_pcm_hw_params_t * params,
			    unsigned int val, int dir)
{
  dbg_printf2 ("snd_pcm_hw_params_set_rate(%d, %d)\n", val, dir);

  params->speed = val;

  if (ioctl (pcm->fd, SNDCTL_DSP_SPEED, &params->speed) < 0)
    return -errno;

  if (val != params->speed)
    return -EINVAL;

  return 0;
}

/**
 * \brief Extract rate from a configuration space
 * \param params Configuration space
 * \param val Returned approximate rate
 * \param dir Sub unit direction
 * \return 0 otherwise a negative error code if not exactly one is present
 *
 * Actual exact value is <,=,> the approximate one following dir (-1, 0, 1)
 */
int INTERNAL (snd_pcm_hw_params_get_rate) (const snd_pcm_hw_params_t * params,
					   unsigned int *val, int *dir)
{
  dbg_printf ("snd_pcm_hw_params_get_rate(params=%x)=%d\n",
	      params, params->speed);

  *val = params->speed;
  *dir = 0;
  return 0;
}

/**
 * \brief Restrict a configuration space to have rate nearest to a target
 * \param pcm PCM handle
 * \param params Configuration space
 * \param val approximate target rate / returned approximate set rate
 * \return 0 otherwise a negative error code if configuration space is empty
 *
 * target/chosen exact value is <,=,> val following dir (-1,0,1)
 */
int
__snd_pcm_hw_params_set_rate_near (snd_pcm_t * pcm,
				   snd_pcm_hw_params_t * params,
				   unsigned int *val, int *dir)
{
  dbg_printf2 ("snd_pcm_hw_params_set_rate_near(%d)\n", *val);

  params->speed = *val;

  if (ioctl (pcm->fd, SNDCTL_DSP_SPEED, &params->speed) < 0)
    return -errno;

  *val = params->speed;
  return 0;
}

/**
 * \brief get size of #snd_pcm_hw_params_t
 * \return size in bytes
 */
size_t
snd_pcm_hw_params_sizeof ()
{
  return sizeof (snd_pcm_hw_params_t);
}

/**
 * \brief set nonblock mode
 * \param pcm PCM handle
 * \param nonblock 0 = block, 1 = nonblock mode
 * \return 0 on success otherwise a negative error code
 */
int
snd_pcm_nonblock (snd_pcm_t * pcm, int nonblock)
{
  dbg_printf2 ("snd_pcm_nonblock(%d)\n", nonblock);

  if (nonblock == 0)
    return 0;

  return -EINVAL;
}

/**
 * \brief Prepare PCM for use
 * \param pcm PCM handle
 * \return 0 on success otherwise a negative error code
 */
int
snd_pcm_prepare (snd_pcm_t * pcm)
{
  dbg_printf2 ("snd_pcm_prepare()\n");
  return 0;
}

/**
 * \brief Read interleaved frames from a PCM
 * \param pcm PCM handle
 * \param buffer frames containing buffer
 * \param size frames to be written
 * \return a positive number of frames actually read otherwise a
 * negative error code
 * \retval -EBADFD PCM is not in the right state (#SND_PCM_STATE_PREPARED or #SN
D_PCM_STATE_RUNNING)
 * \retval -EPIPE an overrun occurred
 * \retval -ESTRPIPE a suspend event occurred (stream is suspended and waiting f
or an application recovery)
 *
 * If the blocking behaviour was selected, then routine waits until
 * all requested bytes are filled. The count of bytes can be less only
 * if a signal or underrun occurred.
 *
 * If the non-blocking behaviour is selected, then routine doesn't wait at all.
 */
snd_pcm_sframes_t
snd_pcm_readi (snd_pcm_t * pcm, void *buffer, snd_pcm_uframes_t size)
{
  int l;

  dbg_printf3 ("snd_pcm_readi(%d)\n", pcm->fd);

  pcm->state = SND_PCM_STATE_RUNNING;

  l = size * pcm->frame_size;

  if ((l = read (pcm->fd, buffer, l)) == -1)
    return -errno;
  return l / pcm->frame_size;
}

/** \brief Remove PCM hardware configuration and free associated resources
 * \param pcm PCM handle
 * \return 0 on success otherwise a negative error code
 */
int
snd_pcm_hw_free (snd_pcm_t * pcm)
{
  dbg_printf ("snd_pcm_hw_free()\n");

  return 0;
}

/**
 * \brief Remove a PCM from a linked group
 * \param pcm PCM handle
 * \return 0 on success otherwise a negative error code
 */
int
snd_pcm_unlink (snd_pcm_t * pcm)
{
  dbg_printf ("snd_pcm_unlink()\n");

  return 0;
}

/**
 * \brief Extract tick time from a configuration space
 * \param params Configuration space
 * \param val Returned approximate tick duration in us
 * \param dir Sub unit direction
 * \return 0 otherwise a negative error code if not exactly one is present
 *
 * Actual exact value is <,=,> the approximate one following dir (-1, 0, 1)
 */
int INTERNAL (snd_pcm_hw_params_get_tick_time) (const snd_pcm_hw_params_t *
						params, unsigned int *val,
						int *dir)
{
  dbg_printf ("snd_pcm_hw_params_get_tick_time()\n");

  *val = 1;
  *dir = 0;
  return 0;
}

/**
 * \brief Write interleaved frames to a PCM
 * \param pcm PCM handle
 * \param buffer frames containing buffer
 * \param size frames to be written
 * \return a positive number of frames actually written otherwise a
 * negative error code
 * \retval -EBADFD PCM is not in the right state (#SND_PCM_STATE_PREPARED or #SN
D_PCM_STATE_RUNNING)
 * \retval -EPIPE an underrun occurred
 * \retval -ESTRPIPE a suspend event occurred (stream is suspended and waiting f
or an application recovery)
 *
 * If the blocking behaviour is selected, then routine waits until
 * all requested bytes are played or put to the playback ring buffer.
 * The count of bytes can be less only if a signal or underrun occurred.
 *
 * If the non-blocking behaviour is selected, then routine doesn't wait at all.
 */
snd_pcm_sframes_t
snd_pcm_writei (snd_pcm_t * pcm, const void *buffer, snd_pcm_uframes_t size)
{
  int l;

  dbg_printf3 ("snd_pcm_writei(%d)\n", size);

  pcm->state = SND_PCM_STATE_RUNNING;

  l = size * pcm->frame_size;

  if ((l = write (pcm->fd, buffer, l)) == -1)
    return -errno;
  return l / pcm->frame_size;
}

/**
 * \brief Resume from suspend, no samples are lost
 * \param pcm PCM handle
 * \return 0 on success otherwise a negative error code
 * \retval -EAGAIN resume can't be proceed immediately (audio hardware is probab
ly still suspended)
 * \retval -ENOSYS hardware doesn't support this feature
 *
 * This function can be used when the stream is in the suspend state
 * to do the fine resume from this state. Not all hardware supports
 * this feature, when an -ENOSYS error is returned, use the \link ::snd_pcm_prep
are() \endlink
 * function to recovery.
 */
int
snd_pcm_resume (snd_pcm_t * pcm)
{
  dbg_printf ("snd_pcm_resume()\n");
  return -EINVAL;
}

/**
 * \brief allocate an invalid #snd_pcm_status_t using standard malloc
 * \param ptr returned pointer
 * \return 0 on success otherwise negative error code
 */
int
snd_pcm_status_malloc (snd_pcm_status_t ** ptr)
{
  assert (ptr);
  *ptr = calloc (1, sizeof (snd_pcm_status_t));
  if (!*ptr)
    return -ENOMEM;
  return 0;
}

/**
 * \brief frees a previously allocated #snd_pcm_status_t
 * \param pointer to object to free
 */
void
snd_pcm_status_free (snd_pcm_status_t * obj)
{
  free (obj);
}


/**
 * \brief Obtain status (runtime) information for PCM handle
 * \param pcm PCM handle
 * \param status Status container
 * \return 0 on success otherwise a negative error code
 */
int
snd_pcm_status (snd_pcm_t * pcm, snd_pcm_status_t * status)
{
  dbg_printf ("snd_pcm_status()\n");
  return -EINVAL;
}

/**
 * \brief Get state from a PCM status container (see #snd_pcm_state)
 * \return PCM state
 */
snd_pcm_state_t
snd_pcm_status_get_state (const snd_pcm_status_t * obj)
{
  dbg_printf ("snd_pcm_status_get_state()\n");
  return -EINVAL;
}

/**
 * \brief Get number of frames available from a PCM status container (see #snd_pcm_avail_update)
 * \return Number of frames ready to be read/written
 */
snd_pcm_uframes_t
snd_pcm_status_get_avail (const snd_pcm_status_t * obj)
{
  dbg_printf ("snd_pcm_status_get_avail()\n");

  return 1;
}

/**
 * \brief get size of #snd_pcm_status_t
 * \return size in bytes
 */
size_t
snd_pcm_status_sizeof ()
{
  return sizeof (snd_pcm_status_t);
}

/**
 * \brief Dump PCM info
 * \param pcm PCM handle
 * \param out Output handle
 * \return 0 on success otherwise a negative error code
 */
int
snd_pcm_dump (snd_pcm_t * pcm, snd_output_t * out)
{
  dbg_printf ("snd_pcm_dump()\n");
  return 0;
}

void
snd_pcm_hw_params_copy (snd_pcm_hw_params_t * dst,
			const snd_pcm_hw_params_t * src)
{
  memcpy (dst, src, sizeof (*dst));
}

void
snd_pcm_hw_params_free (snd_pcm_hw_params_t * p)
{
  free (p);
}

int
snd_pcm_hw_params_can_mmap_sample_resolution (const snd_pcm_hw_params_t *
					      params)
{
  return 0;
}

int
snd_pcm_hw_params_can_overrange (const snd_pcm_hw_params_t * params)
{
  return 0;
}

int
snd_pcm_hw_params_can_pause (const snd_pcm_hw_params_t * params)
{
  return 0;
}

int
snd_pcm_hw_params_can_resume (const snd_pcm_hw_params_t * params)
{
  return 0;
}

int
snd_pcm_hw_params_can_sync_start (const snd_pcm_hw_params_t * params)
{
  return 1;
}

int
snd_pcm_hw_params_get_channels (const snd_pcm_hw_params_t * params,
				unsigned int *val)
{
  *val = params->channels;
  return 0;
}

int INTERNAL (snd_pcm_hw_params_get_channels_min) (const snd_pcm_hw_params_t *
						   params, unsigned int *val)
{
  *val = params->info->min_channels;
  return 0;
}

int INTERNAL (snd_pcm_hw_params_get_channels_max) (const snd_pcm_hw_params_t *
						   params, unsigned int *val)
{
  *val = params->info->max_channels;
  return 0;
}

int INTERNAL (snd_pcm_hw_params_get_rate_min) (const snd_pcm_hw_params_t *
					       params, unsigned int *val,
					       int *dir)
{
  *val = params->info->min_rate;
  return 0;
}

int INTERNAL (snd_pcm_hw_params_get_rate_max) (const snd_pcm_hw_params_t *
					       params, unsigned int *val,
					       int *dir)
{
  *val = params->info->max_rate;
  return 0;
}

// int snd_pcm_hw_params_get_(const snd_pcm_hw_params_t *params, unsigned int *val) { return 0;}

/**
 * \brief get name of PCM sample format
 * \param format PCM sample format
 * \return ascii name of PCM sample format
 */
const char *
snd_pcm_format_name (snd_pcm_format_t format)
{
  if (format > SND_PCM_FORMAT_LAST)
    return NULL;
  return snd_pcm_format_names[format];
}

/**
 * \brief Return PCM state
 * \param pcm PCM handle
 * \return PCM state #snd_pcm_state_t of given PCM handle
 *
 * This is a faster way to obtain only the PCM state without calling
 * \link ::snd_pcm_status() \endlink.
 */
snd_pcm_state_t
snd_pcm_state (snd_pcm_t * pcm)
{
  dbg_printf3 ("snd_pcm_state()\n");
  return pcm->state;
}

/**
 * \brief allocate an invalid #snd_pcm_info_t using standard malloc
 * \param ptr returned pointer
 * \return 0 on success otherwise negative error code
 */
int
snd_pcm_info_malloc (snd_pcm_info_t ** ptr)
{
  assert (ptr);
  *ptr = calloc (1, sizeof (snd_pcm_info_t));
  if (!*ptr)
    return -ENOMEM;
  return 0;
}

/**
 * \brief frees a previously allocated #snd_pcm_info_t
 * \param pointer to object to free
 */
void
snd_pcm_info_free (snd_pcm_info_t * obj)
{
  free (obj);
}


/**
 * \brief Obtain general (static) information for PCM handle
 * \param pcm PCM handle
 * \param info Information container
 * \return 0 on success otherwise a negative error code
 */
int
snd_pcm_info (snd_pcm_t * pcm, snd_pcm_info_t * info)
{
  dbg_printf ("snd_pcm_info()\n");
  memset (info, 0, sizeof (*info));
  info->ainfo = &pcm->info;

  return 0;
}

/**
 * \brief Get card from a PCM info container
 * \param obj PCM info container
 * \return card number otherwise a negative error code if not associable to a card
 */
int
snd_pcm_info_get_card (const snd_pcm_info_t * obj)
{
  dbg_printf ("snd_pcm_info_get_card()\n");
  return obj->ainfo->card_number;
}

/**
 * \brief Get device from a PCM info container
 * \param obj PCM info container
 * \return device number
 */
unsigned int
snd_pcm_info_get_device (const snd_pcm_info_t * obj)
{
  dbg_printf ("snd_pcm_info_get_device()\n");
  return obj->ainfo->dev;
}

/**
 * \brief Get name from a PCM info container
 * \param obj PCM info container
 * \return name of PCM
 */
const char *
snd_pcm_info_get_name (const snd_pcm_info_t * obj)
{
  dbg_printf ("snd_pcm_info_get_name()\n");

  return obj->ainfo->name;
}

/**
 * \brief Get subdevice from a PCM info container
 * \param obj PCM info container
 * \return subdevice number
 */
unsigned int
snd_pcm_info_get_subdevice (const snd_pcm_info_t * obj)
{
  dbg_printf ("snd_pcm_info_get_subdevice()\n");
  return 0;
}

/**
 * \brief Set wanted device inside a PCM info container (see #snd_ctl_pcm_info)
 * \param obj PCM info container
 * \param val Device number
 */
void
snd_pcm_info_set_device (snd_pcm_info_t * obj, unsigned int val)
{
  dbg_printf ("snd_pcm_info_set_device()\n");
}

/**
 * \brief Set wanted subdevice inside a PCM info container (see #snd_ctl_pcm_info)
 * \param obj PCM info container
 * \param val Subdevice number
 */
void
snd_pcm_info_set_subdevice (snd_pcm_info_t * obj, unsigned int val)
{
  dbg_printf ("snd_pcm_info_set_subdevice()\n");
}

void
snd_pcm_info_set_stream (snd_pcm_info_t * obj, snd_pcm_stream_t val)
{
  dbg_printf ("snd_pcm_info_set_stream()\n");
}

/**
 * \brief get size of #snd_pcm_info_t
 * \return size in bytes
 */
size_t
snd_pcm_info_sizeof ()
{
  return sizeof (snd_pcm_info_t);
}


/**
 * \brief Return number of frames ready to be read/written
 * \param pcm PCM handle
 * \return a positive number of frames ready otherwise a negative
 * error code
 *
 * On capture does all the actions needed to transport to application
 * level all the ready frames across underlying layers.
 *
 * Using of this function is useless for the standard read/write
 * operations. Use it only for mmap access. See to #snd_pcm_delay.
 */
snd_pcm_sframes_t
snd_pcm_avail_update (snd_pcm_t * pcm)
{
  audio_buf_info bi;

  if (pcm->stream == SND_PCM_STREAM_PLAYBACK)
    {
      if (ioctl (pcm->fd, SNDCTL_DSP_GETOSPACE, &bi) == -1)
	return -errno;
    }
  else
    {
      if (ioctl (pcm->fd, SNDCTL_DSP_GETISPACE, &bi) == -1)
	return -errno;
    }

  dbg_printf3 ("snd_pcm_avail_update(pcm=%x)=%d\n", pcm,
	       bi.bytes / pcm->frame_size);
  return bi.bytes / pcm->frame_size;
}

/**
 * \brief Obtain delay for a running PCM handle
 * \param pcm PCM handle
 * \param delayp Returned delay in frames
 * \return 0 on success otherwise a negative error code
 *
 * Delay is distance between current application frame position and
 * sound frame position.
 * It's positive and less than buffer size in normal situation,
 * negative on playback underrun and greater than buffer size on
 * capture overrun.
 *
 * Note this function does not update the actual r/w pointer
 * for applications. The function #snd_pcm_avail_update()
 * have to be called before any begin+commit operation.
 */
int
snd_pcm_delay (snd_pcm_t * pcm, snd_pcm_sframes_t * delayp)
{
  int tmp;

  if (ioctl (pcm->fd, SNDCTL_DSP_GETODELAY, &tmp) < 0)
    return -errno;

  *delayp = tmp / pcm->frame_size;
  return 0;
}

/**
 * \brief Return bits needed to store a PCM sample
 * \param format Sample format
 * \return bits per sample, a negative error code if not applicable
 */
int
snd_pcm_format_physical_width (snd_pcm_format_t format)
{
  switch (format)
    {
    case SNDRV_PCM_FORMAT_S8:
    case SNDRV_PCM_FORMAT_U8:
      return 8;
    case SNDRV_PCM_FORMAT_S16_LE:
    case SNDRV_PCM_FORMAT_S16_BE:
    case SNDRV_PCM_FORMAT_U16_LE:
    case SNDRV_PCM_FORMAT_U16_BE:
      return 16;
    case SNDRV_PCM_FORMAT_S18_3LE:
    case SNDRV_PCM_FORMAT_S18_3BE:
    case SNDRV_PCM_FORMAT_U18_3LE:
    case SNDRV_PCM_FORMAT_U18_3BE:
    case SNDRV_PCM_FORMAT_S20_3LE:
    case SNDRV_PCM_FORMAT_S20_3BE:
    case SNDRV_PCM_FORMAT_U20_3LE:
    case SNDRV_PCM_FORMAT_U20_3BE:
    case SNDRV_PCM_FORMAT_S24_3LE:
    case SNDRV_PCM_FORMAT_S24_3BE:
    case SNDRV_PCM_FORMAT_U24_3LE:
    case SNDRV_PCM_FORMAT_U24_3BE:
      return 24;
    case SNDRV_PCM_FORMAT_S24_LE:
    case SNDRV_PCM_FORMAT_S24_BE:
    case SNDRV_PCM_FORMAT_U24_LE:
    case SNDRV_PCM_FORMAT_U24_BE:
    case SNDRV_PCM_FORMAT_S32_LE:
    case SNDRV_PCM_FORMAT_S32_BE:
    case SNDRV_PCM_FORMAT_U32_LE:
    case SNDRV_PCM_FORMAT_U32_BE:
    case SNDRV_PCM_FORMAT_FLOAT_LE:
    case SNDRV_PCM_FORMAT_FLOAT_BE:
    case SNDRV_PCM_FORMAT_IEC958_SUBFRAME_LE:
    case SNDRV_PCM_FORMAT_IEC958_SUBFRAME_BE:
      return 32;
    case SNDRV_PCM_FORMAT_FLOAT64_LE:
    case SNDRV_PCM_FORMAT_FLOAT64_BE:
      return 64;
    case SNDRV_PCM_FORMAT_MU_LAW:
    case SNDRV_PCM_FORMAT_A_LAW:
      return 8;
    case SNDRV_PCM_FORMAT_IMA_ADPCM:
      return 4;
    default:
      return -EINVAL;
    }
}

/**
 * \brief Return bytes needed to store a quantity of PCM sample
 * \param format Sample format
 * \param samples Samples count
 * \return bytes needed, a negative error code if not integer or unknown
 */
ssize_t
snd_pcm_format_size (snd_pcm_format_t format, size_t samples)
{
  switch (format)
    {
    case SNDRV_PCM_FORMAT_S8:
    case SNDRV_PCM_FORMAT_U8:
      return samples;
    case SNDRV_PCM_FORMAT_S16_LE:
    case SNDRV_PCM_FORMAT_S16_BE:
    case SNDRV_PCM_FORMAT_U16_LE:
    case SNDRV_PCM_FORMAT_U16_BE:
      return samples * 2;
    case SNDRV_PCM_FORMAT_S18_3LE:
    case SNDRV_PCM_FORMAT_S18_3BE:
    case SNDRV_PCM_FORMAT_U18_3LE:
    case SNDRV_PCM_FORMAT_U18_3BE:
    case SNDRV_PCM_FORMAT_S20_3LE:
    case SNDRV_PCM_FORMAT_S20_3BE:
    case SNDRV_PCM_FORMAT_U20_3LE:
    case SNDRV_PCM_FORMAT_U20_3BE:
    case SNDRV_PCM_FORMAT_S24_3LE:
    case SNDRV_PCM_FORMAT_S24_3BE:
    case SNDRV_PCM_FORMAT_U24_3LE:
    case SNDRV_PCM_FORMAT_U24_3BE:
      return samples * 3;
    case SNDRV_PCM_FORMAT_S24_LE:
    case SNDRV_PCM_FORMAT_S24_BE:
    case SNDRV_PCM_FORMAT_U24_LE:
    case SNDRV_PCM_FORMAT_U24_BE:
    case SNDRV_PCM_FORMAT_S32_LE:
    case SNDRV_PCM_FORMAT_S32_BE:
    case SNDRV_PCM_FORMAT_U32_LE:
    case SNDRV_PCM_FORMAT_U32_BE:
    case SNDRV_PCM_FORMAT_FLOAT_LE:
    case SNDRV_PCM_FORMAT_FLOAT_BE:
      return samples * 4;
    case SNDRV_PCM_FORMAT_FLOAT64_LE:
    case SNDRV_PCM_FORMAT_FLOAT64_BE:
      return samples * 8;
    case SNDRV_PCM_FORMAT_IEC958_SUBFRAME_LE:
    case SNDRV_PCM_FORMAT_IEC958_SUBFRAME_BE:
      return samples * 4;
    case SNDRV_PCM_FORMAT_MU_LAW:
    case SNDRV_PCM_FORMAT_A_LAW:
      return samples;
    case SNDRV_PCM_FORMAT_IMA_ADPCM:
      if (samples & 1)
	return -EINVAL;
      return samples / 2;
    default:
      assert (0);
      return -EINVAL;
    }
}

/**
 * \brief Dump a PCM hardware configuration space
 * \param params Configuration space
 * \param out Output handle
 * \return 0 on success otherwise a negative error code
 */
int
snd_pcm_hw_params_dump (snd_pcm_hw_params_t * params, snd_output_t * out)
{
  return 0;
}

/**
 * \brief Extract buffer size from a configuration space
 * \param params Configuration space
 * \param val Returned buffer size in frames
 * \return 0 otherwise a negative error code if not exactly one is present
 */
int
__snd_pcm_hw_params_get_buffer_size (const snd_pcm_hw_params_t * params,
				     snd_pcm_uframes_t * val)
{
  *val = 65536;

  return 0;
}

int
snd_pcm_hw_params_get_buffer_size_min (const snd_pcm_hw_params_t * params,
				       snd_pcm_uframes_t * val)
{
  *val = 65536;

  return 0;
}

int
snd_pcm_hw_params_get_buffer_size_max (const snd_pcm_hw_params_t * params,
				       snd_pcm_uframes_t * val)
{
  *val = 65536;

  return 0;
}

/**
 * \brief Extract access type from a configuration space
 * \param params Configuration space
 * \param access Returned value
 * \return access type otherwise a negative error code if not exactly one is present
 */
int INTERNAL (snd_pcm_hw_params_get_access) (const snd_pcm_hw_params_t *
					     params,
					     snd_pcm_access_t * access)
{
  dbg_printf ("snd_pcm_hw_params_get_access()\n"), *access = params->access;

  return 0;
}

/**
 * \brief Extract period size from a configuration space
 * \param params Configuration space
 * \param val Returned approximate period size in frames
 * \param dir Sub unit direction
 * \return 0 otherwise a negative error code if not exactly one is present
 *
 * Actual exact value is <,=,> the approximate one following dir (-1, 0, 1)
 */
int
__snd_pcm_hw_params_get_period_size (const snd_pcm_hw_params_t * params,
				     snd_pcm_uframes_t * val, int *dir)
{
  dbg_printf ("snd_pcm_hw_params_get_period_size(params=%x)=%d\n",
	      params, params->pcm->period_size);
  *val = params->pcm->period_size;
  if (dir) *dir = 0;
  return 0;
}

int
snd_pcm_hw_params_get_period_size_min (const snd_pcm_hw_params_t * params,
				       snd_pcm_uframes_t * val, int *dir)
{
  dbg_printf ("snd_pcm_hw_params_get_period_size_min(params=%x)=%d\n",
	      params, params->pcm->period_size);
  *val = params->pcm->period_size;
  if (dir) *dir = 0;
  return 0;
}

int
snd_pcm_hw_params_get_period_size_max (const snd_pcm_hw_params_t * params,
				       snd_pcm_uframes_t * val, int *dir)
{
  dbg_printf ("snd_pcm_hw_params_get_period_size_min(params=%x)=%d\n",
	      params, params->pcm->period_size);
  *val = params->pcm->period_size;
  if (dir) *dir = 0;
  return 0;
}

/**
 * \brief Extract periods from a configuration space
 * \param params Configuration space
 * \param val approximate periods per buffer
 * \param dir Sub unit direction
 * \return 0 otherwise a negative error code if not exactly one is present
 *
 * Actual exact value is <,=,> the approximate one following dir (-1, 0, 1)
 */
int INTERNAL (snd_pcm_hw_params_get_periods) (const snd_pcm_hw_params_t *
					      params, unsigned int *val,
					      int *dir)
{
  dbg_printf ("snd_pcm_hw_params_get_periods(params=%x)=%d\n",
	      params, params->pcm->periods);

  *val = params->pcm->periods;

  return 0;
}

/**
 * \brief Restrict a configuration space to have period size nearest to a target * \param pcm PCM handle
 * \param params Configuration space
 * \param val approximate target period size in frames / returned chosen approximate target period size
 * \return 0 otherwise a negative error code if configuration space is empty
 *
 * target/chosen exact value is <,=,> val following dir (-1,0,1)
 */
int
__snd_pcm_hw_params_set_period_size_near (snd_pcm_t * pcm,
					  snd_pcm_hw_params_t * params,
					  snd_pcm_uframes_t * val, int *dir)
{
  dbg_printf
    ("snd_pcm_hw_params_set_period_size_near(pcm=%x, params=%x, val=%d, dir=%d)\n",
     pcm, params, *val, dir?*dir:-1);
  pcm->period_size = *val;
  return 0;
}

/**
 * \brief Restrict a configuration space to have buffer time nearest to a target * \param pcm PCM handle
 * \param params Configuration space
 * \param val approximate target buffer duration in us / returned chosen approximate target buffer duration
 * \return 0 otherwise a negative error code if configuration space is empty
 *
 * target/chosen exact value is <,=,> val following dir (-1,0,1)
 */
int
__snd_pcm_hw_params_set_period_time_near (snd_pcm_t * pcm,
					  snd_pcm_hw_params_t * params,
					  unsigned int *val, int *dir)
{
  dbg_printf2 ("snd_pcm_hw_params_set_period_time_near()\n");
  return 0;
}

/**
 * \brief Application request to access a portion of direct (mmap) area
 * \param pcm PCM handle
 * \param areas Returned mmap channel areas
 * \param offset Returned mmap area offset in area steps (== frames)
 * \param frames mmap area portion size in frames (wanted on entry, contiguous available on exit)
 * \return 0 on success otherwise a negative error code
 *
 * It is necessary to call the snd_pcm_avail_update() function directly before
 * this call. Otherwise, this function can return a wrong count of available frames.
 *
 * The function should be called before a sample-direct area can be accessed.
 * The resulting size parameter is always less or equal to the input count of frames
 * and can be zero, if no frames can be processed (the ring buffer is full).
 *
 * See the snd_pcm_mmap_commit() function to finish the frame processing in
 * the direct areas.
 */
int
snd_pcm_mmap_begin (snd_pcm_t * pcm,
		    const snd_pcm_channel_area_t ** areas,
		    snd_pcm_uframes_t * offset, snd_pcm_uframes_t * frames)
{
  dbg_printf2 ("snd_pcm_mmap_begin(frames=%d)\n", *frames);
  if (pcm->area == NULL)
    {
      pcm->area = malloc (sizeof (*pcm->area) * pcm->channels);
      pcm->area[0].addr = malloc (65536);
      pcm->area[0].first = 0;
      pcm->area[0].step = pcm->frame_size * 8;
      pcm->area[1].addr = pcm->area[0].addr + pcm->sample_size;
      pcm->area[1].first = 0;
      pcm->area[1].step = pcm->frame_size * 8;
    }

  *areas = pcm->area;
  *offset = 0;
  if (*frames * pcm->frame_size > 65536)
    *frames = 65536 / pcm->frame_size;
  return 0;
}

/**
 * \brief Application has completed the access to area requested with #snd_pcm_mmap_begin
 * \param pcm PCM handle
 * \param offset area offset in area steps (== frames)
 * \param size area portion size in frames
 * \return count of transferred frames otherwise a negative error code
 *
 * You should pass this function the offset value that
 * snd_pcm_mmap_begin() returned. The frames parameter should hold the
 * number of frames you have written or read to/from the audio
 * buffer. The frames parameter must never exceed the contiguous frames
 * count that snd_pcm_mmap_begin() returned. Each call to snd_pcm_mmap_begin()
 * must be followed by a call to snd_pcm_mmap_commit().
 *
 * Example:
\code
  double phase = 0;
  const snd_pcm_area_t *areas;
  snd_pcm_sframes_t avail, size, commitres;
  snd_pcm_uframes_t offset, frames;
  int err;

  avail = snd_pcm_avail_update(pcm);
  if (avail < 0)
    error(avail);
  // at this point, we can transfer at least 'avail' frames
  
  // we want to process frames in chunks (period_size)
  if (avail < period_size)
    goto _skip;
  size = period_size;
  // it is possible that contiguous areas are smaller, thus we use a loop
  while (size > 0) {
    frames = size;

    err = snd_pcm_mmap_begin(pcm_handle, &areas, &offset, &frames);
    if (err < 0)
      error(err);
    // this function fills the areas from offset with count of frames
    generate_sine(areas, offset, frames, &phase);
    commitres = snd_pcm_mmap_commit(pcm_handle, offset, frames);
    if (commitres < 0 || commitres != frames)
      error(commitres >= 0 ? -EPIPE : commitres);
      
    size -= frames;
  }
 _skip:
\endcode
 *
 * Look to the \ref example_test_pcm "Sine-wave generator" example
 * for more details about the generate_sine function.
 */
snd_pcm_sframes_t
snd_pcm_mmap_commit (snd_pcm_t * pcm,
		     snd_pcm_uframes_t offset, snd_pcm_uframes_t frames)
{
  dbg_printf2 ("snd_pcm_mmap_commit(frames=%d, offs=%d)\n", frames, offset);

  if (pcm->stream == SND_PCM_STREAM_PLAYBACK)
    return snd_pcm_writei (pcm, pcm->area[0].addr, frames);

  if (pcm->stream == SND_PCM_STREAM_CAPTURE)
    return snd_pcm_readi (pcm, pcm->area[0].addr, frames);

  return -EFAULT;
}

/**
 * \brief Restrict a configuration space to have buffer time nearest to a target * \param pcm PCM handle
 * \param params Configuration space
 * \param val approximate target buffer duration in us / returned chosen approximate target buffer duration
 * \return 0 otherwise a negative error code if configuration space is empty
 *
 * target/chosen exact value is <,=,> val following dir (-1,0,1)
 */
int
__snd_pcm_hw_params_set_buffer_time_near (snd_pcm_t * pcm,
					  snd_pcm_hw_params_t * params,
					  unsigned int *val, int *dir)
{
  *val = 10;
  dbg_printf2 ("snd_pcm_hw_params_set_buffer_time_near()\n");
  return 0;
}

/**
 * \brief Pause/resume PCM
 * \param pcm PCM handle
 * \param pause 0 = resume, 1 = pause
 * \return 0 on success otherwise a negative error code
 *
 * Note that this function works only on the hardware which supports
 * pause feature.  You can check it via \link ::snd_pcm_hw_params_can_pause() \endlink
 * function.
 */
int
snd_pcm_pause (snd_pcm_t * pcm, int enable)
{
  dbg_printf ("snd_pcm_pause()\n");
  return -EINVAL;
}

/**
 * \brief Start a PCM
 * \param pcm PCM handle
 * \return 0 on success otherwise a negative error code
 */
int
snd_pcm_start (snd_pcm_t * pcm)
{
  dbg_printf ("snd_pcm_start()\n");
  return 0;
}

/**
 * \brief Convert samples in bytes for a PCM
 * \param pcm PCM handle
 * \param samples quantity in samples
 * \return quantity expressed in bytes
 */
ssize_t
snd_pcm_samples_to_bytes (snd_pcm_t * pcm, long samples)
{
  dbg_printf2 ("snd_pcm_samples_to_bytes(%d)=%d\n", samples,
	       samples * pcm->sample_size);
  return samples * pcm->sample_size;
}


/**
 * \brief Dump status
 * \param status Status container
 * \param out Output handle
 * \return 0 on success otherwise a negative error code
 */
int
snd_pcm_status_dump (snd_pcm_status_t * status, snd_output_t * out)
{
  return 0;
}

/** \brief Install PCM software configuration defined by params
 * \param pcm PCM handle
 * \param params Configuration container
 * \return 0 on success otherwise a negative error code
 */
int
snd_pcm_sw_params (snd_pcm_t * pcm, snd_pcm_sw_params_t * params)
{
  dbg_printf2 ("snd_pcm_sw_params()\n");
  // NOP
  return 0;
}

/**
 * \brief Return current software configuration for a PCM
 * \param pcm PCM handle
 * \param params Software configuration container
 * \return 0 on success otherwise a negative error code
 */
int
snd_pcm_sw_params_current (snd_pcm_t * pcm, snd_pcm_sw_params_t * params)
{
  dbg_printf2 ("snd_pcm_sw_params_current()\n");
  // NOP
  return 0;
}

/**
 * \brief Dump a software configuration
 * \param params Software configuration container
 * \param out Output handle
 * \return 0 on success otherwise a negative error code
 */
int
snd_pcm_sw_params_dump (snd_pcm_sw_params_t * params, snd_output_t * out)
{
  return 0;
}

/**
 * \brief Set start threshold inside a software configuration container
 * \param pcm PCM handle
 * \param params Software configuration container
 * \param val Start threshold in frames
 * \return 0 otherwise a negative error code
 *
 * PCM is automatically started when playback frames available to PCM
 * are >= threshold or when requested capture frames are >= threshold
 */
int
snd_pcm_sw_params_set_start_threshold (snd_pcm_t * pcm,
				       snd_pcm_sw_params_t * params,
				       snd_pcm_uframes_t val)
{
  dbg_printf2 ("snd_pcm_sw_params_set_start_threshold(%d)\n", val);
  // NOP
  return 0;
}

/**
 * \brief get size of #snd_pcm_sw_params_t
 * \return size in bytes
 */
size_t
snd_pcm_sw_params_sizeof ()
{
  return sizeof (snd_pcm_sw_params_t);
}

/**
 * \brief Restrict a configuration space to have periods count nearest to a target
 * \param pcm PCM handle
 * \param params Configuration space
 * \param val approximate target periods per buffer / returned chosen approximate target periods per buffer
 * \return 0 otherwise a negative error code if configuration space is empty
 *
 * target/chosen exact value is <,=,> val following dir (-1,0,1)
 */
int INTERNAL (snd_pcm_hw_params_set_periods_near) (snd_pcm_t * pcm,
						   snd_pcm_hw_params_t *
						   params, unsigned int *val,
						   int *dir)
{
  dbg_printf
    ("snd_pcm_hw_params_set_periods_near(pcm=%x, params=%x, val=%d, dir=%d)\n");
  pcm->periods = *val;
  return 0;
}

/**
 * \brief get poll descriptors
 * \param pcm PCM handle
 * \param pfds array of poll descriptors
 * \param space space in the poll descriptor array
 * \return count of filled descriptors
 *
 * This function fills the given poll descriptor structs for the specified
 * PCM handle.  The poll desctiptor array should have the size returned by
 * \link ::snd_pcm_poll_descriptors_count() \endlink function.
 *
 * The result is intended for direct use with the poll() syscall.
 *
 * For reading the returned events of poll descriptor after poll() system
 * call, use \link ::snd_pcm_poll_descriptors_revents() \endlink function.
 * The field values in pollfd structs may be bogus regarding the stream
 * direction from the application perspective (POLLIN might not imply read
 * direction and POLLOUT might not imply write), but
 * the \link ::snd_pcm_poll_descriptors_revents() \endlink function
 * does the right "demangling".
 *
 * You can use output from this function as arguments for the select()
 * syscall, too.
 */
int
snd_pcm_poll_descriptors (snd_pcm_t * pcm, struct pollfd *pfds,
			  unsigned int space)
{
  dbg_printf2 ("snd_pcm_poll_descriptors(pcm=%x)\n", pcm);

  pfds->fd = pcm->fd;
  pfds->events = 0;
  pfds->revents = 0;

  if (pcm->oss_mode == O_WRONLY || pcm->oss_mode == O_RDWR)
    pfds->events = POLLOUT;
  if (pcm->oss_mode == O_RDONLY || pcm->oss_mode == O_RDWR)
    pfds->events = POLLIN;

  return 1;
}

/**
 * \brief get count of poll descriptors for PCM handle
 * \param pcm PCM handle
 * \return count of poll descriptors
 */
int
snd_pcm_poll_descriptors_count (snd_pcm_t * pcm)
{
  dbg_printf ("snd_pcm_poll_descriptors_count()\n");

  return 1;
}

/**
 * \brief get returned events from poll descriptors
 * \param pcm PCM handle
 * \param pfds array of poll descriptors
 * \param nfds count of poll descriptors
 * \param revents returned events
 * \return zero if success, otherwise a negative error code
 *
 * This function does "demangling" of the revents mask returned from
 * the poll() syscall to correct semantics (POLLIN = read, POLLOUT = write).
 *
 * Note: The null event also exists. Even if poll() or select()
 * syscall returned that some events are waiting, this function might
 * return empty set of events. In this case, application should
 * do next event waiting using poll() or select().
 */
int
snd_pcm_poll_descriptors_revents (snd_pcm_t * pcm, struct pollfd *pfds,
				  unsigned int nfds, unsigned short *revents)
{
  dbg_printf3 ("snd_pcm_poll_descriptors_revents()\n");

  *revents = pfds->revents;
  return 0;
}

/**
 * \brief allocate an invalid #snd_pcm_hw_params_t using standard malloc
 * \param ptr returned pointer
 * \return 0 on success otherwise negative error code
 */
int
snd_pcm_hw_params_malloc (snd_pcm_hw_params_t ** ptr)
{
  *ptr = calloc (1, sizeof (snd_pcm_hw_params_t));
  if (!*ptr)
    return -ENOMEM;
  return 0;
}

/**
 * \brief frees a previously allocated #snd_pcm_sw_params_t
 * \param pointer to object to free
 */
void
snd_pcm_sw_params_free (snd_pcm_sw_params_t * obj)
{
  free (obj);
}

/**
 * \brief Write non interleaved frames to a PCM
 * \param pcm PCM handle
 * \param bufs frames containing buffers (one for each channel)
 * \param size frames to be written
 * \return a positive number of frames actually written otherwise a
 * negative error code
 * \retval -EBADFD PCM is not in the right state (#SND_PCM_STATE_PREPARED or #SND_PCM_STATE_RUNNING)
 * \retval -EPIPE an underrun occurred
 * \retval -ESTRPIPE a suspend event occurred (stream is suspended and waiting for an application recovery)
 *
 * If the blocking behaviour is selected, then routine waits until
 * all requested bytes are played or put to the playback ring buffer.
 * The count of bytes can be less only if a signal or underrun occurred.
 *
 * If the non-blocking behaviour is selected, then routine doesn't wait at all.
 */
snd_pcm_sframes_t
snd_pcm_writen (snd_pcm_t * pcm, void **bufs, snd_pcm_uframes_t size)
{
  dbg_printf ("snd_pcm_writen()\n");
  // Not supported
  return -EINVAL;
}

/**
 * \brief Read non interleaved frames to a PCM
 * \param pcm PCM handle
 * \param bufs frames containing buffers (one for each channel)
 * \param size frames to be written
 * \return a positive number of frames actually read otherwise a
 * negative error code
 * \retval -EBADFD PCM is not in the right state (#SND_PCM_STATE_PREPARED or #SND_PCM_STATE_RUNNING)
 * \retval -EPIPE an overrun occurred
 * \retval -ESTRPIPE a suspend event occurred (stream is suspended and waiting for an application recovery)
 *
 * If the blocking behaviour was selected, then routine waits until
 * all requested bytes are filled. The count of bytes can be less only
 * if a signal or underrun occurred.
 *
 * If the non-blocking behaviour is selected, then routine doesn't wait at all.
 */
snd_pcm_sframes_t
snd_pcm_readn (snd_pcm_t * pcm, void **bufs, snd_pcm_uframes_t size)
{
  dbg_printf ("snd_pcm_readn()\n");
  // Not supported
  return -EINVAL;
}

/**
 * \brief Get trigger timestamp from a PCM status container
 * \param ptr Pointer to returned timestamp
 */
void
snd_pcm_status_get_trigger_tstamp (const snd_pcm_status_t * obj,
				   snd_timestamp_t * ptr)
{
  dbg_printf ("snd_pcm_status_get_trigger_tstamp()\n");
  ptr->tv_sec = 0;
  ptr->tv_usec = 0;
}

/**
 * \brief Set xfer align inside a software configuration container
 * \param pcm PCM handle
 * \param params Software configuration container
 * \param val Chunk size (frames are attempted to be transferred in chunks)
 * \return 0 otherwise a negative error code
 */
int
snd_pcm_sw_params_set_xfer_align (snd_pcm_t * pcm,
				  snd_pcm_sw_params_t * params,
				  snd_pcm_uframes_t val)
{
  dbg_printf ("snd_pcm_sw_params_set_xfer_align(%d)\n", val);
  // TODO
  return 0;
}

/**
 * \brief Get xfer align from a software configuration container
 * \param params Software configuration container
 * \param val returned chunk size (frames are attempted to be transferred in chunks)
 * \param 0 otherwise a negative error code
 */
int INTERNAL (snd_pcm_sw_params_get_xfer_align) (const snd_pcm_sw_params_t *
						 params,
						 snd_pcm_uframes_t * val)
{
  dbg_printf ("snd_pcm_sw_params_get_xfer_align()\n");

  return 8;
}

/**
 * \brief Set avail min inside a software configuration container
 * \param pcm PCM handle
 * \param params Software configuration container
 * \param val Minimum avail frames to consider PCM ready
 * \return 0 otherwise a negative error code
 *
 * Note: This is similar to setting an OSS wakeup point.  The valid
 * values for 'val' are determined by the specific hardware.  Most PC
 * sound cards can only accept power of 2 frame counts (i.e. 512,
 * 1024, 2048).  You cannot use this as a high resolution timer - it
 * is limited to how often the sound card hardware raises an
 * interrupt. Note that you can greatly improve the reponses using
 * \ref snd_pcm_sw_params_set_sleep_min where another timing source
 * is used.
 */
int
snd_pcm_sw_params_set_avail_min (snd_pcm_t * pcm ATTRIBUTE_UNUSED,
				 snd_pcm_sw_params_t * params,
				 snd_pcm_uframes_t val)
{
  dbg_printf ("snd_pcm_sw_params_set_avail_min()\n");
  // NOP
  return 0;
}

/**
 * \brief Set minimum number of ticks to sleep inside a software configuration container
 * \param pcm PCM handle
 * \param params Software configuration container
 * \param val Minimum ticks to sleep or 0 to disable the use of tick timer
 * \return 0 otherwise a negative error code
 */
int
snd_pcm_sw_params_set_sleep_min (snd_pcm_t * pcm ATTRIBUTE_UNUSED,
				 snd_pcm_sw_params_t * params,
				 unsigned int val)
{
  dbg_printf ("snd_pcm_sw_params_set_sleep_min()\n");
  // NOP

  return 0;
}

/**
 * \brief Wait for a PCM to become ready
 * \param pcm PCM handle
 * \param timeout maximum time in milliseconds to wait
 * \return a positive value on success otherwise a negative error code
 *         (-EPIPE for the xrun and -ESTRPIPE for the suspended status,
 *          others for general errors)
 * \retval 0 timeout occurred
 * \retval 1 PCM stream is ready for I/O
 */
int
snd_pcm_wait (snd_pcm_t * pcm, int timeout)
{
  fd_set fds, *readfds = NULL, *writefds = NULL;
  struct timeval wait, *w = NULL;

  dbg_printf ("snd_pcm_wait(%d)\n", timeout);

  FD_ZERO (&fds);
  FD_SET (pcm->fd, &fds);

  if (timeout > 0)
    {
      wait.tv_sec = timeout / 1000;
      wait.tv_usec = (timeout % 1000) * 1000;
      w = &wait;
    }

  if (pcm->stream == SND_PCM_STREAM_PLAYBACK)
    writefds = &fds;
  else
    readfds = &fds;

  dbg_printf ("select(%d, %x, %x, NULL, %x)\n",
	      pcm->fd + 1, readfds, writefds, w);
  if (select (pcm->fd + 1, readfds, writefds, NULL, w) == -1)
    return -errno;

  return FD_ISSET (pcm->fd, &fds);
}

/**
 * \brief allocate an invalid #snd_pcm_sw_params_t using standard malloc
 * \param ptr returned pointer
 * \return 0 on success otherwise negative error code
 */
int
snd_pcm_sw_params_malloc (snd_pcm_sw_params_t ** ptr)
{
  *ptr = calloc (1, sizeof (snd_pcm_sw_params_t));
  if (!*ptr)
    return -ENOMEM;
  return 0;
}

/**
 * \brief Extract period time from a configuration space
 * \param params Configuration space
 * \param val Returned approximate period duration in us
 * \param dir Sub unit direction
 * \return 0 otherwise a negative error code if not exactly one is present
 *
 * Actual exact value is <,=,> the approximate one following dir (-1, 0, 1)
 */
int INTERNAL (snd_pcm_hw_params_get_period_time) (const snd_pcm_hw_params_t *
						  params, unsigned int *val,
						  int *dir)
{
  dbg_printf ("snd_pcm_hw_params_get_period_time()\n");
  // TODO
  return -EINVAL;
}

int INTERNAL (snd_pcm_hw_params_get_buffer_time) (const snd_pcm_hw_params_t *
						  params, unsigned int *val,
						  int *dir)
{
  dbg_printf ("snd_pcm_hw_params_get_buffer_time()\n");
  // What's this?
  return -EINVAL;
}

struct _snd_mask
{
  unsigned int bits[4];
};

struct _snd_pcm_access_mask
{
  unsigned int bits[4];
};

#define MASK_OFS(v) (v/32)
#define MASK_BIT(v) (1<<(v%32))

/**
 * \brief reset all bits in a #snd_pcm_access_mask_t
 * \param mask pointer to mask
 */
void
snd_pcm_access_mask_none (snd_pcm_access_mask_t * mask)
{
  dbg_printf ("snd_pcm_access_mask_none()\n");
  memset (mask, 0, sizeof (*mask));
}

/**
 * \brief make an access type present in a #snd_pcm_access_mask_t
 * \param mask pointer to mask
 * \param val access type
 */
void
snd_pcm_access_mask_set (snd_pcm_access_mask_t * mask, snd_pcm_access_t val)
{
  dbg_printf ("snd_pcm_access_mask_t(%d)\n", val);
  mask->bits[MASK_OFS (val)] |= MASK_BIT (val);
}

/**
 * \brief get size of #snd_pcm_access_mask_t
 * \return size in bytes
 */
size_t
snd_pcm_access_mask_sizeof ()
{
  return sizeof (snd_pcm_access_mask_t);
}

/**
 * \brief Restrict a configuration space to contain only a set of access types
 * \param pcm PCM handle
 * \param params Configuration space
 * \param mask Access mask
 * \return 0 otherwise a negative error code
 */
int
snd_pcm_hw_params_set_access_mask (snd_pcm_t * pcm,
				   snd_pcm_hw_params_t * params,
				   snd_pcm_access_mask_t * mask)
{
  dbg_printf ("snd_pcm_hw_params_set_access_mask()\n");
  // NOP
  return 0;
}

/**
 * \brief Read interleaved frames from a PCM using direct buffer (mmap)
 * \param pcm PCM handle
 * \param buffer frames containing buffer
 * \param size frames to be written
 * \return a positive number of frames actually read otherwise a
 * negative error code
 * \retval -EBADFD PCM is not in the right state (#SND_PCM_STATE_PREPARED or #SND_PCM_STATE_RUNNING)
 * \retval -EPIPE an overrun occurred
 * \retval -ESTRPIPE a suspend event occurred (stream is suspended and waiting for an application recovery)
 *
 * If the blocking behaviour was selected, then routine waits until
 * all requested bytes are filled. The count of bytes can be less only
 * if a signal or underrun occurred.
 *
 * If the non-blocking behaviour is selected, then routine doesn't wait at all.
 */
snd_pcm_sframes_t
snd_pcm_mmap_readi (snd_pcm_t * pcm, void *buffer, snd_pcm_uframes_t size)
{
  dbg_printf ("snd_pcm_mmap_readi()\n");
  return snd_pcm_readi (pcm, buffer, size);
}

/**
 * \brief Read non interleaved frames to a PCM using direct buffer (mmap)
 * \param pcm PCM handle
 * \param bufs frames containing buffers (one for each channel)
 * \param size frames to be written
 * \return a positive number of frames actually read otherwise a
 * negative error code
 * \retval -EBADFD PCM is not in the right state (#SND_PCM_STATE_PREPARED or #SND_PCM_STATE_RUNNING)
 * \retval -EPIPE an overrun occurred
 * \retval -ESTRPIPE a suspend event occurred (stream is suspended and waiting for an application recovery)
 *
 * If the blocking behaviour was selected, then routine waits until
 * all requested bytes are filled. The count of bytes can be less only
 * if a signal or underrun occurred.
 *
 * If the non-blocking behaviour is selected, then routine doesn't wait at all.
 */
snd_pcm_sframes_t
snd_pcm_mmap_readn (snd_pcm_t * pcm, void **bufs, snd_pcm_uframes_t size)
{
  dbg_printf ("snd_pcm_mmap_readn()\n");
  return snd_pcm_readn (pcm, bufs, size);
}

/**
 * \brief Write interleaved frames to a PCM using direct buffer (mmap)
 * \param pcm PCM handle
 * \param buffer frames containing buffer
 * \param size frames to be written
 * \return a positive number of frames actually written otherwise a
 * negative error code
 * \retval -EBADFD PCM is not in the right state (#SND_PCM_STATE_PREPARED or #SND_PCM_STATE_RUNNING)
 * \retval -EPIPE an underrun occurred
 * \retval -ESTRPIPE a suspend event occurred (stream is suspended and waiting for an application recovery)
 *
 * If the blocking behaviour is selected, then routine waits until
 * all requested bytes are played or put to the playback ring buffer.
 * The count of bytes can be less only if a signal or underrun occurred.
 *
 * If the non-blocking behaviour is selected, then routine doesn't wait at all.
 */
snd_pcm_sframes_t
snd_pcm_mmap_writei (snd_pcm_t * pcm, const void *buffer,
		     snd_pcm_uframes_t size)
{
  dbg_printf ("snd_pcm_mmap_writei()\n");

  return snd_pcm_writei (pcm, buffer, size);
}

/**
 * \brief Write non interleaved frames to a PCM using direct buffer (mmap)
 * \param pcm PCM handle
 * \param bufs frames containing buffers (one for each channel)
 * \param size frames to be written
 * \return a positive number of frames actually written otherwise a
 * negative error code
 * \retval -EBADFD PCM is not in the right state (#SND_PCM_STATE_PREPARED or #SND_PCM_STATE_RUNNING)
 * \retval -EPIPE an underrun occurred
 * \retval -ESTRPIPE a suspend event occurred (stream is suspended and waiting for an application recovery)
 *
 * If the blocking behaviour is selected, then routine waits until
 * all requested bytes are played or put to the playback ring buffer.
 * The count of bytes can be less only if a signal or underrun occurred.
 *
 * If the non-blocking behaviour is selected, then routine doesn't wait at all.
 */
snd_pcm_sframes_t
snd_pcm_mmap_writen (snd_pcm_t * pcm, void **bufs, snd_pcm_uframes_t size)
{
  dbg_printf ("snd_pcm_mmap_writen()\n");
  return snd_pcm_writen (pcm, bufs, size);
}

/**
 * \brief Link two PCMs
 * \param pcm1 first PCM handle
 * \param pcm2 first PCM handle
 * \return 0 on success otherwise a negative error code
 *
 * The two PCMs will start/stop/prepare in sync.
 */
int
snd_pcm_link (snd_pcm_t * pcm1, snd_pcm_t * pcm2)
{
  dbg_printf ("snd_pcm_link()\n");
  return -EINVAL;
}

/**
 * \brief Set silence size inside a software configuration container
 * \param pcm PCM handle
 * \param params Software configuration container
 * \param val Silence size in frames (0 for disabled)
 * \return 0 otherwise a negative error code
 *
 * A portion of playback buffer is overwritten with silence when playback
 * underrun is nearer than silence threshold (see
 * #snd_pcm_sw_params_set_silence_threshold)
 *
 * The special case is when silence size value is equal or greater than
 * boundary. The unused portion of the ring buffer (initial written samples
 * are untouched) is filled with silence at start. Later, only just processed
 * sample area is filled with silence. Note: silence_threshold must be set to zero.
 */
int
snd_pcm_sw_params_set_silence_size (snd_pcm_t * pcm,
				    snd_pcm_sw_params_t * params,
				    snd_pcm_uframes_t val)
{
  dbg_printf ("snd_pcm_sw_params_set_silence_size()\n");

  return 0;
}

/**
 * \brief Set silence threshold inside a software configuration container
 * \param pcm PCM handle
 * \param params Software configuration container
 * \param val Silence threshold in frames
 * \return 0 otherwise a negative error code
 *
 * A portion of playback buffer is overwritten with silence (see
 * #snd_pcm_sw_params_set_silence_size) when playback underrun is nearer
 * than silence threshold.
 */
int
snd_pcm_sw_params_set_silence_threshold (snd_pcm_t * pcm, snd_pcm_sw_params_t
					 * params, snd_pcm_uframes_t val)
{
  dbg_printf ("snd_pcm_sw_params_set_silence_threshold()\n");

  return 0;
}

/**
 * \brief Restrict a configuration space to contain only one period size
 * \param pcm PCM handle
 * \param params Configuration space
 * \param val approximate period size in frames
 * \param dir Sub unit direction
 * \return 0 otherwise a negative error code if configuration space would become empty
 *
 * Wanted exact value is <,=,> val following dir (-1,0,1)
 */
int
snd_pcm_hw_params_set_period_size (snd_pcm_t * pcm,
				   snd_pcm_hw_params_t * params,
				   snd_pcm_uframes_t val, int dir)
{
  dbg_printf
    ("snd_pcm_hw_params_set_period_size(pcm=%x, params=%x, val=%d, dir=%d)\n",
     pcm, params, val, dir);
  pcm->period_size = val;
  return 0;
}

/**
 * \brief Set stop threshold inside a software configuration container
 * \param pcm PCM handle
 * \param params Software configuration container
 * \param val Stop threshold in frames
 * \return 0 otherwise a negative error code
 *
 * PCM is automatically stopped in #SND_PCM_STATE_XRUN state when available
 * frames is >= threshold. If the stop threshold is equal to boundary (also
 * software parameter - sw_param) then automatic stop will be disabled
 * (thus device will do the endless loop in the ring buffer).
 */
int
snd_pcm_sw_params_set_stop_threshold (snd_pcm_t * pcm,
				      snd_pcm_sw_params_t * params,
				      snd_pcm_uframes_t val)
{
  dbg_printf ("snd_pcm_sw_params_set_stop_threshold()\n");

  return 0;
}

/**
 * \brief Return 16 bit expressing silence for a PCM sample format
 * \param format Sample format
 * \return silence 16 bit word
 */
u_int16_t
snd_pcm_format_silence_16 (snd_pcm_format_t format)
{
  dbg_printf ("snd_pcm_format_silence_16(%x)\n", format);
  return 0;
}

/**
 * \brief Return 8 bit expressing silence for a PCM sample format
 * \param format Sample format
 * \return silence 8 bit word
 */
u_int8_t
snd_pcm_format_silence (snd_pcm_format_t format)
{

  dbg_printf ("snd_pcm_format_silence(%x)\n", format);

  return 0x80;
}


/**
 * \brief Return 32 bit expressing silence for a PCM sample format
 * \param format Sample format
 * \return silence 32 bit word
 */
u_int32_t
snd_pcm_format_silence_32 (snd_pcm_format_t format)
{
  dbg_printf ("snd_pcm_format_silence_32(%x)\n", format);
  return 0;
}


/**
 * \brief Get available subdevices count from a PCM info container
 * \param obj PCM info container
 * \return available subdevices count of PCM
 */
unsigned int
snd_pcm_info_get_subdevices_avail (const snd_pcm_info_t * obj)
{
  dbg_printf ("snd_pcm_info_get_subdevices_avail()\n");

  return 1;
}

/**
 * \brief get description of PCM sample format
 * \param format PCM sample format
 * \return ascii description of PCM sample format
 */
const char *
snd_pcm_format_description (snd_pcm_format_t format)
{
  if (format > SND_PCM_FORMAT_LAST)
    return NULL;
  return snd_pcm_format_descriptions[format];
}

/**
 * \brief Get subdevices count from a PCM info container
 * \param obj PCM info container
 * \return subdevices total count of PCM
 */
unsigned int
snd_pcm_info_get_subdevices_count (const snd_pcm_info_t * obj)
{
  dbg_printf ("snd_pcm_info_get_subdevices_count()\n");

  return 1;
}

/**
 * \brief Silence a PCM samples buffer
 * \param format Sample format
 * \param data Buffer
 * \return samples Samples count
 */
int
snd_pcm_format_set_silence (snd_pcm_format_t format, void *data,
			    unsigned int samples)
{
  dbg_printf ("snd_pcm_format_set_silence()\n");

  return 0;
}


/**
 * \brief get identifier of PCM handle
 * \param pcm PCM handle
 * \return ascii identifier of PCM handle
 *
 * Returns the ASCII identifier of given PCM handle. It's the same
 * identifier specified in snd_pcm_open().
 */
const char *
snd_pcm_name (snd_pcm_t * pcm)
{
  return pcm->info.name;
}

/**
 * \brief get name of PCM state
 * \param state PCM state
 * \return ascii name of PCM state
 */
const char *
snd_pcm_state_name (snd_pcm_state_t state)
{
  if (state > SND_PCM_STATE_LAST)
    return NULL;
  return snd_pcm_state_names[state];

}

/**
 * \brief get name of PCM stream type
 * \param stream PCM stream type
 * \return ascii name of PCM stream type
 */
const char *
snd_pcm_stream_name (snd_pcm_stream_t stream)
{
  assert (stream <= SND_PCM_STREAM_LAST);
  return snd_pcm_stream_names[stream];
}

/**
 * \brief Extract maximum buffer time from a configuration space
 * \param params Configuration space
 * \param val approximate maximum buffer duration in us
 * \param dir Sub unit direction
 * \return 0 otherwise a negative error code
 *
 * Exact value is <,=,> the returned one following dir (-1,0,1)
 */
int INTERNAL (snd_pcm_hw_params_get_buffer_time_max) (const
						      snd_pcm_hw_params_t *
						      params,
						      unsigned int *val,
						      int *dir)
{
  dbg_printf ("snd_pcm_hw_params_get_buffer_time_max()\n");
  return 0;
}

/**
 * \brief Get subdevice name from a PCM info container
 * \param obj PCM info container
 * \return name of used PCM subdevice
 */
const char *
snd_pcm_info_get_subdevice_name (const snd_pcm_info_t * obj)
{
  dbg_printf ("snd_pcm_info_get_subdevice_name()\n");

  return "OSS subname";
}

int
snd_device_name_hint (int card, const char * iface, void *** hints)
{
  dbg_printf ("snd_device_name_hint()\n");

  return -1;
}

int
snd_pcm_hw_params_is_monotonic (const snd_pcm_hw_params_t * params)
{
  return 0;
}

/**
 * \brief get PCM sample format from name
 * \param name PCM sample format name (case insensitive)
 * \return PCM sample format
 */
snd_pcm_format_t
snd_pcm_format_value (const char *name)
{
  snd_pcm_format_t format;
  for (format = 0; format <= SND_PCM_FORMAT_LAST; format++)
    {
      if (snd_pcm_format_names[format] &&
	  strcasecmp (name, snd_pcm_format_names[format]) == 0)
	{
	  return format;
	}
    }
  for (format = 0; format <= SND_PCM_FORMAT_LAST; format++)
    {
      if (snd_pcm_format_descriptions[format] &&
	  strcasecmp (name, snd_pcm_format_descriptions[format]) == 0)
	{
	  return format;
	}
    }
  return SND_PCM_FORMAT_UNKNOWN;
}


/**
 * \brief Get id from a PCM info container
 * \param obj PCM info container
 * \return short id of PCM
 */
const char *
snd_pcm_info_get_id (const snd_pcm_info_t * obj)
{
  dbg_printf ("snd_pcm_info_get_id()\n");

  return "OSS ID";
}

/**
 * \brief Return nominal bits per a PCM sample
 * \param format Sample format
 * \return bits per sample, a negative error code if not applicable
 */
int
snd_pcm_format_width (snd_pcm_format_t format)
{
  switch (format)
    {
    case SNDRV_PCM_FORMAT_S8:
    case SNDRV_PCM_FORMAT_U8:
      return 8;
    case SNDRV_PCM_FORMAT_S16_LE:
    case SNDRV_PCM_FORMAT_S16_BE:
    case SNDRV_PCM_FORMAT_U16_LE:
    case SNDRV_PCM_FORMAT_U16_BE:
      return 16;
    case SNDRV_PCM_FORMAT_S18_3LE:
    case SNDRV_PCM_FORMAT_S18_3BE:
    case SNDRV_PCM_FORMAT_U18_3LE:
    case SNDRV_PCM_FORMAT_U18_3BE:
      return 18;
    case SNDRV_PCM_FORMAT_S20_3LE:
    case SNDRV_PCM_FORMAT_S20_3BE:
    case SNDRV_PCM_FORMAT_U20_3LE:
    case SNDRV_PCM_FORMAT_U20_3BE:
      return 20;
    case SNDRV_PCM_FORMAT_S24_LE:
    case SNDRV_PCM_FORMAT_S24_BE:
    case SNDRV_PCM_FORMAT_U24_LE:
    case SNDRV_PCM_FORMAT_U24_BE:
    case SNDRV_PCM_FORMAT_S24_3LE:
    case SNDRV_PCM_FORMAT_S24_3BE:
    case SNDRV_PCM_FORMAT_U24_3LE:
    case SNDRV_PCM_FORMAT_U24_3BE:
      return 24;
    case SNDRV_PCM_FORMAT_S32_LE:
    case SNDRV_PCM_FORMAT_S32_BE:
    case SNDRV_PCM_FORMAT_U32_LE:
    case SNDRV_PCM_FORMAT_U32_BE:
    case SNDRV_PCM_FORMAT_FLOAT_LE:
    case SNDRV_PCM_FORMAT_FLOAT_BE:
      return 32;
    case SNDRV_PCM_FORMAT_FLOAT64_LE:
    case SNDRV_PCM_FORMAT_FLOAT64_BE:
      return 64;
    case SNDRV_PCM_FORMAT_IEC958_SUBFRAME_LE:
    case SNDRV_PCM_FORMAT_IEC958_SUBFRAME_BE:
      return 32;
    case SNDRV_PCM_FORMAT_MU_LAW:
    case SNDRV_PCM_FORMAT_A_LAW:
      return 8;
    case SNDRV_PCM_FORMAT_IMA_ADPCM:
      return 4;
    default:
      return -EINVAL;
    }
}


/**
 * \brief Add an async handler for a PCM
 * \param handler Returned handler handle
 * \param pcm PCM handle
 * \param callback Callback function
 * \param private_data Callback private data
 * \return 0 otherwise a negative error code on failure
 *
 * The asynchronous callback is called when period boundary elapses.
 */
int
snd_async_add_pcm_handler (snd_async_handler_t ** handler, snd_pcm_t * pcm,
			   snd_async_callback_t callback, void *private_data)
{
  dbg_printf ("snd_async_add_pcm_handler()\n");

  return -EINVAL;
}

void *
snd_async_handler_get_callback_private (snd_async_handler_t * handler)
{
  // What's this?
  dbg_printf ("snd_async_handler_get_callback_private()\n");
  return NULL;
}

/*
 * What's this?
 */
int
snd_pcm_format_linear (snd_pcm_format_t format)
{
  return 1;
}

/*
 * What's this?
 */
int
snd_pcm_format_float (snd_pcm_format_t format)
{
  return 0;
}

/*
 * What's this?
 */
int
snd_pcm_format_signed (snd_pcm_format_t format)
{
  return (format != SND_PCM_FORMAT_U8);
}

/*
 * What's this?
 */
snd_pcm_format_t
snd_pcm_build_linear_format (int width, int pwidth, int unsignd,
			     int big_endian)
{
  printf ("Invalid ALSA call snd_pcm_build_linear_format()\n");
  return 0;
}

int
snd_pcm_format_cpu_endian (snd_pcm_format_t format)
{
  return 1;
}

int
snd_pcm_format_little_endian (snd_pcm_format_t format)
{
  return 1;
}

/**
 * \brief get size of #snd_pcm_format_mask_t
 * \return size in bytes
 */
size_t
snd_pcm_format_mask_sizeof ()
{
  return sizeof (snd_pcm_format_mask_t);
}

/**
 * \brief test the presence of a format in a #snd_pcm_format_mask_t
 * \param mask pointer to mask
 * \param val format
 */
int
snd_pcm_format_mask_test (const snd_pcm_format_mask_t * mask,
			  snd_pcm_format_t val)
{
  printf ("snd_pcm_format_mask_test()\n");
  return 0;
}

/**
 * \brief Return PCM handle related to an async handler
 * \param handler Async handler handle
 * \return PCM handle
 */
snd_pcm_t *
snd_async_handler_get_pcm (snd_async_handler_t * handler)
{
  dbg_printf ("snd_async_handler_get_pcm()\n");
  return NULL;
}

/**
 * \brief Copy one or more areas
 * \param dst_areas destination areas specification (one for each channel)
 * \param dst_offset offset in frames inside destination area
 * \param src_areas source areas specification (one for each channel)
 * \param src_offset offset in frames inside source area
 * \param channels channels count
 * \param frames frames to copy
 * \param format PCM sample format
 * \return 0 on success otherwise a negative error code
 */
int
snd_pcm_areas_copy (const snd_pcm_channel_area_t * dst_areas,
		    snd_pcm_uframes_t dst_offset,
		    const snd_pcm_channel_area_t * src_areas,
		    snd_pcm_uframes_t src_offset, unsigned int channels,
		    snd_pcm_uframes_t frames, snd_pcm_format_t format)
{
  dbg_printf ("snd_pcm_areas_copy()\n");

  return -EINVAL;
}

/**
 * \brief Get format mask from a configuration space
 * \param params Configuration space
 * \param mask Returned Format mask
 */
void
snd_pcm_hw_params_get_format_mask (snd_pcm_hw_params_t * params,
				   snd_pcm_format_mask_t * mask)
{
  printf ("snd_pcm_hw_params_get_format_mask()\n");
}

#if 1

#ifdef USE_VERSIONED_SYMBOLS

#define OBSOLETE1(name, what, new) \
  symbol_version(__old_##name, name, what); \
  default_symbol_version(__##name, name, new);

#else

#define OBSOLETE1(name, what, new) \
  use_default_symbol_version(__##name, name, new);

#endif /* USE_VERSIONED_SYMBOLS */

#define __P_OLD_GET(pfx, name, val_type, ret_type) \
ret_type pfx##name(const snd_pcm_hw_params_t *params) \
{ \
	val_type val; \
	if (INTERNAL(name)(params, &val) < 0) \
		return 0; \
	return (ret_type)val; \
}

#define __P_OLD_GET1(pfx, name, val_type, ret_type) \
ret_type pfx##name(const snd_pcm_hw_params_t *params, int *dir) \
{ \
	val_type val; \
	if (INTERNAL(name)(params, &val, dir) < 0) \
		return 0; \
	return (ret_type)val; \
}

#define __OLD_GET(name, val_type, ret_type) __P_OLD_GET(__old_, name, val_type, ret_type)
#define __OLD_GET1(name, val_type, ret_type) __P_OLD_GET1(__old_, name, val_type, ret_type)

#define __P_OLD_NEAR(pfx, name, ret_type) \
ret_type pfx##name(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, ret_type val) \
{ \
	if (INTERNAL(name)(pcm, params, &val) < 0) \
		return 0; \
	return (ret_type)val; \
}

#define __P_OLD_NEAR1(pfx, name, ret_type) \
ret_type pfx##name(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, ret_type val, int *dir) \
{ \
	if (INTERNAL(name)(pcm, params, &val, dir) < 0) \
		return 0; \
	return (ret_type)val; \
}

#define __OLD_NEAR(name, ret_type) __P_OLD_NEAR(__old_, name, ret_type)
#define __OLD_NEAR1(name, ret_type) __P_OLD_NEAR1(__old_, name, ret_type)

#define __P_OLD_SET_FL(pfx, name, ret_type) \
ret_type pfx##name(snd_pcm_t *pcm, snd_pcm_hw_params_t *params) \
{ \
	ret_type val; \
	if (INTERNAL(name)(pcm, params, &val) < 0) \
		return 0; \
	return (ret_type)val; \
}

#define __P_OLD_SET_FL1(pfx, name, ret_type) \
ret_type pfx##name(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, int *dir) \
{ \
	ret_type val; \
	if (INTERNAL(name)(pcm, params, &val, dir) < 0) \
		return 0; \
	return (ret_type)val; \
}

#define __OLD_SET_FL(name, ret_type) __P_OLD_SET_FL(__old_, name, ret_type)
#define __OLD_SET_FL1(name, ret_type) __P_OLD_SET_FL1(__old_, name, ret_type)

#define __P_OLD_GET_SW(pfx, name, ret_type) \
ret_type pfx##name(snd_pcm_sw_params_t *params) \
{ \
	ret_type val; \
	if (INTERNAL(name)(params, &val) < 0) \
		return 0; \
	return (ret_type)val; \
}

#define __OLD_GET_SW(name, ret_type) __P_OLD_GET_SW(__old_, name, ret_type)

#endif /* DOC_HIDDEN */

__OLD_NEAR1 (snd_pcm_hw_params_set_rate_near, unsigned int);
OBSOLETE1 (snd_pcm_hw_params_set_rate_near, ALSA_0.9, ALSA_0.9.0rc4);

__OLD_NEAR (snd_pcm_hw_params_set_channels_near, unsigned int);
OBSOLETE1 (snd_pcm_hw_params_set_channels_near, ALSA_0.9, ALSA_0.9.0rc4);

__OLD_GET1 (snd_pcm_hw_params_get_period_size, snd_pcm_uframes_t,
	    snd_pcm_sframes_t);
OBSOLETE1 (snd_pcm_hw_params_get_period_size, ALSA_0.9, ALSA_0.9.0rc4);


__OLD_NEAR1 (snd_pcm_hw_params_set_buffer_time_near, unsigned int);
OBSOLETE1 (snd_pcm_hw_params_set_buffer_time_near, ALSA_0.9,
	   ALSA_0.9.0rc4) __OLD_GET (snd_pcm_hw_params_get_buffer_size,
					snd_pcm_uframes_t, snd_pcm_sframes_t);
OBSOLETE1 (snd_pcm_hw_params_get_buffer_size, ALSA_0.9, ALSA_0.9.0rc4);

__OLD_NEAR1 (snd_pcm_hw_params_set_period_time_near, unsigned int);
OBSOLETE1 (snd_pcm_hw_params_set_period_time_near, ALSA_0.9,
	   ALSA_0.9.0rc4);

__OLD_NEAR (snd_pcm_hw_params_set_buffer_size_near, snd_pcm_uframes_t);
OBSOLETE1 (snd_pcm_hw_params_set_buffer_size_near, ALSA_0.9,
	   ALSA_0.9.0rc4);

__OLD_NEAR1 (snd_pcm_hw_params_set_period_size_near, snd_pcm_uframes_t);
OBSOLETE1 (snd_pcm_hw_params_set_period_size_near, ALSA_0.9,
	   ALSA_0.9.0rc4);

__OLD_NEAR1 (snd_pcm_hw_params_set_periods_near, unsigned int);
OBSOLETE1 (snd_pcm_hw_params_set_periods_near, ALSA_0.9, ALSA_0.9.0rc4);

__OLD_GET1 (snd_pcm_hw_params_get_period_time, unsigned int, int);
OBSOLETE1 (snd_pcm_hw_params_get_period_time, ALSA_0.9, ALSA_0.9.0rc4);

__OLD_GET1 (snd_pcm_hw_params_get_buffer_time, unsigned int, int);
OBSOLETE1 (snd_pcm_hw_params_get_buffer_time, ALSA_0.9, ALSA_0.9.0rc4);

__OLD_GET_SW (snd_pcm_sw_params_get_xfer_align, snd_pcm_uframes_t);
OBSOLETE1 (snd_pcm_sw_params_get_xfer_align, ALSA_0.9, ALSA_0.9.0rc4);

__OLD_GET1 (snd_pcm_hw_params_get_periods, unsigned int, int);
OBSOLETE1 (snd_pcm_hw_params_get_periods, ALSA_0.9, ALSA_0.9.0rc4);

__OLD_GET (snd_pcm_hw_params_get_access, snd_pcm_access_t, int);
OBSOLETE1 (snd_pcm_hw_params_get_access, ALSA_0.9, ALSA_0.9.0rc4);

__OLD_GET1 (snd_pcm_hw_params_get_buffer_time_max, unsigned int,
	    unsigned int);
OBSOLETE1 (snd_pcm_hw_params_get_buffer_time_max, ALSA_0.9,
	   ALSA_0.9.0rc4);

__OLD_GET1 (snd_pcm_hw_params_get_tick_time, unsigned int, int);
OBSOLETE1 (snd_pcm_hw_params_get_tick_time, ALSA_0.9, ALSA_0.9.0rc4);

__OLD_GET (snd_pcm_hw_params_get_channels_max, unsigned int, unsigned int);
OBSOLETE1 (snd_pcm_hw_params_get_channels_max, ALSA_0.9, ALSA_0.9.0rc4);

__OLD_GET (snd_pcm_hw_params_get_channels_min, unsigned int, unsigned int);
OBSOLETE1 (snd_pcm_hw_params_get_channels_min, ALSA_0.9, ALSA_0.9.0rc4);

__OLD_GET1 (snd_pcm_hw_params_get_rate_max, unsigned int, unsigned int);
OBSOLETE1 (snd_pcm_hw_params_get_rate_max, ALSA_0.9, ALSA_0.9.0rc4);

__OLD_GET1 (snd_pcm_hw_params_get_rate_min, unsigned int, unsigned int);
OBSOLETE1 (snd_pcm_hw_params_get_rate_min, ALSA_0.9, ALSA_0.9.0rc4);

__OLD_GET (snd_pcm_hw_params_get_format, snd_pcm_format_t, int);
OBSOLETE1 (snd_pcm_hw_params_get_format, ALSA_0.9, ALSA_0.9.0rc4);

__OLD_GET1 (snd_pcm_hw_params_get_rate, unsigned int, int);
OBSOLETE1 (snd_pcm_hw_params_get_rate, ALSA_0.9, ALSA_0.9.0rc4);
