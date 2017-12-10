;;; FV10K1 room relections code
;;; Generates room reflections according to file big.room.rp, and stores
;;; them in GPRs fvrefl_[fl|fr|rl|rr].
;;; Built on Sat Jan 24 13:18:00 2004 from room definition file big.room.rp.

  ; define delays and dampening constants
readr0 tread dlyr,&0.070249
readl0 tread dlyl,&0.070249
fl0 constant #-0.028783
fr0 constant #-0.011243
dmpstore_r0 sta 0,0,0
dmpstore_l0 sta 0,0,0
readr1 tread dlyr,&0.070249
readl1 tread dlyl,&0.070249
fl1 constant #-0.011243
fr1 constant #-0.028783
dmpstore_r1 sta 0,0,0
dmpstore_l1 sta 0,0,0
readr2 tread dlyr,&0.059064
readl2 tread dlyl,&0.059064
fl2 constant #0.054798
fr2 constant #0.021405
dmpstore_r2 sta 0,0
dmpstore_l2 sta 0,0
readr3 tread dlyr,&0.059064
readl3 tread dlyl,&0.059064
fl3 constant #0.021405
fr3 constant #0.054798
dmpstore_r3 sta 0,0
dmpstore_l3 sta 0,0
readr4 tread dlyr,&0.001778
readl4 tread dlyl,&0.001778
fl4 constant #-0.428866
fr4 constant #-0.167526
dmpstore_r4 sta 0
dmpstore_l4 sta 0
readr5 tread dlyr,&0.001778
readl5 tread dlyl,&0.001778
fl5 constant #-0.167526
fr5 constant #-0.428866
dmpstore_r5 sta 0
dmpstore_l5 sta 0
readr6 tread dlyr,&0.048126
readl6 tread dlyl,&0.048126
fl6 constant #-0.106417
fr6 constant #-0.041569
dmpstore_r6 sta 0
dmpstore_l6 sta 0
readr7 tread dlyr,&0.048126
readl7 tread dlyl,&0.048126
fl7 constant #-0.041569
fr7 constant #-0.106417
dmpstore_r7 sta 0
dmpstore_l7 sta 0
readr8 tread dlyr,&0.059064
readl8 tread dlyl,&0.059064
fl8 constant #0.054798
fr8 constant #0.021405
dmpstore_r8 sta 0,0
dmpstore_l8 sta 0,0
readr9 tread dlyr,&0.059064
readl9 tread dlyl,&0.059064
fl9 constant #0.021405
fr9 constant #0.054798
dmpstore_r9 sta 0,0
dmpstore_l9 sta 0,0
readr10 tread dlyr,&0.077324
readl10 tread dlyl,&0.077324
fr10 constant #-0.047167
rr10 constant #-0.002611
dmpstore_r10 sta 0,0,0
dmpstore_l10 sta 0,0,0
readr11 tread dlyr,&0.051184
readl11 tread dlyl,&0.051184
fr11 constant #0.103186
rr11 constant #0.012640
dmpstore_r11 sta 0,0
dmpstore_l11 sta 0,0
readr12 tread dlyr,&0.035389
readl12 tread dlyl,&0.035389
fr12 constant #0.160243
rr12 constant #0.008872
dmpstore_r12 sta 0,0
dmpstore_l12 sta 0,0
readr13 tread dlyr,&0.050379
readl13 tread dlyl,&0.050379
fr13 constant #-0.161577
rr13 constant #-0.019793
dmpstore_r13 sta 0
dmpstore_l13 sta 0
readr14 tread dlyr,&0.034414
readl14 tread dlyl,&0.034414
fr14 constant #-0.252993
rr14 constant #-0.014007
dmpstore_r14 sta 0
dmpstore_l14 sta 0
readr15 tread dlyr,&0.079794
readl15 tread dlyl,&0.079794
fr15 constant #0.065450
rr15 constant #0.008018
dmpstore_r15 sta 0,0
dmpstore_l15 sta 0,0
readr16 tread dlyr,&0.068240
readl16 tread dlyl,&0.068240
fr16 constant #0.085203
rr16 constant #0.004717
dmpstore_r16 sta 0,0
dmpstore_l16 sta 0,0
readr17 tread dlyr,&0.077324
readl17 tread dlyl,&0.077324
fr17 constant #-0.047167
rr17 constant #-0.002611
dmpstore_r17 sta 0,0,0
dmpstore_l17 sta 0,0,0


	;;; Accumulate reflections for FL
	interp dmpstore_l0+0,readl0,refldamp,dmpstore_l0+0
	interp dmpstore_l0+1,dmpstore_l0+0,refldamp,dmpstore_l0+1
	interp dmpstore_l0+2,dmpstore_l0+1,refldamp,dmpstore_l0+2
	interp dmpstore_r0+0,readr0,refldamp,dmpstore_r0+0
	interp dmpstore_r0+1,dmpstore_r0+0,refldamp,dmpstore_r0+1
	interp dmpstore_r0+2,dmpstore_r0+1,refldamp,dmpstore_r0+2
	interp dmpstore_r1+0,readr1,refldamp,dmpstore_r1+0
	interp dmpstore_r1+1,dmpstore_r1+0,refldamp,dmpstore_r1+1
	interp dmpstore_r1+2,dmpstore_r1+1,refldamp,dmpstore_r1+2
	interp dmpstore_l1+0,readl1,refldamp,dmpstore_l1+0
	interp dmpstore_l1+1,dmpstore_l1+0,refldamp,dmpstore_l1+1
	interp dmpstore_l1+2,dmpstore_l1+1,refldamp,dmpstore_l1+2
	interp dmpstore_l2+0,readl2,refldamp,dmpstore_l2+0
	interp dmpstore_l2+1,dmpstore_l2+0,refldamp,dmpstore_l2+1
	interp dmpstore_r2+0,readr2,refldamp,dmpstore_r2+0
	interp dmpstore_r2+1,dmpstore_r2+0,refldamp,dmpstore_r2+1
	interp dmpstore_r3+0,readr3,refldamp,dmpstore_r3+0
	interp dmpstore_r3+1,dmpstore_r3+0,refldamp,dmpstore_r3+1
	interp dmpstore_l3+0,readl3,refldamp,dmpstore_l3+0
	interp dmpstore_l3+1,dmpstore_l3+0,refldamp,dmpstore_l3+1
	interp dmpstore_l4+0,readl4,refldamp,dmpstore_l4+0
	interp dmpstore_r4+0,readr4,refldamp,dmpstore_r4+0
	interp dmpstore_r5+0,readr5,refldamp,dmpstore_r5+0
	interp dmpstore_l5+0,readl5,refldamp,dmpstore_l5+0
	interp dmpstore_l6+0,readl6,refldamp,dmpstore_l6+0
	interp dmpstore_r6+0,readr6,refldamp,dmpstore_r6+0
	interp dmpstore_r7+0,readr7,refldamp,dmpstore_r7+0
	interp dmpstore_l7+0,readl7,refldamp,dmpstore_l7+0
	interp dmpstore_l8+0,readl8,refldamp,dmpstore_l8+0
	interp dmpstore_l8+1,dmpstore_l8+0,refldamp,dmpstore_l8+1
	interp dmpstore_r8+0,readr8,refldamp,dmpstore_r8+0
	interp dmpstore_r8+1,dmpstore_r8+0,refldamp,dmpstore_r8+1
	interp dmpstore_r9+0,readr9,refldamp,dmpstore_r9+0
	interp dmpstore_r9+1,dmpstore_r9+0,refldamp,dmpstore_r9+1
	interp dmpstore_l9+0,readl9,refldamp,dmpstore_l9+0
	interp dmpstore_l9+1,dmpstore_l9+0,refldamp,dmpstore_l9+1
	interp dmpstore_l10+0,readl10,refldamp,dmpstore_l10+0
	interp dmpstore_l10+1,dmpstore_l10+0,refldamp,dmpstore_l10+1
	interp dmpstore_l10+2,dmpstore_l10+1,refldamp,dmpstore_l10+2
	interp dmpstore_r11+0,readr11,refldamp,dmpstore_r11+0
	interp dmpstore_r11+1,dmpstore_r11+0,refldamp,dmpstore_r11+1
	interp dmpstore_l12+0,readl12,refldamp,dmpstore_l12+0
	interp dmpstore_l12+1,dmpstore_l12+0,refldamp,dmpstore_l12+1
	interp dmpstore_r13+0,readr13,refldamp,dmpstore_r13+0
	interp dmpstore_l14+0,readl14,refldamp,dmpstore_l14+0
	interp dmpstore_r15+0,readr15,refldamp,dmpstore_r15+0
	interp dmpstore_r15+1,dmpstore_r15+0,refldamp,dmpstore_r15+1
	interp dmpstore_l16+0,readl16,refldamp,dmpstore_l16+0
	interp dmpstore_l16+1,dmpstore_l16+0,refldamp,dmpstore_l16+1
	interp dmpstore_l17+0,readl17,refldamp,dmpstore_l17+0
	interp dmpstore_l17+1,dmpstore_l17+0,refldamp,dmpstore_l17+1
	interp dmpstore_l17+2,dmpstore_l17+1,refldamp,dmpstore_l17+2
	macs C_0,C_0,C_0,C_0   ; reset accumulator
	macmv tmp,C_0,dmpstore_l0+2,fl0
	macmv tmp,C_0,dmpstore_r0+2,fr0
	macmv tmp,C_0,dmpstore_r1+2,fl1
	macmv tmp,C_0,dmpstore_l1+2,fr1
	macmv tmp,C_0,dmpstore_l2+1,fl2
	macmv tmp,C_0,dmpstore_r2+1,fr2
	macmv tmp,C_0,dmpstore_r3+1,fl3
	macmv tmp,C_0,dmpstore_l3+1,fr3
	macmv tmp,C_0,dmpstore_l4+0,fl4
	macmv tmp,C_0,dmpstore_r4+0,fr4
	macmv tmp,C_0,dmpstore_r5+0,fl5
	macmv tmp,C_0,dmpstore_l5+0,fr5
	macmv tmp,C_0,dmpstore_l6+0,fl6
	macmv tmp,C_0,dmpstore_r6+0,fr6
	macmv tmp,C_0,dmpstore_r7+0,fl7
	macmv tmp,C_0,dmpstore_l7+0,fr7
	macmv tmp,C_0,dmpstore_l8+1,fl8
	macmv tmp,C_0,dmpstore_r8+1,fr8
	macmv tmp,C_0,dmpstore_r9+1,fl9
	macmv tmp,C_0,dmpstore_l9+1,fr9
	macmv tmp,C_0,dmpstore_l10+2,fr10
	macmv tmp,C_0,dmpstore_r11+1,fr11
	macmv tmp,C_0,dmpstore_l12+1,fr12
	macmv tmp,C_0,dmpstore_r13+0,fr13
	macmv tmp,C_0,dmpstore_l14+0,fr14
	macmv tmp,C_0,dmpstore_r15+1,fr15
	macmv tmp,C_0,dmpstore_l16+1,fr16
	macs fvrefl_fl,ACCUM,dmpstore_l17+2,fr17

	;;; Accumulate reflections for FR
	interp dmpstore_l0+0,readl0,refldamp,dmpstore_l0+0
	interp dmpstore_l0+1,dmpstore_l0+0,refldamp,dmpstore_l0+1
	interp dmpstore_l0+2,dmpstore_l0+1,refldamp,dmpstore_l0+2
	interp dmpstore_r0+0,readr0,refldamp,dmpstore_r0+0
	interp dmpstore_r0+1,dmpstore_r0+0,refldamp,dmpstore_r0+1
	interp dmpstore_r0+2,dmpstore_r0+1,refldamp,dmpstore_r0+2
	interp dmpstore_r1+0,readr1,refldamp,dmpstore_r1+0
	interp dmpstore_r1+1,dmpstore_r1+0,refldamp,dmpstore_r1+1
	interp dmpstore_r1+2,dmpstore_r1+1,refldamp,dmpstore_r1+2
	interp dmpstore_l1+0,readl1,refldamp,dmpstore_l1+0
	interp dmpstore_l1+1,dmpstore_l1+0,refldamp,dmpstore_l1+1
	interp dmpstore_l1+2,dmpstore_l1+1,refldamp,dmpstore_l1+2
	interp dmpstore_l2+0,readl2,refldamp,dmpstore_l2+0
	interp dmpstore_l2+1,dmpstore_l2+0,refldamp,dmpstore_l2+1
	interp dmpstore_r2+0,readr2,refldamp,dmpstore_r2+0
	interp dmpstore_r2+1,dmpstore_r2+0,refldamp,dmpstore_r2+1
	interp dmpstore_r3+0,readr3,refldamp,dmpstore_r3+0
	interp dmpstore_r3+1,dmpstore_r3+0,refldamp,dmpstore_r3+1
	interp dmpstore_l3+0,readl3,refldamp,dmpstore_l3+0
	interp dmpstore_l3+1,dmpstore_l3+0,refldamp,dmpstore_l3+1
	interp dmpstore_l4+0,readl4,refldamp,dmpstore_l4+0
	interp dmpstore_r4+0,readr4,refldamp,dmpstore_r4+0
	interp dmpstore_r5+0,readr5,refldamp,dmpstore_r5+0
	interp dmpstore_l5+0,readl5,refldamp,dmpstore_l5+0
	interp dmpstore_l6+0,readl6,refldamp,dmpstore_l6+0
	interp dmpstore_r6+0,readr6,refldamp,dmpstore_r6+0
	interp dmpstore_r7+0,readr7,refldamp,dmpstore_r7+0
	interp dmpstore_l7+0,readl7,refldamp,dmpstore_l7+0
	interp dmpstore_l8+0,readl8,refldamp,dmpstore_l8+0
	interp dmpstore_l8+1,dmpstore_l8+0,refldamp,dmpstore_l8+1
	interp dmpstore_r8+0,readr8,refldamp,dmpstore_r8+0
	interp dmpstore_r8+1,dmpstore_r8+0,refldamp,dmpstore_r8+1
	interp dmpstore_r9+0,readr9,refldamp,dmpstore_r9+0
	interp dmpstore_r9+1,dmpstore_r9+0,refldamp,dmpstore_r9+1
	interp dmpstore_l9+0,readl9,refldamp,dmpstore_l9+0
	interp dmpstore_l9+1,dmpstore_l9+0,refldamp,dmpstore_l9+1
	interp dmpstore_r10+0,readr10,refldamp,dmpstore_r10+0
	interp dmpstore_r10+1,dmpstore_r10+0,refldamp,dmpstore_r10+1
	interp dmpstore_r10+2,dmpstore_r10+1,refldamp,dmpstore_r10+2
	interp dmpstore_l11+0,readl11,refldamp,dmpstore_l11+0
	interp dmpstore_l11+1,dmpstore_l11+0,refldamp,dmpstore_l11+1
	interp dmpstore_r12+0,readr12,refldamp,dmpstore_r12+0
	interp dmpstore_r12+1,dmpstore_r12+0,refldamp,dmpstore_r12+1
	interp dmpstore_l13+0,readl13,refldamp,dmpstore_l13+0
	interp dmpstore_r14+0,readr14,refldamp,dmpstore_r14+0
	interp dmpstore_l15+0,readl15,refldamp,dmpstore_l15+0
	interp dmpstore_l15+1,dmpstore_l15+0,refldamp,dmpstore_l15+1
	interp dmpstore_r16+0,readr16,refldamp,dmpstore_r16+0
	interp dmpstore_r16+1,dmpstore_r16+0,refldamp,dmpstore_r16+1
	interp dmpstore_r17+0,readr17,refldamp,dmpstore_r17+0
	interp dmpstore_r17+1,dmpstore_r17+0,refldamp,dmpstore_r17+1
	interp dmpstore_r17+2,dmpstore_r17+1,refldamp,dmpstore_r17+2
	macs C_0,C_0,C_0,C_0   ; reset accumulator
	macmv tmp,C_0,dmpstore_l0+2,fr0
	macmv tmp,C_0,dmpstore_r0+2,fl0
	macmv tmp,C_0,dmpstore_r1+2,fr1
	macmv tmp,C_0,dmpstore_l1+2,fl1
	macmv tmp,C_0,dmpstore_l2+1,fr2
	macmv tmp,C_0,dmpstore_r2+1,fl2
	macmv tmp,C_0,dmpstore_r3+1,fr3
	macmv tmp,C_0,dmpstore_l3+1,fl3
	macmv tmp,C_0,dmpstore_l4+0,fr4
	macmv tmp,C_0,dmpstore_r4+0,fl4
	macmv tmp,C_0,dmpstore_r5+0,fr5
	macmv tmp,C_0,dmpstore_l5+0,fl5
	macmv tmp,C_0,dmpstore_l6+0,fr6
	macmv tmp,C_0,dmpstore_r6+0,fl6
	macmv tmp,C_0,dmpstore_r7+0,fr7
	macmv tmp,C_0,dmpstore_l7+0,fl7
	macmv tmp,C_0,dmpstore_l8+1,fr8
	macmv tmp,C_0,dmpstore_r8+1,fl8
	macmv tmp,C_0,dmpstore_r9+1,fr9
	macmv tmp,C_0,dmpstore_l9+1,fl9
	macmv tmp,C_0,dmpstore_r10+2,fr10
	macmv tmp,C_0,dmpstore_l11+1,fr11
	macmv tmp,C_0,dmpstore_r12+1,fr12
	macmv tmp,C_0,dmpstore_l13+0,fr13
	macmv tmp,C_0,dmpstore_r14+0,fr14
	macmv tmp,C_0,dmpstore_l15+1,fr15
	macmv tmp,C_0,dmpstore_r16+1,fr16
	macs fvrefl_fr,ACCUM,dmpstore_r17+2,fr17

	;;; Accumulate reflections for RL
	interp dmpstore_l10+0,readl10,refldamp,dmpstore_l10+0
	interp dmpstore_l10+1,dmpstore_l10+0,refldamp,dmpstore_l10+1
	interp dmpstore_l10+2,dmpstore_l10+1,refldamp,dmpstore_l10+2
	interp dmpstore_r11+0,readr11,refldamp,dmpstore_r11+0
	interp dmpstore_r11+1,dmpstore_r11+0,refldamp,dmpstore_r11+1
	interp dmpstore_l12+0,readl12,refldamp,dmpstore_l12+0
	interp dmpstore_l12+1,dmpstore_l12+0,refldamp,dmpstore_l12+1
	interp dmpstore_r13+0,readr13,refldamp,dmpstore_r13+0
	interp dmpstore_l14+0,readl14,refldamp,dmpstore_l14+0
	interp dmpstore_r15+0,readr15,refldamp,dmpstore_r15+0
	interp dmpstore_r15+1,dmpstore_r15+0,refldamp,dmpstore_r15+1
	interp dmpstore_l16+0,readl16,refldamp,dmpstore_l16+0
	interp dmpstore_l16+1,dmpstore_l16+0,refldamp,dmpstore_l16+1
	interp dmpstore_l17+0,readl17,refldamp,dmpstore_l17+0
	interp dmpstore_l17+1,dmpstore_l17+0,refldamp,dmpstore_l17+1
	interp dmpstore_l17+2,dmpstore_l17+1,refldamp,dmpstore_l17+2
	macs C_0,C_0,C_0,C_0   ; reset accumulator
	macmv tmp,C_0,dmpstore_l10+2,rr10
	macmv tmp,C_0,dmpstore_r11+1,rr11
	macmv tmp,C_0,dmpstore_l12+1,rr12
	macmv tmp,C_0,dmpstore_r13+0,rr13
	macmv tmp,C_0,dmpstore_l14+0,rr14
	macmv tmp,C_0,dmpstore_r15+1,rr15
	macmv tmp,C_0,dmpstore_l16+1,rr16
	macs fvrefl_rl,ACCUM,dmpstore_l17+2,rr17

	;;; Accumulate reflections for RR
	interp dmpstore_r10+0,readr10,refldamp,dmpstore_r10+0
	interp dmpstore_r10+1,dmpstore_r10+0,refldamp,dmpstore_r10+1
	interp dmpstore_r10+2,dmpstore_r10+1,refldamp,dmpstore_r10+2
	interp dmpstore_l11+0,readl11,refldamp,dmpstore_l11+0
	interp dmpstore_l11+1,dmpstore_l11+0,refldamp,dmpstore_l11+1
	interp dmpstore_r12+0,readr12,refldamp,dmpstore_r12+0
	interp dmpstore_r12+1,dmpstore_r12+0,refldamp,dmpstore_r12+1
	interp dmpstore_l13+0,readl13,refldamp,dmpstore_l13+0
	interp dmpstore_r14+0,readr14,refldamp,dmpstore_r14+0
	interp dmpstore_l15+0,readl15,refldamp,dmpstore_l15+0
	interp dmpstore_l15+1,dmpstore_l15+0,refldamp,dmpstore_l15+1
	interp dmpstore_r16+0,readr16,refldamp,dmpstore_r16+0
	interp dmpstore_r16+1,dmpstore_r16+0,refldamp,dmpstore_r16+1
	interp dmpstore_r17+0,readr17,refldamp,dmpstore_r17+0
	interp dmpstore_r17+1,dmpstore_r17+0,refldamp,dmpstore_r17+1
	interp dmpstore_r17+2,dmpstore_r17+1,refldamp,dmpstore_r17+2
	macs C_0,C_0,C_0,C_0   ; reset accumulator
	macmv tmp,C_0,dmpstore_r10+2,rr10
	macmv tmp,C_0,dmpstore_l11+1,rr11
	macmv tmp,C_0,dmpstore_r12+1,rr12
	macmv tmp,C_0,dmpstore_l13+0,rr13
	macmv tmp,C_0,dmpstore_r14+0,rr14
	macmv tmp,C_0,dmpstore_l15+1,rr15
	macmv tmp,C_0,dmpstore_r16+1,rr16
	macs fvrefl_rr,ACCUM,dmpstore_r17+2,rr17

	END
