/*
 * Purpose: A sample program for play target selection
 * Copyright (C) 4Front Technologies, 2002-2007. Released under GPLv2/CDDL.
 *
 * Description:
 * This program demonstrates the new ioctl call interface used to
 * control the play target selection.
 *
 * The first command line argument is the audio device (/dev/dsp#). If there 
 * are no other arguments then the available choices will be printed. If the 
 * second argument is "-" then the current setting will be printed.
 * Finally the source can be changed by giving it's name as the
 * second argument.
 *
 * {!notice Please not that the change may stay in effect even after closing
 * the device. However equally well it's possible that the device returns back
 * to some default source. There is no way to predict how the device will
 * behave and the application must not expect any particular behaviour.}
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <soundcard.h>
#include <time.h>

int
main (int argc, char *argv[])
{
  int fd, i, src;
  oss_mixer_enuminfo ei;

  if (argc < 2)
    {
      fprintf (stderr, "Usage: %s dspdev\n", argv[0]);
      exit (-1);
    }

  if ((fd = open (argv[1], O_WRONLY, 0)) == -1)
    {
      perror (argv[1]);
      exit (-1);
    }

  if (ioctl (fd, SNDCTL_DSP_GET_PLAYTGT_NAMES, &ei) == -1)
    {
      perror ("SNDCTL_DSP_GET_PLAYTGT_NAMES");
      exit (-1);
    }

  if (argc == 2)
    {
      for (i = 0; i < ei.nvalues; i++)
	printf ("Play target #%d = '%s'\n", i, ei.strings + ei.strindex[i]);
      exit (0);
    }

  if (strcmp (argv[2], "?") == 0 || strcmp (argv[2], "-") == 0)
    {
      if (ioctl (fd, SNDCTL_DSP_GET_PLAYTGT, &src) == -1)
	{
	  perror ("SNDCTL_DSP_GET_PLAYTGT");
	  exit (-1);
	}

      printf ("Current play target is #%d\n", src);
      printf ("Current play target is #%d (%s)\n",
	      src, ei.strings + ei.strindex[src]);
      exit (0);
    }

  src = 0;

  for (i = 0; i < ei.nvalues; i++)
    {
      if (strcmp (argv[2], ei.strings + ei.strindex[i]) == 0)
	{
	  if (ioctl (fd, SNDCTL_DSP_SET_PLAYTGT, &src) == -1)
	    {
	      perror ("SNDCTL_DSP_SET_PLAYTGT");
	      exit (-1);
	    }

	  exit (0);
	}

      src++;
    }

  fprintf (stderr, "What?\n");
  exit (-1);
}
