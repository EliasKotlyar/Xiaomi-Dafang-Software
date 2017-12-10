;;     fv-routes.asm - fv10k1 package
;;     Defines routing GPRs
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
;; $Id: fv-routes.inc,v 1.1 2001/09/28 01:56:20 dbertrand Exp $

  ; These registers are used to pass samples from the fv10k1 engine.
  ; fv10k1 attaches to two (R+L) input lines, and generates three sets
  ; of outputs: reverbs, reflections front, reflections rear.
  ; Separate output patches are responsible for mixing these outputs
  ; into their respective output lines

fvrev_r control #1,#0,#1
fvrev_l control #1,#0,#1

  ; Directional outputs of room reflection engine
fvrefl_fl control #1,#0,#1
fvrefl_rl control #0,#0,#1
fvrefl_fr control #1,#0,#1
fvrefl_rr control #0,#0,#1

  END
