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
#include <string.h>

#include <sys/soundcard.h>
#include <sys/libossmix.h>

static void
event_callback(ossmix_callback_parm_t *parms)
{
	switch (parms->event)
	{
	case OSSMIX_EVENT_VALUE:
		printf("Value change, mixer=%d, ctl=%d, value=0x%08x\n", parms->p1, parms->p2, parms->p3);
		break;

	case OSSMIX_EVENT_NEWMIXER:
		printf("Number of mixers increased to %d\n", parms->p1);
		break;

	default:
		printf("Event callback %d, %d, %d, %d, %d\n", parms->event, parms->p1, parms->p2, parms->p3, parms->p4);
	}
}

static void
command_parser(char *line)
{
	char *p = line + strlen(line)-1;

	if (*p == '\n')
	   *p=0;

	if (strcmp(line, "exit")==0 || strcmp(line, "q")==0)
	   exit(0);
}

static void
interactive_mode(void)
{
  int libfd=-1, maxdev, ndevs;
  fd_set readfds;
  ossmix_select_poll_t pollfunc;
  struct timeval tmout;

  libfd=ossmix_get_fd(&pollfunc);

  ossmix_set_callback(event_callback);

  printf("libfd=%d, func=%p\n", libfd, pollfunc);
  printf("\n");
  printf("> ");fflush(stdout);

  if (libfd >= 0)
     {
  	tmout.tv_sec = 0;
  	tmout.tv_usec = 100000; /* 0.1 sec */
     }
  else
     {
  	tmout.tv_sec = 0;
  	tmout.tv_usec = 0;
     }

  while (1)
    {
      FD_ZERO (&readfds);

      maxdev = 0;

      FD_SET (0, &readfds); // Stdin
      if (libfd > 0)
         FD_SET (libfd, &readfds);

      if (libfd > maxdev)
	maxdev = libfd;

      if ((ndevs =
	   select (maxdev + 1, &readfds, NULL, NULL, &tmout)) == -1)
	{
	  perror ("select");
	  exit (-1);
	}

      if (ndevs == 0)
	{
	  ossmix_timertick();
  	  tmout.tv_sec = 0;
  	  tmout.tv_usec = 100000; /* 0.1 sec */
	}

      if (FD_ISSET (0, &readfds)) /* Stdio */
      {
	      char line[128];

	      if (fgets(line, sizeof(line)-1, stdin) != NULL)
	      {
		command_parser(line);
  		printf("> ");fflush(stdout);
	      }
      }

      if (libfd > 0)
      if (FD_ISSET (libfd, &readfds))
	 pollfunc();
    }
}

static void
print_enum_list(int mixnum, int ctl)
{
	oss_mixer_enuminfo desc;
	int i;
	
	if (ossmix_get_enuminfo(mixnum, ctl, &desc)<0)
	{
		fprintf(stderr, "ossmix_get_enuminfo() failed\n");
		return;
	}

	for (i=0;i<desc.nvalues;i++)
	{
		if (i>0)printf(" | ");
		printf("%s", desc.strings+desc.strindex[i]);
	}

	printf("\n");
}

static void
print_description(int mixnum, int ctl)
{
	oss_mixer_enuminfo desc;
	
	if (ossmix_get_description(mixnum, ctl, &desc)<0)
	{
		fprintf(stderr, "ossmix_get_description() failed\n");
		return;
	}

	printf("%s\n", desc.strings);
}

int
main(int argc, char *argv[])
{
	int err, i;
	char *host=NULL;
	int port=7777;
	int nmixers=0;
	extern int mixlib_trace;
	int interactive=0;

	//mixlib_trace=1;

	while ((i = getopt(argc, argv, "ip:h:")) != EOF)
	switch (i)
	{
	case 'i':
		interactive=1;
		break;
	case 'p':
		port = atoi(optarg);
		break;

	case 'h':
		host=optarg;
		break;
	}

	if ((err=ossmix_init())<0)
	{
		fprintf(stderr, "ossmix_init() failed, err=%d\n");
		exit(EXIT_FAILURE);
	}

	if ((err=ossmix_connect(host, port))<0)
	{
		fprintf(stderr, "ossmix_connect() failed, err=%d\n", err);
		exit(EXIT_FAILURE);
	}

	if ((nmixers=ossmix_get_nmixers())<0)
	{
		fprintf(stderr, "ossmix_get_nmixers() failed, err=%d\n", nmixers);
		exit(EXIT_FAILURE);
	}

	printf("Number of mixers=%d\n", nmixers);

	for (i=0;i<nmixers;i++)
	{
		oss_mixerinfo mi;
		int n, ctl;

		if (ossmix_get_mixerinfo(i, &mi)<0)
		{
			fprintf(stderr, "ossmix_get_mixerinfo(%d) failed\n", i);
			exit(EXIT_FAILURE);
		}

		printf("Mixer %2d: %s\n", i, mi.name);

		if (ossmix_open_mixer(i)<0)
		{
			fprintf(stderr, "ossmix_open_mixer(%d) failed\n", i);
			exit(EXIT_FAILURE);
		}

		if ((n=ossmix_get_nrext(i))<0)
		{
			fprintf(stderr, "ossmix_get_nrext(%d) failed, err=\n", i, n);
			exit(EXIT_FAILURE);
		}

		printf("Mixer has %d nodes\n", n);

		for (ctl=0;ctl<n;ctl++)
		{
			oss_mixext node;
			int value=0;

			if (ossmix_get_nodeinfo(i, ctl, &node)<0)
			{
				fprintf(stderr, "ossmix_get_nodeinfo(%d, %d) failed\n",
						i, ctl);
				exit(EXIT_FAILURE);

			}

			if (node.type != MIXT_DEVROOT && node.type != MIXT_GROUP && node.type != MIXT_MARKER)
			if ((value=ossmix_get_value(i, ctl, node.timestamp))<0)
			{
				fprintf(stderr, "ossmix_get_value(%d, %d, %d) failed, err=%d\n",
						i, ctl, node.timestamp, value);
			}

			printf("%3d: %s = 0x%08x\n", ctl, node.extname, value);

			if (node.type == MIXT_ENUM)
			   print_enum_list(i, ctl);

			if (node.flags & MIXF_DESCR)
			   print_description(i, ctl);
			   
		}

		if (!interactive)
		   ossmix_close_mixer(i);
	}

	if (interactive)
	   interactive_mode();

printf("Disconnecting\n");
	ossmix_disconnect();

	exit(EXIT_SUCCESS);
}
