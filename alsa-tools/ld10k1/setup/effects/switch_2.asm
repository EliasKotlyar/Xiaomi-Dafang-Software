;PZU
    name "Switch 2 - channel"
    include "emu_constants.asm"
    
Left	IO
Right	IO

switchL control 0,0,1
switchR control 0,0,1

    macints Left, C_0, Left, switchL
    macints Right, C_0, Right, switchR
;    macs C_0, switchL, C_0, C_0
;    beq .left
;    macs Left, Left, C_0, C_0
;.left
;    macs C_0, switchR, C_0, C_0
;    beq .end
;    macs Right, Right, C_0, C_0
;.end
    end