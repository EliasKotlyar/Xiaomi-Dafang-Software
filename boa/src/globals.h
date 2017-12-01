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

/* $Id: globals.h,v 1.65.2.29 2005/02/22 14:11:29 jnelson Exp $*/

#ifndef _GLOBALS_H
#define _GLOBALS_H

/********************** METHODS **********************/
enum HTTP_METHOD { M_GET = 1, M_HEAD, M_PUT, M_POST,
    M_DELETE, M_LINK, M_UNLINK, M_MOVE, M_TRACE
};

/******************* HTTP VERSIONS *******************/
enum HTTP_VERSION { HTTP09=1, HTTP10, HTTP11 };

/************** REQUEST STATUS (req->status) ***************/
enum REQ_STATUS { READ_HEADER, ONE_CR, ONE_LF, TWO_CR,
    BODY_READ, BODY_WRITE,
    WRITE,
    PIPE_READ, PIPE_WRITE,
    IOSHUFFLE,
    DONE,
    TIMED_OUT,
    DEAD
};

/******************* RESPONSE CODES ******************/
enum RESPONSE_CODE { R_CONTINUE = 100,
                     R_REQUEST_OK = 200,
                     R_CREATED,
                     R_ACCEPTED,
                     R_PROVISIONAL,
                     R_NO_CONTENT,
                     R_205,
                     R_PARTIAL_CONTENT,
                     R_MULTIPLE = 300,
                     R_MOVED_PERM,
                     R_MOVED_TEMP,
                     R_303,
                     R_NOT_MODIFIED,
                     R_BAD_REQUEST = 400,
                     R_UNAUTHORIZED,
                     R_PAYMENT,
                     R_FORBIDDEN,
                     R_NOT_FOUND,
                     R_METHOD_NA, /* method not allowed */
                     R_NON_ACC,   /* non acceptable */
                     R_PROXY,     /* proxy auth required */
                     R_REQUEST_TO, /* request timeout */
                     R_CONFLICT,
                     R_GONE,
                     R_LENGTH_REQUIRED,
                     R_PRECONDITION_FAILED,
                     R_REQUEST_URI_TOO_LONG = 414,
                     R_INVALID_RANGE = 416,
                     R_ERROR = 500,
                     R_NOT_IMP,
                     R_BAD_GATEWAY,
                     R_SERVICE_UNAV,
                     R_GATEWAY_TO, /* gateway timeout */
                     R_BAD_VERSION };

/************* ALIAS TYPES (aliasp->type) ***************/
enum ALIAS { ALIAS, SCRIPTALIAS, REDIRECT };

/*********** KEEPALIVE CONSTANTS (req->keepalive) *******/
enum KA_STATUS { KA_INACTIVE, KA_ACTIVE, KA_STOPPED };

/********* CGI STATUS CONSTANTS (req->cgi_status) *******/
enum CGI_STATUS { CGI_PARSE, CGI_BUFFER, CGI_DONE };

/************** CGI TYPE (req->is_cgi) ******************/
enum CGI_TYPE { NPH = 1, CGI };

/**************** STRUCTURES ****************************/
struct range {
    unsigned long start;
    unsigned long stop;
    struct range *next;
};

typedef struct range Range;

struct mmap_entry {
    dev_t dev;
    ino_t ino;
    char *mmap;
    int use_count;
    off_t len;
};

struct request {                /* pending requests */
    enum REQ_STATUS status;
    enum KA_STATUS keepalive;   /* keepalive status */
    enum HTTP_VERSION http_version;
    enum HTTP_METHOD method;    /* M_GET, M_POST, etc. */
    enum RESPONSE_CODE response_status; /* R_NOT_FOUND, etc.. */

    enum CGI_TYPE cgi_type;
    enum CGI_STATUS cgi_status;

    /* should pollfd_id be zeroable or no ? */
#ifdef HAVE_POLL
    int pollfd_id;
#endif

    char *pathname;             /* pathname of requested file */

    Range *ranges;              /* our Ranges */
    int numranges;

    int data_fd;                /* fd of data */
    unsigned long filesize;     /* filesize */
    unsigned long filepos;      /* position in file */
    unsigned long bytes_written; /* total bytes written (sans header) */
    char *data_mem;             /* mmapped/malloced char array */

    char *logline;              /* line to log file */

    char *header_line;          /* beginning of un or incompletely processed header line */
    char *header_end;           /* last known end of header, or end of processed data */
    int parse_pos;              /* how much have we parsed */

    int buffer_start;           /* where the buffer starts */
    int buffer_end;             /* where the buffer ends */

    char *if_modified_since;    /* If-Modified-Since */
    time_t last_modified;       /* Last-modified: */

    /* CGI vars */
    int cgi_env_index;          /* index into array */

    /* Agent and referer for logfiles */
    char *header_host;
    char *header_user_agent;
    char *header_referer;
    char *header_ifrange;
    char *host;                 /* what we end up using for 'host', no matter the contents of header_host */

    int post_data_fd;           /* fd for post data tmpfile */

    char *path_info;            /* env variable */
    char *path_translated;      /* env variable */
    char *script_name;          /* env variable */
    char *query_string;         /* env variable */
    char *content_type;         /* env variable */
    char *content_length;       /* env variable */

    struct mmap_entry *mmap_entry_var;

    /* everything **above** this line is zeroed in sanitize_request */
    /* this may include 'fd' */
    /* in sanitize_request with the 'new' parameter set to 1,
     * kacount is set to ka_max and client_stream_pos is also zeroed.
     * Also, time_last is set to 'NOW'
     */
    int fd;                     /* client's socket fd */
    time_t time_last;           /* time of last succ. op. */
    char local_ip_addr[BOA_NI_MAXHOST]; /* for virtualhost */
    char remote_ip_addr[BOA_NI_MAXHOST]; /* after inet_ntoa */
    unsigned int remote_port;            /* could be used for ident */

    unsigned int kacount;                /* keepalive count */
    int client_stream_pos;      /* how much have we read... */

    /* everything below this line is kept regardless */
    char buffer[BUFFER_SIZE + 1]; /* generic I/O buffer */
    char request_uri[MAX_HEADER_LENGTH + 1]; /* uri */
    char client_stream[CLIENT_STREAM_SIZE]; /* data from client - fit or be hosed */
    char *cgi_env[CGI_ENV_MAX + 4]; /* CGI environment */

#ifdef ACCEPT_ON
    char accept[MAX_ACCEPT_LENGTH]; /* Accept: fields */
#endif

    struct request *next;       /* next */
    struct request *prev;       /* previous */
};

typedef struct request request;

struct status {
    long requests;
    long errors;
};

extern struct status status;

extern char *optarg;            /* For getopt */

extern request *request_ready;  /* first in ready list */
extern request *request_block;  /* first in blocked list */
extern request *request_free;   /* first in free list */

#ifdef HAVE_POLL
extern struct pollfd *pfds;
extern unsigned int pfd_len;
#else
extern fd_set block_read_fdset; /* fds blocked on read */
extern fd_set block_write_fdset; /* fds blocked on write */
extern int max_fd;
#endif

/* global server variables */

extern char *access_log_name;
extern char *error_log_name;
extern char *cgi_log_name;
extern int cgi_log_fd;
extern int use_localtime;

extern unsigned int server_port;
extern uid_t server_uid;
extern gid_t server_gid;
extern char *server_admin;
extern char *server_root;
extern char *server_name;
extern char *server_ip;

extern char *document_root;
extern char *user_dir;
extern char *directory_index;
extern char *default_type;
extern char *default_charset;
extern char *dirmaker;
extern char *mime_types;
extern char *pid_file;
extern char *cachedir;

extern const char *tempdir;

extern char *cgi_path;
extern short common_cgi_env_count;
extern int single_post_limit;
extern int conceal_server_identity;

extern int ka_timeout;
extern int unsigned default_timeout;
extern int unsigned ka_max;

extern int sighup_flag;
extern int sigchld_flag;
extern int sigalrm_flag;
extern int sigterm_flag;
extern time_t start_time;

extern int pending_requests;
extern unsigned max_connections;

extern int verbose_cgi_logs;

extern int backlog;
extern time_t current_time;

extern int virtualhost;
extern char *vhost_root;
extern const char *default_vhost;

extern unsigned total_connections;
extern unsigned int system_bufsize;      /* Default size of SNDBUF given by system */

extern sigjmp_buf env;
extern int handle_sigbus;
extern unsigned int cgi_umask;

#endif
