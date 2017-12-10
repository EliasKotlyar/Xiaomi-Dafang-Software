/*
 * Purpose: Audio format conversion routines used by audio.c
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

/* SINE_DEBUG enables internal test signal (sine wave) */
#undef  SINE_DEBUG

#include "oss_config.h"
#include "ulaw.h"
#define __int8_t_defined
#include "grc3.h"

/*
 * Audio format conversion stuff 
 */
#define CNV_NONE 	0x00000000
#define CNV_SRC		0x00000001

#define	CNV_CHNMASK	0x000000f0
#define CNV_M2S		0x00000010
#define CNV_S2M		0x00000020
#define CNV_MULT	0x00000040

#define CNV_Flog	0x00001000
#define CNV_F8bit	0x00002000
#define CNV_F16bit	0x00004000
#define CNV_F24bit	0x00008000
#define CNV_F32bit	0x00010000

#define CNV_Tlog	0x00100000
#define CNV_T8bit	0x00200000
#define CNV_T16bit	0x00400000
#define CNV_T24bit	0x00800000
#define CNV_T32bit	0x01000000

#define CNV_SIGN	0x10000000
#define CNV_ENDIAN	0x20000000
#define CNV_TOLOG	0x40000000
#define CNV_FROMLOG	0x80000000

static audio_format_info_t audio_format_info[] = {
  {"mu-Law", AFMT_MU_LAW, 0x80, 8, 0, ENDIAN_NONE, 1},
  {"A-Law", AFMT_A_LAW, 0x80, 8, 0, ENDIAN_NONE, 1},
  {"IMA-ADPCM", AFMT_IMA_ADPCM, 0x00, 4, 0, ENDIAN_NONE, 0, ALIGN_LSB, 1},
  {"8", AFMT_U8, 0x80, 8, 1, ENDIAN_NONE, 0},
  {"16(LE)", AFMT_S16_LE, 0x00, 16, 1, ENDIAN_LITTLE, 1},
  {"16(BE)", AFMT_S16_BE, 0x00, 16, 1, ENDIAN_BIG, 1},
  {"8(signed)", AFMT_S8, 0x00, 8, 1, ENDIAN_NONE, 1},
  {"16(unsigned LE)", AFMT_U16_LE, 0x00, 16, 1, ENDIAN_LITTLE, 0},
  {"16(unsigned BE)", AFMT_U16_BE, 0x00, 16, 1, ENDIAN_BIG, 0},
  {"mp2/mp3", AFMT_MPEG, 0x00, 8, 0, ENDIAN_NONE, 0, ALIGN_LSB, 1},
  {"AC3", AFMT_AC3, 0x00, 16, 0, ENDIAN_NONE, 0, ALIGN_LSB, 1},
  {"SPDIF (RAW)", AFMT_SPDIF_RAW, 0x00, 32, 0, ENDIAN_NONE, 0, ALIGN_LSB, 1},
  {"24(LE)", AFMT_S24_LE, 0x00, 32, 1, ENDIAN_LITTLE, 1, ALIGN_LSB},
  {"24(BE)", AFMT_S24_BE, 0x00, 32, 1, ENDIAN_BIG, 1, ALIGN_LSB},
  {"32(LE)", AFMT_S32_LE, 0x00, 32, 1, ENDIAN_LITTLE, 1},
  {"32(BE)", AFMT_S32_BE, 0x00, 32, 1, ENDIAN_BIG, 1},
  {"24(PACKED)", AFMT_S24_PACKED, 0x00, 24, 1, ENDIAN_LITTLE, 1, ALIGN_LSB},
  {0, 0}
};

audio_format_info_p
oss_find_format (unsigned int fmt)
{
  int i;

  i = 0;

  while (audio_format_info[i].fmt != 0)
    {
      if (audio_format_info[i].fmt == fmt)
	return &audio_format_info[i];
      i++;
    }

  return NULL;
}

#if 0
/*
 * Limiter stuff. Currently not in use.
 */
static const unsigned char limit8[] = {
/*-128*/ 64, 65, 65, 66, 66, 67, 67, 68, 68, 69, 69, 70, 70, 71, 71, 72,
/*-112*/ 72, 73, 73, 74, 74, 75, 75, 76, 76, 77, 77, 78, 78, 79, 79, 80,
/* -96*/ 80, 81, 81, 82, 82, 83, 83, 84, 84, 85, 85, 86, 86, 87, 87, 88,
/* -80*/ 88, 89, 89, 90, 90, 91, 91, 92, 92, 93, 93, 94, 94, 95, 95, 96,
/* -64*/ 96, 97, 97, 98, 98, 99, 99, 100, 100, 101, 101, 102, 102, 103, 103,
  104,
/* -48*/ 104, 105, 105, 106, 106, 107, 107, 108, 108, 109, 109, 110, 110,
  111, 111, 112,
/* -32*/ 112, 113, 113, 114, 114, 115, 115, 116, 116, 117, 117, 118, 118,
  119, 119, 120,
/* -16*/ 120, 121, 121, 122, 122, 123, 123, 124, 124, 125, 125, 126, 126,
  127, 127, 128,
/*   0*/ 128, 128, 129, 129, 130, 130, 131, 131, 132, 132, 133, 133, 134,
  134, 135, 135,
/*  16*/ 136, 136, 137, 137, 138, 138, 139, 139, 140, 140, 141, 141, 142,
  142, 143, 143,
/*  32*/ 144, 144, 145, 145, 146, 146, 147, 147, 148, 148, 149, 149, 150,
  150, 151, 151,
/*  48*/ 152, 152, 153, 153, 154, 154, 155, 155, 156, 156, 157, 157, 158,
  158, 159, 159,
/*  64*/ 160, 160, 161, 161, 162, 162, 163, 163, 164, 164, 165, 165, 166,
  166, 167, 167,
/*  80*/ 168, 168, 169, 169, 170, 170, 171, 171, 172, 172, 173, 173, 174,
  174, 175, 175,
/*  96*/ 176, 176, 177, 177, 178, 178, 179, 179, 180, 180, 181, 181, 182,
  182, 183, 183,
/* 112*/ 184, 184, 185, 185, 186, 186, 187, 187, 188, 188, 189, 189, 190,
  190, 191, 191
};

static __inline__ void
limit_8bit (unsigned char *buf, int l)
{
  int i;

  for (i = 0; i < l; i++)
    buf[i] = limit8[buf[i]];
}

static __inline__ void
limit_16bit (short *buf, int l)
{
  int i;
  l /= sizeof (*buf);

  for (i = 0; i < l; i++)
    buf[i] /= 2;
}

static __inline__ void
limit_32bit (int *buf, int l)
{
  int i;
  l /= sizeof (*buf);

  for (i = 0; i < l; i++)
    buf[i] /= 2;
}

static const unsigned char unlimit8[] = {
/*   0*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*  16*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*  32*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*  48*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*  64*/ 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30,
/*  80*/ 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62,
/*  96*/ 64, 66, 68, 70, 72, 74, 76, 78, 80, 82, 84, 86, 88, 90, 92, 94,
/* 112*/ 96, 98, 100, 102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122,
  124, 126,
/* 128*/ 128, 130, 132, 134, 136, 138, 140, 142, 144, 146, 148, 150, 152,
  154, 156, 158,
/* 144*/ 160, 162, 164, 166, 168, 170, 172, 174, 176, 178, 180, 182, 184,
  186, 188, 190,
/* 160*/ 192, 194, 196, 198, 200, 202, 204, 206, 208, 210, 212, 214, 216,
  218, 220, 222,
/* 176*/ 224, 226, 228, 230, 232, 234, 236, 238, 240, 242, 244, 246, 248,
  250, 252, 254,
/* 192*/ 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255,
/* 208*/ 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255,
/* 224*/ 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255,
/* 240*/ 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255
};

static __inline__ void
unlimit_8bit (unsigned char *buf, int l)
{
  int i;

  for (i = 0; i < l; i++)
    buf[i] = unlimit8[buf[i]];
}

static __inline__ void
unlimit_16bit (short *buf, int l)
{
  int i;
  l /= sizeof (*buf);

  for (i = 0; i < l; i++)
    buf[i] *= 2;
}

static __inline__ void
unlimit_32bit (int *buf, int l)
{
  int i;
  l /= sizeof (*buf);

  for (i = 0; i < l; i++)
    buf[i] *= 2;
}
#endif

/*ARGSUSED*/
static int
do_src (adev_p adev, dmap_p dmap, int srcrate, int tgtrate, void *p1,
	void *p2, int *len, int channels, int bits, int ssize)
{
#if GRC == 2
  int err, l, l1, l2;

  l = *len;

  err = src_convert (dmap->src, p1, l, bits, p2, 4096, 24, &l1, &l2, 0);
  *len = l2;
  if (l1 != l)
    cmn_err (CE_NOTE, "Everything not taken %d/%d\n", l1, l);

#endif

#if GRC == 3
  int ch, l, size, nsamples, err;

  l = *len / ssize;
  size = TMP_CONVERT_BUF_SIZE / ssize;
  size /= channels;
  nsamples = l / channels;

  if (bits == 24)
    bits = 32;

#  ifdef DO_TIMINGS
  oss_timing_printf ("grc3_convert %d bytes / %d samples * %d channels in. ",
	     *len, nsamples, channels);
#  endif

#if 0
  /* Limit the volume levels before calling GRC3 */
  buf = p1;

  switch (ssize)
    {
    case 1:
      limit_8bit ((unsigned char *) buf, *len);
      break;
    case 2:
      limit_16bit ((short *) buf, *len);
      break;
    case 4:
      limit_32bit ((int *) buf, *len);
      break;
    }
#endif

  for (ch = 0; ch < channels; ch++)
    {
      if ((err = grc3_convert (dmap->srcstate[ch], bits, adev->src_quality,	/* Numbits, quality */
			       p1, p2, nsamples, size, channels, ch)) < 0)
	{
	  cmn_err (CE_WARN, "SRC failed (%d), bits=%d, ssize=%d\n", err, bits,
		   ssize);
	  return OSS_EIO;
	}

      *len = ((grc3state_t *) dmap->srcstate[ch])->outsz * ssize * channels;
      VMEM_CHECK (p2, *len);
    }
#if 0
  buf = p2;
  switch (ssize)
    {
    case 1:
      unlimit_8bit ((unsigned char *) buf, *len);
      break;
    case 2:
      unlimit_16bit ((short *) buf, *len);
      break;
    case 4:
      unlimit_32bit ((int *) buf, *len);
      break;
    }
#endif
#  ifdef DO_TIMINGS
  oss_timing_printf ("-> %d bytes, %d samples out, %d samples in",
	     *len, ((grc3state_t *) dmap->srcstate[0])->outsz,
	     ((grc3state_t *) dmap->srcstate[0])->insz);
#  endif
#endif

  return 0;
}

static int
setup_src (adev_p adev, dmap_p dmap, int srate, int trate, int sch, int tch)
{
  int ch, nch;

  nch = sch;
  if (tch < nch)
    nch = tch;

  if (adev->src_quality < 1)
    adev->src_quality = 1;
  if (adev->src_quality > 5)
    adev->src_quality = 5;

#if GRC == 2
  if (nch > 2)
    {
      cmn_err (CE_WARN, "Too many channels for SRC (%d)\n", nch);
      return OSS_EIO;
    }
  {
    int val;
    val = src_find_output_rate (trate, srate);
    if (src_open (&dmap->src, srate, trate, nch, 0) != 0)
      {
	cmn_err (CE_CONT, "OSS audio: SRC open failed\n");
	return OSS_EIO;
      }
  }
#endif
#if GRC == 3
  if (nch > OSS_MAX_CONVERT_CHANNELS)
    {
      cmn_err (CE_WARN, "Too many channels for SRC (%d)\n", nch);
      return OSS_EIO;
    }
#ifdef DO_TIMINGS
  oss_timing_printf ("grc3_setup %d -> %d Hz, %d channels", srate, trate, nch);
#endif

  for (ch = 0; ch < nch; ch++)
    {
      if (dmap->srcstate[ch] == NULL)
	{
	  dmap->srcstate[ch] = PMALLOC (dmap->osdev, sizeof (grc3state_t));
	}
      grc3_reset (dmap->srcstate[ch]);
      grc3_setup (dmap->srcstate[ch], srate, trate);
    }
#endif
  return 0;
}

#if !defined(__OpenBSD__)
static __inline__ unsigned int
swap32 (unsigned int x)
{

#if 1
  unsigned int y = 0;
  unsigned char *a = ((unsigned char *) &x) + 3;
  unsigned char *b = (unsigned char *) &y;

  *b++ = *a--;
  *b++ = *a--;
  *b++ = *a--;
  *b++ = *a--;

  return y;
#else
  /* Old version */
  return ((x & 0x000000ff) << 24) |
    ((x & 0x0000ff00) << 8) |
    ((x & 0x00ff0000) >> 8) | ((x & 0xff000000) >> 24);
#endif
}
#endif

#ifdef SINE_DEBUG
#define SIN_STEPS	48
static short sinebuf[48] = {
  0, 4276, 8480, 12539, 16383, 19947, 23169, 25995,
  28377, 30272, 31650, 32486, 32767, 32486, 31650, 30272,
  28377, 25995, 23169, 19947, 16383, 12539, 8480, 4276,
  0, -4276, -8480, -12539, -16383, -19947, -23169, -25995,
  -28377, -30272, -31650, -32486, -32767, -32486, -31650, -30272,
  -28377, -25995, -23169, -19947, -16383, -12539, -8480, -4276
};
static int sine_ptr = 0;
#endif

static int
cnv_default (adev_p adev, dmap_p dmap, unsigned char **srcp, int *srcl, unsigned char **tgtp,
	     sample_parms * source, sample_parms * target)
{
  void *p1 = *srcp, *p2 = *tgtp, *tmpp;
  int len = *srcl, l, i;

/*
 * Convert samples to 24 bit (32 bit lsb aligned) if necessary.
 */
  VMEM_CHECK (p1, len);
  VMEM_CHECK (p2, len);
  if (source->fmt != AFMT_S24_NE)
    switch (source->fmt)
      {
      case AFMT_U8:
	{
	  unsigned char *fromp = p1;
	  unsigned int *top = p2;

	  l = len;
	  len = l * sizeof (int);

	  for (i = 0; i < l; i++)
	    {
	      *top++ = ((signed char) (*fromp++ ^ 0x80)) << 16;
	    }

	  tmpp = p1;
	  p1 = p2;
	  p2 = tmpp;
	}
	break;

      case AFMT_S8:
	{
	  char *fromp = p1;
	  int *top = p2;
	  int t;

	  l = len;
	  len = l * sizeof (int);

	  for (i = 0; i < l; i++)
	    {
	      t = (*fromp++) << 24;
	      *top++ = t >> 8;
	    }

	  tmpp = p1;
	  p1 = p2;
	  p2 = tmpp;
	}
	break;

      case AFMT_MU_LAW:
	{
	  unsigned char *fromp = p1;
	  unsigned int *top = p2;

	  l = len;
	  len = l * sizeof (int);

	  for (i = 0; i < l; i++)
	    {
	      *top++ = ((signed char) (ulaw_dsp[(*fromp++)] ^ 0x80)) << 16;
	    }

	  tmpp = p1;
	  p1 = p2;
	  p2 = tmpp;
	}
	break;

      case AFMT_S16_NE:
	{
	  short *fromp = p1;
	  int *top = p2;

	  l = len / 2;
	  len = l * sizeof (int);

	  for (i = 0; i < l; i++)
	    {
	      *top++ = *fromp++ << 8;
	    }

	  tmpp = p1;
	  p1 = p2;
	  p2 = tmpp;
	}
	break;

      case AFMT_S16_OE:
	{
	  short *fromp = p1;
	  int *top = p2;

	  l = len / 2;
	  len = l * sizeof (int);

	  for (i = 0; i < l; i++)
	    {
	      short tmp;
	      int t;
	      tmp = *fromp++;
	      /* Try to maintain the sign bit by shifting 8 bits too far */
	      t = ((tmp & 0xff) << 24) | ((tmp & 0xff00) << 8);
	      *top++ = t >> 8;
	    }

	  tmpp = p1;
	  p1 = p2;
	  p2 = tmpp;
	}
	break;

      case AFMT_S32_NE:
	{
	  int *fromp = p1;
	  int *top = p2;

	  l = len / 4;
	  /* len = l * sizeof (int); */

	  for (i = 0; i < l; i++)
	    {
	      *top++ = *fromp++ >> 8;
	    }

	  tmpp = p1;
	  p1 = p2;
	  p2 = tmpp;
	}
	break;

      case AFMT_S32_OE:
	{
	  int *fromp = p1;
	  int *top = p2;

	  l = len / 4;
	  /* len = l * sizeof (int); */

	  for (i = 0; i < l; i++)
	    {
	      *top++ = swap32 (*fromp++) >> 8;
	    }

	  tmpp = p1;
	  p1 = p2;
	  p2 = tmpp;
	}
	break;

      case AFMT_S24_OE:
	{
	  int *fromp = p1;
	  int *top = p2;

	  l = len / 4;
	  /* len = l * sizeof (int); */

	  for (i = 0; i < l; i++)
	    {
	      *top++ = swap32 ((unsigned int) (*fromp++));
	    }

	  tmpp = p1;
	  p1 = p2;
	  p2 = tmpp;
	}
	break;

      case AFMT_S24_PACKED:
	{
	  unsigned char *fromp = p1;
	  int *top = p2;

	  l = len / 3;
	  len = l * sizeof (int);

	  for (i = 0; i < l; i++)
	    {
	      int tmp = 0;

#if 1
	      tmp |= (*fromp++);
	      tmp |= (*fromp++) << 8;
	      tmp |= (*fromp++) << 16;
#else
	      tmp |= (*fromp++) << 16;
	      tmp |= (*fromp++) << 8;
	      tmp |= (*fromp++);
#endif

	      *top++ = tmp;

	    }

	  tmpp = p1;
	  p1 = p2;
	  p2 = tmpp;
	}
	break;

      default:
	cmn_err (CE_WARN, "Unsupported conversion source format %08x\n",
		 source->fmt);
	return OSS_EIO;
      }

  if (source->rate != target->rate && source->channels <= target->channels)
    {
      int err;

      VMEM_CHECK (p1, len);
      err =
	do_src (adev, dmap, source->rate, target->rate, p1, p2, &len,
		source->channels, 32, 4);
      if (err < 0)
	return err;

      tmpp = p1;
      p1 = p2;
      p2 = tmpp;
    }

/*
 * Convert between mono and stereo
 */

  if (source->channels != target->channels)
    {
      VMEM_CHECK (p1, len);
      if (source->channels == 1 && target->channels == 2)
	{
	  /* Mono -> Stereo */
	  int *fromp = p1, *top = p2;

	  l = len / sizeof (int);	/* Number of (mono) samples */
	  len = len * 2;	/* Number of bytes will get doubled */
	  for (i = 0; i < l; i++)
	    {
	      *top++ = *fromp;
	      *top++ = *fromp++;
	    }
	  tmpp = p1;
	  p1 = p2;
	  p2 = tmpp;
	}
      else if (source->channels == 2 && target->channels == 1)
	{
	  /* Stereo -> mono */
	  int *fromp = p1, *top = p2;

	  len = len / 2;	/* Number of bytes will drop to a half */
	  l = len / sizeof (int);	/* Number of (mono) samples */

	  for (i = 0; i < l; i++)
	    {
	      *top++ = *fromp++;	/* Take just the left channel sample */
	      fromp++;		/* discard the right channel one */
	    }
	  tmpp = p1;
	  p1 = p2;
	  p2 = tmpp;
	}
      else
	{
	  /* Multi channel conversions */

	  int *fromp = p1, *top = p2;
	  int n, nc, sc, tc;

	  l = len / sizeof (int);
	  n = l / source->channels;
	  len = len * target->channels / source->channels;

	  tc = target->channels;
	  sc = source->channels;
	  nc = tc;
	  if (nc > sc)
	    nc = sc;

	  memset (top, 0, len);

	  for (i = 0; i < n; i++)
	    {
	      int c;

	      for (c = 0; c < nc; c++)
		top[c] = fromp[c];

	      fromp += sc;
	      top += tc;
	    }
	  tmpp = p1;
	  p1 = p2;
	  p2 = tmpp;
	}
    }

  if (source->rate != target->rate && source->channels > target->channels)
    {
      int err;

      VMEM_CHECK (p1, len);
      err =
	do_src (adev, dmap, source->rate, target->rate, p1, p2, &len,
		target->channels, 24, 4);
      if (err < 0)
	return err;

      tmpp = p1;
      p1 = p2;
      p2 = tmpp;
    }

#ifdef SINE_DEBUG
/*
 * Debugging stuff. Overwrite the sample buffer with pure sine wave.
 */
  {
    int *p = p1;
    l = len / sizeof (int);
    for (i = 0; i < l; i += 2)
      {
#if 1
	*p++ = sinebuf[sine_ptr] << 15;	/* Left chn */
	*p++ = sinebuf[sine_ptr] << 15;	/* Right chn */
	sine_ptr = (sine_ptr + 1) % SIN_STEPS;
#else
	static short pp = 0;
	*p++ = pp * 256;
	*p++ = pp * 256;
	pp++;
#endif
      }
  }
#endif

/*
 * Finally convert the samples from internal 24 bit format to the target format
 */

  if (target->fmt != AFMT_S24_NE)
    switch (target->fmt)
      {
      case AFMT_U8:
	{
	  unsigned int *fromp = p1;
	  unsigned char *top = p2;

	  l = len / sizeof (int);
	  len = l;

	  for (i = 0; i < l; i++)
	    {
	      *top++ = (*fromp++ >> 16) ^ 0x80;
	    }

	  tmpp = p1;
	  p1 = p2;
	  p2 = tmpp;
	}
	break;

      case AFMT_S8:
	{
	  int *fromp = p1;
	  char *top = p2;

	  l = len / sizeof (int);
	  len = l;

	  for (i = 0; i < l; i++)
	    {
	      *top++ = *fromp++ >> 16;
	    }

	  tmpp = p1;
	  p1 = p2;
	  p2 = tmpp;
	}
	break;

      case AFMT_S16_NE:
	{
	  int *fromp = p1;
	  short *top = p2;

	  l = len / sizeof (int);
	  len = l * sizeof (short);

	  for (i = 0; i < l; i++)
	    {
	      *top++ = *fromp++ >> 8;
	    }

	  tmpp = p1;
	  p1 = p2;
	  p2 = tmpp;
	}
	break;

      case AFMT_S16_OE:
	{
	  int *fromp = p1;
	  unsigned char *top = p2;

	  l = len / sizeof (int);
	  len = l * sizeof (short);

	  for (i = 0; i < l; i++)
	    {
	      int tmp;
	      tmp = *fromp++ >> 8;
	      *top++ = tmp & 0xff;
	      *top++ = (tmp >> 8) & 0xff;
	    }

	  tmpp = p1;
	  p1 = p2;
	  p2 = tmpp;
	}
	break;

      case AFMT_S32_NE:
	{
	  int *fromp = p1;
	  int *top = p2;

	  l = len / 4;
	  /* len = l * sizeof (int); */

	  for (i = 0; i < l; i++)
	    {
	      *top++ = *fromp++ << 8;
	    }

	  tmpp = p1;
	  p1 = p2;
	  p2 = tmpp;
	}
	break;

      case AFMT_S32_OE:
	{
	  int *fromp = p1;
	  int *top = p2;

	  l = len / 4;
	  /* len = l * sizeof (int); */

	  for (i = 0; i < l; i++)
	    {
	      *top++ = swap32 ((unsigned int) (*fromp++ << 8));
	    }

	  tmpp = p1;
	  p1 = p2;
	  p2 = tmpp;
	}
	break;

      case AFMT_S24_OE:
	{
	  int *fromp = p1;
	  int *top = p2;

	  l = len / 4;
	  /* len = l * sizeof (int); */

	  for (i = 0; i < l; i++)
	    {
	      *top++ = swap32 ((unsigned int) (*fromp++));
	    }

	  tmpp = p1;
	  p1 = p2;
	  p2 = tmpp;
	}
	break;

      case AFMT_S24_PACKED:
	{
	  int *fromp = p1;
	  unsigned char *top = p2;

	  l = len / 4;
	  len = l * 3;

	  for (i = 0; i < l; i++)
	    {
	      int tmp = *fromp++;
	      *top++ = tmp & 0xff;
	      *top++ = (tmp >> 8) & 0xff;
	      *top++ = (tmp >> 16) & 0xff;
	    }

	  tmpp = p1;
	  p1 = p2;
	  p2 = tmpp;
	}
	break;

      case AFMT_MU_LAW:
	{
	  unsigned int *fromp = p1;
	  unsigned char *top = p2;

	  l = len / sizeof (int);
	  len = l;

	  for (i = 0; i < l; i++)
	    {
	      *top++ = dsp_ulaw[((*fromp++ >> 16) & 0xff) ^ 0x80];
	    }

	  tmpp = p1;
	  p1 = p2;
	  p2 = tmpp;
	}
	break;

      default:
	cmn_err (CE_WARN, "Unsupported conversion target format %08x\n",
		 target->fmt);
	return OSS_EIO;
      }

  VMEM_CHECK (p1, len);
  *srcl = len;
  *srcp = p1;
  *tgtp = p2;

  return 0;
}

static int
cnv_srconly (adev_p adev, dmap_p dmap, unsigned char **srcp, int *srcl, unsigned char **tgtp,
	     sample_parms * source, sample_parms * target)
{
  void *p1 = *srcp, *p2 = *tgtp, *tmpp;
  int len = *srcl;
  int err;
  int ssize = 2;
  audio_format_info_p fmt;

  VMEM_CHECK (p1, len);
  VMEM_CHECK (p2, len);

  if ((fmt = oss_find_format (source->fmt)) == NULL)
    {
      cmn_err (CE_WARN, "Unknown audio format %x\n", source->fmt);
      return OSS_EIO;
    }

  switch (fmt->bits)
    {
    default:
    case 8:
      ssize = 1;
      break;
    case 16:
      ssize = 2;
      break;
    case 24:
      ssize = 4;
      break;
    case 32:
      ssize = 4;
      break;
    }

  err =
    do_src (adev, dmap, source->rate, target->rate, p1, p2, &len,
	    source->channels, fmt->bits, ssize);
  if (err < 0)
    return err;

  tmpp = p1;
  p1 = p2;
  p2 = tmpp;

  VMEM_CHECK (p1, len);
  *srcl = len;
  *srcp = p1;
  *tgtp = p2;

  return 0;
}

#include "audiocnv.inc"

static cnv_func_t
select_converter (unsigned int cnv, int expand, audio_format_info_p src_f,
		  audio_format_info_p tgt_f)
{
#if 1
  {
    int expand2 = expand * 100 / UNIT_EXPAND;
    DDB (cmn_err
	 (CE_CONT, "Expand = %d (%d.%02d)\n", expand, expand2 / 100,
	  expand2 % 100));
    DDB (cmn_err (CE_CONT, "Convert = %08x / ", cnv));
    if (cnv & CNV_SRC)
      DDB (cmn_err (CE_CONT, "CNV_SRC "));
    if (cnv & CNV_M2S)
      DDB (cmn_err (CE_CONT, "CNV_M2S "));
    if (cnv & CNV_S2M)
      DDB (cmn_err (CE_CONT, "CNV_S2M "));
    if (cnv & CNV_MULT)
      DDB (cmn_err (CE_CONT, "CNV_MULT "));
    if (cnv & CNV_Flog)
      DDB (cmn_err (CE_CONT, "CNV_Flog "));
    if (cnv & CNV_F8bit)
      DDB (cmn_err (CE_CONT, "CNV_F8bit "));
    if (cnv & CNV_F16bit)
      DDB (cmn_err (CE_CONT, "CNV_F16bit "));
    if (cnv & CNV_F24bit)
      DDB (cmn_err (CE_CONT, "CNV_F24bit "));
    if (cnv & CNV_F32bit)
      DDB (cmn_err (CE_CONT, "CNV_F32bit "));
    if (cnv & CNV_Tlog)
      DDB (cmn_err (CE_CONT, "CNV_Tlog "));
    if (cnv & CNV_T8bit)
      DDB (cmn_err (CE_CONT, "CNV_T8bit "));
    if (cnv & CNV_T16bit)
      DDB (cmn_err (CE_CONT, "CNV_T16bit "));
    if (cnv & CNV_T24bit)
      DDB (cmn_err (CE_CONT, "CNV_T24bit "));
    if (cnv & CNV_T32bit)
      DDB (cmn_err (CE_CONT, "CNV_T32bit "));
    if (cnv & CNV_SIGN)
      DDB (cmn_err (CE_CONT, "CNV_SIGN "));
    if (cnv & CNV_ENDIAN)
      DDB (cmn_err (CE_CONT, "CNV_ENDIAN "));
    if (cnv & CNV_TOLOG)
      DDB (cmn_err (CE_CONT, "CNV_TOLOG "));
    if (cnv & CNV_FROMLOG)
      DDB (cmn_err (CE_CONT, "CNV_FROMLOG "));
    DDB (cmn_err (CE_CONT, "\n"));
  }
  DDB (cmn_err (CE_CONT, "\n"));
#endif

  if (cnv == CNV_SRC)		/* Only SRC is needed */
    if (src_f->fmt & (CONVERTABLE_FORMATS & ~AFMT_MU_LAW))	/* Can convert this */
#ifdef OSS_BIG_ENDIAN
      if (src_f->endianess == ENDIAN_BIG)
#else
      if (src_f->endianess == ENDIAN_LITTLE)
#endif
	{
	  DDB (cmn_err (CE_CONT, "Only SRC\n"));
	  return cnv_srconly;
	}

  if (cnv & CNV_SRC)
    {				/* Needs SRC together with some other conversion. Use the default. */
      return cnv_default;
    }

  if (src_f->endianess != tgt_f->endianess)
    {				/* Needs endianess handling - use the default for the time being. */
      return cnv_default;
    }

#ifdef OSS_BIG_ENDIAN
  if (src_f->endianess != ENDIAN_BIG)
#else
  if (src_f->endianess != ENDIAN_LITTLE)
#endif
    {				/* Opposite endianess - use the default. */
      return cnv_default;
    }

/*
 * The remaining conversions don't use SRC  and the endianess is native 
 * so they are easier to optimize.
 */

  switch (cnv)
    {
    case CNV_F8bit | CNV_T16bit:
      return cnv_F8bits_T16bits;
    case CNV_F8bit | CNV_T32bit:
      return cnv_F8bits_T32bits;
    case CNV_F16bit | CNV_T32bit:
      return cnv_F16bits_T32bits;
    case CNV_F32bit | CNV_T16bit:
      return cnv_F32bits_T16bits;
    case CNV_F32bit | CNV_T8bit:
      return cnv_F32bits_T8bits;
    case CNV_F16bit | CNV_T8bit:
      return cnv_F16bits_T8bits;
    }

  DDB (cmn_err (CE_CONT, "Will use the default converter\n"));
  return cnv_default;
}

/*ARGSUSED*/
int
setup_format_conversions (adev_p adev, dmap_p dmap, sample_parms * source,
			  sample_parms * target,
			  sample_parms * user,
			  sample_parms * device, int format_mask)
{
  int expand = UNIT_EXPAND;
  unsigned int cnv = CNV_NONE;
  audio_format_info_p src_f, tgt_f;

  dmap->expand_factor = UNIT_EXPAND;

  if (source->fmt == AFMT_AC3)
    {
      source->channels = target->channels = 2;
      source->rate = target->rate;
      target->fmt = source->fmt;
      dmap->convert_mode = 0;
      dmap->convert_func = NULL;
      return 0;
    }

  if ((src_f = oss_find_format (source->fmt)) == NULL)
    {
      cmn_err (CE_CONT, "internal format error 1 (%x)\n", source->fmt);
      return OSS_EIO;
    }

  if ((tgt_f = oss_find_format (target->fmt)) == NULL)
    {
      cmn_err (CE_CONT, "internal format error 2\n");
      return OSS_EIO;
    }

#ifdef DO_TIMINGS
    oss_timing_printf ("Setting up format conversions for device %d",
	     adev->engine_num);
    oss_timing_printf ("  Speed %d->%d", source->rate, target->rate);
    oss_timing_printf ("  Channels %d->%d", source->channels, target->channels);
    oss_timing_printf ("  Format %02x/%s->%02x/%s", source->fmt, src_f->name,
	     target->fmt, tgt_f->name);
#endif
  DDB (cmn_err
       (CE_CONT, "Setting up format conversions for device %d\n",
	adev->engine_num));
  DDB (cmn_err (CE_CONT, "  Speed %d->%d\n", source->rate, target->rate));
  DDB (cmn_err
       (CE_CONT, "  Channels %d->%d\n", source->channels, target->channels));
  DDB (cmn_err
       (CE_CONT, "  Format %02x/%s->%02x/%s\n", source->fmt, src_f->name,
	target->fmt, tgt_f->name));

  if (source->channels > OSS_MAX_CONVERT_CHANNELS
      || target->channels > OSS_MAX_CONVERT_CHANNELS)
    {
      cmn_err
	(CE_WARN,
	 "Audio format conversions not supported with more than %d channels\n",
	 OSS_MAX_CONVERT_CHANNELS);
      dmap->flags &= ~DMAP_COOKED;
      return OSS_EIO;
    }

  expand = (expand * target->rate) / source->rate;
  expand = (expand * target->channels) / source->channels;
  expand = (expand * tgt_f->bits) / src_f->bits;

  dmap->expand_factor = expand;

  if (source->rate != target->rate)
    cnv |= CNV_SRC;

  if (source->channels != target->channels)	/* Change # of channels */
    {
      if (target->channels > 2 || source->channels > 2)
	cnv |= CNV_MULT;
      else
	{
	  if (source->channels == 1 && target->channels == 2)
	    cnv |= CNV_M2S;
	  else
	    cnv |= CNV_S2M;
	}
    }

  if (source->fmt != target->fmt)
    {

      if (src_f->endianess != tgt_f->endianess)
	if (src_f->endianess != ENDIAN_NONE
	    && tgt_f->endianess != ENDIAN_NONE)
	  {			/* Endianess change */
	    cnv |= CNV_ENDIAN;
	  }

      if (src_f->endianess != ENDIAN_NONE && tgt_f->endianess != ENDIAN_NONE)
	if (src_f->is_signed != tgt_f->is_signed)
	  {
	    cnv |= CNV_SIGN;
	  }

      if (src_f->bits != tgt_f->bits)
	{
	  switch (src_f->bits)
	    {
	    case 8:
	      cnv |= CNV_F8bit;
	      break;
	    case 16:
	      cnv |= CNV_F16bit;
	      break;
	    case 24:
	      cnv |= CNV_F24bit;
	      break;
	    case 32:
	      cnv |= CNV_F32bit;
	      break;
	    default:
	      cnv |= CNV_Flog;
	    }

	  switch (tgt_f->bits)
	    {
	    case 8:
	      cnv |= CNV_T8bit;
	      break;
	    case 16:
	      cnv |= CNV_T16bit;
	      break;
	    case 24:
	      cnv |= CNV_T24bit;
	      break;
	    case 32:
	      cnv |= CNV_T32bit;
	      break;
	    default:
	      cnv |= CNV_Tlog;
	    }
	}

      if (!src_f->is_linear)
	cnv |= CNV_FROMLOG;

      if (!tgt_f->is_linear)
	cnv |= CNV_TOLOG;
    }

  dmap->convert_mode = cnv;

  dmap->convert_func = select_converter (cnv, expand, src_f, tgt_f);

  dmap->tmpbuf_len = dmap->tmpbuf_ptr = 0;
  if (dmap->tmpbuf1 == NULL)
    {
      dmap->tmpbuf1 = PMALLOC (dmap->osdev, TMP_CONVERT_BUF_SIZE+512);
    }

  if (dmap->tmpbuf2 == NULL)
    {
      dmap->tmpbuf2 = PMALLOC (dmap->osdev, TMP_CONVERT_BUF_SIZE+512);
    }

  if (dmap->tmpbuf1 == NULL || dmap->tmpbuf2 == NULL)
    return OSS_ENOSPC;

  if (cnv & CNV_SRC)
    if (setup_src
	(adev, dmap, source->rate, target->rate, source->channels,
	 target->channels) < 0)
      {
	cmn_err (CE_CONT, "internal format error 3\n");
	return OSS_EIO;
      }

  return 0;
}
