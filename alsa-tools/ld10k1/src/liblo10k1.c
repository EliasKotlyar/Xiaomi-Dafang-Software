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
#include <string.h>
#include "comm.h"
#include "ld10k1_fnc.h"
#include "ld10k1_error.h"
#include "liblo10k1.h"
#include "liblo10k1ef.h"
#include "liblo10k1lf.h"
#include "ld10k1_debug.h"
#include "config.h"

int send_request_check(int conn_num, int op, void *data, int data_size)
{
	int opr, sizer;
	int err;

	if ((err = send_request(conn_num, op, data, data_size)) < 0)
		return err;

	if ((err = receive_response(conn_num, &opr, &sizer)) < 0)
		return err;
		
	return 0;
}

int send_msg_data_check(int conn_num, void *data, int data_size)
{
	int opr, sizer;
	int err;

	if ((err = send_msg_data(conn_num, data, data_size)) < 0)
		return err;

	if ((err = receive_response(conn_num, &opr, &sizer)) < 0)
		return err;
		
	return 0;
}

int receive_response_data_check(int conn_num, void *data, int idata_size)
{
	int err;
	int op, data_size;

	if ((err = receive_response(conn_num, &op, &data_size)) < 0)
		return err;
		
	if (op == FNC_ERR)
		return LD10K1_ERR_PROTOCOL;
	if (data_size != idata_size)
		return LD10K1_ERR_PROTOCOL;
		
	if ((err = receive_msg_data(conn_num, data, data_size)) < 0)
		return err;
		
	/* receive check */
	if ((err = receive_response(conn_num, &op, &data_size)) < 0)
		return err;
	
	return 0;
}

void liblo10k1_connection_init(liblo10k1_connection_t *conn)
{
	*conn = 0;
}

int liblo10k1_connect(liblo10k1_param *param, liblo10k1_connection_t *conn)
{
	int err;

	param->server = 0;
	*conn = setup_comm(param);
	if (*conn < 0)
		return LD10K1_ERR_COMM_CONN;
	if ((err = connect_comm(*conn, param)) < 0) {
		free_comm(*conn);
		*conn = 0;
		return err;
	}
	return 0;
}

int liblo10k1_disconnect(liblo10k1_connection_t *conn)
{
	send_request(*conn, FNC_CLOSE_CONN, NULL, 0);
	free_comm(*conn);
	*conn = 0;
	return 0;
}

int liblo10k1_is_open(liblo10k1_connection_t *conn)
{
	return *conn != 0;
}

int liblo10k1_patch_set_in_count(liblo10k1_dsp_patch_t *p, int count)
{
	liblo10k1_dsp_pio_t *tmp = NULL;
	
	if (count > 0) {
		tmp = (liblo10k1_dsp_pio_t *)malloc(sizeof(liblo10k1_dsp_pio_t) * count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
		memset(tmp, 0, sizeof(liblo10k1_dsp_pio_t) * count);
	}
	
	p->in_count = count;
	if (p->ins)
		free(p->ins);
	p->ins = tmp;
	return 0;
}

int liblo10k1_patch_set_out_count(liblo10k1_dsp_patch_t *p, int count)
{
	liblo10k1_dsp_pio_t *tmp = NULL;
	
	if (count > 0) {
		tmp = (liblo10k1_dsp_pio_t *)malloc(sizeof(liblo10k1_dsp_pio_t) * count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
		memset(tmp, 0, sizeof(liblo10k1_dsp_pio_t) * count);
	}
	
	p->out_count = count;
	if (p->outs)
		free(p->outs);
	p->outs = tmp;
	return 0;
}

int liblo10k1_patch_set_const_count(liblo10k1_dsp_patch_t *p, int count)
{
	liblo10k1_dsp_cs_t *tmp = NULL;
	
	if (count > 0) {
		tmp = (liblo10k1_dsp_cs_t *)malloc(sizeof(liblo10k1_dsp_cs_t) * count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
		memset(tmp, 0, sizeof(liblo10k1_dsp_cs_t) * count);
	}
	
	p->const_count = count;
	if (p->consts)
		free(p->consts);
	p->consts = tmp;
	return 0;
}

int liblo10k1_patch_set_sta_count(liblo10k1_dsp_patch_t *p, int count)
{
	liblo10k1_dsp_cs_t *tmp = NULL;
	
	if (count > 0) {
		tmp = (liblo10k1_dsp_cs_t *)malloc(sizeof(liblo10k1_dsp_cs_t) * count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
		memset(tmp, 0, sizeof(liblo10k1_dsp_cs_t) * count);
	}
	
	p->sta_count = count;
	if (p->stas)
		free(p->stas);
	p->stas = tmp;
	return 0;
}

int liblo10k1_patch_set_dyn_count(liblo10k1_dsp_patch_t *p, int count)
{
	p->dyn_count = count;
	return 0;
}

int liblo10k1_patch_set_hw_count(liblo10k1_dsp_patch_t *p, int count)
{
	liblo10k1_dsp_hw_t *tmp = NULL;
	
	if (count > 0) {
		tmp = (liblo10k1_dsp_hw_t *)malloc(sizeof(liblo10k1_dsp_hw_t) * count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
		memset(tmp, 0, sizeof(liblo10k1_dsp_hw_t) * count);
	}
	
	p->hw_count = count;
	if (p->hws)
		free(p->hws);
	p->hws = tmp;
	return 0;
}

int liblo10k1_patch_set_tram_count(liblo10k1_dsp_patch_t *p, int count)
{
	liblo10k1_dsp_tram_grp_t *tmp = NULL;
	
	if (count > 0) {
		tmp = (liblo10k1_dsp_tram_grp_t *)malloc(sizeof(liblo10k1_dsp_tram_grp_t) * count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
		memset(tmp, 0, sizeof(liblo10k1_dsp_tram_grp_t) * count);
	}
	
	p->tram_count = count;
	if (p->tram)
		free(p->tram);
	p->tram = tmp;
	return 0;
}

int liblo10k1_patch_set_tram_acc_count(liblo10k1_dsp_patch_t *p, int count)
{
	liblo10k1_dsp_tram_acc_t *tmp = NULL;
	
	if (count > 0) {
		tmp = (liblo10k1_dsp_tram_acc_t *)malloc(sizeof(liblo10k1_dsp_tram_acc_t) * count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
		memset(tmp, 0, sizeof(liblo10k1_dsp_tram_acc_t) * count);
	}
	
	p->tram_acc_count = count;
	if (p->tram_acc)
		free(p->tram_acc);
	p->tram_acc = tmp;
	return 0;
}

int liblo10k1_patch_set_ctl_count(liblo10k1_dsp_patch_t *p, int count)
{
	liblo10k1_dsp_ctl_t *tmp = NULL;
	
	if (count > 0) {
		tmp = (liblo10k1_dsp_ctl_t *)malloc(sizeof(liblo10k1_dsp_ctl_t) * count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
		memset(tmp, 0, sizeof(liblo10k1_dsp_ctl_t) * count);
	}
	
	p->ctl_count = count;
	if (p->ctl)
		free(p->ctl);
	p->ctl = tmp;
	return 0;
}

int liblo10k1_patch_set_instr_count(liblo10k1_dsp_patch_t *p, int count)
{
	liblo10k1_dsp_instr_t *tmp = NULL;
	
	if (count > 0) {
		tmp = (liblo10k1_dsp_instr_t *)malloc(sizeof(liblo10k1_dsp_instr_t) * count);
		if (!tmp)
			return LD10K1_ERR_NO_MEM;
		memset(tmp, 0, sizeof(liblo10k1_dsp_instr_t) * count);
	}
	
	p->instr_count = count;
	if (p->instr)
		free(p->instr);
	p->instr = tmp;
	return 0;
}

liblo10k1_dsp_patch_t *liblo10k1_patch_alloc(int in_count, int out_count, int const_count, int sta_count, int dyn_count, int hw_count, int tram_count, int tram_acc_count, int ctl_count, int instr_count)
{
	liblo10k1_dsp_patch_t *np;
	int en = 0;

	np = (liblo10k1_dsp_patch_t *)malloc(sizeof(liblo10k1_dsp_patch_t));
	if (!np)
		return NULL;

	np->patch_name[0] ='\0';
	np->ins = NULL;
	np->outs = NULL;
	np->consts = NULL;
	np->stas = NULL;
	np->hws = NULL;
	np->tram = NULL;
	np->tram_acc = NULL;
	np->ctl = NULL;
	np->instr = NULL;

	if ((en = liblo10k1_patch_set_dyn_count(np, dyn_count)) < 0)
		goto error;
	if ((en = liblo10k1_patch_set_in_count(np, in_count)) < 0)
		goto error;
	if ((en = liblo10k1_patch_set_out_count(np, out_count)) < 0)
		goto error;
	if ((en = liblo10k1_patch_set_const_count(np, const_count)) < 0)
		goto error;
	if ((en = liblo10k1_patch_set_sta_count(np, sta_count)) < 0)
		goto error;
	if ((en = liblo10k1_patch_set_hw_count(np, hw_count)) < 0)
		goto error;
	if ((en = liblo10k1_patch_set_tram_count(np, tram_count)) < 0)
		goto error;
	if ((en = liblo10k1_patch_set_tram_acc_count(np, tram_acc_count)) < 0)
		goto error;
	if ((en = liblo10k1_patch_set_ctl_count(np, ctl_count)) < 0)
		goto error;
	if ((en = liblo10k1_patch_set_instr_count(np, instr_count)) < 0)
		goto error;
	return np;
error:
    	liblo10k1_patch_free(np);
	return NULL;
}

void liblo10k1_patch_free(liblo10k1_dsp_patch_t *patch)
{
	liblo10k1_patch_set_in_count(patch, 0);
	liblo10k1_patch_set_out_count(patch, 0);
	liblo10k1_patch_set_const_count(patch, 0);
	liblo10k1_patch_set_dyn_count(patch, 0);
	liblo10k1_patch_set_sta_count(patch, 0);
	liblo10k1_patch_set_hw_count(patch, 0);
	liblo10k1_patch_set_tram_count(patch, 0);
	liblo10k1_patch_set_tram_acc_count(patch, 0);
	liblo10k1_patch_set_ctl_count(patch, 0);
	liblo10k1_patch_set_instr_count(patch, 0);
		
	free(patch);
}

int liblo10k1_patch_load(liblo10k1_connection_t *conn, liblo10k1_dsp_patch_t *patch, int before, int *loaded, int *loaded_id)
{
	int err;
	ld10k1_fnc_patch_add_t patch_fnc;
	int tmpres[2];

	strncpy(patch_fnc.patch.patch_name, patch->patch_name, sizeof(patch_fnc.patch.patch_name) - 1);
	patch_fnc.patch.patch_name[sizeof(patch_fnc.patch.patch_name) - 1] = '\0';
	
	patch_fnc.patch.in_count = patch->in_count;
	patch_fnc.patch.out_count = patch->out_count;
	patch_fnc.patch.const_count = patch->const_count;
	patch_fnc.patch.static_count = patch->sta_count;
	patch_fnc.patch.dynamic_count = patch->dyn_count;
	patch_fnc.patch.hw_count = patch->hw_count;
	patch_fnc.patch.tram_count = patch->tram_count;
	patch_fnc.patch.tram_acc_count = patch->tram_acc_count;
	patch_fnc.patch.ctl_count = patch->ctl_count;
	patch_fnc.patch.instr_count = patch->instr_count;

	/* patch */
	/* add */
	patch_fnc.where = before;
	if ((err = send_request_check(*conn, FNC_PATCH_ADD, &patch_fnc, sizeof(ld10k1_fnc_patch_add_t))) < 0)
		return err;

	/* in */
	if (patch->in_count)
		if ((err = send_msg_data_check(*conn, patch->ins, sizeof(ld10k1_dsp_p_in_out_t) * patch->in_count)) < 0)
			return err;

	/* out */
	if (patch->out_count)
		if ((err = send_msg_data_check(*conn, patch->outs, sizeof(ld10k1_dsp_p_in_out_t) * patch->out_count)) < 0)
			return err;

	/* const */
	if (patch->const_count)
		if ((err = send_msg_data_check(*conn, patch->consts, sizeof(ld10k1_dsp_p_const_static_t) * patch->const_count)) < 0)
			return err;

	/* sta */
	if (patch->sta_count)
		if ((err = send_msg_data_check(*conn, patch->stas, sizeof(ld10k1_dsp_p_const_static_t) * patch->sta_count)) < 0)
			return err;

	/* hw */
	if (patch->hw_count)
		if ((err = send_msg_data_check(*conn, patch->hws, sizeof(ld10k1_dsp_p_hw_t) * patch->hw_count)) < 0)
			return err;

	if (patch->tram_count)
		/* tram groups */
		if ((err = send_msg_data_check(*conn, patch->tram, sizeof(ld10k1_dsp_tram_grp_t) * patch->tram_count)) < 0)
			return err;

	if (patch->tram_acc_count)
		/* tram access */
		if ((err = send_msg_data_check(*conn, patch->tram_acc, sizeof(ld10k1_dsp_tram_acc_t) * patch->tram_acc_count)) < 0)
			return err;

	if (patch->ctl_count)
		/* ctls */
		if ((err = send_msg_data_check(*conn, patch->ctl, sizeof(ld10k1_dsp_ctl_t) * patch->ctl_count)) < 0)
			return err;

	/* instr */
	if ((err = send_msg_data_check(*conn, patch->instr, sizeof(ld10k1_dsp_instr_t) * patch->instr_count)) < 0)
		return err;

	if ((err = receive_response_data_check(*conn, tmpres, sizeof(tmpres))) < 0)
		return err;

	if (loaded)
		*loaded = tmpres[0];
	if (loaded_id)
		*loaded_id = tmpres[1];

	return 0;
}

int liblo10k1_debug(liblo10k1_connection_t *conn, int deb, void (*prn_fnc)(char *))
{
	int err;
	ld10k1_fnc_debug_t debug_info;
	char debug_line[1000];
	int opr, sizer;

	/* add */
	debug_info.what = deb;

	if ((err = send_request(*conn, FNC_DEBUG, &debug_info, sizeof(ld10k1_fnc_debug_t))) < 0)
		return err;

	while (1) {
		if ((err = receive_response(*conn, &opr, &sizer)) < 0)
			return err;

		if (opr != FNC_CONTINUE)
			break;

		if (sizer < (int)sizeof(debug_line)) {
			if ((err = receive_msg_data(*conn, &debug_line, sizer)) < 0)
				return err;
		} else
			return LIBLO10K1_ERR_DEBUG;
		(*prn_fnc)(debug_line);
	}

	/* not checked */
	return receive_response(*conn, &opr, &sizer);
}

int liblo10k1_patch_unload(liblo10k1_connection_t *conn, int patch_num)
{
	ld10k1_fnc_patch_del_t patch_fnc;

	patch_fnc.where = patch_num;

	return send_request_check(*conn, FNC_PATCH_DEL, &patch_fnc, sizeof(ld10k1_fnc_patch_del_t));
}

int liblo10k1_dsp_init(liblo10k1_connection_t *conn)
{
	return send_request_check(*conn, FNC_DSP_INIT, NULL, 0);
}

static int liblo10k1_find_any(liblo10k1_connection_t *conn, int op, int patch, char *name, int *out)
{
	ld10k1_fnc_name_t name_info;
	int err;
	int idx = -1;

	name_info.patch_num = patch;
	name_info.gpr = -1;
	
	strncpy(name_info.name, name, sizeof(name_info.name) - 1);
	name_info.name[sizeof(name_info.name) - 1] = '\0';

	if ((err = send_request(*conn, op, &name_info, sizeof(ld10k1_fnc_name_t))) < 0)
		return err;

	if ((err = receive_response_data_check(*conn, &idx, sizeof(idx))) < 0)
		return err;
	
	*out = idx;
	return 0;
}

int liblo10k1_find_patch(liblo10k1_connection_t *conn, char *patch_name, int *out)
{
	return liblo10k1_find_any(conn, FNC_PATCH_FIND, -1, patch_name, out);
}

int liblo10k1_find_fx(liblo10k1_connection_t *conn, char *fx_name, int *out)
{
	return liblo10k1_find_any(conn, FNC_FX_FIND, -1, fx_name, out);
}

int liblo10k1_find_in(liblo10k1_connection_t *conn, char *in_name, int *out)
{
	return liblo10k1_find_any(conn, FNC_IN_FIND, -1, in_name, out);
}

int liblo10k1_find_out(liblo10k1_connection_t *conn, char *out_name, int *out)
{
	return liblo10k1_find_any(conn, FNC_OUT_FIND, -1, out_name, out);
}

int liblo10k1_find_patch_in(liblo10k1_connection_t *conn, int patch_num, char *patch_in_name, int *out)
{
	return liblo10k1_find_any(conn, FNC_PATCH_IN_FIND, patch_num, patch_in_name, out);
}

int liblo10k1_find_patch_out(liblo10k1_connection_t *conn, int patch_num, char *patch_out_name, int *out)
{
	return liblo10k1_find_any(conn, FNC_PATCH_OUT_FIND, patch_num, patch_out_name, out);
}

int liblo10k1_con_add(liblo10k1_connection_t *conn, int multi, int simple, int from_type, int from_patch, int from_io, int to_type, int to_patch, int to_io, int *ci)
{
	ld10k1_fnc_connection_t connection_fnc;
	int err;
	int conn_id;
	
	connection_fnc.what = FNC_CONNECTION_ADD;
	connection_fnc.multi = multi;
	connection_fnc.simple = simple;
	connection_fnc.from_type = from_type;
	connection_fnc.from_patch = from_patch;
	connection_fnc.from_io = from_io;
	connection_fnc.to_type = to_type;
	connection_fnc.to_patch = to_patch;
	connection_fnc.to_io = to_io;

	if ((err = send_request(*conn, FNC_CONNECTION_ADD, &connection_fnc, sizeof(ld10k1_fnc_connection_t))) < 0)
		return err;
	
	if ((err = receive_response_data_check(*conn, &conn_id, sizeof(conn_id))) < 0)
		return err;
		
	if (ci)
		*ci = conn_id;
	return 0;
}

int liblo10k1_con_del(liblo10k1_connection_t *conn, int type, int patch, int io, int *ci)
{
	ld10k1_fnc_connection_t connection_fnc;
	int err;
	int conn_id;

	connection_fnc.what = FNC_CONNECTION_DEL;
	connection_fnc.from_type = type;
	connection_fnc.from_patch = patch;
	connection_fnc.from_io = io;
	connection_fnc.to_type = -1;
	connection_fnc.to_patch = -1;
	connection_fnc.to_io = -1;
	
	if ((err = send_request(*conn, FNC_CONNECTION_DEL, &connection_fnc, sizeof(ld10k1_fnc_connection_t))) < 0)
		return err;
	
	if ((err = receive_response_data_check(*conn, &conn_id, sizeof(conn_id))) < 0)
		return err;
		
	if (ci)
		*ci = conn_id;
	return 0;
}

static int liblo10k1_rename_any(liblo10k1_connection_t *conn, int op, int patch_num, int gpr_num, char *name)
{
	ld10k1_fnc_name_t name_info;

	name_info.patch_num = patch_num;
	name_info.gpr = gpr_num;
	
	strncpy(name_info.name, name, sizeof(name_info.name) - 1);
	name_info.name[sizeof(name_info.name) - 1] = '\0';
	
	return send_request_check(*conn, op, &name_info, sizeof(ld10k1_fnc_name_t));
}

int liblo10k1_rename_patch(liblo10k1_connection_t *conn, int patch_num, char *patch_name)
{
	return liblo10k1_rename_any(conn, FNC_PATCH_RENAME, patch_num, -1, patch_name);
}

int liblo10k1_rename_patch_in(liblo10k1_connection_t *conn, int patch_num, int in, char *patch_in_name)
{
	return liblo10k1_rename_any(conn, FNC_PATCH_IN_RENAME, patch_num, in, patch_in_name);
}

int liblo10k1_rename_patch_out(liblo10k1_connection_t *conn, int patch_num, int out, char *patch_out_name)
{
	return liblo10k1_rename_any(conn, FNC_PATCH_OUT_RENAME, patch_num, out, patch_out_name);
}

int liblo10k1_rename_fx(liblo10k1_connection_t *conn, int fx, char *fx_name)
{
	return liblo10k1_rename_any(conn, FNC_FX_RENAME, -1, fx, fx_name);
}

int liblo10k1_rename_in(liblo10k1_connection_t *conn, int in, char *in_name)
{
	return liblo10k1_rename_any(conn, FNC_IN_RENAME, -1, in, in_name);
}

int liblo10k1_rename_out(liblo10k1_connection_t *conn, int out, char *out_name)
{
	return liblo10k1_rename_any(conn, FNC_OUT_RENAME, -1, out, out_name);
}

int liblo10k1_get_io(liblo10k1_connection_t *conn, int op, int io_num, liblo10k1_get_io_t *out);
int liblo10k1_get_pio(liblo10k1_connection_t *conn, int op, int patch_num, int io_num, liblo10k1_get_io_t *out);

int liblo10k1_get_fx(liblo10k1_connection_t *conn, int fx_num, liblo10k1_get_io_t *out)
{
	return liblo10k1_get_io(conn, FNC_GET_FX, fx_num, out);
}

int liblo10k1_get_in(liblo10k1_connection_t *conn, int in_num, liblo10k1_get_io_t *out)
{
	return liblo10k1_get_io(conn, FNC_GET_IN, in_num, out);
}

int liblo10k1_get_out(liblo10k1_connection_t *conn, int out_num, liblo10k1_get_io_t *out)
{
	return liblo10k1_get_io(conn, FNC_GET_OUT, out_num, out);
}

int liblo10k1_get_pin(liblo10k1_connection_t *conn, int patch_num, int in_num, liblo10k1_get_io_t *out)
{
	return liblo10k1_get_pio(conn, FNC_GET_PIN, patch_num, in_num, out);
}

int liblo10k1_get_pout(liblo10k1_connection_t *conn, int patch_num, int out_num, liblo10k1_get_io_t *out)
{
	return liblo10k1_get_pio(conn, FNC_GET_POUT, patch_num, out_num, out);
}

int liblo10k1_get_io(liblo10k1_connection_t *conn, int op, int io_num, liblo10k1_get_io_t *out)
{
	int err;
	
	if ((err = send_request(*conn, op, &io_num, sizeof(int))) < 0)
		return err;

	if ((err = receive_response_data_check(*conn, out, sizeof(liblo10k1_get_io_t))) < 0)
		return err;

	return 0;
}

int liblo10k1_get_pio(liblo10k1_connection_t *conn, int op, int patch_num, int io_num, liblo10k1_get_io_t *out)
{
	int tmp_num[2];
	int err;
	
	tmp_num[0] = patch_num;
	tmp_num[1] = io_num;
	
	if ((err = send_request(*conn, op, tmp_num, sizeof(int) * 2)) < 0)
		return err;

	if ((err = receive_response_data_check(*conn, out, sizeof(liblo10k1_get_io_t))) < 0)
		return err;
	
	return 0;
}

int liblo10k1_get_io_count(liblo10k1_connection_t *conn, int op, int *out);
int liblo10k1_get_pio_count(liblo10k1_connection_t *conn, int op, int patch_num, int *out);

int liblo10k1_get_fx_count(liblo10k1_connection_t *conn, int *out)
{
	return liblo10k1_get_io_count(conn, FNC_GET_FX_COUNT, out);
}

int liblo10k1_get_in_count(liblo10k1_connection_t *conn, int *out)
{
	return liblo10k1_get_io_count(conn, FNC_GET_IN_COUNT, out);
}

int liblo10k1_get_out_count(liblo10k1_connection_t *conn, int *out)
{
	return liblo10k1_get_io_count(conn, FNC_GET_OUT_COUNT, out);
}

int liblo10k1_get_pin_count(liblo10k1_connection_t *conn, int patch_num, int *out)
{
	return liblo10k1_get_pio_count(conn, FNC_GET_PIN_COUNT, patch_num, out);
}

int liblo10k1_get_pout_count(liblo10k1_connection_t *conn, int patch_num, int *out)
{
	return liblo10k1_get_pio_count(conn, FNC_GET_POUT_COUNT, patch_num, out);
}

int liblo10k1_get_io_count(liblo10k1_connection_t *conn, int op, int *out)
{
	int err;
	
	if ((err = send_request(*conn, op, 0, 0)) < 0)
		return err;

	if ((err = receive_response_data_check(*conn, out, sizeof(int))) < 0)
		return err;

	return 0;
}

int liblo10k1_get_pio_count(liblo10k1_connection_t *conn, int op, int patch_num, int *out)
{
	int err;
	
	if ((err = send_request(*conn, op, &patch_num, sizeof(int))) < 0)
		return err;

	if ((err = receive_response_data_check(*conn, out, sizeof(int))) < 0)
		return err;
	
	return 0;
}

int liblo10k1_get_patches_info(liblo10k1_connection_t *conn, liblo10k1_patches_info_t **out, int *count)
{
	liblo10k1_patches_info_t *info;
	int opr, sizer;
	int err;

	*out = NULL;
	*count = 0;
	info = NULL;

	if ((err = send_request(*conn, FNC_GET_PATCHES_INFO, 0, 0)) < 0)
		return err;

	if ((err = receive_response(*conn, &opr, &sizer)) < 0)
		return err;

	*count = sizer / sizeof(liblo10k1_patches_info_t);
	if (sizer > 0) {
		info = (liblo10k1_patches_info_t *)receive_msg_data_malloc(*conn, sizer);
		if (!info)
			return LD10K1_ERR_NO_MEM;
	}

	if ((err = receive_response(*conn, &opr, &sizer)) < 0) {
		free(info);
		return err;
	}

	*out = info;
	return 0;
}

int liblo10k1_patch_get(liblo10k1_connection_t *conn, int patch_num, liblo10k1_dsp_patch_t **opatch)
{
	liblo10k1_dsp_patch_t *patch;
	int err;

	ld10k1_dsp_patch_t tmp_patch;
	int opr, sizer;


	if ((err = send_request(*conn, FNC_GET_PATCH, &patch_num, sizeof(patch_num))) < 0)
		return err;

	if ((err = receive_response(*conn, &opr, &sizer)) < 0)
		return err;

	if (opr != FNC_CONTINUE)
		return LD10K1_ERR_PROTOCOL;

	if ((err = receive_msg_data(*conn, &tmp_patch, sizeof(tmp_patch))) < 0)
		return err;

	/* alloc patch */
	patch = liblo10k1_patch_alloc(tmp_patch.in_count, tmp_patch.out_count, tmp_patch.const_count, tmp_patch.static_count, tmp_patch.dynamic_count, tmp_patch.hw_count, tmp_patch.tram_count, tmp_patch.tram_acc_count, tmp_patch.ctl_count, tmp_patch.instr_count);
	if (!patch)
		return LD10K1_ERR_NO_MEM;

	strcpy(patch->patch_name, tmp_patch.patch_name);

	/* ins */
	if (patch->in_count) {
		if ((err = receive_response(*conn, &opr, &sizer)) < 0)
			return err;

		if (opr != FNC_CONTINUE || (unsigned int)sizer != patch->in_count * sizeof(ld10k1_dsp_p_in_out_t))
			goto err_protocol;

		patch->ins = (ld10k1_dsp_p_in_out_t *)receive_msg_data_malloc(*conn, sizer);
		if (!patch->ins)
			goto err_nomem;
	}

	/* outs */
	if (patch->out_count) {
		if ((err = receive_response(*conn, &opr, &sizer)) < 0)
			return err;

		if (opr != FNC_CONTINUE || (unsigned int)sizer != patch->out_count * sizeof(ld10k1_dsp_p_in_out_t))
			goto err_protocol;

		patch->outs = (ld10k1_dsp_p_in_out_t *)receive_msg_data_malloc(*conn, sizer);
		if (!patch->outs)
			goto err_nomem;
	}

	/* consts */
	if (patch->const_count) {
		if ((err = receive_response(*conn, &opr, &sizer)) < 0)
			return err;

		if (opr != FNC_CONTINUE || (unsigned int)sizer != patch->const_count * sizeof(ld10k1_dsp_p_const_static_t))
			goto err_protocol;

		patch->consts = (ld10k1_dsp_p_const_static_t *)receive_msg_data_malloc(*conn, sizer);
		if (!patch->consts)
			goto err_nomem;
	}

	/* stas */
	if (patch->sta_count) {
		if ((err = receive_response(*conn, &opr, &sizer)) < 0)
			return err;

		if (opr != FNC_CONTINUE || (unsigned int)sizer != patch->sta_count * sizeof(ld10k1_dsp_p_const_static_t))
			goto err_protocol;

		patch->stas = (ld10k1_dsp_p_const_static_t *)receive_msg_data_malloc(*conn, sizer);
		if (!patch->stas)
			goto err_nomem;
	}

	/* hws */
	if (patch->hw_count) {
		if ((err = receive_response(*conn, &opr, &sizer)) < 0)
			return err;

		if (opr != FNC_CONTINUE || (unsigned int)sizer != patch->hw_count * sizeof(ld10k1_dsp_p_hw_t))
			goto err_protocol;

		patch->hws = (ld10k1_dsp_p_hw_t *)receive_msg_data_malloc(*conn, sizer);
		if (!patch->hws)
			goto err_nomem;
	}

	/* tram grp */
	if (patch->tram_count) {
		if ((err = receive_response(*conn, &opr, &sizer)) < 0)
			return err;

		if (opr != FNC_CONTINUE || (unsigned int)sizer != patch->tram_count * sizeof(liblo10k1_dsp_tram_grp_t))
			goto err_protocol;

		patch->tram = (liblo10k1_dsp_tram_grp_t *)receive_msg_data_malloc(*conn, sizer);
		if (!patch->tram)
			goto err_nomem;
	}

	/* tram acc */
	if (patch->tram_acc_count) {
		if ((err = receive_response(*conn, &opr, &sizer)) < 0)
			return err;

		if (opr != FNC_CONTINUE || (unsigned int)sizer != patch->tram_acc_count * sizeof(liblo10k1_dsp_tram_acc_t))
			goto err_protocol;

		patch->tram_acc = (liblo10k1_dsp_tram_acc_t *)receive_msg_data_malloc(*conn, sizer);
		if (!patch->tram_acc)
			goto err_nomem;
	}

	/* ctls */
	if (patch->ctl_count) {
		if ((err = receive_response(*conn, &opr, &sizer)) < 0)
			return err;

		if (opr != FNC_CONTINUE || (unsigned int)sizer != patch->ctl_count * sizeof(liblo10k1_dsp_ctl_t))
			goto err_protocol;

		patch->ctl = (liblo10k1_dsp_ctl_t *)receive_msg_data_malloc(*conn, sizer);
		if (!patch->ctl)
			goto err_nomem;
	}

	/* instr */
	if (patch->instr_count) {
		if ((err = receive_response(*conn, &opr, &sizer)) < 0)
			return err;

		if (opr != FNC_CONTINUE || (unsigned int)sizer != patch->instr_count * sizeof(liblo10k1_dsp_instr_t))
			goto err_protocol;

		patch->instr = (liblo10k1_dsp_instr_t *)receive_msg_data_malloc(*conn, sizer);
		if (!patch->instr)
			goto err_nomem;
	}

 	if ((err = receive_response(*conn, &opr, &sizer)) < 0) {
		liblo10k1_patch_free(patch);
		return err;
	}

	*opatch = patch;
	return 0;
err_nomem:
	liblo10k1_patch_free(patch);
	return LD10K1_ERR_NO_MEM;

err_protocol:
	liblo10k1_patch_free(patch);
	return LD10K1_ERR_PROTOCOL;
}

int liblo10k1_dump(liblo10k1_connection_t *conn, void **out, int *size)
{
	int opr, sizer;
	int err;
	void *dump = NULL;

	if ((err = send_request(*conn, FNC_DUMP, 0, 0)) < 0)
		return err;

	if ((err = receive_response(*conn, &opr, &sizer)) < 0)
		return err;

	*size = sizer;
	if (sizer > 0) {
		dump = receive_msg_data_malloc(*conn, sizer);
		if (!dump)
			return LD10K1_ERR_NO_MEM;
	}

	if ((err = receive_response(*conn, &opr, &sizer)) < 0) {
		free(dump);
		return err;
	}

	*out = dump;
	return 0;
}

int liblo10k1_check_version(liblo10k1_connection_t *conn)
{
	int opr, sizer;
	int err;
	ld10k1_fnc_version_t ver;

	if ((err = send_request(*conn, FNC_VERSION, 0, 0)) < 0)
		return err;

	if ((err = receive_response(*conn, &opr, &sizer)) < 0)
		return err;

	if (sizer == sizeof(ver)) {
		if ((err = receive_msg_data(*conn, &ver, sizer)) < 0)
			return err;
	}
	else
		return LD10K1_ERR_WRONG_VER;

	if ((err = receive_response(*conn, &opr, &sizer)) < 0)
		return err;

	if (strcmp(ver.ld10k1_version, VERSION) == 0)
		return 0;
	else
		return LD10K1_ERR_WRONG_VER;
}

int liblo10k1_get_points_info(liblo10k1_connection_t *conn, int **out, int *count)
{
	int *info;
	int opr, sizer;
	int err;

	info = NULL;

	if ((err = send_request(*conn, FNC_GET_POINTS_INFO, 0, 0)) < 0)
		return err;

	if ((err = receive_response(*conn, &opr, &sizer)) < 0)
		return err;

	*count = sizer / sizeof(int);
	if (sizer > 0) {
		info = (int *)receive_msg_data_malloc(*conn, sizer);
		if (!info)
			return LD10K1_ERR_NO_MEM;
	}

	if ((err = receive_response(*conn, &opr, &sizer)) < 0) {
		free(info);
		return err;
	}

	*out = info;
	return 0;
}

int liblo10k1_get_point_info(liblo10k1_connection_t *conn, int point_id, liblo10k1_point_info_t *out)
{
	int opr, sizer;
	int err;

	if ((err = send_request(*conn, FNC_GET_POINT_INFO, &point_id, sizeof(point_id))) < 0)
		return err;

	if ((err = receive_response(*conn, &opr, &sizer)) < 0)
		return err;

	if (sizer != sizeof(liblo10k1_point_info_t))
		return LD10K1_ERR_PROTOCOL;
	if ((err = receive_msg_data(*conn, out, sizer)) < 0)
		return LD10K1_ERR_PROTOCOL;

	if ((err = receive_response(*conn, &opr, &sizer)) < 0)
		return err;

	return 0;
}

int liblo10k1_get_dsp_info(liblo10k1_connection_t *conn, liblo10k1_dsp_info_t *info)
{
	int err;
	
	if ((err = send_request(*conn, FNC_GET_DSP_INFO, NULL, 0)) < 0)
		return err;

	if ((err = receive_response_data_check(*conn, info, sizeof(liblo10k1_dsp_info_t))) < 0)
		return err;
	
	return 0;
}

struct errmsg_t
{
	int errnum;
	char *errmsg;
};

struct errmsg_t error_text[] =
{
	{LD10K1_ERR_UNKNOWN, "Unknown error"},
	{LD10K1_ERR_COMM_READ, "Error in read from socket"},
	{LD10K1_ERR_COMM_WRITE, "Error in write to socket"},
	{LD10K1_ERR_UNKNOWN_PATCH_NUM, "Wrong parameter - patch with this num doesn't exists"},
	{LD10K1_ERR_PROTOCOL, "ld10k1 is expecting more or less data as it got"},
	{LD10K1_ERR_COMM_CONN, "Error in socket connect"},

/* add patch */
	{LD10K1_ERR_PROTOCOL_IN_COUNT, "Wrong in registers count"},
	{LD10K1_ERR_PROTOCOL_OUT_COUNT, "Wrong out registers count"},
	{LD10K1_ERR_PROTOCOL_CONST_COUNT, "Wrong const registers count"},
	{LD10K1_ERR_PROTOCOL_STATIC_COUNT, "Wrong static registers count"},
	{LD10K1_ERR_PROTOCOL_DYNAMIC_COUNT, "Wrong dynamic registers count"},
	{LD10K1_ERR_PROTOCOL_HW_COUNT, "Wrong hw registers count"},
	{LD10K1_ERR_PROTOCOL_TRAM_COUNT, "Wrong tram count"},
	{LD10K1_ERR_PROTOCOL_TRAM_ACC_COUNT, "Wrong tram access registers count"},
	{LD10K1_ERR_PROTOCOL_CTL_COUNT, "Wrong controls count"},
	{LD10K1_ERR_PROTOCOL_INSTR_COUNT, "Wrong instructions count"},

/* driver */
	{LD10K1_ERR_DRIVER_CODE_POKE, "Unable to poke code"},
	{LD10K1_ERR_DRIVER_INFO, "Unable to get info"},
	{LD10K1_ERR_DRIVER_CODE_PEEK, "Unable to peek code"},
	{LD10K1_ERR_DRIVER_PCM_POKE, "Unable to poke pcm"},

/* tram */
	{LD10K1_ERR_ITRAM_FULL, "Not enought free itram"},
	{LD10K1_ERR_ETRAM_FULL, "Not enought free etram"},
	{LD10K1_ERR_TRAM_FULL, "Not enought free tram"},
	{LD10K1_ERR_TRAM_FULL_GRP, "Not enought free tram group"},

	{LD10K1_ERR_ITRAM_FULL_ACC, "Not enought free itram acc"},
	{LD10K1_ERR_ETRAM_FULL_ACC, "Not enought free etram acc"},
	{LD10K1_ERR_TRAM_FULL_ACC, "Not enought free tram acc"},

	{LD10K1_ERR_MAX_CON_PER_POINT, "Maximum connections per point reached"},

/* others */
	{LD10K1_ERR_NO_MEM, "Not enought free mem"},
	{LD10K1_ERR_MAX_PATCH_COUNT, "Max patch count excesed"},
	{LD10K1_ERR_NOT_FREE_REG, "There is not free reg"},
	{LD10K1_ERR_NOT_FREE_INSTR, "There is no free instruction slot"},

/* patch chceck */
	{LD10K1_ERR_WRONG_REG_HW_INDEX, "Loaded patch has wrong hw index"},
	{LD10K1_ERR_WRONG_TRAM_POS, "Loaded patch has wrong tram position"},

	{LD10K1_ERR_WRONG_TRAM_TYPE, "Loaded patch has wrong tram type"},
	{LD10K1_ERR_WRONG_TRAM_SIZE, "Loaded patch has wrong tram size"},
	{LD10K1_ERR_WRONG_TRAM_ACC_TYPE, "Loaded patch has wrong tram acc type"},

	{LD10K1_ERR_TRAM_GRP_OUT_OF_RANGE, "Loaded patch has tram gpr index out of range"},
	{LD10K1_ERR_TRAM_ACC_OUT_OF_RANGE, "Loaded patch has tram acc index out of range"},

	{LD10K1_ERR_CTL_VCOUNT_OUT_OF_RANGE, "Loaded patch has wrong count of visible gpr for control"},
	{LD10K1_ERR_CTL_COUNT_OUT_OF_RANGE, "Loaded patch has wrong count of gpr for control"},

	{LD10K1_ERR_CTL_MIN_MAX_RANGE, "Loaded patch has wrong min, max range for control"},
	{LD10K1_ERR_CTL_TRANLSLATION, "Loaded patch has wrong translation function for control"},
	{LD10K1_ERR_CTL_REG_INDEX, "Loaded patch has wrong gpr index for control"},
	{LD10K1_ERR_CTL_REG_VALUE, "Loaded patch has wrong initial value for control"},

	{LD10K1_ERR_INSTR_OPCODE, "Loaded patch has wrong instruction opcode"},
	{LD10K1_ERR_INSTR_ARG_INDEX, "Loaded patch has wrong argument for instruction"},

	{LD10K1_ERR_UNKNOWN_REG_NUM, "There isn't register with this reg number"},
	{LD10K1_ERR_UNKNOWN_PATCH_REG_NUM, "There isn't patch register with this reg number for patch"},

	{LD10K1_ERR_CONNECTION, "Couldn't connect patch to io"},
	{LD10K1_ERR_CONNECTION_FNC, "Not supported connect/disconnect function"},

	{LD10K1_ERR_CTL_EXISTS, "Can't add control because there is one with same name"},

	{LD10K1_ERR_PATCH_RENAME, "Couldn't rename patch"},
	{LD10K1_ERR_PATCH_REG_RENAME, "Couldn't rename patch register"},
	{LD10K1_ERR_REG_RENAME, "Couldn't rename register"},
	{LD10K1_ERR_WRONG_VER, "Wrong ld10k1 version"},
	{LD10K1_ERR_UNKNOWN_POINT, "Unknown point"},
	
	/* errors from liblo10k1ef */
	{LD10K1_EF_ERR_OPEN, "Can not open file"},
	{LD10K1_EF_ERR_STAT, "Can not stat file"},
	{LD10K1_EF_ERR_SIZE, "Wrong file size"},
	{LD10K1_EF_ERR_READ, "Can not read file"},
	{LD10K1_EF_ERR_SIGNATURE, "Wrong file signature"},
	{LD10K1_EF_ERR_FORMAT, "Wrong file format"},

	{LD10K1_EF_ERR_TRANSFORM_CTL, "Can not transform control"},
	{LD10K1_EF_ERR_TRANSFORM, "Can not transform effect"},
	{LD10K1_EF_ERR_TRANSFORM_TRANS, "Wrong control translation"},
	
	/* errors from liblo10k1lf */
	{LD10K1_LF_ERR_OPEN, "Can not open file"},
	{LD10K1_LF_ERR_WRITE, "Can not write file"},
	{LD10K1_LF_ERR_READ, "Can not read file"},
	{LD10K1_LF_ERR_SIGNATURE, "Wrong file signature"},
	{LD10K1_LF_ERR_PART_TYPE, "Wrong part type"},
	{LD10K1_LF_ERR_PART_SIZE, "Wrong part size"},
	{LD10K1_LF_ERR_VERSION, "Wrong file version"},
	{LD10K1_LF_ERR_FILE_TYPE, "Wrong file type"},

	{0, ""}
};

char *liblo10k1_error_str(int error)
{
	int i;

	for (i = 0; error_text[i].errnum != 0; i++)
		if (error_text[i].errnum == error)
			return error_text[i].errmsg;
	return "Error msg not specified in liblo10k1";
}
