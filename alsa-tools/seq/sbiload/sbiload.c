/*
 *  ALSA hwdep SBI FM instrument loader
 *  Copyright (c) 2000 Uros Bizjak <uros@kss-loka.si>
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
 *
 *  Oct. 2007 - Takashi Iwai <tiwai@suse.de>
 *    Changed to use hwdep instead of obsoleted seq-instr interface
 */

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <alsa/asoundlib.h>
#include <alsa/sound/asound_fm.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define DATA_LEN_2OP	16
#define DATA_LEN_4OP	24

/* offsets for SBI params */
#define AM_VIB		0
#define KSL_LEVEL	2
#define ATTACK_DECAY	4
#define SUSTAIN_RELEASE	6
#define WAVE_SELECT	8

/* offset for SBI instrument */
#define CONNECTION	10
#define OFFSET_4OP	11

/* offsets for SBI extensions */
#define ECHO_DELAY	0
#define ECHO_ATTEN	1
#define CHORUS_SPREAD	2
#define TRNSPS		3
#define FIX_DUR		4
#define MODES		5
#define FIX_KEY		6

/* Options for the command */
#define HAS_ARG 1
static struct option long_opts[] = {
  {"device", HAS_ARG, NULL, 'D'},
  {"opl2", 0, NULL, '2'},
  {"opl3", 0, NULL, '4'},
  {"clear", 0, NULL, 'c'},
  {"path", HAS_ARG, NULL, 'P'},
  {"verbose", HAS_ARG, NULL, 'v'},
  {"quiet", 0, NULL, 'q'},
  {"version", 0, NULL, 'V'},
  {0, 0, 0, 0},
};

/* Number of elements in an array */
#define NELEM(a) ( sizeof(a)/sizeof((a)[0]) )

enum {
  FM_PATCH_UNKNOWN,
  FM_PATCH_OPL2,
  FM_PATCH_OPL3
};

/* Default file type */
static int file_type = FM_PATCH_UNKNOWN;

/* Default verbose level */
static int quiet;
static int verbose = 0;

/* Global declarations */
static snd_hwdep_t *handle;
static int iface;

#ifndef PATCHDIR
#define PATCHDIR "/usr/share/sounds/opl3"
#endif

static char *patchdir = PATCHDIR;

/* Function prototypes */
static void show_usage (void);
static void show_op (struct sbi_patch * instr, int type);

static int load_patch (struct sbi_patch * instr);
static int load_file (int bank, char *filename);

static int init_hwdep (const char *name);

/*
 * Show usage message
 */
static void
show_usage () {
  char **cpp;
  static char *msg[] = {
    "Usage: sbiload [options] [instfile [drumfile]]",
    "       sbiload [options] -c",
    "",
    "  -D, --device=name     - hwdep device string",
    "  -c, --clear           - Clear patches and exit",
    "  -2, --opl2            - two operators file type (OPL2)",
    "  -4, --opl3            - four operators file type (OPL3)",
    "  -P, --path=path       - Specify the patch path",
    "                          (default path: " PATCHDIR ")",
    "  -v, --verbose=level   - Verbose level (default = 0)",
    "  -q, --quiet           - Be quiet, no error/warning messages",
    "  -V, --version         - Show version",
  };

  for (cpp = msg; cpp < msg + NELEM (msg); cpp++) {
      printf ("%s\n", *cpp);
  }
}

/*
 * Show version
 */
static void
show_version () {
  printf("Version: " VERSION "\n");
}

/*
 * Show instrument FM operators
 */
static void
show_op (struct sbi_patch * inst, int type) {
  int i = 0;
  int ofs = 0;

  do {
    unsigned char val;
    val = inst->data[AM_VIB + ofs];
    printf ("  OP%i: flags: %s %s %s %s", i,
	    val & (1 << 7) ? "AM" : "  ",
	    val & (1 << 6) ? "VIB" : "   ",
	    val & (1 << 5) ? "EGT" : "   ",
	    val & (1 << 4) ? "KSR" : "   ");
    val = inst->data[AM_VIB + ofs + 1];
    printf ("\011OP%i: flags: %s %s %s %s\n", i + 1,
	    val & (1 << 7) ? "AM" : "  ",
	    val & (1 << 6) ? "VIB" : "   ",
	    val & (1 << 5) ? "EGT" : "   ",
	    val & (1 << 4) ? "KSR" : "");
    val = inst->data[AM_VIB + ofs];
    printf ("  OP%i: MULT = 0x%x", i, val & 0x0f);
    val = inst->data[AM_VIB + ofs + 1];
    printf ("\011\011OP%i: MULT = 0x%x\n", i + 1, val & 0x0f);

    val = inst->data[KSL_LEVEL + ofs];
    printf ("  OP%i: KSL  = 0x%x  TL = 0x%.2x", i,
	    (val >> 6) & 0x03, val & 0x3f);
    val = inst->data[KSL_LEVEL + ofs + 1];
    printf ("\011OP%i: KSL  = 0x%x  TL = 0x%.2x\n", i + 1,
	    (val >> 6) & 0x03, val & 0x3f);
    val = inst->data[ATTACK_DECAY + ofs];
    printf ("  OP%i: AR   = 0x%x  DL = 0x%x", i,
	    (val >> 4) & 0x0f, val & 0x0f);
    val = inst->data[ATTACK_DECAY + ofs + 1];
    printf ("\011OP%i: AR   = 0x%x  DL = 0x%x\n", i + 1,
	    (val >> 4) & 0x0f, val & 0x0f);
    val = inst->data[SUSTAIN_RELEASE + ofs];
    printf ("  OP%i: SL   = 0x%x  RR = 0x%x", i,
	    (val >> 4) & 0x0f, val & 0x0f);
    val = inst->data[SUSTAIN_RELEASE + ofs + 1];
    printf ("\011OP%i: SL   = 0x%x  RR = 0x%x\n", i + 1,
	    (val >> 4) & 0x0f, val & 0x0f);
    val = inst->data[WAVE_SELECT + ofs];
    printf ("  OP%i: WS   = 0x%x", i, val & 0x07);
    val = inst->data[WAVE_SELECT + ofs + 1];
    printf ("\011\011OP%i: WS   = 0x%x\n", i + 1, val & 0x07);
    val = inst->data[CONNECTION + ofs];
    printf (" FB = 0x%x,  %s\n", (val >> 1) & 0x07,
	    val & (1 << 0) ? "parallel" : "serial");
    i += 2;
    ofs += OFFSET_4OP;
  }
  while (i == (type == FM_PATCH_OPL3) << 1);

  printf (" Extended data:\n"
	  "  ED = %.3i  EA = %.3i  CS = %.3i  TR = %.3i\n"
	  "  FD = %.3i  MO = %.3i  FK = %.3i\n",
	  inst->extension[ECHO_DELAY], inst->extension[ECHO_ATTEN],
	  inst->extension[CHORUS_SPREAD], inst->extension[TRNSPS],
	  inst->extension[FIX_DUR], inst->extension[MODES],
	  inst->extension[FIX_KEY]);
}

/*
 * Send patch to destination port
 */
static int
load_patch (struct sbi_patch * inst) {

  ssize_t ret;
  ret = snd_hwdep_write(handle, inst, sizeof(*inst));
  if (ret != sizeof(*inst)) {
    if (!quiet)
      fprintf (stderr, "Unable to write an instrument %.3i put event: %s\n",
	       inst->prog, snd_strerror (ret));
    return -1;
  }

  if (verbose)
    printf ("Loaded instrument %.3i, bank %.3i: %s\n",
	    inst->prog, inst->bank, inst->name);
  return 0;
}

/*
 * Parse standard .sb or .o3 file
 */
static void
load_sb (int bank, int fd) {
  int len;
  int prg;
  struct sbi_patch inst;
  int fm_instr_type;

  len = (file_type == FM_PATCH_OPL3) ? DATA_LEN_4OP : DATA_LEN_2OP;
  for (prg = 0;; prg++) {
    inst.prog = prg;
    inst.bank = bank;

    if (read (fd, inst.key, 4) != 4)
      break;

    if (!memcmp (inst.key, "SBI\032", 4) || !memcmp (inst.key, "2OP\032", 4)) {
      fm_instr_type = FM_PATCH_OPL2;
    } else if (!strncmp (inst.key, "4OP\032", 4)) {
      fm_instr_type = FM_PATCH_OPL3;
    } else {
      if (verbose)
	printf ("%.3i: wrong instrument key!\n", prg);
      fm_instr_type = FM_PATCH_UNKNOWN;
    }

    if (read (fd, &inst.name, sizeof(inst.name)) != sizeof(inst.name) ||
	read (fd, &inst.extension, sizeof(inst.extension)) != sizeof(inst.extension) ||
	read (fd, &inst.data, len) != len)
      break;

    if (fm_instr_type == FM_PATCH_UNKNOWN)
      continue;

    if (verbose > 1) {
      printf ("%.3i: [%s] %s\n", inst.prog,
	      fm_instr_type == FM_PATCH_OPL2 ? "OPL2" : "OPL3",
	      inst.name);
      show_op (&inst, fm_instr_type);
    }

    if (load_patch (&inst) < 0)
      break;
  }
  return;
}

/*
 * Load file
 */
static int
load_file (int bank, char *filename) {
  int fd;
  char path[1024];
  char *name;

  if (*filename != '/') {
    snprintf(path, sizeof(path), "%s/%s", patchdir, filename);
    name = path;
  } else {
    name = filename;
  }

  fd = open (name, O_RDONLY);
  if (fd < 0) {
    /* try to guess from the interface name */
    const char *ext = iface == SND_HWDEP_IFACE_OPL2 ? "sb" : "o3";
    if (*filename != '/')
      snprintf(path, sizeof(path), "%s/%s.%s", patchdir, filename, ext);
    else
      snprintf(path, sizeof(path), "%s.%s", filename, ext);
    name = path;
    fd = open (name, O_RDONLY);
    if (fd < 0) {
      if (!quiet)
	perror (filename);
      return -1;
    }
  }

  /* correct file type if not set explicitly */
  if (file_type == FM_PATCH_UNKNOWN)
    file_type = iface == SND_HWDEP_IFACE_OPL2 ? FM_PATCH_OPL2 : FM_PATCH_OPL3;

  if (verbose)
    fprintf (stderr, "Loading from %s\n", name);

  load_sb(bank, fd);

  close (fd);
  return 0;
}

static void
clear_patches (void)
{
  snd_hwdep_ioctl(handle, SNDRV_DM_FM_IOCTL_CLEAR_PATCHES, 0);
}

/*
 * Open a hwdep device
 */
static int open_hwdep (const char *name)
{
  int err;
  snd_hwdep_info_t *info;

  if ((err = snd_hwdep_open (&handle, name, SND_HWDEP_OPEN_WRITE)) < 0)
    return err;

  snd_hwdep_info_alloca(&info);
  if (!snd_hwdep_info (handle, info)) {
    iface = snd_hwdep_info_get_iface(info);
    if (iface == SND_HWDEP_IFACE_OPL2 ||
	iface == SND_HWDEP_IFACE_OPL3 ||
	iface == SND_HWDEP_IFACE_OPL4)
      return 0;
  }
  snd_hwdep_close(handle);
  handle = NULL;
  return -EINVAL;
}

static int
init_hwdep (const char *name) {

  int err;
  char tmpname[16];

  if (!name || !*name) {
    /* auto probe */
    int card = -1;
    snd_ctl_t *ctl;

    while (!snd_card_next(&card) && card >= 0) {
      int dev;
      sprintf(tmpname, "hw:%d", card);
      if (snd_ctl_open(&ctl, tmpname, 0) < 0)
	continue;
      dev = -1;
      while (!snd_ctl_hwdep_next_device(ctl, &dev) && dev >= 0) {
	sprintf(tmpname, "hw:%d,%d", card, dev);
	if (!open_hwdep(tmpname)) {
	  snd_ctl_close(ctl);
	  return 0;
	}
      }
      snd_ctl_close(ctl);
    }
    if (!quiet)
      fprintf (stderr, "Can't find any OPL3 hwdep device\n");
    return -1;
  }

  if (*name == '/') {
    /* guess card and device numbers - for convenience to user
     * from udev rules
     */
    int card, device;
    if (sscanf(name, "/dev/snd/hwC%dD%d", &card, &device) == 2) {
      if (card >= 0 && card <= 32 && device >= 0 && device <= 32) {
	sprintf(tmpname, "hw:%d,%d", card, device);
	name = tmpname; /* override */
      }
    }
  }

  if ((err = open_hwdep (name)) < 0) {
    if (!quiet)
      fprintf (stderr, "Could not open hwdep %s: %s\n",
	       name, snd_strerror (err));
    return -1;
  }

  return 0;
}

/*
 * Unsubscribe client from destination port
 * and close sequencer
 */
static void
finish_hwdep ()
{
  snd_hwdep_close(handle);
  handle = NULL;
}

/*
 * Load a .SBI FM instrument patch
 *   sbiload [-p client:port] [-l] [-P path] [-v level] instfile drumfile
 *
 *   -D, --device=name       - An ALSA hwdep name to use
 *   -2  --opl2              - two operators file type (*.sb)
 *   -4  --opl3              - four operators file type (*.o3)
 *   -P, --path=path         - Specify the patch path
 *   -v, --verbose=level     - Verbose level
 *   -q, --quiet             - Be quiet, no error/warning messages
 */
int
main (int argc, char **argv) {
  char opts[NELEM (long_opts) * 2 + 1];
  char *name;
  char *cp;
  int c;
  int clear = 0;
  struct option *op;

  /* Build up the short option string */
  cp = opts;
  for (op = long_opts; op < &long_opts[NELEM (long_opts)]; op++) {
    *cp++ = op->val;
    if (op->has_arg)
      *cp++ = ':';
  }

  name = NULL;

  /* Deal with the options */
  for (;;) {
    c = getopt_long (argc, argv, opts, long_opts, NULL);
    if (c == -1)
      break;

    switch (c) {
    case 'D':
      name = optarg;
      break;
    case 'c':
      clear = 1;
      break;
    case '2':
      file_type = FM_PATCH_OPL2;
      break;
    case '4':
      file_type = FM_PATCH_OPL3;
      break;
    case 'q':
      quiet = 1;
      verbose = 0;
      break;
    case 'v':
      quiet = 0;
      verbose = atoi (optarg);
      break;
    case 'V':
      show_version();
      exit (1);
    case 'P':
      patchdir = optarg;
      break;
    case '?':
      show_usage ();
      exit (1);
    }
  }

  if (init_hwdep (name) < 0) {
    return 1;
  }

  clear_patches ();
  if (clear)
    goto done;

  /* Process instrument and drum file */
  if (optind < argc)
    name = argv[optind++];
  else
    name = "std";
  if (load_file (0, name) < 0) {
    finish_hwdep();
    return 1;
  }
  if (optind < argc)
    name = argv[optind];
  else
    name = "drums";
  if (load_file (128, name) < 0) {
    finish_hwdep();
    return 1;
  }

 done:
  finish_hwdep();
  return 0;
}
