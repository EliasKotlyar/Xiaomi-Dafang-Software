/*
 * Purpose: Simple replacement for cpp -M (gcc)
 */
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
#include <string.h>

int ndirs = 0;
char *dirs[1024];

int ndupes = 0;
char *dupes[4096];

static void
add_includedir (char *d)
{
  dirs[ndirs++] = d;
}

static int
dupe_file (char *name)
{
  int i;

  for (i = 0; i < ndupes; i++)
    if (strcmp (dupes[i], name) == 0)
      return 1;

  dupes[ndupes++] = strdup (name);
  return 0;
}

static void parse_f (FILE * f, char *pathname);

static void
do_global_include (char *hdrname, char *pathname)
{
  FILE *f;
  int i;

  char tmp[512];
  sprintf (tmp, "%s/%s", pathname, hdrname);

  if (dupe_file (tmp))
    return;
  if ((f = fopen (tmp, "r")) != NULL)
    {
      printf ("%s ", tmp);
      parse_f (f, pathname);
      fclose (f);
      return;
    }

  for (i = 0; i < ndirs; i++)
    {
      sprintf (tmp, "%s/%s", dirs[i], hdrname);
      if (dupe_file (tmp))
	return;

      if ((f = fopen (tmp, "r")) == NULL)
	continue;
      printf ("\\\n  %s ", tmp);

      parse_f (f, dirs[i]);
      fclose (f);
      return;
    }

  fprintf (stderr, "Cannot locate <%s>\n", hdrname);
  printf ("\n");
  exit (-1);
}

static void
do_local_include (char *hdrname, char *pathname)
{
  FILE *f;
  char tmp[512];

  sprintf (tmp, "%s/%s", pathname, hdrname);

  if (dupe_file (tmp))
    {
      return;
    }

  if ((f = fopen (tmp, "r")) == NULL)
    {
#if 1
      do_global_include (hdrname, pathname);
      return;
#else
      perror (tmp);
      printf ("\n");
      exit (-1);
#endif
    }

  sprintf (tmp, "%s ", tmp);
  parse_f (f, pathname);
  printf ("\\\n  %s ", tmp);

  fclose (f);
}

static void
parse_f (FILE * f, char *pathname)
{
  char line[1024], *p, *s;

  while (fgets (line, sizeof (line), f) != NULL)
    {
      if (*line != '#')
	continue;

      s = line + 1;

      /* Skip space */
      while (*s && (*s == ' ' || *s == '\t'))
	s++;

      /* Extract the preprocessor directive name (p) */
      p = s;
      while (*s && (*s != ' ' && *s != '\t'))
	s++;
      *s++ = 0;

      if (strcmp (p, "include") != 0)
	continue;

      /* Skip space */
      while (*s && (*s == ' ' || *s == '\t'))
	s++;

      if (*s == '"')
	{
	  s++;

	  p = s;
	  while (*s && *s != '"')
	    s++;
	  *s = 0;
	  do_local_include (p, pathname);
	  continue;
	}

      if (*s == '<')
	{
	  s++;

	  p = s;
	  while (*s && *s != '>')
	    s++;
	  *s = 0;
	  do_global_include (p, pathname);
	  continue;
	}
    }
}

static void
parse_sourcefile (char *srcname)
{
  FILE *f;
  char *s, *p;
  char origname[64];

  strcpy (origname, srcname);

  s = NULL;

  p = srcname;
  while (*p)
    {
      if (*p == '.')
	s = p;
      p++;
    }

  if (*s != '.')
    {
      fprintf (stderr, "Bad file name %s\n", srcname);
      printf ("\n");
      exit (-1);
    }

  if ((f = fopen (srcname, "r")) == NULL)
    {
      perror (srcname);
      printf ("\n");
      exit (-1);
    }

  *s++ = 0;
  printf ("%s.o: %s ", srcname, origname);

  parse_f (f, ".");
  printf ("\n");
  fclose (f);
}

int
main (int argc, char *argv[])
{
  int i;

  add_includedir ("/usr/include");
  add_includedir ("/usr/local/include");

  for (i = 1; i < argc; i++)
    if (argv[i][0] == '-')
      switch (argv[i][1])
	{
	case 'I':
	  add_includedir (argv[i] + 2);
	  break;
	}
    else
      parse_sourcefile (argv[i]);

  exit (0);
}
