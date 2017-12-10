/*
 * Purpose: Sources for the ossmix command line mixer shipped with OSS
 *
 * Description:
 * The {!xlink ossmeter}  program was originally developed from a test bed
 * program for the new mixer API. However it has been included in the
 * oss package because there is need for a command line peak meter viewer.
 *
 * Authors:        ??         Original author of ossmix
 *          D. Casey Tucker   Producer of ossmeter
 */
/*
 *
 * This file should be part of Open Sound System.
 *
 * Copyright (C) 4Front Technologies 1996-2008.
 *
 * This this source file is released under GPL v2 license (no other versions).
 * See the COPYING file included in the main directory of this source
 * distribution for the license terms and conditions.
 *
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

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <soundcard.h>
#include <sys/ioctl.h>
#ifndef LOCAL_BUILD
#include <local_config.h>
#endif

static char *progname = NULL;
static int mixerfd = -1, nrext = 0, quiet = 0, verbose = 0, verbose_info = 0, nlines = 0;

static oss_mixext *extrec;
static oss_mixext_root *root;

static void change_level (int, const char *, const char *);
static void dump_all (int);
static void dump_devinfo (int);
static int find_enum (const char *, oss_mixext *, const char *);
static int find_name (const char *);
static void load_devinfo (int);
static void print_description (char *);
static void show_devinfo (int);

static void show_level (int, int);

static char * show_choices (const char *, oss_mixext *);
static char * show_enum (const char *, oss_mixext *, int);
static void usage (void);
static void verbose_devinfo (int);
#ifdef CONFIG_OSS_MIDI
static void midi_set (int, int, int);
static void midi_mixer (int, char *, char **, int, int);
static void smurf (int, int);
#endif

	static void
usage (void)
{
	printf ("Usage: %s -h		Displays help (this screen)\n", progname);
	printf ("Usage: %s [-d<devno>] [arguments]\n", progname);
	printf ("arguments:\n");
	printf ("\t-D			Display device information\n");
	printf ("\t-a			Dump mixer settings for all mixers (normal format)\n");
	printf ("\t-c			Dump mixer settings for all mixers (command format)\n");
	printf ("\tctrl# value		Change value of a mixer control\n");
	printf ("\t-q			Quiet mode\n");
	printf ("\t-v1|-v2		Verbose mode (-v2 is more verbose).\n");
	printf ("\t<no arguments>	Display current/possible settings\n");
	exit (-1);
}

static void
load_devinfo (int dev)
{
	int i;
	oss_mixext *thisrec;
	oss_mixerinfo mi;

	mi.dev = dev;
	if (ioctl (mixerfd, SNDCTL_MIXERINFO, &mi) != -1)
	{
		close (mixerfd);

		if ((mixerfd=open(mi.devnode, O_RDWR, 0)) == -1)
		{
			perror (mi.devnode);
			exit (EXIT_FAILURE);
		}
	}
	nrext = mi.nrext;

	if (nrext < 1)
	{
		fprintf (stderr, "Mixer device %d has no functionality\n", dev);
		exit (-1);
	}

	if ((extrec =
				(oss_mixext *)malloc ((nrext + 1) * sizeof (oss_mixext))) == NULL)
	{
		fprintf (stderr, "malloc of %d entries failed\n", nrext+1);
		exit (-1);
	}

	for (i = 0; i < nrext; i++)
	{
		thisrec = &extrec[i];
		thisrec->dev = dev;
		thisrec->ctrl = i;

		if (ioctl (mixerfd, SNDCTL_MIX_EXTINFO, thisrec) == -1)
		{
			if (errno == EINVAL)
			{
				fprintf (stderr, "Incompatible OSS version\n");
				exit (-1);
			}
			perror ("SNDCTL_MIX_EXTINFO");
			exit (-1);
		}

		if (thisrec->type == MIXT_DEVROOT)
			root = (oss_mixext_root *) thisrec->data;
	}
}

static void
verbose_devinfo (int dev)
{
	int i;
	oss_mixext *thisrec;
	oss_mixer_value val;
	val.dev = dev;


	if( nlines != 0 ) printf("\e[%dA", nlines);
	nlines=0;
	for (i = 0; i < nrext; i++)
	{
		thisrec = &extrec[i];

		val.ctrl = i;
		val.timestamp = thisrec->timestamp;
		val.value = -1;

		switch (thisrec->type)
		{
			case MIXT_STEREOVU:
			case MIXT_STEREOPEAK:
				printf ("%2d: %s \t", i, thisrec->id, thisrec->maxvalue);
				show_level(dev, i );
				nlines++; 
				break;
		}

	}
}

#define COLS 40
#define DB18 COLS * 0.125
#define DB6 COLS * 0.5
#define DB1 COLS * 0.89

void color(int c)
{
	if (c < DB18 )
		printf("\e[36m");
	else if (c < DB6)
		printf("\e[32m");
	else if ( c < DB1 )
		printf("\e[33m");
	else
		printf("\e[31m");
}

static void
show_level (int dev, int ctrl )
{
	int left = 0, right = 0;
	oss_mixer_value val;
	oss_mixext extrec;
	int mask = 0xff;
	int shift = 8;

	float fl_left, fl_right, db,
		  fl_max, fl_min;

	extrec.dev = dev;
	extrec.ctrl = ctrl;

	if (ioctl (mixerfd, SNDCTL_MIX_EXTINFO, &extrec) == -1)
	{
		perror ("SNDCTL_MIX_EXTINFO");
		exit (-1);
	}

	val.dev = dev;
	val.ctrl = ctrl;
	val.timestamp = extrec.timestamp;
	if (ioctl (mixerfd, SNDCTL_MIX_READ, &val) == -1)
	{
		perror ("SNDCTL_MIX_READ");
		exit (-1);
	}

	if (extrec.type == MIXT_MONOSLIDER16 || extrec.type == MIXT_STEREOSLIDER16)
	{
		mask = 0xffff;
		shift = 16;
	}

	switch (extrec.type)
	{
		case MIXT_STEREOSLIDER:
		case MIXT_STEREOSLIDER16:
		case MIXT_STEREODB:
		case MIXT_STEREOPEAK:
		case MIXT_STEREOVU:
			left = val.value & mask;
			right = (val.value >> shift) & mask;

			/*
			if (extrec.flags & MIXF_CENTIBEL)
				printf ("%d.%d:%d.%d (dB)\n",
					  left / 10, left % 10, right / 10, right % 10);
			else
				printf ("%d:%d\n", left, right);
				*/
			break;
		case MIXT_VALUE:
			//printf ("%d\n", val.value);
			break;
		default:
			left = val.value & mask;

			/*
			if (extrec.flags & MIXF_CENTIBEL)
				printf ("%d.%d (dB)\n",
						left / 10, left % 10);
			else
				printf ("%d\n", left);
			*/
			break;
	}
	fl_left = left / (float) extrec.maxvalue;
	fl_right = right / (float) extrec.maxvalue;
	fl_min = fl_left < fl_right ? fl_left : fl_right;
	fl_max = fl_left > fl_right ? fl_left : fl_right;

	int c=0;
	for(; c < fl_min * COLS; c++){
		color(c);
		printf("▐");
	}
	for(; c < fl_left * COLS; c++){
		color(c);
		printf("▀");
	}
	for(; c < fl_right * COLS; c++){
		color(c);
		printf("▄");
	}
	for(; c < COLS; c++){
		color(c);
		printf("-");
	}
	db = 20. * log10(fl_max);
	printf ("\e[0m %+8.2f \n", db );
}

int
main (int argc, char *argv[])
{
	int i;
	extern char * optarg;
	extern int optind, optopt;
	int dev = 0, c;
	const char * devmixer;

	progname = argv[0];

	if ((devmixer = getenv("OSS_MIXERDEV")) == NULL)
		devmixer = "/dev/mixer";

	if ((mixerfd = open (devmixer, O_RDWR, 0)) == -1)
	{
		perror (devmixer);
		exit (-1);
	}

	verbose_info = 1;
	load_devinfo (dev);

	while(1)
	{
		verbose_devinfo (dev);
		usleep(500*1000);
	}
	exit (0);

	close (mixerfd);
	return 0;
}
