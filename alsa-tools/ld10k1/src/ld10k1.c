/*
 *  EMU10k1 loader
 *
 *  Copyright (c) 2003,2004 by Peter Zubaj
 *
 *   Hwdep usage based on sb16_csp
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <limits.h>
#include <sys/stat.h>
#include <alsa/asoundlib.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/ioctl.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include "ld10k1.h"
#include "ld10k1_fnc.h"
#include "ld10k1_fnc1.h"

int card = 0;
snd_hwdep_t *handle;
char comm_pipe[256];
FILE *comm;
char pidpath[256];
FILE *logfile=NULL;

static void vlog(const char *label, const char *fmt, va_list va)
{
	FILE *out = stderr;

	if (logfile)
		out = logfile;

	if (logfile) {
		char timestr[20];
		time_t tp;

		tp = time(NULL);
		strftime(timestr, sizeof(timestr), "%b %d %H:%M:%S",
			 localtime(&tp));
		fprintf(out, "%s %s", timestr, label);
	} else 
		fprintf(out, "%s", label);
	vfprintf(out, fmt, va);
	fprintf(out, "\n");
	fflush(out);
}

void error(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vlog("Error: ", fmt, va);
	va_end(va);
}

static void log(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	vlog("", fmt, va);
	va_end(va);
}

static void help(char *command)
{
	fprintf(stderr,
		"Usage: %s [-options]\n"
		"\nAvailable options:\n"
		"  -h, --help        this help\n"
		"  -c, --card        select card number, default = 0\n"
		"  -p, --pipe_name   connect to this, default = /tmp/.ld10k1_port\n"
		"  -n, --network     listen on port\n"
		"      --port        port number, default = 20480\n"
		"  -d  --daemon      start in background\n"
		"  -i  --pidfile     print daemon process id to file, default /var/run/ld10k1.pid\n"
		"  -l  --logfile     \n"
		"  -t, --tram_size   initialize tram with given size\n"
		"		0 -    0 KB\n"
		"		1 -   16 KB\n"
		"		2 -   32 KB\n"
		"		3 -   64 KB\n"
		"		4 -  128 KB\n"
		"		5 -  256 KB\n"
		"		6 -  512 KB\n"
		"		7 - 1024 KB\n"
		"		8 - 2048 KB\n"
		, command);
}

static void cleanup()
{
	if (pidpath[0])
		unlink(pidpath);
	log("Exiting daemon");
}

static void term_handler(int i)
{
	exit(1);
}

int tram_size_table[] = {0, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576};

int main(int argc, char *argv[])
{
	int dev;
	int c;
	int err;
	int audigy;

	int opt_help = 0;
	int tram_size = 0;
	int opt_daemon = 0;
	unsigned short opt_port = 20480;
	int uses_pipe = 1;
	char logpath[255];

	char card_id[32];
	const char *card_proc_id;

	snd_ctl_t *ctl_handle;
	snd_ctl_card_info_t *card_info;
	snd_hwdep_info_t *hwdep_info;
	
	char name[16];

	comm_param params;
	
	int option_index;
	
	static struct option long_options[] = {
				   {"help", 0, 0, 'h'},
				   {"card", 1, 0, 'c'},
				   {"pipe_name", 1, 0, 'p'},
				   {"network", 0, 0, 'n'},
				   {"port", 1, 0, 0},
				   {"daemon", 0, 0, 'd'},
				   {"tram_size", 1, 0, 't'},
				   {"pidfile", 1, 0, 'i'},
				   {"logfile", 1, 0, 'l'},
                   {0, 0, 0, 0}
               };

	snd_ctl_card_info_alloca(&card_info);
	snd_hwdep_info_alloca(&hwdep_info);

	strcpy(comm_pipe,"/tmp/.ld10k1_port");
	strcpy(pidpath, "/var/run/ld10k1.pid");
	memset(logpath, 0, sizeof(logpath));

	option_index = 0;
	while ((c = getopt_long(argc, argv, "hc:p:t:ndl:i:",
	        long_options, &option_index)) != EOF) {
		switch (c) {
		case 0:
			if (strcmp(long_options[option_index].name, "port") == 0) {
				opt_port = atoi(optarg);
			}
			break;
		case 'h':
			opt_help = 1;
			break;
		case 'c':
			card = snd_card_get_index(optarg);
			if (card < 0 || card > 31) {
				error ("wrong -c argument '%s'\n", optarg);
				return 1;
			}
			break;
		case 'p':
			uses_pipe = 1;
			strncpy(comm_pipe, optarg, sizeof(comm_pipe) - 1);
			comm_pipe[sizeof(comm_pipe) - 1] = '\0';
			break;
		case 'n':
			uses_pipe = 0;
			break;
		case 'd':
			opt_daemon = 1;
			break;
		case 't':
			tram_size = atoi(optarg);
			if (tram_size < 0)
				tram_size = 0;
			else if (tram_size > 8)
				tram_size = 8;
			tram_size = tram_size_table[tram_size];
			break;
		case 'i':
			strncpy(pidpath, optarg, sizeof(pidpath) - 1);
			pidpath[sizeof(pidpath) - 1] = '\0';
			break;
		case 'l':
			strncpy(logpath, optarg, sizeof(logpath) - 1);
			logpath[sizeof(logpath) - 1] = '\0';
			break;
		default:
			return 1;
		}
	}

	if (opt_help) {
		help(argv[0]);
		return 0;
	}
	
	if (getuid() != 0 ) {
		error("You are not running ld10k1 as root.");
		return 1;
	}
	
	if (logpath[0])
		logfile = fopen(logpath, "at");

	if (opt_daemon) {
		FILE *pidfile;

		if (daemon(0, 0) < 0) {
		    error("Unable to run as daemon.");
    		    return 1;
		}
		
		pidfile = fopen(pidpath, "wt");
		if (!pidfile) {
			log("%s: pidfile (%s)\n", strerror(errno), pidpath);
			return 1;
		}
		
		fprintf(pidfile, "%d\n", getpid());
		fflush(pidfile);
		fclose(pidfile);
		
		atexit(cleanup);
		signal(SIGTERM, term_handler);

		if (logfile) {
			dup2(fileno(logfile), fileno(stderr));
			dup2(fileno(logfile), fileno(stdout));
		}
		
		log("Starting daemon");
	}

	params.type = uses_pipe ? COMM_TYPE_LOCAL : COMM_TYPE_IP;
	params.name = comm_pipe;
	params.server = 1;
	params.port = opt_port;
	params.wfc = 0;

	/* Get control handle for selected card */
	sprintf(card_id, "hw:%i", card);
	if ((err = snd_ctl_open(&ctl_handle, card_id, 0)) < 0) {
		error("control open (%s): %s", card_id, snd_strerror(err));
		return 1;
	}

	/* Read control hardware info from card */
	if ((err = snd_ctl_card_info(ctl_handle, card_info)) < 0) {
		error("control hardware info (%s): %s", card_id, snd_strerror(err));
		exit(1);
	}

	if (!(card_proc_id = snd_ctl_card_info_get_id (card_info))) {
		error("card id (%s): %s", card_id, snd_strerror(err));
		exit(1);
	}


	/* EMU10k1/EMU10k2 chip is present only on SB Live, Audigy, Audigy 2, E-mu APS cards */
	if (strcmp(snd_ctl_card_info_get_driver(card_info), "EMU10K1") != 0 &&
	    strcmp(snd_ctl_card_info_get_driver(card_info), "Audigy") != 0 &&
		strcmp(snd_ctl_card_info_get_driver(card_info), "Audigy2") != 0 &&
		strcmp(snd_ctl_card_info_get_driver(card_info), "E-mu APS") != 0) {
		error("not a EMU10K1/EMU10K2 based card");
		exit(1);
	}

	if (strcmp(snd_ctl_card_info_get_driver(card_info), "Audigy") == 0 ||
		strcmp(snd_ctl_card_info_get_driver(card_info), "Audigy2") == 0)
		audigy = 1;
	else
		audigy = 0;
		
	/* find EMU10k1 hardware dependant device and execute command */
	dev = -1;
	err = 1;
	while (1) {
		if (snd_ctl_hwdep_next_device(ctl_handle, &dev) < 0)
			error("hwdep next device (%s): %s", card_id, snd_strerror(err));
		if (dev < 0)
			break;
		snd_hwdep_info_set_device(hwdep_info, dev);
		if (snd_ctl_hwdep_info(ctl_handle, hwdep_info) < 0) {
			if (err != -ENOENT)
				error("control hwdep info (%s): %s", card_id, snd_strerror(err));
			continue;
		}
		if (snd_hwdep_info_get_iface(hwdep_info) == SND_HWDEP_IFACE_EMU10K1) {
			sprintf(name, "hw:%i,%i", card, dev);

			/* open EMU10k1 hwdep device */
			if ((err = snd_hwdep_open(&handle, name, O_WRONLY)) < 0) {
				error("EMU10k1 open (%i-%i): %s", card, dev, snd_strerror(err));
				exit(1);
			}

			while (1)
				if (main_loop(&params, audigy, card_proc_id, tram_size, ctl_handle)) {
					error("error in main loop");
					break;
				}
			
			snd_hwdep_close(handle);

			break;
		}
	}

	snd_ctl_close(ctl_handle);

	return 0;
}
