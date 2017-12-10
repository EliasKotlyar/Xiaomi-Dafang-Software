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
#include "ld10k1_mixer.h"
#include "ld10k1_error.h"

int ld10k1_init_reserved_ctls(ld10k1_dsp_mgr_t *dsp_mgr, snd_ctl_t *ctlp)
{
	snd_ctl_elem_list_t *clist;
	int count;
	int i;
	const char *ctl_name;
	unsigned int ctl_index;
	
	ld10k1_reserved_ctl_list_item_t *res_ctl;
	
	dsp_mgr->reserved_ctl_list = NULL;
	
	snd_ctl_elem_list_alloca(&clist);
	
	if (snd_ctl_elem_list(ctlp, clist) < 0)
		return LD10K1_ERR_NO_MEM;
		
	if ((count = snd_ctl_elem_list_get_count(clist)) < 0)
		return LD10K1_ERR_NO_MEM;
		
	snd_ctl_elem_list_set_offset(clist, 0);
	
	if (snd_ctl_elem_list_alloc_space(clist, count) < 0)
		return LD10K1_ERR_NO_MEM;
		
	if (snd_ctl_elem_list(ctlp, clist) < 0) {
		snd_ctl_elem_list_free_space(clist);
		return LD10K1_ERR_NO_MEM;
	}
		
	for (i = 0; i < count; i++) {
		snd_ctl_elem_id_t *id;
		snd_ctl_elem_id_alloca(&id);
		snd_ctl_elem_list_get_id(clist, i, id);
		
		ctl_name = snd_ctl_elem_id_get_name(id);
		ctl_index = snd_ctl_elem_id_get_index(id);
		
		res_ctl = (ld10k1_reserved_ctl_list_item_t *)malloc(sizeof(ld10k1_reserved_ctl_list_item_t));
		if (!res_ctl) {
			snd_ctl_elem_list_free_space(clist);
			return LD10K1_ERR_NO_MEM;
		}
			
		res_ctl->next = dsp_mgr->reserved_ctl_list;
		dsp_mgr->reserved_ctl_list = res_ctl;
		strncpy(res_ctl->res_ctl.name, ctl_name, 43);
		res_ctl->res_ctl.name[43] = '\0';
		res_ctl->res_ctl.index = ctl_index;
	}
	
	snd_ctl_elem_list_free_space(clist);
	return 0;
}

int ld10k1_free_reserved_ctls(ld10k1_dsp_mgr_t *dsp_mgr)
{
	ld10k1_reserved_ctl_list_item_t *item;
	ld10k1_reserved_ctl_list_item_t *item1;
	
	for (item = dsp_mgr->reserved_ctl_list; item != NULL;) {
		item1 = item->next;
		free(item);
		item = item1;
	}
	
	dsp_mgr->reserved_ctl_list = NULL;
	return 0;
}

