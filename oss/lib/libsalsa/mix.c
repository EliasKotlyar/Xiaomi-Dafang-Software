/*
 *  Copyright (c) 2004 by Hannu Savolainen < hannu@opensound.com>
 *
 *   Parts of the code is derived from the alsa-lib package that is
 *   copyrighted by Jaroslav Kysela and the other ALSA team members.
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License version 2.1 as
 *   published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
#include <stdio.h>
#include "local.h"

#define SND_VERS(a, b, c) (a<<16|b<<8|c)

typedef struct _snd_mixer
{
  int mixdev;
  int nrext;

  oss_mixerinfo info;

  snd_mixer_elem_t *elems;
} snd_mixer_t;

#define MAX_MIXERS	32
static snd_mixer_t *mixers[MAX_MIXERS];
static int nmixers = 0;

typedef struct _snd_mixer_selem_id
{
  int number;
  char name[32];
} snd_mixer_selem_id_t;

typedef struct _snd_mixer_class
{
  int dummy;
} snd_mixer_class_t;

typedef struct _snd_mixer_elem
{
  snd_mixer_t *mixer;
  int ctrl;
  oss_mixext ext;

  snd_mixer_elem_t *next;

  int low, high;
} snd_mixer_elem_t;

/**
 * \brief Opens an empty mixer
 * \param mixerp Returned mixer handle
 * \param mode Open mode
 * \return 0 on success otherwise a negative error code
 */
int
snd_mixer_open (snd_mixer_t ** mixerp, int mode)
{
  snd_mixer_t *mixer;
  dbg_printf2 ("snd_mixer_open()\n");

  ALIB_INIT ();
  if (!alib_appcheck ())
    return -ENODEV;

  if ((mixer = malloc (sizeof (*mixer))) == NULL)
    return -ENOMEM;

  memset (mixer, 0, sizeof (*mixer));
  *mixerp = mixer;

  return 0;
}

/**
 * \brief Attach an HCTL to an opened mixer
 * \param mixer Mixer handle
 * \param name HCTL name (see #snd_hctl_open)
 * \return 0 on success otherwise a negative error code
 */
int
snd_mixer_attach (snd_mixer_t * mixer, const char *name)
{
  int dev = 0;

  ALIB_INIT ();
  dbg_printf2 ("snd_mixer_attach(%s)\n", name);


  if (strcmp (name, "default") == 0)
    dev = 0;
  else if (name[0] == 'h' && name[1] == 'w' && name[2] == ':')
    {
      if (sscanf (name + 3, "%d", &dev) != 1)
	return -ENOENT;

      if (dev < 0 || dev >= sysinfo.nummixers)
	return -ENXIO;
    }
  else
    return -ENOENT;

  mixer->mixdev = dev;

  mixer->info.dev = dev;
  if (ioctl (mixer_fd, SNDCTL_MIXERINFO, &mixer->info) == -1)
    return -errno;

  mixer->nrext = mixer->info.nrext;

  if (nmixers < MAX_MIXERS)
    mixers[nmixers++] = mixer;

  return 0;
}


/**
 * \brief Close a mixer and free all related resources
 * \param mixer Mixer handle
 * \return 0 on success otherwise a negative error code
 */
int
snd_mixer_close (snd_mixer_t * mixer)
{
  dbg_printf2 ("snd_mixer_close()\n");

  free (mixer);
  return 0;
}

/**
 * \brief Register mixer simple element class
 * \param mixer Mixer handle
 * \param options Options container (not used now)
 * \param classp Pointer to returned mixer simple element class handle (or NULL) * \return 0 on success otherwise a negative error code
 */
#if 1
int
snd_mixer_selem_register (snd_mixer_t * mixer,
			  struct snd_mixer_selem_regopt *arg,
			  snd_mixer_class_t ** classp)
#else
int
snd_mixer_selem_register (snd_mixer_t * mixer, void *arg,
			  snd_mixer_class_t ** classp)
#endif
{
  snd_mixer_class_t *class;

  dbg_printf2 ("snd_mixer_selem_register()\n");

  if (classp == NULL)		/* Why this call was ever made ? */
    return 0;

  if ((class = malloc (sizeof (*class))) == NULL)
    return -ENOMEM;

  memset (class, 0, sizeof (*class));
  *classp = class;

  return 0;
}

/**
 * \brief Return info about playback volume control of a mixer simple element
 * \param elem Mixer simple element handle
 * \return 0 if no control is present, 1 if it's present
 */
int
snd_mixer_selem_has_playback_volume (snd_mixer_elem_t * elem)
{
  dbg_printf2 ("snd_mixer_selem_has_playback_volume(%x)\n", elem);
  fflush (stdout);

  if (elem->ext.type == MIXT_STEREOSLIDER)
    return 1;
  if (elem->ext.type == MIXT_MONOSLIDER)
    return 1;
  if (elem->ext.type == MIXT_SLIDER)
    return 1;

  return 0;
}

/**
 * \brief Return info about playback volume control of a mixer simple element
 * \param elem Mixer simple element handle
 * \return 0 if control is separated per channel, 1 if control acts on all channels together
 */
int
snd_mixer_selem_has_playback_volume_joined (snd_mixer_elem_t * elem)
{
  dbg_printf ("snd_mixer_selem_has_playback_volume_joined()\n");

  return elem->ext.type != MIXT_STEREOSLIDER;
}

/**
 * \brief Return info about capture volume control of a mixer simple element
 * \param elem Mixer simple element handle
 * \return 0 if no control is present, 1 if it's present
 */
int
snd_mixer_selem_has_capture_volume (snd_mixer_elem_t * elem)
{
  dbg_printf ("snd_mixer_selem_has_capture_volume()\n");

  return 0;
}


/**
 * \brief Return info about playback switch control existence of a mixer simple
element
 * \param elem Mixer simple element handle
 * \return 0 if no control is present, 1 if it's present
 */
int
snd_mixer_selem_has_playback_switch (snd_mixer_elem_t * elem)
{
  dbg_printf ("snd_mixer_selem_has_playback_switch()\n");

  if (elem->ext.type == MIXT_ONOFF || elem->ext.type == MIXT_MUTE)
    return 1;
  return 0;
}

/**
 * \brief Set value of playback volume control of a mixer simple element
 * \param elem Mixer simple element handle
 * \param channel mixer simple element channel identifier
 * \param value control value
 * \return 0 on success otherwise a negative error code
 */
int
snd_mixer_selem_set_playback_volume (snd_mixer_elem_t * elem,
				     snd_mixer_selem_channel_id_t channel,
				     long value)
{
  int vol, range;
  oss_mixer_value rec;

  dbg_printf2 ("snd_mixer_selem_set_playback_volume(%ld)\n", value);

  if (value < elem->low)
    value = elem->low;
  if (value > elem->high)
    value = elem->high;

  range = elem->high - elem->low;
  if (range == 0)
    range = 100;
  value -= elem->low;
  vol = (value * elem->ext.maxvalue) / range;

  rec.dev = elem->ext.dev;
  rec.ctrl = elem->ext.ctrl;
  rec.ctrl = elem->ext.ctrl;
  rec.timestamp = elem->ext.timestamp;
  rec.value = vol | (vol << 8);

  if (ioctl (mixer_fd, SNDCTL_MIX_WRITE, &rec) == -1)
    return -errno;

  return 0;
}

/**
 * \brief Set range for capture volume of a mixer simple element
 * \param elem Mixer simple element handle
 * \param min minimum volume value
 * \param max maximum volume value
 */
#if SND_LIB_VERSION > SND_VERS(1,0,9)
int
#else
void
#endif
snd_mixer_selem_set_capture_volume_range (snd_mixer_elem_t * elem,
					  long min, long max)
{
  dbg_printf ("snd_mixer_selem_set_capture_volume_range()\n");

#if SND_LIB_VERSION > SND_VERS(1,0,9)
  return 0;
#endif
}

/**
 * \brief Set value of capture volume control of a mixer simple element
 * \param elem Mixer simple element handle
 * \param channel mixer simple element channel identifier
 * \param value control value
 * \return 0 on success otherwise a negative error code
 */
int
snd_mixer_selem_set_capture_volume (snd_mixer_elem_t * elem,
				    snd_mixer_selem_channel_id_t channel,
				    long value)
{
  dbg_printf ("snd_mixer_selem_set_capture_volume()\n");

  return 0;
}

/**
 * \brief Set range for playback volume of a mixer simple element
 * \param elem Mixer simple element handle
 * \param min minimum volume value
 * \param max maximum volume value
 */
#if SND_LIB_VERSION > SND_VERS(1,0,9)
int
#else
void
#endif
snd_mixer_selem_set_playback_volume_range (snd_mixer_elem_t * elem,
					   long min, long max)
{
  dbg_printf2 ("snd_mixer_selem_set_playback_volume_range(%s, %d, %d)\n",
	       elem->ext.extname, min, max);

  elem->low = min;
  elem->high = max;

#if SND_LIB_VERSION > SND_VERS(1,0,9)
  return 0;
#endif
}

/**
 * \brief Return value of playback volume control of a mixer simple element
 * \param elem Mixer simple element handle
 * \param channel mixer simple element channel identifier
 * \param value pointer to returned value
 * \return 0 on success otherwise a negative error code
 */
int
snd_mixer_selem_get_playback_volume (snd_mixer_elem_t * elem,
				     snd_mixer_selem_channel_id_t channel,
				     long *value)
{
  int vol, range, left, right;
  oss_mixer_value rec;

  dbg_printf2 ("snd_mixer_selem_get_playback_volume()\n");

  rec.dev = elem->ext.dev;
  rec.ctrl = elem->ext.ctrl;
  rec.timestamp = elem->ext.timestamp;

  if (ioctl (mixer_fd, SNDCTL_MIX_READ, &rec) == -1)
    return -errno;

  left = rec.value & 0xff;
  right = (rec.value >> 8) & 0xff;

  if (left > right)
    vol = left;
  else
    vol = right;
  range = elem->high - elem->low;

  vol = (vol * range) / elem->ext.maxvalue;
  vol += elem->low;
  *value = vol;

  return 0;
}

/**
 * \brief Get range for playback volume of a mixer simple element
 * \param elem Mixer simple element handle
 * \param min Pointer to returned minimum
 * \param max Pointer to returned maximum
 */
#if SND_LIB_VERSION > SND_VERS(1,0,9)
int
#else
void
#endif
snd_mixer_selem_get_playback_volume_range (snd_mixer_elem_t * elem,
					   long *min, long *max)
{
  dbg_printf2 ("snd_mixer_selem_get_playback_volume_range()\n");
#if 1
  *min = elem->low;
  *max = elem->high;
#endif

#if SND_LIB_VERSION > SND_VERS(1,0,9)
  return 0;
#endif
}

/**
 * \brief get first element for a mixer
 * \param mixer Mixer handle
 * \return pointer to first element
 */
snd_mixer_elem_t *
snd_mixer_first_elem (snd_mixer_t * mixer)
{
  dbg_printf2 ("snd_mixer_first_elem()\n");

  return mixer->elems;
}

/**
 * \brief get next mixer element
 * \param elem mixer element
 * \return pointer to next element
 */
snd_mixer_elem_t *
snd_mixer_elem_next (snd_mixer_elem_t * elem)
{
  dbg_printf2 ("snd_mixer_elem_next(%x/%d)\n", elem, elem);

  if (elem == NULL || (long) elem < 4096)
    {
      dbg_printf2 ("Returning NULL\n");
      return NULL;
    }

  if (elem->next == NULL)
    dbg_printf ("No more elemsnts\n");
  else
    dbg_printf2 ("Returning %d/%s\n", elem->next->ctrl,
		 elem->next->ext.extname);
  return elem->next;
}

/**
 * \brief Find a mixer simple element
 * \param mixer Mixer handle
 * \param id Mixer simple element identifier
 * \return mixer simple element handle or NULL if not found
 *
 * Wuld somebody kindly explain me what in hell is the logic
 * behind this idiotic "simple" mixer stuff.
 */
snd_mixer_elem_t *
snd_mixer_find_selem (snd_mixer_t * mixer, const snd_mixer_selem_id_t * id)
{
  int i;

  dbg_printf2 ("snd_mixer_find_selem(%d/%s)\n", id->number, id->name);

  if (*id->name == 0 && id->number == 0)
    return NULL;

  for (i = 0; i < mixer->nrext; i++)
    {
      oss_mixext *ext = &mixer->elems[i].ext;

      if (ext->type == MIXT_GROUP ||
	  ext->type == MIXT_DEVROOT || ext->type == MIXT_MARKER)
	continue;

      if (strcasecmp (ext->extname, id->name) == 0)
	{
	  return &mixer->elems[i];
	}

      if (ext->ctrl == id->number)
	{
	  return &mixer->elems[i];
	}
    }

  return NULL;
}

/**
 * \brief Handle pending mixer events invoking callbacks
 * \param mixer Mixer handle
 * \return 0 otherwise a negative error code on failure
 */
int
snd_mixer_handle_events (snd_mixer_t * mixer)
{
  dbg_printf2 ("snd_mixer_handle_events()\n");
  // NOP
}

/**
 * \brief Load a mixer elements
 * \param mixer Mixer handle
 * \return 0 on success otherwise a negative error code
 */
int
snd_mixer_load (snd_mixer_t * mixer)
{
  snd_mixer_elem_t *elems;
  int i, n = 0;

  dbg_printf2 ("snd_mixer_load()\n");

  if (mixer->elems != NULL)
    free (mixer->elems);

  if ((elems = malloc (sizeof (*elems) * mixer->nrext)) == NULL)
    return -ENOMEM;

  memset (elems, 0, sizeof (*elems) * mixer->nrext);

  for (i = 0; i < mixer->nrext; i++)
    {
      oss_mixext *ext = &elems[n].ext;
      snd_mixer_elem_t *elem;

      elem = &elems[n];

      ext->dev = mixer->mixdev;
      ext->ctrl = i;

      if (ioctl (mixer_fd, SNDCTL_MIX_EXTINFO, ext) < 0)
	{
	  int e = errno;
	  perror ("SNDCTL_MIX_EXTINFO");
	  return -e;
	}

      if (ext->type == MIXT_DEVROOT)
	continue;
      if (ext->type == MIXT_GROUP)
	continue;
      if (ext->type == MIXT_MARKER)
	continue;

      elem->low = 0;
      elem->high = ext->maxvalue;
      elem->ctrl = elem->ext.ctrl;

      if (n > 0)
	elems[n - 1].next = &elems[n];

      n++;
    }

  mixer->nrext = n;
  mixer->elems = elems;
  return 0;
}

/**
 * \brief Get name part of mixer simple element identifier
 * \param elem Mixer simple element handle
 * \return name part of simple element identifier
 */
const char *
snd_mixer_selem_get_name (snd_mixer_elem_t * elem)
{
  dbg_printf2 ("snd_mixer_selem_get_name()\n");

  return elem->ext.extname;
}

/**
 * \brief Set index part of a mixer simple element identifier
 * \param obj Mixer simple element identifier
 * \param val index part
 */
void
snd_mixer_selem_id_set_index (snd_mixer_selem_id_t * obj, unsigned int val)
{
  dbg_printf2 ("snd_mixer_selem_id_set_index(%u)\n", val);

  obj->number = val;
}

/**
 * \brief Set name part of a mixer simple element identifier
 * \param obj Mixer simple element identifier
 * \param val name part
 */
void
snd_mixer_selem_id_set_name (snd_mixer_selem_id_t * obj, const char *val)
{
  dbg_printf2 ("snd_mixer_selem_id_set_name(%s)\n", val);

  strcpy (obj->name, val);
}

/**
 * \brief get size of #snd_mixer_selem_id_t
 * \return size in bytes
 */
size_t
snd_mixer_selem_id_sizeof ()
{
  return sizeof (snd_mixer_selem_id_t);
}


/**
 * \brief Get info about the active state of a mixer simple element
 * \param elem Mixer simple element handle
 * \return 0 if not active, 1 if active
 */
int
snd_mixer_selem_is_active (snd_mixer_elem_t * elem)
{
  dbg_printf2 ("snd_mixer_selem_is_active()\n");
  return 1;
}

/**
 * \brief Return value of capture switch control of a mixer simple element
 * \param elem Mixer simple element handle
 * \param channel mixer simple element channel identifier
 * \param value pointer to returned value
 * \return 0 on success otherwise a negative error code
 */
int
snd_mixer_selem_get_capture_switch (snd_mixer_elem_t * elem,
				    snd_mixer_selem_channel_id_t channel,
				    int *value)
{
  dbg_printf ("snd_mixer_selem_get_capture_switch()\n");

  *value = 0;
  return 0;
}

/**
 * \brief Return value of playback switch control of a mixer simple element
 * \param elem Mixer simple element handle
 * \param channel mixer simple element channel identifier
 * \param value pointer to returned value
 * \return 0 on success otherwise a negative error code
 */
int
snd_mixer_selem_get_playback_switch (snd_mixer_elem_t * elem,
				     snd_mixer_selem_channel_id_t channel,
				     int *value)
{
  oss_mixer_value rec;

  dbg_printf ("snd_mixer_selem_get_playback_switch()\n");

  *value = 0;

  rec.dev = elem->ext.dev;
  rec.ctrl = elem->ext.ctrl;
  rec.ctrl = elem->ext.ctrl;
  rec.timestamp = elem->ext.timestamp;

  if (ioctl (mixer_fd, SNDCTL_MIX_READ, &rec) == -1)
    return -errno;

  *value = !!rec.value;
  return 0;
}

/**
 * \brief Get info about channels of capture stream of a mixer simple element
 * \param elem Mixer simple element handle
 * \param channel Mixer simple element channel identifier
 * \return 0 if channel is not present, 1 if present
 */
int
snd_mixer_selem_has_capture_channel (snd_mixer_elem_t * elem,
				     snd_mixer_selem_channel_id_t channel)
{
  dbg_printf ("snd_mixer_selem_has_capture_channel()\n");

  return (channel < 2);
}

/**
 * \brief Return info about capture switch control existence of a mixer simple element
 * \param elem Mixer simple element handle
 * \return 0 if no control is present, 1 if it's present
 */
int
snd_mixer_selem_has_capture_switch (snd_mixer_elem_t * elem)
{
  dbg_printf ("snd_mixer_selem_has_capture_switch()\n");

  return 0;
}

/**
 * \brief Return info about capture switch control of a mixer simple element
 * \param elem Mixer simple element handle
 * \return 0 if control is separated per channel, 1 if control acts on all channels together
 */
int
snd_mixer_selem_has_capture_switch_joined (snd_mixer_elem_t * elem)
{
  dbg_printf ("snd_mixer_selem_has_capture_switch_joined()\n");

  return 1;
}

/**
 * \brief Get info about channels of capture stream of a mixer simple element
 * \param elem Mixer simple element handle
 * \return 0 if not mono, 1 if mono
 */
int
snd_mixer_selem_is_capture_mono (snd_mixer_elem_t * elem)
{
  dbg_printf ("snd_mixer_selem_is_capture_mono()\n");
  return 0;
}

/**
 * \brief Get info about channels of playback stream of a mixer simple element
 * \param elem Mixer simple element handle
 * \return 0 if not mono, 1 if mono
 */
int
snd_mixer_selem_is_playback_mono (snd_mixer_elem_t * elem)
{
  dbg_printf ("snd_mixer_selem_is_playback_mono()\n");
  return 0;
}

/**
 * \brief Set value of capture switch control of a mixer simple element
 * \param elem Mixer simple element handle
 * \param channel mixer simple element channel identifier
 * \param value control value
 * \return 0 on success otherwise a negative error code
 */
int
snd_mixer_selem_set_capture_switch (snd_mixer_elem_t * elem,
				    snd_mixer_selem_channel_id_t channel,
				    int value)
{
  dbg_printf ("snd_mixer_selem_set_capture_switch()\n");

  return 0;
}

/**
 * \brief Set value of capture switch control for all channels of a mixer simple element
 * \param elem Mixer simple element handle
 * \param value control value
 * \return 0 on success otherwise a negative error code
 */
int
snd_mixer_selem_set_capture_switch_all (snd_mixer_elem_t * elem, int value)
{
  dbg_printf ("snd_mixer_selem_set_capture_switch_all()\n");

  return 0;
}

/**
 * \brief Set value of playback switch control for all channels of a mixer simple element
 * \param elem Mixer simple element handle
 * \param value control value
 * \return 0 on success otherwise a negative error code
 */
int
snd_mixer_selem_set_playback_switch_all (snd_mixer_elem_t * elem, int value)
{
  oss_mixer_value rec;

  dbg_printf ("snd_mixer_selem_set_playback_switch_all()\n");

  rec.dev = elem->ext.dev;
  rec.ctrl = elem->ext.ctrl;
  rec.ctrl = elem->ext.ctrl;
  rec.timestamp = elem->ext.timestamp;
  rec.value = !!value;

  if (ioctl (mixer_fd, SNDCTL_MIX_WRITE, &rec) == -1)
    return -errno;

  return 0;
}

/**
 * \brief Return value of capture volume control of a mixer simple element
 * \param elem Mixer simple element handle
 * \param channel mixer simple element channel identifier
 * \param value pointer to returned value
 * \return 0 on success otherwise a negative error code
 */
int
snd_mixer_selem_get_capture_volume (snd_mixer_elem_t * elem,
				    snd_mixer_selem_channel_id_t channel,
				    long *value)
{
  dbg_printf ("snd_mixer_selem_get_capture_volume()\n");

  *value = 0;
  return 0;
}

/**
 * \brief Get mixer simple element identifier
 * \param elem Mixer simple element handle
 * \param id returned mixer simple element identifier
 */
void
snd_mixer_selem_get_id (snd_mixer_elem_t * elem, snd_mixer_selem_id_t * id)
{
  dbg_printf ("snd_mixer_selem_get_id()\n");
  // What in hell is this?

  id->number = elem->ctrl;
  strcpy (id->name, elem->ext.extname);
  dbg_printf ("ID=%d / %s\n", id->number, id->name);
}

/**
 * \brief Get name part of a mixer simple element identifier
 * \param obj Mixer simple element identifier
 * \return name part
 */
const char *
snd_mixer_selem_id_get_name (const snd_mixer_selem_id_t * obj)
{
  dbg_printf ("snd_mixer_selem_id_get_name()=%s\n", obj->name);
//return "Objname";
  return obj->name;
}

/**
 * \brief Get index part of a mixer simple element identifier
 * \param obj Mixer simple element identifier
 * \return index part
 */
unsigned int
snd_mixer_selem_id_get_index (const snd_mixer_selem_id_t * obj)
{
  dbg_printf ("snd_mixer_selem_id_get_index()\n");

  return 0;
}


/**
 * \brief Get elements count for a mixer
 * \param mixer mixer handle
 * \return elements count
 */
unsigned int
snd_mixer_get_count (const snd_mixer_t * obj)
{
  dbg_printf ("snd_mixer_get_count()\n");

  return obj->nrext;
}

/**
 * \brief get poll descriptors
 * \param mixer Mixer handle
 * \param pfds array of poll descriptors
 * \param space space in the poll descriptor array
 * \return count of filled descriptors
 */
int
snd_mixer_poll_descriptors (snd_mixer_t * mixer, struct pollfd *pfds,
			    unsigned int space)
{
  dbg_printf ("snd_mixer_poll_descriptors()\n");

  return 0;
}

/**
 * \brief get count of poll descriptors for mixer handle
 * \param mixer Mixer handle
 * \return count of poll descriptors
 */
int
snd_mixer_poll_descriptors_count (snd_mixer_t * mixer)
{
  dbg_printf ("snd_mixer_poll_descriptors_count()\n");

  return 0;
}

/**
 * \brief get returned events from poll descriptors
 * \param mixer Mixer handle
 * \param pfds array of poll descriptors
 * \param nfds count of poll descriptors
 * \param revents returned events
 * \return zero if success, otherwise a negative error code
 */
int
snd_mixer_poll_descriptors_revents (snd_mixer_t * mixer, struct pollfd *pfds,
				    unsigned int nfds,
				    unsigned short *revents)
{
  dbg_printf ("snd_mixer_poll_descriptors_revents()\n");

  return 0;
}

/**
 * \brief Set callback function for a mixer
 * \param mixer mixer handle
 * \param callback callback function
 */
void
snd_mixer_set_callback (snd_mixer_t * obj, snd_mixer_callback_t val)
{
  dbg_printf0 ("snd_mixer_set_callback()\n");

}

/**
 * \brief Get range for capture volume of a mixer simple element
 * \param elem Mixer simple element handle
 * \param min Pointer to returned minimum
 * \param max Pointer to returned maximum
 */
#if SND_LIB_VERSION > SND_VERS(1,0,9)
int
#else
void
#endif
snd_mixer_selem_get_capture_volume_range (snd_mixer_elem_t * elem,
					  long *min, long *max)
{
  dbg_printf ("snd_mixer_selem_get_capture_volume_range()\n");

  *min = 0;
  *max = 100;

#if SND_LIB_VERSION > SND_VERS(1,0,9)
  return 0;
#endif
}

/**
 * \brief Return true if mixer simple element is an enumerated control
 * \param elem Mixer simple element handle
 * \return 0 normal volume/switch control, 1 enumerated control
 */
int
snd_mixer_selem_is_enumerated (snd_mixer_elem_t * elem)
{
  dbg_printf ("snd_mixer_selem_is_enumerated()\n");

  return 0;
}

/**
 * \brief get the current selected enumerated item for the given mixer simple element
 * \param elem Mixer simple element handle
 * \param channel mixer simple element channel identifier
 * \param itemp the pointer to store the index of the enumerated item
 * \return 0 if successful, otherwise a negative error code
 */
int
snd_mixer_selem_get_enum_item (snd_mixer_elem_t * elem,
			       snd_mixer_selem_channel_id_t channel,
			       unsigned int *itemp)
{
  dbg_printf ("snd_mixer_selem_get_enum_item()\n");

  return 0;
}

/**
 * \brief Return the number of enumerated items of the given mixer simple element
 * \param elem Mixer simple element handle
 * \return the number of enumerated items, otherwise a negative error code
 */
int
snd_mixer_selem_get_enum_items (snd_mixer_elem_t * elem)
{
  dbg_printf ("snd_mixer_selem_get_enum_items()\n");
  return 0;
}

/**
 * \brief get the enumerated item string for the given mixer simple element
 * \param elem Mixer simple element handle
 * \param item the index of the enumerated item to query
 * \param maxlen the maximal length to be stored
 * \param buf the buffer to store the name string
 * \return 0 if successful, otherwise a negative error code
 */
int
snd_mixer_selem_get_enum_item_name (snd_mixer_elem_t * elem,
				    unsigned int item,
				    size_t maxlen, char *buf)
{
  dbg_printf ("snd_mixer_selem_get_enum_item_name()\n");
  strncpy (buf, "Enum", maxlen);
  buf[maxlen - 1] = 0;

  return 0;
}

/**
 * \brief Return info about capture volume control of a mixer simple element
 * \param elem Mixer simple element handle
 * \return 0 if control is separated per channel, 1 if control acts on all channels together
 */
int
snd_mixer_selem_has_capture_volume_joined (snd_mixer_elem_t * elem)
{
  dbg_printf ("snd_mixer_selem_has_capture_volume_joined()\n");
  return elem->ext.type != MIXT_STEREOSLIDER;
}

/**
 * \brief Get info about channels of playback stream of a mixer simple element
 * \param elem Mixer simple element handle
 * \param channel Mixer simple element channel identifier
 * \return 0 if channel is not present, 1 if present
 */
int
snd_mixer_selem_has_playback_channel (snd_mixer_elem_t * elem,
				      snd_mixer_selem_channel_id_t channel)
{
  dbg_printf ("snd_mixer_selem_has_playback_channel(%s, %d)\n",
	      elem->ext.extname, channel);

  if (elem->ext.type == MIXT_SLIDER && channel == 0)
    return 1;
  if (elem->ext.type == MIXT_MONOSLIDER && channel == 0)
    return 1;
  if (elem->ext.type == MIXT_STEREOSLIDER && channel < 2)
    return 1;

  return 0;
}

/**
 * \brief Return info about playback switch control of a mixer simple element
 * \param elem Mixer simple element handle
 * \return 0 if control is separated per channel, 1 if control acts on all channels together
 */
int
snd_mixer_selem_has_playback_switch_joined (snd_mixer_elem_t * elem)
{
  dbg_printf ("snd_mixer_selem_has_playback_switch_joined()\n");

  return 0;
}

/**
 * \brief copy one #snd_mixer_selem_id_t to another
 * \param dst pointer to destination
 * \param src pointer to source
 */
void
snd_mixer_selem_id_copy (snd_mixer_selem_id_t * dst,
			 const snd_mixer_selem_id_t * src)
{
  dbg_printf ("snd_mixer_selem_id_copy()\n");
  memcpy (dst, src, sizeof (*dst));
}


/**
 * \brief set the current selected enumerated item for the given mixer simple element
 * \param elem Mixer simple element handle
 * \param channel mixer simple element channel identifier
 * \param item the enumerated item index
 * \return 0 if successful, otherwise a negative error code
 */
int
snd_mixer_selem_set_enum_item (snd_mixer_elem_t * elem,
			       snd_mixer_selem_channel_id_t channel,
			       unsigned int item)
{
  dbg_printf ("snd_mixer_selem_set_enum_item()\n");

  return 0;
}

/**
 * \brief Set value of playback switch control of a mixer simple element
 * \param elem Mixer simple element handle
 * \param channel mixer simple element channel identifier
 * \param value control value
 * \return 0 on success otherwise a negative error code
 */
int
snd_mixer_selem_set_playback_switch (snd_mixer_elem_t * elem,
				     snd_mixer_selem_channel_id_t channel,
				     int value)
{
  dbg_printf ("snd_mixer_selem_set_playback_switch()\n");

  return 0;
}

/**
 *  * \brief Return true if mixer simple element has only one volume control for both playback and capture
 *   * \param elem Mixer simple element handle
 *    * \return 0 separated control, 1 common control
 *     */
int
snd_mixer_selem_has_common_volume (snd_mixer_elem_t * elem)
{
  return 0;			// TODO
}

/**
 *  * \brief Return true if mixer simple element has only one switch control for both playback and capture
 *   * \param elem Mixer simple element handle
 *    * \return 0 separated control, 1 common control
 *     */
int
snd_mixer_selem_has_common_switch (snd_mixer_elem_t * elem)
{
  return 0;			// TODO
}
