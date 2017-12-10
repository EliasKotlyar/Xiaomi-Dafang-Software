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
 * Local driver for libossmix
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>

#include <soundcard.h>

#define OSSMIX_REMOTE

#include "libossmix.h"
#include "libossmix_impl.h"

static int global_fd = -1;
static int num_mixers=0;

static int mixer_fd[MAX_TMP_MIXER];

static int
local_connect (const char *hostname, int port)
{
  char *devmixer;
  int i;

  if (mixlib_trace > 0)
    fprintf (stderr, "Entered local_connect()\n");

  for (i = 0; i < MAX_TMP_MIXER; i++)
    mixer_fd[i] = -1;

  if ((devmixer = getenv ("OSS_MIXERDEV")) == NULL)
    devmixer = "/dev/mixer";

/*
 *	Open the mixer device
 */
  if ((global_fd = open (devmixer, O_RDWR, 0)) == -1)
    {
      perror (devmixer);
      return -1;
    }

  return 0;
}

static void
local_disconnect (void)
{
  if (mixlib_trace > 0)
    fprintf (stderr, "Entered local_disconnect()\n");

  if (global_fd >= 0)
    close (global_fd);

  global_fd = -1;
}

static void
local_enable_events (void)
{
}

static int
local_get_fd (ossmix_select_poll_t * cb)
{
  *cb = NULL;
  return -1;			/* No poll handling required */
}

static int
local_get_nmixers (void)
{
  oss_sysinfo si;

  if (ioctl (global_fd, SNDCTL_SYSINFO, &si) == -1)
    {
      perror ("SNDCTL_SYSINFO");
      return -1;
    }

  return num_mixers = si.nummixers;
}

static int
local_get_mixerinfo (int mixernum, oss_mixerinfo * mi)
{
  mi->dev = mixernum;

  if (ioctl (global_fd, SNDCTL_MIXERINFO, mi) == -1)
    {
      perror ("SNDCTL_MIXERINFO");
      return -1;
    }

  return 0;
}

static int
local_open_mixer (int mixernum)
{
  oss_mixerinfo mi;

  if (mixer_fd[mixernum] > -1)
    return 0;

  if (ossmix_get_mixerinfo (mixernum, &mi) < 0)
    return -1;

//fprintf(stderr, "local_open_mixer(%d: %s)\n", mixernum, mi.devnode);

  if ((mixer_fd[mixernum] = open (mi.devnode, O_RDWR, 0)) == -1)
    {
      perror (mi.devnode);
      return -1;
    }

  return 0;
}

static void
local_close_mixer (int mixernum)
{
//fprintf(stderr, "local_close_mixer(%d)\n", mixernum);

  if (mixer_fd[mixernum] == -1)
    return;

  close (mixer_fd[mixernum]);
  mixer_fd[mixernum] = -1;
}

static int
local_get_nrext (int mixernum)
{
  int n = -1;

  if (ioctl (mixer_fd[mixernum], SNDCTL_MIX_NREXT, &n) == -1)
    {
      perror ("SNDCTL_MIX_NREXT");
      return -1;
    }

  return n;
}

static int
local_get_nodeinfo (int mixernum, int node, oss_mixext * ext)
{
  ext->dev = mixernum;
  ext->ctrl = node;

  if (ioctl (mixer_fd[mixernum], SNDCTL_MIX_EXTINFO, ext) == -1)
    {
      perror ("SNDCTL_MIX_EXTINFO");
      return -1;
    }

  mixc_add_node (mixernum, node, ext);
  return 0;
}

static int
local_get_enuminfo (int mixernum, int node, oss_mixer_enuminfo * ei)
{
  ei->dev = mixernum;
  ei->ctrl = node;

  if (ioctl (mixer_fd[mixernum], SNDCTL_MIX_ENUMINFO, ei) == -1)
    {
      perror ("SNDCTL_MIX_ENUMINFO");
      return -1;
    }

  return 0;
}

static int
local_get_description (int mixernum, int node, oss_mixer_enuminfo * desc)
{
  desc->dev = mixernum;
  desc->ctrl = node;

  if (ioctl (mixer_fd[mixernum], SNDCTL_MIX_DESCRIPTION, desc) == -1)
    {
      perror ("SNDCTL_MIX_DESCRIPTION");
      return -1;
    }

  return 0;
}

static int
local_get_value (int mixernum, int ctl, int timestamp)
{
  oss_mixer_value val;

  val.dev = mixernum;
  val.ctrl = ctl;
  val.timestamp = timestamp;

  if (ioctl (mixer_fd[mixernum], SNDCTL_MIX_READ, &val) == -1)
    {
      perror ("SNDCTL_MIX_READ");
      return -1;
    }

  mixc_set_value (mixernum, ctl, val.value);
  return val.value;
}

static int
private_get_value (int mixernum, int ctl, int timestamp)
{
  oss_mixer_value val;

  val.dev = mixernum;
  val.ctrl = ctl;
  val.timestamp = timestamp;

  if (ioctl (mixer_fd[mixernum], SNDCTL_MIX_READ, &val) == -1)
    {
      perror ("SNDCTL_MIX_READ");
      return -1;
    }

  return val.value;
}

static void
local_set_value (int mixernum, int ctl, int timestamp, int value)
{
  oss_mixer_value val;

  val.dev = mixernum;
  val.ctrl = ctl;
  val.timestamp = timestamp;
  val.value = value;

  if (ioctl (mixer_fd[mixernum], SNDCTL_MIX_WRITE, &val) == -1)
    {
      perror ("SNDCTL_MIX_WRITE");
    }
  mixc_set_value (mixernum, ctl, val.value);
}

static void
update_values (int mixernum)
{
  oss_mixext *ext;
  int i;
  int nrext;
  int value, prev_value;

  nrext = ossmix_get_nrext (mixernum);

  for (i = 0; i < nrext; i++)
    {
      if ((ext = mixc_get_node (mixernum, i)) == NULL)
      {
	continue;
      }

      if (ext->type == MIXT_DEVROOT || ext->type == MIXT_GROUP
	  || ext->type == MIXT_MARKER)
	continue;

      prev_value = mixc_get_value (mixernum, i);

      if ((value = private_get_value (mixernum, i, ext->timestamp)) < 0)
	continue;
      // TODO check for EIDRM

      if (value != prev_value)
      {
  	 mixc_set_value (mixernum, i, value);
	 _client_event (OSSMIX_EVENT_VALUE, mixernum, i, value, 0, 0);
      }

    }
}

static void
local_timertick(void)
{
	int mixernum, n;

	for (mixernum=0;mixernum<num_mixers;mixernum++)
        if (mixer_fd[mixernum] >= 0) /* Open */
	{
		update_values (mixernum);
	}

	n=ossmix_get_nmixers();
	if (n>num_mixers)
	{
		num_mixers=n;
	 	_client_event (OSSMIX_EVENT_NEWMIXER, n, 0, 0, 0, 0);
	}
}

ossmix_driver_t ossmix_local_driver = {
  local_connect,
  local_get_fd,
  local_disconnect,
  local_enable_events,
  local_get_nmixers,
  local_get_mixerinfo,
  local_open_mixer,
  local_close_mixer,
  local_get_nrext,
  local_get_nodeinfo,
  local_get_enuminfo,
  local_get_description,
  local_get_value,
  local_set_value,
  local_timertick
};
