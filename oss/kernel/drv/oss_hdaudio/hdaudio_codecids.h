/*
 * Purpose: Definitions for HDaudio codec chips known by OSS.
 */
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

/*
 * NULL mixer init function. Used to disable mixer creation for given codec.
 */
static int NULL_mixer_init (int dev, hdaudio_mixer_t * mixer, int cad, int top_group) { return 0;}

struct codec_desc
{
  unsigned int id; /* Codec id (however subdevice ID in the subdevices[] table) */
  char *name;
  unsigned long flags;
#define VF_NONE         0x00000000
#define VF_ALC88X_HACK  0x00000001      /* ALC88x requires special handling for
S/PDIF */
#define VF_VAIO_HACK    0x00000002      /* VAIO STAC9872 requires special
handling for headphone DAC */
#define VF_SI3055_HACK  0x00000004      /* Si3055 modem requires manual endpoint
setuping and rate and ioctl hacks. */
  char **remap;

  /*
   * Order of the output converter (DAC) widgets in multi channel mode.
   * If left to 0 then 0x76543210 (sequential order) is assumed. Some
   * motherboards are known to use their own channel ordering which can be
   * fixed using this approach.
   */
  unsigned int multich_map;
  hda_mixer_init_func mixer_init;
  unsigned int main_id; /* In the subdevices[] table this is used for the codec ID */
  unsigned int pci_subdevice; /* PCI subdevice ID of the controller (subdevices[]) */
};


/*
 * Meaningful widget naming schemes for some known codecs.
 * The list is terminated by a NULL string. An empty string
 * (EMPTY_STR) means that the automatically selected name will be used.
 *
 * Define EMPTY_STR as a pointer to a common "" variable to conserve space.
 */

static const char hda_empty_string[] = "";
#define EMPTY_STR hda_empty_string

static const char *alc880remap[] = {
  EMPTY_STR,				/* 0 */
  EMPTY_STR,				/* 1 */
  "front",			/* 2 */
  "rear",			/* 3 */
  "center/LFE",			/* 4 */
  "side",			/* 5 */
  "spdif-out",			/* 6 */
  "rec1",			/* 7 */
  "rec2",			/* 8 */
  "rec3",			/* 9 */
  "spdif-in",			/* 10 */
  "inputmix",			/* 11 */
  "front",			/* 12 */
  "rear",			/* 13 */
  "center/LFE",			/* 14 */
  "side",			/* 15 */
  "source-a",			/* 16 */
  "source-b",			/* 17 */
  "source-c",			/* 18 */
  "source-d",			/* 19 */
  EMPTY_STR,				/* 20 */
  EMPTY_STR,				/* 21 */
  EMPTY_STR,				/* 22 */
  EMPTY_STR,				/* 23 */
  EMPTY_STR,				/* 24 */
  EMPTY_STR,				/* 25 */
  EMPTY_STR,				/* 26 */
  EMPTY_STR,				/* 27 */
  EMPTY_STR,				/* 28 */
  EMPTY_STR,				/* 29 */
  EMPTY_STR,				/* 30 */
  EMPTY_STR,				/* 31 */
  EMPTY_STR,				/* 32 */
  EMPTY_STR,				/* 33 */
  EMPTY_STR,				/* 34 */
  EMPTY_STR,				/* 35 */
  EMPTY_STR,				/* 36 */
  "fp-front",			/* 37 */
  NULL
};

static const char *alc883remap[] = {
  EMPTY_STR,                           /* 0 */
  EMPTY_STR,                           /* 1 */
  "front",                 	/* 2 */
  "rear",                  	/* 3 */
  "center/LFE",            	/* 4 */
  "side",                  	/* 5 */
  "spdif-out",                	/* 6 */
  EMPTY_STR,                       	/* 7 */
  "rec1",                       /* 8 */
  "rec2",                      	/* 9 */
  "spdif-in",                 	/* a */
  "input-mix",                  /* b */
  "front",                      /* c */
  "rear",                       /* d */
  "center/LFE",                 /* e */
  "side",                       /* f */
  EMPTY_STR,                   	/* 10 */
  EMPTY_STR,                   	/* 11 */
  EMPTY_STR,                   	/* 12 */
  EMPTY_STR,                   	/* 13 */
  EMPTY_STR,                           /* 14 */
  EMPTY_STR,                           /* 15 */
  EMPTY_STR,                           /* 16 */
  EMPTY_STR,                           /* 17 */
  EMPTY_STR,                       	/* 18 */
  EMPTY_STR,                   	/* 19 */
  EMPTY_STR,                   	/* 1a */
  EMPTY_STR,                   	/* 1b */
  EMPTY_STR,                         	/* 1c */
  EMPTY_STR,                       	/* 1d */
  EMPTY_STR,                  		/* 1e */
  EMPTY_STR,                   	/* 1f */
  EMPTY_STR,                           /* 20 */
  EMPTY_STR,                           /* 21 */
  EMPTY_STR,                           /* 22 */
  EMPTY_STR,                           /* 23 */
  EMPTY_STR,                           /* 24 */
  "pcm4",                       /* 25 */
  "pcm4",                       /* 26 */
  NULL
};

static const char *alc260remap[] = {
  EMPTY_STR,				/* 0x00 */
  EMPTY_STR,				/* 0x01 */
  "front",			/* 0x02 */
  "spdif-out",			/* 0x03 */
  "rec1",			/* 0x04 */
  "rec2",			/* 0x05 */
  "spdif-in",			/* 0x06 */
  "inputmix",			/* 0x07 */
  "speaker-mix",		/* 0x08 */
  "headphone-mix",		/* 0x09 */
  "mono-mix",			/* 0x0a */
  EMPTY_STR,				/* 0x0b */
  EMPTY_STR,				/* 0x0c */
  EMPTY_STR,				/* 0x0d */
  EMPTY_STR,				/* 0x0e */
  "speaker",			/* 0x0f */
  "headphone",			/* 0x10 */
  "mono",			/* 0x11 */
  EMPTY_STR,				/* 0x12 */
  EMPTY_STR,				/* 0x13 */
  EMPTY_STR,				/* 0x14 */
  EMPTY_STR,				/* 0x15 */
  EMPTY_STR,				/* 0x16 */
  "beep",			/* 0x17 */
  "spdif-out",			/* 0x18 */
  "spdif-in",			/* 0x19 */
  NULL
};

static const char *alc262remap[] = {
  EMPTY_STR,                           /* 0x00 */
  EMPTY_STR,                           /* 0x01 */
  "speaker",               	/* 0x02 */
  "headphone",             	/* 0x03 */
  EMPTY_STR,                       	/* 0x04 */
  EMPTY_STR,                       	/* 0x05 */
  "spdif-out",                  /* 0x06 */
  "rec1",                       /* 0x07 */
  "rec2",                       /* 0x08 */
  "rec3",                       /* 0x09 */
  "spdif-in",                   /* 0x0a */
  "mix",                        /* 0x0b */
  EMPTY_STR,                           /* 0x0c */
  EMPTY_STR,                           /* 0x0d */
  EMPTY_STR,                           /* 0x0e */
  EMPTY_STR,                           /* 0x0f */
  EMPTY_STR,                           /* 0x10 */
  "mono",                       /* 0x11 */
  "dmic",                       /* 0x12 */
  EMPTY_STR,                           /* 0x13 */
  "line-out",                   /* 0x14 */
  "headphone",                  /* 0x15 */
  "mono",                       /* 0x16 */
  "beep",                       /* 0x17 */
  EMPTY_STR,                       	/* 0x18 */
  EMPTY_STR,                       	/* 0x19 */
  EMPTY_STR,                   	/* 0x1a */
  EMPTY_STR,                   	/* 0x1b */
  EMPTY_STR,                         	/* 0x1c */
  "beep",                       /* 0x1d */
  "spdif-out",                  /* 0x1e */
  "spdif-in",                   /* 0x1f */
  NULL
};

static const char *alc662remap[] = {
  EMPTY_STR,                           /* 0x00 */
  EMPTY_STR,                           /* 0x01 */
  "front",                 	/* 0x02 */
  "rear",                 	/* 0x03 */
  "center/LFE",            	/* 0x04 */
  EMPTY_STR,            		/* 0x05 */
  "spdif-out",                  /* 0x06 */
  "spdout",                     /* 0x07 */
  "rec1",                       /* 0x08 */
  "rec2",                      	/* 0x09 */
  EMPTY_STR,                      	/* 0x0a */
  "mix",                        /* 0x0b */
  "front",                      /* 0x0c */
  "rear",                       /* 0x0d */
  "c/lfe",                      /* 0x0e */
  EMPTY_STR,                           /* 0x0f */
  EMPTY_STR,                           /* 0x10 */
  EMPTY_STR,                           /* 0x11 */
  EMPTY_STR,                           /* 0x12 */
  EMPTY_STR,                      	/* 0x13 */
  EMPTY_STR,                    	/* 0x14 */
  EMPTY_STR,                     	/* 0x15 */
  EMPTY_STR,                   	/* 0x16 */
  EMPTY_STR,                    	/* 0x17 */
  EMPTY_STR,                    	/* 0x18 */
  EMPTY_STR,                    	/* 0x19 */
  EMPTY_STR,                  		/* 0x1a */
  EMPTY_STR,                   	/* 0x1b */
  EMPTY_STR,                   	/* 0x1c */
  EMPTY_STR,                      	/* 0x1d */
  EMPTY_STR,                  		/* 0x1e */
  EMPTY_STR,                           /* 0x1f */
  EMPTY_STR,                           /* 0x20 */
  EMPTY_STR,                           /* 0x21 */
  EMPTY_STR,                           /* 0x22 */
  EMPTY_STR,                   	/* 0x23 */
  NULL
};

static const char *alc861remap[] = {
  EMPTY_STR,				/* 0x00 */
  EMPTY_STR,				/* 0x01 */
  EMPTY_STR,				/* 0x02 */
  "front",			/* 0x03 */
  "side",			/* 0x04 */
  "center/LFE",			/* 0x05 */
  "rear",			/* 0x06 */
  "spdout",			/* 0x07 */
  "rec",			/* 0x08 */
  EMPTY_STR,				/* 0x09 */
  EMPTY_STR,				/* 0x0a */
  EMPTY_STR,				/* 0x0b */
  EMPTY_STR,				/* 0x0c */
  EMPTY_STR,				/* 0x0d */
  EMPTY_STR,				/* 0x0e */
  EMPTY_STR,				/* 0x0f */
  EMPTY_STR,				/* 0x10 */
  EMPTY_STR,				/* 0x11 */
  EMPTY_STR,				/* 0x12 */
  EMPTY_STR,				/* 0x13 */
  "recmix",			/* 0x14 */
  "outmix",			/* 0x15 */
  "frontmix",			/* 0x16 */
  "sidemix",			/* 0x17 */
  "c/l-mix",			/* 0x18 */
  "rearmix",			/* 0x19 */
  "line2-mix",			/* 0x1a */
  "mic2mix",			/* 0x1b */
  "line1mix",			/* 0x1c */
  EMPTY_STR,				/* 0x1d */
  "vendor",			/* 0x1e */
  "center/LFE",			/* 0x1f */
  "side",			/* 0x20 */
  EMPTY_STR,				/* 0x21 */
  EMPTY_STR,				/* 0x22 */
  "beep",			/* 0x23 */
  NULL
};

static const char *cmi9880remap[] = {
  EMPTY_STR,				/* 0 */
  EMPTY_STR,				/* 1 */
  EMPTY_STR,				/* 2 */
  "front",			/* 3 */
  "rear",			/* 4 */
  "side",			/* 5 */
  "center/LFE",			/* 6 */
  "spdif-out",			/* 7 */
  EMPTY_STR,				/* 8 */
  EMPTY_STR,				/* 9 */
  EMPTY_STR,				/* 10 */
  EMPTY_STR,				/* 11 */
  EMPTY_STR,				/* 12 */
  EMPTY_STR,				/* 13 */
  EMPTY_STR,				/* 14 */
  EMPTY_STR,				/* 15 */
  EMPTY_STR,				/* 16 */
  EMPTY_STR,				/* 17 */
  EMPTY_STR,				/* 18 */
  EMPTY_STR,				/* 19 */
  EMPTY_STR,				/* 20 */
  EMPTY_STR,				/* 21 */
  EMPTY_STR,				/* 22 */
  "pcbeep",			/* 23 */
  EMPTY_STR,				/* 24 */
  EMPTY_STR,				/* 25 */
  EMPTY_STR,				/* 26 */
  EMPTY_STR,				/* 27 */
  EMPTY_STR,				/* 28 */
  EMPTY_STR,				/* 29 */
  EMPTY_STR,				/* 30 */
  EMPTY_STR,				/* 31 */
  NULL
};

static const char *ad1981remap[] = {
  EMPTY_STR,				/* 0 */
  EMPTY_STR,				/* 1 */
  "spdif",			/* 2 */
  "play",			/* 3 */
  "rec",			/* 4 */
  EMPTY_STR,				/* 5 */
  EMPTY_STR,				/* 6 */
  EMPTY_STR,				/* 7 */
  EMPTY_STR,				/* 8 */
  EMPTY_STR,				/* 9 */
  "spdif-out",			/* a */
  "mono-sel",			/* b */
  "mic-mix",			/* c */
  "pcbeep-sel",			/* d */
  "rec-mix",			/* e */
  "mono-mix",			/* f */
  "digital-beep",		/* 10 */
  "frontmix-amp",		/* 11 */
  "mic-mixamp",			/* 12 */
  "linein-mixamp",		/* 13 */
  "powerdown",			/* 14 */
  "rec-sel",			/* 15 */
  EMPTY_STR,				/* 16 */
  EMPTY_STR,				/* 17 */
  EMPTY_STR,				/* 18 */
  EMPTY_STR,				/* 19 */
  "lineout-mixamp",		/* 1a */
  "aux-mixamp",			/* 1b */
  "mic-mixamp",			/* 1c */
  "CD-mixamp",			/* 1d */
  "fp-mic-mute",		/* 1e */
  "mic-mute",			/* 1f */
  NULL
};

static const char *ad1983remap[] = {
  EMPTY_STR,                           /* 0 */
  EMPTY_STR,                           /* 1 */
  "spdif",                      /* 2 */
  "play",                       /* 3 */
  "rec",                        /* 4 */
  EMPTY_STR,                   	/* 5 */
  EMPTY_STR,          			/* 6 */
  EMPTY_STR,                  		/* 7 */
  EMPTY_STR,                  		/* 8 */
  EMPTY_STR,                  		/* 9 */
  "spdif-out",                  /* a */
  "mono-sel",                	/* b */
  "mic-boost",                  /* c */
  "linein-sel",                 /* d */
  "rec-mix",                    /* e */
  "mono-mix",                   /* f */
  "digital-beep",               /* 10 */
  "frontmix-amp",               /* 11 */
  "mic-mixamp",                 /* 12 */
  "linein-mixamp",              /* 13 */
  "rec-sel",                    /* 14 */
  "powerdown",                  /* 15 */
  NULL
};

static const char *ad1984remap[] = {
  EMPTY_STR,                           /* 0 */
  EMPTY_STR,                           /* 1 */
  "spdif",                      /* 2 */
  "headphone",                  /* 3 */
  "front",                 	/* 4 */
  "dig-mic1",                   /* 5 */
  "dig-mic2",                   /* 6 */
  "headphone-mix",              /* 7 */
  "rec1",                       /* 8 */
  "rec2",                       /* 9 */
  "lineout-mix",                /* a */
  "aux-mix",                    /* b */
  "rec1-sel",                  	/* c */
  "rec2-sel",                 	/* d */
  "mono-sel",                  	/* e */
  "aux-sel",                   	/* f */
  "beep",               	/* 10 */
  EMPTY_STR,               		/* 11 */
  EMPTY_STR,                 		/* 12 */
  EMPTY_STR,              		/* 13 */
  EMPTY_STR,                  		/* 14 */
  EMPTY_STR,                  		/* 15 */
  EMPTY_STR,                  		/* 16 */
  EMPTY_STR,                  		/* 17 */
  EMPTY_STR,                  		/* 18 */
  "mixer-powerdown",            /* 19 */
  "beep",                  	/* 1a */
  "spdif-out",                  /* 1b */
  EMPTY_STR,                  		/* 1c */
  EMPTY_STR,                  		/* 1d */
  "mono-mix",                  	/* 1e */
  "mono-downmix",               /* 1f */
  "input-mix",                  /* 20 */
  "input-mixamp",               /* 21 */
  "headphone-sel",              /* 22 */
  "dock-sel",                  	/* 23 */
  "dock-mix",                  	/* 24 */
  "dock-micboost",              /* 25 */
  EMPTY_STR,                  		/* 26 */
  NULL
};

static const char *ad1986remap[] = {
  EMPTY_STR,				/* 0 */
  EMPTY_STR,				/* 1 */
  "spdif-out",			/* 2 */
  "front",			/* 3 */
  "rear",			/* 4 */
  "center/LFE",			/* 5 */
  "rec",			/* 6 */
  "recmon",			/* 7 */
  "mono-mix",			/* 8 */
  "stereo-downmix",		/* 9 */
  "headphone-sel",		/* a */
  "lineout-sel",		/* b */
  "rear-sel",			/* c */
  "center/LFE-sel",		/* d */
  "mono-sel",			/* e */
  "mic-sel",			/* f */
  "linein-sel",			/* 10 */
  "mic-src",			/* 11 */
  "rec-src"			/* 12 */
  "mic-mix",			/* 13 */
  "phone-mix",			/* 14 */
  "cd-mix",			/* 15 */
  "aux-mix",			/* 16 */
  "linein-mix",			/* 17 */
  "beep",			/* 18 */
  "digital-beep",		/* 19 */
  EMPTY_STR,				/* 1a */
  EMPTY_STR,				/* 1b */
  EMPTY_STR,				/* 1c */
  EMPTY_STR,				/* 1d */
  EMPTY_STR,				/* 1e */
  EMPTY_STR,				/* 1f */
  EMPTY_STR,				/* 20 */
  EMPTY_STR,				/* 21 */
  EMPTY_STR,				/* 22 */
  EMPTY_STR,				/* 23 */
  EMPTY_STR,				/* 24 */
  "spdif-out",			/* 25 */
  "analog-powerdown",		/* 26 */
  "mic-c/LFE-mix",		/* 27 */
  "mic-linein-mix",		/* 28 */
  "c/LFE-linein-mix",		/* 29 */
  "mic-linein-c/LFE-mix",	/* 2a */
  "mic-sel",			/* 2b */
  NULL
};

static const char *ad1988remap[] = {
  EMPTY_STR,				/* 0 */
  EMPTY_STR,				/* 1 */ /* This is both audio-fgr and DAC???? */
  "spdout",			/* 2 */ /* Not used? */
  "headphone",			/* 3 */
  "front",			/* 4 */
  "center/LFE",			/* 5 */
  "rear",			/* 6 */
  "spdin",			/* 7 */
  "rec1",			/* 8 */
  "rec2",			/* 9 */
  "side",			/* a */
  "spdin-src",			/* b */
  "rec1-src",			/* c */
  "rec2-src",			/* d */
  "rec3-src",			/* e */
  "rec3",			/* f */
  "pcbeep",			/* 10 */
  EMPTY_STR,				/* 11 */
  EMPTY_STR,				/* 12 */
  EMPTY_STR,				/* 13 */
  EMPTY_STR,				/* 14 */
  EMPTY_STR,				/* 15 */
  EMPTY_STR,				/* 16 */
  EMPTY_STR,				/* 17 */
  EMPTY_STR,				/* 18 */
  "power-down",			/* 19 */
  "beep",			/* 1a */
  "spdif-out",			/* 1b */
  "spdif-in",			/* 1c */
  "spdifout-mix",		/* 1d */
  "mono-mix",			/* 1e */
  "main-volume",		/* 1f */
  "analog-mix",			/* 20 */
  "outamp",			/* 21 */
  "headphon-mix",		/* 22 */
  "hp-powerdown",		/* 23 */
  EMPTY_STR,				/* 24 */
  EMPTY_STR,				/* 25 */
  "mic-mix",			/* 26 */
  "center/LFE-mix",		/* 27 */
  "side-mix",			/* 28 */
  "front-mix",			/* 29 */
  "rear-mix",			/* 2a */
  "fp-mic-mix",			/* 2b */
  "linein-mix",			/* 2c */
  "mono-mixdown",		/* 2d */
  EMPTY_STR,				/* 2e */
  "bias-pdown",			/* 2f */
  "fppink-outsel",		/* 30 */
  "blue-outsel",		/* 31 */
  "pink-outsel",		/* 32 */
  "blue-insel",			/* 33 */
  "pink-insel",			/* 34 */
  EMPTY_STR,				/* 35 */
  "mono-sel",			/* 36 */
  "fpgreen-outsel",		/* 37 */
  "fpgreen-micboost",		/* 38 */
  "fppink-micboost",		/* 39 */
  "blue-micboost",		/* 3a */
  "black-micboost",		/* 3b */
  "pink-micboost",		/* 3c */
  "green-micboost",		/* 3d */
  NULL
};

#if 0
static const char *stac9200remap[] = {
  EMPTY_STR,				/* 0 */
  EMPTY_STR,				/* 0x01 */
  "play",			/* 0x02 */
  "rec",			/* 0x03 */
  "spdif-in",			/* 0x04 */
  "spdif-out",			/* 0x05 */
  EMPTY_STR,				/* 0x06 */
  "playmux",			/* 0x07 */
  EMPTY_STR,				/* 0x08 */
  EMPTY_STR,				/* 0x09 */
  "recmux",			/* 0x0a */
  "mainvol",			/* 0x0b */
  "inputmux",			/* 0x0c */
  EMPTY_STR,				/* 0x0d */
  EMPTY_STR,				/* 0x0e */
  EMPTY_STR,				/* 0x0f */
  EMPTY_STR,				/* 0x10 */
  EMPTY_STR,				/* 0x11 */
  EMPTY_STR,				/* 0x12 */
  "mono-mix",			/* 0x13 */
  "beep",			/* 0x14 */
  NULL
};
#endif

static const char *stac920xremap[] = {
  EMPTY_STR,                           /* 0 */
  EMPTY_STR,                           /* 0x01 */
  EMPTY_STR,                 		/* 0x02 */
  EMPTY_STR,            		/* 0x03 */
  EMPTY_STR,                  		/* 0x04 */
  EMPTY_STR,                  		/* 0x05 */
  EMPTY_STR,                  		/* 0x06 */
  EMPTY_STR,                  		/* 0x07 */
  EMPTY_STR,                       	/* 0x08 */
  EMPTY_STR,                       	/* 0x09 */
  EMPTY_STR,                           /* 0x0a */
  EMPTY_STR,                           /* 0x0b */
  EMPTY_STR,                           /* 0x0c */
  EMPTY_STR,                           /* 0x0d */
  EMPTY_STR,                           /* 0x0e */
  EMPTY_STR,                           /* 0x0f */
  "front",                 	/* 0x10 */
  "rear",                  	/* 0x11 */
  "rec1",                       /* 0x12 */
  "rec2",                   	/* 0x13 */
  "mono-out",                   /* 0x14 */
  "mono-mix",                 	/* 0x15 */
  "cd",                 	/* 0x16 */
  "dig-mic1",                 	/* 0x17 */
  "dig-mic2", 			/* 0x18 */
  "input1-mux",                 /* 0x19 */
  "input2-mux",                 /* 0x1a */
  "rec1-vol",                   /* 0x1b */
  "rec2-vol",                   /* 0x1c */
  "rec1-mux",                   /* 0x1d */
  "rec2-mux",                  	/* 0x1e */
  "spdif-out",                  /* 0x1f */
  "spdif-in",                   /* 0x20 */
  "digital-out",                /* 0x21 */
  "digital-in",                 /* 0x22 */
  "beep",                       /* 0x23 */
  "mastervol",                  /* 0x24 */
  EMPTY_STR,                       	/* 0x25 */
  NULL
};

static const char *stac925xremap[] = {
  EMPTY_STR,                           /* 0 */
  EMPTY_STR,                           /* 0x01 */
  "play",                       /* 0x02 */
  "rec",                        /* 0x03 */
  "spdif-in",                   /* 0x04 */
  "spdif-out",          	/* 0x05 */
  "output-mux",                	/* 0x06 */
  EMPTY_STR,                    	/* 0x07 */
  EMPTY_STR,                   	/* 0x08 */
  "rec-vol",                   	/* 0x09 */
  EMPTY_STR,                     	/* 0x0a */
  EMPTY_STR,                    	/* 0x0b */
  EMPTY_STR,                   	/* 0x0c */
  EMPTY_STR,                           /* 0x0d */
  "vol",                   	/* 0x0e */
  "input-mux",                  /* 0x0f */
  EMPTY_STR,                           /* 0x10 */
  EMPTY_STR,                           /* 0x11 */
  "mono-mix",                   /* 0x12 */
  "beep",                   	/* 0x13 */
  "rec-mux",                    /* 0x14 */
  EMPTY_STR,                       	/* 0x15 */
  NULL
};

static const char *stac922xremap[] = {
  EMPTY_STR,				/* 0 */
  EMPTY_STR,				/* 0x01 */
  "front",			/* 0x02 */
  "center/LFE",			/* 0x03 */
  "rear",			/* 0x04 */
  "side",			/* 0x05 */
  "rec1",			/* 0x06 */
  "rec2",			/* 0x07 */
  "spdif-out",			/* 0x08 */
  "spdif-in",			/* 0x09 */
  EMPTY_STR,				/* 0x0a */
  EMPTY_STR,				/* 0x0b */
  EMPTY_STR,				/* 0x0c */
  EMPTY_STR,				/* 0x0d */
  EMPTY_STR,				/* 0x0e */
  EMPTY_STR,				/* 0x0f */
  EMPTY_STR,				/* 0x10 */
  EMPTY_STR,				/* 0x11 */
  "rec1mux",			/* 0x12 */
  "rec2mux",			/* 0x13 */
  "pcbeep",			/* 0x14 */
  "cd",				/* 0x15 */
  "mainvol",			/* 0x16 */
  "rec1vol",			/* 0x17 */
  "rec2vol",			/* 0x18 */
  "adat",			/* 0x19 */
  "i2s-out",			/* 0x1a */
  "i2s-in",			/* 0x1b */
  NULL
};

static const char *stac923xremap[] = {
  EMPTY_STR,                           /* 0 */
  EMPTY_STR,                           /* 0x01 */
  "front",                 	/* 0x02 */
  "center/LFE",            	/* 0x03 */
  "rear",                  	/* 0x04 */
  "side",                  	/* 0x05 */
  "headphone",                  /* 0x06 */
  "rec1",                       /* 0x07 */
  "rec2",                  	/* 0x08 */
  "rec3",                   	/* 0x09 */
  EMPTY_STR,                           /* 0x0a */
  EMPTY_STR,                           /* 0x0b */
  EMPTY_STR,                           /* 0x0c */
  EMPTY_STR,                           /* 0x0d */
  EMPTY_STR,                           /* 0x0e */
  EMPTY_STR,                           /* 0x0f */
  EMPTY_STR,                   	/* 0x10 */
  EMPTY_STR,                   	/* 0x11 */
  "cd",                    	/* 0x12 */
  "dig-mic1",                   /* 0x13 */
  "dig-mic2",                   /* 0x14 */
  "input1-mux",                 /* 0x15 */
  "input2-mux",                 /* 0x16 */
  "input3-mux",                 /* 0x17 */
  "rec1vol",                    /* 0x18 */
  "rec2vol",                    /* 0x19 */
  "rec3vol",                    /* 0x1a */
  "rec1-mux",                   /* 0x1b */
  "rec2-mux",                   /* 0x1c */
  "rec3-mux",                   /* 0x1d */
  "spdif-out",                  /* 0x1e */
  "adat",                     	/* 0x1f */
  NULL
};


static const char *conexant_modem_remap[] =
{
	EMPTY_STR,		/* 0x00 */
	EMPTY_STR,		/* 0x01 */
	EMPTY_STR,		/* 0x02 */
	EMPTY_STR,		/* 0x03 */
	EMPTY_STR,		/* 0x04 */
	EMPTY_STR,		/* 0x05 */
	EMPTY_STR,		/* 0x06 */
	EMPTY_STR,		/* 0x07 */
	EMPTY_STR,		/* 0x08 */
	EMPTY_STR,		/* 0x09 */
	EMPTY_STR,		/* 0x0a */
	EMPTY_STR,		/* 0x0b */
	EMPTY_STR,		/* 0x0c */
	EMPTY_STR,		/* 0x0d */
	EMPTY_STR,		/* 0x0e */
	EMPTY_STR,		/* 0x0f */
	EMPTY_STR,		/* 0x10 */
	EMPTY_STR,		/* 0x11 */
	EMPTY_STR,		/* 0x12 */
	EMPTY_STR,		/* 0x13 */
	EMPTY_STR,		/* 0x14 */
	EMPTY_STR,		/* 0x15 */
	EMPTY_STR,		/* 0x16 */
	EMPTY_STR,		/* 0x17 */
	EMPTY_STR,		/* 0x18 */
	EMPTY_STR,		/* 0x19 */
	EMPTY_STR,		/* 0x1a */
	EMPTY_STR,		/* 0x1b */
	EMPTY_STR,		/* 0x1c */
	EMPTY_STR,		/* 0x1d */
	EMPTY_STR,		/* 0x1e */
	EMPTY_STR,		/* 0x1f */
	EMPTY_STR,		/* 0x20 */
	EMPTY_STR,		/* 0x21 */
	EMPTY_STR,		/* 0x22 */
	EMPTY_STR,		/* 0x23 */
	EMPTY_STR,		/* 0x24 */
	EMPTY_STR,		/* 0x25 */
	EMPTY_STR,		/* 0x26 */
	EMPTY_STR,		/* 0x27 */
	EMPTY_STR,		/* 0x28 */
	EMPTY_STR,		/* 0x29 */
	EMPTY_STR,		/* 0x2a */
	EMPTY_STR,		/* 0x2b */
	EMPTY_STR,		/* 0x2c */
	EMPTY_STR,		/* 0x2d */
	EMPTY_STR,		/* 0x2e */
	EMPTY_STR,		/* 0x2f */
	EMPTY_STR,		/* 0x30 */
	EMPTY_STR,		/* 0x31 */
	EMPTY_STR,		/* 0x32 */
	EMPTY_STR,		/* 0x33 */
	EMPTY_STR,		/* 0x34 */
	EMPTY_STR,		/* 0x35 */
	EMPTY_STR,		/* 0x36 */
	EMPTY_STR,		/* 0x37 */
	EMPTY_STR,		/* 0x38 */
	EMPTY_STR,		/* 0x39 */
	EMPTY_STR,		/* 0x3a */
	EMPTY_STR,		/* 0x3b */
	EMPTY_STR,		/* 0x3c */
	EMPTY_STR,		/* 0x3d */
	EMPTY_STR,		/* 0x3e */
	EMPTY_STR,		/* 0x3f */
	EMPTY_STR,		/* 0x40 */
	EMPTY_STR,		/* 0x41 */
	EMPTY_STR,		/* 0x42 */
	EMPTY_STR,		/* 0x43 */
	EMPTY_STR,		/* 0x44 */
	EMPTY_STR,		/* 0x45 */
	EMPTY_STR,		/* 0x46 */
	EMPTY_STR,		/* 0x47 */
	EMPTY_STR,		/* 0x48 */
	EMPTY_STR,		/* 0x49 */
	EMPTY_STR,		/* 0x4a */
	EMPTY_STR,		/* 0x4b */
	EMPTY_STR,		/* 0x4c */
	EMPTY_STR,		/* 0x4d */
	EMPTY_STR,		/* 0x4e */
	EMPTY_STR,		/* 0x4f */
	EMPTY_STR,		/* 0x50 */
	EMPTY_STR,		/* 0x51 */
	EMPTY_STR,		/* 0x52 */
	EMPTY_STR,		/* 0x53 */
	EMPTY_STR,		/* 0x54 */
	EMPTY_STR,		/* 0x55 */
	EMPTY_STR,		/* 0x56 */
	EMPTY_STR,		/* 0x57 */
	EMPTY_STR,		/* 0x58 */
	EMPTY_STR,		/* 0x59 */
	EMPTY_STR,		/* 0x5a */
	EMPTY_STR,		/* 0x5b */
	EMPTY_STR,		/* 0x5c */
	EMPTY_STR,		/* 0x5d */
	EMPTY_STR,		/* 0x5e */
	EMPTY_STR,		/* 0x5f */
	EMPTY_STR,		/* 0x60 */
	EMPTY_STR,		/* 0x61 */
	EMPTY_STR,		/* 0x62 */
	EMPTY_STR,		/* 0x63 */
	EMPTY_STR,		/* 0x64 */
	EMPTY_STR,		/* 0x65 */
	EMPTY_STR,		/* 0x66 */
	EMPTY_STR,		/* 0x67 */
	EMPTY_STR,		/* 0x68 */
	EMPTY_STR,		/* 0x69 */
	EMPTY_STR,		/* 0x6a */
	EMPTY_STR,		/* 0x6b */
	EMPTY_STR,		/* 0x6c */
	EMPTY_STR,		/* 0x6d */
	EMPTY_STR,		/* 0x6e */
	EMPTY_STR,		/* 0x6f */
	"modem-control",/* 0x70 */ // Vendor defined widget
	"modem-in",	/* 0x71 */
	"modem-out",	/* 0x72 */
	"modem-jack",	/* 0x73 */
	NULL
};

extern int hdaudio_GPIO_init_1 (int dev, hdaudio_mixer_t * mixer, int cad, int top_group);

static const codec_desc_t codecs[] = {
  /* Realtek HDA codecs */
  {0x10ec0260, "ALC260", VF_NONE, (char **) &alc260remap},
  {0x10ec0262, "ALC262", VF_NONE, (char **) &alc262remap}, 
  {0x10ec0268, "ALC268", VF_NONE, (char **) &alc262remap}, 
  {0x10ec0298, "ALC298", VF_NONE, (char **) &alc262remap}, 
  {0x10ec0662, "ALC662", VF_NONE, (char **) &alc662remap},
  {0x10ec0663, "ALC663", VF_NONE, (char **) &alc662remap},
  {0x10ec0861, "ALC861", VF_NONE, (char **) &alc861remap},
  {0x10ec0862, "ALC862", VF_NONE, (char **) &alc861remap},
  {0x10ec0880, "ALC880", VF_ALC88X_HACK, (char **) &alc880remap}, 
  {0x10ec0882, "ALC882", VF_ALC88X_HACK, (char **) &alc880remap}, 
  {0x10ec0883, "ALC883", VF_ALC88X_HACK, (char **) &alc883remap}, 
  {0x10ec0885, "ALC885", VF_ALC88X_HACK, (char **) &alc883remap}, 
  {0x10ec0887, "ALC887", VF_ALC88X_HACK, (char **) &alc883remap}, 
  {0x10ec0888, "ALC888", VF_ALC88X_HACK, (char **) &alc883remap}, 
  {0x10ec0889, "ALC889", VF_ALC88X_HACK, (char **) &alc883remap}, 
  {0x10ec0892, "ALC892", VF_ALC88X_HACK, (char **) &alc883remap}, 
  {0x10ec0899, "ALC899", VF_ALC88X_HACK, (char **) &alc883remap},

  /* CMedia HDA codecs */
  {0x13f69880, "CMI9880", VF_NONE, (char **) &cmi9880remap},
  {0x434d4980, "CMI9880", VF_NONE, (char **) &cmi9880remap},

  {0x111d7603, "92HD75B3X5", VF_NONE, NULL, 0, hdaudio_GPIO_init_1},
  {0x111d7608, "92HD75B2X5", VF_NONE, NULL, 0, hdaudio_GPIO_init_1},
  {0x111d76b0, "92HD71B8X", VF_NONE, NULL, 0, hdaudio_GPIO_init_1},
  {0x111d76b1, "92HD71B8X", VF_NONE, NULL, 0, hdaudio_GPIO_init_1},
  {0x111d76b2, "92HD71B7X", VF_NONE, NULL, 0, hdaudio_GPIO_init_1},
  {0x111d76b3, "92HD71B7X", VF_NONE, NULL, 0, hdaudio_GPIO_init_1},
  {0x111d76b4, "92HD71B6X", VF_NONE, NULL, 0, hdaudio_GPIO_init_1},
  {0x111d76b5, "92HD71B6X", VF_NONE, NULL, 0, hdaudio_GPIO_init_1},
  {0x111d76b6, "92HD71B5X", VF_NONE, NULL, 0, hdaudio_GPIO_init_1},
  {0x111d76b7, "92HD71B5X", VF_NONE, NULL, 0, hdaudio_GPIO_init_1},

  /* Analog Devices HDA codecs */
  {0x11d41981, "AD1981", VF_NONE, (char **) &ad1981remap, 0x76543021},
  {0x11d41983, "AD1983", VF_NONE, (char **) &ad1983remap, 0x76543021},
  {0x11d41984, "AD1984", VF_NONE, (char **) &ad1984remap, 0x76543012},
  {0x11d41986, "AD1986A", VF_NONE, (char **) &ad1986remap, 0x76540321},
  {0x11d41988, "AD1988A", VF_NONE, (char **) &ad1988remap, 0x76015432},
  {0x11d4198b, "AD1988B", VF_NONE, (char **) &ad1988remap, 0x76015432},

  /* Sigmatel HDA codecs (some of them) */
  {0x83847690, "STAC9200", VF_NONE, (char **) &stac920xremap },
  {0x838476a0, "STAC9205", VF_NONE, (char **) &stac920xremap },
  {0x838476a1, "STAC9205D", VF_NONE, (char **) &stac920xremap },
  {0x838476a2, "STAC9204", VF_NONE, (char **) &stac920xremap },
  {0x838476a3, "STAC9204D", VF_NONE, (char **) &stac920xremap },
  
  /* Apple Macbook ids */
  {0x83847880, "STAC9220 A1", VF_NONE, (char **) &stac922xremap },
  {0x83847882, "STAC9220 A2", VF_NONE, (char **) &stac922xremap },
  {0x83847680, "STAC9221 A1", VF_NONE, (char **) &stac922xremap },

  {0x83847681, "STAC9220D", VF_NONE, (char **) &stac922xremap },
  {0x83847682, "STAC9221", VF_NONE, (char **) &stac922xremap },
  {0x83847683, "STAC9221D", VF_NONE, (char **) &stac922xremap },

  {0x83847610, "STAC9230XN", VF_NONE, (char **) &stac923xremap },
  {0x83847611, "STAC9230DN", VF_NONE, (char **) &stac923xremap },
  {0x83847612, "STAC9230XT", VF_NONE, (char **) &stac923xremap },
  {0x83847613, "STAC9230DT", VF_NONE, (char **) &stac923xremap },
  {0x83847614, "STAC9229X", VF_NONE, (char **) &stac923xremap },
  {0x83847615, "STAC9229D", VF_NONE, (char **) &stac923xremap },
  {0x83847616, "STAC9228X", VF_NONE, (char **) &stac923xremap },
  {0x83847617, "STAC9228D", VF_NONE, (char **) &stac923xremap },
  {0x83847618, "STAC9227X", VF_NONE, (char **) &stac923xremap }, /* Intel D975XBX2 (at least) */
  {0x83847619, "STAC9227D", VF_NONE, (char **) &stac923xremap },

  {0x838476a4, "STAC9255", VF_NONE, (char **) &stac925xremap },
  {0x838476a5, "STAC9255D", VF_NONE, (char **) &stac925xremap },
  {0x838476a6, "STAC9254", VF_NONE, (char **) &stac925xremap },
  {0x838476a7, "STAC9254D", VF_NONE, (char **) &stac925xremap },

  {0x83847620, "STAC9274", VF_NONE, (char **) &stac923xremap },
  {0x83847621, "STAC9274D", VF_NONE, (char **) &stac923xremap },
  {0x83847622, "STAC9273X", VF_NONE, (char **) &stac923xremap },
  {0x83847623, "STAC9273D", VF_NONE, (char **) &stac923xremap },
  {0x83847624, "STAC9272X", VF_NONE, (char **) &stac923xremap },
  {0x83847625, "STAC9272D", VF_NONE, (char **) &stac923xremap },
  {0x83847626, "STAC9271X", VF_NONE, (char **) &stac923xremap },
  {0x83847627, "STAC9271D", VF_NONE, (char **) &stac923xremap },

  {0x83847628, "STAC9274X5NH", VF_NONE, (char **) &stac923xremap },
  {0x83847629, "STAC9274D5NH", VF_NONE, (char **) &stac923xremap },
  {0x83847661, "CXD9872RD", VF_NONE, NULL, 0x76543012},
  {0x83847662, "STAC9872AK", VF_NONE, NULL, 0x76543012},
  {0x83847664, "STAC9872K", VF_NONE, NULL, 0x76543210}, /* Vaio VGN-AR51J */

  /* Conexant */
  {0x14f15045, "CX20548", VF_NONE, NULL, 0x76543201},
  {0x14f15047, "CX20551", VF_NONE, NULL, 0x76543201},
  {0x14f15051, "CX20561", VF_NONE, NULL, 0x76543210},
  {0x14f12c06, "Conexant2c06", VF_NONE, (char **) &conexant_modem_remap, 0, NULL_mixer_init}, /* Modem codec (Vaio) */
  {0x14f12bfa, "Conexant2bfa", VF_NONE, (char **) &conexant_modem_remap, 0, NULL_mixer_init}, /* Modem codec (Acer Ferrari 5k) */

  /* Si3055 and compatible modems */
  {0x163c3055, "Si3055", VF_SI3055_HACK, NULL, 0, NULL_mixer_init },
  {0x163c3155, "Si3155", VF_SI3055_HACK, NULL, 0, NULL_mixer_init },
  {0x11c13026, "Agere3026", VF_SI3055_HACK, NULL, 0, NULL_mixer_init },
  {0x11c13055, "Agere3055", VF_SI3055_HACK, NULL, 0, NULL_mixer_init },
  {0x11c13155, "Agere3155", VF_SI3055_HACK, NULL, 0, NULL_mixer_init },
  {0x10573055, "Motorola3055", VF_SI3055_HACK, NULL, 0, NULL_mixer_init },
  {0x10573057, "Motorola3057", VF_SI3055_HACK, NULL, 0, NULL_mixer_init },
  {0x10573155, "Motorola3155", VF_SI3055_HACK, NULL, 0, NULL_mixer_init },

  /* Creative Labs */
  {0x1102000a, "Createive XFi XTreme", VF_NONE, NULL, 0x76543210},

  {0x11c11040, "Agere HDA Modem", VF_NONE, NULL, 0, NULL_mixer_init}, 	

  /* Unknown */
  {0, "Unknown"}
};

static const char *abit_AA8_remap[] = {
  EMPTY_STR,				/* 0 */
  EMPTY_STR,				/* 1 */
  "front",			/* 2 */
  "rear",			/* 3 */
  "center/LFE",			/* 4 */
  "side",			/* 5 */
  "spdif-out",			/* 6 */
  "rec1",			/* 7 */
  "rec2",			/* 8 */
  "rec3",			/* 9 */
  "spdif-in",			/* 10 */
  "inputmix",			/* 11 */
  "front",			/* 12 */
  "rear",			/* 13 */
  "center/LFE",			/* 14 */
  "side",			/* 15 */
  "out-source",			/* 16 */
  "out-source",			/* 17 */
  "out-source",			/* 18 */
  "out-source",			/* 19 */
  "green1",			/* 20 */
  "black1",			/* 21 */
  "C-L",			/* 22 */
  "surr",			/* 23 */
  "pink1",			/* 24 */
  "pink2",			/* 25 */
  "blue1",			/* 26 */
  "blue2",			/* 27 */
  "cd",				/* 28 */
  "beep",			/* 29 */
  "spdout",			/* 30 */
  "spdin",			/* 31 */
  "vendor",			/* 32 */
  "vol",			/* 33 */
  NULL
};

static const char *vaio_remap[] = {
	EMPTY_STR,		/* 0x00 */
	EMPTY_STR,		/* 0x01 */
	"headphone",	/* 0x02 */
	"pcm",		/* 0x03 */ // Unused
	"pcm",		/* 0x04 */ // Unused
	"speaker",	/* 0x05 */
	"rec1",		/* 0x06 */
	"rec1-vol",	/* 0x07 */
	"rec",		/* 0x08 */
	"rec-vol",	/* 0x09 */
	"headphone",	/* 0x0a */
	EMPTY_STR,		/* 0x0b */ // Unused
	EMPTY_STR,		/* 0x0c */ // Unused
	"mic",		/* 0x0d */
	EMPTY_STR,		/* 0x0e */
	"int-speaker",	/* 0x0f */
	"spdifout1",	/* 0x10 */
	"int-digout1",	/* 0x11 */
	"spdifin",	/* 0x12 */
	"int-digout",	/* 0x13 */
	"int-mic",	/* 0x14 */
	"rec",		/* 0x15 */
	"beep",		/* 0x16 */
	"vol",		/* 0x17 */
	"spdifout",	/* 0x18 */
	NULL
};

static const char *alc262_vaio_remap[] =
{
        EMPTY_STR,             /* 0x00 */
        EMPTY_STR,             /* 0x01 */
        "speaker",              /* 0x02 */
        "headphone",            /* 0x03 */
        "vendor",               /* 0x04 */
        "vendor",               /* 0x05 */
        "hdmi-out",             /* 0x06 */
        "rec1",         /* 0x07 */
        "rec2",         /* 0x08 */
        "rec3",         /* 0x09 */
        "spdif-in",             /* 0x0a */
        "mix",          /* 0x0b */
        "mix",          /* 0x0c */
        "mix",          /* 0x0d */
        "mono",         /* 0x0e */
        "vendor",               /* 0x0f */
        "vendor",               /* 0x10 */
        "mono",         /* 0x11 */
        "dmic",         /* 0x12 */
        "vendor",               /* 0x13 */
        "line-out",             /* 0x14 */
        "headphone",            /* 0x15 */
        "mono",         /* 0x16 */
        "beep",         /* 0x17 */
        "speaker",              /* 0x18 */
        "speaker",              /* 0x19 */
        "speaker",              /* 0x1a */
        "speaker",              /* 0x1b */
        "speaker",              /* 0x1c */
        "beep",         /* 0x1d */
        "spdif-out",            /* 0x1e */
        "spdif-in",             /* 0x1f */
        "vendor",               /* 0x20 */
        "vol",          /* 0x21 */
        "rec3",         /* 0x22 */
        "rec2",         /* 0x23 */
        "rec1",         /* 0x24 */
	NULL
};

/*
 * Table for subsystem ID's that require special handling
 */

extern int hdaudio_Asus_P4B_E_mixer_init (int dev, hdaudio_mixer_t * mixer, int cad, int top_group);
extern int hdaudio_scaleoP_mixer_init (int dev, hdaudio_mixer_t * mixer, int cad, int top_group);
extern int hdaudio_abit_AA8_mixer_init (int dev, hdaudio_mixer_t * mixer, int cad, int top_group);
extern int hdaudio_ferrari5k_mixer_init (int dev, hdaudio_mixer_t * mixer, int cad, int top_group);
extern int hdaudio_vaio_vgn_mixer_init (int dev, hdaudio_mixer_t * mixer, int cad, int top_group);
extern int hdaudio_thinkpad_r61_mixer_init (int dev, hdaudio_mixer_t * mixer, int cad, int top_group);
extern int hdaudio_mac_sigmatel_GPIO_init (int dev, hdaudio_mixer_t * mixer, int cad, int top_group);
extern int hdaudio_mac_realtek_GPIO_init (int dev, hdaudio_mixer_t * mixer, int cad, int top_group);
extern int hdaudio_eeepc_mixer_init (int dev, hdaudio_mixer_t * mixer, int cad, int top_group);
extern int hdaudio_asus_a7k_GPIO_init (int dev, hdaudio_mixer_t * mixer, int cad, int top_group);
extern int hdaudio_asus_m9_mixer_init (int dev, hdaudio_mixer_t * mixer, int cad, int top_group);

static const codec_desc_t subdevices[] = {
  {0x98801019, "ECS 915P-A", VF_NONE, NULL, 0x76541320},
  {0x104381e1, "Asus P4B-E/AD1988A", VF_NONE, (char **) &ad1988remap, 0x76015432, hdaudio_Asus_P4B_E_mixer_init},
  // {0x1043e601, "ScaleoP/ALC888", VF_ALC88X_HACK, (char **) &alc883remap, 0, hdaudio_scaleoP_mixer_init}, 

  /* Abit AA8 (at least some revisions) have bogus codec config information,
   * including the subvendor ID. 0x08800000 is used by many other motherboards
   * too. Have to use the pci_subvendor field for alc880 based devices:
   * 	0x147b1039 = Abit AA8 motherboard.
   * 	0x15849077 = Rock Pegasus 665 laptop.
   */
  {0x08800000, "Abit AA8/ALC880", VF_ALC88X_HACK, (char **) &abit_AA8_remap, 0, hdaudio_abit_AA8_mixer_init, 0, 0x147b1039},

  {0x10250000, "Ferrari5k/ALC883", VF_ALC88X_HACK, (char **) &alc883remap, 0, hdaudio_ferrari5k_mixer_init, 0x10ec0883, 0x1025010a},
  {0x10250000, "Acer_aspire5052/ALC883", VF_ALC88X_HACK, (char **) &alc883remap, 0, hdaudio_ferrari5k_mixer_init, 0x10ec0883, 0x1025010f},
  {0x1025160d, "Acer_travelmate4060/ALC260", VF_NONE, (char **) &alc260remap, 0, hdaudio_GPIO_init_1, 0x10ec0260, 0x1025008f},
  {0x10431153, "Asus M9", VF_NONE, (char **) &ad1986remap, 0x76540321, hdaudio_asus_m9_mixer_init},

  /*
   **** Sony Vaio VGN-AR51 ***
   * Has three codecs. Primary (Stac9872K), Modem (Conexant) and
   * HDMI digital output (ALC262). Disable the mixer entries for codecs 2 and 3.
   */
  {0x104d2200, "Vaio/STAC9872K", VF_VAIO_HACK, (char**) &vaio_remap, 0x76540213, hdaudio_vaio_vgn_mixer_init, 0x83847664, 0x104d9016},
  /* 2nd codec (#1) is "Conexant2c06" modem */
  {0x104d2200, "Vaio/HDMI", VF_NONE, (char **) &alc262_vaio_remap, 0, NULL_mixer_init, 0x10ec0262, 0x104d9016}, 
  /****/

  /*
   * Sony VAIO SZ2, SZ3, FE and FE31B
   */
  {0x104d0700, "Vaio/CXD9872RD", VF_VAIO_HACK, (char**) &vaio_remap, 0x76540213, hdaudio_vaio_vgn_mixer_init, 0x83847661, 0x104d81e6},
  {0x104d1000, "Vaio/CXD9872RD", VF_VAIO_HACK, (char**) &vaio_remap, 0x76540213, hdaudio_vaio_vgn_mixer_init, 0x83847661, 0x104d81ef},
  {0x104d0c00, "Vaio/CXD9872RD", VF_VAIO_HACK, (char**) &vaio_remap, 0x76540213, hdaudio_vaio_vgn_mixer_init, 0x83847661, 0x104d81ef},

  /*
   * Sony VAIO AR
   */
  {0x104d1300, "Vaio/CXD9872AKD", VF_VAIO_HACK, (char**) &vaio_remap, 0x76540213, hdaudio_vaio_vgn_mixer_init, 0x83847664, 0x104d81fd},

  /*
   * Sony Vaio SZ (SZ650) has two codecs, STAC9872AK and Conexant modem.
   * Assume the audio codec is identical with Vaio AGN (above).
   */
  {0x104d1e00, "Vaio/STAC9872AK", VF_VAIO_HACK, (char**) &vaio_remap, 0x76540213, hdaudio_vaio_vgn_mixer_init, 0x83847662, 0x104d9008},
  /* Vaio VGC-LA1 */
  {0x104d1200, "Vaio/STAC9872AK", VF_VAIO_HACK, (char**) &vaio_remap, 0x76540213, hdaudio_vaio_vgn_mixer_init, 0x83847662, 0x104d8205},

/*
 * Thinkpad R61
 */
  {0x17aa20bb, "Thinkpad R61", VF_NONE, (char**) &ad1984remap, 0, hdaudio_thinkpad_r61_mixer_init},

/*
 * Asus Eee PC (model 900 at least)
 */
  {0x10438337, "Asus Eee PC", VF_NONE, NULL, 0, hdaudio_eeepc_mixer_init},

 /*
  * Asus A7K
  */
  {0x10431339, "Asus A7K", VF_ALC88X_HACK, (char **) &alc880remap, 0, hdaudio_asus_a7k_GPIO_init}, // ALC660

/*
 * Known Macintosh systems
 */
  {0x106b0800, "Intel Mac V1", VF_NONE, (char **) &stac922xremap, 0, hdaudio_mac_sigmatel_GPIO_init},
  {0x106b0700, "Intel Mac V2", VF_NONE, (char **) &stac922xremap, 0, hdaudio_mac_sigmatel_GPIO_init},
  {0x106b0600, "Intel Mac V2", VF_NONE, (char **) &stac922xremap, 0, hdaudio_mac_sigmatel_GPIO_init},
  {0x106b0200, "Intel Mac V3", VF_NONE, (char **) &stac922xremap, 0, hdaudio_mac_sigmatel_GPIO_init},
  {0x106b0e00, "Intel Mac V3", VF_NONE, (char **) &stac922xremap, 0, hdaudio_mac_sigmatel_GPIO_init},
  {0x106b0f00, "Intel Mac V3", VF_NONE, (char **) &stac922xremap, 0, hdaudio_mac_sigmatel_GPIO_init},
  {0x106b1600, "Intel Mac V3", VF_NONE, (char **) &stac922xremap, 0, hdaudio_mac_sigmatel_GPIO_init},
  {0x106b1700, "Intel Mac V3", VF_NONE, (char **) &stac922xremap, 0, hdaudio_mac_sigmatel_GPIO_init},
  {0x106b1e00, "Intel Mac V3", VF_NONE, (char **) &stac922xremap, 0, hdaudio_mac_sigmatel_GPIO_init},
  {0x106b1a00, "Intel Mac V4", VF_NONE, (char **) &stac922xremap, 0, hdaudio_mac_sigmatel_GPIO_init},
  // {0x00000100, "Intel Mac V4", VF_NONE, (char **) &stac922xremap, ?, hdaudio_mac_sigmatel_GPIO_init},
  {0x106b0a00, "Intel Mac V5", VF_NONE, (char **) &stac922xremap, 0, hdaudio_mac_sigmatel_GPIO_init},
  {0x106b2200, "Intel Mac V5", VF_NONE, (char **) &stac922xremap, 0, hdaudio_mac_sigmatel_GPIO_init},
  {0x106b3200, "Intel iMac", VF_ALC88X_HACK, (char **) &alc883remap, 0, hdaudio_mac_realtek_GPIO_init}, // ALC885

  {0, "Unknown"}
};

