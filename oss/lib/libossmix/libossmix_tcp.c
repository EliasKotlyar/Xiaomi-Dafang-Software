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
 * TCP driver for libossmix
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

static int initialized = 0;
static void tcp_disconnect (void);
static int sockfd = -1;
static int do_byteswap = 0;

static void poll_callback (void);
static void handle_packet (ossmix_commad_packet_t * msg, char *payload,
			   int payload_size);

static int
read_all (int sock, void *b, int count)
{
  unsigned char *buf = b;
  int l = 0;

  while (l < count)
    {
      int c, n = count - l;

      if ((c = read (sock, buf + l, n)) <= 0)
	return c;

      l += c;
    }

  return l;
}

static inline int
bswap32 (int x)
{

  int y = 0;
  unsigned char *a = ((unsigned char *) &x) + 3;
  unsigned char *b = (unsigned char *) &y;

  *b++ = *a--;
  *b++ = *a--;
  *b++ = *a--;
  *b++ = *a--;

  return y;
}

#define BSWAP32(x) x=bswap32(x)

static void
byteswap_msg (ossmix_commad_packet_t * msg)
{
  BSWAP32 (msg->cmd);
  BSWAP32 (msg->p1);
  BSWAP32 (msg->p2);
  BSWAP32 (msg->p3);
  BSWAP32 (msg->p4);
  BSWAP32 (msg->p5);
  BSWAP32 (msg->ack_rq);
  BSWAP32 (msg->unsolicited);
  BSWAP32 (msg->payload_size);
}

typedef void (*bswap_func_t) (void *data, int len);

static void
bswap_int_array (void *data, int len)
{
  int *arr = (int *) data;
  int i;

  if (len % sizeof (int) != 0)
    {
      fprintf (stderr, "bswap_int_array: Bad size %d\n", len);
      exit (EXIT_FAILURE);
    }

  for (i = 0; i < len / sizeof (int); i++)
    BSWAP32 (arr[i]);
}

static void
bswap_mixerinfo (void *data, int len)
{
  oss_mixerinfo *mi = (oss_mixerinfo *) data;

  if (len != sizeof (*mi))
    {
      fprintf (stderr, "bswap_mixerinfo: Bad size (%d/%lu)\n", len,
	       (unsigned long)sizeof (*mi));
      exit (EXIT_FAILURE);
    }

  BSWAP32 (mi->dev);
  BSWAP32 (mi->modify_counter);
  BSWAP32 (mi->card_number);
  BSWAP32 (mi->port_number);
  BSWAP32 (mi->magic);
  BSWAP32 (mi->enabled);
  BSWAP32 (mi->caps);
  BSWAP32 (mi->flags);
  BSWAP32 (mi->nrext);
  BSWAP32 (mi->priority);
  BSWAP32 (mi->legacy_device);
}

static void
bswap_nodeinfo (void *data, int len)
{
  oss_mixext *ei = (oss_mixext *) data;

  if (len != sizeof (*ei))
    {
      fprintf (stderr, "bswap_nodeinfo: Bad size (%d/%lu)\n", len,
	       (unsigned long)sizeof (*ei));
      exit (EXIT_FAILURE);
    }

  BSWAP32 (ei->dev);
  BSWAP32 (ei->ctrl);
  BSWAP32 (ei->type);
  BSWAP32 (ei->maxvalue);
  BSWAP32 (ei->minvalue);
  BSWAP32 (ei->flags);
  BSWAP32 (ei->parent);
  BSWAP32 (ei->dummy);
  BSWAP32 (ei->timestamp);
  BSWAP32 (ei->control_no);
  BSWAP32 (ei->desc);
  BSWAP32 (ei->update_counter);
  BSWAP32 (ei->rgbcolor);
}

static void
bswap_nodeinfo_array (void *data, int len)
{
  oss_mixext *ei = (oss_mixext *) data;
  int i, n;

  n = len / sizeof (oss_mixext);

  if (len != n * sizeof (oss_mixext))
    {
      fprintf (stderr, "bswap_enuminfo_array: Bad size (%d/%d*%lu)\n", len, n,
	       (unsigned long)sizeof (*ei));
      exit (EXIT_FAILURE);
    }

  for (i = 0; i < n; i++)
    bswap_nodeinfo ((void *) &ei[i], sizeof (oss_mixext));

}


static void
bswap_enuminfo (void *data, int len)
{
  oss_mixer_enuminfo *ei = (oss_mixer_enuminfo *) data;

  int i;

  if (len != sizeof (*ei))
    {
      fprintf (stderr, "bswap_enuminfo: Bad size (%d/%lu)\n", len,
	       (unsigned long)sizeof (*ei));
      exit (EXIT_FAILURE);
    }

  BSWAP32 (ei->dev);
  BSWAP32 (ei->ctrl);
  BSWAP32 (ei->nvalues);
  BSWAP32 (ei->version);

  for (i = 0; i < OSS_ENUM_MAXVALUE; i++)
    BSWAP32 (ei->strindex[i]);
}

static int
get_response (void)
{
  ossmix_commad_packet_t msg;
  char payload[4096];
  int l;

  if (sockfd == -1)
    return -1;

  while (1)
    {
      payload[0] = 0;

      if ((l = read_all (sockfd, &msg, sizeof (msg))) != sizeof (msg))
	{
	  if (l == 0)		/* Connection closed */
	    return -1;

	  perror ("get response");
	  return -1;
	}

      if (do_byteswap)
	byteswap_msg (&msg);

      if (msg.payload_size > 0)
	{
	  if ((l =
	       read_all (sockfd, payload,
			 msg.payload_size)) != msg.payload_size)
	    {
	      perror ("Get response payload");
	      fprintf (stderr, "Payload size %d/%d\n", l, msg.payload_size);
	      return -1;
	    }

	  payload[l] = 0;
	}

      if (msg.cmd == OSSMIX_CMD_ERROR)
	{
	  fprintf (stderr, "Remote error: %s\n", payload);
	}

      /*
       * Return if this was not an async notification message sent by the
       * server.
       */
      if (!msg.unsolicited)
	return msg.cmd;

      handle_packet (&msg, payload, msg.payload_size);
    }
}

static int
check_welcome (void)
{
  ossmix_commad_packet_t msg;
  int l;

  if (sockfd == -1)
    return 0;

  if ((l = read_all (sockfd, &msg, sizeof (msg))) != sizeof (msg))
    {
      if (l == 0)		/* Connection closed */
	return 0;

      perror ("get response");
      return 0;
    }

  if (msg.cmd != OSSMIX_CMD_HALOO)
    {
      fprintf (stderr, "Bad welcome from the remote server\n");
      return 0;
    }

  do_byteswap = 0;

  if (msg.p1 != OSSMIX_P1_MAGIC)
    {
      byteswap_msg (&msg);

      if (msg.p1 != OSSMIX_P1_MAGIC)
	{
	  fprintf (stderr, "Unrecognized endianess\n");
	  return 0;
	}

      fprintf (stderr, "Using alien endianess\n");
      do_byteswap = 1;
    }

  return 1;
}

static int
wait_payload (void *payload, int len, bswap_func_t swapper, int *truelen)
{
  ossmix_commad_packet_t msg;
  int l;

  if (sockfd == -1)
    return -1;

  while (1)
    {
      if ((l = read_all (sockfd, &msg, sizeof (msg))) != sizeof (msg))
	{
	  if (l == 0)		/* Connection closed */
	    return -1;

	  perror ("get response");
	  return -1;
	}

      if (do_byteswap)
	byteswap_msg (&msg);

      if (msg.payload_size > 0)
	{
	  if ((l =
	       read_all (sockfd, payload,
			 msg.payload_size)) != msg.payload_size)
	    {
	      perror ("Get error message");
	      fprintf (stderr, "Payload size %d/%d\n", l, msg.payload_size);
	      return -1;
	    }
	}

      if (msg.cmd == OSSMIX_CMD_ERROR)
	{
	  fprintf (stderr, "Remote error: %s\n", (char *)payload);
	}

      if (!msg.unsolicited)
	if (truelen == NULL)
	  if (msg.payload_size != len)
	    {
	      fprintf (stderr, "Payload size mismatch (%d/%d)\n",
		       msg.payload_size, len);
	      return -1;
	    }

      if (truelen != NULL)
	*truelen = msg.payload_size;

      /*
       * Return if this was not an async notification message sent by the
       * server.
       */
      if (!msg.unsolicited)
	return msg.cmd;

      handle_packet (&msg, payload, msg.payload_size);
    }
}

int
send_request (int cmd, int p1, int p2, int p3, int p4, int p5)
{
  ossmix_commad_packet_t msg;

  memset (&msg, 0, sizeof (msg));

  msg.cmd = cmd;
  msg.p1 = p1;
  msg.p2 = p2;
  msg.p3 = p3;
  msg.p4 = p4;
  msg.p5 = p5;
  msg.ack_rq = 1;

  if (do_byteswap)
    byteswap_msg (&msg);

  if (write (sockfd, &msg, sizeof (msg)) != sizeof (msg))
    {
      fprintf (stderr, "Write to socket failed\n");
    }
  return get_response ();
}

void
send_request_noreply (int cmd, int p1, int p2, int p3, int p4, int p5)
{
  ossmix_commad_packet_t msg;

  memset (&msg, 0, sizeof (msg));

  msg.cmd = cmd;
  msg.p1 = p1;
  msg.p2 = p2;
  msg.p3 = p3;
  msg.p4 = p4;
  msg.p5 = p5;
  msg.ack_rq = 0;

  if (do_byteswap)
    byteswap_msg (&msg);

  if (write (sockfd, &msg, sizeof (msg)) != sizeof (msg))
    {
      fprintf (stderr, "Write to socket failed\n");
    }
  //send(sockfd, &msg, sizeof(msg), 0);
}

int
send_request_long (int cmd, int p1, int p2, int p3, int p4, int p5,
		   const char *payload)
{
  ossmix_commad_packet_t msg;

  memset (&msg, 0, sizeof (msg));

  msg.cmd = cmd;
  msg.p1 = p1;
  msg.p2 = p2;
  msg.p3 = p3;
  msg.p4 = p4;
  msg.p5 = p5;
  msg.ack_rq = 1;
  msg.payload_size = strlen (payload);

  if (do_byteswap)
    byteswap_msg (&msg);

  if (write (sockfd, &msg, sizeof (msg)) != sizeof (msg))
    {
      fprintf (stderr, "Write to socket failed\n");
    }
  if (write (sockfd, payload, msg.payload_size) != msg.payload_size)
    {
      fprintf (stderr, "Write to socket failed\n");
    }
  return get_response ();
}

static int
tcp_connect (const char *remotehost, int port)
{
  struct sockaddr_in sa;
  struct hostent *he;

  if (mixlib_trace > 0)
    fprintf (stderr, "Entered tcp_connect(%s, %d)\n", remotehost, port);

  if (port == 0)
    port = 7777;

  if (initialized)
    {
      fprintf (stderr, "Panic: ossmixlib already initialized\n");
      exit (EXIT_FAILURE);
    }

  initialized = 1;

  /*
   * Open the network connection
   */

  if ((sockfd = socket (PF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("socket");
      return -1;
    }

  if ((he = gethostbyname (remotehost)) == NULL)
    {
      herror (remotehost);
      fprintf (stderr, "Cannot find the OSSMIX server \"%s\"\n", remotehost);
      return -1;
    }

  sa.sin_family = AF_INET;
  sa.sin_port = htons (port);

  memcpy ((void *) &sa.sin_addr, *he->h_addr_list, he->h_length);
  if (connect (sockfd, (void *) &sa, sizeof (sa)) == -1)
    {
      switch (errno)
	{
	case ECONNREFUSED:
	  fprintf (stderr,
		   "Remote OSSMIX server is not running (Connection refused)\n");
	  break;

	default:
	  perror ("connect");
	}
      fprintf (stderr, "Cannot connect OSSMIX server %s:%d\n", remotehost,
	       port);
      return -1;
    }
#if 0
// For some reason this doesn't work under Linux
  atexit (tcp_disconnect);
#endif

  if (!check_welcome ())
    return -1;
  return send_request (OSSMIX_CMD_INIT, 0, 0, 0, 0, 0);
}

static int
tcp_get_fd (ossmix_select_poll_t * cb)
{
  *cb = poll_callback;

  return sockfd;
}

static void
tcp_enable_events (void)
{
  send_request_noreply (OSSMIX_CMD_START_EVENTS, 0, 0, 0, 0, 0);
}

static void
tcp_disconnect (void)
{
  if (mixlib_trace > 0)
    fprintf (stderr, "Entered tcp_disconnect()\n");

  if (sockfd < 0)
    return;

  send_request (OSSMIX_CMD_EXIT, 0, 0, 0, 0, 0);
  close (sockfd);
  sockfd = -1;
}

static int
tcp_get_nmixers (void)
{
  int nmixers;

  nmixers = send_request (OSSMIX_CMD_GET_NMIXERS, 0, 0, 0, 0, 0);

  // TODO: num_mixers = nmixers; 

  return nmixers;
}

static int
tcp_get_mixerinfo (int mixernum, oss_mixerinfo * mi)
{
  send_request_noreply (OSSMIX_CMD_GET_MIXERINFO, mixernum, 0, 0, 0, 0);
  return wait_payload (mi, sizeof (*mi), bswap_mixerinfo, NULL);
}

static int
tcp_open_mixer (int mixernum)
{
  int nrext, nrext2;
  oss_mixext nodes[MAX_NODES];
  value_packet_t value_packet;

  if (send_request (OSSMIX_CMD_OPEN_MIXER, mixernum, 0, 0, 0, 0) < 0)
    return -1;

  if (mixernum >= MAX_TMP_MIXER)
    {
      fprintf (stderr, "tcp_open_mixer: Mixer number too large %d\n",
	       mixernum);
      return -1;
    }

  if ((nrext = send_request (OSSMIX_CMD_GET_NREXT, mixernum, 0, 0, 0, 0)) < 0)
    return -1;

  nrext2 = nrext;		// Save the value for the next step
/*
 * Load all node info records
 */

  send_request_noreply (OSSMIX_CMD_GET_NODEINFO, mixernum, 0, nrext - 1, 0,
			0);
  while (nrext > 0)
    {
      int i;
      int n;

      n = nrext;
      if (n > MAX_NODES)
	n = MAX_NODES;

      if (wait_payload
	  (nodes, n * sizeof (oss_mixext), bswap_nodeinfo_array, NULL) < 0)
	return -1;

      for (i = 0; i < n; i++)
	{
	  oss_mixext *node;

	  node = &nodes[i];

	  mixc_add_node (mixernum, node->ctrl, node);
	}

      nrext -= n;
    }

  nrext = nrext2;

/*
 * Load all values
 */
  send_request_noreply (OSSMIX_CMD_GET_ALL_VALUES, mixernum, 0, 0, 0, 0);

  if (wait_payload
      (value_packet, nrext * sizeof (value_record_t), bswap_int_array,
       &nrext2) < 0)
    return -1;
  else
    {
      int i;

      for (i = 0; i < nrext2 / sizeof (value_record_t); i++)
	{
	  mixc_set_value (mixernum, value_packet[i].node,
			  value_packet[i].value);
	}
    }

  return 0;
}

static void
tcp_close_mixer (int mixernum)
{
  send_request_noreply (OSSMIX_CMD_CLOSE_MIXER, mixernum, 0, 0, 0, 0);
}

static int
tcp_get_nrext (int mixernum)
{
  // TODO: Cache this information locally
  return send_request (OSSMIX_CMD_GET_NREXT, mixernum, 0, 0, 0, 0);
}

static int
tcp_get_nodeinfo (int mixernum, int node, oss_mixext * ext)
{
  oss_mixext *lnode;

  lnode = mixc_get_node (mixernum, node);

  if (lnode == NULL)
    {
      send_request_noreply (OSSMIX_CMD_GET_NODEINFO, mixernum, node, 0, 0, 0);
      if (wait_payload (ext, sizeof (*ext), bswap_nodeinfo, NULL) < 0)
	{
	  fprintf (stderr,
		   "tcp_get_nodeinfo: Mixer %d: Cannot load nodeinfo for %d\n",
		   mixernum, node);
	  return -1;
	}
      mixc_add_node (mixernum, node, lnode);
    }

  memcpy (ext, lnode, sizeof (*ext));

  return 0;
}

static int
tcp_get_enuminfo (int mixernum, int node, oss_mixer_enuminfo * ei)
{
  send_request_noreply (OSSMIX_CMD_GET_ENUMINFO, mixernum, node, 0, 0, 0);
  return wait_payload (ei, sizeof (*ei), bswap_enuminfo, NULL);
}

static int
tcp_get_description (int mixernum, int node, oss_mixer_enuminfo * desc)
{
  send_request_noreply (OSSMIX_CMD_GET_DESCRIPTION, mixernum, node, 0, 0, 0);
  return wait_payload (desc, sizeof (*desc), bswap_enuminfo, NULL);
}

static int
tcp_get_value (int mixernum, int ctl, int timestamp)
{
  // return send_request(OSSMIX_CMD_GET_VALUE, mixernum, ctl, timestamp, 0, 0);
  return mixc_get_value (mixernum, ctl);
}

static void
tcp_set_value (int mixernum, int ctl, int timestamp, int value)
{
  send_request_noreply (OSSMIX_CMD_SET_VALUE, mixernum, ctl, timestamp, value,
			0);
}

static void
tcp_timertick(void)
{
	// NOP
}

ossmix_driver_t ossmix_tcp_driver = {
  tcp_connect,
  tcp_get_fd,
  tcp_disconnect,
  tcp_enable_events,
  tcp_get_nmixers,
  tcp_get_mixerinfo,
  tcp_open_mixer,
  tcp_close_mixer,
  tcp_get_nrext,
  tcp_get_nodeinfo,
  tcp_get_enuminfo,
  tcp_get_description,
  tcp_get_value,
  tcp_set_value,
  tcp_timertick
};

static void
handle_values(int mixnum, int nvalues, value_record_t values[], int len)
{
	int i;

	if (nvalues*sizeof(value_record_t) > len)
	{
		fprintf(stderr, "Short value record (%lu, %d)\n",
                        (unsigned long)(nvalues*sizeof(value_record_t)), len);
		exit(EXIT_FAILURE);
	}

	for (i=0;i<nvalues;i++)
	{
	    _client_event (OSSMIX_EVENT_VALUE, mixnum, values[i].node, values[i].value, 0, 0);
	}

}

static void
handle_packet (ossmix_commad_packet_t * msg, char *payload, int payload_size)
{
//printf("Got packet %d, p=0x%08x, %d, %d, %d, %d\n",
//              msg->cmd, msg->p1, msg->p2, msg->p3, msg->p4, msg->p5);

//  _client_event (msg->cmd, msg->p1, msg->p2, msg->p3, msg->p4, msg->p5);

	switch(msg->cmd)
	{
	case OSSMIX_EVENT_VALUE:
		handle_values(msg->p2, msg->p1, (value_record_t *)payload, payload_size);
		break;

	case OSSMIX_EVENT_NEWMIXER:
		//num_mixers=msg->p1;
	 	_client_event (OSSMIX_EVENT_NEWMIXER, msg->p1, 0, 0, 0, 0);
		break;

	default:
		fprintf(stderr, "Unrecognized event packet %d\n", msg->cmd);
		exit(EXIT_FAILURE);
	}
}

static void
poll_callback (void)
{
  ossmix_commad_packet_t msg;
  char payload[4096];
  int l;

  if (sockfd == -1)
    return;

  payload[0] = 0;

  if ((l = read_all (sockfd, &msg, sizeof (msg))) != sizeof (msg))
    {
      if (l == 0)		/* Connection closed */
	return;

      perror ("get response");
      return;
    }

  if (do_byteswap)
    byteswap_msg (&msg);

  if (msg.payload_size > 0)
    {
      if ((l =
	   read_all (sockfd, payload, msg.payload_size)) != msg.payload_size)
	{
	  perror ("Get response payload");
	  fprintf (stderr, "Payload size %d/%d\n", l, msg.payload_size);
	  return;
	}

      payload[l] = 0;
    }

  handle_packet (&msg, payload, msg.payload_size);
}
