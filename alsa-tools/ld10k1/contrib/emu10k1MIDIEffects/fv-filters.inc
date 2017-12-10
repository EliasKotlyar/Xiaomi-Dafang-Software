;;     fv-filters.m4 - fv10k1 package
;;     defines delay lines and GPRs for Freeverb's comb and allpass filters
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
;; $Id: fv-filters.m4,v 1.1 2001/09/28 01:56:20 dbertrand Exp $

















;;; macro to apply & accumulate a comb filter
comb MACRO dest,rdelay,wdelay,filterstore
  acc3 dest,dest,rdelay,C_0
  interp filterstore,rdelay,damp,filterstore
  macs wdelay,input,filterstore,roomsize
  endm

;;; macro to apply an allpass filter
allpass MACRO dest,rdelay,wdelay
  macs wdelay,dest,rdelay,allpassfeed
  macs1 dest,rdelay,dest,C_max
  endm






;
;;; Freeverb full mode - allpass filter delay lines go ahead, since
;;; wed rather use internal TRAM. Hence shorter delays  go first.
;;; 4 allpass filters

apl1 delay 626
apr1 delay 651
wapl1 twrite apl1,0
wapr1 twrite apr1,0
rapl1 tread apl1,606
rapr1 tread apr1,631



apl2 delay 500
apr2 delay 525
wapl2 twrite apl2,0
wapr2 twrite apr2,0
rapl2 tread apl2,480
rapr2 tread apr2,505



apl3 delay 391
apr3 delay 416
wapl3 twrite apl3,0
wapr3 twrite apr3,0
rapl3 tread apl3,371
rapr3 tread apr3,396



apl4 delay 265
apr4 delay 290
wapl4 twrite apl4,0
wapr4 twrite apr4,0
rapl4 tread apl4,245
rapr4 tread apr4,270


do_allpass_filters MACRO l,r
  allpass l,rapl1,wapl1
  allpass l,rapl2,wapl2
  allpass l,rapl3,wapl3
  allpass l,rapl4,wapl4
  allpass r,rapr1,wapr1
  allpass r,rapr2,wapr2
  allpass r,rapr3,wapr3
  allpass r,rapr4,wapr4
  ENDM

;;; Now define the comb filter delay lines

;;; Freeverb full mode - 8 comb filters

csl1 sta 0
csr1 sta 0

cdl1 delay 1236
cdr1 delay 1261
wcdl1 twrite cdl1,0
wcdr1 twrite cdr1,0
rcdl1 tread cdl1,1216
rcdr1 tread cdr1,1241



csl2 sta 0
csr2 sta 0

cdl2 delay 1314
cdr2 delay 1339
wcdl2 twrite cdl2,0
wcdr2 twrite cdr2,0
rcdl2 tread cdl2,1294
rcdr2 tread cdr2,1319



csl3 sta 0
csr3 sta 0

cdl3 delay 1411
cdr3 delay 1436
wcdl3 twrite cdl3,0
wcdr3 twrite cdr3,0
rcdl3 tread cdl3,1391
rcdr3 tread cdr3,1416



csl4 sta 0
csr4 sta 0

cdl4 delay 1498
cdr4 delay 1523
wcdl4 twrite cdl4,0
wcdr4 twrite cdr4,0
rcdl4 tread cdl4,1478
rcdr4 tread cdr4,1503



csl5 sta 0
csr5 sta 0

cdl5 delay 1569
cdr5 delay 1594
wcdl5 twrite cdl5,0
wcdr5 twrite cdr5,0
rcdl5 tread cdl5,1549
rcdr5 tread cdr5,1574



csl6 sta 0
csr6 sta 0

cdl6 delay 1645
cdr6 delay 1670
wcdl6 twrite cdl6,0
wcdr6 twrite cdr6,0
rcdl6 tread cdl6,1625
rcdr6 tread cdr6,1650



csl7 sta 0
csr7 sta 0

cdl7 delay 1717
cdr7 delay 1742
wcdl7 twrite cdl7,0
wcdr7 twrite cdr7,0
rcdl7 tread cdl7,1697
rcdr7 tread cdr7,1722



csl8 sta 0
csr8 sta 0

cdl8 delay 1782
cdr8 delay 1807
wcdl8 twrite cdl8,0
wcdr8 twrite cdr8,0
rcdl8 tread cdl8,1762
rcdr8 tread cdr8,1787


do_comb_filters MACRO l,r
  comb l,rcdl1,wcdl1,csl1
  comb l,rcdl2,wcdl2,csl2
  comb l,rcdl3,wcdl3,csl3
  comb l,rcdl4,wcdl4,csl4
  comb l,rcdl5,wcdl5,csl5
  comb l,rcdl6,wcdl6,csl6
  comb l,rcdl7,wcdl7,csl7
  comb l,rcdl8,wcdl8,csl8
  comb r,rcdr1,wcdr1,csr1
  comb r,rcdr2,wcdr2,csr2
  comb r,rcdr3,wcdr3,csr3
  comb r,rcdr4,wcdr4,csr4
  comb r,rcdr5,wcdr5,csr5
  comb r,rcdr6,wcdr6,csr6
  comb r,rcdr7,wcdr7,csr7
  comb r,rcdr8,wcdr8,csr8
  ENDM

;'

  END
