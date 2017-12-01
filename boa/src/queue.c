/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <paulp@go2net.com>
 *  Copyright (C) 1997-2002 Jon Nelson <jnelson@boa.org>
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

/* $Id: queue.c,v 1.21.2.4 2005/02/22 14:11:29 jnelson Exp $*/

#include "boa.h"

request *request_ready = NULL;  /* ready list head */
request *request_block = NULL;  /* blocked list head */
request *request_free = NULL;   /* free list head */

/*
 * Name: block_request
 *
 * Description: Moves a request from the ready queue to the blocked queue
 */

void block_request(request * req)
{
    dequeue(&request_ready, req);
    enqueue(&request_block, req);

    if (req->buffer_end) {
        BOA_FD_SET(req, req->fd, BOA_WRITE);
    } else {
        switch (req->status) {
        case IOSHUFFLE:
#ifndef HAVE_SENDFILE
            if (req->buffer_end - req->buffer_start == 0) {
                BOA_FD_SET(req, req->data_fd, BOA_READ);
                break;
            }
#endif
        case WRITE:
        case PIPE_WRITE:
        case DONE:
            BOA_FD_SET(req, req->fd, BOA_WRITE);
            break;
        case PIPE_READ:
            BOA_FD_SET(req, req->data_fd, BOA_READ);
            break;
        case BODY_WRITE:
            BOA_FD_SET(req, req->post_data_fd, BOA_WRITE);
            break;
        default:
            BOA_FD_SET(req, req->fd, BOA_READ);
            break;
        }
    }
}

/*
 * Name: ready_request
 *
 * Description: Moves a request from the blocked queue to the ready queue
 */

void ready_request(request * req)
{
    dequeue(&request_block, req);
    enqueue(&request_ready, req);

    if (req->buffer_end) {
        BOA_FD_CLR(req, req->fd, BOA_WRITE);
    } else {
        switch (req->status) {
        case IOSHUFFLE:
#ifndef HAVE_SENDFILE
            if (req->buffer_end - req->buffer_start == 0) {
                BOA_FD_CLR(req, req->data_fd, BOA_READ);
                break;
            }
#endif
        case WRITE:
        case PIPE_WRITE:
        case DONE:
            BOA_FD_CLR(req, req->fd, BOA_WRITE);
            break;
        case PIPE_READ:
            BOA_FD_CLR(req, req->data_fd, BOA_READ);
            break;
        case BODY_WRITE:
            BOA_FD_CLR(req, req->post_data_fd, BOA_WRITE);
            break;
        default:
            BOA_FD_CLR(req, req->fd, BOA_READ);
        }
    }
}


/*
 * Name: dequeue
 *
 * Description: Removes a request from its current queue
 */

void dequeue(request ** head, request * req)
{
    if (*head == req)
        *head = req->next;

    if (req->prev)
        req->prev->next = req->next;
    if (req->next)
        req->next->prev = req->prev;

    req->next = NULL;
    req->prev = NULL;
}

/*
 * Name: enqueue
 *
 * Description: Adds a request to the head of a queue
 */

void enqueue(request ** head, request * req)
{
    if (*head)
        (*head)->prev = req;    /* previous head's prev is us */

    req->next = *head;          /* our next is previous head */
    req->prev = NULL;           /* first in list */

    *head = req;                /* now we are head */
}
