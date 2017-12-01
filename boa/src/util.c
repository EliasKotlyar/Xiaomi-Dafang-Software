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

/* $Id: util.c,v 1.61.2.22 2005/02/22 14:11:29 jnelson Exp $ */

#include "boa.h"

static int date_to_tm(struct tm *file_gmt, const char *cmtime);

/* Don't need or want the trailing nul for these character arrays */
static const char month_tab[48] =
    "Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec ";
static const char day_tab[28] = "Sun,Mon,Tue,Wed,Thu,Fri,Sat,";

/*
 * Name: clean_pathname
 *
 * Description: Replaces unsafe/incorrect instances of:
 *  //[...] with /
 *  /./ with /
 *  /../ with / (technically not what we want, but browsers should deal
 *   with this, not servers)
 */

void clean_pathname(char *pathname)
{
    char *cleanpath, c;

    cleanpath = pathname;
    while ((c = *pathname++)) {
        if (c == '/') {
            while (1) {
                if (*pathname == '/')
                    pathname++;
                else if (*pathname == '.' && *(pathname + 1) == '/')
                    pathname += 2;
                else if (*pathname == '.' && *(pathname + 1) == '.' &&
                         *(pathname + 2) == '/') {
                    pathname += 3;
                } else
                    break;
            }
            c = '/';
        }
        *cleanpath++ = c;
    }

    *cleanpath = '\0';
}

#if 0
char *new_clean_pathname(char *pathname)
{
    static char *segment[50];
    int seg_count = 0;
    char *a, *cleanpath, c;

    a = pathname;
    segment[seg_count] = pathname;
    cleanpath = pathname;

    while ((c = *pathname++)) {
        if (c == '/') {         /* /?? */
            while (1) {         /* everything in this loop gets eliminated */
                if (*pathname == '/') /* // */
                    pathname++;
                else if (*pathname == '.') { /* /. */
                    if (*(pathname + 1) == '/') /* /./ */
                        pathname += 2;
                    else if (*(pathname + 1) == '\0') /* /.$ */
                        pathname += 1;
                    else if (*(pathname + 1) == '.') { /* /.. */
                        if (*(pathname + 2) == '/') /* /../ */
                            pathname += 3;
                        else if (*(pathname + 1) == '\0') /* /..$ */
                            pathname += 2;
                        /*
                           cleanpath goes *back* one
                         */
                        if (seg_count)
                            cleanpath = segment[--seg_count];
                        else {
                            *a = '\0';
                            return a;
                        }
                    } else {    /* /.blah */
                        break;
                    }
                } else {        /* we have /something */
                    break;
                }
            }
            if (seg_count > 49) { /* we can store in spots 0...49 */
                *a = '\0';
                return a;
            }
            *cleanpath = '/';
            segment[seg_count++] = cleanpath++;
        } else
            *cleanpath++ = c;
    }

    *cleanpath = '\0';
    return a;
}
#endif

/*
 * Name: get_commonlog_time
 *
 * Description: Returns the current time in common log format in a static
 * char buffer.
 *
 * commonlog time is exactly 25 characters long
 * because this is only used in logging, we add " [" before and "] " after
 * making 29 characters
 * "[27/Feb/1998:20:20:04 +0000] "
 *
 * Contrast with rfc822 time:
 * "Sun, 06 Nov 1994 08:49:37 GMT"
 *
 * Altered 10 Jan 2000 by Jon Nelson ala Drew Streib for non UTC logging
 *
 */

char *get_commonlog_time(void)
{
    struct tm *t;
    char *p;
    unsigned int a;
    static char buf[30];
    int time_offset;

    if (use_localtime) {
        t = localtime(&current_time);
        time_offset = TIMEZONE_OFFSET(t);
    } else {
        t = gmtime(&current_time);
        time_offset = 0;
    }

    p = buf + 29;
    *p-- = '\0';
    *p-- = ' ';
    *p-- = ']';
    a = abs(time_offset / 60);
    *p-- = '0' + a % 10;
    a /= 10;
    *p-- = '0' + a % 6;
    a /= 6;
    *p-- = '0' + a % 10;
    *p-- = '0' + a / 10;
    *p-- = (time_offset >= 0) ? '+' : '-';
    *p-- = ' ';

    a = t->tm_sec;
    *p-- = '0' + a % 10;
    *p-- = '0' + a / 10;
    *p-- = ':';
    a = t->tm_min;
    *p-- = '0' + a % 10;
    *p-- = '0' + a / 10;
    *p-- = ':';
    a = t->tm_hour;
    *p-- = '0' + a % 10;
    *p-- = '0' + a / 10;
    *p-- = ':';
    a = 1900 + t->tm_year;
    while (a) {
        *p-- = '0' + a % 10;
        a /= 10;
    }
    /* p points to an unused spot */
    *p-- = '/';
    p -= 2;
    memcpy(p--, month_tab + 4 * (t->tm_mon), 3);
    *p-- = '/';
    a = t->tm_mday;
    *p-- = '0' + a % 10;
    *p-- = '0' + a / 10;
    *p = '[';
    return p;                   /* should be same as returning buf */
}

/*
 * Name: month2int
 *
 * Description: Turns a three letter month into a 0-11 int
 *
 * Note: This function is from wn-v1.07 -- it's clever and fast
 */

int month2int(const char *monthname)
{
    switch (*monthname) {
    case 'A':
        return (*++monthname == 'p' ? 3 : 7);
    case 'D':
        return (11);
    case 'F':
        return (1);
    case 'J':
        if (*++monthname == 'a')
            return 0;
        return (*++monthname == 'n' ? 5 : 6);
    case 'M':
        return (*(monthname + 2) == 'r' ? 2 : 4);
    case 'N':
        return (10);
    case 'O':
        return (9);
    case 'S':
        return (8);
    default:
        return (-1);
    }
}

/*
 * Name: date_to_seconds
 * Description:
 *

 Sun, 06 Nov 1994 08:49:37 GMT    ; RFC 822, updated by RFC 1123
 Sunday, 06-Nov-94 08:49:37 GMT   ; RFC 850, obsoleted by RFC 1036
 Sun Nov  6 08:49:37 1994         ; ANSI C's asctime() format
 31 September 2000 23:59:59 GMT   ; non-standard

 * RETURN VALUES:
 *  -1 for error, 0 for success
 */

static int date_to_tm(struct tm *parsed_gmt, const char *cmtime)
{
    char monthname[10 + 1];
    const char *cmtime_start = cmtime;
    int day, year, hour, minute, second;

    /* we don't use the weekday portion, so skip over it */
    while (*cmtime != ' ' && *cmtime != '\0')
        ++cmtime;

    if (*cmtime != ' ')
        return -1;
    /* the pre-space in the third scanf skips whitespace for the string */
    if (sscanf(cmtime, "%d %3s %d %d:%d:%d GMT", /* RFC 1123 */
               &day, monthname, &year, &hour, &minute, &second) == 6) {
    } else if (sscanf(cmtime, "%d-%3s-%d %d:%d:%d GMT", /* RFC 1036 */
                      &day, monthname, &year, &hour, &minute, &second) == 6) {
    } else if (sscanf(cmtime, "%3s %d %d:%d:%d %d", /* asctime() format */
                      monthname, &day, &hour, &minute, &second, &year) == 6) {
        /*
         *  allow this non-standard date format: 31 September 2000 23:59:59 GMT
         * NOTE: Use 'cmtime_start' instead of 'cmtime' here, because the date *starts*
         *       with the day, versus a throwaway item
         */
    } else if (sscanf(cmtime_start, "%d %10s %d %d:%d:%d GMT",
                      &day, monthname, &year, &hour, &minute, &second) == 6) {
    } else {
        log_error_time();
        fprintf(stderr,
                "Error in %s, line %d: Unable to sscanf \"%s\"\n",
                __FILE__, __LINE__, cmtime);
        return -1;              /* error */
    }

    if (year < 70)
        year += 100;
    if (year > 1900)
        year -= 1900;

    parsed_gmt->tm_sec = second;
    parsed_gmt->tm_min = minute;
    parsed_gmt->tm_hour = hour;
    parsed_gmt->tm_mday = day;
    parsed_gmt->tm_mon = month2int(monthname);
    parsed_gmt->tm_year = year;
    parsed_gmt->tm_wday = 0;
    parsed_gmt->tm_yday = 0;
    parsed_gmt->tm_isdst = 0;

    if (parsed_gmt->tm_mon == -1) {
        log_error_time();
        fprintf(stderr, "Invalid month name: \"%s\"\n", monthname);
        return -1;
    }

    /* adapted from Squid 2.5 */
    if (parsed_gmt->tm_sec < 0 || parsed_gmt->tm_sec > 59)
        return -1;
    if (parsed_gmt->tm_min < 0 || parsed_gmt->tm_min > 59)
        return -1;
    if (parsed_gmt->tm_hour < 0 || parsed_gmt->tm_hour > 23)
        return -1;
    if (parsed_gmt->tm_mday < 1 || parsed_gmt->tm_mday > 31)
        return -1;
    if (parsed_gmt->tm_mon < 0 || parsed_gmt->tm_mon > 11)
        return -1;
    if (parsed_gmt->tm_year < 70 || parsed_gmt->tm_year > 120)
        return -1;

    return 0;
}

/*
 * Name: modified_since
 * Description: Decides whether a file's mtime is newer than the
 * If-Modified-Since header of a request.
 *

 * RETURN VALUES:
 *  0: File has not been modified since specified time.
 *  >0: File has been (and value is the converted_time)
 * -1: Error!
 */

int modified_since(time_t * mtime, const char *if_modified_since)
{
    struct tm *file_gmt;
    struct tm parsed_gmt;
    int comp;

    if (date_to_tm(&parsed_gmt, if_modified_since) != 0) {
        return -1;
    }

    file_gmt = gmtime(mtime);

    /* Go through from years to seconds -- if they are ever unequal,
       we know which one is newer and can return */
    if ((comp = file_gmt->tm_year - parsed_gmt.tm_year))
        return (comp > 0);
    if ((comp = file_gmt->tm_mon - parsed_gmt.tm_mon))
        return (comp > 0);
    if ((comp = file_gmt->tm_mday - parsed_gmt.tm_mday))
        return (comp > 0);
    if ((comp = file_gmt->tm_hour - parsed_gmt.tm_hour))
        return (comp > 0);
    if ((comp = file_gmt->tm_min - parsed_gmt.tm_min))
        return (comp > 0);
    if ((comp = file_gmt->tm_sec - parsed_gmt.tm_sec))
        return (comp > 0);

    /* this person must really be into the latest/greatest */
    return 0;
}

/*
 * Name: to_upper
 *
 * Description: Turns a string into all upper case (for HTTP_ header forming)
 * AND changes - into _
 */

char *to_upper(char *str)
{
    char *start = str;

    while (*str) {
        if (*str == '-')
            *str = '_';
        else
            *str = toupper(*str);
        str++;
    }

    return start;
}

/*
 * Name: unescape_uri
 *
 * Description: Decodes a uri, changing %xx encodings with the actual
 * character.  The query_string should already be gone.
 *
 * Return values:
 *  1: success
 *  0: illegal string
 */

int unescape_uri(char *uri, char **query_string)
{
    char c, d;
    char *uri_old;

    uri_old = uri;

    while ((c = *uri_old)) {
        if (c == '%') {
            uri_old++;
            if ((c = *uri_old++) && (d = *uri_old++)) {
                *uri = HEX_TO_DECIMAL(c, d);
                if (*uri < 32 || *uri > 126) {
                    /* control chars in URI */
                    *uri = '\0';
                    return 0;
                }
            } else {
                *uri = '\0';
                return 0;
            }
            ++uri;
        } else if (c == '?') {  /* query string */
            if (query_string)
                *query_string = ++uri_old;
            /* stop here */
            *uri = '\0';
            return (1);
        } else if (c == '#') {  /* fragment */
            /* legal part of URL, but we do *not* care.
             * However, we still have to look for the query string */
            if (query_string) {
                ++uri_old;
                while ((c = *uri_old)) {
                    if (c == '?') {
                        *query_string = ++uri_old;
                        break;
                    }
                    ++uri_old;
                }
            }
            break;
        } else {
            *uri++ = c;
            uri_old++;
        }
    }

    *uri = '\0';
    return 1;
}

/* rfc822 (1123) time is exactly 29 characters long
 * "Sun, 06 Nov 1994 08:49:37 GMT"
 */

void rfc822_time_buf(char *buf, time_t s)
{
    struct tm *t;
    char *p;
    unsigned int a;

    if (!s) {
        t = gmtime(&current_time);
    } else
        t = gmtime(&s);

    p = buf + 28;
    /* p points to the last char in the buf */

    p -= 3;
    /* p points to where the ' ' will go */
    memcpy(p--, " GMT", 4);

    a = t->tm_sec;
    *p-- = '0' + a % 10;
    *p-- = '0' + a / 10;
    *p-- = ':';
    a = t->tm_min;
    *p-- = '0' + a % 10;
    *p-- = '0' + a / 10;
    *p-- = ':';
    a = t->tm_hour;
    *p-- = '0' + a % 10;
    *p-- = '0' + a / 10;
    *p-- = ' ';
    a = 1900 + t->tm_year;
    while (a) {
        *p-- = '0' + a % 10;
        a /= 10;
    }
    /* p points to an unused spot to where the space will go */
    p -= 3;
    /* p points to where the first char of the month will go */
    memcpy(p--, month_tab + 4 * (t->tm_mon), 4);
    *p-- = ' ';
    a = t->tm_mday;
    *p-- = '0' + a % 10;
    *p-- = '0' + a / 10;
    *p-- = ' ';
    p -= 3;
    memcpy(p, day_tab + t->tm_wday * 4, 4);
}

char *simple_itoa(unsigned int i)
{
    /* 21 digits plus null terminator, good for 64-bit or smaller ints
     * for bigger ints, use a bigger buffer!
     *
     * 4294967295 is, incidentally, MAX_UINT (on 32bit systems at this time)
     * and is 10 bytes long
     */
    static char local[22];
    char *p = &local[21];
    *p = '\0';
    do {
        *--p = '0' + i % 10;
        i /= 10;
    } while (i != 0);
    return p;
}

/* I don't "do" negative conversions
 * Therefore, -1 indicates error
 */

int boa_atoi(const char *s)
{
    int retval;
    char *reconv;

    if (!isdigit(*s))
        return -1;

    retval = atoi(s);
    if (retval < 0)
        return -1;

    reconv = simple_itoa((unsigned int) retval);
    if (memcmp(s, reconv, strlen(s)) != 0) {
        return -1;
    }
    return retval;
}

int create_temporary_file(short want_unlink, char *storage, unsigned int size)
{
    static char boa_tempfile[MAX_PATH_LENGTH + 1];
    int fd;

    snprintf(boa_tempfile, MAX_PATH_LENGTH, "%s/boa-temp.XXXXXX", tempdir);

    /* open temp file */
    fd = mkstemp(boa_tempfile);
    if (fd == -1) {
        log_error_time();
        perror("mkstemp");
        return 0;
    }

    if (storage != NULL) {
        unsigned int len = strlen(boa_tempfile);

        if (len < size) {
            memcpy(storage, boa_tempfile, len + 1);
        } else {
            close(fd);
            fd = 0;
            log_error_time();
            fprintf(stderr, "not enough memory for memcpy in storage\n");
            want_unlink = 1;
        }
    }

    if (want_unlink) {
        if (unlink(boa_tempfile) == -1) {
            close(fd);
            fd = 0;
            log_error_time();
            fprintf(stderr, "unlink temp file\n");
        }
    }

    return (fd);
}

int real_set_block_fd(int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFL);
    if (flags == -1)
        return -1;

    flags &= ~NOBLOCK;
    flags = fcntl(fd, F_SETFL, flags);
    return flags;
}

int real_set_nonblock_fd(int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFL);
    if (flags == -1)
        return -1;

    flags |= NOBLOCK;
    flags = fcntl(fd, F_SETFL, flags);
    return flags;
}

/* Quoting from rfc1034:

<domain> ::= <subdomain> | " "

<subdomain> ::= <label> | <subdomain> "." <label>

<label> ::= <letter> [ [ <ldh-str> ] <let-dig> ]

<ldh-str> ::= <let-dig-hyp> | <let-dig-hyp> <ldh-str>

<let-dig-hyp> ::= <let-dig> | "-"

<let-dig> ::= <letter> | <digit>

<letter> ::= any one of the 52 alphabetic characters A through Z in
upper case and a through z in lower case

<digit> ::= any one of the ten digits 0 through 9

and

The labels must follow the rules for ARPANET host names.  They must
start with a letter, end with a letter or digit, and have as interior
characters only letters, digits, and hyphen.  There are also some
restrictions on the length.  Labels must be 63 characters or less.

*/

int check_host(const char *r)
{
    /* a hostname can only consist of
     * chars and numbers, and sep. by only
     * one period.
     * It may not end with a period, and must
     * not start with a number.
     *
     * >0: correct
     * -1: error
     *  0: not returned
     *
     */
    const char *c;
    short period_ok = 0;
    short len = 0;

    c = r;
    if (c == NULL) {
        return -1;
    }

    /* must start with a letter or number */
    if (!isalnum(*c))
        return -1;

    if (strlen(c) > 63)
        return -1;

    len = 1;
    while (*(++c) != '\0') {
        /* interior letters may be alphanumeric, '-', or '.' */
        /* '.' may not follow '.' */
        if (isalnum(*c) || *c == '-')
            period_ok = 1;
        else if (*c == '.' && period_ok)
            period_ok = 0;
        else
            return -1;
        ++len;
    }
    /* c points to '\0' */
    --c;
    /* must end with a letter or digit */
    if (!isalnum(*c))
        return -1;
    return len;
}

void strlower(char *s)
{
    while (*s != '\0') {
        *s = tolower(*s);
        ++s;
    }
}

#ifndef DISABLE_DEBUG
struct dbg {
    int level;
    const char *mesg;
};

static struct dbg debug_level_table[] = {
    {DEBUG_ALIAS, "Alias"},
    {DEBUG_CGI_OUTPUT, "CGI Output"},
    {DEBUG_CGI_INPUT, "CGI Input"},
    {DEBUG_CGI_ENV, "CGI Environment"},
    {DEBUG_HEADER_READ, "Header Read State"},
    {DEBUG_PIPELINE, "Pipeline"},
    {DEBUG_PLUGIN_ERRORS, "Plugin Error"},
    {DEBUG_RANGE, "Range related"},
    {DEBUG_CONFIG, "Configuration"},
    {DEBUG_BUFFER_IO, "Buffer I/O"},
    {DEBUG_BODY_READ, "Body Read State"},
    {DEBUG_MMAP_CACHE, "mmap Cache"},
    {DEBUG_REQUEST, "Generic Request"},
    {DEBUG_HASH, "hash table"}
};


void print_debug_usage(void)
{
    struct dbg *p;

    fprintf(stderr,
            "  To calculate the debug level, logically 'or'\n"
            "  some of the following values together to get a debug level:\n");
    for (p = debug_level_table;
         p <
         debug_level_table +
         (sizeof (debug_level_table) / sizeof (struct dbg)); p++) {
        fprintf(stderr, "\t%d:\t%s\n", p->level, p->mesg);
    }
    fprintf(stderr, "\n");
}

void parse_debug(char *foo)
{
    int i;
    struct dbg *p;

    if (!foo)
        return;

    log_error_time();
    fprintf(stderr, "Before parse_debug, debug_level is: %d\n",
            debug_level);
    if (foo[0] == '-') {
        i = boa_atoi(foo + 1);
        if (i == -1) {
            /* error */
            fprintf(stderr, "Invalid level specified.\n");
            exit(EXIT_FAILURE);
        }
        i = -i;
    } else {
        i = boa_atoi(foo);
        if (i == -1) {
            /* error */
            fprintf(stderr, "Invalid level specified.\n");
            exit(EXIT_FAILURE);
        }
    }
    for (p = debug_level_table;
         p <
         debug_level_table +
         (sizeof (debug_level_table) / sizeof (struct dbg)); p++) {
        if (i > 0) {
            if (i & p->level) {
                log_error_time();
                fprintf(stderr, "Enabling %s debug level.\n",
                        p->mesg);
                debug_level |= p->level;
            }
        } else {
            if (-i & p->level) {
                log_error_time();
                fprintf(stderr, "Disabling %s debug level.\n",
                        p->mesg);
                debug_level &= ~(p->level);
            }
        }
    }
    log_error_time();
    fprintf(stderr, "After parse_debug, debug_level is: %d\n",
            debug_level);
}
#endif
