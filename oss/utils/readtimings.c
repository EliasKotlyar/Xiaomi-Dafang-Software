/*
 * Purpose: OSS built in tracing reader
 *
 * Description:
 * This program is used to read timing/tracing information produced by the
 * built in tracing facility of OSS.
 *
 * To use this program you need to rebuild OSS with the DO_TIMINGS macro
 * defined. This macro is located in the os.h file for the target operating
 * system. Output is written to stdout. Tracing information is currently
 * produced only for the audio devices.
 *
 * To understand the output you need to be familiar with the internals of OSS.
 */
/*
 *
 * This file is part of Open Sound System.
 *
 * Copyright (C) 4Front Technologies 1996-2008.
 *
 * This this source file is released under GPL v2 license (no other versions).
 * See the COPYING file included in the main directory of this source
 * distribution for the license terms and conditions.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int
main (void)
{
  int fd;
  char buf[257 * 1024];
  int l;

  if ((fd = open ("/dev/mixer0", O_RDONLY, 0)) == -1)
    {
      perror ("/dev/mixer0");
      exit (-1);
    }

  while (1)
    {
      if ((l = read (fd, buf, sizeof (buf))) < 0)
	{
	  perror ("read");
	  exit (-1);
	}

      if (l)
	write (1, buf, l);

      usleep (100000);
    }

  close (fd);
  exit (0);
}
