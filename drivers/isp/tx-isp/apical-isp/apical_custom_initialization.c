/*-----------------------------------------------------------------------------
  This confidential and proprietary software/information may be used only
  as authorized by a licensing agreement from Apical Limited

  (C) COPYRIGHT 2011 - 2015 Apical Limited
  ALL RIGHTS RESERVED

  The entire notice above must be reproduced on all authorized
  copies and copies may only be made to the extent permitted
  by a licensing agreement from Apical Limited.
  -----------------------------------------------------------------------------*/

#include "apical_custom_initialization.h"
#include <apical-isp/apical_isp_config.h>
#include "tx-isp-load-parameters.h"
#include "apical_command_api.h"
#include "../tx-isp-debug.h"
#include "isp_config_seq.h"
#include "tx-isp-core.h"


static TXispPrivCustomerParamer *customer = NULL;

// this is a pointer to initialization sequence for isp
// it will be requested by firmware at initialization stage
// it is possible to return NULL pointer instead of valid sequence
// in this case no initialization will be made
static uint8_t p_isp_data[] = SENSOR_ISP_SEQUENCE_DEFAULT;

void apical_custom_initialization(void)
{
#if 1
	// this function is called in the end of
	// firmware initialization.
	// it can be used to make customer related
	// tuning of Apical ISP.
	apical_api_control_t api;
	unsigned int reason = 0;
	unsigned int status = 0;
	unsigned int tmp = 0;
	APICAL_WRITE_32(0x40, APICAL_ISP_TOP_CONTROL_LOW_REG_DEFAULT);
	APICAL_WRITE_32(0x44, APICAL_ISP_TOP_CONTROL_HIGH_REG_DEFAULT);
	if(customer){
		/* enable the isp modules */
		tmp = APICAL_READ_32(0x40);
		tmp = (tmp | 0x0c02da6c) & (~(customer->top));
		if(TX_ISP_EXIST_FR_CHANNEL == 0)
			tmp |= 0x00fc0000;
		APICAL_WRITE_32(0x40, tmp);
		/* if it is T20,the FR is corresponding to DS2 in bin file. */
		if ((customer->top) & (1 << 19)){
#if TX_ISP_EXIST_FR_CHANNEL
			apical_isp_top_bypass_fr_gamma_rgb_write(0);
			apical_isp_fr_gamma_rgb_enable_write(1);
#endif
#ifdef TX_ISP_EXIST_DS2_CHANNEL
			apical_isp_top_bypass_ds2_gamma_rgb_write(0);
			apical_isp_ds2_gamma_rgb_enable_write(1);
#endif
		} else {
#if TX_ISP_EXIST_FR_CHANNEL
			apical_isp_top_bypass_fr_gamma_rgb_write(1);
			apical_isp_fr_gamma_rgb_enable_write(0);
#endif
#ifdef TX_ISP_EXIST_DS2_CHANNEL
			apical_isp_top_bypass_ds2_gamma_rgb_write(1);
			apical_isp_ds2_gamma_rgb_enable_write(0);
#endif
		}

		if ((customer->top) & (1 << 20)){
#if TX_ISP_EXIST_FR_CHANNEL
			apical_isp_top_bypass_fr_sharpen_write(0);
			apical_isp_fr_sharpen_enable_write(1);
#endif
#if TX_ISP_EXIST_DS2_CHANNEL
			apical_isp_top_bypass_ds2_sharpen_write(0);
			apical_isp_ds2_sharpen_enable_write(1);
#endif
		} else {
#if TX_ISP_EXIST_FR_CHANNEL
			apical_isp_top_bypass_fr_sharpen_write(1);
			apical_isp_fr_sharpen_enable_write(0);
#endif
#ifdef TX_ISP_EXIST_DS2_CHANNEL
			apical_isp_top_bypass_ds2_sharpen_write(1);
			apical_isp_ds2_sharpen_enable_write(0);
#endif
		}
		if (customer->top & (1 << 27))
			apical_isp_ds1_sharpen_enable_write(1);
		else
			apical_isp_ds1_sharpen_enable_write(0);

		/* green equalization */
		apical_isp_raw_frontend_ge_strength_write(customer->ge_strength);
		apical_isp_raw_frontend_ge_threshold_write(customer->ge_threshold);
		apical_isp_raw_frontend_ge_slope_write(customer->ge_slope);
		apical_isp_raw_frontend_ge_sens_write(customer->ge_sensitivity);

		/* dpc configuration	 */
		apical_isp_raw_frontend_dp_enable_write(customer->dp_module);
		apical_isp_raw_frontend_hpdev_threshold_write(customer->hpdev_threshold);
		apical_isp_raw_frontend_line_thresh_write(customer->line_threshold);
		apical_isp_raw_frontend_hp_blend_write(customer->hp_blend);

		/* Demoasic */
		apical_isp_demosaic_vh_slope_write(customer->dmsc_vh_slope);
		apical_isp_demosaic_aa_slope_write(customer->dmsc_aa_slope);
		apical_isp_demosaic_va_slope_write(customer->dmsc_va_slope);
		apical_isp_demosaic_uu_slope_write(customer->dmsc_uu_slope);
		apical_isp_demosaic_sat_slope_write(customer->dmsc_sat_slope);
		apical_isp_demosaic_vh_thresh_write(customer->dmsc_vh_threshold);
		apical_isp_demosaic_aa_thresh_write(customer->dmsc_aa_threshold);
		apical_isp_demosaic_va_thresh_write(customer->dmsc_va_threshold);
		apical_isp_demosaic_uu_thresh_write(customer->dmsc_uu_threshold);
		apical_isp_demosaic_sat_thresh_write(customer->dmsc_sat_threshold);
		apical_isp_demosaic_vh_offset_write(customer->dmsc_vh_offset);
		apical_isp_demosaic_aa_offset_write(customer->dmsc_aa_offset);
		apical_isp_demosaic_va_offset_write(customer->dmsc_va_offset);
		apical_isp_demosaic_uu_offset_write(customer->dmsc_uu_offset);
		apical_isp_demosaic_sat_offset_write(customer->dmsc_sat_offset);
		apical_isp_demosaic_lum_thresh_write(customer->dmsc_luminance_thresh);
		apical_isp_demosaic_np_offset_write(customer->dmsc_np_offset);
		apical_isp_demosaic_dmsc_config_write(customer->dmsc_config);
		apical_isp_demosaic_ac_thresh_write(customer->dmsc_ac_threshold);
		apical_isp_demosaic_ac_slope_write(customer->dmsc_ac_slope);
		apical_isp_demosaic_ac_offset_write(customer->dmsc_ac_offset);
		apical_isp_demosaic_fc_slope_write(customer->dmsc_fc_slope);
		apical_isp_demosaic_fc_alias_slope_write(customer->dmsc_fc_alias_slope);
		apical_isp_demosaic_fc_alias_thresh_write(customer->dmsc_fc_alias_thresh);
		apical_isp_demosaic_np_off_write(customer->dmsc_np_off);
		apical_isp_demosaic_np_off_reflect_write(customer->dmsc_np_reflect);

		/* Temper */
		apical_isp_temper_recursion_limit_write(customer->temper_recursion_limit);
		/* FS WDR */
		apical_isp_frame_stitch_short_thresh_write(customer->wdr_short_thresh);
		apical_isp_frame_stitch_long_thresh_write(customer->wdr_long_thresh);
		apical_isp_frame_stitch_exposure_ratio_write(customer->wdr_expo_ratio_thresh);
		apical_isp_frame_stitch_stitch_correct_write(customer->wdr_stitch_correct);
		apical_isp_frame_stitch_stitch_error_thresh_write(customer->wdr_stitch_error_thresh);
		apical_isp_frame_stitch_stitch_error_limit_write(customer->wdr_stitch_error_limit);
		apical_isp_frame_stitch_black_level_out_write(customer->wdr_stitch_bl_output);
		apical_isp_frame_stitch_black_level_short_write(customer->wdr_stitch_bl_short);
		apical_isp_frame_stitch_black_level_long_write(customer->wdr_stitch_bl_long);

		/* Max ISP Digital Gain */
		api.type = TSYSTEM;
		api.dir = COMMAND_SET;
		api.value = customer->max_isp_dgain;
		api.id = SYSTEM_MAX_ISP_DIGITAL_GAIN;

		status = apical_command(api.type, api.id, api.value, api.dir, &reason);
		if(status != ISP_SUCCESS) {
			ISP_PRINT(ISP_WARNING_LEVEL,"Custom set max isp digital gain failure!reture value is %d,reason is %d\n",status,reason);
		}

		/* Max Sensor Analog Gain */
		api.type = TSYSTEM;
		api.dir = COMMAND_SET;
		api.value = customer->max_sensor_again;
		api.id = SYSTEM_MAX_SENSOR_ANALOG_GAIN;

		status = apical_command(api.type, api.id, api.value, api.dir, &reason);
		if(status != ISP_SUCCESS) {
			ISP_PRINT(ISP_WARNING_LEVEL,"Custom set max isp digital gain failure!reture value is %d,reason is %d\n",status,reason);
		}
	}
#if 0
	/* Anti Flicker  */
	api.type = TALGORITHMS;
	api.dir = COMMAND_SET;
	api.value = 50;
	api.id = ANTIFLICKER_MODE_ID;

	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	if(status != ISP_SUCCESS) {
		ISP_PRINT(ISP_WARNING_LEVEL,"Custom set anti flicker frequency failure!reture value is %d,reason is %d\n",status,reason);
	}

	api.type = TSYSTEM;
	api.dir = COMMAND_SET;
	api.value = 1;
	api.id = SYSTEM_ANTIFLICKER_ENABLE;

	status = apical_command(api.type, api.id, api.value, api.dir, &reason);
	if(status != ISP_SUCCESS) {
		ISP_PRINT(ISP_WARNING_LEVEL,"Custom enable anti-flicker failure!reture value is %d,reason is %d\n",status,reason);
	}
#endif

#else
	int index = 0;
	TXispPrivCustomParamType *ptr = custom.ptr;
	for(index = 0; index < custom.rows; index++){
		APICAL_WRITE_32(ptr[index].reg, ptr[index].value);
	}
#endif
}

void init_tx_isp_customer_parameter(TXispPrivCustomerParamer *m)
{
	customer = m;
	return;
}

uint8_t* apical_custom_sequence(void) {
	return p_isp_data ;
}
