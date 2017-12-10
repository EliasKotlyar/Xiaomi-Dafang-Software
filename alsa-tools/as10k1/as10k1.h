/***************************************************************************
                          as10k1.h  -  description
                             -------------------
    Date                 : May 25, 2000
    Copyright            : (C) 2000 by Daniel Bertrand
    Email                : d.bertrand@ieee.ca
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include"types.h"
#include"proto.h"


#define max_depth 3   //max include nesting depth
FILE *dc_fp;
int include_depth=0;


#define MAXINCLUDES 25

struct list_head sym_head;

struct  alloc
{
        char *mem_start;
        char *mem_end;
        char *name;
};

struct alloc buff[MAXINCLUDES];
u32 dsp_code[DSP_CODE_SIZE];
int ip=0;

int ds_addr=0x100;     // DS start at 0x100 ( start of the general purpose registers).
int tram_addr=0;       // tram data/addr read/write counter

struct delay tram_delay[MAX_TANK_ADDR];
struct lookup tram_lookup[MAX_TANK_ADDR];

int gpr_input_count=0;
int gpr_output_count=0;
int gpr_static_count=0;
int gpr_dynamic_count=0;
int gpr_control_count=0;
int tram_delay_count=0;
int tram_table_count=0;
int gpr_constant_count=0;

char patch_name[PATCH_NAME_SIZE]="NO_NAME";
int macro_depth=0;


