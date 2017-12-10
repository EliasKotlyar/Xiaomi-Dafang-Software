; Surround Active Matrix for Emu10k1
; Author: Robert Mazur <robertmazur@yahoo.com>
; Date: Jan 14, 2002
; Version 1.1

; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2 of the License, or
; (at your option) any later version.

;========================================

	name	"ProLogic"

	include	"emu_constants.asm"


;========================================

delline		delay	&0.02		; 0.02 sec delay
write		twrite	delline,&0	; write at 0 sec
read		tread	delline,&0.02	; read at 0.02 sec

;----------------------------------------
ml		con	#0.575997	; lpf 7000Hz
yl		sta	0

mlp		con	#0.277015	; lpf 2500Hz
mhp		con	#3.7076e-2	; hpf 300Hz
ylp		sta	0
shp		sta	0

;----------------------------------------
Lt		io			; Stereo Left  In
Rt		io			; Stereo Right In
L		equ Lt			; Front  Left  Out
R		equ Rt			; Front  Right Out
Ls		io
Rs		io
C		io			; Center
LFE		io			; LFE

;----------------------------------------
tmp		dyn

ll		dyn
rr		dyn

vcal		sta	#0.5
vcar		sta	#0.5

fl		sta	0
fr		sta	0
mf		con	#0.013  ; ~ 100Hz

;----------------------------------------
;	     abs(x)
tmp_abs	dyn
abs	macro ret, xx
	sub	tmp_abs, C_0, xx
	limit	ret, C_0, tmp_abs, xx
	endm

	
;========================================
;		Start
;========================================

;;	Servo

	fracmult ll, vcal, Lt		; ll = vcal * Lt
	abs	tmp, ll			; tmp = abs(ll)
	lpf	fl, mf, tmp		; fl = LowPass((n)Hz, tmp);

	fracmult rr, vcar, Rt		; rr = vcar * Rt
	abs	tmp, rr			; tmp = abs(rr)
	lpf	fr, mf, tmp		; fr = LowPass((n)Hz, tmp);
	
	intmult ll, C_2, ll		; vca0 = 0.5 so we must multiply 'll' and 'rr' by 2
	intmult rr, C_2, rr

	sub	tmp, fr, fl		; serv = fr - fl
	
	macints	vcal, C_2^30, C_2, tmp	;vcal = vca0 + 2*serv
	macints vcar, C_2^30, C_n2, tmp	;vcar = vca0 - 2*serv
	
;;	Suround
	
	sub	tmp, ll, rr		; delay.in = L - R
	
	lpf	yl, ml, tmp 		; yl = LowPass(7kHz, delay.out) = rear
	
;	macs	L, Lt, vcar, yl		; L = Lt - vcar * S  Remove Surround from front speakers
;	macs1	R, Rt, vcal, yl		; R = Rt + vcal * S	
	
	move	write, yl		; delay surround

	fracmult tmp, vcar, read	; Ls = 2 * vcar * rear ( 2* becouse vca0 = 0.5)
	intmult	Ls,C_2,tmp
	fracmult tmp, vcal, read	; Rs = 2 * vcal * rear
	intmult Rs,C_2,tmp
	
;;	Center

	add	tmp, ll, rr		; tmp = L + R

	hpf	tmp, shp, mhp, tmp	; tmp = HighPass(300Hz, tmp)
	lpf	ylp, mlp, tmp		; ylp = LowPass(2.5kHz, tmp) = center

	move	C, ylp			; Center

	sub	R, Rt, read		; R = R - rear
	sub	L, Lt, read  		; L = L - rear

	add	LFE, Lt, Rt		; tmp = Lt + Rt
	lpf	LFE, mhp, tmp		; LFE = LowPass((n)Hz, tmp)

	end
;========================================
