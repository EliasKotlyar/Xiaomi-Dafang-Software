/*
 * Purpose: Simple audio programming example that plays morse code.
 * Copyright (C) 4Front Technologies, 2002-2004. Released under GPLv2/CDDL.
 *
 * Description:
 * This program reads stdin and plays the input to an audio device using morse
 * code. The stdin input is supposed to be originated from a file. This
 * program is not capable to play live keyboard input.
 *
 * This is a great OSS programming example because it shows how simple
 * audio programming can be with OSS.
 *
 * You can use this program as a template. Just replace the
 * while loop of the main routine by your own code.
 *
 * The {!nlink morse2.c} and {!nlink morse3.c} programs are more complex
 * versions of the same program. They demonstrate how the {!nlink select}
 * system call can be used for serving the audio device in parallel 
 * while handling terminal input.
 *
 * This program was tuned to be used when practising for the 
 * finnish morse code test for radio amateurs. It supports the
 * scandinavian version of the morse code alphabet used in this test.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <math.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>

#define BUFSZ (16*1024)
#define SRATE 48000
#define ATTACK 100
#define CHARDELAY 3

int charspeed = 25;

int dotsize, charsize;
int audiofd = -1;

char randomlist[256];
int nrandom;

static int ncodes;

double a, step;

/*
 * The genpulse() routine converts generates a single dot, dash or
 * a pause between the symbols. this is done by generating sine wave 
 * (using cos()). It's pretty slow but works for us.
 */
#include "charlist.h"

static int
genpulse (short *buf, int w, int state)
{
  int i, l;
  a = 0.0;

  l = w * dotsize;

  for (i = 0; i < ATTACK; i++)
    {
      double tmp = 0x7fff * cos (a * M_PI / 180.0);
      tmp *= (double) (i) / (double) ATTACK;

      *buf++ = (int) tmp *state;

      a += step;
      if (a > 360.0)
	a -= 360.0;
    }

  for (i = ATTACK; i < l - ATTACK; i++)
    {
      double tmp = 0x7fff * cos (a * M_PI / 180.0);

      *buf++ = (int) tmp *state;

      a += step;
      if (a > 360.0)
	a -= 360.0;
    }

  for (i = l - ATTACK; i < l; i++)
    {
      double tmp = 0x7fff * cos (a * M_PI / 180.0);

      tmp *= (double) (l - i) / (double) ATTACK;

      *buf++ = (int) tmp *state;

      a += step;
      if (a > 360.0)
	a -= 360.0;
    }

  return l;
}

/*
 * The genmorse() routine converts an ASCII character to the
 * equivivalent audio morse code signal.
 */
static int
genmorse (short *buf, char c)
{
  int l = 0, i;
  const char *s;

  //printf("%c", c);
  //fflush(stdout);

  if (c == ' ')
    return genpulse (buf, 4, 0);

  for (i = 0; i < ncodes; i++)
    if (Chars[i] == c)
      {
	s = Codes[i];

	while (*s)
	  {
	    if (*s++ == '.')
	      l += genpulse (&buf[l], 1, 1);
	    else
	      l += genpulse (&buf[l], 3, 1);

	    l += genpulse (&buf[l], 1, 0);
	  }

	l += genpulse (&buf[l], CHARDELAY, 0);
	return l;
      }

  return 0;
}

/*
 * The playchar() routine handles some special characters that are not
 * included in the international morse aplhabet. Characters are then played by
 * calling the genmorse() routine.
 */

static void
playchar (char c)
{
  short buf[16 * BUFSZ];
  int l;

  l = 0;

  if (c <= 'Z' && c >= 'A')
    c += 32;

  switch (c)
    {
    case '\n':
      return;
      break;
    case ' ':
    case '\t':
      l = genmorse (buf, ' ');
      break;

    case '\r':
      break;

    case 'Å':
      l = genmorse (buf, 'å');
      break;
    case 'Ä':
      l = genmorse (buf, 'ä');
      break;
    case 'Ö':
      l = genmorse (buf, 'ö');
      break;
    case 'Ü':
      l = genmorse (buf, 'ü');
      break;

    default:
      l = genmorse (buf, c);
    }

  write (audiofd, buf, 2 * l);
  if (2 * l < charsize)
    {
      char tmp[4 * 1024 * 1024];
      l = charsize - 2 * l;
      memset (tmp, 0, l);
      write (audiofd, tmp, l);
    }
}

int
main (int argc, char *argv[])
{
  char *devname = "/dev/dsp";
  short buf[16 * BUFSZ];
  char line[1024];
  int i, parm;
  int l, speed, charspeed, wpm = 8;

/*
 * Charcter rate (CPS/WPS) handling.
 */

  if (argc > 1)
    {
      wpm = atoi (argv[1]);
      if (wpm == 0)
	wpm = 12;
    }

  if (argc > 2)
    charspeed = atoi (argv[2]);

  speed = wpm;
  charsize = 60 * SRATE * 2 / charspeed;

  printf ("Words per minute %d. Characters per minute %d\n", wpm, wpm * 5);
  printf ("Charrate %d chars/min -> (%d samples)\n", charspeed, charsize);
  dotsize = SRATE / speed;

  ncodes = strlen (Chars);

/*
 * Open the audio device and set up the parameters.
 */

  if ((audiofd = open (devname, O_WRONLY, 0)) == -1)
    {
      perror (devname);
      exit (-1);
    }

  parm = AFMT_S16_LE;
  if (ioctl (audiofd, SNDCTL_DSP_SETFMT, &parm) == -1)
    {
      perror ("SETFMT");
      close (audiofd);
      exit (-1);
    }

  if (parm != AFMT_S16_LE)
    {
      printf
	("Error: 32/24 bit sample format is not supported by the device\n");
      printf ("%08x/%08x\n", parm, AFMT_S16_LE);
      close (audiofd);
      exit (-1);
    }

  parm = SRATE;
  if (ioctl (audiofd, SNDCTL_DSP_SPEED, &parm) == -1)
    {
      perror ("SPEED");
      close (audiofd);
      exit (-1);
    }

  if (parm != SRATE)
    {
      printf
	("Error: %d Hz sampling rate is not supported by the device (%d)\n",
	 SRATE, parm);
      close (audiofd);
      exit (-1);
    }

/*
 * The setup phase is complete. After this moment we can forget that we are
 * working on a device. The remainder of this program behaves just like
 * it's writing to any (disk) file.
 */
  a = 0.0;

  step = 360.0 * 600.0 / parm;

  l = 0;
  l += genpulse (&buf[l], 1, 0);
  write (audiofd, buf, l * 2);

  /* Some initial delay */
  memset (buf, 0, 4096);
  for (l = 0; l < 30; l++)
    write (audiofd, buf, 4096);

  while (fgets (line, sizeof (line), stdin) != NULL)
    {

      if (*line == '#')
	continue;

      for (i = 0; i < strlen (line); i++)
	{
	  playchar (line[i]);
	}
    }

  /* Some final delay */
  memset (buf, 0, 4096);
  for (l = 0; l < 20; l++)
    write (audiofd, buf, 4096);

  close (audiofd);

  exit (0);
}
