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

static snd_seq_event_t *
alloc_event (snd_seq_t * seq)
{
  snd_seq_event_t *ev;

  if (seq->nevents >= MAX_EVENTS)
    {
      fprintf (stderr, "libsalsa: Local buffer overflow - event dropped\n");
      return NULL;
    }

  ev = &seq->events[seq->nevents++];

  ev->type = SND_SEQ_EVENT_SENSING;	/* NOP message */

  return ev;
}

static void
voice_message (snd_seq_t * seq, unsigned char msg, unsigned char ch,
	       unsigned char *parms, int len)
{
  snd_seq_event_t *ev;
  if ((ev = alloc_event (seq)) == NULL)
    return;

  dbg_printf3 ("Voice message %02x, ch=%d, %3d, %3d\n", msg, ch, parms[0],
	       parms[1]);

  switch (msg)
    {
    case MIDI_NOTEON:
      ev->type = SND_SEQ_EVENT_NOTEON;
      ev->data.note.channel = ch;
      ev->data.note.note = parms[0];
      ev->data.note.velocity = parms[1];
      break;

    case MIDI_NOTEOFF:
      ev->type = SND_SEQ_EVENT_NOTEOFF;
      ev->data.note.channel = ch;
      ev->data.note.note = parms[0];
      ev->data.note.velocity = parms[1];
      break;
    }
}

static void
channel_message (snd_seq_t * seq, unsigned char msg, unsigned char ch,
		 unsigned char *parms, int len)
{
  snd_seq_event_t *ev;
  if ((ev = alloc_event (seq)) == NULL)
    return;

  dbg_printf3 ("Channel message %02x, ch=%d, %3d, %3d\n", msg, ch, parms[0],
	       parms[1]);
}

static void
realtime_message (snd_seq_t * seq, unsigned char msg, unsigned char *parms,
		  int len)
{
  snd_seq_event_t *ev;
  if ((ev = alloc_event (seq)) == NULL)
    return;

  dbg_printf3 ("Realtime message %02x, %2x\n", msg, parms[0]);
}

static void
sysex_message (snd_seq_t * seq, unsigned char *parms, int len)
{
  int i;
  snd_seq_event_t *ev;
  if ((ev = alloc_event (seq)) == NULL)
    return;


  if (alib_verbose > 2)
    {
      printf ("Sysex message: ");
      for (i = 0; i < len; i++)
	printf ("%02x ", parms[i]);
      printf ("\n");
    }
}

void
midiparser_callback (void *context, int category, unsigned char msg,
		     unsigned char ch, unsigned char *parms, int len)
{

  switch (category)
    {
    case CAT_VOICE:
      voice_message ((snd_seq_t *) context, msg, ch, parms, len);
      break;

    case CAT_CHN:
      channel_message ((snd_seq_t *) context, msg, ch, parms, len);
      break;

    case CAT_REALTIME:
      realtime_message ((snd_seq_t *) context, msg, parms, len);
      break;

    case CAT_SYSEX:
      sysex_message ((snd_seq_t *) context, parms, len);
      break;

    case CAT_MTC:
    default:
      dbg_printf ("Unknown MIDI message category %d\n", category);
    }
}
