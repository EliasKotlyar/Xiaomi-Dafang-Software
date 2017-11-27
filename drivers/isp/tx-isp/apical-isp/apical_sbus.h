/*-----------------------------------------------------------------------------
  This confidential and proprietary software/information may be used only
  as authorized by a licensing agreement from Apical Limited

  (C) COPYRIGHT 2011 - 2015 Apical Limited
  ALL RIGHTS RESERVED

  The entire notice above must be reproduced on all authorized
  copies and copies may only be made to the extent permitted
  by a licensing agreement from Apical Limited.
  -----------------------------------------------------------------------------*/

#ifndef __APICAL_SBUS_H__
#define __APICAL_SBUS_H__

#include <sensor-common.h>
#include <apical-isp/apical.h>
#include <apical-isp/apical_interface_config.h>

typedef struct _apical_sbus_t apical_sbus_t;
typedef struct _apical_sbus_t *apical_sbus_ptr_t;
typedef const struct _apical_sbus_t *apical_sbus_const_ptr_t;

struct _apical_sbus_t
{
	uint32_t mask;
	uint8_t device;
	uint32_t control;
	void *p_control;
	uint32_t (*read_sample)(apical_sbus_ptr_t p_bus, uint32_t addr, uint8_t sample_size);
	void (*write_sample)(apical_sbus_ptr_t p_bus, uint32_t addr, uint32_t sample, uint8_t sample_size);
};
uint8_t apical_sbus_read_u8(apical_sbus_ptr_t p_bus, uint32_t addr);
uint16_t apical_sbus_read_u16(apical_sbus_ptr_t p_bus, uint32_t addr);
uint32_t apical_sbus_read_u32(apical_sbus_ptr_t p_bus, uint32_t addr);
void apical_sbus_read_data_u8(apical_sbus_ptr_t p_bus, uint32_t addr, uint8_t *p_data, int n_count);
void apical_sbus_read_data_u16(apical_sbus_ptr_t p_bus, uint32_t addr, uint16_t *p_data, int n_count);
void apical_sbus_read_data_u32(apical_sbus_ptr_t p_bus, uint32_t addr, uint32_t *p_data, int n_count);
void apical_sbus_write_u8(apical_sbus_ptr_t p_bus, uint32_t addr, uint8_t sample);
void apical_sbus_write_u16(apical_sbus_ptr_t p_bus, uint32_t addr, uint16_t sample);
void apical_sbus_write_u32(apical_sbus_ptr_t p_bus, uint32_t addr, uint32_t sample);
void apical_sbus_write_data_u8(apical_sbus_ptr_t p_bus, uint32_t addr, uint8_t *p_data, int n_count);
void apical_sbus_write_data_u16(apical_sbus_ptr_t p_bus, uint32_t addr, uint16_t *p_data, int n_count);
void apical_sbus_write_data_u32(apical_sbus_ptr_t p_bus, uint32_t addr, uint32_t *p_data, int n_count);
void apical_sbus_write_data(apical_sbus_ptr_t p_bus, uint32_t addr, void *p_data, int n_size);
void apical_sbus_copy(apical_sbus_t *p_bus_to, uint32_t addr_to, apical_sbus_t *p_bus_from, uint32_t addr_from, int n_size);

#include "apical_sbus_i2c.h"
#include "apical_sbus_spi.h"

#endif /* __APICAL_SBUS_H__ */
