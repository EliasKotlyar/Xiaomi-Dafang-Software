/*
 * Purpose: A program that demonstrates MIDI input with /dev/music (obsolete)
 * Copyright (C) 4Front Technologies, 2002-2004. Released under GPLv2/CDDL.
 *
 * Description:
 * This program was supposed to be a sample program for doing MIDI input
 * with the /dev/music interface. However it has not much use since the
 * /dev/music interface is now obsoleted.
 *
 * The /dev/midi interface is recommended in the new applications. Please
 * see the "{!link MIDI}" section of the OSS Developer's manual.
 */
#include <soundcard.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

void
decode_event (unsigned char *ev)
{
  int i;

  for (i = 0; i < 8; i++)
    {
      printf ("%02x ", ev[i]);
    }

  printf (": ");

  switch (ev[0])
    {
    case EV_CHN_VOICE:
      printf ("EV_CHN_VOICE(%d)\t| ", ev[1]);
      switch (ev[2])
	{
	case MIDI_NOTEON:
	  printf ("Note On   ch%d note%d vel%d", ev[3], ev[4], ev[5]);
	  break;

	case MIDI_NOTEOFF:
	  printf ("Note Off  ch%d note%d vel%d", ev[3], ev[4], ev[5]);
	  break;

	case MIDI_KEY_PRESSURE:
	  printf ("KPressure ch%d note%d vel%d", ev[3], ev[4], ev[5]);
	  break;

	default:
	  printf ("*** Unknown ***");
	}
      break;

    case EV_CHN_COMMON:
      printf ("EV_CHN_COMMON\tdev%d\t| ", ev[1]);
      switch (ev[2])
	{
	case MIDI_CHN_PRESSURE:
	  printf ("MIDI_CHN_PRESSURE ch%d %d", ev[3], ev[4]);
	  break;
	case MIDI_PGM_CHANGE:
	  printf ("MIDI_PGM_CHANGE ch%d %d", ev[3], ev[4]);
	  break;
	case MIDI_CTL_CHANGE:
	  printf ("MIDI_CTL_CHANGE ch%d %d,%d", ev[3], ev[4],
		  *(short *) &ev[6]);
	  break;
	case MIDI_PITCH_BEND:
	  printf ("MIDI_CTL_CHANGE ch%d %d", ev[3], *(short *) &ev[6]);
	  break;
	default:
	  printf ("*** Unknown ***");
	}
      break;

    case EV_SYSEX:
      printf ("EV_SYSEX\tdev%d: ", ev[1]);
      for (i = 2; i < 8; i++)
	printf ("%02x ", ev[i]);
      break;

    case EV_TIMING:
      printf ("EV_TIMING\t\t| ");
      switch (ev[1])
	{
	case TMR_START:
	  printf ("TMR_START\t");
	  break;
	case TMR_STOP:
	  printf ("TMR_STOP\t");
	  break;
	case TMR_CONTINUE:
	  printf ("TMR_CONTINUE\t");
	  break;
	case TMR_WAIT_ABS:
	  printf ("TMR_WAIT_ABS\t%10u", *(unsigned int *) &ev[4]);
	  break;
	case TMR_WAIT_REL:
	  printf ("TMR_WAIT_REL\t%10u", *(unsigned int *) &ev[4]);
	  break;
	case TMR_ECHO:
	  printf ("TMR_ECHO\t%10u", *(unsigned int *) &ev[4]);
	  break;
	case TMR_TEMPO:
	  printf ("TMR_TEMPO\t%10u", *(unsigned int *) &ev[4]);
	  break;
	case TMR_SPP:
	  printf ("TMR_SPP\t%10u", *(unsigned int *) &ev[4]);
	  break;
	case TMR_TIMESIG:
	  printf ("TMR_TIMESIG\t%10u", *(unsigned int *) &ev[4]);
	  break;
	}
      break;

    case EV_SEQ_LOCAL:
      printf ("EV_SEQ_LOCAL\t*** Should not happen ***");
      break;

    case EV_SYSTEM:
      printf ("EV_SYSTEM(%d)\t\t| ", ev[1]);
      switch (ev[2])
	{
	case 0xf0:
	  printf ("SysEx *** Should not happen ***");
	  break;

	case 0xf1:
	  printf ("MTC Qframe  %02x", ev[3]);
	  break;

	case 0xf2:
	  printf ("Songpos ptr %02x,%02x", ev[3], ev[4]);
	  break;

	case 0xf3:
	  printf ("Song select %02x", ev[3]);
	  break;

	case 0xf4:
	case 0xf5:
	  printf ("*** Undefined ***");
	  break;

	case 0xf6:
	  printf ("Tune request");
	  break;

	case 0xf7:
	  printf ("EOX");
	  break;
	case 0xf8:
	  printf ("Timing clock");
	  break;
	case 0xf9:
	  printf ("** Undefined ***");
	  break;
	case 0xfa:
	  printf ("Start");
	  break;
	case 0xfb:
	  printf ("Continue");
	  break;
	case 0xfc:
	  printf ("Stop");
	  break;
	case 0xfd:
	  printf ("** Undefined ***");
	  break;
	case 0xfe:
	  printf ("Active sensing");
	  break;
	case 0xff:
	  printf ("SYSTEM RESET");
	  break;
	}
      break;

    default:
      printf ("*** Unknown event type ***");
    }
  printf ("\n");
}

int
main (int argc, int *argv[])
{
  int fd, l, i;
  unsigned char buf[4096];

  if ((fd = open ("/dev/music", O_RDONLY, 0)) == -1)
    {
      perror ("/dev/music");
      exit (-1);
    }

  if (ioctl (fd, SNDCTL_SEQ_ACTSENSE_ENABLE, 0) == -1)
    {
      perror ("/dev/music ACTSENSE_ENABLE");
      exit (-1);
    }

  if (ioctl (fd, SNDCTL_SEQ_TIMING_ENABLE, 0) == -1)
    {
      perror ("/dev/music TIMING_ENABLE");
      exit (-1);
    }

  if (ioctl (fd, SNDCTL_SEQ_RT_ENABLE, 0) == -1)
    {
      perror ("/dev/music RT_ENABLE");
      exit (-1);
    }

  while ((l = read (fd, buf, sizeof (buf))) != -1)
    {
      for (i = 0; i < l; i += 8)
	{
	  decode_event (&buf[i]);
	}
    }
}
