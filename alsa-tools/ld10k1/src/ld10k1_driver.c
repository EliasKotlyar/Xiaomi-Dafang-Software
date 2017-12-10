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

#include <sys/ioctl.h>
#include <alsa/asoundlib.h>
#include <alsa/sound/emu10k1.h>
#include <stdint.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "bitops.h"
#include "ld10k1.h"
#include "ld10k1_driver.h"
#include "ld10k1_error.h"
#include "ld10k1_fnc.h"
#include "ld10k1_fnc_int.h"

//#define DEBUG_DRIVER 1

extern snd_hwdep_t *handle;

void ld10k1_syntetize_instr(int audigy, int op, int arg1, int arg2, int arg3, int arg4, unsigned int *out)
{
	if (audigy) {
		*out = ((arg3 & 0x7ff) << 12) | (arg4 & 0x7ff);
		*(out + 1) = ((op & 0x0f) << 24) | ((arg1 & 0x7ff) << 12) | (arg2 & 0x7ff);
	} else {
		*out = ((arg3 & 0x3ff) << 10) | (arg4 & 0x3ff);
		*(out + 1) = ((op & 0x0f) << 20) | ((arg1 & 0x3ff) << 10) | (arg2 & 0x3ff);
	}
}

void ld10k1_init_must_init_output(ld10k1_dsp_mgr_t *dsp_mgr);
void ld10k1_set_must_init_output(ld10k1_dsp_mgr_t *dsp_mgr, int reg);
void ld10k1_check_must_init_output(ld10k1_dsp_mgr_t *dsp_mgr, emu10k1_fx8010_code_t *code);

/* outputs what must be initialized on audigy */
static int audigy_must_init_output[] = {
	0x68, 0,
	0x69, 0,
	0x6a, 0,
	0x6b, 0,
	0x6e, 0,
	0x6f, 0,
	-1};

#define LD10K1_SIGNATURE "LD10K1 ver. " VERSION " managed DSP code"

void ld10k1_free_code_struct(emu10k1_fx8010_code_t *code)
{
	if (code->gpr_map)
		free(code->gpr_map);
	if (code->tram_data_map)
		free(code->tram_data_map);
	if (code->tram_addr_map)
		free(code->tram_addr_map);
	if (code->code)
		free(code->code);
}

int ld10k1_alloc_code_struct(emu10k1_fx8010_code_t *code)
{
	/* alloc code structure */
	code->gpr_map = NULL;
	code->tram_data_map = NULL;
	code->tram_addr_map = NULL;
	code->code = NULL;
	
	code->gpr_map = (uint32_t *)malloc(sizeof(uint32_t) * 0x200);
	if (!code->gpr_map)
		goto err;
	memset(code->gpr_map, 0, sizeof(uint32_t) * 0x200);

	code->tram_data_map = (uint32_t *)malloc(sizeof(uint32_t) * 0x100);
	if (!code->tram_data_map)
		goto err;
	memset(code->tram_data_map, 0, sizeof(uint32_t) * 0x100);

	code->tram_addr_map = (uint32_t *)malloc(sizeof(uint32_t) * 0x100);
	if (!code->tram_addr_map)
		goto err;
	memset(code->tram_addr_map, 0, sizeof(uint32_t) * 0x100);

	code->code = (uint32_t *)malloc(sizeof(uint32_t) * 1024 * 2);
	if (!code->code)
		goto err;
	memset(code->code, 0, sizeof(uint32_t) * 1024 * 2);

        return 0;
err:
	ld10k1_free_code_struct(code);
	return LD10K1_ERR_NO_MEM;
}

int ld10k1_update_driver(ld10k1_dsp_mgr_t *dsp_mgr)
{
	emu10k1_fx8010_code_t code;
	emu10k1_fx8010_control_gpr_t *add_ctrl;
	emu10k1_ctl_elem_id_t *del_ids;

	ld10k1_ctl_list_item_t *item;
	unsigned int i, j;
	int max;
	int modified;
	unsigned int addr;
	unsigned int vaddr;
	unsigned int op;
	unsigned int idx_offset;
	unsigned int *iptr;
	ld10k1_ctl_t gctl;
	
	int err;
	
	if ((err = ld10k1_alloc_code_struct(&code)) < 0)
    		return err;
	
	/* new name */
	strcpy(code.name, LD10K1_SIGNATURE);

	for (i = 0; i < sizeof(code.gpr_valid) / sizeof(unsigned long); i++)
		code.gpr_valid[i] = 0;
	for (i = 0; i < sizeof(code.tram_valid) / sizeof(unsigned long); i++)
		code.tram_valid[i] = 0;
	for (i = 0; i < sizeof(code.code_valid) / sizeof(unsigned long); i++)
		code.code_valid[i] = 0;

	/* registers */
	for (i = 0; i < dsp_mgr->regs_max_count; i++)
		if (dsp_mgr->regs[i].modified) {
			set_bit(i, code.gpr_valid);
			code.gpr_map[i] = dsp_mgr->regs[i].val;
		}

	/* tram addr + data */
	for (j = 0; j < 2; j++) {
		max = (j == 0 ? dsp_mgr->max_itram_hwacc : dsp_mgr->max_etram_hwacc);
		for (i = 0; i < max; i++) {
			modified = (j == 0 ? dsp_mgr->itram_hwacc[i].modified : dsp_mgr->etram_hwacc[i].modified);
			if (modified) {
				addr = (j == 0 ? dsp_mgr->itram_hwacc[i].addr_val : dsp_mgr->etram_hwacc[i].addr_val);
				vaddr = addr & 0xFFFFF;
				idx_offset = (j == 0 ? 0 : dsp_mgr->max_itram_hwacc);
				op = (j == 0 ? dsp_mgr->itram_hwacc[i].op : dsp_mgr->etram_hwacc[i].op);

				set_bit(i + idx_offset, code.tram_valid);
				switch(op) {
					case TRAM_OP_READ:
						if (dsp_mgr->audigy)
							vaddr = vaddr | 0x2 << 20;
						else
							vaddr = vaddr | TANKMEMADDRREG_READ | TANKMEMADDRREG_ALIGN;
						break;
					case TRAM_OP_WRITE:
						if (dsp_mgr->audigy)
							vaddr = vaddr | 0x6 << 20;
						else
							vaddr = vaddr | TANKMEMADDRREG_WRITE | TANKMEMADDRREG_ALIGN;
						break;
					case TRAM_OP_NULL:
					default:
						vaddr = 0;
						break;
				}

				code.tram_addr_map[i + idx_offset] = vaddr;
				code.tram_data_map[i + idx_offset] = (j == 0 ? dsp_mgr->itram_hwacc[i].data_val : dsp_mgr->etram_hwacc[i].data_val);
			}
		}
	}

	/* controls to add */
	if (dsp_mgr->add_list_count > 0) {
		add_ctrl = calloc(dsp_mgr->add_list_count,
				  sizeof(emu10k1_fx8010_control_gpr_t));
		if (!add_ctrl)
			return LD10K1_ERR_NO_MEM;
		for (i = 0, item = dsp_mgr->add_ctl_list; item != NULL; item = item->next, i++) {
			strcpy(add_ctrl[i].id.name, item->ctl.name);
			add_ctrl[i].id.iface = EMU10K1_CTL_ELEM_IFACE_MIXER;
			add_ctrl[i].id.index = item->ctl.index;
			add_ctrl[i].vcount = item->ctl.vcount;
			add_ctrl[i].count = item->ctl.count;
			for (j = 0; j < 32; j++) {
				add_ctrl[i].gpr[j] = item->ctl.gpr_idx[j];
				add_ctrl[i].value[j] = item->ctl.value[j];
			}
			add_ctrl[i].min = item->ctl.min;
			add_ctrl[i].max = item->ctl.max;
			add_ctrl[i].translation = item->ctl.translation;
		}
	} else
		add_ctrl = NULL;

	code.gpr_add_control_count = dsp_mgr->add_list_count;
	code.gpr_add_controls = add_ctrl;

	/* controls to del */
	if (dsp_mgr->del_list_count > 0) {
		del_ids = calloc(dsp_mgr->del_list_count,
				 sizeof(emu10k1_ctl_elem_id_t));
		if (!del_ids)
			return LD10K1_ERR_NO_MEM;
		for (i = 0, item = dsp_mgr->del_ctl_list; item != NULL; item = item->next, i++) {
			strcpy(del_ids[i].name, item->ctl.name);
			del_ids[i].iface = EMU10K1_CTL_ELEM_IFACE_MIXER;
			del_ids[i].index = item->ctl.index;
		}
	} else
		del_ids = NULL;
		
	code.gpr_del_control_count = dsp_mgr->del_list_count;
	code.gpr_del_controls = del_ids;

	code.gpr_list_control_count = 0;

	for (iptr = code.code, i = 0; i < dsp_mgr->instr_count; i++, iptr += 2) {
		if (dsp_mgr->instr[i].modified) {
			set_bit(i, code.code_valid);
			if (dsp_mgr->instr[i].used) {
				if (dsp_mgr->audigy) {
					ld10k1_syntetize_instr(dsp_mgr->audigy,
						dsp_mgr->instr[i].op_code,
						dsp_mgr->instr[i].arg[0], dsp_mgr->instr[i].arg[1], dsp_mgr->instr[i].arg[2], dsp_mgr->instr[i].arg[3], iptr);
				} else {
					if (i < 0x200) {
						ld10k1_syntetize_instr(dsp_mgr->audigy,
							dsp_mgr->instr[i].op_code,
							dsp_mgr->instr[i].arg[0], dsp_mgr->instr[i].arg[1], dsp_mgr->instr[i].arg[2], dsp_mgr->instr[i].arg[3], iptr);
					}
				}
			} else {
				if (dsp_mgr->audigy) {
					ld10k1_syntetize_instr(dsp_mgr->audigy,
						0x0f,
						0xc0, 0xc0, 0xcf, 0xc0, iptr);
				} else {
					if (i < 0x200) {
						ld10k1_syntetize_instr(dsp_mgr->audigy,
							0x06,
							0x40, 0x40, 0x40, 0x40, iptr);
					}
				}
			}
		}
	}
	
	/* check initialization of i2s outputs on audigy */
	if (dsp_mgr->audigy)
		ld10k1_check_must_init_output(dsp_mgr, &code);


#ifndef DEBUG_DRIVER
	if (snd_hwdep_ioctl(handle, SNDRV_EMU10K1_IOCTL_CODE_POKE, &code) < 0) {
		error("unable to poke code");
		ld10k1_free_code_struct(&code);
		if (add_ctrl)
			free(add_ctrl);
		if (del_ids)
			free(del_ids);
		return LD10K1_ERR_DRIVER_CODE_POKE;
	}
#endif

	/* update state */
	for (item = dsp_mgr->del_ctl_list; item != NULL; item = item->next) {
		strcpy(gctl.name, item->ctl.name);
		ld10k1_del_control_from_list(&(dsp_mgr->ctl_list), &(dsp_mgr->ctl_list_count), &gctl);
	}

	ld10k1_del_all_controls_from_list(&(dsp_mgr->del_ctl_list), &dsp_mgr->del_list_count);

	for (item = dsp_mgr->add_ctl_list; item != NULL; item = item->next)
		ld10k1_add_control_to_list(&(dsp_mgr->ctl_list), &(dsp_mgr->ctl_list_count), &(item->ctl));

	ld10k1_del_all_controls_from_list(&(dsp_mgr->add_ctl_list), &dsp_mgr->add_list_count);

	for (i = 0; i < dsp_mgr->regs_max_count; i++)
		dsp_mgr->regs[i].modified = 0;

	for (i = 0; i < dsp_mgr->instr_count; i++)
		dsp_mgr->instr[i].modified = 0;

	for (j = 0; j < 2; j++) {
		max = (j == 0 ? dsp_mgr->max_itram_hwacc : dsp_mgr->max_etram_hwacc);
		for (i = 0; i < max; i++) {
			if (j == 0)
				dsp_mgr->itram_hwacc[i].modified = 0;
			else
				dsp_mgr->etram_hwacc[i].modified = 0;
		}
	}
	
	ld10k1_free_code_struct(&code);

	if (add_ctrl)
		free(add_ctrl);
	if (del_ids)
		free(del_ids);
	return 0;
}


int ld10k1_init_driver(ld10k1_dsp_mgr_t *dsp_mgr, int tram_size)
{
	emu10k1_fx8010_info_t info;
	int i;
	emu10k1_fx8010_code_t code;
	emu10k1_fx8010_control_gpr_t *ctrl;
	emu10k1_ctl_elem_id_t *ids;
	emu10k1_fx8010_pcm_t ipcm;
	
	unsigned int *iptr;
	
	int err;
	
	if (snd_hwdep_ioctl(handle, SNDRV_EMU10K1_IOCTL_PVERSION, &i) < 0) {
		error("Cannot get emu10k1 driver version, likely an old driver is running.");
		return LD10K1_ERR_DRIVER_INFO;
	}

	if ((err = ld10k1_alloc_code_struct(&code)) < 0)
    		return err;
	
	/* setup tram size */
	if (tram_size >= 0 && snd_hwdep_ioctl(handle, SNDRV_EMU10K1_IOCTL_TRAM_SETUP, &tram_size) < 0) {
		error("unable to setup tram");
		if (dsp_mgr->audigy)
			error("You are probably user of audigy, audigy 2 and you not aplyed patch to enable tram");
		/* this is not fatal, but do not use tram */
		dsp_mgr->i_tram.size = 0;
		dsp_mgr->e_tram.size = 0;
	} else {
		if (snd_hwdep_ioctl(handle, SNDRV_EMU10K1_IOCTL_INFO, &info) < 0) {
			error("unable to get info ");
			ld10k1_free_code_struct(&code);
			return LD10K1_ERR_DRIVER_INFO;
		}

		dsp_mgr->i_tram.size = info.internal_tram_size;
		dsp_mgr->e_tram.size = info.external_tram_size;
	}
	
	/* get count of controls */
	code.gpr_list_control_count = 0;
	if (snd_hwdep_ioctl(handle, SNDRV_EMU10K1_IOCTL_CODE_PEEK, &code) < 0) {
		error("unable to peek code");
		ld10k1_free_code_struct(&code);
		return LD10K1_ERR_DRIVER_CODE_PEEK;
	}

	ctrl = calloc(code.gpr_list_control_total,
		      sizeof(emu10k1_fx8010_control_gpr_t));
	if (!ctrl) {
		ld10k1_free_code_struct(&code);
		return LD10K1_ERR_NO_MEM;
	}

	code.gpr_list_control_count = code.gpr_list_control_total;
	code.gpr_list_controls = ctrl;

	for (i = 0; i < sizeof(code.gpr_valid) / sizeof(unsigned long); i++)
		code.gpr_valid[i] = 0x0;
	for (i = 0; i < sizeof(code.tram_valid) / sizeof(unsigned long); i++)
		code.tram_valid[i] = 0x0;
	for (i = 0; i < sizeof(code.code_valid) / sizeof(unsigned long); i++)
		code.code_valid[i] = 0x0;;

	if (snd_hwdep_ioctl(handle, SNDRV_EMU10K1_IOCTL_CODE_PEEK, &code) < 0) {
		error("unable to peek code");
		ld10k1_free_code_struct(&code);
		free(ctrl);
		return LD10K1_ERR_DRIVER_CODE_PEEK;
	}
	
	

	/* new name */
	strcpy(code.name, LD10K1_SIGNATURE);
	for (i = 0; i < sizeof(code.gpr_valid) / sizeof(unsigned long); i++)
		code.gpr_valid[i] = ~0;

	for (i = 0; i < sizeof(code.gpr_valid) * 8; i++) {
		code.gpr_map[i] = 0;
	}

	ids = calloc(code.gpr_list_control_total,
		     sizeof(emu10k1_ctl_elem_id_t));
	if (!ids) {
		ld10k1_free_code_struct(&code);
		free(ctrl);
		return LD10K1_ERR_NO_MEM;
	}

	code.gpr_del_control_count = code.gpr_list_control_total;
	if (code.gpr_del_control_count) {
		for (i = 0; i < code.gpr_del_control_count; i++) {
			memcpy(&(ids[i]), &(ctrl[i].id), sizeof(emu10k1_ctl_elem_id_t));
		}
	}

	free(ctrl);

	code.gpr_del_controls = ids;
	code.gpr_list_control_count = 0;
	code.gpr_add_control_count = 0;
	code.gpr_list_control_count = 0;

	for (i = 0; i < sizeof(code.tram_valid) / sizeof(unsigned long); i++)
		code.tram_valid[i] = ~0;
	for (i = 0; i < sizeof(code.code_valid) / sizeof(unsigned long); i++)
		code.code_valid[i] = ~0;

	for (i = 0; i < sizeof(code.tram_valid) * 8; i++) {
		code.tram_addr_map[i] = 0;
		code.tram_data_map[i] = 0;
	}

	for (iptr = code.code, i = 0; i < sizeof(code.code_valid) * 8; i++, iptr += 2)
		if (dsp_mgr->audigy) {
			ld10k1_syntetize_instr(dsp_mgr->audigy,
				0x0f,
				0xc0, 0xc0, 0xcf, 0xc0, iptr);
		} else {
			ld10k1_syntetize_instr(dsp_mgr->audigy,
				0x06,
				0x40, 0x40, 0x40, 0x40, iptr);
		}
		
	/* initialize i2s outputs on audigy */
	if (dsp_mgr->audigy) {
		for (iptr = code.code, i = 0; audigy_must_init_output[i] > 0; i += 2, iptr += 2)
			ld10k1_syntetize_instr(dsp_mgr->audigy,	0x00,
				audigy_must_init_output[i], 0xc0, 0xc0, 0xc0, iptr);
	}
	
#ifndef DEBUG_DRIVER
	if (snd_hwdep_ioctl(handle, SNDRV_EMU10K1_IOCTL_CODE_POKE, &code) < 0) {
		error("unable to poke code");
		ld10k1_free_code_struct(&code);
		free(ids);
		return LD10K1_ERR_DRIVER_CODE_POKE;
	}
#endif

	free(ids);

	/* delete tram pcm dsp part */
	if (!dsp_mgr->audigy) {
		for (i = 0; i < EMU10K1_FX8010_PCM_COUNT; i++) {
			ipcm.substream = i;
			ipcm.channels = 0;
#ifndef DEBUG_DRIVER
			if (snd_hwdep_ioctl(handle, SNDRV_EMU10K1_IOCTL_PCM_POKE, &ipcm) < 0) {
				error("unable to poke code");
				ld10k1_free_code_struct(&code);
				return LD10K1_ERR_DRIVER_PCM_POKE;
			}
#endif
		}
	}
	
	ld10k1_free_code_struct(&code);
	return 0;
}

void ld10k1_init_must_init_output(ld10k1_dsp_mgr_t *dsp_mgr)
{
	int i;
	if (dsp_mgr->audigy) {
		for (i = 0; audigy_must_init_output[i] > 0; i += 2)
			audigy_must_init_output[i + 1] = 1;
	}
}
	
void ld10k1_set_must_init_output(ld10k1_dsp_mgr_t *dsp_mgr, int reg)
{
	int i ;
	if (dsp_mgr->audigy) {
		for (i = 0; audigy_must_init_output[i] > 0; i += 2) {
			if (audigy_must_init_output[i] == reg) {
				audigy_must_init_output[i + 1] = 0;
				return;			
			}
		}
	}
}

void ld10k1_check_must_init_output(ld10k1_dsp_mgr_t *dsp_mgr, emu10k1_fx8010_code_t *code)
{
	int j;
	
	ld10k1_init_must_init_output(dsp_mgr);
	for (j = 0; j < dsp_mgr->instr_count; j++) {
		if (dsp_mgr->instr[j].used)
			ld10k1_set_must_init_output(dsp_mgr, dsp_mgr->instr[j].arg[0]);
	}
	
	int i;
	int l;
	int ioffset = dsp_mgr->instr_count - 1;
	if (dsp_mgr->audigy) {
		for (i = 0; audigy_must_init_output[i] > 0; i += 2) {
			if (audigy_must_init_output[i + 1]) {
				/* find free instruction slot */
				for (;ioffset >= 0; ioffset--) {
					if (!dsp_mgr->instr[ioffset].used) {
						ld10k1_instr_t *instr = &(dsp_mgr->instr[ioffset]);
						ld10k1_syntetize_instr(dsp_mgr->audigy, 
							0x0, 
							audigy_must_init_output[i], 0xc0, 0xc0, 0xc0, 
							code->code + ioffset * 2);
						instr->op_code = 0;
						instr->arg[0] = audigy_must_init_output[i];
						for (l = 1; l < 4; l++)
							instr->arg[l] = 0xc0;
						set_bit(ioffset, code->code_valid);
						dsp_mgr->instr[ioffset].used = 1;
						ioffset--;
						break;
					}
				}
					
				if (ioffset < 0)
					return;				
			}		
		}
	}
}
