/**
 * @filename USBMissileLauncher.c
 *
 * @brief USB Missile Launcher and USB Circus Cannon Library
 *
 * @copyright Copyright (C) 2006-2008 Luke Cole
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 * 
 * 
 * @author  Luke Cole
 *          luke@coletek.org
 *
 * @version
 *   $Id$
 */

#ifndef DEPEND
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <asm/io.h>
#include <assert.h>
#include <usb.h>
#include <errno.h>
#endif
#include "USBMissileLauncher.h"

extern int debug_level;

//=============================================================================

struct missile_usb {
  struct usb_device *device;
  usb_dev_handle    *handle;
  int                debug;
  int                timeout;
  int                interface_claimed;
};

//=============================================================================

int missile_usb_initialise(void) {
  usb_set_debug(0);
  usb_init();
  usb_find_busses();
  usb_find_devices();
  
  return 0;
}

//=============================================================================

missile_usb *missile_usb_create(int debug, int timeout) {
  missile_usb *control = malloc(sizeof(*control));
  if (control == NULL)
    return NULL;
  
  control->device            = NULL;
  control->handle            = NULL;
  control->debug             = debug;
  control->timeout           = timeout;
  control->interface_claimed = 0;
  
  return control;
}

//=============================================================================

void missile_usb_destroy(missile_usb *control) {
  if (control == NULL)
    return;
  
  if (control->handle != NULL)
    usb_close(control->handle);
  
  free(control);
}

//=============================================================================

int missile_usb_finddevice(missile_usb *control, int device_num, 
			   int device_type) {
  
  int found_device = 0;
  int device_count = 0;
  struct usb_bus  *bus;
  
  assert(control != NULL);
  assert(control->interface_claimed == 0);
  assert(device_num >= 0 && device_num < 255);
  
  for (bus = usb_get_busses(); bus != NULL; bus = bus->next) {

    struct usb_device *dev;
    
    for (dev = bus->devices; dev != NULL; dev = dev->next) {
      
      if (control->debug) {
	printf("Found device: %04x-%04x\n",
	       dev->descriptor.idVendor,
	       dev->descriptor.idProduct);
      }

      switch (device_type) {
	
      case DEVICE_TYPE_MISSILE_LAUNCHER:
	if (dev->descriptor.idVendor  == MISSILE_LAUNCHER_VENDOR_ID &&
	    dev->descriptor.idProduct == MISSILE_LAUNCHER_PRODUCT_ID)
	  found_device = 1;
	break;
	
      case DEVICE_TYPE_CIRCUS_CANNON:
	if (dev->descriptor.idVendor  == CIRCUS_CANNON_VENDOR_ID &&
	    dev->descriptor.idProduct == CIRCUS_CANNON_PRODUCT_ID)
	  found_device = 1;
	break;
	
      default:
 	printf("Device Type (%d) not implemented, please do it!\n",
	       device_type);
	return -1;
	
      }
      
      if (found_device) {
	  	
	if (control->debug)
	  printf("Found control num %d\n", device_count);
	
	if (device_count == device_num) {
	  
	  control->device = dev;
	  control->handle = usb_open(control->device);
	  
	  if (control->handle == NULL) {
	    perror("usb_open failed\n");
	    return -1;
	  }
	  
	  if (control->debug)
	    printf("Successfully opened device\n");

	  return 0;
	}

	device_count++;
      }

    }

  }
  
  if (debug_level)
    fprintf(stderr, 
	    "missile_usb_finddevice(%d) failed to find "
	    "missile launcher device %04x-%04x or "
	    "circus cannon device %04x-%04x\n",
	    device_count, 
	    MISSILE_LAUNCHER_VENDOR_ID, MISSILE_LAUNCHER_PRODUCT_ID,
	    CIRCUS_CANNON_VENDOR_ID, CIRCUS_CANNON_PRODUCT_ID);
  
  return -1;
}

//=============================================================================

static int claim_interface(missile_usb *control) {
  int  ret;
  
  if (control->interface_claimed == 0) {

    if (control->debug)
      printf("Trying to detach kernel driver\n");
    
    /* try to detach device in case usbhid has got hold of it */
    ret = usb_detach_kernel_driver_np(control->handle, 0);
    
    if (ret != 0) {
      if (errno == ENODATA) {
	if (control->debug)
	  printf("Device already detached\n");
      } else {
	if (control->debug) {
	  printf("Detach failed: %s[%d]\n", strerror(errno), errno);
	  printf("Continuing anyway\n");
	}
      }
    } else {
      if (control->debug)
	printf("detach successful\n");
    }

    /* And now for the other interface */
    ret = usb_detach_kernel_driver_np(control->handle, 1);
    
    if (ret != 0) {
      if (errno == ENODATA) {
	if (control->debug)
	  printf("Device already detached\n");
      } else {
	if (control->debug) {
	  printf("Detach failed: %s[%d]\n", strerror(errno), errno);
	  printf("Continuing anyway\n");
	}
      }
    } else {
      if (control->debug)
	printf("detach successful\n");
    }
    
    ret = usb_set_configuration(control->handle, 1);
    if (ret < 0) {
      perror("usb_set_configuration failed\n");
      return -1;
    }
    
    ret = usb_claim_interface(control->handle, 0);
    if (ret < 0) {
      perror("usb_claim_interface failed\n");
      return -1;
    }
    
    ret = usb_claim_interface(control->handle, 1);
    if (ret < 0) {
      perror("usb_claim_interface failed\n");
      return -1;
    }

    control->interface_claimed = 1;

  }

  return 0;
}

//=============================================================================

int missile_do(missile_usb *control, int cmd, int device_type) {

  int a, b, c, d, e;
    
  switch (device_type) {
      
  case DEVICE_TYPE_MISSILE_LAUNCHER:
    
    /* Two fixed-format initiator packets appear to be required */
    
    if (missile_usb_sendcommand(control, 'U', 'S', 'B', 'C', 0, 0, 4, 0 )) {
      fprintf(stderr, 
	      "missile_usb_sendcommand() failed: %s\n", strerror(errno));
      return -1;
    }
    
    if (missile_usb_sendcommand(control, 'U', 'S', 'B', 'C', 0, 64, 2, 0 )) {
      fprintf(stderr, 
	      "missile_usb_sendcommand() failed: %s\n", strerror(errno));
      return -1;
    }
    
    /* Now the actual movement command! */
    
    a = cmd & MISSILE_LAUNCHER_CMD_LEFT ? 1 : 0;
    b = cmd & MISSILE_LAUNCHER_CMD_RIGHT ? 1 : 0;
    c = cmd & MISSILE_LAUNCHER_CMD_UP ? 1 : 0;
    d = cmd & MISSILE_LAUNCHER_CMD_DOWN ? 1 : 0;
    e = cmd & MISSILE_LAUNCHER_CMD_FIRE ? 1 : 0;
    
    if (missile_usb_sendcommand64(control, 0, a, b, c, d, e, 8, 8 )) {
      fprintf(stderr, 
	      "missile_usb_sendcommand() failed: %s\n", strerror(errno));
      return -1;
    }

    break;
    
  case DEVICE_TYPE_CIRCUS_CANNON:

    if (missile_usb_sendcommand(control, cmd, 0, 0, 0, 0, 0, 0, 0)) {
      fprintf(stderr, "missile_usb_sendcommand() failed: %s\n", 
	      strerror(errno));
      return -1;
    }
    break;
        
  default:
    printf("Device Type (%d) not implemented, please do it!\n", device_type);
    return -1;

  }
  
  return 0;
}

//=============================================================================

int missile_usb_sendcommand(missile_usb *control, 
			    int a, int b, int c, int d, 
			    int e, int f, int g, int h) {
    
  unsigned char buf[8];
  int  ret;
  
  assert(control != NULL);
  
  ret = claim_interface(control);
  if (ret != 0) {
    return -1;
  }
  
  buf[0] = a;
  buf[1] = b;
  buf[2] = c;
  buf[3] = d;
  buf[4] = e;
  buf[5] = f;
  buf[6] = g;
  buf[7] = h;
  
  if (control->debug) {
    printf("sending bytes %d, %d, %d, %d, %d, %d, %d, %d\n",
	   a, b, c, d, e, f, g, h );
  }
  
  ret = usb_control_msg( control->handle, 0x21, 9, 0x2, 0x01, (char*) buf, 
			 8, control->timeout);
  if (ret != 8) {
    perror("usb_control_msg failed\n");
    return -1;
  }
  
  return 0;
}

//=============================================================================

int missile_usb_sendcommand64(missile_usb *control, 
			      int a, int b, int c, int d, 
			      int e, int f, int g, int h) {
  
  unsigned char buf[64];
  int  ret;
  
  assert(control != NULL);
  
  ret = claim_interface(control);
  if (ret != 0) {
    return -1;
  }
  
  memset(buf, 0, 64 );
  buf[0] = a;
  buf[1] = b;
  buf[2] = c;
  buf[3] = d;
  buf[4] = e;
  buf[5] = f;
  buf[6] = g;
  buf[7] = h;
  
  if (control->debug) {
    printf("sending bytes %d, %d, %d, %d, %d, %d, %d, %d\n",
	   a, b, c, d, e, f, g, h );
  }
  
  ret = usb_control_msg(control->handle, 0x21, 9, 
			0x2, 0x0, (char*) buf, 64,  control->timeout);

  if (ret != 64) {
    perror("usb_control_msg failed\n");
    return -1;
  }
  
  return 0;
}

//=============================================================================

/*
 * Local Variables:
 * mode: C
 * End:
 */
