/*
 * Purpose: A minimalistic MIDI output programming sample.
 * Copyright (C) 4Front Technologies, 2002-2004. Released under GPLv2/CDDL.
 *
 * Description:
 * This program does nothing but plays a note on MIDI channel 1
 * after sending a program change message.
 *
 * This program demonstrates how simple it's to write programs that play
 * MIDI. All you need to do is assembling the MIDI message and writing 
 * it to the device. The MIDI format is defined  in "MIDI 1.0 Detailed
 * Specification" which is available from MIDI Manufacturs Association (MMA)
 * (see {!hlink http://www.midi.org}).
 *
 * This program does timing by calling the sleep(3) system call. In some
 * systems like Linux there may be better sleep routines like usleep(3)
 * that provide better timing resolution.
 *
 * However application based timing may not be as precise as required in
 * musical applications. For this reason the MIDI interface of OSS will
 * provide a driver based timing approach in the near future.
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

  unsigned char note_on[] = { 0xc0, 0,	/* Program change */
    0x90, 60, 60
  };				/* Note on */
  unsigned char note_off[] = { 0x80, 60, 60 };	/* Note off */

  if ((fd = open (DEVICE, O_WRONLY, 0)) == -1)
    {
      perror ("open " DEVICE);
      exit (-1);
    }

  if (write (fd, note_on, sizeof (note_on)) == -1)
    {
      perror ("write " DEVICE);
      exit (-1);
    }

  sleep (1);			/* Delay one second */

  if (write (fd, note_off, sizeof (note_off)) == -1)
    {
      perror ("write " DEVICE);
      exit (-1);
    }

  close (fd);
  exit (0);
}
