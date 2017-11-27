// ------------------------------------------------------------------------------ //
//     This confidential and proprietary software/information may be used only    //
//        as authorized by a licensing agreement from Apical Limited              //
//                                                                                //
//                   (C) COPYRIGHT 2014 Apical Limited                            //
//                          ALL RIGHTS RESERVED                                   //
//                                                                                //
//      The entire notice above must be reproduced on all authorized              //
//       copies and copies may only be made to the extent permitted               //
//             by a licensing agreement from Apical Limited.                      //
//                                                                                //
// ------------------------------------------------------------------------------ //
#include <apical-isp/apical_calibrations_id.h>


// CALIBRATION_NP_LUT_MEAN
static  uint16_t _calibration_np_lut_mean[ ]
= { 53 } ;

// CALIBRATION_NP_LUT_MEAN
static  uint8_t _calibration_evtolux_probability_enable[ ]
= { 0 } ;
//jz
// CALIBRATION_AWB_AVG_COEF
static  uint8_t _calibration_awb_avg_coef[ ]
= { 18 } ;
//jz
// CALIBRATION_AWB_AVG_COEF
static  uint8_t _calibration_ae_exposure_avg_coef[ ]
= { 13 } ;

// CALIBRATION_AWB_AVG_COEF
static  uint8_t _calibration_iridix_avg_coef[ ]
= { 13 } ;

// CALIBRATION_AWB_AVG_COEF
static  uint16_t _calibration_af_min_table[ ][ 2 ]
= {
	{ 0 * 256, 0 }
} ;

// CALIBRATION_AWB_AVG_COEF
static  uint16_t _calibration_af_max_table[ ][ 2 ]
= {
	{ 0 * 256, 0 }
} ;

// CALIBRATION_AWB_AVG_COEF
static  uint16_t _calibration_af_window_resize_table[ ][ 2 ]
= {
	{ 0 * 256, 0 }
} ;

// CALIBRATION_AWB_AVG_COEF
static  uint16_t _calibration_cnr_uv_delta12_slope[ ][ 2 ]
= {
	{ 0 * 256, 0 }
} ;
//useless
// CALIBRATION_AWB_AVG_COEF
static  uint16_t _calibration_exp_ratio_table[ ][ 2 ]
= {
	{ 0 * 256, 0}
} ;

// CALIBRATION_CCM_ONE_GAIN_THRESHOLD
static  uint16_t _calibration_ccm_one_gain_threshold[ ]
= { 255 } ;

// CALIBRATION_FLASH_RG
static  uint8_t _calibration_flash_rg[ 1 ]
=  { 147 } ;

// CALIBRATION_FLASH_BG
static  uint8_t _calibration_flash_bg[ 1 ]
=  { 210 } ;


// CALIBRATION_GAMMA_FE_1_FS_LIN
static  uint8_t _calibration_iridix_strength_maximum_linear[ ]
=  { 180} ;

// CALIBRATION_GAMMA_FE_1_FS_LIN
static  uint8_t _calibration_iridix_strength_maximum_wdr[ ]
=  { 180 } ;
//jz same
// CALIBRATION_GAMMA_FE_1_FS_LIN
static  uint16_t _calibration_iridix_black_prc[ ]
=  { 30 } ;
//jz 1000 in old version
// CALIBRATION_GAMMA_FE_1_FS_LIN
static  uint16_t _calibration_iridix_gain_max[ ]
=  { 2400 } ;

// CALIBRATION_GAMMA_FE_1_FS_LIN
static  uint16_t _calibration_iridix_min_max_str[ ]
=  { 0 } ;
//jz
// CALIBRATION_GAMMA_FE_1_FS_LIN
static  uint32_t _calibration_iridix_ev_lim_full_str[ ]
=  {550000} ;//500000

//jz
// CALIBRATION_GAMMA_FE_1_FS_LIN
static  uint32_t _calibration_iridix_ev_lim_no_str_linear[ ]
=  { 1000000 } ;//782000

// CALIBRATION_GAMMA_FE_1_FS_LIN
static  uint32_t _calibration_iridix_ev_lim_no_str_fs_hdr[ ]
=  { 1000000 } ;
//jz lut_correction_linear
// CALIBRATION_GAMMA_FE_1_FS_LIN
static  uint8_t _calibration_ae_correction_linear[ ]
=  { 133,133,133,133,133,130,90,90,40,40 } ;

// CALIBRATION_GAMMA_FE_1_FS_LIN
static  uint8_t _calibration_ae_correction_fs_hdr[ ]
=  { 128,128,128,128,135,135,135,128,128 } ;

//jz lut_exp
// CALIBRATION_GAMMA_FE_1_FS_LIN
static  uint32_t _calibration_ae_exposure_correction[ ]
=  { 1000,2848, 26012, 64828, 168506, 334399, 412489, 2000000, 24119268, 221092671 } ;

//jz
static  uint16_t _calibration_sinter_strength_linear[ ][ 2 ]
= {{0,10},{1*256,15},{2*256,25},{3*256,35},{4*256,40},{5*256,45},{6*256,52},{7*256,55},{8*256,79}} ;

static  uint16_t _calibration_sinter_strength_fs_hdr[ ][ 2 ]
= {{0,10},{1*256,20},{2*256,30},{3*256,52},{4*256,64},{5*256,66},{6*256,68},{7*256,69},{8*256,85}} ;

//jz
static  uint16_t _calibration_sinter_strength1_linear[ ][ 2 ]
= {{0,255},{1*256,255},{2*256,255},{3*256,255},{4*256,200},{5*256,180}} ;

static  uint16_t _calibration_sinter_strength1_fs_hdr[ ][ 2 ]
= {{0,255},{1*256,255},{2*256,255},{3*256,255},{4*256,255}} ;

static  uint16_t _calibration_sinter_thresh1_linear[ ][ 2 ]
= {{0,0},{2*256,10},{3*256,15},{4*256,20},{5*256,20},{6*256,30}} ;

static  uint16_t _calibration_sinter_thresh1_fs_hdr[ ][ 2 ]
 = {
	 {0,0},{3*256,15},{4*256,20},{5*256,24},{6*256,30}
   } ;

//jz
static  uint16_t _calibration_sinter_thresh4_linear[ ][ 2 ]
= {{0,0},{2*256,6},{3*256,10},{4*256,15},{5*256,16},{6*256,16},{7*256,16}} ;

static  uint16_t _calibration_sinter_thresh4_fs_hdr[ ][ 2 ]
= {{0,15},{1*256,15},{2*256,15},{3*256,25},{4*256,35}} ;

//jz
static  uint16_t _calibration_sharp_alt_d_linear[ ][ 2 ]
= {{0,95},{1*256,95},{2*256,90},{3*256,75},{4*256,60},{5*256,45}} ;

static  uint16_t _calibration_sharp_alt_d_fs_hdr[ ][ 2 ]
= {{0,50},{4*256,35},{5*256,25},{6*256,15}} ;

//jz
static  uint16_t _calibration_sharp_alt_ud_linear[ ][ 2 ]
= {{0,85},{1*256,80},{2*256,75},{3*256,60},{4*256,40},{5*256,30}} ;

static  uint16_t _calibration_sharp_alt_ud_fs_hdr[ ][ 2 ]
= {{0,55},{4*256,50},{5*256,35},{6*256,5}} ;

static  uint16_t _calibration_sharpen_fr_linear[ ][ 2 ]
= {{0,10}};

static  uint16_t _calibration_sharpen_fr_hdr[ ][ 2 ]
= {
	{0,20}
	/* {0,20},{1*256,20},{2*256,20},{3*256,20},{4*256,18},{5*256,15},{6*256,15},{7*256,10},{8*256,10} */
} ;


static  uint16_t _calibration_sharpen_ds1_linear[ ][ 2 ]
= {{0,50},{1*256,50},{2*256,50},{3*256,50},{4*256,50},{5*256,50},{6*256,40},{7*256,25},{8*256,10}} ;

static  uint16_t _calibration_sharpen_ds1_hdr[ ][ 2 ]
= {
	{0,50},{1*256,50},{2*256,50},{3*256,50},{4*256,50},{5*256,50},{6*256,40},{7*256,25},{8*256,10}
} ;


static  uint16_t _calibration_sharpen_ds2_linear[ ][ 2 ]
= {{0,70},{1*256,70},{2*256,70},{3*256,70},{4*256,70},{5*256,50},{6*256,40},{7*256,25},{8*256,10}} ;

static  uint16_t _calibration_sharpen_ds2_hdr[ ][ 2 ]
= {
	{0,70},{1*256,70},{2*256,70},{3*256,70},{4*256,70},{5*256,50},{6*256,40},{7*256,25},{8*256,10}
} ;

static  uint16_t _calibration_demosaic_np_offset_linear[ ][ 2 ]
= {{0*256,0},{3*256,0},{4*256,8},{4*256,18},{5*256,22},{6*256,30}} ;
/* = {{0*256,0},{3*256,0},{4*256,8},{4*256,18},{5*256,22},{6*256,30}} ; */

static  uint16_t _calibration_demosaic_np_offset_fs_hdr[ ][ 2 ]
= {{0*256,0},{3*256,0},{4*256,8},{4*256,18},{5*256,22},{6*256,30}} ;

static  uint16_t _calibration_mesh_shading_strength[ ][ 2 ]
= {
	{ 3 * 256, 0x1000 },
	{ 5 * 256, 0}
} ;

static  uint16_t _calibration_saturation_strength_linear[ ][ 2 ]
= {{0*256,128},{1*256,128},{2*256,128},{3*256,118},{4*256,95},{5*256,72}} ;

static  uint16_t _calibration_saturation_strength_fs_hdr[ ][ 2 ]
= {{0*256,130}} ;

static  uint16_t _calibration_temper_strength[ ][ 2 ]
= {{0,80},{1*256,85},{2*256,93},{3*256,110},{4*256,100},{5*256,100}} ;



static  uint16_t _calibration_stitching_error_thresh[ ][ 2 ]
= { { 0 * 256, 64 },
	{ 1 * 256, 64 },
	{ 2 * 256, 64 },
	{ 3 * 256, 64 },
	{ 4 * 256, 64  },
	{ 5 * 256, 64  }
} ;



static  uint16_t _calibration_dp_slope_linear[ ][ 2 ]
 = {{ 0 * 256, 170 },
	{ 3 * 256, 170 },
	{ 4 * 256, 1023 },

} ;

static  uint16_t _calibration_dp_slope_fs_hdr[ ][ 2 ]
= { { 0 * 256, 170 },
    { 1 * 256, 170 },
	{ 3 * 256, 170 },
	{ 4 * 256, 170 },
	{ 5 * 256, 300 }
} ;


static  uint16_t _calibration_dp_threshold_linear[ ][ 2 ]
= {{ 0 * 256, 4095 },
   { 1 * 256, 4095 },
   { 2 * 256, 2600 },
   { 3 * 256, 1023 },
   { 4 * 256, 64 },
   { 5 * 256, 64 }

} ;

static  uint16_t _calibration_dp_threshold_fs_hdr[ ][ 2 ]
= {{ 0 * 256, 4095 },
   { 1 * 256, 4095 },
   { 2 * 256, 2600 },
   { 3 * 256, 1023 },
   { 4 * 256, 64 },
   { 5 * 256, 64 }

} ;
// CALIBRATION_AE_BALANCED_LINEAR
// AE_PI_COEFF,AE_TARGET_POINT,AE_TAIL_WEIGHT,AE_LONG_CLIP,AE_ER_AVG_COEFF
static  uint32_t _calibration_ae_balanced_linear[ ]
=  { 10, 60, 0, 0, 0, 0, ANALOG_GAIN_ACCURACY};

// CALIBRATION_AE_BALANCED_LINEAR
// AE_PI_COEFF,AE_TARGET_POINT,AE_TAIL_WEIGHT,AE_LONG_CLIP,AE_ER_AVG_COEFF
static  uint32_t _calibration_ae_balanced_wdr[ ]
=  { 10,  66, 0, 96, 5, 0, ANALOG_GAIN_ACCURACY} ;

static  uint16_t _calibration_iridix_strength_table[ ][2]
= { {1*64,8}
	/* {2*64,65}, */
	/* {4*64,115}, */
	/* {8*64,165}, */
	/* {16*64,255} */
};
static uint16_t _calibration_rgb2yuv_conversion[]={ 47,157,16,32794, 32855, 112, 112, 32870, 32778, 64, 512,512};
static LookupTable calibration_dp_slope_linear = { .ptr = _calibration_dp_slope_linear, .rows = sizeof(_calibration_dp_slope_linear) / sizeof(_calibration_dp_slope_linear[ 0 ]), .cols = 2, .width = sizeof(_calibration_dp_slope_linear[ 0 ][ 0 ]) } ;
static LookupTable calibration_dp_slope_fs_hdr = { .ptr = _calibration_dp_slope_fs_hdr, .rows = sizeof(_calibration_dp_slope_fs_hdr) / sizeof(_calibration_dp_slope_fs_hdr[ 0 ]), .cols = 2, .width = sizeof(_calibration_dp_slope_fs_hdr[ 0 ][ 0 ]) } ;
static LookupTable calibration_dp_threshold_linear = { .ptr = _calibration_dp_threshold_linear, .rows = sizeof(_calibration_dp_threshold_linear) / sizeof(_calibration_dp_threshold_linear[ 0 ]), .cols = 2, .width = sizeof(_calibration_dp_threshold_linear[ 0 ][ 0 ]) } ;
static LookupTable calibration_dp_threshold_fs_hdr = { .ptr = _calibration_dp_threshold_fs_hdr, .rows = sizeof(_calibration_dp_threshold_fs_hdr) / sizeof(_calibration_dp_threshold_fs_hdr[ 0 ]), .cols = 2, .width = sizeof(_calibration_dp_threshold_fs_hdr[ 0 ][ 0 ]) } ;
static LookupTable calibration_stitching_error_thresh = { .ptr = _calibration_stitching_error_thresh, .rows = sizeof(_calibration_stitching_error_thresh) / sizeof(_calibration_stitching_error_thresh[ 0 ]), .cols = 2, .width = sizeof(_calibration_stitching_error_thresh[ 0 ][ 0 ]) } ;
static LookupTable calibration_np_lut_mean = { .ptr = _calibration_np_lut_mean, .rows = 1, .cols = sizeof(_calibration_np_lut_mean) / sizeof(_calibration_np_lut_mean[ 0 ] ), .width = sizeof(_calibration_np_lut_mean[ 0 ]) } ;
static LookupTable calibration_evtolux_probability_enable = { .ptr = _calibration_evtolux_probability_enable, .rows = 1, .cols = sizeof(_calibration_evtolux_probability_enable) / sizeof(_calibration_evtolux_probability_enable[ 0 ] ), .width = sizeof(_calibration_evtolux_probability_enable[ 0 ]) } ;
static LookupTable calibration_cnr_uv_delta12_slope = { .ptr = _calibration_cnr_uv_delta12_slope, .rows = sizeof(_calibration_cnr_uv_delta12_slope) / sizeof(_calibration_cnr_uv_delta12_slope[ 0 ]), .cols = 2, .width = sizeof(_calibration_cnr_uv_delta12_slope [ 0 ][ 0 ]) } ;
static LookupTable calibration_af_min_table = { .ptr = _calibration_af_min_table, .rows = sizeof(_calibration_af_min_table) / sizeof(_calibration_af_min_table[ 0 ]), .cols = 2, .width = sizeof(_calibration_af_min_table [ 0 ][ 0 ]) } ;
static LookupTable calibration_af_max_table = { .ptr = _calibration_af_max_table, .rows = sizeof(_calibration_af_max_table) / sizeof(_calibration_af_max_table[ 0 ]), .cols = 2, .width = sizeof(_calibration_af_max_table [ 0 ][ 0 ]) } ;
static LookupTable calibration_af_window_resize_table = { .ptr = _calibration_af_window_resize_table, .rows = sizeof(_calibration_af_window_resize_table) / sizeof(_calibration_af_window_resize_table[ 0 ]), .cols = 2, .width = sizeof(_calibration_af_window_resize_table [ 0 ][ 0 ]) } ;
static LookupTable calibration_exp_ratio_table = { .ptr = _calibration_exp_ratio_table, .rows = sizeof(_calibration_exp_ratio_table) / sizeof(_calibration_exp_ratio_table[ 0 ]), .cols = 2, .width = sizeof(_calibration_exp_ratio_table [ 0 ][ 0 ]) } ;
static LookupTable calibration_awb_avg_coef = { .ptr = _calibration_awb_avg_coef, .rows = 1, .cols = sizeof(_calibration_awb_avg_coef) / sizeof(uint8_t), .width = sizeof(_calibration_awb_avg_coef[ 0 ]) } ;
static LookupTable calibration_iridix_avg_coef = { .ptr = _calibration_iridix_avg_coef, .rows = 1, .cols = sizeof(_calibration_iridix_avg_coef) / sizeof(uint8_t), .width = sizeof(_calibration_iridix_avg_coef[ 0 ]) } ;
static LookupTable calibration_exposure_avg_coef = { .ptr = _calibration_ae_exposure_avg_coef, .rows = 1, .cols = sizeof(_calibration_ae_exposure_avg_coef) / sizeof(uint8_t), .width = sizeof(_calibration_ae_exposure_avg_coef[ 0 ]) } ;
static LookupTable calibration_iridix_strength_maximum_linear = { .ptr = _calibration_iridix_strength_maximum_linear, .rows = 1, .cols = sizeof(_calibration_iridix_strength_maximum_linear) / sizeof(uint8_t), .width = sizeof(_calibration_iridix_strength_maximum_linear[ 0 ]) } ;
static LookupTable calibration_iridix_strength_maximum_wdr = { .ptr = _calibration_iridix_strength_maximum_wdr, .rows = 1, .cols = sizeof(_calibration_iridix_strength_maximum_wdr) / sizeof(uint8_t), .width = sizeof(_calibration_iridix_strength_maximum_wdr[ 0 ]) } ;
static LookupTable calibration_iridix_black_prc = { .ptr = _calibration_iridix_black_prc, .rows = 1, .cols = sizeof(_calibration_iridix_black_prc) / sizeof(uint16_t), .width = sizeof(_calibration_iridix_black_prc[ 0 ]) } ;
static LookupTable calibration_iridix_gain_max = { .ptr = _calibration_iridix_gain_max, .rows = 1, .cols = sizeof(_calibration_iridix_gain_max) / sizeof(uint16_t), .width = sizeof(_calibration_iridix_gain_max[ 0 ]) } ;
static LookupTable calibration_iridix_min_max_str = { .ptr =_calibration_iridix_min_max_str, .rows = 1, .cols = sizeof(_calibration_iridix_min_max_str) / sizeof(uint16_t), .width = sizeof(_calibration_iridix_min_max_str[ 0 ]) } ;
static LookupTable calibration_iridix_ev_lim_full_str = { .ptr = _calibration_iridix_ev_lim_full_str, .rows = 1, .cols = sizeof(_calibration_iridix_ev_lim_full_str) / sizeof(uint32_t), .width = sizeof(_calibration_iridix_ev_lim_full_str[ 0 ]) } ;
static LookupTable calibration_iridix_ev_lim_no_str_linear = { .ptr = _calibration_iridix_ev_lim_no_str_linear, .rows = 1, .cols = sizeof(_calibration_iridix_ev_lim_no_str_linear) / sizeof(uint32_t), .width = sizeof(_calibration_iridix_ev_lim_no_str_linear[ 0 ]) } ;
static LookupTable calibration_iridix_ev_lim_no_str_fs_hdr = { .ptr = _calibration_iridix_ev_lim_no_str_fs_hdr, .rows = 1, .cols = sizeof(_calibration_iridix_ev_lim_no_str_fs_hdr) / sizeof(uint32_t), .width = sizeof(_calibration_iridix_ev_lim_no_str_fs_hdr[ 0 ]) } ;
static LookupTable calibration_ae_correction_linear = { .ptr = _calibration_ae_correction_linear, .rows = 1, .cols = sizeof(_calibration_ae_correction_linear) / sizeof(uint8_t), .width = sizeof(_calibration_ae_correction_linear[ 0 ]) } ;
static LookupTable calibration_ae_correction_fs_hdr = { .ptr = _calibration_ae_correction_fs_hdr, .rows = 1, .cols = sizeof(_calibration_ae_correction_fs_hdr) / sizeof(uint8_t), .width = sizeof(_calibration_ae_correction_fs_hdr[ 0 ]) } ;
static LookupTable calibration_ae_exposure_correction = { .ptr = _calibration_ae_exposure_correction, .rows = 1, .cols = sizeof(_calibration_ae_exposure_correction) / sizeof(_calibration_ae_exposure_correction[ 0 ] ), .width = sizeof(_calibration_ae_exposure_correction[ 0 ]) } ;
static LookupTable calibration_sinter_strength_linear = { .ptr = _calibration_sinter_strength_linear, .rows = sizeof(_calibration_sinter_strength_linear) / sizeof(_calibration_sinter_strength_linear[ 0 ]), .cols = 2, .width = sizeof(_calibration_sinter_strength_linear[ 0 ][ 0 ]) } ;
static LookupTable calibration_sinter_strength_fs_hdr = { .ptr = _calibration_sinter_strength_fs_hdr, .rows = sizeof(_calibration_sinter_strength_fs_hdr) / sizeof(_calibration_sinter_strength_fs_hdr[ 0 ]), .cols = 2, .width = sizeof(_calibration_sinter_strength_fs_hdr[ 0 ][ 0 ]) } ;
static LookupTable calibration_sinter_strength1_linear = { .ptr = _calibration_sinter_strength1_linear, .rows = sizeof(_calibration_sinter_strength1_linear) / sizeof(_calibration_sinter_strength1_linear[ 0 ]), .cols = 2, .width = sizeof(_calibration_sinter_strength1_linear[ 0 ][ 0 ]) } ;
static LookupTable calibration_sinter_strength1_fs_hdr = { .ptr = _calibration_sinter_strength1_fs_hdr, .rows = sizeof(_calibration_sinter_strength1_fs_hdr) / sizeof(_calibration_sinter_strength1_fs_hdr[ 0 ]), .cols = 2, .width = sizeof(_calibration_sinter_strength1_fs_hdr[ 0 ][ 0 ]) } ;
static LookupTable calibration_sinter_thresh1_linear = { .ptr = _calibration_sinter_thresh1_linear, .rows = sizeof(_calibration_sinter_thresh1_linear) / sizeof(_calibration_sinter_thresh1_linear[ 0 ]), .cols = 2, .width = sizeof(_calibration_sinter_thresh1_linear[ 0 ][ 0 ]) } ;
static LookupTable calibration_sinter_thresh1_fs_hdr = { .ptr = _calibration_sinter_thresh1_fs_hdr, .rows = sizeof(_calibration_sinter_thresh1_fs_hdr) / sizeof(_calibration_sinter_thresh1_fs_hdr[ 0 ]), .cols = 2, .width = sizeof(_calibration_sinter_thresh1_fs_hdr[ 0 ][ 0 ]) } ;
static LookupTable calibration_sinter_thresh4_linear = { .ptr = _calibration_sinter_thresh4_linear, .rows = sizeof(_calibration_sinter_thresh4_linear) / sizeof(_calibration_sinter_thresh4_linear[ 0 ]), .cols = 2, .width = sizeof(_calibration_sinter_thresh4_linear[ 0 ][ 0 ]) } ;
static LookupTable calibration_sinter_thresh4_fs_hdr = { .ptr = _calibration_sinter_thresh4_fs_hdr, .rows = sizeof(_calibration_sinter_thresh4_fs_hdr) / sizeof(_calibration_sinter_thresh4_fs_hdr[ 0 ]), .cols = 2, .width = sizeof(_calibration_sinter_thresh4_fs_hdr[ 0 ][ 0 ]) } ;
static LookupTable calibration_sharp_alt_d_linear = { .ptr = _calibration_sharp_alt_d_linear, .rows = sizeof(_calibration_sharp_alt_d_linear) / sizeof(_calibration_sharp_alt_d_linear[ 0 ]), .cols = 2, .width = sizeof(_calibration_sharp_alt_d_linear[ 0 ][ 0 ]) } ;
static LookupTable calibration_sharp_alt_d_fs_hdr = { .ptr = _calibration_sharp_alt_d_fs_hdr, .rows = sizeof(_calibration_sharp_alt_d_fs_hdr) / sizeof(_calibration_sharp_alt_d_fs_hdr[ 0 ]), .cols = 2, .width = sizeof(_calibration_sharp_alt_d_fs_hdr[ 0 ][ 0 ]) } ;
static LookupTable calibration_sharp_alt_ud_linear = { .ptr = _calibration_sharp_alt_ud_linear, .rows = sizeof(_calibration_sharp_alt_ud_linear) / sizeof(_calibration_sharp_alt_ud_linear[ 0 ]), .cols = 2, .width = sizeof(_calibration_sharp_alt_ud_linear[ 0 ][ 0 ]) } ;
static LookupTable calibration_sharp_alt_ud_fs_hdr = { .ptr = _calibration_sharp_alt_ud_fs_hdr, .rows = sizeof(_calibration_sharp_alt_ud_fs_hdr) / sizeof(_calibration_sharp_alt_ud_fs_hdr[ 0 ]), .cols = 2, .width = sizeof(_calibration_sharp_alt_ud_fs_hdr[ 0 ][ 0 ]) } ;
static LookupTable calibration_sharpen_fr_linear = { .ptr = _calibration_sharpen_fr_linear, .rows = sizeof(_calibration_sharpen_fr_linear) / sizeof(_calibration_sharpen_fr_linear[ 0 ]), .cols = 2, .width = sizeof(_calibration_sharpen_fr_linear[ 0 ][ 0 ]) } ;
static LookupTable calibration_sharpen_fr_hdr = { .ptr = _calibration_sharpen_fr_hdr, .rows = sizeof(_calibration_sharpen_fr_hdr) / sizeof(_calibration_sharpen_fr_hdr[ 0 ]), .cols = 2, .width = sizeof(_calibration_sharpen_fr_hdr[ 0 ][ 0 ]) } ;
static LookupTable calibration_sharpen_ds1_linear = { .ptr = _calibration_sharpen_ds1_linear, .rows = sizeof(_calibration_sharpen_ds1_linear) / sizeof(_calibration_sharpen_ds1_linear[ 0 ]), .cols = 2, .width = sizeof(_calibration_sharpen_ds1_linear[ 0 ][ 0 ]) } ;
static LookupTable calibration_sharpen_ds1_hdr = { .ptr = _calibration_sharpen_ds1_hdr, .rows = sizeof(_calibration_sharpen_ds1_hdr) / sizeof(_calibration_sharpen_ds1_hdr[ 0 ]), .cols = 2, .width = sizeof(_calibration_sharpen_ds1_hdr[ 0 ][ 0 ]) } ;
static LookupTable calibration_sharpen_ds2_linear = { .ptr = _calibration_sharpen_ds2_linear, .rows = sizeof(_calibration_sharpen_ds2_linear) / sizeof(_calibration_sharpen_ds2_linear[ 0 ]), .cols = 2, .width = sizeof(_calibration_sharpen_ds2_linear[ 0 ][ 0 ]) } ;
static LookupTable calibration_sharpen_ds2_hdr = { .ptr = _calibration_sharpen_ds2_hdr, .rows = sizeof(_calibration_sharpen_ds2_hdr) / sizeof(_calibration_sharpen_ds2_hdr[ 0 ]), .cols = 2, .width = sizeof(_calibration_sharpen_ds2_hdr[ 0 ][ 0 ]) } ;
static LookupTable calibration_demosaic_np_offset_linear = { .ptr = _calibration_demosaic_np_offset_linear, .rows = sizeof(_calibration_demosaic_np_offset_linear) / sizeof(_calibration_demosaic_np_offset_linear[ 0 ]), .cols = 2, .width = sizeof(_calibration_demosaic_np_offset_linear[ 0 ][ 0 ]) } ;
static LookupTable calibration_demosaic_np_offset_fs_hdr = { .ptr = _calibration_demosaic_np_offset_fs_hdr, .rows = sizeof(_calibration_demosaic_np_offset_fs_hdr) / sizeof(_calibration_demosaic_np_offset_fs_hdr[ 0 ]), .cols = 2, .width = sizeof(_calibration_demosaic_np_offset_fs_hdr[ 0 ][ 0 ]) } ;
static LookupTable calibration_mesh_shading_strength = { .ptr = _calibration_mesh_shading_strength, .rows = sizeof(_calibration_mesh_shading_strength) / sizeof(_calibration_mesh_shading_strength[ 0 ]), .cols = 2, .width = sizeof(_calibration_mesh_shading_strength[ 0 ][ 0 ]) } ;
static LookupTable calibration_saturation_strength_linear = { .ptr = _calibration_saturation_strength_linear, .rows = sizeof(_calibration_saturation_strength_linear) / sizeof(_calibration_saturation_strength_linear[ 0 ]), .cols = 2, .width = sizeof(_calibration_saturation_strength_linear[ 0 ][ 0 ]) } ;
static LookupTable calibration_saturation_strength_fs_hdr = { .ptr = _calibration_saturation_strength_fs_hdr, .rows = sizeof(_calibration_saturation_strength_fs_hdr) / sizeof(_calibration_saturation_strength_fs_hdr[ 0 ]), .cols = 2, .width = sizeof(_calibration_saturation_strength_fs_hdr[ 0 ][ 0 ]) } ;
static LookupTable calibration_temper_strength = { .ptr = _calibration_temper_strength, .rows = sizeof(_calibration_temper_strength) / sizeof(_calibration_temper_strength[ 0 ]), .cols = 2, .width = sizeof(_calibration_temper_strength[ 0 ][ 0 ]) } ;
static LookupTable calibration_ccm_one_gain_threshold = { .ptr = _calibration_ccm_one_gain_threshold, .cols = sizeof(_calibration_ccm_one_gain_threshold) / sizeof(_calibration_ccm_one_gain_threshold[ 0 ]), .rows = 1, .width = sizeof(_calibration_ccm_one_gain_threshold[ 0 ]) } ;
static LookupTable calibration_flash_rg =  { .ptr = _calibration_flash_rg, .rows = 1, .cols = sizeof(_calibration_flash_rg) / sizeof(_calibration_flash_rg[ 0 ]), .width = sizeof(_calibration_flash_rg[0]) } ;
static LookupTable calibration_flash_bg =  { .ptr = _calibration_flash_bg, .rows = 1, .cols = sizeof(_calibration_flash_bg) / sizeof(_calibration_flash_bg[ 0 ]), .width = sizeof(_calibration_flash_bg[0]) } ;
static LookupTable calibration_ae_balanced_linear =  { .ptr = _calibration_ae_balanced_linear, .rows = 1, .cols = sizeof(_calibration_ae_balanced_linear) / sizeof(_calibration_ae_balanced_linear[ 0 ]), .width = sizeof(_calibration_ae_balanced_linear[0]) } ;
static LookupTable calibration_ae_balanced_wdr =  { .ptr = _calibration_ae_balanced_wdr, .rows = 1, .cols = sizeof(_calibration_ae_balanced_wdr) / sizeof(_calibration_ae_balanced_wdr[ 0 ]), .width = sizeof(_calibration_ae_balanced_wdr[0]) } ;
static LookupTable calibration_iridix_strength_table = { .ptr = _calibration_iridix_strength_table, .rows = sizeof(_calibration_iridix_strength_table) / sizeof(_calibration_iridix_strength_table[ 0 ]), .cols = 2, .width = sizeof(_calibration_iridix_strength_table[ 0 ][ 0 ]) } ;
static LookupTable calibration_rgb2yuv_conversion =  { .ptr = _calibration_rgb2yuv_conversion, .rows = 1, .cols = sizeof(_calibration_rgb2yuv_conversion) / sizeof(_calibration_rgb2yuv_conversion[ 0 ]), .width = sizeof(_calibration_rgb2yuv_conversion[0]) } ; 

 uint32_t get_dynamic_calibrations( ApicalCalibrations *c ) {
	uint32_t result = 0 ;
	if ( c != 0 ) {
		c->calibrations[ _CALIBRATION_DP_SLOPE_LINEAR ] = &calibration_dp_slope_linear ;
		c->calibrations[ _CALIBRATION_DP_SLOPE_FS_HDR ] = &calibration_dp_slope_fs_hdr ;
		c->calibrations[ _CALIBRATION_DP_THRESHOLD_LINEAR ] = &calibration_dp_threshold_linear ;
		c->calibrations[ _CALIBRATION_DP_THRESHOLD_FS_HDR ] = &calibration_dp_threshold_fs_hdr ;
		c->calibrations[ _CALIBRATION_STITCHING_ERROR_THRESH ] = &calibration_stitching_error_thresh ;
		c->calibrations[ _CALIBRATION_EVTOLUX_PROBABILITY_ENABLE ] = &calibration_evtolux_probability_enable ;
		c->calibrations[ _CALIBRATION_NP_LUT_MEAN ] = &calibration_np_lut_mean ;
		c->calibrations[ _CALIBRATION_AWB_AVG_COEF ] = &calibration_awb_avg_coef ;
		c->calibrations[ _CALIBRATION_IRIDIX_AVG_COEF ] = &calibration_iridix_avg_coef ;
		c->calibrations[ _CALIBRATION_AE_EXPOSURE_AVG_COEF ] = &calibration_exposure_avg_coef ;
		c->calibrations[ _CALIBRATION_CNR_UV_DELTA12_SLOPE ] = &calibration_cnr_uv_delta12_slope ;
		c->calibrations[ _CALIBRATION_AF_MIN_TABLE ] = &calibration_af_min_table ;
		c->calibrations[ _CALIBRATION_AF_MAX_TABLE ] = &calibration_af_max_table ;
		c->calibrations[ _CALIBRATION_AF_WINDOW_RESIZE_TABLE ] = &calibration_af_window_resize_table ;
		c->calibrations[ _CALIBRATION_EXP_RATIO_TABLE ] = &calibration_exp_ratio_table ;
		c->calibrations[ _CALIBRATION_IRIDIX_STRENGTH_MAXIMUM_LINEAR ] = &calibration_iridix_strength_maximum_linear ;
		c->calibrations[ _CALIBRATION_IRIDIX_STRENGTH_MAXIMUM_WDR ] = &calibration_iridix_strength_maximum_wdr ;
		c->calibrations[ _CALIBRATION_IRIDIX_BLACK_PRC ] = &calibration_iridix_black_prc ;
		c->calibrations[ _CALIBRATION_IRIDIX_GAIN_MAX ] = &calibration_iridix_gain_max ;
		c->calibrations[ _CALIBRATION_IRIDIX_MIN_MAX_STR ] = &calibration_iridix_min_max_str ;
		c->calibrations[ _CALIBRATION_IRIDIX_EV_LIM_FULL_STR ] = &calibration_iridix_ev_lim_full_str ;
		c->calibrations[ _CALIBRATION_IRIDIX_EV_LIM_NO_STR_LINEAR ] = &calibration_iridix_ev_lim_no_str_linear ;
		c->calibrations[ _CALIBRATION_IRIDIX_EV_LIM_NO_STR_FS_HDR ] = &calibration_iridix_ev_lim_no_str_fs_hdr ;
		c->calibrations[ _CALIBRATION_AE_CORRECTION_LINEAR ] = &calibration_ae_correction_linear ;
		c->calibrations[ _CALIBRATION_AE_CORRECTION_FS_HDR ] = &calibration_ae_correction_fs_hdr ;
		c->calibrations[ _CALIBRATION_AE_EXPOSURE_CORRECTION ] = &calibration_ae_exposure_correction ;
		c->calibrations[ _CALIBRATION_SINTER_STRENGTH_LINEAR ] = &calibration_sinter_strength_linear ;
		c->calibrations[ _CALIBRATION_SINTER_STRENGTH_FS_HDR ] = &calibration_sinter_strength_fs_hdr ;
		c->calibrations[ _CALIBRATION_SINTER_STRENGTH1_LINEAR ] = &calibration_sinter_strength1_linear ;
		c->calibrations[ _CALIBRATION_SINTER_STRENGTH1_FS_HDR ] = &calibration_sinter_strength1_fs_hdr ;
		c->calibrations[ _CALIBRATION_SINTER_THRESH1_LINEAR ] = &calibration_sinter_thresh1_linear ;
		c->calibrations[ _CALIBRATION_SINTER_THRESH1_FS_HDR ] = &calibration_sinter_thresh1_fs_hdr ;
		c->calibrations[ _CALIBRATION_SINTER_THRESH4_LINEAR ] = &calibration_sinter_thresh4_linear ;
		c->calibrations[ _CALIBRATION_SINTER_THRESH4_FS_HDR ] = &calibration_sinter_thresh4_fs_hdr ;
		c->calibrations[ _CALIBRATION_SHARP_ALT_D_LINEAR ] = &calibration_sharp_alt_d_linear ;
		c->calibrations[ _CALIBRATION_SHARP_ALT_D_FS_HDR ] = &calibration_sharp_alt_d_fs_hdr ;
		c->calibrations[ _CALIBRATION_SHARP_ALT_UD_LINEAR ] = &calibration_sharp_alt_ud_linear ;
		c->calibrations[ _CALIBRATION_SHARP_ALT_UD_FS_HDR ] = &calibration_sharp_alt_ud_fs_hdr ;
		c->calibrations[ _CALIBRATION_SHARPEN_FR_LINEAR ] = &calibration_sharpen_fr_linear ;
		c->calibrations[ _CALIBRATION_SHARPEN_FR_WDR ] = &calibration_sharpen_fr_hdr ;
		c->calibrations[ _CALIBRATION_SHARPEN_DS1_LINEAR ] = &calibration_sharpen_ds1_linear ;
		c->calibrations[ _CALIBRATION_SHARPEN_DS1_WDR ] = &calibration_sharpen_ds1_hdr ;
		c->calibrations[ _CALIBRATION_SHARPEN_DS2_LINEAR ] = &calibration_sharpen_ds2_linear ;
		c->calibrations[ _CALIBRATION_SHARPEN_DS2_WDR ] = &calibration_sharpen_ds2_hdr ;
		c->calibrations[ _CALIBRATION_DEMOSAIC_NP_OFFSET_LINEAR ] = &calibration_demosaic_np_offset_linear ;
		c->calibrations[ _CALIBRATION_DEMOSAIC_NP_OFFSET_FS_HDR ] = &calibration_demosaic_np_offset_fs_hdr ;
		c->calibrations[ _CALIBRATION_MESH_SHADING_STRENGTH ] = &calibration_mesh_shading_strength ;
		c->calibrations[ _CALIBRATION_SATURATION_STRENGTH_LINEAR ] = &calibration_saturation_strength_linear ;
		c->calibrations[ _CALIBRATION_SATURATION_STRENGTH_FS_HDR ] = &calibration_saturation_strength_fs_hdr ;
		c->calibrations[ _CALIBRATION_TEMPER_STRENGTH ] = &calibration_temper_strength ;
		c->calibrations[ _CALIBRATION_FLASH_RG ] = &calibration_flash_rg ;
		c->calibrations[ _CALIBRATION_FLASH_BG ] = &calibration_flash_bg ;
		c->calibrations[ _CALIBRATION_CCM_ONE_GAIN_THRESHOLD ] = &calibration_ccm_one_gain_threshold ;
		c->calibrations[ _CALIBRATION_AE_BALANCED_LINEAR ] = &calibration_ae_balanced_linear ;
		c->calibrations[ _CALIBRATION_AE_BALANCED_WDR ] = &calibration_ae_balanced_wdr ;
		c->calibrations[ _CALIBRATION_IRIDIX_STRENGTH_TABLE] = &calibration_iridix_strength_table;
		c->calibrations[ _CALIBRATION_RGB2YUV_CONVERSION] = &calibration_rgb2yuv_conversion;
	} else {
		result = -1 ;
	}
	return result ;
}
