#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/soundcard.h"
#include "midiparser.h"

struct _mtc_state
{
  int prev_ix;
  int state;
  int offset;
  oss_mtc_data_t mtc, mtc0;
};

typedef struct midi_input_info
{				/* MIDI input scanner variables */
#define MI_MAX	64
  int m_busy;
  unsigned char m_buf[MI_MAX];
  unsigned char m_prev_status;	/* For running status */
  int m_ptr;
#define MST_INIT			0
#define MST_DATA			1
#define MST_SYSEX			2
  int m_state;
  int m_left;
  int m_f1_flag;
} midi_input_info_t;

struct midiparser_common
{
  midi_input_info_t inc;
  midiparser_callback_t callback;
  midiparser_mtc_callback_t mtc_callback;
  void *client_context;
  struct _mtc_state mtc_state;
};

#define DBG(x) {}

#define CALLBACK(cat, msg, ch, p1, p2, p3) \
	{ \
		unsigned char arr[3];arr[0]=p1;arr[1]=p2;arr[2]=p3; \
		synth->callback(synth->client_context, cat, msg, ch, arr, 3); \
	}
#define CALLBACK_A(cat, msg, ch, parms, len) \
	synth->callback(synth->client_context, cat, msg, ch, parms, len)

void
do_system_msg (midiparser_common_p synth, unsigned char *msg, int mlen)
{
  DBG (printf ("System msg %02x, %02x (%d)\n", msg[0], msg[1], mlen));
  CALLBACK_A (CAT_REALTIME, *msg, 0, msg, mlen);
  return;
}

static void
do_sysex_msg (midiparser_common_p synth, unsigned char *msg, int mlen)
{
  CALLBACK_A (CAT_SYSEX, 0, 0, msg, mlen);
  return;
}

static void
do_realtime_msg (midiparser_common_p synth, unsigned char data)
{
}

void
do_midi_msg (midiparser_common_p synth, unsigned char *msg, int mlen)
{
  switch (msg[0] & 0xf0)
    {
    case 0x90:
      if (msg[2] != 0)
	{
	  CALLBACK (CAT_VOICE, 0x90, msg[0] & 0x0f, msg[1], msg[2], 0);
	  break;
	}
      msg[2] = 64;

    case 0x80:
      CALLBACK (CAT_VOICE, 0x80, msg[0] & 0x0f, msg[1], msg[2], 0);
      break;

    case 0xA0:
      CALLBACK (CAT_VOICE, 0xA0, msg[0] & 0x0f, msg[1], msg[2], 0);
      break;

    case 0xB0:
      CALLBACK (CAT_CHN, 0xB0, msg[0] & 0x0f, msg[1], msg[2], 0);
      break;

    case 0xC0:
      CALLBACK (CAT_CHN, 0xC0, msg[0] & 0x0f, msg[1], 0, 0);
      break;

    case 0xD0:
      CALLBACK (CAT_CHN, 0xD0, msg[0] & 0x0f, msg[1], 0, 0);
      break;

    case 0xE0:
      CALLBACK (CAT_VOICE, 0xE0, msg[0] & 0x0f, msg[1], msg[2], 0);
      break;

    case 0xf0:			/* System common messages */
      do_system_msg (synth, msg, mlen);
      break;

    default:
      ;
    }
}

static void
send_mtc (midiparser_common_p synth, struct _mtc_state *st)
{
  oss_mtc_data_t mtc;

  memcpy (&mtc, &st->mtc, sizeof (mtc));

  mtc.qframes += st->offset;
  mtc.frames += mtc.qframes / 4;
  mtc.qframes %= 4;

  if (mtc.time_code_type == 0)
    mtc.time_code_type = 30;
  mtc.seconds += mtc.frames / mtc.time_code_type;	/* TODO: Handle drop frames */
  mtc.frames %= mtc.time_code_type;

  mtc.minutes += mtc.seconds / 60;
  mtc.seconds %= 60;

  mtc.hours += mtc.minutes / 60;
  mtc.minutes %= 60;

#if 0
  printf ("%2d:%02d:%02d %02d.%d %2dFPS\n",
	  mtc.hours, mtc.minutes,
	  mtc.seconds, mtc.frames, mtc.qframes, mtc.time_code_type);
#endif

  synth->mtc_callback (synth->client_context, &mtc);
}

void
mtc_message (midiparser_common_p synth, struct _mtc_state *st,
	     unsigned char b)
{
  static char frame_types[4] = { 24, 25, 29, 30 };
  int ix, data;
  int previx;

  ix = b >> 4;
  data = b & 0x0f;

  previx = (st->prev_ix + 1) % 8;

  if (ix == previx)
    st->mtc0.direction = st->mtc.direction = MTC_DIR_FORWARD;
  else if (ix == st->prev_ix)
    st->mtc0.direction = st->mtc.direction = MTC_DIR_STOPPED;
  else
    st->mtc0.direction = st->mtc.direction = MTC_DIR_BACKWARD;
  st->prev_ix = ix;

  if (st->state == 0)
    {
      if (ix != 0)		/* Not the beginning of the sequence yet */
	return;
      st->state = 1;
      st->offset = -1;
    }

  switch (ix)
    {
    case 0:			/* Frame count LS nibble */
      st->mtc0.qframes = 0;
      st->mtc0.frames = data;
      break;

    case 1:			/* Frame count MS nibble */
      st->mtc0.frames |= data << 4;
      break;

    case 2:			/* Seconds count LS nibble */
      st->mtc0.seconds = data;
      break;

    case 3:			/* Seconds count MS nibble */
      st->mtc0.seconds |= data << 4;
      break;

    case 4:			/* Minutes count LS nibble */
      st->mtc0.minutes = data;
      break;

    case 5:			/* Minutes count MS nibble */
      st->mtc0.minutes |= data << 4;
      break;

    case 6:			/* Hours count LS nibble */
      st->mtc0.hours = data;
      break;

    case 7:			/* Hours count MS nibble */
      st->mtc0.hours |= data << 4;
      st->mtc0.time_code_type = frame_types[(st->mtc0.hours >> 5) & 0x03];
      st->mtc0.hours &= 0x1f;
      memcpy (&st->mtc, &st->mtc0, sizeof (st->mtc));
      break;
    }

  if (ix == 7)
    st->offset = 7;
  else
    st->offset++;
  send_mtc (synth, st);
}

static void
handle_midi_input (midiparser_common_p synth, midi_input_info_t * inc,
		   unsigned char data)
{
  extern int seq_actsense_enable;
  extern int seq_rt_enable;

  static unsigned char len_tab[] =	/* # of data bytes following a status
					 */
  {
    2,				/* 8x */
    2,				/* 9x */
    2,				/* Ax */
    2,				/* Bx */
    1,				/* Cx */
    1,				/* Dx */
    2,				/* Ex */
    0				/* Fx */
  };
  /* printf("%02x (%d) ", data, inc->m_state); */

  if (data == 0xfe)		/* Active sensing */
    {
      return;
    }

  if (data >= 0xf8)		/* Real time message */
    {
      do_realtime_msg (synth, data);
      CALLBACK (CAT_REALTIME, data, 0, 0, 0, 0);
      return;
    }

  if (data == 0xf1)		/* MTC quarter frame (1st byte) */
    {
      inc->m_f1_flag = 1;
      return;
    }

  if (inc->m_f1_flag)		/* MTC quarter frame (2nd byte) */
    {
      inc->m_f1_flag = 0;

      if (synth->mtc_callback != NULL)
	{
	  mtc_message (synth, &synth->mtc_state, data);
	  return;
	}
      CALLBACK (CAT_MTC, 0xf1, 0, data, 0, 0);
      return;
    }

  switch (inc->m_state)
    {
    case MST_INIT:
      if (data & 0x80)		/* MIDI status byte */
	{
	  if ((data & 0xf0) == 0xf0)	/* Common message */
	    {
	      switch (data)
		{
		case 0xf0:	/* Sysex */
		  inc->m_state = MST_SYSEX;
		  inc->m_ptr = 1;
		  inc->m_left = MI_MAX;
		  inc->m_buf[0] = data;
		  break;	/* Sysex */

		case 0xf1:	/* MTC quarter frame */
		case 0xf3:	/* Song select */
		  inc->m_state = MST_DATA;
		  inc->m_ptr = 1;
		  inc->m_left = 1;
		  inc->m_buf[0] = data;
		  break;

		case 0xf2:	/* Song position pointer */
		  inc->m_state = MST_DATA;
		  inc->m_ptr = 1;
		  inc->m_left = 2;
		  inc->m_buf[0] = data;
		  break;

		default:	/* Other common messages */
		  inc->m_buf[0] = data;
		  inc->m_ptr = 1;
		  do_midi_msg (synth, inc->m_buf, inc->m_ptr);
		  inc->m_ptr = 0;
		  inc->m_left = 0;
		}
	    }
	  else
	    {
	      /* Channel messages */
	      inc->m_state = MST_DATA;
	      inc->m_ptr = 1;
	      inc->m_left = len_tab[(data >> 4) - 8];
	      inc->m_buf[0] = inc->m_prev_status = data;
	    }
	}
      else /* Running status */ if (inc->m_prev_status & 0x80)	/* Ignore if no previous status (yet) */
	{
	  inc->m_state = MST_DATA;
	  inc->m_ptr = 2;
	  inc->m_left = len_tab[(inc->m_prev_status >> 4) - 8] - 1;
	  inc->m_buf[0] = inc->m_prev_status;
	  inc->m_buf[1] = data;
	}
      break;			/* MST_INIT */

    case MST_DATA:
      inc->m_buf[inc->m_ptr++] = data;
      if (--inc->m_left <= 0)
	{
	  inc->m_state = MST_INIT;
	  do_midi_msg (synth, inc->m_buf, inc->m_ptr);
	  inc->m_ptr = 0;
	}
      break;			/* MST_DATA */

    case MST_SYSEX:
      inc->m_buf[inc->m_ptr++] = data;
      if (data == 0xf7)		/* Sysex end */
	{
	  do_sysex_msg (synth, inc->m_buf, inc->m_ptr);
	  inc->m_state = MST_INIT;
	  inc->m_left = 0;
	  inc->m_ptr = 0;

	}
      else if (inc->m_ptr >= MI_MAX)	/* Overflow protection */
	{
	  do_sysex_msg (synth, inc->m_buf, inc->m_ptr);
	  inc->m_ptr = 0;
	}

      break;			/* MST_SYSEX */

    default:
      inc->m_state = MST_INIT;
    }
}

midiparser_common_p
midiparser_create (midiparser_callback_t callback, void *context)
{
  midiparser_common_p synth;

  if ((synth = malloc (sizeof (*synth))) == NULL)
    return NULL;

  memset (synth, 0, sizeof (*synth));

  synth->callback = callback;
  synth->client_context = context;
  synth->mtc_state.prev_ix = -1;

  return synth;
}

void
midiparser_mtc_callback (midiparser_common_p common,
			 midiparser_mtc_callback_t callback)
{
  common->mtc_callback = callback;
}

void
midiparser_input (midiparser_common_p synth, unsigned char data)
{
  handle_midi_input (synth, &synth->inc, data);
}

void
midiparser_input_buf (midiparser_common_p synth, unsigned char *data, int len)
{
  int i;

  for (i = 0; i < len; i++)
    handle_midi_input (synth, &synth->inc, data[i]);
}
