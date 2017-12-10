/***************************************************************************
                          parse.c  -  parses each line, stores in temp space
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
#include"parse.h"
#include"proto.h" 

/*
This function parses the asm file calling appropriate functions to blow up macros, include files,
define constants, keep track of equates, and handling assembler directives.

This function is called on a line by line basis.

normally returns value 0 except when "end" directive is encountered in which case it returns 1,
 the "endm" directive which return -1, or the "endfor" deirective which returns -2
*/

extern char type_strings[GPR_TYPE_EQUATE+1][20];
extern int dbg_opt;
extern char *listing,listtemp[60];
extern FILE *listfile;

int parse( char line_string[MAX_LINE_LENGTH], char *line)
{
        int tmp,i,arg[4];
        static int defmacro=0; //set to 0 unless we're in a macro definition
        int op_num;
	char  *leading_symbol=NULL, *op_name_ptr,*tmpc;
        extern char patch_name[PATCH_NAME_SIZE];
	extern int ds_addr,ip,tram_addr,tram_delay_count,tram_table_count;
        extern unsigned int macro_depth;
     
        struct sym *sym;
        struct control *control;
        
        extern struct delay tram_delay[MAX_TANK_ADDR];
        extern struct lookup tram_lookup[MAX_TANK_ADDR];
	
	
	if( line_string[0]=='\0' || line_string[0]==';'||line_string[0]=='%'||line_string[0]=='*')
	    	return(0);
        //remove anything after a ; if one exist
        tmpc=line_string;
        while( *tmpc !=  ';' &&*tmpc != '\0')
                tmpc++;
        *tmpc='\0';

        //check for a leading symbol
	if( line_string[0] != ' ' && line_string[0] != '\t'){
		
		if(strlen(leading_symbol=strtok(line_string,": \t\n"))>MAX_SYM_LEN ){
			printf("symbol \"%s\" is more than %d characters long\n",leading_symbol,MAX_SYM_LEN );
			as_exit("Parse error");
                }
		
                //address ref for skip command:
                if(*leading_symbol=='.'){
                        if( issymbol(leading_symbol,&sym)!=-1)
                                (sym->data.value)+=ip-1;
                }
		op_name_ptr=strtok(NULL, " \t\n");  
        }else{
                op_name_ptr=strtok(line_string, " \t\n");
        }
                
	if(op_name_ptr==NULL)
		return(0);

	        
        //check if it a macro:
        if((tmp=ismacro(op_name_ptr)) != -1 ){
                if(defmacro==0)
                        macro_expand(tmp,strtok(NULL,""));
			return(0);
        }
	
        if( (op_num=op_decode(op_name_ptr))==-1) {
                printf("**Parse Error with op code field \"%s\"\n",op_name_ptr);
                as_exit("Parse Error: Bad neumonic");
        }
	
	//check to see if we're defining a macro
	if(leading_symbol!=NULL && defmacro!=0 && op_num!=CON && op_num!=CONSTANT)
		as_exit("Parse error: Cannot define symbols inside of a macro");
                      
             
        switch(op_num){
                
        case EQU:
                add_symbol(leading_symbol,GPR_TYPE_EQUATE,arg_decode(strtok(NULL, " \t\n"),0),0);
                return(0);
                
        case DS:
                printf("**Assembler warning: \"DS\" will be obsoleted\n");
        case DYNAMIC:
        case DYN:
                add_symbol(leading_symbol,GPR_TYPE_DYNAMIC,ds_addr++,0);

                if( (tmpc=strtok(NULL, " \t\n"))==NULL)
                        tmp=0;
                else if((tmp=arg_decode(tmpc,0)) <=0)
                        tmp=1;
                for(i=1;i<tmp;i++){
                        add_symbol( (char *)NO_SYM ,GPR_TYPE_DYNAMIC,ds_addr++,0);

                }
                return(0);
        case MACRO:
                new_macro(leading_symbol,line,strtok(NULL, "")-line_string+line);
                defmacro++;
                return(0);                                
        case DC:
                printf("**Assembler warning: \"DC\" will be obsoleted\n");
        case STA:
        case STATIC:
                tmpc = strtok(NULL, " ,\t\n") ;
                
                if(tmpc == NULL)
                        as_exit("Error DC directive must contain an initial value");
                while(tmpc!=NULL ){
                        
                        if(  tmpc[0] == '&' )
                                tmp=arg2long(tmpc)*0x800; //account for 11 bit shift of addresses
                        else
                                tmp=arg2long(tmpc);

                        add_symbol(leading_symbol,GPR_TYPE_STATIC,ds_addr++,tmp);

                        leading_symbol=(char *)NO_SYM;
                        
			tmpc=strtok(NULL, " ,\t");
                }
                return(0);

        case CONSTANT:
        case CON:
		//declaring constants inside of a macro is legal and needed for branch macros
		if(defmacro!=0)
			return (0);
		

		tmpc = strtok(NULL, " ,\t\n") ;
                
                if(tmpc == NULL)
                        as_exit("Error Constant directive must contain a value");
                while(tmpc!=NULL ){
                        
                        if(  tmpc[0] == '&' )
                                tmp=arg2long(tmpc)*0x800; //account for 11 bit shift of addresses
                        else
                                tmp=arg2long(tmpc);
                        
			//     add_constant(leading_symbol,tmp);
			
			add_symbol(leading_symbol,GPR_TYPE_CONSTANT,ds_addr++,tmp);
                        
			leading_symbol=(char *)NO_SYM;
                        
                        tmpc=strtok(NULL, " ,\t");
                }
		
                return(0);
        case IO:
                add_symbol(leading_symbol,GPR_TYPE_INPUT,ds_addr++,0);
                add_symbol(strcat(leading_symbol,".o"),GPR_TYPE_OUTPUT,ds_addr++,0);
                return(0);
        case DIN:
                as_exit("DIN is obsoleted, use IO instead");
                add_symbol(leading_symbol,GPR_TYPE_INPUT,ds_addr++,0);
                return(0);
        case DOUT:
                as_exit("DOUT is obsoleted, use IO instead");
                        add_symbol(leading_symbol,GPR_TYPE_OUTPUT,ds_addr++,0);
                        return(0);
        case DD:
		add_symbol(leading_symbol,GPR_TYPE_EQUATE,0x100+tram_delay_count,0);
		(&tram_delay[tram_delay_count])->size = arg2long( strtok(NULL, " \t\n" ) ) +1;
		
		INIT_LIST_HEAD( &(tram_delay[tram_delay_count].tram  )  );
		strcpy((&tram_delay[tram_delay_count])->name,leading_symbol);
		if((dbg_opt&DBG_TRAM))
			printf("Delay Line:  %-16s, length: 0x%05x samples,\n",(&tram_delay[tram_delay_count])->name, (&tram_delay[tram_delay_count])->size);
		tram_delay_count++;
                        return(0);
        case DT:
                add_symbol(leading_symbol,GPR_TYPE_EQUATE,tram_table_count,0);
                (&tram_lookup[tram_table_count])->size = arg2long( strtok(NULL, " \t\n" ) );
                INIT_LIST_HEAD(   &(tram_lookup[tram_table_count].tram) );
		strcpy((&tram_lookup[tram_table_count])->name,leading_symbol);
		
		if((dbg_opt&DBG_TRAM))
			printf("Lookup table: %-16s, length: 0x%05x samples\n",leading_symbol, (&tram_delay[tram_delay_count])->size);
		tram_table_count++;
                        return(0);
        case DW:
                //two symbols are created, "symbol"   ->  addr:0x2xx ; value: tram id #
                //                         "symbol.a" ->  addr:0x3xx ; value: write offset
		
		add_symbol(leading_symbol,TYPE_TRAM_DATA,tram_addr+0x200, arg_decode(tmpc=strtok(NULL, " \t," ),0) );
		add_symbol( strcat(leading_symbol,".a") ,TYPE_TRAM_ADDR_WRITE, (tram_addr++)+0x300 ,
                            arg2long(strtok(NULL," \t\n")));
		if(dbg_opt&DBG_TRAM)
			printf(", in segment: \"%s\"\n",tmpc);
		
		return(0);

	case DR:
		add_symbol(leading_symbol,TYPE_TRAM_DATA,tram_addr+0x200,arg_decode(tmpc=strtok(NULL, " \t," ),0) );
		add_symbol(strcat(leading_symbol,".a"),TYPE_TRAM_ADDR_READ,(tram_addr++)+0x300,
                           arg2long(strtok(NULL," \t\n")));
		if(dbg_opt&DBG_TRAM)
			printf(", in segment: \"%s\"\n",tmpc);
		return(0);
        case CONTROL:
			if( (tmpc = strtok(NULL, "\t ,\n")) ==NULL)
			    as_exit("Parse Error: missing operand(s)");
			    
                        if(  tmpc[0] == '&' )
                                tmp=arg2long(tmpc)<<11; //account for 11 bit shift of addresses
                        else
                                tmp=arg2long(tmpc);
                        add_symbol(leading_symbol,GPR_TYPE_CONTROL,ds_addr++,tmp);
                        issymbol(leading_symbol,(struct sym **)(&control));


                        if( (tmpc = strtok(NULL, "\t ,\n") )==NULL)
			    as_exit("Parse Error: missing operand(s)");
                       

                        if(  tmpc[0] == '&' ) 
                                control->min=arg2long(tmpc)<<11; //account for 11 bit shift of addresses
                        else
                                control->min=arg2long(tmpc);
			
			if( (tmpc = strtok(NULL, "\t ,\n")) ==NULL)
			    as_exit("Parse Error: missing operand(s)");

                       
                        if(  tmpc[0] == '&' ) 
                                control->max=arg2long(tmpc)<<11; //account for 11 bit shift of addresses
                        else
                                control->max=arg2long(tmpc);
                        
                        return(0);
        case ENDM:
                        if(defmacro==1) {
                                defmacro--;
                                return(0);
                        }else if(macro_depth!=0)
                                return(-1);
                        else
                                as_exit("Error, stray ENDM directive");
        case END:
                        if(defmacro==1)
                                as_exit("Error end directive in macro definition");
                        return(1);	
        case INCLUDE:
                if(defmacro==1)
                        as_exit("Error, cannot include file from within macro definition");
		if(listing){
			sprintf(listtemp,"including file");
			fprintf(listfile,"%-50s ||   %s\n",listtemp,line);
                }
		asm_open(strtok(NULL, "\'\""));
		
                return(-3);
        case NAME:
                advance_to_end(op_name_ptr);
                op_name_ptr++;
                advance_over_whites(op_name_ptr);
		if(dbg_opt)
			printf("Patch name:%s\n",op_name_ptr);

		//	printf("%s\n",op_name_ptr);
                tmpc=strtok(op_name_ptr,"\"");
                if(tmpc==NULL)
                        as_exit("Bad name string, did you remember quotes\"\"");
                if(strlen(tmpc)>PATCH_NAME_SIZE)
                        as_exit("Error Patch name exceeds maximum allowed amount (16)");
		memset(patch_name,0,PATCH_NAME_SIZE);
                strcpy(patch_name,tmpc);
                return(0);
        case FOR:
		if(listing){
			sprintf(listtemp,"FOR LOOP");
			fprintf(listfile,"%-50s ||   %s\n",listtemp,line);
                }
                for_handler(line,strtok(NULL,""));
		
                return(-3);
        case ENDFOR:
		sprintf(listtemp,"FOR LOOP DONE");
                return(-2);
        default:
                
                if(defmacro==0){
			
                        for(i=0;i<=3;i++)
                                arg[i]=arg_decode(strtok(NULL,","),0);
                        op(op_num,arg[0],arg[1],arg[2],arg[3]);
                        return(0);

                }else
                        return(0);
        }
        return(0);
}
//assembly-time for loop handling:
void for_handler(char *begin, char *operand )
{
        char *ptr,*next,*line,string[MAX_LINE_LENGTH];
        int start,end,i,done;
        int diff, incr=1;
        struct sym *sym;
        
        ptr=strtok(operand,"=");

        start= arg_decode(strtok(NULL,":"),0);
        end  = arg_decode(strtok(NULL," \t"),0);

        
        if(end>start)
                diff=end-start;
        else{
                diff=start-end;
                incr=-1;
        }
        
        if( (issymbol(ptr,&sym))!=-1)
                sym->data.address=start;
        else
                add_symbol(ptr,GPR_TYPE_EQUATE, start,0);
        
        issymbol(ptr,&sym);
        
        
        while(*begin!='\0')
                begin++;
        begin++;
        
        for(i=0;i<diff;i++){
                next=begin;
                line=next;
                done=0;
                
                while(done==0)
                {
                        while((*next!= '\n') )
                                next++;
			listtemp[0]='\0';
                        *next='\0';
                        if(strlen(line)>MAX_LINE_LENGTH)
                                as_exit("Parse error: Line exceeds allowable limit");
                        strcpy(&string[0],line);
                        //printf("%s\n",string);
                        done=parse(string, line);
			if(listing)
				if(done!=-2)
					fprintf(listfile,"%-50s ||   %s\n",listtemp,line);
                        *next='\n';
                        if(done==-2)
                                break;
                        next++;
                
                
                        line=next;          
                }
                sym->data.address = start+(incr*(i+1));
               
        }

	
}



int op_decode(char *op_name_ptr)
{
	int op_num;

	for(op_num=0;op_num<NUM_OPS;op_num++){
      		if( strcasecmp(&op_codes[op_num][0],op_name_ptr) == 0 )
			return(op_num);
    	}
	return(-1);
}


//check if a symbol is used and returns it's pointer value in sym 
//normally returns 0, if symbol is non exitant, return -1

int issymbol(char *symbol,struct sym **sym)
{
        extern unsigned int macro_depth;
        extern struct list_head sym_head;
        struct list_head *entry;

        
        list_for_each(entry,&sym_head){
                (*sym)=list_entry(entry,struct sym,list);
                if(symcmp((char *)&(*sym)->data.name,symbol)==0){
                        if((*sym)->type!=TYPE_MACRO_ARG)
                                return(0);
                        else if( (*sym)->data.value==(macro_depth+1) )
                                return(0);
                }
                
                        
        }
  
        return(-1);
}


//compares to words, the words can be terminated with a ' ', '\t',  ',' or '\0'
int symcmp (char *symbol1,char *symbol2)
{

        
        while(1){
                if(*symbol1!=*symbol2)
                        return(-1);
                symbol1++;
                symbol2++;
                
                if(symend(symbol1) && symend(symbol2))
                        return(0);
                
        }
        
        
}

//copies a symbol, symbols can be terminated with a ' ' ,  '\t' ,  ','  ,  '\n'  ,  a '\0'
void symcpy (char *dest, char *source)
{  int i=0;
        for(i=0;i<=MAX_SYM_LEN;i++){
                if(source[i]== ' ' || source[i]=='\0' ||source[i]==',' ||source[i]=='\n' || source[i]=='\t'  ) {
                        dest[i]='\0';
                        return;
                }
                dest[i]=source[i];
        }
        as_exit("Error, Maximum symbol length exceeded");
       
       
}
