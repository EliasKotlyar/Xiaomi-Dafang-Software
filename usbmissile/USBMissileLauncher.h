/**
 * @filename USBMissileLauncher.h
 *
 * @brief Header file for USBMissileLauncher.c
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
#include <sys/io.h>
#include <assert.h>
#include <usb.h>
#include <errno.h>
#endif

#ifndef __INC_USBMissileLauncher_h
#define __INC_USBMissileLauncher_h

// Device type, such as Missile Launcher and Circus Cannon
#define DEVICE_TYPE_UNKNOWN          0
#define DEVICE_TYPE_MISSILE_LAUNCHER 1
#define DEVICE_TYPE_CIRCUS_CANNON    2

// missile launcher usb vendor and product id
#define MISSILE_LAUNCHER_VENDOR_ID  0x1130
#define MISSILE_LAUNCHER_PRODUCT_ID 0x0202

// circus cannon usb vendor and product id
#define CIRCUS_CANNON_VENDOR_ID  0x1941
#define CIRCUS_CANNON_PRODUCT_ID 0x8021
  
// missile launcher packet cmd masks
#define MISSILE_LAUNCHER_CMD_STOP  0
#define MISSILE_LAUNCHER_CMD_LEFT  1
#define MISSILE_LAUNCHER_CMD_RIGHT 2
#define MISSILE_LAUNCHER_CMD_UP    4
#define MISSILE_LAUNCHER_CMD_DOWN  8
#define MISSILE_LAUNCHER_CMD_FIRE  16

// circus cannon packet cmd masks
#define CIRCUS_CANNON_CMD_STOP  0
#define CIRCUS_CANNON_CMD_LEFT  4
#define CIRCUS_CANNON_CMD_RIGHT 8
#define CIRCUS_CANNON_CMD_UP    1
#define CIRCUS_CANNON_CMD_DOWN  2
#define CIRCUS_CANNON_CMD_FIRE  16

#ifdef __cplusplus
extern "C" {
#endif

  /** opaque handle for device */
  typedef struct missile_usb missile_usb;
  
  /**
   * @brief Initialise library
   *
   * @note Must be called before calling any other functions
   *
   * @return 0 on success, -1 on failure
   */
  int missile_usb_initialise(void);
  
  /**
   * @brief Finalise library
   *
   * Do not call any library functions after calling this.
   */
  void missile_usb_finalise(void);
  
  /**
   * @brief Create a device handle
   *
   * @param debug   set to 1 to enable debug, 0 otherwise
   * @param timeout usb comms timeout in milliseconds (1000 would be 
   * reasonable)
   * 
   * @return pointer to opaque device handle on success, NULL on failure
   */
  missile_usb *missile_usb_create(int debug, int timeout);

  /**
   * @brief Destroy a device handle
   *
   * @param control device handle to destroy
   */
  void missile_usb_destroy(missile_usb *control);
  
  /**
   * @brief Attempt to find the usb missile launcher device
   *
   * @note Scans through all the devices on all the usb busses and
   * finds the 'device_num'th device.
   *
   * @param control    library handle
   * @param device_num 0 for first device, 1 for second, etc
   *
   * @return 0 on success, -1 on failure
   */
  int missile_usb_finddevice(missile_usb *control, int device_num,
			     int device_type);  
  
  /**
   * @brief Perform an USB Missile Launcher Command
   *
   * @note Thanks to Florent Papin <florent.papin@free.fr> for his
   *       code for the Circus Cannon
   */
  int missile_do(missile_usb *control, int cmd, int device_type);
    
  /**
   * @brief Send a usb command
   *
   * @note missile_usb_sendcommand64 not used for circus cannon
   *
   * @param control    library handle
   * @param ...        (first) 8 bytes of packet
   *
   * @return 0 on success, -1 on failure
   */
  int missile_usb_sendcommand(missile_usb *control, 
			      int, int, int, int, 
			      int, int, int, int);
  int missile_usb_sendcommand64(missile_usb *control, 
				int, int, int, int, 
				int, int, int, int);
  
  //===========================================================================
  
#ifdef __cplusplus
}
#endif

#endif //__INC_USBMissileLauncher_h

//=============================================================================

/*
 * Local Variables:
 * mode: C
 * End:
 */
