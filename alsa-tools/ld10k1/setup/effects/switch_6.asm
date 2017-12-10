;PZU
    name "Switch 6 - channel"
    include "emu_constants.asm"
    
Left		IO
Right		IO
LeftSurr	IO
RightSurr	IO
Center		IO
LFE		IO

switch control 0,0,1

    macints Left, C_0, Left, switch
    macints Right, C_0, Right, switch
    macints LeftSurr, C_0, LeftSurr, switch
    macints RightSurr, C_0, RightSurr, switch
    macints Center, C_0, Center, switch
    macints LFE, C_0, LFE, switch
;    macs C_0, switch, C_0, C_0
;    beq .end
;    macs Left, Left, C_0, C_0
;    macs Right, Right, C_0, C_0
;    macs LeftSurr, LeftSurr, C_0, C_0
;    macs RightSurr, RightSurr, C_0, C_0
;    macs Center, Center, C_0, C_0
;    macs LFE, LFE, C_0, C_0
;.end
    end