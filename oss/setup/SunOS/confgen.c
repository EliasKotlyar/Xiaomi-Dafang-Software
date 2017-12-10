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
#include <libgen.h>
#include <sys/stat.h>

char *targetdir = "";
char *confpath = "";

static void
copy_parms (FILE * f, FILE * conf)
{
  char line[1024], *s, *p;
  char var[256] = "", comment[64 * 1024] = "";
  int i;

  while (fgets (line, sizeof (line) - 1, f) != NULL)
    {
      for (i = 0; i < strlen (line); i++)
	if (line[i] == '\n')
	  line[i] = 0;

      s = line;

      while (*s == ' ' || *s == '\t')
	s++;
      if (strncmp (s, "int ", 4) == 0)
	{
	  if (*var != 0)
	    {
	      fprintf (conf, "%s\n", comment);
	      if (*var != 0)
		fprintf (conf, "%s\n", var);
	      *var = 0;
	      *comment = 0;
	    }
	  s += 4;

	  for (i = 0; i < strlen (line); i++)
	    if (line[i] == ';')
	      line[i] = 0;
	  strcpy (var, s);
	  continue;
	}


      s = line;

      while (*s == ' ' || *s == '\t' || *s == '/' || *s == '*')
	s++;

      strcat (comment, "\n# ");
      strcat (comment, s);
    }

  if (*var != 0)
    {
      fprintf (conf, "%s\n", comment);
      fprintf (conf, "%s\n", var);
    }
}

static void
scan_dir (char *srcdir, char *module)
{
  char confname[256], tmp[256], line[1024], syscmd[255];
  FILE *conf;
  FILE *f;
  int i;
  struct stat st;

  int is_pci = 1;
  int check_platform = 0;
  int platform_ok = 0;

  if (stat (srcdir, &st) == -1)
  {
    // perror("stat");
    // fprintf(stderr, "confgen: Cannot access %s\n", srcdir);
    return;
  }

  if (!S_ISDIR (st.st_mode))	/* Not a directory? */
  {
    // fprintf(stderr, "confgen: %s is not a directory\n", srcdir);
    return;
  }

  sprintf (tmp, "%s/.nomake", srcdir);
  if (stat (tmp, &st) != -1)	/* File exists */
    return;			/* Skip this one */

  sprintf (tmp, "%s/.config", srcdir);
  if ((f = fopen (tmp, "r")) != NULL)
    {
      while (fgets (line, sizeof (line) - 1, f) != NULL)
	{
	  char *s;
	  for (i = 0; i < strlen (line); i++)
	    if (line[i] == '\n')
	      line[i] = 0;

	  s = line;
	  while (*s && *s != '=')
	    s++;
	  if (*s == '=')
	    *s++ = 0;

	  if (strcmp (line, "bus") == 0)
	    {
	      if (strcmp (s, "PCI") != 0)
		is_pci = 0;
	      continue;
	    }

	  if (strcmp (line, "platform") == 0)
	    {
	      check_platform = 1;
#ifdef sparc
	      if (strcmp (s, "sparc") == 0)
		platform_ok = 1;
#endif

#ifdef i386
	      if (strcmp (s, "i86pc") == 0)
		platform_ok = 1;
#endif
	      continue;
	    }
	}
      fclose (f);
    }
#if 0
  else
    perror (tmp);
#endif

  if (check_platform && !platform_ok)
    {
      return;
    }

  /* generate the man pages */

  sprintf (tmp, "%s/%s.man", srcdir, module);
  if (stat (tmp, &st) != 0)	/* Man File doesn't exist */
    goto no_manual;			/* Skip this one */

  sprintf (syscmd, "sed 's/CONFIGFILEPATH/%s/' < %s/%s.man > /tmp/ossman.man",
	   confpath, srcdir, module);
  //printf ("%s\n", syscmd);
  unlink ("/tmp/ossman.man");
  system (syscmd);

  sprintf (syscmd,
	   TXT2MAN
	   " -t \"%s\" -v \"Devices\" -s 7d  /tmp/ossman.man > prototype/usr/man/man7d/%s.7d",
	   module, module);
  //printf ("%s\n", syscmd);
  system (syscmd);

no_manual:
  sprintf (confname, "%s/%s.conf", targetdir, module);

  if ((conf = fopen (confname, "w")) == NULL)
    {
      perror (confname);
      exit (-1);
    }

  fprintf (conf, "# Open Sound System configuration file\n");

  if (is_pci)
    fprintf (conf,
	     "# Please consult the documentation before changing\n# interrupt-priorities\ninterrupt-priorities=9 ");
  else
    fprintf (conf, "name=\"%s\" parent=\"pseudo\" instance=0 ", module);

   fprintf (conf, "ddi-no-autodetach=1 ddi-forceattach=1 \n");

  sprintf (tmp, "%s/.params", srcdir);
  if ((f = fopen (tmp, "r")) != NULL)
    {
      copy_parms (f, conf);
      fclose (f);
    }

  fprintf (conf, ";\n");
  fclose (conf);
}

int
main (int argc, char *argv[])
{
  int i;

  if (argc < 3)
    exit (-1);

  targetdir = argv[1];
  confpath = argv[2];

  for (i = 3; i < argc; i++)
    {
      scan_dir (argv[i], basename (argv[i]));
    }

  exit (0);
}
