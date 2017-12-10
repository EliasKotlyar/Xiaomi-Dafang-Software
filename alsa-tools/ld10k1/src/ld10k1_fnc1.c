/*
 *  EMU10k1 loader
 *
 *  Copyright (c) 2003,2004 by Peter Zubaj
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation;  either version 2 of the License, or
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

#include <alsa/asoundlib.h>

#include <signal.h>
#include "ld10k1.h"
#include "ld10k1_fnc.h"
#include "ld10k1_fnc_int.h"
#include "ld10k1_debug.h"
#include "ld10k1_error.h"
#include "ld10k1_dump.h"
#include "ld10k1_driver.h"
#include "ld10k1_mixer.h"
#include "comm.h"


void ld10k1_fnc_prepare_free();
int ld10k1_fnc_patch_add(int data_conn, int op, int size);
int ld10k1_fnc_patch_del(int data_conn, int op, int size);
int ld10k1_fnc_patch_conn(int data_conn, int op, int size);
int ld10k1_fnc_name_find(int data_conn, int op, int size);
int ld10k1_fnc_name_rename(int data_conn, int op, int size);
int ld10k1_fnc_dsp_init(int data_conn, int op, int size);
int ld10k1_fnc_get_io_count(int data_conn, int op, int size);
int ld10k1_fnc_get_io(int data_conn, int op, int size);
int ld10k1_fnc_get_pio_count(int data_conn, int op, int size);
int ld10k1_fnc_get_pio(int data_conn, int op, int size);
int ld10k1_fnc_get_patches_info(int data_conn, int op, int size);
int ld10k1_fnc_get_patch(int data_conn, int op, int size);
int ld10k1_fnc_dump(int data_conn, int op, int size);
int ld10k1_fnc_version(int data_conn, int op, int size);
int ld10k1_fnc_get_points_info(int data_conn, int op, int size);
int ld10k1_fnc_get_point_info(int data_conn, int op, int size);
int ld10k1_fnc_get_dsp_info(int data_conn, int op, int size);

ld10k1_dsp_mgr_t dsp_mgr;

struct fnc_table_t
{
	int fnc;
	int min_size;
	int max_size;
	int (*fnc_code)(int data_conn, int op, int size);
};

struct fnc_table_t fnc_table[] =
{
	{FNC_PATCH_ADD, sizeof(ld10k1_fnc_patch_add_t), sizeof(ld10k1_fnc_patch_add_t), ld10k1_fnc_patch_add},
	{FNC_PATCH_DEL, sizeof(ld10k1_fnc_patch_del_t), sizeof(ld10k1_fnc_patch_del_t), ld10k1_fnc_patch_del},
	{FNC_CONNECTION_ADD, sizeof(ld10k1_fnc_connection_t), sizeof(ld10k1_fnc_connection_t), ld10k1_fnc_patch_conn},
	{FNC_CONNECTION_DEL, sizeof(ld10k1_fnc_connection_t), sizeof(ld10k1_fnc_connection_t), ld10k1_fnc_patch_conn},
	{FNC_DEBUG, sizeof(ld10k1_fnc_debug_t), sizeof(ld10k1_fnc_debug_t), ld10k1_fnc_debug},
	{FNC_PATCH_RENAME, sizeof(ld10k1_fnc_name_t), sizeof(ld10k1_fnc_name_t), ld10k1_fnc_name_rename},
	{FNC_PATCH_FIND, sizeof(ld10k1_fnc_name_t), sizeof(ld10k1_fnc_name_t), ld10k1_fnc_name_find},
	{FNC_FX_FIND, sizeof(ld10k1_fnc_name_t), sizeof(ld10k1_fnc_name_t), ld10k1_fnc_name_find},
	{FNC_IN_FIND, sizeof(ld10k1_fnc_name_t), sizeof(ld10k1_fnc_name_t), ld10k1_fnc_name_find},
	{FNC_OUT_FIND, sizeof(ld10k1_fnc_name_t), sizeof(ld10k1_fnc_name_t), ld10k1_fnc_name_find},
	{FNC_PATCH_IN_FIND, sizeof(ld10k1_fnc_name_t), sizeof(ld10k1_fnc_name_t), ld10k1_fnc_name_find},
	{FNC_PATCH_OUT_FIND, sizeof(ld10k1_fnc_name_t), sizeof(ld10k1_fnc_name_t), ld10k1_fnc_name_find},
	{FNC_FX_RENAME, sizeof(ld10k1_fnc_name_t), sizeof(ld10k1_fnc_name_t), ld10k1_fnc_name_rename},
	{FNC_IN_RENAME, sizeof(ld10k1_fnc_name_t), sizeof(ld10k1_fnc_name_t), ld10k1_fnc_name_rename},
	{FNC_OUT_RENAME, sizeof(ld10k1_fnc_name_t), sizeof(ld10k1_fnc_name_t), ld10k1_fnc_name_rename},
	{FNC_PATCH_IN_RENAME, sizeof(ld10k1_fnc_name_t), sizeof(ld10k1_fnc_name_t), ld10k1_fnc_name_rename},
	{FNC_PATCH_OUT_RENAME, sizeof(ld10k1_fnc_name_t), sizeof(ld10k1_fnc_name_t), ld10k1_fnc_name_rename},
	{FNC_GET_FX_COUNT, 0, 0, ld10k1_fnc_get_io_count},
	{FNC_GET_IN_COUNT, 0, 0, ld10k1_fnc_get_io_count},
	{FNC_GET_OUT_COUNT, 0, 0, ld10k1_fnc_get_io_count},
	{FNC_GET_PIN_COUNT, sizeof(int), sizeof(int), ld10k1_fnc_get_pio_count},
	{FNC_GET_POUT_COUNT, sizeof(int), sizeof(int), ld10k1_fnc_get_pio_count},
	{FNC_GET_FX, sizeof(int), sizeof(int), ld10k1_fnc_get_io},
	{FNC_GET_IN, sizeof(int), sizeof(int), ld10k1_fnc_get_io},
	{FNC_GET_OUT, sizeof(int), sizeof(int), ld10k1_fnc_get_io},
	{FNC_GET_PIN, sizeof(int) * 2, sizeof(int) * 2, ld10k1_fnc_get_pio},
	{FNC_GET_POUT, sizeof(int) * 2, sizeof(int) * 2, ld10k1_fnc_get_pio},
	{FNC_GET_PATCHES_INFO, 0, 0, ld10k1_fnc_get_patches_info},
	{FNC_GET_PATCH, sizeof(int), sizeof(int), ld10k1_fnc_get_patch},
	{FNC_DSP_INIT, 0, 0, ld10k1_fnc_dsp_init},
	{FNC_DUMP, 0, 0, ld10k1_fnc_dump},
	{FNC_VERSION, 0, 0, ld10k1_fnc_version},
	{FNC_GET_POINTS_INFO, 0, 0, ld10k1_fnc_get_points_info},
	{FNC_GET_POINT_INFO, sizeof(int), sizeof(int), ld10k1_fnc_get_point_info},
	{FNC_GET_DSP_INFO, 0, 0, ld10k1_fnc_get_dsp_info},
	{-1, 0, 0, NULL}
};

ld10k1_dsp_mgr_t dsp_mgr;

int send_response_ok(int conn_num)
{
	return send_response(conn_num, FNC_OK, 0, NULL, 0);
}

int send_response_err(int conn_num, int err)
{
	return send_response(conn_num, FNC_ERR, err, NULL, 0);
}

int send_response_wd(int conn_num, void *data, int data_size)
{
	return send_response(conn_num, FNC_OK, 0, data, data_size);
}

struct ClientDefTag
{
	int used;
	int socket;
};

typedef struct ClientDefTag ClientDef;

#define MAX_CLIENTS 10
ClientDef clients[MAX_CLIENTS];
int clients_count = 0;

static void client_init()
{
	int i;
	for (i = 0; i < MAX_CLIENTS; i++)
		clients[i].used = 0;

	clients_count = 0;
}

static int client_add()
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
		if (clients[i].used == 0) {
			clients[i].used = 1;
			clients_count++;
			return i;
		}
	return -1;
}

static void client_del(int client)
{
    if (client >= 0 && client < MAX_CLIENTS && clients[client].used == 1) {
	clients[client].used = 0;
	clients_count--;
    }
}

static int client_find_by_socket(int socket)
{
	int i;

	for (i = 0; i < MAX_CLIENTS; i++)
		if (clients[i].socket == socket)
			return i;
	return -1;
}

int main_loop(comm_param *param, int audigy, const char *card_id, int tram_size, snd_ctl_t *ctlp)
{
	fd_set active_fd_set/*, read_fd_set*/;
	int i, j, res = 0;
	__sighandler_t old_sig_pipe;

	int main_sock = 0;
	int data_sock = 0;
	int new_client = 0;
	int op = 0;
	int data_size = 0;

	int retval = 0;

	dsp_mgr.audigy = audigy;
	dsp_mgr.card_id = card_id;

	if (ld10k1_dsp_mgr_init(&dsp_mgr))
		return -1;

	/* initialize id generators */
	ld10k1_dsp_mgr_init_id_gen(&dsp_mgr);


	if (ld10k1_init_driver(&dsp_mgr, tram_size) < 0) {
		ld10k1_dsp_mgr_free(&dsp_mgr);
		return -1;
	}
	
	if (ld10k1_init_reserved_ctls(&dsp_mgr, ctlp) < 0) {
		ld10k1_dsp_mgr_free(&dsp_mgr);
		return -1;
	}

	old_sig_pipe = signal(SIGPIPE, SIG_IGN);

	param->server = 1;
	if ((main_sock = setup_comm(param)) < 0)
		goto error;

	if (listen_comm(main_sock))
		goto error;

	/* Initialize the set of active sockets. */
	client_init();

	while (1) {
		/* Block until input arrives on one or more active sockets. */
		FD_ZERO (&active_fd_set);
		FD_SET (main_sock, &active_fd_set);

		for (i = 0; i < MAX_CLIENTS; i++)
			if (clients[i].used)
				FD_SET(clients[i].socket, &active_fd_set);


		if (select(FD_SETSIZE, &active_fd_set, NULL, NULL, NULL) < 0)
			goto error;


		for (i = 0; i < FD_SETSIZE; i++)
			if (FD_ISSET (i, &active_fd_set))	{
				if (i == main_sock)	{
					/* Connection request on original socket. */
					if ((data_sock = accept_comm(main_sock)) < 0)
						goto error;

					new_client = client_add();
					if (new_client < 0)
						free_comm(data_sock);
					else
						clients[new_client].socket = data_sock;
						/*FD_SET(data_sock, &active_fd_set);*/
				} else {
					/* Data arriving on an already-connected socket. */
					if (receive_request(i, &op, &data_size))
						/*goto error;*/
						op = -1; /* probably client closes */

					if (op == FNC_CLOSE_CONN) {
						/* wait some time */
						usleep(10000);
						goto e_close;
					}

					if (op >= 0) {
						/* search in function table */
						res = 1;
						for (j = 0; fnc_table[j].fnc >= 0; j++)	{
							if ((fnc_table[j].fnc == op) &&
								(data_size >= fnc_table[j].min_size) &&
								(data_size <= fnc_table[j].max_size)) {
								res = (*fnc_table[j].fnc_code)(i, op, data_size);
								break;
							}
						}
						if (!res) {
							if (send_response(i, FNC_OK, 0, NULL, 0) < 0)
								goto e_close;
						} else {
							if (send_response(i, FNC_ERR, res, NULL, 0) < 0)
								goto e_close;
						}
					} else {
e_close:
						if (op != FNC_CLOSE_CONN)
							printf("error protocol fnc:%d - %d\n", op, res);
						client_del(client_find_by_socket(i));
						/*FD_CLR (i, &active_fd_set);*/

						/* close connection */
						if (free_comm(i))
							data_sock = 0;
					}
				}
			}
	}
end:
	signal(SIGPIPE, old_sig_pipe);
	for (i = 0; i < MAX_CLIENTS; i++)
		if (clients[i].used) {
			client_del(i);
			free_comm(clients[i].socket);
		}
	free_comm(main_sock);

	ld10k1_free_reserved_ctls(&dsp_mgr);
	ld10k1_dsp_mgr_free(&dsp_mgr);

	return retval;

error:
	retval = -1;
	goto end;
}

int ld10k1_fnc_receive_patch_info(int data_conn, ld10k1_dsp_patch_t *new_patch, int *where)
{
	ld10k1_fnc_patch_add_t tmp_info;

	if (receive_msg_data(data_conn, &tmp_info, sizeof(ld10k1_fnc_patch_add_t)) < 0)
		return LD10K1_ERR_PROTOCOL;

	memcpy(new_patch, &(tmp_info.patch), sizeof(ld10k1_dsp_patch_t));
	*where = tmp_info.where;

	new_patch->patch_name[MAX_NAME_LEN - 1] = '\n';
	if (new_patch->in_count < 0 || new_patch->in_count > 32)
		return LD10K1_ERR_PROTOCOL_IN_COUNT;
	if (new_patch->out_count < 0 || new_patch->out_count > 32)
		return LD10K1_ERR_PROTOCOL_OUT_COUNT;
	if (new_patch->const_count < 0 || new_patch->const_count > 255)
		return LD10K1_ERR_PROTOCOL_CONST_COUNT;
	if (new_patch->static_count < 0 || new_patch->static_count > 255)
		return LD10K1_ERR_PROTOCOL_STATIC_COUNT;
	if (new_patch->dynamic_count < 0 || new_patch->dynamic_count > 255)
		return LD10K1_ERR_PROTOCOL_DYNAMIC_COUNT;
	if (new_patch->hw_count < 0 || new_patch->hw_count > 255)
		return LD10K1_ERR_PROTOCOL_HW_COUNT;
	if (new_patch->tram_count < 0 || new_patch->tram_count > 255)
		return LD10K1_ERR_PROTOCOL_TRAM_COUNT;
	if (new_patch->tram_acc_count < 0 || new_patch->tram_acc_count > 255)
		return LD10K1_ERR_PROTOCOL_TRAM_ACC_COUNT;
	if (new_patch->ctl_count < 0 || new_patch->ctl_count > 255)
		return LD10K1_ERR_PROTOCOL_CTL_COUNT;
	if (new_patch->instr_count < 0 || new_patch->instr_count > 512)
		return LD10K1_ERR_PROTOCOL_INSTR_COUNT;

	return send_response_ok(data_conn);
}

int ld10k1_fnc_receive_patch_in(int data_conn, ld10k1_patch_t *new_patch)
{
	ld10k1_dsp_p_in_out_t *new_in = NULL;
	int i;
	int err;

 	if (!new_patch->in_count)
		return 0;

 	if (!(new_in = (ld10k1_dsp_p_in_out_t *)receive_msg_data_malloc(data_conn, sizeof(ld10k1_dsp_p_in_out_t) * new_patch->in_count)))
		return LD10K1_ERR_PROTOCOL;

	/* copy values */
	for (i = 0; i < new_patch->in_count; i++) {
		if (!ld10k1_dsp_mgr_name_new(&(new_patch->ins[i].name), new_in[i].name)) {
			err = LD10K1_ERR_NO_MEM;
			goto error;
		}
	}

	return send_response_ok(data_conn);
error:
	free(new_in);
	return err;
}

int ld10k1_fnc_receive_patch_out(int data_conn, ld10k1_patch_t *new_patch)
{
	ld10k1_dsp_p_in_out_t *new_out = NULL;
	int i;
	int err;

 	if (!new_patch->out_count)
		return 0;

 	if (!(new_out = (ld10k1_dsp_p_in_out_t *)receive_msg_data_malloc(data_conn, sizeof(ld10k1_dsp_p_in_out_t) * new_patch->out_count)))
		return LD10K1_ERR_PROTOCOL;

	/* copy values */
	for (i = 0; i < new_patch->out_count; i++) {
		if (!ld10k1_dsp_mgr_name_new(&(new_patch->outs[i].name), new_out[i].name)) {
			err = LD10K1_ERR_NO_MEM;
			goto error;
		}
	}

	return send_response_ok(data_conn);
error:
	free(new_out);
	return err;
}

int ld10k1_fnc_receive_patch_const(int data_conn, ld10k1_patch_t *new_patch)
{
	ld10k1_dsp_p_const_static_t *new_const = NULL;
	int i;

 	if (!new_patch->const_count)
		return 0;

 	if (!(new_const = (ld10k1_dsp_p_const_static_t *)receive_msg_data_malloc(data_conn, sizeof(ld10k1_dsp_p_const_static_t) * new_patch->const_count)))
		return LD10K1_ERR_PROTOCOL;

	/* copy values */
	for (i = 0; i < new_patch->const_count; i++)
		new_patch->consts[i].const_val = new_const[i].const_val;

	return send_response_ok(data_conn);
}

int ld10k1_fnc_receive_patch_sta(int data_conn, ld10k1_patch_t *new_patch)
{
	ld10k1_dsp_p_const_static_t *new_sta = NULL;
	int i;

 	if (!new_patch->sta_count)
		return 0;

 	if (!(new_sta = (ld10k1_dsp_p_const_static_t *)receive_msg_data_malloc(data_conn, sizeof(ld10k1_dsp_p_const_static_t) * new_patch->sta_count)))
		return LD10K1_ERR_PROTOCOL;

	/* copy values */
	for (i = 0; i < new_patch->sta_count; i++)
		new_patch->stas[i].const_val = new_sta[i].const_val;

	return send_response_ok(data_conn);
}

int ld10k1_fnc_receive_patch_hw(int data_conn, ld10k1_patch_t *new_patch)
{
	ld10k1_dsp_p_hw_t *new_hw = NULL;
	int i;

 	if (!new_patch->hw_count)
		return 0;

 	if (!(new_hw = (ld10k1_dsp_p_hw_t *)receive_msg_data_malloc(data_conn, sizeof(ld10k1_dsp_p_hw_t) * new_patch->hw_count)))
		return LD10K1_ERR_PROTOCOL;

	/* copy values */
	for (i = 0; i < new_patch->hw_count; i++)
		new_patch->hws[i].reg_idx = new_hw[i].hw_val;

	return send_response_ok(data_conn);
}

int ld10k1_fnc_receive_patch_tram_grp(int data_conn, ld10k1_patch_t *new_patch)
{
	ld10k1_dsp_tram_grp_t *new_tram_grp = NULL;
	int i;

 	if (!new_patch->tram_count)
		return 0;

 	if (!(new_tram_grp = (ld10k1_dsp_tram_grp_t *)receive_msg_data_malloc(data_conn, sizeof(ld10k1_dsp_tram_grp_t) * new_patch->tram_count)))
		return LD10K1_ERR_PROTOCOL;

	/* copy values */
	for (i = 0; i < new_patch->tram_count; i++) {
		new_patch->tram_grp[i].grp_type = new_tram_grp[i].grp_type;
		new_patch->tram_grp[i].grp_size = new_tram_grp[i].grp_size;
		new_patch->tram_grp[i].grp_pos = new_tram_grp[i].grp_pos;
	}

	return send_response_ok(data_conn);
}

int ld10k1_fnc_receive_patch_tram_acc(int data_conn, ld10k1_patch_t *new_patch)
{
	ld10k1_dsp_tram_acc_t *new_tram_acc = NULL;
	int i;

 	if (!new_patch->tram_acc_count)
		return 0;

 	if (!(new_tram_acc = (ld10k1_dsp_tram_acc_t *)receive_msg_data_malloc(data_conn, sizeof(ld10k1_dsp_tram_acc_t) * new_patch->tram_acc_count)))
		return LD10K1_ERR_PROTOCOL;

	/* copy values */
	for (i = 0; i < new_patch->tram_acc_count; i++) {
		new_patch->tram_acc[i].acc_type = new_tram_acc[i].acc_type;
		new_patch->tram_acc[i].acc_offset = new_tram_acc[i].acc_offset;
		new_patch->tram_acc[i].grp = new_tram_acc[i].grp;
	}

	return send_response_ok(data_conn);
}

int ld10k1_fnc_receive_patch_ctl(int data_conn, ld10k1_patch_t *new_patch)
{
	ld10k1_dsp_ctl_t *new_ctl = NULL;
	int i, j;

 	if (!new_patch->ctl_count)
		return 0;

 	if (!(new_ctl = (ld10k1_dsp_ctl_t *)receive_msg_data_malloc(data_conn, sizeof(ld10k1_dsp_ctl_t) * new_patch->ctl_count)))
		return LD10K1_ERR_PROTOCOL;

	/* copy values */
	for (i = 0; i < new_patch->ctl_count; i++) {
		strncpy(new_patch->ctl[i].name, new_ctl[i].name, 43);
		new_patch->ctl[i].name[43] = '\0';
		new_patch->ctl[i].index = -1;
		new_patch->ctl[i].want_index = new_ctl[i].index;
		new_patch->ctl[i].count = new_ctl[i].count;
		new_patch->ctl[i].vcount = new_ctl[i].vcount;
		new_patch->ctl[i].min = new_ctl[i].min;
		new_patch->ctl[i].max = new_ctl[i].max;
		new_patch->ctl[i].translation = new_ctl[i].translation;

		for (j = 0; j < new_patch->ctl[i].count; j++)
			new_patch->ctl[i].value[j] = new_ctl[i].value[j];
	}

	return send_response_ok(data_conn);
}

int ld10k1_fnc_receive_patch_instr(int data_conn, ld10k1_patch_t *new_patch)
{
	ld10k1_dsp_instr_t *new_instr = NULL;
	int i, j;

 	if (!new_patch->instr_count)
		return 0;

 	if (!(new_instr = (ld10k1_dsp_instr_t *)receive_msg_data_malloc(data_conn, sizeof(ld10k1_dsp_instr_t) * new_patch->instr_count)))
		return LD10K1_ERR_PROTOCOL;

	/* copy values */
	for (i = 0; i < new_patch->instr_count; i++) {
		new_patch->instr[i].op_code = new_instr[i].op_code;
		for (j = 0; j < 4; j++)
			new_patch->instr[i].arg[j] = new_instr[i].arg[j];
		new_patch->instr[i].used = 1;
		new_patch->instr[i].modified = 1;
	}

	return send_response_ok(data_conn);
}

int ld10k1_fnc_patch_add(int data_conn, int op, int size)
{
	int err;
	int loaded[2];
	int where;

	ld10k1_dsp_patch_t new_patch_info;
	/* allocate new patch */
	ld10k1_patch_t *new_patch = NULL;

	if ((err = ld10k1_fnc_receive_patch_info(data_conn, &new_patch_info, &where)) < 0)
		goto error;

	if (!(new_patch = ld10k1_dsp_mgr_patch_new())) {
		err = LD10K1_ERR_NO_MEM;
		goto error;
	}

	/* name */
	if (!ld10k1_dsp_mgr_name_new(&(new_patch->patch_name), new_patch_info.patch_name)) {
		err = LD10K1_ERR_NO_MEM;
		goto error;
	}


	/* set sizes */
	if (new_patch_info.in_count)
		if (!ld10k1_dsp_mgr_patch_in_new(new_patch, new_patch_info.in_count)) {
			err = LD10K1_ERR_NO_MEM;
			goto error;
		}

	if (new_patch_info.out_count)
		if (!ld10k1_dsp_mgr_patch_out_new(new_patch, new_patch_info.out_count)) {
			err = LD10K1_ERR_NO_MEM;
			goto error;
		}

	if (new_patch_info.const_count)
		if (!ld10k1_dsp_mgr_patch_const_new(new_patch, new_patch_info.const_count)) {
			err = LD10K1_ERR_NO_MEM;
			goto error;
		}

	if (new_patch_info.static_count)
		if (!ld10k1_dsp_mgr_patch_sta_new(new_patch, new_patch_info.static_count)) {
			err = LD10K1_ERR_NO_MEM;
			goto error;
		}

	if (new_patch_info.dynamic_count)
		if (!ld10k1_dsp_mgr_patch_dyn_new(new_patch, new_patch_info.dynamic_count)) {
			err = LD10K1_ERR_NO_MEM;
			goto error;
		}

	if (new_patch_info.hw_count)
		if (!ld10k1_dsp_mgr_patch_hw_new(new_patch, new_patch_info.hw_count)) {
			err = LD10K1_ERR_NO_MEM;
			goto error;
		}

	if (new_patch_info.tram_count)
		if (!ld10k1_dsp_mgr_patch_tram_new(new_patch, new_patch_info.tram_count)) {
			err = LD10K1_ERR_NO_MEM;
			goto error;
		}

	if (new_patch_info.tram_acc_count)
		if (!ld10k1_dsp_mgr_patch_tram_acc_new(new_patch, new_patch_info.tram_acc_count)) {
			err = LD10K1_ERR_NO_MEM;
			goto error;
		}

	if (new_patch_info.ctl_count)
		if (!ld10k1_dsp_mgr_patch_ctl_new(new_patch,new_patch_info.ctl_count)) {
			err = LD10K1_ERR_NO_MEM;
			goto error;
		}

	if (!ld10k1_dsp_mgr_patch_instr_new(new_patch, new_patch_info.instr_count)) {
		err = LD10K1_ERR_NO_MEM;
		goto error;
	}

	/* receive next parts */
	if (new_patch_info.in_count)
		if ((err = ld10k1_fnc_receive_patch_in(data_conn, new_patch)) < 0)
			goto error;

	if (new_patch_info.out_count)
		if ((err = ld10k1_fnc_receive_patch_out(data_conn, new_patch)) < 0)
			goto error;

	if (new_patch_info.const_count)
		if ((err = ld10k1_fnc_receive_patch_const(data_conn, new_patch)) < 0)
			goto error;

	if (new_patch_info.static_count)
		if ((err = ld10k1_fnc_receive_patch_sta(data_conn, new_patch)) < 0)
			goto error;

	if (new_patch_info.hw_count)
		if ((err = ld10k1_fnc_receive_patch_hw(data_conn, new_patch)) < 0)
			goto error;

	if (new_patch_info.tram_count)
		if ((err = ld10k1_fnc_receive_patch_tram_grp(data_conn, new_patch)) < 0)
			goto error;

	if (new_patch_info.tram_acc_count)
		if ((err = ld10k1_fnc_receive_patch_tram_acc(data_conn, new_patch)) < 0)
			goto error;

	if (new_patch_info.ctl_count)
		if ((err = ld10k1_fnc_receive_patch_ctl(data_conn, new_patch)) < 0)
			goto error;

	if ((err = ld10k1_fnc_receive_patch_instr(data_conn, new_patch)) < 0)
		goto error;

	/* check patch */
	if ((err = ld10k1_patch_fnc_check_patch(&dsp_mgr, new_patch)) < 0)
		goto error;

	/* load patch */
	if ((err = ld10k1_dsp_mgr_patch_load(&dsp_mgr, new_patch, where, loaded)) < 0)
		goto error;

	if ((err = send_response_wd(data_conn, loaded, sizeof(loaded))) < 0)
		return err;

	return 0;
error:
	if (new_patch)
		ld10k1_dsp_mgr_patch_free(new_patch);
	return err;
}

int ld10k1_fnc_patch_del(int data_conn, int op, int size)
{
	ld10k1_fnc_patch_del_t patch_info;
	int err;

	if ((err = receive_msg_data(data_conn, &patch_info, sizeof(ld10k1_fnc_patch_del_t))) < 0)
		return err;

	return ld10k1_patch_fnc_del(&dsp_mgr, &patch_info);
}

int ld10k1_fnc_patch_conn(int data_conn, int op, int size)
{
	ld10k1_fnc_connection_t connection_info;
	int err;
	int conn_id;

	if ((err = receive_msg_data(data_conn, &connection_info, sizeof(ld10k1_fnc_connection_t))) < 0)
		return err;

	if ((err = ld10k1_connection_fnc(&dsp_mgr, &connection_info, &conn_id)) < 0)
		return err;
		
	return send_response_wd(data_conn, &conn_id, sizeof(conn_id));
}

int ld10k1_fnc_name_find(int data_conn, int op, int size)
{
	ld10k1_fnc_name_t name_info;
	int i;
	static int ret;
	int err;
	ld10k1_patch_t *patch;

	ret = -1;

	if ((err = receive_msg_data(data_conn, &name_info, sizeof(ld10k1_fnc_name_t))) < 0)
		return err;

	name_info.name[MAX_NAME_LEN - 1] = '\0';

	switch (op)	{
		case FNC_PATCH_FIND:
			for (i = 0; i < EMU10K1_PATCH_MAX ; i++)
				if (dsp_mgr.patch_ptr[i])
					if (strcmp(dsp_mgr.patch_ptr[i]->patch_name, name_info.name) == 0) {
						ret = i;
						break;
					}
			break;
		case FNC_FX_FIND:
			for (i = 0; i < dsp_mgr.fx_count ; i++)
				if (dsp_mgr.fxs[i].name)
					if (strcmp(dsp_mgr.fxs[i].name, name_info.name) == 0) {
						ret = i;
						break;
					}
			break;
		case FNC_IN_FIND:
			for (i = 0; i < dsp_mgr.in_count ; i++)
				if (dsp_mgr.ins[i].name)
					if (strcmp(dsp_mgr.ins[i].name, name_info.name) == 0) {
						ret = i;
						break;
					}
			break;
		case FNC_OUT_FIND:
			for (i = 0; i < dsp_mgr.out_count ; i++)
				if (dsp_mgr.outs[i].name)
					if (strcmp(dsp_mgr.outs[i].name, name_info.name) == 0) {
						ret = i;
						break;
					}
			break;
		case FNC_PATCH_IN_FIND :
			if (name_info.patch_num >= 0 || name_info.patch_num < EMU10K1_PATCH_MAX) {
				patch = dsp_mgr.patch_ptr[name_info.patch_num];
				if (patch)
					for (i = 0; i < patch->in_count ; i++)
						if (patch->ins[i].name)
							if (strcmp(patch->ins[i].name, name_info.name) == 0) {
								ret = i;
								break;
							}
			}
			break;
		case FNC_PATCH_OUT_FIND :
			if (name_info.patch_num >= 0 || name_info.patch_num < EMU10K1_PATCH_MAX) {
				patch = dsp_mgr.patch_ptr[name_info.patch_num];
				if (patch)
					for (i = 0; i < patch->out_count ; i++)
						if (patch->outs[i].name)
							if (strcmp(patch->outs[i].name, name_info.name) == 0) {
								ret = i;
								break;
							}
			}
			break;
	}

	return send_response_wd(data_conn, &ret, sizeof(ret));
}

int ld10k1_fnc_name_rename(int data_conn, int op, int size)
{
	ld10k1_fnc_name_t name_info;
	int ret;
	int err;
	ld10k1_patch_t *patch;

	ret = -1;

	if ((err = receive_msg_data(data_conn, &name_info, sizeof(ld10k1_fnc_name_t))) < 0)
		return err;

	name_info.name[MAX_NAME_LEN - 1] = '\0';

	switch (op)	{
		case FNC_PATCH_RENAME:
			if (name_info.patch_num >= 0 || name_info.patch_num < EMU10K1_PATCH_MAX) {
				patch = dsp_mgr.patch_ptr[name_info.patch_num];
				if (patch) {
					if (!ld10k1_dsp_mgr_name_new(&(patch->patch_name), name_info.name))
						return LD10K1_ERR_PATCH_RENAME;
				} else
					return LD10K1_ERR_UNKNOWN_PATCH_NUM;
			} else
				return LD10K1_ERR_UNKNOWN_PATCH_NUM;
			break;
		case FNC_FX_RENAME:
			if (name_info.gpr < 0 || name_info.gpr >= dsp_mgr.fx_count)
				return LD10K1_ERR_UNKNOWN_REG_NUM;
			if (!ld10k1_dsp_mgr_name_new(&(dsp_mgr.fxs[name_info.gpr].name), name_info.name))
				return LD10K1_ERR_REG_RENAME;
			break;
		case FNC_IN_RENAME:
			if (name_info.gpr < 0 || name_info.gpr >= dsp_mgr.in_count)
				return LD10K1_ERR_UNKNOWN_REG_NUM;
			if (!ld10k1_dsp_mgr_name_new(&(dsp_mgr.ins[name_info.gpr].name), name_info.name))
				return LD10K1_ERR_REG_RENAME;
			break;
		case FNC_OUT_RENAME:
			if (name_info.gpr < 0 || name_info.gpr >= dsp_mgr.out_count)
				return LD10K1_ERR_UNKNOWN_REG_NUM;
			if (!ld10k1_dsp_mgr_name_new(&(dsp_mgr.outs[name_info.gpr].name), name_info.name))
				return LD10K1_ERR_REG_RENAME;
			break;
		case FNC_PATCH_IN_RENAME:
			if (name_info.patch_num >= 0 || name_info.patch_num < EMU10K1_PATCH_MAX) {
				patch = dsp_mgr.patch_ptr[name_info.patch_num];
				if (patch) {
					if (name_info.gpr < 0 || name_info.gpr >= patch->in_count)
						return LD10K1_ERR_UNKNOWN_PATCH_REG_NUM;
					if (!ld10k1_dsp_mgr_name_new(&(patch->ins[name_info.gpr].name), name_info.name))
						return LD10K1_ERR_PATCH_REG_RENAME;
				} else
					return LD10K1_ERR_UNKNOWN_PATCH_NUM;
			} else
				return LD10K1_ERR_UNKNOWN_PATCH_NUM;
			break;
		case FNC_PATCH_OUT_RENAME:
			if (name_info.patch_num >= 0 || name_info.patch_num < EMU10K1_PATCH_MAX) {
				patch = dsp_mgr.patch_ptr[name_info.patch_num];
				if (patch) {
					if (name_info.gpr < 0 || name_info.gpr >= patch->out_count)
						return LD10K1_ERR_UNKNOWN_PATCH_REG_NUM;
					if (!ld10k1_dsp_mgr_name_new(&(patch->outs[name_info.gpr].name), name_info.name))
						return LD10K1_ERR_PATCH_REG_RENAME;
				} else
					return LD10K1_ERR_UNKNOWN_PATCH_NUM;
			} else
				return LD10K1_ERR_UNKNOWN_PATCH_NUM;
			break;
	}
	return 0;
}

int ld10k1_fnc_dsp_init(int data_conn, int op, int size)
{
	int audigy;
	int err, i;
	
	ld10k1_reserved_ctl_list_item_t *rlist;
	int save_ids[EMU10K1_PATCH_MAX];

	audigy = dsp_mgr.audigy;

	rlist = dsp_mgr.reserved_ctl_list; /* FIXME - hack to save reserved ctls and ids */
	for (i = 0; i < EMU10K1_PATCH_MAX; i++)
		save_ids[i] = dsp_mgr.patch_id_gens[i];
		
	ld10k1_dsp_mgr_free(&dsp_mgr);
	memset(&dsp_mgr, 0, sizeof(dsp_mgr));

	dsp_mgr.audigy = audigy;

	if ((err = ld10k1_dsp_mgr_init(&dsp_mgr)) < 0)
		return err;
		
	dsp_mgr.reserved_ctl_list = rlist; /* hack to seve reserved ctls */
	
	for (i = 0; i < EMU10K1_PATCH_MAX; i++)
		dsp_mgr.patch_id_gens[i] = save_ids[i];

	return ld10k1_init_driver(&dsp_mgr, -1);
}

int ld10k1_fnc_get_io_count(int data_conn, int op, int size)
{
	int reg_count;
	
	reg_count = 0;
		
	if (op == FNC_GET_FX_COUNT)
		/* fxs */
		reg_count = dsp_mgr.fx_count;
	else if (op == FNC_GET_IN_COUNT)
		/* ins */
		reg_count = dsp_mgr.in_count;
	else
		/* outs */
		reg_count = dsp_mgr.out_count;

	return send_response_wd(data_conn, &reg_count, sizeof(int));
}

int ld10k1_fnc_get_io(int data_conn, int op, int size)
{
	int err;

	int reg_count;
	int reg_num;
	ld10k1_fnc_get_io_t io;

	if ((err = receive_msg_data(data_conn, &reg_num, sizeof(int))) < 0)
		return err;

	if (op == FNC_GET_FX)
		/* fx */
		reg_count = dsp_mgr.fx_count;
	else if (op == FNC_GET_IN)
		/* in */
		reg_count = dsp_mgr.in_count;
	else
		/* out */
		reg_count = dsp_mgr.out_count;
		
	if (reg_num < 0 || reg_num >= reg_count)
		return LD10K1_ERR_UNKNOWN_REG_NUM;

	if (op == FNC_GET_FX) {
		/* fx */
		memset(io.name, 0, sizeof(io.name));
		if (dsp_mgr.fxs[reg_num].name)
			strcpy(io.name, dsp_mgr.fxs[reg_num].name);
	} else if (op == FNC_GET_IN) {
		/* in */
		memset(io.name, 0, sizeof(io.name));
		if (dsp_mgr.ins[reg_num].name)
			strcpy(io.name, dsp_mgr.ins[reg_num].name);
	} else {
		/* out */
		memset(io.name, 0, sizeof(io.name));
		if (dsp_mgr.outs[reg_num].name)
			strcpy(io.name, dsp_mgr.outs[reg_num].name);
	}

	return send_response_wd(data_conn, &io, sizeof(ld10k1_fnc_get_io_t));
}

int ld10k1_fnc_get_pio_count(int data_conn, int op, int size)
{
	int patch_num;
	int err;

	int reg_count;
	ld10k1_patch_t *patch;

	if ((err = receive_msg_data(data_conn, &patch_num, sizeof(int))) < 0)
		return err;

	reg_count = 0;
	/* patch */
	if (patch_num >= 0 && patch_num < EMU10K1_PATCH_MAX) {
		/* patch register */
		patch = dsp_mgr.patch_ptr[patch_num];
		if (!patch)
			return LD10K1_ERR_UNKNOWN_PATCH_NUM;

		if (op == FNC_GET_PIN_COUNT)
			/* pin */
			reg_count = patch->in_count;
		else
			/* pout */
			reg_count = patch->out_count;
	} else
		return LD10K1_ERR_UNKNOWN_PATCH_NUM;
	
	return send_response_wd(data_conn, &reg_count, sizeof(int));
}

int ld10k1_fnc_get_pio(int data_conn, int op, int size)
{
	int patch_num;
	int reg_num;
	int err;

	int tmp_num[2];
	int reg_count;
	ld10k1_fnc_get_io_t io;
	ld10k1_patch_t *patch;

	if ((err = receive_msg_data(data_conn, tmp_num, sizeof(int) * 2)) < 0)
		return err;
	
	patch_num = tmp_num[0];
	reg_num = tmp_num[1];
		
	reg_count = 0;
	
	/* patch */
	if (patch_num >= 0 && patch_num < EMU10K1_PATCH_MAX) {
		/* patch register */
		patch = dsp_mgr.patch_ptr[patch_num];
		if (!patch)
			return LD10K1_ERR_UNKNOWN_PATCH_NUM;

		if (op == FNC_GET_PIN)
			/* pin */
			reg_count = patch->in_count;
		else
			/* pout */
			reg_count = patch->out_count;
			
		if (reg_num < 0 || reg_num >= reg_count)
			return LD10K1_ERR_UNKNOWN_PATCH_REG_NUM;

		if (op == FNC_GET_PIN) {
			/* pin */
			memset(io.name, 0, sizeof(io.name));
			if (patch->ins[reg_num].name)
				strcpy(io.name, patch->ins[reg_num].name);
		} else {
			/* pout */
			memset(io.name, 0, sizeof(io.name));
			if (patch->outs[reg_num].name)
				strcpy(io.name, patch->outs[reg_num].name);
		}
	} else
		return LD10K1_ERR_UNKNOWN_PATCH_NUM;

	return send_response_wd(data_conn, &io, sizeof(ld10k1_fnc_get_io_t));
}


int ld10k1_fnc_get_patches_info(int data_conn, int op, int size)
{
	int i, idx, j;
	ld10k1_fnc_patches_info_t *info;
	ld10k1_patch_t *patch;

	info = NULL;

	if (dsp_mgr.patch_count >= 0) {
		/* alloc space */
		info = (ld10k1_fnc_patches_info_t *)malloc(sizeof(ld10k1_fnc_patches_info_t) * dsp_mgr.patch_count);
		if (!info)
			return LD10K1_ERR_NO_MEM;
		memset(info, 0, sizeof(ld10k1_fnc_patches_info_t) * dsp_mgr.patch_count);

		/* copy values */
		for (i = 0, j = 0; i < dsp_mgr.patch_count; i++) {
			idx = dsp_mgr.patch_order[i];
			patch = dsp_mgr.patch_ptr[idx];
			if (patch) {
				info[j].patch_num = idx;
				info[j].id = patch->id;
				strcpy(info[j].patch_name, patch->patch_name);
				j++;
			}
		}
	}

	return send_response_wd(data_conn, info, sizeof(ld10k1_fnc_patches_info_t) * dsp_mgr.patch_count);
}

int ld10k1_fnc_version(int data_conn, int op, int size)
{

	ld10k1_fnc_version_t version;

	strcpy(version.ld10k1_version, VERSION);
	return send_response_wd(data_conn, &version, sizeof(ld10k1_fnc_version_t));
}


int ld10k1_fnc_send_patch_in(int data_conn, ld10k1_patch_t *patch)
{
	int i, err;
	ld10k1_dsp_p_in_out_t *ins = NULL;

	if (patch->in_count) {
		ins = (ld10k1_dsp_p_in_out_t *)malloc(sizeof(ld10k1_dsp_p_in_out_t) * patch->in_count);
		if (!ins)
			return LD10K1_ERR_NO_MEM;
		memset(ins, 0, sizeof(ld10k1_dsp_p_in_out_t) * patch->in_count);

		for (i = 0; i < patch->in_count; i++) {
			if (patch->ins[i].name)
				strcpy(ins[i].name, patch->ins[i].name);
			else
				ins[i].name[0] = '\0';
		}

		if ((err = send_response(data_conn, FNC_CONTINUE, 0, ins, sizeof(ld10k1_dsp_p_in_out_t) * patch->in_count)) < 0) {
			free(ins);
			return err;
		}

		free(ins);
	}
	return 0;
}

int ld10k1_fnc_send_patch_out(int data_conn, ld10k1_patch_t *patch)
{
	int i, err;
	ld10k1_dsp_p_in_out_t *outs = NULL;

	if (patch->out_count) {
		outs = (ld10k1_dsp_p_in_out_t *)malloc(sizeof(ld10k1_dsp_p_in_out_t) * patch->out_count);
		if (!outs)
			return LD10K1_ERR_NO_MEM;
		memset(outs, 0, sizeof(ld10k1_dsp_p_in_out_t) * patch->out_count);

		for (i = 0; i < patch->out_count; i++) {
			if (patch->outs[i].name)
				strcpy(outs[i].name, patch->outs[i].name);
			else
				outs[i].name[0] = '\0';
		}

		if ((err = send_response(data_conn, FNC_CONTINUE, 0, outs, sizeof(ld10k1_dsp_p_in_out_t) * patch->out_count)) < 0) {
			free(outs);
			return err;
		}

		free(outs);
	}
	return 0;
}

int ld10k1_fnc_send_patch_const(int data_conn, ld10k1_patch_t *patch)
{
	int i, err;
	ld10k1_dsp_p_const_static_t *consts = NULL;

	if (patch->const_count) {
		consts = (ld10k1_dsp_p_const_static_t *)malloc(sizeof(ld10k1_dsp_p_const_static_t) * patch->const_count);
		if (!consts)
			return LD10K1_ERR_NO_MEM;

		for (i = 0; i < patch->const_count; i++)
			consts[i].const_val = patch->consts[i].const_val;

		if ((err = send_response(data_conn, FNC_CONTINUE, 0, consts, sizeof(ld10k1_dsp_p_const_static_t) * patch->const_count)) < 0) {
			free(consts);
			return err;
		}

		free(consts);
	}
	return 0;
}

int ld10k1_fnc_send_patch_sta(int data_conn, ld10k1_patch_t *patch)
{
	int i, err;
	ld10k1_dsp_p_const_static_t *stas = NULL;

	if (patch->sta_count) {
		stas = (ld10k1_dsp_p_const_static_t *)malloc(sizeof(ld10k1_dsp_p_const_static_t) * patch->sta_count);
		if (!stas)
			return LD10K1_ERR_NO_MEM;

		for (i = 0; i < patch->sta_count; i++)
			stas[i].const_val = patch->stas[i].const_val;

		if ((err = send_response(data_conn, FNC_CONTINUE, 0, stas, sizeof(ld10k1_dsp_p_const_static_t) * patch->sta_count)) < 0) {
			free(stas);
			return err;
		}

		free(stas);
	}
	return 0;
}

int ld10k1_fnc_send_patch_hw(int data_conn, ld10k1_patch_t *patch)
{
	int i, err;
	ld10k1_dsp_p_hw_t *hws = NULL;

	if (patch->hw_count) {
		hws = (ld10k1_dsp_p_hw_t *)malloc(sizeof(ld10k1_dsp_p_hw_t) * patch->hw_count);
		if (!hws)
			return LD10K1_ERR_NO_MEM;

		for (i = 0; i < patch->hw_count; i++)
			hws[i].hw_val = patch->hws[i].reg_idx;

		if ((err = send_response(data_conn, FNC_CONTINUE, 0, hws, sizeof(ld10k1_dsp_p_hw_t) * patch->hw_count)) < 0) {
			free(hws);
			return err;
		}

		free(hws);
	}
	return 0;
}

int ld10k1_fnc_send_patch_tram_grp(int data_conn, ld10k1_patch_t *patch)
{
	int i, err;
	ld10k1_dsp_tram_grp_t *grps = NULL;

	if (patch->tram_count) {
		grps = (ld10k1_dsp_tram_grp_t *)malloc(sizeof(ld10k1_dsp_tram_grp_t) * patch->tram_count);
		if (!grps)
			return LD10K1_ERR_NO_MEM;

		for (i = 0; i < patch->tram_count; i++) {
			grps[i].grp_type = patch->tram_grp[i].grp_type;
			grps[i].grp_size = patch->tram_grp[i].grp_size;
			grps[i].grp_pos = patch->tram_grp[i].grp_pos;
		}

		if ((err = send_response(data_conn, FNC_CONTINUE, 0, grps, sizeof(ld10k1_dsp_tram_grp_t) * patch->tram_count)) < 0) {
			free(grps);
			return err;
		}

		free(grps);
	}
	return 0;
}

int ld10k1_fnc_send_patch_tram_acc(int data_conn, ld10k1_patch_t *patch)
{
	int i, err;
	ld10k1_dsp_tram_acc_t *accs = NULL;

	if (patch->tram_acc_count) {
		accs = (ld10k1_dsp_tram_acc_t *)malloc(sizeof(ld10k1_dsp_tram_acc_t) * patch->tram_acc_count);
		if (!accs)
			return LD10K1_ERR_NO_MEM;

		for (i = 0; i < patch->tram_acc_count; i++) {
			accs[i].acc_type = patch->tram_acc[i].acc_type;
			accs[i].acc_offset = patch->tram_acc[i].acc_offset;
			accs[i].grp = patch->tram_acc[i].grp;
		}

		if ((err = send_response(data_conn, FNC_CONTINUE, 0, accs, sizeof(ld10k1_dsp_tram_acc_t) * patch->tram_acc_count)) < 0) {
			free(accs);
			return err;
		}

		free(accs);
	}
	return 0;
}

int ld10k1_fnc_send_patch_ctl(int data_conn, ld10k1_patch_t *patch)
{
	int i, j, err;
	ld10k1_dsp_ctl_t *ctls = NULL;

	if (patch->ctl_count) {
		ctls = (ld10k1_dsp_ctl_t *)malloc(sizeof(ld10k1_dsp_ctl_t) * patch->ctl_count);
		if (!ctls)
			return LD10K1_ERR_NO_MEM;
		memset(ctls, 0, sizeof(ld10k1_dsp_ctl_t) * patch->ctl_count);

		for (i = 0; i < patch->ctl_count; i++) {
			strncpy(ctls[i].name, patch->ctl[i].name, 43);
			ctls[i].name[43] = '\0';
			ctls[i].index = patch->ctl[i].want_index;
			ctls[i].count = patch->ctl[i].count;
			ctls[i].vcount = patch->ctl[i].vcount;
			ctls[i].min = patch->ctl[i].min;
			ctls[i].max = patch->ctl[i].max;
			ctls[i].translation = patch->ctl[i].translation;

			for (j = 0; j < ctls[i].count; j++)
				ctls[i].value[j] = patch->ctl[i].value[j];
		}

		if ((err = send_response(data_conn, FNC_CONTINUE, 0, ctls, sizeof(ld10k1_dsp_ctl_t) * patch->ctl_count)) < 0) {
			free(ctls);
			return err;
		}

		free(ctls);
	}
	return 0;
}

int ld10k1_fnc_send_patch_instr(int data_conn, ld10k1_patch_t *patch)
{
	int i, j, err;
	ld10k1_dsp_instr_t *instrs = NULL;

	if (patch->instr_count) {
		instrs = (ld10k1_dsp_instr_t *)malloc(sizeof(ld10k1_dsp_instr_t) * patch->instr_count);
		if (!instrs)
			return LD10K1_ERR_NO_MEM;

		for (i = 0; i < patch->instr_count; i++) {
			instrs[i].op_code = patch->instr[i].op_code;
			for (j = 0; j < 4; j++)
				instrs[i].arg[j] = patch->instr[i].arg[j];
		}

		if ((err = send_response(data_conn, FNC_CONTINUE, 0, instrs, sizeof(ld10k1_dsp_instr_t) * patch->instr_count)) < 0) {
			free(instrs);
			return err;
		}

		free(instrs);
	}
	return 0;
}


int ld10k1_fnc_get_patch(int data_conn, int op, int size)
{
	int err;

	ld10k1_dsp_patch_t patch_info;
	int patch_num = -1;
	ld10k1_patch_t *patch;

	if ((err = receive_msg_data(data_conn, &patch_num, sizeof(patch_num))) < 0)
		return err;

	if (dsp_mgr.patch_count >= 0) {

		if (patch_num > EMU10K1_PATCH_MAX)
			return LD10K1_ERR_UNKNOWN_PATCH_NUM;
		patch = dsp_mgr.patch_ptr[patch_num];
		if (!patch)
			return LD10K1_ERR_UNKNOWN_PATCH_NUM;

		strcpy(patch_info.patch_name, patch->patch_name);
		patch_info.id = patch->id;
		patch_info.in_count = patch->in_count;
		patch_info.out_count = patch->out_count;
		patch_info.const_count = patch->const_count;
		patch_info.static_count = patch->sta_count;
		patch_info.dynamic_count = patch->dyn_count;
		patch_info.hw_count = patch->hw_count;
		patch_info.tram_count = patch->tram_count;
		patch_info.tram_acc_count = patch->tram_acc_count;
		patch_info.ctl_count = patch->ctl_count;
		patch_info.instr_count = patch->instr_count;

		if ((err = send_response(data_conn, FNC_CONTINUE, 0, &patch_info, sizeof(ld10k1_dsp_patch_t))) < 0)
			return err;

  		/* send next parts */
		if ((err = ld10k1_fnc_send_patch_in(data_conn, patch)) < 0)
			return err;

		if ((err = ld10k1_fnc_send_patch_out(data_conn, patch)) < 0)
			return err;

		if ((err = ld10k1_fnc_send_patch_const(data_conn, patch)) < 0)
			return err;

		if ((err = ld10k1_fnc_send_patch_sta(data_conn, patch)) < 0)
			return err;

		if ((err = ld10k1_fnc_send_patch_hw(data_conn, patch)) < 0)
			return err;

		if ((err = ld10k1_fnc_send_patch_tram_grp(data_conn, patch)) < 0)
			return err;

		if ((err = ld10k1_fnc_send_patch_tram_acc(data_conn, patch)) < 0)
			return err;

		if ((err = ld10k1_fnc_send_patch_ctl(data_conn, patch)) < 0)
			return err;

		if ((err = ld10k1_fnc_send_patch_instr(data_conn, patch)) < 0)
			return err;
	}

	return 0;
}

int ld10k1_fnc_dump(int data_conn, int op, int size)
{
	int err;
	void *dump = NULL;
	int dump_size = 0;


	if ((err = ld10k1_make_dump(&dsp_mgr, &dump, &dump_size)) < 0)
		return err;

	if ((err = send_response(data_conn, FNC_CONTINUE, 0, dump, dump_size)) < 0)
		return err;

	free(dump);
	return 0;
}

int ld10k1_fnc_get_points_info(int data_conn, int op, int size)
{
	int point_count;
	int i, j = 0;
	int *info = NULL;
	ld10k1_conn_point_t *point;
	
	point_count = 0;
	for (i = 0; i < 2; i++) {
		if (i) {
			if (!point_count)
				break;
			info = (int *)malloc(sizeof(int) * point_count);
			if (!info)
				return LD10K1_ERR_NO_MEM;
			j = 0;
		}
		point = dsp_mgr.point_list;
		while (point) {
			if (!i)
				point_count++;
			else
				info[j++] = point->id;

			point = point->next;
		}
	}

	return send_response_wd(data_conn, info, sizeof(int) * point_count);
}

int ld10k1_fnc_get_point_info(int data_conn, int op, int size)
{
	int err;

	int k, l;
	ld10k1_dsp_point_t info;
	ld10k1_conn_point_t *point;
	ld10k1_conn_point_t *found_point;
	int what_point_id;

	/*info = NULL;*/

	if ((err = receive_msg_data(data_conn, &what_point_id, sizeof(int))) < 0)
		return err;

	found_point = NULL;
	
	point = dsp_mgr.point_list;
	while (point) {
		if (point->id == what_point_id) {
			found_point = point;
			break;
		}
		point = point->next;
	}
	
	if (!found_point)
		return LD10K1_ERR_UNKNOWN_POINT;
	
	info.id = point->id;
			
	if (EMU10K1_REG_TYPE_B(point->con_gpr_idx) == EMU10K1_REG_TYPE_NORMAL)
		info.type = CON_IO_NORMAL;
	else if (EMU10K1_REG_TYPE_B(point->con_gpr_idx) == EMU10K1_REG_TYPE_INPUT)
		info.type = CON_IO_IN;
	else if (EMU10K1_REG_TYPE_B(point->con_gpr_idx) == EMU10K1_REG_TYPE_OUTPUT)
		info.type = CON_IO_OUT;
	else if (EMU10K1_REG_TYPE_B(point->con_gpr_idx) == EMU10K1_REG_TYPE_FX)
		info.type = CON_IO_FX;
	info.io_idx = point->con_gpr_idx & ~EMU10K1_REG_TYPE_MASK;
	info.simple = point->simple;
	info.conn_count = point->con_count;
	if (info.conn_count > 2 && info.type == CON_IO_NORMAL)
		info.multi = 1;
	else if (info.conn_count > 1 && info.type != CON_IO_NORMAL)
		info.multi = 1;
	else
		info.multi = 0;
	for (k = 0, l = 0; k < POINT_MAX_CONN_PER_POINT; k++) {
		if (point->type[k]) {
			info.io_type[l] = point->type[k] == CON_IO_PIN ? 0 : 1;
			info.patch[l] = point->patch[k] ? point->patch[k]->id : -1;
			info.io[l] = point->io[k];
			l++;
		}
	}

	return send_response_wd(data_conn, &info, sizeof(ld10k1_dsp_point_t));
}

int ld10k1_fnc_get_dsp_info(int data_conn, int op, int size)
{
	ld10k1_fnc_dsp_info_t info;
	
	info.chip_type = dsp_mgr.audigy;
	
	return send_response_wd(data_conn, &info, sizeof(ld10k1_fnc_dsp_info_t));
}
