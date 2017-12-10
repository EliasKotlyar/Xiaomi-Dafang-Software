/*
 * Purpose: A minimalistic MIDI input programming sample.
 * Copyright (C) 4Front Technologies, 2002-2004. Released under GPLv2/CDDL.
 *
 * Description:
 * This simple program opens a MIDI device file and displays everything
 * it receives in hexadecimal format.
 *
 * Please look at the "{!link MIDI}" section of the OSS Developer's
 * manual for more info about MIDI programming.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <soundcard.h>

#define DEVICE "/dev/midi"

int
main ()
{
  int fd;
  unsigned char buf[128];
  int l;

  if ((fd = open (DEVICE, O_RDONLY, 0)) == -1)
    {
      perror ("open " DEVICE);
      exit (-1);
    }

/*
 * Note that read may return any number of bytes between 0 and sizeof(buf).
 * -1 means that some error has occurred.
 */

  while ((l = read (fd, buf, sizeof (buf))) != -1)
    {
      int i;

      for (i = 0; i < l; i++)
	{
	  if (buf[i] & 0x80)	/* Status byte */
	    printf ("\n");
	  printf ("%02x ", buf[i]);
	}

      fflush (stdout);
    }

  close (fd);
  exit (0);
}
