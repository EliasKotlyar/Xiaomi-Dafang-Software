
/*
 *  Boa, an http server
 *  Based on code Copyright (C) 1995 Paul Phillips <paulp@go2net.com>
 *  Copyright (C) 1997-2004 Jon Nelson <jnelson@boa.org>
 *  Copyright (C) 1997-2005 Larry Doolittle <ldoolitt@boa.org>
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

/* $Id: pipe.c,v 1.39.2.16 2005/02/22 14:13:03 jnelson Exp $*/

#include "boa.h"

/*
 * Name: read_from_pipe
 * Description: Reads data from a pipe
 *
 * Return values:
 *  -1: request blocked, move to blocked queue
 *   0: EOF or error, close it down
 *   1: successful read, recycle in ready queue
 */

int read_from_pipe(request * req)
{
    int bytes_read; /* signed */
    unsigned int bytes_to_read; /* unsigned */

    bytes_to_read = BUFFER_SIZE - (req->header_end - req->buffer - 1);

    if (bytes_to_read == 0) {   /* buffer full */
        if (req->cgi_status == CGI_PARSE) { /* got+parsed header */
            req->cgi_status = CGI_BUFFER;
            *req->header_end = '\0'; /* points to end of read data */
            /* Could the above statement overwrite data???
               No, because req->header_end points to where new data
               should begin, not where old data is.
             */
            return process_cgi_header(req); /* cgi_status will change */
        }
        req->status = PIPE_WRITE;
        return 1;
    }

    bytes_read = read(req->data_fd, req->header_end, bytes_to_read);
#ifdef FASCIST_LOGGING
    if (bytes_read > 0) {
        *(req->header_end + bytes_read) = '\0';
        fprintf(stderr, "pipe.c - read %d bytes: \"%s\"\n",
                bytes_read, req->header_end);
    } else
        fprintf(stderr, "pipe.c - read %d bytes\n", bytes_read);
    fprintf(stderr, "status, cgi_status: %d, %d\n", req->status,
            req->cgi_status);
#endif

    if (bytes_read == -1) {
        if (errno == EINTR)
            return 1;
        else if (errno == EWOULDBLOCK || errno == EAGAIN)
            return -1;          /* request blocked at the pipe level, but keep going */
        else {
            req->status = DEAD;
            log_error_doc(req);
            perror("pipe read");
            return 0;
        }
    }
    *(req->header_end + bytes_read) = '\0';

    if (bytes_read == 0) {      /* eof, write rest of buffer */
        req->status = PIPE_WRITE;
        if (req->cgi_status == CGI_PARSE) { /* hasn't processed header yet */
            req->cgi_status = CGI_DONE;
            *req->header_end = '\0'; /* points to end of read data */
            return process_cgi_header(req); /* cgi_status will change */
        }
        req->cgi_status = CGI_DONE;
        return 1;
    }

    req->header_end += bytes_read;

    if (req->cgi_status != CGI_PARSE)
        return write_from_pipe(req); /* why not try and flush the buffer now? */
    else {
        char *c, *buf;

        buf = req->header_line;

        c = strstr(buf, "\n\r\n");
        if (c == NULL) {
            c = strstr(buf, "\n\n");
            if (c == NULL) {
                return 1;
            }
        }
        req->cgi_status = CGI_DONE;
        *req->header_end = '\0'; /* points to end of read data */
        return process_cgi_header(req); /* cgi_status will change */
    }
    return 1;
}

/*
 * Name: write_from_pipe
 * Description: Writes data previously read from a pipe
 *
 * Return values:
 *  -1: request blocked, move to blocked queue
 *   0: EOF or error, close it down
 *   1: successful write, recycle in ready queue
 */

int write_from_pipe(request * req)
{
    int bytes_written;
    size_t bytes_to_write = req->header_end - req->header_line;

    if (bytes_to_write == 0) {
        if (req->cgi_status == CGI_DONE)
            return 0;

        req->status = PIPE_READ;
        req->header_end = req->header_line = req->buffer;
        return 1;
    }

    bytes_written = write(req->fd, req->header_line, bytes_to_write);

    if (bytes_written == -1) {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
            return -1;          /* request blocked at the pipe level, but keep going */
        else if (errno == EINTR)
            return 1;
        else {
            req->status = DEAD;
            log_error_doc(req);
            perror("pipe write");
            return 0;
        }
    }

    req->header_line += bytes_written;
    req->bytes_written += bytes_written;

    /* if there won't be anything to write next time, switch state */
    if ((unsigned) bytes_written == bytes_to_write) {
        req->status = PIPE_READ;
        req->header_end = req->header_line = req->buffer;
    }

    return 1;
}

#ifdef HAVE_SENDFILE
int io_shuffle_sendfile(request * req)
{
    int bytes_written;
    size_t bytes_to_write;
    off_t sendfile_offset;

    if (req->method == M_HEAD) {
        return complete_response(req);
    }

    /* XXX trouble if range is exactly 4G on a 32-bit machine? */
    bytes_to_write = (req->ranges->stop - req->ranges->start) + 1;

    if (bytes_to_write > system_bufsize)
        bytes_to_write = system_bufsize;

retrysendfile:
    if (bytes_to_write == 0) {
        /* shouldn't get here, but... */
        bytes_written = 0;
    } else {
	/* arg 3 of sendfile should have type "off_t *"
	 * struct range element start has type "unsigned long"
	 * Where POSIX got the idea that an offset into a file
	 * should be signed, I'll never know.
	 */
	sendfile_offset = req->ranges->start;
	if (sendfile_offset < 0) {
		req->status = DEAD;
		log_error_doc(req);
		fprintf(stderr, "impossible offset (%lu) requested of sendfile\n",
				 req->ranges->start);
		return 0;
	}
        bytes_written = sendfile(req->fd, req->data_fd,
                                 &sendfile_offset,
                                 bytes_to_write);
	if (sendfile_offset < 0) {
		req->status = DEAD;
		log_error_doc(req);
		fprintf(stderr,
			"bad craziness in sendfile offset, returned %ld\n",
			(long) sendfile_offset);
		return 0;
	}
	req->ranges->start = sendfile_offset;
        if (bytes_written < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                return -1;          /* request blocked at the pipe level, but keep going */
            } else if (errno == EINTR) {
                goto retrysendfile;
            } else {
                req->status = DEAD;
#ifdef QUIET_DISCONNECT
                if (0)
#else
                if (errno != EPIPE && errno != ECONNRESET)
#endif
                {
                    log_error_doc(req);
                    perror("sendfile write");
                }
            }
            return 0;
        } else if (bytes_written == 0) {
            /* not sure how to handle this.
             * For now, treat it like it is blocked.
             */
            return -1;
        }/* bytes_written */
    } /* bytes_to_write */

    /* sendfile automatically updates req->ranges->start
     * don't touch!
     * req->ranges->start += bytes_written;
     */
    req->bytes_written += bytes_written;

    if (req->ranges->stop + 1 <= req->ranges->start) {
        return complete_response(req);
    }
    return 1;
}
#endif

/* always try to read unless data_fs is 0 (and there is space)
 * then try to write
 *
 * Return values:
 *  -1: request blocked, move to blocked queue
 *   0: EOF or error, close it down
 *   1: successful read, recycle in ready queue
 */

int io_shuffle(request * req)
{
    int bytes_to_read;
    int bytes_written, bytes_to_write;

    if (req->method == M_HEAD) {
        return complete_response(req);
    }

    /* FIXME: This function doesn't take into account req->filesize
     * when *reading* into the buffer. Grr.
     * June 09, 2004: jdn, I don't think it's a problem anymore,
     * because the ranges are verified against the filesize,
     * and we cap bytes_to_read at bytes_to_write.
     */
    bytes_to_read = BUFFER_SIZE - req->buffer_end - 256;

    bytes_to_write = (req->ranges->stop - req->ranges->start) + 1;

    if (bytes_to_read > bytes_to_write)
        bytes_to_read = bytes_to_write;

    if (bytes_to_read > 0 && req->data_fd) {
        int bytes_read;
        off_t temp;

        temp = lseek(req->data_fd, req->ranges->start, SEEK_SET);
        if (temp < 0) {
            req->status = DEAD;
            log_error_doc(req);
            perror("ioshuffle lseek");
            return 0;
        }

      restartread:
        bytes_read =
            read(req->data_fd, req->buffer + req->buffer_end,
                 bytes_to_read);

        if (bytes_read == -1) {
            if (errno == EINTR)
                goto restartread;
            else if (errno == EWOULDBLOCK || errno == EAGAIN) {
                /* not a fatal error, don't worry about it */
                /* buffer is empty, we're blocking on read! */
                if (req->buffer_end - req->buffer_start == 0)
                    return -1;
            } else {
                req->status = DEAD;
                log_error_doc(req);
                perror("ioshuffle read");
                return 0;
            }
        } else if (bytes_read == 0) { /* eof, write rest of buffer */
            close(req->data_fd);
            req->data_fd = 0;
        } else {
            req->buffer_end += bytes_read;

            req->ranges->start += bytes_read;

            if ((req->ranges->stop + 1 - req->ranges->start) == 0) {
                return complete_response(req);
            }
        }
    }

    bytes_to_write = req->buffer_end - req->buffer_start;
    if (bytes_to_write == 0) {
        if (req->data_fd == 0)
            return 0;           /* done */
        req->buffer_end = req->buffer_start = 0;
        return 1;
    }

  restartwrite:
    bytes_written =
        write(req->fd, req->buffer + req->buffer_start, bytes_to_write);

    if (bytes_written == -1) {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
            return -1;          /* request blocked at the pipe level, but keep going */
        else if (errno == EINTR)
            goto restartwrite;
        else {
            req->status = DEAD;
            log_error_doc(req);
            perror("ioshuffle write");
            return 0;
        }
    } else if (bytes_written == 0) {
    }

    req->buffer_start += bytes_written;
    req->bytes_written += bytes_written;

    if (bytes_to_write == bytes_written) {
        req->buffer_end = req->buffer_start = 0;
    }

    return 1;
}
