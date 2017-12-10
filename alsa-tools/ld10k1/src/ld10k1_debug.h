/*
 *  EMU10k1 loader
 *
 *  Copyright (c) 2003 by Peter Zubaj
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation;
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
 
#ifndef __LD10K1_DEBUG_H
#define __LD10K1_DEBUG_H

typedef struct {
	int what;
} ld10k1_fnc_debug_t;

int ld10k1_fnc_debug(int data_conn, int op, int size);

#endif /* __LD10K1_DEBUG_H */
