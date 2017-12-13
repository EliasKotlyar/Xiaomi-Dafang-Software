/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_digi96
#define DRIVER_NICK	"oss_digi96"
#define DRIVER_PURPOSE	"RME Digi96"
#define DRIVER_STR_INFO	oss_digi96_str_info
#define DRIVER_ATTACH	oss_digi96_attach
#define DRIVER_DETACH	oss_digi96_detach
#define DRIVER_TYPE	DRV_PCI

#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x10ee,	.device=0x3fc0,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x10ee,	.device=0x3fc1,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x10ee,	.device=0x3fc2,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x10ee,	.device=0x3fc3,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"



