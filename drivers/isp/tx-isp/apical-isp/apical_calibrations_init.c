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
#include "log.h"




int32_t apical_calibrations_init( ApicalCalibrations *c )
{
	int32_t result = 0 ;
	if ( c != 0 ) {
		// first of all we have to guaruantee that ECalibrationID enum has unique numbers

		// by default initialize the whole memory by zeros
		// some pointers will remain NULL and they
		// don't have to be used in firmware
		memset( (void *)c, 0, sizeof( ApicalCalibrations ) ) ;

		// set default set of ISP parameters for firmware.
		// They can be changed later if neccessary.
		// Please note that LUTs which will stay NULL MUST be
		// initialized later before firmware runs

	} else {
		result = -1 ;
	}
	return result ;
}
