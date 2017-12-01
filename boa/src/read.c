/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <paulp@go2net.com>
 *  Copyright (C) 1996-2005 Larry Doolittle <ldoolitt@boa.org>
 *  Copyright (C) 1997-2004 Jon Nelson <jnelson@boa.org>
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

/* $Id: read.c,v 1.49.2.14 2005/02/23 15:41:55 jnelson Exp $*/

#include "boa.h"

/*
 * Name: read_header
 * Description: Reads data from a request socket.  Manages the current
 * status via a state machine.  Changes status from READ_HEADER to
 * READ_BODY or WRITE as necessary.
 *
 * Return values:
 *  -1: request blocked, move to blocked queue
 *   0: request done, close it down
 *   1: more to do, leave on ready list
 */

int read_header(request * req)
{
    int bytes;
    char *check, *buffer;
    unsigned char uc;

    check = req->client_stream + req->parse_pos;
    buffer = req->client_stream;
    bytes = req->client_stream_pos;

    DEBUG(DEBUG_HEADER_READ) {
        if (check < (buffer + bytes)) {
            buffer[bytes] = '\0';
            log_error_time();
            fprintf(stderr, "%s:%d - Parsing headers (\"%s\")\n",
                    __FILE__, __LINE__, check);
        }
    }
    while (check < (buffer + bytes)) {
        /* check for illegal characters here
         * Anything except CR, LF, and US-ASCII - control is legal
         * We accept tab but don't do anything special with it.
         */
        uc = *check;
	if (uc != '\r' && uc != '\n' && uc != '\t' &&
	    (uc < 32 || uc > 127)) {
            log_error_doc(req);
            fprintf(stderr, "Illegal character (%d) in stream.\n", (unsigned int) uc);
            send_r_bad_request(req);
            return 0;
        }
        switch (req->status) {
        case READ_HEADER:
            if (uc == '\r') {
                req->status = ONE_CR;
                req->header_end = check;
            } else if (uc == '\n') {
                req->status = ONE_LF;
                req->header_end = check;
            }
            break;

        case ONE_CR:
            if (uc == '\n')
                req->status = ONE_LF;
            else if (uc != '\r')
                req->status = READ_HEADER;
            break;

        case ONE_LF:
            /* if here, we've found the end (for sure) of a header */
            if (uc == '\r') /* could be end o headers */
                req->status = TWO_CR;
            else if (uc == '\n')
                req->status = BODY_READ;
            else
                req->status = READ_HEADER;
            break;

        case TWO_CR:
            if (uc == '\n')
                req->status = BODY_READ;
            else if (uc != '\r')
                req->status = READ_HEADER;
            break;

        default:
            break;
        }

#ifdef VERY_FASCIST_LOGGING
        log_error_time();
        fprintf(stderr, "status, check (unsigned): %d, %d\n", req->status, uc);
#endif

        req->parse_pos++;       /* update parse position */
        check++;

        if (req->status == ONE_LF) {
            *req->header_end = '\0';

            if (req->header_end - req->header_line >= MAX_HEADER_LENGTH) {
                log_error_doc(req);
                fprintf(stderr, "Header too long at %lu bytes: \"%s\"\n",
                        (unsigned long) (req->header_end - req->header_line),
                        req->header_line);
                send_r_bad_request(req);
                return 0;
            }

            /* terminate string that begins at req->header_line */

            if (req->logline) {
                if (process_option_line(req) == 0) {
                    /* errors already logged */
                    return 0;
                }
            } else {
                if (process_logline(req) == 0)
                    /* errors already logged */
                    return 0;
                if (req->http_version == HTTP09)
                    return process_header_end(req);
            }
            /* set header_line to point to beginning of new header */
            req->header_line = check;
        } else if (req->status == BODY_READ) {
#ifdef VERY_FASCIST_LOGGING
            int retval;
            log_error_time();
            fprintf(stderr, "%s:%d -- got to body read.\n",
                    __FILE__, __LINE__);
            retval = process_header_end(req);
#else
            int retval = process_header_end(req);
#endif
            /* process_header_end inits non-POST CGIs */

            if (retval && req->method == M_POST) {
                /* for body_{read,write}, set header_line to start of data,
                   and header_end to end of data */
                req->header_line = check;
                req->header_end =
                    req->client_stream + req->client_stream_pos;

                req->status = BODY_WRITE;
                /* so write it */
                /* have to write first, or read will be confused
                 * because of the special case where the
                 * filesize is less than we have already read.
                 */

                /*

                   As quoted from RFC1945:

                   A valid Content-Length is required on all HTTP/1.0 POST requests. An
                   HTTP/1.0 server should respond with a 400 (bad request) message if it
                   cannot determine the length of the request message's content.

                 */

                if (req->content_length) {
                    int content_length;

                    content_length = boa_atoi(req->content_length);
                    /* Is a content-length of 0 legal? */
                    if (content_length < 0) {
                        log_error_doc(req);
                        fprintf(stderr,
                                "Invalid Content-Length [%s] on POST!\n",
                                req->content_length);
                        send_r_bad_request(req);
                        return 0;
                    }
                    if (single_post_limit
                        && content_length > single_post_limit) {
                        log_error_doc(req);
                        fprintf(stderr,
                                "Content-Length [%d] > SinglePostLimit [%d] on POST!\n",
                                content_length, single_post_limit);
                        send_r_bad_request(req);
                        return 0;
                    }
                    req->filesize = content_length;
                    req->filepos = 0;
                    if ((unsigned) (req->header_end - req->header_line) > req->filesize) {
                        req->header_end = req->header_line + req->filesize;
                    }
                } else {
                    log_error_doc(req);
                    fprintf(stderr, "Unknown Content-Length POST!\n");
                    send_r_bad_request(req);
                    return 0;
                }
            }                   /* either process_header_end failed or req->method != POST */
            return retval;      /* 0 - close it done, 1 - keep on ready */
        }                       /* req->status == BODY_READ */
    }                           /* done processing available buffer */

#ifdef VERY_FASCIST_LOGGING
    log_error_time();
    fprintf(stderr, "%s:%d - Done processing buffer.  Status: %d\n",
            __FILE__, __LINE__, req->status);
#endif

    if (req->status < BODY_READ) {
        /* only reached if request is split across more than one packet */
        unsigned int buf_bytes_left;

        buf_bytes_left = CLIENT_STREAM_SIZE - req->client_stream_pos;
        if (buf_bytes_left < 1 || buf_bytes_left > CLIENT_STREAM_SIZE) {
            log_error_doc(req);
            fputs("No space left in client stream buffer, closing\n",
                  stderr);
            req->response_status = 400;
            req->status = DEAD;
            return 0;
        }

        bytes =
            read(req->fd, buffer + req->client_stream_pos, buf_bytes_left);

        if (bytes < 0) {
            if (errno == EINTR)
                return 1;
            else if (errno == EAGAIN || errno == EWOULDBLOCK) /* request blocked */
                return -1;
            log_error_doc(req);
            perror("header read"); /* don't need to save errno because log_error_doc does */
            req->response_status = 400;
            return 0;
        } else if (bytes == 0) {
            if (req->kacount < ka_max &&
                !req->logline &&
                req->client_stream_pos == 0) {
                /* A keepalive request wherein we've read
                 * nothing.
                 * Ignore.
                 */
                ;
            } else {
#ifndef QUIET_DISCONNECT
                log_error_doc(req);
                fputs("client unexpectedly closed connection.\n", stderr);
#endif
            }
            req->response_status = 400;
            return 0;
        }

        /* bytes is positive */
        req->client_stream_pos += bytes;

        DEBUG(DEBUG_HEADER_READ) {
            log_error_time();
            req->client_stream[req->client_stream_pos] = '\0';
            fprintf(stderr, "%s:%d -- We read %d bytes: \"%s\"\n",
                    __FILE__, __LINE__, bytes,
#ifdef VERY_FASCIST_LOGGING2
                    req->client_stream + req->client_stream_pos - bytes
#else
                    ""
#endif
                   );
        }

        return 1;
    }
    return 1;
}

/*
 * Name: read_body
 * Description: Reads body from a request socket for POST CGI
 *
 * Return values:
 *
 *  -1: request blocked, move to blocked queue
 *   0: request done, close it down
 *   1: more to do, leave on ready list
 *

 As quoted from RFC1945:

 A valid Content-Length is required on all HTTP/1.0 POST requests. An
 HTTP/1.0 server should respond with a 400 (bad request) message if it
 cannot determine the length of the request message's content.

 */

int read_body(request * req)
{
    int bytes_read;
    unsigned int bytes_to_read, bytes_free;

    bytes_free = BUFFER_SIZE - (req->header_end - req->header_line);
    bytes_to_read = req->filesize - req->filepos;

    if (bytes_to_read > bytes_free)
        bytes_to_read = bytes_free;

    if (bytes_to_read <= 0) {
        req->status = BODY_WRITE; /* go write it */
        return 1;
    }

    bytes_read = read(req->fd, req->header_end, bytes_to_read);

    if (bytes_read == -1) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            return -1;
        } else {
            boa_perror(req, "read body");
            req->response_status = 400;
            return 0;
        }
    } else if (bytes_read == 0) {
        /* this is an error.  premature end of body! */
        log_error_doc(req);
        fprintf(stderr, "%s:%d - Premature end of body!!\n",
                __FILE__, __LINE__);
        send_r_bad_request(req);
        return 0;
    }

    req->status = BODY_WRITE;

#ifdef FASCIST_LOGGING1
    log_error_time();
    fprintf(stderr, "%s:%d - read %d bytes.\n",
            __FILE__, __LINE__, bytes_to_read);
#endif

    req->header_end += bytes_read;

    return 1;
}

/*
 * Name: write_body
 * Description: Writes a chunk of data to a file
 *
 * Return values:
 *  -1: request blocked, move to blocked queue
 *   0: EOF or error, close it down
 *   1: successful write, recycle in ready queue
 */

int write_body(request * req)
{
    int bytes_written;
    unsigned int bytes_to_write = req->header_end - req->header_line;

    if (req->filepos + bytes_to_write > req->filesize)
        bytes_to_write = req->filesize - req->filepos;

    if (bytes_to_write == 0) {  /* nothing left in buffer to write */
        req->header_line = req->header_end = req->buffer;
        if (req->filepos >= req->filesize)
            return init_cgi(req);
        /* if here, we can safely assume that there is more to read */
        req->status = BODY_READ;
        return 1;
    }
    bytes_written = write(req->post_data_fd,
                          req->header_line, bytes_to_write);

    if (bytes_written < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
            return -1;          /* request blocked at the pipe level, but keep going */
        else if (errno == EINTR)
            return 1;
        else if (errno == ENOSPC) {
            /* 20010520 - Alfred Fluckiger */
            /* No test was originally done in this case, which might  */
            /* lead to a "no space left on device" error.             */
            boa_perror(req, "write body"); /* OK to disable if your logs get too big */
            return 0;
        } else {
            boa_perror(req, "write body"); /* OK to disable if your logs get too big */
            return 0;
        }
    }
    DEBUG(DEBUG_HEADER_READ) {
        log_error_time();
        fprintf(stderr, "%s:%d - wrote %d bytes of CGI body. %ld of %ld\n",
                __FILE__, __LINE__,
                bytes_written, req->filepos, req->filesize);
    }

    req->filepos += bytes_written;
    req->header_line += bytes_written;

    DEBUG(DEBUG_CGI_INPUT) {
        log_error_time();
        {
            char c = req->header_line[bytes_written];

            req->header_line[bytes_written] = '\0';
            fprintf(stderr,
                    "%s:%d - wrote %d bytes (%s). %lu of %lu\n",
                    __FILE__, __LINE__, bytes_written,
                    req->header_line, req->filepos, req->filesize);
            req->header_line[bytes_written] = c;
        }
    }

    return 1;                   /* more to do */
}
