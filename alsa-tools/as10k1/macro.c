/***************************************************************************
                          macro.c  - various functions to handle macros
                             -------------------
    Date                 : May 23 2000
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
#include"types.h"
#include"proto.h"
#include <ctype.h>

int macro_ctn;
struct macrdef macro[MAX_DEF_MACRO];
extern char *listing,listtemp[60];
extern FILE *listfile;

//determines if an opcode neumonic is a macro

int ismacro(char *mac)
{
       
        int i;
        
        for(i=0;i<macro_ctn; i++){
                if(strcasecmp(macro[i].name,mac)==0){
                        return(i);
                }
        }
        return(-1);
}

//defines a new macro, adds it to the macro list
void new_macro(char *symbol, char *line, char *operand)
{
        extern int source_line_num;
        struct sym *sym;

	if (macro_ctn >= MAX_DEF_MACRO)
                as_exit("Parse Error: Too many macros");

        if(isalpha(*symbol)==0)
                as_exit("Parse Error: Symbol must start with an alpha character");
        
        if(ismacro(symbol)!=-1)
                as_exit("Parsed Error: macro is already defined");

        if(issymbol(symbol,&sym)!=-1)
                as_exit("Parse Error: Symbol is already defined");

        macro[macro_ctn].line_num=source_line_num;
        macro[macro_ctn].ptr=line;
        strcpy(macro[macro_ctn].name,symbol); 
        macro[macro_ctn].operands=operand;
        macro_ctn++;
        
}

//called from parsed() when a macro is used, stores the arguments and recursively calls the parse().

void macro_expand(int macnum,char *operand )
{
        char *line,*next;
        int done=0,i,old;
        extern unsigned int macro_depth;
        extern int macro_line_num;
        char string[MAX_LINE_LENGTH];
       
        //initialize macro use:
        i=0;
       
        if(macro_depth+1> MAX_MAC_DEPTH)
                as_exit("Error exceeded maximum number of recursive macro calls");

        old=macro_line_num;
        macro_line_num=macro[macnum].line_num;
        macro_operand(macro[macnum].operands,operand);
        macro_depth++;
        
        line=macro[macnum].ptr;
        next=line;

        
        while((*next!= '\n') )         //skip to the line after the macro definition
                next++;
        line=next;
        
        
        //Expand the macro calling parse()
        
        while(done!=-1)
        {
                
                while((*next!= '\n') )
                        next++;
                
                *next='\0';
   
                strcpy(&string[0],line);
                listtemp[0]='\0';
                done=parse(string, line);
                macro_line_num++;
                *next='\n';
		if(listing){
			if(done==1)
				sprintf(listtemp,"macro expansion done");
			if(done!=-3)
				fprintf(listfile,"%-50s ||   %s\n",listtemp,line);
		}
                if(done==-1)
                        break;
                next++;
        line=next;
        }
        macro_line_num=old;
        macro_depth--;

        return;

}
//assigns calling arguments with defined symbols.
void macro_operand(char *symbols,char *val)
{
        char tmp[MAX_LINE_LENGTH],*ptr=symbols,*sym=tmp,*next_sym=sym,*next_val=val;
        extern unsigned int macro_depth;
       
       
        if(symbols==NULL&&val==NULL)
                return;
        if(symbols==NULL||val==NULL)
                as_exit("error in macro_operand, Null operand list");
                
       
        while(*ptr!='\n' && *ptr!=';')
                ptr++;
        *ptr='\0';
         
        strcpy(tmp,symbols);
        
        //#ifdef DEBUG        
        //        printf("syms:\"%s\",vals:\"%s\"\n",sym,val);
        //#endif
          *ptr='\n';
        
        while(1){

                //skip over blanks:
                advance(next_sym);
                advance(next_val);
                
                
                sym = next_sym;
                val = next_val;
               
                if(*next_val=='\0' && *next_sym=='\0')
                        return;
                if(*next_sym=='\0')
                        as_exit("Error, To many arguments for defined Macro");
                
                
                if(*next_val=='\0')
                        as_exit("Error, Not enough arguments for defined macro");

                
                
                while(*next_sym != '\0' && *next_sym!= ',' )
                        next_sym++;

                while(*next_val != '\0' && *next_val!= ',' )
                        next_val++;
                //                                 printf("sym=\"%s\";val=\"%s\"(=0x%x)\n",sym, val,arg_decode(val,0) );
                if( sym!=next_sym || val!=next_val ){
                        update_symbol(sym,TYPE_MACRO_ARG,arg_decode(val,0),macro_depth+1);
                }
               
        }
        
} 





















