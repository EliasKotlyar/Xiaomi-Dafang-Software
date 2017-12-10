/*
 * Purpose: Common definitions for the ACME Labs Evil Audio driver.
 *
 * The intial comment usually doesn't contain much information.
 */

/*
 * As in the C sources you need to include a placeholder define for the 
 * copyright notice. To avoid getting multiple define warnings for the COPYING
 * macro the header files should use macro name like COPYING2..COPYING9.
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

/*
 * Each device instance should have a per-device data structure that contains
 * variables common to all sub-devices of the card. By convenntion this
 * structure is called devc. Driver designers may use different terminology.
 * However use of devc is highly recomended in all OSS drivers because it
 * will make maintenance of the code easier.
 */

typedef struct _myossdev_devc_t *myoss_devc_t;

struct _myossdev_devc_t
{
  oss_device_t *osdev;		/* A handle to the device given by the OSS core. */
  oss_mutex_t *mutex;		/* A lock/mutex variable for the device */
};
