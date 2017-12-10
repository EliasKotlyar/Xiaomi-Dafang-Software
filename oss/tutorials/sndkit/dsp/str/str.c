/************************************************************************/
/*									*/
/* str.c - plays sound-/noisetracker files on a SparcStation		*/
/*									*/
/* Authors:								*/
/*	    Liam Corner - zenith@dcs.warwick.ac.uk			*/
/*	    Marc Espie - espie@dmi.ens.fr				*/
/* Minor modificatios for Linux by					*/
/*	    Hannu Savolainen - hannu@voxware.pp.fi			*/
/* Command-line switches added by					*/
/*	    Muhammad Saggaf - alsaggaf@erl.mit.edu			*/
/*	    Leif Kornstaedt - l_kornst@informatik.uni-kl.de		*/
/* Relaxed program name checking					*/
/*	    Craig Metz - cmetz@thor.tjhsst.edu				*/
/* Stereo and 16 bit support by						*/
/*	    Hannu							*/
/* Some effects and minor bugfixes/ameliorations by			*/
/*	    Leif Kornstaedt - l_kornst@informatik.uni-kl.de		*/
/*									*/
/* Version: 1.20 - 3 November 1991 / 31 January 1993			*/
/* 01/31/93 effects added						*/
/*	    by Leif Kornstaedt						*/
/* 08/27/92 modified to use integer arithmetic				*/
/* 08/31/92 SB Pro stereo support					*/
/*	    by Hannu Savolainen						*/
/*									*/
/************************************************************************/

#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include <sys/soundcard.h>
#include "str.h"

/* output file name or dsp device */
char AUDIO[256] = AUDIONAME;

int audio;			/* file handle */
unsigned char *audiobuf;
int abuf_size;
int abuf_ptr;

/* for analyzing the command line: */
char *command;			/* the actual command name used (argv[0]) */
int loop_forever = FALSE;
int quiet_mode = FALSE, verbose_mode = FALSE;
static int use_zcat = FALSE;
int mute[4] = { FALSE, FALSE, FALSE, FALSE };
int dsp_stereo = DSP_DEFAULT_STEREO;
int dsp_samplesize = DSP_DEFAULT_SAMPLESIZE;
long dsp_speed = DEFAULT_DSP_SPEED;

FILE *fp;

void play_song (void);
extern void describe_error (void);	/* from ../help.c */

void
usage (command)
     char *command;
{
  fprintf (stderr, "Usage: %s [ -cqvzS1234 -b size -s kHz -o output_file ] "
	   "[ filename ... ]\n", command);
  fprintf (stderr, "\t-c\t\tplay infinitely\n"
	   "\t-q\t\tquiet mode\n"
	   "\t-v\t\tverbose mode\n"
	   "\t-z\t\tuse zcat on file\n"
	   "\t-1234\t\tmute voices 1, 2, 3 or 4\n"
	   "\t-S\t\ttoggle stereo\n"
	   "\t-b size\t\tset sample size (8 or 16 bits)\n"
	   "\t-s kHz\t\tuse sampling speed kHz\n"
	   "\t-o file\t\tredirect output to a file\n");
  exit (EXIT_FAILURE);
}

char *
find_file (name)
     char *name;
{
  char *dirs;
  struct stat statbuffer;
  static char fname[256];

  strcpy (fname, name);
  if (stat (fname, &statbuffer) != -1 && errno != ENOENT)
    return fname;
  else
    {
      dirs = getenv ("MODPATH");
      while (dirs != NULL && strlen (dirs))
	{
	  if (strchr (dirs, ':') != NULL)
	    {
	      strncpy (fname, dirs, strchr (dirs, ':') - dirs);
	      fname[strchr (dirs, ':') - dirs] = '\0';
	    }
	  else
	    strcpy (fname, dirs);
	  if (fname[strlen (fname) - 1] != '/')
	    strcat (fname, "/");
	  strcat (fname, name);
	  if (stat (fname, &statbuffer) != -1 && errno != ENOENT)
	    return fname;
	  if ((dirs = strchr (dirs, ':')) != NULL)
	    dirs++;
	}
    }

  return NULL;
}

int
main (argc, argv)
     int argc;
     char *argv[];
{
  int outdev, c;
  struct stat statbuffer;
  int opmode = O_WRONLY;	/* mode for opening the output device */
  char zcat_cmd[256];
  char *currfile;

  command = argv[0];

  while ((c = getopt (argc, argv, "cqvz1234Sb:s:o:")) != EOF)
    switch (c)
      {
      case 'c':		/* play infinitely */
	loop_forever = TRUE;
	break;
      case 'q':		/* quiet mode */
	quiet_mode = TRUE;
	break;
      case 'v':		/* verbose mode */
	verbose_mode = TRUE;
	break;
      case 'z':		/* use zcat on file */
	use_zcat = TRUE;
	break;
      case '1':
      case '2':
      case '3':
      case '4':
	mute[c - '1'] = TRUE;
	break;
      case 'S':		/* toggle stereo */
	dsp_stereo = !DSP_DEFAULT_STEREO;
	break;
      case 'b':		/* sample size */
	dsp_samplesize = atoi (optarg);
	break;
      case 's':		/* dsp speed */
	dsp_speed = atoi (optarg);
	if (dsp_speed < 300)
	  dsp_speed *= 1000;
	break;
      case 'o':		/* output file */
	strcpy (AUDIO, optarg);
	break;
      default:
	usage (command);
      }

  /* output to a character device (TRUE) or a file (FALSE)? */
  outdev = !stat (AUDIO, &statbuffer) && S_ISCHR (statbuffer.st_mode);

  if (!outdev)
    opmode |= O_CREAT;		/* file: create it */
  if ((audio = open (AUDIO, opmode, 0)) == -1)
    {
      perror (AUDIO);
      describe_error ();
      exit (EXIT_FAILURE);
    }


  if (outdev)
    {				/* set dsp parameters */
      if (ioctl (audio, SNDCTL_DSP_SETFMT, &dsp_samplesize) == -1)
	dsp_samplesize = 8;
      if (ioctl (audio, SNDCTL_DSP_STEREO, &dsp_stereo) == -1)
	dsp_stereo = FALSE;
      if (ioctl (audio, SNDCTL_DSP_SPEED, &dsp_speed) == -1)
	{
	  fprintf (stderr, "%s: unable to set playback speed\n", command);
	  perror (AUDIO);
	  exit (EXIT_FAILURE);
	}
      if (!quiet_mode)
	printf ("Playback speed %d Hz (%s) %d bits/sample.\n", dsp_speed,
		dsp_stereo ? "stereo" : "mono", dsp_samplesize);
    }

  if (outdev)
    {				/* get the dsp buffer size */
#if 0
/*
 * There is no idea in using SNDCTL_DSP_GETBLKSIZE in applications like this.
 * Using some fixed local buffer size will work equally well.
 */
      ioctl (audio, SNDCTL_DSP_GETBLKSIZE, &abuf_size);
      if (abuf_size < 1)
	{
	  perror ("audio_size");
	  close (audio);
	  exit (EXIT_FAILURE);
	}
#else
      abuf_size = 1024;
#endif
    }
  else
    {				/* to a file: use block size of 1kB */
      abuf_size = 1024;
      loop_forever = 0;
    }

  if ((audiobuf = malloc (abuf_size)) == NULL)
    {
      fprintf (stderr, "%s: unable to allocate output buffer\n", command);
      close (audio);
      exit (EXIT_FAILURE);
    }
  abuf_ptr = 0;

  if (optind > argc - 1)
    {
      if (use_zcat)
	{
	  if ((fp = popen (ZCAT, "rb")) == NULL)
	    {
	      fprintf (stderr, "%s: can't execute " ZCAT ".\n", command);
	      return (EXIT_FAILURE);
	    }
	}
      else
	fp = stdin;
      play_song ();
      if (use_zcat)
	pclose (fp);
    }
  else
    while (optind <= argc - 1)
      if ((currfile = find_file (argv[optind])) == NULL)
	{
	  fprintf (stderr, "%s: can't find file %s - skipping.\n",
		   command, argv[optind]);
	  optind++;
	}
      else if (use_zcat)
	{
	  if ((fp = popen (strcat (strcpy (zcat_cmd, ZCAT " "),
				   currfile), "rb")) == NULL)
	    {
	      fprintf (stderr, "%s: can't execute " ZCAT " %s.\n",
		       command, currfile);
	      exit (EXIT_FAILURE);
	    }
	  else
	    {
	      play_song ();
	      pclose (fp);
	      if (!loop_forever)
		optind++;
	    }
	}
      else
	{
	  if ((fp = fopen (currfile, "rb")) == NULL)
	    {
	      fprintf (stderr, "%s: unable to open tune file %s.\n",
		       command, currfile);
	      exit (EXIT_FAILURE);
	    }
	  else
	    {
	      play_song ();
	      fclose (fp);
	      if (!loop_forever)
		optind++;
	    }
	}

  if (abuf_ptr)
    write (audio, audiobuf, abuf_ptr);
  close (audio);

  return EXIT_SUCCESS;
}
