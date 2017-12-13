/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_audigyls
#define DRIVER_NICK	"oss_audigyls"
#define DRIVER_PURPOSE	"Sound Blaster Audigy LS / Live7.1"
#define DRIVER_STR_INFO	oss_audigyls_str_info
#define DRIVER_ATTACH	oss_audigyls_attach
#define DRIVER_DETACH	oss_audigyls_detach
#define DRIVER_TYPE	DRV_PCI

int audigyls_spdif_enable=0;
/*
 * Set the Orange Jack to SPDIF or Analog output
 * Values 1=SPDIF  0=Analog (side-surround) 	Default=0
 */

#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x1102,	.device=0x7,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"

module_param(audigyls_spdif_enable, int, S_IRUGO);
MODULE_PARM_DESC(audigyls_spdif_enable, 
"\n"
"Set the Orange Jack to SPDIF or Analog output\n"
"Values 1=SPDIF  0=Analog (side-surround) 	Default=0\n"
"\n"
"\n");


