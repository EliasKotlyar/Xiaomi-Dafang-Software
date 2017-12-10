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
 
#ifndef __LIBLO10K1LF_H
#define __LIBLO10K1LF_H

#ifdef __cplusplus
extern "C" {
#endif

/* description of structures used in native ld10k1 files */

typedef struct {
	/* asciz string with signature - have to be 
	"LD10K1 NATIVE EFFECT FILE      "
	 01234567890123456789012345678901 */
	char signature[32];
	/* don't use this */
	char reserved[32];
} liblo10k1_file_header_t;

/* don't use this */
#define LD10K1_FP_TYPE_RESERVED 0
/* normal part type, part_length is valid */
#define LD10K1_FP_TYPE_NORMAL 1
/* part start type, part_length have to be 0 */
#define LD10K1_FP_TYPE_START 2
/* part end type, part_length have to be 0 */
#define LD10K1_FP_TYPE_END 3

/* parts can be nested */
typedef struct {
	/* don't use this */
	unsigned int reserved : 2,
	/* part type definition
		- normal
		- part content start
		- part content end */
		part_type : 6,
		part_id : 24;
	/* part data length */
	unsigned int part_length;
} liblo10k1_file_part_t;

#define LD10K1_FP_INFO 0
#define LD10K1_FP_DSP_SETUP 1
#define LD10K1_FP_FX 2
#define LD10K1_FP_FX_LIST 3
#define LD10K1_FP_IN 4
#define LD10K1_FP_IN_LIST 5
#define LD10K1_FP_OUT 6
#define LD10K1_FP_OUT_LIST 7
#define LD10K1_FP_PATCH 8
#define LD10K1_FP_PATCH_LIST 9
#define LD10K1_FP_PATCH_INFO 10
#define LD10K1_FP_PIO 11
#define LD10K1_FP_PIN_LIST 12
#define LD10K1_FP_POUT_LIST 13
#define LD10K1_FP_CS 14
#define LD10K1_FP_CONST_LIST 15
#define LD10K1_FP_STA_LIST 16
#define LD10K1_FP_DYN_LIST 17
#define LD10K1_FP_HW 18
#define LD10K1_FP_HW_LIST 19
#define LD10K1_FP_TRAM 20
#define LD10K1_FP_TRAM_LIST 21
#define LD10K1_FP_TRAM_ACC 22
#define LD10K1_FP_TRAM_ACC_LIST 23
#define LD10K1_FP_CTL 24
#define LD10K1_FP_CTL_LIST 25
#define LD10K1_FP_INSTR 26
#define LD10K1_FP_INSTR_LIST 27
#define LD10K1_FP_POINT 28
#define LD10K1_FP_POINT_LIST 29

#define LD10K1_FP_FILE_INFO_NAME 30
#define LD10K1_FP_FILE_INFO_DESC 31
#define LD10K1_FP_FILE_INFO_CREATER 32
#define LD10K1_FP_FILE_INFO_AUTHOR 33
#define LD10K1_FP_FILE_INFO_COPYRIGHT 34
#define LD10K1_FP_FILE_INFO_LICENCE 35

/* file contains whole dsp config */
#define LD10K1_FP_INFO_FILE_TYPE_DSP_SETUP 0
/* file contains only 1 patch */
#define LD10K1_FP_INFO_FILE_TYPE_PATCH 1


typedef struct {
	unsigned int file_type;
	/* file version 
	   application can ignore this version and read file, 
	   but must be prepared to ignore unknown parts */
	unsigned int file_version_major : 8,
		file_version_minor : 8,
		file_version_subminor : 8,
		file_version_pad : 8;
	/* minimal version of lo10k1/ld10k1 which will load file
	   application must be prepared to ignore unknown parts */
	unsigned int minimal_reader_version_major : 8,
		minimal_reader_version_minor : 8,
		minimal_reader_version_subminor : 8,
		minimal_reader_version_pad : 8;
	/* version of lo10k1/ld10k1 which which created file */
	unsigned int creater_version_major : 8,
		creater_version_minor : 8,
		creater_version_subminor : 8,
		creater_version_pad : 8;
} liblo10k1_file_part_info_t;

#define LD10K1_FP_INFO_DSP_TYPE_EMU10K1 0
#define LD10K1_FP_INFO_DSP_TYPE_EMU10K2 1
typedef struct {
	unsigned int dsp_type;
	/* tram size to setup */
	/* unsigned int externa_tram_size; */
	/* used to restore io names */
	unsigned int fx_count;
	unsigned int in_count;
	unsigned int out_count;
	/* patch count contained in this file */
	unsigned int patch_count;
	/* point count contained in this file */
	unsigned int point_count;
} liblo10k1_file_part_dsp_setup_t;


/* structure used to store and restore config */
typedef struct {
	unsigned int dsp_type;
	/* used to restore io names */
	unsigned int fx_count;
	liblo10k1_get_io_t *fxs;
	unsigned int in_count;
	liblo10k1_get_io_t *ins;
	unsigned int out_count;
	liblo10k1_get_io_t *outs;
	/* patch count contained in this file */
	unsigned int patch_count;
	liblo10k1_dsp_patch_t **patches;
	/* point count contained in this file */
	unsigned int point_count;
	liblo10k1_point_info_t *points;
} liblo10k1_file_dsp_setup_t;

typedef struct {
	char patch_name[MAX_NAME_LEN];
	unsigned int in_count;
	unsigned int out_count;
	unsigned int const_count;
	unsigned int sta_count;
	unsigned int dyn_count;
	unsigned int hw_count;
	unsigned int tram_count;
	unsigned int tram_acc_count;
	unsigned int ctl_count;
	unsigned int instr_count;
} liblo10k1_file_patch_info_t;

int liblo10k1lf_get_dsp_config(liblo10k1_connection_t *conn, liblo10k1_file_dsp_setup_t **setup);
int liblo10k1lf_put_dsp_config(liblo10k1_connection_t *conn, liblo10k1_file_dsp_setup_t *setup);

liblo10k1_file_dsp_setup_t *liblo10k1lf_dsp_config_alloc();
void liblo10k1lf_dsp_config_free(liblo10k1_file_dsp_setup_t *c);

typedef struct {
	/* file name */
	char *name;
	/* file description */
	char *desc;
	/* description of creater application - can be anything */
	char *creater;
	/* author */
	char *author;
	/* copyright string */
	char *copyright;
	/* licence use for this file */
	char *license;
} liblo10k1_file_info_t;

liblo10k1_file_info_t *liblo10k1lf_file_info_alloc();
void liblo10k1lf_file_info_free(liblo10k1_file_info_t *fi);

int liblo10k1lf_save_dsp_config(liblo10k1_file_dsp_setup_t *c, char *file_name, liblo10k1_file_info_t *fi);
int liblo10k1lf_load_dsp_config(liblo10k1_file_dsp_setup_t **c, char *file_name, liblo10k1_file_info_t **fi);

int liblo10k1lf_save_dsp_patch(liblo10k1_dsp_patch_t *p, char *file_name, liblo10k1_file_info_t *fi);
int liblo10k1lf_load_dsp_patch(liblo10k1_dsp_patch_t **p, char *file_name, liblo10k1_file_info_t **fi);

#define LD10K1_LF_ERR_OPEN -2000
#define LD10K1_LF_ERR_WRITE -2001
#define LD10K1_LF_ERR_READ -2002
#define LD10K1_LF_ERR_SIGNATURE -2003
#define LD10K1_LF_ERR_PART_TYPE -2004
#define LD10K1_LF_ERR_PART_SIZE -2005
#define LD10K1_LF_ERR_VERSION -2006
#define LD10K1_LF_ERR_FILE_TYPE -2007

#ifdef __cplusplus
}
#endif

#endif /* __LIBLO10K1LF_H */
