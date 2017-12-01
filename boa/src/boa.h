/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <paulp@go2net.com>
 *  Copyright (C) 1996-1999 Larry Doolittle <ldoolitt@boa.org>
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

/* $Id: boa.h,v 1.63.2.27 2005/02/22 14:11:29 jnelson Exp $*/

#ifndef _BOA_H
#define _BOA_H

#include "config.h"
#include <errno.h>
#include <stdlib.h>             /* malloc, free, etc. */
#include <stdio.h>              /* stdin, stdout, stderr */
#include <string.h>             /* strdup */
#include <ctype.h>
#include <time.h>               /* localtime, time */
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>             /* OPEN_MAX */
#include <setjmp.h>

#include <netinet/in.h>

#include <sys/mman.h>
#include <sys/types.h>          /* socket, bind, accept */
#include <sys/socket.h>         /* socket, bind, accept, setsockopt, */
#include <sys/stat.h>           /* open */

#include "compat.h"             /* oh what fun is porting */
#include "defines.h"
#include "globals.h"

/* alias */
void add_alias(const char *fakename, const char *realname, enum ALIAS type);
int translate_uri(request * req);
void dump_alias(void);

/* config */
void read_config_files(void);

/* escape */
#include "escape.h"

/* get */
int init_get(request * req);
int process_get(request * req);
int get_dir(request * req, struct stat *statbuf);

/* hash */
unsigned get_mime_hash_value(const char *extension);
char *get_mime_type(const char *filename);
char *get_home_dir(const char *name);
void dump_mime(void);
void dump_passwd(void);
void hash_show_stats(void);
void add_mime_type(const char *extension, const char *type);

/* log */
void open_logs(void);
void log_access(request * req);
void log_error_doc(request * req);
void boa_perror(request * req, const char *message);
void log_error_time(void);
void log_error(const char *mesg);
#ifdef HAVE_FUNC
void log_error_mesg(const char *file, int line, const char *func, const char *mesg);
void log_error_mesg_fatal(const char *file, int line, const char *func, const char *mesg);
#else
void log_error_mesg(const char *file, int line, const char *mesg);
void log_error_mesg_fatal(const char *file, int line, const char *mesg);
#endif

/* queue */
void block_request(request * req);
void ready_request(request * req);
void dequeue(request ** head, request * req);
void enqueue(request ** head, request * req);

/* read */
int read_header(request * req);
int read_body(request * req);
int write_body(request * req);

/* request */
request *new_request(void);
void get_request(int);
void process_requests(int server_s);
int process_header_end(request * req);
int process_header_line(request * req);
int process_logline(request * req);
int process_option_line(request * req);
void add_accept_header(request * req, const char *mime_type);
void free_requests(void);

/* response */
const char *http_ver_string(enum HTTP_VERSION ver);
void print_ka_phrase(request * req);
void print_content_type(request * req);
void print_content_length(request * req);
void print_last_modified(request * req);
void print_http_headers(request * req);
void print_content_range(request * req);
void print_partial_content_continue(request * req);
void print_partial_content_done(request * req);
int complete_response(request *req);

void send_r_continue(request * req); /* 100 */
void send_r_request_ok(request * req); /* 200 */
void send_r_no_content(request * req); /* 204 */
void send_r_partial_content(request * req); /* 206 */
void send_r_moved_perm(request * req, const char *url); /* 301 */
void send_r_moved_temp(request * req, const char *url, const char *more_hdr); /* 302 */
void send_r_not_modified(request * req); /* 304 */
void send_r_bad_request(request * req); /* 400 */
void send_r_unauthorized(request * req, const char *name); /* 401 */
void send_r_forbidden(request * req); /* 403 */
void send_r_not_found(request * req); /* 404 */
void send_r_length_required(request * req); /* 411 */
void send_r_precondition_failed(request * req); /* 412 */
void send_r_request_uri_too_long(request * req); /* 414 */
void send_r_invalid_range(request * req); /* 416 */
void send_r_error(request * req); /* 500 */
void send_r_not_implemented(request * req); /* 501 */
void send_r_bad_gateway(request * req); /* 502 */
void send_r_service_unavailable(request * req); /* 503 */
void send_r_bad_version(request * req, const char * version); /* 505 */

/* cgi */
void create_common_env(void);
void add_to_common_env(char *key, char *value);
void clear_common_env(void);
int add_cgi_env(request * req, const char *key, const char *value, int http_prefix);
int init_cgi(request * req);

/* signals */
void init_signals(void);
void reset_signals(void);
void sighup_run(void);
void sigchld_run(void);
void sigalrm_run(void);
void sigterm_stage1_run(void);
void sigterm_stage2_run(void);

/* util.c */
void clean_pathname(char *pathname);
char *get_commonlog_time(void);
void rfc822_time_buf(char *buf, time_t s);
char *simple_itoa(unsigned int i);
int boa_atoi(const char *s);
int month2int(const char *month);
int modified_since(time_t * mtime, const char *if_modified_since);
int unescape_uri(char *uri, char **query_string);
int create_temporary_file(short want_unlink, char *storage, unsigned int size);
int real_set_block_fd(int fd);
int real_set_nonblock_fd(int fd);
char *to_upper(char *str);
void strlower(char *s);
int check_host(const char *r);
#ifndef DISABLE_DEBUG
void parse_debug(char *foo);
void print_debug_usage(void);
#endif

/* buffer */
int req_write(request * req, const char *msg);
void reset_output_buffer(request * req);
int req_write_escape_http(request * req, const char *msg);
int req_write_escape_html(request * req, const char *msg);
int req_flush(request * req);
char *escape_uri(const char *uri);
char *escape_string(const char *inp, char *buf);

/* timestamp */
void timestamp(void);

/* mmap_cache */
struct mmap_entry *find_mmap(int data_fd, struct stat *s);
void release_mmap(struct mmap_entry *e);

/* sublog */
int open_gen_fd(char *spec);
int process_cgi_header(request * req);

/* pipe */
int read_from_pipe(request * req);
int write_from_pipe(request * req);
int io_shuffle(request * req);
#ifdef HAVE_SENDFILE
#include <sys/sendfile.h>
int io_shuffle_sendfile(request * req);
#endif

/* ip */
int bind_server(int sock, char *ip, unsigned int port);
char *ascii_sockaddr(struct SOCKADDR *s, char *dest, unsigned int len);
int net_port(struct SOCKADDR *s);

/* select or poll */
void loop(int server_s);

/* range.c */
void ranges_reset(request * req);
Range *range_pool_pop(void);
void range_pool_empty(void);
void range_pool_push(Range * r);
int ranges_fixup(request * req);
int range_parse(request * req, const char *str);
 
#endif
