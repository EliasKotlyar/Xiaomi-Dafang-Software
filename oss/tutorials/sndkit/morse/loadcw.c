#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define TESTFILE "Vvcw"

static int ngroups = 0;

static void
printchar (unsigned char c)
{
  switch (c)
    {
    case ' ':
      ngroups++;
      if (ngroups && !(ngroups % 6))
	printf (" \n");
      else
	printf (" ");
      break;

    case 0x99:
      printf ("ö");
      break;
    case 0x8e:
      printf ("ä");
      break;
    case 0x8f:
      printf ("å");
      break;
    default:
      if (c <= 'Z' && c >= 'A')
	c = c + 32;
      printf ("%c", c);
    }
}

static int
showtest (char *tname)
{
  FILE *f;
  unsigned char line[1024], *p;
  int on = 0;

  if ((f = fopen (TESTFILE, "r")) == NULL)
    {
      perror (TESTFILE);
      return -1;
    }

  while (fgets (line, sizeof (line), f))
    {
      p = &line[strlen (line) - 1];
      while (p > line)
	{
	  if (*p == '\n' || *p == '\r')
	    *p = 0;
	  p--;
	}

      if (*line == '#')
	{
	  if (on)
	    {
	      fclose (f);
	      printf ("\n");
	      return 0;
	    }

	  if (strcmp (line + 1, tname) == 0)
	    on = 1;
	}
      else if (on)
	{
	  p = line;

	  while (*p)
	    {
	      switch (*p)
		{
		default:
		  printchar (*p);
		}
	      p++;
	    }
	}

    }
  fclose (f);

  return 1;
}

static char *
randomtest (void)
{
  FILE *f;
  unsigned char line[1024], *p;
  static char name[10] = "????";
  int n = 0, x;
  char *tests[1000];

  if ((f = fopen (TESTFILE, "r")) == NULL)
    {
      perror (TESTFILE);
      return NULL;
    }

  while (fgets (line, sizeof (line), f))
    {
      if (strlen (line) < 1)
	continue;
      p = &line[strlen (line) - 1];
      while (p > line)
	{
	  if (*p == '\n' || *p == '\r')
	    *p = 0;
	  p--;
	}
      if (strlen (line) < 1)
	continue;

      if (*line == '#')
	{
	  char *tmp;
	  tmp = malloc (strlen (line + 1) + 1);
	  strcpy (tmp, line + 1);

	  if (n >= 1000)
	    {
	      fprintf (stderr, "Too many test texts\n");
	      exit (-1);
	    }

	  tests[n++] = tmp;
	}
    }
  fclose (f);

  srandom (time (0));

  x = random () % n;		/* Select one of the tests randomly */
  strcpy (name, tests[x]);

  return name;
}

int
main (int argc, char *argv[])
{
  char *thistest;

  if (argc > 1)
    exit (showtest (argv[1]));

  thistest = randomtest ();
  if (thistest == NULL)
    exit (-1);
  fprintf (stderr, "Selected test text is '%s'\n", thistest);
  fflush (stderr);
  exit (showtest (thistest));
}
