/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_ymf7xx
#define DRIVER_NICK	"oss_ymf7xx"
#define DRIVER_PURPOSE	"Yamaha DS-XG YMF7xx"
#define DRIVER_STR_INFO	oss_ymf7xx_str_info
#define DRIVER_ATTACH	oss_ymf7xx_attach
#define DRIVER_DETACH	oss_ymf7xx_detach
#define DRIVER_TYPE	DRV_PCI

int yamaha_mpu_ioaddr=0;
/*
 * Yamaha DSXG MPU I/O Address
 * Values: 0x300, 0x330, 0x332, 0x334  Default: 0
 */

int yamaha_mpu_irq=0;
/*
 * Yamaha DSXG MPU IRQ 
 * Values: 5, 7, 9, 10, 11   Default: 0
 */

int yamaha_fm_ioaddr=0;
/*
 * Yamaha DSXG FM IO Address
 * Values: 0x388, 0x398, 0x3a0, 0x3a8  Default: 0
 */
#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x1073,	.device=0x10,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1073,	.device=0x12,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1073,	.device=0x4,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1073,	.device=0x5,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1073,	.device=0xa,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1073,	.device=0xc,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1073,	.device=0xd,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"

module_param(yamaha_mpu_ioaddr, int, S_IRUGO);
MODULE_PARM_DESC(yamaha_mpu_ioaddr, 
"\n"
"Yamaha DSXG MPU I/O Address\n"
"Values: 0x300, 0x330, 0x332, 0x334  Default: 0\n"
"\n"
"\n");
module_param(yamaha_mpu_irq, int, S_IRUGO);
MODULE_PARM_DESC(yamaha_mpu_irq, 
"\n"
"Yamaha DSXG MPU IRQ \n"
"Values: 5, 7, 9, 10, 11   Default: 0\n"
"\n"
"\n");
module_param(yamaha_fm_ioaddr, int, S_IRUGO);
MODULE_PARM_DESC(yamaha_fm_ioaddr, 
"\n"
"Yamaha DSXG FM IO Address\n"
"Values: 0x388, 0x398, 0x3a0, 0x3a8  Default: 0\n"
"\n");


