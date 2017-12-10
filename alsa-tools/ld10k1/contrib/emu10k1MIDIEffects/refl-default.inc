;;; FV10K1 room relections code
;;; Generates room reflections according to file default.room.rp, and stores
;;; them in GPRs fvrefl_[fl|fr|rl|rr].
;;; Built on Sat Jan 24 13:18:00 2004 from room definition file default.room.rp.

  ; define delays and dampening constants
readr0 tread dlyr,&0.003511
readl0 tread dlyl,&0.003511
fl0 constant #-0.371429
fr0 constant #-0.092857
dmpstore_r0 sta 0
dmpstore_l0 sta 0
readr1 tread dlyr,&0.003511
readl1 tread dlyl,&0.003511
fl1 constant #-0.092857
fr1 constant #-0.371429
dmpstore_r1 sta 0
dmpstore_l1 sta 0
readr2 tread dlyr,&0.006548
readl2 tread dlyl,&0.006548
fr2 constant #-0.508333
dmpstore_r2 sta 0
dmpstore_l2 sta 0
readr3 tread dlyr,&0.007252
readl3 tread dlyl,&0.007252
fl3 constant #-0.298947
fr3 constant #-0.074737
dmpstore_r3 sta 0
dmpstore_l3 sta 0
readr4 tread dlyr,&0.007252
readl4 tread dlyl,&0.007252
fl4 constant #-0.074737
fr4 constant #-0.298947
dmpstore_r4 sta 0
dmpstore_l4 sta 0
readr5 tread dlyr,&0.009261
readl5 tread dlyl,&0.009261
fr5 constant #0.270341
dmpstore_r5 sta 0,0
dmpstore_l5 sta 0,0
readr6 tread dlyr,&0.012327
readl6 tread dlyl,&0.012327
fr6 constant #0.240611
dmpstore_r6 sta 0,0
dmpstore_l6 sta 0,0
readr7 tread dlyr,&0.016174
readl7 tread dlyl,&0.016174
fr7 constant #-0.253287
rr7 constant #-0.015830
dmpstore_r7 sta 0
dmpstore_l7 sta 0
readr8 tread dlyr,&0.016690
readl8 tread dlyl,&0.016690
fl8 constant #0.105486
fr8 constant #0.026371
dmpstore_r8 sta 0,0
dmpstore_l8 sta 0,0
readr9 tread dlyr,&0.016690
readl9 tread dlyl,&0.016690
fl9 constant #0.026371
fr9 constant #0.105486
dmpstore_r9 sta 0,0
dmpstore_l9 sta 0,0
readr10 tread dlyr,&0.016690
readl10 tread dlyl,&0.016690
fl10 constant #0.105486
fr10 constant #0.026371
dmpstore_r10 sta 0,0
dmpstore_l10 sta 0,0
readr11 tread dlyr,&0.016690
readl11 tread dlyl,&0.016690
fl11 constant #0.026371
fr11 constant #0.105486
dmpstore_r11 sta 0,0
dmpstore_l11 sta 0,0
readr12 tread dlyr,&0.018195
readl12 tread dlyl,&0.018195
fr12 constant #0.147307
rr12 constant #0.009207
dmpstore_r12 sta 0,0
dmpstore_l12 sta 0,0
readr13 tread dlyr,&0.020577
readl13 tread dlyl,&0.020577
fr13 constant #-0.098203
dmpstore_r13 sta 0,0,0
dmpstore_l13 sta 0,0,0
readr14 tread dlyr,&0.020577
readl14 tread dlyl,&0.020577
fr14 constant #0.142194
rr14 constant #0.008887
dmpstore_r14 sta 0,0
dmpstore_l14 sta 0,0
readr15 tread dlyr,&0.020577
readl15 tread dlyl,&0.020577
fr15 constant #-0.098203
dmpstore_r15 sta 0,0,0
dmpstore_l15 sta 0,0,0
readr16 tread dlyr,&0.021591
readl16 tread dlyl,&0.021591
rl16 constant #-0.130920
rr16 constant #-0.071413
dmpstore_r16 sta 0
dmpstore_l16 sta 0
readr17 tread dlyr,&0.021591
readl17 tread dlyl,&0.021591
rl17 constant #-0.071413
rr17 constant #-0.130920
dmpstore_r17 sta 0
dmpstore_l17 sta 0
readr18 tread dlyr,&0.023355
readl18 tread dlyl,&0.023355
rl18 constant #0.078182
rr18 constant #0.042646
dmpstore_r18 sta 0,0
dmpstore_l18 sta 0,0
readr19 tread dlyr,&0.023355
readl19 tread dlyl,&0.023355
rl19 constant #0.042646
rr19 constant #0.078182
dmpstore_r19 sta 0,0
dmpstore_l19 sta 0,0


	;;; Accumulate reflections for FL
	interp dmpstore_l0+0,readl0,refldamp,dmpstore_l0+0
	interp dmpstore_r0+0,readr0,refldamp,dmpstore_r0+0
	interp dmpstore_r1+0,readr1,refldamp,dmpstore_r1+0
	interp dmpstore_l1+0,readl1,refldamp,dmpstore_l1+0
	interp dmpstore_l2+0,readl2,refldamp,dmpstore_l2+0
	interp dmpstore_l3+0,readl3,refldamp,dmpstore_l3+0
	interp dmpstore_r3+0,readr3,refldamp,dmpstore_r3+0
	interp dmpstore_r4+0,readr4,refldamp,dmpstore_r4+0
	interp dmpstore_l4+0,readl4,refldamp,dmpstore_l4+0
	interp dmpstore_l5+0,readl5,refldamp,dmpstore_l5+0
	interp dmpstore_l5+1,dmpstore_l5+0,refldamp,dmpstore_l5+1
	interp dmpstore_l6+0,readl6,refldamp,dmpstore_l6+0
	interp dmpstore_l6+1,dmpstore_l6+0,refldamp,dmpstore_l6+1
	interp dmpstore_r7+0,readr7,refldamp,dmpstore_r7+0
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
	interp dmpstore_r10+0,readr10,refldamp,dmpstore_r10+0
	interp dmpstore_r10+1,dmpstore_r10+0,refldamp,dmpstore_r10+1
	interp dmpstore_r11+0,readr11,refldamp,dmpstore_r11+0
	interp dmpstore_r11+1,dmpstore_r11+0,refldamp,dmpstore_r11+1
	interp dmpstore_l11+0,readl11,refldamp,dmpstore_l11+0
	interp dmpstore_l11+1,dmpstore_l11+0,refldamp,dmpstore_l11+1
	interp dmpstore_r12+0,readr12,refldamp,dmpstore_r12+0
	interp dmpstore_r12+1,dmpstore_r12+0,refldamp,dmpstore_r12+1
	interp dmpstore_l13+0,readl13,refldamp,dmpstore_l13+0
	interp dmpstore_l13+1,dmpstore_l13+0,refldamp,dmpstore_l13+1
	interp dmpstore_l13+2,dmpstore_l13+1,refldamp,dmpstore_l13+2
	interp dmpstore_r14+0,readr14,refldamp,dmpstore_r14+0
	interp dmpstore_r14+1,dmpstore_r14+0,refldamp,dmpstore_r14+1
	interp dmpstore_l15+0,readl15,refldamp,dmpstore_l15+0
	interp dmpstore_l15+1,dmpstore_l15+0,refldamp,dmpstore_l15+1
	interp dmpstore_l15+2,dmpstore_l15+1,refldamp,dmpstore_l15+2
	macs C_0,C_0,C_0,C_0   ; reset accumulator
	macmv tmp,C_0,dmpstore_l0+0,fl0
	macmv tmp,C_0,dmpstore_r0+0,fr0
	macmv tmp,C_0,dmpstore_r1+0,fl1
	macmv tmp,C_0,dmpstore_l1+0,fr1
	macmv tmp,C_0,dmpstore_l2+0,fr2
	macmv tmp,C_0,dmpstore_l3+0,fl3
	macmv tmp,C_0,dmpstore_r3+0,fr3
	macmv tmp,C_0,dmpstore_r4+0,fl4
	macmv tmp,C_0,dmpstore_l4+0,fr4
	macmv tmp,C_0,dmpstore_l5+1,fr5
	macmv tmp,C_0,dmpstore_l6+1,fr6
	macmv tmp,C_0,dmpstore_r7+0,fr7
	macmv tmp,C_0,dmpstore_l8+1,fl8
	macmv tmp,C_0,dmpstore_r8+1,fr8
	macmv tmp,C_0,dmpstore_r9+1,fl9
	macmv tmp,C_0,dmpstore_l9+1,fr9
	macmv tmp,C_0,dmpstore_l10+1,fl10
	macmv tmp,C_0,dmpstore_r10+1,fr10
	macmv tmp,C_0,dmpstore_r11+1,fl11
	macmv tmp,C_0,dmpstore_l11+1,fr11
	macmv tmp,C_0,dmpstore_r12+1,fr12
	macmv tmp,C_0,dmpstore_l13+2,fr13
	macmv tmp,C_0,dmpstore_r14+1,fr14
	macs fvrefl_fl,ACCUM,dmpstore_l15+2,fr15

	;;; Accumulate reflections for FR
	interp dmpstore_l0+0,readl0,refldamp,dmpstore_l0+0
	interp dmpstore_r0+0,readr0,refldamp,dmpstore_r0+0
	interp dmpstore_r1+0,readr1,refldamp,dmpstore_r1+0
	interp dmpstore_l1+0,readl1,refldamp,dmpstore_l1+0
	interp dmpstore_r2+0,readr2,refldamp,dmpstore_r2+0
	interp dmpstore_l3+0,readl3,refldamp,dmpstore_l3+0
	interp dmpstore_r3+0,readr3,refldamp,dmpstore_r3+0
	interp dmpstore_r4+0,readr4,refldamp,dmpstore_r4+0
	interp dmpstore_l4+0,readl4,refldamp,dmpstore_l4+0
	interp dmpstore_r5+0,readr5,refldamp,dmpstore_r5+0
	interp dmpstore_r5+1,dmpstore_r5+0,refldamp,dmpstore_r5+1
	interp dmpstore_r6+0,readr6,refldamp,dmpstore_r6+0
	interp dmpstore_r6+1,dmpstore_r6+0,refldamp,dmpstore_r6+1
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
	interp dmpstore_r10+0,readr10,refldamp,dmpstore_r10+0
	interp dmpstore_r10+1,dmpstore_r10+0,refldamp,dmpstore_r10+1
	interp dmpstore_r11+0,readr11,refldamp,dmpstore_r11+0
	interp dmpstore_r11+1,dmpstore_r11+0,refldamp,dmpstore_r11+1
	interp dmpstore_l11+0,readl11,refldamp,dmpstore_l11+0
	interp dmpstore_l11+1,dmpstore_l11+0,refldamp,dmpstore_l11+1
	interp dmpstore_l12+0,readl12,refldamp,dmpstore_l12+0
	interp dmpstore_l12+1,dmpstore_l12+0,refldamp,dmpstore_l12+1
	interp dmpstore_r13+0,readr13,refldamp,dmpstore_r13+0
	interp dmpstore_r13+1,dmpstore_r13+0,refldamp,dmpstore_r13+1
	interp dmpstore_r13+2,dmpstore_r13+1,refldamp,dmpstore_r13+2
	interp dmpstore_l14+0,readl14,refldamp,dmpstore_l14+0
	interp dmpstore_l14+1,dmpstore_l14+0,refldamp,dmpstore_l14+1
	interp dmpstore_r15+0,readr15,refldamp,dmpstore_r15+0
	interp dmpstore_r15+1,dmpstore_r15+0,refldamp,dmpstore_r15+1
	interp dmpstore_r15+2,dmpstore_r15+1,refldamp,dmpstore_r15+2
	macs C_0,C_0,C_0,C_0   ; reset accumulator
	macmv tmp,C_0,dmpstore_l0+0,fr0
	macmv tmp,C_0,dmpstore_r0+0,fl0
	macmv tmp,C_0,dmpstore_r1+0,fr1
	macmv tmp,C_0,dmpstore_l1+0,fl1
	macmv tmp,C_0,dmpstore_r2+0,fr2
	macmv tmp,C_0,dmpstore_l3+0,fr3
	macmv tmp,C_0,dmpstore_r3+0,fl3
	macmv tmp,C_0,dmpstore_r4+0,fr4
	macmv tmp,C_0,dmpstore_l4+0,fl4
	macmv tmp,C_0,dmpstore_r5+1,fr5
	macmv tmp,C_0,dmpstore_r6+1,fr6
	macmv tmp,C_0,dmpstore_l7+0,fr7
	macmv tmp,C_0,dmpstore_l8+1,fr8
	macmv tmp,C_0,dmpstore_r8+1,fl8
	macmv tmp,C_0,dmpstore_r9+1,fr9
	macmv tmp,C_0,dmpstore_l9+1,fl9
	macmv tmp,C_0,dmpstore_l10+1,fr10
	macmv tmp,C_0,dmpstore_r10+1,fl10
	macmv tmp,C_0,dmpstore_r11+1,fr11
	macmv tmp,C_0,dmpstore_l11+1,fl11
	macmv tmp,C_0,dmpstore_l12+1,fr12
	macmv tmp,C_0,dmpstore_r13+2,fr13
	macmv tmp,C_0,dmpstore_l14+1,fr14
	macs fvrefl_fr,ACCUM,dmpstore_r15+2,fr15

	;;; Accumulate reflections for RL
	interp dmpstore_r7+0,readr7,refldamp,dmpstore_r7+0
	interp dmpstore_r12+0,readr12,refldamp,dmpstore_r12+0
	interp dmpstore_r12+1,dmpstore_r12+0,refldamp,dmpstore_r12+1
	interp dmpstore_r14+0,readr14,refldamp,dmpstore_r14+0
	interp dmpstore_r14+1,dmpstore_r14+0,refldamp,dmpstore_r14+1
	interp dmpstore_l16+0,readl16,refldamp,dmpstore_l16+0
	interp dmpstore_r16+0,readr16,refldamp,dmpstore_r16+0
	interp dmpstore_r17+0,readr17,refldamp,dmpstore_r17+0
	interp dmpstore_l17+0,readl17,refldamp,dmpstore_l17+0
	interp dmpstore_l18+0,readl18,refldamp,dmpstore_l18+0
	interp dmpstore_l18+1,dmpstore_l18+0,refldamp,dmpstore_l18+1
	interp dmpstore_r18+0,readr18,refldamp,dmpstore_r18+0
	interp dmpstore_r18+1,dmpstore_r18+0,refldamp,dmpstore_r18+1
	interp dmpstore_r19+0,readr19,refldamp,dmpstore_r19+0
	interp dmpstore_r19+1,dmpstore_r19+0,refldamp,dmpstore_r19+1
	interp dmpstore_l19+0,readl19,refldamp,dmpstore_l19+0
	interp dmpstore_l19+1,dmpstore_l19+0,refldamp,dmpstore_l19+1
	macs C_0,C_0,C_0,C_0   ; reset accumulator
	macmv tmp,C_0,dmpstore_r7+0,rr7
	macmv tmp,C_0,dmpstore_r12+1,rr12
	macmv tmp,C_0,dmpstore_r14+1,rr14
	macmv tmp,C_0,dmpstore_l16+0,rl16
	macmv tmp,C_0,dmpstore_r16+0,rr16
	macmv tmp,C_0,dmpstore_r17+0,rl17
	macmv tmp,C_0,dmpstore_l17+0,rr17
	macmv tmp,C_0,dmpstore_l18+1,rl18
	macmv tmp,C_0,dmpstore_r18+1,rr18
	macmv tmp,C_0,dmpstore_r19+1,rl19
	macs fvrefl_rl,ACCUM,dmpstore_l19+1,rr19

	;;; Accumulate reflections for RR
	interp dmpstore_l7+0,readl7,refldamp,dmpstore_l7+0
	interp dmpstore_l12+0,readl12,refldamp,dmpstore_l12+0
	interp dmpstore_l12+1,dmpstore_l12+0,refldamp,dmpstore_l12+1
	interp dmpstore_l14+0,readl14,refldamp,dmpstore_l14+0
	interp dmpstore_l14+1,dmpstore_l14+0,refldamp,dmpstore_l14+1
	interp dmpstore_l16+0,readl16,refldamp,dmpstore_l16+0
	interp dmpstore_r16+0,readr16,refldamp,dmpstore_r16+0
	interp dmpstore_r17+0,readr17,refldamp,dmpstore_r17+0
	interp dmpstore_l17+0,readl17,refldamp,dmpstore_l17+0
	interp dmpstore_l18+0,readl18,refldamp,dmpstore_l18+0
	interp dmpstore_l18+1,dmpstore_l18+0,refldamp,dmpstore_l18+1
	interp dmpstore_r18+0,readr18,refldamp,dmpstore_r18+0
	interp dmpstore_r18+1,dmpstore_r18+0,refldamp,dmpstore_r18+1
	interp dmpstore_r19+0,readr19,refldamp,dmpstore_r19+0
	interp dmpstore_r19+1,dmpstore_r19+0,refldamp,dmpstore_r19+1
	interp dmpstore_l19+0,readl19,refldamp,dmpstore_l19+0
	interp dmpstore_l19+1,dmpstore_l19+0,refldamp,dmpstore_l19+1
	macs C_0,C_0,C_0,C_0   ; reset accumulator
	macmv tmp,C_0,dmpstore_l7+0,rr7
	macmv tmp,C_0,dmpstore_l12+1,rr12
	macmv tmp,C_0,dmpstore_l14+1,rr14
	macmv tmp,C_0,dmpstore_l16+0,rr16
	macmv tmp,C_0,dmpstore_r16+0,rl16
	macmv tmp,C_0,dmpstore_r17+0,rr17
	macmv tmp,C_0,dmpstore_l17+0,rl17
	macmv tmp,C_0,dmpstore_l18+1,rr18
	macmv tmp,C_0,dmpstore_r18+1,rl18
	macmv tmp,C_0,dmpstore_r19+1,rr19
	macs fvrefl_rr,ACCUM,dmpstore_l19+1,rl19

	END
