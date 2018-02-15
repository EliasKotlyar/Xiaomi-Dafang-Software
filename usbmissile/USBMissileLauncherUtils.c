/**
 * @filename USBMissileLauncherUtils.c
 *
 * @brief USB Missile Launcher and USB Circus Cannon Utilities
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
#include <errno.h>
#include <linux/input.h>
#endif
#include "InputEvent.h"
#include "USBMissileLauncher.h"

#define USB_TIMEOUT 1000 /* milliseconds */

int debug_level = 0;
missile_usb *control;

//=============================================================================

int HandleKeyboardEvent(struct input_event *ev, int events, int device_type);

//=============================================================================

void usage(char *filename) {
  fprintf(stderr, "\n"
	  "M&S USB Missile Luncer Utils Created by Luke Cole\n"
	  "=================================================================\n"
	  "Usage: %s [option]\n\n"

	  "Options:\n"
	  "-h            Help\n"
	  "-d level      Debug on and set to 'level'\n"
	  "-c device     Path to Input Event Device (e.g. /dev/input/event0)\n"
	  "-t type       Device Type (1 - Missile Launcher - default\n"
	  "                           2 - Circus Cannon)\n"
	  "-F            Fire Missile\n"
	  "-L            Rotate Left\n"
	  "-R            Rotate Right\n"
	  "-U            Tilt Up\n"
	  "-D            Tilt Down\n"
	  "-S n          Stop after n milliseconds (> 100)\n",
	  
	  filename);
}

//=============================================================================

int main(int argc, char **argv) {
  const char *optstring = "hd:c:t:FLRUDS:";

  int o;
  int delay = 0;

  int device_type = 1;
  
  char *device = NULL;

  unsigned int set_event = 0;
  unsigned int set_fire = 0, set_left = 0, set_right = 0;
  unsigned int set_up = 0, set_down = 0, set_stop = 0;

  //---------------------------------------------------------------------------
  // Get args

  o = getopt(argc, argv, optstring);

  while (o != -1) {
    switch (o) {

    case 'h':
      usage(argv[0]);
      return 0;

    case 'd':
      debug_level = atoi(optarg);
      fprintf(stderr, "debug_level set to %d\n", debug_level);
      break;

    case 'c':
      device = optarg;
      set_event = 1;
      break;

    case 't':
      device_type = atoi(optarg);
      break;

    case 'F':
      set_fire = 1;
      break;

    case 'U':
      set_up = 1;
      break;

    case 'D':
      set_down = 1;
      break;

    case 'L':
      set_left = 1;
      break;

    case 'R':
      set_right = 1;
      break;

    case 'S':
      set_stop = 1;
      delay = atoi(optarg);
      break;

    }
    o = getopt(argc, argv, optstring);
  }

  //---------------------------------------------------------------------------

  if (missile_usb_initialise() != 0) {
    fprintf(stderr, "missile_usb_initalise failed: %s\n", strerror(errno));
    return -1;
  }
  
  control = missile_usb_create(debug_level, USB_TIMEOUT);
  if (control == NULL) {
    fprintf(stderr, "missile_usb_create() failed\n");
    return -1;
  }
  
  if (missile_usb_finddevice(control, 0, device_type) != 0) {
    fprintf(stderr, "USBMissileLauncher device not found\n");
    return -1;
  }

  if (debug_level)
    fprintf(stderr, "Now we're ready.  Move the thing around, and FIRE!\n");

  //---------------------------------------------------------------------------

  int fd;
  struct input_event ev[64];
  int events;

  if (set_event) {

    if (device == NULL) {
      fprintf(stderr, "No device given\n");
      usage(argv[0]);    
      return 0;
    }
    
    if (IEOpen(device, &fd) < 0) {
      fprintf(stderr, "IEOpen(%s, fd) failed\n", device);
      return -1;
    }

    while (1) {

      if (IERead(fd, ev, &events) < 0) {
	fprintf(stderr, "IERead() failed\n");
	return -1;
      }

      if (HandleKeyboardEvent(ev, events, device_type) < 0) {
	fprintf(stderr, "HandleKeyboardEvent() failed\n");
	return -1;
      }

      usleep(200000);
	
    }
    
  }

  //---------------------------------------------------------------------------

  char msg = 0x00;

  switch (device_type) {
    
  case DEVICE_TYPE_MISSILE_LAUNCHER:
  
    if (set_left)
      msg |= MISSILE_LAUNCHER_CMD_LEFT;
    
    if (set_right)
      msg |= MISSILE_LAUNCHER_CMD_RIGHT;
    
    if (set_up)
      msg |= MISSILE_LAUNCHER_CMD_UP;
    
    if (set_down)
      msg |= MISSILE_LAUNCHER_CMD_DOWN;
    
    if (set_fire)
      msg |= MISSILE_LAUNCHER_CMD_FIRE;

    missile_do(control, msg, device_type);
    
    if (set_stop) {
      usleep(delay * 1000);
      missile_do(control, MISSILE_LAUNCHER_CMD_STOP, device_type);
    }

    break;

  case DEVICE_TYPE_CIRCUS_CANNON:

    if (set_left)
      msg |= CIRCUS_CANNON_CMD_LEFT;
    
    if (set_right)
      msg |= CIRCUS_CANNON_CMD_RIGHT;
    
    if (set_up)
      msg |= CIRCUS_CANNON_CMD_UP;
    
    if (set_down)
      msg |= CIRCUS_CANNON_CMD_DOWN;
    
    if (set_fire)
      msg |= CIRCUS_CANNON_CMD_FIRE;

    missile_do(control, msg, device_type);

    if (set_stop) {
      usleep(delay * 1000);
      missile_do(control, CIRCUS_CANNON_CMD_STOP, device_type);
    }
      
    break;
    
  default:
    printf("Device Type (%d) not implemented, please do it!\n",
	   device_type);
    return -1;
    
  }

  missile_usb_destroy(control);  

  //---------------------------------------------------------------------------

  return 0;
}

//=============================================================================

int HandleKeyboardEvent(struct input_event *ev, int events, int device_type) {

  char msg = 0x00;
  int i;

  int key_pressed = 0;

  for (i = 0; i < events; i++) {

    switch (ev[i].type) {

    case EV_KEY:

      switch (device_type) {
	
      case DEVICE_TYPE_MISSILE_LAUNCHER:
	
	switch (ev[i].code) {
	  
	case KEY_UP:
	  msg |= MISSILE_LAUNCHER_CMD_UP;
	  key_pressed = 1;
	  break;
	  
	case KEY_DOWN:
	  msg |= MISSILE_LAUNCHER_CMD_DOWN;
	  key_pressed = 1;
	  break;
	  
	case KEY_LEFT:
	  msg |= MISSILE_LAUNCHER_CMD_LEFT;
	  key_pressed = 1;
	  break;
	  
	case KEY_RIGHT:
	  msg |= MISSILE_LAUNCHER_CMD_RIGHT;
	  key_pressed = 1;
	  break;
	  
	case KEY_F:
	  msg |= MISSILE_LAUNCHER_CMD_FIRE;
	  key_pressed = 1;
	  break;
	  
	case KEY_S: case KEY_SPACE:
	  msg |= MISSILE_LAUNCHER_CMD_STOP;
	  key_pressed = 1;
	  break;
	  
	default:
	  if (debug_level > 1000)
	    fprintf(stderr, "Unused Event Code: %d\n", ev[i].code);
	}

	break;

      case DEVICE_TYPE_CIRCUS_CANNON:

	switch (ev[i].code) {
	  
	case KEY_UP:
	  msg |= CIRCUS_CANNON_CMD_UP;
	  key_pressed = 1;
	  break;
	  
	case KEY_DOWN:
	  msg |= CIRCUS_CANNON_CMD_DOWN;
	  key_pressed = 1;
	  break;
	  
	case KEY_LEFT:
	  msg |= CIRCUS_CANNON_CMD_LEFT;
	  key_pressed = 1;
	  break;
	  
	case KEY_RIGHT:
	  msg |= CIRCUS_CANNON_CMD_RIGHT;
	  key_pressed = 1;
	  break;
	  
	case KEY_F:
	  msg |= CIRCUS_CANNON_CMD_FIRE;
	  key_pressed = 1;
	  break;
	  
	case KEY_S: case KEY_SPACE:
	  msg |= CIRCUS_CANNON_CMD_STOP;
	  key_pressed = 1;
	  break;
	  
	default:
	  if (debug_level > 1000)
	    fprintf(stderr, "Unused Event Code: %d\n", ev[i].code);
	}

	break;
	
      default:
	printf("Device Type (%d) not implemented, please do it!\n",
	       device_type);
	return -1;

      } // switch (device_type)
      
      break;
	
    default:
      if (debug_level > 1000)
	fprintf(stderr, "Unused Event Type: %d\n", ev[i].type);
    }

  }

  if (key_pressed) {
    if (debug_level)
      fprintf(stderr, "msg %d\n", msg);
    missile_do(control, msg, device_type);
  }
  
  return 0;
}

//=============================================================================

/*
 * Local Variables:
 * mode: C
 * End:
 */
