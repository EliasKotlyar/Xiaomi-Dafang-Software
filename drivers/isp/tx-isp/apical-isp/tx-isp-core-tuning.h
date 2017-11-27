#ifndef __TX_ISP_CORE_TUNING_H__
#define __TX_ISP_CORE_TUNING_H__

#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <tx-isp-common.h>
#include "tx-isp-core.h"

#define	WEIGHT_ZONE_NUM (15*15)

typedef enum isp_core_module_ops_mode {
	ISPCORE_MODULE_DISABLE,
	ISPCORE_MODULE_ENABLE,
	ISPCORE_MODULE_BUTT,			/**< 用于判断参数的有效性，参数大小必须小于这个值 */
} ISPMODULE_OPS_MODE_E;

typedef enum isp_core_module_ops_type {
	ISPCORE_MODULE_AUTO,
	ISPCORE_MODULE_MANUAL,
} ISPMODULE_OPS_TYPE_E;
/*
 * select which zones are used to gather isp's statistics.
 * the region of interest is defined as rectangle with top-left coordinates(startx, starty)
 * and bottom-right coordinates(endx, endy).
 */
union isp_core_zone_select{
	struct {
		unsigned int startx :8;
		unsigned int starty :8;
		unsigned int endx   :8;
		unsigned int endy   :8;
	};
	unsigned int val;
};

struct isp_core_awb_attr{
	union isp_core_zone_select zone_sel;
	unsigned short low_color_temp;
	unsigned short high_color_temp;
	unsigned char weight[WEIGHT_ZONE_NUM];
};

struct isp_core_mwb_attr{
	unsigned short red_gain;
	unsigned short blue_gain;
};

struct isp_core_wb_zone_info{
	unsigned short red_green;
	unsigned short blue_green;
	unsigned int sum;
};

typedef enum isp_core_wb_statistic_mode {
	ISPCORE_WB_STA_GR_GB,
	ISPCORE_WB_STA_RG_BG,
} ISPCORE_WB_STA_MODE_E;

struct isp_core_wb_statistic_info{
	ISPCORE_WB_STA_MODE_E sta_mode;
	unsigned short gr_gain;
	unsigned short gb_gain;
	unsigned short awb_sum;
	unsigned short cb_max;
	unsigned short cb_min;
	unsigned short cr_max;
	unsigned short cr_min;
	unsigned short white_level;
	unsigned short black_level;
	struct isp_core_wb_zone_info zonesta[WEIGHT_ZONE_NUM];
};

struct isp_core_ae_attr {
	unsigned short gain;
	unsigned short comp;
	unsigned char exp_hist_thresh[4];
	union isp_core_zone_select zone_sel;
	unsigned char weight[WEIGHT_ZONE_NUM];
};

struct isp_core_ae_statistic_info {
	unsigned char exp_statistic[WEIGHT_ZONE_NUM][5];
	unsigned char exp_hist_256value[256];
	unsigned char exp_hist_5value[5];
	unsigned char ave_lum;
};

struct isp_core_af_attr {
	unsigned short af_threshold;
	unsigned short af_threshold_alt;
	unsigned char af_metrics_shift;
	unsigned char  af_low_range;
	unsigned char  af_high_range;
	unsigned char  af_intensity_mode;
	unsigned char  af_np_offset;
	union isp_core_zone_select zone_sel;
};

struct isp_core_af_statistic_info {
	unsigned short af_metrics;
	unsigned short af_metrics_alt;
	unsigned short af_threshold;
	unsigned short af_intensity;
	unsigned short af_intensity_zone;
	unsigned char  af_intensity_mode;
	unsigned char  af_np_offset;
};

/* the rule of elements is rr, rg, rb, gr, gg, gb, br, bg, bb */
struct isp_core_ccm_attr {
	unsigned short high_ccm_linear[9];
	unsigned short middle_ccm_linear[9];
	unsigned short low_ccm_linear[9];
	unsigned short high_ccm_hdr[9];
	unsigned short middle_ccm_hdr[9];
	unsigned short low_ccm_hdr[9];
};

typedef struct isp_core_ccm_saturation_attr {
	unsigned char targer_sat;
	unsigned char sat_linear[8];
	unsigned char sat_hdr[8];
} ISPCORE_CCM_SAT_S;

/* demosaic module */
struct isp_core_false_color_attr {
	unsigned char strength;
	unsigned char alias_strength;
	unsigned char alias_thresh;
};

typedef enum imp_isp_tuning_modules_ops_mode {
	IMPISP_TUNING_OPS_MODE_DISABLE,			/**< 不使能该模块功能 */
	IMPISP_TUNING_OPS_MODE_ENABLE,			/**< 使能该模块功能 */
	IMPISP_TUNING_OPS_MODE_BUTT,			/**< 用于判断参数的有效性，参数大小必须小于这个值 */
} IMPISP_TUNING_OPS_MODE_E;

struct isp_core_sharpness_attr {
#if 1
	IMPISP_TUNING_OPS_MODE_E enable;
	unsigned char target_sharp;
	unsigned char sharp_d[4];
	unsigned char sharp_ud[4];
#else
	unsigned char demo_strength_d;
	unsigned char demo_strength_ud;
	unsigned short demo_threshold;
	unsigned char fr_enable;
	unsigned char fr_strength;
	unsigned char ds1_enable;
	unsigned char ds1_strength;
	unsigned char ds2_enable;
	unsigned char ds2_strength;
#endif
};

struct isp_core_demosaic_attr {
	unsigned char vh_slope;
	unsigned char aa_slope;
	unsigned char va_slope;
	unsigned char uu_slope;
	unsigned char sat_slope;
	unsigned char dmsc_config;
	unsigned short vh_thresh;
	unsigned short aa_thresh;
	unsigned short va_thresh;
	unsigned short uu_thresh;
	unsigned short sat_thresh;
	unsigned short vh_offset;
	unsigned short aa_offset;
	unsigned short va_offset;
	unsigned short uu_offset;
	unsigned short sat_offset;
};


#define ISP_RAW_GAMMA_FE0_SIZE 33
#define ISP_RAW_GAMMA_FE1_SIZE 257
typedef struct isp_core_raw_gamma_attr {
	ISPMODULE_OPS_MODE_E mode;
	unsigned short gamma_fe_0[ISP_RAW_GAMMA_FE0_SIZE];
	unsigned short gamma_fe_1[ISP_RAW_GAMMA_FE1_SIZE];
} ISPCORE_RAW_GAMMA_ATTR_S;


#define ISP_RGB_GAMMA_SIZE 129
typedef struct isp_core_rgb_gamma_attr {
	ISPMODULE_OPS_MODE_E mode;
	unsigned short gamma_linear[ISP_RGB_GAMMA_SIZE];
	unsigned short gamma_hdr[ISP_RGB_GAMMA_SIZE];
} ISPCORE_RGB_GAMMMA_ATTR_S;

typedef enum {
	ISPMODULE_DRC_MANUAL,
	ISPMODULE_DRC_UNLIMIT,
	ISPMODULE_DRC_HIGH,
	ISPMODULE_DRC_MEDIUM,
	ISPMODULE_DRC_LOW,
	ISPMODULE_DRC_DISABLE,
} ISPMODULE_DRC_MODE;

/* iridix */
struct isp_core_drc_attr {
	ISPMODULE_DRC_MODE mode;
	unsigned char strength;
	unsigned char slope_max;
	unsigned char slope_min;
	unsigned short black_level;
	unsigned short white_level;
};

enum isp_core_mesh_shading_scale {
	ISP_MESH_SHADING_0_2 = 0x00,
	ISP_MESH_SHADING_0_4,
	ISP_MESH_SHADING_0_8,
	ISP_MESH_SHADING_0_16,
	ISP_MESH_SHADING_1_2,
	ISP_MESH_SHADING_1_3,
	ISP_MESH_SHADING_1_5,
	ISP_MESH_SHADING_1_9,
	ISP_MESH_SHADING_BUTT,
};

enum isp_core_mesh_shading_tab_format {
	ISP_MESH_SHADING_TAB_1 = 0x00,
	ISP_MESH_SHADING_TAB_2,
	ISP_MESH_SHADING_TAB_4,
	ISP_MESH_SHADING_TAB_BUTT,
};

typedef enum isp_core_shading_page_index {
	ISPCORE_SHADING_PAGE_0,
	ISPCORE_SHADING_PAGE_1,
	ISPCORE_SHADING_PAGE_2,
} ISPCORE_SHADING_PAGE_INDEX_E;

union isp_core_mesh_shading_tab {
	unsigned char mesh_tab_1[64*64];
	struct mesh_table_2 {
		unsigned char ls0;
		unsigned char ls1;
	} tab_2[32*64];
	struct mesh_table_4 {
		unsigned char ls0;
		unsigned char ls1;
		unsigned char ls2;
		unsigned char ls3;
	} tab_3[32*32];
	unsigned int regs[1024];
};

enum isp_core_mesh_sources_blend {
	/* enum = LBa and LBb */
	ISP_MESH_S_LS0_LS1,
	ISP_MESH_S_LS1_LS2,
	ISP_MESH_S_LS2_LS3,
	ISP_MESH_S_LS3_LS0,
	ISP_MESH_S_LS0_LS2,
	ISP_MESH_S_LS1_LS3,
};
/*
@ mesh_scale :select format of coefficient
@ mesh_page_x :select page of mesh tables
@ mesh_bank_x :select light sources blended
@ mesh_alpha_bank_x : the alpha of blending different light sources.
	final blend coefficient = (a * LSb coefficient + (255 - a) * LSa cofficient) / 255
*/
struct isp_core_shading_attr {
	ISPMODULE_OPS_MODE_E mode;
	enum isp_core_mesh_shading_scale mesh_scale;
	enum isp_core_mesh_shading_tab_format	mesh_mode;
	ISPCORE_SHADING_PAGE_INDEX_E r_page;
	ISPCORE_SHADING_PAGE_INDEX_E g_page;
	ISPCORE_SHADING_PAGE_INDEX_E b_page;
	enum isp_core_mesh_sources_blend mesh_alpha_bank_r;
	enum isp_core_mesh_sources_blend mesh_alpha_bank_g;
	enum isp_core_mesh_sources_blend mesh_alpha_bank_b;
	unsigned char mesh_alpha_r;
	unsigned char mesh_alpha_g;
	unsigned char mesh_alpha_b;
	unsigned char update_page;
	union isp_core_mesh_shading_tab page0;
	union isp_core_mesh_shading_tab page1;
	union isp_core_mesh_shading_tab page2;
};


/* green equalization */
struct isp_core_green_eq_attr {
	ISPMODULE_OPS_MODE_E mode;
	unsigned char strength;
	unsigned char threshold;
	unsigned char slope;
	unsigned char sensitivity;
};

struct isp_core_dynamic_defect_pixel_attr {
	ISPMODULE_OPS_MODE_E mode;
	ISPMODULE_OPS_MODE_E dark_pixels;
	ISPMODULE_OPS_MODE_E bright_pixels;
	unsigned char d_threshold;
	unsigned char d_slope;
	unsigned short hpdev_thresh;
	unsigned short line_thresh;
	unsigned char hp_blend;
};

struct isp_core_static_defect_pixel_attr {
	ISPMODULE_OPS_MODE_E mode;
#if 0
	unsigned char show_reference;
	unsigned char show_defect_pixels;
	unsigned char detect_trigger;
	unsigned char threshold;
	unsigned char slope;
	unsigned short bad_pixel_count;
	unsigned int bad_pixel_table[1024];
#endif
};

struct isp_core_sinter_attr{
	ISPMODULE_OPS_MODE_E mode;
	ISPMODULE_OPS_TYPE_E type;
#if 1
	unsigned char manual_strength;
#else
	unsigned char thresh_short;
	unsigned char thresh_long;
	unsigned char v_thresh[2];
	unsigned char h_thresh[2];
	unsigned char strength[2];
	unsigned char rm_enable;
	unsigned short rm_center_x;
	unsigned short rm_center_y;
	unsigned short rm_center_mult;
#endif
};

typedef enum isp_core_temper_mode_enum{
	ISPCORE_TEMPER_MODE_DISABLE,
	ISPCORE_TEMPER_MODE_AUTO,
	ISPCORE_TEMPER_MODE_MANUAL,
} ISP_CORE_TEMPER_MODE;

struct isp_core_temper_attr{
	/* ISP_CORE_TEMPER_MODE mode; */
#if 1
	unsigned char manual_strength;
#else
	unsigned char thresh_short;
	unsigned char thresh_long;
	unsigned char recursion;
#endif
};

struct isp_core_wdr_attr {
	unsigned char stitch_correct;
	unsigned short short_thresh;
	unsigned short long_thresh;
	unsigned short exp_ratio;
	unsigned short stitch_err_thresh;
	unsigned short stitch_err_limit;
	unsigned short black_level_long;
	unsigned short black_level_short;
	unsigned short black_level_out;
};

/* noise profile raw fronted */
#define ISPCORE_NOISE_PROFILE_SIZE 128
struct isp_core_noise_profile_attr{
	unsigned short exp_thresh;
	unsigned char short_ratio;
	unsigned char long_ratio;
	unsigned char noise_profile_offet;
	unsigned char noise_profile_reflect;
	unsigned char noise_pfile_linear[ISPCORE_NOISE_PROFILE_SIZE];
	unsigned char noise_pfile_hdr[ISPCORE_NOISE_PROFILE_SIZE];
};

struct isp_core_gamma_attr{
	unsigned short gamma[129];
};

typedef struct _system_tab_ctrl{
	bool ctrl_global_freeze_firmware ;
	bool ctrl_global_manual_exposure ;
	bool ctrl_global_manual_exposure_ratio ;
	bool ctrl_global_manual_integration_time ;
	bool ctrl_global_manual_sensor_analog_gain ;
	bool ctrl_global_manual_sensor_digital_gain ;
	bool ctrl_global_manual_isp_digital_gain ;
	bool ctrl_global_manual_directional_sharpening ;
	bool ctrl_global_manual_un_directional_sharpening ;
	bool ctrl_global_manual_iridix ;
	bool ctrl_global_manual_sinter ;
	bool ctrl_global_manual_temper ;
	bool ctrl_global_manual_awb ;
	bool ctrl_global_antiflicker_enable ;
	bool ctrl_global_slow_frame_rate_enable ;
	bool ctrl_global_manual_saturation ;
	bool ctrl_global_manual_exposure_time;
	bool ctrl_global_exposure_dark_target;
	bool ctrl_global_exposure_bright_target;
	bool ctrl_global_exposure_ratio;
	bool ctrl_global_max_exposure_ratio;
	bool ctrl_global_integration_time;
	bool ctrl_global_max_integration_time;
	bool ctrl_global_sensor_analog_gain;
	bool ctrl_global_max_sensor_analog_gain;
	bool ctrl_global_sensor_digital_gain;
	bool ctrl_global_max_sensor_digital_gain;
	bool ctrl_global_isp_digital_gain;
	bool ctrl_global_max_isp_digital_gain;
	bool ctrl_global_directional_sharpening_target;
	bool ctrl_global_maximum_directional_sharpening;
	bool ctrl_global_minimum_directional_sharpening;
	bool ctrl_global_un_directional_sharpening_target;
	bool ctrl_global_maximum_un_directional_sharpening;
	bool ctrl_global_minimum_un_directional_sharpening;
	bool ctrl_global_iridix_strength_target;
	bool ctrl_global_maximum_iridix_strength;
	bool ctrl_global_minimum_iridix_strength;
	bool ctrl_global_sinter_threshold_target;
	bool ctrl_global_maximum_sinter_strength;
	bool ctrl_global_minimum_sinter_strength;
	bool ctrl_global_temper_threshold_target;
	bool ctrl_global_maximum_temper_strength;
	bool ctrl_global_minimum_temper_strength;
	bool ctrl_global_awb_red_gain;
	bool ctrl_global_awb_blue_gain;
	bool ctrl_global_saturation_target;
	bool ctrl_global_anti_flicker_frequency ;
	bool ctrl_global_ae_compensation;
	bool ctrl_global_calibrate_bad_pixels ;
	bool ctrl_global_dis_x ;
	bool ctrl_global_dis_y ;
} system_tab_ctrl;

struct isp_core_stab_attr{
	system_tab stab;
	system_tab_ctrl stab_ctrl;
};

/* expr */
enum isp_core_expr_mode {
	ISP_CORE_EXPR_MODE_AUTO,
	ISP_CORE_EXPR_MODE_MANUAL,
};

enum isp_core_expr_unit {
	ISP_CORE_EXPR_UNIT_LINE,
	ISP_CORE_EXPR_UNIT_US,
};

union isp_core_expr_attr{
	struct {
		enum isp_core_expr_mode mode;
		enum isp_core_expr_unit unit;
		unsigned short time;
	} s_attr;
	struct {
		enum isp_core_expr_mode mode;
		unsigned short integration_time;
		unsigned short integration_time_min;
		unsigned short integration_time_max;
		unsigned short one_line_expr_in_us;
	} g_attr;
};

/* awb */
enum isp_core_wb_mode {
	ISP_CORE_WB_MODE_AUTO = 0,
	ISP_CORE_WB_MODE_MANUAL,
	ISP_CORE_WB_MODE_DAY_LIGHT,
	ISP_CORE_WB_MODE_CLOUDY,
	ISP_CORE_WB_MODE_INCANDESCENT,
	ISP_CORE_WB_MODE_FLOURESCENT,
	ISP_CORE_WB_MODE_TWILIGHT,
	ISP_CORE_WB_MODE_SHADE,
	ISP_CORE_WB_MODE_WARM_FLOURESCENT,
};

struct isp_core_wb_attr {
		enum isp_core_wb_mode mode;
		unsigned short rgain;
		unsigned short bgain;
};

/* ev */
struct isp_core_ev_attr {
	unsigned int ev;
	unsigned int expr_us;
	unsigned int ev_log2;
	unsigned int again;
	unsigned int dgain;
	unsigned int gain_log2;
};

/* mve */
struct isp_core_dis_statistic_info {
};

/* the defination of mode of isp during the day or night */
typedef enum isp_core_mode_day_and_night {
	ISP_CORE_RUNING_MODE_DAY_MODE,
	ISP_CORE_RUNING_MODE_NIGHT_MODE,
	ISP_CORE_RUNING_MODE_BUTT,
} ISP_CORE_MODE_DN_E;

/* The defination of strategy of AE */
typedef enum {
	IMPISP_AE_STRATEGY_SPLIT_BALANCED = 0,
	IMPISP_AE_STRATEGY_SPLIT_INTEGRATION_PRIORITY = 1,
	IMPISP_AE_STRATEGY_BUTT,
} ISP_CORE_AE_STRATEGY_E;

int isp_core_ops_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);
int isp_core_ops_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl);

/*
* it is defined as follows that is v4l2-ctrls.
*/

/* isp core tuning */
enum isp_image_tuning_cmd_id {
	IMAGE_TUNING_CID_CUSTOM_BASE = (V4L2_CID_USER_BASE | 0xe000),
	IMAGE_TUNING_CID_CUSTOM_AE_GAIN = IMAGE_TUNING_CID_CUSTOM_BASE,
	IMAGE_TUNING_CID_CUSTOM_AE_COMP,
	IMAGE_TUNING_CID_CUSTOM_AF_MODE,
	IMAGE_TUNING_CID_CUSTOM_ANTI_FOG,
	IMAGE_TUNING_CID_CUSTOM_RESOLUTION,
	IMAGE_TUNING_CID_CUSTOM_FPS,
	IMAGE_TUNING_CID_CUSTOM_TEST_PATTERN,		//test pattern
	IMAGE_TUNING_CID_CUSTOM_ISP_PROCESS, 		//isp process
	IMAGE_TUNING_CID_CUSTOM_ISP_FREEZE, 		//isp process
	IMAGE_TUNING_CID_CUSTOM_BL,			//black level
	IMAGE_TUNING_CID_CUSTOM_SHAD,			//lens shading
	IMAGE_TUNING_CID_CUSTOM_SINTER_DNS,		//sinter denoise
	IMAGE_TUNING_CID_CUSTOM_TEMPER_DNS,		//temper denoise
	IMAGE_TUNING_CID_CUSTOM_DYNAMIC_DP,		//dynamic defect pixels
	IMAGE_TUNING_CID_CUSTOM_GE,			//green equalist
	IMAGE_TUNING_CID_CUSTOM_STATIC_DP,		//static defect pixels
	IMAGE_TUNING_CID_CUSTOM_DRC,			//raw dynamic range compression
	IMAGE_TUNING_CID_CUSTOM_WDR_FLT,		//WDR companded frontend lookup table
	IMAGE_TUNING_CID_CUSTOM_WDR,			//sharpen
	IMAGE_TUNING_CID_CUSTOM_BUTT,			//the end
};

enum isp_image_tuning_private_cmd_id {
	IMAGE_TUNING_CID_AWB_ATTR = V4L2_CID_PRIVATE_BASE,
	IMAGE_TUNING_CID_AWB_CWF_SHIFT,
	IMAGE_TUNING_CID_MWB_ATTR,
	IMAGE_TUNING_CID_WB_STAINFO,
	IMAGE_TUNING_CID_WB_ATTR,
	IMAGE_TUNING_CID_WB_STATIS_ATTR,
	IMAGE_TUNING_CID_AE_ATTR = V4L2_CID_PRIVATE_BASE + 0x20,
	IMAGE_TUNING_CID_AE_STAINFO,
	IMAGE_TUNING_CID_AE_STRATEGY,
	IMAGE_TUNING_CID_AE_COMP,
	IMAGE_TUNING_CID_AE_ROI,
	IMAGE_TUNING_CID_EXPR_ATTR,
	IMAGE_TUNING_CID_ISP_EV_ATTR,
	IMAGE_TUNING_CID_GET_TOTAL_GAIN,
	IMAGE_TUNING_CID_MAX_AGAIN_ATTR,
	IMAGE_TUNING_CID_MAX_DGAIN_ATTR,
	IMAGE_TUNING_CID_HILIGHT_DEPRESS_STRENGTH,
	IMAGE_TUNING_CID_GAMMA_ATTR,
	IMAGE_TUNING_CID_SYSTEM_TAB,
	IMAGE_TUNING_CID_AF_ATTR = V4L2_CID_PRIVATE_BASE + 0x20 * 2,
	IMAGE_TUNING_CID_AF_STAINFO,
	IMAGE_TUNING_CID_DYNAMIC_DP_ATTR = V4L2_CID_PRIVATE_BASE + 0x20 * 3,
	IMAGE_TUNING_CID_STATIC_DP_ATTR,
	IMAGE_TUNING_CID_NOISE_PROFILE_ATTR = V4L2_CID_PRIVATE_BASE + 0x20 * 4,
	IMAGE_TUNING_CID_SINTER_ATTR ,
	IMAGE_TUNING_CID_TEMPER_STRENGTH,
	IMAGE_TUNING_CID_TEMPER_ATTR,
	IMAGE_TUNING_CID_DRC_ATTR = V4L2_CID_PRIVATE_BASE + 0x20 * 5,
	IMAGE_TUNING_CID_WDR_ATTR,
	IMAGE_TUNING_CID_SHARP_ATTR = V4L2_CID_PRIVATE_BASE + 0x20 * 6,
	IMAGE_TUNING_CID_DEMO_ATTR,
	IMAGE_TUNING_CID_FC_ATTR,
	IMAGE_TUNING_CID_CONTROL_FPS= V4L2_CID_PRIVATE_BASE + 0x20 * 7,
	IMAGE_TUNING_CID_DAY_OR_NIGHT,
	IMAGE_TUNING_CID_CCM_ATTR = V4L2_CID_PRIVATE_BASE + 0x20 * 8,
	IMAGE_TUNING_CID_SHAD_ATTR = V4L2_CID_PRIVATE_BASE + 0x20 * 9,
	IMAGE_TUNING_CID_GE_ATTR = V4L2_CID_PRIVATE_BASE + 0x20 * 10,
	IMAGE_TUNING_CID_DIS_STAINFO = V4L2_CID_PRIVATE_BASE + 0x20 * 11,
	IMAGE_TUNING_CID_ISP_TABLE_ATTR,
};

struct image_tuning_ctrls {
	struct v4l2_ctrl_handler handler;

	/* Auto white balance control cluster */
	struct {
		struct v4l2_ctrl *wb_mode;
		struct v4l2_ctrl *wb_temperature;
		struct isp_core_awb_attr awb_attr;
		struct isp_core_mwb_attr mwb_attr;
		struct isp_core_wb_statistic_info wb_info;
	};

	/* Auto/manual exposure control cluster */
	struct {
		struct v4l2_ctrl *exp_mode;
		struct v4l2_ctrl *manual_exp;
		struct v4l2_ctrl *exp_gain;
		struct v4l2_ctrl *exp_compensation;
		struct isp_core_ae_attr ae_attr;
		struct isp_core_ae_statistic_info ae_info;
	};

	/* Auto focus control cluster */
	struct {
		struct v4l2_ctrl *af_mode;
		struct v4l2_ctrl *af_status;
		struct isp_core_af_attr af_attr;
		struct isp_core_af_statistic_info af_info;
	};

	/* AE/AWB/AF lock/unlock */
	struct v4l2_ctrl *aefwb_lock;

	/* Enable - vertically flip */
	struct v4l2_ctrl *vflip;
	/* Enable - horizontally flip */
	struct v4l2_ctrl *hflip;

	/* Enable Sinter denoise module */
	struct v4l2_ctrl *sinter;
	struct isp_core_sinter_attr sinter_attr;

	/* Enable Temper denoise module */
	struct v4l2_ctrl *temper;
	struct isp_core_temper_attr temper_attr;

	/* Enable Iridix -- RAW Dynamic range compression module */
	struct v4l2_ctrl *raw_drc;
	struct isp_core_drc_attr drc_attr;

	/* Enable WDR Companded fronted LUT module */
	struct v4l2_ctrl *wdr_lut;
	/* Enable Wide Dynamic Range module */
	struct v4l2_ctrl *wdr;
	struct isp_core_wdr_attr wdr_attr;

	/* Enable ISP Process module */
	struct v4l2_ctrl *isp_process;
	struct v4l2_ctrl *freeze_fw;
	/* test pattern control */
	struct v4l2_ctrl *test_pattern;
	/* Enable Black level module */
	struct v4l2_ctrl *black_level;

	/* Enable - digital image stabilization */
	struct v4l2_ctrl *dis;
	struct isp_core_dis_statistic_info dis_info;

	/* Adjust - a power line frequency filter to avoid flicker */
	struct v4l2_ctrl *flicker;
	/* Enable Len shading module */
	struct v4l2_ctrl *lens_shad;
	struct isp_core_shading_attr shad_attr;

	/* Enable Static Defect pixels module */
	struct v4l2_ctrl *static_dp;
	struct isp_core_static_defect_pixel_attr sdp_attr;

	/* Enable Dynamic Defect pixels module */
	struct v4l2_ctrl *dynamic_dp;
	struct isp_core_dynamic_defect_pixel_attr ddp_attr;
	/* Enable Green Equalist module */
	struct v4l2_ctrl *green_eq;
	struct isp_core_green_eq_attr ge_attr;

	/* Adjust demosaic module  */
	struct isp_core_demosaic_attr demo_attr;
	struct isp_core_false_color_attr fc_attr;
	struct v4l2_ctrl *sharpness;
	struct isp_core_sharpness_attr sharp_attr;
	/* Noise Profile RAW frontend */
	struct isp_core_noise_profile_attr np_attr;


	/* Enable - Anti fog */
	struct v4l2_ctrl *fog;

	/* Adjust - contrast */
	struct v4l2_ctrl *contrast;
	/* Adjust - saturation */
	struct v4l2_ctrl *saturation;

	/* Adjust - brightness */
	struct v4l2_ctrl *brightness;

	/* ISP image scene */
	struct v4l2_ctrl *scene;
	/* ISP image effect */
	struct v4l2_ctrl *colorfx;

	/* sensor output resolution  */
	struct v4l2_ctrl *resolution;
	/* sensor output fps */
	struct v4l2_ctrl *fps;
	/* The mode of isp day and night */
	ISP_CORE_MODE_DN_E daynight;

};

/**
 * struct fimc_isp - FIMC-IS ISP data structure
 * @parent: pointer to ISP CORE device
 * @video: the ISP block image tuning device
 * @ctrl: v4l2 controls handler
 * @mlock: mutex serializing video device and the subdev operations
 * @state: driver state flags
 */
typedef struct tx_isp_image_tuning_video_driver {
	struct v4l2_subdev 		*parent;
	struct video_device 		*video;
	struct image_tuning_ctrls	ctrls;
	unsigned int			temper_buffer_size;
	unsigned int 			temper_paddr;
	unsigned int			wdr_buffer_size;
	unsigned int 			wdr_paddr;

	spinlock_t 			slock;
	struct mutex			mlock;
	atomic_t 			state;
	struct tx_isp_driver_fh		fh;
} image_tuning_vdrv_t;

#define ctrl_to_image_tuning(_ctrl) \
	container_of(ctrl->handler, image_tuning_vdrv_t, ctrls.handler)

struct video_device *tx_isp_image_tuning_device_register(struct v4l2_subdev *parent);
void tx_isp_image_tuning_device_release(struct video_device *vfd);



#endif //__TX_ISP_CORE_TUNING_H__
