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
 
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <alsa/asoundlib.h>

#include "ld10k1.h"
#include "ld10k1_dump_file.h"
#include "ld10k1_dump.h"
#include "ld10k1_error.h"

int ld10k1_make_dump(ld10k1_dsp_mgr_t *dsp_mgr, void **dump, int *size)
{
	int dump_size = 0;
	void *dump_file = NULL;
	void *ptr = NULL;
	ld10k1_dump_t *header = NULL;
	ld10k1_ctl_dump_t *ctl = NULL;
	int i, j;
	ld10k1_ctl_list_item_t *item;
	unsigned int *ival = NULL;
	ld10k1_tram_dump_t *tram = NULL;
	ld10k1_instr_dump_t *instr = NULL;

	dump_size += sizeof(ld10k1_dump_t);
	dump_size += sizeof(ld10k1_ctl_dump_t) * dsp_mgr->ctl_list_count;
	dump_size += sizeof(unsigned int) * dsp_mgr->regs_max_count;
	dump_size += sizeof(ld10k1_tram_dump_t) * (dsp_mgr->max_itram_hwacc + dsp_mgr->max_etram_hwacc);
	dump_size += sizeof(ld10k1_instr_dump_t) * dsp_mgr->instr_count;

	dump_file = malloc(dump_size);
	if (!dump_file)
		return LD10K1_ERR_NO_MEM;

	ptr = dump_file;
	header = (ld10k1_dump_t *)ptr;
	strcpy(header->signature, "LD10K1 DUMP 001");
	if (!dsp_mgr->audigy)
		header->dump_type = DUMP_TYPE_LIVE;
	else
		header->dump_type = DUMP_TYPE_AUDIGY;
	
	header->tram_size = dsp_mgr->e_tram.size;
	header->ctl_count = dsp_mgr->ctl_list_count;
	header->gpr_count = dsp_mgr->regs_max_count;
	header->tram_count = dsp_mgr->max_itram_hwacc + dsp_mgr->max_etram_hwacc;
	header->instr_count = dsp_mgr->instr_count;
	
	/*printf("Size header%d\n", dump_size);
	printf("Size header%d\nctc %d %d\ngpr %d %d\ntram %d %d\ninstr %d %d\n", sizeof(ld10k1_dump_t),
		header->ctl_count, sizeof(ld10k1_ctl_dump_t),
		header->gpr_count, sizeof(unsigned int),
		header->tram_count, sizeof(ld10k1_tram_dump_t),
		header->instr_count, sizeof(ld10k1_instr_dump_t));*/

	ptr += sizeof(ld10k1_dump_t);
	/* ctls */
	for (item = dsp_mgr->ctl_list; item != NULL; item = item->next) {
		ctl = (ld10k1_ctl_dump_t *)ptr;
		strcpy(ctl->name, item->ctl.name);
		ctl->index = item->ctl.index;
		ctl->vcount = item->ctl.vcount;
		ctl->count = item->ctl.count;
		for (j = 0; j < 32; j++) {
			ctl->gpr_idx[j] = item->ctl.gpr_idx[j];
   			ctl->value[j] = item->ctl.value[j];
		}
		ctl->min = item->ctl.min;
		ctl->max = item->ctl.max;
		ctl->translation = item->ctl.translation;

		ptr += sizeof(ld10k1_ctl_dump_t);
	}

	/* regs */
	for (i = 0; i < dsp_mgr->regs_max_count; i++) {
		ival = (unsigned int *) ptr;
		*ival = dsp_mgr->regs[i].val;
		ptr += sizeof(unsigned int);
	}

	/* tram */
	for (i = 0; i < dsp_mgr->max_itram_hwacc; i++) {
		tram = (ld10k1_tram_dump_t *) ptr;
		if (dsp_mgr->itram_hwacc[i].used) {
			tram->type = dsp_mgr->itram_hwacc[i].op;
			tram->addr = dsp_mgr->itram_hwacc[i].addr_val;
			tram->data = dsp_mgr->itram_hwacc[i].data_val;
		} else {
			tram->type = 0;
			tram->addr = 0;
			tram->data = 0;
		}
		ptr += sizeof(ld10k1_tram_dump_t);
	}

	for (i = 0; i < dsp_mgr->max_etram_hwacc; i++) {
		tram = (ld10k1_tram_dump_t *) ptr;
		if (dsp_mgr->etram_hwacc[i].used) {
			if (dsp_mgr->etram_hwacc[i].op == TRAM_OP_READ)
				tram->type = DUMP_TRAM_READ;
			else if (dsp_mgr->etram_hwacc[i].op == TRAM_OP_WRITE)
				tram->type = DUMP_TRAM_WRITE;
			else
				tram->type = DUMP_TRAM_NULL;
			tram->addr = dsp_mgr->etram_hwacc[i].addr_val;
			tram->data = dsp_mgr->etram_hwacc[i].data_val;
		} else {
			tram->type = 0;
			tram->addr = 0;
			tram->data = 0;
		}
		ptr += sizeof(ld10k1_tram_dump_t);
	}

	/* instr */
	for (i = 0; i < dsp_mgr->instr_count; i++) {
		instr = (ld10k1_instr_dump_t *) ptr;
		instr->used = dsp_mgr->instr[i].used;
		instr->op = dsp_mgr->instr[i].op_code;
		instr->arg[0] = dsp_mgr->instr[i].arg[0];
		instr->arg[1] = dsp_mgr->instr[i].arg[1];
		instr->arg[2] = dsp_mgr->instr[i].arg[2];
		instr->arg[3] = dsp_mgr->instr[i].arg[3];
		ptr += sizeof(ld10k1_instr_dump_t);
	}

	*dump = dump_file;
	*size = dump_size;

	return 0;
}
