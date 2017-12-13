/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_envy24
#define DRIVER_NICK	"oss_envy24"
#define DRIVER_PURPOSE	"VIA Envy24 (ICE1712) sound chipset"
#define DRIVER_STR_INFO	oss_envy24_str_info
#define DRIVER_ATTACH	oss_envy24_attach
#define DRIVER_DETACH	oss_envy24_detach
#define DRIVER_TYPE	DRV_PCI

int envy24_skipdevs = 2;
/*
 * envy24_skipdevs: By default OSS creates an audio device file for each
 * stereo pair. However it's possible to change OSS to create a device file
 * for each input/output channel. See also the envy24_force_mono option
 * if you want to set the card to mono-only mode.
 * Values: 1 to 8. Default is 2 (stereo mode).
 *
 * Note that the application(s) using the device files can request any number
 * of channels regardless of the envy24_skipdevs option.
 */
int envy24_force_mono = 0;
/*
 * envy24_force_mono: Forces all device files to work only in mono mode
 * (to be used together with envy24_skipdevs=1). This option is necessary
 * with some software packages that otherwise open the devices in stereo mode.
 * There is no need to use this option with properly implemented applications.
 * Values: 0 (default) any number of channels can be used, 1 means that
 * the device files will only be available in mono mode.
 *
 * Note that even envy24_force_mono=1 the application can request and get
 * stereo. In this way both channel signals will be identical.
 */
int envy24_swapdevs = 0;
/*
 * envy24_swapdevs: By default OSS will create the output device files before
 * the recording ones. Some applications may expect input devices to be before 
 * the output ones.
 * Values: 0=outputs before inputs (default), 1=inputs before outputs.
 */
int envy24_devmask = 65535;
/*
 * envy24_devmask: Selects the device files to be created. By default OSS
 * will create all available device files. However in some systems it may
 * be necessary to conserve device numbers.
 *
 * Values: The envy24_devmask number is the SUM of the following values:
 * 1  - Create primary (analog/ADAT/TDIF) outputs
 * 2  - Create primary (analog/ADAT/TDIF) inputs
 * 4  - Create S/PDIF outputs
 * 8  - Create S/PDIF inputs
 * 16 - Create monitor input device
 * 32 - Create the "raw" input and output devices.
 * 
 * For example envy24_devmask=12 (4+8) creates only the S/PDIF devices.
 * To enable all possible (current or future) device files set envy24_devmask
 * to 65535 (default).
 */
int envy24_realencoder_hack = 0;
/*
 * envy24_realencoder_hack: Older versions of realencoder expect a mixer device
 * for each audio device file. This option makes OSS to create dummy micers to
 * make realencoder happy.
 * Values: 0=normal (default), 1=create dmmy mixers.
 */
int envy24_gain_sliders = 0;
/*
 * envy24_gain_sliders: By default OSS will create the gain controls as
 * selection controls (+4 dB / CONSUMER / -10 dB) which is ideal for
 * professional environments. However home users may prefer having volume
 * sliders instead.
 * Values: 0=level selectors (default), 1=volume sliders.
 */
int envy24_nfrags = 16;
/*
 * envy24_nfrags: For normal operation this setting must be 16.
 */
int envy24_mixerstyle = 1;
/*
 * envy24_mixerstyle: Defines layout of the peak meter and monitor mixer
 * sections in the control panel.
 * NOTE! The envy24_skipdevs parameter must be set to 2. Otherwise the
 *       traditional mixer style will be used regardless of the value of
 *       this parameter.
 * Possible values:
 * 1=Traditional mixer with separate peak meter and monitor gain sections
 *   (default).
 * 2=Alternative style with peak meters and monitor gain sliders grouped
 *   together (separate output and input sections).
 */
#include <linux/mod_devicetable.h>

#include <linux/pci_ids.h>

static struct pci_device_id id_table[] = {
	{.vendor=0x1412,	.device=0x1712,	.subvendor=PCI_ANY_ID,	.subdevice=PCI_ANY_ID,	.class=PCI_CLASS_MULTIMEDIA_AUDIO},
	{0}
};

#include "module.inc"

module_param(envy24_skipdevs , int, S_IRUGO);
MODULE_PARM_DESC(envy24_skipdevs , 
"\n"
"envy24_skipdevs: By default OSS creates an audio device file for each\n"
"stereo pair. However it's possible to change OSS to create a device file\n"
"for each input/output channel. See also the envy24_force_mono option\n"
"if you want to set the card to mono-only mode.\n"
"Values: 1 to 8. Default is 2 (stereo mode).\n"
"\n"
"Note that the application(s) using the device files can request any number\n"
"of channels regardless of the envy24_skipdevs option.\n"
"\n");
module_param(envy24_force_mono , int, S_IRUGO);
MODULE_PARM_DESC(envy24_force_mono , 
"\n"
"envy24_force_mono: Forces all device files to work only in mono mode\n"
"(to be used together with envy24_skipdevs=1). This option is necessary\n"
"with some software packages that otherwise open the devices in stereo mode.\n"
"There is no need to use this option with properly implemented applications.\n"
"Values: 0 (default) any number of channels can be used, 1 means that\n"
"the device files will only be available in mono mode.\n"
"\n"
"Note that even envy24_force_mono=1 the application can request and get\n"
"stereo. In this way both channel signals will be identical.\n"
"\n");
module_param(envy24_swapdevs , int, S_IRUGO);
MODULE_PARM_DESC(envy24_swapdevs , 
"\n"
"envy24_swapdevs: By default OSS will create the output device files before\n"
"the recording ones. Some applications may expect input devices to be before \n"
"the output ones.\n"
"Values: 0=outputs before inputs (default), 1=inputs before outputs.\n"
"\n");
module_param(envy24_devmask , int, S_IRUGO);
MODULE_PARM_DESC(envy24_devmask , 
"\n"
"envy24_devmask: Selects the device files to be created. By default OSS\n"
"will create all available device files. However in some systems it may\n"
"be necessary to conserve device numbers.\n"
"\n"
"Values: The envy24_devmask number is the SUM of the following values:\n"
"1  - Create primary (analog/ADAT/TDIF) outputs\n"
"2  - Create primary (analog/ADAT/TDIF) inputs\n"
"4  - Create S/PDIF outputs\n"
"8  - Create S/PDIF inputs\n"
"16 - Create monitor input device\n"
"32 - Create the \"raw\" input and output devices.\n"
"\n"
"For example envy24_devmask=12 (4+8) creates only the S/PDIF devices.\n"
"To enable all possible (current or future) device files set envy24_devmask\n"
"to 65535 (default).\n"
"\n");
module_param(envy24_realencoder_hack , int, S_IRUGO);
MODULE_PARM_DESC(envy24_realencoder_hack , 
"\n"
"envy24_realencoder_hack: Older versions of realencoder expect a mixer device\n"
"for each audio device file. This option makes OSS to create dummy micers to\n"
"make realencoder happy.\n"
"Values: 0=normal (default), 1=create dmmy mixers.\n"
"\n");
module_param(envy24_gain_sliders , int, S_IRUGO);
MODULE_PARM_DESC(envy24_gain_sliders , 
"\n"
"envy24_gain_sliders: By default OSS will create the gain controls as\n"
"selection controls (+4 dB / CONSUMER / -10 dB) which is ideal for\n"
"professional environments. However home users may prefer having volume\n"
"sliders instead.\n"
"Values: 0=level selectors (default), 1=volume sliders.\n"
"\n");
module_param(envy24_nfrags , int, S_IRUGO);
MODULE_PARM_DESC(envy24_nfrags , 
"\n"
"envy24_nfrags: For normal operation this setting must be 16.\n"
"\n");
module_param(envy24_mixerstyle , int, S_IRUGO);
MODULE_PARM_DESC(envy24_mixerstyle , 
"\n"
"envy24_mixerstyle: Defines layout of the peak meter and monitor mixer\n"
"sections in the control panel.\n"
"NOTE! The envy24_skipdevs parameter must be set to 2. Otherwise the\n"
"traditional mixer style will be used regardless of the value of\n"
"this parameter.\n"
"Possible values:\n"
"1=Traditional mixer with separate peak meter and monitor gain sections\n"
"(default).\n"
"2=Alternative style with peak meters and monitor gain sliders grouped\n"
"together (separate output and input sections).\n"
"\n");


