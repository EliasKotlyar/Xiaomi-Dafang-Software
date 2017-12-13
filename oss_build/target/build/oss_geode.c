/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_geode
#define DRIVER_NICK	"oss_geode"
#define DRIVER_PURPOSE	"National Semiconductor Geode SC1200/CS5530/CS5536"
#define DRIVER_STR_INFO	oss_geode_str_info
#define DRIVER_ATTACH	oss_geode_attach
#define DRIVER_DETACH	oss_geode_detach
#define DRIVER_TYPE	DRV_PCI

#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x100b,	.device=0x503,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1078,	.device=0x103,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1022,	.device=0x2093,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"



