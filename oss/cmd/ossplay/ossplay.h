#ifndef OSSPLAY_H
#define OSSPLAY_H

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <soundcard.h>
#include "ossplay_console.h"

#define PLAYBUF_SIZE		1024
#define RECBUF_SIZE		512
/* Parser's buf length */
#define P_READBUF_SIZE		1024
#define DEFAULT_CHANNELS	1
#define DEFAULT_FORMAT		AFMT_U8
#define DEFAULT_SPEED		11025
#define MAX_CHANNELS		128
/*
 * Every update of output in verbose mode while playing is separated by at
 * least PLAY_UPDATE_INTERVAL milliseconds.
 */
#define PLAY_UPDATE_INTERVAL		200.0
/* As above, but for recording */
#define REC_UPDATE_INTERVAL		1000.0
/* As above, but used for level meters while recording */
#define LMETER_UPDATE_INTERVAL		20.0
/* Should be smaller than the above. Used to ensure an update at end of file */
#define UPDATE_EPSILON			1.0

/* Sanity check - no allocation by ossplay should pass this. */
#define OSSPLAY_MAX_MALLOC		32*1024*1024

#if !defined(OSS_NO_INTTYPES_H) && !defined(OSS_NO_LONG_LONG)
#define  __STDC_LIMIT_MACROS
#include <inttypes.h>

typedef long double ldouble_t;
typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef char flag;
typedef intptr_t intptr;
#define S32_MAX INT32_MAX
#define S32_MIN INT32_MIN
#define U32_MAX UINT32_MAX
typedef uintmax_t big_t;
typedef intmax_t sbig_t;
#define _PRIbig_t "%ju"
#define BIG_SPECIAL UINTMAX_MAX

#else
#ifdef OSS_NO_LONG_LONG
typedef long sbig_t;
typedef unsigned long big_t;
#define _PRIbig_t "%lu"
#define BIG_SPECIAL ULONG_MAX
#else
typedef long long sbig_t;
typedef unsigned long long big_t;
#define _PRIbig_t "%llu"
#define BIG_SPECIAL ULLONG_MAX
#endif

typedef long double ldouble_t;
typedef signed char int8;
typedef unsigned char uint8;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef char flag;
typedef long intptr;
#define S32_MAX 2147483647
#define S32_MIN (-S32_MAX - 1)
#define U32_MAX 4294967295U
#endif /* !OSS_NO_INTTYPES_H */

/*
 * We overload the format definitions to include some "fake" formats.
 * Therefor, the values should be negative to avoid collusions.
 */
enum {
 AFMT_MS_ADPCM = -256,
 AFMT_MS_IMA_ADPCM,
 AFMT_MS_IMA_ADPCM_3BITS,
 AFMT_MAC_IMA_ADPCM,
 AFMT_S24_PACKED_BE,
 AFMT_CR_ADPCM_2,
 AFMT_CR_ADPCM_3,
 AFMT_CR_ADPCM_4,
 AFMT_FIBO_DELTA,
 AFMT_EXP_DELTA,
 AFMT_FLOAT32_BE,
 AFMT_FLOAT32_LE,
 AFMT_DOUBLE64_BE,
 AFMT_DOUBLE64_LE
};
#define AFMT_S24_PACKED_LE	AFMT_S24_PACKED

typedef struct {
  int fd;
  int format;
  int channels;
  int speed;
  int flags;
  int reclevel;
#ifndef OSS_DEVNODE_SIZE
#define OSS_DEVNODE_SIZE	32
#endif
  char dname[OSS_DEVNODE_SIZE];
#ifndef OSS_LONGNAME_SIZE
#define OSS_LONGNAME_SIZE	64
#endif
  char current_songname[OSS_LONGNAME_SIZE];
  char * recsrc;
  char * playtgt;
}
dspdev_t;

typedef enum errors_t {
  E_OK,
  E_SETUP_ERROR,
  E_FORMAT_UNSUPPORTED,
  E_CHANNELS_UNSUPPORTED,
  E_DECODE,
  E_ENCODE,
  E_USAGE,
  /*
   * Not an error, but since seek function can also return an error this needs
   * to be different from the others
   */
  SEEK_CONT_AFTER_DECODE
}
errors_t;

#ifdef OGG_SUPPORT
#include <vorbis/vorbisfile.h>

typedef struct {
  void * vorbisfile_handle;
  int (*ov_clear) (OggVorbis_File *);
  vorbis_comment * (*ov_comment) (OggVorbis_File *, int);
  vorbis_info * (*ov_info) (OggVorbis_File *, int);
  int (*ov_open_callbacks) (void *, OggVorbis_File *, char *, long, ov_callbacks);
  long (*ov_raw_tell) (OggVorbis_File *);
  long (*ov_read) (OggVorbis_File *, char *, int, int, int, int, int *);
  int (*ov_seekable) (OggVorbis_File *);
  double (*ov_time_total) (OggVorbis_File *, int);
  int (*ov_time_seek) (OggVorbis_File *, double);
} dlopen_funcs_t;

typedef struct {
  OggVorbis_File vf;
  dlopen_funcs_t * f;

  int bitstream, setup;
  dspdev_t * dsp;
}
ogg_data_t;
#else
typedef void * dlopen_funcs_t;
#endif

/*
 * ossplay supports more containers than the list below. This type is used by
 * the IFF parser, ossrecord and some other functions though.
 */
typedef enum fctypes_t {
  RAW_FILE,
  WAVE_FILE,
  AU_FILE,
  AIFF_FILE,
  CAF_FILE,
  AIFC_FILE,
  WAVE_FILE_BE,
  _8SVX_FILE,
  _16SV_FILE,
  MAUD_FILE,
  W64_FILE,
  OGG_FILE
}
fctypes_t;

#define IS_IFF_FILE(t) (((t) == WAVE_FILE) || ((t) == WAVE_FILE_BE) || \
                        ((t) == AIFF_FILE) || ((t) == AIFC_FILE) || \
                        ((t) == _8SVX_FILE) || ((t) == _16SV_FILE) || \
                        ((t) == MAUD_FILE) \
                       )
/*
 * Used in the format_t table below.
 * Shows what actions can be done with the format - Play, Record or both.
 */
typedef enum direction_t {
  CP = 0x1,
  CR = 0x2,
  CRP = 0x3
}
direction_t;

typedef struct fmt_struct {
  const char * name;
  const int fmt;
  const direction_t dir;
  const int may_conv;
}
format_t;

typedef struct cnt_struct {
  const char * name;
  const fctypes_t type;
  const int dformat;
  const int dchannels;
  const int dspeed;
}
container_t;

typedef struct {
  int coeff1, coeff2;
}
adpcm_coeff;

typedef struct msadpcm_values {
  uint16 nBlockAlign;
  uint16 wSamplesPerBlock;
  uint16 wNumCoeff;
  uint16 bits;
  adpcm_coeff coeff[32];
  int channels;
}
msadpcm_values_t;

typedef ssize_t (decfunc_t) (unsigned char **, unsigned char *, ssize_t,
                             void *);
typedef errors_t (seekfunc_t) (int, big_t *, big_t, double, big_t, int, void *);
typedef ssize_t (readfunc_t) (int, void *, size_t, void *);

typedef enum decoder_flag_t {
  FREE_NONE = 0,
  FREE_OBUF = 1,
  FREE_META = 2
}
decoder_flag_t;

typedef struct decoders_queue {
  struct decoders_queue * next;
  decfunc_t * decoder;
  unsigned char * outbuf;
  void * metadata;
  decoder_flag_t flag;
}
decoders_queue_t;

big_t be_int (const unsigned char *, int);
const char * filepart (const char *);
float format2bits (int);
big_t le_int (const unsigned char *, int);
ldouble_t ossplay_ldexpl (ldouble_t, int);
int ossplay_parse_opts (int, char **, dspdev_t *);
int ossrecord_parse_opts (int, char **, dspdev_t *);
int play (dspdev_t *, int, big_t *, big_t, double, double,
          readfunc_t *, decoders_queue_t *, seekfunc_t *);
int record (dspdev_t *, FILE *, const char *, double, double,
            big_t *, decoders_queue_t * dec);
const char * sample_format_name (int);
errors_t setup_device (dspdev_t *, int, int, int);
errors_t silence (dspdev_t *, big_t, int);

void select_playtgt (dspdev_t *);
void select_recsrc (dspdev_t *);
void open_device (dspdev_t *);
void close_device (dspdev_t *);

#if !defined(OSS_BIG_ENDIAN) && !defined(OSS_LITTLE_ENDIAN)
#if AFMT_S16_NE == AFMT_S16_BE
#define OSS_BIG_ENDIAN
#else
#define OSS_LITTLE_ENDIAN
#endif /* AFMT_S16_NE == AFMT_S16_BE */
#endif /* !OSS_BIG_ENDIAN && !OSS_LITTLE_ENDIAN */

#endif
