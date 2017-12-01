/*
 *  Copyright (C) 1997-2005 Jon Nelson <jnelson@boa.org>
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

/* $Id: index_dir.c,v 1.32.2.7 2005/02/22 03:00:24 jnelson Exp $*/

#include <stdio.h>
#include <sys/stat.h>
#include <limits.h>             /* for PATH_MAX */
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "compat.h"

#define MAX_FILE_LENGTH                         MAXNAMLEN
#define MAX_PATH_LENGTH                         PATH_MAX

#define INT_TO_HEX(x) \
  ((((x)-10)>=0)?('A'+((x)-10)):('0'+(x)))

#include "escape.h"

char *html_escape_string(const char *inp, char *dest,
                         const unsigned int len);
char *http_escape_string(const char *inp, char *buf,
                         const unsigned int len);

int select_files(const struct dirent *d);
int index_directory(char *dir, char *title);
void send_error(int error);

/*
 * Name: html_escape_string
 */
char *html_escape_string(const char *inp, char *dest,
                         const unsigned int len)
{
    int max;
    char *buf;
    unsigned char c;

    max = len * 6;

    if (dest == NULL && max)
        dest = malloc(sizeof (unsigned char) * (max + 1));

    if (dest == NULL)
        return NULL;

    buf = dest;
    while ((c = *inp++)) {
        switch (c) {
        case '>':
            *dest++ = '&';
            *dest++ = 'g';
            *dest++ = 't';
            *dest++ = ';';
            break;
        case '<':
            *dest++ = '&';
            *dest++ = 'l';
            *dest++ = 't';
            *dest++ = ';';
            break;
        case '&':
            *dest++ = '&';
            *dest++ = 'a';
            *dest++ = 'm';
            *dest++ = 'p';
            *dest++ = ';';
            break;
        case '\"':
            *dest++ = '&';
            *dest++ = 'q';
            *dest++ = 'u';
            *dest++ = 'o';
            *dest++ = 't';
            *dest++ = ';';
            break;
        default:
            *dest++ = c;
        }
    }
    *dest = '\0';
    return buf;
}


/*
 * Name: escape_string
 *
 * Description: escapes the string inp.  Uses variable buf.  If buf is
 *  NULL when the program starts, it will attempt to dynamically allocate
 *  the space that it needs, otherwise it will assume that the user
 *  has already allocated enough space for the variable buf, which
 *  could be up to 3 times the size of inp.  If the routine dynamically
 *  allocates the space, the user is responsible for freeing it afterwords
 * Returns: NULL on error, pointer to string otherwise.
 */

char *http_escape_string(const char *inp, char *buf,
                         const unsigned int len)
{
    int max;
    char *index_c;
    unsigned char c;
    int found_a_colon = 0;

    max = len * 3;

    if (buf == NULL && max)
        buf = malloc(sizeof (unsigned char) * (max + 1));

    if (buf == NULL)
        return NULL;

    index_c = buf;
    while ((c = *inp++)) {
        if (c == ':' && !found_a_colon && index_c > buf) {
            found_a_colon = 1;
            memmove(buf + 2, buf, (index_c - buf));
            *buf = '.';
            *(buf + 1) = '/';
            index_c += 2;
            *index_c++ = ':';
        } else if (needs_escape((unsigned int) c) || c == '?') {
            *index_c++ = '%';
            *index_c++ = INT_TO_HEX((c >> 4) & 0xf);
            *index_c++ = INT_TO_HEX(c & 0xf);
        } else
            *index_c++ = c;
    }
    *index_c = '\0';

    return buf;
}

void send_error(int error)
{
    const char *the_error;

    switch (error) {

    case 1:
        the_error = "Not enough arguments were passed to the indexer.";
        break;
    case 2:
        the_error = "The Directory Sorter ran out of Memory";
        break;
    case 3:
        the_error =
            "The was a problem changing to the appropriate directory.";
        break;
    case 4:
        the_error = "There was an error escaping a string.";
    case 5:
        the_error = "Too many arguments were passed to the indexer.";
        break;
    case 6:
        the_error = "No files in this directory.";
        break;
    default:
        the_error = "An unknown error occurred producing the directory.";
        break;
    }
    printf("<html>\n<head>\n<title>\n%s\n</title>\n"
           "<body>\n%s\n</body>\n</html>\n", the_error, the_error);
}

int select_files(const struct dirent *dirbuf)
{
    if (dirbuf->d_name[0] == '.')
        return 0;
    else
        return 1;
}

/*
 * Name: index_directory
 * Description: Called from get_dir_mapping if a directory html
 * has to be generated on the fly
 * If no_slash is true, prepend slashes to hrefs
 * returns -1 for problem, else 0
 */

int index_directory(char *dir, char *title)
{
    struct dirent *dirbuf;
    int numdir;
    struct dirent **array;
    struct stat statbuf;
    char http_filename[MAX_FILE_LENGTH * 3];
    char html_filename[MAX_FILE_LENGTH * 6];
    char escaped_filename[MAX_FILE_LENGTH * 18]; /* *both* http and html escape */
    int i;

    if (chdir(dir) == -1) {
        send_error(3);
        return -1;
    }
    numdir = scandir(".", &array, select_files, alphasort);
    if (numdir == -1) {
        send_error(2);
        return -1;
    } else if (numdir == -2) {
        send_error(6);
        return -1;
    }

    if (html_escape_string(title, html_filename, strlen(title)) == NULL) {
        send_error(4);
        return -1;
    }

    printf("<html>\n"
           "<head>\n<title>Index of %s</title>\n</head>\n\n"
           "<body bgcolor=\"#ffffff\">\n"
           "<H2>Index of %s</H2>\n"
           "<table>\n%s",
           html_filename, html_filename,
           (strcmp(title, "/") == 0 ? "" :
            "<tr><td colspan=3><h3>Directories</h3></td></tr>"
            "<tr><td colspan=3><a href=\"../\">Parent Directory</a></td></tr>\n"));

    for (i = 0; i < numdir; ++i) {
        dirbuf = array[i];

        if (stat(dirbuf->d_name, &statbuf) == -1)
            continue;

        if (!S_ISDIR(statbuf.st_mode))
            continue;

        if (html_escape_string(dirbuf->d_name, html_filename,
                               NAMLEN(dirbuf)) == NULL) {
            send_error(4);
            return -1;
        }
        if (http_escape_string(dirbuf->d_name, http_filename,
                               NAMLEN(dirbuf)) == NULL) {
            send_error(4);
            return -1;
        }
        if (html_escape_string(http_filename, escaped_filename,
                               strlen(http_filename)) == NULL) {
            send_error(4);
            return -1;
        }
        printf("<tr>"
               "<td width=\"40%%\"><a href=\"%s/\">%s/</a></td>"
               "<td align=right>%s</td>"
               "<td align=right>%ld bytes</td>"
               "</tr>\n",
               escaped_filename, html_filename,
               ctime(&statbuf.st_mtime), (long) statbuf.st_size);
    }

    printf
        ("<tr><td colspan=3>&nbsp;</td></tr>\n<tr><td colspan=3><h3>Files</h3></td></tr>\n");

    for (i = 0; i < numdir; ++i) {
        int len;
        dirbuf = array[i];

        if (stat(dirbuf->d_name, &statbuf) == -1)
            continue;


        if (S_ISDIR(statbuf.st_mode))
            continue;

        if (html_escape_string(dirbuf->d_name, html_filename,
                               NAMLEN(dirbuf)) == NULL) {
            send_error(4);
            return -1;
        }
        if (http_escape_string(dirbuf->d_name, http_filename,
                               NAMLEN(dirbuf)) == NULL) {
            send_error(4);
            return -1;
        }

        len = strlen(http_filename);
#ifdef GUNZIP
        if (len > 3 && !memcmp(http_filename + len - 3, ".gz", 3)) {
            http_filename[len - 3] = '\0';
            html_filename[strlen(html_filename) - 3] = '\0';
            if (html_escape_string(http_filename, escaped_filename,
                                   strlen(http_filename)) == NULL) {
                send_error(4);
                return -1;
            }

            printf("<tr>"
                   "<td width=\"40%%\"><a href=\"%s\">%s</a> "
                   "<a href=\"%s.gz\">(.gz)</a></td>"
                   "<td align=right>%s</td>"
                   "<td align=right>%ld bytes</td>"
                   "</tr>\n",
                   escaped_filename, html_filename, http_filename,
                   ctime(&statbuf.st_mtime), (long) statbuf.st_size);
        } else {
#endif
            if (html_escape_string(http_filename, escaped_filename,
                                   strlen(http_filename)) == NULL) {
                send_error(4);
                return -1;
            }
            printf("<tr>"
                   "<td width=\"40%%\"><a href=\"%s\">%s</a></td>"
                   "<td align=right>%s</td>"
                   "<td align=right>%ld bytes</td>"
                   "</tr>\n",
                   escaped_filename, html_filename,
                   ctime(&statbuf.st_mtime), (long) statbuf.st_size);
#ifdef GUNZIP
        }
#endif
    }
    /* hey -- even though this is a one-shot deal, we should
     * still free memory we ought to free
     * You never know -- this code might get used elsewhere!
     */
    for (i = 0; i < numdir; ++i) {
        free(array[i]);
        array[i] = NULL;
    }
    free(array);
    array = NULL;

    return 0;                   /* success */
}

int main(int argc, char *argv[])
{
    time_t timep;
    struct tm *timeptr;
    char *now;

    if (argc < 3) {
        send_error(1);
        return -1;
    } else if (argc > 3) {
        send_error(5);
        return -1;
    }

    build_needs_escape();

    if (argv[2] == NULL)
        index_directory(argv[1], argv[1]);
    else
        index_directory(argv[1], argv[2]);

    time(&timep);
#ifdef USE_LOCALTIME
    timeptr = localtime(&timep);
#else
    timeptr = gmtime(&timep);
#endif
    now = strdup(asctime(timeptr));
    now[strlen(now) - 1] = '\0';
#ifdef USE_LOCALTIME
    printf("</table>\n<hr noshade>\nIndex generated %s %s\n"
           "<!-- This program is part of the Boa Webserver Copyright (C) 1991-2002 http://www.boa.org -->\n"
           "</body>\n</html>\n", now, TIMEZONE(timeptr));
#else
    printf("</table>\n<hr noshade>\nIndex generated %s UTC\n"
           "<!-- This program is part of the Boa Webserver Copyright (C) 1991-2002 http://www.boa.org -->\n"
           "</body>\n</html>\n", now);
#endif

    return 0;
}
