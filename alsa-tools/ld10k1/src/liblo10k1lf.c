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
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "version.h"
#include "ld10k1.h"
#include "ld10k1_fnc.h"
#include "ld10k1_error.h"
#include "comm.h"
#include "liblo10k1.h"
#include "liblo10k1lf.h"

#define CREATER_MAJOR LD10K1_LIB_MAJOR
#define CREATER_MINOR LD10K1_LIB_MINOR
#define CREATER_SUBMINOR LD10K1_LIB_SUBMINOR

#define FILE_MAJOR 0
#define FILE_MINOR 1
#define FILE_SUBMINOR 0

#define READER_MAJOR 0
#define READER_MINOR 1
#define READER_SUBMINOR 7

#define LD10K1_FILE_SIGNATURE "LD10K1 NATIVE EFFECT FILE      "

int liblo10k1lf_save_dsp_setup(liblo10k1_file_dsp_setup_t *c, FILE *file);

int liblo10k1lf_save_file_header(FILE *file, unsigned int ft);
int liblo10k1lf_save_file_info(FILE *file, liblo10k1_file_info_t *fi);
int liblo10k1lf_save_part(FILE *file, unsigned int part_type, unsigned int part_id, unsigned int part_length, void *data);
int liblo10k1lf_find_part(FILE *file, unsigned int part_type, unsigned int part_id, unsigned int part_length, liblo10k1_file_part_t *part);
int liblo10k1lf_find_part_il(FILE *file, unsigned int part_type, unsigned int part_id, unsigned int part_length, int il, liblo10k1_file_part_t *part);

int liblo10k1lf_save_dsp_config(liblo10k1_file_dsp_setup_t *c, char *file_name, liblo10k1_file_info_t *fi)
{
	FILE *file = NULL;
	int err;
	
	file = fopen(file_name, "w");
	if (!file)
		return LD10K1_LF_ERR_OPEN;
		
	if ((err = liblo10k1lf_save_file_header(file, LD10K1_FP_INFO_FILE_TYPE_DSP_SETUP)) < 0)
		goto err;
		
	if ((err = liblo10k1lf_save_file_info(file, fi)) < 0)
		goto err;
		
	if ((err =  liblo10k1lf_save_dsp_setup(c, file)) < 0)
		goto err;
		
	fclose(file);
	return 0;
err:
	fclose(file);
	return err;
}

int liblo10k1lf_save_part(FILE *file, unsigned int part_type, unsigned int part_id, unsigned int part_length, void *data)
{
	liblo10k1_file_part_t part;
	
	memset(&part, 0, sizeof(part));
	part.part_type = part_type;
	part.part_id = part_id;
	part.part_length = part_length;
	
	if (fwrite(&part, sizeof(liblo10k1_file_part_t), 1, file) != 1)
		return LD10K1_LF_ERR_WRITE;
		
	if (part_length > 0)
		if (fwrite(data, part_length, 1, file) != 1)
			return LD10K1_LF_ERR_WRITE;
	return 0;
}

int liblo10k1lf_save_file_header(FILE *file, unsigned int ft)
{
	liblo10k1_file_header_t fhdr;
	liblo10k1_file_part_info_t file_info;
	int err;
	
	strcpy(fhdr.signature, LD10K1_FILE_SIGNATURE);
	memset(fhdr.reserved, 0, sizeof(fhdr.reserved));
	
	if (fwrite(&fhdr, sizeof(liblo10k1_file_header_t), 1, file) != 1)
		return LD10K1_LF_ERR_WRITE;
	
	memset(&file_info, 0, sizeof(file_info));
	
	file_info.file_type = ft;
	
	file_info.file_version_major = FILE_MAJOR;
	file_info.file_version_minor = FILE_MINOR;
	file_info.file_version_subminor = FILE_SUBMINOR; /* file version = 0.0.1 */
	
	file_info.minimal_reader_version_major = READER_MAJOR;
	file_info.minimal_reader_version_minor = READER_MINOR;
	file_info.minimal_reader_version_subminor = READER_SUBMINOR; /* minimal reader version = 0.1.7 */
	
	file_info.creater_version_major = CREATER_MAJOR;
	file_info.creater_version_minor = CREATER_MINOR;
	file_info.creater_version_subminor = CREATER_SUBMINOR; /* creater version = 0.1.7 */
	
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_NORMAL, LD10K1_FP_INFO, sizeof(file_info), &file_info)) < 0)
		return err;
	return 0;
}

int liblo10k1lf_save_string_info(FILE *file, int id, char *str)
{
	int str_len;
	int err;
	
	if (str)
		str_len = strlen(str) + 1;
	else
		str_len = 0;
		
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_NORMAL, id, str_len, str)) < 0)
		return err;
	return 0;
}

int liblo10k1lf_load_string_info(FILE *file, int id, char **str)
{
	char *tmp;
	int err;
	liblo10k1_file_part_t part;
	
	if ((err = liblo10k1lf_find_part_il(file, LD10K1_FP_TYPE_NORMAL, id, 0, 1, &part)) < 0)
		return err;
	
	tmp = NULL;
	if (part.part_length > 0) {
		tmp = (char *)malloc(part.part_length);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
			
		if (fread(tmp, part.part_length, 1, file) != 1) {
			free(tmp);
			return LD10K1_LF_ERR_READ;
		}
	}
	
	if (*str)
		free(*str);
	*str = tmp;
	return 0;
}

int liblo10k1lf_save_file_info(FILE *file, liblo10k1_file_info_t *fi)
{
	int err;
	
	if ((err = liblo10k1lf_save_string_info(file, LD10K1_FP_FILE_INFO_NAME, fi->name)) < 0)
		return err;
	if ((err = liblo10k1lf_save_string_info(file, LD10K1_FP_FILE_INFO_DESC, fi->desc)) < 0)
		return err;
	if ((err = liblo10k1lf_save_string_info(file, LD10K1_FP_FILE_INFO_CREATER, fi->creater)) < 0)
		return err;
	if ((err = liblo10k1lf_save_string_info(file, LD10K1_FP_FILE_INFO_AUTHOR, fi->author)) < 0)
		return err;
	if ((err = liblo10k1lf_save_string_info(file, LD10K1_FP_FILE_INFO_COPYRIGHT, fi->copyright)) < 0)
		return err;
	if ((err = liblo10k1lf_save_string_info(file, LD10K1_FP_FILE_INFO_LICENCE, fi->license)) < 0)
		return err;
	return 0;
}

int liblo10k1lf_load_file_info(FILE *file, liblo10k1_file_info_t **fi)
{
	int err;
	
	liblo10k1_file_info_t *i = liblo10k1lf_file_info_alloc();
	
	if (!i)
		return LD10K1_ERR_NO_MEM;
	
	if ((err = liblo10k1lf_load_string_info(file, LD10K1_FP_FILE_INFO_NAME, &(i->name))) < 0)
		goto err;
	if ((err = liblo10k1lf_load_string_info(file, LD10K1_FP_FILE_INFO_DESC, &(i->desc))) < 0)
		goto err;
	if ((err = liblo10k1lf_load_string_info(file, LD10K1_FP_FILE_INFO_CREATER, &(i->creater))) < 0)
		goto err;
	if ((err = liblo10k1lf_load_string_info(file, LD10K1_FP_FILE_INFO_AUTHOR, &(i->author))) < 0)
		goto err;
	if ((err = liblo10k1lf_load_string_info(file, LD10K1_FP_FILE_INFO_COPYRIGHT, &(i->copyright))) < 0)
		goto err;
	if ((err = liblo10k1lf_load_string_info(file, LD10K1_FP_FILE_INFO_LICENCE, &(i->license))) < 0)
		goto err;
		
	*fi = i;
	return 0;
err:
	if (i)
		liblo10k1lf_file_info_free(i);
	return err;
}

int liblo10k1lf_save_io(liblo10k1_get_io_t *ios, int count, int ptl, int pt, FILE *file)
{
	int i, err;
	/* io list start */
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_START, ptl, 0, NULL)) < 0)
		return err;
		
	for (i = 0; i < count; i++) {
		if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_NORMAL, pt, sizeof(liblo10k1_get_io_t), &(ios[i]))) < 0)
			return err;
	}
		
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_END, ptl, 0, NULL)) < 0)
		return err;
	
	return 0;
}

int liblo10k1lf_save_pio(liblo10k1_dsp_pio_t *ios, int count, int ptl, int pt, FILE *file)
{
	int i, err;
	/* io list start */
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_START, ptl, 0, NULL)) < 0)
		return err;
		
	for (i = 0; i < count; i++) {
		if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_NORMAL, pt, sizeof(liblo10k1_dsp_pio_t), &(ios[i]))) < 0)
			return err;
	}
		
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_END, ptl, 0, NULL)) < 0)
		return err;
	
	return 0;
}

int liblo10k1lf_save_cs(liblo10k1_dsp_cs_t *css, int count, int ptl, int pt, FILE *file)
{
	int i, err;
	/* io list start */
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_START, ptl, 0, NULL)) < 0)
		return err;
		
	for (i = 0; i < count; i++) {
		if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_NORMAL, pt, sizeof(liblo10k1_dsp_cs_t), &(css[i]))) < 0)
			return err;
	}
		
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_END, ptl, 0, NULL)) < 0)
		return err;
		
	return 0;
}

int liblo10k1lf_save_hw(liblo10k1_dsp_hw_t *hws, int count, FILE *file)
{
	int i, err;
	/* io list start */
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_START, LD10K1_FP_HW_LIST, 0, NULL)) < 0)
		return err;
		
	for (i = 0; i < count; i++) {
		if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_NORMAL, LD10K1_FP_HW, sizeof(liblo10k1_dsp_hw_t), &(hws[i]))) < 0)
			return err;
	}
		
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_END, LD10K1_FP_HW_LIST, 0, NULL)) < 0)
		return err;

	return 0;
}

int liblo10k1lf_save_tram(liblo10k1_dsp_tram_grp_t *trams, int count, FILE *file)
{
	int i, err;
	/* io list start */
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_START, LD10K1_FP_TRAM_LIST, 0, NULL)) < 0)
		return err;
		
	for (i = 0; i < count; i++) {
		if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_NORMAL, LD10K1_FP_TRAM, sizeof(liblo10k1_dsp_tram_grp_t), &(trams[i]))) < 0)
			return err;
	}
		
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_END, LD10K1_FP_TRAM_LIST, 0, NULL)) < 0)
		return err;
		
	return 0;
}

int liblo10k1lf_save_tram_acc(liblo10k1_dsp_tram_acc_t *tram_accs, int count, FILE *file)
{
	int i, err;
	/* io list start */
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_START, LD10K1_FP_TRAM_ACC_LIST, 0, NULL)) < 0)
		return err;
		
	for (i = 0; i < count; i++) {
		if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_NORMAL, LD10K1_FP_TRAM_ACC, sizeof(liblo10k1_dsp_tram_acc_t), &(tram_accs[i]))) < 0)
			return err;
	}
		
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_END, LD10K1_FP_TRAM_ACC_LIST, 0, NULL)) < 0)
		return err;
		
	return 0;
}

int liblo10k1lf_save_ctl(liblo10k1_dsp_ctl_t *ctls, int count, FILE *file)
{
	int i, err;
	/* io list start */
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_START, LD10K1_FP_CTL_LIST, 0, NULL)) < 0)
		return err;
		
	for (i = 0; i < count; i++) {
		if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_NORMAL, LD10K1_FP_CTL, sizeof(liblo10k1_dsp_ctl_t), &(ctls[i]))) < 0)
			return err;
	}
		
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_END, LD10K1_FP_CTL_LIST, 0, NULL)) < 0)
		return err;
		
	return 0;
}

int liblo10k1lf_save_instr(liblo10k1_dsp_instr_t *instrs, int count, FILE *file)
{
	int i, err;
	/* io list start */
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_START, LD10K1_FP_INSTR_LIST, 0, NULL)) < 0)
		return err;
		
	for (i = 0; i < count; i++) {
		if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_NORMAL, LD10K1_FP_INSTR, sizeof(liblo10k1_dsp_instr_t), &(instrs[i]))) < 0)
			return err;
	}
		
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_END, LD10K1_FP_INSTR_LIST, 0, NULL)) < 0)
		return err;
		
	return 0;
}

int liblo10k1lf_save_points(liblo10k1_point_info_t *points, int count, FILE *file)
{
	int i, err;
	/* io list start */
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_START, LD10K1_FP_POINT_LIST, 0, NULL)) < 0)
		return err;
		
	for (i = 0; i < count; i++) {
		if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_NORMAL, LD10K1_FP_POINT, sizeof(liblo10k1_point_info_t), &(points[i]))) < 0)
			return err;
	}
		
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_END, LD10K1_FP_POINT_LIST, 0, NULL)) < 0)
		return err;
		
	return 0;
}

int liblo10k1lf_save_patch(liblo10k1_dsp_patch_t *p, FILE *file)
{
	int err;
	liblo10k1_file_patch_info_t pinfo;
	
	/* io list start */
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_START, LD10K1_FP_PATCH, 0, NULL)) < 0)
		return err;
		
	/* patch info */
	strcpy(pinfo.patch_name, p->patch_name);
	pinfo.in_count = p->in_count;
	pinfo.out_count = p->out_count;
	pinfo.const_count = p->const_count;
	pinfo.sta_count = p->sta_count;
	pinfo.dyn_count = p->dyn_count;
	pinfo.hw_count = p->hw_count;
	pinfo.tram_count = p->tram_count;
	pinfo.tram_acc_count = p->tram_acc_count;
	pinfo.ctl_count = p->ctl_count;
	pinfo.instr_count = p->instr_count;

	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_NORMAL, LD10K1_FP_PATCH_INFO, sizeof(liblo10k1_file_patch_info_t), &pinfo)) < 0)
			return err;
			
	/* pins */
	if ((err = liblo10k1lf_save_pio(p->ins, p->in_count, LD10K1_FP_PIN_LIST, LD10K1_FP_PIO, file)) < 0)
		return err;
	/* pouts */
	if ((err = liblo10k1lf_save_pio(p->outs, p->out_count, LD10K1_FP_POUT_LIST, LD10K1_FP_PIO, file)) < 0)
		return err;
	/* consts */
	if ((err = liblo10k1lf_save_cs(p->consts, p->const_count, LD10K1_FP_CONST_LIST, LD10K1_FP_CS, file)) < 0)
		return err;
	/* stas */
	if ((err = liblo10k1lf_save_cs(p->stas, p->sta_count, LD10K1_FP_STA_LIST, LD10K1_FP_CS, file)) < 0)
		return err;
	/* hws */
	if ((err = liblo10k1lf_save_hw(p->hws, p->hw_count, file)) < 0)
		return err;
	/* trams */
	if ((err = liblo10k1lf_save_tram(p->tram, p->tram_count, file)) < 0)
		return err;
	/* tram_accs */
	if ((err = liblo10k1lf_save_tram_acc(p->tram_acc, p->tram_acc_count, file)) < 0)
		return err;
	/* ctls */
	if ((err = liblo10k1lf_save_ctl(p->ctl, p->ctl_count, file)) < 0)
		return err;
	/* instrs */
	if ((err = liblo10k1lf_save_instr(p->instr, p->instr_count, file)) < 0)
		return err;
		
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_END, LD10K1_FP_PATCH, 0, NULL)) < 0)
		return err;
		
	return 0;
}

int liblo10k1lf_save_dsp_setup(liblo10k1_file_dsp_setup_t *c, FILE *file)
{
	liblo10k1_file_part_dsp_setup_t setup;
	int err;
	int i;
	
	setup.dsp_type = c->dsp_type;
	setup.fx_count = c->fx_count;
	setup.in_count = c->in_count;
	setup.out_count = c->out_count;
	setup.patch_count = c->patch_count;
	setup.point_count = c->point_count;
	
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_NORMAL, LD10K1_FP_DSP_SETUP, sizeof(setup), &setup)) < 0)
		return err;
		
	/* save fx */
	if ((err = liblo10k1lf_save_io(c->fxs, c->fx_count, LD10K1_FP_FX_LIST, LD10K1_FP_FX, file)) < 0)
		return err;
		
	/* save in */
	if ((err = liblo10k1lf_save_io(c->ins, c->in_count, LD10K1_FP_IN_LIST, LD10K1_FP_IN, file)) < 0)
		return err;
		
	/* save out */
	if ((err = liblo10k1lf_save_io(c->outs, c->out_count, LD10K1_FP_OUT_LIST, LD10K1_FP_OUT, file)) < 0)
		return err;
	
	/* save patches */	
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_START, LD10K1_FP_PATCH_LIST, 0, NULL)) < 0)
		return err;
		
	for (i = 0; i < c->patch_count; i++) {
		if ((err = liblo10k1lf_save_patch(c->patches[i], file)) < 0)
			return err;
	}
	
	if ((err = liblo10k1lf_save_part(file, LD10K1_FP_TYPE_END, LD10K1_FP_PATCH_LIST, 0, NULL)) < 0)
		return err;
	
	/* save points */
	if ((err = liblo10k1lf_save_points(c->points, c->point_count, file)) < 0)
		return err;
		
	return 0;
}

liblo10k1_file_dsp_setup_t *liblo10k1lf_dsp_config_alloc()
{
	liblo10k1_file_dsp_setup_t *tmp = (liblo10k1_file_dsp_setup_t *)malloc(sizeof(liblo10k1_file_dsp_setup_t));
	
	memset(tmp, 0, sizeof(liblo10k1_file_dsp_setup_t));
	
	return tmp;
}

void liblo10k1lf_dsp_config_free(liblo10k1_file_dsp_setup_t *c)
{
	int i;
	
	if (c->fxs)
		free(c->fxs);
	if (c->ins)
		free(c->ins);
	if (c->outs)
		free(c->outs);
	if (c->patches) {
		for (i = 0; i < c->patch_count; i++) {
			if (c->patches[i])
				liblo10k1_patch_free(c->patches[i]);
		}
		free(c->patches);
	}
	if (c->points)
		free(c->points);
}

int liblo10k1lf_dsp_config_set_fx_count(liblo10k1_file_dsp_setup_t *c, int count)
{
	liblo10k1_get_io_t *tmp = NULL;
	
	if (count > 0) {
		tmp = (liblo10k1_get_io_t *)malloc(sizeof(liblo10k1_get_io_t) * count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
	}
	
	if (c->fxs)
		free(c->fxs);
		
	c->fx_count = count;
	c->fxs = tmp;
	return 0;
}	
	
int liblo10k1lf_dsp_config_set_in_count(liblo10k1_file_dsp_setup_t *c, int count)
{	
	liblo10k1_get_io_t *tmp = NULL;
	
	if (count > 0) {
		tmp = (liblo10k1_get_io_t *)malloc(sizeof(liblo10k1_get_io_t) * count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
	}
	
	if (c->ins)
		free(c->ins);
		
	c->in_count = count;
	c->ins = tmp;
	return 0;
}
	
int liblo10k1lf_dsp_config_set_out_count(liblo10k1_file_dsp_setup_t *c, int count)
{		
	liblo10k1_get_io_t *tmp = NULL;
	
	if (count > 0) {
		tmp = (liblo10k1_get_io_t *)malloc(sizeof(liblo10k1_get_io_t) * count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
	}
	
	if (c->outs)
		free(c->outs);
		
	c->out_count = count;
	c->outs = tmp;
	return 0;
}

int liblo10k1lf_dsp_config_set_patch_count(liblo10k1_file_dsp_setup_t *c, int count)
{		
	int i;
	/* alloc patches list */
	liblo10k1_dsp_patch_t **tmp = NULL;
	
	if (count > 0) {
		tmp = (liblo10k1_dsp_patch_t **)malloc(sizeof(liblo10k1_dsp_patch_t *) * count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
			
		memset(tmp, 0, sizeof(liblo10k1_dsp_patch_t *) * count);
	}
	
	if (c->patches) {
		for (i = 0; i < c->patch_count; i++) {
			if (c->patches[i])
				liblo10k1_patch_free(c->patches[i]);
		}
		free(c->patches);
	}
	
	c->patch_count = count;
	c->patches = tmp;
	return 0;
}
	
int liblo10k1lf_dsp_config_set_point_count(liblo10k1_file_dsp_setup_t *c, int count)
{		
	liblo10k1_point_info_t *tmp = NULL;
	
	if (count > 0) {
		tmp = (liblo10k1_point_info_t *)malloc(sizeof(liblo10k1_point_info_t) * count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
		
		memset(tmp, 0, sizeof(liblo10k1_point_info_t) * count);
	}
	
	if (c->points)
		free(c->points);
		
	c->point_count = count;
	c->points = tmp;
	return 0;	
}

int liblo10k1lf_get_dsp_config(liblo10k1_connection_t *conn, liblo10k1_file_dsp_setup_t **setup)
{
	liblo10k1_dsp_info_t info;
	int err;
	int i, j, k;
	
	liblo10k1_file_dsp_setup_t *s;
	liblo10k1_patches_info_t *plist;
	int pcount;
	int tmp;
	
	int *points;

	plist = NULL;
	points = NULL;
	s = liblo10k1lf_dsp_config_alloc();
	if (!s)
		return LD10K1_ERR_NO_MEM;
		
	/* get dsp type */
	if ((err = liblo10k1_get_dsp_info(conn, &info)) < 0)
		return err;
	
	s->dsp_type = LD10K1_FP_INFO_DSP_TYPE_EMU10K1;
	if (info.chip_type == CHIP_LIVE)
		s->dsp_type = LD10K1_FP_INFO_DSP_TYPE_EMU10K1;
	else if (info.chip_type == CHIP_AUDIGY)
		s->dsp_type = LD10K1_FP_INFO_DSP_TYPE_EMU10K2;

	/* now get everything what is needed */
	if ((err = liblo10k1_get_fx_count(conn, &tmp)) < 0)
		goto err;
		
	if ((err = liblo10k1lf_dsp_config_set_fx_count(s, tmp)) < 0)
		goto err;
	
	for (i = 0; i < s->fx_count; i++) {
		if ((err = liblo10k1_get_fx(conn, i, &(s->fxs[i]))) < 0)
			goto err;
	}
		
	if ((err = liblo10k1_get_in_count(conn, &tmp)) < 0)
		goto err;
	
	if ((err = liblo10k1lf_dsp_config_set_in_count(s, tmp)) < 0)
		goto err;
	
	for (i = 0; i < s->in_count; i++) {
		if ((err = liblo10k1_get_in(conn, i, &(s->ins[i]))) < 0)
			goto err;
	}	
		
	if ((err = liblo10k1_get_out_count(conn, &tmp)) < 0)
		goto err;
		
	if ((err = liblo10k1lf_dsp_config_set_out_count(s, tmp)) < 0)
		goto err;

	for (i = 0; i < s->out_count; i++) {
		if ((err = liblo10k1_get_out(conn, i, &(s->outs[i]))) < 0)
			goto err;
	}
	
	if ((err = liblo10k1_get_patches_info(conn, &plist, &pcount)) < 0)
		goto err;
		
	/* alloc patches list */
	if ((err = liblo10k1lf_dsp_config_set_patch_count(s, pcount)) < 0)
		goto err;
	
	for (i = 0; i < s->patch_count; i++) {
		if ((err = liblo10k1_patch_get(conn, plist[i].patch_num, &(s->patches[i]))) < 0)
			goto err;
	}
	
	if ((err = liblo10k1_get_points_info(conn, &points, &pcount)) < 0)
		goto err;

	if ((err = liblo10k1lf_dsp_config_set_point_count(s, pcount)) < 0)
		goto err;
	
	for (i = 0; i < s->point_count; i++) {
		if ((err = liblo10k1_get_point_info(conn, points[i], &(s->points[i]))) < 0)
			goto err;
		
		/* id to patch index */
		for (j = 0; j < s->points[i].conn_count;j++) {
			if (s->points[i].patch[j] >= 0) {
				for (k = 0; k < s->patch_count; k++) {
					if (plist[k].id == s->points[i].patch[j]) {
						s->points[i].patch[j] = k;
						break;
					}
				}

				if (s->points[i].patch[j] != k) {
					err = LD10K1_ERR_UNKNOWN_PATCH_NUM;
					goto err;
				}
			}
		}
	}

	free(plist);
	free(points);

	*setup = s;
	return 0;
err:
	if (plist)
		free(plist);
	if (points)
		free(points);
	
	liblo10k1lf_dsp_config_free(s);
	return err;
}

int liblo10k1lf_put_dsp_config(liblo10k1_connection_t *conn, liblo10k1_file_dsp_setup_t *setup)
{
	int err;
	int i, j;
	int loaded_id;
	int loaded;
	int *trans_nums;
	
	int tin_type, tout_type;
	int tin, tout;
	int tpin, tpout;
	
	int pnum;
	
	tin_type = 0;
	tout_type = 0;
	tin = 0;
	tout = 0;
	tpin = 0;
	tpout = 0;
	
	/* first initialize dsp */
	if ((err = liblo10k1_dsp_init(conn)) < 0)
		return err;
		
	for (i = 0; i < setup->fx_count; i++) {
		if ((err = liblo10k1_rename_fx(conn, i, setup->fxs[i].name)) < 0)
			return err;
	}
	
	for (i = 0; i < setup->in_count; i++) {
		if ((err = liblo10k1_rename_in(conn, i, setup->ins[i].name)) < 0)
			return err;
	}
	
	for (i = 0; i < setup->out_count; i++) {
		if ((err = liblo10k1_rename_out(conn, i, setup->outs[i].name)) < 0)
			return err;
	}
	
	if (setup->patch_count <= 0)
		return 0;
	
	trans_nums = (int *)malloc(sizeof(int) * setup->patch_count);
	if (!trans_nums)
		return LD10K1_ERR_NO_MEM;
	
	memset(trans_nums, 0, sizeof(int) * setup->patch_count);
	
	/* load all patches - remember ids */
	for (i = 0; i < setup->patch_count; i++) {
		if ((err = liblo10k1_patch_load(conn, setup->patches[i], -1, &loaded, &loaded_id)) < 0)
			goto err;
		trans_nums[i] = loaded;
	}
	
	/* connect all connections */
	for (i = 0; i < setup->point_count; i++) {
		if (setup->points[i].type == CON_IO_NORMAL) {
			/* find first pin */
			for (j = 0; j < setup->points[i].conn_count;j++)
				if (!setup->points[i].io_type[j]) {
					tin_type = CON_IO_PIN;
					tin = setup->points[i].io[j];
					tpin = setup->points[i].patch[j];
					if (tpin >= 0)
						tpin = trans_nums[tpin];
					break;
				}
				
			/* find first pout */
			for (j = 0; j < setup->points[i].conn_count;j++)
				if (setup->points[i].io_type[j]) {
					tout_type = CON_IO_POUT;
					tout = setup->points[i].io[j];
					tpout = setup->points[i].patch[j];
					if (tpout >= 0)
						tpout = trans_nums[tpout];
					break;
				}
						
			for (j = 0; j < setup->points[i].conn_count; j++) {
				pnum = setup->points[i].patch[j];
				if (pnum >= 0)
					pnum = trans_nums[pnum];
				if (!setup->points[i].io_type[j]) {
					if ((err = liblo10k1_con_add(conn, j == 0 ? 0 : setup->points[i].multi,
						setup->points[i].simple, CON_IO_PIN, pnum, setup->points[i].io[j],
						tout_type, tpout, tout, NULL)) < 0)
						goto err;
				} else {
					if ((err = liblo10k1_con_add(conn, j == 0 ? 0 : setup->points[i].multi,
						setup->points[i].simple, CON_IO_POUT, pnum, setup->points[i].io[j],
						tin_type, tpin, tin, NULL)) < 0)
						goto err;
				}
			}
		} else {
			tin_type = tout_type = setup->points[i].type;
			tin = tout = setup->points[i].io_idx;
			tpin = tpout = -1;
			
			for (j = 0; j < setup->points[i].conn_count; j++) {
				pnum = setup->points[i].patch[j];
				if (pnum >= 0)
					pnum = trans_nums[pnum];
				if ((err = liblo10k1_con_add(conn, j == 0 ? 0 : setup->points[i].multi,
					setup->points[i].simple, setup->points[i].io_type[j] ? CON_IO_POUT : CON_IO_PIN, pnum, setup->points[i].io[j],
					tin_type, tpin, tin, NULL)) < 0)
					goto err;
			}
		}
	}
	
	return 0;
err:
	if (trans_nums)
		free(trans_nums);
	return err;
}

int liblo10k1lf_skip_part(FILE *file, liblo10k1_file_part_t *part)
{
	char tmp_char;
	int i;
	int err;
	int found_end_part = 0;
	
	if (part->part_type == LD10K1_FP_TYPE_NORMAL) {
		/* read all data */
		for (i = 0; i < part->part_length; i++)
			if (fread(&tmp_char, 1, 1, file) != 1)
				return LD10K1_LF_ERR_READ;
	} else if (part->part_type == LD10K1_FP_TYPE_END) {
		return 0;
	} else {	
		while (!found_end_part) {
			/* read next part */
			if (fread(part, sizeof(liblo10k1_file_part_t), 1, file) != 1)
				return LD10K1_LF_ERR_READ;
				
			/* check type & id */
			if (part->part_type == LD10K1_FP_TYPE_END)
				found_end_part = 1;
			else {
				if ((err = liblo10k1lf_skip_part(file, part)) < 0)
					return err;
			}
		}
	}
	return 0;
}

int liblo10k1lf_find_part_il(FILE *file, unsigned int part_type, unsigned int part_id, unsigned int part_length, int il, liblo10k1_file_part_t *part)
{
	int err;
	while (1) {
		if (fread(part, sizeof(liblo10k1_file_part_t), 1, file) != 1)
			return LD10K1_LF_ERR_READ;
			
		/* check type & id */
		if (part->part_type == part_type && part->part_id == part_id) {
			if (part->part_type == LD10K1_FP_TYPE_NORMAL) {
				if (il || part->part_length == part_length)
					return 0;
				else
					return LD10K1_LF_ERR_PART_SIZE;
			} else {
				if (part->part_length != 0)
					return LD10K1_LF_ERR_PART_SIZE;
				else
					return 0;
			}
		} else {
			if ((err = liblo10k1lf_skip_part(file, part)) < 0)
				return err;
		}
	}
}

int liblo10k1lf_find_part(FILE *file, unsigned int part_type, unsigned int part_id, unsigned int part_length, liblo10k1_file_part_t *part)
{
	return liblo10k1lf_find_part_il(file, part_type, part_id, part_length, 0, part);
}

int liblo10k1lf_find_part_ws(FILE *file, unsigned int part_id, unsigned int part_length, liblo10k1_file_part_t *part)
{
	return liblo10k1lf_find_part(file, LD10K1_FP_TYPE_NORMAL, part_id, part_length, part);
}

int liblo10k1lf_find_part_start(FILE *file, unsigned int part_id)
{
	liblo10k1_file_part_t part;
	return liblo10k1lf_find_part(file, LD10K1_FP_TYPE_START, part_id, 0, &part);
}

int liblo10k1lf_find_part_end(FILE *file, unsigned int part_id)
{
	liblo10k1_file_part_t part;
	return liblo10k1lf_find_part(file, LD10K1_FP_TYPE_END, part_id, 0, &part);
}

int liblo10k1lf_find_load_part_ws(FILE *file, unsigned int part_id, unsigned int part_length, void *where)
{
	int err;
	liblo10k1_file_part_t part;
	
	if ((err = liblo10k1lf_find_part_ws(file, part_id, part_length, &part)) < 0)
		return err;
		
	if (fread(where, part_length, 1, file) != 1)
		return LD10K1_LF_ERR_READ;
		
	return 0;
}

int liblo10k1lf_can_load_file(FILE *file, unsigned int ft)
{
	liblo10k1_file_header_t fhdr;
	liblo10k1_file_part_info_t file_info;
	int err;
	
	if (fread(&fhdr, sizeof(liblo10k1_file_header_t), 1, file) != 1)
		return LD10K1_LF_ERR_READ;
		
	/* check signature */
	if (strcmp(fhdr.signature, LD10K1_FILE_SIGNATURE) != 0)
		return LD10K1_LF_ERR_SIGNATURE;
	
	/* now load file info part & check version */
	if ((err = liblo10k1lf_find_load_part_ws(file, LD10K1_FP_INFO, sizeof(file_info), &file_info)) < 0)
		return err;
		
	if (file_info.minimal_reader_version_major > CREATER_MAJOR)
		return LD10K1_LF_ERR_VERSION;
		
	if (file_info.minimal_reader_version_major == CREATER_MAJOR &&
		file_info.minimal_reader_version_minor > CREATER_MINOR)
		return LD10K1_LF_ERR_VERSION;
		
	if (file_info.minimal_reader_version_major == CREATER_MAJOR &&
		file_info.minimal_reader_version_minor == CREATER_MINOR &&
		file_info.minimal_reader_version_subminor > CREATER_SUBMINOR)
		return LD10K1_LF_ERR_VERSION;
	
	/* check file type */
	if (file_info.file_type != ft)
		return LD10K1_LF_ERR_FILE_TYPE;
	
	return 0;
}

int liblo10k1lf_load_io(liblo10k1_get_io_t *ios, int count, int ptl, int pt, FILE *file)
{
	int i, err;
	
	/* io list start */
	if ((err = liblo10k1lf_find_part_start(file, ptl)) < 0)
		return err;
		
	for (i = 0; i < count; i++) {
		if ((err = liblo10k1lf_find_load_part_ws(file, pt, sizeof(liblo10k1_get_io_t), &(ios[i]))) < 0)
			return err;
	}
		
	if ((err = liblo10k1lf_find_part_end(file, ptl)) < 0)
		return err;
		
	return 0;
}

int liblo10k1lf_load_points(liblo10k1_point_info_t *points, int count, FILE *file)
{
	int i, err;
	/* io list start */
	if ((err = liblo10k1lf_find_part_start(file, LD10K1_FP_POINT_LIST)) < 0)
		return err;
		
	for (i = 0; i < count; i++) {
		if ((err = liblo10k1lf_find_load_part_ws(file, LD10K1_FP_POINT, sizeof(liblo10k1_point_info_t), &(points[i]))) < 0)
			return err;
	}
		
	if ((err = liblo10k1lf_find_part_end(file, LD10K1_FP_POINT_LIST)) < 0)
		return err;
		
	return 0;
}

int liblo10k1lf_load_pio(liblo10k1_dsp_pio_t *ios, int count, int ptl, int pt, FILE *file)
{
	int i, err;
	/* io list start */
	if ((err = liblo10k1lf_find_part_start(file, ptl)) < 0)
		return err;
		
	for (i = 0; i < count; i++) {
		if ((err = liblo10k1lf_find_load_part_ws(file, pt, sizeof(liblo10k1_dsp_pio_t), &(ios[i]))) < 0)
			return err;
	}
		
	if ((err = liblo10k1lf_find_part_end(file, ptl)) < 0)
		return err;
	
	return 0;
}

int liblo10k1lf_load_cs(liblo10k1_dsp_cs_t *css, int count, int ptl, int pt, FILE *file)
{
	int i, err;
	/* io list start */
	if ((err = liblo10k1lf_find_part_start(file, ptl)) < 0)
		return err;
		
	for (i = 0; i < count; i++) {
		if ((err = liblo10k1lf_find_load_part_ws(file,  pt, sizeof(liblo10k1_dsp_cs_t), &(css[i]))) < 0)
			return err;
	}
		
	if ((err = liblo10k1lf_find_part_end(file, ptl)) < 0)
		return err;
	
	return 0;
}

int liblo10k1lf_load_hw(liblo10k1_dsp_hw_t *hws, int count, FILE *file)
{
	int i, err;
	/* io list start */
	if ((err = liblo10k1lf_find_part_start(file, LD10K1_FP_HW_LIST)) < 0)
		return err;
		
	for (i = 0; i < count; i++) {
		if ((err = liblo10k1lf_find_load_part_ws(file, LD10K1_FP_HW, sizeof(liblo10k1_dsp_hw_t), &(hws[i]))) < 0)
			return err;
	}
		
	if ((err = liblo10k1lf_find_part_end(file, LD10K1_FP_HW_LIST)) < 0)
		return err;

	return 0;
}

int liblo10k1lf_load_tram(liblo10k1_dsp_tram_grp_t *trams, int count, FILE *file)
{
	int i, err;
	/* io list start */
	if ((err = liblo10k1lf_find_part_start(file, LD10K1_FP_TRAM_LIST)) < 0)
		return err;
		
	for (i = 0; i < count; i++) {
		if ((err = liblo10k1lf_find_load_part_ws(file, LD10K1_FP_TRAM, sizeof(liblo10k1_dsp_tram_grp_t), &(trams[i]))) < 0)
			return err;
	}
		
	if ((err = liblo10k1lf_find_part_end(file, LD10K1_FP_TRAM_LIST)) < 0)
		return err;
		
	return 0;
}

int liblo10k1lf_load_tram_acc(liblo10k1_dsp_tram_acc_t *tram_accs, int count, FILE *file)
{
	int i, err;
	/* io list start */
	if ((err = liblo10k1lf_find_part_start(file, LD10K1_FP_TRAM_ACC_LIST)) < 0)
		return err;
		
	for (i = 0; i < count; i++) {
		if ((err = liblo10k1lf_find_load_part_ws(file, LD10K1_FP_TRAM_ACC, sizeof(liblo10k1_dsp_tram_acc_t), &(tram_accs[i]))) < 0)
			return err;
	}
		
	if ((err = liblo10k1lf_find_part_end(file, LD10K1_FP_TRAM_ACC_LIST)) < 0)
		return err;
		
	return 0;
}

int liblo10k1lf_load_ctl(liblo10k1_dsp_ctl_t *ctls, int count, FILE *file)
{
	int i, err;
	/* io list start */
	if ((err = liblo10k1lf_find_part_start(file, LD10K1_FP_CTL_LIST)) < 0)
		return err;
		
	for (i = 0; i < count; i++) {
		if ((err = liblo10k1lf_find_load_part_ws(file, LD10K1_FP_CTL, sizeof(liblo10k1_dsp_ctl_t), &(ctls[i]))) < 0)
			return err;
	}
		
	if ((err = liblo10k1lf_find_part_end(file, LD10K1_FP_CTL_LIST)) < 0)
		return err;
		
	return 0;
}

int liblo10k1lf_load_instr(liblo10k1_dsp_instr_t *instrs, int count, FILE *file)
{
	int i, err;
	/* io list start */
	if ((err = liblo10k1lf_find_part_start(file, LD10K1_FP_INSTR_LIST)) < 0)
		return err;
		
	for (i = 0; i < count; i++) {
		if ((err = liblo10k1lf_find_load_part_ws(file, LD10K1_FP_INSTR, sizeof(liblo10k1_dsp_instr_t), &(instrs[i]))) < 0)
			return err;
	}
		
	if ((err = liblo10k1lf_find_part_end(file, LD10K1_FP_INSTR_LIST)) < 0)
		return err;
		
	return 0;
}

int liblo10k1lf_load_patch(liblo10k1_dsp_patch_t **p, FILE *file)
{
	int err;
	liblo10k1_file_patch_info_t pinfo;
	liblo10k1_dsp_patch_t *patch = NULL;
	
	/* io list start */
	if ((err = liblo10k1lf_find_part_start(file, LD10K1_FP_PATCH)) < 0)
		return err;
		
	/* patch info */
	if ((err = liblo10k1lf_find_load_part_ws(file, LD10K1_FP_PATCH_INFO, sizeof(liblo10k1_file_patch_info_t), &pinfo)) < 0)
		return err;
	
	patch = liblo10k1_patch_alloc(pinfo.in_count, pinfo.out_count, pinfo.const_count, pinfo.sta_count, pinfo.dyn_count, 
		pinfo.hw_count, pinfo.tram_count, pinfo.tram_acc_count, pinfo.ctl_count, pinfo.instr_count);
	if (!patch) {
		err = LD10K1_ERR_NO_MEM;
		goto err;
	}
	
	strcpy(patch->patch_name, pinfo.patch_name);
	
	/* pins */
	if ((err = liblo10k1lf_load_pio(patch->ins, patch->in_count, LD10K1_FP_PIN_LIST, LD10K1_FP_PIO, file)) < 0)
		return err;
	/* pouts */
	if ((err = liblo10k1lf_load_pio(patch->outs, patch->out_count, LD10K1_FP_POUT_LIST, LD10K1_FP_PIO, file)) < 0)
		return err;
	/* consts */
	if ((err = liblo10k1lf_load_cs(patch->consts, patch->const_count, LD10K1_FP_CONST_LIST, LD10K1_FP_CS, file)) < 0)
		return err;
	/* stas */
	if ((err = liblo10k1lf_load_cs(patch->stas, patch->sta_count, LD10K1_FP_STA_LIST, LD10K1_FP_CS, file)) < 0)
		return err;
	/* hws */
	if ((err = liblo10k1lf_load_hw(patch->hws, patch->hw_count, file)) < 0)
		return err;
	/* trams */
	if ((err = liblo10k1lf_load_tram(patch->tram, patch->tram_count, file)) < 0)
		return err;
	/* tram_accs */
	if ((err = liblo10k1lf_load_tram_acc(patch->tram_acc, patch->tram_acc_count, file)) < 0)
		return err;
	/* ctls */
	if ((err = liblo10k1lf_load_ctl(patch->ctl, patch->ctl_count, file)) < 0)
		return err;
	/* instrs */
	if ((err = liblo10k1lf_load_instr(patch->instr, patch->instr_count, file)) < 0)
		return err;
		
	if ((err = liblo10k1lf_find_part_end(file, LD10K1_FP_PATCH)) < 0)
		return err;
		
	*p = patch;
	return 0;
err:
	if (patch)
		liblo10k1_patch_free(patch);
	return err;	
}

int liblo10k1lf_load_dsp_setup(liblo10k1_file_dsp_setup_t **c, FILE *file)
{
	liblo10k1_file_part_dsp_setup_t setup;
	int err;
	int i;
	
	liblo10k1_file_dsp_setup_t *cfg;
	
	if ((err = liblo10k1lf_find_load_part_ws(file, LD10K1_FP_DSP_SETUP, sizeof(setup), &setup)) < 0)
		return err;
	
	cfg = liblo10k1lf_dsp_config_alloc();

	cfg->dsp_type = setup.dsp_type;
	
	/* alloc space */
	if ((err = liblo10k1lf_dsp_config_set_fx_count(cfg, setup.fx_count)) < 0)
		goto err;
	if ((err = liblo10k1lf_dsp_config_set_in_count(cfg, setup.in_count)) < 0)
		goto err;
	if ((err = liblo10k1lf_dsp_config_set_out_count(cfg, setup.out_count)) < 0)
		goto err;
	if ((err = liblo10k1lf_dsp_config_set_patch_count(cfg, setup.patch_count)) < 0)
		goto err;
	if ((err = liblo10k1lf_dsp_config_set_point_count(cfg, setup.point_count)) < 0)
		goto err;
			
	/* load fx */
	if ((err = liblo10k1lf_load_io(cfg->fxs, cfg->fx_count, LD10K1_FP_FX_LIST, LD10K1_FP_FX, file)) < 0)
		return err;
		
	/* load in */
	if ((err = liblo10k1lf_load_io(cfg->ins, cfg->in_count, LD10K1_FP_IN_LIST, LD10K1_FP_IN, file)) < 0)
		return err;
		
	/* load out */
	if ((err = liblo10k1lf_load_io(cfg->outs, cfg->out_count, LD10K1_FP_OUT_LIST, LD10K1_FP_OUT, file)) < 0)
		return err;
	
	/* load patches */	
	if ((err = liblo10k1lf_find_part_start(file, LD10K1_FP_PATCH_LIST)) < 0)
		return err;
		
	for (i = 0; i < cfg->patch_count; i++) {
		if ((err = liblo10k1lf_load_patch(&(cfg->patches[i]), file)) < 0)
			return err;
	}
	
	if ((err = liblo10k1lf_find_part_end(file, LD10K1_FP_PATCH_LIST)) < 0)
		return err;
	
	
	/* load points */
	if ((err = liblo10k1lf_load_points(cfg->points, cfg->point_count, file)) < 0)
		return err;
	
	*c = cfg;
	return 0;
err:
	if (cfg)
		liblo10k1lf_dsp_config_free(cfg);
	return err;
}

int liblo10k1lf_load_dsp_config(liblo10k1_file_dsp_setup_t **c, char *file_name, liblo10k1_file_info_t **fi)
{
	FILE *file = NULL;
	int err;
	
	liblo10k1_file_info_t *i = NULL;
	
	file = fopen(file_name, "r");
	if (!file)
		return LD10K1_LF_ERR_OPEN;
		
	if ((err = liblo10k1lf_can_load_file(file, LD10K1_FP_INFO_FILE_TYPE_DSP_SETUP)) < 0)
		goto err;
		
	if ((err =  liblo10k1lf_load_file_info(file, &i)) < 0)
		goto err;
		
	if ((err =  liblo10k1lf_load_dsp_setup(c, file)) < 0)
		goto err;
		
	*fi = i;
	fclose(file);
	return 0;
err:
	if (i)
		liblo10k1lf_file_info_free(i);
	fclose(file);
	return err;
}

liblo10k1_file_info_t *liblo10k1lf_file_info_alloc()
{
	liblo10k1_file_info_t *tmp = (liblo10k1_file_info_t *)malloc(sizeof(liblo10k1_file_info_t));
	if (tmp)
		memset(tmp, 0, sizeof(liblo10k1_file_info_t));
	return tmp;
}

void liblo10k1lf_file_info_free(liblo10k1_file_info_t *fi)
{
	if (fi->name)
		free(fi->name);
	if (fi->desc)
		free(fi->desc);
	if (fi->creater)
		free(fi->creater);
	if (fi->author)
		free(fi->author);
	if (fi->copyright)
		free(fi->copyright);
	if (fi->license)
		free(fi->license);
}

int liblo10k1lf_save_dsp_patch(liblo10k1_dsp_patch_t *p, char *file_name, liblo10k1_file_info_t *fi)
{
	FILE *file = NULL;
	int err;
	
	file = fopen(file_name, "w");
	if (!file)
		return LD10K1_LF_ERR_OPEN;
		
	if ((err = liblo10k1lf_save_file_header(file, LD10K1_FP_INFO_FILE_TYPE_PATCH)) < 0)
		goto err;
		
	if ((err = liblo10k1lf_save_file_info(file, fi)) < 0)
		goto err;
		
	if ((err =  liblo10k1lf_save_patch(p, file)) < 0)
		goto err;
		
	fclose(file);
	return 0;
err:
	fclose(file);
	return err;
}

int liblo10k1lf_load_dsp_patch(liblo10k1_dsp_patch_t **p, char *file_name, liblo10k1_file_info_t **fi)
{
	FILE *file = NULL;
	int err;
	
	liblo10k1_file_info_t *i = NULL;
	
	file = fopen(file_name, "r");
	if (!file)
		return LD10K1_LF_ERR_OPEN;
		
	if ((err = liblo10k1lf_can_load_file(file, LD10K1_FP_INFO_FILE_TYPE_PATCH)) < 0)
		goto err;
		
	if ((err =  liblo10k1lf_load_file_info(file, &i)) < 0)
		goto err;
		
	if ((err =  liblo10k1lf_load_patch(p, file)) < 0)
		goto err;
		
	*fi = i;
	fclose(file);
	return 0;
err:
	if (i)
		liblo10k1lf_file_info_free(i);
	fclose(file);
	return err;
}
