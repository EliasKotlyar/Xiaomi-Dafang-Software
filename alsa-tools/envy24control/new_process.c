#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef MAX_PARAM
#define MAX_PARAM 10
#endif

/*
 * start child process
 */
int new_process(char * const cmd_line[MAX_PARAM])
{
	int proc_status;
	pid_t pid;
	pid_t w;
	struct stat file_status;

	/* memory for storage of function pointers from the signal handling routines */
	void (*int_stat)();
	void (*quit_stat)();
	void (*usr2_stat)();

	/*
	 * check command file
	 */

	/* search file */
	if (stat(cmd_line[0], &file_status) < 0) {
		fprintf(stderr, "Cannot find program '%s'.\n", cmd_line[0]);
		fprintf(stderr, "You must specify path for '%s'.\n", cmd_line[0]);
		return -errno;
	}

	proc_status = 0;
	/* check file status and permissions */
	if (file_status.st_mode & S_IFREG) {
		if (!(file_status.st_mode & S_IXOTH)) {
			if (!(file_status.st_mode & S_IXGRP)) {
				if (!(file_status.st_mode & S_IXUSR)) {
					proc_status = -EACCES;
				} else if (file_status.st_uid != getuid()) {
					proc_status = -EACCES;
				}
			} else if ((file_status.st_gid != getgid()) && (file_status.st_uid != getuid())) {
				proc_status = -EACCES;
			}
		}
	} else {
		proc_status = -EACCES;
	}
		
	if (proc_status != 0) {
		fprintf(stderr, "No permissions to execute program '%s'.\n", cmd_line[0]);
		return proc_status;
	}

	if ( (pid = fork() ) == 0) {
		execv(cmd_line[0], cmd_line);
	}

	/* for waiting ingnoring special interrupts */

	int_stat = signal(SIGINT, SIG_IGN);
	quit_stat = signal(SIGQUIT, SIG_IGN);
	usr2_stat = signal(SIGUSR2, SIG_IGN);

	/* waiting for the end of the child process */

	while ( ( (w = wait(&proc_status)) != pid ) && (w != -1) )
		;
	if (w == -1) {
		proc_status = -errno;
	}

	/* restore pointers from signal handling routines */

	signal(SIGINT, int_stat);
	signal(SIGQUIT, quit_stat);
	signal(SIGUSR2, usr2_stat);

	return proc_status;
}
