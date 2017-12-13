/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_sblive
#define DRIVER_NICK	"oss_sblive"
#define DRIVER_PURPOSE	"Creative Sound Blaster Live/Audigy"
#define DRIVER_STR_INFO	oss_sblive_str_info
#define DRIVER_ATTACH	oss_sblive_attach
#define DRIVER_DETACH	oss_sblive_detach
#define DRIVER_TYPE	DRV_PCI

int sblive_memlimit = 8;
/*
 * Amount of memory allocated to SBLive Synth.
 * Values: 4-4096 (in megabytes).  Default: 8MB
 */

int sblive_devices = 0;
/*
 * Specifies number of audio output engines for SBLive/Audigy.
 * Changing this setting is not recommanded. 
 * Values: 0, 5-32    
 * Default: 0 (Use device dependent optimum value).
 */

int sblive_digital_din = 0;
/*
 * Sets the SPDIF/Analog combo output to audio or spdif mode
 * Values: 1 = Digital, 0=Analog Default: 0
 */

int audigy_digital_din = 1;
/*
 * Sets the SPDIF/Analog combo output to analog or spdif mode
 * Values: 1 = Digital, 0 = Analog Default: 1
 */
#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x1102,	.device=0x2,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1102,	.device=0x4,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1102,	.device=0x8,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1102,	.device=0x2001,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"

module_param(sblive_memlimit , int, S_IRUGO);
MODULE_PARM_DESC(sblive_memlimit , 
"\n"
"Amount of memory allocated to SBLive Synth.\n"
"Values: 4-4096 (in megabytes).  Default: 8MB\n"
"\n"
"\n");
module_param(sblive_devices , int, S_IRUGO);
MODULE_PARM_DESC(sblive_devices , 
"\n"
"Specifies number of audio output engines for SBLive/Audigy.\n"
"Changing this setting is not recommanded. \n"
"Values: 0, 5-32    \n"
"Default: 0 (Use device dependent optimum value).\n"
"\n"
"\n");
module_param(sblive_digital_din , int, S_IRUGO);
MODULE_PARM_DESC(sblive_digital_din , 
"\n"
"Sets the SPDIF/Analog combo output to audio or spdif mode\n"
"Values: 1 = Digital, 0=Analog Default: 0\n"
"\n"
"\n");
module_param(audigy_digital_din , int, S_IRUGO);
MODULE_PARM_DESC(audigy_digital_din , 
"\n"
"Sets the SPDIF/Analog combo output to analog or spdif mode\n"
"Values: 1 = Digital, 0 = Analog Default: 1\n"
"\n");


