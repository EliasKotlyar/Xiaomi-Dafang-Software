#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

int ncodes;

char randomlist[256];
int nrandom = 0;

#include "charlist.h"

int
main (int argc, char *argv[])
{
  int ngroups = 0, maxgroups = 20;
  ncodes = strlen (Chars);

  srandom (time (0));

  if (argc > 1)
    {
      parse_charlist (argv[1]);
    }
  else
    {
      strcpy (randomlist, Chars);
      nrandom = strlen (randomlist);
    }

  if (argc > 2)
    maxgroups = atoi (argv[2]);

  if (nrandom < 2)
    {
      printf ("Bad character list\n");
      exit (-1);
    }

  while (1)
    {
      int i, c;

      if (ngroups && !(ngroups % 5))
	printf ("\n");

      if (ngroups++ >= maxgroups)
	{
	  printf ("\n");
	  exit (0);
	}


      for (i = 0; i < 5; i++)
	{
	  c = random () % nrandom;

	  printf ("%c", randomlist[c]);
	}

      printf (" ");
      fflush (stdout);
    }
}
