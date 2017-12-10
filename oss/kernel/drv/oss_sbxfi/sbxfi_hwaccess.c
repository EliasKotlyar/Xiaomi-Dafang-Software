/**
*******************************************************************************
Confidential & Proprietary
Private & Confidential
Creative Confidential
*******************************************************************************
*/
/**
*******************************************************************************
Copyright (C) Creative Technology, Ltd., 2007. All rights reserved.
*******************************************************************************
**/
#include "oss_sbxfi_cfg.h"
#include <oss_pci.h>
#include "sbxfi.h"
#include "20k1reg.h"
#include "hwaccess.h"

static const int 
volume_table[MIXER_VOLSTEPS+1] =
{
	0x0000000, 0x000010a, 0x0000110, 0x0000116, 0x000011d, 
	0x0000124, 0x000012a, 0x0000131, 0x0000138, 0x0000140, 
	0x0000147, 0x000014f, 0x0000157, 0x000015f, 0x0000167, 
	0x000016f, 0x0000178, 0x0000180, 0x0000189, 0x0000193, 
	0x000019c, 0x00001a6, 0x00001af, 0x00001b9, 0x00001c4, 
	0x00001ce, 0x00001d9, 0x00001e4, 0x00001ef, 0x00001fb, 
	0x0000207, 0x0000213, 0x000021f, 0x000022c, 0x0000239, 
	0x0000246, 0x0000254, 0x0000262, 0x0000270, 0x000027e, 
	0x000028d, 0x000029c, 0x00002ac, 0x00002bc, 0x00002cc, 
	0x00002dd, 0x00002ee, 0x0000300, 0x0000311, 0x0000324, 
	0x0000336, 0x000034a, 0x000035d, 0x0000371, 0x0000386, 
	0x000039b, 0x00003b0, 0x00003c6, 0x00003dd, 0x00003f4, 
	0x000040c, 0x0000424, 0x000043c, 0x0000456, 0x0000470, 
	0x000048a, 0x00004a5, 0x00004c1, 0x00004dd, 0x00004fa, 
	0x0000518, 0x0000536, 0x0000555, 0x0000575, 0x0000596, 
	0x00005b7, 0x00005d9, 0x00005fc, 0x0000620, 0x0000644, 
	0x000066a, 0x0000690, 0x00006b7, 0x00006df, 0x0000708, 
	0x0000732, 0x000075d, 0x0000789, 0x00007b6, 0x00007e4, 
	0x0000813, 0x0000843, 0x0000874, 0x00008a7, 0x00008da, 
	0x000090f, 0x0000945, 0x000097c, 0x00009b5, 0x00009ef, 
	0x0000a2a, 0x0000a67, 0x0000aa5, 0x0000ae4, 0x0000b25, 
	0x0000b68, 0x0000bac, 0x0000bf1, 0x0000c38, 0x0000c81, 
	0x0000ccc, 0x0000d18, 0x0000d66, 0x0000db6, 0x0000e08, 
	0x0000e5c, 0x0000eb1, 0x0000f09, 0x0000f63, 0x0000fbe, 
	0x000101c, 0x000107c, 0x00010df, 0x0001143, 0x00011aa, 
	0x0001214, 0x000127f, 0x00012ee, 0x000135f, 0x00013d2, 
	0x0001448, 0x00014c1, 0x000153d, 0x00015bc, 0x000163d, 
	0x00016c2, 0x000174a, 0x00017d4, 0x0001863, 0x00018f4, 
	0x0001989, 0x0001a21, 0x0001abd, 0x0001b5c, 0x0001c00
};

unsigned char
DetectAndConfigureHardware (sbxfi_devc_t * devc)
{
  unsigned short wData;

  // Default setting for hendrix card is memory access, so must get IO access port from bar5.
  // bar0 will be converted to IO access in SwitchToXFiCore()
  if (devc->hw_family == HW_UAA)
    {
      // Base IO address is at register lcoation 0x24 (bar5)
      pci_read_config_word (devc->osdev, PCI_BASE_ADDRESS_5, &wData);
      devc->wIOPortBase = wData & 0xFFFC;
    }
  else
    {
      // Get the IO base address
      pci_read_config_word (devc->osdev, PCI_BASE_ADDRESS_0, &wData);
      devc->wIOPortBase = wData & 0xFFFC;
    }

  return TRUE;
}

unsigned char
IsVistaCompatibleHardware (sbxfi_devc_t * devc)
{
  // Check the subsystem id
  if (devc->hw_family == HW_UAA)
    {
      return TRUE;
    }

  return FALSE;
}

void
SwitchToXFiCore (sbxfi_devc_t * devc)
{
  unsigned int bar0, bar1, bar2, bar3, bar4, bar5, irq, clSize, lTimer;

  // program the hardware to X-Fi core.
  // Check whether its hendrix card
  // Save the previous memory/io address

  pci_read_config_dword (devc->osdev, PCI_BASE_ADDRESS_0, &bar0);
  pci_read_config_dword (devc->osdev, PCI_BASE_ADDRESS_1, &bar1);
  pci_read_config_dword (devc->osdev, PCI_BASE_ADDRESS_2, &bar2);
  pci_read_config_dword (devc->osdev, PCI_BASE_ADDRESS_3, &bar3);
  pci_read_config_dword (devc->osdev, PCI_BASE_ADDRESS_4, &bar4);
  pci_read_config_dword (devc->osdev, PCI_BASE_ADDRESS_5, &bar5);

  pci_read_config_dword (devc->osdev, PCI_INTERRUPT_LINE, &irq);
  pci_read_config_dword (devc->osdev, PCI_CFGHDR_CACHESIZE, &clSize);
  pci_read_config_dword (devc->osdev, PCI_CFGHDR_LATENCY, &lTimer);

  cmn_err (CE_CONT, "Switching to xfi core...\n");

  // Switch to XFi core config space with BAR0
  pci_write_config_dword (devc->osdev, 0xA0, 0x87654321);

  // copy Base I/O address from UAA core to X-Fi core
  pci_write_config_dword (devc->osdev, PCI_BASE_ADDRESS_5, bar5);

  // Switch to XFi core config space without BAR0
  pci_write_config_dword (devc->osdev, 0xA0, 0x12345678);

  // copy all other setting from UAA config space to X-Fi config space
  pci_write_config_dword (devc->osdev, PCI_BASE_ADDRESS_1, bar1);
  pci_write_config_dword (devc->osdev, PCI_BASE_ADDRESS_2, bar2);
  pci_write_config_dword (devc->osdev, PCI_BASE_ADDRESS_3, bar3);
  pci_write_config_dword (devc->osdev, PCI_BASE_ADDRESS_4, bar4);

  pci_write_config_dword (devc->osdev, PCI_INTERRUPT_LINE, irq);
  pci_write_config_dword (devc->osdev, PCI_CFGHDR_CACHESIZE, clSize);
  pci_write_config_dword (devc->osdev, PCI_CFGHDR_LATENCY, lTimer);

  pci_write_config_dword (devc->osdev, PCI_CFGHDR_CMDREG, 0x07);

  /*
     NOTE: 
     The steps below is needed to switch the control signals to X-Fi core.

     It needs to access the mode change register which reside in the UAA core BAR0 + 0x00003ffc.
     Since this demo sample is a real-mode DOS program, it will need other services such as XMS to access 
     memory above 1MB.  

     Here is the pseudo code:

     WriteMemory((bar0 + 0x00003ffc),0x43544c58);  // CTLX
     WriteMemory((bar0 + 0x00003ffc),0x43544c2d);  // CTL-
     WriteMemory((bar0 + 0x00003ffc),0x43544c46);  // CTLF
     WriteMemory((bar0 + 0x00003ffc),0x43544c69);  // CTLi  
   */
}


CTSTATUS
InitHardware (sbxfi_devc_t * devc)
{
  unsigned int gctlorg;
  unsigned int dwIterCount, dwData;


  // kick in auto-init
  gctlorg = HwRead20K1 (devc, GCTL);
  HwWrite20K1 (devc, GCTL, (~0x2 & gctlorg));
  HwWrite20K1 (devc, GCTL, (0x2 | gctlorg));
  osDelayms (1000);
  // poll for AID in GCTL to be set
  dwIterCount = 0x400000;
  do
    {
      dwData = HwRead20K1 (devc, GCTL);
    }
  while (!(dwData & 0x00100000) && --dwIterCount);

  // AID bit is not set when time out, return failure.
  if (!(dwData & 0x00100000))
    return CTSTATUS_ERROR;

  gctlorg = HwRead20K1 (devc, GCTL);
  HwWrite20K1 (devc, GCTL, (0x100aa3 | gctlorg));
  osDelayms (10000);

  HwWrite20K1 (devc, GIE, 0);
  HwWrite20K1 (devc, SRCIP(0), 0);
  osDelayms (30000);

  if (((HwRead20K1 (devc, PLLCTL)) != 0x1480a001)
      && ((HwRead20K1 (devc, PLLCTL)) != 0x1480a731))
    {
      HwWrite20K1 (devc, PLLCTL, 0x1480a001);
    }
  osDelayms (40000);
  dwData = HwRead20K1 (devc, PLLCTL);

  // configure GPIO per the card's family.
  switch (devc->hw_family)
    {
    case HW_055x:
      HwWrite20K1 (devc, GPIOCTL, 0x13fe);
      break;
    case HW_073x:
      HwWrite20K1 (devc, GPIOCTL, 0x00e6);
      break;
    case HW_UAA:
      HwWrite20K1 (devc, GPIOCTL, 0x00c2);
      break;
    case HW_ORIG:
    default:
      HwWrite20K1 (devc, GPIOCTL, 0x01e6);
      break;
    }

  return CTSTATUS_SUCCESS;
}

CTSTATUS
AllocateBuffers (sbxfi_devc_t * devc, sbxfi_portc_t * portc)
{
  int ctStatus = CTSTATUS_SUCCESS;

#if 0
  if (devc->pdwPageTable == NULL)
    ctStatus = CTSTATUS_NOMEMORY;
  else
    {
      // alloc playL buffer
      portc->pdwPlayLBuffer = CONTIG_MALLOC (devc->osdev,
					     portc->dwPlayLBufferSize,
					     MEMLIMIT_32BITS,
					     &portc->dwPlayLPhysAddx, portc->playl_dma_handle);

      if (portc->pdwPlayLBuffer == NULL)
	ctStatus = CTSTATUS_NOMEMORY;
      else
	{
	  // alloc playR buffer
	  portc->pdwPlayRBuffer = CONTIG_MALLOC (devc->osdev,
						 portc->dwPlayRBufferSize,
						 MEMLIMIT_32BITS,
						 &portc->dwPlayLPhysAddx,portc->playr_dma_handle);

	  if (portc->pdwPlayRBuffer == NULL)
	    ctStatus = CTSTATUS_NOMEMORY;
	  else
	    {
	      // alloc recordL buffer
	      portc->pdwRecordLBuffer = CONTIG_MALLOC (devc->osdev,
						       portc->
						       dwRecordLBufferSize,
						       MEMLIMIT_32BITS,
						       &portc->
						       dwRecordLPhysAddx, portc->recl_dma_handle);

	      if (portc->pdwRecordLBuffer == NULL)
		ctStatus = CTSTATUS_NOMEMORY;
	      else
		{
		  // alloc recordR buffer
		  portc->pdwRecordRBuffer = CONTIG_MALLOC (devc->osdev,
							   portc->
							   dwRecordRBufferSize,
							   MEMLIMIT_32BITS,
							   &portc->
							   dwRecordRPhysAddx, portc->recr_dma_handle);
		  if (portc->pdwRecordRBuffer == NULL)
		    ctStatus = CTSTATUS_NOMEMORY;
		}
	    }
	}
    }

  if (ctStatus != CTSTATUS_SUCCESS)
    FreeBuffers (devc, portc);
#endif

  return ctStatus;
}

void
FreeBuffers (sbxfi_devc_t * devc, sbxfi_portc_t * portc)
{
#if 0
  if (portc->pdwRecordLBuffer != NULL)
    {
      CONTIG_FREE (devc->osdev, portc->pdwRecordLBuffer,
		   portc->dwRecordLBufferSize, portc->recl_dma_handle);
      portc->pdwRecordLBuffer = NULL;
    }

  if (portc->pdwRecordRBuffer != NULL)
    {
      CONTIG_FREE (devc->osdev, portc->pdwRecordRBuffer,
		   portc->dwRecordRBufferSize, portc->recr_dma_handle);
      portc->pdwRecordRBuffer = NULL;
    }

  if (portc->pdwPlayLBuffer != NULL)
    {
      CONTIG_FREE (devc->osdev, portc->pdwPlayLBuffer,
		   portc->dwPlayLBufferSize, portc->playl_dma_handle);
      portc->pdwPlayLBuffer = NULL;
    }

  if (portc->pdwPlayRBuffer != NULL)
    {
      CONTIG_FREE (devc->osdev, portc->pdwPlayRBuffer,
		   portc->dwPlayRBufferSize, portc->playr_dma_handle);
      portc->pdwPlayRBuffer = NULL;
    }
#endif
}

void
_SetupSB055xADC (sbxfi_devc_t * devc, unsigned int src, unsigned char mic20db)
{
  unsigned short gpioorg;
  unsigned short gpioval = 0x28;


  // check and set the following GPIO bits accordingly
  //   ADC_Gain       = GPIO2
  //   Mic_Pwr_on     = GPIO7 
  //   Digital_IO_Sel = GPIO8      
  //   Mic_Sw         = GPIO9
  //   Aux/MicLine_Sw = GPIO12
  switch (src)
    {
    case ADC_SRC_MICIN:
      gpioval = 0x28;
      if (mic20db)
	gpioval |= 4;
      break;

    case ADC_SRC_LINEIN:
      gpioval = 0;
      break;

    case ADC_SRC_VIDEO:
      gpioval = 0x100;		// not supported, set to digital
      break;

    case ADC_SRC_AUX:
      gpioval = 0x1000;
      break;

    case ADC_SRC_NONE:
      gpioval = 0x100;		// set to digital
      break;

    default:
      break;
    }

  gpioorg = (unsigned short) HwRead20K1 (devc, GPIO);
  gpioorg &= 0xec7b;
  gpioorg |= gpioval;
  HwWrite20K1 (devc, GPIO, gpioorg);

  return;
}

void
_SetupADC (sbxfi_devc_t * devc, unsigned int src, unsigned char mic20db)
{
  unsigned int i = 0;
  unsigned short gpioorg;
  unsigned short input_source;
  unsigned int adcdata = 0;

  input_source = 0x100;		// default to analog
  switch (src)
    {
    case ADC_SRC_MICIN:
      adcdata = 0x1;
      input_source = 0x180;	// set GPIO7 to select Mic
      break;

    case ADC_SRC_LINEIN:
      adcdata = 0x2;
      break;

    case ADC_SRC_VIDEO:
      adcdata = 0x4;
      break;

    case ADC_SRC_AUX:
      adcdata = 0x8;
      break;

    case ADC_SRC_NONE:
      adcdata = 0x0;
      input_source = 0x0;	// set to Digital
      break;

    default:
      break;
    }


  HwWrite20K1PCI (devc, 0xcc, 0x8c);
  HwWrite20K1PCI (devc, 0xcc, 0x0e);
  if (((HwRead20K1PCI (devc, 0xcc)) & 0xff) != 0xaa)
    {
      HwWrite20K1PCI (devc, 0xcc, 0xee);
      HwWrite20K1PCI (devc, 0xcc, 0xaa);
    }

  if (((HwRead20K1PCI (devc, 0xcc)) & 0xff) != 0xaa)
    return;

  HwWrite20K1PCI (devc, 0xEC, 0x05);	//write to i2c status control

  HwWrite20K1PCI (devc, 0xE0, 0x001a0080);
  HwWrite20K1PCI (devc, 0xE4, 0x080e);

  i = 0;
  while (i != 0x800000)
    {
      i = ((HwRead20K1PCI (devc, 0xEC)) & 0x800000);
    }

  HwWrite20K1PCI (devc, 0xE0, 0x001a0080);
  HwWrite20K1PCI (devc, 0xE4, 0x0a18);

  i = 0;
  while (i != 0x800000)
    {
      i = ((HwRead20K1PCI (devc, 0xEC)) & 0x800000);
    }

  HwWrite20K1PCI (devc, 0xE0, 0x001a0080);

  if (mic20db)
    HwWrite20K1PCI (devc, 0xE4, 0xf71c);
  else
    HwWrite20K1PCI (devc, 0xE4, 0xcf1c);

  i = 0;
  while (i != 0x800000)
    {
      i = ((HwRead20K1PCI (devc, 0xEC)) & 0x800000);
    }

  HwWrite20K1PCI (devc, 0xE0, 0x001a0080);

  if (mic20db)
    HwWrite20K1PCI (devc, 0xE4, 0xf71e);
  else
    HwWrite20K1PCI (devc, 0xE4, 0xcf1e);

  i = 0;
  while (i != 0x800000)
    {
      i = ((HwRead20K1PCI (devc, 0xEC)) & 0x800000);
    }

  HwWrite20K1PCI (devc, 0xE0, 0x001a0080);
  HwWrite20K1PCI (devc, 0xE4, 0x8628);

  i = 0;
  while (i != 0x800000)
    {
      i = ((HwRead20K1PCI (devc, 0xEC)) & 0x800000);
    }

  HwWrite20K1PCI (devc, 0xE0, 0x001a0080);
  HwWrite20K1PCI (devc, 0xE4, 0x2a | (adcdata << 0x8));

  i = 0;
  while (i != 0x800000)
    {
      i = ((HwRead20K1PCI (devc, 0xEC)) & 0x800000);
    }				//i2c ready poll

  gpioorg = (unsigned short) HwRead20K1 (devc, GPIO);
  gpioorg &= 0xfe7f;
  gpioorg |= input_source;
  HwWrite20K1 (devc, GPIO, gpioorg);

  i = 0;
  while (i != 0x800000)
    {
      i = ((HwRead20K1PCI (devc, 0xEC)) & 0x800000);
    }

  if (!((HwRead20K1 (devc, ID0)) & 0x100))
    {
      HwWrite20K1PCI (devc, 0xE0, 0x001a0080);
      HwWrite20K1PCI (devc, 0xE4, 0x2616);
    }

  return;
}

void
InitADC (sbxfi_devc_t * devc, unsigned int src, unsigned char mic20db)
{
  unsigned short wSSID;

  wSSID = devc->wSubsystemID;
  if ((wSSID == 0x0022) || (wSSID == 0x002F))
    {
      // Sb055x card
      _SetupSB055xADC (devc, src, mic20db);
    }
  else
    {
      _SetupADC (devc, src, mic20db);
    }

  return;
}

void
ResetDAC (sbxfi_devc_t * devc)
{
  unsigned int i = 0;
  unsigned short gpioorg;


  HwWrite20K1PCI (devc, 0xcc, 0x8c);
  HwWrite20K1PCI (devc, 0xcc, 0x0e);
  if (((HwRead20K1PCI (devc, 0xcc)) & 0xff) != 0xaa)
    {
      HwWrite20K1PCI (devc, 0xcc, 0xee);
      HwWrite20K1PCI (devc, 0xcc, 0xaa);
    }
  if (((HwRead20K1PCI (devc, 0xcc)) & 0xff) != 0xaa)
    return;

  HwWrite20K1PCI (devc, 0xEC, 0x05);	//write to i2c status control
  i = 0;
  while (i != 0x800000)
    {
      i = ((HwRead20K1PCI (devc, 0xEC)) & 0x800000);
    }

  // To be effective, need to reset the DAC twice.
  for (i = 0; i < 2; i++)
    {
      osDelayms (100000);
      gpioorg = (unsigned short) HwRead20K1 (devc, GPIO);
      gpioorg &= 0xfffd;
      HwWrite20K1 (devc, GPIO, gpioorg);
      osDelayms (1000);
      HwWrite20K1 (devc, GPIO, gpioorg | 0x2);
    }				//set gpio

  HwWrite20K1PCI (devc, 0xE0, 0x00180080);
  HwWrite20K1PCI (devc, 0xE4, 0x8001);

  i = 0;
  while (i != 0x800000)
    {
      i = ((HwRead20K1PCI (devc, 0xEC)) & 0x800000);
    }

  HwWrite20K1PCI (devc, 0xE0, 0x00180080);
  HwWrite20K1PCI (devc, 0xE4, 0x1002);

  i = 0;
  while (i != 0x800000)
    {
      i = ((HwRead20K1PCI (devc, 0xEC)) & 0x800000);
    }
}

void
InitDAC (sbxfi_devc_t * devc, sbxfi_portc_t * portc)
{
  unsigned int i = 0;
  unsigned int wData;
  unsigned short gpioorg;
  unsigned int dwSamplingRate;
  unsigned short wSSID;


  wSSID = devc->wSubsystemID;
  // if SB055x, unmute outputs
  if ((wSSID == 0x0022) || (wSSID == 0x002F))
    {
      gpioorg = (unsigned short) HwRead20K1 (devc, GPIO);
      gpioorg &= 0xffbf;	// set GPIO6 to low
      gpioorg |= 2;		// set GPIO1 to high
      HwWrite20K1 (devc, GPIO, gpioorg);

      return;
    }


  dwSamplingRate = portc->rate;

  // Mute outputs
  gpioorg = (unsigned short) HwRead20K1 (devc, GPIO);
  gpioorg &= 0xffbf;
  HwWrite20K1 (devc, GPIO, gpioorg);

  ResetDAC (devc);

  HwWrite20K1PCI (devc, 0xcc, 0x8c);
  HwWrite20K1PCI (devc, 0xcc, 0x0e);
  if (((HwRead20K1PCI (devc, 0xcc)) & 0xff) != 0xaa)
    {
      HwWrite20K1PCI (devc, 0xcc, 0xee);
      HwWrite20K1PCI (devc, 0xcc, 0xaa);
    }
  if (((HwRead20K1PCI (devc, 0xcc)) & 0xff) != 0xaa)
    return;

  HwWrite20K1PCI (devc, 0xEC, 0x05);	//write to i2c status control

  i = 0;
  while (i != 0x800000)
    {
      i = ((HwRead20K1PCI (devc, 0xEC)) & 0x800000);
    }

  if (dwSamplingRate == 48000)
    wData = 0x2400;
  else if (dwSamplingRate == 96000)
    wData = 0x2500;
  else if (dwSamplingRate == 192000)
    wData = 0x2600;
  else
    wData = 0x2400;

  HwWrite20K1PCI (devc, 0xE0, 0x00180080);
  HwWrite20K1PCI (devc, 0xE4, (wData | 0x6));

  i = 0;
  while (i != 0x800000)
    {
      i = ((HwRead20K1PCI (devc, 0xEC)) & 0x800000);
    }

  HwWrite20K1PCI (devc, 0xE0, 0x00180080);
  HwWrite20K1PCI (devc, 0xE4, (wData | 0x9));

  i = 0;
  while (i != 0x800000)
    {
      i = ((HwRead20K1PCI (devc, 0xEC)) & 0x800000);
    }

  HwWrite20K1PCI (devc, 0xE0, 0x00180080);
  HwWrite20K1PCI (devc, 0xE4, (wData | 0xc));

  i = 0;
  while (i != 0x800000)
    {
      i = ((HwRead20K1PCI (devc, 0xEC)) & 0x800000);
    }

  HwWrite20K1PCI (devc, 0xE0, 0x00180080);
  HwWrite20K1PCI (devc, 0xE4, (wData | 0xf));

  i = 0;
  while (i != 0x800000)
    {
      i = ((HwRead20K1PCI (devc, 0xEC)) & 0x800000);
    }

  // unmute outputs
  gpioorg = (unsigned short) HwRead20K1 (devc, GPIO);
  gpioorg = gpioorg | 0x40;
  HwWrite20K1 (devc, GPIO, gpioorg);
}


void
SetupPlayInputMapper (sbxfi_devc_t * devc, sbxfi_portc_t * portc)
{
/*
 * TODO: This routine supports only stereo
 */
  unsigned int i;
  unsigned int srcch;
  unsigned int dio1, dio2;
  unsigned int dwSamplingRate;


  srcch = portc->SrcChan;
  dio1 = portc->dwDAChan[0];
  dio2 = portc->dwDAChan[1];
  dwSamplingRate = portc->rate;

  // initialize input mappers
  for (i = 0; i < 0x50; i++)
    HwWrite20K1 (devc, DAOIMAP_START(i), 0);

  if (dwSamplingRate == 48000)
    {
      if (dio1 == 0)
	{
	  HwWrite20K1 (devc, DAOIMAP_START(dio1), 0);
	  for (i=1;i<portc->channels;i++)
	  HwWrite20K1 (devc, DAOIMAP_START(dio2),
		       (dio1 << 16) | GetAudioSrcChan (srcch+i));
	  HwWrite20K1 (devc, DAOIMAP_START(dio1),
		       (dio2 << 16) | GetAudioSrcChan (srcch));
	}
      else
	{
	  HwWrite20K1 (devc, DAOIMAP_START(0), 0);
	  HwWrite20K1 (devc, DAOIMAP_START(dio1),
		       (dio2 << 16) | GetAudioSrcChan (srcch));
	  for (i=1;i<portc->channels;i++)
	  HwWrite20K1 (devc, DAOIMAP_START(dio2),
		       (0 << 16) | GetAudioSrcChan (srcch+i));
	  HwWrite20K1 (devc, DAOIMAP_START(0), (dio1 << 16) | 0);
	}
    }
  else if (dwSamplingRate == 96000)
    {
      // input mapper.  Input mapper is a circular linked-list
      if (dio1 == 0)
	{
	  HwWrite20K1 (devc, DAOIMAP_START(dio1), 0);
	  for (i=1;i<portc->channels;i++)
	  HwWrite20K1 (devc, DAOIMAP_START(dio2),
		       ((dio1 + 2) << 16) | GetAudioSrcChan (srcch+i));
	  HwWrite20K1 (devc, DAOIMAP_START(dio1 + 2),
		       ((dio2 + 2) << 16) | GetAudioSrcChan (srcch + 0x80));
	  for (i=1;i<portc->channels;i++)
	  HwWrite20K1 (devc, DAOIMAP_START(dio2 + 2),
		       (dio1 << 16) | GetAudioSrcChan (srcch+i + 0x80));
	  HwWrite20K1 (devc, DAOIMAP_START(dio1),
		       (dio2 << 16) | GetAudioSrcChan (srcch));
	}
      else
	{
	  HwWrite20K1 (devc, DAOIMAP_START(0), 0);
	  HwWrite20K1 (devc, DAOIMAP_START(dio1),
		       (dio2 << 16) | GetAudioSrcChan (srcch));
	  for (i=1;i<portc->channels;i++)
	  HwWrite20K1 (devc, DAOIMAP_START(dio2),
		       ((dio1 + 2) << 16) | GetAudioSrcChan (srcch+i));
	  HwWrite20K1 (devc, DAOIMAP_START(dio1 + 2),
		       ((dio2 + 2) << 16) | GetAudioSrcChan (srcch + 0x80));
	  for (i=1;i<portc->channels;i++)
	  HwWrite20K1 (devc, DAOIMAP_START(dio2 + 2),
		       (0 << 16) | GetAudioSrcChan (srcch+i + 0x80));
	  HwWrite20K1 (devc, DAOIMAP_START(0), (dio1 << 16) | 0);
	}
    }
}

void
SetupPlayFormat (sbxfi_devc_t * devc, sbxfi_portc_t * portc)
{
  unsigned int i2sorg;
  unsigned int dio1;
  unsigned int dwSamplingRate;


  dio1 = portc->dwDAChan[0];
  dwSamplingRate = portc->rate;

  // Read I2S CTL.  Keep original value.
  i2sorg = HwRead20K1 (devc, I2SCTL);

#if 1
  i2sorg = i2sorg | 0x04040404; // All I2S outputs enabled
#else
  // setup I2S value to program
  switch (dio1)
    {
    case I2SA_L:
      i2sorg = i2sorg | 0x4;
      break;
    case I2SB_L:
      i2sorg = i2sorg | 0x400;
      break;
    case I2SC_L:
      i2sorg = i2sorg | 0x40000;
      break;
    case I2SD_L:
      i2sorg = i2sorg | 0x4000000;
      break;
    default:
      i2sorg = i2sorg | 0x4;
      break;
    }
#endif

  // Program I2S with proper sample rate and enable the correct I2S channel.
  i2sorg &= 0xfffffffc;
  if (dwSamplingRate == 96000)
    {
      i2sorg = i2sorg | 2;
      HwWrite20K1 (devc, I2SCTL, i2sorg);
    }
  else
    {
      i2sorg = i2sorg | 1;
      HwWrite20K1 (devc, I2SCTL, i2sorg);
    }
}

void
SetupPlayMixer (sbxfi_devc_t * devc, sbxfi_portc_t * portc)
{
  int i;
  unsigned int fixed_pitch;
  unsigned int srcArchn, srcArchnC;
  unsigned int srcPrchn, srcPrchnC;
  unsigned int srcArchn2, srcArchnC2;
  unsigned int srcch;
  unsigned int dwSamplingRate;
  unsigned int dwYData;

  srcch = portc->SrcChan;
  dwSamplingRate = portc->rate;

  // NOTE: Y-Data is a 14-bit immediate floating-point constant multiplier.
  // Adjust the Y-Data to control the multiplier.
  // This can be used to control the level of the signal.
  // dwYData = 0x1c00; // Original level used by Creative's driver.
  dwYData = volume_table[portc->vol_left];

  srcArchn = GetAudioSrcChan (srcch);
  srcArchnC = GetAudioSrcChan (srcch + 0x80);	// conjugate channel for srcch
  srcPrchn = GetParamPitchChan (srcch);
  srcPrchnC = GetParamPitchChan (srcch + 0x80);

  // since input is same as output, pitch is 1.0
  // convert to fixed-point 8.24 format, shift left 24 bit.
  fixed_pitch = 1;
  fixed_pitch = fixed_pitch << 24;

  // write the pitch to param ring of the corresponsing SRC pitch slot
  HwWrite20K1 (devc, PRING_LO_HI_START(srcPrchn), fixed_pitch);
  HwWrite20K1 (devc, PRING_LO_HI_START(srcPrchnC), fixed_pitch);

  WriteAMOP (devc, srcArchn, dwYData, srcArchn, 0);
  if (dwSamplingRate == 96000)
    {
      WriteAMOP (devc, srcArchnC, dwYData, srcArchnC, 0);
    }

  // Handle subsequent channels

  for (i=1;i<portc->channels;i++)
  {
  	  dwYData = volume_table[(i&1) ? portc->vol_right : portc->vol_left];

	  // Since we will use 1st SRC ch as pitch master, 
	  // we do not need to program the pitch for SRC ch2
	
	  srcArchn2 = GetAudioSrcChan (srcch+i);
	  srcArchnC2 = GetAudioSrcChan (srcch+i + 0x80);	// conjugate channel for srcch+i
	
	  WriteAMOP (devc, srcArchn2, dwYData, srcArchn2, 0);
	  if (dwSamplingRate == 96000)
	    {
	      WriteAMOP (devc, srcArchnC2, dwYData, srcArchnC2, 0);
	    }
  }
}

void
SetupAndStartPlaySRC (sbxfi_devc_t * devc, sbxfi_portc_t * portc)
{
  unsigned int Sa, Ladr, Ca, Ctl = 0x44c;
  unsigned int srcch;
  unsigned int dwSamplingRate;
  int count;
  int i;

  srcch = portc->SrcChan;
  dwSamplingRate = portc->rate;

  count = audio_engines[portc->dev]->dmap_out->bytes_in_use;

  //  start addx: 1st entry in page table.
  //  Note: this must match with pagetable entry 
  Sa = portc->pgtable_index * 4096;
  Ladr = Sa + count;
  Ca = Sa + 0x100;
  if (dwSamplingRate == 48000)
    Ctl = 0x44c;		// Set the Pitch Master for stereo.
  else if ((dwSamplingRate == 96000))
    Ctl = 0x45c;		// Set the Pitch Master for stereo.

  Ctl |= (portc->channels-1)*SRCCTL_ILSZ; /* Number of interleaved channels to follow */

  // Program SRC for channel 1, enable interrupts and interleaved channels
  WriteSRC (devc, Ca, 0, Sa, Ladr, 0x100, Ctl, srcch);

  Ladr = Sa + count;
  Ca = Sa + 0x100;

  for (i=1;i<portc->channels;i++)
  {
	  if (dwSamplingRate == 48000)
	    Ctl = 0x4c;			// slave
	  else if ((dwSamplingRate == 96000))
	    Ctl = 0x5c;			// slave
  	  Ctl |= (portc->channels-i-1)*SRCCTL_ILSZ;
	
	  // Program SRC for channel 2
	  WriteSRC (devc, Ca, 0, Sa, Ladr, 0x100, Ctl, srcch+i);
  }

  //_dumpRegisters (devc, portc);
}

void
StopPlay (sbxfi_devc_t * devc, sbxfi_portc_t * portc)
{
  unsigned int srcch;
  unsigned int dwData;
  int i;

  srcch = portc->SrcChan;

  //WriteSRC(devc, 0, 0, 0, 0, 0, 0, srcch);
  //WriteSRC(devc, 0, 0, 0, 0, 0, 0, srcch2);

  dwData = HwRead20K1 (devc, SRCCTL(srcch));
  dwData &= 0xfffffff0;
  dwData |= 0;
  dwData &= ~SRCCTL_IE; /* Interrupt disable */
  HwWrite20K1 (devc, SRCCTL(srcch), dwData);

  for (i=1;i<portc->channels;i++)
  {
	  dwData = HwRead20K1 (devc, SRCCTL(srcch+i));
	  dwData &= 0xfffffff0;
	  dwData |= 0;
	  HwWrite20K1 (devc, SRCCTL(srcch+i), dwData);
  }
}


void
StopPlaySRC (sbxfi_devc_t * devc, sbxfi_portc_t * portc)
{
#ifndef INTERNAL_LOOPBACK
  StopPlay (devc, portc);
#endif
}


//======================== RECORD ==========================



void
SetupRecordInputMapper (sbxfi_devc_t * devc, sbxfi_portc_t * portc)
{
  unsigned int srcch, srcch2;


  srcch = portc->SrcChan;
  srcch2 = portc->SrcChan+1;

  // Internal loopback means loop play channels to record
#ifdef INTERNAL_LOOPBACK
  {
    unsigned int playch1, playch2;

    playch1 = portc->dwPlayLSrcChan;
    playch2 = portc->dwPlayRSrcChan;
    if (srcch == 0)
      {
	HwWrite20K1 (devc, SRCIMAP(0), 0);
	HwWrite20K1 (devc, SRCIMAP(srcch2),
		     srcch2 << 24 | (0x80 +
				     srcch) << 16 |
		     GetAudioSrcChan (playch2));
	HwWrite20K1 (devc, SRCIMAP(0x80 + srcch),
		     (0x80 + srcch) << 24 | (srcch2 +
					      0x80) << 16 |
		     GetAudioSrcChan (playch1 + 0x80));
	HwWrite20K1 (devc, SRCIMAP(0x81 + srcch2),
		     (0x80 +
		      srcch2) << 24 | 0 << 16 | GetAudioSrcChan (playch2 +
								 0x80));
	HwWrite20K1 (devc, SRCIMAP(srcch),
		     srcch << 24 | srcch2 << 16 | GetAudioSrcChan (playch1));
      }
    else
      {
	HwWrite20K1 (devc, SRCIMAP(0), 0);
	HwWrite20K1 (devc, SRCIMAP(srcch),
		     srcch << 24 | srcch2 << 16 | GetAudioSrcChan (playch1));
	HwWrite20K1 (devc, SRCIMAP(srcch2),
		     srcch2 << 24 | (0x80 +
				     srcch) << 16 |
		     GetAudioSrcChan (playch2));
	HwWrite20K1 (devc, SRCIMAP(0x80 + srcch),
		     (0x80 + srcch) << 24 | (srcch2 +
					      0x80) << 16 |
		     GetAudioSrcChan (playch1 + 0x80));
	HwWrite20K1 (devc, SRCIMAP(0x80 + srcch2),
		     (0x80 +
		      srcch2) << 24 | 0 << 16 | GetAudioSrcChan (playch2 +
								 0x80));
	HwWrite20K1 (devc, SRCIMAP(0),
		     (0 << 24) | (srcch << 16) | 0x0);
      }
  }
#else
  {
    if (srcch == 0)
      {
	HwWrite20K1 (devc, SRCIMAP(0), 0);
	HwWrite20K1 (devc, SRCIMAP(srcch2),
		     srcch2 << 24 | (0x80 +
				     srcch) << 16 |
		     GetAudioSumChan (srcch2));
	HwWrite20K1 (devc, SRCIMAP(0x80 + srcch),
		     (0x80 + srcch) << 24 | (srcch2 +
					      0x80) << 16 |
		     GetAudioSumChan (srcch + 0x80));
	HwWrite20K1 (devc, SRCIMAP(0x81 + srcch2),
		     (0x80 +
		      srcch2) << 24 | 0 << 16 | GetAudioSumChan (srcch2 +
								 0x80));
	HwWrite20K1 (devc, SRCIMAP(srcch),
		     srcch << 24 | srcch2 << 16 | GetAudioSumChan (srcch));
      }
    else
      {
	HwWrite20K1 (devc, SRCIMAP(0), 0);
	HwWrite20K1 (devc, SRCIMAP(srcch),
		     srcch << 24 | srcch2 << 16 | GetAudioSumChan (srcch));
	HwWrite20K1 (devc, SRCIMAP(srcch2),
		     srcch2 << 24 | (0x80 +
				     srcch) << 16 |
		     GetAudioSumChan (srcch2));
	HwWrite20K1 (devc, SRCIMAP(0x80 + srcch),
		     (0x80 + srcch) << 24 | (srcch2 +
					      0x80) << 16 |
		     GetAudioSumChan (srcch + 0x80));
	HwWrite20K1 (devc, SRCIMAP(0x80 + srcch2),
		     (0x80 +
		      srcch2) << 24 | 0 << 16 | GetAudioSumChan (srcch2 +
								 0x80));
	HwWrite20K1 (devc, SRCIMAP(0),
		     (0 << 24) | (srcch << 16) | 0x0);
      }
  }
#endif
}


void
_SetupInputToOutputMonitoring (sbxfi_devc_t * devc, sbxfi_portc_t * portc)
{
  unsigned int i;
  unsigned int dio1, dio2;
  unsigned int srcch, srcch2;


  srcch = portc->SrcChan;
  srcch2 = portc->SrcChan+1;

  dio1 = portc->dwDAChan[0];
  dio2 = portc->dwDAChan[1];

  // initialize input mappers
  for (i = 0; i < 0x50; i++)
    HwWrite20K1 (devc, DAOIMAP_START(i), 0);

  HwWrite20K1 (devc, DAOIMAP_START(dio1), 0);
  HwWrite20K1 (devc, DAOIMAP_START(dio2),
	       ((dio1 + 2) << 16) | GetAudioSumChan (srcch2));
  HwWrite20K1 (devc, DAOIMAP_START(dio1 + 2),
	       ((dio2 + 2) << 16) | GetAudioSumChan (srcch + 0x80));
  HwWrite20K1 (devc, DAOIMAP_START(dio2 + 2),
	       (dio1 << 16) | GetAudioSumChan (srcch2 + 0x80));
  HwWrite20K1 (devc, DAOIMAP_START(dio1),
	       (dio2 << 16) | GetAudioSumChan (srcch));
}

void
SetupRecordMixer (sbxfi_devc_t * devc, sbxfi_portc_t * portc)
{
  unsigned int fixed_pitch;
  unsigned int srcPrchn1, srcPrchnC1;
  unsigned int srcch, srcch2, srcchnC1, srcchnC2;
  unsigned int dwYData;
  unsigned short i, inch1, inch2;


  srcch = portc->SrcChan;
  srcch2 = portc->SrcChan+1;

  // NOTE: Y-Data is a 14-bit immediate floating-point constant multiplier.
  // Adjust the Y-Data to control the multiplier.
  // This can be used to control the level of the signal.
  dwYData = 0x1c00;

  srcchnC1 = srcch + 0x80;
  srcchnC2 = srcch2 + 0x80;

  srcPrchn1 = GetParamPitchChan (srcch);
  srcPrchnC1 = GetParamPitchChan (srcch + 0x80);

  // since input is 2x of output, pitch is 2.0
  // convert to fixed-point 8.24 format, shift left 24 bit.
  fixed_pitch = 2;
  fixed_pitch = fixed_pitch << 24;

  // write the pitch to param ring of the corresponsing SRC pitch slot
  HwWrite20K1 (devc, PRING_LO_HI_START(srcPrchn1), fixed_pitch);
  HwWrite20K1 (devc, PRING_LO_HI_START(srcPrchnC1), fixed_pitch);

  inch1 = 0x1b5;		// I2S-In3 L
  inch2 = 0x1bd;		// I2S-In3 R
  // program all I2S-In3 slots
  for (i = 0; i < 8; i++)
    {
      if (i <= 3)
	{
	  WriteAMOP (devc, inch1 + (i * 0x200), dwYData, inch1 + (i * 0x200),
		     (0x80000000 + srcch));
	  WriteAMOP (devc, inch2 + (i * 0x200), dwYData, inch2 + (i * 0x200),
		     (0x80000000 + srcch2));
	}
      else
	{
	  WriteAMOP (devc, inch1 + (i * 0x200), dwYData, inch1 + (i * 0x200),
		     (0x80000000 + srcchnC1));
	  WriteAMOP (devc, inch2 + (i * 0x200), dwYData, inch2 + (i * 0x200),
		     (0x80000000 + srcchnC2));
	}
    }

  // enable physical input I2S_in3 to I2S-Out0 monitoring
  _SetupInputToOutputMonitoring (devc, portc);
}

void
SetupRecordFormat (sbxfi_devc_t * devc)
{
  unsigned int i2sorg;

  i2sorg = HwRead20K1 (devc, I2SCTL);

  // enable I2S-D input
  i2sorg |= 0x90000000;
  HwWrite20K1 (devc, I2SCTL, i2sorg);
}

void
SetupAndStartRecordSRC (sbxfi_devc_t * devc, sbxfi_portc_t * portc)
{
  unsigned int Sa, Ladr, Ca, Ctl = 0x64d;
  int count;
  unsigned int srcch, srcch2;
  unsigned int dwSamplingRate;


  srcch = portc->SrcChan;
  srcch2 = portc->SrcChan+1;
  dwSamplingRate = portc->rate;

  count = audio_engines[portc->dev]->dmap_in->bytes_in_use;

  // convert the num samples to bytes count

  // hardcoded values:  
  //  start addx: 4th entry in page table.
  Sa = portc->pgtable_index * 4096;
  Ladr = Sa + count;
  Ca = Sa + 0x80;
  if (dwSamplingRate == 48000)
    Ctl = 0x64d;		// record must start with RUN state!.
  else if ((dwSamplingRate == 96000))
    Ctl = 0x65d;

  Ctl |= SRCCTL_ILSZ;	// Interleaved stereo

  WriteSRC (devc, Ca, 0, Sa, Ladr, 0x100, Ctl, srcch);

  Ladr = Sa + count;
  Ca = Sa + 0x80;
  if (dwSamplingRate == 48000)
    Ctl = 0x24d;
  else if ((dwSamplingRate == 96000))
    Ctl = 0x25d;

  WriteSRC (devc, Ca, 0, Sa, Ladr, 0x80, Ctl, srcch2);

  // Enable SRC input from Audio Ring
  HwWrite20K1 (devc, SRCMCTL, 0x1);

//    _dumpRegisters(devc);
}

void
StopRecordSRC (sbxfi_devc_t * devc, sbxfi_portc_t * portc)
{
  unsigned int srcch, srcch2;
  unsigned int dwData;
  unsigned int i;

  srcch = portc->SrcChan;
  srcch2 = portc->SrcChan+1;

  //WriteSRC(devc, 0, 0, 0, 0, 0, 0, srcch);
  //WriteSRC(devc, 0, 0, 0, 0, 0, 0, srcch2);

  dwData = HwRead20K1 (devc, SRCCTL(srcch));
  dwData &= 0xfffffff0;
  dwData |= 0;
  HwWrite20K1 (devc, SRCCTL(srcch), dwData);

  dwData = HwRead20K1 (devc, SRCCTL(srcch2));
  dwData &= 0xfffffff0;
  dwData |= 0;
  HwWrite20K1 (devc, SRCCTL(srcch2), dwData);

#ifdef INTERNAL_LOOPBACK
  StopPlay (devc, portc);
#endif

  // Disable SRC inputs from Audio Ring
  HwWrite20K1 (devc, SRCMCTL, 0x0);

  for (i = 0; i < 0x50; i++)
    HwWrite20K1 (devc, DAOIMAP_START(i), 0);
}

//========================

unsigned int
HwRead20K1PCI (sbxfi_devc_t * devc, unsigned int dwReg)
{
  unsigned int dwVal;
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  osOutportd (devc, (IOADDR) devc->wIOPortBase + 0x10, dwReg);
  dwVal = osInportd (devc, (IOADDR) (devc->wIOPortBase + 0x14));
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);

  return dwVal;
}

unsigned int
HwRead20K1 (sbxfi_devc_t * devc, unsigned int dwReg)
{
  unsigned int dwVal;

  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  osOutportd (devc, (IOADDR) devc->wIOPortBase + 0x0, dwReg);
  dwVal = osInportd (devc, (IOADDR) (devc->wIOPortBase + 0x4));
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);

  return dwVal;
}

void
HwWrite20K1PCI (sbxfi_devc_t * devc, unsigned int dwReg, unsigned int dwData)
{
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  osOutportd (devc, (IOADDR) devc->wIOPortBase + 0x10, dwReg);
  osOutportd (devc, (IOADDR) (devc->wIOPortBase + 0x14), dwData);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
}

void
HwWrite20K1 (sbxfi_devc_t * devc, unsigned int dwReg, unsigned int dwData)
{
  oss_native_word flags;

  MUTEX_ENTER_IRQDISABLE (devc->low_mutex, flags);
  osOutportd (devc, (IOADDR) devc->wIOPortBase + 0x0, dwReg);
  osOutportd (devc, (IOADDR) (devc->wIOPortBase + 0x4), dwData);
  MUTEX_EXIT_IRQRESTORE (devc->low_mutex, flags);
}

void
WriteSRC
  (sbxfi_devc_t * devc,
   unsigned int srcca,
   unsigned int srccf,
   unsigned int srcsa, unsigned int srcla, unsigned int srcccr, unsigned int srcctl, unsigned int chn)
{
  HwWrite20K1 (devc, SRCCA(chn), srcca);	// Current Address
  HwWrite20K1 (devc, SRCCF(chn), srccf);	// Current Fraction
  HwWrite20K1 (devc, SRCSA(chn), srcsa);	// START address
  HwWrite20K1 (devc, SRCLA(chn), srcla);	// LOOP address
  HwWrite20K1 (devc, SRCCCR(chn), srcccr);	// Cache control
  HwWrite20K1 (devc, SRCCTL(chn), srcctl);	// SRCCTL
}

#define CRM_TIMESLOT_ALLOC_BLOCK_SIZE   16
#define CRM_PTS_PITCH                   6
#define CRM_PARAM_SRC_OFFSET            0x60

unsigned int
GetParamPitchChan (unsigned int i)
{
  int interpChanID =
    (((int) i * CRM_TIMESLOT_ALLOC_BLOCK_SIZE) + CRM_PTS_PITCH) -
    CRM_PARAM_SRC_OFFSET;
  if (interpChanID < 0)
    {
      interpChanID += 4096;
    }
  return (unsigned int) interpChanID;
}

unsigned int
GetAudioSrcChan (unsigned int srcchn)
{
  //  SRC channel is in Audio Ring slot 1, after every 16 slot.
  return (unsigned int) ((srcchn << 4) + 0x1);
}

unsigned int
GetAudioSumChan (unsigned int chn)
{
  //  SUM channel is in Audio Ring slot 0xc, after every 16 slot.
  return (unsigned int) ((chn << 4) + 0xc);
}

void
WriteAMOP
  (sbxfi_devc_t * devc,
   unsigned int xdata, unsigned int ydata, unsigned int chn, unsigned int hidata)
{
  HwWrite20K1 (devc, AMOP_START(chn), ((((unsigned int) ydata) << 18) | xdata << 4 | 1));	// Audio mixer, y-immediate
  HwWrite20K1 (devc, AMOP_START(chn) + 4, hidata);	//  Audio mixer.
}

void
_dumpRegisters (sbxfi_devc_t * devc, sbxfi_portc_t * portc)
{
  _dumpGlobal (devc);
  _dumpSRCs (devc, portc);
}

void
_dumpSRCs (sbxfi_devc_t * devc, sbxfi_portc_t * portc)
{
  unsigned int chn;

  chn = portc->SrcChan;
  cmn_err (CE_CONT,
	   "SRC chn=%lx, CA=%lx, CF=%lx, SA=%lx, LA=%lx, CCR=%lx, CTL=%lx\n",
	   chn, HwRead20K1 (devc, SRCCA(chn)),
	   HwRead20K1 (devc, SRCCF(chn)), HwRead20K1 (devc,
									 SRCSA(chn)),
	   HwRead20K1 (devc, SRCLA(chn)), HwRead20K1 (devc,
									 SRCCCR(chn)),
	   HwRead20K1 (devc, SRCCTL(chn)));

  chn = portc->SrcChan+1;
  cmn_err (CE_CONT,
	   "SRC chn=%lx, CA=%lx, CF=%lx, SA=%lx, LA=%lx, CCR=%lx, CTL=%lx\n",
	   chn, HwRead20K1 (devc, SRCCA(chn)),
	   HwRead20K1 (devc, SRCCF(chn)), HwRead20K1 (devc,
									 SRCSA(chn)),
	   HwRead20K1 (devc, SRCLA(chn)), HwRead20K1 (devc,
									 SRCCCR(chn)),
	   HwRead20K1 (devc, SRCCTL(chn)));
}


void
_dumpGlobal (sbxfi_devc_t * devc)
{
  unsigned int i;

  cmn_err (CE_CONT,
	   "GCTL=%lx, PLLCTL=%lx, GPIOCTL=%lx, GPIO=%lx, I2SCTL=%lx\n",
	   HwRead20K1 (devc, GCTL), HwRead20K1 (devc, PLLCTL),
	   HwRead20K1 (devc, GPIOCTL), HwRead20K1 (devc, GPIO),
	   HwRead20K1 (devc, I2SCTL));
#if 1
  cmn_err (CE_CONT, "DAOIMAP....\n");
  for (i = 0; i < 0x50; i++)
    {
      cmn_err (CE_CONT, "%02lx: %lx  ", i,
	       HwRead20K1 (devc, DAOIMAP_START(i)));
      if (((i + 1) % 8) == 0)
	cmn_err (CE_CONT, "\n");
    }
#endif
#if 0
  cmn_err (CE_CONT, "PageTable PhysAddx=%lx\n", HwRead20K1 (devc, PTPALX));
  for (i = 0; i < 10; i++)
    {
      cmn_err (CE_CONT, "Entry[%lx]=%lx\n", i, devc->pdwPageTable[i]);
    }
#endif
}
