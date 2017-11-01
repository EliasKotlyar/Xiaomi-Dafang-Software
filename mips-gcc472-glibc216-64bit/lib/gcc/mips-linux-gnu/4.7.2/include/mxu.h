/* MIPS MXU intrinsics include file.

   Copyright (C) 2006 Ingenic Semiconductor CO.,LTD.
   Contributed by Cheng Lulu, lulu.cheng@ingenic.com.

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

#ifndef _MXU_H
#define _MXU_H 1

#if defined(__mips_mxu)
typedef signed char v4i8 __attribute__((vector_size(4), aligned(4)));
typedef unsigned char v4u8 __attribute__((vector_size(4), aligned(4)));
typedef short v2i16 __attribute__((vector_size(4), aligned(4)));
typedef unsigned short v2u16 __attribute__((vector_size(4), aligned(4)));
typedef int v1i32 __attribute__((vector_size(4), aligned(4)));
typedef unsigned int v1u32 __attribute__((vector_size(4), aligned(4)));


#ifndef __clang__
/* Load/Store */
extern v4i8 __builtin_mxu_s32ldd(void*, int);
#define __mxu_s32ldd __builtin_mxu_s32ldd
extern void __builtin_mxu_s32std(v4i8, void*, int);
#define __mxu_s32std __builtin_mxu_s32std
extern v4i8 __builtin_mxu_s32lddr(void*, int);
#define __mxu_s32lddr __builtin_mxu_s32lddr
extern void __builtin_mxu_s32stdr(v4i8, void*, int);
#define __mxu_s32stdr __builtin_mxu_s32stdr
extern v4i8 __builtin_mxu_s32lddvr(void*, int, int);
#define __mxu_s32lddvr __builtin_mxu_s32lddvr
extern void __builtin_mxu_s32stdvr(v4i8, void*, int, int);
#define __mxu_s32stdvr __builtin_mxu_s32stdvr
extern v4i8 __builtin_mxu_s32lddv(void*, int, int);
#define __mxu_s32lddv __builtin_mxu_s32lddv
extern void __builtin_mxu_s32stdv(v4i8, void*, int, int);
#define __mxu_s32stdv __builtin_mxu_s32stdv
extern int __builtin_mxu_lxw(void*, int, int);
#define __mxu_lxw __builtin_mxu_lxw
extern int __builtin_mxu_lxh(void*, int, int);
#define __mxu_lxh __builtin_mxu_lxh
extern int __builtin_mxu_lxhu(void*, int, int);
#define __mxu_lxhu __builtin_mxu_lxhu
extern int __builtin_mxu_lxb(void*, int, int);
#define __mxu_lxb __builtin_mxu_lxb
extern int __builtin_mxu_lxbu(void*, int, int);
#define __mxu_lxbu __builtin_mxu_lxbu
extern v2i16 __builtin_mxu_s16ldd(v2i16, void*, int, int);
#define __mxu_s16ldd __builtin_mxu_s16ldd
extern void __builtin_mxu_s16std(v2i16, void*, int, int);
#define __mxu_s16std __builtin_mxu_s16std
extern v4i8 __builtin_mxu_s8ldd(v4i8, void*, int, int);
#define __mxu_s8ldd __builtin_mxu_s8ldd
extern void __builtin_mxu_s8std(v4i8, void*, int, int);
#define __mxu_s8std __builtin_mxu_s8std
/* MUL */
extern v2i16 __builtin_mxu_d16mulf(v2i16, v2i16, int);
#define __mxu_d16mulf __builtin_mxu_d16mulf
extern v4i8 __builtin_mxu_q8madl(v4i8, v4u8, v4u8, int);
#define __mxu_q8madl __builtin_mxu_q8madl
extern v2i16 __builtin_mxu_d16macf(v2i16, v2i16, v2i16, v2i16, int, int);
#define __mxu_d16macf __builtin_mxu_d16macf
extern v2i16 __builtin_mxu_d16madl(v2i16, v2i16, v2i16, int , int);
#define __mxu_d16madl __builtin_mxu_d16madl
extern v1i32 __builtin_mxu_s16mad(v1i32, v2i16, v2i16, int, int);
#define __mxu_s16mad __builtin_mxu_s16mad
/* Bitwise */
extern v1i32 __builtin_mxu_s32and_w(v1i32, v1i32);
#define __mxu_s32and_w __builtin_mxu_s32and_w
extern v2i16 __builtin_mxu_s32and_h(v2i16, v2i16);
#define __mxu_s32and_h __builtin_mxu_s32and_h
extern v4i8 __builtin_mxu_s32and_b(v4i8, v4i8);
#define __mxu_s32and_b __builtin_mxu_s32and_b
extern v1i32 __builtin_mxu_s32or_w(v1i32, v1i32);
#define __mxu_s32or_w __builtin_mxu_s32or_w
extern v2i16 __builtin_mxu_s32or_h(v2i16, v2i16);
#define __mxu_s32or_h __builtin_mxu_s32or_h
extern v4i8 __builtin_mxu_s32or_b(v4i8, v4i8);
#define __mxu_s32or_b __builtin_mxu_s32or_b
extern v1i32 __builtin_mxu_s32xor_w(v1i32, v1i32);
#define __mxu_s32xor_w __builtin_mxu_s32xor_w
extern v2i16 __builtin_mxu_s32xor_h(v2i16, v2i16);
#define __mxu_s32xor_h __builtin_mxu_s32xor_h
extern v4i8 __builtin_mxu_s32xor_b(v4i8, v4i8);
#define __mxu_s32xor_b __builtin_mxu_s32xor_b
extern v1i32 __builtin_mxu_s32nor_w(v1i32, v1i32);
#define __mxu_s32nor_w __builtin_mxu_s32nor_w
extern v2i16 __builtin_mxu_s32nor_h(v2i16, v2i16);
#define __mxu_s32nor_h __builtin_mxu_s32nor_h
extern v4i8 __builtin_mxu_s32nor_b(v4i8, v4i8);
#define __mxu_s32nor_b __builtin_mxu_s32nor_b
/* Add and subtract */
extern v1i32 __builtin_mxu_s32cps(v1i32, v1i32);
#define __mxu_s32cps __builtin_mxu_s32cps
extern v1i32 __builtin_mxu_s32slt(v1i32, v1i32);
#define __mxu_s32slt __builtin_mxu_s32slt
extern v1i32 __builtin_mxu_s32movz(v1i32, v1i32, v1i32);
#define __mxu_s32movz __builtin_mxu_s32movz
extern v1i32 __builtin_mxu_s32movn(v1i32, v1i32, v1i32);
#define __mxu_s32movn __builtin_mxu_s32movn
extern v2i16 __builtin_mxu_d16cps(v2i16, v2i16);
#define __mxu_d16cps __builtin_mxu_d16cps
extern v2i16 __builtin_mxu_d16slt(v2i16, v2i16);
#define __mxu_d16slt __builtin_mxu_d16slt
extern v2i16 __builtin_mxu_d16movz(v2i16, v2i16, v2i16);
#define __mxu_d16movz __builtin_mxu_d16movz
extern v2i16 __builtin_mxu_d16movn(v2i16, v2i16, v2i16);
#define __mxu_d16movn __builtin_mxu_d16movn
extern v2i16 __builtin_mxu_d16avg(v2i16, v2i16);
#define __mxu_d16avg __builtin_mxu_d16avg
extern v2i16 __builtin_mxu_d16avgr(v2i16, v2i16);
#define __mxu_d16avgr __builtin_mxu_d16avgr
extern v4i8 __builtin_mxu_q8add(v4i8, v4i8, int);
#define __mxu_q8add __builtin_mxu_q8add
extern v2u16 __builtin_mxu_d8sum(v4u8, v4u8);
#define __mxu_d8sum __builtin_mxu_d8sum
extern v2u16 __builtin_mxu_d8sumc(v4u8, v4u8);
#define __mxu_d8sumc __builtin_mxu_d8sumc
extern v4u8 __builtin_mxu_q8abd(v4u8, v4u8);
#define __mxu_q8abd __builtin_mxu_q8abd
extern v4i8 __builtin_mxu_q8slt(v4i8, v4i8);
#define __mxu_q8slt __builtin_mxu_q8slt
extern v4u8 __builtin_mxu_q8sltu(v4u8, v4u8);
#define __mxu_q8sltu __builtin_mxu_q8sltu
extern v4i8 __builtin_mxu_q8movz(v4i8, v4i8, v4i8);
#define __mxu_q8movz __builtin_mxu_q8movz
extern v4i8 __builtin_mxu_q8movn(v4i8, v4i8, v4i8);
#define __mxu_q8movn __builtin_mxu_q8movn
extern v4u8 __builtin_mxu_q8avg(v4u8, v4u8);
#define __mxu_q8avg __builtin_mxu_q8avg
extern v4u8 __builtin_mxu_q8avgr(v4u8, v4u8);
#define __mxu_q8avgr __builtin_mxu_q8avgr
/* Shift */
extern v2i16 __builtin_mxu_d32sarl(v1i32, v1i32, int);
#define __mxu_d32sarl __builtin_mxu_d32sarl
extern v2i16 __builtin_mxu_d32sarw(v1i32, v1i32, int);
#define __mxu_d32sarw __builtin_mxu_d32sarw
extern v1i32 __builtin_mxu_s32extr(v1i32, v1i32, int, int);
#define __mxu_s32extr __builtin_mxu_s32extr
extern v1i32 __builtin_mxu_s32extrv(v1i32, v1i32, int, int);
#define __mxu_s32extrv __builtin_mxu_s32extrv

/* MAX/MIN */
extern v1i32 __builtin_mxu_s32max(v1i32, v1i32);
#define __mxu_s32max __builtin_mxu_s32max
extern v1i32 __builtin_mxu_s32min(v1i32, v1i32);
#define __mxu_s32min __builtin_mxu_s32min
extern v2i16 __builtin_mxu_d16max(v2i16, v2i16);
#define __mxu_d16max __builtin_mxu_d16max
extern v2i16 __builtin_mxu_d16min(v2i16, v2i16);
#define __mxu_d16min __builtin_mxu_d16min
extern v4i8 __builtin_mxu_q8max(v4i8, v4i8);
#define __mxu_q8max __builtin_mxu_q8max
extern v4i8 __builtin_mxu_q8min(v4i8, v4i8);
#define __mxu_q8min __builtin_mxu_q8min
/* Register move IU and XRF */
extern int __builtin_mxu_s32m2i_w(v1i32);
#define __mxu_s32m2i_w __builtin_mxu_s32m2i_w
extern int __builtin_mxu_s32m2i_h(v2i16);
#define __mxu_s32m2i_h __builtin_mxu_s32m2i_h
extern int __builtin_mxu_s32m2i_b(v4i8);
#define __mxu_s32m2i_b __builtin_mxu_s32m2i_b 
extern v1i32 __builtin_mxu_s32i2m_w(int);
#define __mxu_s32i2m_w __builtin_mxu_s32i2m_w
extern v2i16 __builtin_mxu_s32i2m_h(int);
#define __mxu_s32i2m_h __builtin_mxu_s32i2m_h
extern v4i8 __builtin_mxu_s32i2m_b(int);
#define __mxu_s32i2m_b __builtin_mxu_s32i2m_b
/* Miscellaneous */
extern v4i8 __builtin_mxu_s32aln_b(v4i8, v4i8, int);
#define __mxu_s32aln_b __builtin_mxu_s32aln_b
extern v2i16 __builtin_mxu_s32aln_h(v2i16, v2i16, int);
#define __mxu_s32aln_h __builtin_mxu_s32aln_h
extern v1i32 __builtin_mxu_s32aln_w(v1i32, v1i32, int);
#define __mxu_s32aln_w __builtin_mxu_s32aln_w
extern v4i8 __builtin_mxu_s32alni_b(v4i8, v4i8, int);
#define __mxu_s32alni_b __builtin_mxu_s32alni_b
extern v2i16 __builtin_mxu_s32alni_h(v2i16, v2i16, int);
#define __mxu_s32alni_h __builtin_mxu_s32alni_h
extern v1i32 __builtin_mxu_s32alni_w(v1i32, v1i32, int);
#define __mxu_s32alni_w __builtin_mxu_s32alni_w
extern v4i8 __builtin_mxu_q16sat(v2i16, v2i16);
#define __mxu_q16sat __builtin_mxu_q16sat
extern v4i8 __builtin_mxu_s32lui_b(int, int);
#define __mxu_s32lui_b __builtin_mxu_s32lui_b
extern v2i16 __builtin_mxu_s32lui_h(int, int);
#define __mxu_s32lui_h __builtin_mxu_s32lui_h
extern v1i32 __builtin_mxu_s32lui_w(int, int);
#define __mxu_s32lui_w __builtin_mxu_s32lui_w

#define __mxu_s32mul(xra,xrd,rs,rt)				\
	do {							\
		__asm__ __volatile ("s32mul %0,%1,%z2,%z3"	\
				    :"=u"((xra)),"=u"((xrd))	\
				    :"d" ((rs)),"d"((rt))	\
				    :"hi","lo");		\
	} while (0)

#define __mxu_s32mulu(xra,xrd,rs,rt)				\
	do {							\
		__asm__ __volatile ("s32mulu %0,%1,%z2,%z3"	\
				    :"=u"((xra)),"=u"((xrd))	\
				    :"d" ((rs)),"d"((rt))	\
				    :"hi","lo");		\
	} while (0)

#define __mxu_s32madd(xra,xrd,rs,rt)				\
	do {							\
		__asm__ __volatile ("s32madd %0,%1,%z2,%z3"	\
				    :"+u"((xra)),"+u"((xrd))	\
				    :"d" ((rs)),"d"((rt))	\
				    :"hi","lo");		\
	} while (0)

#define __mxu_s32maddu(xra,xrd,rs,rt)				\
	do {							\
		__asm__ __volatile ("s32maddu %0,%1,%z2,%z3"	\
				    :"+u"((xra)),"+u"((xrd))	\
				    :"d" ((rs)),"d"((rt))	\
				    :"hi","lo");		\
	} while (0)

#define __mxu_s32msub(xra,xrd,rs,rt)				\
	do {							\
		__asm__ __volatile ("s32msub %0,%1,%z2,%z3"	\
				    :"+u"((xra)),"+u"((xrd))	\
				    :"d" ((rs)),"d"((rt))	\
				    :"hi","lo");		\
	} while (0)

#define __mxu_s32msubu(xra,xrd,rs,rt)				\
	do {							\
		__asm__ __volatile ("s32msubu %0,%1,%z2,%z3"	\
				    :"+u"((xra)),"+u"((xrd))	\
				    :"d" ((rs)),"d"((rt))	\
				    :"hi","lo");		\
	} while (0)

#define __mxu_d16mul(xra,xrb,xrc,xrd,OPTN2)				\
	do {								\
		__asm__ __volatile ("d16mul %0,%2,%3,%1," #OPTN2	\
				    :"=u"((xra)),"=u"((xrd))		\
				    :"u"((xrb)),"u"((xrc)));		\
	} while (0)

#define __mxu_d16mule(xra,xrb,xrc,xrd,OPTN2)				\
	do {								\
		__asm__ __volatile ("d16mule %0,%2,%3,%1," #OPTN2	\
				    :"=u"((xra)),"=u"((xrd))		\
				    :"u"((xrb)),"u"((xrc)));		\
	} while (0)

#define __mxu_d16mac(xra,xrb,xrc,xrd,APTN2,OPTN2)			\
	do {								\
		__asm__ __volatile ("d16mac %0,%2,%3,%1," #APTN2 "," #OPTN2 \
				    :"+u"((xra)),"+u"((xrd))		\
				    :"u"((xrb)),"u"((xrc)));		\
	} while (0)

#define __mxu_d16mace(xra,xrb,xrc,xrd,APTN2,OPTN2)			\
	do {								\
		__asm__ __volatile ("d16mace %0,%2,%3,%1," #APTN2 "," #OPTN2 \
				    :"+u"((xra)),"+u"((xrd))		\
				    :"u"((xrb)),"u"((xrc)));		\
	} while (0)

#define __mxu_q8mul(xra,xrb,xrc,xrd)				\
	do {							\
		__asm__ __volatile ("q8mul %0,%2,%3,%1"		\
				    :"=u"((xra)),"=u"((xrd))	\
				    :"u"((xrb)),"u"((xrc)));	\
	} while (0)

#define __mxu_q8mulsu(xra,xrb,xrc,xrd)				\
	do {							\
		__asm__ __volatile ("q8mulsu %0,%2,%3,%1"	\
				    :"=u"((xra)),"=u"((xrd))	\
				    :"u"((xrb)),"u"((xrc)));	\
	} while (0)

#define __mxu_q8mac(xra,xrb,xrc,xrd,APTN2)			\
	do {							\
		__asm__ __volatile ("q8mac %0,%2,%3,%1," #APTN2	\
				    :"+u"((xra)),"+u"((xrd))	\
				    :"u"((xrb)),"u"((xrc)));	\
	} while (0)

#define __mxu_q8macsu(xra,xrb,xrc,xrd,APTN2)				\
	do {								\
		__asm__ __volatile ("q8macsu %0,%2,%3,%1," #APTN2	\
				    :"+u"((xra)),"+u"((xrd))		\
				    :"u"((xrb)),"u"((xrc)));		\
	} while (0)

#define __mxu_d32add(xra,xrb,xrc,xrd,APTN2)				\
	do {								\
		__asm__ __volatile ("d32add %0,%2,%3,%1," #APTN2	\
				    :"=u"((xra)),"=u"((xrd))		\
				    :"u"((xrb)),"u"((xrc)));		\
	} while (0)

#define __mxu_d32addc(xra,xrb,xrc,xrd)				\
	do {							\
		__asm__ __volatile ("d32addc %0,%2,%3,%1"	\
				    :"+u"((xra)),"+u"((xrd))	\
				    :"u"((xrb)),"u"((xrc)));	\
	} while (0)

#define __mxu_d32acc(xra,xrb,xrc,xrd,APTN2)				\
	do {								\
		__asm__ __volatile ("d32acc %0,%2,%3,%1," #APTN2	\
				    :"+u"((xra)),"+u"((xrd))		\
				    :"u"((xrb)),"u"((xrc)));		\
	} while (0)

#define __mxu_d32accm(xra,xrb,xrc,xrd,APTN2)				\
	do {								\
		__asm__ __volatile ("d32accm %0,%2,%3,%1," #APTN2	\
				    :"+u"((xra)),"+u"((xrd))		\
				    :"u"((xrb)),"u"((xrc)));		\
	} while (0)

#define __mxu_d32asum(xra,xrb,xrc,xrd,APTN2)				\
	do {								\
		__asm__ __volatile ("d32asum %0,%2,%3,%1," #APTN2	\
				    :"+u"((xra)),"+u"((xrd))		\
				    :"u"((xrb)),"u"((xrc)));		\
	} while (0)

#define __mxu_q16add(xra,xrb,xrc,xrd,EPTN2,OPTN2)			\
	do {								\
		__asm__ __volatile ("q16add %0,%2,%3,%1," #EPTN2 "," #OPTN2 \
				    :"=u"((xra)),"=u"((xrd))		\
				    :"u"((xrb)),"u"((xrc)));		\
	} while (0)

#define __mxu_q16acc(xra,xrb,xrc,xrd,EPTN2)				\
	do {								\
		__asm__ __volatile ("q16acc %0,%2,%3,%1," #EPTN2	\
				    :"+u"((xra)),"+u"((xrd))		\
				    :"u"((xrb)),"u"((xrc)));		\
	} while (0)

#define __mxu_q16accm(xra,xrb,xrc,xrd,EPTN2)				\
	do {								\
		__asm__ __volatile ("q16accm %0,%2,%3,%1," #EPTN2	\
				    :"+u"((xra)),"+u"((xrd))		\
				    :"u"((xrb)),"u"((xrc)));		\
	} while (0)

#define __mxu_d16asum(xra,xrb,xrc,xrd,EPTN2)				\
	do {								\
		__asm__ __volatile ("d16asum %0,%2,%3,%1," #EPTN2	\
				    :"+u"((xra)),"+u"((xrd))		\
				    :"u"((xrb)),"u"((xrc)));		\
	} while (0)

#define __mxu_q8adde(xra,xrb,xrc,xrd,EPTN2)				\
	do {								\
		__asm__ __volatile ("q8adde %0,%2,%3,%1," #EPTN2	\
				    :"=u"((xra)),"=u"((xrd))		\
				    :"u"((xrb)),"u"((xrc)));		\
	} while (0)

#define __mxu_q8acce(xra,xrb,xrc,xrd,EPTN2)				\
	do {								\
		__asm__ __volatile ("q8acce %0,%2,%3,%1," #EPTN2	\
				    :"+u"((xra)),"+u"((xrd))		\
				    :"u"((xrb)),"u"((xrc)));		\
	} while (0)

#define __mxu_q8sad(xra,xrb,xrc,xrd)				\
	do {							\
		__asm__ __volatile ("q8sad %0,%2,%3,%1"		\
				    :"=u"((xra)),"+u"((xrd))	\
				    :"u"((xrb)),"u"((xrc)));	\
	} while (0)

#define __mxu_d32sll(xra,xrb,xrc,xrd,SFT4)				\
	do {								\
		__asm__ __volatile ("d32sll %0,%2,%3,%1,%d4"		\
				    :"=u"((xra)),"=u"((xrd))		\
				    :"u"((xrb)),"u"((xrc)),"i"((SFT4))); \
	} while (0)

#define __mxu_d32slr(xra,xrb,xrc,xrd,SFT4)				\
	do {								\
		__asm__ __volatile ("d32slr %0,%2,%3,%1,%d4"		\
				    :"=u"((xra)),"=u"((xrd))		\
				    :"u"((xrb)),"u"((xrc)),"i"((SFT4))); \
	} while (0)

#define __mxu_d32sar(xra,xrb,xrc,xrd,SFT4)				\
	do {								\
		__asm__ __volatile ("d32sar %0,%2,%3,%1,%d4"		\
				    :"=u"((xra)),"=u"((xrd))		\
				    :"u"((xrb)),"u"((xrc)),"i"((SFT4))); \
	} while (0)

#define __mxu_d32sllv(xra,xrd,rb)				\
	do {							\
		__asm__ __volatile ("d32sllv %0,%1,%z2"		\
				    :"+u"((xra)),"+u"((xrd))	\
				    :"d"((rb)));		\
	} while (0)

#define __mxu_d32slrv(xra,xrd,rb)				\
	do {							\
		__asm__ __volatile ("d32slrv %0,%1,%z2"		\
				    :"+u"((xra)),"+u"((xrd))	\
				    :"d"((rb)));		\
	} while (0)

#define __mxu_d32sarv(xra,xrd,rb)				\
	do {							\
		__asm__ __volatile ("d32sarv %0,%1,%z2"		\
				    :"+u"((xra)),"+u"((xrd))	\
				    :"d"((rb)));		\
	} while (0)

#define __mxu_q16sll(xra,xrb,xrc,xrd,SFT4)				\
	do {								\
		__asm__ __volatile ("q16sll %0,%2,%3,%1,%d4"		\
				    :"=u"((xra)),"=u"((xrd))		\
				    :"u"((xrb)),"u"((xrc)),"i"((SFT4))); \
	} while (0)

#define __mxu_q16slr(xra,xrb,xrc,xrd,SFT4)				\
	do {								\
		__asm__ __volatile ("q16slr %0,%2,%3,%1,%d4"		\
				    :"=u"((xra)),"=u"((xrd))		\
				    :"u"((xrb)),"u"((xrc)),"i"((SFT4))); \
	} while (0)

#define __mxu_q16sar(xra,xrb,xrc,xrd,SFT4)				\
	do {								\
		__asm__ __volatile ("q16sar %0,%2,%3,%1,%d4"		\
				    :"=u"((xra)),"=u"((xrd))		\
				    :"u"((xrb)),"u"((xrc)),"i"((SFT4))); \
	} while (0)

#define __mxu_q16sllv(xra,xrd,rb)				\
	do {							\
		__asm__ __volatile ("q16sllv %0,%1,%z2"		\
				    :"+u"((xra)),"+u"((xrd))	\
				    :"d"((rb)));		\
	} while (0)

#define __mxu_q16slrv(xra,xrd,rb)				\
	do {							\
		__asm__ __volatile ("q16slrv %0,%1,%z2"		\
				    :"+u"((xra)),"+u"((xrd))	\
				    :"d"((rb)));		\
	} while (0)

#define __mxu_q16sarv(xra,xrd,rb)				\
	do {							\
		__asm__ __volatile ("q16sarv %0,%1,%z2"		\
				    :"+u"((xra)),"+u"((xrd))	\
				    :"d"((rb)));		\
	} while (0)

#define __mxu_s32sfl(xra,xrb,xrc,xrd,OPTN2)				\
	do {								\
		__asm__ __volatile ("s32sfl %0,%2,%3,%1," #OPTN2	\
				    :"=u"((xra)),"=u"((xrd))		\
				    :"u"((xrb)),"u"((xrc)));		\
	} while (0)

#define __mxu_q16scop(xra,xrb,xrc,xrd)				\
	do {							\
		__asm__ __volatile ("q16scop %0,%2,%3,%1"	\
				    :"=u"((xra)),"=u"((xrd))	\
				    :"u"((xrb)),"u"((xrc)));	\
	} while (0)
//==================Load/Store==================
#define __mxu_s32ldir(xra,rb,s12)				\
	do {							\
		__asm__ __volatile ("s32ldir %0,%1,%2"		\
				    :"=u"((xra)),"+d"((rb))	\
				    :"i"((s12)));		\
	} while (0)

#define __mxu_s32sdir(xra,rb,s12)				\
	do {							\
		__asm__ __volatile ("s32sdir %1,%0,%2"		\
				    :"+d"((rb))			\
				    :"u"((xra)),"i"((s12)));	\
	} while (0)

#define __mxu_s32ldi(xra,rb,s12)				\
	do {							\
		__asm__ __volatile ("s32ldi %0,%1,%2"		\
				    :"=u"((xra)),"+d"((rb))	\
				    :"i"((s12)));		\
	} while (0)

#define __mxu_s32sdi(xra,rb,s12)				\
	do {							\
		__asm__ __volatile ("s32sdi %1,%0,%2"		\
				    :"+d"((rb))			\
				    :"u"((xra)),"i"((s12)));	\
	} while (0)

#define __mxu_s32ldivr(xra,rb,rc,STRD2)				\
	do {							\
		__asm__ __volatile ("s32ldivr %0,%1,%2,%3"	\
				    :"=u"((xra)),"+d"((rb))	\
				    :"d"((rc)),"i"((STRD2)));	\
	} while (0)

#define __mxu_s32sdivr(xra,rb,rc,STRD2)					\
	do {								\
		__asm__ __volatile ("s32sdivr %1,%0,%2,%3"		\
				    :"+d"((rb))				\
				    :"u"((xra)),"d"((rc)),"i"((STRD2))); \
	} while (0)

#define __mxu_s32ldiv(xra,rb,rc,STRD2)				\
	do {							\
		__asm__ __volatile ("s32ldiv %0,%1,%2,%3"	\
				    :"=u"((xra)),"+d"((rb))	\
				    :"d"((rc)),"i"((STRD2)));	\
	} while (0)

#define __mxu_s32sdiv(xra,rb,rc,STRD2)					\
	do {								\
		__asm__ __volatile ("s32sdiv %1,%0,%2,%3"		\
				    :"+d"((rb))				\
				    :"u"((xra)),"d"((rc)),"i"((STRD2))); \
	} while (0)

#define __mxu_s16ldi(xra,rb,s10,OPTN2)					\
	do {								\
		switch(OPTN2){						\
		case 0:							\
		case 1:							\
			__asm__ __volatile ("s16ldi %0,%1,%2," #OPTN2	\
					    :"+u"((xra)),"+d"((rb))	\
					    :"i"((s10)));		\
			break;						\
		case 2:							\
		case 3:							\
			__asm__ __volatile ("s16ldi %0,%1,%2," #OPTN2	\
					    :"=u"((xra)),"+d"((rb))	\
					    :"i"((s10)));		\
			break;						\
		}							\
	} while (0)

#define __mxu_s16sdi(xra,rb,s10,OPTN2)				\
	do {							\
		__asm__ __volatile ("s16sdi %1,%0,%2," #OPTN2	\
				    :"+d"((rb))			\
				    :"u"((xra)),"i"((s10)));	\
	} while (0)

#define __mxu_s8ldi(xra,rb,s8,OPTN3)					\
	do {								\
		switch(OPTN3){						\
		case 0:							\
		case 1:							\
		case 2:							\
		case 3:							\
			__asm__ __volatile ("s8ldi %0,%1,%2," #OPTN3	\
					    :"+u"((xra)),"+d"((rb))	\
					    :"i"((s8)));		\
			break;						\
		case 4:							\
		case 5:							\
		case 6:							\
		case 7:							\
			__asm__ __volatile ("s8ldi %0,%1,%2," #OPTN3	\
					    :"=u"((xra)),"+d"((rb))	\
					    :"i"((s8)));		\
			break;						\
		}							\
	} while (0)

#define __mxu_s8sdi(xra,rb,s8,OPTN3)				\
	do {							\
		__asm__ __volatile ("s8sdi %1,%0,%2," #OPTN3	\
				    :"+d"((rb))			\
				    :"u"((xra)),"i"((s8)));	\
	} while (0)

#endif	/* __clang__ */
#endif	/* define(__mips_mxu) */
#endif	/* _MXU_H */
