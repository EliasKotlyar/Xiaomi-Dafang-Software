;PZU
;;; Mikael Magnusson

    name "Mono Switch 2 channels"
    include "emu_constants.asm"
    
Left	IO
Right	IO

switch control 0,0,1

    macints Left, C_0, Left, switch
    macints Right, C_0, Right, switch
    end
