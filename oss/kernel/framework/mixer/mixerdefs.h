/*
 * Purpose: Mixer enum control defines for older OSS drivers.
 *
 * This file contains choice names for MIXT_ENUM controls defined by some
 * older drivers. All drivers developed recently will use an embedded
 * mechanism for setting this information.
 */
/*
 *
 * This file is part of Open Sound System.
 *
 * Copyright (C) 4Front Technologies 1996-2008.
 *
 * This this source file is released under GPL v2 license (no other versions).
 * See the COPYING file included in the main directory of this source
 * distribution for the license terms and conditions.
 *
 */

typedef struct
{
  char *name, *strings;
} mixer_def_t;

static const mixer_def_t mixer_defs[] = {
  {"setup.mon1l", "OFF A0 A1 A2 A3 A4 A5 A6 A7 B0 B1 B2 B3 B4 B5 B6 B7"},
  {"setup.mon1r", "OFF A0 A1 A2 A3 A4 A5 A6 A7 B0 B1 B2 B3 B4 B5 B6 B7"},
  {"setup.mon2l", "OFF A0 A1 A2 A3 A4 A5 A6 A7 B0 B1 B2 B3 B4 B5 B6 B7"},
  {"setup.mon2r", "OFF A0 A1 A2 A3 A4 A5 A6 A7 B0 B1 B2 B3 B4 B5 B6 B7"},
  {"fpga.srcclock", "44.1K 48K PLL AES"},
  {"fpga.clock", "44.1K 48K PLL AES"},
  {"fpga.pll", "PORTA PORTB TIMER1 EXTERNAL"},
  {"aes.mode", "CONSUMER PRO"},
  {"aes.copy", "INHIBITED PERMITTED"},
  {"aes.audio", "AUDIO DATA"},
  {"aes.preemph", "NONE 50/15us"},
  {"digi32.sync", "EXTERNAL INTERNAL"},
  {"digi32.aesmode", "CONSUMER PRO"},
  {"digi32.input", "OPTICAL RCA INTERNAL XLR"},
  {"out1.src", "CODEC DSP"},
  {"in.src", "CODEC LINE OPTICAL COAX"},
  {"reverb.type", "ROOM1 ROOM2 ROOM3 HALL1 HALL2 PLATE DELAY PANDELAY"},
  {"chorus.type",
   "CHORUS1 CHORUS2 CHORUS3 CHORUS4 FBCHORUS FLANGER SHORTDELAY FBDELAY"},
  {"digi96.sync", "EXTERNAL INTERNAL"},
  {"digi96.input", "OPTICAL COAXIAL INTERNAL XLR"},
  {"digi96.sel", "BYPASS NORMAL"},
  {"digi96.mode", "SPDIF AESEBU ADAT"},
  {"digi96.data", "AUDIO DATA"},
  {"envy24.sync", "INTERNAL SPDIF WCLOCK"},
  {"envy24.spdin", "COAX OPTICAL"},
  {"gain.out1/2", "+4DB CONSUMER -10DB"},
  {"gain.out3/4", "+4DB CONSUMER -10DB"},
  {"gain.out5/6", "+4DB CONSUMER -10DB"},
  {"gain.out7/8", "+4DB CONSUMER -10DB"},
  {"gain.in1/2", "+4DB CONSUMER -10DB"},
  {"gain.in3/4", "+4DB CONSUMER -10DB"},
  {"gain.in5/6", "+4DB CONSUMER -10DB"},
  {"gain.in7/8", "+4DB CONSUMER -10DB"},
  {"gain.out1", "+4DB CONSUMER -10DB"},
  {"gain.out2", "+4DB CONSUMER -10DB"},
  {"gain.out3", "+4DB CONSUMER -10DB"},
  {"gain.out4", "+4DB CONSUMER -10DB"},
  {"gain.out5", "+4DB CONSUMER -10DB"},
  {"gain.out6", "+4DB CONSUMER -10DB"},
  {"gain.out7", "+4DB CONSUMER -10DB"},
  {"gain.out8", "+4DB CONSUMER -10DB"},
  {"gain.in1", "+4DB CONSUMER -10DB"},
  {"gain.in2", "+4DB CONSUMER -10DB"},
  {"gain.in3", "+4DB CONSUMER -10DB"},
  {"gain.in4", "+4DB CONSUMER -10DB"},
  {"gain.in5", "+4DB CONSUMER -10DB"},
  {"gain.in6", "+4DB CONSUMER -10DB"},
  {"gain.in7", "+4DB CONSUMER -10DB"},
  {"gain.in8", "+4DB CONSUMER -10DB"},
  {"route.out1/2", "DMA MONITOR IN1/2 IN3/4 IN5/6 IN7/8 SPDIF"},
  {"route.out3/4", "DMA MONITOR IN1/2 IN3/4 IN5/6 IN7/8 SPDIF"},
  {"route.out5/6", "DMA MONITOR IN1/2 IN3/4 IN5/6 IN7/8 SPDIF"},
  {"route.out7/8", "DMA MONITOR IN1/2 IN3/4 IN5/6 IN7/8 SPDIF"},
  {"route.spdif", "DMA MONITOR IN1/2 IN3/4 IN5/6 IN7/8 SPDIF"},
  {"route.out1", "DMA MONITOR IN1 IN2 IN3 IN4 IN5 IN6 IN7 IN8 SPDIFL SPDIFR"},
  {"route.out2", "DMA MONITOR IN1 IN2 IN3 IN4 IN5 IN6 IN7 IN8 SPDIFL SPDIFR"},
  {"route.out3", "DMA MONITOR IN1 IN2 IN3 IN4 IN5 IN6 IN7 IN8 SPDIFL SPDIFR"},
  {"route.out4", "DMA MONITOR IN1 IN2 IN3 IN4 IN5 IN6 IN7 IN8 SPDIFL SPDIFR"},
  {"route.out5", "DMA MONITOR IN1 IN2 IN3 IN4 IN5 IN6 IN7 IN8 SPDIFL SPDIFR"},
  {"route.out6", "DMA MONITOR IN1 IN2 IN3 IN4 IN5 IN6 IN7 IN8 SPDIFL SPDIFR"},
  {"route.out7", "DMA MONITOR IN1 IN2 IN3 IN4 IN5 IN6 IN7 IN8 SPDIFL SPDIFR"},
  {"route.out8", "DMA MONITOR IN1 IN2 IN3 IN4 IN5 IN6 IN7 IN8 SPDIFL SPDIFR"},
  {"route.spdifl",
   "DMA MONITOR IN1 IN2 IN3 IN4 IN5 IN6 IN7 IN8 SPDIFL SPDIFR"},
  {"route.spdifr",
   "DMA MONITOR IN1 IN2 IN3 IN4 IN5 IN6 IN7 IN8 SPDIFL SPDIFR"},
  {"ews88d.spdin", "OPTICAL COAX"},
  {"ews88d.optout", "SPDIF ADAT"},
  {"codec.recsrc", "ANALOG OPTICAL COAX CD AUX"},
  {"route.front", "DMA ANALOGIN DIGITALIN"},
  {"route.rear", "DMA ANALOGIN DIGITALIN"},
  {"route.surround", "DMA ANALOGIN DIGITALIN"},
  {"route.c/l", "DMA ANALOGIN DIGITALIN"},
  {"route.spdifout", "DMA ANALOGIN DIGITALIN"},
  {"lynxone.sync", "INTERNAL DIGITAL EXTW EXT27 EXT13 HDRW HDR27 HDR13"},
  {"lynxone.format", "AESEBU SPDIF"},
  {"lynxone.trim", "+4DB -10DB"},
  {"spkmode", "FRONT SURR FRONT+SURR DISCRETE 3D"},
  {"ext.recsrc", "SPDIF_OUT I2S_OUT SPDIF_IN I2S_IN AC97 SRC"},
  {"ext.loopback", "DSP0 DSP1 DSP2 DSP3"},
  {"3dsurround.mode", "OFF NORMAL 2X 3X"},
  {"spdout.pro", "Consumer Professional"},
  {"spdout.audio", "AUDIO DATA"},
  {"spdout.rate", "48000 44100 32000"},
  {"spdif.mode", "CONSUMER PRO"},
  {"spdif.audio", "AUDIO DATA"},
  {"spdif.copyright", "YES NO"},
  {"spdif.generat", "COPY ORIGINAL"},
  {"spdif.preemph", "OFF 50/16usec"},
  {"mixext.spkmode", "FRONT SPREAD"},
  {"effects.reverb.preset",
   "SMALL_ROOM MEDIUM_ROOM LARGE_ROOM SMALL_HALL LARGE_HALL"},
  {NULL}
};
