/*
 *  EMU10k1 loader
 *
 *  Copyright (c) 2003,2004 by Peter Zubaj
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef __LD10K1_H
#define __LD10K1_H

#define MAX_CONST_COUNT 0x220
#define MAX_GPR_COUNT 0x200
#define MAX_TRAM_COUNT 0x100

/* instructions */
typedef struct {
	unsigned int used: 1,
		modified: 1;
	unsigned int op_code;
	unsigned int arg[4];
} ld10k1_instr_t;

/* tram */
typedef struct {
	unsigned int used: 1,
		type: 2,
		req_pos: 2,
		pos: 2;
	unsigned int size;
	unsigned int offset;
	int acc_count;
} ld10k1_tram_grp_t;

#define TRAM_OP_NULL 0
#define TRAM_OP_READ 1
#define TRAM_OP_WRITE 2

typedef struct {
	unsigned int used: 1,
		modified: 1;
	unsigned int op;
	unsigned int addr_val;
	unsigned int data_val;
} ld10k1_tram_hwacc_t;

typedef struct {
	unsigned int used: 1,
		type: 2;
	unsigned int offset;
	unsigned int hwacc;
	unsigned int grp;
} ld10k1_tram_acc_t;

typedef struct {
	unsigned int size;
	unsigned int max_hwacc;
	ld10k1_tram_hwacc_t *hwacc;
	unsigned int used_hwacc;
} ld10k1_tram_t;

#define MAX_CONN_PER_POINT 15
#define MAX_INSTR_PER_POINT 7

#define INSERT_BEFORE_OWNER 0
#define INSERT_AFTER_OWNER 1

typedef struct ld10k1_conn_point_tag
{
	struct ld10k1_conn_point_tag *next;

	int id;
	int con_count; /* count of io connected to this point */
	unsigned int con_gpr_idx;
	int type[MAX_CONN_PER_POINT];
	struct ld10k1_patch_tag *patch[MAX_CONN_PER_POINT];
	int io[MAX_CONN_PER_POINT];
	unsigned int out_gpr_idx[MAX_CONN_PER_POINT];

	int simple;

	int reserved_gpr;
	int reserved_instr;

	struct ld10k1_patch_tag *owner;
	int position;

	unsigned int out_instr_offset;
	ld10k1_instr_t out_instr[MAX_INSTR_PER_POINT];
} ld10k1_conn_point_t;

typedef struct {
	char *name;
	ld10k1_conn_point_t *point;
} ld10k1_p_in_out_t;

typedef struct {
	unsigned int gpr_idx;
	unsigned int const_val;
} ld10k1_p_const_sta_t;

typedef struct {
	unsigned int gpr_idx;
} ld10k1_p_dyn_t;

typedef struct {
	unsigned int reg_idx;
	unsigned int gpr_idx;
} ld10k1_p_hw_t;

typedef struct {
	unsigned int grp_type;
	unsigned int grp_size;
	unsigned int grp_pos;
	unsigned int grp_idx;
} ld10k1_p_tram_grp_t;

typedef struct {
	unsigned int acc_type;
	unsigned int acc_offset;
	unsigned int acc_idx;
	unsigned int grp;
} ld10k1_p_tram_acc_t;

typedef struct {
	char name[44];
	int index;
	int want_index;
	unsigned int vcount;		/* count of GPR (1..32) */
	unsigned int count;			/* count of GPR (1..32) */
	unsigned int gpr_idx[32];	/* GPR number(s) */
	unsigned int value[32];
	unsigned int min;			/* minimum range */
	unsigned int max;			/* maximum range */
	unsigned int translation;	/* typ - 0 - bool, num 1 - enum */
} ld10k1_ctl_t;

typedef struct ld10k1_ctl_list_item_tag {
	struct ld10k1_ctl_list_item_tag *next;
	ld10k1_ctl_t ctl;
} ld10k1_ctl_list_item_t;

typedef struct ld10k1_patch_tag {
	char *patch_name;
	int order;
	int id;

	unsigned int in_count;
	ld10k1_p_in_out_t *ins;

	unsigned int out_count;
	ld10k1_p_in_out_t *outs;

	unsigned int const_count;
	ld10k1_p_const_sta_t *consts;

	unsigned int sta_count;
	ld10k1_p_const_sta_t *stas;

	unsigned int dyn_count;
	ld10k1_p_dyn_t *dyns;

	unsigned int hw_count;
	ld10k1_p_hw_t *hws;

	unsigned int tram_count;
	ld10k1_p_tram_grp_t *tram_grp;

	unsigned int tram_acc_count;
	ld10k1_p_tram_acc_t *tram_acc;

	unsigned int ctl_count;
	ld10k1_ctl_t *ctl;

	unsigned int instr_count;
	unsigned int instr_offset;
	ld10k1_instr_t *instr;
} ld10k1_patch_t;

#define EMU10K1_PATCH_MAX 128

typedef struct {
	unsigned int gpr_idx;
	unsigned int const_val;
	unsigned int hw;
	unsigned int ref;
	unsigned int used: 1;
} ld10k1_dsp_const_t;

#define GPR_USAGE_NONE 0
#define GPR_USAGE_NORMAL 1
#define GPR_USAGE_CONST 2
#define GPR_USAGE_DYNAMIC 3

typedef struct {
	unsigned int gpr_usage;
	unsigned int val;
	unsigned int ref;
	unsigned int modified: 1,
		used: 1;
} ld10k1_dsp_gpr_t;

/* reserved ctls - for example AC97 */

typedef struct {
	char name[44];
	unsigned int index;
} ld10k1_reserved_ctl_t;

typedef struct ld10k1_reserved_ctl_list_item_tag {
	struct ld10k1_reserved_ctl_list_item_tag *next;
	ld10k1_reserved_ctl_t res_ctl;
} ld10k1_reserved_ctl_list_item_t;

typedef struct {
	int audigy;
	const char *card_id;

	/* registers */
	unsigned int fx_count;
	ld10k1_p_in_out_t fxs[0x40];

	unsigned int in_count;
	ld10k1_p_in_out_t ins[0x20];

	unsigned int out_count;
	ld10k1_p_in_out_t outs[0x40];

	unsigned int consts_max_count;
	ld10k1_dsp_const_t consts[MAX_CONST_COUNT];

	unsigned int regs_max_count;
	ld10k1_dsp_gpr_t regs[MAX_GPR_COUNT];

	/* instructions */
	unsigned int instr_count;
	ld10k1_instr_t instr[1024];

	unsigned int instr_free;

	/* internal tram */
	ld10k1_tram_t i_tram;

	/* external tram */
	ld10k1_tram_t e_tram;

	unsigned int max_tram_grp;
	ld10k1_tram_grp_t tram_grp[MAX_TRAM_COUNT];

	unsigned int max_tram_acc;
	ld10k1_tram_acc_t tram_acc[MAX_TRAM_COUNT];

	unsigned int max_itram_hwacc;
	ld10k1_tram_hwacc_t itram_hwacc[0xC0];
	unsigned int max_etram_hwacc;
	ld10k1_tram_hwacc_t etram_hwacc[0x40];

	unsigned int patch_count;
	ld10k1_patch_t *patch_ptr[EMU10K1_PATCH_MAX];
	unsigned int patch_order[EMU10K1_PATCH_MAX];

	unsigned short patch_id_gens[EMU10K1_PATCH_MAX];

	ld10k1_ctl_list_item_t *add_ctl_list;
	int add_list_count;

	ld10k1_ctl_list_item_t *del_ctl_list;
	int del_list_count;

	ld10k1_ctl_list_item_t *ctl_list;
	int ctl_list_count;
	
	ld10k1_reserved_ctl_list_item_t *reserved_ctl_list;

	ld10k1_conn_point_t *point_list;
} ld10k1_dsp_mgr_t;

void error(const char *fmt,...);
#endif /* __LD10K1_H */
