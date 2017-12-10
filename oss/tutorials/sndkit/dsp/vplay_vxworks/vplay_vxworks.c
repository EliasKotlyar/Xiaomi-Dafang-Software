/*
   vplay.c - plays
             CREATIVE LABS VOICE-files, Microsoft WAVE-files and raw data

   Autor:    Michael Beck - beck@informatik.hu-berlin.de

   Modified for VxWorks by 4Front Technologies

   Usage: int play_wave (char *filename);
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <ioLib.h>
#include <sys/ioctl.h>
#include "soundcard.h"		/* Edit this to contain the right location of soundcard.h */
#include "fmtheaders.h"

/*
 * The following defines break support for .snd files. Implement these routines
 * properly if you need to play them.
 */
#define htonl(x) x
#define htons(x) x

#define DEFAULT_DSP_SPEED 	8000

#define AUDIO "/oss/dsp"

#ifndef min
#define min(a,b) 		((a) <= (b) ? (a) : (b))
#endif
#define d_printf(x)        if (verbose_mode) fprintf x

#define VOC_FMT			0
#define WAVE_FMT		1
#define RAW_DATA		2
#define SND_FMT			3

/* global data */

static int timelimit = 0, dsp_speed = DEFAULT_DSP_SPEED, dsp_stereo = 0;
static int samplesize = 8;
static int quiet_mode = 0;
static int verbose_mode = 0;
static int convert = 0;
static u_long count;
static int audio, abuf_size;
static int omode;
static u_char audiobuf[512];
static int vocminor, vocmajor;	/* .VOC version */

/* defaults for playing raw data */
static struct
{
  int timelimit, dsp_speed, dsp_stereo, samplesize;
}
raw_info =
{
0, DEFAULT_DSP_SPEED, 0, 8};

/* needed prototypes */

static int player (char *name);
static u_long calc_count ();

int
play_wave (char *filename)
{
  omode = O_WRONLY;

  quiet_mode = 1;

  audio = open (AUDIO, omode, 0);
  if (audio == -1)
    {
      perror (AUDIO);
      return -1;
    }

  if (player (filename) < 0)
    return -1;
  close (audio);
  return 0;
}

/*
 test, if it is a .VOC file and return >=0 if ok (this is the length of rest)
                                       < 0 if not 
*/
static int
test_vocfile (void *buffer)
{
  VocHeader *vp = buffer;
  if (!strcmp ((const char *) vp->magic, MAGIC_STRING))
    {
      vocminor = vp->version & 0xFF;
      vocmajor = vp->version / 256;
      if (vp->version != (0x1233 - vp->coded_ver))
	return -2;		/* coded version mismatch */
      return vp->headerlen - sizeof (VocHeader);	/* 0 mostly */
    }
  return -1;			/* magic string fail */
}

/*
 test, if it's a .WAV file, 0 if ok (and set the speed, stereo etc.)
                          < 0 if not
*/
static int
test_wavefile (void *buffer)
{
  WaveHeader *wp = buffer;
  if (wp->main_chunk == RIFF && wp->chunk_type == WAVE &&
      wp->sub_chunk == FMT && wp->data_chunk == DATA)
    {
      if (wp->format != PCM_CODE)
	{
	  fprintf (stderr, "Can't play non PCM-coded WAVE-files\n");
	  return -1;
	}
      if (wp->modus > 2)
	{
	  fprintf (stderr, "Can't play WAVE-files with %d tracks\n",
		   wp->modus);
	  return -1;
	}
      dsp_stereo = (wp->modus == WAVE_STEREO) ? 1 : 0;
      samplesize = wp->bit_p_spl;
      dsp_speed = wp->sample_fq;
      count = wp->data_length;
      return 0;
    }
  return -1;
}

/*
* test, if it's a .SND file, 0 if ok (and set the speed, stereo etc.)
*                          < 0 if not
*/
static int
test_sndfile (void *buffer, int fd)
{
  long infolen;
  char *info;
  SndHeader *snd = buffer;

  if (snd->magic == SND_MAGIC)
    convert = 0;
  else
    {
      if (htonl (snd->magic) == SND_MAGIC)
	{
	  convert = 1;
	  snd->dataLocation = htonl (snd->dataLocation);
	  snd->dataSize = htonl (snd->dataSize);
	  snd->dataFormat = htonl (snd->dataFormat);
	  snd->samplingRate = htonl (snd->samplingRate);
	  snd->channelCount = htonl (snd->channelCount);
	}
      else
	{
	  /* No SoundFile */
	  return (-1);
	}
    }
  switch (snd->dataFormat)
    {
    case SND_FORMAT_LINEAR_8:
      samplesize = 8;
      break;
    case SND_FORMAT_LINEAR_16:
      samplesize = 16;
      break;
    default:
      fprintf (stderr, "Unsupported SND_FORMAT\n");
      return -1;
    }

  dsp_stereo = (snd->channelCount == 2) ? 1 : 0;
  dsp_speed = snd->samplingRate;
  count = snd->dataSize;

  /* read Info-Strings */
  infolen = snd->dataLocation - sizeof (SndHeader);
  info = (char *) malloc (infolen);
  read (fd, info, infolen);
  if (!quiet_mode)
    fprintf (stderr, "SoundFile Info: %s\n", info);
  free (info);

  return 0;
}


/* if need a SYNC, 
   (is needed if we plan to change speed, stereo ... during output)
*/
static void
sync_dsp (void)
{
}

/* setting the speed for output */
static int
set_dsp_speed (int *dsp_speed)
{
  if (ioctl (audio, SNDCTL_DSP_SPEED, dsp_speed) < 0)
    {
      fprintf (stderr, "Unable to set audio speed\n");
      perror (AUDIO);
      return -1;
    }
  return 0;
}

/* if to_mono: 
   compress 8 bit stereo data 2:1, needed if we want play 'one track';
   this is usefull, if you habe SB 1.0 - 2.0 (I have 1.0) and want
   hear the sample (in Mono)
   if to_8:
   compress 16 bit to 8 by using hi-byte; wave-files use signed words,
   so we need to convert it in "unsigned" sample (0x80 is now zero)

   WARNING: this procedure can't compress 16 bit stereo to 16 bit mono,
            because if you have a 16 (or 12) bit card you should have
            stereo (or I'm wrong ?? )
   */
static u_long
one_channel (u_char * buf, u_long l, char to_mono, char to_8)
{
  register u_char *w = buf;
  register u_char *w2 = buf;
  char ofs = 0;
  u_long incr = 0;
  u_long c, ret;

  if (to_mono)
    ++incr;
  if (to_8)
    {
      ++incr;
      ++w2;
      ofs = 128;
    }
  ret = c = l >> incr;
  incr = incr << 1;

  while (c--)
    {
      *w++ = *w2 + ofs;
      w2 += incr;
    }
  return ret;
}

/*
  ok, let's play a .voc file
*/
static int
vplay (int fd, int ofs, char *name)
{
  int l, real_l;
  BlockType *bp;
  Voice_data *vd;
  Ext_Block *eb;
  u_long nextblock, in_buffer;
  u_char *data = audiobuf;
  char was_extended = 0, output = 0;
  u_short *sp, repeat = 0;
  u_long silence;
  int filepos = 0;
  char one_chn = 0;

#define COUNT(x)	nextblock -= x; in_buffer -=x ;data += x

  /* first SYNC the dsp */
  sync_dsp ();

  if (!quiet_mode)
    fprintf (stderr, "Playing Creative Labs Voice file ...\n");

  /* first we waste the rest of header, ugly but we don't need seek */
  while (ofs > abuf_size)
    {
      read (fd, (char *) audiobuf, abuf_size);
      ofs -= abuf_size;
    }
  if (ofs)
    read (fd, audiobuf, ofs);

  /* .voc files are 8 bit (now) */
  samplesize = VOC_SAMPLESIZE;
  ioctl (audio, SNDCTL_DSP_SETFMT, &samplesize);
  if (samplesize != VOC_SAMPLESIZE)
    {
      fprintf (stderr, "Unable to set 8 bit sample size!\n");
      return -1;
    }
  /* and there are MONO by default */
  dsp_stereo = MODE_MONO;
  ioctl (audio, SNDCTL_DSP_STEREO, &dsp_stereo);

  in_buffer = nextblock = 0;
  while (1)
    {
    Fill_the_buffer:		/* need this for repeat */
      if (in_buffer < 32)
	{
	  /* move the rest of buffer to pos 0 and fill the audiobuf up */
	  if (in_buffer)
	    memcpy ((char *) audiobuf, data, in_buffer);
	  data = audiobuf;
	  if ((l =
	       read (fd, (char *) audiobuf + in_buffer,
		     abuf_size - in_buffer)) > 0)
	    in_buffer += l;
	  else if (!in_buffer)
	    {
	      /* the file is truncated, so simulate 'Terminator' 
	         and reduce the datablock for save landing */
	      nextblock = audiobuf[0] = 0;
	      if (l == -1)
		{
		  perror (name);
		  return -1;
		}
	    }
	}
      while (!nextblock)
	{			/* this is a new block */
	  bp = (BlockType *) data;
	  COUNT (sizeof (BlockType));
	  nextblock = DATALEN (bp);
	  if (output && !quiet_mode)
	    fprintf (stderr, "\n");	/* write /n after ASCII-out */
	  output = 0;
	  switch (bp->type)
	    {
	    case 0:
	      d_printf ((stderr, "Terminator\n"));
	      return -1;	/* VOC-file stop */
	    case 1:
	      vd = (Voice_data *) data;
	      COUNT (sizeof (Voice_data));
	      /* we need a SYNC, before we can set new SPEED, STEREO ... */
	      sync_dsp ();

	      if (!was_extended)
		{
		  dsp_speed = (int) (vd->tc);
		  dsp_speed = 1000000 / (256 - dsp_speed);
		  d_printf ((stderr, "Voice data %d Hz\n", dsp_speed));
		  if (vd->pack)
		    {		/* /dev/dsp can't it */
		      fprintf (stderr, "Can't play packed .voc files\n");
		      return -1;
		    }
		  if (dsp_stereo)
		    {		/* if we are in Stereo-Mode, switch back */
		      dsp_stereo = MODE_MONO;
		      ioctl (audio, SNDCTL_DSP_STEREO, &dsp_stereo);
		    }
		}
	      else
		{		/* there was extended block */
		  if (one_chn)	/* if one Stereo fails, why test another ? */
		    dsp_stereo = MODE_MONO;
		  else if (dsp_stereo)
		    {		/* want Stereo */
		      /* shit, my MACRO dosn't work here */
#ifdef OSS_VERSION
		      if (ioctl (audio, SNDCTL_DSP_STEREO, &dsp_stereo) < 0)
			{
#else
		      if (dsp_stereo !=
			  ioctl (audio, SNDCTL_DSP_STEREO, dsp_stereo))
			{
#endif
			  dsp_stereo = MODE_MONO;
			  fprintf (stderr,
				   "Can't play in Stereo; playing only one channel\n");
			  one_chn = 1;
			}
		    }
		  was_extended = 0;
		}
	      if (set_dsp_speed (&dsp_speed) < 0)
		return -1;
	      break;
	    case 2:		/* nothing to do, pure data */
	      d_printf ((stderr, "Voice continuation\n"));
	      break;
	    case 3:		/* a silence block, no data, only a count */
	      sp = (u_short *) data;
	      COUNT (sizeof (u_short));
	      dsp_speed = (int) (*data);
	      COUNT (1);
	      dsp_speed = 1000000 / (256 - dsp_speed);
	      sync_dsp ();
	      if (set_dsp_speed (&dsp_speed) < 0)
		return -1;
	      silence = (((u_long) * sp) * 1000) / dsp_speed;
	      d_printf ((stderr, "Silence for %ld ms\n", silence));
	      break;
	    case 4:		/* a marker for syncronisation, no effect */
	      sp = (u_short *) data;
	      COUNT (sizeof (u_short));
	      d_printf ((stderr, "Marker %d\n", *sp));
	      break;
	    case 5:		/* ASCII text, we copy to stderr */
	      output = 1;
	      d_printf ((stderr, "ASCII - text :\n"));
	      break;
	    case 6:		/* repeat marker, says repeatcount */
	      /* my specs don't say it: maybe this can be recursive, but
	         I don't think somebody use it */
	      repeat = *(u_short *) data;
	      COUNT (sizeof (u_short));
	      d_printf ((stderr, "Repeat loop %d times\n", repeat));
	      if (filepos >= 0)	/* if < 0, one seek fails, why test another */
		if ((filepos = lseek (fd, 0, 1)) < 0)
		  {
		    fprintf (stderr,
			     "Can't play loops; %s isn't seekable\n", name);
		    repeat = 0;
		  }
		else
		  filepos -= in_buffer;	/* set filepos after repeat */
	      else
		repeat = 0;
	      break;
	    case 7:		/* ok, lets repeat that be rewinding tape */
	      if (repeat)
		{
		  if (repeat != 0xFFFF)
		    {
		      d_printf ((stderr, "Repeat loop %d\n", repeat));
		      --repeat;
		    }
		  else
		    d_printf ((stderr, "Neverending loop\n"));
		  lseek (fd, filepos, 0);
		  in_buffer = 0;	/* clear the buffer */
		  goto Fill_the_buffer;
		}
	      else
		d_printf ((stderr, "End repeat loop\n"));
	      break;
	    case 8:		/* the extension to play Stereo, I have SB 1.0 :-( */
	      was_extended = 1;
	      eb = (Ext_Block *) data;
	      COUNT (sizeof (Ext_Block));
	      dsp_speed = (int) (eb->tc);
	      dsp_speed = 256000000L / (65536 - dsp_speed);
	      dsp_stereo = eb->mode;
	      if (dsp_stereo == MODE_STEREO)
		dsp_speed = dsp_speed >> 1;
	      if (eb->pack)
		{		/* /dev/dsp can't it */
		  fprintf (stderr, "Can't play packed .voc files\n");
		  return -1;
		}
	      d_printf ((stderr, "Extended block %s %d Hz\n",
			 (eb->mode ? "Stereo" : "Mono"), dsp_speed));
	      break;
	    default:
	      fprintf (stderr, "Unknown blocktype %d. terminate.\n",
		       bp->type);
	      return -1;
	    }			/* switch (bp->type) */
	}			/* while (! nextblock)  */
      /* put nextblock data bytes to dsp */
      l = min (in_buffer, nextblock);
      if (l)
	{
	  if (output && !quiet_mode)
	    write (2, data, l);	/* to stderr */
	  else
	    {
	      real_l = one_chn ? one_channel (data, l, one_chn, 0) : l;
	      if (write (audio, data, real_l) != real_l)
		{
		  perror (AUDIO);
		  return -1;
		}
	    }
	  COUNT (l);
	}
    }				/* while(1) */
}

/* that was a big one, perhaps somebody split it :-) */

/* setting the globals for playing raw data */
static void
init_raw_data (void)
{
  timelimit = raw_info.timelimit;
  dsp_speed = raw_info.dsp_speed;
  dsp_stereo = raw_info.dsp_stereo;
  samplesize = raw_info.samplesize;
}

/* calculate the data count to read from/to dsp */
static u_long
calc_count (void)
{
  u_long count;

  if (!timelimit)
    count = 0x7fffffff;
  else
    {
      count = timelimit * dsp_speed;
      if (dsp_stereo)
	count *= 2;
      if (samplesize != 8)
	count *= 2;
    }
  return count;
}

/* playing raw data, this proc handels WAVE files and
   .VOCs (as one block) */
static int
do_play (int fd, int loaded, u_long count, int rtype, char *name)
{
  int l, real_l;
  u_long c;
  char one_chn = 0;
  char to_8 = 0;
  int tmps;

  sync_dsp ();
  tmps = samplesize;
  ioctl (audio, SNDCTL_DSP_SETFMT, &tmps);
  if (tmps != samplesize)
    {
      fprintf (stderr, "Unable to set %d bit sample size", samplesize);
      if (samplesize == 16)
	{
	  samplesize = 8;
	  ioctl (audio, SNDCTL_DSP_SETFMT, &samplesize);
	  if (samplesize != 8)
	    {
	      fprintf (stderr, "Unable to set 8 bit sample size!\n");
	      return -1;
	    }
	  fprintf (stderr, "; playing 8 bit\n");
	  to_8 = 1;
	}
      else
	{
	  fprintf (stderr, "\n");
	  return -1;
	}
    }
#ifdef OSS_VERSION
  if (ioctl (audio, SNDCTL_DSP_STEREO, &dsp_stereo) < 0)
    {
#else
  if (dsp_stereo != ioctl (audio, SNDCTL_DSP_STEREO, dsp_stereo))
    {
#endif
      fprintf (stderr, "Can't play in Stereo; playing only one channel\n");
      dsp_stereo = MODE_MONO;
      one_chn = 1;
    }
  if (set_dsp_speed (&dsp_speed) < 0)
    return -1;

  abuf_size = 512;

  while (count)
    {
      c = count;

      if (c > abuf_size)
	c = abuf_size;

      if ((l = read (fd, (char *) audiobuf + loaded, c - loaded)) > 0)
	{
	  l += loaded;
	  loaded = 0;		/* correct the count; ugly but ... */
	  real_l = (one_chn
		    || to_8) ? one_channel (audiobuf, l, one_chn, to_8) : l;

	  /* change byte order if necessary */
	  if (convert && (samplesize == 16))
	    {
	      long i;

	      for (i = 0; i < real_l; i += 2)
		*((short *) (audiobuf + i)) =
		  htons (*((short *) (audiobuf + i)));
	    }

	  if (write (audio, (char *) audiobuf, real_l) != real_l)
	    {
	      perror (AUDIO);
	      return -1;
	    }
	  count -= l;
	}
      else
	{
	  if (l == -1)
	    {
	      perror (name);
	      return -1;
	    }
	  count = 0;		/* Stop */
	}
    }				/* while (count) */

  return 0;
}

/*
  let's play)
*/
static int
player (char *name)
{
  int fd, ofs;

  if (!name)
    {
      fd = 0;
      name = "stdin";
    }
  else if ((fd = open (name, O_RDONLY, 0)) == -1)
    {
      perror (name);
      return -1;
    }
  /* Read the smallest header first, then the
     missing bytes for the next, etc. */

  /* read SND-header */
  read (fd, (char *) audiobuf, sizeof (SndHeader));
  if (test_sndfile (audiobuf, fd) >= 0)
    {
      if (do_play (fd, 0, count, SND_FMT, name) < 0)
	return -1;
    }

  else
    {
      /* read VOC-Header */
      read (fd, (char *) audiobuf + sizeof (SndHeader),
	    sizeof (VocHeader) - sizeof (SndHeader));
      if ((ofs = test_vocfile (audiobuf)) >= 0)
	{
	  if (vplay (fd, ofs, name) < 0)
	    return -1;
	}

      else
	{
	  /* read bytes for WAVE-header */
	  read (fd, (char *) audiobuf + sizeof (VocHeader),
		sizeof (WaveHeader) - sizeof (VocHeader));
	  if (test_wavefile (audiobuf) >= 0)
	    {
	      if (do_play (fd, 0, count, WAVE_FMT, name) < 0)
		return -1;
	    }
	  else
	    {
	      /* should be raw data */
	      init_raw_data ();
	      count = calc_count ();
	      if (do_play (fd, sizeof (WaveHeader), count, RAW_DATA, name) <
		  0)
		return -1;
	    }
	}
    }
  if (fd != 0)
    close (fd);
  return 0;
}

#if 0
int
main (int agrc, char *argv[])
{
  exit (play_wave (argv[1]));
}
#endif
