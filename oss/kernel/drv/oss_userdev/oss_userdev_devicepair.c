/*
 * Purpose: Client/server audio device pair for oss_userdev
 *
 * This file implements the actual client/server device pair. There will be
 * separate oss_userdev instance for each process that has opened the
 * client side.
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

#include "oss_userdev_cfg.h"
#include <oss_userdev_exports.h>
#include "userdev.h"
static void userdev_free_device_pair (userdev_devc_t *devc);

extern int userdev_visible_clientnodes;

static void
transfer_audio (userdev_portc_t * server_portc, dmap_t * dmap_from,
		dmap_t * dmap_to)
{
  int l = dmap_from->fragment_size;
  unsigned char *fromp, *top;

  if (dmap_to->fragment_size != l)
    {
      cmn_err (CE_WARN, "Fragment size mismatch (%d != %d)\n",
	       dmap_to->fragment_size, l);

      /* Perform emergency stop */
      server_portc->input_triggered = 0;
      server_portc->output_triggered = 0;
      server_portc->peer->input_triggered = 0;
      server_portc->peer->output_triggered = 0;
      return;
    }

  fromp =
    dmap_from->dmabuf + (dmap_from->byte_counter % dmap_from->bytes_in_use);
  top = dmap_to->dmabuf + (dmap_to->byte_counter % dmap_to->bytes_in_use);

  memcpy (top, fromp, l);

}

static void
handle_input (userdev_portc_t * server_portc)
{
  userdev_portc_t *client_portc = server_portc->peer;

  if (client_portc->output_triggered)
    {
      transfer_audio (server_portc,
		      audio_engines[client_portc->audio_dev]->dmap_out,
		      audio_engines[server_portc->audio_dev]->dmap_in);
      oss_audio_outputintr (client_portc->audio_dev, 0);
    }

  oss_audio_inputintr (server_portc->audio_dev, 0);
}

static void
handle_output (userdev_portc_t * server_portc)
{
  userdev_portc_t *client_portc = server_portc->peer;

  if (client_portc->input_triggered)
    {
      transfer_audio (server_portc,
		      audio_engines[server_portc->audio_dev]->dmap_out,
		      audio_engines[client_portc->audio_dev]->dmap_in);
      oss_audio_inputintr (client_portc->audio_dev, 0);
    }

  oss_audio_outputintr (server_portc->audio_dev, 0);
}

static void
userdev_cb (void *pc)
{
/*
 * This timer callback routine will get called 100 times/second. It handles
 * movement of audio data between the client and server sides.
 */
  userdev_portc_t *server_portc = pc;
  userdev_devc_t *devc = server_portc->devc;
  int tmout = devc->poll_ticks;

  if (tmout < 1)
    tmout = 1;

  devc->timeout_id = 0;	/* No longer valid */

  if (server_portc->input_triggered)
    handle_input (server_portc);

  if (server_portc->output_triggered)
    handle_output (server_portc);

  /* Retrigger timer callback */
  if (server_portc->input_triggered || server_portc->output_triggered)
    devc->timeout_id = timeout (userdev_cb, server_portc, tmout);
}

static int
userdev_check_input (int dev)
{
  userdev_portc_t *portc = audio_engines[dev]->portc;
  if (!portc->peer->output_triggered)
    {
      return OSS_ECONNRESET;
    }
  return 0;
}

static int
userdev_check_output (int dev)
{
  userdev_portc_t *portc = audio_engines[dev]->portc;

  if (!portc->peer->input_triggered)
    {
      return OSS_ECONNRESET;
    }

  if (portc->peer->open_mode == 0)
    return OSS_EIO;
  return 0;
}

static void
setup_sample_format (userdev_portc_t * portc)
{
  adev_t *adev;
  userdev_devc_t *devc = portc->devc;
  int fragsize, frame_size;

  frame_size = devc->channels * devc->fmt_bytes;
  if (frame_size == 0)
    frame_size = 4;

  fragsize = (devc->rate * frame_size * devc->poll_ticks) / OSS_HZ;	/* Number of bytes/fragment */
  devc->rate = fragsize * 100 / frame_size;

/* Setup the server side */
  adev = audio_engines[portc->audio_dev];
  adev->min_block = adev->max_block = fragsize;

/* Setup the client side */
  adev = audio_engines[portc->peer->audio_dev];
  adev->min_block = adev->max_block = fragsize;

  adev->max_rate = adev->min_rate = devc->rate;
  adev->iformat_mask = devc->fmt;
  adev->oformat_mask = devc->fmt;
  adev->xformat_mask = devc->fmt;
  adev->min_channels = adev->max_channels = devc->channels;
}

static int
userdev_server_set_rate (int dev, int arg)
{
  userdev_portc_t *portc = audio_engines[dev]->portc;
  userdev_devc_t *devc = audio_engines[dev]->devc;
  adev_t *client_adev = audio_engines[portc->peer->audio_dev];

  if (arg == 0)
    return devc->rate;

  if (portc->peer->input_triggered || portc->peer->output_triggered)
    return devc->rate;

  if (arg < 5000)
    arg = 5000;
  if (arg > MAX_RATE)
    arg = MAX_RATE;

  /* Force the sample rate to be multiple of 100 */
  arg = (arg / 100) * 100;

  devc->rate = arg;

  client_adev->min_rate = arg;
  client_adev->max_rate = arg;

  setup_sample_format (portc);

  return devc->rate = arg;
}

/*ARGSUSED*/
static int
userdev_client_set_rate (int dev, int arg)
{
  userdev_devc_t *devc = audio_engines[dev]->devc;

  return devc->rate;
}

static short
userdev_server_set_channels (int dev, short arg)
{
  userdev_portc_t *portc = audio_engines[dev]->portc;
  userdev_devc_t *devc = audio_engines[dev]->devc;
  adev_t *client_adev = audio_engines[portc->peer->audio_dev];

  if (arg == 0)
    return devc->channels;

  if (portc->peer->input_triggered || portc->peer->output_triggered)
    return devc->channels;

  if (arg < 1)
    arg = 1;
  if (arg > MAX_CHANNELS)
    arg = MAX_CHANNELS;

  devc->channels = arg;
  client_adev->min_channels=client_adev->max_channels=arg;

  setup_sample_format (portc);

  return devc->channels;
}

/*ARGSUSED*/
static short
userdev_client_set_channels (int dev, short arg)
{
  userdev_devc_t *devc = audio_engines[dev]->devc;

  return devc->channels;	/* Server side channels */
}

static unsigned int
userdev_server_set_format (int dev, unsigned int arg)
{
  userdev_devc_t *devc = audio_engines[dev]->devc;
  userdev_portc_t *portc = audio_engines[dev]->portc;
  adev_t *client_adev = audio_engines[portc->peer->audio_dev];

  if (arg == 0)
    return devc->fmt;

  if (portc->peer->input_triggered || portc->peer->output_triggered)
    return devc->fmt;

  switch (arg)
    {
    case AFMT_S16_NE:
      devc->fmt_bytes = 2;
      break;

    case AFMT_S32_NE:
      devc->fmt_bytes = 4;
      break;

    default:			/* Unsupported format */
      arg = AFMT_S16_NE;
      devc->fmt_bytes = 2;

    }

  devc->fmt = arg;

  client_adev->oformat_mask = arg;
  client_adev->iformat_mask = arg;

  setup_sample_format (portc);

  return devc->fmt;
}

/*ARGSUSED*/
static unsigned int
userdev_client_set_format (int dev, unsigned int arg)
{
  userdev_devc_t *devc = audio_engines[dev]->devc;

  return devc->fmt;	/* Server side sample format */
}

static void userdev_trigger (int dev, int state);

static void
userdev_reset (int dev)
{
  userdev_trigger (dev, 0);
}

/*ARGSUSED*/
static int
userdev_server_open (int dev, int mode, int open_flags)
{
  userdev_portc_t *portc = audio_engines[dev]->portc;
  userdev_devc_t *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  if (portc == NULL || portc->peer == NULL)
    return OSS_ENXIO;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (portc->open_mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  portc->open_mode = mode;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  devc->open_count++;

  return 0;
}

/*ARGSUSED*/
static int
userdev_client_open (int dev, int mode, int open_flags)
{
  userdev_portc_t *portc = audio_engines[dev]->portc;
  userdev_devc_t *devc = audio_engines[dev]->devc;
  oss_native_word flags;

  if (portc == NULL || portc->peer == NULL)
    return OSS_ENXIO;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);

  if (portc->open_mode)
    {
      MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
      return OSS_EBUSY;
    }

  portc->open_mode = mode;
  devc->open_count++;

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
  return 0;
}

static void
wipe_audio_buffers(userdev_devc_t *devc)
{
/*
 * Write silence to the audio buffers when only one of the sides
 * is open. This prevents the device from looping the last fragments
 * written to the device.
 */
	dmap_t *dmap;

	dmap = audio_engines[devc->client_portc.audio_dev]->dmap_out;
	if (dmap != NULL && dmap->dmabuf != NULL)
	   memset(dmap->dmabuf, 0, dmap->buffsize);

	dmap = audio_engines[devc->client_portc.audio_dev]->dmap_in;
	if (dmap != NULL && dmap->dmabuf != NULL)
	   memset(dmap->dmabuf, 0, dmap->buffsize);

	dmap = audio_engines[devc->server_portc.audio_dev]->dmap_out;
	if (dmap != NULL && dmap->dmabuf != NULL)
	   memset(dmap->dmabuf, 0, dmap->buffsize);

	dmap = audio_engines[devc->server_portc.audio_dev]->dmap_in;
	if (dmap != NULL && dmap->dmabuf != NULL)
	   memset(dmap->dmabuf, 0, dmap->buffsize);
}

/*ARGSUSED*/
static void
userdev_server_close (int dev, int mode)
{
  userdev_portc_t *portc = audio_engines[dev]->portc;
  userdev_devc_t *devc = audio_engines[dev]->devc;
  oss_native_word flags;
  int open_count;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  portc->open_mode = 0;

  /* Stop the client side because there is no server */
  portc->peer->input_triggered = 0;
  portc->peer->output_triggered = 0;
  open_count = --devc->open_count;

  if (open_count == 0)
     userdev_free_device_pair (devc);
  else
     wipe_audio_buffers(devc);
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
static void
userdev_client_close (int dev, int mode)
{
  userdev_portc_t *portc = audio_engines[dev]->portc;
  userdev_devc_t *devc = audio_engines[dev]->devc;
  oss_native_word flags;
  int open_count;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  portc->open_mode = 0;

  open_count = --devc->open_count;

  if (open_count == 0)
     userdev_free_device_pair (devc);
  else
     wipe_audio_buffers(devc);

  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);
}

/*ARGSUSED*/
static int
userdev_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  switch (cmd)
    {
    case SNDCTL_GETLABEL:
      {
	/*
	 * Return an empty string so that this feature can be tested.
	 * Complete functionality is to be implemented later.
	 */
	oss_label_t *s = (oss_label_t *) arg;
	memset (s, 0, sizeof (oss_label_t));
	return 0;
      }
      break;

    case SNDCTL_GETSONG:
      {
	/*
	 * Return an empty string so that this feature can be tested.
	 * Complete functionality is to be implemented later.
	 */
	oss_longname_t *s = (oss_longname_t *) arg;
	memset (s, 0, sizeof (oss_longname_t));
	return 0;
      }
      break;
    }

  return OSS_EINVAL;
}

static void
set_adev_name(int dev, const char *name)
{
  adev_t *adev = audio_engines[dev];

  strcpy(adev->name, name);

#ifdef CONFIG_OSS_VMIX
  if (adev->vmix_mixer != NULL)
     vmix_change_devnames(adev->vmix_mixer, name);
#endif
	
}

static int
create_instance(int dev, userdev_create_t *crea)
{
  userdev_devc_t *devc = audio_engines[dev]->devc;
  char tmp_name[64];

  devc->match_method = crea->match_method;
  devc->match_key = crea->match_key;
  devc->create_flags = crea->flags;

  devc->poll_ticks = (crea->poll_interval * OSS_HZ) / 1000;

  if (devc->poll_ticks < 1) 
     devc->poll_ticks = 1;

  crea->poll_interval = (1000*devc->poll_ticks) / OSS_HZ;

  if (crea->poll_interval<1)
     crea->poll_interval = 1;

  crea->name[sizeof(crea->name)-1]=0; /* Overflow protectgion */

  sprintf(tmp_name, "%s (server)", crea->name);
  tmp_name[49]=0;
  set_adev_name (devc->client_portc.audio_dev, crea->name);
  set_adev_name (devc->server_portc.audio_dev, tmp_name);

  strcpy(crea->devnode, audio_engines[devc->client_portc.audio_dev]->devnode);

  return 0;
}

static int
userdev_set_control (int dev, int ctl, unsigned int cmd, int value)
{
  	userdev_devc_t *devc = mixer_devs[dev]->devc;

	if (ctl < 0 || ctl >= USERDEV_MAX_MIXERS)
	   return OSS_EINVAL;

	if (cmd == SNDCTL_MIX_READ)
	   {
		   return devc->mixer_values[ctl];
	   }

	devc->mixer_values[ctl] = value;
	devc->modify_counter++;

	return 0;
}

/*ARGSUSED*/
static int
userdev_server_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  switch (cmd)
    {
    case USERDEV_CREATE_INSTANCE:
	    {
		userdev_create_t *crea = (userdev_create_t *)arg;

	    	return create_instance(dev, crea);
	    }
	    break;

    case USERDEV_GET_CLIENTCOUNT:
	    {
  		userdev_devc_t *devc = audio_engines[dev]->devc;

		return *arg = (devc->client_portc.open_mode != 0);
	    }
	    break;

   /*
    * Mixer related ioctl calls
    */

    case USERDEV_CREATE_MIXGROUP:
	    {
  		userdev_devc_t *devc = audio_engines[dev]->devc;
		userdev_mixgroup_t *grp=(userdev_mixgroup_t*)arg;
		int group;

		grp->name[sizeof(grp->name)-1]=0; /* Buffer overflow protection */
		if ((group=mixer_ext_create_group(devc->mixer_dev, grp->parent, grp->name))<0)
			return group;
		grp->num = group;
		return 0;
	    }
	    break;

    case USERDEV_CREATE_MIXCTL:
	    {
  		userdev_devc_t *devc = audio_engines[dev]->devc;
		userdev_mixctl_t *c=(userdev_mixctl_t*)arg;
  		oss_mixext *ext;
		int ctl;

		c->name[sizeof(c->name)-1]=0; /* Buffer overflow protection */

		if (c->index < 0 || c->index >= USERDEV_MAX_MIXERS)
		   return OSS_EINVAL;

		if ((ctl = mixer_ext_create_control (devc->mixer_dev,
			       c->parent,
			       c->index,
			       userdev_set_control,
			       c->type,
			       c->name,
			       c->maxvalue,
			       c->flags)) < 0)
	    		return ctl;

		c->num = ctl;
  		ext = mixer_find_ext (devc->mixer_dev, ctl);

		ext->minvalue = c->offset;
		ext->control_no= c->control_no;
		ext->rgbcolor = c->rgbcolor;

		if (c->type == MIXT_ENUM)
		{
			memcpy(ext->enum_present, c->enum_present, sizeof(ext->enum_present));
  			mixer_ext_set_strings (devc->mixer_dev, ctl, c->enum_choises, 0);
		}

		return 0;
	    }
	    break;

    case USERDEV_GET_MIX_CHANGECOUNT:
	    {
  		userdev_devc_t *devc = audio_engines[dev]->devc;

		return *arg = devc->modify_counter;
	    }
	    break;

    case USERDEV_SET_MIXERS:
	    {
  		userdev_devc_t *devc = audio_engines[dev]->devc;

		memcpy(devc->mixer_values, arg, sizeof(devc->mixer_values));
		mixer_devs[devc->mixer_dev]->modify_counter++;
//cmn_err(CE_CONT, "Set %08x %08x\n", devc->mixer_values[0], devc->mixer_values[1]);
		return 0;
	    }
	    break;

    case USERDEV_GET_MIXERS:
	    {
  		userdev_devc_t *devc = audio_engines[dev]->devc;

		memcpy(arg, devc->mixer_values, sizeof(devc->mixer_values));
		return 0;
	    }
	    break;

    }

  return userdev_ioctl(dev, cmd, arg);
}

/*ARGSUSED*/
static void
userdev_output_block (int dev, oss_native_word buf, int count, int fragsize,
			int intrflag)
{
}

/*ARGSUSED*/
static void
userdev_start_input (int dev, oss_native_word buf, int count, int fragsize,
		       int intrflag)
{
}

static void
userdev_trigger (int dev, int state)
{
  userdev_portc_t *portc = audio_engines[dev]->portc;
  userdev_devc_t *devc = audio_engines[dev]->devc;

  if (portc->open_mode & OPEN_READ)	/* Handle input */
    {
      portc->input_triggered = !!(state & OPEN_READ);
    }

  if (portc->open_mode & OPEN_WRITE)	/* Handle output */
    {
      portc->output_triggered = !!(state & OPEN_WRITE);
    }

  if (portc->output_triggered || portc->input_triggered)	/* Something is going on */
    {
      int tmout = devc->poll_ticks;

      if (tmout < 1)
	tmout = 1;

      if (portc->port_type != PT_SERVER)
	portc = portc->peer;	/* Switch to the server side */

      if (portc->output_triggered || portc->input_triggered)	/* Something is going on */
	if (devc->timeout_id == 0)
	{
	  devc->timeout_id = timeout (userdev_cb, portc, tmout);
	}
    }
  else
    {
      if (portc->port_type == PT_SERVER)
	if (devc->timeout_id != 0)
	  {
	    untimeout (devc->timeout_id);
	    devc->timeout_id = 0;
	  }
    }
}

/*ARGSUSED*/
static int
userdev_server_prepare_for_input (int dev, int bsize, int bcount)
{
  oss_native_word flags;

  userdev_portc_t *portc = audio_engines[dev]->portc;
  userdev_devc_t *devc = audio_engines[dev]->devc;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  portc->input_triggered = 0;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

/*ARGSUSED*/
static int
userdev_server_prepare_for_output (int dev, int bsize, int bcount)
{
  oss_native_word flags;

  userdev_portc_t *portc = audio_engines[dev]->portc;
  userdev_devc_t *devc = audio_engines[dev]->devc;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  portc->output_triggered = 0;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

/*ARGSUSED*/
static int
userdev_client_prepare_for_input (int dev, int bsize, int bcount)
{
  oss_native_word flags;
  userdev_portc_t *portc = audio_engines[dev]->portc;
  userdev_devc_t *devc = audio_engines[dev]->devc;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  portc->input_triggered = 0;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

/*ARGSUSED*/
static int
userdev_client_prepare_for_output (int dev, int bsize, int bcount)
{
  oss_native_word flags;
  userdev_portc_t *portc = audio_engines[dev]->portc;
  userdev_devc_t *devc = audio_engines[dev]->devc;

  MUTEX_ENTER_IRQDISABLE (devc->mutex, flags);
  portc->output_triggered = 0;
  MUTEX_EXIT_IRQRESTORE (devc->mutex, flags);

  return 0;
}

/*ARGSUSED*/
static int
userdev_alloc_buffer (int dev, dmap_t * dmap, int direction)
{
#define MY_BUFFSIZE (64*1024)
  if (dmap->dmabuf != NULL)
    return 0;
  dmap->dmabuf_phys = 0;	/* Not mmap() capable */
  dmap->dmabuf = KERNEL_MALLOC (MY_BUFFSIZE);
  if (dmap->dmabuf == NULL)
    return OSS_ENOSPC;
  dmap->buffsize = MY_BUFFSIZE;

  return 0;
}

/*ARGSUSED*/
static int
userdev_free_buffer (int dev, dmap_t * dmap, int direction)
{
  if (dmap->dmabuf == NULL)
    return 0;
  KERNEL_FREE (dmap->dmabuf);

  dmap->dmabuf = NULL;
  return 0;
}

#if 0
static int
userdev_get_buffer_pointer (int dev, dmap_t * dmap, int direction)
{
}
#endif

/*ARGSUSED*/
static int
userdev_ioctl_override (int dev, unsigned int cmd, ioctl_arg arg)
{
/*
 * Purpose of this ioctl override function is to intercept mixer
 * ioctl calls made on the client side and to hide everything
 * outside the userdev instance from the application.
 *
 * Note that this ioctl is related with the client side audio device. However
 * if /dev/mixer points to this (audio) device then all mixer acess will
 * be redirected too. Also the vmix driver will redirect mixer/system ioctl
 * calls to this function.
 */
  int err;
  userdev_devc_t *devc = audio_engines[dev]->devc;
  adev_t *adev = audio_engines[devc->client_portc.audio_dev];

  switch (cmd)
  {
  case SNDCTL_MIX_NRMIX:
	  return *arg=1;
	  break;

  case SNDCTL_MIX_NREXT:
	  *arg = devc->mixer_dev;
	  return OSS_EAGAIN; /* Continue with the default handler */
	  break;

  case SNDCTL_SYSINFO:
	  {
	  /*
	   * Fake SNDCTL_SYSINFO to report just one mixer device which is
	   * the one associated with the client.
	   */
	    oss_sysinfo *info = (oss_sysinfo *) arg;
	    int i;

	    if ((err=oss_mixer_ext(dev, OSS_DEV_DSP_ENGINE, cmd, arg))<0)
	       return err;

	    /*
	     * Hide all non-oss_userdev devices
	     */
	    strcpy (info->product, "OSS (userdev)");
	    info->nummixers = 1;
	    info->numcards = 1;
	    info->numaudios = 1;
	    for (i = 0; i < 8; i++)
	       info->openedaudio[i] = 0;

	    return 0;
	  }
	  break;

  case SNDCTL_MIXERINFO:
	{
		oss_mixerinfo *info = (oss_mixerinfo *) arg;

		info->dev = devc->mixer_dev; /* Redirect to oss_userdev mixer */

	    	if ((err=oss_mixer_ext(dev, OSS_DEV_DSP_ENGINE, cmd, arg))<0)
	           return err;

		strcpy(info->name, adev->name);
		info->card_number = 0;
		return 0;
	}

  case SNDCTL_AUDIOINFO:
  case SNDCTL_AUDIOINFO_EX:
	{
		oss_audioinfo *info = (oss_audioinfo *) arg;

		info->dev = devc->client_portc.audio_dev;

		cmd = SNDCTL_ENGINEINFO;

	    	if ((err=oss_mixer_ext(dev, OSS_DEV_DSP_ENGINE, cmd, arg))<0)
	           return err;

		info->card_number = 0;
		return 0;
	}

    case SNDCTL_CARDINFO:
      {
	oss_card_info *info = (oss_card_info *) arg;

	info->card = adev->card_number; /* Redirect to oss_userdev0 */

    	if ((err=oss_mixer_ext(dev, OSS_DEV_DSP_ENGINE, cmd, arg))<0)
           return err;

	info->card = 0;
	return 0;
      }

  case SNDCTL_MIX_EXTINFO:
      {
	      oss_mixext *ext = (oss_mixext*)arg;

	      ext->dev = devc->mixer_dev;

	      if ((err=oss_mixer_ext(dev, OSS_DEV_DSP_ENGINE, cmd, arg))<0)
	           return err;

	      if (ext->type == MIXT_DEVROOT)
	      {
		oss_mixext_root *root = (oss_mixext_root *) ext->data;
		strncpy(root->name, adev->name, 48);
		root->name[47]=0;
	      }

	      return 0;
      }
      break;

  case SNDCTL_MIX_READ:
  case SNDCTL_MIX_WRITE:
      {
	      oss_mixer_value *ent = (oss_mixer_value*)arg;

	      ent->dev = devc->mixer_dev;

	      return OSS_EAGAIN; 	/* Redirect */
      }
      break;

  default:
	  return OSS_EAGAIN;
  }
}

static audiodrv_t userdev_server_driver = {
  userdev_server_open,
  userdev_server_close,
  userdev_output_block,
  userdev_start_input,
  userdev_server_ioctl,
  userdev_server_prepare_for_input,
  userdev_server_prepare_for_output,
  userdev_reset,
  NULL,
  NULL,
  NULL,
  NULL,
  userdev_trigger,
  userdev_server_set_rate,
  userdev_server_set_format,
  userdev_server_set_channels,
  NULL,
  NULL,
  userdev_check_input,
  userdev_check_output,
  userdev_alloc_buffer,
  userdev_free_buffer,
  NULL,
  NULL,
  NULL				/* userdev_get_buffer_pointer */
};

static audiodrv_t userdev_client_driver = {
  userdev_client_open,
  userdev_client_close,
  userdev_output_block,
  userdev_start_input,
  userdev_ioctl,
  userdev_client_prepare_for_input,
  userdev_client_prepare_for_output,
  userdev_reset,
  NULL,
  NULL,
  NULL,
  NULL,
  userdev_trigger,
  userdev_client_set_rate,
  userdev_client_set_format,
  userdev_client_set_channels,
  NULL,
  NULL,
  userdev_check_input,
  userdev_check_output,
  userdev_alloc_buffer,
  userdev_free_buffer,
  NULL,
  NULL,
  NULL,	// userdev_get_buffer_pointer
  NULL,	// userdev_calibrate_speed
  NULL,	// userdev_sync_control
  NULL,	// userdev_prepare_to_stop
  NULL,	// userdev_get_input_pointer
  NULL,	// userdev_get_output_pointer
  NULL,	// userdev_bind
  NULL,	// userdev_setup_fragments
  NULL,	// userdev_redirect
  userdev_ioctl_override
};

static int
userdev_mixer_ioctl (int dev, int audiodev, unsigned int cmd, ioctl_arg arg)
{

  if (cmd == SOUND_MIXER_READ_CAPS)
    return *arg = SOUND_CAP_NOLEGACY;

#if 0
  if (cmd == SOUND_MIXER_READ_DEVMASK ||
      cmd == SOUND_MIXER_READ_RECMASK || cmd == SOUND_MIXER_READ_RECSRC)
    return *arg = 0;

  if (cmd == SOUND_MIXER_READ_VOLUME || cmd == SOUND_MIXER_READ_PCM)
    return *arg = 100 | (100 << 8);
  if (cmd == SOUND_MIXER_WRITE_VOLUME || cmd == SOUND_MIXER_WRITE_PCM)
    return *arg = 100 | (100 << 8);
#endif
  return OSS_EINVAL;
}

static mixer_driver_t userdev_mixer_driver = {
  userdev_mixer_ioctl
};

static int
install_server (userdev_devc_t * devc)
{
  userdev_portc_t *portc = &devc->server_portc;
  int adev;

  int opts =
    ADEV_STEREOONLY | ADEV_16BITONLY | ADEV_VIRTUAL |
    ADEV_FIXEDRATE | ADEV_SPECIAL | ADEV_HIDDEN | ADEV_DUPLEX;

  memset (portc, 0, sizeof (*portc));

  portc->devc = devc;
  portc->port_type = PT_SERVER;

  if ((adev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
				    devc->osdev,
				    devc->osdev,
				    "User space audio device server side",
				    &userdev_server_driver,
				    sizeof (audiodrv_t),
				    opts, SUPPORTED_FORMATS, devc, -1)) < 0)
    {
      return adev;
    }

  audio_engines[adev]->portc = portc;
  audio_engines[adev]->min_rate = 5000;
  audio_engines[adev]->max_rate = MAX_RATE;
  audio_engines[adev]->min_channels = 1;
  audio_engines[adev]->max_channels = MAX_CHANNELS;
  audio_engines[adev]->vmix_mixer=NULL;
  strcpy(audio_engines[adev]->devnode, userdev_server_devnode);

  portc->audio_dev = adev;

  return adev;
}

static int 
null_mixer_init(int de)
{
	return 0;
}

static void
userdev_create_mixer(userdev_devc_t * devc)
{
  if ((devc->mixer_dev = oss_install_mixer (OSS_MIXER_DRIVER_VERSION,
				     devc->osdev,
				     devc->osdev,
				     "OSS userdev mixer",
				     &userdev_mixer_driver,
				     sizeof (mixer_driver_t), devc)) < 0)
    {
	devc->mixer_dev = -1;
	return;
    }
  mixer_ext_set_init_fn (devc->mixer_dev, null_mixer_init, USERDEV_MAX_MIXERS*2);
}

static int
install_client (userdev_devc_t * devc)
{
  userdev_portc_t *portc = &devc->client_portc;
  int adev;

  int opts =
    ADEV_STEREOONLY | ADEV_16BITONLY | ADEV_VIRTUAL | ADEV_DUPLEX |
    ADEV_FIXEDRATE | ADEV_SPECIAL | ADEV_LOOP;

  memset (portc, 0, sizeof (*portc));

  userdev_create_mixer(devc);

  portc->devc = devc;
  portc->port_type = PT_CLIENT;

  if (!userdev_visible_clientnodes && !(devc->create_flags & USERDEV_F_VMIX_PRIVATENODE))
  {
     opts |= ADEV_HIDDEN;
  }

  if ((adev = oss_install_audiodev (OSS_AUDIO_DRIVER_VERSION,
				    devc->osdev,
				    devc->osdev,
				    "User space audio device",
				    &userdev_client_driver,
				    sizeof (audiodrv_t),
				    opts, SUPPORTED_FORMATS, devc, -1)) < 0)
    {
      return adev;
    }

  if (!userdev_visible_clientnodes) /* Invisible client device nodes */
     strcpy(audio_engines[adev]->devnode, userdev_client_devnode);

  audio_engines[adev]->portc = portc;
  audio_engines[adev]->mixer_dev = devc->mixer_dev;
  audio_engines[adev]->min_rate = 5000;
  audio_engines[adev]->max_rate = MAX_RATE;
  audio_engines[adev]->min_channels = 1;
  audio_engines[adev]->max_channels = MAX_CHANNELS;

  portc->audio_dev = adev;
#ifdef CONFIG_OSS_VMIX
  vmix_attach_audiodev(devc->osdev, adev, -1, 0);
#endif

  return adev;
}

int
userdev_create_device_pair(void)
{
  int client_engine, server_engine;
  userdev_devc_t *devc;
  oss_native_word flags;

  if ((devc=PMALLOC(userdev_osdev, sizeof (*devc))) == NULL)
     return OSS_ENOMEM;
  memset(devc, 0, sizeof(*devc));

  devc->osdev = userdev_osdev;
  MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);
  devc->active=1;

  devc->rate = 48000;
  devc->fmt = AFMT_S16_NE;
  devc->fmt_bytes = 2;
  devc->channels = 2;
  devc->poll_ticks = 10;

  if ((server_engine=install_server (devc)) < 0)
     return server_engine;

  if ((client_engine=install_client (devc)) < 0)
	return client_engine;

  devc->client_portc.peer = &devc->server_portc;
  devc->server_portc.peer = &devc->client_portc;

  /*
   * Insert the device to the list of available devices
   */
  MUTEX_ENTER_IRQDISABLE(userdev_global_mutex, flags);
  devc->next_instance = userdev_active_device_list;
  userdev_active_device_list = devc;
  MUTEX_EXIT_IRQRESTORE(userdev_global_mutex, flags);

  return server_engine;
}

static void
userdev_free_device_pair (userdev_devc_t *devc)
{
  oss_native_word flags;

  set_adev_name(devc->client_portc.audio_dev, "User space audio device");
  set_adev_name(devc->server_portc.audio_dev, "User space audio device server side");

  MUTEX_ENTER_IRQDISABLE(userdev_global_mutex, flags);

  devc->match_method = 0;
  devc->match_key = 0;

  /*
   * Add to the free device pair list.
   */
  devc->next_instance = userdev_free_device_list;
  userdev_free_device_list = devc;

  /*
   * Remove the device pair from the active device list.
   */

  if (userdev_active_device_list == devc) /* First device in the list */
     {
	     userdev_active_device_list = userdev_active_device_list->next_instance;
     }
  else
     {
	     userdev_devc_t *this = userdev_active_device_list, *prev = NULL;

	     while (this != NULL)
	     {
		     if (this == devc)
			{
				prev->next_instance = this->next_instance; /* Remove */
				break;
			}

		     prev = this;
		     this = this->next_instance;
	     }
     }
  MUTEX_EXIT_IRQRESTORE(userdev_global_mutex, flags);
}

void
userdev_reinit_instance(userdev_devc_t *devc)
{
	if (devc->mixer_dev < 0)
	   return;

  mixer_ext_rebuild_all (devc->mixer_dev, null_mixer_init, USERDEV_MAX_MIXERS*2);
}

void
userdev_delete_device_pair(userdev_devc_t *devc)
{
  if (!devc->active)
     return;

  devc->active = 0;
  MUTEX_CLEANUP(devc->mutex);
}

int
usrdev_find_free_device_pair(void)
{
  oss_native_word flags;
  userdev_devc_t *devc;

  MUTEX_ENTER_IRQDISABLE(userdev_global_mutex, flags);

  if (userdev_free_device_list != NULL)
     {
	devc = userdev_free_device_list;
	userdev_free_device_list = userdev_free_device_list->next_instance;

  	devc->next_instance = userdev_active_device_list;
  	userdev_active_device_list = devc;

  	MUTEX_EXIT_IRQRESTORE(userdev_global_mutex, flags);
	return devc->server_portc.audio_dev;
     }
  MUTEX_EXIT_IRQRESTORE(userdev_global_mutex, flags);

  return OSS_ENXIO;
}
