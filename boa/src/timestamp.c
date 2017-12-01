/*
 *  Boa, an http server
 *  Copyright (C) 1998-2002 Jon Nelson <jnelson@boa.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 1, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* $Id: timestamp.c,v 1.9.2.1 2005/02/22 14:11:29 jnelson Exp $*/

#include "boa.h"

void timestamp(void)
{
    log_error_time();
    fprintf(stderr, "boa: server version %s\n", SERVER_VERSION);
    log_error_time();
    fprintf(stderr, "boa: server built " __DATE__ " at " __TIME__ ".\n");
    log_error_time();
    fprintf(stderr, "boa: starting server pid=%d, port %d\n",
            (int) getpid(), server_port);
}
