/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_hdaudio
#define DRIVER_NICK	"oss_hdaudio"
#define DRIVER_PURPOSE	"High Definition Audio (Azalia)"
#define DRIVER_STR_INFO	oss_hdaudio_str_info
#define DRIVER_ATTACH	oss_hdaudio_attach
#define DRIVER_DETACH	oss_hdaudio_detach
#define DRIVER_TYPE	DRV_PCI

int hdaudio_snoopy=0;
/*
 * hdaudio_snopy is reserved for diagnostic purposes and it must be 0
 * in all situations. Other values may make the driver vulnerable to
 * DoS attacks. For security reasons only root can use this diagnostic
 * interface.
 */
int hdaudio_jacksense=0;
/*
 * Setting hdaudio_jacksense=1 enables jack sensing mode when the
 * hdaudio driver is loaded. In this mode all I/O pin's that are not
 * in use will be disabled as well as the mixer controls that are related
 * with them. In this way the mixer/control panel will become more intuitive.
 * However OSS will need to be restarted with soundoff;soundon every time
 * new inputs or outputs are attached to the audio jacks.
 *
 * NOTE! hdaudio_jacksense=1 works only in some systems. Many laptops and
 *       motherboards don't support jack sensing.
 */
int hdaudio_noskip=0;
/*
 * Disable checks to skip unconnected jack. Values: 0-7, where value is a
 * bitmask - every bit disables another check. Can override hdaudio_jacksense.
 */
#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x8086,	.device=0x2668,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x27d8,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x269a,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x284b,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x293e,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x293f,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x3a3e,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x3a6e,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x3b56,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x3b57,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x1c20,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x1d20,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x1e20,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x811b,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x8c20,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0x9d70,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x8086,	.device=0xa170,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x10de,	.device=0x26c,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x10de,	.device=0x371,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x10de,	.device=0x3e4,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x10de,	.device=0x3f0,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x10de,	.device=0x44a,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x10de,	.device=0x55c,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x10de,	.device=0x7fc,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x10de,	.device=0x774,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x10de,	.device=0xac0,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1002,	.device=0x437b,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1002,	.device=0x4383,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1022,	.device=0x780d,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1106,	.device=0x3288,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1039,	.device=0x7502,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x10b9,	.device=0x5461,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{.vendor=0x1102,	.device=0x9,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"

module_param(hdaudio_snoopy, int, S_IRUGO);
MODULE_PARM_DESC(hdaudio_snoopy, 
"\n"
"hdaudio_snopy is reserved for diagnostic purposes and it must be 0\n"
"in all situations. Other values may make the driver vulnerable to\n"
"DoS attacks. For security reasons only root can use this diagnostic\n"
"interface.\n"
"\n");
module_param(hdaudio_jacksense, int, S_IRUGO);
MODULE_PARM_DESC(hdaudio_jacksense, 
"\n"
"Setting hdaudio_jacksense=1 enables jack sensing mode when the\n"
"hdaudio driver is loaded. In this mode all I/O pin's that are not\n"
"in use will be disabled as well as the mixer controls that are related\n"
"with them. In this way the mixer/control panel will become more intuitive.\n"
"However OSS will need to be restarted with soundoff;soundon every time\n"
"new inputs or outputs are attached to the audio jacks.\n"
"\n"
"NOTE! hdaudio_jacksense=1 works only in some systems. Many laptops and\n"
"motherboards don't support jack sensing.\n"
"\n");
module_param(hdaudio_noskip, int, S_IRUGO);
MODULE_PARM_DESC(hdaudio_noskip, 
"\n"
"Disable checks to skip unconnected jack. Values: 0-7, where value is a\n"
"bitmask - every bit disables another check. Can override hdaudio_jacksense.\n"
"\n");


