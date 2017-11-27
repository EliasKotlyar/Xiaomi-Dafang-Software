#ifndef __APICAL_JTAG_CONFIG_H__
#define __APICAL_JTAG_CONFIG_H__


/*-----------------------------------------------------------------------------
  This confidential and proprietary software/information may be used only
  as authorized by a licensing agreement from Apical Limited

  (C) COPYRIGHT 2011 - 2014 Apical Limited
  ALL RIGHTS RESERVED

  The entire notice above must be reproduced on all authorized
  copies and copies may only be made to the extent permitted
  by a licensing agreement from Apical Limited.
  -----------------------------------------------------------------------------*/

#include "apical_isp_io.h"

// ------------------------------------------------------------------------------ //
// Instance 'jtag' of module 'jtag_frame_capture'
// ------------------------------------------------------------------------------ //

#define APICAL_JTAG_BASE_ADDR (0x21000L)
#define APICAL_JTAG_SIZE (0x400)

#define APICAL_JTAG_ARRAY_DATA_DEFAULT (0x0)
#define APICAL_JTAG_ARRAY_DATA_DATASIZE (32)

// args: index (0-255), data (32-bit)
static __inline void apical_jtag_array_data_write(uint32_t index, uint32_t data) {
	APICAL_WRITE_32(0x21000L + (index << 2), data);
}
static __inline uint32_t apical_jtag_array_data_read(uint32_t index) {
	return APICAL_READ_32(0x21000L + (index << 2));
}
// ------------------------------------------------------------------------------ //
#endif //__APICAL_JTAG_CONFIG_H__
