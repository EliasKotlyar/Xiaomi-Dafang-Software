	name  "volLR"
	

	include "emu_constants.asm"
volLR   control  #1,0,#1
	
inl	IO
inr     IO
outl	equ	inl
outr    equ     inr

	macs	outl,inl.o,inl,volLR
        macs    outr,inr.o,inr,volLR
	end
	
	
