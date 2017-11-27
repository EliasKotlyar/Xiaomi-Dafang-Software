/*-----------------------------------------------------------------------------
	 This confidential and proprietary software/information may be used only
		as authorized by a licensing agreement from Apical Limited

				   (C) COPYRIGHT 2011 - 2015 Apical Limited
						  ALL RIGHTS RESERVED

	  The entire notice above must be reproduced on all authorized
	   copies and copies may only be made to the extent permitted
			 by a licensing agreement from Apical Limited.
-----------------------------------------------------------------------------*/

#ifndef SYSTEM_CHARDEV_H
#define SYSTEM_CHARDEV_H

int system_chardev_init(void);
int system_chardev_read(char *data, int size);
int system_chardev_write(const char *data, int size);
int system_chardev_destroy(void);

#endif /* SYSTEM_CHARDEV_H */
