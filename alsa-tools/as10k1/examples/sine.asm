	name "Sine wave Gen"
	include "emu_constants.asm"

in	io
out	equ  in
	

delta control $3afa691,0,$7fffffff ; controls frequency

		
cosx control #1,0,#1		; amplitude of sinewave 
sinx sta 0	

	
		
	macs  sinx,sinx,delta,cosx       
	macs1 cosx,cosx,delta,sinx 
	macmv  out,cosx,C_0,C_0

	
	end








