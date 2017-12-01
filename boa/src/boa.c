/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <paulp@go2net.com>
 *  Some changes Copyright (C) 1996 Charles F. Randall <crandall@goldsys.com>
 *  Copyright (C) 1996-1999 Larry Doolittle <ldoolitt@boa.org>
 *  Copyright (C) 1996-2005 Jon Nelson <jnelson@boa.org>
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

/* $Id: boa.c,v 1.99.2.26 2005/02/22 14:11:29 jnelson Exp $*/

#include "boa.h"

/* globals */
int backlog = SO_MAXCONN;
time_t start_time;

int debug_level = 0;
int sighup_flag = 0;            /* 1 => signal has happened, needs attention */
int sigchld_flag = 0;           /* 1 => signal has happened, needs attention */
int sigalrm_flag = 0;           /* 1 => signal has happened, needs attention */
int sigterm_flag = 0;           /* lame duck mode */
time_t current_time;
int pending_requests = 0;

extern const char *config_file_name;

/* static to boa.c */
static void usage(const char *programname);
static void parse_commandline(int argc, char *argv[]);
static void fixup_server_root(void);
static int create_server_socket(void);
static void drop_privs(void);

static int sock_opt = 1;
static int do_fork = 1;

int main(int argc, char *argv[])
{
    int server_s;               /* boa socket */
    pid_t pid;

    /* set umask to u+rw, u-x, go-rwx */
    /* according to the man page, umask always succeeds */
    umask(077);

    /* but first, update timestamp, because log_error_time uses it */
    (void) time(&current_time);

    /* set timezone right away */
    tzset();

    {
        int devnullfd = -1;
        devnullfd = open("/dev/null", 0);

        /* make STDIN point to /dev/null */
        if (devnullfd == -1) {
            DIE("can't open /dev/null");
        }

        if (dup2(devnullfd, STDIN_FILENO) == -1) {
            DIE("can't dup2 /dev/null to STDIN_FILENO");
        }

        (void) close(devnullfd);
    }

    parse_commandline(argc, argv);
    fixup_server_root();
    read_config_files();
    create_common_env();
    open_logs();
    server_s = create_server_socket();
    init_signals();
    build_needs_escape();

    /* background ourself */
    if (do_fork) {
        pid = fork();
    } else {
        pid = getpid();
    }

    switch (pid) {
    case -1:
        /* error */
        perror("fork/getpid");
        exit(EXIT_FAILURE);
    case 0:
        /* child, success */
        break;
    default:
        /* parent, success */
        if (pid_file != NULL) {
            FILE *PID_FILE = fopen(pid_file, "w");
            if (PID_FILE != NULL) {
                fprintf(PID_FILE, "%d", pid);
                fclose(PID_FILE);
            } else {
                perror("fopen pid file");
            }
        }

        if (do_fork)
            exit(EXIT_SUCCESS);
        break;
    }

    drop_privs();
    /* main loop */
    timestamp();

    status.requests = 0;
    status.errors = 0;

    start_time = current_time;
    loop(server_s);
    return 0;
}

static void usage(const char *programname)
{
    fprintf(stderr,
            "Usage: %s [-c serverroot] [-d] [-f configfile] [-r chroot]%s\n",
            programname,
#ifndef DISABLE_DEBUG
            " [-l debug_level]"
#else
            ""
#endif
        );
#ifndef DISABLE_DEBUG
    print_debug_usage();
#endif
    exit(EXIT_FAILURE);

}

static void parse_commandline(int argc, char *argv[])
{
    int c;                      /* command line arg */

    while ((c = getopt(argc, argv, "c:dl:f:r:")) != -1) {
        switch (c) {
        case 'c':
            if (server_root)
                free(server_root);
            server_root = strdup(optarg);
            if (!server_root) {
                perror("strdup (for server_root)");
                exit(EXIT_FAILURE);
            }
            break;
        case 'd':
            do_fork = 0;
            break;
        case 'f':
            config_file_name = optarg;
            break;
        case 'r':
            if (chdir(optarg) == -1) {
                log_error_time();
                perror("chdir (to chroot)");
                exit(EXIT_FAILURE);
            }
            if (chroot(optarg) == -1) {
                log_error_time();
                perror("chroot");
                exit(EXIT_FAILURE);
            }
            if (chdir("/") == -1) {
                log_error_time();
                perror("chdir (after chroot)");
                exit(EXIT_FAILURE);
            }
            break;
#ifndef DISABLE_DEBUG
        case 'l':
            parse_debug(optarg);
            break;
#endif
        default:
            usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}

static int create_server_socket(void)
{
    int server_s;

    server_s = socket(SERVER_PF, SOCK_STREAM, IPPROTO_TCP);
    if (server_s == -1) {
        DIE("unable to create socket");
    }

    /* server socket is nonblocking */
    if (set_nonblock_fd(server_s) == -1) {
        DIE("fcntl: unable to set server socket to nonblocking");
    }

    /* close server socket on exec so CGIs can't write to it */
    if (fcntl(server_s, F_SETFD, 1) == -1) {
        DIE("can't set close-on-exec on server socket!");
    }

    /* reuse socket addr */
    if ((setsockopt(server_s, SOL_SOCKET, SO_REUSEADDR, (void *) &sock_opt,
                    sizeof (sock_opt))) == -1) {
        DIE("setsockopt");
    }

    /* Internet family-specific code encapsulated in bind_server()  */
    if (bind_server(server_s, server_ip, server_port) == -1) {
        DIE("unable to bind");
    }

    /* listen: large number just in case your kernel is nicely tweaked */
    if (listen(server_s, backlog) == -1) {
        DIE("unable to listen");
    }
    return server_s;
}

static void drop_privs(void)
{
    /* give away our privs if we can */
    if (getuid() == 0) {
        struct passwd *passwdbuf;
        passwdbuf = getpwuid(server_uid);
        if (passwdbuf == NULL) {
            DIE("getpwuid");
        }
        if (initgroups(passwdbuf->pw_name, passwdbuf->pw_gid) == -1) {
            DIE("initgroups");
        }
        if (setgid(server_gid) == -1) {
            DIE("setgid");
        }
        if (setuid(server_uid) == -1) {
            DIE("setuid");
        }
        /* test for failed-but-return-was-successful setuid
         * http://www.securityportal.com/list-archive/bugtraq/2000/Jun/0101.html
         */
        if (server_uid != 0 && setuid(0) != -1) {
            DIE("icky Linux kernel bug!");
        }
    } else {
        if (server_gid || server_uid) {
            log_error_time();
            fprintf(stderr, "Warning: "
                    "Not running as root: no attempt to change"
                    " to uid %u gid %u\n", server_uid, server_gid);
        }
        server_gid = getgid();
        server_uid = getuid();
    }
}

/*
 * Name: fixup_server_root
 *
 * Description: Makes sure the server root is valid.
 *
 */

static void fixup_server_root()
{
    if (!server_root) {
#ifdef SERVER_ROOT
        server_root = strdup(SERVER_ROOT);
        if (!server_root) {
            perror("strdup (SERVER_ROOT)");
            exit(EXIT_FAILURE);
        }
#else
        fputs("boa: don't know where server root is.  Please #define "
              "SERVER_ROOT in boa.h\n"
              "and recompile, or use the -c command line option to "
              "specify it.\n", stderr);
        exit(EXIT_FAILURE);
#endif
    }

    if (chdir(server_root) == -1) {
        fprintf(stderr, "Could not chdir to \"%s\": aborting\n",
                server_root);
        exit(EXIT_FAILURE);
    }
}
