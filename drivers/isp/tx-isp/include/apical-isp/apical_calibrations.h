/*-----------------------------------------------------------------------------
  This confidential and proprietary software/information may be used only
  as authorized by a licensing agreement from Apical Limited

  (C) COPYRIGHT 2011 - 2015 Apical Limited
  ALL RIGHTS RESERVED

  The entire notice above must be reproduced on all authorized
  copies and copies may only be made to the extent permitted
  by a licensing agreement from Apical Limited.
  -----------------------------------------------------------------------------*/

#ifndef __APICAL_CALIBRATIONS_H__
#define __APICAL_CALIBRATIONS_H__

//#include "log.h"
#include "apical_types.h"
#include "apical_calibrations_id.h"


#define CALIBRATION_BASE_SET 0
#define CALIBRATION_PREVIEW_SET 1

/**
 *   Initialize a current set of settings for apical ISP.
 *
 *   This function MUST be called before any access to ISP LUTS.
 *   It initializes a pointer on a structure with ISP settings.
 *   You can update this function to use any predefined set you need.
 *
 *   @param c_set - which set of calibrations to use base or preview.
 *
 *   @return SUCCESS - pointer on ISP settings has been initialized properly
 *           FAIL - failed initialization. Firmware cannot continue to run
 */
int32_t init_isp_set( uint32_t c_set ) ;


/**
 *   Returns 1 if the preview set is available
 *
 *   A firmware build can support a specific set of calibrations for each resolution.
 *   This function returns true if there are two calibrations sets
 *   which are supported by firmware.
 *   For example if returned value is 0 this mean that only CALIBRATION_BASE_SET can be used for any resolution
 *   If firmware supports more than one set then defines CALIBRATION_BASE_SET should be used
 *   for full resolution picture and CALIBRATION_PREVIEW_SET should be used for preview resolution.
 *
 *
 *   @return 1 - preview set is supported by firmware
 *           0 - only base set is available for all resolutions
 */
uint32_t preview_set_supported( void ) ;


/**
 *   Get current settings for apical ISP.
 *
 *   This function returns a pointer on current set of ISP settings. Apical ISP library uses
 *   this function each time it needs an access to predefined ISP LUT.
 *   You can update a returned pointer to change ISP settings in real-time.
 *
 *   @return a valid pointer on ISP settings
 */
ApicalCalibrations* get_current_set( void ) ;


LookupTable* _GET_LOOKUP_PTR( uint32_t idx ) ;

const void* _GET_LUT_PTR( uint32_t idx ) ;

uint8_t* _GET_UCHAR_PTR( uint32_t idx ) ;

uint16_t* _GET_USHORT_PTR( uint32_t idx ) ;

uint32_t* _GET_UINT_PTR( uint32_t idx ) ;

modulation_entry_t* _GET_MOD_ENTRY16_PTR( uint32_t idx ) ;

modulation_entry_32_t* _GET_MOD_ENTRY32_PTR( uint32_t idx ) ;

uint32_t _GET_ROWS( uint32_t idx ) ;

uint32_t _GET_COLS( uint32_t idx ) ;

uint32_t _GET_LEN( uint32_t idx ) ;

uint32_t _GET_WIDTH( uint32_t idx ) ;

uint32_t _GET_SIZE( uint32_t idx ) ;


/**
 *   Get a current settings index for hdr lut table.
 *
 *   This function returns an index on a hdr lut table for the current wdr mode.
 *   There are two possible scenarious. First one when we should have a separate
 *   lut for every of four wdr modes i.e. for linear, fs_lin, native and fs_hdr.
 *   A second scenario when we have one lut for linear mode and one lut for all
 *   other modes. It is the main reason why we need this function. It should
 *   distinguish luts by start_idx parameter and return a valid index for the current
 *   wdr mode.
 *
 *   @param start_idx - a start index of the desired lut table from ECalibrationID
 *                      see apical_calibrations.h
 *   @param mode - a hdr mode you would like to get a lut for. Must be one from EWDRModeID
 *                 see apical_calibrations.h
 *
 *   @return  non zero value - a valid LUT index in calibrations array
 *            -1 - if desired lut doesn't exist
 */
int32_t _GET_HDR_TABLE_INDEX( uint32_t start_idx, uint32_t mode ) ;


#endif // __APICAL_CALIBRATIONS_H__
