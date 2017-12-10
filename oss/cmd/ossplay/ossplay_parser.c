/*
 * Purpose: File format parse routines for ossplay
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

#include "ossplay_parser.h"
#include "ossplay_decode.h"

#include <ctype.h>
#include <sys/stat.h>

/* Magic numbers used in Sun and NeXT audio files (.au/.snd) */
#define SUN_MAGIC 	0x2e736e64	/* Really '.snd' */
#define SUN_INV_MAGIC	0x646e732e	/* '.snd' upside-down */
#define DEC_MAGIC	0x2e736400	/* Really '\0ds.' (for DEC) */
#define DEC_INV_MAGIC	0x0064732e	/* '\0ds.' upside-down */

/* Magic numbers for .w64 */
#define riff_GUID	0x2E91CF11
#define riff_GUID2	0xA5D628DB
#define riff_GUID3	0x04C10000

enum {
  COMM_BIT,
  SSND_BIT,
  FVER_BIT
};

#define COMM_FOUND (1 << COMM_BIT)
#define SSND_FOUND (1 << SSND_BIT)
#define FVER_FOUND (1 << FVER_BIT)

#define H(A, B, C, D) ((A << 24) | (B << 16) | (C << 8) | D)

typedef struct {
  msadpcm_values_t msadpcm_val;
  int channels, fd, format, found, speed;
  fctypes_t type;
  uint32 chunk_id;
  big_t cpos, chunk_size, cur_size, fut_size, sound_loc, sound_size, total_size;
  const char * filename;
  big_t (* ne_int) (const unsigned char *, int);
}
file_t;

typedef int (chunk_parser_t) (uint32, unsigned char *, big_t, file_t *);

enum {
  CP_STOP_READING = -2,
  CP_PLAY_NOW,
  CP_OK
};

typedef ssize_t (file_read_t) (file_t *, unsigned char *, size_t);
typedef int (file_init_t) (file_t *, unsigned char *);
typedef int (file_iterator_t) (file_t *, unsigned char *, int);
typedef ssize_t (file_seek_t) (file_t *, off_t, int);

typedef enum {
  R_ZERO_FLAG,
  READ_NONE,
  READ_ALL,
  READ_PART
}
read_flag_t;

typedef struct chunk_functions {
  const uint32 id;
  const uint32 d_chunk_size;
  const read_flag_t read_chunk_f;
  chunk_parser_t * f;
}
chunk_functions_t;

typedef struct parser {
  file_init_t * init;
  file_read_t * read;
  file_iterator_t * iterator;
  const chunk_functions_t * perfile;
  const chunk_functions_t * common;
}
parser_t;

extern int quiet, verbose, force_fmt, force_speed;
extern long seek_byte;
extern flag from_stdin, raw_file;
extern off_t (*ossplay_lseek) (int, off_t, int);

static errors_t play_au (dspdev_t *, const char *, int, unsigned char *, int);
static errors_t play_iff (dspdev_t *, const char *, int, unsigned char *, int, parser_t *);
static errors_t play_voc (dspdev_t *, const char *, int, unsigned char *, int);
static void print_verbose_fileinfo (const char *, int, int, int, int);

static file_init_t caf_init;
static file_iterator_t caf_iterator;
static file_init_t iff_init;
static file_read_t iff_read;
static file_iterator_t iff_iterator;
static chunk_parser_t iff_comment_parse;
static file_init_t w64_init;
static file_iterator_t w64_iterator;

static chunk_parser_t _16sv_vhdr_parse;
static chunk_parser_t _8svx_vhdr_parse;
static chunk_parser_t aifc_comm_parse;
static chunk_parser_t aifc_fver_parse;
static chunk_parser_t aiff_comm_parse;
static chunk_parser_t aiff_ssnd_parse;
static chunk_parser_t caf_data_parse;
static chunk_parser_t caf_desc_parse;
static chunk_parser_t maud_chan_parse;
static chunk_parser_t maud_mhdr_parse;
static chunk_parser_t wave_data_parse;
static chunk_parser_t wave_disp_parse;
static chunk_parser_t wave_fmt_parse;
static chunk_parser_t wave_list_parse;

static const chunk_functions_t IFF_common[] = {
  { H('A', 'N', 'N', 'O'), 0, READ_ALL, &iff_comment_parse },
  { H('N', 'A', 'M', 'E'), 0, READ_ALL, &iff_comment_parse },
  { H('(', 'c', ')', ' '), 0, READ_ALL, &iff_comment_parse },
  { H('A', 'U', 'T', 'H'), 0, READ_ALL, &iff_comment_parse },
  { 0, 0, READ_NONE, NULL }
};

static const chunk_functions_t AIFF_funcs[] = {
  { H('C', 'O', 'M', 'M'), 18, READ_ALL, &aiff_comm_parse },
  { H('S', 'S', 'N', 'D'), 8, READ_PART, &aiff_ssnd_parse },
  { 0, 0, R_ZERO_FLAG, NULL }
};

static const chunk_functions_t AIFC_funcs[] = {
  { H('C', 'O', 'M', 'M'), 22, READ_ALL, &aifc_comm_parse },
  { H('S', 'S', 'N', 'D'), 8, READ_PART, &aiff_ssnd_parse },
  { H('F', 'V', 'E', 'R'), 4, READ_ALL, &aifc_fver_parse },
  { 0, 0, R_ZERO_FLAG, NULL }
};

static const chunk_functions_t WAVE_funcs[] = {
  { H('f', 'm', 't', ' '), 14, READ_ALL, &wave_fmt_parse },
  { H('d', 'a', 't', 'a'), 0, READ_NONE, &wave_data_parse },
  { H('D', 'I', 'S', 'P'), 5, READ_ALL, &wave_disp_parse },
  { H('L', 'I', 'S', 'T'), 12, READ_ALL, &wave_list_parse },
  { 0, 0, R_ZERO_FLAG, NULL }
};

static const chunk_functions_t _8SVX_funcs[] = {
  { H('B', 'O', 'D', 'Y'), 0, READ_NONE, &wave_data_parse },
  { H('V', 'H', 'D', 'R'), 16, READ_ALL, &_8svx_vhdr_parse },
  { 0, 0, R_ZERO_FLAG, NULL }
};

static const chunk_functions_t _16SV_funcs[] = {
  { H('V', 'H', 'D', 'R'), 14, READ_ALL, &_16sv_vhdr_parse },
  { H('B', 'O', 'D', 'Y'), 0, READ_NONE, &wave_data_parse },
  { 0, 0, R_ZERO_FLAG, NULL }
};

static const chunk_functions_t MAUD_funcs[] = {
  { H('M', 'D', 'A', 'T'), 0, READ_NONE, &wave_data_parse },
  { H('C', 'H', 'A', 'N'), 4, READ_ALL, &maud_chan_parse },
  { H('M', 'H', 'D', 'R'), 20, READ_ALL, &maud_mhdr_parse },
  { 0, 0, R_ZERO_FLAG, NULL }
};

static const chunk_functions_t CAF_funcs[] = {
  { H('d', 'e', 's', 'c'), 32, READ_ALL, &caf_desc_parse },
  { H('d', 'a', 't', 'a'), 4, READ_NONE, &caf_data_parse },
  { 0, 0, R_ZERO_FLAG, NULL }
};

#ifdef OGG_SUPPORT
static errors_t play_ogg (dspdev_t *, const char *, int, unsigned char *, int,
                          dlopen_funcs_t **);

/*
 * OV_CALLBACKS_DEFAULT is not defined by older Vorbis versions so 
 * we have to define our own version.
 */
static int _ov_header_fseek_wrap_ (FILE *f, ogg_int64_t off, int whence)
{
  if (f == NULL) return -1;
  return fseek(f, off, whence);
}

static ov_callbacks OV_CALLBACKS_DEFAULT_ = {
  (size_t (*)(void *, size_t, size_t, void *))  fread,
  (int (*)(void *, ogg_int64_t, int))           _ov_header_fseek_wrap_,
  (int (*)(void *))                             fclose,
  (long (*)(void *))                            ftell
};

/* Only handles Ogg/Vorbis */
static errors_t
play_ogg (dspdev_t * dsp, const char * filename, int fd, unsigned char * hdr,
          int l, dlopen_funcs_t ** vft)
{

  FILE * f;
  errors_t ret = E_OK;
  ogg_data_t ogg_data;
  vorbis_info * vi;

  f = fdopen (fd, "rb");

  if (*vft == NULL)
    {
      *vft = (dlopen_funcs_t *)ossplay_malloc (sizeof (dlopen_funcs_t));

      (*vft)->vorbisfile_handle = ossplay_dlopen ("libvorbisfile.so.3");

      if ((*vft)->vorbisfile_handle == NULL)
        {
          const char * msg = ossplay_dlerror();

          ossplay_free (*vft);
          *vft = NULL;
          print_msg (ERRORM, "Can't dlopen libvorbisfile! (Error was %s)\n",
                     msg?msg:"");
          return E_DECODE;
        }

      /*
       * Assigning to the address pointed by the casted lvalue is the
       * POSIX.1-2003 workaround to the dlsym return type issue, and both the
       * opengroup and glibc examples use that method, so this should be safe
       * on POSIX systems.
       */
      if (ossplay_vdlsym ((*vft)->vorbisfile_handle,
                     (void **)&(*vft)->ov_clear, "ov_clear",
                     (void **)&(*vft)->ov_comment, "ov_comment",
                     (void **)&(*vft)->ov_info, "ov_info",
                     (void **)&(*vft)->ov_open_callbacks, "ov_open_callbacks",
                     (void **)&(*vft)->ov_raw_tell, "ov_raw_tell",
                     (void **)&(*vft)->ov_read, "ov_read",
                     (void **)&(*vft)->ov_seekable, "ov_seekable",
                     (void **)&(*vft)->ov_time_total, "ov_time_total",
                     (void **)&(*vft)->ov_time_seek, "ov_time_seek",
                     (void **)NULL))
        {
          /* The call already took care of making an error printout for us */
          ossplay_dlclose ((*vft)->vorbisfile_handle);
          ossplay_free (*vft);
          *vft = NULL;
          return E_DECODE;
        }
    }

  ogg_data.f = *vft;

  if (ogg_data.f->ov_open_callbacks (f, &ogg_data.vf, (char *)hdr, l,
                                     OV_CALLBACKS_DEFAULT_) < 0)
    {
      print_msg (ERRORM, "File %s is not an OggVorbis file!\n", filename);
      return E_DECODE;
    }

  ogg_data.dsp = dsp;
  ogg_data.setup = 0;
  ogg_data.bitstream = -1;

  vi = ogg_data.f->ov_info (&ogg_data.vf, -1);
  if (verbose)
    {
      vorbis_comment * oc = ogg_data.f->ov_comment(&ogg_data.vf,-1);
      char **p = oc->user_comments;

      while (*p)
        print_msg (VERBOSEM, "%s: Comments: %s\n", filename, *p++);
      if ((verbose > 1) && (oc->vendor))
        print_msg (VERBOSEM, "%s: Vendor: %s\n", filename, oc->vendor);
      print_msg (VERBOSEM, "%s: Nominal bitrate: %d\n", filename, vi->bitrate_nominal);

      print_verbose_fileinfo (filename, OGG_FILE, AFMT_VORBIS, vi->channels, vi->rate);
    }

  ret = decode_sound (dsp, fd, BIG_SPECIAL, AFMT_VORBIS, vi->channels, vi->rate,
                      (void *)&ogg_data);

  ogg_data.f->ov_clear (&ogg_data.vf);

  return ret;
}
#endif

errors_t
play_file (dspdev_t * dsp, const char * filename, dlopen_funcs_t ** dlt)
{
  int fd, id;
  ssize_t l, i;
  unsigned char buf[P_READBUF_SIZE];
  const char * bname, * suffix;
  struct stat st;
  errors_t ret = E_OK;

  parser_t piff = {
    &iff_init,
    &iff_read,
    &iff_iterator,
    NULL,
    NULL
  };

  if (from_stdin) {
    FILE *fp;

    fp = fdopen(0, "rb");
    fd = fileno(fp);
    /*
     * Use emulation if stdin is not seekable (e.g. on Linux).
     */
    if (lseek (fd, 0, SEEK_CUR) == -1) ossplay_lseek = ossplay_lseek_stdin;
    errno = 0;
    bname = "-";
  } else {
    fd = open (filename, O_RDONLY, 0);
    bname = filepart (filename);
  }

  if (fd == -1)
    {
      perror_msg (filename);
      return E_DECODE;
    }

  if (seek_byte != 0)
    {
      ossplay_lseek (fd, (off_t)seek_byte, SEEK_CUR);
    }

  if (raw_file)
    {
      big_t len;

      if (fstat (fd, &st) == -1) {
        perror_msg (filename);
        len = BIG_SPECIAL;
      } else {
        len = st.st_size;
      }
      print_msg (NORMALM, "%s: Playing RAW file.\n", bname);

      ret = decode_sound (dsp, fd, len, DEFAULT_FORMAT,
                          DEFAULT_CHANNELS, DEFAULT_SPEED, NULL);
      goto done;
    }

  if ((l = read (fd, buf, 12)) == -1)
    {
      perror_msg (filename);
      goto seekerror;
    }

  if (l == 0)
    {
      print_msg (ERRORM, "%s is empty file.\n", bname);
      goto seekerror;
    }

/*
 * Try to detect the file type
 */
  id = be_int (buf, 4);
  switch (id) {
    case SUN_MAGIC:
    case DEC_MAGIC:
    case SUN_INV_MAGIC:
    case DEC_INV_MAGIC:
      if ((i = read (fd, buf + 12, 12)) == -1) {
        perror_msg (filename);
        goto seekerror;
      }
      l += i;
      if (l < 24) break;
      ret = play_au (dsp, bname, fd, buf, l);
      goto done;
    case H('C', 'r', 'e', 'a'):
      if ((i = read (fd, buf + 12, 7)) == -1) {
        perror_msg (filename);
        goto seekerror;
      }
      l += i;
      if ((l < 19) || (memcmp (buf, "Creative Voice File", 19))) break;
      ret = play_voc (dsp, bname, fd, buf, l);
      goto done;
    case H('R', 'I', 'F', 'F'):
    case H('R', 'I', 'F', 'X'):
      if ((l < 12) || be_int (buf + 8, 4) != H('W', 'A', 'V', 'E')) break;
      if (force_fmt == AFMT_IMA_ADPCM) force_fmt = AFMT_MS_IMA_ADPCM;
      piff.perfile = WAVE_funcs;
      ret = play_iff (dsp, bname, fd, buf, (id == H('R', 'I', 'F', 'X'))?
                      WAVE_FILE_BE:WAVE_FILE, &piff);
      goto done;
    case H('r', 'i', 'f', 'f'):
      if ((l < 12) || (read (fd, buf + 12, 4) < 4)) break;
      if (be_int (buf + 4, 4) != riff_GUID) break;
      if (be_int (buf + 8, 4) != riff_GUID2) break;
      if (be_int (buf + 12, 4) != riff_GUID3) break;
      piff.perfile = WAVE_funcs;
      piff.iterator = w64_iterator;
      piff.init = w64_init;
      ret = play_iff (dsp, bname, fd, buf, W64_FILE, &piff);
      goto done;
    case H('F', 'O', 'R', 'M'):
      if (l < 12) break;
      piff.common = IFF_common;
      switch (be_int (buf + 8, 4)) {
        case H('A', 'I', 'F', 'F'):
          if (force_fmt == AFMT_IMA_ADPCM) force_fmt = AFMT_MAC_IMA_ADPCM;
          piff.perfile = AIFF_funcs;
          ret = play_iff (dsp, bname, fd, buf, AIFF_FILE, &piff);
          goto done;
        case H('A', 'I', 'F', 'C'):
          if (force_fmt == AFMT_IMA_ADPCM) force_fmt = AFMT_MAC_IMA_ADPCM;
          piff.perfile = AIFC_funcs;
          ret = play_iff (dsp, bname, fd, buf, AIFC_FILE, &piff);
          goto done;
        case H('8', 'S', 'V', 'X'):
          piff.perfile = _8SVX_funcs;
          ret = play_iff (dsp, bname, fd, buf, _8SVX_FILE, &piff);
          goto done;
        case H('1', '6', 'S', 'V'):
          piff.perfile = _16SV_funcs;
          ret = play_iff (dsp, bname, fd, buf, _16SV_FILE, &piff);
          goto done;
        case H('M', 'A', 'U', 'D'):
          piff.perfile = MAUD_funcs;
          ret = play_iff (dsp, bname, fd, buf, MAUD_FILE, &piff);
          goto done;
        default: break;
      }
      piff.common = NULL;
      break;
    case H('c', 'a', 'f', 'f'):
      piff.init = caf_init;
      piff.iterator = caf_iterator;
      piff.perfile = CAF_funcs;
      ret = play_iff (dsp, bname, fd, buf, CAF_FILE, &piff);
      goto done;
    case H('O', 'g', 'g', 'S'):
#ifdef OGG_SUPPORT
      ret = play_ogg (dsp, bname, fd, buf, l, dlt);
      fd = -1;
      goto done;
#endif
    default: break;
  }

  ossplay_lseek (fd, 0, SEEK_SET);	/* Start from the beginning */

/*
 *	The file was not identified by its content. Try using the file name
 *	suffix.
 */

  suffix = strrchr (filename, '.');
  if (suffix == NULL) suffix = filename;

  if (fstat (fd, &st) == -1)
    {
      perror_msg (filename);
      return E_DECODE;
    }

  if (strcmp (suffix, ".au") == 0 || strcmp (suffix, ".AU") == 0)
    {				/* Raw mu-Law data */
      print_msg (VERBOSEM, "Playing raw mu-Law file %s\n", bname);

      ret = decode_sound (dsp, fd, st.st_size, AFMT_MU_LAW, 1, 8000, NULL);
      goto done;
    }

  if (strcmp (suffix, ".snd") == 0 || strcmp (suffix, ".SND") == 0)
    {
      print_msg (VERBOSEM,
                 "%s: Unknown format. Assuming RAW audio (%d/%d/%d).\n",
                 bname, DEFAULT_SPEED, DEFAULT_FORMAT, DEFAULT_CHANNELS);

      ret = decode_sound (dsp, fd, st.st_size, DEFAULT_FORMAT, DEFAULT_CHANNELS,
                          DEFAULT_SPEED, NULL);
      goto done;
    }

  if (strcmp (suffix, ".cdr") == 0 || strcmp (suffix, ".CDR") == 0)
    {
      print_msg (VERBOSEM, "%s: Playing CD-R (cdwrite) file.\n", bname);

      ret = decode_sound (dsp, fd, st.st_size, AFMT_S16_BE, 2, 44100, NULL);
      goto done;
    }


  if (strcmp (suffix, ".raw") == 0 || strcmp (suffix, ".RAW") == 0)
    {
      print_msg (VERBOSEM, "%s: Playing RAW file.\n", bname);

      ret = decode_sound (dsp, fd, st.st_size, DEFAULT_FORMAT, DEFAULT_CHANNELS,
                          DEFAULT_SPEED, NULL);
      goto done;
    }

  print_msg (ERRORM, "%s: Unrecognized audio file type.\n", filename);
done:
  if (fd != -1) close (fd);

#if 0
  ioctl (fd, SNDCTL_DSP_SYNC, NULL);
#endif
  return ret;
seekerror:
  close (fd);
  return E_DECODE;
}

/*
 * Generalized parser for chunked files, especially IFF based ones - handles
 * WAV, AIFF, AIFC, CAF, W64, RF64, 8SVX, 16SV and MAUD.
 */
static errors_t
play_iff (dspdev_t * dsp, const char * filename, int fd, unsigned char * buf,
          int type, parser_t * p)
{
  int ret;

  sbig_t rbytes;
  const chunk_functions_t * i, * oi;
  file_t f = { {
    256, 496, 7, 4, {
      {256, 0},
      {512, -256},
      {0, 0},
      {192, 64},
      {240, 0},
      {460, -208},
      {392, -232} },
    DEFAULT_CHANNELS
  } };

  if (force_fmt != 0) f.format = force_fmt;
  else f.format = AFMT_S16_LE;
  f.channels = DEFAULT_CHANNELS;
  f.speed = DEFAULT_SPEED;
  f.fd = fd;
  f.filename = filename;
  f.cur_size = 12;
  f.fut_size = 12;
  f.type = type;

  rbytes = p->init (&f, buf);
  if (rbytes < 0) return E_DECODE;
  if (verbose > 1)
    print_msg (VERBOSEM, "FORM len = %u\n", f.total_size);

  if (p->perfile) oi = p->perfile;
  else if (p->common) oi = p->common;
  else return E_DECODE;
  while (!p->iterator (&f, buf, rbytes)) {
    for (i = oi; i->id != 0;) {
      if (i->id != f.chunk_id) {
        i++;
        if ((i->id == 0) && (p->common) && (!i->read_chunk_f)) i = p->common;
        continue;
      }
      if (f.chunk_size < i->d_chunk_size) {
        print_msg (ERRORM, "%c%c%c%c chunk's size (" _PRIbig_t ") is smaller "
                   "than the expected size (%d)!\n", buf[0], buf[1], buf[2],
                   buf[3], f.chunk_size, i->d_chunk_size);
        break;
      }
      if (i->read_chunk_f != READ_NONE) {
        int rlen = (f.chunk_size > P_READBUF_SIZE)?P_READBUF_SIZE:f.chunk_size;

        if ((i->read_chunk_f == READ_PART) && (f.chunk_size >= i->d_chunk_size))
          rlen = i->d_chunk_size;
        if ((rlen = p->read (&f, buf, rlen)) < 0) goto nexta;
        rbytes = rlen;
      } else {
        rbytes = f.chunk_size;
      }
      if (i->f == NULL) break;
      ret = i->f (f.chunk_id, buf, rbytes, &f);
      if (ret == CP_PLAY_NOW) {
        if ((i->read_chunk_f == READ_NONE) && (i->d_chunk_size))
          ossplay_lseek (f.fd, i->d_chunk_size, SEEK_CUR);
        goto stdinext;
      }
      if (ret == CP_STOP_READING) goto nexta;
      if (ret) return ret;
      break;
    }

    if ((f.chunk_size >= rbytes) && (f.fut_size < f.total_size))
      if (ossplay_lseek (f.fd, f.fut_size - f.cur_size - f.cpos, SEEK_CUR) < 0)
        break;
    rbytes = 0;
  }

nexta:
  if ((f.found & COMM_FOUND) == 0) {
    print_msg (ERRORM, "%s: Couldn't find format chunk!\n", filename);
    return E_DECODE;
  }

  if ((f.found & SSND_FOUND) == 0) {
    print_msg (ERRORM, "%s: Couldn't find sound chunk!\n", filename);
    return E_DECODE;
  }

  if ((type == AIFC_FILE) && ((f.found & FVER_FOUND) == 0))
    print_msg (WARNM, "%s: Couldn't find AIFC FVER chunk.\n", filename);

  if (ossplay_lseek (f.fd, f.sound_loc, SEEK_SET) == -1) {
    perror_msg (filename);
    return E_DECODE;
  }

stdinext:
  if (verbose)
    print_verbose_fileinfo (filename, type, f.format, f.channels, f.speed);

  return decode_sound (dsp, fd, f.sound_size, f.format, f.channels, f.speed,
                       (void *)&(f.msadpcm_val));
}

/*ARGSUSED*/
static errors_t
play_au (dspdev_t * dsp, const char * filename, int fd, unsigned char * hdr,
         int l)
{
  int channels = 1, format = AFMT_S8, speed = 11025;
  big_t filelen;
  uint32 fmt = 0, i, p = 24, an_len = 0;

  p = be_int (hdr + 4, 4);
  fmt = be_int (hdr + 12, 4);
  speed = be_int (hdr + 16, 4);
  channels = be_int (hdr + 20, 4);
  if (memcmp (hdr + 8, "\xFF\xFF\xFF\xFF", 4)) {
    filelen = be_int (hdr + 8, 4);
    if (verbose > 1)
      print_msg (VERBOSEM, "%s: Filelen: " _PRIbig_t "\n", filename, filelen);
  } else {
    struct stat st;

    if (from_stdin || (fstat (fd, &st) == -1)) filelen = BIG_SPECIAL;
    else filelen = st.st_size - 24 - p;
    if (verbose > 1)
      print_msg (VERBOSEM, "%s: Filelen: unspecified\n", filename);
  }
  if (verbose > 2) print_msg (VERBOSEM, "%s: Offset: %u\n", filename, p);

  if (force_fmt == 0) switch (fmt)
    {
    case 1:
      format = AFMT_MU_LAW;
      break;

    case 2:
      format = AFMT_S8;
      break;

    case 3:
      format = AFMT_S16_BE;
      break;

    case 4:
      format = AFMT_S24_PACKED_BE;
      break;

    case 5:
      format = AFMT_S32_BE;
      break;

    case 6:
      format = AFMT_FLOAT32_BE;
      break;

    case 7:
      format = AFMT_DOUBLE64_BE;
      break;

    case 23:
    case 24:
    case 25:
    case 26:
      print_msg (ERRORM, "%s: G.72x ADPCM encoded .au files are not supported\n",
                 filename);
      return E_FORMAT_UNSUPPORTED;

    case 27:
      format = AFMT_A_LAW;
      break;

    default:
      print_msg (ERRORM, "%s: Unknown encoding %d.\n", filename, fmt);
      return E_FORMAT_UNSUPPORTED;
    }

  if (verbose)
    {
      print_verbose_fileinfo (filename, AU_FILE, format, channels, speed);

      if (p > 24)
        {
          unsigned char * tag;

          if (p > 1047) an_len = 1023;
          else an_len = p - 24;
          if (read (fd, hdr, an_len) < an_len)
            {
              print_msg (ERRORM, "%s: Can't read %u bytes from pos 24\n",
                         filename, an_len);
              return E_DECODE;
            }

          tag = hdr + an_len;
          for (i = 0; i < an_len; i++)
            {
              if (!isprint (hdr[i])) hdr[i] = ' ';
              else tag = hdr + i + 1;
            }
          *tag = '\0';
          print_msg (VERBOSEM, "%s: Annotations: %s\n", filename, hdr);
        }
    }

  if (ossplay_lseek (fd, p - l - an_len, SEEK_CUR) == -1)
    {
      perror_msg (filename);
      print_msg (ERRORM, "Can't seek to the data chunk\n");
      return E_DECODE;
    }

  return decode_sound (dsp, fd, filelen, format, channels, speed, NULL);
}

/*ARGSUSED*/
static errors_t
play_voc (dspdev_t * dsp, const char * filename, int fd, unsigned char * hdr,
          int l)
{
#define VREAD(fd, buf, len) \
  do { \
    if (read (fd, buf, len) < len) \
      { \
        print_msg (ERRORM, "%s: Can't read %d bytes at pos %d\n", \
                   filename, len, l); \
        return E_DECODE; \
      } \
    pos += len; \
  } while (0)

  uint32 blklen, data_offs, fmt, id, len, loopcount = 0, loopoffs = 4,
         pos = l + 7, tmp, vers;
  unsigned char buf[256], block_type;
  flag plock = 0;
  int speed = 11025, channels = 1, bits = 8, format = AFMT_U8;
  errors_t ret;

  if (read (fd, hdr + 19, 7) < 7)
    {
      print_msg (ERRORM, "%s: Not a valid .VOC file\n", filename);
      return E_DECODE;
    }

  data_offs = le_int (hdr + 0x14, 2);
  vers = le_int (hdr + 0x16, 2);
  id = le_int (hdr + 0x18, 2);

  if ((((~vers) + 0x1234) & 0xffff) != id)
    {
      print_msg (ERRORM, "%s: Not a valid .VOC file\n", filename);
      return E_DECODE;
    }

  print_msg (VERBOSEM, "Playing .VOC file %s\n", filename);

   /*LINTED*/ while (1)
    {
      if (ossplay_lseek (fd, data_offs - pos, SEEK_CUR) == -1)
        {
          print_msg (ERRORM, "%s: Can't seek to pos %d\n", filename, data_offs);
          return E_DECODE;
        }
      pos = data_offs + 4;

      if ((tmp = read (fd, buf, 1)) < 1)
        {
          /* Don't warn when read returns 0 - it may be end of file. */
          if (tmp != 0)
            print_msg (ERRORM,
                       "%s: Can't read 1 byte at pos %d\n", filename, l);
          return E_DECODE;
        }

      block_type = buf[0];

      if (block_type == 0)
	return E_OK;			/* End */

      if (read (fd, buf, 3) != 3)
	{
	  print_msg (ERRORM, "%s: Truncated .VOC file (%d)\n",
		     filename, buf[0]);
	  return E_DECODE;
	}

      blklen = len = le_int (buf, 3);

      if (verbose > 2)
	print_msg (VERBOSEM, "%s: %0x: Block type %d, len %d\n",
		   filename, data_offs, block_type, len);
      switch (block_type)
	{

	case 1:		/* Sound data buf */
          if (!plock)
            {
	      VREAD (fd, buf, 2);

	      tmp = 256 - buf[0];	/* Time constant */
	      speed = (1000000 + tmp / 2) / tmp / channels;

              fmt = buf[1];
              len -= 2;

               if (force_fmt != 0) break;
               switch (fmt)
                 {
                   case 0: format = AFMT_U8; break;
                   case 1: format = AFMT_CR_ADPCM_4; break;
                   case 2: format = AFMT_CR_ADPCM_3; break;
                   case 3: format = AFMT_CR_ADPCM_2; break;
                   case 4: format = AFMT_S16_LE; break;
                   case 6: format = AFMT_A_LAW; break;
                   case 7: format = AFMT_MU_LAW; break;
                   default:
                     print_msg (ERRORM,
                                "%s: encoding %d is not supported\n",
                                filename, fmt);
                     return E_FORMAT_UNSUPPORTED;
                 }
            }

	case 2:		/* Continuation data */
          if ((ret = decode_sound(dsp, fd, len, format, channels, speed, NULL)))
            return ret;
          pos += len;
	  break;

	case 3:		/* Silence */
	  VREAD (fd, buf, 3);
	  len = le_int (buf, 2);
	  tmp = 256 - buf[2];	/* Time constant */
	  speed = (1000000 + tmp / 2) / tmp;
	  if ((ret = silence (dsp, len, speed))) return ret;
	  break;

        case 5: 	/* Text */
          if (!quiet)
            {
              size_t i;

              if (len > 256) len = 256;
              VREAD (fd, buf, len);
              for (i = 0; i < len; i++) if (!isprint (buf[i])) buf[i] = '.';
              buf[len-1] = '\0';
              print_msg (NORMALM, "Text: %s\n", buf);
            }
          break;

        case 6:		/* Loop start */
          VREAD (fd, buf, 2);
          loopoffs = data_offs + blklen + 4;
          loopcount = le_int (buf, 2);
          break;

        case 7:		/* End of repeat loop */
          if (loopcount != 0xffff) loopcount--;

          /* Set "return" point. Compensate for increment of data_offs. */
          if (loopcount > 0) data_offs = loopoffs - blklen - 4;

          break;

        case 8:		/* Sampling parameters */
          VREAD (fd, buf, 4);

          speed = 256000000/(channels * (65536 - le_int (buf, 2)));
          channels = buf[3] + 1;
          fmt = buf[2];
          plock = 1;

          if (force_fmt != 0) break;
          switch (fmt)
            {
              case 0: format = AFMT_U8; break;
              case 1: format = AFMT_CR_ADPCM_4; break;
              case 2: format = AFMT_CR_ADPCM_3; break;
              case 3: format = AFMT_CR_ADPCM_2; break;
              case 4: format = AFMT_S16_LE; break;
              case 6: format = AFMT_A_LAW; break;
              case 7: format = AFMT_MU_LAW; break;
              default:
                print_msg (ERRORM,
                           "%s: encoding %d is not supported\n", filename, fmt);
                return E_FORMAT_UNSUPPORTED;
            }
          break;

        case 9:		/* New format sound data */
          VREAD (fd, buf, 12);

          len -= 12;

          speed = le_int (buf, 3);
          bits = buf[4];
          channels = buf[5];
          fmt = le_int (buf + 6, 2);

          if (force_fmt == 0) switch (fmt)
            {
              case 0: format = AFMT_U8; break;
              case 1: format = AFMT_CR_ADPCM_4; break;
              case 2: format = AFMT_CR_ADPCM_3; break;
              case 3: format = AFMT_CR_ADPCM_2; break;
              case 4: format = AFMT_S16_LE; break;
              case 6: format = AFMT_A_LAW; break;
              case 7: format = AFMT_MU_LAW; break;
              default:
                print_msg (ERRORM,
                           "%s: encoding %d is not supported\n", filename, fmt);
                return E_FORMAT_UNSUPPORTED;
            }

          if ((ret = decode_sound(dsp, fd, len, format, channels, speed, NULL)))
            return ret;
          pos += len;
	  break;
	}

      if (block_type != 8) plock = 0;
      data_offs += blklen + 4;
    }
  /* return 0; */ /* Not reached */
}

static void
print_verbose_fileinfo (const char * filename, int type, int format,
                        int channels, int speed)
{
  char chn[32];
  const char * fmt = "";

  if (force_speed) speed = force_speed;
  switch (type)
    {
      case WAVE_FILE:
      case WAVE_FILE_BE:
        print_msg (VERBOSEM, "Playing WAVE file %s, ", filename); break;
      case AIFC_FILE:
        print_msg (VERBOSEM, "Playing AIFC file %s, ", filename); break;
      case AIFF_FILE:
        print_msg (VERBOSEM, "Playing AIFF file %s, ", filename); break;
      case CAF_FILE:
        print_msg (VERBOSEM, "Playing CAF file %s, ", filename); break;
      case AU_FILE:
        print_msg (VERBOSEM, "Playing AU file %s, ", filename); break;
      case _8SVX_FILE:
        print_msg (VERBOSEM, "Playing 8SVX file %s, ", filename); break;
      case _16SV_FILE:
        print_msg (VERBOSEM, "Playing 16SV file %s, ", filename); break;
      case MAUD_FILE:
        print_msg (VERBOSEM, "Playing MAUD file %s, ", filename); break;
      case W64_FILE:
        print_msg (VERBOSEM, "Playing W64 file %s, ", filename); break;
      case OGG_FILE:
        print_msg (VERBOSEM, "Playing OGG file %s, ", filename); break;
    }

  if (channels == 1)
    strcpy (chn, "mono");
  else if (channels == 2)
    strcpy (chn, "stereo");
  else
    snprintf (chn, sizeof(chn), "%d channels", channels);

  switch (format)
    {
       case AFMT_QUERY: fmt = "Invalid format"; break;
       case AFMT_MAC_IMA_ADPCM:
       case AFMT_MS_IMA_ADPCM:
       case AFMT_IMA_ADPCM: fmt = "IMA ADPCM"; break;
       case AFMT_MS_IMA_ADPCM_3BITS: fmt = "3BIT DVI ADPCM"; break;
       case AFMT_MS_ADPCM: fmt = "MS-ADPCM"; break;
       case AFMT_MU_LAW: fmt = "mu-law"; break;
       case AFMT_A_LAW: fmt = "A-law"; break;
       case AFMT_U8:
       case AFMT_S8: fmt = "8 bits"; break;
       case AFMT_S16_LE:
       case AFMT_S16_BE:
       case AFMT_U16_LE:
       case AFMT_U16_BE: fmt = "16 bits"; break;
       case AFMT_S24_LE:
       case AFMT_S24_BE:
       case AFMT_S24_PACKED_BE:
       case AFMT_S24_PACKED: fmt = "24 bits"; break;
       case AFMT_SPDIF_RAW:
       case AFMT_S32_LE:
       case AFMT_S32_BE: fmt = "32 bits"; break;
       case AFMT_FLOAT32_LE:
       case AFMT_FLOAT32_BE: fmt = "32 bit float"; break;
       case AFMT_FLOAT: fmt = "float"; break;
       case AFMT_DOUBLE64_LE:
       case AFMT_DOUBLE64_BE: fmt = "64 bit float"; break;
       case AFMT_VORBIS: fmt = "vorbis"; break;
       case AFMT_MPEG: fmt = "mpeg"; break;
       case AFMT_FIBO_DELTA: fmt = "fibonacci delta"; break;
       case AFMT_EXP_DELTA: fmt = "exponential delta"; break;
    }
  print_msg (VERBOSEM, "%s/%s/%d Hz\n", fmt, chn, speed);
}

static int
iff_init (file_t * f, unsigned char * buf)
{
  struct stat st;

  if (f->type != WAVE_FILE) f->ne_int = be_int;
  else f->ne_int = le_int;
  if (f->type == _8SVX_FILE) f->format = AFMT_S8;
  else if (f->type == WAVE_FILE) f->format = AFMT_S16_LE;
  else f->format = AFMT_S16_BE;
  f->total_size = f->ne_int (buf + 4, 4) + 8;

  if (from_stdin) return 0;
  if (fstat (f->fd, &st) == -1) return 0;
  if (st.st_size < f->total_size) {
    f->total_size = st.st_size;
    print_msg (NOTIFYM, "%s: File size is smaller than the form size!\n",
               f->filename);
  }
  return 0;
}

static ssize_t
iff_read (file_t * f, unsigned char * buf, size_t l)
{
  ssize_t ret;

  if (l > P_READBUF_SIZE) l = P_READBUF_SIZE;
  if ((ret = read (f->fd, buf, l)) < l) return CP_STOP_READING;
  f->cpos += ret;
  return ret;
}

#if 0
static ssize_t
iff_seek (file_t * f, off_t off, int flags)
{
  off_t l;

  /* We only do SEEK_CUR since we want to be able to stream from stdin */
  if (flags != SEEK_CUR) return -1;
  if (off <= 0) return 0;

  l = ossplay_lseek (f->fd, off, flags);
  return l;
}
#endif

static int
iff_iterator (file_t * f, unsigned char * buf, int l)
{
  f->cur_size = f->fut_size + 8;
  if (f->cur_size >= f->total_size) return 1;

  if (read (f->fd, buf, 8) < 8) {
    print_msg (ERRORM, "%s: Cannot read chunk header at pos %u\n",
               f->filename, f->cur_size);
    if ((f->found & SSND_FOUND) && (f->found & COMM_FOUND))
      return CP_STOP_READING;
    return E_DECODE;
  }
  f->chunk_id = be_int (buf, 4);
  f->chunk_size = f->ne_int (buf + 4, 4);
  f->cpos = 0;
  f->fut_size += f->chunk_size + (f->chunk_size & 1) + 8;

  if (verbose > 3)
    print_msg (VERBOSEM, "%s: Reading chunk %c%c%c%c, size %d, pos %d, next %d\n",
               f->filename, buf[0], buf[1], buf[2], buf[3], f->chunk_size,
               f->cur_size - 8, f->fut_size);

  if (f->chunk_size == 0) {
    print_msg (NOTIFYM, "%s: Chunk size equals 0 (pos: %u)!\n",
               f->filename, f->cur_size);
    if ((f->found & SSND_FOUND) && (f->found & COMM_FOUND))
      return CP_STOP_READING;
    return iff_iterator (f, buf, l);
  }

  return 0;
}

static int
iff_comment_parse (uint32 id, unsigned char * buf, big_t len, file_t * f)
{
  unsigned char * tag;
  uint32 i;

  if (!verbose) return 0;

  print_msg (STARTM,  "%s: ", f->filename);
  switch (id) {
    case H('N', 'A', 'M', 'E'):
      print_msg (CONTM, "Name: ");
      break;
    case H('A', 'U', 'T', 'H'):
      print_msg (CONTM, "Author: ");
      break;
    case H('(', 'c', ')', ' '):
      print_msg (CONTM, "Copyright: ");
      break;
    case H('A', 'N', 'N', 'O'):
      print_msg (CONTM, "Annonations: ");
      break;
    /* Should never be reached */
    default: return 0;
  }

  tag = buf + len + 1;
  for (i = 0; i < len; i++) {
    if (!isprint (buf[i])) buf[i] = ' ';
    else tag = buf + i + 1;
  }
  *tag = '\0';
  print_msg (ENDM, "%s\n", buf);
  return 0;
}

#define BITS2SFORMAT(endian) \
  do { \
    if (force_fmt == 0) switch (bits) \
      { \
         case 8: f->format = AFMT_S8; break; \
         case 16: f->format = AFMT_S16_##endian; break; \
         case 24: f->format = AFMT_S24_PACKED_##endian; break; \
         case 32: f->format = AFMT_S32_##endian; break; \
         default: f->format = AFMT_S16_##endian; break; \
     } break; \
  } while (0)

static int
aiff_comm_parse (uint32 id, unsigned char * buf, big_t len, file_t * f)
{
  int bits;

  f->channels = be_int (buf, 2);
#if 0
  num_frames = be_int (buf + 2, 4); /* ossplay doesn't use this */
#endif
  bits = be_int (buf + 6, 2);
  bits += bits % 8;
  f->msadpcm_val.bits = bits;
  BITS2SFORMAT (BE);
  {
    /*
     * Conversion from SANE's IEEE-754 extended 80-bit to long double.
     * We take some shortcuts which don't affect this application.
     * See http://www.mactech.com/articles/mactech/Vol.06/06.01/SANENormalized/
     * for format into.
     */
    int exp;
    ldouble_t COMM_rate = 0;

    exp = ((buf[8] & 127) << 8) + buf[9] - 16383;
#if 0
    /*
     * This part of the mantissa will typically be resolved to
     * sub-Hz rates which we don't support anyway.
     */
    COMM_rate = (buf[14] << 24) + (buf[15] << 16) +
                (buf[16] << 8) + buf[17];
    COMM_rate /= 1L << 32;
#endif
    COMM_rate += ((buf[10] & 127) << 24) + (buf[11] << 16) +
                 (buf[12] << 8) + buf[13];
    COMM_rate = ossplay_ldexpl (COMM_rate, exp-31);
    if (buf[10] & 128)
      COMM_rate += ossplay_ldexpl (1, exp); /* Normalize bit */
    if (buf[8] & 128) COMM_rate = -COMM_rate; /* Sign bit */
    if ((exp == 16384) || (COMM_rate <= 0)) {
      print_msg (ERRORM, "Invalid sample rate!\n");
      return E_DECODE;
    }
    f->speed = COMM_rate;
  }
  f->found |= COMM_FOUND;
  return 0;
}

static int
aifc_comm_parse (uint32 id, unsigned char * buf, big_t len, file_t * f)
{
  int ret, bits;

  if ((ret = aiff_comm_parse (id, buf, len, f))) return ret;

  bits = f->msadpcm_val.bits;
  f->msadpcm_val.bits = 0;

  if ((force_fmt != 0) || (len < 22)) return 0;
  switch (be_int (buf + 18, 4)) {
    case H('N', 'O', 'N', 'E'): break;
    case H('i', 'n', '1', '6'): f->format = AFMT_S16_BE; break;
    case H('i', 'n', '2', '4'): f->format = AFMT_S24_BE; break;
    case H('i', 'n', '3', '2'): f->format = AFMT_S32_BE; break;
    case H('n', 'i', '2', '4'): f->format = AFMT_S24_LE; break;
    case H('2', '3', 'n', 'i'):
    case H('n', 'i', '3', '2'): f->format = AFMT_S32_LE; break;
    /*
     * sowt/tows were intended as 16 bits only, but some recording
     * programs misinterpret this. We can try to handle that,
     * since complaint programs set the bits field to 16 anyway.
     * (See QT doc chap.4 section 3).
     */
    case H('s', 'o', 'w', 't'): BITS2SFORMAT (LE); break;
    case H('t', 'w', 'o', 's'): BITS2SFORMAT (BE); break;
    case H('r', 'a', 'w', ' '): f->format = AFMT_U8; break;
    case H('a', 'l', 'a', 'w'):
    case H('A', 'L', 'A', 'W'): f->format = AFMT_A_LAW; break;
    case H('u', 'l', 'a', 'w'):
    case H('U', 'L', 'A', 'W'): f->format = AFMT_MU_LAW; break;
    case H('i', 'm', 'a', '4'): f->format = AFMT_MAC_IMA_ADPCM; break;
    case H('f', 'l', '3', '2'):
    case H('F', 'L', '3', '2'): f->format = AFMT_FLOAT32_BE; break;
    case H('f', 'l', '6', '4'):
    case H('F', 'L', '6', '4'): f->format = AFMT_DOUBLE64_BE; break;
    default:
      print_msg (ERRORM,
               "%s: error: %c%c%c%c compression is not supported\n",
               f->filename, *(buf + 18), *(buf + 19),
               *(buf + 20), *(buf + 21));
      return E_FORMAT_UNSUPPORTED;
  }
  return 0;
}

static int
aiff_ssnd_parse (uint32 id, unsigned char * buf, big_t len, file_t * f)
{
  uint32 offset;

  if (f->found & SSND_FOUND) {
    print_msg (ERRORM, "%s: error: SSND hunk not singular!\n", f->filename);
    return E_DECODE;
  }
  f->found |= SSND_FOUND;

  offset = be_int (buf, 4);
#if 0
  block_size = be_int (buf + 4, 4); /* ossplay doesn't use this */
#endif
  f->sound_loc = f->cur_size + 8 + offset;
  f->sound_size = f->chunk_size - 8;
  if (verbose > 2)
    print_msg (VERBOSEM,  "DATA chunk. Offs = " _PRIbig_t ", len = " _PRIbig_t
               "\n", f->sound_loc, f->sound_size);

  if (from_stdin) return CP_PLAY_NOW;
  return 0;
}

static int
aifc_fver_parse (uint32 id, unsigned char * buf, big_t len, file_t * f)
{
  uint32 timestamp = be_int (buf, 4);
  if (timestamp != 0xA2805140)
    print_msg (WARNM, "%s: Timestamp doesn't match AIFC v1 timestamps!\n",
               f->filename);
  f->found |= FVER_FOUND;
  return 0;
}

static int
_8svx_vhdr_parse (uint32 id, unsigned char * buf, big_t len, file_t * f)
{
  f->speed = be_int (buf + 12, 2);
  f->found |= COMM_FOUND;
  if (force_fmt != 0) return 0;
  switch (buf[15]) {
    case 0: f->format = AFMT_S8; break;
    case 1: f->format = AFMT_FIBO_DELTA; break;
    case 2: f->format = AFMT_EXP_DELTA; break;
    default:
      print_msg (ERRORM, "%s: Unsupported compression %d\n",
                 f->filename, buf[15]);
      return E_FORMAT_UNSUPPORTED;
  }
  return 0;
}

static int
_16sv_vhdr_parse (uint32 id, unsigned char * buf, big_t len, file_t * f)
{
  f->speed = be_int (buf + 12, 2);
  f->found |= COMM_FOUND;
  if (force_fmt != 0) f->format = AFMT_S16_BE;
  return 0;
}

static int
maud_chan_parse (uint32 id, unsigned char * buf, big_t len, file_t * f)
{
  f->channels = be_int (buf, 4);
  if (f->channels > 2) {
    print_msg (ERRORM, "ossplay doesn't support MAUD format's "
               "%d channel configuration\n", f->channels);
    return E_CHANNELS_UNSUPPORTED;
  }
  return 0;
}

static int
maud_mhdr_parse (uint32 id, unsigned char * buf, big_t len, file_t * f)
{
  int bits = be_int (buf + 4, 2);

  BITS2SFORMAT (BE);

  if (be_int (buf + 12, 2) == 0) {
    print_msg (ERRORM, "Invalid rate!\n");
    return E_DECODE;
  }
  f->speed = be_int (buf + 8, 4) / be_int (buf + 12, 2);
  f->channels = be_int (buf + 16, 2);
  f->found |= COMM_FOUND;

  if (force_fmt != 0) return 0;
  switch (be_int (buf + 18, 2)) {
    case 0: /* NONE */ break;
    case 2: f->format = AFMT_A_LAW; break;
    case 3: f->format = AFMT_MU_LAW; break;
    case 6: f->format = AFMT_IMA_ADPCM; break;
    default:
      print_msg (ERRORM, "%s: format not supported", f->filename);
      return E_FORMAT_UNSUPPORTED;
  }

  return 0;
}

static int
wave_data_parse (uint32 id, unsigned char * buf, big_t len, file_t * f)
{
  if (f->found & SSND_FOUND) {
    print_msg (ERRORM, "%s: error: SSND hunk not singular!\n", f->filename);
    return E_DECODE;
  }

  f->sound_loc = f->cur_size;
  f->sound_size = f->chunk_size;
  f->found |= SSND_FOUND;
  if (verbose > 2)
    print_msg (VERBOSEM,  "DATA chunk. Offs = " _PRIbig_t ", len = " _PRIbig_t
               "\n", f->sound_loc, f->sound_size);

  if (from_stdin) return CP_PLAY_NOW;
  return 0;
}

/* Cool Edit can create this chunk. Also some Windows files use it */
static int
wave_disp_parse (uint32 id, unsigned char * buf, big_t len, file_t * f)
{
  unsigned char * tag;
  int i;

  if (verbose < 2) return 0;
  if (f->ne_int (buf, 4) != 1) return 0;

  buf += 4;
  tag = buf + len;
  for (i = 0; i < len-1; i++)
    {
      if (!isprint (buf[i])) buf[i] = ' ';
      else tag = buf + i + 1;
    }
  *tag = '\0';
  print_msg (VERBOSEM, "%s: %s\n", f->filename, buf);
  return 0;
}

static int
wave_list_parse (uint32 id, unsigned char * buf, big_t len, file_t * f)
{
  unsigned char * sbuf = buf + 4, * tag;
  int cssize = 4, i, slen = 4, subchunk_size;
  uint32 chunk_id, subchunk_id;

  if (!verbose) return 0;
  chunk_id = be_int (buf, 4);
  if (chunk_id != H('I', 'N', 'F', 'O')) return 0;
  do {
    subchunk_id = be_int (sbuf, 4);
    subchunk_size = f->ne_int (sbuf + 4, 4);
    if (verbose > 3)
      print_msg (VERBOSEM, "%s: Reading subchunk %c%c%c%c, size %d\n",
                 f->filename, sbuf[0], sbuf[1], sbuf[2], sbuf[3],
                 subchunk_size);
    if (subchunk_size == 0) return 0;
    cssize += subchunk_size;
    if (cssize > len) return 0;
    sbuf += 8;
    slen = subchunk_size + (subchunk_size & 1);
    switch (subchunk_id) {
      case H('I', 'A', 'R', 'L'):
        print_msg (STARTM, "%s: Archival Location: ", f->filename);
        break;
      case H('I', 'A', 'R', 'T'):
        print_msg (STARTM, "%s: Artist Name: ", f->filename);
        break;
      case H('I', 'C', 'M', 'S'):
        print_msg (STARTM, "%s: Commissioned: ", f->filename);
        break;
      case H('I', 'C', 'M', 'T'):
        print_msg (STARTM, "%s: Comment: ", f->filename);
        break;
      case H('I', 'C', 'O', 'P'):
        print_msg (STARTM, "%s: Copyright: ", f->filename);
        break;
      case H('I', 'C', 'R', 'D'):
        print_msg (STARTM, "%s: Creation date: ", f->filename);
        break;
      case H('I', 'E', 'N', 'G'):
        print_msg (STARTM, "%s: Engineer: ", f->filename);
        break;
      case H('I', 'G', 'N', 'R'):
        print_msg (STARTM, "%s: Genre: ", f->filename);
        break;
      case H('I', 'K', 'E', 'Y'):
        print_msg (STARTM, "%s: Keywords: ", f->filename);
        break;
      case H('I', 'N', 'A', 'M'):
        print_msg (STARTM, "%s: Name: ", f->filename);
        break;
      case H('I', 'P', 'R', 'D'):
        print_msg (STARTM, "%s: Product: ", f->filename);
        break;
      case H('I', 'S', 'B', 'J'):
        print_msg (STARTM, "%s: Subject: ", f->filename);
        break;
      case H('I', 'S', 'F', 'T'):
        print_msg (STARTM, "%s: Software: ", f->filename);
        break;
      case H('I', 'S', 'R', 'C'):
        print_msg (STARTM, "%s: Source: ", f->filename);
        break;
      case H('I', 'T', 'C', 'H'):
        print_msg (STARTM, "%s: Technician: ", f->filename);
        break;
      default:
        sbuf += slen;
        continue;
    }

    tag = buf + slen;
    /*
     * According to the spec, all of the above hunks contain ZSTRs,
     * so we can safely ignore the last char.
     */
    for (i = 0; i < slen-1; i++) {
      if (!isprint (sbuf[i])) sbuf[i] = ' ';
      else tag = sbuf + i + 1;
    }
    /* Remove trailing nonprintables */
    *tag = '\0';
    print_msg (ENDM, "%s\n", sbuf);
    sbuf += slen;
    cssize += 8;
  } while (cssize < len);

  return 0;
}

static int
wave_fmt_parse (uint32 id, unsigned char * buf, big_t len, file_t * f)
{
  unsigned int bits, i, x;
  int wtype = 0x1;

  if (f->found & COMM_FOUND) {
    print_msg (ERRORM, "%s: error: fmt hunk not singular!\n",
               f->filename);
    return E_DECODE;
  }

  if (force_fmt == 0) wtype = f->format = f->ne_int (buf, 2);
  if (verbose > 2)
    print_msg (VERBOSEM,  "FMT chunk: len = %u, fmt = %#x\n",
               len, f->format);

  f->msadpcm_val.channels = f->channels = f->ne_int (buf + 2, 2);
  f->speed = f->ne_int (buf + 4, 4);
#if 0
  bytes_per_sec = be_int (buf + 8, 4); /* ossplay doesn't use this */
#endif
  f->msadpcm_val.nBlockAlign = f->ne_int (buf + 12, 2);
  if (f->msadpcm_val.nBlockAlign == 0)
    print_msg (WARNM, "%s: nBlockAlign is 0!\n", f->filename);
  f->msadpcm_val.bits = bits = f->ne_int (buf + 14, 2);

  if (wtype == 0xFFFE) { /* WAVE_FORMAT_EXTINSIBLE */
    if (len < 40) {
      print_msg (ERRORM, "%s: invalid fmt chunk\n", f->filename);
      return E_DECODE;
    }
      /* TODO: parse the rest of WAVE_FORMAT_EXTENSIBLE */
    f->format = f->ne_int (buf + 24, 2);
  }
  if (force_fmt == 0) switch (f->format) {
    case 0x1: /* WAVE_FORMAT_PCM */
      bits += bits % 8;
      if (f->type == WAVE_FILE_BE) BITS2SFORMAT (BE);
      else BITS2SFORMAT (LE);
      if (bits == 8) f->format = AFMT_U8;
      break;
    case 0x2: /* WAVE_FORMAT_MS_ADPCM */
      f->format = AFMT_MS_ADPCM;
      break;
    case 0x3: /* WAVE_FORMAT_IEEE_FLOAT */
      if (bits == 32) f->format = AFMT_FLOAT32_LE;
      else if (bits == 64) f->format = AFMT_DOUBLE64_LE;
      else {
        print_msg (ERRORM, "%s: Odd number of bits (%d) for "
                   "WAVE_FORMAT_IEEE_FLOAT!\n", f->filename, bits);
        return E_FORMAT_UNSUPPORTED;
      }
      break;
    case 0x102: /* IBM_FORMAT_ALAW */
    case 0x6: /* WAVE_FORMAT_ALAW */
      f->format = AFMT_A_LAW;
      break;
    case 0x101: /* IBM_FORMAT_MULAW */
    case 0x7: /* WAVE_FORMAT_MULAW */
      f->format = AFMT_MU_LAW;
      break;
    case 0x11: /* WAVE_FORMAT_IMA_ADPCM */
      if (bits == 4) f->format = AFMT_MS_IMA_ADPCM;
      else if (bits == 3) f->format = AFMT_MS_IMA_ADPCM_3BITS;
      else {
        print_msg (ERRORM, "%s: Invalid number of bits (%d) for "
                   "WAVE_FORMAT_IMA_ADPCM!\n", f->filename, bits);
        return E_FORMAT_UNSUPPORTED;
      }
      break;
#if 0
    case 0x31: /* GSM 06.10 */
    case 0x50: /* MPEG */
    case 0x55: /* MPEG 3 */
#endif
    default:
      print_msg (ERRORM, "%s: Unsupported wave format %#x\n",
                 f->filename, f->format);
      return E_FORMAT_UNSUPPORTED;
  }
  f->found |= COMM_FOUND;

  if ((len < 20) ||
      ((f->format != AFMT_MS_ADPCM) &&
       (f->format != AFMT_MS_IMA_ADPCM) &&
       (f->format != AFMT_MS_IMA_ADPCM_3BITS)
      )
     ) return 0;
  f->msadpcm_val.wSamplesPerBlock = f->ne_int (buf + 18, 2);
  if ((f->format != AFMT_MS_ADPCM) || (len < 22)) return 0;
  f->msadpcm_val.wNumCoeff = f->ne_int (buf + 20, 2);
  if (f->msadpcm_val.wNumCoeff > 32) f->msadpcm_val.wNumCoeff = 32;

  x = 22;

  for (i = 0; (i < f->msadpcm_val.wNumCoeff) && (x < len-3); i++) {
    f->msadpcm_val.coeff[i].coeff1 = (int16) f->ne_int (buf + x, 2);
    x += 2;
    f->msadpcm_val.coeff[i].coeff2 = (int16) f->ne_int (buf + x, 2);
    x += 2;
  }
  f->msadpcm_val.wNumCoeff = i;
  return 0;
}

static int
caf_init (file_t * f, unsigned char * buf)
{
  struct stat st;

  memcpy (buf, buf+8, 4);
  if (from_stdin || (fstat (f->fd, &st) == -1)) {
    f->total_size = BIG_SPECIAL;
    return 4;
  }

  f->total_size = st.st_size;
  return 4;
}

static int
caf_iterator (file_t * f, unsigned char * buf, int l)
{
  f->cur_size = f->fut_size + 12 - l;
  if (f->cur_size >= f->total_size) return 1;

  if (read (f->fd, buf + l, 12 - l) < 12 - l) {
    print_msg (ERRORM, "%s: Cannot read chunk header at pos %u\n",
               f->filename, f->cur_size);
    if ((f->found & SSND_FOUND) && (f->found & COMM_FOUND))
      return CP_STOP_READING;
    return E_DECODE;
  }
  f->chunk_id = be_int (buf, 4);
  f->chunk_size = be_int (buf + 4, 8);
  f->cpos = 0;
  f->fut_size += f->chunk_size + 12 - l;

  if (verbose > 3)
    print_msg (VERBOSEM, "%s: Reading chunk %c%c%c%c, size %d, pos %d\n",
               f->filename, buf[0], buf[1], buf[2], buf[3], f->chunk_size,
               f->cur_size - 12);

  if (f->chunk_size == 0) {
    print_msg (NOTIFYM, "%s: Chunk size equals 0 (pos: %u)!\n",
               f->filename, f->cur_size);
    if ((f->found & SSND_FOUND) && (f->found & COMM_FOUND))
      return CP_STOP_READING;
    return caf_iterator (f, buf, l);
  }

  return 0;
}

static int
caf_data_parse (uint32 id, unsigned char * buf, big_t len, file_t * f)
{
  if (f->found & SSND_FOUND) {
    print_msg (ERRORM, "%s: error: SSND hunk not singular!\n", f->filename);
    return E_DECODE;
  }

#if 0
  uint32 editcount = be_int (buf, 4);
#endif
  f->sound_loc = f->cur_size + 4;
  f->sound_size = f->chunk_size - 4;
  f->found |= SSND_FOUND;

  if (verbose > 2)
    print_msg (VERBOSEM,  "DATA chunk. Offs = " _PRIbig_t ", len = " _PRIbig_t
               "\n", f->sound_loc, f->sound_size);
  if (!memcmp (buf + 4, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 8)) {
    if (from_stdin) f->sound_size = BIG_SPECIAL;
  /*
   * CAF spec requires data chunks with -1 size to be last in file,
   * so we're allowed to do the calculation below.
   */
    else f->sound_size = f->total_size - f->sound_loc;
    return CP_PLAY_NOW;
  }

  if (from_stdin) return CP_PLAY_NOW;
  return 0;
}

static int
caf_desc_parse (uint32 id, unsigned char * buf, big_t len, file_t * f)
{
  int format;
  uint32 bits, bytes_per_packet, flags, frames_per_packet;

  {
    /*
     * Conversion from IEEE-754 extended 64-bit to double.
     * We take some shortcuts which don't affect this application.
     */
    int exp;
    double rate = 0;

    exp = ((buf[0] & 127) << 4) + ((buf[1] & 240) >> 4) - 1023;
#if 0
   /*
    * This part of the mantissa will typically be resolved to
    * sub-Hz rates which we don't support anyway. We can also
    * shave a few bits from buf[3] (buf[3] & 224 will do) and
    * still get the same effect.
    */
    rate = (buf[4] << 24) + (buf[5] << 16) + (buf[6] << 8) +
            buf[7];
    rate /= 1L << 32;
#endif
    rate += ((buf[1] & 15) << 16) + (buf[2] << 8) + buf[3];
    rate = ossplay_ldexpl (rate, exp-20);
    if (exp != -1023)
      rate += ossplay_ldexpl (1, exp); /* Normalize */
    if (buf[0] & 128) rate = -rate; /* Sign bit */
    if ((exp == 1024) || (rate <= 0)) {
      print_msg (ERRORM, "%s: Invalid sample rate!\n", f->filename);
      return E_DECODE;
    }
    f->speed = rate;
  }

  format = be_int (buf + 8, 4);
  flags = be_int (buf + 12, 4);
  bytes_per_packet = be_int (buf + 16, 4);
  frames_per_packet = be_int (buf + 20, 4);
  f->channels = be_int (buf + 24, 4);
  bits = be_int (buf + 28, 4);
  f->found |= COMM_FOUND;
  if (force_fmt != 0) return 0;

#define FLCHECK(fmt) do { \
  f->format = (flags & 2)?AFMT_##fmt##_LE:AFMT_##fmt##_BE; \
} while(0)

  f->format = 0;
  switch (format) {
    case H('l', 'p', 'c', 'm'):
      if ((flags & 1) && (bits != 32) && (bits != 64)) break;
      switch (bits) {
        case 8:
          if (bytes_per_packet == f->channels) f->format = AFMT_S8;
          break;
        case 16:
          if (bytes_per_packet == 2*f->channels) FLCHECK (S16);
          break;
        case 24:
          if (bytes_per_packet == 3*f->channels) FLCHECK (S24_PACKED);
        case 32:
          if (bytes_per_packet == 4*f->channels) {
            if (flags & 1) FLCHECK(FLOAT32);
            else FLCHECK (S32);
          }
          break;
        case 64:
          if (flags & 1) FLCHECK (DOUBLE64);
        default: break;
      }
      break;
    case H('a', 'l', 'a', 'w'): f->format = AFMT_A_LAW; break;
    case H('u', 'l', 'a', 'w'): f->format = AFMT_MU_LAW; break;
    case H('i', 'm', 'a', '4'): f->format = AFMT_MAC_IMA_ADPCM; break;
    default: break;
  }
  if (f->format == 0) {
    print_msg (ERRORM, "%s: \"%c%c%c%c\" format (bits %d, bytes per "
               "%d, flags 0x%X) is not supported\n", f->filename, buf[8],
               buf[9], buf[10], buf[11], bits, bytes_per_packet, flags);
    return E_FORMAT_UNSUPPORTED;
  }
  return 0;
}

static int
w64_init (file_t * f, unsigned char * buf)
{
  struct stat st;

  f->ne_int = le_int;
  f->format = AFMT_S16_LE;
  if (read (f->fd, buf, 8) < 8) return -1;
  f->total_size = le_int (buf, 8);
  f->fut_size = 40;
  ossplay_lseek (f->fd, 16, SEEK_CUR);

  if (from_stdin) return 0;
  if (fstat (f->fd, &st) == -1) return 0;
  if (st.st_size < f->total_size) {
    f->total_size = st.st_size;
    print_msg (NOTIFYM, "%s: File size is smaller than the form size!\n",
               f->filename);
  }
  return 0;
}

static int
w64_iterator (file_t * f, unsigned char * buf, int l)
{
  f->cur_size = f->fut_size + 24;
  if (f->cur_size >= f->total_size) return 1;

  if (read (f->fd, buf, 24) < 24) {
    print_msg (ERRORM, "%s: Cannot read chunk header at pos %u\n",
               f->filename, f->cur_size);
    if ((f->found & SSND_FOUND) && (f->found & COMM_FOUND))
      return CP_STOP_READING;
    return E_DECODE;
  }

  /* Only WAVE chunks are supported, so we can ignore the rest of the GUID */
  f->chunk_id = be_int (buf, 4);
  f->chunk_size = f->ne_int (buf + 16, 8);
  f->cpos = 0;
  f->fut_size += f->chunk_size;
  f->chunk_size -= 24;

  if (verbose > 3)
    print_msg (VERBOSEM, "%s: Reading chunk %c%c%c%c, size " _PRIbig_t
               ", pos %d\n", f->filename, buf[0], buf[1], buf[2], buf[3],
               f->chunk_size - 24, f->cur_size);

  if (f->chunk_size == 0) {
    print_msg (NOTIFYM, "%s: Chunk size equals 0 (pos: %u)!\n",
               f->filename, f->cur_size);
    if ((f->found & SSND_FOUND) && (f->found & COMM_FOUND))
      return CP_STOP_READING;
    return w64_iterator (f, buf, l);
  }

  return 0;
}
