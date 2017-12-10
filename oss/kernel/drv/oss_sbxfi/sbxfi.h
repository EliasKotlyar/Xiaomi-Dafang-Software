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

#define PCI_SUBDEVICE_ID_CREATIVE_SB0760        0x0024
#define PCI_SUBDEVICE_ID_CREATIVE_SB08801       0x0041
#define PCI_SUBDEVICE_ID_CREATIVE_SB08802       0x0042
#define PCI_SUBDEVICE_ID_CREATIVE_SB08803       0x0043

#define MAX_OUTPUTDEVS 	1
#define MAX_INPUTDEVS 	1
#define SUPPORTED_FORMAT	(AFMT_S16_LE)

#define MIXER_VOLSTEPS	144	/* Centibel steps */

//#define MAX_PLAY_CHANNELS	6 /* Does not work */
#define MAX_PLAY_CHANNELS	2

#if 0
typedef unsigned char CTBYTE, *PCTBYTE;
typedef unsigned short unsigned short, *Punsigned short;
typedef signed short CTSHORT, *PCTSHORT;
typedef unsigned int unsigned int, *unsigned int *;
typedef signed long CTLONG, *PCTLONG;
typedef void CTVOID, *PCTVOID;
typedef unsigned int CTBOOL, *PCTBOOL;
typedef unsigned int CTUINT, *PCTUINT;
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef unsigned int CTSTATUS;
typedef oss_native_word IOADDR;

enum GlobalErrorCode
{
  CTSTATUS_SUCCESS = 0x0000,
  CTSTATUS_ERROR,
  CTSTATUS_INVALIDPARAM,
  CTSTATUS_NOTSUPPORTED,
  CTSTATUS_NOMEMORY,
  CTSTATUS_INVALIDIO,
  CTSTATUS_INVALIDIRQ,
  CTSTATUS_INVALIDDMA,
  CTSTATUS_INVALIDID,
  CTSTATUS_INVALIDVALUE,
  CTSTATUS_BADFORMAT_BITS,
  CTSTATUS_BADFORMAT_RATE,
  CTSTATUS_BADFORMAT_CHANNELS,
  CTSTATUS_INUSE,
  CTSTATUS_STILLPLAYING,
  CTSTATUS_ALLOCATED,
  CTSTATUS_INVALID_FORMAT,
  CTSTATUS_OUT_OF_RESOURCE,
  CTSTATUS_CHIP_INUSE,
  CTSTATUS_NOCHIPRESOURCE,
  CTSTATUS_PORTS_INUSE,
  CTSTATUS_EXIT,
  CTSTATUS_FAILURE
};


#define ADC_SRC_MICIN       0x0
#define ADC_SRC_LINEIN      0x1
#define ADC_SRC_VIDEO       0x2
#define ADC_SRC_AUX         0x3
#define ADC_SRC_NONE        0x4

typedef struct
{
  char *name;
  int dev;
  int open_mode;
  int fmt;
  int dev_flags;
  int direction;
  int state_bits;
  int pgtable_index; 	// Pointer to the first page table entry

  int running;

  int channels;

  unsigned int rate;

  // Audio Ring resources
  unsigned int SrcChan;

  unsigned int dwDAChan[MAX_PLAY_CHANNELS];

  // Play volumes
  int vol_left, vol_right;
} sbxfi_portc_t;

typedef struct
{
  oss_device_t *osdev;
  oss_mutex_t mutex;
  oss_mutex_t low_mutex;

  char *name;
  int hw_family;

// 20K1 models
#define HW_ORIG		0x0001
#define HW_073x		0x0002
#define HW_055x		0x0004
#define HW_UAA		0x0008

// 20K2 models
#define HW_0760        0x0010
#define HW_08801       0x0020
#define HW_08802       0x0040
#define HW_08803       0x0080


  unsigned int interrupt_count;

  // Hardware IDs
  unsigned short wVendorID;
  unsigned short wDeviceID;
  unsigned short wSubsystemVendorID;
  unsigned short wSubsystemID;
  unsigned short wChipRevision;

  // Hardware Resources
  unsigned int dwMemBase;
  unsigned short wIOPortBase;

  // Buffers
  oss_native_word dwPTBPhysAddx;
  unsigned int *pdwPageTable;
  unsigned int dwPageTableSize;
  oss_dma_handle_t pgtable_dma_handle;
  int next_pg;	/* Next free index in the page table */

  sbxfi_portc_t play_portc[MAX_OUTPUTDEVS];
  int nr_outdevs;

  sbxfi_portc_t rec_portc[MAX_INPUTDEVS];
  int nr_indevs;

  // Mixer
  int mixer_dev;

  // Audio
  int first_dev;

  int next_src;	// Next free SRC channel

  sbxfi_portc_t *src_to_portc[256];
} sbxfi_devc_t;
