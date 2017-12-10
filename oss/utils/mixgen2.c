/*
 * Temporary utility used to develop the next hdaudio driver
 * version. Not usable.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <soundcard.h>
#include <stdarg.h>

#define DRIVER_NICK "hdaudio"
#define CE_CONT	0
#define CE_NOTE	1
#define CE_WARN	2

#define DDB(x)

#define PMALLOC(osdev, s)	malloc(s)

typedef int sound_os_info, mixer_create_controls_t;

typedef struct
{
  char *hw_info;
} oss_device_t;

#include <hdaudio.h>

#define ioctl_arg int
#define mixer_ext_init_fn int
#include <hdaudio_codec.h>
//#include <hdaudio_codecids.h>

int fd;

#undef corb_read
#undef corb_write

typedef struct
{
	int association, sequence;
	int nwidgets;
	int jack_count;

#define PATH_MAXWID	8
	widget_t *widgets[PATH_MAXWID];
} path_t;

#define MAX_PATHS	32
path_t *paths[MAX_PATHS];
int npaths=0;

int
corb_write (void *dc, unsigned int cad, unsigned int nid, unsigned int d,
	    unsigned int verb, unsigned int parm)
{
  unsigned int tmp;

  tmp = (cad << 28) | (d << 27) | (nid << 20) | (verb << 8) | parm;

  if (ioctl (fd, HDA_IOCTL_WRITE, &tmp) == -1)
    {
      perror ("HDA_IOCTL_WRITE");
      return 0;
    }

  return 1;
}

static int
corb_read (void *dc, unsigned int cad, unsigned int nid, unsigned int d,
	   unsigned int verb, unsigned int parm, unsigned int *upper,
	   unsigned int *lower)
{
  unsigned int tmp;

  tmp = (cad << 28) | (d << 27) | (nid << 20) | (verb << 8) | parm;

  if (ioctl (fd, HDA_IOCTL_READ, &tmp) == -1)
    {
      /* perror("HDA_IOCTL_READ"); */
      if (errno == EINVAL)
	{
	  fprintf (stderr, "hdaudio_snoopy mode is not available\n");
	  exit (-1);
	}
      return 0;
    }

  *upper = tmp;
  *lower = 0;

  return 1;
}

void
cmn_err (int level, char *s, ...)
{
  char tmp[1024], *a[6];
  va_list ap;
  int i, n = 0;

  va_start (ap, s);

  for (i = 0; i < strlen (s); i++)
    if (s[i] == '%')
      n++;

  for (i = 0; i < n && i < 6; i++)
    {
      a[i] = va_arg (ap, char *);
    }

  for (i = n; i < 6; i++)
    a[i] = NULL;

  if (level == CE_CONT)
    {
      sprintf (tmp, s, a[0], a[1], a[2], a[3], a[4], a[5], NULL,
	       NULL, NULL, NULL);
      printf ("%s", tmp);
    }
  else
    {
      strcpy (tmp, DRIVER_NICK ": ");

      sprintf (tmp + strlen (tmp), s, a[0], a[1], a[2], a[3], a[4], a[5],
	       NULL, NULL, NULL, NULL);
      printf ("%s", tmp);
    }

  va_end (ap);
}

/*
 ********************************
 */

static const char *widget_id[16] = {
    "pcm",
    "rec",
    "mix",
    "select",
    "jack",
    "power",
    "vol",
    "beep",
    "unkn",
    "unkn",
    "unkn",
    "unkn",
    "unkn",
    "unkn",
    "unkn",
    "vendor"
  };

static int
attach_pin_widget (hdaudio_mixer_t * mixer, codec_t * codec,
		   widget_t * widget, int cad, int wid, int group_type)
{
  unsigned int conf;
  int num = codec->jack_number++;
  unsigned int pincaps, b;
  int association, sequence;
  int default_device;
  int default_loc;
  int conn;

  int color;
  int no_color = 0;
  char *name = NULL, *loc = "", *color_name = NULL;

  if (!corb_read (mixer, cad, wid, 0, GET_CONFIG_DEFAULT, 0, &conf, &b))
    return 0;

  default_device = (conf >> 20) & 0x0f;
  default_loc = (conf >> 24) & 0x3f;
  conn = (conf >> 30) & 0x03;

  if (!corb_read (mixer, cad, wid, 0,
		  GET_PARAMETER, HDA_PIN_CAPS, &pincaps, &b))
    {
      cmn_err (CE_WARN, "GET_PARAMETER HDA_PIN_CAPS failed\n");
      return 0;
    }

  widget->pincaps = pincaps;

  if (conf == 0)
    {
      cmn_err (CE_WARN,
	       "CONFIG_DEFAULT information not provided by BIOS for cad=%d, wid=%02x\n",
	       cad, wid);
      cmn_err (CE_CONT, "Cannot work in this system.\n");
      return 0;
    }

  color = (conf >> 12) & 0x0f;

  association = (conf>>4) & 0xf;
  sequence = conf & 0xf;

  if (pincaps & (1 << 6))
    cmn_err (CE_WARN, "Balanced I/O not supported\n");
  if (!(pincaps & (1 << 5)))	/* No input */
    widget->pin_type = PIN_OUT;
  if (!(pincaps & (1 << 4)))
    widget->pin_type = PIN_IN;

  DDB (cmn_err (CE_CONT, "\tConfig default %08x\n", conf));

  if ((default_loc & 0x0f) == 0x1)	/* Rear panel - default */
    loc = "";
  if ((default_loc & 0x0f) == 0x2)	/* Front panel */
    loc = "fp-";
  if ((default_loc & 0xf0) == 0x10)	/* Internal func - eg cd/tad/spk */
    loc = "int-";

  if (conn == 1)		/* Pin not connected to anything */
    {
      widget->skip = 1;
      widget->skip_output = 1;
    }

  widget->association = association;
  widget->sequence = sequence;

  switch (default_device)
    {
    case 0x0:
      name = "lineout";
      widget->pin_type = PIN_OUT;
      break;
    case 0x1:
      name = "speaker";
      no_color = 1;
      widget->pin_type = PIN_OUT;
      break;
    case 0x2:
      name = "headphone";
      widget->pin_type = PIN_OUT;
      break;
    case 0x3:
      name = "cd";
      no_color = 1;
      widget->pin_type = PIN_IN;
      break;
    case 0x4:
      name = "spdifout";
      no_color = 1;
      widget->pin_type = PIN_OUT;
      break;
    case 0x5:
      name = "digout";
      no_color = 1;
      widget->pin_type = PIN_OUT;
      break;
    case 0x6:
      name = "modem";
      no_color = 1;
      break;
    case 0x7:
      name = "phone";
      no_color = 1;
      break;
    case 0x8:
      name = "linein";
      widget->pin_type = PIN_IN;
      break;
    case 0x9:
      name = "aux";
      break;
    case 0xa:
      name = "mic";
      widget->pin_type = PIN_MIC;
      break;
    case 0xb:
      name = "telephony";
      no_color = 1;
      break;
    case 0xc:
      name = "spdifin";
      no_color = 1;
      break;
    case 0xd:
      name = "digin";
      no_color = 1;
      break;
    case 0xe:
      name = "reserved";
      no_color = 1;
      break;
    case 0xf:			/* Unused pin widget */
      widget->skip = 1;
      widget->skip_output = 1;
      break;
    }

/* process only colored jacks and skip fixed function jacks */
  switch (color)
    {
    case 0x1:
      color_name = "black";
      widget->rgbcolor = OSS_RGB_BLACK;
      break;
    case 0x2:
      color_name = "gray";
      widget->rgbcolor = OSS_RGB_GRAY;
      break;
    case 0x3:
      color_name = "blue";
      widget->rgbcolor = OSS_RGB_BLUE;
      break;
    case 0x4:
      color_name = "green";
      widget->rgbcolor = OSS_RGB_GREEN;
      break;
    case 0x5:
      color_name = "red";
      widget->rgbcolor = OSS_RGB_RED;
      break;
    case 0x6:
      color_name = "orange";
      widget->rgbcolor = OSS_RGB_ORANGE;
      break;
    case 0x7:
      color_name = "yellow";
      widget->rgbcolor = OSS_RGB_YELLOW;
      break;
    case 0x8:
      color_name = "purple";
      widget->rgbcolor = OSS_RGB_PURPLE;
      break;
    case 0x9:
      color_name = "pink";
      widget->rgbcolor = OSS_RGB_PINK;
      break;
    case 0xe:
      color_name = "white";
      widget->rgbcolor = OSS_RGB_WHITE;
      break;

    default:
      if (name != NULL)
	color_name = name;
      else
	color_name = "internal";
    }

  if (no_color)
    widget->rgbcolor = 0;

  if (default_device == 0xf)	/* Not present */
    {
      widget->rgbcolor = 0;
      color_name = "internal";
    }

  sprintf (widget->color, "%s%s", loc, color_name);

  if (name == NULL || default_device == 0x00)
    name = color_name;
  sprintf (widget->name, "%s%s", loc, name);

  DDB(cmn_err(CE_CONT, "\tJack name %s, color %s (%06x)\n", 
			  widget->name, widget->color, widget->rgbcolor));
  DDB(cmn_err(CE_CONT, "\tAssociation %x, sequence %x\n", association, sequence));

printf("Widget %02x: default_device=%d, name=%s, color=%s\n", wid, default_device, widget->name, widget->color);
  return 1;
}

static int
attach_widget (hdaudio_mixer_t * mixer, int cad, int wid, int group_type)
{
  static const char *widget_types[16] = {
    "Audio output",
    "Audio input",
    "Audio mixer",
    "Audio selector",
    "Pin complex",
    "Power widget",
    "Volume knob",
    "Beep generator",
    "Reserved8",
    "Reserved9",
    "ReservedA",
    "ReservedB",
    "ReservedC",
    "ReservedD",
    "ReservedE",
    "Vendor defined audio"
  };

  static const int bit_sizes[] = {
    8,
    16,
    20,
    24,
    32
  };

  static const unsigned int bit_fmts[] = {
    AFMT_U8,
    AFMT_S16_LE,
    AFMT_S32_LE,
    AFMT_S32_LE,
    AFMT_S32_LE
  };

  static const int srates[] = {
    8000,
    11025,
    16000,
    22050,
    32000,
    44100,
    48000,
    88200,
    96000,
    176400,
    192000,
    384000
  };

  unsigned int widget_caps, b, pstate;
  unsigned int inamp_caps, outamp_caps;
  int wid_type;
  int i;

  codec_t *codec = mixer->codecs[cad];
  widget_t *widget;

  if (codec == NULL)
    return 0;

  if (wid >= MAX_WIDGETS)
    {
      cmn_err (CE_WARN, "Too many widgets for codec %d (%d/%d)\n", cad, wid,
	       MAX_WIDGETS);
      return 0;
    }

  mixer->ncontrols++;

  codec->nwidgets = wid + 1;
  widget = &codec->widgets[wid];

  widget->cad = cad;
  widget->wid = wid;

  widget->rgbcolor = 0;

  widget->group_type = group_type;

  DDB (cmn_err (CE_CONT, "   * Widget %02x, type %d\n", wid, group_type));

  if (!corb_read
      (mixer, cad, wid, 0, GET_PARAMETER, HDA_WIDGET_CAPS, &widget_caps, &b))
    {
      cmn_err (CE_WARN, "GET_PARAMETER HDA_WIDGET_CAPS failed\n");
      return 0;
    }

  if (widget_caps & WCAP_AMP_CAP_OVERRIDE)	/* Amp param override? */
    {
      if (!corb_read (mixer, widget->cad, widget->wid, 0,
		      GET_PARAMETER, HDA_OUTPUTAMP_CAPS, &outamp_caps, &b))
	{
	  cmn_err (CE_WARN, "GET_PARAMETER HDA_OUTPUTAMP_CAPS failed\n");
	  return -EIO;
	}
      widget->outamp_caps = outamp_caps;

      if (!corb_read (mixer, widget->cad, widget->wid, 0,
		      GET_PARAMETER, HDA_INPUTAMP_CAPS, &inamp_caps, &b))
	{
	  cmn_err (CE_WARN, "GET_PARAMETER HDA_INPUTAMP_CAPS failed\n");
	  return -EIO;
	}
      widget->inamp_caps = inamp_caps;
    }
  else
    {
      widget->outamp_caps = outamp_caps = codec->default_outamp_caps;
      widget->inamp_caps = inamp_caps = codec->default_inamp_caps;
    }

  if (!corb_read (mixer, cad, wid, 0, GET_POWER_STATE, 0, &pstate, &b))
    return 0;

  /* power up each of the widgets if there is a Power Capability (1<<10) */
  if (widget_caps & WCAP_POWER_CTL)
    corb_write (mixer, cad, wid, 0, SET_POWER_STATE, 0);

  widget->widget_caps = widget_caps;

  wid_type = (widget_caps >> 20) & 0x0f;
  DDB (cmn_err
       (CE_CONT, "\tWidget type %d (%s)(%s)\n", wid_type,
	widget_types[wid_type], widget_id[wid_type]));
  DDB (cmn_err (CE_CONT, "\tPower State %d\n", pstate));

  if (widget_caps & WCAP_CONN_LIST)
    {
      unsigned int clen;
      /* Handle connection list */

      if (!corb_read
	  (mixer, cad, wid, 0, GET_PARAMETER, HDA_CONNLIST_LEN, &clen, &b))
	{
	  cmn_err (CE_WARN, "GET_PARAMETER HDA_CONNLIST_LEN failed\n");
	  return 0;
	}

      if (clen & 0x80)
	{
	  cmn_err (CE_WARN, "Long form connection list not supported\n");
	  return 0;
	}

      if (clen > 0)
	{
	  if (clen > MAX_CONN)
	    {
	      cmn_err (CE_WARN, "Too many connections\n");
	      return 0;
	    }

	  DDB (cmn_err (CE_CONT, "\tConn list (%d): ", clen));

	  for (i = 0; i < clen; i += 4)
	    {
	      int j;
	      unsigned int a;

	      if (!corb_read
		  (mixer, cad, wid, 0, GET_CONNECTION_LIST_ENTRY, i, &a, &b))
		{
		  cmn_err (CE_WARN, "GET_CONNECTION_LIST_ENTRY failed\n");
		  return 0;
		}

	      for (j = 0; j < 4 && (i + j) < clen; j++)
		{
		  int v, is_range = 0;

		  if (widget->nconn >= MAX_CONN)
		    {
		      cmn_err (CE_WARN,
			       "Too many connections for widget %d (%d)\n",
			       widget->wid, widget->nconn);
		      break;
		    }

		  v = (a >> (j * 8)) & 0xff;
		  DDB (cmn_err (CE_CONT, "%d ", v));

		  if (v & 0x80)
		    {
		      is_range = 1;
		      v &= ~0x80;
		    }

		  if (v < 0 || v >= MAX_WIDGETS)
		    {
		      cmn_err (CE_NOTE,
			       "Connection %d for widget %d is out of range (%d) - Skipped\n",
			       j, widget->wid, v);
		      continue;
		    }

		  if (is_range)	/* Range: prev...v */
		    {
		      int x;
		      codec->widgets[v].references[codec->widgets[v].
						   refcount++] = wid;

		      if (widget->nconn < 1)
			{
			  cmn_err (CE_CONT,
				   "Bad range connection for widget %d\n",
				   widget->wid);
			  continue;
			}

		      for (x = widget->connections[widget->nconn - 1] + 1;
			   x <= v; x++)
			{
			  if (widget->nconn >= MAX_CONN)
			    {
			      cmn_err (CE_WARN,
				       "Too many connectionsi(B) for widget %d (%d)\n",
				       widget->wid, widget->nconn);
			      break;
			    }

			  widget->connections[widget->nconn++] = x;
			  codec->widgets[x].references[codec->widgets[x].
						       refcount++] = wid;
			}
		    }
		  else
		    {
		      widget->connections[widget->nconn++] = v;
		      codec->widgets[v].references[codec->widgets[v].
						   refcount++] = wid;
		    }
		}
	    }

	  DDB (cmn_err (CE_CONT, "\n"));
	}
    }

  widget->wid_type = wid_type;
  strcpy (widget->name, widget_id[wid_type]);

/*
 * Handle widget based on its type.
 */

  switch (wid_type)
    {
    case NT_PIN:
      if (!attach_pin_widget (mixer, codec, widget, cad, wid, group_type))
	return 0;
      break;
    }

  return 1;
}

 /*ARGSUSED*/ static int
attach_function_group (hdaudio_mixer_t * mixer, int cad, int wid,
		       int group_type)
{
  unsigned int a, b, gt;
  int i, first_node, num_nodes;
  codec_t *codec = mixer->codecs[cad];

  if (codec == NULL)
    return 0;
  cmn_err (CE_CONT, "Attach function group, cad=%02x, wid=%02x\n", cad, wid);

  if (!corb_read (mixer, cad, wid, 0,
		  GET_PARAMETER, HDA_OUTPUTAMP_CAPS,
		  &codec->default_outamp_caps, &b))
    {
      cmn_err (CE_WARN, "GET_PARAMETER HDA_OUTPUTAMP_CAPS failed\n");
      return 0;
    }

  if (!corb_read (mixer, cad, wid, 0,
		  GET_PARAMETER, HDA_INPUTAMP_CAPS,
		  &codec->default_inamp_caps, &b))
    {
      cmn_err (CE_WARN, "GET_PARAMETER HDA_INPUTTAMP_CAPS failed\n");
      return 0;
    }

  if (!corb_read (mixer, cad, wid, 0, GET_PARAMETER, HDA_NODE_COUNT, &a, &b))
    {
      cmn_err (CE_WARN, "GET_PARAMETER HDA_NODE_COUNT2 failed\n");
      return 0;
    }

  first_node = (a >> 16) & 0xff;
  num_nodes = a & 0xff;

  corb_read (mixer, cad, wid, 0, GET_PARAMETER, HDA_GROUP_TYPE, &gt, &b);
  gt &= 0xff;

  DDB (cmn_err
       (CE_CONT,
	" * Function group %d First node %d, num nodes %d, group type %x\n",
	wid, first_node, num_nodes, gt));
/*
 * Ignore other than audio function groups. Codecs probably allocate
 * higher widget number for the modem group than the audio group. So in this
 * way we can have smaller MAX_WIDGETS which in turn conserves memory.
 */
  if (gt != group_type)
    return 0;

  if (corb_read (mixer, cad, wid, 0, GET_PARAMETER, HDA_PCM_SIZES, &a, &b))
    {
      codec->sizes = a;
    }

  if (first_node > 0)
    for (i = first_node; i < first_node + num_nodes; i++)
      if (!attach_widget (mixer, cad, i, gt))
	return 0;

  if (num_nodes >= 1)
    codec->active = 1;
  return 1;
}
static int
attach_codec (hdaudio_mixer_t * mixer, int cad, char *hw_info,
	      unsigned int pci_subdevice, int group_type)
{
  unsigned int a, b, x;
  int i;
  int first_node, num_nodes;
  int has_audio_group = 0;
  codec_t *codec;

  if (cad >= MAX_CODECS)
    {
      cmn_err (CE_WARN, "attach_codec: Too many codecs %d\n", cad);
      return -EIO;
    }

  mixer->ncodecs = cad + 1;

  if (mixer->codecs[cad] == NULL)
    {
      if ((codec = PMALLOC (mixer->osdev, sizeof (*codec))) == NULL)
	{
	  cmn_err (CE_CONT, "Cannot allocate codec descriptor\n");
	  return -ENOMEM;
	}

      memset (codec, 0, sizeof (*codec));

      mixer->codecs[cad] = codec;
    }
  else
    {
      codec = mixer->codecs[cad];
    }

  corb_write (mixer, cad, 0, 0, SET_POWER_STATE, 0);	/* Power up everything */

  if (!corb_read (mixer, cad, 0, 0, GET_PARAMETER, HDA_VENDOR, &a, &b))
    {
      if (group_type == 1)
	{
	  sprintf (hw_info, " Codec %2d: Not present\n", cad);
	  cmn_err (CE_NOTE,
		   "attach_codec: Codec #%d is not physically present\n",
		   cad);
	}
      return -EIO;
    }

  codec->vendor_id = a;

/*
 * Find out the primary group list
 */

  if (!corb_read (mixer, cad, 0, 0, GET_PARAMETER, HDA_NODE_COUNT, &x, &b))
    {
      cmn_err (CE_WARN, "GET_PARAMETER HDA_NODE_COUNT3 failed\n");
      return -EIO;
    }

  codec->first_node = first_node = (x >> 16) & 0xff;
  num_nodes = x & 0xff;

/*
 * Check if this one is an audio codec (has an audio function group)
 */
  for (i = first_node; i < first_node + num_nodes; i++)
    {
      unsigned int gt;

      if (corb_read
	  (mixer, cad, i, 0, GET_PARAMETER, HDA_GROUP_TYPE, &gt, &b));
      if ((gt & 0xff) != 1)	/* Audio function group */
	continue;

      has_audio_group = 1;
    }

#if 0
/*
 * Find codec specific settings
 */
  for (ix = 0; codecs[ix].id != 0; ix++)
    if (codecs[ix].id == a)
      break;

  DDB (cmn_err
       (CE_CONT, "HD audio Codec ID: %08x (%s)\n", a, codecs[ix].name));

  if (codecs[ix].id == 0)	/* Unknown codec */
    {
      if (group_type == 1)
	sprintf (hw_info, " Codec %2d: Unknown (0x%08x", cad, a);
      cmn_err (CE_NOTE, "Unknown HDA codec 0x%08x\n", a);
      /* 
       * Create hexadecimal codec ID
       */
      if (has_audio_group && mixer->chip_name == NULL)
	if ((mixer->chip_name = PMALLOC (mixer->osdev, 32)) != NULL)
	  {
	    sprintf (mixer->chip_name, "0x%08x", a);
	  }
    }
  else
    {
      if (group_type == 1)
	sprintf (hw_info, " Codec %2d: %s (0x%08x", cad, codecs[ix].name, a);
    }

  if (has_audio_group && mixer->chip_name == NULL)
    {
      mixer->chip_name = codecs[ix].name;

    }

  if (codecs[ix].remap != NULL)
    codec->remap = codecs[ix].remap;

  if (codecs[ix].flags != 0)
    codec->vendor_flags = codecs[ix].flags;

  if (codecs[ix].mixer_init != NULL)
    codec->mixer_init = codecs[ix].mixer_init;

  codec->multich_map = codecs[ix].multich_map;
  codec->codec_desc = &codecs[ix];
#endif

  if (corb_read (mixer, cad, 0, 0, GET_PARAMETER, HDA_REVISION, &a, &b))
    {
      DDB (cmn_err (CE_CONT, "HDA codec revision %d.%d (%d.%d) (0x%08x)\n",
		    (a >> 20) & 0xf,
		    (a >> 16) & 0xf, (a >> 8) & 0xff, a & 0xff, a));
    }
  else
    DDB (cmn_err (CE_CONT, "Can't get codec revision\n"));

  DDB (cmn_err (CE_CONT, "**** Codec %d ****\n", cad));
  DDB (cmn_err
       (CE_CONT, "First group node %d, num nodes %d\n", first_node,
	num_nodes));

  for (i = first_node; i < first_node + num_nodes; i++)
    {
      corb_read (mixer, cad, i, 0, GET_PARAMETER, HDA_GROUP_TYPE, &a, &b);
      if ((a & 0xff) == group_type)	/* Proper function group type */
	{
	  codec->afg = i;

	  if (corb_read (mixer, cad, i, 0, GET_SUBSYSTEM_ID, 0, &a, &b))
	    {
	      DDB (cmn_err (CE_CONT, "Subsystem ID = 0x%08x\n", a));

	      if (group_type == 1)
		{
		  /* Append subvendor ID to hw_info */
		  hw_info += strlen (hw_info);
		  sprintf (hw_info, "/0x%08x", a);
		}

	      codec->subvendor_id = a;

#if 0
	      for (subix = 0; subdevices[subix].id != 0; subix++)
		if (subdevices[subix].id == a)
		  {
		    if (subdevices[subix].main_id != 0)
		      if (subdevices[subix].main_id != codec->vendor_id)
			continue;

		    if (subdevices[subix].pci_subdevice != 0)
		      if (subdevices[subix].pci_subdevice != pci_subdevice)
			continue;


		    DDB (cmn_err
			 (CE_CONT, "Subdevice known as %s\n",
			  subdevices[subix].name));
		    if (group_type == 1)
		      {
			hw_info += strlen (hw_info);
			sprintf (hw_info, " %s", subdevices[subix].name);
		      }
		    if (subdevices[subix].remap != NULL)
		      {
			codec->remap = subdevices[subix].remap;
		      }

		    if (subdevices[subix].multich_map != 0)
		      codec->multich_map = subdevices[subix].multich_map;
		    if (subdevices[subix].flags != 0)
		      codec->vendor_flags = subdevices[subix].flags;
		    if (subdevices[subix].mixer_init != NULL)
		      {
			codec->mixer_init = subdevices[subix].mixer_init;
		      }
		  }
#endif
	    }
	  break;
	}
    }

  hw_info += strlen (hw_info);
  if (group_type == 1)
    strcpy (hw_info, ")\n");

  for (i = first_node; i < first_node + num_nodes; i++)
    {
      if (!attach_function_group (mixer, cad, i, group_type))
	continue;

      /* power up the AFG! */
      corb_write (mixer, cad, i, 0, SET_POWER_STATE, 0);
    }

#if 0
  if (has_audio_group)
    {
      polish_widget_list (mixer, cad);
    }

  copy_endpoints (mixer, codec, 0);	/* Copy analog endpoints from codec to mixer */
#endif

  return (has_audio_group) ? 0 : -EIO;
}

static void
follow_path(hdaudio_mixer_t *mixer, codec_t *codec, widget_t *widget, path_t *path, int use_force)
{
  int i;

  if (widget->used)
     return;

  widget->used = 1;
  path->widgets[path->nwidgets++] = widget;
  if (widget->wid_type == NT_PIN)
  {
	path->jack_count++;
	path->association=widget->association;
	path->sequence=widget->sequence;

	/*
	 * Stop at a pin widget that is not the first widget of the path.
	 * In this way we don't walk back the input chain from the pin.
	 */

	if (path->nwidgets > 1)
           return;
  }

  if (use_force || path->nwidgets == 1 || widget->nconn == 1 || (widget->nconn>0 && widget->wid_type == NT_SELECT))
  {
     for (i=0;i<widget->nconn;i++)
     if (!codec->widgets[widget->connections[i]].used)
     //if (codec->widgets[widget->connections[i]].refcount < 2)
     {
     	follow_path(mixer, codec, &codec->widgets[widget->connections[i]], path, use_force);
	return;
     }
  }
}

static void
dump_path(hdaudio_mixer_t *mixer, path_t *path)
{
	int i, j;

	if (path->nwidgets == 0)
	   return;

	printf("Path (a=%x, s=%x, jc=%d):\n ", path->association, path->sequence, path->jack_count);

	for (i=0;i<path->nwidgets;i++)
	{
	    widget_t *widget = path->widgets[i];

	    printf("\t%02x(%s/%s/nc=%d/rc=%d", widget->wid, widget_id[widget->wid_type], widget->name, widget->nconn, widget->refcount);
	    printf(")\n");

	    if (widget->widget_caps & WCAP_OUTPUT_AMP_PRESENT)
	       {
		       printf("\t\tOutput amp\n");
	       }

	    if (widget->widget_caps & WCAP_INPUT_AMP_PRESENT)
	       {
		       printf("\t\t%d input amp(s)\n", widget->nconn);

		       for (j=0;j<widget->nconn;j++)
		       {
	    		  widget_t *in_widget;

			  in_widget = &mixer->codecs[widget->cad]->
				        widgets[widget->connections[j]];
	    printf("\t\t\t%02x(%s/%s/nc=%d/rc=%d)\n", in_widget->wid, widget_id[in_widget->wid_type], in_widget->name, in_widget->nconn, in_widget->refcount);

	    		  in_widget->used=1;
		       }
	       }
	}
	printf("\n");
}

static void
store_path(hdaudio_mixer_t *mixer, path_t *path)
{
	if (npaths>=MAX_PATHS)
	{
		cmn_err(CE_WARN, "Too many paths\n");
		return;
	}

	paths[npaths++]=path;
}

static void
create_path_list(hdaudio_mixer_t *mixer, int cad, int node_type, int use_force)
{
  codec_t *codec;
  int wid;

  path_t *path;

cmn_err(CE_CONT, "Create path list %d (%s)\n", node_type, widget_id[node_type]);
  codec=mixer->codecs[cad];

  for (wid=0;wid<codec->nwidgets;wid++)
  if (codec->widgets[wid].wid_type == node_type)
  if (!codec->widgets[wid].skip)
  if (codec->widgets[wid].wid == wid)
  {
	  path=malloc(sizeof(*path));

	  follow_path(mixer, codec, &codec->widgets[wid], path, use_force);

	  if (path->nwidgets>1)
	     store_path(mixer, path);
	  else
	     if (path->nwidgets>0)
	        path->widgets[0]->used=0;
  }

}

static void
create_path_list_for_pin(hdaudio_mixer_t *mixer, int cad, int association, int sequence)
{
  codec_t *codec;
  int wid;

  path_t *path;

  codec=mixer->codecs[cad];

  for (wid=0;wid<codec->nwidgets;wid++)
  if (codec->widgets[wid].wid_type == NT_PIN && !codec->widgets[wid].skip)
  if (codec->widgets[wid].pin_type == PIN_HEADPHONE || codec->widgets[wid].pin_type == PIN_OUT)
  if (codec->widgets[wid].wid == wid)
  if (codec->widgets[wid].nconn > 0)
  if (!codec->widgets[wid].skip)
  if (codec->widgets[wid].sequence == sequence)
  if (codec->widgets[wid].association == association)
  {
	  path=malloc(sizeof(*path));

	  follow_path(mixer, codec, &codec->widgets[wid], path, 0);

	  if (path->nwidgets>1)
	     store_path(mixer, path);
	  else
	     if (path->nwidgets>0)
	        path->widgets[0]->used=0;
  }

}

hdaudio_mixer_t *
hdaudio_mixer_create (char *name, void *devc,
		      oss_device_t * osdev,
		      hdmixer_write_t writefunc,
		      hdmixer_read_t readfunc, unsigned int codecmask,
		      unsigned int vendor_id, unsigned int subvendor_id)
{
  hdaudio_mixer_t *mixer;
  int i, func;
  int ncodecs = 0;
  char tmp[128];
  int mixer_dev;

  char *hw_info = osdev->hw_info;	/* For printing hardware information */

  if ((mixer = PMALLOC (osdev, sizeof (*mixer))) == NULL)
    {
      cmn_err (CE_WARN, "hdaudio_mixer_create: Out of memory\n");
      return NULL;
    }

  memset (mixer, 0, sizeof (*mixer));

  mixer->devc = devc;
  mixer->osdev = osdev;
  mixer->mixer_dev = 0;
  strncpy (mixer->name, name, sizeof (mixer->name) - 1);
  mixer->name[sizeof (mixer->name) - 1] = 0;

  for (i = 0; i < MAX_CODECS; i++)
    mixer->codecs[i] = NULL;

#ifdef _KERNEL
  mixer->read = readfunc;
  mixer->write = writefunc;
#endif

  mixer->codecmask = codecmask;

  sprintf (hw_info, "HD Audio controller %s\n"
	   "Vendor ID    0x%08x\n"
	   "Subvendor ID 0x%08x\n", name, vendor_id, subvendor_id);
  hw_info += strlen (hw_info);

/*
 * Search first all audio function groups for all codecs and then
 * handle modem function groups.
 */
  for (func = 1; func <= 2; func++)
    for (i = 0; i < 16; i++)
      if (mixer->codecmask & (1 << i))
	{
	  if (attach_codec (mixer, i, hw_info, subvendor_id, func) >= 0)
	    ncodecs++;
	  hw_info += strlen (hw_info);
	}

  for (i = 0; i < 16; i++)
  if (mixer->codecmask & (1 << i))
  {
     int association, sequence;

printf("*** Codec %d\n", i);
     for (association=1;association<16;association++)
     for (sequence=0;sequence<16;sequence++)
         create_path_list_for_pin(mixer, i, association, sequence);

     create_path_list(mixer, i, NT_ADC, 0);
     create_path_list(mixer, i, NT_MIXER, 1);
     create_path_list(mixer, i, NT_SELECT, 1);
  }

  for (i=0;i<npaths;i++)
      dump_path(mixer, paths[i]);

#if 1
  printf("\n\nOther widgets:\n");

  for (i = 0; i < 16; i++)
  if (mixer->codecmask & (1 << i))
  {
	int wid;

	for (wid=0;wid<mixer->codecs[i]->nwidgets;wid++)
	{
		widget_t *widget = &mixer->codecs[i]->widgets[wid];

		if (widget->wid != wid)
		   continue;

		if (widget->used || widget->skip)
		   continue;
	    printf("Codec %d, Widget %02x %s/%s/%d\n", widget->cad, widget->wid, widget_id[widget->wid_type], widget->name, widget->nconn);
	}
  }
#endif

  return mixer;
}

int
main (int argc, char *argv[])
{
  unsigned int rev, b;
  int first_node, num_nodes;
  int i;
  int cad = 0;

  int codecmask = 0;

  oss_device_t osdev = { 0 };

  osdev.hw_info = malloc (256);

  if ((fd = open ("/dev/oss/oss_hdaudio0/pcm0", O_RDWR | O_EXCL, 0)) == -1)
    {
      perror ("/dev/oss/oss_hdaudio0/pcm0");
      exit (-1);
    }

  for (cad = 0; cad < 15; cad++)
    if (corb_read (NULL, cad, 0, 0, GET_PARAMETER, HDA_REVISION, &rev, &b))
      {
#if 1
	printf ("Codec %2d: HD codec revision %d.%d (%d.%d) (0x%08x)\n", cad,
		(rev >> 20) & 0xf, (rev >> 16) & 0xf, (rev >> 8) & 0xff,
		rev & 0xff, rev);
#endif
	codecmask |= (1 << cad);
      }

  hdaudio_mixer_create ("ACME hdaudio", NULL,
			&osdev, NULL, NULL, codecmask, 0x12345678, 0x8754321);

  //printf("HW Info: %s\n", osdev.hw_info);
  exit (0);
}
