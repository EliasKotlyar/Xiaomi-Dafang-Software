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

#define OSS_LICENSE ""

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include "/tmp/buildid.h"

#define MAX_ERRORS	1000

typedef struct
{
  int err;
  char *sourcename;
  int linenum;
  char *msg;
  char *desc;
} errordesc_t;

errordesc_t *errors[MAX_ERRORS];
int nerrors = 0;

char *
strmatch (char *s, const char *pattern)
{
  int i, l, pl;

  if (s == NULL || *pattern == 0)
    return NULL;

  l = strlen (s);
  pl = strlen (pattern);

  if (pl > l)
    return NULL;

  while (s && l >= pl)
    {
      int ok = 0;

      for (i = 0; !ok && i < pl; i++)
	if (s[i] != pattern[i])
	  {
	    ok = 1;
	    continue;
	  }

      if (!ok)
	return s;

      s++;
      l--;
    }
  return NULL;
}

char *
check_comment (FILE * f, const char *sourcename, int *linenum)
{
  char line[4096];
  int is_desc = 0;
  static char errorbuf[64 * 1024];
  int done = 0;

  *errorbuf = 0;

  if (fgets (line, sizeof (line) - 1, f) != NULL)
    {
      char *s, *p;
      int l;

      *linenum = *linenum + 1;

      if ((l = strlen (line)) > 4000)
	{
	  fprintf (stderr, "Too long line in %s\n", sourcename);
	  exit (-1);
	}

      /*
       * Strip trailing CR and LF characters.
       */
      s = line + l - 1;
      while (*s == '\n' || *s == '\r')
	*s-- = 0;

      s = line;

      while (*s == ' ' || *s == '\t')
	s++;

      if (s[0] != '/' || s[1] != '*')
	return NULL;

      if ((p = strmatch (s, "Errordesc:")))
	{
	  p += 10;
	  while (*p == ' ')
	    p++;

	  is_desc = 1;
	  if ((s = strmatch (p, "*/")))
	    {
	      *s = 0;
	      return p;
	    }
	  else
	    {
	      strcat (errorbuf, p);
	    }
	}
    }

/* 
 * Handle comment continuation lines.
 */

  while (!done && fgets (line, sizeof (line) - 1, f) != NULL)
    {
      char *s, *p;
      int l;

      *linenum = *linenum + 1;

      if ((l = strlen (line)) > 4000)
	{
	  fprintf (stderr, "Too long line in %s\n", sourcename);
	  exit (-1);
	}

      /*
       * Strip trailing CR and LF characters.
       */
      s = line + l - 1;
      while (*s == '\n' || *s == '\r')
	*s-- = 0;

      s = line;

      while (*s == ' ' || *s == '\t' || *s == '*')
	s++;
      if (*s == '/' && s[-1] == '*')	/* End of comment */
	break;

      if ((p = strmatch (s, "Errordesc:")))
	{
	  p += 10;
	  while (*p == ' ')
	    p++;

	  is_desc = 1;
	  if ((s = strmatch (p, "*/")))
	    {
	      *s = 0;
	      if (*errorbuf != 0)
		strcat (errorbuf, " ");
	      strcat (errorbuf, p);
	      done = 1;
	    }
	  else
	    {
	      if (*errorbuf != 0)
		strcat (errorbuf, " ");
	      strcat (errorbuf, p);
	    }
	  continue;
	}

      p = s;
      if ((s = strmatch (p, "*/")))
	{
	  *s = 0;
	  if (*errorbuf != 0)
	    strcat (errorbuf, " ");
	  strcat (errorbuf, p);
	  done = 1;
	}
      else
	{
	  if (*errorbuf != 0)
	    strcat (errorbuf, " ");
	  strcat (errorbuf, p);
	}
    }

  if (is_desc)
    return errorbuf;
  return NULL;
}

void
store_error (int errnum, const char *sourcename, int linenum, const char *msg,
	     const char *desc)
{
  errordesc_t *err;

  //printf("%s:%d: %05d=%s\n", sourcename, linenum, errnum, msg);

  if (nerrors >= MAX_ERRORS)
    {
      fprintf (stderr, "Too many errors\n");
      exit (-1);
    }

  if ((err = malloc (sizeof (*err))) == NULL)
    {
      fprintf (stderr, "Too many errors defined\n");
      exit (-1);
    }

  err->err = errnum;
  err->sourcename = strdup (sourcename);
  err->linenum = linenum;
  err->msg = strdup (msg);
  if (desc == NULL)
    err->desc = NULL;
  else
    err->desc = strdup (desc);

  errors[nerrors++] = err;
}

void
parse_sourcefile (char *sourcename)
{
  int linenum = 0;
  char line[4096];
  FILE *f;

  if (*sourcename == '.')
    sourcename += 2;		/* Skip the "./" directory prefix */

/*
 * Don't handle myself or oss_config.h.
 */

  if (strcmp (basename (sourcename), "update_errors.c") == 0)
    return;
  if (strcmp (basename (sourcename), "oss_config.h") == 0)
    return;

  if ((f = fopen (sourcename, "r")) == NULL)
    {
      perror (sourcename);
      exit (-1);
    }

  while (fgets (line, sizeof (line) - 1, f) != NULL)
    {
      char *s;
      int l;

      linenum++;

      if ((l = strlen (line)) > 4000)
	{
	  fprintf (stderr, "Too long line in %s\n", sourcename);
	  exit (-1);
	}

      /*
       * Strip trailing CR and LF characters.
       */
      s = line + l - 1;
      while (*s == '\n' || *s == '\r')
	*s-- = 0;

      if ((s = strmatch (line, "OSSERR")))
	{
	  char *p;
	  char tmp[4096];
	  char tmp2[4096];
	  int num, thisline;

	  s += 6;		/* Skip "OSSERR" */
	  while (*s == ' ' || *s == '\t' || *s == '(')
	    s++;

	  /* 
	   * Get the error number
	   */
	  p = s;
	  while (*s >= '0' && *s <= '9')
	    s++;

	  while (*s == ',' || *s == ' ' || *s == '\t' || *s == '"')
	    *s++ = 0;

	  if (sscanf (p, "%d", &num) != 1)
	    {
	      fprintf (stderr, "Bad error number in %s:%d\n",
		       sourcename, linenum);
	    }

	  p = s;

	  if (*p == 0)
	    {
	      /*
	       * No '"' was found. Read the continuation line.
	       */
	      fgets (tmp, sizeof (tmp) - 1, f);
	      p = tmp;

	      while (*p == ' ' || *p == '\t' || *p == '"')
		p++;

	      s = p;
	      while (*s && *s != '"')
		s++;
	      if (*s)
		*s++ = 0;
	    }

	  while (*s && *s != '"')
	    s++;
	  if (*s)
	    *s++ = 0;
	  // printf("%s:%d: %d=%s\n", sourcename, linenum, num, p);
	  while (*s && *s != ';')
	    s++;

	  if (*s != ';')	/* No semicolon. Discard the next line */
	    fgets (tmp2, sizeof (tmp2) - 1, f);
	  *s = 0;

	  /*
	   * Check if the next line starts the descriptive comment
	   */

	  thisline = linenum;
	  if ((s = check_comment (f, sourcename, &linenum)))
	    store_error (num, sourcename, thisline, p, s);
	  else
	    store_error (num, sourcename, thisline, p, NULL);
	}
    }

  fclose (f);
}

int
compare (const void *a_, const void *b_)
{
  const char *const *a__ = a_;
  const char *const *b__ = b_;
  const errordesc_t *a = *a__;
  const errordesc_t *b = *b__;

  if (a->err == b->err)
    return 0;
  if (a->err < b->err)
    return -1;

  return 1;
}

int
main (int argc, char *argv[])
{
  int i, status = 0;

  for (i = 1; i < argc; i++)
    {
      parse_sourcefile (argv[i]);
    }

  qsort (errors, nerrors, sizeof (errordesc_t *), compare);

  for (i = 0; i < nerrors; i++)
    {
      if (errors[i]->err == 0)
	{
	  fprintf (stderr, "%s:%d: Error number is zero\n",
		   errors[i]->sourcename, errors[i]->linenum);
	  status = 1;
	  continue;
	}

      if (i > 0 && errors[i]->err == errors[i - 1]->err)
	{
	  fprintf (stderr, "%s:%d: duplicate error %05d\n",
		   errors[i]->sourcename, errors[i]->linenum, errors[i]->err);
	  status = 1;
	  continue;
	}

      if (i < nerrors - 1 && errors[i]->err == errors[i + 1]->err)
	{
	  fprintf (stderr, "%s:%d: duplicate error %05d\n",
		   errors[i]->sourcename, errors[i]->linenum, errors[i]->err);
	  status = 1;
	  continue;
	}
    }

  printf ("<html>\n");
  printf ("<head><title>OSS run time error list for OSS v%s</title></head>\n",
	  OSS_VERSION_STRING);
  printf ("<body style=\"font-family: Helvetica,Arial,sans-serif;\">\n");
  printf ("<h2>OSS run time error list for OSS v%s</h2>\n",
	  OSS_VERSION_STRING);

  printf ("<table border=\"0\">\n");
  printf ("<tr><td><b>Code</b></td><td><b>Description</b></td>/</tr>\n");
  for (i = 0; i < nerrors; i++)
    {
#if 1
      printf
	("<tr><td style=\"vertical-align: top;\">%05d</td><td><b>%s</b>\n",
	 errors[i]->err, errors[i]->msg);

      if (errors[i]->desc != NULL)
	printf ("<br>%s\n", errors[i]->desc);
      printf ("</td></tr>\n");
#else
      printf ("<p><b>%05d: %s</b></p>\n", errors[i]->err, errors[i]->msg);

      if (errors[i]->desc != NULL)
	printf ("<ul><li>%s</li></ul>\n", errors[i]->desc);
#endif
    }
  printf ("</table>\n");
  printf ("</body>\n");
  printf ("</html>\n");

  exit (status);
}
