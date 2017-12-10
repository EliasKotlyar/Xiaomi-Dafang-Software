/*
 * Purpose: GRC3 Sample Rate Converter
 *
 * GRC library version 3.1
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

#ifdef _KERNEL
//#include "oss_config.h"
#endif

#ifdef _KERNEL
#include <inttypes.h>
#endif

#include "grc3.h"

char _grc3_copyright[] =
  "GRC library version 3.0\n"
  "Copyright (C)2001-2014 4Front Technologies\n"
  "Copyright (C)2001-2002 by George Yohng\n\0" "GRC3 ENGINE";

#ifdef GRC3_COMPILE_L
#define filter_data filter_data_L
#include "fltdata3_l.inc"
#undef filter_data
#endif

#ifdef GRC3_COMPILE_M
#define filter_data filter_data_M
#include "fltdata1_m.inc"
#undef filter_data
#endif

#ifdef GRC3_COMPILE_H
#define filter_data filter_data_H
#include "fltdata2_h.inc"
#undef filter_data
#define filter_data_HX filter_data_H
#endif

#ifdef GRC3_COMPILE_P
#define filter_data filter_data_P
#include "fltdata4_p.inc"
#undef filter_data
#define filter_data_PX filter_data_P
#endif

#if (CONFIG_OSS_GRC_MIN_QUALITY>CONFIG_OSS_GRC_MAX_QUALITY)
#error Invalid values specified in CONFIG_OSS_GRC_XXX_QUALITY
#endif

#if !defined(GRC3_COMPILE_L) && !defined(GRC3_COMPILE_M) && !defined(GRC3_COMPILE_H) && !defined(GRC3_COMPILE_P)
#error Invalid values specified in CONFIG_OSS_GRC_XXX_QUALITY
#endif


/* TODO: Wy doesn't -O and inlining work? */
#undef __inline__
#define __inline__

static __inline__ int32_t
_muldivu64 (GRCpreg uint32_t a, GRCpreg uint32_t val1, GRCpreg uint32_t val2)
{
  GRCvreg uint64_t v = ((uint64_t) a) * val1 / val2;
  return (uint32_t) (v);
}


static __inline__ int32_t
_grc_sat6 (GRCpreg int32_t a, GRCpreg int32_t b)
{
  GRCvreg int64_t v = ((int64_t) a) * b + (1 << 5);
  return (int32_t) (v >> 6);
}

#if 0
static __inline__ int32_t
_grc_sat21 (GRCpreg int32_t a, GRCpreg int32_t b)
{
  GRCvreg int64_t v = ((int64_t) a) * b + (1 << 20);
  return (int32_t) (v >> 21);
}
#endif

static __inline__ int32_t
_grc_sat31 (GRCpreg int32_t a, GRCpreg int32_t b)
{
  GRCvreg int64_t v = ((int64_t) a) * b + (1 << 30);
  return (int32_t) (v >> 31);
}


#define DEFINE_FILTER(T)\
    static __inline__ int32_t _filt31_##T(GRCpreg int32_t a, GRCpreg int32_t idx)\
    {\
        GRCvreg int64_t v=((int64_t)a)*filter_data_##T[idx>>15];\
        return (int32_t)(v>>31);\
    }

#define DEFINE_FILTER_HQ(T)\
    static __inline__ int32_t _filt31_##T(GRCpreg int32_t a, GRCpreg int32_t idx)\
    {\
        GRCvreg int32_t idx2=idx>>15;\
        GRCvreg int64_t v=((int64_t)a)*\
        \
        (filter_data_##T[idx2] +\
        \
        (((int64_t)(idx&32767))*(filter_data_##T[idx2+1] - filter_data_##T[idx2])>>15));\
        return (int32_t)(v>>31);\
    }


#ifdef GRC3_COMPILE_L
DEFINE_FILTER (L)
#endif

#ifdef GRC3_COMPILE_M
DEFINE_FILTER (M)
#endif

#ifdef GRC3_COMPILE_H
DEFINE_FILTER (H)
DEFINE_FILTER_HQ (HX)
#endif

#ifdef GRC3_COMPILE_P 
DEFINE_FILTER (P) 
DEFINE_FILTER_HQ (PX)
#endif 

int32_t
_clamp24 (int32_t v)
{
  return (v <= -0x007FFFFF) ? -0x007FFFFF :
    (v >= 0x007FFFFF) ? 0x007FFFFF : v;
}

#define DEFINE_CONVD(T,SZ)\
    static __inline__ int32_t _conv31d_##T(GRCpreg int32_t *history,GRCpreg uint32_t filter,GRCpreg uint32_t incv)\
    {\
        GRCvreg int32_t accum=0;\
        \
        filter = (1024<<15) - filter;\
        \
        while(filter<((uint32_t)(SZ<<15)))\
        {\
            accum+=_filt31_##T(*history,filter);\
            filter+=incv;\
            history--;\
        }\
        \
        return accum;\
    }

#ifdef GRC3_COMPILE_L
DEFINE_CONVD (L, 4096)
#endif

#ifdef GRC3_COMPILE_M
DEFINE_CONVD (M, 8192)
#endif

#ifdef GRC3_COMPILE_H
DEFINE_CONVD (H, 16384)
DEFINE_CONVD (HX, 16384)
#endif

#ifdef GRC3_COMPILE_P
DEFINE_CONVD (P, 32768)
DEFINE_CONVD (PX, 32768)
#endif

#define ITERATION(p)\
        accum+=_filt31_##p(*history,filter);\
        filter+=(1024<<15);\
        history--;

#ifdef GRC3_COMPILE_L
static __inline__ int32_t 
_conv31_L (GRCpreg int32_t * history, GRCpreg uint32_t filter)
{
  GRCvreg int32_t accum = 0;

  ITERATION (L) ITERATION (L) ITERATION (L) ITERATION (L) return accum;
}
#endif


#ifdef GRC3_COMPILE_M
static __inline__ int32_t
_conv31_M (GRCpreg int32_t * history, GRCpreg uint32_t filter)
{
  GRCvreg int32_t accum = 0;

  ITERATION (M) ITERATION (M) ITERATION (M) ITERATION (M)
    ITERATION (M) ITERATION (M) ITERATION (M) ITERATION (M) return accum;
}
#endif

#ifdef GRC3_COMPILE_H
static __inline__ int32_t
_conv31_H (GRCpreg int32_t * history, GRCpreg uint32_t filter)
{
  GRCvreg int32_t accum = 0;

  ITERATION (H) ITERATION (H) ITERATION (H) ITERATION (H)
    ITERATION (H) ITERATION (H) ITERATION (H) ITERATION (H)
    ITERATION (H) ITERATION (H) ITERATION (H) ITERATION (H)
    ITERATION (H) ITERATION (H) ITERATION (H) ITERATION (H) return accum;
}

static __inline__ int32_t
_conv31_HX (GRCpreg int32_t * history, GRCpreg uint32_t filter)
{
  GRCvreg int32_t accum = 0;

  ITERATION (HX) ITERATION (HX) ITERATION (HX) ITERATION (HX)
    ITERATION (HX) ITERATION (HX) ITERATION (HX) ITERATION (HX)
    ITERATION (HX) ITERATION (HX) ITERATION (HX) ITERATION (HX)
    ITERATION (HX) ITERATION (HX) ITERATION (HX) ITERATION (HX) return accum;
}
#endif

#ifdef GRC3_COMPILE_P
static __inline__ int32_t
_conv31_P (GRCpreg int32_t * history, GRCpreg uint32_t filter)
{
  GRCvreg int32_t accum = 0;

  ITERATION (P) ITERATION (P) ITERATION (P) ITERATION (P)
    ITERATION (P) ITERATION (P) ITERATION (P) ITERATION (P)
    ITERATION (P) ITERATION (P) ITERATION (P) ITERATION (P)
    ITERATION (P) ITERATION (P) ITERATION (P) ITERATION (P)
    ITERATION (P) ITERATION (P) ITERATION (P) ITERATION (P)
    ITERATION (P) ITERATION (P) ITERATION (P) ITERATION (P)
    ITERATION (P) ITERATION (P) ITERATION (P) ITERATION (P)
    ITERATION (P) ITERATION (P) ITERATION (P) ITERATION (P) return accum;
}

static __inline__ int32_t
_conv31_PX (GRCpreg int32_t * history, GRCpreg uint32_t filter)
{
  GRCvreg int32_t accum = 0;

  ITERATION (PX) ITERATION (PX) ITERATION (PX) ITERATION (PX)
    ITERATION (PX) ITERATION (PX) ITERATION (PX) ITERATION (PX)
    ITERATION (PX) ITERATION (PX) ITERATION (PX) ITERATION (PX)
    ITERATION (PX) ITERATION (PX) ITERATION (PX) ITERATION (PX)
    ITERATION (PX) ITERATION (PX) ITERATION (PX) ITERATION (PX)
    ITERATION (PX) ITERATION (PX) ITERATION (PX) ITERATION (PX)
    ITERATION (PX) ITERATION (PX) ITERATION (PX) ITERATION (PX)
    ITERATION (PX) ITERATION (PX) ITERATION (PX) ITERATION (PX) return accum;
}
#endif

static __inline__ int16_t
_swap16 (GRCpreg int32_t v)
{
  return (int16_t) ((((uint16_t) v) << 8) + (((uint16_t) v) >> 8));
}

static __inline__ int32_t
_swap32 (GRCpreg int32_t v)
{
  return (int32_t) ((((uint32_t) v) >> 24) +
		    (((uint32_t) v) << 24) +
		    (((uint32_t) _swap16 ((int16_t) (v >> 8))) << 8));
}

#include "grc3code.inc"

#define DECLCVT(bit,q,func)\
    if ((domain==bit)&&(quality==q)) \
    {\
        if (!grc) return 0; \
        return grc3_resample_##func(grc,\
                    (void *)src, \
            (void *)dst, \
            (int32_t)sz, \
            (int32_t)bufsz, \
            (int32_t)inc, \
            (int32_t)offset \
           ); \
    }else

int
grc3_convert (grc3state_t * grc,
	      int domain, int quality,
	      void *src, void *dst,
	      int sz, int bufsz, int inc, int offset)
{

again:

#ifdef GRC3_COMPILE_L
    DECLCVT (8, 0, L8_8)
    DECLCVT (16, 0, L16_16)
    DECLCVT (-16, 0, Lv16_v16)
    DECLCVT (32, 0, L32_32) 
    DECLCVT (-32, 0, Lv32_v32)

    DECLCVT (8, 1, L8_8)
    DECLCVT (16, 1, L16_16)
    DECLCVT (-16, 1, Lv16_v16)
    DECLCVT (32, 1, L32_32) 
    DECLCVT (-32, 1, Lv32_v32)
#endif
#ifdef GRC3_COMPILE_M
    DECLCVT (8, 2, M8_8)
    DECLCVT (16, 2, M16_16)
    DECLCVT (-16, 2, Mv16_v16)
    DECLCVT (32, 2, M32_32) 
    DECLCVT (-32, 2, Mv32_v32)
#endif
#ifdef GRC3_COMPILE_H
    DECLCVT (8, 3, H8_8)
    DECLCVT (16, 3, H16_16)
    DECLCVT (-16, 3, Hv16_v16)
    DECLCVT (32, 3, H32_32)
    DECLCVT (-32, 3, Hv32_v32)
    DECLCVT (8, 4, HX8_8)
    DECLCVT (16, 4, HX16_16)
    DECLCVT (-16, 4, HXv16_v16)
    DECLCVT (32, 4, HX32_32) 
    DECLCVT (-32, 4, HXv32_v32)
#endif
#ifdef GRC3_COMPILE_P
    DECLCVT (8, 5, P8_8)
    DECLCVT (16, 5, P16_16)
    DECLCVT (-16, 5, Pv16_v16)
    DECLCVT (32, 5, P32_32)
    DECLCVT (-32, 5, Pv32_v32)
    DECLCVT (8, 6, PX8_8)
    DECLCVT (16, 6, PX16_16)
    DECLCVT (-16, 6, PXv16_v16)
    DECLCVT (32, 6, PX32_32) 
    DECLCVT (-32, 6, PXv32_v32)
#endif
  {
    if ((quality > 6) && (grc))
      {
	quality = DEFAULT_GRC_QUALITY;
	goto again;
      }

    return -1;
  }
}

int
grc3_reset (grc3state_t * grc)
{
  int32_t t;
  grc->ptr = 0;
  grc->historyptr = grc->history + GRC3_MAXHISTORY;

  for (t = 0; t < GRC3_MAXHISTORY * 2; t++)
    grc->history[t] = 0;

  return 1;
}

static int
grc3_setup_up (grc3state_t * grc, uint32_t fromRate, uint32_t toRate)
{
  grc->srcrate = fromRate;
  grc->dstrate = toRate;
  grc->filtfactor = 0x80000000U / toRate;
  return 1;
}

static int
grc3_setup_dn (grc3state_t * grc, uint32_t fromRate, uint32_t toRate)
{
  grc->srcrate = fromRate;
  grc->dstrate = toRate;
  grc->filtfactor = 0x80000000U / fromRate;
  grc->ptr_incv = _muldivu64 (1024 << 15, toRate, fromRate);
  grc->sat = _muldivu64 (0x80000000U, toRate, fromRate);
  return 1;
}

int
grc3_setup (grc3state_t * grc, uint32_t fromRate, uint32_t toRate)
{
  while ((!(fromRate & 1)) && (!(toRate & 1)) && (fromRate > 0))
    {
      fromRate >>= 1;
      toRate >>= 1;
    }

  if (fromRate <= toRate)
    return grc3_setup_up (grc, fromRate, toRate);
  else
    return grc3_setup_dn (grc, fromRate, toRate);
}


#ifdef TESTCASE

#include <stdio.h>
grc3state_t grc1, grc2;

#define BUFLEN (135*4)		/* Randomish lenghth */

int
main ()
{
  int t, k, l;
  short tmp;
  short p[BUFLEN], o[BUFLEN * 8];

  grc3_reset (&grc1);
  grc3_setup (&grc1, 44100, 48000);
  grc3_reset (&grc2);
  grc3_setup (&grc2, 44100, 48000);


  while ((l = read (0, p, sizeof (p))) > 0)
    {
      t = l / 4;
      fprintf (stderr, "Convert %d (%d)", t, l);
      grc3_convert (&grc1, 16, 4, p, o, t, (uint32_t) - 1, 2, 0);
      grc3_convert (&grc2, 16, 4, p, o, t, (uint32_t) - 1, 2, 1);
      l = grc2.outsz * 4;
      fprintf (stderr, " -> %d (%d)\n", grc2.outsz, l);

      if (write (1, o, l) != l)
	{
	  perror ("write");
	  exit (-1);
	}

    }

  return 0;
}

#endif
