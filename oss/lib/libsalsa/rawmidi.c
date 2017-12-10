/*
 *  Copyright (c) 2004 by Hannu Savolainen < hannu@opensound.com>
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

/**
 * \brief Opens a new connection to the RawMidi interface.
 * \param inputp Returned input handle (NULL if not wanted)
 * \param outputp Returned output handle (NULL if not wanted)
 * \param name ASCII identifier of the RawMidi handle
 * \param mode Open mode
 * \return 0 on success otherwise a negative error code
 *
 * Opens a new connection to the RawMidi interface specified with
 * an ASCII identifier and mode.
 */
int
snd_rawmidi_open (snd_rawmidi_t ** inputp, snd_rawmidi_t ** outputp,
		  const char *name, int mode)
{
  dbg_printf ("snd_rawmidi_open(%s)\n", name);

  ALIB_INIT ();

  if (!alib_appcheck ())
    return -ENODEV;

  return 0;
}

/**
 * \brief close RawMidi handle
 * \param rawmidi RawMidi handle
 * \return 0 on success otherwise a negative error code
 *
 * Closes the specified RawMidi handle and frees all associated
 * resources.
 */
int
snd_rawmidi_close (snd_rawmidi_t * rawmidi)
{
  dbg_printf ("snd_rawmidi_close()\n");

  return 0;
}

/**
 * \brief read MIDI bytes from MIDI stream
 * \param rawmidi RawMidi handle
 * \param buffer buffer to store the input MIDI bytes
 * \param size input buffer size in bytes
 */
ssize_t
snd_rawmidi_read (snd_rawmidi_t * rawmidi, void *buffer, size_t size)
{
  dbg_printf ("snd_rawmidi_read(%d)\n", size);
}

/**
 * \brief write MIDI bytes to MIDI stream
 * \param rawmidi RawMidi handle
 * \param buffer buffer containing MIDI bytes
 * \param size output buffer size in bytes
 */
ssize_t
snd_rawmidi_write (snd_rawmidi_t * rawmidi, const void *buffer, size_t size)
{
  dbg_printf ("snd_rawmidi_write(%d)\n", size);

  return size;
}


/**
 * \brief set nonblock mode
 * \param rawmidi RawMidi handle
 * \param nonblock 0 = block, 1 = nonblock mode
 * \return 0 on success otherwise a negative error code
 *
 * The nonblock mode cannot be used when the stream is in
 * #SND_RAWMIDI_APPEND state.
 */
int
snd_rawmidi_nonblock (snd_rawmidi_t * rawmidi, int nonblock)
{
  dbg_printf ("snd_rawmidi_nonblock(%d)\n", nonblock);

  return 0;
}

/**
 * \brief get count of poll descriptors for RawMidi handle
 * \param rawmidi RawMidi handle
 * \return count of poll descriptors
 */
int
snd_rawmidi_poll_descriptors_count (snd_rawmidi_t * rawmidi)
{
  dbg_printf ("snd_rawmidi_poll_descriptors_count()\n");

  return 0;
}

/**
 * \brief get poll descriptors
 * \param rawmidi RawMidi handle
 * \param pfds array of poll descriptors
 * \param space space in the poll descriptor array
 * \return count of filled descriptors
 */
int
snd_rawmidi_poll_descriptors (snd_rawmidi_t * rawmidi, struct pollfd *pfds,
			      unsigned int space)
{
  dbg_printf ("snd_rawmidi_poll_descriptors()\n");

  return 0;
}

/**
 * \brief get returned events from poll descriptors
 * \param pcm rawmidi RawMidi handle
 * \param pfds array of poll descriptors
 * \param nfds count of poll descriptors
 * \param revents returned events
 * \return zero if success, otherwise a negative error code
 */
int
snd_rawmidi_poll_descriptors_revents (snd_rawmidi_t * rawmidi, struct pollfd
				      *pfds, unsigned int nfds,
				      unsigned short *revents)
{
  dbg_printf ("snd_rawmidi_poll_descriptors_revents()\n");

  return 0;
}

/**
 * \brief get size of the snd_rawmidi_info_t structure in bytes
 * \return size of the snd_rawmidi_info_t structure in bytes
 */
size_t
snd_rawmidi_info_sizeof ()
{
  return sizeof (snd_rawmidi_info_t);
}

/**
 * \brief get rawmidi hardware driver name
 * \param info pointer to a snd_rawmidi_info_t structure
 * \return rawmidi hardware driver name
 */
const char *
snd_rawmidi_info_get_name (const snd_rawmidi_info_t * info)
{
  dbg_printf ("snd_rawmidi_info_get_name()\n");

  return "OSS rawmidi";
}

/**
 * \brief get rawmidi count of subdevices
 * \param info pointer to a snd_rawmidi_info_t structure
 * \return rawmidi count of subdevices
 */
unsigned int
snd_rawmidi_info_get_subdevices_count (const snd_rawmidi_info_t * info)
{
  dbg_printf ("snd_rawmidi_info_get_subdevices_count()\n");

  return 1;
}

/**
 * \brief set rawmidi device number
 * \param info pointer to a snd_rawmidi_info_t structure
 * \param val device number
 */
void
snd_rawmidi_info_set_device (snd_rawmidi_info_t * info, unsigned int val)
{
  dbg_printf ("snd_rawmidi_info_set_device(%d)\n", val);

}


/**
 * \brief set rawmidi subdevice number
 * \param info pointer to a snd_rawmidi_info_t structure
 * \param val subdevice number
 */
void
snd_rawmidi_info_set_subdevice (snd_rawmidi_info_t * info, unsigned int val)
{
  dbg_printf ("snd_rawmidi_info_set_subdevice(%d)\n", val);
}

/**
 * \brief set rawmidi stream identifier
 * \param info pointer to a snd_rawmidi_info_t structure
 * \param val rawmidi stream identifier
 */
void
snd_rawmidi_info_set_stream (snd_rawmidi_info_t * info,
			     snd_rawmidi_stream_t val)
{
  dbg_printf ("snd_rawmidi_info_set_stream(%d)\n", val);
}

/**
 * \brief get rawmidi subdevice name
 * \param info pointer to a snd_rawmidi_info_t structure
 * \return rawmidi subdevice name
 */
const char *
snd_rawmidi_info_get_subdevice_name (const snd_rawmidi_info_t * info)
{
  dbg_printf ("snd_rawmidi_info_get_subdevice_name()\n");

  return "OSS rawmidi subdevice";
}
