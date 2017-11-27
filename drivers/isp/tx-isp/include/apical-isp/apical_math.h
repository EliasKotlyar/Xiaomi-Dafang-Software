/*-----------------------------------------------------------------------------
  This confidential and proprietary software/information may be used only
  as authorized by a licensing agreement from Apical Limited

  (C) COPYRIGHT 2011 - 2015 Apical Limited
  ALL RIGHTS RESERVED

  The entire notice above must be reproduced on all authorized
  copies and copies may only be made to the extent permitted
  by a licensing agreement from Apical Limited.
  -----------------------------------------------------------------------------*/

#ifndef __APICAL_MATH_H__
#define __APICAL_MATH_H__

#include "apical.h"

#define APICAL_ABS(a)        ((a)>=0?(a):-(a))

#define U16_MAX 					0xFFFF
#define APICAL_SIGN(a)  ((a)>=0?(1):(-1))
#define APICAL_MIN(a,b) ((a)>=b?(b):(a))
#define APICAL_MAX(a,b) ((a)>=b?(a):(b))
#define APICAL_ABSDIFF(a,b) ((a)>(b)? (a-b) : (b-a))
#define LIN_EQUATION_FRACTION_SIZE 5

#define round_shift(a,sh)   (((a)>>(sh))+(((a)>>(sh-1))&1))
#define PI   12868 //Q12 Format
uint8_t leading_one_position(const uint32_t in);
uint16_t sqrt32(uint32_t arg);
uint32_t log2_int_to_fixed(const uint32_t val, const uint8_t out_precision, const uint8_t shift_out);
uint32_t log2_fixed_to_fixed(const uint32_t val, const int in_fix_point, const uint8_t out_fix_point);
uint32_t math_exp2(uint32_t val, const unsigned char shift_in, const unsigned char shift_out);
uint8_t sqrt16(uint16_t arg);
uint8_t log16(uint16_t arg);
uint32_t math_log2(const uint32_t val, const uint8_t out_precision, const uint8_t shift_out);
uint32_t multiplication_fixed_to_fixed(uint32_t a, uint32_t b, const int x1, const int x2);

int32_t solving_lin_equation_a(int32_t y1, int32_t y2, int32_t x1, int32_t x2, int16_t a_fraction_size);
int32_t solving_lin_equation_b(int32_t y1, int32_t a, int32_t x1, int16_t a_fraction_size);
int32_t solving_nth_root_045(int32_t x, const int16_t fraction_size);
uint32_t div_fixed(uint32_t a, uint32_t b, int16_t a_fraction_size);
uint16_t sqrt32(uint32_t arg);

uint16_t line_offset(uint16_t line_len, uint8_t bytes_per_pixel);
int16_t apical_cosine(uint32_t theta);
int16_t apical_sine(uint32_t theta);

#define APICAL_MODULO(N,D)  ((N)-(((N)/(D))*(D)))
#endif /* __APICAL_MATH_H__ */
