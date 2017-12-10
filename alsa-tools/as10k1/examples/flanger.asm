;;; Simple mono flanger
;;; Author:Daniel Bertrand
;;; Date: May 29,2000

;;; This program is free software; you can redistribute it and/or modify  
;;; it under the terms of the GNU General Public License as published by  
;;; the Free Software Foundation; either version 2 of the License, or     
;;; (at your option) any later version.

;;; References:	
;;; http://www.harmony-central.com/Effects/Articles/Flanging/
		
;;;  speed( formerly "delta")=2*pi*freq/48000
;;;  this give us our delta value for a specific freq (0.1-0.3Hz is good)
		

	include "emu_constants.asm"
	name "flanger"
		
in	IO
out	equ in	

	
	
speed	  control  2e-05 , 0 , 1e-4 	; Controls frequency (radians)
delay   control  &7e-3  ,$1600 , 20e-3	; twice (2*) average delay (sec)
width	  control  #0.33  ,0	 ,0.5	; width control 
forward	  control  #1	 ,0      ,#1	; forward mix 
feedback  control  0.3   ,0      ,0.5	; feedback level 

;; sine generator storage spaces:	
sinx  sta  0	
cosx  sta  #0.5
			
tmp  dyn 	
tmp2 dyn
	
;;; Two Delay Lines:	
	
	
dly   delay  &20e-3		;20msec delay line
	
write	twrite dly,0		; tram writes
ready	tread dly,0		; tram reads
reada   tread dly,0
				
;;;The code:	
	
		
;;; two opcode sinewave generator (I love this chip!):
	macs  sinx,sinx,speed,cosx
	macs1 cosx,cosx,speed,sinx 

;;; 0.5Asint+0.5:	
	macs tmp,C_2^30,sinx,width
	
;;; calculate address:
	macs  ready.a,write.a,delay,tmp

;second addresses for interpolation:
;(interesting how the emu engineers decided that $800 wasn't a needed value) 
	macints reada.a,ready.a,C_8,C_256 
	
	
;;; output values:		
;;; 0x55 is 00100000 (?)
	macints tmp,C_0,reada.a,C_LSshift; get least significant part of address
	
	interp tmp2,ready,tmp,reada ;interpolate in-between the two delay line readings
	macs  out,in,tmp2,forward
	

;;; feedback and write to the delay line:
	
	macs  write,in,tmp2,feedback
		
	
	end





