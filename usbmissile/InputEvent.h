/**
 * @filename InputEvent.h
 *
 * @brief header files for InputEvent.c
 * 
 * @copyright Copyright (C) 2005-2008 Luke Cole
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
 *   $Id:$ 
 */

#ifndef __INC_InputEvent_h
#define __INC_InputEvent_h

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================

int IEOpen(char *device, int *fd);
int IEName(int fd, char *name, int size);
int IEVersion(int fd, int *version);
int IERead(int fd, struct input_event *ev, int *events);
int IEPrintDeviceInfo(int fd);

//=============================================================================

#ifdef __cplusplus
}
#endif

#endif //__INC_InputEvent_h

//=============================================================================

/*
 * Local Variables:
 * mode: C
 * End:
 */
