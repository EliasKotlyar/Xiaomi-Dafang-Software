#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef __STDC__
#include <string.h>
#else /* __STDC__ */
#include <strings.h>
#endif /* __STDC__ */
#include <sys/types.h>

#include <sys/soundcard.h>
#include "str.h"

extern int audio;
extern unsigned char *audiobuf;
extern int abuf_ptr;
extern int abuf_size;

extern char *command;
extern int quiet_mode, verbose_mode;
extern int mute[4];
extern int dsp_stereo;
extern int dsp_samplesize;
extern long dsp_speed;

extern FILE *fp;

static int VSYNC;		/* number of sample bytes to output in 1/50 sec */

static int notes, channel, vsync, bytes, pat_num;	/* loop variables */
static int note, pat;
static int end_pattern;

/* song data and parameters */
static int speed;
static char songlength, songrepeat, tune[128];
static int nvoices;
static Voice voices[32];
static char num_patterns;
static Pattern patterns[128];
static Channel ch[4];

/* table for generating pitches */
static int step_table[1024];

/* sine wave table for the vibrato effect */
static int sin_table[] = {
  0x00, 0x18, 0x31, 0x4A, 0x61, 0x78, 0x8D, 0xA1,
  0xB4, 0xC5, 0xD4, 0xE0, 0xEB, 0xF4, 0xFA, 0xFD,
  0xFF, 0xFD, 0xFA, 0xF4, 0xEB, 0xE0, 0xD4, 0xC5,
  0xB4, 0xA1, 0x8D, 0x78, 0x61, 0x4A, 0x31, 0x18
};

/* period table for notes C0-B2, used for arpeggio effect */
static int periods[] = {
  0x358, 0x328, 0x2FA, 0x2D0, 0x2A6, 0x280,
  0x25C, 0x23A, 0x21A, 0x1FC, 0x1E0, 0x1C5,
  0x1AC, 0x194, 0x17D, 0x168, 0x153, 0x140,
  0x12E, 0x11D, 0x10D, 0x0FE, 0x0F0, 0x0E2,
  0x0D6, 0x0CA, 0x0BE, 0x0B4, 0x0AA, 0x0A0,
  0x097, 0x08F, 0x087, 0x07F, 0x078, 0x071
};

/* skip n bytes of file f - stdin has no fseek */
void
byteskip (f, n)
     FILE *f;
     int n;
{
  while (n--)
    fgetc (f);
}

/* get string of length <len> from file f */
char *
getstring (f, len)
     FILE *f;
     int len;
{
  static char s[256];
  int i;

  for (i = 0; i < len; i++)
    {
      if ((s[i] = fgetc (f)) == '\1')
	s[i] = '\0';		/* bug fix for some few modules */
      if (s[i] < 32 && s[i])
	{
	  fprintf (stderr, "This is not a string. Wrong format?\n");
	  exit (EXIT_FAILURE);
	}
    }
  s[len] = '\0';

  return s;
}

void
audioput (c)
     int c;
{
  audiobuf[abuf_ptr++] = c;
  if (abuf_ptr >= ABUF_SIZE)
    {
#if 0
      int count;

      if (ioctl (audio, SNDCTL_DSP_GETODELAY, &count) == -1)
	perror ("GETODELAY");
      printf ("%6d\n", count);
#endif
      if (write (audio, audiobuf, abuf_ptr) == -1)
	{
	  perror ("audio_write");
	  exit (EXIT_FAILURE);
	}
      abuf_ptr = 0;
    }
}

void
audioput16 (c)
     int c;
{
  audiobuf[abuf_ptr++] = c & 0xFF;
  audiobuf[abuf_ptr++] = c >> 8;
  if ((abuf_ptr + 1) >= ABUF_SIZE)
    {
      if (write (audio, audiobuf, abuf_ptr) == -1)
	{
	  perror ("audio_write");
	  exit (EXIT_FAILURE);
	}
      abuf_ptr = 0;
    }
}

void
play_song ()
{
  int i;

  /* set up pitch table:
   * 3576872 is the base clock frequency of the Amiga's Paula chip,
   * the pitch i is the number of cycles between outputting two samples.
   * => i / 3576872 is the delay between two bytes
   * => our_freq * i / 3576872 is our delay.
   * The 65536 are just there for accuracy of integer operations (<< 16).
   */
  step_table[0] = 0;
  for (i = 1; i < 1024; i++)
    step_table[i] = (int) (((3575872.0 / dsp_speed) / i) * 65536.0);

  /* VSYNC is the number of bytes in 1/50 sec
   * computed as freq * (1 / 50).
   */
  VSYNC = (dsp_speed + 25) / 50;	/* with round */

  /* read song name */
  if (quiet_mode)
    byteskip (fp, 20);
  else
    printf ("Module : %s\n\n", getstring (fp, 20));

  /* determine number of voices from command name */
  nvoices = strstr (command, "15") == NULL ? 31 : 15;

  /* read in the sample information tables */
  voices[0].length = 0;
  for (i = 1; i <= nvoices; i++)
    {
      /* sample name */
      if (quiet_mode)
	byteskip (fp, 22);
      else if (nvoices == 15)
	printf ("%6d : %s\n", i, getstring (fp, 22));
      else
	{
	  printf ("%6d : %22s", i, getstring (fp, 22));
	  if (!(i & 1))
	    printf ("\n");
	}
      /* the sample length and repeat data are stored as numbers of words,
       * we want the number of bytes << 16.
       */
      voices[i].length = (fgetc (fp) << 25) | (fgetc (fp) << 17);
      fgetc (fp);
      if ((voices[i].volume = fgetc (fp)) > 64)
	voices[i].volume = 64;
      voices[i].rep_start = (fgetc (fp) << 25) | (fgetc (fp) << 17);
      voices[i].rep_end = (fgetc (fp) << 25) | (fgetc (fp) << 17);
      if (voices[i].rep_end <= 4 << 16)
	voices[i].rep_end = 0;
      else
	{
	  voices[i].rep_end += voices[i].rep_start;
	  if (voices[i].rep_end > voices[i].length)
	    voices[i].rep_end = voices[i].length;
	}
    }

  /* read sequence data: length and repeat position */
  songlength = fgetc (fp);
  songrepeat = fgetc (fp);
  if (songrepeat > songlength)	/* this is often buggy in modules */
    songrepeat = 0;

  /* read in the sequence and determine the number of patterns
   * by looking for the highest pattern number.
   */
  num_patterns = 0;
  for (i = 0; i < 128; i++)
    {
      tune[i] = fgetc (fp);
      if (tune[i] > num_patterns)
	num_patterns = tune[i];
    }
  num_patterns++;		/* pattern numbers begin at 0 */

  /* skip over sig in new modules (usually M.K.).
   * note: this sig could be used for determining whether the module
   * is of an old type (15 samples max) or new (up to 31 samples)
   */
  if (nvoices == 31)
    byteskip (fp, 4);

  /* read in the patterns.
   * Each pattern consists of 64 lines,
   * each containing data for the four voices.
   */
  for (pat_num = 0; pat_num < num_patterns; pat_num++)
    for (notes = 0; notes < 64; notes++)
      for (channel = 0; channel < 4; channel++)
	{
	  note = (fgetc (fp) << 8) | fgetc (fp);
	  patterns[pat_num].sample[notes][channel] = (note >> 8) & 0x10;
	  patterns[pat_num].period[notes][channel] = MIN (note & 0xFFF, 1023);
	  note = (fgetc (fp) << 8) | fgetc (fp);
	  patterns[pat_num].sample[notes][channel] |= note >> 12;
	  patterns[pat_num].effect[notes][channel] = (note >> 8) & 0xF;
	  patterns[pat_num].params[notes][channel] = note & 0xFF;
	}

  /* store each sample as an array of char */
  for (i = 1; i <= nvoices; i++)
    if (voices[i].length)
      {
	if ((voices[i].info = malloc (voices[i].length >> 16)) == NULL)
	  {
	    fprintf (stderr, "%s: can't allocate memory\n", command);
	    exit (EXIT_FAILURE);
	  }
	fread (voices[i].info, 1, voices[i].length >> 16, fp);
	if (voices[i].rep_end)
	  voices[i].length = voices[i].rep_end;
	voices[i].rep_start -= voices[i].length;
      }

  /* initialize global and channel replay data */
  speed = 6;
  for (i = 0; i < 4; i++)
    {
      ch[i].pointer = 0;
      ch[i].step = 0;
      ch[i].volume = 0;
      ch[i].pitch = 0;
    }

  if (!quiet_mode)
    if (verbose_mode)
      printf ("\n");
    else
      {
	printf ("\nPosition (%d):", songlength);
	fflush (stdout);
      }


	/*******************************/
  /* Here begins the replay part */
	/*******************************/

  for (pat_num = 0; pat_num < songlength; pat_num++)
    {
      pat = tune[pat_num];

      if (!quiet_mode)
	if (verbose_mode)
	  printf ("   --------------+--------------+--------------+-----"
		  "---------  %02X (%02X) = %02X\n", pat_num,
		  songlength, pat);
	else
	  {
	    printf ("\r\t\t%3d", pat_num);
	    fflush (stdout);
	  }

      end_pattern = FALSE;
      for (notes = 0; notes < 64; notes++)
	{
	  int samp, pitch, cmd, para;
	  Channel *curch;

	  if (!quiet_mode && verbose_mode)
	    printf ("%02X ", notes);

	  curch = ch;
	  for (channel = 0; channel < 4; channel++, curch++)
	    {
	      samp = patterns[pat].sample[notes][channel];
	      pitch = patterns[pat].period[notes][channel];
	      cmd = patterns[pat].effect[notes][channel];
	      para = patterns[pat].params[notes][channel];

	      if (verbose_mode && !quiet_mode)
		if (pitch)
		  printf ("  %03X %2X %X%02X%s", pitch, samp, cmd, para,
			  channel == 3 ? "\n" : "  |");
		else
		  printf ("  --- %2X %X%02X%s", samp, cmd, para,
			  channel == 3 ? "\n" : "  |");

	      if (mute[channel])
		{
		  if (cmd == 0x0B || cmd == 0x0D || cmd == 0x0F)
		    switch (cmd)
		      {
		      case 0xB:	/* Pattern jump */
			end_pattern = TRUE;
			pat_num = (para & 0xF) + (10 * (para >> 4));
			break;
		      case 0xD:	/* Pattern break */
			end_pattern = TRUE;
			break;
		      case 0xF:	/* Set speed */
			speed = para;
			break;
		      }
		  continue;
		}

	      /* if a sample number is given, it is loaded and the note
	       * is restarted
	       */
	      if (samp)
		{
		  curch->samp = samp;
		  /* load new instrument */
		  curch->volume = voices[samp].volume;
		}

	      if (samp || (pitch && cmd != 3))
		{
		  if (pitch)
		    curch->pitch = curch->arp[0] = pitch;
		  curch->pointer = 0;
		  curch->viboffs = 0;
		}

	      /* by default there is no effect */
	      curch->doarp = FALSE;
	      curch->doslide = FALSE;
	      curch->doporta = FALSE;
	      curch->dovib = FALSE;
	      curch->doslidevol = FALSE;

	      switch (cmd)
		{		/* Do effects */
		case 0:	/* Arpeggio */
		  if (para)
		    {
		      for (i = 0; i < 36 && periods[i] > curch->arp[0]; i++);
		      if (i + (para >> 4) < 36 && i + (para & 0xF) < 36)
			{
			  curch->doarp = TRUE;
			  curch->arp[0] = periods[i];
			  curch->arp[1] = periods[i + (para >> 4)];
			  curch->arp[2] = periods[i + (para & 0xF)];
			}
		    }
		  break;
		case 1:	/* Portamento up */
		  curch->doslide = TRUE;
		  if (para)
		    curch->slide = -para;
		  break;
		case 2:	/* Portamento down */
		  curch->doslide = TRUE;
		  if (para)
		    curch->slide = para;
		  break;
		case 3:	/* Note portamento */
		  curch->doporta = TRUE;
		  if (para)
		    curch->portarate = para;
		  if (pitch)
		    curch->pitchgoal = pitch;
		  break;
		case 4:	/* Vibrato */
		  curch->dovib = TRUE;
		  if (para)
		    {
		      curch->vibspeed = (para >> 2) & 0x3C;
		      curch->vibamp = para & 0xF;
		    }
		  break;
		case 0xA:	/* Volume slide */
		  curch->doslidevol = TRUE;
		  if (para)
		    {
		      if (!(para & 0xF0))
			curch->volslide = -para;
		      else
			curch->volslide = (para >> 4);
		    }
		  break;
		case 0xB:	/* Pattern jump */
		  end_pattern = TRUE;
		  pat_num = (para & 0xF) + (10 * (para >> 4));
		  break;
		case 0xC:	/* Set volume */
		  curch->volume = MIN (para, 64);
		  break;
		case 0xD:	/* Pattern break */
		  end_pattern = TRUE;
		  break;
		case 0xF:	/* Set speed */
		  speed = para;
		  break;
		default:
		  /* printf(" [%d][%d] ", cmd, para); */
		  break;
		}
	    }

	  {
	    register Channel *curch;
	    register Voice *curv;

	    for (vsync = 0; vsync < speed; vsync++)
	      {

		ch[0].step = step_table[ch[0].pitch];
		ch[1].step = step_table[ch[1].pitch];
		ch[2].step = step_table[ch[2].pitch];
		ch[3].step = step_table[ch[3].pitch];

		for (bytes = 0; bytes < VSYNC; bytes++)
		  {
		    int byte[2] = { 0, 0 };

		    curch = ch;
		    for (channel = 0; channel < 4; channel++, curch++)
		      {
			if (!curch->samp || mute[channel])
			  continue;

			curv = &voices[(int) curch->samp];

			/* test for end of sample */
			if (curch->pointer >= curv->length)
			  if (!curv->rep_end)
			    continue;
			  else
			    curch->pointer += curv->rep_start;

			/* mix samples.
			 * the sample is read and multiplied by the volume
			 */
			if (curch->pointer < curv->length)
			  /* in stereo, channels 1 & 3 and 2 & 4 are mixed
			   * seperately
			   */
			  byte[channel & dsp_stereo] += (int)
			    ((curv->info[curch->pointer >> 16]) *
			     ((int) curch->volume << 2));
			/* advance the sample pointer */
			curch->pointer += curch->step;
		      }

		    /* output sample */
		    if (dsp_samplesize == 8)
		      {
			if (dsp_stereo)
			  {
			    byte[0] >>= 9;
			    audioput (byte[0] + 128);
			    byte[1] >>= 9;
			    audioput (byte[1] + 128);
			  }
			else
			  audioput ((byte[0] >> 10) + 128);
		      }
		    else
		      {
			if (dsp_stereo)
			  {
			    audioput16 (byte[0] >> 1);
			    audioput16 (byte[1] >> 1);
			  }
			else
			  audioput16 (byte[0] >> 2);
		      }
		  }

		/* Do end of vsync */
		if (vsync == 0)
		  continue;
		curch = ch;
		for (channel = 0; channel < 4; channel++, curch++)
		  {
		    if (mute[channel])
		      continue;
		    if (curch->doarp)
		      curch->pitch = curch->arp[vsync % 3];
		    else if (curch->doslide)
		      {
			curch->arp[0] += curch->slide;
			curch->arp[0] = MIN (curch->arp[0], 1023);
			curch->arp[0] = MAX (curch->arp[0], 113);
			curch->pitch = curch->arp[0];
		      }
		    else if (curch->doporta)
		      {
			if (curch->arp[0] < curch->pitchgoal)
			  {
			    curch->arp[0] += curch->portarate;
			    if (curch->arp[0] > curch->pitchgoal)
			      curch->arp[0] = curch->pitchgoal;
			  }
			else if (curch->arp[0] > curch->pitchgoal)
			  {
			    curch->arp[0] -= curch->portarate;
			    if (curch->arp[0] < curch->pitchgoal)
			      curch->arp[0] = curch->pitchgoal;
			  }
			curch->pitch = curch->arp[0];
		      }
		    else if (curch->dovib)
		      {
			if (curch->viboffs & 0x80)
			  curch->pitch = curch->arp[0] - ((sin_table
							   [(curch->
							     viboffs >> 2) &
							    0x1F] *
							   curch->
							   vibamp) >> 6);
			else
			  curch->pitch = curch->arp[0] + ((sin_table
							   [(curch->
							     viboffs >> 2) &
							    0x1F] *
							   curch->
							   vibamp) >> 6);
			curch->viboffs += curch->vibspeed;
		      }
		    else if (curch->doslidevol)
		      {
			curch->volume += curch->volslide;
			if (curch->volume < 0)
			  curch->volume = 0;
			else if (curch->volume >= 64)
			  curch->volume = 64;
		      }
		  }
	      }
	  }
	  if (end_pattern == 1)
	    break;
	}
    }

  if (!quiet_mode)
    printf ("\n");
}
