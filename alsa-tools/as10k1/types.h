#ifndef TYPES_H
#define TYPES_H

#include "list.h"
//i'm not sure about these type definitions, especially on non-x86
#ifdef NO_LINUX //in the event this actually is used on non-linux platforms
#define u8 unsigned char
#define u16 unsigned short int
#define u32 unsigned int
#else
#include <linux/types.h>
#include <asm/byteorder.h>
#define u8 __u8
#define u16 __u16
#define u32 __u32
#endif

#define MAX_SYM_LEN 32
#define PATCH_NAME_SIZE 32

#define MAX_TANK_ADDR 0x9f  //maximum number of tank address
#define MAX_LINE_LENGTH 256   //max length of a source code line


#define GPR_TYPE_INPUT		0x0
#define GPR_TYPE_OUTPUT		0x1
#define GPR_TYPE_CONSTANT	0x2
#define GPR_TYPE_STATIC		0x3
#define GPR_TYPE_DYNAMIC	0x4
#define GPR_TYPE_CONTROL	0x5

#define TYPE_TRAM_DATA		0x6
#define TYPE_TRAM_ADDR_READ	0x7
#define TYPE_TRAM_ADDR_WRITE	0x8


#define TYPE_MACRO_ARG		0x9
#define GPR_TYPE_EQUATE		0xa  //just a symbol           



#define TRAM_READ  0x1
#define TRAM_WRITE 0x2
 



#define DBG_SYM 1
#define DBG_GPR 2
#define DBG_TRAM 4
#define DBG_INSTR 8


struct symbol{
        char name[MAX_SYM_LEN ];
        u32 value; //initial value of GPR, or the value (if it's an equate);
        u16 address; //address of GPR
};


struct sym{
        struct list_head list;
        u16 type;
        struct symbol data;
};

struct control{
        struct list_head list;
        u16 type;
        struct symbol data;
        u32 max;
        u32 min;
};

//all tram read/writes from a linked-list with list head in the delay/lookup-table definition block.
struct tram{
        struct list_head list;
        u16 type;
        struct symbol data;
        struct list_head tram;
};

//a delay block
struct delay{
        u32 size;
        u8 read;
        u8 write;
        struct list_head tram;
	char name[MAX_SYM_LEN];
};
//a lookup-table block
struct lookup{
        u32 size;
        u8 read;
        u8 write;
        struct list_head tram;
	char name[MAX_SYM_LEN];
};

struct macrdef{
        char *ptr;
        char name[MAX_SYM_LEN ];
        char *operands;
        int line_num;
};


#define NO_SYM "__NO_NAME"


#define MAX_DEF_MACRO 30
#define MAX_MAC_DEPTH 5


//some C macros:
//blank ptr:
#define blank(PTR) (*PTR==' ' || *PTR=='\t')

//value is end of a symbol:
#define symend(ptr) ( blank(ptr) || *ptr=='\0'|| *ptr==','||*ptr=='+'||*ptr=='-'||*ptr=='/'||*ptr=='*')

//used for advancing over white spaces and comma:
#define advance(ptr) while( *ptr == ' ' || *ptr== '\t' ||*ptr==',' ){ ptr++;}
//advance over white spaces only:
#define advance_over_whites(ptr) while(*ptr == ' ' || *ptr== '\t'){ptr++;}
//advances to end of symbol
#define advance_to_end(ptr) while(!symend(ptr)){ptr++;}

//"returns" pointer to the previous entry:
#define prev_sym(entry) list_entry(entry->prev,struct sym,list)

#endif

#define GPR_BASE        0x100
#define TRAM_DATA_BASE  0x200
#define TRAM_ADDR_BASE  0x300
