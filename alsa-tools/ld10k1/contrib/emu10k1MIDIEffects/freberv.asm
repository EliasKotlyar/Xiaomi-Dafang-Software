;;     fv10k1.m4 - fv10k1 package
;;     This implements Jezar Wakefield's Freeverb algorithm
;;         
;;     Copyright (C) 2001 Oleg Smirnov <smirnov@astron.nl>
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
;; $Id: fv10k1.m4,v 1.1 2001/09/28 01:56:20 dbertrand Exp $

  name "Freeverb"
  
  include "emu_constants.inc"
  include "fv-routes.inc"
  include "fv-controls.inc"
  include "fv-basstreble.inc"
  
	; IO lines right/left
ior io
iol io
	

  ; No room reflection support - load full set of Freeverb filters and put 
  ; them into ITRAM first
  include "fv-filters.inc"
  ; delay lines for channel & reverb predelay will use XTRAM
dlyr delay &2   
dlyl delay &2   


writer twrite dlyr,0
writel twrite dlyl,0
oreadr tread dlyr,64     ; use 64 samples to avoid some TRAM glitches
oreadl tread dlyl,64
revreadr tread dlyr,64
revreadl tread dlyl,64
	
input dyn 1         ; wet reverb input [== (inl+inr)*gain ]

dly_b2	sta 0,0   ; storage for second bass/treble filter
dly_t2	sta 0,0


dly_b3	sta 0,0   
dly_t3	sta 0,0
dly_b4	sta 0,0   
dly_t4	sta 0,0

reflr dyn 2
refll dyn 2
reverbr dyn 1
reverbl dyn 1
ptr dyn 2
ptl dyn 2
  




  

  ;;; update TRAM read addresses from control GPRs
  acc3 oreadr.a,delay_r,writer.a,C_0
  acc3 oreadl.a,delay_l,writel.a,C_0
  acc3 revreadr.a,revdelay,writer.a,C_0
  acc3 revreadl.a,revdelay,writel.a,C_0

  ;;; init reverb outputs (and clear ACCUM for code below)
  macs fvrev_l,C_0,C_0,C_0
  macs fvrev_r,C_0,C_0,C_0

  ;;; accumulate reverb inputs ( predelayed R+L * revgain )
  ;;; and at the same time pass input to output w/delay
  macmv ior,oreadr,revreadr,revgain
  macs input,ACCUM,revreadl,revgain
	acc3 iol,oreadl,C_0,C_0
  acc3 writer,ior,C_0,C_0
  acc3 writel,iol,C_0,C_0

  ;;; apply & accumulate comb filters
  do_comb_filters fvrev_l,fvrev_r
  
  ;;; apply allpass filters
  do_allpass_filters fvrev_l,fvrev_r
  
  ;;; feed accumulated values to outputs, multiplying by wet & dry controls
  interp tmp,fvrev_l,wet1,fvrev_r
  interp tmpout,fvrev_r,wet1,fvrev_l
  macs fvrev_l,tmp,revreadl,dry
  macs fvrev_r,tmpout,revreadr,dry
  
  ;;; apply bass/treble controls to output
  test revdefeat
  bne .skipbasstreble
  basstreble fvrev_l,fvrev_l,revbass,revtreble,dly_b1,dly_t1
  basstreble fvrev_r,fvrev_r,revbass,revtreble,dly_b2,dly_t2
.skipbasstreble

  ;;; reset level meters at specified interval (use DBAC to track it)
  andxor tmp,DBAC,level_interval,C_0
  bne .skipreset
  acc3 maxlev_fr,C_0,C_0,C_0
  acc3 maxlev_fl,C_0,C_0,C_0
  acc3 maxlev_rr,C_0,C_0,C_0
  acc3 maxlev_rl,C_0,C_0,C_0
.skipreset
  

  ;;; apply reflection levels and bass/treble
  macs reflr,C_0,fvrefl_fr,refl_f
  macs refll,C_0,fvrefl_fl,refl_f
  
  macs reflr,reflr,fvrefl_rr,refl_f   ; two-speaker mode - add in other line
  macs refll,refll,fvrefl_rl,refl_f
  
  
  ;;; apply reverb levels
  macs  reverbr,C_0,fvrev_r,reverb_f
  macs  reverbl,C_0,fvrev_l,reverb_f
  

  

  ;;; write in+reverb_reflections to output    
  ;;; use macmv accumulation for extra precision
makeoutput MACRO io,refl,reverb,passthru
  macs C_0,C_0,C_0,C_0
  macmv tmp,tmp,refl,C_max
  
    macmv tmp,C_0,io,ptf_level
  
  macs io,ACCUM,reverb,C_max
  ENDM
  makeoutput ior,reflr,reverbr,ptr
  makeoutput iol,refll,reverbl,ptl
  
  ;;; maintain the maximum level 
maxlevel MACRO io,maxlev
  tstneg tmp,io,io,C_0             ; tmp = abs(io)
  limit maxlev,tmp,tmp,maxlev  ; maxlevel=max(tmp,maxlevel)
  ENDM
  maxlevel ior.o,maxlev_fr
  maxlevel iol.o,maxlev_fl
  
  END
