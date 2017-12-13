/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_trident
#define DRIVER_NICK	"oss_trident"
#define DRIVER_PURPOSE	"Trident 4DWave, SiS7018, ALI M5451"
#define DRIVER_STR_INFO	oss_trident_str_info
#define DRIVER_ATTACH	oss_trident_attach
#define DRIVER_DETACH	oss_trident_detach
#define DRIVER_TYPE	DRV_PCI

int trident_mpu_ioaddr=0;
/*
 * Trident MPU 401 I/O Address
 * Values: 0x300, 0x330 Default: 0x330
 */

#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x1023,	.device=0x2000,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1023,	.device=0x2001,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1023,	.device=0x2002,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1039,	.device=0x7018,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x10b9,	.device=0x5451,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"

module_param(trident_mpu_ioaddr, int, S_IRUGO);
MODULE_PARM_DESC(trident_mpu_ioaddr, 
"\n"
"Trident MPU 401 I/O Address\n"
"Values: 0x300, 0x330 Default: 0x330\n"
"\n"
"\n");


