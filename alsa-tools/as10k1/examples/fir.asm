;;; low pass filter with cut off at 0.004pi (96Hz)
	name "trebass"
	
	include "emu_constants.asm"
	
coef con 0.038684406  0.058115275  0.113007075  0.194116501  0.287525429  0.377072924  0.447195555  0.485671998  0.485783252 0.447503000  0.377505237  0.287987288  0.194517783  0.113292922  0.058289230  0.038818213



n equ 15	; filter order
	
in	io
out equ in	
bass	control	0,0,#1
delay	sta 0,0,0,0,0 ,0,0,0,0,0 ,0,0,0,0,0 ,0	
tmp  dyn

	macints  delay,in,C_0,C_0
	
;;;our filter for the left channel

	macs  C_0,C_0,C_0,C_0	
	for i = n : 1
		macmv   delay+i,delay+i-1,delay+i,coef+i
	endfor
	
	macs tmp,ACCUM,delay,coef

	macs1 out,in,tmp,bass	
	
	end








