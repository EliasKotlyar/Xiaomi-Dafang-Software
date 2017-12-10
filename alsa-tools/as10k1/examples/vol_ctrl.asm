	name  "hw vol ctrl"
	
	include "emu_constants.asm"
Vol_ctrl  control  #1,0,#1
	
in	IO
out	equ	in
	macs	out,C_0,in,Vol_ctrl
	end
	
	
		
	
