/***************************************************************************
 HalAudio.h

 Created: David A. Hoatson, March 1997
	
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

 Revision History
 
 When      Who  Description
 --------- ---  ---------------------------------------------------------------
****************************************************************************/

#ifndef _HALAUDIO_H
#define _HALAUDIO_H

//#define MULTI_INPUT

//#include <Environ.h>
//#include <HalLynx.h>
#ifndef WIN32USER
//#include <DrvDebug.h>
#endif

/////////////////////////////////////////////////////////////////////////////
//  Structures
/////////////////////////////////////////////////////////////////////////////
//#pragma pack( push )
//#pragma pack( 1 )

typedef struct _PCI_DEVICE_INFO
{
  USHORT VendorID;
  USHORT DeviceID;
}
PCI_DEVICE_INFO, *PPCI_DEVICE_INFO;

typedef struct _PCI_BASEADDRESS
{
  ULONG ulAddress;
  ULONG ulSize;
  USHORT usType;
}
PCI_BASEADDRESS, *PPCI_BASEADDRESS;

typedef struct _PCI_CONFIGURATION
{
  ULONG ulDeviceNode;		// For Windows 95
#ifdef MACINTOSH
  RegEntryID EntryID;		// The NameRegistry entry ID for this adapter
#endif
  USHORT usVendorID;
  USHORT usDeviceID;
  USHORT usBusNumber;
  USHORT usDeviceFunction;
  USHORT usInterruptLine;
  USHORT usInterruptPin;
  PCI_BASEADDRESS Base[PCI_TYPE0_ADDRESSES];
}
PCI_CONFIGURATION, *PPCI_CONFIGURATION;

enum
{
  PCI_SPACE_IO = 0,
  PCI_SPACE_MEM
};

typedef struct
{
  PULONG pAddress;
  ULONG ulValue;
}
REGISTER, *PREGISTER;

/////////////////////////////////////////////////////////////////////////////
//  Midi
/////////////////////////////////////////////////////////////////////////////
typedef struct
{
  USHORT usDeviceIndex;
  USHORT usMode;		// the mode of the midi record

  ULONG ulPCIndex;		// the PC's Circular Buffer Pointer
  PULONG pulHWIndex;		// ptr to the HW's Circular Buffer Pointer
  PULONG pBuffer;		// the address of the midi buffer

  PVOID pA;			// pointer to the parent adapter
}
LMIDI, *PMIDI;

/////////////////////////////////////////////////////////////////////////////
//  Wave
/////////////////////////////////////////////////////////////////////////////
typedef struct
{
  USHORT usDeviceIndex;		// the number of this device
  USHORT usMode;		// the mode this device is currently in
  WAVEFORMATEX WaveFormat;	// the current format of this device

  ULONG ulDeviceEnableBit;	// the bitmask for this devices enable bit in DACTL

  ULONG ulEntryCount;		// entry count to HalDeviceGetSampleCount
  ULONG ulLastHWIndex;		// the previous hardware index
  ULONG ulSampleCount;
  ULONG ulPCIndex;		// the PC's Circular Buffer Index
  LONG lHWIndex;		// saved copy of the HWIndex
  PULONG pulHWIndex;		// ptr to the HW's Circular Buffer Pointer
  ULONG ulInterruptSamples;	// number of samples to interrupt on
  PULONG pBuffer;		// the address of the device's wave buffer
  REGISTER RegCBLIM;		// Digital Audio Limit & Interrupt Register
  REGISTER RegCBPTR;

  BOOLEAN b32BitMode;		// non zero if 32 bit mode is on for this device

  SHORT sLeftLevel;		// the current left level
  SHORT sRightLevel;		// the current right level

  USHORT usDitherNoise[2];
  USHORT usDitherType;

  // Record devices only
#ifdef MULTI_INPUT
  ULONG ulInputSource;		// 0 is analog, 1 is digital
#endif

  // Play devices only
  LONG lLeftVolume;		// the current volume to apply to the left channel
  LONG lRightVolume;		// the current volume to apply to the right channel
  BOOLEAN bMute;		// TRUE if mute on
  LONG lPreviousMonitorState;

  // ASIO
  PVOID pvLeftBuffer[2];
  PVOID pvRightBuffer[2];

  PVOID pA;			// pointer to the parent adapter
}
DEVICE, *PDEVICE;

/////////////////////////////////////////////////////////////////////////////
//  Mixer
/////////////////////////////////////////////////////////////////////////////
typedef struct
{
  PVOID pA;			// pointer to the parent adapter
}
MIXER, *PMIXER;

/////////////////////////////////////////////////////////////////////////////
//  Adapter
/////////////////////////////////////////////////////////////////////////////
typedef struct
{
  BOOLEAN bOpen;		// 
  USHORT usAdapterIndex;	// set by the driver
  PVOID pDriverContext;

  ULONG ulSyncGroup;		// bitmap of devices that are in the sync group
  ULONG ulSyncReady;		// bitmap of devices that are ready to go in the sync group

  USHORT usSerialNumber;	// this cards serial number
  USHORT usRevision;
  USHORT usMonthDay;
  USHORT usYear;

  ULONG ulClockSource;		//
  ULONG ulClockReference;	// 
  ULONG ulBaseSampleRate;	// the base samplerate of the adapter
  ULONG ulCurrentSampleRate;	// the current samplerate of the adapter (maybe changed by driver)

  BOOLEAN bAutoClockSelection;
  ULONG ulPreviousClockSource;	//
  ULONG ulPreviousClockReference;	// 

  ULONG ulDigitalFormat;	// AES/EBU or S/PDIF
  ULONG ulTrim;			// trim setting
  BOOLEAN bLevels;		// on/off for level calculation
  BOOLEAN bCalibrate;		// for compatibilty only
  BOOLEAN bAnalogMonitor;
  BOOLEAN bDigitalMonitor;
  ULONG ulMonitorState;
  ULONG ulMonitorSource;	// 0 for Analog, 1 for Digital
  BOOLEAN bMonitorOffPlay;
  BOOLEAN bMonitorOnRecord;
  BOOLEAN bAutoMute;
  BOOLEAN bSyncStart;		// 0 for Disable, 1 for Enable
  BOOLEAN bADHPF;		// 0 for Disable, 1 for Enable
  ULONG ulRecordDither;
  ULONG ulPlayDither;

  ULONG ulDigitalOutStatus;	// 

  BOOLEAN bASIO;		// true when in ASIO mode

  //BOOLEAN               bMTCGenerate;
  //tTIMECODE         MTCTime;
  //ULONG             ulMTCFrameRate;

  PBYTE pPLX;			// Address in memory of the PLX local configuration registers
  PMBX pMailbox;		// Address in memory of the entire memory window
  REGISTER RegPlxIntCsr;	// PLX 9050 Interrupt Control/Status Register
  REGISTER RegPlxCntrl;		// PLX 9050 Initialization Control Register
  REGISTER RegDACTL;
  REGISTER RegMIDICTL;
  REGISTER RegAESCTL;
  REGISTER RegPLLCTL;
  PULONG pulFPGAConfig;		// the address of the FPGA configuration area

//  PCI_CONFIGURATION   PCI;
  DEVICE Device[NUM_WAVE_DEVICES];	// the wave device structures
#ifndef MULTI_INPUT
  LMIDI Midi[NUM_MIDI_DEVICES];	// the midi device structures
#endif
  MIXER Mixer;			// the mixer structure
}
ADAPTER, *PADAPTER;


/////////////////////////////////////////////////////////////////////////////
//  Status Codes
/////////////////////////////////////////////////////////////////////////////
enum
{
  HSTATUS_OK = 0,
  HSTATUS_CANNOT_FIND_ADAPTER,
  HSTATUS_CANNOT_MAP_ADAPTER,
  HSTATUS_CANNOT_UNMAP_ADAPTER,
  HSTATUS_ADAPTER_NOT_OPEN,
  HSTATUS_ADAPTER_NOT_FOUND,
  HSTATUS_BAD_ADAPTER_RAM,
  HSTATUS_DOWNLOAD_FAILED,
  HSTATUS_HW_NOT_RESPONDING,
  HSTATUS_INVALID_MODE,
  HSTATUS_INVALID_FORMAT,
  HSTATUS_INVALID_ADDRESS,
  HSTATUS_INVALID_CLOCK_SOURCE,
  HSTATUS_INVALID_SAMPLERATE,
  HSTATUS_INVALID_MIXER_LINE,
  HSTATUS_INVALID_MIXER_CONTROL,
  HSTATUS_INVALID_MIXER_VALUE,
  HSTATUS_BUFFER_FULL,
  HSTATUS_INSUFFICIENT_RESOURCES,
  NUM_HSTATUS_CODES
};

/////////////////////////////////////////////////////////////////////////////
//  Mixer Line Types
/////////////////////////////////////////////////////////////////////////////
enum
{
  // Destinations
  LINE_ANALOG_OUT = 0,		// must be first destination line
  LINE_DIGITAL_OUT,
  LINE_RECORD_0,
  LINE_RECORD_1,
  LINE_RECORD_2,
  LINE_RECORD_3,
  LINE_RECORD_4,
  LINE_RECORD_5,
  LINE_RECORD_6,
  LINE_RECORD_7,
  // Custom
  LINE_ADAPTER,
  // Sources
  LINE_NO_SOURCE,		// must be the first source line
  LINE_ANALOG_IN,
  LINE_DIGITAL_IN,
  LINE_PLAY_0,
  LINE_PLAY_1,
  LINE_PLAY_2,
  LINE_PLAY_3,
  LINE_PLAY_4,
  LINE_PLAY_5,
  LINE_PLAY_6,
  LINE_PLAY_7,
  NUM_LINES
};

/////////////////////////////////////////////////////////////////////////////
//  Mixer Control Types
/////////////////////////////////////////////////////////////////////////////
enum
{
  CONTROL_NUMCHANNELS = 0,	// the number of channels this line has
  CONTROL_VOLUME,		// ushort
  CONTROL_PEAKMETER,		// short
  CONTROL_TRIM,			// short
  CONTROL_MUTE,			// boolean
  CONTROL_INPUT_SOURCE,		// mux (MULTI_INPUT only)
  CONTROL_MONITOR,		// boolean (on lineout and digital out lines)
  CONTROL_MONITOR_SOURCE,	// mux (on adapter)
  CONTROL_MONITOR_OFF_PLAY,	// boolean
  CONTROL_MONITOR_ON_RECORD,	// boolean

  CONTROL_DIGITALSTATUS,	// ulong
  CONTROL_CLOCKSOURCE,		// mux
  CONTROL_CLOCKREFERENCE,	// mux
  CONTROL_CLOCKRATE,		// ulong
  CONTROL_AUTOCLOCKSELECT,	// boolean (enable/disable automatic clock source selection)
  CONTROL_DIGITALFORMAT,	// boolean
  CONTROL_LEVELS,		// boolean (enable/disable level calculation)
  CONTROL_AUTOMUTE,		// boolean
  CONTROL_SYNCSTART,		// boolean
  CONTROL_ADHIPASSFILTER,	// boolean (enable/disable HPF on A>D converter)
  CONTROL_RECORD_DITHER,	// mux
  CONTROL_PLAY_DITHER,		// mux
  CONTROL_RECALIBRATE,		// boolean (momentary)

  CONTROL_SERIALNUMBER,		// ulong, serial number in loword, revision in hiword
  CONTROL_MFGDATE,		// ulong, monthday in loword, year in hiword

  NUM_CONTROLS
};

/////////////////////////////////////////////////////////////////////////////
//  Forward Declarations
/////////////////////////////////////////////////////////////////////////////
USHORT HalFindAdapter (PVOID pDriverContext, PPCI_CONFIGURATION pPCI);
USHORT HalCloseAdapter (PADAPTER pA);
USHORT HalAdapterSetASIOMode (PADAPTER pA, BOOLEAN bASIOMode);
USHORT HalFindInterrupt (PADAPTER pA, PULONG pulDeviceInterrupt);
USHORT HalSetSampleRate (PADAPTER pA, ULONG ulSampleRate);
USHORT HalGetSampleRate (PADAPTER pA, PULONG pulBaseSampleRate,
			 PULONG pulCurrentSampleRate, PULONG pulMinSampleRate,
			 PULONG pulMaxSampleRate);
USHORT HalDeviceSetMode (PDEVICE pD, USHORT usDeviceMode);
USHORT HalDeviceSetFormat (PDEVICE pD, PWAVEFORMATEX pWaveFormat);
USHORT HalDeviceValidateFormat (PDEVICE pD, PWAVEFORMATEX pWaveFormat);
USHORT HalDeviceGetTransferSize (PDEVICE pD, PULONG pulTransferSize,
				 PULONG pulCircularBufferSize);
USHORT HalDeviceTransferAudio (PDEVICE pD, PBYTE pBuffer, ULONG ulBufferSize,
			       PULONG pulBytesTranferred);
USHORT HalDeviceASIOTransferAudio (PDEVICE pD, PVOID pvLeft, PVOID pvRight,
				   ULONG ulPCIndex,
				   ULONG ulSamplesToTransfer);
USHORT HalDeviceTransferComplete (PDEVICE pD, LONG lBytesProcessed);
USHORT HalDeviceSetInterruptSamples (PDEVICE pD, ULONG ulSampleCount);
USHORT HalDeviceGetSampleCount (PDEVICE pD, PULONG pulSampleCount);
USHORT HalDeviceSetSampleCount (PDEVICE pD, ULONG ulSampleCount);
USHORT HalMixerSetControl (PMIXER pM, USHORT usDstLine, USHORT usSrcLine,
			   USHORT usControl, USHORT usChannel, ULONG ulValue);
USHORT HalMixerGetControl (PMIXER pM, USHORT usDstLine, USHORT usSrcLine,
			   USHORT usControl, USHORT usChannel,
			   PULONG pulValue);

USHORT HalMidiWrite (PMIDI pM, PBYTE pBuffer, ULONG ulBytesToWrite,
		     PULONG pulBytesWritten);
USHORT HalMidiRead (PMIDI pM, PBYTE pBuffer, ULONG ulBufferSize,
		    PULONG pulBytesRead);
USHORT HalMidiSetMode (PMIDI pM, USHORT usMode);

#ifdef DEBUG
USHORT HalAdapterWrite (PADAPTER pA, ULONG ulAddress, PULONG pOutBuffer,
			ULONG ulSize);
USHORT HalAdapterRead (PADAPTER pA, ULONG ulAddress, PULONG pInBuffer,
		       ULONG ulSize);
#endif

void RegDefine (PREGISTER pReg, void *pAddress);
void RegDefineInit (PREGISTER pReg, void *pAddress, ULONG ulInitialValue);

/////////////////////////////////////////////////////////////////////////////
// Functions actually found in the driver
/////////////////////////////////////////////////////////////////////////////
BOOLEAN DrvFindPCIDevice (PVOID pDriverContext, PPCI_CONFIGURATION pPCI);
BOOLEAN DrvMapPCIConfiguration (PVOID pDriverContext,
				PPCI_CONFIGURATION pPCI);
BOOLEAN DrvUnmapPCIConfiguration (PPCI_CONFIGURATION pPCI);
PADAPTER DrvGetAdapter (PVOID pDriverContext, int nAdapter);

#ifdef ALLOC_PRAGMA
#pragma alloc_text( init, RegDefineInit )
#pragma alloc_text( init, RegDefine )
#endif

#endif // _HALAUDIO_H
