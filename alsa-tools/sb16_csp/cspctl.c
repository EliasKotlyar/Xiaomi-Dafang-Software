/*
 *  SB16/AWE32 Creative Signal Processor (ASP/CSP) control program
 *
 *  Copyright (c) 2000 by Uros Bizjak <uros@kss-loka.si>
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

#include <alsa/asoundlib.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <linux/ioctl.h>
#include <alsa/sound/sb16_csp.h>

/* --- commands --- */
enum {
     COMMAND_LOAD,
     COMMAND_UNLOAD
};

snd_sb_csp_microcode_t microcode;

int card = 0;

static void error(const char *fmt,...)
{
	va_list va;

	va_start(va, fmt);
	fprintf(stderr, "Error: ");
	vfprintf(stderr, fmt, va);
	fprintf(stderr, "\n");
	va_end(va);
}

static void help(char *command)
{
	fprintf(stderr,
		"Usage: %s command [-options] <filename>\n"
		"\nAvailable commands:\n"
		"  load    load microcode from filename\n"
		"  unload  unload microcode from CSP\n"
		"\nAvailable options:\n"
		"  -h,--help        this help\n"
		"  -c card          select card number, default = 0\n"
		"  -f function      select CSP function #, defaults to 1\n"
		"  -d description   optional codec description string\n"
		, command);
}

static int csp_command (int idx, int dev, int command, char *filename)
{
	int fd, err;
	snd_hwdep_t *handle;
	char name[16];

	sprintf(name, "hw:%i,%i", idx, dev);

	/* open CSP hwdep device */
	if ((err = snd_hwdep_open(&handle, name, O_WRONLY)) < 0) {
		error("CSP open (%i-%i): %s", idx, dev, snd_strerror(err));
		exit(1);
	}

	switch (command) {
	case COMMAND_LOAD:
		/* open microcode file */
		if ((fd = open(filename, O_RDONLY, 0)) == -1) {
			perror(filename);
			exit(1);
		}
		/* read microcode to buffer */
		if (read(fd, &microcode.data, sizeof(microcode.data)) < 0) {
			error("%s: read error", filename);
			exit(1);
		}
		close(fd);

		/* load microcode to CSP */
		if (snd_hwdep_ioctl(handle, SNDRV_SB_CSP_IOCTL_LOAD_CODE, &microcode) < 0) {
			error("unable to load microcode");
			exit(1);
		}
		break;
	case COMMAND_UNLOAD:
		/* unload microcode from CSP */
		if (snd_hwdep_ioctl(handle, SNDRV_SB_CSP_IOCTL_UNLOAD_CODE, NULL) < 0) {
			error("unable to unload microcode");
			exit(1);
		}
	}

	snd_hwdep_close(handle);
	return 0;
}

int main(int argc, char *argv[])
{
	int dev;
	int command, c;
	int err;

	char card_id[32];

	snd_ctl_t *ctl_handle;
	snd_ctl_card_info_t *card_info;
	snd_hwdep_info_t *hwdep_info;

	snd_ctl_card_info_alloca(&card_info);
	snd_hwdep_info_alloca(&hwdep_info);

	if (argc > 1 && !strcmp(argv[1], "--help")) {
		help(argv[0]);
		return 0;
	}

	strcpy (microcode.info.codec_name, "UNKNOWN");
	microcode.info.func_req = 1;
	while ((c = getopt(argc, argv, "hc:f:d:")) != EOF) {
		switch (c) {
		case 'h':
			help(argv[0]);
			return 0;
		case 'c':
			{
				card = snd_card_get_index(optarg);
				if (card < 0 || card > 31) {
					error ("wrong -c argument '%s'\n", optarg);
					return 1;
				}
			}
			break;
		case 'f':
			microcode.info.func_req = atoi(optarg);
			if ((microcode.info.func_req < 1) || (microcode.info.func_req > 4)) {
				error("value %i for function number is invalid",
				      microcode.info.func_req);
				return 1;
			}
			break;
		case 'd':
			if (strlen(optarg) > 16) {
				error("codec description '%s' too long", optarg);
				return 1;
			}
			strcpy(microcode.info.codec_name, optarg);
			break;
		default:
			return 1;
		}
	}
	if (optind >= argc) {
		error("please specify command");
		return 1;
	}
	if (!strcmp (argv[optind], "load")) {
	     command = COMMAND_LOAD;
	} else if (!strcmp (argv[optind], "unload")) {
	     command = COMMAND_UNLOAD;
	} else {
		error ("command should be either 'load' or 'unload'");
		return 1;
	}

	if ((command == COMMAND_LOAD) && (++optind >= argc)) {
		error ("missing microcode filename");
		return 1;
	}

	// Get control handle for selected card
	sprintf(card_id, "hw:%i", card);
	if ((err = snd_ctl_open(&ctl_handle, card_id, 0)) < 0) {
		error("control open (%s): %s", card_id, snd_strerror(err));
		return 1;
	}

	// Read control hardware info from card
	if ((err = snd_ctl_card_info(ctl_handle, card_info)) < 0) {
		error("control hardware info (%s): %s", card_id, snd_strerror(err));
		exit(1);
	}

	// CSP chip is present only on SB16 and SB AWE cards
	if (strcmp(snd_ctl_card_info_get_driver(card_info), "SB16") != 0 &&
	    strcmp(snd_ctl_card_info_get_driver(card_info), "SB AWE") != 0) {
		error("not a SB16 or SB AWE type card");
		exit(1);
	}

	// find CSP hardware dependant device and execute command
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
		if (snd_hwdep_info_get_iface(hwdep_info) == SND_HWDEP_IFACE_SB16CSP) {
			err = csp_command (card, dev, command, argv[optind]);
			break;
		}
	}
	if (err)
		error("no CSP device present");

	snd_ctl_close(ctl_handle);
	return 0;
}
