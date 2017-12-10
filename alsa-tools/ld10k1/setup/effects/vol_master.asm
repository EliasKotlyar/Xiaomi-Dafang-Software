	name "Master Vol"

VolM control  0,0,100
;VolM control  100,0,#1
	
left	IO
right	IO
rleft	IO
rright	IO
center	IO
lfe	IO
	
	macs  left.o,$40,left,VolM
	macs  right.o,$40,right,VolM
	macs  rleft.o,$40,rleft,VolM
	macs  rright.o,$40,rright,VolM
	macs  center.o,$40,center,VolM
	macs  lfe.o,$40,lfe,VolM

	end
	

	
