/*
 *  EMU10k1 loader
 *
 *  Copyright (c) 2003,2004 by Peter Zubaj
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation;  either version 2 of the License, or
 *   (at your option) any later version.
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

#ifndef __LD10K1_DUMP_FILE_H
#define __LD10K1_DUMP_FILE_H

#define DUMP_TYPE_LIVE 0
#define DUMP_TYPE_AUDIGY_OLD 1
#define DUMP_TYPE_AUDIGY 2

typedef struct {
	char signature[16]; /* LD10K1 DUMP 001 */
	int dump_type;
	int tram_size;
	int ctl_count;
	int gpr_count;
	int tram_count;
	int instr_count;
} ld10k1_dump_t;

#define DUMP_TRAM_NULL 0
#define DUMP_TRAM_READ 1
#define DUMP_TRAM_WRITE 2

typedef struct {
	int type;
	unsigned int addr;
	unsigned int data;
} ld10k1_tram_dump_t;

typedef struct {
	int used;
	unsigned int op;
	unsigned int arg[4];
} ld10k1_instr_dump_t;

typedef struct {
	char name[44];
	int index;
	unsigned int vcount;		/* count of GPR (1..32) */
	unsigned int count;			/* count of GPR (1..32) */
	unsigned int gpr_idx[32];	/* GPR number(s) */
	unsigned int value[32];
	unsigned int min;			/* minimum range */
	unsigned int max;			/* maximum range */
	unsigned int translation;	/* typ - 0 - bool, num 1 - enum */
} ld10k1_ctl_dump_t;

#endif /* __LD10K1_DUMP_FILE_H */
