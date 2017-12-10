/*
 * Purpose: Utility to control the vmix subsystem of Open Sound System
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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <oss_config.h>
#include <sys/ioctl.h>

char *cmdname=NULL;

#ifndef CONFIG_OSS_VMIX
int
main(int argc, char *argv[])
{
	fprintf (stderr, "%s: Virtual mixer is not included in this version of OSS\n", argv[0]);
	exit(1);
}
#else

static void
usage(void)
{
	fprintf (stdout, "Usage:\n");
	fprintf (stdout, "%s attach [attach_options...] devname\n", cmdname);
	fprintf (stdout, "%s attach [attach_options...] devname inputdev\n", cmdname);
	fprintf (stdout, "%s detach devname\n", cmdname);
	fprintf (stdout, "%s rate devname samplerate\n", cmdname);
	fprintf (stdout, "%s remap devname channels\n", cmdname);
	fprintf (stdout, "\n");
	fprintf (stdout, "Use ossinfo -a to find out the devname and inputdev parameters\n");
	fprintf (stdout, "Use ossinfo -a -v2 to find out a suitable sample rate.\n");
	fprintf (stdout, "\n");
	fprintf (stdout, "attach_options:\n");
	fprintf (stdout, "\n");
	fprintf (stdout, "\t-r\tDisable recording\n");
	fprintf (stdout, "\t-p\tDo not preallocate client engines\n");
	fprintf (stdout, "\t-M\tUse more fragments\n");
	fprintf (stdout, "\t-V\tMake clients visible by creating device files for them.\n");
	fprintf (stdout, "\t-c<N>\tPrecreate <N> client engines (see -p).\n");

	exit(-1);
}

static int
find_audiodev(char *fname, int mode, int *fd_)
{
	int fd;
	oss_audioinfo ai;

	if ((fd=*fd_=open(fname, mode | O_EXCL, 0))==-1)
	{
		perror(fname);
		exit(-1);
	}

	ai.dev=-1;
	if (ioctl(fd, SNDCTL_ENGINEINFO, &ai)==-1)
	{
		perror("SNDCTL_ENGINEINFO");
		fprintf (stderr, "Cannot get engine info for %s\n", fname);
		exit(-1);
	}

	return ai.dev;
}

static int
vmix_attach(int argc, char **argv)
{
	int masterfd, inputfd=-1;
	int masterdev, inputdev=-1;
	int c, n;
	int relink_devices = 0;
  	extern int optind;
  	extern char * optarg;

	vmixctl_attach_t att={0};

	att.attach_flags = VMIX_INSTALL_MANUAL;
/*
 * Simple command line switch handling.
 */
	argv++;argc--; /* Skip the initial command ("attach") */

  	while ((c = getopt (argc, argv, "MVc:pr")) != EOF)
    	   {
      		switch (c)
        	{
		case 'r': /* No input */
			att.attach_flags |= VMIX_INSTALL_NOINPUT;
			break;

		case 'p': /* No client engine preallocation */
			att.attach_flags |= VMIX_INSTALL_NOPREALLOC;
			break;

		case 'V': /* Allocate private device files for all clients */
			att.attach_flags |= VMIX_INSTALL_VISIBLE;
			relink_devices=1;
			break;

		case 'M': /* Use more fragments */
			att.attach_flags |= VMIX_MULTIFRAG;
			break;

		case 'c': /* Force prealloc of N client devices */
			n = atoi(optarg);
			if (n>255)n=255;
			if (n<0)n=0;
			att.attach_flags |= n;
			break;		

		default:
			usage();
		}
	   }

	if (optind >= argc)
	   usage();

	masterdev=find_audiodev(argv[optind], O_WRONLY, &masterfd);

	optind++;

	if (optind<argc)
	   inputdev=find_audiodev(argv[optind], O_RDONLY, &inputfd);
	   
	att.masterdev=masterdev;
	att.inputdev=inputdev;

	if (ioctl(masterfd, VMIXCTL_ATTACH, &att)==-1)
	{
		perror("VMIXCTL_ATTACH");
		exit(-1);
	}

	fprintf (stdout, "Virtual mixer attached to device.\n");

	if (relink_devices)
	{
		n = system("ossdetect -d");
		n = system("ossdevlinks");
	}

	return 0;
}

static int
vmix_detach(int argc, char **argv)
{
	int masterfd;
	int masterdev;

	vmixctl_attach_t att;

	masterdev=find_audiodev(argv[2], O_WRONLY, &masterfd);

	att.masterdev=masterdev;
	att.inputdev=-1;

	if (ioctl(masterfd, VMIXCTL_DETACH, &att)==-1)
	{
		perror("VMIXCTL_DETACH");
		exit(-1);
	}

	fprintf (stdout, "Virtual mixer detached from device.\n");

	return 0;
}

static int
vmix_rate(int argc, char **argv)
{
	int masterfd;
	int masterdev;

	vmixctl_rate_t rate;

	if (argc<4)
	{
		usage ();
	}

	masterdev=find_audiodev(argv[2], O_WRONLY, &masterfd);

	rate.masterdev=masterdev;
	rate.rate=atoi(argv[3]);

	if (ioctl(masterfd, VMIXCTL_RATE, &rate)==-1)
	{
		perror("VMIXCTL_RATE");
		exit(-1);
	}

	fprintf (stdout, "Virtual mixer rate change requested.\n");

	return 0;
}

static int
vmix_remap(int argc, char **argv)
{
	int i;
	int masterfd;
	int masterdev;

	vmixctl_map_t map;

	if (argc<4)
	{
		usage ();
	}

	masterdev=find_audiodev(argv[2], O_WRONLY, &masterfd);
	memset (&map, 0, sizeof (map));

	map.masterdev=masterdev;
	map.map[0]=atoi(argv[3]);

        for (i=4; i<argc; i++)
	{
		map.map[i-3] = atoi(argv[i]);
	}

	if (ioctl(masterfd, VMIXCTL_REMAP, &map)==-1)
	{
		perror("VMIXCTL_REMAP");
		exit(-1);
	}

	fprintf (stdout, "Virtual mixer map change requested.\n");

	return 0;
}

int
main(int argc, char **argv)
{
	cmdname=argv[0];

	if (argc < 3)
	{
		usage ();
	}

	if (strcmp(argv[1], "attach")==0)
	   exit(vmix_attach(argc, argv));

	if (strcmp(argv[1], "detach")==0)
	   exit(vmix_detach(argc, argv));

	if (strcmp(argv[1], "rate")==0)
	   exit(vmix_rate(argc, argv));

	if (strcmp(argv[1], "remap")==0)
	   exit(vmix_remap(argc, argv));

	usage();
	exit(0);
}
#endif
