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
 * Main module for libossmix
 */
#include <stdio.h>
#include <soundcard.h>

#include "libossmix.h"
#include "libossmix_impl.h"

static ossmix_driver_t *mixer_driver = NULL;
int mixlib_trace = 0;
static int num_mixers = 0;

ossmix_callback_t event_callback = NULL;

int
ossmix_init (void)
{
  if (mixlib_trace > 0)
    fprintf (stderr, "ossmix_init() called\n");
  return 0;
}

void
ossmix_close (void)
{
  if (mixlib_trace > 0)
    fprintf (stderr, "ossmix_close() called\n");
}

void
_client_event (int event, int p1, int p2, int p3, int p4, int p5)
{
  /*
   * To be called only by the internals of ossmixlib
   */

  if (event_callback != NULL)
    {
      ossmix_callback_parm_t parm;

      parm.event = event;
      parm.p1 = p1;
      parm.p2 = p2;
      parm.p3 = p3;
      parm.p4 = p4;
      parm.p5 = p5;
      event_callback (&parm);
    }
}

int
ossmix_connect (const char *hostname, int port)
{
  if (mixlib_trace > 0)
    fprintf (stderr, "ossmix_connect(%s, %d)) called\n", hostname, port);

  if (hostname == NULL)
    mixer_driver = &ossmix_local_driver;
  else
    mixer_driver = &ossmix_tcp_driver;

  event_callback = NULL;

  return mixer_driver->connect (hostname, port);
}

int
ossmix_get_fd (ossmix_select_poll_t * cb)
{
  return mixer_driver->get_fd (cb);
}

void
ossmix_set_callback (ossmix_callback_t cb)
{
  event_callback = cb;
  mixer_driver->enable_events ();
}

void
ossmix_disconnect (void)
{
  if (mixlib_trace > 0)
    fprintf (stderr, "ossmix_disconnect() called\n");

  event_callback = NULL;

  mixer_driver->disconnect ();
}

int
ossmix_get_nmixers (void)
{
  if (mixlib_trace > 0)
    fprintf (stderr, "ossmix_get_nmixes() called\n");
  return (num_mixers = mixer_driver->get_nmixers ());
}

int
ossmix_get_mixerinfo (int mixernum, oss_mixerinfo * mi)
{
  if (mixernum >= num_mixers)
    {
      fprintf (stderr, "ossmix_get_mixerinfo: Bad mixer number (%d >= %d)\n",
	       mixernum, num_mixers);
      return -1;
    }

  return mixer_driver->get_mixerinfo (mixernum, mi);
}

int
ossmix_open_mixer (int mixernum)
{
  if (mixernum >= num_mixers)
    {
      fprintf (stderr, "ossmix_open_mixer: Bad mixer number (%d >= %d)\n",
	       mixernum, num_mixers);
      return -1;
    }
  return mixer_driver->open_mixer (mixernum);
}

void
ossmix_close_mixer (int mixernum)
{
  if (mixernum >= num_mixers)
    {
      fprintf (stderr, "ossmix_close_mixer: Bad mixer number (%d >= %d)\n",
	       mixernum, num_mixers);
      return;
    }

  mixer_driver->close_mixer (mixernum);
}

int
ossmix_get_nrext (int mixernum)
{
  if (mixernum >= num_mixers)
    {
      fprintf (stderr, "ossmix_get_nrext: Bad mixer number (%d >= %d)\n",
	       mixernum, num_mixers);
      return -1;
    }
  return mixer_driver->get_nrext (mixernum);
}

int
ossmix_get_nodeinfo (int mixernum, int node, oss_mixext * ext)
{
  if (mixernum >= num_mixers)
    {
      fprintf (stderr, "ossmix_get_nodeinfo: Bad mixer number (%d >= %d)\n",
	       mixernum, num_mixers);
      return -1;
    }
  return mixer_driver->get_nodeinfo (mixernum, node, ext);
}

int
ossmix_get_enuminfo (int mixernum, int node, oss_mixer_enuminfo * ei)
{
  if (mixernum >= num_mixers)
    {
      fprintf (stderr, "ossmix_get_enuminfo: Bad mixer number (%d >= %d)\n",
	       mixernum, num_mixers);
      return -1;
    }
  return mixer_driver->get_enuminfo (mixernum, node, ei);
}

int
ossmix_get_description (int mixernum, int node, oss_mixer_enuminfo * desc)
{
  if (mixernum >= num_mixers)
    {
      fprintf (stderr,
	       "ossmix_get_description: Bad mixer number (%d >= %d)\n",
	       mixernum, num_mixers);
      return -1;
    }
  return mixer_driver->get_description (mixernum, node, desc);
}

int
ossmix_get_value (int mixernum, int node, int timestamp)
{
  if (mixernum >= num_mixers)
    {
      fprintf (stderr, "ossmix_get_value: Bad mixer number (%d >= %d)\n",
	       mixernum, num_mixers);
      return -1;
    }
  return mixer_driver->get_value (mixernum, node, timestamp);
}

void
ossmix_set_value (int mixernum, int node, int timestamp, int value)
{
  if (mixernum >= num_mixers)
    {
      fprintf (stderr, "ossmix_set_value: Bad mixer number (%d >= %d)\n",
	       mixernum, num_mixers);
      return;
    }

  mixer_driver->set_value (mixernum, node, timestamp, value);
}

void
ossmix_timertick(void)
{
  mixer_driver->timertick();
}

/*
 * Internal use functions (not to be used by applications)
 */
int
_ossmix_refresh_mixer(int mixernum, int prev_nmixers)
{
printf("_ossmix_refresh_mixer(%d, %d) called\n", mixernum, prev_nmixers);

	return prev_nmixers; // TODO
}
