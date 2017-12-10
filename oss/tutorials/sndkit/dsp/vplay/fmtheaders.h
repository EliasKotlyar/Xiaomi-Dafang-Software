#ifndef _FMTHEADERS_H
#define _FMTHEADERS_H	1

#include <sys/types.h>

/* Definitions for .VOC files */

#define MAGIC_STRING	"Creative Voice File\x1A"
#define ACTUAL_VERSION	0x010A
#define VOC_SAMPLESIZE	8

#define MODE_MONO	0
#define MODE_STEREO	1

#define DATALEN(bp)	((u_long)(bp->datalen) | \
                         ((u_long)(bp->datalen_m) << 8) | \
                         ((u_long)(bp->datalen_h) << 16) )

typedef struct _vocheader
{
  u_char magic[20];		/* must be MAGIC_STRING */
  u_short headerlen;		/* Headerlength, should be 0x1A */
  u_short version;		/* VOC-file version */
  u_short coded_ver;		/* 0x1233-version */
}
VocHeader;

typedef struct _blocktype
{
  u_char type;
  u_char datalen;		/* low-byte    */
  u_char datalen_m;		/* medium-byte */
  u_char datalen_h;		/* high-byte   */
}
BlockType;

typedef struct _voice_data
{
  u_char tc;
  u_char pack;
}
Voice_data;

typedef struct _ext_block
{
  u_short tc;
  u_char pack;
  u_char mode;
}
Ext_Block;


/* Definitions for Microsoft WAVE format */

#define RIFF		0x46464952
#define WAVE		0x45564157
#define FMT		0x20746D66
#define DATA		0x61746164
#define PCM_CODE	1
#define WAVE_MONO	1
#define WAVE_STEREO	2

/* it's in chunks like .voc and AMIGA iff, but my source say there
   are in only in this combination, so I combined them in one header;
   it works on all WAVE-file I have
*/
typedef struct _waveheader
{
  u_long main_chunk;		/* 'RIFF' */
  u_long length;		/* filelen */
  u_long chunk_type;		/* 'WAVE' */

  u_long sub_chunk;		/* 'fmt ' */
  u_long sc_len;		/* length of sub_chunk, =16 */
  u_short format;		/* should be 1 for PCM-code */
  u_short modus;		/* 1 Mono, 2 Stereo */
  u_long sample_fq;		/* frequence of sample */
  u_long byte_p_sec;
  u_short byte_p_spl;		/* samplesize; 1 or 2 bytes */
  u_short bit_p_spl;		/* 8, 12 or 16 bit */

  u_long data_chunk;		/* 'data' */
  u_long data_length;		/* samplecount */
}
WaveHeader;

typedef struct
{
  long magic;			/* must be equal to SND_MAGIC */
  long dataLocation;		/* Offset or pointer to the raw data */
  long dataSize;		/* Number of bytes of data in the raw data */
  long dataFormat;		/* The data format code */
  long samplingRate;		/* The sampling rate */
  long channelCount;		/* The number of channels */
}
SndHeader;

#define SND_MAGIC ((long int)0x2e736e64)

#define SND_FORMAT_UNSPECIFIED          (0)
#define SND_FORMAT_MULAW_8              (1)
#define SND_FORMAT_LINEAR_8             (2)
#define SND_FORMAT_LINEAR_16            (3)
#define SND_FORMAT_LINEAR_24            (4)
#define SND_FORMAT_LINEAR_32            (5)
#define SND_FORMAT_FLOAT                (6)

#endif
