/*
 * Purpose: Template for OSS modules under Linux
 *
 * This file is included by the low level driver modules when they are
 * compiled in the target system. In this way this file can be modified
 * for non-standard kernels by the customer. Compiling in the target is
 * necessary because the driver/module framework of Linux cannot support
 * binary drivers. Another reason is that the kbuild mechanism used to build
 * kernel modules under Linux is not compatible with the way how the source
 * code of OSS is organized.
 */
/*
 * Copyright (C) 4Front Technologies 2005-2007. Released under GPL2 license.
 */
#define strcpy dummy_strcpy
#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include "timestamp.h"
#include "oss_exports.h"
#include "wrap.h"
#include "ossddk.h"
#include "ossdip.h"
#undef strcpy
#define strcpy oss_strcpy
#undef strlen
#define strlen oss_strlen

static int module_major = 0;
static int instance = 0;

MODULE_LICENSE ("GPL v2");
MODULE_DESCRIPTION ("Open Sound System '" DRIVER_PURPOSE "' driver module");
MODULE_AUTHOR ("4Front Technologies (support@opensound.com)");

extern int DRIVER_ATTACH (oss_device_t * osdev);
extern int DRIVER_DETACH (oss_device_t * osdev);

#if DRIVER_TYPE==DRV_PCI
#define DRIVER_TYPE_OK

#include "pci_wrapper.inc"

// MODULE_DEVICE_TABLE(pci, id_table);
static struct pci_driver osspci_driver = {
  .name = DRIVER_NICK,
  .id_table = id_table,
  .probe = osspci_probe,
  .remove = osspci_remove
};

static int __init
pcidrv_init (void)
{
  int err;

  if ((err = pci_register_driver (&osspci_driver)) < 0)
    return err;
  oss_register_module (THIS_MODULE);
  return 0;
}

static void __exit
pcidrv_exit (void)
{
  if (module_major > 0)
    {
      oss_unregister_chrdev (module_major, DRIVER_NICK);
    }
  pci_unregister_driver (&osspci_driver);
  oss_unregister_module (THIS_MODULE);
}

module_init (pcidrv_init);
module_exit (pcidrv_exit);
#endif /* PCI driver */

#if DRIVER_TYPE==DRV_VIRTUAL || DRIVER_TYPE==DRV_VMIX
#define DRIVER_TYPE_OK
static oss_device_t *osdev = NULL;

static int
virtualdrv_init (void)
{
  static int instance = 0, maj;

  if ((osdev =
       osdev_create (NULL, DRIVER_TYPE, instance++, DRIVER_NICK,
		     NULL)) == NULL)
    {
      return -ENOMEM;
    }
  osdev_set_owner (osdev, THIS_MODULE);
  maj = oss_request_major (osdev, 0, DRIVER_NICK);
  osdev_set_major (osdev, maj);
  if (!DRIVER_ATTACH (osdev))
    return -EIO;
  oss_register_module (THIS_MODULE);
  oss_audio_delayed_attach ();
  return 0;
}

static void
virtualdrv_exit (void)
{

  if (!DRIVER_DETACH (osdev))
    printk (KERN_ALERT DRIVER_NICK ": Unloading busy device\n");
  osdev_delete (osdev);
  oss_unregister_module (THIS_MODULE);
}

module_init (virtualdrv_init);
module_exit (virtualdrv_exit);
#endif /* Virtual driver */

#if DRIVER_TYPE==DRV_USB
#define DRIVER_TYPE_OK
oss_device_t *osdev = NULL;
static int usb_major = 0;

#include "usb_wrapper.inc"

static int
usbdrv_init (void)
{
  int res;

  if (udi_usb_installed)
    return 0;
  if ((osdev =
       osdev_create (NULL, DRV_VIRTUAL, instance++, DRIVER_NICK,
		     NULL)) == NULL)
    {
      return -ENOMEM;
    }
  osdev_set_owner (osdev, THIS_MODULE);
  usb_major = oss_request_major (osdev, 0, DRIVER_NICK);
  if (usb_major < 0)
    {
      oss_cmn_err (CE_WARN, "Failed to allocate USB major (%d)\n", usb_major);
      return -EIO;
    }
  osdev_set_major (osdev, usb_major);
  oss_register_device (osdev, "USB audio core services");
  if (!DRIVER_ATTACH (osdev))
    return -EIO;
  oss_register_module (THIS_MODULE);
  oss_audio_delayed_attach ();

  udi_usb_installed = 1;

  if ((res = usb_register (&oss_usb)) < 0)
    {
      printk ("oss: usb_register failed, err=%d\n", res);
      drv = NULL;
      return -ENXIO;
    }
  return 0;
}

static void
usbdrv_exit (void)
{
  if (!udi_usb_installed)
    return;
  if (!DRIVER_DETACH (osdev))
    printk (KERN_ALERT DRIVER_NICK ": Unloading busy device\n");
  usb_deregister (&oss_usb);
  udi_usb_installed = 0;
  known_devices = NULL;
  osdev_delete (osdev);
  osdev = NULL;
  oss_unregister_module (THIS_MODULE);
}

module_init (usbdrv_init);
module_exit (usbdrv_exit);
#endif /* USB driver */

#ifndef DRIVER_TYPE_OK
#error Unrecognized driver type
#endif

char *
strcpy (char *s1, const char *s2)
{
  char *s = s1;

  while (*s2)
    *s1++ = *s2++;
  *s1 = 0;
  return s;
}

void
oss_cmn_err (int level, const char *s, ...)
{
  char tmp[1024], *a[6];
  va_list ap;
  int i, n = 0;

  va_start (ap, s);

  for (i = 0; i < strlen (s); i++)
    if (s[i] == '%')
      n++;

  for (i = 0; i < n && i < 6; i++)
    {
      a[i] = va_arg (ap, char *);
    }

  for (i = n; i < 6; i++)
    a[i] = NULL;

  if (level == CE_CONT)
    {
      sprintf (tmp, s, a[0], a[1], a[2], a[3], a[4], a[5], NULL,
	       NULL, NULL, NULL);
      printk ("%s", tmp);
    }
  else
    {
      strcpy (tmp, DRIVER_NICK ": ");

      sprintf (tmp + strlen (tmp), s, a[0], a[1], a[2], a[3], a[4], a[5],
	       NULL, NULL, NULL, NULL);
      if (level == CE_PANIC)
	panic (tmp);
      printk (KERN_ALERT "%s", tmp);
    }
#if 0
  /* This may cause a crash under SMP */
  if (sound_started)
    store_msg (tmp);
#endif

  va_end (ap);
}

#include "osscore_symbols.inc"
