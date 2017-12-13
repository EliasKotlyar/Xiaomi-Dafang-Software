/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_via97
#define DRIVER_NICK	"oss_via97"
#define DRIVER_PURPOSE	"VIA VT82C686 (via97)"
#define DRIVER_STR_INFO	oss_via97_str_info
#define DRIVER_ATTACH	oss_via97_attach
#define DRIVER_DETACH	oss_via97_detach
#define DRIVER_TYPE	DRV_PCI

#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x1106,	.device=0x3058,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"



