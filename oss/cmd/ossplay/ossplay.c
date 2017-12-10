/*
 * Purpose: Sources for the ossplay audio player and for the ossrecord
 *          audio recorder shipped with OSS.
 *
 * Description:
 * OSSPlay is a audio file player that supports most commonly used uncompressed
 * audio formats (.wav, .snd, .au, .aiff). It doesn't play compressed formats
 * such as MP3.
 * OSSRecord is a simple file recorder. It can write simple file formats
 * (.wav, .au, .aiff).
 *
 * This file contains the audio backend and misc. functions.
 *
 * This program is bit old and it uses some OSS features that may no longer be
 * required.
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
#include "ossplay_parser.h"
#include "ossplay_wparser.h"

#include <signal.h>
#include <strings.h>
#include <unistd.h>

unsigned int amplification = 100;
int eflag = 0, force_speed = 0, force_fmt = 0, force_channels = 0,
    overwrite = 1, verbose = 0, quiet = 0;
flag from_stdin = 0, int_conv = 0, level_meters = 0, loop = 0, 
     raw_file = 0, raw_mode = 0;
double seek_time = 0;
long seek_byte = 0;
off_t (*ossplay_lseek) (int, off_t, int) = lseek;

char script[512] = "";
unsigned int nfiles = 1;
double datalimit = 0;
fctypes_t type = WAVE_FILE;

const format_t format_a[] = {
  {"S8",		AFMT_S8,		CRP,		AFMT_S16_NE},
  {"U8",		AFMT_U8,		CRP,		AFMT_S16_NE},
  {"S16_LE",		AFMT_S16_LE,		CRP,		AFMT_S16_NE},
  {"S16_BE",		AFMT_S16_BE,		CRP,		AFMT_S16_NE},
  {"U16_LE",		AFMT_U16_LE,		CRP,		AFMT_S16_NE},
  {"U16_BE",		AFMT_U16_BE,		CRP,		AFMT_S16_NE},
  {"S24_LE",		AFMT_S24_LE,		CRP,		0},
  {"S24_BE",		AFMT_S24_BE,		CRP,		0},
  {"S32_LE",		AFMT_S32_LE,		CRP,		AFMT_S32_NE},
  {"S32_BE",		AFMT_S32_BE,		CRP,		AFMT_S32_NE},
  {"A_LAW",		AFMT_A_LAW,		CRP,		AFMT_S16_NE},
  {"MU_LAW",		AFMT_MU_LAW,		CRP,		AFMT_S16_NE},
  {"FLOAT32_LE",	AFMT_FLOAT32_LE,	CP,		0},
  {"FLOAT32_BE",	AFMT_FLOAT32_BE,	CP,		0},
  {"DOUBLE64_LE",	AFMT_DOUBLE64_LE,	CP,		0},
  {"DOUBLE64_BE",	AFMT_DOUBLE64_BE,	CP,		0},
  {"S24_PACKED",	AFMT_S24_PACKED,	CRP,		0},
  {"S24_PACKED_BE",	AFMT_S24_PACKED_BE,	CP,		0},
  {"IMA_ADPCM",		AFMT_IMA_ADPCM,		CP,		0},
  {"IMA_ADPCM_3BITS",	AFMT_MS_IMA_ADPCM_3BITS,CP,		0},
  {"MS_ADPCM",		AFMT_MS_ADPCM,		CP,		0},
  {"CR_ADPCM_2",	AFMT_CR_ADPCM_2,	CP,		0},
  {"CR_ADPCM_3",	AFMT_CR_ADPCM_3,	CP,		0},
  {"CR_ADPCM_4",	AFMT_CR_ADPCM_4,	CP,		0},
  {"SPDIF_RAW",		AFMT_SPDIF_RAW,		CR,		0},
  {"FIBO_DELTA",	AFMT_FIBO_DELTA,	CP,		0},
  {"EXP_DELTA",		AFMT_EXP_DELTA,		CP,		0},
  {NULL,		0,			CP,		0}
};

static const container_t container_a[] = {
  {"RAW",		RAW_FILE,	AFMT_S16_LE,	2,	44100},
  {"WAV",		WAVE_FILE,	AFMT_S16_LE,	2,	48000},
  {"AU",		AU_FILE,	AFMT_MU_LAW,	1,	8000},
  {"AIFF",		AIFF_FILE,	AFMT_S16_BE,	2,	48000},
  {"CAF",		CAF_FILE,	AFMT_S16_NE,	2,	48000},
  {NULL,		RAW_FILE,	0,		0,	0}
}; /* Order should match fctypes_t enum so that container_a[type] works */

static void describe_error (void);
static void find_devname (char *, const char *);
static fctypes_t select_container (const char *);
static int select_format (const char *, int);
static void ossplay_usage (const char *);
static void ossrecord_usage (const char *);
static void ossplay_getint (int);
static void print_play_verbose_info (const unsigned char *, ssize_t, void *);
static void print_record_verbose_info (const unsigned char *, ssize_t, void *);

big_t
be_int (const unsigned char * p, int l)
{
  int i;
  big_t val;

  val = 0;

  for (i = 0; i < l; i++)
    {
      val = (val << 8) | p[i];
    }

  return val;
}

big_t
le_int (const unsigned char * p, int l)
{
  int i;
  big_t val;

  val = 0;

  for (i = l - 1; i >= 0; i--)
    {
      val = (val << 8) | p[i];
    }

  return val;
}

static void
describe_error (void)
{
  switch (errno)
    {
    case ENXIO:
    case ENODEV:
      print_msg (ERRORM, "\nThe device file was found in /dev but\n"
	         "there is no driver for it currently loaded.\n"
	         "\n"
	         "You can start it by executing the soundon command as\n"
	         "super user (root).\n");
      break;

    case ENOSPC:
      print_msg (ERRORM, "\nThe soundcard driver was not installed\n"
	         "properly. The system is out of DMA compatible memory.\n"
	         "Please reboot your system and try again.\n");

      break;

    case ENOENT:
      print_msg (ERRORM, "\nThe sound device file is missing from /dev.\n"
	         "You should try re-installing OSS.\n");
      break;

    case EBUSY:
      print_msg (ERRORM,
	         "\nThere is some other application using this audio device.\n"
	         "Exit it and try again.\n");
      print_msg (ERRORM,
	         "You can possibly find out the conflicting application by"
                 "looking\n",
	         "at the printout produced by command 'ossinfo -a -v1'\n");
      break;

    default:;
    }
}

static void
find_devname (char * devname, const char * num)
{
/*
 * OSS 4.0 the audio device numbering may be different from the
 * legacy /dev/dsp# numbering reported by /dev/sndstat. Try to find the
 * device name (devnode) that matches the given device number.
 *
 * Prior versions of ossplay simply used the the /dev/dsp# number.
 */
  int dev;
  int mixer_fd;
  oss_audioinfo ai;
  const char * devmixer;

  if ((devmixer = getenv("OSS_MIXERDEV")) == NULL)
     devmixer = "/dev/mixer";

  if (sscanf (num, "%d", &dev) != 1)
    {
      print_msg (ERRORM, "Invalid audio device number '%s'\n", num);
      exit (E_SETUP_ERROR);
    }

  if ((mixer_fd = open (devmixer, O_RDWR, 0)) == -1)
    {
      perror_msg (devmixer);
      print_msg (WARNM, "Warning: Defaulting to /dev/dsp%s\n", num);
      snprintf (devname, OSS_DEVNODE_SIZE, "/dev/dsp%s", num);
      return;
    }

  ai.dev = dev;

  if (ioctl (mixer_fd, SNDCTL_AUDIOINFO, &ai) == -1)
    {
      perror_msg ("SNDCTL_AUDIOINFO");
      print_msg (WARNM, "Warning: Defaulting to /dev/dsp%s\n", num);
      snprintf (devname, OSS_DEVNODE_SIZE, "/dev/dsp%s", num);
      close (mixer_fd);
      return;
    }

  strncpy (devname, ai.devnode, OSS_DEVNODE_SIZE);

  close (mixer_fd);
  return;
}

const char *
filepart (const char *name)
{
  const char * s = name;

  if (name == NULL) return "";

  while (*name)
    {
      if (name[0] == '/' && name[1] != '\0')
	s = name + 1;
      name++;
    }

  return s;
}

float
format2bits (int format)
{
  switch (format)
    {
      case AFMT_CR_ADPCM_2: return 2;
      case AFMT_CR_ADPCM_3: return 2.6666F;
      case AFMT_MS_IMA_ADPCM_3BITS: return 3;
      case AFMT_CR_ADPCM_4:
      case AFMT_MAC_IMA_ADPCM:
      case AFMT_MS_IMA_ADPCM:
      case AFMT_IMA_ADPCM:
      case AFMT_MS_ADPCM:
      case AFMT_FIBO_DELTA:
      case AFMT_EXP_DELTA: return 4;
      case AFMT_MU_LAW:
      case AFMT_A_LAW:
      case AFMT_U8:
      case AFMT_S8: return 8;
      case AFMT_VORBIS:
      case AFMT_MPEG:
      case AFMT_S16_LE:
      case AFMT_S16_BE:
      case AFMT_U16_LE:
      case AFMT_U16_BE: return 16;
      case AFMT_S24_PACKED:
      case AFMT_S24_PACKED_BE: return 24;
      case AFMT_S24_LE:
      case AFMT_S24_BE:
      case AFMT_SPDIF_RAW:
      case AFMT_FLOAT32_LE:
      case AFMT_FLOAT32_BE:
      case AFMT_S32_LE:
      case AFMT_S32_BE: return 32;
      case AFMT_DOUBLE64_LE:
      case AFMT_DOUBLE64_BE: return 64;
      case AFMT_FLOAT: return sizeof (float) * 8;
      case AFMT_QUERY:
      default: return 0;
   }
}

void
close_device (dspdev_t * dsp)
{
  if (dsp->fd == -1) return;
  close (dsp->fd);
  dsp->fd = -1;
}

void
open_device (dspdev_t * dsp)
{
  const char * devdsp;

  if (dsp->fd >= 0)
     close_device (dsp);

  dsp->format = 0; dsp->channels = 0; dsp->speed = 0;

  if ((devdsp = getenv("OSS_AUDIODEV")) == NULL)
     devdsp = "/dev/dsp";

  if (raw_mode)
    dsp->flags |= O_EXCL;	/* Disable redirection to the virtual mixer */

  if (dsp->dname[0] == '\0') strcpy (dsp->dname, devdsp);

  if ((dsp->fd = open (dsp->dname, dsp->flags, 0)) == -1)
    {
      perror_msg (dsp->dname);
      describe_error ();
      exit (E_SETUP_ERROR);
    }

  if (raw_mode)
    {
      /*
       * Disable sample rate/format conversions.
       */
      int tmp = 0;
      ioctl (dsp->fd, SNDCTL_DSP_COOKEDMODE, &tmp);
    }
}

static void
ossplay_usage (const char * prog)
{
  print_msg (HELPM, "Usage: %s [options] filename(s)\n", prog?prog:"ossplay");
  print_msg (HELPM, "  Options:  -v             Verbose output.\n");
  print_msg (HELPM, "            -q             No informative printouts.\n");
  print_msg (HELPM, "            -d<devname>    Change output device.\n");
  print_msg (HELPM, "            -g<gain>       Change gain.\n");
  print_msg (HELPM, "            -s<rate>       Change playback rate.\n");
  print_msg (HELPM, "            -f<fmt>|?      Change/Query input format.\n");
  print_msg (HELPM, "            -c<channels>   Change number of channels.\n");
  print_msg (HELPM, "            -o<playtgt>|?  Select/Query output target.\n");
  print_msg (HELPM, "            -l             Loop playback indefinitely.\n");
  print_msg (HELPM, "            -W             Treat all input as raw PCM.\n");
  print_msg (HELPM, "            -S<secs>       Start playing from offset.\n");
  print_msg (HELPM,
             "            -R             Open sound device in raw mode.\n");
  exit (E_USAGE);
}

static void
ossrecord_usage (const char * prog)
{
  print_msg (HELPM, "Usage: %s [options] filename\n", prog?prog:"ossrecord");
  print_msg (HELPM, "  Options:  -v             Verbose output.\n");
  print_msg (HELPM, "            -d<device>     Change input device.\n");
  print_msg (HELPM, "            -c<channels>   Change number of channels\n");
  print_msg (HELPM, "            -L<level>      Change recording level.\n");
  print_msg (HELPM,
             "            -g<gain>       Change gain percentage.\n");
  print_msg (HELPM, "            -s<rate>       Change recording rate.\n");
  print_msg (HELPM, "            -f<fmt|?>      Change/Query sample format.\n");
  print_msg (HELPM,
             "            -F<cnt|?>      Change/Query container format.\n");
  print_msg (HELPM, "            -l             Display level meters.\n");
  print_msg (HELPM,
             "            -i<recsrc|?>   Select/Query recording source.\n");
  print_msg (HELPM,
             "            -m<nfiles>     Repeat recording <nfiles> times.\n");
  print_msg (HELPM,
             "            -r<command>    Run <command> after recording.\n");
  print_msg (HELPM,
             "            -t<maxsecs>    Record no more than <maxsecs> in a"
             " single recording.\n");
  print_msg (HELPM,
             "            -R             Open sound device in raw mode.\n");
  print_msg (HELPM, "            -O             Do not allow overwrite.\n");
  exit (E_USAGE);
}

const char *
sample_format_name (int sformat)
{
  int i;

  for (i = 0; format_a[i].fmt != 0; i++)
    if (format_a[i].fmt == sformat)
      return format_a[i].name;

  return "";
}

static fctypes_t
select_container (const char * optstr)
{
/*
 * Handling of the -F command line option (force container format).
 *
 * Empty or "?" shows the supported container format names.
 */
  int i;

  if ((!strcmp(optstr, "?")) || (*optstr == '\0'))
    {
      print_msg (STARTM, "\nSupported container format names are:\n\n");
      for (i = 0; container_a[i].name != NULL; i++)
        print_msg (CONTM, "%s ", container_a[i].name);
      print_msg (ENDM, "\n");
      exit (0);
    }

  for (i = 0; container_a[i].name != NULL; i++)
    if (!strcasecmp(container_a[i].name, optstr))
      return container_a[i].type;

  print_msg (ERRORM, "Unsupported container format name '%s'!\n", optstr);
  exit (E_USAGE);
}

static int
select_format (const char * optstr, int dir)
{
/*
 * Handling of the -f command line option (force input format).
 *
 * Empty or "?" shows the supported format names.
 */
  int i;

  if ((!strcmp(optstr, "?")) || (*optstr == '\0'))
    {
      print_msg (STARTM, "\nSupported format names are:\n\n");
      for (i = 0; format_a[i].name != NULL; i++)
        if (dir & format_a[i].dir)
          print_msg (CONTM, "%s ", format_a[i].name);
      print_msg (ENDM, "\n");
      exit (0);
    }

  for (i = 0; format_a[i].name != NULL; i++)
    if ((format_a[i].dir & dir) && (!strcasecmp(format_a[i].name, optstr)))
      return format_a[i].fmt;

  print_msg (ERRORM, "Unsupported format name '%s'!\n", optstr);
  exit (E_USAGE);
}

void
select_playtgt (dspdev_t * dsp)
{
/*
 * Handling of the -o command line option (playback target selection).
 *
 * Empty or "?" shows the available playback sources.
 */
  int i, src;
  oss_mixer_enuminfo ei;

  if (ioctl (dsp->fd, SNDCTL_DSP_GET_PLAYTGT_NAMES, &ei) == -1)
    {
      perror_msg ("SNDCTL_DSP_GET_PLAYTGT_NAMES");
      exit (E_SETUP_ERROR);
    }

  if (ioctl (dsp->fd, SNDCTL_DSP_GET_PLAYTGT, &src) == -1)
    {
      perror_msg ("SNDCTL_DSP_GET_PLAYTGT");
      exit (E_SETUP_ERROR);
    }

  if ((dsp->playtgt[0] == '\0') || (strcmp (dsp->playtgt, "?") == 0))
    {
      print_msg (STARTM,
                 "\nPossible playback targets for the selected device:\n\n");

      for (i = 0; i < ei.nvalues; i++)
	{
	  print_msg (CONTM, "\t%s", ei.strings + ei.strindex[i]);
	  if (i == src)
	    print_msg (CONTM, " (currently selected)");
	  print_msg (CONTM, "\n");
	}
      print_msg (ENDM, "\n");
      exit (0);
    }

  for (i = 0; i < ei.nvalues; i++)
    {
      char *s = ei.strings + ei.strindex[i];
      if (strcmp (s, dsp->playtgt) == 0)
	{
	  src = i;
	  if (ioctl (dsp->fd, SNDCTL_DSP_SET_PLAYTGT, &src) == -1)
	    {
	      perror_msg ("SNDCTL_DSP_SET_PLAYTGT");
	      exit (E_SETUP_ERROR);
	    }

	  return;
	}
    }

  print_msg (ERRORM,
	     "Unknown playback target name '%s' - use -o? to get the list\n",
	     dsp->playtgt);
  exit (E_USAGE);
}

void
select_recsrc (dspdev_t * dsp)
{
/*
 * Handling of the -i command line option (recording source selection).
 *
 * Empty or "?" shows the available recording sources.
 */
  int i, src;
  oss_mixer_enuminfo ei;

  if (ioctl (dsp->fd, SNDCTL_DSP_GET_RECSRC_NAMES, &ei) == -1)
    {
      perror_msg ("SNDCTL_DSP_GET_RECSRC_NAMES");
      exit (E_SETUP_ERROR);
    }

  if (ioctl (dsp->fd, SNDCTL_DSP_GET_RECSRC, &src) == -1)
    {
      perror_msg ("SNDCTL_DSP_GET_RECSRC");
      exit (E_SETUP_ERROR);
    }

  if (dsp->recsrc[0] == '\0' || strcmp (dsp->recsrc, "?") == 0)
    {
      print_msg (STARTM,
                 "\nPossible recording sources for the selected device:\n\n");

      for (i = 0; i < ei.nvalues; i++)
	{
	  print_msg (CONTM, "\t%s", ei.strings + ei.strindex[i]);
	  if (i == src)
	    print_msg (CONTM, " (currently selected)");
	  print_msg (CONTM, "\n");
	}
      print_msg (ENDM, "\n");
      exit (0);
    }

  for (i = 0; i < ei.nvalues; i++)
    {
      char *s = ei.strings + ei.strindex[i];
      if (strcmp (s, dsp->recsrc) == 0)
	{
	  src = i;
	  if (ioctl (dsp->fd, SNDCTL_DSP_SET_RECSRC, &src) == -1)
	    {
	      perror_msg ("SNDCTL_DSP_SET_RECSRC");
	      exit (E_SETUP_ERROR);
	    }
	  return;
	}
    }

  print_msg (ERRORM,
	     "Unknown recording source name '%s' - use -i? to get the list\n",
	     dsp->recsrc);
  exit (E_USAGE);
}

errors_t
setup_device (dspdev_t * dsp, int format, int channels, int speed)
{
  int tmp;

  if (dsp->speed != speed || dsp->format != format ||
      dsp->channels != channels || dsp->fd == -1)
    {
#if 0
      ioctl (dsp->fd, SNDCTL_DSP_SYNC, NULL);
      ioctl (dsp->fd, SNDCTL_DSP_HALT, NULL);
#else
      close_device (dsp);
      open_device (dsp);
      if (dsp->playtgt != NULL)	select_playtgt (dsp);
      if (dsp->recsrc != NULL)	select_recsrc (dsp);
#endif
    }
  else
    {
      ioctl (dsp->fd, SNDCTL_SETSONG, dsp->current_songname);
      return E_OK;
    }

  /*
   * Report the current filename as the song name.
   */
  ioctl (dsp->fd, SNDCTL_SETSONG, dsp->current_songname);

  tmp = APF_NORMAL;
  ioctl (dsp->fd, SNDCTL_DSP_PROFILE, &tmp);

  tmp = format;

  if (ioctl (dsp->fd, SNDCTL_DSP_SETFMT, &tmp) == -1)
    {
      perror_msg (dsp->dname);
      print_msg (ERRORM, "Failed to select bits/sample\n");
      return E_SETUP_ERROR;
    }

  if (tmp != format)
    {
      print_msg (ERRORM, "%s doesn't support this audio format (%x/%x).\n",
                 dsp->dname, format, tmp);
      return E_FORMAT_UNSUPPORTED;
    }

  tmp = channels;

  if (ioctl (dsp->fd, SNDCTL_DSP_CHANNELS, &tmp) == -1)
    {
      perror_msg (dsp->dname);
      print_msg (ERRORM, "Failed to select number of channels.\n");
      return E_SETUP_ERROR;
    }

  if (tmp != channels)
    {
#ifdef SRC_SUPPORT
      /* We'll convert mono to stereo, so it's no use warning */
      if ((channels != 1) || (tmp != 2))
#endif
        print_msg (ERRORM, "%s doesn't support %d channels (%d).\n",
	           dsp->dname, channels, tmp);
      return E_CHANNELS_UNSUPPORTED;
    }

  tmp = speed;

  if (ioctl (dsp->fd, SNDCTL_DSP_SPEED, &tmp) == -1)
    {
      perror_msg (dsp->dname);
      print_msg (ERRORM, "Failed to select sampling rate.\n");
      return E_SETUP_ERROR;
    }

#ifndef SRC_SUPPORT
  if (tmp != speed)
    {
      print_msg (WARNM, "Warning: Playback using %d Hz (file %d Hz)\n",
	         tmp, speed);
    }
#endif

  dsp->speed = tmp;
  dsp->channels = channels;
  dsp->format = format;

  if (verbose > 1)
    print_msg (VERBOSEM, "Setup device %s/%d/%d\n",
               sample_format_name (dsp->format), dsp->channels, dsp->speed);

  if (dsp->reclevel != 0)
    {
      tmp = dsp->reclevel | (dsp->reclevel << 8);

      if (ioctl (dsp->fd, SNDCTL_DSP_SETRECVOL, &tmp) == -1)
        perror ("SNDCTL_DSP_SETRECVOL");
    }

  return E_OK;
}

static void
ossplay_getint (int signum)
{
#if 0
  if (eflag == signum + 128)
    {
      signal (signum, SIG_DFL);
      raise (signum);
    }
#endif
  eflag = signum + 128;
}

int
ossplay_parse_opts (int argc, char ** argv, dspdev_t * dsp)
{
  extern char * optarg;
  extern int optind;
  char * p;
  int c;

  while ((c = getopt (argc, argv, "FRS:Wc:d:f:g:hlo:qs:v")) != EOF)
    {
      switch (c)
	{
	case 'v':
	  verbose++;
	  quiet = 0;
	  int_conv = 2;
	  break;

	case 'R':
	  raw_mode = 1;
	  break;

	case 'q':
	  quiet++;
	  verbose = 0;
	  if (int_conv == 2) int_conv = 0;
	  break;

	case 'd':
	  if (*optarg >= '0' && *optarg <= '9')	/* Only device number given */
	    find_devname (dsp->dname, optarg);
	  else
            snprintf (dsp->dname, OSS_DEVNODE_SIZE, "%s", optarg);
	  break;

	case 'o':
          if (!strcmp(optarg, "?"))
            {
              dsp->playtgt = optarg;
              dsp->flags = O_WRONLY;
              open_device (dsp);
              select_playtgt (dsp);
            }
	  dsp->playtgt = optarg;
	  break;

	case 'f':
	  force_fmt = select_format (optarg, CP);
	  break;

	case 's':
	  sscanf (optarg, "%d", &force_speed);
	  break;

	case 'c':
	  sscanf (optarg, "%d", &force_channels);
	  break;

	case 'g':
	  sscanf (optarg, "%u", &amplification);
	  int_conv = 1;
	  break;

        case 'l':
          loop = 1;
          break;

	case 'F':
	case 'W':
	  raw_file = 1;
	  break;

	case 'S':
          c = strlen (optarg);
          if ((c > 0) && ((optarg[c - 1] == 'b') || (optarg[c - 1] == 'B')))
            {
              errno = 0;
              seek_byte = strtol (optarg, &p, 10);
              if ((*p != '\0') || (seek_byte < 0)) ossplay_usage (argv[0]);
            }
          else
            {
              errno = 0;
              seek_time = strtod (optarg, &p);
              if ((*p != '\0') || (errno) || (seek_time < 0)) ossplay_usage (argv[0]);
            }
	  break;

	default:
	  ossplay_usage (argv[0]);
	}

    }

  if (argc < optind + 1)
    ossplay_usage (argv[0]);

#ifdef SIGQUIT
  signal (SIGQUIT, ossplay_getint);
#endif
  return optind;
}

int
ossrecord_parse_opts (int argc, char ** argv, dspdev_t * dsp)
{
  char * p;
  int c;
  extern char * optarg;
  extern int optind;

  if (argc < 2)
    ossrecord_usage (argv[0]);

  while ((c = getopt (argc, argv, "F:L:MORSb:c:d:f:g:hi:lm:r:s:t:wv")) != EOF)
    switch (c)
      {
        case 'F':
          type = select_container (optarg);
          break;

        case 'L':
          dsp->reclevel = atoi (optarg);
          if (dsp->reclevel < 1 || dsp->reclevel > 100)
            {
              print_msg (ERRORM, "%s: Bad recording level '%s'\n",
                         argv[0]?argv[0]:"", optarg);
              exit (-1);
            }
          break;

        case 'M':
          force_channels = 1;
          break;

        case 'R':
          raw_mode = 1;
          break;

        case 'S':
          force_channels = 2;
          break;

        case 'b':
          c = atoi (optarg);
          c += c % 8; /* Simple WAV format always pads to a multiple of 8 */
          switch (c)
            {
              case 8: force_fmt = AFMT_U8; break;
              case 16: force_fmt = AFMT_S16_LE; break;
              case 24: force_fmt = AFMT_S24_PACKED; break;
              case 32: force_fmt = AFMT_S32_LE; break;
              default:
                print_msg (ERRORM, "Error: Unsupported number of bits %d\n", c);
                exit (E_FORMAT_UNSUPPORTED);
            }
          break;

        case 'c':
          sscanf (optarg, "%d", &force_channels);
          break;

        case 'd':
	  if (*optarg >= '0' && *optarg <= '9')	/* Only device number given */
	    find_devname (dsp->dname, optarg);
	  else
            snprintf (dsp->dname, OSS_DEVNODE_SIZE, "%s", optarg);
          break;

        case 'f':
          force_fmt = select_format (optarg, CR);
          break;

        case 'g':
	  sscanf (optarg, "%u", &amplification);
          if (amplification == 0) ossrecord_usage (argv[0]);

        case 'l':
          level_meters = 1;
          verbose = 1;
          break;

        case 'i':
          if (!strcmp(optarg, "?"))
            {
              dsp->recsrc = optarg;
              dsp->flags = O_RDONLY;
              open_device (dsp);
              select_recsrc (dsp);
            }
          dsp->recsrc = optarg;
          break;

        case 'm':
          sscanf (optarg, "%u", &nfiles);
          break;

        case 's':
          sscanf (optarg, "%d", &force_speed);
          if (force_speed == 0)
            {
              print_msg (ERRORM, "Bad sampling rate given\n");
              exit (E_USAGE);
            }
          if (force_speed < 1000) force_speed *= 1000;
          break;

        case 'r':
          c = snprintf (script, sizeof (script), "%s", optarg);
          if (((size_t)c >= sizeof (script)) || (c < 0))
            {
              print_msg (ERRORM, "-r argument is too long!\n");
              exit (E_USAGE);
            }
          break;

        case 't':
          errno = 0;
          datalimit = strtod (optarg, &p);
          if ((*p != '\0') || (errno) || (datalimit <= 0)) ossrecord_usage (argv[0]);
          break;

        case 'O':
          overwrite = 0;
          break;

        case 'w':
          break;

        case 'v':
          verbose = 1;
          break;

        case 'h':
        default:
          ossrecord_usage (argv[0]);
      }

  if (argc != optind + 1)
  /* No file or multiple file names given */
      ossrecord_usage (argv[0]);

  if (force_fmt == 0) force_fmt = container_a[type].dformat;
  if (force_channels == 0) force_channels = container_a[type].dchannels;
  if (force_speed == 0) force_speed = container_a[type].dspeed;
  switch (force_fmt)
    {
      case AFMT_S8:
      case AFMT_U8:
      case AFMT_S16_NE:
      case AFMT_S24_NE:
      case AFMT_S32_NE: break;
      default: level_meters = 0; /* Not implemented */
    }

  if ((signal (SIGSEGV, ossplay_getint) == SIG_ERR) ||
#ifdef SIGPIPE
      (signal (SIGPIPE, ossplay_getint) == SIG_ERR) ||
#endif
      (signal (SIGTERM, ossplay_getint) == SIG_ERR) ||
#ifdef SIGQUIT
      (signal (SIGQUIT, ossplay_getint) == SIG_ERR) ||
#endif
      (signal (SIGINT, ossplay_getint) == SIG_ERR))
    print_msg (WARNM, "Signal handler not set up!\n");

  if (verbose)
    {
      oss_audioinfo ai;

      ai.dev = -1;

      if (ioctl(dsp->fd, SNDCTL_ENGINEINFO, &ai) != -1)
        print_msg (VERBOSEM, "Recording from %s\n", ai.name);
   }

  return optind;
}

ldouble_t
ossplay_ldexpl (ldouble_t num, int exp)
{
  /*
   * Very simple emulation of ldexpl to avoid linking to libm or assuming
   * anything about float representation.
   */
  if (exp > 0)
    {
      while (exp > 31)
        {
          num *= 1UL << 31;
          exp -= 31;
        }
      num *= 1UL << exp;
    }
  else if (exp < 0)
    {
      while (exp < -31)
        {
          num /= 1UL << 31;
          exp += 31;
        }
      num /= 1UL << -exp;
    }

  return num;
}

static void
print_play_verbose_info (const unsigned char * buf, ssize_t l, void * metadata)
{
/*
 * Display a rough recording level meter, and the elapsed time.
 */

  verbose_values_t * val = (verbose_values_t *)metadata;

  val->secs += l/val->constant;
  if (val->secs < val->next_sec) return;
  val->next_sec += PLAY_UPDATE_INTERVAL/1000;
  /*
   * This check is done to ensure an update at the end of the playback.
   * Note that some files lie about total time, so the second condition is
   * necessary so that updates will still be constricted by PLAY_UPDATE_INTERVAL.
   */
  if ((val->next_sec > val->tsecs) && (val->secs < val->tsecs)) val->next_sec = val->tsecs;

  print_update (get_db_level (buf, l, val->format), val->secs, val->tstring);

  return;
}

static void
print_record_verbose_info (const unsigned char * buf, ssize_t l,
                           void * metadata)
{
/*
 * Display a rough recording level meter if enabled, and the elapsed time.
 */

  verbose_values_t * val = (verbose_values_t *)metadata;
  int update_dots = 1;

  val->secs += l / val->constant;

  if (val->secs >= val->next_sec)
    {
      val->next_sec += REC_UPDATE_INTERVAL/1000;
      if ((val->tsecs) && (val->next_sec > val->tsecs))
        val->next_sec = val->tsecs;
      if (level_meters)
        {
          val->secs_timer2 = val->next_sec_timer2 = val->secs;
          goto print_level;
        }
      print_record_update (-1, val->secs, val->tstring, 1);
    }
  else if ((level_meters) && (val->secs >= val->next_sec_timer2))
    {
      update_dots = 0;
print_level:
      val->next_sec_timer2 += LMETER_UPDATE_INTERVAL/1000;
      if ((val->tsecs) && (val->next_sec_timer2 > val->tsecs))
        val->next_sec_timer2 = val->tsecs;
      print_record_update (get_db_level (buf, l, val->format), val->secs_timer2,
                           val->tstring, update_dots);
    }
}

int
play (dspdev_t * dsp, int fd, big_t * datamark, big_t bsize, double total_time,
      double constant, readfunc_t * readf, decoders_queue_t * dec, seekfunc_t * seekf)
{
#define EXITPLAY(code) \
  do { \
    ossplay_free (buf); \
    ossplay_free (verbose_meta); \
    clear_update (); \
    ioctl (dsp->fd, SNDCTL_DSP_HALT_OUTPUT, NULL); \
    errno = 0; \
    return (code); \
  } while (0)

  big_t rsize = bsize;
  big_t filesize = *datamark;
  ssize_t outl;
  unsigned char * buf, * obuf, contflag = 0;
  decoders_queue_t * d;
  verbose_values_t * verbose_meta = NULL;

  buf = (unsigned char *)ossplay_malloc (bsize);

  if (verbose)
    {
      verbose_meta = setup_verbose (dsp->format,
                              format2bits(dsp->format) * dsp->channels *
                              dsp->speed / 8.0, total_time);
      if (seek_time == 0) print_play_verbose_info (NULL, 0, verbose_meta);
    }

  *datamark = 0;

  while (*datamark < filesize)
    {
      if (eflag) EXITPLAY (eflag);

      rsize = bsize;
      if (rsize > filesize - *datamark) rsize = filesize - *datamark;

      if ((seek_time != 0) && (seekf != NULL))
        {
          errors_t ret;

          ret = seekf (fd, datamark, filesize, constant, rsize, dsp->channels,
                       dec->metadata);
          if (ret == E_OK)
            {
              if (verbose)
                {
                  verbose_meta->secs = (double)seek_time;
                  verbose_meta->next_sec = (double)seek_time;
                  print_play_verbose_info (NULL, 0, verbose_meta);
                }
              seek_time = 0;
              continue;
            }
          else if (ret == SEEK_CONT_AFTER_DECODE) contflag = 1;
          else EXITPLAY (ret);
        }

      if ((outl = readf (fd, buf, rsize, dec->metadata)) <= 0)
        {
          if (errno) perror_msg ("read");
          if ((filesize != BIG_SPECIAL) && (*datamark < filesize) && (!eflag))
            {
              print_msg (NOTIFYM, "Sound data ended prematurely!\n");
            }
          EXITPLAY (eflag);
        }
      *datamark += outl;

      if (contflag)
        {
          contflag = 0;
          continue;
        }

      obuf = buf; d = dec;
      do
        {
          outl = d->decoder (&(d->outbuf), obuf, outl, d->metadata);
          obuf = d->outbuf;
          d = d->next;
        }
      while (d != NULL);

      if (verbose) print_play_verbose_info (obuf, outl, verbose_meta);
      if (write (dsp->fd, obuf, outl) == -1)
        {
          if ((errno == EINTR) && (eflag)) EXITPLAY (eflag);
          ossplay_free (buf);
          perror_msg ("audio write");
          exit (E_DECODE);
        }
    }

  ossplay_free (buf);
  ossplay_free (verbose_meta);
  clear_update ();
  return 0;
}

int
record (dspdev_t * dsp, FILE * wave_fp, const char * filename, double constant,
        double datatime, big_t * data_size, decoders_queue_t * dec)
{
#define EXITREC(code) \
  do { \
    ossplay_free (buf); \
    ossplay_free (verbose_meta); \
    clear_update (); \
    if ((eflag) && (verbose)) \
      print_msg (VERBOSEM, "\nStopped (%d).\n", eflag-128); \
    ioctl (dsp->fd, SNDCTL_DSP_HALT_INPUT, NULL); \
    return (code); \
  } while(0)

  unsigned char * buf, * obuf;
  ssize_t l, outl;
  big_t data_size_limit = *data_size;
  decoders_queue_t * d;
  verbose_values_t * verbose_meta = NULL;

  if (verbose)
    {
      verbose_meta = setup_verbose (dsp->format, constant, datatime);
      strncpy (verbose_meta->tstring, filename, 20)[19] = 0;
    }

  *data_size = 0;
  buf = (unsigned char *)ossplay_malloc (RECBUF_SIZE);
   /*LINTED*/ while (1)
    {
      if ((l = read (dsp->fd, buf, RECBUF_SIZE)) < 0)
	{
          if ((errno == EINTR) && (eflag)) EXITREC (eflag);
	  if (errno == ECONNRESET) EXITREC (E_ENCODE); /* Device disconnected */
          perror_msg (dsp->dname);
          EXITREC (E_ENCODE);
	}
      if (l == 0)
	{
	  print_msg (ERRORM, "Unexpected EOF on audio device\n");
          EXITREC (eflag);
	}

      obuf = buf; d = dec; outl = l;
      do
        {
          outl = d->decoder (&(d->outbuf), obuf, outl, d->metadata);
          obuf = d->outbuf;
          d = d->next;
        }
      while (d != NULL);

      if (eflag) EXITREC (eflag);

      if (fwrite (obuf, outl, 1, wave_fp) != 1)
        {
          if ((errno == EINTR) && (eflag)) EXITREC (eflag);
          perror_msg (filename);
          EXITREC (E_ENCODE);
        }

      *data_size += outl;
      if (verbose) print_record_verbose_info (obuf, outl, verbose_meta);

      if ((datalimit != 0) && (*data_size >= data_size_limit)) break;
    }

  ossplay_free (buf);
  ossplay_free (verbose_meta);
  clear_update ();
  print_msg (VERBOSEM, "\nDone.\n");
  return 0;
}

errors_t
silence (dspdev_t * dsp, big_t len, int speed)
{
  errors_t ret;
  ssize_t i;
  unsigned char empty[1024];

  ret = setup_device (dsp, AFMT_U8, 1, speed);

  if (ret == E_FORMAT_UNSUPPORTED)
    {
      len *= 4;
      if ((ret = setup_device (dsp, AFMT_S16_NE, 2, speed))) return ret;
    }
  else if (ret) return ret;

  memset (empty, 0, 1024 * sizeof (unsigned char));

  while (len > 0)
    {
      i = 1024;
      if ((big_t)i > len) i = len;
      if ((i = write (dsp->fd, empty, i)) < 0) return -1;

      len -= i;
    }

  return E_OK;
}
