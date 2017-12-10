/*
 * Purpose: Another morse code program that uses select
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
 * (based on the command line options). It then waits until the
 * users hits a key. If the input character matches the morse output then
 * the program continues by playing another morse character. If there was an
 * error then the next character is repeated. The Esc key is used to stop
 * the program.
 *
 * Parts on this program are common with {!nlink morse.c} and commented only
 * there.
 *
 * The {!nlink morse3.c} program is a slightly different version of this one.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <math.h>
#include <termios.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>

#define BUFSZ (16*1024)
#define SRATE 48000
#define ATTACK 100
#define CHARDELAY 3

char randomlist[65536];
int nrandom = 0;
int done = 0;
int chars_to_play = 100;

int dotsize;
int audiofd = -1;
static int ncodes;
static int playc;
static int terminal_fd = 0;
static struct termios ti, saved_ti;
static int totalchars = 0, totalerrors = 0, errors = 0;
static time_t t0;

double a, step;
#include "charlist.h"

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
  printf ("Errors: %d\n", totalerrors);
  printf ("Elapsed time: %d seconds\n", t);
  if (errors == 0) totalchars--;
  if (totalchars <= totalerrors) totalchars = 0;
  else
    {
      totalchars -= totalerrors;
      totalchars += totalchars / 5;
    }

  printf ("Characters per minute: %g\n",
	  t > 0?(float) totalchars / (float) t * 60.0:0);
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

/*
 * Next write whatever we have in the buffer. Note that this write will
 * block but that time is at most the time required to play one morse 
 * code character. There is no need to avoid blocking because it doesn't cause
 * any annoying delays.
 *
 * This is often the case with audio application. While many programmers think
 * blocking is evil it's actually programmer's best friend.
 */
  if (write (audiofd, buf, 2 * l) != 2 * l)
    {
      perror ("write audio");
      terminate (15);
    }
}

/*
 * The randomplay() routine picks a character from the list of characters
 * selected for practice.
 */
static void
randomplay (void)
{
  int old = playc;
  if (totalchars == chars_to_play)
    {
      done = 1;
      return;
    }

//                      while (playc==old)
  {
    int i, x, tmp;

    x = random () % nrandom;
    playc = randomlist[x];
#if 1
    if (x != nrandom - 1)
      {
	tmp = randomlist[x];
	for (i = x; i < nrandom - 1; i++)
	  randomlist[i] = randomlist[i + 1];
	randomlist[nrandom - 1] = tmp;
      }
#endif
  }

  playchar (playc);
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

int
main (int argc, char *argv[])
{
  char *devname = "/dev/dsp";
  short buf[32 * BUFSZ];
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

  parm = 0x0003000a;
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
 * Set up the terminal (stdin) for single character input.
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

/*
 * Play some "extra" audio data to delay the startup. This just gives
 * some extra time to the user to prepare before we start.
 */

  step = 360.0 * 600.0 / parm;

  l = 0;
  l += genpulse (&buf[l], 1, 0);
  write (audiofd, buf, l * 2);
  memset (buf, 0, 4096);
  for (i = 0; i < 2; i++)
    write (audiofd, buf, 4096);

/*
 * The actual playback starts here 
 */

  randomplay ();

  t0 = time (0);
  while (!done)
    {
      int n;

/*
 * Set up select for output events on the audio device and input events on
 * the keyboard.
 */
      FD_ZERO (&readfds);
      FD_ZERO (&writefds);

      FD_SET (audiofd, &writefds);
      FD_SET (0, &readfds);

/*
 * Call select with no timeouts
 */
      if ((n = select (audiofd + 1, &readfds, &writefds, NULL, NULL)) == -1)
	{
	  perror ("select");
	  exit (-1);
	}

      if (n == 0)
	continue;

      if (FD_ISSET (0, &readfds))	/* 0 means stdin */
	{
/*
 * Handling of keyboard input. Check if the answer was right.
 */
	  if (read (0, line, 1) == 1)
	    {
	      if (*line == 27)	/* ESC */
		terminate (SIGINT);
	      if ((unsigned char) *line != (unsigned char) playc)
		{
		  int x;

		  totalerrors++;
		  chars_to_play += 4;
		  randomlist[nrandom++] = playc;
		  randomlist[nrandom++] = playc;
		  for (x = 0; x < nrandom; x++)
		    if (randomlist[x] == *line)
		      {
			randomlist[nrandom++] = *line;
			break;
		      }
		  playerror ();
		  if (++errors > 3)
		    {
		      printf ("It is '%c' not '%c'\r\n", playc, *line);
		      fflush (stdout);
		    }
		  playchar (playc);
		}
	      else
		{
		  errors = 0;
		  randomplay ();
		}
	    }
	}

      if (FD_ISSET (audiofd, &writefds))
	{
/*
 * The audio device is ready to accept more data. Keep the device happy by
 * writing some silent samples to it.
 *
 * Note that the real "playload" signal will be played by the playchar()
 * routine.
 */
	  memset (buf, 0, 1024);
	  write (audiofd, buf, 1024);
	}
    }

/*
 * Everything done. Restore the teminal settings and exit.
 */
  terminate (15);

  close (audiofd);

  exit (0);
}
