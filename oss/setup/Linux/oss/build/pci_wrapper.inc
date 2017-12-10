/*
 * Purpose: PCI wrapper routines for Linux
 *
 * This source file contains probe and remove routines for PCI devices.
 * This code will be compiled in the target system.
 */
/*
 * Copyright (C) 4Front Technologies 2005-2007. Released under GPL2 license.
 */
typedef struct
{
  struct pci_dev *pcidev;
  oss_device_t *osdev;
} dev_map_t;

#define MAX_INSTANCE 10
static dev_map_t dev_map[MAX_INSTANCE];
static int n_devmap = 0;

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,8,0)
 static int __devinit
#else
 static int
#endif
osspci_probe (struct pci_dev *pcidev, const struct pci_device_id *pciid)
{
  oss_device_t *osdev;
  dev_info_t *dip;

  if (n_devmap >= MAX_INSTANCE)
    {
      printk (KERN_ALERT "oss: Too many instances of " DRIVER_NICK);
      return -ENOMEM;
    }

  if ((dip = oss_create_pcidip (pcidev)) == NULL)
    return -ENOMEM;

  if ((osdev =
       osdev_create (dip, DRIVER_TYPE, instance++, DRIVER_NICK,
		     NULL)) == NULL)
    {
      return -ENOMEM;
    }

  osdev_set_owner (osdev, THIS_MODULE);

  if (module_major == 0)
    module_major = oss_request_major (osdev, 0, DRIVER_NICK);
  if (module_major <= 0)
    {
      printk (KERN_ALERT "Failed to allocate major device for " DRIVER_NICK);
      return -EIO;

    }
  osdev_set_major (osdev, module_major);

  pci_enable_device (pcidev);
  if (!DRIVER_ATTACH (osdev))
    {
      pci_disable_device (pcidev);
      return -EIO;
    }

  dev_map[n_devmap].pcidev = pcidev;
  dev_map[n_devmap++].osdev = osdev;
  oss_audio_delayed_attach ();

  return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,8,0)
 static int __devexit
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4,6,0)
 static int
#else
 void
#endif
osspci_remove (struct pci_dev *pcidev)
{
  int i;
  oss_device_t *osdev;

  for (i = 0; i < n_devmap; i++)
    if (dev_map[i].pcidev == pcidev)
      {
	osdev = dev_map[i].osdev;
	if (!DRIVER_DETACH (osdev))
	  printk (KERN_ALERT DRIVER_NICK ": Unloading busy device\n");
	pci_disable_device (dev_map[i].pcidev);
	osdev_delete (osdev);

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,6,0)
	return 0;
#else
	return;
#endif
      }

  printk (KERN_ALERT DRIVER_NICK ": Can't find the PCI device to detach\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,6,0)
  return -EIO;
#else
  return;
#endif
}

void
oss_pcie_init (oss_device_t * osdev, int flags)
{
	/* TODO: Do we need to do something here? */
}
