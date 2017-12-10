;;; Bass and Treble Effect
;;; By:	   Daniel Bertrand
;;; Date:  Dec 19th,200
;;; License: GPL v2
;;; 
	name "Eq2"
	include "emu_constants.asm"

;;; a and b coefs for bass:	
b_b	con   2.736129417e-01    5.240710533e-01    2.620355267e-01
a_b	con   9.560258858e-01    -4.576868881e-01
	
;;; a and b coef for treble:	
b_t con		-4.982305773e-01   9.964611547e-01    -4.982305773e-01
a_t con 	 9.317583774e-01    -4.356836381e-01
	
scalein   con   2.449e-05, 1.157407407e-04
scaleout con 128, 16192
		
bass   control 0.25,#0,#1
treble control 0.25,#0,#1
	
in	IO
out	equ in
	
tmp	dyn
tmpout	dyn	
		
dly_b	sta 0,0
dly_t	sta 0,0
			
	
	;;; bass filter(iir):

	macw	tmp, C_0,  dly_b+1,  a_b+1
	macw	tmp, tmp,  dly_b  ,  a_b
	macw	tmp,tmp,in,scalein
	macints	tmp, C_0, tmp, C_2
	
	macs	C_0,C_0,C_0,C_0
	
	macmv	dly_b+1,dly_b,	dly_b+1, b_b+2
	macmv	dly_b,tmp,	dly_b,   b_b+1
	macw	tmp,ACCUM,	tmp,     b_b	
	
	
	macs	tmp,C_0,bass,tmp
	macints	tmpout,C_0,tmp,scaleout

;;; treble
	

	macw	tmp, C_0,  dly_t+1,  a_t+1 	
	macw	tmp, tmp,  dly_t  ,  a_t
	macw	tmp, tmp, in,scalein+1
	macints	tmp,C_0,tmp,C_2
	
	macs	C_0,C_0,C_0,C_0
	
	macmv	dly_t+1,dly_t,	dly_t+1, b_t+2
	macmv	dly_t,tmp,	dly_t,   b_t+1
	macw	tmp,ACCUM,	tmp,     b_t	
	
	macs	tmp,C_0,treble,tmp
	macints	out,tmpout,tmp,scaleout+1

	
	end
	
