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
 * \brief create a port - simple version
 * \param seq sequencer handle
 * \param name the name of the port
 * \param caps capability bits
 * \param type type bits
 * \return the created port number or negative error code
 *
 * Creates a port with the given capability and type bits.
 *
 * \sa snd_seq_create_port(), snd_seq_delete_simple_port()
 */
int
snd_seq_create_simple_port (snd_seq_t * seq, const char *name,
			    unsigned int caps, unsigned int type)
{
  snd_seq_port_info_t pinfo;
  int result;

  dbg_printf
    ("snd_seq_create_simple_port(seq=%x, name='%s', caps=%x, type=%x)\n", seq,
     name, caps, type);

  memset (&pinfo, 0, sizeof (pinfo));
  if (name)
    strncpy (pinfo.name, name, sizeof (pinfo.name) - 1);
  pinfo.capability = caps;
  pinfo.type = type;
  pinfo.midi_channels = 16;
  pinfo.midi_voices = 64;	/* XXX */
  pinfo.synth_voices = 0;	/* XXX */

  result = snd_seq_create_port (seq, &pinfo);
  if (result < 0)
    return result;
  else
    return pinfo.port;
}

/**
 * \brief delete the port
 * \param seq sequencer handle
 * \param port port id
 * \return 0 on success or negative error code
 *
 * \sa snd_seq_delete_port(), snd_seq_create_simple_port()
 */
int
snd_seq_delete_simple_port (snd_seq_t * seq, int port)
{
  dbg_printf ("snd_seq_delete_simple_port()\n");

  return 0;
}

/**
 * \brief set client name
 * \param seq sequencer handle
 * \param name name string
 * \return 0 on success or negative error code
 *
 * \sa snd_seq_set_client_info()
 */
int
snd_seq_set_client_name (snd_seq_t * seq, const char *name)
{
  snd_seq_client_info_t info;
  int err;

  dbg_printf ("snd_seq_set_client_name(seq=%x, name='%s')\n", seq, name);

  if ((err = snd_seq_get_client_info (seq, &info)) < 0)
    return err;
  strncpy (info.name, name, sizeof (info.name) - 1);
  return snd_seq_set_client_info (seq, &info);
}

/**
 * \brief change the output pool size of the given client
 * \param seq sequencer handle
 * \param size output pool size
 * \return 0 on success or negative error code
 *
 * \sa snd_seq_set_client_pool()
 */
int
snd_seq_set_client_pool_output (snd_seq_t * seq, size_t size)
{
  dbg_printf ("snd_seq_set_client_pool_output(seq=%x, size=%d)\n", seq, size);
  return 0;
}

/**
 * \brief parse the given string and get the sequencer address
 * \param seq sequencer handle
 * \param addr the address pointer to be returned
 * \param arg the string to be parsed
 * \return 0 on success or negative error code
 *
 * This function parses the sequencer client and port numbers from the given string.
 * The client and port tokes are separated by either colon or period, e.g. 128:1.
 * When \a seq is not NULL, the function accepts also a client name not only
 * digit numbers.
 */
int
snd_seq_parse_address (snd_seq_t * seq, snd_seq_addr_t * addr,
		       const char *arg)
{
  dbg_printf ("snd_seq_parse_address()\n");

  return 0;
}

/**
 * \brief wait until all events are processed
 * \param seq sequencer handle
 * \return 0 on success or negative error code
 *
 * This function waits until all events of this client are processed.
 *
 * \sa snd_seq_drain_output()
 */
int
snd_seq_sync_output_queue (snd_seq_t * seq)
{
  dbg_printf ("snd_seq_sync_output_queue()\n");

  return 0;
}

/**
 * \brief simple disconnection
 * \param myport the port id as sender
 * \param dest_client destination client id
 * \param dest_port destination port id
 * \return 0 on success or negative error code
 *
 * Remove connection from the given sender client:port
 * to the given destination port in the current client.
 *
 * \sa snd_seq_unsubscribe_port(), snd_seq_connect_to()
 */
int
snd_seq_disconnect_to (snd_seq_t * seq, int myport, int dest_client,
		       int dest_port)
{
  dbg_printf
    ("snd_seq_disconnect_to(seq=%x, myport=%d, dest_client=%d, dest_port=%d)\n",
     seq, myport, dest_client, dest_port);

  return 0;
}

/**
 * \brief change the input pool size of the given client
 * \param seq sequencer handle
 * \param size input pool size
 * \return 0 on success or negative error code
 *
 * \sa snd_seq_set_client_pool()
 */
int
snd_seq_set_client_pool_input (snd_seq_t * seq, size_t size)
{
  dbg_printf ("snd_seq_set_client_pool_input(seq=%x, size=%d)\n", seq, size);

  return 0;
}

/**
 * \brief change the output room size of the given client
 * \param seq sequencer handle
 * \param size output room size
 * \return 0 on success or negative error code
 *
 * \sa snd_seq_set_client_pool()
 */
int
snd_seq_set_client_pool_output_room (snd_seq_t * seq, size_t size)
{
  dbg_printf ("snd_seq_set_client_pool_output_room(seq=%x, size=%d)\n", seq,
	      size);

  return 0;
}
