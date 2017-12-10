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
 
#ifndef __LIBLO10K1EF_H
#define __LIBLO10K1EF_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	unsigned int sc;
	unsigned int sc_val;
} liblo10k1_emu_sc_t;

typedef struct {
	unsigned int ctl;
	unsigned int ctl_val;
	unsigned int ctl_val_min;
	unsigned int ctl_val_max;
	char ctl_name[32];
} liblo10k1_emu_ctl_t;

typedef struct {
	unsigned int line;
	unsigned int line_size;
} liblo10k1_emu_tram_line_t;

typedef struct {
	unsigned int size;
	unsigned int read_line_count;
	liblo10k1_emu_tram_line_t *read_lines;
	unsigned int write_line_count;
	liblo10k1_emu_tram_line_t *write_lines;
} liblo10k1_emu_tram_t;

typedef struct {
	unsigned int op;
	unsigned int arg[4];
} liblo10k1_emu_instr_t;

typedef struct {
	char patch_name[32];
	unsigned int in_count;
	unsigned int *ins;
	unsigned int out_count;
	unsigned int *outs;
	unsigned int dyn_count;
	unsigned int *dyns;
	unsigned int sta_count;
	liblo10k1_emu_sc_t *stas;
	unsigned int ctl_count;
	liblo10k1_emu_ctl_t *ctls;
	unsigned int con_count;
	liblo10k1_emu_sc_t *cons;
	unsigned int tram_lookup_count;
	liblo10k1_emu_tram_t *tram_lookups;
	unsigned int tram_delay_count;
	liblo10k1_emu_tram_t *tram_delays;
	unsigned int instr_count;
	liblo10k1_emu_instr_t *instrs;
} liblo10k1_emu_patch_t;

liblo10k1_emu_patch_t *liblo10k1_emu_new_patch();
void liblo10k1_emu_free_patch(liblo10k1_emu_patch_t *p);

int liblo10k1_emu_patch_set_in_count(liblo10k1_emu_patch_t *p, int count);
int liblo10k1_emu_patch_set_out_count(liblo10k1_emu_patch_t *p, int count);
int liblo10k1_emu_patch_set_dyn_count(liblo10k1_emu_patch_t *p, int count);
int liblo10k1_emu_patch_set_sta_count(liblo10k1_emu_patch_t *p, int count);
int liblo10k1_emu_patch_set_ctl_count(liblo10k1_emu_patch_t *p, int count);
int liblo10k1_emu_patch_set_con_count(liblo10k1_emu_patch_t *p, int count);
int liblo10k1_emu_patch_set_lookup_count(liblo10k1_emu_patch_t *p, int count);
int liblo10k1_emu_patch_set_delay_count(liblo10k1_emu_patch_t *p, int count);
int liblo10k1_emu_patch_set_instr_count(liblo10k1_emu_patch_t *p, int count);

int liblo10k1_emu_patch_set_line_count(liblo10k1_emu_tram_t *t, int write, int count);
int liblo10k1_emu_load_patch(char *file_name, liblo10k1_emu_patch_t **p);

typedef struct
{
	int emu_ctls[32];
	int emu_ctl_count;
	char ctl_name[44];
} liblo10k1_ctl_transform_t;

int liblo10k1_emu_transform_patch(liblo10k1_emu_patch_t *ep,  liblo10k1_ctl_transform_t *tctl, int tctl_count, liblo10k1_dsp_patch_t **lp);
int liblo10k1_patch_find_ctl_by_name(liblo10k1_dsp_patch_t *p, char *ctl_name);
int liblo10k1_patch_ctl_set_trans(liblo10k1_dsp_patch_t *p, int idx, int trans);
int liblo10k1_patch_ctl_set_vcount(liblo10k1_dsp_patch_t *p, int idx, int vc);
int liblo10k1_patch_ctl_set_index(liblo10k1_dsp_patch_t *p, int idx, int i);
int liblo10k1_patch_ctl_set_value(liblo10k1_dsp_patch_t *p, int idx, int vi, int val);


#define LD10K1_EF_ERR_OPEN -1000 /* error at file open */
#define LD10K1_EF_ERR_STAT -1001 /* error at file stat */
#define LD10K1_EF_ERR_SIZE -1002 /* wrong file size */
#define LD10K1_EF_ERR_READ -1003 /* error at file read */
#define LD10K1_EF_ERR_SIGNATURE -1004 /* wrong file signature */
#define LD10K1_EF_ERR_FORMAT -1005 /* wrong file format */

#define LD10K1_EF_ERR_TRANSFORM_CTL -1100 /* wrong ctl transformation */
#define LD10K1_EF_ERR_TRANSFORM -1101 /* wrong transformation */
#define LD10K1_EF_ERR_TRANSFORM_TRANS -1102 /* wrong ctl translation */

#ifdef __cplusplus
}
#endif

#endif /* __LIBLO10K1EF_H */
