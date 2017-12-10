/*
 * Purpose: A simple sample program for using the new mixer API
 * Copyright (C) 4Front Technologies, 2002-2004. Released under GPLv2/CDDL.
 *
 * Description:
 * This program is as simple as possible example for using the new mixer API
 * of OSS 4.0 (and later) from applications. It shows how to read or
 * change the settings. Please read the OSS API Developer's Manual for
 * more info about this API.
 *
 * This mixer interface is designed to ne "human readable". The idea is that
 * the mixer program just shows whatever controls are available and lets the
 * user to adjust them as he/she likes.
 *
 * Please note that the control names and numbers are fully dynamic. There are
 * some control names such as "spdif.enable" that are used by several different
 * drivers. It's relatively safe to assume that such controls have always
 * the same meaning. However there is no list of such control names
 * available at this moment. Even this kind of controls are supported just by 
 * some of the devices (some devices just don't have that feature).
 *
 * Most control names are 100% non-portable between devices. In addition some
 * settings may be available depending on the configuration.
 *
 * {!notice Applications using this mixer interface must not assume that
 * certain controls are always available or that they have exactly defined
 * meanings.}
 *
 * This program can be run without any command line arguments to 
 * list the available controls and their values. The mixer device number
 * (0 by default) can be given on the command line.
 *
 * Another way is to give the mixer number, control ame and value
 * on the command line ({!shell mixext 1 spdif.enable 1}).
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <soundcard.h>
#include <time.h>
#include <errno.h>

static int mixer_dev = 0;	/* Change this to access another mixer device */
static int mixerfd = -1;

typedef struct
{
  int num;
  char *name;
} name_ent;

#define TYPE_ENTRY(x) {x, #x}

name_ent type_names[] = {
  TYPE_ENTRY (MIXT_DEVROOT),
  TYPE_ENTRY (MIXT_GROUP),
  TYPE_ENTRY (MIXT_ONOFF),
  TYPE_ENTRY (MIXT_MUTE),
  TYPE_ENTRY (MIXT_ENUM),
  TYPE_ENTRY (MIXT_MONOSLIDER),
  TYPE_ENTRY (MIXT_STEREOSLIDER),
  TYPE_ENTRY (MIXT_MESSAGE),
  TYPE_ENTRY (MIXT_MONOVU),
  TYPE_ENTRY (MIXT_STEREOVU),
  TYPE_ENTRY (MIXT_MONOPEAK),
  TYPE_ENTRY (MIXT_STEREOPEAK),
  TYPE_ENTRY (MIXT_RADIOGROUP),
  TYPE_ENTRY (MIXT_MARKER),
  TYPE_ENTRY (MIXT_VALUE),
  TYPE_ENTRY (MIXT_HEXVALUE),
  TYPE_ENTRY (MIXT_MONODB),
  TYPE_ENTRY (MIXT_STEREODB),
  TYPE_ENTRY (MIXT_SLIDER),
  TYPE_ENTRY (MIXT_3D),
  TYPE_ENTRY (MIXT_MONOSLIDER16),
  TYPE_ENTRY (MIXT_STEREOSLIDER16),
  {-1, NULL}
};

static char *
mixt_name (int num)
{
  int i;

  i = 0;

  while (type_names[i].num != -1)
    {
      if (type_names[i].num == num)
	return type_names[i].name;
      i++;
    }
  return "Unknown type";
}

void
list_controls (int mixer_dev)
{
  int i, n, nn = 0;
  oss_mixext ext;
  int marker_seen = 0;

  n = mixer_dev;

  if (ioctl (mixerfd, SNDCTL_MIX_NREXT, &n) == -1)
    {
      perror ("SNDCTL_MIX_NREXT");
      if (errno == EINVAL)
	fprintf (stderr, "Error: OSS version 3.9 or later is required\n");
      return;
    }

  printf ("%d mixer controls available (including ordinary mixer controls)\n",
	  n);

  for (i = 0; i < n; i++)
    {
      oss_mixer_value val;

      ext.dev = mixer_dev;
      ext.ctrl = i;

      if (ioctl (mixerfd, SNDCTL_MIX_EXTINFO, &ext) == -1)
	{
	  if (errno == EINVAL)
	    {
	      fprintf (stderr, "SNDCTL_MIX_EXTINFO failed\n");
	      fprintf (stderr,
		       "This is almost certainly caused by a too old or new OSS version.\n");
	      return;
	    }

	  perror ("SNDCTL_MIX_EXTINFO");
	  return;
	}

      if (ext.type == MIXT_MARKER)
	{
	  marker_seen = 1;
	  continue;
	}
      else if (!marker_seen)
	continue;

      val.dev = mixer_dev;
      val.ctrl = i;
      val.timestamp = ext.timestamp;

      val.value = 0;

      if (ext.type != MIXT_DEVROOT && ext.type != MIXT_GROUP
	  && ext.type != MIXT_MARKER)
	if (ioctl (mixerfd, SNDCTL_MIX_READ, &val) == -1)
	  {
	    perror ("SNDCTL_MIX_READ");
	    val.value = 0xffffffff;
	  }

      printf ("%3d: \"%s\" (\"%s\"), parent %d\n", i, ext.extname, ext.id,
	      ext.parent);
      printf ("     Type %d (%s), value %d (0x%08x) (max %d)\n", ext.type,
	      mixt_name (ext.type), val.value, val.value, ext.maxvalue);
      printf ("     Update counter %d\n", ext.update_counter);
      nn++;
    }

  printf ("%d controls accessible by this program\n", nn);
}

int
set_control (int mixer_dev, char *name, int value)
{
  int i, n;
  oss_mixext ext;
  int marker_seen = 0;

  n = mixer_dev;

  if (ioctl (mixerfd, SNDCTL_MIX_NREXT, &n) == -1)
    {
      perror ("SNDCTL_MIX_NREXT");
      if (errno == EINVAL)
	fprintf (stderr, "Error: OSS version 3.9 or later is required\n");
      return -4;
    }

  for (i = 0; i < n; i++)
    {
      oss_mixer_value val;

      ext.dev = mixer_dev;
      ext.ctrl = i;

      if (ioctl (mixerfd, SNDCTL_MIX_EXTINFO, &ext) == -1)
	{
	  perror ("SNDCTL_MIX_EXTINFO");
	  return -1;
	}

      if (ext.type == MIXT_MARKER)
	{
	  marker_seen = 1;
	  continue;
	}
      else if (!marker_seen)
	continue;


      if (strcmp (ext.extname, name) != 0)	/* No match */
	continue;

      if (!(ext.flags & MIXF_WRITEABLE))
	return -2;

      val.dev = mixer_dev;
      val.ctrl = i;
      val.timestamp = ext.timestamp;
      val.value = value;

      if (ioctl (mixerfd, SNDCTL_MIX_WRITE, &val) == -1)
	{
	  perror ("SNDCTL_MIX_WRITE");
	  return -3;
	}

      return 1;
    }

  return 0;
}

int
main (int argc, char *argv[])
{
  char *devmixer;

  if ((devmixer=getenv("OSS_MIXERDEV"))==NULL)
     devmixer = "/dev/mixer";

  if (argc > 1)
    mixer_dev = atoi (argv[1]);

  if ((mixerfd = open (devmixer, O_RDWR, 0)) == -1)
    {
      perror (devmixer);
      exit (-1);
    }

  if (argc > 3)
    {
      int val;

      val = atoi (argv[3]);

      if (set_control (mixer_dev, argv[2], val) > 0)
	{
	  fprintf (stderr, "OK\n");
	}
      else
	{
	  fprintf (stderr, "Failed!!!\n");
	}
      exit (0);
    }

  list_controls (mixer_dev);

  exit (0);
}
