/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_atiaudio
#define DRIVER_NICK	"oss_atiaudio"
#define DRIVER_PURPOSE	"ATI IXP motherboard audio"
#define DRIVER_STR_INFO	oss_atiaudio_str_info
#define DRIVER_ATTACH	oss_atiaudio_attach
#define DRIVER_DETACH	oss_atiaudio_detach
#define DRIVER_TYPE	DRV_PCI

#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x1002,	.device=0x4341,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1002,	.device=0x4361,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1002,	.device=0x4370,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"



