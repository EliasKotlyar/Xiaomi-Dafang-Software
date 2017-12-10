/*
 * Checks the current pin/jack status of the codec
 *
 * Copyright (c) 2014 David Henningsson, Canonical Ltd. <david.henningsson@canonical.com>
 * (With some minor pieces copy-pasted from hda-verb by Takashi Iwai)
 *
 * Licensed under GPL v2 or later.
 */


#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <glib.h>
#include <errno.h>
#include "sysfs-pin-configs.h"

#include <sys/ioctl.h>
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#include "hda_hwdep.h"

gint card_index, codec_index;
gboolean try_all_pins, set_pin_sense;

static GOptionEntry arg_entries[] =
{
  { "card", 'c', 0, G_OPTION_ARG_INT, &card_index, "card index (as can be seen in /proc/asound/cards)", "X" },
  { "codec", 'd', 0, G_OPTION_ARG_INT, &codec_index, "codec device index (as can be seen in /proc/asound/cardX/codecY)", "Y" },
  { "allpins", 'a', 0, G_OPTION_ARG_NONE, &try_all_pins, "try all pins, even those who (probably) does not have a physical jack", NULL },
  { "setpinsense", 's', 0, G_OPTION_ARG_NONE, &set_pin_sense, "execute 'Set pin sense' before the pin sense is measured", NULL },
  { NULL }
};

static void parse_command_line(int argc, char **argv)
{
	GError *error = NULL;
	GOptionContext *context = g_option_context_new("- check current jack/pin sense");
	g_option_context_add_main_entries(context, arg_entries, NULL);
	if (!g_option_context_parse (context, &argc, &argv, &error)) {
		fprintf(stderr, "Option parsing failed: %s\n", error->message);
		exit(1);
	}
	g_option_context_free(context);
}


static gboolean should_check_pin(pin_configs_t *pin)
{
	unsigned long defcfg = actual_pin_config(pin);
	if (try_all_pins)
		return TRUE;
	if (get_port_conn(defcfg) != 0)
		return FALSE; // Not a jack
	if (defcfg & (1 << 8)) // Jack has NO_PRESENCE set
		return FALSE;
	return TRUE;
}

int fd;

static void codec_open()
{
	char filename[64];
	int version = 0;

	snprintf(filename, 64, "/dev/snd/hwC%dD%d", card_index, codec_index);
	fd = open(filename, O_RDWR);
	if (fd < 0) {
		if (errno == EACCES)
			fprintf(stderr, "Permission error (hint: this program usually requires root)\n");
		else
			fprintf(stderr, "Ioctl call failed with error %d\n", errno);
		exit(1);
	}

	if (ioctl(fd, HDA_IOCTL_PVERSION, &version) < 0) {
		fprintf(stderr, "Ioctl call failed with error %d\n", errno);
		fprintf(stderr, "Looks like an invalid hwdep device...\n");
		close(fd);
		exit(1);
	}
	if (version < HDA_HWDEP_VERSION) {
		fprintf(stderr, "Invalid version number 0x%x\n", version);
		fprintf(stderr, "Looks like an invalid hwdep device...\n");
		close(fd);
		exit(1);
	}
}

static unsigned long codec_rw(int nid, int verb, int param)
{
	struct hda_verb_ioctl val;

	val.verb = HDA_VERB(nid, verb, param);
	if (ioctl(fd, HDA_IOCTL_VERB_WRITE, &val) < 0) {
		fprintf(stderr, "Ioctl call failed with error %d\n", errno);
		close(fd);
		exit(1);
	}
	return val.res;
}

#define AC_VERB_GET_PIN_SENSE	0x0f09
#define AC_VERB_SET_PIN_SENSE	0x709

#define MAX_PINS 32

pin_configs_t pin_configs[MAX_PINS];

int main(int argc, char **argv)
{
	int pin_count, i;

	parse_command_line(argc, argv);
	pin_count = get_pin_configs_list(pin_configs, MAX_PINS, card_index, codec_index);
	if (pin_count == 0) {
		fprintf(stderr, "No pins found for card %d codec %d, did you pick the right one?\n", card_index, codec_index);
		exit(1);
	}

	codec_open();

	if (set_pin_sense) {
		for (i = 0; i < pin_count; i++)
			if (should_check_pin(&pin_configs[i])) {
				codec_rw(pin_configs[i].nid, AC_VERB_SET_PIN_SENSE, 0);
			}
		sleep(1);
	}

	for (i = 0; i < pin_count; i++)
		if (should_check_pin(&pin_configs[i])) {
			gchar *desc = get_config_description(actual_pin_config(&pin_configs[i]));
			unsigned long present = codec_rw(pin_configs[i].nid, AC_VERB_GET_PIN_SENSE, 0);
			printf("Pin 0x%.2x (%s): present = %s\n", pin_configs[i].nid, desc, present & 0x80000000 ? "Yes" : "No");
			g_free(desc);
		}

	close(fd);
	return 0;
}
