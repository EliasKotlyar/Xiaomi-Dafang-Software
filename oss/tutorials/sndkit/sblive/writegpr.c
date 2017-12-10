#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#define oss_native_ulong unsigned long
#define sound_os_info unsigned long
#define ac97_devc unsigned long
typedef int oss_mutex;
#include <soundcard.h>
#include <sblive.h>

int
main (int argc, char *argv[])
{
  int ossfd, reg, value;
  char *dspname = "/dev/dsp";
  sblive_reg rg;

  if (argc < 3)
    {
      fprintf (stderr, "Usage: %s GPR hexvalue\n", argv[0]);
      exit (-1);
    }

  if (sscanf (argv[1], "%d", &reg) != 1)
    exit (-1);

  if (sscanf (argv[2], "%x", &value) != 1)
    exit (-1);

  if ((ossfd = open (dspname, O_WRONLY, 0)) == -1)
    {
      perror (dspname);
      exit (-1);
    }

  rg.reg = GPR0 + reg;
  rg.chn = 0;
  rg.value = value;

  if (ioctl (ossfd, SBLIVE_WRITEREG, &rg) == -1)
    perror (dspname);


  exit (0);
}
