#ifndef PROTO_H
#define PROTO_H

//as10k1.c:

void as_exit(const char *message);
void asm_open(char *name);
void header(void);

//assemble.c:
void op(int,int,int,int,int);
int arg_decode(char *operand,int prev_val);
int arg_decode2(char *operand);
int symbol2index(char *operand, int *flag);
long arg2long(char *operand);
void update_symbol(char *name,u16 type,u16 address, u32 value);
void add_symbol(char *name,u16 type,u16 address, u32 value);
int declared(int operand,int i);
//parse.c:
int parse( char line_string[MAX_LINE_LENGTH], char *line);
int op_decode(char *op_name_ptr);
void new_symbol( char *name_ptr, int constant);
void new_dc(char *symbol,long value, int addr);
int issymbol(char *symbol,struct sym **sym);
void for_handler(char *begin, char *operand );
int symcmp (char *symbol1,char *symbol2);
void symcpy (char *dest, char *source);
//macro.c
void new_macro(char *symbol, char *line, char *operands);
void macro_expand(int macnum,char * operand);
void macro_operand(char *line,char *value);
int ismacro(char *mac);


#define DSP_CODE_SIZE 0x400 

#endif







