/*
 *  EMU10k1 loader lib
 *  Copyright (c) 2003,2004 by Peter Zubaj
 *
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as
 *   published by the Free Software Foundation; either version 2.1 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef __LD10K1_ERROR_H
#define __LD10K1_ERROR_H

#define LD10K1_ERR_UNKNOWN -1 /* unknown error */
#define LD10K1_ERR_COMM_READ -2 /* errorn in read from socket */
#define LD10K1_ERR_COMM_WRITE -3 /* error in write to socket */
#define LD10K1_ERR_UNKNOWN_PATCH_NUM -4 /* wrong parameter - patch with this num doesn't exists */
#define LD10K1_ERR_PROTOCOL -5 /* ld10k1 is expecting more or less data as it got */
#define LD10K1_ERR_COMM_CONN -29 /*  */

#define LD10K1_ERR_PROTOCOL_IN_COUNT -6 /* */
#define LD10K1_ERR_PROTOCOL_OUT_COUNT -7 /* */
#define LD10K1_ERR_PROTOCOL_CONST_COUNT -8 /* */
#define LD10K1_ERR_PROTOCOL_STATIC_COUNT -9 /* */
#define LD10K1_ERR_PROTOCOL_DYNAMIC_COUNT -10 /* */
#define LD10K1_ERR_PROTOCOL_HW_COUNT -11 /* */
#define LD10K1_ERR_PROTOCOL_TRAM_COUNT -12 /* */
#define LD10K1_ERR_PROTOCOL_TRAM_ACC_COUNT -13 /* */
#define LD10K1_ERR_PROTOCOL_CTL_COUNT -14 /* */
#define LD10K1_ERR_PROTOCOL_INSTR_COUNT -15 /* */

/* driver */
#define LD10K1_ERR_DRIVER_CODE_POKE -16 /* unable to poke code */
#define LD10K1_ERR_DRIVER_INFO -17 /* unable to get info */
#define LD10K1_ERR_DRIVER_CODE_PEEK -18 /* unable to peek code */
#define LD10K1_ERR_DRIVER_PCM_POKE -19 /* unable to poke pcm */

/* tram */
#define LD10K1_ERR_ITRAM_FULL -20 /* not enought free itram */
#define LD10K1_ERR_ETRAM_FULL -21 /* not enought free etram */
#define LD10K1_ERR_TRAM_FULL -22 /* not enought free tram */
#define LD10K1_ERR_TRAM_FULL_GRP -23 /* not enought free tram group */

#define LD10K1_ERR_ITRAM_FULL_ACC -25 /* not enought free itram acc */
#define LD10K1_ERR_ETRAM_FULL_ACC -26 /* not enought free etram acc */
#define LD10K1_ERR_TRAM_FULL_ACC -27 /* not enought free tram acc */

#define LD10K1_ERR_MAX_CON_PER_POINT -28 /* maximum connections per point reached */

/* others */
#define LD10K1_ERR_NO_MEM -30 /* not enought free mem */
#define LD10K1_ERR_MAX_PATCH_COUNT -31 /* max patch count excesed */
#define LD10K1_ERR_NOT_FREE_REG -32 /* there is not free reg */
#define LD10K1_ERR_NOT_FREE_INSTR -34 /* there is no free instruction slot */

/* patch chceck */
#define LD10K1_ERR_WRONG_REG_HW_INDEX -36 /*  */
#define LD10K1_ERR_WRONG_TRAM_POS -37 /*  */

#define LD10K1_ERR_WRONG_TRAM_TYPE -39 /*  */
#define LD10K1_ERR_WRONG_TRAM_SIZE -40 /*  */
#define LD10K1_ERR_WRONG_TRAM_ACC_TYPE -41 /*  */

#define LD10K1_ERR_TRAM_GRP_OUT_OF_RANGE -42 /*  */
#define LD10K1_ERR_TRAM_ACC_OUT_OF_RANGE -43 /*  */

#define LD10K1_ERR_CTL_VCOUNT_OUT_OF_RANGE -48 /*  */
#define LD10K1_ERR_CTL_COUNT_OUT_OF_RANGE -49 /*  */

#define LD10K1_ERR_CTL_MIN_MAX_RANGE -50 /*  */
#define LD10K1_ERR_CTL_TRANLSLATION -51 /*  */
#define LD10K1_ERR_CTL_REG_INDEX -52 /*  */
#define LD10K1_ERR_CTL_REG_VALUE -53 /*  */

#define LD10K1_ERR_INSTR_OPCODE -54 /*  */
#define LD10K1_ERR_INSTR_ARG_INDEX -56 /*  */

#define LD10K1_ERR_UNKNOWN_REG_NUM -57 /* */
#define LD10K1_ERR_UNKNOWN_PATCH_REG_NUM -58 /* */

#define LD10K1_ERR_CONNECTION -59 /* can't connect */
#define LD10K1_ERR_CONNECTION_FNC -60 /* wrong connection funcion requested */

#define LD10K1_ERR_CTL_EXISTS -61 /*  */

#define LD10K1_ERR_PATCH_RENAME -62 /*  */
#define LD10K1_ERR_PATCH_REG_RENAME -63 /*  */
#define LD10K1_ERR_REG_RENAME -64 /*  */
#define LD10K1_ERR_WRONG_VER -65 /* wrong ld10k1 <=> lo10k1 version */

#define LD10K1_ERR_UNKNOWN_POINT -66 /*  */

#endif /* __LD10K1_ERROR_H */
