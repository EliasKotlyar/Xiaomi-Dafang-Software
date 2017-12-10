;;     fv-basstreble.m4 - fv10k1 package
;;     Defines macros for bass/treble filters
;; 
;;     This is wholly derived from Daniel Bertrand's tone.asm in the emu10k1 
;;     driver (see emu10k1/utils/as10k1/effects/tone.asm).
;;    
;;     This program is free software; you can redistribute it and/or modify
;;     it under the terms of the GNU General Public License as published by
;;     the Free Software Foundation; either version 2 of the License, or
;;     (at your option) any later version.
;; 
;;     This program is distributed in the hope that it will be useful,
;;     but WITHOUT ANY WARRANTY; without even the implied warranty of
;;     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;     GNU General Public License for more details.
;; 
;;     You should have received a copy of the GNU General Public License
;;     along with this program; if not, write to the Free Software
;;     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
;; 
;; $Id: fv-basstreble.inc,v 1.1 2001/09/28 01:56:20 dbertrand Exp $

;;; a and b coefs for bass:	


b_b	con   2.736129417e-01    5.240710533e-01    2.620355267e-01
a_b	con   9.560258858e-01    -4.576868881e-01
	
;;; a and b coef for treble:	
b_t con		-4.982305773e-01   9.964611547e-01    -4.982305773e-01
a_t con 	 9.317583774e-01    -4.356836381e-01
	
scalein  con 2.449e-05, 1.157407407e-04
scaleout con 128, 16192

dly_b1	sta 0,0
dly_t1	sta 0,0

tmp dyn 1     ; miscellaneous temp storage
tmpout dyn 1

;;; end of bass/treble definitions

;;;
;;; This macro applies the bass/treble controls (based on eq2.asm)
;;;
basstreble MACRO dest,source,tone_bass,tone_treble,dly_b,dly_t
	;;; tone_bass filter(iir):
	macw	tmp, C_0,  dly_b+1,  a_b+1
	macw	tmp, tmp,  dly_b  ,  a_b
	macw	tmp,tmp,source,scalein
	macints	tmp, C_0, tmp, C_2
  
	macs	C_0,C_0,C_0,C_0
	macmv	dly_b+1,dly_b,	dly_b+1, b_b+2
	macmv	dly_b,tmp,	dly_b,   b_b+1
	macw	tmp,ACCUM,	tmp,     b_b	
	macs	tmp,C_0,tone_bass,tmp
	macints	tmpout,C_0,tmp,scaleout

  ;;; tone_treble
	macw	tmp, C_0,  dly_t+1,  a_t+1 	
	macw	tmp, tmp,  dly_t  ,  a_t
	macw	tmp, tmp, source,scalein+1
	macints	tmp,C_0,tmp,C_2
	
	macs	C_0,C_0,C_0,C_0
	macmv	dly_t+1,dly_t,	dly_t+1, b_t+2
	macmv	dly_t,tmp,	dly_t,   b_t+1
	macw	tmp,ACCUM,	tmp,     b_t	
	macs	tmp,C_0,tone_treble,tmp
  
	macints	dest,tmpout,tmp,scaleout+1
  
  ENDM

;;;
;;; This macro applies the bass/treble controls (based on eq2.asm)
;;; and stores results into two separate GPRs
;;;
basstreblesep MACRO dest_b,dest_t,source,tone_bass,tone_treble,dly_b,dly_t
	;;; tone_bass filter(iir):
	macw	tmp, C_0,  dly_b+1,  a_b+1
	macw	tmp, tmp,  dly_b  ,  a_b
	macw	tmp,tmp,source,scalein
	macints	tmp, C_0, tmp, C_2
  
	macs	C_0,C_0,C_0,C_0
	macmv	dly_b+1,dly_b,	dly_b+1, b_b+2
	macmv	dly_b,tmp,	dly_b,   b_b+1
	macw	tmp,ACCUM,	tmp,     b_b	
	macs	tmp,C_0,tone_bass,tmp
	macints	tmpout,C_0,tmp,scaleout

  ;;; tone_treble
	macw	tmp, C_0,  dly_t+1,  a_t+1 	
	macw	tmp, tmp,  dly_t  ,  a_t
	macw	tmp, tmp, source,scalein+1
	macints	tmp,C_0,tmp,C_2
	
	macs	C_0,C_0,C_0,C_0
	macmv	dly_t+1,dly_t,	dly_t+1, b_t+2
	macmv	dly_t,tmp,	dly_t,   b_t+1
	macw	tmp,ACCUM,	tmp,     b_t	
	macs	tmp,C_0,tone_treble,tmp
  
	macints	dest_t,C_0,tmp,scaleout+1
  acc3    dest_b,tmpout,C_0,C_0
  
  ENDM

  END
