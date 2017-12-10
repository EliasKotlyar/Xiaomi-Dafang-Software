#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "../../include/soundcard.h"
#include <sys/time.h>

#undef  DEBUG

extern int __seqfd;

static midi_packet_t midi_packet;

static unsigned char *midibuf = &midi_packet.payload[0];
static int mp = 0;

oss_midi_time_t current_tick = 0;

static int timer_started = 0;

void
OSS_set_timebase (int tb)
{
/* TODO? */
}

#if 1
#define _write write
#else
static size_t
_write (int fildes, void *b, size_t nbyte)
{
  midi_packet_header_t *hdr = b;
  unsigned char *buf;
  int l = nbyte;


  buf = (unsigned char *) b + sizeof (*hdr);

  if (hdr->magic == MIDI_HDR_MAGIC)	/* Timed mode write */
    {
      l -= sizeof (*hdr);

      switch (hdr->event_type)
	{
	case MIDI_EV_WRITE:
	  printf ("%9lu: MIDI_EV_WRITE: ", hdr->time);
	  break;
	case MIDI_EV_TEMPO:
	  printf ("%9lu: MIDI_EV_TEMPO: ", hdr->time);
	  break;
	case MIDI_EV_ECHO:
	  printf ("%9lu: MIDI_EV_ECHO: ", hdr->time);
	  break;
	case MIDI_EV_START:
	  printf ("%9lu: MIDI_EV_START: ", hdr->time);
	  break;
	case MIDI_EV_STOP:
	  printf ("%9lu: MIDI_EV_STOP: ", hdr->time);
	  break;
	default:
	  printf ("%9lu: MIDI_EV_%d: ", hdr->time, hdr->event_type);
	  break;
	}

      if (hdr->options & MIDI_OPT_TIMED)
	printf ("TIMED ");

      if (l > 0)
	{
	  int i;

	  printf ("%d bytes (", l);

	  for (i = 0; i < l; i++)
	    printf ("%02x ", buf[i]);
	  printf (")");
	}

      printf ("\n");
    }

  return write (fildes, b, nbyte);
}
#endif

static void
start_timer (void)
{
#ifndef DEBUG
  midi_packet_header_t hdr;

  hdr.magic = MIDI_HDR_MAGIC;
  hdr.event_type = MIDI_EV_START;
  hdr.options = MIDI_OPT_NONE;
  hdr.time = 0;
  hdr.parm = 0;
  if (_write (__seqfd, &hdr, sizeof (hdr)) != sizeof (hdr))
    {
      perror ("Write start timer");
      exit (-1);
    }

  timer_started = 1;
  printf ("Timer started\n");
#endif
}

void
_dump_midi (void)
{
  if (mp > 0)
    {
      if (!timer_started)
	start_timer ();
      midi_packet.hdr.magic = MIDI_HDR_MAGIC;
      midi_packet.hdr.options = MIDI_OPT_TIMED;
      midi_packet.hdr.event_type = MIDI_EV_WRITE;
      midi_packet.hdr.parm = 0;
      midi_packet.hdr.time = current_tick;

#ifdef DEBUG
      if (write (1, &midi_packet.payload[0], mp) == -1)
#else
      if (_write (__seqfd, &midi_packet, sizeof (midi_packet_header_t) + mp)
	  == -1)
#endif
	{
	  perror ("MIDI write");
	  exit (-1);
	}
      mp = 0;
    }
}

static void
oss_do_timing (unsigned char *ev)
{
  unsigned int parm = *(unsigned int *) &ev[4];
  midi_packet_header_t hdr;

  oss_midi_time_t tick;

  _dump_midi ();

  switch (ev[1])
    {
    case TMR_TEMPO:
#ifndef DEBUG
      if (!timer_started)
	start_timer ();
      hdr.magic = MIDI_HDR_MAGIC;
      hdr.event_type = MIDI_EV_TEMPO;
      hdr.options = MIDI_OPT_TIMED;
      hdr.time = current_tick;
      hdr.parm = parm;
      if (_write (__seqfd, &hdr, sizeof (hdr)) != sizeof (hdr))
	{
	  perror ("Write tempo");
	  exit (-1);
	}
#endif
      break;

    case TMR_WAIT_REL:
      tick = current_tick + parm;

      current_tick = tick;
      break;

    case TMR_WAIT_ABS:
      tick = parm;
      current_tick = tick;
      break;
    }

}

static void
out_midi3 (unsigned char a, unsigned char b, unsigned char c)
{
  if (mp > 950)
    _dump_midi ();
  midibuf[mp++] = a;
  midibuf[mp++] = b;
  midibuf[mp++] = c;
/* printf("Out %02x %02x %02x\n", a, b, c); */
}

static void
out_midi2 (unsigned char a, unsigned char b)
{
  if (mp > 950)
    _dump_midi ();
  midibuf[mp++] = a;
  midibuf[mp++] = b;
/* printf("Out %02x %02x\n", a, b); */
}

void
play_event (unsigned char *ev)
{
  int i;

  switch (ev[0])
    {
    case EV_TIMING:
      oss_do_timing (ev);
      return;
      break;

    case EV_SEQ_LOCAL:
      break;

    case EV_CHN_COMMON:
      switch (ev[2])
	{
	case MIDI_PGM_CHANGE:
	  /* printf("PGM change %02x %d\n", ev[2]|ev[3], ev[4]); */
	  out_midi2 (ev[2] | ev[3], ev[4]);
	  break;

	case MIDI_CHN_PRESSURE:
	  out_midi2 (ev[2] | ev[3], ev[4]);
	  break;

	case MIDI_CTL_CHANGE:
	  out_midi3 (ev[2] | ev[3], ev[4], *(short *) &ev[6]);
	  break;

	default:
	  out_midi3 (ev[2] | ev[3], ev[4], *(short *) &ev[6]);
	}
      return;
      break;

    case EV_CHN_VOICE:
      out_midi3 (ev[2] | ev[3], ev[4], ev[5]);
      return;
      break;

    case EV_SYSEX:
      {
	int i, l;
	l = 8;
	for (i = 2; i < 8; i++)
	  if (ev[i] == 0xff)
	    {
	      l = i;
	      break;
	    }
	if (mp > 950)
	  _dump_midi ();
	for (i = 2; i < l; i++)
	  midibuf[mp++] = ev[i];
      }
      return;
      break;

    case EV_SYSTEM:
      printf ("EV_SYSTEM: ");
      break;

    default:
      printf ("Unknown event %d: ", ev[0]);

    }

  for (i = 0; i < 8; i++)
    printf ("%02x ", ev[i]);
  printf ("\n");
}
