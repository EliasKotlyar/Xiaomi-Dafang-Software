/*-----------------------------------------------------------------------------
  This confidential and proprietary software/information may be used only
  as authorized by a licensing agreement from Apical Limited

  (C) COPYRIGHT 2011 - 2015 Apical Limited
  ALL RIGHTS RESERVED

  The entire notice above must be reproduced on all authorized
  copies and copies may only be made to the extent permitted
  by a licensing agreement from Apical Limited.
  -----------------------------------------------------------------------------*/

#ifndef __SENSOR_DRV_H__
#define __SENSOR_DRV_H__

#include <apical-isp/apical_types.h>
#include <apical-isp/apical_firmware_config.h>
#include <apical-isp/apical_sensor_config.h>
#include <apical-isp/apical_interface_config.h>
#include "apical_sbus.h"
#include "apical_sbus_i2c.h"
#include "apical_sbus_spi.h"
#include "tx-isp-core.h"

typedef struct _sensor_context_t
{
	uint16_t again;
	uint16_t dgain;
	uint8_t n_context;
	uint8_t wdr_mode;
	uint16_t again_x2;
	uint16_t dgain_coarse;
	uint16_t dgain_fine;
	uint8_t column_buffer_gain_index;
} sensor_context_t;

typedef struct _image_resolution_t
{
	uint16_t width;
	uint16_t height;
} image_resolution_t;

typedef struct _sensor_param_t
{
	uint8_t mode;
	image_resolution_t total;
	image_resolution_t active;
	sensor_context_t sensor_ctx;
	int32_t again_log2_max;
	int32_t dgain_log2_max;
	uint32_t integration_time_min;
	uint32_t integration_time_max;
	uint32_t integration_time_long_max;
	uint32_t integration_time_limit;
	uint16_t day_light_integration_time_max;
	uint8_t integration_time_apply_delay;
	uint8_t analog_gain_apply_delay;
	uint8_t digital_gain_apply_delay;
	int32_t xoffset;
	int32_t yoffset;
	int32_t anti_flicker_pos;
	uint32_t lines_per_second;
} sensor_param_t;
typedef struct _sensor_control_t
{
	apical_sbus_t sbus;
	sensor_param_t param;
	void (*hw_reset_disable)(void);
	void (*hw_reset_enable)(void);
	int32_t (*alloc_analog_gain)(int32_t gain, sensor_context_t *p_ctx);
	int32_t (*alloc_digital_gain)(int32_t gain, sensor_context_t *p_ctx);

	void (*alloc_integration_time)(uint16_t *int_time
#if SENSOR_EXP_NUMBER >= 3
			,uint16_t* int_time_M
#endif
#if SENSOR_EXP_NUMBER >= 2
			,uint16_t* int_time_L
#endif
			,sensor_context_t *p_ctx);

	void (*set_integration_time)(apical_sbus_ptr_t, uint16_t int_time
#if SENSOR_EXP_NUMBER >= 3
			,uint16_t int_time_M
#endif
#if SENSOR_EXP_NUMBER >= 2
			,uint16_t int_time_L
#endif
			,sensor_param_t *param);

	void (*start_changes)(apical_sbus_ptr_t, sensor_context_t *p_ctx);
	void (*end_changes)(apical_sbus_ptr_t, sensor_context_t *p_ctx);
	void (*set_analog_gain)(apical_sbus_ptr_t, uint32_t again_reg_val, sensor_context_t *p_ctx);
	void (*set_digital_gain)(apical_sbus_ptr_t, uint32_t dgain_reg_val, sensor_context_t *p_ctx);
#if SENSOR_CROP_OFFSET
	uint32_t (*set_xoffset)(apical_sbus_ptr_t, uint32_t xoffset, sensor_context_t *p_ctx);
	uint32_t (*set_yoffset)(apical_sbus_ptr_t, uint32_t yoffset, sensor_context_t *p_ctx);
#endif
	uint16_t (*get_normal_fps)(sensor_param_t *param);
	uint16_t (*read_black_pedestal)(apical_sbus_ptr_t, int i, uint32_t gain);
	void (*set_mode)(apical_sbus_ptr_t, uint8_t mode, sensor_param_t *param);
	void (*set_wdr_mode)(apical_sbus_ptr_t, uint8_t wdr, sensor_param_t *param);
	uint8_t (*fps_control)(apical_sbus_ptr_t, uint8_t fps, sensor_param_t *param);
	uint16_t (*get_id)(apical_sbus_ptr_t);
	void (*disable_isp)(apical_sbus_ptr_t);
	uint32_t (*get_lines_per_second)(apical_sbus_ptr_t, sensor_param_t *param);
} sensor_control_t;

typedef sensor_control_t *sensor_control_ptr_t;

void sensor_init(sensor_control_ptr_t);
int apical_sensor_early_init(struct tx_isp_core_device *core);

#endif /* __SENSOR_DRV_H__ */
