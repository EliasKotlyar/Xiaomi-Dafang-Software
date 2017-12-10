/*
 * Purpose: A simple demonstration of 
 * Copyright (C) 4Front Technologies, 2007. All rights reserved.
 *
 * Description:
 * This program is seriously broken. It's only purpose is to fail so that
 * the !nlink SNDCTL_DSP_GETERROR} ioctl call can be tested. Otherwise this 
 * program is based on the {!nlink singen.c} program.
 *
 * However this program demonstrates how SNDCTL_DSP_GETERROR can be used in 
 * applications.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <soundcard.h>

int fd_out;

static int
open_audio_device (char *name, int mode)
{
  int tmp, fd;
  int sample_rate;

  if ((fd = open (name, mode, 0)) == -1)
    {
      perror (name);
      exit (-1);
    }

/*
 * Setup the device. Note that it's important to set the
 * sample format, number of channels and sample rate exactly in this order.
 * Some devices depend on the order.
 */

/*
 * Set the sample format
 */
  tmp = AFMT_S16_NE;		/* Native 16 bits */
  if (ioctl (fd, SNDCTL_DSP_SETFMT, &tmp) == -1)
    {
      perror ("SNDCTL_DSP_SETFMT");
      exit (-1);
    }

  if (tmp != AFMT_S16_NE)
    {
      fprintf (stderr,
	       "The device doesn't support the 16 bit sample format.\n");
      exit (-1);
    }

/*
 * Set the number of channels
 */
  tmp = 1;
  if (ioctl (fd, SNDCTL_DSP_CHANNELS, &tmp) == -1)
    {
      perror ("SNDCTL_DSP_CHANNELS");
      exit (-1);
    }

  if (tmp != 1)
    {
      fprintf (stderr, "The device doesn't support mono mode.\n");
      exit (-1);
    }

/*
 * Set the sample rate
 */
  sample_rate = 48000;
  if (ioctl (fd, SNDCTL_DSP_SPEED, &sample_rate) == -1)
    {
      perror ("SNDCTL_DSP_SPEED");
      exit (-1);
    }

/*
 * No need for error checking because we will automatically adjust the
 * signal based on the actual sample rate. However most application must
 * check the value of sample_rate and compare it to the requested rate.
 *
 * Small differences between the rates (10% or less) are normal and the
 * applications should usually tolerate them. However larger differences may
 * cause annoying pitch problems (Mickey Mouse).
 */

  return fd;
}

int
main (int argc, char *argv[])
{
  char *name_out = "/dev/dsp";
  int tmp;

  audio_errinfo ei;
  audio_buf_info bi;

  if (argc > 1)
    name_out = argv[1];

  fd_out = open_audio_device (name_out, O_WRONLY);

/*
 * Cause an intentional error by using wrong parameter to SNDCTL_DSP_STEREO
 * which expects 0 or 1. Don't do error checking in this case because
 * we would like to see two errors reported.
 */
  tmp = 2;
  ioctl (fd_out, SNDCTL_DSP_STEREO, &tmp);

/*
 * Cause an intentional failure by calling {!nlink SNDCTL_DSP_GETISPACE}
 * which is not permitted on write-only devices.
 */

  if (ioctl (fd_out, SNDCTL_DSP_GETISPACE, &bi) == -1)
    {
      perror ("SNDCTL_DSP_GETISPACE");	/* Report the "primary" error first */

      /*
       * Next show the explanation to the user.
       */
      fprintf (stderr,
	       "Audio error: Cannot obtain recorded byte count from the device.\n");
      fprintf (stderr, "\n");

      /*
       * Next call {!nlink SNDCTL_DSP_GETERROR} to see if there is
       * any additional info available.
       */

      if (ioctl (fd_out, SNDCTL_DSP_GETERROR, &ei) != -1)
	{
	  if (ei.play_errorcount > 0 && ei.play_lasterror != 0)
	    fprintf (stderr, "%d OSS play event(s), last=%05d:%d\n",
		     ei.play_errorcount, ei.play_lasterror,
		     ei.play_errorparm);

	  if (ei.rec_errorcount > 0 && ei.rec_lasterror != 0)
	    fprintf (stderr, "%d OSS rec event(s), last=%05d:%d\n",
		     ei.rec_errorcount, ei.rec_lasterror, ei.rec_errorparm);
	}
    }
  else
    fprintf (stderr, "SNDCTL_DSP_GETISPACE didn't fail as expected.\n");

  exit (0);
}
