/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_envy24ht
#define DRIVER_NICK	"oss_envy24ht"
#define DRIVER_PURPOSE	"VIA Envy24HT/PT (ICE1724) sound chipset"
#define DRIVER_STR_INFO	oss_envy24ht_str_info
#define DRIVER_ATTACH	oss_envy24ht_attach
#define DRIVER_DETACH	oss_envy24ht_detach
#define DRIVER_TYPE	DRV_PCI

int envy24ht_model = -1;
/*
 * Select the Model number if the card isn't autodetected
 * Values: 0 = Envy24ht 1=Envy24PT/HT-s compatible -1=Autodetect Default: -1
 */
#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x1412,	.device=0x1724,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"

module_param(envy24ht_model , int, S_IRUGO);
MODULE_PARM_DESC(envy24ht_model , 
"\n"
"Select the Model number if the card isn't autodetected\n"
"Values: 0 = Envy24ht 1=Envy24PT/HT-s compatible -1=Autodetect Default: -1\n"
"\n");


