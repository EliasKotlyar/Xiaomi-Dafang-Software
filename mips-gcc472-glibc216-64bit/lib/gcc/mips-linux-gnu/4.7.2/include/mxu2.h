/* MIPS MXU2 intrinsics include file.

   Copyright (C) 2006 Ingenic Semiconductor CO.,LTD.
   Contributed by Qian Liu, qian.liu@ingenic.com.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3, or (at your
   option) any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef _MXU2_H
#define _MXU2_H 1

#if defined(__mips_mxu2)
typedef signed char v16i8 __attribute__((vector_size(16), aligned(16)));
typedef unsigned char v16u8 __attribute__((vector_size(16), aligned(16)));
typedef short v8i16 __attribute__((vector_size(16), aligned(16)));
typedef unsigned short v8u16 __attribute__((vector_size(16), aligned(16)));
typedef int v4i32 __attribute__((vector_size(16), aligned(16)));
typedef unsigned int v4u32 __attribute__((vector_size(16), aligned(16)));
typedef long long v2i64 __attribute__((vector_size(16), aligned(16)));
typedef unsigned long long v2u64 __attribute__((vector_size(16), aligned(16)));
typedef float v4f32 __attribute__((vector_size(16), aligned(16)));
typedef double v2f64 __attribute__ ((vector_size(16), aligned(16)));

/* The Intel API is flexible enough that we must allow aliasing with other
   vector types, and their scalar components.  */
typedef long long __m128i __attribute__ ((__vector_size__ (16), __may_alias__));
typedef double __m128d __attribute__ ((__vector_size__ (16), __may_alias__));
typedef float __m128 __attribute__ ((__vector_size__ (16), __may_alias__));



#ifndef __clang__
extern int __builtin_mxu2_bnez1q(v16u8);
#define _mx128_bnez1q __builtin_mxu2_bnez1q
extern int __builtin_mxu2_bnez16b(v16u8);
#define _mx128_bnez16b __builtin_mxu2_bnez16b
extern int __builtin_mxu2_bnez8h(v8u16);
#define _mx128_bnez8h __builtin_mxu2_bnez8h
extern int __builtin_mxu2_bnez4w(v4u32);
#define _mx128_bnez4w __builtin_mxu2_bnez4w
extern int __builtin_mxu2_bnez2d(v2u64);
#define _mx128_bnez2d __builtin_mxu2_bnez2d
extern int __builtin_mxu2_beqz1q(v16u8);
#define _mx128_beqz1q __builtin_mxu2_beqz1q
extern int __builtin_mxu2_beqz16b(v16u8);
#define _mx128_beqz16b __builtin_mxu2_beqz16b
extern int __builtin_mxu2_beqz8h(v8u16);
#define _mx128_beqz8h __builtin_mxu2_beqz8h
extern int __builtin_mxu2_beqz4w(v4u32);
#define _mx128_beqz4w __builtin_mxu2_beqz4w
extern int __builtin_mxu2_beqz2d(v2u64);
#define _mx128_beqz2d __builtin_mxu2_beqz2d
/* Compare */
extern v16i8 __builtin_mxu2_ceq_b(v16i8, v16i8);
#define _mx128_ceq_b __builtin_mxu2_ceq_b
extern v8i16 __builtin_mxu2_ceq_h(v8i16, v8i16);
#define _mx128_ceq_h __builtin_mxu2_ceq_h
extern v4i32 __builtin_mxu2_ceq_w(v4i32, v4i32);
#define _mx128_ceq_w __builtin_mxu2_ceq_w
extern v2i64 __builtin_mxu2_ceq_d(v2i64, v2i64);
#define _mx128_ceq_d __builtin_mxu2_ceq_d
extern v16i8 __builtin_mxu2_ceqz_b(v16i8);
#define _mx128_ceqz_b __builtin_mxu2_ceqz_b
extern v8i16 __builtin_mxu2_ceqz_h(v8i16);
#define _mx128_ceqz_h __builtin_mxu2_ceqz_h
extern v4i32 __builtin_mxu2_ceqz_w(v4i32);
#define _mx128_ceqz_w __builtin_mxu2_ceqz_w
extern v2i64 __builtin_mxu2_ceqz_d(v2i64);
#define _mx128_ceqz_d __builtin_mxu2_ceqz_d
extern v16i8 __builtin_mxu2_cne_b(v16i8, v16i8);
#define _mx128_cne_b __builtin_mxu2_cne_b
extern v8i16 __builtin_mxu2_cne_h(v8i16, v8i16);
#define _mx128_cne_h __builtin_mxu2_cne_h
extern v4i32 __builtin_mxu2_cne_w(v4i32, v4i32);
#define _mx128_cne_w __builtin_mxu2_cne_w
extern v2i64 __builtin_mxu2_cne_d(v2i64, v2i64);
#define _mx128_cne_d __builtin_mxu2_cne_d
extern v16i8 __builtin_mxu2_cnez_b(v16i8);
#define _mx128_cnez_b __builtin_mxu2_cnez_b
extern v8i16 __builtin_mxu2_cnez_h(v8i16);
#define _mx128_cnez_h __builtin_mxu2_cnez_h
extern v4i32 __builtin_mxu2_cnez_w(v4i32);
#define _mx128_cnez_w __builtin_mxu2_cnez_w
extern v2i64 __builtin_mxu2_cnez_d(v2i64);
#define _mx128_cnez_d __builtin_mxu2_cnez_d
extern v16i8 __builtin_mxu2_cles_b(v16i8, v16i8);
#define _mx128_cles_b __builtin_mxu2_cles_b
extern v8i16 __builtin_mxu2_cles_h(v8i16, v8i16);
#define _mx128_cles_h __builtin_mxu2_cles_h
extern v4i32 __builtin_mxu2_cles_w(v4i32, v4i32);
#define _mx128_cles_w __builtin_mxu2_cles_w
extern v2i64 __builtin_mxu2_cles_d(v2i64, v2i64);
#define _mx128_cles_d __builtin_mxu2_cles_d
extern v16i8 __builtin_mxu2_cleu_b(v16u8, v16u8);
#define _mx128_cleu_b __builtin_mxu2_cleu_b
extern v8i16 __builtin_mxu2_cleu_h(v8u16, v8u16);
#define _mx128_cleu_h __builtin_mxu2_cleu_h
extern v4i32 __builtin_mxu2_cleu_w(v4u32, v4u32);
#define _mx128_cleu_w __builtin_mxu2_cleu_w
extern v2i64 __builtin_mxu2_cleu_d(v2u64, v2u64);
#define _mx128_cleu_d __builtin_mxu2_cleu_d
extern v16i8 __builtin_mxu2_clez_b(v16i8);
#define _mx128_clez_b __builtin_mxu2_clez_b
extern v8i16 __builtin_mxu2_clez_h(v8i16);
#define _mx128_clez_h __builtin_mxu2_clez_h
extern v4i32 __builtin_mxu2_clez_w(v4i32);
#define _mx128_clez_w __builtin_mxu2_clez_w
extern v2i64 __builtin_mxu2_clez_d(v2i64);
#define _mx128_clez_d __builtin_mxu2_clez_d
extern v16i8 __builtin_mxu2_clts_b(v16i8, v16i8);
#define _mx128_clts_b __builtin_mxu2_clts_b
extern v8i16 __builtin_mxu2_clts_h(v8i16, v8i16);
#define _mx128_clts_h __builtin_mxu2_clts_h
extern v4i32 __builtin_mxu2_clts_w(v4i32, v4i32);
#define _mx128_clts_w __builtin_mxu2_clts_w
extern v2i64 __builtin_mxu2_clts_d(v2i64, v2i64);
#define _mx128_clts_d __builtin_mxu2_clts_d
extern v16i8 __builtin_mxu2_cltu_b(v16u8, v16u8);
#define _mx128_cltu_b __builtin_mxu2_cltu_b
extern v8i16 __builtin_mxu2_cltu_h(v8u16, v8u16);
#define _mx128_cltu_h __builtin_mxu2_cltu_h
extern v4i32 __builtin_mxu2_cltu_w(v4u32, v4u32);
#define _mx128_cltu_w __builtin_mxu2_cltu_w
extern v2i64 __builtin_mxu2_cltu_d(v2u64, v2u64);
#define _mx128_cltu_d __builtin_mxu2_cltu_d
extern v16i8 __builtin_mxu2_cltz_b(v16i8);
#define _mx128_cltz_b __builtin_mxu2_cltz_b
extern v8i16 __builtin_mxu2_cltz_h(v8i16);
#define _mx128_cltz_h __builtin_mxu2_cltz_h
extern v4i32 __builtin_mxu2_cltz_w(v4i32);
#define _mx128_cltz_w __builtin_mxu2_cltz_w
extern v2i64 __builtin_mxu2_cltz_d(v2i64);
#define _mx128_cltz_d __builtin_mxu2_cltz_d
/* Integer Arithmetic */
extern v16i8 __builtin_mxu2_adda_b(v16i8, v16i8);
#define _mx128_adda_b __builtin_mxu2_adda_b
extern v8i16 __builtin_mxu2_adda_h(v8i16, v8i16);
#define _mx128_adda_h __builtin_mxu2_adda_h
extern v4i32 __builtin_mxu2_adda_w(v4i32, v4i32);
#define _mx128_adda_w __builtin_mxu2_adda_w
extern v2i64 __builtin_mxu2_adda_d(v2i64, v2i64);
#define _mx128_adda_d __builtin_mxu2_adda_d
extern v16i8 __builtin_mxu2_addas_b(v16i8, v16i8);
#define _mx128_addas_b __builtin_mxu2_addas_b
extern v8i16 __builtin_mxu2_addas_h(v8i16, v8i16);
#define _mx128_addas_h __builtin_mxu2_addas_h
extern v4i32 __builtin_mxu2_addas_w(v4i32, v4i32);
#define _mx128_addas_w __builtin_mxu2_addas_w
extern v2i64 __builtin_mxu2_addas_d(v2i64, v2i64);
#define _mx128_addas_d __builtin_mxu2_addas_d
extern v16i8 __builtin_mxu2_addss_b(v16i8, v16i8);
#define _mx128_addss_b __builtin_mxu2_addss_b
extern v8i16 __builtin_mxu2_addss_h(v8i16, v8i16);
#define _mx128_addss_h __builtin_mxu2_addss_h
extern v4i32 __builtin_mxu2_addss_w(v4i32, v4i32);
#define _mx128_addss_w __builtin_mxu2_addss_w
extern v2i64 __builtin_mxu2_addss_d(v2i64, v2i64);
#define _mx128_addss_d __builtin_mxu2_addss_d
extern v16u8 __builtin_mxu2_adduu_b(v16u8, v16u8);
#define _mx128_adduu_b __builtin_mxu2_adduu_b
extern v8u16 __builtin_mxu2_adduu_h(v8u16, v8u16);
#define _mx128_adduu_h __builtin_mxu2_adduu_h
extern v4u32 __builtin_mxu2_adduu_w(v4u32, v4u32);
#define _mx128_adduu_w __builtin_mxu2_adduu_w
extern v2u64 __builtin_mxu2_adduu_d(v2u64, v2u64);
#define _mx128_adduu_d __builtin_mxu2_adduu_d
extern v16i8 __builtin_mxu2_add_b(v16i8, v16i8);
#define _mx128_add_b __builtin_mxu2_add_b
extern v8i16 __builtin_mxu2_add_h(v8i16, v8i16);
#define _mx128_add_h __builtin_mxu2_add_h
extern v4i32 __builtin_mxu2_add_w(v4i32, v4i32);
#define _mx128_add_w __builtin_mxu2_add_w
extern v2i64 __builtin_mxu2_add_d(v2i64, v2i64);
#define _mx128_add_d __builtin_mxu2_add_d
extern v16i8 __builtin_mxu2_subsa_b(v16i8, v16i8);
#define _mx128_subsa_b __builtin_mxu2_subsa_b
extern v8i16 __builtin_mxu2_subsa_h(v8i16, v8i16);
#define _mx128_subsa_h __builtin_mxu2_subsa_h
extern v4i32 __builtin_mxu2_subsa_w(v4i32, v4i32);
#define _mx128_subsa_w __builtin_mxu2_subsa_w
extern v2i64 __builtin_mxu2_subsa_d(v2i64, v2i64);
#define _mx128_subsa_d __builtin_mxu2_subsa_d
extern v16i8 __builtin_mxu2_subua_b(v16u8, v16u8);
#define _mx128_subua_b __builtin_mxu2_subua_b
extern v8i16 __builtin_mxu2_subua_h(v8u16, v8u16);
#define _mx128_subua_h __builtin_mxu2_subua_h
extern v4i32 __builtin_mxu2_subua_w(v4u32, v4u32);
#define _mx128_subua_w __builtin_mxu2_subua_w
extern v2i64 __builtin_mxu2_subua_d(v2u64, v2u64);
#define _mx128_subua_d __builtin_mxu2_subua_d
extern v16i8 __builtin_mxu2_subss_b(v16i8, v16i8);
#define _mx128_subss_b __builtin_mxu2_subss_b
extern v8i16 __builtin_mxu2_subss_h(v8i16, v8i16);
#define _mx128_subss_h __builtin_mxu2_subss_h
extern v4i32 __builtin_mxu2_subss_w(v4i32, v4i32);
#define _mx128_subss_w __builtin_mxu2_subss_w
extern v2i64 __builtin_mxu2_subss_d(v2i64, v2i64);
#define _mx128_subss_d __builtin_mxu2_subss_d
extern v16u8 __builtin_mxu2_subuu_b(v16u8, v16u8);
#define _mx128_subuu_b __builtin_mxu2_subuu_b
extern v8u16 __builtin_mxu2_subuu_h(v8u16, v8u16);
#define _mx128_subuu_h __builtin_mxu2_subuu_h
extern v4u32 __builtin_mxu2_subuu_w(v4u32, v4u32);
#define _mx128_subuu_w __builtin_mxu2_subuu_w
extern v2u64 __builtin_mxu2_subuu_d(v2u64, v2u64);
#define _mx128_subuu_d __builtin_mxu2_subuu_d
extern v16i8 __builtin_mxu2_subus_b(v16u8, v16u8);
#define _mx128_subus_b __builtin_mxu2_subus_b
extern v8i16 __builtin_mxu2_subus_h(v8u16, v8u16);
#define _mx128_subus_h __builtin_mxu2_subus_h
extern v4i32 __builtin_mxu2_subus_w(v4u32, v4u32);
#define _mx128_subus_w __builtin_mxu2_subus_w
extern v2i64 __builtin_mxu2_subus_d(v2u64, v2u64);
#define _mx128_subus_d __builtin_mxu2_subus_d
extern v16i8 __builtin_mxu2_sub_b(v16i8, v16i8);
#define _mx128_sub_b __builtin_mxu2_sub_b
extern v8i16 __builtin_mxu2_sub_h(v8i16, v8i16);
#define _mx128_sub_h __builtin_mxu2_sub_h
extern v4i32 __builtin_mxu2_sub_w(v4i32, v4i32);
#define _mx128_sub_w __builtin_mxu2_sub_w
extern v2i64 __builtin_mxu2_sub_d(v2i64, v2i64);
#define _mx128_sub_d __builtin_mxu2_sub_d
extern v16i8 __builtin_mxu2_aves_b(v16i8, v16i8);
#define _mx128_aves_b __builtin_mxu2_aves_b
extern v8i16 __builtin_mxu2_aves_h(v8i16, v8i16);
#define _mx128_aves_h __builtin_mxu2_aves_h
extern v4i32 __builtin_mxu2_aves_w(v4i32, v4i32);
#define _mx128_aves_w __builtin_mxu2_aves_w
extern v2i64 __builtin_mxu2_aves_d(v2i64, v2i64);
#define _mx128_aves_d __builtin_mxu2_aves_d
extern v16u8 __builtin_mxu2_aveu_b(v16u8, v16u8);
#define _mx128_aveu_b __builtin_mxu2_aveu_b
extern v8u16 __builtin_mxu2_aveu_h(v8u16, v8u16);
#define _mx128_aveu_h __builtin_mxu2_aveu_h
extern v4u32 __builtin_mxu2_aveu_w(v4u32, v4u32);
#define _mx128_aveu_w __builtin_mxu2_aveu_w
extern v2u64 __builtin_mxu2_aveu_d(v2u64, v2u64);
#define _mx128_aveu_d __builtin_mxu2_aveu_d
extern v16i8 __builtin_mxu2_avers_b(v16i8, v16i8);
#define _mx128_avers_b __builtin_mxu2_avers_b
extern v8i16 __builtin_mxu2_avers_h(v8i16, v8i16);
#define _mx128_avers_h __builtin_mxu2_avers_h
extern v4i32 __builtin_mxu2_avers_w(v4i32, v4i32);
#define _mx128_avers_w __builtin_mxu2_avers_w
extern v2i64 __builtin_mxu2_avers_d(v2i64, v2i64);
#define _mx128_avers_d __builtin_mxu2_avers_d
extern v16u8 __builtin_mxu2_averu_b(v16u8, v16u8);
#define _mx128_averu_b __builtin_mxu2_averu_b
extern v8u16 __builtin_mxu2_averu_h(v8u16, v8u16);
#define _mx128_averu_h __builtin_mxu2_averu_h
extern v4u32 __builtin_mxu2_averu_w(v4u32, v4u32);
#define _mx128_averu_w __builtin_mxu2_averu_w
extern v2u64 __builtin_mxu2_averu_d(v2u64, v2u64);
#define _mx128_averu_d __builtin_mxu2_averu_d
extern v16i8 __builtin_mxu2_divs_b(v16i8, v16i8);
#define _mx128_divs_b __builtin_mxu2_divs_b
extern v8i16 __builtin_mxu2_divs_h(v8i16, v8i16);
#define _mx128_divs_h __builtin_mxu2_divs_h
extern v4i32 __builtin_mxu2_divs_w(v4i32, v4i32);
#define _mx128_divs_w __builtin_mxu2_divs_w
extern v2i64 __builtin_mxu2_divs_d(v2i64, v2i64);
#define _mx128_divs_d __builtin_mxu2_divs_d
extern v16u8 __builtin_mxu2_divu_b(v16u8, v16u8);
#define _mx128_divu_b __builtin_mxu2_divu_b
extern v8u16 __builtin_mxu2_divu_h(v8u16, v8u16);
#define _mx128_divu_h __builtin_mxu2_divu_h
extern v4u32 __builtin_mxu2_divu_w(v4u32, v4u32);
#define _mx128_divu_w __builtin_mxu2_divu_w
extern v2u64 __builtin_mxu2_divu_d(v2u64, v2u64);
#define _mx128_divu_d __builtin_mxu2_divu_d
extern v16i8 __builtin_mxu2_mods_b(v16i8, v16i8);
#define _mx128_mods_b __builtin_mxu2_mods_b
extern v8i16 __builtin_mxu2_mods_h(v8i16, v8i16);
#define _mx128_mods_h __builtin_mxu2_mods_h
extern v4i32 __builtin_mxu2_mods_w(v4i32, v4i32);
#define _mx128_mods_w __builtin_mxu2_mods_w
extern v2i64 __builtin_mxu2_mods_d(v2i64, v2i64);
#define _mx128_mods_d __builtin_mxu2_mods_d
extern v16u8 __builtin_mxu2_modu_b(v16u8, v16u8);
#define _mx128_modu_b __builtin_mxu2_modu_b
extern v8u16 __builtin_mxu2_modu_h(v8u16, v8u16);
#define _mx128_modu_h __builtin_mxu2_modu_h
extern v4u32 __builtin_mxu2_modu_w(v4u32, v4u32);
#define _mx128_modu_w __builtin_mxu2_modu_w
extern v2u64 __builtin_mxu2_modu_d(v2u64, v2u64);
#define _mx128_modu_d __builtin_mxu2_modu_d
extern v16i8 __builtin_mxu2_madd_b(v16i8, v16i8, v16i8);
#define _mx128_madd_b __builtin_mxu2_madd_b
extern v8i16 __builtin_mxu2_madd_h(v8i16, v8i16, v8i16);
#define _mx128_madd_h __builtin_mxu2_madd_h
extern v4i32 __builtin_mxu2_madd_w(v4i32, v4i32, v4i32);
#define _mx128_madd_w __builtin_mxu2_madd_w
extern v2i64 __builtin_mxu2_madd_d(v2i64, v2i64, v2i64);
#define _mx128_madd_d __builtin_mxu2_madd_d
extern v16i8 __builtin_mxu2_msub_b(v16i8, v16i8, v16i8);
#define _mx128_msub_b __builtin_mxu2_msub_b
extern v8i16 __builtin_mxu2_msub_h(v8i16, v8i16, v8i16);
#define _mx128_msub_h __builtin_mxu2_msub_h
extern v4i32 __builtin_mxu2_msub_w(v4i32, v4i32, v4i32);
#define _mx128_msub_w __builtin_mxu2_msub_w
extern v2i64 __builtin_mxu2_msub_d(v2i64, v2i64, v2i64);
#define _mx128_msub_d __builtin_mxu2_msub_d
extern v16i8 __builtin_mxu2_mul_b(v16i8, v16i8);
#define _mx128_mul_b __builtin_mxu2_mul_b
extern v8i16 __builtin_mxu2_mul_h(v8i16, v8i16);
#define _mx128_mul_h __builtin_mxu2_mul_h
extern v4i32 __builtin_mxu2_mul_w(v4i32, v4i32);
#define _mx128_mul_w __builtin_mxu2_mul_w
extern v2i64 __builtin_mxu2_mul_d(v2i64, v2i64);
#define _mx128_mul_d __builtin_mxu2_mul_d
extern v16i8 __builtin_mxu2_maxa_b(v16i8, v16i8);
#define _mx128_maxa_b __builtin_mxu2_maxa_b
extern v8i16 __builtin_mxu2_maxa_h(v8i16, v8i16);
#define _mx128_maxa_h __builtin_mxu2_maxa_h
extern v4i32 __builtin_mxu2_maxa_w(v4i32, v4i32);
#define _mx128_maxa_w __builtin_mxu2_maxa_w
extern v2i64 __builtin_mxu2_maxa_d(v2i64, v2i64);
#define _mx128_maxa_d __builtin_mxu2_maxa_d
extern v16i8 __builtin_mxu2_maxs_b(v16i8, v16i8);
#define _mx128_maxs_b __builtin_mxu2_maxs_b
extern v8i16 __builtin_mxu2_maxs_h(v8i16, v8i16);
#define _mx128_maxs_h __builtin_mxu2_maxs_h
extern v4i32 __builtin_mxu2_maxs_w(v4i32, v4i32);
#define _mx128_maxs_w __builtin_mxu2_maxs_w
extern v2i64 __builtin_mxu2_maxs_d(v2i64, v2i64);
#define _mx128_maxs_d __builtin_mxu2_maxs_d
extern v16u8 __builtin_mxu2_maxu_b(v16u8, v16u8);
#define _mx128_maxu_b __builtin_mxu2_maxu_b
extern v8u16 __builtin_mxu2_maxu_h(v8u16, v8u16);
#define _mx128_maxu_h __builtin_mxu2_maxu_h
extern v4u32 __builtin_mxu2_maxu_w(v4u32, v4u32);
#define _mx128_maxu_w __builtin_mxu2_maxu_w
extern v2u64 __builtin_mxu2_maxu_d(v2u64, v2u64);
#define _mx128_maxu_d __builtin_mxu2_maxu_d
extern v16i8 __builtin_mxu2_mina_b(v16i8, v16i8);
#define _mx128_mina_b __builtin_mxu2_mina_b
extern v8i16 __builtin_mxu2_mina_h(v8i16, v8i16);
#define _mx128_mina_h __builtin_mxu2_mina_h
extern v4i32 __builtin_mxu2_mina_w(v4i32, v4i32);
#define _mx128_mina_w __builtin_mxu2_mina_w
extern v2i64 __builtin_mxu2_mina_d(v2i64, v2i64);
#define _mx128_mina_d __builtin_mxu2_mina_d
extern v16i8 __builtin_mxu2_mins_b(v16i8, v16i8);
#define _mx128_mins_b __builtin_mxu2_mins_b
extern v8i16 __builtin_mxu2_mins_h(v8i16, v8i16);
#define _mx128_mins_h __builtin_mxu2_mins_h
extern v4i32 __builtin_mxu2_mins_w(v4i32, v4i32);
#define _mx128_mins_w __builtin_mxu2_mins_w
extern v2i64 __builtin_mxu2_mins_d(v2i64, v2i64);
#define _mx128_mins_d __builtin_mxu2_mins_d
extern v16u8 __builtin_mxu2_minu_b(v16u8, v16u8);
#define _mx128_minu_b __builtin_mxu2_minu_b
extern v8u16 __builtin_mxu2_minu_h(v8u16, v8u16);
#define _mx128_minu_h __builtin_mxu2_minu_h
extern v4u32 __builtin_mxu2_minu_w(v4u32, v4u32);
#define _mx128_minu_w __builtin_mxu2_minu_w
extern v2u64 __builtin_mxu2_minu_d(v2u64, v2u64);
#define _mx128_minu_d __builtin_mxu2_minu_d
extern v16i8 __builtin_mxu2_sats_b(v16i8, unsigned char);
#define _mx128_sats_b __builtin_mxu2_sats_b
extern v8i16 __builtin_mxu2_sats_h(v8i16, unsigned char);
#define _mx128_sats_h __builtin_mxu2_sats_h
extern v4i32 __builtin_mxu2_sats_w(v4i32, unsigned char);
#define _mx128_sats_w __builtin_mxu2_sats_w
extern v2i64 __builtin_mxu2_sats_d(v2i64,  unsigned char);
#define _mx128_sats_d __builtin_mxu2_sats_d
extern v16u8 __builtin_mxu2_satu_b(v16u8, unsigned char);
#define _mx128_satu_b __builtin_mxu2_satu_b
extern v8u16 __builtin_mxu2_satu_h(v8u16, unsigned char);
#define _mx128_satu_h __builtin_mxu2_satu_h
extern v4u32 __builtin_mxu2_satu_w(v4u32, unsigned char);
#define _mx128_satu_w __builtin_mxu2_satu_w
extern v2u64 __builtin_mxu2_satu_d(v2u64, unsigned char);
#define _mx128_satu_d __builtin_mxu2_satu_d
/* Dot */
extern v8i16 __builtin_mxu2_dotps_h(v16i8, v16i8);
#define _mx128_dotps_h __builtin_mxu2_dotps_h
extern v4i32 __builtin_mxu2_dotps_w(v8i16, v8i16);
#define _mx128_dotps_w __builtin_mxu2_dotps_w
extern v2i64 __builtin_mxu2_dotps_d(v4i32,  v4i32);
#define _mx128_dotps_d __builtin_mxu2_dotps_d
extern v8u16 __builtin_mxu2_dotpu_h(v16u8, v16u8);
#define _mx128_dotpu_h __builtin_mxu2_dotpu_h
extern v4u32 __builtin_mxu2_dotpu_w(v8u16, v8u16);
#define _mx128_dotpu_w __builtin_mxu2_dotpu_w
extern v2u64 __builtin_mxu2_dotpu_d(v4u32,  v4u32);
#define _mx128_dotpu_d __builtin_mxu2_dotpu_d
extern v8i16 __builtin_mxu2_dadds_h(v8i16, v16i8, v16i8);
#define _mx128_dadds_h __builtin_mxu2_dadds_h
extern v4i32 __builtin_mxu2_dadds_w(v4i32, v8i16, v8i16);
#define _mx128_dadds_w __builtin_mxu2_dadds_w
extern v2i64 __builtin_mxu2_dadds_d(v2i64 ,v4i32,  v4i32);
#define _mx128_dadds_d __builtin_mxu2_dadds_d
extern v8u16 __builtin_mxu2_daddu_h(v8u16, v16u8, v16u8);
#define _mx128_daddu_h __builtin_mxu2_daddu_h
extern v4u32 __builtin_mxu2_daddu_w(v4u32, v8u16, v8u16);
#define _mx128_daddu_w __builtin_mxu2_daddu_w
extern v2u64 __builtin_mxu2_daddu_d(v2u64, v4u32,  v4u32);
#define _mx128_daddu_d __builtin_mxu2_daddu_d
extern v8i16 __builtin_mxu2_dsubs_h(v8i16, v16i8, v16i8);
#define _mx128_dsubs_h __builtin_mxu2_dsubs_h
extern v4i32 __builtin_mxu2_dsubs_w(v4i32, v8i16, v8i16);
#define _mx128_dsubs_w __builtin_mxu2_dsubs_w
extern v2i64 __builtin_mxu2_dsubs_d(v2i64 ,v4i32,  v4i32);
#define _mx128_dsubs_d __builtin_mxu2_dsubs_d
extern v8i16 __builtin_mxu2_dsubu_h(v8i16, v16u8, v16u8);
#define _mx128_dsubu_h __builtin_mxu2_dsubu_h
extern v4i32 __builtin_mxu2_dsubu_w(v4i32, v8u16, v8u16);
#define _mx128_dsubu_w __builtin_mxu2_dsubu_w
extern v2i64 __builtin_mxu2_dsubu_d(v2i64, v4u32,  v4u32);
#define _mx128_dsubu_d __builtin_mxu2_dsubu_d
/* Bitwise */
extern v16i8 __builtin_mxu2_loc_b(v16i8);
#define _mx128_loc_b __builtin_mxu2_loc_b
extern v8i16 __builtin_mxu2_loc_h(v8i16);
#define _mx128_loc_h __builtin_mxu2_loc_h
extern v4i32 __builtin_mxu2_loc_w(v4i32);
#define _mx128_loc_w __builtin_mxu2_loc_w
extern v2i64 __builtin_mxu2_loc_d(v2i64);
#define _mx128_loc_d __builtin_mxu2_loc_d
extern v16i8 __builtin_mxu2_lzc_b(v16i8);
#define _mx128_lzc_b __builtin_mxu2_lzc_b
extern v8i16 __builtin_mxu2_lzc_h(v8i16);
#define _mx128_lzc_h __builtin_mxu2_lzc_h
extern v4i32 __builtin_mxu2_lzc_w(v4i32);
#define _mx128_lzc_w __builtin_mxu2_lzc_w
extern v2i64 __builtin_mxu2_lzc_d(v2i64);
#define _mx128_lzc_d __builtin_mxu2_lzc_d
extern v16i8 __builtin_mxu2_bcnt_b(v16i8);
#define _mx128_bcnt_b __builtin_mxu2_bcnt_b
extern v8i16 __builtin_mxu2_bcnt_h(v8i16);
#define _mx128_bcnt_h __builtin_mxu2_bcnt_h
extern v4i32 __builtin_mxu2_bcnt_w(v4i32);
#define _mx128_bcnt_w __builtin_mxu2_bcnt_w
extern v2i64 __builtin_mxu2_bcnt_d(v2i64);
#define _mx128_bcnt_d __builtin_mxu2_bcnt_d
extern v16i8 __builtin_mxu2_andv(v16i8, v16i8);
#define _mx128_andv __builtin_mxu2_andv
extern v16i8 __builtin_mxu2_andib(v16i8, unsigned char);
#define _mx128_andib __builtin_mxu2_andib
extern v16i8 __builtin_mxu2_norv(v16i8, v16i8);
#define _mx128_norv __builtin_mxu2_norv
extern v16i8 __builtin_mxu2_norib(v16i8, unsigned char);
#define _mx128_norib __builtin_mxu2_norib
extern v16i8 __builtin_mxu2_orv(v16i8, v16i8);
#define _mx128_orv __builtin_mxu2_orv
extern v16i8 __builtin_mxu2_orib(v16i8, unsigned char);
#define _mx128_orib __builtin_mxu2_orib
extern v16i8 __builtin_mxu2_xorv(v16i8, v16i8);
#define _mx128_xorv __builtin_mxu2_xorv
extern v16i8 __builtin_mxu2_xorib(v16i8, unsigned char);
#define _mx128_xorib __builtin_mxu2_xorib
extern v16i8 __builtin_mxu2_bselv(v16i8, v16i8, v16i8);
#define _mx128_bselv __builtin_mxu2_bselv
/* Float Point Arithmetic */
extern v4f32 __builtin_mxu2_fadd_w(v4f32, v4f32);
#define _mx128_fadd_w __builtin_mxu2_fadd_w
extern v2f64 __builtin_mxu2_fadd_d(v2f64, v2f64);
#define _mx128_fadd_d __builtin_mxu2_fadd_d
extern v4f32 __builtin_mxu2_fsub_w(v4f32, v4f32);
#define _mx128_fsub_w __builtin_mxu2_fsub_w
extern v2f64 __builtin_mxu2_fsub_d(v2f64, v2f64);
#define _mx128_fsub_d __builtin_mxu2_fsub_d
extern v4f32 __builtin_mxu2_fmul_w(v4f32, v4f32);
#define _mx128_fmul_w __builtin_mxu2_fmul_w
extern v2f64 __builtin_mxu2_fmul_d(v2f64, v2f64);
#define _mx128_fmul_d __builtin_mxu2_fmul_d
extern v4f32 __builtin_mxu2_fdiv_w(v4f32, v4f32);
#define _mx128_fdiv_w __builtin_mxu2_fdiv_w
extern v2f64 __builtin_mxu2_fdiv_d(v2f64, v2f64);
#define _mx128_fdiv_d __builtin_mxu2_fdiv_d
extern v4f32 __builtin_mxu2_fsqrt_w(v4f32);
#define _mx128_fsqrt_w __builtin_mxu2_fsqrt_w
extern v2f64 __builtin_mxu2_fsqrt_d(v2f64);
#define _mx128_fsqrt_d __builtin_mxu2_fsqrt_d
extern v4f32 __builtin_mxu2_fmadd_w(v4f32, v4f32, v4f32);
#define _mx128_fmadd_w __builtin_mxu2_fmadd_w
extern v2f64 __builtin_mxu2_fmadd_d(v2f64, v2f64, v2f64);
#define _mx128_fmadd_d __builtin_mxu2_fmadd_d
extern v4f32 __builtin_mxu2_fmsub_w(v4f32, v4f32, v4f32);
#define _mx128_fmsub_w __builtin_mxu2_fmsub_w
extern v2f64 __builtin_mxu2_fmsub_d(v2f64, v2f64, v2f64);
#define _mx128_fmsub_d __builtin_mxu2_fmsub_d
extern v4f32 __builtin_mxu2_fmax_w(v4f32, v4f32);
#define _mx128_fmax_w __builtin_mxu2_fmax_w
extern v2f64 __builtin_mxu2_fmax_d(v2f64, v2f64);
#define _mx128_fmax_d __builtin_mxu2_fmax_d
extern v4f32 __builtin_mxu2_fmaxa_w(v4f32, v4f32);
#define _mx128_fmaxa_w __builtin_mxu2_fmaxa_w
extern v2f64 __builtin_mxu2_fmaxa_d(v2f64, v2f64);
#define _mx128_fmaxa_d __builtin_mxu2_fmaxa_d
extern v4f32 __builtin_mxu2_fmin_w(v4f32, v4f32);
#define _mx128_fmin_w __builtin_mxu2_fmin_w
extern v2f64 __builtin_mxu2_fmin_d(v2f64, v2f64);
#define _mx128_fmin_d __builtin_mxu2_fmin_d
extern v4f32 __builtin_mxu2_fmina_w(v4f32, v4f32);
#define _mx128_fmina_w __builtin_mxu2_fmina_w
extern v2f64 __builtin_mxu2_fmina_d(v2f64, v2f64);
#define _mx128_fmina_d __builtin_mxu2_fmina_d
extern v4i32 __builtin_mxu2_fclass_w(v4f32);
#define _mx128_fclass_w __builtin_mxu2_fclass_w
extern v2i64 __builtin_mxu2_fclass_d(v2f64);
#define _mx128_fclass_d __builtin_mxu2_fclass_d
/* Float Point Compare */
extern v4i32 __builtin_mxu2_fceq_w(v4f32, v4f32);
#define _mx128_fceq_w __builtin_mxu2_fceq_w
extern v2i64 __builtin_mxu2_fceq_d(v2f64, v2f64);
#define _mx128_fceq_d __builtin_mxu2_fceq_d
extern v4i32 __builtin_mxu2_fcle_w(v4f32, v4f32);
#define _mx128_fcle_w __builtin_mxu2_fcle_w
extern v2i64 __builtin_mxu2_fcle_d(v2f64, v2f64);
#define _mx128_fcle_d __builtin_mxu2_fcle_d
extern v4i32 __builtin_mxu2_fclt_w(v4f32, v4f32);
#define _mx128_fclt_w __builtin_mxu2_fclt_w
extern v2i64 __builtin_mxu2_fclt_d(v2f64, v2f64);
#define _mx128_fclt_d __builtin_mxu2_fclt_d
extern v4i32 __builtin_mxu2_fcor_w(v4f32, v4f32);
#define _mx128_fcor_w __builtin_mxu2_fcor_w
extern v2i64 __builtin_mxu2_fcor_d(v2f64, v2f64);
#define _mx128_fcor_d __builtin_mxu2_fcor_d
/*Float Point Conversion */
extern v8i16 __builtin_mxu2_vcvths(v4f32, v4f32);
#define _mx128_vcvths __builtin_mxu2_vcvths
extern v4f32 __builtin_mxu2_vcvtsd(v2f64, v2f64);
#define _mx128_vcvtsd __builtin_mxu2_vcvtsd
extern v4f32 __builtin_mxu2_vcvtesh(v8i16);
#define _mx128_vcvtesh __builtin_mxu2_vcvtesh
extern v2f64 __builtin_mxu2_vcvteds(v4f32);
#define _mx128_vcvteds __builtin_mxu2_vcvteds
extern v4f32 __builtin_mxu2_vcvtosh(v8i16);
#define _mx128_vcvtosh __builtin_mxu2_vcvtosh
extern v2f64 __builtin_mxu2_vcvtods(v4f32);
#define _mx128_vcvtods __builtin_mxu2_vcvtods
extern v4f32 __builtin_mxu2_vcvtssw(v4i32);
#define _mx128_vcvtssw __builtin_mxu2_vcvtssw
extern v2f64 __builtin_mxu2_vcvtsdl(v2i64);
#define _mx128_vcvtsdl __builtin_mxu2_vcvtsdl
extern v4f32 __builtin_mxu2_vcvtusw(v4u32);
#define _mx128_vcvtusw __builtin_mxu2_vcvtusw
extern v2f64 __builtin_mxu2_vcvtudl(v2u64);
#define _mx128_vcvtudl __builtin_mxu2_vcvtudl
extern v4i32 __builtin_mxu2_vcvtsws(v4f32);
#define _mx128_vcvtsws __builtin_mxu2_vcvtsws
extern v2i64 __builtin_mxu2_vcvtsld(v2f64);
#define _mx128_vcvtsld __builtin_mxu2_vcvtsld
extern v4u32 __builtin_mxu2_vcvtuws(v4f32);
#define _mx128_vcvtuws __builtin_mxu2_vcvtuws
extern v2u64 __builtin_mxu2_vcvtuld(v2f64);
#define _mx128_vcvtuld __builtin_mxu2_vcvtuld
extern v4i32 __builtin_mxu2_vcvtrws(v4f32);
#define _mx128_vcvtrws __builtin_mxu2_vcvtrws
extern v2i64 __builtin_mxu2_vcvtrld(v2f64);
#define _mx128_vcvtrld __builtin_mxu2_vcvtrld
extern v4i32 __builtin_mxu2_vtruncsws(v4f32);
#define _mx128_vtruncsws __builtin_mxu2_vtruncsws
extern v2i64 __builtin_mxu2_vtruncsld(v2f64);
#define _mx128_vtruncsld __builtin_mxu2_vtruncsld
extern v4u32 __builtin_mxu2_vtruncuws(v4f32);
#define _mx128_vtruncuws __builtin_mxu2_vtruncuws
extern v2u64 __builtin_mxu2_vtrunculd(v2f64);
#define _mx128_vtrunculd __builtin_mxu2_vtrunculd
extern v4f32 __builtin_mxu2_vcvtqesh(v8i16);
#define _mx128_vcvtqesh __builtin_mxu2_vcvtqesh
extern v2f64 __builtin_mxu2_vcvtqedw(v4i32);
#define _mx128_vcvtqedw __builtin_mxu2_vcvtqedw
extern v4f32 __builtin_mxu2_vcvtqosh(v8i16);
#define _mx128_vcvtqosh __builtin_mxu2_vcvtqosh
extern v2f64 __builtin_mxu2_vcvtqodw(v4i32);
#define _mx128_vcvtqodw __builtin_mxu2_vcvtqodw
extern v8i16 __builtin_mxu2_vcvtqhs(v4f32, v4f32);
#define _mx128_vcvtqhs __builtin_mxu2_vcvtqhs
extern v4i32 __builtin_mxu2_vcvtqwd(v2f64, v2f64);
#define _mx128_vcvtqwd __builtin_mxu2_vcvtqwd
/* Fixed Point Multiplication */
extern v8i16 __builtin_mxu2_maddq_h(v8i16, v8i16, v8i16);
#define _mx128_maddq_h __builtin_mxu2_maddq_h
extern v4i32 __builtin_mxu2_maddq_w(v4i32, v4i32, v4i32);
#define _mx128_maddq_w __builtin_mxu2_maddq_w
extern v8i16 __builtin_mxu2_maddqr_h(v8i16, v8i16, v8i16);
#define _mx128_maddqr_h __builtin_mxu2_maddqr_h
extern v4i32 __builtin_mxu2_maddqr_w(v4i32, v4i32, v4i32);
#define _mx128_maddqr_w __builtin_mxu2_maddqr_w
extern v8i16 __builtin_mxu2_msubq_h(v8i16, v8i16, v8i16);
#define _mx128_msubq_h __builtin_mxu2_msubq_h
extern v4i32 __builtin_mxu2_msubq_w(v4i32, v4i32, v4i32);
#define _mx128_msubq_w __builtin_mxu2_msubq_w
extern v8i16 __builtin_mxu2_msubqr_h(v8i16, v8i16, v8i16);
#define _mx128_msubqr_h __builtin_mxu2_msubqr_h
extern v4i32 __builtin_mxu2_msubqr_w(v4i32, v4i32, v4i32);
#define _mx128_msubqr_w __builtin_mxu2_msubqr_w
extern v8i16 __builtin_mxu2_mulq_h(v8i16, v8i16);
#define _mx128_mulq_h __builtin_mxu2_mulq_h
extern v4i32 __builtin_mxu2_mulq_w(v4i32, v4i32);
#define _mx128_mulq_w __builtin_mxu2_mulq_w
extern v8i16 __builtin_mxu2_mulqr_h(v8i16, v8i16);
#define _mx128_mulqr_h __builtin_mxu2_mulqr_h
extern v4i32 __builtin_mxu2_mulqr_w(v4i32, v4i32);
#define _mx128_mulqr_w __builtin_mxu2_mulqr_w
/* Shift */
extern v16i8 __builtin_mxu2_sll_b(v16i8, v16i8);
#define _mx128_sll_b __builtin_mxu2_sll_b
extern v8i16 __builtin_mxu2_sll_h(v8i16, v8i16);
#define _mx128_sll_h __builtin_mxu2_sll_h
extern v4i32 __builtin_mxu2_sll_w(v4i32, v4i32);
#define _mx128_sll_w __builtin_mxu2_sll_w
extern v2i64 __builtin_mxu2_sll_d(v2i64, v2i64);
#define _mx128_sll_d __builtin_mxu2_sll_d
extern v16i8 __builtin_mxu2_slli_b(v16i8, unsigned char);
#define _mx128_slli_b __builtin_mxu2_slli_b
extern v8i16 __builtin_mxu2_slli_h(v8i16, unsigned char);
#define _mx128_slli_h __builtin_mxu2_slli_h
extern v4i32 __builtin_mxu2_slli_w(v4i32, unsigned char);
#define _mx128_slli_w __builtin_mxu2_slli_w
extern v2i64 __builtin_mxu2_slli_d(v2i64, unsigned char);
#define _mx128_slli_d __builtin_mxu2_slli_d
extern v16i8 __builtin_mxu2_sra_b(v16i8, v16i8);
#define _mx128_sra_b __builtin_mxu2_sra_b
extern v8i16 __builtin_mxu2_sra_h(v8i16, v8i16);
#define _mx128_sra_h __builtin_mxu2_sra_h
extern v4i32 __builtin_mxu2_sra_w(v4i32, v4i32);
#define _mx128_sra_w __builtin_mxu2_sra_w
extern v2i64 __builtin_mxu2_sra_d(v2i64, v2i64);
#define _mx128_sra_d __builtin_mxu2_sra_d
extern v16i8 __builtin_mxu2_srai_b(v16i8, unsigned char);
#define _mx128_srai_b __builtin_mxu2_srai_b
extern v8i16 __builtin_mxu2_srai_h(v8i16, unsigned char);
#define _mx128_srai_h __builtin_mxu2_srai_h
extern v4i32 __builtin_mxu2_srai_w(v4i32, unsigned char);
#define _mx128_srai_w __builtin_mxu2_srai_w
extern v2i64 __builtin_mxu2_srai_d(v2i64, unsigned char);
#define _mx128_srai_d __builtin_mxu2_srai_d
extern v16i8 __builtin_mxu2_srar_b(v16i8, v16i8);
#define _mx128_srar_b __builtin_mxu2_srar_b
extern v8i16 __builtin_mxu2_srar_h(v8i16, v8i16);
#define _mx128_srar_h __builtin_mxu2_srar_h
extern v4i32 __builtin_mxu2_srar_w(v4i32, v4i32);
#define _mx128_srar_w __builtin_mxu2_srar_w
extern v2i64 __builtin_mxu2_srar_d(v2i64, v2i64);
#define _mx128_srar_d __builtin_mxu2_srar_d
extern v16i8 __builtin_mxu2_srari_b(v16i8, unsigned char);
#define _mx128_srari_b __builtin_mxu2_srari_b
extern v8i16 __builtin_mxu2_srari_h(v8i16, unsigned char);
#define _mx128_srari_h __builtin_mxu2_srari_h
extern v4i32 __builtin_mxu2_srari_w(v4i32, unsigned char);
#define _mx128_srari_w __builtin_mxu2_srari_w
extern v2i64 __builtin_mxu2_srari_d(v2i64, unsigned char);
#define _mx128_srari_d __builtin_mxu2_srari_d
extern v16i8 __builtin_mxu2_srl_b(v16i8, v16i8);
#define _mx128_srl_b __builtin_mxu2_srl_b
extern v8i16 __builtin_mxu2_srl_h(v8i16, v8i16);
#define _mx128_srl_h __builtin_mxu2_srl_h
extern v4i32 __builtin_mxu2_srl_w(v4i32, v4i32);
#define _mx128_srl_w __builtin_mxu2_srl_w
extern v2i64 __builtin_mxu2_srl_d(v2i64, v2i64);
#define _mx128_srl_d __builtin_mxu2_srl_d
extern v16i8 __builtin_mxu2_srli_b(v16i8, unsigned char);
#define _mx128_srli_b __builtin_mxu2_srli_b
extern v8i16 __builtin_mxu2_srli_h(v8i16, unsigned char);
#define _mx128_srli_h __builtin_mxu2_srli_h
extern v4i32 __builtin_mxu2_srli_w(v4i32, unsigned char);
#define _mx128_srli_w __builtin_mxu2_srli_w
extern v2i64 __builtin_mxu2_srli_d(v2i64, unsigned char);
#define _mx128_srli_d __builtin_mxu2_srli_d
extern v16i8 __builtin_mxu2_srlr_b(v16i8, v16i8);
#define _mx128_srlr_b __builtin_mxu2_srlr_b
extern v8i16 __builtin_mxu2_srlr_h(v8i16, v8i16);
#define _mx128_srlr_h __builtin_mxu2_srlr_h
extern v4i32 __builtin_mxu2_srlr_w(v4i32, v4i32);
#define _mx128_srlr_w __builtin_mxu2_srlr_w
extern v2i64 __builtin_mxu2_srlr_d(v2i64, v2i64);
#define _mx128_srlr_d __builtin_mxu2_srlr_d
extern v16i8 __builtin_mxu2_srlri_b(v16i8, unsigned char);
#define _mx128_srlri_b __builtin_mxu2_srlri_b
extern v8i16 __builtin_mxu2_srlri_h(v8i16, unsigned char);
#define _mx128_srlri_h __builtin_mxu2_srlri_h
extern v4i32 __builtin_mxu2_srlri_w(v4i32, unsigned char);
#define _mx128_srlri_w __builtin_mxu2_srlri_w
extern v2i64 __builtin_mxu2_srlri_d(v2i64, unsigned char);
#define _mx128_srlri_d __builtin_mxu2_srlri_d
/* Element Permute */
extern v16i8 __builtin_mxu2_shufv(v16i8, v16i8, v16i8);
#define _mx128_shufv __builtin_mxu2_shufv
extern v16i8 __builtin_mxu2_insfcpu_b(v16i8, unsigned char, int);
#define _mx128_insfcpu_b __builtin_mxu2_insfcpu_b
extern v8i16 __builtin_mxu2_insfcpu_h(v8i16, unsigned char, int);
#define _mx128_insfcpu_h __builtin_mxu2_insfcpu_h
extern v4i32 __builtin_mxu2_insfcpu_w(v4i32, unsigned char, int);
#define _mx128_insfcpu_w __builtin_mxu2_insfcpu_w
extern v2i64 __builtin_mxu2_insfcpu_d(v2i64, unsigned char, long long);
#define _mx128_insfcpu_d __builtin_mxu2_insfcpu_d
extern v4f32 __builtin_mxu2_insffpu_w(v4f32, unsigned char, float);
#define _mx128_insffpu_w __builtin_mxu2_insffpu_w
extern v2f64 __builtin_mxu2_insffpu_d(v2f64, unsigned char, double);
#define _mx128_insffpu_d __builtin_mxu2_insffpu_d
extern v16i8 __builtin_mxu2_insfmxu_b(v16i8, unsigned char, v16i8);
#define _mx128_insfmxu_b __builtin_mxu2_insfmxu_b
extern v8i16 __builtin_mxu2_insfmxu_h(v8i16, unsigned char, v8i16);
#define _mx128_insfmxu_h __builtin_mxu2_insfmxu_h
extern v4i32 __builtin_mxu2_insfmxu_w(v4i32, unsigned char, v4i32);
#define _mx128_insfmxu_w __builtin_mxu2_insfmxu_w
extern v2i64 __builtin_mxu2_insfmxu_d(v2i64, unsigned char, v2i64);
#define _mx128_insfmxu_d __builtin_mxu2_insfmxu_d
extern v16i8 __builtin_mxu2_repx_b(v16i8, int);
#define _mx128_repx_b __builtin_mxu2_repx_b
extern v8i16 __builtin_mxu2_repx_h(v8i16, int);
#define _mx128_repx_h __builtin_mxu2_repx_h
extern v4i32 __builtin_mxu2_repx_w(v4i32, int);
#define _mx128_repx_w __builtin_mxu2_repx_w
extern v2i64 __builtin_mxu2_repx_d(v2i64, int);
#define _mx128_repx_d __builtin_mxu2_repx_d
extern v16i8 __builtin_mxu2_repi_b(v16i8, unsigned char);
#define _mx128_repi_b __builtin_mxu2_repi_b
extern v8i16 __builtin_mxu2_repi_h(v8i16, unsigned char);
#define _mx128_repi_h __builtin_mxu2_repi_h
extern v4i32 __builtin_mxu2_repi_w(v4i32, unsigned char);
#define _mx128_repi_w __builtin_mxu2_repi_w
extern v2i64 __builtin_mxu2_repi_d(v2i64, unsigned char);
#define _mx128_repi_d __builtin_mxu2_repi_d
/* Load/Store */
extern int __builtin_mxu2_mtcpus_b(v16i8, unsigned char);
#define _mx128_mtcpus_b __builtin_mxu2_mtcpus_b
extern int __builtin_mxu2_mtcpus_h(v8i16, unsigned char);
#define _mx128_mtcpus_h __builtin_mxu2_mtcpus_h
extern int __builtin_mxu2_mtcpus_w(v4i32, unsigned char);
#define _mx128_mtcpus_w __builtin_mxu2_mtcpus_w
extern long long __builtin_mxu2_mtcpus_d(v2i64, unsigned char);
#define _mx128_mtcpus_d __builtin_mxu2_mtcpus_d
extern unsigned int __builtin_mxu2_mtcpuu_b(v16i8, unsigned char);
#define _mx128_mtcpuu_b __builtin_mxu2_mtcpuu_b
extern unsigned int __builtin_mxu2_mtcpuu_h(v8i16, unsigned char);
#define _mx128_mtcpuu_h __builtin_mxu2_mtcpuu_h
extern unsigned int __builtin_mxu2_mtcpuu_w(v4i32, unsigned char);
#define _mx128_mtcpuu_w __builtin_mxu2_mtcpuu_w
extern unsigned long long __builtin_mxu2_mtcpuu_d(v2i64, unsigned char);
#define _mx128_mtcpuu_d __builtin_mxu2_mtcpuu_d
extern v16i8 __builtin_mxu2_mfcpu_b(int);
#define _mx128_mfcpu_b __builtin_mxu2_mfcpu_b
extern v8i16 __builtin_mxu2_mfcpu_h(int);
#define _mx128_mfcpu_h __builtin_mxu2_mfcpu_h
extern v4i32 __builtin_mxu2_mfcpu_w(int);
#define _mx128_mfcpu_w __builtin_mxu2_mfcpu_w
extern v2i64 __builtin_mxu2_mfcpu_d(long long);
#define _mx128_mfcpu_d __builtin_mxu2_mfcpu_d
extern float __builtin_mxu2_mtfpu_w(v4f32, unsigned char);
#define _mx128_mtfpu_w __builtin_mxu2_mtfpu_w
extern double __builtin_mxu2_mtfpu_d(v2f64, unsigned char);
#define _mx128_mtfpu_d __builtin_mxu2_mtfpu_d
extern v4f32 __builtin_mxu2_mffpu_w(float);
#define _mx128_mffpu_w __builtin_mxu2_mffpu_w
extern  v2f64 __builtin_mxu2_mffpu_d(double);
#define _mx128_mffpu_d __builtin_mxu2_mffpu_d
extern  void __builtin_mxu2_ctcmxu(unsigned char, int);
#define _mx128_ctcmxu __builtin_mxu2_ctcmxu
extern  int __builtin_mxu2_cfcmxu(unsigned char);
#define _mx128_cfcmxu __builtin_mxu2_cfcmxu
extern  v16i8 __builtin_mxu2_lu1q(void * , int);
#define _mx128_lu1q __builtin_mxu2_lu1q
extern  v16i8 __builtin_mxu2_lu1qx(void * , int);
#define _mx128_lu1qx __builtin_mxu2_lu1qx
extern  v16i8 __builtin_mxu2_la1q(void * , int);
#define _mx128_la1q __builtin_mxu2_la1q
extern  v16i8 __builtin_mxu2_la1qx(void * , int);
#define _mx128_la1qx __builtin_mxu2_la1qx
extern void __builtin_mxu2_su1q(v16i8, void * , int);
#define _mx128_su1q __builtin_mxu2_su1q
extern void __builtin_mxu2_su1qx(v16i8, void * , int);
#define _mx128_su1qx __builtin_mxu2_su1qx
extern void __builtin_mxu2_sa1q(v16i8, void * , int);
#define _mx128_sa1q __builtin_mxu2_sa1q
extern void __builtin_mxu2_sa1qx(v16i8, void * , int);
#define _mx128_sa1qx __builtin_mxu2_sa1qx
extern v16i8 __builtin_mxu2_li_b(short);
#define _mx128_li_b __builtin_mxu2_li_b
extern v8i16 __builtin_mxu2_li_h(short);
#define _mx128_li_h __builtin_mxu2_li_h
extern v4i32 __builtin_mxu2_li_w(short);
#define _mx128_li_w __builtin_mxu2_li_w
extern v2i64 __builtin_mxu2_li_d(short);
#define _mx128_li_d __builtin_mxu2_li_d

#include <mxu2_compatible.h>

#endif /* __clang__ */
#endif /* defined(__mips_mxu2) */
#endif /* _MXU2_H */
