//

// DSP code for SB Live (C) 4Front Technologies 2001. All rights reserved.

#define HAVE_EQ
#define HAVE_SURR_EQ

#define COPY(a,b)	ACC3(a, b, 0, 0)

// Temporary variables

.gpr TMP_L
.gpr TMP_R
.gpr PCMTMP_L
.gpr PCMTMP_R
.gpr S_SUM_L
.gpr S_SUM_R
.gpr AUX_L
.gpr AUX_R
.gpr MIC_L
.gpr MIC_R
.gpr VUTMP
.gpr VUTMP2
.gpr CENTER_TMP
.gpr LFE_TMP
.gpr SURR_TMP_R
.gpr SURR_TMP_L
#ifdef AUDIGY
.gpr SPDIF_TMP_L
.gpr SPDIF_TMP_R
#endif

// Some parameters that are currently predefined by the assembler
//.parm PCM			100		stereo
//.parm MAIN			100		stereo		VOL

#ifdef HAVE_EQ
#include "equalizer.mac"
#endif

// Monitor volumes and peak meters

#ifdef HAVE_EQ
.parm EQUALIZER         0              group
	INCLUDE_EQPARMS
#endif

.parm FRONT		0		group
.parm SPDINVOL		100		mono		SPDIF
.parm SPDINMONVU	0		stereopeak	-
.parm CDSPDINVOL	100		mono		DIGCD
.parm CDSPDINMONVU	0		stereopeak  	-
.parm AC97VOL		0		mono		AC97
.parm AC97MONVU		0		stereopeak	-
.parm FRONTPCM		100		mono		PCM
.parm FRONTPCMVU	0		stereopeak  -
.parm FRONTAUX		100		mono		AUX
.parm FRONTAUXVU	0		stereopeak  -
#ifdef AUDIGY
.parm FRONTMIC		100		mono		MIC
.parm FRONTMICVU	0		stereopeak  -
#endif
.parm FRONTVOL		100		stereo		VOL
.parm FRONTVU		0		stereopeak	-

.parm SURR		0		group
.parm R_SPDINVOL		0		mono	SPDIF
.parm R_SPDINMONVU	0		stereopeak	-
.parm R_CDSPDINVOL	0		mono		DIGCD
.parm R_CDSPDINMONVU	0		stereopeak  	-
.parm R_AC97VOL		0		mono		AC97
.parm R_AC97MONVU		0		stereopeak	-
.parm SURRPCM		100		mono		PCM
.parm SURRPCMVU		0		stereopeak	-
.parm SURRAUX		0		mono		AUX
.parm SURRAUXVU		0		stereopeak  -
#ifdef AUDIGY
.parm SURRMIC		100		mono		MIC
.parm SURRMICVU		0			stereopeak  -
#endif
.parm SURRVOL		100		stereo		VOL
.parm SURRVU		0		stereopeak	-

// Recording volumes and VU meters
.parm RECORD			0		group
.parm SPDIFREC		100		mono		SPDIF
.parm SPDIFVU		0		stereopeak	-
.parm CDSPDIFREC	100		mono		DIGCD
.parm CDVU			0		stereopeak	-
.parm AC97REC		100		mono		AC97
.parm AC97VU		0		stereopeak	-
.parm LOOP		0		mono		PCM
.parm LOOPVU		0		stereopeak	-
.parm RECAUX		100		mono		AUX
.parm RECAUXVU		0		stereopeak  -
#ifdef AUDIGY
.parm RECMIC		0		mono		MIC
.parm RECMICVU	0			stereopeak  -
#endif
.parm RECVOL		100		stereo		VOL
.parm RECVU		0		stereopeak	-

// Dummy placeholders for the main VU meter.
.gpr VU_L
.gpr VU_R

#ifdef AUDIGY
#include "emu10k2.mac"
#else
#include "emu10k1.mac"
#endif

#ifdef AUDIGY
.parm _PASSTHROUGH		0		onoff;
#endif

#ifdef HAVE_EQ
	INCLUDE_EQVARS
	INCLUDE_DELAYS(L)
	INCLUDE_DELAYS(R)
# ifdef HAVE_SURR_EQ
	INCLUDE_DELAYS(SL)
	INCLUDE_DELAYS(SR)
# endif
#endif

#include "vu.mac"

#ifdef AUDIGY
// Take wavetable engine front sends (0,1)
  MACINTS(TMP_L, 0, SEND_L, 1)
  MACINTS(TMP_R, 0, SEND_R, 1)
  MACS(PCMTMP_L, 0, TMP_L, PCM_L)
  MACS(PCMTMP_R, 0, TMP_R, PCM_R)

// Take raw S/PDIF from the dedicated sends
  COPY(SPDIF_TMP_L, SPDIF_L)
  COPY(SPDIF_TMP_R, SPDIF_R)

// Do the same for surround and rear speaker outputs (sum them together)
  SUM(S_SUM_L, SEND_SL, SEND_RL, 0)
  SUM(S_SUM_R, SEND_SR, SEND_RR, 0)
  MACINTS(CENTER_TMP, 0, SEND_C, 1)
  MACINTS(LFE_TMP, 0, SEND_W, 1)
  MACS(S_SUM_L, 0, S_SUM_L, PCM_L)
  MACS(S_SUM_R, 0, S_SUM_R, PCM_R)
#else
// Take wavetable engine front sends (0,1) and multiply them by 4 to boost volume
  MACINTS(TMP_L, 0, SEND_L, 4)
  MACINTS(TMP_R, 0, SEND_R, 4)
  MACS(PCMTMP_L, 0, TMP_L, PCM_L)
  MACS(PCMTMP_R, 0, TMP_R, PCM_R)

// Do the same for rear outputs (2,3)
  MACINTS(S_SUM_L, 0, SEND_SL, 4)
  MACINTS(S_SUM_R, 0, SEND_SR, 4)
  MACS(S_SUM_L, 0, S_SUM_L, PCM_L)
  MACS(S_SUM_R, 0, S_SUM_R, PCM_R)
#endif

//
// Auxiliary input monitors (front channel)
//

  MACS(AUX_L, 0, PCMTMP_L, FRONTPCM)
  MACS(AUX_R, 0, PCMTMP_R, FRONTPCM)
  VU(FRONTPCMVU_L, AUX_L)
  VU(FRONTPCMVU_R, AUX_R)
  MACS(AUX_L, AUX_L, CDSPDIFIN_L, CDSPDINVOL)
  MACS(AUX_R, AUX_R, CDSPDIFIN_R, CDSPDINVOL)
  // CD S/PDIF input monitor VU
  VUSCALE(CDSPDINMONVU_L, CDSPDIFIN_L, CDSPDINVOL)
  VUSCALE(CDSPDINMONVU_R, CDSPDIFIN_R, CDSPDINVOL)

  MACS(AUX_L, AUX_L, AC97IN_L, AC97VOL)
  MACS(AUX_R, AUX_R, AC97IN_R, AC97VOL)
  // AC97 monitor VU
  VUSCALE(AC97MONVU_L, AC97IN_L, AC97VOL)
  VUSCALE(AC97MONVU_R, AC97IN_R, AC97VOL)

  MACS(AUX_L, AUX_L, GPSPDIFIN_L, SPDINVOL)
  MACS(AUX_R, AUX_R, GPSPDIFIN_R, SPDINVOL)
  // S/PDIF input monitor VU
  VUSCALE(SPDINMONVU_L, GPSPDIFIN_L, SPDINVOL)
  VUSCALE(SPDINMONVU_R, GPSPDIFIN_R, SPDINVOL)
  // Boost S/PDIF input level by summing it twice
  MACS(AUX_L, AUX_L, GPSPDIFIN_L, SPDINVOL)
  MACS(AUX_R, AUX_R, GPSPDIFIN_R, SPDINVOL)

  // Auxiliary inputs (Live Drive)
  MACS(AUX_L, AUX_L, AUXIN_L, FRONTAUX)
  MACS(AUX_R, AUX_R, AUXIN_R, FRONTAUX)
  VUSCALE(FRONTAUXVU_L, AUXIN_L, FRONTAUX)
  VUSCALE(FRONTAUXVU_R, AUXIN_R, FRONTAUX)

// Apply main volume to the aux input front volume
  MACS(AUX_L, 0, AUX_L, MAIN_L)
  MACS(AUX_R, 0, AUX_R, MAIN_R)

#ifdef AUDIGY
  // Livedrive mic inputs
  MACS(MIC_L, MIC_L, MICIN_L, FRONTMIC)
  MACS(MIC_R, MIC_R, MICIN_R, FRONTMIC)
  VUSCALE(FRONTMICVU_L, MICIN_L, FRONTMIC)
  VUSCALE(FRONTMICVU_R, MICIN_R, FRONTMIC)

// Apply main volume to the aux input front volume
  MACS(MIC_L, 0, AUX_L, MAIN_L)
  MACS(MIC_R, 0, AUX_R, MAIN_R)
#endif

#ifdef HAVE_EQ
  EQUALIZER(L, L, AUX_L)
  EQUALIZER(R, R, AUX_R)
#endif

// Secondary sends before applying front volume
#ifdef AUDIGY
  MACINTS(SPDOUT1_L, 0, AUX_L, _PASSTHROUGH_OFF)
  MACINTS(SPDOUT1_R, 0, AUX_R, _PASSTHROUGH_OFF)

// S/PDIF passthrough
.const 0x1008 0x1008
.const 0xffff0000 0xffff0000
.gpr TMP0
.gpr TMP1
.gpr TMP2

// Left channel
  COPY(TMP2, SPDIF_TMP_L)
  SKIP(CCR, CCR, 0x1008, 1)
     ACC3(TMP2, 0, 65536, TMP2)
  ANDXOR(TMP2, TMP2, 0xffff0000, 0)
  MACINTS(TMP0, 0, TMP2, _PASSTHROUGH_ON)
  ANDXOR(TMP1, _PASSTHROUGH_ON, 1, 1)
  MACINTS(TMP1, 0, AUX_L, TMP1)
  ACC3(SPDOUT1_L, TMP0, TMP1, 0)

// Right channel
  COPY(TMP2, SPDIF_TMP_R)
  SKIP(CCR, CCR, 0x1008, 1)
      ACC3(TMP2, 0, 65536, TMP2)
  ANDXOR(TMP2, TMP2, 0xffff0000, 0)
  MACINTS(TMP0, 0, TMP2, _PASSTHROUGH_ON)
  ANDXOR(TMP1, _PASSTHROUGH_ON, 1, 1)
  MACINTS(TMP1, 0, AUX_R, TMP1)
  ACC3(SPDOUT1_R, TMP0, TMP1, 0)
#else
  COPY(SPDOUT1_L, AUX_L)
  COPY(SPDOUT1_R, AUX_R)
#endif

// Update main peak meters
  VU(VU_L, AUX_L)
  VU(VU_R, AUX_R)

// Mix the PCM sends with monitor inputs in and write to front output
  MACS(FRONT_L, 0, AUX_L, FRONTVOL_L)
  MACS(FRONT_R, 0, AUX_R, FRONTVOL_R)

  MACS(HEADPH_L, 0, AUX_L, FRONTVOL_L)
  MACS(HEADPH_R, 0, AUX_R, FRONTVOL_R)
  VUSCALE(FRONTVU_L, AUX_L, FRONTVOL_L)
  VUSCALE(FRONTVU_R, AUX_R, FRONTVOL_R)

//
// Auxiliary input monitors (rear channel)
//

  MACS(S_SUM_L, 0, S_SUM_L, SURRPCM)
  MACS(S_SUM_R, 0, S_SUM_R, SURRPCM)
  VU(SURRPCMVU_L, S_SUM_L)
  VU(SURRPCMVU_R, S_SUM_R)
  MACS(S_SUM_L, S_SUM_L, CDSPDIFIN_L, R_CDSPDINVOL)
  MACS(S_SUM_R, S_SUM_R, CDSPDIFIN_R, R_CDSPDINVOL)
  // CD S/PDIF input monitor VU
  VUSCALE(R_CDSPDINMONVU_L, CDSPDIFIN_L, R_CDSPDINVOL)
  VUSCALE(R_CDSPDINMONVU_R, CDSPDIFIN_R, R_CDSPDINVOL)

  MACS(S_SUM_L, S_SUM_L, AC97IN_L, R_AC97VOL)
  MACS(S_SUM_R, S_SUM_R, AC97IN_R, R_AC97VOL)
  // AC97 monitor VU
  VUSCALE(R_AC97MONVU_L, AC97IN_L, R_AC97VOL)
  VUSCALE(R_AC97MONVU_R, AC97IN_R, R_AC97VOL)

  MACS(S_SUM_L, S_SUM_L, GPSPDIFIN_L, R_SPDINVOL)
  MACS(S_SUM_R, S_SUM_R, GPSPDIFIN_R, R_SPDINVOL)
  // S/PDIF input monitor VU
  VUSCALE(R_SPDINMONVU_L, GPSPDIFIN_L, R_SPDINVOL)
  VUSCALE(R_SPDINMONVU_R, GPSPDIFIN_R, R_SPDINVOL)

  MACS(S_SUM_L, S_SUM_L, AUXIN_L, SURRAUX)
  MACS(S_SUM_R, S_SUM_R, AUXIN_R, SURRAUX)
  VUSCALE(SURRAUXVU_L, AUXIN_L, SURRAUX)
  VUSCALE(SURRAUXVU_R, AUXIN_R, SURRAUX)
#ifdef AUDIGY
  MACS(S_SUM_L, S_SUM_L, MICIN_L, SURRMIC)
  MACS(S_SUM_R, S_SUM_R, MICIN_R, SURRMIC)
  VUSCALE(SURRMICVU_L, MICIN_L, SURRMIC)
  VUSCALE(SURRMICVU_R, MICIN_R, SURRMIC)
#endif
#if 0
  // Boost S/PDIF input level by summing it twice 
  MACS(S_SUM_L, S_SUM_L, GPSPDIFIN_L, R_SPDINVOL)
  MACS(S_SUM_R, S_SUM_R, GPSPDIFIN_R, R_SPDINVOL)
#endif

  MACS(TMP_L, 0, S_SUM_L, MAIN_L)
  MACS(TMP_R, 0, S_SUM_R, MAIN_R)
#if defined(HAVE_EQ) && defined(HAVE_SURR_EQ)
  EQUALIZER(L, SL, TMP_L)
  EQUALIZER(R, SR, TMP_R)
#endif
  MACS(SURR_L, 0, TMP_L, SURRVOL_L)
  MACS(SURR_R, 0, TMP_R, SURRVOL_R)

#ifdef AUDIGY
  MACS(DSURR_L, 0, TMP_L, SURRVOL_L)
  MACS(DSURR_R, 0, TMP_R, SURRVOL_R)
#else
  MACS(AC97SURR_L, 0, TMP_L, SURRVOL_L)
  MACS(AC97SURR_R, 0, TMP_R, SURRVOL_R)
  MACINTS(CENTER_TMP, 0, SEND_C, 1)
  MACINTS(LFE_TMP, 0, SEND_W, 1)

#endif
  VUSCALE(SURRVU_L, TMP_L, SURRVOL_L)
  VUSCALE(SURRVU_R, TMP_R, SURRVOL_R)

//
// Misc outputs
//

  SUM(DCENTER, CENTER_TMP, 0, 0)
  SUM(DLFE, LFE_TMP, 0, 0)
  SUM(ACENTER, CENTER_TMP, 0, 0)
  SUM(ALFE, LFE_TMP, 0, 0)

//
// Recording sources and VU meters
//

  MACS(TMP_L, 0, AC97IN_L, AC97REC)
  MACS(TMP_R, 0, AC97IN_R, AC97REC)
  VUSCALE(AC97VU_L, AC97IN_L, AC97REC)
  VUSCALE(AC97VU_R, AC97IN_R, AC97REC)

  MACS(TMP_L, TMP_L, GPSPDIFIN_L, SPDIFREC)
  MACS(TMP_R, TMP_R, GPSPDIFIN_R, SPDIFREC)
  VUSCALE(SPDIFVU_L, GPSPDIFIN_L, SPDIFREC)
  VUSCALE(SPDIFVU_R, GPSPDIFIN_R, SPDIFREC)

  MACS(TMP_L, TMP_L, CDSPDIFIN_L, CDSPDIFREC)
  MACS(TMP_R, TMP_R, CDSPDIFIN_R, CDSPDIFREC)
  VUSCALE(CDVU_L, CDSPDIFIN_L, CDSPDIFREC)
  VUSCALE(CDVU_R, CDSPDIFIN_R, CDSPDIFREC)

  MACS(TMP_L, TMP_L, PCMTMP_L, LOOP)
  MACS(TMP_R, TMP_R, PCMTMP_R, LOOP)
  VUSCALE(LOOPVU_L, PCMTMP_L, LOOP)
  VUSCALE(LOOPVU_R, PCMTMP_R, LOOP)

  MACS(TMP_L, TMP_L, AUXIN_L, RECAUX)
  MACS(TMP_R, TMP_R, AUXIN_R, RECAUX)
  VUSCALE(RECAUXVU_L, AUXIN_L, RECAUX)
  VUSCALE(RECAUXVU_R, AUXIN_R, RECAUX)

#ifdef AUDIGY
  MACS(TMP_L, TMP_L, MICIN_L, RECMIC)
  MACS(TMP_R, TMP_R, MICIN_R, RECMIC)
  VUSCALE(RECMICVU_L, MICIN_L, RECMIC)
  VUSCALE(RECMICVU_R, MICIN_R, RECMIC)
#endif

  MACS(ADC_L, 0, TMP_L, RECVOL_L)
  MACS(ADC_R, 0, TMP_R, RECVOL_R)
  VUSCALE(RECVU_L, TMP_L, RECVOL_L)
  VUSCALE(RECVU_R, TMP_R, RECVOL_R)
