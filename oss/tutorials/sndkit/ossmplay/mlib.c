#define DEB(stmt)	/* stmt	*/	/* For debug printing */
#define DEB2(stmt)	/* stmt */	/* For debug printing */
#define DEB3(stmt)	/* stmt */	/* For debug printing */
#define DEB_ALWAYS(stmt) /* stmt */	/* Interesting things */

#define _INCLUDE_POSIX_SOURCE 1
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/errno.h>

extern int errno;

#define USE_SIMPLE_MACROS
#include "../../../include/soundcard.h"

#include "mlib.h"

#define _seqbuf eventptr
#define _seqbufptr 4
#define _SEQ_ADVBUF(len) *optr = *optr + 1;

#define STORE(cmd) \
	if (track->events) \
	  { \
	  	unsigned char *eventptr = &(track->events[*optr*12]); \
		*(int *)&eventptr[0] = track->current_time; \
	  	cmd; \
	  } \
	else \
	  { \
	    *optr =  *optr + 1; \
	  }

#define META_EVENT(type, buf, len) \
	{ \
		eventptr[_seqbufptr + 0] = EV_PRIVATE_META; \
		eventptr[_seqbufptr + 1] = (type); \
		*(short*)&eventptr[_seqbufptr + 2] = (short)len; \
		*(char**)&eventptr[_seqbufptr + 4] = malloc(len); \
		memcpy((char**)&eventptr[_seqbufptr + 4], &(buf), len); \
		*optr = *optr + 1; \
	}

#define MAX_EVENT_SIZE	4095

static int midi_msglen[8] = {
  2,				/* 8x = Note off */
  2,				/* 9x = Note on */
  2,				/* Ax = Polyphonic key pressure (After-touch) */
  2,				/* Bx = Control change */
  1,				/* Cx = Program change */
  1,				/* Dx = Channel pressure (After-touch) */
  2,				/* Ex = Pitch wheel change */
  0				/* Fx = system messages */
};

static char mlib_errstring[256] = "No errors so far\n";

static char *
mlib_seterr (char *msg)
{
  strncpy (mlib_errstring, msg, sizeof (mlib_errstring));
  return NULL;
}

static char *
mlib_syserr (char *name)
{
  sprintf (mlib_errstring, "%s: %s", name, strerror (errno));
  return NULL;
}

int
mlib_chkdesc (mlib_desc * desc)
{
  if (desc == NULL)
    {
      mlib_seterr ("NULL mlib descriptor specified");
      return 0;
    }

  if (desc->magic != 0x121234)
    {
      mlib_seterr ("Invalid MAGIC in the mlib descriptor");
      return 0;
    }


  if (desc->fd < 0)
    {
      mlib_seterr ("Invalid fd in the mlib descriptor");
      return 0;
    }

  return 1;
}

static int
mlib_loadbuf (mlib_desc * desc)
{
  desc->bufp = 0;

  if ((desc->bufcnt = read (desc->fd, desc->buf, sizeof (desc->buf))) == -1)
    {
      mlib_syserr (desc->path);
      return 0;
    }

  if (desc->bufcnt == 0)
    {
      mlib_seterr ("End of MIDI file");
      return 0;
    }

  return 1;
}

static int
mlib_seek (mlib_desc * desc, long pos)
{

  if (lseek (desc->fd, pos, 0) == -1)
    {
      mlib_syserr (desc->path);
      return 0;
    }

  desc->bufcnt = 0;
  desc->bufp = 1;

  return 1;
}

static int
mlib_rdstr (mlib_desc * desc, char *target, int nchars)
{
  int i;

  if (!mlib_chkdesc (desc))
    return 0;

  for (i = 0; i < nchars; i++)
    {
      if (desc->bufp >= desc->bufcnt)
	if (!mlib_loadbuf (desc))
	  return 0;

      target[i] = desc->buf[desc->bufp];
      desc->bufp++;
    }

  target[nchars] = 0;		/* End of string */

  return nchars;
}

static int
mlib_rdvarlen (mlib_desc * desc, int *target)
{
  int tmp, i;

  *target = 0;

  for (i = 0; i < 6; i++)	/* Read at most 6 bytes */
    {
      if (desc->bufp >= desc->bufcnt)
	if (!mlib_loadbuf (desc))
	  return 0;

      tmp = desc->buf[desc->bufp];
      desc->bufp++;

      *target <<= 7;
      *target |= tmp & 0x7f;	/* Extract 7 bits */
      if ((tmp & 0x80) == 0)
	return i + 1;		/* Last byte */
    }

  mlib_seterr ("Unterminated variable length value");

  return 0;
}

static int
mlib_rdint16 (mlib_desc * desc, int *target)
{
  int tmp, i;

  *target = 0;

  for (i = 0; i < 2; i++)
    {
      if (desc->bufp >= desc->bufcnt)
	if (!mlib_loadbuf (desc))
	  return 0;

      tmp = desc->buf[desc->bufp];
      desc->bufp++;

      *target <<= 8;
      *target |= tmp & 0xff;
    }

  return 2;
}

static int
mlib_rdbyte (mlib_desc * desc, unsigned char *target)
{
  if (desc->bufp >= desc->bufcnt)
    if (!mlib_loadbuf (desc))
      return 0;

  *target = (char) (desc->buf[desc->bufp]);
  desc->bufp++;
  return 1;
}

static int
mlib_rdint32 (mlib_desc * desc, int *target)
{
  int tmp, i;

  *target = 0;

  for (i = 0; i < 4; i++)
    {
      if (desc->bufp >= desc->bufcnt)
	if (!mlib_loadbuf (desc))
	  return 0;

      tmp = desc->buf[desc->bufp];
      desc->bufp++;

      *target <<= 8;
      *target += tmp & 0xff;
    }

  return 4;
}

static int
mlib_read_mthd (mlib_desc * desc)
{
  char sig[5];
  unsigned char hi, lo;
  int len;

  if (!mlib_chkdesc (desc))
    return 0;

  if (!mlib_rdstr (desc, sig, 4))
    return 0;

  if (strcmp (sig, "MThd"))
    {
      mlib_seterr ("Invalid header signature (!= MThd)");
      return 0;
    }

  if (mlib_rdint32 (desc, &len))

    if (mlib_rdint16 (desc, &desc->hdr.MThd_fmt))
      DEB2 (printf ("Header: Format %d\n", desc->hdr.MThd_fmt));

  if (mlib_rdint16 (desc, &desc->hdr.MThd_ntrk))
    DEB2 (printf ("Header: ntrks %d\n", desc->hdr.MThd_ntrk));

  if (mlib_rdbyte (desc, &hi))
    if (mlib_rdbyte (desc, &lo))

      if (hi & 0x80)		/* Negative */
	{
	  DEB2 (printf ("SMPTE timing: format = %d, resolution = %d\n",
			(char) hi, lo));
	  desc->hdr.time_mode = TIME_SMPTE;
	  desc->hdr.SMPTE_format = (char) hi;
	  desc->hdr.SMPTE_resolution = lo;
	}
      else
	{
	  desc->hdr.time_mode = TIME_MIDI;
	  desc->hdr.division = (hi << 8) + lo;
	  DEB2 (printf ("Midi timing: timebase = %d ppqn\n",
			desc->hdr.division));
	}

  desc->curr_trk = -1;
  desc->trk_offs = 0;
  desc->next_trk_offs = 4 + 4 + len;;

  return mlib_seek (desc, desc->trk_offs);	/* Point to the first track */
}

static int
mlib_store_chn_voice_msg (mlib_desc * desc, mlib_track * track, int *optr,
			  unsigned char *evbuf, int len)
{

  unsigned char chn, pgm, note, vel;

  chn = evbuf[0] & 0x0f;	/* Midi channel */

  if (track->init_chn == -1)
    track->init_chn = chn;
  else if (chn != track->init_chn)
    track->flags |= TRK_MULTICHN;

  track->chnmask |= (1 << chn);	/* Update channel mask */

  switch (evbuf[0] & 0xf0)
    {
    case 0x90:			/* Note on */
      vel = evbuf[2];
      note = evbuf[1];

      if (vel != 0)
	{			/* Velocity given -> true note on event */
	  track->flags |= TRK_NOTES;

	  if (note > track->max_note)
	    track->max_note = note;

	  if (note < track->min_note)
	    track->min_note = note;

	  if (track->noteon_time == -1)
	    track->noteon_time = track->current_time;

	  if (vel != 64)
	    track->flags |= TRK_VEL_NOTEON;

	  STORE (SEQ_START_NOTE (0, chn, note, vel));
	  if (chn == 9)		/* Drum channel */
	    track->drum_map[note] = note;
	  break;		/* NOTE! The break is here, not later */
	}
      /* Note on with zero G -> note off with vel=64. */
      /* Fall to the next case */
      evbuf[2] = 64;		/* Velocity for the note off handler */

    case 0x80:			/* Note off */
      vel = evbuf[2];
      note = evbuf[1];

      if (vel != 64)
	track->flags |= TRK_VEL_NOTEOFF;
      STORE (SEQ_STOP_NOTE (0, chn, note, vel));
      break;

    case 0xA0:			/* Polyphonic key pressure/Aftertouch */
      track->flags |= TRK_POLY_AFTERTOUCH | TRK_AFTERTOUCH;
      STORE (SEQ_KEY_PRESSURE (0, chn, note, vel));
      break;

    case 0xB0:			/* Control change */
      {
	unsigned short value = evbuf[2];	/* Incorrect */
	unsigned char ctl = evbuf[1];

	track->flags |= TRK_CONTROLS;
	STORE (SEQ_CONTROL (0, chn, ctl, value));
      }
      break;

    case 0xC0:			/* Program change */
      pgm = evbuf[1];
      if (track->init_pgm == -1)
	track->init_pgm = pgm;
      else if (pgm != track->init_pgm)
	track->flags |= TRK_MULTIPGM;

      track->pgm_map[pgm] = pgm;
      STORE (SEQ_SET_PATCH (0, chn, pgm));
      break;

    case 0xD0:			/* Channel pressure/Aftertouch */
      track->flags |= TRK_AFTERTOUCH;
      STORE (SEQ_CHN_PRESSURE (0, chn, evbuf[1]));
      break;

    case 0xE0:			/* Pitch bend change */
      track->flags |= TRK_BENDER;
      STORE (SEQ_BENDER
	     (0, chn, (evbuf[1] & 0x7f) + ((evbuf[2] & 0x7f) << 7)));
      break;

    default:
      mlib_seterr ("Internal error: Unexpected midi event");
      fprintf (stderr, "Internal error: Unexpected midi event %02x\n",
	       evbuf[0]);

      return 0;
    }

  return 1;
}

static int
mlib_store_meta_event (mlib_desc * desc, mlib_track * track, int *optr,
		       unsigned char *evbuf, int len)
{
  int type = evbuf[0];
  int i;

/*
 * The event is stored at the end of this function. If there is no 
 * need to store the event, the case entry should return (return 1).
 */

  switch (type)
    {
    case 0x00:			/* Extension events ?????????????? */
      /* Not supported yet */
      break;

    case 0x01:			/* Descriptive text */
    case 0x02:			/* Copyright notice */
    case 0x03:			/* Sequence/track name */
    case 0x04:			/* Instrument name */
    case 0x05:			/* Lyric */
    case 0x06:			/* Marker */
    case 0x07:			/* Cue point */
#if 0
      for (i = 0; i < len - 1; i++)
	printf ("%c", evbuf[1 + i]);
      printf ("\n");
#endif
      break;

    case 0x21:			/* What is this */
      break;

/*	Here is a big hole in the known meta event space */

    case 0x2f:			/* End of track */
      break;

/*	Here is a big hole in the known meta event space */

    case 0x51:			/* Set tempo (usec per MIDI quarter-note) */
      {
	int tempo_bpm = 120;
	unsigned int usecs_per_qn;

	usecs_per_qn = (evbuf[1] << 16) | (evbuf[2] << 8) | evbuf[3];
	tempo_bpm = (60000000 + (usecs_per_qn / 2)) / usecs_per_qn;

	STORE (SEQ_SET_TEMPO (tempo_bpm));
	return 1;
      }
      break;

    case 0x54:			/* SMPTE offset */
      break;

    case 0x58:			/* Time signature */
      {
	unsigned sig;

	sig = evbuf[1] << 24;
	sig |= evbuf[2] << 16;
	sig |= evbuf[3] << 8;
	sig |= evbuf[4];

	if (!desc->timesig)
	  desc->timesig = sig;
	STORE (SEQ_TIME_SIGNATURE (sig));
      }
      break;

    case 0x59:			/* Key signature */
      break;

    case 0x7f:			/* Vendor specific meta event */
      if (evbuf[1] == 0 && evbuf[2] == 0 && evbuf[3] == 0x41)
	{			/* Microsoft ?? */
	  if (len == 4)
	    DEB_ALWAYS (printf ("GM file flag \n"));

/* Don't forget to add more code here */
	}
      else
	{
	  DEB_ALWAYS (printf ("Private meta event:\n"));
	  for (i = 0; i < len; i++)
	    DEB_ALWAYS (printf ("%02x ", evbuf[i]));
	  DEB_ALWAYS (printf ("\n"));
	  for (i = 0; i < len; i++)
	    DEB_ALWAYS (printf ("%c", (evbuf[i] >= ' ' && evbuf[i] < 127) ?
				evbuf[i] : '.'));
	  DEB_ALWAYS (printf ("\n"));
	}
      break;

    default:
      DEB_ALWAYS (printf ("Meta event: 0x%02x???\n", type));
      for (i = 0; i < len; i++)
	DEB_ALWAYS (printf ("%02x ", evbuf[i]));
      DEB_ALWAYS (printf ("\n"));
      for (i = 0; i < len; i++)
	DEB_ALWAYS (printf ("%c", (evbuf[i] >= ' ' && evbuf[i] < 127) ?
			    evbuf[i] : '.'));
      DEB_ALWAYS (printf ("\n"));
    }

  STORE (META_EVENT (type, evbuf, len));

  return 1;
}

static int
mlib_input_event (mlib_desc * desc, int *iptr, int *optr, mlib_track * track)
{
  int time, len, event_len, i, p;
  unsigned char b, status, type;

  unsigned char buf[MAX_EVENT_SIZE + 1];

  if (!(len = mlib_rdvarlen (desc, &time)))
    return 0;
  *iptr += len;

  track->current_time += time;
  if (track->end_time < track->current_time)
    track->end_time = track->current_time;

  if (!(len = mlib_rdbyte (desc, &type)))
    return 0;
  *iptr += len;

  DEB (printf ("EVENT: time = %d type = 0x%02x: ", time, type));

  p = 0;

  switch (type)
    {
    case 0xff:			/* Meta event */
      if (!(len = mlib_rdbyte (desc, &type)))
	return 0;
      *iptr += len;

      buf[p++] = type;

      if (!(len = mlib_rdvarlen (desc, &event_len)))
	return 0;
      *iptr += len;

      DEB (printf ("meta(type=%02x, len=%d): ", type, event_len));

      if ((event_len + 1) > MAX_EVENT_SIZE)
	{
	  mlib_seterr ("Too long meta event in file");
	  *iptr += event_len;
	  return 0;		/* Fatal error */
	}

      for (i = 0; i < event_len; i++)
	{
	  if (!(len = mlib_rdbyte (desc, &b)))
	    return 0;
	  *iptr += len;

	  buf[p++] = b;
	  DEB (printf ("%02x ", b));
	}

/*
 * End of track test
 */

      if (type == 0x2f)
	{
	  *iptr = 0x7fffffff;
	  return 1;
	}
      if (!mlib_store_meta_event (desc, track, optr, buf, p))
	return 0;
      break;

    case 0xf0:			/* Sysex, variation 1 */
      if (!(len = mlib_rdvarlen (desc, &event_len)))
	return 0;
      *iptr += len;
      DEB (printf ("sysex1 (%d): f0 ", event_len));
      p = 0;
      buf[p++] = 0xf0;
      for (i = 0; i < event_len; i++)
	{
	  if (!(len = mlib_rdbyte (desc, &b)))
	    return 0;
	  *iptr += len;
	  buf[p++] = b;

	  DEB (printf ("%02x ", b));
	}
      {
	int l;

	i = 0;
	while (i < p)
	  {
	    int xx;
	    l = p - i;
	    if (l > 6)
	      l = 6;
	    STORE (SEQ_SYSEX (0, &buf[i], l));
	    i += l;
	  }
      }
      break;

    case 0xf7:			/* Sysex, variation 2 */
      if (!(len = mlib_rdvarlen (desc, &event_len)))
	return 0;
      *iptr += len;
      DEB (printf ("sysex2 (%d): ", event_len));
      for (i = 0; i < event_len; i++)
	{
	  if (!(len = mlib_rdbyte (desc, &b)))
	    return 0;
	  buf[p++] = b;
	  *iptr += len;

	  DEB (printf ("%02x ", b));
	}
      {
	int l;

	i = 0;
	while (i < p)
	  {
	    l = p - i;
	    if (l > 6)
	      l = 6;
	    STORE (SEQ_SYSEX (0, &buf[i], l));
	    i += l;
	  }
      }
      break;

    default:

      if ((type & 0xf0) == 0xf0)	/* Sys message */
	{
	  DEB (printf ("system message "));
	  DEB (printf ("\n"));
	  mlib_seterr ("Unsupported midi file event");
	  return 0;
	}
      else if (type < 0x80)
	{
	  buf[p++] = status = desc->prev_status;
	  DEB (printf ("Midi message (RST): %02x %02x ", status, type));
	  buf[p++] = type;

	  event_len = midi_msglen[((status >> 4) & 0xf) - 8] - 1;
	  for (i = 0; i < event_len; i++)
	    {
	      if (!(len = mlib_rdbyte (desc, &b)))
		return 0;
	      *iptr += len;
	      buf[p++] = b;
	      DEB (printf ("%02x ", b));
	    }
	  if (!mlib_store_chn_voice_msg (desc, track, optr, buf, p))
	    return 0;
	}
      else
	{
	  buf[p++] = status = desc->prev_status = type;
	  DEB (printf ("Midi message: %02x ", status));

	  event_len = midi_msglen[((status >> 4) & 0xf) - 8];
	  for (i = 0; i < event_len; i++)
	    {
	      if (!(len = mlib_rdbyte (desc, &b)))
		return 0;
	      *iptr += len;
	      buf[p++] = b;
	      DEB (printf ("%02x ", b));
	    }
	  if (!mlib_store_chn_voice_msg (desc, track, optr, buf, p))
	    return 0;
	}
    }

  DEB (printf ("\n"));

  return 1;
}

static int
mlib_load_pass (mlib_desc * desc, mlib_track * track, int *iptr, int *optr,
		int len)
{
  int i;

  for (i = 0; i < 128; i++)
    {
      track->pgm_map[i] = -1;
      track->drum_map[i] = -1;
    }

  track->len = 0;
  track->flags = 0;
  track->init_chn = -1;
  track->init_pgm = -1;
  track->port = -1;
  track->chn = -1;
  track->chnmask = 0;
  track->pgm = -1;
  track->current_time = 0;
  track->noteon_time = -1;
  track->end_time = 0;
  track->min_note = 300;
  track->max_note = -1;

  *iptr = *optr = 0;
  desc->prev_status = 0;

  while (*iptr < len)
    if (!mlib_input_event (desc, iptr, optr, track))
      {
	return 0;
      }

  return 1;
}

mlib_track *
mlib_loadtrack (mlib_desc * desc, int *end_detected)
{
  mlib_track *track;
  char sig[5];
  int dummy, len, iptr, optr, pass1events;

  *end_detected = 0;

  if (!mlib_chkdesc (desc))
    return NULL;

  if ((track = (mlib_track *) malloc (sizeof (*track))) == NULL)
    return (mlib_track *) mlib_seterr ("Can't malloc track descriptor");

  desc->curr_trk++;

  if (desc->curr_trk >= desc->hdr.MThd_ntrk)
    {
      *end_detected = 1;
      return (mlib_track *) mlib_seterr ("No more tracks in the midi file");
    }

  desc->trk_offs = desc->next_trk_offs;

  if (!mlib_seek (desc, desc->trk_offs))
    {
      free ((char *) track);
      return NULL;
    }

  if (!mlib_rdstr (desc, sig, 4))
    {
      free ((char *) track);
      return NULL;
    }

  if (strcmp (sig, "MTrk"))
    {
      free ((char *) track);
      return (mlib_track *) mlib_seterr ("Invalid track signature (!= MTrk)");
    }

  if (!mlib_rdint32 (desc, &len))
    {
      free ((char *) track);
      return 0;
    }

  desc->next_trk_offs = desc->trk_offs + 4 + 4 + len;

  track->events = NULL;

  if (!mlib_load_pass (desc, track, &iptr, &optr, len))
    {
      free ((char *) track);
      return NULL;
    }

  pass1events = optr;

/*
 *	Pass 2 actually store the messages
 */

  dummy = pass1events;
  if (dummy < 10)
    dummy = 10;
  track->events = malloc (12 * dummy);
  if (pass1events == 0)
    return track;		/* Empty track */

  if (!mlib_seek (desc, desc->trk_offs + 4))	/* Past the MTrk */
    {
      free (track->events);
      free ((char *) track);
      return NULL;
    }

  if (!mlib_rdint32 (desc, &dummy))
    {
      free (track->events);
      free ((char *) track);
      return 0;
    }

  if (!mlib_load_pass (desc, track, &iptr, &optr, len))
    {
      free (track->events);
      free ((char *) track);
      return NULL;
    }

  track->len = optr;

  if (optr != pass1events)
    fprintf (stderr,
	     "Warning: Events counts of pass 1 and 2 differ (%d/%d)\n",
	     pass1events, optr);

  return track;
}

void
mlib_deltrack (mlib_track * track)
{
  if (track == NULL)
    return;

  if (track->events != NULL)
    free (track->events);

  free ((char *) track);
}

mlib_desc *
mlib_open (char *path)
{
  mlib_desc *desc;
  int i;

  desc = (mlib_desc *) malloc (sizeof (*desc));
  if (desc == NULL)
    return (mlib_desc *) mlib_seterr ("Malloc failed");

  if ((desc->fd = open (path, O_RDONLY, 0)) == -1)
    {
      free ((char *) desc);
      return (mlib_desc *) mlib_syserr (path);
    }

  strncpy (desc->path, path, sizeof (desc->path));

  desc->bufcnt = 0;
  desc->bufp = 1;
  desc->curr_trk = 0;
  desc->trk_offs = 0;
  desc->next_trk_offs = 0;
  desc->magic = 0x121234;
  desc->control_track = NULL;
  desc->timesig = 0;
  for (i = 0; i < MAX_TRACK; i++)
    desc->tracks[i] = NULL;

  if (!mlib_read_mthd (desc))
    {
      close (desc->fd);
      free ((char *) desc);
      return NULL;
    }

  return desc;
}

void
mlib_close (mlib_desc * desc)
{
  if (mlib_chkdesc (desc))
    {
      close (desc->fd);
      desc->fd = -1;
      free (desc);
    }
}

char *
mlib_errmsg (void)
{
  return &mlib_errstring[0];
}
