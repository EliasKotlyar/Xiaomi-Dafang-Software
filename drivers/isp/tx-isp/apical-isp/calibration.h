/*-----------------------------------------------------------------------------
  This confidential and proprietary software/information may be used only
  as authorized by a licensing agreement from Apical Limited

  (C) COPYRIGHT 2011 - 2015 Apical Limited
  ALL RIGHTS RESERVED

  The entire notice above must be reproduced on all authorized
  copies and copies may only be made to the extent permitted
  by a licensing agreement from Apical Limited.
  -----------------------------------------------------------------------------*/

#ifndef __CALIBRATION_H__
#define __CALIBRATION_H__

#define PACK_DATE(y, m, d) (((((y)-2000) << 9)+(((m) & 0xF) << 5)+((d) & 0x1F)) & 0xFFFF)

#endif // __CALIBRATION_H__
