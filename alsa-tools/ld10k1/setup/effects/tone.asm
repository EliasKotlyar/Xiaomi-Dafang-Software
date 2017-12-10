; stolen from alsa-driver

	name "Tone - Bass, Treble"
	include "emu_constants.asm"

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

; Tone Control - Bass
bass0	control 20, 0, 40
bass1	control 20, 0, 40
bass2	control 20, 0, 40
bass3	control 20, 0, 40
bass4	control 20, 0, 40

; Tone Control - Treble
treble0	control 20, 0, 40
treble1	control 20, 0, 40
treble2	control 20, 0, 40
treble3	control 20, 0, 40
treble4	control 20, 0, 40

; Tone Control - Switch
toneonoff	control 0, 0, 1

; temporary
templb	sta 0, 0, 0, 0, 0
templt	sta 0, 0, 0, 0, 0
temprb	sta 0, 0, 0, 0, 0
temprt	sta 0, 0, 0, 0, 0
temprlb	sta 0, 0, 0, 0, 0
temprlt	sta 0, 0, 0, 0, 0
temprrb	sta 0, 0, 0, 0, 0
temprrt	sta 0, 0, 0, 0, 0
tempcb	sta 0, 0, 0, 0, 0
tempct	sta 0, 0, 0, 0, 0

outl	dyn
outr	dyn
outrl	dyn
outrr	dyn
outc	dyn
outlfe	dyn

tmp	sta 0, 0

BT	macro   tempb, tempt, chn
	macs C_0, C_0, chn, bass0
	macmv tempb+1, tempb, tempb+1, bass2
	macmv tempb, chn, tempb, bass1
	macmv tempb+3, tempb+2, tempb+3, bass4
	macs tempb+2, ACCUM, tempb+2, bass3
	acc3 tempb+2, tempb+2, tempb+2, C_0

	macs C_0, C_0, tempb+2, treble0
	macmv tempt+1, tempt, tempt+1, treble2
	macmv tempt, tempb+2, tempt, treble1
	macmv tempt+3, tempt+2, tempt+3, treble4
	macs tempt+2, ACCUM, tempt+2, treble3
	macints tempt+2, C_0, tempt+ 2, C_16

	acc3 chn, tempt+2, C_0, C_0
	endm

SONOFF	macro out, in
	macints tmp, C_0, out, toneonoff
	andxor tmp+1, toneonoff, C_1, C_1
	macints tmp+1, C_0, in, tmp+1
	acc3 out, tmp, tmp+1, C_0
	endm

;Process tone control
	macs outl, inl, C_0, C_0
	macs outr, inr, C_0, C_0
	macs outrl, inrl, C_0, C_0
	macs outrr, inrr, C_0, C_0
	macs outc, inc, C_0, C_0
	macs outlfe, inlfe, C_0, C_0

	BT templb, templt, outl
	BT temprb, temprt, outr
	BT temprlb, temprlt, outrl
	BT temprrb, temprrt, outrr
	BT tempcb, tempct, outc

	SONOFF outl, inl
	SONOFF outr, inr
	SONOFF outrl, inrl
	SONOFF outrr, inrr
	SONOFF outc, inc
	SONOFF outlfe, inlfe
	
	macs toutl, outl, C_0, C_0
	macs toutr, outr, C_0, C_0
	macs toutrl, outrl, C_0, C_0
	macs toutrr, outrr, C_0, C_0
	macs toutc, outc, C_0, C_0
	macs toutlfe, outlfe, C_0, C_0

	end