/*
 * Sample server program that uses the oss_userdev driver.
 *
 * Copyright (C) 4Front Technologies. Released under the BSD license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>

/*
 * OSS specific includes. Use correct -I setting when compiling. Typically
 * -I/usr/lib/oss/include/sys or -I/usr/include/sys
 */
#include <soundcard.h>
#include <oss_userdev_exports.h>

#define SERVER_DEVNAME		"/dev/oss/oss_userdev0/server"

int server_fd = -1;	/* File descriptor for the server side device. */

static void
terminator(int sig)
{
	wait();
	exit(0);
}

static void
create_mixer_interface(int fd)
{
	userdev_mixctl_t ctl;
	userdev_mixgroup_t grp;
	userdev_mixvalues_t rec;
	int group;

/*
 * Create a slider on the top level
 */

	memset(&ctl, 0, sizeof(ctl));
	strcpy(ctl.name, "volumitaz");
	ctl.parent	=	0; /* Device root group */
	ctl.type	= 	MIXT_STEREOSLIDER16;
	ctl.flags	= 	MIXF_READABLE|MIXF_WRITEABLE;
	ctl.index	= 	0;	/* Use position 0 of the value array */
	ctl.maxvalue	= 	100;
	ctl.offset	= 	0;
	ctl.rgbcolor	=	OSS_RGB_RED;

	if (ioctl(fd, USERDEV_CREATE_MIXCTL, &ctl)==-1)
	{
		perror("USERDEV_CREATE_MIXCTL");
		return;
	}
	
/*
 * Create a mixer group under the device root
 */

	memset(&grp, 0, sizeof(grp));

	strcat(grp.name, "private");
	grp.parent	=	0;
	
	if (ioctl(fd, USERDEV_CREATE_MIXGROUP, &grp)==-1)
	{
		perror("USERDEV_CREATE_MIXGROUP");
		return;
	}

	group = grp.num;

/*
 * Create an enumerated control under the "private" group that was created above
 */
	memset(&ctl, 0, sizeof(ctl));
	strcpy(ctl.name, "mode");
	ctl.parent	=	group; /* See above */
	ctl.type	= 	MIXT_ENUM;
	ctl.flags	= 	MIXF_READABLE|MIXF_WRITEABLE;
	ctl.index	= 	1;	/* Use position 1 of the value array */
	ctl.maxvalue	= 	4;

	memset (&ctl.enum_present, 0xff, sizeof(ctl.enum_present)); /* Mark all choices active */

	strcpy(ctl.enum_choises, "stall crawl cruise warp");

	if (ioctl(fd, USERDEV_CREATE_MIXCTL, &ctl)==-1)
	{
		perror("USERDEV_CREATE_MIXCTL");
		return;
	}

/*
 * Finally set the initial values for all the controls
 */
	memset(&rec, 0, sizeof(rec)); /* Set all to zeroes */

	rec.values[0] = 100 | (100<<16);	// volumitaz = 100:100
	rec.values[1] = 2;			// private.mode = "cruise"

/*
 * Post the initial settings
 */
	if (ioctl(fd, USERDEV_SET_MIXERS, &rec)==-1)
	{
		perror("USERDEV_SET_MIXERS");
	}
}

static void
poll_mixer(int fd)
{
	static int prev_count=0;
	int count;

	if (ioctl(fd, USERDEV_GET_MIX_CHANGECOUNT, &count)==-1)
	{
		perror("USERDEV_GET_MIX_CHANGECOUNT");
		return;
	}

	if (count > prev_count) /* Something has changed */
	{
		userdev_mixvalues_t rec;
		int i;

		if (ioctl(fd, USERDEV_GET_MIXERS, &rec)==-1)
		{
			perror("USERDEV_GET_MIXERS");
			return;
		}

		printf("Mixer change %d\n", count);

		/*
		 * Print only the controls that were allocated in
		 * create_mixer_interface()
		 */
		for (i=0;i<2;i++)
		    printf("%2d: %08x\n", i, rec.values[i]);
		fflush(stdout);
	}

	prev_count = count;
}

int
main(int argc, char *argv[])
{
	userdev_create_t crea = {0};
	char cmd[512];

/*
 * Sample rate/format we would like to use on server side. The client side
 * can use whatever they want since OSS will automatically do the required 
 * conversions.
 */
	int rate = 48000;
	int fmt = AFMT_S16_LE;
	int channels = 2;



	int tmp;
	int fragsize=0;

	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <command>\n", argv[0]);
		exit(-1);
	}

/*
 * Open the server side device. A new userdev instance (client&server)
 * will automatically get created when the device is opened. However
 * creation of the client side will be delayed until USERDEV_CREATE_INSTANCE
 * gets called.
 */

	if ((server_fd = open(SERVER_DEVNAME, O_RDWR, 0))==-1)
	{
		perror(SERVER_DEVNAME);
		exit(-1);
	}

/*
 * Create the client side device.
 */
	sprintf(crea.name, "udserver device for %s", getenv("USER"));
	crea.flags = USERDEV_F_VMIX_ATTACH | USERDEV_F_VMIX_PRIVATENODE; /* Doesn't work at this moment */
	crea.match_method = UD_MATCH_UID;
	crea.match_key = geteuid();
	crea.poll_interval = 10; /* In milliseconds */

	if (ioctl(server_fd, USERDEV_CREATE_INSTANCE, &crea)==-1)
	{
		perror("USERDEV_CREATE_INSTANCE");
		exit(-1);
	}

/*
 * Set up the master side parameters such as sampling rate and sample format.
 * The server application can select whatever format is best for its
 * purposes. The client side can select different rate/format if necessary.
 */

	tmp=0;
	ioctl(server_fd, SNDCTL_DSP_COOKEDMODE, &tmp); /* Turn off conversions */

	if (ioctl(server_fd, SNDCTL_DSP_SETFMT, &fmt)==-1)
	   perror("SNDCTL_DSP_SETFMT");

	if (ioctl(server_fd, SNDCTL_DSP_CHANNELS, &channels)==-1)
	   perror("SNDCTL_DSP_CHANNELS");

	if (ioctl(server_fd, SNDCTL_DSP_SPEED, &rate)==-1)
	   perror("SNDCTL_DSP_SPEED");

	if (ioctl(server_fd, SNDCTL_DSP_GETBLKSIZE, &fragsize)==-1)
	   fragsize = 1024;

	create_mixer_interface(server_fd);

	if (fork())
	{
		/*
		 * Server side code. In this case we have just a simple echo loop
		 * that writes back everything everything received from the client side.
		 */
		int l;
		int poll_count=0;

		char *buffer;
		signal(SIGCHLD, terminator);

		buffer = malloc (fragsize);
		memset(buffer, 0, fragsize);

		write(server_fd, buffer, fragsize);
		write(server_fd, buffer, fragsize);

		while ((l=read(server_fd, buffer, fragsize))>0)
		{
			if (write(server_fd, buffer, l)!=l)
			{
				perror("write");
				exit(-1);
			}

			if (poll_count++ > 10)
			{
				poll_mixer(server_fd);
				poll_count = 0;
			}
		}

		exit(0);
	}

/*
 * Client side code. Simply execute the command that was given in
 * argv[1]. However replace all %s's by the client side device node name.
 */

	if (setenv("OSS_AUDIODEV", crea.devnode, 1) == -1)
	   perror("setenv OSS_AUDIODEV");

	if (setenv("OSS_MIXERDEV", crea.devnode, 1) == -1)
	   perror("setenv OSS_MIXERDEV");

	setenv("PS1", "udserver> ", 1);

	sprintf(cmd, argv[1], crea.devnode, crea.devnode, crea.devnode);
	printf("Running '%s'\n", cmd);

	system(cmd);
	exit(0);
}
