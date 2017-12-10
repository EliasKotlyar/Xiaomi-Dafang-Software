/***************************************************************************
                          parse.h  -  description
                             -------------------
    Date                 : May 23  2000
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

//#define NUM_OPS 17   //number of op code mneumonic and directives


enum foo {
    MACS=0,
    MACS1,
    MACW,
    MACW1,
    MACINTS,
    MACINTW,
    ACC3,
    MACMV,
    ANDXOR,
    TSTNEG,
    LIMIT,
    LIMIT1,
    LOG,
    EXP,
    INTERP,
    SKIP,
    EQU,
    DS,
    DYNAMIC,
    DYN,
    MACRO,
    DC,
    STATIC,
    STA,
    DIN,
    DOUT,
    DD,
    DT,
    DW,
    DR,
    CONTROL,
    ENDM,
    END,
    INCLUDE,
    NAME,
    FOR,
    ENDFOR,
    IO,
    CONSTANT,
    CON,
    NUM_OPS
        
};


char op_codes[NUM_OPS+1][9]=
  {
    "MACS",
    "MACS1",
    "MACW",
    "MACW1",

    "MACINTS",
    "MACINTW",
    
    "ACC3",
    "MACMV",
    "ANDXOR",
    "TSTNEG",
    "LIMIT",
    "LIMIT1",
    "LOG",
    "EXP",
    "INTERP",
    "SKIP",
    "equ",
    "ds",
    "dynamic",
    "dyn",
    "macro",
    "dc",
    "static",
    "sta",
    "din",
    "dout",
    "delay",
    "table",
    "twrite",
    "tread",
    "control",
    "endm",
    "end",
    "include",
    "name",
    "for",
    "endfor",
    "IO",
    "constant",
    "con",
    "NotAnOp"
  };

//extern int file_num,source_line_num

