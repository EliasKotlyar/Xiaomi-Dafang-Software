/*
 * Purpose: A sample program for developing a simple mixer applet.
 * Copyright (C) 4Front Technologies, 2007. Released under GPLv2/CDDL.
 *
 * This program is not usefull by itself. It just demonstrates techniques that
 * can be used when developing very simple mixer applets that control just
 * the key volumes in the system.
 *
 * The full OSS 4.0 mixer API is rather complex and designed for allmighty
 * master mixer applications. However there is a subset of the API that can be
 * used rather easily. This subset is limited to control of the main output volume,
 * audio/wave/pcm playback volume and/or recording input level. It cannot be
 * used for anything else.
 *
 * This program demonstrates three main techniques to be used by mixer applets:
 *
 * 1) How to find the default mixer device that controls the primary
 *    sound card/device in the system. This device is connected to the
 *    primary (desktop) speakers and the default system sounds/beep are
 *    directed to it. Normally this device is the audio chip installed on
 *    the motherboard of the computer.
 *
 * 2) How to find out the main, pcm and recording volume controls for
 *    the given device.
 *
 * 3) How to read the current volume and how to change it.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <soundcard.h>
#include <time.h>
#include <errno.h>

oss_sysinfo sysinfo;

#define MAX_CTL	50

static int mixer_dev = -1;	/* Use the default mixer */

int
find_default_mixer (int mixer_fd)
{
  int default_mix = -1;
  int best_pri = -2;

  oss_mixerinfo mi;

  int i;

/*
 * The default mixer device in the system can be found by checking the
 * priority parameter of all mixer devices in the system. The device with the
 * highest priority value is the winner. If there are multiple devices with the
 * same priority then the first one should be selected.
 *
 * Note that there should be some method for selecting the mixer device number.
 * In many cases the user actually wants to use some other mixer than the
 * motherboard one.
 */

  for (i = 0; i < sysinfo.nummixers; i++)
    {
      mi.dev = i;

      if (ioctl (mixer_fd, SNDCTL_MIXERINFO, &mi) == -1)
	{
	  perror ("SNDCTL_MIXERINFO");
	  continue;
	}

      if (mi.priority < -1)	/* Not suitable default mixer */
	continue;

      if (mi.priority > best_pri)
	{
	  default_mix = i;
	  best_pri = mi.priority;
	}
    }

  printf("Mixer device %d seems to be the most probable motherboard device\n",
	 default_mix);

  return default_mix;
}

void
show_controls (int mixer_fd, char *heading, int mixer_dev, int ctls[], int count)
{
  oss_mixext ext;
  oss_mixer_value val;
  int ctl, i;

  printf("\n***** %s *****\n", heading);

  for (i=0;i<count;i++)
  {
	  ctl = ctls[i];
/*
 * Obtain the mixer extension definition. It might be a good idea to cache
 * this info in global variables so that doesn't need to be reloaded
 * every time. 
 *
 * Reloading this info every time may cause serious troubles because in that
 * way the application cannot be noticed after the mixer interface has changed.
 */

  ext.dev = mixer_dev;
  ext.ctrl = ctl;

  if (ioctl (mixer_fd, SNDCTL_MIX_EXTINFO, &ext) == -1)
    {
      perror ("SNDCTL_MIX_EXTINFO");
      exit (-1);
    }

/*
 * Have to initialize the dev, ctl and timestamp fields before reading the
 * actual value.
 */

  val.dev = mixer_dev;
  val.ctrl = ctl;
  val.timestamp = ext.timestamp;

  if (ioctl (mixer_fd, SNDCTL_MIX_READ, &val) == -1)
    {
      if (errno == EIDRM)
	{
/*
 * Getting errno=EIDRM tells that the mixer struicture has been changed. This
 * may happen for example if new firmware gets loaded to the device. In such 
 * case the application should start from the beginning and to load all
 * the information again.
 *
 */
	  fprintf (stderr, "Mixer structure changed. Please try again\n");
	  exit (-1);
	}

      perror ("SNDCTL_MIX_READ");
      exit (-1);
    }

  printf ("\t%3d: %s\t ", ctl, ext.extname);

  switch (ext.type)
    {
    case MIXT_MONOSLIDER:
      printf ("monoslider %d ", val.value & 0xff);
      break;

    case MIXT_STEREOSLIDER:
      printf ("stereoslider %d:%d ", val.value & 0xff,
	      (val.value >> 8) & 0xff);
      break;

    case MIXT_SLIDER:
      printf ("slider %d ", val.value);
      break;

    case MIXT_MONOSLIDER16:
      printf ("monoslider %d ", val.value & 0xffff);
      break;

    case MIXT_STEREOSLIDER16:
      printf ("stereoslider %d:%d ", val.value & 0xffff,
	      (val.value >> 16) & 0xffff);
      break;

/*
 * Sometimes there may be just a MUTE control instead of a slider. However
 * it's also possible that there is both mute and a slider.
 */
    case MIXT_ONOFF:
      printf ("ONOFF %d ", val.value);
      break;

    case MIXT_MUTE:
      printf ("mute %d ", val.value);
      break;

/*
 * Enumerated controls may be used for example for recording source
 * selection.
 */
    case MIXT_ENUM:
      printf ("Selection %d ", val.value);
      break;


    default:
      printf ("Unknown control type (%d), value=0x%08x ", ext.type,
	      val.value);
    }

  printf ("\n");
  }

}

int
main (int argc, char *argv[])
{
  int mixer_fd = -1;
  int i, n;

  /*
   * Bins for the mixer controls.
   */
#define ADD_TO_BIN(bin, ctl) \
  if (n_##bin >= MAX_CTL) \
  { \
	  fprintf(stderr, #bin " table is full\n"); exit(-1); \
  } \
  bin##_ctls[n_##bin++] = ctl

  int mainvol_ctls[MAX_CTL];
  int pcmvol_ctls[MAX_CTL];
  int recvol_ctls[MAX_CTL];
  int monvol_ctls[MAX_CTL];
  int n_mainvol=0, n_pcmvol=0, n_recvol=0, n_monvol=0;
  char *devmixer;

  if ((devmixer=getenv("OSS_MIXERDEV"))==NULL)
     devmixer = "/dev/mixer";

/*
 * Get the mixer device number from command line.
 */
  if (argc > 1)
    mixer_dev = atoi (argv[1]);

/*
 * Open /dev/mixer. This device file can be used regardless of the actual
 * mixer device number.
 */

  if ((mixer_fd = open (devmixer, O_RDWR, 0)) == -1)
    {
      perror (devmixer);
      exit (-1);
    }

/*
 * Get OSS system info to a global buffer.
 */

  if (ioctl (mixer_fd, SNDCTL_SYSINFO, &sysinfo) == -1)
    {
      perror ("SNDCTL_SYSINFO");
      exit (-1);
    }

/*
 * Check the mixer device number.
 */

  if (mixer_dev == -1)
    mixer_dev = find_default_mixer (mixer_fd);

  if (mixer_dev < 0 || mixer_dev >= sysinfo.nummixers)
    {
      fprintf (stderr, "Nonexistent mixer device %d\n", mixer_dev);
      exit (-1);
    }

  printf ("Using OSS mixer device %d\n", mixer_dev);

/*
 * The second step is to find the main volume, audio/pcm playback volume and
 * recording level controls.
 *
 * It's important to understand that many mixer devices don't have such
 * controls. This is perfectly normal and the mixer applet must be able to
 * handle this. Aborting or displaying loud error message should be avoided.
 *
 * It's also possible that some mixers have multiple main volume, pcm or
 * record level controls. In such case the application can support all of
 * of them or select just the first one. Having multiple controls means that
 * the device hase multiple sets of speakers or audio devices and each of
 * them has separate volume controls.
 */

  n = mixer_dev;
  if (ioctl (mixer_fd, SNDCTL_MIX_NREXT, &n) == -1)
    {
      perror ("SNDCTL_MIX_NREXT");
      exit (-1);
    }

  for (i = 0; i < n; i++)
    {
      oss_mixext ext;

      ext.dev = mixer_dev;
      ext.ctrl = i;

      if (ioctl (mixer_fd, SNDCTL_MIX_EXTINFO, &ext) == -1)
	{
	  perror ("SNDCTL_MIX_EXTINFO");
	  exit (-1);
	}

/*
 * The MIXF_MAINVOL, MIXF_PCMVOL, MIXF_MONVOL and MIXF_RECVOL flags are used to mark
 * potential main volume, pcm and recording level controls. This makes it
 * possible to implement support for these common types of controls without
 * having to implement fully featured mixer program.
 *
 * Mixer applets using this simplified interface should ignore all mixer
 * controls that don't have any of these three flags. However
 *
 * Note that while mixer controls should have at most one of thse flags defined
 * it may happen that some devices violate this rule. It's up to 
 * application what it does with such controls. Preferably it gets added
 * to all of the bins.
 */

      if (ext.
	  flags & (MIXF_MAINVOL | MIXF_PCMVOL | MIXF_RECVOL | MIXF_MONVOL))
	{
	  printf ("Mixer control %d is ", i);

	  if (ext.flags & MIXF_MAINVOL)
	    {
	      printf ("Mainvol ");

	      ADD_TO_BIN(mainvol, i);
	    }

	  if (ext.flags & MIXF_PCMVOL)
	    {
	      printf ("PCMvol ");

	      ADD_TO_BIN(pcmvol, i);
	    }

	  if (ext.flags & MIXF_RECVOL)
	    {
	      printf ("Recvol ");

	      ADD_TO_BIN(recvol, i);
	    }

	  if (ext.flags & MIXF_MONVOL)
	    {
	      printf ("Monvol ");

	      ADD_TO_BIN(monvol, i);
	    }
/*
 * It is possible that many/most/all mixer controls don't have any of the above
 * flags set. This means that such controls are for expert use only. It is
 * recommended that mixer applets have an [Advanced options] button that is
 * enabled if such controls are found. This button can launch ossxmix (or
 * some configurable program).
 */

	  printf ("%s\n", ext.extname);
	}
    }

/*
 * Now we have selected the mixer controls. Next show their values.
 * Since setting the value is pretty much identical to reading them we don't
 * demonstrate it in this program.
 */
  printf ("\n");

  if (n_mainvol > 0)
    show_controls (mixer_fd, "Main volume controls", mixer_dev, mainvol_ctls, n_mainvol);
  else
    printf ("No main volume control available\n");

  if (n_pcmvol > 0)
    show_controls (mixer_fd, "Pcm volume controls", mixer_dev, pcmvol_ctls, n_pcmvol);
  else
    printf ("No pcm volume control available\n");

  if (n_recvol > 0)
    show_controls (mixer_fd, "Rec volume controls", mixer_dev, recvol_ctls, n_recvol);
  else
    printf ("No rec volume control available\n");

  if (n_monvol > 0)
    show_controls (mixer_fd, "Monitor volume controls", mixer_dev, monvol_ctls, n_monvol);
  else
    printf ("No monitor volume control available\n");

  close (mixer_fd);

  if (n_mainvol + n_pcmvol + n_recvol + n_monvol == 0)
     {
	     printf("\nNo 'simple' mixer controls available for this device\n");
	     printf("It may be a good idea to start 'ossxmix -d %d' which can access advanced options.\n", mixer_dev);
     }

  exit (0);
}
