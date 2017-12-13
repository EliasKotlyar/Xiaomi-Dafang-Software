/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_sbxfi
#define DRIVER_NICK	"oss_sbxfi"
#define DRIVER_PURPOSE	"Creative Sound Blaster X-Fi"
#define DRIVER_STR_INFO	oss_sbxfi_str_info
#define DRIVER_ATTACH	oss_sbxfi_attach
#define DRIVER_DETACH	oss_sbxfi_detach
#define DRIVER_TYPE	DRV_PCI

int sbxfi_type=0;
/* 
 * Override sbxfi autodetection.
 * Values: 0-4. Default: 0
 * 0 - Autodetect
 * 1 - Sound Blaster X-Fi (SB046x/067x/076x)
 * 2 - Sound Blaster X-Fi (SB073x)
 * 3 - Sound Blaster X-Fi (SB055x)
 * 4 - Sound Blaster X-Fi (UAA)
 * 5 - Sound Blaster X-Fi (SB0760)
 * 6 - Sound Blaster X-Fi (SB0880-1)
 * 7 - Sound Blaster X-Fi (SB0880-2)
 * 8 - Sound Blaster X-Fi (SB0880-3)
 */

#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x1102,	.device=0x5,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1102,	.device=0xb,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"

module_param(sbxfi_type, int, S_IRUGO);
MODULE_PARM_DESC(sbxfi_type, 
"\n"
"Override sbxfi autodetection.\n"
"Values: 0-4. Default: 0\n"
"0 - Autodetect\n"
"1 - Sound Blaster X-Fi (SB046x/067x/076x)\n"
"2 - Sound Blaster X-Fi (SB073x)\n"
"3 - Sound Blaster X-Fi (SB055x)\n"
"4 - Sound Blaster X-Fi (UAA)\n"
"5 - Sound Blaster X-Fi (SB0760)\n"
"6 - Sound Blaster X-Fi (SB0880-1)\n"
"7 - Sound Blaster X-Fi (SB0880-2)\n"
"8 - Sound Blaster X-Fi (SB0880-3)\n"
"\n"
"\n");


