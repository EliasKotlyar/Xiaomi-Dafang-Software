/*
 * Purpose: A simple sample program for doing dull duplex using mmap
 *
 * Description: 
 *
 * This is a sample program for doing full duplex using mmap. 
 *
 * Unfortunately this program doesn't work at this moment (under construction).
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <soundcard.h>

/*
 * Note that the mmap approach bypasses the OSS drivers completely so you are
 * on your own. You will need to write the application so that it
 * can work with any sample rate or sample format one can imagine.
 *
 * We will try to use 48 kHz / 16 bits / stereo which is the most likely
 * format supported by the consumer audio devices.
 */
#define SAMPLE_RATE	48000
#define SAMPLE_FORMAT	AFMT_S16_NE
#define NUM_CHANNELS	2
/*
 * NOTICE!
 *
 * The mmap() feature is not supported by all devices. Using it will make
 * the application to fail with some devices (professional ones in particular).
 *
 * When using the mmap mode it's very important to disable any format 
 * conversions done by OSS. This can be done by calling SNDCTL_DSP_COOKEDMODE
 * _IMMEDIATELY_ after opening the device.
 */

int sample_rate, num_channels, sample_format;

int
main (int argc, char *argv[])
{
  int fd, sz, i, tmp, n, l, have_data = 0;

/*
 * /dev/dsp_mmap is the best default device. Another alternative is /dev/dsp
 * but in some systems the user may want to use different devices with
 * mmap applications than with the normal ones.
 *
 * /dev/dsp_mmap will not be present in pre OSS 4.0 systems. The application
 * may automatically try to open /dev/dsp if /dev/dsp_mmap is missing. 
 * Another approach is asking the user to create /dev/dsp_mmap.
 *
 * 	ln -s /dev/dsp /dev/dsp_mmap
 *
 * It's recommended that there is some method for configuring or selecting
 * the audio device (such as a command line option or an environment
 * variable.
 */
  char *audio_dev = "/dev/dsp_mmap";

  struct buffmem_desc imemd, omemd;
  caddr_t buf;

  char *ip, *op;

  struct audio_buf_info info;

  int frag = 0x00200008;	/* 32 fragments of 2^8=256 bytes */

  fd_set reads, writes;

/*
 * Getting the device name to open. First the command line method
 * (simplified).
 */

  if (argc >= 2)
    {
      audio_dev = argv[1];
    }
  else
    {
      /*
       * No device given on command line. Try to see if an environment
       * variable is set.
       *
       * We will use MYAPP_AUDIODEV as the variable name. Replace the
       * MYAPP_ prefix with your application name.
       */

      char *p;

      if ((p = getenv ("MYAPP_AUDIODEV")) != NULL)
	audio_dev = p;
    }

  if ((fd = open (audio_dev, O_RDWR, 0)) == -1)
    {
      perror (audio_dev);
      exit (-1);
    }
  /*
   * Disable cooked mode to permit mmap() with some devices.
   * Don't do any error checking since usually this call will fail.
   * There is no need to care about the return value.
   */
  tmp = 0;
  ioctl (fd, SNDCTL_DSP_COOKEDMODE, &tmp);	/* Don't check the error return */

  ioctl (fd, SNDCTL_DSP_SETFRAGMENT, &frag);	/* No error checking needed */

/*
 * Set the sample format. AFMT_S16_NE is best because it is supported by
 * practically all devices. Also it equals to the "short" data type.
 */

  sample_format = SAMPLE_FORMAT;
  if (ioctl (fd, SNDCTL_DSP_SETFMT, &sample_format) == -1)
    {				/* Something really fatal occurred. */
      perror ("SNDCTL_DSP_SETFMT");
      exit (-1);
    }

/*
 * Check that we can support the returned sample format. 
 */
  switch (sample_format)
    {
    case AFMT_S16_NE:
      /*
       * Everything is OK. The sample format equals to 16 bit native
       * integer (short in C/C++) so we do not need to do any
       * format conversions.
       */
      break;

#if 0
/*
 * We do not support any other formats. However you must be prepared to do
 * so. Please take a look at the formats section of the OSS API
 * manual.
 *
 * Here are the most common other formats.
 */

    case AFMT_S16_OE:
      /*
       * 16 bits but with alien endianess. We can use short as the data type
       * but the samples must be "byte swapped" prior writing them to
       * the device.
       */
      break;

    case AFMT_U8:
      /* 8 bits unsigned format (unsigned char). Note that signed
       * 16 bit samples can be converted to this format by 
       * doing u8_sample = (s16_ne_sample >> 8) - 128.
       */
      break;

    case AFMT_S24_NE:
    case AFMT_S24_OE:
    case AFMT_S32_NE:
    case AFMT_S32_OE:
      /* Please see the formats section of the OSS API
       * manual.
       */

#endif

    default:
      fprintf (stderr, "The device doesn't support the requested format\n");
      fprintf (stderr, "0x%08x != 0x%08x\n", tmp, SAMPLE_FORMAT);
      exit (-1);
    }

/*
 * Set up the number of channels.
 * This program will automatically support any number of channels by
 * writing the same sample value to each of the channels (mono).
 *
 * Equally well stereo samples can be mixed for a 1 channel
 * device by summing the channel values together. Stereo samples
 * can be played with multi channel devices by setting the remaining
 * channel samples to a silent value (0 with signed formats).
 */

  num_channels = NUM_CHANNELS;
  if (ioctl (fd, SNDCTL_DSP_CHANNELS, &num_channels) == -1)
    {
      perror ("SNDCTL_DSP_CHANNELS");
      exit (-1);
    }

/*
 * Set the sample rate.
 */
  tmp = SAMPLE_RATE;
  if (ioctl (fd, SNDCTL_DSP_SPEED, &tmp) == -1)
    {
      perror ("SNDCTL_DSP_SPEED");
      exit (-1);
    }

  if (tmp != SAMPLE_RATE)
    {
      /*
       * In some cases the device is not capable to support exactly
       * the requested sampling rate.
       *
       * We will tolerate a 5% error in between the requested and
       * granted sampling rates.
       *
       * If the error is larger then we cannot continue.
       *
       * NOTE! Applications written for the mass market must be prepared
       *       to support every possible sampling rate locally.
       */
      int v;

      v = abs (tmp - SAMPLE_RATE);

      if (v > ((SAMPLE_RATE * 5) / 100))
	{
	  fprintf (stderr,
		   "The device doesn't support sampling rate of %d Hz.\n",
		   SAMPLE_RATE);
	  fprintf (stderr, "The nearest rate it supports is %d Hz\n", tmp);
	  exit (-1);
	}
    }

  if (ioctl (fd, SNDCTL_DSP_GETISPACE, &info) == -1)
    {
      perror ("GETISPACE");
      exit (-1);
    }

  sz = info.fragstotal * info.fragsize;

  if ((buf =
       mmap (NULL, sz, PROT_READ, MAP_FILE | MAP_PRIVATE, fd,
	     0)) == (caddr_t) - 1)
    {
      perror ("mmap (read)");
      exit (-1);
    }
  printf ("mmap (in) returned %08x\n", buf);
  ip = buf;

  if ((buf =
       mmap (NULL, sz, PROT_WRITE, MAP_FILE | MAP_PRIVATE, fd,
	     0)) == (caddr_t) - 1)
    {
      perror ("mmap (write)");
      exit (-1);
    }
  printf ("mmap (out) returned %08x\n", buf);
  op = buf;

/*
 * Prepare for launch. Set the trigger bits to 0
 */
  tmp = 0;
  ioctl (fd, SNDCTL_DSP_GETTRIGGER, &tmp);
  printf ("Trigger was %08x\n", tmp);

  tmp = PCM_ENABLE_OUTPUT;
  ioctl (fd, SNDCTL_DSP_SETTRIGGER, &tmp);
  printf ("Trigger set to %08x\n", tmp);

  for (i = 0; i < sz; i++)
    op[i] = 0x00;

  while (1)
    {
      struct timeval time;

      FD_ZERO (&reads);
      FD_ZERO (&writes);

      /*  FD_SET(fd, &writes); */
      FD_SET (fd, &reads);

      time.tv_sec = 1;
      time.tv_usec = 0;
      if (select (fd + 1, &reads, &writes, NULL, &time) == -1)
	{
	  perror ("select");
	  exit (-1);
	}

      if (FD_ISSET (fd, &reads))
	{
	  count_info count, ocount;
	  int l, p;

	  if (ioctl (fd, SNDCTL_DSP_GETIPTR, &count) == -1)
	    perror ("GETIPTR");

	  if (ioctl (fd, SNDCTL_DSP_GETOPTR, &ocount) == -1)
	    perror ("GETOPTR");

	  printf ("read(%08x/%08x/%d)\n", count.bytes, count.ptr,
		  count.blocks);
	  printf ("write(%08x/%08x/%d)\n", ocount.bytes, ocount.ptr,
		  ocount.blocks);

	  l = count.ptr;
	  p = ocount.ptr;

	  if (l > (sz - ocount.ptr))
	    l = sz - ocount.ptr;

	  for (i = 0; i < l; i++)
	    op[i + p] = ip[i];
	}

      if (FD_ISSET (fd, &writes))
	{
	  printf ("write()\n");
	  have_data = 0;
	}
    }

  exit (0);
}
