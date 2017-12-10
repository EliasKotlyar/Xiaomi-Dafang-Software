/*
 * Purpose: "Reverse telnet" helper utility for OSS technical support
 *
 * Description:
 * The osspartysh program is an utility that permits safe access to the
 * customer systems by the OSS support staff.
 *
 * Unlike usual ssh/telnet access this program works in revewrse way. The
 * person who gives technical support can establish a termibal server using
 * osspartysh -d -p <TCP port>. The client can then connect to this
 * terminal server program by executing osspartysh -h <the support host> -p <port>.
 * Outgoing TCP connections can easily pass usual firewall and NAT settings
 * which makes establishing the connection very easy. After the connection is 
 * open both sides can type commands and see what happens in the system. If
 * the customer thiks the tech suport guy is doing something too dangerous he
 * can always hit ^C or ^Z and unplug the ethernet connector.
 */
/*
 *
 * This file is part of Open Sound System.
 *
 * Copyright (C) 4Front Technologies 1996-2008.
 *
 * This this source file is released under GPL v2 license (no other versions).
 * See the COPYING file included in the main directory of this source
 * distribution for the license terms and conditions.
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#ifdef __FreeBSD__
#include <libutil.h>
#else
#include <pty.h>
#include <utmp.h>
#endif
#include <errno.h>

#define PARTYSH_MAGIC		"ParTySH"
#define PARTYSH_VERSION		"0100"		// 1.0
#define PARTYSH_HDRSIZE		250

int localfd=0;
int connfd = -1;
struct termios saved_tc;
int listenfd=-1;
int off=0;
int on=1;

#define max(a, b) ((a>b) ? a : b)

static void
restore_terminal(void)
{
	if (tcsetattr(localfd, 0, &saved_tc) == -1)
	   perror("tcsetattr");
}

static void
setup_terminal(void)
{
	struct termios tc;

		if (tcgetattr(0, &tc)==-1)
		{
			perror("tcgetattr");
			exit(1);
		}

	   	if (tcgetattr(0, &saved_tc)==-1)
		{
			perror("tcgetattr");
			exit(1);
		}

		atexit(restore_terminal);

		tc.c_lflag &= ~(ICANON|ECHO|ISIG);
		tc.c_cc[VMIN]=1;
		tc.c_cc[VTIME]=1;

		if (tcsetattr(0, 0, &tc) == -1)
		   perror("tcsetattr");
}

int
wait_connect(void)
{
	if (listen(listenfd, 1) == -1)
	{
		perror("listen");
		exit(1);
	}

	printf("Waiting for a new connection - hit ^C to exit.\n");

	if ((connfd=accept(listenfd, NULL, NULL))==-1)
	{
		perror("accept");
		exit(1);
	}

	printf("Connected\n\n");fflush(stdout);

	printf("You may need to type ^L after running vi.\n");
	printf("\n");
	printf("Hit enter couple of times after exiting the shell so that\n");
	printf("you can kill the osspartysh program. If the program is left\n");
	printf("running after a session then new clients can connect the same host/port again.\n");
	printf("\n");fflush(stdout);

	setsockopt(connfd, IPPROTO_TCP, TCP_NODELAY, &off, sizeof(off));

	return 1;
}

static void
terminal_loop(int connfd)
{
	fd_set readfds;
	int n, l;
	char buf[512];

	while (1)
	{
		FD_ZERO(&readfds);

		FD_SET(0, &readfds);
		FD_SET(connfd, &readfds);

		if ((n=select(connfd+1, &readfds, NULL, NULL, NULL))==-1)
		{
			perror("select");
			return;
		}

		if (FD_ISSET(0, &readfds))
		{
			if ((l=read(0, buf, sizeof(buf)))<0)
			{
				perror("read(0)");
				return;
			}

			if (write(connfd, buf, l)!=l)
			{
				perror("write(conn)");
				return;
			}
		}

		if (FD_ISSET(connfd, &readfds))
		{
			if ((l=read(connfd, buf, sizeof(buf)))<0)
			{
				perror("read(conn)");
				exit(0);
			}

			if (write(1, buf, l)!=l)
			{
				perror("write(1)");
				return;
			}
		}
	}
}

static void
shell_loop(int connfd, int infd, int outfd)
{
	fd_set readfds;
	int n, l;
	char buf[512];

	while (1)
	{
		FD_ZERO(&readfds);

		FD_SET(infd, &readfds);
		if (infd != 0)
		   FD_SET(0, &readfds);
		FD_SET(connfd, &readfds);

		if ((n=select(max(infd, connfd)+1, &readfds, NULL, NULL, NULL))==-1)
		{
			perror("select");
			return;
		}

		if (FD_ISSET(infd, &readfds))
		{
			if ((l=read(infd, buf, sizeof(buf)))<0)
			{
				if (errno == EIO)
				{
					printf("Disconnected\n");
					exit(1);
				}

				perror("read(infd)");
				return;
			}

			if (write(connfd, buf, l)!=l)
			{
				perror("write(conn)");
				return;
			}

			if (write(1, buf, l)!=l) // Echo to the local terminal
			   exit(1);
		}

		if (FD_ISSET(connfd, &readfds))
		{
			if ((l=read(connfd, buf, sizeof(buf)))<0)
			{
				perror("read(conn)");
				exit(0);
			}

			if (write(outfd, buf, l)!=l)
			{
				perror("write(outfd)");
				return;
			}

			// write(1, buf, l); // Echo to the local terminal
		}

		if (infd != 0 && FD_ISSET(0, &readfds))
		{
			if ((l=read(0, buf, sizeof(buf)))<0)
			{
				perror("read(0)");
				exit(0);
			}

			if (write(outfd, buf, l)!=l)
			{
				perror("write(outfd)");
				return;
			}

			// write(1, buf, l); // Echo to the local terminal
		}
	}
}

void
handle_connection(int connfd)
{
	char welcomebuf[PARTYSH_HDRSIZE] = {0};

	sprintf(welcomebuf, PARTYSH_MAGIC " " PARTYSH_VERSION " Just testing");
	if (write(connfd, welcomebuf, PARTYSH_HDRSIZE) != PARTYSH_HDRSIZE)
	{
		perror("send welcome");
		fprintf(stderr, "Cannot send the welcome string\n");
		exit(1);
	}

   	setup_terminal();
	localfd=dup(0);

	if (fork())
	   {
	   	/* Parent process */
		close(connfd);
		wait(NULL);
		printf("Session closed1\n");fflush(stdout);
		restore_terminal();
	   }
	else
	   {
		terminal_loop(connfd);
		close(connfd);
	   }

}

int
run_daemon(int port)
{
	struct sockaddr_in servaddr;

	if (port==0)
	{
		fprintf(stderr, "Please give a port (-p)\n");
		exit(1);
	}

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr))==-1)
	{
		perror("bind");
		exit(1);
	}

	while (1)
	{
	
	   if (!wait_connect())
	   {
	      perror("wait connection");
	      exit(1);
	   }

	   handle_connection(connfd);
	}

	close(listenfd);

	return 0;
}

void
pty_session(int connfd)
{
	mode_t mode;
	pid_t pid;
	int ptyfd, fd;
	FILE *f;
	char tmpl[32]="/tmp/osspartyshXXXXXX";

	mode = umask(077);
	if ((fd = mkstemp(tmpl)) == -1)
	{
		perror("mkstemp");
		exit(-1);
	}
	chmod(tmpl, 0700);
	umask(mode);

	if ((f = fdopen(fd, "w")) != NULL)
	{
		fprintf(f, "PS1=\"[SHARED]@\\h:\\w $ \"\n");
		fprintf(f, "export PS1\n");
		fclose(f);
	}
	else close(fd);

	if ((pid=forkpty(&ptyfd, NULL, NULL, NULL))==-1)
	{
		perror("forkpty");
		exit(1);
	}

	if (pid != 0)
	   {
	   	setup_terminal();
		sleep(1);
		unlink(tmpl);
		shell_loop(connfd, ptyfd, ptyfd);
	   	wait(NULL);
		restore_terminal();
	   }
	else
	   {
    		execlp("bash", "bash", "--rcfile", tmpl, NULL);
	   }
}

int
run_client(char *host, int port)
{

  struct sockaddr_in sa;
  struct hostent *he;
  char *ps1, *p;
  char welcomebuf[PARTYSH_HDRSIZE];

  int sockfd, connfd;

	if (host == NULL)
	{
		fprintf(stderr, "Please give a host (-h)\n");
		exit(1);
	}

	if (port==0)
	{
		fprintf(stderr, "Please give a port (-p)\n");
		exit(1);
	}

  if ((sockfd = socket (PF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("socket");
      return 0;
    }

  printf("Looking for the server\n");

  if ((he = gethostbyname (host)) == NULL)
    {
      perror("lookup");
      fprintf(stderr, "Cannot find the support host\n");
      return 0;
    }

  sa.sin_family = AF_INET;
  sa.sin_port = htons (port);

  printf ("Connecting to the support center at %s:%d\n", host, port);

  memcpy ((void *) &sa.sin_addr, *he->h_addr_list, he->h_length);
  if ((connfd = connect (sockfd, (void *) &sa, sizeof (sa))) == -1)
    {
      perror("connect");
      fprintf(stderr, "Cannot connect to the support center\n");
      return 0;
    }

    if (read(sockfd, welcomebuf, PARTYSH_HDRSIZE) != PARTYSH_HDRSIZE)
    {
    	perror("receive welcome");
	exit(1);
    }

    // printf("Welcome %s\n", welcomebuf);

    if (strncmp(welcomebuf, PARTYSH_MAGIC, strlen(PARTYSH_MAGIC))!=0)
    {
    	fprintf(stderr, "Didn't receive valid velcome string - abort\n");
	exit(1);
    }

    if ((ps1=getenv("PS1")) != NULL)
    {
    	p = ps1;

	while (*p)
	{
		if (*p == '#') *p= '$';
		if (*p == '>') *p= '<';
		p++;
	}
	//printf("PS1='%s'\n", ps1);

        setenv("PS1", ps1, 1);
    } else setenv("PS1", "ossremote> ", 1);

    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &off, sizeof(off));
    printf("*** Established connection - You are now sharing this session ***\n\n");fflush(stdout);
    printf("You may need to type ^L after running vi.\n");
    printf("\n");
    fflush(stdout);

#if 0
    close(0);
    dup(sockfd);
    close(1);
    dup(sockfd);
    close(2);
    dup(sockfd);

    printf("Now connected to the remote system - Hit ^C to disconnect\n");fflush(stdout);

    execlp("bash", "bash", "-i", NULL);
    //execlp("sh", "sh", NULL);
#else
#  if 1
    pty_session(sockfd);
#  else
    shell_loop(sockfd, 0, 1);
#  endif    
#endif
    close(sockfd);
    close(connfd);

	return 0;
}

int main(int argc, char *argv[])
{
	int i;
	int daemon_mode=0;
	int port=0;
	char *host=NULL;

	while ((i = getopt(argc, argv, "dp:h:")) != EOF)
	switch (i)
	{
	case 'd':
		daemon_mode=1;
		break;

	case 'p':
		port = atoi(optarg);
		break;

	case 'h':
		host=optarg;
		break;
	}

	if (daemon_mode)
	   exit(run_daemon(port));

	exit(run_client(host, port));
}
