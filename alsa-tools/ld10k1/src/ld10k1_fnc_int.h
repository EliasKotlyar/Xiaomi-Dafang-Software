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
 
#ifndef __LD10K1_FNC_INT_H
#define __LD10K1_FNC_INT_H

int ld10k1_dsp_mgr_init(ld10k1_dsp_mgr_t *dsp_mgr);
void ld10k1_dsp_mgr_init_id_gen(ld10k1_dsp_mgr_t *dsp_mgr);
void ld10k1_dsp_mgr_free(ld10k1_dsp_mgr_t *dsp_mgr);

ld10k1_patch_t *ld10k1_dsp_mgr_patch_new(void);
void ld10k1_dsp_mgr_patch_free(ld10k1_patch_t *patch);
ld10k1_p_in_out_t *ld10k1_dsp_mgr_patch_in_new(ld10k1_patch_t *patch, unsigned int count);
ld10k1_p_in_out_t *ld10k1_dsp_mgr_patch_out_new(ld10k1_patch_t *patch, unsigned int count);
ld10k1_p_const_sta_t *ld10k1_dsp_mgr_patch_const_new(ld10k1_patch_t *patch, unsigned int count);
ld10k1_p_const_sta_t *ld10k1_dsp_mgr_patch_sta_new(ld10k1_patch_t *patch, unsigned int count);
ld10k1_p_dyn_t *ld10k1_dsp_mgr_patch_dyn_new(ld10k1_patch_t *patch, unsigned int count);
ld10k1_p_hw_t *ld10k1_dsp_mgr_patch_hw_new(ld10k1_patch_t *patch, unsigned int count);
ld10k1_p_tram_grp_t *ld10k1_dsp_mgr_patch_tram_new(ld10k1_patch_t *patch, unsigned int count);
ld10k1_p_tram_acc_t *ld10k1_dsp_mgr_patch_tram_acc_new(ld10k1_patch_t *patch, unsigned int count);
ld10k1_instr_t *ld10k1_dsp_mgr_patch_instr_new(ld10k1_patch_t *patch, unsigned int count);
ld10k1_ctl_t *ld10k1_dsp_mgr_patch_ctl_new(ld10k1_patch_t *patch, unsigned int count);
char *ld10k1_dsp_mgr_name_new(char **where, const char *from);

int ld10k1_dsp_mgr_patch_load(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_patch_t *patch, int before, int *loaded);
int ld10k1_patch_fnc_check_patch(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_patch_t *new_patch);
int ld10k1_patch_fnc_del(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_fnc_patch_del_t *patch_fnc);
int ld10k1_connection_fnc(ld10k1_dsp_mgr_t *dsp_mgr, ld10k1_fnc_connection_t *connection_fnc, int *conn_id);

void ld10k1_del_control_from_list(ld10k1_ctl_list_item_t **list, int *count, ld10k1_ctl_t *gctl);
void ld10k1_del_all_controls_from_list(ld10k1_ctl_list_item_t **list, int *count);
int ld10k1_add_control_to_list(ld10k1_ctl_list_item_t **list, int *count, ld10k1_ctl_t *gctl);

#endif /* __LD10K1_FNC_INT_H */
