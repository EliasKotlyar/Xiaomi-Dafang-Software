/*
 * Purpose: OSS core pseudo driver (for Solaris)
 *
 * The osscore driver is used under Solaris to load the configuration settings 
 * (osscore.conf) and to install the /dev/sndstat device.
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

#include "osscore_cfg.h"

int
osscore_attach (oss_device_t * osdev)
{
  oss_register_device (osdev, "OSS common devices");
  install_sndstat (osdev);
  install_dev_mixer (osdev);
  return 1;
}

int
osscore_detach (oss_device_t * osdev)
{
  if (oss_disable_device (osdev) < 0)
    return 0;

  oss_unregister_device (osdev);

  return 1;
}
