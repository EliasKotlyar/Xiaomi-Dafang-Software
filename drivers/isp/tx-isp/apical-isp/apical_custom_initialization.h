/*-----------------------------------------------------------------------------
  This confidential and proprietary software/information may be used only
  as authorized by a licensing agreement from Apical Limited

  (C) COPYRIGHT 2011 - 2015 Apical Limited
  ALL RIGHTS RESERVED

  The entire notice above must be reproduced on all authorized
  copies and copies may only be made to the extent permitted
  by a licensing agreement from Apical Limited.
  -----------------------------------------------------------------------------*/

#ifndef __APICAL_CUSTOM_INITIALIZATION_H__
#define __APICAL_CUSTOM_INITIALIZATION_H__
#include <apical-isp/apical_types.h>

/**
 *   Make a customer related initialization of Apical ISP
 *
 *   This function is called at the end of firmware initialization and
 *   before any processing is being started.
 *   You can update this function to make any initialization you need.
 *
 */
void apical_custom_initialization(void) ;

/**
 *   Return Apical ISP initialization sequence
 *
 *   This function is called at initialization stage
 *   before any processing is being started.
 *   Customer has to provide a valid pointer to initialization sequence or
 *   just return NULL pointer if no initialization should be made by firmware.
 *
 */
uint8_t* apical_custom_sequence(void) ;

#endif // __APICAL_CUSTOM_INITIALIZATION_H__
