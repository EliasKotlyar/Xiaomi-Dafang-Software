/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_imux
#define DRIVER_NICK	"oss_imux"
#define DRIVER_PURPOSE	"OSS Input Multiplexer (IMUX)"
#define DRIVER_STR_INFO	oss_imux_str_info
#define DRIVER_ATTACH	oss_imux_attach
#define DRIVER_DETACH	oss_imux_detach
#define DRIVER_TYPE	DRV_VIRTUAL

/*
 * IMUX supports multiple instances, please consult the man page
 */

int imux_devices=5;
/*
 * Number of IMUX client side devices to configure. 
 * Values: 2-48    Default: 5.
 */

int imux_masterdev=-1;
/*
 * Master physical device to use (ie attach IMUX to a specific soundcard)
 * -1 means automatically detect the device. Use the device index numbers
 * reported by ossinfo -a.
 * Values: 0-N 	Default: -1 (autodetec)
 */

int imux_rate=48000;
/*
 * Select the base sampling rate for the IMUX device. The base rate must be
 * one supported by the actual physical device (as reported by audioinfo -a -v2).
 * Values: 8000-192000	Default: 48000
 */
#include "module.inc"

module_param(imux_devices, int, S_IRUGO);
MODULE_PARM_DESC(imux_devices, 
"\n"
"Number of IMUX client side devices to configure. \n"
"Values: 2-48    Default: 5.\n"
"\n"
"\n");
module_param(imux_masterdev, int, S_IRUGO);
MODULE_PARM_DESC(imux_masterdev, 
"\n"
"Master physical device to use (ie attach IMUX to a specific soundcard)\n"
"-1 means automatically detect the device. Use the device index numbers\n"
"reported by ossinfo -a.\n"
"Values: 0-N 	Default: -1 (autodetec)\n"
"\n"
"\n");
module_param(imux_rate, int, S_IRUGO);
MODULE_PARM_DESC(imux_rate, 
"\n"
"Select the base sampling rate for the IMUX device. The base rate must be\n"
"one supported by the actual physical device (as reported by audioinfo -a -v2).\n"
"Values: 8000-192000	Default: 48000\n"
"\n");


