/***************************************************************************
                          assemble.c  -  Assembles the parsed lines
                             -------------------
    Date                : May 24 2000
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
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include <ctype.h>


#include"types.h"
#include"proto.h"

extern int dbg_opt;
extern FILE *listfile;
extern char *listing;
char type_strings[GPR_TYPE_EQUATE+1][20]={
"Input",
"Output",
"Constant",
"Static",
"Dynamic",
"Control",
"Tram Data Reg",
"Tram Address/Read",
"Tram Address/Write",
"Macro arg",
"Equate"
};

void op(int op, int z,int  w,int  x,int  y)
{
	int  w0, w1;
        extern int dsp_code[DSP_CODE_SIZE];
        extern int ip;
        extern char op_codes[35][9];
	extern char listtemp[60];
	if (ip >= 0x200)
		as_exit("to many instructions");
	if (op >= 0x10 || op < 0x00)
		as_exit("illegal op code");
        
        //check if gpr is valid, optional do additional modifications
	z=declared(z,1);
	declared(w,2);
	declared(x,3);
	declared(y,4);

        if ( (dbg_opt & DBG_INSTR) !=0  )
		printf( "0x%03x\t%s(%d)  \t0x%03x,0x%03x,0x%03x,0x%03x\n",2*ip,op_codes[op],op,z,w,x,y);
	if(listing)
		sprintf(listtemp, "0x%03x   %-9s(%02d)   0x%03x,0x%03x,0x%03x,0x%03x",2*ip,op_codes[op],op,z,w,x,y);
	
	w0 = (x << 10) | y;
	w1 = (op << 20) | (z << 10) | w;
	dsp_code[ip * 2] = w0;
	dsp_code[ip * 2 + 1] = w1;
	ip++;

}

int declared(int operand,int i){

        struct sym *sym;
        extern struct list_head sym_head;
        struct list_head *entry;
 
        
       
        if ((operand < 0x040)||(operand >= 0x400)){
                printf("** Assembler Error with Operand %d:0x%x\n",i,operand);
                as_exit("Operand has value out of range");
        }
        
        if((operand < 0x400) && operand >= 0x100)
        {
                list_for_each(entry,&sym_head){
                        sym=list_entry(entry,struct sym,list);
                        if( (sym->data.address == operand ) && sym->type!=GPR_TYPE_EQUATE){
				if( ( sym->type==GPR_TYPE_CONSTANT) && (i==1) ){
					printf("** Assembler Error with Operand %d:0x%x\n",i,operand);
					as_exit("** Error: Destination register is a read-only constant");
				}
				
				else if(sym->type!=GPR_TYPE_INPUT)
                                        return(operand);
                                else
                                        return( i==1? operand + 1 : operand);
                        }
                }
                
               
        }
        else if(operand<0x100)
                return(operand);
        
         printf("** Assembler Error with Operand %d:0x%x\n",i,operand);
         as_exit("Operand address is undeclared");
         return(0);
}

//each operand will be something like :  ,  sym1 + sym2 * sym3 ,
//we will recursively decode each symbol and perform proper operations:
int arg_decode(char *operand, int prev_val)
{
        int value;
        char oper='0';

        
	//Nothing: 
        if(operand==NULL)
                as_exit("Parse Error: missing operand(s)");

        
        if(*operand==','||*operand=='\n'||*operand=='\0')
                return(prev_val);

 
        //skip over leading blanks(if any):
        advance_over_whites(operand);
        
        if(*operand==','||*operand=='\n' ||*operand=='\0')
                return(prev_val);
        
        //get the operator:
        if(*operand=='+' || *operand=='-' || *operand=='/' || *operand== '*'){
                oper=*operand;
                operand++;
        }
        
        //skip over any blanks after the oper
        advance_over_whites(operand);
        
        //decode the symbol/value:
        value=arg_decode2(operand);

        //advance to next symbol
        while( *operand!='+' && *operand!='-' && *operand!='/' && *operand!= '*'  && *operand != '\0'  &&*operand!=',' &&*operand!='\n')
                operand++;
        
        switch (oper){
                
        case '+':
                return(arg_decode(operand,prev_val+value));
        case '-':
                return(arg_decode(operand,prev_val-value));
        case '/':
                return(arg_decode(operand,prev_val/value));
        case '*':
                return(arg_decode(operand,prev_val*value));       
        default:
                return(arg_decode(operand,value));
        
        }
        
}

//this function does argument decoding
int arg_decode2(char *operand)
{
        extern int ip,ds_addr;
        extern unsigned int macro_depth;
        struct sym *sym;
        extern struct list_head sym_head;
        struct list_head *entry;
        //printf("operand:%s\n",operand);

	
        if(operand[0]=='.' &&isalpha(operand[1])){
                add_symbol(operand,GPR_TYPE_STATIC, ds_addr++, -(long)ip);
                return(ds_addr-1);
         }

        
        // Hex
        if((char)(*operand)=='$')
                return((int)strtol(operand+1,NULL,16));
	// Octal 
        if((char)(*operand)=='@')
                return((int)strtol(operand+1,NULL,8));
        // Binary:
        if((char)(*operand)=='%')
                return((int)strtol(operand+1,NULL,2));
	// Decimal:
        if( (operand[0] >= '0' && operand[0] <='9') ||operand[0]=='-')
                return((int)strtol(operand,NULL,10));

        
        
        //Symbol:
        list_for_each(entry,&sym_head){
                sym=list_entry(entry,struct sym,list);
                if(symcmp(sym->data.name,operand)==0){
                        if(sym->type!=TYPE_MACRO_ARG)
                                return(sym->data.address);
                        else if(sym->data.value==(macro_depth))
                                return(sym->data.address);     
                        
                }
        }
               
        
        
        printf("Parse error with operand: \"");
        while(!symend(operand))
                printf("%c",*(operand++));
        printf("\"\n");
        as_exit("Bad operand");
        
        //printf("** Parse error with operand: \"%s\"\n",operand);
        as_exit("\"\nOperand isn't a defined symbol or value");
        return(0);
}


#define FACTOR 0x7fffffff
#define SAMP_FREQ 48000
//used by the DC operation to get a long int:
long arg2long(char *operand){


	//Nothing: 	
        if(operand==NULL)
                as_exit("Parse Error: missing operand(s)");
	
        advance(operand);

        //Fractional ( -1 <= operand <= 1 )
        if(operand[0]=='#')
                return((long)(atof(operand+1)*FACTOR));
        // Time value
        if(operand[0]=='&')
                 return((long)(atof(operand+1)*48000));
        // Hex:
        if((char)(*operand)=='$')
                return(strtoul(operand+1,NULL,16));
        // Binary:
        if((char)(*operand)=='%')
                return(strtoul(operand+1,NULL,2));
	// Octal:
        if((char)(*operand)=='@')
                return(strtoul(operand+1,NULL,8));
	// Decimal:
        if( (operand[0] >= '0' && operand[0] <='9') || operand[0]=='-' || operand[0]=='.'){
                if(atof(operand)<1 && atof(operand)>-1)
                        return((long)(atof(operand)*FACTOR));
                else
                        return(strtol(operand,NULL,10));
        }
        
        
        printf("Parse error with operand:\"%s\"\n",operand);
        //        while(!symend(operand))
        //      printf("%c",*operand);
        //printf("\"\n");
        as_exit("Bad operand");
        
        return(0);
}
void update_symbol(char *name,u16 type,u16 address,u32 value){
        struct sym *sym;
        
        
        switch(type){
                
        case TYPE_MACRO_ARG:
               
                if( issymbol(name,&sym) == -1 ){
                        add_symbol(name,type,address,value);
                        return;
                }

                if(sym->type!=TYPE_MACRO_ARG){
                        printf("Error: with argument:%s",name);
                        as_exit("Error:symbol is already defined");
                }
                sym->data.address=address;
                break;
        default:
                if( issymbol(name,&sym) == -1 ){
                        add_symbol(name,type,address,value);
                        return;
        }
                break;  
        }
        
}




void add_symbol(char *name, u16 type, u16 address, u32 value)
{
       
        extern int gpr_input_count,gpr_output_count,gpr_static_count,gpr_dynamic_count,gpr_control_count,gpr_constant_count;
        struct sym *sym;
        struct tram *tmp_ptr;
        extern struct list_head sym_head;
        extern struct delay tram_delay[MAX_TANK_ADDR];
        extern struct lookup tram_lookup[MAX_TANK_ADDR];
	int tmp;
	
        
        if(name==NULL)
                as_exit("Parse Error: This directive requires a label");

        if(symcmp(name,NO_SYM)!=0 &&type== GPR_TYPE_CONSTANT){
		if(issymbol(name,&sym)==0){	
			if(sym->data.value != value)
				as_exit("Error: Constant redeclared as another value");
			else
				
				return;
			}
	}
	
	
        if(symcmp(name,NO_SYM)!=0 && type!=TYPE_MACRO_ARG)
        {
                if(issymbol(name,&sym)!=-1)
                        as_exit("Parse Error: Symbol is already defined");
                if(ismacro(name)!=-1)
                        as_exit("Parse Error: Symbol is already defined as a macro");
                if(isalpha(*name)==0 && name[0]!='.')
                        as_exit("Parse Error: Symbol must start with a alpha character (a-z)");
        }
       
        switch(type){
        case GPR_TYPE_CONTROL:
                sym=(struct sym *)malloc(sizeof(struct control));
                list_add_tail(&sym->list, &sym_head);
                break;
        case TYPE_TRAM_ADDR_READ:
        case TYPE_TRAM_ADDR_WRITE:
                sym=(struct sym *)malloc(sizeof(struct tram));
                list_add_tail(&sym->list, &sym_head);
                
                //if ID is that of a delay:
                if((tmp=((struct sym * ) sym->list.prev)->data.value)>0xff){
                        tmp=tmp-0x100;
                        list_add_tail(&(((struct tram *)sym)->tram) , &(tram_delay[tmp].tram) );
                        if(type== TYPE_TRAM_ADDR_READ)
                                tram_delay[tmp].read++;   
                        else
                                tram_delay[tmp].write++;
                }else{
                        tmp_ptr=(struct tram *)sym;
                        list_add_tail(&(((struct tram *)sym)->tram) , &(tram_lookup[tmp].tram) );
			tmp_ptr=(struct tram *)sym;
                                if(type== TYPE_TRAM_ADDR_READ)
                                tram_lookup[tmp].read++;   
                        else
                                tram_lookup[tmp].write++;
                }
                break;
        default:
		
		sym=(struct sym *)malloc(sizeof(struct sym));
                list_add_tail(&sym->list, &sym_head);
		
        }
        
        
	
        symcpy(sym->data.name,name);
        sym->data.address=address;
        sym->type=type;
        sym->data.value=value;	
	//GPR debugging:
	if((dbg_opt&DBG_GPR) && type<=GPR_TYPE_CONTROL)
		printf("GPR:    %-16s 0x%03x Value=0x%08x, Type: %s\n",name,address,value,type_strings[type] );
	
	//tram debugging:
	else if((dbg_opt&DBG_TRAM && type ==  TYPE_TRAM_DATA))
		printf("TRAM Access: %-16s",name);
	else if((dbg_opt&DBG_TRAM && type ==  TYPE_TRAM_ADDR_WRITE))
		printf(", type: Write, using 0x%03x/0x%03x, offset:0x%07x",address,address-0x100,value );
	else if((dbg_opt&DBG_TRAM && type ==  TYPE_TRAM_ADDR_READ))
		printf(", type: Read,  using 0x%03x/0x%03x, offset:0x%07x",address,address-0x100,value );
	//General Symbol debugging:		
	else if((dbg_opt&DBG_SYM )){
		printf("symbol: %-16s 0x%03x Type: %s\n",name,address,type_strings[type]);
	}
	

	switch(type){
        case TYPE_MACRO_ARG:
                return;
        case GPR_TYPE_INPUT:
                gpr_input_count++;
                return;
        case  GPR_TYPE_OUTPUT:
                gpr_output_count++;
                return;
        case GPR_TYPE_STATIC:
                gpr_static_count++;
                return;
        case GPR_TYPE_DYNAMIC:
                gpr_dynamic_count++;
                return;
        case GPR_TYPE_CONTROL:
                gpr_control_count++;
                return;
	case GPR_TYPE_CONSTANT:
		gpr_constant_count++;
		return;
        default:
                return;
        }
        
}















