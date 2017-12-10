/*
 * Purpose: A sample program for using mmap()
 * Copyright (C) 4Front Technologies, 2002-2004. Released under GPLv2/CDDL.
 *
 * Description:
 * This is a simple program which demonstrates use of mmapped DMA buffer
 * of the sound driver directly from application program.
 *
 * This program tries to open a file called "smpl" in the current directory and
 * play it. If present this file must be a "raw" audio file recorded with
 * the same sample rate and format as this program uses. There is no checking 
 * for the format in this program.
 *
 * {!notice This program needs some fine tuning. At this moment it doesn't
 * perform adequate error checkings.}
 *
 */

#define VERBOSE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <soundcard.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#ifndef MAP_FILE
#define MAP_FILE 0
#endif

static int sinebuf[48] = {
  0, 4276, 8480, 12539, 16383, 19947, 23169, 25995,
  28377, 30272, 31650, 32486, 32767, 32486, 31650, 30272,
  28377, 25995, 23169, 19947, 16383, 12539, 8480, 4276,
  0, -4276, -8480, -12539, -16383, -19947, -23169, -25995,
  -28377, -30272, -31650, -32486, -32767, -32486, -31650, -30272,
  -28377, -25995, -23169, -19947, -16383, -12539, -8480, -4276
};
static int sinep = 0;

static void
produce_output (short *op, int offs, int len)
{
  int i;

  op += offs * 2;

  for (i = 0; i < len; i++)
    {
      int v = sinebuf[sinep];
      sinep = (sinep + 1) % 48;

      *op++ = v;		/* Left channel */
      *op++ = v;		/* Right channel */
    }
}

int
main (int argc, char *argv[])
{
  int fd, tmp;
  int sz, fsz, num_samples;
  int caps;
  struct audio_buf_info info;
  count_info ci;
  caddr_t buf;
  oss_audioinfo ai;
  unsigned char *op;
  int device_p, app_p;

/*
 * This program must use O_RDWR in some operating systems like Linux.
 * However in some other operating systems it may need to be O_WRONLY.
 *
 * {!code /dev/dsp_mmap} is the default device for mmap applications. 
 */

  if ((fd = open ("/dev/dsp_mmap", O_RDWR, 0)) == -1)
    {
      perror ("/dev/dsp_mmap");
      exit (-1);
    }

  ai.dev = -1;
  if (ioctl (fd, SNDCTL_ENGINEINFO, &ai) != -1)
    {
      printf ("Using audio device %s (engine %d)\n", ai.name, ai.dev);
    }
/*
 * Disable cooked mode to permit mmap() with some devices.
 * Don't do any error checking since usually this call will fail.
 * There is no need to care about the return value.
 *
 * Cooked mode must be disabled before setting the sample rate and format.
 */
  tmp = 0;
  ioctl (fd, SNDCTL_DSP_COOKEDMODE, &tmp);	/* Don't check the error return */

/*
 * Set up the sample format. We will use AFMT_S16_LE because it's the most
 * common audio file format. AFMT_S16_NE is better in programs that
 * generate the audio signal themselves.
 */

  tmp = AFMT_S16_LE;
  if (ioctl (fd, SNDCTL_DSP_SETFMT, &tmp) == -1)
    {
      perror ("SNDCTL_DSP_SETFMT");
      exit (-1);
    }

/*
 * Check the format returned by the driver.
 *
 * This program will simply refuse to work if it doesn't get the format it
 * supports. Playing with incompatible formats will cause terrible noise so 
 * it must be avoided.
 */
  if (tmp != AFMT_S16_LE)
    {
      fprintf (stderr,
	       "Error: The device doesn't support the requested sample format\n");
      exit (-1);
    }

/*
 * Set the number of channels and the sample rate. We do not care about the 
 * returned values. They will just be reported to the user.
 *
 * {!notice Real applications must be prepared to support sampling rates
 * between 8 kHz and 192 kHz (at least). Equally well the number of channels
 * may be between 1 and 16 (or even more).}
 *
 * Two channels and 48 kHz is the most likely combination that works.
 */
  tmp = 2;			/* Stereo */
  if (ioctl (fd, SNDCTL_DSP_CHANNELS, &tmp) == -1)
    {
      perror ("SNDCTL_DSP_CHANNELS");
      exit (-1);
    }

  printf ("Number of channels is %d\n", tmp);

  tmp = 44100;			/* 48000 is the most recommended rate */
  if (ioctl (fd, SNDCTL_DSP_SPEED, &tmp) == -1)
    {
      perror ("SNDCTL_DSP_SPEED");
      exit (-1);
    }

  printf ("Sample rate set to %d\n", tmp);

  if (ioctl (fd, SNDCTL_DSP_GETCAPS, &caps) == -1)
    {
      perror ("/dev/dsp");
      fprintf (stderr, "Sorry but your sound driver is too old\n");
      exit (-1);
    }

  if (!(caps & PCM_CAP_TRIGGER))
    {
      fprintf (stderr, "Sorry but your soundcard can't do this (TRIGGER)\n");
      exit (-1);
    }

  if (!(caps & PCM_CAP_MMAP))
    {
      fprintf (stderr, "Sorry but your soundcard can't do this (MMAP)\n");
      exit (-1);
    }

/*
 * Compute total size of the buffer. It's important to use this value
 * in mmap() call.
 */

  if (ioctl (fd, SNDCTL_DSP_GETOSPACE, &info) == -1)
    {
      perror ("GETOSPACE");
      exit (-1);
    }

  sz = info.fragstotal * info.fragsize;
  fsz = info.fragsize;

/*
 * Call mmap().
 * 
 * IMPORTANT NOTE!!!!!!!!!!!
 *
 * Full duplex audio devices have separate input and output buffers. 
 * It is not possible to map both of them at the same mmap() call. The buffer
 * is selected based on the prot argument in the following way:
 *
 * - PROT_READ (alone) selects the input buffer.
 * - PROT_WRITE (alone) selects the output buffer.
 * - PROT_WRITE|PROT_READ together select the output buffer. This combination
 *   is required in BSD to make the buffer accessible. With just PROT_WRITE
 *   every attempt to access the returned buffer will result in segmentation/bus
 *   error. PROT_READ|PROT_WRITE is also permitted in Linux with OSS version
 *   3.8-beta16 and later (earlier versions don't accept it).
 *
 * Non duplex devices have just one buffer. When an application wants to do both
 * input and output it's recommended that the device is closed and re-opened when
 * switching between modes. PROT_READ|PROT_WRITE can be used to open the buffer
 * for both input and output (with OSS 3.8-beta16 and later) but the result may be
 * unpredictable.
 */

  if ((buf =
       mmap (NULL, sz, PROT_WRITE, MAP_FILE | MAP_SHARED, fd,
	     0)) == (caddr_t) - 1)
    {
      perror ("mmap (write)");
      exit (-1);
    }
  printf ("mmap (out) returned %08lx\n", (long) buf);
  op = buf;

/*
 * op contains now a pointer to the DMA buffer. Preload some audio data.
 */

  num_samples = sz / 4;
  produce_output ((short *) op, 0, num_samples);
  app_p = 0;

/*
 * Then it's time to start the engine. The driver doesn't allow read() and/or
 * write() when the buffer is mapped. So the only way to start operation is
 * to togle device's enable bits. First set them off. Setting them on enables
 * recording and/or playback.
 */

  tmp = 0;
  ioctl (fd, SNDCTL_DSP_SETTRIGGER, &tmp);
  printf ("Trigger set to %08x\n", tmp);

/*
 * It might be usefull to write some data to the buffer before starting.
 */

  tmp = PCM_ENABLE_OUTPUT;
  ioctl (fd, SNDCTL_DSP_SETTRIGGER, &tmp);
  printf ("Trigger set to %08x\n", tmp);

/*
 * The machine is up and running now. Use SNDCTL_DSP_GETOPTR to get the
 * buffer status.
 *
 * NOTE! The driver empties each buffer fragmen after they have been
 * played. This prevents looping sound if there are some performance problems
 * in the application side. For similar reasons it recommended that the
 * application uses some amout of play ahead. It can rewrite the unplayed
 * data later if necessary.
 */

  while (1)
    {
      usleep (50 * 1000);

      if (ioctl (fd, SNDCTL_DSP_GETOPTR, &ci) == -1)
	{
	  perror ("SNDCTL_DSP_GETOPTR");
	  exit (-1);
	}

      device_p = ci.ptr / 4;

      if (device_p < app_p)
	{
	  produce_output ((short *) op, app_p, num_samples - app_p);
	  app_p = 0;
	}

      if (device_p > app_p)
	{
	  produce_output ((short *) op, app_p, device_p - app_p);
	  app_p = device_p;
	}
    }

  exit (0);
}
