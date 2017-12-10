
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <soundcard.h>

typedef int sound_os_info, mixer_create_controls_t, oss_device_t;

#include <hdaudio.h>

#define ioctl_arg int
#define mixer_ext_init_fn int
#include <hdaudio_codec.h>

static int num_widgets = 0;

static int print_widgets = 0;

typedef struct
{
  int type;
  int valid;
  int used;

  char name[32];

  unsigned int wcaps;
  unsigned int pincaps;
  unsigned int inamp_caps;
  unsigned int outamp_caps;
  int nconn;
  int connections[MAX_CONN];
  widget_t widget_info;
} mixgen_widget_t;

static mixgen_widget_t widgets[256] = { {0} };

static int errors = 0;

static unsigned int default_outamp_caps = 0;
static unsigned int default_inamp_caps = 0;
static unsigned int subdevice = 0;

#undef corb_read
#undef corb_write

int fd;
void *mixer = NULL;
int trace = 0;

int cad = -1;

int
corb_write (void *dc, unsigned int cad, unsigned int nid, unsigned int d,
	    unsigned int verb, unsigned int parm)
{
  unsigned int tmp;

  tmp = (cad << 28) | (d << 27) | (nid << 20) | (verb << 8) | parm;
  if (trace)
    printf ("WRITE %08x\n", tmp);

  if (ioctl (fd, HDA_IOCTL_WRITE, &tmp) == -1)
    {
      /* perror("HDA_IOCTL_WRITE"); */
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
  if (trace)
    printf ("READ %08x\n", tmp);

  return 1;
}

static int
set_error (char *msg)
{
  fprintf (stderr, "%s");
  errors++;

  return 0;
}

static int
gen_amps (mixgen_widget_t * widget, int wid)
{
  int i;
  char *name;

  if (widget->wcaps & WCAP_OUTPUT_AMP_PRESENT)
    {
      if (widget->outamp_caps & ~AMPCAP_MUTE)	/* Has gain control */
	{
	  name = "-";
	  if (widget->type == NT_PIN)
	    name = "invol";
	  printf ("\t\tHDA_OUTAMP(0x%02x, group, \"%s\", 90);\n", wid, name);
	}
      else if (widget->outamp_caps & AMPCAP_MUTE)	/* Only mute control */
	{
	  name = "mute";
	  if (widget->type == NT_PIN)
	    name = "inmute";
	  printf ("\t\tHDA_OUTMUTE(0x%02x, group, \"%s\", UNMUTE);\n", wid,
		  name);
	}
    }

  if (widget->wcaps & WCAP_INPUT_AMP_PRESENT)
    {
      int n = widget->nconn;
      if (widget->type == NT_PIN)
	n = 1;

      if (n > 1 && !(widget->inamp_caps & ~AMPCAP_MUTE))
	{			/* Create mute group */
	  printf ("\t\t{\n");
	  printf ("\t\t\tint amp_group;\n\n");
	  printf ("\t\t\tHDA_GROUP(amp_group, group, \"mute\");\n");
	  for (i = 0; i < widget->nconn; i++)
	    {
	      char *name;

	      name = widgets[widget->connections[i]].name;
	      if (widgets[widget->connections[i]].type == NT_PIN)
		name = widgets[widget->connections[i]].widget_info.color;
	      printf
		("\t\t\tHDA_INMUTE(0x%02x, %d, amp_group, \"%s\", UNMUTE);\t/* From widget 0x%02x */\n",
		 wid, i, name, widget->connections[i]);
	    }
	  printf ("\t\t}\n");
	}
      else
	{
	  for (i = 0; i < n; i++)
	    {
	      char *name;

	      name = widgets[widget->connections[i]].name;
	      if (widgets[widget->connections[i]].type == NT_PIN)
		name = widgets[widget->connections[i]].widget_info.color;
	      if (widget->type == NT_PIN)
		name = "out";
	      if (widget->inamp_caps & ~AMPCAP_MUTE)	/* Has gain control */
		{
		  printf
		    ("\t\tHDA_INAMP(0x%02x, %d, group, \"%s\", 90);\t/* From widget 0x%02x */\n",
		     wid, i, name, widget->connections[i]);
		}
	      else
		{
		  printf
		    ("\t\tHDA_INMUTE(0x%02x, %d, group, \"%smute\", UNMUTE);\t/* From widget 0x%02x */\n",
		     wid, i, name, widget->connections[i]);
		}
	    }
	}
    }

  return 0;
}

static int
follow_widget_chain (int wid, int force)
{
  int i;
  char choice_list[1024] = "", *s = choice_list;
  mixgen_widget_t *widget;

  if (!widgets[wid].valid || widgets[wid].used)
    return 0;

  widget = &widgets[wid];

  if (widget->widget_info.refcount > 1 && !force)
    return 0;

  widget->used = 1;

  printf ("\n\t\t/* Widget 0x%02x (%s) */\n", wid, widget->name);

  for (i = 0; i < widget->nconn; i++)
    {
      char *name = widgets[widget->connections[i]].name;
      if (widgets[widget->connections[i]].type == NT_PIN)
	name = widgets[widget->connections[i]].widget_info.color;
      if (i > 0)
	*s++ = ' ';
      if (widget->connections[i] < num_widgets)
	strcpy (s, widgets[widget->connections[i]].name);
      else
	strcpy (s, "unknown");
      printf ("\t\t/* Src 0x%x=%s */\n", widget->connections[i], name);
      s += strlen (s);
    }

  if (widget->type == NT_SELECT && widget->nconn > 1)
    {
      printf ("\t\tif (HDA_SELECT(0x%02x, \"src\", ctl, group, -1))\n", wid);
      printf ("\t\t   {\n");
      printf ("\t\t\tHDA_CHOICES(ctl, \"%s\");\n", choice_list);
      printf ("\t\t   }\n");
    }

  gen_amps (widget, wid);
/*
 * Follow the input connection chain.
 */
  if (widget->nconn == 1)
    follow_widget_chain (widget->connections[0], force);

  return 0;
}

static int
handle_pin_widget (int wid, mixgen_widget_t * widget)
{
  int outselects = 0, inselects = 0, num_amps = 0;
  char choice_list[1024] = "", *s = choice_list;
  int i;

  widget->used = 1;

  if (widget->pincaps & PINCAP_INPUT_CAPABLE)
    {
      if (!(widget->wcaps & WCAP_DIGITAL))	/* Analog pin */
	{
	  inselects = 1;
	}
    }

  if (widget->pincaps & PINCAP_OUTPUT_CAPABLE)
    {
      if (!(widget->wcaps & WCAP_DIGITAL))	/* Analog pin */
	{
	  outselects = widget->nconn;
	}
    }

  if (widget->wcaps & WCAP_INPUT_AMP_PRESENT)
    {
      num_amps += widget->nconn;
    }

  if (widget->wcaps & WCAP_OUTPUT_AMP_PRESENT)
    {
      num_amps++;
    }

  if (inselects + outselects + num_amps == 0)
    return 0;

  printf
    ("\n\tif (HDA_PIN_GROUP(0x%02x, group, pin_group, \"%s\", n, \"jack\", 4))\t/* Pin widget 0x%02x */\n",
     wid, widget->widget_info.color, wid);
  printf ("\t   {\n");

  for (i = 0; i < widget->nconn; i++)
    {
      if (i > 0)
	*s++ = ' ';

      if (widget->connections[i] < num_widgets)
	sprintf (s, "%s-out", widgets[widget->connections[i]].name);
      else
	strcpy (s, "unknown");
      printf ("\t\t/* Src 0x%x=%s */\n", widget->connections[i],
	      widgets[widget->connections[i]].name);
      s += strlen (s);
    }

  if (widget->nconn > 0)
    *s++ = ' ';
  sprintf (s, "input");
  s += strlen (s);

  printf ("\t\tif (HDA_PINSELECT(0x%02x, ctl, group, \"mode\", -1))\n", wid);
  printf ("\t\t\tHDA_CHOICES(ctl, \"%s\");\n", choice_list);

  gen_amps (widget, wid);

  for (i = 0; i < widget->nconn; i++)
    follow_widget_chain (widget->connections[i], 0);

  printf ("\t   }\n");

  return 0;
}

static int
handle_adc_widget (int wid, mixgen_widget_t * widget)
{
  int outselects = 0, inselects = 0, num_amps = 0;
  char choice_list[4000] = "", *s = choice_list;
  int i;

  widget->used = 1;

  if (widget->wcaps & WCAP_INPUT_AMP_PRESENT)
    {
      num_amps += widget->nconn;
    }

  if (widget->wcaps & WCAP_OUTPUT_AMP_PRESENT)
    {
      num_amps++;
    }

  printf
    ("\n\tif (HDA_ADC_GROUP(0x%02x, group, rec_group, \"%s\", n, \"record\", 4))\t/* ADC widget 0x%02x */\n",
     wid, widget->name, wid);
  printf ("\t   {\n");

  for (i = 0; i < widget->nconn; i++)
    {
      if (i > 0)
	*s++ = ' ';
      if (widget->connections[i] < num_widgets)
	strcpy (s, widgets[widget->connections[i]].name);
      else
	strcpy (s, "unknown");
      printf ("\t\t/* Src 0x%x=%s */\n", widget->connections[i],
	      widgets[widget->connections[i]].name);
      s += strlen (s);
    }

  if (widget->nconn > 1)
    {
      printf ("\t\tif (HDA_SELECT(0x%02x, \"src\", ctl, group, -1))\n", wid);
      printf ("\t\t   {\n");
      printf ("\t\t\tHDA_CHOICES(ctl, \"%s\");\n", choice_list);
      printf ("\t\t   }\n");
    }

  gen_amps (widget, wid);

  for (i = 0; i < widget->nconn; i++)
    follow_widget_chain (widget->connections[i], 0);

  printf ("\t   }\n");

  return 0;
}

static int
handle_misc_widget (int wid, mixgen_widget_t * widget)
{
  int outselects = 0, inselects = 0, num_amps = 0;
  char choice_list[4000] = "", *s = choice_list;
  int i;

  widget->used = 1;

  if (widget->wcaps & WCAP_INPUT_AMP_PRESENT)
    {
      num_amps += widget->nconn;
    }

  if (widget->wcaps & WCAP_OUTPUT_AMP_PRESENT)
    {
      num_amps++;
    }

  if (num_amps == 0)
    return 0;

  printf
    ("\n\tif (HDA_MISC_GROUP(0x%02x, group, misc_group, \"%s\", n, \"misc\", 8))\t/* Misc widget 0x%02x */\n",
     wid, widget->name, wid);
  printf ("\t   {\n");

  for (i = 0; i < widget->nconn; i++)
    {
      if (i > 0)
	*s++ = ' ';
      if (widget->connections[i] < num_widgets)
	strcpy (s, widgets[widget->connections[i]].name);
      else
	strcpy (s, "unknown");
      printf ("\t\t/* Src 0x%x=%s */\n", widget->connections[i],
	      widgets[widget->connections[i]].name);
      s += strlen (s);
    }

  if (widget->type == NT_SELECT && widget->nconn > 1)
    {
      printf ("\t\tif (HDA_SELECT(0x%02x, \"src\", ctl, group, -1))\n", wid);
      printf ("\t\t   {\n");
      printf ("\t\t\tHDA_CHOICES(ctl, \"%s\");\n", choice_list);
      printf ("\t\t   }\n");
    }

  gen_amps (widget, wid);

  for (i = 0; i < widget->nconn; i++)
    follow_widget_chain (widget->connections[i], 1);

  printf ("\t   }\n");

  return 0;
}

static int
attach_widget (int wid)
{
  hda_name_t name;
  hda_widget_info_t info;
  unsigned int wcaps, b;
  mixgen_widget_t *widget = &widgets[wid];
  int type;
  int i;
  unsigned int nconn, connections[256];

  memset (&name, 0, sizeof (name));
  memset (&info, 0, sizeof (info));

  name.cad = cad;
  name.wid = wid;
  if (ioctl (fd, HDA_IOCTL_NAME, &name) == -1)
    strcpy (name.name, "Unknown_widget");

  info.cad = cad;
  info.wid = wid;
  if (ioctl (fd, HDA_IOCTL_WIDGET, &info) != -1)
    memcpy (&widget->widget_info, info.info, sizeof (widget->widget_info));

  if (!corb_read
      (mixer, cad, wid, 0, GET_PARAMETER, HDA_WIDGET_CAPS, &wcaps, &b))
    return set_error ("Can't get widget capabilities\n");

  type = (wcaps >> 20) & 0xf;

  if (type == NT_PIN)
    if (!corb_read (mixer, cad, wid, 0,
		    GET_PARAMETER, HDA_PIN_CAPS, &widget->pincaps, &b))
      return set_error ("Can't get widget pin capabilities\n");

  widget->valid = 1;
  strcpy (widget->name, name.name);
  widget->wcaps = wcaps;
  widget->inamp_caps = default_inamp_caps;
  widget->outamp_caps = default_outamp_caps;
  widget->type = type;

  /*
   * Handle connection list.
   */
  nconn = 0;
  if (wcaps & WCAP_CONN_LIST)
    {

      if (corb_read
	  (mixer, cad, wid, 0, GET_PARAMETER, HDA_CONNLIST_LEN, &nconn, &b))

	if (nconn >= MAX_CONN)
	  {
	    fprintf (stderr,
		     "Too many input connections (%d) for widget %d\n", nconn,
		     wid);
	    exit (-1);
	  }
      if (nconn != 0)
	{
	  unsigned int clist;
	  int j;
	  int n = 0;

	  nconn &= 0x7f;

	  for (i = 0; i < nconn; i += 4)
	    if (corb_read
		(mixer, cad, wid, 0, GET_CONNECTION_LIST_ENTRY, i, &clist,
		 &b))
	      for (j = 0; j < 4 && (i + j) < nconn; j++)
		{
		  connections[n++] = (clist >> (j * 8)) & 0xff;
		}
	}
    }

  for (i = 0; i < nconn; i++)
    widget->connections[i] = connections[i];
  widget->nconn = nconn;

  if ((wcaps & WCAP_AMP_CAP_OVERRIDE) && (wcaps & WCAP_OUTPUT_AMP_PRESENT))
    if (!corb_read (mixer, cad, wid, 0,
		    GET_PARAMETER, HDA_OUTPUTAMP_CAPS,
		    &widget->outamp_caps, &b))
      {
	fprintf (stderr, "GET_PARAMETER HDA_OUTPUTAMP_CAPS failed\n");
	return set_error ("Can't get outamp capabilities\n");
      }

  if ((wcaps & WCAP_AMP_CAP_OVERRIDE) && (wcaps & WCAP_INPUT_AMP_PRESENT))
    if (!corb_read (mixer, cad, wid, 0,
		    GET_PARAMETER, HDA_INPUTAMP_CAPS,
		    &widget->inamp_caps, &b))
      {
	fprintf (stderr, "GET_PARAMETER HDA_INPUTTAMP_CAPS failed\n");
	return set_error ("Can't get inamp capabilities\n");
      }

  return 0;
}

static void
dump_node (int wid)
{
  int i;
  unsigned int a, b, gtype, gcaps, wcaps, sizes, fmts, pincaps;
  unsigned int inamp_caps, outamp_caps, clen, pstates, pcaps, gpio_count;
  unsigned int vkcaps;
  int first_node = 0, num_nodes = 0;
  char *s;
  int ntype = -1;

  if (corb_read
      (mixer, cad, wid, 0, GET_PARAMETER, HDA_GROUP_TYPE, &gtype, &b))
    if ((gtype & 0x1ff) != 0)
      {
	s = "Unknown";

	switch (gtype & 0xff)
	  {
	  case 0:
	    s = "Reserved";
	    return;
	    break;
	  case 1:
	    s = "Audio function group";
	    break;
	  case 2:
	    s = "Vendor defined modem function group";
	    //return;
	    break;
	  }

      }
  if (corb_read (mixer, cad, wid, 0, GET_SUBSYSTEM_ID, 0, &a, &b))
    {
      printf ("/* Subsystem ID %08x */\n", a);
      subdevice = a;
    }

  if (!corb_read (mixer, cad, wid, 0, GET_PARAMETER, HDA_NODE_COUNT, &a, &b))
    {
      fprintf (stderr, "GET_PARAMETER HDA_NODE_COUNT2 failed\n");
      return;
    }

  if (!corb_read (mixer, cad, wid, 0,
		  GET_PARAMETER, HDA_OUTPUTAMP_CAPS,
		  &default_outamp_caps, &b))
    {
      fprintf (stderr, "GET_PARAMETER HDA_OUTPUTAMP_CAPS failed\n");
      return;
    }

  if (!corb_read (mixer, cad, wid, 0,
		  GET_PARAMETER, HDA_INPUTAMP_CAPS, &default_inamp_caps, &b))
    {
      fprintf (stderr, "GET_PARAMETER HDA_INPUTTAMP_CAPS failed\n");
      return;
    }

  printf ("/* Default amplifier caps: in=%08x, out=%08x */\n",
	  default_inamp_caps, default_outamp_caps);

  first_node = (a >> 16) & 0xff;
  num_nodes = a & 0xff;

  for (wid = first_node; wid < first_node + num_nodes; wid++)
    {
      attach_widget (wid);
      if (wid >= num_widgets)
	num_widgets = wid + 1;
    }
}

static void
dump_widgets (void)
{
  int i;

  for (i = 0; i < num_widgets; i++)
    {
      if (widgets[i].valid)
	{
	  if (widgets[i].name[0] == 0) /* Empty string */
	     printf("\tES,\n");
	  else
	     printf ("\t\"%s\",\t\t/* 0x%02x */\n", widgets[i].name, i);
	}
      else
	printf ("\tES,\t\t/* 0x%02x */\n", i);
    }
}

int
main (int argc, char *argv[])
{
  unsigned int a, b;
  int first_node, num_nodes;
  int i;
  char codec_name[32] = "";

  if (argc > 1)
    strcpy (codec_name, argv[1]);

  if ((fd = open ("/dev/oss/oss_hdaudio0/pcm0", O_RDWR | O_EXCL, 0)) == -1)
    {
      perror ("/dev/oss/oss_hdaudio0/pcm0");
      exit (-1);
    }

  if (argc > 2)
    cad = atoi (argv[2]);

  for (i = 3; i < argc; i++)
    switch (argv[i][0])
      {
      case 'w':
	print_widgets = 1;
	break;
      }

  if (cad == -1)		/* Not given on command line so find it. */
    for (cad = 0; cad < 16; cad++)
      if (corb_read (mixer, cad, 0, 0, GET_PARAMETER, HDA_VENDOR, &a, &b))
	break;
  printf
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

  printf ("/* Codec index is %d */\n", cad);
  printf ("/* Codec vendor %04x:%04x */\n", a >> 16, a & 0xffff);

  if (corb_read (mixer, cad, 0, 0, GET_PARAMETER, HDA_REVISION, &a, &b))
    {
      printf ("/* HD codec revision %d.%d (%d.%d) (0x%08x) */\n",
	      (a >> 20) & 0xf, (a >> 16) & 0xf, (a >> 8) & 0xff, a & 0xff, a);
    }
  else
    printf ("hdaudio: Can't get codec revision\n");

/*
 * Find out the primary group list
 */

  if (!corb_read (mixer, cad, 0, 0, GET_PARAMETER, HDA_NODE_COUNT, &a, &b))
    exit(1);

  first_node = (a >> 16) & 0xff;
  num_nodes = a & 0xff;

  for (i = first_node; i < first_node + num_nodes; i++)
    dump_node (i);

/*
 * Next produce the output
 */

  if (*codec_name == 0)
    sprintf (codec_name, "subdevice%08x", subdevice);

  printf ("#include \"oss_hdaudio_cfg.h\"\n");
  printf ("#include \"hdaudio.h\"\n");
  printf ("#include \"hdaudio_codec.h\"\n");
  printf ("#include \"hdaudio_dedicated.h\"\n");
  printf ("\n");

  printf ("int\n");
  printf
    ("hdaudio_%s_mixer_init (int dev, hdaudio_mixer_t * mixer, int cad, int top_group)\n",
     codec_name);
  printf ("{\n");
  //printf ("  int wid;\n");
  printf ("  int ctl=0;\n");
  //printf ("  codec_t *codec = mixer->codecs[cad];\n");

  printf ("\n");
  printf
    ("  DDB(cmn_err(CE_CONT, \"hdaudio_%s_mixer_init got called.\\n\"));\n",
     codec_name);
  printf ("\n");

/*
 * Handle PIN widgets first
 */
  printf ("  /* Handle PIN widgets */\n");
  printf ("  {\n");
  printf ("\tint n, group, pin_group;\n");
  printf ("\n\tn=0;\n");

  printf ("\n\tHDA_GROUP(pin_group, top_group, \"jack\");\n");

  for (i = 0; i < num_widgets; i++)
    {
      mixgen_widget_t *widget = &widgets[i];

      if (!widget->valid)
	continue;

      if (widget->type != NT_PIN)
	continue;

      handle_pin_widget (i, widget);
    }
  printf ("  }\n");

/*
 * Handle ADC widgets
 */
  printf ("  /* Handle ADC widgets */\n");
  printf ("  {\n");
  printf ("\tint n, group, rec_group;\n");
  printf ("\n\tn=0;\n");

  printf ("\n\tHDA_GROUP(rec_group, top_group, \"record\");\n");

  for (i = 0; i < num_widgets; i++)
    {
      mixgen_widget_t *widget = &widgets[i];

      if (!widget->valid || widget->used)
	continue;

      if (widget->type != NT_ADC)
	continue;

      handle_adc_widget (i, widget);
    }
  printf ("  }\n");

/*
 * Handle the remaining widgets
 */
  printf ("  /* Handle misc widgets */\n");
  printf ("  {\n");
  printf ("\tint n, group, misc_group;\n");
  printf ("\n\tn=0;\n");

  printf ("\n\tHDA_GROUP(misc_group, top_group, \"misc\");\n");

  for (i = 0; i < num_widgets; i++)
    {
      mixgen_widget_t *widget = &widgets[i];

      if (!widget->valid || widget->used)
	continue;

      handle_misc_widget (i, widget);
    }
  printf ("  }\n");

  printf ("  return 0;\n");
  printf ("}\n");

  if (print_widgets)
    dump_widgets ();
  exit (errors);
}
