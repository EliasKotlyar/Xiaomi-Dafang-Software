/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_solo
#define DRIVER_NICK	"oss_solo"
#define DRIVER_PURPOSE	"ESS Solo-1 "
#define DRIVER_STR_INFO	oss_solo_str_info
#define DRIVER_ATTACH	oss_solo_attach
#define DRIVER_DETACH	oss_solo_detach
#define DRIVER_TYPE	DRV_PCI

#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x125d,	.device=0x1969,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"



