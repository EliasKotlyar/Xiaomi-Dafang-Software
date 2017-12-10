/****************************************************************************
 HalLynx.h

 Description:	LynxONE Application Programming Interface Header File

 Created: David A. Hoatson, April 1998
	
 Copyright © 1998, 1999	Lynx Studio Technology, Inc.

 This software contains the valuable TRADE SECRETS and CONFIDENTIAL INFORMATION 
 of Lynx Studio Technology, Inc. The software is protected under copyright 
 laws as an unpublished work of Lynx Studio Technology, Inc.  Notice is 
 for informational purposes only and does not imply publication.  The user 
 of this software may make copies of the software for use with products 
 manufactured by Lynx Studio Technology, Inc. or under license from 
 Lynx Studio Technology, Inc. and for no other use.

 THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 PURPOSE.

 Environment: Windows NT Kernel mode

 4 spaces per tab

 Revision History
 
 When      Who  Description
 --------- ---  ------------------------------------------------------------
 Jun 15 98 DAH	Changed DACTL & CBLIM per new spec.
 May 11 98 DAH	Changed wave buffer size to 2048 DWORDs
 Apr 29 98 DAH	Put in prelimiary structures for the Pathfinder
****************************************************************************/
#ifndef _HALLYNX_H
#define _HALLYNX_H

#define PCIVENDOR_PLX				0x10b5
#define PCIDEVICE_PLX_9050			0x9050
#define PCIDEVICE_PLX_9052			0x9052

//#define PCIVENDOR_LYNX                0x1621      // Lynx Studio Technology, Inc. PCI ID
#define PCIVENDOR_LYNX				0x10b5
#define PCIDEVICE_LYNX_PATHFINDER	0x1142

#define PCI_MAX_BUSES				256

#ifndef MM_LYNX
#define	MM_LYNX						212
#endif

typedef unsigned long tMbx;

#define SET( value, mask )	value |= (mask)
#define CLR( value, mask )	value &= (~mask)

/////////////////////////////////////////////////////////////////////////////
// Sample Rate
/////////////////////////////////////////////////////////////////////////////
#define MIN_SAMPLE_RATE		8000	// 8kHz
#define MAX_ANALOG_RATE		50000	// 50kHz
//#define MAX_ANALOG_RATE       100000      // 100kHz
#define MAX_SAMPLE_RATE		100000	// 100kHz

/////////////////////////////////////////////////////////////////////////////
// Number of Adapters
/////////////////////////////////////////////////////////////////////////////
#define MAX_NUMBER_OF_ADAPTERS		4	// Number of Valid Adapters

/////////////////////////////////////////////////////////////////////////////
// Number of Devices
/////////////////////////////////////////////////////////////////////////////

#ifdef MULTI_INPUT		/////////////////////////////////////////////////////
#define NUM_WAVE_RECORD_DEVICES		5
#else /////////////////////////////////////////////////////
#define NUM_WAVE_RECORD_DEVICES		2
#endif /////////////////////////////////////////////////////

#define NUM_WAVE_PLAY_DEVICES		2
#define NUM_CHANNELS_PER_DEVICE		2
#define NUM_WAVE_DEVICES			(NUM_WAVE_RECORD_DEVICES + NUM_WAVE_PLAY_DEVICES)

#ifdef MULTI_INPUT		/////////////////////////////////////////////////////

#define NUM_MIDI_RECORD_DEVICES		0
#define NUM_MIDI_PLAY_DEVICES		0
#define NUM_MIDI_DEVICES			(NUM_MIDI_RECORD_DEVICES + NUM_MIDI_PLAY_DEVICES)

#define NUM_MIDI_PHYSICAL_RECORD_DEVICES	0
#define NUM_MIDI_PHYSICAL_PLAY_DEVICES		0
#define NUM_MIDI_PHYSICAL_DEVICES			(NUM_MIDI_PHYSICAL_RECORD_DEVICES + NUM_MIDI_PHYSICAL_PLAY_DEVICES)

#else /////////////////////////////////////////////////////

#define NUM_MIDI_RECORD_DEVICES		2
#define NUM_MIDI_PLAY_DEVICES		2
#define NUM_MIDI_DEVICES			(NUM_MIDI_RECORD_DEVICES + NUM_MIDI_PLAY_DEVICES)

#define NUM_MIDI_PHYSICAL_RECORD_DEVICES	2
#define NUM_MIDI_PHYSICAL_PLAY_DEVICES		2
#define NUM_MIDI_PHYSICAL_DEVICES			(NUM_MIDI_PHYSICAL_RECORD_DEVICES + NUM_MIDI_PHYSICAL_PLAY_DEVICES)

#define	MIDI_RECORD0_DEVICE			0
#define	MIDI_RECORD1_DEVICE			1
#define	MIDI_PLAY0_DEVICE			2
#define	MIDI_PLAY1_DEVICE			3

#endif /////////////////////////////////////////////////////

#define	WAVE_RECORD0_DEVICE			0
#define	WAVE_PLAY0_DEVICE			1
#define	WAVE_RECORD1_DEVICE			2
#define	WAVE_PLAY1_DEVICE			3

#ifdef MULTI_INPUT		/////////////////////////////////////////////////////

#define	WAVE_RECORD2_DEVICE			4
#define	WAVE_RECORD3_DEVICE			5
#define	WAVE_RECORD4_DEVICE			6

#endif /////////////////////////////////////////////////////

// Needed for DOS HTest utility
#define WAVE_RECORD0_INTERRUPT		(1<<0)
#define WAVE_PLAY0_INTERRUPT		(1<<1)
#define WAVE_RECORD1_INTERRUPT		(1<<2)
#define WAVE_PLAY1_INTERRUPT		(1<<3)
#define MIDI_RECORD0_INTERRUPT		(1<<4)
#define MIDI_RECORD1_INTERRUPT		(1<<5)

/////////////////////////////////////////////////////////////////////////////
// PLX
/////////////////////////////////////////////////////////////////////////////

// Where things get setup in the PLX 9050
#define NUM_BASE_ADDRESS_REGIONS			4
#define PLX_REGISTERS_INDEX					0	// The base address of the PLX PCI/Local configuration registers
#define PLX_IO_INDEX						1	// The base address of the PLX IO configuration registers
#define BASE_ADDRESS_INDEX					2	// The base address is the third item in the PCI array
#define FPGA_CONFIG_INDEX					3	// The base address of the FPGA Configuration Region

#define PLX_REGISTERS_SIZE					0x80
#define FPGA_CONFIG_SIZE					0x10

/////////////////////////////////////////////////////////////////////////////
// PLX 9050 Register Offsets
/////////////////////////////////////////////////////////////////////////////
#define PLX_LAS0RR							0x00	// Item 1: Local Address Space 0 Range Register
#define PLX_LAS1RR							0x04	// Item 2: Local Address Space 1 Range Register
#define PLX_LAS2RR							0x08	// Item 3: Local Address Space 2 Range Register
#define PLX_LAS3RR							0x0c	// Item 4: Local Address Space 3 Range Register
#define PLX_EROMRR							0x10	// Item 5: Expansion ROM Range Register
#define PLX_LAS0BA							0x14	// Item 6: Local Address Space 0 Base Address Register
#define PLX_LAS1BA							0x18	// Item 7: Local Address Space 1 Base Address Register
#define PLX_LAS2BA							0x1c	// Item 8: Local Address Space 2 Base Address Register
#define PLX_LAS3BA							0x20	// Item 9: Local Address Space 3 Base Address Register
#define PLX_EROMBA							0x24	// Item 10: Expansion ROM Range Register
#define PLX_LAS0BRD							0x28	// Item 11: Local Address Space 0 Bus Region Descriptor Register
#define PLX_LAS1BRD							0x2c	// Item 12: Local Address Space 1 Bus Region Descriptor Register
#define PLX_LAS2BRD							0x30	// Item 13: Local Address Space 2 Bus Region Descriptor Register
#define PLX_LAS3BRD							0x34	// Item 14: Local Address Space 3 Bus Region Descriptor Register
#define PLX_EROMBRD							0x38	// Item 15: Expansion ROM Bus Region Descriptor Register
#define PLX_CS0BASE							0x3c	// Item 16: Chip Select 0 Base Address Register
#define PLX_CS1BASE							0x40	// Item 17: Chip Select 1 Base Address Register
#define PLX_CS2BASE							0x44	// Item 18: Chip Select 2 Base Address Register
#define PLX_CS3BASE							0x48	// Item 19: Chip Select 3 Base Address Register
#define PLX_LCR_INTCSR						0x4c	// Item 20: Interrupt Control/Status Register
#define PLX_LCR_CNTRL						0x50	// Item 21: Initialization Control Register

/////////////////////////////////////////////////////////////////////////////
// Interrupt Control/Status Register (0x4c)
/////////////////////////////////////////////////////////////////////////////
#define PLX_INTCSR_MASK							0xF	// only bits 0-7 are used
#define PLX_INTCSR_LOCAL_INTERRUPT_1_ENABLE		(1<<0)
#define PLX_INTCSR_LOCAL_INTERRUPT_1_POLARITY	(1<<1)
#define PLX_INTCSR_LOCAL_INTERRUPT_1_ACTIVE		(1<<2)
#define PLX_INTCSR_LOCAL_INTERRUPT_2_ENABLE		(1<<3)
#define PLX_INTCSR_LOCAL_INTERRUPT_2_POLARITY	(1<<4)
#define PLX_INTCSR_LOCAL_INTERRUPT_2_ACTIVE		(1<<5)
#define PLX_INTCSR_PCI_ENABLE_INTERRUPT			(1<<6)
#define PLX_INTCSR_SOFTWARE_INTERRUPT			(1<<7)

/////////////////////////////////////////////////////////////////////////////
// Initialization Control Register (0x50)
/////////////////////////////////////////////////////////////////////////////
#define PLX_CNTRL_USER0_WAITO					(1<<0)	// WAITO if Set, USER0 if Clear
#define PLX_CNTRL_USER0_OUTPUT					(1<<1)	// Output if Set, Input if Clear
#define PLX_CNTRL_USER0_DATA					(1<<2)
#define PLX_CNTRL_USER1_LLOCK					(1<<3)	// LLOCK if Set, USER1 if Clear
#define PLX_CNTRL_USER1_OUTPUT					(1<<4)	// Output if Set, Input if Clear
#define PLX_CNTRL_USER1_DATA					(1<<5)
#define PLX_CNTRL_USER2_CS2						(1<<6)	// CS2 if Set, USER2 if Clear
#define PLX_CNTRL_USER2_OUTPUT					(1<<7)	// Output if Set, Input if Clear
#define PLX_CNTRL_USER2_DATA					(1<<8)
#define PLX_CNTRL_USER3_CS3						(1<<9)	// CS3 if Set, USER3 if Clear
#define PLX_CNTRL_USER3_OUTPUT					(1<<10)	// Output if Set, Input if Clear
#define PLX_CNTRL_USER3_DATA					(1<<11)

#define PLX_CNTRL_LCBAR_MEMIO					(0<<12)	// Manual is in error
#define PLX_CNTRL_LCBAR_MEM						(1<<12)	// Should be set to this value
#define PLX_CNTRL_LCBAR_IO						(2<<12)
#define PLX_CNTRL_LCBAR_NONE					(3<<12)	// Manual is in error

#define PLX_CNTRL_EEPROM_CLOCK					0x01000000	// (1<<24)
#define PLX_CNTRL_EEPROM_CHIP_SELECT			0x02000000	// (1<<25)
#define PLX_CNTRL_EEPROM_WRITE_DATA				0x04000000	// (1<<26)
#define PLX_CNTRL_EEPROM_READ_DATA				0x08000000	// (1<<27)
#define PLX_CNTRL_EEPROM_VALID					0x10000000	// (1<<28)
#define PLX_CNTRL_RELOAD_CONFIGURATION			0x20000000	// (1<<29)

/////////////////////////////////////////////////////////////////////////////
// Control Configuration 
/////////////////////////////////////////////////////////////////////////////
#define REG_FPGA_CONFIG							PLX_CNTRL_USER0_DATA
#define REG_FPGA_STATUS							PLX_CNTRL_USER2_DATA
#define REG_FPGA_CONFIG_DONE					PLX_CNTRL_USER3_DATA

/////////////////////////////////////////////////////////////////////////////
// EEPROM
/////////////////////////////////////////////////////////////////////////////
#define EEPROM_READ								0x180	// 1 10XX XXXX
#define EEPROM_WRITE							0x140	// 1 01XX XXXX
#define EEPROM_WEN								0x130	// 1 0011 XXXX

#define EEPROM_COMMAND_LENGTH					9	// in bits
#define EEPROM_DATA_LENGTH						16	// 16 write data bits

#define EEPROM_SERIALNUMBER						0x34
#define EEPROM_REVISION							0x35	// Decimal
#define EEPROM_YEAR								0x36
#define EEPROM_MONTHDAY							0x37	// Month in HIBYTE, Day in LOBYTE

/////////////////////////////////////////////////////////////////////////////
// Register Mailboxes
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Digital Audio Control Register (Version 2)
/////////////////////////////////////////////////////////////////////////////
#define REG_DACTL_DAR1EN				(1<<0)	// Digital Audio 1 Receive Enable
#define REG_DACTL_DAT1EN				(1<<1)	// Digital Audio 1 Transmit Enable
#define REG_DACTL_DAR2EN				(1<<2)	// Digital Audio 2 Receive Enable
#define REG_DACTL_DAT2EN				(1<<3)	// Digital Audio 2 Transmit Enable

#define REG_DACTL_MIXSRC_DAR1			(0<<4)	// Monitor Source (Analog In)
#define REG_DACTL_MIXSRC_DAR2			(1<<4)	// Monitor Source (Digital In)
#define REG_DACTL_MIXSRC_DAT1			(2<<4)	// Monitor Source (Analog Out)
#define REG_DACTL_MIXSRC_DAT2			(3<<4)	// Monitor Source (Digital Out)
#define REG_DACTL_MIXSRCMASK			(3<<4)	// Mix Source Bit-Mask

#define REG_DACTL_DAT1MIXEN				(1<<6)	// Monitor Enable for Analog Output
#define REG_DACTL_DAT2MIXEN				(1<<7)	// Monitor Enable for Digital Output

#define REG_DACTL_TRIM					(1<<8)	// Analog Input & Output Trim (0 -10dBV, 1 +4dBu)
#define REG_DACTL_ADRSTn				(1<<9)	// A/D converter reset, active low
#define REG_DACTL_ADCAL					(1<<10)	// A/D converter calibration enable
#define REG_DACTL_ADHDP					(1<<11)	// A/D converter high-pass filter enable, should be enabled by default
#define REG_DACTL_DACAL					(1<<12)	// D/A converter calibration enable
#define REG_DACTL_DAMUTEn				(1<<13)	// D/A converter output mute, active low
#define REG_DACTL_DAAUTOn				(1<<14)	// D/A converter auto-mute enable, active low
#define REG_DACTL_ACLKENn				(1<<15)	// A/D and D/A LRCK enable, active low (low < 50kHz, high > 50kHz)

#ifdef MULTI_INPUT		/////////////////////////////////////////////////////

#define REG_DACTL_DAR3EN				(1<<16)
#define REG_DACTL_DAR4EN				(1<<17)
#define REG_DACTL_DAR5EN				(1<<18)

#define REG_DACTL_DAR1SRC				(1<<19)	// Digital Audio Receive Source, 0=Analog, 1=Digital
#define REG_DACTL_DAR2SRC				(1<<20)	// Digital Audio Receive Source, 0=Analog, 1=Digital
#define REG_DACTL_DAR3SRC				(1<<21)	// Digital Audio Receive Source, 0=Analog, 1=Digital
#define REG_DACTL_DAR4SRC				(1<<22)	// Digital Audio Receive Source, 0=Analog, 1=Digital
#define REG_DACTL_DAR5SRC				(1<<23)	// Digital Audio Receive Source, 0=Analog, 1=Digital

#define REG_DACTL_DAENMASK				(0x7000F)	// Digital Audio Enable Mask

#else /////////////////////////////////////////////////////

#define REG_DACTL_DAENMASK				(0xF)	// Digital Audio Enable Mask

#endif /////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// MIDI Control Register
/////////////////////////////////////////////////////////////////////////////
#define REG_MIDICTL_M1REN				(1<<0)
#define REG_MIDICTL_M1TEN				(1<<1)
#define REG_MIDICTL_M2REN				(1<<2)
#define REG_MIDICTL_M2TEN				(1<<3)
#define REG_MIDICTL_M1IE				(1<<4)
#define REG_MIDICTL_M2IE				(1<<5)

/////////////////////////////////////////////////////////////////////////////
// AES Control Register
/////////////////////////////////////////////////////////////////////////////
/*
typedef union
{
	struct
	{
		UINT	DORST		: 1;	// Digital Out Reset (Active Low)
		UINT	DIOFMT		: 1;	// Digital In/Out Format Control (0 AES/EBU, 1 S/PDIF)
		UINT	DIOTYPE		: 1;	// Digital IO Type (0 AES/EBU, 1 S/PDIF) (Switches Relay)
		UINT	DOCS		: 7;	// Digital Out Channel Status
		UINT	DISEL		: 1;	// Digital Input Status Select (0 Errors, 1 Channel Status)
		UINT	DICHNL		: 1;	// Digital Input Channel Select (0 Subframe 1, 1 Subframe 2) (Also LED)
	} Bits;
	ULONG	ulAESCTL;
} tAESCTL;
*/
//Digital output channel status bits
/*
typedef union
{
	struct
	{
		UINT	offset	: 3;
		UINT	EM0		: 1;	// Emphasis
		UINT	EM1		: 1;	// Emphasis
		UINT	C1n		: 1;	// CS bit 1 inverted
		UINT	C6n		: 1;	// CS bit 6 inverted
		UINT	C7n		: 1;	// CS bit 7 inverted
		UINT	C9n		: 1;	// CS bit 9 inverted
		UINT	TRNPT	: 1;	// must be low!

	} ProBits;

	struct
	{
		UINT	offset	: 3;
		UINT	C9n		: 1;	// CS bit 9 inverted
		UINT	C8n		: 1;	// CS bit 8 inverted
		UINT	FC0		: 1;	// sample rate indication bit 0
		UINT	C2n		: 1;	// CS bit 2 inverted
		UINT	C3n		: 1;	// CS bit 3 inverted
		UINT	C15n	: 1;	// CS bit 15 inverted
		UINT	FC1		: 1;	// sample rate indication bit 1
	} ConBits;
	ULONG	ulDOCS;
} tDOCS;
*/

#define REG_AESCTL_DORSTn				(1<<0)	// Digital Out Reset
#define REG_AESCTL_DOFMT				(1<<1)	// Digital In/Out Format Control (0 AES/EBU, 1 S/PDIF)
#define	REG_AESCTL_DIOTYPE				(1<<2)	// Same as above: Switches AESEBU/CONSUMER relay

#define REG_AESCTL_DOCS_PRO_EM0			(1<<3)
#define REG_AESCTL_DOCS_PRO_EM1			(1<<4)
#define REG_AESCTL_DOCS_PRO_C1n			(1<<5)
#define REG_AESCTL_DOCS_PRO_C6n			(1<<6)
#define REG_AESCTL_DOCS_PRO_C7n			(1<<7)
#define REG_AESCTL_DOCS_PRO_C9n			(1<<8)
#define REG_AESCTL_DOCS_PRO_TRNPT		(1<<9)

#define REG_AESCTL_DOCS_CON_C9n			(1<<3)
#define REG_AESCTL_DOCS_CON_C8n			(1<<4)
#define REG_AESCTL_DOCS_CON_FC0			(1<<5)
#define REG_AESCTL_DOCS_CON_C2n			(1<<6)
#define REG_AESCTL_DOCS_CON_C3n			(1<<7)
#define REG_AESCTL_DOCS_CON_C15n		(1<<8)
#define REG_AESCTL_DOCS_CON_FC1			(1<<9)

#define REG_AESCTL_DISEL				(1<<10)
#define REG_AESCTL_DICHNL				(1<<11)

/////////////////////////////////////////////////////////////////////////////
// PLL Control Register
/////////////////////////////////////////////////////////////////////////////
/*
typedef union
{
	struct
	{
		UINT	M		: 11;	// M Register (2 - 2047)
		UINT	BypassM	: 1;	// Bypass M bit
		UINT	N		: 11;	// N Register (2 - 2047)
		UINT	P		: 2;	// P Register
		UINT	CLKSRC	: 3;	// Clock Source Select
	} Bits;
	ULONG	ulPLLCTL;
} tPLLCTL;
*/
#define REG_PLLCTL_M_OFFSET			0
#define REG_PLLCTL_BypassM_OFFSET	11
#define REG_PLLCTL_N_OFFSET			12
#define REG_PLLCTL_P_OFFSET			23
#define REG_PLLCTL_CLKSRC_OFFSET	25

#define MAKE_PLLCTL( M, bpM, N, P, CLK )	((M) | (bpM << 11) | (N << 12) | (P << 23) | (CLK << 25))

#define SR_P2	0
#define SR_P4	1
#define SR_P8	2
#define SR_P16	3

enum
{
  CLKSRC_INTERNAL = 0,
  CLKSRC_EXTERNAL = 2,
  CLKSRC_HEADER = 3,
  CLKSRC_DIGITAL = 5,
  CLKSRC_EXTERNAL_WORD = 6,
  CLKSRC_HEADER_WORD = 7
};

#define REG_PLLCTL_CLKSRC_INTERNAL		(CLKSRC_INTERNAL	<< REG_PLLCTL_CLKSRC_OFFSET)
#define REG_PLLCTL_CLKSRC_EXTERNAL		(CLKSRC_EXTERNAL	<< REG_PLLCTL_CLKSRC_OFFSET)
#define REG_PLLCTL_CLKSRC_HEADER		(CLKSRC_HEADER		<< REG_PLLCTL_CLKSRC_OFFSET)
#define REG_PLLCTL_CLKSRC_DIGITAL		(CLKSRC_DIGITAL		<< REG_PLLCTL_CLKSRC_OFFSET)
#define REG_PLLCTL_CLKSRC_EXTERNAL_WORD	(CLKSRC_EXTERNAL_WORD << REG_PLLCTL_CLKSRC_OFFSET)
#define REG_PLLCTL_CLKSRC_HEADER_WORD	(CLKSRC_HEADER_WORD	<< REG_PLLCTL_CLKSRC_OFFSET)

/////////////////////////////////////////////////////////////////////////////
// Global Status Register
/////////////////////////////////////////////////////////////////////////////
/*
typedef union
{
	struct
	{
		UINT	DAR1IF		: 1;	// Digital Audio 1 Receive Interrupt Flag (Status Only)
		UINT	DAT1IF		: 1;	// Digital Audio 1 Transmit Interrupt Flag (Status Only)
		UINT	DAR2IF		: 1;	// Digital Audio 2 Receive Interrupt Flag (Status Only)
		UINT	DAT2IF		: 1;	// Digital Audio 2 Transmit Interrupt Flag (Status Only)
		UINT	M1RIF		: 1;	// Midi 1 Receive Interrupt Flag (Status Only)
		UINT	M2RIF		: 1;	// Midi 2 Receive Interrupt Flag (Status Only)
		UINT	LRCK		: 1;	// Left/Right Clock bit
		UINT	DICS		: 6;	// Digital In Channel Status

#ifdef MULTI_INPUT		/////////////////////////////////////////////////////

		UINT	DAR3IF		: 1;	// Digital Audio 3 Receive Interrupt Flag (Status Only)
		UINT	DAR4IF		: 1;	// Digital Audio 4 Receive Interrupt Flag (Status Only)
		UINT	DAR5IF		: 1;	// Digital Audio 5 Receive Interrupt Flag (Status Only)

#endif					/////////////////////////////////////////////////////
	} Bits;
	ULONG	ulGSTAT;
} tGSTAT;
*/
#define REG_GSTAT_LRCK					(1<<6)	// Left/Right Clock

// DISEL must be 0 to read these errors
#define REG_GSTAT_DIERR_NOERROR			(0<<7)	// No Error Detected
#define REG_GSTAT_DIERR_VALIDITY		(1<<7)	// Validity Bit High
#define REG_GSTAT_DIERR_CONFIDENCE		(2<<7)	// Confidence Flag
#define REG_GSTAT_DIERR_SLIPPEDSAMPLE	(3<<7)	// Slipped Sample
#define REG_GSTAT_DIERR_CRC				(4<<7)	// CRC Error (Pro Mode Only)
#define REG_GSTAT_DIERR_PARITY			(5<<7)	// Parity Error
#define REG_GSTAT_DIERR_BIPHASE			(6<<7)	// Bi-phase Coding Error
#define REG_GSTAT_DIERR_NOLOCK			(7<<7)	// Receiver Not Locked
#define REG_GSTAT_DIERR_MASK			(7<<7)

// DISEL must be 1 to read these bits
#define REG_GSTAT_DICS_CONSUMER			(1<<7)	// C0 Inverted
#define REG_GSTAT_DICS_NONAUDIO			(1<<8)	// C1 Inverted

#define REG_GSTAT_DICS_PRO_C0n			(1<<7)	// C0 Inverted
#define REG_GSTAT_DICS_PRO_C1n			(1<<8)	// C1 Inverted
#define REG_GSTAT_DICS_PRO_EM0			(1<<9)	// EM0
#define REG_GSTAT_DICS_PRO_EM1			(1<<10)	// EM1
#define REG_GSTAT_DICS_PRO_C9n			(1<<11)	// C9 Inverted
#define REG_GSTAT_DICS_PRO_CRCEn		(1<<12)	// CRC Error Inverted

#define REG_GSTAT_DICS_CON_C0n			(1<<7)	// C0 Inverted
#define REG_GSTAT_DICS_CON_C1n			(1<<8)	// C1 Inverted
#define REG_GSTAT_DICS_CON_C2n			(1<<9)	// C2 Inverted
#define REG_GSTAT_DICS_CON_C3n			(1<<10)	// C3 Inverted
#define REG_GSTAT_DICS_CON_ORIGn		(1<<11)	// Orginal Source Inverted
#define REG_GSTAT_DICS_CON_IGCAT		(1<<12)	// SCMS Related (Ignore Category) Set if ONE copy permitted

/////////////////////////////////////////////////////////////////////////////
// Interrupt Status Register
/////////////////////////////////////////////////////////////////////////////
/*
typedef union
{
	struct
	{
		UINT	DAR1IF		: 1;	// Digital Audio 1 Receive Interrupt Flag (Status and Reset)
		UINT	DAT1IF		: 1;	// Digital Audio 1 Transmit Interrupt Flag (Status and Reset)
		UINT	DAR2IF		: 1;	// Digital Audio 2 Receive Interrupt Flag (Status and Reset)
		UINT	DAT2IF		: 1;	// Digital Audio 2 Transmit Interrupt Flag (Status and Reset)
		UINT	M1RIF		: 1;	// Midi 1 Receive Interrupt Flag (Status and Reset)
		UINT	M2RIF		: 1;	// Midi 2 Receive Interrupt Flag (Status and Reset)
		UINT	LRCK		: 1;	// Left/Right Clock bit

#ifdef MULTI_INPUT		/////////////////////////////////////////////////////

		UINT	DICS		: 6;	// Must skip over 6 bits from the LRCK and DICS
		UINT	DAR3IF		: 1;	// Digital Audio 3 Receive Interrupt Flag (Status Only)
		UINT	DAR4IF		: 1;	// Digital Audio 4 Receive Interrupt Flag (Status Only)
		UINT	DAR5IF		: 1;	// Digital Audio 5 Receive Interrupt Flag (Status Only)

#endif					/////////////////////////////////////////////////////

	} Bits;
	ULONG	ulISTAT;
} tISTAT;
*/
#ifdef MULTI_INPUT		/////////////////////////////////////////////////////

#define REG_ISTAT_INTERRUPT_MASK	(0xE00F)

#else

#define REG_ISTAT_INTERRUPT_MASK	(0x3F)

#endif /////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Circular Buffer Limit / Interrupt Enable Register
/////////////////////////////////////////////////////////////////////////////
/*
typedef union
{
	struct
	{
		UINT	CBPLIM		: 11;	// b0-10 Circular Buffer Pointer Limit
		UINT	unused		: 3;	// bits 11,12,13 unused
		UINT	LEN32		: 1;	// b14
		UINT	INTEN		: 1;	// b15 Interrupt Enable
	} Bits;
	ULONG	ulCBLIM;
} tCBLIM;
*/
#define REG_CBLIM_MASK				(0x7FF)
#define REG_CBLIM_LEN32				(1<<14)
#define REG_CBLIM_INTEN				(1<<15)

/////////////////////////////////////////////////////////////////////////////
// Registers
/////////////////////////////////////////////////////////////////////////////
typedef struct
{
  tMbx DACTL;			// (W) Digital Audio Control
  tMbx MIDICTL;			// (W) Midi Control
  tMbx AESCTL;			// (W) AES/EBU and S/PDIF digital I/O Control
  tMbx PLLCTL;			// (W) Sample Clock PLL M, N, P registers
  tMbx GSTAT;			// (R) Global Status
  tMbx ISTAT;			// (R) Interrupt Status / Reset
  tMbx DAR1LIM;			// (W) Digital Audio Receive 1 CB Pointer Limit
  tMbx DAT1LIM;			// (W) Digital Audio Transmit 1 CB Pointer Limit
  tMbx DAR2LIM;			// (W) Digital Audio Receive 2 CB Pointer Limit
  tMbx DAT2LIM;			// (W) Digital Audio Transmit 2 CB Pointer Limit
}
tREGISTERS, FAR * PREGISTERS;

#define REGISTER_SIZE			(sizeof( tREGISTERS ) / sizeof( tMbx ))
#define STANDARD_REGISTER_SIZE	16	// in DWORDs

/////////////////////////////////////////////////////////////////////////////
// Circular Buffer Pointer Mailboxes
/////////////////////////////////////////////////////////////////////////////

typedef struct
{
  tMbx DAR1CBPTR;		// Digtial Audio Receive 1 Circular Buffer Pointer
  tMbx DAT1CBPTR;		// Digtial Audio Transmit 1 Circular Buffer Pointer
  tMbx DAR2CBPTR;		// Digtial Audio Receive 2 Circular Buffer Pointer
  tMbx DAT2CBPTR;		// Digtial Audio Transmit 2 Circular Buffer Pointer

#ifdef MULTI_INPUT		/////////////////////////////////////////////////////

  tMbx CBPTRSOFFSET;		// Unused MIDI 
  tMbx DAR3CBPTR;		// Digtial Audio Receive 3 Circular Buffer Pointer
  tMbx DAR4CBPTR;		// Digtial Audio Receive 4 Circular Buffer Pointer
  tMbx DAR5CBPTR;		// Digtial Audio Receive 5 Circular Buffer Pointer

#else

  tMbx MT1CBPTR;		// Midi Transmit 1 Circular Buffer Pointer
  tMbx MT2CBPTR;		// Midi Transmit 1 Circular Buffer Pointer
  tMbx MR1CBPTR;		// Midi Receive 1 Circular Buffer Pointer
  tMbx MR2CBPTR;		// Midi Receive 1 Circular Buffer Pointer

#endif				/////////////////////////////////////////////////////

}
tCBPTRS, FAR FAR * PCBPTRS;

#define CBPTR_SIZE				(sizeof( tCBPTRS ) / sizeof( tMbx ))
#define STANDARD_CBPTR_SIZE		16	// in DWORDs

/////////////////////////////////////////////////////////////////////////////
// Midi Buffer
/////////////////////////////////////////////////////////////////////////////

#define MIDI_CIRCULAR_BUFFER_SIZE	16

typedef struct
{
  tMbx ulBuffer[MIDI_CIRCULAR_BUFFER_SIZE];
}
tMIDIBUFFER, FAR * PMIDIBUFFER;

#define MIDIBUFFER_SIZE			(sizeof( tMIDIBUFFER ) / sizeof( tMbx ))

/////////////////////////////////////////////////////////////////////////////
// Digital Audio Buffer
/////////////////////////////////////////////////////////////////////////////
#ifdef MULTI_INPUT
#define WAVE_CIRCULAR_BUFFER_OFFSET	(0x0800 * sizeof( ULONG ))	// BYTE offset to first buffer
#else
#define WAVE_CIRCULAR_BUFFER_OFFSET	(0x2000 * sizeof( ULONG ))	// BYTE offset to first buffer
#endif
#define WAVE_CIRCULAR_BUFFER_SIZE	2048	// in DWORDs

/*
typedef struct 
{
	tMbx	ulBuffer[ WAVE_CIRCULAR_BUFFER_SIZE ];
} tWAVEBUFFER, FAR *PWAVEBUFFER;

#define WAVEBUFFER_SIZE			(sizeof( tWAVEBUFFER ) / sizeof( tMbx ))
*/

/////////////////////////////////////////////////////////////////////////////
// Overall Shared Memory
/////////////////////////////////////////////////////////////////////////////

typedef struct
{
  tREGISTERS Registers;
  tMbx RegisterSpare[(STANDARD_REGISTER_SIZE - REGISTER_SIZE)];
  tCBPTRS CBPointers;
  tMbx CBPointersSpare[(STANDARD_CBPTR_SIZE - CBPTR_SIZE)];
  tMbx Reserved[32];
  tMIDIBUFFER MidiBuffer[NUM_MIDI_PHYSICAL_DEVICES];
/*	No Longer Used (Address is Run-time computed)
	tMbx		NotAccessible[ 8064 ];
	tWAVEBUFFER	WaveBuffer[ NUM_WAVE_DEVICES ];
*/
}
tSHAREDMEMORY, FAR * PMBX;

#define MEMORY_WINDOW_SIZE	(16384L * sizeof( ULONG ))	// in bytes
#define MEMORY_TEST_OFFSET	(STANDARD_REGISTER_SIZE)	// in DWORDs
#define MEMORY_TEST_SIZE	((MEMORY_WINDOW_SIZE/sizeof( ULONG ))-STANDARD_REGISTER_SIZE)	// in DWORDs

/////////////////////////////////////////////////////////////////////////////
// Defines
/////////////////////////////////////////////////////////////////////////////

// Values for CONTROL_CLOCKSOURCE
enum
{
  MIXVAL_CLKSRC_INTERNAL = 0,	// Internal Clock
  MIXVAL_CLKSRC_EXTERNAL,	// External BNC Input
  MIXVAL_CLKSRC_HEADER,		// External Header Input
  MIXVAL_CLKSRC_DIGITAL		// Digital (AESEBU) Input
};

// Values for CONTROL_CLOCKREFERENCE
enum
{
  MIXVAL_CLKREF_AUTO = 0,	// Automatic Clock Reference
  MIXVAL_CLKREF_13p5MHZ,	// 13.5Mhz Clock Reference
  MIXVAL_CLKREF_27MHZ,		// 27Mhz Clock Reference
  MIXVAL_CLKREF_WORD,		// Word Clock
  MIXVAL_CLKREF_WORD256,	// SuperClock
  NUM_CLKREFS
};

// Values for CONTROL_MIXSRC
enum
{
  MIXVAL_MONSRC_ANALOGIN = 0,	// Analog In Monitor Mixing
  MIXVAL_MONSRC_DIGITALIN,	// Digital In Monitor Mixing
  MIXVAL_MONSRC_ANALOGOUT,	// Analog Out to Digital Out Mixing
  MIXVAL_MONSRC_DIGITALOUT	// Digital Out to Analog Out Mixing
};

// Values for CONTROL_DIGITALFORMAT
enum
{
  MIXVAL_DF_AESEBU = 0,		// AES/EBU
  MIXVAL_DF_SPDIF		// S/P DIF
};

// Values for CONTROL_TRIM
enum
{
  MIXVAL_TRIM_PROFESSIONAL = 0,	// +4dBu
  MIXVAL_TRIM_CONSUMER		// -10dBV
};

// Values for CONTROL_DITHER
enum
{
  MIXVAL_DITHER_NONE = 0,
  MIXVAL_DITHER_TRIANGULAR_PDF,
  MIXVAL_DITHER_TRIANGULAR_NS_PDF,	// noise shaped triangular
  MIXVAL_DITHER_RECTANGULAR_PDF
};

// Values for Device Modes 
enum
{
  MODE_UNDEFINED = 0,		// Default mode when initialized
  MODE_IDLE,			// Device is idle (no interrupts generated)
  MODE_PRELOAD,			// Device is being setup for playback (loading buffers for playback)
  MODE_PLAY,			// Device is playing
  MODE_RECORD,			// Device is recording
  MODE_SYNCSTART,		// Flag Device as part of SYNCSTART group. NEVER USED BY HARDWARE!!
  MODE_RECORD_OPEN,		// Flag Device is being opened. NEVER USED BY HARDWARE!!
  MODE_RECORD_CLOSE,		// Flag Device is being closed. NEVER USED BY HARDWARE!!
  NUM_MODES			// Maximum number of modes available
};

// Values for Device Formats
enum
{
  FORMAT_PCM16 = 0,		// PCM Signed 16 bit
  FORMAT_PCMU8,			// PCM Unsigned 8 bit
  FORMAT_PCM32,			// PCM Signed 32 bit integer
  NUM_FORMATS			// Maximum number of formats available
};

// Digital In Status
#define MIXVAL_DIS_CONSUMER					(0<<0)
#define MIXVAL_DIS_PROFESSIONAL				(1<<0)
#define MIXVAL_DIS_AUDIO_MODE				(0<<1)
#define MIXVAL_DIS_NONAUDIO_MODE			(1<<1)
#define MIXVAL_DIS_COPY_PROHIBITED			(0<<2)
#define MIXVAL_DIS_COPY_PERMITTED			(1<<2)
#define MIXVAL_DIS_ORIGINAL					(1<<3)
#define MIXVAL_DIS_NO_EMPHASIS				(0<<4)
#define MIXVAL_DIS_EMPHASIS					(1<<4)

#define MIXVAL_DIERR_NOERROR				(0<<16)	// No Error Detected
#define MIXVAL_DIERR_VALIDITY				(1<<16)	// Validity Bit High
#define MIXVAL_DIERR_CONFIDENCE				(2<<16)	// Confidence Flag
#define MIXVAL_DIERR_SLIPPEDSAMPLE			(3<<16)	// Slipped Sample
#define MIXVAL_DIERR_CRC					(4<<16)	// CRC Error (Pro Mode Only)
#define MIXVAL_DIERR_PARITY					(5<<16)	// Parity Error
#define MIXVAL_DIERR_BIPHASE				(6<<16)	// Bi-phase Coding Error
#define MIXVAL_DIERR_NOLOCK					(7<<16)	// Receiver Not Locked
#define MIXVAL_DIERR_MASK					(7<<16)

// Digital Out Status
#define MIXVAL_DOS_AUDIO_MODE				(0<<0)
#define MIXVAL_DOS_NONAUDIO_MODE			(1<<0)

/////////////////////////////////////////////////////////////////////////////
// Timecode
/////////////////////////////////////////////////////////////////////////////
typedef union
{
  struct
  {
    UINT nFrames:8;
    UINT nSeconds:8;
    UINT nMinutes:8;
    UINT nHours:8;
  }
  Units;
  ULONG ulTime;
}
tTIMECODE;

#endif // _HALLYNX_H
