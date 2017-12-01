/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <paulp@go2net.com>
 *  Copyright (C) 1996-1999 Larry Doolittle <ldoolitt@boa.org>
 *  Copyright (C) 1996-2005 Jon Nelson <jnelson@boa.org>
 *  Some changes Copyright (C) 1997 Alain Magloire <alain.magloire@rcsm.ee.mcgill.ca>
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

/* $Id: signals.c,v 1.37.2.14 2005/02/22 14:11:29 jnelson Exp $*/

#include "boa.h"
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>           /* wait */
#endif
#include <signal.h>             /* signal */

sigjmp_buf env;
int handle_sigbus;

void sigsegv(int);
void sigbus(int);
void sigterm(int);
void sighup(int);
void sigint(int);
void sigchld(int);
void sigalrm(int);

/*
 * Name: init_signals
 * Description: Sets up signal handlers for all our friends.
 */

void init_signals(void)
{
    struct sigaction sa;

    sa.sa_flags = 0;

    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGSEGV);
    sigaddset(&sa.sa_mask, SIGBUS);
    sigaddset(&sa.sa_mask, SIGTERM);
    sigaddset(&sa.sa_mask, SIGHUP);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGPIPE);
    sigaddset(&sa.sa_mask, SIGCHLD);
    sigaddset(&sa.sa_mask, SIGALRM);
    sigaddset(&sa.sa_mask, SIGUSR1);
    sigaddset(&sa.sa_mask, SIGUSR2);

    sa.sa_handler = sigsegv;
    sigaction(SIGSEGV, &sa, NULL);

    sa.sa_handler = sigbus;
    sigaction(SIGBUS, &sa, NULL);

    sa.sa_handler = sigterm;
    sigaction(SIGTERM, &sa, NULL);

    sa.sa_handler = sighup;
    sigaction(SIGHUP, &sa, NULL);

    sa.sa_handler = sigint;
    sigaction(SIGINT, &sa, NULL);

    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);

    sa.sa_handler = sigchld;
    sigaction(SIGCHLD, &sa, NULL);

    sa.sa_handler = sigalrm;
    sigaction(SIGALRM, &sa, NULL);

    sa.sa_handler = SIG_IGN;
    sigaction(SIGUSR1, &sa, NULL);

    sa.sa_handler = SIG_IGN;
    sigaction(SIGUSR2, &sa, NULL);
}

void reset_signals(void)
{
    struct sigaction sa;

    sa.sa_flags = 0;
    sa.sa_handler = SIG_DFL;

    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGSEGV);
    sigaddset(&sa.sa_mask, SIGBUS);
    sigaddset(&sa.sa_mask, SIGTERM);
    sigaddset(&sa.sa_mask, SIGHUP);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGPIPE);
    sigaddset(&sa.sa_mask, SIGCHLD);
    sigaddset(&sa.sa_mask, SIGALRM);
    sigaddset(&sa.sa_mask, SIGUSR1);
    sigaddset(&sa.sa_mask, SIGUSR2);

    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGPIPE, &sa, NULL);
    sigaction(SIGCHLD, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
}

void sigsegv(int dummy)
{
    time(&current_time);
    log_error_time();
    fprintf(stderr, "caught SIGSEGV, dumping core in %s\n", tempdir);
    chdir(tempdir);
    abort();
}

extern sigjmp_buf env;
extern int handle_sigbus;

void sigbus(int dummy)
{
    if (handle_sigbus) {
        longjmp(env, dummy);
    }
    time(&current_time);
    log_error_time();
    fprintf(stderr, "caught SIGBUS, dumping core in %s\n", tempdir);
    chdir(tempdir);
    abort();
}

void sigterm(int dummy)
{
    if (!sigterm_flag)
        sigterm_flag = 1;
}

void sigterm_stage1_run(void)
{                               /* lame duck mode */
    time(&current_time);
    log_error_time();
    fputs("caught SIGTERM, starting shutdown\n", stderr);
    sigterm_flag = 2;
}

void sigterm_stage2_run(void)
{                               /* lame duck mode */
    log_error_time();
    fprintf(stderr,
            "exiting Boa normally (uptime %d seconds)\n",
            (int) (current_time - start_time));
    chdir(tempdir);
    clear_common_env();
    dump_mime();
    dump_passwd();
    dump_alias();
    free_requests();
    range_pool_empty();
    free(server_root);
    free(server_name);
    server_root = NULL;
    exit(EXIT_SUCCESS);
}


void sighup(int dummy)
{
    sighup_flag = 1;
}

void sighup_run(void)
{
    sighup_flag = 0;
    time(&current_time);
    log_error_time();
    fputs("caught SIGHUP, restarting\n", stderr);

    /* Philosophy change for 0.92: don't close and attempt reopen of logfiles,
     * since usual permission structure prevents such reopening.
     */

    /* why ? */
    /*
       FD_ZERO(&block_read_fdset);
       FD_ZERO(&block_write_fdset);
     */
    /* clear_common_env(); NEVER DO THIS */
    dump_mime();
    dump_passwd();
    dump_alias();
    free_requests();
    range_pool_empty();

    log_error_time();
    fputs("re-reading configuration files\n", stderr);
    read_config_files();

    log_error_time();
    fputs("successful restart\n", stderr);
}

void sigint(int dummy)
{
    time(&current_time);
    log_error_time();
    fputs("caught SIGINT: shutting down\n", stderr);
    chdir(tempdir);
    exit(EXIT_FAILURE);
}

void sigchld(int dummy)
{
    sigchld_flag = 1;
}

void sigchld_run(void)
{
    int child_status;
    pid_t pid;

    sigchld_flag = 0;

    while ((pid = waitpid(-1, &child_status, WNOHANG)) > 0)
        if (verbose_cgi_logs) {
            time(&current_time);
            log_error_time();
            fprintf(stderr, "reaping child %d: status %d\n", (int) pid,
                    child_status);
        }
    return;
}

void sigalrm(int dummy)
{
    sigalrm_flag = 1;
}

void sigalrm_run(void)
{
    time(&current_time);
    log_error_time();
    fprintf(stderr, "%ld requests, %ld errors\n",
            status.requests, status.errors);
    hash_show_stats();
    sigalrm_flag = 0;
}
