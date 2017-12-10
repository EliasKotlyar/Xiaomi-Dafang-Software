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

#include <malloc.h>
#include <stdlib.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <alsa/asoundlib.h>
#include <alsa/sound/emu10k1.h>

#include "ld10k1.h"
#include "ld10k1_fnc.h"
#include "ld10k1_fnc_int.h"
#include "ld10k1_driver.h"
#include "ld10k1_tram.h"
#include "ld10k1_error.h"

char *ld10k1_dsp_mgr_name_new(char **where, const char *from);
int ld10k1_add_control(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_ctl_t *gctl);
void ld10k1_del_control(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_ctl_t *gctl);
int ld10k1_dsp_mgr_patch_unload(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_patch_t *patch, unsigned int idx);
int ld10k1_get_used_index_for_control(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_ctl_t *gctl, int **idxs, int *cnt);

unsigned int ld10k1_resolve_named_reg(ld10k1_dsp_mgr_t *dsp_mgr, unsigned int reg);
unsigned int ld10k1_gpr_reserve(ld10k1_dsp_mgr_t *dsp_mgr, int max_res_count, int *res_count, int *res,
	unsigned int usage, unsigned int val);
unsigned int ld10k1_gpr_dyn_reserve(ld10k1_dsp_mgr_t *dsp_mgr, int max_res_count, int *res_count, int *res);
unsigned int ld10k1_const_reserve(ld10k1_dsp_mgr_t *dsp_mgr, int max_res_const_count, int *res_const_count, int *res_const,
	int max_res_count, int *res_count, int *res, int const_val);

void ld10k1_const_alloc(ld10k1_dsp_mgr_t *dsp_mgr, int reg);
void ld10k1_const_free(ld10k1_dsp_mgr_t *dsp_mgr, int reg);
void ld10k1_gpr_alloc(ld10k1_dsp_mgr_t *dsp_mgr, int reg);
void ld10k1_gpr_free(ld10k1_dsp_mgr_t *dsp_mgr, int reg);

ld10k1_conn_point_t *ld10k1_conn_point_alloc(int simple);
void ld10k1_conn_point_free(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_conn_point_t *point);
int ld10k1_conn_point_set_to(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_conn_point_t *point, int type, int io);
void ld10k1_conn_point_unset(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_conn_point_t *point);
int ld10k1_conn_point_add(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_conn_point_t *point, int type, ld10k1_patch_t *patch, int io);
int ld10k1_conn_point_del(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_conn_point_t *point, int type, ld10k1_patch_t *patch, int io);

int ld10k1_conn_point_get_reg(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_conn_point_t *point, int type, ld10k1_patch_t *patch, int io);

void ld10k1_conn_point_add_to_list(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_conn_point_t *point);
void ld10k1_conn_point_del_from_list(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_conn_point_t *point);
int ld10k1_gen_patch_id(ld10k1_dsp_mgr_t *dsp_mgr, int pnum);

/*
 *  Tables
 */

unsigned int hw_const[22 * 2] =
{
	0x00000000, EMU10K1_REG_HW(0),
	0x00000001, EMU10K1_REG_HW(1),
	0x00000002, EMU10K1_REG_HW(2),
	0x00000003, EMU10K1_REG_HW(3),
	0x00000004, EMU10K1_REG_HW(4),
	0x00000008, EMU10K1_REG_HW(5),
	0x00000010, EMU10K1_REG_HW(6),
	0x00000020, EMU10K1_REG_HW(7),
	0x00000100, EMU10K1_REG_HW(8),
	0x00010000, EMU10K1_REG_HW(9),
	0x10000000, EMU10K1_REG_HW(11),
	0x20000000, EMU10K1_REG_HW(12),
	0x40000000, EMU10K1_REG_HW(13),
	0x80000000, EMU10K1_REG_HW(14),
	0x7fffffff, EMU10K1_REG_HW(15),
	0xffffffff, EMU10K1_REG_HW(16),
	0xfffffffe, EMU10K1_REG_HW(17),
	0xc0000000, EMU10K1_REG_HW(18),
	0x4f1bbcde, EMU10K1_REG_HW(19),
	0x5a7ef9db, EMU10K1_REG_HW(20),
	0x00100000, EMU10K1_REG_HW(21)
};

int ld10k1_dsp_mgr_init(ld10k1_dsp_mgr_t *dsp_mgr)
{
	int tmp_gpr_count = 0;
	int tmp_a_gpr_count = 0;
	int tmp_itram_count = 0;
	int tmp_etram_count = 0;
	int tmp_op_count = 0;
	int i, j;

	dsp_mgr->add_ctl_list = NULL;
	dsp_mgr->add_list_count = 0;

	dsp_mgr->del_ctl_list = NULL;
	dsp_mgr->del_list_count = 0;

	dsp_mgr->ctl_list = NULL;
	dsp_mgr->ctl_list_count = 0;
	
	dsp_mgr->point_list = 0;

	if (dsp_mgr->audigy) {
		tmp_itram_count = 0xC0;
		tmp_etram_count = 0x100 - 0xC0;
		tmp_op_count = 0x400;
		tmp_a_gpr_count = 0x200;
	} else {
		tmp_itram_count = 0x80;
		tmp_etram_count = 0xA0 - 0x80;
		tmp_op_count = 0x200;
		tmp_gpr_count = 0x100;
	}

	dsp_mgr->instr_count = tmp_op_count;
	dsp_mgr->instr_free = tmp_op_count;

	for (i = 0; i < tmp_op_count; i++) {
		dsp_mgr->instr[i].used = 0;
		dsp_mgr->instr[i].modified = 1;
		dsp_mgr->instr[i].op_code = 0;
		for (j = 0; j < 4; j++)
		    dsp_mgr->instr[i].arg[j] = 0;
	}

	/* fx */
	for (i = 0; i < (dsp_mgr->audigy ? 0x40 : 0x10); i++) {
		dsp_mgr->fxs[i].name = NULL;
		dsp_mgr->fxs[i].point = NULL;
	}
	dsp_mgr->fx_count = dsp_mgr->audigy ? 0x40 : 0x10;

	/* input */
	for (i = 0; i < (dsp_mgr->audigy ? 0x20 : 0x10); i++) {
		dsp_mgr->ins[i].name = NULL;
		dsp_mgr->ins[i].point = NULL;
	}

	dsp_mgr->in_count = dsp_mgr->audigy ? 0x20 : 0x10;

	/* output */
	for (i = 0; i < (dsp_mgr->audigy ? 0x40 : 0x20); i++) {
		dsp_mgr->outs[i].name = NULL;
		dsp_mgr->outs[i].point = NULL;
	}

	dsp_mgr->out_count = dsp_mgr->audigy ? 0x40 : 0x20;

	ld10k1_dsp_mgr_name_new(&(dsp_mgr->fxs[0x00].name), "FX_PCM_Left");
	ld10k1_dsp_mgr_name_new(&(dsp_mgr->fxs[0x01].name), "FX_PCM_Right");
	ld10k1_dsp_mgr_name_new(&(dsp_mgr->fxs[0x02].name), "FX_PCM_Surr_Left");
	ld10k1_dsp_mgr_name_new(&(dsp_mgr->fxs[0x03].name), "FX_PCM_Surr_Right");
	ld10k1_dsp_mgr_name_new(&(dsp_mgr->fxs[0x04].name), "FX_MIDI_Left");
	ld10k1_dsp_mgr_name_new(&(dsp_mgr->fxs[0x05].name), "FX_MIDI_Right");
	ld10k1_dsp_mgr_name_new(&(dsp_mgr->fxs[0x06].name), "FX_Center");
	ld10k1_dsp_mgr_name_new(&(dsp_mgr->fxs[0x07].name), "FX_LFE");
	ld10k1_dsp_mgr_name_new(&(dsp_mgr->fxs[0x0c].name), "FX_MIDI_Reverb");
	ld10k1_dsp_mgr_name_new(&(dsp_mgr->fxs[0x0d].name), "FX_MIDI_Chorus");

	if (dsp_mgr->audigy) {
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->fxs[0x08].name), "FX_PCM_Front_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->fxs[0x09].name), "FX_PCM_Front_Right");

		ld10k1_dsp_mgr_name_new(&(dsp_mgr->fxs[0x14].name), "FX_Passthrough_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->fxs[0x15].name), "FX_Passthrough_Right");

		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x00].name), "IN_AC97_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x01].name), "IN_AC97_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x02].name), "IN_Audigy_CD_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x03].name), "IN_Audigy_CD_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x04].name), "IN_Opt_IEC958_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x05].name), "IN_Opt_IEC958_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x08].name), "IN_Line_Mic_2_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x09].name), "IN_Line_Mic_2_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x0a].name), "IN_ADC_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x0b].name), "IN_ADC_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x0c].name), "IN_Aux2_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x0d].name), "IN_Aux2_Right");

		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x00].name), "OUT_Dig_Front_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x01].name), "OUT_Dig_Front_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x02].name), "OUT_Dig_Center");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x03].name), "OUT_Dig_LEF");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x04].name), "OUT_Headphone_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x05].name), "OUT_Headphone_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x06].name), "OUT_Dig_Rear_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x07].name), "OUT_Dig_Rear_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x08].name), "OUT_Front_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x09].name), "OUT_Front_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x0a].name), "OUT_Center");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x0b].name), "OUT_LFE");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x0e].name), "OUT_Rear_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x0f].name), "OUT_Rear_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x10].name), "OUT_AC97_Front_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x11].name), "OUT_AC97_Front_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x16].name), "OUT_ADC_Capture_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x17].name), "OUT_ADC_Capture_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x18].name), "OUT_Mic_Capture");
	} else {
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x00].name), "IN_AC97_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x01].name), "IN_AC97_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x02].name), "IN_TTL_IEC958_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x03].name), "IN_TTL_IEC958_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x04].name), "IN_Zoom_Video_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x05].name), "IN_Zoom_Video_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x06].name), "IN_Optical_IEC958_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x07].name), "IN_Optical_IEC958_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x08].name), "IN_Line_Mic_1_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x09].name), "IN_Line_Mic_1_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x0a].name), "IN_Coax_IEC958_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x0b].name), "IN_Coax_IEC958_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x0c].name), "IN_Line_Mic_2_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->ins[0x0d].name), "IN_Line_Mic_2_Right");

		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x00].name), "OUT_AC97_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x01].name), "OUT_AC97_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x02].name), "OUT_Opt_IEC958_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x03].name), "OUT_Opt_IEC958_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x04].name), "OUT_Center");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x05].name), "OUT_LFE");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x06].name), "OUT_Headphone_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x07].name), "OUT_Headphone_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x08].name), "OUT_Analog_Surr_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x09].name), "OUT_Analog_Surr_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x0a].name), "OUT_PCM_Capture_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x0b].name), "OUT_PCM_Capture_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x0c].name), "OUT_MIC_Capture");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x0d].name), "OUT_AC97_Surr_Left");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x0e].name), "OUT_AC97_Surr_Right");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x11].name), "OUT_AC97_Center");
		ld10k1_dsp_mgr_name_new(&(dsp_mgr->outs[0x12].name), "OUT_AC97_LFE");
	}

	dsp_mgr->regs_max_count = dsp_mgr->audigy ? tmp_a_gpr_count : tmp_gpr_count;

	/* constants */
	for (i = 0; i < 21; i++) {
		dsp_mgr->consts[i].const_val = hw_const[i * 2];
		dsp_mgr->consts[i].gpr_idx = hw_const[i * 2 + 1];
		dsp_mgr->consts[i].hw = 1;
		dsp_mgr->consts[i].ref = 1;
		dsp_mgr->consts[i].used = 1;
	}

	dsp_mgr->consts[21].const_val = dsp_mgr->audigy ? 0x00000800 : 0x00080000;
	dsp_mgr->consts[21].gpr_idx = EMU10K1_REG_HW(10);
	dsp_mgr->consts[21].hw = 1;
	dsp_mgr->consts[21].used = 1;
	dsp_mgr->consts[21].ref = 1;
	dsp_mgr->consts_max_count = 22 + dsp_mgr->regs_max_count;

	/* gprs */
	for (i = 0; i < dsp_mgr->regs_max_count; i++) {
		dsp_mgr->regs[i].used = 0;
		dsp_mgr->regs[i].gpr_usage = GPR_USAGE_NONE;
		dsp_mgr->regs[i].val = 0;
		dsp_mgr->regs[i].ref = 0;
		dsp_mgr->regs[i].modified = 1;
	}

	dsp_mgr->patch_count = 0;

	for (i = 0; i < 0x100; i++) {
		dsp_mgr->tram_acc[i].used = 0;
		dsp_mgr->tram_acc[i].type = 0;
		dsp_mgr->tram_acc[i].offset = 0;
		dsp_mgr->tram_acc[i].hwacc = 0xFFFFFFFF;
		dsp_mgr->tram_acc[i].grp = 0xFFFFFFFF;

		dsp_mgr->tram_grp[i].used = 0;
		dsp_mgr->tram_grp[i].type = 0;
		dsp_mgr->tram_grp[i].size = 0;
		dsp_mgr->tram_grp[i].offset = 0;
		dsp_mgr->tram_grp[i].req_pos = TRAM_POS_NONE;
		dsp_mgr->tram_grp[i].pos = TRAM_POS_NONE;
	}

	for (i = 0; i < 0x40; i++) {
		dsp_mgr->etram_hwacc[i].used = 0;
		dsp_mgr->etram_hwacc[i].op = 0;
		dsp_mgr->etram_hwacc[i].modified = 1;
		dsp_mgr->etram_hwacc[i].data_val = 0;
		dsp_mgr->etram_hwacc[i].addr_val = 0;
	}

	for (i = 0; i < 0xC0; i++) {
		dsp_mgr->itram_hwacc[i].used = 0;
		dsp_mgr->itram_hwacc[i].modified = 1;
		dsp_mgr->itram_hwacc[i].data_val = 0;
		dsp_mgr->itram_hwacc[i].addr_val = 0;
	}

	dsp_mgr->max_tram_grp = dsp_mgr->max_tram_acc = tmp_itram_count + tmp_etram_count;
	dsp_mgr->max_itram_hwacc = tmp_itram_count;
	dsp_mgr->max_etram_hwacc = tmp_etram_count;

	dsp_mgr->i_tram.size = 0;
	dsp_mgr->i_tram.max_hwacc = tmp_itram_count;
	dsp_mgr->i_tram.hwacc = dsp_mgr->itram_hwacc;
	dsp_mgr->i_tram.used_hwacc = 0;

	dsp_mgr->e_tram.size = 0;
	dsp_mgr->e_tram.max_hwacc = tmp_etram_count;
	dsp_mgr->e_tram.hwacc = dsp_mgr->etram_hwacc;
	dsp_mgr->e_tram.used_hwacc = 0;

	dsp_mgr->patch_count = 0;
	for (i = 0; i < EMU10K1_PATCH_MAX; i++) {
		dsp_mgr->patch_ptr[i] = NULL;
		dsp_mgr->patch_order[i] = 0xFFFFFFFF;
	}

	return 0;
}

void ld10k1_dsp_mgr_init_id_gen(ld10k1_dsp_mgr_t *dsp_mgr)
{
	int i;

	for (i = 0; i < EMU10K1_PATCH_MAX; i++)
		dsp_mgr->patch_id_gens[i] = 0;
}

void ld10k1_dsp_mgr_free(ld10k1_dsp_mgr_t *dsp_mgr)
{
	unsigned int i;

	for (i = 0; i < EMU10K1_PATCH_MAX; i++) {
		if (dsp_mgr->patch_ptr[i])
		    ld10k1_dsp_mgr_patch_unload(dsp_mgr, dsp_mgr->patch_ptr[i], i);
	}

	ld10k1_del_all_controls_from_list(&(dsp_mgr->del_ctl_list), &dsp_mgr->del_list_count);
	ld10k1_del_all_controls_from_list(&(dsp_mgr->add_ctl_list), &dsp_mgr->add_list_count);
	ld10k1_del_all_controls_from_list(&(dsp_mgr->ctl_list), &dsp_mgr->ctl_list_count);

	/* FIXME - uvolnovanie point - asi netreba - su uvolnovane pri patch */
	for (i = 0; i < dsp_mgr->fx_count; i++)
		if (dsp_mgr->fxs[i].name)
			free(dsp_mgr->fxs[i].name);

	for (i = 0; i < dsp_mgr->in_count; i++)
		if (dsp_mgr->ins[i].name)
			free(dsp_mgr->ins[i].name);

	for (i = 0; i < dsp_mgr->out_count; i++)
		if (dsp_mgr->outs[i].name)
			free(dsp_mgr->outs[i].name);
}

ld10k1_patch_t *ld10k1_dsp_mgr_patch_new(void)
{
	ld10k1_patch_t *np;

	np = (ld10k1_patch_t *)malloc(sizeof(ld10k1_patch_t));
	if (!np)
		return NULL;

	np->patch_name = NULL;
	np->id = 0;

	np->in_count = 0;
	np->ins = NULL;

	np->out_count = 0;
	np->outs = NULL;

	np->const_count = 0;
	np->consts = NULL;

	np->sta_count = 0;
	np->stas = NULL;

	np->dyn_count = 0;
	np->dyns = NULL;

	np->hw_count = 0;
	np->hws = NULL;

	np->tram_count = 0;
	np->tram_grp = NULL;

	np->tram_acc_count = 0;
	np->tram_acc = NULL;

	np->ctl_count = 0;
	np->ctl = NULL;

	np->instr_count = 0;
	np->instr_offset = 0;
	np->instr = NULL;

	return np;
}

void ld10k1_dsp_mgr_patch_free(ld10k1_patch_t *patch)
{
	int i;

	if (patch->patch_name)
		free(patch->patch_name);

 	for (i = 0; i < patch->in_count; i++)
		if (patch->ins[i].name)
			free(patch->ins[i].name);

	for (i = 0; i < patch->out_count; i++)
		if (patch->outs[i].name)
			free(patch->outs[i].name);

	if (patch->ins)
		free(patch->ins);

	if (patch->outs)
		free(patch->outs);

	if (patch->consts)
		free(patch->consts);

	if (patch->stas)
		free(patch->stas);

	if (patch->dyns)
		free(patch->dyns);

	if (patch->hws)
		free(patch->hws);

	if (patch->tram_grp)
		free(patch->tram_grp);

	if (patch->tram_acc)
		free(patch->tram_acc);

	if (patch->ctl)
		free(patch->ctl);

	if (patch->instr)
		free(patch->instr);

	free(patch);
}

ld10k1_p_in_out_t *ld10k1_dsp_mgr_patch_in_new(ld10k1_patch_t *patch, unsigned int count)
{
	ld10k1_p_in_out_t *ins;

	ins = (ld10k1_p_in_out_t *)malloc(sizeof(ld10k1_p_in_out_t) * count);
	if (!ins)
		return NULL;

	if (patch->ins)
   		free(patch->ins);

	memset(ins, 0, sizeof(ld10k1_p_in_out_t) * count);

	patch->ins = ins;
	patch->in_count = count;
	return ins;
}

ld10k1_p_in_out_t *ld10k1_dsp_mgr_patch_out_new(ld10k1_patch_t *patch, unsigned int count)
{
	ld10k1_p_in_out_t *outs;

	outs = (ld10k1_p_in_out_t *)malloc(sizeof(ld10k1_p_in_out_t) * count);
	if (!outs)
		return NULL;

	if (patch->outs)
   		free(patch->outs);

	memset(outs, 0, sizeof(ld10k1_p_in_out_t) * count);

	patch->outs = outs;
	patch->out_count = count;
	return outs;
}

ld10k1_p_const_sta_t *ld10k1_dsp_mgr_patch_const_new(ld10k1_patch_t *patch, unsigned int count)
{
	ld10k1_p_const_sta_t *consts;

	consts = (ld10k1_p_const_sta_t *)malloc(sizeof(ld10k1_p_const_sta_t) * count);
	if (!consts)
		return NULL;

	if (patch->consts)
   		free(patch->consts);

	memset(consts, 0, sizeof(ld10k1_p_const_sta_t) * count);

	patch->consts = consts;
	patch->const_count = count;
	return consts;
}

ld10k1_p_const_sta_t *ld10k1_dsp_mgr_patch_sta_new(ld10k1_patch_t *patch, unsigned int count)
{
	ld10k1_p_const_sta_t *stas;

	stas = (ld10k1_p_const_sta_t *)malloc(sizeof(ld10k1_p_const_sta_t) * count);
	if (!stas)
		return NULL;

	if (patch->stas)
   		free(patch->stas);

	memset(stas, 0, sizeof(ld10k1_p_const_sta_t) * count);

	patch->stas = stas;
	patch->sta_count = count;
	return stas;
}

ld10k1_p_dyn_t *ld10k1_dsp_mgr_patch_dyn_new(ld10k1_patch_t *patch, unsigned int count)
{
	ld10k1_p_dyn_t *dyns;

	dyns = (ld10k1_p_dyn_t *)malloc(sizeof(ld10k1_p_dyn_t) * count);
	if (!dyns)
		return NULL;

	if (patch->dyns)
   		free(patch->dyns);

	memset(dyns, 0, sizeof(ld10k1_p_dyn_t) * count);

	patch->dyns = dyns;
	patch->dyn_count = count;
	return dyns;
}

ld10k1_p_hw_t *ld10k1_dsp_mgr_patch_hw_new(ld10k1_patch_t *patch, unsigned int count)
{
	ld10k1_p_hw_t *hws;

	hws = (ld10k1_p_hw_t *)malloc(sizeof(ld10k1_p_hw_t) * count);
	if (!hws)
		return NULL;

	if (patch->hws)
   		free(patch->hws);

	memset(hws, 0, sizeof(ld10k1_p_hw_t) * count);

	patch->hws = hws;
	patch->hw_count = count;
	return hws;
}

ld10k1_p_tram_grp_t *ld10k1_dsp_mgr_patch_tram_new(ld10k1_patch_t *patch, unsigned int count)
{
	ld10k1_p_tram_grp_t *tram;

	tram = (ld10k1_p_tram_grp_t *)malloc(sizeof(ld10k1_p_tram_grp_t) * count);
	if (!tram)
		return NULL;

	if (patch->tram_grp)
   		free(patch->tram_grp);

	memset(tram, 0, sizeof(ld10k1_p_tram_grp_t) * count);

	patch->tram_grp = tram;
	patch->tram_count = count;
	return tram;
}

ld10k1_p_tram_acc_t *ld10k1_dsp_mgr_patch_tram_acc_new(ld10k1_patch_t *patch, unsigned int count)
{
	ld10k1_p_tram_acc_t *tram_acc;
	
	tram_acc = (ld10k1_p_tram_acc_t *)malloc(sizeof(ld10k1_p_tram_acc_t) * count);
	if (!tram_acc)
		return NULL;

	if (patch->tram_acc)
   		free(patch->tram_acc);
		
	memset(tram_acc, 0, sizeof(ld10k1_p_tram_acc_t) * count);
	
	patch->tram_acc = tram_acc;
	patch->tram_acc_count = count;
	return tram_acc;
}

ld10k1_instr_t *ld10k1_dsp_mgr_patch_instr_new(ld10k1_patch_t *patch, unsigned int count)
{
	ld10k1_instr_t *instr;
	
	instr = (ld10k1_instr_t *)malloc(sizeof(ld10k1_instr_t) * count);
	if (!instr)
		return NULL;
		
	if (patch->instr)
   		free(patch->instr);

	memset(instr, 0, sizeof(ld10k1_instr_t) * count);

	patch->instr = instr;
	patch->instr_count = count;
	return instr;
}

ld10k1_ctl_t *ld10k1_dsp_mgr_patch_ctl_new(ld10k1_patch_t *patch, unsigned int count)
{
	ld10k1_ctl_t *ctl;
	
	ctl = (ld10k1_ctl_t *)malloc(sizeof(ld10k1_ctl_t) * count);
	if (!ctl)
		return NULL;
		
	if (patch->ctl)
   		free(patch->ctl);

	memset(ctl, 0, sizeof(ld10k1_ctl_t) * count);

	patch->ctl = ctl;
	patch->ctl_count = count;
	return ctl;
}

char *ld10k1_dsp_mgr_name_new(char **where, const char *from)
{
	char *ns;

	ns = (char *)malloc(strlen(from) + 1);
	if (!ns)
		return NULL;

	if (*where)
   		free(*where);

	*where = ns;
	strcpy(ns, from);
	return ns;
}

void ld10k1_dsp_mgr_op(ld10k1_instr_t *instr, unsigned int op, unsigned int arg1, unsigned int arg2, unsigned int arg3, unsigned int arg4)
{
	instr->used = 1;
 	instr->modified = 1;
	instr->op_code = op;
	instr->arg[0] = arg1;
	instr->arg[1] = arg2;
	instr->arg[2] = arg3;
	instr->arg[3] = arg4;
}

int ld10k1_dsp_mgr_get_phys_reg(ld10k1_dsp_mgr_t *dsp_mgr, unsigned int reg)
{
	int idx = reg & 0xFFFFFFF;

	switch(EMU10K1_REG_TYPE_B(reg)) {
		case EMU10K1_REG_TYPE_FX:
			if (dsp_mgr->audigy) {
				if (idx > 0x3F)
					return -1;
				else
					return idx;
			} else {
				if (idx > 0x0F)
					return -1;
				else
					return idx;
			}
			break;
		case EMU10K1_REG_TYPE_INPUT:
			if (dsp_mgr->audigy) {
				if (idx > 0x1F)
					return -1;
				else
					return idx + 0x40;
			} else {
				if (idx > 0x0F)
					return -1;
				else
					return idx + 0x10;
			}
			break;
		case EMU10K1_REG_TYPE_OUTPUT:
			if (dsp_mgr->audigy) {
				if (idx > 0x3F)
					return -1;
				else
					return idx + 0x60;
			} else {
				if (idx > 0x1F)
					return -1;
				else
					return idx + 0x20;
			}
		case EMU10K1_REG_TYPE_HW:
			if (dsp_mgr->audigy) {
				if (idx > 0x1F)
					return -1;
				else
					return idx + 0xC0;
			} else {
				if (idx > 0x1F)
					return -1;
				else
					return idx + 0x40;
			}
		case EMU10K1_REG_TYPE_TRAM_CTL:
			if (dsp_mgr->audigy) {
				if (idx > 0xFF)
					return -1;
				else
					return idx + 0x10;
			} else {
					return -1;
			}
		case EMU10K1_REG_TYPE_TRAM_DATA:
			if (dsp_mgr->audigy) {
				if (idx > 0xFF)
					return -1;
				else
					return idx + 0x200;
			} else {
				if (idx > 0xBF)
					return -1;
				else
					return idx + 0x200;
			}
		case EMU10K1_REG_TYPE_TRAM_ADDR:
			if (dsp_mgr->audigy) {
				if (idx > 0xFF)
					return -1;
				else
					return idx + 0x300;
			} else {
				if (idx > 0xBF)
					return -1;
				else
					return idx + 0x300;
			}
		case EMU10K1_REG_TYPE_NORMAL:
			if (dsp_mgr->audigy) {
				if (idx > 0x1FF)
					return -1;
				else
					return idx + 0x400;
			} else {
				if (idx > 0xFF)
					return -1;
				else
					return idx + 0x100;
			}
		case EMU10K1_REG_TYPE_CONST:
			if (idx > MAX_CONST_COUNT)
				return -1;
			else
				return ld10k1_dsp_mgr_get_phys_reg(dsp_mgr, dsp_mgr->consts[idx].gpr_idx);
		default:
			return -1;
	}
}

int ld10k1_dsp_mgr_get_phys_reg_for_patch(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_patch_t *patch, unsigned int reg)
{
	unsigned int ind_reg = 0;
	unsigned int acc_idx = 0;

	ld10k1_conn_point_t *point;

	switch (EMU10K1_PREG_TYPE_B(reg)) {
		case EMU10K1_PREG_TYPE_IN:
			/*if (patch->ins[reg & 0xFFFFFFF].point)
				ind_reg = patch->ins[reg & 0xFFFFFFF].point->con_gpr_idx;
			else
				ind_reg = EMU10K1_REG_HW(0);
			break;*/
			if (patch->ins[reg & 0xFFFFFFF].point) {
				point = patch->ins[reg & 0xFFFFFFF].point;
				ind_reg = ld10k1_conn_point_get_reg(dsp_mgr, point, CON_IO_PIN, patch, reg & 0xFFFFFFF);
			} else
				ind_reg = EMU10K1_REG_HW(0);
			break;
		case EMU10K1_PREG_TYPE_OUT:
			if (patch->outs[reg & 0xFFFFFFF].point) {
				point = patch->outs[reg & 0xFFFFFFF].point;

				ind_reg = ld10k1_conn_point_get_reg(dsp_mgr, point, CON_IO_POUT, patch, reg & 0xFFFFFFF);
			} else
				ind_reg = EMU10K1_REG_HW(0); /* const 0 */
			break;
		case EMU10K1_PREG_TYPE_CONST:
			ind_reg = patch->consts[reg & 0xFFFFFFF].gpr_idx;
			break;
		case EMU10K1_PREG_TYPE_STA:
			ind_reg = patch->stas[reg & 0xFFFFFFF].gpr_idx;
			break;
		case EMU10K1_PREG_TYPE_DYN:
			ind_reg = patch->dyns[reg & 0xFFFFFFF].gpr_idx;
			break;
		case EMU10K1_PREG_TYPE_HW:
			ind_reg = patch->hws[reg & 0xFFFFFFF].gpr_idx;
			break;
		case EMU10K1_PREG_TYPE_CTL:
			ind_reg = patch->ctl[(reg & 0xFF00) >> 8].gpr_idx[reg & 0xFF];
			break;
		case EMU10K1_PREG_TYPE_TRAM_DATA:
			acc_idx = patch->tram_acc[reg & 0xFFFFFFF].acc_idx;
			ind_reg = EMU10K1_REG_TRAM_DATA(dsp_mgr->tram_acc[acc_idx].hwacc);
			break;
		case EMU10K1_PREG_TYPE_TRAM_ADDR:
			acc_idx = patch->tram_acc[reg & 0xFFFFFFF].acc_idx;
			ind_reg = EMU10K1_REG_TRAM_ADDR(dsp_mgr->tram_acc[acc_idx].hwacc);
			break;
		default:
			return -1;
	}

	if (ind_reg)
		return ld10k1_dsp_mgr_get_phys_reg(dsp_mgr, ind_reg);
	return -1;
}

int ld10k1_dsp_mgr_actualize_instr(ld10k1_dsp_mgr_t *dsp_mgr)
{
	unsigned int i, j, k, l, m, z;
	unsigned int instr_offset;
	ld10k1_patch_t *tmpp;
	ld10k1_instr_t *instr;
	ld10k1_conn_point_t *tmp_point;
	int allmodified = 0;
	int found;
	
	instr_offset = 0;

	/* intruction actualization */
	for (i = 0; i < dsp_mgr->patch_count; i++) {
  		tmpp = dsp_mgr->patch_ptr[dsp_mgr->patch_order[i]];


		for (m = 0; m < 3; m++) {
			if (m == 0 || m == 2) {
				/* get all owned points */
				for (j = 0; j < (m == 0 ? tmpp->in_count : tmpp->out_count); j++) {
					allmodified = 0;
					if (m == 0)
						tmp_point = tmpp->ins[j].point;
					else
						tmp_point = tmpp->outs[j].point;
					if (tmp_point && tmp_point->owner == tmpp &&
						((m == 0 && tmp_point->position == INSERT_BEFORE_OWNER) ||
						(m == 2 && tmp_point->position == INSERT_AFTER_OWNER)))
					{
						/* check if not generated before */
						found = 0;
						for (z = 0; z < j; z++)
							if ((m == 0 && tmpp->ins[z].point == tmp_point) ||
								(m == 2 && tmpp->outs[z].point == tmp_point)) {
								found = 1;
								break;
							}
						/* if generated - continue */
						if (found)
							continue;
								
						if (tmp_point->reserved_instr > 0 && tmp_point->out_instr_offset != instr_offset)
							allmodified = 1;

						tmp_point->out_instr_offset = instr_offset;
						/* copy instructions */
						for (k = 0; k < tmp_point->reserved_instr; k++)
							if (allmodified || tmp_point->out_instr[k].modified) {
								instr = &(dsp_mgr->instr[k + tmp_point->out_instr_offset]);

								instr->used = 1;
								instr->modified = 1;
								instr->op_code = tmp_point->out_instr[k].op_code;
								for (l = 0; l < 4; l++)
									instr->arg[l] = ld10k1_dsp_mgr_get_phys_reg(dsp_mgr, tmp_point->out_instr[k].arg[l]);
								tmp_point->out_instr[k].modified = 0;
							}
						instr_offset += tmp_point->reserved_instr;
					}
				}
			} else {
				/* patch*/
				allmodified = 0;
				if (tmpp->instr_offset != instr_offset)
					allmodified = 1;

				tmpp->instr_offset = instr_offset;

				for (j = 0; j < tmpp->instr_count; j++)
					if (allmodified || tmpp->instr[j].modified) {
						instr = &(dsp_mgr->instr[j + tmpp->instr_offset]);

						instr->used = 1;
						instr->modified = 1;
						instr->op_code = tmpp->instr[j].op_code;
						for (k = 0; k < 4; k++)
							instr->arg[k] = ld10k1_dsp_mgr_get_phys_reg_for_patch(dsp_mgr, tmpp, tmpp->instr[j].arg[k]);
						tmpp->instr[j].modified = 0;
					}
				instr_offset += tmpp->instr_count;
			}
		}
	}

	for (j = instr_offset; j < dsp_mgr->instr_count; j++) {
		if (dsp_mgr->instr[j].used) {
			dsp_mgr->instr[j].modified = 1;
			dsp_mgr->instr[j].used = 0;
		}
	}
	return ld10k1_update_driver(dsp_mgr);
}

int ld10k1_dsp_mgr_actualize_instr_for_reg(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_patch_t *patch, unsigned int reg)
{
	int j, k;

	for (j = 0; j < patch->instr_count; j++)
		for (k = 0; k < 4; k++)
			if (patch->instr[j].arg[k] == reg) {
				patch->instr[j].modified = 1;
			}
	return 0;
}

void ld10k1_dsp_mgr_actualize_order(ld10k1_dsp_mgr_t *dsp_mgr)
{
	int i;

	for (i = 0; i < dsp_mgr->patch_count; i++)
		dsp_mgr->patch_ptr[dsp_mgr->patch_order[i]]->order = i;
}

int ld10k1_dsp_mgr_patch_load(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_patch_t *patch, int before, int *loaded)
{
	/* check if i can add patch */
	int pp, i, j;
	int err;

	int res[MAX_GPR_COUNT];
	int res_count = 0;
	int max_res_count = MAX_GPR_COUNT;
	int const_res[MAX_CONST_COUNT];
	int const_res_count = 0;
	int max_const_res_count = MAX_CONST_COUNT;

	unsigned int reserved;
	
	ld10k1_ctl_t tmp_ctl;

	ld10k1_dsp_tram_resolve_t tram_res;

	ld10k1_patch_t *tpatch;

	if (dsp_mgr->patch_count >= EMU10K1_PATCH_MAX)
		return LD10K1_ERR_MAX_PATCH_COUNT;

	/* get patch number */
	for (i = 0, pp = -1; i < dsp_mgr->patch_count; i++)
		if (dsp_mgr->patch_ptr[i] == NULL)
			pp = i;

	if (pp < 0)
		pp = dsp_mgr->patch_count;

	if (before > dsp_mgr->patch_count)
		before =  dsp_mgr->patch_count;

	/* static */
	for (i = 0; i < patch->sta_count; i++) {
		reserved = ld10k1_gpr_reserve(dsp_mgr, max_res_count, &res_count, res, GPR_USAGE_NORMAL, patch->stas[i].const_val);
		if (!reserved)
			return LD10K1_ERR_NOT_FREE_REG;
		patch->stas[i].gpr_idx = reserved;
	}

	/* constant */
	for (i = 0; i < patch->const_count; i++) {
		
		/* try allocate */
		reserved = ld10k1_const_reserve(dsp_mgr, max_const_res_count, &const_res_count, const_res,
			max_res_count, &res_count, res, patch->consts[i].const_val);
		if (reserved == 0)
			return LD10K1_ERR_NOT_FREE_REG;
		patch->consts[i].gpr_idx = reserved;
	}

	/* dynamic */
	for (i = 0; i < patch->dyn_count; i++) {
		reserved = ld10k1_gpr_dyn_reserve(dsp_mgr, max_res_count, &res_count, res);
		if (!reserved)
			return LD10K1_ERR_NOT_FREE_REG;
		patch->dyns[i].gpr_idx = reserved;
	}

	/* hw */
	for (i = 0; i < patch->hw_count; i++)
		patch->hws[i].gpr_idx = ld10k1_resolve_named_reg(dsp_mgr, patch->hws[i].reg_idx);

	/* ctl regs */
	for (i = 0; i < patch->ctl_count; i++) {
		for (j = 0; j < patch->ctl[i].count; j++) {
			reserved = ld10k1_gpr_reserve(dsp_mgr, max_res_count, &res_count, res,
				GPR_USAGE_NORMAL, patch->ctl[i].value[j]);
			if (!reserved)
				return LD10K1_ERR_NOT_FREE_REG;
			patch->ctl[i].gpr_idx[j] = reserved;
		}
	}

	if (dsp_mgr->instr_free < patch->instr_count)
		return LD10K1_ERR_NOT_FREE_INSTR;

	/* tram */
	if (patch->tram_count > 0)
		if ((err = ld10k1_tram_reserve_for_patch(dsp_mgr, patch, &tram_res)) < 0)
			return err;

	for (i = 0; i < patch->ctl_count; i++) {
		memcpy(&tmp_ctl, &(patch->ctl[i]), sizeof(ld10k1_ctl_t));
		/* resolve gpr_idx */
		for (j = 0; j < tmp_ctl.count; j++)
			tmp_ctl.gpr_idx[j] = tmp_ctl.gpr_idx[j] & ~EMU10K1_REG_TYPE_MASK;
		if ((err = ld10k1_add_control(dsp_mgr, &tmp_ctl)) < 0) {
			for (j = 0; j < i - 1; j++)
				ld10k1_del_control(dsp_mgr, &(patch->ctl[j]));
			return err;
		}
		
		/* copy index back */
		patch->ctl[i].index = tmp_ctl.index;
	}

	if (patch->tram_count > 0)
		if ((err = ld10k1_tram_alloc_for_patch(dsp_mgr, patch, &tram_res)) < 0)
			return err;

	for (i = dsp_mgr->patch_count - 1; i >= before ; i--)
		dsp_mgr->patch_order[i + 1] = dsp_mgr->patch_order[i];

	dsp_mgr->patch_order[before] = pp;
	dsp_mgr->patch_count++;

	dsp_mgr->patch_ptr[pp] = patch;
	loaded[0] = pp;

	patch->id = ld10k1_gen_patch_id(dsp_mgr, pp);
	loaded[1] = patch->id;

	/* allocate registers */
	for (i = 0; i < const_res_count; i++)
		ld10k1_const_alloc(dsp_mgr, const_res[i]);

	for (i = 0; i < res_count; i++)
		ld10k1_gpr_alloc(dsp_mgr, res[i]);

	/* actualize tram */
	if (patch->tram_count > 0)
		for (i = 0; i < dsp_mgr->patch_count; i++) {
			tpatch = dsp_mgr->patch_ptr[dsp_mgr->patch_order[i]];
			if (tpatch->tram_count)
				ld10k1_tram_actualize_tram_for_patch(dsp_mgr, tpatch);
		}

	dsp_mgr->instr_free -= patch->instr_count;

	ld10k1_dsp_mgr_actualize_order(dsp_mgr);
	return ld10k1_dsp_mgr_actualize_instr(dsp_mgr);
}

int ld10k1_dsp_mgr_patch_unload(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_patch_t *patch, unsigned int idx)
{
	unsigned int i,j ;

	/* free in registers */
	for (i = 0; i < patch->in_count; i++)
		if (patch->ins[i].point)
			ld10k1_conn_point_del(dsp_mgr, patch->ins[i].point, CON_IO_PIN, patch, i);

	/* free out registers */
	for (i = 0; i < patch->out_count; i++)
		if (patch->outs[i].point)
			ld10k1_conn_point_del(dsp_mgr, patch->outs[i].point, CON_IO_POUT, patch, i);

	/* free dyn registers */
	for (i = 0; i < patch->dyn_count; i++)
		ld10k1_gpr_free(dsp_mgr, patch->dyns[i].gpr_idx);

	/* free sta registers */
	for (i = 0; i < patch->sta_count; i++)
		ld10k1_gpr_free(dsp_mgr, patch->stas[i].gpr_idx);

	/* free const registers */
	for (i = 0; i < patch->const_count; i++)
		ld10k1_const_free(dsp_mgr, patch->consts[i].gpr_idx);

	/* free ctl regs */
	for (i = 0; i < patch->ctl_count; i++) {
		for (j = 0; j < patch->ctl[i].count; j++) {
			ld10k1_gpr_free(dsp_mgr, patch->ctl[i].gpr_idx[j]);
		}
	}

	/* free controls */
	for (i = 0; i < patch->ctl_count; i++)
		ld10k1_del_control(dsp_mgr, &(patch->ctl[i]));

	/* free tram */
	if (patch->tram_count > 0)
		ld10k1_tram_free_tram_for_patch(dsp_mgr, patch);

	/* free from registers */
	for (i = 0; i < dsp_mgr->patch_count; i++)
		if (dsp_mgr->patch_order[i] == idx)
			for (;i < dsp_mgr->patch_count; i++)
				dsp_mgr->patch_order[i] = dsp_mgr->patch_order[i + 1];
	dsp_mgr->patch_ptr[idx] = NULL;

	/* free from mem */
	ld10k1_dsp_mgr_patch_free(patch);

	/* decrement patch count */
	dsp_mgr->patch_count--;
	dsp_mgr->instr_free += patch->instr_count;

	ld10k1_dsp_mgr_actualize_order(dsp_mgr);
	/* actualize instructons */
	return ld10k1_dsp_mgr_actualize_instr(dsp_mgr);
}

int ld10k1_patch_fnc_check_patch(ld10k1_dsp_mgr_t *dsp_mgr,
	ld10k1_patch_t *new_patch)
{
	int i, j, k;

	/* check hw registers */
	for (i = 0; i < new_patch->hw_count; i++) {
		if (!ld10k1_resolve_named_reg(dsp_mgr, new_patch->hws[i].reg_idx))
			return LD10K1_ERR_WRONG_REG_HW_INDEX;
	}

	/* tram check */
	for (i = 0; i < new_patch->tram_count; i++) {
		if (new_patch->tram_grp[i].grp_type != TRAM_GRP_DELAY && new_patch->tram_grp[i].grp_type != TRAM_GRP_TABLE)
			return LD10K1_ERR_WRONG_TRAM_TYPE;
		if (new_patch->tram_grp[i].grp_size < 0)
			return LD10K1_ERR_WRONG_TRAM_SIZE;
		if (new_patch->tram_grp[i].grp_pos != TRAM_POS_AUTO &&
			new_patch->tram_grp[i].grp_pos != TRAM_POS_INTERNAL &&
			new_patch->tram_grp[i].grp_pos != TRAM_POS_EXTERNAL)
			return LD10K1_ERR_WRONG_TRAM_POS;
	}

	/* tram access check */
	for (i = 0; i < new_patch->tram_acc_count; i++) {
		/* type */
		if ((new_patch->tram_acc[i].acc_type & ~(TRAM_ACC_READ | TRAM_ACC_WRITE | TRAM_ACC_ZERO)) != 0)
			return LD10K1_ERR_WRONG_TRAM_ACC_TYPE;

		if (((new_patch->tram_acc[i].acc_type & (TRAM_ACC_READ | TRAM_ACC_WRITE)) != TRAM_ACC_READ) &&
			((new_patch->tram_acc[i].acc_type & (TRAM_ACC_READ | TRAM_ACC_WRITE)) != TRAM_ACC_WRITE))
			return LD10K1_ERR_WRONG_TRAM_ACC_TYPE;

		if (new_patch->tram_acc[i].grp < 0 && new_patch->tram_acc[i].grp >= new_patch->tram_count)
			return LD10K1_ERR_TRAM_GRP_OUT_OF_RANGE;

		if (new_patch->tram_acc[i].acc_offset < 0 || new_patch->tram_acc[i].acc_offset >= new_patch->tram_grp[new_patch->tram_acc[i].grp].grp_size)
			return LD10K1_ERR_TRAM_ACC_OUT_OF_RANGE;
	}

	/* control check */
	for (i = 0; i < new_patch->ctl_count; i++) {
		new_patch->ctl[i].name[43] = '\0';

		if (new_patch->ctl[i].vcount > new_patch->ctl[i].count)
			return LD10K1_ERR_CTL_VCOUNT_OUT_OF_RANGE;
		if (new_patch->ctl[i].count > MAX_CTL_GPR_COUNT)
			return LD10K1_ERR_CTL_COUNT_OUT_OF_RANGE;

		if (new_patch->ctl[i].min > new_patch->ctl[i].max)
			return LD10K1_ERR_CTL_MIN_MAX_RANGE;

		if (new_patch->ctl[i].translation < 0 || new_patch->ctl[i].translation > EMU10K1_GPR_TRANSLATION_LAST)
			return LD10K1_ERR_CTL_TRANLSLATION;

		for(j = 0; j < new_patch->ctl[i].count; j++) {
			if (new_patch->ctl[i].value[j] < new_patch->ctl[i].min || new_patch->ctl[i].value[j] > new_patch->ctl[i].max)
				return LD10K1_ERR_CTL_REG_VALUE;
		}
	}

	/* instruction check */
	for (i = 0; i < new_patch->instr_count; i++) {
		if (new_patch->instr[i].op_code < 0 || new_patch->instr[i].op_code > iSKIP)
			return LD10K1_ERR_INSTR_OPCODE;

		for (k = 0; k < 4; k++) {
			int arg = new_patch->instr[i].arg[k];
			switch (EMU10K1_PREG_TYPE_B(arg)) {
				case EMU10K1_PREG_TYPE_IN:
					if ((arg & 0xFFFFFFF) >= new_patch->in_count)
						return LD10K1_ERR_INSTR_ARG_INDEX;
					break;
				case EMU10K1_PREG_TYPE_OUT:
					if ((arg & 0xFFFFFFF) >= new_patch->out_count)
						return LD10K1_ERR_INSTR_ARG_INDEX;
					break;
				case EMU10K1_PREG_TYPE_CONST:
					if ((arg & 0xFFFFFFF) >= new_patch->const_count)
						return LD10K1_ERR_INSTR_ARG_INDEX;
					break;
				case EMU10K1_PREG_TYPE_STA:
					if ((arg & 0xFFFFFFF) >= new_patch->sta_count)
						return LD10K1_ERR_INSTR_ARG_INDEX;
					break;
				case EMU10K1_PREG_TYPE_DYN:
					if ((arg & 0xFFFFFFF) >= new_patch->dyn_count)
						return LD10K1_ERR_INSTR_ARG_INDEX;
					break;
				case EMU10K1_PREG_TYPE_HW:
					if ((arg & 0xFFFFFFF) >= new_patch->hw_count)
						return LD10K1_ERR_INSTR_ARG_INDEX;
					break;
				case EMU10K1_PREG_TYPE_CTL:
					if ((((arg & 0xFF00) >> 8) >= new_patch->ctl_count) ||
						((arg & 0xFF) >= new_patch->ctl[(arg & 0xFF00) >> 8].count))
						return LD10K1_ERR_INSTR_ARG_INDEX;
					break;
				case EMU10K1_PREG_TYPE_TRAM_DATA:
				case EMU10K1_PREG_TYPE_TRAM_ADDR:
					if ((arg & 0xFFFFFFF) >= new_patch->tram_acc_count)
						return LD10K1_ERR_INSTR_ARG_INDEX;
					break;
				default:
					return LD10K1_ERR_INSTR_ARG_INDEX;
			}
		}
	}

	return 0;
}

int ld10k1_patch_fnc_del(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_fnc_patch_del_t *patch_fnc)
{
	int err;

	if (patch_fnc->where >= 0 && patch_fnc->where < EMU10K1_PATCH_MAX) {
		if (dsp_mgr->patch_ptr[patch_fnc->where]) {
			if ((err = ld10k1_dsp_mgr_patch_unload(dsp_mgr,
				dsp_mgr->patch_ptr[patch_fnc->where],
				patch_fnc->where)) < 0)
				return err;
			else
				return 0;
		} else
			return LD10K1_ERR_UNKNOWN_PATCH_NUM;
	}
	else
		return LD10K1_ERR_UNKNOWN_PATCH_NUM;
}

int ld10k1_connection_fnc(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_fnc_connection_t *connection_fnc, int *conn_id)
{
	ld10k1_patch_t *from_patch = NULL;
	ld10k1_patch_t *to_patch = NULL;
	ld10k1_conn_point_t *from_point = NULL;
	ld10k1_conn_point_t *to_point = NULL;
	ld10k1_conn_point_t *tmp_point = NULL;
	int err;

	if (connection_fnc->what == FNC_CONNECTION_ADD) {
		if (connection_fnc->from_type != CON_IO_PIN &&
			connection_fnc->from_type != CON_IO_POUT &&
			connection_fnc->from_type != CON_IO_FX &&
			connection_fnc->from_type != CON_IO_IN &&
			connection_fnc->from_type != CON_IO_OUT)
			return LD10K1_ERR_CONNECTION;
		if (connection_fnc->to_type != CON_IO_PIN &&
			connection_fnc->to_type != CON_IO_POUT &&
			connection_fnc->to_type != CON_IO_FX &&
			connection_fnc->to_type != CON_IO_IN &&
			connection_fnc->to_type != CON_IO_OUT)
			return LD10K1_ERR_CONNECTION;

		/* first must be patch */
		if (connection_fnc->from_type != CON_IO_PIN && connection_fnc->from_type != CON_IO_POUT)
			return LD10K1_ERR_CONNECTION;

		if (connection_fnc->from_type == CON_IO_PIN &&
			connection_fnc->to_type == CON_IO_OUT)
			return LD10K1_ERR_CONNECTION;
		if (connection_fnc->from_type == CON_IO_POUT &&
			(connection_fnc->to_type == CON_IO_FX ||
			connection_fnc->to_type == CON_IO_IN))
			return LD10K1_ERR_CONNECTION;

		if (connection_fnc->from_patch < 0 || connection_fnc->from_patch >= EMU10K1_PATCH_MAX)
			return LD10K1_ERR_UNKNOWN_PATCH_NUM;
		from_patch = dsp_mgr->patch_ptr[connection_fnc->from_patch];
		if (!from_patch)
			return LD10K1_ERR_UNKNOWN_PATCH_NUM;

		if (connection_fnc->to_type == CON_IO_PIN ||
			connection_fnc->to_type == CON_IO_POUT) {
			if (connection_fnc->to_patch < 0 || connection_fnc->to_patch >= EMU10K1_PATCH_MAX)
				return LD10K1_ERR_UNKNOWN_PATCH_NUM;
			to_patch = dsp_mgr->patch_ptr[connection_fnc->to_patch];
			if (!to_patch)
				return LD10K1_ERR_UNKNOWN_PATCH_NUM;
		}
		else
			to_patch = NULL;
			
		if (from_patch == to_patch)
			return LD10K1_ERR_CONNECTION;

		if (connection_fnc->to_io < 0)
			return LD10K1_ERR_CONNECTION;
		if (connection_fnc->from_io < 0)
			return LD10K1_ERR_UNKNOWN_PATCH_REG_NUM;
		if (connection_fnc->from_type == CON_IO_PIN) {
			if (connection_fnc->from_io >= from_patch->in_count)
				return LD10K1_ERR_UNKNOWN_PATCH_REG_NUM;
			from_point = from_patch->ins[connection_fnc->from_io].point;
		} else {
			if (connection_fnc->from_io >= from_patch->out_count)
				return LD10K1_ERR_UNKNOWN_PATCH_REG_NUM;
			from_point = from_patch->outs[connection_fnc->from_io].point;
		}

		switch (connection_fnc->to_type) {
			case CON_IO_FX:
				if (connection_fnc->to_io >= dsp_mgr->fx_count)
					return LD10K1_ERR_CONNECTION;
				to_point = dsp_mgr->fxs[connection_fnc->to_io].point;
				break;
			case CON_IO_IN:
				if (connection_fnc->to_io >= dsp_mgr->in_count)
					return LD10K1_ERR_CONNECTION;
				to_point = dsp_mgr->ins[connection_fnc->to_io].point;
				break;
			case CON_IO_OUT:
				if (connection_fnc->to_io >= dsp_mgr->out_count)
					return LD10K1_ERR_CONNECTION;
				to_point = dsp_mgr->outs[connection_fnc->to_io].point;
				break;
			case CON_IO_PIN:
				if (connection_fnc->to_io >= to_patch->in_count)
					return LD10K1_ERR_UNKNOWN_PATCH_REG_NUM;
				to_point = to_patch->ins[connection_fnc->to_io].point;
				break;
			case CON_IO_POUT:
				if (connection_fnc->to_io >= to_patch->out_count)
					return LD10K1_ERR_UNKNOWN_PATCH_REG_NUM;
				to_point = to_patch->outs[connection_fnc->to_io].point;
				break;
		}

		if (from_patch && to_patch) {
			if (from_point == to_point)
				if (from_point)
					if (connection_fnc->multi || from_point->con_count == 2)
						return 0;
			if (!connection_fnc->multi || !to_point) {
				if (!(tmp_point = ld10k1_conn_point_alloc(connection_fnc->simple)))
					return LD10K1_ERR_NO_MEM;
				if ((err = ld10k1_conn_point_set_to(dsp_mgr, tmp_point, 0, -1)) < 0) {
					ld10k1_conn_point_free(dsp_mgr, tmp_point);
					return err;
				}

				/* reconnect to patch */
				if ((err = ld10k1_conn_point_add(dsp_mgr, tmp_point, connection_fnc->to_type, to_patch, connection_fnc->to_io)) < 0) {
					ld10k1_conn_point_free(dsp_mgr, tmp_point);
					return err;
				}
			} else
				tmp_point = to_point;

			/* reconnect from patch */
			if ((err = ld10k1_conn_point_add(dsp_mgr, tmp_point, connection_fnc->from_type, from_patch, connection_fnc->from_io)) < 0) {
				/* if point is allocated now this call can't fail */
				/* ld10k1_conn_point_free(dsp_mgr, tmp_point); */
				return err;
			}
		} else {
			if (from_point == to_point && from_point)
				return 0;
			if (!to_point) {
				if (!(tmp_point = ld10k1_conn_point_alloc(0)))
					return LD10K1_ERR_NO_MEM;
				if ((err = ld10k1_conn_point_set_to(dsp_mgr, tmp_point, connection_fnc->to_type, connection_fnc->to_io)) < 0) {
					ld10k1_conn_point_free(dsp_mgr, tmp_point);
					return err;
				}
			} else
				tmp_point = to_point;

			/* reconnect from patch */
			if ((err = ld10k1_conn_point_add(dsp_mgr, tmp_point, connection_fnc->from_type, from_patch, connection_fnc->from_io)) < 0) {
				/* if point is allocated now this call can't fail */
				/* ld10k1_conn_point_free(dsp_mgr, tmp_point); */
				return err;
			}
		}

		if (tmp_point->id <= 0)
			tmp_point->id = ld10k1_gen_patch_id(dsp_mgr, connection_fnc->from_patch);
			
		*conn_id = tmp_point->id;
		
		ld10k1_dsp_mgr_actualize_instr(dsp_mgr);
		return 0;
	} if (connection_fnc->what == FNC_CONNECTION_DEL) {
		if (connection_fnc->from_type != CON_IO_PIN &&
			connection_fnc->from_type != CON_IO_POUT)
			return LD10K1_ERR_CONNECTION;

		if (connection_fnc->from_patch < 0 || connection_fnc->from_patch >= EMU10K1_PATCH_MAX)
			return LD10K1_ERR_UNKNOWN_PATCH_NUM;
		from_patch = dsp_mgr->patch_ptr[connection_fnc->from_patch];
		if (!from_patch)
			return LD10K1_ERR_UNKNOWN_PATCH_NUM;

		if (connection_fnc->from_io < 0)
			return LD10K1_ERR_UNKNOWN_PATCH_REG_NUM;

		if (connection_fnc->from_type == CON_IO_PIN) {
			if (connection_fnc->from_io >= from_patch->in_count)
				return LD10K1_ERR_UNKNOWN_PATCH_REG_NUM;
			from_point = from_patch->ins[connection_fnc->from_io].point;
		} else {
			if (connection_fnc->from_io >= from_patch->out_count)
				return LD10K1_ERR_UNKNOWN_PATCH_REG_NUM;
			from_point = from_patch->outs[connection_fnc->from_io].point;
		}
		
		*conn_id = -1;

		if (!from_point)
			return 0;

		if ((EMU10K1_REG_TYPE_B(from_point->con_gpr_idx) == EMU10K1_REG_TYPE_NORMAL && from_point->con_count > 1) ||
			from_point->con_count > 0)
			*conn_id = from_point->id;			
			
		ld10k1_conn_point_del(dsp_mgr, from_point, connection_fnc->from_type, from_patch, connection_fnc->from_io);

		ld10k1_dsp_mgr_actualize_instr(dsp_mgr);
		return 0;
	} else
		return LD10K1_ERR_CONNECTION_FNC;
}

ld10k1_ctl_list_item_t *ld10k1_look_control_from_list(ld10k1_ctl_list_item_t *list, ld10k1_ctl_t *gctl)
{
	ld10k1_ctl_list_item_t *item;

	for (item = list; item != NULL; item = item->next)
		if (strcmp(item->ctl.name, gctl->name) == 0 && item->ctl.index == gctl->index)
			return item;

	return NULL;
}

int ld10k1_add_control_to_list(ld10k1_ctl_list_item_t **list, int *count, ld10k1_ctl_t *gctl)
{
	ld10k1_ctl_list_item_t *item;
	
	item = ld10k1_look_control_from_list(*list, gctl);
	if (!item) {
		item = (ld10k1_ctl_list_item_t *)malloc(sizeof(ld10k1_ctl_list_item_t));
		if (!item)
			return LD10K1_ERR_NO_MEM;

		item->next = NULL;

		if (!*list)
			/* empty */
			*list = item;
		else {
			/* add to begining */
			item->next = *list;
			*list = item;
		}
		(*count)++;
	}

	memcpy(&(item->ctl), gctl, sizeof(*gctl));
	
	return 0;
}

void ld10k1_del_control_from_list(ld10k1_ctl_list_item_t **list, int *count, ld10k1_ctl_t *gctl)
{
	ld10k1_ctl_list_item_t *item;
	ld10k1_ctl_list_item_t *item1;

	if (!*list)
		return;

	item = ld10k1_look_control_from_list(*list, gctl);
	if (!item)
		return;
		
	if (*list == item) {
		*list = item->next;
	} else {
		for (item1 = *list; item1->next != NULL; item1 = item1->next)
			if (item1->next == item) {
				item1->next = item->next;
				break;
			}
	}

	free(item);
	(*count)--;
}

void ld10k1_del_all_controls_from_list(ld10k1_ctl_list_item_t **list, int *count)
{
	ld10k1_ctl_list_item_t *item;
	ld10k1_ctl_list_item_t *item1;
	
	for (item = *list; item != NULL;) {
		item1 = item->next;
		free(item);
		item = item1;
	}
	
	*count = 0;
	*list = NULL;
}

int ld10k1_get_used_index_for_control(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_ctl_t *gctl, int **idxs, int *cnt)
{
	int i;
	ld10k1_ctl_list_item_t *item;
	ld10k1_reserved_ctl_list_item_t *itemr;
	int count;
	int *index_list;

	count = 0;
	i = 0;
	
	/* first get count */
	for (item = dsp_mgr->ctl_list; item != NULL; item = item->next)
		if (strcmp(item->ctl.name, gctl->name) == 0)
			count++;
			
	for (item = dsp_mgr->add_ctl_list; item != NULL; item = item->next)
		if (strcmp(item->ctl.name, gctl->name) == 0)
			count++;
			
	for (itemr = dsp_mgr->reserved_ctl_list; itemr != NULL; itemr = itemr->next)
		if (strcmp(itemr->res_ctl.name, gctl->name) == 0)
			count++;
			
	if (!count) {
		*idxs = NULL;
		*cnt = 0;
		return 0;
	}
	
	/* second get array */
	index_list = (int *)malloc(sizeof(int) * count);
	if (!index_list)
		return LD10K1_ERR_NO_MEM;
		
	for (item = dsp_mgr->ctl_list; item != NULL; item = item->next)
		if (strcmp(item->ctl.name, gctl->name) == 0)
			index_list[i++] = item->ctl.index;
	
	for (item = dsp_mgr->add_ctl_list; item != NULL; item = item->next)
		if (strcmp(item->ctl.name, gctl->name) == 0)
			index_list[i++] = item->ctl.index;
	
	for (itemr = dsp_mgr->reserved_ctl_list; itemr != NULL; itemr = itemr->next)
		if (strcmp(itemr->res_ctl.name, gctl->name) == 0)
			index_list[i++] = itemr->res_ctl.index;
			
	*idxs = index_list;
	*cnt = count;

	return 0;
}


int ld10k1_add_control(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_ctl_t *gctl)
{
	int err;
	int *idxs;
	int cnt;
	int new_idx;
	int i;
	int found;
	
	new_idx = 0;
	
	/* check index */
	if (gctl->want_index < 0) {
		/* find free index */
		if ((err = ld10k1_get_used_index_for_control(dsp_mgr, gctl, &idxs, &cnt)) < 0)
			return err;
	
		while (new_idx < 1000) {
			found = 0;
			for (i = 0; i < cnt; i++) {
				if (new_idx == idxs[i]) {
					new_idx++;
					found = 1;
					break;
				}
			}
		 	if (!found) {
				gctl->index = new_idx;
				break;
			}
		}	
		
		if (idxs)
			free(idxs);
		
		if (new_idx >= 1000)
			return LD10K1_ERR_CTL_EXISTS;
	}
	else
		gctl->index = gctl->want_index;
	
	/* is there control ??? */
	if (ld10k1_look_control_from_list(dsp_mgr->ctl_list, gctl))
		return LD10K1_ERR_CTL_EXISTS;
	/* is for add ??? */
	if (ld10k1_look_control_from_list(dsp_mgr->add_ctl_list, gctl))
		return 0;

	/* add */
	return ld10k1_add_control_to_list(&(dsp_mgr->add_ctl_list), &(dsp_mgr->add_list_count), gctl);
}

void ld10k1_del_control(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_ctl_t *gctl)
{
	/* is for add ??? */
	if (ld10k1_look_control_from_list(dsp_mgr->add_ctl_list, gctl)) {
		ld10k1_del_control_from_list(&(dsp_mgr->add_ctl_list), &(dsp_mgr->add_list_count), gctl);
		return;
	}
	
	/* is for del ??? */
	if (ld10k1_look_control_from_list(dsp_mgr->del_ctl_list, gctl))
		return;

	/* delete ??? */
	if (ld10k1_look_control_from_list(dsp_mgr->ctl_list, gctl)) {
		ld10k1_add_control_to_list(&(dsp_mgr->del_ctl_list), &(dsp_mgr->del_list_count), gctl);
		return;
	}
	return;
}

/* this will by usefull with modified as10k1 */
unsigned int named_to_standard[] =
{
	/* FX buses */
	EMU10K1_NREG_FXBUS_PCM_LEFT,		EMU10K1_REG_FX(0x00),	EMU10K1_REG_FX(0x00),
	EMU10K1_NREG_FXBUS_PCM_RIGHT,		EMU10K1_REG_FX(0x01),	EMU10K1_REG_FX(0x01),
	EMU10K1_NREG_FXBUS_PCM_FRONT_LEFT,	EMU10K1_REG_FX(0x08),	EMU10K1_REG_FX(0x08),
	EMU10K1_NREG_FXBUS_PCM_FRONT_RIGHT,	EMU10K1_REG_FX(0x09),	EMU10K1_REG_FX(0x09),
	EMU10K1_NREG_FXBUS_PCM_REAR_LEFT,	EMU10K1_REG_FX(0x02),	EMU10K1_REG_FX(0x02),
	EMU10K1_NREG_FXBUS_PCM_REAR_RIGHT,	EMU10K1_REG_FX(0x03),	EMU10K1_REG_FX(0x03),
	EMU10K1_NREG_FXBUS_PCM_CENTER,		EMU10K1_REG_FX(0x06),	EMU10K1_REG_FX(0x06),
	EMU10K1_NREG_FXBUS_PCM_LFE,			EMU10K1_REG_FX(0x07),	EMU10K1_REG_FX(0x07),
	EMU10K1_NREG_FXBUS_MIDI_LEFT,		EMU10K1_REG_FX(0x04),	EMU10K1_REG_FX(0x04),
	EMU10K1_NREG_FXBUS_MIDI_RIGHT,		EMU10K1_REG_FX(0x05),	EMU10K1_REG_FX(0x05),
	EMU10K1_NREG_FXBUS_MIDI_REVERB,		EMU10K1_REG_FX(0x0C),	EMU10K1_REG_FX(0x0C),
	EMU10K1_NREG_FXBUS_MIDI_CHORUS,		EMU10K1_REG_FX(0x0D),	EMU10K1_REG_FX(0x0D),

	EMU10K1_A_NREG_FXBUS_PT_LEFT,		0,						EMU10K1_REG_FX(0x14),
	EMU10K1_A_NREG_FXBUS_PT_RIGHT,		0,						EMU10K1_REG_FX(0x15),

	/* inputs */
	EMU10K1_NREG_IN_AC97_LEFT,			EMU10K1_REG_IN(0x00),	EMU10K1_REG_IN(0x00),
	EMU10K1_NREG_IN_AC97_RIGHT,			EMU10K1_REG_IN(0x01),	EMU10K1_REG_IN(0x01),
	EMU10K1_NREG_IN_SPDIF_CD_LEFT,		EMU10K1_REG_IN(0x02),	EMU10K1_REG_IN(0x02),
	EMU10K1_NREG_IN_SPDIF_CD_RIGHT,		EMU10K1_REG_IN(0x03),	EMU10K1_REG_IN(0x03),
	EMU10K1_NREG_IN_SPDIF_OPT_LEFT,		EMU10K1_REG_IN(0x06),	EMU10K1_REG_IN(0x04),
	EMU10K1_NREG_IN_SPDIF_OPT_RIGHT,	EMU10K1_REG_IN(0x07),	EMU10K1_REG_IN(0x05),
	EMU10K1_NREG_IN_I2S_1_LEFT,			EMU10K1_REG_IN(0x08),	EMU10K1_REG_IN(0x08),
	EMU10K1_NREG_IN_I2S_1_RIGHT,		EMU10K1_REG_IN(0x09),	EMU10K1_REG_IN(0x09),
	EMU10K1_NREG_IN_I2S_2_LEFT,			EMU10K1_REG_IN(0x0C),	EMU10K1_REG_IN(0x0C),
	EMU10K1_NREG_IN_I2S_2_RIGHT,		EMU10K1_REG_IN(0x0D),	EMU10K1_REG_IN(0x0D),

	EMU10K1_L_NREG_IN_SPDIF_COAX_LEFT,	EMU10K1_REG_IN(0x0A),	0,
	EMU10K1_L_NREG_IN_SPDIF_COAX_RIGHT,	EMU10K1_REG_IN(0x0B),	0,
	EMU10K1_L_NREG_IN_ZOOM_LEFT,		EMU10K1_REG_IN(0x04),	0,
	EMU10K1_L_NREG_IN_ZOOM_RIGHT,		EMU10K1_REG_IN(0x05),	0,
	EMU10K1_L_NREG_IN_LINE_1_LEFT,		EMU10K1_REG_IN(0x08),	0,
	EMU10K1_L_NREG_IN_LINE_1_RIGHT,		EMU10K1_REG_IN(0x09),	0,
	EMU10K1_L_NREG_IN_LINE_2_LEFT,		EMU10K1_REG_IN(0x0C),	0,
	EMU10K1_L_NREG_IN_LINE_2_RIGHT,		EMU10K1_REG_IN(0x0D),	0,

	EMU10K1_A_NREG_IN_LINE_1_LEFT,		0,						EMU10K1_REG_IN(0x0A),
	EMU10K1_A_NREG_IN_LINE_1_RIGHT,		0,						EMU10K1_REG_IN(0x0B),
	EMU10K1_A_NREG_IN_LINE_2_LEFT,		0,						EMU10K1_REG_IN(0x08),
	EMU10K1_A_NREG_IN_LINE_2_RIGHT,		0,						EMU10K1_REG_IN(0x09),
	EMU10K1_A_NREG_IN_LINE_3_LEFT,		0,						EMU10K1_REG_IN(0x0C),
	EMU10K1_A_NREG_IN_LINE_3_RIGHT,		0,						EMU10K1_REG_IN(0x0D),

	/* outputs */
	EMU10K1_NREG_OUT_FRONT_LEFT,		EMU10K1_REG_OUT(0x00),	EMU10K1_REG_OUT(0x08),
	EMU10K1_NREG_OUT_FRONT_RIGHT,		EMU10K1_REG_OUT(0x01),	EMU10K1_REG_OUT(0x09),
	EMU10K1_NREG_OUT_REAR_LEFT,			EMU10K1_REG_OUT(0x08),	EMU10K1_REG_OUT(0x0E),
	EMU10K1_NREG_OUT_REAR_RIGHT,		EMU10K1_REG_OUT(0x09),	EMU10K1_REG_OUT(0x0F),
	EMU10K1_NREG_OUT_CENTER,			EMU10K1_REG_OUT(0x04),	EMU10K1_REG_OUT(0x0A),
	EMU10K1_NREG_OUT_LFE,				EMU10K1_REG_OUT(0x05),	EMU10K1_REG_OUT(0x0B),
	EMU10K1_NREG_OUT_AC97_LEFT,			EMU10K1_REG_OUT(0x00),	EMU10K1_REG_OUT(0x10),
	EMU10K1_NREG_OUT_AC97_RIGHT,		EMU10K1_REG_OUT(0x01),	EMU10K1_REG_OUT(0x11),
	EMU10K1_NREG_OUT_ADC_LEFT,			EMU10K1_REG_OUT(0x0A),	EMU10K1_REG_OUT(0x16),
	EMU10K1_NREG_OUT_ADC_RIGHT,			EMU10K1_REG_OUT(0x0B),	EMU10K1_REG_OUT(0x17),
	EMU10K1_NREG_OUT_MIC,				EMU10K1_REG_OUT(0x0C),	EMU10K1_REG_OUT(0x18),
	EMU10K1_NREG_OUT_HEADPHONE_LEFT,	EMU10K1_REG_OUT(0x06),	EMU10K1_REG_OUT(0x04),
	EMU10K1_NREG_OUT_HEADPHONE_RIGHT,	EMU10K1_REG_OUT(0x07),	EMU10K1_REG_OUT(0x05),

	EMU10K1_L_NREG_OUT_OPT_LEFT,		EMU10K1_REG_OUT(0x02),	0,
	EMU10K1_L_NREG_OUT_OPT_RIGHT,		EMU10K1_REG_OUT(0x03),	0,

	EMU10K1_A_NREG_OUT_D_FRONT_LEFT,	0,						EMU10K1_REG_OUT(0x00),
	EMU10K1_A_NREG_OUT_D_FRONT_RIGHT,	0,						EMU10K1_REG_OUT(0x01),
	EMU10K1_A_NREG_OUT_D_REAR_LEFT,		0,						EMU10K1_REG_OUT(0x06),
	EMU10K1_A_NREG_OUT_D_REAR_RIGHT,	0,						EMU10K1_REG_OUT(0x07),
	EMU10K1_A_NREG_OUT_D_CENTER,		0,						EMU10K1_REG_OUT(0x02),
	EMU10K1_A_NREG_OUT_D_LFE,			0,						EMU10K1_REG_OUT(0x03),

	/* hardware */
	EMU10K1_NREG_CONST_00000000,		EMU10K1_REG_HW(0x00),	EMU10K1_REG_HW(0x00),
	EMU10K1_NREG_CONST_00000001,		EMU10K1_REG_HW(0x01),	EMU10K1_REG_HW(0x01),
	EMU10K1_NREG_CONST_00000002,		EMU10K1_REG_HW(0x02),	EMU10K1_REG_HW(0x02),
	EMU10K1_NREG_CONST_00000003,		EMU10K1_REG_HW(0x03),	EMU10K1_REG_HW(0x03),
	EMU10K1_NREG_CONST_00000004,		EMU10K1_REG_HW(0x04),	EMU10K1_REG_HW(0x04),
	EMU10K1_NREG_CONST_00000008,		EMU10K1_REG_HW(0x05),	EMU10K1_REG_HW(0x05),
	EMU10K1_NREG_CONST_00000010,		EMU10K1_REG_HW(0x06),	EMU10K1_REG_HW(0x06),
	EMU10K1_NREG_CONST_00000020,		EMU10K1_REG_HW(0x07),	EMU10K1_REG_HW(0x07),
	EMU10K1_NREG_CONST_00000100,		EMU10K1_REG_HW(0x08),	EMU10K1_REG_HW(0x08),
	EMU10K1_NREG_CONST_00010000,		EMU10K1_REG_HW(0x09),	EMU10K1_REG_HW(0x09),
	EMU10K1_L_NREG_CONST_00080000,		EMU10K1_REG_HW(0x0A),	0,
	EMU10K1_A_NREG_CONST_00000800,		0,						EMU10K1_REG_HW(0x0A),
	EMU10K1_NREG_CONST_10000000,		EMU10K1_REG_HW(0x0B),	EMU10K1_REG_HW(0x0B),
	EMU10K1_NREG_CONST_20000000,		EMU10K1_REG_HW(0x0C),	EMU10K1_REG_HW(0x0C),
	EMU10K1_NREG_CONST_40000000,		EMU10K1_REG_HW(0x0D),	EMU10K1_REG_HW(0x0D),
	EMU10K1_NREG_CONST_80000000,		EMU10K1_REG_HW(0x0E),	EMU10K1_REG_HW(0x0E),
	EMU10K1_NREG_CONST_7FFFFFFF,		EMU10K1_REG_HW(0x0F),	EMU10K1_REG_HW(0x0F),
	EMU10K1_NREG_CONST_FFFFFFFF,		EMU10K1_REG_HW(0x10),	EMU10K1_REG_HW(0x10),
	EMU10K1_NREG_CONST_FFFFFFFE,		EMU10K1_REG_HW(0x11),	EMU10K1_REG_HW(0x11),
	EMU10K1_NREG_CONST_C0000000,		EMU10K1_REG_HW(0x12),	EMU10K1_REG_HW(0x12),
	EMU10K1_NREG_CONST_4F1BBCDC,		EMU10K1_REG_HW(0x13),	EMU10K1_REG_HW(0x13),
	EMU10K1_NREG_CONST_5A7EF9DB,		EMU10K1_REG_HW(0x14),	EMU10K1_REG_HW(0x14),
	EMU10K1_NREG_CONST_00100000,		EMU10K1_REG_HW(0x15),	EMU10K1_REG_HW(0x15),

	EMU10K1_NREG_HW_ACCUM,				EMU10K1_REG_HW(0x16),	EMU10K1_REG_HW(0x16),
	EMU10K1_NREG_HW_CCR,				EMU10K1_REG_HW(0x17),	EMU10K1_REG_HW(0x17),
	EMU10K1_NREG_HW_NOISE1,				EMU10K1_REG_HW(0x18),	EMU10K1_REG_HW(0x18),
	EMU10K1_NREG_HW_NOISE2,				EMU10K1_REG_HW(0x19),	EMU10K1_REG_HW(0x19),
	EMU10K1_NREG_HW_IRQ,				EMU10K1_REG_HW(0x1A),	EMU10K1_REG_HW(0x1A),
	EMU10K1_NREG_HW_DBAC,				EMU10K1_REG_HW(0x1B),	EMU10K1_REG_HW(0x1B),
	EMU10K1_A_NREG_HW_DBACE,			0,						EMU10K1_REG_HW(0x1D),
	0
};

unsigned int ld10k1_resolve_named_reg(ld10k1_dsp_mgr_t *dsp_mgr, unsigned int reg)
{
	/* find named - better will be use of binary search */
	int i;

	i = 0;
	while (named_to_standard[i]) {
		if (named_to_standard[i] == reg)
			return dsp_mgr->audigy ? named_to_standard[i + 2] : named_to_standard[i + 1];
		i += 3;
	}

	return 0;
}

unsigned int ld10k1_gpr_reserve(ld10k1_dsp_mgr_t *dsp_mgr, int max_res_count, int *res_count, int *res,
	unsigned int usage, unsigned int val)
{
	int i, j;
	if (*res_count >= max_res_count)
		return 0;

	for (i = 0; i < dsp_mgr->regs_max_count; i++) {
		if (!dsp_mgr->regs[i].used) {
			/* check in reserved */
			for (j = 0; j < *res_count; j++) {
				if (res[j] == i)
					break;
			}

			if (j >= *res_count) {
				res[*res_count] = i;
				(*res_count)++;
				dsp_mgr->regs[i].gpr_usage = usage;
				dsp_mgr->regs[i].val = val;
				return EMU10K1_REG_NORMAL(i);
			}
		}
	}
	return 0;
}

unsigned int ld10k1_gpr_dyn_reserve(ld10k1_dsp_mgr_t *dsp_mgr, int max_res_count, int *res_count, int *res)
{
	int i, j;
	if (*res_count >= max_res_count)
		return 0;

	/* try find other dyn not reserved */
	for (i = 0; i < dsp_mgr->regs_max_count; i++) {
		if (dsp_mgr->regs[i].used && dsp_mgr->regs[i].gpr_usage == GPR_USAGE_DYNAMIC) {
			/* check in reserved */
			for (j = 0; j < *res_count; j++) {
				if (res[j] == i)
					break;
			}

			if (j >= *res_count) {
				res[*res_count] = i;
				(*res_count)++;
				dsp_mgr->regs[i].gpr_usage = GPR_USAGE_DYNAMIC;
				dsp_mgr->regs[i].val = 0;
				return EMU10K1_REG_NORMAL(i);
			}
		}
	}

	/* not found - try normal */
	return ld10k1_gpr_reserve(dsp_mgr, max_res_count, res_count, res, GPR_USAGE_DYNAMIC, 0);
}

void ld10k1_gpr_alloc(ld10k1_dsp_mgr_t *dsp_mgr, int reg)
{
	int i = reg & 0x0FFFFFFF;
	dsp_mgr->regs[i].ref++;
	dsp_mgr->regs[i].modified = 1;
	dsp_mgr->regs[i].used = 1;
}

void ld10k1_gpr_free(ld10k1_dsp_mgr_t *dsp_mgr, int reg)
{
	int i = reg & 0x0FFFFFFF;
	dsp_mgr->regs[i].gpr_usage = GPR_USAGE_NONE;
	dsp_mgr->regs[i].val = 0;
	dsp_mgr->regs[i].ref--;
	dsp_mgr->regs[i].modified = 1;
	dsp_mgr->regs[i].used = 0;
}

unsigned int ld10k1_const_reserve(ld10k1_dsp_mgr_t *dsp_mgr, int max_res_const_count, int *res_const_count, int *res_const,
	int max_res_count, int *res_count, int *res, int const_val)
{
	int i, j;
	int free_gpr;

	if (*res_const_count >= max_res_const_count)
		return 0;

	/* check in reserved */
	for (i = 0; i < *res_const_count; i++) {
		if (dsp_mgr->consts[res_const[i]].const_val == const_val)
			return EMU10K1_REG_CONST(res_const[i]);
	}

	/* check in all constants */
	for (i = 0; i < dsp_mgr->consts_max_count; i++)
		if (dsp_mgr->consts[i].used && dsp_mgr->consts[i].const_val == const_val) {
			/* add to reserved */
			res_const[*res_const_count] = i;
			(*res_const_count)++;
			return EMU10K1_REG_CONST(i);
		}


	/* try find other dyn not reserved */
	for (i = 0; i < dsp_mgr->consts_max_count; i++) {
		if (!dsp_mgr->consts[i].used) {
			/* there is free room */
			/* if in reserved continue */
			for (j = 0; j < *res_const_count; j++) {
				if (res_const[j] == i)
					break;
			}
			if (j < *res_const_count)
				continue;

			free_gpr = ld10k1_gpr_reserve(dsp_mgr, max_res_count, res_count, res, GPR_USAGE_CONST, const_val);
			if (!free_gpr)
				return 0;
			res_const[*res_const_count] = i;
			(*res_const_count)++;
			dsp_mgr->consts[i].gpr_idx = free_gpr;
			dsp_mgr->consts[i].const_val = const_val;
			dsp_mgr->consts[i].hw = 0;
			return EMU10K1_REG_CONST(i);
		}
	}

	return 0;
}

void ld10k1_const_alloc(ld10k1_dsp_mgr_t *dsp_mgr, int reg)
{
	int i = reg & 0x0FFFFFFF;
	dsp_mgr->consts[i].ref++;
	if (!dsp_mgr->consts[i].used) {
		/*ld10k1_gpr_free(dsp_mgr, dsp_mgr->consts[i].gpr_idx);*/
		dsp_mgr->consts[i].used = 1;
	}
}

void ld10k1_const_free(ld10k1_dsp_mgr_t *dsp_mgr, int reg)
{
	int i = reg & 0x0FFFFFFF;
	dsp_mgr->consts[i].ref--;
	if (dsp_mgr->consts[i].ref == 0) {
		if (!dsp_mgr->consts[i].hw)
			dsp_mgr->consts[i].used = 0;
	}
}

ld10k1_conn_point_t *ld10k1_conn_point_alloc(int simple)
{
	ld10k1_conn_point_t *tmp = (ld10k1_conn_point_t *)malloc(sizeof(ld10k1_conn_point_t));
	int i;

	if (!tmp)
		return NULL;

	tmp->simple = simple;
	tmp->id = 0;

	tmp->next = NULL;
	tmp->con_count = 0;
	tmp->con_gpr_idx = 0;

	tmp->reserved_gpr = 0;
	tmp->reserved_instr = 0;

	tmp->out_instr_offset = 0;

	for (i = 0; i < MAX_CONN_PER_POINT; i++) {
		tmp->type[i] = 0;
		tmp->patch[i] = NULL;
		tmp->io[i] = -1;
		tmp->out_gpr_idx[i] = 0;
	}

	tmp->owner = NULL;
	tmp->position = INSERT_BEFORE_OWNER;
	return tmp;
}

void ld10k1_conn_point_free(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_conn_point_t *point)
{
	int i;

	point->owner = NULL;

	for (i = 0; i < MAX_CONN_PER_POINT; i++)
		if (point->type[i] != 0) {
		    if (point->out_gpr_idx[i]) {
				ld10k1_gpr_free(dsp_mgr, point->out_gpr_idx[i]);
				point->out_gpr_idx[i] = 0;
			}

			if (point->type[i] == CON_IO_PIN) {
				point->patch[i]->ins[point->io[i]].point = NULL;
				ld10k1_dsp_mgr_actualize_instr_for_reg(dsp_mgr, point->patch[i], EMU10K1_PREG_IN(point->io[i]));
			} else {
				point->patch[i]->outs[point->io[i]].point = NULL;
				ld10k1_dsp_mgr_actualize_instr_for_reg(dsp_mgr, point->patch[i], EMU10K1_PREG_OUT(point->io[i]));
			}

			point->con_count--;

			point->type[i] = 0;
			point->patch[i] = NULL;
			point->io[i] = -1;
		}

	dsp_mgr->instr_free += point->reserved_instr;
	point->reserved_instr = 0;

	ld10k1_conn_point_unset(dsp_mgr, point);
}

void ld10k1_point_actualize_owner(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_conn_point_t *point)
{
	/* instructions should be alocated */
	int i;
	int icount, iarg, iout;

	ld10k1_patch_t *tmp_owner = NULL;

	if (point->simple)
		return;

	if (point->reserved_gpr > 0) {
		if (EMU10K1_REG_TYPE_B(point->con_gpr_idx) == EMU10K1_REG_TYPE_NORMAL) {
			/* patch reg */
			for (i = 0; i < MAX_CONN_PER_POINT; i++)
				if (point->type[i] == CON_IO_PIN) {
					if (!tmp_owner)
						tmp_owner = point->patch[i];
					else {
						if (tmp_owner->order > point->patch[i]->order)
							tmp_owner = point->patch[i];
					}
				}
			point->owner = tmp_owner;
			point->position = INSERT_BEFORE_OWNER;
		} else {
			for (i = 0; i < MAX_CONN_PER_POINT; i++)
				if (point->type[i] == CON_IO_POUT) {
					if (!tmp_owner)
						tmp_owner = point->patch[i];
					else {
						if (tmp_owner->order < point->patch[i]->order)
							tmp_owner = point->patch[i];
					}
				}
			point->owner = tmp_owner;
			point->position = INSERT_AFTER_OWNER;
		}

		icount = 0;
		iarg = 0;
		iout = 0;
		for (i = 0; i < MAX_CONN_PER_POINT; i++) {
			if (point->out_gpr_idx[i] != 0) {
				if (iarg == 0) {
					point->out_instr[icount].used = 1;
					point->out_instr[icount].modified = 1;
					point->out_instr[icount].op_code = iACC3;
					point->out_instr[icount].arg[0] = point->con_gpr_idx;
					iarg++;
					if (iout >= 3) {
						point->out_instr[icount].arg[1] = point->con_gpr_idx;
						iarg++;
					}
				}
				point->out_instr[icount].arg[iarg++] = point->out_gpr_idx[i];

				iout++;
				if (iarg > 3) {
					icount++;
					iarg = 0;
				}
			}
		}

		if (iarg > 0/* && iarg <= 3*/)
			for (i = iarg; i < 4; i++)
				point->out_instr[icount].arg[i] = EMU10K1_REG_HW(0);
	} else {
		point->owner = NULL;
		point->position = INSERT_BEFORE_OWNER;
	}
}

int ld10k1_conn_point_set_to(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_conn_point_t *point, int type, int io)
{
	int reserved_tmp;
	int reserved_tmp_count = 0;
	unsigned int reserved;

	switch (type) {
		case CON_IO_FX:
			dsp_mgr->fxs[io].point = point;
			point->con_gpr_idx = EMU10K1_REG_FX(io);
			break;
		case CON_IO_IN:
			dsp_mgr->ins[io].point = point;
			point->con_gpr_idx = EMU10K1_REG_IN(io);
			break;
		case CON_IO_OUT:
			dsp_mgr->outs[io].point = point;
			point->con_gpr_idx = EMU10K1_REG_OUT(io);
			break;
		default:
			reserved = ld10k1_gpr_reserve(dsp_mgr, 1, &reserved_tmp_count, &reserved_tmp, GPR_USAGE_NORMAL, 0);
			if (!reserved)
				return LD10K1_ERR_NOT_FREE_REG;
			ld10k1_gpr_alloc(dsp_mgr, reserved);
			point->con_gpr_idx = reserved;
	}

	/* FIXME - aktualizacia instrukcii */


	ld10k1_conn_point_add_to_list(dsp_mgr, point);

	return 0;
}

void ld10k1_conn_point_unset(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_conn_point_t *point)
{
	int index = point->con_gpr_idx & ~EMU10K1_REG_TYPE_MASK;
	switch (EMU10K1_REG_TYPE_B(point->con_gpr_idx)) {
		case EMU10K1_REG_TYPE_FX:
			dsp_mgr->fxs[index].point = NULL;
			point->con_gpr_idx = 0;
			break;
		case EMU10K1_REG_TYPE_INPUT:
			dsp_mgr->ins[index].point = NULL;
			point->con_gpr_idx = 0;
			break;
		case EMU10K1_REG_TYPE_OUTPUT:
			dsp_mgr->outs[index].point = NULL;
			point->con_gpr_idx = 0;
			break;
		default:
			ld10k1_gpr_free(dsp_mgr, point->con_gpr_idx);
			point->con_gpr_idx = 0;
	}

	ld10k1_conn_point_del_from_list(dsp_mgr, point);

	/* FIXME - aktualizacia instrukcii */
}

int ld10k1_conn_point_add(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_conn_point_t *point, int type, ld10k1_patch_t *patch, int io)
{
	int i, j;
	int poutcount;
	int allocgprcount = 0;
	int allocinstrcount = 0;
	unsigned int reserved[2];
	unsigned int res[2];
	int reservedcount = 0;
	int usedreserved = 0;

	if (point->con_count >= MAX_CONN_PER_POINT)
		return LD10K1_ERR_MAX_CON_PER_POINT;

	/* check pout count */
	if (!point->simple && type == CON_IO_POUT) {
		poutcount = 0;
		for (i = 0; i < MAX_CONN_PER_POINT; i++)
			if (point->type[i] == CON_IO_POUT)
				poutcount++;

		if (poutcount > 0) {
			if (poutcount < 2) {
				allocgprcount = 2;
				allocinstrcount = 1;
			} else if (poutcount >= 2) {
				allocgprcount = 1;
				if ((poutcount - 3) % 2 == 0) {
					allocinstrcount = 1;
				}
			}

			/* allocate instr */
			if (dsp_mgr->instr_free < allocinstrcount)
				return LD10K1_ERR_NOT_FREE_INSTR;

			/* allocate gpr */
			for (i = 0; i < allocgprcount; i++) {
				reserved[i] = ld10k1_gpr_reserve(dsp_mgr, 2, &reservedcount, res, GPR_USAGE_NORMAL, 0);
				if (!reserved[i])
					return LD10K1_ERR_NOT_FREE_REG;
			}

			for (i = 0; i < allocgprcount; i++)
				ld10k1_gpr_alloc(dsp_mgr, reserved[i]);
		}
	}

	for (i = 0; i < MAX_CONN_PER_POINT; i++)
		if (point->type[i] == 0) {
			point->type[i] = type;
			point->patch[i] = patch;
			point->io[i] = io;
			point->out_gpr_idx[i] = 0;

			if (type == CON_IO_PIN) {
				if (patch->ins[io].point)
					ld10k1_conn_point_del(dsp_mgr, patch->ins[io].point, CON_IO_PIN, patch, io);
				patch->ins[io].point = point;
				ld10k1_dsp_mgr_actualize_instr_for_reg(dsp_mgr, patch, EMU10K1_PREG_IN(io));
			} else {
				if (patch->outs[io].point)
					ld10k1_conn_point_del(dsp_mgr, patch->outs[io].point, CON_IO_POUT, patch, io);
				patch->outs[io].point = point;

				if (point->simple)
					ld10k1_dsp_mgr_actualize_instr_for_reg(dsp_mgr, patch, EMU10K1_PREG_OUT(io));
				else {
					/* i is not used anymore */
					/* for all CON_IO_POUT where isn't reg add reserved */
					if (reservedcount > 0) {
						for (j = 0; j < MAX_CONN_PER_POINT; j++) {
							if (point->type[j] == CON_IO_POUT && point->out_gpr_idx[j] == 0) {
								point->out_gpr_idx[j] = reserved[usedreserved++];
								ld10k1_dsp_mgr_actualize_instr_for_reg(dsp_mgr, point->patch[j], EMU10K1_PREG_OUT(point->io[j]));
								if (usedreserved >= reservedcount)
									break;
							}
						}
					} else
						ld10k1_dsp_mgr_actualize_instr_for_reg(dsp_mgr, patch, EMU10K1_PREG_OUT(io));

					dsp_mgr->instr_free -= allocinstrcount;
					point->reserved_gpr += reservedcount;
					point->reserved_instr += allocinstrcount;

					if (reservedcount > 0)
						ld10k1_point_actualize_owner(dsp_mgr, point);
				}
			}

			point->con_count++;

			return 0;
		}

	return LD10K1_ERR_MAX_CON_PER_POINT;
}

int ld10k1_conn_point_del(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_conn_point_t *point, int type, ld10k1_patch_t *patch, int io)
{
	int i, j;
	int poutcount;
	int inc, outc;

	for (i = 0; i < MAX_CONN_PER_POINT; i++)
		if (point->type[i] == type && point->patch[i] == patch && point->io[i] == io) {
			point->type[i] = 0;
			point->patch[i] = NULL;
			point->io[i] = -1;
			if (type == CON_IO_PIN) {
				patch->ins[io].point = NULL;
				ld10k1_dsp_mgr_actualize_instr_for_reg(dsp_mgr, patch, EMU10K1_PREG_IN(io));
			} else {
				patch->outs[io].point = NULL;
				if (!point->simple && point->out_gpr_idx[i]) {
					ld10k1_gpr_free(dsp_mgr, point->out_gpr_idx[i]);
					point->reserved_gpr--;
					point->out_gpr_idx[i] = 0;
				}
				ld10k1_dsp_mgr_actualize_instr_for_reg(dsp_mgr, patch, EMU10K1_PREG_OUT(io));

				if (!point->simple) {
					/* get pout count */
					poutcount = 0;
					for (i = 0; i < MAX_CONN_PER_POINT; i++)
						if (point->type[i] == CON_IO_POUT)
							poutcount++;

					if (poutcount > 0) {
						if (poutcount < 2) {
							/* free all gpr and instr */
							for (j = 0; j < MAX_CONN_PER_POINT; j++)
								if (point->type[j] == CON_IO_POUT && point->out_gpr_idx[j]) {
									ld10k1_dsp_mgr_actualize_instr_for_reg(dsp_mgr, point->patch[j], EMU10K1_PREG_OUT(point->io[j]));
									ld10k1_gpr_free(dsp_mgr, point->out_gpr_idx[j]);
									point->reserved_gpr--;
									point->out_gpr_idx[j] = 0;
								}
							dsp_mgr->instr_free += point->reserved_instr;
							point->reserved_instr -= point->reserved_instr;
						} else if ((poutcount - 3) % 2 == 0) {
							/* free 1 instruction */
							dsp_mgr->instr_free += 1;
							point->reserved_instr -= 1;
						}
					}
					ld10k1_point_actualize_owner(dsp_mgr, point);
				}
			}
			point->con_count--;
			
			/* check in out count */
			outc = 0;
			inc = 0;
			for (j = 0; j < MAX_CONN_PER_POINT; j++) {
				if (point->type[j] == CON_IO_POUT)
					outc++;
				else if (point->type[j] == CON_IO_PIN)
					inc++;
			}

			if ((EMU10K1_REG_TYPE_B(point->con_gpr_idx) == EMU10K1_REG_TYPE_NORMAL && point->con_count <= 1) ||
				point->con_count <= 0 ||
				((outc <= 0 || inc <= 0) && EMU10K1_REG_TYPE_B(point->con_gpr_idx) == EMU10K1_REG_TYPE_NORMAL))
				ld10k1_conn_point_free(dsp_mgr, point);

			return 0;
		}
	return 0;
}

int ld10k1_conn_point_get_reg(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_conn_point_t *point, int type, ld10k1_patch_t *patch, int io)
{
	unsigned int reg = 0;
	int i;

	for (i = 0; i < MAX_CONN_PER_POINT; i++) {
		if (point->patch[i] == patch &&
			point->type[i] == type &&
			point->io[i] == io) {
			reg = point->out_gpr_idx[i];
			break;
		}
	}
	if (reg == 0)
		reg = point->con_gpr_idx;
	return reg;
}

int ld10k1_gen_patch_id(ld10k1_dsp_mgr_t *dsp_mgr, int pnum)
{
	int nid = 0;

	nid = dsp_mgr->patch_id_gens[pnum]++ | pnum << 16;
	return nid;
}

void ld10k1_conn_point_add_to_list(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_conn_point_t *point)
{
	if (dsp_mgr->point_list)
		point->next = dsp_mgr->point_list;
	else
		point->next = NULL;
	dsp_mgr->point_list = point;
}

void ld10k1_conn_point_del_from_list(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_conn_point_t *point)
{
	ld10k1_conn_point_t *tmp = dsp_mgr->point_list;

	if (tmp == point) {
		dsp_mgr->point_list = point->next;
		point->next = NULL;
		return;
	}

	while (tmp) {
		if (tmp->next == point) {
			tmp->next = point->next;
			point->next = NULL;
			return;
		}
		tmp = tmp->next;
	}
}
