	name "2-channel Vol"

Left control  0,0,100
Right control  0,0,100
	
signal_l	IO
signal_r	IO
	
	macs  signal_l,$40,signal_l,Left
	macs  signal_r,$40,signal_r,Right

		end
	

	
