/*
 * Purpose: A program that prints the S/PDIF receiver status.
 * Copyright (C) 4Front Technologies, 2002-2004. Released under GPLv2/CDDL.
 *
 * Description:
 * This program demonstrates use of the {!nlink SNDCTL_DSP_READCTL}
 * call. It's actually a low cost digital (S/PDIF) input analyzer.
 *
 * {!notice This program will work just with a bunch of sound cards because
 * most devices are not able to return this information. AT this moment the
 * only card that is verified is M Audio Audiophile 2496. It's possible that
 * some other M Audio Delta models work too. It's almost certain that
 * "ordinary" sound cards will never have a digital receiver chip capable to
 * return this information.}
 *
 * Please read the "{!link spdif_control}" section of the OSS Developer's
 * manual for more info.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <time.h>
#include <soundcard.h>

#undef  SHOW_DATA
#define SHOW_STATUS

#define RATE 48000
#define CHANNELS 2
#define BUFSZ (CHANNELS*RATE)

#ifdef SHOW_STATUS
static int
xbits (unsigned char b, int first, int last)
{
  int v, i;

  v = 0;

  for (i = first; i <= last; i++)
    {
      v <<= 1;
      if (b & (1 << i))
	v |= 1;
    }

  return v;
}

static void
decode_pro_mode (unsigned char *bits)
{
  printf ("Professional mode (PRO=1)\n");
}

static void
decode_consumer_mode (unsigned char *bits)
{
  int tmp, tmp2, tmp3;

  printf ("Consumer mode (PRO=0)\n");

  printf ("Byte 00=%02x: ", bits[0]);
  if (bits[0] & 0x02)
    printf ("Data (not audio) ");
  else
    printf ("Audio (not data) ");
  if (bits[0] & 0x04)
    printf ("Copy permitted ");
  else
    printf ("Copy inhibited ");

  tmp = xbits (bits[0], 3, 5);
  if (bits[0] & 0x02)
    printf ("Non-audio=0x%x ", tmp);
  else
    printf ("Pre-emph=0x%x ", tmp);

  tmp = xbits (bits[0], 6, 7);
  printf ("Mode=0x%x ", tmp);
  printf ("\n");

  printf ("Byte 01=%02x: ", bits[1]);
  tmp = xbits (bits[1], 0, 2);
  tmp2 = xbits (bits[1], 3, 6);
  tmp3 = xbits (bits[1], 7, 7);

  printf ("Category code = %x:%x, L=%d ", tmp, tmp2, tmp3);
  printf ("\n");

  printf ("Byte 02=%02x: ", bits[2]);
  tmp = xbits (bits[2], 0, 3);
  tmp2 = xbits (bits[2], 4, 7);

  printf ("Source number=0x%x Channel number=0x%x ", tmp, tmp2);
  printf ("\n");

  printf ("Byte 03=%02x: ", bits[3]);
  tmp = xbits (bits[3], 0, 3);
  tmp2 = xbits (bits[3], 4, 5);

  printf ("Sample rate=0x%x Clock accuracy=0x%x ", tmp, tmp2);
  printf ("\n");

}
#endif

int
main (int argc, char *argv[])
{
  char *devname = "/dev/dsp";
  unsigned short buf[BUFSZ], expected = 0;
  int fd, parm, l, i;
  int bcount = 0;

  if (argc > 1)
    devname = argv[1];

  if ((fd = open (devname, O_RDONLY, 0)) == -1)
    {
      perror (devname);
      exit (-1);
    }

  parm = AFMT_S16_NE;
  if (ioctl (fd, SNDCTL_DSP_SETFMT, &parm) == -1)
    {
      perror ("SETFMT");
      close (fd);
      exit (-1);
    }

  if (parm != AFMT_S16_NE)
    {
      printf
	("Error: 16 bit sample format is not supported by the device\n");
      printf ("%08x/%08x\n", parm, AFMT_S16_LE);
      close (fd);
      exit (-1);
    }

  parm = CHANNELS;
  if (ioctl (fd, SNDCTL_DSP_CHANNELS, &parm) == -1)
    {
      perror ("CHANNELS");
      close (fd);
      exit (-1);
    }

  parm = RATE;
  if (ioctl (fd, SNDCTL_DSP_SPEED, &parm) == -1)
    {
      perror ("SPEED");
      close (fd);
      exit (-1);
    }

  if (parm != RATE)
    {
      printf
	("Warning: %d Hz sampling rate is not supported by the device. Will use %d)\n",
	 RATE, parm);
    }

#ifdef SHOW_DATA
  while ((l = read (fd, buf, sizeof (buf))) > 0)
#else
  while (1)
#endif
    {
#ifdef SHOW_STATUS
      time_t t;
      oss_digital_control c;

      c.valid = VAL_CBITIN | VAL_ISTATUS;

      if (ioctl (fd, SNDCTL_DSP_READCTL, &c) == -1)
	{
	  perror ("SNDCTL_DSP_READCTL");
	  exit (-1);
	}

      time (&t);
      printf ("\n%s\n", ctime (&t));

      if (c.valid & VAL_ISTATUS)
	{
	  switch (c.in_locked)
	    {
	    case LOCK_NOT_INDICATED:
	      printf ("Receiver locked: Status unknown\n");
	      break;
	    case LOCK_UNLOCKED:
	      printf ("receiver locked: *** NOT LOCKED ***\n");
	      break;
	    case LOCK_LOCKED:
	      printf ("receiver locked: Locked OK\n");
	      break;
	    }

	  switch (c.in_quality)
	    {
	    case IN_QUAL_NOT_INDICATED:
	      printf ("Signal quality: Unknown\n");
	      break;
	    case IN_QUAL_POOR:
	      printf ("Signal quality: *** POOR ***\n");
	      break;
	    case IN_QUAL_GOOD:
	      printf ("Signal quality: Good\n");
	      break;
	    }

	  switch (c.in_vbit)
	    {
	    case VBIT_NOT_INDICATED:
	      printf ("V-bit: Unknown\n");
	      break;
	    case VBIT_ON:
	      printf ("V-bit: On (not valid audio)\n");
	      break;
	    case VBIT_OFF:
	      printf ("V-bit: Off (valid audio signal)\n");
	      break;
	    }

	  switch (c.in_data)
	    {
	    case IND_UNKNOWN:
	      printf ("Audio/data: Unknown\n");
	      break;
	    case IND_AUDIO:
	      printf ("Audio/data: Audio\n");
	      break;
	    case IND_DATA:
	      printf ("Audio/data: Data\n");
	      break;
	    }

	  printf ("Errors: ");
	  if (c.in_errors & INERR_CRC)
	    printf ("CRC ");
	  if (c.in_errors & INERR_QCODE_CRC)
	    printf ("QCODE_CRC ");
	  if (c.in_errors & INERR_PARITY)
	    printf ("PARITY ");
	  if (c.in_errors & INERR_BIPHASE)
	    printf ("BIPHASE ");
	  printf ("\n");
	}
      else
	printf ("No input status information available\n");

      if (c.valid & VAL_CBITIN && c.in_locked != LOCK_UNLOCKED)
	{
	  printf ("\n");
	  printf ("Control bits: ");
	  for (i = 0; i < 24; i++)
	    printf ("%02x ", c.cbitin[i]);
	  printf ("\n");

	  if (c.cbitin[0] & 0x01)
	    decode_pro_mode (c.cbitin);
	  else
	    decode_consumer_mode (c.cbitin);
	}
      else
	printf ("No incoming control bit information available\n");
#endif

#ifdef SHOW_DATA
#  ifdef SHOW_STATUS
      if (c.in_locked != LOCK_UNLOCKED)
#  endif
	{
	  for (i = 0; i < l / 2; i++)
	    {
	      if (buf[i] == expected)
		{
		  printf ("%04x\n", buf[i]);
		}
	      else
		{
		  printf ("Error %04x != %04x (%4x), c=%d/%x\n", buf[i],
			  expected, buf[i] ^ expected, bcount, bcount);
		}
	      expected = buf[i] + 1;
	      bcount++;
	    }
	}

#else
      sleep (1);
#endif
    }

  perror (devname);

  exit (-1);
}
