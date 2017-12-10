/*
 * Purpose: OSS module wrapper for Solaris
 *
 * This file will be included from the auto-generated drv_cfg.c files. Under
 * Solaris this file will be compiled in the development system.
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
#include <sys/types.h>
#include <sys/modctl.h>
#include <sys/kmem.h>
#include <sys/conf.h>
#include <sys/ddi.h>
#include <sys/sunddi.h>

static int ossdrv_getinfo (dev_info_t *, ddi_info_cmd_t, void *, void **);
static int ossdrv_attach (dev_info_t *, ddi_attach_cmd_t);
static int ossdrv_detach (dev_info_t *, ddi_detach_cmd_t);

#ifdef OSS_POWER_MANAGE
static int ossdrv_power (dev_info_t *, int component, int level);
#endif

/* Entry points structure */
#if DRIVER_TYPE!=DRV_STREAMS

static struct cb_ops ossdrv_cb_ops = {
  oss_open,
  oss_close,
  nodev,			/* not a block driver   */
  nodev,			/* no print routine     */
  nodev,			/* no dump routine      */
  oss_read,
  oss_write,
  oss_ioctl,
#ifdef ALLOW_BUFFER_MAPPING
  oss_devmap,
#else
  nodev,			/* no devmap routine    */
#endif
  nodev,
  nodev,			/* no segmap routine    */
  oss_chpoll,			/* no chpoll routine    */
  ddi_prop_op,
  0,				/* not a STREAMS driver */
  D_NEW | D_MP | D_64BIT,	/* safe for multi-thread/multi-processor */
  CB_REV
};
#else
extern struct streamtab DRIVER_STR_INFO;

struct cb_ops ossdrv_streams_cb_ops = {
  nulldev,
  nulldev,
  nodev,			/* not a block driver   */
  nodev,			/* no print routine     */
  nodev,			/* no dump routine      */
  nodev,
  nodev,
  nodev,
  nodev,			/* no devmap routine    */
  nodev,
  nodev,			/* no segmap routine    */
  nochpoll,			/* no chpoll routine    */
  ddi_prop_op,
  &DRIVER_STR_INFO,		/* cb_str */
  D_NEW | D_MP | D_64BIT,	/* safe for multi-thread/multi-processor */
  CB_REV,
  nodev,			/* cb_aread */
  nodev,			/* cb_awrite */
};
#endif

static struct dev_ops ossdrv_dev_ops = {
  DEVO_REV,			/* devo_rev */
  0,				/* devo_refcnt */
  ossdrv_getinfo,		/* devo_getinfo */
  nulldev,			/* devo_identify - obsolete */
#if DRIVER_TYPE==DRV_ISA
  ossdrv_probe,
#else
  nulldev,			/* devo_probe */
#endif
  ossdrv_attach,		/* devo_attach */
  ossdrv_detach,		/* devo_detach */
  nodev,			/* devo_reset */
#if DRIVER_TYPE==DRV_STREAMS
  &ossdrv_streams_cb_ops,	/* devi_cb_ops */
#else
  &ossdrv_cb_ops,		/* devi_cb_ops */
#endif
  NULL,				/* devo_bus_ops */
#ifdef OSS_POWER_MANAGE
  ossdrv_power
#else
  NULL				/* devo_power */
#endif
};

static struct modldrv ossdrv_modldrv = {
  &mod_driverops,		/* drv_modops */
  "OSS " OSS_VERSION_STRING,
  &ossdrv_dev_ops,		/* drv_dev_ops */
};

static struct modlinkage ossdrv_modlinkage = {
  MODREV_1,			/* ml_rev */
  (void *) &ossdrv_modldrv,	/* ml_linkage */
  NULL				/* NULL terminates the list */
};

/*
 * _init, _info, and _fini support loading and unloading the driver.
 */
int
_init (void)
{
  int error;

  error = mod_install (&ossdrv_modlinkage);

  return error;
}

int
_fini (void)
{
  int error;

  error = mod_remove (&ossdrv_modlinkage);
  return error;
}

int
_info (struct modinfo *modinfop)
{
  int error;

  error = mod_info (&ossdrv_modlinkage, modinfop);

  return error;
}

/*ARGSUSED*/
static int
ossdrv_getinfo (dev_info_t * dontcare_dip, ddi_info_cmd_t cmd, void *arg,
		void **result)
{
  dev_t dev;
  register int error;
  int instance;
  dev_info_t *dip;
#ifndef SOL9
  oss_cdev_t *cdev;
  int minor;

  dev = (dev_t) arg;

  minor = getminor (dev);

  if (minor >= oss_num_cdevs)
   {
        *result = NULL;
        return DDI_FAILURE;
   }

  if ((cdev = oss_cdevs[minor]) == NULL || cdev->osdev == NULL)
   {
        *result = NULL;
        return DDI_FAILURE;
   }

  dip = cdev->osdev->dip;
#else
  dip = dontcare_dip;
#endif

  if (dip == NULL)
    {
      /* cmn_err (CE_WARN, "ossdrv_getinfo: dip==NULL\n"); */
      return DDI_FAILURE;
    }

  instance = ddi_get_instance (dip);

  switch (cmd)
    {
    case DDI_INFO_DEVT2DEVINFO:
      *result = dip;
      error = DDI_SUCCESS;
      break;
    case DDI_INFO_DEVT2INSTANCE:
      *result = (void *) (long)instance;
      error = DDI_SUCCESS;
      break;
    default:
      *result = NULL;
      error = DDI_FAILURE;
    }

  DDB (cmn_err (CE_CONT,
		"oss_getinfo: returns %d, result=%lx instance=%d dev=%x\n",
		error, (unsigned long)*result, instance, (unsigned int)dev));
  return error;
}

static int
ossdrv_attach (dev_info_t * dip, ddi_attach_cmd_t cmd)
{
  int instance;
  oss_device_t *osdev;

#ifdef OSS_SUSPEND_RESUME
  if (cmd != DDI_RESUME) /* Allow resume */
#endif
  if (cmd != DDI_ATTACH)
    {
      cmn_err (CE_WARN, "bad attach cmd %d\n", cmd);
      return 0;
    }

  if (dip == NULL)
    {
      cmn_err (CE_WARN, "ossdrv_attach: dip==NULL\n");
      return DDI_FAILURE;
    }

#ifdef OSS_SUSPEND_RESUME
  if (cmd == DDI_RESUME)
     {
	  if ((osdev = (oss_device_t *) ddi_get_driver_private (dip)) == NULL)
	    {
	      cmn_err (CE_WARN, "ddi_get_driver_private() failed\n");
	      return DDI_SUCCESS;
	    }
  	if (!DRIVER_RESUME(osdev))
     	   return DDI_FAILURE;

	return DDI_SUCCESS;
     }
#endif

  instance = ddi_get_instance (dip);
  DDB (cmn_err
       (CE_CONT, "Attach started " DRIVER_NICK "%d (%s)\n", instance,
	ddi_get_name (dip)));

  if ((osdev =
       osdev_create (dip, DRIVER_TYPE, instance, DRIVER_NICK, NULL)) == NULL)
    {
      return DDI_FAILURE;
    }

  oss_load_options (osdev, oss_global_options);
  oss_load_options (osdev, local_driver_options);

#if DRIVER_TYPE==DRV_PCI || DRIVER_TYPE == DRV_ISA
  {
  int err;
  if ((err =
       ddi_get_iblock_cookie (dip, 0, &osdev->iblock_cookie)) != DDI_SUCCESS)
    {
      cmn_err (CE_WARN, "Cannot get iblock cookie (%d)\n", err);
      return DDI_FAILURE;
    }
  }
#else
/* NOTE! The USB driver (actually udi.c) will call usb_get_data() to obtain */
/*       the iblock_cookie. There is no need to do that here. */
  osdev->iblock_cookie = 0;
#endif

  if (!DRIVER_ATTACH (osdev))
    {
      cmn_err (CE_WARN, "Attach failed\n");
      osdev_delete (osdev);
      return DDI_FAILURE;
    }

  ddi_set_driver_private (dip, (caddr_t) osdev);
  ddi_report_dev (dip);
#if DRIVER_TYPE != DRV_USB
  oss_audio_delayed_attach ();
#endif

  DDB (cmn_err (CE_CONT, "Attach done " DRIVER_NICK "%d\n", instance));

  return DDI_SUCCESS;
}

static int
ossdrv_detach (dev_info_t * dip, ddi_detach_cmd_t cmd)
{
  oss_device_t *osdev;

#ifdef OSS_SUSPEND_RESUME
  if (cmd != DDI_SUSPEND) /* Allow suspend */
#endif
  if (cmd != DDI_DETACH)
    {
      cmn_err (CE_WARN, "bad detach cmd %d\n", cmd);
      return 0;
    }

  if (dip == NULL)
    {
      cmn_err (CE_WARN, "ossdrv_detach: dip==NULL\n");
      return DDI_FAILURE;
    }
//cmn_err(CE_CONT, "Detach started " DRIVER_NICK "\n");

  DDB (cmn_err
       (CE_CONT, "Detach started " DRIVER_NICK "(%s)\n", ddi_get_name (dip)));
  if ((osdev = (oss_device_t *) ddi_get_driver_private (dip)) == NULL)
    {
      cmn_err (CE_WARN, "ddi_get_driver_private() failed\n");
      return DDI_SUCCESS;
    }

  if (osdev==NULL)
    {
      cmn_err (CE_WARN, "Driver osdev==NULL - cannot detach\n");
      return DDI_FAILURE;
    }

#ifdef OSS_SUSPEND_RESUME
  if (cmd == DDI_SUSPEND)
  {
     if (!DRIVER_SUSPEND(osdev))
        return DDI_FAILURE;
     return DDI_SUCCESS;
  }
#endif

  if (DRIVER_DETACH (osdev) <= 0)
    {
      cmn_err (CE_WARN, "Driver busy - cannot detach\n");
      return DDI_FAILURE;
    }

#if DRIVER_TYPE!=DRV_VMIX
  osdev_delete (osdev);
#endif

  DDB (cmn_err (CE_CONT, "Detach done " DRIVER_NICK "\n"));
  return DDI_SUCCESS;
}

#ifdef OSS_POWER_MANAGE
static int
ossdrv_power (dev_info_t *dip, int component, int level)
{
  oss_device_t *osdev;

  if (dip == NULL)
    {
      cmn_err (CE_WARN, "ossdrv_detach: dip==NULL\n");
      return DDI_FAILURE;
    }

  DDB (cmn_err
       (CE_CONT, "ossdrv_power " DRIVER_NICK "(%s, %d, %d)\n", ddi_get_name (dip), component, level));
  if ((osdev = (oss_device_t *) ddi_get_driver_private (dip)) == NULL)
    {
      cmn_err (CE_WARN, "ddi_get_driver_private() failed\n");
      return DDI_SUCCESS;
    }

  if (osdev==NULL)
    {
      cmn_err (CE_WARN, "Driver osdev==NULL - cannot detach\n");
      return DDI_FAILURE;
    }

	if (!DRIVER_POWER(osdev, component, level))
	   return DDI_FAILURE;

	return DDI_SUCCESS;
}
#endif
