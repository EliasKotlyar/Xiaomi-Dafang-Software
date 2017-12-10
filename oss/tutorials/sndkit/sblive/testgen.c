#include <stdio.h>
#include <stdlib.h>

#define N 64

static void
gen_inputtest (void)
{
  int i;

  printf ("// Input test program\n");
  printf ("#include \"vu.mac\"\n");
  printf (".gpr VUTMP\n");
  printf (".gpr TMP\n");

  for (i = 0; i < N; i += 2)
    {
      printf (".input I%d_L	%d\n", i, i);
      printf (".input I%d_R	%d\n", i, i + 1);
    }

  for (i = 0; i < N; i += 2)
    {
      if (!(i % 16))
	printf (".parm G%x		0		group\n", i);
      printf (".parm IVU%d	0		stereopeak\n", i);
    }

  for (i = 0; i < N; i += 2)
    {
      printf ("ACC3(TMP, I%d_L, 0, 0)\n", i);
      printf ("VU(IVU%d_L, TMP)\n", i);
      printf ("ACC3(TMP, I%d_R, 0, 0)\n", i);
      printf ("VU(IVU%d_R, TMP)\n", i);
    }

}

static void
gen_outputtest (void)
{
  int i;

  printf ("// Output test program\n");
  printf ("#include \"vu.mac\"\n");
  printf (".parm VU 0 stereopeak\n");
  printf (".gpr VUTMP\n");
  printf (".gpr TMP\n");
  printf (".gpr TMP_L\n");
  printf (".gpr TMP_R\n");
  printf (".send TEST_L 0\n");
  printf (".send TEST_R 1\n");

  for (i = 0; i < N; i += 2)
    {
      printf (".output O%d_L	%d\n", i, i);
      printf (".output O%d_R	%d\n", i, i + 1);
    }

  for (i = 0; i < N; i += 2)
    {
      if (!(i % 16))
	printf (".parm G%x		0		group\n", i);
      printf (".parm OUT%d	0		stereo\n", i);
    }

  printf ("ACC3(TMP_L, TEST_L, 0, 0)\n");
  printf ("ACC3(TMP_R, TEST_R, 0, 0)\n");
  printf ("VU(VU_L, TMP_L)\n");
  printf ("VU(VU_R, TMP_R)\n");

  for (i = 0; i < N; i += 2)
    {
      printf ("MACS(O%d_L, 0, TMP_L, OUT%d_L)\n", i, i);
      printf ("MACS(O%d_R, 0, TMP_R, OUT%d_R)\n", i, i);
    }

}

int
main (int argc, char *argv[])
{
  if (argc < 2)
    exit (-1);

  if (strcmp (argv[1], "in") == 0)
    gen_inputtest ();

  if (strcmp (argv[1], "out") == 0)
    gen_outputtest ();
  exit (0);
}
