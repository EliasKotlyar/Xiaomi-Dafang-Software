;PZU
;parts are taken from passthrough-audigy

	name "Output patch"
	include "emu_constants.asm"

;inputs - and analog outputs
Left		IO
Right		IO
LeftSurr	IO
RightSurr	IO
Center		IO
LFE		IO
; these are used for digital output
DLeft		IO
DRight		IO
DLeftSurr	IO
DRightSurr	IO
DCenter		IO
DLFE		IO

enableL		control 0,0,1
enableR		control 0,0,1

tmp_a		dyn
tmp_b		dyn
mask		con $ffff0000


;simple copy to analog output
    macs Left, Left, C_0, C_0
    macs Right, Right, C_0, C_0
    macs LeftSurr, LeftSurr, C_0, C_0
    macs RightSurr, RightSurr, C_0, C_0
    macs Center, Center, C_0, C_0
    macs LFE, LFE, C_0, C_0
;
    macs DLeft, Left, C_0, C_0
    macs DRight, Right, C_0, C_0
    macs C_0, enableL, C_0, C_0
    beq .endL
    macs tmp_a, DLeft, C_0, C_0
    ble .next_a
    acc3 tmp_a, C_0, C_65536, tmp_a
.next_a
    and DLeft, tmp_a, mask
.endL
    macs C_0, enableR, C_0, C_0
    beq .end
    macs tmp_b, DRight, C_0, C_0
    ble .next_b
    acc3 tmp_b, C_0, C_65536, tmp_b
.next_b
    and DRight, tmp_b, mask
.end
    macs DLeftSurr, LeftSurr, C_0, C_0
    macs DRightSurr, RightSurr, C_0, C_0
    macs DCenter, Center, C_0, C_0
    macs DLFE, LFE, C_0, C_0

    end