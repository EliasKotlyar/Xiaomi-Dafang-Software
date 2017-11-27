/*-----------------------------------------------------------------------------
  This confidential and proprietary software/information may be used only
  as authorized by a licensing agreement from Apical Limited

  (C) COPYRIGHT 2011 - 2015 Apical Limited
  ALL RIGHTS RESERVED

  The entire notice above must be reproduced on all authorized
  copies and copies may only be made to the extent permitted
  by a licensing agreement from Apical Limited.
  -----------------------------------------------------------------------------*/

#ifndef __APICAL_ISP_IO_H__
#define __APICAL_ISP_IO_H__


#include <asm/io.h>
#include "apical_types.h"
#include "system_isp_io.h"
#include "apical_firmware_config.h"


#if ISP_ASIC_BUILD
uint32_t APICAL_READ_32(uint32_t addr);
uint16_t APICAL_READ_16(uint32_t addr);
uint8_t APICAL_READ_8(uint32_t addr);
void APICAL_WRITE_32(uint32_t addr, uint32_t data);
void APICAL_WRITE_16(uint32_t addr, uint16_t data);
void APICAL_WRITE_8(uint32_t addr, uint8_t data);
#else
#define APICAL_READ_32 system_isp_read_32
#define APICAL_READ_16 system_isp_read_16
#define APICAL_READ_8 system_isp_read_8
#define APICAL_WRITE_32 system_isp_write_32
#define APICAL_WRITE_16 system_isp_write_16
#define APICAL_WRITE_8 system_isp_write_8
#endif

#include "apical_configuration.h"
#endif /* __APICAL_ISP_IO_H__ */
