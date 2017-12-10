/*
 * recplay.c
 * 
 * A simple recording and playback program for OSS 4.0
 * or later.
 * 
 * Copyright by Hannu Savolainen 1993-2004
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
/* #include <getopt.h> */
#include <fcntl.h>
#include <string.h>
#include <soundcard.h>

#define DEFAULT_DSP_SPEED 8000

#define RECORD	0
#define PLAY	1
#define AUDIO "/dev/dsp"

int prof = APF_NORMAL;
int timelimit = 0, dsp_speed = DEFAULT_DSP_SPEED, dsp_channels = 1;
int samplefmt = AFMT_S16_LE;
int quiet_mode = 0;
int audio, abuf_size;
int direction, omode;
int blksize = 0;
int fragsize = 0;
char *audiobuf, c;

char audio_name[64] = AUDIO;

void recplay (char *name);
extern void describe_error (void);

typedef struct
{
  char *name;
  int fmt;
  int bits;
} sample_fmt_t;

static const sample_fmt_t formats[] = {
  {"AFMT_MU_LAW", AFMT_MU_LAW},
  {"AFMT_A_LAW", AFMT_A_LAW},
  {"AFMT_IMA_ADPCM", AFMT_IMA_ADPCM},
  {"AFMT_U8", AFMT_U8, 8},
  {"AFMT_S16_LE", AFMT_S16_LE, 16},
  {"AFMT_S16_BE", AFMT_S16_BE, 16},
  {"AFMT_S8", AFMT_S8, 8},
  {"AFMT_U16_LE", AFMT_U16_LE, 16},
  {"AFMT_U16_BE", AFMT_U16_BE, 16},
  {"AFMT_MPEG", AFMT_MPEG},
  {"AFMT_AC3", AFMT_AC3},
  {"AFMT_VORBIS", AFMT_VORBIS},
  {"AFMT_S32_LE", AFMT_S32_LE, 32},
  {"AFMT_S32_BE", AFMT_S32_BE, 32},
  {"AFMT_FLOAT", AFMT_FLOAT},
  {"AFMT_S24_LE", AFMT_S24_LE, 24},
  {"AFMT_S24_BE", AFMT_S24_BE, 24},
  {"AFMT_SPDIF_RAW", AFMT_SPDIF_RAW},
  {"AFMT_S24_PACKED", AFMT_S24_PACKED, 24},
  {NULL, 0}
};

static int
set_samplefmt (char *fmt)
{
  int i;

  for (i = 0; formats[i].name != NULL; i++)
    if (strcmp (formats[i].name, fmt) == 0)
      {
	return formats[i].fmt;
      }

  fprintf (stderr, "Error: Unrecognized sample format '%s'\n", fmt);
  exit (-1);
}

int
main (int argc, char *argv[])
{

  char *command;
  int tmp;
  int i;

  command = argv[0];
  if (strstr (argv[0], "srec"))
    {
      direction = RECORD;
      omode = O_RDONLY;
    }
  else if (strstr (argv[0], "splay"))
    {
      direction = PLAY;
      omode = O_WRONLY;
    }
  else
    {
      fprintf (stderr,
	       "Error: command should be named either srec or splay\n");
      exit (1);
    }

  while ((c = getopt (argc, argv, "pqs:St:b:d:B:f:F:c:")) != EOF)
    switch (c)
      {
      case 'S':		/* Stereo is obsolete - use -c instead */
	dsp_channels = 2;
	break;
      case 'c':
	dsp_channels = atoi (optarg);
	break;
      case 'q':
	quiet_mode = 1;
	break;
      case 's':
	dsp_speed = atoi (optarg);
	if (dsp_speed < 300)
	  dsp_speed *= 1000;
	break;
      case 't':
	timelimit = atoi (optarg);
	break;

      case 'b':		/* Bits (obsolete) supports only 8 and 16 */
	samplefmt = atoi (optarg);
	break;

      case 'F':
	samplefmt = set_samplefmt (optarg);
	break;

      case 'B':
	blksize = atoi (optarg);
	break;

      case 'd':
	strncpy (audio_name, optarg, sizeof (audio_name) - 1);
	break;

      case 'f':
	fragsize = atoi (optarg);
	if (fragsize == 0)
	  {
	    fprintf (stderr, "Bad fragment size (-f %s)\n", optarg);
	    exit (-1);
	  }
	fragsize |= 0x7fff0000;
	break;

      case 'p':
	prof = APF_CPUINTENS;
	break;

      default:
	fprintf (stderr,
		 "Usage: %s [-q] [-c channels] [-t secs] [-s Hz] [-b 8|12|16] [-d device] [filename]\n",
		 command);
	exit (-1);
      }


  audio = open (audio_name, omode, 0);
  if (audio == -1)
    {
      perror (audio_name);
      describe_error ();
      exit (-1);
    }

  if (fragsize != 0)
    if (ioctl (audio, SNDCTL_DSP_SETFRAGMENT, &fragsize) == -1)
      {
	perror ("SETFRAGMENT");
	exit (-1);
      }

  tmp = samplefmt;
  ioctl (audio, SNDCTL_DSP_SETFMT, &tmp);
  if (tmp != samplefmt)
    {
      fprintf (stderr, "Unable to set the requested sample format\n");

      for (i = 0; formats[i].fmt != 0; i++)
	if (formats[i].fmt == samplefmt)
	  {
	    fprintf (stderr, "The required format is %s (%08x)\n",
		     formats[i].name, formats[i].fmt);
	    break;
	  }

      for (i = 0; formats[i].fmt != 0; i++)
	if (formats[i].fmt == tmp)
	  {
	    fprintf (stderr, "The device supports %s (%08x)\n",
		     formats[i].name, formats[i].fmt);
	    break;
	  }
      exit (-1);
    }

  ioctl (audio, SNDCTL_DSP_PROFILE, &prof);

  if (ioctl (audio, SNDCTL_DSP_CHANNELS, &dsp_channels) == -1)
    {
      fprintf (stderr, "%s: Unable to set the number of channels\n", command);
      perror (audio_name);
      exit (-1);
    }

  if (ioctl (audio, SNDCTL_DSP_SPEED, &dsp_speed) == -1)
    {
      fprintf (stderr, "%s: Unable to set audio speed\n", command);
      perror (audio_name);
      exit (-1);
    }
  if (!quiet_mode)
    {
      fprintf (stderr, "Speed %d Hz ", dsp_speed);
      fprintf (stderr, "%d channels  ", dsp_channels);

      for (i = 0; formats[i].fmt != 0; i++)
	if (formats[i].fmt == samplefmt)
	  {
	    fprintf (stderr, "Sample format is %s", formats[i].name);
	    if (formats[i].bits != 0)
	      fprintf (stderr, "(%d bits)", formats[i].bits);
	    break;
	  }
      fprintf (stderr, "\n");
    }

  if (blksize > 0)
    abuf_size = blksize;
  else
    {
#if 0
/*
 * There is no idea in using SNDCTL_DSP_GETBLKSIZE in applications like this.
 * Using some fixed local buffer size will work equally well.
 */
      ioctl (audio, SNDCTL_DSP_GETBLKSIZE, &abuf_size);
      if (abuf_size < 1)
	{
	  perror ("GETBLKSIZE");
	  exit (-1);
	}
#else
      abuf_size = 1024;
#endif
    }

  if ((audiobuf = malloc (abuf_size)) == NULL)
    {
      fprintf (stderr, "Unable to allocate input/output buffer\n");
      exit (-1);
    }

  if (optind > argc - 1)
    recplay (NULL);
  else
    while (optind <= argc - 1)
      {
	recplay (argv[optind++]);
      }

  close (audio);
  return 0;
}

void
recplay (char *name)
{
  int fd, l;

  int count, c;

  if (!timelimit)
    count = 0x7fffffff;
  else
    {
      count = timelimit * dsp_speed * dsp_channels;
      if (samplefmt != AFMT_U8)
	count *= 2;		/* TODO: This is bogus because just few formats are 16 bits */
    }

  if (direction == PLAY)
    {
      if (!name)
	{
	  fd = 0;
	  name = "stdin";
	}
      else
	{
	  if ((fd = open (name, O_RDONLY, 0)) == -1)
	    {
	      perror (name);
	      exit (-1);
	    }
	}

      while (count)
	{
	  c = count;

	  if (c > abuf_size)
	    c = abuf_size;

	  if ((l = read (fd, audiobuf, c)) > 0)
	    {
	      if (write (audio, audiobuf, l) != l)
		{
		  perror (audio_name);
		  exit (-1);
		}
	      count -= l;
	    }
	  else
	    {
	      if (l == -1)
		{
		  perror (name);
		  exit (-1);
		}
	      count = 0;	/* Stop */
	    }

	}			/* while (count) */
      if (fd != 0)
	close (fd);
    }
  else
    {
      if (!name)
	{
	  fd = 1;
	  name = "stdout";
	}
      else
	{
	  if ((fd = open (name, O_WRONLY | O_CREAT, 0666)) == -1)
	    {
	      perror (name);
	      exit (-1);
	    }
	}

      while (count)
	{
	  c = count;
	  if (c > abuf_size)
	    c = abuf_size;

	  if ((l = read (audio, audiobuf, c)) > 0)
	    {
	      if (write (fd, audiobuf, l) != l)
		{
		  perror (name);
		  exit (-1);
		}
	      count -= l;
	    }

	  if (l == -1)
	    {
	      perror (audio_name);
	      exit (-1);
	    }
	}			/* While count */

      if (fd != 1)
	close (fd);
    }
}
