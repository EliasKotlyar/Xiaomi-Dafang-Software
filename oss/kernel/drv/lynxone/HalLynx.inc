/***************************************************************************
 HalLynx.c

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

 Environment: Windows NT Kernel mode, Windows 95 VxD, DOS 16 Bit, Macintosh

 Revision History (New Comments on Top)
 
 When      Who  Description
 --------- ---  ---------------------------------------------------------------
 Sep 13 00 DAH	Changed SetSampleRate & SetDigitalFormat to be compatible with 
				Macintosh (no bit structures)
 Dec 02 99 DAH	Added Enable/Disable SyncStart mixer control
 Nov 02 99 DAH	Added Analog Out & Digital Out Monitor Source Selection.
 Aug 24 99 DAH	Added Non Audio handling for Digital In & Out
				Added check for Unlock on switch to Digital Clock Source
 Jun 24 99 DAH	Fixed SyncStart problem that caused monitor to be turned off
				when RECORD0 went active on card two of a multicard system.
 Mar 07 99 RJB	Fixed digital in channel status reporting. Made volume bypass 
				independent for left and right channels. Added rounding to 
				volume calculations.
 Dec 10 98 DAH	Changed default clock ref from word to 27MHz when changing from
				Internal or Digital.  This fixes the 24kHz word clock problem.
 Oct 25 98 RJB	Added code to reset 8404A for changes to clock reference or
				source to maintain DO timing. Fixed PLL values for 27MHz,
				13.5 MHz, and CK256. Added lower limit to word clock freq.
				Fixed CK128 error in FPGA.
 Jul 27 98 DAH	Added CalibrateConverters() function.
 Jul 24 98 DAH	Moved DOCS stuff to SetDigitalFormat().
				Call SetDigitalFormat from SetSampleRate.
				SetSampleRate now only allows changes when all devices are IDLE.
				Moved M,N,P values to tables. Tables should stay in .c file as 
				they use memory from the heap.
				Set the Levels to 0 upon MODE_IDLE.
****************************************************************************/
//#include <HalAudio.h>
//#include <HalDwnld.h>
//#include <stdlib.h>           // for Abs definition

#define OSS
#define TIMEOUT_SHORT		1000	// ~100ms
#define TIMEOUT_LONG		100000	// ~1 second

/////////////////////////////////////////////////////////////////////////////
//  Forward Declarations
/////////////////////////////////////////////////////////////////////////////
USHORT SetSampleRate (PADAPTER pA, ULONG ulSampleRate, ULONG ulClockSource,
		      ULONG ulClockReference, BOOL bPitchChange);
USHORT SetDigitalFormat (PADAPTER pA, ULONG ulDigitalFormat);
USHORT CalibrateConverters (PADAPTER pA);
ULONG GetDigitalInStatus (PADAPTER pA);
USHORT HalMixerInit (PMIXER pM);
void RegWrite (PREGISTER pReg, ULONG ulValue);
ULONG RegRead (PREGISTER pReg);
void RegWriteMask (PREGISTER pReg, ULONG ulMask, ULONG ulValue);
void RegBitSet (PREGISTER pReg, ULONG ulBitPosition, BOOLEAN bValue);
void DelayuS (PADAPTER pA, ULONG ulCount);

USHORT EEPromGetData (PADAPTER pA, BOOLEAN bFirstWord);
void EEPromSendData (PADAPTER pA, USHORT usData, USHORT usDataLength);
USHORT EEPromReadWord (PADAPTER pA, USHORT usAddress);
void EEPromReadAll (PADAPTER pA, PUSHORT pEEPromBuffer);
void EEPromWriteWord (PADAPTER pA, USHORT usAddress, USHORT usData);
void EEPromWriteAll (PADAPTER pA, USHORT * pEEPromBuffer);

#define Abs(x) ((x) < 0 ? -(x) : (x))

#if 0
/////////////////////////////////////////////////////////////////////////////
USHORT
HalFindAdapter (PVOID pDriverContext, PPCI_CONFIGURATION pPCI)
// Find the next adapter that matches the vendor ID and device ID required.
/////////////////////////////////////////////////////////////////////////////
{
  //DPF(("HalFindAdapter\n"));

  RtlZeroMemory (pPCI, sizeof (PCI_CONFIGURATION));

  pPCI->usVendorID = PCIVENDOR_PLX;
  pPCI->usDeviceID = PCIDEVICE_LYNX_PATHFINDER;
  //pPCI->usDeviceID  = PCIDEVICE_PLX_9050;

  // call the driver to find the device
  if (!DrvFindPCIDevice (pDriverContext, pPCI))
    {
#ifdef DOS
      pPCI->usDeviceID = PCIDEVICE_PLX_9050;
      if (!DrvFindPCIDevice (pDriverContext, pPCI))
#endif

	//DPF(("DrvFindPCIDevice Failed!\n"));
	return (HSTATUS_CANNOT_FIND_ADAPTER);
    }
  return (HSTATUS_OK);
}
#endif

/////////////////////////////////////////////////////////////////////////////
USHORT
HalOpenAdapter (lynx_devc * devc)
/////////////////////////////////////////////////////////////////////////////
{
  PMBX pMbx;
  PDEVICE pD;
  PADAPTER pA = &devc->adapter;

  USHORT usDevice;
#ifdef MULTI_INPUT
  USHORT usDeviceOrder = 3;	// the first wave device is the 4th in the circular buffer order
#endif

  //DPF(("HalOpenAdapter\n"));

  if (pA->bOpen)
    return (HSTATUS_OK);

  // make sure the ADAPTER structure is clean
  RtlZeroMemory (pA, sizeof (ADAPTER));

  // store the driver context away for future use
  pA->pDriverContext = devc;

  pA->pPLX = devc->plx_ptr;
  pA->pMailbox = (void *) devc->base_ptr;
  pMbx = pA->pMailbox;

  /////////////////////////////////////////////////////////////////////////
  // Setup the Configuration Registers
  /////////////////////////////////////////////////////////////////////////
  //DPF(("Setup Configuration Registers\n"));

  // This must be initalized to ZERO otherwise if the FPGA is loaded and 
  // asserts an interrupt, the machine will hang!
  RegDefineInit (&pA->RegPlxIntCsr, (PULONG) (pA->pPLX + PLX_LCR_INTCSR), 0);
  RegDefine (&pA->RegPlxCntrl, (PULONG) (pA->pPLX + PLX_LCR_CNTRL));
  pA->pulFPGAConfig = (void *) devc->fpga_ptr;

  /////////////////////////////////////////////////////////////////////////
  // Download the FPGA file to the Pathfinder
  /////////////////////////////////////////////////////////////////////////
  if (HalDownloadFile ((PVOID) pA, "PATHFIND.RBF"))
    {
      DPF (("HalDownloadFile Failed!\n"));
      return (HSTATUS_DOWNLOAD_FAILED);
    }

  /////////////////////////////////////////////////////////////////////////
  // Setup the Remaining Registers
  /////////////////////////////////////////////////////////////////////////
  // we cannot read from any of these registers until the FPGA code is running.
  RegDefineInit (&pA->RegDACTL,  & pMbx->Registers.DACTL, REG_DACTL_DAMUTEn | REG_DACTL_ADRSTn);	// | REG_DACTL_ADHDP );
  //RegDefineInit( &pA->RegDACTL, &pMbx->Registers.DACTL, REG_DACTL_DAMUTEn | REG_DACTL_ADRSTn | REG_DACTL_DAAUTOn );
  RegDefineInit (&pA->RegMIDICTL,  & pMbx->Registers.MIDICTL, 0);
  RegDefineInit (&pA->RegAESCTL,  & pMbx->Registers.AESCTL, REG_AESCTL_DORSTn | REG_AESCTL_DICHNL);	// turns on LED
  RegDefine (&pA->RegPLLCTL,  & pMbx->Registers.PLLCTL);

  /////////////////////////////////////////////////////////////////////////
  // Test the SRAM
  /////////////////////////////////////////////////////////////////////////
  {
    PULONG pAddr;
    ULONG i;
    ULONG ulData;
    ULONG ulTestData;

#define TEST_PATTERN	0x1234ABCD

    pAddr = ((PULONG) pMbx) + MEMORY_TEST_OFFSET;
    ulTestData = TEST_PATTERN;
    for (i = 0; i < MEMORY_TEST_SIZE; i++)
      {
	ulTestData += TEST_PATTERN;
	WRITE_REGISTER_ULONG (pAddr + i, ulTestData);
      }
    pAddr = ((PULONG) pMbx) + MEMORY_TEST_OFFSET;
    ulTestData = TEST_PATTERN;
    for (i = 0; i < MEMORY_TEST_SIZE; i++)
      {
	ulTestData += TEST_PATTERN;
	ulData = READ_REGISTER_ULONG (pAddr + i);
	if (ulData != ulTestData)
	  {
	    DPF (("Bad Adapter Ram at %08lx Read [%08lx] XOR [%08lx]\n", i,
		  ulData, ulData ^ ulTestData));
	    return (HSTATUS_BAD_ADAPTER_RAM);
	  }
      }

    /////////////////////////////////////////////////////////////////////////
    // Clear the SRAM
    /////////////////////////////////////////////////////////////////////////
    pAddr = ((PULONG) pMbx) + MEMORY_TEST_OFFSET;
    for (i = 0; i < MEMORY_TEST_SIZE; i++)
      WRITE_REGISTER_ULONG (pAddr + i, 0);
  }

  /////////////////////////////////////////////////////////////////////////
  // Setup internal HAL variables
  /////////////////////////////////////////////////////////////////////////
  pA->bAutoClockSelection = TRUE;

  // setup all of the wave devices
  for (usDevice = 0; usDevice < NUM_WAVE_DEVICES; usDevice++)
    {
      pD = &pA->Device[usDevice];

      pD->usDeviceIndex = usDevice;
      pD->usMode = MODE_IDLE;
      pD->pA = (PVOID) pA;

      //Initialize DitherNoise generators to seed value - different for eachdevice
      // 0 <= seed <65536
      pD->usDitherNoise[0] = usDevice << 1;
      pD->usDitherNoise[1] = (usDevice << 1) + 1;
      pD->usDitherType = MIXVAL_DITHER_NONE;

      switch (usDevice)
	{
	case WAVE_RECORD0_DEVICE:
	case WAVE_PLAY0_DEVICE:
	case WAVE_RECORD1_DEVICE:
	case WAVE_PLAY1_DEVICE:
	  pD->ulDeviceEnableBit = (1 << usDevice);
	  break;
#ifdef MULTI_INPUT		/////////////////////////////////////////////////////
	case WAVE_RECORD2_DEVICE:
	  pD->ulDeviceEnableBit = (1 << 16);
	  break;
	case WAVE_RECORD3_DEVICE:
	  pD->ulDeviceEnableBit = (1 << 17);
	  break;
	case WAVE_RECORD4_DEVICE:
	  pD->ulDeviceEnableBit = (1 << 18);
	  break;
#endif /////////////////////////////////////////////////////
	}

      // assign the wave buffers to each wave device

#ifdef MULTI_INPUT		/////////////////////////////////////////////////////

      pD->pBuffer = (PULONG) (((PBYTE) pMbx) + WAVE_CIRCULAR_BUFFER_OFFSET
			      +
			      ((WAVE_CIRCULAR_BUFFER_SIZE * sizeof (ULONG)) *
			       usDeviceOrder));
      usDeviceOrder++;
      if (usDeviceOrder >= NUM_WAVE_DEVICES)
	usDeviceOrder = 0;

#else /////////////////////////////////////////////////////

      pD->pBuffer = (PULONG) (((PBYTE) pMbx) + WAVE_CIRCULAR_BUFFER_OFFSET
			      +
			      ((WAVE_CIRCULAR_BUFFER_SIZE * sizeof (ULONG)) *
			       usDevice));

#endif /////////////////////////////////////////////////////

      // define the digital audio limit & interrupt register for each device
      // set to zero, disable interrupt, 16 bits
      RegDefineInit (&pD->RegCBLIM,
		     ( & pMbx->Registers.DAR1LIM) + usDevice, 0);

      // assign the circlar buffer pointers to each wave device
#ifdef MULTI_INPUT		/////////////////////////////////////////////////////
      if (usDevice > 3)
	pD->pulHWIndex =
	  ((PULONG) & pMbx->CBPointers.DAR1CBPTR) + usDevice + 1;
      else
#endif /////////////////////////////////////////////////////
	pD->pulHWIndex = (PULONG)(& pMbx->CBPointers.DAR1CBPTR) + usDevice;

      // define the digital audio limit & interrupt register for each device
      // set to zero, disable interrupt, 16 bits
      RegDefineInit (&pD->RegCBLIM,
		     ( & pMbx->Registers.DAR1LIM) + usDevice, 0);
    }

  /////////////////////////////////////////////////////////////////////////
  // Set sample rate & clock source to the default values
  /////////////////////////////////////////////////////////////////////////
  SetSampleRate (pA, 44100, MIXVAL_CLKSRC_INTERNAL, MIXVAL_CLKREF_AUTO,
		 FALSE);

  /////////////////////////////////////////////////////////////////////////
  // Setup any defaults required by Hardware
  /////////////////////////////////////////////////////////////////////////
  CalibrateConverters (pA);

  // make sure the mixer knows how to get back to the adapter
  pA->Mixer.pA = (PVOID) pA;
  HalMixerInit (&pA->Mixer);

#ifndef MULTI_INPUT		/////////////////////////////////////////////////////

  // make sure the midi stuff knows how to get back to the adapter
  for (usDevice = 0; usDevice < NUM_MIDI_DEVICES; usDevice++)
    {
      PMIDI pMidi;

      pMidi = &pA->Midi[usDevice];

      pMidi->usDeviceIndex = usDevice;
      pMidi->usMode = MODE_IDLE;
      pMidi->pA = (PVOID) pA;
    }

  // Zero out the MIDI circular buffer pointers
  WRITE_REGISTER_ULONG (&pMbx->CBPointers.MR1CBPTR, 0);
  WRITE_REGISTER_ULONG (&pMbx->CBPointers.MT1CBPTR, 0);
  WRITE_REGISTER_ULONG (&pMbx->CBPointers.MR2CBPTR, 0);
  WRITE_REGISTER_ULONG (&pMbx->CBPointers.MT2CBPTR, 0);

  // assign the circlar buffer pointers to each midi device
  pA->Midi[MIDI_RECORD0_DEVICE].pulHWIndex =
    (void *) &pMbx->CBPointers.MR1CBPTR;
  pA->Midi[MIDI_RECORD1_DEVICE].pulHWIndex =
    (void *) &pMbx->CBPointers.MR2CBPTR;
  pA->Midi[MIDI_PLAY0_DEVICE].pulHWIndex =
    (void *) &pMbx->CBPointers.MT1CBPTR;
  pA->Midi[MIDI_PLAY1_DEVICE].pulHWIndex =
    (void *) &pMbx->CBPointers.MT2CBPTR;

  // assign the midi buffers to each midi device
  pA->Midi[MIDI_RECORD0_DEVICE].pBuffer =
    (void *) pMbx->MidiBuffer[2].ulBuffer;
  pA->Midi[MIDI_RECORD1_DEVICE].pBuffer =
    (void *) pMbx->MidiBuffer[3].ulBuffer;
  pA->Midi[MIDI_PLAY0_DEVICE].pBuffer = (void *) pMbx->MidiBuffer[0].ulBuffer;
  pA->Midi[MIDI_PLAY1_DEVICE].pBuffer = (void *) pMbx->MidiBuffer[1].ulBuffer;

#endif // MULTI_INPUT

  pA->usSerialNumber = EEPromReadWord (pA, EEPROM_SERIALNUMBER);
  pA->usRevision = EEPromReadWord (pA, EEPROM_REVISION);
  pA->usMonthDay = EEPromReadWord (pA, EEPROM_MONTHDAY);
  pA->usYear = EEPromReadWord (pA, EEPROM_YEAR);

  //DPF(("Serial Number %d Rev %c Date %d/%d/%d\n", pA->usSerialNumber, (char)(pA->usRevision + 0x40), HIBYTE(pA->usMonthDay), LOBYTE(pA->usMonthDay), pA->usYear ));

  //DPF(("Enable Interrupt from PLX\n"));
  // Allow the HW to interrupt the PC
  RegBitSet (&pA->RegPlxIntCsr,
	     PLX_INTCSR_LOCAL_INTERRUPT_1_ENABLE |
	     PLX_INTCSR_LOCAL_INTERRUPT_1_POLARITY |
	     PLX_INTCSR_PCI_ENABLE_INTERRUPT, 1);

  pA->bOpen = TRUE;
  return (HSTATUS_OK);
}

/////////////////////////////////////////////////////////////////////////////
USHORT
HalCloseAdapter (PADAPTER pA)
/////////////////////////////////////////////////////////////////////////////
{

  //DPF(("HalCloseAdapter\n"));

  if (!pA->bOpen)
    return (HSTATUS_ADAPTER_NOT_OPEN);

  // Disallow interrupts to the Host
  RegWrite (&pA->RegPlxIntCsr, 0);

  pA->bOpen = FALSE;

  return (HSTATUS_OK);
}

/////////////////////////////////////////////////////////////////////////////
USHORT
HalAdapterSetASIOMode (PADAPTER pA, BOOLEAN bASIO)
/////////////////////////////////////////////////////////////////////////////
{
  pA->bASIO = bASIO;

  return (HSTATUS_OK);
}

/////////////////////////////////////////////////////////////////////////////
void
DelayuS (PADAPTER pA, ULONG ulCount)
//  1 us delay generated by reads of the FPGA config BAR. The number
//  of read wait states is set to 22 for this BAR which causes at least
//  a 1us PCI transaction cycle length.
/////////////////////////////////////////////////////////////////////////////
{
  ULONG i, ulDummy;

  for (i = 0; i < ulCount; i++)
    ulDummy = READ_REGISTER_ULONG (pA->pulFPGAConfig);
}

/////////////////////////////////////////////////////////////////////////////
USHORT
HalDownloadBuffer (PVOID pContext, PBYTE pBuffer, ULONG ulLoadOffset,
		   ULONG ulLoadLength)
//  pSection        Address in memory of section to load (source)
//  ulLoadOffset    Address on adapter to load section (not used on Pathfinder)
//  ulLoadSize      Length of buffer to load (in bytes)
/////////////////////////////////////////////////////////////////////////////
{
  PADAPTER pA = (PADAPTER) pContext;
  int nDataBit;
  int nBitCount;
  BYTE ucData = 0;
  ULONG ulBitsToSend = ulLoadLength * 8;
  ULONG ulControl;
  PULONG pulFPGAConfig = pA->pulFPGAConfig;

  //DPF(("HalDownloadBuffer\n"));

  // Start configuration - rising edge of nCONFIG starts it
  // This signal will be high after boot-up due to EEPROM config
  RegBitSet (&pA->RegPlxCntrl, REG_FPGA_CONFIG, 0);
  DelayuS (pA, 2);		// 2us min low
  RegBitSet (&pA->RegPlxCntrl, REG_FPGA_CONFIG, 1);
  DelayuS (pA, 5);		// 5us min from nCONFIG hi to first DCLK

  // Check status for errors after nCONFIG is set high
  if (!(RegRead (&pA->RegPlxCntrl) & REG_FPGA_STATUS))
    {
      DPF (("HalDownloadBuffer: CFGSTATn is LO!\n"));
      return (HSTATUS_DOWNLOAD_FAILED);
    }

  nBitCount = 0;
  while (ulBitsToSend--)
    {
      // Read config byte every 8 bits
      if (!nBitCount)
	{
	  ucData = *pBuffer++;	// go get the byte from the buffer
	  nBitCount = 8;	// 8 more bits to go before we read again
	}

      // Get each bit - LSB first
      nDataBit = ucData & 1;
      ucData >>= 1;		// shift next bit into position
      nBitCount--;		// use a bit from the byte

      // Write the bit
      WRITE_REGISTER_ULONG (pulFPGAConfig, nDataBit);
    }

  ulControl = RegRead (&pA->RegPlxCntrl);

  if (!(ulControl & REG_FPGA_CONFIG_DONE))
    {
      DPF (("DONE is LO!\n"));
      return (HSTATUS_DOWNLOAD_FAILED);
    }

  if (!(ulControl & REG_FPGA_STATUS))
    {
      DPF (("CFGSTATn is LO!\n"));
      return (HSTATUS_DOWNLOAD_FAILED);
    }

  return (HSTATUS_OK);
}

#ifdef DEBUG
/////////////////////////////////////////////////////////////////////////////
USHORT
HalAdapterWrite (PADAPTER pA, ULONG ulAddress, PULONG pOutBuffer,
		 ULONG ulSize)
/////////////////////////////////////////////////////////////////////////////
{
  PULONG pWindow;

  // make sure we have a valid adapter structure
  if (!pA)
    return (HSTATUS_ADAPTER_NOT_OPEN);

  // make sure the adapter memory window is ok
  if (!pA->pMailbox)
    return (HSTATUS_ADAPTER_NOT_OPEN);

  if (ulAddress > MEMORY_WINDOW_SIZE)
    return (HSTATUS_INVALID_ADDRESS);

  if (ulAddress + (ulSize * sizeof (ULONG)) > MEMORY_WINDOW_SIZE)
    return (HSTATUS_INVALID_ADDRESS);

  pWindow = (PULONG) (((PBYTE) pA->pMailbox) + ulAddress);

  //DPF(("Going to write to %08lx %08lx %08lx\n", pWindow, (ULONG)pWindow - (ULONG)pA->pMemoryWindow, ulAddress ));

  WRITE_REGISTER_BUFFER_ULONG (pWindow, pOutBuffer, ulSize);

  return (HSTATUS_OK);
}

/////////////////////////////////////////////////////////////////////////////
USHORT
HalAdapterRead (PADAPTER pA, ULONG ulAddress, PULONG pInBuffer, ULONG ulSize)
/////////////////////////////////////////////////////////////////////////////
{
  PULONG pWindow;

  //DPF(("HalAdapterRead\n"));

  // make sure we have a valid adapter structure
  if (!pA)
    return (HSTATUS_ADAPTER_NOT_OPEN);

  // make sure the adapter memory window is ok
  if (!pA->pMailbox)
    return (HSTATUS_ADAPTER_NOT_OPEN);

  if (ulAddress > MEMORY_WINDOW_SIZE)
    {
      DPF (("Invalid Address!\n"));
      return (HSTATUS_INVALID_ADDRESS);
    }

  if (ulAddress + (ulSize * sizeof (ULONG)) > MEMORY_WINDOW_SIZE)
    {
      DPF (("Invalid Address!\n"));
      return (HSTATUS_INVALID_ADDRESS);
    }

  pWindow = (PULONG) (((PBYTE) pA->pMailbox) + ulAddress);

  //DPF(("Going to read from %08lx to %08lx size %08lx\n", ulAddress, pInBuffer, ulSize ));

  READ_REGISTER_BUFFER_ULONG (pWindow, pInBuffer, ulSize);

  return (HSTATUS_OK);
}
#endif

/////////////////////////////////////////////////////////////////////////////
USHORT
HalFindInterrupt (PADAPTER pA, PULONG pulDeviceInterrupt)
//  Stops the device from interrupting the PC and determines which devices 
//  are requesting service.
/////////////////////////////////////////////////////////////////////////////
{
  PMBX pMbx = pA->pMailbox;
  ULONG ulInterrupt;

  // make sure we have a valid adapter structure
  if (!pA)
    return (HSTATUS_ADAPTER_NOT_OPEN);

#ifdef DOS
  // read the GSTAT register from the hardware
  ulInterrupt =
    (READ_REGISTER_ULONG (&pMbx->Registers.GSTAT) & REG_ISTAT_INTERRUPT_MASK);
  if (ulInterrupt)
    ulInterrupt =
      (READ_REGISTER_ULONG (&pMbx->Registers.ISTAT) &
       REG_ISTAT_INTERRUPT_MASK);
#else
  // read the ISTAT register from the hardware
  ulInterrupt =
    (READ_REGISTER_ULONG (&pMbx->Registers.ISTAT) & REG_ISTAT_INTERRUPT_MASK);
#endif

#ifdef MULTI_INPUT
  // move the 3 extra input interrupt bits over to the left
  ulInterrupt |= (ulInterrupt >> 9);
  ulInterrupt &= 0x7F;
#endif

  // if the adapter isn't opened, then this must be a test interrupt
  if (!pA->bOpen)
    {
      if (ulInterrupt)
	{
	  ulInterrupt = 0;
	}
    }

  *pulDeviceInterrupt = ulInterrupt;

  return (HSTATUS_OK);
}

/////////////////////////////////////////////////////////////////////////////
USHORT
EnableDevices (PADAPTER pA, ULONG ulDeviceBitMask)
// Puts the devices into a state ready to be started
/////////////////////////////////////////////////////////////////////////////
{
  int nDevice;

  // for each device getting started
  for (nDevice = 0; nDevice < NUM_WAVE_DEVICES; nDevice++)
    {
      PDEVICE pD = &pA->Device[nDevice];

      // is this device getting started?
      if ((1 << nDevice) & ulDeviceBitMask)
	{
	  // Disable this device
	  RegBitSet (&pA->RegDACTL, pD->ulDeviceEnableBit, 0);

	  // zero the CB pointers
	  WRITE_REGISTER_ULONG (pD->pulHWIndex, 0);
	  pD->ulLastHWIndex = 0;

	  // The Circular Buffer Limit should have already been set by the PRELOAD or RECORD mode set.

	  // Enable the Interrupt for this device
/*
			if( pA->bASIO )
			{
				//DPF(("EnableDevice: ASIO Mode\n"));

				if( nDevice == WAVE_PLAY0_DEVICE )	// master device
				{
					//DPF(("Enabling Interrupt for PLAY0\n"));
					RegBitSet( &pD->RegCBLIM, REG_CBLIM_INTEN, 1 );
				}
			}
			else	// MME/DirectSound Mode
*/
	  {
	    RegBitSet (&pD->RegCBLIM, REG_CBLIM_INTEN, 1);
	  }
	}
    }

  return (HSTATUS_OK);
}

static ULONG gulAdapterSyncGroup = 0;
static ULONG gulAdapterSyncReady = 0;

/////////////////////////////////////////////////////////////////////////////
USHORT
SyncStartDevices (PVOID pDriverContext)
// Try to start all of the devices in the sync start group across all adapters.
/////////////////////////////////////////////////////////////////////////////
{
  PADAPTER pA;
  int nAdapter;
  PULONG pulGSTAT;
  int nDevice;

  // if there are no adapters in the sync start group, just return
  if (!gulAdapterSyncGroup)
    return (HSTATUS_OK);

  // if all the adapters in the group are not ready, just return
  if (gulAdapterSyncGroup != gulAdapterSyncReady)
    return (HSTATUS_OK);

  //DPF(("SyncStart %08lx\n", gulSyncStart ));

  // scan thru each opened adapter
  for (nAdapter = 0; nAdapter < MAX_NUMBER_OF_ADAPTERS; nAdapter++)
    {
      // does this adapter have any devices in the sync group?
      if ((1 << nAdapter) & gulAdapterSyncGroup)
	{
	  pA = DrvGetAdapter (pDriverContext, nAdapter);
	  if (!pA)
	    continue;

	  // go enable all the devices in the sync group for this adapter
	  EnableDevices (pA, pA->ulSyncGroup);
	}
    }
  // pA is undefined at this point

  // go get the first adapter
  pA = DrvGetAdapter (pDriverContext, 0);

  pulGSTAT = (PULONG) & pA->pMailbox->Registers.GSTAT;

  // Wait for L/R clock to transition
  // if LRCK is currently high, wait for it to go low
  while (!(READ_REGISTER_ULONG (pulGSTAT) & REG_GSTAT_LRCK))
    ;
  // LRCK is low, wait for it to go high
  while ((READ_REGISTER_ULONG (pulGSTAT) & REG_GSTAT_LRCK))
    ;
  // ok, we should be safe to start the adapters now

  // Start the adapters
  for (nAdapter = 0; nAdapter < MAX_NUMBER_OF_ADAPTERS; nAdapter++)
    {
      // does this adapter have any devices in the sync group?
      if ((1 << nAdapter) & gulAdapterSyncGroup)
	{
	  ULONG ulDeviceEnableBitmask = 0;

	  pA = DrvGetAdapter (pDriverContext, nAdapter);
	  if (!pA)
	    continue;

	  // scan thru all the devices on this adapter and build up the enable bitmap
	  for (nDevice = 0; nDevice < NUM_WAVE_DEVICES; nDevice++)
	    // is this device getting started?
	    if ((1 << nDevice) & pA->ulSyncGroup)
	      ulDeviceEnableBitmask |= pA->Device[nDevice].ulDeviceEnableBit;

	  // enable the devices on this adapter
	  RegBitSet (&pA->RegDACTL,
		     (ulDeviceEnableBitmask & REG_DACTL_DAENMASK), 1);

	  // done with this adapter, reset for next time
	  pA->ulSyncGroup = 0;
	  pA->ulSyncReady = 0;
	}
    }
  // pA is undefined at this point

  // reset for next time
  gulAdapterSyncGroup = 0;
  gulAdapterSyncReady = 0;

  return (HSTATUS_OK);
}


/////////////////////////////////////////////////////////////////////////////
USHORT
HalDeviceSetMode (PDEVICE pD, USHORT usDeviceMode)
// The ENABLE bit will always be on for record devices
/////////////////////////////////////////////////////////////////////////////
{
  PADAPTER pA = (PADAPTER) pD->pA;

  switch (usDeviceMode)
    {
    case MODE_PRELOAD:
      //DPF(("MODE_PRELOAD\n"));  // just set the mode and return
      // set the indexes to zero
      WRITE_REGISTER_ULONG (pD->pulHWIndex, 0);
      pD->ulPCIndex = 0;
      pD->ulLastHWIndex = 0;
      pD->ulInterruptSamples = 0;

      // Setup the Dither Mode (bit depth does not change for playback)
      pD->usDitherType = (USHORT) pA->ulPlayDither;
      break;
    case MODE_IDLE:
      //DPF(("MODE_IDLE\n"));

      // remove this device from the sync group
      CLR (pA->ulSyncReady, (1 << pD->usDeviceIndex));
      CLR (pA->ulSyncGroup, (1 << pD->usDeviceIndex));

      // if we just cleared ourselves from the entire sync group for this adapter
      // make sure we take this adapter out of the global sync group
      if (!pA->ulSyncGroup)
	{
	  CLR (gulAdapterSyncReady, (1 << pA->usAdapterIndex));
	  CLR (gulAdapterSyncGroup, (1 << pA->usAdapterIndex));
	}

      // Disable the Interrupt for this device
      RegBitSet (&pD->RegCBLIM, REG_CBLIM_INTEN, 0);

      // Disable this device
      RegBitSet (&pA->RegDACTL, pD->ulDeviceEnableBit, 0);

      // we could have just removed ourselves from the sync start group, so 
      // go check the group again to see if we should start anyone else.
      SyncStartDevices (pA->pDriverContext);

      // make sure the levels are set to zero
      pD->sLeftLevel = pD->sRightLevel = 0;
      // set the indexes to zero
      WRITE_REGISTER_ULONG (pD->pulHWIndex, 0);
      pD->lHWIndex = 0;
      pD->ulPCIndex = 0;
      pD->ulLastHWIndex = 0;
      pD->ulInterruptSamples = 0;

      if (pA->bMonitorOffPlay)
	{
	  if ((pD->usDeviceIndex == WAVE_PLAY0_DEVICE)
	      && (pD->lPreviousMonitorState != -1))
	    {
	      //DPF(("MONITOR OFF PLAY: Restoring Analog Monitor\n"));
	      pA->bAnalogMonitor = (BOOLEAN) pD->lPreviousMonitorState;
	      RegBitSet (&pA->RegDACTL, REG_DACTL_DAT1MIXEN,
			 pA->bAnalogMonitor);
	      pD->lPreviousMonitorState = -1;	// kill off the previous monitor state
	    }
	  if ((pD->usDeviceIndex == WAVE_PLAY1_DEVICE)
	      && (pD->lPreviousMonitorState != -1))
	    {
	      //DPF(("MONITOR OFF PLAY: Restoring Digital Monitor\n"));
	      pA->bDigitalMonitor = (BOOLEAN) pD->lPreviousMonitorState;
	      RegBitSet (&pA->RegDACTL, REG_DACTL_DAT2MIXEN,
			 pA->bDigitalMonitor);
	      pD->lPreviousMonitorState = -1;	// kill off the previous monitor state
	    }
	}
      break;
    case MODE_RECORD:
      // Setup the Dither Mode
      pD->usDitherType = (USHORT) pA->ulRecordDither;
      if (pD->usDitherType != MIXVAL_DITHER_NONE)
	{
	  pD->b32BitMode = 1;
	  // update the 16/32 bit status for this device
	  RegBitSet (&pD->RegCBLIM, REG_CBLIM_LEN32, pD->b32BitMode);
	}
      // if pD->ulInterruptSamples is set for a record device, then the interrupt point has already been set
      if (!pD->ulInterruptSamples)
	{
	  // set the interrupt point for the record device
	  pD->lHWIndex = (WAVE_CIRCULAR_BUFFER_SIZE / 2);
	  RegWriteMask (&pD->RegCBLIM, REG_CBLIM_MASK, pD->lHWIndex);	// in ASIO mode this gets ignored
	}
      pD->ulPCIndex = 0;
      // fall-through...
    case MODE_PLAY:
      //DPF(("MODE_PLAY/RECORD\n"));
#ifdef MULTI_INPUT
      // if this devices input source is digital
      if (pD->ulInputSource)
#else
      // if we are trying to record on the digital input, change the clock source
      if (pD->usDeviceIndex == WAVE_RECORD1_DEVICE)
#endif
	{
	  USHORT usStatus;

	  // remember the old values
	  pA->ulPreviousClockSource = pA->ulClockSource;
	  pA->ulPreviousClockReference = pA->ulClockReference;
	  // usMode should still be MODE_IDLE so we can do this right now...
	  usStatus =
	    SetSampleRate (pA, pA->ulCurrentSampleRate, MIXVAL_CLKSRC_DIGITAL,
			   MIXVAL_CLKREF_AUTO, FALSE);
	  if (usStatus)
	    return (usStatus);
	}

      // if Monitor Off Play is enabled, turn off monitor
      if (pA->bMonitorOffPlay)
	{
	  if ((pD->usDeviceIndex == WAVE_PLAY0_DEVICE)
	      && (pD->lPreviousMonitorState == -1))
	    {
	      //DPF(("MONITOR OFF PLAY: Disable Analog Monitor\n"));
	      pD->lPreviousMonitorState = pA->bAnalogMonitor;
	      pA->bAnalogMonitor = FALSE;
	      RegBitSet (&pA->RegDACTL, REG_DACTL_DAT1MIXEN,
			 pA->bAnalogMonitor);
	    }
	  if ((pD->usDeviceIndex == WAVE_PLAY1_DEVICE)
	      && (pD->lPreviousMonitorState == -1))
	    {
	      //DPF(("MONITOR OFF PLAY: Disable Digital Monitor\n"));
	      pD->lPreviousMonitorState = pA->bDigitalMonitor;
	      pA->bDigitalMonitor = FALSE;
	      RegBitSet (&pA->RegDACTL, REG_DACTL_DAT2MIXEN,
			 pA->bDigitalMonitor);
	    }
	}

      // is this device part of the sync start group?
      if (pA->ulSyncGroup & (1 << pD->usDeviceIndex))
	{
	  // Make this device ready for sync start
	  SET (pA->ulSyncReady, (1 << pD->usDeviceIndex));
	  // if all the devices on this adapter are ready, inform the global variable
	  if (pA->ulSyncGroup == pA->ulSyncReady)
	    SET (gulAdapterSyncReady, (1 << pA->usAdapterIndex));

	  // And try to start the group
	  SyncStartDevices (pA->pDriverContext);
	}
      else
	{
	  //DPF(("Normal Start\n"));
	  EnableDevices (pA, 1 << pD->usDeviceIndex);

	  // Enable this device
	  //DPF(("Start the Device\n"));
	  RegBitSet (&pA->RegDACTL, pD->ulDeviceEnableBit, 1);
	}
      break;
    case MODE_RECORD_OPEN:	// record device has been opened
      if (pA->bMonitorOnRecord && !pA->ulMonitorState)
	{
	  pA->ulMonitorState = (1 << 2);	// don't allow anyone to change this until all record devices go to IDLE.
	  pA->ulMonitorState |= (pA->bAnalogMonitor << 0);
	  pA->ulMonitorState |= (pA->bDigitalMonitor << 1);

	  if (pA->ulMonitorSource != MIXVAL_MONSRC_ANALOGOUT)
	    pA->bAnalogMonitor = TRUE;

	  if (pA->ulMonitorSource != MIXVAL_MONSRC_DIGITALOUT)
	    pA->bDigitalMonitor = TRUE;

	  //DPF(("RECORD_OPEN: Enabling Monitors\n"));
	  RegBitSet (&pA->RegDACTL, REG_DACTL_DAT1MIXEN, pA->bAnalogMonitor);
	  RegBitSet (&pA->RegDACTL, REG_DACTL_DAT2MIXEN, pA->bDigitalMonitor);
	}
      return (HSTATUS_OK);
    case MODE_RECORD_CLOSE:	// record device has been closed
      if (pA->bMonitorOnRecord &&
	  (pA->Device[WAVE_RECORD0_DEVICE].usMode == MODE_IDLE) &&
	  (pA->Device[WAVE_RECORD1_DEVICE].usMode == MODE_IDLE)
#ifdef MULTI_INPUT
	  && (pA->Device[WAVE_RECORD2_DEVICE].usMode == MODE_IDLE)
	  && (pA->Device[WAVE_RECORD3_DEVICE].usMode == MODE_IDLE)
	  && (pA->Device[WAVE_RECORD4_DEVICE].usMode == MODE_IDLE)
#endif
	)
	{
	  pA->bAnalogMonitor = (BOOLEAN) ((pA->ulMonitorState >> 0) & 0x1);
	  pA->bDigitalMonitor = (BOOLEAN) ((pA->ulMonitorState >> 1) & 0x1);

	  //DPF(("RECORD_CLOSE: Disabling Monitors\n"));
	  RegBitSet (&pA->RegDACTL, REG_DACTL_DAT1MIXEN, pA->bAnalogMonitor);
	  RegBitSet (&pA->RegDACTL, REG_DACTL_DAT2MIXEN, pA->bDigitalMonitor);

	  pA->ulMonitorState = 0;	// allow this to be changed again
	}
      return (HSTATUS_OK);
    case MODE_SYNCSTART:	// driver is requesting this device go into "sync start" mode
      if (pA->bSyncStart)
	{
	  // join the sync group for this device on the adapter
	  SET (pA->ulSyncGroup, (1 << pD->usDeviceIndex));
	  // join the sync group for this adapter 
	  SET (gulAdapterSyncGroup, (1 << pA->usAdapterIndex));
	}
      return (HSTATUS_OK);
    default:
      return (HSTATUS_INVALID_MODE);
    }

  // make sure we update the device mode
  pD->usMode = usDeviceMode;

  // if we are going to IDLE on the digital input, try and change the clock source back.
  // we don't care if SetSampleRate fails, we are trying to switch back to Internal (probably)
  if (pA->bAutoClockSelection)
    {
#ifdef MULTI_INPUT
      // if this devices input source is digital
      if (pD->ulInputSource && (pD->usMode == MODE_IDLE))
#else
      if ((pD->usDeviceIndex == WAVE_RECORD1_DEVICE)
	  && (pD->usMode == MODE_IDLE))
#endif
	{
	  SetSampleRate (pA, pA->ulCurrentSampleRate,
			 pA->ulPreviousClockSource,
			 pA->ulPreviousClockReference, FALSE);
	}
    }

  return (HSTATUS_OK);
}

/////////////////////////////////////////////////////////////////////////////
USHORT
HalDeviceSetFormat (PDEVICE pD, PWAVEFORMATEX pWaveFormat)
/////////////////////////////////////////////////////////////////////////////
{
  PADAPTER pA = (PADAPTER) pD->pA;

  //DPF(("HalDeviceSetFormat\n"));

  // must check to see if device is idle first
  if (!pA->bASIO)		// don't limit changing the format if in ASIO mode
    {
      if (pD->usMode != MODE_IDLE)
	{
	  DPF (("HalDeviceSetFormat: Device Not IDLE!\n"));
	  return (HSTATUS_INVALID_MODE);
	}
    }

  // Validate wFormatTag field
  if (pWaveFormat->wFormatTag != WAVE_FORMAT_PCM)
    {
      DPF (("Format Not PCM!\n"));
      return (HSTATUS_INVALID_FORMAT);
    }

  // Validate wBitsPerSample field
  if (pWaveFormat->wBitsPerSample == 8)
    pD->b32BitMode = 0;

  else if (pWaveFormat->wBitsPerSample == 16)
    pD->b32BitMode = 0;

  else if (pWaveFormat->wBitsPerSample == 24)
    pD->b32BitMode = 1;

  else if (pWaveFormat->wBitsPerSample == 32)
    pD->b32BitMode = 1;

  else
    {
      DPF (("Format Not 8, 16, 24 or 32 bits!\n"));
      return (HSTATUS_INVALID_FORMAT);
    }

  // update the 16/32 bit status for this device
  RegBitSet (&pD->RegCBLIM, REG_CBLIM_LEN32, pD->b32BitMode);

  // Validate nChannels field
  if ((pWaveFormat->nChannels < 1) || (pWaveFormat->nChannels > 2))
    {
      DPF (("nChannels Invalid!\n"));
      return (HSTATUS_INVALID_FORMAT);
    }

  // if the base sample rate is different than the current sample rate
  if (pA->ulCurrentSampleRate != pWaveFormat->nSamplesPerSec)
    {
      int nDevice;
      // if any devices is not in MODE_IDLE, refuse to change the sample rate
      for (nDevice = 0; nDevice < NUM_WAVE_DEVICES; nDevice++)
	{
	  if (pA->Device[nDevice].usMode != MODE_IDLE)
	    {
	      DPF (("HalDeviceSetFormat: Device Not Idle!\n"));
	      return (HSTATUS_INVALID_SAMPLERATE);
	    }
	}
    }

  // Set the sample rate, no matter what
  if (SetSampleRate
      (pA, pWaveFormat->nSamplesPerSec, pA->ulClockSource,
       pA->ulClockReference, FALSE))
    {
      DPF (("SetSampleRate Failed!\n"));
      return (HSTATUS_INVALID_SAMPLERATE);
    }

  // remember the format structure for our device
  RtlCopyMemory (&pD->WaveFormat, pWaveFormat, sizeof (PCMWAVEFORMAT));
  //pD->WaveFormat = *pWaveFormat;

  return (HSTATUS_OK);
}

/////////////////////////////////////////////////////////////////////////////
USHORT
HalDeviceValidateFormat (PDEVICE pD, PWAVEFORMATEX pWaveFormat)
/////////////////////////////////////////////////////////////////////////////
{
  ULONG ulMaxSampleRate;

  // Validate wFormatTag field
  if (pWaveFormat->wFormatTag != WAVE_FORMAT_PCM)
    return (HSTATUS_INVALID_FORMAT);

  // Validate wBitsPerSample field
  if ((pWaveFormat->wBitsPerSample != 8)
      && (pWaveFormat->wBitsPerSample != 16)
      && (pWaveFormat->wBitsPerSample != 24)
      && (pWaveFormat->wBitsPerSample != 32))
    {
      DPF (("Validate: Format Not 8, 16, 24 or 32 bits!\n"));
      return (HSTATUS_INVALID_FORMAT);
    }

  // Validate nChannels field
  if ((pWaveFormat->nChannels < 1) || (pWaveFormat->nChannels > 2))
    return (HSTATUS_INVALID_FORMAT);

  // Validate nSamplesPerSec field
  if (pWaveFormat->nSamplesPerSec < MIN_SAMPLE_RATE)
    return (HSTATUS_INVALID_SAMPLERATE);

  // is this an analog device?
  if (pD->usDeviceIndex <= WAVE_PLAY0_DEVICE)
    ulMaxSampleRate = MAX_ANALOG_RATE;
  else
    ulMaxSampleRate = MAX_SAMPLE_RATE;

  if (pWaveFormat->nSamplesPerSec > ulMaxSampleRate)
    return (HSTATUS_INVALID_SAMPLERATE);

  // Validate nAvgBytesPerSec

  // Validate nBlockAlign
  if (pWaveFormat->nBlockAlign !=
      ((pWaveFormat->wBitsPerSample * pWaveFormat->nChannels) / 8))
    return (HSTATUS_INVALID_FORMAT);

  return (HSTATUS_OK);
}

/////////////////////////////////////////////////////////////////////////////
USHORT
HalDeviceGetTransferSize (PDEVICE pD, PULONG pulTransferSize,
			  PULONG pulCircularBufferSize)
//  Gets the size in bytes of the data that is to be transferred.
//  pulTransferSize is always in HOST bytes
/////////////////////////////////////////////////////////////////////////////
{
  LONG lPCIndex = pD->ulPCIndex;
  LONG lHWIndex;
  LONG lSize;

  // if we are using pre-programmed interrupt sizes, don't read the actual hardware
  if (!pD->ulInterruptSamples)
    pD->lHWIndex = READ_REGISTER_ULONG (pD->pulHWIndex) >> 1;	// adjust for HWIndex being 16 bit aligned

  lHWIndex = pD->lHWIndex;

  //DPF(("HalDeviceGetTransferSize %lu\n", (ULONG)WAVE_CIRCULAR_BUFFER_SIZE ));

  *pulCircularBufferSize = (WAVE_CIRCULAR_BUFFER_SIZE - 1);

  // if we are in 32 bit mode then we can only fit half as many samples in the buffer
  if (pD->b32BitMode)
    *pulCircularBufferSize /= 2;

  // Circular Buffer Size is now in samples, convert to bytes
  *pulCircularBufferSize *= pD->WaveFormat.nBlockAlign;

  switch (pD->usMode)
    {
    default:
    case MODE_IDLE:
    case MODE_PRELOAD:
      lSize = WAVE_CIRCULAR_BUFFER_SIZE - 1;
      break;
    case MODE_RECORD:
      lSize = lHWIndex - lPCIndex;
      // lSize == 0 means no data is waiting
      if (lSize < 0)
	lSize += WAVE_CIRCULAR_BUFFER_SIZE;
#ifdef DEBUG
      if (!lSize)
	DPF (("HW Error: Record interrupt generated when no data is available!\n"));
#endif
      break;
    case MODE_PLAY:
      lSize = lHWIndex - lPCIndex;
      // lSize == 0 means the buffer can be completely filled
      if (lSize <= 0)
	lSize += WAVE_CIRCULAR_BUFFER_SIZE;
      lSize--;			// make sure we don't fill the buffer
      break;
    }

  //DPF(( "H[%ld] P[%ld] ", lHWIndex, lPCIndex ));

  // make sure we always leave one DWORD free
  if (lSize >= WAVE_CIRCULAR_BUFFER_SIZE)
    lSize = WAVE_CIRCULAR_BUFFER_SIZE - 1;

  // if we are in 32 bit mode then we can only fit half as many samples 
  // in the buffer, so we must adjust lSize by a factor of 2
  if (pD->b32BitMode)
    lSize /= 2;

  // lSize is now in samples, convert to bytes
  lSize *= pD->WaveFormat.nBlockAlign;

  *pulTransferSize = lSize;
  return (HSTATUS_OK);
}

/////////////////////////////////////////////////////////////////////////////
USHORT
HalDeviceTransferAudio (PDEVICE pD, PBYTE pBuffer, ULONG ulBufferSize,
			PULONG pulBytesTransfered)
//  Reads/Writes audio data to/from the device depending on the device mode.
/////////////////////////////////////////////////////////////////////////////
{
#define RNDBY8	0x0080		//2^7, 8 bit shift used
#define RNDBY16	0x00008000L	//2^15, 16 bit shift used
#define RNDBY24	0x00800000L	//2^23, 24 bit shift used
  PULONG pulBuffer = pD->pBuffer;	// the devices buffer
  ULONG ulPCIndex = pD->ulPCIndex;
  SHORT sPendingLeftLevel = 0;	// the level that is being calculated for this packet
  SHORT sPendingRightLevel = 0;	// the level that is being calculated for this packet
  LONG lLeftVolume;
  LONG lRightVolume;
  BOOLEAN bLevels = ((PADAPTER) pD->pA)->bLevels;
  BOOLEAN bLVolume, bRVolume;
  WORD wBitsPerSample = pD->WaveFormat.wBitsPerSample;
  WORD nChannels = pD->WaveFormat.nChannels;
  ULONG ulSamplesToTransfer;
  register SHORT sLSample, sRSample;
  register LONG lLSample, lRSample;
  register ULONG ulPacked;
  LONG lSum;
  LONGLONG llSum;		// Only a problem for DOS

  // Dither generator components
  USHORT usRandom[2], usA[2], usC[2];
  USHORT usDitherType = pD->usDitherType;
  USHORT usLastRandom[2];	// Stores Noise(n-1)
  LONG lLDither = 0, lRDither = 0;

  usRandom[0] = pD->usDitherNoise[0];
  usRandom[1] = pD->usDitherNoise[1];

  //Initialize noise gen multiplier and increment
  //Carefully chosen to create a non-correlated pseudo-random
  //sequence for each device
  usA[0] = (pD->usDeviceIndex << 3) + 5;	//5,13,21,29
  usA[1] = (pD->usDeviceIndex << 3) + 9;	//9,17,25,33
  usC[0] = 17 - (pD->usDeviceIndex << 2);	//17,13,9,5
  usC[1] = 15 - (pD->usDeviceIndex << 2);	//15,11,7,3

  //Generate 16-bit RPDF (random) noise using Linear Congruential method
  //X(n+1) = (a * X(n) + c) mod 16
#define MAKE_DITHER \
	usLastRandom[0] = usRandom[0];\
	usLastRandom[1] = usRandom[1];\
	usRandom[0] = usA[0] * usLastRandom[0] + usC[0];	\
	usRandom[1] = usA[1] * usLastRandom[1] + usC[1];	\
	switch( usDitherType )					\
	{							\
	case MIXVAL_DITHER_NONE:				\
		lLDither = lRDither = RNDBY16;			\
		break;						\
	case MIXVAL_DITHER_RECTANGULAR_PDF:			\
		lLDither = (LONG)(SHORT)usRandom[0];		\
		lRDither = (LONG)(SHORT)usRandom[1];		\
		break;						\
	case MIXVAL_DITHER_TRIANGULAR_PDF:			\
		lLDither = (LONG)(SHORT)usRandom[0] + (LONG)(SHORT)usRandom[1] + RNDBY16;\
		lRDither = (LONG)(SHORT)usRandom[0] - (LONG)(SHORT)usRandom[1] + RNDBY16;	\
		break;						\
	case MIXVAL_DITHER_TRIANGULAR_NS_PDF:			\
		lLDither = (LONG)(SHORT)usRandom[0] - (LONG)(SHORT)usLastRandom[0] + RNDBY16;	\
		lRDither = (LONG)(SHORT)usRandom[1] - (LONG)(SHORT)usLastRandom[1] + RNDBY16;	\
		break;	\
	}

  /* =============== Dither Operational Notes =======================
   * Dither word width is 16-bits and resides in the lower word
   of a LONG.
   * For the dither addition, the sample to be processed must be
   aligned so that its final LSB is shifted left by 16.
   * The word width of the result of rounding or dithering always
   matches the format word width, i.e., additional bits created
   by a volume calc are processed to the width specified by FMT.

   MODE   FMT DITHER  32BMode PROCESSING
   --------------------------------------------------------------------
   Record PCM8    NO  NO  Round always
   Record PCM8    YES YES Dither always
   Record PCM16   NO  NO  Round for volume only
   Record PCM16   YES YES Dither always
   Record PCM24   NO  YES Round for volume only
   Record PCM24   YES YES Dither for volume only
   Record PCM32   NO  YES Round for volume only
   Record PCM32   YES YES Dither for volume only

   Play   PCM8    NO  NO  Round for volume only
   Play   PCM8    YES NO  Dither for volume only
   Play   PCM16   NO  NO  Round for volume only
   Play   PCM16   YES NO  Dither for volume only
   Play   PCM24   NO  YES Round for volume only
   Play   PCM24   YES YES Dither for volume only
   Play   PCM32   NO  YES Round for volume only
   Play   PCM32   YES YES Dither for volume only

   Missing mode: rounding for PCM16 record without volume
   ==================================================================*/

  //DPF(("HalDeviceTransferAudio %lu bytes\n", ulBufferSize ));

  switch (pD->usMode)
    {
    case MODE_PRELOAD:
    case MODE_PLAY:
    case MODE_RECORD:
      break;
    default:
      DPF (("HalDeviceTransferAudio: Invalid Mode\n"));
      return (HSTATUS_INVALID_MODE);
    }

  if (pD->bMute)
    {
      lLeftVolume = lRightVolume = 0;
    }
  else
    {
      lLeftVolume = pD->lLeftVolume;
      lRightVolume = pD->lRightVolume;
    }

  // if both volume is full scale, don't compute
  if (lLeftVolume == 0xFFFF)
    bLVolume = FALSE;
  else
    bLVolume = TRUE;

  if (lRightVolume == 0xFFFF)
    bRVolume = FALSE;
  else
    bRVolume = TRUE;


  // Convert the buffer size into the number of samples to transfer
  ulSamplesToTransfer = ulBufferSize / pD->WaveFormat.nBlockAlign;

  // inform the driver how much we actually used
  *pulBytesTransfered = ulSamplesToTransfer * pD->WaveFormat.nBlockAlign;

  if (pD->usMode == MODE_RECORD)
    {
      switch (wBitsPerSample)
	{
	case 8:
	  if (usDitherType == MIXVAL_DITHER_NONE)
	    {
	      register PBYTE pDst = pBuffer;

	      while (ulSamplesToTransfer--)
		{
		  ulPacked = READ_REGISTER_ULONG (&pulBuffer[ulPCIndex++]);
		  if (ulPCIndex >= WAVE_CIRCULAR_BUFFER_SIZE)
		    ulPCIndex = 0;

		  lLSample = (LONG) (short) LOWORD (ulPacked);
		  lRSample = (LONG) (short) HIWORD (ulPacked);
#ifndef MULTI_INPUT
		  if (bLVolume)
		    lLSample = (lLSample * lLeftVolume) >> 8;
		  else
		    lLSample <<= 8;	//align for rounding addition

		  if (bRVolume)
		    lRSample = (lRSample * lRightVolume) >> 8;
		  else
		    lRSample <<= 8;	//align for rounding addition
#else
		  lLSample <<= 8;	//align for rounding addition
		  lRSample <<= 8;	//align for rounding addition
#endif

		  lLSample = (lLSample + RNDBY16) >> 8;	//sample in byte 2, dither in byte 0,1

		  // Saturation logic
		  if (lLSample > 32767)
		    lLSample = 32767;
		  if (lLSample < -32768)
		    lLSample = -32768;

		  lRSample = (lRSample + RNDBY16) >> 8;	//sample in byte 2, dither in byte 0,1

		  // Saturation logic
		  if (lRSample > 32767)
		    lRSample = 32767;
		  if (lRSample < -32768)
		    lRSample = -32768;

		  sLSample = (SHORT) (lLSample);
		  sRSample = (SHORT) (lRSample);

		  if (bLevels)
		    {
		      if (Abs (sLSample) > Abs (sPendingLeftLevel))
			sPendingLeftLevel = sLSample;
		      if (Abs (sRSample) > Abs (sPendingRightLevel))
			sPendingRightLevel = sRSample;
		    }

		  // 16 signed to 8 unsigned conversion
		  if (nChannels > 1)
		    {
		      *pDst++ = (BYTE) ((sLSample >> 8) + 128);
		      *pDst++ = (BYTE) ((sRSample >> 8) + 128);
		    }
		  else
		    {
		      lSum = (LONG) sLSample + (LONG) sRSample;

		      // make sure we don't over saturate the value
		      if (lSum > 32767)
			lSum = 32767;
		      if (lSum < -32768)
			lSum = -32768;

		      *pDst++ = (BYTE) ((((USHORT) lSum) >> 8) + 128);
		    }
		}		// while
	    }
	  else			//dither enabled
	    {
	      register PBYTE pDst = pBuffer;

	      while (ulSamplesToTransfer--)
		{
		  lLSample = READ_REGISTER_ULONG (&pulBuffer[ulPCIndex++]);
		  lRSample = READ_REGISTER_ULONG (&pulBuffer[ulPCIndex++]);
		  if (ulPCIndex >= WAVE_CIRCULAR_BUFFER_SIZE)
		    ulPCIndex = 0;

#ifndef MULTI_INPUT
		  if (bLVolume)
		    lLSample =
		      (LONG) (((LONGLONG) lLSample * lLeftVolume) >> 24);
		  else
		    lLSample >>= 8;	//align for dither addition

		  if (bRVolume)
		    lRSample =
		      (LONG) (((LONGLONG) lRSample * lRightVolume) >> 24);
		  else
		    lRSample >>= 8;	//align for dither addition
#else
		  lLSample >>= 8;	//align for dither addition
		  lRSample >>= 8;	//align for dither addition
#endif

		  MAKE_DITHER;	//generate L/R dither - in low word of long

		  lLSample = (lLSample + lLDither) >> 8;	//sample in byte 2, dither in byte 0,1

		  // Saturation logic
		  if (lLSample > 32767)
		    lLSample = 32767;
		  if (lLSample < -32768)
		    lLSample = -32768;

		  lRSample = (lRSample + lRDither) >> 8;	//sample in byte 2, dither in byte 0,1

		  // Saturation logic
		  if (lRSample > 32767)
		    lRSample = 32767;
		  if (lRSample < -32768)
		    lRSample = -32768;

		  sLSample = (SHORT) (lLSample);
		  sRSample = (SHORT) (lRSample);

		  if (bLevels)
		    {
		      if (Abs (sLSample) > Abs (sPendingLeftLevel))
			sPendingLeftLevel = sLSample;
		      if (Abs (sRSample) > Abs (sPendingRightLevel))
			sPendingRightLevel = sRSample;
		    }

		  // 16 signed to 8 unsigned conversion
		  if (nChannels > 1)
		    {
		      *pDst++ = (BYTE) ((sLSample >> 8) + 128);
		      *pDst++ = (BYTE) ((sRSample >> 8) + 128);
		    }
		  else
		    {
		      lSum = (LONG) sLSample + (LONG) sRSample;

		      // make sure we don't over saturate the value
		      if (lSum > 32767)
			lSum = 32767;
		      if (lSum < -32768)
			lSum = -32768;

		      *pDst++ = (BYTE) ((((USHORT) lSum) >> 8) + 128);
		    }
		}
	    }
	  break;
	case 16:
	  if (usDitherType == MIXVAL_DITHER_NONE)
	    {
	      register PSHORT pDst = (PSHORT) pBuffer;

	      while (ulSamplesToTransfer--)
		{
		  ulPacked = READ_REGISTER_ULONG (&pulBuffer[ulPCIndex++]);
		  if (ulPCIndex >= WAVE_CIRCULAR_BUFFER_SIZE)
		    ulPCIndex = 0;

		  sLSample = LOWORD (ulPacked);
		  sRSample = HIWORD (ulPacked);

#ifndef MULTI_INPUT
		  if (bLVolume)
		    sLSample =
		      (SHORT) (((LONG) sLSample * lLeftVolume +
				RNDBY16) >> 16);
		  if (bRVolume)
		    sRSample =
		      (SHORT) (((LONG) sRSample * lRightVolume +
				RNDBY16) >> 16);
#endif

		  if (bLevels)
		    {
		      if (Abs (sLSample) > Abs (sPendingLeftLevel))
			sPendingLeftLevel = sLSample;
		      if (Abs (sRSample) > Abs (sPendingRightLevel))
			sPendingRightLevel = sRSample;
		    }

		  if (nChannels > 1)
		    {
		      *pDst++ = sLSample;
		      *pDst++ = sRSample;
		    }
		  else
		    {
		      lSum = (LONG) sLSample + (LONG) sRSample;

		      // make sure we don't over saturate the value
		      if (lSum > 32767)
			lSum = 32767;
		      if (lSum < -32768)
			lSum = -32768;

		      *pDst++ = (USHORT) lSum;
		    }

		}
	    }
	  else			//dither enabled
	    {
	      register PSHORT pDst = (PSHORT) pBuffer;

	      while (ulSamplesToTransfer--)
		{
		  lLSample = READ_REGISTER_ULONG (&pulBuffer[ulPCIndex++]);
		  lRSample = READ_REGISTER_ULONG (&pulBuffer[ulPCIndex++]);
		  if (ulPCIndex >= WAVE_CIRCULAR_BUFFER_SIZE)
		    ulPCIndex = 0;

#ifndef MULTI_INPUT
		  if (bLVolume)
		    lLSample =
		      (LONG) (((LONGLONG) lLSample * lLeftVolume) >> 16);
		  if (bRVolume)
		    lRSample =
		      (LONG) (((LONGLONG) lRSample * lRightVolume) >> 16);
#endif

		  MAKE_DITHER;	//generate L/R dither - in low word of long

		  lLSample =
		    (LONG) (((LONGLONG) lLSample +
			     (LONGLONG) lLDither) >> 16);

		  // Saturation logic
		  if (lLSample > 32767)
		    lLSample = 32767;
		  if (lLSample < -32768)
		    lLSample = -32768;


		  lRSample =
		    (LONG) (((LONGLONG) lRSample +
			     (LONGLONG) lRDither) >> 16);

		  // Saturation logic
		  if (lRSample > 32767)
		    lRSample = 32767;
		  if (lRSample < -32768)
		    lRSample = -32768;

		  sLSample = (SHORT) lLSample;
		  sRSample = (SHORT) lRSample;

		  if (bLevels)
		    {
		      if (Abs (sLSample) > Abs (sPendingLeftLevel))
			sPendingLeftLevel = sLSample;
		      if (Abs (sRSample) > Abs (sPendingRightLevel))
			sPendingRightLevel = sRSample;
		    }

		  if (nChannels > 1)
		    {
		      *pDst++ = sLSample;
		      *pDst++ = sRSample;
		    }
		  else
		    {
		      lSum = (LONG) sLSample + (LONG) sRSample;

		      // make sure we don't over saturate the value
		      if (lSum > 32767)
			lSum = 32767;
		      if (lSum < -32768)
			lSum = -32768;

		      *pDst++ = (USHORT) lSum;
		    }
		}
	    }
	  break;
	case 24:
	  if (usDitherType == MIXVAL_DITHER_NONE)
	    {
	      register PBYTE pDst = (PBYTE) pBuffer;

	      while (ulSamplesToTransfer--)
		{
		  lLSample = READ_REGISTER_ULONG (&pulBuffer[ulPCIndex++]);
		  lRSample = READ_REGISTER_ULONG (&pulBuffer[ulPCIndex++]);
		  if (ulPCIndex >= WAVE_CIRCULAR_BUFFER_SIZE)
		    ulPCIndex = 0;

#ifndef MULTI_INPUT
		  if (bLVolume)
		    lLSample =
		      (LONG) (((LONGLONG) lLSample * lLeftVolume +
			       RNDBY24) >> 16);
		  if (bRVolume)
		    lRSample =
		      (LONG) (((LONGLONG) lRSample * lRightVolume +
			       RNDBY24) >> 16);
#endif

		  if (bLevels)
		    {
		      SHORT sLSample = (SHORT) (lLSample >> 16);	// levels are always 16 bit
		      SHORT sRSample = (SHORT) (lRSample >> 16);

		      if (Abs (sLSample) > Abs (sPendingLeftLevel))
			sPendingLeftLevel = sLSample;
		      if (Abs (sRSample) > Abs (sPendingRightLevel))
			sPendingRightLevel = sRSample;
		    }

		  if (nChannels > 1)
		    {
		      *pDst++ = (BYTE) (lLSample >> 8);
		      *pDst++ = (BYTE) (lLSample >> 16);
		      *pDst++ = (BYTE) (lLSample >> 24);

		      *pDst++ = (BYTE) (lRSample >> 8);
		      *pDst++ = (BYTE) (lRSample >> 16);
		      *pDst++ = (BYTE) (lRSample >> 24);
		    }
		  else
		    {
		      // make sure we are using only the 24-bit values
		      lSum = (lLSample >> 8) + (lRSample >> 8);

		      // make sure we don't over saturate the value
		      if (lSum > 8388607)
			lSum = 8388607;	// 2^23 - 1
		      if (lSum < -8388608)
			lSum = -8388608;	// -2^23

		      *pDst++ = (BYTE) (lSum);	// was 8
		      *pDst++ = (BYTE) (lSum >> 8);	// was 16
		      *pDst++ = (BYTE) (lSum >> 16);	// was 24
		    }
		}
	    }
	  else			//dither enabled, 24-bit
	    {
	      register PBYTE pDst = (PBYTE) pBuffer;

	      while (ulSamplesToTransfer--)
		{
		  lLSample = READ_REGISTER_ULONG (&pulBuffer[ulPCIndex++]);
		  lRSample = READ_REGISTER_ULONG (&pulBuffer[ulPCIndex++]);
		  if (ulPCIndex >= WAVE_CIRCULAR_BUFFER_SIZE)
		    ulPCIndex = 0;

		  MAKE_DITHER;	//generate L/R dither - in low word of long

#ifndef MULTI_INPUT
		  if (bLVolume)
		    lLSample =
		      (LONG) (((LONGLONG) lLSample * lLeftVolume +
			       (LONGLONG) (lLDither << 8)) >> 16);
		  if (bRVolume)
		    lRSample =
		      (LONG) (((LONGLONG) lRSample * lRightVolume +
			       (LONGLONG) (lRDither << 8)) >> 16);
#endif

		  // NOTE:At this point 24-bit samples are left justified in long words
		  // lLSample and lRSample

		  if (bLevels)
		    {
		      SHORT sLSample = (SHORT) (lLSample >> 16);	// levels are always 16 bit
		      SHORT sRSample = (SHORT) (lRSample >> 16);

		      if (Abs (sLSample) > Abs (sPendingLeftLevel))
			sPendingLeftLevel = sLSample;
		      if (Abs (sRSample) > Abs (sPendingRightLevel))
			sPendingRightLevel = sRSample;
		    }

		  if (nChannels > 1)
		    {
		      *pDst++ = (BYTE) ((lLSample) >> 8);
		      *pDst++ = (BYTE) (lLSample >> 16);
		      *pDst++ = (BYTE) (lLSample >> 24);

		      *pDst++ = (BYTE) ((lRSample) >> 8);
		      *pDst++ = (BYTE) (lRSample >> 16);
		      *pDst++ = (BYTE) (lRSample >> 24);
		    }
		  else
		    {
		      // make sure we are using only the 24-bit values
		      lSum = (lLSample >> 8) + (lRSample >> 8);

		      // make sure we don't over saturate the value
		      if (lSum > 8388607)
			lSum = 8388607;	// 2^23 - 1
		      if (lSum < -8388608)
			lSum = -8388608;	// -2^23

		      *pDst++ = (BYTE) (lSum);
		      *pDst++ = (BYTE) (lSum >> 8);
		      *pDst++ = (BYTE) (lSum >> 16);
		    }
		}
	    }
	  break;

	case 32:
	  if (usDitherType == MIXVAL_DITHER_NONE)
	    {
	      register PLONG pDst = (PLONG) pBuffer;

	      while (ulSamplesToTransfer--)
		{
		  lLSample = READ_REGISTER_ULONG (&pulBuffer[ulPCIndex++]);
		  lRSample = READ_REGISTER_ULONG (&pulBuffer[ulPCIndex++]);
		  if (ulPCIndex >= WAVE_CIRCULAR_BUFFER_SIZE)
		    ulPCIndex = 0;

#ifndef MULTI_INPUT
		  //Round to 24-bit level since noise floor is already set by 24-bit A/D
		  if (bLVolume)
		    lLSample =
		      (LONG) (((LONGLONG) lLSample * lLeftVolume +
			       RNDBY24) >> 16);
		  if (bRVolume)
		    lRSample =
		      (LONG) (((LONGLONG) lRSample * lRightVolume +
			       RNDBY24) >> 16);
#endif

		  if (bLevels)
		    {
		      SHORT sLSample = (SHORT) (lLSample >> 16);	// levels are always 16 bit
		      SHORT sRSample = (SHORT) (lRSample >> 16);

		      if (Abs (sLSample) > Abs (sPendingLeftLevel))
			sPendingLeftLevel = sLSample;
		      if (Abs (sRSample) > Abs (sPendingRightLevel))
			sPendingRightLevel = sRSample;
		    }

		  if (nChannels > 1)
		    {
		      *pDst++ = lLSample;
		      *pDst++ = lRSample;
		    }
		  else
		    {
		      llSum = (LONGLONG) lLSample + (LONGLONG) lRSample;
#ifndef DOS
		      // make sure we don't over saturate the value
#if !defined(MACINTOSH) && !defined(OSS)
		      if (llSum > 2147483647i 64)
			llSum = 2147483647i 64;	// 2^31 - 1
		      if (llSum < -2147483648i 64)
			llSum = -2147483648i 64;	// -2^31
#else
		      if (llSum > 2147483647)
			llSum = 2147483647;	// 2^31 - 1
		      if (llSum < -2147483648UL)
			llSum = -2147483648UL;	// -2^31
#endif
#endif

		      *pDst++ = (LONG) llSum;
		    }
		}
	    }
	  else			//dither enabled, 32-bit
	    {
	      register PLONG pDst = (PLONG) pBuffer;

	      while (ulSamplesToTransfer--)
		{
		  lLSample =
		    (LONG) READ_REGISTER_ULONG (&pulBuffer[ulPCIndex++]);
		  lRSample =
		    (LONG) READ_REGISTER_ULONG (&pulBuffer[ulPCIndex++]);
		  if (ulPCIndex >= WAVE_CIRCULAR_BUFFER_SIZE)
		    ulPCIndex = 0;

		  MAKE_DITHER;	//generate L/R dither - in low word of long

#ifndef MULTI_INPUT
		  //Dither to 24-bit level since noise floor is already set by 24-bit A/D
		  if (bLVolume)
		    lLSample =
		      (LONG) (((LONGLONG) lLSample * lLeftVolume +
			       (LONGLONG) (lLDither << 8)) >> 16);
		  if (bRVolume)
		    lRSample =
		      (LONG) (((LONGLONG) lRSample * lRightVolume +
			       (LONGLONG) (lRDither << 8)) >> 16);
#endif
		  if (bLevels)
		    {
		      SHORT sLSample = (SHORT) (lLSample >> 16);	// levels are always 16 bit
		      SHORT sRSample = (SHORT) (lRSample >> 16);

		      if (Abs (sLSample) > Abs (sPendingLeftLevel))
			sPendingLeftLevel = sLSample;
		      if (Abs (sRSample) > Abs (sPendingRightLevel))
			sPendingRightLevel = sRSample;
		    }

		  if (nChannels > 1)
		    {
		      *pDst++ = lLSample;
		      *pDst++ = lRSample;
		    }
		  else
		    {
		      llSum = (LONGLONG) lLSample + (LONGLONG) lRSample;
#ifndef DOS
		      // make sure we don't over saturate the value
#if !defined(MACINTOSH) && !defined(OSS)
		      if (llSum > 2147483647i 64)
			llSum = 2147483647i 64;	// 2^31 - 1
		      if (llSum < -2147483648i 64)
			llSum = -2147483648i 64;	// -2^31
#else
		      if (llSum > 2147483647UL)
			llSum = 2147483647UL;	// 2^31 - 1
		      if (llSum < -2147483647L)
			llSum = -2147483647L;	// -2^31
#endif
#endif

		      *pDst++ = (LONG) llSum;
		    }
		}
	    }
	  break;
	}
    }
  else				// MODE_PLAY or MODE_PRELOAD
    {
      switch (wBitsPerSample)
	{
	case 8:
	  if (usDitherType == MIXVAL_DITHER_NONE)
	    {
	      register PBYTE pSrc = pBuffer;

	      while (ulSamplesToTransfer--)
		{
		  sLSample = *pSrc++;
		  if (nChannels > 1)
		    sRSample = *pSrc++;
		  else
		    sRSample = sLSample;

		  sLSample = (sLSample - 128) << 8;	// convert from 8 bit to 16 bit
		  sRSample = (sRSample - 128) << 8;	// convert from 8 bit to 16 bit

		  // Round to 8 bits
		  if (bLVolume)
		    sLSample =
		      (SHORT) ((((LONG) sLSample * lLeftVolume +
				 RNDBY24) >> 16) & 0x0000FF00L);
		  if (bRVolume)
		    sRSample =
		      (SHORT) ((((LONG) sRSample * lRightVolume +
				 RNDBY24) >> 16) & 0x0000FF00L);

		  if (bLevels)
		    {
		      if (Abs (sLSample) > Abs (sPendingLeftLevel))
			sPendingLeftLevel = sLSample;
		      if (Abs (sRSample) > Abs (sPendingRightLevel))
			sPendingRightLevel = sRSample;
		    }

		  WRITE_REGISTER_ULONG (&pulBuffer[ulPCIndex++],
					MAKELONG (sLSample, sRSample));
		  if (ulPCIndex >= WAVE_CIRCULAR_BUFFER_SIZE)
		    ulPCIndex = 0;
		}
	    }
	  else			//dither enabled 8-bit
	    {
	      register PBYTE pSrc = pBuffer;

	      while (ulSamplesToTransfer--)
		{
		  sLSample = *pSrc++;
		  if (nChannels > 1)
		    sRSample = *pSrc++;
		  else
		    sRSample = sLSample;

		  sLSample = (sLSample - 128) << 8;	// convert from 8 bit to 16 bit
		  sRSample = (sRSample - 128) << 8;	// convert from 8 bit to 16 bit

		  MAKE_DITHER;	//generate L/R dither - in low word of long

		  //Dither to 8 bits - not 16
		  if (bLVolume)
		    sLSample =
		      (SHORT) ((((LONG) sLSample * lLeftVolume +
				 (lLDither << 8)) >> 16) & 0x0000FF00L);
		  if (bRVolume)
		    sRSample =
		      (SHORT) ((((LONG) sRSample * lRightVolume +
				 (lRDither << 8)) >> 16) & 0x0000FF00L);

		  if (bLevels)
		    {
		      if (Abs (sLSample) > Abs (sPendingLeftLevel))
			sPendingLeftLevel = sLSample;
		      if (Abs (sRSample) > Abs (sPendingRightLevel))
			sPendingRightLevel = sRSample;
		    }

		  WRITE_REGISTER_ULONG (&pulBuffer[ulPCIndex++],
					MAKELONG (sLSample, sRSample));
		  if (ulPCIndex >= WAVE_CIRCULAR_BUFFER_SIZE)
		    ulPCIndex = 0;
		}
	    }
	  break;
	case 16:
	  if (usDitherType == MIXVAL_DITHER_NONE)
	    {
	      register PSHORT pSrc = (PSHORT) pBuffer;

	      while (ulSamplesToTransfer--)
		{
		  sLSample = *pSrc++;
		  if (nChannels > 1)
		    sRSample = *pSrc++;
		  else
		    sRSample = sLSample;

		  if (bLVolume)
		    sLSample =
		      (SHORT) (((LONG) sLSample * lLeftVolume +
				RNDBY16) >> 16);
		  if (bRVolume)
		    sRSample =
		      (SHORT) (((LONG) sRSample * lRightVolume +
				RNDBY16) >> 16);

		  if (bLevels)
		    {
		      if (Abs (sLSample) > Abs (sPendingLeftLevel))
			sPendingLeftLevel = sLSample;
		      if (Abs (sRSample) > Abs (sPendingRightLevel))
			sPendingRightLevel = sRSample;
		    }

		  WRITE_REGISTER_ULONG (&pulBuffer[ulPCIndex++],
					MAKELONG (sLSample, sRSample));
		  if (ulPCIndex >= WAVE_CIRCULAR_BUFFER_SIZE)
		    ulPCIndex = 0;
		}
	    }
	  else			//dither enable 16-bit
	    {
	      register PSHORT pSrc = (PSHORT) pBuffer;

	      while (ulSamplesToTransfer--)
		{
		  sLSample = *pSrc++;
		  if (nChannels > 1)
		    sRSample = *pSrc++;
		  else
		    sRSample = sLSample;

		  MAKE_DITHER;	//generate L/R dither - in low word of long

		  if (bLVolume)
		    sLSample =
		      (SHORT) (((LONG) sLSample * lLeftVolume +
				lLDither) >> 16);
		  if (bRVolume)
		    sRSample =
		      (SHORT) (((LONG) sRSample * lRightVolume +
				lRDither) >> 16);

		  if (bLevels)
		    {
		      if (Abs (sLSample) > Abs (sPendingLeftLevel))
			sPendingLeftLevel = sLSample;
		      if (Abs (sRSample) > Abs (sPendingRightLevel))
			sPendingRightLevel = sRSample;
		    }

		  WRITE_REGISTER_ULONG (&pulBuffer[ulPCIndex++],
					MAKELONG (sLSample, sRSample));
		  if (ulPCIndex >= WAVE_CIRCULAR_BUFFER_SIZE)
		    ulPCIndex = 0;
		}
	    }
	  break;
	case 24:
	  if (usDitherType == MIXVAL_DITHER_NONE)
	    {
	      register PBYTE pSrc = (PBYTE) pBuffer;
	      BYTE uc1;
	      BYTE uc2;
	      BYTE uc3;

	      while (ulSamplesToTransfer--)
		{
		  uc1 = *pSrc++;
		  uc2 = *pSrc++;
		  uc3 = *pSrc++;
		  lLSample =
		    MAKEULONG (MAKEWORD (0, uc1), MAKEWORD (uc2, uc3));

		  if (nChannels > 1)
		    {
		      uc1 = *pSrc++;
		      uc2 = *pSrc++;
		      uc3 = *pSrc++;
		      lRSample =
			MAKEULONG (MAKEWORD (0, uc1), MAKEWORD (uc2, uc3));
		    }
		  else
		    lRSample = lLSample;

		  // Rounded volume calculations
		  // Tricky rounding here - a hidden shift right by 8 occurs since only the upper
		  // 24-bits of of the long sample are used by the D/A converter.
		  if (bLVolume)
		    lLSample =
		      (LONG) (((LONGLONG) lLSample * lLeftVolume +
			       RNDBY24) >> 16);
		  if (bRVolume)
		    lRSample =
		      (LONG) (((LONGLONG) lRSample * lRightVolume +
			       RNDBY24) >> 16);

		  if (bLevels)
		    {
		      SHORT sLSample = (SHORT) (lLSample >> 16);	// levels are always 16 bit
		      SHORT sRSample = (SHORT) (lRSample >> 16);

		      if (Abs (sLSample) > Abs (sPendingLeftLevel))
			sPendingLeftLevel = sLSample;
		      if (Abs (sRSample) > Abs (sPendingRightLevel))
			sPendingRightLevel = sRSample;
		    }

		  WRITE_REGISTER_ULONG (&pulBuffer[ulPCIndex++], lLSample);	// Left Channel
		  WRITE_REGISTER_ULONG (&pulBuffer[ulPCIndex++], lRSample);	// Right Channel
		  if (ulPCIndex >= WAVE_CIRCULAR_BUFFER_SIZE)
		    ulPCIndex = 0;
		}
	    }
	  else			//dither enabled 24-bit
	    {
	      register PBYTE pSrc = (PBYTE) pBuffer;
	      BYTE uc1;
	      BYTE uc2;
	      BYTE uc3;

	      while (ulSamplesToTransfer--)
		{
		  uc1 = *pSrc++;
		  uc2 = *pSrc++;
		  uc3 = *pSrc++;
		  lLSample =
		    MAKEULONG (MAKEWORD (0, uc1), MAKEWORD (uc2, uc3));

		  if (nChannels > 1)
		    {
		      uc1 = *pSrc++;
		      uc2 = *pSrc++;
		      uc3 = *pSrc++;
		      lRSample =
			MAKEULONG (MAKEWORD (0, uc1), MAKEWORD (uc2, uc3));
		    }
		  else
		    lRSample = lLSample;

		  MAKE_DITHER;	//generate L/R dither - in low word of long

		  if (bLVolume)
		    lLSample =
		      (LONG) (((((LONGLONG) lLSample * lLeftVolume) >> 8) +
			       (LONGLONG) lLDither) >> 8);
		  if (bRVolume)
		    lRSample =
		      (LONG) (((((LONGLONG) lRSample * lRightVolume) >> 8) +
			       (LONGLONG) lRDither) >> 8);

		  if (bLevels)
		    {
		      SHORT sLSample = (SHORT) (lLSample >> 16);	// levels are always 16 bit
		      SHORT sRSample = (SHORT) (lRSample >> 16);

		      if (Abs (sLSample) > Abs (sPendingLeftLevel))
			sPendingLeftLevel = sLSample;
		      if (Abs (sRSample) > Abs (sPendingRightLevel))
			sPendingRightLevel = sRSample;
		    }

		  WRITE_REGISTER_ULONG (&pulBuffer[ulPCIndex++], lLSample);	// Left Channel
		  WRITE_REGISTER_ULONG (&pulBuffer[ulPCIndex++], lRSample);	// Right Channel
		  if (ulPCIndex >= WAVE_CIRCULAR_BUFFER_SIZE)
		    ulPCIndex = 0;
		}
	    }
	  break;
	case 32:
	  if (usDitherType == MIXVAL_DITHER_NONE)
	    {
	      register PLONG pSrc = (PLONG) pBuffer;

	      while (ulSamplesToTransfer--)
		{
		  lLSample = *pSrc++;
		  if (nChannels > 1)
		    lRSample = *pSrc++;
		  else
		    lRSample = lLSample;

		  // Rounded volume calculations
		  // Tricky rounding here - a hidden shift right by 8 occurs since only the upper
		  // 24-bits of of the long sample are used by the D/A converter.
		  if (bLVolume)
		    lLSample =
		      (LONG) (((LONGLONG) lLSample * lLeftVolume +
			       RNDBY24) >> 16);
		  if (bRVolume)
		    lRSample =
		      (LONG) (((LONGLONG) lRSample * lRightVolume +
			       RNDBY24) >> 16);

		  if (bLevels)
		    {
		      SHORT sLSample = (SHORT) (lLSample >> 16);	// levels are always 16 bit
		      SHORT sRSample = (SHORT) (lRSample >> 16);

		      if (Abs (sLSample) > Abs (sPendingLeftLevel))
			sPendingLeftLevel = sLSample;
		      if (Abs (sRSample) > Abs (sPendingRightLevel))
			sPendingRightLevel = sRSample;
		    }

		  WRITE_REGISTER_ULONG (&pulBuffer[ulPCIndex++], lLSample);	// Left Channel
		  WRITE_REGISTER_ULONG (&pulBuffer[ulPCIndex++], lRSample);	// Right Channel
		  if (ulPCIndex >= WAVE_CIRCULAR_BUFFER_SIZE)
		    ulPCIndex = 0;
		}
	    }
	  else			//dither enabled 32-bit
	    {
	      register PLONG pSrc = (PLONG) pBuffer;

	      while (ulSamplesToTransfer--)
		{
		  lLSample = *pSrc++;
		  if (nChannels > 1)
		    lRSample = *pSrc++;
		  else
		    lRSample = lLSample;

		  MAKE_DITHER;	//generate L/R dither - in low word of long

		  //Dither to 24-bits, same as 24-bit packed format
		  if (bLVolume)
		    lLSample =
		      (LONG) (((((LONGLONG) lLSample * lLeftVolume) >> 8) +
			       (LONGLONG) lLDither) >> 8);
		  if (bRVolume)
		    lRSample =
		      (LONG) (((((LONGLONG) lRSample * lRightVolume) >> 8) +
			       (LONGLONG) lRDither) >> 8);

		  if (bLevels)
		    {
		      SHORT sLSample = (SHORT) (lLSample >> 16);	// levels are always 16 bit
		      SHORT sRSample = (SHORT) (lRSample >> 16);

		      if (Abs (sLSample) > Abs (sPendingLeftLevel))
			sPendingLeftLevel = sLSample;
		      if (Abs (sRSample) > Abs (sPendingRightLevel))
			sPendingRightLevel = sRSample;
		    }

		  WRITE_REGISTER_ULONG (&pulBuffer[ulPCIndex++], lLSample);	// Left Channel
		  WRITE_REGISTER_ULONG (&pulBuffer[ulPCIndex++], lRSample);	// Right Channel
		  if (ulPCIndex >= WAVE_CIRCULAR_BUFFER_SIZE)
		    ulPCIndex = 0;
		}
	    }
	  break;
	}
    }

  pD->ulPCIndex = ulPCIndex;
  pD->sLeftLevel = sPendingLeftLevel;
  pD->sRightLevel = sPendingRightLevel;
  pD->usDitherNoise[0] = usRandom[0];
  pD->usDitherNoise[1] = usRandom[1];

  return (HSTATUS_OK);
}

/////////////////////////////////////////////////////////////////////////////
USHORT
HalDeviceASIOTransferAudio (PDEVICE pD, PVOID pvLeft, PVOID pvRight,
			    ULONG ulPCIndex, ULONG ulSamplesToTransfer)
//  Reads/Writes audio data to/from the device depending on the device mode.
/////////////////////////////////////////////////////////////////////////////
{
  PULONG pulBuffer = pD->pBuffer;	// the devices buffer
  //ULONG     ulPCIndex = pD->ulPCIndex;      // the devices PC index
  SHORT sPendingLeftLevel = 0;	// the level that is being calculated for this packet
  SHORT sPendingRightLevel = 0;	// the level that is being calculated for this packet
  LONG lLeftVolume;
  LONG lRightVolume;
  BOOLEAN bLevels = ((PADAPTER) pD->pA)->bLevels;
  BOOLEAN bLVolume, bRVolume;
  WORD wBitsPerSample = pD->WaveFormat.wBitsPerSample;
  SHORT sLSample = 0, sRSample = 0;
  LONG lLSample = 0, lRSample = 0;
  ULONG ulPacked;

#ifdef DEBUG
//  if( pD->ulPCIndex != ulPCIndex )
//      DPF(("ulPCIndex Mismatch!\n"));
#endif

  switch (pD->usMode)
    {
    case MODE_PRELOAD:
    case MODE_PLAY:
    case MODE_RECORD:
      break;
    default:
      DPF (("HalASIOTransferAudio: Invalid Mode\n"));
      return (HSTATUS_INVALID_MODE);
    }

  if (pD->bMute)
    {
      lLeftVolume = lRightVolume = 0;
    }
  else
    {
      lLeftVolume = pD->lLeftVolume;
      lRightVolume = pD->lRightVolume;
    }

  // if both volume is full scale, don't compute
  if (lLeftVolume == 0xFFFF)
    bLVolume = FALSE;
  else
    bLVolume = TRUE;

  if (lRightVolume == 0xFFFF)
    bRVolume = FALSE;
  else
    bRVolume = TRUE;

  if (pD->usMode == MODE_RECORD)
    {
      switch (wBitsPerSample)
	{
	case 16:
	  {
	    register PSHORT pLDst = (PSHORT) pvLeft;
	    register PSHORT pRDst = (PSHORT) pvRight;

	    while (ulSamplesToTransfer--)
	      {
		ulPacked = READ_REGISTER_ULONG (&pulBuffer[ulPCIndex++]);
		if (ulPCIndex >= WAVE_CIRCULAR_BUFFER_SIZE)
		  ulPCIndex = 0;

		sLSample = LOWORD (ulPacked);
		sRSample = HIWORD (ulPacked);

		if (bLVolume)
		  sLSample =
		    (SHORT) (((LONG) sLSample * lLeftVolume + RNDBY16) >> 16);
		if (bRVolume)
		  sRSample =
		    (SHORT) (((LONG) sRSample * lRightVolume +
			      RNDBY16) >> 16);

		if (bLevels)
		  {
		    if (Abs (sLSample) > Abs (sPendingLeftLevel))
		      sPendingLeftLevel = sLSample;
		    if (Abs (sRSample) > Abs (sPendingRightLevel))
		      sPendingRightLevel = sRSample;
		  }

		// always in stereo
		if (pLDst)
		  *pLDst++ = sLSample;
		if (pRDst)
		  *pRDst++ = sRSample;
	      }
	  }
	  break;
	case 32:
	  {
	    register PLONG pLDst = (PLONG) pvLeft;
	    register PLONG pRDst = (PLONG) pvRight;

	    while (ulSamplesToTransfer--)
	      {
		lLSample = READ_REGISTER_ULONG (&pulBuffer[ulPCIndex++]);
		lRSample = READ_REGISTER_ULONG (&pulBuffer[ulPCIndex++]);
		if (ulPCIndex >= WAVE_CIRCULAR_BUFFER_SIZE)
		  ulPCIndex = 0;

		if (bLVolume)
		  lLSample =
		    (LONG) (((LONGLONG) lLSample * lLeftVolume +
			     RNDBY16) >> 16);
		if (bRVolume)
		  lRSample =
		    (LONG) (((LONGLONG) lRSample * lRightVolume +
			     RNDBY16) >> 16);

		if (bLevels)
		  {
		    SHORT sLSample = (SHORT) (lLSample >> 16);	// levels are always 16 bit
		    SHORT sRSample = (SHORT) (lRSample >> 16);

		    if (Abs (sLSample) > Abs (sPendingLeftLevel))
		      sPendingLeftLevel = sLSample;
		    if (Abs (sRSample) > Abs (sPendingRightLevel))
		      sPendingRightLevel = sRSample;
		  }

		if (pLDst)
		  *pLDst++ = lLSample;
		if (pRDst)
		  *pRDst++ = lRSample;
	      }			// while
	  }
	  break;
	}			// switch( wBitsPerSample )
    }
  else				// MODE_PLAY or MODE_PRELOAD
    {
      switch (wBitsPerSample)
	{
	case 16:
	  {
	    register PSHORT pLSrc = (PSHORT) pvLeft;
	    register PSHORT pRSrc = (PSHORT) pvRight;

	    while (ulSamplesToTransfer--)
	      {
		// always in stereo
		if (pLSrc)
		  sLSample = *pLSrc++;
		if (pRSrc)
		  sRSample = *pRSrc++;

		if (bLVolume)
		  sLSample =
		    (SHORT) (((LONG) sLSample * lLeftVolume + RNDBY16) >> 16);
		if (bRVolume)
		  sRSample =
		    (SHORT) (((LONG) sRSample * lRightVolume +
			      RNDBY16) >> 16);

		if (bLevels)
		  {
		    if (Abs (sLSample) > Abs (sPendingLeftLevel))
		      sPendingLeftLevel = sLSample;
		    if (Abs (sRSample) > Abs (sPendingRightLevel))
		      sPendingRightLevel = sRSample;
		  }

		WRITE_REGISTER_ULONG (&pulBuffer[ulPCIndex++],
				      MAKELONG (sLSample, sRSample));
		if (ulPCIndex >= WAVE_CIRCULAR_BUFFER_SIZE)
		  ulPCIndex = 0;
	      }
	  }
	  break;
	case 32:
	  {
	    register PLONG pLSrc = (PLONG) pvLeft;
	    register PLONG pRSrc = (PLONG) pvRight;

	    while (ulSamplesToTransfer--)
	      {
		if (pLSrc)
		  lLSample = *pLSrc++;
		if (pRSrc)
		  lRSample = *pRSrc++;

		if (bLVolume)
		  lLSample =
		    (LONG) (((LONGLONG) lLSample * lLeftVolume +
			     RNDBY24) >> 16);
		if (bRVolume)
		  lRSample =
		    (LONG) (((LONGLONG) lRSample * lRightVolume +
			     RNDBY24) >> 16);

		if (bLevels)
		  {
		    SHORT sLSample = (SHORT) (lLSample >> 16);	// levels are always 16 bit
		    SHORT sRSample = (SHORT) (lRSample >> 16);

		    if (Abs (sLSample) > Abs (sPendingLeftLevel))
		      sPendingLeftLevel = sLSample;
		    if (Abs (sRSample) > Abs (sPendingRightLevel))
		      sPendingRightLevel = sRSample;
		  }

		WRITE_REGISTER_ULONG (&pulBuffer[ulPCIndex++], lLSample);	// Left Channel
		WRITE_REGISTER_ULONG (&pulBuffer[ulPCIndex++], lRSample);	// Right Channel
		if (ulPCIndex >= WAVE_CIRCULAR_BUFFER_SIZE)
		  ulPCIndex = 0;
	      }			// while
	  }
	  break;
	}			// switch( wBitsPerSample )
    }

  pD->ulPCIndex = ulPCIndex;
  pD->sLeftLevel = sPendingLeftLevel;
  pD->sRightLevel = sPendingRightLevel;

  return (HSTATUS_OK);
}

/////////////////////////////////////////////////////////////////////////////
USHORT
HalDeviceTransferComplete (PDEVICE pD, LONG lBytesProcessed)
// Signals that the PC has completed processing the interrupt for this device 
// and is ready to accept another interrupt from the adapter.
//
// This function assumes that HalDeviceTransferAudio was just run, and the buffer
// is either completely full (PLAYBACK) or completely empty (RECORD).
/////////////////////////////////////////////////////////////////////////////
{
  LONG lPCIndex = pD->ulPCIndex;
  LONG lHWIndex = pD->lHWIndex;	//READ_REGISTER_ULONG( pD->pulHWIndex ) >> 1;    // adjust for HWIndex being 16 bit aligned
  LONG lSize;
  ULONG ulSampleCount;

  /////////////////////////////////////////////////////////////////////////
  // Reprogram the Circular Buffer Limit for this device
  /////////////////////////////////////////////////////////////////////////

  if (pD->usMode == MODE_RECORD)
    {
      if (pD->ulInterruptSamples)
	{
	  lHWIndex = pD->lHWIndex + pD->ulInterruptSamples;	// don't use the actual hardware index
	}
      else
	{
	  lSize = lPCIndex - lHWIndex;
	  // lSize == 0 means no data is waiting, so entire buffer is available
	  if (lSize <= 0)
	    lSize += WAVE_CIRCULAR_BUFFER_SIZE;

	  //lHWIndex += (lSize / 4);          // 1/4 buffer
	  lHWIndex += (lSize / 2);	// 1/2 buffer
	  //lHWIndex += ((lSize / 4) * 3);        // 3/4 buffer
	}
    }
  else				// MODE_PLAY or MODE_PRELOAD
    {
      // if we transferred any data
      if (lBytesProcessed)
	{
	  if (pD->ulInterruptSamples)
	    {
	      lHWIndex = pD->lHWIndex + pD->ulInterruptSamples;	// don't use the actual hardware index
	    }
	  else
	    {
	      lSize = lPCIndex - lHWIndex;
	      // lSize == 0 means no data is on card
	      if (lSize < 0)
		lSize += WAVE_CIRCULAR_BUFFER_SIZE;

	      //lHWIndex += (lSize / 4);      // 1/4 buffer
	      lHWIndex += (lSize / 2);	// 1/2 buffer
	      //lHWIndex += ((lSize / 4) * 3);    // 3/4 buffer
	    }
	}
      // no data was transferred (end of buffer)
      else
	{
	  DB ('D', COLOR_BOLD);
	  lHWIndex = lPCIndex;
	}
    }

  lHWIndex &= REG_CBLIM_MASK;	// limit to valid range

  if (pD->b32BitMode)		// if in 32 bit mode, make sure HWIndex is divisible by 2
    CLR (lHWIndex, 1);		// clear the lsb

  RegWriteMask (&pD->RegCBLIM, REG_CBLIM_MASK, lHWIndex);
  pD->lHWIndex = lHWIndex;	// save this HWIndex for use on the next interrupt

  // call the get sample count code to make sure the sample count doesn't roll over
  HalDeviceGetSampleCount (pD, &ulSampleCount);

  return (HSTATUS_OK);
}

/////////////////////////////////////////////////////////////////////////////
USHORT
HalDeviceSetInterruptSamples (PDEVICE pD, ULONG ulSampleCount)
/////////////////////////////////////////////////////////////////////////////
{
  pD->ulInterruptSamples = ulSampleCount;
  return (HSTATUS_OK);
}

/////////////////////////////////////////////////////////////////////////////
USHORT
HalDeviceGetSampleCount (PDEVICE pD, PULONG pulSampleCount)
//  Gets the number of samples the HW has tranferred.  
//  This count is reset when the device mode is set to MODE_IDLE.
/////////////////////////////////////////////////////////////////////////////
{
  ULONG ulHWIndex;
  LONG lDiff;

  // protect this function from re-entrancy problems - without a spin lock or other such thing
  if (pD->ulEntryCount)
    {
      *pulSampleCount = pD->ulSampleCount;	// this should always be the ISR interrupting
      return (HSTATUS_OK);
    }

  pD->ulEntryCount++;

  ulHWIndex = READ_REGISTER_ULONG (pD->pulHWIndex) >> 1;

  if (pD->b32BitMode)		// if in 32 bit mode, make sure HWIndex is divisible by 2
    CLR (ulHWIndex, 1);		// clear the lsb

  lDiff = (LONG) ulHWIndex - (LONG) pD->ulLastHWIndex;
  if (lDiff < 0)
    lDiff += WAVE_CIRCULAR_BUFFER_SIZE;

  // save the current HW index as the last HW index for the next time around.
  pD->ulLastHWIndex = ulHWIndex;

  // lDiff now has the number of DWORDs that have be processed by the hardware
  // since the last GetSampleCount call.  This needs to be changed to the number
  // of samples based on the format the device is currently in.

  // if we are in 32 bit mode, then lDiff must be reduced by a factor of 2 to get samples
  if (pD->b32BitMode)
    lDiff >>= 1;

  // increase the overall sample count for this device
  pD->ulSampleCount += (ULONG) lDiff;

  // and let the driver know what the count is right now
  *pulSampleCount = pD->ulSampleCount;

  pD->ulEntryCount--;
  return (HSTATUS_OK);
}

/////////////////////////////////////////////////////////////////////////////
USHORT
HalDeviceSetSampleCount (PDEVICE pD, ULONG ulSampleCount)
//  The device must be in mode MODE_IDLE.
/////////////////////////////////////////////////////////////////////////////
{
  if (pD->usMode != MODE_IDLE)
    {
      DPF (("SetSampleCount when Device != IDLE\n"));
      return (HSTATUS_INVALID_MODE);
    }

  pD->ulSampleCount = ulSampleCount;
  pD->ulLastHWIndex = 0;

  return (HSTATUS_OK);
}

/////////////////////////////////////////////////////////////////////////////
USHORT
HalSetSampleRate (PADAPTER pA, ULONG ulSampleRate)
// allow the driver to change the sample rate directly
/////////////////////////////////////////////////////////////////////////////
{
  USHORT usResult = HSTATUS_OK;

  if (pA->ulClockSource != MIXVAL_CLKSRC_DIGITAL)
    {
      usResult =
	SetSampleRate (pA, ulSampleRate, pA->ulClockSource,
		       pA->ulClockReference, TRUE);
    }

  return (usResult);
}

/////////////////////////////////////////////////////////////////////////////
USHORT
HalGetSampleRate (PADAPTER pA, PULONG pulBaseSampleRate,
		  PULONG pulCurrentSampleRate, PULONG pulMinSampleRate,
		  PULONG pulMaxSampleRate)
/////////////////////////////////////////////////////////////////////////////
{
  *pulBaseSampleRate = pA->ulBaseSampleRate;
  *pulCurrentSampleRate = pA->ulCurrentSampleRate;
  *pulMinSampleRate = MIN_SAMPLE_RATE;

  if ((pA->Device[WAVE_RECORD0_DEVICE].usMode > MODE_IDLE)
      || (pA->Device[WAVE_PLAY0_DEVICE].usMode > MODE_IDLE))
    {
      *pulMaxSampleRate = MAX_ANALOG_RATE;
    }
  else
    {
      *pulMaxSampleRate = MAX_SAMPLE_RATE;
    }

  return (HSTATUS_OK);
}

/////////////////////////////////////////////////////////////////////////////
USHORT
SetSampleRate (PADAPTER pA, ULONG ulSampleRate, ULONG ulClockSource,
	       ULONG ulClockReference, BOOL bPitchChange)
/////////////////////////////////////////////////////////////////////////////
{
  USHORT usDevice;
  ULONG ulM, ulBypassM, ulN, ulP, ulClkSrc;

  typedef struct
  {
    ULONG ulSRate;
    USHORT M;
    USHORT N;
    USHORT P;
  }
  SRREGS;

#define NUM_CLOCK_RATES		11

  // Internal 32MHz Clock Standard Frequency PLL Values
  static const SRREGS tblIntClk[] = {
    //Fsr   M      N  PX2
    {11025, 625, 441, SR_P8},	// exact
    {22050, 625, 441, SR_P4},	// exact
    {32000, 125, 64, SR_P2},	// exact
    {44056, 715, 504, SR_P2},	//  1.27 ppm error
    {44100, 625, 441, SR_P2},	// exact
    {44144, 143, 101, SR_P2},	// 14.6  ppm error
    {48000, 125, 96, SR_P2},	// exact
    {88112, 715, 1008, SR_P2},	//  1.27 ppm error
    {88200, 625, 882, SR_P2},	// exact
    {88288, 143, 202, SR_P2},	// 14.6  ppm error
    {96000, 125, 192, SR_P2}	// exact
  };

  static const SRREGS tbl13p5MHzClk[] = {
    //Fsr   M     N    PX2
    {11025, 284, 475, SR_P8},	// 1.123 ppm error
    {22050, 284, 475, SR_P4},	// 1.123 ppm error
    {32000, 543, 659, SR_P2},	// 1.35  ppm error
    {44056, 556, 929, SR_P2},	// 0.768 ppm error
    {44100, 284, 475, SR_P2},	// 1.123 ppm error
    {44144, 221, 370, SR_P2},	// 3.63  ppm error
    {48000, 362, 659, SR_P2},	// 1.349 ppm error
    {88112, 278, 929, SR_P2},	// 0.768 ppm error
    {88200, 142, 475, SR_P2},	// 1.123 ppm error
    {88288, 221, 740, SR_P2},	// 3.63  ppm error
    {96000, 181, 659, SR_P2}	// 1.349 ppm error
  };

  static const SRREGS tbl27MHzClk[] = {
    //Fsr     M     N     PX2
    {11025, 568, 475, SR_P8},	// 1.123 ppm error
    {22050, 568, 475, SR_P4},	// 1.123 ppm error
    {32000, 763, 463, SR_P2},	// 0.640 ppm error
    {44056, 1112, 929, SR_P2},	// 0.77  ppm error
    {44100, 568, 475, SR_P2},	// 1.12  ppm error
    {44144, 1062, 889, SR_P2},	// 1.46  ppm error
    {48000, 1125, 1024, SR_P2},	// exact
    {88112, 556, 929, SR_P2},	// 0.77 ppm error
    {88200, 284, 475, SR_P2},	// 1.12 ppm error
    {88288, 531, 889, SR_P2},	// 1.46 ppm error
    {96000, 763, 1389, SR_P2}	// 0.640 ppm error
  };

  //tPLLCTL       PLLCTL;
  USHORT i;
  USHORT nNumActiveDevices = 0;

  // if the requested sample rate is less than the minimum rate, set it at the minimum
  if (ulSampleRate < MIN_SAMPLE_RATE)
    {
      ulSampleRate = MIN_SAMPLE_RATE;
    }

  // is the analog side active?
  if ((pA->Device[WAVE_RECORD0_DEVICE].usMode > MODE_IDLE)
      || (pA->Device[WAVE_PLAY0_DEVICE].usMode > MODE_IDLE))
    {
      // limit the max sample rate
      if (ulSampleRate > MAX_ANALOG_RATE)
	{
	  DPF (("Limiting ulSampleRate Because Analog Is Active\n"));
	  ulSampleRate = MAX_ANALOG_RATE;
	}
    }

  // if the requested sample rate is greater than the maximum rate, set it at the maximum
  if (ulSampleRate > MAX_SAMPLE_RATE)
    {
      ulSampleRate = MAX_SAMPLE_RATE;
    }

  // If the monitor source is the digital input, then we must set the 
  // clock source to digital as well.
  if (pA->ulMonitorSource == MIXVAL_MONSRC_DIGITALIN)
    {
      ulClockSource = MIXVAL_CLKSRC_DIGITAL;
      ulClockReference = MIXVAL_CLKREF_AUTO;
    }

  // if there is no change from current setting, just return.
  if ((pA->ulCurrentSampleRate == ulSampleRate)
      && (pA->ulClockSource == ulClockSource)
      && (pA->ulClockReference == ulClockReference))
    {
      return (HSTATUS_OK);
    }

  // count the number of active devices
  for (usDevice = 0; usDevice < NUM_WAVE_DEVICES; usDevice++)
    {
      if (pA->Device[usDevice].usMode > MODE_IDLE)
	{
	  nNumActiveDevices++;
	}
    }

  // if any devices are active, and this is not a pitch change request, disallow
  // DAH Jan 25 2000 allow sample rate changes in ASIO mode
  if ((nNumActiveDevices > 0) && !bPitchChange && !pA->bASIO)
    {
      DPF (("SetSampleRate: Devices are Active!\n"));
      return (HSTATUS_INVALID_MODE);
    }

  // if more than one device is active and the requested sample rate isn't the same as the current one, disallow
  // this allows pitch changes ONLY when one device is active
  // DAH Jan 25 2000 allow sample rate changes in ASIO mode if more than one device is active
  if ((nNumActiveDevices > 1) && (pA->ulCurrentSampleRate != ulSampleRate)
      && !pA->bASIO)
    {
      DPF (("SetSampleRate: More than one device active!\n"));
      return (HSTATUS_INVALID_MODE);
    }

  // if we are requesting the clock source to switch to Digital, make sure the Digital Input is locked
  if (ulClockSource == MIXVAL_CLKSRC_DIGITAL)
    {
      if ((GetDigitalInStatus (pA) & MIXVAL_DIERR_MASK) ==
	  MIXVAL_DIERR_NOLOCK)
	{
	  DPF (("SetSampleRate: Digital In not locked!\n"));
	  return (HSTATUS_INVALID_MODE);
	}
    }

#ifdef DEBUG
#ifdef WIN95VXD
  {
    char szBuffer[40];
    _Sprintf (szBuffer, "ulSampleRate %lu\n", ulSampleRate);
    DPF ((szBuffer));
  }
#else
  DPF (("ulSampleRate %lu\n", ulSampleRate));
#endif
#endif

  // start off with everything turned off
  ulM = ulBypassM = ulN = ulP = ulClkSrc = 0;	//PLLCTL.ulPLLCTL = 0;

  switch (ulClockSource)
    {
    case MIXVAL_CLKSRC_INTERNAL:
      //DPF(("MIXVAL_CLKSRC_INTERNAL\n"));
      ulClockReference = MIXVAL_CLKREF_AUTO;
      ulClkSrc = CLKSRC_INTERNAL;

      // search for the sample rate in our table
      for (i = 0; i < NUM_CLOCK_RATES; i++)
	{
	  if (ulSampleRate == tblIntClk[i].ulSRate)
	    {
	      ulM = tblIntClk[i].M;
	      ulN = tblIntClk[i].N;
	      ulP = tblIntClk[i].P;
	      break;		// done
	    }
	}

      if (i == NUM_CLOCK_RATES)
	{
	  if (ulSampleRate > 30000)
	    {
	      ulM = 625;
	      ulN = ulSampleRate / 100;	// Fvco reaches 49MHz at 96kHz
	      ulP = SR_P2;
	      ulSampleRate = ulN * 100;
	    }
	  else if (ulSampleRate > 15000)
	    {
	      ulM = 625;
	      ulN = ulSampleRate / 50;
	      ulP = SR_P4;
	      ulSampleRate = ulN * 50;
	    }
	  else			// Samples Rates 0 - 15kHz
	    {
	      ulM = 625;
	      ulN = ulSampleRate / 25;
	      ulP = SR_P8;
	      ulSampleRate = ulN * 25;
	    }
	}
      break;
    case MIXVAL_CLKSRC_EXTERNAL:
    case MIXVAL_CLKSRC_HEADER:
      if (ulClockSource == MIXVAL_CLKSRC_EXTERNAL)
	{
	  //DPF(("MIXVAL_CLKSRC_EXTERNAL\n"));
	  ulClkSrc = CLKSRC_EXTERNAL;
	}
      else
	{
	  //DPF(("MIXVAL_CLKSRC_HEADER\n"));
	  ulClkSrc = CLKSRC_HEADER;
	}

      // make sure we allow the change from INTERNAL or DIGITAL
      if (ulClockReference == MIXVAL_CLKREF_AUTO)
	{
	  ulClockReference = MIXVAL_CLKREF_27MHZ;
	}

      switch (ulClockReference)
	{
	case MIXVAL_CLKREF_WORD:
	  //DPF(("MIXVAL_CLKREF_WORD\n"));
	  if (pA->ulClockReference != ulClockReference)
	    {
	      if (ulSampleRate < 24000)
		ulSampleRate = 44100;
	    }
	  if (ulSampleRate < 24000)
	    {
	      //DPF(("Invalid Sample Rate for Word Clock!\n"));
	      DPF (("SetSampleRate: Word Clock Below 24000!\n"));
	      return (HSTATUS_INVALID_SAMPLERATE);
	    }
	  ulM = 1;		// Ignored
	  ulBypassM = 1;
	  ulN = 512;
	  ulP = SR_P2;
	  // override above values because we are in word mode
	  if (ulClockSource == MIXVAL_CLKSRC_EXTERNAL)
	    ulClkSrc = CLKSRC_EXTERNAL_WORD;
	  else
	    ulClkSrc = CLKSRC_HEADER_WORD;
	  break;
	case MIXVAL_CLKREF_WORD256:
	  //DPF(("MIXVAL_CLKREF_WORD256\n"));
	  if (ulSampleRate > 30000)
	    {
	      ulM = 26;
	      ulN = 52;
	      ulP = SR_P2;
	    }
	  else if (ulSampleRate > 15000)
	    {
	      ulM = 8;
	      ulN = 32;
	      ulP = SR_P4;
	    }
	  else
	    {
	      ulM = 4;
	      ulN = 32;
	      ulP = SR_P8;
	    }
	  break;
	case MIXVAL_CLKREF_13p5MHZ:
	  //DPF(("MIXVAL_CLKREF_13p5MHZ\n"));
	  // search for the sample rate in our table
	  for (i = 0; i < NUM_CLOCK_RATES; i++)
	    {
	      if (ulSampleRate == tbl13p5MHzClk[i].ulSRate)
		{
		  ulM = tbl13p5MHzClk[i].M;
		  ulN = tbl13p5MHzClk[i].N;
		  ulP = tbl13p5MHzClk[i].P;
		  break;	// done
		}
	    }
	  if (i == NUM_CLOCK_RATES)
	    {
	      if (ulSampleRate > 30000)
		{
		  ulM = 264;
		  ulN = ulSampleRate / 100;	// Fvco reaches 49MHz at 96kHz
		  ulP = SR_P2;
		  ulSampleRate = ulN * 100;
		}
	      else if (ulSampleRate > 15000)
		{
		  ulM = 264;
		  ulN = ulSampleRate / 50;
		  ulP = SR_P4;
		  ulSampleRate = ulN * 50;
		}
	      else		// Samples Rates 0 - 15kHz
		{
		  ulM = 264;
		  ulN = ulSampleRate / 25;
		  ulP = SR_P8;
		  ulSampleRate = ulN * 25;
		}
	    }
	  break;
	case MIXVAL_CLKREF_27MHZ:
	  //DPF(("MIXVAL_CLKREF_27MHZ\n"));
	  // search for the sample rate in our table
	  for (i = 0; i < NUM_CLOCK_RATES; i++)
	    {
	      if (ulSampleRate == tbl27MHzClk[i].ulSRate)
		{
		  ulM = tbl27MHzClk[i].M;
		  ulN = tbl27MHzClk[i].N;
		  ulP = tbl27MHzClk[i].P;
		  break;	// done
		}
	    }

	  if (i == NUM_CLOCK_RATES)
	    {
	      if (ulSampleRate > 30000)
		{
		  ulM = 527;
		  ulN = ulSampleRate / 100;	// Fvco reaches 49MHz at 96kHz
		  ulP = SR_P2;
		  ulSampleRate = ulN * 100;
		}
	      else if (ulSampleRate > 15000)
		{
		  ulM = 527;
		  ulN = ulSampleRate / 50;
		  ulP = SR_P4;
		  ulSampleRate = ulN * 50;
		}
	      else		// Samples Rates 0 - 15kHz
		{
		  ulM = 527;
		  ulN = ulSampleRate / 25;
		  ulP = SR_P8;
		  ulSampleRate = ulN * 25;
		}
	    }
	  break;
	default:
	  DPF (("Invalid ulClockReference [%08lx]\n", ulClockReference));
	  return (HSTATUS_INVALID_MIXER_VALUE);
	}
      break;
    case MIXVAL_CLKSRC_DIGITAL:
      //DPF(("MIXVAL_CLKSRC_DIGITAL\n"));
      ulClockReference = MIXVAL_CLKREF_AUTO;

      ulM = 1;			// Ignored
      ulBypassM = 1;
      ulN = 512;
      ulP = SR_P2;
      ulClkSrc = CLKSRC_DIGITAL;
      break;
    default:
      DPF (("Invalid ulClockSource [%08lx]\n", ulClockSource));
      return (HSTATUS_INVALID_MIXER_VALUE);
    }

  // if we are in ASIO mode, we always need to update the base rate
  if (pA->bASIO)
    {
      pA->ulBaseSampleRate = ulSampleRate;
    }
  else
    {
      if (!bPitchChange)
	pA->ulBaseSampleRate = ulSampleRate;
    }

  // if the sample rate is above the upper limit for the D>A & A>D converters, mute them
  // we check at the top of this function to see if the analog side is active
  if (ulSampleRate > MAX_ANALOG_RATE)
    {
      RegBitSet (&pA->RegDACTL, REG_DACTL_ACLKENn, 1);
      RegBitSet (&pA->RegDACTL, REG_DACTL_DAMUTEn, 0);
    }
  else
    {
      RegBitSet (&pA->RegDACTL, REG_DACTL_ACLKENn, 0);
      RegBitSet (&pA->RegDACTL, REG_DACTL_DAMUTEn, 1);
    }

  // turn off the analog monitor before we change the sample rate
  RegBitSet (&pA->RegDACTL, REG_DACTL_DAT1MIXEN, 0);

  // change the sample rate in hardware
  RegWrite (&pA->RegPLLCTL, MAKE_PLLCTL (ulM, ulBypassM, ulN, ulP, ulClkSrc));

  // Make sure the Digital format gets updated
  SetDigitalFormat (pA, pA->ulDigitalFormat);

  //Reset 8404A after clock source or reference change to maintain
  //stable LRCK edge to DO preamble timing. Minimum reset pulse
  //width requirement is 150ns. (measured > 500 ns for this code)
  if ((pA->ulClockSource != ulClockSource)
      || (pA->ulClockReference != ulClockReference))
    {
      RegBitSet (&pA->RegAESCTL, REG_AESCTL_DORSTn, 0);
      RegBitSet (&pA->RegAESCTL, REG_AESCTL_DORSTn, 1);
    }

  // save the new values
  pA->ulClockSource = ulClockSource;
  pA->ulClockReference = ulClockReference;
  pA->ulCurrentSampleRate = ulSampleRate;

  // if the analog monitor is suppose to be on, turn it back on
  if (pA->bAnalogMonitor)
    {
      DelayuS (pA, 5000);	// 4 ms seems to keep the click at bay 80% of the time
      // so we wait 5 ms for the A>D converter to stabilize

      // put the analog monitor back where it should be
      RegBitSet (&pA->RegDACTL, REG_DACTL_DAT1MIXEN, 1);
    }

  return (HSTATUS_OK);
}

/////////////////////////////////////////////////////////////////////////////
USHORT
SetDigitalFormat (PADAPTER pA, ULONG ulDigitalFormat)
/////////////////////////////////////////////////////////////////////////////
{
  ULONG ulAESCTL = pA->RegAESCTL.ulValue;

  switch (ulDigitalFormat)
    {
    case MIXVAL_DF_AESEBU:
      // Set the relay to AESEBU and set the format to PRO
      CLR (ulAESCTL, (REG_AESCTL_DOFMT | REG_AESCTL_DIOTYPE));

      // turn off emphasis
      CLR (ulAESCTL, (REG_AESCTL_DOCS_PRO_EM0 | REG_AESCTL_DOCS_PRO_EM1));

      // The Non-Audio bit is not available on the 8404 for Consumer Mode.
      if (pA->ulDigitalOutStatus & MIXVAL_DOS_NONAUDIO_MODE)
	CLR (ulAESCTL, REG_AESCTL_DOCS_PRO_C1n);
      else
	SET (ulAESCTL, REG_AESCTL_DOCS_PRO_C1n);

      CLR (ulAESCTL, REG_AESCTL_DOCS_PRO_C9n);	// Stereo channel mode
      CLR (ulAESCTL, REG_AESCTL_DOCS_PRO_TRNPT);	// must be low

      switch (pA->ulBaseSampleRate)
	{
	case 48000:
	  SET (ulAESCTL, REG_AESCTL_DOCS_PRO_C6n);
	  CLR (ulAESCTL, REG_AESCTL_DOCS_PRO_C7n);
	  break;
	case 44100:
	  CLR (ulAESCTL, REG_AESCTL_DOCS_PRO_C6n);
	  SET (ulAESCTL, REG_AESCTL_DOCS_PRO_C7n);
	  break;
	case 32000:
	  CLR (ulAESCTL, REG_AESCTL_DOCS_PRO_C6n);
	  CLR (ulAESCTL, REG_AESCTL_DOCS_PRO_C7n);
	  break;
	default:		//sampling freq not indicated
	  SET (ulAESCTL, REG_AESCTL_DOCS_PRO_C6n);
	  SET (ulAESCTL, REG_AESCTL_DOCS_PRO_C7n);
	  break;
	}

      RegWrite (&pA->RegAESCTL, ulAESCTL);
      break;
    case MIXVAL_DF_SPDIF:
      // Set the relay to SPDIF and set the format to CONSUMER
      SET (ulAESCTL, (REG_AESCTL_DOFMT | REG_AESCTL_DIOTYPE));

      SET (ulAESCTL, REG_AESCTL_DOCS_CON_C9n);	// general category device
      SET (ulAESCTL, REG_AESCTL_DOCS_CON_C8n);
      CLR (ulAESCTL, REG_AESCTL_DOCS_CON_C2n);	// copy permitted
      SET (ulAESCTL, REG_AESCTL_DOCS_CON_C3n);	// no pre-emphasis
      SET (ulAESCTL, REG_AESCTL_DOCS_CON_C15n);	// 1st generation

      switch (pA->ulBaseSampleRate)
	{
	case 48000:
	  SET (ulAESCTL, REG_AESCTL_DOCS_CON_FC0);
	  CLR (ulAESCTL, REG_AESCTL_DOCS_CON_FC1);
	  break;
	case 44100:
	default:
	  CLR (ulAESCTL, REG_AESCTL_DOCS_CON_FC0);
	  CLR (ulAESCTL, REG_AESCTL_DOCS_CON_FC1);
	  break;
	case 32000:
	  CLR (ulAESCTL, REG_AESCTL_DOCS_CON_FC0);
	  SET (ulAESCTL, REG_AESCTL_DOCS_CON_FC1);
	  break;
	}

      RegWrite (&pA->RegAESCTL, ulAESCTL);
      break;
    default:
      return (HSTATUS_INVALID_MIXER_VALUE);
    }

  pA->ulDigitalFormat = ulDigitalFormat;
  return (HSTATUS_OK);
}

/////////////////////////////////////////////////////////////////////////////
USHORT
CalibrateConverters (PADAPTER pA)
// This function recalibrates the A>D and D>A converters
/////////////////////////////////////////////////////////////////////////////
{
  //USHORT    usDevice;
  PULONG pulGSTAT = (PULONG) & pA->pMailbox->Registers.GSTAT;
  int i;

  //DPF(("CalibrateConverters\n"));
/*
	// if there is any device that is active, don't allow this to proceed
	for( usDevice=0; usDevice<NUM_WAVE_DEVICES; usDevice++ )
	{
		if( pA->Device[ usDevice ].usMode > MODE_IDLE )
			return( HSTATUS_INVALID_MODE );
	}
*/
  // Make the DA and AD CAL bits low
  RegBitSet (&pA->RegDACTL, (REG_DACTL_ADCAL | REG_DACTL_DACAL), 0);
  // Put the DA and AD into CAL mode
  RegBitSet (&pA->RegDACTL, (REG_DACTL_ADCAL | REG_DACTL_DACAL), 1);
  // Four sets of L/R clock transitions seem to be just right to get the 
  // converters to calibrate...
  for (i = 0; i < 4; i++)
    {
      // LRCK is currently high, wait for it to go low
      while (!(READ_REGISTER_ULONG (pulGSTAT) & REG_GSTAT_LRCK))
	;
      // LRCK is currently low, wait for it to go high
      while ((READ_REGISTER_ULONG (pulGSTAT) & REG_GSTAT_LRCK))
	;
    }

  RegBitSet (&pA->RegDACTL, (REG_DACTL_ADCAL | REG_DACTL_DACAL), 0);

  return (HSTATUS_OK);
}

/////////////////////////////////////////////////////////////////////////////
ULONG
GetDigitalInStatus (PADAPTER pA)
/////////////////////////////////////////////////////////////////////////////
{
  PDEVICE pD = &pA->Device[WAVE_RECORD1_DEVICE];
  PULONG pulGSTAT = (PULONG) & pA->pMailbox->Registers.GSTAT;
  ULONG ulChStatus;
  ULONG ulErrors;
  ULONG ulStatus = 0;

  // Clear any errors
  RegBitSet (&pA->RegAESCTL, REG_AESCTL_DISEL, 1);

  // Wait for LRCK transition to guarantee new status data is latched after DISEL change
  if (READ_REGISTER_ULONG (pulGSTAT) & REG_GSTAT_LRCK)
    {
      while ((READ_REGISTER_ULONG (pulGSTAT) & REG_GSTAT_LRCK))
	;
    }
  else
    {
      while (!(READ_REGISTER_ULONG (pulGSTAT) & REG_GSTAT_LRCK))
	;
    }

  // Read Channel Status right now (subframe 2 only)
  ulChStatus = READ_REGISTER_ULONG (pulGSTAT);

  if (ulChStatus & REG_GSTAT_DICS_CONSUMER)	// if C0n is set, then we are in consumer mode
    {
      if (!(ulChStatus & REG_GSTAT_DICS_CON_C3n))	// if C3n is clear, then emphasis is on
	SET (ulStatus, MIXVAL_DIS_EMPHASIS);

      if (!(ulChStatus & REG_GSTAT_DICS_CON_C2n))	// if C2n is clear, then copy permitted
	SET (ulStatus, MIXVAL_DIS_COPY_PERMITTED);

      if (!(ulChStatus & REG_GSTAT_DICS_CON_ORIGn))	// if ORIGn is clear, then its original
	SET (ulStatus, MIXVAL_DIS_ORIGINAL);
    }
  else
    {
      SET (ulStatus, MIXVAL_DIS_PROFESSIONAL);
      if (!(ulChStatus & REG_GSTAT_DICS_PRO_EM1))	// if EM0 is clear, then emphasis is on
	SET (ulStatus, MIXVAL_DIS_EMPHASIS);
    }

  if (!(ulChStatus & REG_GSTAT_DICS_NONAUDIO))	// if C1n is clear, then data is non-audio for both Consumer & Pro modes
    {
      SET (ulStatus, MIXVAL_DIS_NONAUDIO_MODE);
      // make sure the volume controls of the RECORD1 device are all the way up (so no calculation happens)
#ifndef MULTI_INPUT
      pD->lLeftVolume = 0xFFFF;
      pD->lRightVolume = 0xFFFF;
      pD->bMute = FALSE;
#endif
    }

  // make DISEL low
  RegBitSet (&pA->RegAESCTL, REG_AESCTL_DISEL, 0);

  // Wait for LRCK transition to guarantee new status data is latched after DISEL change
  if ((READ_REGISTER_ULONG (pulGSTAT) & REG_GSTAT_LRCK))
    {
      while ((READ_REGISTER_ULONG (pulGSTAT) & REG_GSTAT_LRCK))
	;
    }
  else
    {
      while (!(READ_REGISTER_ULONG (pulGSTAT) & REG_GSTAT_LRCK))
	;
    }

  // read errors - 3 bits: b7 - b9
  ulErrors = (READ_REGISTER_ULONG (pulGSTAT) >> 7) & 0x7;

  //Status bits in low word, error bits in hi word
  return (ulStatus | (ulErrors << 16));
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
//  REGISTER
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
void
RegDefine (PREGISTER pReg, void * pAddress)
/////////////////////////////////////////////////////////////////////////////
{
  pReg->pAddress = pAddress;
  pReg->ulValue = READ_REGISTER_ULONG (pReg->pAddress);
}

/////////////////////////////////////////////////////////////////////////////
void
RegDefineInit (PREGISTER pReg, void *pAddress, ULONG ulInitialValue)
/////////////////////////////////////////////////////////////////////////////
{
  pReg->pAddress = pAddress;
  pReg->ulValue = ulInitialValue;

  WRITE_REGISTER_ULONG (pReg->pAddress, pReg->ulValue);
}

/////////////////////////////////////////////////////////////////////////////
void
RegWrite (PREGISTER pReg, ULONG ulValue)
/////////////////////////////////////////////////////////////////////////////
{
  pReg->ulValue = ulValue;

  //DPF(("Write %08lx %08lx\n", pReg->pAddress, pReg->ulValue ));
  WRITE_REGISTER_ULONG (pReg->pAddress, pReg->ulValue);
}

/////////////////////////////////////////////////////////////////////////////
ULONG
RegRead (PREGISTER pReg)
/////////////////////////////////////////////////////////////////////////////
{
  pReg->ulValue = READ_REGISTER_ULONG (pReg->pAddress);
  return (pReg->ulValue);
}

/////////////////////////////////////////////////////////////////////////////
void
RegBitSet (PREGISTER pReg, ULONG ulBitPosition, BOOLEAN bValue)
/////////////////////////////////////////////////////////////////////////////
{
  pReg->ulValue &= ~ulBitPosition;	// clear position(s)
  if (bValue)
    pReg->ulValue |= ulBitPosition;	// if SET then set position(s)

  //DPF(("Write %08lx %08lx\n", pReg->pAddress, pReg->ulValue ));
  WRITE_REGISTER_ULONG (pReg->pAddress, pReg->ulValue);
}

/////////////////////////////////////////////////////////////////////////////
void
RegWriteMask (PREGISTER pReg, ULONG ulMask, ULONG ulValue)
/////////////////////////////////////////////////////////////////////////////
{
  CLR (pReg->ulValue, ulMask);
  SET (pReg->ulValue, (ulValue & ulMask));

  //DPF(("Write %08lx %08lx\n", pReg->pAddress, pReg->ulValue ));
  WRITE_REGISTER_ULONG (pReg->pAddress, pReg->ulValue);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
//  MIDI
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

#ifndef MULTI_INPUT

/////////////////////////////////////////////////////////////////////////////
USHORT
HalMidiWrite (PMIDI pM, PBYTE pBuffer, ULONG ulBytesToWrite,
	      PULONG pulBytesWritten)
// The hardware detects each DWORD write into the Midi Circular Buffer Address 
// space and increments a counter for the number of DWORDs written.
/////////////////////////////////////////////////////////////////////////////
{
  ULONG ulHWIndex, ulPacked, ulBytesAvailable;
  LONG lDWordsAvailable;
  USHORT usBytesUsed;

  *pulBytesWritten = 0;

  if (!pM)
    {
      DPF (("HalMidiWrite pM Invalid\n"));
      return (HSTATUS_ADAPTER_NOT_OPEN);
    }

  if (pM->usMode != MODE_PLAY)
    {
      DPF (("HalMidiWrite Invalid Mode!\n"));
      return (HSTATUS_INVALID_MODE);
    }

  ulHWIndex = READ_REGISTER_ULONG (pM->pulHWIndex) >> 1;

  lDWordsAvailable = (LONG) ulHWIndex - (LONG) pM->ulPCIndex;	// for PLAY, the HWIndex is the tail, and the PCIndex is the head
  // if lDWordsAvailable is exactly zero, then the buffer is completely empty and we can fill it up.
  if (lDWordsAvailable <= 0)
    lDWordsAvailable += MIDI_CIRCULAR_BUFFER_SIZE;

  // always remove one from the size available so we never overfill
  lDWordsAvailable--;

  // Sanity Check, make sure we always leave one DWORD free
  if (lDWordsAvailable >= MIDI_CIRCULAR_BUFFER_SIZE)
    lDWordsAvailable = MIDI_CIRCULAR_BUFFER_SIZE - 1;

  if (lDWordsAvailable)
    {
      // figure out the number of bytes available
      ulBytesAvailable = lDWordsAvailable * sizeof (ULONG);
    }
  else
    {
      //DPF(("Full: PC %ld HW %ld\n", pM->ulPCIndex, ulHWIndex ));
      DB ('F', COLOR_BOLD);
      return (HSTATUS_BUFFER_FULL);
    }

  // limit the number of bytes to write
  if (ulBytesToWrite > ulBytesAvailable)
    {
      DB ('L', COLOR_UNDERLINE);
      ulBytesToWrite = ulBytesAvailable;
    }

  ulPacked = usBytesUsed = 0;
  while (ulBytesToWrite--)
    {
      ulPacked |= (ULONG) (*pBuffer++) << (usBytesUsed * 8);
      usBytesUsed++;
      // was this the last byte of a DWORD?
      if (usBytesUsed >= sizeof (ULONG))
	{
	  WRITE_REGISTER_ULONG (&pM->pBuffer[pM->ulPCIndex++], ulPacked);
	  // check for circular buffer wrap
	  if (pM->ulPCIndex >= MIDI_CIRCULAR_BUFFER_SIZE)
	    pM->ulPCIndex = 0;

	  // let driver know how many bytes we actually wrote this time
	  *pulBytesWritten += usBytesUsed;	// should always be 4
	  ulPacked = usBytesUsed = 0;
	}
    }

  // if we have some left over bytes
  if (usBytesUsed)
    {
      // pad unused bytes with 0xFD
      ulPacked |= (ULONG) (0xFDFDFDFD << (usBytesUsed * 8));
      WRITE_REGISTER_ULONG (&pM->pBuffer[pM->ulPCIndex++], ulPacked);
      // check for circular buffer wrap
      if (pM->ulPCIndex >= MIDI_CIRCULAR_BUFFER_SIZE)
	pM->ulPCIndex = 0;
      // let driver know how many bytes we actually wrote this time
      *pulBytesWritten += usBytesUsed;
    }

  //DPF(("DWords %ld Avail %ld Written %ld\n", 
  //  dwDWordsAvailable, dwBytesAvailable, dwBytesWritten ));
  return (HSTATUS_OK);
}

/////////////////////////////////////////////////////////////////////////////
USHORT
HalMidiRead (PMIDI pM, PBYTE pBuffer, ULONG ulBufferSize, PULONG pulBytesRead)
//  Reads all messages from the midi input buffer on the hardware
/////////////////////////////////////////////////////////////////////////////
{
  ULONG ulHWIndex, ulPacked, ulBytesToGo, ulBytesRead;
  BYTE ucByte;

  *pulBytesRead = 0;

  if (!pM)
    {
      DPF (("HalMidiRead pM Invalid\n"));
      return (HSTATUS_ADAPTER_NOT_OPEN);
    }

  if (pM->usMode != MODE_RECORD)
    {
      DPF (("HalMidiRead Invalid Mode!\n"));
      return (HSTATUS_INVALID_MODE);
    }

  ulHWIndex = READ_REGISTER_ULONG (pM->pulHWIndex) >> 1;

  ulBytesRead = 0;

  while ((ulHWIndex != pM->ulPCIndex) && (ulBufferSize > ulBytesRead))
    {
      ulPacked = READ_REGISTER_ULONG (&pM->pBuffer[pM->ulPCIndex++]);
      // Check for buffer wrap
      if (pM->ulPCIndex >= MIDI_CIRCULAR_BUFFER_SIZE)
	pM->ulPCIndex = 0;

      // we always process 4 bytes at a time
      ulBytesToGo = sizeof (ULONG);

      while (ulBytesToGo--)
	{
	  // Save the byte to the buffer
	  ucByte = (BYTE) (ulPacked & 0xFF);
	  // Shift the next byte to read into position
	  ulPacked >>= 8;

	  // if this is a pad byte, don't save it
	  if (ucByte == 0xFD)
	    continue;

	  // save the byte to the buffer
	  *pBuffer++ = ucByte;
	  // Yep, we read a byte
	  ulBytesRead++;
	}
    }

  *pulBytesRead = ulBytesRead;
  //DX8((BYTE)ulBytesRead,COLOR_BOLD);

  return (HSTATUS_OK);
}

////////////////////////////////////////////////////////////////////////////
USHORT
HalMidiSetMode (PMIDI pM, USHORT usMode)
////////////////////////////////////////////////////////////////////////////
{
  PADAPTER pA = (PADAPTER) pM->pA;

  //DPF(("HalMidiSetMode %d\n", pM->usDeviceIndex ));

  switch (usMode)
    {
    case MODE_IDLE:
      switch (pM->usDeviceIndex)
	{
	case MIDI_PLAY0_DEVICE:
	  //DPF(("MIDI Out 1: IDLE\n"));
	  //RegBitSet( &pA->RegMIDICTL, REG_MIDICTL_M1TEN, 0 );
	  break;
	case MIDI_PLAY1_DEVICE:
	  //DPF(("MIDI Out 2: IDLE\n"));
	  //RegBitSet( &pA->RegMIDICTL, REG_MIDICTL_M2TEN, 0 );
	  break;
	case MIDI_RECORD0_DEVICE:
	  //DPF(("MIDI In 1: IDLE\n"));
	  // turn off midi receive and it's interrupt
	  RegBitSet (&pA->RegMIDICTL, (REG_MIDICTL_M1REN | REG_MIDICTL_M1IE),
		     0);
	  break;
	case MIDI_RECORD1_DEVICE:
	  //DPF(("MIDI In 2: IDLE\n"));
	  // turn off midi receive and it's interrupt
	  RegBitSet (&pA->RegMIDICTL, (REG_MIDICTL_M2REN | REG_MIDICTL_M2IE),
		     0);
	  break;
	}
      break;
    case MODE_PLAY:
      switch (pM->usDeviceIndex)
	{
	case MIDI_PLAY0_DEVICE:
	  RegBitSet (&pA->RegMIDICTL, REG_MIDICTL_M1TEN, 0);
	  break;
	case MIDI_PLAY1_DEVICE:
	  RegBitSet (&pA->RegMIDICTL, REG_MIDICTL_M2TEN, 0);
	  break;
	}

      pM->ulPCIndex = 0;
      WRITE_REGISTER_ULONG (pM->pulHWIndex, 0);

      switch (pM->usDeviceIndex)
	{
	case MIDI_PLAY0_DEVICE:
	  //DPF(("MIDI Out 1: PLAY\n"));
	  RegBitSet (&pA->RegMIDICTL, REG_MIDICTL_M1TEN, 1);
	  break;
	case MIDI_PLAY1_DEVICE:
	  //DPF(("MIDI Out 2: PLAY\n"));
	  RegBitSet (&pA->RegMIDICTL, REG_MIDICTL_M2TEN, 1);
	  break;
	}
      break;
    case MODE_RECORD:
      // zero out the pointers for this midi device
      pM->ulPCIndex = 0;
      WRITE_REGISTER_ULONG (pM->pulHWIndex, 0);

      switch (pM->usDeviceIndex)
	{
	case MIDI_RECORD0_DEVICE:
	  //DPF(("MIDI In 1: RECORD\n"));
	  // turn on midi receive and it's interrupt
	  RegBitSet (&pA->RegMIDICTL, (REG_MIDICTL_M1REN | REG_MIDICTL_M1IE),
		     1);
	  break;
	case MIDI_RECORD1_DEVICE:
	  //DPF(("MIDI In 2: RECORD\n"));
	  // turn on midi receive and it's interrupt
	  RegBitSet (&pA->RegMIDICTL, (REG_MIDICTL_M2REN | REG_MIDICTL_M2IE),
		     1);
	  break;
	}
      break;
    default:
      return (HSTATUS_INVALID_MODE);
    }

  pM->usMode = usMode;

  return (HSTATUS_OK);
}

#endif // MULTI_INPUT

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//
//  MIXER
//
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
USHORT
HalMixerInit (PMIXER pM)
/////////////////////////////////////////////////////////////////////////////
{
  PADAPTER pA = (PADAPTER) pM->pA;
  int i;

  pA->bLevels = TRUE;
  pA->bAutoMute = TRUE;
  pA->bSyncStart = TRUE;
  pA->bADHPF = TRUE;

  for (i = 0; i < NUM_WAVE_DEVICES; i++)
    {
      pA->Device[i].sLeftLevel = 0;
      pA->Device[i].sRightLevel = 0;
      pA->Device[i].lLeftVolume = 0xFFFF;
      pA->Device[i].lRightVolume = 0xFFFF;
      pA->Device[i].bMute = FALSE;
    }

  return (HSTATUS_OK);
}

/////////////////////////////////////////////////////////////////////////////
PDEVICE
GetDeviceFromMixerLine (PADAPTER pA, USHORT usLine)
/////////////////////////////////////////////////////////////////////////////
{
  switch (usLine)
    {
    case LINE_RECORD_0:
      return (&pA->Device[WAVE_RECORD0_DEVICE]);
    case LINE_RECORD_1:
      return (&pA->Device[WAVE_RECORD1_DEVICE]);
#ifdef MULTI_INPUT		/////////////////////////////////////////////////////
    case LINE_RECORD_2:
      return (&pA->Device[WAVE_RECORD2_DEVICE]);
    case LINE_RECORD_3:
      return (&pA->Device[WAVE_RECORD3_DEVICE]);
    case LINE_RECORD_4:
      return (&pA->Device[WAVE_RECORD4_DEVICE]);
#endif /////////////////////////////////////////////////////
    case LINE_PLAY_0:
      return (&pA->Device[WAVE_PLAY0_DEVICE]);
    case LINE_PLAY_1:
      return (&pA->Device[WAVE_PLAY1_DEVICE]);
    }
  return (NULL);
}

/////////////////////////////////////////////////////////////////////////////
USHORT
HalMixerSetControl (PMIXER pM, USHORT usDstLine, USHORT usSrcLine,
		    USHORT usControl, USHORT usChannel, ULONG ulValue)
/////////////////////////////////////////////////////////////////////////////
{
  PADAPTER pA;
  PDEVICE pD;

  //DPF(("HalMixerSetControl\n"));

  if (!pM)
    {
      DPF (("HalMixerSetControl pM Invalid\n"));
      return (HSTATUS_ADAPTER_NOT_OPEN);
    }

  pA = (PADAPTER) pM->pA;

  if (!pA)
    {
      DPF (("HalMixerSetControl pA Invalid\n"));
      return (HSTATUS_ADAPTER_NOT_OPEN);
    }

  switch (usDstLine)
    {
    case LINE_ADAPTER:
      if (usSrcLine != LINE_NO_SOURCE)
	return (HSTATUS_INVALID_MIXER_LINE);

      switch (usControl)
	{
	case CONTROL_TRIM:	// set trim to +4/-10
	  switch (ulValue)
	    {
	    case MIXVAL_TRIM_PROFESSIONAL:
	      pA->ulTrim = ulValue;
	      RegBitSet (&pA->RegDACTL, REG_DACTL_TRIM, 0);
	      break;
	    case MIXVAL_TRIM_CONSUMER:
	      pA->ulTrim = ulValue;
	      RegBitSet (&pA->RegDACTL, REG_DACTL_TRIM, 1);
	      break;
	    default:
	      return (HSTATUS_INVALID_MIXER_VALUE);
	    }
	  break;
	case CONTROL_LEVELS:	// turn level computation on/off
	  if (ulValue)
	    pA->bLevels = TRUE;
	  else
	    pA->bLevels = FALSE;
	  break;
	case CONTROL_RECALIBRATE:
	  CalibrateConverters (pA);
	  // this is here just so we can pass the Windows Mixer Test
	  if (ulValue)
	    pA->bCalibrate = TRUE;
	  else
	    pA->bCalibrate = FALSE;
	  break;
	case CONTROL_CLOCKSOURCE:
	  return (SetSampleRate
		  (pA, pA->ulCurrentSampleRate, ulValue, pA->ulClockReference,
		   FALSE));
	case CONTROL_CLOCKREFERENCE:
	  return (SetSampleRate
		  (pA, pA->ulCurrentSampleRate, pA->ulClockSource, ulValue,
		   FALSE));
	case CONTROL_AUTOCLOCKSELECT:
	  if (ulValue)
	    pA->bAutoClockSelection = TRUE;
	  else
	    pA->bAutoClockSelection = FALSE;
	  break;
	case CONTROL_DIGITALFORMAT:
	  return (SetDigitalFormat (pA, ulValue));
	case CONTROL_MONITOR_SOURCE:
	  switch (ulValue)
	    {
	    case MIXVAL_MONSRC_ANALOGIN:
	      // this must be set before the call to SetSampleRate!
	      pA->ulMonitorSource = ulValue;

	      // If auto clock source selection is on, try and restore the clock source
	      // we don't care if this fails, we will always allow the monitoring analog
	      if (pA->bAutoClockSelection && !pA->bASIO)	// don't do this in ASIO mode
		{
		  SetSampleRate (pA, pA->ulCurrentSampleRate,
				 pA->ulPreviousClockSource,
				 pA->ulPreviousClockReference, FALSE);
		}

	      RegWriteMask (&pA->RegDACTL, REG_DACTL_MIXSRCMASK,
			    REG_DACTL_MIXSRC_DAR1);
	      break;
	    case MIXVAL_MONSRC_DIGITALIN:
	      // Must set the clock source to digital first

	      // if the current clock source is not digital, remember the old values
	      if (pA->ulClockSource != MIXVAL_CLKSRC_DIGITAL)
		{
		  pA->ulPreviousClockSource = pA->ulClockSource;
		  pA->ulPreviousClockReference = pA->ulClockReference;
		}
	      if (SetSampleRate
		  (pA, pA->ulCurrentSampleRate, MIXVAL_CLKSRC_DIGITAL,
		   MIXVAL_CLKREF_AUTO, FALSE))
		return (HSTATUS_INVALID_MIXER_VALUE);

	      pA->ulMonitorSource = ulValue;
	      RegWriteMask (&pA->RegDACTL, REG_DACTL_MIXSRCMASK,
			    REG_DACTL_MIXSRC_DAR2);
	      break;
	    case MIXVAL_MONSRC_ANALOGOUT:
	      // this must be set before the call to SetSampleRate!
	      pA->ulMonitorSource = ulValue;

	      // If auto clock source selection is on, try and restore the clock source
	      // we don't care if this fails, we will always allow the monitoring analog
	      if (pA->bAutoClockSelection && !pA->bASIO)	// don't do this in ASIO mode
		{
		  SetSampleRate (pA, pA->ulCurrentSampleRate,
				 pA->ulPreviousClockSource,
				 pA->ulPreviousClockReference, FALSE);
		}
	      // Make sure the MONITOR is turned off for Analog Out
	      pA->bAnalogMonitor = FALSE;
	      RegBitSet (&pA->RegDACTL, REG_DACTL_DAT1MIXEN, 0);

	      RegWriteMask (&pA->RegDACTL, REG_DACTL_MIXSRCMASK,
			    REG_DACTL_MIXSRC_DAT1);
	      break;
	    case MIXVAL_MONSRC_DIGITALOUT:
	      // this must be set before the call to SetSampleRate!
	      pA->ulMonitorSource = ulValue;

	      // If auto clock source selection is on, try and restore the clock source
	      // we don't care if this fails, we will always allow the monitoring analog
	      if (pA->bAutoClockSelection && !pA->bASIO)	// don't do this in ASIO mode
		{
		  SetSampleRate (pA, pA->ulCurrentSampleRate,
				 pA->ulPreviousClockSource,
				 pA->ulPreviousClockReference, FALSE);
		}
	      // Make sure the MONITOR is turned off for Digital Out
	      pA->bDigitalMonitor = FALSE;
	      RegBitSet (&pA->RegDACTL, REG_DACTL_DAT2MIXEN, 0);

	      RegWriteMask (&pA->RegDACTL, REG_DACTL_MIXSRCMASK,
			    REG_DACTL_MIXSRC_DAT2);
	      break;
	    default:
	      return (HSTATUS_INVALID_MIXER_VALUE);
	    }
	  break;
	case CONTROL_AUTOMUTE:
	  if (ulValue)
	    {
	      pA->bAutoMute = TRUE;
	      RegBitSet (&pA->RegDACTL, REG_DACTL_DAAUTOn, 0);
	    }
	  else
	    {
	      pA->bAutoMute = FALSE;
	      RegBitSet (&pA->RegDACTL, REG_DACTL_DAAUTOn, 1);
	    }
	  break;
	case CONTROL_SYNCSTART:
	  if (ulValue)
	    pA->bSyncStart = TRUE;
	  else
	    pA->bSyncStart = FALSE;
	  break;
	case CONTROL_ADHIPASSFILTER:
	  if (ulValue)
	    {
	      pA->bADHPF = TRUE;
	      RegBitSet (&pA->RegDACTL, REG_DACTL_ADHDP, 0);	// this enables the hipass filter
	    }
	  else
	    {
	      pA->bADHPF = FALSE;
	      RegBitSet (&pA->RegDACTL, REG_DACTL_ADHDP, 1);	// this disables the hipass filter
	    }
	  break;
	case CONTROL_RECORD_DITHER:
	  switch (ulValue)
	    {
	    case MIXVAL_DITHER_NONE:
	    case MIXVAL_DITHER_TRIANGULAR_PDF:
	    case MIXVAL_DITHER_TRIANGULAR_NS_PDF:
	    case MIXVAL_DITHER_RECTANGULAR_PDF:
	      pA->ulRecordDither = ulValue;
	      break;
	    default:
	      return (HSTATUS_INVALID_MIXER_VALUE);
	    }
	  break;
	case CONTROL_PLAY_DITHER:
	  switch (ulValue)
	    {
	    case MIXVAL_DITHER_NONE:
	    case MIXVAL_DITHER_TRIANGULAR_PDF:
	    case MIXVAL_DITHER_TRIANGULAR_NS_PDF:
	    case MIXVAL_DITHER_RECTANGULAR_PDF:
	      pA->ulPlayDither = ulValue;
	      break;
	    default:
	      return (HSTATUS_INVALID_MIXER_VALUE);
	    }
	  break;
	case CONTROL_MONITOR_OFF_PLAY:
	  if (ulValue)
	    pA->bMonitorOffPlay = TRUE;
	  else
	    pA->bMonitorOffPlay = FALSE;
	  break;
	case CONTROL_MONITOR_ON_RECORD:
	  if (ulValue)
	    pA->bMonitorOnRecord = TRUE;
	  else
	    pA->bMonitorOnRecord = FALSE;
	  break;
	default:
	  return (HSTATUS_INVALID_MIXER_CONTROL);
	}
      break;

      // Destinations
    case LINE_RECORD_0:
    case LINE_RECORD_1:
#ifdef MULTI_INPUT
    case LINE_RECORD_2:
    case LINE_RECORD_3:
    case LINE_RECORD_4:
#endif // MULTI_INPUT
      pD = GetDeviceFromMixerLine (pA, usDstLine);
      if (!pD)
	return (HSTATUS_INVALID_MIXER_LINE);

      switch (usSrcLine)
	{
	case LINE_NO_SOURCE:	// controls on the destination
	  switch (usControl)
	    {
#ifdef MULTI_INPUT
	    case CONTROL_INPUT_SOURCE:
	      if (ulValue)
		{
		  // make sure the sample clock source is set to digital first
		  if (pA->ulClockSource != MIXVAL_CLKSRC_DIGITAL)
		    {
		      pA->ulPreviousClockSource = pA->ulClockSource;
		      pA->ulPreviousClockReference = pA->ulClockReference;
		    }
		  if (SetSampleRate
		      (pA, pA->ulCurrentSampleRate, MIXVAL_CLKSRC_DIGITAL,
		       MIXVAL_CLKREF_AUTO, FALSE))
		    return (HSTATUS_INVALID_MIXER_VALUE);

		  pD->ulInputSource = 1;
		}
	      else
		{
		  // should we try and set the sample clock source back to internal?
		  pD->ulInputSource = 0;
		}

	      RegBitSet (&pA->RegDACTL,
			 (REG_DACTL_DAR1SRC << (usDstLine - LINE_RECORD_0)),
			 (BOOLEAN) pD->ulInputSource);
	      break;
#else // MULTI_INPUT
	    case CONTROL_VOLUME:
	      if (usChannel)
		pD->lRightVolume = ulValue;
	      else
		pD->lLeftVolume = ulValue;
	      break;
	    case CONTROL_MUTE:
	      if (ulValue)
		pD->bMute = TRUE;
	      else
		pD->bMute = FALSE;
	      break;
	    case CONTROL_DIGITALSTATUS:
	      // nothing to set here (don't generate an error either)
	      break;
#endif // MULTI_INPUT
	    default:
	      return (HSTATUS_INVALID_MIXER_CONTROL);
	    }
	  break;
	  // Sources
	case LINE_ANALOG_IN:
	  switch (usControl)
	    {
	    default:
	      return (HSTATUS_INVALID_MIXER_CONTROL);
	    }
	  break;
	case LINE_DIGITAL_IN:
	  switch (usControl)
	    {
	    case CONTROL_DIGITALSTATUS:
	      // nothing to set here (don't generate an error either)
	      break;
	    default:
	      return (HSTATUS_INVALID_MIXER_CONTROL);
	    }
	  break;
	default:
	  return (HSTATUS_INVALID_MIXER_LINE);
	}
      break;
    case LINE_ANALOG_OUT:
      switch (usSrcLine)
	{
	case LINE_NO_SOURCE:	// controls on the destination
	  switch (usControl)
	    {
	    default:
	      return (HSTATUS_INVALID_MIXER_CONTROL);
	    }
	  break;
	case LINE_PLAY_0:
	  pD = &pA->Device[WAVE_PLAY0_DEVICE];

	  switch (usControl)
	    {
	    case CONTROL_VOLUME:
	      if (usChannel)
		pD->lRightVolume = ulValue;
	      else
		pD->lLeftVolume = ulValue;
	      break;
	    case CONTROL_MUTE:
	      if (ulValue)
		pD->bMute = TRUE;
	      else
		pD->bMute = FALSE;
	      break;
	    case CONTROL_MONITOR:
	      if (ulValue && (pA->ulMonitorSource != MIXVAL_MONSRC_ANALOGOUT))
		{
		  pA->bAnalogMonitor = TRUE;
		  RegBitSet (&pA->RegDACTL, REG_DACTL_DAT1MIXEN, 1);
		  //DPF(("Enabling Analog Monitor\n"));
		}
	      else
		{
		  pA->bAnalogMonitor = FALSE;
		  RegBitSet (&pA->RegDACTL, REG_DACTL_DAT1MIXEN, 0);
		  //DPF(("Disabling Analog Monitor\n"));
		}
	      break;
	    default:
	      return (HSTATUS_INVALID_MIXER_CONTROL);
	    }
	  break;
	default:
	  return (HSTATUS_INVALID_MIXER_LINE);
	}
      break;
    case LINE_DIGITAL_OUT:
      switch (usSrcLine)
	{
	case LINE_NO_SOURCE:	// controls on the destination
	  switch (usControl)
	    {
	    default:
	      return (HSTATUS_INVALID_MIXER_CONTROL);
	    }
	  break;
	case LINE_PLAY_1:
	  pD = &pA->Device[WAVE_PLAY1_DEVICE];

	  switch (usControl)
	    {
	    case CONTROL_VOLUME:
	      if (usChannel)
		pD->lRightVolume = ulValue;
	      else
		pD->lLeftVolume = ulValue;
	      break;
	    case CONTROL_MUTE:
	      if (ulValue)
		pD->bMute = TRUE;
	      else
		pD->bMute = FALSE;
	      break;
	    case CONTROL_MONITOR:
	      if (ulValue
		  && (pA->ulMonitorSource != MIXVAL_MONSRC_DIGITALOUT))
		{
		  pA->bDigitalMonitor = TRUE;
		  RegBitSet (&pA->RegDACTL, REG_DACTL_DAT2MIXEN, 1);
		  //DPF(("Enabling Digital Monitor\n"));
		}
	      else
		{
		  pA->bDigitalMonitor = FALSE;
		  RegBitSet (&pA->RegDACTL, REG_DACTL_DAT2MIXEN, 0);
		  //DPF(("Disabling Digital Monitor\n"));
		}
	      break;
	    case CONTROL_DIGITALSTATUS:
	      pA->ulDigitalOutStatus = ulValue;
	      SetDigitalFormat (pA, pA->ulDigitalFormat);	// force update of digital out status
	      break;
	    default:
	      return (HSTATUS_INVALID_MIXER_CONTROL);
	    }
	  break;
	default:
	  return (HSTATUS_INVALID_MIXER_LINE);
	}
      break;
    default:
      return (HSTATUS_INVALID_MIXER_LINE);
    }

  return (HSTATUS_OK);
}

/////////////////////////////////////////////////////////////////////////////
USHORT
HalMixerGetControl (PMIXER pM, USHORT usDstLine, USHORT usSrcLine,
		    USHORT usControl, USHORT usChannel, PULONG pulValue)
//  if control is on the source, then a source line must be specified,
//  otherwise, the control is assume to be on the destination (usSrcLine should be LINE_NO_SOURCE)
/////////////////////////////////////////////////////////////////////////////
{
  PADAPTER pA;
  PDEVICE pD;

  //DPF(("HalMixerGetControl\n"));

  if (!pM)
    {
      DPF (("HalMixerGetControl pM Invalid\n"));
      return (HSTATUS_ADAPTER_NOT_OPEN);
    }

  pA = (PADAPTER) pM->pA;

  if (!pA)
    {
      DPF (("HalMixerGetControl pA Invalid\n"));
      return (HSTATUS_ADAPTER_NOT_OPEN);
    }

  // start out with no value
  *pulValue = 0;

  switch (usDstLine)
    {
    case LINE_ADAPTER:
      // only allow NO_SOURCE thru...
      if (usSrcLine != LINE_NO_SOURCE)
	return (HSTATUS_INVALID_MIXER_LINE);

      switch (usControl)
	{
	case CONTROL_NUMCHANNELS:	// the number of channels this line has
	  *pulValue = 1;
	  break;
	case CONTROL_DIGITALFORMAT:
	  *pulValue = pA->ulDigitalFormat;
	  break;
	case CONTROL_CLOCKSOURCE:
	  *pulValue = pA->ulClockSource;
	  break;
	case CONTROL_CLOCKREFERENCE:
	  *pulValue = pA->ulClockReference;
	  break;
	case CONTROL_CLOCKRATE:
	  *pulValue = pA->ulBaseSampleRate;
	  break;
	case CONTROL_AUTOCLOCKSELECT:
	  *pulValue = pA->bAutoClockSelection;
	  break;
	case CONTROL_TRIM:
	  *pulValue = pA->ulTrim;
	  break;
	case CONTROL_LEVELS:
	  *pulValue = pA->bLevels;
	  break;
	case CONTROL_RECALIBRATE:
	  // this is here just so we can pass the Windows Mixer Test
	  *pulValue = pA->bCalibrate;
	  break;
	case CONTROL_MONITOR_SOURCE:
	  *pulValue = pA->ulMonitorSource;
	  break;
	case CONTROL_AUTOMUTE:
	  *pulValue = pA->bAutoMute;
	  break;
	case CONTROL_SYNCSTART:
	  *pulValue = pA->bSyncStart;
	  break;
	case CONTROL_ADHIPASSFILTER:
	  *pulValue = pA->bADHPF;
	  break;
	case CONTROL_RECORD_DITHER:
	  *pulValue = pA->ulRecordDither;
	  break;
	case CONTROL_PLAY_DITHER:
	  *pulValue = pA->ulPlayDither;
	  break;
	case CONTROL_MONITOR_OFF_PLAY:
	  *pulValue = pA->bMonitorOffPlay;
	  break;
	case CONTROL_MONITOR_ON_RECORD:
	  *pulValue = pA->bMonitorOnRecord;
	  break;
	case CONTROL_SERIALNUMBER:
	  *pulValue = MAKEULONG (pA->usSerialNumber, pA->usRevision);
	  break;
	case CONTROL_MFGDATE:
	  *pulValue = MAKEULONG (pA->usMonthDay, pA->usYear);
	  break;
	default:
	  return (HSTATUS_INVALID_MIXER_CONTROL);
	}
      break;
      // Destinations
    case LINE_RECORD_0:
    case LINE_RECORD_1:
#ifdef MULTI_INPUT
    case LINE_RECORD_2:
    case LINE_RECORD_3:
    case LINE_RECORD_4:
#endif // MULTI_INPUT
      pD = GetDeviceFromMixerLine (pA, usDstLine);
      if (!pD)
	return (HSTATUS_INVALID_MIXER_LINE);

      switch (usSrcLine)
	{
	case LINE_NO_SOURCE:	// controls on the destination
	  switch (usControl)
	    {
	    case CONTROL_NUMCHANNELS:	// the number of channels this line has
	      *pulValue = 2;
	      break;
#ifdef MULTI_INPUT
	    case CONTROL_INPUT_SOURCE:
	      *pulValue = pD->ulInputSource;
	      break;
#else // MULTI_INPUT
	      // These controls only exist if we are not the MULTI_INPUT driver
	    case CONTROL_VOLUME:
	      if (usChannel)
		*pulValue = pD->lRightVolume;
	      else
		*pulValue = pD->lLeftVolume;
	      break;
	    case CONTROL_MUTE:
	      *pulValue = pD->bMute;
	      break;
	    case CONTROL_DIGITALSTATUS:
	      // on the single input driver, only put the digital status on the second record device
	      if (usDstLine != LINE_RECORD_1)
		return (HSTATUS_INVALID_MIXER_CONTROL);

	      *pulValue = GetDigitalInStatus (pA);
	      break;
	    case CONTROL_PEAKMETER:
	      if (usChannel)
		*pulValue = pD->sRightLevel;
	      else
		*pulValue = pD->sLeftLevel;
	      break;
#endif // MULTI_INPUT
	    default:
	      return (HSTATUS_INVALID_MIXER_CONTROL);
	    }
	  break;
	  // Sources
	case LINE_ANALOG_IN:
#ifndef MULTI_INPUT
	  // only put the analog in on the first device
	  if (usDstLine != LINE_RECORD_0)
	    return (HSTATUS_INVALID_MIXER_LINE);
#endif // MULTI_INPUT
	  switch (usControl)
	    {
	    case CONTROL_NUMCHANNELS:	// the number of channels this line has
	      *pulValue = 2;
	      break;
#ifdef MULTI_INPUT
	    case CONTROL_PEAKMETER:
	      {
		USHORT usDevice;

		// go find the first analog device that is actively recording
		for (usDevice = 0; usDevice < NUM_WAVE_DEVICES; usDevice++)
		  {
		    pD = &pA->Device[usDevice];

		    if (pD->ulInputSource != 0)	// if the input source is digital for this device, skip
		      continue;

		    // if the mode is record and the device is enabled
		    if ((pD->usMode == MODE_RECORD)
			&& (pA->RegDACTL.ulValue & pD->ulDeviceEnableBit))
		      {
			if (usChannel)
			  *pulValue = pD->sRightLevel;
			else
			  *pulValue = pD->sLeftLevel;

			// all done
			break;
		      }
		  }
	      }
	      break;
#endif // MULTI_INPUT
	    default:
	      return (HSTATUS_INVALID_MIXER_CONTROL);
	    }
	  break;
	case LINE_DIGITAL_IN:
#ifndef MULTI_INPUT
	  // only put the digital in on the second device
	  if (usDstLine != LINE_RECORD_1)
	    return (HSTATUS_INVALID_MIXER_LINE);
#endif // MULTI_INPUT
	  switch (usControl)
	    {
	    case CONTROL_NUMCHANNELS:	// the number of channels this line has
	      *pulValue = 2;
	      break;
#ifdef MULTI_INPUT		// the MULTI_INPUT driver has meters & digital status on each digital in source line
	    case CONTROL_PEAKMETER:
	      {
		USHORT usDevice;

		// go find the first digital device that is actively recording
		for (usDevice = 0; usDevice < NUM_WAVE_DEVICES; usDevice++)
		  {
		    pD = &pA->Device[usDevice];

		    if (pD->ulInputSource != 1)	// if the input source is analog for this device, skip
		      continue;

		    // if the mode is record and the device is enabled
		    if ((pD->usMode == MODE_RECORD)
			&& (pA->RegDACTL.ulValue & pD->ulDeviceEnableBit))
		      {
			if (usChannel)
			  *pulValue = pD->sRightLevel;
			else
			  *pulValue = pD->sLeftLevel;

			// all done
			break;
		      }
		  }
	      }
	      break;
	    case CONTROL_DIGITALSTATUS:
	      // the MULTI_INPUT driver has digital status on each digital in source line
	      *pulValue = GetDigitalInStatus (pA);
	      break;
#endif // MULTI_INPUT
	    default:
	      return (HSTATUS_INVALID_MIXER_CONTROL);
	    }
	  break;
	default:
	  return (HSTATUS_INVALID_MIXER_LINE);
	}
      break;
    case LINE_ANALOG_OUT:
      switch (usSrcLine)
	{
	case LINE_NO_SOURCE:	// controls on the destination
	  switch (usControl)
	    {
	    case CONTROL_NUMCHANNELS:	// the number of channels this line has
	      *pulValue = 2;
	      break;
	    default:
	      return (HSTATUS_INVALID_MIXER_CONTROL);
	    }
	  break;
	case LINE_PLAY_0:
	  pD = &pA->Device[WAVE_PLAY0_DEVICE];

	  switch (usControl)
	    {
	    case CONTROL_NUMCHANNELS:	// the number of channels this line has
	      *pulValue = 2;
	      break;
	    case CONTROL_VOLUME:
	      if (usChannel)
		*pulValue = pD->lRightVolume;
	      else
		*pulValue = pD->lLeftVolume;
	      break;
	    case CONTROL_PEAKMETER:
	      if (usChannel)
		*pulValue = pD->sRightLevel;
	      else
		*pulValue = pD->sLeftLevel;
	      break;
	    case CONTROL_MUTE:
	      *pulValue = pD->bMute;
	      break;
	    case CONTROL_MONITOR:
	      *pulValue = pA->bAnalogMonitor;
	      break;
	    default:
	      return (HSTATUS_INVALID_MIXER_CONTROL);
	    }
	  break;
	default:
	  return (HSTATUS_INVALID_MIXER_LINE);
	}
      break;
    case LINE_DIGITAL_OUT:
      switch (usSrcLine)
	{
	case LINE_NO_SOURCE:	// controls on the destination
	  switch (usControl)
	    {
	    case CONTROL_NUMCHANNELS:	// the number of channels this line has
	      *pulValue = 2;
	      break;
	    default:
	      return (HSTATUS_INVALID_MIXER_CONTROL);
	    }
	  break;
	case LINE_PLAY_1:
	  pD = &pA->Device[WAVE_PLAY1_DEVICE];

	  switch (usControl)
	    {
	    case CONTROL_NUMCHANNELS:	// the number of channels this line has
	      *pulValue = 2;
	      break;
	    case CONTROL_VOLUME:
	      if (usChannel)
		*pulValue = pD->lRightVolume;
	      else
		*pulValue = pD->lLeftVolume;
	      break;
	    case CONTROL_PEAKMETER:
	      if (usChannel)
		*pulValue = pD->sRightLevel;
	      else
		*pulValue = pD->sLeftLevel;
	      break;
	    case CONTROL_MUTE:
	      *pulValue = pD->bMute;
	      break;
	    case CONTROL_MONITOR:
	      *pulValue = pA->bDigitalMonitor;
	      break;
	    case CONTROL_DIGITALSTATUS:
	      *pulValue = pA->ulDigitalOutStatus;
	      break;
	    default:
	      return (HSTATUS_INVALID_MIXER_CONTROL);
	    }			// switch( usControl )
	  break;
	default:
	  return (HSTATUS_INVALID_MIXER_LINE);
	}			// switch( usSrcLine )
      break;
    default:
      return (HSTATUS_INVALID_MIXER_LINE);
    }				// switch( usDstLine )

  return (HSTATUS_OK);
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// EEPROM
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
USHORT
EEPromGetData (PADAPTER pA, BOOLEAN bFirstWord)
// usDataLength is always 16 bits
////////////////////////////////////////////////////////////////////////////
{
  int i;
  ULONG ulControl = pA->RegPlxCntrl.ulValue;
  USHORT usValue = 0;

  //DPF(("\nEEPromGetData: "));
  CLR (ulControl, PLX_CNTRL_EEPROM_WRITE_DATA);

  for (i = 15 + bFirstWord; i >= 0; i--)	// 1st word has one dummy bit
    {
      CLR (ulControl, PLX_CNTRL_EEPROM_CLOCK);	// clock lo
      RegWrite (&pA->RegPlxCntrl, ulControl);	// for 500ns, min
      RegWrite (&pA->RegPlxCntrl, ulControl);
      RegWrite (&pA->RegPlxCntrl, ulControl);

      // Read the data bit
      ulControl = RegRead (&pA->RegPlxCntrl);
      if (ulControl & PLX_CNTRL_EEPROM_READ_DATA)
	{
	  //DPF(("1"));
	  SET (usValue, (1 << i));
	}
      else
	{
	  //DPF(("0"));
	}

      SET (ulControl, PLX_CNTRL_EEPROM_CLOCK);	// clock hi
      RegWrite (&pA->RegPlxCntrl, ulControl);	// for 500 ns,min
      RegWrite (&pA->RegPlxCntrl, ulControl);
      RegWrite (&pA->RegPlxCntrl, ulControl);
    }
  return (usValue);
}

////////////////////////////////////////////////////////////////////////////
void
EEPromSendData (PADAPTER pA, USHORT usData, USHORT usDataLength)
// usDataLength is in bits
////////////////////////////////////////////////////////////////////////////
{
  int i;
  ULONG ulControl = pA->RegPlxCntrl.ulValue;
  USHORT usBit;

  //DPF(("EEPromSendData: %04x ",usData));

  for (i = usDataLength - 1; i >= 0; i--)
    {
      // commands are sent MSB first
      usBit = (usData & (1 << i));	// mask just the bit we care about this time around
      // if the bit is set, set the write data bit
      if (usBit)
	{
	  //DPF(("1"));
	  SET (ulControl, PLX_CNTRL_EEPROM_WRITE_DATA);
	}
      else
	{
	  //DPF(("0"));
	  CLR (ulControl, PLX_CNTRL_EEPROM_WRITE_DATA);
	}

      CLR (ulControl, PLX_CNTRL_EEPROM_CLOCK);	// clock lo for
      RegWrite (&pA->RegPlxCntrl, ulControl);	// 500 ns,min
      RegWrite (&pA->RegPlxCntrl, ulControl);
      RegWrite (&pA->RegPlxCntrl, ulControl);

      SET (ulControl, PLX_CNTRL_EEPROM_CLOCK);	// clock hi
      RegWrite (&pA->RegPlxCntrl, ulControl);	// for 500 ns, min
      RegWrite (&pA->RegPlxCntrl, ulControl);
      RegWrite (&pA->RegPlxCntrl, ulControl);
    }
}

////////////////////////////////////////////////////////////////////////////
USHORT
EEPromReadWord (PADAPTER pA, USHORT usAddress)
////////////////////////////////////////////////////////////////////////////
{
  USHORT usValue;

  // Enable serial EEPROM CS[3:0]

  //Clear EECS and EESK - EESK must be low 50ns prior to EECS hi
  RegWriteMask (&pA->RegPlxCntrl,
		(PLX_CNTRL_EEPROM_CHIP_SELECT | PLX_CNTRL_EEPROM_CLOCK), 0);

  //DPF(("CHIP_SELECT HI\n"));
  RegBitSet (&pA->RegPlxCntrl, PLX_CNTRL_EEPROM_CHIP_SELECT, 1);

  EEPromSendData (pA, (USHORT) (EEPROM_READ | usAddress),
		  EEPROM_COMMAND_LENGTH);
  usValue = EEPromGetData (pA, TRUE);

  // Disable serial EEPROM CS[3:0]
  //DPF(("CHIP_SELECT LO\n"));
  RegBitSet (&pA->RegPlxCntrl, PLX_CNTRL_EEPROM_CHIP_SELECT, 0);

  return (usValue);
}

////////////////////////////////////////////////////////////////////////////
void
EEPromReadAll (PADAPTER pA, PUSHORT pEEPromBuffer)
//  Reads entire contents of NM93CS46 EEPROM - 64 words.
//      Utilizes sequetial read function.
////////////////////////////////////////////////////////////////////////////
{
  ULONG i;

  // Clear EECS and EESK - EESK must be low 50ns prior to EECS hi
  RegWriteMask (&pA->RegPlxCntrl,
		(PLX_CNTRL_EEPROM_CHIP_SELECT | PLX_CNTRL_EEPROM_CLOCK), 0);

  //DPF(("CHIP_SELECT HI\n"));
  RegBitSet (&pA->RegPlxCntrl, PLX_CNTRL_EEPROM_CHIP_SELECT, 1);

  EEPromSendData (pA, (USHORT) (EEPROM_READ), EEPROM_COMMAND_LENGTH);

  for (i = 0; i < 64; i++)
    *(pEEPromBuffer + i) = EEPromGetData (pA, (BOOLEAN) (i == 0));

  // Disable serial EEPROM CS[3:0]
  //DPF(("CHIP_SELECT LO\n"));
  RegBitSet (&pA->RegPlxCntrl, PLX_CNTRL_EEPROM_CHIP_SELECT, 0);
}

////////////////////////////////////////////////////////////////////////////
void
EEPromWriteWord (PADAPTER pA, USHORT usAddress, USHORT usData)
////////////////////////////////////////////////////////////////////////////
{
  //Clear EECS and EESK - EESK must be low 50ns prior to EECS hi
  RegWriteMask (&pA->RegPlxCntrl,
		(PLX_CNTRL_EEPROM_CHIP_SELECT | PLX_CNTRL_EEPROM_CLOCK), 0);

  RegBitSet (&pA->RegPlxCntrl, PLX_CNTRL_EEPROM_CHIP_SELECT, 1);

  //Send write enable command
  EEPromSendData (pA, (USHORT) EEPROM_WEN, EEPROM_COMMAND_LENGTH);

  //Clear EECS and EESK - EESK must be low 50ns prior to EECS hi
  RegWriteMask (&pA->RegPlxCntrl,
		(PLX_CNTRL_EEPROM_CHIP_SELECT | PLX_CNTRL_EEPROM_CLOCK), 0);

  //One more write to guarantee EECS is low a minimum of 250 ns
  RegBitSet (&pA->RegPlxCntrl, PLX_CNTRL_EEPROM_CHIP_SELECT, 0);

  //Now, ready to pull EECS hi to start write command
  RegBitSet (&pA->RegPlxCntrl, PLX_CNTRL_EEPROM_CHIP_SELECT, 1);

  //Send write command/address
  EEPromSendData (pA, (USHORT) (EEPROM_WRITE | usAddress),
		  EEPROM_COMMAND_LENGTH);

  //Send write data
  EEPromSendData (pA, usData, EEPROM_DATA_LENGTH);


  //Set EECS low for a minimum of 250ns
  RegBitSet (&pA->RegPlxCntrl, PLX_CNTRL_EEPROM_CHIP_SELECT, 0);
  RegBitSet (&pA->RegPlxCntrl, PLX_CNTRL_EEPROM_CHIP_SELECT, 0);

  //Set EECS hi for min of 500 ns prior to reading ready flag
  RegBitSet (&pA->RegPlxCntrl, PLX_CNTRL_EEPROM_CHIP_SELECT, 1);
  RegBitSet (&pA->RegPlxCntrl, PLX_CNTRL_EEPROM_CHIP_SELECT, 1);
  RegBitSet (&pA->RegPlxCntrl, PLX_CNTRL_EEPROM_CHIP_SELECT, 1);


  // Wait for the ready flag to go high to indicate write is complete
  // NOTE: This could take up to 10 milliseconds!
  while (!(RegRead (&pA->RegPlxCntrl) & PLX_CNTRL_EEPROM_READ_DATA))
    ;


  RegBitSet (&pA->RegPlxCntrl, PLX_CNTRL_EEPROM_CHIP_SELECT, 0);

  return;
}

////////////////////////////////////////////////////////////////////////////
void
EEPromWriteAll (PADAPTER pA, USHORT * pEEPromBuffer)
////////////////////////////////////////////////////////////////////////////
{
  USHORT i;

  for (i = 0; i < 64; i++)
    EEPromWriteWord (pA, i, *(pEEPromBuffer + i));

}
