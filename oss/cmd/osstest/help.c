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
#include <errno.h>

void
describe_error (int err)
{
  switch (err)
    {
    case ENODEV:
      fprintf (stderr, "The device file was found in /dev but\n"
	       "OSS is not loaded. You need to load it by executing\n"
	       "the soundon command.\n");
      break;

    case ENXIO:
      fprintf (stderr, "There are no sound devices available.\n"
	       "The most likely reason is that the device you have\n"
	       "is malfunctioning or it's not supported by OSS.\n"
	       "It's also possible that you are trying to use the wrong "
	       "device file.\n"
	       "Please fill the problem report at\n"
	       "http://www.opensound.com/support.cgi\n");
      break;

    case ENOSPC:
      fprintf (stderr, "Your system cannot allocate memory for the device\n"
	       "buffers. Reboot your machine and try again.\n");
      break;

    case ENOENT:
      fprintf (stderr, "The device file is missing from /dev.\n"
	       "Perhaps you have not installed and started Open Sound System yet\n");
      break;


    case EBUSY:
      fprintf (stderr, "The device is busy. There is some other application\n"
	       "using it.\n");

      break;
    default:;
    }
}
