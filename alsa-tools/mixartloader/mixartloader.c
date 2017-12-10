/*
 * Firmware Loaderfor Digigram miXart soundcards
 *
 * Copyright (c) 2003 by Digigram <alsa@digigram.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdarg.h>
#include <alsa/asoundlib.h>


#define PROGNAME	"mixartloader"


/* max. number of cards (shouldn't be in the public header?) */
#define SND_CARDS	8


static int verbose = 0;

static void usage(void)
{
	printf("Boot loader for Digigram miXart cards\n");
	printf("version %s\n", VERSION);
	printf("usage: mixartloader [-c card] [-D device]\n");
}

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

/*
 * parse the index file and get the file to read from the key
 */

#define MAX_PATH	128

static int get_file_name(const char *key, unsigned int idx, char *fname)
{
	FILE *fp;
	char buf[128];
	char temp[32], *p;
	int len;

	snprintf(buf, sizeof(buf), "%s/%s.conf", DATAPATH, key);
	if ((fp = fopen(buf, "r")) == NULL) {
		error("cannot open the index file %s\n", buf);
		return -ENODEV;
	}

	sprintf(temp, "dsp%d", idx);
	len = strlen(temp);

	while (fgets(buf, sizeof(buf), fp)) {
		if (strncmp(buf, temp, len))
			continue;
		for (p = buf + len; *p && isspace(*p); p++)
			;
		if (*p == '/') {
			strncpy(fname, p, MAX_PATH);
		} else {
			snprintf(fname, MAX_PATH, "%s/%s", DATAPATH, p);
		}
		fname[MAX_PATH-1] = 0;
		/* chop the last linefeed */
		for (p = fname; *p && *p != '\n'; p++)
			;
		*p = 0;
		fclose(fp);
		return 0;
	}
	fclose(fp);
	error("cannot find the file entry for %s\n", temp);
	return -ENODEV;
}


/*
 * read and transfer the firmware binary
 */
static int load_firmware(snd_hwdep_t *hw, const char *id, unsigned int idx)
{
	int err;
	char fname[MAX_PATH];
	snd_hwdep_dsp_image_t *dsp;
	struct stat st;
	int fd, length;
	unsigned char *imgbuf;

	if (get_file_name(id, idx, fname) < 0)
		return -EINVAL;

	snd_hwdep_dsp_image_alloca(&dsp);

	snd_hwdep_dsp_image_set_name(dsp, fname);
	if (stat(fname, &st) < 0) {
		error("cannot call stat %s\n", fname);
		return -ENODEV;
	}

	length = st.st_size;
	if (length == 0) {
		error("zero file size %s\n", fname);
		return -EINVAL;
	}

	imgbuf = malloc(st.st_size);
	if (! imgbuf) {
		error("cannot malloc %d bytes\n", length);
		return -ENOMEM;
	}
	snd_hwdep_dsp_image_set_length(dsp, length);
	snd_hwdep_dsp_image_set_image(dsp, imgbuf);

	fd = open(fname, O_RDONLY);
	if (fd < 0) {
		error("cannot open %s\n", fname);
		return -ENODEV;
	}
	if (read(fd, imgbuf, length) != length) {
		error("cannot read %d bytes from %s\n", length, fname);
		close(fd);
		return -EINVAL;
	}

	close(fd);

	snd_hwdep_dsp_image_set_index(dsp, idx);

	err = snd_hwdep_dsp_load(hw, dsp);
	if (err < 0)
		error("error in loading %s\n", fname);

	printf("%s loaded (%d bytes).\n",fname,length);
	return err;
}


/*
 * check the name id of the given hwdep handle
 */
static int check_hwinfo(snd_hwdep_t *hw, const char *id)
{
	snd_hwdep_info_t *info;
	int err;

	snd_hwdep_info_alloca(&info);
	if ((err = snd_hwdep_info(hw, info)) < 0)
		return err;
	if (strcmp(snd_hwdep_info_get_id(info), id))
		return -ENODEV;
	return 0; /* ok */
}

/*
 * load the firmware binaries
 */
static int miXart_boot(const char *devname)
{
	snd_hwdep_t *hw;
	const char *id;
	int err;
	unsigned int idx, dsps, loaded;
	snd_hwdep_dsp_status_t *stat;

	if ((err = snd_hwdep_open(&hw, devname, O_RDWR)) < 0) {
		error("cannot open hwdep %s\n", devname);
		return err;
	}
      
         /* check hwdep ID string */
	if (check_hwinfo(hw, "miXart Loader") < 0) {
		error("invalid hwdep ID string %s\n", devname);
		snd_hwdep_close(hw);
		return -ENODEV;
	}

	snd_hwdep_dsp_status_alloca(&stat);
	/* get the current status (number of files to load = 3)*/
	if ((err = snd_hwdep_dsp_status(hw, stat)) < 0) {
		printf("cannot get version for %s\n", devname);
		snd_hwdep_close(hw);
		return err;
	}

	if (snd_hwdep_dsp_status_get_chip_ready(stat))
	  {
	    printf("nothing to do for device %s.\n", devname);
            return 0; /* already loaded */
	  }

	/* get "miXart" string */
        id = snd_hwdep_dsp_status_get_id(stat);
	dsps = snd_hwdep_dsp_status_get_num_dsps(stat);
	loaded = snd_hwdep_dsp_status_get_dsp_loaded(stat);

	for (idx = 0; idx < dsps; idx++) {
		if (loaded & (1 << idx))
		  {
		    printf("%d already loaded (loaded = %d).\n", idx, loaded);
		    continue;
		  }
		if ((err = load_firmware(hw, id, idx)) < 0) {
			snd_hwdep_close(hw);
			return err;
		}
	}

	snd_hwdep_close(hw);
	return 0;
}


int main(int argc, char **argv)
{
	int c;
	int card = -1;
	char *device_name = NULL;
	char name[64];

	while ((c = getopt(argc, argv, "c:D:")) != -1) {
		switch (c) {
		case 'c':
			card = atoi(optarg);
			break;
		case 'D':
			device_name = optarg;
			break;
		default:
			usage();
			return 1;
		}
	}

	if (device_name) {
		verbose = 1;
		return miXart_boot(device_name) != 0;
	}
	if (card >= 0) {
		sprintf(name, "hw:%d", card);
		verbose = 1;
		return miXart_boot(name) != 0;
	}

	/* probe the all cards */
	for (c = 0; c < SND_CARDS; c++) {
		sprintf(name, "hw:%d", c);
		if (! miXart_boot(name))
			card = c;
	}
	if (card < 0) {
		fprintf(stderr, PROGNAME ": no miXart-compatible card found\n");
		return 1;
	}
	return 0;
}
