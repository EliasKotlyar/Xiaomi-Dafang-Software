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
 
#ifndef __COMM_H
#define __COMM_H

#ifdef __cplusplus
extern "C" {
#endif

struct msg_req
{
	int op;
	int size;
};

struct msg_resp
{
	int op;
	int err;
	int size;
};

#define COMM_TYPE_LOCAL 0
#define COMM_TYPE_IP 1

typedef struct
{
	int type;
	int server;
	char *name;
	unsigned short port;
	int wfc;
} comm_param;

int setup_comm(comm_param *param);
int connect_comm(int conn_num, comm_param *param);
int listen_comm(int conn_num);
int accept_comm(int conn_num);
int free_comm(int conn_num);
int send_request(int conn_num, int op, void *data, int data_size);
int send_response(int conn_num, int op, int err, void *data, int data_size);
int send_msg_data(int conn_num, void *data, int data_size);
int receive_request(int conn_num, int *op, int *data_size);
int receive_response(int conn_num, int *op, int *data_size);
int receive_msg_data(int conn_num, void *data, int data_size);
void *receive_msg_data_malloc(int conn_num, int data_size);

#ifdef __cplusplus
}
#endif

#endif /* __COMM_H */
