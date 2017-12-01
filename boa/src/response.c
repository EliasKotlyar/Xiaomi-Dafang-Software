/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <paulp@go2net.com>
 *  Copyright (C) 1996-1999 Larry Doolittle <ldoolitt@boa.org>
 *  Copyright (C) 1996-2004 Jon Nelson <jnelson@boa.org>
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

/* $Id: response.c,v 1.41.2.17 2005/02/22 14:11:29 jnelson Exp $*/

#include "boa.h"

#define HTML "text/html; charset=ISO-8859-1"

const char *http_ver_string(enum HTTP_VERSION ver)
{
    switch(ver) {
    case HTTP09:
        return "HTTP/0.9";
        break;
    case HTTP10:
        return "HTTP/1.0";
        break;
    case HTTP11:
        return "HTTP/1.1";
        break;
    default:
        return "HTTP/1.0";
    }
    return NULL;
}

void print_content_type(request * req)
{
    char * mime_type = get_mime_type(req->request_uri);

    if (mime_type != NULL) {
        req_write(req, "Content-Type: ");
        req_write(req, mime_type);
        if (default_charset != NULL &&
            strncasecmp( mime_type, "text", 4)==0) {

            /* add default charset */
            req_write( req, "; charset=");
            req_write( req, default_charset);
        }
        req_write(req, CRLF);
    }
}

void print_content_length(request * req)
{
    req_write(req, "Content-Length: ");
    req_write(req, simple_itoa(req->filesize));
    req_write(req, CRLF);
}

void print_last_modified(request * req)
{
    static char lm[] = "Last-Modified: "
        "                             " CRLF;
    rfc822_time_buf(lm + 15, req->last_modified);
    req_write(req, lm);
}

void print_ka_phrase(request * req)
{
    if (req->kacount > 0 &&
        req->keepalive == KA_ACTIVE && req->response_status < 500) {
        /* FIXME: Should we only print one or the other if we are HTTP
         * version between 1.0 (incl.) and 1.1 (not incl.) ?
         */
        req_write(req, "Connection: Keep-Alive" CRLF "Keep-Alive: timeout=");
        req_write(req, simple_itoa(ka_timeout));
        req_write(req, ", max=");
        req_write(req, simple_itoa(req->kacount));
        req_write(req, CRLF);
    } else
        req_write(req, "Connection: close" CRLF);
}

void print_http_headers(request * req)
{
    static char date_header[] = "Date: "
        "                             " CRLF;
    static char server_header[] = "Server: " SERVER_VERSION CRLF;

    rfc822_time_buf(date_header + 6, 0);
    req_write(req, date_header);
    if (!conceal_server_identity)
        req_write(req, server_header);
    req_write(req, "Accept-Ranges: bytes" CRLF);
    print_ka_phrase(req);
}

void print_content_range(request * req)
{
    req_write(req, "Content-Range: bytes ");
    req_write(req, simple_itoa(req->ranges->start));
    req_write(req, "-");
    req_write(req, simple_itoa(req->ranges->stop));
    req_write(req, "/");
    req_write(req, simple_itoa(req->filesize));
    req_write(req, CRLF);
}

void print_partial_content_continue(request * req)
{
    static char msg[] = CRLF "--THIS_STRING_SEPARATES" CRLF;

    if (req->numranges > 1) {
        req_write(req, msg);
    }
    print_content_type(req);
    print_content_range(req);
}

void print_partial_content_done(request * req)
{
    static char msg[] = CRLF "--THIS_STRING_SEPARATES--" CRLF;

    req_write(req, msg);
}

/* The routines above are only called by the routines below.
 * The rest of Boa only enters through the routines below.
 */

int complete_response(request *req)
{
    Range *r;

    /* we only want to push off the one range */
    r = req->ranges;
    req->ranges = req->ranges->next;
    range_pool_push(r);

    /* successfully flushed */
    if (req->response_status == R_PARTIAL_CONTENT && req->numranges > 1) {
        if (req->ranges != NULL) {
            print_partial_content_continue(req);
            req_write(req, CRLF);
        } else {
            print_partial_content_done(req);
            req->status = DONE;
            req_flush(req);
        }
    } else {
        req->status = DONE;
        return 0;
    }
    return 1;
}

/* R_CONTINUE: 100 */
void send_r_continue(request * req)
{
    static char msg[] = " 100 Continue" CRLF CRLF;

    /* shouldn't need this */
    if (req->http_version != HTTP11)
        return;

    req->response_status = R_CONTINUE;

    req_write(req, http_ver_string(req->http_version));
    req_write(req, msg);
}

/* R_REQUEST_OK: 200 */
void send_r_request_ok(request * req)
{
    req->response_status = R_REQUEST_OK;
    if (req->http_version == HTTP09)
        return;

    req_write(req, http_ver_string(req->http_version));
    req_write(req, " 200 OK" CRLF);
    print_http_headers(req);

    if (!req->cgi_type) {
        print_content_length(req);
        print_last_modified(req);
        print_content_type(req);
        req_write(req, CRLF);
    }
}

/* R_NO_CONTENT: 204 */
void send_r_no_content(request * req)
{
    static char msg[] = " 204 NO CONTENT" CRLF;

    req->response_status = R_NO_CONTENT;
    if (req->http_version == HTTP09)
        return;

    req_write(req, http_ver_string(req->http_version));
    req_write(req, msg);
    print_http_headers(req);

    /* FIXME: Why is this here? */
    if (!req->cgi_type) {
        req_write(req, CRLF);
    }
}

/* R_PARTIAL_CONTENT: 206 */
void send_r_partial_content(request * req)
{
    static char msg[] = " 206 Partial Content" CRLF;
    static char msg2[] = "Content-Type: multipart/byteranges; "
        "boundary=THIS_STRING_SEPARATES" CRLF;

    req->response_status = R_PARTIAL_CONTENT;
#if 0
    if (req->http_version != HTTP11) {
        log_error("can't do partial content if not HTTP/1.1!!");
        send_r_request_ok(req);
        return;
    }
#endif

    req_write(req, http_ver_string(req->http_version));
    req_write(req, msg);
    print_http_headers(req);
    print_last_modified(req);
    if (req->numranges > 1) {
        req_write(req, msg2);
        req_write(req, CRLF);
    } else {
        req_write(req, "Content-Length: ");
        req_write(req, simple_itoa(req->ranges->stop - req->ranges->start + 1));
        req_write(req, CRLF);
    }
    print_partial_content_continue(req);
    req_write(req, CRLF);
}


/* R_MOVED_PERM: 301 */
void send_r_moved_perm(request * req, const char *url)
{
    SQUASH_KA(req);
    req->response_status = R_MOVED_PERM;
    if (req->http_version != HTTP09) {
        req_write(req, http_ver_string(req->http_version));
        req_write(req, " 301 Moved Permanently" CRLF);
        print_http_headers(req);
        req_write(req, "Content-Type: " HTML CRLF);

        req_write(req, "Location: ");
        req_write_escape_http(req, url);
        req_write(req, CRLF CRLF);
    }
    if (req->method != M_HEAD) {
        req_write(req,
                  "<HTML><HEAD><TITLE>301 Moved Permanently</TITLE></HEAD>\n"
                  "<BODY>\n<H1>301 Moved</H1>The document has moved\n"
                  "<A HREF=\"");
        req_write_escape_html(req, url);
        req_write(req, "\">here</A>.\n</BODY></HTML>\n");
    }
    req_flush(req);
}

/* R_MOVED_TEMP: 302 */
void send_r_moved_temp(request * req, const char *url, const char *more_hdr)
{
    SQUASH_KA(req);
    req->response_status = R_MOVED_TEMP;
    if (req->http_version != HTTP09) {
        req_write(req, http_ver_string(req->http_version));
        req_write(req, " 302 Moved Temporarily" CRLF);
        print_http_headers(req);
        req_write(req, "Content-Type: " HTML CRLF);

        req_write(req, "Location: ");
        req_write_escape_http(req, url);
        req_write(req, CRLF);
        req_write(req, more_hdr);
        req_write(req, CRLF CRLF);
    }
    if (req->method != M_HEAD) {
        req_write(req,
                  "<HTML><HEAD><TITLE>302 Moved Temporarily</TITLE></HEAD>\n"
                  "<BODY>\n<H1>302 Moved</H1>The document has moved\n"
                  "<A HREF=\"");
        req_write_escape_html(req, url);
        req_write(req, "\">here</A>.\n</BODY></HTML>\n");
    }
    req_flush(req);
}

/* R_NOT_MODIFIED: 304 */
void send_r_not_modified(request * req)
{
    SQUASH_KA(req);
    req->response_status = R_NOT_MODIFIED;
    req_write(req, http_ver_string(req->http_version));
    req_write(req, " 304 Not Modified" CRLF);
    print_http_headers(req);
    print_content_type(req);
    req_write(req, CRLF);
    req_flush(req);
}

/* R_BAD_REQUEST: 400 */
void send_r_bad_request(request * req)
{
    SQUASH_KA(req);
    req->response_status = R_BAD_REQUEST;
    if (req->http_version != HTTP09) {
        req_write(req, http_ver_string(req->http_version));
        req_write(req, " 400 Bad Request" CRLF);
        print_http_headers(req);
        req_write(req, "Content-Type: " HTML CRLF CRLF); /* terminate header */
    }
    if (req->method != M_HEAD)
        req_write(req,
                  "<HTML><HEAD><TITLE>400 Bad Request</TITLE></HEAD>\n"
                  "<BODY><H1>400 Bad Request</H1>\nYour client has issued "
                  "a malformed or illegal request.\n</BODY></HTML>\n");
    req_flush(req);
}

/* R_UNAUTHORIZED: 401 */
void send_r_unauthorized(request * req, const char *realm_name)
{
    SQUASH_KA(req);
    req->response_status = R_UNAUTHORIZED;
    if (req->http_version != HTTP09) {
        req_write(req, http_ver_string(req->http_version));
        req_write(req, " 401 Unauthorized" CRLF);
        print_http_headers(req);
        req_write(req, "WWW-Authenticate: Basic realm=\"");
        req_write(req, realm_name);
        req_write(req, CRLF);
        req_write(req, "Content-Type: " HTML CRLF CRLF); /* terminate header */
    }
    if (req->method != M_HEAD) {
        req_write(req,
                  "<HTML><HEAD><TITLE>401 Unauthorized</TITLE></HEAD>\n"
                  "<BODY><H1>401 Unauthorized</H1>\nYour client does not "
                  "have permission to get URL ");
        req_write_escape_html(req, req->request_uri);
        req_write(req, " from this server.\n</BODY></HTML>\n");
    }
    req_flush(req);
}

/* R_FORBIDDEN: 403 */
void send_r_forbidden(request * req)
{
    SQUASH_KA(req);
    req->response_status = R_FORBIDDEN;
    if (req->http_version != HTTP09) {
        req_write(req, http_ver_string(req->http_version));
        req_write(req, " 403 Forbidden" CRLF);
        print_http_headers(req);
        req_write(req, "Content-Type: " HTML CRLF CRLF); /* terminate header */
    }
    if (req->method != M_HEAD) {
        req_write(req, "<HTML><HEAD><TITLE>403 Forbidden</TITLE></HEAD>\n"
                  "<BODY><H1>403 Forbidden</H1>\nYour client does not "
                  "have permission to get URL ");
        req_write_escape_html(req, req->request_uri);
        req_write(req, " from this server.\n</BODY></HTML>\n");
    }
    req_flush(req);
}

/* R_NOT_FOUND: 404 */
void send_r_not_found(request * req)
{
    SQUASH_KA(req);
    req->response_status = R_NOT_FOUND;
    if (req->http_version != HTTP09) {
        req_write(req, http_ver_string(req->http_version));
        req_write(req, " 404 Not Found" CRLF);
        print_http_headers(req);
        req_write(req, "Content-Type: " HTML CRLF CRLF); /* terminate header */
    }
    if (req->method != M_HEAD) {
        req_write(req, "<HTML><HEAD><TITLE>404 Not Found</TITLE></HEAD>\n"
                  "<BODY><H1>404 Not Found</H1>\nThe requested URL ");
        req_write_escape_html(req, req->request_uri);
        req_write(req, " was not found on this server.\n</BODY></HTML>\n");
    }
    req_flush(req);
}

/* R_LENGTH_REQUIRED: 411 */
/* FIXME: incomplete */
void send_r_length_required(request * req)
{
    SQUASH_KA(req);
    req->response_status = R_LENGTH_REQUIRED;
    if (req->http_version != HTTP09) {
        req_write(req, http_ver_string(req->http_version));
        req_write(req, " 411 Length Required" CRLF);
        print_http_headers(req);
        req_write(req, "Content-Type: " HTML CRLF);
        print_last_modified(req);
        print_content_type(req);
        req_write(req, CRLF);
    }
    if (req->method != M_HEAD) {
        req_write(req,
                  "<HTML><HEAD><TITLE>411 Length Required</TITLE></HEAD>\n"
                  "<BODY><H1>411 Length Required</H1>\nThe requested URL ");
        req_write_escape_html(req, req->request_uri);
        req_write(req,
                  " requires that a valid Content-Length header be "
                  "sent with it.\n</BODY></HTML>\n");
    }
    req_flush(req);
}

/* R_PRECONDITION_FAILED: 412 */
void send_r_precondition_failed(request * req)
{
    SQUASH_KA(req);
    req->response_status = R_PRECONDITION_FAILED;
    if (req->http_version != HTTP09) {
        req_write(req, http_ver_string(req->http_version));
        req_write(req, " 412 Precondition Failed" CRLF);
        print_http_headers(req);
        req_write(req, "Content-Type: " HTML CRLF CRLF); /* terminate header */
    }
    if (req->method != M_HEAD) {
        req_write(req,
                  "<HTML><HEAD><TITLE>412 Precondition Failed</TITLE></HEAD>\n"
                  "<BODY><H1>412 Precondition Failed</H1>\n</BODY></HTML>\n");
    }
    req_flush(req);
}

/* R_BAD_REQUEST_URI_TOO_LONG: 414 */
void send_r_request_uri_too_long(request * req)
{
    char body[] =
        "<HTML><HEAD><TITLE>414 Request URI Too Long</TITLE></HEAD>\n"
        "<BODY><H1>414 Request URI Too Long</H1>\nYour client has issued "
        "a malformed or illegal request.\n</BODY></HTML>\n";
    static unsigned int len = 0;

    req->response_status = R_REQUEST_URI_TOO_LONG;
    if (req->http_version != HTTP09) {
        req_write(req, http_ver_string(req->http_version));
        req_write(req, " 414 Request URI Too Long" CRLF);
        print_http_headers(req);
        req_write(req, "Content-Type: " HTML CRLF);
        req_write(req, "Content-Length: ");
        if (!len)
            len = strlen(body);
        req_write(req, simple_itoa(len));
        req_write(req, CRLF CRLF);
    }
    if (req->method != M_HEAD)
        req_write(req, body);
    req_flush(req);
}

/* R_INVALID_RANGE: 416 */
void send_r_invalid_range(request * req)
{
    static char body[] =
        "<HTML><HEAD><TITLE>416 Invalid Range</TITLE></HEAD>\n"
        "<BODY><H1>416 Invalid Range</H1>\n</BODY></HTML>\n";
    static unsigned int body_len = 0;

    SQUASH_KA(req);
    req->response_status = R_INVALID_RANGE;
    if (req->http_version != HTTP09) {
        req_write(req, http_ver_string(req->http_version));
        req_write(req, " 416 Invalid Range" CRLF);
        print_http_headers(req);
        if (!body_len)
            body_len = strlen(body);
        req_write(req, "Content-Length: ");
        req_write(req, simple_itoa(body_len));
        req_write(req, CRLF "Content-Type: " HTML CRLF CRLF); /* terminate header */
    }
    if (req->method != M_HEAD) {
        req_write(req, body);
    }
    req_flush(req);
}

/* R_ERROR: 500 */
void send_r_error(request * req)
{
    SQUASH_KA(req);
    req->response_status = R_ERROR;
    if (req->http_version != HTTP09) {
        req_write(req, http_ver_string(req->http_version));
        req_write(req, " 500 Server Error" CRLF);
        print_http_headers(req);
        req_write(req, "Content-Type: " HTML CRLF CRLF); /* terminate header */
    }
    if (req->method != M_HEAD) {
        req_write(req,
                  "<HTML><HEAD><TITLE>500 Server Error</TITLE></HEAD>\n"
                  "<BODY><H1>500 Server Error</H1>\nThe server encountered "
                  "an internal error and could not complete your request.\n"
                  "</BODY></HTML>\n");
    }
    req_flush(req);
}

/* R_NOT_IMP: 501 */
void send_r_not_implemented(request * req)
{
    SQUASH_KA(req);
    req->response_status = R_NOT_IMP;
    if (req->http_version != HTTP09) {
        req_write(req, http_ver_string(req->http_version));
        req_write(req, " 501 Not Implemented" CRLF);
        print_http_headers(req);
        req_write(req, "Content-Type: " HTML CRLF CRLF); /* terminate header */
    }
    /* we always write the body, because we don't *KNOW*
     * what the method is.
     */
    req_write(req,
              "<HTML><HEAD><TITLE>501 Not Implemented</TITLE></HEAD>\n"
              "<BODY><H1>501 Not Implemented</H1>\nPOST to non-script "
              "is not supported in Boa.\n</BODY></HTML>\n");
    req_flush(req);
}

/* R_BAD_GATEWAY: 502 */
void send_r_bad_gateway(request * req)
{
    SQUASH_KA(req);
    req->response_status = R_BAD_GATEWAY;
    if (req->http_version != HTTP09) {
        req_write(req, http_ver_string(req->http_version));
        req_write(req, " 502 Bad Gateway" CRLF);
        print_http_headers(req);
        req_write(req, "Content-Type: " HTML CRLF CRLF); /* terminate header */
    }
    if (req->method != M_HEAD) {
        req_write(req,
                  "<HTML><HEAD><TITLE>502 Bad Gateway</TITLE></HEAD>\n"
                  "<BODY><H1>502 Bad Gateway</H1>\nThe CGI was "
                  "not CGI/1.1 compliant.\n" "</BODY></HTML>\n");
    }
    req_flush(req);
}

/* R_SERVICE_UNAVAILABLE: 503 */
void send_r_service_unavailable(request * req)
{                               /* 503 */
    static char body[] =
        "<HTML><HEAD><TITLE>503 Service Unavailable</TITLE></HEAD>\n"
        "<BODY><H1>503 Service Unavailable</H1>\n"
        "There are too many connections in use right now.\n"
	"Please try again later.\n"
	"</BODY></HTML>\n";
    static unsigned int _body_len;
    static char *body_len;

    if (!_body_len)
        _body_len = strlen(body);
    if (!body_len)
        body_len = strdup(simple_itoa(_body_len));
    if (!body_len) {
        log_error_time();
        perror("strdup of _body_len from simple_itoa");
    }


    SQUASH_KA(req);
    req->response_status = R_SERVICE_UNAV;
    if (req->http_version != HTTP09) {
        req_write(req, http_ver_string(req->http_version));
        req_write(req, " 503 Service Unavailable" CRLF);
        print_http_headers(req);
        if (body_len) {
            req_write(req, "Content-Length: ");
            req_write(req, body_len);
            req_write(req, CRLF);
        }
        req_write(req, "Content-Type: " HTML CRLF CRLF); /* terminate header */
    }
    if (req->method != M_HEAD) {
        req_write(req, body);
    }
    req_flush(req);
}


/* R_NOT_IMP: 505 */
void send_r_bad_version(request * req, const char *version)
{
    SQUASH_KA(req);
    req->response_status = R_BAD_VERSION;
    if (req->http_version != HTTP09) {
        req_write(req, http_ver_string(req->http_version));
        req_write(req, " 505 HTTP Version Not Supported" CRLF);
        print_http_headers(req);
        req_write(req, "Content-Type: " HTML CRLF CRLF); /* terminate header */
    }
    if (req->method != M_HEAD) {
        req_write(req,
                  "<HTML><HEAD><TITLE>505 HTTP Version Not Supported</TITLE></HEAD>\n"
                  "<BODY><H1>505 HTTP Version Not Supported</H1>\nHTTP versions "
                  "other than 0.9 and 1.0 "
                  "are not supported in Boa.\n<p><p>Version encountered: ");
        req_write(req, version);
        req_write(req, "<p><p></BODY></HTML>\n");
    }
    req_flush(req);
}
