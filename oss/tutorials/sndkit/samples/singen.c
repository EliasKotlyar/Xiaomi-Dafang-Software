/*
 * Purpose: A simple audio playback program that plays continuous 1 kHz sine wave.
 * Copyright (C) 4Front Technologies, 2002-2004. Released under GPLv2/CDDL.
 *
 * Description:
 * This minimalistic program shows how to play udio with OSS. It outputs
 * 1000 Hz sinewave signal (based on a 48 step lookup table).
 *
 * This is pretty much the simpliest possible audio playback program
 * one can imagine. It could be possible to make it even simplier
 * by removing all error checking but that is in no way recommended.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <soundcard.h>

int fd_out;
int sample_rate = 48000;

static void
write_sinewave (void)
{
/*
 * This routine is a typical example of application routine that
 * produces audio signal using synthesis. This is actually a very
 * basic "wave table" algorithm (btw). It uses precomputed sine
 * function values for a complete cycle of a sine function.
 * This is much faster than calling the sin() function once for
 * each sample.
 *
 * In other applications this routine can simply be replaced by
 * whatever the application needs to do.
 */

  static unsigned int phase = 0;	/* Phase of the sine wave */
  unsigned int p;
  int i;
  short buf[1024];		/* 1024 samples/write is a safe choice */

  int outsz = sizeof (buf) / 2;

  static int sinebuf[48] = {

    0, 4276, 8480, 12539, 16383, 19947, 23169, 25995,
    28377, 30272, 31650, 32486, 32767, 32486, 31650, 30272,
    28377, 25995, 23169, 19947, 16383, 12539, 8480, 4276,
    0, -4276, -8480, -12539, -16383, -19947, -23169, -25995,
    -28377, -30272, -31650, -32486, -32767, -32486, -31650, -30272,
    -28377, -25995, -23169, -19947, -16383, -12539, -8480, -4276
  };

  for (i = 0; i < outsz; i++)
    {
/*
 * The sinebuf[] table was computed for 48000 Hz. We will use simple
 * sample rate compensation.
 *
 * {!notice We must prevent the phase variable from groving too large
 * because that would cause cause arihmetic overflows after certain time.
 * This kind of error posibilities must be identified when writing audio 
 * programs that could be running for hours or even months or years without
 * interruption. When computing (say) 192000 samples each second the 32 bit
 * integer range may get overflown very quickly. The number of samples
 * played at 192 kHz will cause an overflow after about 6 hours.}
 */

      p = (phase * sample_rate) / 48000;

      phase = (phase + 1) % 4800;
      buf[i] = sinebuf[p % 48];
    }

/*
 * Proper error checking must be done when using write. It's also
 * important to reporte the error code returned by the system.
 */

  if (write (fd_out, buf, sizeof (buf)) != sizeof (buf))
    {
      perror ("Audio write");
      exit (-1);
    }
}

/*
 * The open_audio_device opens the audio device and initializes it 
 * for the required mode.
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
/*
 * Use /dev/dsp as the default device because the system administrator
 * may select the device using the {!xlink ossctl} program or some other
 * methods
 */
  char *name_out = "/dev/dsp";

/*
 * It's recommended to provide some method for selecting some other
 * device than the default. We use command line argument but in some cases
 * an environment variable or some configuration file setting may be better.
 */
  if (argc > 1)
    name_out = argv[1];

/*
 * It's mandatory to use O_WRONLY in programs that do only playback. Other
 * modes may cause increased resource (memory) usage in the driver. It may
 * also prevent other applications from using the same device for 
 * recording at the same time.
 */
  fd_out = open_audio_device (name_out, O_WRONLY);

  while (1)
    write_sinewave ();

  exit (0);
}
