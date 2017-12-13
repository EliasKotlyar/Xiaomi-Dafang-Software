/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_midiloop
#define DRIVER_NICK	"oss_midiloop"
#define DRIVER_PURPOSE	"MIDI loopback pseudo device"
#define DRIVER_STR_INFO	oss_midiloop_str_info
#define DRIVER_ATTACH	oss_midiloop_attach
#define DRIVER_DETACH	oss_midiloop_detach
#define DRIVER_TYPE	DRV_VIRTUAL

int midiloop_instances = 1;	/* Number of loopback MIDI devs to install */
/*
 * midiloop_instances gives the number of MIDI loopback devices to be created.
 * The default value is 1
 */
#include "module.inc"

module_param(midiloop_instances , int, S_IRUGO);
MODULE_PARM_DESC(midiloop_instances , 
"\n"
"midiloop_instances gives the number of MIDI loopback devices to be created.\n"
"The default value is 1\n"
"\n");


