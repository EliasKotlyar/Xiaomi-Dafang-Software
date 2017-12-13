/*
 * Automatically generated file - do not edit.
 */
#define DRIVER_NAME	oss_usb
#define DRIVER_NICK	"oss_usb"
#define DRIVER_PURPOSE	"USB audio device support (BETA)"
#define DRIVER_STR_INFO	oss_usb_str_info
#define DRIVER_ATTACH	oss_usb_attach
#define DRIVER_DETACH	oss_usb_detach
#define DRIVER_TYPE	DRV_USB

int usb_trace=0;
int usb_mixerstyle=1;
#include <linux/mod_devicetable.h>

#undef strcpy
#define strcpy dummy_strcpy
#include <linux/usb.h>
#undef strcpy
static struct usb_device_id udi_usb_table[] = {
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0xf41e,	.idProduct=0x3000},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0xf41e,	.idProduct=0x3010},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0xf41e,	.idProduct=0x3020},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0xf46d,	.idProduct=0x8b2},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0xf46d,	.idProduct=0xa01},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0xf471,	.idProduct=0x311},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0xf672,	.idProduct=0x1041},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0xfd8c,	.idProduct=0xc},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0xfd8c,	.idProduct=0x103},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0xfd8c,	.idProduct=0x102},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0xf6f8,	.idProduct=0xc000},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0x763,	.idProduct=0x1010},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0x763,	.idProduct=0x1011},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0x763,	.idProduct=0x1001},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0x763,	.idProduct=0x1002},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0x763,	.idProduct=0x1031},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0xf763,	.idProduct=0x2001},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0xf763,	.idProduct=0x2002},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0xf763,	.idProduct=0xc},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0xf763,	.idProduct=0x2007},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0xf763,	.idProduct=0x200d},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0xf763,	.idProduct=0x2805},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0x763,	.idProduct=0x1014},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0x763,	.idProduct=0x1015},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0xfa92,	.idProduct=0x1010},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0x499,	.idProduct=0x1009},
	{.match_flags=USB_DEVICE_ID_MATCH_DEVICE,	.idVendor=0x499,	.idProduct=0x101e},
	{match_flags:USB_DEVICE_ID_MATCH_INT_CLASS,
	bInterfaceClass: USB_CLASS_AUDIO},
	{0}
};

#include "module.inc"

module_param(usb_trace, int, S_IRUGO);
module_param(usb_mixerstyle, int, S_IRUGO);


