/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_madi
#define DRIVER_NICK	"oss_madi"
#define DRIVER_PURPOSE	"RME MADI Digital Interface"
#define DRIVER_STR_INFO	oss_madi_str_info
#define DRIVER_ATTACH	oss_madi_attach
#define DRIVER_DETACH	oss_madi_detach
#define DRIVER_TYPE	DRV_PCI

int madi_maxchannels=64;
/*
 * By default OSS will create device files for all 64 channels (32 stereo
 * pairs). Number of channels can be decreased by changing this parameter.
 */
int madi_devsize=2;
/*
 * This parameter tells how the device files should be created for MADI 
 * channels. By default (2) a device file will be created for each stereo
 * channel pair. Value of 1 means that separate device file will be 
 * created for each channel.
 */
#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x10ee,	.device=0x3fc6,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"

module_param(madi_maxchannels, int, S_IRUGO);
MODULE_PARM_DESC(madi_maxchannels, 
"\n"
"By default OSS will create device files for all 64 channels (32 stereo\n"
"pairs). Number of channels can be decreased by changing this parameter.\n"
"\n");
module_param(madi_devsize, int, S_IRUGO);
MODULE_PARM_DESC(madi_devsize, 
"\n"
"This parameter tells how the device files should be created for MADI \n"
"channels. By default (2) a device file will be created for each stereo\n"
"channel pair. Value of 1 means that separate device file will be \n"
"created for each channel.\n"
"\n");


