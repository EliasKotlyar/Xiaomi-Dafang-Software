/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_emu10k1x
#define DRIVER_NICK	"oss_emu10k1x"
#define DRIVER_PURPOSE	"Creative Sound Blaster 5.1 (Dell)"
#define DRIVER_STR_INFO	oss_emu10k1x_str_info
#define DRIVER_ATTACH	oss_emu10k1x_attach
#define DRIVER_DETACH	oss_emu10k1x_detach
#define DRIVER_TYPE	DRV_PCI

int emu10k1x_spdif_enable=0;
/*
 * Enable SPDIF on Combo Jack
 * Values: 1=Enable 0=Disable 	Default: 0
 */
#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x1102,	.device=0x6,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"

module_param(emu10k1x_spdif_enable, int, S_IRUGO);
MODULE_PARM_DESC(emu10k1x_spdif_enable, 
"\n"
"Enable SPDIF on Combo Jack\n"
"Values: 1=Enable 0=Disable 	Default: 0\n"
"\n");


