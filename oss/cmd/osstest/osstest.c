/*
 * Purpose: The osstest program shipped with OSS
 * Description:
 *
 * This file contains the main parts of the {!xlink osstest} utility that is
 * shipped with the OSS package.
 *
 * {!notice The sample rate converter (GRC3) required by this program
 * is not released by 4Front Technologies. For this reason it will not be
 * possible to compile this program. The sources have been made
 * available just because they use some of the new OSS features such as 
 * {!nlink SNDCTL_AUDIOINFO}.  However the {!nlink ossinfo.c} program might 
 * be more interesting sample source for this subject.}
 *
 * It's rather easy to get this program to work without GRC3 by
 * commenting out the contents of the src_convert routine and by 
 * modifying the Makefile. However there should be no reason to do this
 * since the precompiled program with full functionality is available for
 * free with OSS.
 *
 * The wavedata.c and wavedata.h files contain the actual samples compressed
 * using the MS ASPCM algorithm.
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

#define CONFIGURE_C
#define OSSTEST
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <soundcard.h>

/*
 * Channel selectors
 */
#define CH_MONO		0
#define CH_LEFT		1
#define CH_RIGHT	2
#define CH_STEREO	(CH_LEFT|CH_RIGHT)

#ifdef SRC_SUPPORT
#include <inttypes.h>
#define __inline__	inline
#include "../../kernel/framework/audio/oss_grc3.c"
#endif

/*
 * uncompress_wave() is defined in wavedata.c. It expands the audio samples
 * stored in wavedata.h and returns the lenghth of the uncompressed version
 * in bytes.
 *
 * The uncompressed wave data format is 16 bit (native) stereo recorded at 48000 Hz.
 */
extern int uncompress_wave (short *outbuf);

#define SHORTER_TEST 0		/* SET THIS TO 1 if you want SHORTER TEST */

static int data_len;

#define MAXDEVICE   64
extern void describe_error (void);	/* From ../dsp/help.c */

#define SAMPLE_RATE 48000

/*
 * Operating mode flags (set from the command line).
 */
#define TF_VIRTUAL	0x00000001	/* Test virtual devices */
#define TF_SYSTEST	0x00000002	/* Test started by oss-install */
#define TF_SNDCONF	0x00000004	/* Test started by sndconf */
#define TF_QUICK	0x00000008	/* Shortened test */
#define TF_LOOP		0x00000010	/* Loop until interrupted */

int cardno = -1, mixerfd, num_devices_tested = 0, play_gain = 100, skip = 0;

static short *sample_buf;

void
prepare (void)
{
  if ((sample_buf = malloc (2000000)) == NULL)
    {
      fprintf (stderr, "Out of memory\n");
      exit (-1);
    }

  data_len = uncompress_wave (sample_buf);
}

#ifdef SRC_SUPPORT
/*
 * The src_convert() routine converts the wave data to the requested
 * sampling rate.
 */
static int
src_convert (short *buf, short *newbuf, int count, int srate, int sz)
{
  int newcount = 0, c, p = 0, np = 0;

  grc3state_t grc1, grc2;

  grc3_reset (&grc1);
  grc3_reset (&grc2);

  grc3_setup (&grc1, SAMPLE_RATE, srate);
  grc3_setup (&grc2, SAMPLE_RATE, srate);

  count /= 2;
  sz /= 2;

  while (newcount < count)
    {
      int n;
      c = count - newcount;
      if (c > 1024)
	c = 1024;
      grc3_convert (&grc1, 16, 4, buf + p, newbuf + np, c / 2, 2048, 2, 0);
      n =
	grc3_convert (&grc2, 16, 4, buf + p, newbuf + np, c / 2, 2048, 2, 1);
      newcount += c;
      p += c;
      np += n * 2;
    }

  return np * 2;
}
#endif

/*
 * audio_write() writes the requested audio channels of the original stereo
 * recording(buf). This is done simply by setting the unnecessary
 * channel (if any) to 0.
 */

static int
audio_write (int fd, short *buf, int count, int chmask)
{
  short *buf2;
  int count2, l, ret, i;

  if (chmask == CH_STEREO)
    return write (fd, buf, count);

  l = count / 4;
  count2 = count;

  switch (chmask)
    {
    case CH_LEFT:
      buf2 = malloc (count);
      memcpy (buf2, buf, count);
      for (i = 0; i < l; i++)
	buf2[i * 2 + 1] = 0;
      break;

    case CH_RIGHT:
      buf2 = malloc (count);
      memcpy (buf2, buf, count);
      for (i = 0; i < l; i++)
	buf2[i * 2] = 0;
      break;

    default:
      abort ();
    }

  ret = write (fd, buf2, count2);
  free (buf2);

  return ret;
}

/*
 * The testdsp() routine checks the capabilities of a given audio device number
 * (parameter n) and decides if the test sound needs to be played.
 */

/*ARGSUSED*/
int
testdsp (char *devnode, int n, int flags)
{
  float ratio;
  struct timeval t1, t2;
  unsigned long t;
  int sample_rate;
  int hd, delay;
  int test_bytes;
  int open_flags = O_WRONLY;
  long long total_bytes = 0;
  unsigned int tmp, caps;

  short *test_data, *tmp_buf = NULL;

/*
 * Use {!nlink O_EXCL} to bypass virtual mixing and to access the actual
 * hardware device directly. Note that we also have to use
 * {!nlink SNDCTL_AUDIOINFO_EX} instead of usual SNDCTL_AUDIOINFO since it
 * returns information that is valid when the device is opened with 
 * O_EXCL.
 *
 * If the device is busy we will try to open it without O_EXCL. 
 */

/*
 * If the -V option was set then don't use O_EXCL.
 */
  if (!(flags & TF_VIRTUAL))
     open_flags |= O_EXCL;

  hd = open (devnode, open_flags, 0);
  if (hd == -1 && errno == EBUSY)
    {
      /*
       * Try without O_EXCL which enables virtual mixing. In this way the device
       * can almost certainly be opened. However the results may be different
       * than when the device is used directly.
       */

      hd = open (devnode, O_WRONLY, 0);
    }

  if (hd == -1)
    {
      int err = errno;
      perror (devnode);
      errno = err;
      describe_error ();
      printf ("Can't open the device\n");
      return 0;
    }

  caps = 0;
  if (ioctl (hd, SNDCTL_DSP_GETCAPS, &caps) == -1)
    {
      perror ("SNDCTL_DSP_GETCAPS");
      printf ("Couldn't get the device capabilities\n");
      close (hd);
      return -1;
    }

  test_bytes = ((SHORTER_TEST) ? 2000 : data_len);

/*
 * Setup the sample format. Since OSS will support {!nlink AFMT_S16_NE} regardless
 * of the device we do not need to support any other formats.
 */

  tmp = AFMT_S16_NE;
  if (ioctl (hd, SNDCTL_DSP_SETFMT, &tmp) == -1 || tmp != AFMT_S16_NE)
    {
      close (hd);
      printf ("Device doesn't support the native 16 bit sample format (%x)\n",
	      tmp);
      return -1;
    }

/*
 * Setup the device for stereo playback. Once again we can simply assume that
 * stereo will always work before OSS takes care of this by emulation if
 * necessary.
 */
  tmp = 2;
  if (ioctl (hd, SNDCTL_DSP_CHANNELS, &tmp) == -1 || tmp != 2)
    {
      close (hd);
      printf ("The device doesn't support stereo (%d)\n", tmp);
      return -2;
    }

/*
 * Set up the sample rate. Convrt the sample rate if necessary.
 * Note that actually OSS will handle any sample rates by doing the
 * required conversions on fly. However it's possible that some professional
 * audio devices are configured so that the sample rate conversions are
 * not permitted. This is unusual but we wanted the osstest utility to 
 * work OK even in these cases. This is not necessary in ordinary
 * applications.
 */

  tmp = SAMPLE_RATE;
  if (ioctl (hd, SNDCTL_DSP_SPEED, &tmp) == -1)
    {
      close (hd);
      perror ("Set speed");
      return -3;
    }

  sample_rate = tmp;
  if (sample_rate != SAMPLE_RATE)
    {
#ifdef SRC_SUPPORT
/*
 * We need to do the sample rate conversion because the device
 * is configured not to do it.
 */
      int sz, a, b;

      printf ("sr=%d Hz ", sample_rate);

      a = SAMPLE_RATE / 100;
      b = sample_rate / 100;

      sz = ((test_bytes + 4096) * b) / a;
      tmp_buf = test_data = malloc (sz);
      memset (tmp_buf, 0, sz);

      test_bytes =
	src_convert (sample_buf, test_data, test_bytes, sample_rate, sz);
#else
      printf ("The device doesn't support %d Hz\n", SAMPLE_RATE);
      return -3;
#endif
    }
  else
    test_data = sample_buf;

  if (skip) goto tend;

  printf ("\n");

  /* TF_SNDCONF is used when longer messages should be printed. */
  if (flags & TF_SNDCONF)
    printf ("   Performing left channel test on %s\n", devnode);
  else
    printf ("  <left> ");
  fflush (stdout);

/*
 * This program will measure the real sampling rate by computing the
 * total time required to play the sample.
 *
 * This is not terribly presice with short test sounds but it can be used
 * to detect if the sampling rate badly wrong. Errors of few percents
 * is more likely to be caused by poor accuracy of the system clock
 * rather than problems with the sampling rate.
 */
  gettimeofday (&t1, NULL);
  if (audio_write (hd, test_data, test_bytes, CH_LEFT) < 0)
    {
      printf ("Device returned error: %s\n", strerror (errno));
      close (hd);
      return -3;
    }
  total_bytes = test_bytes;
  if (flags & TF_SNDCONF)
    printf ("    Test completed OK\n");
  else
    printf ("OK ");

  if (skip) goto tend;

  if (flags & TF_SNDCONF)
    printf ("   Performing right channel test on %s\n", devnode);
  else
    printf ("<right> ");
  fflush (stdout);
  if (audio_write (hd, test_data, test_bytes, CH_RIGHT) < 0)
    {
      printf ("Device returned error: %s\n", strerror (errno));
      close (hd);
      return -3;
    }
  total_bytes += test_bytes;
  if (flags & TF_SNDCONF)
    printf ("    Test completed OK\n");
  else
    printf ("OK ");

  if (skip) goto tend;

  if (flags & TF_SNDCONF)
    printf ("   Performing stereo test on %s\n", devnode);
  else
    printf ("<stereo> ");
  fflush (stdout);
  if (audio_write (hd, test_data, test_bytes, CH_STEREO) < 0)
    {
      printf ("Device returned error: %s\n", strerror (errno));
      close (hd);
      return -3;
    }
  total_bytes += test_bytes;
  gettimeofday (&t2, NULL);
  delay = 0;
  ioctl (hd, SNDCTL_DSP_GETODELAY, &delay);	/* Ignore errors */

/*
 * Perform the time computations using milliseconds.
 */

  t = t2.tv_sec - t1.tv_sec;
  t *= 1000;

  t += t2.tv_usec / 1000;
  t -= t1.tv_usec / 1000;

  total_bytes -= delay;
  total_bytes *= 1000;

  total_bytes /= t;
  total_bytes /= 4;

  ratio = ((float) total_bytes / (float) sample_rate) * 100.0;
  if (flags & TF_SNDCONF)
    printf
      ("    Test completed OK.\n    Sample rate was %8.2f Hz (%4.2f%%)\n",
       (float) sample_rate * ratio / 100.0, ratio - 100.0);
  else
    printf ("OK <measured srate %8.2f Hz (%4.2f%%)> ",
	    (float) sample_rate * ratio / 100.0, ratio - 100.0);
tend:
  skip = 0;
  printf ("\n");
  num_devices_tested++;

  close (hd);
  if (tmp_buf != NULL)
    free (tmp_buf);
  return 1;
}

static int
find_num_devices (void)
{
  oss_sysinfo info;
  struct utsname un;
/*
 * Find out the number of available audio devices by calling SNDCTL_SYSINFO.
 */

  if (ioctl (mixerfd, SNDCTL_SYSINFO, &info) == -1)
    {
      if (errno == ENXIO)
	{
	  fprintf (stderr,
		   "OSS has not detected any supported sound hardware\n");
	  fprintf (stderr, "in your system.\n");
	  exit (-1);
	}
      else
	{
	  printf ("SNDCTL_SYSINFO failed: %s\n", strerror (errno));
	  printf
	    ("Cannot get system information. Perhaps you are not running OSS 4.x\nbut some slightly incompatible sound subsystem.\n");
	  exit (-1);
	}
    }
  printf ("Sound subsystem and version: %s %s (0x%08X)\n",
	  info.product, info.version, info.versionnum);

  if (uname (&un) != -1)
    printf ("Platform: %s/%s %s %s\n", un.sysname, un.machine, un.release,
	    un.version);

  return info.numaudios;
}

/*
 * The test_device() routine checks certain information about the device
 * and calls testdsp() to play the test sound.
 */

int
test_device (int t, int flags)
{
  oss_audioinfo ainfo;
  int code;

/*
 * Notice! We use {!nlink SNDCTL_AUDIOINFO_EX} because the device is
 * going to be opened with {!nlink O_EXCL}. Practically all other
 * applications should use the normal SNDCTL_AUDIOINFO call instead.
 */
  ainfo.dev = t;
  if (ioctl (mixerfd, SNDCTL_AUDIOINFO_EX, &ainfo) == -1)
    {
      perror ("SNDCTL_AUDIOINFO_EX");
      return 1;
    }

  if (ainfo.card_number != cardno)	/* Switched to a new card */
    {
      printf ("\n*** Scanning sound adapter #%d ***\n", cardno);
    }

  printf ("%s (audio engine %d): %s\n", ainfo.devnode, ainfo.dev, ainfo.name);

  if (!ainfo.enabled)
    {
      printf ("- Device not present - Skipping\n");
      return 1;
    }

  if (!(ainfo.caps & PCM_CAP_OUTPUT))
    {
      printf ("- Skipping input only device\n");
      return 1;
    }

/*
 * By default skip virtual devices except if we have not tested
 * any devices yet.
 */
  if (!(flags & TF_VIRTUAL) && num_devices_tested > 0)
    if (ainfo.caps & PCM_CAP_VIRTUAL)
      {
	printf ("- Skipping virtual device (use -V to force test)\n");
	return 1;
      }

  if ((ainfo.caps & DSP_CH_MASK) == DSP_CH_MULTI)
    {
      printf ("- Skipping multi channel device\n");
      return 1;
    }

  if (ainfo.pid != -1)
    {
      printf ("Note! Device is in use (by PID %d/%s) but will try anyway\n",
	      ainfo.pid, ainfo.cmd);
      /* return 1; */
    }

  if (flags & TF_QUICK)
    if (cardno == ainfo.card_number)
      {
	printf ("- card already tested\n");
	return 1;
      }

  printf ("- Performing audio playback test... ");
  fflush (stdout);

  cardno = ainfo.card_number;

  code = testdsp (ainfo.devnode, t, flags);

  return code == 1;
}

static void
skip_handler (int c)
{
  skip = 1;
}

int
main (int argc, char *argv[])
{
  int t, i;
  int maxdev;
  int flags = 0;
  int status = 0;
  int dev = -1;
  extern int optind;

/*
 * Simple command line switch handling.
 */

  while ((i = getopt (argc, argv, "CVflsg:")) != EOF)
    {
      switch (i)
        {
          case 'V':
            flags |= TF_VIRTUAL;
            break;
          case 's':
            flags |= TF_SYSTEST;
            break;
          case 'C':
            flags |= TF_SNDCONF;
            break;
          case 'f':
            flags |= TF_QUICK;
            break;
	  case 'l':
	    flags |= TF_LOOP;
	    break;
	  case 'g':
	    play_gain = atoi (optarg);
	    break;
          default:
            printf ("Usage: osstest [options...] [device number]\n"
                    "	-V	Test virtual mixer devices as well\n"
                    "	-l	Loop indefinately until interrupted\n"
		    "   -g gain	Set playback gain (0-100). Default 100.\n"
                    "	-f	Faster test\n");
            exit (-1);
        }
    }

   if ((optind < argc) && (sscanf (argv[optind], "%d", &dev) != 1))
      {
        fprintf (stderr, "Bad device number %s\n", argv[optind]);
        exit (-1);
      }

  if (flags & TF_SYSTEST)
    {
      printf ("++++ osstest results ++++\n");
    }

/*
 * Open the mixer device used for calling SNDCTL_SYSINFO and
 * SNDCTL_AUDIOINFO.
 */
  if ((mixerfd = open ("/dev/mixer", O_RDWR, 0)) == -1)
    {
      int err = errno;
      perror ("/dev/mixer");
      errno = err;
      describe_error ();
      exit (-1);
    }

  prepare ();			/* Prepare the wave data */

/*
 * Enumerate all devices and play the test sounds.
 */
  maxdev = find_num_devices ();
  if (maxdev < 1)
    {
      printf ("\n\nNOTICE! You don't have any audio devices available.\n"
	      "        It looks like your audio hardware was not recognized\n"
	      "        by OSS.\n"
	      "	       \n"
	      "        If you have installed OSS just a moment ago then it may be necessary to.\n"
	      "        to rebot the system before trying to use the device(s).\n");
      exit (-1);
    }

#ifdef SIGQUIT
  signal (SIGQUIT, skip_handler);
#endif

  do {
    if (dev > -1)
      {
	if (dev >= maxdev)
  	  {
	    fprintf (stderr, "Bad device number %d\n", dev);
	    exit (-1);
	  }
	if (!test_device (dev, flags))
	  status++;
    }
    else
      for (t = 0; t < maxdev; t++)
	if (!test_device (t, flags))
	  status++;

    if (!(flags & TF_SYSTEST))
      {
	if (status == 0)
	  printf ("\n*** All tests completed OK ***\n");
	else
	  printf ("\n*** Some errors were detected during the tests ***\n");
      }

    cardno = -1;
  } while (flags & TF_LOOP);

  close (mixerfd);

  return status;
}
