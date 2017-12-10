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
 
#ifndef __LD10K1_TRAM_H
#define __LD10K1_TRAM_H

typedef struct {
	int grp_idx;
	unsigned int grp_size;
	unsigned int grp_acc_count;
	unsigned int res_value;
	unsigned int pos;
} ld10k1_dsp_tram_resolve_item_t;

typedef struct {
	int isize;
	int ifree;
	int iacc_count;
	int iacc_free_count;
	int esize;
	int efree;
	int eacc_count;
	int eacc_free_count;
	int grp_free;
	int item_count;
	ld10k1_dsp_tram_resolve_item_t items[MAX_TRAM_COUNT];
} ld10k1_dsp_tram_resolve_t;

int ld10k1_tram_reserve_for_patch(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_patch_t *patch, ld10k1_dsp_tram_resolve_t *res);
int ld10k1_tram_alloc_for_patch(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_patch_t *patch, ld10k1_dsp_tram_resolve_t *res);
int ld10k1_tram_actualize_tram_for_patch(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_patch_t *patch);
void ld10k1_tram_get_hwacc(ld10k1_dsp_mgr_t *dsp_mgr, int acc, unsigned int *addr, unsigned int *data);
int ld10k1_tram_free_tram_for_patch(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_patch_t *patch);

#endif /* __LD10K1_TRAM_H */
