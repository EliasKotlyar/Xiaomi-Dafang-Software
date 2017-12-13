/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_fmedia
#define DRIVER_NICK	"oss_fmedia"
#define DRIVER_PURPOSE	"ForteMedia FM 801"
#define DRIVER_STR_INFO	oss_fmedia_str_info
#define DRIVER_ATTACH	oss_fmedia_attach
#define DRIVER_DETACH	oss_fmedia_detach
#define DRIVER_TYPE	DRV_PCI

int fmedia_mpu_irq=0;
/* 
 * FM801 MPU IRQ
 * Values: 5,7,9,10,11 	Default: 0
 */

#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x1319,	.device=0x801,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"

module_param(fmedia_mpu_irq, int, S_IRUGO);
MODULE_PARM_DESC(fmedia_mpu_irq, 
"\n"
"FM801 MPU IRQ\n"
"Values: 5,7,9,10,11 	Default: 0\n"
"\n"
"\n");


