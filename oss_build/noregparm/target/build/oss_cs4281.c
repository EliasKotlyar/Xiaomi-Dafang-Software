/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_cs4281
#define DRIVER_NICK	"oss_cs4281"
#define DRIVER_PURPOSE	"Crystal CS4281"
#define DRIVER_STR_INFO	oss_cs4281_str_info
#define DRIVER_ATTACH	oss_cs4281_attach
#define DRIVER_DETACH	oss_cs4281_detach
#define DRIVER_TYPE	DRV_PCI

#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x1013,	.device=0x6005,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"



