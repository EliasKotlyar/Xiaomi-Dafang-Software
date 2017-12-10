/*
 * Purpose: Full duplex sample program using the single device approach.
 *
 * Description:
 * This sample program explains how to use the one and twodevicefile based
 * methods for full duplex.
 *
 * This program uses full duplex for echo-like processing (usefull for
 * applications like guitar effect processors). However this task is
 * actually very challenging. Applications that require almost zero
 * latencies will need to be run on very high priority levels. The exact method
 * required for this depends on the operating system and is beyond the scope
 * of this simplistic sample program.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/select.h>
#include <soundcard.h>

char *dspname = "/dev/dsp";
char *dspname_in = NULL;

int fd_out = -1, fd_in = -1;
char buffer[32 * 1024];		/* Max 32k local buffer */
int rate = 48000;
int fragsize;

static void
open_one_device (char *dspname)
{
/*
 * Open the device file. The one device full duplex scheme requires that
 * the device file is opened with O_RDWR. See the description of the
 * {!nlink open} system call for more info.
 */
  oss_audioinfo ai;
  int fd;
  int tmp;
  int devcaps;
  int channels = 2;
  int format;
  int frag;

  if ((fd = open (dspname, O_RDWR, 0)) == -1)
    {
      perror (dspname);
      exit (-1);
    }

  ai.dev = -1;
  if (ioctl (fd, SNDCTL_ENGINEINFO, &ai) != -1)
    {
      printf ("\nUsing audio engine %d=%s for duplex\n\n", ai.dev, ai.name);
    }

#ifdef USE_RAW_FORMATS
/*
 * In some cases it's recommended that all sample rate and format
 * conversions are disabled. These conversions may make timing very
 * tricky. However the drawback of disabling format conversions is that
 * the application must be able to handle the format and rate
 * conversions itself.
 *
 * We don't do any error checking because SNDCTL_DSP_COOKEDMODE is an optional
 * ioctl call that may not be supported by all OSS implementations (in such
 * cases there are no format conversions anyway).
 */

  tmp = 0;
  ioctl (fd, SNDCTL_DSP_COOKEDMODE, &tmp);
#endif

/*
 * Check that the device supports full duplex. Otherwise there is no point in
 * continuing.
 */

  if (ioctl (fd, SNDCTL_DSP_GETCAPS, &devcaps) == -1)
    {
      perror ("SNDCTL_DSP_GETCAPS");
      exit (-1);
    }

  if (!(devcaps & PCM_CAP_DUPLEX))
    {
      fprintf (stderr,
	       "%s doesn't support one device based full duplex scheme\n",
	       dspname);
      fprintf (stderr, "Please use the two device scheme.\n");
      exit (-1);
    }

#if 0
  /*
   * There is no point in calling SNDCTL_DSP_SETDUPLEX any more. This call has not had any
   * effect since SB16.
   */
  if (ioctl (fd, SNDCTL_DSP_SETDUPLEX, NULL) == -1)
    {
      perror ("SNDCTL_DSP_SETDUPLEX");
      exit (-1);
    }
#endif

/*
 * Try to set the fragment size to suitable level.
 */

  frag = 0x7fff000a;		/* Unlimited number of 1k fragments */

  if (ioctl (fd, SNDCTL_DSP_SETFRAGMENT, &frag) == -1)
    {
      perror ("SNDCTL_DSP_SETFRAGMENT");
      exit (-1);
    }

/*
 * Set up the sampling rate and other sample parameters.
 */

  tmp = channels;

  if (ioctl (fd, SNDCTL_DSP_CHANNELS, &tmp) == -1)
    {
      perror ("SNDCTL_DSP_CHANNELS");
      exit (-1);
    }

  if (tmp != channels)
    {
      fprintf (stderr, "%s doesn't support stereo (%d)\n", dspname, tmp);
      exit (-1);
    }

  /*
   * Request 16 bit native endian sample format.
   */
  tmp = AFMT_S16_NE;

  if (ioctl (fd, SNDCTL_DSP_SETFMT, &tmp) == -1)
    {
      perror ("SNDCTL_DSP_SETFMT");
      exit (-1);
    }

/*
 * Note that most devices support only the usual litle endian (Intel)
 * byte order. This may be a problem under big endian architectures such
 * as Sparc. We accept also the opposite (alien) endianess but
 * this may require different handling (byte swapping) in most applications.
 * However we don't care about this issue because this applicaton doesn't
 * do any processing on the data.
 */

  if (tmp != AFMT_S16_NE && tmp != AFMT_S16_OE)
    {
      fprintf (stderr, "%s doesn't support 16 bit sample format (%x)\n",
	       dspname, tmp);
      exit (-1);
    }

  format = tmp;

  if (format == AFMT_S16_OE)
    {
      fprintf (stderr,
	       "Warning: Using 16 bit sample format with wrong endianess.\n");
    }

  tmp = rate;

  if (ioctl (fd, SNDCTL_DSP_SPEED, &tmp) == -1)
    {
      perror ("SNDCTL_DSP_SPEED");
      exit (-1);
    }

  if (tmp != rate)
    {
      fprintf (stderr, "%s doesn't support requested rate %d (%d)\n", dspname,
	       rate, tmp);
      exit (-1);
    }

/*
 * Get the actual fragment size. SNDCTL_DSP_GETBLKSIZE gives a good
 * compromise. If cooked mode is not disabled then the fragment sizes
 * used for input and output may be different. Using 
 * {!nlink SNDCTL_DSP_GETOSPACE} or {!nlink SNDCTL_DSP_GETISPACE}
 * may return different values. In this case SNDCTL_DSP_GETBLKSIZE will
 * return a value that is between them.
 */

  if (ioctl (fd, SNDCTL_DSP_GETBLKSIZE, &fragsize) == -1)
    {
      perror ("SNDCTL_DSP_GETBLKSIZE");
      exit (-1);
    }

  if (fragsize > sizeof (buffer))
    {
      fprintf (stderr, "Too large fragment size %d\n", fragsize);
      exit (-1);
    }

  printf ("Sample parameters set OK. Using fragment size %d\n", fragsize);

  fd_in = fd_out = fd;
}

static void
open_two_devices (char *dspname_out, char *dspname_in)
{
/*
 * Open the device file. The one device full duplex scheme requires that
 * the device file is opened with O_RDWR. See the description of the
 * {!nlink open} system call for more info.
 */
  oss_audioinfo ai_in, ai_out;
  int tmp;
  int devcaps;
  int channels = 2;
  int format;
  int frag = 0x7fff000a;	/* Unlimited number of 1k fragments */

/*
 * Open the output device
 */
  if ((fd_out = open (dspname_out, O_WRONLY, 0)) == -1)
    {
      perror (dspname_out);
      exit (-1);
    }

  ai_out.dev = -1;
  if (ioctl (fd_out, SNDCTL_ENGINEINFO, &ai_out) != -1)
    {
      printf ("\nUsing audio engine %d=%s for output\n", ai_out.dev,
	      ai_out.name);
    }

/*
 * Open the input device
 */
  if ((fd_in = open (dspname_in, O_RDONLY, 0)) == -1)
    {
      perror (dspname_in);
      exit (-1);
    }

  ai_in.dev = -1;
  if (ioctl (fd_in, SNDCTL_ENGINEINFO, &ai_in) != -1)
    {
      printf ("Using audio engine %d=%s for input\n\n", ai_in.dev,
	      ai_in.name);
    }

  if (ai_in.rate_source != ai_out.rate_source)
    {
/*
 * Input and output devices should have their sampling rates derived from
 * the same crystal clock. Otherwise there will be drift in the sampling rates
 * which may cause dropouts/hiccup (unless the application can handle the rate
 * error). So check that the rate sources are the same.
 *
 * However two devices may still be driven by the same clock even if the 
 * rate sources are different. OSS has no way to know if the user is using some
 * common sample clock (word clock) generator to syncronize all devices
 * together (which is _THE_ practice in production environments). So do not
 * overreact. Just let the user to know about the potential problem.
 */
      fprintf (stderr,
	       "Note! %s and %s are not necessarily driven by the same clock.\n",
	       dspname_out, dspname_in);
    }

#ifdef USE_RAW_FORMATS
/*
 * In many cases it's recommended that all sample rate and format
 * conversions are disabled. These conversions may make timing very
 * tricky. However the drawback of disabling format conversions is that
 * the application must be able to handle the format and rate
 * conversions itself.
 *
 * We don't do any error checking because SNDCTL_DSP_COOKEDMODE is an optional
 * ioctl call that may not be supported by all OSS implementations (in such
 * cases there are no format conversions anyway).
 */

  tmp = 0;
  ioctl (fd_out, SNDCTL_DSP_COOKEDMODE, &tmp);
  tmp = 0;
  ioctl (fd_in, SNDCTL_DSP_COOKEDMODE, &tmp);
#endif

/*
 * Check output device capabilities.
 */
  if (ioctl (fd_out, SNDCTL_DSP_GETCAPS, &devcaps) == -1)
    {
      perror ("SNDCTL_DSP_GETCAPS");
      exit (-1);
    }

  if (devcaps & PCM_CAP_DUPLEX)
    {
      fprintf (stderr,
	       "Device %s supports duplex so you may want to use the single device approach instead\n",
	       dspname_out);
    }

  if (!(devcaps & PCM_CAP_OUTPUT))
    {
      fprintf (stderr, "%s doesn't support output\n", dspname_out);
      fprintf (stderr, "Please use different device.\n");
#if 0
      /*
       * NOTE! Earlier OSS versions don't necessarily support PCM_CAP_OUTPUT
       *       so don't panic.
       */
      exit (-1);
#endif
    }

/*
 * Check input device capabilities.
 */
  if (ioctl (fd_in, SNDCTL_DSP_GETCAPS, &devcaps) == -1)
    {
      perror ("SNDCTL_DSP_GETCAPS");
      exit (-1);
    }

  if (devcaps & PCM_CAP_DUPLEX)
    {
      fprintf (stderr,
	       "Device %s supports duplex so you may want to use the single device approach instead\n",
	       dspname_in);
    }

  if (!(devcaps & PCM_CAP_INPUT))
    {
      fprintf (stderr, "%s doesn't support input\n", dspname_in);
      fprintf (stderr, "Please use different device.\n");
#if 0
      /*
       * NOTE! Earlier OSS versions don't necessarily support PCM_CAP_INPUT
       *       so don't panic.
       */
      exit (-1);
#endif
    }

/*
 * No need to turn on the full duplex mode when using separate input and output
 * devices. In fact calling SNDCTL_DSP_SETDUPLEX in this mode would be a
 * major mistake
 */

/*
 * Try to set the fragment size to suitable level.
 */

  if (ioctl (fd_out, SNDCTL_DSP_SETFRAGMENT, &frag) == -1)
    {
      perror ("SNDCTL_DSP_SETFRAGMENT");
      exit (-1);
    }
  if (ioctl (fd_in, SNDCTL_DSP_SETFRAGMENT, &frag) == -1)
    {
      perror ("SNDCTL_DSP_SETFRAGMENT");
      exit (-1);
    }

/*
 * Set up the sampling rate and other sample parameters.
 */

  tmp = channels;

  if (ioctl (fd_out, SNDCTL_DSP_CHANNELS, &tmp) == -1)
    {
      perror ("SNDCTL_DSP_CHANNELS");
      exit (-1);
    }

  if (tmp != channels)
    {
      fprintf (stderr, "%s doesn't support stereo (%d)\n", dspname_out, tmp);
      exit (-1);
    }
  if (ioctl (fd_in, SNDCTL_DSP_CHANNELS, &tmp) == -1)
    {
      perror ("SNDCTL_DSP_CHANNELS");
      exit (-1);
    }

  if (tmp != channels)
    {
      fprintf (stderr, "%s doesn't support stereo (%d)\n", dspname_in, tmp);
      exit (-1);
    }

  /*
   * Request 16 bit native endian sample format.
   */
  tmp = AFMT_S16_NE;

  if (ioctl (fd_out, SNDCTL_DSP_SETFMT, &tmp) == -1)
    {
      perror ("SNDCTL_DSP_SETFMT");
      exit (-1);
    }

/*
 * Note that most devices support only the usual litle endian (Intel)
 * byte order. This may be a problem under big endian architectures such
 * as Sparc. We accept also the opposite (alien) endianess but
 * this may require different handling (byte swapping) in most applications.
 * However we don't care about this issue because this applicaton doesn't
 * do any processing on the data.
 */

  if (tmp != AFMT_S16_NE && tmp != AFMT_S16_OE)
    {
      fprintf (stderr, "%s doesn't support 16 bit sample format (%x)\n",
	       dspname_out, tmp);
      exit (-1);
    }

  format = tmp;
  if (ioctl (fd_in, SNDCTL_DSP_SETFMT, &tmp) == -1)
    {
      perror ("SNDCTL_DSP_SETFMT");
      exit (-1);
    }

  if (tmp != format)
    {
      fprintf (stderr,
	       "Error: Input and output devices use different formats (%x/%x)\n",
	       tmp, format);
      exit (-1);
    }

  if (format == AFMT_S16_OE)
    {
      fprintf (stderr,
	       "Warning: Using 16 bit sample format with wrong endianess.\n");
    }

/*
 * It might be better to set the sampling rate firs on the input device and
 * then use the same rate with the output device. The reason for this is that
 * the vmix driver supports sample rate conversions for output. However input
 * is locked to the master device rate.
 */
  tmp = rate;

  if (ioctl (fd_in, SNDCTL_DSP_SPEED, &tmp) == -1)
    {
      perror ("SNDCTL_DSP_SPEED");
      exit (-1);
    }

  if (tmp != rate)
    {
      fprintf (stderr, "%s doesn't support requested rate %d (%d)\n",
	       dspname_out, rate, tmp);
      exit (-1);
    }

  if (ioctl (fd_out, SNDCTL_DSP_SPEED, &tmp) == -1)
    {
      perror ("SNDCTL_DSP_SPEED");
      exit (-1);
    }

  if (tmp != rate)
    {
      fprintf (stderr, "%s doesn't support the same rate %d!=%d as %s\n",
	       dspname_out, rate, tmp, dspname_in);
      exit (-1);
    }

/*
 * Get the actual fragment size. SNDCTL_DSP_GETBLKSIZE gives a good
 * compromise. If cooked mode is not disabled then the fragment sizes
 * used for input and output may be different. Using 
 * {!nlink SNDCTL_DSP_GETOSPACE} or {!nlink SNDCTL_DSP_GETISPACE}
 * may return different values. In this case SNDCTL_DSP_GETBLKSIZE will
 * return a value that is between them.
 */

  if (ioctl (fd_in, SNDCTL_DSP_GETBLKSIZE, &fragsize) == -1)
    {
      perror ("SNDCTL_DSP_GETBLKSIZE");
      exit (-1);
    }

  if (fragsize > sizeof (buffer))
    {
      fprintf (stderr, "Too large fragment size %d\n", fragsize);
      exit (-1);
    }

/*
 * Do not check the fragment sizes on both devices. It is perfectly normal
 * that they are different. Instead check just the input fragment size because
 * it is the one that matters.
 */

  printf ("Sample parameters set OK. Using fragment size %d\n", fragsize);

}

static void
method_0 (int fd_out, int fd_in)
{
/*
 * This function demonstrates how to implement an full duplex
 * application in the easiest and most reliable way. This method uses
 * blocking reads for synchronization with the device.
 */

  int l;
  int loopcount = 0;

  /*
   * Fragments can be as large as 32k (or even up to 64k)
   * with certain devices.
   */
  char silence[32 * 1024];	/* Buffer for one fragment of silence */

/*
 * This is the record/play loop. This block uses simple model where recorded data
 * is just read from the device and written back without use of any additional
 * ioctl calls. This approach should be used if the application doesn't need
 * to correlate the recorded samples with playback (for example for echo
 * cancellation).
 *
 * In this approach OSS will automatically find out how large buffer is
 * needed to handle the process without buffer underruns. However there will
 * probably be few dropouts before the buffer gets adapted for the worst
 * case latencies. 
 *
 * There are two methods to avoid the initial dropouts. The easiest method is
 * to write few milliseconds of silent sampels to the output before writing
 * the first block of actual recorded data. Another approach is to warm up
 * the device by replacing first seconds of recorded data with silecene
 * before writing the data to the output.
 *
 * After few moments of run the output should settle. After that the lag
 * between input and output should be very close to the minimum input-output
 * latency that can be obtained without using applied woodoo. The latencies can 
 * possibly be reduced by optimizing the application itself (if it's CPU bound),
 * by using shorter fragments (also check the max_intrate parameter in
 * osscore.conf), by terminating unnecessary applications and by using
 * higher (linear) priority.
 */

  while ((l = read (fd_in, buffer, fragsize)) == fragsize)
    {
      int delay;
      float t;

      if (loopcount == 0)
	{
	  /*
	   * Output one extra fragment filled with silence
	   * before starting to write the actual data. This must
	   * be done after we have recorded the first fragment
	   * Without this extra silence in the beginning playback
	   * data will run out microseconds before next recorded
	   * data becomes available.
	   *
	   * Number of silence bytes written doesn't need to be
	   * exactly one fragment or any multiple of it. Sometimes
	   * (under highly loaded system) more silence will be
	   * needed. Sometimes just few samples may be
	   * enough.
	   */

	  memset (silence, 0, fragsize);
	  if (write (fd_out, silence, fragsize) != fragsize)
	    {
	      perror ("write");
	      exit (-1);
	    }
	}

      /*
       * Compute the output delay (in milliseconds)
       */
      if (ioctl (fd_out, SNDCTL_DSP_GETODELAY, &delay) == -1)
	delay = 0;

      delay /= 4;		/* Get number of 16 bit stereo samples */

      t = (float) delay / (float) rate;	/* Get delay in seconds */

      t *= 1000.0;		/* Convert delay to milliseconds */

      if ((l = write (fd_out, buffer, fragsize)) != fragsize)
	{
	  perror ("write");
	  exit (-1);
	}

#if 1
      /*
       * Printing the delay level will slow down the application which
       * makes the delay longer. So don't print it by default.
       */
      printf ("\rDelay=%5.3g msec", t);
      fflush (stdout);
#endif

      loopcount++;
    }

  perror ("read");
  exit (-1);
}

static void
method_1 (int fd_out, int fd_in)
{
/*
 * Many applications use select/poll or the equivivalent facilities provided
 * by GUI libraries like GTK+ to handle I/O with multiple devices. We use
 * select() in this example.
 *
 * This routine reads audio input, does some processing on the data and
 * forwards it to the output device. Hitting ENTER terminates the program.
 *
 * The logic is that any input available on the device will be read. However
 * the amount of space available on output is not checked because it's
 * unnecessary. Checking both input and output status would be very tricky
 * if not impossible.
 */

  char silence[32 * 1024];	/* Buffer for one fragment of silence */
  fd_set reads;
  int n;
  unsigned int trig;
  int first_time = 1;

/*
 * First we have to start the recording engine. Otherwise select() will never
 * report available input and the application will just iddle.
 */
  trig = 0;			/* Trigger OFF */
  if (ioctl (fd_in, SNDCTL_DSP_SETTRIGGER, &trig) == -1)
    perror ("SETTRIGGER 0");

  trig = PCM_ENABLE_INPUT;	/* Trigger ON */

  /*
   * Trigger output too if using the single device mode. Otherwise all writes
   * will fail.
   */
  if (fd_in == fd_out)
    trig |= PCM_ENABLE_OUTPUT;

  if (ioctl (fd_in, SNDCTL_DSP_SETTRIGGER, &trig) == -1)
    perror ("SETTRIGGER 1");

  while (1)			/* Infinite loop */
    {
      struct timeval time;

      FD_ZERO (&reads);

      FD_SET (0, &reads);	/* stdin */
      FD_SET (fd_in, &reads);

      time.tv_sec = 1;
      time.tv_usec = 0;
      if ((n = select (fd_in + 1, &reads, NULL, NULL, &time)) == -1)
	{
	  perror ("select");
	  exit (-1);
	}

      if (n == 0)
	{
	  fprintf (stderr, "Timeout\n");
	  continue;
	}

      if (FD_ISSET (0, &reads))	/* Keyboard input */
	{
	  printf ("Finished.\n");
	  exit (0);
	}

      if (FD_ISSET (fd_in, &reads))
	{
	  /*
	   * Recorded data is available.
	   */
	  int l, n;
	  struct audio_buf_info info;

	  if (ioctl (fd_in, SNDCTL_DSP_GETISPACE, &info) == -1)
	    {
	      perror ("GETISPACE");
	      exit (-1);
	    }

	  n = info.bytes;	/* How much */

	  if ((l = read (fd_in, buffer, n)) == n)
	    {
	      printf ("\r %5d bytes ", n);
	      fflush (stdout);

	      if (first_time)
		{
		  /*
		   * Write one fragment of silence before the actual data.
		   * This is necessary to avoid regular underruns while
		   * waiting for more data.
		   */
		  memset (silence, 0, fragsize);
		  if (write (fd_out, silence, fragsize) != fragsize)
		    {
		      perror ("write");
		      exit (-1);
		    }
		  first_time = 0;
		}

	      /*
	       * This is the place where you can add your processing.
	       * There are 'l' bytes of audio data stored in 'buffer'. This
	       * program uses 16 bit native endian format and two channels
	       * (stereo).
	       */
	      if (write (fd_out, buffer, l) != l)
		{
		  perror ("write");
		  exit (-1);
		}
	      continue;
	    }
	}

      perror ("read");
      exit (-1);
    }
}

int
main (int argc, char *argv[])
{

  int method = 0;

/*
 * Check the working method
 */
  if (argc > 1)
    method = atoi (argv[1]);

/*
 * Check if the sampling rate was given on command line.
 */
  if (argc > 2)
    {
      rate = atoi (argv[2]);
      if (rate == 0)
	rate = 48000;
    }

/*
 * Check if the device name is given on command line.
 */
  if (argc > 3)
    dspname = argv[3];

/*
 * Check if anotherdevice name is given for input.
 */
  if (argc > 4)
    dspname_in = argv[4];

  if (dspname_in == NULL)
    open_one_device (dspname);
  else
    open_two_devices (dspname, dspname_in);

  switch (method)
    {
    case 0:
      method_0 (fd_out, fd_in);
      break;

    case 1:
      method_1 (fd_out, fd_in);
      break;

    default:
      fprintf (stderr, "Method %d not defined\n", method);
      exit (-1);
    }

  exit (0);
}
