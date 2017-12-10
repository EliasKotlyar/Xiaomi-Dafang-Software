/*
 * help.c
 *
 * Some error printing routines.
 * 
 * Copyright by Hannu Savolainen 1993
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer. 2.
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 */
#include <stdio.h>
#include <errno.h>

void
describe_error (void)
{
  switch (errno)
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
	       "\n"

	       "NOTE! It may be necessary to reboot the system after installing OSS\n"
#ifdef LICENSED_VERSION
	       "\n"
		"If you are a licensed customer then please fill the problem report at\n"
	        "http://www.opensound.com/support.cgi\n"
#endif
		);
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

    default:;
    }
}
