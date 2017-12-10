/*
 * Purpose: MIDI loopback driver
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

#include "oss_midiloop_cfg.h"
#include "midi_core.h"

#define MIDI_SYNTH_NAME	"OSS loopback MIDI"
#define MIDI_SYNTH_CAPS	SYNTH_CAP_INPUT

#define MAX_INSTANCES		8
#define CLIENT_NAME "MIDI loopback"
#define SERVER_NAME "MIDI loopback server side"

#define POLL_HZ	(OSS_HZ/100)

static int open_clients = 0;
static int open_servers = 0;

typedef struct midiloop_devc
{
  oss_device_t *osdev;
  oss_mutex_t mutex;

  int instance_no;
  int side;
#define SIDE_SERVER	0
#define SIDE_CLIENT	0
  int midi_dev;
  struct midiloop_devc *client, *server, *peer;

  int open_mode;
  oss_midi_inputbyte_t inputbyte_func;
  oss_midi_inputbuf_t inputbuf_func;
  oss_longname_t song_name;

} midiloop_devc;

static midiloop_devc midiloop_devs[2 * MAX_INSTANCES] = { {0} };
static int ndevs = 0;

static int
midiloop_open_server (int dev, int mode, oss_midi_inputbyte_t inputbyte,
		      oss_midi_inputbuf_t inputbuf,
		      oss_midi_outputintr_t outputintr)
{
  oss_native_word flags;
  midiloop_devc *devc, *client_devc;
  char *cmd;
  int client_dev;

  devc = midi_devs[dev]->devc;

  client_devc = devc->client;

  client_dev = client_devc->midi_dev;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (devc->open_mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  devc->open_mode = mode;
  devc->inputbyte_func = inputbyte;
  devc->inputbuf_func = inputbuf;

  open_servers++;

  if ((cmd = midi_devs[dev]->cmd) != NULL)
    {
      sprintf (midi_devs[client_dev]->name, CLIENT_NAME " (%s)", cmd);
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

static void
midiloop_close_server (int dev, int mode)
{
  oss_native_word flags;
  midiloop_devc *devc, *client_devc;
  int client_dev;

  devc = midi_devs[dev]->devc;
  client_devc = devc->client;
  client_dev = client_devc->midi_dev;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  devc->open_mode = 0;
  devc->inputbyte_func = NULL;
  devc->inputbuf_func = NULL;
  strcpy (midi_devs[client_dev]->name, CLIENT_NAME);
  midi_devs[client_dev]->latency = -1;	/* Not indicated */

  open_servers--;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

static int
midiloop_open_client (int dev, int mode, oss_midi_inputbyte_t inputbyte,
		      oss_midi_inputbuf_t inputbuf,
		      oss_midi_outputintr_t outputintr)
{
  oss_native_word flags;
  midiloop_devc *devc;

  devc = midi_devs[dev]->devc;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (devc->open_mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  devc->open_mode = mode;
  devc->inputbyte_func = inputbyte;
  devc->inputbuf_func = inputbuf;
  open_clients++;

  /* Notify the server */
  if (devc->server->open_mode & OPEN_READ)
    {
      if (devc->server->inputbyte_func != NULL)
	devc->server->inputbyte_func (devc->server->midi_dev, 0xfa);	/* Start */

      /* Restart the MTC timer of the server side (if necessary) */
      if (midi_devs[devc->server->midi_dev]->mtc_timebase > -1)
	{
	  int timebase = 25;
	  oss_midi_ioctl (devc->server->midi_dev, NULL, SNDCTL_MIDI_MTCINPUT,
			  (ioctl_arg) & timebase);
	}
    }

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

static void
midiloop_close_client (int dev, int mode)
{
  oss_native_word flags;
  midiloop_devc *devc;

  devc = midi_devs[dev]->devc;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  devc->open_mode = 0;
  devc->inputbyte_func = NULL;
  devc->inputbuf_func = NULL;
  open_clients--;

  /* Notify the server */
  if (devc->server->open_mode & OPEN_READ)
    if (devc->server->inputbyte_func != NULL)
      devc->server->inputbyte_func (devc->server->midi_dev, 0xfc);	/* Stop */
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  /* Halt the MTC timer of the server side (if necessary) */
  if (midi_devs[devc->server->midi_dev]->mtc_timebase > 0)
    {
      int timebase = 0;
      oss_midi_ioctl (devc->server->midi_dev, NULL, SNDCTL_MIDI_MTCINPUT,
		      (ioctl_arg) & timebase);
    }
}

static int
midiloop_out (int dev, unsigned char midi_byte)
{
  midiloop_devc *devc;
  oss_native_word flags;
  int ok = 0;

  devc = midi_devs[dev]->devc;

  if (devc->peer->open_mode == 0)
    return 1;
  MUTEX_ENTER_IRQDISABLE (devc->peer->mutex, flags);
  if (devc->peer->inputbyte_func != NULL)
    {
      ok = devc->peer->inputbyte_func (devc->peer->midi_dev, midi_byte);
    }
  MUTEX_EXIT_IRQRESTORE (devc->peer->mutex, flags);
  return ok;
}

static int
midiloop_bulk_out (int dev, unsigned char *buf, int len)
{
  midiloop_devc *devc;
  oss_native_word flags;
  int ok = 0;

  devc = midi_devs[dev]->devc;

  MUTEX_ENTER_IRQDISABLE (devc->peer->mutex, flags);
  if (devc->peer->open_mode == 0 || devc->peer->inputbuf_func == NULL)
    {
      MUTEX_EXIT_IRQRESTORE (devc->peer->mutex, flags);
      return len;
    }

  ok = devc->peer->inputbuf_func (devc->peer->midi_dev, buf, len);
  MUTEX_EXIT_IRQRESTORE (devc->peer->mutex, flags);
  return ok;
}

static void
midiloop_timer_setup (int dev /* Client dev */ )
{
  midiloop_devc *devc, *server_devc;
  int client_dev;
  int server_dev;

  devc = midi_devs[dev]->devc;
  server_devc = devc->server;
  client_dev = devc->midi_dev;
  server_dev = devc->server->midi_dev;

  oss_midi_copy_timer (server_dev, client_dev);
}

static int
midiloop_ioctl (int dev, unsigned cmd, ioctl_arg arg)
{
  char *song_name;
  oss_native_word flags;
  midiloop_devc *devc, *client_devc;
  int client_dev;

  devc = midi_devs[dev]->devc;
  client_devc = devc->client;
  client_dev = client_devc->midi_dev;

  switch (cmd)
    {
    case SNDCTL_SETSONG:
      if (devc->side != SIDE_CLIENT)
	return 0;

      song_name = (char *) arg;
      song_name[OSS_LONGNAME_SIZE - 1] = 0;

      MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
      strcpy (devc->song_name, song_name);
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return 0;
      break;

    case SNDCTL_SETNAME:
      if (devc->side != SIDE_SERVER)
	return 0;

      song_name = (char *) arg;
      song_name[OSS_LONGNAME_SIZE - 1] = 0;

      MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
      strcpy (midi_devs[devc->client->midi_dev]->name, song_name);
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return 0;
      break;

    case SNDCTL_GETSONG:
      song_name = (char *) arg;
      memset (song_name, 0, OSS_LONGNAME_SIZE);

      MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
      if (devc->side == SIDE_SERVER)
	strcpy (song_name, devc->peer->song_name);
      else
	strcpy (song_name, devc->song_name);
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return 0;
      break;

    case SNDCTL_MIDI_SET_LATENCY:
      if (devc->side != SIDE_SERVER)
	return OSS_EINVAL;
      if (*arg < -1 || *arg > 10000000)
	return OSS_EINVAL;
      midi_devs[devc->client->midi_dev]->latency = *arg;
      break;
    }
  return OSS_EINVAL;
}

static midi_driver_t midiloop_client_driver = {
  midiloop_open_client,
  midiloop_close_client,
  midiloop_ioctl,
  midiloop_out,
  midiloop_bulk_out,
  MIDI_PAYLOAD_SIZE,
  NULL,
  NULL,
  midiloop_timer_setup
};

static midi_driver_t midiloop_server_driver = {
  midiloop_open_server,
  midiloop_close_server,
  midiloop_ioctl,
  midiloop_out,
  midiloop_bulk_out,
  MIDI_PAYLOAD_SIZE
};

static void
attach_midiloop_dev (oss_device_t * osdev)
{
  midiloop_devc *server_devc = NULL, *client_devc = NULL;

  if (POLL_HZ < 1)
    {
      cmn_err (CE_CONT, "midiloop: Too low system timer resolution\n");
      return;
    }

  if (ndevs >= MAX_INSTANCES)
    {
      cmn_err (CE_CONT, "MidiLoop: Too many instances\n");
      return;
    }

  client_devc = &midiloop_devs[ndevs * 2];
  client_devc->osdev = osdev;
  MUTEX_INIT (client_devc->osdev, client_devc->mutex, MH_DRV);
  client_devc->instance_no = ndevs;

  client_devc->midi_dev =
    oss_install_mididev (OSS_MIDI_DRIVER_VERSION, "LOOP", CLIENT_NAME,
			 &midiloop_client_driver, sizeof (midi_driver_t),
			 MFLAG_VIRTUAL | MFLAG_CLIENT, client_devc,
			 client_devc->osdev);

  server_devc = &midiloop_devs[ndevs * 2 + 1];
  server_devc->osdev = osdev;
  MUTEX_INIT (server_devc->osdev, server_devc->mutex, MH_DRV);
  server_devc->instance_no = ndevs;

  midi_devs[client_devc->midi_dev]->latency = 1000000;

  server_devc->midi_dev =
    oss_install_mididev (OSS_MIDI_DRIVER_VERSION, "LOOP_S", SERVER_NAME,
			 &midiloop_server_driver, sizeof (midi_driver_t),
			 MFLAG_NOSEQUENCER | MFLAG_VIRTUAL | MFLAG_SERVER |
			 MFLAG_SELFTIMING, server_devc, server_devc->osdev);

  client_devc->server = client_devc->peer = server_devc;
  client_devc->client = client_devc;
  client_devc->side = SIDE_CLIENT;

  server_devc->client = server_devc->peer = client_devc;
  server_devc->server = server_devc;
  server_devc->side = SIDE_SERVER;

  ndevs++;
}

int
oss_midiloop_attach (oss_device_t * osdev)
{
  extern int midiloop_instances;
  int i;

  oss_register_device (osdev, "OSS MIDI loopback driver");

  if (midiloop_instances > MAX_INSTANCES)
    midiloop_instances = MAX_INSTANCES;
  for (i = 0; i < midiloop_instances; i++)
    {
      attach_midiloop_dev (osdev);
    }

  return 1;
}

int
oss_midiloop_detach (oss_device_t * osdev)
{
  int i, err;

  if ((err = oss_disable_device (osdev)) < 0)
    return 0;

  for (i = 0; i < 2 * ndevs; i++)
    {
      midiloop_devc *devc;

      devc = &midiloop_devs[i];
    }

  oss_unregister_device (osdev);

  return 1;
}
