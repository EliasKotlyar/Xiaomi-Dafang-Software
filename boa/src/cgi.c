/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <paulp@go2net.com>
 *  Some changes Copyright (C) 1996 Charles F. Randall <crandall@goldsys.com>
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

/* $Id: cgi.c,v 1.83.2.28 2005/02/22 14:11:29 jnelson Exp $ */

#include "boa.h"

static char *env_gen_extra(const char *key, const char *value,
                           unsigned int extra);
static void create_argv(request * req, char **aargv);
static int complete_env(request * req);

int verbose_cgi_logs = 0;
/* The +1 is for the the NULL in complete_env */
static char **common_cgi_env = NULL;
short common_cgi_env_count = 0;

/*
 * Name: create_common_env
 *
 * Description: Set up the environment variables that are common to
 * all CGI scripts
 */

void create_common_env(void)
{
    int i;
    common_cgi_env = calloc((COMMON_CGI_COUNT + 1),sizeof(char *));
    common_cgi_env_count = 0;

    if (common_cgi_env == NULL) {
        DIE("unable to allocate memory for common_cgi_env");
    }

    /* NOTE NOTE NOTE:
       If you (the reader) someday modify this chunk of code to
       handle more "common" CGI environment variables, then bump the
       value COMMON_CGI_COUNT in defines.h UP

       Also, in the case of document_root and server_admin, two variables
       that may or may not be defined depending on the way the server
       is configured, we check for null values and use an empty
       string to denote a NULL value to the environment, as per the
       specification. The quote for which follows:

       "In all cases, a missing environment variable is
       equivalent to a zero-length (NULL) value, and vice versa."
     */
    common_cgi_env[common_cgi_env_count++] = env_gen_extra("PATH",
                                         ((cgi_path !=
                                           NULL) ? cgi_path :
                                          DEFAULT_PATH), 0);
    common_cgi_env[common_cgi_env_count++] =
        env_gen_extra("SERVER_SOFTWARE", SERVER_VERSION, 0);
    common_cgi_env[common_cgi_env_count++] = env_gen_extra("SERVER_NAME", server_name, 0);
    common_cgi_env[common_cgi_env_count++] =
        env_gen_extra("GATEWAY_INTERFACE", CGI_VERSION, 0);

    common_cgi_env[common_cgi_env_count++] =
        env_gen_extra("SERVER_PORT", simple_itoa(server_port), 0);

    /* NCSA and APACHE added -- not in CGI spec */
#ifdef USE_NCSA_CGI_ENV
    common_cgi_env[common_cgi_env_count++] =
        env_gen_extra("DOCUMENT_ROOT", document_root, 0);

    /* NCSA added */
    common_cgi_env[common_cgi_env_count++] = env_gen_extra("SERVER_ROOT", server_root, 0);
#endif

    /* APACHE added */
    common_cgi_env[common_cgi_env_count++] = env_gen_extra("SERVER_ADMIN", server_admin, 0);
    common_cgi_env[common_cgi_env_count] = NULL;

    /* Sanity checking -- make *sure* the memory got allocated */
    if (common_cgi_env_count != COMMON_CGI_COUNT) {
        log_error_time();
        fprintf(stderr, "COMMON_CGI_COUNT not high enough.\n");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < common_cgi_env_count; ++i) {
        if (common_cgi_env[i] == NULL) {
            log_error_time();
            fprintf(stderr,
                    "Unable to allocate a component of common_cgi_env - out of memory.\n");
            exit(EXIT_FAILURE);
        }
    }
}

void add_to_common_env(char *key, char *value)
{
    common_cgi_env = realloc(common_cgi_env, (common_cgi_env_count + 2) * (sizeof(char *)));
    if (common_cgi_env== NULL) {
        DIE("Unable to allocate memory for common CGI environment variable.");
    }
    common_cgi_env[common_cgi_env_count] = env_gen_extra(key,value, 0);
    if (common_cgi_env[common_cgi_env_count] == NULL) {
        /* errors already reported */
        DIE("memory allocation failure in add_to_common_env");
    }
    common_cgi_env[++common_cgi_env_count] = NULL;
    /* I find it hard to believe that somebody would actually
     * make 90+ *common* CGI variables, but it's always better
     * to be safe.
     */
    if (common_cgi_env_count > CGI_ENV_MAX) {
        DIE("far too many common CGI environment variables added.");
    }
}

void clear_common_env(void)
{
    int i;

    for (i = 0; i <= COMMON_CGI_COUNT; ++i) {
        if (common_cgi_env[i] != NULL) {
            free(common_cgi_env[i]);
            common_cgi_env[i] = NULL;
        }
    }
}

/*
 * Name: env_gen_extra
 *       (and via a not-so-tricky #define, env_gen)
 * This routine calls malloc: please free the memory when you are done
 */
static char *env_gen_extra(const char *key, const char *value,
                           unsigned int extra)
{
    char *result;
    unsigned int key_len, value_len;

    if (value == NULL)          /* ServerAdmin may not be defined, eg */
        value = "";
    key_len = strlen(key);
    value_len = strlen(value);
    /* leave room for '=' sign and null terminator */
    result = malloc(extra + key_len + value_len + 2);
    if (result) {
        memcpy(result + extra, key, key_len);
        *(result + extra + key_len) = '=';
        memcpy(result + extra + key_len + 1, value, value_len);
        *(result + extra + key_len + value_len + 1) = '\0';
    } else {
        log_error_time();
        perror("malloc");
        log_error_time();
        fprintf(stderr, "tried to allocate (key=value) extra=%u: %s=%s\n",
                extra, key, value);
    }
    return result;
}

/*
 * Name: add_cgi_env
 *
 * Description: adds a variable to CGI's environment
 * Used for HTTP_ headers
 */

int add_cgi_env(request * req, const char *key, const char *value,
                int http_prefix)
{
    char *p;
    unsigned int prefix_len;

    if (http_prefix) {
        prefix_len = 5;
    } else {
        prefix_len = 0;
    }

    if (req->cgi_env_index < CGI_ENV_MAX) {
        p = env_gen_extra(key, value, prefix_len);
        if (!p) {
            log_error_doc(req);
            fprintf(stderr,
                    "Unable to generate additional CGI environment "
                    "variable -- ran out of memory!\n");
            return 0;
        }
        if (prefix_len)
            memcpy(p, "HTTP_", 5);
        req->cgi_env[req->cgi_env_index++] = p;
        return 1;
    }
    log_error_doc(req);
    fprintf(stderr, "Unable to generate additional CGI Environment "
            "variable \"%s%s=%s\" -- not enough space!\n",
            (prefix_len ? "HTTP_" : ""), key, value);
    return 0;
}

#define my_add_cgi_env(req, key, value) { \
    int ok = add_cgi_env(req, key, value, 0); \
    if (!ok) return 0; \
    }

/*
 * Name: complete_env
 *
 * Description: adds the known client header env variables
 * and terminates the environment array
 */

static int complete_env(request * req)
{
    int i;

    for (i = 0; common_cgi_env[i]; i++)
        req->cgi_env[i] = common_cgi_env[i];

    {
        const char *w;
        switch (req->method) {
        case M_POST:
            w = "POST";
            break;
        case M_HEAD:
            w = "HEAD";
            break;
        case M_GET:
            w = "GET";
            break;
        default:
            w = "UNKNOWN";
            break;
        }
        my_add_cgi_env(req, "REQUEST_METHOD", w);
    }

    if (req->header_host)
        my_add_cgi_env(req, "HTTP_HOST", req->header_host);
    my_add_cgi_env(req, "SERVER_ADDR", req->local_ip_addr);
    my_add_cgi_env(req, "SERVER_PROTOCOL",
                   http_ver_string(req->http_version));
    my_add_cgi_env(req, "REQUEST_URI", req->request_uri);

    if (req->path_info)
        my_add_cgi_env(req, "PATH_INFO", req->path_info);

    if (req->path_translated)
        /* while path_translated depends on path_info,
         * there are cases when path_translated might
         * not exist when path_info does
         */
        my_add_cgi_env(req, "PATH_TRANSLATED", req->path_translated);

    my_add_cgi_env(req, "SCRIPT_NAME", req->script_name);

    if (req->query_string)
        my_add_cgi_env(req, "QUERY_STRING", req->query_string);
    my_add_cgi_env(req, "REMOTE_ADDR", req->remote_ip_addr);
    my_add_cgi_env(req, "REMOTE_PORT", simple_itoa(req->remote_port));

    if (req->method == M_POST) {
        if (req->content_type) {
            my_add_cgi_env(req, "CONTENT_TYPE", req->content_type);
        } else {
            my_add_cgi_env(req, "CONTENT_TYPE", default_type);
        }
        if (req->content_length) {
            my_add_cgi_env(req, "CONTENT_LENGTH", req->content_length);
        }
    }
#ifdef ACCEPT_ON
    if (req->accept[0])
        my_add_cgi_env(req, "HTTP_ACCEPT", req->accept);
#endif

    if (req->cgi_env_index < CGI_ENV_MAX + 1) {
        req->cgi_env[req->cgi_env_index] = NULL; /* terminate */
        return 1;
    }
    log_error_doc(req);
    fprintf(stderr, "Not enough space in CGI environment for remainder"
            " of variables.\n");
    return 0;
}

/*
 * Name: make_args_cgi
 *
 * Build argv list for a CGI script according to spec
 *
 */

static void create_argv(request * req, char **aargv)
{
    char *p, *q, *r;
    int aargc;

    q = req->query_string;
    aargv[0] = req->pathname;

    /* here, we handle a special "indexed" query string.
     * Taken from the CGI/1.1 SPEC:
     * This is identified by a GET or HEAD request with a query string
     * with no *unencoded* '=' in it.
     * For such a request, I'm supposed to parse the search string
     * into words, according to the following rules:

     search-string = search-word *( "+" search-word )
     search-word   = 1*schar
     schar         = xunreserved | escaped | xreserved
     xunreserved   = alpha | digit | xsafe | extra
     xsafe         = "$" | "-" | "_" | "."
     xreserved     = ";" | "/" | "?" | ":" | "@" | "&"

     After parsing, each word is URL-decoded, optionally encoded in a system
     defined manner, and then the argument list
     is set to the list of words.


     Thus, schar is alpha|digit|"$"|"-"|"_"|"."|";"|"/"|"?"|":"|"@"|"&"

     As of this writing, escape.pl escapes the following chars:

     "-", "_", ".", "!", "~", "*", "'", "(", ")",
     "0".."9", "A".."Z", "a".."z",
     ";", "/", "?", ":", "@", "&", "=", "+", "\$", ","

     Which therefore means
     "=", "+", "~", "!", "*", "'", "(", ")", ","
     are *not* escaped and should be?
     Wait, we don't do any escaping, and nor should we.
     According to the RFC draft, we unescape and then re-escape
     in a "system defined manner" (here: none).

     The CGI/1.1 draft (03, latest is 1999???) is very unclear here.

     I am using the latest published RFC, 2396, for what does and does
     not need escaping.

     Since boa builds the argument list and does not call /bin/sh,
     (boa uses execve for CGI)
     */

    if (q && !strchr(q, '=')) {
        /* we have an 'index' style */
        q = strdup(q);
        if (!q) {
            log_error_doc(req);
            fputs("unable to strdup 'q' in create_argv!\n", stderr);
            _exit(EXIT_FAILURE);
        }
        for (aargc = 1; q && (aargc < CGI_ARGC_MAX);) {
            r = q;
            /* for an index-style CGI, + is used to separate arguments
             * an escaped '+' is of no concern to us
             */
            if ((p = strchr(q, '+'))) {
                *p = '\0';
                q = p + 1;
            } else {
                q = NULL;
            }
            if (unescape_uri(r, NULL)) {
                /* printf("parameter %d: %s\n",aargc,r); */
                aargv[aargc++] = r;
            }
        }
        aargv[aargc] = NULL;
    } else {
        aargv[1] = NULL;
    }
}

/*
 * Name: init_cgi
 *
 * Description: Called for GET/POST requests that refer to ScriptAlias
 * directories or application/x-httpd-cgi files.  Ties stdout to socket,
 * stdin to data if POST, and execs CGI.
 * stderr remains tied to our log file; is this good?
 *
 * Returns:
 * 0 - error or NPH, either way the socket is closed
 * 1 - success
 */

int init_cgi(request * req)
{
    int child_pid;
    int pipes[2];
    int use_pipes = 0;

    SQUASH_KA(req);

    if (req->cgi_type) {
        if (complete_env(req) == 0) {
            return 0;
        }
    }
    DEBUG(DEBUG_CGI_ENV) {
        int i;
        for (i = 0; i < req->cgi_env_index; ++i)
            log_error_time();
            fprintf(stderr, "%s - environment variable for cgi: \"%s\"\n",
                    __FILE__, req->cgi_env[i]);
    }

    /* we want to use pipes whenever it's a CGI or directory */
    /* otherwise (NPH, gunzip) we want no pipes */
    if (req->cgi_type == CGI ||
        (!req->cgi_type &&
         (req->pathname[strlen(req->pathname) - 1] == '/'))) {
        use_pipes = 1;
        if (pipe(pipes) == -1) {
            log_error_doc(req);
            perror("pipe");
            return 0;
        }

        /* set the read end of the socket to non-blocking */
        if (set_nonblock_fd(pipes[0]) == -1) {
            log_error_doc(req);
            perror("cgi-fcntl");
            close(pipes[0]);
            close(pipes[1]);
            return 0;
        }
    }

    child_pid = fork();
    switch (child_pid) {
    case -1:
        /* fork unsuccessful */
        /* FIXME: There is a problem here. send_r_error (called by
         * boa_perror) would work for NPH and CGI, but not for GUNZIP.  
         * Fix that. 
         */
        boa_perror(req, "fork failed");
        if (use_pipes) {
            close(pipes[0]);
            close(pipes[1]);
        }
        return 0;
        break;
    case 0:
        /* child */
        reset_signals();

        if (req->cgi_type == CGI || req->cgi_type == NPH) {
            char *c;
            unsigned int l;
            char *newpath, *oldpath;

            c = strrchr(req->pathname, '/');
            if (!c) {
                /* there will always be a '.' */
                log_error_doc(req);
                fprintf(stderr,
                        "unable to find '/' in req->pathname: \"%s\"\n",
                        req->pathname);
                if (use_pipes)
                    close(pipes[1]);
                _exit(EXIT_FAILURE);
            }

            *c = '\0';

            if (chdir(req->pathname) != 0) {
                int saved_errno = errno;
                log_error_doc(req);
                fprintf(stderr, "Could not chdir to \"%s\":",
                        req->pathname);
                errno = saved_errno;
                perror("chdir");
                if (use_pipes)
                    close(pipes[1]);
                _exit(EXIT_FAILURE);
            }

            oldpath = req->pathname;
            req->pathname = ++c;
            l = strlen(req->pathname) + 3;
            /* prefix './' */
            newpath = malloc(sizeof (char) * l);
            if (!newpath) {
                /* there will always be a '.' */
                log_error_doc(req);
                perror("unable to malloc for newpath");
                if (use_pipes)
                    close(pipes[1]);
                _exit(EXIT_FAILURE);
            }
            newpath[0] = '.';
            newpath[1] = '/';
            memcpy(&newpath[2], req->pathname, l - 2); /* includes the trailing '\0' */
            free(oldpath);
            req->pathname = newpath;
        }
        if (use_pipes) {
            /* close the 'read' end of the pipes[] */
            close(pipes[0]);
            /* tie CGI's STDOUT to our write end of pipe */
            if (dup2(pipes[1], STDOUT_FILENO) == -1) {
                log_error_doc(req);
                perror("dup2 - pipes");
                _exit(EXIT_FAILURE);
            }
            close(pipes[1]);
        } else {
            /* tie stdout to socket */
            if (dup2(req->fd, STDOUT_FILENO) == -1) {
                log_error_doc(req);
                perror("dup2 - fd");
                _exit(EXIT_FAILURE);
            }
            close(req->fd);
        }
        /* Switch socket flags back to blocking */
        if (set_block_fd(STDOUT_FILENO) == -1) {
            log_error_doc(req);
            perror("cgi-fcntl");
            _exit(EXIT_FAILURE);
        }
        /* tie post_data_fd to POST stdin */
        if (req->method == M_POST) { /* tie stdin to file */
            lseek(req->post_data_fd, SEEK_SET, 0);
            dup2(req->post_data_fd, STDIN_FILENO);
            close(req->post_data_fd);
        }

#ifdef USE_SETRLIMIT
        /* setrlimit stuff.
         * This is neat!
         * RLIMIT_STACK    max stack size
         * RLIMIT_CORE     max core file size
         * RLIMIT_RSS      max resident set size
         * RLIMIT_NPROC    max number of processes
         * RLIMIT_NOFILE   max number of open files
         * RLIMIT_MEMLOCK  max locked-in-memory address space
         * RLIMIT_AS       address space (virtual memory) limit
         *
         * RLIMIT_CPU      CPU time in seconds
         * RLIMIT_DATA     max data size
         *
         * Currently, we only limit the CPU time and the DATA segment
         * We also "nice" the process.
         *
         * This section of code adapted from patches sent in by Steve Thompson
         * (no email available)
         */

        {
            struct rlimit rl;
            int retval;

            if (cgi_rlimit_cpu) {
                rl.rlim_cur = rl.rlim_max = cgi_rlimit_cpu;
                retval = setrlimit(RLIMIT_CPU, &rl);
                if (retval == -1) {
                    log_error_time();
                    fprintf(stderr,
                            "setrlimit(RLIMIT_CPU,%d): %s\n",
                            rlimit_cpu, strerror(errno));
                    _exit(EXIT_FAILURE);
                }
            }

            if (cgi_limit_data) {
                rl.rlim_cur = rl.rlim_max = cgi_rlimit_data;
                retval = setrlimit(RLIMIT_DATA, &rl);
                if (retval == -1) {
                    log_error_time();
                    fprintf(stderr,
                            "setrlimit(RLIMIT_DATA,%d): %s\n",
                            rlimit_data, strerror(errno));
                    _exit(EXIT_FAILURE);
                }
            }

            if (cgi_nice) {
                retval = nice(cgi_nice);
                if (retval == -1) {
                    log_error_time();
                    perror("nice");
                    _exit(EXIT_FAILURE);
                }
            }
        }
#endif

        umask(cgi_umask);       /* change umask *again* u=rwx,g=rxw,o= */

        /*
         * tie STDERR to cgi_log_fd
         * cgi_log_fd will automatically close, close-on-exec rocks!
         * if we don't tie STDERR (current log_error) to cgi_log_fd,
         *  then we ought to tie it to /dev/null
         *  FIXME: we currently don't tie it to /dev/null, we leave it
         *  tied to whatever 'error_log' points to.  This means CGIs can
         *  scribble on the error_log, probably a bad thing.
         */
        if (cgi_log_fd) {
            dup2(cgi_log_fd, STDERR_FILENO);
        }

        if (req->cgi_type) {
            char *aargv[CGI_ARGC_MAX + 1];
            create_argv(req, aargv);
            execve(req->pathname, aargv, req->cgi_env);
        } else {
            if (req->pathname[strlen(req->pathname) - 1] == '/')
                execl(dirmaker, dirmaker, req->pathname, req->request_uri,
                      (void *) NULL);
#ifdef GUNZIP
            else
                execl(GUNZIP, GUNZIP, "--stdout", "--decompress",
                      req->pathname, (void *) NULL);
#endif
        }
        /* execve failed */
        log_error_doc(req);
        fprintf(stderr, "Unable to execve/execl pathname: \"%s\"",
                req->pathname);
        perror("");
        _exit(EXIT_FAILURE);
        break;

    default:
        /* parent */
        /* if here, fork was successful */
        if (verbose_cgi_logs) {
            log_error_time();
            fprintf(stderr, "Forked child \"%s\" pid %d\n",
                    req->pathname, child_pid);
        }

        if (req->method == M_POST) {
            close(req->post_data_fd); /* child closed it too */
            req->post_data_fd = 0;
        }

        /* NPH, GUNZIP, etc... all go straight to the fd */
        if (!use_pipes)
            return 0;

        close(pipes[1]);
        req->data_fd = pipes[0];

        req->status = PIPE_READ;
        if (req->cgi_type == CGI) {
            req->cgi_status = CGI_PARSE; /* got to parse cgi header */
            /* for cgi_header... I get half the buffer! */
            req->header_line = req->header_end =
                (req->buffer + BUFFER_SIZE / 2);
        } else {
            req->cgi_status = CGI_BUFFER;
            /* I get all the buffer! */
            req->header_line = req->header_end = req->buffer;
        }

        /* reset req->filepos for logging (it's used in pipe.c) */
        /* still don't know why req->filesize might be reset though */
        req->filepos = 0;
        break;
    }

    return 1;
}
