/*
 * Purpose: OSS module wrapper for SCO OpenServer/UnixWare
 *
 * This file will be included from the auto-generated drv_cfg.c files. Under
 * UnixWare and OpenServer this will will be compiled during the initial build
 * of OSS (in the development system).
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

#include <errno.h>

static int ossdrv_config (cfg_func_t func, void *idata, rm_key_t key);
#if DRIVER_TYPE==DRV_VIRTUAL || DRIVER_TYPE==DRV_VMIX
oss_device_t *osdev = NULL;
#endif
/*
 * Driver information structures, for drv_attach().
 */
static const drvops_t oss_ops = {
  ossdrv_config,
  oss_open,
  oss_close,
  oss_devinfo,
  oss_biostart,
  oss_ioctl,
  NULL,				/* drvctl */
  NULL				/* mmap */
};

static const drvinfo_t oss_drvinfo = {
  &oss_ops,
  DRIVER_NICK,
  D_MP,				/* MP-safe */
  NULL,				/* Not a STREAMS driver */
  256				/* Must match $maxchan in Node file */
};

static int
rd_hex (char *s)
{
/*
 * Convert a 4 digit hexadecimal string to integer value 
 */
  int v = 0;
  int i;

  for (i = 0; i < 4; i++)
    {
      char c = *s++;

      if (c >= '0' && c <= '9')
	{
	  v = (v << 4) | (c - '0');
	  continue;
	}

      if (c >= 'A' && c <= 'F')
	{
	  v = (v << 4) | (c - 'A' + 10);
	  continue;
	}

      if (c >= 'a' && c <= 'f')
	{
	  v = (v << 4) | (c - 'a' + 10);
	  continue;
	}
    }

  return v;
}

static int
cfg_add (void *idata, rm_key_t key)
{
  int err;
  cm_args_t cma;
  cm_num_t btype;
  char id[32];
  int vendor, product;
  static int instance = 0;
  oss_device_t *osdev;

  cm_begin_trans (key, RM_READ);
  cma.cm_key = key;
  cma.cm_param = CM_BRDBUSTYPE;
  cma.cm_val = &btype;
  cma.cm_vallen = sizeof (btype);
  cma.cm_n = 0;
  err = cm_getval (&cma);
  cm_end_trans (key);

  if (err != 0 || btype != CM_BUS_PCI)
    {
      cmn_err (CE_WARN, "Bad BUS type %d\n", btype);
      return ENODEV;
    }

  if ((osdev =
       osdev_create (&key, DRIVER_TYPE, instance, DRIVER_NICK, NULL)) == NULL)
    {
      return EIO;
    }

  cm_begin_trans (key, RM_READ);
  cma.cm_key = key;
  cma.cm_param = CM_BRDID;
  cma.cm_val = id;
  cma.cm_vallen = sizeof (id);
  cma.cm_n = 0;
  err = cm_getval (&cma);
  cm_end_trans (key);

  vendor = rd_hex (id + 2);
  product = rd_hex (id + 6);

  osdev->vendor = vendor;
  osdev->product = product;
  osdev->drvinfo = (drvinfo_t *) & oss_drvinfo;
  osdev->key = key;

  if (!DRIVER_ATTACH (osdev))
    {
      cmn_err (CE_WARN, "Attach failed\n");
      osdev_delete (osdev);
      return EIO;
    }

  *(void **) idata = osdev;
  oss_audio_delayed_attach ();

  instance++;

  return 0;
}

static int
cfg_remove (void *idata)
{
  oss_device_t *osdev = idata;

  if (idata == NULL)
    {
      cmn_err (CE_WARN, DRIVER_NICK ": ossdrv_detach: dip==NULL\n");
      return EIO;
    }

  if (osdev == NULL)
    {
      cmn_err (CE_WARN, DRIVER_NICK ": Bad idatata\n");
      return 0;
    }

  if (DRIVER_DETACH (osdev) <= 0)
    {
      DDB (cmn_err (CE_WARN, "Driver busy - cannot detach\n"));
      return EBUSY;
    }

  osdev_delete (osdev);

  DDB (cmn_err (CE_CONT, "Detach done " DRIVER_NICK, "\n"));
  return 0;
}

#if DRIVER_TYPE==DRV_VIRTUAL || DRIVER_TYPE==DRV_VMIX
static int
attach_virtual (void)
{
  DDB (cmn_err (CE_CONT, "Attach started " DRIVER_NICK "\n"));
  if ((osdev =
       osdev_create (NULL, DRIVER_TYPE, 0, DRIVER_NICK, NULL)) == NULL)
    {
      return EIO;
    }

  osdev->drvinfo = &oss_drvinfo;

  if (!DRIVER_ATTACH (osdev))
    {
      cmn_err (CE_WARN, "Attach failed\n");
      osdev_delete (osdev);
      return EIO;
    }

  return 0;
}

static int
detach_virtual (void)
{
  if (osdev == NULL)
    {
      return 0;
    }

  if (DRIVER_DETACH (osdev) <= 0)
    {
      DDB (cmn_err (CE_WARN, "Driver busy - cannot detach\n"));
      return EBUSY;
    }

  osdev_delete (osdev);

  return 0;
}
#endif

static int
ossdrv_config (cfg_func_t func, void *idata, rm_key_t key)
{
  switch (func)
    {
    case CFG_ADD:
      return cfg_add (idata, key);
      break;

    case CFG_REMOVE:
      return cfg_remove (idata);
      break;

    case CFG_VERIFY:
      return 0;
      break;

    }

  return EOPNOTSUPP;
}

/*
 * Driver entry point routines
 */

int
_load ()
{
  int err;

  if ((err = drv_attach (&oss_drvinfo)) != 0)
    {
      cmn_err (CE_WARN, "drv_attach failed %d\n", err);
      return err;
    }

#if DRIVER_TYPE==DRV_VIRTUAL || DRIVER_TYPE==DRV_VMIX
  attach_virtual ();
#endif

  return 0;
}

int
_unload ()
{
  extern volatile int oss_open_devices;

  if (oss_open_devices > 0)
    return EBUSY;

#if DRIVER_TYPE==DRV_VIRTUAL || DRIVER_TYPE==DRV_VMIX
  detach_virtual ();
#endif

  drv_detach (&oss_drvinfo);
  return 0;
}
