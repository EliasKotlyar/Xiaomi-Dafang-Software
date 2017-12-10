/*
 * Purpose: Low level macros for GRC3
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

__inline__ signed short
clamp16 (int sample)
{
  if (sample > 32767)
    return 32767;
  else if (sample < -32768)
    return -32768;
  else
    return sample;
}

__inline__ signed char
clamp8 (int sample)
{
  if (sample > 127)
    return 127;
  else if (sample < -128)
    return -128;
  else
    return sample;
}


#define IN8(a)      ((a^0x80)<<15)
#define OUT8(d,a)   d=((clamp8(a>>15))^0x80)

#define IN16(a)     (a<<7)
#define OUT16(d,a)  d=clamp16(a>>7)

#define INv16(a)    (_swap16(a)<<7)
#define OUTv16(d,a) d=_swap16(clamp16(a>>7))

#define IN32(a)     (a)
#define OUT32(d,a)  d=(a)

#define INv32(a)    _swap32(a)
#define OUTv32(d,a) d=_swap32(a)

#define DOCLAMP(a)  (a)		/* do not clip overflows */

#undef QSUFFIX

/***********************************************************************/

#ifdef GRC3_COMPILE_L

#define QSUFFIX(a) a##L

#if 0
#define SUFFIX(a) a##L
#define TYPEIN    int16_t
#define TYPEOUT   int16_t
#define IN        IN16
#define OUT       OUT16
#include "grc3inc.inc"
#endif

#define SUFFIX(a) a##L8_8
#define TYPEIN    uint8_t
#define TYPEOUT   uint8_t
#define IN        IN8
#define OUT       OUT8
#include "grc3inc.inc"

#define SUFFIX(a) a##L16_16
#define TYPEIN    int16_t
#define TYPEOUT   int16_t
#define IN        IN16
#define OUT       OUT16
#include "grc3inc.inc"

#define SUFFIX(a) a##Lv16_v16
#define TYPEIN    int16_t
#define TYPEOUT   int16_t
#define IN        INv16
#define OUT       OUTv16
#include "grc3inc.inc"

#define SUFFIX(a) a##L32_32
#define TYPEIN    int32_t
#define TYPEOUT   int32_t
#define IN        IN32
#define OUT       OUT32
#include "grc3inc.inc"

#define SUFFIX(a) a##Lv32_v32
#define TYPEIN    int32_t
#define TYPEOUT   int32_t
#define IN        INv32
#define OUT       OUTv32
#include "grc3inc.inc"

#undef QSUFFIX

#endif

/***********************************************************************/

#ifdef GRC3_COMPILE_M

#define QSUFFIX(a) a##M

#if 0
#define SUFFIX(a) a##M
#define TYPEIN    int16_t
#define TYPEOUT   int16_t
#define IN        IN16
#define OUT       OUT16
#include "grc3inc.inc"
#endif

#define SUFFIX(a) a##M8_8
#define TYPEIN    uint8_t
#define TYPEOUT   uint8_t
#define IN        IN8
#define OUT       OUT8
#include "grc3inc.inc"

#define SUFFIX(a) a##M16_16
#define TYPEIN    int16_t
#define TYPEOUT   int16_t
#define IN        IN16
#define OUT       OUT16
#include "grc3inc.inc"

#define SUFFIX(a) a##Mv16_v16
#define TYPEIN    int16_t
#define TYPEOUT   int16_t
#define IN        INv16
#define OUT       OUTv16
#include "grc3inc.inc"

#define SUFFIX(a) a##M32_32
#define TYPEIN    int32_t
#define TYPEOUT   int32_t
#define IN        IN32
#define OUT       OUT32
#include "grc3inc.inc"

#define SUFFIX(a) a##Mv32_v32
#define TYPEIN    int32_t
#define TYPEOUT   int32_t
#define IN        INv32
#define OUT       OUTv32
#include "grc3inc.inc"

#undef QSUFFIX

#endif

/***********************************************************************/

#ifdef GRC3_COMPILE_H

#define QSUFFIX(a) a##H

#if 0
#define SUFFIX(a) a##H
#define TYPEIN    int16_t
#define TYPEOUT   int16_t
#define IN        IN16
#define OUT       OUT16
#include "grc3inc.inc"
#endif

#define SUFFIX(a) a##H8_8
#define TYPEIN    uint8_t
#define TYPEOUT   uint8_t
#define IN        IN8
#define OUT       OUT8
#include "grc3inc.inc"

#define SUFFIX(a) a##H16_16
#define TYPEIN    int16_t
#define TYPEOUT   int16_t
#define IN        IN16
#define OUT       OUT16
#include "grc3inc.inc"

#define SUFFIX(a) a##Hv16_v16
#define TYPEIN    int16_t
#define TYPEOUT   int16_t
#define IN        INv16
#define OUT       OUTv16
#include "grc3inc.inc"

#define SUFFIX(a) a##H32_32
#define TYPEIN    int32_t
#define TYPEOUT   int32_t
#define IN        IN32
#define OUT       OUT32
#include "grc3inc.inc"

#define SUFFIX(a) a##Hv32_v32
#define TYPEIN    int32_t
#define TYPEOUT   int32_t
#define IN        INv32
#define OUT       OUTv32
#include "grc3inc.inc"

#undef QSUFFIX

#endif

/***********************************************************************/

#ifdef GRC3_COMPILE_H

#define QSUFFIX(a) a##HX

#if 0
#define SUFFIX(a) a##HX
#define TYPEIN    int16_t
#define TYPEOUT   int16_t
#define IN        IN16
#define OUT       OUT16
#include "grc3inc.inc"
#endif

#define SUFFIX(a) a##HX8_8
#define TYPEIN    uint8_t
#define TYPEOUT   uint8_t
#define IN        IN8
#define OUT       OUT8
#include "grc3inc.inc"

#define SUFFIX(a) a##HX16_16
#define TYPEIN    int16_t
#define TYPEOUT   int16_t
#define IN        IN16
#define OUT       OUT16
#include "grc3inc.inc"

#define SUFFIX(a) a##HXv16_v16
#define TYPEIN    int16_t
#define TYPEOUT   int16_t
#define IN        INv16
#define OUT       OUTv16
#include "grc3inc.inc"

#define SUFFIX(a) a##HX32_32
#define TYPEIN    int32_t
#define TYPEOUT   int32_t
#define IN        IN32
#define OUT       OUT32
#include "grc3inc.inc"

#define SUFFIX(a) a##HXv32_v32
#define TYPEIN    int32_t
#define TYPEOUT   int32_t
#define IN        INv32
#define OUT       OUTv32
#include "grc3inc.inc"

#undef QSUFFIX

#endif

/***********************************************************************/

#ifdef GRC3_COMPILE_P

#define QSUFFIX(a) a##P

#if 0
#define SUFFIX(a) a##P
#define TYPEIN    int16_t
#define TYPEOUT   int16_t
#define IN        IN16
#define OUT       OUT16
#include "grc3inc.inc"
#endif

#define SUFFIX(a) a##P8_8
#define TYPEIN    uint8_t
#define TYPEOUT   uint8_t
#define IN        IN8
#define OUT       OUT8
#include "grc3inc.inc"

#define SUFFIX(a) a##P16_16
#define TYPEIN    int16_t
#define TYPEOUT   int16_t
#define IN        IN16
#define OUT       OUT16
#include "grc3inc.inc"

#define SUFFIX(a) a##Pv16_v16
#define TYPEIN    int16_t
#define TYPEOUT   int16_t
#define IN        INv16
#define OUT       OUTv16
#include "grc3inc.inc"

#define SUFFIX(a) a##P32_32
#define TYPEIN    int32_t
#define TYPEOUT   int32_t
#define IN        IN32
#define OUT       OUT32
#include "grc3inc.inc"

#define SUFFIX(a) a##Pv32_v32
#define TYPEIN    int32_t
#define TYPEOUT   int32_t
#define IN        INv32
#define OUT       OUTv32
#include "grc3inc.inc"

#undef QSUFFIX

#endif

/***********************************************************************/

#ifdef GRC3_COMPILE_P

#define QSUFFIX(a) a##PX

#if 0
#define SUFFIX(a) a##PX
#define TYPEIN    int16_t
#define TYPEOUT   int16_t
#define IN        IN16
#define OUT       OUT16
#include "grc3inc.inc"
#endif

#define SUFFIX(a) a##PX8_8
#define TYPEIN    uint8_t
#define TYPEOUT   uint8_t
#define IN        IN8
#define OUT       OUT8
#include "grc3inc.inc"

#define SUFFIX(a) a##PX16_16
#define TYPEIN    int16_t
#define TYPEOUT   int16_t
#define IN        IN16
#define OUT       OUT16
#include "grc3inc.inc"

#define SUFFIX(a) a##PXv16_v16
#define TYPEIN    int16_t
#define TYPEOUT   int16_t
#define IN        INv16
#define OUT       OUTv16
#include "grc3inc.inc"

#define SUFFIX(a) a##PX32_32
#define TYPEIN    int32_t
#define TYPEOUT   int32_t
#define IN        IN32
#define OUT       OUT32
#include "grc3inc.inc"

#define SUFFIX(a) a##PXv32_v32
#define TYPEIN    int32_t
#define TYPEOUT   int32_t
#define IN        INv32
#define OUT       OUTv32
#include "grc3inc.inc"

#undef QSUFFIX

#endif

/***********************************************************************/
