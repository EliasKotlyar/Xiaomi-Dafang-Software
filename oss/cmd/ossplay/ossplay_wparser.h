#ifndef _OSSRECORD_WPARSER_H
#define _OSSRECORD_WPARSER_H

#include "ossplay.h"

int write_head (FILE *, fctypes_t, big_t, int, int, int);
int finalize_head (FILE *, fctypes_t, big_t, int, int, int);

#ifdef OSS_BIG_ENDIAN
#define BE_INT(x) x
#define BE_SH(x) x
#define LE_INT(x) bswap(x)
#define LE_SH(x) bswaps(x)
#else
#define BE_INT(x) bswap(x)
#define BE_SH(x) bswaps(x)
#define LE_INT(x) x
#define LE_SH(x) x
#endif /* OSS_BIG_ENDIAN */

#endif
