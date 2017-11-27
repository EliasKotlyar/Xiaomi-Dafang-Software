#if !defined(__SYSTEM_IO_H__)
#define __SYSTEM_IO_H__

#include "apical_types.h"

void system_isp_set_base_address(void *address);
//------------------------------------------------------
uint32_t system_isp_read_32(uint32_t addr);
uint16_t system_isp_read_16(uint32_t addr);
uint8_t  system_isp_read_8(uint32_t addr);
void system_isp_write_32(uint32_t addr, uint32_t data);
void system_isp_write_16(uint32_t addr, uint16_t data);
void system_isp_write_8( uint32_t addr, uint8_t  data);
//------------------------------------------------------

#endif /* __SYSTEM_IO_H__ */
