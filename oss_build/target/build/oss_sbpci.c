/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_sbpci
#define DRIVER_NICK	"oss_sbpci"
#define DRIVER_PURPOSE	"Creative AudioPCI97 (ES1371/ES1373/EV1938)"
#define DRIVER_STR_INFO	oss_sbpci_str_info
#define DRIVER_ATTACH	oss_sbpci_attach
#define DRIVER_DETACH	oss_sbpci_detach
#define DRIVER_TYPE	DRV_PCI

int apci_latency=0;
/*
 * Set the latency to 32, 64, 96, 128 clocks - some APCI97 devices exhibit 
 * garbled audio in some cases and setting the latency to higer values fixes it
 * Values: 32, 64, 96, 128 - Default: 64 (or defined by bios)
 */

int apci_spdif=0;
/*
 * Enable SPDIF port on SoundBlaster 128D or Sound Blaster Digital-4.1 models
 * Values: 1=Enable 0=Disable Default: 0
 */
#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x1102,	.device=0x8938,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1274,	.device=0x1371,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1274,	.device=0x5880,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1274,	.device=0x8001,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1274,	.device=0x8002,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"

module_param(apci_latency, int, S_IRUGO);
MODULE_PARM_DESC(apci_latency, 
"\n"
"Set the latency to 32, 64, 96, 128 clocks - some APCI97 devices exhibit \n"
"garbled audio in some cases and setting the latency to higer values fixes it\n"
"Values: 32, 64, 96, 128 - Default: 64 (or defined by bios)\n"
"\n"
"\n");
module_param(apci_spdif, int, S_IRUGO);
MODULE_PARM_DESC(apci_spdif, 
"\n"
"Enable SPDIF port on SoundBlaster 128D or Sound Blaster Digital-4.1 models\n"
"Values: 1=Enable 0=Disable Default: 0\n"
"\n");


