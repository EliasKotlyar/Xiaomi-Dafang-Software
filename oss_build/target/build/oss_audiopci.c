/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_audiopci
#define DRIVER_NICK	"oss_audiopci"
#define DRIVER_PURPOSE	"Ensoniq/Creative AudioPCI (ES1370)"
#define DRIVER_STR_INFO	oss_audiopci_str_info
#define DRIVER_ATTACH	oss_audiopci_attach
#define DRIVER_DETACH	oss_audiopci_detach
#define DRIVER_TYPE	DRV_PCI

#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x1274,	.device=0x5000,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"



