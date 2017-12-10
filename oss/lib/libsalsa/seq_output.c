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

static int
midi_out3 (snd_seq_t * seq, int msg, int parm1, int parm2)
{
  unsigned char buf[3];
  int l;

  if (msg < 0 || msg >= 0xff)
    return -ERANGE;
  if (parm1 < 0 || parm1 > 0x7f)
    return -ERANGE;
  if (parm2 < 0 || parm2 > 0x7f)
    return -ERANGE;

  buf[0] = msg;
  buf[1] = parm1;
  buf[2] = parm2;

  if ((l = write (seq->fd, buf, 3)) != 3)
    {
      if (l == -1)
	return -errno;

      return -EBADE;		/* Randomly selected error */
    }

  return 0;
}

static int
midi_out2 (snd_seq_t * seq, int msg, int parm1)
{
  unsigned char buf[3];
  int l;

  if (msg < 0 || msg >= 0xff)
    return -ERANGE;
  if (parm1 < 0 || parm1 > 0x7f)
    return -ERANGE;

  buf[0] = msg;
  buf[1] = parm1;

  if ((l = write (seq->fd, buf, 2)) != 2)
    {
      if (l == -1)
	return -errno;

      return -EBADE;		/* Randomly selected error */
    }

  return 0;
}

int
convert_event (snd_seq_t * seq, snd_seq_event_t * ev)
{
  int value;

  dbg_printf3
    ("Event %2d: flags=%08x tag=%08x, q=%2d, time=%d, src=%x, dst=%x\n",
     ev->type, ev->flags, ev->tag, ev->queue, ev->time, ev->source, ev->dest);

  switch (ev->type)
    {
    case SND_SEQ_EVENT_CONTROLLER:
      dbg_printf3 ("\tSND_SEQ_EVENT_CONTRLLER %2d, %3d, %3d\n",
		   ev->data.control.channel,
		   ev->data.control.param, ev->data.control.value);
      if (ev->data.control.channel > 15)
	return -ERANGE;
      return midi_out3 (seq, 0xB0 + ev->data.control.channel,
			ev->data.control.param, ev->data.control.value);
      break;

    case SND_SEQ_EVENT_PGMCHANGE:
      dbg_printf3 ("\tSND_SEQ_EVENT_PGMCHANGE %2d, %3d, %3d\n",
		   ev->data.control.channel,
		   ev->data.control.param, ev->data.control.value);
      if (ev->data.control.channel > 15)
	return -ERANGE;
      return midi_out2 (seq, 0xC0 + ev->data.control.channel,
			ev->data.control.value);
      break;

    case SND_SEQ_EVENT_CHANPRESS:
      dbg_printf3 ("\tSND_SEQ_EVENT_CHANPRESS %2d, %5d\n",
		   ev->data.control.channel, ev->data.control.value);
      value = ev->data.control.value + 8192;
      if (ev->data.control.channel > 15)
	return -ERANGE;
      return midi_out3 (seq, 0xD0 + ev->data.control.channel,
			value & 0x7f, (value >> 7) & 0x7f);
      break;

    case SND_SEQ_EVENT_PITCHBEND:
      dbg_printf3 ("\tSND_SEQ_EVENT_PITCHBEND %2d, %5d\n",
		   ev->data.control.channel, ev->data.control.value);
      value = ev->data.control.value + 8192;
      if (ev->data.control.channel > 15)
	return -ERANGE;
      return midi_out3 (seq, 0xE0 + ev->data.control.channel,
			value & 0x7f, (value >> 7) & 0x7f);
      break;

    case SND_SEQ_EVENT_NOTEON:
      dbg_printf3 ("\tSND_SEQ_EVENT_NOTEON  %2d, %3d, %d\n",
		   ev->data.note.channel,
		   ev->data.note.note, ev->data.note.velocity);
      if (ev->data.control.channel > 15)
	return -ERANGE;
      return midi_out3 (seq, 0x90 + ev->data.note.channel,
			ev->data.note.note, ev->data.note.velocity);
      break;

    case SND_SEQ_EVENT_NOTEOFF:
      dbg_printf3 ("\tSND_SEQ_EVENT_NOTEOFF %2d, %3d, %d\n",
		   ev->data.note.channel,
		   ev->data.note.note, ev->data.note.velocity);
      if (ev->data.control.channel > 15)
	return -ERANGE;
      return midi_out3 (seq, 0x80 + ev->data.note.channel,
			ev->data.note.note, ev->data.note.velocity);
      break;

    case SND_SEQ_EVENT_KEYPRESS:
      dbg_printf3 ("\tSND_SEQ_EVENT_KEYPRESS %2d, %3d, %d\n",
		   ev->data.note.channel,
		   ev->data.note.note, ev->data.note.velocity);
      if (ev->data.control.channel > 15)
	return -ERANGE;
      return midi_out3 (seq, 0xA0 + ev->data.note.channel,
			ev->data.note.note, ev->data.note.velocity);
      break;

    case SND_SEQ_EVENT_SYSTEM:
      dbg_printf ("\tSND_SEQ_EVENT_SYSTEM\n");
      break;
    case SND_SEQ_EVENT_RESULT:
      dbg_printf ("\tSND_SEQ_EVENT_RESULT\n");
      break;
    case SND_SEQ_EVENT_NOTE:
      dbg_printf ("\tSND_SEQ_EVENT_NOTE\n");
      break;
    case SND_SEQ_EVENT_CONTROL14:
      dbg_printf ("\tSND_SEQ_EVENT_CONTROL14\n");
      break;
    case SND_SEQ_EVENT_NONREGPARAM:
      dbg_printf ("\tSND_SEQ_EVENT_NONREGPARAM\n");
      break;
    case SND_SEQ_EVENT_REGPARAM:
      dbg_printf ("\tSND_SEQ_EVENT_REGPARAM\n");
      break;
    case SND_SEQ_EVENT_SONGPOS:
      dbg_printf ("\tSND_SEQ_EVENT_SONGPOS\n");
      break;
    case SND_SEQ_EVENT_SONGSEL:
      dbg_printf ("\tSND_SEQ_EVENT_SONGSEL\n");
      break;
    case SND_SEQ_EVENT_QFRAME:
      dbg_printf ("\tSND_SEQ_EVENT_QFRAME\n");
      break;
    case SND_SEQ_EVENT_TIMESIGN:
      dbg_printf ("\tSND_SEQ_EVENT_TIMESIGN\n");
      break;
    case SND_SEQ_EVENT_KEYSIGN:
      dbg_printf ("\tSND_SEQ_EVENT_KEYSIGN\n");
      break;
    case SND_SEQ_EVENT_START:
      dbg_printf ("\tSND_SEQ_EVENT_START\n");
      break;
    case SND_SEQ_EVENT_CONTINUE:
      dbg_printf ("\tSND_SEQ_EVENT_CONTINUE\n");
      break;
    case SND_SEQ_EVENT_STOP:
      dbg_printf ("\tSND_SEQ_EVENT_STOP\n");
      break;
    case SND_SEQ_EVENT_SETPOS_TICK:
      dbg_printf ("\tSND_SEQ_EVENT_SETPOS_TICK\n");
      break;
    case SND_SEQ_EVENT_SETPOS_TIME:
      dbg_printf ("\tSND_SEQ_EVENT_SETPOS_TIME\n");
      break;
    case SND_SEQ_EVENT_TEMPO:
      dbg_printf ("\tSND_SEQ_EVENT_TEMPO\n");
      break;
    case SND_SEQ_EVENT_CLOCK:
      dbg_printf ("\tSND_SEQ_EVENT_CLOCK\n");
      break;
    case SND_SEQ_EVENT_TICK:
      dbg_printf ("\tSND_SEQ_EVENT_TICK\n");
      break;
    case SND_SEQ_EVENT_QUEUE_SKEW:
      dbg_printf ("\tSND_SEQ_EVENT_QUEUE_SKEW\n");
      break;
    case SND_SEQ_EVENT_SYNC_POS:
      dbg_printf ("\tSND_SEQ_EVENT_SYNC_POS\n");
      break;
    case SND_SEQ_EVENT_TUNE_REQUEST:
      dbg_printf ("\tSND_SEQ_EVENT_TUNE_REQUEST\n");
      break;
    case SND_SEQ_EVENT_RESET:
      dbg_printf ("\tSND_SEQ_EVENT_RESET\n");
      break;
    case SND_SEQ_EVENT_SENSING:
      dbg_printf ("\tSND_SEQ_EVENT_SENSING\n");
      break;
    case SND_SEQ_EVENT_ECHO:
      dbg_printf ("\tSND_SEQ_EVENT_ECHO\n");
      break;
    case SND_SEQ_EVENT_OSS:
      dbg_printf ("\tSND_SEQ_EVENT_OSS\n");
      break;
    case SND_SEQ_EVENT_CLIENT_START:
      dbg_printf ("\tSND_SEQ_EVENT_CLIENT_START\n");
      break;
    case SND_SEQ_EVENT_CLIENT_EXIT:
      dbg_printf ("\tSND_SEQ_EVENT_CLIENT_EXIT\n");
      break;
    case SND_SEQ_EVENT_CLIENT_CHANGE:
      dbg_printf ("\tSND_SEQ_EVENT_CLIENT_CHANGE\n");
      break;
    case SND_SEQ_EVENT_PORT_START:
      dbg_printf ("\tSND_SEQ_EVENT_PORT_START\n");
      break;
    case SND_SEQ_EVENT_PORT_EXIT:
      dbg_printf ("\tSND_SEQ_EVENT_PORT_EXIT\n");
      break;
    case SND_SEQ_EVENT_PORT_CHANGE:
      dbg_printf ("\tSND_SEQ_EVENT_PORT_CHANGE\n");
      break;
    case SND_SEQ_EVENT_PORT_SUBSCRIBED:
      dbg_printf ("\tSND_SEQ_EVENT_PORT_SUBSCRIBED\n");
      break;
    case SND_SEQ_EVENT_PORT_UNSUBSCRIBED:
      dbg_printf ("\tSND_SEQ_EVENT_PORT_UNSUBSCRIBED\n");
      break;
    case SND_SEQ_EVENT_USR0:
      dbg_printf ("\tSND_SEQ_EVENT_USR0\n");
      break;
    case SND_SEQ_EVENT_USR1:
      dbg_printf ("\tSND_SEQ_EVENT_USR1\n");
      break;
    case SND_SEQ_EVENT_USR2:
      dbg_printf ("\tSND_SEQ_EVENT_USR2\n");
      break;
    case SND_SEQ_EVENT_USR3:
      dbg_printf ("\tSND_SEQ_EVENT_USR3\n");
      break;
    case SND_SEQ_EVENT_USR4:
      dbg_printf ("\tSND_SEQ_EVENT_USR4\n");
      break;
    case SND_SEQ_EVENT_USR5:
      dbg_printf ("\tSND_SEQ_EVENT_USR5\n");
      break;
    case SND_SEQ_EVENT_USR6:
      dbg_printf ("\tSND_SEQ_EVENT_USR6\n");
      break;
    case SND_SEQ_EVENT_USR7:
      dbg_printf ("\tSND_SEQ_EVENT_USR7\n");
      break;
    case SND_SEQ_EVENT_USR8:
      dbg_printf ("\tSND_SEQ_EVENT_USR8\n");
      break;
    case SND_SEQ_EVENT_USR9:
      dbg_printf ("\tSND_SEQ_EVENT_USR9\n");
      break;
    case SND_SEQ_EVENT_SYSEX:
      dbg_printf ("\tSND_SEQ_EVENT_SYSEX\n");
      break;
    case SND_SEQ_EVENT_BOUNCE:
      dbg_printf ("\tSND_SEQ_EVENT_BOUNCE\n");
      break;
    case SND_SEQ_EVENT_USR_VAR0:
      dbg_printf ("\tSND_SEQ_EVENT_USR_VAR0\n");
      break;
    case SND_SEQ_EVENT_USR_VAR1:
      dbg_printf ("\tSND_SEQ_EVENT_USR_VAR1\n");
      break;
    case SND_SEQ_EVENT_USR_VAR2:
      dbg_printf ("\tSND_SEQ_EVENT_USR_VAR2\n");
      break;
    case SND_SEQ_EVENT_USR_VAR3:
      dbg_printf ("\tSND_SEQ_EVENT_USR_VAR3\n");
      break;
    case SND_SEQ_EVENT_USR_VAR4:
      dbg_printf ("\tSND_SEQ_EVENT_USR_VAR4\n");
      break;
    case SND_SEQ_EVENT_NONE:
      dbg_printf ("\tSND_SEQ_EVENT_NONE\n");
      break;
    default:
      dbg_printf ("\tUnknown event type %d\n", ev->type);
    }
  return 0;
}
