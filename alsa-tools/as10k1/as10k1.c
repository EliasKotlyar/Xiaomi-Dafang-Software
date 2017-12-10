/***************************************************************************
                          as10k1.c  -  Main assembler routine
                             -------------------
    Date                 : May 22, 2000
    Copyright            : (C) 2000 by Daniel Bertrand
    Email                : d.bertrand@ieee.ca
 ***************************************************************************/

/*
 * This program was changed to conform the ALSA ideas. Please,
 * bug reports and all other things should be discussed on the
 * <alsa-devel@alsa-project.org> mailing list.
 *                                   Jaroslav Kysela <perex@perex.cz>
 */

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include<string.h>
#include "types.h"

#include "as10k1.h"

char *ptralloc[MAXINCLUDES];
static int num_alloc;
int source_line_num=0,file_num=0;
int macro_line_num=0;
FILE *fp=NULL,*listfile;
char *input=NULL,*output=NULL,*listing=NULL,*current_line=NULL,listtemp[60];


int dbg_opt=0;

char version[]="As10k1 assembler version " VERSION;

char help[]="\n"
"Usage: as10k1 [option...] asmfile\n"
"\n"
"The as10k1 assembler is for the emu10k1 dsp processor\n"
"found in Creative Lab's sblive series of sound cards.\n"
"\n"
"Author: Daniel Bertrand <d.bertrand@ieee.ca>\n"
"\n"
"Options:\n\n"
"     -l [listfile]         Specify a listing file, default is none.\n"
"     -o [emu10k1 file]     Specify an output file, default is based input,\n"
"                           Substituting .asm to .emu10k1\n"
"     -d [dbg_options...]   Turn on debug messages.\n"
"            s              prints all symbols                \n"
"            g              prints defined gprs\n"
"            t              prints tram usage\n"
"            i              prints instructions\n"
"     -h                    Prints this message  \n"
"     -v                    Prints version number. \n"
"\n"
"This program is distributed under the GPL.\n"
"\n";

//  cli==Command-Line Interface && !Creative Labs Inc.

void parse_cli_args(int argc, char *argv[])
{
	int i,j;
	for(i=1;i<argc;i++){
		
		if(argv[i][0]=='-'){
			switch(argv[i][1]){

			case 'o'://specify an output file name
				i++;
				if((i==argc)||(argv[i][0]=='-')){
					printf("Error -o option requires a destination file name\n");
					exit(-1);
				}
				output=argv[i];
				break;
			case 'd': //turn on debugging messages
				j=0;
				i++;
				printf("Debug on, Displaying:");
				
				if((i==argc)||(argv[i][0]=='-')){
					dbg_opt=dbg_opt|DBG_INSTR;//default
					i--;
					printf(" instructions by default\n");
					goto next_cli_arg;
				}
				
				while(argv[i][j]!='\0'){
						
					switch(argv[i][j]){
					case 's':
						dbg_opt=dbg_opt|DBG_SYM;
						printf(" Symbols,");
						break;
					case 'g':
						dbg_opt=dbg_opt|DBG_GPR;
						printf(" GPRs,");
						break;
					case 't':
						dbg_opt=dbg_opt|DBG_TRAM;
						printf(" TRAM,");
						break;
					case 'i':
						dbg_opt=dbg_opt|DBG_INSTR;
						printf(" Instructions,");
						break;
						
					default:
						printf("\b \n**Bad debug option.  ");
						exit(-1);
					}
					j++;
				}
				printf("\b \n");
				break;
						
			case 'l'://produce a listing file
				//printf("Will save list file to %s\n",  );
				i++;
				if((i==argc)||(argv[i][0]=='-')){
					printf("Error -l option requires a destination file name\n");
					exit(-1);
				}
				listing=argv[i];
				
				
				break;
				
			case 'h'://printf help message

			default:
				printf("%s",help);
					
			case 'v':
				printf("%s\n",version);
				exit(0);
			}
			
				
		}else{
			if(input==NULL)
				input=argv[i];
			else{
				printf("Error, only one input file can be specified");	
				as_exit("");
				
			}

		}
next_cli_arg:
	;
	}
}

int main( int argc, char *argv[] )
{
 
        int i;
        char filename[FILENAME_MAX];
        extern int ip;
        u32 val;
	
        parse_cli_args(argc,argv);
	if(input==NULL){
		printf("Error, an input file must be specified\n");
		exit(-1);
	}

        //init symbol list:
        
        INIT_LIST_HEAD(&sym_head);
       
	if(listing!=NULL)
		if((listfile = fopen(listing, "w"))==NULL){
			printf("\nError writing to file %s\n",argv[1]);	
			as_exit("error");       
		}
	asm_open(input); //opens the source file and starts parsing it.
	
	if(output==NULL){
		strcpy(filename, input);
		strtok(filename,".\0");
		strcat(filename, ".emu10k1");
		output = filename;
	}
	
	if((fp = fopen(output, "w"))==NULL){
		printf("\nError writing to file %s\n",argv[1]);	
		as_exit("error");       
	}
	
	if(listing)
		fprintf(listfile,"Summary:\n");
        /*create header*/
        header();

        /*output number of instructions*/
       
        val = __cpu_to_le32(ip);
        fwrite(&val,sizeof(u16),1,fp);
        
        /* write binary code */
        

        for (i = 0; i < ip * 2; i++) {
        	val = __cpu_to_le32(dsp_code[i]);
                fwrite(&val,sizeof(u32),1,fp);
		//for (j = 3; j >= 0; j--)
                //fprintf(fp, "%c", ((u8 *) dsp_code)[i * 4 + j]);
	}
       
  	
	if(listing)
		fclose(listfile);
	fclose(fp);
        for(i=0;i<num_alloc;i++) //free mem, is this necessary, or will the kernel free it automatically?
                free(ptralloc[i]);
              
  	return 0;  //that's it were done
}

/*this function is called to open a asm file and parse it using the parse function.
this function is called by the main function and also by the parse function
when it encounters an "INCLUDE" directive.
*/

void asm_open(char *name)
{
        
        int fd,i;
        int done=0;
        char string[MAX_LINE_LENGTH];
        struct stat st;
        char *next;
        int backup_line_num,backup_file_num;

    
                
        
        
        backup_line_num=source_line_num;
        backup_file_num=file_num;
        
        if( (include_depth++) > max_depth){
		printf("Error: maximum recursive include depth(%d) exceeded\n",max_depth);
		as_exit("");
	}	 		
        buff[num_alloc].name=name;
        source_line_num=0;
        file_num=num_alloc;
        //open the file
        
        if ((unsigned) (fd = open(name, O_RDONLY)) > 255){
                as_exit("error opening input file\n");
        } 
        //get it's stats
        if ( -1 ==  fstat( fd, &st)){
                printf("Error occured attempting to stat %s\n", name);
                as_exit("");
        }
        
        if(( ptralloc[num_alloc]=(char *) malloc(st.st_size+2) )== 0){
                printf("error allocating memory for file %s\n",name);
                close(fd);
                as_exit("");
        }else{
                buff[num_alloc].mem_start=ptralloc[num_alloc]; 

        }
        
        i=num_alloc;
        num_alloc++;
        
        buff[i].mem_end = buff[i].mem_start+st.st_size;
        
        read(fd, buff[i].mem_start, st.st_size);
        close(fd);
         
#ifdef DEBUG        
  	printf("File %s opened:\n",name);
#endif

        //
        //get each line and parse it:
        //
        current_line=buff[i].mem_start;
        source_line_num=1;
        next=current_line;
        while(next!=buff[i].mem_end){
                while((*next!= '\n') && (next!=buff[i].mem_end) )
                        next++;
                listtemp[0]='\0';
                *next='\0';

#ifdef DEBUG        
                printf("%s\n",current_line);
#endif                
                if(strlen(current_line)>MAX_LINE_LENGTH)
                        as_exit("Parse error: Line exceeds allowable limit");
                strcpy(&string[0],current_line);

                done = parse(string,current_line);
		if(listing){
			if(done==1 &&include_depth!=1)
				sprintf(listtemp,"Exiting included file");
			if(done!=-3)
				fprintf(listfile,"%-50s ||   %s\n",listtemp,current_line);
			}
			
		*next='\n';
		
                if(done==1)
			goto done;
		
                if(next!=buff[i].mem_end){
                        source_line_num++;
                        next++;
                }
                
                current_line=next;
        }
        
        if(done==0)
                printf("warning no END directive at end of file %s\n",name);
done:        
        source_line_num=backup_line_num;
        file_num=backup_file_num;

        include_depth--;
       
        
#ifdef DEBUG	
  	printf("File %s closed:\n",name);
#endif
        return;
}




void as_exit(const char *message)
{
        int i;
        
        if(macro_line_num!=0)
                fprintf(stderr, "** Error while expanding macro at line %d\n",macro_line_num);
                
        if(source_line_num!=0)
                fprintf(stderr, "** %s.\n** line number %d:\n %s\nIn file: %s\n", message, source_line_num,current_line,buff[file_num].name);
        else
                fprintf(stderr, "** Error with file:%s\n",buff[file_num].name);
        for(i=num_alloc-1;i>=0;i--)
                free(ptralloc[i]);
        
	exit(1);
}

void output_tram_line(struct list_head *line_head, int type)
{
        struct tram *tram_sym;
        struct list_head *entry;

        list_for_each(entry, line_head ){
        
                tram_sym=list_entry(entry,struct tram,tram);
        
                if(tram_sym->type==type){
                	u32 val;
                        //printf("table read:%s,%x\n",tram_sym->data.name,tram_sym->data.address);
                        tram_sym->data.address-=TRAM_ADDR_BASE;
                        fwrite(&(tram_sym->data.address),sizeof(u8),1,fp);
                        val = __cpu_to_le32(tram_sym->data.value);
                        fwrite(&val,sizeof(u32),1,fp);
			if(listing){
				if(type==TYPE_TRAM_ADDR_READ)
					fprintf(listfile,"\tRead");
				else
					fprintf(listfile,"\tWrite");
			
					fprintf(listfile,": 0x3%02x/0x2%02x (%s), offset 0x%07x\n",tram_sym->data.address,tram_sym->data.address,
						(prev_sym((&tram_sym->list)))->data.name,tram_sym->data.value);
			}
			
                }
        }
}


//creates output header
void header(void)
{
        int i;
        struct sym *sym;
       
        extern struct list_head sym_head;
	struct list_head *entry;
	if(listing)
		fprintf(listfile,"Patch name: \"%s\"\n\n",patch_name);
	
	//patch signature
		   //1234567890123456
	fprintf(fp, "EMU10K1 FX8010 1");
	
	//patchname
        fwrite(patch_name,sizeof(char), PATCH_NAME_SIZE,fp);

        
        fwrite(&gpr_input_count,sizeof(u8),1,fp);
        //write ins/outs

	if(listing)
		fprintf(listfile,"*****************************GPR******************************\n");
        list_for_each(entry,&sym_head){
                sym=list_entry(entry,struct sym,list);
                if(sym->type==GPR_TYPE_INPUT){
                        sym->data.address-=GPR_BASE;
                        fwrite(&(sym->data.address),sizeof(u8),1,fp);
			if(listing)
				fprintf(listfile,"%s IN: 0x%03x, OUT: 0x%03x\n",sym->data.name,sym->data.address+GPR_BASE,sym->data.address+GPR_BASE+1);
                        sym->data.address++;
                        fwrite(&(sym->data.address),sizeof(u8),1,fp);
		}
         }
               
                
	/* dynamic gprs */      
        fwrite(&gpr_dynamic_count,sizeof(u8),1,fp);
        list_for_each(entry,&sym_head){
                sym=list_entry(entry,struct sym,list);
                if(sym->type==GPR_TYPE_DYNAMIC) {
                        sym->data.address-=GPR_BASE;
                        fwrite(&(sym->data.address),sizeof(u8),1,fp);
			if(listing)
				fprintf(listfile,"GPR Dynamic:  0x%03x(%s)\n",sym->data.address+GPR_BASE,sym->data.name);
                }
        }
     
        
        /* static gprs */
	fwrite(&gpr_static_count,sizeof(u8),1,fp);
       
        list_for_each(entry,&sym_head){
                sym=list_entry(entry,struct sym,list);
                if(sym->type==GPR_TYPE_STATIC){
                	u32 value;
                        sym->data.address-=GPR_BASE;
                        fwrite(&(sym->data.address),sizeof(u8),1,fp);
                        value = __cpu_to_le32(sym->data.value);
                        fwrite(&value,sizeof(u32),1,fp);
			if(listing)
				fprintf(listfile,"GPR Static:  0x%03x(%s), Value:0x%08x\n",sym->data.address+GPR_BASE
					,sym->data.name,sym->data.value);
                }

        }
        
        /* control gprs */
        fwrite(&gpr_control_count,sizeof(u8),1,fp);
         
      
        list_for_each(entry,&sym_head){
                sym=list_entry(entry,struct sym,list);
                if(sym->type==GPR_TYPE_CONTROL){
                	u32 value;
                        sym->data.address-=GPR_BASE;
                        fwrite(&(sym->data.address),sizeof(u8),1,fp);
                        value = __cpu_to_le32(sym->data.value);			
                        fwrite(&value,sizeof(u32),1,fp);
                        value = __cpu_to_le32(((struct control *)sym)->min);
                        fwrite(&value,sizeof(u32),1,fp);
                        value = __cpu_to_le32(((struct control *)sym)->max);
                        fwrite(&value,sizeof(u32),1,fp);
                       
                        fwrite(&(sym->data.name), sizeof(char), MAX_SYM_LEN, fp);
			if(listing)
				fprintf(listfile,"GPR Control: 0x%03x(%s), value:0x%08x, Min:0x%08x, Max:0x%08x\n",sym->data.address+GPR_BASE,sym->data.name,
					sym->data.value,((struct control *)sym)->min,((struct control *)sym)->max);
				

                }                
        }
	
        /*constant GPRs*/
	fwrite(&gpr_constant_count,sizeof(u8),1,fp);

	list_for_each(entry,&sym_head){
		sym=list_entry(entry,struct sym,list);
		if(sym->type==GPR_TYPE_CONSTANT){
			sym->data.address-=GPR_BASE;
			fwrite(&(sym->data.address),sizeof(u8),1,fp);
			fwrite(&(sym->data.value),sizeof(u32),1,fp);
			if(listing)
				fprintf(listfile,"GPR Constant: 0x%03x(%s), Value:0x%08x\n",sym->data.address+0x100
					,sym->data.name,sym->data.value);
		}
	}


	if(listing)
		fprintf(listfile,"*****************************TRAM*****************************\n");

	/*lookup-tables*/
        fwrite(&tram_table_count,sizeof(u8),1,fp);
	
	for(i=0;i<tram_table_count;i++){
		u32 value;
		value = __cpu_to_le32(tram_lookup[i].size);
                fwrite(&value,sizeof(u32),1,fp);
		if(listing)
			fprintf(listfile,"Lookup-table block:%s, size:0x%08x\n",(&tram_lookup[i])->name,tram_lookup[i].size);
		// read lines
                fwrite(&(tram_lookup[i].read),sizeof(u8),1,fp);
                output_tram_line(&(tram_lookup[i].tram),TYPE_TRAM_ADDR_READ);
                //write lines
                fwrite(&(tram_lookup[i].write),sizeof(u8),1,fp);
                output_tram_line(&(tram_lookup[i].tram),TYPE_TRAM_ADDR_WRITE);
	}
               
	/*Delay Lines*/
        fwrite(&tram_delay_count,sizeof(u8),1,fp);
        for(i=0;i<tram_delay_count;i++){
                
                fwrite(&(tram_delay[i].size),sizeof(u32),1,fp);
		if(listing)
			fprintf(listfile,"Delay-line block:%s, size:0x%08x\n",tram_delay[i].name,tram_delay[i].size);
                // read lines
                fwrite(&(tram_delay[i].read),sizeof(u8),1,fp);
                output_tram_line(&(tram_delay[i].tram),TYPE_TRAM_ADDR_READ);
             
                //write lines
                fwrite(&(tram_delay[i].write),sizeof(u8),1,fp);
                output_tram_line(&(tram_delay[i].tram),TYPE_TRAM_ADDR_WRITE);
        }
}
