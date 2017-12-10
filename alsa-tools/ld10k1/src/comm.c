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
 
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>

#include "comm.h"
#include "ld10k1_error.h"

/* taken from glibc example */
int setup_comm(comm_param *param)
{
	int sock;
	struct sockaddr_un lname;
	struct sockaddr_in iname;
	size_t size;

	/* Create the socket. */
	if (param->type == COMM_TYPE_LOCAL)
		sock = socket (PF_LOCAL, SOCK_STREAM, 0);
	else
		sock = socket (PF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		return -1;

	if (param->server)	{
		if (param->type == COMM_TYPE_LOCAL) {
			unlink(param->name);

			/* Bind a name to the socket. */
			memset(&lname, 0, sizeof(struct sockaddr_un));
			lname.sun_family = AF_LOCAL;
			strncpy (lname.sun_path, param->name, sizeof (lname.sun_path) - 1);
			lname.sun_path[sizeof (lname.sun_path) - 1] = '\0';

			/* The size of the address is
			the offset of the start of the filename,
			plus its length,
			plus one for the terminating null byte.
			Alternatively you can just do:
			size = SUN_LEN (&name);
			*/
			size = (offsetof (struct sockaddr_un, sun_path)	+ strlen (lname.sun_path) + 1);

			if (bind (sock, (struct sockaddr *) &lname, size) < 0)
				return -1;

			chmod(param->name, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
		} else {
			/* Give the socket a name. */
			memset(&iname, 0, sizeof(struct sockaddr_in));
			iname.sin_family = AF_INET;
			iname.sin_port = htons (param->port);
			iname.sin_addr.s_addr = htonl(INADDR_ANY);
			if (bind(sock, (struct sockaddr *) &iname, sizeof (iname)) < 0)
				return -1;
		}
	}

	return sock;
}

int connect_comm(int conn_num, comm_param *param)
{
	struct sockaddr_un lname;
	struct sockaddr_in iname;
	struct hostent *hostinfo;
	size_t size;
	
	int attempt;
	int max_attempt;
	int not_connected;
	
	attempt = 0;
	if (param->wfc)
		max_attempt = param->wfc / 10;
	else
		max_attempt = 0;

	if (param->type == COMM_TYPE_LOCAL) {
		memset(&lname, 0, sizeof(struct sockaddr_un));
		lname.sun_family = AF_LOCAL;
		strncpy (lname.sun_path, param->name, sizeof (lname.sun_path) - 1);
		lname.sun_path[sizeof(lname.sun_path) - 1] = '\0';

		size = (offsetof(struct sockaddr_un, sun_path)) + strlen(lname.sun_path) + 1;

		while (1)
		{
			not_connected = connect(conn_num, (struct sockaddr *) &lname, size);
			if (!not_connected)
				break;
			if (attempt >= max_attempt)
				return -1;
			attempt++;
			usleep(10000);
		}
	} else {
		memset(&iname, 0, sizeof(struct sockaddr_in));
		iname.sin_family = AF_INET;
		iname.sin_port = htons(param->port);
		hostinfo = gethostbyname(param->name);
		if (hostinfo == NULL)
			return -1;
		iname.sin_addr = *(struct in_addr *)hostinfo->h_addr;
		
		while (1)
		{
			not_connected = connect(conn_num, (struct sockaddr *) &iname, sizeof(struct sockaddr_in));
			if (!not_connected)
				break;
			if (attempt >= max_attempt)
				return -1;
			attempt++;
			usleep(10000);
		}
	}
	return 0;
}

int listen_comm(int conn_num)
{
	if (listen(conn_num, 1) < 0)
		return -1;
	return 0;
}

int accept_comm(int conn_num)
{
	struct sockaddr addr;
	socklen_t addr_len;
	int sock;

	addr_len = sizeof(addr);
	
	sock = accept(conn_num, &addr, &addr_len);
	
	if (sock < 0)
		return -1;

	return sock;
}

int free_comm(int conn_num)
{	
	if (shutdown(conn_num, 2))
		return -1;
	if (close(conn_num) < 0)
		return -1;

	return 0;
}

#define MAX_ATEMPT 5

int read_all(int conn_num, void *data, int data_size)
{
	int offset = 0;
	int how_much = data_size;
	int atempt = 0;
	int readed = 0;
	
	while (atempt < MAX_ATEMPT && how_much > 0)	{
		readed = read(conn_num, ((char *)data) + offset, how_much);
		if (readed < 0)
			return LD10K1_ERR_COMM_READ;
		offset += readed;
		how_much -= readed;
		atempt++;
		if (how_much > 0)
			usleep(10000);
	}
	
	if (how_much > 0)
		return LD10K1_ERR_COMM_READ;
	else
		return data_size;
}

int write_all(int conn_num, void *data, int data_size)
{
	int offset = 0;
	int how_much = data_size;
	int atempt = 0;
	int writed = 0;
	
	while (atempt < MAX_ATEMPT && how_much > 0)	{
		writed = write(conn_num, ((char *)data) + offset, how_much);
		if (writed < 0)
			return LD10K1_ERR_COMM_WRITE;
		offset += writed;
		how_much -= writed;
		atempt++;
		if (how_much > 0)
			usleep(50000);
	}
	
	if (how_much > 0)
		return LD10K1_ERR_COMM_WRITE;
	else
		return data_size;
}

int send_request(int conn_num, int op, void *data, int data_size)
{
	int nbytes;
	struct msg_req header;

	header.op = op;
	header.size = data_size;

	/* header */
	nbytes = write_all(conn_num, &header, sizeof(header));
	if (nbytes < 0)
		return nbytes;

	if (data_size > 0) {
		/* data */
		nbytes = write_all(conn_num, data, data_size);
		if (nbytes < 0)
			return nbytes;
	}
	return 0;
}

int send_response(int conn_num, int op, int err, void *data, int data_size)
{
	int nbytes;
	struct msg_resp header;

	header.op = op;
	header.err = err;
	header.size = data_size;

	/* header */
	nbytes = write_all(conn_num, &header, sizeof(header));
	if (nbytes < 0)
		return nbytes;

	if (data_size > 0) {
		/* data */
		nbytes = write_all(conn_num, data, data_size);
		if (nbytes < 0)
			return nbytes;
	}
	return 0;
}

int send_msg_data(int conn_num, void *data, int data_size)
{
	int nbytes;

	if (data_size > 0) {
		/* data */
		nbytes = write_all(conn_num, data, data_size);
		if (nbytes < 0)
			return nbytes;
	}
	return 0;
}

int receive_request(int conn_num, int *op, int *data_size)
{
	struct msg_req header;
	int nbytes;
	
	nbytes = read_all(conn_num, &header, sizeof(header));
	if (nbytes < 0)
		return nbytes;
	if (nbytes == 0) {
		*op = -1;
		*data_size = 0;
		return 0;
	}
	*op = header.op;
	*data_size = header.size;
	return 0;
}

int receive_response(int conn_num, int *op, int *data_size)
{
	struct msg_resp header;
	int nbytes;
	
	nbytes = read_all(conn_num, &header, sizeof(header));
	if (nbytes < 0)
		return nbytes;
	if (nbytes == 0) {
		*op = -1;
		*data_size = 0;
		return 0;
	}
	*op = header.op;
	*data_size = header.size;
	if (header.err < 0)
		return header.err;
	return 0;
}

int receive_msg_data(int conn_num, void *data, int data_size)
{
	int nbytes;
	nbytes = read_all(conn_num, data, data_size);
	if (nbytes < 0)
		return nbytes;
	return 0;
}

void *receive_msg_data_malloc(int conn_num, int data_size)
{
	void *tmp;
	
	tmp = malloc(data_size);
	if (!tmp)
		return NULL;
		
	if (receive_msg_data(conn_num, tmp, data_size)) {
		free(tmp);
		return NULL;	
	}
	return tmp;
}
