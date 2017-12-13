/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_cmpci
#define DRIVER_NICK	"oss_cmpci"
#define DRIVER_PURPOSE	"C-Media CM833x audio chipset"
#define DRIVER_STR_INFO	oss_cmpci_str_info
#define DRIVER_ATTACH	oss_cmpci_attach
#define DRIVER_DETACH	oss_cmpci_detach
#define DRIVER_TYPE	DRV_PCI

#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x13f6,	.device=0x100,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x13f6,	.device=0x100,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x13f6,	.device=0x101,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x13f6,	.device=0x111,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x14af,	.device=0x20,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"



