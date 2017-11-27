/*-----------------------------------------------------------------------------
  This confidential and proprietary software/information may be used only
  as authorized by a licensing agreement from Apical Limited

  (C) COPYRIGHT 2011 - 2015 Apical Limited
  ALL RIGHTS RESERVED

  The entire notice above must be reproduced on all authorized
  copies and copies may only be made to the extent permitted
  by a licensing agreement from Apical Limited.
  -----------------------------------------------------------------------------*/

#ifndef __SENSOR_INIT_H__
#define __SENSOR_INIT_H__

#include "apical_sbus.h"
#include <apical-isp/apical_configuration.h>

#if SENSOR_BINARY_SEQUENCE == 1
#define sensor_load_sequence sensor_load_binary_sequence
#else
#define sensor_load_sequence sensor_load_array_sequence
#endif

typedef struct sensor_reg_t {
	uint16_t address ;
	uint16_t value ;
} sensor_reg_t ;

void sensor_load_binary_sequence(apical_sbus_ptr_t p_sbus, const char *sequence, int group);
void sensor_load_array_sequence(apical_sbus_ptr_t p_sbus, const sensor_reg_t **sequence, int group);

#endif /* __SENSOR_INIT_H__ */
