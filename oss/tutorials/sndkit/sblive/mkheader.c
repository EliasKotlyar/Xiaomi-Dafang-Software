#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int
main (int argc, char *argv[])
{
  unsigned char buffer[1024];
  int i, l, n = 0;

  if (argc != 2)
    {
      fprintf (stderr, "Bad usage\n");
      exit (-1);
    }

  printf ("static unsigned char %s[] =\n", argv[1]);
  printf ("{\n");

  while ((l = read (0, buffer, sizeof (buffer))) > 0)
    {
      for (i = 0; i < l; i++)
	{
	  if (n > 0)
	    printf (", ");

	  if (n && (n % 16) == 0)
	    printf ("\n");
	  printf ("0x%02x", buffer[i]);
	  n++;
	}
    }

  printf ("\n};\n");

  exit (0);
}
