/*
 * Purpose: Sample format decode routines for ossplay
 *
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

#include "ossplay_decode.h"
#include "ossplay_wparser.h"

typedef struct cradpcm_values {
  const unsigned char * const * table;

  signed char limit;
  signed char shift;
  signed char step;
  unsigned char ratio;
  unsigned char pred;
}
cradpcm_values_t;

typedef struct fib_values {
  unsigned char pred;
  const signed char * table;
}
fib_values_t;

typedef struct ima_values {
  int channels;
  int16 pred[MAX_CHANNELS];
  int8 index[MAX_CHANNELS];
}
ima_values_t;

#ifdef SRC_SUPPORT
/*
 * For actual use, we can rely on vmix.
 * This is useful for testing though.
 */
#include "../../kernel/framework/audio/oss_grc3.c"

typedef struct grc_values {
  int bits;
  int channels;
  int speed;
  int ospeed;
  int obsize;
  grc3state_t grc[];
} grc_data_t;
static decfunc_t decode_src;
static decfunc_t decode_mono_to_stereo;
static grc_data_t * setup_grc3 (int, int, int, int, int);
#endif

extern int amplification, eflag, force_speed, force_fmt, force_channels;
extern flag int_conv, overwrite, verbose;
extern char audio_devname[32];
extern off_t (*ossplay_lseek) (int, off_t, int);
extern double seek_time;
extern const format_t format_a[];

static void decode_ima (unsigned char *, unsigned char *, ssize_t, int16 *,
                        int8 *, int, int);
static void decode_ima_3bits (unsigned char *, unsigned char *, ssize_t,
                              int16 *, int8 *, int, int);
static decfunc_t decode_24;
static decfunc_t decode_8_to_s16;
static decfunc_t decode_amplify;
static decfunc_t decode_cr;
static decfunc_t decode_double64_be;
static decfunc_t decode_double64_le;
static decfunc_t decode_endian;
static decfunc_t decode_fib;
static decfunc_t decode_float32_be;
static decfunc_t decode_float32_le;
static decfunc_t decode_mac_ima;
static decfunc_t decode_ms_ima;
static decfunc_t decode_ms_adpcm;
static decfunc_t decode_nul;
static decfunc_t decode_raw_ima;

static int32 float32_to_s32 (int, int, int);
static int32 double64_to_s32 (int, int32, int32, int);

static cradpcm_values_t * setup_cr (int, int);
static fib_values_t * setup_fib (int, int);
static decoders_queue_t * setup_normalize (int *, int *, decoders_queue_t *);

static seekfunc_t seek_normal;
static seekfunc_t seek_compressed;

static readfunc_t read_normal;

#ifdef OGG_SUPPORT
static readfunc_t read_ogg;
static seekfunc_t seek_ogg;
#endif

errors_t
decode_sound (dspdev_t * dsp, int fd, big_t filesize, int format,
              int channels, int speed, void * metadata)
{
  decoders_queue_t * dec, * decoders;
  readfunc_t * readf;
  seekfunc_t * seekf;
  int bsize, obsize;
  double constant, total_time;
  errors_t ret = E_DECODE;

  if (force_speed != 0) speed = force_speed;
  if (force_channels != 0) channels = force_channels;
  if (force_fmt != 0) format = force_fmt;
  if ((channels > MAX_CHANNELS) || (channels == 0))
    {
      print_msg (ERRORM, "An unreasonable number of channels (%d), aborting\n",
                 channels);
      return E_DECODE;
    }

  constant = format2bits (format) * speed * channels / 8.0;
  if (constant == 0) return E_DECODE; /* Shouldn't ever happen */
#if 0
  /*
   * There is no reason to use SNDCTL_DSP_GETBLKSIZE in applications like this.
   * Using some fixed local buffer size will work equally well.
   */
  ioctl (dsp->fd, SNDCTL_DSP_GETBLKSIZE, &bsize);
#else
  bsize = PLAYBUF_SIZE;
#endif

  if (filesize < 2) return E_OK;
  decoders = dec = (decoders_queue_t *)ossplay_malloc (sizeof (decoders_queue_t));
  dec->next = NULL;
  dec->flag = 0;
  seekf = seek_normal;
  readf = read_normal;
  if (filesize != BIG_SPECIAL) total_time = filesize / constant;
  else total_time = 0;

  switch (format)
    {
      case AFMT_MS_ADPCM:
        if (metadata == NULL)
          {
            msadpcm_values_t * val =
              (msadpcm_values_t *)ossplay_malloc (sizeof (msadpcm_values_t));

            val->channels = channels;
            if (speed < 22000) val->nBlockAlign = 256;
            else if (speed < 44000) val->nBlockAlign = 512;
            else val->nBlockAlign = 1024;
            val->wSamplesPerBlock = 8 * (val->nBlockAlign - 7 * channels) / (4 * channels) + 2;
            val->wNumCoeff = 7;
            val->coeff[0].coeff1 = 256; val->coeff[0].coeff2 = 0;
            val->coeff[1].coeff1 = 512; val->coeff[1].coeff2 = -256;
            val->coeff[2].coeff1 = 0; val->coeff[2].coeff2 = 0;
            val->coeff[3].coeff1 = 192; val->coeff[3].coeff2 = 64;
            val->coeff[4].coeff1 = 240; val->coeff[4].coeff2 = 0;
            val->coeff[5].coeff1 = 460; val->coeff[5].coeff2 = -208;
            val->coeff[6].coeff1 = 392; val->coeff[6].coeff2 = -232;

            /* total_time = val->wSamplesPerBlock * filesize / val->nBlockAlign / constant; */
            bsize = val->nBlockAlign;
            total_time = 0;
            dec->metadata = (void *)val;
            dec->flag = FREE_META;
          }
        else
          {
            msadpcm_values_t * val = (msadpcm_values_t *)metadata;

            /* Let's try anyway */
            if (val->nBlockAlign == 0)
              {
                val->nBlockAlign = filesize - filesize % 4;
              }
            else
              {
                total_time = val->wSamplesPerBlock * filesize * channels /
                             val->nBlockAlign / constant / 2; /* 4/8 == 1/2 */
              }
            bsize = val->nBlockAlign;
            dec->metadata = metadata;
          }

        dec->decoder = decode_ms_adpcm;
        obsize = 4 * bsize;
        dec->outbuf = (unsigned char *)ossplay_malloc (obsize);
        dec->flag |= FREE_OBUF;
        seekf = seek_compressed;

        format = AFMT_S16_NE;
        break;
      case AFMT_MS_IMA_ADPCM:
      case AFMT_MS_IMA_ADPCM_3BITS:
        dec->metadata = metadata;
        if (dec->metadata == NULL)
          {
            msadpcm_values_t * val =
              (msadpcm_values_t *)ossplay_malloc (sizeof (msadpcm_values_t));

            val->channels = channels;
            val->bits = (format == AFMT_MS_IMA_ADPCM)?4:3;
            val->nBlockAlign = 256 * channels * (speed > 11000)?speed/11000:1;
          }
        else
          {
            msadpcm_values_t * val = (msadpcm_values_t *)metadata;

            /* Let's try anyway - some cameras make defective WAVs */
            if (val->nBlockAlign == 0)
              {
                val->nBlockAlign = filesize - filesize % 4;
              }
            else
              {
                total_time = val->wSamplesPerBlock * filesize * val->bits * channels /
                             val->nBlockAlign / constant / 8.0;
              }
            bsize = val->nBlockAlign;
          }

        dec->decoder = decode_ms_ima;
        if (format == AFMT_MS_IMA_ADPCM_3BITS)
          obsize = (bsize * 16)/3 + 2;
         /*
          * 8 sample words per 3 bytes, each expanding to 2 bytes, plus 2 bytes
          * to deal with fractions. Slight overestimation because bsize
          * includes the headers too.
          */
        else
          obsize = 4 * bsize;
        dec->outbuf = (unsigned char *)ossplay_malloc (obsize);
        dec->flag = FREE_OBUF;
        seekf = seek_compressed;

        format = AFMT_S16_NE;
        break;
      case AFMT_MAC_IMA_ADPCM:
        dec->metadata = (void *)(intptr)channels;
        dec->decoder = decode_mac_ima;
        bsize -=  bsize % (MAC_IMA_BLKLEN * channels);
        obsize = 4 * bsize;
        dec->outbuf = (unsigned char *)ossplay_malloc (obsize);
        dec->flag = FREE_OBUF;
        seekf = seek_compressed;

        format = AFMT_S16_NE;
        break;
      case AFMT_IMA_ADPCM:
        dec->metadata = (void *)ossplay_malloc (sizeof (ima_values_t));
        memset (dec->metadata, 0, sizeof (ima_values_t));
        ((ima_values_t *)(dec->metadata))->channels = channels;

        dec->decoder = decode_raw_ima;
        obsize = 4 * bsize;
        dec->outbuf = (unsigned char *)ossplay_malloc (obsize);
        dec->flag = FREE_OBUF | FREE_META;
        seekf = seek_compressed;

        format = AFMT_S16_NE;
        break;
      case AFMT_CR_ADPCM_2:
      case AFMT_CR_ADPCM_3:
      case AFMT_CR_ADPCM_4:
        dec->metadata = (void *)setup_cr (fd, format);;
        if (dec->metadata == NULL) goto exit;
        dec->decoder = decode_cr;
        obsize = ((cradpcm_values_t *)dec->metadata)->ratio * bsize;
        dec->outbuf = (unsigned char *)ossplay_malloc (obsize);
        dec->flag = FREE_OBUF | FREE_META;
        seekf = seek_compressed;

        if (filesize != BIG_SPECIAL) filesize--;
        format = AFMT_U8;
        break;
      case AFMT_FIBO_DELTA:
      case AFMT_EXP_DELTA:
        dec->metadata = (void *)setup_fib (fd, format);;
        if (dec->metadata == NULL) goto exit;
        dec->decoder = decode_fib;
        obsize = 2 * bsize;
        dec->outbuf = (unsigned char *)ossplay_malloc (obsize);
        dec->flag = FREE_OBUF | FREE_META;
        seekf = seek_compressed;

        if (filesize != BIG_SPECIAL) filesize--;
        format = AFMT_U8;
        break;
      case AFMT_S24_PACKED:
      case AFMT_S24_PACKED_BE:
        dec->metadata = (void *)(intptr)format;
        dec->decoder = decode_24;
        bsize -= bsize % 3;
        obsize = bsize/3*4;
        dec->outbuf = (unsigned char *)ossplay_malloc (obsize);
        dec->flag = FREE_OBUF;

        format = AFMT_S32_NE;
        break;
      case AFMT_FLOAT32_BE:
      case AFMT_FLOAT32_LE:
        if (format == AFMT_FLOAT32_BE) dec->decoder = decode_float32_be;
        else dec->decoder = decode_float32_le;
        bsize -= bsize % 4;
        obsize = bsize;
        dec->outbuf = NULL;

        format = AFMT_S32_NE;
        break;
      case AFMT_DOUBLE64_BE:
      case AFMT_DOUBLE64_LE:
        if (format == AFMT_DOUBLE64_BE) dec->decoder = decode_double64_be;
        else dec->decoder = decode_double64_le;
        bsize -= bsize % 8;
        obsize = bsize/2;
        dec->outbuf = NULL;

        format = AFMT_S32_NE;
        break;
#ifdef OGG_SUPPORT
      case AFMT_VORBIS:
        readf = read_ogg;
        dec->decoder = decode_nul;
        dec->metadata = metadata;
        obsize = bsize;
        if (metadata == NULL) goto exit;
        else
          {
            ogg_data_t * val = (ogg_data_t *)metadata;
            if (val->f->ov_seekable (&val->vf))
              {
                seekf = seek_ogg;
                total_time = val->f->ov_time_total (&val->vf, -1);
              }
            else
              {
                seekf = NULL;
                total_time = 0;
              }
          }

        format = AFMT_S16_NE;
        break;
#endif
      default:
        dec->decoder = decode_nul;

        obsize = bsize;
        break;
    }

  if (int_conv)
    decoders = setup_normalize (&format, &obsize, decoders);

  if ((amplification > 0) && (amplification != 100))
    {
      decoders->next =
        (decoders_queue_t *)ossplay_malloc (sizeof (decoders_queue_t));
      decoders = decoders->next;
      decoders->metadata = (void *)(intptr)format;
      decoders->decoder = decode_amplify;
      decoders->next = NULL;
      decoders->outbuf = NULL;
      decoders->flag = 0;
    }

  ret = setup_device (dsp, format, channels, speed);
  if (ret == E_FORMAT_UNSUPPORTED)
    {
      int i, tmp;

      for (i = 0; format_a[i].name != NULL; i++)
        if (format_a[i].fmt == format)
          {
            tmp = format_a[i].may_conv;
            if ((tmp == 0) || (tmp == format)) continue;
            print_msg (WARNM, "Converting to format %s\n",
                       sample_format_name (tmp));
            ret = setup_device (dsp, tmp, channels, speed);
            if (ret == E_FORMAT_UNSUPPORTED) goto exit;
            decoders = setup_normalize (&format, &obsize, decoders);
            goto dcont;
          }
      goto exit;
    }

dcont:
#ifdef SRC_SUPPORT
  if ((ret == E_CHANNELS_UNSUPPORTED) && (channels == 1)) {
    channels = 2;
    if ((ret = setup_device (dsp, format, channels, speed))) goto exit;
    decoders->next =
      (decoders_queue_t *)ossplay_malloc (sizeof (decoders_queue_t));
    decoders = decoders->next;
    decoders->metadata = (void *)(intptr)format;
    decoders->decoder = decode_mono_to_stereo;
    decoders->next = NULL;
    obsize *= 2;
    decoders->outbuf = (unsigned char *)ossplay_malloc (obsize);
    decoders->flag = FREE_OBUF;
  }
#endif

  if (ret) goto exit;
#ifdef SRC_SUPPORT
  if (dsp->speed != speed) {
    if ((format == AFMT_MU_LAW) || (format == AFMT_A_LAW))
      decoders = setup_normalize (&format, &obsize, decoders);
    decoders->next =
      (decoders_queue_t *)ossplay_malloc (sizeof (decoders_queue_t));
    decoders = decoders->next;
    decoders->decoder = decode_src;
    decoders->next = NULL;
    obsize *= (dsp->speed / speed + 1) * channels * sizeof (int);
    decoders->metadata =
      (void *)setup_grc3 (format, channels, dsp->speed, speed, obsize);
    decoders->outbuf = (unsigned char *)ossplay_malloc (obsize);
    decoders->flag = FREE_OBUF | FREE_META;
    speed = dsp->speed;
  }
#endif

  ret = play (dsp, fd, &filesize, bsize, total_time, constant, readf,
              dec, seekf);

exit:
  decoders = dec;
  while (decoders != NULL)
    {
      if (decoders->flag & FREE_META) ossplay_free (decoders->metadata);
      if (decoders->flag & FREE_OBUF) ossplay_free (decoders->outbuf);
      decoders = decoders->next;
      ossplay_free (dec);
      dec = decoders;
    }

  return ret;
}

errors_t
encode_sound (dspdev_t * dsp, fctypes_t type, const char * fname, int format,
              int channels, int speed, double data_time)
{
  big_t data_size = 0;
  double constant;
  int fd = -1;
  decoders_queue_t * dec, * decoders = NULL;
  errors_t ret;
  FILE * wave_fp;

  if ((ret = setup_device (dsp, format, channels, speed))) return ret;
  constant = format2bits (format) * speed * channels / 8.0;

  if (data_time != 0) data_size = data_time * constant;

  if (strcmp (fname, "-") == 0) {
    wave_fp = fdopen (1, "wb");
  } else {
    fd = open (fname, O_WRONLY | O_CREAT | (overwrite?O_TRUNC:O_EXCL), 0644);
    if (fd == -1) {
      perror (fname);
      return E_ENCODE;
    }
    wave_fp = fdopen (fd, "wb");
  }

  if (wave_fp == NULL)
    {
      perror (fname);
      if (fd != -1) close (fd);
      return E_ENCODE;
    }

  if (channels == 1)
    print_msg (VERBOSEM, "Recording wav: Speed %dHz %d bits Mono\n",
               speed, (int)format2bits (format));
  if (channels == 2)
    print_msg (VERBOSEM, "Recording wav: Speed %dHz %d bits Stereo\n",
               speed, (int)format2bits (format));
  if (channels > 2)
    print_msg (VERBOSEM, "Recording wav: Speed %dHz %d bits %d channels\n",
               speed, (int)format2bits (format), channels);

  /*
   * Write the initial header
   */
  if (write_head (wave_fp, type, data_size, format, channels, speed))
    return E_ENCODE;

  decoders = dec =
    (decoders_queue_t *)ossplay_malloc (sizeof (decoders_queue_t));
  dec->next = NULL;
  dec->flag = 0;
  dec->decoder = decode_nul;

  if ((amplification > 0) && (amplification != 100))
    {
      decoders->next =
        (decoders_queue_t *)ossplay_malloc (sizeof (decoders_queue_t));
      decoders = decoders->next;
      decoders->metadata = (void *)(intptr)format;
      decoders->decoder = decode_amplify;
      decoders->next = NULL;
      decoders->outbuf = NULL;
      decoders->flag = 0;
    }

  ret = record (dsp, wave_fp, fname, constant, data_time, &data_size, dec);

  finalize_head (wave_fp, type, data_size, format, channels, speed);
  fflush (wave_fp);
  /*
   * EINVAL and EROFS are returned for "special files which don't support
   * syncronization". The user should already know he's writing to a special
   * file (e.g. "ossrecord /dev/null"), so no need to warn.
   */
  if ((fsync (fileno (wave_fp)) == -1) && (errno != EINVAL) && (errno != EROFS))
    {
      perror (fname);
      ret = E_ENCODE;
    }
  if (fclose (wave_fp) != 0)
    {
      perror (fname);
      ret = E_ENCODE;
    }

  decoders = dec;
  while (decoders != NULL)
    {
      if (decoders->flag & FREE_META) ossplay_free (decoders->metadata);
      if (decoders->flag & FREE_OBUF) ossplay_free (decoders->outbuf);
      decoders = decoders->next;
      ossplay_free (dec);
      dec = decoders;
    }
  return ret;
}

static ssize_t
decode_24 (unsigned char ** obuf, unsigned char * buf,
           ssize_t l, void * metadata)
{
  big_t outlen = 0;
  ssize_t i;
  int v1;
  uint32 * u32;
  int32 sample_s32, * outbuf = (int32 *) * obuf;
  int format = (int)(intptr)metadata;

  if (format == AFMT_S24_PACKED) v1 = 8;
  else v1 = 24;

  for (i = 0; i < l-2; i += 3)
    {
      u32 = (uint32 *) &sample_s32;	/* Alias */

      *u32 = (buf[i] << v1) | (buf[i + 1] << 16) | (buf[i + 2] << (32-v1));
      outbuf[outlen++] = sample_s32;
    }

  return 4 * outlen;
}

static fib_values_t *
setup_fib (int fd, int format)
{
  static const signed char CodeToDelta[16] = {
    -34, -21, -13, -8, -5, -3, -2, -1, 0, 1, 2, 3, 5, 8, 13, 21
  };
  static const signed char CodeToExpDelta[16] = {
    -128, -64, -32, -16, -8, -4, -2, -1, 0, 1, 2, 4, 8, 16, 32, 64
  };
  unsigned char buf;
  fib_values_t * val;

  if (read (fd, &buf, 1) <= 0) return NULL;
  val = (fib_values_t *)ossplay_malloc (sizeof (fib_values_t));
  if (format == AFMT_EXP_DELTA) val->table = CodeToExpDelta;
  else val->table = CodeToDelta;

  val->pred = buf;

  return val;
}

static cradpcm_values_t *
setup_cr (int fd, int format)
{
  static const unsigned char T2[4][3] = {
    { 128, 6, 1 },
    { 32,  4, 1 },
    { 8,   2, 1 },
    { 2,   0, 1 }
  };

  static const unsigned char T3[3][3] = {
    { 128, 5, 3 },
    { 16,  2, 3 },
    { 2,   0, 1 }
  };

  static const unsigned char T4[2][3] = {
    { 128, 4, 7 },
    { 8,   0, 7 }
  };

  static const unsigned char * t_row[4];

  unsigned char buf;
  cradpcm_values_t * val;
  int i;

  if (read (fd, &buf, 1) <= 0) return NULL;
  val = (cradpcm_values_t *)ossplay_malloc (sizeof (cradpcm_values_t));
  val->table = t_row;

  if (format == AFMT_CR_ADPCM_2)
    {
      val->limit = 1;
      val->step = val->shift = 2;
      val->ratio = 4;
      for (i=0; i < 4; i++) t_row[i] = T2[i];
    }
  else if (format == AFMT_CR_ADPCM_3)
    {
      val->limit = 3;
      val->ratio = 3;
      val->step = val->shift = 0;
      for (i=0; i < 3; i++) t_row[i] = T3[i];
    }
  else /* if (format == AFMT_CR_ADPCM_4) */
    {
      val->limit = 5;
      val->ratio = 2;
      val->step = val->shift = 0;
      for (i=0; i < 2; i++) t_row[i] = T4[i];
    }

  val->pred = buf;

  return val;
}

static ssize_t
decode_8_to_s16 (unsigned char ** obuf, unsigned char * buf,
                 ssize_t l, void * metadata)
{
  int format = (int)(intptr)metadata;
  ssize_t i;
  int16 * outbuf = (int16 *) * obuf;
  static const int16 mu_law_table[256] = {
    -32124,-31100,-30076,-29052,-28028,-27004,-25980,-24956,
    -23932,-22908,-21884,-20860,-19836,-18812,-17788,-16764,
    -15996,-15484,-14972,-14460,-13948,-13436,-12924,-12412,
    -11900,-11388,-10876,-10364,-9852, -9340, -8828, -8316,
    -7932, -7676, -7420, -7164, -6908, -6652, -6396, -6140,
    -5884, -5628, -5372, -5116, -4860, -4604, -4348, -4092,
    -3900, -3772, -3644, -3516, -3388, -3260, -3132, -3004,
    -2876, -2748, -2620, -2492, -2364, -2236, -2108, -1980,
    -1884, -1820, -1756, -1692, -1628, -1564, -1500, -1436,
    -1372, -1308, -1244, -1180, -1116, -1052, -988,  -924,
    -876,  -844,  -812,  -780,  -748,  -716,  -684,  -652,
    -620,  -588,  -556,  -524,  -492,  -460,  -428,  -396,
    -372,  -356,  -340,  -324,  -308,  -292,  -276,  -260,
    -244,  -228,  -212,  -196,  -180,  -164,  -148,  -132,
    -120,  -112,  -104,  -96,   -88,   -80,   -72,   -64,
    -56,   -48,   -40,   -32,   -24,   -16,   -8,     0,
    32124, 31100, 30076, 29052, 28028, 27004, 25980, 24956,
    23932, 22908, 21884, 20860, 19836, 18812, 17788, 16764,
    15996, 15484, 14972, 14460, 13948, 13436, 12924, 12412,
    11900, 11388, 10876, 10364, 9852,  9340,  8828,  8316,
    7932,  7676,  7420,  7164,  6908,  6652,  6396,  6140,
    5884,  5628,  5372,  5116,  4860,  4604,  4348,  4092,
    3900,  3772,  3644,  3516,  3388,  3260,  3132,  3004,
    2876,  2748,  2620,  2492,  2364,  2236,  2108,  1980,
    1884,  1820,  1756,  1692,  1628,  1564,  1500,  1436,
    1372,  1308,  1244,  1180,  1116,  1052,  988,   924,
    876,   844,   812,   780,   748,   716,   684,   652,
    620,   588,   556,   524,   492,   460,   428,   396,
    372,   356,   340,   324,   308,   292,   276,   260,
    244,   228,   212,   196,   180,   164,   148,   132,
    120,   112,   104,   96,    88,    80,    72,    64,
    56,    48,    40,    32,    24,    16,    8,     0
  };

  static const int16 a_law_table[256] = {
    -5504, -5248, -6016, -5760, -4480, -4224, -4992, -4736,
    -7552, -7296, -8064, -7808, -6528, -6272, -7040, -6784,
    -2752, -2624, -3008, -2880, -2240, -2112, -2496, -2368,
    -3776, -3648, -4032, -3904, -3264, -3136, -3520, -3392,
    -22016,-20992,-24064,-23040,-17920,-16896,-19968,-18944,
    -30208,-29184,-32256,-31232,-26112,-25088,-28160,-27136,
    -11008,-10496,-12032,-11520,-8960, -8448, -9984, -9472,
    -15104,-14592,-16128,-15616,-13056,-12544,-14080,-13568,
    -344,  -328,  -376,  -360,  -280,  -264,  -312,  -296,
    -472,  -456,  -504,  -488,  -408,  -392,  -440,  -424,
    -88,   -72,   -120,  -104,  -24,   -8,    -56,   -40,
    -216,  -200,  -248,  -232,  -152,  -136,  -184,  -168,
    -1376, -1312, -1504, -1440, -1120, -1056, -1248, -1184,
    -1888, -1824, -2016, -1952, -1632, -1568, -1760, -1696,
    -688,  -656,  -752,  -720,  -560,  -528,  -624,  -592,
    -944,  -912,  -1008, -976,  -816,  -784,  -880,  -848,
    5504,  5248,  6016,  5760,  4480,  4224,  4992,  4736,
    7552,  7296,  8064,  7808,  6528,  6272,  7040,  6784,
    2752,  2624,  3008,  2880,  2240,  2112,  2496,  2368,
    3776,  3648,  4032,  3904,  3264,  3136,  3520,  3392,
    22016, 20992, 24064, 23040, 17920, 16896, 19968, 18944,
    30208, 29184, 32256, 31232, 26112, 25088, 28160, 27136,
    11008, 10496, 12032, 11520, 8960,  8448,  9984,  9472,
    15104, 14592, 16128, 15616, 13056, 12544, 14080, 13568,
    344,   328,   376,   360,   280,   264,   312,   296,
    472,   456,   504,   488,   408,   392,   440,   424,
    88,    72,    120,   104,   24,    8,     56,    40,
    216,   200,   248,   232,   152,   136,   184,   168,
    1376,  1312,  1504,  1440,  1120,  1056,  1248,  1184,
    1888,  1824,  2016,  1952,  1632,  1568,  1760,  1696,
    688,   656,   752,   720,   560,   528,   624,   592,
    944,   912,   1008,  976,   816,   784,   880,   848
  };

  switch (format)
    {
      case AFMT_U8:
        for (i = 0; i < l; i++) outbuf[i] = (buf[i] - 128) << 8;
        break;
      case AFMT_S8:
        for (i = 0; i < l; i++) outbuf[i] = buf[i] << 8;
        break;
      case AFMT_MU_LAW:
        for (i = 0; i < l; i++) outbuf[i] = mu_law_table[buf[i]];
        break;
      case AFMT_A_LAW:
        for (i = 0; i < l; i++) outbuf[i] = a_law_table[buf[i]];
        break;
    }

  return 2*l;
}

static ssize_t
decode_cr (unsigned char ** obuf, unsigned char * buf,
           ssize_t l, void * metadata)
{
  cradpcm_values_t * val = (cradpcm_values_t *) metadata;
  int j, pred = val->pred, step = val->step;
  unsigned char value;
  signed char sign;
  ssize_t i;

  for (i=0; i < l; i++)
    for (j=0; j < val->ratio; j++)
      {
        sign = (buf[i] & val->table[j][0])?-1:1;
        value = (buf[i] >> val->table[j][1]) & val->table[j][2];
        pred += sign*(value << step);
        if (pred > 255) pred = 255;
        else if (pred < 0) pred = 0;
        (*obuf)[val->ratio*i+j] = pred;
        if ((value >= val->limit) && (step < 3+val->shift)) step++;
        if ((value == 0) && (step > val->shift)) step--;
      }

  val->pred = pred;
  val->step = step;
  return val->ratio*l;
}

static ssize_t
decode_fib (unsigned char ** obuf, unsigned char * buf,
            ssize_t l, void * metadata)
{
  fib_values_t * val = (fib_values_t *)metadata;
  int x = val->pred;
  unsigned char d;
  ssize_t i;

  for (i = 0; i < 2*l; i++)
    {
      d = buf[i/2];
      if (i & 1) d &= 0xF;
      else d >>= 4;
      x += val->table[d];
      if (x > 255) x = 255;
      if (x < 0) x = 0;
      (*obuf)[i] = x;
    }

  val->pred = x;
  return 2*l;
}

static ssize_t
decode_ms_adpcm (unsigned char ** obuf, unsigned char * buf,
                 ssize_t l, void * metadata)
{
  msadpcm_values_t * val = (msadpcm_values_t *)metadata;

  int error_delta, i_delta, i = 0, nib = 0, channels = val->channels;
  int AdaptionTable[16] = {
    230, 230, 230, 230, 307, 409, 512, 614,
    768, 614, 512, 409, 307, 230, 230, 230
  };
  ssize_t outp = 0, x = 0;
  int16 * wbuf = (int16 *)*obuf;
  int32 delta[MAX_CHANNELS], samp1[MAX_CHANNELS], samp2[MAX_CHANNELS],
        predictor[MAX_CHANNELS], new_samp, pred, n = 0;

/*
 * Playback procedure
 */
#define OUT_SAMPLE(s) \
 do { \
   if (s > 32767) s = 32767; else if (s < -32768) s = -32768; \
   wbuf[outp++] = s; \
   n += 2; \
 } while(0)

#define GETNIBBLE \
        ((nib == 0) ? \
                (buf[x + nib++] >> 4) & 0x0f : \
                buf[x++ + --nib] & 0x0f \
	)

  for (i = 0; i < channels; i++)
    {
      predictor[i] = buf[x];
      if (predictor[i] > val->wNumCoeff)
        /* Shouldn't ever happen */
        predictor[i] = val->wNumCoeff;
      x++;
    }

  for (i = 0; i < channels; i++)
    {
      delta[i] = (int16) le_int (&buf[x], 2);
      x += 2;
    }

  for (i = 0; i < channels; i++)
    {
      samp1[i] = (int16) le_int (&buf[x], 2);
      x += 2;
      OUT_SAMPLE (samp1[i]);
    }

  for (i = 0; i < channels; i++)
    {
      samp2[i] = (int16) le_int (&buf[x], 2);
      x += 2;
      OUT_SAMPLE (samp2[i]);
    }

  while (n < (val->wSamplesPerBlock * 2 * channels))
    for (i = 0; i < channels; i++)
      {
        pred = ((samp1[i] * val->coeff[predictor[i]].coeff1)
                + (samp2[i] * val->coeff[predictor[i]].coeff2)) / 256;

        if (x > l) return 2*outp;
        i_delta = error_delta = GETNIBBLE;

        if (i_delta & 0x08)
        i_delta -= 0x10;	/* Convert to signed */

        new_samp = pred + (delta[i] * i_delta);
        OUT_SAMPLE (new_samp);

        delta[i] = delta[i] * AdaptionTable[error_delta] / 256;
        if (delta[i] < 16) delta[i] = 16;

        samp2[i] = samp1[i];
        samp1[i] = new_samp;
      }

  return 2*outp;
}

static ssize_t
decode_nul (unsigned char ** obuf, unsigned char * buf,
            ssize_t l, void * metadata)
{
  *obuf = buf;
  return l;
}

static ssize_t
decode_endian (unsigned char ** obuf, unsigned char * buf,
               ssize_t l, void * metadata)
{
  int format = (int)(intptr)metadata;
  ssize_t i;

  switch (format)
    {
      case AFMT_S16_OE:
        {
          int16 * s = (int16 *)buf;

          for (i = 0; i < l / 2; i++)
            s[i] = ((s[i] >> 8) & 0x00FF) |
                   ((s[i] << 8) & 0xFF00);
        }
        break;
      case AFMT_S32_OE:
      case AFMT_S24_OE:
        {
          int32 * s = (int32 *)buf;

          for (i = 0; i < l / 4; i++)
            s[i] = ((s[i] >> 24) & 0x000000FF) |
                   ((s[i] << 8) & 0x00FF0000) | ((s[i] >> 8) & 0x0000FF00) |
                   ((s[i] << 24) & 0xFF000000);
        }
        break;
#ifdef OSS_LITTLE_ENDIAN
      case AFMT_U16_BE: /* U16_BE -> S16_LE */
#else
      case AFMT_U16_LE: /* U16_LE -> S16_BE */
#endif
        {
          int16 * s = (int16 *)buf;

          for (i = 0; i < l / 2; i++)
            s[i] = (((s[i] >> 8) & 0x00FF) | ((s[i] << 8) & 0xFF00)) -
                   USHRT_MAX/2;
        }
      break;
 /* Not an endian conversion, but included for completeness sake */
#ifdef OSS_LITTLE_ENDIAN
      case AFMT_U16_LE: /* U16_LE -> S16_LE */
#else
      case AFMT_U16_BE: /* U16_BE -> S16_BE */
#endif
       {
          int16 * s = (int16 *)buf;

          for (i = 0; i < l / 2; i++)
             s[i] -= USHRT_MAX/2;
       }
      break;
    }
  *obuf = buf;
  return l;
}

static ssize_t
decode_amplify (unsigned char ** obuf, unsigned char * buf,
                ssize_t l, void * metadata)
{
  int format = (int)(intptr)metadata;
  ssize_t i, len;

  switch (format)
    {
      case AFMT_S16_NE:
        {
          int16 *s = (int16 *)buf;
          int32 tmp;

          len = l / 2;
          for (i = 0; i < len ; i++)
            {
              tmp = (int32)s[i] * amplification / 100;
              if (tmp > SHRT_MAX) s[i] = SHRT_MAX;
              else if (tmp < SHRT_MIN) s[i] = SHRT_MIN;
              else s[i] = tmp;
            }
        }
        break;
      case AFMT_S32_NE:
      case AFMT_S24_NE:
        {
          int32 *s = (int32 *)buf;
          sbig_t tmp;

          len = l / 4;
          for (i = 0; i < len; i++)
            {
              tmp = (sbig_t)s[i] * amplification / 100;
              if (tmp > S32_MAX) s[i] = S32_MAX;
              else if (tmp < S32_MIN) s[i] = S32_MIN;
              else s[i] = tmp;
            }
        }
       break;
   }

  *obuf = buf;
  return l;
}

static void
decode_ima (unsigned char * obuf, unsigned char * buf, ssize_t l, int16 * pred0,
            int8 * index0, int channels, int ch)
{
  int j;
  int32 pred = *pred0;
  int16 step;
  int16 * outbuf = (int16 *) obuf;
  int8 index = *index0, value;
  signed char sign;
  ssize_t i;
  static const int step_tab[89] = {
    7,     8,     9,     10,    11,    12,    13,    14,
    16,    17,    19,    21,    23,    25,    28,    31,
    34,    37,    41,    45,    50,    55,    60,    66,
    73,    80,    88,    97,    107,   118,   130,   143,
    157,   173,   190,   209,   230,   253,   279,   307,
    337,   371,   408,   449,   494,   544,   598,   658,
    724,   796,   876,   963,   1060,  1166,  1282,  1411,
    1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,
    3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,
    7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767
  };

  static const int8 iTab4[16] =
    {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};

  for (i=0; i < l; i++)
    for (j=0; j < 2; j++)
      {
        value = (buf[i] >> 4*j) & 15;

        step = step_tab[index];
        index += iTab4[value];
        if (index < 0) index = 0;
        else if (index > 88) index = 88;

        sign = 1 - 2 * ((value >> 3) & 1);
        value &= 7;

        pred += sign * (2 * value + 1) * step / 4;
        if (pred > 32767) pred = 32767;
        else if (pred < -32768) pred = -32768;

        outbuf[channels*(2*i+j)+ch] = pred;
      }

  *index0 = index;
  *pred0 = pred;

  return;
}

static void
decode_ima_3bits (unsigned char * obuf, unsigned char * buf, ssize_t l,
                  int16 * pred0, int8 * index0, int channels, int ch)
{
  int j;
  signed char sign;
  ssize_t i;

  int32 pred = *pred0, raw;
  int8 index = *index0, value;
  int16 * outbuf = (int16 *) obuf, step;

  static const int step_tab[89] = {
    7,     8,     9,     10,    11,    12,    13,    14,
    16,    17,    19,    21,    23,    25,    28,    31,
    34,    37,    41,    45,    50,    55,    60,    66,
    73,    80,    88,    97,    107,   118,   130,   143,
    157,   173,   190,   209,   230,   253,   279,   307,
    337,   371,   408,   449,   494,   544,   598,   658,
    724,   796,   876,   963,   1060,  1166,  1282,  1411,
    1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,
    3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,
    7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
    32767
  };

  static const int8 iTab3[8] =
    {-1, -1, 1, 2, -1, -1, 1, 2};

  for (i=0; i < l-2; i += 3)
    {
      raw = buf[i] + (buf[i+1] << 8) + (buf[i+2] << 16);
      for (j = 0; j < 8; j++)
        {
          value = (raw >> (3*j)) & 7;

          step = step_tab[index];
          index += iTab3[value];
          if (index < 0) index = 0;
          else if (index > 88) index = 88;

          sign = 1 - 2 * ((value >> 2) & 1);
          value &= 3;

          pred += sign * (2 * value + 1) * step / 4;
          if (pred > 32767) pred = 32767;
          else if (pred < -32768) pred = -32768;

          outbuf[channels*(8*i/3+j)+ch] = pred;
        }
    }

  *index0 = index;
  *pred0 = pred;

  return;
}

static ssize_t
decode_mac_ima (unsigned char ** obuf, unsigned char * buf,
                ssize_t l, void * metadata)
{
  ssize_t len = 0, olen = 0;
  int i, channels = (int)(intptr)metadata;
  int16 pred;
  int8 index;

  while (len < l)
    {
      for (i = 0; i < channels; i++)
        {
          if (len + MAC_IMA_BLKLEN > l) return olen;
          pred = (int16)((buf[len] << 8) | (buf[len+1] & 128));
          index = buf[len+1] & 127;
          if (index > 88) index = 88;
          len += 2;

          decode_ima (*obuf + olen, buf + len, MAC_IMA_BLKLEN - 2, &pred,
                      &index, channels, i);
          len += MAC_IMA_BLKLEN-2;
        }
      olen += 4*(MAC_IMA_BLKLEN - 2)*channels;
    }

  return olen;
}

static ssize_t
decode_ms_ima (unsigned char ** obuf, unsigned char * buf,
                ssize_t l, void * metadata)
{
  int i;
  ssize_t len = 0, olen = 0;
  msadpcm_values_t * val = (msadpcm_values_t *)metadata;
  int8 index[MAX_CHANNELS];
  int16 * outbuf = (int16 *) * obuf, pred[MAX_CHANNELS];

  for (i = 0; i < val->channels; i++)
    {
      if (len >= l) return olen;
      pred[i] = (int16) le_int (buf + len, 2);
 /*
  * The microsoft docs says the sample from the block header should be
  * played.
  */
      outbuf[i] = pred[i];
      olen += 2;
      index[i] = buf[len + 2];
      if (index[i] > 88) index[i] = 88;
      if (index[i] < 0) index[i] = 0;
      len += 4;
    }

  if (val->bits == 4)
    while (len < l)
      {
        for (i = 0; i < val->channels; i++)
          {
            if (len + 4 > l) return olen;
            decode_ima (*obuf + olen, buf + len, 4, &pred[i], &index[i],
                        val->channels, i);
            len += 4;
          }
        olen += 2*8*val->channels;
      }
  else
    {
      unsigned char rbuf[12];
      int j;

      while (len < l)
        {
          if (len + 12*val->channels > l) return olen;
          for (i = 0; i < val->channels; i++)
            {
            /*
             * Each sample word for a channel in an IMA ADPCM RIFF file is 4
             * bits. This doesn't resolve to an integral number of samples
             * in a 3 bit ADPCM, so we use a simple method around this.
             * This shouldn't skip samples since the spec guarantees the
             * number of sample words in a block is divisible by 3.
             */
              for (j = 0; j < 12; j++)
                rbuf[j] = buf[len + j%4 + (j/4)*(val->channels*4) + i*4];
              decode_ima_3bits (*obuf + olen, rbuf, 12, &pred[i], &index[i],
                                val->channels, i);
            }
          /* 12 = 3 words per channel, each containing 4 bytes */
          len += 12*val->channels;
          /* 64 = 32 samples per channel, each expanding to 2 bytes */
          olen += 64*val->channels;
        }
    }
  return olen;
}

static ssize_t
decode_raw_ima (unsigned char ** obuf, unsigned char * buf,
                ssize_t l, void * metadata)
{
  ima_values_t * val = (ima_values_t *)metadata;
  ssize_t olen = 0;

  /* We can't tell if/how it's interleaved. */
  decode_ima (*obuf, buf, l, &val->pred[0], &val->index[0], 1, 0);
  olen = 4*l;

  return olen;
}

static ssize_t
decode_float32_be (unsigned char ** obuf, unsigned char * buf, ssize_t l,
                   void * metadata)
{
  ssize_t i;
  int exp, man;
  int32 * wbuf = (int32 *) buf;

  for (i=0; i < l-3; i += 4)
    {
      exp = ((buf[i] & 0x7F) << 1) | ((buf[i+1] & 0x80) / 0x80);
      man = ((buf[i+1] & 0x7F) << 16) | (buf[i+2] << 8) | buf[i+3];

      *wbuf++ = float32_to_s32 (exp, man, (buf[i] & 0x80));
    }

  *obuf = buf;
  return l;
}

static ssize_t
decode_float32_le (unsigned char ** obuf, unsigned char * buf, ssize_t l,
                   void * metadata)
{
  ssize_t i;
  int exp, man;
  int32 * wbuf = (int32 *) buf;

  for (i=0; i < l-3; i += 4)
    {
      exp = ((buf[i+3] & 0x7F) << 1) | ((buf[i+2] & 0x80) / 0x80);
      man = ((buf[i+2] & 0x7F) << 16) | (buf[i+1] << 8) | buf[i];

      *wbuf++ = float32_to_s32 (exp, man, (buf[i+3] & 0x80));
    }

  *obuf = buf;
  return l;
}

static ssize_t
decode_double64_be (unsigned char ** obuf, unsigned char * buf, ssize_t l,
                    void * metadata)
{
  ssize_t i;
  int exp;
  int32 * wbuf = (int32 *) buf, lower, upper;

  for (i=0; i < l-7; i += 8)
    {
      exp = ((buf[i] & 0x7F) << 4) | ((buf[i+1] >> 4) & 0xF) ;

      upper = ((buf[i+1] & 0xF) << 24) | (buf[i+2] << 16) | (buf[i+3] << 8) |
              buf[i+4];
      lower = (buf[i+5] << 16) | (buf[i+6] << 8) | buf[i+7];

      *wbuf++ = double64_to_s32 (exp, upper, lower, buf[i] & 0x80);
    }

  *obuf = buf;
  return l/2;
}

static ssize_t
decode_double64_le (unsigned char ** obuf, unsigned char * buf, ssize_t l,
                    void * metadata)
{
  ssize_t i;
  int exp;
  int32 * wbuf = (int32 *) buf, lower, upper;

  for (i=0; i < l-7; i += 8)
    {
      exp = ((buf[i+7] & 0x7F) << 4) | ((buf[i+6] >> 4) & 0xF);

      upper = ((buf[i+6] & 0xF) << 24) | (buf[i+5] << 16) | (buf[i+4] << 8) |
              buf[i+3];
      lower = (buf[i+2] << 16) | (buf[i+1] << 8) | buf[i];

      *wbuf++ = double64_to_s32 (exp, upper, lower, buf[i+7] & 0x80);
    }

  *obuf = buf;
  return l/2;
}

static int32
double64_to_s32 (int exp, int32 upper, int32 lower, int sign)
{
  ldouble_t out, value;

  if ((exp != 0) && (exp != 2047))
    {
      value = (upper + lower / ((double)0x1000000))/((double)0x10000000) + 1;
      value = ossplay_ldexpl (value, exp - 1023);
    }
  else if (exp == 0)
    {
#if 0
      int j;

      out = (upper + lower / ((double)0x1000000))/((double)0x10000000);
      for (j=0; j < 73; j++) out /= 1 << 14;
#endif
      /* So low, that it's pretty much 0 for us */
      return 0;
    }
  else /* exp == 2047 */
    {
      /*
       * Either NaN, or +/- Inf. 0 is almost as close an approximation of
       * Inf as the maximum sample value....
       */
      print_msg (WARNM, "exp == 2047 in file!\n");
      return 0;
    }

  out = (sign ? 1 : -1) * value * S32_MIN;
  if (out > S32_MAX) out = S32_MAX;
  else if (out < S32_MIN) out = S32_MIN;

  return out;
}

static int32
float32_to_s32 (int exp, int man, int sign)
{
  ldouble_t out, value;

  if ((exp != 0) && (exp != 255))
    {
      value = man ? (float)man/(float)0x800000 + 1 : 0.0;
      value = ossplay_ldexpl (value, exp - 127);
    }
  else if (exp == 0)
    {
#if 0
      value = (float)man / (float)0x800000;
      value /= 1UL << 31; value /= 1UL << 31; value /= 1UL << 32;
      value /= 1UL << 32;
#endif
      /* So low, that it's pretty much 0 for us */
      return 0;
    }
  else /* exp == 255 */
    {
      /*
       * Either NaN, or +/- Inf. 0 is almost as close an approximation of
       * Inf as the maximum sample value....
       */
      print_msg (WARNM, "exp == 255 in file!\n");
      return 0;
    }

  out = (sign ? 1 : -1) * value * S32_MIN;
  if (out > S32_MAX) out = S32_MAX;
  else if (out < S32_MIN) out = S32_MIN;

  return out;
}

int
get_db_level (const unsigned char * buf, ssize_t l, int format)
{
/*
 * Display a rough recording level meter, and the elapsed time.
 */
  static const unsigned char db_table[256] = {
  /* Lookup table for log10(ix)*2, ix=0..255 */
    0, 0, 1, 2, 2, 3, 3, 3, 4, 4,
    4, 4, 4, 5, 5, 5, 5, 5, 5, 5,
    5, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
    8, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 11, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11
  };

  int32 level, v = 0;
  ssize_t i;

  level = 0;
  if ((buf == NULL) || (l == 0)) return 0;

  switch (format)
    {
      case AFMT_U8:
        {
          uint8 * p;

          p = (uint8 *)buf;

          for (i = 0; i < l; i++) {
            v = (*p++);
            if (v > level) level = v;
          }
        }
      case AFMT_S8:
        {
          int8 * p;

          p = (int8 *)buf;

          for (i = 0; i < l; i++) {
            v = *p++;
            if (v < 0) {
              /* This can be false on a two's-complement machine */
              if (v != -v) v = -v;
              else v = -(v+1);
            }
            if (v > level) level = v;
          }
        }
       break;

      case AFMT_S16_NE:
        {
          int16 * p;

          p = (int16 *)buf;

          for (i = 0; i < l / 2; i++) {
            v = *p++;
            if (v < 0) {
              if (v != -v) v = -v;
              else v = -(v+1);
            }
            if (v > level) level = v;
          }
        }
        level >>= 8;
        break;

      case AFMT_S24_NE:
      case AFMT_S32_NE:
        {
          int32 * p;

          p = (int32 *)buf;

          for (i = 0; i < l / 4; i++) {
            v = *p++;
            if (v < 0) {
              if (v != -v) v = -v;
              else v = -(v+1);
            }
            if (v > level) level = v;
          }
        }
        level >>= 24;
        break;
      default: return -1;
    }

  if (level > 255) level = 255;
  v = db_table[level];

  return v;
}

static decoders_queue_t *
setup_normalize (int * format, int * obsize, decoders_queue_t * decoders)
{
  if ((*format == AFMT_S16_OE) || (*format == AFMT_S32_OE) ||
      (*format == AFMT_S24_OE) || (*format == AFMT_U16_LE) ||
      (*format == AFMT_U16_BE))
    {
      decoders->next =
        (decoders_queue_t *)ossplay_malloc (sizeof (decoders_queue_t));
      decoders = decoders->next;
      decoders->decoder = decode_endian;
      decoders->metadata = (void *)(intptr)*format;
      switch (*format)
        {
          case AFMT_S32_OE: *format = AFMT_S32_NE; break;
          case AFMT_S24_OE: *format = AFMT_S24_NE; break;
          default: *format = AFMT_S16_NE; break;
        }
      decoders->next = NULL;
      decoders->outbuf = NULL;
      decoders->flag = 0;
    }
  else if ((*format == AFMT_U8) || (*format == AFMT_MU_LAW) ||
           (*format == AFMT_S8) || (*format == AFMT_A_LAW))
    {
      decoders->next =
        (decoders_queue_t *)ossplay_malloc (sizeof (decoders_queue_t));
      decoders = decoders->next;
      decoders->decoder = decode_8_to_s16;
      decoders->metadata = (void *)(intptr)*format;
      decoders->next = NULL;
      *obsize *= 2;
      decoders->outbuf = (unsigned char *)ossplay_malloc (*obsize);
      decoders->flag = FREE_OBUF;
      *format = AFMT_S16_NE;
    }
  return decoders;
}

verbose_values_t *
setup_verbose (int format, double oconstant, double total_time)
{
  verbose_values_t * val;

  val = (verbose_values_t *)ossplay_malloc (sizeof (verbose_values_t));

  if (total_time == 0)
    {
      val->tsecs = 0;
      strcpy (val->tstring, "unknown");
    }
  else
    {
      char * p;

      val->tsecs = total_time;
      p = totime (val->tsecs);
      strncpy (val->tstring, p, sizeof (val->tstring));
      ossplay_free (p);
      val->tsecs -= UPDATE_EPSILON/1000;
    }

  val->secs = 0;
  val->secs_timer2 = 0;
  val->next_sec = 0;
  val->next_sec_timer2 = 0;
  val->format = format;
  val->constant = oconstant;

  return val;
}

static errors_t
seek_normal (int fd, big_t * datamark, big_t filesize, double constant,
             big_t rsize, int channels, void * metadata)
{
  big_t pos = seek_time * constant;
  int ret;

  pos -= pos % channels;
  if ((pos > filesize) || (pos < *datamark)) return E_DECODE;

  ret = ossplay_lseek (fd, pos - *datamark, SEEK_CUR);
  if (ret == -1)
    {
      seek_time = 0;
      return E_DECODE;
    }
  *datamark = ret;

  return E_OK;
}

static errors_t
seek_compressed (int fd, big_t * datamark, big_t filesize, double constant,
                 big_t rsize, int channels, void * metadata)
/*
 * We have to use this method because some compressed formats depend on the
 * previous state of the decoder, and don't (yet?) have own seek function.
 */
{
  big_t pos = seek_time * constant;

  if (pos > filesize)
    {
      seek_time = 0;
      return E_DECODE;
    }

  if (*datamark + rsize < pos)
    {
      return SEEK_CONT_AFTER_DECODE;
    }
  else
    {
      /* Still not entirely accurate. */
      seek_time = *datamark / constant;
      return E_OK;
    }
}

static ssize_t
read_normal (int fd, void * buf, size_t len, void * metadata)
{
  return read (fd, buf, len);
}

#ifdef OGG_SUPPORT
static errors_t
seek_ogg (int fd, big_t * datamark, big_t filesize, double constant,
          big_t rsize, int channels, void * metadata)
{
  ogg_data_t * val = (ogg_data_t *)metadata;

  if (val->f->ov_time_seek (&val->vf, seek_time) < 0)
    {
      seek_time = 0;
      return E_DECODE;
    }
  *datamark = (big_t)val->f->ov_raw_tell (&val->vf);
  return E_OK;
}

static ssize_t
read_ogg (int fd, void * buf, size_t len, void * metadata)
{
  int c_bitstream;
  ssize_t ret = 0;
  ogg_data_t * val = (ogg_data_t *)metadata;

  if (val->setup == 1)
    {
      vorbis_info * vi;

      vi = val->f->ov_info (&val->vf, -1);

      ret = setup_device (val->dsp, AFMT_S16_NE, vi->channels, vi->rate);
      if (ret < 0) return -1;
      val->setup = 0;
    }

  do
    {
#if 0
      if (ret == OV_HOLE)
          print_msg (NOTIFYM, "Hole in the OggVorbis stream!\n");
#endif
      c_bitstream = val->bitstream;
      ret = (ssize_t)val->f->ov_read (&val->vf, (char *)buf, (int)len,
#ifdef OSS_LITTLE_ENDIAN
                     0,
#else
                     1,
#endif
                     2, 1, &val->bitstream);
    }
  while (ret == OV_HOLE);

  if (ret == 0) return 0;
  else if (ret < 0) return ret;

  if ((c_bitstream != val->bitstream) && (c_bitstream != -1))
    {
      val->bitstream = c_bitstream;
      val->setup = 1;
    }

  return ret;
}
#endif

#ifdef SRC_SUPPORT
#define GRC3_HIGH_QUALITY 4
static ssize_t
decode_mono_to_stereo (unsigned char ** obuf, unsigned char * buf,
                       ssize_t l, void * metadata)
{
  ssize_t i;
  int format = (int)(intptr)metadata;

  switch (format) {
    case AFMT_U8:
    case AFMT_S8: {
         uint8 *r = (uint8 *)buf, *s = (uint8 *)*obuf;
         for (i=0; i < l; i++) {
           *s++ = *r;
           *s++ = *r++;
        }
      }
      break;
    case AFMT_S16_LE:
    case AFMT_S16_BE: {
        int16 *r = (int16 *)buf, *s = (int16 *)*obuf;

        for (i = 0; i < l/2 ; i++) {
          *s++ = *r;
          *s++ = *r++;
        }
      }
      break;
    case AFMT_S32_LE:
    case AFMT_S32_BE:
    case AFMT_S24_LE:
    case AFMT_S24_BE: {
        int32 *r = (int32 *)buf, *s = (int32 *)*obuf;

        for (i = 0; i < l/4; i++) {
          *s++ = *r;
          *s++ = *r++;
        }
      }
      break;
  }
  return 2*l;
}

static ssize_t 
decode_src (unsigned char ** obuf, unsigned char * buf, ssize_t l, void * metadata)
{
  grc_data_t * val = (grc_data_t *)metadata;
  ssize_t outc = 0;
  int i;

  for (i=0; i<val->channels; i++) {
    outc += grc3_convert (&val->grc[i], val->bits, GRC3_HIGH_QUALITY, buf,
                          *obuf, 8 * l / val->channels / val->bits,
                          val->obsize / val->channels / sizeof (int),
                          val->channels, i);
  }

  return outc * val->bits / 8;
}

static grc_data_t *
setup_grc3 (int format, int channels, int speed, int ospeed, int obsize)
{
  int i;
  grc_data_t * val = (grc_data_t *)ossplay_malloc (sizeof (grc_data_t) +
                                     sizeof (grc3state_t) * channels);

  val->bits = format2bits (format);
  val->channels = channels;
  val->speed = speed;
  val->ospeed = ospeed;
  val->obsize = obsize;

  for (i=0; i<channels; i++) {
    grc3_reset (&val->grc[i]);
    grc3_setup (&val->grc[i], ospeed, speed);
  }

  return val;
}
#endif

