;;; Tremolo Effect 
;;; By:	 Daniel Bertrand
;;; Oct 29, 2000

	include "emu_constants.asm"
	name "tremolo"

in	IO
out	equ	in
	
	
;;; sinewave generator:
delta control 10e-4,0,1e-2 ; controls frequency (2*pi*freq/48000)
cosx sta #0.5		 
sinx sta 0		
depth control &0.001,0,&0.001


tmp  dyn	
	
delay	delay	&0.01
wrt	twrite	delay,0
rd	tread	delay,0	
rd2	tread	delay,0
c1000	sta	$1000
	
	macs	wrt,in,C_0,C_0
;;; sinwave generator:	
	macs  sinx,sinx,delta,cosx       
	macs1 cosx,cosx,delta,sinx 

;;; calulate address = depth*sin(wt)+0.5*depth
	
	
	macs tmp,c1000,depth,C_2^30
	macs tmp,tmp,sinx,depth
	acc3 rd.a,tmp,C_0,wrt.a	
	
	macints rd2.a,rd.a,C_8,C_256  ;;;next address 
	
;;; get fractional address:	
	macints tmp,C_0,rd.a,C_LSshift
;;; linear interpolate fraction between the 2 reads
;;; output result	
	
			
	interp  out,rd,tmp,rd2

	
	end
			
