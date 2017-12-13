/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_ali5455
#define DRIVER_NICK	"oss_ali5455"
#define DRIVER_PURPOSE	"ALI M5455 chipset"
#define DRIVER_STR_INFO	oss_ali5455_str_info
#define DRIVER_ATTACH	oss_ali5455_attach
#define DRIVER_DETACH	oss_ali5455_detach
#define DRIVER_TYPE	DRV_PCI

#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x10b9,	.device=0x5455,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"



