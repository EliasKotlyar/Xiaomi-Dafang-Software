/*
 * Purpose: Measuring the hardware level latencies.
 * Copyright (C) 4Front Technologies, 2002-2004. Released under GPLv2/CDDL.
 *
 * Description:
 * This simple program was once used to measure internal latencies of some
 * sound cards. It was written in great hurry so don't use it as a program
 * template. Error checking is completely inadequate.
 *
 * This program requires that the output of the sound card is connected
 * to the line in of the same card.
 *
 * The program will play a short spike and then measure how many "silent "
 * samples there are in the input before the spike is seen. You can
 * get the delay in seconds by dividing this sample count by the
 * sample rate.
 *
 * This is the total delay value that includes both the output and input
 * delays. There is no way to estimate how large part of it is caused by
 * input or output.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <soundcard.h>
#ifdef _AIX
#include <sys/select.h>
#endif

int frag_size = 0;
static int pos = 0;

static int
open_device (char *name, int mode)
{
  int tmp, fd;

  int frag = 0x0020000b;	/* 32 fragments of 2^11=2048 bytes */

  if ((fd = open (name, mode, 0)) == -1)
    {
      perror (name);
      exit (-1);
    }

/*
 * WARNING!!!!!!!!!!!
 *
 * The following code makes ioctl calls without verifying that the
 * result was OK. Don't do this at home.
 */
  if (ioctl (fd, SNDCTL_DSP_SETFRAGMENT, &frag) == -1)
    perror ("SNDCTL_DSP_SETFRAGMENT");

  tmp = 1;
  if (ioctl (fd, SNDCTL_DSP_CHANNELS, &tmp) == -1)
    perror ("SNDCTL_DSP_CHANNELS");

  tmp = AFMT_S16_NE;
  if (ioctl (fd, SNDCTL_DSP_SETFMT, &tmp) == -1)
    perror ("SNDCTL_DSP_SETFMT");

  tmp = 48000;
  if (ioctl (fd, SNDCTL_DSP_SPEED, &tmp) == -1)	/* And #channels & #bits if required */
    perror ("SNDCTL_DSP_SPEED");

  if (ioctl (fd, SNDCTL_DSP_GETBLKSIZE, &frag_size) == -1)
    perror ("SNDCTL_DSP_GETBLKSIZE");

  tmp = 0;
  if (ioctl (fd, SNDCTL_DSP_SETTRIGGER, &tmp) == -1)
    perror ("SNDCTL_DSP_SETTRIGGER");
/*
 * WARNING!!!!!!!!!!!
 *
 * The above code makes ioctl calls without verifying that the
 * result was OK. Don't do this at home.
 */

  return fd;
}

static void
gen_spike (short *buf)
{
  int i, p;

  for (i = 0; i < 30; i++)
    {
      p = (i / 4) % 3;
      buf[i] = (p - 1) * 8 * 1024;
    }
}

static void
check_buf (short *buf, int l)
{
  int i;

  for (i = 0; i < l; i++)
    {
      int v;

      v = buf[i];
      if (v < 0)
	v = -v;
      if (v > 4096)
	printf ("%d = %d\n", pos, v);
      pos++;
    }
}

int
main (int argc, char *argv[])
{
  int fd_in, fd_out;
  char *name_in = "/dev/dsp", *name_out = NULL;

  int tmp;
  int have_data = 0;

  char buf[128 * 1024];

  int n, l;

  fd_set reads, writes;

  if (argc > 1)
    name_in = argv[1];
  if (argc > 2)
    name_out = argv[2];

  if (name_out != NULL)
    {
      fd_in = open_device (name_in, O_RDONLY);
      fd_out = open_device (name_out, O_WRONLY);
    }
  else
    {
      fd_in = fd_out == open_device (name_in, O_RDWR);
    }

  memset (buf, 0, frag_size);
  gen_spike ((short *) buf);
  write (fd_out, buf, frag_size);

  if (fd_out != fd_in)
    {
      tmp = PCM_ENABLE_INPUT;
      ioctl (fd_in, SNDCTL_DSP_SETTRIGGER, &tmp);
      tmp = PCM_ENABLE_OUTPUT;
      ioctl (fd_out, SNDCTL_DSP_SETTRIGGER, &tmp);
    }
  else
    {
      tmp = PCM_ENABLE_INPUT | PCM_ENABLE_OUTPUT;
      ioctl (fd_in, SNDCTL_DSP_SETTRIGGER, &tmp);
    }

  while (1)
    {
      struct timeval time;

      FD_ZERO (&reads);
      FD_ZERO (&writes);

      FD_SET (fd_out, &writes);
      FD_SET (fd_in, &reads);

      time.tv_sec = 0;
      time.tv_usec = 100000;
      if ((l = select (fd_out + 1, &reads, &writes, NULL, &time)) == -1)
	{
	  perror ("select");
	  exit (-1);
	}
      if (l == 0)
	printf ("Timeout ");

      if (FD_ISSET (fd_in, &reads))
	{
	  struct audio_buf_info info;

	  if (ioctl (fd_in, SNDCTL_DSP_GETISPACE, &info) == -1)
	    {
	      perror ("select");
	      exit (-1);
	    }

	  n = info.bytes;
	  if (n <= 0)
	    {
	      printf ("Error: NREAD=%d\n", n);
	      exit (-1);
	    }

	  l = read (fd_in, buf, n);
	  if (l > 0)
	    have_data = 1;
	  else
	    perror ("read");
	  check_buf ((short *) buf, l / 2);
	}

      if (FD_ISSET (fd_out, &writes))
	{
	  int i;

	  struct audio_buf_info info;

	  if (ioctl (fd_out, SNDCTL_DSP_GETOSPACE, &info) == -1)
	    {
	      perror ("select");
	      exit (-1);
	    }

	  n = info.bytes;

	  /* printf("Write %d", l); */
	  l = n;
	  memset (buf, 0, l);
	  if (write (fd_out, buf, l) == l);
	  have_data = 0;
	}
    }

  exit (0);
}
