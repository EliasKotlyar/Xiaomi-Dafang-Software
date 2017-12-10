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
 * Settings cache for libossmix
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <soundcard.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>

#define OSSMIX_REMOTE

#include "libossmix.h"
#include "libossmix_impl.h"

static local_mixer_t *mixers[MAX_TMP_MIXER] = { NULL };

void
mixc_add_node (int mixernum, int node, oss_mixext * ext)
{
  local_mixer_t *lmixer;
  oss_mixext *lnode;

  if (mixers[mixernum] == NULL)
    {
      int i;

      mixers[mixernum] = lmixer = malloc (sizeof (*lmixer));
      if (lmixer == NULL)
	{
	  fprintf (stderr, "mixc_add_node: Out of memory\n");
	  exit (EXIT_FAILURE);
	}

      memset (lmixer, 0, sizeof (*lmixer));
      for (i = 0; i < MAX_TMP_NODES; i++)
	lmixer->values[i] = -1;	// Invalid
    }
  else
    lmixer = mixers[mixernum];

  if (ext->ctrl >= lmixer->nrext)
    lmixer->nrext = ext->ctrl + 1;

  if (node >= MAX_TMP_NODES)
    {
      fprintf (stderr, "mixc_add_node: Node number too large %d\n", node);
      exit (EXIT_FAILURE);
    }

  lnode = lmixer->nodes[node];

  if (lnode == NULL)
    {
      lmixer->nodes[node] = lnode = malloc (sizeof (*lnode));

      if (lnode == NULL)
	{
	  fprintf (stderr, "mixc_get_node: Out of memory\n");
	  exit (EXIT_FAILURE);
	}
    }

  memcpy (lnode, ext, sizeof (*ext));

}

oss_mixext *
mixc_get_node (int mixernum, int node)
{
  local_mixer_t *lmixer;
  oss_mixext *lnode;

  if (mixers[mixernum] == NULL)
    {
      return NULL;
    }
  lmixer = mixers[mixernum];

  if (node >= MAX_TMP_NODES)
    {
      fprintf (stderr, "mixc_get_node: Node number too large %d\n", node);
      exit (EXIT_FAILURE);
    }

  lnode = lmixer->nodes[node];

  return lnode;
}

void
mixc_clear_changeflags(int mixernum)
{
  local_mixer_t *lmixer;

  if (mixers[mixernum] == NULL)
    {
      return;
    }
  lmixer = mixers[mixernum];

  memset(lmixer->changemask, 0, sizeof(lmixer->changemask));
}

void
mixc_set_value (int mixernum, int node, int value)
{
  local_mixer_t *lmixer;

  if (mixers[mixernum] == NULL)
    {
      return;
    }
  lmixer = mixers[mixernum];

  if (node >= MAX_TMP_NODES)
    {
      fprintf (stderr, "mixc_set_value: Node number too large %d\n", node);
      exit (EXIT_FAILURE);
    }

  if (lmixer->values[node] != value)
     lmixer->changemask[node / 8] |= (1 << (node % 8));

  lmixer->values[node] = value;
}

int
mixc_get_value (int mixernum, int node)
{
  local_mixer_t *lmixer;

  if (mixers[mixernum] == NULL)
    {
      return -1;
    }
  lmixer = mixers[mixernum];

  if (node >= MAX_TMP_NODES)
    {
      fprintf (stderr, "mixc_get_value: Node number too large %d\n", node);
      exit (EXIT_FAILURE);
    }
  lmixer->changemask[node / 8] &= ~(1 << (node % 8));

  return lmixer->values[node];
}

int
mixc_get_all_values (int mixernum, value_packet_t value_packet, int changecheck)
{
  int i, n = 0;
  oss_mixext *lnode;
  local_mixer_t *lmixer;

  if (mixers[mixernum] == NULL)
    {
      fprintf (stderr, "mixc_get_all_values: Mixer %d doesn't exist\n",
	       mixernum);
      return 0;
    }
  lmixer = mixers[mixernum];

  for (i = 0; i < lmixer->nrext; i++)
    {
      lnode = lmixer->nodes[i];

      if (lnode == NULL)
	{
	  fprintf (stderr, "mixc_get_all_values: Mixer %d, node %d == NULL\n",
		   mixernum, i);
	  continue;
	}

      if (changecheck) // Not changed since the last time
      if (!(lmixer->changemask[i / 8] & (1 << (i % 8)) ))
	 continue;

      if (lnode->type != MIXT_DEVROOT && lnode->type != MIXT_GROUP
	  && lnode->type != MIXT_MARKER)
	{
	  value_packet[n].node = i;
	  value_packet[n].value = lmixer->values[i];

     	  lmixer->changemask[i / 8] &= ~(1 << (i % 8));

//printf("Send %d = %08x\n", i, lmixer->values[i]);
	  n++;
	}
    }

  return n;
}
