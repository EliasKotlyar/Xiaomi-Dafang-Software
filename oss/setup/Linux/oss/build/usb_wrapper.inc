/*
 * Purpose: Low level USB wrapper routines for Linux (2.6.x and later)
 *
 * This file contains the Linux specific USB support routines defined in udi.h
 */
/*
 * Copyright (C) 4Front Technologies 2005-2007. Released under GPL2 license.
 */
#include "udi.h"

#undef IO_DEBUG
static const udi_usb_devinfo *known_devices = NULL;

#define MAX_DEVICE_SLOTS 20

int udi_usb_trace = 0;

static udi_usb_driver *drv = NULL;
extern void *oss_pmalloc (size_t sz);

struct udi_usb_devc
{
  struct usb_device *usb_dev;
  const struct usb_device_id *id;
  const udi_usb_devinfo *udi_usb_dev;
  char *devpath;

  int enabled;

  int vendor, product, class, subclass;
  struct usb_interface *iface;
  int iface_number;
  char *dev_name;
  char *altsetting_labels;
  int default_altsetting;
  unsigned int altsetting_mask;
  udi_usb_driver *drv;
  void *client_devc;
  int num_altsettings;
  int usb_version;
  struct usb_host_interface *altsetting;

};

struct _udi_endpoint_handle_t
{
  unsigned char desc[32];
};

#define MAX_DEVC 32

static udi_usb_devc usb_devc_list[MAX_DEVC] = { {0} };
static int ndevs = 0;

udi_endpoint_handle_t *
udi_open_endpoint (udi_usb_devc * usbdev, void *ep_descr)
{
  return (udi_endpoint_handle_t *) ep_descr;
}

void
udi_close_endpoint (udi_endpoint_handle_t * eph)
{
  // NOP
}

int
udi_endpoint_get_num (udi_endpoint_handle_t * eph)
{
  return eph->desc[2] /* & 0x7f */;
}

#if 1
static void
dump_configs (struct usb_device *dev)
{
  int c;

  printk ("#configs %d\n", dev->descriptor.bNumConfigurations);

  for (c = 0; c < dev->descriptor.bNumConfigurations; c++)
    {
      int i, j, k;
      struct usb_host_config *config = &dev->config[c];

      printk ("\tConfig #%d - #interfaces=%d\n", c,
	      config->desc.bNumInterfaces);

      for (j = 0; j < config->desc.bNumInterfaces; j++)
	{
	  struct usb_interface *ifp = config->interface[j];
	  printk ("\t\tInterface #%d - altsettings=%d\n", j,
		  ifp->num_altsetting);

	  for (k = 0; k < ifp->num_altsetting; k++)
	    {
	      struct usb_host_interface *alt = &ifp->altsetting[k];
	      unsigned char *buf = alt->extra;

	      printk ("\t\t\tAlt setting #%d:\n", k);

	      for (i = 0; i < alt->extralen; i++)
		{
		  if (!(i % 8))
		    {
		      if (i)
			printk ("\n");
		      printk ("\t\t\t%04x: ", i);
		    }
		  printk ("%02x ", buf[i]);
		}

	      printk ("\n");
	    }
	}
    }
}
#endif

static int
udi_attach_usbdev (struct usb_device *dev,
		   oss_device_t * osdev,
		   char *devpath,
		   struct usb_interface *iface,
		   const struct usb_device_id *id,
		   const udi_usb_devinfo * udi_usb_dev)
{
  int cfg_num, ep;

  udi_usb_devc *devc = &usb_devc_list[ndevs];
  struct usb_host_interface *alts;

  if (ndevs >= MAX_DEVC)
    {
      printk ("OSS: Too many USB audio/midi devices\n");
      return -ENODEV;
    }

  if (udi_usb_trace > 1)
    printk ("OSS: Attaching USB device %x:%x/%d, class=%x:%x, name=%s\n",
	    dev->descriptor.idVendor, dev->descriptor.idProduct,
	    iface->altsetting[0].desc.bInterfaceNumber,
	    iface->altsetting[0].desc.bInterfaceClass,
	    iface->altsetting[0].desc.bInterfaceSubClass, udi_usb_dev->name);

  devc->usb_dev = dev;
  devc->id = id;
  devc->udi_usb_dev = udi_usb_dev;

  devc->vendor = dev->descriptor.idVendor;
  devc->product = dev->descriptor.idProduct;
  devc->usb_version = dev->descriptor.bcdUSB >> 8;
  devc->class = iface->altsetting[0].desc.bInterfaceClass;
  devc->subclass = iface->altsetting[0].desc.bInterfaceSubClass;
  devc->iface_number = iface->altsetting[0].desc.bInterfaceNumber;
  devc->iface = iface;
  devc->dev_name = udi_usb_dev->name;
  devc->devpath = devpath;
  devc->num_altsettings = iface->num_altsetting;
  devc->altsetting = iface->altsetting;

  alts = &iface->altsetting[devc->num_altsettings - 1];
  ep = 0;

  if (alts->desc.bNumEndpoints > 1)
    ep = 1;

  if (udi_usb_trace > 2)
    {
      int i;

      for (i = 0; i < alts->desc.bNumEndpoints; i++)
	{
	  printk ("Endpoint: %02x\n",
		  alts->endpoint[i].desc.bEndpointAddress);
	}
    }

  cfg_num = 0;

  devc->enabled = 1;
  devc->drv = drv;

  if (udi_usb_trace > 2)
    dump_configs (dev);

  if ((devc->client_devc = drv->attach (devc, osdev)) == NULL)
    {
      return -EIO;
    }

  ndevs++;

  usb_set_intfdata (iface, devc);
  return 0;
}

static char *prev_devices[32] = { NULL };
static int nprev_devices = 0;

static int
udi_usb_probe (struct usb_interface *iface, const struct usb_device_id *id)
{
  int i;
  static int ncalls = 0;
  oss_device_t *osdev = NULL;
  dev_info_t *dip = NULL;	// TODO

  char nick[32];
  int inst = 0;

  struct usb_device *dev = interface_to_usbdev (iface);

  if (ncalls++ > 100)
    return -EIO;

  sprintf (nick, "usb%04x%04x-", dev->descriptor.idVendor,
	   dev->descriptor.idProduct);

/*
 * Find out how many instances of this device (ID) are already attached.
 */

  for (i = 0; i < nprev_devices; i++)
  {
    if (strcmp (nick, prev_devices[i]) == 0)
    {
      inst++;
    }
  }

  prev_devices[nprev_devices] = oss_pmalloc (strlen (nick) + 1);
  strcpy (prev_devices[nprev_devices], nick);
  if (nprev_devices < 32)
    nprev_devices++;

  if ((osdev = osdev_create (dip, DRV_USB, inst, nick, NULL)) == NULL)
    {
      return -ENOMEM;
    }
  osdev_set_owner (osdev, THIS_MODULE);
  osdev_set_major (osdev, usb_major);

  i = 0;
  if (udi_usb_trace > 1)
    printk ("\n\nProbing dev=%s id=%x:%x/%d\n", dev->devpath,
	    dev->descriptor.idVendor,
	    dev->descriptor.idProduct,
	    iface->altsetting[0].desc.bInterfaceNumber);

  while (i >= 0 && known_devices[i].vendor >= 0)
    {
      if (dev->descriptor.idVendor == known_devices[i].vendor &&
	  dev->descriptor.idProduct == known_devices[i].product)
	{
	  int ret;
	  const udi_usb_devinfo *d = &known_devices[i];
	  ret = udi_attach_usbdev (dev, osdev, dev->devpath, iface, id, d);
	  return ret;
	}
      else
	i++;
    }

/* Try the "generic" device */
  {
    int ret;
    const udi_usb_devinfo *d = &known_devices[i];
    ret = udi_attach_usbdev (dev, osdev, dev->devpath, iface, id, d);
    return ret;
  }

  return -ENODEV;
}

static void
udi_usb_disconnect (struct usb_interface *iface)
{
  udi_usb_devc *devc = usb_get_intfdata (iface);
  //struct usb_device *dev = interface_to_usbdev (iface);

  if (devc == (udi_usb_devc *) - 1)
    return;

  if (!devc->enabled)
    return;

  if (udi_usb_trace > 0)
    printk ("OSS: Disconnect USB device %x:%x %s\n", devc->vendor,
	    devc->product, devc->udi_usb_dev->name);
  devc->drv->disconnect (devc->client_devc);
  devc->enabled = 0;
}

static struct usb_driver oss_usb = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15)
owner:THIS_MODULE,
#endif
name:"oss_usb",
probe:udi_usb_probe,
disconnect:udi_usb_disconnect,
id_table:udi_usb_table
};

static int udi_usb_installed = 0;

int
udi_attach_usbdriver (oss_device_t * osdev, const udi_usb_devinfo * devlist,
		      udi_usb_driver * driver)
{
  drv = driver;
  known_devices = devlist;
  return 1;
}

void
udi_unload_usbdriver (oss_device_t * osdev)
{
}

/* 
 * Device access routines
 */

int
udi_usbdev_get_class (udi_usb_devc * usbdev)
{
  udi_usb_devc *devc = (udi_usb_devc *) usbdev;

  return devc->class;
}

int
udi_usbdev_get_subclass (udi_usb_devc * usbdev)
{
  udi_usb_devc *devc = (udi_usb_devc *) usbdev;

  return devc->subclass;
}

int
udi_usbdev_get_vendor (udi_usb_devc * usbdev)
{
  udi_usb_devc *devc = (udi_usb_devc *) usbdev;

  return devc->vendor;
}

int
udi_usbdev_get_product (udi_usb_devc * usbdev)
{
  udi_usb_devc *devc = (udi_usb_devc *) usbdev;

  return devc->product;
}

int
udi_usbdev_get_inum (udi_usb_devc * usbdev)
{
  udi_usb_devc *devc = (udi_usb_devc *) usbdev;

  return devc->iface_number;
}

int
udi_usbdev_set_interface (udi_usb_devc * usbdev, int inum, int altset)
{
  udi_usb_devc *devc = (udi_usb_devc *) usbdev;

  return usb_set_interface (devc->usb_dev, inum, altset);
}

unsigned char *
udi_usbdev_get_endpoint (udi_usb_devc * usbdev, int altsetting, int n,
			 int *len)
{
  udi_usb_devc *devc = (udi_usb_devc *) usbdev;
  int num_endpoints;
  struct usb_device *dev;
  struct usb_host_interface *alts;
  struct usb_interface *iface;

  dev = devc->usb_dev;
  iface = devc->iface;

  if (altsetting >= devc->num_altsettings)
    return NULL;

  alts = &iface->altsetting[altsetting];

  num_endpoints = alts->desc.bNumEndpoints;

  if (n >= num_endpoints)
    return NULL;

  *len = alts->endpoint[n].desc.bLength;
  return (unsigned char *) &alts->endpoint[n].desc;
}

int
udi_usbdev_get_num_altsettings (udi_usb_devc * usbdev)
{
  udi_usb_devc *devc = (udi_usb_devc *) usbdev;

  return devc->num_altsettings;
}

int
udi_usbdev_get_usb_version (udi_usb_devc * usbdev)
{
  udi_usb_devc *devc = (udi_usb_devc *) usbdev;

  return devc->usb_version;
}

unsigned char *
udi_usbdev_get_altsetting (udi_usb_devc * usbdev, int n, int *size)
{
  udi_usb_devc *devc = (udi_usb_devc *) usbdev;
  struct usb_host_interface *alt;

  if (n < 0 || n >= devc->num_altsettings)
    {
      /* printk("udi usb: Bad altsetting %d (%d)\n", n, n >= devc->num_altsettings); */
      return NULL;
    }

  alt = &devc->altsetting[n];

  *size = alt->extralen;
  return alt->extra;
}

char *
udi_usbdev_get_name (udi_usb_devc * usbdev)
{
  udi_usb_devc *devc = (udi_usb_devc *) usbdev;

  return devc->dev_name == NULL ? "Unknown" : devc->dev_name;
}


char *
udi_usbdev_get_altsetting_labels (udi_usb_devc * usbdev, int if_num, int *default_alt, unsigned int *mask)
{
  int i;

  *default_alt=1;
  *mask=0xffffffff;

  if (usbdev->udi_usb_dev == NULL) /* No device definitions available */
  {
     return NULL;
  }

  for (i=0;usbdev->udi_usb_dev->altsettings[i].altsetting_labels!=NULL;i++)
  if (i==if_num)
  {
     *default_alt = usbdev->udi_usb_dev->altsettings[i].default_altsetting;
     *mask = usbdev->udi_usb_dev->altsettings[i].altsetting_mask;
     if (*mask==0)
	*mask=0xffffffff;
     return usbdev->udi_usb_dev->altsettings[i].altsetting_labels;
  }

  return NULL; /* Not found */
}

char *
udi_usbdev_get_string (udi_usb_devc * usbdev, int ix)
{
  udi_usb_devc *devc = (udi_usb_devc *) usbdev;
  static char str[100];
  int err;

  if (ix == 0)
    return NULL;

  if ((err = usb_string (devc->usb_dev, ix, str, sizeof (str) - 1)) != 0)
    {
      return NULL;
    }

  return str;
}

char *
udi_usbdev_get_devpath (udi_usb_devc * usbdev)
{
  udi_usb_devc *devc = (udi_usb_devc *) usbdev;

  return devc->devpath;
}

int
udi_usb_snd_control_msg (udi_usb_devc * usbdev, unsigned int endpoint,
			 unsigned char rq,
			 unsigned char rqtype,
			 unsigned short value,
			 unsigned short index,
			 void *buf, int len, int timeout)
{
  udi_usb_devc *devc = (udi_usb_devc *) usbdev;
  int err;

  if (!devc->enabled)
    return -EPIPE;

  if (timeout < 0)
    timeout = 0;

#ifdef IO_DEBUG
  printk ("Snd %x (%x) rq=%x, rt=%x, v=%x, ix=%x, l=%d %02x %02x\n",
	  devc->usb_dev,
	  usb_sndctrlpipe (devc->usb_dev, endpoint),
	  rq, rqtype, value, index, len, b[0], b[1]);
#endif
  err = usb_control_msg (devc->usb_dev,
			 usb_sndctrlpipe (devc->usb_dev, endpoint),
			 rq, rqtype, value, index, buf, len, timeout);
#ifdef IO_DEBUG
  if (err < 0)
    printk ("Usb write error %d\n", err);
#endif

  return err;
}

int
udi_usb_rcv_control_msg (udi_usb_devc * usbdev, unsigned int endpoint,
			 unsigned char rq,
			 unsigned char rqtype,
			 unsigned short value,
			 unsigned short index,
			 void *buf, int len, int timeout)
{
  udi_usb_devc *devc = (udi_usb_devc *) usbdev;
  int err;

  if (!devc->enabled)
    return -EPIPE;

  if (timeout < 0)
    timeout = 0;

#ifdef IO_DEBUG
  printk ("Rcv %x (%x) rq=%x, rt=%x, v=%x, ix=%x, l=%d\n",
	  devc->usb_dev,
	  (unsigned int) usb_rcvctrlpipe (devc->usb_dev, endpoint),
	  rq, rqtype | USB_DIR_IN, value, index, len);
#endif
  err = usb_control_msg (devc->usb_dev,
			 (unsigned int) usb_rcvctrlpipe (devc->usb_dev,
							 endpoint), rq,
			 rqtype | USB_DIR_IN, value, index, buf, len,
			 timeout);
#ifdef IO_DEBUG
  if (err < 0)
    printk ("Usb read error %d\n", err);
  else
    printk ("Got %02x %02x\n", b[0], b[1]);
#endif

  return err;
}

/* Request stuff */

struct udi_usb_request
{
  struct urb *urb;
  udi_usb_complete_func_t callback;
  void *callback_arg;
  int active;
  void *data;
};

udi_usb_request_t
  * udi_usb_alloc_request (udi_usb_devc * usbdev, udi_endpoint_handle_t * eph,
			   int nframes, int xfer_type)
{
  udi_usb_request_t *rq;
  udi_usb_devc *devc = (udi_usb_devc *) usbdev;

  if ((rq = kmalloc (sizeof (*rq), GFP_KERNEL)) == NULL)
    {
      printk ("udi_usb_alloc_request: Out of memory\n");
      return NULL;
    }

  memset (rq, 0, sizeof (*rq));

  if ((rq->urb = usb_alloc_urb (nframes, 0)) == NULL)
    {
      kfree (rq);
      printk ("udi_usb_alloc_request: Failed to allocate URB\n");
      return NULL;
    }

  rq->urb->dev = devc->usb_dev;
  rq->urb->number_of_packets = nframes;
  rq->active = 0;

  return rq;
}

void
udi_usb_free_request (udi_usb_request_t * request)
{
  if (request == NULL)
    return;

  udi_usb_cancel_request (request);

  usb_free_urb (request->urb);
  kfree (request);
}

unsigned char *
udi_usb_request_actdata (udi_usb_request_t * request)
{
  return request->data;
}

static void
complete_func (struct urb *urb)
{
  udi_usb_request_t *request = urb->context;

  request->active = 0;
  request->callback (request, request->callback_arg);
}

int
udi_usb_submit_request (udi_usb_request_t * request,
			udi_usb_complete_func_t callback, void *callback_arg,
			udi_endpoint_handle_t * eph, int xfer_type,
			void *data, int data_len)
{
  struct urb *urb;
  struct usb_device *d;
  int i, err;
  int endpoint = eph->desc[2] & 0x7f;

  if (request == NULL)
    return -EINVAL;

  urb = request->urb;
  d = urb->dev;
  request->callback = callback;
  request->callback_arg = callback_arg;
  request->data = data;
  urb->complete = (usb_complete_t) complete_func;
  urb->context = request;
  urb->transfer_buffer = data;

  for (i = 0; i < urb->number_of_packets; i++)
    {
      urb->iso_frame_desc[i].status = 0;
      urb->iso_frame_desc[i].length = data_len;
      urb->iso_frame_desc[i].offset = i * data_len;
    }

  urb->transfer_buffer_length = urb->actual_length = data_len;

  switch (xfer_type)
    {
    case UDI_USBXFER_ISO_WRITE:
      urb->pipe = usb_sndisocpipe (urb->dev, endpoint);
      urb->transfer_flags = URB_ISO_ASAP;
      urb->interval = 1;
      break;

    case UDI_USBXFER_ISO_READ:
      urb->pipe = usb_rcvisocpipe (urb->dev, endpoint);
      urb->transfer_flags = URB_ISO_ASAP;
      urb->interval = 1;
      break;

    case UDI_USBXFER_BULK_READ:
      usb_fill_bulk_urb (urb, d, usb_rcvbulkpipe (d, endpoint),
			 data, data_len,
			 (usb_complete_t) complete_func, request);
      break;

    case UDI_USBXFER_INTR_READ:
      usb_fill_int_urb (urb, d, usb_rcvintpipe (d, endpoint),
			data, data_len,
			(usb_complete_t) complete_func, request, 8);
      break;

    case UDI_USBXFER_BULK_WRITE:
      usb_fill_bulk_urb (urb, d, usb_sndbulkpipe (d, endpoint),
			 data, data_len,
			 (usb_complete_t) complete_func, request);
      break;

    default:
      printk ("udi usb: Bad xfer type %d\n", xfer_type);
      return -EINVAL;
    }

#ifdef SLAB_ATOMIC
  if ((err = usb_submit_urb (request->urb, SLAB_ATOMIC)) >= 0)
    request->active = 1;
#else
  /*
   * Linux 2.6.20 and later don't have SLAB_ATOMIC
   */
  if ((err = usb_submit_urb (request->urb, GFP_ATOMIC)) >= 0)
    request->active = 1;
#endif
  return err;
}

int
udi_usb_request_actlen (udi_usb_request_t * request)
{
  return request->urb->actual_length;
}

void
udi_usb_cancel_request (udi_usb_request_t * request)
{
  if (request == NULL || !request->active)
    return;

  usb_kill_urb (request->urb);

}
