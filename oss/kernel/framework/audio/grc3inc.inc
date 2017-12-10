/*
 * Purpose: Resampling routines for GRC3
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

static __inline__ int32_t SUFFIX (grc3_upsample_) (grc3state_t * grc,
						   TYPEIN * src,
						   TYPEOUT * dst, uint32_t sz,
						   uint32_t bufsz,
						   int32_t inc,
						   int32_t offset)
{
  GRCvreg uint32_t ptr = grc->ptr;
  uint32_t srcrate = grc->srcrate;
  uint32_t dstrate = grc->dstrate;
  GRCvreg int32_t *history = grc->historyptr;
  uint32_t filtfactor = grc->filtfactor;
  uint32_t dstsz = 0;

  grc->insz = sz;

  src += offset;
  dst += offset;

  while (sz > 0)
    {
      while (ptr < dstrate)
	{
	  if (dstsz >= bufsz)
	    goto endloop;
	  OUT (dst[0],
	       DOCLAMP (QSUFFIX (_conv31_)
			(history, _grc_sat6 (ptr, filtfactor))));
	  ptr += srcrate;
#ifndef INCOUT
	  dst += inc;
#else
	  INCOUT
#endif
	    dstsz++;
	}

      history++;
      if (history >= (grc->history + GRC3_MAXHISTORY * 2))
	history -= GRC3_MAXHISTORY;

      history[0] = history[-GRC3_MAXHISTORY] = IN (((*src)));

      ptr -= dstrate;

      sz--;
#ifndef INCIN
      src += inc;
#else
      INCIN
#endif
    }
endloop:

  grc->ptr = ptr;
  grc->historyptr = history;
  grc->outsz = dstsz;
  grc->insz -= sz;

  return (int32_t) dstsz;
}

__inline__ int32_t SUFFIX (grc3_dnsample_) (grc3state_t * grc, TYPEIN * src,
					    TYPEOUT * dst, uint32_t sz,
					    uint32_t bufsz, int32_t inc,
					    int32_t offset)
{
  GRCvreg uint32_t ptr = grc->ptr;
  uint32_t srcrate = grc->srcrate;
  uint32_t dstrate = grc->dstrate;
  uint32_t sat = grc->sat;
  GRCvreg int32_t *history = grc->historyptr;
  uint32_t filtfactor = grc->filtfactor;
  uint32_t dstsz = 0;

  grc->insz = sz;

  src += offset;
  dst += offset;

  while (sz > 0)
    {
      while (ptr >= srcrate)
	{
	  if (dstsz >= bufsz)
	    goto endloop;
	  ptr -= srcrate;
	  OUT (dst[0],
	       DOCLAMP (QSUFFIX (_conv31d_)
			(history, _grc_sat6 (ptr, filtfactor),
			 grc->ptr_incv)));
#ifndef INCOUT
	  dst += inc;
#else
	  INCOUT
#endif
	    dstsz++;
	}

      history++;
      if (history >= (grc->history + GRC3_MAXHISTORY * 2))
	history -= GRC3_MAXHISTORY;

      /* TODO: for better quality multiplier is worth moving to output cascade */
      history[0] = history[-GRC3_MAXHISTORY] =
	_grc_sat31 (IN (((*src))), sat);

      ptr += dstrate;

      sz--;
#ifndef INCIN
      src += inc;
#else
      INCIN
#endif
    }
endloop:

  grc->ptr = ptr;
  grc->historyptr = history;
  grc->outsz = dstsz;
  grc->insz -= sz;

  return (int32_t) dstsz;
}

static __inline__ int32_t SUFFIX (grc3_resample_) (grc3state_t * grc,
						   void *src, void *dst,
						   uint32_t sz,
						   uint32_t bufsz,
						   int32_t inc,
						   int32_t offset)
{
  if (grc->srcrate <= grc->dstrate)
    return SUFFIX (grc3_upsample_) (grc, (TYPEIN *) src, (TYPEOUT *) dst, sz,
				    bufsz, inc, offset);
  else
    return SUFFIX (grc3_dnsample_) (grc, (TYPEIN *) src, (TYPEOUT *) dst, sz,
				    bufsz, inc, offset);
}

#undef TYPEIN
#undef TYPEOUT
#undef INCIN
#undef INCOUT
#undef SUFFIX
#undef IN
#undef OUT
