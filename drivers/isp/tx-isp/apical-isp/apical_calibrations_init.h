/*-----------------------------------------------------------------------------
  This confidential and proprietary software/information may be used only
  as authorized by a licensing agreement from Apical Limited

  (C) COPYRIGHT 2011 - 2015 Apical Limited
  ALL RIGHTS RESERVED

  The entire notice above must be reproduced on all authorized
  copies and copies may only be made to the extent permitted
  by a licensing agreement from Apical Limited.
  -----------------------------------------------------------------------------*/

#ifndef __APICAL_CALIBRATIONS_INIT_H__
#define __APICAL_CALIBRATIONS_INIT_H__

#include <apical-isp/apical_calibrations_id.h>


/**
 *   Initialize a current set of settings for apical ISP.
 *
 *   This function MUST be called before applying a new set of ISP LUTS.
 *   It initializes a memory by default LUTs values.
 *
 *   This LUTs may be initialized to NULL - it means that you have to set
 *   corresponding values additionaly.
 *
 *   Also some default LUTs may be set - it means that it is not neccessary
 *   to update them later.
 *
 *   @param ispSet - a pointer to ispCalibrations structure
 *
 *   @return SUCCESS - everything has been initialized properly
 *           FAIL - failed initialization. Firmware cannot continue to run
 */
int32_t apical_calibrations_init( ApicalCalibrations *ispSet ) ;


#endif /* __APICAL_CALIBRATIONS_INIT_H__ */
