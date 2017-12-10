/*
 * Purpose: Plays a funny synthetic engine stop sound
 * Copyright (C) 4Front Technologies, 2002-2004. Released under GPLv2/CDDL.
 *
 * Description:
 * This program was supposed to do a frequency sweep down from sample_rate/2
 * to 0 Hz. However due to some arithmetic problem the result was much more
 * interesting.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <soundcard.h>

char *dspname = "/dev/dsp";

int fd;

#define SIN_STEPS	48
static short sinebuf[48] = {
  0, 4276, 8480, 12539, 16383, 19947, 23169, 25995,
  28377, 30272, 31650, 32486, 32767, 32486, 31650, 30272,
  28377, 25995, 23169, 19947, 16383, 12539, 8480, 4276,
  0, -4276, -8480, -12539, -16383, -19947, -23169, -25995,
  -28377, -30272, -31650, -32486, -32767, -32486, -31650, -30272,
  -28377, -25995, -23169, -19947, -16383, -12539, -8480, -4276
};

#define F_SCALE (1000000)

int phase = 0, freq = F_SCALE / 2;

static void
sweeper (int speed, int channels, int fragsize)
{
  short buf[4096], *p = buf;
  int c, i, n, v, x;

  fragsize /= 2 * channels;
  speed /= 100;

  for (i = 0; i < fragsize; i++)
    {
      x = (phase + F_SCALE / 2) / F_SCALE;
      if (x < 0) x = 0;
      v = sinebuf[x % SIN_STEPS] / 4;

      phase = phase + freq * 480 / speed;

      if (freq > 1000)
	freq -= 3 * 480 / speed;
      else
	freq -= 1;

      for (c = 0; c < channels; c++)
	*p++ = v;

      if (freq <= 0)
	break;
    }

  write (fd, buf, i * 2 * channels);
}

int
main (int argc, char *argv[])
{
  int bits, channels, speed, tmp;
  int i, l;

  if (argc == 2)
    dspname = argv[1];


  if ((fd = open (dspname, O_WRONLY, 0)) == -1)
    {
      perror (dspname);
      exit (-1);
    }

  speed = 96000;
  channels = 2;
  bits = AFMT_S16_NE;

  if (ioctl (fd, SNDCTL_DSP_CHANNELS, &channels) == -1)
    {
      perror ("SNDCTL_DSP_CHANNELS");
      exit (-1);
    }

  if (ioctl (fd, SNDCTL_DSP_SETFMT, &bits) == -1)
    {
      perror ("SNDCTL_DSP_SETFMT");
      exit (-1);
    }

  if (bits != AFMT_S16_NE)
    {
      fprintf (stderr,
	       "Device %s doesn't support 16 bit (native endian) format\n",
	       dspname);
      exit (-1);
    }

  if (ioctl (fd, SNDCTL_DSP_SPEED, &speed) == -1)
    {
      perror ("SNDCTL_DSP_SPEED");
      exit (-1);
    }

  if (ioctl (fd, SNDCTL_DSP_GETBLKSIZE, &tmp) == -1)
    {
      perror ("SNDCTL_DSP_GETBLKSIZE");
      exit (-1);
    }

  printf ("Outputting sweep at %d Hz, %d channels, 16 bits\n", speed,
	  channels);
  printf ("Fragment size is %d\n", tmp);

  while (freq > 0)
    sweeper (speed, channels, tmp);

  exit (0);
}
