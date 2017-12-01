/*
 *  Boa, an http server
 *  escape.c
 *  Copyright (C) 2001 Jon Nelson <jnelson@boa.org>
 *  Based on escape.pl, Copyright (C) 1996 Larry Doolittle <ldoolitt@boa.org>
 *  Copyright (C) 2001 Larry Doolittle <ldoolitt@boa.org>
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

/* $Id: escape.c,v 1.7.2.2 2004/06/04 02:45:26 jnelson Exp $ */

/*
 unreserved = alnum | mark
 alnum = "0".."9" | "A".."Z" | "a".."z"
 mark = "-" | "_" | "." | "!" | "~" | "*" | "'" | "(" | ")"
 noescape = unreserved | ":" | "@" | "&" | "=" | "+" | "$" | "," | "/"
 */

#ifdef TEST
#include <stdio.h>
#include <stdlib.h>
#else
#include "boa.h"
#endif

#include "escape.h"

unsigned long
    _needs_escape[(NEEDS_ESCAPE_BITS + NEEDS_ESCAPE_WORD_LENGTH -
                   1) / NEEDS_ESCAPE_WORD_LENGTH];

void build_needs_escape(void)
{
    unsigned int a, b;
    const unsigned char special[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz" "0123456789" "-_.!~*'():@&=+$,/?";
    /* 21 Mar 2002 - jnelson - confirm with Apache 1.3.23 that '?'
     * is safe to leave unescaped.
     */
    unsigned short i, j;

    b = 1;
    for (a = 0; b != 0; a++)
        b = b << 1;
    /* I found $a bit positions available in an unsigned long. */
    if (a < NEEDS_ESCAPE_WORD_LENGTH) {
        fprintf(stderr,
                "NEEDS_ESCAPE_SHIFT configuration error -- "
                "%d should be <= log2(%d)\n", NEEDS_ESCAPE_SHIFT, a);
        exit(EXIT_FAILURE);
    } else if (a >= 2 * NEEDS_ESCAPE_WORD_LENGTH) {
        /* needs_escape_shift configuration suboptimal */
    } else {
        /* Ahh, just right! */ ;
    }
    memset(_needs_escape, ~0, sizeof (_needs_escape));
    for (i = 0; i < sizeof (special) - 1; ++i) {
        j = special[i];
        if (j >= NEEDS_ESCAPE_BITS) {
            /* warning: character $j will be needlessly escaped. */
        } else {
            _needs_escape[NEEDS_ESCAPE_INDEX(j)] &= ~NEEDS_ESCAPE_MASK(j);
        }
    }
}

#ifdef TEST
int main(void)
{
    int i;
    build_needs_escape();
    for (i = 0; i <= NEEDS_ESCAPE_BITS; ++i) {
        if (needs_escape(i)) {
            fprintf(stdout, "%3d needs escape.\n", i);
        }
    }
    return (0);
}
#endif
