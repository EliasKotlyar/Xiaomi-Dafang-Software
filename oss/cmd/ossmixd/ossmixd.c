/*
 * OSS mixer service daemon (used by libossmix)
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#ifdef __SCO_VERSION__
#include <sys/time.h>
#else
#include <time.h>
#endif
#include <sched.h>
#include <soundcard.h>

#define OSSMIX_REMOTE
#include "libossmix.h"

static int connfd;
static int listenfd;
static int verbose = 0;
static int polling_started = 0;
static int num_mixers=0;

static unsigned char mixer_open_mask[MAX_TMP_MIXER/8] = {0};

static void
send_response (int cmd, int p1, int p2, int p3, int p4, int p5, int unsol)
{
  ossmix_commad_packet_t msg;

  memset (&msg, 0, sizeof (msg));

  if (verbose)
    printf ("Send %02d, p=0x%08x, %d, %d, %d, %d, unsol=%d\n",
	    cmd, p1, p2, p3, p4, p5, unsol);

  msg.cmd = cmd;
  msg.unsolicited = unsol;
  msg.p1 = p1;
  msg.p2 = p2;
  msg.p3 = p3;
  msg.p4 = p4;
  msg.p5 = p5;

  if (write (connfd, &msg, sizeof (msg)) != sizeof (msg))
    {
      fprintf (stderr, "Write to socket failed\n");
    }
}

static void
send_response_long (int cmd, int p1, int p2, int p3, int p4, int p5,
		    const char *payload, int plsize, int unsol)
{
  ossmix_commad_packet_t msg;

  if (verbose)
    printf ("Send %02d, p=0x%08x, %d, %d, %d, %d, unsol=%d, pl=%d\n",
	    cmd, p1, p2, p3, p4, p5, unsol, plsize);

  memset (&msg, 0, sizeof (msg));

  msg.cmd = cmd;
  msg.unsolicited = unsol;
  msg.p1 = p1;
  msg.p2 = p2;
  msg.p3 = p3;
  msg.p4 = p4;
  msg.p5 = p5;
  msg.payload_size = plsize;

  if (write (connfd, &msg, sizeof (msg)) != sizeof (msg))
    {
      fprintf (stderr, "Write to socket failed\n");
    }

  if (write (connfd, payload, msg.payload_size) != msg.payload_size)
    {
      fprintf (stderr, "Write to socket failed\n");
    }
}

static void
send_error (const char *msg)
{
  int l = strlen (msg) + 1;

  send_response_long (OSSMIX_CMD_ERROR, 0, 0, 0, 0, 0, msg, l, 0);
}

int
wait_connect (void)
{
  if (listen (listenfd, 1) == -1)
    {
      perror ("listen");
      exit (-1);
    }

  if ((connfd = accept (listenfd, NULL, NULL)) == -1)
    {
      perror ("accept");
      exit (-1);
    }

  return 1;
}

static void
send_ack (void)
{
  send_response (OSSMIX_CMD_OK, 0, 0, 0, 0, 0, 0);
}

static void
return_value (int val)
{
  send_response (val, 0, 0, 0, 0, 0, 0);
}

static void
send_multiple_nodes (ossmix_commad_packet_t * pack)
{
  int i, n;
  oss_mixext nodes[MAX_NODES];

  n = 0;
  for (i = pack->p2; i <= pack->p3; i++)
    {
      if (ossmix_get_nodeinfo (pack->p1, i, &nodes[n]) < 0)
	{
	  send_error ("Cannot get mixer node info\n");
	  return;
	}

      mixc_add_node (pack->p1, i, &nodes[n]);

      if (++n >= MAX_NODES)
	{
	  send_response_long (OSSMIX_CMD_GET_NODEINFO, n, i, 0, 0, 0,
			      (void *) &nodes, n * sizeof (oss_mixext), 0);
	  n = 0;
	}
    }

  if (n > 0)
    send_response_long (OSSMIX_CMD_GET_NODEINFO, n, pack->p3, 0, 0, 0,
			(void *) &nodes, n * sizeof (oss_mixext), 0);
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
	continue;

      if (ext->type == MIXT_DEVROOT || ext->type == MIXT_GROUP
	  || ext->type == MIXT_MARKER)
	continue;

      prev_value = mixc_get_value (mixernum, i);

      if ((value = ossmix_get_value (mixernum, i, ext->timestamp)) < 0)
	continue;
      // TODO check for EIDRM

      mixc_set_value (mixernum, i, value);
    }
}

static void
serve_command (ossmix_commad_packet_t * pack)
{
  switch (pack->cmd)
    {
    case OSSMIX_CMD_INIT:
      polling_started = 0;
      if (pack->ack_rq)
	send_ack ();
      break;

    case OSSMIX_CMD_EXIT:
      //fprintf(stderr, "Exit\n");
      polling_started = 0;
      memset(mixer_open_mask, 0, sizeof(mixer_open_mask));
      if (pack->ack_rq)
	send_ack ();
      break;

    case OSSMIX_CMD_START_EVENTS:
      polling_started = 1;
      break;

    case OSSMIX_CMD_GET_NMIXERS:
      return_value (num_mixers=ossmix_get_nmixers ());
      break;

    case OSSMIX_CMD_GET_MIXERINFO:
      {
	oss_mixerinfo mi;

	if (ossmix_get_mixerinfo (pack->p1, &mi) < 0)
	  send_error ("Cannot get mixer info\n");
	else
	  send_response_long (OSSMIX_CMD_OK, 0, 0, 0, 0, 0, (void *) &mi,
			      sizeof (mi), 0);
      }
      break;

    case OSSMIX_CMD_OPEN_MIXER:
      mixer_open_mask[pack->p1 / 8] |= (1<<(pack->p1 % 8)); // Open
      return_value (ossmix_open_mixer (pack->p1));
      break;

    case OSSMIX_CMD_CLOSE_MIXER:
      mixer_open_mask[pack->p1 / 8] &= ~(1<<(pack->p1 % 8)); // Closed
      ossmix_close_mixer (pack->p1);
      break;

    case OSSMIX_CMD_GET_NREXT:
      return_value (ossmix_get_nrext (pack->p1));
      break;

    case OSSMIX_CMD_GET_NODEINFO:
      {
	oss_mixext ext;

	if (pack->p3 > pack->p2)
	  {
	    send_multiple_nodes (pack);
	    break;
	  }

	if (ossmix_get_nodeinfo (pack->p1, pack->p2, &ext) < 0)
	  send_error ("Cannot get mixer node info\n");
	else
	  {
	    mixc_add_node (pack->p1, pack->p2, &ext);
	    send_response_long (OSSMIX_CMD_OK, 0, 0, 0, 0, 0, (void *) &ext,
				sizeof (ext), 0);
	  }
      }
      break;

    case OSSMIX_CMD_GET_ENUMINFO:
      {
	oss_mixer_enuminfo desc;

	if (ossmix_get_enuminfo (pack->p1, pack->p2, &desc) < 0)
	  send_error ("Cannot get mixer enum strings\n");
	else
	  send_response_long (OSSMIX_CMD_OK, 0, 0, 0, 0, 0, (void *) &desc,
			      sizeof (desc), 0);
      }
      break;

    case OSSMIX_CMD_GET_DESCRIPTION:
      {
	oss_mixer_enuminfo desc;

	if (ossmix_get_description (pack->p1, pack->p2, &desc) < 0)
	  send_error ("Cannot get mixer description\n");
	else
	  send_response_long (OSSMIX_CMD_OK, 0, 0, 0, 0, 0, (void *) &desc,
			      sizeof (desc), 0);
      }
      break;

    case OSSMIX_CMD_GET_VALUE:
      return_value (ossmix_get_value (pack->p1, pack->p2, pack->p3));
      break;

    case OSSMIX_CMD_GET_ALL_VALUES:
      {
	int n;
	value_packet_t value_packet;

	update_values (pack->p1);
	n = mixc_get_all_values (pack->p1, value_packet, 0);

	send_response_long (OSSMIX_CMD_GET_ALL_VALUES, n, pack->p1, 0, 0, 0,
			    (void *) &value_packet,
			    n * sizeof (value_record_t), 0);
	mixc_clear_changeflags (pack->p1);
      }
      break;

    case OSSMIX_CMD_SET_VALUE:
      ossmix_set_value (pack->p1, pack->p2, pack->p3, pack->p4);
      break;

    default:

      if (pack->ack_rq)
	send_error ("Unrecognized request");
    }
}

static void
poll_devices (void)
{
	int n;
	int mixernum;
	value_packet_t value_packet;

	for (mixernum=0;mixernum<num_mixers;mixernum++)
        if (mixer_open_mask[mixernum / 8] & 1<<(mixernum % 8))
	{
		update_values (mixernum);
		n = mixc_get_all_values (mixernum, value_packet, 1);
	
		if (n==0)	/* Nothing changed */
		   continue;

		send_response_long (OSSMIX_EVENT_VALUE, n, mixernum, 0, 0, 0,
				    (void *) &value_packet,
				    n * sizeof (value_record_t), 1);
		mixc_clear_changeflags (mixernum);
	}

	n=ossmix_get_nmixers();
	if (n>num_mixers)
	{
		num_mixers=n;
	 	send_response (OSSMIX_EVENT_NEWMIXER, n, 0, 0, 0, 0, 1);
	}
}

static void
handle_connection (int connfd)
{
  ossmix_commad_packet_t pack;
  struct timeval tmout;

  send_response (OSSMIX_CMD_HALOO, OSSMIX_P1_MAGIC, 0, 0, 0, 0, 0);

  tmout.tv_sec = 1;
  tmout.tv_usec = 0;

  while (1)
    {
      int ndevs;
      fd_set readfds, exfds;
      FD_ZERO (&readfds);
      FD_ZERO (&exfds);

      FD_SET (connfd, &readfds);
      FD_SET (connfd, &exfds);

      if ((ndevs = select (connfd + 1, &readfds, NULL, &exfds, &tmout)) == -1)
	{
	  perror ("select");
	  exit (-1);
	}

      if (ndevs == 0)
	{
	  if (polling_started)
	    {
	      poll_devices ();
	      tmout.tv_sec = 0;
	      tmout.tv_usec = 100000;
	    }
	  else
	    {
	      tmout.tv_sec = 1;
	      tmout.tv_usec = 0;
	    }
	}

      if (FD_ISSET (connfd, &readfds) || FD_ISSET (connfd, &exfds))
	{
	  if (read (connfd, &pack, sizeof (pack)) == sizeof (pack))
	    {
	      serve_command (&pack);
	    }
	  else
	    return;
	}
    }


}

int
main (int argc, char *argv[])
{
  struct sockaddr_in servaddr;
  int port = 7777;
  int c;
  int err;

  if ((err = ossmix_init ()) < 0)
    {
      fprintf (stderr, "ossmix_init() failed, err=%d\n", err);
      exit (EXIT_FAILURE);
    }

  if ((err = ossmix_connect (NULL, 0)) < 0)	/* Force local connection */
    {
      fprintf (stderr, "ossmix_connect() failed, err=%d\n", err);
      exit (EXIT_FAILURE);
    }

  while ((c = getopt (argc, argv, "vp:")) != EOF)
    {
      switch (c)
	{
	case 'p':		/* TCP/IP port */
	  port = atoi (optarg);
	  if (port <= 0)
	    port = 9876;
	  break;

	case 'v':		/* Verbose */
	  verbose++;
	  break;
	}
    }

  printf ("Listening socket %d\n", port);

  if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("socket");
      exit (-1);
    }

  memset (&servaddr, 0, sizeof (servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl (INADDR_ANY);
  servaddr.sin_port = htons (port);

  if (bind (listenfd, (struct sockaddr *) &servaddr, sizeof (servaddr)) == -1)
    {
      perror ("bind");
      exit (-1);
    }

  while (1)
    {

      if (!wait_connect ())
	exit (-1);

      handle_connection (connfd);
      close (connfd);
    }

  // close (listenfd); /* Not reached */
  exit (0);
}
