/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <paulp@go2net.com>
 *  Copyright (C) 1996,99 Larry Doolittle <ldoolitt@boa.org>
 *  Copyright (C) 1996-2002 Jon Nelson <jnelson@boa.org>
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

/* $Id: get.c,v 1.76.2.35 2005/02/22 14:11:29 jnelson Exp $*/

#include "boa.h"
#include "access.h"

#define STR(s) __STR(s)
#define __STR(s) #s


/* I think that permanent redirections (301) are supposed
 * to be absolute URIs, but they can be troublesome.
 * change to 1 to allow much simpler redirections.
 */
/* #define ALLOW_LOCAL_REDIRECT */

/* local prototypes */
static int get_cachedir_file(request * req, struct stat *statbuf);
static int index_directory(request * req, char *dest_filename);

/*
 * Name: init_get
 * Description: Initializes a non-script GET or HEAD request.
 *
 * Return values:
 *   0: finished or error, request will be freed
 *   1: successfully initialized, added to ready queue
 */

int init_get(request * req)
{
    int data_fd, saved_errno;
    struct stat statbuf;
    volatile unsigned int bytes_free;

    data_fd = open(req->pathname, O_RDONLY);
    saved_errno = errno;        /* might not get used */

#ifdef GUNZIP
    if (data_fd == -1 && errno == ENOENT) {
        /* cannot open */
        /* it's either a gunzipped file or a directory */
        char gzip_pathname[MAX_PATH_LENGTH];
        unsigned int len;

        len = strlen(req->pathname);

        if (len + 4 > sizeof(gzip_pathname)) {
            log_error_doc(req);
            fprintf(stderr, "Pathname + .gz too long! (%s)\n", req->pathname);
            send_r_bad_request(req);
            return 0;
        }

        memcpy(gzip_pathname, req->pathname, len);
        memcpy(gzip_pathname + len, ".gz", 3);
        gzip_pathname[len + 3] = '\0';
        data_fd = open(gzip_pathname, O_RDONLY);
        if (data_fd != -1) {
            close(data_fd);

            req->response_status = R_REQUEST_OK;
            if (req->pathname)
                free(req->pathname);
            req->pathname = strdup(gzip_pathname);
            if (!req->pathname) {
                boa_perror(req, "strdup req->pathname for gzipped filename " __FILE__ ":" STR(__LINE__));
                return 0;
            }
            if (req->http_version != HTTP09) {
                req_write(req, http_ver_string(req->http_version));
                req_write(req, " 200 OK-GUNZIP" CRLF);
                print_http_headers(req);
                print_content_type(req);
                print_last_modified(req);
                req_write(req, CRLF);
                req_flush(req);
            }
            if (req->method == M_HEAD)
                return 0;

            return init_cgi(req);
        }
    }
#endif

    if (data_fd == -1) {
        log_error_doc(req);
        errno = saved_errno;
        perror("document open");

        if (saved_errno == ENOENT)
            send_r_not_found(req);
        else if (saved_errno == EACCES)
            send_r_forbidden(req);
        else
            send_r_bad_request(req);
        return 0;
    }

#ifdef ACCESS_CONTROL
    if (!access_allow(req->pathname)) {
      send_r_forbidden(req);
      return 0;
    }
#endif

    fstat(data_fd, &statbuf);

    if (S_ISDIR(statbuf.st_mode)) { /* directory */
        close(data_fd);         /* close dir */

        if (req->pathname[strlen(req->pathname) - 1] != '/') {
            char buffer[3 * MAX_PATH_LENGTH + 128];
            unsigned int len;

#ifdef ALLOW_LOCAL_REDIRECT
            len = strlen(req->request_uri);
            if (len + 2 > sizeof(buffer)) {
                send_r_error(req);
                return 0;
            }
            memcpy(buffer, req->request_uri, len);
            buffer[len] = '/';
            buffer[len+1] = '\0';
#else
            char *host = server_name;
            unsigned int l2;
            char *port = NULL;
            const char *prefix = "http://";
            static unsigned int l3 = 0;
            static unsigned int l4 = 0;

            if (l4 == 0) {
                l4 = strlen(prefix);
            }
            len = strlen(req->request_uri);
            if (!port && server_port != 80) {
                port = strdup(simple_itoa(server_port));
                if (port == NULL) {
                    errno = ENOMEM;
                    boa_perror(req, "Unable to perform simple_itoa conversion on server port!");
                    return 0;
                }
                l3 = strlen(port);
            }
            /* l3 and l4 are done */

            if (req->host) {
                /* only shows up in vhost mode */
                /* what about the port? (in vhost_mode?) */
                /* we don't currently report ports that differ
                 * from out "bound" (listening) port, so we don't care
                 */
                host = req->host;
            }
            l2 = strlen(host);

            if (server_port != 80) {
                if (l4 + l2 + 1 + l3 + len + 1 > sizeof(buffer)) {
                    errno = ENOMEM;
                    boa_perror(req, "buffer not large enough for directory redirect");
                    return 0;
                }
                memcpy(buffer, prefix, l4);
                memcpy(buffer + l4, host, l2);
                buffer[l4 + l2] = ':';
                memcpy(buffer + l4 + l2 + 1, port, l3);
                memcpy(buffer + l4 + l2 + 1 + l3, req->request_uri, len);
                buffer[l4 + l2 + 1 + l3 + len] = '/';
                buffer[l4 + l2 + 1 + l3 + len + 1] = '\0';
            } else {
                if (l4 + l2 + len + 1 > sizeof(buffer)) {
                    errno = ENOMEM;
                    boa_perror(req, "buffer not large enough for directory redirect");
                    return 0;
                }
                memcpy(buffer, prefix, l4);
                memcpy(buffer + l4, host, l2);
                memcpy(buffer + l4 + l2, req->request_uri, len);
                buffer[l4 + l2 + len] = '/';
                buffer[l4 + l2 + len + 1] = '\0';
            }
#endif /* ALLOW LOCAL REDIRECT */
            send_r_moved_perm(req, buffer);
            return 0;
        }
        data_fd = get_dir(req, &statbuf); /* updates statbuf */

        if (data_fd < 0)      /* couldn't do it */
            return 0;           /* errors reported by get_dir */
        else if (data_fd == 0 || data_fd == 1)
            return data_fd;
        /* else, data_fd contains the fd of the file... */
    }

    if (!S_ISREG(statbuf.st_mode)) { /* regular file */
        log_error_doc(req);
        fprintf(stderr, "Resulting file is not a regular file.\n");
        send_r_bad_request(req);
        close(data_fd);
        return 0;
    }

    /* If-UnModified-Since asks
     *  is the file newer than date located in time_cval
     *  yes -> return 412
     *   no -> return 200
     *
     * If-Modified-Since asks
     *  is the file date less than or same as the date located in time_cval
     *  yes -> return 304
     *  no  -> return 200
     *
     * If-Unmodified-Since overrides If-Modified-Since
     */

    /*
    if (req->headers[H_IF_UNMODIFIED_SINCE] &&
        modified_since(&(statbuf.st_mtime),
                       req->headers[H_IF_UNMODIFIED_SINCE])) {
        send_r_precondition_failed(req);
        return 0;
    } else
    */
    if (req->if_modified_since &&
        !modified_since(&(statbuf.st_mtime), req->if_modified_since)) {
        send_r_not_modified(req);
        close(data_fd);
        return 0;
    }

    req->filesize = statbuf.st_size;
    req->last_modified = statbuf.st_mtime;

    /* ignore if-range without range */
    if (req->header_ifrange && !req->ranges)
        req->header_ifrange = NULL;

    /* we don't support it yet */
    req->header_ifrange = NULL;

    /* parse ranges now */
    /* we have to wait until req->filesize exists to fix them up */
    /* fixup handles handles communicating with the client */
    /* ranges_fixup logs as appropriate, and sends
     * send_r_invalid_range on error.
     */

    if (req->filesize == 0) {
        if (req->http_version < HTTP11) {
            send_r_request_ok(req);
            close(data_fd);
            return 0;
        }
        send_r_no_content(req);
        close(data_fd);
        return 0;
    }

    if (req->ranges && !ranges_fixup(req)) {
        close(data_fd);
        return 0;
    }

    /* if no range has been set, use default range */
#if 0
    DEBUG(DEBUG_RANGE) {
        log_error_time();
        fprintf(stderr, "if-range: %s\time_cval: %d\tmtime: %d\n",
                req->header_ifrange, req->time_cval, statbuf->st_mtime);
    }
#endif

    /*
     If the entity tag given in the If-Range header matches the current
     entity tag for the entity, then the server should provide the
     specified sub-range of the entity using a 206 (Partial content)
     response.

     If the entity tag does not match, then the server should
     return the entire entity using a 200 (OK) response.
     */
    /* IF we have range data *and* no if-range or if-range matches... */

#ifdef MAX_FILE_MMAP
    if (req->filesize > MAX_FILE_MMAP) {
        req->data_fd = data_fd;
        req->status = IOSHUFFLE;
    } else
#endif
    {
        /* NOTE: I (Jon Nelson) tried performing a read(2)
         * into the output buffer provided the file data would
         * fit, before mmapping, and if successful, writing that
         * and stopping there -- all to avoid the cost
         * of a mmap.  Oddly, it was *slower* in benchmarks.
         */
        req->mmap_entry_var = find_mmap(data_fd, &statbuf);
        if (req->mmap_entry_var == NULL) {
            req->data_fd = data_fd;
            req->status = IOSHUFFLE;
        } else {
            req->data_mem = req->mmap_entry_var->mmap;
            close(data_fd);             /* close data file */
        }
    }

    if (!req->ranges) {
        req->ranges = range_pool_pop();
        req->ranges->start = 0;
        req->ranges->stop = -1;
        if (!ranges_fixup(req)) {
            return 0;
        }
        send_r_request_ok(req);
    } else {
        /* FIXME: support if-range header here, by the following logic:
         * if !req->header_ifrange || st_mtime > header_ifrange,
         *   send_r_partial_content
         * else
         *   reset-ranges, etc...
         */
        if (!req->header_ifrange) {
            send_r_partial_content(req);
        } else {
            /* either no if-range or the if-range does not match */
            ranges_reset(req);
            req->ranges = range_pool_pop();
            req->ranges->start = 0;
            req->ranges->stop = -1;
            if (!ranges_fixup(req)) {
                return 0;
            }
            send_r_request_ok(req);
        }
    }

    if (req->method == M_HEAD) {
        return complete_response(req);
    }

    bytes_free = 0;
    if (req->data_mem) {
        /* things can really go tilt if req->buffer_end > BUFFER_SIZE,
         * but basically that can't happen
         */

        /* We lose statbuf here, so make sure response has been sent */
        bytes_free = BUFFER_SIZE - req->buffer_end;
        /* 256 bytes for the **trailing** headers */

        /* bytes is now how much the buffer can hold
         * after the headers
         */
    }

    if (req->data_mem && bytes_free > 256) {
        unsigned int want;
        Range *r;

        r = req->ranges;

        want = (r->stop - r->start) + 1;

        if (bytes_free > want)
            bytes_free = want;
        else {
            /* bytes_free <= want */
            ;
        }

        if (setjmp(env) == 0) {
            handle_sigbus = 1;
            memcpy(req->buffer + req->buffer_end,
                   req->data_mem + r->start, bytes_free);
            handle_sigbus = 0;
            /* OK, SIGBUS **after** this point is very bad! */
        } else {
            /* sigbus! */
            log_error_doc(req);
            reset_output_buffer(req);
            send_r_error(req);
            log_error("Got SIGBUS in memcpy\n");
            return 0;
        }
        req->buffer_end += bytes_free;
        req->bytes_written += bytes_free;
        r->start += bytes_free;
        if (bytes_free == want) {
            /* this will fit due to the 256 extra bytes_free */
            return complete_response(req);
        }
    }

    /* We lose statbuf here, so make sure response has been sent */
    return 1;
}

/*
 * Name: process_get
 * Description: Writes a chunk of data to the socket.
 *
 * Return values:
 *  -1: request blocked, move to blocked queue
 *   0: EOF or error, close it down
 *   1: successful write, recycle in ready queue
 */

int process_get(request * req)
{
    int bytes_written;
    volatile unsigned int bytes_to_write;

    if (req->method == M_HEAD) {
        return complete_response(req);
    }

    bytes_to_write = (req->ranges->stop - req->ranges->start) + 1;

    if (bytes_to_write > system_bufsize)
        bytes_to_write = system_bufsize;

    if (setjmp(env) == 0) {
        handle_sigbus = 1;
        bytes_written = write(req->fd, req->data_mem + req->ranges->start,
                              bytes_to_write);
        handle_sigbus = 0;
        /* OK, SIGBUS **after** this point is very bad! */
    } else {
        /* sigbus! */
        req->status = DEAD;
        log_error_doc(req);
        fprintf(stderr, "%sGot SIGBUS in write(2)!\n",
                get_commonlog_time());
        /* sending an error here is inappropriate
         * if we are here, the file is mmapped, and thus,
         * a content-length has been sent. If we send fewer bytes
         * the client knows there has been a problem.
         * We run the risk of accidentally sending the right number
         * of bytes (or a few too many) and the client
         * won't be the wiser.
         */
        return 0;
    }

    if (bytes_written < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
            return -1;
        /* request blocked at the pipe level, but keep going */
        else {
#ifdef QUIET_DISCONNECT
            if (errno != EPIPE) {
#else
            if (1) {
#endif
                log_error_doc(req);
                /* Can generate lots of log entries, */
                perror("write");
                /* OK to disable if your logs get too big */
            }
            req->status = DEAD;
            return 0;
        }
    }

    req->bytes_written += bytes_written;
    req->ranges->start += bytes_written;

    if ((req->ranges->stop + 1 - req->ranges->start) == 0) {
        return complete_response(req);
    }

    return 1;               /* more to do */
}

/*
 * Name: get_dir
 * Description: Called from process_get if the request is a directory.
 * statbuf must describe directory on input, since we may need its
 *   device, inode, and mtime.
 * statbuf is updated, since we may need to check mtimes of a cache.
 * returns:
 *  -1 error
 *  0  cgi (either gunzip or auto-generated)
 *  >0  file descriptor of file
 */

int get_dir(request * req, struct stat *statbuf)
{

    char pathname_with_index[MAX_PATH_LENGTH];
    int data_fd;

    if (directory_index) {      /* look for index.html first?? */
        unsigned int l1, l2;

        l1 = strlen(req->pathname);
        l2 = strlen(directory_index);
#ifdef GUNZIP
        if (l1 + l2 + 3 + 1 > sizeof(pathname_with_index)) { /* for .gz */
#else
        if (l1 + l2 + 1 > sizeof(pathname_with_index)) {
#endif

            errno = ENOMEM;
            boa_perror(req, "pathname_with_index not large enough for pathname + index");
            return -1;
        }
        memcpy(pathname_with_index, req->pathname, l1); /* doesn't copy NUL */
        memcpy(pathname_with_index + l1, directory_index, l2 + 1); /* does */

        data_fd = open(pathname_with_index, O_RDONLY);

        if (data_fd != -1) {    /* user's index file */
            /* We have to assume that directory_index will fit, because
             * if it doesn't, well, that's a huge configuration problem.
             * this is only the 'index.html' pathname for mime type
             */
            memcpy(req->request_uri, directory_index, l2 + 1); /* for mimetype */
            fstat(data_fd, statbuf);
            return data_fd;
        }
        if (errno == EACCES) {
            send_r_forbidden(req);
            return -1;
        } else if (errno != ENOENT) {
            /* if there is an error *other* than EACCES or ENOENT */
            send_r_not_found(req);
            return -1;
        }
#ifdef GUNZIP
        /* if we are here, trying index.html didn't work
         * try index.html.gz
         */
        strcat(pathname_with_index, ".gz");
        data_fd = open(pathname_with_index, O_RDONLY);
        if (data_fd != -1) {    /* user's index file */
            close(data_fd);

            req->response_status = R_REQUEST_OK;
            SQUASH_KA(req);
            if (req->pathname)
                free(req->pathname);
            req->pathname = strdup(pathname_with_index);
            if (!req->pathname) {
                boa_perror(req, "strdup of pathname_with_index for .gz files " __FILE__ ":" STR(__LINE__));
                return 0;
            }
            if (req->http_version != HTTP09) {
                req_write(req, http_ver_string(req->http_version));
                req_write(req, " 200 OK-GUNZIP" CRLF);
                print_http_headers(req);
                print_last_modified(req);
                req_write(req, "Content-Type: ");
                req_write(req, get_mime_type(directory_index));
                req_write(req, CRLF CRLF);
                req_flush(req);
            }
            if (req->method == M_HEAD)
                return 0;
            return init_cgi(req);
        }
#endif
    }

    /* only here if index.html, index.html.gz don't exist */
    if (dirmaker != NULL) {     /* don't look for index.html... maybe automake? */
        req->response_status = R_REQUEST_OK;
        SQUASH_KA(req);

        /* the indexer should take care of all headers */
        if (req->http_version != HTTP09) {
            req_write(req, http_ver_string(req->http_version));
            req_write(req, " 200 OK" CRLF);
            print_http_headers(req);
            print_last_modified(req);
            req_write(req, "Content-Type: text/html" CRLF CRLF);
            req_flush(req);
        }
        if (req->method == M_HEAD)
            return 0;

        return init_cgi(req);
        /* in this case, 0 means success */
    } else if (cachedir) {
        return get_cachedir_file(req, statbuf);
    } else {                    /* neither index.html nor autogenerate are allowed */
        send_r_forbidden(req);
        return -1;              /* nothing worked */
    }
}

static int get_cachedir_file(request * req, struct stat *statbuf)
{

    char pathname_with_index[MAX_PATH_LENGTH];
    int data_fd;
    time_t real_dir_mtime;

    real_dir_mtime = statbuf->st_mtime;
    /* the sizeof() doesn't need a -1 because snprintf will
     * include the NUL when calculating if the size is enough
     */
    snprintf(pathname_with_index, sizeof(pathname_with_index),
             "%s/dir.%d.%ld", cachedir,
             (int) statbuf->st_dev, statbuf->st_ino);
    data_fd = open(pathname_with_index, O_RDONLY);

    if (data_fd != -1) {        /* index cache */

        fstat(data_fd, statbuf);
        if (statbuf->st_mtime > real_dir_mtime) {
            statbuf->st_mtime = real_dir_mtime; /* lie */
            strcpy(req->request_uri, directory_index); /* for mimetype */
            return data_fd;
        }
        close(data_fd);
        unlink(pathname_with_index); /* cache is stale, delete it */
    }
    if (index_directory(req, pathname_with_index) == -1)
        return -1;

    data_fd = open(pathname_with_index, O_RDONLY); /* Last chance */
    if (data_fd != -1) {
        strcpy(req->request_uri, directory_index); /* for mimetype */
        fstat(data_fd, statbuf);
        statbuf->st_mtime = real_dir_mtime; /* lie */
        return data_fd;
    }

    boa_perror(req, "re-opening dircache");
    return -1;                  /* Nothing worked. */

}

/*
 * Name: index_directory
 * Description: Called from get_cachedir_file if a directory html
 * has to be generated on the fly
 * returns -1 for problem, else 0
 * This version is the fastest, ugliest, and most accurate yet.
 * It solves the "stale size or type" problem by not ever giving
 * the size or type.  This also speeds it up since no per-file
 * stat() is required.
 */

static int index_directory(request * req, char *dest_filename)
{
    DIR *request_dir;
    FILE *fdstream;
    struct dirent *dirbuf;
    int bytes = 0;
    char *escname = NULL;

    if (chdir(req->pathname) == -1) {
        if (errno == EACCES || errno == EPERM) {
            send_r_forbidden(req);
        } else {
            log_error_doc(req);
            perror("chdir");
            send_r_bad_request(req);
        }
        return -1;
    }

    request_dir = opendir(".");
    if (request_dir == NULL) {
        int errno_save = errno;
        send_r_error(req);
        log_error_doc(req);
        fprintf(stderr, "opendir failed on directory \"%s\": ", req->pathname);
        errno = errno_save;
        perror("opendir");
        return -1;
    }

    fdstream = fopen(dest_filename, "w");
    if (fdstream == NULL) {
        boa_perror(req, "dircache fopen");
        closedir(request_dir);
        return -1;
    }

    bytes += fprintf(fdstream,
                     "<HTML><HEAD>\n<TITLE>Index of %s</TITLE>\n</HEAD>\n\n",
                     req->request_uri);
    bytes += fprintf(fdstream, "<BODY>\n\n<H2>Index of %s</H2>\n\n<PRE>\n",
                     req->request_uri);

    while ((dirbuf = readdir(request_dir))) {
        if (!strcmp(dirbuf->d_name, "."))
            continue;

        if (!strcmp(dirbuf->d_name, "..")) {
            bytes += fprintf(fdstream,
                             " [DIR] <A HREF=\"../\">Parent Directory</A>\n");
            continue;
        }

        /* FIXME: ought to use (as-yet unwritten) html_escape_string */
        escname = escape_string(dirbuf->d_name, NULL);
        if (escname != NULL) {
            bytes += fprintf(fdstream, " <A HREF=\"%s\">%s</A>\n",
                             escname, dirbuf->d_name);
            free(escname);
            escname = NULL;
        }
    }
    closedir(request_dir);
    bytes += fprintf(fdstream, "</PRE>\n\n</BODY>\n</HTML>\n");

    fclose(fdstream);

    chdir(server_root);

    req->filesize = bytes;      /* for logging transfer size */
    return 0;                   /* success */
}
