	name "4-channel Gain"

;;; The next line controls the amount of gain (must be an integer)
gain	constant	4

	
signal1	io
signal2	io
signal3	io
signal4 io

 	
	macints	signal1, $40, signal1, gain ; signal= 0 + signal * gain
	macints	signal2, $40, signal2, gain
	macints	signal3, $40, signal3, gain
	macints	signal4, $40, signal4, gain


	end
