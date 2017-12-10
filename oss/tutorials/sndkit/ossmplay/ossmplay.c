#define DEB(stmt)	/* stmt	*/	/* For debug printing */
#define DEB2(stmt)	/* stmt */	/* For debug printing */
#define DEB3(stmt)	stmt	/* For debug printing */
#define DEB_ALWAYS(stmt)	stmt	/* Interesting things */

#define _INCLUDE_POSIX_SOURCE 1
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/errno.h>
#include "../../../include/soundcard.h"

extern int errno;

#include "mlib.h"

SEQ_DEFINEBUF (1024);

int seqfd;
mlib_track *tracks[1024];
int ntrks = 0;
int dev = 0;

void
player ()
{
  int i, track;
  int prev_time = 0;

  int ptrs[1024] = { 0 };
  unsigned char *ptr;
  unsigned char *event;
  int time, n = 0;

#if 0
  for (track = 0; track < ntrks; track++)
    for (i = 0; i < 128; i++)
      {
	if (tracks[track]->pgm_map[i] != -1)
	  {
	    n++;
	    SEQ_LOAD_GMINSTR (dev, i);
	  }
	tracks[track]->pgm_map[i] = i;

	if (n == 0)		/* No program changes. Assume pgm# 0 */
	  SEQ_LOAD_GMINSTR (dev, 0);

	if (tracks[track]->drum_map[i] != -1)
	  SEQ_LOAD_GMDRUM (dev, i);
	tracks[track]->drum_map[i] = i;
      }

  if (n == 0)			/* No program changes detected */
    SEQ_LOAD_GMINSTR (dev, 0);	/* Acoustic piano */

#endif

  SEQ_START_TIMER ();
  while (1)
    {
      int best = -1, best_time = 0x7fffffff;

      for (i = 0; i < ntrks; i++)
	if (ptrs[i] < tracks[i]->len)
	  {
	    ptr = &(tracks[i]->events[ptrs[i] * 12]);
	    event = &ptr[4];
	    time = *(int *) ptr;

	    if (time < best_time)
	      {
		best = i;
		best_time = time;
	      }
	  }

      if (best == -1)
	return;

      ptr = &(tracks[best]->events[ptrs[best] * 12]);
      event = &ptr[4];
      time = *(int *) ptr;
      ptrs[best]++;

      if (event[0] < 128)
	{
	}
      else
	{
	  int j;

	  if (time > prev_time)
	    {
	      SEQ_WAIT_TIME (time);
	      prev_time = time;
	    }

	  if (event[0] == EV_SYSEX)
	    {
	      event[1] = dev;
	    }

	  if ((event[0] & 0xf0) == 0x90)
	    {
	      event[1] = dev;

	      if (event[0] == EV_CHN_COMMON && event[2] == MIDI_PGM_CHANGE)
		{
		  event[4] = tracks[best]->pgm_map[event[4]];
		}
	    }

	  _SEQ_NEEDBUF (8);
	  memcpy (&_seqbuf[_seqbufptr], event, 8);
	  _SEQ_ADVBUF (8);

	}
    }
}

int
main (int argc, char *argv[])
{
  mlib_desc *mdesc;
  int was_last;
  int tmp, argp = 1;
  oss_longname_t song_name;
  char *p, *s, *devname="/dev/midi00";;
  extern void OSS_set_timebase(int tb);

  if (argc < 2)
    {
      fprintf (stderr, "Usage: %s midifile\n", argv[0]);
      exit (-1);
    }

  if (argc==3)
  {
     devname=argv[1];
     argp++;
  }


  if ((seqfd = open (devname, O_WRONLY, 0)) == -1)
    {
      perror (devname);
      exit (-1);
    }
  dev=0;

#ifdef OSSLIB
  OSS_init (seqfd, 1024);
#endif

  if ((mdesc = mlib_open (argv[argp])) == NULL)
    {
      fprintf (stderr, "Can't open MIDI file %s: %s\n",
	       argv[argp], mlib_errmsg ());
      exit (-1);
    }
  ioctl(seqfd, SNDCTL_SETSONG, argv[argp]);

/*
 * Extract the file name part of the argument
 */

  p=s=argv[argp];
  while (*s)
  {
	if (*s=='/')
	   p=s+1;
	s++;
  }
  
  memset(song_name, 0, sizeof(song_name));
  strcpy(song_name, p);

#if 1
  tmp=MIDI_MODE_TIMED;
  if (ioctl(seqfd, SNDCTL_MIDI_SETMODE, &tmp)==-1)
  {
	perror("SNDCTL_MIDI_SETMODE");
	exit(-1);
  }
#endif

  tmp = mdesc->hdr.division;
printf("Timebase %d\n", tmp);
  OSS_set_timebase(tmp);
  if (ioctl(seqfd, SNDCTL_MIDI_TIMEBASE, &tmp)==-1)
  {
	perror("SNDCTL_MIDI_TIMEBASE");
	exit(-1);
  }

  ntrks = 0;

  while ((tracks[ntrks] = mlib_loadtrack (mdesc, &was_last)) != NULL)
    {
      int i;

      DEB2 (printf ("Loaded track %03d: len = %d events, flags = %08x\n",
		    mdesc->curr_trk, tracks[ntrks]->len,
		    tracks[ntrks]->flags));
      ntrks++;
    }

  if (!was_last)
    {
      fprintf (stderr, "%s: %s\n", argv[argp], mlib_errmsg ());
      exit (-1);
    }

  tmp = (int) mdesc->timesig;
  printf("Timesig %08x\n", tmp);

/*
 * Set the current song name (OSS 4.0 feature).
 */
  ioctl(seqfd, SNDCTL_SETSONG, song_name);
  player ();
  SEQ_DELTA_TIME (mdesc->hdr.division * 8);
  SEQ_PGM_CHANGE(0, 0, 0);
  SEQ_DUMPBUF ();

  mlib_close (mdesc);
  close (seqfd);
  exit (0);
}
