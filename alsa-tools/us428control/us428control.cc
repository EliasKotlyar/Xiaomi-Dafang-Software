/* -*- mode:C++; indent-tabs-mode:t; tab-width:8; c-basic-offset: 8 -*- */
/*
 * Controller for Tascam US-X2Y
 *
 * Copyright (c) 2003 by Karsten Wiese <annabellesgarden@yahoo.de>
 * Copyright (c) 2004-2007 by Rui Nuno Capela <rncbc@rncbc.org>
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
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <poll.h>
#include <alsa/asoundlib.h>
#include "Cus428_ctls.h"
#include "Cus428State.h"
#include "Cus428Midi.h"


#define PROGNAME		"us428control"
#define SND_USX2Y_LOADER_ID	"USX2Y Loader"

/* max. number of cards (shouldn't be in the public header?) */
#define SND_CARDS	8

int verbose = 1;

Cus428Midi Midi;


static void error(const char *fmt, ...)
{
	if (verbose) {
		va_list ap;
		va_start(ap, fmt);
		fprintf(stderr, "%s: ", PROGNAME);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
}


static void usage(void)
{
	printf("Tascam US-428 Control\n");
	printf("version %s\n", VERSION);
	printf("usage: "PROGNAME" [-v verbosity_level 0..2] [-c card] [-D device] [-u usb-device] [-m mode]\n");
	printf("mode is one of (us224, us428, mixxx)\n");
}
/*
 * check the name id of the given hwdep handle
 */
static int check_hwinfo(snd_hwdep_t *hw, const char *id, const char* usb_dev_name)
{
	snd_hwdep_info_t *info;
	int err;

	snd_hwdep_info_alloca(&info);
	if ((err = snd_hwdep_info(hw, info)) < 0)
		return err;
	if (strcmp(snd_hwdep_info_get_id(info), id))
		return -ENODEV;
	if (usb_dev_name)
		if (strcmp(snd_hwdep_info_get_name(info), usb_dev_name))
			return -ENODEV;

	return 0; /* ok */
}

int US428Control(const char* DevName, int mode, int y)
{
	snd_hwdep_t		*hw;
	int			err;
	unsigned int		idx, dsps, loaded;
	us428ctls_sharedmem_t	*us428ctls_sharedmem;
	struct pollfd *pfds;
	int npfd, pollrc;

	if ((err = snd_hwdep_open(&hw, DevName, O_RDWR)) < 0) {
		if (verbose > 1)
			error("cannot open hwdep %s\n", DevName);
		return err;
	}

	if (check_hwinfo(hw, SND_USX2Y_LOADER_ID, NULL) < 0) {
		error("invalid hwdep %s\n", DevName);
		snd_hwdep_close(hw);
		return -ENODEV;
	}

	if (verbose > 0)
		fprintf(stderr, PROGNAME ": US-X2Y-compatible card found on hwdep %s\n", DevName);

	Midi.CreatePorts();

	npfd = snd_seq_poll_descriptors_count(Midi.Seq, POLLIN) + 1;
	pfds = (struct pollfd *) alloca(npfd * sizeof(struct pollfd));
	snd_hwdep_poll_descriptors(hw, &pfds[0], 1);
	snd_seq_poll_descriptors(Midi.Seq, &pfds[1], npfd - 1, POLLIN);

	us428ctls_sharedmem = (us428ctls_sharedmem_t *) mmap(NULL, sizeof(us428ctls_sharedmem_t), PROT_READ|PROT_WRITE, MAP_SHARED, pfds[0].fd, 0);
	if (us428ctls_sharedmem == MAP_FAILED) {
		perror("mmap failed:");
		snd_hwdep_close(hw);
		return -ENOMEM;
	}

	us428ctls_sharedmem->CtlSnapShotRed = us428ctls_sharedmem->CtlSnapShotLast;
	if (mode == 1)
		OneState = new Cus428StateMixxx(us428ctls_sharedmem, y);
	else
		OneState = new Cus428State(us428ctls_sharedmem, y);

	OneState->InitDevice();

	while ((pollrc = poll(pfds, npfd, 60000)) >= 0) {
		if (pfds[0].revents) {
			if (verbose > 1 || pfds[0].revents & (POLLERR|POLLHUP))
				printf("poll returned 0x%X\n", pfds[0].revents);
			if (pfds[0].revents & (POLLERR|POLLHUP))
				return 0; /* -ENXIO; */
			int Last = us428ctls_sharedmem->CtlSnapShotLast;
			if (verbose > 1)
				printf("Last is %i\n", Last);
			while (us428ctls_sharedmem->CtlSnapShotRed != Last) {
				static Cus428_ctls *Red = 0;
				int Read = us428ctls_sharedmem->CtlSnapShotRed + 1;
				if (Read >= N_us428_ctl_BUFS || Read < 0)
					Read = 0;
				Cus428_ctls* PCtlSnapShot = ((Cus428_ctls*)(us428ctls_sharedmem->CtlSnapShot)) + Read;
				int DiffAt = us428ctls_sharedmem->CtlSnapShotDiffersAt[Read];
				if (verbose > 1)
					PCtlSnapShot->dump(DiffAt);
				PCtlSnapShot->analyse(Red, DiffAt);
				Red = PCtlSnapShot;
				us428ctls_sharedmem->CtlSnapShotRed = Read;
			}
		}
		else if (pollrc > 0) Midi.ProcessMidiEvents();
	}

	return pollrc;
}

int main (int argc, char *argv[])
{
	int c;
	int y = 8;
	int mode = 0;
	int card = -1;
	char	*device_name = NULL,
		*usb_device_name = getenv("DEVICE");
	char name[64];

	while ((c = getopt(argc, argv, "c:D:u:v:m:")) != -1) {
		switch (c) {
		case 'c':
			card = atoi(optarg);
			break;
		case 'D':
			device_name = optarg;
			break;
		case 'u':
			usb_device_name = optarg;
			break;
		case 'v':
			verbose = atoi(optarg);
			break;
		case 'm':
			if (!strcmp(optarg, "us224"))
				y = 4;
			else
			if (!strcmp(optarg, "us428"))
				y = 8;
			else
			if (!strcmp(optarg, "mixxx"))
				mode = 1;
			break;
		default:
			usage();
			return 1;
		}
	}

	if (usb_device_name) {
		snd_hwdep_t *hw = NULL;
		for (c = 0; c < SND_CARDS; c++) {
			sprintf(name, "hw:%d", c);
			if ((0 <= snd_hwdep_open(&hw, name, O_RDWR))
			    && (0 <= check_hwinfo(hw, SND_USX2Y_LOADER_ID, usb_device_name))
			    && (0 <= snd_hwdep_close(hw))){
				card = c;
				break;
			}
		}
	}
	if (device_name) {
		return US428Control(device_name, mode, y) != 0;
	}
	if (card >= 0) {
		sprintf(name, "hw:%d", card);
		return US428Control(name, mode, y) != 0;
	}

	/* probe the all cards */
	for (c = 0; c < SND_CARDS; c++) {
		//	verbose--;
		sprintf(name, "hw:%d", c);
		if (US428Control(name, mode, y) == 0) {
			card = c;
			break;
		}
	}

	if (card < 0) {
		fprintf(stderr, PROGNAME ": no US-X2Y-compatible cards found\n");
		return 1;
	}

	return 0;
}

