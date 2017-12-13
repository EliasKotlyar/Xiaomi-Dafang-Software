/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_via823x
#define DRIVER_NICK	"oss_via823x"
#define DRIVER_PURPOSE	"VIA VT8233/8235/8237 motherboard audio"
#define DRIVER_STR_INFO	oss_via823x_str_info
#define DRIVER_ATTACH	oss_via823x_attach
#define DRIVER_DETACH	oss_via823x_detach
#define DRIVER_TYPE	DRV_PCI

#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x1106,	.device=0x3059,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1106,	.device=0x4551,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1106,	.device=0x7059,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"



