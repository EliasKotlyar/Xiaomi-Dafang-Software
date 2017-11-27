/*-----------------------------------------------------------------------------
  This confidential and proprietary software/information may be used only
  as authorized by a licensing agreement from Apical Limited

  (C) COPYRIGHT 2011 - 2015 Apical Limited
  ALL RIGHTS RESERVED

  The entire notice above must be reproduced on all authorized
  copies and copies may only be made to the extent permitted
  by a licensing agreement from Apical Limited.
  -----------------------------------------------------------------------------*/

#ifndef __APICAL_H__
#define __APICAL_H__


#include "apical_types.h"
#include "apical_firmware_config.h"
#include "apical_sensor_config.h"
#include "apical_calibrations.h"

typedef struct __apical_isp_register_value{
	unsigned int addr;
	unsigned int value;
} apical_isp_regval_t;

#define ISP_MODULE_BYPASS_ENABLE 1
#define ISP_MODULE_BYPASS_DISABLE 0
typedef union _apical_isp_top_contrl{
	struct bitmap {
		unsigned int test_gen		: 1;
		unsigned int mirror		: 1;
		unsigned int sensor_offset	: 1;
		unsigned int dig_gain		: 1;
		unsigned int gamma_fe		: 1;
		unsigned int raw_front		: 1;
		unsigned int defect_pixel	: 1;
		unsigned int frame_stitch	: 1;
		unsigned int gamma_fe_pos	: 1;
		unsigned int sinter		: 1;
		unsigned int temper		: 1;
		unsigned int order		: 1;
		unsigned int wb			: 1;
		unsigned int radial		: 1;
		unsigned int mesh		: 1;
		unsigned int iridix		: 1;
		unsigned int demosaic		: 1;
		unsigned int matrix		: 1;
		unsigned int fr_crop		: 1;
		unsigned int fr_gamma		: 1;
		unsigned int fr_sharpen		: 1;
		unsigned int fr_logo		: 1;
		unsigned int fr_csc		: 1;
		unsigned int 			: 1;
		unsigned int ds1_crop		: 1;
		unsigned int ds1_scaler		: 1;
		unsigned int ds1_gamma		: 1;
		unsigned int ds1_sharpen	: 1;
		unsigned int ds1_logo		: 1;
		unsigned int ds1_csc		: 1;
		unsigned int ds1_dma		: 1;
		unsigned int 			: 1;
		unsigned int ds2_scaler_source	: 1;
		unsigned int ds2_crop		: 1;
		unsigned int ds2_scaler		: 1;
		unsigned int ds2_gamma		: 1;
		unsigned int ds2_sharpen	: 1;
		unsigned int ds2_logo		: 1;
		unsigned int ds2_csc		: 1;
		unsigned int 			: 2;
		unsigned int raw_bypass		: 1;
		unsigned int debug		: 2;
		unsigned int 			: 1;
		unsigned int proc_bypass_mode	: 2;
		unsigned int  			: 17;
	} bits;
	struct topregs {
		unsigned int low;
		unsigned int high;
	} reg;
} apical_isp_top_ctl_t;

enum apical_isp_top_ctl_bit {
	TEST_GEN_BIT,
	MIRROR_BIT,
	SENSOR_OFFSET_BIT,
	DIG_GAIN_BIT,
	GAMMA_FE_BIT,
	RAW_FRONT_BIT,
	DEFECT_PIXEL_BIT,
	FRAME_STITCH_BIT,
	GAMMA_FE_POS_BIT,
	SINTER_BIT,
	TEMPER_BIT,
	ORDER_BIT,
	WB_BIT,
	RADIAL_BIT,
	MESH_BIT,
	IRIDIX_BIT,
	DEMOSAIC_BIT,
	MATRIX_BIT,
	FR_CROP_BIT,
	FR_GAMMA_BIT,
	FR_SHARPEN_BIT,
	FR_LOGO_BIT,
	FR_CSC_BIT,

	DS1_CROP_BIT,
	DS1_SCALER_BIT,
	DS1_GAMMA_BIT,
	DS1_SHARPEN_BIT,
	DS1_LOGO_BIT,
	DS1_CSC_BIT,
	DS1_DMA_BIT,

	DS2_SCALER_SOURCE_BIT,
	DS2_CROP_BIT,
	DS2_SCALER_BIT,
	DS2_GAMMA_BIT,
	DS2_SHARPEN_BIT,
	DS2_LOGO_BIT,
	DS2_CSC_BIT,

	RAW_BYPASS_BIT,
	DEBUG_BIT,

	PROC_BYPASS_MODE_BIT,
};



typedef struct _metadata_t
{
	uint8_t format;
	uint16_t width;
	uint16_t height;
	uint16_t line_size;
	uint32_t exposure;
	uint32_t int_time;
	uint32_t again;
	uint32_t dgain;
	uint32_t isp_dgain;
	int8_t dis_offset_x;
	int8_t dis_offset_y;
} metadata_t;

typedef struct _sytem_tab{
	uint8_t global_freeze_firmware ;
	uint8_t  global_manual_exposure ;
	uint8_t  global_manual_exposure_ratio ;
	uint8_t  global_manual_integration_time ;
	uint8_t  global_manual_sensor_analog_gain ;
	uint8_t  global_manual_sensor_digital_gain ;
	uint8_t  global_manual_isp_digital_gain ;
	uint8_t  global_manual_directional_sharpening ;
	uint8_t  global_manual_un_directional_sharpening ;
	uint8_t  global_manual_iridix ;
	uint8_t  global_manual_sinter ;
	uint8_t  global_manual_temper ;
	uint8_t  global_manual_awb ;
	uint8_t  global_antiflicker_enable ;
	uint8_t  global_slow_frame_rate_enable ;
	uint8_t  global_manual_saturation ;
	uint32_t  global_manual_exposure_time;
	uint8_t  global_exposure_dark_target;
	uint8_t  global_exposure_bright_target;
	uint8_t  global_exposure_ratio;
	uint8_t  global_max_exposure_ratio;
	uint16_t  global_integration_time;
	uint16_t  global_max_integration_time;
	uint8_t  global_sensor_analog_gain;
	uint8_t  global_max_sensor_analog_gain;
	uint8_t  global_sensor_digital_gain;
	uint8_t  global_max_sensor_digital_gain;
	uint8_t  global_isp_digital_gain;
	uint8_t  global_max_isp_digital_gain;
	uint8_t  global_directional_sharpening_target;
	uint8_t  global_maximum_directional_sharpening;
	uint8_t  global_minimum_directional_sharpening;
	uint8_t  global_un_directional_sharpening_target;
	uint8_t  global_maximum_un_directional_sharpening;
	uint8_t  global_minimum_un_directional_sharpening;
	uint8_t  global_iridix_strength_target;
	uint8_t  global_maximum_iridix_strength;
	uint8_t  global_minimum_iridix_strength;
	uint8_t  global_sinter_threshold_target;
	uint8_t  global_maximum_sinter_strength;
	uint8_t  global_minimum_sinter_strength;
	uint8_t  global_temper_threshold_target;
	uint8_t  global_maximum_temper_strength;
	uint8_t  global_minimum_temper_strength;
	uint8_t  global_awb_red_gain;
	uint8_t  global_awb_blue_gain;
	uint8_t  global_saturation_target;
	uint8_t  global_anti_flicker_frequency ;
	uint8_t  global_ae_compensation;
	uint8_t  global_calibrate_bad_pixels ;
	uint16_t  global_dis_x ;
	uint16_t  global_dis_y ;
} system_tab;

extern system_tab stab ;
typedef void (*buffer_callback_t)(uint32_t addr, const metadata_t *metadata);

#define DMA_FORMAT_DISABLE 0
#define DMA_FORMAT_RGB32 1
#define DMA_FORMAT_A2R10G10B10 2
#define DMA_FORMAT_RGB565 3
#define DMA_FORMAT_RGB24 4
#define DMA_FORMAT_RAW16 6
#define DMA_FORMAT_RAW12 7
#define DMA_FORMAT_AYUV 8
#define DMA_FORMAT_Y410 9
#define DMA_FORMAT_YUY2 10
#define DMA_FORMAT_UYVY 11
#define DMA_FORMAT_Y210 12
#define DMA_FORMAT_NV12_Y (13|0x000)
#define DMA_FORMAT_NV12_UV (13|0x40)
#define DMA_FORMAT_NV12_VU (13|0x80)
#define DMA_FORMAT_YV12_Y (14|0x000)
#define DMA_FORMAT_YV12_U (14|0x100)
#define DMA_FORMAT_YV12_V (14|0x200)

void apical_init(void);

void apical_init_calibrations( uint32_t mode ) ;

void apical_process(void);

#if ISP_HAS_TEMPER
void apical_frame_buffer_configure_temper(uint32_t buf_base_addr);
#endif

void apical_frame_buffer_configure_all(uint32_t start_addr, uint32_t fr_buf_size, uint16_t fr_format, buffer_callback_t fr_callback
#if ISP_HAS_DOWNSCALER || ISP_HAS_DOUBLE_DS
		,uint32_t ds_buf_size
		,uint16_t ds_format
		,buffer_callback_t ds_callback
#endif
#if ISP_HAS_DOUBLE_DS
		,uint32_t ds2_buf_size
		,uint16_t ds2_format
		,buffer_callback_t ds2_callback
#endif

		);

void apical_change_resolution(uint32_t exposure_correction);
#if ISP_HAS_CONNECTION_SOCKET
/* please call these routines from the platform specific code to setup socket routines */
struct apical_socket_f *apical_socket_f_impl(void);
void apical_socket_set_f_impl(struct apical_socket_f *f);
#endif // ISP_HAS_CONNECTION_SOCKET
#define ISP_HAS_STREAM_CONNECTION (ISP_HAS_CONNECTION_SOCKET || ISP_HAS_CONNECTION_BUFFER || ISP_HAS_CONNECTION_UART || ISP_HAS_CONNECTION_CHARDEV)


#define BUS_ERROR_RESET	-1
#define BUS_ERROR_FATAL	-2

void apical_connection_init(void);
void apical_connection_process(void);
void apical_connection_destroy(void);


#endif /* __APICAL_H__ */
