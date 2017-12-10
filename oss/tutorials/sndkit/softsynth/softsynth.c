/*
 * Purpose: A simple software MIDI synthesizer program.
 * Copyright (C) 4Front Technologies, 2002-2004. Released in public domain.
 *
 * Description:
 * This is a pretty simple program that demonstrates how to do MIDI input at
 * the same time with audio playback (using select). It also demonstrates how
 * to use the MIDI loopback devices of OSS 4.0 (and later). 
 * Please note that this program is nothing but a programming example. It's
 * "output quality" equals to $10 (or cheaper) toy organs. However it's very
 * amazing how great some songs (MIDI files) sound even 90% of the MIDI 
 * information is simply ignored.
 *
 * What this program actually does is that it listen's to the MIDI input port
 * and interprets the incoming MIDI stream (using the midiparser routines
 * included in the OSSlib library).
 *
 * For simplicity reasons this program does nothing else but plays simple
 * sine waves at the right note frequencies. Percussive sounds (MIDI
 * channel 10) are simply ignored because playing them as sine waves doesn't
 * make any sense. All MIDI controllers, pitch bend as well as all the other
 * MIDI features are ignored too. However the all notes off control change
 * message is handled because otherwise hanging notes will be left if the
 * client (player) application gets killed abnormally.
 *
 * There is simple fixed envelope handling (actually just attack and decay)
 * and primitive handling of note on velocity. These features appeared to be
 * necessary because otherwise nobody can listen the output.
 *
 * This program is not too usefull as a synthesizer. It's not intended to be
 * any super "modular synthesizer". However it demonstrates how simple it is
 * to implement any kind of software MIDI synthesizer using the OSS API.
 * You don't need to know how to use some 450 audio related calls or 300
 * MIDI/sequencer related calls. As you will see practically everything will
 * be handled automagically by OSS. So you can spend all your time on
 * writing the application itself. This program was written, and debugged
 * in less than 5 hours from scratch (including MIDI input, audio output
 * and the actual synthesis). In fact it took longer time to write these
 * comments than the application itself.
 *
 * The major benefit of this super simple design is that it cannot fail. 
 * Provided that you don't try to set the buffer size to a too small value
 * the application logic is fully nuke proof. It will work unmodified with
 * every sound card in the world (past, current and future).
 *
 * The MIDI parser code was taken from some earlier work but we have included
 * if in the OSSlib library for you (under LGPL). Please feel free to use it
 * in your own OSS MIDI applications.
 *
 * To use this program you will need to install the "4Front MIDI loopback"
 * driver using the "Add new card/device" function of soundconf.
 * Then start this program in background (the audio and MIDI device names
 * can be given as command line arguments. For example
 *
 *	softsynth /dev/dsp /dev/midi01
 *
 * You can find out the loopback MIDI device number by looking for the
 * "MIDI loopback server side" devices using the {!xlink ossinfo} -m
 * command. Btw, nothing prevents using any "real" physical MIDI port as the
 * input.
 *
 * When the synthesizer server is running you can play any MIDI file using
 * some OSS based MIDI sequencer/player such as {!xlink ossmplay}.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <sys/select.h>
#include <sys/soundcard.h>
#include <midiparser.h>
midiparser_common_t *parser = NULL;

int audio_fd;
int midi_fd;
int sample_rate = 48000;

/*
 * The open_audio_device routine opens the audio device and initializes it 
 * for the required mode. This code was borrowed directly from the
 * {!nlink singen.c} sample program. However since the buffer size
 * is inportant with this kind of application we have added a call that
 * sets the fragment and buffer sizes.
 */

static int
open_audio_device (char *name, int mode)
{
  int tmp, fd;

  if ((fd = open (name, mode, 0)) == -1)
    {
      perror (name);
      exit (-1);
    }

/*
 * Setup the audio buffering policy so that reasonably small latencies 
 * can be obtained.
 *
 * 4 fragments of 256 samples (512 bytes) might be good. 256 samples
 * will give timing granularity of 256/sample_rate seconds (5.33 msec)
 * which is fairly adequate. The effect of the granularity (fragment size) in
 * this type of applications is timing jitter (or choking). Every event that
 * occurs withing the 5.33 msec period (fragment time) will get executed
 * in the beginning of the next period. If the fragment size is decreased
 * then the granularity will decrease too. However this will cause slight
 * increase in the CPU consumption of the application.
 *
 * The total buffer size (number_of_fragments*fragment_time) will define the
 * maximum latency between the event (note on/off) and the actual change in the
 * audio output. The average latency will be something like
 * (number_of_fragments-0.5)*fragment_time). The theoretical average latency
 * caused by this application is (4-0.5)*5.33 msec = ~19 msec). 
 * 
 * In musical terms 5.33 msec granularity equals to 1/750 note at 60 bpm
 * and 19 msecs equals to 1/214. This should be pretty adequate.
 *
 * The latency can be decreased by limiting the number of fragments and/or the
 * fragment size. However the after the buffer size drops close to the
 * capabilities of the system (delays caused by the other applications) the
 * audio output will start breaking. This can cured only by tuning the
 * hardware and the software environments (tuning some kernel parameters and
 * by killing all the other applications). However this is in no way an OSS
 * issue.
 *
 * With these parameters it was possible to compile Linux kernel in another
 * terminal window without any hiccup (fairly entry level 2.4 GHz P4 system
 * running Linux 2.6.x).
 */
  tmp = 0x00040009;
  if (ioctl (fd, SNDCTL_DSP_SETFRAGMENT, &tmp) == -1)
    {
      perror ("SNDCTL_DSP_SETFRAGMENT");
    }

/*
 * Setup the device. Note that it's important to set the
 * sample format, number of channels and sample rate exactly in this order.
 * Some devices depend on the order.
 */

/*
 * Set the sample format
 */
  tmp = AFMT_S16_NE;		/* Native 16 bits */
  if (ioctl (fd, SNDCTL_DSP_SETFMT, &tmp) == -1)
    {
      perror ("SNDCTL_DSP_SETFMT");
      exit (-1);
    }

  if (tmp != AFMT_S16_NE)
    {
      fprintf (stderr,
	       "The device doesn't support the 16 bit sample format.\n");
      exit (-1);
    }

/*
 * Set the number of channels (mono)
 */
  tmp = 1;
  if (ioctl (fd, SNDCTL_DSP_CHANNELS, &tmp) == -1)
    {
      perror ("SNDCTL_DSP_CHANNELS");
      exit (-1);
    }

  if (tmp != 1)
    {
      fprintf (stderr, "The device doesn't support mono mode.\n");
      exit (-1);
    }

/*
 * Set the sample rate
 */
  sample_rate = 48000;
  if (ioctl (fd, SNDCTL_DSP_SPEED, &sample_rate) == -1)
    {
      perror ("SNDCTL_DSP_SPEED");
      exit (-1);
    }

/*
 * No need for rate checking because we will automatically adjust the
 * signal based on the actual sample rate. However most application must
 * check the value of sample_rate and compare it to the requested rate.
 *
 * Small differences between the rates (10% or less) are normal and the
 * applications should usually tolerate them. However larger differences may
 * cause annoying pitch problems (Mickey Mouse).
 */

  return fd;
}

static int
open_midi_device (char *name, int mode)
{
  int tmp, fd;

/*
 * This is pretty much all we nbeed.
 */

  if ((fd = open (name, mode, 0)) == -1)
    {
      perror (name);
      exit (-1);
    }

  return fd;
}

#define MAX_VOICES 256

typedef struct
{
  int active;			/* ON/OFF */
  int chn, note, velocity;	/* MIDI note parameters */

  float phase, step;		/* Sine frequency generator */

  float volume;			/* Note volume */

  float envelope, envelopestep;	/* Envelope generator */
  int envelopedir;		/* 0=fixed level, 1=attack, -1=decay */
} voice_t;

static voice_t voices[MAX_VOICES] = { 0 };

static int
note_to_freq (int note_num)
{

/*
 * This routine converts a midi note to a frequency (multiplied by 1000)
 * Notice! This routine was copied from the OSS sequencer code.
 */

  int note, octave, note_freq;
  static int notes[] = {
    261632, 277189, 293671, 311132, 329632, 349232,
    369998, 391998, 415306, 440000, 466162, 493880
  };

#define BASE_OCTAVE	5

  octave = note_num / 12;
  note = note_num % 12;

  note_freq = notes[note];

  if (octave < BASE_OCTAVE)
    note_freq >>= (BASE_OCTAVE - octave);
  else if (octave > BASE_OCTAVE)
    note_freq <<= (octave - BASE_OCTAVE);

  return note_freq;
}

/*
 * The note_on() routine initializes a voice with the right
 * frequency, volume and envelope parameters.
 */

static void
note_on (int ch, int note, int velocity)
{
  int i;

  for (i = 0; i < MAX_VOICES; i++)
    if (!voices[i].active)
      {
	voice_t *v = &voices[i];
	int freq;
	float step;

/*
 * Record the MIDI note on message parameters (just in case)
 */

	v->chn = ch;
	v->note = note;
	v->velocity = velocity;

/*
 * Convert the note number to the actual frequency (multiplied by 1000).
 * Then compute the step to be added to the phase angle to get the right
 * frequency.
 */

	freq = note_to_freq (note);
	step = 1000.0 * (float) sample_rate / (float) freq;	/* Samples/cycle */
	v->step = 2.0 * M_PI / step;
	if (v->step > M_PI)	/* Nyqvist was here */
	  return;
	v->phase = 0.0;

/*
 * Compute the note volume based on the velocity. Use linear scale which
 * maps velocity=0 to the 25% volume level. Proper synthesizers will use more
 * advanced methods (such as logarithmic scales) but this is good for our 
 * purposes.
 */
	v->volume = 0.25 + ((float) velocity / 127.0) * 0.75;

/*
 * Initialize the envelope engine to start from zero level and to add
 * some fixed amount to the envelope level after each sample.
 */
	v->envelope = 0.0;
	v->envelopedir = 1;
	v->envelopestep = 0.01;

/*
 * Fire the voice. However nothing will happen before the next audio
 * period (fragment) gets computed. This means that all the voices started
 * during the ending period will be rounded to start at the same moment.
 */
	v->active = 1;
	break;
      }
}

/*
 * The note_off() routine finds all the voices that have matching channel and
 * note numbers. Then it starts the envelope decay phase (10 times slower
 * than the attack phase.
 */

static void
note_off (int ch, int note, int velocity)
{
  int i;

  for (i = 0; i < MAX_VOICES; i++)
    if (voices[i].active && voices[i].chn == ch)
      if (voices[i].note = note)
	{
	  voice_t *v = &voices[i];
	  v->envelopedir = -1;
	  v->envelopestep = -0.001;
	}
}

/*
 * all_notes_off() is a version of note_off() that checks only the channel
 * number. Used for the All Notes Off MIDI controller (123).
 */

static void
all_notes_off (int ch)
{
  int i;

  for (i = 0; i < MAX_VOICES; i++)
    if (voices[i].active && voices[i].chn == ch)
      {
	voice_t *v = &voices[i];
	v->envelopedir = -1;
	v->envelopestep = -0.01;
      }
}

/*
 * Compute voice computes few samples (nloops) and sums them to the
 * buffer (that contains the sum of all previously computed voices).
 *
 * In real world applications it may be necessary to convert this routine to 
 * use floating point buffers (-1.0 to 1.0 range) and do the conversion
 * to fixed point only in the final output stage. Another change you may
 * want to do is using multiple output buffers (for stereo or multiple
 * channels) instead of the current mono scheme.
 *
 * For clarity reasons we have not done that.
 */

static void
compute_voice (voice_t * v, short *buf, int nloops)
{
  int i;

  for (i = 0; i < nloops; i++)
    {
      float val;

/*
 * First compute the sine wave (-1.0 to 1.0) and scale it to the right
 * level. Finally sum the sample with the earlier voices in the buffer.
 */
      val = sin (v->phase) * 1024.0 * v->envelope * v->volume;
      buf[i] += (short) val;

/*
 * Increase the phase angle for the next sample.
 */
      v->phase += v->step;

/*
 * Handle envelope attack or decay
 */
      switch (v->envelopedir)
	{
	case 1:
	  v->envelope += v->envelopestep;
	  if (v->envelope >= 1.0)	/* Full level ? */
	    {
	      v->envelope = 1.0;
	      v->envelopestep = 0.0;
	      v->envelopedir = 0;
	    }
	  break;

	case -1:
	  v->envelope += v->envelopestep;
	  if (v->envelope <= 0.0)	/* Decay done */
	    {
	      v->envelope = 0.0;
	      v->envelopestep = 0.0;
	      v->envelopedir = 0;
	      v->active = 0;	/* Shut up */
	    }
	  break;
	}
    }
}

/*
 * The midi_callback() function is called by the midi parser library when
 * a complete MIDI message is seen in the input. The MIDI message number
 * (lowest 4 bits usually set to zero), the channel (0-15), as well as the
 * remaining bytes will be passed in the parameters.
 *
 * The MIDI parser library will handle oddities (like running status
 * or use of note on with velocity of 0 as note off) so the application
 * doesn't need to care about such nasty things.
 *
 * Note that the MIDI percussion channel 10 (9 as passed in the ch parameter)
 * will be ignored. All other MIDI messages other than note on, note off
 * and the "all notes off" controller are simply ignored. 
 *
 * Macros like MIDI_NOTEON and MIDI_NOTEOFF are defined in soundcard.h.
 */

static void
midi_callback (void *context, int category, unsigned char msg,
	       unsigned char ch, unsigned char *parms, int len)
{
  switch (msg)
    {
    case MIDI_NOTEON:
      if (ch != 9)		/* Avoid percussions */
	note_on (ch, parms[0], parms[1]);
      break;

    case MIDI_NOTEOFF:
      if (ch != 9)		/* Avoid percussions */
	note_off (ch, parms[0], parms[1]);
      break;

    case MIDI_CTL_CHANGE:
      if (parms[0] == 123)
	all_notes_off (ch);
      break;

    }
}

/*
 * The handle_midi_input() routine reads all the MIDI input bytes
 * that have been received by OSS since the last read. Note that 
 * this read will not block.
 *
 * Finally the received buffer is sent to the midi parser library which in turn
 * calls midi_callback (see above) to handle the actual events.
 */

static void
handle_midi_input (void)
{
  unsigned char buffer[256];
  int l, i;

  if ((l = read (midi_fd, buffer, sizeof (buffer))) == -1)
    {
      perror ("MIDI read");
      exit (-1);
    }

  if (l > 0)
    midiparser_input_buf (parser, buffer, l);
}

/*
 * handle_audio_output() computes a new block of audio and writes it to the
 * audio device. As you see there is no checking for blocking or available
 * buffer space because it's simply not necessary with OSS 4.0 any more.
 * If there is any blocking then the time below our "tolerances".
 */

static void
handle_audio_output (void)
{
/*
 * Ideally the buffer size equals to the fragment size (in samples).
 * Using different sizes is not a big mistake but the granularity is 
 * defined by the buffer size or the fragment size (depending on which
 * one is larger),
 */
  short buf[256];
  int i;

  memset (buf, 0, sizeof (buf));

  /* Loop all the active voices */
  for (i = 0; i < MAX_VOICES; i++)
    if (voices[i].active)
      compute_voice (&voices[i], buf, sizeof (buf) / sizeof (*buf));

  if (write (audio_fd, buf, sizeof (buf)) == -1)
    {
      perror ("Audio write");
      exit (-1);
    }
}

int
main (int argc, char *argv[])
{
  fd_set readfds, writefds;
/*
 * Use /dev/dsp as the default device because the system administrator
 * may select the device using the {!xlink ossctl} program or some other
 * methods
 */
  char *audiodev_name;
  char *mididev_name;

/*
 * It's recommended to provide some method for selecting some other
 * device than the default. We use command line argument but in some cases
 * an environment variable or some configuration file setting may be better.
 */
  if (argc != 3)
    {
      fprintf (stderr, "Usage: %s audio_device midi_device\n", argv[0]);
      exit (-1);
    }

  audiodev_name = argv[1];
  mididev_name = argv[2];

/*
 * It's mandatory to use O_WRONLY in programs that do only playback. Other
 * modes may cause increased resource (memory) usage in the driver. It may
 * also prevent other applications from using the same device for 
 * recording at the same time.
 */
  audio_fd = open_audio_device (audiodev_name, O_WRONLY);

/*
 * Open the MIDI device for read access (only).
 */

  midi_fd = open_midi_device (mididev_name, O_RDONLY);

/*
 * Init the MIDI input parser (from OSSlib)
 */

  if ((parser = midiparser_create (midi_callback, NULL)) == NULL)
    {
      fprintf (stderr, "Creating a MIDI parser failed\n");
      exit (-1);
    }

/*
 * Then the select loop. This program uses select instead of poll. However 
 * you can use select if you like (it should not matter).
 *
 * The logic is very simple. Wait for MIDI input and audio output events.
 * If there is any MIDI input then handle it (by modifying the voices[]
 * array.
 *
 * When there is space to write more audio data then we simply compute one
 * block of output and write it to the device.
 */

  while (1)			/* Infinite loop */
    {
      int i, n;

      FD_ZERO (&readfds);
      FD_ZERO (&writefds);

      FD_SET (audio_fd, &writefds);
      FD_SET (midi_fd, &readfds);

      if ((n = select (midi_fd + 1, &readfds, &writefds, NULL, NULL)) == -1)
	{
	  perror ("select");
	  exit (-1);
	}

      if (n > 0)
	{
	  if (FD_ISSET (midi_fd, &readfds))
	    handle_midi_input ();
	  if (FD_ISSET (audio_fd, &writefds))
	    handle_audio_output ();
	}
    }

/*
 * You may wonder what do we do between the songs. The answer is nothing.
 * The note off messages (or the all notes off controller) takes care of
 * shutting up the voices. When there are no voices playing the application
 * will just output silent audio (until it's killed). So there is no need to
 * know if a song has ended.
 *
 * However the MIDI loopback devices will retgurn a MIDI stop (0xfc) message
 * when the client side is closed and a MIDI start (0xfa) message when some
 * application starts playing. The server side application (synth) can
 * use these events for it's purposes.
 */

/*
 * That's all folks!
 *
 * This is pretty much all of it. This program can be easily improced by
 * using some more advanced synthesis algorithm (wave table, sample playback,
 * physical modelling or whatever else) and by interpreting all the MIDI
 * messages. You can also add a nice GUI. You have complete freedom to
 * modify this program and distribute it as your own work (under GPL, BSD
 * proprietary or whatever license you can imagine) but only AS LONG AS YOU
 * DON*T DO ANY STUPID CHANGES THAT BREAK THE RELIABILITY AND ROBUSTNESS.
 *
 * The point is that regardless of what you do there is no need to touch the
 * audio/MIDI device related parts. They are already "state of the art".
 * So you can spend all your time to work on the "payload" code. What you
 * can do is changing the compute_voice() and midi_callback() routines and
 * everything called by them.
 */

  exit (0);
}
