/*
 *  Boa, an http server
 *  This file Copyright (C) 2002 Peter Korsgaard <jacmet@sunsite.dk>
 *  Some changes Copyright (C) 2003-2004 Jon Nelson <jnelson@boa.org>
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
 */

/* $Id: access.c,v 1.1.2.6 2005/02/22 14:11:29 jnelson Exp $ */

#include <string.h>
#include <stdlib.h>
#include <fnmatch.h>
#include "boa.h"
#include "access.h"

struct access_node {
    char *pattern;
    enum access_type type;
};

static int n_access;

static struct access_node *nodes = NULL;

static void access_shutdown(void);

static void access_shutdown(void)
{
    int i;

    if (nodes) {
        for (i = 0; i < n_access; i++) {
            if (nodes[i].pattern) {
                free(nodes[i].pattern);
            } else {
                WARN("Not freeing NULL access pattern!");
            }
        }
        free(nodes);
    }

    nodes = NULL;
    n_access = 0;
}

void access_init(void)
{
    if (n_access || nodes) {
        access_shutdown();
    }
}

void access_add(const char *pattern, enum access_type type)
{
    nodes = realloc(nodes, (n_access + 1) * sizeof (struct access_node));
    if (!nodes) {
        DIE("realloc of nodes failed!");
    }

    nodes[n_access].type = type;
    nodes[n_access].pattern = strdup(pattern);
    if (!nodes[n_access].pattern) {
        DIE("strdup of pattern failed!");
    }
    ++n_access;
}                               /* access_add */


enum access_type access_allow(const char *file)
{
    int i;

    /* find first match in allow / deny rules */
    for (i = 0; i < n_access; i++) {
        if (fnmatch(nodes[i].pattern, file, 0) == 0) {
            return nodes[i].type;
        }
    }

    /* default to allow */
    return ACCESS_ALLOW;
}                               /* access_allow */
