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
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "ld10k1_error.h"
#include "ld10k1_fnc.h"
#include "comm.h"
#include "liblo10k1.h"
#include "liblo10k1ef.h"

#define AS10K1_FILE_SIGNATURE_ALSA "EMU10K1 FX8010 1"
#define AS10K1_FILE_SIGNATURE_EMU "emu10k1-dsp-file"
#define AS10K1_FILE_FORMAT_VERSION_EMU 1

liblo10k1_emu_patch_t *liblo10k1_emu_new_patch()
{
	liblo10k1_emu_patch_t *tmp = (liblo10k1_emu_patch_t *)malloc(sizeof(liblo10k1_emu_patch_t));
	
	if (!tmp)
		return NULL;
	memset(tmp, 0, sizeof(liblo10k1_emu_patch_t));
	
	return tmp;
}

void liblo10k1_emu_free_patch(liblo10k1_emu_patch_t *p)
{
	liblo10k1_emu_patch_set_in_count(p, 0);
	liblo10k1_emu_patch_set_out_count(p, 0);
	liblo10k1_emu_patch_set_dyn_count(p, 0);
	liblo10k1_emu_patch_set_sta_count(p, 0);
	liblo10k1_emu_patch_set_ctl_count(p, 0);
	liblo10k1_emu_patch_set_con_count(p, 0);
	liblo10k1_emu_patch_set_lookup_count(p, 0);
	liblo10k1_emu_patch_set_delay_count(p, 0);
	liblo10k1_emu_patch_set_instr_count(p, 0);
	
	free(p);
}

int liblo10k1_emu_patch_set_in_count(liblo10k1_emu_patch_t *p, int count)
{
	unsigned int *tmp = NULL;
	
	if (count > 0) {
		tmp = (unsigned int *)malloc(sizeof(unsigned int)*count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
		memset(tmp, 0, sizeof(unsigned int) * count);
	}
	
	p->in_count = count;
	if (p->ins)
		free(p->ins);
	p->ins = tmp;
	return 0;
}

int liblo10k1_emu_patch_set_out_count(liblo10k1_emu_patch_t *p, int count)
{
	unsigned int *tmp = NULL;
	
	if (count > 0) {
		tmp = (unsigned int *)malloc(sizeof(unsigned int)*count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
		memset(tmp, 0, sizeof(unsigned int) * count);
	}
	
	p->out_count = count;
	if (p->outs)
		free(p->outs);
	p->outs = tmp;
	return 0;
}

int liblo10k1_emu_patch_set_dyn_count(liblo10k1_emu_patch_t *p, int count)
{
	unsigned int *tmp = NULL;
	
	if (count > 0) {
		tmp = (unsigned int *)malloc(sizeof(unsigned int)*count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
		memset(tmp, 0, sizeof(unsigned int) * count);
	}
	
	p->dyn_count = count;
	if (p->dyns)
		free(p->dyns);
	p->dyns = tmp;
	return 0;
}

int liblo10k1_emu_patch_set_sta_count(liblo10k1_emu_patch_t *p, int count)
{
	liblo10k1_emu_sc_t *tmp = NULL;
	
	if (count > 0) {
		tmp = (liblo10k1_emu_sc_t *)malloc(sizeof(liblo10k1_emu_sc_t) * count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
		memset(tmp, 0, sizeof(liblo10k1_emu_sc_t)*count);
	}
	
	p->sta_count = count;
	if (p->stas)
		free(p->stas);
	p->stas = tmp;
	return 0;
}

int liblo10k1_emu_patch_set_ctl_count(liblo10k1_emu_patch_t *p, int count)
{
	liblo10k1_emu_ctl_t *tmp = NULL;
	
	if (count > 0) {
		tmp = (liblo10k1_emu_ctl_t *)malloc(sizeof(liblo10k1_emu_ctl_t) * count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
		memset(tmp, 0, sizeof(liblo10k1_emu_ctl_t)*count);
	}
	
	p->ctl_count = count;
	if (p->ctls)
		free(p->ctls);
	p->ctls = tmp;
	return 0;
}

int liblo10k1_emu_patch_set_con_count(liblo10k1_emu_patch_t *p, int count)
{
	liblo10k1_emu_sc_t *tmp = NULL;
	
	if (count > 0) {
		tmp = (liblo10k1_emu_sc_t *)malloc(sizeof(liblo10k1_emu_sc_t) * count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
		memset(tmp, 0, sizeof(liblo10k1_emu_sc_t)*count);
	}
	
	p->con_count = count;
	if (p->cons)
		free(p->cons);
	p->cons = tmp;
	return 0;
}

int liblo10k1_emu_patch_set_line_count(liblo10k1_emu_tram_t *t, int write, int count)
{
	liblo10k1_emu_tram_line_t *tmp = NULL;
	
	if (count > 0) {
		tmp = (liblo10k1_emu_tram_line_t *)malloc(sizeof(liblo10k1_emu_tram_line_t) * count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
		memset(tmp, 0, sizeof(liblo10k1_emu_tram_line_t)*count);
	}
	
	if (write) {
		t->write_line_count = count;
		if (t->write_lines)
			free(t->write_lines);
		t->write_lines = tmp;
	} else {
		t->read_line_count = count;
		if (t->read_lines)
			free(t->read_lines);
		t->read_lines = tmp;
	}
	return 0;
}

int liblo10k1_emu_patch_set_lookup_count(liblo10k1_emu_patch_t *p, int count)
{
	liblo10k1_emu_tram_t *tmp = NULL;
	int i;
	
	if (count > 0) {
		tmp = (liblo10k1_emu_tram_t *)malloc(sizeof(liblo10k1_emu_tram_t) * count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
		memset(tmp, 0, sizeof(liblo10k1_emu_tram_t)*count);
	}
	
	if (p->tram_lookups) {
		for (i = 0; i < p->tram_lookup_count; i++) {
			liblo10k1_emu_patch_set_line_count(&(p->tram_lookups[i]), 0, 0);
			liblo10k1_emu_patch_set_line_count(&(p->tram_lookups[i]), 1, 0);
		}
		free(p->tram_lookups);
	}
	p->tram_lookup_count = count;
	p->tram_lookups = tmp;
	return 0;
}

int liblo10k1_emu_patch_set_delay_count(liblo10k1_emu_patch_t *p, int count)
{
	liblo10k1_emu_tram_t *tmp = NULL;
	int i;
	
	if (count > 0) {
		tmp = (liblo10k1_emu_tram_t *)malloc(sizeof(liblo10k1_emu_tram_t) * count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
		memset(tmp, 0, sizeof(liblo10k1_emu_tram_t)*count);
	}
	
	if (p->tram_delays) {
		for (i = 0; i < p->tram_delay_count; i++) {
			liblo10k1_emu_patch_set_line_count(&(p->tram_delays[i]), 0, 0);
			liblo10k1_emu_patch_set_line_count(&(p->tram_delays[i]), 1, 0);
		}
		free(p->tram_delays);
	}
	p->tram_delay_count = count;
	p->tram_delays = tmp;
	return 0;
}

int liblo10k1_emu_patch_set_instr_count(liblo10k1_emu_patch_t *p, int count)
{
	liblo10k1_emu_instr_t *tmp = NULL;
	
	if (count > 0) {
		tmp = (liblo10k1_emu_instr_t *)malloc(sizeof(liblo10k1_emu_instr_t) * count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
		memset(tmp, 0, sizeof(liblo10k1_emu_instr_t)*count);
	}
	
	p->instr_count = count;
	if (p->instrs)
		free(p->instrs);
	p->instrs = tmp;
	return 0;
}

static int read_byte(char *patch_data, int size, int *pos, unsigned char *out)
{
	if (*pos < size) {
		*out = patch_data[*pos];
		(*pos)++;
		return 0;
	} else
		return LD10K1_EF_ERR_FORMAT;
}

static int read_ushort(char *patch_data, int size, int *pos, unsigned short *out)
{
	if (*pos + 1 < size) {
		*out = *((unsigned short *)(patch_data + *pos));
		*(pos) += 2;
		return 0;
	} else
		return LD10K1_EF_ERR_FORMAT;
}

static int read_uint(char *patch_data, int size, int *pos, unsigned int *out)
{
	if (*pos + 3 < size) {
		*out = *((unsigned int *)(patch_data + *pos));
		(*pos) += 4;
		return 0;
	} else
		return LD10K1_EF_ERR_FORMAT;
}

static int read_string(char *patch_data, int size, int *pos, char *out, int ssize)
{
	if (*pos + ssize - 1 < size) {
		strncpy(out, &(patch_data[*pos]), ssize - 1);
		out[ssize - 1] = '\0';
		(*pos) += ssize;
		return 0;
	} else
		return LD10K1_EF_ERR_FORMAT;
}

int liblo10k1_emu_load_patch(char *file_name, liblo10k1_emu_patch_t **p)
{
	struct stat patch_stat;
	char *patch_data;
	FILE *patch_file;
	liblo10k1_emu_patch_t *new_patch = NULL;
	unsigned int i, j, z, k;
	int en = 0;

	int file_pos = 0;
	int file_size = 0;
	
	unsigned char byte_tmp;
	unsigned short ushort_tmp;
	
	liblo10k1_emu_tram_t *tmp_tram;
	liblo10k1_emu_tram_line_t *tmp_tram_lines;
	
	unsigned int part1, part2;

	if (!(patch_file = fopen(file_name, "r")))
		return LD10K1_EF_ERR_OPEN;

	/* first load patch to mem */
	if (stat(file_name, &patch_stat))
		return LD10K1_EF_ERR_STAT;

	/* minimal patch len is 57 */
	if (patch_stat.st_size < 57 || patch_stat.st_size > 1000000)
		return LD10K1_EF_ERR_SIZE;

	file_size = patch_stat.st_size;

	patch_data = (char *)malloc(patch_stat.st_size);
	if (!patch_data)
		return LD10K1_ERR_NO_MEM;

	if (fread(patch_data, patch_stat.st_size, 1, patch_file) != 1) {
		fclose(patch_file);
		en = LD10K1_EF_ERR_READ;
		goto err;
	} else
		fclose(patch_file);

	int file_sig = 0;
		
	/* signature checks - two kinds of as10k1 files, one from alsa-tools, other from emu-tools. */
	if(strncmp(patch_data, AS10K1_FILE_SIGNATURE_ALSA, 16) != 0)
	{
	  if((strncmp(patch_data, AS10K1_FILE_SIGNATURE_EMU, 16) == 0) && (*((unsigned short *)&patch_data[17]) == AS10K1_FILE_FORMAT_VERSION_EMU))
	    file_sig = 3;
		else	
		{
	    en = LD10K1_EF_ERR_SIGNATURE;
	    goto err;
		}
	}
	
	new_patch = liblo10k1_emu_new_patch();
	if (!new_patch) {
		en = LD10K1_ERR_NO_MEM;
		goto err;
	}
	
	/* next patch name */
	strncpy(new_patch->patch_name, &(patch_data[16 + file_sig]), 31);
	new_patch->patch_name[31] = '\0';
	/* registers */
	file_pos = 32+16 + file_sig; 
	
	/* in count */
	if ((en = read_byte(patch_data, file_size, &file_pos, &byte_tmp)) < 0)
		  goto err;
	
	if (byte_tmp >= 32) {
		en = LD10K1_EF_ERR_FORMAT;
		goto err;
	}
	
	if ((en = liblo10k1_emu_patch_set_in_count(new_patch, byte_tmp)) < 0 ||
		(en = liblo10k1_emu_patch_set_out_count(new_patch, byte_tmp)) < 0)
		goto err;
	
	/* read in gprs */
	for (i = 0; i < new_patch->in_count; i++) {
		if ((en = read_byte(patch_data, file_size, &file_pos, &byte_tmp)) < 0)
			goto err;
		new_patch->ins[i] = byte_tmp;
		if ((en = read_byte(patch_data, file_size, &file_pos, &byte_tmp)) < 0)
			goto err;
		new_patch->outs[i] = byte_tmp;
	}
	
	/* read dyn gprs */
	if ((en = read_byte(patch_data, file_size, &file_pos, &byte_tmp)) < 0)
		goto err;
		
	if ((en = liblo10k1_emu_patch_set_dyn_count(new_patch, byte_tmp)) < 0)
		goto err;
	
	for (i = 0; i < new_patch->dyn_count; i++) {
		if ((en = read_byte(patch_data, file_size, &file_pos, &byte_tmp)) < 0)
			goto err;
		new_patch->dyns[i] = byte_tmp;
	}
	
	/* read sta gprs */
	if ((en = read_byte(patch_data, file_size, &file_pos, &byte_tmp)) < 0)
		goto err;
		
	if ((en = liblo10k1_emu_patch_set_sta_count(new_patch, byte_tmp)) < 0)
		goto err;
	
	for (i = 0; i < new_patch->sta_count; i++) {
		if ((en = read_byte(patch_data, file_size, &file_pos, &byte_tmp)) < 0)
			goto err;
		new_patch->stas[i].sc = byte_tmp;
		if ((en = read_uint(patch_data, file_size, &file_pos, &(new_patch->stas[i].sc_val))) < 0)
			goto err;
	}
	
	/* read ctl gprs */
	if ((en = read_byte(patch_data, file_size, &file_pos, &byte_tmp)) < 0)
		goto err;
		
	if ((en = liblo10k1_emu_patch_set_ctl_count(new_patch, byte_tmp)) < 0)
		goto err;
	
	for (i = 0; i < new_patch->ctl_count; i++) {
		if ((en = read_byte(patch_data, file_size, &file_pos, &byte_tmp)) < 0)
			goto err;
		new_patch->ctls[i].ctl = byte_tmp;
		if ((en = read_uint(patch_data, file_size, &file_pos, &(new_patch->ctls[i].ctl_val))) < 0)
			goto err;
		if ((en = read_uint(patch_data, file_size, &file_pos, &(new_patch->ctls[i].ctl_val_min))) < 0)
			goto err;
		if ((en = read_uint(patch_data, file_size, &file_pos, &(new_patch->ctls[i].ctl_val_max))) < 0)
			goto err;
		if ((en = read_string(patch_data, file_size, &file_pos, new_patch->ctls[i].ctl_name, 32)) < 0)
			goto err;
	}
	
	/* read const gprs */
	if ((en = read_byte(patch_data, file_size, &file_pos, &byte_tmp)) < 0)
		goto err;
		
	if ((en = liblo10k1_emu_patch_set_con_count(new_patch, byte_tmp)) < 0)
		goto err;
	
	for (i = 0; i < new_patch->con_count; i++) {
		if ((en = read_byte(patch_data, file_size, &file_pos, &byte_tmp)) < 0)
			goto err;
		new_patch->cons[i].sc = byte_tmp;
		if ((en = read_uint(patch_data, file_size, &file_pos, &(new_patch->cons[i].sc_val))) < 0)
			goto err;
	}
	
	/* read tram lookup gprs */
	for (z = 0; z < 2; z++) {
		if ((en = read_byte(patch_data, file_size, &file_pos, &byte_tmp)) < 0)
			goto err;
	
		if (z) {
			if ((en = liblo10k1_emu_patch_set_delay_count(new_patch, byte_tmp)) < 0)
				goto err;
		} else {		
			if ((en = liblo10k1_emu_patch_set_lookup_count(new_patch, byte_tmp)) < 0)
				goto err;
		}
	
		for (i = 0; i < (z ? new_patch->tram_delay_count : new_patch->tram_lookup_count); i++) {
			/* size */
			if (z) {
				if ((en = read_uint(patch_data, file_size, &file_pos, &(new_patch->tram_delays[i].size))) < 0)
					goto err;
				tmp_tram = new_patch->tram_delays + i;
			} else {
				if ((en = read_uint(patch_data, file_size, &file_pos, &(new_patch->tram_lookups[i].size))) < 0)
					goto err;
				tmp_tram = new_patch->tram_lookups + i;
			}
			
			for (k = 0; k < 2; k++) {
				/* read lines */
				if ((en = read_byte(patch_data, file_size, &file_pos, &byte_tmp)) < 0)
					goto err;
				
				if ((en = liblo10k1_emu_patch_set_line_count(tmp_tram, k, byte_tmp)) < 0)
					goto err;
				
				if (k)
					tmp_tram_lines = tmp_tram->write_lines;
				else
					tmp_tram_lines = tmp_tram->read_lines;
				
				for (j = 0; j < (k ? tmp_tram->write_line_count : tmp_tram->read_line_count); j++) {
					if ((en = read_byte(patch_data, file_size, &file_pos, &byte_tmp)) < 0)
						goto err;
						
					tmp_tram_lines[j].line = byte_tmp;
				
					if ((en = read_uint(patch_data, file_size, &file_pos, &(tmp_tram_lines[j].line_size))) < 0)
						goto err;
				}
			}
		}
	}
	
	/* instruction lines */
	if ((en = read_ushort(patch_data, file_size, &file_pos, &ushort_tmp)) < 0)
		goto err;
		
	if(file_sig)
	  ushort_tmp >>= 1;
		
	if ((en = liblo10k1_emu_patch_set_instr_count(new_patch, ushort_tmp)) < 0)
		goto err;
	
	for (i = 0; i < new_patch->instr_count; i++) {
		if ((en = read_uint(patch_data, file_size, &file_pos, &part1)) < 0)
			goto err;
		if ((en = read_uint(patch_data, file_size, &file_pos, &part2)) < 0)
			goto err;
			
		/* fill instr */
		new_patch->instrs[i].arg[2] = part1 >> 10;
		new_patch->instrs[i].arg[3] = part1 & 0x3FF;
		new_patch->instrs[i].op = part2 >> 20;
		new_patch->instrs[i].arg[0] = (part2 >> 10) & 0x3FF;
		new_patch->instrs[i].arg[1] = part2 & 0x3FF;
	}
	
	*p = new_patch;
	
	if (patch_data)
		free(patch_data);
	return 0;
err:
	if (patch_data)
		free(patch_data);
	if (new_patch)
		liblo10k1_emu_free_patch(new_patch);
	return en;
}

typedef struct
{
	unsigned int gpr;
	unsigned int ld_gpr;
} used_gpr_t;


static int check_if_used(used_gpr_t *gprs, int count, unsigned int gpr)
{
	int i;
	for (i = 0; i < count; i++) {
		if (gprs[i].gpr == gpr)
			return i;
	}
	return -1;
}

static char *default_in_names[] =
{
	"IL",
	"IR",
	"IRL",
	"IRR",
	"IC",
	"ILFE"
};

static char *default_out_names[] =
{
	"OL",
	"OR",
	"ORL",
	"ORR",
	"OC",
	"OLFE"
};

int liblo10k1_patch_find_ctl_by_name(liblo10k1_dsp_patch_t *p, char *ctl_name)
{
	int i;
	for (i = 0; i < p->ctl_count; i++)
		if (strcmp(p->ctl[i].name, ctl_name) == 0)
			return i;
	return -1;
}

int liblo10k1_patch_ctl_set_trans(liblo10k1_dsp_patch_t *p, int idx, int trans) 
{
	int i;
	switch (trans) {
		case EMU10K1_GPR_TRANSLATION_NONE:
			break;
		case EMU10K1_GPR_TRANSLATION_TABLE100:
			if (p->ctl[idx].min != 0 && p->ctl[idx].max != 100)
				return LD10K1_EF_ERR_TRANSFORM_TRANS;
			break;
		case EMU10K1_GPR_TRANSLATION_BASS:
		case EMU10K1_GPR_TRANSLATION_TREBLE:
			if (p->ctl[idx].min != 0 && p->ctl[idx].max != 0xFFFFFFFF)
				return LD10K1_EF_ERR_TRANSFORM_TRANS;
			if (p->ctl[idx].count != 5)
				return LD10K1_EF_ERR_TRANSFORM_TRANS;
			break;
		case EMU10K1_GPR_TRANSLATION_ONOFF:
			if (p->ctl[idx].min != 0 && p->ctl[idx].max != 1)
				return LD10K1_EF_ERR_TRANSFORM_TRANS;
			break;
	}
	for (i = 0; i < p->ctl[idx].count; i++)
		if (p->ctl[idx].value[i] < p->ctl[idx].min || p->ctl[idx].value[i] > p->ctl[idx].max)
			return LD10K1_EF_ERR_TRANSFORM_TRANS;
	p->ctl[idx].translation = trans;
	return 0;
}

int liblo10k1_patch_ctl_set_vcount(liblo10k1_dsp_patch_t *p, int idx, int vc) 
{
	if (p->ctl[idx].count < vc)
		return LD10K1_EF_ERR_TRANSFORM_CTL;
	p->ctl[idx].vcount = vc;
	return 0;
}

int liblo10k1_patch_ctl_set_index(liblo10k1_dsp_patch_t *p, int idx, int i) 
{
	p->ctl[idx].index = i;
	return 0;
}

int liblo10k1_patch_ctl_set_value(liblo10k1_dsp_patch_t *p, int idx, int vi, int val) 
{
	if (p->ctl[idx].count < vi)
		return LD10K1_EF_ERR_TRANSFORM_CTL;
	if (val < p->ctl[idx].min || val > p->ctl[idx].max)
			return LD10K1_EF_ERR_TRANSFORM_CTL;
	p->ctl[idx].value[vi] = val;
	return 0;
}

int liblo10k1_emu_transform_patch(liblo10k1_emu_patch_t *ep,  liblo10k1_ctl_transform_t *tctl, int tctl_count, liblo10k1_dsp_patch_t **lp)
{
	used_gpr_t used_gpr[0x400];
	int used_gpr_count = 0;
	int i, j, k;
	int gpr;
	int tmp_cnt;
	int idx;
	int en = 0;
	int io_name_map[6];
	int transformed;
	liblo10k1_dsp_ctl_t *tmp_nctl;
	liblo10k1_emu_ctl_t *tmp_octl;
	int const_count, hw_count;	/*int ctl_transform_map[256];*/
	liblo10k1_dsp_patch_t *np = NULL;
	liblo10k1_emu_tram_t *tram_tmp;
	liblo10k1_dsp_tram_acc_t *tram_nacc;
	liblo10k1_emu_tram_line_t *tram_line;
	int val, addhw, addconst;
	
	/* for all instruction get used gpr list */
	for (i = 0; i < ep->instr_count; i++) {
		for (j = 0; j < 4; j++) {
			gpr = ep->instrs[i].arg[j];
			for (k = 0; k < used_gpr_count; k++)
				if (gpr == used_gpr[k].gpr)
					break;
					
			if (k < used_gpr_count)
				continue;
			
			used_gpr[used_gpr_count].gpr = gpr;
			used_gpr[used_gpr_count].ld_gpr = 0;
			used_gpr_count++;
		}
	}
	
	np = liblo10k1_patch_alloc(0, 0, 0, 0, 0, 0, 0, 0, 0, ep->instr_count);
	if (!np)
		return LD10K1_ERR_NO_MEM;
		
	/* set patch name */
	strcpy(np->patch_name, ep->patch_name);
	
	/* in gprs */
	tmp_cnt = 0;
	for (i = 0; i < ep->in_count; i++) {
		idx = check_if_used(used_gpr, used_gpr_count, ep->ins[i] + 0x100);
		if (idx >= 0) {
			used_gpr[idx].ld_gpr = EMU10K1_PREG_IN(tmp_cnt);
			if (i < 6)
				io_name_map[i] = tmp_cnt;
			tmp_cnt++;
		} else {
			if (i < 6)
				io_name_map[i] = -1;
		}
	}
		
	if ((en = liblo10k1_patch_set_in_count(np, tmp_cnt)) < 0)
		goto err;
	
	/* set in name */
	if (ep->in_count == 2 || /* stereo */
		ep->in_count == 4 || /* 4 channel */
		ep->in_count == 6) { /* 5.1 */
		for (i = 0; i < ep->in_count; i++) {
			if (io_name_map[i] >= 0)
				strcpy(np->ins[io_name_map[i]].name, default_in_names[i]);
		}
	}
		
	/* out gprs */
	tmp_cnt = 0;
	for (i = 0; i < ep->out_count; i++) {
		idx = check_if_used(used_gpr, used_gpr_count, ep->outs[i] + 0x100);
		if (idx >= 0) {
			used_gpr[idx].ld_gpr = EMU10K1_PREG_OUT(tmp_cnt);
			if (i < 6)
				io_name_map[i] = tmp_cnt;
			tmp_cnt++;
		} else {
			if (i < 6)
				io_name_map[i] = -1;
		}
	}
		
	if ((en = liblo10k1_patch_set_out_count(np, tmp_cnt)) < 0)
		goto err;
	
	/* set out name */
	if (ep->out_count == 2 || /* stereo */
		ep->out_count == 4 || /* 4 channel */
		ep->out_count == 6) { /* 5.1 */
		for (i = 0; i < ep->out_count; i++) {
			if (io_name_map[i] >= 0)
				strcpy(np->outs[io_name_map[i]].name, default_out_names[i]);
		}
	}
	
	/* dyn regs */
	tmp_cnt = 0;
	for (i = 0; i < ep->dyn_count; i++) {
		idx = check_if_used(used_gpr, used_gpr_count, ep->dyns[i] + 0x100);
		if (idx >= 0) {
			used_gpr[idx].ld_gpr = EMU10K1_PREG_DYN(tmp_cnt);
			tmp_cnt++;
		}
	}
	
	if ((en = liblo10k1_patch_set_dyn_count(np, tmp_cnt)) < 0)
		goto err;
	
	/* sta regs */
	tmp_cnt = 0;
	for (i = 0; i < ep->sta_count; i++) {
		idx = check_if_used(used_gpr, used_gpr_count, ep->stas[i].sc + 0x100);
		if (idx >= 0) {
			used_gpr[idx].ld_gpr = EMU10K1_PREG_STA(tmp_cnt);
			tmp_cnt++;
		}
	}
	
	if ((en = liblo10k1_patch_set_sta_count(np, tmp_cnt)) < 0)
		goto err;
		
	/* sta regs - initilization */
	tmp_cnt = 0;
	for (i = 0; i < ep->sta_count; i++) {
		idx = check_if_used(used_gpr, used_gpr_count, ep->stas[i].sc + 0x100);
		if (idx >= 0) {
			np->stas[tmp_cnt].const_val = ep->stas[i].sc_val;
			tmp_cnt++;
		}
	}
	
	/* FIXME - ak je niektoty ctl pouzity v tctl viackrat tak to zblbne */
	/* ctls regs */
	/* first get count of ctls */
	tmp_cnt = 0;
	for (i = 0; i < tctl_count; i++)
		tmp_cnt += tctl[i].emu_ctl_count;
	
	if ((en = liblo10k1_patch_set_ctl_count(np, ep->ctl_count - tmp_cnt + tctl_count)) < 0)
		goto err;
		
	tmp_cnt = 0;
	for (i = 0; i < ep->ctl_count; i++) {
		tmp_octl = &(ep->ctls[i]);
		/* find if transformed */
		transformed = 0;
		for (j = 0; j < tctl_count; j++) {
			for (k = 0; k < tctl[j].emu_ctl_count; k++) {
				if (i == tctl[j].emu_ctls[k]) {
					/* it is transformed */
					tmp_nctl = &(np->ctl[j]);
					if (strcmp(tmp_nctl->name, "") != 0) {
						/* initialized - check min, max */
						if (tmp_octl->ctl_val_min != tmp_nctl->min ||
							tmp_octl->ctl_val_max != tmp_nctl->max) {
							en = LD10K1_EF_ERR_TRANSFORM_CTL;
							goto err;
						}
						
						tmp_nctl->value[k] = tmp_octl->ctl_val;
					} else {
						/* initialize it */
						strcpy(tmp_nctl->name, tctl[j].ctl_name);
						tmp_nctl->index = -1;
						tmp_nctl->vcount = tctl[j].emu_ctl_count;/*1;*/
						tmp_nctl->count = tctl[j].emu_ctl_count;/*1;*/
						tmp_nctl->value[k] = tmp_octl->ctl_val;
						tmp_nctl->min = tmp_octl->ctl_val_min;
						tmp_nctl->max = tmp_octl->ctl_val_max;
						tmp_nctl->translation = EMU10K1_GPR_TRANSLATION_NONE;
					}
					
					idx = check_if_used(used_gpr, used_gpr_count, tmp_octl->ctl + 0x100);
					if (idx >= 0)
						used_gpr[idx].ld_gpr = EMU10K1_PREG_CTL(j, k);
					
					transformed = 1;
					break;
				}
			}
			if (transformed)
				break;
		}
		
		if (transformed)
			continue;
		/* not transformed */
		tmp_nctl = &(np->ctl[tmp_cnt + tctl_count]);
		
		strcpy(tmp_nctl->name, tmp_octl->ctl_name);
		tmp_nctl->index = -1;
		tmp_nctl->vcount = 1;
		tmp_nctl->count = 1;
		tmp_nctl->value[0] = tmp_octl->ctl_val;
		tmp_nctl->min = tmp_octl->ctl_val_min;
		tmp_nctl->max = tmp_octl->ctl_val_max;
		tmp_nctl->translation = EMU10K1_GPR_TRANSLATION_NONE;

		idx = check_if_used(used_gpr, used_gpr_count, tmp_octl->ctl + 0x100);
		if (idx >= 0)
			used_gpr[idx].ld_gpr = EMU10K1_PREG_CTL(tmp_cnt + tctl_count, 0);
		
		tmp_cnt++;
		
		if (tmp_cnt > np->ctl_count) {
			en = LD10K1_EF_ERR_TRANSFORM_CTL;
			goto err;
		}
	}
	
	if (tmp_cnt + tctl_count < np->ctl_count) {
		en = LD10K1_EF_ERR_TRANSFORM_CTL;
		goto err;
	}
	
	/* tram */
	if ((en = liblo10k1_patch_set_tram_count(np, ep->tram_lookup_count + ep->tram_delay_count)) < 0)
		goto err;
	
	tmp_cnt = 0;
	for (i = 0; i < ep->tram_lookup_count + ep->tram_delay_count; i++) {
		if (i < ep->tram_lookup_count)
			tram_tmp = &(ep->tram_lookups[i]);
		else
			tram_tmp = &(ep->tram_delays[i - ep->tram_lookup_count]);
			
		tmp_cnt += tram_tmp->read_line_count + tram_tmp->write_line_count;
		np->tram[i].grp_type = i < ep->tram_lookup_count ? TRAM_GRP_TABLE : TRAM_GRP_DELAY;
		np->tram[i].grp_size = tram_tmp->size;
		np->tram[i].grp_pos = TRAM_POS_AUTO;
	}
	
	/* tram acc */
	if ((en = liblo10k1_patch_set_tram_acc_count(np, tmp_cnt)) < 0)
		goto err;
		
	tmp_cnt = 0;
	for (i = 0; i < ep->tram_lookup_count + ep->tram_delay_count; i++) {
		if (i < ep->tram_lookup_count)
			tram_tmp = &(ep->tram_lookups[i]);
		else
			tram_tmp = &(ep->tram_delays[i - ep->tram_lookup_count]);
			
		for (k = 0; k < 2; k++) {
			for (j = 0; j < (k ? tram_tmp->write_line_count : tram_tmp->read_line_count); j++) {
				if (k)
					tram_line = &(tram_tmp->write_lines[j]);
				else
					tram_line = &(tram_tmp->read_lines[j]);
					
				tram_nacc = &(np->tram_acc[tmp_cnt]);
					
				tram_nacc->grp = i;
				tram_nacc->acc_offset = tram_line->line_size;
				tram_nacc->acc_type = k ? TRAM_ACC_WRITE : TRAM_ACC_READ;
				
				idx = check_if_used(used_gpr, used_gpr_count, tram_line->line + 0x200);
				if (idx >= 0)
					used_gpr[idx].ld_gpr = EMU10K1_PREG_TRAM_DATA(tmp_cnt);
				
				idx = check_if_used(used_gpr, used_gpr_count, tram_line->line + 0x300);
				if (idx >= 0)
					used_gpr[idx].ld_gpr = EMU10K1_PREG_TRAM_ADDR(tmp_cnt);
			
				tmp_cnt++;
			}
		}
	}
	
	/* const and hw */
	const_count = 0;
	hw_count = 0;
	
	for (i = 0; i < used_gpr_count; i++) {
		gpr = used_gpr[i].gpr;
		
		if (gpr < 0x40)
			hw_count++;
		else if (gpr < 0x56)
			const_count++;
		else if (gpr < 0x5C)
			hw_count++;
	}
	
	/* const regs */
	tmp_cnt = 0;
	for (i = 0; i < ep->con_count; i++) {
		idx = check_if_used(used_gpr, used_gpr_count, ep->cons[i].sc + 0x100);
		if (idx >= 0) {
			used_gpr[idx].ld_gpr = EMU10K1_PREG_CONST(tmp_cnt);
			tmp_cnt++;
		}
	}
	
	/* consts */
	if ((en = liblo10k1_patch_set_const_count(np, const_count + tmp_cnt)) < 0)
		goto err;
		
	const_count = tmp_cnt;
	
	/* const regs - initilization */
	tmp_cnt = 0;
	for (i = 0; i < ep->con_count; i++) {
		idx = check_if_used(used_gpr, used_gpr_count, ep->cons[i].sc + 0x100);
		if (idx >= 0) {
			np->consts[tmp_cnt].const_val = ep->cons[i].sc_val;
			tmp_cnt++;
		}
	}
	
	/* hw */
	if ((en = liblo10k1_patch_set_hw_count(np, hw_count)) < 0)
		goto err;
		
	hw_count = 0;
	for (i = 0; i < used_gpr_count; i++) {
		gpr = used_gpr[i].gpr;
		addconst = 0;
		addhw = 0;		
		
		if (gpr < 0x40) {
			/* fx */
			addhw = 1;
			if (gpr < 0x10)
				val = EMU10K1_REG_FX(gpr);
			else if (gpr < 0x20)
				val = EMU10K1_REG_IN(gpr - 0x10);
			else
				val = EMU10K1_REG_OUT(gpr - 0x20);
		} else {
			switch (gpr) {
				case 0x40:
					addconst = 1; val = 0x00000000;
					break;
				case 0x41:
					addconst = 1; val = 0x00000001;
					break;
				case 0x42:
					addconst = 1; val = 0x00000002;
					break;
				case 0x43:
					addconst = 1; val = 0x00000003;
					break;
				case 0x44:
					addconst = 1; val = 0x00000004;
					break;
				case 0x45:
					addconst = 1; val = 0x00000008;
					break;
				case 0x46:
					addconst = 1; val = 0x00000010;
					break;
				case 0x47:
					addconst = 1; val = 0x00000020;
					break;
				case 0x48:
					addconst = 1; val = 0x00000100;
					break;
				case 0x49:
					addconst = 1; val = 0x00010000;
					break;
				case 0x4A:
					addconst = 1; val = 0x00080000;
					break;
				case 0x4B:
					addconst = 1; val = 0x10000000;
					break;
				case 0x4C:
					addconst = 1; val = 0x20000000;
					break;
				case 0x4D:
					addconst = 1; val = 0x40000000;
					break;
				case 0x4E:
					addconst = 1; val = 0x80000000;
					break;
				case 0x4F:
					addconst = 1; val = 0x7FFFFFFF;
					break;
				case 0x50:
					addconst = 1; val = 0xFFFFFFFF;
					break;
				case 0x51:
					addconst = 1; val = 0xFFFFFFFE;
					break;
				case 0x52:
					addconst = 1; val = 0xC0000000;
					break;
				case 0x53:
					addconst = 1; val = 0x4F1BBCDC;
					break;
				case 0x54:
					addconst = 1; val = 0x5A7EF9DB;
					break;
				case 0x55:
					addconst = 1; val = 0x00100000;
					break;
				case 0x56:
					addhw = 1; val = EMU10K1_NREG_HW_ACCUM;
					break;
				case 0x57:
					addhw = 1; val = EMU10K1_NREG_HW_CCR;
					break;
				case 0x58:
					addhw = 1; val = EMU10K1_NREG_HW_NOISE1;
					break;
				case 0x59:
					addhw = 1; val = EMU10K1_NREG_HW_NOISE2;
					break;
				case 0x5A:
					addhw = 1; val = EMU10K1_NREG_HW_IRQ;
					break;
				case 0x5B:
					addhw = 1; val = EMU10K1_NREG_HW_DBAC;
					break;
				default:
					if (gpr < 0x100) {
						en = LD10K1_EF_ERR_TRANSFORM;
						goto err;
					}
					break;
			}
		}
		
		if (addhw) {
			np->hws[hw_count].hw_val = val;
			used_gpr[i].ld_gpr = EMU10K1_PREG_HW(hw_count);
			hw_count++;
		} else if (addconst) {
			np->consts[const_count].const_val = val;
			used_gpr[i].ld_gpr = EMU10K1_PREG_CONST(const_count);
			const_count++;
		}	
	}	

	/* instrs */
	if ((en = liblo10k1_patch_set_instr_count(np, ep->instr_count)) < 0)
		goto err;
	
	for (i = 0; i < ep->instr_count; i++) {
		np->instr[i].op_code = ep->instrs[i].op;
		for (j = 0; j < 4; j++) {
			gpr = ep->instrs[i].arg[j];
			
			idx = check_if_used(used_gpr, used_gpr_count, gpr);
			if (!used_gpr[idx].ld_gpr) {
				en = LD10K1_EF_ERR_TRANSFORM;
				goto err;
			}
				
			np->instr[i].arg[j] = used_gpr[idx].ld_gpr;
		}
	}
	
	*lp = np;
	return 0;
err:
	if (np)
		liblo10k1_patch_free(np);
	return en;
}

