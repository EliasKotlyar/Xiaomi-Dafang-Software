/*
 * Purpose: Wrapper functions for virtual drivers under FreeBSD
 */
/*
 * Copyright (C) 4Front Technologies 2005-2007. Released under BSD license.
 */
static int ndevs = 0;
oss_device_t *device_list[16];

static int
module_attach (void)
{
  oss_device_t *osdev;

  if ((osdev =
       osdev_create (NULL, DRIVER_TYPE, ndevs, DRIVER_NICK, NULL)) == NULL)
    {
      return ENOMEM;
    }
  if (!DRIVER_ATTACH (osdev))
    return EIO;
  device_list[ndevs++] = osdev;

  return 0;
}

static int
module_detach (void)
{
  oss_device_t *osdev;
  int i;

  for (i = 0; i < ndevs; i++)
    {
      osdev = device_list[i];

      if (osdev->dip != NULL && device_get_state(osdev->dip) == DS_BUSY)
	device_unbusy(osdev->dip);
      if (!DRIVER_DETACH (osdev))
	{
	  printf (DRIVER_NICK ": Unloading busy device\n");
	  return EBUSY;
	}
      osdev_delete (osdev);
    }

  return 0;
}

/* 
 * Load handler that deals with the loading and unloading of a KLD.
 */

static int
ossmodule_loader (struct module *m, int what, void *arg)
{
  int err = 0;

  switch (what)
    {
    case MOD_LOAD:		/* kldload */
      return module_attach ();
      break;
    case MOD_UNLOAD:
      return module_detach ();
      break;
    default:
      err = EINVAL;
      break;
    }
  return (err);
}

/* Declare this module to the rest of the kernel */

static moduledata_t ossmodule_mod = {
  "ossmodule",
  ossmodule_loader,
  NULL
};

DECLARE_MODULE (ossmodule, ossmodule_mod, SI_SUB_KLD, SI_ORDER_ANY);
MODULE_VERSION (ossmodule, 4);
