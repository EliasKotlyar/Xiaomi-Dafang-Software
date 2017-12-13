/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	lynxone
#define DRIVER_NICK	"lynxone"
#define DRIVER_PURPOSE	"lynxone"
#define DRIVER_STR_INFO	lynxone_str_info
#define DRIVER_ATTACH	lynxone_attach
#define DRIVER_DETACH	lynxone_detach
#define DRIVER_TYPE	DRV_PCI

#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x10b5,	.device=0x1142,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"



