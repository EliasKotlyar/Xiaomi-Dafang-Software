#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>

#include <soundcard.h>
#ifdef USERLAND
#define oss_native_word unsigned long
#define oss_mutex_t unsigned long
#define oss_device_t unsigned long
#define ac97_devc unsigned long
#define oss_midi_inputbyte_t char
#define uart401_devc unsigned long
typedef int oss_mutex;
typedef void *oss_dma_handle_t;
#else
#include "../../../kernel/framework/include/os.h"
#endif /* USERLAND */
#include "../../../kernel/drv/oss_sblive/sblive.h"

int work_done = 0;
int ossfd;
emu10k1_file fle;

static int
find_last_device (void)
{
  oss_sysinfo info;
  int mixerfd;

  if ((mixerfd = open ("/dev/mixer0", O_RDWR, 0)) == -1)
    {
      perror ("/dev/mixer0");
      return 0;
    }

  if (ioctl (mixerfd, SNDCTL_SYSINFO, &info) == -1)
    {
      perror ("SNDCTL_SYSINFO");
      exit (-1);
    }

  close (mixerfd);
  return info.numaudios - 1;
}

static void
touch_all_mixers (int ossfd)
{
  int i, n;

  for (i = 0; i < 16; i++)
    {
      n = i;
      if (ioctl (ossfd, SNDCTL_MIX_NREXT, &n) == -1)
	return;
    }
}

static void
do_download (void)
{
  int card_type;

  if (ioctl (ossfd, SBLIVE_GETCHIPTYPE, &card_type) == -1)
    {
      perror ("???");
      return;
    }

  if (card_type != fle.feature_mask)
    {
      fprintf (stderr, "Device/file incompatibility (%x/%x)\n", card_type,
	       fle.feature_mask);
      return;
    }

  if (ioctl (ossfd, SBLIVE_WRITECODE1, fle.code) == -1)
    {
      perror ("code1");
      return;
    }

  if (ioctl (ossfd, SBLIVE_WRITECODE2, fle.code + 512) == -1)
    {
      perror ("code2");
      return;
    }

  if (fle.parms.ngpr > 0)
    {
      if (ioctl (ossfd, SBLIVE_WRITEPARMS, &fle.parms) == -1)
	{
	  perror ("parms");
	  return;
	}

      touch_all_mixers (ossfd);
    }

  if (fle.consts.nconst > 0)
    {
      if (ioctl (ossfd, SBLIVE_WRITECONST, &fle.consts) == -1)
	{
	  perror ("consts");
	  return;
	}

    }
  work_done = 1;
}

static void
do_all_devices (void)
{
  int i, mixerfd;
  oss_sysinfo info;
  char dspname[100];

  if ((mixerfd = open ("/dev/mixer0", O_RDWR, 0)) == -1)
    {
      perror ("/dev/mixer");
      exit (-1);
    }

  if (ioctl (mixerfd, SNDCTL_SYSINFO, &info) == -1)
    {
      perror ("SNDCTL_SYSINFO");
      exit (-1);
    }

  for (i = 0; i < info.numaudios; i++)
    {
      oss_audioinfo ainfo;
      int acc;

      ainfo.dev = i;
      if (ioctl (mixerfd, SNDCTL_AUDIOINFO, &ainfo) == -1)
	{
	  perror ("SNDCTL_AUDIOINFO");
	  exit (-1);
	}

      if (ainfo.magic == fle.magic)
	{

	  sprintf (dspname, "/dev/dsp%d", i);
	  if ((ossfd = open (dspname, O_RDWR, 0)) == -1)
	    {
	      perror (dspname);
	      exit (-1);
	    }

	  do_download ();
	  close (ossfd);
	}
    }
  close (mixerfd);
}

int
main (int argc, char *argv[])
{
  int fd, dspnum = 0;
  char *dspname = "/dev/dsp", namebuf[32];

  if (argc < 2)
    {
      fprintf (stderr, "Usage: %s dspcodefile\n", argv[0]);
      exit (-1);
    }

  if ((fd = open (argv[1], O_RDONLY, 0)) == -1)
    {
      perror (argv[1]);
      exit (-1);
    }

  if (argc > 2)
    dspname = argv[2];

  if (read (fd, &fle, sizeof (fle)) != sizeof (fle))
    {
      fprintf (stderr, "Short file\n");
      exit (-1);
    }

  if (fle.magic != EMU10K1_MAGIC && fle.magic != EMU10K2_MAGIC)
    {
      fprintf (stderr, "Bad microcode file %s\n", argv[1]);
      exit (-1);
    }

  if (strcmp (dspname, "-a") == 0)
    {
      do_all_devices ();
      exit (0);
    }

  if (strcmp (dspname, "-l") == 0)
    {				/* Last /dev/dsp# file requested */
      dspnum = find_last_device ();
      sprintf (namebuf, "/dev/dsp%d", dspnum);
      dspname = namebuf;
    }

  if ((ossfd = open (dspname, O_RDWR, 0)) == -1)
    {
      perror (dspname);
      exit (-1);
    }

  do_download ();

  if (!work_done)
    exit (-1);
  exit (0);
}
