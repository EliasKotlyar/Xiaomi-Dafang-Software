/*
 * FPGA loader for Tascam US-X2Y
 *
 * Copyright (c) 2003 by Karsten Wiese <annabellesgarden@yahoo.de>
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


#define PROGNAME		"usx2yloader"
#define SND_USX2Y_LOADER_ID	"USX2Y Loader"

/* max. number of cards (shouldn't be in the public header?) */
#define SND_CARDS	8


static int verbose = 0;

static void usage(void)
{
	printf("FPGA loader for Tascam US-X2Y\n");
	printf("version %s\n", VERSION);
	printf("usage: usx2yloader [-c card] [-D device] [-u usb-device]\n");
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
 * read a xilinx bitstream file.
 * NOTE: This interprets somehow differently from the vxloaders read_xilinx_image()!
 * I took this from rbtload.c of the project usb-midi-fw.sf.net.
 */
static int read_xilinx_image(snd_hwdep_dsp_image_t *img, const char *fname)
{
	FILE *fp;
	char buf[256];
	int data = 0, c = 0, idx = 0, length = 0;
	char *p;
	char *imgbuf = 0;

	if ((fp = fopen(fname, "r")) == NULL) {
		fprintf(stderr, PROGNAME ": cannot open %s\n", fname);
		return -EINVAL;
	}
	snd_hwdep_dsp_image_set_name(img, fname);

	while (fgets(buf, sizeof(buf), fp)) {
		if (strncmp(buf, "Bits:", 5) == 0) {
			for (p = buf + 5; *p && isspace(*p); p++);
			if (!*p) {
				fprintf(stderr,
					PROGNAME
					": corrupted file %s in Bits line\n",
					fname);
				fclose(fp);
				return -EINVAL;
			}
			length = atoi(p);
			length /= 8;
			if (length <= 0) {
				fprintf(stderr,
					PROGNAME
					": corrupted file %s, detected length = %d\n",
					fname, length);
				fclose(fp);
				return -EINVAL;
			}
			imgbuf = malloc(length);
			if (!imgbuf) {
				fprintf(stderr,
					PROGNAME
					": cannot alloc %d bytes\n",
					length);
				fclose(fp);
				return -ENOMEM;
			}
			continue;
		}
		if ((buf[0] != '0') && (buf[0] != '1')) {
			//printf("line %d skipped\n", line);
			continue;
		}
		if (length <= 0) {
			fprintf(stderr,
				PROGNAME
				": corrupted file %s, starting without Bits line\n",
				fname);
			fclose(fp);
			return -EINVAL;
		}
		//printf("\n idx=%d", idx);
		for (p = buf; *p == '0' || *p == '1'; p++) {
			data |= (*p - '0') << c;
			c++;
			if (c >= 8) {
				//printf(" %02X", data);
				imgbuf[idx++] = data;
				data = c = 0;
				if (idx > length)
					break;
			}
		}
	}
	if (idx != length || 0 == imgbuf) {
		fprintf(stderr, PROGNAME ": length doesn't match: %d != %d\n", idx, length);
		fclose(fp);
		return -EINVAL;
	}
	if (c)
		imgbuf[idx++] = data;
	snd_hwdep_dsp_image_set_length(img, length);
	snd_hwdep_dsp_image_set_image(img, imgbuf);
	fclose(fp);
	return 0;
}


/*
 * read a binary image file
 */
static int read_boot_image(snd_hwdep_dsp_image_t *img, const char *fname)
{
	struct stat st;
	int fd, length;
	unsigned char *imgbuf;

	snd_hwdep_dsp_image_set_name(img, fname);
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
	snd_hwdep_dsp_image_set_length(img, length);
	snd_hwdep_dsp_image_set_image(img, imgbuf);

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
	return 0;
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
	int			err, prepad;
	char			fname[MAX_PATH];
	snd_hwdep_dsp_image_t*	dsp;

	if ((prepad = get_file_name(id, idx, fname)) < 0)
		return -EINVAL;

	snd_hwdep_dsp_image_alloca(&dsp);
	snd_hwdep_dsp_image_set_index(dsp, idx);
	if (strcmp(fname + strlen(fname) - 4, ".rbt"))
		err = read_boot_image(dsp, fname);
	else
		err = read_xilinx_image(dsp, fname);
	if (err < 0)
		return err;

	err = snd_hwdep_dsp_load(hw, dsp);
	if (err < 0)
		error("error in loading %s\n", fname);
	return err;
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

/*
 * load the firmware binaries
 */
static int usx2y_boot(const char *devname)
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

	if (check_hwinfo(hw, SND_USX2Y_LOADER_ID, NULL) < 0) {
		error("invalid hwdep %s\n", devname);
		snd_hwdep_close(hw);
		return -ENODEV;
	}

	snd_hwdep_dsp_status_alloca(&stat);
	/* get the current status */
	if ((err = snd_hwdep_dsp_status(hw, stat)) < 0) {
		error("cannot get version for %s\n", devname);
		snd_hwdep_close(hw);
		return err;
	}

	if (snd_hwdep_dsp_status_get_chip_ready(stat))
		return 0; /* already loaded */

	id = snd_hwdep_dsp_status_get_id(stat);

	dsps = snd_hwdep_dsp_status_get_num_dsps(stat);
	loaded = snd_hwdep_dsp_status_get_dsp_loaded(stat);

	for (idx = 0; idx < dsps; idx++) {
		if (loaded & (1 << idx))
			continue;
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
	char	*device_name = NULL,
		*usb_device_name = getenv("DEVICE");
	char name[64];

	while ((c = getopt(argc, argv, "c:D:u:")) != -1) {
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
		verbose = 1;
		return usx2y_boot(device_name) != 0;
	}
	if (card >= 0) {
		sprintf(name, "hw:%d", card);
		verbose = 1;
		return usx2y_boot(name) != 0;
	}

	/* probe the all cards */
	for (c = 0; c < SND_CARDS; c++) {
		sprintf(name, "hw:%d", c);
		if (! usx2y_boot(name))
			card = c;
	}
	if (card < 0) {
		fprintf(stderr, PROGNAME ": no US-X2Y-compatible cards found\n");
		return 1;
	}
	return 0;
}
