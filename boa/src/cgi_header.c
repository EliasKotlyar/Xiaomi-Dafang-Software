/*
 *  Boa, an http server
 *  cgi_header.c - cgi header parsing and control
 *  Copyright (C) 1997-2003 Jon Nelson <jnelson@boa.org>
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

#include "boa.h"

/* process_cgi_header

* returns 0 -=> error or HEAD, close down.
* returns 1 -=> done processing
* leaves req->cgi_status as WRITE
*/

/*
 The server MUST also resolve any conflicts between header fields returned by
 the script and header fields that it would otherwise send itself.

 ...

 At least one CGI-Field MUST be supplied, but no CGI field name may be used
 more than once in a response. If a body is supplied, then a
 "Content-type" header field MUST be supplied by the script,
 otherwise the script MUST send a "Location" or "Status" header
 field. If a Location CGI-Field is returned, then the script
 MUST NOT supply any HTTP-Fields.
 */

/* TODO:
 We still need to cycle through the data before the end of the headers,
 line-by-line, and check for any problems with the CGI
 outputting overriding http responses, etc...
 */

int process_cgi_header(request * req)
{
    char *buf;
    char *c;

    if (req->cgi_status != CGI_DONE)
        req->cgi_status = CGI_BUFFER;

    buf = req->header_line;

    c = strstr(buf, "\n\r\n");
    if (c == NULL) {
        c = strstr(buf, "\n\n");
        if (c == NULL) {
            log_error_doc(req);
            fputs("cgi_header: unable to find LFLF\n", stderr);
#ifdef FASCIST_LOGGING
            log_error_time();
            fprintf(stderr, "\"%s\"\n", buf);
#endif
            send_r_bad_gateway(req);
            return 0;
        }
    }
    if (req->http_version == HTTP09) {
        if (*(c + 1) == '\r')
            req->header_line = c + 2;
        else
            req->header_line = c + 1;
        return 1;
    }
    if (!strncasecmp(buf, "Status: ", 8)) {
        req->header_line--;
        memcpy(req->header_line, "HTTP/1.0 ", 9);
    } else if (!strncasecmp(buf, "Location: ", 10)) { /* got a location header */
#ifdef FASCIST_LOGGING

        log_error_time();
        fprintf(stderr, "%s:%d - found Location header \"%s\"\n",
                __FILE__, __LINE__, buf + 10);
#endif


        if (buf[10] == '/') {   /* virtual path */
            log_error_doc(req);
            fprintf(stderr,
                    "server does not support internal redirection: "
                    "\"%s\"\n", buf + 10);
            send_r_bad_request(req);

            /*
             * We (I, Jon) have declined to support absolute-path parsing
             * because I see it as a major security hole.
             * Location: /etc/passwd or Location: /etc/shadow is not funny.
             *
             * Also, the below code is borked.
             * request_uri could contain /cgi-bin/bob/extra_path
             */

            /*
               strcpy(req->request_uri, buf + 10);
               return internal_redirect(req);
             */
        } else {                /* URL */
            char *c2;
            c2 = strchr(buf + 10, '\n');
            /* c2 cannot ever equal NULL here because we already have found one */

            --c2;
            while (*c2 == '\r')
                --c2;
            ++c2;
            /* c2 now points to a '\r' or the '\n' */
            *c2++ = '\0';       /* end header */

            /* first next header, or is at req->header_end */
            while ((*c2 == '\n' || *c2 == '\r') && c2 < req->header_end)
                ++c2;
            if (c2 == req->header_end)
                send_r_moved_temp(req, buf + 10, "");
            else
                send_r_moved_temp(req, buf + 10, c2);
        }
        req->status = DONE;
        return 1;
    } else {                    /* not location and not status */
        char *dest;
        unsigned int howmuch;
        send_r_request_ok(req); /* does not terminate */
        /* got to do special things because
           a) we have a single buffer divided into 2 pieces
           b) we need to merge those pieces
           Easiest way is to memmove the cgi data backward until
           it touches the buffered data, then reset the cgi data pointers
         */
        dest = req->buffer + req->buffer_end;
        if (req->method == M_HEAD) {
            if (*(c + 1) == '\r')
                req->header_end = c + 2;
            else
                req->header_end = c + 1;
            req->cgi_status = CGI_DONE;
        }
        howmuch = req->header_end - req->header_line;

        if (dest + howmuch > req->buffer + BUFFER_SIZE) {
            /* big problem */
            log_error_doc(req);
            fprintf(stderr, "Too much data to move! Aborting! %s %d\n",
                    __FILE__, __LINE__);
            /* reset buffer pointers because we already called
               send_r_request_ok... */
            req->buffer_start = req->buffer_end = 0;
            send_r_error(req);
            return 0;
        }
        memmove(dest, req->header_line, howmuch);
        req->buffer_end += howmuch;
        req->header_line = req->buffer + req->buffer_end;
        req->header_end = req->header_line;
        req_flush(req);
        if (req->method == M_HEAD)
            return 0;
    }
    return 1;
}
