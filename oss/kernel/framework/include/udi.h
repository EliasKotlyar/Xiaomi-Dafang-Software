/*
 * Purpose: USB abstraction structures and functions used by OSS
 *
 * OS dependent wrappers for various USB related kernel interfaces. Each
 * operating system will have it's private udi.c file which implements
 * the services defined here.
 */
/*
 *
 * This file is part of Open Sound System.
 *
 * Copyright (C) 4Front Technologies 1996-2008.
 *
 * This this source file is released under GPL v2 license (no other versions).
 * See the COPYING file included in the main directory of this source
 * distribution for the license terms and conditions.
 *
 */

/* typedef struct udi_usb_devc udi_usb_devc; // Moved to os.h */

typedef struct
{
  char *altsetting_labels;
  int default_altsetting;
  unsigned int altsetting_mask;
} ossusb_altsetting_def_t;

#define MAX_ALTSETTINGS	20

typedef struct udi_usb_devinfo
{
  int vendor, product;
  char *name;
  const ossusb_altsetting_def_t altsettings[MAX_ALTSETTINGS];
}
udi_usb_devinfo;

typedef struct
{
  void *(*attach) (udi_usb_devc * devc, oss_device_t * osdev);
  void (*disconnect) (void *devc);
} udi_usb_driver;

extern int udi_attach_usbdriver (oss_device_t * osdev,
				 const udi_usb_devinfo * devlist,
				 udi_usb_driver * driver);
extern void udi_unload_usbdriver (oss_device_t * osdev);

extern int udi_usb_trace;
extern int udi_usbdev_get_class (udi_usb_devc * usbdev);
extern int udi_usbdev_get_subclass (udi_usb_devc * usbdev);
extern int udi_usbdev_get_vendor (udi_usb_devc * usbdev);
extern int udi_usbdev_get_product (udi_usb_devc * usbdev);
extern int udi_usbdev_get_inum (udi_usb_devc * usbdev);
extern unsigned char *udi_usbdev_get_endpoint (udi_usb_devc * usbdev,
					       int altsetting, int n,
					       int *len);
#define EP_IN	0
#define EP_OUT	1
extern char *udi_usbdev_get_name (udi_usb_devc * usbdev);
extern char *udi_usbdev_get_altsetting_labels (udi_usb_devc * usbdev, int if_num, int *default_alt, unsigned int *mask);
extern char *udi_usbdev_get_string (udi_usb_devc * usbdev, int ix);
extern char *udi_usbdev_get_devpath (udi_usb_devc * usbdev);
extern int udi_usbdev_get_num_altsettings (udi_usb_devc * usbdev);
extern int udi_usbdev_get_usb_version (udi_usb_devc * usbdev);
extern unsigned char *udi_usbdev_get_altsetting (udi_usb_devc * usbdev, int n,
						 int *size);
extern int udi_usbdev_set_interface (udi_usb_devc * usbdev, int inum,
				     int altset);
extern int udi_usb_snd_control_msg (udi_usb_devc * usbdev,
				    unsigned int endpoint, unsigned char rq,
				    unsigned char rqtype,
				    unsigned short value,
				    unsigned short index, void *buf, int len,
				    int timeout);
extern int udi_usb_rcv_control_msg (udi_usb_devc * usbdev,
				    unsigned int endpoint, unsigned char rq,
				    unsigned char rqtype,
				    unsigned short value,
				    unsigned short index, void *buf, int len,
				    int timeout);

typedef struct udi_usb_request udi_usb_request_t;	/* Opaque type */
typedef struct _udi_endpoint_handle_t udi_endpoint_handle_t;

extern udi_endpoint_handle_t *udi_open_endpoint (udi_usb_devc * usbdev,
						 void *ep_descr);
extern void udi_close_endpoint (udi_endpoint_handle_t * eph);
extern int udi_endpoint_get_num (udi_endpoint_handle_t * eph);

#define UDI_USBXFER_ISO_WRITE		1
#define UDI_USBXFER_ISO_READ		2
#define UDI_USBXFER_BULK_READ		3
#define UDI_USBXFER_BULK_WRITE		4
#define UDI_USBXFER_INTR_READ		5
typedef void (*udi_usb_complete_func_t) (udi_usb_request_t * request,
					 void *arg);

extern udi_usb_request_t *udi_usb_alloc_request (udi_usb_devc * usbdev,
						 udi_endpoint_handle_t * eph,
						 int nframes, int xfer_type);
extern void udi_usb_free_request (udi_usb_request_t * request);
extern int udi_usb_submit_request (udi_usb_request_t * request,
				   udi_usb_complete_func_t callback,
				   void *callback_arg,
				   udi_endpoint_handle_t * eph, int xfer_type,
				   void *data, int data_len);
extern void udi_usb_cancel_request (udi_usb_request_t * request);
extern int udi_usb_request_actlen (udi_usb_request_t * request);
extern unsigned char *udi_usb_request_actdata (udi_usb_request_t * request);
