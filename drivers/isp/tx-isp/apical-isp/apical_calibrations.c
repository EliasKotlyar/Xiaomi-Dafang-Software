/*-----------------------------------------------------------------------------
     This confidential and proprietary software/information may be used only
        as authorized by a licensing agreement from Apical Limited

                   (C) COPYRIGHT 2011 - 2015 Apical Limited
                          ALL RIGHTS RESERVED

      The entire notice above must be reproduced on all authorized
       copies and copies may only be made to the extent permitted
             by a licensing agreement from Apical Limited.
-----------------------------------------------------------------------------*/

#include <apical-isp/apical.h>
#include "apical_calibrations_init.h"
#include <apical-isp/apical_firmware_config.h>
#include "log.h"



// declare all functions which will return isp and sensor settings
extern uint32_t get_static_calibrations( ApicalCalibrations *) ;
extern uint32_t get_dynamic_calibrations( ApicalCalibrations *) ;

#if CALIBRATION_PREVIEW_SET_SUPPORT== 1
extern uint32_t get_dynamic_calibrations_pr( ApicalCalibrations *) ;
extern uint32_t get_static_calibrations_pr( ApicalCalibrations *) ;
#endif

// you can use as many calibrations set as you need
uint32_t (*static_calibrations_func[])( ApicalCalibrations *) = {
    get_static_calibrations
#if CALIBRATION_PREVIEW_SET_SUPPORT == 1
    ,get_static_calibrations_pr
#endif
    } ;


uint32_t (*dynamic_calibrations_func[])( ApicalCalibrations *) = {
    get_dynamic_calibrations
#if CALIBRATION_PREVIEW_SET_SUPPORT == 1
    ,get_dynamic_calibrations_pr
#endif
     } ;

// current settings for sensor and isp
ApicalCalibrations apicalCalibrations ;


#if RESTRICTED_SOURCES==0
    // use this array only for debug purposes
    // to detect calibrations which are not initialized but
    // used inside firmware.
    // it should be disabled in the release version to save memory
    char *s_calibrationID[] = {
        "_CALIBRATION_LUTS_START_INDEX",
        "_CALIBRATION_NOISE_PROFILE_LINEAR",
        "_CALIBRATION_NOISE_PROFILE_FS_HDR",
        "_CALIBRATION_NOISE_PROFILE_NATIVE",
        "_CALIBRATION_NOISE_PROFILE_FS_LIN",
        "_CALIBRATION_NOISE_PROFILE_STOP",
        "_CALIBRATION_DEMOSAIC_LINEAR",
        "_CALIBRATION_DEMOSAIC_FS_HDR",
        "_CALIBRATION_DEMOSAIC_NATIVE",
        "_CALIBRATION_DEMOSAIC_FS_LIN",
        "_CALIBRATION_DEMOSAIC_STOP",
        "_CALIBRATION_GAMMA_LINEAR",
        "_CALIBRATION_GAMMA_FS_HDR",
        "_CALIBRATION_GAMMA_NATIVE",
        "_CALIBRATION_GAMMA_FS_LIN",
        "_CALIBRATION_GAMMA_STOP",
        "_CALIBRATION_GAMMA_BE_0_LINEAR",
        "_CALIBRATION_GAMMA_BE_0_FS_HDR",
        "_CALIBRATION_GAMMA_BE_0_NATIVE",
        "_CALIBRATION_GAMMA_BE_0_FS_LIN",
        "_CALIBRATION_GAMMA_BE_0_STOP",
        "_CALIBRATION_GAMMA_BE_1_LINEAR",
        "_CALIBRATION_GAMMA_BE_1_FS_HDR",
        "_CALIBRATION_GAMMA_BE_1_NATIVE",
        "_CALIBRATION_GAMMA_BE_1_FS_LIN",
        "_CALIBRATION_GAMMA_BE_1_STOP",
        "_CALIBRATION_GAMMA_FE_0_LINEAR",
        "_CALIBRATION_GAMMA_FE_0_FS_HDR",
        "_CALIBRATION_GAMMA_FE_0_NATIVE",
        "_CALIBRATION_GAMMA_FE_0_FS_LIN",
        "_CALIBRATION_GAMMA_FE_0_STOP",
        "_CALIBRATION_GAMMA_FE_1_LINEAR",
        "_CALIBRATION_GAMMA_FE_1_FS_HDR",
        "_CALIBRATION_GAMMA_FE_1_NATIVE",
        "_CALIBRATION_GAMMA_FE_1_FS_LIN",
        "_CALIBRATION_GAMMA_FE_1_STOP",
        "_CALIBRATION_IRIDIX_EV_LIM_NO_STR_LINEAR",
        "_CALIBRATION_IRIDIX_EV_LIM_NO_STR_FS_HDR",
        "_CALIBRATION_IRIDIX_EV_LIM_NO_STR_NATIVE",
        "_CALIBRATION_IRIDIX_EV_LIM_NO_STR_FS_LIN",
        "_CALIBRATION_IRIDIX_EV_LIM_NO_STR_STOP",
        "_CALIBRATION_AE_CORRECTION_LINEAR",
        "_CALIBRATION_AE_CORRECTION_FS_HDR",
        "_CALIBRATION_AE_CORRECTION_NATIVE",
        "_CALIBRATION_AE_CORRECTION_FS_LIN",
        "_CALIBRATION_AE_CORRECTION_STOP",
        "_CALIBRATION_SINTER_STRENGTH_LINEAR",
        "_CALIBRATION_SINTER_STRENGTH_FS_HDR",
        "_CALIBRATION_SINTER_STRENGTH_NATIVE",
        "_CALIBRATION_SINTER_STRENGTH_FS_LIN",
        "_CALIBRATION_SINTER_STRENGTH_STOP",
        "_CALIBRATION_SINTER_STRENGTH1_LINEAR",
        "_CALIBRATION_SINTER_STRENGTH1_FS_HDR",
        "_CALIBRATION_SINTER_STRENGTH1_NATIVE",
        "_CALIBRATION_SINTER_STRENGTH1_FS_LIN",
        "_CALIBRATION_SINTER_STRENGTH1_STOP",
        "_CALIBRATION_SINTER_THRESH1_LINEAR",
        "_CALIBRATION_SINTER_THRESH1_FS_HDR",
        "_CALIBRATION_SINTER_THRESH1_NATIVE",
        "_CALIBRATION_SINTER_THRESH1_FS_LIN",
        "_CALIBRATION_SINTER_THRESH1_STOP",
        "_CALIBRATION_SINTER_THRESH4_LINEAR",
        "_CALIBRATION_SINTER_THRESH4_FS_HDR",
        "_CALIBRATION_SINTER_THRESH4_NATIVE",
        "_CALIBRATION_SINTER_THRESH4_FS_LIN",
        "_CALIBRATION_SINTER_THRESH4_STOP",
        "_CALIBRATION_SHARP_ALT_D_LINEAR",
        "_CALIBRATION_SHARP_ALT_D_FS_HDR",
        "_CALIBRATION_SHARP_ALT_D_NATIVE",
        "_CALIBRATION_SHARP_ALT_D_FS_LIN",
        "_CALIBRATION_SHARP_ALT_D_STOP",
        "_CALIBRATION_SHARP_ALT_UD_LINEAR",
        "_CALIBRATION_SHARP_ALT_UD_FS_HDR",
        "_CALIBRATION_SHARP_ALT_UD_NATIVE",
        "_CALIBRATION_SHARP_ALT_UD_FS_LIN",
        "_CALIBRATION_SHARP_ALT_UD_STOP",
        "_CALIBRATION_DEMOSAIC_NP_OFFSET_LINEAR",
        "_CALIBRATION_DEMOSAIC_NP_OFFSET_FS_HDR",
        "_CALIBRATION_DEMOSAIC_NP_OFFSET_NATIVE",
        "_CALIBRATION_DEMOSAIC_NP_OFFSET_FS_LIN",
        "_CALIBRATION_DEMOSAIC_NP_OFFSET_STOP",
        "_CALIBRATION_SATURATION_STRENGTH_LINEAR",
        "_CALIBRATION_SATURATION_STRENGTH_FS_HDR",
        "_CALIBRATION_SATURATION_STRENGTH_NATIVE",
        "_CALIBRATION_SATURATION_STRENGTH_FS_LIN",
        "_CALIBRATION_SATURATION_STRENGTH_STOP",
        "_CALIBRATION_IRIDIX_STRENGTH_MAXIMUM_LINEAR",
        "_CALIBRATION_IRIDIX_STRENGTH_MAXIMUM_WDR",
        "_CALIBRATION_IRIDIX_STRENGTH_MAXIMUM - INVALID",
        "_CALIBRATION_IRIDIX_STRENGTH_MAXIMUM - INVALID",
        "_CALIBRATION_IRIDIX_STRENGTH_MAXIMUM_STOP",
        "_CALIBRATION_SINTER_SAD_LINEAR",
        "_CALIBRATION_SINTER_SAD_WDR",
        "_CALIBRATION_SINTER_SAD - INVALID",
        "_CALIBRATION_SINTER_SAD - INVALID",
        "_CALIBRATION_SINTER_SAD_STOP",
        "_CALIBRATION_SHARPEN_FR_LINEAR",
        "_CALIBRATION_SHARPEN_FR_WDR",
        "_CALIBRATION_SHARPEN_FR - INVALID",
        "_CALIBRATION_SHARPEN_FR - INVALID",
        "_CALIBRATION_SHARPEN_FR_STOP",
        "_CALIBRATION_SHARPEN_DS1_LINEAR",
        "_CALIBRATION_SHARPEN_DS1_WDR",
        "_CALIBRATION_SHARPEN_DS1 - INVALID",
        "_CALIBRATION_SHARPEN_DS1 - INVALID",
        "_CALIBRATION_SHARPEN_DS1_STOP",
        "_CALIBRATION_SHARPEN_DS2_LINEAR",
        "_CALIBRATION_SHARPEN_DS2_WDR",
        "_CALIBRATION_SHARPEN_DS2 - INVALID",
        "_CALIBRATION_SHARPEN_DS2 - INVALID",
        "_CALIBRATION_SHARPEN_DS2_STOP",
        "_CALIBRATION_BLACK_LEVEL_R_LINEAR",
        "_CALIBRATION_BLACK_LEVEL_R_FS_HDR",
        "_CALIBRATION_BLACK_LEVEL_R_NATIVE",
        "_CALIBRATION_BLACK_LEVEL_R_FS_LIN",
        "_CALIBRATION_BLACK_LEVEL_R_STOP",
        "_CALIBRATION_BLACK_LEVEL_B_LINEAR",
        "_CALIBRATION_BLACK_LEVEL_B_FS_HDR",
        "_CALIBRATION_BLACK_LEVEL_B_NATIVE",
        "_CALIBRATION_BLACK_LEVEL_B_FS_LIN",
        "_CALIBRATION_BLACK_LEVEL_B_STOP",
        "_CALIBRATION_BLACK_LEVEL_GB_LINEAR",
        "_CALIBRATION_BLACK_LEVEL_GB_FS_HDR",
        "_CALIBRATION_BLACK_LEVEL_GB_NATIVE",
        "_CALIBRATION_BLACK_LEVEL_GB_FS_LIN",
        "_CALIBRATION_BLACK_LEVEL_GB_STOP",
        "_CALIBRATION_BLACK_LEVEL_GR_LINEAR",
        "_CALIBRATION_BLACK_LEVEL_GR_FS_HDR",
        "_CALIBRATION_BLACK_LEVEL_GR_NATIVE",
        "_CALIBRATION_BLACK_LEVEL_GR_FS_LIN",
        "_CALIBRATION_BLACK_LEVEL_GR_STOP",
        "_CALIBRATION_DP_SLOPE_LINEAR",
        "_CALIBRATION_DP_SLOPE_FS_HDR",
        "_CALIBRATION_DP_SLOPE_NATIVE",
        "_CALIBRATION_DP_SLOPE_FS_LIN",
        "_CALIBRATION_DP_SLOPE_FS_STOP",
        "_CALIBRATION_DP_THRESHOLD_LINEAR",
        "_CALIBRATION_DP_THRESHOLD_FS_HDR",
        "_CALIBRATION_DP_THRESHOLD_NATIVE",
        "_CALIBRATION_DP_THRESHOLD_FS_LIN",
        "_CALIBRATION_DP_THRESHOLD_FS_STOP",
        "_CALIBRATION_AE_BALANCED_LINEAR",
        "_CALIBRATION_AE_BALANCED_WDR",
        "_CALIBRATION_AE_BALANCED - INVALID",
        "_CALIBRATION_AE_BALANCED - INVALID",
        "_CALIBRATION_AE_BALANCED_STOP",
        "_CALIBRATION_EVTOLUX_EV_LUT_LINEAR",
        "_CALIBRATION_EVTOLUX_EV_LUT_FS_HDR",
        "_CALIBRATION_EVTOLUX_EV_LUT_NATIVE",
        "_CALIBRATION_EVTOLUX_EV_LUT_FS_LIN",
        "_CALIBRATION_EVTOLUX_EV_LUT_STOP",
        "_CALIBRATION_STITCHING_ERROR_THRESH",
        "_CALIBRATION_MESH_SHADING_STRENGTH",
        "_CALIBRATION_IRIDIX_RGB2REC709",
        "_CALIBRATION_IRIDIX_REC709TORGB",
        "_CALIBRATION_TEMPER_STRENGTH",
        "_CALIBRATION_AE_EXPOSURE_CORRECTION",
        "_CALIBRATION_IRIDIX_ASYMMETRY",
        "_CALIBRATION_IRIDIX_BLACK_PRC",
        "_CALIBRATION_IRIDIX_GAIN_MAX",
        "_CALIBRATION_IRIDIX_MIN_MAX_STR",
        "_CALIBRATION_IRIDIX_EV_LIM_FULL_STR",
        "_CALIBRATION_LUXLOW_LUT",
        "_CALIBRATION_MT_ABSOLUTE_LS_A_CCM_LINEAR",
        "_CALIBRATION_MESH_COLOR_TEMPERATURE",
        "_CALIBRATION_RGHIGH_LUT_MIN",
        "_CALIBRATION_CT30POS",
        "_CALIBRATION_MESH_LS_WEIGHT",
        "_CALIBRATION_AWB_WARMING_LS_D50",
        "_CALIBRATION_MT_ABSOLUTE_LS_D50_CCM_LINEAR",
        "_CALIBRATION_MT_ABSOLUTE_LS_D40_CCM_LINEAR",
        "_CALIBRATION_CT65POS",
        "_CALIBRATION_AUTO_WB",
        "_CALIBRATION_SHADING_LS_TL84_G",
        "_CALIBRATION_AWB_WARMING_LS_A",
        "_CALIBRATION_SHADING_LS_A_G",
        "_CALIBRATION_CT_RG_POS_CALC",
        "_CALIBRATION_SHADING_LS_D65_R",
        "_CALIBRATION_RGHIGH_LUT_MAX",
        "_CALIBRATION_RG_POS",
        "_CALIBRATION_LUXLOW_LUT_STEP",
        "_CALIBRATION_CT_BG_POS_CALC",
        "_CALIBRATION_SHADING_LS_D65_B",
        "_CALIBRATION_RGLOW_LUT_STEP",
        "_CALIBRATION_WB_STRENGTH",
        "_CALIBRATION_LUXLOW_LUT_MIN",
        "_CALIBRATION_SKIN_TONE_CCM",
        "_CALIBRATION_STATIC_WB",
        "_CALIBRATION_AWB_WARMING_LS_D40",
        "_CALIBRATION_COLOR_TEMP",
        "_CALIBRATION_RGLOW_LUT_MIN",
        "_CALIBRATION_MT_ABSOLUTE_LS_D40_CCM_HDR",
        "_CALIBRATION_MESH_RGBG_WEIGHT",
        "_CALIBRATION_LIGHT_SRC",
        "_CALIBRATION_RGLOW_LUT_MAX",
        "_CALIBRATION_SHADING_LS_A_R",
        "_CALIBRATION_CT40POS",
        "_CALIBRATION_MT_ABSOLUTE_LS_A_CCM_HDR",
        "_CALIBRATION_EVTOLUX_LUX_LUT",
        "_CALIBRATION_LUXHIGH_LUT_STEP",
        "_CALIBRATION_SHADING_LS_TL84_B",
        "_CALIBRATION_SHADING_LS_D65_G",
        "_CALIBRATION_RGHIGH_LUT",
        "_CALIBRATION_LUXLOW_LUT_MAX",
        "_CALIBRATION_SHADING_LS_A_B",
        "_CALIBRATION_SKY_LUX_TH",
        "_CALIBRATION_RGLOW_LUT",
        "_CALIBRATION_MT_ABSOLUTE_LS_D50_CCM_HDR",
        "_CALIBRATION_LUXHIGH_LUT_MIN",
        "_CALIBRATION_BG_POS",
        "_CALIBRATION_LUXHIGH_LUT_MAX",
        "_CALIBRATION_SHADING_LS_TL84_R",
        "_CALIBRATION_LUXHIGH_LUT",
        "_CALIBRATION_RGHIGH_LUT_STEP",
        "_CALIBRATION_AWB_SCENE_PRESETS",
        "_CALIBRATION_CNR_UV_DELTA12_SLOPE",
        "_CALIBRATION_AF_MIN_TABLE",
        "_CALIBRATION_AF_MAX_TABLE",
        "_CALIBRATION_AF_WINDOW_RESIZE_TABLE",
        "_CALIBRATION_EXP_RATIO_TABLE",
        "_CALIBRATION_IRIDIX_STRENGTH_TABLE",
        "_CALIBRATION_AWB_AVG_COEF",
        "_CALIBRATION_IRIDIX_AVG_COEF",
        "_CALIBRATION_AE_EXPOSURE_AVG_COEF",
        "_CALIBRATION_CCM_ONE_GAIN_THRESHOLD",
        "_CALIBRATION_NP_LUT_MEAN",
        "_CALIBRATION_EVTOLUX_PROBABILITY_ENABLE",
        "_CALIBRATION_DEFECT_PIXELS",
        "_CALIBRATION_FLASH_RG",
        "_CALIBRATION_FLASH_BG",
        "_CALIBRATION_AF_LMS",
        "_CALIBRATION_SINTER_RADIAL_LUT",
        "_CALIBRATION_SINTER_RADIAL_PARAMS",
        "_CALIBRATION_GDC_CONFIG",
        "_CALIBRATION_TOTAL_SIZE"
    } ;
#endif



int32_t init_isp_set( uint32_t c_set )
{
    int32_t result = 0 ;
    int32_t dynamic_set_num = sizeof( dynamic_calibrations_func ) / sizeof( dynamic_calibrations_func[0] ) ;
    int32_t static_set_num = sizeof( static_calibrations_func ) / sizeof( static_calibrations_func[0] ) ;
    LOG( LOG_DEBUG, "Switching calibrations for set %d ", (int)c_set ) ;
    if ( c_set < dynamic_set_num && c_set < static_set_num ) {
        // initialize default parameters
        result = apical_calibrations_init( &apicalCalibrations ) ;
        if ( result == 0 ) {
            // apply a current isp set
            result = dynamic_calibrations_func[ c_set ]( &apicalCalibrations ) ;
            if ( result == 0 ) {
                result = static_calibrations_func[ c_set ]( &apicalCalibrations ) ;
                if ( result == 0 ) {
                } else {
                    LOG( LOG_CRIT, "Failed to get static calibration set. Fatal error" ) ;
                }
            } else {
                LOG( LOG_CRIT, "Failed to get dynamic calibration set. Fatal error" ) ;
            }
        } else {
            LOG( LOG_CRIT, "Failed to initialize an dynamic calibrations. Fatal error" ) ;
        }

    } else {
        LOG( LOG_CRIT, "Failed to find calibrations. Unsupported %d set required", (int)c_set ) ;
    }
    return result ;
}


uint32_t preview_set_supported( void )
{
    return CALIBRATION_PREVIEW_SET_SUPPORT ;
}


ApicalCalibrations* get_current_set()
{
    return &apicalCalibrations ;
}


LookupTable* _GET_LOOKUP_PTR( uint32_t idx ) {
  LookupTable* result = NULL ;
  if ( idx < _CALIBRATION_TOTAL_SIZE ) {
    result = get_current_set()->calibrations[ idx ] ;
  } else {
    result = NULL ;
    LOG( LOG_CRIT, "Trying to access an isp lut with invalid index %d", (int)idx ) ;
  }
  return result ;
}


#if RESTRICTED_SOURCES
const void* _GET_LUT_PTR( uint32_t idx ) {
  const void* result = NULL ;
  LookupTable *lut = _GET_LOOKUP_PTR( idx ) ;
  if ( lut != NULL ) {
    result = lut->ptr ;
  } else {
    while ( 1 ) {
      LOG( LOG_CRIT, "LUT %d is not initialized. Pointer is NULL. Going to the infinite loop", (int)idx ) ;
    } ;
  }
  return result ;
}

// use fast version of lut access routines
inline uint8_t* _GET_UCHAR_PTR( uint32_t idx ) {
  uint8_t* result = (uint8_t *) _GET_LUT_PTR( idx ) ;
  return result ;
}

inline uint16_t* _GET_USHORT_PTR( uint32_t idx ) {
  uint16_t* result = (uint16_t *) _GET_LUT_PTR( idx ) ;
  return result ;
}

inline uint32_t* _GET_UINT_PTR( uint32_t idx ) {
  uint32_t* result = (uint32_t *) _GET_LUT_PTR( idx ) ;
  return result ;
}

#else
// size control version
const void* _GET_LUT_PTR( uint32_t idx ) {
  const void* result = NULL ;
  LookupTable *lut = _GET_LOOKUP_PTR( idx ) ;
  if ( lut != NULL ) {
    result = lut->ptr ;
  } else {
    while ( 1 ) {
      LOG( LOG_CRIT, "LUT %d - %s is not initialized. Pointer is NULL. Going to the infinite loop", (int)idx, s_calibrationID[ idx ] ) ;
    } ;
  }
  return result ;
}

inline uint8_t* _GET_UCHAR_PTR( uint32_t idx ) {
  LookupTable *lut =  _GET_LOOKUP_PTR( idx ) ;
  if ( lut != NULL ) {
    if ( lut->width != sizeof( uint8_t ) ) {
      while ( 1 ) { LOG( LOG_CRIT, "Trying to access %d byte lut as one byte.", (int)lut->width ) ; }
    }
  }
  uint8_t* result = (uint8_t *) _GET_LUT_PTR( idx ) ;
  return result ;
}

inline uint16_t* _GET_USHORT_PTR( uint32_t idx ) {
  LookupTable *lut =  _GET_LOOKUP_PTR( idx ) ;
  if ( lut != NULL ) {
    if ( lut->width != sizeof( uint16_t ) ) {
      while ( 1 ) { LOG( LOG_CRIT, "Trying to access %d byte lut as two bytes.", (int)lut->width ) ; }
    }
  }
  uint16_t* result = (uint16_t *) _GET_LUT_PTR( idx ) ;
  return result ;
}

inline uint32_t* _GET_UINT_PTR( uint32_t idx ) {
  LookupTable *lut =  _GET_LOOKUP_PTR( idx ) ;
  if ( lut != NULL ) {
    if ( lut->width != sizeof( uint32_t ) ) {
      while ( 1 ) { LOG( LOG_CRIT, "Trying to access %d byte lut as four bytes.", (int)lut->width ) ; }
    }
  }
  uint32_t* result = (uint32_t *) _GET_LUT_PTR( idx ) ;
  return result ;
}
#endif

inline modulation_entry_t* _GET_MOD_ENTRY16_PTR( uint32_t idx ) {
  modulation_entry_t* result = (modulation_entry_t *) _GET_LUT_PTR( idx ) ;
  return result ;
}

inline modulation_entry_32_t* _GET_MOD_ENTRY32_PTR( uint32_t idx ) {
  modulation_entry_32_t* result = (modulation_entry_32_t *) _GET_LUT_PTR( idx ) ;
  return result ;
}

inline uint32_t _GET_ROWS( uint32_t idx ) {
  uint32_t result = 0 ;
  LookupTable *lut = _GET_LOOKUP_PTR( idx ) ;
  if ( lut != NULL ) {
     result = lut->rows ;
  }
  return result ;
}

inline uint32_t _GET_COLS( uint32_t idx ) {
  uint32_t result = 0 ;
  LookupTable *lut = _GET_LOOKUP_PTR( idx ) ;
  if ( lut != NULL ) {
     result = lut->cols ;
  }
  return result ;
}

inline uint32_t _GET_LEN( uint32_t idx ) {
  uint32_t result = 0 ;
  LookupTable *lut = _GET_LOOKUP_PTR( idx ) ;
  if ( lut != NULL ) {
     result = lut->cols * lut->rows ;
  }
  return result ;
}

inline uint32_t _GET_WIDTH( uint32_t idx ) {
  uint32_t result = 0 ;
  LookupTable *lut = _GET_LOOKUP_PTR( idx ) ;
  if ( lut != NULL ) {
     result = lut->width ;
  }
  return result ;
}

inline uint32_t _GET_SIZE( uint32_t idx ) {
  uint32_t result = 0 ;
  LookupTable *lut = _GET_LOOKUP_PTR( idx ) ;
  if ( lut != NULL ) {
     result = lut->cols * lut->rows * lut->width ;
  }
  return result ;
}

int32_t _GET_HDR_TABLE_INDEX( uint32_t start_idx, uint32_t mode ) {
    uint32_t result = start_idx ;
    // This function returns an index on a hdr lut table for the current wdr mode.
    // There are two possible scenarious. First one when we should have a separate
    // lut for every of four wdr modes i.e. for linear, fs_lin, native and fs_hdr.
    // A second scenario when we have one lut for linear mode and one lut for all
    // other modes. It is the main reason why we need this function. It should
    // distinguish luts by start_idx parameter and return a valid index for the current
    // wdr mode.
    if ( start_idx < _CALIBRATION_TOTAL_SIZE ) {
      switch ( start_idx ) {
      case _CALIBRATION_SINTER_SAD_START :
      case _CALIBRATION_IRIDIX_STRENGTH_MAXIMUM_START :
      case _CALIBRATION_SHARPEN_FR_START :
      case _CALIBRATION_SHARPEN_DS1_START :
      case _CALIBRATION_SHARPEN_DS2_START :
      case _CALIBRATION_AE_BALANCED_START :
      case _CALIBRATION_SHADING_LS_A_R_START :
      case _CALIBRATION_SHADING_LS_A_G_START :
      case _CALIBRATION_SHADING_LS_A_B_START :
      case _CALIBRATION_SHADING_LS_TL84_R_START :
      case _CALIBRATION_SHADING_LS_TL84_G_START :
      case _CALIBRATION_SHADING_LS_TL84_B_START :
      case _CALIBRATION_SHADING_LS_D65_R_START :
      case _CALIBRATION_SHADING_LS_D65_G_START :
      case _CALIBRATION_SHADING_LS_D65_B_START :
        // we have only two luts for linear and all wdr modes.
        if ( mode == WDR_MODE_LINEAR ) {
          result = start_idx + WDR_MODE_LINEAR ;
        } else {
          result = start_idx + WDR_MODE_POSITION ;
        }
        break ;
      default :
        result = start_idx + mode ;
        break ;
      }
    } else {
      result = -1 ;
    }

    return result ;
}

