;PZU
	name "Simple 5.1 for Wave"
	include "emu_constants.asm"

inl	io
outl	equ  inl
inr	io
outr	equ  inr
inrl	io
outrl	equ  inrl
inrr	io
outrr	equ  inrr
inc	io
outc	equ  inc
inlfe	io
outlfe	equ  inlfe

Left control 0,0,100
Right control 0,0,100
LeftSurr control 0,0,100
RightSurr control 0,0,100
Center control 0,0,100
LFE control 0,0,100

tmp dyn
c40 con $40000000


tmpl dyn
tmpr dyn

;5.1 playback
	macs  tmpl, inl, C_0, C_0
	macs  tmpr, inr, C_0, C_0

;	macs  outl, inl.o, Left, tmpl
;	macs  outr, inr.o, Right, tmpr
;	macs  outrl, inrl.o, LeftSurr, tmpl
;	macs  outrr, inrr.o, RightSurr, tmpr
;	interp tmp, tmpl, c40, tmpr
;	macs  outc, inc.o, Center, tmp
;	macs  outlfe, inlfe.o, LFE, tmp

	macs  outl, $40, Left, tmpl
	macs  outr, $40, Right, tmpr
	macs  outrl, $40, LeftSurr, tmpl
	macs  outrr, $40, RightSurr, tmpr
	interp tmp, tmpl, c40, tmpr
	macs  outc, $40, Center, tmp
	macs  outlfe, $40, LFE, tmp

	end








