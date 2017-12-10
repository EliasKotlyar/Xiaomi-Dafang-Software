;;; written by:	 Daniel Bertrand <d.bertrand@ieee.ca>
	
	include "emu_constants.asm"
	name "Vibro Effect"

in	io
out	equ in
			

		
;;; sinewave generator:
delta control 1.5e-3,0,1e-2 ; controls frequency (2*pi*freq/48000)
cosx sta #0.5		 
sinx sta 0	
depth control #1,0,#1
sin2 dyn 1

	
	macs  sinx,sinx,delta,cosx       
	macs1 cosx,cosx,delta,sinx 
;; depth control (and add 0.5 DC offset):
	macs sin2,C_2^30,sinx,depth
	
;;; multiply signals by sinewave
	macs out,C_0,in,sin2
	end
	




