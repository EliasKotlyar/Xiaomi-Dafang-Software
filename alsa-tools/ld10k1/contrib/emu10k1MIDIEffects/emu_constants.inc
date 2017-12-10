;some hardware constants C_[n]<DecimalValue>, 'n' indicates negative value
;		
;these are in 2's complement representation
 
C_0	equ $040	;;00000000
C_1	equ $041	;;00000001
C_2	equ $042	;;00000002
C_3	equ $043	;;00000003
C_4	equ $044	;;00000004	
C_8	equ $045	;;00000008
C_16	equ $046	;;00000010
C_32	equ $047	;;00000020
C_256	equ $048	;;00000100
C_65536 equ $049	;;00010000
C_2^23 	equ $04A	;;00080000
C_2^28	equ $04b	;;10000000
C_2^29	equ $04c	;;20000000 (C_max /4) +1
C_2^30	equ $04d	;;40000000 ( C_max / 2 ) + 1 (almost half)	
C_nmax	equ $04e	;;80000000 most negative number
C_max	equ $04f	;;7fffffff most positive number	
C_n1	equ $050	;;ffffffff -1
C_n2	equ $051	;;fffffffe -2
C_n2^30	equ $052	;;c0000000 C_nmax /2

C_LSshift equ $55	;;to left shift an address by using macsints
			;;for fractional addresses


ZERO equ C_0;
ONE equ	C_1;
	
;;; Hardware Registers:	
		
ACCUM	equ $56
CCR	equ $57	
NOISE1	equ $58
NOISE2	equ $59	
IRQ	equ $5A	
DBAC	equ $5B	
	
and	macro	dest,srcA,srcB
	andxor	dest,srcA,srcB,C_0
	endm
	
xor	macro   dest,srcA,srcB
	andxor	dest,C_n1,srcA,srcB
	endm
	
not	macro	 dest,src
	andxor   dest,src,C_n1,C_n1		
	endm

nand	macro	dest,srcA,srcB	
	andxor	dest,srcA,srcB,C_n1
	endm
	
or	macro	 dest,srcA,srcB
	not	 C_0,srcA
	andxor	 dest,ACCUM,srcA,srcB	
	endm	

nor	macro	dest,srcA,scrB
	not	dest,srcA
	andxor	dest,srcB,dest,srcA	
	not	dest,dest
	endm


neg     macro   dest,src
	macs1   dest,C_0,C_1,C_nmax 	
	endm	
	
;;; branch on:
;;; ==0
beq	macro   count
	skip	CCR,CCR,C_8,count	
	endm
;;; !=0
bne	macro	count
	skip	CCR,CCR,C_256,count
	endm
;;; <0
blt	macro	count
	skip	CCR,CCR,C_4,count
	endm
;;; always branch
bra	macro	count
	skip	C_0,C_max,C_max,count
	endm
;;; on saturation
bsa     macro count
	skip CCR,CCR,C_16,count
	endm
bge	macro  count
C___80	con $80
	skip CCR,CCR,C___80,count	
	endm
		
bgt	macro	count
C___180	con $180
	skip CCR,CCR,C___180,count
	endm
	
move	macro dest,src
	macs  dest,src,C_0,C_0
	endm	 	
	

;;; usefull for testing values before a skip
test	macro test
	macs C_0,test,C_0,C_0
	endm

cmp	macro src1,scr2
	macints C_0,src1,C_n1,src2
	endm

	end
