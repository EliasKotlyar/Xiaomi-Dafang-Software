/*
 * Purpose: Driver for the ACME Laboratories Evil Audio family of sound cards
 * Description:
 * This description section contains general information about the driver
 * and the name and contact information of author and maintainer of the code.
 *
 * _NO_ copyright statement must be given in the comments since all drivers
 * to be included in the OSS source tree must be compatible with the copying 
 * policy of the rest of OSS.
 */

/*
 * Create a #define for the copying conditions. This define must be
 * as above. Only the name of the copyright owner and the year(s) can be
 * changed. This definition will be replaced by the actual OSS copyright
 * statement by the build tools.
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
 * The _cfg.h file for the driver must be included before anything else
 * by all OS drivers. This configuration file will be automatically generated
 * based on the configuration files and operating system requirements.
 */
#include "myossdev_cfg.h"

/*
 * If the driver is split to multiple C source files then you should create
 * a common header file for the driver and include it here.
 */

#include "myossdev.h"

/*
 * Drivers for PCI devices need to include <oss_pci.h>. Other drivers should
 * not include it.
 */
#include <oss_pci.h>


/*
 * The attach routine for the device. This routine will be called once for
 * each device instance found from the system. The osdev parameter is a handle
 * to the device that will be needed by many subsequent OSS core functions.
 */
int
myossdev_attach (oss_device_t * osdev)
{
  myossdev_devc_t *devc;

  /*
   * Some debugging printout. All OSS drivers must use cmn_err() instead 
   * of printf(), printk() or something else to print debugging and error
   * messages.
   *
   * The cmn_err() call can be oput inside a DDB() macro. In this way the
   * message will only be printed if OSS was started with detect_trace=1
   * configuration option.
   */
  DDB (cmn_err (CE_CONT, "myossdev_attach called\n"));

  /*
   * First create the devc structure and initialize it to zeroes.
   * The PMALLOC macro will allocate memory that will be automatically
   * released when OSS gets unloaded. This kind of memory should be used
   * for structures that will stay in use until the driver is unloaded.
   */

  if ((devc = PMALLOC (osdev, sizeof (*devc))) == NULL)
    {
      cmn_err (CE_WARN, "Cannot allocate devc\n");
      return 0;			/* Failure */
    }

  memset (devc, 0, sizeof (*devc));

  /*
   * Drivers must initialize osdev to have a link to the devc structure.
   * Without this link the driver cannot be unloaded.
   */
  osdev->devc = devc;

  /*
   * Init the devc fields and create the mutex. Register the device.
   */

  devc->osdev = osdev;
  MUTEX_INIT (devc->osdev, devc->mutex);
  oss_register_device (osdev, "ACME Labs Evil Audio");

  /*
   * This is all for now. More will follow in the next episode.
   */

  return 1;			/* Success */
}

/*
 * Device detach routine. Note that this routine will be called once for
 * each instance.
 */

int
myossdev_detach (oss_device_t * osdev)
{
  myossdev_devc_t *devc = osdev->devc;
  int err;

  DDB (cmn_err (CE_CONT, "myossdev_detach called\n"));

/*
 * Check if the device is not busy.
 */
  if ((err = oss_disable_device (osdev)) < 0)
    {
      cmn_err (CE_WARN, "Cannot detach %s\n", osdev->nick);
      return 0;
    }

  /*
   * Uninitialize the instance variables. Do not try to free any
   * memory allocated with PMALLOC().
   */

  MUTEX_CLEANUP (devc->mutex);	/* Mutexes must be cleared */

  /*
   * Finally mark the device as inactive
   */
  oss_unregister_device (osdev);

  return 1;			/* Ok */
}
