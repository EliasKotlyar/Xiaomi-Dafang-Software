/*
 *  Boa, an http server
 *  Copyright (C) 1999-2005 Larry Doolittle <ldoolitt@boa.org>
 *  Copyright (C) 2000-2005 Jon Nelson <jnelson@boa.org>
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

/* $Id: config.c,v 1.31.2.30 2005/02/22 14:11:29 jnelson Exp $*/

#include "boa.h"
#include "access.h"
#include <sys/resource.h>

const char *config_file_name;

unsigned int server_port;
uid_t server_uid;
gid_t server_gid;
char *server_root;
char *server_name;
char *server_admin;
char *server_ip;
int virtualhost;
char *vhost_root;
const char *default_vhost;
unsigned max_connections;

char *document_root;
char *user_dir;
char *directory_index;
char *default_type;
char *default_charset;
char *dirmaker;
char *cachedir;

const char *tempdir;

unsigned int cgi_umask = 027;

char *pid_file;
char *cgi_path;
int single_post_limit = SINGLE_POST_LIMIT_DEFAULT;
int conceal_server_identity = 0;

int ka_timeout;
unsigned int default_timeout;
unsigned int ka_max;

/* These came from log.c */
char *error_log_name;
char *access_log_name;
char *cgi_log_name;

int use_localtime;

#ifdef USE_SETRLIMIT
extern int cgi_rlimit_cpu;      /* boa.c */
extern int cgi_rlimit_data;     /* boa.c */
extern int cgi_nice;            /* boa.c */
#endif

/* These are new */
static void c_add_cgi_env(char *v1, char *v2, void *table_ptr);
static void c_set_user(char *v1, char *v2, void *t);
static void c_set_group(char *v1, char *v2, void *t);
static void c_set_string(char *v1, char *v2, void *t);
static void c_set_int(char *v1, char *v2, void *t);
static void c_set_unity(char *v1, char *v2, void *t);
static void c_add_mime_types_file(char *v1, char *v2, void *t);
static void c_add_mime_type(char *v1, char *v2, void *t);
static void c_add_alias(char *v1, char *v2, void *t);
static void c_add_access(char *v1, char *v2, void *t);

struct ccommand {
    const char *name;
    const int type;
    void (*action) (char *, char *, void *);
    void *object;
};

typedef struct ccommand Command;

static void apply_command(Command * p, char *args);
static void trim(char *s);
static void parse(FILE * f);

/* Fakery to keep the value passed to action() a void *,
   see usage in table and c_add_alias() below */
static enum ALIAS script_number = SCRIPTALIAS;
static enum ALIAS redirect_number = REDIRECT;
static enum ALIAS alias_number = ALIAS;
static int access_allow_number = ACCESS_ALLOW;
static int access_deny_number = ACCESS_DENY;
static uid_t current_uid = 0;

/* Help keep the table below compact */
#define STMT_NO_ARGS 1
#define STMT_ONE_ARG 2
#define STMT_TWO_ARGS 3

#define S0A STMT_NO_ARGS
#define S1A STMT_ONE_ARG
#define S2A STMT_TWO_ARGS

/* function prototype */
Command *lookup_keyword(char *c);

struct ccommand clist[] = {
    {"Port", S1A, c_set_int, &server_port},
    {"Listen", S1A, c_set_string, &server_ip},
    {"BackLog", S1A, c_set_int, &backlog},
    {"User", S1A, c_set_user, NULL},
    {"Group", S1A, c_set_group, NULL},
    {"ServerAdmin", S1A, c_set_string, &server_admin},
    {"ServerRoot", S1A, c_set_string, &server_root},
    {"UseLocaltime", S0A, c_set_unity, &use_localtime},
    {"ErrorLog", S1A, c_set_string, &error_log_name},
    {"AccessLog", S1A, c_set_string, &access_log_name},
    {"CgiLog", S1A, c_set_string, &cgi_log_name}, /* compatibility with CGILog */
    {"CGILog", S1A, c_set_string, &cgi_log_name},
    {"VerboseCGILogs", S0A, c_set_unity, &verbose_cgi_logs},
    {"ServerName", S1A, c_set_string, &server_name},
    {"VirtualHost", S0A, c_set_unity, &virtualhost},
    {"VHostRoot", S1A, c_set_string, &vhost_root},
    {"DefaultVHost", S1A, c_set_string, &default_vhost},
    {"DocumentRoot", S1A, c_set_string, &document_root},
    {"UserDir", S1A, c_set_string, &user_dir},
    {"DirectoryIndex", S1A, c_set_string, &directory_index},
    {"DirectoryMaker", S1A, c_set_string, &dirmaker},
    {"DirectoryCache", S1A, c_set_string, &cachedir},
    {"PidFile", S1A, c_set_string, &pid_file},
    {"KeepAliveMax", S1A, c_set_int, &ka_max},
    {"KeepAliveTimeout", S1A, c_set_int, &ka_timeout},
    {"MimeTypes", S1A, c_add_mime_types_file, NULL},
    {"DefaultType", S1A, c_set_string, &default_type},
    {"DefaultCharset", S1A, c_set_string, &default_charset},
    {"AddType", S2A, c_add_mime_type, NULL},
    {"ScriptAlias", S2A, c_add_alias, &script_number},
    {"Redirect", S2A, c_add_alias, &redirect_number},
    {"Alias", S2A, c_add_alias, &alias_number},
    {"SinglePostLimit", S1A, c_set_int, &single_post_limit},
    {"CGIPath", S1A, c_set_string, &cgi_path},
    {"CGIumask", S1A, c_set_int, &cgi_umask},
    {"MaxConnections", S1A, c_set_int, &max_connections},
    {"ConcealServerIdentity", S0A, c_set_unity, &conceal_server_identity},
    {"Allow", S1A, c_add_access, &access_allow_number},
    {"Deny", S1A, c_add_access, &access_deny_number},
#ifdef USE_SETRLIMIT
    {"CGIRlimitCpu", S2A, c_set_int, &cgi_rlimit_cpu},
    {"CGIRlimitData", S2A, c_set_int, &cgi_rlimit_data},
    {"CGINice", S2A, c_set_int, &cgi_nice},
#endif
    {"CGIEnv", S2A, c_add_cgi_env, NULL},
};

static void c_add_cgi_env(char *v1, char *v2, void *t)
{
    add_to_common_env(v1, v2);
}

static void c_set_user(char *v1, char *v2, void *t)
{
    struct passwd *passwdbuf;
    char *endptr;
    int i;

    DEBUG(DEBUG_CONFIG) {
        log_error_time();
        printf("User %s = ", v1);
    }
    i = strtol(v1, &endptr, 0);
    if (*v1 != '\0' && *endptr == '\0') {
        server_uid = i;
    } else {
        passwdbuf = getpwnam(v1);
        if (!passwdbuf) {
            log_error_time();
            fprintf(stderr, "No such user: %s\n", v1);
            if (current_uid)
                return;
            exit(EXIT_FAILURE);
        }
        server_uid = passwdbuf->pw_uid;
    }
    DEBUG(DEBUG_CONFIG) {
        printf("%d\n", server_uid);
    }
}

static void c_set_group(char *v1, char *v2, void *t)
{
    struct group *groupbuf;
    char *endptr;
    int i;
    DEBUG(DEBUG_CONFIG) {
        log_error_time();
        printf("Group %s = ", v1);
    }
    i = strtol(v1, &endptr, 0);
    if (*v1 != '\0' && *endptr == '\0') {
        server_gid = i;
    } else {
        groupbuf = getgrnam(v1);
        if (!groupbuf) {
            log_error_time();
            fprintf(stderr, "No such group: %s\n", v1);
            if (current_uid)
                return;
            exit(EXIT_FAILURE);
        }
        server_gid = groupbuf->gr_gid;
    }
    DEBUG(DEBUG_CONFIG) {
        printf("%d\n", server_gid);
    }
}

static void c_set_string(char *v1, char *v2, void *t)
{
    DEBUG(DEBUG_CONFIG) {
        log_error_time();
        printf("Setting pointer %p to string %s ..", t, v1);
    }
    if (t) {
        if (*(char **) t != NULL)
            free(*(char **) t);
        *(char **) t = strdup(v1);
        if (!*(char **) t) {
            DIE("Unable to strdup in c_set_string");
        }
        DEBUG(DEBUG_CONFIG) {
            printf("done.\n");
        }
    } else {
        DEBUG(DEBUG_CONFIG) {
            printf("skipped.\n");
        }
    }
}

static void c_set_int(char *v1, char *v2, void *t)
{
    char *endptr;
    int i;
    DEBUG(DEBUG_CONFIG) {
        log_error_time();
        printf("Setting pointer %p to integer string %s ..", t, v1);
    }
    if (t) {
        i = strtol(v1, &endptr, 0); /* Automatic base 10/16/8 switching */
        if (*v1 != '\0' && *endptr == '\0') {
            *(int *) t = i;
            DEBUG(DEBUG_CONFIG) {
                printf(" Integer converted as %d, done\n", i);
            }
        } else {
            /* XXX should tell line number to user */
            fprintf(stderr, "Error: %s found where integer expected\n",
                    v1);
        }
    } else {
        DEBUG(DEBUG_CONFIG) {
            printf("skipped.\n");
        }
    }
}

static void c_set_unity(char *v1, char *v2, void *t)
{
    DEBUG(DEBUG_CONFIG) {
        log_error_time();
        printf("Setting pointer %p to unity\n", t);
    }
    if (t)
        *(int *) t = 1;
}

static void c_add_mime_type(char *v1, char *v2, void *t)
{
    add_mime_type(v2, v1);
}

static void c_add_mime_types_file(char *v1, char *v2, void *t)
{
    /* v1 is the file */
    FILE *f;
    char buf[256], *p;
    char *type, *extension, *c2;
    int len;

    f = fopen(v1, "r");
    if (f == NULL)
        DIE("Can't open mime-types file");
    while (fgets(buf, 255, f) != NULL) {
        if (buf[0] == '\0' || buf[0] == '#' || buf[0] == '\n')
            continue;

        c2 = strchr(buf, '\n');
        if (c2)
            *c2 = '\0';

        len = strcspn(buf, "\t ");
        if (!len) {
            DEBUG(DEBUG_CONFIG) {
                log_error_time();
                fprintf(stderr,
                        "Improperly formatted mime-type: \"%s\"\n", buf);
            }
            continue;
        }

        buf[len] = '\0';
        type = buf;
        for (p = buf + len + 1; *p; ++p) {
            if (isalnum(*p))
                break;
        }

        for (len = strcspn(p, "\t "); len; len = strcspn(p, "\t ")) {
            p[len] = '\0';
            extension = p;
            add_mime_type(extension, type);
            /* blah blah */
            for (p = p + len + 1; *p; ++p) {
                if (isalnum(*p))
                    break;
            }
        }
    }
    fclose(f);
}



static void c_add_alias(char *v1, char *v2, void *t)
{
    DEBUG(DEBUG_CONFIG) {
        log_error_time();
        printf("Calling add_alias with args \"%s\", \"%s\", and %d\n",
               v1, v2, *(int *) t);
    }
    add_alias(v1, v2, *(enum ALIAS *) t);
}

static void c_add_access(char *v1, char *v2, void *t)
{
#ifdef ACCESS_CONTROL
    access_add(v1, *(int *) t);
#else
    log_error_time();
    fprintf(stderr,
            "This version of Boa doesn't support access controls.\n"
            "Please recompile with --enable-access-control.\n");
#endif                          /* ACCESS_CONTROL */
}

struct ccommand *lookup_keyword(char *c)
{
    struct ccommand *p;
    DEBUG(DEBUG_CONFIG) {
        log_error_time();
        printf("Checking string '%s' against keyword list\n", c);
    }
    for (p = clist;
         p < clist + (sizeof (clist) / sizeof (struct ccommand)); p++) {
        if (strcasecmp(c, p->name) == 0)
            return p;
    }
    return NULL;
}

static void apply_command(Command * p, char *args)
{
    char *second;

    switch (p->type) {
    case STMT_NO_ARGS:
        (p->action) (NULL, NULL, p->object);
        break;
    case STMT_ONE_ARG:
        (p->action) (args, NULL, p->object);
        break;
    case STMT_TWO_ARGS:
        /* FIXME: if no 2nd arg exists, we use NULL. Desirable? */
        while (isspace(*args))
            ++args;
        if (*args == '\0') {
            log_error_time();
            fprintf(stderr, "expected at least 1 arg! (%s)\n", p->name);
            exit(EXIT_FAILURE);
        }

        second = args;
        while (!isspace(*second))
            ++second;
        if (*second == '\0') {
            /* nuthin but spaces */
            second = NULL;
        } else {
            *second = '\0';
            ++second;
            while (isspace(*second))
                ++second;
            if (*second == '\0') {
                second = NULL;
            }
        }

        (p->action) (args, second, p->object);
        break;
    default:
        exit(EXIT_FAILURE);
    }
}

static void trim(char *s)
{
    char *c = s + strlen(s) - 1;

    while (isspace(*c) && c > s) {
        *c = '\0';
        --c;
    }
}

static void parse(FILE * f)
{
    char buf[1025], *c;
    Command *p;
    int line = 0;

    current_uid = getuid();
    while (fgets(buf, 1024, f) != NULL) {
        ++line;
        if (buf[0] == '\0' || buf[0] == '#' || buf[0] == '\n')
            continue;

        /* kill the linefeed and any trailing whitespace */
        trim(buf);
        if (buf[0] == '\0')
            continue;

        /* look for multiple arguments */
        c = buf;
        while (!isspace(*c))
            ++c;

        if (*c == '\0') {
            /* no args */
            c = NULL;
        } else {
            /* one or more args */
            *c = '\0';
            ++c;
        }

        p = lookup_keyword(buf);

        if (!p) {
            log_error_time();
            fprintf(stderr, "Line %d: Did not find keyword \"%s\"\n", line,
                    buf);
            exit(EXIT_FAILURE);
        } else {
            DEBUG(DEBUG_CONFIG) {
                log_error_time();
                fprintf(stderr,
                        "Found keyword %s in \"%s\" (%s)!\n",
                        p->name, buf, c);
            }
            apply_command(p, c);
        }
    }
}

/*
 * Name: read_config_files
 *
 * Description: Reads config files, then makes sure that
 * all required variables were set properly.
 */
void read_config_files(void)
{
    FILE *config;

    current_uid = getuid();

    if (!config_file_name) {
        config_file_name = DEFAULT_CONFIG_FILE;
    }
#ifdef ACCESS_CONTROL
    access_init();
#endif                          /* ACCESS_CONTROL */

    config = fopen(config_file_name, "r");
    if (!config) {
        fputs("Could not open boa.conf for reading.\n", stderr);
        exit(EXIT_FAILURE);
    }
    parse(config);
    fclose(config);

    if (!server_name) {
        struct hostent *he;
        char temp_name[100];

        if (gethostname(temp_name, 100) == -1) {
            perror("gethostname:");
            exit(EXIT_FAILURE);
        }

        he = gethostbyname(temp_name);
        if (he == NULL) {
            perror("gethostbyname:");
            exit(EXIT_FAILURE);
        }

        server_name = strdup(he->h_name);
        if (server_name == NULL) {
            perror("strdup:");
            exit(EXIT_FAILURE);
        }
    }
    tempdir = getenv("TMP");
    if (tempdir == NULL)
        tempdir = "/tmp";

    if (single_post_limit < 0) {
        fprintf(stderr, "Invalid value for single_post_limit: %d\n",
                single_post_limit);
        exit(EXIT_FAILURE);
    }

    if (vhost_root && virtualhost) {
        fprintf(stderr, "Both VHostRoot and VirtualHost were enabled, and "
                "they are mutually exclusive.\n");
        exit(EXIT_FAILURE);
    }

    if (vhost_root && document_root) {
        fprintf(stderr,
                "Both VHostRoot and DocumentRoot were enabled, and "
                "they are mutually exclusive.\n");
        exit(EXIT_FAILURE);
    }

    if (!default_vhost) {
        default_vhost = DEFAULT_VHOST;
    }

#ifdef USE_SETRLIMIT
    if (cgi_rlimit_cpu < 0)
        cgi_rlimit_cpu = 0;

    if (cgi_rlimit_data < 0)
        cgi_rlimit_data = 0;

    if (cgi_nice < 0)
        cgi_nice = 0;
#endif

    if (max_connections < 1) {
        struct rlimit rl;
        int c;

        /* has not been set explicitly */
        c = getrlimit(RLIMIT_NOFILE, &rl);
        if (c < 0) {
            DIE("getrlimit");
        }
        max_connections = rl.rlim_cur;
    }
    if (max_connections > FD_SETSIZE - 20)
        max_connections = FD_SETSIZE - 20;

    if (ka_timeout < 0) ka_timeout=0;  /* not worth a message */
    /* save some time */
    default_timeout = (ka_timeout ? ka_timeout : REQUEST_TIMEOUT);
#ifdef HAVE_POLL
    default_timeout *= 1000;
#endif

    if (default_type == NULL) {
        DIE("DefaultType *must* be set!");
    }
}
