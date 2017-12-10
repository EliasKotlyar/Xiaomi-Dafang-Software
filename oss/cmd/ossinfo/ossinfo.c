/*
 * Purpose: The ossinfo program that is included in the OSS package.
 *
 * Description:
 * The {!xlink ossinfo} program is a replacement of the old 
 * {!code /dev/sndstat} device file.
 *
 * The program can be used to list all possible information about the sound 
 * devices available in the system. It uses the OSS Device Discovery API
 * introduced in OSS 4.0 (see the "{!link device_discovery}" section of the
 * OSS 4.0 Developer's guide for more info).
 *
 * The best way to find out what this program does is executing 
 * {!shell ossinfo -v8}. Then look at the result and try to figure out
 * how this program gets that information.
 *
 * In short the program calls {!nlink SNDCTL_SYSINFO} to find out
 * how many {!nlink audio}, {!nlink MIDI} and {!nlink mixer} devices there
 * are in the system. Then it uses the {!nlink SNDCTL_AUDIOINFO},
 * {!nlink SNDCTL_MIDIINFO} and {!nlink SNDCTL_MIXERINFO} calls to obtain
 * device information for all devices in the system.
 */

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

#ifndef LOCAL_BUILD
#include <local_config.h>
#endif
#include <soundcard.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/utsname.h>
#include <sys/ioctl.h>

static int mixerfd = -1, phys_only = 0, show_engines = 0, show_ex = 0, verbose = 0;
static oss_sysinfo sysinfo;

static void
print_symlink(const char *link)
{
	char devname[256];
	ssize_t len;

	if ((len = readlink(link, devname, sizeof(devname)-1)) != -1)
	  {
	     devname[len] = '\0';
	     printf("  %s -> %s\n", link, devname);
	  }
}

/*
 * Display filters for selecting what to print.
 */
static unsigned int action_mask = 0;
#define ACT_GLOBAL			0x00000001
#define ACT_AUDIO			0x00000002
#define ACT_MIDI			0x00000004
#define ACT_MIXER			0x00000008
#define ACT_CARDS			0x00000010

/*
 * The print_global_info() routine prints the information returned by the
 * {!nlink SNDCTL_SYSINFO} call (in the main program).
 */
static void
print_global_info (void)
{
  struct utsname un;

  printf ("Version info: %s %s (0x%08X) %s\n",
	  sysinfo.product, sysinfo.version, sysinfo.versionnum,
	  sysinfo.license);
  if (sysinfo.revision_info[0] != 0)
	  printf ("Hg revision: %s\n", sysinfo.revision_info);
  if (uname (&un) != -1)
    printf ("Platform: %s/%s %s %s (%s)\n", un.sysname, un.machine,
	    un.release, un.version, un.nodename);
  printf ("\n");

  printf ("Number of audio devices:	%d\n", sysinfo.numaudios);
  printf ("Number of audio engines:	%d\n", sysinfo.numaudioengines);
#ifdef CONFIG_OSS_MIDI
  printf ("Number of MIDI devices:		%d\n", sysinfo.nummidis);
#endif
  printf ("Number of mixer devices:	%d\n", sysinfo.nummixers);
  printf ("\n");
}

/*
 * The print_verbose_formats() routine print's verbose descriptions of all the
 * formats that are reported in the {!code mask} parameter. The mask can
 * be obtained by calling {!nlink SNDCTL_AUDIOINFO} or 
 * {!nlink SNDCTL_DSP_GETFMTS}.
 */

static void
print_verbose_formats (unsigned long mask)
{
  if (mask & AFMT_MU_LAW)
    printf ("      AFMT_MU_LAW\t\t- mu-Law encoded\n");
  if (mask & AFMT_A_LAW)
    printf ("      AFMT_A_LAW\t\t- A-Law encoded\n");
  if (mask & AFMT_IMA_ADPCM)
    printf ("      AFMT_IMA_ADPCM\t- IMA ADPCM encoded\n");
  if (mask & AFMT_U8)
    printf ("      AFMT_U8\t\t- 8 bit unsigned\n");
  if (mask & AFMT_S16_LE)
    printf ("      AFMT_S16_LE\t- 16 bit signed little endian\n");
  if (mask & AFMT_S16_BE)
    printf ("      AFMT_S16_BE\t- 16 bit signed big endian\n");
  if (mask & AFMT_S8)
    printf ("      AFMT_S8\t\t- 8 bit signed\n");
  if (mask & AFMT_U16_LE)
    printf ("      AFMT_U16_LE\t- 16 bit unsigned little endian\n");
  if (mask & AFMT_U16_BE)
    printf ("      AFMT_U16_BE\t- 16 bit unsigned big endian\n");
  if (mask & AFMT_MPEG)
    printf ("      AFMT_MPEG\t- MPEG (MP2/MP3) encoded audio\n");
  if (mask & AFMT_AC3)
    printf ("      AFMT_AC3\t\t- AC3 (Dolby Digital) encoded audio\n");
  if (mask & AFMT_VORBIS)
    printf ("      AFMT_VORBIS\t- Vorbis encoded audio\n");
  if (mask & AFMT_S32_LE)
    printf ("      AFMT_S32_LE\t- 32 bit signed little endian\n");
  if (mask & AFMT_S32_BE)
    printf ("      AFMT_S32_BE\t- 32 bit signed big endian\n");
  if (mask & AFMT_FLOAT)
    printf
      ("      AFMT_FLOAT\t- Single precision floating point (native endianess)\n");
  if (mask & AFMT_S24_LE)
    printf ("      AFMT_S24_LE\t- 24/32 bit signed little endian\n");
  if (mask & AFMT_S24_BE)
    printf ("      AFMT_S24_BE\t- 24/32 bit signed big endian\n");
  if (mask & AFMT_S24_PACKED)
    printf ("      AFMT_S24_PACKED\t- 24 bit packed (3 byte)\n");
  if (mask & AFMT_SPDIF_RAW)
    printf ("      AFMT_SPDIF_RAW\t- Raw S/PDIF frames\n");
}

static void
print_engine_info (oss_audioinfo * ainfo, int num)
{
  const char * lbl = "Engine    ";

  if (!(ainfo->caps & DSP_CAP_INPUT))
    lbl = "Out engine";
  else if (!(ainfo->caps & DSP_CAP_OUTPUT))
    lbl = "In engine ";

  printf ("      %s %2d: ", lbl, num);

  if (verbose > 1)
    printf ("%d/%s\n                     ", ainfo->dev, ainfo->name);

  switch (ainfo->busy)
    {
    case OSS_OPEN_READ:
      printf ("Busy (IN) ");
      break;
    case OSS_OPEN_WRITE:
      printf ("Busy (OUT) ");
      break;
    case OSS_OPEN_READWRITE:
      printf ("Busy (IN/OUT) ");
      break;
    default:
      printf ("Available for use ");
    }

  if (ainfo->pid > 0)
    printf ("by PID %d / %s ", ainfo->pid, ainfo->cmd);
  if (*ainfo->song_name)
    printf ("songname '%s' ", ainfo->song_name);
  if (*ainfo->label)
    printf ("label '%s' ", ainfo->label);
  printf ("\n");
}

/*
 * The print_audio_info() routine prints all possible information available
 * for an audio device. The verbose level defines what to print.
 */

static void
print_audio_info (void)
{
  int i;
  int n = sysinfo.numaudios;
  unsigned int cmd = SNDCTL_AUDIOINFO;

  if (show_engines)
    {
      n = sysinfo.numaudioengines;
      cmd = SNDCTL_ENGINEINFO;
      printf ("\nAudio engines\n");
    }
  else
    {
      if (show_ex)
	cmd = SNDCTL_AUDIOINFO_EX;
      printf ("\nAudio devices\n");
    }

  for (i = 0; i < n; i++)
    {
      oss_audioinfo ainfo;
      int acc;
      memset (&ainfo, 0, sizeof (ainfo));

      ainfo.dev = i;
      if (ioctl (mixerfd, cmd, &ainfo) == -1)
	{
	  int e = errno;

	  printf ("Can't get device info for /dev/dsp%d (SNDCTL_AUDIOINFO)\n",
		  i);
	  printf ("errno = %d: %s\n\n", e, strerror (e));
	  continue;
	}

      if (phys_only)
	if (ainfo.caps & PCM_CAP_VIRTUAL)
	  continue;

      if (show_engines)
	{
	  printf ("%02d: ", i);
	  if (!ainfo.enabled)
	    printf ("(");
	  printf ("%s", ainfo.name);
	  if (!ainfo.enabled)
	    printf (")");
	  printf(" ");
	  printf ("(device file %s)\n", ainfo.devnode);
	}
      else
	{
	  if (!ainfo.enabled)
	    printf ("(");
	  printf ("%-32s  ", ainfo.name);
	  printf ("%s ", ainfo.devnode);
	  if (!ainfo.enabled)
	    printf (")");
	  printf (" (device index %d)\n", i);
	}

      if (verbose == 0)
	continue;

      if (ainfo.legacy_device >= 0)
	printf ("    Legacy device /dev/dsp%d\n", ainfo.legacy_device);
      else
	printf ("    Legacy device NONE\n");

      printf ("    Caps: ");
      if (ainfo.caps & PCM_CAP_DUPLEX)
	printf ("DUPLEX ");
      if (ainfo.caps & PCM_CAP_REALTIME)
	printf ("REALTIME ");
      if (ainfo.caps & PCM_CAP_BATCH)
	printf ("BATCH ");
      if (ainfo.caps & PCM_CAP_COPROC)
	printf ("COPROC ");
      if (ainfo.caps & PCM_CAP_TRIGGER)
	printf ("TRIGGER ");
      if (ainfo.caps & PCM_CAP_MMAP)
	printf ("MMAP ");
      if (ainfo.caps & PCM_CAP_MULTI)
	printf ("MULTI ");
      if (ainfo.caps & PCM_CAP_BIND)
	printf ("BIND ");
      if (ainfo.caps & PCM_CAP_VIRTUAL)
	printf ("VIRTUAL ");
      if (ainfo.caps & PCM_CAP_SHADOW)
	printf ("SHADOW ");
      if (ainfo.caps & PCM_CAP_HIDDEN)
	printf ("HIDDEN ");
      printf ("\n");
      printf ("    Modes: ");

      /* Check if the device supports input and/or output */
      acc = ainfo.caps & (PCM_CAP_INPUT | PCM_CAP_OUTPUT);

      switch (acc)
	{
	case PCM_CAP_INPUT:
	  printf ("INPUT  ");
	  break;
	case PCM_CAP_OUTPUT:
	  printf ("OUTPUT ");
	  break;
	case PCM_CAP_INPUT | PCM_CAP_OUTPUT:
	  printf ("IN/OUT ");
	  break;
	}
      printf ("\n");

      /* Check if the device is currently busy or not */
      if (!ainfo.enabled)
	{
	  /*
	   * The device is disconnected for some reason. For example an
	   * USB or FireWire device can be powered off or unplugged.
	   */
	  printf ("   ******* DEVICE NOT PLUGGED IN *******\n");
	}
      else
	{
	  oss_audioinfo ai;
	  int next, n;

	  ai.dev = i;
	  ioctl (mixerfd, SNDCTL_AUDIOINFO_EX, &ai);
	  print_engine_info (&ai, 1);
	  n = 2;

/*
 * Show all playback/duplex engines
 */
	  next = ainfo.next_play_engine;
	  if (!show_engines)
	    while (next > 0)
	      {
		ai.dev = next;
		if (ioctl (mixerfd, SNDCTL_ENGINEINFO, &ai) == -1)
		  {
		    int e = errno;

		    printf
		      ("Can't get device info for /dev/dsp%d (SNDCTL_AUDIOINFO)\n",
		       i);
		    printf ("errno = %d: %s\n\n", e, strerror (e));
		    continue;
		  }

		print_engine_info (&ai, n++);

		next = ai.next_play_engine;
	      }

/*
 * Show all recording engines
 */
	  next = ainfo.next_rec_engine;
	  if (!show_engines)
	    if (ainfo.next_rec_engine != ainfo.next_play_engine)
	      while (next > 0)
		{
		  ai.dev = next;
		  if (ioctl (mixerfd, SNDCTL_ENGINEINFO, &ai) == -1)
		    {
		      int e = errno;

		      printf
			("Can't get device info for /dev/dsp%d (SNDCTL_AUDIOINFO)\n",
			 i);
		      printf ("errno = %d: %s\n\n", e, strerror (e));
		      continue;
		    }

		  print_engine_info (&ai, n++);

		  next = ai.next_play_engine;
		}
	}

      if (verbose < 2)
	continue;

      if (verbose > 2)
	{
	  printf ("    Input formats (0x%08x):\n", ainfo.iformats);
	  print_verbose_formats (ainfo.iformats);
	  printf ("    Output formats (0x%08x):\n", ainfo.oformats);
	  print_verbose_formats (ainfo.oformats);
	}
      else
	printf ("    Formats: 0x%x in, 0x%x out\n", ainfo.iformats,
		ainfo.oformats);
      printf ("    Device handle: %s\n", ainfo.handle);
      printf ("    Related mixer dev: %d\n", ainfo.mixer_dev);
      printf ("    Sample rate source: %d\n", ainfo.rate_source);


      printf ("    Preferred channel configuration: ");
      switch (ainfo.caps & DSP_CH_MASK)
	{
	case DSP_CH_ANY:
	  printf ("Not indicated\n");
	  break;
	case DSP_CH_MONO:
	  printf ("MONO\n");
	  break;
	case DSP_CH_STEREO:
	  printf ("STEREO\n");
	  break;
	case DSP_CH_MULTI:
	  printf ("MULTICH\n");
	  break;
	}

      printf ("    Supported number of channels (min - max): %d - %d\n",
	      ainfo.min_channels, ainfo.max_channels);
      printf ("    Native sample rates (min - max): %d - %d",
	      ainfo.min_rate, ainfo.max_rate);
      if (ainfo.nrates > 0)
	{
	  unsigned int j;
	  printf (" (");
	  for (j = 0; j < ainfo.nrates; j++)
	    {
	      if (j > 0)
		printf (",");
	      printf ("%d", ainfo.rates[j]);
	    }
	  printf (")");
	}
      printf ("\n");

      printf ("    HW Type: ");
      if (ainfo.caps & PCM_CAP_ANALOGOUT)
	printf ("ANALOG_OUT ");
      if (ainfo.caps & PCM_CAP_ANALOGIN)
	printf ("ANALOG_IN ");
      if (ainfo.caps & PCM_CAP_DIGITALOUT)
	printf ("DIGITAL_OUT ");
      if (ainfo.caps & PCM_CAP_DIGITALIN)
	printf ("DIGITAL_IN ");

      if (!
	  (ainfo.
	   caps & (PCM_CAP_ANALOGOUT | PCM_CAP_ANALOGIN | PCM_CAP_DIGITALOUT |
		   PCM_CAP_DIGITALIN)))
	printf ("Not indicated.\n");
      printf ("    Minimum latency: ");
      if (ainfo.latency == -1)
	printf ("Not indicated");
      else
	printf ("%d usec", ainfo.latency);
      printf ("\n");

      printf ("\n");
    }

  printf ("\nNodes\n");
  print_symlink ("/dev/dsp");
  print_symlink ("/dev/dsp_in");
  print_symlink ("/dev/dsp_out");
  print_symlink ("/dev/dsp_ac3");
  print_symlink ("/dev/dsp_mmap");
  print_symlink ("/dev/dsp_multich");
  print_symlink ("/dev/dsp_spdifout");
  print_symlink ("/dev/dsp_spdifin");
  print_symlink ("/dev/mixer");
  print_symlink ("/dev/sndstat");
}


#ifdef CONFIG_OSS_MIDI
/*
 * The print_midi_info() routine prints all possible information available
 * for a MIDI device. The verbose level defines what to print.
 */

static void
print_midi_info (void)
{
  int i;

  printf ("MIDI devices (/dev/midi*)\n");
  for (i = 0; i < sysinfo.nummidis; i++)
    {
      oss_midi_info minfo;
      int acc;

      minfo.dev = i;
      if (ioctl (mixerfd, SNDCTL_MIDIINFO, &minfo) == -1)
	{
	  perror ("SNDCTL_MIDIINFO");
	  exit (-1);
	}

      if (phys_only)
	if (minfo.caps & MIDI_CAP_VIRTUAL)
	  continue;

      printf ("%d: ", i);
      printf ("%s ", minfo.name);
      printf ("(MIDI port %d of device object %d)\n", minfo.port_number,
	      minfo.card_number);

      if (verbose == 0)
	continue;

      printf ("    Device file %s, Legacy device /dev/midi%02d\n",
	      minfo.devnode, minfo.legacy_device);
      printf ("    Modes: ");

      /* Check if the device supports input and/or output */
      acc = minfo.caps & MIDI_CAP_INOUT;

      switch (acc)
	{
	case MIDI_CAP_INPUT:
	  printf ("INPUT  ");
	  break;
	case MIDI_CAP_OUTPUT:
	  printf ("OUTPUT ");
	  break;
	case MIDI_CAP_INOUT:
	  printf ("IN/OUT ");
	  break;
	}
      printf (", ");

      /* Check if the device is currently busy or not */
      if (!minfo.enabled)
	{
	  /*
	   * The device is disconnected for some reason. For example an
	   * USB or FireWire device can be powered off or unplugged.
	   */
	  printf ("   ******* DEVICE NOT PLUGGED IN *******\n");
	}
      else
	{
	  switch (minfo.busy)
	    {
	    case OSS_OPEN_READ:
	      printf ("Busy (IN) ");
	      break;
	    case OSS_OPEN_WRITE:
	      printf ("Busy (OUT) ");
	      break;
	    case OSS_OPEN_READWRITE:
	      printf ("Busy (IN/OUT) ");
	      break;
	    default:
	      printf ("Available for use ");
	    }

	  if (minfo.pid > 0)
	    printf ("by PID %d / %s ", minfo.pid, minfo.cmd);
	  if (*minfo.song_name != 0)
	    printf ("(%s) ", minfo.song_name);
	  if (*minfo.label != 0)
	    printf ("(label=%s) ", minfo.label);
	  printf ("\n");
	}

      if (verbose < 2)
	continue;
      printf ("    Caps: ");
      if (minfo.caps & MIDI_CAP_VIRTUAL)
	printf ("VIRTUAL ");
      if (minfo.caps & MIDI_CAP_CLIENT)
	printf ("CLIENT ");
      if (minfo.caps & MIDI_CAP_SERVER)
	printf ("SERVER ");
      if (minfo.caps & MIDI_CAP_INTERNAL)
	printf ("INTERNAL ");
      if (minfo.caps & MIDI_CAP_EXTERNAL)
	printf ("EXTERNAL ");
      if (minfo.caps & MIDI_CAP_PTOP)
	printf ("PTOP ");
      if (minfo.caps & MIDI_CAP_MTC)
	printf ("MTC ");
      printf ("\n");
      printf ("    Minimum latency: ");
      if (minfo.latency == -1)
	printf ("Not indicated");
      else
	printf ("%d usec", minfo.latency);
      printf ("\n");

      printf ("    Device handle: %s\n", minfo.handle);
      printf ("\n");
    }
}
#endif

/*
 * The print_mixer_info() routine prints all possible information available
 * for a mixer device. The verbose level defines what to print.
 */

static void
print_mixer_info (void)
{
  int i;

  printf ("\nMixer devices\n");
  for (i = 0; i < sysinfo.nummixers; i++)
    {
      oss_mixerinfo minfo;

      minfo.dev = i;
      if (ioctl (mixerfd, SNDCTL_MIXERINFO, &minfo) == -1)
	{
	  if (errno == ENXIO || errno == ENODEV)
	    {
	      printf ("%2d: Device not available\n", i);
	      continue;
	    }

	  perror ("SNDCTL_MIXERINFO");
	  exit (-1);
	}

      if (phys_only)
	if (minfo.caps & MIXER_CAP_VIRTUAL)
	  continue;

      printf ("%2d: ", i);

      if (!minfo.enabled)
	printf ("(");
      printf ("%s ", minfo.name);
      if (!minfo.enabled)
	printf (")");
      printf ("(Mixer %d of device object %d)\n", minfo.port_number,
	      minfo.card_number);

      if (verbose == 0)
	continue;

      printf ("    Device file %s, Legacy device /dev/mixer%d\n",
	      minfo.devnode, minfo.legacy_device);

      /* Check if the device is currently busy or not */
      if (!minfo.enabled)
	{
	  /*
	   * The device is disconnected for some reason. For example an
	   * USB or FireWire device can be powered off or unplugged.
	   */
	  printf ("   ******* DEVICE NOT PLUGGED IN *******\n");
	}

      if (verbose < 1)
	continue;
      printf ("    Priority: %d\n", minfo.priority);
      printf ("    Caps: ");
      if (minfo.caps & MIXER_CAP_VIRTUAL)
	printf ("VIRTUAL ");
      if (minfo.caps & MIXER_CAP_LAYOUT_B)
	printf ("LAYOUT_B ");
      if (minfo.caps & MIXER_CAP_NARROW)
	printf ("NARROW ");
      printf ("\n");
      if (verbose < 2)
	continue;

      printf ("    Device handle: %s\n", minfo.handle);
      printf ("    Device priority: %d\n", minfo.priority);
      printf ("\n");
    }
}

/*
 * Helper function for printing HW information.
 */
static void
print_hwinfo (char *hw_info)
{
  while (*hw_info != 0)
    {
      char *s = hw_info;

      while (*s && *s != '\n')
	s++;

      if (*s != 0)
	*s++ = 0;

      printf ("    %s\n", hw_info);
      hw_info = s;
    }
}

/*
 * Print list of device objects/cards
 */

static void
print_card_info (void)
{
  int i;

  printf ("\nDevice objects\n");
  for (i = 0; i < sysinfo.numcards; i++)
    {
      oss_card_info cinfo;

      cinfo.card = i;
      if (ioctl (mixerfd, SNDCTL_CARDINFO, &cinfo) == -1)
	{
	  perror ("SNDCTL_CARDINFO");
	  exit (-1);
	}

      printf ("%2d: %s %s", i, cinfo.shortname, cinfo.longname);
      if (cinfo.intr_count > 0)
	printf (" interrupts=%d (%d)", cinfo.ack_count, cinfo.intr_count);
      printf ("\n");

      if (*cinfo.hw_info != 0)
	print_hwinfo (cinfo.hw_info);
    }
  printf ("\n");
}

static void
usage (char *progname)
{
  printf ("Usage: %s [options]\n", progname);
  printf ("\n");
  printf ("Options:\n");
  printf ("\t-h\t\tHelp (this message)\n");
  printf ("\t-g\t\tPrint global info\n");
  printf ("\t-a\t\tPrint audio devicefile info\n");
  printf ("\t-A\t\tPrint audio devicefile info (for O_EXCL applications)\n");
  printf ("\t-e\t\tPrint audio engine info\n");
  printf ("\t-x\t\tPrint mixer info\n");
#ifdef CONFIG_OSS_MIDI
  printf ("\t-m\t\tPrint MIDI info\n");
#endif
  printf ("\t-d\t\tPrint device object (\"card\")list\n");
  printf ("\t-v[0-9]\t\tSet verbose level\n");
}

static void
bad_usage (char *progname)
{
  fprintf (stderr, "Bad usage \n");
  usage (progname);
  exit (-1);
}

int
main (int argc, char *argv[])
{
  const char * devmixer;
  int i;

  if ((devmixer = getenv("OSS_MIXERDEV")) == NULL)
     devmixer = "/dev/mixer";

  if ((mixerfd = open (devmixer, O_RDWR, 0)) == -1)
    {
      switch (errno)
	{
	case ENXIO:
	case ENODEV:
	  fprintf (stderr,
		   "Open Sound System is not running in your system.\n");
	  break;

	case ENOENT:
	  fprintf (stderr,
		   "No %s device available in your system.\n", devmixer);
	  fprintf (stderr,
		   "Perhaps Open Sound System is not installed or running.\n");
	  break;

	default:
	  perror (devmixer);
	}
      exit (-1);
    }

  if (ioctl (mixerfd, SNDCTL_SYSINFO, &sysinfo) == -1)
    {
      if (errno == ENXIO)
	{
	  fprintf (stderr,
		   "OSS has not detected any supported sound hardware ");
	  fprintf (stderr, "in your system.\n");
	  exit (-1);
	}
      else
	{
	  perror ("SNDCTL_SYSINFO");
	  if (errno == EINVAL)
	    fprintf (stderr, "Error: OSS version 4.0 or later is required\n");
	  exit (-1);
	}
    }

/*
 * Simple and brutal command line argument parsing. We do not use
 * getopt() because it causes dependency problems in some environments.
 */

  for (i = 1; i < argc; i++)
    if (argv[i][0] == '-')
      {
	switch (argv[i][1])
	  {
	  case 'v':
	    if (argv[i][2])
	      verbose = atoi (&argv[i][2]);
	    else
	      verbose++;
	    break;
	  case 'p':
	    phys_only = 1;
	    break;
	  case 'g':
	    action_mask |= ACT_GLOBAL;
	    break;
	  case 'a':
	    action_mask |= ACT_AUDIO;
	    break;
	  case 'A':
	    action_mask |= ACT_AUDIO;
	    show_ex = 1;
	    break;
	  case 'e':
	    action_mask |= ACT_AUDIO;
	    show_engines = 1;
	    break;
	  case 'm':
#ifdef CONFIG_OSS_MIDI
	    action_mask |= ACT_MIDI;
#else
	    fprintf (stderr, "Note! MIDI support is available in the current version of OSS so -m has no effect.\n");
#endif
	    break;
	  case 'x':
	    action_mask |= ACT_MIXER;
	    break;
	  case 'd':
	    action_mask |= ACT_CARDS;
	    break;
	  case 'h':
	    usage (argv[0]);
	    exit (0);
	    break;
	  default:
	    bad_usage (argv[0]);
	  }
      }
    else
      bad_usage (argv[0]);

/*
 * Finally print the information that was requested. By default
 * all device types will be shown.
 */
  if (action_mask == 0)
    action_mask = 0xffffffff;

  if (action_mask & ACT_GLOBAL)
    print_global_info ();
  if (action_mask & ACT_CARDS)
    print_card_info ();
#ifdef CONFIG_OSS_MIDI
  if (action_mask & ACT_MIDI)
    print_midi_info ();
#endif
  if (action_mask & ACT_MIXER)
    print_mixer_info ();
  if (action_mask & ACT_AUDIO)
    print_audio_info ();

  close (mixerfd);
  return 0;
}
