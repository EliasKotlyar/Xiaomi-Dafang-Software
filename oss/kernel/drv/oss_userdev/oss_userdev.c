/*
 * Purpose: Kernel space support module for user land audio/mixer drivers
 *
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

oss_device_t *userdev_osdev = NULL;
static int client_dev = -1, server_dev = -1; /* Control devces */

char *userdev_client_devnode = "/dev/oss/oss_userdev0/client";
char *userdev_server_devnode = "/dev/oss/oss_userdev0/server";

/*
 * Global device lists and the mutex that protects them.
 */
oss_mutex_t userdev_global_mutex;

/*
 * The oss_userdev driver creates new device pairs on-demand. All device
 * pairs that are not in use will be kept in the userdev_free_device_list
 * (linked) list. If this list contains any entries then they will be
 * reused whenever a new device pair is required.
 */
userdev_devc_t *userdev_free_device_list = NULL;

/*
 * Linked list for device pairs that have a server attached. These device 
 * pairs are available for the clients.
 */
userdev_devc_t *userdev_active_device_list = NULL;

/*ARGSUSED*/
static int
userdev_server_redirect (int dev, int mode, int open_flags)
{
/*
 * Purpose: This entry point is used to create new userdev instances and to redirect clients to them.
 */
  int server_engine;


  if ((server_engine=usrdev_find_free_device_pair()) >= 0)
  {
     userdev_devc_t *devc = audio_engines[server_engine]->devc;

     userdev_reinit_instance(devc);
     return server_engine;
  }
  
  return userdev_create_device_pair();
}

/*ARGSUSED*/
static int
userdev_client_redirect (int dev, int mode, int open_flags)
{
/*
 * Purpose: This entry point is used to create new userdev instances and to redirect clients to them.
 */

  userdev_devc_t *devc;
  oss_native_word flags;

  uid_t uid;

  uid = oss_get_procinfo(OSS_GET_PROCINFO_UID);

  MUTEX_ENTER_IRQDISABLE(userdev_global_mutex, flags);
  devc=userdev_active_device_list;

  while (devc != NULL)
  {
	  int ok=1;

	  switch (devc->match_method)
	  {
	  case UD_MATCH_UID:
		if (devc->match_key != uid)	/* Wrong UID */
		   ok=0;
		break;
	  }

	  if (ok)
	     {
  	  	MUTEX_EXIT_IRQRESTORE(userdev_global_mutex, flags);
	  	return devc->client_portc.audio_dev;
	     }

	  devc = devc->next_instance;
  }

  MUTEX_EXIT_IRQRESTORE(userdev_global_mutex, flags);
  return OSS_EIO;
}

/*
 * Dummy audio driver entrypoint functions. 
 *
 * Functionality of the control device is handled by userdev_[client|server]_redirect().
 * The other entry points are not used for any purpose but the audio core
 * framework expects to see them.
 */
/*ARGSUSED*/
static int
userdev_control_set_rate (int dev, int arg)
{
  /* Dumy routine - Not actually used */
  return 48000;
}

/*ARGSUSED*/
static short
userdev_control_set_channels (int dev, short arg)
{
  /* Dumy routine - Not actually used */
	return 2;
}

/*ARGSUSED*/
static unsigned int
userdev_control_set_format (int dev, unsigned int arg)
{
  /* Dumy routine - Not actually used */
	return AFMT_S16_NE;
}

static void
userdev_control_reset (int dev)
{
  /* Dumy routine - Not actually used */
}

/*ARGSUSED*/
static int
userdev_control_open (int dev, int mode, int open_flags)
{
  /* Dumy routine - Not actually used */
  return OSS_EIO;
}

/*ARGSUSED*/
static void
userdev_control_close (int dev, int mode)
{
  /* Dumy routine - Not actually used */
}

/*ARGSUSED*/
static int
userdev_control_ioctl (int dev, unsigned int cmd, ioctl_arg arg)
{
  /* Dumy routine - Not actually used */
  return OSS_EINVAL;
}

/*ARGSUSED*/
static void
userdev_control_output_block (int dev, oss_native_word buf, int count, int fragsize,
			int intrflag)
{
  /* Dumy routine - Not actually used */
}

/*ARGSUSED*/
static void
userdev_control_start_input (int dev, oss_native_word buf, int count, int fragsize,
		       int intrflag)
{
  /* Dumy routine - Not actually used */
}

/*ARGSUSED*/
static int
userdev_control_prepare_for_input (int dev, int bsize, int bcount)
{
  /* Dumy routine - Not actually used */
	return OSS_EIO;
}

/*ARGSUSED*/
static int
userdev_control_prepare_for_output (int dev, int bsize, int bcount)
{
  /* Dumy routine - Not actually used */
  return OSS_EIO;
}

static audiodrv_t userdev_server_control_driver = {
  userdev_control_open,
  userdev_control_close,
  userdev_control_output_block,
  userdev_control_start_input,
  userdev_control_ioctl,
  userdev_control_prepare_for_input,
  userdev_control_prepare_for_output,
  userdev_control_reset,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL, /* trigger */
  userdev_control_set_rate,
  userdev_control_set_format,
  userdev_control_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  userdev_server_redirect
};

static audiodrv_t userdev_client_control_driver = {
  userdev_control_open,
  userdev_control_close,
  userdev_control_output_block,
  userdev_control_start_input,
  userdev_control_ioctl,
  userdev_control_prepare_for_input,
  userdev_control_prepare_for_output,
  userdev_control_reset,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL, /* trigger */
  userdev_control_set_rate,
  userdev_control_set_format,
  userdev_control_set_channels,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  userdev_client_redirect
};

static void
attach_control_device(void)
{
/*
 * Create the control device files that are used to create client/server
 * device pairs and to redirect access to them.
 */
  userdev_devc_t *devc = (userdev_devc_t*)0xdeadcafe; /* This should never get referenced */

  if ((client_dev = oss_install_audiodev_with_devname (OSS_AUDIO_DRIVER_VERSION,
				    userdev_osdev,
				    userdev_osdev,
				    "User space audio device client side",
				    &userdev_client_control_driver,
				    sizeof (audiodrv_t),
				    ADEV_AUTOMODE, AFMT_S16_NE, devc, -1,
				    "client")) < 0)
    {
      return;
    }
  userdev_server_devnode = audio_engines[server_dev]->devnode;
  audio_engines[client_dev]->vmix_mixer=NULL;

  if ((server_dev = oss_install_audiodev_with_devname (OSS_AUDIO_DRIVER_VERSION,
				    userdev_osdev,
				    userdev_osdev,
				    "User space audio device server side",
				    &userdev_server_control_driver,
				    sizeof (audiodrv_t),
				    ADEV_AUTOMODE, AFMT_S16_NE, devc, -1, 
				    "server")) < 0)
    {
      return;
    }
  audio_engines[server_dev]->caps |= PCM_CAP_HIDDEN;
  audio_engines[server_dev]->vmix_mixer=NULL;
  userdev_client_devnode = audio_engines[client_dev]->devnode;
}

int
oss_userdev_attach (oss_device_t * osdev)
{
  userdev_osdev = osdev;

  osdev->devc = NULL;
  MUTEX_INIT (osdev, userdev_global_mutex, MH_DRV);

  oss_register_device (osdev, "User space audio driver subsystem");

  attach_control_device();

  return 1;
}

int
oss_userdev_detach (oss_device_t * osdev)
{
  userdev_devc_t *devc;

  if (oss_disable_device (osdev) < 0)
    return 0;

  devc = userdev_free_device_list;

  while (devc != NULL)
  {
	  userdev_devc_t *next = devc->next_instance;

	  userdev_delete_device_pair(devc);

	  devc = next;
  }

  oss_unregister_device (osdev);

  MUTEX_CLEANUP(userdev_global_mutex);

  return 1;
}
