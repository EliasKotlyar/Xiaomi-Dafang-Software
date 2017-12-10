/*
 * Purpose: A simple software MIDI synthesizer program with GTK GUI.
 * Copyright (C) 4Front Technologies, 2002-2004. Released in public domain.
 *
 * Description:
 *
 * This is an expanded version of the {!nlink softsynth.c} program
 * (please take a look at the original program first). Also this program
 * is more or less just an programming example that demontstrates how to
 * implement "real-time" GUI controls in the simpliest possible way. While
 * you can use this program as a program template, you may equally well start
 * from the original one.
 *
 * Like the original there is not much to learn about synthesis in this program
 * the only design goal has been to demonstrate some additional OSS MIDI 
 * features and to work as a test bed for them.
 *
 * The primary thing that has been done is replacing the original select() loop
 * with a GTK (actually GDK) compatible mechanism (gdk_add_input and 
 * gtk_main). This is done in the main program.
 *
 * There are few other added features that should be mentioned:
 *
 * 1) The "Enhance switch" that makes the sound slightly "different" by
 * adding some harmonics of the original note frequency. The sound is just
 * different (not necessarily better). This together with the "Mute" button
 * demonstrates how to do real-time adjustment of the settings.
 * 2) The "mute" button was added just to be able to check if the latencies
 * are audible or not.
 * 3) The new SNDCTL_SETNAME call is used to change the description of the
 * MIDI device. In this way the user of the client program can select the
 * right device much easier.
 * 4) The SNDCTL_GETSONG call is used to obtain the song name given
 * by the client (ossmplay does this using SNDCTL_SETSONG).
 * 5) Some code has been added to handle the MIDI timer start (0xfa) and
 * stop (0xfc) messages. WIth loopback devices these messages can be
 * used to find out when the client closed/opened the device. However if
 * the server starts in the middle of a song there will be no start message.
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
int new_song = 1;

int enhanced_mode = 0;
float main_vol = 1.0;

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

/*
 ************** Some GTK+ related routines **********************
 */
#include <gtk/gtk.h>

GtkWidget *main_window, *song_name, *time_code;

static void
close_all (GtkWidget * window, gpointer data)
{
  gtk_main_quit ();
  exit (-1);
}

/*
 * toggle_enhance() gets called when the "Enhance" button is hit. It just
 * changes the state of the enhanced_mode flag which is used by the
 * note_on() routine. This setting affects only the notes to be started
 * after the change. It doesn't affect any of the currently playing voices.
 */

static void
toggle_enhance (GtkWidget * window, gpointer data)
{
  enhanced_mode = !enhanced_mode;	/* ON <-> OFF */
}

/*
 * toggle_mute() handles the "Mute" button. The change will take effect
 * at the moment when the next audio block to be computed starts playing 
 * on the device. So this button can be used to check how long the total
 * latencies are (including any delays caused by device level FIFOs).
 */

static void
toggle_mute (GtkWidget * window, gpointer data)
{
  if (main_vol > 0)
    main_vol = 0.0;
  else
    main_vol = 1.0;
}

static void
update_song_name (void)
{
  oss_longname_t name;
  char tmp[256];

/*
 * Get the song name from the client and update the label. Notice that
 * SNDCTL_GETSONG will return error (EINVAL) if the device doesn't
 * support song names. This is not an error. It simple means that no
 * song information is available. The song name may also be an empty
 * string if the client has not registered any song name. Also this is
 * perfectly normal.
 *
 * The difference between EINVAL and an empty string (if it matters) is that
 * EINVAL means that the device will not return this info later (the
 * application may stop polling for it).
 */

  if (ioctl (midi_fd, SNDCTL_GETSONG, name) != -1)
    {
      if (*name == 0)
	strcpy (name, "Unknown song");
      sprintf (tmp, "Song: %s", name);
      gtk_label_set (GTK_LABEL (song_name), tmp);

      /* Forward the song name to the audio device too */
      ioctl (audio_fd, SNDCTL_SETSONG, name);
    }

  new_song = 0;
}

/*
 * The create_user_interface() routine is pretty much a standard
 * GUI initialization for any GTK based application. Not important
 * for the logic of this program. We just create some buttons and one
 * label and assign the callbacks to handle them.
 */

static void
create_user_interface (void)
{
  GtkWidget *button, *vbox;
  main_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_signal_connect (GTK_OBJECT (main_window),
		      "destroy", GTK_SIGNAL_FUNC (close_all), NULL);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (main_window), vbox);

  song_name = gtk_label_new ("Song: Unknown song");
  gtk_box_pack_start (GTK_BOX (vbox), song_name, FALSE, FALSE, 0);
  gtk_widget_show (song_name);

  time_code = gtk_label_new ("Song: Unknown song");
  gtk_box_pack_start (GTK_BOX (vbox), time_code, FALSE, FALSE, 0);
  gtk_widget_show (time_code);

  update_song_name ();

  button = gtk_check_button_new_with_label ("Mute");
  gtk_signal_connect (GTK_OBJECT (button),
		      "clicked", GTK_SIGNAL_FUNC (toggle_mute), NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_check_button_new_with_label ("Enhance");
  gtk_signal_connect (GTK_OBJECT (button),
		      "clicked", GTK_SIGNAL_FUNC (toggle_enhance), NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Exit");
  gtk_signal_connect (GTK_OBJECT (button),
		      "clicked", GTK_SIGNAL_FUNC (close_all), NULL);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  gtk_widget_show (vbox);
  gtk_widget_show (main_window);
}

/*
 ************** The actual synthesis engine *********************
 */

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
 * fork_voice() creates another instance of a voice (just like the
 * fork system call does). It's possible to change the pitch and volume
 * by setting the freq_ratio and vol_scale parameters to values below 1.0.
 */
static void
fork_voice (voice_t * orig, float freq_ratio, float vol_scale)
{
  int i;

  for (i = 0; i < MAX_VOICES; i++)
    if (!voices[i].active)
      {
	voice_t *v = &voices[i];

	memcpy (v, orig, sizeof (voice_t));
	v->step /= freq_ratio;
	v->volume *= vol_scale;
	return;
      }
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

	if (enhanced_mode)
	  {
/*
 * Stupid test that adds some harmonic frequencies. This makes the output
 * to sound bolder. This algorithm is called additive synthesis. However
 * this program is not the best possible one for learning that technique.
 */
	    fork_voice (v, 1.001, 0.9);	/* Add some beating */
	    fork_voice (v, 2.0, 0.1);
	    fork_voice (v, 3.0, 0.2);
	    fork_voice (v, 4.0, 0.02);
	    fork_voice (v, 6.0, 0.01);
	    fork_voice (v, 8.0, 0.01);
	  }
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
 * all_voices_off() mutes all voices immediately.
 */

static void
all_voices_off (int ch)
{
  int i;

  for (i = 0; i < MAX_VOICES; i++)
    if (voices[i].active && voices[i].chn == ch)
      {
	voice_t *v = &voices[i];

	v->active = 0;
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
      val = sin (v->phase) * 1024.0 * v->envelope * v->volume * main_vol;
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
 *********** Handling of OSS MIDI input and audio output ***********
 */

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
      if (new_song)
	update_song_name ();

      if (ch != 9)		/* Avoid percussions */
	note_on (ch, parms[0], parms[1]);
      break;

    case MIDI_NOTEOFF:
      if (ch != 9)		/* Avoid percussions */
	note_off (ch, parms[0], parms[1]);
      break;

    case MIDI_CTL_CHANGE:
      if (parms[0] == 120)
	all_voices_off (ch);
      if (parms[0] == 123)
	all_notes_off (ch);
      break;

    case 0xfa:			/* Start */
      /*
       * Note that the start message arrives at the moment when the
       * client side of the loopback device is opened. At that moment
       * the client has not updated the song name so we should
       * not try to read it immediately. Instead we have to do it
       * (for example) at the moment the first note is started.
       */
      new_song = 1;
      break;

    case 0xfc:			/* Stop */
      /*
       * The stop message arrives after the client side of the
       * loopback device has been closed. We will just re-draw
       * the song name (to clear the display field on the screen).
       */
      update_song_name ();
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
handle_midi_input (gpointer data, gint source, GdkInputCondition cond)
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
handle_audio_output (gpointer data, gint source, GdkInputCondition cond)
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

/*
 * The mtc_callback() routine updates the SMTPE/MTC time display on the
 * screen. The quarter frames (qframes) field is not shown.
 */

static void
mtc_callback (void *context, oss_mtc_data_t * mtc)
{
  char tmp[64];

  if (mtc->qframes != 0)
    return;

  sprintf (tmp, "%02d:%02d:%02d.%02d\n",
	   mtc->hours, mtc->minutes, mtc->seconds, mtc->frames);
  gtk_label_set (GTK_LABEL (time_code), tmp);
}

/*
 * Finally the main program
 */

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

  int tmp;

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
 * Request input of MTC time (25 FPS). This is just for fun.
 */
  tmp = 25;
  ioctl (midi_fd, SNDCTL_MIDI_MTCINPUT, &tmp);

/*
 * Report the server name to the client side. This name will be reported
 * by applications that check the device names. It will also be shown
 * in /dev/sndstat
 *
 * SNDCTL_SETNAME is a new ioctl call in OSS 4.0. It doesn't make any 
 * sense to do error checking with it.
 */

  ioctl (midi_fd, SNDCTL_SETNAME, "OSS user land synth demo");

/*
 * Init the MIDI input parser (from OSSlib)
 */

  if ((parser = midiparser_create (midi_callback, NULL)) == NULL)
    {
      fprintf (stderr, "Creating a MIDI parser failed\n");
      exit (-1);
    }

/*
 * Register the MTC timecode handler
 */
  midiparser_mtc_callback (parser, mtc_callback);

/*
 * Standard GTK+ initialization.
 */

  gtk_init (&argc, &argv);

  create_user_interface ();

  gdk_input_add (audio_fd, GDK_INPUT_WRITE, handle_audio_output, NULL);
  gdk_input_add (midi_fd, GDK_INPUT_READ, handle_midi_input, NULL);

  gtk_main ();

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
