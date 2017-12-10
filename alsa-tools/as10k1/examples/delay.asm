;;; A simple delay routine
	
	include "emu_constants.asm"	
	name   "delay"

		 
level		control   0.5, #0 , #1	
feedback	control   #0.3, #0 , #1
delay		control   &0.2, &0, &0.5
		
io     IO
	
dly	delay	&0.5		; 0.5 sec delay block
	
write	twrite	dly,0		; write at 0 sec
read	tread	dly,&.2		; read at 0.2 sec

			
	acc3  read.a,delay,write.a,C_0


	macs  io,io,level,read
	macs  write,io,read,feedback

	end
		



