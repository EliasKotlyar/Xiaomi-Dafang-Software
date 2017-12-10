;;; Simple Chorus
;;; Author:Daniel Bertrand
;;; Date: Oct 12, 2000

;;; This program is free software; you can redistribute it and/or modify  
;;; it under the terms of the GNU General Public License as published by  
;;; the Free Software Foundation; either version 2 of the License, or     
;;; (at your option) any later version.

;;; References:	
;;; http://www.harmony-central.com/Effects/Articles/Chorus

			
;;;  speed(formerly "delta")=2*pi*freq/48000
;;;  this give us our delta value for a specific freq (0.1-0.3Hz is good)
		

	include "emu_constants.asm"
	name "ChorusR"
		
in	IO
   
outr	equ in

	
speedR	  control  5e-05 , 0 , 1e-4 	; Controls frequency (radians)
delayR   control  &30e-3  ,&10e-3 , &80e-3; twice (2*) average delay (sec)
widthR	  control  #0.78   ,0	 ,#1	; width control 
mixR	  control  #1	 ,0      ,#1	; forward mix 

;; sine generator storage spaces:	
sinx  sta  0	
cosx  sta  #0.5
			
tmp  dyn 	
tmp2 dyn
	
;;; Two Delay Lines:	
	
	
dly   delay  &80e-3		;10msec delay line
	
write	twrite dly,0		; tram writes
ready	tread dly,0		; tram reads
reada   tread dly,0
				
;;;The code:	
	
		
;;; two opcode sinewave generator (I love this chip!):
	macs  sinx,sinx,speedR,cosx
	macs1 cosx,cosx,speedR,sinx 

;;; 0.5Asint+0.5:	
	macs tmp,C_2^30,sinx,widthR
	
;;; calculate address:
	macs  ready.a,write.a,delayR,tmp

;second addresses for interpolation:
;(interesting how the emu engineers decided that $800 wasn't a needed value) 
	macints reada.a,ready.a,C_8,C_256 

	
;;; Write to the delay line:
	
	macs  write,C_0,in,C_2^29
       	
		
	
;;; output values:		
;;; 0x55 is 00100000 (?)
	macints tmp,C_0,reada.a,C_LSshift; get least significant part of address
	
	interp tmp2,ready,tmp,reada ;interpolate in-between the two delay line readings
	
	macs  outr,tmp2,tmp2,mixR


		
	
	end





