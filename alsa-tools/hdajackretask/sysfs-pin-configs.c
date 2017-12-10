/* Copyright 2011 David Henningsson, Canonical Ltd.
   License: GPLv2+ 
*/

#include <stdlib.h>
#include <string.h>
#include "sysfs-pin-configs.h"
#include "apply-changes.h"

const gchar *hint_names[25] = {
"jack_detect",  "inv_jack_detect", "trigger_sense", "inv_eapd",
"pcm_format_first", "sticky_stream", "spdif_status_reset",
"pin_amp_workaround", "single_adc_amp", "auto_mute", "auto_mic",
"line_in_auto_switch", "auto_mute_via_amp", "need_dac_fix", "primary_hp",
"multi_io", "multi_cap_vol", "inv_dmic_split", "indep_hp",
"add_stereo_mix_input", "add_jack_modes", "power_down_unused", "add_hp_mic",
"hp_mic_detect", NULL };

const gchar** get_standard_hint_names()
{
    return hint_names;
}


int get_codec_name_list(codec_name_t* names, int entries)
{
    GDir* sysdir = g_dir_open("/sys/class/sound", 0, NULL);
    int count = 0;
    if (!sysdir)
        return 0;
  
    while (entries > 1) {
        gchar** cd = NULL;
        gboolean ok;
        const gchar* dirname = g_dir_read_name(sysdir);
        if (!dirname)
            break;
        /* Split e g "hwC0D1" into "hw", "0" and "1" */
        cd = g_strsplit_set(dirname, "CD", 9);
        ok = g_strv_length(cd) == 3;
        ok &= strcmp(cd[0], "hw") == 0;
        if (ok) {
            gchar* filetest = g_strdup_printf("/sys/class/sound/%s/init_pin_configs", dirname);
            ok = g_file_test(filetest, G_FILE_TEST_IS_REGULAR);
            g_free(filetest);
        }

        if (ok) {
            gchar* chip_name = NULL, *vendor_name = NULL;
            gchar* chip_file = g_strdup_printf("/sys/class/sound/%s/chip_name", dirname);
            gchar* vendor_file = g_strdup_printf("/sys/class/sound/%s/vendor_name", dirname);
            ok = g_file_get_contents(chip_file, &chip_name, NULL, NULL);
            ok &= g_file_get_contents(vendor_file, &vendor_name, NULL, NULL);
            if (ok) {
                names->name = g_strdup_printf("%s %s", g_strchomp(vendor_name), g_strchomp(chip_name));
            }
            g_free(chip_name);
            g_free(vendor_name);
            g_free(chip_file);
            g_free(vendor_file);
        }

        if (ok) {
            names->card = atoi(cd[1]);
            names->device = atoi(cd[2]); 

            names++; 
            count++;
            entries--;
        }
        g_strfreev(cd);
    }
    if (entries) {
        names->name = NULL;
        names->card = -1;
    }
    g_dir_close(sysdir);
    return count;
}

static unsigned long read_header_value(gchar* contents, gchar* key)
{
    gchar* s = strstr(contents, key);
    s += strlen(key);
    return g_ascii_strtoull(s, NULL, 0);
}

void get_codec_header(int card, int device, unsigned int* address, 
    unsigned int* codec_vendorid, unsigned int* codec_ssid)
{
    gchar* filename = g_strdup_printf("/proc/asound/card%d/codec#%d", card, device);
    gchar* contents = NULL;
    int ok = g_file_get_contents(filename, &contents, NULL, NULL);
    g_free(filename);
    if (!ok)
        return;
    *address = read_header_value(contents, "Address: ");
    *codec_vendorid = read_header_value(contents, "Vendor Id: ");
    *codec_ssid = read_header_value(contents, "Subsystem Id: ");

    g_free(contents);
}

static void get_pin_caps(pin_configs_t* pins, int entries, int card, int device)
{
    gchar* filename = g_strdup_printf("/proc/asound/card%d/codec#%d", card, device);
    gchar* contents = NULL;
    int ok = g_file_get_contents(filename, &contents, NULL, NULL);
    g_free(filename);
    if (!ok) 
        return;

    for (; entries; entries--, pins++) {
        gchar* nodestr = g_strdup_printf("Node 0x%02x [", pins->nid);
        gchar* q = strstr(contents, nodestr);
        g_free(nodestr);
        if (!q) 
            continue;

        q = strstr(q, "wcaps 0x");
        if (!q) 
            continue;
        q += strlen("wcaps ");
        pins->wid_caps = g_ascii_strtoull(q, NULL, 0);

        q = strstr(q, "Pincap 0x");
        if (!q) 
            continue;
        q += strlen("Pincap ");
        pins->pin_caps = g_ascii_strtoull(q, NULL, 0);
    }
    g_free(contents);
}

gchar *get_hint_overrides(int card, int device)
{
    gchar* filename = g_strdup_printf("/sys/class/sound/hwC%dD%d/hints", card, device);
    gchar* contents = NULL;
    int ok = g_file_get_contents(filename, &contents, NULL, NULL);
    g_free(filename);
    if (!ok)
        return NULL;
    return contents;
}

static void read_pin_overrides(pin_configs_t* pins, int entries, int card, int device, gboolean is_user)
{
    gchar* filename = g_strdup_printf("/sys/class/sound/hwC%dD%d/%s_pin_configs", card, device, is_user ? "user" : "driver");
    gchar* contents = NULL;
    gchar** lines = NULL, **line_iterator;
    int count = 0;
    int ok = g_file_get_contents(filename, &contents, NULL, NULL);
    g_free(filename);
    if (!ok)
        return;
    line_iterator = lines = g_strsplit(contents, "\n", entries);
    while (count < entries && *line_iterator) {
        gchar** line = g_strsplit(*line_iterator, " ", 0);
        line_iterator++;
        if (g_strv_length(line) == 2) {
            int nid = g_ascii_strtoull(line[0], NULL, 0) & 0xff;
            unsigned long config = g_ascii_strtoull(line[1], NULL, 0);
            int i;
            for (i=0; i < entries; i++)
                if (nid == pins[i].nid) {
                    if (is_user) {
                        pins[i].user_override = FALSE;
                        pins[i].user_override = config != actual_pin_config(&pins[i]);
                        pins[i].user_pin_config = config;
                    } else {
                        pins[i].driver_override = config != pins[i].init_pin_config;
                        pins[i].driver_pin_config = config;
                    }
                }
        }
        g_strfreev(line); 
    }    
    g_strfreev(lines);
    g_free(contents);
}

int get_pin_configs_list(pin_configs_t* pins, int entries, int card, int device)
{
    gchar* filename = g_strdup_printf("/sys/class/sound/hwC%dD%d/init_pin_configs", card, device);
    gchar* contents = NULL;
    gchar** lines = NULL, **line_iterator;
    int count = 0;
    int ok = g_file_get_contents(filename, &contents, NULL, NULL);
    g_free(filename);
    if (!ok) 
        return 0;
    line_iterator = lines = g_strsplit(contents, "\n", entries);
    g_free(contents);
    while (count < entries && *line_iterator) {
        gchar** line = g_strsplit(*line_iterator, " ", 0);
        line_iterator++;
        if (g_strv_length(line) == 2) {
            pins[count].nid = g_ascii_strtoull(line[0], NULL, 0);
            pins[count].init_pin_config = g_ascii_strtoull(line[1], NULL, 0) & 0xffffffff;
            pins[count].driver_override = FALSE;
            pins[count].user_override = FALSE;
            count++;
        }
        g_strfreev(line); 
    }    
    g_strfreev(lines);

    read_pin_overrides(pins, count, card, device, FALSE);
    read_pin_overrides(pins, count, card, device, TRUE);
    get_pin_caps(pins, count, card, device);

    return count;
}

unsigned long actual_pin_config(pin_configs_t* pins)
{
    if (pins->user_override)
        return pins->user_pin_config;
    if (pins->driver_override)
        return pins->driver_pin_config;
    return pins->init_pin_config;
}

/*** Code below taken from sound/pci/hda/hda_proc.c and hda_codec.h, (C) Takashi Iwai, GPLv2+. ****/

#define u32 unsigned long

#define AC_WCAP_STEREO                  (1<<0)  /* stereo I/O */
#define AC_WCAP_IN_AMP                  (1<<1)  /* AMP-in present */
#define AC_WCAP_OUT_AMP                 (1<<2)  /* AMP-out present */
#define AC_WCAP_AMP_OVRD                (1<<3)  /* AMP-parameter override */
#define AC_WCAP_FORMAT_OVRD             (1<<4)  /* format override */
#define AC_WCAP_STRIPE                  (1<<5)  /* stripe */
#define AC_WCAP_PROC_WID                (1<<6)  /* Proc Widget */
#define AC_WCAP_UNSOL_CAP               (1<<7)  /* Unsol capable */
#define AC_WCAP_CONN_LIST               (1<<8)  /* connection list */
#define AC_WCAP_DIGITAL                 (1<<9)  /* digital I/O */
#define AC_WCAP_POWER                   (1<<10) /* power control */
#define AC_WCAP_LR_SWAP                 (1<<11) /* L/R swap */
#define AC_WCAP_CP_CAPS                 (1<<12) /* content protection */
#define AC_WCAP_CHAN_CNT_EXT            (7<<13) /* channel count ext */
#define AC_WCAP_DELAY                   (0xf<<16)
#define AC_WCAP_DELAY_SHIFT             16
#define AC_WCAP_TYPE                    (0xf<<20)
#define AC_WCAP_TYPE_SHIFT              20

#define AC_PINCAP_IMP_SENSE		(1<<0)	/* impedance sense capable */
#define AC_PINCAP_TRIG_REQ		(1<<1)	/* trigger required */
#define AC_PINCAP_PRES_DETECT		(1<<2)	/* presence detect capable */
#define AC_PINCAP_HP_DRV		(1<<3)	/* headphone drive capable */
#define AC_PINCAP_OUT			(1<<4)	/* output capable */
#define AC_PINCAP_IN			(1<<5)	/* input capable */
#define AC_PINCAP_BALANCE		(1<<6)	/* balanced I/O capable */
/* Note: This LR_SWAP pincap is defined in the Realtek ALC883 specification,
 *       but is marked reserved in the Intel HDA specification.
 */
#define AC_PINCAP_LR_SWAP		(1<<7)	/* L/R swap */
/* Note: The same bit as LR_SWAP is newly defined as HDMI capability
 *       in HD-audio specification
 */
#define AC_PINCAP_HDMI			(1<<7)	/* HDMI pin */
#define AC_PINCAP_DP			(1<<24)	/* DisplayPort pin, can
						 * coexist with AC_PINCAP_HDMI
						 */
#define AC_PINCAP_VREF			(0x37<<8)
#define AC_PINCAP_VREF_SHIFT		8
#define AC_PINCAP_EAPD			(1<<16)	/* EAPD capable */
#define AC_PINCAP_HBR			(1<<27)	/* High Bit Rate */
/* Vref status (used in pin cap) */
#define AC_PINCAP_VREF_HIZ		(1<<0)	/* Hi-Z */
#define AC_PINCAP_VREF_50		(1<<1)	/* 50% */
#define AC_PINCAP_VREF_GRD		(1<<2)	/* ground */
#define AC_PINCAP_VREF_80		(1<<4)	/* 80% */
#define AC_PINCAP_VREF_100		(1<<5)	/* 100% */


/* configuration default - 32bit */
#define AC_DEFCFG_SEQUENCE		(0xf<<0)
#define AC_DEFCFG_DEF_ASSOC		(0xf<<4)
#define AC_DEFCFG_ASSOC_SHIFT		4
#define AC_DEFCFG_MISC			(0xf<<8)
#define AC_DEFCFG_MISC_SHIFT		8
#define AC_DEFCFG_MISC_NO_PRESENCE	(1<<0)
#define AC_DEFCFG_COLOR			(0xf<<12)
#define AC_DEFCFG_COLOR_SHIFT		12
#define AC_DEFCFG_CONN_TYPE		(0xf<<16)
#define AC_DEFCFG_CONN_TYPE_SHIFT	16
#define AC_DEFCFG_DEVICE		(0xf<<20)
#define AC_DEFCFG_DEVICE_SHIFT		20
#define AC_DEFCFG_LOCATION		(0x3f<<24)
#define AC_DEFCFG_LOCATION_SHIFT	24
#define AC_DEFCFG_GROSSLOC		(0x3<<28)
#define AC_DEFCFG_GROSSLOC_SHIFT	28
#define AC_DEFCFG_PORT_CONN		(0x3<<30)
#define AC_DEFCFG_PORT_CONN_SHIFT	30

static const char *get_jack_color(u32 cfg)
{
	static char *names[16] = {
		"", "Black", "Grey", "Blue",
		"Green", "Red", "Orange", "Yellow",
		"Purple", "Pink", NULL, NULL,
		NULL, NULL, "White", "Other",
	};
	cfg = (cfg & AC_DEFCFG_COLOR) >> AC_DEFCFG_COLOR_SHIFT;
	if (names[cfg])
		return names[cfg];
	else
		return "UNKNOWN";
}

static const char *get_jack_type(u32 cfg)
{
	static char *jack_types[16] = {
		"Line Out", "Speaker", "Headphone", "CD",
		"SPDIF Out", "Digital Out", "Modem Line", "Modem Hand",
		"Line In", "Aux", "Mic", "Telephony",
		"SPDIF In", "Digital In", "Reserved", "Other"
	};

	return jack_types[(cfg & AC_DEFCFG_DEVICE)
				>> AC_DEFCFG_DEVICE_SHIFT];
}

static const char *get_jack_location(u32 cfg)
{
	static char *bases[7] = {
		"", ", Rear side", ", Front side", ", Left side", ", Right side", ", Top", ", Bottom",
	};
	static unsigned char specials_idx[] = {
		0x07, 0x08,
		0x17, 0x18, 0x19,
		0x37, 0x38
	};
	static char *specials[] = {
		", Rear Panel", ", Drive Bar",
		", Riser", ", HDMI", ", ATAPI",
		", Mobile-In", ", Mobile-Out"
	};
	int i;
	cfg = (cfg & AC_DEFCFG_LOCATION) >> AC_DEFCFG_LOCATION_SHIFT;
	if ((cfg & 0x0f) < 7)
		return bases[cfg & 0x0f];
	for (i = 0; i < 7; i++) {
		if (cfg == specials_idx[i])
			return specials[i];
	}
	return "UNKNOWN";
}


/**** Borrowed code end *****/

static free_override_t pc_arr[] = {
    {"Not connected", 1 << AC_DEFCFG_PORT_CONN_SHIFT},
    {"Jack", 0},
    {"Internal", 2 << AC_DEFCFG_PORT_CONN_SHIFT},
    {"Both", 3 << AC_DEFCFG_PORT_CONN_SHIFT},
    {}
};

static free_override_t location_arr[] = {
    {"External", 0},
    {"Rear", 1 << AC_DEFCFG_LOCATION_SHIFT},
    {"Front", 2 << AC_DEFCFG_LOCATION_SHIFT},
    {"Left", 3 << AC_DEFCFG_LOCATION_SHIFT},
    {"Right", 4 << AC_DEFCFG_LOCATION_SHIFT},
    {"Top", 5 << AC_DEFCFG_LOCATION_SHIFT},
    {"Bottom", 6 << AC_DEFCFG_LOCATION_SHIFT},
    {"Rear panel", 7 << AC_DEFCFG_LOCATION_SHIFT},
    {"Drive bay", 8 << AC_DEFCFG_LOCATION_SHIFT},

    {"Internal", 16 << AC_DEFCFG_LOCATION_SHIFT},
    {"Internal riser", (16+7) << AC_DEFCFG_LOCATION_SHIFT},
    {"Internal display", (16+8) << AC_DEFCFG_LOCATION_SHIFT},
    {"Internal ATAPI", (16+9) << AC_DEFCFG_LOCATION_SHIFT},

    {"Dock", 32 << AC_DEFCFG_LOCATION_SHIFT},
    {"Dock Rear", 33 << AC_DEFCFG_LOCATION_SHIFT},
    {"Dock Front", 34 << AC_DEFCFG_LOCATION_SHIFT},
    {"Dock Left", 35 << AC_DEFCFG_LOCATION_SHIFT},
    {"Dock Right", 36 << AC_DEFCFG_LOCATION_SHIFT},
    {"Dock Top", 37 << AC_DEFCFG_LOCATION_SHIFT},
    {"Dock Bottom", 38 << AC_DEFCFG_LOCATION_SHIFT},

    {"Other", 48 << AC_DEFCFG_LOCATION_SHIFT},
    {"Other bottom", (48+7) << AC_DEFCFG_LOCATION_SHIFT},
    {"Inside mobile lid", (48+8) << AC_DEFCFG_LOCATION_SHIFT},
    {"Outside mobile lid", (48+9) << AC_DEFCFG_LOCATION_SHIFT},

    {}
};

static free_override_t device_arr[] = {
    {"Line Out", 0},
    {"Speaker", 1 << AC_DEFCFG_DEVICE_SHIFT},
    {"Headphone", 2 << AC_DEFCFG_DEVICE_SHIFT},
    {"CD", 3 << AC_DEFCFG_DEVICE_SHIFT},
    {"SPDIF Out", 4 << AC_DEFCFG_DEVICE_SHIFT},
    {"Digital Out", 5 << AC_DEFCFG_DEVICE_SHIFT},
    {"Modem (Line side)", 6 << AC_DEFCFG_DEVICE_SHIFT},
    {"Modem (Handset side)", 7 << AC_DEFCFG_DEVICE_SHIFT},
    {"Line In", 8 << AC_DEFCFG_DEVICE_SHIFT},
    {"Aux", 9 << AC_DEFCFG_DEVICE_SHIFT},
    {"Microphone", 10 << AC_DEFCFG_DEVICE_SHIFT},
    {"Telephony", 11 << AC_DEFCFG_DEVICE_SHIFT},
    {"SPDIF In", 12 << AC_DEFCFG_DEVICE_SHIFT},
    {"Other Digital In", 13 << AC_DEFCFG_DEVICE_SHIFT},
    {"Other", 15 << AC_DEFCFG_DEVICE_SHIFT},
    {}
};

static free_override_t jack_arr[] = {
    {"Unknown", 0},
    {"3.5 mm", 1 << AC_DEFCFG_CONN_TYPE_SHIFT},
    {"6.3 mm", 2 << AC_DEFCFG_CONN_TYPE_SHIFT},
    {"ATAPI", 3 << AC_DEFCFG_CONN_TYPE_SHIFT},
    {"RCA", 4 << AC_DEFCFG_CONN_TYPE_SHIFT},
    {"Optical", 5 << AC_DEFCFG_CONN_TYPE_SHIFT},
    {"Other Digital", 6 << AC_DEFCFG_CONN_TYPE_SHIFT},
    {"Other Analog", 7 << AC_DEFCFG_CONN_TYPE_SHIFT},
    {"DIN", 8 << AC_DEFCFG_CONN_TYPE_SHIFT},
    {"XLR", 9 << AC_DEFCFG_CONN_TYPE_SHIFT},
    {"RJ-11 (Modem)", 10 << AC_DEFCFG_CONN_TYPE_SHIFT},
    {"Combination", 11 << AC_DEFCFG_CONN_TYPE_SHIFT},
    {"Other", 15 << AC_DEFCFG_CONN_TYPE_SHIFT},
    {}
};

static free_override_t color_arr[] = {
    {"Unknown", 0},
    {"Black", 1 << AC_DEFCFG_COLOR_SHIFT},
    {"Grey", 2 << AC_DEFCFG_COLOR_SHIFT},
    {"Blue", 3 << AC_DEFCFG_COLOR_SHIFT},
    {"Green", 4 << AC_DEFCFG_COLOR_SHIFT},
    {"Red", 5 << AC_DEFCFG_COLOR_SHIFT},
    {"Orange", 6 << AC_DEFCFG_COLOR_SHIFT},
    {"Yellow", 7 << AC_DEFCFG_COLOR_SHIFT},
    {"Purple", 8 << AC_DEFCFG_COLOR_SHIFT},
    {"Pink", 9 << AC_DEFCFG_COLOR_SHIFT},
    {"White", 14 << AC_DEFCFG_COLOR_SHIFT},
    {"Other", 15 << AC_DEFCFG_COLOR_SHIFT},
    {}
};

static free_override_t no_presence_arr[] = {
    {"Present", 0},
    {"Not present", 1 << AC_DEFCFG_MISC_SHIFT},
    {}
};

static free_override_t group_nr_arr[] = {
    {"1", 1 << AC_DEFCFG_ASSOC_SHIFT},
    {"2", 2 << AC_DEFCFG_ASSOC_SHIFT},
    {"3", 3 << AC_DEFCFG_ASSOC_SHIFT},
    {"4", 4 << AC_DEFCFG_ASSOC_SHIFT},
    {"5", 5 << AC_DEFCFG_ASSOC_SHIFT},
    {"6", 6 << AC_DEFCFG_ASSOC_SHIFT},
    {"7", 7 << AC_DEFCFG_ASSOC_SHIFT},
    {"8", 8 << AC_DEFCFG_ASSOC_SHIFT},
    {"9", 9 << AC_DEFCFG_ASSOC_SHIFT},
    {"10", 10 << AC_DEFCFG_ASSOC_SHIFT},
    {"11", 11 << AC_DEFCFG_ASSOC_SHIFT},
    {"12", 12 << AC_DEFCFG_ASSOC_SHIFT},
    {"13", 13 << AC_DEFCFG_ASSOC_SHIFT},
    {"14", 14 << AC_DEFCFG_ASSOC_SHIFT},
    {"15", 15 << AC_DEFCFG_ASSOC_SHIFT},
    {}
};

static free_override_t channel_arr[] = {
    {"Front", 0},
    {"Center/LFE", 1},
    {"Back", 2},
    {"Side", 3},
    {"Channel 8 & 9", 4},
    {"Channel 10 & 11", 5},
    {"Channel 12 & 13", 6},
    {"Channel 14 & 15", 7},
    {"Channel 16 & 17", 8},
    {"Channel 18 & 19", 9},
    {"Channel 20 & 21", 10},
    {"Channel 22 & 23", 11},
    {"Channel 24 & 25", 12},
    {"Channel 26 & 27", 13},
    {"Channel 28 & 29", 14},
    {"Channel 30 & 31", 15},
    {}
};


static free_override_t* type_order[FREE_OVERRIDES_COUNT] = {
    pc_arr, location_arr, device_arr, jack_arr,
    color_arr, no_presence_arr, group_nr_arr, channel_arr
};

unsigned long get_free_override_mask(int type)
{
    static unsigned long masks[FREE_OVERRIDES_COUNT] = {
        AC_DEFCFG_PORT_CONN,
        AC_DEFCFG_LOCATION,
        AC_DEFCFG_DEVICE,
        AC_DEFCFG_CONN_TYPE,
        AC_DEFCFG_COLOR,
        AC_DEFCFG_MISC & (AC_DEFCFG_MISC_NO_PRESENCE << AC_DEFCFG_MISC_SHIFT),
        AC_DEFCFG_DEF_ASSOC,
        AC_DEFCFG_SEQUENCE,
    };

    return masks[type];
}

free_override_t* get_free_override_list(int type)
{
    return type_order[type];
}


int get_port_conn(unsigned long config)
{
    return (config & AC_DEFCFG_PORT_CONN) >> AC_DEFCFG_PORT_CONN_SHIFT;
}

gchar* get_config_description(unsigned long config)
{
    int port_conn = get_port_conn(config);
    if (port_conn == 1)
        return g_strdup("Not connected");
    return g_strdup_printf("%s %s%s%s", port_conn == 2 ? "Internal" : get_jack_color(config), 
        get_jack_type(config), 
        ((config >> (AC_DEFCFG_LOCATION_SHIFT+4)) & 3) == 2 ? ", Docking station" : "",
        get_jack_location(config));
}

/*
gchar* get_caps_description(unsigned long pin_caps)
{
    int vref = (pin_caps & AC_PINCAP_VREF) >> AC_PINCAP_VREF_SHIFT;
    gboolean linein = (pin_caps & AC_PINCAP_IN) && (vref & AC_PINCAP_VREF_HIZ);
    gboolean lineout = pin_caps & AC_PINCAP_OUT;
    gboolean hp = pin_caps & AC_PINCAP_HP_DRV;
    gboolean mic = (pin_caps & AC_PINCAP_IN) && (vref & AC_PINCAP_VREF_50 || vref & AC_PINCAP_VREF_80);
    return g_strjoin("", 
            lineout ? ", Line out": "", 
            hp ? ", Headphone": "", 
            linein ? ", Line in": "", 
            mic ? ", Microphone": "", NULL);

}
*/

static gboolean extmic_caps(unsigned long pin_caps, unsigned long wid_caps)
{
    int vref = (pin_caps & AC_PINCAP_VREF) >> AC_PINCAP_VREF_SHIFT;
    return (pin_caps & AC_PINCAP_IN) && (vref & AC_PINCAP_VREF_50 || vref & AC_PINCAP_VREF_80);
}

static gboolean headphone_caps(unsigned long pin_caps, unsigned long wid_caps)
{
    return pin_caps & AC_PINCAP_HP_DRV;
}

static gboolean lineout_caps(unsigned long pin_caps, unsigned long wid_caps)
{
    return (pin_caps & AC_PINCAP_OUT) && !(wid_caps & AC_WCAP_DIGITAL);
}

static gboolean spdifout_caps(unsigned long pin_caps, unsigned long wid_caps)
{
    return (pin_caps & AC_PINCAP_OUT) && (wid_caps & AC_WCAP_DIGITAL) && !(pin_caps & AC_PINCAP_HDMI);
}

static gboolean hdmi_caps(unsigned long pin_caps, unsigned long wid_caps)
{
    return pin_caps & AC_PINCAP_HDMI;
}

static gboolean intmic_caps(unsigned long pin_caps, unsigned long wid_caps)
{
    return (pin_caps & AC_PINCAP_IN) && !(wid_caps & AC_WCAP_DIGITAL);
}

static gboolean spdifin_caps(unsigned long pin_caps, unsigned long wid_caps)
{
    return (pin_caps & AC_PINCAP_IN) && (wid_caps & AC_WCAP_DIGITAL);
}

static gboolean linein_caps(unsigned long pin_caps, unsigned long wid_caps)
{
    int vref = (pin_caps & AC_PINCAP_VREF) >> AC_PINCAP_VREF_SHIFT;
    return (pin_caps & AC_PINCAP_IN) && (vref & AC_PINCAP_VREF_HIZ);
}

static gboolean disabled_caps(unsigned long pin_caps, unsigned long wid_caps)
{
    return TRUE;
}

#define DEFAULT_MASK (AC_DEFCFG_PORT_CONN | AC_DEFCFG_GROSSLOC | AC_DEFCFG_DEVICE)

static typical_pins_t simple_typical_pins[] = {
    {"Headphone", headphone_caps, 0x0321403f, DEFAULT_MASK, },
    {"Microphone", extmic_caps, 0x03a19020, DEFAULT_MASK,},
    {"Line out (Front)", lineout_caps, 0x01014010, DEFAULT_MASK | AC_DEFCFG_SEQUENCE,},
    {"Line out (Center/LFE)", lineout_caps, 0x01014011, DEFAULT_MASK | AC_DEFCFG_SEQUENCE,},
    {"Line out (Back)", lineout_caps, 0x01014012, DEFAULT_MASK | AC_DEFCFG_SEQUENCE,},
    {"Line out (Side)", lineout_caps, 0x01014013, DEFAULT_MASK | AC_DEFCFG_SEQUENCE,},
    {"Line in", linein_caps, 0x0181304f, DEFAULT_MASK,},
    {"Internal speaker", lineout_caps, 0x90170150, DEFAULT_MASK | AC_DEFCFG_SEQUENCE, },
    {"Internal speaker (LFE)", lineout_caps, 0x90170151, DEFAULT_MASK | AC_DEFCFG_SEQUENCE, },
    {"Internal speaker (Back)", lineout_caps, 0x90170152, DEFAULT_MASK | AC_DEFCFG_SEQUENCE, },
    {"Internal mic", intmic_caps, 0x90a60160, DEFAULT_MASK,},
    {"HDMI / DisplayPort", hdmi_caps, 0x18560070, AC_DEFCFG_PORT_CONN | AC_DEFCFG_LOCATION,},
    {"SPDIF out", spdifout_caps, 0x014b1180, AC_DEFCFG_PORT_CONN | AC_DEFCFG_DEVICE,},
    {"SPDIF in", spdifin_caps, 0x01cb6190, AC_DEFCFG_PORT_CONN | AC_DEFCFG_DEVICE,},
    {"Dock Headphone", headphone_caps, 0x222140af, DEFAULT_MASK, },
    {"Dock Microphone", extmic_caps, 0x22a190a0, DEFAULT_MASK,},
    {"Dock Line out", lineout_caps, 0x220140b0, DEFAULT_MASK | AC_DEFCFG_SEQUENCE,},
    {"Dock Line in", linein_caps, 0x228130bf, DEFAULT_MASK,},
    {"Not connected", disabled_caps, 0x40f000f0, AC_DEFCFG_PORT_CONN,},
    {}
};



int get_typical_pins(typical_pins_t* result, int entries, pin_configs_t* pin_cfg, int caps_limit)
{
    int count = 0;
    int index = -1;
    unsigned long actual = actual_pin_config(pin_cfg);
    typical_pins_t* src;
    for (src = simple_typical_pins; src->name && entries; src++) {
        if (caps_limit && !src->caps_limit(pin_cfg->pin_caps, pin_cfg->wid_caps))
            continue;         
        if ((actual & src->match_mask) == (src->pin_set & src->match_mask))
            index = count;
        *result = *src;
        result++;
        count++;
        entries--;
    }
    if (entries) {
        result->name = NULL;
    }
    return index;
}

gboolean find_pin_channel_match(pin_configs_t* pins, int count, unsigned long pinval)
{
    int i;
    pinval &= (AC_DEFCFG_DEF_ASSOC + AC_DEFCFG_SEQUENCE);
    for (i = 0; i < count; i++, pins++) {
        unsigned long val2 = actual_pin_config(pins);
        if (get_port_conn(val2) == 1)
            continue;
        if (pinval == (val2 & (AC_DEFCFG_DEF_ASSOC + AC_DEFCFG_SEQUENCE)))
            return TRUE;
    }
    return FALSE;
}


