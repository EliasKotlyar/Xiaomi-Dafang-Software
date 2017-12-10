/*
 * Multi channel audio test.
 *
 * This program is intended to test playback of 16 bit samples using 4 or more
 * channels at 48000 Hz. The program plays sine wave pulses sequentially on
 * channels 0 to N-1.
 *
 * Arguments:
 *
 * 1:	Number of channelts (default is 8).
 * 2:	Audio device (/dev/dsp by default).
 * 3-N: Options
 * 	  -b	Bypass virtual mixer
 * 	  -r	Raw mode (disables automatic sample rate/format conversions)
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <soundcard.h>

static int sinedata[48] = {

  0, 4276, 8480, 12539, 16383, 19947, 23169, 25995,
  28377, 30272, 31650, 32486, 32767, 32486, 31650, 30272,
  28377, 25995, 23169, 19947, 16383, 12539, 8480, 4276,
  0, -4276, -8480, -12539, -16383, -19947, -23169, -25995,
  -28377, -30272, -31650, -32486, -32767, -32486, -31650, -30272,
  -28377, -25995, -23169, -19947, -16383, -12539, -8480, -4276
};

int
main (int argc, char *argv[])
{
  char *dev = "/dev/dsp";
  int fd, l, i, n = 0, ch, p = 0, phase = 0, arg, channels, srate, thisch = 0;
  int tick = 0;
  int nch = 8;
  int bypass_vmix=0;
  int disable_format_conversions=0;

  int buf[32*1024];

  if (argc > 1)
    if (sscanf (argv[1], "%d", &nch) != 1)
      nch = 2;

  if (argc > 2)
    dev = argv[2];

  for (i=3;i<argc;i++)
  if (argv[i][0]=='-')
  switch (argv[i][1])
  {
  case 'b': /* Bypass virtual mixer */
	  bypass_vmix = O_EXCL;
	  break;

  case 'r': /* Use raw mode (disable automatic format conversions) */
	  disable_format_conversions=1;
	  break;
  }

  if ((fd = open (dev, O_WRONLY|bypass_vmix, 0)) == -1)
    {
      perror (dev);
      exit (-1);
    }

  if (disable_format_conversions)
     {
  	arg=0;
  	ioctl(fd, SNDCTL_DSP_COOKEDMODE, &arg);
     }

  arg = nch;
  if (ioctl (fd, SNDCTL_DSP_CHANNELS, &arg) == -1)
    perror ("SNDCTL_DSP_CHANNELS");
  channels = arg;
  fprintf (stderr, "Channels %d\n", arg);

  arg = AFMT_S32_NE;
  if (ioctl (fd, SNDCTL_DSP_SETFMT, &arg) == -1)
    perror ("SNDCTL_DSP_SETFMT");
  fprintf (stderr, "Format %x\n", arg);

  arg = 48000;
  if (ioctl (fd, SNDCTL_DSP_SPEED, &arg) == -1)
    perror ("SNDCTL_DSP_SPEED");
  printf ("Using sampling rate %d\n", arg);
  srate = arg;

  while (1)
    {
      for (ch = 0; ch < channels; ch++)
	{
	  if (ch == thisch)
	    {
	      buf[p] = sinedata[phase]<<16;
	      phase = (phase + 1 + (ch / 2)) % 48;
	      if (phase == 0 && tick > 10 * channels)
		{
		  thisch = (thisch + 1) % channels;
		  tick = 0;
		}
	    }
	  else
	    buf[p] = 0;

	  p++;

	  if (p >= sizeof (buf) / 4)
	    {
	      if (write (fd, buf, p * 4) != p * 4)
		perror ("write");
	      p = 0;
	      tick++;

	    }
	}
      n++;
    }

  exit (0);
}
