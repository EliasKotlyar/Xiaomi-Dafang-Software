/*
 * Purpose: File header write routines for OSSRecord.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <soundcard.h>
#include <errno.h>
#include "ossplay_wparser.h"

#pragma pack(1)

typedef struct {
  char main_chunk[4];
  uint32 length;
  char chunk_type[4];

  char sub_chunk[4];
  uint32 sc_len;
  uint16 format;
  uint16 modus;
  uint32 sample_fq;
  uint32 byte_p_sec;
  uint16 block_align;
  uint16 bit_p_spl;

  char data_chunk[4];
  uint32 data_length;
}
WaveHeader;

typedef struct {
  char magic[4];
  uint32 offset;
  uint32 filelen;
  uint32 fmt;
  uint32 speed;
  uint32 channels;
 /*
  * Some old specs say the field below is optional. Others say that this field
  * is mandatory, and must be at least 4 bytes long (SoX prints out a warning).
  * We'll put a nice "Made by OSSRecord" comment and shut SoX up.
  */
  char comment[18];
}
AuHeader;

typedef struct {
  char main_chunk[4];
  uint32 length;
  char chunk_type[4];

  char sub_chunk[4];
  uint32 comm_len;
  uint16 channels;
  uint32 num_frames;
  uint16 bits;
  unsigned char speed[10];
  char data_chunk[4];
  uint32 data_length;
  uint32 offset;
  uint32 blocksize;
}
AiffHeader;

typedef struct {
  char magic[4];
  uint16 version;
  uint16 flags;

  char desc_chunk[4];
  uint32 desc_length_hi, desc_length_lo;
  unsigned char speed[8];
  char format[4];
  uint32 format_flags;
  uint32 bytes_per_packet;
  uint32 frames_per_packet;
  uint32 channels;
  uint32 bits;

  char data_chunk[4];
  uint32 length_hi, length_lo;
  uint32 edit_count;
}
CafHeader;

#pragma pack()

extern FILE *wave_fp;

static uint32
bswap (uint32 x)
{
  uint32 y = 0;
  unsigned char *a = ((unsigned char *) &x) + 3;
  unsigned char *b = (unsigned char *) &y;

  *b++ = *a--;
  *b++ = *a--;
  *b++ = *a--;
  *b = *a;

  return y;
}

static uint16
bswaps (uint16 x)
{
  uint16 y = 0;
  unsigned char *a = ((unsigned char *) &x) + 1;
  unsigned char *b = (unsigned char *) &y;

  *b++ = *a--;
  *b = *a;

  return y;
}

int
write_head (FILE * wave_fp, fctypes_t type, big_t datalimit,
            int format, int channels, int speed)
{
  uint32 dl = datalimit;

  if ((speed <= 0) || (channels <= 0) || (format <= 0)) return -1;

  if (datalimit > U32_MAX) dl = U32_MAX;

  switch (type)
    {
      case WAVE_FILE: {
        WaveHeader wh;
        int bits = format2bits (format);

        if (datalimit > U32_MAX - sizeof (WaveHeader))
          print_msg (WARNM, "Data size exceeds file format limit!\n");
        if ((datalimit == 0) || (datalimit > U32_MAX - sizeof (WaveHeader)))
          dl = U32_MAX - (U32_MAX % 2) - sizeof (WaveHeader);
        switch (format) {
          case AFMT_U8:
          case AFMT_S16_LE:
          case AFMT_S24_PACKED:
          case AFMT_S32_LE:
            wh.format = LE_SH (1);
            break;
          case AFMT_A_LAW:
            wh.format = LE_SH (6);
            break;
          case AFMT_MU_LAW:
            wh.format = LE_SH (7);
            break;
          default:
            print_msg (ERRORM,
                       "%s sample format not supported by WAV writer!\n",
                       sample_format_name (format));
            return E_FORMAT_UNSUPPORTED;
        }

        memcpy ((char *) &wh.main_chunk, "RIFF", 4);
        wh.length = LE_INT (dl + sizeof (WaveHeader) - 8);
        if (dl % 2) dl--;
        memcpy ((char *) &wh.chunk_type, "WAVE", 4);
        memcpy ((char *) &wh.sub_chunk, "fmt ", 4);
        wh.sc_len = LE_INT (16);
        wh.modus = LE_SH (channels);
        wh.sample_fq = LE_INT (speed);
        wh.block_align = LE_SH (channels * bits / 8);
        wh.byte_p_sec = LE_INT (speed * channels * bits / 8);
        wh.bit_p_spl = LE_SH (bits);
        memcpy ((char *) &wh.data_chunk, "data", 4);
        wh.data_length = LE_INT (dl);
        if (fwrite (&wh, sizeof (WaveHeader), 1, wave_fp) == 0) return E_ENCODE;
        } break;
      case AU_FILE: {
        AuHeader ah;

        if ((datalimit == 0) || (datalimit > U32_MAX)) dl = 0xffffffff;
        memcpy ((char *) &ah.magic, ".snd", 4);
        ah.offset = BE_INT (sizeof (AuHeader));
        ah.filelen = BE_INT (dl);
        switch (format)
          {
            case AFMT_MU_LAW: ah.fmt = BE_INT (1); break;
            case AFMT_S8: ah.fmt = BE_INT (2); break;
            case AFMT_S16_BE: ah.fmt = BE_INT (3);  break;
            case AFMT_S24_PACKED_BE: ah.fmt = BE_INT (4);  break;
            case AFMT_S32_BE: ah.fmt = BE_INT (5);  break;
            case AFMT_A_LAW: ah.fmt = BE_INT (27); break;
            default:
              print_msg (ERRORM,
                         "%s sample format not supported by AU writer!\n",
                         sample_format_name (format));
              return E_FORMAT_UNSUPPORTED;
          }
        ah.speed = BE_INT (speed);
        ah.channels = BE_INT (channels);
        memcpy ((char *) &ah.comment, "Made by OSSRecord", 18);
        if (fwrite (&ah, sizeof (AuHeader), 1, wave_fp) == 0) return -1;
        } break;
      case CAF_FILE: {
        CafHeader cfh;
        int bits = format2bits (format);
        uint32 i;

        memcpy ((char *)&cfh.magic, "caff", 4);
        cfh.version = BE_SH(1);
        cfh.flags = 0;

        memcpy ((char *)&cfh.desc_chunk, "desc", 4);
        cfh.desc_length_hi = 0;
        cfh.desc_length_lo = BE_INT (32);
        cfh.format_flags = 0;
        switch (format) {
          case AFMT_S16_LE:
          case AFMT_S24_PACKED_LE:
          case AFMT_S32_LE: cfh.format_flags = BE_INT (2);
          case AFMT_S8:
          case AFMT_S16_BE:
          case AFMT_S24_PACKED_BE:
          case AFMT_S32_BE:
            memcpy ((char *)&cfh.format, "lpcm", 4);
            break;
          case AFMT_MU_LAW:
            memcpy ((char *)&cfh.format, "ulaw", 4);
            break;
          case AFMT_A_LAW:
            memcpy ((char *)&cfh.format, "alaw", 4);
            break;
          default:
            print_msg (ERRORM,
                       "%s sample format not supported by CAF writer!\n",
                       sample_format_name (format));
            return E_FORMAT_UNSUPPORTED;
        }
        cfh.frames_per_packet = BE_INT (1);
        cfh.bytes_per_packet = BE_INT (bits / 8 * channels);
        cfh.channels = BE_INT (channels);
        cfh.bits = BE_INT (bits);

        /* The method used here is good enough for sane values */
        memset (cfh.speed, 0, sizeof (cfh.speed));
        i = 0;
        while ((1L << (i + 1)) <= speed) i++;
        cfh.speed[0] = 64; cfh.speed[1] = (i-1) << 4;
        i = (speed - (1 << i)) << (32-i);
        cfh.speed[5] = i & 240;
        cfh.speed[4] = (i >> 4) & 255;
        cfh.speed[3] = (i >> 12) & 255;
        cfh.speed[2] = (i >> 20) & 255;
        cfh.speed[1] |= (i >> 28) & 15;

        memcpy ((char *)&cfh.data_chunk, "data", 4);
        if ((datalimit == 0) || (datalimit > U32_MAX)) {
          cfh.length_lo = 0xffffffff;
          cfh.length_hi = 0xffffffff;
        } else {
          cfh.length_lo = BE_INT (datalimit);
          cfh.length_hi = 0;
        }
        cfh.edit_count = 0;
        if (fwrite (&cfh, sizeof (CafHeader), 1, wave_fp) == 0)
          return E_ENCODE;
      } break;
      case AIFF_FILE: {
        AiffHeader afh;
        int bits = format2bits (format);
        uint32 i;

        if (datalimit > U32_MAX - sizeof (AiffHeader))
          print_msg (WARNM, "Data size exceeds file format limit!\n");
        if ((datalimit == 0) || (datalimit > U32_MAX - sizeof (AiffHeader)))
          dl = U32_MAX - (U32_MAX % 2) - sizeof (AiffHeader);
        memcpy ((char *) &afh.main_chunk, "FORM", 4);
        afh.length = BE_INT (dl + sizeof (AiffHeader) - 8);
        if (dl % 2) dl--;
        memcpy ((char *) &afh.chunk_type, "AIFF", 4);
        memcpy ((char *) &afh.sub_chunk, "COMM", 4);
        afh.comm_len = BE_INT (18);
        afh.channels = BE_SH (channels);
        afh.num_frames = BE_INT (dl / bits);
        afh.bits = BE_SH (bits);
        switch (format)
          {
            case AFMT_S8:
            case AFMT_S16_BE:
            case AFMT_S24_PACKED_BE:
            case AFMT_S32_BE:
              break;
            default:
              print_msg (ERRORM,
                         "%s sample format not supported by AIFF writer!\n",
                         sample_format_name (format));
              return E_FORMAT_UNSUPPORTED;
          }

        /* The method used here is good enough for sane values */
        memset (afh.speed, 0, sizeof (afh.speed));
        i = 0;
        while ((1L << (i + 1)) <= speed) i++;
        afh.speed[0] = 64; afh.speed[1] = i-1;
        i = (speed - (1 << i)) << (31-i);
        afh.speed[5] = i & 255;
        afh.speed[4] = (i >> 8) & 255;
        afh.speed[3] = (i >> 16) & 255;
        afh.speed[2] = ((i >> 24) & 127) + 128;

        memcpy ((char *) &afh.data_chunk, "SSND", 4);
        afh.data_length = BE_INT (dl);
        afh.offset = BE_SH (0);
        afh.blocksize = BE_SH (0);
        if (fwrite (&afh, sizeof (AiffHeader), 1, wave_fp) == 0)
          return E_ENCODE;
        } break;
      case RAW_FILE:
      default: return 0;
    }
  return 0;
}

int
finalize_head (FILE * wave_fp, fctypes_t type, big_t datalimit,
               int format, int channels, int speed)
{
  if ((IS_IFF_FILE (type)) && (datalimit % 2))
    {
      /*
       * All chunks must have an even length in an IFF file,
       * so we have to add a pad byte in this case.
       * Since we always write the data chunk last, we can
       * just append it to end of file.
       */
      char flag = 0;

      fseek (wave_fp, 0, SEEK_END);
      if (fwrite (&flag , 1, 1, wave_fp) == 0)
        print_msg (WARNM, "Couldn't add padding byte to SSND chunk!\n");
    }
  if ((type != RAW_FILE) && (fseek (wave_fp, 0, SEEK_SET) == -1)) return E_ENCODE;
  write_head (wave_fp, type, datalimit, format, channels, speed);
  return 0;
}
