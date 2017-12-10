#ifndef MIXER_CORE_H
#define MIXER_CORE_H

/*
 * Purpose: Mixer specific internal structure and function definitions.
 *
 * IMPORTANT NOTICE!
 *
 * This file contains internal structures used by Open Sound Systems.
 * They will change without any notice between OSS versions. Care must be taken
 * to make sure any software using this header gets properly re-compiled before
 * use.
 *
 * 4Front Technologies (or anybody else) takes no responsibility of damages
 * caused by use of this file.
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

typedef struct
{
  oss_mixext ext;
  mixer_ext_fn handler;
  oss_mixer_enuminfo *enum_info;
  char *description; /* For "tooltips" */
}
oss_mixext_desc;

struct _mixer_operations_t
{
  oss_devname_t name;
  oss_id_t id;

  mixer_driver_t *d;

  void *devc;
  void *hw_devc;
  int modify_counter;

  /* Mixer extension interface */
  int nr_extra_ext;
  mixer_create_controls_t create_controls;
  int nr_ext;
  int names_checked;
  int max_ext;
  int timestamp;
  oss_mixext_desc *extensions;
  int ignore_mask;		/* Controls ignored by mixer ext API */

  /* Misc info */
  int card_number;
  int port_number;
  int enabled;
  int unloaded;
  int magic;
  int caps;
  int flags;
  oss_handle_t handle;
  oss_device_t *osdev;
  oss_device_t *master_osdev;	/* osdev struct of the master device (for virtual drivers) */
  void *os_id;			/* The device ID (dip) given by the system. */

/*
 * Priority is used for selecting the preferred mixer (usually the motherboard
 * device). By default the priority is 0. Value of -2 means that this mixer
 * should not be used as the default mixer. -1 means that this mixer can
 * be used as the default mixer only if no better mixer exists. priority=10
 * is used for known motherboard devices. 2 can be used by non-motherboard
 * mixers that support the usual main volume setting
 * (SNDCTL_MIXER_READ/WRITE_MAINVOL). 3-9 are reserved. Values higher than 10
 * can only be set manually by the user.
 */
  int priority;
  oss_devnode_t devnode;
  int real_dev;			/* Legacy device mapping */

  /* 
   * Virtual mixer extension support (optional)
   */
  void *vmix_devc;
  mixer_create_controls_t create_vmix_controls;
};

typedef struct _mixer_operations_t mixdev_t, *mixdev_p;

extern mixer_operations_t **mixer_devs;
extern void *mixer_devs_p;
extern int num_mixers;
extern void touch_mixer (int dev);
extern int oss_mixer_ext (int orig_dev, int dev_class, unsigned int cmd,
			  ioctl_arg arg);
extern int mixer_ext_set_enum (oss_mixer_enuminfo * ent);

extern char mix_cvt[];

int oss_install_mixer (int vers,
		       oss_device_t * osdev,
		       oss_device_t * master_osdev,
		       const char *name,
		       mixer_driver_t * driver, int driver_size, void *devc);

extern int oss_legacy_mixer_ioctl (int mixdev, int audiodev, unsigned int cmd,
				   ioctl_arg arg);
extern int mixer_ext_create_group (int dev, int parent, const char *id);
extern int mixer_ext_create_group_flags (int dev, int parent, const char *id,
					 unsigned int flags);
extern int mixer_ext_create_control (int dev, int parent, int ctrl,
				     mixer_ext_fn func, int type,
				     const char *id, int maxvalue, int flags);
extern int mixer_ext_rw (int dev, int ctrl, unsigned int cmd, int value);
extern int mixer_ext_recrw (int dev, int ctrl, unsigned int cmd, int value);
extern int mixer_ext_set_init_fn (int dev, mixer_create_controls_t func,
				  int nextra);
extern int mixer_ext_rebuild_all (int dev, mixer_create_controls_t func, int nextra);
/*
 * mixer_ext_set_vmix_init_fn() is reserved for the vmix driver to create
 * additional virtual mixing related mixer controls.
 */
extern int mixer_ext_set_vmix_init_fn (int dev, mixer_create_controls_t func,
				       int nextra, void *devc);
oss_mixext *mixer_find_ext (int dev, int enumber);
extern int mixer_ext_truncate (int dev, int index);
extern int mixer_ext_set_strings (int dev, int ctl, const char *s,
				  int version);
extern int mixer_ext_set_description (int dev, int ctrl, const char *desc);
extern int oss_mixer_ioctl (int dev, struct fileinfo *bogus, unsigned int cmd,
			    ioctl_arg arg);
#endif
