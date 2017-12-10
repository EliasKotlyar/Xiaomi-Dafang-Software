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
 
#ifndef __LIBLO10K1_H
#define __LIBLO10K1_H

#ifdef __cplusplus
extern "C" {
#endif

#define LIBLO10K1_ERR_DEBUG -10000

typedef ld10k1_dsp_p_in_out_t liblo10k1_dsp_pio_t;
typedef ld10k1_dsp_p_const_static_t liblo10k1_dsp_cs_t;
typedef ld10k1_dsp_p_hw_t liblo10k1_dsp_hw_t;
typedef ld10k1_dsp_ctl_t liblo10k1_dsp_ctl_t;
typedef ld10k1_dsp_instr_t liblo10k1_dsp_instr_t;
typedef ld10k1_dsp_tram_grp_t liblo10k1_dsp_tram_grp_t;
typedef ld10k1_dsp_tram_acc_t liblo10k1_dsp_tram_acc_t;
typedef ld10k1_fnc_get_io_t liblo10k1_get_io_t;

typedef struct {
	char patch_name[MAX_NAME_LEN];
	unsigned int in_count;
	liblo10k1_dsp_pio_t *ins;
	unsigned int out_count;
	liblo10k1_dsp_pio_t *outs;
	unsigned int const_count;
	liblo10k1_dsp_cs_t *consts;
	unsigned int sta_count;
	liblo10k1_dsp_cs_t *stas;
	unsigned int dyn_count;
	unsigned int hw_count;
	liblo10k1_dsp_hw_t *hws;
	unsigned int tram_count;
	liblo10k1_dsp_tram_grp_t *tram;
	unsigned int tram_acc_count;
	liblo10k1_dsp_tram_acc_t *tram_acc;
	unsigned int ctl_count;
	liblo10k1_dsp_ctl_t *ctl;
	unsigned int instr_count;
	liblo10k1_dsp_instr_t *instr;
} liblo10k1_dsp_patch_t;

typedef int liblo10k1_connection_t;
typedef ld10k1_fnc_patches_info_t liblo10k1_patches_info_t;

typedef ld10k1_fnc_dsp_info_t liblo10k1_dsp_info_t;

typedef ld10k1_dsp_point_t liblo10k1_point_info_t;

typedef comm_param liblo10k1_param;

void liblo10k1_connection_init(liblo10k1_connection_t *conn);
int liblo10k1_connect(liblo10k1_param *param, liblo10k1_connection_t *conn);
int liblo10k1_is_open(liblo10k1_connection_t *conn);
int liblo10k1_disconnect(liblo10k1_connection_t *conn);

liblo10k1_dsp_patch_t *liblo10k1_patch_alloc(int in_count, int out_count, int const_count, int sta_count, int dyn_count, int hw_count, int tram_count, int tram_acc_count, int ctl_count, int instr_count);
void liblo10k1_patch_free(liblo10k1_dsp_patch_t *patch);

int liblo10k1_patch_set_in_count(liblo10k1_dsp_patch_t *p, int count);
int liblo10k1_patch_set_out_count(liblo10k1_dsp_patch_t *p, int count);
int liblo10k1_patch_set_const_count(liblo10k1_dsp_patch_t *p, int count);
int liblo10k1_patch_set_sta_count(liblo10k1_dsp_patch_t *p, int count);
int liblo10k1_patch_set_dyn_count(liblo10k1_dsp_patch_t *p, int count);
int liblo10k1_patch_set_hw_count(liblo10k1_dsp_patch_t *p, int count);
int liblo10k1_patch_set_tram_count(liblo10k1_dsp_patch_t *p, int count);
int liblo10k1_patch_set_tram_acc_count(liblo10k1_dsp_patch_t *p, int count);
int liblo10k1_patch_set_ctl_count(liblo10k1_dsp_patch_t *p, int count);
int liblo10k1_patch_set_instr_count(liblo10k1_dsp_patch_t *p, int count);

int liblo10k1_patch_load(liblo10k1_connection_t *conn, liblo10k1_dsp_patch_t *patch, int before, int *loaded, int *loaded_id);
int liblo10k1_patch_unload(liblo10k1_connection_t *conn, int patch_num);
int liblo10k1_patch_get(liblo10k1_connection_t *conn, int patch_num, liblo10k1_dsp_patch_t **patch);

int liblo10k1_debug(liblo10k1_connection_t *conn, int deb, void (*prn_fnc)(char *));

int liblo10k1_dsp_init(liblo10k1_connection_t *conn);

int liblo10k1_find_patch(liblo10k1_connection_t *conn, char *patch_name, int *out);
int liblo10k1_find_fx(liblo10k1_connection_t *conn, char *fx_name, int *out);
int liblo10k1_find_in(liblo10k1_connection_t *conn, char *in_name, int *out);
int liblo10k1_find_out(liblo10k1_connection_t *conn, char *out_name, int *out);
int liblo10k1_find_patch_in(liblo10k1_connection_t *conn, int patch_num, char *patch_in_name, int *out);
int liblo10k1_find_patch_out(liblo10k1_connection_t *conn, int patch_num, char *patch_in_name, int *out);

int liblo10k1_rename_patch(liblo10k1_connection_t *conn, int patch_num, char *patch_name);
int liblo10k1_rename_fx(liblo10k1_connection_t *conn, int fx, char *fx_name);
int liblo10k1_rename_in(liblo10k1_connection_t *conn, int in, char *in_name);
int liblo10k1_rename_out(liblo10k1_connection_t *conn, int out, char *out_name);
int liblo10k1_rename_patch_in(liblo10k1_connection_t *conn, int patch_num, int in, char *patch_in_name);
int liblo10k1_rename_patch_out(liblo10k1_connection_t *conn, int patch_num, int out, char *patch_out_name);

int liblo10k1_con_add(liblo10k1_connection_t *conn, int multi, int simple, int from_type, int from_patch, int from_io, int to_type, int to_patch, int to_io, int *ci);
int liblo10k1_con_del(liblo10k1_connection_t *conn, int type, int patch, int io, int *ci);

int liblo10k1_get_fx(liblo10k1_connection_t *conn, int fx_num, liblo10k1_get_io_t *out);
int liblo10k1_get_in(liblo10k1_connection_t *conn, int in_num, liblo10k1_get_io_t *out);
int liblo10k1_get_out(liblo10k1_connection_t *conn, int out_num, liblo10k1_get_io_t *out);
int liblo10k1_get_pin(liblo10k1_connection_t *conn, int patch_num, int in_num, liblo10k1_get_io_t *out);
int liblo10k1_get_pout(liblo10k1_connection_t *conn, int patch_num, int out_num, liblo10k1_get_io_t *out);

int liblo10k1_get_fx_count(liblo10k1_connection_t *conn, int *count);
int liblo10k1_get_in_count(liblo10k1_connection_t *conn, int *count);
int liblo10k1_get_out_count(liblo10k1_connection_t *conn, int *count);
int liblo10k1_get_pin_count(liblo10k1_connection_t *conn, int patch_num, int *count);
int liblo10k1_get_pout_count(liblo10k1_connection_t *conn, int patch_num, int *count);

int liblo10k1_get_patches_info(liblo10k1_connection_t *conn, liblo10k1_patches_info_t **out, int *count);
int liblo10k1_get_points_info(liblo10k1_connection_t *conn, int **out, int *count);
int liblo10k1_get_point_info(liblo10k1_connection_t *conn, int point_id, liblo10k1_point_info_t *out);
int liblo10k1_dump(liblo10k1_connection_t *conn, void **out, int *size);
int liblo10k1_check_version(liblo10k1_connection_t *conn);

int liblo10k1_get_dsp_info(liblo10k1_connection_t *conn, liblo10k1_dsp_info_t *info);

char *liblo10k1_error_str(int error);

#ifdef __cplusplus
}
#endif

#endif /* __LIBLO10K1_H */
