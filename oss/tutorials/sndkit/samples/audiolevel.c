/*
 * Purpose: A simple program that does audio recording.
 * Copyright (C) 4Front Technologies, 2002-2004. Released under GPLv2/CDDL.
 *
 * Description:
 * This is a very minimal program that does audio recording. However to
 * demonstrate processiong of audio data it computes the
 * maximum value of the signal and displays a "LED" bars
 * (using character mode console).
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <soundcard.h>

int fd_in;
int sample_rate = 48000;

/*
 * The open_audio_device opens the audio device and initializes it 
 * for the required mode. The same routine is used in the other
 * simple sample programs too.
 */

static int
open_audio_device (char *name, int mode)
{
  int tmp, fd;

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
 * Set the number of channels (mono)
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

void
process_input (void)
{
  short buffer[1024];

  int l, i, level;

/*
 * First read a block of audio samples with proper error checking.
 */

  if ((l = read (fd_in, buffer, sizeof (buffer))) == -1)
    {
      perror ("Audio read");
      exit (-1);		/* Or return an error code */
    }

/*
 * We are using 16 bit samples so the number of bytes returned by read must be
 * converted to the number of samples (2 bytes per a 16 bit sample).
 *
 * Some care must be taken if this program is converted from 1 channels
 * (mono) to 2 channels (stereo) or more.
 *
 * Handling more than 1 channels is bit more complicated because the channels
 * are interleaved. This will be demonstrated in some other programs.
 */

  l = l / 2;

/*
 * After this point this routine will perform the peak volume computations.
 * The {!code l} variable contains the number of samples in the buffer.
 *
 * The remaining lines can be removed and replaced with the required 
 * application code.
 */

  level = 0;

  for (i = 0; i < l; i++)
    {
/*
 * Take the next sample (i) and compute it's absolute value. Check if it
 * was larger than the previous peak value.
 */
      int v = buffer[i];

      if (v < 0)
	v = -v;			/* abs */

      if (v > level)
	level = v;
    }

/*
 * Finally print the simple LED bar. The maximum value for a 16 bit
 * sample is 32*1024-1. Convert this to 32 bars.
 *
 * This program uses linear scale for simplicity. Real world audio programs
 * should probably use logarithmic scale (dB).
 */

  level = (level + 1) / 1024;

  for (i = 0; i < level; i++)
    printf ("*");
  for (i = level; i < 32; i++)
    printf (".");
  printf ("\r");
  fflush (stdout);
}

int
main (int argc, char *argv[])
{
/*
 * Use /dev/dsp as the default device because the system administrator
 * may select the device using the {!xlink ossctl} program or some other
 * methods
 */
  char *name_in = "/dev/dsp";

/*
 * It's recommended to provide some method for selecting some other
 * device than the default. We use command line argument but in some cases
 * an environment variable or some configuration file setting may be better.
 */
  if (argc > 1)
    name_in = argv[1];

/*
 * It's mandatory to use O_RDONLY in programs that do only recording. Other
 * modes may cause increased resource (memory) usage in the driver. It may
 * also prevent other applications from using the same device for 
 * playback at the same time.
 */
  fd_in = open_audio_device (name_in, O_RDONLY);

  while (1)
    process_input ();

  exit (0);
}
