/*
 * Purpose: Wrapper functions for PCI drivers under FreeBSD
 */
/*
 * Copyright (C) 4Front Technologies 2005-2007. Released under BSD license.
 */
#include <dev/pci/pcivar.h>	/* For pci_get macros! */
#include <dev/pci/pcireg.h>

/* PCI Support Functions */

static oss_device_t *device_list[16];
static device_t bsd_devices[16];
static int ndevs = 0;

/*
 * Compare the device ID of this device against the IDs that this driver
 * supports.  If there is a match, set the description and return success.
 */
static int
osspci_probe (device_t dev)
{
#ifdef DEVTYPE_PCI
  int i, ok = 0;
  int vendor, device, class;
  oss_device_t *osdev;

  for (i = 0; i < ndevs; i++)
    if (dev == bsd_devices[i])	/* Already detected */
      {
	return ENXIO;
      }

  if (ndevs >= 16)
    {
      printf (DRIVER_NICK ": Too many instances\n");
      return ENXIO;
    }

  vendor = pci_get_vendor (dev);
  device = pci_get_device (dev);
  class = pci_get_class (dev);
// printf("PCI dev %08lx c=%x, v=%04x, d=%04x\n", (unsigned long)dev, class, vendor, device);

  if (class != 4)		/* Not a multimedia device */
    return ENXIO;

  for (i = 0; id_table[i].vendor != 0; i++)
    if (vendor == id_table[i].vendor && device == id_table[i].device)	/* Match */
      {
	ok = 1;
	break;
      }

  if (!ok)
    {
      return (ENXIO);
    }

  if ((osdev =
       osdev_create (dev, DRIVER_TYPE, ndevs, DRIVER_NICK, NULL)) == NULL)
    {
      return ENOMEM;
    }
  if (!DRIVER_ATTACH (osdev))
    return EIO;

  bsd_devices[ndevs] = dev;
  device_list[ndevs++] = osdev;
#endif
  return (BUS_PROBE_DEFAULT);
}

/* Attach function is only called if the probe is successful */

static int
osspci_attach (device_t dev)
{
  return 0;
}

/* Detach device. */

static int
osspci_detach (device_t dev)
{
  oss_device_t *osdev;
  int i;

  for (i = 0; i < ndevs; i++)
    {
      osdev = device_list[i];
      if (osdev->dip == dev)
	{
	  if (device_get_state(dev) == DS_BUSY)
	    device_unbusy(dev);
	  if (!DRIVER_DETACH (osdev))
	    {
	      printf (DRIVER_NICK ": Unloading busy device\n");
	      return EBUSY;
	    }
	  osdev_delete (osdev);
	}
    }

  return (0);
}

/* Called during system shutdown after sync. */

static int
osspci_shutdown (device_t dev)
{

  printf ("Mypci shutdown!\n");
  return (0);
}

/*
 * Device suspend routine.
 */
static int
osspci_suspend (device_t dev)
{

  printf ("Mypci suspend!\n");
  return (0);
}

/*
 * Device resume routine.
 */
static int
osspci_resume (device_t dev)
{

  printf ("Mypci resume!\n");
  return (0);
}

static device_method_t osspci_methods[] = {
  /* Device interface */
  DEVMETHOD (device_probe, osspci_probe),
  DEVMETHOD (device_attach, osspci_attach),
  DEVMETHOD (device_detach, osspci_detach),
  DEVMETHOD (device_shutdown, osspci_shutdown),
  DEVMETHOD (device_suspend, osspci_suspend),
  DEVMETHOD (device_resume, osspci_resume),

  {0, 0}
};

static devclass_t osspci_devclass;
