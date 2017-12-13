/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_userdev
#define DRIVER_NICK	"oss_userdev"
#define DRIVER_PURPOSE	"OSS loopback audio driver"
#define DRIVER_STR_INFO	oss_userdev_str_info
#define DRIVER_ATTACH	oss_userdev_attach
#define DRIVER_DETACH	oss_userdev_detach
#define DRIVER_TYPE	DRV_VIRTUAL

int userdev_visible_clientnodes=0;
/* 
 * By default the oss_userdev driver will not create private device nodes
 * for the client side devices. Instead all client devices will share
 * the same device node (/dev/oss/oss_userdev/client).
 *
 * If userdev_visible_clientnodes is set to 1 then each oss_userdev instance 
 * will have private device node (/dev/oss/oss_userdev0/pcmN) that can be
 * opened directly. This mode can be used when the oss_userdev driver is used
 * for multiple purposes in the same system.
 */
#include "module.inc"

module_param(userdev_visible_clientnodes, int, S_IRUGO);
MODULE_PARM_DESC(userdev_visible_clientnodes, 
"\n"
"By default the oss_userdev driver will not create private device nodes\n"
"for the client side devices. Instead all client devices will share\n"
"the same device node (/dev/oss/oss_userdev/client).\n"
"\n"
"If userdev_visible_clientnodes is set to 1 then each oss_userdev instance \n"
"will have private device node (/dev/oss/oss_userdev0/pcmN) that can be\n"
"opened directly. This mode can be used when the oss_userdev driver is used\n"
"for multiple purposes in the same system.\n"
"\n");


