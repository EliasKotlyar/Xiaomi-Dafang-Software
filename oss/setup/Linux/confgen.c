#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

char *targetdir = "";
char *confpath = "";

static int
copy_parms (FILE * f, FILE * conf)
{
  char line[1024], *s;
  char var[256] = "", comment[64 * 1024] = "";
  int i;
  int ok = 0;

  while (fgets (line, sizeof (line), f) != NULL)
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
		fprintf (conf, "#%s\n#\n", var);
	      *var = 0;
	      *comment = 0;
	    }
	  s += 4;

	  for (i = 0; i < strlen (line); i++)
	    if (line[i] == ';')
	      line[i] = 0;
	  ok = 1;
	  strcpy (var, s);
	  continue;
	}


      s = line;

      while (*s == ' ' || *s == '\t' || *s == '/' || *s == '*')
	s++;

      if (*comment != 0)
	strcat (comment, "\n");
      strcat (comment, "# ");
      strcat (comment, s);
    }

  if (*var != 0)
    {
      fprintf (conf, "%s\n", comment);
      fprintf (conf, "#%s\n", var);
    }

  return ok;
}

static void
scan_dir (const char *srcdir, const char *modnam)
{
  char confname[256], tmp[256], line[1024];
  char module[256], *p;
  FILE *conf;
  FILE *f;
  int i;
  struct stat st;

  int check_platform = 0;
  int platform_ok = 0;
  int ok = 0;

  strcpy (module, modnam);
  p = module;
  while (*p)
    {
      if (*p == '.')
	*p = 0;
      else
	p++;
    }

  if (stat (srcdir, &st) == -1)
    return;

  if (!S_ISDIR (st.st_mode))	/* Not a directory? */
    return;

  sprintf (tmp, "%s/.nomake", srcdir);
  if (stat (tmp, &st) != -1)	/* File exists */
    return;			/* Skip this one */

  sprintf (tmp, "%s/.config", srcdir);
  if ((f = fopen (tmp, "r")) != NULL)
    {
      while (fgets (line, sizeof (line), f) != NULL)
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

	  if (strcmp (line, "OS") == 0 || strcmp (line, "targetos") == 0)
	    {
	      check_platform = 1;
	      if (strcmp (s, "Linux") == 0)
		platform_ok = 1;
	      continue;
	    }
	}
      fclose (f);
    }

  if (check_platform && !platform_ok && strcmp (modnam, "osscore"))
    {
      return;
    }
#if 0
  /* copy the man pages */
  sprintf (syscmd, "sed 's:CONFIGFILEPATH:%s:g' < %s/%s.man > /tmp/ossman.man",
	   confpath, srcdir, module);
  printf ("%s\n", syscmd);
  unlink ("/tmp/ossman.man");
  system (syscmd);

  sprintf (syscmd,
	   "./origdir/setup/txt2man -t \"%s\" -v \"Devices\" -s 7  /tmp/ossman.man > prototype/usr/man/man7/%s.7",
	   module, module);
  printf ("%s\n", syscmd);
  system (syscmd);
#endif

  sprintf (confname, "%s/%s.conf", targetdir, module);
  sprintf (tmp, "%s/.params", srcdir);
  if ((f = fopen (tmp, "r")) != NULL)
    {
      if ((conf = fopen (confname, "w")) == NULL)
        {
          perror (confname);
          exit (-1);
        }
      fprintf (conf, "# Open Sound System configuration file\n");
      fprintf (conf,
	   "# Remove the '#' in front of the option(s) you like to set.\n#\n");
      ok = copy_parms (f, conf);
      fclose (f);
//      fprintf(conf, "#\n");
      fclose (conf);
      if (!ok)
        unlink (confname);
    }
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
