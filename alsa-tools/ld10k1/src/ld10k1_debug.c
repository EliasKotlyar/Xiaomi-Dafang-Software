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

#include <alsa/asoundlib.h>
#include "ld10k1.h"
#include "ld10k1_fnc.h"
#include "ld10k1_fnc1.h"
#include "ld10k1_debug.h"
#include "ld10k1_error.h"
#include "ld10k1_tram.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static char *usage_str[] = {
	"NONE",
	"NORMAL",
	"CONST",
	"DYNAMIC"
};

char debug_line[1000];
int send_debug_line(int data_conn)
{
	return send_response(data_conn, FNC_CONTINUE, 0, debug_line, strlen(debug_line) + 1);
}

int ld10k1_debug_new_gpr_read_one(int data_conn, ld10k1_dsp_mgr_t *dsp_mgr, unsigned int idx)
{
	int usage;
	int value;
	int ref_count;
	int modified;

	modified = dsp_mgr->regs[idx].modified;
	usage = dsp_mgr->regs[idx].gpr_usage;
	value = dsp_mgr->regs[idx].val;
	ref_count = dsp_mgr->regs[idx].ref;

	sprintf(debug_line, "%c 0x%03x : %-12s  0x%08x  %3d\n",
			modified ? '*' : ' ',
			idx,
			usage_str[usage],
			value,
			ref_count);
	return send_debug_line(data_conn);
}

int ld10k1_debug_new_gpr_read_hdr(int data_conn)
{
	sprintf(debug_line, "M Idx     Usage         Value         Ref\n");
	return send_debug_line(data_conn);
}

static int ld10k1_debug_new_gpr_read(int data_conn, ld10k1_dsp_mgr_t *dsp_mgr)
{
	int i;
	int err;

	sprintf(debug_line, "FX8010 GPR List\n");
	if ((err = send_debug_line(data_conn)) < 0)
		return err;
	if ((err = ld10k1_debug_new_gpr_read_hdr(data_conn)) < 0)
		return err;
	for (i = 0; i < dsp_mgr->regs_max_count; i++)
		if (dsp_mgr->regs[i].used)
			if ((err = ld10k1_debug_new_gpr_read_one(data_conn, dsp_mgr, i)) < 0)
				return err;
	return 0;
}

static int ld10k1_debug_new_fx_read(int data_conn, ld10k1_dsp_mgr_t *dsp_mgr)
{
	int i;
	int err;

	sprintf(debug_line, "FX8010 FX List\n");
	if ((err = send_debug_line(data_conn)) < 0)
		return err;
	for (i = 0; i < dsp_mgr->fx_count; i++) {
		sprintf(debug_line, "%03x : %-20s\n",
			i,
			dsp_mgr->fxs[i].name ? dsp_mgr->fxs[i].name : "");
		if ((err = send_debug_line(data_conn)) < 0)
			return err;
	}
	return 0;
}

static int ld10k1_debug_new_in_read(int data_conn, ld10k1_dsp_mgr_t *dsp_mgr)
{
	int i;
	int err;

	sprintf(debug_line, "FX8010 IN List\n");
	if ((err = send_debug_line(data_conn)) < 0)
		return err;
	for (i = 0; i < dsp_mgr->in_count; i++) {
		sprintf(debug_line, "%03x : %-20s\n",
			i,
			dsp_mgr->ins[i].name ? dsp_mgr->ins[i].name : "");
		if ((err = send_debug_line(data_conn)) < 0)
			return err;
	}
	return 0;
}

static int ld10k1_debug_new_out_read(int data_conn, ld10k1_dsp_mgr_t *dsp_mgr)
{
	int i;
	int err;

	sprintf(debug_line, "FX8010 OUT List\n");
	if ((err = send_debug_line(data_conn)) < 0)
		return err;
	for (i = 0; i < dsp_mgr->out_count; i++) {
		sprintf(debug_line, "%03x : %-20s\n",
			i,
			dsp_mgr->outs[i].name ? dsp_mgr->outs[i].name : "");
		if ((err = send_debug_line(data_conn)) < 0)
			return err;
	}
	return 0;
}

int ld10k1_debug_new_const_read_one(int data_conn, ld10k1_dsp_mgr_t *dsp_mgr, unsigned int idx)
{
	int hw;
	int value;
	int ref_count;

	hw = dsp_mgr->consts[idx].hw;
	value = dsp_mgr->consts[idx].const_val;
	ref_count = dsp_mgr->consts[idx].ref;

	sprintf(debug_line, "0x%03x : 0x%08x  %c  %3d\n",
			idx,
			value,
			hw ? '*' : ' ',
			ref_count);
	return send_debug_line(data_conn);
}

int ld10k1_debug_new_const_read_hdr(int data_conn)
{
	sprintf(debug_line, "Idx     Value       HW   Ref\n");
	return send_debug_line(data_conn);
}

static int ld10k1_debug_new_const_read(int data_conn, ld10k1_dsp_mgr_t *dsp_mgr)
{
	int i;
	int err;

	sprintf(debug_line, "CONST List\n");
	if ((err = send_debug_line(data_conn)) < 0)
		return err;
	if ((err = ld10k1_debug_new_const_read_hdr(data_conn)) < 0)
		return err;
	for (i = 0; i < dsp_mgr->consts_max_count; i++)
		if (dsp_mgr->consts[i].used)
			if ((err = ld10k1_debug_new_const_read_one(data_conn, dsp_mgr, i)) < 0)
				return err;
	return 0;
}

char *instr_name[] = {
	"MACS",
	"MACS1",
	"MACW",
	"MACW1",
	"MACINTS",
	"MACINTW",
	"ACC3",
	"MACMV",
	"ANDXOR",
	"TSTNEG",
	"LIMIT",
	"LIMIT1",
	"LOG",
	"EXP",
	"INTERP",
	"SKIP",
};

static void ld10k1_debug_decode_preg_idx(char *type, unsigned int reg)
{
	switch ((reg & EMU10K1_PREG_TYPE_MASK) >> 28) {
		case EMU10K1_PREG_TYPE_IN:
			sprintf(type, "IN(%03d)", reg & ~EMU10K1_PREG_TYPE_MASK);
			break;
		case EMU10K1_PREG_TYPE_OUT:
			sprintf(type, "OUT(%03d)", reg & ~EMU10K1_PREG_TYPE_MASK);
			break;
		case EMU10K1_PREG_TYPE_CONST:
			sprintf(type, "CON(%03d)", reg & ~EMU10K1_PREG_TYPE_MASK);
			break;
		case EMU10K1_PREG_TYPE_STA:
			sprintf(type, "STA(%03d)", reg & ~EMU10K1_PREG_TYPE_MASK);
			break;
		case EMU10K1_PREG_TYPE_DYN:
			sprintf(type, "DYN(%03d)", reg & ~EMU10K1_PREG_TYPE_MASK);
			break;
		case EMU10K1_PREG_TYPE_HW:
			sprintf(type, "HW(%03d)", reg & ~EMU10K1_PREG_TYPE_MASK);
			break;
		case EMU10K1_PREG_TYPE_CTL:
			sprintf(type, "CTL(%03d, %03d)", (reg & ~EMU10K1_PREG_TYPE_MASK) >> 8, reg & ~EMU10K1_PREG_TYPE_MASK & 0xFF);
			break;
		case EMU10K1_PREG_TYPE_TRAM_DATA:
			sprintf(type, "TD(%03d)", reg & ~EMU10K1_PREG_TYPE_MASK);
			break;
		case EMU10K1_PREG_TYPE_TRAM_ADDR:
			sprintf(type, "TA(%03d)", reg & ~EMU10K1_PREG_TYPE_MASK);
			break;
		default:
			sprintf(type, "??? 0x%08x", reg);
	}
}

int ld10k1_debug_new_code_read_one(int data_conn, int preg, ld10k1_instr_t *instr, unsigned int idx)
{
	char type1[100];
	char type2[100];
	char type3[100];
	char type4[100];

	if (instr->used) {
		if (preg) {
			ld10k1_debug_decode_preg_idx(type1, instr->arg[0]);
			ld10k1_debug_decode_preg_idx(type2, instr->arg[1]);
			ld10k1_debug_decode_preg_idx(type3, instr->arg[2]);
			ld10k1_debug_decode_preg_idx(type4, instr->arg[3]);

			sprintf(debug_line, "%c 0x%03x : %-10s %s, %s, %s, %s\n",
				instr->modified ? '*' : ' ',
				idx,
				instr_name[instr->op_code],
				type1,
				type2,
				type3,
				type4);
		} else {
			sprintf(debug_line, "%c 0x%03x : %-10s 0x%03x, 0x%03x, 0x%03x, 0x%03x\n",
				instr->modified ? '*' : ' ',
				idx,
				instr_name[instr->op_code],
				instr->arg[0],
				instr->arg[1],
				instr->arg[2],
				instr->arg[3]);
		}

		return send_debug_line(data_conn);
	} else {
		sprintf(debug_line, "%c 0x%03x : NOT USED\n",
			instr->modified ? '*' : ' ',
			idx);
		return send_debug_line(data_conn);
	}
}

int ld10k1_debug_new_code_read_hdr(int data_conn)
{
	sprintf(debug_line, "M Idx     OPCODE\n");
	return send_debug_line(data_conn);
}

static int ld10k1_debug_new_code_read(int data_conn, ld10k1_dsp_mgr_t *dsp_mgr)
{
	int i;
	int err;
	ld10k1_instr_t *instr;

	sprintf(debug_line, "FX8010 Code\n");
	if ((err = send_debug_line(data_conn)) < 0)
		return err;
	if ((err = ld10k1_debug_new_code_read_hdr(data_conn)) < 0)
		return err;
	for (i = 0; i < dsp_mgr->instr_count; i++) {
  		instr = &(dsp_mgr->instr[i]);
		if (instr->used)
			if ((err = ld10k1_debug_new_code_read_one(data_conn, 0, instr, i)) < 0)
				return err;
	}
	return 0;
}

static int ld10k1_debug_new_tram_info_read(int data_conn, ld10k1_dsp_mgr_t *dsp_mgr)
{
	int i, j;
	int err;

	char *req_pos_str;
	char *pos_str;

	ld10k1_tram_acc_t *tram_acc;
	unsigned int data, addr;

	int ifree = dsp_mgr->i_tram.size;
	int efree = dsp_mgr->e_tram.size;

	sprintf(debug_line, "TRAM\n\n");
	if ((err = send_debug_line(data_conn)) < 0)
		return err;

	sprintf(debug_line, "Internal tram size: 0x%08x\n", dsp_mgr->i_tram.size);
	if ((err = send_debug_line(data_conn)) < 0)
		return err;
	sprintf(debug_line, "External tram size: 0x%08x\n", dsp_mgr->e_tram.size);
	if ((err = send_debug_line(data_conn)) < 0)
		return err;

	sprintf(debug_line, "\nTram groups:\n");
	if ((err = send_debug_line(data_conn)) < 0)
		return err;

	for (i = 0; i < dsp_mgr->max_tram_grp; i++) {
		if (dsp_mgr->tram_grp[i].used) {
			sprintf(debug_line, "%03d  %10s  ", i, dsp_mgr->tram_grp[i].type == TRAM_GRP_DELAY ? "DELAY" : "TABLE");
			if ((err = send_debug_line(data_conn)) < 0)
				return err;

			req_pos_str = "NONE";
			if (dsp_mgr->tram_grp[i].req_pos == TRAM_POS_AUTO)
				req_pos_str = "AUTO";
			else if (dsp_mgr->tram_grp[i].req_pos == TRAM_POS_INTERNAL)
				req_pos_str = "INTERNAL";
			else if (dsp_mgr->tram_grp[i].req_pos == TRAM_POS_EXTERNAL)
				req_pos_str = "EXTERNAL";

			pos_str = "NONE";
			if (dsp_mgr->tram_grp[i].pos == TRAM_POS_INTERNAL) {
				pos_str = "INTERNAL";
				ifree -= dsp_mgr->tram_grp[i].size;
			} else if (dsp_mgr->tram_grp[i].pos == TRAM_POS_EXTERNAL) {
				pos_str = "EXTERNAL";
				efree -= dsp_mgr->tram_grp[i].size;
			}

			sprintf(debug_line, "%10s  %10s   %08x  %08x  %03d\n", req_pos_str, pos_str,
				dsp_mgr->tram_grp[i].size, dsp_mgr->tram_grp[i].offset, dsp_mgr->tram_grp[i].acc_count);
			if ((err = send_debug_line(data_conn)) < 0)
				return err;

			for (j = 0; j < dsp_mgr->max_tram_acc; j++) {
				tram_acc = &(dsp_mgr->tram_acc[j]);

				ld10k1_tram_get_hwacc(dsp_mgr, tram_acc->hwacc, &addr, &data);
				if ((tram_acc->used) && (tram_acc->grp == i)) {
					sprintf(debug_line, "  %c%c%c  Off:0x%08x  HWacc:%03d  ADDR:0x%08x   DATA:0x%08x\n",
						(tram_acc->type & TRAM_ACC_READ) ? 'R' : '-',
						(tram_acc->type & TRAM_ACC_WRITE) ? 'W' : '-',
						(tram_acc->type & TRAM_ACC_ZERO) ? 'Z' : '-',
						tram_acc->offset,
						tram_acc->hwacc,
						addr,
						data);
					if ((err = send_debug_line(data_conn)) < 0)
						return err;
				}
			}
		}
	}
	return 0;
}

int ld10k1_debug_new_patch_read1(int data_conn, ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_patch_t *patch)
{
	int i, j;
	int err;

	ld10k1_conn_point_t *point;

	sprintf(debug_line, "Patch name: %s\n\n", patch->patch_name);
	if ((err = send_debug_line(data_conn)) < 0)
		return err;

	/* in list */
	sprintf(debug_line, "IN registers:\n");
	if ((err = send_debug_line(data_conn)) < 0)
		return err;
	for (i = 0; i < patch->in_count; i++) {
		sprintf(debug_line, "%03d   %s  ->  0x%08x\n", i,
			patch->ins[i].name ? patch->ins[i].name : "",
			patch->ins[i].point ? patch->ins[i].point->con_gpr_idx : 0);
		if ((err = send_debug_line(data_conn)) < 0)
			return err;

		point = patch->ins[i].point;
		if (point != NULL)
			for (j = 0; j < MAX_CONN_PER_POINT; j++) {
				if (point->out_gpr_idx[j] != 0) {
					sprintf(debug_line, "   +0x%08x\n", point->out_gpr_idx[j]);
					if ((err = send_debug_line(data_conn)) < 0)
						return err;
				}
			}
	}

	/* out list */
	sprintf(debug_line, "OUT registers:\n");
	if ((err = send_debug_line(data_conn)) < 0)
		return err;
	for (i = 0; i < patch->out_count; i++) {
		sprintf(debug_line, "%03d   %s  ->  0x%08x\n", i,
			patch->outs[i].name ? patch->outs[i].name : "",
			patch->outs[i].point ? patch->outs[i].point->con_gpr_idx : 0);
		if ((err = send_debug_line(data_conn)) < 0)
			return err;
	}

	/* const list */
	sprintf(debug_line, "CONST registers:\n");
	if ((err = send_debug_line(data_conn)) < 0)
		return err;
	for (i = 0; i < patch->const_count; i++) {
		sprintf(debug_line, "%03d   0x%08x  ->  0x%08x\n", i,
			patch->consts[i].const_val,
			patch->consts[i].gpr_idx);
		if ((err = send_debug_line(data_conn)) < 0)
			return err;
	}

	/* sta list */
	sprintf(debug_line, "STA registers:\n");
	if ((err = send_debug_line(data_conn)) < 0)
		return err;
	for (i = 0; i < patch->sta_count; i++) {
		sprintf(debug_line, "%03d   0x%08x  ->  0x%08x\n", i,
			patch->stas[i].const_val,
			patch->stas[i].gpr_idx);
		if ((err = send_debug_line(data_conn)) < 0)
			return err;
	}

	/* hw list */
	sprintf(debug_line, "HW registers:\n");
	if ((err = send_debug_line(data_conn)) < 0)
		return err;
	for (i = 0; i < patch->hw_count; i++) {
		sprintf(debug_line, "%03d   0x%08x  ->  0x%08x\n", i,
			patch->hws[i].reg_idx,
			patch->hws[i].gpr_idx);
		if ((err = send_debug_line(data_conn)) < 0)
			return err;
	}

	/* tram list */
	sprintf(debug_line, "\nUsed tram groups:\n");
	if ((err = send_debug_line(data_conn)) < 0)
		return err;
	for (i = 0; i < patch->tram_count; i++)	{
		sprintf(debug_line, "%03d  0x%01x 0x%08x 0x%01x ->  %03d\n", i,
			patch->tram_grp[i].grp_type,
			patch->tram_grp[i].grp_size,
			patch->tram_grp[i].grp_pos,
			patch->tram_grp[i].grp_idx);
		if ((err = send_debug_line(data_conn)) < 0)
			return err;
	}

	/* tram acc list */
	sprintf(debug_line, "\nUsed tram acc:\n");
	if ((err = send_debug_line(data_conn)) < 0)
		return err;
	for (i = 0; i < patch->tram_acc_count; i++) {
		sprintf(debug_line, "%03d   0x%01x  0x%08x  0x%03x ->  0x%03x\n", i,
			patch->tram_acc[i].acc_type,
			patch->tram_acc[i].acc_offset,
			patch->tram_acc[i].grp,
			patch->tram_acc[i].acc_idx);
		if ((err = send_debug_line(data_conn)) < 0)
			return err;
	}

	/* cotrol list */
	sprintf(debug_line, "\nUsed controls:\n");
	if ((err = send_debug_line(data_conn)) < 0)
		return err;
	for (i = 0; i < patch->ctl_count; i++) {
		sprintf(debug_line, "%03d\n", i);
		if ((err = send_debug_line(data_conn)) < 0)
			return err;
		sprintf(debug_line, "  Name:%s\n", patch->ctl[i].name);
		if ((err = send_debug_line(data_conn)) < 0)
			return err;
		sprintf(debug_line, "  Min: 0x%08x\n", patch->ctl[i].min);
		if ((err = send_debug_line(data_conn)) < 0)
			return err;
		sprintf(debug_line, "  Max: 0x%08x\n", patch->ctl[i].max);
		if ((err = send_debug_line(data_conn)) < 0)
			return err;
		sprintf(debug_line, "  GPRS:\n");
		if ((err = send_debug_line(data_conn)) < 0)
			return err;
		for (j = 0; j < patch->ctl[i].count; j++) {
			sprintf(debug_line, "    %03d  0x%08x ->  0x%08x  %c\n", j,
				patch->ctl[i].value[j],
				patch->ctl[i].gpr_idx[j],
				j < patch->ctl[i].vcount ? 'v' : ' ');
			if ((err = send_debug_line(data_conn)) < 0)
				return err;
		}
	}

	/* instruction list */
	sprintf(debug_line, "\nCode:\n");
	if ((err = send_debug_line(data_conn)) < 0)
		return err;

	if ((err = ld10k1_debug_new_code_read_hdr(data_conn)) < 0)
		return err;
	for (i = 0; i < patch->instr_count; i++) {
		ld10k1_instr_t *instr;

		instr = &(patch->instr[i]);
		if ((err = ld10k1_debug_new_code_read_one(data_conn, 1, instr, i)) < 0)
			return err;
	}
	return 0;
}

static int ld10k1_debug_new_patch_read(int data_conn, ld10k1_dsp_mgr_t *dsp_mgr, int idx)
{
	ld10k1_patch_t *patch;
	patch = dsp_mgr->patch_ptr[idx];
	if (!patch)
		return LD10K1_ERR_UNKNOWN_PATCH_NUM;

	return ld10k1_debug_new_patch_read1(data_conn, dsp_mgr, patch);
}

static int ld10k1_debug_new_patch_list_read(int data_conn, ld10k1_dsp_mgr_t *dsp_mgr)
{
	int i;
	ld10k1_patch_t *patch;
	int err;

	sprintf(debug_line, "\nPatch List:\n");
	if ((err = send_debug_line(data_conn)) < 0)
		return err;
	for (i = 0; i < EMU10K1_PATCH_MAX; i++) {
		patch = dsp_mgr->patch_ptr[i];
		if (patch) {
			sprintf(debug_line, "%03d  %s\n\n", i, patch->patch_name);
			if ((err = send_debug_line(data_conn)) < 0)
				return err;
		}
	}
	return 0;
}

static int ld10k1_debug_new_patch_order_read(int data_conn, ld10k1_dsp_mgr_t *dsp_mgr)
{
	int i, idx;
	ld10k1_patch_t *patch;
	int err;

	sprintf(debug_line, "\nPatch order:\n");
	if ((err = send_debug_line(data_conn)) < 0)
		return err;
	for (i = 0; i < dsp_mgr->patch_count; i++) {
		idx = dsp_mgr->patch_order[i];
		patch = dsp_mgr->patch_ptr[idx];
		if (patch) {
			sprintf(debug_line, "%03d   %03d %s\n\n", i, idx, patch->patch_name);
			if ((err = send_debug_line(data_conn)) < 0)
				return err;
		}
	}

	return 0;
}

int ld10k1_fnc_debug(int data_conn, int op, int size)
{
	ld10k1_fnc_debug_t debug_info;
	int err;

	if (size != sizeof(ld10k1_fnc_debug_t))
		return LD10K1_ERR_PROTOCOL;

	if ((err = receive_msg_data(data_conn, &debug_info, sizeof(ld10k1_fnc_debug_t))))
		return err;

	if (debug_info.what >= 100 && debug_info.what <= 100 + EMU10K1_PATCH_MAX) {
		if ((err = ld10k1_debug_new_patch_read(data_conn, &dsp_mgr, debug_info.what - 100)) < 0)
			return err;
		if ((err = send_response_ok(data_conn)) < 0)
			return err;
	} else if (debug_info.what == 1) {
		/* registers */
		if ((err = ld10k1_debug_new_gpr_read(data_conn, &dsp_mgr)) < 0)
			return err;
		if ((err = send_response_ok(data_conn)) < 0)
			return err;
	} else if (debug_info.what == 2) {
		/* registers */
		if ((err = ld10k1_debug_new_const_read(data_conn, &dsp_mgr)) < 0)
			return err;
		if ((err = send_response_ok(data_conn)) < 0)
			return err;
	} else if (debug_info.what == 3) {
		/* instruction */
		if ((err = ld10k1_debug_new_code_read(data_conn, &dsp_mgr)) < 0)
			return err;
		if ((err = send_response_ok(data_conn)) < 0)
			return err;
	} else if (debug_info.what == 4) {
		/* tram */
		if ((err = ld10k1_debug_new_tram_info_read(data_conn, &dsp_mgr)) < 0)
			return err;
		if ((err = send_response_ok(data_conn)) < 0)
			return err;
	} else if (debug_info.what == 5) {
		if ((err = ld10k1_debug_new_patch_list_read(data_conn, &dsp_mgr)) < 0)
			return err;
		if ((err = send_response_ok(data_conn)) < 0)
			return err;
	} else if (debug_info.what == 6) {
		if ((err = ld10k1_debug_new_patch_order_read(data_conn, &dsp_mgr)) < 0)
			return err;
		if ((err = send_response_ok(data_conn)) < 0)
			return err;
	} else if (debug_info.what == 7) {
		/* fx */
		if ((err = ld10k1_debug_new_fx_read(data_conn, &dsp_mgr)) < 0)
			return err;
		if ((err = send_response_ok(data_conn)) < 0)
			return err;
	} else if (debug_info.what == 8) {
		/* in */
		if ((err = ld10k1_debug_new_in_read(data_conn, &dsp_mgr)) < 0)
			return err;
		if ((err = send_response_ok(data_conn)) < 0)
			return err;
	} else if (debug_info.what == 9) {
		/* out */
		if ((err = ld10k1_debug_new_out_read(data_conn, &dsp_mgr)) < 0)
			return err;
		if ((err = send_response_ok(data_conn)) < 0)
			return err;
	} else
		if ((err = send_response_ok(data_conn)) < 0)
			return err;

	return 0;
}
