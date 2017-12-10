/*
   vplay.c - plays and records 
             CREATIVE LABS VOICE-files, Microsoft WAVE-files and raw data

   Autor:    Michael Beck - beck@informatik.hu-berlin.de
*/

#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include "fmtheaders.h"
#include <netinet/in.h>

#define DEFAULT_DSP_SPEED 	8000

#define RECORD	0
#define PLAY	1
#define AUDIO "/dev/dsp"

#define min(a,b) 		((a) <= (b) ? (a) : (b))
#define d_printf(x)        if (verbose_mode) fprintf x

#define VOC_FMT			0
#define WAVE_FMT		1
#define RAW_DATA		2
#define SND_FMT			3

/* global data */

int timelimit = 0, dsp_speed = DEFAULT_DSP_SPEED, dsp_stereo = 0;
int samplesize = 8;
int quiet_mode = 0;
int verbose_mode = 0;
int convert = 0;
int record_type = VOC_FMT;
u_long count;
int audio, abuf_size, zbuf_size;
int direction, omode;
u_char *audiobuf, *zerobuf;
char *command;
int vocminor, vocmajor;		/* .VOC version */

/* defaults for playing raw data */
struct
{
  int timelimit, dsp_speed, dsp_stereo, samplesize;
}
raw_info =
{
0, DEFAULT_DSP_SPEED, 0, 8};

/* needed prototypes */

void record_play (char *name);
void start_voc (int fd, u_long count);
void start_wave (int fd, u_long count);
void start_snd (int fd, u_long count);
void end_voc (int fd);
void end_wave_raw (int fd);
void end_snd (int fd);
u_long calc_count ();

struct fmt_record
{
  void (*start) (int fd, u_long count);
  void (*end) (int fd);
  char *what;
}
fmt_rec_table[] =
{
  {
  start_voc, end_voc, "VOC"}
  ,
  {
  start_wave, end_wave_raw, "WAVE"}
  ,
  {
  NULL, end_wave_raw, "raw data"}
  ,
  {
  start_snd, end_snd, "SND"}
};

int
main (int argc, char *argv[])
{
  int c, n = 0;

  command = argv[0];
  if (strstr (argv[0], "vrec"))
    {
      direction = RECORD;
      omode = O_RDONLY;
    }
  else if (strstr (argv[0], "vplay"))
    {
      direction = PLAY;
      omode = O_WRONLY;
    }
  else
    {
      fprintf (stderr,
	       "Error: command should be named either vrec or vplay\n");
      exit (1);
    }

  while ((c = getopt (argc, argv, "aqs:St:b:vrwd")) != EOF)
    switch (c)
      {
      case 'S':
	dsp_stereo = raw_info.dsp_stereo = 1;
	break;
      case 'q':
	quiet_mode = 1;
	break;
      case 'r':
	record_type = RAW_DATA;
	break;
      case 'v':
	record_type = VOC_FMT;
	break;
      case 'w':
	record_type = WAVE_FMT;
	break;
      case 'a':
	record_type = SND_FMT;
	break;
      case 's':
	dsp_speed = atoi (optarg);
	if (dsp_speed < 300)
	  dsp_speed *= 1000;
	raw_info.dsp_speed = dsp_speed;
	break;
      case 't':
	timelimit = raw_info.timelimit = atoi (optarg);
	break;
      case 'b':
	samplesize = raw_info.samplesize = atoi (optarg);
	break;
      case 'd':
	verbose_mode = 1;
	quiet_mode = 0;
	break;
      default:
	fprintf (stderr,
		 "Usage: %s [-qvwraS] [-t secs] [-s Hz] [-b 8|12|16] [filename]\n",
		 command);
	exit (-1);
      }

  audio = open (AUDIO, omode, 0);
  if (audio == -1)
    {
      perror (AUDIO);
      exit (-1);
    }

  if (optind > argc - 1)
    record_play (NULL);
  else
    while (optind <= argc - 1)
      {

	if (n++ > 0)
	  if (ioctl (audio, SNDCTL_DSP_SYNC, NULL) < 0)
	    {
	      perror (AUDIO);
	      exit (-1);
	    }

	record_play (argv[optind++]);
      }
  close (audio);
  return 0;
}

/*
 test, if it is a .VOC file and return >=0 if ok (this is the length of rest)
                                       < 0 if not 
*/
int
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
int
test_wavefile (void *buffer)
{
  WaveHeader *wp = buffer;
  if (wp->main_chunk == RIFF && wp->chunk_type == WAVE &&
      wp->sub_chunk == FMT && wp->data_chunk == DATA)
    {
      if (wp->format != PCM_CODE)
	{
	  fprintf (stderr, "%s: can't play not PCM-coded WAVE-files\n",
		   command);
	  exit (-1);
	}
      if (wp->modus > 2)
	{
	  fprintf (stderr, "%s: can't play WAVE-files with %d tracks\n",
		   command, wp->modus);
	  exit (-1);
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
int
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
      fprintf (stderr, "%s: Unsupported SND_FORMAT\n", command);
      exit (-1);
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


/*
  writing zeros from the zerobuf to simulate silence,
  perhaps it's enough to use a long var instead of zerobuf ?
*/
void
write_zeros (unsigned x)
{
  unsigned l;

  while (x)
    {
      l = min (x, zbuf_size);
      if (write (audio, (char *) zerobuf, l) != l)
	{
	  perror (AUDIO);
	  exit (-1);
	}
      x -= l;
    }
}

/* if need a SYNC, 
   (is needed if we plan to change speed, stereo ... during output)
*/
void
sync_dsp (void)
{
#if 0
  if (ioctl (audio, SNDCTL_DSP_SYNC, NULL) < 0)
    {
      perror (AUDIO);
      exit (-1);
    }
#endif
}

/* setting the speed for output */
void
set_dsp_speed (int *dsp_speed)
{
  if (ioctl (audio, SNDCTL_DSP_SPEED, dsp_speed) < 0)
    {
      fprintf (stderr, "%s: unable to set audio speed\n", command);
      perror (AUDIO);
      exit (-1);
    }
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
u_long
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
void
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
      fprintf (stderr, "%s: unable to set 8 bit sample size!\n", command);
      exit (-1);
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
		  exit (-1);
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
	      return;		/* VOC-file stop */
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
		      fprintf (stderr, "%s: can't play packed .voc files\n",
			       command);
		      return;
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
				   "%s: can't play in Stereo; playing only one channel\n",
				   command);
			  one_chn = 1;
			}
		    }
		  was_extended = 0;
		}
	      set_dsp_speed (&dsp_speed);
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
	      set_dsp_speed (&dsp_speed);
	      silence = (((u_long) * sp) * 1000) / dsp_speed;
	      d_printf ((stderr, "Silence for %ld ms\n", silence));
	      write_zeros (*sp);
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
			     "%s: can't play loops; %s isn't seekable\n",
			     command, name);
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
		  fprintf (stderr, "%s: can't play packed .voc files\n",
			   command);
		  return;
		}
	      d_printf ((stderr, "Extended block %s %d Hz\n",
			 (eb->mode ? "Stereo" : "Mono"), dsp_speed));
	      break;
	    default:
	      fprintf (stderr, "%s: unknown blocktype %d. terminate.\n",
		       command, bp->type);
	      return;
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
		  exit (-1);
		}
	    }
	  COUNT (l);
	}
    }				/* while(1) */
}

/* that was a big one, perhaps somebody split it :-) */

/* setting the globals for playing raw data */
void
init_raw_data (void)
{
  timelimit = raw_info.timelimit;
  dsp_speed = raw_info.dsp_speed;
  dsp_stereo = raw_info.dsp_stereo;
  samplesize = raw_info.samplesize;
}

/* calculate the data count to read from/to dsp */
u_long
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

/* write a .VOC-header */
void
start_voc (int fd, u_long cnt)
{
  VocHeader vh;
  BlockType bt;
  Voice_data vd;
  Ext_Block eb;

  strcpy ((char *) vh.magic, MAGIC_STRING);
  vh.headerlen = sizeof (VocHeader);
  vh.version = ACTUAL_VERSION;
  vh.coded_ver = 0x1233 - ACTUAL_VERSION;

  write (fd, &vh, sizeof (VocHeader));

  if (dsp_stereo)
    {
      /* write a extended block */
      bt.type = 8;
      bt.datalen = 4;
      bt.datalen_m = bt.datalen_h = 0;
      write (fd, &bt, sizeof (BlockType));
      eb.tc = (u_short) (65536 - 256000000L / (dsp_speed << 1));
      eb.pack = 0;
      eb.mode = 1;
      write (fd, &eb, sizeof (Ext_Block));
    }
  bt.type = 1;
  cnt += sizeof (Voice_data);	/* Voice_data block follows */
  bt.datalen = (u_char) (cnt & 0xFF);
  bt.datalen_m = (u_char) ((cnt & 0xFF00) >> 8);
  bt.datalen_h = (u_char) ((cnt & 0xFF0000) >> 16);
  write (fd, &bt, sizeof (BlockType));
  vd.tc = (u_char) (256 - (1000000 / dsp_speed));
  vd.pack = 0;
  write (fd, &vd, sizeof (Voice_data));
}

/* write a WAVE-header */
void
start_wave (int fd, u_long cnt)
{
  WaveHeader wh;

  wh.main_chunk = RIFF;
  wh.length = cnt + sizeof (WaveHeader) - 8;
  wh.chunk_type = WAVE;
  wh.sub_chunk = FMT;
  wh.sc_len = 16;
  wh.format = PCM_CODE;
  wh.modus = dsp_stereo ? 2 : 1;
  wh.sample_fq = dsp_speed;
  wh.byte_p_spl = ((samplesize == 8) ? 1 : 2) * (dsp_stereo ? 2 : 1);
  wh.byte_p_sec = dsp_speed * wh.modus * wh.byte_p_spl;
  wh.bit_p_spl = samplesize;
  wh.data_chunk = DATA;
  wh.data_length = cnt;
  write (fd, &wh, sizeof (WaveHeader));
}

/* closing .VOC */
void
end_voc (int fd)
{
  char dummy = 0;		/* Write a Terminator */
  write (fd, &dummy, 1);
  if (fd != 1)
    close (fd);
}

void
end_wave_raw (int fd)
{				/* only close output */
  if (fd != 1)
    close (fd);
}

void
start_snd (int fd, u_long count)
{
  SndHeader snd;
  char *sndinfo = "Recorded by vrec\000";

  snd.magic = SND_MAGIC;
  snd.dataLocation = sizeof (SndHeader) + strlen (sndinfo);
  snd.dataSize = count;
  switch (samplesize)
    {
    case 8:
      snd.dataFormat = SND_FORMAT_LINEAR_8;
      break;
    case 16:
      snd.dataFormat = SND_FORMAT_LINEAR_16;
      break;
    default:
      fprintf (stderr,
	       "%d bit: unsupported sample size for NeXt sound file!\n",
	       samplesize);
      exit (-1);
    }
  snd.samplingRate = dsp_speed;
  snd.channelCount = dsp_stereo ? 2 : 1;

  write (fd, &snd, sizeof (SndHeader));
  write (fd, sndinfo, strlen (sndinfo));
}

void
end_snd (int fd)
{
  if (fd != 1)
    close (fd);
}

/* playing/recording raw data, this proc handels WAVE files and
   recording .VOCs (as one block) */
void
recplay (int fd, int loaded, u_long count, int rtype, char *name)
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
      fprintf (stderr, "%s: unable to set %d bit sample size",
	       command, samplesize);
      if (samplesize == 16)
	{
	  samplesize = 8;
	  ioctl (audio, SNDCTL_DSP_SETFMT, &samplesize);
	  if (samplesize != 8)
	    {
	      fprintf (stderr, "%s: unable to set 8 bit sample size!\n",
		       command);
	      exit (-1);
	    }
	  fprintf (stderr, "; playing 8 bit\n");
	  to_8 = 1;
	}
      else
	{
	  fprintf (stderr, "\n");
	  exit (-1);
	}
    }
#ifdef OSS_VERSION
  if (ioctl (audio, SNDCTL_DSP_STEREO, &dsp_stereo) < 0)
    {
#else
  if (dsp_stereo != ioctl (audio, SNDCTL_DSP_STEREO, dsp_stereo))
    {
#endif
      if (direction == PLAY)
	{
	  fprintf (stderr,
		   "%s: can't play in Stereo; playing only one channel\n",
		   command);
	  dsp_stereo = MODE_MONO;
	  one_chn = 1;
	}
      else
	{
	  fprintf (stderr, "%s: can't record in Stereo\n", command);
	  exit (-1);
	}
    }
  set_dsp_speed (&dsp_speed);
  if (!quiet_mode)
    {
      fprintf (stderr, "%s %s : ",
	       (direction == PLAY) ? "Playing" : "Recording",
	       fmt_rec_table[rtype].what);
      if (samplesize != 8)
	fprintf (stderr, "%d bit, ", samplesize);
      fprintf (stderr, "Speed %d Hz ", dsp_speed);
      fprintf (stderr, "%d bits ", samplesize);
      fprintf (stderr, "%s ...\n", dsp_stereo ? "Stereo" : "Mono");
    }

  abuf_size = -1;
#if 0
/*
 * There is no idea in using SNDCTL_DSP_GETBLKSIZE in applications like this.
 * Using some fixed local buffer size will work equally well.
 */
  ioctl (audio, SNDCTL_DSP_GETBLKSIZE, &abuf_size);
  if (abuf_size == -1)
    {
      perror (AUDIO);
      exit (-1);
    }
#endif

  zbuf_size = 256;
  if ((audiobuf = (u_char *) malloc (abuf_size)) == NULL ||
      (zerobuf = (u_char *) malloc (zbuf_size)) == NULL)
    {
      fprintf (stderr, "%s: unable to allocate input/output buffer\n",
	       command);
      exit (-1);
    }
  memset ((char *) zerobuf, 128, zbuf_size);

  if (direction == PLAY)
    {
      while (count)
	{
	  c = count;

	  if (c > abuf_size)
	    c = abuf_size;

	  if ((l = read (fd, (char *) audiobuf + loaded, c - loaded)) > 0)
	    {
	      l += loaded;
	      loaded = 0;	/* correct the count; ugly but ... */
	      real_l = (one_chn
			|| to_8) ? one_channel (audiobuf, l, one_chn,
						to_8) : l;

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
		  exit (-1);
		}
	      count -= l;
	    }
	  else
	    {
	      if (l == -1)
		{
		  perror (name);
		  exit (-1);
		}
	      count = 0;	/* Stop */
	    }
	}			/* while (count) */
    }
  else
    {				/* we are recording */

      while (count)
	{
	  c = count;
	  if (c > abuf_size)
	    c = abuf_size;

	  if ((l = read (audio, (char *) audiobuf, c)) > 0)
	    {
	      if (write (fd, (char *) audiobuf, l) != l)
		{
		  perror (name);
		  exit (-1);
		}
	      count -= l;
	    }

	  if (l == -1)
	    {
	      perror (AUDIO);
	      exit (-1);
	    }
	}			/* While count */
    }
}

/*
  let's play or record it (record_type says VOC/WAVE/raw)
*/
void
record_play (char *name)
{
  int fd, ofs;

  if (direction == PLAY)
    {

      if (!name)
	{
	  fd = 0;
	  name = "stdin";
	}
      else if ((fd = open (name, O_RDONLY, 0)) == -1)
	{
	  perror (name);
	  exit (-1);
	}
      /* Read the smallest header first, then the
         missing bytes for the next, etc. */

      /* read SND-header */
      read (fd, (char *) audiobuf, sizeof (SndHeader));
      if (test_sndfile (audiobuf, fd) >= 0)
	recplay (fd, 0, count, SND_FMT, name);

      else
	{
	  /* read VOC-Header */
	  read (fd, (char *) audiobuf + sizeof (SndHeader),
		sizeof (VocHeader) - sizeof (SndHeader));
	  if ((ofs = test_vocfile (audiobuf)) >= 0)
	    vplay (fd, ofs, name);

	  else
	    {
	      /* read bytes for WAVE-header */
	      read (fd, (char *) audiobuf + sizeof (VocHeader),
		    sizeof (WaveHeader) - sizeof (VocHeader));
	      if (test_wavefile (audiobuf) >= 0)
		recplay (fd, 0, count, WAVE_FMT, name);
	      else
		{
		  /* should be raw data */
		  init_raw_data ();
		  count = calc_count ();
		  recplay (fd, sizeof (WaveHeader), count, RAW_DATA, name);
		}
	    }
	}
      if (fd != 0)
	close (fd);
    }
  else
    {				/* recording ... */
      if (!name)
	{
	  fd = 1;
	  name = "stdout";
	}
      else
	{
	  if ((fd = open (name, O_WRONLY | O_CREAT | O_TRUNC, 0666)) == -1)
	    {
	      perror (name);
	      exit (-1);
	    }
	}
      count = calc_count () & 0xFFFFFFFE;
      /* WAVE-file should be even (I'm not sure), but wasting one byte
         isn't a problem (this can only be in 8 bit mono) */
      if (fmt_rec_table[record_type].start)
	fmt_rec_table[record_type].start (fd, count);
      recplay (fd, 0, count, record_type, name);
      fmt_rec_table[record_type].end (fd);
    }
}
