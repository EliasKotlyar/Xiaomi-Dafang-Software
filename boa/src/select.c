/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <paulp@go2net.com>
 *  Some changes Copyright (C) 1996 Charles F. Randall <crandall@goldsys.com>
 *  Copyright (C) 1996-1999 Larry Doolittle <ldoolitt@boa.org>
 *  Copyright (C) 1996-2003 Jon Nelson <jnelson@boa.org>
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

/* $Id: select.c,v 1.1.2.17 2005/02/22 14:11:29 jnelson Exp $*/

/* algorithm:
 * handle any signals
 * if we still want to accept new connections, add the server to the
 * list.
 * if there are any blocked requests or the we are still accepting new
 * connections, determine appropriate timeout and select, then move
 * blocked requests back into the active list.
 * handle active connections
 * repeat
 */



#include "boa.h"

static void fdset_update(void);
fd_set block_read_fdset;
fd_set block_write_fdset;
int max_fd = 0;

void loop(int server_s)
{
    FD_ZERO(BOA_READ);
    FD_ZERO(BOA_WRITE);

    max_fd = -1;

    while (1) {
        /* handle signals here */
        if (sighup_flag)
            sighup_run();
        if (sigchld_flag)
            sigchld_run();
        if (sigalrm_flag)
            sigalrm_run();

        if (sigterm_flag) {
            /* sigterm_flag:
             * 1. caught, unprocessed.
             * 2. caught, stage 1 processed
             */
            if (sigterm_flag == 1) {
                sigterm_stage1_run();
                BOA_FD_CLR(req, server_s, BOA_READ);
                close(server_s);
                /* make sure the server isn't in the block list */
                server_s = -1;
            }
            if (sigterm_flag == 2 && !request_ready && !request_block) {
                sigterm_stage2_run(); /* terminal */
            }
        } else {
            if (total_connections > max_connections) {
                /* FIXME: for poll we don't subtract 20. why? */
                BOA_FD_CLR(req, server_s, BOA_READ);
            } else {
                BOA_FD_SET(req, server_s, BOA_READ); /* server always set */
            }
        }

        pending_requests = 0;
        /* max_fd is > 0 when something is blocked */

        if (max_fd) {
            struct timeval req_timeout; /* timeval for select */

            req_timeout.tv_sec = (request_ready ? 0 : default_timeout);
            req_timeout.tv_usec = 0l; /* reset timeout */

            if (select(max_fd + 1, BOA_READ,
                       BOA_WRITE, NULL,
                       (request_ready || request_block ?
                        &req_timeout : NULL)) == -1) {
                /* what is the appropriate thing to do here on EBADF */
                if (errno == EINTR)
                    continue;       /* while(1) */
                else if (errno != EBADF) {
                    DIE("select");
                }
            }
            /* FIXME: optimize for when select returns 0 (timeout).
             * Thus avoiding many operations in fdset_update
             * and others.
             */
            if (!sigterm_flag && FD_ISSET(server_s, BOA_READ)) {
                pending_requests = 1;
            }
            time(&current_time); /* for "new" requests if we've been in
            * select too long */
            /* if we skip this section (for example, if max_fd == 0),
             * then we aren't listening anyway, so we can't accept
             * new conns.  Don't worry about it.
             */
        }

        /* reset max_fd */
        max_fd = -1;

        if (request_block) {
            /* move selected req's from request_block to request_ready */
            fdset_update();
        }

        /* any blocked req's move from request_ready to request_block */
        if (pending_requests || request_ready) {
            process_requests(server_s);
        }
    }
}

/*
 * Name: fdset_update
 *
 * Description: iterate through the blocked requests, checking whether
 * that file descriptor has been set by select.  Update the fd_set to
 * reflect current status.
 *
 * Here, we need to do some things:
 *  - keepalive timeouts simply close
 *    (this is special:: a keepalive timeout is a timeout where
       keepalive is active but nothing has been read yet)
 *  - regular timeouts close + error
 *  - stuff in buffer and fd ready?  write it out
 *  - fd ready for other actions?  do them
 */

static void fdset_update(void)
{
    request *current, *next;

    time(&current_time);
    for (current = request_block; current; current = next) {
        time_t time_since = current_time - current->time_last;
        next = current->next;

        /* hmm, what if we are in "the middle" of a request and not
         * just waiting for a new one... perhaps check to see if anything
         * has been read via header position, etc... */
        if (current->kacount < ka_max && /* we *are* in a keepalive */
            (time_since >= ka_timeout) && /* ka timeout */
            !current->logline) { /* haven't read anything yet */
            log_error_doc(current);
            fputs("connection timed out\n", stderr);
            current->status = TIMED_OUT; /* connection timed out */
        } else if (time_since > REQUEST_TIMEOUT) {
            log_error_doc(current);
            fputs("connection timed out\n", stderr);
            current->status = TIMED_OUT; /* connection timed out */
        }
        if (current->buffer_end && /* there is data to write */
            current->status < DONE) {
            if (FD_ISSET(current->fd, BOA_WRITE))
                ready_request(current);
            else {
                BOA_FD_SET(current, current->fd, BOA_WRITE);
            }
        } else {
            switch (current->status) {
            case IOSHUFFLE:
#ifndef HAVE_SENDFILE
                if (current->buffer_end - current->buffer_start == 0) {
                    if (FD_ISSET(current->data_fd, BOA_READ))
                        ready_request(current);
                    break;
                }
#endif
            case WRITE:
            case PIPE_WRITE:
                if (FD_ISSET(current->fd, BOA_WRITE))
                    ready_request(current);
                else {
                    BOA_FD_SET(current, current->fd, BOA_WRITE);
                }
                break;
            case BODY_WRITE:
                if (FD_ISSET(current->post_data_fd, BOA_WRITE))
                    ready_request(current);
                else {
                    BOA_FD_SET(current, current->post_data_fd,
                               BOA_WRITE);
                }
                break;
            case PIPE_READ:
                if (FD_ISSET(current->data_fd, BOA_READ))
                    ready_request(current);
                else {
                    BOA_FD_SET(current, current->data_fd,
                               BOA_READ);
                }
                break;
            case DONE:
                if (FD_ISSET(current->fd, BOA_WRITE))
                    ready_request(current);
                else {
                    BOA_FD_SET(current, current->fd, BOA_WRITE);
                }
                break;
            case TIMED_OUT:
            case DEAD:
                ready_request(current);
                break;
            default:
                if (FD_ISSET(current->fd, BOA_READ))
                    ready_request(current);
                else {
                    BOA_FD_SET(current, current->fd, BOA_READ);
                }
                break;
            }
        }
        current = next;
    }
}
