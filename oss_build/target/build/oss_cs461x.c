/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_cs461x
#define DRIVER_NICK	"oss_cs461x"
#define DRIVER_PURPOSE	"Crystal CS461x/CS4280"
#define DRIVER_STR_INFO	oss_cs461x_str_info
#define DRIVER_ATTACH	oss_cs461x_attach
#define DRIVER_DETACH	oss_cs461x_detach
#define DRIVER_TYPE	DRV_PCI

int cs461x_clkrun_fix=0;
/*
 * Some IBM Thinkpads require a workaround for the CLKRUN protocol. 
 * Values: 1=Enable 0=Disable  Default: 0
 */

#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x1013,	.device=0x6001,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1013,	.device=0x6003,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1013,	.device=0x6004,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"

module_param(cs461x_clkrun_fix, int, S_IRUGO);
MODULE_PARM_DESC(cs461x_clkrun_fix, 
"\n"
"Some IBM Thinkpads require a workaround for the CLKRUN protocol. \n"
"Values: 1=Enable 0=Disable  Default: 0\n"
"\n"
"\n");


