/*
  Purpose: This program has been used to verify that the select() call works
 * Copyright (C) 4Front Technologies, 2002-2004. Released under GPLv2/CDDL.
 *
 * Description:
 * This program opens an audio device and then just
 * copies input to output. Select is used for flow control.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <soundcard.h>
#ifdef _AIX
#include <sys/select.h>
#endif

int
main (int agrc, char *argv[])
{
  int fd;

  int tmp;

  char buf[128 * 1024];

  int have_data = 0;
  int n, l;

  int frag = 0x00200008;	/* 32 fragments of 2^8=256 bytes */

  fd_set reads, writes;

  close (0);

  if ((fd = open ("/dev/dsp", O_RDWR, 0)) == -1)
    {
      perror ("/dev/dsp open");
      exit (-1);
    }

  ioctl (fd, SNDCTL_DSP_SETFRAGMENT, &frag);

/*
 * Set just the sampling tahe. Use the default format. We do not do any
 * error checking (maybe not so good idea) because we don't care what
 * the sampling rate really is.
 */
  tmp = 48000;
  ioctl (fd, SNDCTL_DSP_SPEED, &tmp);

  while (1)
    {
      struct timeval time;

      FD_ZERO (&reads);
      FD_ZERO (&writes);

      if (have_data)
	FD_SET (fd, &writes);
      else
	FD_SET (fd, &reads);

      time.tv_sec = 1;
      time.tv_usec = 0;
      if (select (fd + 1, &reads, &writes, NULL, &time) == -1)
	{
	  perror ("select");
	  exit (-1);
	}

      if (FD_ISSET (fd, &reads))
	{
	  struct audio_buf_info info;

	  if (ioctl (fd, SNDCTL_DSP_GETISPACE, &info) == -1)
	    {
	      perror ("select");
	      exit (-1);
	    }

	  n = info.bytes;

	  l = read (fd, buf, n);
	  if (l > 0)
	    have_data = 1;
	}

      if (FD_ISSET (fd, &writes))
	{
	  int i;

	  struct audio_buf_info info;

	  if (ioctl (fd, SNDCTL_DSP_GETOSPACE, &info) == -1)
	    {
	      perror ("select");
	      exit (-1);
	    }

	  n = info.bytes;

	  printf ("Write %d\n", l);
	  write (fd, buf, l);
	  printf ("OK");
	  have_data = 0;
	}
    }

  exit (0);
}
