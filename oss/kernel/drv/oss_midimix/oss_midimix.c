/*
 * Purpose: MIDI mixer pseudo driver
 *
 * This driver creates a pseudo MIDI port device that can be used for
 * real-time mixer volume changes. The pseudo MIDI device will return MIDI
 * control change messages when a mixer setting changes. Application using the
 * device may also do mixer changes (cross fading, etc) by sending (or
 * playing back) control messages.
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

#include "oss_midimix_cfg.h"
#include "midi_core.h"
#include "midiparser.h"

#define MAX_INSTANCES	1

typedef struct
{
  oss_device_t *osdev;
  oss_mutex_t mutex;
  int midi_dev;
  int open_mode;
  oss_midi_inputbuf_t inputbuf;
  midiparser_common_p parser;
  int modify_counters[16];
  int update_counters[16][128];
  timeout_id_t timeout_id;
} midimix_devc;

static midimix_devc midimix_devs[MAX_INSTANCES] = {{ 0 }};
static int ndevs = 0;

static void
midimix_close (int dev, int mode)
{
  midimix_devc *devc = midi_devs[dev]->devc;
  oss_native_word flags;

  midiparser_unalloc (devc->parser);
  devc->parser = NULL;
  untimeout (devc->timeout_id);

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  devc->open_mode = 0;
  devc->inputbuf = NULL;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

void
parser_cb (void *context, int category, unsigned char msg, unsigned char ch,
	   unsigned char *parms, int len)
{

  if (category == CAT_CHN && msg == MIDI_CTL_CHANGE)
    {
      oss_mixext *ext;
      oss_mixer_value rec;

      int dev = ch;
      int ctl = parms[0];
      int val = parms[1];

      if ((ext = mixer_find_ext (dev, ctl)) == NULL)
	return;			/* Not found */

      if (!(ext->flags & MIXF_READABLE) || !(ext->flags & MIXF_WRITEABLE))
	return;			/* Not accessible */

      if (ext->maxvalue > 127)
	val = (val * ext->maxvalue + 63) / 127;

      if (val < 0 || val > ext->maxvalue)
	return;			/* Value out of range */

      switch (ext->type)
	{
	case MIXT_MONOSLIDER:
	case MIXT_STEREOSLIDER:
	  val = val & 0xff;
	  val = val | (val << 8);
	  break;

	case MIXT_ONOFF:
	  val = !!val;
	  break;

	case MIXT_ENUM:
	  if (val >= ext->maxvalue)
	    return;		/* Out of range */
	  break;

	case MIXT_VALUE:
	case MIXT_HEXVALUE:
	case MIXT_SLIDER:
	  /* Accept as-is */
	  break;

	default:
	  return;		/* Type not supported */
	}

      memset (&rec, 0, sizeof (rec));
      rec.dev = dev;
      rec.ctrl = ctl;
      rec.value = val;
      rec.timestamp = ext->timestamp;
      oss_mixer_ext (dev, OSS_DEV_MIXER, SNDCTL_MIX_WRITE, (ioctl_arg) & rec);
    }

}

static void
check_for_updates (midimix_devc * devc, int force)
{
  int dev, ctl;

  for (dev = 0; dev < 16 && dev < num_mixers; dev++)
    {
      touch_mixer (dev);

      if (force
	  || (mixer_devs[dev]->modify_counter > devc->modify_counters[dev]))
	{
	  devc->modify_counters[dev] = mixer_devs[dev]->modify_counter;
	  for (ctl = 0; ctl < 128; ctl++)
	    {
	      oss_mixext *ext;
	      oss_mixer_value rec;
	      int val;
	      unsigned char midibuf[3];

	      if ((ext = mixer_find_ext (dev, ctl)) == NULL)
		break;		/* All controls processed I think */
	      if (!(ext->flags & MIXF_READABLE)
		  || !(ext->flags & MIXF_WRITEABLE))
		continue;	/* Not accessible */

	      if (!force
		  && devc->update_counters[dev][ctl] == ext->update_counter)
		continue;	/* No change */
	      devc->update_counters[dev][ctl] = ext->update_counter;

	      switch (ext->type)
		{
		case MIXT_MONOSLIDER:
		case MIXT_STEREOSLIDER:
		case MIXT_ONOFF:
		case MIXT_ENUM:
		case MIXT_VALUE:
		case MIXT_HEXVALUE:
		case MIXT_SLIDER:
		  /* Type OK */
		  break;

		default:
		  continue;	/* Type not supported */
		}

	      memset (&rec, 0, sizeof (rec));
	      rec.dev = dev;
	      rec.ctrl = ctl;
	      rec.timestamp = ext->timestamp;

	      if (oss_mixer_ext
		  (dev, OSS_DEV_MIXER, SNDCTL_MIX_READ,
		   (ioctl_arg) & rec) < 0)
		{
		  continue;	/* Read failed */
		}

	      val = rec.value & 0xff;
	      if (ext->maxvalue > 127)
		val = (val * 127 + (ext->maxvalue / 2)) / ext->maxvalue;

	      if (val < 0)
		val = 0;
	      if (val > 127)
		val = 127;

	      midibuf[0] = 0xb0 | dev;
	      midibuf[1] = ctl;
	      midibuf[2] = val;

	      devc->inputbuf (devc->midi_dev, midibuf, 3);
	    }
	}
    }
}

static void
timer_callback (void *arg)
{
  midimix_devc *devc = arg;
  check_for_updates (devc, 0);
  devc->timeout_id = timeout (timer_callback, devc, OSS_HZ / 10);	/* 0.1s */
}

static int
midimix_open (int dev, int mode, oss_midi_inputbyte_t inputbyte,
	      oss_midi_inputbuf_t inputbuf, oss_midi_outputintr_t outputintr)
{
  midimix_devc *devc = midi_devs[dev]->devc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  if (devc->open_mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }
  devc->open_mode = mode;
  devc->inputbuf = inputbuf;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  if ((devc->parser = midiparser_create (parser_cb, devc)) == NULL)
    {
      devc->open_mode = 0;
      devc->inputbuf = NULL;
      cmn_err (CE_WARN, "Cannot create MIDI parser\n");
      return OSS_ENOMEM;
    }

  devc->timeout_id = timeout (timer_callback, devc, OSS_HZ / 10);	/* 0.1s */
  return 0;
}

static int
midimix_ioctl (int dev, unsigned cmd, ioctl_arg arg)
{
  return OSS_EINVAL;
}

static int
midimix_out (int dev, unsigned char midi_byte)
{
  midimix_devc *devc = midi_devs[dev]->devc;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  midiparser_input (devc->parser, midi_byte);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 1;
}

static int
midimix_bulk_out (int dev, unsigned char *buf, int len)
{
  midimix_devc *devc = midi_devs[dev]->devc;
  int i;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  for (i = 0; i < len; i++)
    {
      midiparser_input (devc->parser, buf[i]);
    }
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return len;
}

static void
midimix_init_device (int dev)
{
  midimix_devc *devc = midi_devs[dev]->devc;

  if (devc->open_mode & OPEN_READ)
    check_for_updates (devc, 1);
}

static midi_driver_t midimix_driver = {
  midimix_open,
  midimix_close,
  midimix_ioctl,
  midimix_out,
  midimix_bulk_out,
  MIDI_PAYLOAD_SIZE,
  NULL,
  NULL,
  NULL,
  midimix_init_device
};

static void
attach_midimix_dev (oss_device_t * osdev)
{
  midimix_devc *devc = &midimix_devs[ndevs++];

  devc->osdev = osdev;

  MUTEX_INIT (osdev, devc->mutex, MH_DRV);

  devc->midi_dev =
    oss_install_mididev (OSS_MIDI_DRIVER_VERSION, "MIX",
			 "Mixer access MIDI device", &midimix_driver,
			 sizeof (midi_driver_t),
			 MFLAG_VIRTUAL | MFLAG_QUIET, devc, devc->osdev);
}

int
oss_midimix_attach (oss_device_t * osdev)
{
  int midimix_instances = 1;
  int i;

  oss_register_device (osdev, "OSS MIDI mixer driver");

  if (midimix_instances > MAX_INSTANCES)
    midimix_instances = MAX_INSTANCES;
  for (i = 0; i < midimix_instances; i++)
    {
      attach_midimix_dev (osdev);
    }

  return 1;
}

int
oss_midimix_detach (oss_device_t * osdev)
{
  int i, err;

  if ((err = oss_disable_device (osdev)) < 0)
    return 0;

  for (i = 0; i < ndevs; i++)
    {
      midimix_devc *devc;

      devc = &midimix_devs[i];
    }

  oss_unregister_device (osdev);

  return 1;
}
