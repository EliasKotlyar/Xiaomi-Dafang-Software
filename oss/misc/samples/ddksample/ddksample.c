/*
 * ddksample.c: OSS DDK sample driver - attach/detach
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
 * Solaris DDI includes
 */
#include <sys/types.h>
#include <sys/modctl.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>

/*
 * OSS specific includes
 */
#include <sys/soundcard.h>
#include <sys/ossddk/ossddk.h>

/***************************************************
 * Private types and variables
 */

#define DRIVER_NICK	"ddksample"
#define DRIVER_FULLNAME "OSS DDK sample device"
#define DRIVER_TYPE	DRV_VIRTUAL

#include "ddksample.h"

/**************************************************/

static int ddksample_getinfo (dev_info_t *, ddi_info_cmd_t, void *, void **);
static int ddksample_attach (dev_info_t *, ddi_attach_cmd_t);
static int ddksample_detach (dev_info_t *, ddi_detach_cmd_t);

/* Entry points structure */
static struct cb_ops ddksample_cb_ops = {
  oss_open,
  oss_close,
  nodev,			/* not a block driver   */
  nodev,			/* no print routine     */
  nodev,			/* no dump routine      */
  oss_read,
  oss_write,
  oss_ioctl,
  nodev,			/* no devmap routine    */
  oss_mmap,			/* mmap routine */
  nodev,			/* no segmap routine    */
  oss_chpoll,			/* no chpoll routine    */
  ddi_prop_op,
  0,				/* not a STREAMS driver */
  D_NEW | D_MP | D_64BIT,	/* safe for multi-thread/multi-processor */
  CB_REV
};

static struct dev_ops ddksample_dev_ops = {
  DEVO_REV,			/* devo_rev */
  0,				/* devo_refcnt */
  ddksample_getinfo,		/* devo_getinfo */
  nulldev,			/* devo_identify - obsolete */
#if DRIVER_TYPE==DRV_ISA
  ddksample_probe,
#else
  nulldev,			/* devo_probe */
#endif
  ddksample_attach,		/* devo_attach */
  ddksample_detach,		/* devo_detach */
  nodev,			/* devo_reset */
#if DRIVER_TYPE==DRV_STREAMS
  &ddksample_streams_cb_ops,	/* devi_cb_ops */
#else
  &ddksample_cb_ops,		/* devi_cb_ops */
#endif
  NULL,				/* devo_bus_ops */
  NULL				/* devo_power */
};

static struct modldrv ddksample_modldrv = {
  &mod_driverops,		/* drv_modops */
  "Sample OSS DDK virtual driver",	/* String "OSS" must be included in the name */
  &ddksample_dev_ops,		/* drv_dev_ops */
};

static struct modlinkage ddksample_modlinkage = {
  MODREV_1,			/* ml_rev */
  (void *) &ddksample_modldrv,	/* ml_linkage */
  NULL				/* NULL terminates the list */
};

/*
 * _init, _info, and _fini support loading and unloading the driver.
 */
int
_init (void)
{
  int error;

  error = mod_install (&ddksample_modlinkage);

  return error;
}

int
_fini (void)
{
  int error;

  error = mod_remove (&ddksample_modlinkage);
  return error;
}

int
_info (struct modinfo *modinfop)
{
  int error;

  error = mod_info (&ddksample_modlinkage, modinfop);

  return error;
}

/**************************************************/


static int
ddksample_attach (dev_info_t * dip, ddi_attach_cmd_t cmd)
{
  int instance, err;
  oss_device_t *osdev;
  ddi_iblock_cookie_t iblock_cookie;
  ddksample_devc *devc;

  if (cmd != DDI_ATTACH)
    {
      cmn_err (CE_WARN, "bad attach cmd %d\n", cmd);
      return 0;
    }

  if (dip == NULL)
    {
      cmn_err (CE_WARN, "ddksample_attach: dip==NULL\n");
      return DDI_FAILURE;
    }

  instance = ddi_get_instance (dip);
  cmn_err (CE_CONT, "Attach started " DRIVER_NICK "%d\n", instance);

#if 0
  // Pseudo drivers don't have any iblock_cookie

  if ((err = ddi_get_iblock_cookie (dip, 0, iblock_cookie)) != DDI_SUCCESS)
    {
      cmn_err (CE_WARN, "Cannot get iblock cookie (%d)\n", err);
      return DDI_FAILURE;
    }
#endif

  devc = kmem_zalloc (sizeof (*devc), KM_SLEEP);

  mutex_init (&devc->mutex, NULL, MUTEX_DRIVER, NULL /* iblock_cookie */ );

  if ((osdev =
       ossddk_register_device (OSSDDK_VERSION, dip, DRIVER_TYPE, instance,
			       DRIVER_NICK, iblock_cookie, devc,
			       DRIVER_FULLNAME)) == NULL)
    {
      cmn_err (CE_WARN, "Registering OSS DDK driver failed\n");
      return DDI_FAILURE;
    }

  devc->osdev = osdev;

  ddksample_mixer_init (devc);	/* From ddksample_mixer.c */
  ddksample_audio_init (devc);	/* From ddksample_audio.c */

  ddi_set_driver_private (dip, (caddr_t) osdev);
  ddi_report_dev (dip);

  return DDI_SUCCESS;
}

static int
ddksample_detach (dev_info_t * dip, ddi_detach_cmd_t cmd)
{
  oss_device_t *osdev;
  int err;

  ddksample_devc *devc;

  if (cmd != DDI_DETACH)
    {
      cmn_err (CE_WARN, "bad attach cmd %d\n", cmd);
      return 0;
    }

  if (dip == NULL)
    {
      cmn_err (CE_WARN, "ddksample_detach: dip==NULL\n");
      return DDI_FAILURE;
    }

  cmn_err (CE_CONT, "Detach started " DRIVER_NICK "\n");
  if ((osdev = ddi_get_driver_private (dip)) == NULL)
    {
      cmn_err (CE_WARN, "ddi_get_driver_private() failed\n");
      return DDI_SUCCESS;
    }

/*
 * Check if the device can be removed (not busy) and make sure it will not be
 * opened any more.
 */
  if ((err = ossddk_disable_device (osdev)) < 0)
    return DDI_FAILURE;

  devc = ossddk_osdev_get_devc (osdev);

/*
 * Stop the device and make sure it will not raise any
 * interrupts any morfe.
 */

  ossddk_unregister_device (osdev);

  mutex_destroy (&devc->mutex);
  kmem_free (devc, sizeof (*devc));

  return DDI_SUCCESS;
}

/*
 * Misc routines
 */

static int
ddksample_getinfo (dev_info_t * dip, ddi_info_cmd_t cmd, void *arg,
		   void **result)
{
  dev_t dev;
  register int error;
  int minor_num, instance;

  if (dip == NULL)
    {
      cmn_err (CE_WARN, "ddksample_getinfo: dip==NULL\n");
      return DDI_FAILURE;
    }

  dev = (dev_t) arg;
  minor_num = getminor (dev);
  instance = ddi_get_instance (dip);

  switch (cmd)
    {
    case DDI_INFO_DEVT2DEVINFO:
      *result = dip;
      error = DDI_SUCCESS;
      break;
    case DDI_INFO_DEVT2INSTANCE:
      *result = (void *) instance;
      error = DDI_SUCCESS;
      break;
    default:
      *result = NULL;
      error = DDI_FAILURE;
    }

  return (error);
}
