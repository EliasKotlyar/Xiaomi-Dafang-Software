/*
 * Purpose: Yet another morse code program that uses select
 * Copyright (C) 4Front Technologies, 2002-2004. Released under GPLv2/CDDL.
 *
 * Description:
 * This program is a significantly more complicated version of
 * {!nlink morse.c}. It uses the {!nlink select} system call to be able to
 * handle keyboard input and audio output at the same time. It shows how
 * to prevent output underruns by writing silent samples to the audio device
 * when there is no "payload" signal to play.
 *
 * What the program does is playing randomly selected morse symbols
 * (based on the command line options). At the same time it reads the keyboard
 * and checks if the user typed the right character. This program
 * doesn't stop to wait for the input. This simulates the real morse code
 * exam but ufortunately this dosesn't work in practice.
 *
 * Parts on this program are common with {!nlink morse.c} and commented only
 * there.
 *
 * The {!nlink morse2.c} program is a slightly different version of this one.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <termios.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <soundcard.h>

#define BUFSZ (16*1024)
#define SRATE 48000
#define ATTACK 100
#define CHARDELAY 3

char randomlist[256];
int nrandom = 0;
int done = 0;
int charspeed, charsize;

int dotsize;
int audiofd = -1;
static int ncodes;
static int playc;
static int terminal_fd = 0;
static struct termios ti, saved_ti;
static int totalchars = 0;
static time_t t0;

double a, step;
#include "charlist.h"

int nplayed = 0, ntyped = 0;
char played_chars[4096], typed_chars[4096];

static void
check_results (void)
{
  int i, j, n;
  int errors = 0;

  n = nplayed;

  for (j = 0; j < n; j += 50)
    {
      printf ("Answer: ");
      for (i = j; i < n && i < j + 50; i++)
	{
	  if (i != j && !(i % 5))
	    printf (" ");
	  printf ("%c", typed_chars[i]);
	}
      printf ("\n");

      printf ("Correct:");
      for (i = j; i < n && i < j + 50; i++)
	{
	  if (i != j && !(i % 5))
	    printf (" ");
	  if (typed_chars[i] != played_chars[i])
	    {
	      printf ("%c", played_chars[i]);
	      errors++;
	    }
	  else
	    printf (" ");
	}
      printf ("\n");
    }
  printf ("\n");

  printf ("Errors: %d\n", errors);
}

static void
terminate (int sig)
{
  time_t t;

  t = time (0) - t0;

  if (terminal_fd == -1)
    return;

  if (tcsetattr (terminal_fd, TCSAFLUSH, &saved_ti) == -1)
    {
      perror ("tcgetattr");
      exit (-1);
    }

  printf ("\n\nTotal characters: %d\n", totalchars);
  printf ("Elapsed time: %d seconds\n", t);

  printf ("Characters per minute: %g\n",
	  t > 0?(float) totalchars / (float) t * 60.0:0);
  printf ("\n");
  check_results ();
  exit (sig);
}

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

static void
playerror (void)
{
  short buffer[65536], *buf = buffer;
  int i, l;
  a = 0.0;

  l = dotsize;

  for (i = 0; i < ATTACK; i++)
    {
      double tmp = 0x7fff * cos (a * M_PI / 180.0);
      tmp *= (double) (i) / (double) ATTACK;

      *buf++ = (int) tmp;

      a += step * 2.0;
      if (a > 360.0)
	a -= 360.0;
    }

  for (i = ATTACK; i < l - ATTACK; i++)
    {
      double tmp = 0x7fff * cos (a * M_PI / 180.0);

      *buf++ = (int) tmp;

      a += step * 2.0;
      if (a > 360.0)
	a -= 360.0;
    }

  for (i = l - ATTACK; i < l; i++)
    {
      double tmp = 0x7fff * cos (a * M_PI / 180.0);

      tmp *= (double) (l - i) / (double) ATTACK;

      *buf++ = (int) tmp;

      a += step * 2.0;
      if (a > 360.0)
	a -= 360.0;
    }

  write (audiofd, buffer, 2 * l);
}

static int
genmorse (short *buf, char c)
{
  int l = 0, i;
  const char *s;

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

  printf ("<?>");
  fflush (stdout);
  return 0;
}

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
    case ' ':
    case '\n':
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
      char tmp[512 * 1024];
      l = charsize - 2 * l;
      memset (tmp, 0, l);
      write (audiofd, tmp, l);
    }
}

static void
randomplay (void)
{
  int old = playc;
  if (totalchars == 100)
    {
      return;
    }

//                      while (playc==old)
  {
    playc = random () % nrandom;
    playc = randomlist[playc];
  }

  playchar (playc);
  played_chars[nplayed++] = playc;
  totalchars++;
}

static int
findcode (char c)
{
  int i;

  for (i = 0; i < ncodes; i++)
    if (Chars[i] == c)
      return i;

  return 0;
}

static void
editor (char c)
{
  int i;

  if (c == 27)			/* ESC */
    terminate (SIGINT);

  if (c == 8)			/* BS */
    {
      if (ntyped > 0)
	ntyped--;
      return;
    }

  if (c == ' ')			/* Sync */
    {
      for (i = ntyped; i < nplayed; i++)
	typed_chars[i] = '_';	/* Empty placeholder */
      ntyped = nplayed;
      ioctl (audiofd, SNDCTL_DSP_HALT, 0);
      return;
    }
  typed_chars[ntyped++] = c;

  if (ntyped >= 100)
    done = 1;
}

int
main (int argc, char *argv[])
{
  char *devname = "/dev/dsp";
  short buf[16 * BUFSZ];
  char line[1024];
  int i, parm;
  int l, speed, wpm = 12;

  fd_set readfds, writefds;

  if (argc > 1)
    devname = argv[1];

  if (argc > 2)
    {
      wpm = atoi (argv[2]);
      if (wpm == 0)
	wpm = 12;
    }

  ncodes = strlen (Chars);

  srandom (time (0));

  if (argc > 3)
    {
      for (i = 3; i < argc; i++)
	parse_charlist (argv[i]);
    }
  else
    {
      strcpy (randomlist, Chars);
      nrandom = strlen (randomlist);
    }

  if (nrandom < 2)
    {
      printf ("Bad character list\n");
      exit (-1);
    }

  randomlist[nrandom] = 0;

  memset (typed_chars, ' ', sizeof (typed_chars));

  printf ("Practicing codes: %s\n", randomlist);
  for (i = 0; i <= nrandom; i += 4)
    {
      int j, k;
      char line[256], tmp[20];
      memset (line, ' ', 80), line[78] = 0;

      for (j = 0; j < 4; j++)
	if (i + j <= nrandom)
	  {
	    int ix;

	    ix = findcode (randomlist[i + j]);

	    sprintf (tmp, "%c %s", randomlist[i + j], Codes[ix]);
	    for (k = 0; k < strlen (tmp); k++)
	      line[j * 20 + k] = tmp[k];
	  }

      printf ("%s\n", line);
    }

  speed = wpm;

  printf ("Words per minute %d. Characters per minute %d\n", wpm, wpm * 5);

  dotsize = SRATE / speed;

  if ((audiofd = open (devname, O_WRONLY, 0)) == -1)
    {
      perror (devname);
      exit (-1);
    }

  parm = 0x0004000a;
  ioctl (audiofd, SNDCTL_DSP_SETFRAGMENT, &parm);

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

  if (tcgetattr (terminal_fd, &saved_ti) == -1)
    {
      perror ("tcgetattr");
      exit (-1);
    }

  signal (SIGINT, terminate);
/*
 * Line setup
 */

  if (tcgetattr (terminal_fd, &ti) == -1)
    {
      perror ("tcgetattr");
      exit (-1);
    }


  ti.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  ti.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  ti.c_cflag &= ~(CSIZE | PARENB);
  ti.c_cflag |= CS8;
  ti.c_oflag &= ~(OPOST);

  ti.c_cc[VMIN] = 1;
  ti.c_cc[VTIME] = 1;

  if (tcsetattr (terminal_fd, TCSAFLUSH, &ti) == -1)
    {
      perror ("tcgetattr");
      exit (-1);
    }

  a = 0.0;

  step = 360.0 * 600.0 / parm;

  l = 0;
  l += genpulse (&buf[l], 1, 0);
  write (audiofd, buf, l * 2);
  memset (buf, 0, 4096);
  for (i = 0; i < 2; i++)
    write (audiofd, buf, 4096);

  charspeed = speed * 5;
  if (charspeed > 25)
    charspeed = 25;
  charsize = 60 * SRATE * 2 / charspeed;
  printf ("Charrate %d chars/min -> (%d samples)\n", charspeed, charsize);

  printf ("\r\n");
  randomplay ();

  t0 = time (0);
  while (!done)
    {
      int n;

      FD_ZERO (&readfds);
      FD_ZERO (&writefds);

      FD_SET (audiofd, &writefds);
      FD_SET (0, &readfds);

      if ((n = select (audiofd + 1, &readfds, &writefds, NULL, NULL)) == -1)
	{
	  perror ("select");
	  exit (-1);
	}

      if (n == 0)
	continue;

      if (FD_ISSET (0, &readfds))
	{
	  if (read (0, line, 1) == 1)
	    editor (*line);
	}

      if (FD_ISSET (audiofd, &writefds))
	{
	  if (!(nplayed % 5))
	    playchar (' ');
	  randomplay ();
	}
    }

  terminate (15);

  close (audiofd);

  exit (0);
}
