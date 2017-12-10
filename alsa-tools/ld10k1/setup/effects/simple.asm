	name "Simple 5.1 volume"
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

	macs  outl,C_0, Left, inl
	macs  outr,C_0, Right, inr
	macs  outc,C_0, Center, inc
	macs  outrl,C_0, LeftSurr, inrl
	macs  outrr,C_0, RightSurr, inrr
	macs  outlfe,C_0, LFE, inlfe
	
	end








