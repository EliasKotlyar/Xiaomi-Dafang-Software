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

#ifndef _HWACCESS_H_
#define _HWACCESS_H_

unsigned int GetAudioSrcChan (unsigned int srcchn);
unsigned int GetAudioSumChan (unsigned int chn);
unsigned int GetParamPitchChan (unsigned int i);
void WriteAMOP (sbxfi_devc_t * devc, unsigned int xdata, unsigned int ydata,
		unsigned int chn, unsigned int hidata);
void WriteSRC (sbxfi_devc_t * devc, unsigned int srcca, unsigned int srccf,
	       unsigned int srcsa, unsigned int srcla, unsigned int srcccr, unsigned int srcctl,
	       unsigned int chn);
unsigned int HwRead20K1PCI (sbxfi_devc_t * devc, unsigned int dwReg);
unsigned int HwRead20K1 (sbxfi_devc_t * devc, unsigned int dwReg);
void HwWrite20K1 (sbxfi_devc_t * devc, unsigned int dwReg, unsigned int dwData);
void HwWrite20K1PCI (sbxfi_devc_t * devc, unsigned int dwReg, unsigned int dwData);
unsigned int ReadCfgDword (unsigned int dwBusNum, unsigned int dwDevNum, unsigned int dwFuncNum,
		      unsigned int dwReg);
unsigned short ReadCfgWord (unsigned int dwBusNum, unsigned int dwDevNum, unsigned int dwFuncNum,
		    unsigned int dwReg);
void WriteConfigDword (unsigned int dwBusNum, unsigned int dwDevNum, unsigned int dwFuncNum,
		       unsigned int dwReg, unsigned int dwData);

unsigned char DetectAndConfigureHardware (sbxfi_devc_t * devc);
unsigned char IsVistaCompatibleHardware (sbxfi_devc_t * devc);
void SwitchToXFiCore (sbxfi_devc_t * devc);
CTSTATUS InitHardware (sbxfi_devc_t * devc);
CTSTATUS AllocateBuffers (sbxfi_devc_t * devc, sbxfi_portc_t * portc);
void FreeBuffers (sbxfi_devc_t * devc, sbxfi_portc_t * portc);
void SetupHardwarePageTable (sbxfi_devc_t * devc, sbxfi_portc_t * portc);
void InitADC (sbxfi_devc_t * devc, unsigned int src, unsigned char mic20db);
void ResetDAC (sbxfi_devc_t * devc);
void InitDAC (sbxfi_devc_t * devc, sbxfi_portc_t * portc);

void SetupPlayMixer (sbxfi_devc_t * devc, sbxfi_portc_t * portc);
void UpdatePlayMixer (sbxfi_devc_t * devc, sbxfi_portc_t * portc);
void SetupPlayFormat (sbxfi_devc_t * devc, sbxfi_portc_t * portc);
void SetupAndStartPlaySRC (sbxfi_devc_t * devc, sbxfi_portc_t * portc);
void StopPlaySRC (sbxfi_devc_t * devc, sbxfi_portc_t * portc);
void SetupPlayInputMapper (sbxfi_devc_t * devc, sbxfi_portc_t * portc);
void StopPlay (sbxfi_devc_t * devc, sbxfi_portc_t * portc);

void SetupRecordMixer (sbxfi_devc_t * devc, sbxfi_portc_t * portc);
void SetupRecordFormat (sbxfi_devc_t * devc);
void SetupAndStartRecordSRC (sbxfi_devc_t * devc, sbxfi_portc_t * portc);
void StopRecordSRC (sbxfi_devc_t * devc, sbxfi_portc_t * portc);
void SetupRecordInputMapper (sbxfi_devc_t * devc, sbxfi_portc_t * portc);
void SetupInputToOutputMonitoring (sbxfi_devc_t * devc,
				    sbxfi_portc_t * portc);

void _dumpRegisters (sbxfi_devc_t * devc, sbxfi_portc_t * portc);
void _dumpSRCs (sbxfi_devc_t * devc, sbxfi_portc_t * portc);
void _dumpGlobal (sbxfi_devc_t * devc);

#define osDelayms(usecs) oss_udelay(usecs)
#define osInportd(devc, ioaddr) INL(devc->osdev, ioaddr)
#define osOutportd(devc, ioaddr, data) OUTL(devc->osdev, data, ioaddr)

#endif
