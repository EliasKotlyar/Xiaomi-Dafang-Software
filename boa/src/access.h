/*
 *  Boa, an http server
 *  This file Copyright (C) 2002 Peter Korsgaard <jacmet@sunsite.dk>
 *  Some changes (C) 2004 Jon Nelson <jnelson@boa.org>
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

/* $Id: access.h,v 1.1.2.4 2005/02/22 14:11:29 jnelson Exp $ */

#ifndef _ACCESS_H
#define _ACCESS_H

enum access_type { ACCESS_DENY, ACCESS_ALLOW };

void access_init(void);
void access_add(const char *pattern, enum access_type);
enum access_type access_allow(const char *file);

#endif                          /* _ACCESS_H */
