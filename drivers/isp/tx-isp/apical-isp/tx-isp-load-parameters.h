#ifndef __TX_ISP_LOAD_PARAMETERS_H__
#define __TX_ISP_LOAD_PARAMETERS_H__
#include <linux/fs.h>
#include <tx-isp-common.h>
#include <apical-isp/apical_calibrations_id.h>

#define TX_ISP_VERSION_SIZE 8
#define TX_ISP_VERSION_ID "1.38"
#define TX_ISP_PRIV_PARAM_FLAG_SIZE	8

enum __tx_isp_private_parameters_index {
	TX_ISP_PRIV_PARAM_BASE_INDEX,
	TX_ISP_PRIV_PARAM_CUSTOM_INDEX,
	TX_ISP_PRIV_PARAM_MAX_INDEX,
};
typedef struct __tx_isp_private_parameters_header{
	char flag[TX_ISP_PRIV_PARAM_FLAG_SIZE];
	unsigned int size;		/* the memory size of the parameter array */
	unsigned int crc;
} TXispPrivParamHeader;

enum __tx_isp_private_parameters_mode {
	TX_ISP_PRIV_PARAM_DAY_MODE,
	TX_ISP_PRIV_PARAM_NIGHT_MODE,
	TX_ISP_PRIV_PARAM_BUTT_MODE,
};

#define CONTRAST_CURVES_MAXNUM 10
typedef unsigned char contrast_curves[CONTRAST_CURVES_MAXNUM][2];

typedef struct __tx_isp_private_customer_paramters{
	union {
		struct {
			unsigned int 				: 2;
			unsigned int sensor_offset	: 1;
			unsigned int digital_gain	: 1;
			unsigned int gamma_fe		: 1;
			unsigned int raw_front		: 1;
			unsigned int defect_pixel	: 1;
			unsigned int frame_stitch	: 1;
			unsigned int gamma_fe_pos	: 1;
			unsigned int sinter		: 1;
			unsigned int temper		: 1;
			unsigned int order		: 1;
			unsigned int wb_module	: 1;
			unsigned int 			: 1;
			unsigned int mesh		: 1;
			unsigned int iridix		: 1;
			unsigned int 			: 1;
			unsigned int matrix		: 1;
			unsigned int fr_crop		: 1;
			unsigned int fr_gamma		: 1;
			unsigned int fr_sharpen		: 1;
			unsigned int 			: 3;
			unsigned int ds1_crop		: 1;
			unsigned int ds1_scaler		: 1;
			unsigned int ds1_gamma		: 1;
			unsigned int ds1_sharpen	: 1;
			unsigned int 			: 4;
		};
		unsigned int top;
	};
	/* green equalization */
	unsigned int ge_strength;
	unsigned int ge_threshold;
	unsigned int ge_slope;
	unsigned int ge_sensitivity;
	/* defect pixel correct configuration */
	unsigned int dp_module;
	unsigned int hpdev_threshold;
	unsigned int line_threshold;
	unsigned int hp_blend;
	/* demosaic configuration */
	unsigned int dmsc_vh_slope;
	unsigned int dmsc_aa_slope;
	unsigned int dmsc_va_slope;
	unsigned int dmsc_uu_slope;
	unsigned int dmsc_sat_slope;
	unsigned int dmsc_vh_threshold;
	unsigned int dmsc_aa_threshold;
	unsigned int dmsc_va_threshold;
	unsigned int dmsc_uu_threshold;
	unsigned int dmsc_sat_threshold;
	unsigned int dmsc_vh_offset;
	unsigned int dmsc_aa_offset;
	unsigned int dmsc_va_offset;
	unsigned int dmsc_uu_offset;
	unsigned int dmsc_sat_offset;
	unsigned int dmsc_luminance_thresh;
	unsigned int dmsc_np_offset;
	unsigned int dmsc_config;
	unsigned int dmsc_ac_threshold;
	unsigned int dmsc_ac_slope;
	unsigned int dmsc_ac_offset;
	unsigned int dmsc_fc_slope;
	unsigned int dmsc_fc_alias_slope;
	unsigned int dmsc_fc_alias_thresh;
	struct {
		unsigned int dmsc_np_off : 6;
		unsigned int dmsc_np_reflect : 1;
		unsigned int : 25;
	};
	/* Temper */
	unsigned int temper_recursion_limit;
	/* WDR configuration */
	unsigned int wdr_short_thresh;
	unsigned int wdr_long_thresh;
	unsigned int wdr_expo_ratio_thresh;
	unsigned int wdr_stitch_correct;
	unsigned int wdr_stitch_error_thresh;
	unsigned int wdr_stitch_error_limit;
	unsigned int wdr_stitch_bl_long;
	unsigned int wdr_stitch_bl_short;
	unsigned int wdr_stitch_bl_output;
	/* other configuration */
	unsigned int max_isp_dgain;
	unsigned int max_sensor_again;

	unsigned char sharpness;
	unsigned char saturation;
	unsigned char brightness;
	/* the parameters is contrast curve */
	contrast_curves contrast;
} TXispPrivCustomerParamer;

typedef struct __tx_isp_private_parameters_manage {
	char version[TX_ISP_VERSION_SIZE];
	TXispPrivParamHeader headers[TX_ISP_PRIV_PARAM_MAX_INDEX];
	void *data;								//the base address of all data.
	unsigned int data_size;
	void *fw_data;								//the base address of isp FW parameters.
	ApicalCalibrations isp_param[TX_ISP_PRIV_PARAM_BUTT_MODE];			//the struct of private0 manager.
	LookupTable param_table[_CALIBRATION_TOTAL_SIZE * TX_ISP_PRIV_PARAM_BUTT_MODE];
	void *base_buf;							//the address of private0 data.
	TXispPrivCustomerParamer *customer;				//the struct of private1 pointer.
	void *customer_buf;							//the address of private1 data.
} TXispPrivParamManage;

TXispPrivParamManage* load_tx_isp_parameters(struct tx_isp_sensor_attribute *attr);
void free_tx_isp_priv_param_manage(void);
#endif //__TX_ISP_LOAD_PARAMETERS_H__
