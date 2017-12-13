/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_audioloop
#define DRIVER_NICK	"oss_audioloop"
#define DRIVER_PURPOSE	"OSS loopback audio driver"
#define DRIVER_STR_INFO	oss_audioloop_str_info
#define DRIVER_ATTACH	oss_audioloop_attach
#define DRIVER_DETACH	oss_audioloop_detach
#define DRIVER_TYPE	DRV_VIRTUAL

int audioloop_instances=1;
/*
 * audioloop_instances: Number of instances (client/server pairs) to create.
 */
#include "module.inc"

module_param(audioloop_instances, int, S_IRUGO);
MODULE_PARM_DESC(audioloop_instances, 
"\n"
"audioloop_instances: Number of instances (client/server pairs) to create.\n"
"\n");


