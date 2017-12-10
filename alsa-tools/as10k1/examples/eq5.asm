	name  "5 band EQ"
	include "emu_constants.asm"

	
c0	con -0.98485626   0.98502633 0.99034926  -0.99034926
c1	con -0.95169465   0.95337028 0.93878619  -0.93878619
c2	con -0.84376963   0.85967945 0.84174451  -0.84174451
c3	con -0.47720462   0.61368058 0.73503304  -0.73503304
c4	con -0.28987550   0.11999291 0.72670869  -0.72670869
					
scalein sta  0.00013665  0.00134590  0.01265823  0.10000000  0.50000000
scaleout sta 420.00000000  140.00000000  50.00000000  20.00000000  10.00000000

in	io
out equ in		

F_100Hz control #0.2,0,#1	
F_316Hz control #0.1,0,#1	
F_1000Hz control #0.1,0,#1	
F_3160Hz control #0.1,0,#1	
F_10000Hz control #0.2,0,#1	
	
dly0 sta 0 0
dly1 sta 0 0
dly2 sta 0 0 
dly3 sta 0 0
dly4 sta  0 0	
				
out_tmp	dyn	
tmp2 dyn
tmp dyn
	
;;; Band Pass Filter Macro:
BPF	macro   OUT , IN , DELAY , COEF , SCALEIN , SCALEOUT , FOO , GAIN
		macs  tmp,C_0,SCALEIN,IN
		macs1  tmp,tmp,DELAY,FOO
 		macw1  tmp,tmp,DELAY,COEF
		macw1 tmp,tmp,DELAY+1,COEF+1
		macs	tmp2,C_0,DELAY+1,COEF+3
		macs	DELAY+1,DELAY,C_0,C_0
		macs	tmp2,tmp2,tmp,COEF+2
		macs	DELAY,tmp,C_0,C_0	
		macints  tmp2,C_0,tmp2,SCALEOUT
		macs	OUT,OUT,tmp2,GAIN
	endm

	
	macs out_tmp,C_0,C_0,C_0
	BPF  out_tmp,in,dly0,c0,scalein,scaleout,C_nmax,F_100Hz
	BPF  out_tmp,in,dly1,c1,scalein+1,scaleout+1,C_nmax,F_316Hz
	BPF  out_tmp,in,dly2,c2,scalein+2,scaleout+2,C_nmax,F_1000Hz
	BPF  out_tmp,in,dly3,c3,scalein+3,scaleout+3,C_nmax,F_3160Hz
	BPF  out_tmp,in,dly4,c4,scalein+4,scaleout+4,C_0,F_10000Hz
	macs out,out_tmp,C_0,C_0

	
	end



