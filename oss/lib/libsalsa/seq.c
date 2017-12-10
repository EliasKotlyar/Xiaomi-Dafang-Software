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

static int nports = 0;

typedef struct _snd_seq_port_subscribe
{
  int dummy;
} snd_seq_port_subscribe_t;

typedef struct _snd_seq_queue_tempo
{
  int dummy;
} snd_seq_queue_tempo_t;

typedef struct _snd_seq_system_info
{
  int dummy;
} snd_seq_system_info_t;

struct _snd_seq_remove_events
{
  int dummy;
};

struct _snd_seq_query_subscribe
{
  int dummy;
};

/**
 * \brief Open the ALSA sequencer
 *
 * \param seqp Pointer to a snd_seq_t pointer.  This pointer must be
 * kept and passed to most of the other sequencer functions.
 * \param name The sequencer's "name".  This is \em not a name you make
 * up for your own purposes; it has special significance to the ALSA
 * library.  Usually you need to pass \c "default" here.
 * \param streams The read/write mode of the sequencer.  Can be one of
 * three values:
 * - #SND_SEQ_OPEN_OUTPUT - open the sequencer for output only
 * - #SND_SEQ_OPEN_INPUT - open the sequencer for input only
 * - #SND_SEQ_OPEN_DUPLEX - open the sequencer for output and input
 * \note Internally, these are translated to \c O_WRONLY, \c O_RDONLY and
 * \c O_RDWR respectively and used as the second argument to the C library
 * open() call.
 * \param mode Optional modifier.  Can be either 0, or
 * #SND_SEQ_NONBLOCK, which will make read/write operations
 * non-blocking.  This can also be set later using #snd_seq_nonblock().
 * \return 0 on success otherwise a negative error code
 *
* Creates a new handle and opens a connection to the kernel
 * sequencer interface.
 * After a client is created successfully, an event
 * with #SND_SEQ_EVENT_CLIENT_START is broadcast to announce port.
 *
 * \sa snd_seq_open_lconf(), snd_seq_close(), snd_seq_type(), snd_seq_name(),
 *     snd_seq_nonblock(), snd_seq_client_id()
 */
int
snd_seq_open (snd_seq_t ** seqp, const char *name, int streams, int mode)
{
  snd_seq_t *seq;
  int err, oss_mode;
  static int instance = 0;

  char *fname = "/dev/midi", *envname = NULL, tmp[128];

  ALIB_INIT ();

  dbg_printf ("snd_seq_open(name='%s', streams=%d, mode=%x)\n", name, streams,
	      mode);

  if (!alib_appcheck ())
    return -ENODEV;

  instance++;

  sprintf (tmp, "%s_mididev%d", alib_appname, instance);

  if ((envname = getenv (tmp)) != NULL)
    {
      fname = envname;
    }
#if 0
  if (streams != 1)
    {
      fprintf (stderr, "salsa: snd_seq_open doesn't support streams=%d\n",
	       streams);
      return -EIO;
    }
#endif

  if ((seq = malloc (sizeof (*seq))) == NULL)
    return -ENOMEM;

  dbg_printf ("Created sequencer seq=%x\n", seq);

  memset (seq, 0, sizeof (*seq));

  switch (streams)
    {
    case SND_SEQ_OPEN_INPUT:
      oss_mode = O_RDONLY;
      dbg_printf ("Open SND_SEQ_OPEN_INPUT\n");
      break;
    case SND_SEQ_OPEN_OUTPUT:
      oss_mode = O_WRONLY;
      dbg_printf ("Open SND_SEQ_OPEN_OUTPUT\n");
      break;
    case SND_SEQ_OPEN_DUPLEX:
      dbg_printf ("SND_SEQ_OPEN_DUPLEX\n");
      oss_mode = O_RDWR;
      break;

    default:
      fprintf (stderr, "snd_seq_open: Unknown stream %x\n", streams);
      return -ENODEV;
    }

  if ((seq->fd = open (fname, oss_mode, 0)) == -1)
    {
      err = errno;
      perror (fname);

      if (envname == NULL)
	{
	  fprintf (stderr,
		   "You can select another filename using environment variable %s_mididev%d\n",
		   alib_appname, instance);
	}
      return -err;
    }

  seq->streams = streams;
  seq->oss_mode = oss_mode;

  if (streams == SND_SEQ_OPEN_INPUT || streams == SND_SEQ_OPEN_DUPLEX)
    {
      seq->parser = midiparser_create (midiparser_callback, seq);
      if (seq->parser == NULL)
	{
	  fprintf (stderr, "libsalsa: Can't create MIDI parser\n");
	  return -ENODEV;
	}
    }

  *seqp = seq;
  return 0;
}

/**
 * \brief Close the sequencer
 * \param handle Handle returned from #snd_seq_open()
 * \return 0 on success otherwise a negative error code
 *
 * Closes the sequencer client and releases its resources.
 * After a client is closed, an event with
 * #SND_SEQ_EVENT_CLIENT_EXIT is broadcast to announce port.
 * The connection between other clients are disconnected.
 * Call this just before exiting your program.
 *
 * \sa snd_seq_close()
 */
int
snd_seq_close (snd_seq_t * seq)
{
  dbg_printf ("snd_seq_close(seq=%x)\n", seq);

  close (seq->fd);
  free (seq);
}

/**
 * \brief Set nonblock mode
 * \param seq sequencer handle
 * \param nonblock 0 = block, 1 = nonblock mode
 * \return 0 on success otherwise a negative error code
 *
 * Change the blocking mode of the given client.
 * In block mode, the client falls into sleep when it fills the
 * output memory pool with full events.  The client will be woken up
 * after a certain amount of free space becomes available.
 *
 * \sa snd_seq_open()
 */
int
snd_seq_nonblock (snd_seq_t * seq, int nonblock)
{
  dbg_printf ("snd_seq_nonblock(seq=%x, nonblock=%d)\n", seq, nonblock);

  seq->nonblock = nonblock;
  return 0;
}

/**
 * \brief create a sequencer port on the current client
 * \param seq sequencer handle
 * \param port port information for the new port
 * \return 0 on success otherwise a negative error code
 *
 * Creates a sequencer port on the current client.
 * The attributes of created port is specified in \a info argument.
 *
 * The client field in \a info argument is overwritten with the current client id.
 * The port id to be created can be specified via #snd_seq_port_info_set_port_specified.
 * You can get the created port id by reading the port pointer via #snd_seq_port_info_get_port.
 *
 * Each port has the capability bit-masks to specify the access capability
 * of the port from other clients.
 * The capability bit flags are defined as follows:
 * - #SND_SEQ_PORT_CAP_READ Readable from this port
 * - #SND_SEQ_PORT_CAP_WRITE Writable to this port.
 * - #SND_SEQ_PORT_CAP_SYNC_READ For synchronization (not implemented)
 * - #SND_SEQ_PORT_CAP_SYNC_WRITE For synchronization (not implemented)
 * - #SND_SEQ_PORT_CAP_DUPLEX Read/write duplex access is supported
 * - #SND_SEQ_PORT_CAP_SUBS_READ Read subscription is allowed
 * - #SND_SEQ_PORT_CAP_SUBS_WRITE Write subscription is allowed
 * - #SND_SEQ_PORT_CAP_SUBS_NO_EXPORT Subscription management from 3rd client is disallowed
 *
 * Each port has also the type bitmasks defined as follows:
 * - #SND_SEQ_PORT_TYPE_SPECIFIC Hardware specific port
 * - #SND_SEQ_PORT_TYPE_MIDI_GENERIC Generic MIDI device
 * - #SND_SEQ_PORT_TYPE_MIDI_GM General MIDI compatible device
 * - #SND_SEQ_PORT_TYPE_MIDI_GS GS compatible device
 * - #SND_SEQ_PORT_TYPE_MIDI_XG XG compatible device
 * - #SND_SEQ_PORT_TYPE_MIDI_MT32 MT-32 compatible device
 * - #SND_SEQ_PORT_TYPE_SYNTH Synth device
 * - #SND_SEQ_PORT_TYPE_DIRECT_SAMPLE Sampling device (supporting download)
 * - #SND_SEQ_PORT_TYPE_SAMPLE Sampling device (sample can be downloaded at any time)
 * - #SND_SEQ_PORT_TYPE_APPLICATION Application (sequencer/editor)
 *
 * A port may contain specific midi channels, midi voices and synth voices.
 * These values could be zero as default.
 *
 * \sa snd_seq_delete_port(), snd_seq_get_port_info(),
 *     snd_seq_create_simple_port()
 */
int
snd_seq_create_port (snd_seq_t * seq, snd_seq_port_info_t * port)
{
  dbg_printf ("snd_seq_create_port(seq=%x, port=%x)\n", seq, port);

  port->port = nports++;
  return 0;
}

/**
 * \brief Get the client id
 * \param seq sequencer handle
 * \return the client id
 *
 * Returns the id of the specified client.
 * If an error occurs, function returns the negative error code.
 * A client id is necessary to inquiry or to set the client information.
 * A user client is assigned from 128 to 191.
 *
 * \sa snd_seq_open()
 */
int
snd_seq_client_id (snd_seq_t * seq)
{
  static int client_id = 128;

  dbg_printf ("snd_seq_client_id(seq=%x)=%d\n", seq, client_id);

  return client_id++;
}

/**
 * \brief Get client id of a client_info container
 * \param info client_info container
 * \return client id
 *
 * \sa snd_seq_get_client_info(), snd_seq_client_info_set_client(), snd_seq_client_id()
 */
int
snd_seq_client_info_get_client (const snd_seq_client_info_t * info)
{
  dbg_printf ("snd_seq_client_info_get_client()\n");

  return 0;
}

/**
 * \brief Get client type of a client_info container
 * \param info client_info container
 * \return client type
 *
 * The client type is either #SEQ_CLIENT_TYPE_KERNEL or #SEQ_CLIENT_TYPE_USER
 * for kernel or user client respectively.
 *
 * \sa snd_seq_get_client_info()
 */
snd_seq_client_type_t
snd_seq_client_info_get_type (const snd_seq_client_info_t * info)
{
  dbg_printf ("snd_seq_client_info_get_type(infp=%x)\n", info);

  return SND_SEQ_KERNEL_CLIENT;	// TODO
}

/**
 * \brief Set the client id of a client_info container
 * \param info client_info container
 * \param client client id
 *
 * \sa snd_seq_get_client_info(), snd_seq_client_info_get_client()
 */
void
snd_seq_client_info_set_client (snd_seq_client_info_t * info, int client)
{
  dbg_printf ("snd_seq_client_info_set_client(%x, %d)\n", info, client);

  info->client = client;
}

/**
 * \brief get size of #snd_seq_client_info_t
 * \return size in bytes
 */
size_t
snd_seq_client_info_sizeof ()
{
  dbg_printf ("snd_seq_client_info_sizeof()\n");

  return sizeof (snd_seq_client_info_t);
}

/**
 * \brief retrieve an event from sequencer
 * \param seq sequencer handle
 * \param ev event pointer to be stored
 * \return
 *
 * Obtains an input event from sequencer.
 * The event is created via snd_seq_create_event(), and its pointer is stored on * ev argument.
 *
 * This function firstly receives the event byte-stream data from sequencer
 * as much as possible at once.  Then it retrieves the first event record
 * and store the pointer on ev.
 * By calling this function sequentially, events are extracted from the input buffer.
 *
 * If there is no input from sequencer, function falls into sleep
 * in blocking mode until an event is received,
 * or returns \c -EAGAIN error in non-blocking mode.
 * Occasionally, this function may return \c -ENOSPC error.
 * This means that the input FIFO of sequencer overran, and some events are
 * lost.
 * Once this error is returned, the input FIFO is cleared automatically.
 *
 * Function returns the byte size of remaining events on the input buffer
 * if an event is successfully received.
 * Application can determine from the returned value whether to call
 * input once more or not.
 *
 * \sa snd_seq_event_input_pending(), snd_seq_drop_input()
 */
int
snd_seq_event_input (snd_seq_t * seq, snd_seq_event_t ** evp)
{
  static snd_seq_event_t *ev;
  unsigned char buf[256];
  int i, l;

  dbg_printf2 ("snd_seq_event_input(seq=%x)\n", seq);

  while (1)
    {

      if (seq->nextevent < seq->nevents)
	{
	  *evp = &seq->events[seq->nextevent++];

	  return (seq->nevents - seq->nextevent) * sizeof (snd_seq_event_t);
	}

      seq->nextevent = seq->nevents = 0;
      memset (seq->events, 0, sizeof (seq->events));

      // TODO Handling of nonblocking mode
      if ((l = read (seq->fd, buf, sizeof (buf))) == -1)
	return -errno;

      midiparser_input_buf (seq->parser, buf, l);

    }

}

/**
 * \brief output an event directly to the sequencer NOT through output buffer
 * \param seq sequencer handle
 * \param ev event to be output
 * \return the byte size sent to sequencer or a negative error code
 *
 * This function sends an event to the sequencer directly not through the
 * output buffer.  When the event is a variable length event, a temporary
 * buffer is allocated inside alsa-lib and the data is copied there before
 * actually sent.
 *
 * \sa snd_seq_event_output()
 */
int
snd_seq_event_output_direct (snd_seq_t * seq, snd_seq_event_t * ev)
{
  int err;

  dbg_printf3 ("snd_seq_event_output_direct()\n");

  if ((err = convert_event (seq, ev)) < 0)
    return err;
  if ((err = snd_seq_drain_output (seq)) < 0)
    return err;

  return 0;
}

/**
 * \brief (DEPRECATED) free an event
 *
 * In the former version, this function was used to
 * release the event pointer which was allocated by snd_seq_event_input().
 * In the current version, the event record is not allocated, so
 * you don't have to call this function any more.
 */
int
snd_seq_free_event (snd_seq_event_t * ev)
{
  return 0;
}

/**
 * \brief obtain the current client information
 * \param seq sequencer handle
 * \param info the pointer to be stored
 * \return 0 on success otherwise a negative error code
 *
 * Obtains the information of the current client stored on info.
 * client and type fields are ignored.
 *
 * \sa snd_seq_get_any_client_info(), snd_seq_set_client_info(),
 *     snd_seq_query_next_client()
 */
int
snd_seq_get_client_info (snd_seq_t * seq, snd_seq_client_info_t * info)
{
  dbg_printf ("snd_seq_get_client_info(seq=%x, info=%x)\n", seq, info);

  return 0;
}

/**
 * \brief query the next matching port
 * \param seq sequencer handle
 * \param info query pattern and result
                                                                                
 * Queries the next matching port on the client specified in
 * \a info argument.
 * The search begins at the next port specified in
 * port field of \a info argument.
 * For finding the first port at a certain client, give -1.
 *
 * If a matching port is found, its attributes are stored on
 * \a info and function returns zero.
 * Otherwise, a negative error code is returned.
 *
 * \sa snd_seq_get_port_info()
 */
int
snd_seq_query_next_port (snd_seq_t * seq, snd_seq_port_info_t * info)
{
  dbg_printf ("snd_seq_query_next_port()\n");

  return -EINVAL;
}

/**
 * \brief Set the name of a client_info container
 * \param info client_info container
 * \param name name string
 *
 * \sa snd_seq_get_client_info(), snd_seq_client_info_get_name(),
 *     snd_seq_set_client_name()
 */
void
snd_seq_client_info_set_name (snd_seq_client_info_t * info, const char *name)
{
  dbg_printf ("snd_seq_client_info_set_name(%s)\n", name);

  strncpy (info->name, name, sizeof (info->name) - 1);
  info->name[sizeof (info->name) - 1] = 0;
}

/**
 * \brief subscribe a port connection
 * \param seq sequencer handle
 * \param sub subscription information
 * \return 0 on success otherwise a negative error code
 *
 * Subscribes a connection between two ports.
 * The subscription information is stored in sub argument.
 *
 * \sa snd_seq_get_port_subscription(), snd_seq_unsubscribe_port(),
 *     snd_seq_connect_from(), snd_seq_connect_to()
 */
int
snd_seq_subscribe_port (snd_seq_t * seq, snd_seq_port_subscribe_t * sub)
{
  dbg_printf ("snd_seq_subscribe_port()\n");

  return -EINVAL;
}

/**
 * \brief get size of #snd_seq_port_subscribe_t
 * \return size in bytes
 */
size_t
snd_seq_port_subscribe_sizeof ()
{
  return sizeof (snd_seq_port_subscribe_t);
}

/**
 * \brief Set sender address of a port_subscribe container
 * \param info port_subscribe container
 * \param addr sender address
 *
 * \sa snd_seq_subscribe_port(), snd_seq_port_subscribe_get_sender()
 */
void
snd_seq_port_subscribe_set_sender (snd_seq_port_subscribe_t * info,
				   const snd_seq_addr_t * addr)
{
  dbg_printf ("snd_seq_port_subscribe_set_sender()\n");
}

/**
 * \brief Set destination address of a port_subscribe container
 * \param info port_subscribe container
 * \param addr destination address
 *
 * \sa snd_seq_subscribe_port(), snd_seq_port_subscribe_get_dest()
 */
void
snd_seq_port_subscribe_set_dest (snd_seq_port_subscribe_t * info,
				 const snd_seq_addr_t * addr)
{
  dbg_printf ("snd_seq_port_subscribe_set_dest()\n");
}

/**
 * \brief Set the queue id of a port_subscribe container
 * \param info port_subscribe container
 * \param q queue id
 *
 * \sa snd_seq_subscribe_port(), snd_seq_port_subscribe_get_queue()
 */
void
snd_seq_port_subscribe_set_queue (snd_seq_port_subscribe_t * info, int q)
{
  dbg_printf ("snd_seq_port_subscribe_set_queue()\n");
}

/**
 * \brief Set the real-time mode of a port_subscribe container
 * \param info port_subscribe container
 * \param val non-zero to enable
 *
 * \sa snd_seq_subscribe_port(), snd_seq_port_subscribe_get_time_real()
 */
void
snd_seq_port_subscribe_set_time_real (snd_seq_port_subscribe_t * info,
				      int val)
{
  dbg_printf ("snd_seq_port_subscribe_set_time_real()\n");
}

/**
 * \brief Set the time-update mode of a port_subscribe container
 * \param info port_subscribe container
 * \param val non-zero to enable
 *
 * \sa snd_seq_subscribe_port(), snd_seq_port_subscribe_get_time_update()
 */
void
snd_seq_port_subscribe_set_time_update (snd_seq_port_subscribe_t * info, int
					val)
{
  dbg_printf ("snd_seq_port_subscribe_set_time_update()\n");
}

/*
 * Port
 */

/**
 * \brief get size of #snd_seq_port_info_t
 * \return size in bytes
 */
size_t
snd_seq_port_info_sizeof ()
{
  return sizeof (snd_seq_port_info_t);
}

/**
 * \brief Get the capability bits of a port_info container
 * \param info port_info container
 * \return capability bits
 *
 * \sa snd_seq_get_port_info(), snd_seq_port_info_set_capability()
 */
unsigned int
snd_seq_port_info_get_capability (const snd_seq_port_info_t * info)
{
  dbg_printf ("snd_seq_port_info_get_capability()\n");

  return info->capability;
}

/**
 * \brief Get client/port address of a port_info container
 * \param info port_info container
 * \return client/port address pointer
 *
 * \sa snd_seq_get_port_info(), snd_seq_port_info_set_addr()
 */
const snd_seq_addr_t *
snd_seq_port_info_get_addr (const snd_seq_port_info_t * info)
{
  dbg_printf ("snd_seq_port_info_get_addr(info=%x)\n", info);

  return NULL;			// TODO
}

/**
 * \brief set the capability bits of a port_info container
 * \param info port_info container
 * \param capability capability bits
 *
 * \sa snd_seq_get_port_info(), snd_seq_port_info_get_capability()
 */
void
snd_seq_port_info_set_capability (snd_seq_port_info_t * info,
				  unsigned int capability)
{
  dbg_printf ("snd_seq_port_info_set_capability()\n");
}

/**
 * \brief Set the port-specified mode of a port_info container
 * \param info port_info container
 * \param val non-zero if specifying the port id at creation
 *
 * \sa snd_seq_get_port_info(), snd_seq_port_info_get_port_specified()
 */
void
snd_seq_port_info_set_port_specified (snd_seq_port_info_t * info, int val)
{
  dbg_printf ("snd_seq_port_info_set_port_specified()\n");
}

/**
 * \brief Get the midi channels of a port_info container
 * \param info port_info container
 * \return number of midi channels (default 0)
 *
 * \sa snd_seq_get_port_info(), snd_seq_port_info_set_midi_channels()
 */
int
snd_seq_port_info_get_midi_channels (const snd_seq_port_info_t * info)
{
  dbg_printf ("snd_seq_port_info_get_midi_channels(info=%x)=%d\n",
	      info, info->midi_channels);

  return info->midi_channels;
}

/**
 * \brief set the midi channels of a port_info container
 * \param info port_info container
 * \param channels midi channels (default 0)
 *
 * \sa snd_seq_get_port_info(), snd_seq_port_info_get_midi_channels()
 */
void
snd_seq_port_info_set_midi_channels (snd_seq_port_info_t * info, int channels)
{
  dbg_printf ("snd_seq_port_info_set_midi_channels(info=%x, channels=%d)\n",
	      info, channels);
  info->midi_channels = channels;
}

/**
 * \brief Get the queue id to update timestamps
 * \param info port_info container
 * \return the queue id to get the timestamps
 *
 * \sa snd_seq_get_port_info(), snd_seq_port_info_set_timestamp_queue()
 */
int
snd_seq_port_info_get_timestamp_queue (const snd_seq_port_info_t * info)
{
  dbg_printf ("snd_seq_port_info_get_timestamp_queue(info=%x)\n", info);

  return 0;			// TODO
}

/**
 * \brief Set the queue id for timestamping
 * \param info port_info container
 * \param queue the queue id to get timestamps
 *
 * \sa snd_seq_get_port_info(), snd_seq_port_info_get_timestamp_queue()
 */
void
snd_seq_port_info_set_timestamp_queue (snd_seq_port_info_t * info, int queue)
{
  dbg_printf ("snd_seq_port_info_set_timestamp_queue(info=%x, queue=%d)\n",
	      info, queue);

  // TODO
}

/**
 * \brief Get whether the time-stamping of the given port is real-time mode
 * \param info port_info container
 * \return 1 if the time-stamping is in the real-time mode
 *
 * \sa snd_seq_get_port_info(), snd_seq_port_info_set_timestamp_real()
 */
int
snd_seq_port_info_get_timestamp_real (const snd_seq_port_info_t * info)
{
  dbg_printf ("snd_seq_port_info_get_timestamp_real(info=%x)\n", info);

  return 0;			// TODO
}

/**
 * \brief Set whether the timestime is updated in the real-time mode
 * \param info port_info container
 * \param enable non-zero if updating the timestamps in real-time mode
 *
 * \sa snd_seq_get_port_info(), snd_seq_port_info_get_timestamp_real()
 */
void
snd_seq_port_info_set_timestamp_real (snd_seq_port_info_t * info, int enable)
{
  dbg_printf ("snd_seq_port_info_set_timestamp_real(infp=%x, enable=%d)\n",
	      info, enable);
}

/**
 * \brief Get the time-stamping mode of the given port in a port_info container * \param info port_info container
 * \return 1 if the port updates timestamps of incoming events
 *
 * \sa snd_seq_get_port_info(), snd_seq_port_info_set_timestamping()
 */
int
snd_seq_port_info_get_timestamping (const snd_seq_port_info_t * info)
{
  dbg_printf ("snd_seq_port_info_get_timestamping(info=%x)\n", info);

  return 0;			// TODO
}

/**
 * \brief Set the time-stamping mode of the given port
 * \param info port_info container
 * \param enable non-zero if updating the timestamps of incoming events
 *
 * \sa snd_seq_get_port_info(), snd_seq_port_info_get_timestamping()
 */
void
snd_seq_port_info_set_timestamping (snd_seq_port_info_t * info, int enable)
{
  dbg_printf ("snd_seq_port_info_set_timestamping(info=%x, enable=%d)\n",
	      info, enable);

  // TODO
}

/**
 * \brief Set the name of a port_info container
 * \param info port_info container
 * \param name name string
 *
 * \sa snd_seq_get_port_info(), snd_seq_port_info_get_name()
 */
void
snd_seq_port_info_set_name (snd_seq_port_info_t * info, const char *name)
{
  dbg_printf ("snd_seq_port_info_set_name()\n");

  strncpy (info->name, name, sizeof (info->name));
}

/**
 * \brief Get the name of a port_info container
 * \param info port_info container
 * \return name string
 *
 * \sa snd_seq_get_port_info(), snd_seq_port_info_set_name()
 */
const char *
snd_seq_port_info_get_name (const snd_seq_port_info_t * info)
{
  dbg_printf ("snd_seq_port_info_get_name()\n");

  return "Port Name";
}

/**
 * \brief Get port id of a port_info container
 * \param info port_info container
 * \return port id
 *
 * \sa snd_seq_get_port_info(), snd_seq_port_info_set_port()
 */
int
snd_seq_port_info_get_port (const snd_seq_port_info_t * info)
{
  dbg_printf ("snd_seq_port_info_get_port()\n");

  return -EIO;
}

/**
 * \brief Get the type bits of a port_info container
 * \param info port_info container
 * \return port type bits
 *
 * \sa snd_seq_get_port_info(), snd_seq_port_info_set_type()
 */
unsigned int
snd_seq_port_info_get_type (const snd_seq_port_info_t * info)
{
  dbg_printf ("snd_seq_port_info_get_type()\n");

  return info->type;
}

/**
 * \brief Set the client id of a port_info container
 * \param info port_info container
 * \param client client id
 *
 * \sa snd_seq_get_port_info(), snd_seq_port_info_get_client()
 */
void
snd_seq_port_info_set_client (snd_seq_port_info_t * info, int client)
{
  dbg_printf ("snd_seq_port_info_set_client()\n");
}

/**
 * \brief Set the port id of a port_info container
 * \param info port_info container
 * \param port port id
 *
 * \sa snd_seq_get_port_info(), snd_seq_port_info_get_port()
 */
void
snd_seq_port_info_set_port (snd_seq_port_info_t * info, int port)
{
  dbg_printf ("snd_seq_port_info_set_port()\n");
}

/**
 * \brief query the next matching client
 * \param seq sequencer handle
 * \param info query pattern and result
 *
 * Queries the next matching client with the given condition in
 * info argument.
 * The search begins at the client with an id one greater than
 * client field in info.
 * If name field in info is not empty, the client name is compared.
 * If a matching client is found, its attributes are stored o
 * info and returns zero.
 * Otherwise returns a negative error code.
 *
 * \sa snd_seq_get_any_client_info()
 */
int
snd_seq_query_next_client (snd_seq_t * seq, snd_seq_client_info_t * info)
{
  dbg_printf ("snd_seq_query_next_client()\n");

  return -EIO;
}

/**
 * \brief set the current client information
 * \param seq sequencer handle
 * \param info the client info data to set
 * \return 0 on success otherwise a negative error code
 *
 * Obtains the information of the current client stored on info.
 * client and type fields are ignored.
 *
 * \sa snd_seq_get_client_info()
 */
int
snd_seq_set_client_info (snd_seq_t * seq, snd_seq_client_info_t * info)
{
  dbg_printf ("snd_seq_set_client_info(seq=%x, info=%x)\n", seq, info);

  if (*info->name)
    {
      strcpy (seq->name, info->name);
      ioctl (seq->fd, SNDCTL_SETNAME, seq->name);
      ioctl (seq->fd, SNDCTL_SETSONG, seq->name);
    }

  return 0;
}

/**
 * \brief check events in input buffer
 * \return the byte size of remaining input events on input buffer.
 *
 * If events remain on the input buffer of user-space, function returns
 * the total byte size of events on it.
 * If fetch_sequencer argument is non-zero,
 * this function checks the presence of events on sequencer FIFO
 * When events exist, they are transferred to the input buffer,
 * and the number of received events are returned.
 * If fetch_sequencer argument is zero and
 * no events remain on the input buffer, function simply returns zero.
 *
 * \sa snd_seq_event_input()
 */
int
snd_seq_event_input_pending (snd_seq_t * seq, int fetch_sequencer)
{
  dbg_printf ("snd_seq_event_input_pending()\n");

  return 0;
}

/**
 * \brief Get poll descriptors
 * \param seq sequencer handle
 * \param pfds array of poll descriptors
 * \param space space in the poll descriptor array
 * \param events polling events to be checked (\c POLLIN and \c POLLOUT)
 * \return count of filled descriptors
 *
 * Get poll descriptors assigned to the sequencer handle.
 * Since a sequencer handle can duplex streams, you need to set which direction(s)
 * is/are polled in \a events argument.  When \c POLLIN bit is specified,
 * the incoming events to the ports are checked.
 *
 * To check the returned poll-events, call #snd_seq_poll_descriptors_revents()
 * instead of reading the pollfd structs directly.
 *
 * \sa snd_seq_poll_descriptors_count(), snd_seq_poll_descriptors_revents()
 */
int
snd_seq_poll_descriptors (snd_seq_t * seq, struct pollfd *pfds,
			  unsigned int space, short events)
{
  dbg_printf ("snd_seq_poll_descriptors(seq=%x)\n", seq);

  pfds->fd = seq->fd;
  pfds->events = 0;
  pfds->revents = 0;

  if (seq->oss_mode == O_WRONLY || seq->oss_mode == O_RDWR)
    pfds->events = POLLOUT;
  if (seq->oss_mode == O_RDONLY || seq->oss_mode == O_RDWR)
    pfds->events = POLLIN;

  return 1;
}


/**
 * \brief Returns the number of poll descriptors
 * \param seq sequencer handle
 * \param events the poll events to be checked (\c POLLIN and \c POLLOUT)
 * \return the number of poll descriptors.
 *
 * Get the number of poll descriptors.  The polling events to be checked
 * can be specified by the second argument.  When both input and output
 * are checked, pass \c POLLIN|POLLOUT
 *
 * \sa snd_seq_poll_descriptors()
 */
int
snd_seq_poll_descriptors_count (snd_seq_t * seq, short events)
{
  dbg_printf ("snd_seq_poll_descriptors_count(seq=%x, events=%x)\n",
	      seq, events);
  return 1;
}

/**
 * \brief create a queue
 * \param seq sequencer handle
 * \param info queue information to initialize
 * \return the queue id (zero or positive) on success otherwise a negative error code
 *
 * \sa snd_seq_alloc_queue()
 */
int
snd_seq_create_queue (snd_seq_t * seq, snd_seq_queue_info_t * info)
{
  dbg_printf ("snd_seq_create_queue()\n");

  return 0;
}

/**
 * \brief simple subscription (w/o exclusive & time conversion)
 * \param myport the port id as sender
 * \param dest_client destination client id
 * \param dest_port destination port id
 * \return 0 on success or negative error code
 *
 * Connect from the given receiver port in the current client
 * to the given destination client:port.
 *
 * \sa snd_seq_subscribe_port(), snd_seq_disconnect_to()
 */
int
snd_seq_connect_to (snd_seq_t * seq, int myport, int dest_client,
		    int dest_port)
{
  dbg_printf ("snd_seq_connect_to(%d->%d/%d)\n", myport, dest_client,
	      dest_port);

  return 0;
}

/**
 * \brief simple subscription (w/o exclusive & time conversion)
 * \param myport the port id as receiver
 * \param src_client sender client id
 * \param src_port sender port id
 * \return 0 on success or negative error code
 *
 * Connect from the given sender client:port to the given destination port in the
 * current client.
 *
 * \sa snd_seq_subscribe_port(), snd_seq_disconnect_from()
 */
int
snd_seq_connect_from (snd_seq_t * seq, int myport, int src_client,
		      int src_port)
{
  dbg_printf
    ("snd_seq_connect_from(seq=%x, myport=%d, src_client=%d, src_port=%d)\n",
     seq, myport, src_client, src_port);

  return 0;
}

/**
 * \brief queue controls - start/stop/continue
 * \param seq sequencer handle
 * \param q queue id to control
 * \param type event type
 * \param value event value
 * \param ev event instance
 *
 * This function sets up general queue control event and sends it.
 * To send at scheduled time, set the schedule in \a ev.
 * If \a ev is NULL, the event is composed locally and sent immediately
 * to the specified queue.  In any cases, you need to call #snd_seq_drain_output()
 * appropriately to feed the event.
 *
 * \sa snd_seq_alloc_queue()
 */
int
snd_seq_control_queue (snd_seq_t * seq, int q, int type, int value,
		       snd_seq_event_t * ev)
{
  dbg_printf ("snd_seq_control_queue()\n");

  return 0;
}

/**
 * \brief drain output buffer to sequencer
 * \param seq sequencer handle
 * \return 0 when all events are drained and sent to sequencer.
 *         When events still remain on the buffer, the byte size of remaining
 *         events are returned.  On error a negative error code is returned.
 *
 * This function drains all pending events on the output buffer.
 * The function returns immediately after the events are sent to the queues
 * regardless whether the events are processed or not.
 * To get synchronization with the all event processes, use
 * #snd_seq_sync_output_queue() after calling this function.
 *
 * \sa snd_seq_event_output(), snd_seq_sync_output_queue()
 */
int
snd_seq_drain_output (snd_seq_t * seq)
{
  dbg_printf3 ("snd_seq_drain_output(seq=%x)\n", seq);

  return 0;
}


/**
 * \brief remove all events on output buffer
 * \param seq sequencer handle
 *
 * Removes all events on both user-space output buffer and
 * output memory pool on kernel.
 *
 * \sa snd_seq_drain_output(), snd_seq_drop_output_buffer(), snd_seq_remove_events()
 */
int
snd_seq_drop_output (snd_seq_t * seq)
{
  dbg_printf ("snd_seq_drop_output()\n");

  return 0;
}

/**
 * \brief output an event
 * \param seq sequencer handle
 * \param ev event to be output
 * \return the number of remaining events or a negative error code
 *
 * An event is once expanded on the output buffer.
 * The output buffer will be drained automatically if it becomes full.
 *
 * If events remain unprocessed on output buffer before drained,
 * the size of total byte data on output buffer is returned.
 * If the output buffer is empty, this returns zero.
 *
 * \sa snd_seq_event_output_direct(), snd_seq_event_output_buffer(),
 *    snd_seq_event_output_pending(), snd_seq_drain_output(),
 *    snd_seq_drop_output(), snd_seq_extract_output(),
 *    snd_seq_remove_events()
 */
int
snd_seq_event_output (snd_seq_t * seq, snd_seq_event_t * ev)
{
  dbg_printf3 ("snd_seq_event_output(seq=%x, ev=%x)\n", seq, ev);

  return convert_event (seq, ev);
}

/**
 * \brief delete the specified queue
 * \param seq sequencer handle
 * \param q queue id to delete
 * \return 0 on success otherwise a negative error code
 *
 * \sa snd_seq_alloc_queue()
 */
int
snd_seq_free_queue (snd_seq_t * seq, int q)
{
  dbg_printf ("snd_seq_free_queue()\n");

  return 0;
}

/**
 * \brief obtain the information of the given client
 * \param seq sequencer handle
 * \param client client id
 * \param info the pointer to be stored
 * \return 0 on success otherwise a negative error code
 *
 * Obtains the information of the client with a client id specified by
 * info argument.
 * The obtained information is written on info parameter.
 *
 * \sa snd_seq_get_client_info()
 */
int
snd_seq_get_any_client_info (snd_seq_t * seq, int client,
			     snd_seq_client_info_t * info)
{
  dbg_printf ("snd_seq_get_any_client_info()\n");

  strcpy (info->name, seq->name);

  return 0;
}

/**
 * \brief obtain the information of a port on an arbitrary client
 * \param seq sequencer handle
 * \param client client id to get
 * \param port port id to get
 * \param info pointer information returns
 * \return 0 on success otherwise a negative error code
 *
 * \sa snd_seq_get_port_info()
 */
int
snd_seq_get_any_port_info (snd_seq_t * seq, int client, int port,
			   snd_seq_port_info_t * info)
{
  dbg_printf ("snd_seq_get_any_port_info()\n");

  return 0;
}

/**
 * \brief allocate an empty #snd_seq_port_info_t using standard malloc
 * \param ptr returned pointer
 * \return 0 on success otherwise negative error code
 */
int
snd_seq_port_info_malloc (snd_seq_port_info_t ** ptr)
{
  snd_seq_port_info_t *p;

  dbg_printf ("snd_seq_port_info_malloc()\n");

  if ((p = malloc (sizeof (*p))) == NULL)
    return -ENOMEM;

  *ptr = p;

  return 0;
}

/**
 * \brief frees a previously allocated #snd_seq_port_info_t
 * \param pointer to object to free
 */
void
snd_seq_port_info_free (snd_seq_port_info_t * obj)
{
  free (obj);
}

/**
 * \brief allocate an empty #snd_seq_queue_tempo_t using standard malloc
 * \param ptr returned pointer
 * \return 0 on success otherwise negative error code
 */
int
snd_seq_queue_tempo_malloc (snd_seq_queue_tempo_t ** ptr)
{
  assert (ptr);
  *ptr = calloc (1, sizeof (snd_seq_queue_tempo_t));
  dbg_printf ("snd_seq_queue_tempo_malloc()=%x\n", *ptr);
  if (!*ptr)
    return -ENOMEM;
  return 0;
}

/**
 * \brief frees a previously allocated #snd_seq_queue_tempo_t
 * \param pointer to object to free
 */
void
snd_seq_queue_tempo_free (snd_seq_queue_tempo_t * obj)
{
  dbg_printf ("snd_seq_queue_tempo_free(%x)\n", obj);
  free (obj);
}

/**
 * \brief Set the ppq of a queue_status container
 * \param info queue_status container
 * \param ppq ppq value
 *
 * \sa snd_seq_get_queue_tempo()
 */
void
snd_seq_queue_tempo_set_ppq (snd_seq_queue_tempo_t * info, int ppq)
{
  dbg_printf ("snd_seq_queue_tempo_set_ppq(info=%x, %d)\n", info, ppq);
}

/**
 * \brief Set the tempo of a queue_status container
 * \param info queue_status container
 * \param tempo tempo value
 *
 * \sa snd_seq_get_queue_tempo()
 */
void
snd_seq_queue_tempo_set_tempo (snd_seq_queue_tempo_t * info,
			       unsigned int tempo)
{
  dbg_printf ("snd_seq_queue_tempo_set_tempo(info=%x, %d)\n", info, tempo);
}

/**
 * \brief get size of #snd_seq_queue_tempo_t
 * \return size in bytes
 */
size_t
snd_seq_queue_tempo_sizeof ()
{
  return sizeof (snd_seq_queue_tempo_t);
}

/**
 * \brief set the queue timer information
 * \param seq sequencer handle
 * \param q queue id to change the timer
 * \param timer timer information
 * \return 0 on success otherwise a negative error code
 *
 * \sa snd_seq_get_queue_timer()
 */
int
snd_seq_set_queue_timer (snd_seq_t * seq, int q,
			 snd_seq_queue_timer_t * timer)
{
  dbg_printf ("snd_seq_get_queue_timer(seq=%x, q=%d, timer=%X)\n", seq, q,
	      timer);
  return -ENXIO;		// TODO
}


/**
 * \brief Opens a new connection to the timer query interface.
 * \param timer Returned handle (NULL if not wanted)
 * \param name ASCII identifier of the RawMidi handle
 * \param mode Open mode
 * \return 0 on success otherwise a negative error code
 *
 * Opens a new connection to the RawMidi interface specified with
 * an ASCII identifier and mode.
 */
int
snd_timer_query_open (snd_timer_query_t ** timer, const char *name, int mode)
{
  dbg_printf ("snd_timer_query_open(name=%s, mode=%x)\n", name, mode);
  return -ENXIO;		// TODO
}

/**
 * \brief obtain the current tempo of the queue
 * \param seq sequencer handle
 * \param q queue id to be queried
 * \param tempo pointer to store the current tempo
 * \return 0 on success otherwise a negative error code
 *
 * \sa snd_seq_set_queue_tempo()
 */
int
snd_seq_get_queue_tempo (snd_seq_t * seq, int q,
			 snd_seq_queue_tempo_t * tempo)
{
  dbg_printf ("snd_seq_get_queue_tempo(seq=%x, q=%d, tempo=%x)\n", seq, q,
	      tempo);

  return 0;
}

/**
 * \brief allocate an empty #snd_seq_client_info_t using standard malloc
 * \param ptr returned pointer
 * \return 0 on success otherwise negative error code
 */
int
snd_seq_client_info_malloc (snd_seq_client_info_t ** ptr)
{
  dbg_printf ("snd_seq_client_info_malloc()\n");

  *ptr = malloc (sizeof (snd_seq_client_info_t));
  if (!*ptr)
    return -ENOMEM;
  return 0;
}

/**
 * \brief frees a previously allocated #snd_seq_client_info_t
 * \param pointer to object to free
 */
void
snd_seq_client_info_free (snd_seq_client_info_t * obj)
{
  dbg_printf ("snd_seq_client_info_free()\n");

  free (obj);
}

/**
 * \brief Get the name of a client_info container
 * \param info client_info container
 * \return name string
 *
 * \sa snd_seq_get_client_info(), snd_seq_client_info_set_name()
 */
const char *
snd_seq_client_info_get_name (snd_seq_client_info_t * info)
{
  dbg_printf ("snd_seq_client_info_get_name()\n");
  return "OSS seq client";
}

/**
 * \brief Get the number of opened ports of a client_info container
 * \param info client_info container
 * \return number of opened ports
 *
 * \sa snd_seq_get_client_info()
 */
int
snd_seq_client_info_get_num_ports (const snd_seq_client_info_t * info)
{
  dbg_printf ("snd_seq_client_info_get_num_ports()\n");

  return 1;
}

/**
 * \brief Get size of #snd_seq_system_info_t
 * \return size in bytes
 */
size_t
snd_seq_system_info_sizeof ()
{
  return sizeof (snd_seq_system_info_t);
}


/**
 * \brief Allocate an empty #snd_seq_system_info_t using standard malloc
 * \param ptr returned pointer
 * \return 0 on success otherwise negative error code
 */
int
snd_seq_system_info_malloc (snd_seq_system_info_t ** ptr)
{
  dbg_printf ("snd_seq_system_info_malloc()\n");

  *ptr = malloc (sizeof (snd_seq_system_info_t));
  if (*ptr == NULL)
    return -ENOMEM;
  return 0;
}

/**
 * \brief Frees a previously allocated #snd_seq_system_info_t
 * \param pointer to object to free
 */
void
snd_seq_system_info_free (snd_seq_system_info_t * obj)
{
  free (obj);
}

/**
 * \brief obtain the sequencer system information
 * \param seq sequencer handle
 * \param info the pointer to be stored
 * \return 0 on success otherwise a negative error code
 *
 * Stores the global system information of ALSA sequencer system.
 * The returned data contains
 * the maximum available numbers of queues, clients, ports and channels.
 */
int
snd_seq_system_info (snd_seq_t * seq, snd_seq_system_info_t * info)
{
  dbg_printf ("snd_seq_system_info()\n");

  return 0;
}

/**
 * \brief Get maximum number of clients
 * \param info #snd_seq_system_info_t container
 * \return maximum number of clients
 *
 * \sa snd_seq_system_info()
 */
int
snd_seq_system_info_get_clients (const snd_seq_system_info_t * info)
{
  dbg_printf ("snd_seq_system_info_get_clients()\n");

  return 4;
}

/**
 * \brief Get maximum number of queues
 * \param info #snd_seq_system_info_t container
 * \return maximum number of queues
 *
 * \sa snd_seq_system_info()
 */
int
snd_seq_system_info_get_queues (const snd_seq_system_info_t * info)
{
  dbg_printf ("snd_seq_system_info_get_queues(info=%x)\n", info);

  return 1;			// TODO
}

/**
 * \brief Get maximum number of ports
 * \param info #snd_seq_system_info_t container
 * \return maximum number of ports
 *
 * \sa snd_seq_system_info()
 */
int
snd_seq_system_info_get_ports (const snd_seq_system_info_t * info)
{
  dbg_printf ("snd_seq_system_info_get_ports()\n");

  return 4;
}

/**
 * \brief allocate a queue with the specified name
 * \param seq sequencer handle
 * \param name the name of the new queue
 * \return the queue id (zero or positive) on success otherwise a negative error code
 *
 * \sa snd_seq_alloc_queue()
 */
int
snd_seq_alloc_named_queue (snd_seq_t * seq, const char *name)
{
  dbg_printf ("snd_seq_alloc_named_queue(seq=%x, name=%s)\n", seq, name);

  return 0;
}


/**
 * \brief Get client id of a port_info container
 * \param info port_info container
 * \return client id
 *
 * \sa snd_seq_get_port_info(), snd_seq_port_info_set_client()
 */
int
snd_seq_port_info_get_client (const snd_seq_port_info_t * info)
{
  dbg_printf ("snd_seq_port_info_get_client()\n");

  return 0;
}

/**
 * \brief Get the port-specified mode of a port_info container
 * \param info port_info container
 * \return 1 if port id is specified at creation
 *
 * \sa snd_seq_get_port_info(), snd_seq_port_info_set_port_specified()
 */
int
snd_seq_port_info_get_port_specified (const snd_seq_port_info_t * info)
{
  dbg_printf ("snd_seq_port_info_get_port_specified()\n");

  return 0;
}

/**
 * \brief Get the type bits of a port_info container
 * \param info port_info container
 * \return port type bits
 *
 * \sa snd_seq_get_port_info(), snd_seq_port_info_get_type()
 */
void
snd_seq_port_info_set_type (snd_seq_port_info_t * info, unsigned int type)
{
  dbg_printf ("snd_seq_port_info_set_type(%u)\n", type);
}


/**
 * \brief Get the ppq of a queue_status container
 * \param info queue_status container
 * \return ppq value
 *
 * \sa snd_seq_get_queue_tempo()
 */
int
snd_seq_queue_tempo_get_ppq (const snd_seq_queue_tempo_t * info)
{
  dbg_printf ("snd_seq_queue_tempo_get_ppq()\n");

  return 0;
}


/**
 * \brief Get the tempo of a queue_status container
 * \param info queue_status container
 * \return tempo value
 *
 * \sa snd_seq_get_queue_tempo()
 */
unsigned int
snd_seq_queue_tempo_get_tempo (const snd_seq_queue_tempo_t * info)
{
  dbg_printf ("snd_seq_queue_tempo_get_tempo()\n");

  return 0;
}

/**
 * \brief set the tempo of the queue
 * \param seq sequencer handle
 * \param q queue id to change the tempo
 * \param tempo tempo information
 * \return 0 on success otherwise a negative error code
 *
 * \sa snd_seq_get_queue_tempo()
 */
int
snd_seq_set_queue_tempo (snd_seq_t * seq, int q,
			 snd_seq_queue_tempo_t * tempo)
{
  dbg_printf ("snd_seq_set_queue_tempo(seq=%x, q=%d, tempo=%x)\n", seq, q,
	      tempo);

  return 0;
}

/**
 * \brief allocate a queue
 * \param seq sequencer handle
 * \return the queue id (zero or positive) on success otherwise a negative error code
 *
 * \sa snd_seq_alloc_named_queue(), snd_seq_create_queue(), snd_seq_free_queue(),
 *     snd_seq_get_queue_info()
 */
int
snd_seq_alloc_queue (snd_seq_t * seq)
{
  static int queue_num = 0;
  dbg_printf ("snd_seq_alloc_queue(seq=%x)=%d\n", seq, queue_num);

  return queue_num++;
}

#define FIXED_EV(x)	(_SND_SEQ_TYPE(SND_SEQ_EVFLG_FIXED) | _SND_SEQ_TYPE(x))

/** Event types conversion array */
const unsigned int snd_seq_event_types[256] = {
  [SND_SEQ_EVENT_SYSTEM...SND_SEQ_EVENT_RESULT]
    = FIXED_EV (SND_SEQ_EVFLG_RESULT),
  [SND_SEQ_EVENT_NOTE]
    =
    FIXED_EV (SND_SEQ_EVFLG_NOTE) |
    _SND_SEQ_TYPE_OPT (SND_SEQ_EVFLG_NOTE_TWOARG),
  [SND_SEQ_EVENT_NOTEON...SND_SEQ_EVENT_KEYPRESS] =
    FIXED_EV (SND_SEQ_EVFLG_NOTE),
  [SND_SEQ_EVENT_CONTROLLER...SND_SEQ_EVENT_REGPARAM] =
    FIXED_EV (SND_SEQ_EVFLG_CONTROL),
  [SND_SEQ_EVENT_START...SND_SEQ_EVENT_STOP] = FIXED_EV (SND_SEQ_EVFLG_QUEUE),
  [SND_SEQ_EVENT_SETPOS_TICK]
    =
    FIXED_EV (SND_SEQ_EVFLG_QUEUE) |
    _SND_SEQ_TYPE_OPT (SND_SEQ_EVFLG_QUEUE_TICK),
  [SND_SEQ_EVENT_SETPOS_TIME] =
    FIXED_EV (SND_SEQ_EVFLG_QUEUE) |
    _SND_SEQ_TYPE_OPT (SND_SEQ_EVFLG_QUEUE_TIME),
  [SND_SEQ_EVENT_TEMPO...SND_SEQ_EVENT_SYNC_POS] =
    FIXED_EV (SND_SEQ_EVFLG_QUEUE) |
    _SND_SEQ_TYPE_OPT (SND_SEQ_EVFLG_QUEUE_VALUE),
  [SND_SEQ_EVENT_TUNE_REQUEST...SND_SEQ_EVENT_SENSING] =
    FIXED_EV (SND_SEQ_EVFLG_NONE),
  [SND_SEQ_EVENT_ECHO...SND_SEQ_EVENT_OSS] =
    FIXED_EV (SND_SEQ_EVFLG_RAW) | FIXED_EV (SND_SEQ_EVFLG_SYSTEM),
  [SND_SEQ_EVENT_CLIENT_START...SND_SEQ_EVENT_PORT_CHANGE] =
    FIXED_EV (SND_SEQ_EVFLG_MESSAGE),
  [SND_SEQ_EVENT_PORT_SUBSCRIBED...SND_SEQ_EVENT_PORT_UNSUBSCRIBED] =
    FIXED_EV (SND_SEQ_EVFLG_CONNECTION),
  [SND_SEQ_EVENT_USR0...SND_SEQ_EVENT_USR9] =
    FIXED_EV (SND_SEQ_EVFLG_RAW) | FIXED_EV (SND_SEQ_EVFLG_USERS),
  [SND_SEQ_EVENT_SYSEX...SND_SEQ_EVENT_BOUNCE] =
    _SND_SEQ_TYPE (SND_SEQ_EVFLG_VARIABLE),
  [SND_SEQ_EVENT_USR_VAR0...SND_SEQ_EVENT_USR_VAR4] =
    _SND_SEQ_TYPE (SND_SEQ_EVFLG_VARIABLE) |
    _SND_SEQ_TYPE (SND_SEQ_EVFLG_USERS),
  [SND_SEQ_EVENT_NONE] = FIXED_EV (SND_SEQ_EVFLG_NONE),
};

/**
 * \brief obtain the running state of the queue
 * \param seq sequencer handle
 * \param q queue id to query
 * \param status pointer to store the current status
 * \return 0 on success otherwise a negative error code
 *
 * Obtains the running state of the specified queue q.
 */
int
snd_seq_get_queue_status (snd_seq_t * seq, int q,
			  snd_seq_queue_status_t * status)
{
  dbg_printf ("snd_seq_get_queue_status(seq=%x. q=%d)\n", seq, q);

  return 0;
}

/**
 * \brief allocate an empty #snd_seq_queue_status_t using standard malloc
 * \param ptr returned pointer
 * \return 0 on success otherwise negative error code
 */
int
snd_seq_queue_status_malloc (snd_seq_queue_status_t ** ptr)
{
  dbg_printf ("snd_seq_queue_status_malloc()\n");
  *ptr = calloc (1, sizeof (snd_seq_queue_status_t));
  dbg_printf ("snd_seq_queue_status_malloc()=%x\n", *ptr);
  if (!*ptr)
    return -ENOMEM;
  return 0;
}

/**
 * \brief frees a previously allocated #snd_seq_queue_status_t
 * \param pointer to object to free
 */
void
snd_seq_queue_status_free (snd_seq_queue_status_t * obj)
{
  dbg_printf ("snd_seq_queue_status_free(%x)\n", obj);

  free (obj);
}

/**
 * \brief Get the tick time of a queue_status container
 * \param info queue_status container
 * \return tick time
 *
 * \sa snd_seq_get_queue_status()
 */
snd_seq_tick_time_t
snd_seq_queue_status_get_tick_time (const snd_seq_queue_status_t * info)
{
  dbg_printf ("snd_seq_queue_status_get_tick_time(info=%x)\n", info);

  return 0;
}

/**
 * \brief get size of #snd_seq_remove_events_t
 * \return size in bytes
 */
size_t
snd_seq_remove_events_sizeof ()
{
  return sizeof (snd_seq_remove_events_t);
}

/**
 * \brief allocate an empty #snd_seq_remove_events_t using standard malloc
 * \param ptr returned pointer
 * \return 0 on success otherwise negative error code
 */
int
snd_seq_remove_events_malloc (snd_seq_remove_events_t ** ptr)
{
  assert (ptr);
  *ptr = calloc (1, sizeof (snd_seq_remove_events_t));
  dbg_printf ("snd_seq_remove_events_malloc()=%x\n", *ptr);
  if (!*ptr)
    return -ENOMEM;
  return 0;
}

/**
 * \brief frees a previously allocated #snd_seq_remove_events_t
 * \param pointer to object to free
 */
void
snd_seq_remove_events_free (snd_seq_remove_events_t * obj)
{
  free (obj);
}

/**
 * \brief Set the removal condition bits
 * \param info remove_events container
 * \param flags removal condition bits
 *
 * \sa snd_seq_remove_events()
 */
void
snd_seq_remove_events_set_condition (snd_seq_remove_events_t * info,
				     unsigned int flags)
{
  dbg_printf ("snd_seq_remove_events_set_condition(rmp=%x, flags=%lu)\n",
	      info, flags);
}

/**
 * \brief Set the queue as removal condition
 * \param info remove_events container
 * \param queue queue id
 *
 * \sa snd_seq_remove_events()
 */
void
snd_seq_remove_events_set_queue (snd_seq_remove_events_t * info, int queue)
{
  dbg_printf ("snd_seq_remove_events_set_queue(rmp=%x, q=%d)\n", info, queue);
}

/**
 * \brief remove events on input/output buffers and pools
 * \param seq sequencer handle
 * \param rmp remove event container
 *
 * Removes matching events with the given condition from input/output buffers
 * and pools.
 * The removal condition is specified in \a rmp argument.
 *
 * \sa snd_seq_event_output(), snd_seq_drop_output(), snd_seq_reset_pool_output()
 */
int
snd_seq_remove_events (snd_seq_t * seq, snd_seq_remove_events_t * rmp)
{
  dbg_printf ("snd_seq_remove_events(seq=%x, rmp=%x)\n", seq, rmp);

  return 1;
}


/**
 * \brief clear input buffer and and remove events in sequencer queue
 * \param seq sequencer handle
 *
 * \sa snd_seq_drop_input_buffer(), snd_seq_remove_events()
 */
int
snd_seq_drop_input (snd_seq_t * seq)
{
  dbg_printf ("snd_seq_drop_input(seq=%x)\n", seq);

  return 0;
}

/**
 * \brief remove all events on user-space output buffer
 * \param seq sequencer handle
 *
 * Removes all events on user-space output buffer.
 * Unlike snd_seq_drain_output(), this function doesn't remove
 * events on output memory pool of sequencer.
 *
 * \sa snd_seq_drop_output()
 */
int
snd_seq_drop_output_buffer (snd_seq_t * seq)
{
  dbg_printf ("snd_seq_drop_output_buffer(seq=%x)\n", seq);

  return 0;
}

/**
 * \brief extract the first event in output buffer
 * \param seq sequencer handle
 * \param ev_res event pointer to be extracted
 * \return 0 on success otherwise a negative error code
 *
 * Extracts the first event in output buffer.
 * If ev_res is NULL, just remove the event.
 *
 * \sa snd_seq_event_output()
 */
int
snd_seq_extract_output (snd_seq_t * seq, snd_seq_event_t ** ev_res)
{
  dbg_printf ("snd_seq_extract_output(seq=%x)\n", seq);

  return -EIO;
}

/**
 * \brief get size of #snd_seq_queue_status_t
 * \return size in bytes
 */
size_t
snd_seq_queue_status_sizeof ()
{
  return sizeof (snd_seq_queue_status_t);
}

/**
 * \brief Return the size of output buffer
 * \param seq sequencer handle
 * \return the size of output buffer in bytes
 *
 * Obtains the size of output buffer.
 * This buffer is used to store decoded byte-stream of output events
 * before transferring to sequencer.
 *
 * \sa snd_seq_set_output_buffer_size()
 */
size_t
snd_seq_get_output_buffer_size (snd_seq_t * seq)
{
  dbg_printf ("snd_seq_get_output_buffer_size(seq=%x)\n", seq);

  return 1024;
}

/**
 * \brief Change the size of output buffer
 * \param seq sequencer handle
 * \param size the size of output buffer to be changed in bytes
 * \return 0 on success otherwise a negative error code
 *
 * Changes the size of output buffer.
 *
 * \sa snd_seq_get_output_buffer_size()
 */
int
snd_seq_set_output_buffer_size (snd_seq_t * seq, size_t size)
{
  dbg_printf ("snd_seq_set_output_buffer_size(seq=%x, size=%d)\n", seq, size);
  return 0;
}

/**
 * \brief unsubscribe a connection between ports
 * \param seq sequencer handle
 * \param sub subscription information to disconnect
 * \return 0 on success otherwise a negative error code
 *
 * Unsubscribes a connection between two ports,
 * described in sender and dest fields in sub argument.
 *
 * \sa snd_seq_subscribe_port(), snd_seq_disconnect_from(), snd_seq_disconnect_to()
 */
int
snd_seq_unsubscribe_port (snd_seq_t * seq, snd_seq_port_subscribe_t * sub)
{
  dbg_printf ("snd_seq_unsubscribe_port(seq=%x, sub=%x)\n", seq, sub);

  return 0;
}

/**
 * \brief Get the queue id of a queue_timer container
 * \param info queue_timer container
 * \return queue id
 *
 * \sa snd_seq_get_queue_timer()
 */
int
snd_seq_queue_timer_get_queue (const snd_seq_queue_timer_t * info)
{
  dbg_printf ("snd_seq_queue_timer_get_queue(timer=%x)\n", info);

  return 0;
}

/**
 * \brief obtain the queue timer information
 * \param seq sequencer handle
 * \param q queue id to query
 * \param timer pointer to store the timer information
 * \return 0 on success otherwise a negative error code
 *
 * \sa snd_seq_set_queue_timer()
 */
int
snd_seq_get_queue_timer (snd_seq_t * seq, int q,
			 snd_seq_queue_timer_t * timer)
{
  dbg_printf ("snd_seq_get_queue_timer(seq=%x, q=%d, timer=%x)\n",
	      seq, 1, timer);

  return 0;
}

/**
 * \brief Set the timer id of a queue_timer container
 * \param info queue_timer container
 * \param id timer id pointer
 *
 * \sa snd_seq_get_queue_timer()
 */
void
snd_seq_queue_timer_set_id (snd_seq_queue_timer_t * info,
			    const snd_timer_id_t * id)
{
  dbg_printf ("snd_seq_queue_timer_set_id(timer=%x, id=%x)\n", info, id);
}

/**
 * \brief get size of #snd_seq_queue_timer_t
 * \return size in bytes
 */
size_t
snd_seq_queue_timer_sizeof ()
{
  return sizeof (snd_seq_queue_timer_t);
}

/**
 * \brief get size of #snd_seq_query_subscribe_t
 * \return size in bytes
 */
size_t
snd_seq_query_subscribe_sizeof ()
{
  return sizeof (snd_seq_query_subscribe_t);
}

/**
 * \brief allocate an empty #snd_seq_query_subscribe_t using standard malloc
 * \param ptr returned pointer
 * \return 0 on success otherwise negative error code
 */
int
snd_seq_query_subscribe_malloc (snd_seq_query_subscribe_t ** ptr)
{
  *ptr = calloc (1, sizeof (snd_seq_query_subscribe_t));

  dbg_printf ("snd_seq_query_subscribe_malloc()=%x\n", *ptr);
  if (!*ptr)
    return -ENOMEM;
  return 0;
}

/**
 * \brief frees a previously allocated #snd_seq_query_subscribe_t
 * \param pointer to object to free
 */
void
snd_seq_query_subscribe_free (snd_seq_query_subscribe_t * obj)
{
  dbg_printf ("snd_seq_query_subscribe_free(obj=%x)\n", obj);
  free (obj);
}

/**
 * \brief Get the address of subscriber of a query_subscribe container
 * \param info query_subscribe container
 * \return subscriber's address pointer
 *
 * \sa snd_seq_query_port_subscribers()
 */
const snd_seq_addr_t *
snd_seq_query_subscribe_get_addr (const snd_seq_query_subscribe_t * info)
{
  dbg_printf ("snd_seq_query_subscribe_get_addr(info=%x)\n", info);

  return NULL;			// TODO
}

/**
 * \brief Get the index of subscriber of a query_subscribe container
 * \param info query_subscribe container
 * \return subscriber's index
 *
 * \sa snd_seq_query_port_subscribers(), snd_seq_query_subscribe_set_index()
 */
int
snd_seq_query_subscribe_get_index (const snd_seq_query_subscribe_t * info)
{
  dbg_printf ("snd_seq_query_subscribe_get_index(info=%x)\n", info);

  return 0;			// TODO
}

/**
 * \brief Set the subscriber's index to be queried
 * \param info query_subscribe container
 * \param index index to be queried
 *
 * \sa snd_seq_query_port_subscribers(), snd_seq_query_subscribe_get_index()
 */
void
snd_seq_query_subscribe_set_index (snd_seq_query_subscribe_t * info,
				   int index)
{
  dbg_printf ("snd_seq_query_subscribe_t(info=%x, index=%d)\n", info, index);

  // TODO
}

/**
 * \brief Get the client/port address of a query_subscribe container
 * \param info query_subscribe container
 * \return client/port address pointer
 *
 * \sa snd_seq_query_port_subscribers(), snd_seq_query_subscribe_set_root()
 */
const snd_seq_addr_t *
snd_seq_query_subscribe_get_root (const snd_seq_query_subscribe_t * info)
{
  dbg_printf ("snd_seq_query_subscribe_get_root(info=%x)\n", info);

  return NULL;			// TODO
}

/**
 * \brief Set the client/port address of a query_subscribe container
 * \param info query_subscribe container
 * \param addr client/port address pointer
 *
 * \sa snd_seq_query_port_subscribers(), snd_seq_query_subscribe_get_root()
 */
void
snd_seq_query_subscribe_set_root (snd_seq_query_subscribe_t * info,
				  const snd_seq_addr_t * addr)
{
  dbg_printf ("snd_seq_query_subscribe_set_root(info=%d, addr=%x)\n",
	      info, addr);
}

/**
 * \brief Get the query type of a query_subscribe container
 * \param info query_subscribe container
 * \return query type
 *
 * \sa snd_seq_query_port_subscribers(), snd_seq_query_subscribe_set_type()
 */
snd_seq_query_subs_type_t
snd_seq_query_subscribe_get_type (const snd_seq_query_subscribe_t * info)
{
  dbg_printf ("snd_seq_query_subscribe_get_type(info=%x)\n", info);

  return 0;			// TODO
}

/**
 * \brief Set the query type of a query_subscribe container
 * \param info query_subscribe container
 * \param type query type
 *
 * \sa snd_seq_query_port_subscribers(), snd_seq_query_subscribe_get_type()
 */
void
snd_seq_query_subscribe_set_type (snd_seq_query_subscribe_t * info,
				  snd_seq_query_subs_type_t type)
{
  dbg_printf ("snd_seq_query_subscribe_set_type(info=%x, type=%x)\n",
	      info, type);

  // TODO
}

/**
 * \brief obtain subscription information
 * \param seq sequencer handle
 * \param sub pointer to return the subscription information
 * \return 0 on success otherwise a negative error code
 *
 * \sa snd_seq_subscribe_port(), snd_seq_query_port_subscribers()
 */
int
snd_seq_get_port_subscription (snd_seq_t * seq,
			       snd_seq_port_subscribe_t * sub)
{
  dbg_printf ("snd_seq_get_port_subscription(seq=%x, sub=%x)\n", seq, sub);

  return 0;			// TODO
}

/**
 * \brief query port subscriber list
 * \param seq sequencer handle
 * \param subs subscription to query
 * \return 0 on success otherwise a negative error code
 *
 * Queries the subscribers accessing to a port.
 * The query information is specified in subs argument.
 *
 * At least, the client id, the port id, the index number and
 * the query type must be set to perform a proper query.
 * As the query type, #SND_SEQ_QUERY_SUBS_READ or #SND_SEQ_QUERY_SUBS_WRITE
 * can be specified to check whether the readers or the writers to the port.
 * To query the first subscription, set 0 to the index number.  To list up
 * all the subscriptions, call this function with the index numbers from 0
 * until this returns a negative value.
 *
 * \sa snd_seq_get_port_subscription()
 */
int
snd_seq_query_port_subscribers (snd_seq_t * seq, snd_seq_query_subscribe_t *
				subs)
{
  dbg_printf ("snd_seq_query_port_subscribers(seq=%x, subs=%x)\n", seq, subs);

  return 0;			// TODO
}

/**
 * \brief Get the real time of a queue_status container
 * \param info queue_status container
 * \param time real time
 *
 * \sa snd_seq_get_queue_status()
 */
const snd_seq_real_time_t *
snd_seq_queue_status_get_real_time (const snd_seq_queue_status_t * info)
{
  dbg_printf ("snd_seq_queue_status_get_real_time(info=%x)\n", info);

  return NULL;			// TODO
}
