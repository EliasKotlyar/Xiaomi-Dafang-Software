/*
 * Accessing HD-audio verbs via hwdep interface
 * Version 0.3
 *
 * Copyright (c) 2008 Takashi Iwai <tiwai@suse.de>
 *
 * Licensed under GPL v2 or later.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/fcntl.h>

#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#include "hda_hwdep.h"

#define AC_VERB_GET_STREAM_FORMAT		0x0a00
#define AC_VERB_GET_AMP_GAIN_MUTE		0x0b00
#define AC_VERB_GET_PROC_COEF			0x0c00
#define AC_VERB_GET_COEF_INDEX			0x0d00
#define AC_VERB_PARAMETERS			0x0f00
#define AC_VERB_GET_CONNECT_SEL			0x0f01
#define AC_VERB_GET_CONNECT_LIST		0x0f02
#define AC_VERB_GET_PROC_STATE			0x0f03
#define AC_VERB_GET_SDI_SELECT			0x0f04
#define AC_VERB_GET_POWER_STATE			0x0f05
#define AC_VERB_GET_CONV			0x0f06
#define AC_VERB_GET_PIN_WIDGET_CONTROL		0x0f07
#define AC_VERB_GET_UNSOLICITED_RESPONSE	0x0f08
#define AC_VERB_GET_PIN_SENSE			0x0f09
#define AC_VERB_GET_BEEP_CONTROL		0x0f0a
#define AC_VERB_GET_EAPD_BTLENABLE		0x0f0c
#define AC_VERB_GET_DIGI_CONVERT_1		0x0f0d
#define AC_VERB_GET_DIGI_CONVERT_2		0x0f0e
#define AC_VERB_GET_VOLUME_KNOB_CONTROL		0x0f0f
#define AC_VERB_GET_GPIO_DATA			0x0f15
#define AC_VERB_GET_GPIO_MASK			0x0f16
#define AC_VERB_GET_GPIO_DIRECTION		0x0f17
#define AC_VERB_GET_GPIO_WAKE_MASK		0x0f18
#define AC_VERB_GET_GPIO_UNSOLICITED_RSP_MASK	0x0f19
#define AC_VERB_GET_GPIO_STICKY_MASK		0x0f1a
#define AC_VERB_GET_CONFIG_DEFAULT		0x0f1c
#define AC_VERB_GET_SUBSYSTEM_ID		0x0f20

#define AC_VERB_SET_STREAM_FORMAT		0x200
#define AC_VERB_SET_AMP_GAIN_MUTE		0x300
#define AC_VERB_SET_PROC_COEF			0x400
#define AC_VERB_SET_COEF_INDEX			0x500
#define AC_VERB_SET_CONNECT_SEL			0x701
#define AC_VERB_SET_PROC_STATE			0x703
#define AC_VERB_SET_SDI_SELECT			0x704
#define AC_VERB_SET_POWER_STATE			0x705
#define AC_VERB_SET_CHANNEL_STREAMID		0x706
#define AC_VERB_SET_PIN_WIDGET_CONTROL		0x707
#define AC_VERB_SET_UNSOLICITED_ENABLE		0x708
#define AC_VERB_SET_PIN_SENSE			0x709
#define AC_VERB_SET_BEEP_CONTROL		0x70a
#define AC_VERB_SET_EAPD_BTLENABLE		0x70c
#define AC_VERB_SET_DIGI_CONVERT_1		0x70d
#define AC_VERB_SET_DIGI_CONVERT_2		0x70e
#define AC_VERB_SET_VOLUME_KNOB_CONTROL		0x70f
#define AC_VERB_SET_GPIO_DATA			0x715
#define AC_VERB_SET_GPIO_MASK			0x716
#define AC_VERB_SET_GPIO_DIRECTION		0x717
#define AC_VERB_SET_GPIO_WAKE_MASK		0x718
#define AC_VERB_SET_GPIO_UNSOLICITED_RSP_MASK	0x719
#define AC_VERB_SET_GPIO_STICKY_MASK		0x71a
#define AC_VERB_SET_CONFIG_DEFAULT_BYTES_0	0x71c
#define AC_VERB_SET_CONFIG_DEFAULT_BYTES_1	0x71d
#define AC_VERB_SET_CONFIG_DEFAULT_BYTES_2	0x71e
#define AC_VERB_SET_CONFIG_DEFAULT_BYTES_3	0x71f
#define AC_VERB_SET_CODEC_RESET			0x7ff

#define AC_PAR_VENDOR_ID		0x00
#define AC_PAR_SUBSYSTEM_ID		0x01
#define AC_PAR_REV_ID			0x02
#define AC_PAR_NODE_COUNT		0x04
#define AC_PAR_FUNCTION_TYPE		0x05
#define AC_PAR_AUDIO_FG_CAP		0x08
#define AC_PAR_AUDIO_WIDGET_CAP		0x09
#define AC_PAR_PCM			0x0a
#define AC_PAR_STREAM			0x0b
#define AC_PAR_PIN_CAP			0x0c
#define AC_PAR_AMP_IN_CAP		0x0d
#define AC_PAR_CONNLIST_LEN		0x0e
#define AC_PAR_POWER_STATE		0x0f
#define AC_PAR_PROC_CAP			0x10
#define AC_PAR_GPIO_CAP			0x11
#define AC_PAR_AMP_OUT_CAP		0x12
#define AC_PAR_VOL_KNB_CAP		0x13

/*
 */
#define VERBSTR(x)	{ .val = AC_VERB_##x, .str = #x }
#define PARMSTR(x)	{ .val = AC_PAR_##x, .str = #x }

struct strtbl {
	int val;
	const char *str;
};

static struct strtbl hda_verbs[] = {
	VERBSTR(GET_STREAM_FORMAT),
	VERBSTR(GET_AMP_GAIN_MUTE),
	VERBSTR(GET_PROC_COEF),
	VERBSTR(GET_COEF_INDEX),
	VERBSTR(PARAMETERS),
	VERBSTR(GET_CONNECT_SEL),
	VERBSTR(GET_CONNECT_LIST),
	VERBSTR(GET_PROC_STATE),
	VERBSTR(GET_SDI_SELECT),
	VERBSTR(GET_POWER_STATE),
	VERBSTR(GET_CONV),
	VERBSTR(GET_PIN_WIDGET_CONTROL),
	VERBSTR(GET_UNSOLICITED_RESPONSE),
	VERBSTR(GET_PIN_SENSE),
	VERBSTR(GET_BEEP_CONTROL),
	VERBSTR(GET_EAPD_BTLENABLE),
	VERBSTR(GET_DIGI_CONVERT_1),
	VERBSTR(GET_DIGI_CONVERT_2),
	VERBSTR(GET_VOLUME_KNOB_CONTROL),
	VERBSTR(GET_GPIO_DATA),
	VERBSTR(GET_GPIO_MASK),
	VERBSTR(GET_GPIO_DIRECTION),
	VERBSTR(GET_GPIO_WAKE_MASK),
	VERBSTR(GET_GPIO_UNSOLICITED_RSP_MASK),
	VERBSTR(GET_GPIO_STICKY_MASK),
	VERBSTR(GET_CONFIG_DEFAULT),
	VERBSTR(GET_SUBSYSTEM_ID),

	VERBSTR(SET_STREAM_FORMAT),
	VERBSTR(SET_AMP_GAIN_MUTE),
	VERBSTR(SET_PROC_COEF),
	VERBSTR(SET_COEF_INDEX),
	VERBSTR(SET_CONNECT_SEL),
	VERBSTR(SET_PROC_STATE),
	VERBSTR(SET_SDI_SELECT),
	VERBSTR(SET_POWER_STATE),
	VERBSTR(SET_CHANNEL_STREAMID),
	VERBSTR(SET_PIN_WIDGET_CONTROL),
	VERBSTR(SET_UNSOLICITED_ENABLE),
	VERBSTR(SET_PIN_SENSE),
	VERBSTR(SET_BEEP_CONTROL),
	VERBSTR(SET_EAPD_BTLENABLE),
	VERBSTR(SET_DIGI_CONVERT_1),
	VERBSTR(SET_DIGI_CONVERT_2),
	VERBSTR(SET_VOLUME_KNOB_CONTROL),
	VERBSTR(SET_GPIO_DATA),
	VERBSTR(SET_GPIO_MASK),
	VERBSTR(SET_GPIO_DIRECTION),
	VERBSTR(SET_GPIO_WAKE_MASK),
	VERBSTR(SET_GPIO_UNSOLICITED_RSP_MASK),
	VERBSTR(SET_GPIO_STICKY_MASK),
	VERBSTR(SET_CONFIG_DEFAULT_BYTES_0),
	VERBSTR(SET_CONFIG_DEFAULT_BYTES_1),
	VERBSTR(SET_CONFIG_DEFAULT_BYTES_2),
	VERBSTR(SET_CONFIG_DEFAULT_BYTES_3),
	VERBSTR(SET_CODEC_RESET),
	{ }, /* end */
};

static struct strtbl hda_params[] = {
	PARMSTR(VENDOR_ID),
	PARMSTR(SUBSYSTEM_ID),
	PARMSTR(REV_ID),
	PARMSTR(NODE_COUNT),
	PARMSTR(FUNCTION_TYPE),
	PARMSTR(AUDIO_FG_CAP),
	PARMSTR(AUDIO_WIDGET_CAP),
	PARMSTR(PCM),
	PARMSTR(STREAM),
	PARMSTR(PIN_CAP),
	PARMSTR(AMP_IN_CAP),
	PARMSTR(CONNLIST_LEN),
	PARMSTR(POWER_STATE),
	PARMSTR(PROC_CAP),
	PARMSTR(GPIO_CAP),
	PARMSTR(AMP_OUT_CAP),
	PARMSTR(VOL_KNB_CAP),
	{ }, /* end */
};

static void list_keys(struct strtbl *tbl, int one_per_line)
{
	int c = 0;
	for (; tbl->str; tbl++) {
		int len = strlen(tbl->str) + 2;
		if (!one_per_line && c + len >= 80) {
			fprintf(stderr, "\n");
			c = 0;
		}
		if (one_per_line)
			fprintf(stderr, "  %s\n", tbl->str);
		else if (!c)
			fprintf(stderr, "  %s", tbl->str);
		else
			fprintf(stderr, ", %s", tbl->str);
		c += 2 + len;
	}
	if (!one_per_line)
		fprintf(stderr, "\n");
}

/* look up a value from the given string table */
static int lookup_str(struct strtbl *tbl, const char *str)
{
	struct strtbl *p, *found;
	int len = strlen(str);

	found = NULL;
	for (p = tbl; p->str; p++) {
		if (!strncmp(str, p->str, len)) {
			if (found) {
				fprintf(stderr, "No unique key '%s'\n", str);
				return -1;
			}
			found = p;
		}
	}
	if (!found) {
		fprintf(stderr, "No key matching with '%s'\n", str);
		return -1;
	}
	return found->val;
}

/* convert a string to upper letters */
static void strtoupper(char *str)
{
	for (; *str; str++)
		*str = toupper(*str);
}

static void usage(void)
{
	fprintf(stderr, "usage: hda-verb [option] hwdep-device nid verb param\n");
	fprintf(stderr, "   -l      List known verbs and parameters\n");
	fprintf(stderr, "   -L      List known verbs and parameters (one per line)\n");
}

static void list_verbs(int one_per_line)
{
	fprintf(stderr, "known verbs:\n");
	list_keys(hda_verbs, one_per_line);
	fprintf(stderr, "known parameters:\n");
	list_keys(hda_params, one_per_line);
}

int main(int argc, char **argv)
{
	int version;
	int fd;
	int nid, verb, param;
	int c;
	struct hda_verb_ioctl val;
	char **p;

	while ((c = getopt(argc, argv, "lL")) >= 0) {
		switch (c) {
		case 'l':
			list_verbs(0);
			return 0;
		case 'L':
			list_verbs(1);
			return 0;
		default:
			usage();
			return 1;
		}
	}

	if (argc - optind < 4) {
		usage();
		return 1;
	}
	p = argv + optind;
	fd = open(*p, O_RDWR);
	if (fd < 0) {
		perror("open");
		return 1;
	}
	version = 0;
	if (ioctl(fd, HDA_IOCTL_PVERSION, &version) < 0) {
		perror("ioctl(PVERSION)");
		fprintf(stderr, "Looks like an invalid hwdep device...\n");
		return 1;
	}
	if (version < HDA_HWDEP_VERSION) {
		fprintf(stderr, "Invalid version number 0x%x\n", version);
		fprintf(stderr, "Looks like an invalid hwdep device...\n");
		return 1;
	}

	p++;
	nid = strtol(*p, NULL, 0);
	if (nid < 0 || nid > 0xff) {
		fprintf(stderr, "invalid nid %s\n", *p);
		return 1;
	}

	p++;
	if (!isdigit(**p)) {
		strtoupper(*p);
		verb = lookup_str(hda_verbs, *p);
		if (verb < 0)
			return 1;
	} else {
		verb = strtol(*p, NULL, 0);
		if (verb < 0 || verb > 0xfff) {
			fprintf(stderr, "invalid verb %s\n", *p);
			return 1;
		}
	}
	p++;
	if (!isdigit(**p)) {
		strtoupper(*p);
		param = lookup_str(hda_params, *p);
		if (param < 0)
			return 1;
	} else {
		param = strtol(*p, NULL, 0);
		if (param < 0 || param > 0xffff) {
			fprintf(stderr, "invalid param %s\n", *p);
			return 1;
		}
	}
	fprintf(stderr, "nid = 0x%x, verb = 0x%x, param = 0x%x\n",
		nid, verb, param);

	val.verb = HDA_VERB(nid, verb, param);
	if (ioctl(fd, HDA_IOCTL_VERB_WRITE, &val) < 0)
		perror("ioctl");
	printf("value = 0x%x\n", val.res);
	close(fd);
	return 0;
}
