/*-----------------------------------------------------------------------------
  This confidential and proprietary software/information may be used only
  as authorized by a licensing agreement from Apical Limited

  (C) COPYRIGHT 2011 - 2015 Apical Limited
  ALL RIGHTS RESERVED

  The entire notice above must be reproduced on all authorized
  copies and copies may only be made to the extent permitted
  by a licensing agreement from Apical Limited.
  -----------------------------------------------------------------------------*/

#ifndef __APICAL_MODULATION_H__
#define __APICAL_MODULATION_H__

#include <apical-isp/apical.h>
#include <apical-isp/apical_types.h>


uint16_t calc_modulation_u16(uint16_t x,const modulation_entry_t *p_table,int table_len);
uint32_t calc_modulation_u32(uint32_t x,const modulation_entry_32_t *p_table,int table_len);

uint16_t calc_scaled_modulation_u16(uint16_t x,uint16_t target_min_y,uint16_t target_max_y,const modulation_entry_t *p_table,int table_len);

uint16_t calc_equidistant_modulation_u16(uint16_t x, const uint16_t *p_table, uint16_t table_len);
uint32_t calc_equidistant_modulation_u32(uint32_t x, const uint32_t *p_table, uint32_t table_len);
uint16_t calc_inv_equidistant_modulation_u16(uint16_t x, const uint16_t *p_table, uint16_t table_len);
uint32_t calc_inv_equidistant_modulation_u32(uint32_t x, const uint32_t *p_table, uint32_t table_len);

#endif /* __APICAL_MODULATION_H__ */
