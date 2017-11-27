/*-----------------------------------------------------------------------------
  This confidential and proprietary software/information may be used only
  as authorized by a licensing agreement from Apical Limited

  (C) COPYRIGHT 2011 - 2015 Apical Limited
  ALL RIGHTS RESERVED

  The entire notice above must be reproduced on all authorized
  copies and copies may only be made to the extent permitted
  by a licensing agreement from Apical Limited.
  -----------------------------------------------------------------------------*/

#ifndef __APICALDEFAULT_H__
#define __APICALDEFAULT_H__

//#include <stdint.h>
#include "apical_sbus.h"

typedef struct _sensor_ApicalDefault_iface_t
{
	apical_sbus_t mipi_i2c_bus;
} sensor_ApicalDefault_iface_t;
typedef sensor_ApicalDefault_iface_t *sensor_ApicalDefault_iface_ptr_t;

void reset_sensor_interface(sensor_ApicalDefault_iface_ptr_t p_iface);
void load_sensor_interface(sensor_ApicalDefault_iface_ptr_t p_iface, uint8_t mode);
void mipi_auto_tune(sensor_ApicalDefault_iface_ptr_t p_iface, uint8_t mode, uint32_t refw, uint32_t refh);

#endif /* __APICALDEFAULT_H__ */
