	name  "EQ10"
	include "emu_constants.asm"

;Piotr Tajdu¶ 
;gadrael@interia.pl

; 31 Hz
c0 con  9.9713475915e-02 1.4326204244e-04 1.9971183163e-01 
; 62 Hz
c1 con   9.9427771143e-02 2.8611442874e-04 1.9942120343e-01 
; 125 Hz
c2 con   9.8849666727e-02 5.7516663664e-04 1.9882304829e-01 
; 250 Hz
c3 con   9.7712566171e-02 1.1437169144e-03 1.9760670839e-01 
; 500 Hz
c4 con   9.5477456091e-02 2.2612719547e-03 1.9505892385e-01 
; 1k Hz
c5 con   9.1159452679e-02 4.4202736607e-03 1.8952405706e-01 
; 2k Hz
c6 con   8.3100647694e-02 8.4496761532e-03 1.7686164442e-01 
; 4k Hz
c7 con   6.9062328809e-02 1.5468835596e-02 1.4641227157e-01 
; 8k Hz
c8 con   4.7820368352e-02 2.6089815824e-02 7.3910184176e-02
; 16k Hz
c9 con   2.5620076154e-02 3.7189961923e-02 -6.2810038077e-02 



sco	sta 10.0

inl	io
toutl	equ inl
inr	io
toutr	equ inr
inrl	io
toutrl	equ inrl
inrr	io
toutrr	equ inrr
inc	io
toutc	equ inc
inlfe	io
toutlfe	equ inlfe

Equalizer control #0,0,#1
F0_31Hz control #0.5,0,#1
F1_62Hz control #0.5,0,#1	
F2_125Hz control #0.5,0,#1	
F3_250Hz control #0.5,0,#1	
F4_500Hz control #0.5,0,#1	
F5_1000Hz control #0.5,0,#1	
F6_2000Hz control #0.5,0,#1	
F7_4000Hz control #0.5,0,#1	
F8_8000Hz control #0.5,0,#1	
F9_16000Hz control #0.5,0,#1	
	
dly0 sta 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
dly1 sta 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
dly2 sta 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
dly3 sta 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
dly4 sta 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
dly5 sta 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0

out_tmp	dyn	
tmp2 dyn
tmp dyn
	
dlx0 sta 0 0
dlx1 sta 0 0
dlx2 sta 0 0
dlx3 sta 0 0
dlx4 sta 0 0
dlx5 sta 0 0


;;; Band Pass Filter Macro:
BPF	macro   OUT , IN , DELAY , DLXCB , COEF , GAIN
		macs1	tmp2,C_0,COEF+1,DLXCB+1
    		macs	tmp2,tmp2,COEF+1,IN
		macs	tmp2,tmp2,COEF+2,DELAY
		macs1	tmp2,tmp2,COEF,DELAY+1
		macs	DELAY+1,DELAY,C_0,C_0
		macints	DELAY,C_0,tmp2,sco
		macs	OUT,OUT,DELAY,GAIN
	endm

IIR	macro	OUT, IN , DLXC, DLYC
	    macs out_tmp,C_0,C_0,C_0
	    BPF  out_tmp,IN,DLYC,DLXC,c0,F0_31Hz 
	    BPF  out_tmp,IN,DLYC+2,DLXC,c1,F1_62Hz 	
	    BPF  out_tmp,IN,DLYC+4,DLXC,c2,F2_125Hz	
	    BPF  out_tmp,IN,DLYC+6,DLXC,c3,F3_250Hz	
	    BPF  out_tmp,IN,DLYC+8,DLXC,c4,F4_500Hz	
	    BPF  out_tmp,IN,DLYC+10,DLXC,c5,F5_1000Hz	
	    BPF  out_tmp,IN,DLYC+12,DLXC,c6,F6_2000Hz	
	    BPF  out_tmp,IN,DLYC+14,DLXC,c7,F7_4000Hz	
	    BPF  out_tmp,IN,DLYC+16,DLXC,c8,F8_8000Hz	
	    BPF  out_tmp,IN,DLYC+18,DLXC,c9,F9_16000Hz	
	    macs	DLXC+1,DLXC,C_0,C_0
	    macs	DLXC,IN,C_0,C_0
	    macs1	tmp,IN,IN,Equalizer
	    macs	tmp,tmp,out_tmp,Equalizer
	    macs	OUT,tmp,out_tmp,Equalizer
	endm

	IIR	toutl,inl,dlx0,dly0
	IIR	toutr,inr,dlx1,dly1
	IIR	toutrl,inrl,dlx2,dly2
	IIR	toutrr,inrr,dlx3,dly3
	IIR	toutc,inc,dlx4,dly4
	IIR	toutlfe,inlfe,dlx5,dly5

	end
