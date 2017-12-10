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

#include "ld10k1.h"
#include "ld10k1_fnc.h"
#include "ld10k1_tram.h"
#include "ld10k1_error.h"
#include <stdlib.h>

int ld10k1_tram_res_alloc_hwacc(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_dsp_tram_resolve_t *res);
int ld10k1_tram_realloc_space(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_dsp_tram_resolve_t *res);

void ld10k1_tram_init_res(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_dsp_tram_resolve_t *res)
{
	res->isize = dsp_mgr->i_tram.size;
	res->ifree = dsp_mgr->i_tram.size;
	res->iacc_count = dsp_mgr->i_tram.max_hwacc;
	res->iacc_free_count = dsp_mgr->i_tram.max_hwacc;

	res->esize = dsp_mgr->e_tram.size;
	res->efree = dsp_mgr->e_tram.size;
	res->eacc_count = dsp_mgr->e_tram.max_hwacc;
	res->eacc_free_count = dsp_mgr->e_tram.max_hwacc;

	res->grp_free = res->iacc_free_count + res->eacc_free_count;

	res->item_count = 0;
}

void ld10k1_tram_init_res_from_dsp_mgr(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_dsp_tram_resolve_t *res)
{
	/* throught all groups */
	int i;
	for (i = 0; i < dsp_mgr->max_tram_grp; i++) {
		if (dsp_mgr->tram_grp[i].used) {
			/* get position */
			res->grp_free--;
			switch (dsp_mgr->tram_grp[i].req_pos) {
				case TRAM_POS_NONE:
				case TRAM_POS_AUTO:
					/* add to res */
					res->items[res->item_count].grp_idx = i;
					res->items[res->item_count].grp_size = dsp_mgr->tram_grp[i].size;
					res->items[res->item_count].grp_acc_count = dsp_mgr->tram_grp[i].acc_count;
					res->items[res->item_count].res_value = 0;
					res->items[res->item_count].pos = TRAM_POS_NONE;
					res->item_count++;
					break;
				case TRAM_POS_INTERNAL:
					/* decrease resources */
					res->ifree -= dsp_mgr->tram_grp[i].size;
					res->iacc_free_count -= dsp_mgr->tram_grp[i].acc_count;
					break;
				case TRAM_POS_EXTERNAL:
					res->efree -= dsp_mgr->tram_grp[i].size;
					res->eacc_free_count -= dsp_mgr->tram_grp[i].acc_count;
					break;
			}
		}
	}
}

int ld10k1_tram_acc_count_from_patch(ld10k1_patch_t *patch, int grp)
{
	int i, count;

	for (count = 0, i = 0; i < patch->tram_acc_count; i++)
		if (patch->tram_acc[i].grp == grp)
			count++;

	return count;
}

int ld10k1_tram_init_res_from_patch(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_dsp_tram_resolve_t *res, ld10k1_patch_t *patch)
{
	int i;
	int acc_count;

	/* through all groups */
	for (i = 0; i < patch->tram_count; i++) {
		if (res->grp_free <= 0)
			return LD10K1_ERR_TRAM_FULL_GRP;
		/* get acc count */
		acc_count = ld10k1_tram_acc_count_from_patch(patch, i);
		if (acc_count <= 0)
			continue;
			/* get position */
		switch (patch->tram_grp[i].grp_pos) {
			case TRAM_POS_NONE:
			case TRAM_POS_AUTO:
				/* add to res */
				res->items[res->item_count].grp_idx = -i - 1;
				res->items[res->item_count].grp_size = patch->tram_grp[i].grp_size;
				res->items[res->item_count].grp_acc_count = acc_count;
				res->items[res->item_count].res_value = 0;
				res->items[res->item_count].pos = TRAM_POS_NONE;
				res->item_count++;
				break;
			case TRAM_POS_INTERNAL:
				/* decrease resources */
				if (res->ifree < patch->tram_grp[i].grp_size)
					return LD10K1_ERR_ITRAM_FULL;
				if (res->iacc_free_count < acc_count)
					return LD10K1_ERR_ITRAM_FULL_ACC;
				res->ifree -= patch->tram_grp[i].grp_size;
				res->iacc_free_count -= acc_count;
				break;
			case TRAM_POS_EXTERNAL:
				/* decrease resources */
				if (res->efree < patch->tram_grp[i].grp_size)
					return LD10K1_ERR_ETRAM_FULL;
				if (res->eacc_free_count < acc_count)
					return LD10K1_ERR_ETRAM_FULL_ACC;
				res->efree -= patch->tram_grp[i].grp_size;
				res->eacc_free_count -= acc_count;
				break;
		}
	}
	return 0;
}

int ld10k1_tram_init_res_from_patch_copy(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_dsp_tram_resolve_t *res, ld10k1_patch_t *patch)
{
	/* throught all groups */
	int i;
	int acc_count;

	for (i = 0; i < patch->tram_count; i++) {
		/* get acc count */
		acc_count = ld10k1_tram_acc_count_from_patch(patch, i);
			/* get position */
		if (patch->tram_grp[i].grp_pos == TRAM_POS_INTERNAL ||
			patch->tram_grp[i].grp_pos == TRAM_POS_EXTERNAL) {

			res->items[res->item_count].grp_idx = -i - 1;
			res->items[res->item_count].grp_size = patch->tram_grp[i].grp_size;
			res->items[res->item_count].grp_acc_count = acc_count;
			res->items[res->item_count].pos = patch->tram_grp[i].grp_pos;
			res->items[res->item_count].res_value = 0;
			res->item_count++;
		}
	}
	return 0;
}

int ld10k1_tram_calc_res_value(ld10k1_dsp_tram_resolve_t *res)
{
	/* res_value is calculated as grp_size / acc_count */
	int i;
	for (i = 0; i < res->item_count; i++)
		res->items[i].res_value = res->items[i].grp_size / res->items[i].grp_acc_count;
	return 0;
}

static int ld10k1_tram_sort_res_compare(const void *item1, const void *item2)
{
	ld10k1_dsp_tram_resolve_item_t *i1 = (ld10k1_dsp_tram_resolve_item_t *)item1;
	ld10k1_dsp_tram_resolve_item_t *i2 = (ld10k1_dsp_tram_resolve_item_t *)item2;

	if (i1->res_value == i2->res_value)
		return 0;
	else if (i1->res_value > i2->res_value)
		return 1;
	else
		return -1;
}

int ld10k1_tram_sort_res(ld10k1_dsp_tram_resolve_t *res)
{
	qsort(res->items, res->item_count, sizeof(ld10k1_dsp_tram_resolve_item_t), ld10k1_tram_sort_res_compare);
	return 0;
}

int ld10k1_tram_resolve_res(ld10k1_dsp_tram_resolve_t *res)
{
	int i;
	for (i = 0; i < res->item_count; i++) {
		/* first try internal tram then external tram */
		if (res->items[i].grp_size <= res->ifree &&
			res->items[i].grp_acc_count <= res->iacc_free_count) {
			/* put it into itram */
			res->ifree -= res->items[i].grp_size;
			res->iacc_free_count -= res->items[i].grp_acc_count;
			res->items[i].pos = TRAM_POS_INTERNAL;
		} else if (res->items[i].grp_size <= res->efree &&
			res->items[i].grp_acc_count <= res->eacc_free_count) {
			/* put it into etram */
			res->efree -= res->items[i].grp_size;
			res->eacc_free_count -= res->items[i].grp_acc_count;
			res->items[i].pos = TRAM_POS_EXTERNAL;
		} else
			return LD10K1_ERR_TRAM_FULL;
	}
	return 0;
}

int ld10k1_tram_grp_alloc(ld10k1_dsp_mgr_t *dsp_mgr)
{
	int i;
	for (i = 0; i < dsp_mgr->max_tram_grp; i++) {
		if (!dsp_mgr->tram_grp[i].used) {
			dsp_mgr->tram_grp[i].used = 1;
			return i;
		}
	}
	return LD10K1_ERR_TRAM_FULL_GRP;
}

void ld10k1_tram_grp_free(ld10k1_dsp_mgr_t *dsp_mgr, int grp)
{
	dsp_mgr->tram_grp[grp].used = 0;
}

int ld10k1_tram_acc_alloc(ld10k1_dsp_mgr_t *dsp_mgr)
{
	int i;
	for (i = 0; i < dsp_mgr->max_tram_acc; i++) {
		if (!dsp_mgr->tram_acc[i].used) {
			dsp_mgr->tram_acc[i].used = 1;
			return i;
		}
	}
	return LD10K1_ERR_TRAM_FULL_ACC;
}

void ld10k1_tram_acc_free(ld10k1_dsp_mgr_t *dsp_mgr, int acc)
{
	dsp_mgr->tram_acc[acc].used = 0;
}

int ld10k1_tram_reserve_for_patch(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_patch_t *patch, ld10k1_dsp_tram_resolve_t *res)
{
	int err;

	ld10k1_tram_init_res(dsp_mgr, res);
	ld10k1_tram_init_res_from_dsp_mgr(dsp_mgr, res);

	if ((err = ld10k1_tram_init_res_from_patch(dsp_mgr, res, patch)) < 0)
		return err;

	ld10k1_tram_calc_res_value(res);
	ld10k1_tram_sort_res(res);

	if ((err = ld10k1_tram_resolve_res(res)) < 0)
		return err;

	ld10k1_tram_init_res_from_patch_copy(dsp_mgr, res, patch);

	return 0;
}

int ld10k1_tram_alloc_for_patch(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_patch_t *patch, ld10k1_dsp_tram_resolve_t *res)
{
	int i;
	int grp;
	int acc;

	/* allocate tram grp and acc for patch */
	for (i = 0; i < patch->tram_count; i++) {
		grp = ld10k1_tram_grp_alloc(dsp_mgr);
		patch->tram_grp[i].grp_idx = grp;
		dsp_mgr->tram_grp[grp].type = patch->tram_grp[i].grp_type;
		dsp_mgr->tram_grp[grp].size = patch->tram_grp[i].grp_size;
	}

	for (i = 0; i < res->item_count; i++) {
		if (res->items[i].grp_idx < 0) {
			res->items[i].grp_idx = patch->tram_grp[-(res->items[i].grp_idx + 1)].grp_idx;
			dsp_mgr->tram_grp[res->items[i].grp_idx].pos = TRAM_POS_NONE;
			dsp_mgr->tram_grp[res->items[i].grp_idx].acc_count = res->items[i].grp_acc_count;
		}
	}

	for (i = 0; i < patch->tram_acc_count; i++) {
		acc = ld10k1_tram_acc_alloc(dsp_mgr);
		patch->tram_acc[i].acc_idx = acc;
		dsp_mgr->tram_acc[acc].type = patch->tram_acc[i].acc_type;
		dsp_mgr->tram_acc[acc].offset = patch->tram_acc[i].acc_offset;
		dsp_mgr->tram_acc[acc].grp = patch->tram_grp[patch->tram_acc[i].grp].grp_idx;
	}

	ld10k1_tram_res_alloc_hwacc(dsp_mgr, res);
	ld10k1_tram_realloc_space(dsp_mgr, res);
	return 0;
}

int ld10k1_tram_hwacc_alloc(ld10k1_dsp_mgr_t *dsp_mgr, int external)
{
	int i;

	if (!external) {
		for (i = 0; i < dsp_mgr->max_itram_hwacc; i++) {
			if (!dsp_mgr->itram_hwacc[i].used) {
				dsp_mgr->itram_hwacc[i].used = 1;
				dsp_mgr->i_tram.used_hwacc++;
				return i;
			}
		}
	} else {
		for (i = 0; i < dsp_mgr->max_etram_hwacc; i++) {
			if (!dsp_mgr->etram_hwacc[i].used) {
				dsp_mgr->etram_hwacc[i].used = 1;
				dsp_mgr->e_tram.used_hwacc++;
				return i + dsp_mgr->max_itram_hwacc;
			}
		}
	}
	return LD10K1_ERR_TRAM_FULL_ACC;
}

void ld10k1_tram_hwacc_free(ld10k1_dsp_mgr_t *dsp_mgr, int acc)
{
	if (acc < dsp_mgr->max_itram_hwacc) {
		dsp_mgr->itram_hwacc[acc].used = 0;
		dsp_mgr->itram_hwacc[acc].addr_val = 0;
		dsp_mgr->itram_hwacc[acc].data_val = 0;
		dsp_mgr->itram_hwacc[acc].modified = 1;
		dsp_mgr->i_tram.used_hwacc--;
	} else {
		int nacc = acc - dsp_mgr->max_itram_hwacc;
		dsp_mgr->etram_hwacc[nacc].used = 0;
		dsp_mgr->etram_hwacc[nacc].used = 0;
		dsp_mgr->etram_hwacc[nacc].addr_val = 0;
		dsp_mgr->etram_hwacc[nacc].data_val = 0;
		dsp_mgr->etram_hwacc[nacc].modified = 1;
		dsp_mgr->e_tram.used_hwacc--;
	}
}

void ld10k1_tram_actualize_hwacc(ld10k1_dsp_mgr_t *dsp_mgr, int acc, unsigned int op, unsigned int addr, unsigned int data)
{
	if (acc < dsp_mgr->max_itram_hwacc) {
		dsp_mgr->itram_hwacc[acc].op = op;
		dsp_mgr->itram_hwacc[acc].addr_val = addr;
		dsp_mgr->itram_hwacc[acc].data_val = data;
		dsp_mgr->itram_hwacc[acc].modified = 1;
	} else {
		int nacc = acc - dsp_mgr->max_itram_hwacc;
		dsp_mgr->etram_hwacc[nacc].op = op;
		dsp_mgr->etram_hwacc[nacc].addr_val = addr;
		dsp_mgr->etram_hwacc[nacc].data_val = data;
		dsp_mgr->etram_hwacc[nacc].modified = 1;
	}
}

void ld10k1_tram_get_hwacc(ld10k1_dsp_mgr_t *dsp_mgr, int acc, unsigned int *addr, unsigned int *data)
{
	int nacc;

	if (acc < dsp_mgr->max_itram_hwacc) {
		*addr = dsp_mgr->itram_hwacc[acc].addr_val;
		*data = dsp_mgr->itram_hwacc[acc].data_val;
	} else {
		nacc = acc - dsp_mgr->max_itram_hwacc;
		*addr = dsp_mgr->etram_hwacc[nacc].addr_val;
		*data = dsp_mgr->etram_hwacc[nacc].data_val;
	}
}

int ld10k1_tram_res_alloc_hwacc(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_dsp_tram_resolve_t *res)
{
	int i, j;
	int grp_idx;
	int hwacc;

	/* free hw acc - where pos changed */
	for (i = 0; i < res->item_count; i++) {
		grp_idx = res->items[i].grp_idx;
		if (dsp_mgr->tram_grp[grp_idx].pos != TRAM_POS_NONE &&
			dsp_mgr->tram_grp[grp_idx].pos != res->items[i].pos) {
			for (j = 0; j < dsp_mgr->max_tram_acc; j++)
				if (dsp_mgr->tram_acc[j].used &&
					dsp_mgr->tram_acc[j].grp == grp_idx)
					ld10k1_tram_hwacc_free(dsp_mgr, dsp_mgr->tram_acc[j].hwacc);
			dsp_mgr->tram_grp[grp_idx].pos = TRAM_POS_NONE;
		}
	}

	/* now allocate */
	for (i = 0; i < res->item_count; i++) {
		grp_idx = res->items[i].grp_idx;
		if (dsp_mgr->tram_grp[grp_idx].pos == TRAM_POS_NONE &&
			dsp_mgr->tram_grp[grp_idx].pos != res->items[i].pos) {
			dsp_mgr->tram_grp[grp_idx].pos = res->items[i].pos;
			for (j = 0; j < dsp_mgr->max_tram_acc; j++)
				if (dsp_mgr->tram_acc[j].used &&
					dsp_mgr->tram_acc[j].grp == grp_idx) {
					hwacc = ld10k1_tram_hwacc_alloc(dsp_mgr, res->items[i].pos == TRAM_POS_EXTERNAL ? 1 : 0);
					dsp_mgr->tram_acc[j].hwacc = hwacc;
				}
		}
	}
	return 0;
}

int ld10k1_tram_realloc_space(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_dsp_tram_resolve_t *res)
{
	int itram_size = res->isize;
	int etram_size = res->esize;
	int i;

	/* allocate from end */
	for (i = 0; i < dsp_mgr->max_tram_grp; i++)
		if (dsp_mgr->tram_grp[i].used) {
			if (dsp_mgr->tram_grp[i].pos == TRAM_POS_INTERNAL) {
				itram_size -= dsp_mgr->tram_grp[i].size;
				dsp_mgr->tram_grp[i].offset = itram_size;
			} else if (dsp_mgr->tram_grp[i].pos == TRAM_POS_EXTERNAL) {
				etram_size -= dsp_mgr->tram_grp[i].size;
				dsp_mgr->tram_grp[i].offset = etram_size;
			}
		}
	
	return 0;
}


int ld10k1_tram_actualize_tram_for_patch(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_patch_t *patch)
{
	int i;
	int grp_idx;
	int acc_idx;
	int tram_op;

	/* for all patch accs */
	for (i = 0; i < patch->tram_acc_count; i++) {
		grp_idx = patch->tram_grp[patch->tram_acc[i].grp].grp_idx;
		acc_idx = patch->tram_acc[i].acc_idx;
		tram_op = 0;

		if (dsp_mgr->tram_acc[acc_idx].type == TRAM_ACC_WRITE)
			tram_op = TRAM_OP_WRITE;
		else
			tram_op = TRAM_OP_READ;

		ld10k1_tram_actualize_hwacc(dsp_mgr, dsp_mgr->tram_acc[acc_idx].hwacc,
			tram_op, dsp_mgr->tram_grp[grp_idx].offset + patch->tram_acc[i].acc_offset, 0);
	}
	return 0;
}

int ld10k1_tram_free_tram_for_patch(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_patch_t *patch)
{
	int i;
	int acc_idx;
	int grp_idx;
	
	/* free all patch accs */
	for (i = 0; i < patch->tram_acc_count; i++) {
		acc_idx = patch->tram_acc[i].acc_idx;
		ld10k1_tram_hwacc_free(dsp_mgr, dsp_mgr->tram_acc[acc_idx].hwacc);
		ld10k1_tram_acc_free(dsp_mgr, acc_idx);
	}

	/* free all patch grps */
	for (i = 0; i < patch->tram_count; i++) {
		grp_idx = patch->tram_grp[i].grp_idx;
		ld10k1_tram_grp_free(dsp_mgr, grp_idx);
	}
	
	return 0;
}

