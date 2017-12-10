;;     fv-controls.asm - fv10k1 package
;;     Defines control GPRs for the fv10k1 package
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
;; $Id: fv-controls.inc,v 1.1 2001/09/28 01:56:20 dbertrand Exp $

  ; debugging registers
;debug control #0,#0,#1
;debug2 control #0,#0,#1
;debug3 control #0,#0,#1
;debug4 control #0,#0,#1


	; This delay is applied to the input before passing it through
	; to the output. Note that this is independent of the room reflections 
	; or reverb delays, and mainly intended to compensate for pathlength 
	; differential in an environment where the speakers are not equidistant 
	; from the listening position (i.e. a car!). 1/Vsound is about 3 ms/m.
delay_r control &0,&0,&0.8
delay_l control &0,&0,&0.8

  ; high-frequency damping for room reflections
refldamp control #0,#0,#1

  ; delay for start of reverbrations. Note that this control is ignored
  ; when using input patches with a room reflection engine -- there we use
  ; a constant revdelay instead.
revdelay control &.002,&0,&1

  ; gains for "wet" reverbrations
revgain control #0.10,#0,#1

  ; room size parameter, 0-1. Bigger values produce deeper reverb.
roomsize control #.90,#0,#1

  ; room dampening 0-1
damp control #.2,#0,#1

  ; allpass filter feedback -- normally constant at .5
allpassfeed control #.2,#0,#1

  ; "wet1" level controls the amount of left/right reverb intermixing:
  ;   L = L*(1-wet1) + R*wet1
  ;   R = R*(1-wet1) + L*wet1 
  ; (original Freeverb uses wet1 and wet2, computed from "wet" and "width":
  ;     wet1 = wet*(width+1)/2
  ;     wet2 = wet*(1-width)/2 )
wet1 control #0,#0,#1

  ; "dry" reverb level:
  ; output = reverb + input*dry
  ; Note that this is somewhat different from the "passthru" level, since
  ; reverb bass/treble [and predelay, if any] are applied to the dry signal,
  ; but not to the passthru signal
dry control #0,#0,#1

  ; bass and treble controls for the reverb engine
revbass   control #0,#0,#1
revtreble control #0,#0,#1
  ; set to 1 to skip reverb bass/treble filters
revdefeat control #1,#0,#1

  ; fractional levels of reflections for F and R speakers
refl_f control #0,#0,#1
refl_r control #0,#0,#1
  ; additional integer gains for rear reflections
refl_rgain control #0,#0,#1

  
  ; fractional levels of reverbs for F and R speakers
reverb_f control #0.8,#0,#1
reverb_r control #0.0,#0,#1
  ; additional integer gains for rear reverbs
reverb_rgain control #0,#0,#1

  ; passthru controls (separate for front/rear)
  ; output = input*passthru + reverbs + reflections
ptf_level control #1,#0,#1
ptr_level control #0,#0,#1
  ; additional integer gains for rear passthru
ptr_gain control #0,#0,#1

  ; separate bass/treble controls are available for the rear passthrus
ptr_bass control #0,#0,#1
ptr_treble control #0,#0,#1
ptr_defeat control #0,#0,1

  ; Max output level meters. Output patches update these GPRs with the 
  ; maximum output level encountered.
maxlev_fr control #1,#0,#1
maxlev_fl control #1,#0,#1
maxlev_rr control #.0,#0,#1
maxlev_rl control #.0,#0,#1

  ; interval at which level meters are reset. For an interval of 2N samples,
  ; it should be set to (2^N-1)*$800.
  ; The DBAC register is ANDed by this value, and if the result is 0,
  ; the meters are reset. $800 corresponds to 1 sample.
  ; $0efff00 is  4096 samples
  ; $0fffE00 is  8192 samples
  ; $1fffC00 is 16384 samples
  ; $3fff800 is 32768 samples
  ; $7fff800 is 65536 samples
level_interval control $1fffC00,$0,$8000000

  END
