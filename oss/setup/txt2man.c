/*
 *
 * This file is part of Open Sound System.
 *
 * Copyright (C) 4Front Technologies 1996-2008.
 *
 * This this source file is released under GPL v2 license (no other versions).
 * See the COPYING file included in the main directory of this source
 * distribution for the license terms and conditions.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <string.h>

int section = 0;
char *volume = "Unknown volume";
char *title = "unknown";

int
main (int argc, char *argv[])
{
  char line[1024], *s, upper;
  const char * date;
  int c;
  FILE *f;
  time_t tt;
  extern char *optarg;
  extern int optind;

  if (time (&tt) == (time_t)-1) date = "August 31, 2006";
  else {
    date = ctime (&tt);
    s = strchr (date, '\n');
    if (s) *s = '\0';
  }

  while ((c = getopt (argc, argv, "v:s:t:")) != EOF)
    switch (c)
      {
      case 'v':
	volume = optarg;
	break;
      case 't':
	title = optarg;
	break;
      case 's':
	section = atoi (optarg);
	break;
      }

  if (optind >= argc)
    {
      fprintf (stderr, "%s: No input file specified\n", argv[0]);
      exit (-1);
    }

  if ((f = fopen (argv[optind], "r")) == NULL)
    {
      perror (argv[optind]);
      exit (-1);
    }

  printf (".\" Automatically generated text\n");
  printf (".TH %s %d \"%s\" \"4Front Technologies\" \"%s\"\n", title, section, date, volume);

  while (fgets (line, sizeof (line), f) != NULL)
    {
      s = line;
      upper = 1;

      while (*s && *s != '\n')
	{
	  if (!isupper (*s) && *s != ' ')
	    upper = 0;
	  s++;
	}
      *s = 0;
      if (line[0] == 0)
	upper = 0;

      if (upper) {
	printf (".SH %s\n", line);
      } else {
	  s = line;

	  while (isspace(*s)) s++;
	  if (*s == '\0') {
	    printf (".PP\n");
	    continue;
	  }
	  if (*s == 'o' && isspace(s[1]))
	    {
	      printf (".IP \\(bu 3\n");
	      s += 2;
	      printf ("%s\n", s);
	      continue;
	    }
	  if (*s == '-' && !isspace(s[1])) {
            printf (".TP\n");
            printf ("\\fB");
            while (!isspace (*s)) printf ("%c", *s++);
            printf ("\\fP\n");
            while (isspace(*s)) s++;
          }
	  printf ("%s\n", s);
	}
    }

  fclose (f);

  exit (0);
}
