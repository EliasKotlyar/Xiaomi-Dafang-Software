/*
 * Purpose: A simple program that plays frequency sweep from 10 Hz to fs/2.
 * Copyright (C) 4Front Technologies, 2002-2004. Released under GPLv2/CDDL.
 *
 * Description:
 * This program plays a frequency seep up starting from 10 Hz and
 * goes up to sample_rate/2.
 */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <soundcard.h>

char *dspname = "/dev/dsp";

int fd;

int
main (int argc, char *argv[])
{
  int bits, channels, speed, tmp;
  int i, l, n;
#define BUFFSIZE	(64*1024)
  short buf[BUFFSIZE];
  int p, t;
  double freq = 10.0 /* Hz */ , phase = 0.0;
  int delay = 0;

  if (argc > 1)
    dspname = argv[1];

  if (argc > 2)
    delay = atoi (argv[2]);

  if ((fd = open (dspname, O_WRONLY, 0)) == -1)
    {
      perror (dspname);
      exit (-1);
    }

  speed = 48000;
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

  fprintf (stderr, "Outputting sweep at %d Hz, %d channels, 16 bits\n", speed,
	   channels);

  n = 0;
  p = 0;

  while (1)			/* Infinite loop */
    {
      int v;
      double step;

      v = sin (phase) * 16483.0;

      for (i = 0; i < channels; i++)
	{
	  buf[p++] = v;

	  if (p >= BUFFSIZE)
	    {
//            write (1, buf, p * 2);

	      if (write (fd, buf, p * 2) != p * 2)
		{
		  perror ("write");
		  exit (-1);
		}
	      if (delay > 0)
		usleep (delay * 1000);
	      fprintf (stderr, "\r%d ", (int) (freq));
	      fflush (stderr);
	      p = 0;
	    }
	}

      step = 2.0 * M_PI * freq / (double) speed;

      phase += step;

      if (freq < (double) speed / 2.1)
	freq *= 1.000002;
      else
	break;
    }

  fprintf (stderr, "\n");
  if (p > 0)
    write (fd, buf, p * 2);

  exit (0);
}
