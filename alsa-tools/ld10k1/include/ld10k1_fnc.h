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

#ifndef __LD10K1_FNC_H
#define __LD10K1_FNC_H

#define MAX_NAME_LEN 256
#define MAX_CTL_GPR_COUNT 32
#define EMU10K1_GPR_TRANSLATION_LAST EMU10K1_GPR_TRANSLATION_ONOFF

#define EMU10K1_REG_TYPE_NONE 0
#define EMU10K1_REG_TYPE_FX 1
#define EMU10K1_REG_TYPE_INPUT 2
#define EMU10K1_REG_TYPE_OUTPUT 3
#define EMU10K1_REG_TYPE_HW 4
#define EMU10K1_REG_TYPE_TRAM_CTL 5 /* only on Audigy */
#define EMU10K1_REG_TYPE_TRAM_DATA 6
#define EMU10K1_REG_TYPE_TRAM_ADDR 7
#define EMU10K1_REG_TYPE_NORMAL 8
#define EMU10K1_REG_TYPE_CONST 9
#define EMU10K1_REG_TYPE_ALL 10
#define EMU10K1_REG_TYPE_NAMED 11

#define EMU10K1_REG_TYPE_MASK 0xF0000000

/* access macros */
#define EMU10K1_REG_TYPE(type) (((type) << 28) & EMU10K1_REG_TYPE_MASK)
#define EMU10K1_REG_TYPE_B(type) (((type) & EMU10K1_REG_TYPE_MASK) >> 28)
#define EMU10K1_REG_FX(fxnum) (EMU10K1_REG_TYPE(EMU10K1_REG_TYPE_FX) | ((fxnum) & 0x3F))
#define EMU10K1_REG_IN(innum) (EMU10K1_REG_TYPE(EMU10K1_REG_TYPE_INPUT) | ((innum) & 0x1F))
#define EMU10K1_REG_OUT(outnum) (EMU10K1_REG_TYPE(EMU10K1_REG_TYPE_OUTPUT) | ((outnum) & 0x3F))
#define EMU10K1_REG_HW(hwnum) (EMU10K1_REG_TYPE(EMU10K1_REG_TYPE_HW) | ((hwnum) & 0x3F))
#define EMU10K1_REG_TRAM_CTL(ctlnum) (EMU10K1_REG_TYPE(EMU10K1_REG_TYPE_TRAM_CTL) | ((ctlnum) & 0xFF))
#define EMU10K1_REG_TRAM_DATA(datanum) (EMU10K1_REG_TYPE(EMU10K1_REG_TYPE_TRAM_DATA) | ((datanum) & 0xFF))
#define EMU10K1_REG_TRAM_ADDR(addrnum) (EMU10K1_REG_TYPE(EMU10K1_REG_TYPE_TRAM_ADDR) | ((addrnum) & 0xFF))
#define EMU10K1_REG_NORMAL(num) (EMU10K1_REG_TYPE(EMU10K1_REG_TYPE_NORMAL) | ((num) & 0x1FF))
#define EMU10K1_REG_CONST(num) (EMU10K1_REG_TYPE(EMU10K1_REG_TYPE_CONST) | ((num) & 0x2ff))
#define EMU10K1_REG_ALL(num) (EMU10K1_REG_TYPE(EMU10K1_REG_TYPE_ALL) | ((num) & 0x7FF))
#define EMU10K1_REG_NAMED(num) (EMU10K1_REG_TYPE(EMU10K1_REG_TYPE_ALL) | ((num) & 0xFFFFFFF))

/* this will not be changed */
/* FX buses */
#define EMU10K1_NREG_FXBUS_PCM_LEFT			EMU10K1_REG_NAMED(0x0000)
#define EMU10K1_NREG_FXBUS_PCM_RIGHT		EMU10K1_REG_NAMED(0x0001)
#define EMU10K1_NREG_FXBUS_PCM_FRONT_LEFT	EMU10K1_REG_NAMED(0x0002)
#define EMU10K1_NREG_FXBUS_PCM_FRONT_RIGHT	EMU10K1_REG_NAMED(0x0003)
#define EMU10K1_NREG_FXBUS_PCM_REAR_LEFT	EMU10K1_REG_NAMED(0x0004)
#define EMU10K1_NREG_FXBUS_PCM_REAR_RIGHT	EMU10K1_REG_NAMED(0x0005)
#define EMU10K1_NREG_FXBUS_PCM_CENTER		EMU10K1_REG_NAMED(0x0006)
#define EMU10K1_NREG_FXBUS_PCM_LFE			EMU10K1_REG_NAMED(0x0007)
#define EMU10K1_NREG_FXBUS_MIDI_LEFT		EMU10K1_REG_NAMED(0x0008)
#define EMU10K1_NREG_FXBUS_MIDI_RIGHT		EMU10K1_REG_NAMED(0x0009)
#define EMU10K1_NREG_FXBUS_MIDI_REVERB		EMU10K1_REG_NAMED(0x000A)
#define EMU10K1_NREG_FXBUS_MIDI_CHORUS		EMU10K1_REG_NAMED(0x000B)

#define EMU10K1_A_NREG_FXBUS_PT_LEFT		EMU10K1_REG_NAMED(0x000C)
#define EMU10K1_A_NREG_FXBUS_PT_RIGHT		EMU10K1_REG_NAMED(0x000D)

/* inputs */
#define EMU10K1_NREG_IN_AC97_LEFT			EMU10K1_REG_NAMED(0x0100)
#define EMU10K1_NREG_IN_AC97_RIGHT			EMU10K1_REG_NAMED(0x0101)
#define EMU10K1_NREG_IN_SPDIF_CD_LEFT		EMU10K1_REG_NAMED(0x0102)
#define EMU10K1_NREG_IN_SPDIF_CD_RIGHT		EMU10K1_REG_NAMED(0x0103)
#define EMU10K1_NREG_IN_SPDIF_OPT_LEFT		EMU10K1_REG_NAMED(0x0104)
#define EMU10K1_NREG_IN_SPDIF_OPT_RIGHT		EMU10K1_REG_NAMED(0x0105)
#define EMU10K1_NREG_IN_I2S_1_LEFT			EMU10K1_REG_NAMED(0x0106)
#define EMU10K1_NREG_IN_I2S_1_RIGHT			EMU10K1_REG_NAMED(0x0107)
#define EMU10K1_NREG_IN_I2S_2_LEFT			EMU10K1_REG_NAMED(0x0108)
#define EMU10K1_NREG_IN_I2S_2_RIGHT			EMU10K1_REG_NAMED(0x0109)

#define EMU10K1_L_NREG_IN_SPDIF_COAX_LEFT	EMU10K1_REG_NAMED(0x010A)
#define EMU10K1_L_NREG_IN_SPDIF_COAX_RIGHT	EMU10K1_REG_NAMED(0x010B)
#define EMU10K1_L_NREG_IN_ZOOM_LEFT			EMU10K1_REG_NAMED(0x010C)
#define EMU10K1_L_NREG_IN_ZOOM_RIGHT		EMU10K1_REG_NAMED(0x010D)
#define EMU10K1_L_NREG_IN_LINE_1_LEFT		EMU10K1_REG_NAMED(0x010E)
#define EMU10K1_L_NREG_IN_LINE_1_RIGHT		EMU10K1_REG_NAMED(0x010F)
#define EMU10K1_L_NREG_IN_LINE_2_LEFT		EMU10K1_REG_NAMED(0x0110)
#define EMU10K1_L_NREG_IN_LINE_2_RIGHT		EMU10K1_REG_NAMED(0x0111)

#define EMU10K1_A_NREG_IN_LINE_1_LEFT		EMU10K1_REG_NAMED(0x0112)
#define EMU10K1_A_NREG_IN_LINE_1_RIGHT		EMU10K1_REG_NAMED(0x0113)
#define EMU10K1_A_NREG_IN_LINE_2_LEFT		EMU10K1_REG_NAMED(0x0114)
#define EMU10K1_A_NREG_IN_LINE_2_RIGHT		EMU10K1_REG_NAMED(0x0115)
#define EMU10K1_A_NREG_IN_LINE_3_LEFT		EMU10K1_REG_NAMED(0x0116)
#define EMU10K1_A_NREG_IN_LINE_3_RIGHT		EMU10K1_REG_NAMED(0x0117)
/* outputs */
#define EMU10K1_NREG_OUT_FRONT_LEFT			EMU10K1_REG_NAMED(0x0200)
#define EMU10K1_NREG_OUT_FRONT_RIGHT		EMU10K1_REG_NAMED(0x0201)
#define EMU10K1_NREG_OUT_REAR_LEFT			EMU10K1_REG_NAMED(0x0202)
#define EMU10K1_NREG_OUT_REAR_RIGHT			EMU10K1_REG_NAMED(0x0203)
#define EMU10K1_NREG_OUT_CENTER				EMU10K1_REG_NAMED(0x0204)
#define EMU10K1_NREG_OUT_LFE				EMU10K1_REG_NAMED(0x0205)
#define EMU10K1_NREG_OUT_AC97_LEFT			EMU10K1_REG_NAMED(0x0206)
#define EMU10K1_NREG_OUT_AC97_RIGHT			EMU10K1_REG_NAMED(0x0207)
#define EMU10K1_NREG_OUT_ADC_LEFT			EMU10K1_REG_NAMED(0x0208)
#define EMU10K1_NREG_OUT_ADC_RIGHT			EMU10K1_REG_NAMED(0x0209)
#define EMU10K1_NREG_OUT_MIC				EMU10K1_REG_NAMED(0x020A)
#define EMU10K1_NREG_OUT_HEADPHONE_LEFT		EMU10K1_REG_NAMED(0x020B)
#define EMU10K1_NREG_OUT_HEADPHONE_RIGHT	EMU10K1_REG_NAMED(0x020C)

#define EMU10K1_L_NREG_OUT_OPT_LEFT			EMU10K1_REG_NAMED(0x020D)
#define EMU10K1_L_NREG_OUT_OPT_RIGHT		EMU10K1_REG_NAMED(0x020E)

#define EMU10K1_A_NREG_OUT_D_FRONT_LEFT		EMU10K1_REG_NAMED(0x020F)
#define EMU10K1_A_NREG_OUT_D_FRONT_RIGHT	EMU10K1_REG_NAMED(0x0210)
#define EMU10K1_A_NREG_OUT_D_REAR_LEFT		EMU10K1_REG_NAMED(0x0211)
#define EMU10K1_A_NREG_OUT_D_REAR_RIGHT		EMU10K1_REG_NAMED(0x0212)
#define EMU10K1_A_NREG_OUT_D_CENTER			EMU10K1_REG_NAMED(0x0213)
#define EMU10K1_A_NREG_OUT_D_LFE			EMU10K1_REG_NAMED(0x0214)

/* hardware */
#define EMU10K1_NREG_CONST_00000000			EMU10K1_REG_NAMED(0x0300)
#define EMU10K1_NREG_CONST_00000001			EMU10K1_REG_NAMED(0x0301)
#define EMU10K1_NREG_CONST_00000002			EMU10K1_REG_NAMED(0x0302)
#define EMU10K1_NREG_CONST_00000003			EMU10K1_REG_NAMED(0x0303)
#define EMU10K1_NREG_CONST_00000004			EMU10K1_REG_NAMED(0x0304)
#define EMU10K1_NREG_CONST_00000008			EMU10K1_REG_NAMED(0x0305)
#define EMU10K1_NREG_CONST_00000010			EMU10K1_REG_NAMED(0x0306)
#define EMU10K1_NREG_CONST_00000020			EMU10K1_REG_NAMED(0x0307)
#define EMU10K1_NREG_CONST_00000100			EMU10K1_REG_NAMED(0x0308)
#define EMU10K1_NREG_CONST_00010000			EMU10K1_REG_NAMED(0x0309)
#define EMU10K1_L_NREG_CONST_00080000		EMU10K1_REG_NAMED(0x030A)
#define EMU10K1_A_NREG_CONST_00000800		EMU10K1_REG_NAMED(0x030B)
#define EMU10K1_NREG_CONST_10000000			EMU10K1_REG_NAMED(0x030C)
#define EMU10K1_NREG_CONST_20000000			EMU10K1_REG_NAMED(0x030D)
#define EMU10K1_NREG_CONST_40000000			EMU10K1_REG_NAMED(0x030E)
#define EMU10K1_NREG_CONST_80000000			EMU10K1_REG_NAMED(0x030F)
#define EMU10K1_NREG_CONST_7FFFFFFF			EMU10K1_REG_NAMED(0x0310)
#define EMU10K1_NREG_CONST_FFFFFFFF			EMU10K1_REG_NAMED(0x0311)
#define EMU10K1_NREG_CONST_FFFFFFFE			EMU10K1_REG_NAMED(0x0312)
#define EMU10K1_NREG_CONST_C0000000			EMU10K1_REG_NAMED(0x0313)
#define EMU10K1_NREG_CONST_4F1BBCDC			EMU10K1_REG_NAMED(0x0314)
#define EMU10K1_NREG_CONST_5A7EF9DB			EMU10K1_REG_NAMED(0x0315)
#define EMU10K1_NREG_CONST_00100000			EMU10K1_REG_NAMED(0x0316)

#define EMU10K1_NREG_HW_ACCUM				EMU10K1_REG_NAMED(0x0317)
#define EMU10K1_NREG_HW_CCR					EMU10K1_REG_NAMED(0x0318)
#define EMU10K1_NREG_HW_NOISE1				EMU10K1_REG_NAMED(0x0319)
#define EMU10K1_NREG_HW_NOISE2				EMU10K1_REG_NAMED(0x031A)
#define EMU10K1_NREG_HW_IRQ					EMU10K1_REG_NAMED(0x031B)
#define EMU10K1_NREG_HW_DBAC				EMU10K1_REG_NAMED(0x031C)
#define EMU10K1_A_NREG_HW_DBACE				EMU10K1_REG_NAMED(0x031D)

/* patch registers */
#define EMU10K1_PREG_TYPE_IN 1
#define EMU10K1_PREG_TYPE_OUT 2
#define EMU10K1_PREG_TYPE_CONST 3
#define EMU10K1_PREG_TYPE_STA 4
#define EMU10K1_PREG_TYPE_DYN 5
#define EMU10K1_PREG_TYPE_HW 6
#define EMU10K1_PREG_TYPE_CTL 7
#define EMU10K1_PREG_TYPE_TRAM_DATA 8
#define EMU10K1_PREG_TYPE_TRAM_ADDR 9

#define EMU10K1_PREG_TYPE_MASK 0xF0000000

/* access macros */
#define EMU10K1_PREG_TYPE(type) (((type) << 28) & EMU10K1_PREG_TYPE_MASK)
#define EMU10K1_PREG_TYPE_B(type) (((type) & EMU10K1_PREG_TYPE_MASK) >> 28)
#define EMU10K1_PREG_IN(num) (EMU10K1_REG_TYPE(EMU10K1_PREG_TYPE_IN) | ((num) & 0x1F))
#define EMU10K1_PREG_OUT(num) (EMU10K1_REG_TYPE(EMU10K1_PREG_TYPE_OUT) | ((num) & 0x1F))
#define EMU10K1_PREG_CONST(num) (EMU10K1_REG_TYPE(EMU10K1_PREG_TYPE_CONST) | ((num) & 0xFF))
#define EMU10K1_PREG_STA(num) (EMU10K1_REG_TYPE(EMU10K1_PREG_TYPE_STA) | ((num) & 0xFF))
#define EMU10K1_PREG_DYN(num) (EMU10K1_REG_TYPE(EMU10K1_PREG_TYPE_DYN) | ((num) & 0xFF))
#define EMU10K1_PREG_HW(num) (EMU10K1_REG_TYPE(EMU10K1_PREG_TYPE_HW) | ((num) & 0xFF))
#define EMU10K1_PREG_CTL(ctlnum, num) (EMU10K1_REG_TYPE(EMU10K1_PREG_TYPE_CTL) | (((ctlnum) & 0xFF) << 8) | ((num) & 0xFF))
#define EMU10K1_PREG_TRAM_DATA(num) (EMU10K1_REG_TYPE(EMU10K1_PREG_TYPE_TRAM_DATA) | ((num) & 0xFF))
#define EMU10K1_PREG_TRAM_ADDR(num) (EMU10K1_REG_TYPE(EMU10K1_PREG_TYPE_TRAM_ADDR) | ((num) & 0xFF))

typedef struct {
	char name[MAX_NAME_LEN];
} ld10k1_dsp_p_in_out_t;

typedef struct {
	unsigned int const_val;
} ld10k1_dsp_p_const_static_t;

typedef struct {
	unsigned int hw_val;
} ld10k1_dsp_p_hw_t;


#define EMU10K1_GPR_TRANSLATION_NONE		0
#define EMU10K1_GPR_TRANSLATION_TABLE100	1
#define EMU10K1_GPR_TRANSLATION_BASS		2
#define EMU10K1_GPR_TRANSLATION_TREBLE		3
#define EMU10K1_GPR_TRANSLATION_ONOFF		4

typedef struct {
	char name[44];
	int index;			/* -1 - auto choose index */
	unsigned int vcount;		/* count of GPR (1..32) */
	unsigned int count;			/* count of GPR (1..32) */
	unsigned int value[MAX_CTL_GPR_COUNT];
	unsigned int min;			/* minimum range */
	unsigned int max;			/* maximum range */
	unsigned int translation;
} ld10k1_dsp_ctl_t;

typedef struct {
	unsigned int op_code;
	unsigned int arg[4];
} ld10k1_dsp_instr_t;

#define TRAM_GRP_DELAY 1
#define TRAM_GRP_TABLE 2

#define TRAM_POS_NONE 0
#define TRAM_POS_AUTO 1
#define TRAM_POS_INTERNAL 2
#define TRAM_POS_EXTERNAL 3

typedef struct {
	unsigned int grp_type;
	unsigned int grp_size;
	unsigned int grp_pos;
} ld10k1_dsp_tram_grp_t;

#define TRAM_ACC_READ 1
#define TRAM_ACC_WRITE 2
#define TRAM_ACC_ZERO 4

typedef struct {
	unsigned int acc_type;
	unsigned int acc_offset;
	unsigned int grp;
} ld10k1_dsp_tram_acc_t;

typedef struct {
	char patch_name[MAX_NAME_LEN];
	int id;
	unsigned int in_count;
	unsigned int out_count;
	unsigned int const_count;
	unsigned int static_count;
	unsigned int dynamic_count;
	unsigned int hw_count;
	unsigned int tram_count;
	unsigned int tram_acc_count;
	unsigned int ctl_count;
	unsigned int instr_count;
} ld10k1_dsp_patch_t;


#define CON_IO_FX 'F'
#define CON_IO_IN 'I'
#define CON_IO_OUT 'O'
#define CON_IO_PIN 'A'
#define CON_IO_POUT 'B'
#define CON_IO_NORMAL '\0'

/* must be changed in ld10k1.h too */
#define POINT_MAX_CONN_PER_POINT 15
typedef struct {
	int id;
	int type;
	int io_idx;
	int simple;
	int multi;
	unsigned int conn_count;
	int io_type[POINT_MAX_CONN_PER_POINT];
	int patch[POINT_MAX_CONN_PER_POINT];
	int io[POINT_MAX_CONN_PER_POINT];
} ld10k1_dsp_point_t;

typedef struct {
	int where;
	ld10k1_dsp_patch_t patch;
} ld10k1_fnc_patch_add_t;

typedef struct {
	int where;
} ld10k1_fnc_patch_del_t;

typedef struct {
	int what;
	int multi;
	int simple;
	int from_type;
	int from_patch;
	int from_io;
	int to_type;
	int to_patch;
	int to_io;
} ld10k1_fnc_connection_t;

typedef struct {
	int patch_num;
	int gpr;
	char name[MAX_NAME_LEN];
} ld10k1_fnc_name_t;

typedef struct {
	char name[MAX_NAME_LEN];
} ld10k1_fnc_get_io_t;

typedef struct {
	int patch_num;
	int id;
	char patch_name[MAX_NAME_LEN];
} ld10k1_fnc_patches_info_t;

typedef struct {
	char ld10k1_version[MAX_NAME_LEN];
} ld10k1_fnc_version_t;

#define CHIP_LIVE 0
#define CHIP_AUDIGY 1

typedef struct {
	unsigned int chip_type;
} ld10k1_fnc_dsp_info_t;

#define FNC_PATCH_ADD 1
#define FNC_PATCH_DEL 2

#define FNC_CONNECTION_ADD 3
#define FNC_CONNECTION_DEL 4

#define FNC_PATCH_RENAME 5
#define FNC_PATCH_FIND 6

#define FNC_GET_FX 11
#define FNC_GET_IN 12
#define FNC_GET_OUT 13
#define FNC_GET_PIN 14
#define FNC_GET_POUT 15

#define FNC_GET_FX_COUNT 21
#define FNC_GET_IN_COUNT 22
#define FNC_GET_OUT_COUNT 23
#define FNC_GET_PIN_COUNT 24
#define FNC_GET_POUT_COUNT 25

#define FNC_FX_RENAME 30
#define FNC_IN_RENAME 31
#define FNC_OUT_RENAME 32
#define FNC_PATCH_IN_RENAME 33
#define FNC_PATCH_OUT_RENAME 34


#define FNC_GET_PATCHES_INFO 40
#define FNC_GET_PATCH 41

#define FNC_FX_FIND 50
#define FNC_IN_FIND 51
#define FNC_OUT_FIND 52
#define FNC_PATCH_IN_FIND 53
#define FNC_PATCH_OUT_FIND 54

#define FNC_DUMP 60

#define FNC_GET_POINTS_INFO 70
#define FNC_GET_POINT_INFO 71

#define FNC_GET_DSP_INFO 97

#define FNC_VERSION 98
#define FNC_DSP_INIT 99

#define FNC_OK 100
#define FNC_ERR 101
#define FNC_CONTINUE 102
#define FNC_CLOSE_CONN 103

#define FNC_DEBUG 200

#endif /* __LD10K1_FNC_H */
