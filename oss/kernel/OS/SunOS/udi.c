/*
 * Purpose: USB device interface (udi.h) routines for Solaris
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
#if !defined(SOL9)
#include "oss_config.h"
#include <udi.h>

#define USBDRV_MAJOR_VER	2
#define USBDRV_MINOR_VER	0
#include <sys/usb/usba.h>
#include <sys/strsubr.h>

#undef IO_DEBUG
int udi_usb_trace = 0;

#define MAX_DEVICE_SLOTS 20

struct udi_usb_devc
{
  oss_device_t *osdev;
  usb_client_dev_data_t *dev_data;
  char *name;

  const struct usb_device_id *id;
  const udi_usb_devinfo *udi_usb_dev;
  char devpath[32];

  int enabled;

  struct usb_interface *iface;
  udi_usb_driver *drv;
  void *client_devc;
  void (*client_disconnect) (void *devc);
};

#define MAX_DEVC 32

int
udi_attach_usbdriver (oss_device_t * osdev, const udi_usb_devinfo * devlist,
		      udi_usb_driver * driver)
{
  int err, i, nalt, vendor, product, busaddr;
  udi_usb_devc *usbdev;
  dev_info_t *dip = osdev->dip;
  char *name;
  unsigned long osid;
  //usb_alt_if_data_t *altif_data;
  usb_client_dev_data_t *dev_data;

  if (dip==NULL)
     {
	cmn_err(CE_WARN, "dip==NULL\n");
	return 0;
     }

  name = ddi_get_name (osdev->dip);

  if (strcmp (name, "oss_usb") == 0)
    {
      oss_register_device (osdev, "USB audio/MIDI device");
      return 1;
    }

  /*
   * Select the default configuration
   */
  if ((err = usb_set_cfg (osdev->dip, USB_DEV_DEFAULT_CONFIG_INDEX,
			  USB_FLAGS_SLEEP, NULL, NULL)) == USB_SUCCESS)
    {
      cmn_err (CE_CONT, "usb_set_cfg returned %d\n", err);
    }

  busaddr = ddi_prop_get_int (DDI_DEV_T_ANY, osdev->dip,
			      DDI_PROP_NOTPROM, "assigned-address", 1234);
  if ((usbdev = KERNEL_MALLOC (sizeof (*usbdev))) == NULL)
    {
      cmn_err (CE_WARN, "udi_attach_usbdriver: Out of memory\n");
      return 0;
    }

  memset (usbdev, 0, sizeof (*usbdev));
  osdev->usbdev = usbdev;
  usbdev->osdev = osdev;

  if ((err = usb_client_attach (dip, USBDRV_VERSION, 0)) != USB_SUCCESS)
    {
      cmn_err (CE_WARN, "usb_client_attach failed, err=%d\n", err);
      KERNEL_FREE (usbdev);
      return 0;
    }

  if ((err =
       usb_get_dev_data (osdev->dip, &usbdev->dev_data, USB_PARSE_LVL_CFG,
			 0)) != USB_SUCCESS)
    {
      cmn_err (CE_WARN, "usb_get_dev_data failed, err=%d\n", err);
      KERNEL_FREE (usbdev);
      return 0;
    }
  dev_data = usbdev->dev_data;

  osdev->iblock_cookie = dev_data->dev_iblock_cookie;

  sprintf (usbdev->devpath, "usb%x,%x@port%d", udi_usbdev_get_vendor (usbdev),
	   udi_usbdev_get_product (usbdev), busaddr);
  oss_register_device (osdev, "USB audio/MIDI device");
  sprintf (osdev->nick, "usb%x_%x_%d", udi_usbdev_get_vendor (usbdev),
	   udi_usbdev_get_product (usbdev), busaddr);

#if 0
  if (udi_usb_trace > 5)
    usb_print_descr_tree (osdev->dip, dev_data);
#endif

  nalt = 1;
  if (nalt >= dev_data->dev_curr_cfg->cfg_if[dev_data->dev_curr_if].if_n_alt)
    nalt = 0;

  //altif_data = &dev_data->dev_curr_cfg->
  //  cfg_if[dev_data->dev_curr_if].if_alt[nalt];

  usbdev->name = usbdev->dev_data->dev_product;
  vendor = udi_usbdev_get_vendor (usbdev);
  product = udi_usbdev_get_product (usbdev);

  /* inum = usbdev->dev_data->dev_curr_if; */
  osid = (vendor << 16) | product;
  osdev->osid = (void *) osid;

  sprintf (osdev->handle, "usb%x,%x.%d:%d", vendor, product,
	   dev_data->dev_curr_if, osdev->instance);

  for (i = 0; devlist[i].vendor != -1; i++)
    if (devlist[i].vendor == vendor && devlist[i].product == product)
      {
	usbdev->name = devlist[i].name;
	usbdev->udi_usb_dev = &devlist[i];
	oss_register_device (osdev, usbdev->name);
	break;
      }

  if ((usbdev->client_devc = driver->attach (usbdev, osdev)) == NULL)
    {
      cmn_err (CE_WARN, "Client attach failed, err=%d\n", err);
      udi_unload_usbdriver (osdev);
      return 0;
    }

  usbdev->client_disconnect = driver->disconnect;

  return 1;
}

static char *
usb_errstr (int err)
{

  static struct errmsg_
  {
    int err;
    char *str;
  } errs[] =
  {
    {
    USB_FAILURE, "USB_FAILURE"},
    {
    USB_NO_RESOURCES, "USB_NO_RESOURCES"},
    {
    USB_NO_BANDWIDTH, "USB_NO_BANDWIDTH"},
    {
    USB_NOT_SUPPORTED, "USB_NOT_SUPPORTED"},
    {
    USB_PIPE_ERROR, "USB_PIPE_ERROR"},
    {
    USB_INVALID_PIPE, "USB_INVALID_PIPE"},
    {
    USB_NO_FRAME_NUMBER, "USB_NO_FRAME_NUMBER"},
    {
    USB_INVALID_START_FRAME, "USB_INVALID_START_FRAME"},
    {
    USB_HC_HARDWARE_ERROR, "USB_HC_HARDWARE_ERROR"},
    {
    USB_INVALID_REQUEST, "USB_INVALID_REQUEST"},
    {
    USB_INVALID_CONTEXT, "USB_INVALID_CONTEXT"},
    {
    USB_INVALID_VERSION, "USB_INVALID_VERSION"},
    {
    USB_INVALID_ARGS, "USB_INVALID_ARGS"},
    {
    USB_INVALID_PERM, "USB_INVALID_PERM"},
    {
    USB_BUSY, "USB_BUSY"},
    {
    0, NULL}
  };

  int i;
  static char msg[20];

  for (i = 0; errs[i].err != 0; i++)
    if (errs[i].err == err)
      {
	return errs[i].str;
      }

  sprintf (msg, "Err %d", err);
  return msg;
}

static char *
usb_cb_err (int err)
{

  static struct errmsg_
  {
    int err;
    char *str;
  } errs[] =
  {
    {
    USB_CB_STALL_CLEARED, "USB_CB_STALL_CLEARED"},
    {
    USB_CB_FUNCTIONAL_STALL, "USB_CB_FUNCTIONAL_STALL"},
    {
    USB_CB_PROTOCOL_STALL, "USB_CB_PROTOCOL_STALL"},
    {
    USB_CB_RESET_PIPE, "USB_CB_RESET_PIPE"},
    {
    USB_CB_ASYNC_REQ_FAILED, "USB_CB_ASYNC_REQ_FAILED"},
    {
    USB_CB_NO_RESOURCES, "USB_CB_NO_RESOURCES"},
    {
    USB_CB_SUBMIT_FAILED, "USB_CB_SUBMIT_FAILED"},
    {
    USB_CB_INTR_CONTEXT, "USB_CB_INTR_CONTEXT"},
    {
    USB_FAILURE, "USB_FAILURE"},
    {
    USB_NO_RESOURCES, "USB_NO_RESOURCES"},
    {
    USB_NO_BANDWIDTH, "USB_NO_BANDWIDTH"},
    {
    USB_NOT_SUPPORTED, "USB_NOT_SUPPORTED"},
    {
    USB_PIPE_ERROR, "USB_PIPE_ERROR"},
    {
    USB_INVALID_PIPE, "USB_INVALID_PIPE"},
    {
    USB_NO_FRAME_NUMBER, "USB_NO_FRAME_NUMBER"},
    {
    USB_INVALID_START_FRAME, "USB_INVALID_START_FRAME"},
    {
    USB_HC_HARDWARE_ERROR, "USB_HC_HARDWARE_ERROR"},
    {
    USB_INVALID_REQUEST, "USB_INVALID_REQUEST"},
    {
    USB_INVALID_CONTEXT, "USB_INVALID_CONTEXT"},
    {
    USB_INVALID_VERSION, "USB_INVALID_VERSION"},
    {
    USB_INVALID_ARGS, "USB_INVALID_ARGS"},
    {
    USB_INVALID_PERM, "USB_INVALID_PERM"},
    {
    USB_BUSY, "USB_BUSY"},
    {
    0, NULL}
  };

  int i;
  static char msg[20];

  if (err == 0)
    return "USB_CB_NO_INFO";

  for (i = 0; errs[i].err != 0; i++)
    {
      if (errs[i].err == err)
	{
	  return errs[i].str;
	}
    }

  sprintf (msg, "CB Err %d", err);
  return msg;
}

static char *
usb_cr_err (int err)
{

  static struct errmsg_
  {
    int err;
    char *str;
  } errs[] =
  {
    {
    USB_CR_CRC, "USB_CR_CRC"},
    {
    USB_CR_BITSTUFFING, "USB_CR_BITSTUFFING"},
    {
    USB_CR_DATA_TOGGLE_MM, "USB_CR_DATA_TOGGLE_MM"},
    {
    USB_CR_STALL, "USB_CR_STALL"},
    {
    USB_CR_DEV_NOT_RESP, "USB_CR_DEV_NOT_RESP"},
    {
    USB_CR_PID_CHECKFAILURE, "USB_CR_PID_CHECKFAILURE"},
    {
    USB_CR_UNEXP_PID, "USB_CR_UNEXP_PID"},
    {
    USB_CR_DATA_OVERRUN, "USB_CR_DATA_OVERRUN"},
    {
    USB_CR_DATA_UNDERRUN, "USB_CR_DATA_UNDERRUN"},
    {
    USB_CR_BUFFER_OVERRUN, "USB_CR_BUFFER_OVERRUN"},
    {
    USB_CR_BUFFER_UNDERRUN, "USB_CR_BUFFER_UNDERRUN"},
    {
    USB_CR_TIMEOUT, "USB_CR_TIMEOUT"},
    {
    USB_CR_NOT_ACCESSED, "USB_CR_NOT_ACCESSED"},
    {
    USB_CR_NO_RESOURCES, "USB_CR_NO_RESOURCES"},
    {
    USB_CR_UNSPECIFIED_ERR, "USB_CR_UNSPECIFIED_ERR"},
    {
    USB_CR_STOPPED_POLLING, "USB_CR_STOPPED_POLLING"},
    {
    USB_CR_PIPE_CLOSING, "USB_CR_PIPE_CLOSING"},
    {
    USB_CR_PIPE_RESET, "USB_CR_PIPE_RESET"},
    {
    USB_CR_NOT_SUPPORTED, "USB_CR_NOT_SUPPORTED"},
    {
    USB_CR_FLUSHED, "USB_CR_FLUSHED"},
    {
    USB_CR_HC_HARDWARE_ERR, "USB_CR_HC_HARDWARE_ERR"},
    {
    0, NULL}
  };

  int i;
  static char msg[20];

  if (err == 0)
    return "USB_CR_OK";

  for (i = 0; errs[i].err != 0; i++)
    {
      if (errs[i].err == err)
	{
	  return errs[i].str;
	}
    }

  sprintf (msg, "CR Err %d", err);
  return msg;
}

void
udi_unload_usbdriver (oss_device_t * osdev)
{
  udi_usb_devc *usbdev = osdev->usbdev;

  if (usbdev == NULL)
    return;

  if (usbdev->client_disconnect != NULL)
    usbdev->client_disconnect (usbdev->client_devc);
  usb_client_detach (osdev->dip, usbdev->dev_data);

  KERNEL_FREE (usbdev);
  osdev->usbdev = NULL;
}

/* 
 * Device access routines
 */

int
udi_usbdev_get_class (udi_usb_devc * usbdev)
{
  udi_usb_devc *devc = (udi_usb_devc *) usbdev;

  return devc->dev_data->dev_curr_cfg->cfg_if[devc->dev_data->dev_curr_if].
    if_alt[0].altif_descr.bInterfaceClass;
}

int
udi_usbdev_get_subclass (udi_usb_devc * usbdev)
{
  udi_usb_devc *devc = (udi_usb_devc *) usbdev;

  return devc->dev_data->dev_curr_cfg->cfg_if[devc->dev_data->dev_curr_if].
    if_alt[0].altif_descr.bInterfaceSubClass;
}

int
udi_usbdev_get_vendor (udi_usb_devc * usbdev)
{
  udi_usb_devc *devc = (udi_usb_devc *) usbdev;

  return devc->dev_data->dev_descr->idVendor;
}

int
udi_usbdev_get_product (udi_usb_devc * usbdev)
{
  udi_usb_devc *devc = (udi_usb_devc *) usbdev;

  return devc->dev_data->dev_descr->idProduct;
}

int
udi_usbdev_get_inum (udi_usb_devc * usbdev)
{
  udi_usb_devc *devc = (udi_usb_devc *) usbdev;

  return devc->dev_data->dev_curr_if;
}

int
udi_usbdev_set_interface (udi_usb_devc * usbdev, int inum, int altset)
{
  //udi_usb_devc *devc = (udi_usb_devc *) usbdev;

  if (usb_set_alt_if (usbdev->osdev->dip, inum, altset, USB_FLAGS_SLEEP,
		      NULL, 0) != USB_SUCCESS)
    {
      return OSS_EIO;
    }

  return 0;
}

unsigned char *
udi_usbdev_get_endpoint (udi_usb_devc * usbdev, int altsetting, int n,
			 int *len)
{
  usb_alt_if_data_t *altif_data;
  usb_client_dev_data_t *dev_data;

  *len = 0;

  if (usbdev->dev_data == NULL)
    {
      cmn_err (CE_WARN, "Missing USB devdata\n");
      return NULL;
    }
  dev_data = usbdev->dev_data;

  if (altsetting < 0
      || altsetting >=
      dev_data->dev_curr_cfg->cfg_if[dev_data->dev_curr_if].if_n_alt)
    {
      return NULL;
    }

  altif_data = &dev_data->dev_curr_cfg->
    cfg_if[dev_data->dev_curr_if].if_alt[altsetting];

  if (altif_data == NULL)
    return NULL;

  if (n < 0 || n >= altif_data->altif_n_ep)
    return NULL;

  *len = altif_data->altif_ep[n].ep_descr.bLength;
  return (unsigned char *) &altif_data->altif_ep[n].ep_descr;
}

int
udi_usbdev_get_num_altsettings (udi_usb_devc * usbdev)
{
  //udi_usb_devc *devc = (udi_usb_devc *) usbdev;
  usb_client_dev_data_t *dev_data;

  dev_data = usbdev->dev_data;
  return dev_data->dev_curr_cfg->cfg_if[dev_data->dev_curr_if].if_n_alt;
}

unsigned char *
udi_usbdev_get_altsetting (udi_usb_devc * usbdev, int nalt, int *size)
{
  int i, n;
  usb_cvs_data_t *cvs;
  usb_alt_if_data_t *altif_data;
  usb_client_dev_data_t *dev_data;
  static unsigned char buf[1024];

  dev_data = usbdev->dev_data;
  if ((unsigned long) dev_data <= 4096)
    {
      cmn_err (CE_WARN, "Internal error: dev_data==NULL)\n");
      return NULL;
    }

  if (nalt < 0
      || nalt >=
      dev_data->dev_curr_cfg->cfg_if[dev_data->dev_curr_if].if_n_alt)
    {
      return NULL;
    }

  altif_data = &dev_data->dev_curr_cfg->
    cfg_if[dev_data->dev_curr_if].if_alt[nalt];

  if (altif_data == NULL)
    return NULL;

  n = 0;

  for (i = 0; i < altif_data->altif_n_cvs; i++)
    {
      cvs = &altif_data->altif_cvs[i];
      memcpy (buf + n, cvs->cvs_buf, cvs->cvs_buf_len);
      n += cvs->cvs_buf_len;
    }

  cvs = &altif_data->altif_cvs[0];
  if (cvs == NULL || cvs->cvs_buf == NULL)
    return NULL;

  *size = n;
  return buf;
}

char *
udi_usbdev_get_name (udi_usb_devc * usbdev)
{
  udi_usb_devc *devc = (udi_usb_devc *) usbdev;

  return devc->name;
}

char *
udi_usbdev_get_altsetting_labels (udi_usb_devc * usbdev, int if_num,
				  int *default_alt, unsigned int *mask)
{
  int i;

  *default_alt = 1;
  *mask = 0xffffffff;

  if (usbdev->udi_usb_dev == NULL)	/* No device definitions available */
    {
      return NULL;
    }

  for (i = 0; usbdev->udi_usb_dev->altsettings[i].altsetting_labels != NULL;
       i++)
    if (i == if_num)
      {
	*default_alt = usbdev->udi_usb_dev->altsettings[i].default_altsetting;
	*mask = usbdev->udi_usb_dev->altsettings[i].altsetting_mask;
	if (*mask == 0)
	  *mask = 0xffffffff;
	return usbdev->udi_usb_dev->altsettings[i].altsetting_labels;
      }

  return NULL;			/* Not found */
}

char *
udi_usbdev_get_string (udi_usb_devc * usbdev, int ix)
{
  static char str[256];

  if (usb_get_string_descr
      (usbdev->osdev->dip, USB_LANG_ID, ix, str,
       sizeof (str) - 1) != USB_SUCCESS)
    return NULL;
  return str;
}

char *
udi_usbdev_get_devpath (udi_usb_devc * usbdev)
{
  udi_usb_devc *devc = (udi_usb_devc *) usbdev;

  return devc->devpath;
}

/*ARGSUSED*/
int
udi_usb_snd_control_msg (udi_usb_devc * usbdev, unsigned int endpoint,
			 unsigned char rq,
			 unsigned char rqtype,
			 unsigned short value,
			 unsigned short index,
			 void *buf, int len, int timeout)
{

  int err;

  usb_ctrl_setup_t setup;
  usb_cr_t completion_reason;
  usb_cb_flags_t cb_flags;
  mblk_t *data = NULL;

  if (usbdev == NULL)
    {
      cmn_err (CE_CONT, "udi_usb_snd_control_msg: usbdev==NULL\n");
      return OSS_EFAULT;
    }

  data = allocb_wait (len + 1, BPRI_HI, STR_NOSIG, NULL);

  memcpy (data->b_wptr, buf, len);
  data->b_wptr = data->b_rptr + len;

  setup.bmRequestType = rqtype | USB_DEV_REQ_HOST_TO_DEV;
  setup.bRequest = rq;
  setup.wValue = value;
  setup.wIndex = index;
  setup.wLength = len;
  setup.attrs = 0;

  if ((err = usb_pipe_ctrl_xfer_wait (usbdev->dev_data->dev_default_ph,
				      &setup,
				      &data,
				      &completion_reason,
				      &cb_flags, 0)) != USB_SUCCESS)
    {
      char tmp[128], *s = tmp;
      unsigned char *p = buf;
      int i;

      sprintf (s, "Msg (rq=0x%x, val=0x%04x, ix=0x%04x, len=%d): ", rq, value,
	       index, len);
      for (i = 0; i < len; i++)
	{
	  s += strlen (s);
	  sprintf (s, "%02x ", p[i]);
	}
      cmn_err (CE_CONT, "%s\n", tmp);

      cmn_err (CE_NOTE, "usb_pipe_ctrl_xfer_wait write failed: %s (%s)\n",
	       usb_errstr (err), usb_cb_err (completion_reason));
      cmn_err (CE_CONT, "bRq %x, wIx %x, wVal %x, wLen %d\n", rq, index,
	       value, len);
      freemsg (data);
      return OSS_EIO;
    }

  freemsg (data);
  return len;
}

/*ARGSUSED*/
int
udi_usb_rcv_control_msg (udi_usb_devc * usbdev, unsigned int endpoint,
			 unsigned char rq,
			 unsigned char rqtype,
			 unsigned short value,
			 unsigned short index,
			 void *buf, int len, int timeout)
{
  int err, l;

  usb_ctrl_setup_t setup;
  usb_cr_t completion_reason;
  usb_cb_flags_t cb_flags;
  mblk_t *data = NULL;

  if (usbdev == NULL)
    {
      cmn_err (CE_CONT, "udi_usb_rcv_control_msg: usbdev==NULL\n");
      return OSS_EFAULT;
    }

  setup.bmRequestType = rqtype | USB_DEV_REQ_DEV_TO_HOST;
  setup.bRequest = rq;
  setup.wValue = value;
  setup.wIndex = index;
  setup.wLength = len;
  setup.attrs = 0;

  if ((err = usb_pipe_ctrl_xfer_wait (usbdev->dev_data->dev_default_ph,
				      &setup,
				      &data,
				      &completion_reason,
				      &cb_flags, 0)) != USB_SUCCESS)
    {
      char tmp[128], *s = tmp;

      sprintf (s, "Msg (rq=0x%02x, val=0x%04x, ix=0x%04x, len=%d): ", rq,
	       value, index, len);
      cmn_err (CE_CONT, "%s\n", tmp);
      cmn_err (CE_NOTE, "usb_pipe_ctrl_xfer_wait read failed: %s (%s)\n",
	       usb_errstr (err), usb_cb_err (completion_reason));
      freemsg (data);
      return OSS_EIO;
    }

   /*LINTED*/ l = data->b_wptr - data->b_rptr;

  memcpy (buf, data->b_rptr, l);
  freemsg (data);

  return l;
}

/* Endpoint/pipe access */

struct _udi_endpoint_handle_t
{
  usb_pipe_handle_t pipe_handle;
  unsigned char *ep_desc;
  udi_usb_devc *usbdev;
};

udi_endpoint_handle_t *
udi_open_endpoint (udi_usb_devc * usbdev, void *ep_desc)
{
  udi_endpoint_handle_t *h;
  int err;
  usb_pipe_policy_t policy;

  if ((h = KERNEL_MALLOC (sizeof (*h))) == NULL)
    return NULL;

  policy.pp_max_async_reqs = 2;

  if ((err = usb_pipe_open (usbdev->osdev->dip, ep_desc, &policy,
			    USB_FLAGS_SLEEP, &h->pipe_handle)) != USB_SUCCESS)
    {
      cmn_err (CE_WARN, "usb_pipe_open() failed %d (%s)\n", err,
	       usb_cb_err (err));
      KERNEL_FREE (h);
      return NULL;
    }

  h->ep_desc = ep_desc;
  h->usbdev = usbdev;

  return h;
}

void
udi_close_endpoint (udi_endpoint_handle_t * h)
{
  usb_pipe_close (h->usbdev->osdev->dip, h->pipe_handle, USB_FLAGS_SLEEP,
		  NULL, 0);
  KERNEL_FREE (h);
}

int
udi_endpoint_get_num (udi_endpoint_handle_t * eph)
{
  return eph->ep_desc[2] & 0xff;
}

/* Request stuff */

struct udi_usb_request
{
  dev_info_t *dip;
  usb_pipe_handle_t pipe_handle;
  udi_usb_complete_func_t callback;
  void *callback_arg;
  int active;
  int xfer_type;
  mblk_t *data;

  /*
   * Recording
   */
  int actlen;

  usb_isoc_req_t *isoc_req;
  usb_bulk_req_t *bulk_req;
  usb_intr_req_t *intr_req;
};

 /*ARGSUSED*/
  udi_usb_request_t
  * udi_usb_alloc_request (udi_usb_devc * usbdev, udi_endpoint_handle_t * eph,
			   int nframes, int xfer_type)
{
  udi_usb_request_t *req;

  if ((req = KERNEL_MALLOC (sizeof (*req))) == NULL)
    {
      cmn_err (CE_WARN, "udi_usb_alloc_request: Out of memory\n");
      return NULL;
    }

  req->xfer_type = xfer_type;
  req->dip = usbdev->osdev->dip;
  req->pipe_handle = eph->pipe_handle;

  switch (xfer_type)
    {
    case UDI_USBXFER_ISO_READ:
      return req;
      break;

    case UDI_USBXFER_ISO_WRITE:
      return req;
      break;

    case UDI_USBXFER_BULK_READ:
      return req;
      break;

    case UDI_USBXFER_INTR_READ:
      return req;
      break;

    case UDI_USBXFER_BULK_WRITE:
      return req;
      break;

    default:
      cmn_err (CE_WARN, "Internal error - bad transfer type %d\n", xfer_type);
      KERNEL_FREE (req);
      return NULL;
    }
}

void
udi_usb_free_request (udi_usb_request_t * req)
{
  if (req == NULL)
    return;

  udi_usb_cancel_request (req);

  if (req->active)
    {
      cmn_err (CE_WARN, "Warning: Freeing active request\n");
    }

  switch (req->xfer_type)
    {
    case UDI_USBXFER_ISO_WRITE:
      req->isoc_req = NULL;
      break;

    case UDI_USBXFER_ISO_READ:
      req->isoc_req = NULL;
      break;

    case UDI_USBXFER_BULK_WRITE:
      req->bulk_req = NULL;
      break;

    case UDI_USBXFER_BULK_READ:
      req->bulk_req = NULL;
      break;

    case UDI_USBXFER_INTR_READ:
      req->intr_req = NULL;
      break;

    default:
      cmn_err (CE_WARN, "Internal error - bad transfer type %d\n",
	       req->xfer_type);
    }

  KERNEL_FREE (req);
}

/*ARGSUSED*/
static void
isoc_play_callback (usb_pipe_handle_t ph, usb_isoc_req_t * isoc_req)
{
  udi_usb_request_t *req =
    (udi_usb_request_t *) isoc_req->isoc_client_private;

  usb_free_isoc_req (isoc_req);

  req->isoc_req = NULL;
  req->active = 0;

  req->callback (req, req->callback_arg);
}

/*ARGSUSED*/
static void
isoc_rec_callback (usb_pipe_handle_t ph, usb_isoc_req_t * isoc_req)
{
  int i;
  udi_usb_request_t *req =
    (udi_usb_request_t *) isoc_req->isoc_client_private;

  req->data = isoc_req->isoc_data;

  for (i = 0; i < isoc_req->isoc_pkts_count; i++)
    {
      int len;

      len = isoc_req->isoc_pkt_descr[i].isoc_pkt_actual_length;

      req->actlen = len;

      req->callback (req, req->callback_arg);

      req->data->b_rptr += len;
    }

  req->active = 0;
  usb_free_isoc_req (isoc_req);
}

/*ARGSUSED*/
static void
isoc_exc_callback (usb_pipe_handle_t ph, usb_isoc_req_t * isoc_req)
{
  udi_usb_request_t *req =
    (udi_usb_request_t *) isoc_req->isoc_client_private;
  usb_cr_t reason;

  reason = isoc_req->isoc_completion_reason;

  usb_free_isoc_req (isoc_req);
  req->isoc_req = NULL;
  req->active = 0;

  if (reason && reason != USB_CR_FLUSHED && reason != USB_CR_PIPE_CLOSING)
    cmn_err (CE_CONT, "USB isoc transfer completion status %s\n",
	     usb_cr_err (reason));
}

/*ARGSUSED*/
void
bulk_play_callback (usb_pipe_handle_t ph, usb_bulk_req_t * bulk_req)
{
  udi_usb_request_t *req =
    (udi_usb_request_t *) bulk_req->bulk_client_private;

  usb_free_bulk_req (bulk_req);

  req->bulk_req = NULL;
  req->active = 0;

  req->callback (req, req->callback_arg);
}

/*ARGSUSED*/
void
bulk_rec_callback (usb_pipe_handle_t ph, usb_bulk_req_t * bulk_req)
{
  udi_usb_request_t *req =
    (udi_usb_request_t *) bulk_req->bulk_client_private;

  req->data = bulk_req->bulk_data;
  req->actlen = bulk_req->bulk_len;
  req->active = 0;

  req->callback (req, req->callback_arg);
  //usb_free_bulk_req (bulk_req);
}

/*ARGSUSED*/
static void
bulk_exc_callback (usb_pipe_handle_t ph, usb_bulk_req_t * bulk_req)
{
  udi_usb_request_t *req =
    (udi_usb_request_t *) bulk_req->bulk_client_private;
  usb_cr_t reason;

  reason = bulk_req->bulk_completion_reason;

  usb_free_bulk_req (bulk_req);
  req->bulk_req = NULL;
  req->active = 0;

  if (reason && reason != USB_CR_FLUSHED && reason != USB_CR_PIPE_CLOSING)
    cmn_err (CE_CONT, "USB bulk transfer completion status %s\n",
	     usb_cr_err (reason));
}

/*ARGSUSED*/
void
intr_rec_callback (usb_pipe_handle_t ph, usb_intr_req_t * intr_req)
{
  udi_usb_request_t *req =
    (udi_usb_request_t *) intr_req->intr_client_private;

  req->data = intr_req->intr_data;
  req->actlen = intr_req->intr_len;
  req->active = 0;

  req->callback (req, req->callback_arg);
  //usb_free_intr_req (intr_req);
}

/*ARGSUSED*/
static void
intr_exc_callback (usb_pipe_handle_t ph, usb_intr_req_t * intr_req)
{
  udi_usb_request_t *req =
    (udi_usb_request_t *) intr_req->intr_client_private;
  usb_cr_t reason;

  reason = intr_req->intr_completion_reason;

  usb_free_intr_req (intr_req);
  req->intr_req = NULL;
  req->active = 0;

  if (reason && reason != USB_CR_FLUSHED && reason != USB_CR_PIPE_CLOSING)
    cmn_err (CE_CONT, "USB intr transfer completion status %s\n",
	     usb_cr_err (reason));
}

/*ARGSUSED*/
int
udi_usb_submit_request (udi_usb_request_t * req,
			udi_usb_complete_func_t callback, void *callback_arg,
			udi_endpoint_handle_t * eph, int xfer_type,
			void *data, int len)
{
  int err;

  req->callback = callback;
  req->callback_arg = callback_arg;

  if (req->active)
    return 0;

  switch (xfer_type)
    {
    case UDI_USBXFER_ISO_WRITE:
      {
	usb_isoc_req_t *isoc_req;

	if (req->isoc_req != NULL)
	  isoc_req = req->isoc_req;
	else
	  {
	    req->data = allocb (len, BPRI_HI);
	    if (req->data == NULL)
	      {
		cmn_err (CE_WARN, "allocb_wait (isoc) failed\n");
		KERNEL_FREE (req);
		return OSS_ENOMEM;
	      }

	    if ((isoc_req = usb_alloc_isoc_req (req->dip, 1, 0, 0)) == NULL)
	      {
		cmn_err (CE_WARN, "usb_alloc_isoc_req failed\n");
		freemsg (req->data);
		KERNEL_FREE (req);
		return OSS_ENOMEM;
	      }
	    req->isoc_req = isoc_req;
	  }

	if (isoc_req == NULL)
	  {
	    cmn_err (CE_WARN, "req->isoc==NULL\n");
	    return OSS_EIO;
	  }

	memcpy (req->data->b_wptr, data, len);
	req->data->b_wptr += len;

	isoc_req->isoc_data = req->data;
	isoc_req->isoc_pkt_descr[0].isoc_pkt_length = len;
	isoc_req->isoc_pkts_count = 1;
	isoc_req->isoc_pkts_length = len;
	isoc_req->isoc_attributes =
	  USB_ATTRS_ISOC_XFER_ASAP | USB_ATTRS_AUTOCLEARING;
	isoc_req->isoc_cb = isoc_play_callback;
	isoc_req->isoc_exc_cb = isoc_exc_callback;
	isoc_req->isoc_client_private = (usb_opaque_t) req;

	if ((err =
	     usb_pipe_isoc_xfer (req->pipe_handle, isoc_req,
				 0)) != USB_SUCCESS)
	  {
	    cmn_err (CE_WARN, "usb_pipe_isoc_xfer failed (%s)\n",
		     usb_errstr (err));
	    return OSS_EIO;
	  }
	req->active = 1;
      }
      break;

    case UDI_USBXFER_ISO_READ:
      {
	usb_isoc_req_t *isoc_req;

	if (req->isoc_req != NULL)
	  isoc_req = req->isoc_req;
	else
	  {
	    if ((isoc_req =
		 usb_alloc_isoc_req (req->dip, 1, len,
				     USB_FLAGS_SLEEP)) == NULL)
	      {
		cmn_err (CE_WARN, "usb_alloc_isoc_req failed\n");
		KERNEL_FREE (req);
		return OSS_ENOMEM;
	      }
	    req->isoc_req = isoc_req;
	  }

	if (isoc_req == NULL)
	  {
	    cmn_err (CE_WARN, "req->isoc==NULL\n");
	    return OSS_EIO;
	  }

	isoc_req->isoc_attributes = USB_ATTRS_ISOC_XFER_ASAP;
	isoc_req->isoc_cb = isoc_rec_callback;
	isoc_req->isoc_exc_cb = isoc_exc_callback;
	isoc_req->isoc_client_private = (usb_opaque_t) req;
	isoc_req->isoc_pkt_descr[0].isoc_pkt_length = len;
	isoc_req->isoc_pkts_count = 1;
	isoc_req->isoc_pkts_length = len;
	req->active = 1;

	if ((err =
	     usb_pipe_isoc_xfer (req->pipe_handle, isoc_req,
				 USB_FLAGS_NOSLEEP)) != USB_SUCCESS)
	  {
	    cmn_err (CE_WARN, "usb_pipe_isoc_xfer failed (%s)\n",
		     usb_errstr (err));
	    return OSS_EIO;
	  }
      }
      break;

    case UDI_USBXFER_BULK_WRITE:
      {
	usb_bulk_req_t *bulk_req;

	if (req->bulk_req != NULL)
	  bulk_req = req->bulk_req;
	else
	  {
	    req->data = allocb (len, BPRI_HI);
	    if (req->data == NULL)
	      {
		cmn_err (CE_WARN, "allocb_wait (bulk) failed\n");
		KERNEL_FREE (req);
		return OSS_ENOMEM;
	      }

	    if ((bulk_req = usb_alloc_bulk_req (req->dip, len, 0)) == NULL)
	      {
		cmn_err (CE_WARN, "usb_alloc_bulk_req failed\n");
		freemsg (req->data);
		KERNEL_FREE (req);
		return OSS_ENOMEM;
	      }
	    req->bulk_req = bulk_req;
	  }

	if (bulk_req == NULL)
	  {
	    cmn_err (CE_WARN, "req->bulk==NULL\n");
	    return OSS_EIO;
	  }

	memcpy (req->data->b_wptr, data, len);
	req->data->b_wptr += len;

	bulk_req->bulk_data = req->data;
	bulk_req->bulk_len = len;
	bulk_req->bulk_timeout = 5;	/* 5 seconds */
	bulk_req->bulk_attributes = USB_ATTRS_AUTOCLEARING;
	bulk_req->bulk_cb = bulk_play_callback;
	bulk_req->bulk_exc_cb = bulk_exc_callback;
	bulk_req->bulk_client_private = (usb_opaque_t) req;

	if ((err =
	     usb_pipe_bulk_xfer (req->pipe_handle, bulk_req,
				 0)) != USB_SUCCESS)
	  {
	    cmn_err (CE_WARN, "usb_pipe_bulk_xfer failed (%s)\n",
		     usb_errstr (err));
	    return OSS_EIO;
	  }
	req->active = 1;
      }
      break;

    case UDI_USBXFER_BULK_READ:
      {
	usb_bulk_req_t *bulk_req;

	if (req->bulk_req != NULL)
	  bulk_req = req->bulk_req;
	else
	  {
#if 0
	    req->data = allocb (len, BPRI_HI);
	    if (req->data == NULL)
	      {
		cmn_err (CE_WARN, "allocb_wait (bulk) failed\n");
		KERNEL_FREE (req);
		return OSS_ENOMEM;
	      }
#endif

	    if ((bulk_req =
		 usb_alloc_bulk_req (req->dip, len, USB_FLAGS_SLEEP)) == NULL)
	      {
		cmn_err (CE_WARN, "usb_alloc_bulk_req failed\n");
		freemsg (req->data);
		KERNEL_FREE (req);
		return OSS_ENOMEM;
	      }
	    req->bulk_req = bulk_req;
	  }

	if (bulk_req == NULL)
	  {
	    cmn_err (CE_WARN, "req->bulk==NULL\n");
	    return OSS_EIO;
	  }

	bulk_req->bulk_attributes = USB_ATTRS_SHORT_XFER_OK;
	bulk_req->bulk_cb = bulk_rec_callback;
	bulk_req->bulk_exc_cb = bulk_exc_callback;
	bulk_req->bulk_client_private = (usb_opaque_t) req;
	// bulk_req->bulk_data = data;
	bulk_req->bulk_len = len;
	bulk_req->bulk_timeout = 0x7fffffff;	/* As long as possible */
	req->active = 1;

	if ((err =
	     usb_pipe_bulk_xfer (req->pipe_handle, bulk_req,
				 USB_FLAGS_NOSLEEP)) != USB_SUCCESS)
	  {
	    cmn_err (CE_WARN, "usb_pipe_bulk_xfer failed (%s)\n",
		     usb_errstr (err));
	    return OSS_EIO;
	  }
      }
      break;

    case UDI_USBXFER_INTR_READ:
      {
	usb_intr_req_t *intr_req;

	if (req->intr_req != NULL)
	  intr_req = req->intr_req;
	else
	  {
#if 0
	    req->data = allocb (len, BPRI_HI);
	    if (req->data == NULL)
	      {
		cmn_err (CE_WARN, "allocb_wait (intr) failed\n");
		KERNEL_FREE (req);
		return OSS_ENOMEM;
	      }
#endif

	    if ((intr_req =
		 usb_alloc_intr_req (req->dip, len, USB_FLAGS_SLEEP)) == NULL)
	      {
		cmn_err (CE_WARN, "usb_alloc_intr_req failed\n");
		freemsg (req->data);
		KERNEL_FREE (req);
		return OSS_ENOMEM;
	      }
	    req->intr_req = intr_req;
	  }

	if (intr_req == NULL)
	  {
	    cmn_err (CE_WARN, "req->intr==NULL\n");
	    return OSS_EIO;
	  }

	intr_req->intr_attributes =
	  USB_ATTRS_SHORT_XFER_OK | USB_ATTRS_ONE_XFER;
	intr_req->intr_cb = intr_rec_callback;
	intr_req->intr_exc_cb = intr_exc_callback;
	intr_req->intr_client_private = (usb_opaque_t) req;
	intr_req->intr_data = NULL;
	intr_req->intr_len = len;
	intr_req->intr_timeout = 0x7fffffff;	/* As long as possible */
	req->active = 1;

	if ((err =
	     usb_pipe_intr_xfer (req->pipe_handle, intr_req,
				 0)) != USB_SUCCESS)
	  {
	    cmn_err (CE_WARN, "usb_pipe_intr_xfer failed (%s)\n",
		     usb_errstr (err));
	    return OSS_EIO;
	  }
      }
      break;

    default:
      cmn_err (CE_WARN, "Unimplemented transfer type %d\n", xfer_type);
      return OSS_EIO;
    }

  return 0;
}

int
udi_usb_request_actlen (udi_usb_request_t * request)
{
  return request->actlen;
}

unsigned char *
udi_usb_request_actdata (udi_usb_request_t * request)
{
  return request->data->b_rptr;
}

void
udi_usb_cancel_request (udi_usb_request_t * req)
{
  if (req == NULL || !req->active)
    return;

  req->active = 0;
  usb_pipe_reset (req->dip, req->pipe_handle, USB_FLAGS_SLEEP, NULL, 0);

  switch (req->xfer_type)
    {
    case UDI_USBXFER_ISO_WRITE:
      break;

    case UDI_USBXFER_ISO_READ:
      usb_pipe_stop_isoc_polling (req->pipe_handle, USB_FLAGS_SLEEP);
      //if (req->isoc_req!=NULL)
      //usb_free_isoc_req(req->isoc_req);
      req->isoc_req = NULL;
      break;

    case UDI_USBXFER_BULK_WRITE:
      break;

    case UDI_USBXFER_BULK_READ:
      req->bulk_req = NULL;
      break;

    case UDI_USBXFER_INTR_READ:
      req->intr_req = NULL;
      break;

    default:
      cmn_err (CE_WARN, "Internal error - bad transfer type %d\n",
	       req->xfer_type);
    }
}
#endif
