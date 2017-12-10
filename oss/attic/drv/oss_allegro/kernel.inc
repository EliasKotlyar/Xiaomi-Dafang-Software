/******************************************************************************
 *                                                                            *
 *                       (C) 1997-1999 ESS Technology, Inc.                   *
 *                                                                            *
 * This source code, its compiled object code, and its associated data sets   *
 * are copyright (C) 1997-1999 ESS Technology, Inc. 			      *
 *                                                                            *
 ******************************************************************************/

/*---------------------------------------------------------------------------
 *              Copyright (C) 1997-1999, ESS Technology, Inc.
 *---------------------------------------------------------------------------
 * FILENAME: kernel.c V2.10  08/19/99
 *---------------------------------------------------------------------------
 * DESCRIPTION: Maestro 3/Allegro1 host kernel
 *---------------------------------------------------------------------------
 * AUTHOR:  Henry Tang/Hong Kim/Alger Yeung/Don Kim
 *---------------------------------------------------------------------------
 * HISTORY:
 *    09/03/97  HT  Created.
 *    04/23/99  AY  Use C call for lowest level access (in and out)
 *    04/23/99  AY  Remove delay since M3/Allegro1 has no delay bug
 *    04/23/99  AY  Cleanup dead code
 *    05/08/99  AY  SPDIF support SCMS
 *    05/21/99  AY  Add SwitchClient API to add/remove client from/to task list
 *    05/24/99  AY  Enable Cpythru (dwclient=0) to be loaded even though
 *                  its code memory is not available at certain location
 *    06/02/99  AY  Enable PassThru support for adc1 -> mixer & adc2->mixer
 *    07/29/99  AY  Adding 4-speaker support
 *    08/18/99  AY  Adding SPDIF IN  support
 *    08/18/99  AY  Remove PIO and SoundBlaster support
 *    08/18/99  AY  Reduce Cpythru to 2 instances instead of 4
 *    09/22/99  HK  Add M3I Features
 *---------------------------------------------------------------------------
 */

#define NON_INTEL  1		/* avoid Intel x86 inline assembly instruction */
/*#define ALLEGRO_DEBUG 1      // KERNEL debug flag */
/*#define K_DBG 1      // KERNEL debug flag */

#ifdef NT_MODEL
#define  HEAPZEROINIT   0
#ifdef DON
#include "../port.h"
#include "kernel.h"
#endif
#endif

#ifdef NON_INTEL
/*#error ******** No Intel x86 assembly instructions ************* */
#endif

PCLIENT_BIN kBinStructAddress (PHWI phwi, DWORD dwClient, DWORD dwSearchKey);

/* */
/* Client info */
/* */

HWI ghwi = {

  0,
  0,
  0,
  0,

  0,

  0, 0,

  0,

  /* client table */

  {
   {
    gasCpyThruVectCode,
    0,
    MAX_INSTANCE_CPYTHRU,
    KDATA_INSTANCE0_CPYTHRU,
    0,
    0,
    0}
   ,

   {
    gasModemVectCode,
    0,
    MAX_INSTANCE_MODEM,
    KDATA_INSTANCE0_MODEM,
    0,
    0,
    0}
   ,

   {
    gasPos3DVectCode,
    0,
    MAX_INSTANCE_POS3D,
    KDATA_INSTANCE0_POS3D,
    0,
    0,
    0}
   ,

   {
    gasSpkVirtVectCode,
    0,
    MAX_INSTANCE_SPKVIRT,
    KDATA_INSTANCE0_SPKVIRT,
    0,
    0,
    0}
   ,

   {
    gasSpkVirtVectCode_CRL,
    0,
    MAX_INSTANCE_SPKVIRT,
    KDATA_INSTANCE0_SPKVIRT,
    0,
    0,
    0}
   ,

   {
    gasSRCVectCode,
    0,
    MAX_INSTANCE_SRC,
    KDATA_INSTANCE0_SRC,
    0,
    0,
    0}
   ,

   {
    gasMINISRCVectCode,
    0,
    MAX_INSTANCE_MINISRC,
    KDATA_INSTANCE0_MINISRC,
    0,
    0,
    0}
   ,

   {
    gasSPDIFVectCode,
    0,
    MAX_INSTANCE_SPDIF,
    KDATA_INSTANCE0_SPDIF,
    0,
    0,
    0}

   }
  ,
#if 0
#if (F_FREE || (F_END != -1))
#error Assumption about storage flags failed.
#endif
#endif
  /* task resource list */

  {
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,

   (WORD) F_END}
  ,

  /* Copy Through resource list */

  {
   F_FREE, F_FREE,		/* AY reduce to 2 for SPDIF IN */

   (WORD) F_END}
  ,

  /* Modem resource list */

  {
   F_FREE,

   (WORD) F_END}
  ,

  /* Positional 3D resource list */

  {
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE,

   (WORD) F_END}
  ,

  /* Speaker Virtualization resource list */

  {
   F_FREE,

   (WORD) F_END}
  ,

  /* Sample Rate Conversion resource list */

  {
   F_FREE, F_FREE,

   (WORD) F_END}
  ,

  /* MINI Sample Rate Conversion resource list */

  {
   F_FREE, F_FREE,
   F_FREE, F_FREE,

   (WORD) F_END}
  ,

  /* SPDIF resource list */

  {
   F_FREE,

   (WORD) F_END}
  ,

  /* DMA resource list */

  {
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE,

   (WORD) F_END}
  ,

  /* ADC1 resource list */

  {
   F_FREE,

   (WORD) F_END}
  ,

  /* ADC2 resource list */

  {
   F_FREE,

   (WORD) F_END}
  ,

  /* CD resource list */
  {
   F_FREE,

   (WORD) F_END}
  ,

  /* MIC resource list */

  {
   F_FREE,

   (WORD) F_END}
  ,

  /* I2S resource list */

  {
   F_FREE,

   (WORD) F_END}
  ,

  /* CHI resource list */

  {
   F_FREE,

   (WORD) F_END}
  ,


  /* SPDIF IN resource list */

  {
   F_FREE,

   (WORD) F_END}
  ,

  /* MIXER resource list */
  {
   F_FREE, F_FREE,
   F_FREE, F_FREE,
   F_FREE, F_FREE,
   F_FREE, F_FREE,
   F_FREE, F_FREE,

   (WORD) F_END}
  ,

  /*AY */
  /* FMIXER resource list */
  {
   F_FREE,

   (WORD) F_END}
  ,

  /* RMIXER resource list */
  {
   F_FREE,

   (WORD) F_END}
  ,

  /* DSP code memory map */

  0, 0, 0, 0,

  0, 0, 0, 0,

  {
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,

   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,

   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,

   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,

   (BYTE) F_END}
  ,

  /* DSP data memory map */

  {
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,

   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,

   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,

   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,

   (BYTE) F_END}
  ,

  /* DSP vector list */

  {
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,

   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,

   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE,
   F_FREE, F_FREE, F_FREE, F_FREE}

};

/* */
/* Memory map images */
/* */
#if 0
#if (NUM_UNITS_KERNEL_CODE != 16)
#error Assumption about kernel code size failed.
#endif
#endif

BYTE gabRevBCodeMemoryMapImage[] = {
  F_USED, F_USED, F_USED, F_USED,	/* 0000 - 03FF */
  F_USED, F_USED, F_USED, F_USED,
  F_USED, F_USED, F_USED, F_USED,
  F_USED, F_USED, F_USED, F_USED,

  F_FREE, F_FREE, F_FREE, F_FREE,	/* 0400 - 07FF */
  F_FREE, F_FREE, F_FREE, F_FREE,
  F_FREE, F_FREE, F_FREE, F_FREE,
  F_FREE, F_FREE, F_FREE, F_FREE,

  F_FREE, F_FREE, F_FREE, F_FREE,	/* 0800 - 0BFF */
  F_FREE, F_FREE, F_FREE, F_FREE,
  F_FREE, F_FREE, F_FREE, F_FREE,
  F_FREE, F_FREE, F_FREE, F_FREE,

  (BYTE) F_END
};

#if 0
#if (NUM_UNITS_KERNEL_DATA != 2)
#error Assumption about kernel data size failed.
#endif
#endif

#if 0
#if (KDATA_BASE_ADDR != 0x1000)
#error Assumption about kernel data memory location failed.
#endif
#endif

BYTE gabRevBDataMemoryMapImage[] = {
  F_USED, F_USED, F_FREE, F_FREE,	/* 1000 - 17FF */
  F_FREE, F_FREE, F_FREE, F_FREE,
  F_FREE, F_FREE, F_FREE, F_FREE,
  F_FREE, F_FREE, F_FREE, F_FREE,

  F_FREE, F_FREE, F_FREE, F_FREE,	/* 1800 - 1BFF */
  F_FREE, F_FREE, F_FREE, F_FREE,

  F_USED, F_USED, F_USED, F_USED,	/* 1C00 - 1FFF */
  F_USED, F_USED, F_USED, F_USED,

  F_USED, F_USED, F_USED, F_USED,	/* 2000 - 27FF */
  F_USED, F_USED, F_USED, F_USED,
  F_USED, F_USED, F_USED, F_USED,
  F_USED, F_USED, F_USED, F_USED,

  F_USED, F_USED, F_USED, F_USED,	/* 2800 - 2BFF */
  F_USED, F_USED, F_USED, F_USED,

  (BYTE) F_END
};


extern WORD MIXER_TASK_NUMBER;

#ifdef VXD_MODEL
#pragma VxD_LOCKED_CODE_SEG
#endif

#if defined( DOS_MODEL ) || defined( NT_MODEL )
/*-------------------------------------------------------------------------- */
/* */
/*  PBYTE _HeapAllocate */
/* */
/*  Description: */
/*      Allocate memory block of specified length. */
/* */
/*  Parameters: */
/*      DWORD dwLength */
/*         Length in bytes of memory block. */
/* */
/*      DWORD dwFlags */
/*         Allocation flags. */
/* */
/*  Return (PBYTE): */
/*      Pointer to allocated memory block. */
/* */
/*-------------------------------------------------------------------------- */

/*ARGSUSED*/
PBYTE
_HeapAllocate (DWORD dwLength, DWORD dwFlags)
{
#ifdef DOS_MODEL
  return (PBYTE) malloc (dwLength);
#endif

#ifdef NT_MODEL
  return (PBYTE) KERNEL_MALLOC (dwLength);
#endif

}				/* _HeapAllocate() */

/*-------------------------------------------------------------------------- */
/* */
/*  DWORD _HeapFree */
/* */
/*  Description: */
/*      Deallocate previously allocated memory block. */
/* */
/*  Parameters: */
/*      PVOID pvBlock, */
/*         Pointer to previously allocated memory block. */
/* */
/*      DWORD dwFlags */
/*         Allocation flags. */
/* */
/*  Return (DWORD): */
/*      Non-zero if successful, zero otherwise. */
/* */
/*-------------------------------------------------------------------------- */

/*ARGSUSED*/
DWORD
_HeapFree (PVOID pvBlock, DWORD dwFlags)
{
#ifdef DOS_MODEL
  free (pvBlock);
#endif

#ifdef NT_MODEL
  KERNEL_FREE (pvBlock);
#endif

  return 1;

}				/* _HeapFree() */
#endif

/*-------------------------------------------------------------------------- */
/* */
/*  VOID kDelayNMicroseconds */
/* */
/*  Description: */
/*      Delay specified number of microseconds. */
/* */
/*  Parameters: */
/*      DWORD dwCount */
/*         Number of microseconds to delay. */
/* */
/*  Return (VOID): */
/* */
/*-------------------------------------------------------------------------- */

VOID
kDelayNMicroseconds (DWORD dwCount)
{
  oss_udelay (dwCount);
}				/* kDelayNMicroseconds() */

/*-------------------------------------------------------------------------- */
/* */
/*  BYTE kInB */
/* */
/*  Description: */
/*      Do BYTE I/O read. */
/* */
/*  Parameters: */
/*      DWORD dwPort */
/*         I/O port address. */
/* */
/*  Return (BYTE): */
/*      Data read from specified address. */
/* */
/*-------------------------------------------------------------------------- */

/*ARGSUSED*/
BYTE
kInB (allegro_devc * devc, DWORD dwPort)
{
  BYTE bData;
  bData = READ_PORT_UCHAR (devc->osdev, dwPort);
  return bData;
}				/* kInB() */

/*-------------------------------------------------------------------------- */
/* */
/*  VOID kOutB */
/* */
/*  Description: */
/*      Do BYTE I/O write. */
/* */
/*  Parameters: */
/*      DWORD dwPort */
/*         I/O port address. */
/* */
/*      BYTE bData */
/*         I/O port data. */
/* */
/*  Return (VOID): */
/* */
/*-------------------------------------------------------------------------- */

/*ARGSUSED*/
VOID
kOutB (allegro_devc * devc, DWORD dwPort, BYTE bData)
{
  WRITE_PORT_UCHAR (devc->osdev, dwPort, bData);
}				/* kOutB() */

/*-------------------------------------------------------------------------- */
/* */
/*  WORD kInW */
/* */
/*  Description: */
/*      Do WORD I/O read. */
/* */
/*  Parameters: */
/*      DWORD dwPort */
/*         I/O port address. */
/* */
/*  Return (WORD): */
/*      Data read from specified address. */
/* */
/*-------------------------------------------------------------------------- */

/*ARGSUSED*/
WORD
kInW (allegro_devc * devc, DWORD dwPort)
{
  WORD wData;
  wData = READ_PORT_USHORT (devc->osdev, dwPort);
  return wData;
}				/* kInW() */

/*-------------------------------------------------------------------------- */
/* */
/*  VOID kOutW */
/* */
/*  Description: */
/*      Do WORD I/O write. */
/* */
/*  Parameters: */
/*      DWORD dwPort */
/*         I/O port address. */
/* */
/*      WORD wData */
/*         I/O port data. */
/* */
/*  Return (VOID): */
/* */
/*-------------------------------------------------------------------------- */

/*ARGSUSED*/
VOID
kOutW (allegro_devc * devc, DWORD dwPort, WORD wData)
{
  WRITE_PORT_USHORT (devc->osdev, dwPort, wData);
}				/* kOutW() */

/*-------------------------------------------------------------------------- */
/* */
/*  VOID kInsW */
/* */
/*  Description: */
/*      Do WORD I/O repeat read. */
/* */
/*  Parameters: */
/*      DWORD dwPort */
/*         I/O port address. */
/* */
/*      DWORD dwLen */
/*         Number of WORDs to read from specified address. */
/* */
/*      DWORD dwAddr */
/*         Host memory address to write to. */
/* */
/*  Return (VOID): */
/* */
/*-------------------------------------------------------------------------- */

/*ARGSUSED*/
VOID
kInsW (allegro_devc * devc, DWORD dwPort, DWORD dwLen, DWORD dwAddr)
{
#ifdef NON_INTEL
  DWORD i;
  WORD *wptr;
#endif

  if (!dwLen)
    return;

#ifdef DOS_MODEL

#ifdef NON_INTEL
  wptr = (WORD *) dwAddr;
  for (i = 0; i < dwLen; ++i)
    {
      *wptr++ = inpw (devc, (WORD) dwPort);
    }				/* endfor */
#else
  _asm
  {
  push di
      push es
      cld
      mov cx, word ptr dwLen
      mov dx, word ptr dwPort les di, dwAddr rep insw pop es pop di}
#endif

#endif

#if defined( VXD_MODEL ) || defined( WDM_MODEL ) || defined( NT_MODEL )

#ifdef NON_INTEL
  wptr = (WORD *) dwAddr;
  for (i = 0; i < dwLen; ++i)
    {
      *wptr++ = inpw (devc->osdev, (WORD) dwPort);
    }				/* endfor */
#else
  _asm
  {
  push edi
      cld mov ecx, dwLen mov edx, dwPort mov edi, dwAddr rep insw pop edi}
#endif

#endif

}				/* kInsW() */

/*-------------------------------------------------------------------------- */
/* */
/*  VOID kOutsW */
/* */
/*  Description: */
/*      Do WORD I/O repeat write. */
/* */
/*  Parameters: */
/*      DWORD dwPort */
/*         I/O port address. */
/* */
/*      DWORD dwLen */
/*         Number of WORDs to write to specified address. */
/* */
/*      DWORD dwAddr */
/*         Host memory address to read from. */
/* */
/*  Return (VOID): */
/* */
/*-------------------------------------------------------------------------- */

/*ARGSUSED*/
VOID
kOutsW (allegro_devc * devc, DWORD dwPort, DWORD dwLen, DWORD dwAddr)
{
#ifdef NON_INTEL
  DWORD i;
  WORD *wptr;
#endif

  if (!dwLen)
    return;


#ifdef DOS_MODEL
#ifdef NON_INTEL

  wptr = (WORD *) dwAddr;
  for (i = 0; i < dwLen; ++i)
    {
      outpw (devc, (WORD) dwPort, *wptr++);
    }				/* endfor */

#else
/*#pragma message("----Using slow I/O to overcome hardware bug") */
  _asm
  {
    push si
      push ds
      cld
      mov cx, word ptr dwLen mov dx, word ptr dwPort lds si, dwAddr rep outsw;
  xx:
    ;
    in al, 80 h;
    delay !;
    in al, 80 h;
    in al, 80 h;
    in al, 80 h;
    in al, 80 h;
    in al, 80 h;
    in al, 80 h;
    in al, 80 h;
    ;
    lodsw;
    get data;
    out dx, ax;
    write it;
    loop xx;
  get going ... pop ds pop si}
#endif
#endif

#if defined( VXD_MODEL ) || defined( WDM_MODEL ) || defined( NT_MODEL )
#ifdef NON_INTEL
  wptr = (WORD *) dwAddr;
  for (i = 0; i < dwLen; ++i)
    {
      outpw (devc->osdev, (WORD) dwPort, *wptr++);
    }				/* endfor */
#else
  _asm
  {
    push esi cld mov ecx, dwLen mov edx, dwPort mov esi, dwAddr rep outsw;
      xx:;
    in al, 80 h;
      delay !;
    in al, 80 h;
    in al, 80 h;
    in al, 80 h;
    in al, 80 h;
    in al, 80 h;
    in al, 80 h;
    in al, 80 h;
      lodsw;
    get data;
    out dx, ax;
    write it;
    loop xx;
  get going ... pop esi}
#endif
#endif

}				/* kOutsW() */

/*-------------------------------------------------------------------------- */
/* */
/*  WORD kDspReadWord */
/* */
/*  Description: */
/*      Read WORD from DSP memory. */
/* */
/*  Parameters: */
/*      DWORD dwBaseIO */
/*         Base I/O address of device. */
/* */
/*      DWORD dwMemType */
/*         Type of memory to read from. */
/* */
/*      DWORD dwMemAddr */
/*         Memory address to read from. */
/* */
/*  Return (WORD): */
/*      Data read from specified address. */
/* */
/*-------------------------------------------------------------------------- */

WORD
kDspReadWord (allegro_devc * devc, DWORD dwBaseIO, DWORD dwMemType,
	      DWORD dwMemAddr)
{
  WORD wData;

  CRITENTER
    /* 10/04/97, per Henry Tsay, write zeros to bits 15-2 of type register */
    kOutW (devc, dwBaseIO + DSP_PORT_MEMORY_TYPE, (WORD) dwMemType);
  kOutW (devc, dwBaseIO + DSP_PORT_MEMORY_INDEX, (WORD) dwMemAddr);
  wData = kInW (devc, dwBaseIO + DSP_PORT_MEMORY_DATA);
  CRITLEAVE return wData;
}				/* kDspReadWord() */

/*-------------------------------------------------------------------------- */
/* */
/*  VOID kDspWriteWord */
/* */
/*  Description: */
/*      Write WORD to DSP memory. */
/* */
/*  Parameters: */
/*      DWORD dwBaseIO */
/*         Base I/O address of device. */
/* */
/*      DWORD dwMemType */
/*         Type of memory to write to. */
/* */
/*      DWORD dwMemAddr */
/*         Memory address to write to. */
/* */
/*      WORD wMemData */
/*         Data to write to specified address. */
/* */
/*  Return (VOID): */
/* */
/*-------------------------------------------------------------------------- */

VOID kDspWriteWord
  (allegro_devc * devc, DWORD dwBaseIO, DWORD dwMemType, DWORD dwMemAddr,
   WORD wMemData)
{

  CRITENTER
    /* 10/04/97, per Henry Tsay, write zeros to bits 15-2 of type register */
    kOutW (devc, dwBaseIO + DSP_PORT_MEMORY_TYPE, (WORD) dwMemType);
  kOutW (devc, dwBaseIO + DSP_PORT_MEMORY_INDEX, (WORD) dwMemAddr);
  kOutW (devc, dwBaseIO + DSP_PORT_MEMORY_DATA, wMemData);
CRITLEAVE}			/* kDspWriteWord() */

/*----------------------------------------------------------------------*/
/* 									*/
/*  VOID kDspReadWords 							*/
/* 									*/
/*  Description: 							*/
/*      Read WORD block from DSP memory. 				*/
/* 									*/
/*  Parameters: 							*/
/*      DWORD dwBaseIO 							*/
/*         Base I/O address of device. 					*/
/* 									*/
/*      DWORD dwMemType 						*/
/*         Type of memory to read from. 				*/
/* 									*/
/*      DWORD dwMemAddr 						*/
/*         Memory address to read from. 				*/
/* 									*/
/*      DWORD dwMemLen 							*/
/*         Number of WORDs to read from specified address. 		*/
/* 									*/
/*      PWORD pwHostAddr 						*/
/*         Host memory address to write to. 				*/
/* 									*/
/*  Return (VOID): 							*/
/* 									*/
/*----------------------------------------------------------------------*/

VOID kDspReadWords
  (allegro_devc * devc, DWORD dwBaseIO,
   DWORD dwMemType, DWORD dwMemAddr, DWORD dwMemLen, PWORD pwHostAddr)
{

  CRITENTER
    /* 10/04/97, per Henry Tsay, write zeros to bits 15-2 of type register */
    kOutW (devc, dwBaseIO + DSP_PORT_MEMORY_TYPE, (WORD) dwMemType);
  kOutW (devc, dwBaseIO + DSP_PORT_MEMORY_INDEX, (WORD) dwMemAddr);
  kInsW (devc, dwBaseIO + DSP_PORT_MEMORY_DATA, dwMemLen, (DWORD) pwHostAddr);
CRITLEAVE}			/* kDspReadWords() */


/*-------------------------------------------------------------------------- */
/* */
/*  VOID kDspReadLongWords */
/* */
/*  Description: */
/*      Read WORD block which could be > 0x1000 from DSP memory. */
/*      Since auto-increment mode can cross 4K boundary, 4K block at a time */
/* */
/*  Parameters: */
/*      DWORD dwBaseIO */
/*         Base I/O address of device. */
/* */
/*      DWORD dwMemType */
/*         Type of memory to read from. */
/* */
/*      DWORD dwMemAddr */
/*         Memory address to read from. */
/* */
/*      DWORD dwMemLen */
/*         Number of WORDs to read from specified address. */
/* */
/*      PWORD pwHostAddr */
/*         Host memory address to write to. */
/* */
/*  Return (VOID): */
/* */
/*-------------------------------------------------------------------------- */

VOID kDspReadLongWords
  (allegro_devc * devc, DWORD dwBaseIO,
   DWORD dwMemType, DWORD dwMemLongAddr, DWORD dwMemLongLen, PWORD pwHostAddr)
{
  DWORD dwMemLen;
  DWORD dwMemBegin;
  WORD *pwBuffer;

  /* take care the Data area which could be > 0x1000 in size */
  dwMemLen = dwMemLongLen;
  dwMemBegin = dwMemLongAddr;
  pwBuffer = pwHostAddr;

  while (dwMemLen > 0)
    {
      if (dwMemLen >= 0x1000)
	{
	  kDspReadWords (devc, dwBaseIO, dwMemType, dwMemBegin, 0x1000,
			 pwBuffer);

	  dwMemBegin += 0x1000;
	  dwMemLen -= 0x1000;
	  pwBuffer = (pwBuffer + 0x1000);

	}
      else
	{
	  kDspReadWords (devc, dwBaseIO, dwMemType, dwMemBegin, dwMemLen,
			 pwBuffer);

	  dwMemLen = 0;

	}			/* endif */
    }				/* endwhile */
}				/* kDspReadLongWords() */


/*-------------------------------------------------------------------------- */
/* */
/*  VOID kDspWriteWords */
/* */
/*  Description: */
/*      Write WORD block to DSP memory. */
/* */
/*  Parameters: */
/*      DWORD dwBaseIO */
/*         Base I/O address of device. */
/* */
/*      DWORD dwMemType */
/*         Type of memory to write to. */
/* */
/*      DWORD dwMemAddr */
/*         Memory address to write to. */
/* */
/*      DWORD dwMemLen */
/*         Number of WORDs to write to specified address. */
/* */
/*      PWORD pwHostAddr */
/*         Host memory address to read from. */
/* */
/*  Return (VOID): */
/* */
/*-------------------------------------------------------------------------- */

VOID kDspWriteWords
  (allegro_devc * devc, DWORD dwBaseIO,
   DWORD dwMemType, DWORD dwMemAddr, DWORD dwMemLen, PWORD pwHostAddr)
{

  CRITENTER
    /* 10/04/97, per Henry Tsay, write zeros to bits 15-2 of type register */
    kOutW (devc, dwBaseIO + DSP_PORT_MEMORY_TYPE, (WORD) dwMemType);
  kOutW (devc, dwBaseIO + DSP_PORT_MEMORY_INDEX, (WORD) dwMemAddr);
  kOutsW (devc, dwBaseIO + DSP_PORT_MEMORY_DATA, dwMemLen,
	  (DWORD) pwHostAddr);
CRITLEAVE}			/* kDspWriteWords() */


/*-------------------------------------------------------------------------- */
/* */
/*  VOID kDspWriteLongWords */
/* */
/*  Description: */
/*      Write WORD block which could be > 0x1000 from DSP memory. */
/*      Since auto-increment mode can cross 4K boundary, 4K block at a time */
/* */
/*  Parameters: */
/*      DWORD dwBaseIO */
/*         Base I/O address of device. */
/* */
/*      DWORD dwMemType */
/*         Type of memory to Write to. */
/* */
/*      DWORD dwMemAddr */
/*         Memory address to Write to . */
/* */
/*      DWORD dwMemLen */
/*         Number of WORDs to write to specified address. */
/* */
/*      PWORD pwHostAddr */
/*         Host memory address to read from. */
/* */
/*  Return (VOID): */
/* */
/*-------------------------------------------------------------------------- */

VOID kDspWriteLongWords
  (allegro_devc * devc, DWORD dwBaseIO,
   DWORD dwMemType, DWORD dwMemLongAddr, DWORD dwMemLongLen, PWORD pwHostAddr)
{
  DWORD dwMemLen;
  DWORD dwMemBegin;
  WORD *pwBuffer;

  /* take care the Data area which could be > 0x1000 in size */
  dwMemLen = dwMemLongLen;
  dwMemBegin = dwMemLongAddr;
  pwBuffer = pwHostAddr;

  while (dwMemLen > 0)
    {
      if (dwMemLen >= 0x1000)
	{
	  kDspWriteWords (devc, dwBaseIO, dwMemType, dwMemBegin, 0x1000,
			  pwBuffer);

	  dwMemBegin += 0x1000;
	  dwMemLen -= 0x1000;
	  pwBuffer = (pwBuffer + 0x1000);

	}
      else
	{
	  kDspWriteWords (devc, dwBaseIO,
			  dwMemType, dwMemBegin, dwMemLen, pwBuffer);

	  dwMemLen = 0;

	}			/* endif */
    }				/* endwhile */
}				/* kDspWriteLongWords() */


/*-------------------------------------------------------------------------- */
/* */
/*  VOID kDspWriteZeros */
/* */
/*  Description: */
/*      Write zeros to DSP memory. */
/* */
/*  Parameters: */
/*      DWORD dwBaseIO */
/*         Base I/O address of device. */
/* */
/*      DWORD dwMemType */
/*         Type of memory to write to. */
/* */
/*      DWORD dwMemAddr */
/*         Memory address to write to. */
/* */
/*      DWORD dwMemLen */
/*         Number of WORDs of zero to write to specified address. */
/* */
/*  Return (VOID): */
/* */
/*-------------------------------------------------------------------------- */

VOID kDspWriteZeros
  (allegro_devc * devc, DWORD dwBaseIO, DWORD dwMemType, DWORD dwMemAddr,
   DWORD dwMemLen)
{
  while (dwMemLen--)
    {
/*#pragma message("----Using slow I/O to overcome hardware bug") */
/*         kDelayNMicroseconds( 8 ) ; */
      kDspWriteWord (devc, dwBaseIO, dwMemType, dwMemAddr++, 0);
    }

}				/* kDspWriteZeros() */


/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kQueryPosition */
/* */
/*  Description: */
/*      Return a client's current stream position.  A position equal to 0 */
/*      indicates the first byte in the buffer is being transferred.  A */
/*      position equal to dwHostXXXBufferLen-1 indicates the last byte */
/*      in the buffer is being transferred. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      PCLIENT_INST pClient_Inst */
/*         Pointer to client instance */
/* */
/*      DWORD dwQueryOutput */
/*         TRUE if querying host client output position, FALSE if querying input */
/* */
/*      PDWORD pdwPosition */
/*         Pointer to DWORD that will contain the byte position */
/* */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */

KRETURN kQueryPosition
  (allegro_devc * devc, PHWI phwi,
   PCLIENT_INST pClient_Inst, DWORD dwQueryOutput, PDWORD pdwPosition)
{
  DWORD dwClient = pClient_Inst->dwClient;
  WORD wPosition = 0;
  WORD wRetry = 10;
  WORD wPositionL = 0;
  WORD wPositionH = 0;
  PWORD pwCur;

  /* */
  /* Fail if the DSP kernel has been unloaded */
  /* */

  if (phwi->dwFlags & HWI_FLAG_UNLOADED)
    return KRETURN_ERROR_UNLOADED;

  /* */
  /* Make sure client and an instance exist... */
  /* */

  if (dwClient >= NUMBER_OF_CLIENTS)
    return KRETURN_ERROR_GENERIC;

  if (!phwi->asClientTable[dwClient].dwReferenceCount)
    return KRETURN_ERROR_GENERIC;

  if ((dwQueryOutput && pClient_Inst->dwDSPInConnection == KCONNECT_DMA) ||
      (!dwQueryOutput && pClient_Inst->dwDSPOutConnection == KCONNECT_DMA))
    {
      /* */
      /* if DMA is not active return position 0 */
      /* */

      pwCur = phwi->awVirtualDMAList;

      while ((*pwCur != F_FREE) && (*pwCur != (WORD) F_END))
	{
	  if (*pwCur ==
	      (WORD) (pClient_Inst->dwDspDataClientArea >> DP_SHIFT_COUNT))
	    {
	      break;
	    }
	  ++pwCur;
	}

      if (*pwCur !=
	  (WORD) (pClient_Inst->dwDspDataClientArea >> DP_SHIFT_COUNT))
	{
	  *pdwPosition = 0;
	  return KRETURN_SUCCESS;
	}

      /* */
      /* Get the position */
      /* */

      while (wRetry--)
	{
	  /* read high/low word of current position */

	  wPositionH = kDspReadWord (devc, phwi->dwBaseIO,
				     MEMTYPE_INTERNAL_DATA,
				     pClient_Inst->
				     dwDspDataClientArea +
				     CDATA_HOST_SRC_CURRENTH);

	  wPositionL = kDspReadWord (devc, phwi->dwBaseIO,
				     MEMTYPE_INTERNAL_DATA,
				     pClient_Inst->
				     dwDspDataClientArea +
				     CDATA_HOST_SRC_CURRENTL);

	  /* if the high word hasn't changed, we've got a meaningful */
	  /* current position value */

	  wPosition = kDspReadWord (devc, phwi->dwBaseIO,
				    MEMTYPE_INTERNAL_DATA,
				    pClient_Inst->
				    dwDspDataClientArea +
				    CDATA_HOST_SRC_CURRENTH);

	  if (wPosition == wPositionH)
	    break;
	}

      /* fail if we couldn't get a meaningful position */

      if (wPosition != wPositionH)
	return KRETURN_ERROR_GENERIC;

      *pdwPosition = MAKELONG (wPositionL, wPositionH) -
	(dwQueryOutput ?
	 pClient_Inst->dwHostSrcBufferAddr :
	 pClient_Inst->dwHostDstBufferAddr);
    }
  else if (!dwQueryOutput && pClient_Inst->dwDSPOutConnection == KCONNECT_PIO)
    {
      /* */
      /* Get the position */
      /* */

      *pdwPosition = pClient_Inst->dwHostDstCurrent -
	pClient_Inst->dwHostDstBufferAddr;
    }
  else
    {
      return KRETURN_ERROR_GENERIC;
    }

  return KRETURN_SUCCESS;

}				/* kQueryPosition() */


/*-------------------------------------------------------------------------- */
/* */
/*  BYTE kDspHalt */
/* */
/*  Description: */
/*      Halt the DSP. */
/* */
/*  Parameters: */
/*      DWORD dwBaseIO */
/*         Base I/O address of device. */
/* */
/*  Return (BYTE): */
/*      Contents of reset port. */
/* */
/*-------------------------------------------------------------------------- */

BYTE
kDspHalt (allegro_devc * devc, DWORD dwBaseIO)
{
  BYTE bData;
  bData = kInB (devc, dwBaseIO + DSP_PORT_CONTROL_REG_B) & ~REGB_STOP_CLOCK;
  /* Fix for HP Typhoon Hibernation Problem RJJ 6/17/00 */
  kDelayNMicroseconds (10);
  kOutB (devc, dwBaseIO + DSP_PORT_CONTROL_REG_B,
	 (BYTE) (bData & ~REGB_ENABLE_RESET));

  return bData;

}				/* kDspHalt() */

/*-------------------------------------------------------------------------- */
/* */
/*  VOID kDspReset */
/* */
/*  Description: */
/*      Reset the DSP and let it run. */
/* */
/*  Parameters: */
/*      DWORD dwBaseIO */
/*         Base I/O address of device. */
/* */
/*      BYTE bData */
/*         Contents of reset port. */
/* */
/*  Return (VOID): */
/* */
/*-------------------------------------------------------------------------- */

VOID
kDspReset (allegro_devc * devc, DWORD dwBaseIO, BYTE bData)
{
  kOutB (devc, dwBaseIO + DSP_PORT_CONTROL_REG_B,
	 (BYTE) (bData | REGB_ENABLE_RESET));

}				/* kDspReset() */

/*-------------------------------------------------------------------------- */
/* */
/*  VOID kDisableFMMap */
/* */
/*  Description: */
/*      Enable/disable FM address mapping. */
/* */
/*  Parameters: */
/*      DWORD dwBaseIO */
/*         Base I/O address of device. */
/* */
/*      DWORD dwDisable */
/*         TRUE if disabling FM mapping, FALSE otherwise */
/* */
/*  Return (VOID): */
/* */
/*-------------------------------------------------------------------------- */

VOID
kDisableFMMap (allegro_devc * devc, DWORD dwBaseIO, DWORD dwDisable)
{
  BYTE bData;

  bData =
    kInB (devc, dwBaseIO + DSP_PORT_CONTROL_REG_C) & ~REGC_DISABLE_FM_MAPPING;
  bData |= (dwDisable ? REGC_DISABLE_FM_MAPPING : 0);
  kOutB (devc, dwBaseIO + DSP_PORT_CONTROL_REG_C, (BYTE) bData);

}				/* kDisableFMMap() */

/*-------------------------------------------------------------------------- */
/* */
/*  VOID kRestartStreams */
/* */
/*  Description: */
/*      Restart all previously active streams. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*  Return (VOID): */
/* */
/*-------------------------------------------------------------------------- */

VOID
kRestartStreams (allegro_devc * devc, PHWI phwi)
{
  /* */
  /* reset DMA state machine in case it was active during the suspend */
  /* */
  /* If DMA was active, the worst thing that could happen is that we */
  /* will re-transfer the 16 words that were being transfered during */
  /* the suspend.  The 16 words are not duplicated in the stream, they */
  /* simply overwrite the exact copy that was previously transfered. */
  /* */

  if (phwi->awVirtualDMAList[0])
    {
      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA, KDATA_DMA_ACTIVE, FALSE);
    }


}				/* kRestartStreams() */

/*-------------------------------------------------------------------------- */
/* */
/*  DWORD kStateExists */
/* */
/*  Description: */
/*      Checks if the specified DSP data memory location is TRUE or FALSE. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      DWORD dwMemAddr */
/*         Memory address to check. */
/* */
/*      DWORD dwState */
/*         Specified state to check for (TRUE or FALSE) */
/* */
/*  Return (DWORD): */
/*      TRUE if specified state exists, FALSE otherwise */
/* */
/*-------------------------------------------------------------------------- */

#define RESET_IF_TIMEOUT

DWORD
kStateExists (allegro_devc * devc, PHWI phwi, DWORD dwMemAddr, DWORD dwState)
{
#ifdef RESET_IF_TIMEOUT
  DWORD dwResetLoops = 50;
#endif
  DWORD dwLoops;
  WORD wData;
  BYTE bData;

  for (dwLoops = 0;; ++dwLoops)
    {
      wData =
	kDspReadWord (devc, phwi->dwBaseIO, MEMTYPE_INTERNAL_DATA, dwMemAddr);

      if ((dwState && wData) || (!dwState && !wData))
	return TRUE;

      kDelayNMicroseconds (1);

      if (dwLoops == DSP_TIMEOUT)
	{
#ifdef RESET_IF_TIMEOUT

	  /* */
	  /* For some as yet unknown reason, if HSP/DSP modem runs */
	  /* concurrently with other DSP clients for a long time, */
	  /* the DSP stops executing.  Under such conditions a reset */
	  /* will resume DSP execution and the clients appear to run */
	  /* fine again.  Hence we do a reset if we detect the DSP */
	  /* kernel is no longer running. */
	  /* */

	  if (--dwResetLoops)
	    {
	      dwLoops = 0;
	      continue;
	    }

	  bData = kDspHalt (devc, phwi->dwBaseIO);

	  kRestartStreams (devc, phwi);

	  kDspReset (devc, phwi->dwBaseIO, bData);

	  /* */
	  /* Update our DSP reset counter.  This is purely for */
	  /* diagnostic purposes. */
	  /* */

	  if (phwi->wDspResetCount + 1)
	    ++phwi->wDspResetCount;

	  kDspWriteWord (devc, phwi->dwBaseIO,
			 MEMTYPE_INTERNAL_DATA,
			 KDATA_DSP_RESET_COUNT, phwi->wDspResetCount);
#endif

	  return FALSE;
	}
    }

}				/* kStateExists() */

/*-------------------------------------------------------------------------- */
/* */
/*  PBYTE kAllocDspMemory */
/* */
/*  Description: */
/*      Find and allocate available DSP memory. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      DWORD dwClient */
/*         Client ID */
/* */
/*      PBYTE pbMemoryMap */
/*         Pointer to memory map. */
/* */
/*      DWORD dwNumWanted */
/*         Number of contiguous memory units requested. */
/* */
/*  Return (PBYTE): */
/*      Pointer to starting memory map unit if successful, NULL otherwise */
/* */
/*-------------------------------------------------------------------------- */

PBYTE kAllocDspMemory
  (PHWI phwi, DWORD dwClient, PBYTE pbMemoryMap, DWORD dwNumWanted)
{
  PBYTE pbMemFound = NULL;
  PBYTE pbMemFound2 = NULL;
  DWORD dwNumFound = (DWORD) - 1;	/* maximum 0xFFFF */
  PBYTE pbMemTmp;
  DWORD dwNumTmp;
  PBYTE pbMemType;


  if (!dwNumWanted)
    return NULL;

  pbMemType = pbMemoryMap;	/*AY990524 Data & Code type */

  /* */
  /* Search for available memory.  We want to find the smallest */
  /* chunk of memory that can satisfy the request.  This will minimize */
  /* memory fragmentation. */
  /* */

  for (; *pbMemoryMap != (BYTE) F_END; ++pbMemoryMap)
    {
      if (*pbMemoryMap == F_USED)
	continue;

      pbMemTmp = pbMemoryMap;
      dwNumTmp = 1;

      while (*++pbMemoryMap == F_FREE)
	++dwNumTmp;

      if ((dwNumTmp >= dwNumWanted) && (dwNumTmp < dwNumFound))
	{
	  pbMemFound = pbMemTmp;
	  dwNumFound = dwNumTmp;
	  /*AY+ */
	  /* */
	  /* memory found, check whether it matches with client Code address */
	  /* */
	  if (pbMemType == (phwi->abCodeMemoryMap))	/* code memory */
	    {
	      DWORD dwCodeArea;
	      /*LINTED*/
	      dwCodeArea = ((pbMemFound - phwi->abCodeMemoryMap) *
			     phwi->dwCodeMemoryUnitLength) +
			     phwi->dwCodeMemoryBegin;

	      if (!kBinStructAddress (phwi, dwClient, dwCodeArea))
		{
		  /* no code binary found */
		  pbMemoryMap = pbMemTmp;	/* continue with next search */
		  ++pbMemoryMap;
		  dwCodeArea = NULL;
		  pbMemFound = NULL;
		  dwNumFound = (DWORD) - 1;

		}
	      else
		{

		  /* */
		  /* At least we found something availble, so remember just in case. */
		  /* */

		  pbMemFound2 = pbMemFound;

		}

	    }
	  /*AY- */
	}
      --pbMemoryMap;
    }

  /* */
  /* If memory found, allocate it.  Only allocate as many units that are */
  /* necessary to satisfy the request.   If the smallest memory is not  */
  /* found, then check whether other memory is available or not because  */
  /* just failing is the last thing that kernel would want to do. */
  /* */

  if (pbMemFound)
    {

      pbMemTmp = pbMemFound;
      dwNumTmp = dwNumWanted;

      while (dwNumTmp--)
	*pbMemTmp++ = F_USED;

    }
  else
    {
      if (pbMemFound2)
	{

	  pbMemTmp = pbMemFound2;
	  dwNumTmp = dwNumWanted;
	  pbMemFound = pbMemFound2;

	  while (dwNumTmp--)
	    *pbMemTmp++ = F_USED;

	}

    }

  return pbMemFound;

}				/* kAllocDspMemory() */

/*-------------------------------------------------------------------------- */
/* */
/*  DWORD kDeallocDspMemory */
/* */
/*  Description: */
/*      Deallocate DSP memory. */
/* */
/*  Parameters: */
/*      PBYTE pbMemFree */
/*         Pointer to starting memory map unit to free. */
/* */
/*      DWORD dwNumFree */
/*         Number of contiguous memory units to free. */
/* */
/*  Return (DWORD): */
/*      TRUE if successful, FALSE otherwise */
/* */
/*-------------------------------------------------------------------------- */

DWORD
kDeallocDspMemory (PBYTE pbMemFree, DWORD dwNumFree)
{

  while (dwNumFree--)
    *pbMemFree++ = F_FREE;

  return TRUE;

}				/* kDeallocDspMemory() */

/*-------------------------------------------------------------------------- */
/* */
/*  DWORD kConnectDirectMixer */
/* */
/*  Description: */
/*      Connect/Disconnect the specified directly mixed input and output. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      DWORD dwDSPInConnection */
/*         KCONNECT_XXX indicating input connection */
/* */
/*      DWORD dwDSPOutConnection */
/*         KCONNECT_XXX indicating output connection */
/* */
/*      DWORD dwConnect */
/*         TRUE if connecting, FALSE otherwise */
/* */
/*  Return (DWORD): */
/*      TRUE if successful, FALSE otherwise */
/* */
/*-------------------------------------------------------------------------- */

DWORD kConnectDirectMixer
  (allegro_devc * devc, PHWI phwi,
   DWORD dwDSPInConnection, DWORD dwDSPOutConnection, DWORD dwConnect)
{
  WORD wAddress = 0;
  WORD wData;

  /* */
  /* Get input setting */
  /* */

  if (dwDSPInConnection == KCONNECT_ADC1)
    wData = DIRECTMIXER_ADC1;
  else if (dwDSPInConnection == KCONNECT_ADC2)
    wData = DIRECTMIXER_ADC2;
  else
    wData = 0;

  /* */
  /* Get output setting */
  /* */

  if ((dwDSPOutConnection == KCONNECT_SPDIF) && dwConnect)
    {
      /* set the DSP SPDIF request flag */

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA, KDATA_SPDIF_REQUEST, TRUE);
      wAddress = KDATA_SPDIF_XFER;
    }

  /* */
  /* Write setting */
  /* */

  if (dwConnect)
    {
      wData |=
	kDspReadWord (devc, phwi->dwBaseIO, MEMTYPE_INTERNAL_DATA, wAddress);
    }
  else
    {
      wData ^=
	kDspReadWord (devc, phwi->dwBaseIO, MEMTYPE_INTERNAL_DATA, wAddress);
    }

  kDspWriteWord (devc, phwi->dwBaseIO, MEMTYPE_INTERNAL_DATA, wAddress,
		 wData);

#if 0
#if (NUMBER_OF_CONNECTIONS != 0x10)
#error Assumption about number of connections failed.
#endif
#endif

  return TRUE;

}				/* kConnectDirectMixer() */

/*-------------------------------------------------------------------------- */
/* */
/*  DWORD kAddListEntry */
/* */
/*  Description: */
/*      Add entry to host and DSP resource lists. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      PWORD pwHostListBegin */
/*         Start address of host resource list. */
/* */
/*      WORD wDSPListBegin */
/*         Start address of DSP resource list. */
/* */
/*      WORD wEntryValue */
/*         Value of entry to add.  Must not equal F_FREE or F_END! */
/* */
/*  Return (DWORD): */
/*      TRUE if successful, FALSE otherwise */
/* */
/*-------------------------------------------------------------------------- */

DWORD kAddListEntry
  (allegro_devc * devc, PHWI phwi, PWORD pwHostListBegin, WORD wDSPListBegin,
   WORD wEntryValue)
{
  PWORD pwCur = pwHostListBegin;

  /* */
  /* Search for an unused list entry */
  /* */

  while (*pwCur != (WORD) F_END)
    {
      if (*pwCur == F_FREE)
	{
	  /* */
	  /* Write wEntryValue to both the host list and the DSP list */
	  /* */

	  *pwCur = wEntryValue;

	  kDspWriteWord (devc, phwi->dwBaseIO,
			 MEMTYPE_INTERNAL_DATA,
			 wDSPListBegin + (pwCur - pwHostListBegin),
			 wEntryValue);
	  return TRUE;
	}
      ++pwCur;
    }

  return FALSE;

}				/* kAddListEntry() */

/*-------------------------------------------------------------------------- */
/* */
/*  DWORD kRemoveListEntry */
/* */
/*  Description: */
/*      Remove entry from host and DSP resource lists.  Then compact */
/*      the lists if necessary. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      PWORD pwHostListBegin */
/*         Start address of host resource list. */
/* */
/*      WORD wDSPListBegin */
/*         Start address of DSP resource list. */
/* */
/*      WORD wEntryValue */
/*         Value of entry to remove.  Must not equal F_FREE or F_END! */
/* */
/*  Return (DWORD): */
/*      TRUE if successful, FALSE otherwise */
/* */
/*-------------------------------------------------------------------------- */

DWORD kRemoveListEntry
  (allegro_devc * devc, PHWI phwi, PWORD pwHostListBegin, WORD wDSPListBegin,
   WORD wEntryValue)
{
  PWORD pwCur = pwHostListBegin;
  PWORD pwRemove = NULL;

  /* */
  /* Search for the entry containing wEntryValue and for the */
  /* last data entry */
  /* */

  while ((*pwCur != F_FREE) && (*pwCur != (WORD) F_END))
    {
      if (*pwCur == wEntryValue)
	{
	  pwRemove = pwCur;
	}
      ++pwCur;
    }

  /* Point to last data entry */

  if (pwCur != pwHostListBegin)
    {
      --pwCur;
    }

  /* */
  /* Fail if an entry containing wEntryValue could not be found */
  /* */

  if (!pwRemove)
    return FALSE;

  /* */
  /* OK, we've found our entry to remove and we've got the location */
  /* of the last data entry.  When we remove the entry, the void */
  /* that is created is filled by moving the last data entry into */
  /* the void.  The last data entry is then deleted.  We have to fill */
  /* up the void because list entries are defined to be packed together */
  /* (i.e. no gaps between entries). */
  /* */

  /* */
  /* If the entry to remove is NOT the same as the last data entry, we */
  /* first overwrite the entry to be removed with the value of the last */
  /* data entry.  Then we delete the last data entry. */
  /* */
  /* If the entry to remove is the same as the last data entry, we */
  /* simply delete the last data entry. */
  /* */

  if (pwRemove != pwCur)
    {
      *pwRemove = *pwCur;
      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     wDSPListBegin + (pwRemove - pwHostListBegin), *pwCur);
    }

  /* */
  /* Delete the last data entry */
  /* */

  *pwCur = NULL;
  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA,
		 wDSPListBegin + (pwCur - pwHostListBegin), 0);

  return TRUE;

}				/* kRemoveListEntry() */

/*-------------------------------------------------------------------------- */
/* */
/*  PWORD kInstanceListAddress */
/* */
/*  Description: */
/*      Return pointer to requested client instance list. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      DWORD dwClient */
/*         Client ID */
/* */
/*  Return (PWORD): */
/*      Pointer to client instance list. */
/* */
/*-------------------------------------------------------------------------- */

PWORD
kInstanceListAddress (PHWI phwi, DWORD dwClient)
{
  switch (dwClient)
    {
    case CLIENT_CPYTHRU:
      return phwi->awInstanceCpyThruList;

    case CLIENT_MODEM:
      return phwi->awInstanceModemList;

    case CLIENT_POS3D:
      return phwi->awInstancePos3DList;

    case CLIENT_SPKVIRT:
    case CLIENT_SPKVIRT_CRL:
      return phwi->awInstanceSpkVirtList;

    case CLIENT_SRC:
      return phwi->awInstanceSRCList;

    case CLIENT_MINISRC:
      return phwi->awInstanceMINISRCList;

    case CLIENT_SPDIF:
      return phwi->awInstanceSPDIFList;


      /* we should never end up here! */
    default:
      return NULL;
    }
#if 0
#if (NUMBER_OF_CLIENTS != 8)
#error Assumption about number of clients failed.
#endif
#endif
}				/* kInstanceListAddress() */

/*-------------------------------------------------------------------------- */
/* */
/*  PWORD kHostXferListAddress */
/* */
/*  Description: */
/*      Return pointer to requested host transfer list. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      DWORD dwDSPConnection */
/*         KCONNECT_XXX indicating connection */
/* */
/*  Return (PWORD): */
/*      Pointer to host transfer list. */
/* */
/*-------------------------------------------------------------------------- */

PWORD
kHostXferListAddress (PHWI phwi, DWORD dwDSPConnection)
{
  switch (dwDSPConnection)
    {
    case KCONNECT_NONE:
      return NULL;

    case KCONNECT_DMA:
      return phwi->awVirtualDMAList;

    case KCONNECT_ADC1:
      return phwi->awVirtualADC1List;

    case KCONNECT_ADC2:
      return phwi->awVirtualADC2List;

    case KCONNECT_CD:
      return phwi->awVirtualCDList;

    case KCONNECT_MIC:
      return phwi->awVirtualMICList;

    case KCONNECT_I2S:
      return phwi->awVirtualI2SList;

    case KCONNECT_CHI:
      return phwi->awVirtualCHIList;

    case KCONNECT_SPDIFIN:
      return phwi->awVirtualSPDIFINList;

    case KCONNECT_MIXER:
      return phwi->awVirtualMIXERList;

      /*AY */
    case KCONNECT_FMIXER:
      return phwi->awVirtualFMIXERList;

    case KCONNECT_RMIXER:
      return phwi->awVirtualRMIXERList;

    case KCONNECT_SAME:
      return NULL;

      /* we should never end up here! */
    default:
      return NULL;
    }
#if 0
#if (NUMBER_OF_CONNECTIONS != 0x10)
#error Assumption about number of connections failed.
#endif
#endif
}				/* kHostXferListAddress() */

/*-------------------------------------------------------------------------- */
/* */
/*  WORD kDSPXferListAddress */
/* */
/*  Description: */
/*      Return pointer to requested DSP transfer list. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      DWORD dwDSPConnection */
/*         KCONNECT_XXX indicating connection */
/* */
/*  Return (WORD): */
/*      Pointer to DSP transfer list. */
/* */
/*-------------------------------------------------------------------------- */

/*ARGSUSED*/
WORD
kDSPXferListAddress (PHWI phwi, DWORD dwDSPConnection)
{
  switch (dwDSPConnection)
    {
    case KCONNECT_NONE:
      return NULL;

    case KCONNECT_DMA:
      return KDATA_DMA_XFER0;

    case KCONNECT_ADC1:
      return KDATA_ADC1_XFER0;

    case KCONNECT_ADC2:
      return KDATA_ADC2_XFER0;

    case KCONNECT_CD:
      return KDATA_CD_XFER0;

    case KCONNECT_MIC:
      return KDATA_MIC_XFER0;

    case KCONNECT_I2S:
      return KDATA_I2S_XFER0;

    case KCONNECT_CHI:
      return KDATA_CHI_XFER0;

    case KCONNECT_SPDIF:
      return KDATA_SPDIF_XFER;

    case KCONNECT_SPDIFIN:
      return KDATA_SPDIFIN_XFER0;

    case KCONNECT_MIXER:
      return KDATA_MIXER_XFER0;

      /*AY */
    case KCONNECT_FMIXER:
      return KDATA_FMIXER_XFER0;

    case KCONNECT_RMIXER:
      return KDATA_RMIXER_XFER0;

    case KCONNECT_SAME:
      return NULL;

      /* we should never end up here! */
    default:
      return NULL;
    }
#if 0
#if (NUMBER_OF_CONNECTIONS != 0x10)
#error Assumption about number of connections failed.
#endif
#endif
}				/* kDSPXferListAddress() */

/*-------------------------------------------------------------------------- */
/* */
/*  DWORD kConnectInputOutput */
/* */
/*  Description: */
/*      Connect the specified input and output. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      DWORD dwDSPInConnection */
/*         KCONNECT_XXX indicating input connection */
/* */
/*      DWORD dwDSPOutConnection */
/*         KCONNECT_XXX indicating output connection */
/* */
/*      DWORD dwDspDataArea */
/*         Address of DSP data area containing connection header */
/* */
/*  Return (DWORD): */
/*      TRUE if successful, FALSE otherwise */
/* */
/*-------------------------------------------------------------------------- */

DWORD kConnectInputOutput
  (allegro_devc * devc, PHWI phwi,
   DWORD dwDSPInConnection, DWORD dwDSPOutConnection, DWORD dwDspDataArea)
{
  /* */
  /* connect the input */
  /* */

  if ((dwDSPInConnection != KCONNECT_NONE) &&
      (dwDSPInConnection != KCONNECT_SAME) &&
      (dwDSPInConnection < NUMBER_OF_CONNECTIONS))
    {
      if (!kAddListEntry (devc, phwi,
			  kHostXferListAddress (phwi, dwDSPInConnection),
			  kDSPXferListAddress (phwi, dwDSPInConnection),
			  (WORD) (dwDspDataArea >> DP_SHIFT_COUNT)))
	return FALSE;

      if (dwDSPInConnection == KCONNECT_ADC1)
	kDspWriteWord (devc, phwi->dwBaseIO,
		       MEMTYPE_INTERNAL_DATA, KDATA_ADC1_REQUEST, TRUE);


      if (dwDSPInConnection == KCONNECT_ADC2)
	kDspWriteWord (devc, phwi->dwBaseIO,
		       MEMTYPE_INTERNAL_DATA, KDATA_ADC2_REQUEST, TRUE);


      if (dwDSPInConnection == KCONNECT_CD)
	kDspWriteWord (devc, phwi->dwBaseIO,
		       MEMTYPE_INTERNAL_DATA, KDATA_CD_REQUEST, TRUE);

      if (dwDSPInConnection == KCONNECT_MIC)
	kDspWriteWord (devc, phwi->dwBaseIO,
		       MEMTYPE_INTERNAL_DATA, KDATA_MIC_REQUEST, TRUE);

      if (dwDSPInConnection == KCONNECT_I2S)
	kDspWriteWord (devc, phwi->dwBaseIO,
		       MEMTYPE_INTERNAL_DATA, KDATA_I2S_INT_METER, 0);

      if (dwDSPInConnection == KCONNECT_SPDIFIN)
	{
	  kDspWriteWord (devc, phwi->dwBaseIO,
			 MEMTYPE_INTERNAL_DATA, KDATA_SPDIFIN_INT_METER, 0);

	}

    }
  else
    {
      /* */
      /*   input buffer is connected to other instance's output buffer */
      /* */

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     (WORD) (dwDspDataArea) + CDATA_IN_BUF_CONNECT,
		     (WORD) dwDSPInConnection);
    }


  /* */
  /* connect the output */
  /* */

  if ((dwDSPOutConnection != KCONNECT_NONE) &&
      (dwDSPOutConnection != KCONNECT_SAME) &&
      (dwDSPOutConnection < NUMBER_OF_CONNECTIONS))
    {
      if (!kAddListEntry (devc, phwi,
			  kHostXferListAddress (phwi, dwDSPOutConnection),
			  kDSPXferListAddress (phwi, dwDSPOutConnection),
			  (WORD) (dwDspDataArea >> DP_SHIFT_COUNT)))
	return FALSE;

      /* */
      /* if mixer, tell the dsp the total number of mixing list */
      /* */

      if (dwDSPOutConnection == KCONNECT_MIXER)
	{

	  MIXER_TASK_NUMBER++;

	  kDspWriteWord (devc, phwi->dwBaseIO,
			 MEMTYPE_INTERNAL_DATA,
			 KDATA_MIXER_TASK_NUMBER, MIXER_TASK_NUMBER);
	}
    }
  else
    {
      /* */
      /* output buffer is connected to other instance's input buffer */
      /* */

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     (WORD) (dwDspDataArea) + CDATA_OUT_BUF_CONNECT,
		     (WORD) dwDSPOutConnection);
    }

  return TRUE;

}				/* kConnectInputOutput() */

/*-------------------------------------------------------------------------- */
/* */
/*  DWORD kDisconnectInputOutput */
/* */
/*  Description: */
/*      Disconnect the specified input and output. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      DWORD dwDSPInConnection */
/*         KCONNECT_XXX indicating input connection */
/* */
/*      DWORD dwDSPOutConnection */
/*         KCONNECT_XXX indicating output connection */
/* */
/*      DWORD dwDspDataArea */
/*         Address of DSP data area containing connection header */
/* */
/*  Return (DWORD): */
/*      TRUE if successful, FALSE otherwise */
/* */
/*-------------------------------------------------------------------------- */

DWORD kDisconnectInputOutput
  (allegro_devc * devc, PHWI phwi,
   DWORD dwDSPInConnection, DWORD dwDSPOutConnection, DWORD dwDspDataArea)
{
  WORD wdata;

  /* */
  /* disconnect the input */
  /* */

  if ((dwDSPInConnection != KCONNECT_NONE) &&
      (dwDSPInConnection != KCONNECT_SAME))
    {

      kRemoveListEntry (devc, phwi,
			kHostXferListAddress (phwi, dwDSPInConnection),
			kDSPXferListAddress (phwi, dwDSPInConnection),
			(WORD) (dwDspDataArea >> DP_SHIFT_COUNT));

      if (dwDSPInConnection == KCONNECT_ADC1)
	kDspWriteWord (devc, phwi->dwBaseIO,
		       MEMTYPE_INTERNAL_DATA, KDATA_ADC1_REQUEST, FALSE);

      if (dwDSPInConnection == KCONNECT_ADC2)
	kDspWriteWord (devc, phwi->dwBaseIO,
		       MEMTYPE_INTERNAL_DATA, KDATA_ADC2_REQUEST, FALSE);

      if (dwDSPInConnection == KCONNECT_CD)
	kDspWriteWord (devc, phwi->dwBaseIO,
		       MEMTYPE_INTERNAL_DATA, KDATA_CD_REQUEST, FALSE);

      if (dwDSPInConnection == KCONNECT_MIC)
	kDspWriteWord (devc, phwi->dwBaseIO,
		       MEMTYPE_INTERNAL_DATA, KDATA_MIC_REQUEST, FALSE);

      if (dwDSPInConnection == KCONNECT_I2S)
	{
	  wdata = kDspReadWord (devc, phwi->dwBaseIO,
				MEMTYPE_INTERNAL_DATA, KDATA_I2S_ACTIVE);

	  wdata &= ~0x1;

	  kDspWriteWord (devc, phwi->dwBaseIO,
			 MEMTYPE_INTERNAL_DATA, KDATA_I2S_ACTIVE, wdata);
	}

      if (dwDSPInConnection == KCONNECT_SPDIFIN)
	{
	  wdata = kDspReadWord (devc, phwi->dwBaseIO,
				MEMTYPE_INTERNAL_DATA, KDATA_I2S_ACTIVE);

	  wdata &= ~0x2;

	  kDspWriteWord (devc, phwi->dwBaseIO,
			 MEMTYPE_INTERNAL_DATA, KDATA_I2S_ACTIVE, wdata);
	}

    }

  /* */
  /* disconnect the output */
  /* */

  if ((dwDSPOutConnection != KCONNECT_NONE) &&
      (dwDSPOutConnection != KCONNECT_SAME))
    {
      kRemoveListEntry (devc, phwi,
			kHostXferListAddress (phwi, dwDSPOutConnection),
			kDSPXferListAddress (phwi, dwDSPOutConnection),
			(WORD) (dwDspDataArea >> DP_SHIFT_COUNT));
    }

  /* */
  /* if mixer, tell the dsp the total number of mixing list */
  /* */

  if (dwDSPOutConnection == KCONNECT_MIXER)
    {

      MIXER_TASK_NUMBER--;

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     KDATA_MIXER_TASK_NUMBER, MIXER_TASK_NUMBER);
    }

  /* */
  /* handle special cases */
  /* */

  if ((dwDSPInConnection == KCONNECT_DMA) ||
      (dwDSPOutConnection == KCONNECT_DMA))
    {
      /* do DMA synchronization */

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA, KDATA_DMA_SWITCH, FALSE);
    }

  return TRUE;

}				/* kDisconnectInputOutput() */

/*-------------------------------------------------------------------------- */
/* */
/*  PCLIENT_BIN kBinStructAddress */
/* */
/*  Description: */
/*      Return pointer to requested binary image structure. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      DWORD dwClient */
/*         Client ID */
/* */
/*      DWORD dwSearchKey */
/*         Search key */
/* */
/*  Return (PCLIENT_BIN): */
/*      Pointer to structure if successful, NULL otherwise */
/* */
/*-------------------------------------------------------------------------- */

PCLIENT_BIN
kBinStructAddress (PHWI phwi, DWORD dwClient, DWORD dwSearchKey)
{
  PCLIENT_BIN pClient_Bin;

  if (dwClient >= NUMBER_OF_CLIENTS)
    return NULL;

  pClient_Bin = phwi->asClientTable[dwClient].pClient_Bin;

  /* */
  /* If search key is NULL, then just return address of first struct */
  /* */

  if (!dwSearchKey)
    return pClient_Bin;

  /* */
  /* Search key is code load address */
  /* */

  while (pClient_Bin->dwCodeAddress)
    {
      if (pClient_Bin->dwCodeAddress == dwSearchKey)
	{
	  return pClient_Bin;
	}

      ++pClient_Bin;
    }

  return NULL;

}				/* kBinStructAddress() */

/*-------------------------------------------------------------------------- */
/* */
/*  DWORD kAllocDspVectors */
/* */
/*  Description: */
/*      Allocate DSP vectors required by client. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      DWORD dwClient */
/*         Client ID */
/* */
/*  Return (DWORD): */
/*      TRUE if successful, FALSE otherwise */
/* */
/*-------------------------------------------------------------------------- */

DWORD
kAllocDspVectors (allegro_devc * devc, PHWI phwi, DWORD dwClient)
{
  PCLIENT_BIN pClient_Bin;
  DWORD dwAllocate;
  DWORD dwIndex;

  /* */
  /* Check if client needs to allocate DSP vectors */
  /* */

  if (!(pClient_Bin = kBinStructAddress (phwi, dwClient,
					 phwi->asClientTable[dwClient].
					 dwDspCodeClientArea)))
    return FALSE;

  dwAllocate = FALSE;

  for (dwIndex = KCODE_VECTORS_UNIT_LENGTH;	/* vector 0 reserved */
       dwIndex < KCODE_VECTORS_LENGTH; dwIndex += KCODE_VECTORS_UNIT_LENGTH)
    {
      if (pClient_Bin->pwBinVect[dwIndex + 0] ||
	  pClient_Bin->pwBinVect[dwIndex + 1])
	{
	  dwAllocate = TRUE;

	  /* fail if the vector is already allocated */

	  if ((phwi->awVectorList[dwIndex + 0] != F_FREE) ||
	      (phwi->awVectorList[dwIndex + 1] != F_FREE))
	    return FALSE;
	}
    }

  if (!dwAllocate)
    return TRUE;

  /* */
  /* Make sure DSP kernel is ready to be halted */
  /* */

  if (!kStateExists (devc, phwi, KDATA_HALT_ACKNOWLEDGE, FALSE))
    return FALSE;

  /* */
  /* halt the DSP kernel and wait for an acknowledgement */
  /* */

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA, KDATA_HALT_SYNCH_CLIENT, TRUE);

  if (!kStateExists (devc, phwi, KDATA_HALT_ACKNOWLEDGE, TRUE))
    {
      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA, KDATA_HALT_SYNCH_CLIENT, FALSE);

      return FALSE;
    }

  /* */
  /* DSP kernel has halted, do vector allocation */
  /* */
  /* Note: We halt the DSP in order to safely modify vectors */
  /* */

  for (dwIndex = KCODE_VECTORS_UNIT_LENGTH;	/* vector 0 reserved */
       dwIndex < KCODE_VECTORS_LENGTH; dwIndex += KCODE_VECTORS_UNIT_LENGTH)
    {
      if (pClient_Bin->pwBinVect[dwIndex + 0] ||
	  pClient_Bin->pwBinVect[dwIndex + 1])
	{
	  /* save current DSP vector */

	  phwi->awVectorList[dwIndex + 0] =
	    kDspReadWord (devc, phwi->dwBaseIO, MEMTYPE_INTERNAL_CODE,
			  dwIndex + 0);

	  phwi->awVectorList[dwIndex + 1] =
	    kDspReadWord (devc, phwi->dwBaseIO, MEMTYPE_INTERNAL_CODE,
			  dwIndex + 1);

	  /* write new DSP vector */

	  kDspWriteWord (devc, phwi->dwBaseIO,
			 MEMTYPE_INTERNAL_CODE,
			 dwIndex + 0, pClient_Bin->pwBinVect[dwIndex + 0]);

	  kDspWriteWord (devc, phwi->dwBaseIO,
			 MEMTYPE_INTERNAL_CODE,
			 dwIndex + 1, pClient_Bin->pwBinVect[dwIndex + 1]);
	}
    }

  /* */
  /* all done, reset halt request flag to resume DSP kernel execution */
  /* */

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA, KDATA_HALT_SYNCH_CLIENT, FALSE);

  return TRUE;

}				/* kAllocDspVectors() */

/*-------------------------------------------------------------------------- */
/* */
/*  DWORD kDeallocDspVectors */
/* */
/*  Description: */
/*      Deallocate DSP vectors previously required by client. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      DWORD dwClient */
/*         Client ID */
/* */
/*  Return (DWORD): */
/*      TRUE if successful, FALSE otherwise */
/* */
/*-------------------------------------------------------------------------- */

DWORD
kDeallocDspVectors (allegro_devc * devc, PHWI phwi, DWORD dwClient)
{
  PCLIENT_BIN pClient_Bin;
  DWORD dwDeallocate;
  DWORD dwIndex;

  /* */
  /* Check if client needs to deallocate DSP vectors */
  /* */

  if (!(pClient_Bin = kBinStructAddress (phwi, dwClient,
					 phwi->asClientTable[dwClient].
					 dwDspCodeClientArea)))
    return FALSE;

  dwDeallocate = FALSE;

  for (dwIndex = KCODE_VECTORS_UNIT_LENGTH;	/* vector 0 reserved */
       dwIndex < KCODE_VECTORS_LENGTH; dwIndex += KCODE_VECTORS_UNIT_LENGTH)
    {
      if (pClient_Bin->pwBinVect[dwIndex + 0] ||
	  pClient_Bin->pwBinVect[dwIndex + 1])
	{
	  dwDeallocate = TRUE;

	  /* fail if the vector was not allocated */

	  if ((phwi->awVectorList[dwIndex + 0] == F_FREE) &&
	      (phwi->awVectorList[dwIndex + 1] == F_FREE))
	    return FALSE;
	}
    }

  if (!dwDeallocate)
    return TRUE;

  /* */
  /* Make sure DSP kernel is ready to be halted */
  /* */

  if (!kStateExists (devc, phwi, KDATA_HALT_ACKNOWLEDGE, FALSE))
    return FALSE;

  /* */
  /* halt the DSP kernel and wait for an acknowledgement */
  /* */

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA, KDATA_HALT_SYNCH_CLIENT, TRUE);

  if (!kStateExists (devc, phwi, KDATA_HALT_ACKNOWLEDGE, TRUE))
    {
      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA, KDATA_HALT_SYNCH_CLIENT, FALSE);

      return FALSE;
    }

  /* */
  /* DSP kernel has halted, do vector deallocation */
  /* */
  /* Note: We halt the DSP in order to safely modify vectors */
  /* */

  for (dwIndex = KCODE_VECTORS_UNIT_LENGTH;	/* vector 0 reserved */
       dwIndex < KCODE_VECTORS_LENGTH; dwIndex += KCODE_VECTORS_UNIT_LENGTH)
    {
      if (pClient_Bin->pwBinVect[dwIndex + 0] ||
	  pClient_Bin->pwBinVect[dwIndex + 1])
	{
	  /* restore original DSP vector */

	  kDspWriteWord (devc, phwi->dwBaseIO,
			 MEMTYPE_INTERNAL_CODE,
			 dwIndex + 0, phwi->awVectorList[dwIndex + 0]);

	  kDspWriteWord (devc, phwi->dwBaseIO,
			 MEMTYPE_INTERNAL_CODE,
			 dwIndex + 1, phwi->awVectorList[dwIndex + 1]);

	  /* reset vector list */

	  phwi->awVectorList[dwIndex + 0] = F_FREE;
	  phwi->awVectorList[dwIndex + 1] = F_FREE;
	}
    }

  /* */
  /* all done, reset halt request flag to resume DSP kernel execution */
  /* */

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA, KDATA_HALT_SYNCH_CLIENT, FALSE);

  return TRUE;

}				/* kDeallocDspVectors() */

/*-------------------------------------------------------------------------- */
/* */
/*  VOID kLoadKernel */
/* */
/*  Description: */
/*      Load the kernel into DSP memory. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      DWORD dwBaseIO */
/*         Base I/O address of device. */
/* */
/*      DWORD dwFlags */
/*         Hardware instance flags. */
/* */
/*  Return (VOID): */
/* */
/*-------------------------------------------------------------------------- */

/*ARGSUSED*/
VOID
kLoadKernel (allegro_devc * devc, PHWI phwi, DWORD dwBaseIO, DWORD dwFlags)
{
  BYTE bData;

  bData = kDspHalt (devc, dwBaseIO);

  kDisableFMMap (devc, dwBaseIO, TRUE);

  /* clear DSP kernel data memory */

  kDspWriteZeros (devc, dwBaseIO,
		  MEMTYPE_INTERNAL_DATA,
		  KDATA_BASE_ADDR,
		  NUM_UNITS_KERNEL_DATA * phwi->dwDataMemoryUnitLength);

  /* clear DSP kernel mixer data memory */

  kDspWriteZeros (devc, dwBaseIO,
		  MEMTYPE_INTERNAL_DATA,
		  KDATA_BASE_ADDR2,
		  NUM_UNITS_KERNEL_DATA * phwi->dwDataMemoryUnitLength);


  /* initialize current DMA pointer */

  kDspWriteWord (devc, dwBaseIO, MEMTYPE_INTERNAL_DATA, KDATA_CURRENT_DMA, KDATA_DMA_XFER0);	/* khs 121198 */

  /* initialize SPDIF frame status pointer and array */

  kDspWriteWord (devc, dwBaseIO,
		 MEMTYPE_INTERNAL_DATA,
		 KDATA_SPDIF_CURRENT_FRAME, KDATA_SPDIF_FRAME0);

  kDspWriteWord (devc, dwBaseIO, MEMTYPE_INTERNAL_DATA, KDATA_SPDIF_FRAME0,
		 /* 0x0204 ) ; */
		 0x0100);	/*AY990508 CD data and copy prohibited for SCMS */

  kDspWriteWord (devc, dwBaseIO, MEMTYPE_INTERNAL_DATA, KDATA_SPDIF_FRAME1, 0x0200);	/*48K only */

  /* download kernel to DSP memory */
  {
    kDspWriteWords (devc, dwBaseIO,
		    MEMTYPE_INTERNAL_CODE,
		    phwi->dwCodeMemoryBegin,
		    (gsKernelVectCode.dwLengthCode + 1) / 2,
		    gsKernelVectCode.pwBinCode);
  }

  /* indicate that kernel is loaded */
  phwi->dwFlags &= ~HWI_FLAG_UNLOADED;

  /* Set Master Volume to Maximum */
  kSetMasterVolume (devc, phwi, 0x7FFF, 0x7FFF);

  kDspReset (devc, dwBaseIO, bData);

}				/* kLoadKernel() */

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kInitKernel */
/* */
/*  Description: */
/*      Initialize the host and DSP kernel. */
/* */
/*  Parameters: */
/*      PHWI *pphwi */
/*         Pointer to Pointer to hardware instance. */
/* */
/*      DWORD dwDeviceID */
/*         Device ID. */
/* */
/*      DWORD dwRevisionID */
/*         Revision ID. */
/*         should always be 0x10 for M3/Allegro1 */
/* */
/*      DWORD dwBaseIO */
/*         Base I/O address of device. */
/* */
/*      DWORD dwFlags */
/*         Hardware instance flags. */
/*         should always be 0 since there is no difference for HSP */
/* */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */

KRETURN kInitKernel
  (allegro_devc * devc, PHWI * pphwi,
   DWORD dwDeviceID, DWORD dwRevisionID, DWORD dwBaseIO, DWORD dwFlags)
{

  PHWI phwi;
  BYTE *pbSrc;
  BYTE *pbDst;
  WORD i;

#ifdef K_DBG
  _Debug_Printf_Service ("kInitKernel DID=%X RID=%X IO=%X Flag=%X\n",
			 dwDeviceID, dwRevisionID, dwBaseIO, dwFlags);
#endif
  /* */
  /* Allocate hardware instance buffer, initialize it, and pass */
  /* the buffer address back to the caller */
  /* */

  if (!(phwi = (PHWI) _HeapAllocate (sizeof (HWI), HEAPZEROINIT)))
    return KRETURN_ERROR_GENERIC;

  *phwi = ghwi;
  *pphwi = phwi;

  /* */
  /* Initialize the hardware instance */
  /* */

  phwi->dwDeviceID = dwDeviceID;
  phwi->dwRevisionID = dwRevisionID;
  phwi->dwBaseIO = dwBaseIO;
  phwi->dwFlags = dwFlags;	/* should be 0x0 */


  /* */
  /*   Check and report bad sectors in DSP Memory */
  /* */
/*aaa */
  /*if (phwi -> dwFlags & HWI_FLAG_MEM_CHECK) { */
  kDSPMemCheck (devc, phwi);
  /*      return KRETURN_ERROR_GENERIC ; */
  /* } */

  /* init memory map bounds */

  {
    phwi->dwCodeMemoryBegin = REV_B_CODE_MEMORY_BEGIN;
    phwi->dwCodeMemoryEnd = REV_B_CODE_MEMORY_END;
    phwi->dwCodeMemoryUnitLength = REV_B_CODE_MEMORY_UNIT_LENGTH;
    phwi->dwCodeMemoryLength = REV_B_CODE_MEMORY_LENGTH;

    phwi->dwDataMemoryBegin = REV_B_DATA_MEMORY_BEGIN;
    phwi->dwDataMemoryEnd = REV_B_DATA_MEMORY_END;
    phwi->dwDataMemoryUnitLength = REV_B_DATA_MEMORY_UNIT_LENGTH;
    phwi->dwDataMemoryLength = REV_B_DATA_MEMORY_LENGTH;

    /* transfer memory map images */

    pbSrc = gabRevBCodeMemoryMapImage;
    pbDst = phwi->abCodeMemoryMap;

    while ((*pbDst++ = *pbSrc++) != (BYTE) F_END);

    pbSrc = gabRevBDataMemoryMapImage;
    pbDst = phwi->abDataMemoryMap;

    while ((*pbDst++ = *pbSrc++) != (BYTE) F_END);
  }

  /* */
  /* Initialize the DSP */
  /* */

  kLoadKernel (devc, phwi, dwBaseIO, dwFlags);



  /* testing : dump memory map to 0x1800 */
  pbDst = phwi->abDataMemoryMap;

  *pbDst++ = F_USED;
  *pbDst++ = F_USED;

  pbDst = phwi->abDataMemoryMap;

  for (i = 0; i < 24; i++)
    {
      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     (DWORD) 0x10c0 + i, (WORD) ((*pbDst++) * 0xFFFF));
    }

  return KRETURN_SUCCESS;

}				/* kInitKernel() */

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kDSPMemCheck */
/* */
/*  Description: */
/*      Check the DSP Data memory and update a memory map in host kernel. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/*       */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */
/*kkk    */
KRETURN
kDSPMemCheck (allegro_devc * devc, PHWI phwi)
{
  BYTE bData;
  BYTE bData2;

  WORD wLoop = 0;
  WORD wFlag = 0;
  WORD wFlag2 = 1;
  WORD wFlag3 = 1;
  WORD i;
  DWORD addr = 0;


  /*  */
  /* begin to check memory  */
  /* */

  /* save the clock speed for restoreing */
  bData2 = kInB (devc, phwi->dwBaseIO + DSP_PORT_CONTROL_REG_A);

  for (wLoop = 0; wLoop < 20; wLoop++)
    {

      bData = kDspHalt (devc, phwi->dwBaseIO);

      /*  change to 49Mhz for intensive memory test */

      kOutB (devc, phwi->dwBaseIO + DSP_PORT_CONTROL_REG_A, (BYTE) 0x09);
/*            kOutB( phwi -> dwBaseIO + DSP_PORT_CONTROL_REG_C, (BYTE) 0x10 ) ; */


      /* clear DSP kernel data memory */

      kDspWriteZeros (devc, phwi->dwBaseIO,
		      MEMTYPE_INTERNAL_DATA,
		      KDATA_BASE_ADDR, REV_B_DATA_MEMORY_LENGTH);

      /* load memory check code */

      kDspWriteWords (devc, phwi->dwBaseIO,
		      MEMTYPE_INTERNAL_CODE,
		      phwi->dwCodeMemoryBegin,
		      (gsMemChkVectCode.dwLengthCode + 1) / 2,
		      gsMemChkVectCode.pwBinCode);

      /* start DSP */


      kDspReset (devc, phwi->dwBaseIO, bData);

      for (i = 0; i < 3; i++)
	{

	  if (i == 0)
	    addr = FLAGADD1;
	  if (i == 1)
	    addr = FLAGADD2;
	  if (i == 2)
	    addr = FLAGADD3;


	  /* initialize */

	  kDspWriteWord (devc, phwi->dwBaseIO, MEMTYPE_INTERNAL_DATA,
			 addr + 9, wFlag);

	  kDspWriteWord (devc, phwi->dwBaseIO, MEMTYPE_INTERNAL_DATA,
			 addr + 5, wFlag2);

	  kDspWriteWord (devc, phwi->dwBaseIO, MEMTYPE_INTERNAL_DATA,
			 addr + 6, wFlag3);


	  /* first 1k check */

	  /*LINTED*/
	  while (1)
	    {

	      wFlag =
		kDspReadWord (devc, phwi->dwBaseIO, MEMTYPE_INTERNAL_DATA,
			      addr);

	      if (wFlag == 0x0001)	/* passed */
		{

		  wFlag = 0;

		  kDspWriteWord (devc, phwi->dwBaseIO, MEMTYPE_INTERNAL_DATA,
				 addr, wFlag);

		  break;

		}
	      else if (wFlag == 0x0002)	/* found a bad sector  */
		{

		  wFlag =
		    kDspReadWord (devc, phwi->dwBaseIO, MEMTYPE_INTERNAL_DATA,
				  addr + 2);

		  /* update memory map */
		  gabRevBDataMemoryMapImage[((wFlag -
					      REV_B_DATA_MEMORY_BEGIN) >> 7)]
		    = F_USED;

		  wFlag = 0;

		  kDspWriteWord (devc, phwi->dwBaseIO, MEMTYPE_INTERNAL_DATA,
				 addr, wFlag);

		  kDspWriteWord (devc, phwi->dwBaseIO, MEMTYPE_INTERNAL_DATA,
				 addr + 5, wFlag2);
		}

	    }			/* while */

	}			/* for(3) */

    }				/* for(100) */


  /* */
  /* everything is done, so restore the clock */
  /* */

  bData = kDspHalt (devc, phwi->dwBaseIO);

  kOutB (devc, phwi->dwBaseIO + DSP_PORT_CONTROL_REG_A, (BYTE) bData2);

  kDspReset (devc, phwi->dwBaseIO, bData);


  return KRETURN_SUCCESS;

}

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kTermKernel */
/* */
/*  Description: */
/*      Terminate the host and DSP kernel. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      DWORD dwBaseIO */
/*         Base I/O address of device. */
/* */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */

KRETURN
kTermKernel (allegro_devc * devc, PHWI phwi, DWORD dwBaseIO)
{
  kDspHalt (devc, dwBaseIO);

  _HeapFree (phwi, 0);

  return KRETURN_SUCCESS;

}				/* kTermKernel() */

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kSuspendKernel */
/* */
/*  Description: */
/*      Suspend the kernel and save its state. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */

KRETURN
kSuspendKernel (allegro_devc * devc, PHWI phwi)
{
  /* */
  /* FM is a special case */
  /* */
#ifdef K_DBG
  _Debug_Printf_Service ("kSuspendKernel %X\n", phwi);
#endif

  if (phwi->dwFlags & HWI_FLAG_FM_LOADED)
    {
      /* */
      /* Allocate host memory */
      /* */

      if (!(phwi->pwSuspendBuffer = (PWORD)
	    _HeapAllocate ((phwi->dwCodeMemoryLength +
			    phwi->dwDataMemoryLength) * 2, HEAPZEROINIT)))
	return KRETURN_ERROR_GENERIC;

      /* */
      /* Do memory save */
      /* */

      kDspReadWords (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_CODE,
		     phwi->dwCodeMemoryBegin,
		     phwi->dwCodeMemoryLength, phwi->pwSuspendBuffer);

      kDspReadLongWords (devc, phwi->dwBaseIO,
			 MEMTYPE_INTERNAL_DATA,
			 phwi->dwDataMemoryBegin,
			 phwi->dwDataMemoryLength,
			 phwi->pwSuspendBuffer + phwi->dwCodeMemoryLength);

      /* */
      /* Indicate FM is suspended */
      /* */

      phwi->dwFlags |= HWI_FLAG_SUSPENDED;

      return KRETURN_SUCCESS;
    }

  /* */
  /* Fail if the DSP kernel has been unloaded */
  /* */

  if (phwi->dwFlags & HWI_FLAG_UNLOADED)
    return KRETURN_ERROR_UNLOADED;

  /* */
  /* Allocate host memory */
  /* */

  if (!(phwi->pwSuspendBuffer = (PWORD)
	_HeapAllocate ((phwi->dwCodeMemoryLength +
			phwi->dwDataMemoryLength) * 2, HEAPZEROINIT)))
    return KRETURN_ERROR_GENERIC;

  /* */
  /* Make sure DSP kernel is ready to be halted */
  /* */

  if (!kStateExists (devc, phwi, KDATA_HALT_ACKNOWLEDGE, FALSE))
    goto kSuspend_Error;

  /* */
  /* halt the DSP kernel and wait for an acknowledgement */
  /* */

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA, KDATA_HALT_SYNCH_CLIENT, TRUE);

  if (!kStateExists (devc, phwi, KDATA_HALT_ACKNOWLEDGE, TRUE))
    {
      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA, KDATA_HALT_SYNCH_CLIENT, FALSE);

      goto kSuspend_Error;
    }

  /* */
  /* DSP kernel has halted, do memory save */
  /* */
  /* Note: We halt the DSP in order to place it in a known state */
  /* */

  kDspReadWords (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_CODE,
		 phwi->dwCodeMemoryBegin,
		 phwi->dwCodeMemoryLength, phwi->pwSuspendBuffer);

  kDspReadLongWords (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     phwi->dwDataMemoryBegin,
		     phwi->dwDataMemoryLength,
		     phwi->pwSuspendBuffer + phwi->dwCodeMemoryLength);

  /* */
  /* Kernel was successfully suspended, indicate kernel is unloaded */
  /* */

  phwi->dwFlags |= (HWI_FLAG_SUSPENDED | HWI_FLAG_UNLOADED);

  return KRETURN_SUCCESS;

kSuspend_Error:

  /* */
  /* Failure, undo all that was done... */
  /* */

  if (phwi->pwSuspendBuffer)
    {
      _HeapFree (phwi->pwSuspendBuffer, 0);
    }

  return KRETURN_ERROR_GENERIC;

}				/* kSuspendKernel() */

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kResumeKernel */
/* */
/*  Description: */
/*      Resume the kernel and restore its state. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */

KRETURN
kResumeKernel (allegro_devc * devc, PHWI phwi)
{
  BYTE bData;

#ifdef K_DBG
  _Debug_Printf_Service ("kResumeKernel %X\n", phwi);
#endif
  /* */
  /* FM is a special case */
  /* */

  if (phwi->dwFlags & HWI_FLAG_FM_LOADED)
    {
      /* */
      /* Fail if FM isn't suspended */
      /* */

      if (!(phwi->dwFlags & HWI_FLAG_SUSPENDED))
	return KRETURN_ERROR_GENERIC;

      /* */
      /* Do memory restore */
      /* */

      bData = kDspHalt (devc, phwi->dwBaseIO);

      if (!phwi->pwSuspendBuffer)
	return KRETURN_ERROR_GENERIC;

      kDspWriteWords (devc, phwi->dwBaseIO,
		      MEMTYPE_INTERNAL_CODE,
		      phwi->dwCodeMemoryBegin,
		      phwi->dwCodeMemoryLength, phwi->pwSuspendBuffer);

      kDspWriteLongWords (devc, phwi->dwBaseIO,
			  MEMTYPE_INTERNAL_DATA,
			  phwi->dwDataMemoryBegin,
			  phwi->dwDataMemoryLength,
			  phwi->pwSuspendBuffer + phwi->dwCodeMemoryLength);

      _HeapFree (phwi->pwSuspendBuffer, 0);

      phwi->pwSuspendBuffer = 0;

      kDspReset (devc, phwi->dwBaseIO, bData);

      /* Indicate FM is no longer suspended */

      phwi->dwFlags &= ~HWI_FLAG_SUSPENDED;

      return KRETURN_SUCCESS;
    }

  /* */
  /* Fail if the kernel isn't suspended */
  /* */

  if (!(phwi->dwFlags & HWI_FLAG_SUSPENDED))
    return KRETURN_ERROR_GENERIC;

  /* */
  /* Do memory restore */
  /* */

  bData = kDspHalt (devc, phwi->dwBaseIO);

  if (!phwi->pwSuspendBuffer)
    return KRETURN_ERROR_GENERIC;

  kDspWriteWords (devc, phwi->dwBaseIO,
		  MEMTYPE_INTERNAL_CODE,
		  phwi->dwCodeMemoryBegin,
		  phwi->dwCodeMemoryLength, phwi->pwSuspendBuffer);

  kDspWriteLongWords (devc, phwi->dwBaseIO,
		      MEMTYPE_INTERNAL_DATA,
		      phwi->dwDataMemoryBegin,
		      phwi->dwDataMemoryLength,
		      phwi->pwSuspendBuffer + phwi->dwCodeMemoryLength);

  _HeapFree (phwi->pwSuspendBuffer, 0);

  phwi->pwSuspendBuffer = 0;

  kRestartStreams (devc, phwi);

  /* */
  /* all done, reset halt request flag to resume DSP kernel execution */
  /* */

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA, KDATA_HALT_SYNCH_CLIENT, FALSE);

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA, KDATA_HALT_ACKNOWLEDGE, FALSE);

  kDspReset (devc, phwi->dwBaseIO, bData);

  /* Indicate kernel is loaded */

  phwi->dwFlags &= ~(HWI_FLAG_SUSPENDED | HWI_FLAG_UNLOADED);

  return KRETURN_SUCCESS;

}				/* kResumeKernel() */

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kOpenInstance */
/* */
/*  Description: */
/*      Open a client instance. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      DWORD dwClient */
/*         Client ID */
/* */
/*      DWORD dwFlags */
/*         KOPENCLOSE_XXX flags indicating open to perform */
/* */
/*      DWORD dwLen */
/*         Length in bytes of data area of CLIENT_INST (zero allowed) */
/* */
/*      PCLIENT_INST *ppClient_Inst */
/*         Pointer to PCLIENT_INST that will contain the instance pointer */
/* */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */

KRETURN kOpenInstance
  (allegro_devc * devc, PHWI phwi,
   DWORD dwClient, DWORD dwFlags, DWORD dwLen, PCLIENT_INST * ppClient_Inst)
{
  PCLIENT_INST pClient_Inst = NULL;
  PCLIENT_BIN pClient_Bin = NULL;

#ifdef K_DBG
  _Debug_Printf_Service ("kOpenInstance %X %X %X %X\n", phwi,
			 dwClient, dwFlags, dwLen);
#endif
  /* */
  /* Fail if the DSP kernel has been unloaded */
  /* */

  if (phwi->dwFlags & HWI_FLAG_UNLOADED)
    return KRETURN_ERROR_UNLOADED;

#ifndef NT_MODEL
  /* */
  /* FM client is a special case */
  /* */
  /* The FM client isn't really a client of the DSP kernel.  It requires */
  /* the entire DSP to execute therefore the DSP kernel must be swapped */
  /* out of the DSP before it can run.  When FM is closed the DSP kernel */
  /* is reloaded into the DSP. */
  /* */

  if (dwClient == CLIENT_FM)
    {
      DWORD dwClientMasks;
      DWORD dwConnectMasks;
      BYTE bData;

      /* Fail if the DSP kernel is not idle */

      if (kQueryActivity (devc, phwi, &dwClientMasks, &dwConnectMasks) !=
	  KRETURN_SUCCESS)
	return KRETURN_ERROR_GENERIC;

      if (dwClientMasks || dwConnectMasks)
	return KRETURN_ERROR_BUSY;

      /* */
      /* Allocate host memory */
      /* */

      if (!(pClient_Inst = (PCLIENT_INST)
	    _HeapAllocate (sizeof (CLIENT_INST) + dwLen, HEAPZEROINIT)))
	return KRETURN_ERROR_GENERIC;

      pClient_Inst->dwClient = dwClient;

      *ppClient_Inst = pClient_Inst;

      /* download FM client to DSP memory */

      bData = kDspHalt (devc, phwi->dwBaseIO);

      kDisableFMMap (devc, phwi->dwBaseIO, FALSE);

      kDspWriteWords (devc, phwi->dwBaseIO,
		      MEMTYPE_INTERNAL_CODE,
		      phwi->dwCodeMemoryBegin,
		      (gsFMVectCode.dwLengthVect + 1) / 2,
		      gsFMVectCode.pwBinVect);

      kDspWriteWords (devc, phwi->dwBaseIO,
		      MEMTYPE_INTERNAL_CODE,
		      gsFMVectCode.dwCodeAddress,
		      (gsFMVectCode.dwLengthCode + 1) / 2,
		      gsFMVectCode.pwBinCode);

      kDspWriteWords (devc, phwi->dwBaseIO,
		      MEMTYPE_INTERNAL_DATA,
		      phwi->dwDataMemoryBegin,
		      (gsFMVectCode.dwLengthData + 1) / 2,
		      gsFMVectCode.pwBinData);

      kDspWriteWords (devc, phwi->dwBaseIO,
		      MEMTYPE_INTERNAL_DATA,
		      gsFMVectCode.dwData2Address,
		      (gsFMVectCode.dwLengthData2 + 1) / 2,
		      gsFMVectCode.pwBinData2);

      kDspReset (devc, phwi->dwBaseIO, bData);

      /* FM client was successfully loaded, indicate kernel is unloaded */

      phwi->dwFlags |= (HWI_FLAG_FM_LOADED | HWI_FLAG_UNLOADED);

      return KRETURN_SUCCESS;
    }
#endif

  /* */
  /* Do open/close synchronization */
  /* */
  /* Generally, an open can safely be done except just after a close. */
  /* The reason a close might cause a problem is that when a client */
  /* requests a close, the host-side may close but the DSP-side */
  /* may be executing AT THAT MOMENT.  This means the host-side thinks */
  /* resources that were formally allocated are now free, whereas on */
  /* the DSP-side they actually are still in use.  So if an open is done, */
  /* and the open happens to be assigned some of these same resources, a */
  /* resource conflict arises.  For example, the executable of the open */
  /* client might get downloaded over the still executing close client, */
  /* likely causing a DSP crash. */
  /* */
  /* One way to overcome this danger is by making sure a client task */
  /* switch has occured SINCE THE CLOSE WAS COMPLETED.  A task switch */
  /* covers the following cases: */
  /* */
  /* 1) A DSP-side client is executing just as the kernel closes it. */
  /*    A subsequent task switch means this client is no longer running, */
  /*    so the close is complete and the freed resources are available. */
  /* */
  /* 2) A DSP-side client is idle when the kernel closes it.  A */
  /*    subsequent task switch to this client either means the client */
  /*    doesn't run at all (client closed) or an instance is not */
  /*    processed at all (instance closed).  Thus the resources */
  /*    previously used by the client or instance are freed. */
  /* */

  if (!kStateExists (devc, phwi, KDATA_TASK_SWITCH, TRUE))
    return KRETURN_ERROR_BUSY;

  /* */
  /* Make sure client and an available instance exist... */
  /* */

  if (dwClient >= NUMBER_OF_CLIENTS)
    return KRETURN_ERROR_GENERIC;

  if (phwi->asClientTable[dwClient].dwReferenceCount >=
      phwi->asClientTable[dwClient].dwMaxReference)
    return KRETURN_ERROR_GENERIC;

  /* */
  /* Allocate host memory */
  /* */

  if (!(pClient_Inst = (PCLIENT_INST)
	_HeapAllocate (sizeof (CLIENT_INST) + dwLen, HEAPZEROINIT)))
    goto kOpen_Error;

  pClient_Inst->dwClient = dwClient;
  pClient_Inst->dwDspDataNumUnits =
    NUM_UNITS (dwLen, phwi->dwDataMemoryUnitLength);

  /* */
  /* Allocate DSP data memory */
  /* */

  if (dwLen)
    {
      if (!(pClient_Inst->pbDspDataMapPtr =
	    kAllocDspMemory (phwi, NULL,
			     phwi->abDataMemoryMap,
			     pClient_Inst->dwDspDataNumUnits)))
	goto kOpen_Error;

      pClient_Inst->dwDspDataClientArea =
      /*LINTED*/
	((pClient_Inst->pbDspDataMapPtr - phwi->abDataMemoryMap) *
	  phwi->dwDataMemoryUnitLength) + phwi->dwDataMemoryBegin;

      /* clear DSP client data memory */

      kDspWriteZeros (devc, phwi->dwBaseIO,
		      MEMTYPE_INTERNAL_DATA,
		      pClient_Inst->dwDspDataClientArea,
		      pClient_Inst->dwDspDataNumUnits *
		      phwi->dwDataMemoryUnitLength);

      /* download client data binary */

      if (!(pClient_Bin = kBinStructAddress (phwi, dwClient, NULL)))
	goto kOpen_Error;

      /* make sure allocation size can hold data binary */

      if (dwLen < pClient_Bin->dwLengthData)
	goto kOpen_Error;

      kDspWriteWords (devc, phwi->dwBaseIO,
		      MEMTYPE_INTERNAL_DATA,
		      pClient_Inst->dwDspDataClientArea,
		      (pClient_Bin->dwLengthData + 1) / 2,
		      pClient_Bin->pwBinData);

      /* add instance to instance list */
      if (dwFlags & KOPENCLOSE_SYNCHRONOUS)
	/*if ( 0 ) */
	{
	  DWORD dwReturn;

	  /* */
	  /* Make sure DSP kernel is ready to be halted */
	  /* */

	  if (!kStateExists (devc, phwi, KDATA_HALT_ACKNOWLEDGE, FALSE))
	    goto kOpen_Error;

	  /* */
	  /* halt the DSP kernel and wait for an acknowledgement */
	  /* */

	  kDspWriteWord (devc, phwi->dwBaseIO,
			 MEMTYPE_INTERNAL_DATA,
			 KDATA_HALT_SYNCH_CLIENT, TRUE);

	  if (!kStateExists (devc, phwi, KDATA_HALT_ACKNOWLEDGE, TRUE))
	    {
	      kDspWriteWord (devc, phwi->dwBaseIO,
			     MEMTYPE_INTERNAL_DATA,
			     KDATA_HALT_SYNCH_CLIENT, FALSE);

	      goto kOpen_Error;
	    }

	  /* */
	  /* DSP kernel has halted, do open */
	  /* */
	  /* Note: We halt the DSP in order to synchronize the open */
	  /* with the client */
	  /* */

	  dwReturn = kAddListEntry (devc, phwi,
				    kInstanceListAddress (phwi, dwClient),
				    (WORD) phwi->asClientTable[dwClient].
				    dwInstanceListArea,
				    (WORD) (pClient_Inst->
					    dwDspDataClientArea >>
					    DP_SHIFT_COUNT));

	  /* */
	  /* all done, reset halt request flag to resume DSP kernel execution */
	  /* */

	  kDspWriteWord (devc, phwi->dwBaseIO,
			 MEMTYPE_INTERNAL_DATA,
			 KDATA_HALT_SYNCH_CLIENT, FALSE);

	  if (!dwReturn)
	    goto kOpen_Error;
	}
      else
	{
	  if (!kAddListEntry (devc, phwi,
			      kInstanceListAddress (phwi, dwClient),
			      (WORD) phwi->asClientTable[dwClient].
			      dwInstanceListArea,
			      (WORD) (pClient_Inst->
				      dwDspDataClientArea >> DP_SHIFT_COUNT)))
	    goto kOpen_Error;

	}



    }

/*#pragma message("----Assuming individual client code binaries are equal length!") */

  phwi->asClientTable[dwClient].dwDspCodeNumUnits =
    NUM_UNITS (phwi->asClientTable[dwClient].pClient_Bin->dwLengthCode,
	       phwi->dwCodeMemoryUnitLength);

  /* */
  /* Allocate DSP code memory */
  /* */

  if (!phwi->asClientTable[dwClient].dwReferenceCount)
    {
      if (!(phwi->asClientTable[dwClient].pbDspCodeMapPtr =
	    kAllocDspMemory (phwi, dwClient,
			     phwi->abCodeMemoryMap,
			     phwi->asClientTable[dwClient].
			     dwDspCodeNumUnits)))
	goto kOpen_Error;

      phwi->asClientTable[dwClient].dwDspCodeClientArea =
      /*LINTED*/
	((phwi->asClientTable[dwClient].pbDspCodeMapPtr -
	  phwi->abCodeMemoryMap) * phwi->dwCodeMemoryUnitLength) +
	  phwi->dwCodeMemoryBegin;

      pClient_Inst->dwDspCodeClientArea =
	phwi->asClientTable[dwClient].dwDspCodeClientArea;

      /* clear DSP client code memory */

      kDspWriteZeros (devc, phwi->dwBaseIO,
		      MEMTYPE_INTERNAL_CODE,
		      phwi->asClientTable[dwClient].dwDspCodeClientArea,
		      phwi->asClientTable[dwClient].dwDspCodeNumUnits *
		      phwi->dwCodeMemoryUnitLength);

      /* download client code binary */

      if (!(pClient_Bin = kBinStructAddress (phwi, dwClient,
					     phwi->asClientTable[dwClient].
					     dwDspCodeClientArea)))
	goto kOpen_Error;

      kDspWriteWords (devc, phwi->dwBaseIO,
		      MEMTYPE_INTERNAL_CODE,
		      phwi->asClientTable[dwClient].dwDspCodeClientArea,
		      (pClient_Bin->dwLengthCode + 1) / 2,
		      pClient_Bin->pwBinCode);

      /* allocate DSP vectors */

      if (!kAllocDspVectors (devc, phwi, dwClient))
	goto kOpen_Error;

      /* add client to task list */

      if (!kAddListEntry (devc, phwi,
			  phwi->awTaskList,
			  KDATA_TASK0,
			  (WORD) phwi->asClientTable[dwClient].
			  dwDspCodeClientArea))
	goto kOpen_Error;
    }
  else
    {
      pClient_Inst->dwDspCodeClientArea =
	phwi->asClientTable[dwClient].dwDspCodeClientArea;
    }

  /* */
  /* Everything worked, increment reference counter */
  /* */

  ++phwi->asClientTable[dwClient].dwReferenceCount;

  *ppClient_Inst = pClient_Inst;
#ifdef K_DBG
  _Debug_Printf_Service ("kOpenInstance OK  %X\n", pClient_Inst);
#endif

  return KRETURN_SUCCESS;

kOpen_Error:

  /* */
  /* Failure, undo all that was done... */
  /* */

  if (!phwi->asClientTable[dwClient].dwReferenceCount)
    {
      if (phwi->asClientTable[dwClient].pbDspCodeMapPtr)
	{
	  kDeallocDspVectors (devc, phwi, dwClient);

	  kDeallocDspMemory (phwi->asClientTable[dwClient].pbDspCodeMapPtr,
			     phwi->asClientTable[dwClient].dwDspCodeNumUnits);
	  phwi->asClientTable[dwClient].pbDspCodeMapPtr = NULL;
	}
    }

  if (pClient_Inst)
    {
      if (pClient_Inst->pbDspDataMapPtr)
	{
	  kRemoveListEntry (devc, phwi,
			    kInstanceListAddress (phwi, dwClient),
			    (WORD) phwi->asClientTable[dwClient].
			    dwInstanceListArea,
			    (WORD) (pClient_Inst->
				    dwDspDataClientArea >> DP_SHIFT_COUNT));

	  kDeallocDspMemory (pClient_Inst->pbDspDataMapPtr,
			     pClient_Inst->dwDspDataNumUnits);
	}

      _HeapFree (pClient_Inst, 0);
    }
#ifdef K_DBG
  _Debug_Printf_Service ("kOpenInstance Failed\n");
#endif

  return KRETURN_ERROR_GENERIC;

}				/* kOpenInstance() */

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kCloseInstance */
/* */
/*  Description: */
/*      Close a client instance. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      PCLIENT_INST pClient_Inst */
/*         Pointer to client instance */
/* */
/*      DWORD dwFlags */
/*         KOPENCLOSE_XXX flags indicating close to perform */
/* */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */

KRETURN
kCloseInstance (allegro_devc * devc, PHWI phwi, PCLIENT_INST pClient_Inst,
		DWORD dwFlags)
{
  DWORD dwClient = pClient_Inst->dwClient;

#ifdef K_DBG
  _Debug_Printf_Service ("kCloseInstance %X %X %X\n", phwi,
			 pClient_Inst, dwFlags);


#endif
#ifndef NT_MODEL
  /* */
  /* FM client is a special case */
  /* */

  if (dwClient == CLIENT_FM)
    {
      /* Fail if the FM client isn't open */

      if (!(phwi->dwFlags & HWI_FLAG_FM_LOADED))
	return KRETURN_ERROR_GENERIC;

      kLoadKernel (devc, phwi, phwi->dwBaseIO, phwi->dwFlags);

      _HeapFree (pClient_Inst, 0);

      /* Indicate kernel is loaded */

      phwi->dwFlags &= ~(HWI_FLAG_FM_LOADED | HWI_FLAG_UNLOADED);

      return KRETURN_SUCCESS;
    }
#endif

  /* */
  /* Fail if the DSP kernel has been unloaded */
  /* */

  if (phwi->dwFlags & HWI_FLAG_UNLOADED)
    return KRETURN_ERROR_UNLOADED;

  /* */
  /* Do DMA synchronization */
  /* */
  /* (see discussion in kOpenInstance) */
  /* */

  if (!kStateExists (devc, phwi, KDATA_DMA_SWITCH, TRUE))
    return KRETURN_ERROR_BUSY;

  /* */
  /* Make sure client and an instance exist... */
  /* */

  if (dwClient >= NUMBER_OF_CLIENTS)
    return KRETURN_ERROR_GENERIC;

  if (!phwi->asClientTable[dwClient].dwReferenceCount)
    return KRETURN_ERROR_GENERIC;

  /* */
  /* Deallocate memory */
  /* */

  if (--phwi->asClientTable[dwClient].dwReferenceCount == 0)
    {
      /* remove client from task list */

      kRemoveListEntry (devc, phwi,
			phwi->awTaskList,
			KDATA_TASK0,
			(WORD) phwi->asClientTable[dwClient].
			dwDspCodeClientArea);

      if (!phwi->asClientTable[dwClient].pbDspCodeMapPtr)
	return KRETURN_ERROR_GENERIC;

      kDeallocDspVectors (devc, phwi, dwClient);

      kDeallocDspMemory (phwi->asClientTable[dwClient].pbDspCodeMapPtr,
			 phwi->asClientTable[dwClient].dwDspCodeNumUnits);
      phwi->asClientTable[dwClient].pbDspCodeMapPtr = NULL;
    }


  if (!pClient_Inst)
    return KRETURN_ERROR_GENERIC;

  if (pClient_Inst->pbDspDataMapPtr)
    {
      if (dwFlags & KOPENCLOSE_SYNCHRONOUS)
	/* if ( 0 ) */
	{
	  /* */
	  /* Make sure DSP kernel is ready to be halted */
	  /* */

	  if (!kStateExists (devc, phwi, KDATA_HALT_ACKNOWLEDGE, FALSE))
	    goto kClose_Error;

	  /* */
	  /* halt the DSP kernel and wait for an acknowledgement */
	  /* */

	  kDspWriteWord (devc, phwi->dwBaseIO,
			 MEMTYPE_INTERNAL_DATA,
			 KDATA_HALT_SYNCH_CLIENT, TRUE);

	  kStateExists (devc, phwi, KDATA_HALT_ACKNOWLEDGE, TRUE);

	kClose_Error:

	  /* */
	  /* DSP kernel has halted, do close */
	  /* */
	  /* Note: We halt the DSP in order to synchronize the close */
	  /* with the client */
	  /* */

	  kRemoveListEntry (devc, phwi,
			    kInstanceListAddress (phwi, dwClient),
			    (WORD) phwi->asClientTable[dwClient].
			    dwInstanceListArea,
			    (WORD) (pClient_Inst->
				    dwDspDataClientArea >> DP_SHIFT_COUNT));

	  /* */
	  /* all done, reset halt request flag to resume DSP kernel execution */
	  /* */

	  kDspWriteWord (devc, phwi->dwBaseIO,
			 MEMTYPE_INTERNAL_DATA,
			 KDATA_HALT_SYNCH_CLIENT, FALSE);
	}
      else
	{
	  kRemoveListEntry (devc, phwi,
			    kInstanceListAddress (phwi, dwClient),
			    (WORD) phwi->asClientTable[dwClient].
			    dwInstanceListArea,
			    (WORD) (pClient_Inst->
				    dwDspDataClientArea >> DP_SHIFT_COUNT));

	}

      kDeallocDspMemory (pClient_Inst->pbDspDataMapPtr,
			 pClient_Inst->dwDspDataNumUnits);
    }

  _HeapFree (pClient_Inst, 0);

  /* */
  /* Do open/close synchronization */
  /* */

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA, KDATA_TASK_SWITCH, FALSE);

#ifdef K_DBG
  _Debug_Printf_Service ("kCloseInstance OK\n");
#endif
  return KRETURN_SUCCESS;

}				/* kCloseInstance() */

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kSwitchClient */
/* */
/*  Description: */
/*      Remove/Add the client from/to the task list. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      PCLIENT_INST pClient_Inst */
/*         Pointer to client instance */
/* */
/*      DWORD dwFlags */
/*         KENABLE_CLIENT flags adding client back to task list */
/*         KDISABLE_CLIENT flags removing client from task list */
/* */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */

KRETURN
kSwitchClient (allegro_devc * devc, PHWI phwi, PCLIENT_INST pClient_Inst,
	       DWORD dwFlags)
{
  DWORD dwClient = pClient_Inst->dwClient;
#ifdef K_DBG
  _Debug_Printf_Service ("kSwitchClient %X %X %X\n", phwi,
			 pClient_Inst, dwFlags);



#endif
  /* */
  /* Fail if the DSP kernel has been unloaded */
  /* */

  if (phwi->dwFlags & HWI_FLAG_UNLOADED)
    return KRETURN_ERROR_UNLOADED;

  /* */
  /* Do DMA synchronization */
  /* */
  /* (see discussion in kOpenInstance) */
  /* */

  if (!kStateExists (devc, phwi, KDATA_DMA_SWITCH, TRUE))
    return KRETURN_ERROR_BUSY;

  /* */
  /* Make sure client and an instance exist... */
  /* */

  if (dwClient >= NUMBER_OF_CLIENTS)
    return KRETURN_ERROR_GENERIC;

  if (phwi->asClientTable[dwClient].dwReferenceCount != 1)
    return KRETURN_ERROR_GENERIC;

  if (dwFlags == KDISABLE_CLIENT)
    {
      /* remove client from task list */

      kRemoveListEntry (devc, phwi,
			phwi->awTaskList,
			KDATA_TASK0,
			(WORD) phwi->asClientTable[dwClient].
			dwDspCodeClientArea);

    }
  else
    {
      if (dwFlags == KENABLE_CLIENT)
	{
	  /* add client to task list */

	  if (!kAddListEntry (devc, phwi,
			      phwi->awTaskList,
			      KDATA_TASK0,
			      (WORD) phwi->asClientTable[dwClient].
			      dwDspCodeClientArea))
	    {
	      return KRETURN_ERROR_GENERIC;
	    }

	}
      else
	{
	  return KRETURN_ERROR_GENERIC;
	}			/* endif */
    }				/* endif */

#ifdef K_DBG
  _Debug_Printf_Service ("kSwitchClient OK\n");
#endif

  return KRETURN_SUCCESS;

}				/* kSwitchClient() */

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kSetInstanceReady */
/* */
/*  Description: */
/*      Set a client instance to the ready state. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      PCLIENT_INST pClient_Inst */
/*         Pointer to client instance */
/* */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */

KRETURN
kSetInstanceReady (allegro_devc * devc, PHWI phwi, PCLIENT_INST pClient_Inst)
{
  DWORD dwClient = pClient_Inst->dwClient;

#ifdef K_DBG
  _Debug_Printf_Service ("kSetInstanceReady %X %x\n", phwi, pClient_Inst);
#endif
  /* */
  /* Fail if the DSP kernel has been unloaded */
  /* */

  if (phwi->dwFlags & HWI_FLAG_UNLOADED)
    return KRETURN_ERROR_UNLOADED;

  /* */
  /* Make sure client and an instance exist... */
  /* */

  if (dwClient >= NUMBER_OF_CLIENTS)
    return KRETURN_ERROR_GENERIC;

  if (!phwi->asClientTable[dwClient].dwReferenceCount)
    return KRETURN_ERROR_GENERIC;

  /* */
  /* Set the instance ready flag */
  /* */

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA,
		 pClient_Inst->
		 dwDspDataClientArea + CDATA_INSTANCE_READY, TRUE);

  return KRETURN_SUCCESS;

}				/* kSetInstanceReady() */

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kSetInstanceNotReady */
/* */
/*  Description: */
/*      Set a client instance to the not ready state. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      PCLIENT_INST pClient_Inst */
/*         Pointer to client instance */
/* */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */

KRETURN
kSetInstanceNotReady (allegro_devc * devc, PHWI phwi,
		      PCLIENT_INST pClient_Inst)
{
  DWORD dwClient = pClient_Inst->dwClient;

#ifdef K_DBG
  _Debug_Printf_Service ("kSetInstanceNotReady %X %x\n", phwi, pClient_Inst);
#endif
  /* */
  /* Fail if the DSP kernel has been unloaded */
  /* */

  if (phwi->dwFlags & HWI_FLAG_UNLOADED)
    return KRETURN_ERROR_UNLOADED;

  /* */
  /* Make sure client and an instance exist... */
  /* */

  if (dwClient >= NUMBER_OF_CLIENTS)
    return KRETURN_ERROR_GENERIC;

  if (!phwi->asClientTable[dwClient].dwReferenceCount)
    return KRETURN_ERROR_GENERIC;

  /* */
  /* Clear the instance ready flag */
  /* */

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA,
		 pClient_Inst->
		 dwDspDataClientArea + CDATA_INSTANCE_READY, FALSE);

  return KRETURN_SUCCESS;

}				/* kSetInstanceNotReady() */

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kStartTransfer */
/* */
/*  Description: */
/*      Start data transfer to/from client instance data memory. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      PCLIENT_INST pClient_Inst */
/*         Pointer to client instance */
/* */
/*      DWORD dwAutoRepeat */
/*         TRUE if auto-repeat buffer, FALSE if one-shot buffer */
/* */
/*      DWORD dwHostSrcBufferAddr */
/*         Host source buffer physical address */
/* */
/*      DWORD dwHostSrcBufferLen */
/*         Host source buffer length in bytes */
/* */
/*      DWORD dwHostDstBufferAddr */
/*         Host destination buffer linear address */
/* */
/*      DWORD dwHostDstBufferLen */
/*         Host destination buffer length in bytes */
/* */
/*      DWORD dwDSPInBufferAddr */
/*         DSP input buffer absolute address */
/* */
/*      DWORD dwDSPInBufferLen */
/*         DSP input buffer length in bytes */
/* */
/*      DWORD dwDSPOutBufferAddr */
/*         DSP output buffer absolute address */
/* */
/*      DWORD dwDSPOutBufferLen */
/*         DSP output buffer length in bytes */
/* */
/*      DWORD dwDSPInConnection */
/*         KCONNECT_XXX indicating input connection */
/* */
/*      DWORD dwDSPOutConnection */
/*         KCONNECT_XXX indicating output connection */
/* */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */

KRETURN kStartTransfer
  (allegro_devc * devc, PHWI phwi,
   PCLIENT_INST pClient_Inst,
   DWORD dwAutoRepeat,
   DWORD dwHostSrcBufferAddr,
   DWORD dwHostSrcBufferLen,
   DWORD dwHostDstBufferAddr,
   DWORD dwHostDstBufferLen,
   DWORD dwDSPInBufferAddr,
   DWORD dwDSPInBufferLen,
   DWORD dwDSPOutBufferAddr,
   DWORD dwDSPOutBufferLen, DWORD dwDSPInConnection, DWORD dwDSPOutConnection)
{
  DWORD dwClient = pClient_Inst->dwClient;
  DWORD dwDspDataClientArea = pClient_Inst->dwDspDataClientArea;

#ifdef K_DBG
  _Debug_Printf_Service
    ("kStartTransfer %X %X %X %X %X %X %X %X %X %X %X %X %X\n", phwi,
     pClient_Inst, dwAutoRepeat, dwHostSrcBufferAddr, dwHostSrcBufferLen,
     dwHostDstBufferAddr, dwHostDstBufferLen, dwDSPInBufferAddr,
     dwDSPInBufferLen, dwDSPOutBufferAddr, dwDSPOutBufferLen,
     dwDSPInConnection, dwDSPOutConnection);

#endif
  /* */
  /* Fail if the DSP kernel has been unloaded */
  /* */

  if (phwi->dwFlags & HWI_FLAG_UNLOADED)
    return KRETURN_ERROR_UNLOADED;


  if ((dwDSPInConnection == KCONNECT_DMA) ||
      (dwDSPOutConnection == KCONNECT_DMA))
    {
      /* */
      /* Do DMA synchronization */
      /* */
      /* (see discussion in kOpenInstance) */
      /* */

      if (!kStateExists (devc, phwi, KDATA_DMA_SWITCH, TRUE))
	return KRETURN_ERROR_BUSY;
    }

  /* */
  /* Make sure client and an instance exist... */
  /* */

  if (dwClient >= NUMBER_OF_CLIENTS)
    return KRETURN_ERROR_GENERIC;

  if (!phwi->asClientTable[dwClient].dwReferenceCount)
    return KRETURN_ERROR_GENERIC;

  /* */
  /* Verify input connection parameters */
  /* */

  if ((dwDSPInConnection != KCONNECT_NONE) &&
      (dwDSPInConnection != KCONNECT_SAME))
    {
      if (!dwDSPInBufferLen)
	return KRETURN_ERROR_GENERIC;

      /* DSP buffer length must be block multiple */

      if (dwDSPInBufferLen & (DMA_BLOCK_LENGTH - 1))
	return KRETURN_ERROR_GENERIC;

      if (dwDSPInConnection == KCONNECT_DMA)
	{
	  /* host buffer must be DWORD aligned */

	  if (dwHostSrcBufferAddr & (sizeof (DWORD) - 1))
	    return KRETURN_ERROR_GENERIC;

	  if (!dwHostSrcBufferLen)
	    return KRETURN_ERROR_GENERIC;
	}
    }

  /* */
  /* Verify output connection parameters */
  /* */

  if ((dwDSPOutConnection != KCONNECT_NONE) &&
      (dwDSPOutConnection != KCONNECT_SAME))
    {
      if (!dwDSPOutBufferLen)
	return KRETURN_ERROR_GENERIC;

      /* DSP buffer length must be block multiple */

      if (dwDSPOutBufferLen & (DMA_BLOCK_LENGTH - 1))
	return KRETURN_ERROR_GENERIC;

      if ((dwDSPOutConnection == KCONNECT_DMA) ||
	  (dwDSPOutConnection == KCONNECT_PIO))
	{
	  /* host buffer must be DWORD aligned */

	  if (dwHostDstBufferAddr & (sizeof (DWORD) - 1))
	    return KRETURN_ERROR_GENERIC;

	  if (!dwHostDstBufferLen)
	    return KRETURN_ERROR_GENERIC;
	}
    }

  pClient_Inst->dwHostSrcBufferAddr = dwHostSrcBufferAddr;
  pClient_Inst->dwHostSrcBufferLen = dwHostSrcBufferLen;
  pClient_Inst->dwHostDstBufferAddr = dwHostDstBufferAddr;
  pClient_Inst->dwHostDstBufferLen = dwHostDstBufferLen;
  pClient_Inst->dwHostDstCurrent = dwHostDstBufferAddr;
  pClient_Inst->dwDSPOutBufferAddr = dwDSPOutBufferAddr;
  pClient_Inst->dwDSPOutBufferLen = dwDSPOutBufferLen;
  pClient_Inst->dwDSPInConnection = dwDSPInConnection;
  pClient_Inst->dwDSPOutConnection = dwDSPOutConnection;

  /* */
  /* Write parameters to client instance memory */
  /* */

  if (dwDSPInConnection == KCONNECT_DMA)
    {
      /* host source */

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     dwDspDataClientArea + CDATA_HOST_SRC_ADDRL,
		     LOWORD (dwHostSrcBufferAddr));

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     dwDspDataClientArea + CDATA_HOST_SRC_ADDRH,
		     HIWORD (dwHostSrcBufferAddr));

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     dwDspDataClientArea + CDATA_HOST_SRC_END_PLUS_1L,
		     LOWORD (dwHostSrcBufferAddr + dwHostSrcBufferLen));

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     dwDspDataClientArea + CDATA_HOST_SRC_END_PLUS_1H,
		     HIWORD (dwHostSrcBufferAddr + dwHostSrcBufferLen));

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     dwDspDataClientArea + CDATA_HOST_SRC_CURRENTL,
		     LOWORD (dwHostSrcBufferAddr));

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     dwDspDataClientArea + CDATA_HOST_SRC_CURRENTH,
		     HIWORD (dwHostSrcBufferAddr));
    }

/*#pragma message("----Stream loopback not supported!") */

  else if (dwDSPOutConnection == KCONNECT_DMA)
    {
      /* host destination */

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     dwDspDataClientArea + CDATA_HOST_SRC_ADDRL,
		     LOWORD (dwHostDstBufferAddr));

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     dwDspDataClientArea + CDATA_HOST_SRC_ADDRH,
		     HIWORD (dwHostDstBufferAddr));

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     dwDspDataClientArea + CDATA_HOST_SRC_END_PLUS_1L,
		     LOWORD (dwHostDstBufferAddr + dwHostDstBufferLen));

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     dwDspDataClientArea + CDATA_HOST_SRC_END_PLUS_1H,
		     HIWORD (dwHostDstBufferAddr + dwHostDstBufferLen));

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     dwDspDataClientArea + CDATA_HOST_SRC_CURRENTL,
		     LOWORD (dwHostDstBufferAddr));

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     dwDspDataClientArea + CDATA_HOST_SRC_CURRENTH,
		     HIWORD (dwHostDstBufferAddr));
    }

  /* DSP input */

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA,
		 dwDspDataClientArea + CDATA_IN_BUF_BEGIN,
		 (WORD) dwDSPInBufferAddr);

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA,
		 dwDspDataClientArea + CDATA_IN_BUF_END_PLUS_1,
		 (WORD) (dwDSPInBufferAddr + (dwDSPInBufferLen / 2)));

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA,
		 dwDspDataClientArea + CDATA_IN_BUF_HEAD,
		 (WORD) dwDSPInBufferAddr);

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA,
		 dwDspDataClientArea + CDATA_IN_BUF_TAIL,
		 (WORD) dwDSPInBufferAddr);

  /* DSP output */

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA,
		 dwDspDataClientArea + CDATA_OUT_BUF_BEGIN,
		 (WORD) dwDSPOutBufferAddr);

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA,
		 dwDspDataClientArea + CDATA_OUT_BUF_END_PLUS_1,
		 (WORD) (dwDSPOutBufferAddr + (dwDSPOutBufferLen / 2)));

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA,
		 dwDspDataClientArea + CDATA_OUT_BUF_HEAD,
		 (WORD) dwDSPOutBufferAddr);

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA,
		 dwDspDataClientArea + CDATA_OUT_BUF_TAIL,
		 (WORD) dwDSPOutBufferAddr);

  /* connect the input and output */

  if (dwDSPInConnection == KCONNECT_DMA)
    {
      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     dwDspDataClientArea + CDATA_DMA_CONTROL,
		     (WORD) (dwAutoRepeat ?
			     (DMACONTROL_AUTOREPEAT + DMAC_PAGE3_SELECTOR +
			      DMAC_BLOCKF_SELECTOR) : (DMAC_PAGE3_SELECTOR +
						       DMAC_BLOCKF_SELECTOR)));
    }

/*#pragma message("----Stream loopback not supported!") */

  /* */
  /* The kernel currently does not support stream loopback. */
  /* Loopback means data is tranfered via DMA from the host to the */
  /* DSP, then the data is transfered via DMA from the DSP back to */
  /* the host.  While the data is in DSP memory it is filtered in */
  /* some way.  Thus the DSP acts as a hardware accelerator by */
  /* off-loading filter processing from the host processor. */
  /* */

  else if (dwDSPOutConnection == KCONNECT_DMA)
    {
      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     dwDspDataClientArea + CDATA_DMA_CONTROL,
		     (WORD) (dwAutoRepeat ?
			     (DMACONTROL_DIRECTION + DMACONTROL_AUTOREPEAT +
			      DMAC_PAGE3_SELECTOR +
			      DMAC_BLOCKF_SELECTOR) : (DMACONTROL_DIRECTION +
						       DMAC_PAGE3_SELECTOR +
						       DMAC_BLOCKF_SELECTOR)));
    }

  if (!kConnectInputOutput (devc, phwi,
			    dwDSPInConnection,
			    dwDSPOutConnection, dwDspDataClientArea))
    return KRETURN_ERROR_GENERIC;

#ifdef K_DBG
  _Debug_Printf_Service ("kStartTransfer OK\n");
#endif
  return KRETURN_SUCCESS;

}				/* kStartTransfer() */

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kStopTransfer */
/* */
/*  Description: */
/*      Stop data transfer to/from client instance data memory. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      PCLIENT_INST pClient_Inst */
/*         Pointer to client instance */
/* */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */

KRETURN
kStopTransfer (allegro_devc * devc, PHWI phwi, PCLIENT_INST pClient_Inst)
{
  DWORD dwClient = pClient_Inst->dwClient;

#ifdef K_DBG
  _Debug_Printf_Service ("kStopTransfer %X %X\n", phwi, pClient_Inst);
#endif
  /* */
  /* Fail if the DSP kernel has been unloaded */
  /* */

  if (phwi->dwFlags & HWI_FLAG_UNLOADED)
    return KRETURN_ERROR_UNLOADED;

  /* */
  /* Make sure client and an instance exist... */
  /* */

  if (dwClient >= NUMBER_OF_CLIENTS)
    return KRETURN_ERROR_GENERIC;

  if (!phwi->asClientTable[dwClient].dwReferenceCount)
    return KRETURN_ERROR_GENERIC;

  /* */
  /* disconnect the input and output */
  /* */

  if (!kDisconnectInputOutput (devc, phwi,
			       pClient_Inst->dwDSPInConnection,
			       pClient_Inst->dwDSPOutConnection,
			       pClient_Inst->dwDspDataClientArea))
    return KRETURN_ERROR_GENERIC;

#ifdef K_DBG
  _Debug_Printf_Service ("kStopTransfer OK\n");
#endif
  return KRETURN_SUCCESS;

}				/* kStopTransfer() */

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kAlterTransfer */
/* */
/*  Description: */
/*      Alter a client's DMA transfer. */
/* */
/*      When altering a client's current stream position, a position */
/*      equal to 0 indicates the first byte in the buffer.  A position */
/*      equal to dwHostXXXBufferLen-1 indicates the last byte in the */
/*      buffer. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      PCLIENT_INST pClient_Inst */
/*         Pointer to client instance */
/* */
/*      DWORD dwFlags */
/*         KALTER_XXX flags indicating alterations to perform */
/* */
/*      DWORD dwAutoRepeat */
/*         TRUE if auto-repeat buffer, FALSE if one-shot buffer */
/* */
/*      DWORD dwPosition */
/*         The byte position to set */
/* */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */

KRETURN kAlterTransfer
  (allegro_devc * devc, PHWI phwi,
   PCLIENT_INST pClient_Inst,
   DWORD dwFlags, DWORD dwAutoRepeat, DWORD dwPosition)
{
  DWORD dwClient = pClient_Inst->dwClient;
  WORD wDmaControl;

#ifdef K_DBG
  _Debug_Printf_Service ("kAlterTransfer %X %X %X %X %X\n", phwi,
			 pClient_Inst, dwFlags, dwAutoRepeat, dwPosition);
#endif
  /* */
  /* Fail if the DSP kernel has been unloaded */
  /* */

  if (phwi->dwFlags & HWI_FLAG_UNLOADED)
    return KRETURN_ERROR_UNLOADED;

  /* */
  /* Make sure client and an instance exist... */
  /* */

  if (dwClient >= NUMBER_OF_CLIENTS)
    return KRETURN_ERROR_GENERIC;

  if (!phwi->asClientTable[dwClient].dwReferenceCount)
    return KRETURN_ERROR_GENERIC;

  /* */
  /* Make sure DSP kernel is ready to be halted */
  /* */

  if (!kStateExists (devc, phwi, KDATA_HALT_ACKNOWLEDGE, FALSE))
    return KRETURN_ERROR_BUSY;

  /* */
  /* halt the DSP kernel and wait for an acknowledgement */
  /* */

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA, KDATA_HALT_SYNCH_DMA, TRUE);

  if (!kStateExists (devc, phwi, KDATA_HALT_ACKNOWLEDGE, TRUE))
    {
      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA, KDATA_HALT_SYNCH_DMA, FALSE);

      return KRETURN_ERROR_BUSY;
    }

  /* */
  /* DSP kernel has halted, do DMA alterations */
  /* */
  /* Note: We halt the DSP in order to synchronize the alterations */
  /* with the DMA state machine */
  /* */

  if (dwFlags & KALTER_AUTOREPEAT)
    {
      wDmaControl = kDspReadWord (devc, phwi->dwBaseIO,
				  MEMTYPE_INTERNAL_DATA,
				  pClient_Inst->dwDspDataClientArea +
				  CDATA_DMA_CONTROL);

      wDmaControl &= ~(DMACONTROL_STOPPED | DMACONTROL_AUTOREPEAT);

      if (dwAutoRepeat)
	wDmaControl |= DMACONTROL_AUTOREPEAT;

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     pClient_Inst->dwDspDataClientArea + CDATA_DMA_CONTROL,
		     wDmaControl);
    }

  if (dwFlags & KALTER_POSITION)
    {
      /* */
      /* Position alterations are only permitted on playback streams... */
      /* */

      if (pClient_Inst->dwDSPInConnection == KCONNECT_DMA)
	{
	  dwPosition += pClient_Inst->dwHostSrcBufferAddr;

	  kDspWriteWord (devc, phwi->dwBaseIO,
			 MEMTYPE_INTERNAL_DATA,
			 pClient_Inst->dwDspDataClientArea +
			 CDATA_HOST_SRC_CURRENTL, LOWORD (dwPosition));

	  kDspWriteWord (devc, phwi->dwBaseIO,
			 MEMTYPE_INTERNAL_DATA,
			 pClient_Inst->dwDspDataClientArea +
			 CDATA_HOST_SRC_CURRENTH, HIWORD (dwPosition));
	}
    }

  /* */
  /* all done, reset halt request flag to resume DSP kernel execution */
  /* */

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA, KDATA_HALT_SYNCH_DMA, FALSE);

  return KRETURN_SUCCESS;

}				/* kAlterTransfer() */

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kSwitchPINConnection */
/* */
/*  Description: */
/*      Switch data transfer PIN connection to/from client instance data memory. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      PCLIENT_INST pClient_Inst */
/*         Pointer to client instance */
/* */
/*      DWORD dwDSPInConnection */
/*         KCONNECT_XXX indicating input connection */
/* */
/*      DWORD dwDSPOutConnection */
/*         KCONNECT_XXX indicating output connection */
/* */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */

KRETURN kSwitchPINConnection
  (allegro_devc * devc, PHWI phwi,
   PCLIENT_INST pClient_Inst,
   DWORD dwDSPInConnection, DWORD dwDSPOutConnection)
{
  DWORD dwClient = pClient_Inst->dwClient;
  DWORD dwDspDataClientArea = pClient_Inst->dwDspDataClientArea;
#ifdef K_DBG
  _Debug_Printf_Service ("kSwitchPINConnection %X %X %X %X \n",
			 phwi,
			 pClient_Inst, dwDSPInConnection, dwDSPOutConnection);

#endif

  /* */
  /* Fail if the DSP kernel has been unloaded */
  /* */

  if (phwi->dwFlags & HWI_FLAG_UNLOADED)
    return KRETURN_ERROR_UNLOADED;

  /* */
  /* Make sure connections are valid... */
  /* */

  if (dwDSPInConnection >= NUMBER_OF_CONNECTIONS)
    return KRETURN_ERROR_GENERIC;

  if (dwDSPOutConnection >= NUMBER_OF_CONNECTIONS)
    return KRETURN_ERROR_GENERIC;

  if ((dwDSPInConnection == KCONNECT_DMA) ||
      (dwDSPOutConnection == KCONNECT_DMA))
    {
      /* */
      /* Do DMA synchronization */
      /* */
      /* DMA engine is not switchable at this moment!!! */
      /* */

      if (!kStateExists (devc, phwi, KDATA_DMA_SWITCH, TRUE))
	return KRETURN_ERROR_BUSY;
    }

  /* */
  /* Make sure client and an instance exist... */
  /* */

  if (dwClient >= NUMBER_OF_CLIENTS)
    return KRETURN_ERROR_GENERIC;

  if (!phwi->asClientTable[dwClient].dwReferenceCount)
    return KRETURN_ERROR_GENERIC;


  /* */
  /* disconnect the input and output first */
  /* */

  if (!kDisconnectInputOutput (devc, phwi,
			       pClient_Inst->dwDSPInConnection,
			       pClient_Inst->dwDSPOutConnection,
			       pClient_Inst->dwDspDataClientArea))
    return KRETURN_ERROR_GENERIC;


  /* */
  /*  update PIN connection  */
  /* */

  if (dwDSPInConnection != KCONNECT_SAME)
    pClient_Inst->dwDSPInConnection = dwDSPInConnection;

  if (dwDSPOutConnection != KCONNECT_SAME)
    pClient_Inst->dwDSPOutConnection = dwDSPOutConnection;


  /* */
  /*  Finally, connect updated PIN  */
  /* */

  if (!kConnectInputOutput (devc, phwi,
			    pClient_Inst->dwDSPInConnection,
			    pClient_Inst->dwDSPOutConnection,
			    dwDspDataClientArea))
    return KRETURN_ERROR_GENERIC;


#ifdef K_DBG
  _Debug_Printf_Service ("kSwitchPINConnection OK\n");
#endif
  return KRETURN_SUCCESS;

}				/* kSwitchPINConnection() */

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kQueryActivity */
/* */
/*  Description: */
/*      Indicate what clients and/or connections are active */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      PDWORD pdwClientMasks */
/*         Pointer to DWORD that will contain MASK_CLIENT_XXX masks */
/* */
/*      PDWORD pdwConnectMasks */
/*         Pointer to DWORD that will contain MASK_KCONNECT_XXX masks */
/* */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */

KRETURN kQueryActivity
  (allegro_devc * devc, PHWI phwi, PDWORD pdwClientMasks,
   PDWORD pdwConnectMasks)
{
  DWORD dwClient;

#ifdef K_DBG
  _Debug_Printf_Service ("kQueryActivity %X %X %X\n", phwi,
			 pdwClientMasks, pdwConnectMasks);


#endif
  /* */
  /* Fail if the DSP kernel has been unloaded */
  /* */

  if (phwi->dwFlags & HWI_FLAG_UNLOADED)
    return KRETURN_ERROR_UNLOADED;

  /* */
  /* Check if any client instance is defined */
  /* */

  *pdwClientMasks = 0;

  for (dwClient = 0; dwClient < NUMBER_OF_CLIENTS; ++dwClient)
    {
      if (phwi->asClientTable[dwClient].dwReferenceCount)
	{
	  *pdwClientMasks |= (1 << dwClient);
	}
    }

  /* */
  /* Check if any connections are defined */
  /* */

  *pdwConnectMasks = 0;

  if (phwi->awVirtualDMAList[0])
    *pdwConnectMasks |= MASK_KCONNECT_DMA;

  if (phwi->awVirtualADC1List[0])
    *pdwConnectMasks |= MASK_KCONNECT_ADC1;

  if (phwi->awVirtualADC2List[0])
    *pdwConnectMasks |= MASK_KCONNECT_ADC2;

  if (phwi->awVirtualI2SList[0])
    *pdwConnectMasks |= MASK_KCONNECT_I2S;

  if (phwi->awVirtualCHIList[0])
    *pdwConnectMasks |= MASK_KCONNECT_CHI;

  if (kDspReadWord
      (devc, phwi->dwBaseIO, MEMTYPE_INTERNAL_DATA, KDATA_SPDIF_XFER))
    *pdwConnectMasks |= MASK_KCONNECT_SPDIF;

  if (phwi->awVirtualMIXERList[0])
    *pdwConnectMasks |= MASK_KCONNECT_MIXER;

#if 0
#if (NUMBER_OF_CONNECTIONS != 0x10)
#error Assumption about number of connections failed.
#endif
#endif

  return KRETURN_SUCCESS;

}				/* kQueryActivity() */

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kSetTimer */
/* */
/*  Description: */
/*      Set the DSP to Host interrupt time interval */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      DWORD dwTimeInterval */
/*         Number of APU interrupts per DSP to Host interrupt */
/* */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */

KRETURN
kSetTimer (allegro_devc * devc, PHWI phwi, DWORD dwTimeInterval)
{
#ifdef K_DBG
  _Debug_Printf_Service ("kSetTimer %X %X\n", phwi, dwTimeInterval);
#endif
  /* */
  /* Fail if the DSP kernel has been unloaded */
  /* */

  if (phwi->dwFlags & HWI_FLAG_UNLOADED)
    return KRETURN_ERROR_UNLOADED;

  /* */
  /* Check time interval size */
  /* */

  if (HIWORD (dwTimeInterval))
    return KRETURN_ERROR_GENERIC;

  /* */
  /* Write timer values */
  /* */

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA,
		 KDATA_TIMER_COUNT_RELOAD, (WORD) dwTimeInterval);

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA,
		 KDATA_TIMER_COUNT_CURRENT, (WORD) dwTimeInterval);

  return KRETURN_SUCCESS;

}				/* kSetTimer() */

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kOpenPassThru */
/* */
/*  Description: */
/*      Open a pass through. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      PPASSTHRU *ppPassThru */
/*         Pointer to PPASSTHRU that will contain the pass through pointer */
/* */
/*      DWORD dwDSPInConnection */
/*         KCONNECT_XXX indicating input connection */
/* */
/*      DWORD dwDSPOutConnection */
/*         KCONNECT_XXX indicating output connection */
/* */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */

KRETURN kOpenPassThru
  (allegro_devc * devc, PHWI phwi,
   PPASSTHRU * ppPassThru, DWORD dwDSPInConnection, DWORD dwDSPOutConnection)
{
  PPASSTHRU pPassThru = NULL;

#ifdef K_DBG
  _Debug_Printf_Service ("kOpenPassThru %X %X <-> %X\n", phwi,
			 dwDSPInConnection, dwDSPOutConnection);
#endif
  /* */
  /* Fail if the DSP kernel has been unloaded */
  /* */

  if (phwi->dwFlags & HWI_FLAG_UNLOADED)
    return KRETURN_ERROR_UNLOADED;

  /* */
  /* Make sure connections are valid... */
  /* */

  if ((dwDSPInConnection == KCONNECT_NONE) ||
      (dwDSPInConnection == KCONNECT_SAME) ||
      (dwDSPInConnection >= NUMBER_OF_CONNECTIONS))
    return KRETURN_ERROR_GENERIC;

  if ((dwDSPOutConnection == KCONNECT_NONE) ||
      (dwDSPOutConnection == KCONNECT_SAME) ||
      (dwDSPOutConnection >= NUMBER_OF_CONNECTIONS))
    return KRETURN_ERROR_GENERIC;

  /* */
  /* Do open/close synchronization */
  /* */

  if (!kStateExists (devc, phwi, KDATA_TASK_SWITCH, TRUE))
    return KRETURN_ERROR_BUSY;

  /* */
  /* Allocate host memory */
  /* */

  if (!(pPassThru = (PPASSTHRU)
	_HeapAllocate (sizeof (PASSTHRU), HEAPZEROINIT)))
    return KRETURN_ERROR_GENERIC;

  pPassThru->dwDSPInConnection = dwDSPInConnection;
  pPassThru->dwDSPOutConnection = dwDSPOutConnection;

  /* */
  /* Handle directly mixed (i.e. non-buffered) connections */
  /* */

  if (dwDSPOutConnection == KCONNECT_SPDIF)
    {
      kConnectDirectMixer (devc, phwi, dwDSPInConnection, dwDSPOutConnection,
			   TRUE);
    }
  else
    {
      if (dwDSPOutConnection == KCONNECT_MIXER)
	{
	  switch (dwDSPInConnection)
	    {
	    case KCONNECT_ADC1:
	      kDspWriteWord (devc, phwi->dwBaseIO,
			     MEMTYPE_INTERNAL_DATA,
			     KDATA_ADC1_MIXER_REQUEST, TRUE);
	      break;
	    case KCONNECT_ADC2:
	      kDspWriteWord (devc, phwi->dwBaseIO,
			     MEMTYPE_INTERNAL_DATA,
			     KDATA_ADC2_MIXER_REQUEST, TRUE);
	      break;

	    case KCONNECT_CD:
	      kDspWriteWord (devc, phwi->dwBaseIO,
			     MEMTYPE_INTERNAL_DATA,
			     KDATA_CD_MIXER_REQUEST, TRUE);
	      break;

	    case KCONNECT_MIC:
	      kDspWriteWord (devc, phwi->dwBaseIO,
			     MEMTYPE_INTERNAL_DATA,
			     KDATA_MIC_MIXER_REQUEST, TRUE);
	      break;

	    default:
	      goto kOpen_Error;
	    }			/* endswitch */
	}			/* endif */

    }

  /* */
  /* Everything worked */
  /* */

  *ppPassThru = pPassThru;

#ifdef K_DBG
  _Debug_Printf_Service ("kOpenPassThru OK %X\n", pPassThru);
#endif
  return KRETURN_SUCCESS;

kOpen_Error:

  /* */
  /* Failure, undo all that was done... */
  /* */

  if (pPassThru)
    {
      _HeapFree (pPassThru, 0);
    }

#ifdef K_DBG
  _Debug_Printf_Service ("kOpenPassThru Failed\n");
#endif
  return KRETURN_ERROR_GENERIC;

}				/* kOpenPassThru() */

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kClosePassThru */
/* */
/*  Description: */
/*      Close a pass through. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      PPASSTHRU pPassThru */
/*         Pointer to pass through */
/* */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */

KRETURN
kClosePassThru (allegro_devc * devc, PHWI phwi, PPASSTHRU pPassThru)
{
#ifdef K_DBG
  _Debug_Printf_Service ("kClosePassThru %X %X\n", phwi, pPassThru);
#endif
  /* */
  /* Fail if the DSP kernel has been unloaded */
  /* */

  if (phwi->dwFlags & HWI_FLAG_UNLOADED)
    return KRETURN_ERROR_UNLOADED;

  /* */
  /* Deallocate memory */
  /* */

  if (!pPassThru)
    return KRETURN_ERROR_GENERIC;

  /* */
  /* Handle directly mixed (i.e. non-buffered) connections */
  /* */

  if (pPassThru->dwDSPOutConnection == KCONNECT_SPDIF)
    {
      kConnectDirectMixer (devc, phwi,
			   pPassThru->dwDSPInConnection,
			   pPassThru->dwDSPOutConnection, FALSE);
    }
  else
    {
      if (pPassThru->dwDSPOutConnection == KCONNECT_MIXER)
	{
	  switch (pPassThru->dwDSPInConnection)
	    {
	    case KCONNECT_ADC1:
	      kDspWriteWord (devc, phwi->dwBaseIO,
			     MEMTYPE_INTERNAL_DATA,
			     KDATA_ADC1_MIXER_REQUEST, FALSE);
	      break;

	    case KCONNECT_ADC2:
	      kDspWriteWord (devc, phwi->dwBaseIO,
			     MEMTYPE_INTERNAL_DATA,
			     KDATA_ADC2_MIXER_REQUEST, FALSE);
	      break;

	    case KCONNECT_CD:
	      kDspWriteWord (devc, phwi->dwBaseIO,
			     MEMTYPE_INTERNAL_DATA,
			     KDATA_CD_MIXER_REQUEST, FALSE);
	      break;

	    case KCONNECT_MIC:
	      kDspWriteWord (devc, phwi->dwBaseIO,
			     MEMTYPE_INTERNAL_DATA,
			     KDATA_MIC_MIXER_REQUEST, FALSE);
	      break;

	    }			/* endswitch */

	}			/* endif */

    }

  _HeapFree (pPassThru, 0);

  /* */
  /* Do open/close synchronization */
  /* */

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA, KDATA_TASK_SWITCH, FALSE);

  return KRETURN_SUCCESS;

}				/* kClosePassThru() */

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kSetVolume */
/* */
/*  Description: */
/*      Set a client's current stream left/right Volume. */
/*      Maximum is 0x7FFF and Minimum is 0x0000. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      PCLIENT_INST pClient_Inst */
/*         Pointer to client instance */
/* */
/*      WORD wLeftVolume */
/*         16 bit constant for left channel volume */
/* */
/*      WORD wRightVolume */
/*         16 bit constant for right channel volume */
/* */
/*		WORD wBoosterMode */
/*		   TRUE activates a broader range of volume setting.   */
/*				In this mode, 7FFF represents 18dB gain */
/*							  1000 represents  0dB gain */
/*							     0 represents -Inf gain(mute)  */
/*		   FALSE supports only an attenuation mode. */
/*				In this mode, 7FFF represents  0dB gain				  	 */
/*								 0 represents -Inf gain(mute) */
/* */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */

KRETURN kSetVolume
  (allegro_devc * devc, PHWI phwi,
   PCLIENT_INST pClient_Inst,
   WORD wLeftVolume, WORD wRightVolume, WORD wBoosterMode)
{
  DWORD dwClient = pClient_Inst->dwClient;

#if 0
  WORD wCurLVol, wCurRVol, wLTemp, wRTemp;
#endif

#ifdef K_DBG
  _Debug_Printf_Service ("kSetVolume %X %X %X %X\n",
			 phwi, pClient_Inst, wLeftVolume, wRightVolume);
#endif
  /* */
  /* Fail if the DSP kernel has been unloaded */
  /* */

  if (phwi->dwFlags & HWI_FLAG_UNLOADED)
    return KRETURN_ERROR_UNLOADED;

  /* */
  /* Make sure client and an instance exist... */
  /* */

  if (dwClient >= NUMBER_OF_CLIENTS)
    return KRETURN_ERROR_GENERIC;

  if (!phwi->asClientTable[dwClient].dwReferenceCount)
    return KRETURN_ERROR_GENERIC;

#if 0				/* micro step the volume */
  wCurLVol = kDspReadWord (devc, phwi->dwBaseIO,
			   MEMTYPE_INTERNAL_DATA,
			   pClient_Inst->
			   dwDspDataClientArea + CDATA_LEFT_VOLUME);

  wCurRVol = kDspReadWord (devc, phwi->dwBaseIO,
			   MEMTYPE_INTERNAL_DATA,
			   pClient_Inst->
			   dwDspDataClientArea + CDATA_RIGHT_VOLUME);

  /* need to do micro steps to avoid zipper noise */
  /* max 7 steps */
  wLTemp = wLeftVolume;
  wRTemp = wRightVolume;


  wCurLVol &= 0xF000;
  wCurLVol |= (wLeftVolume & 0x0FFF);

  if (wCurLVol < wLeftVolume)
    {
      wLeftVolume = wCurLVol;
      wCurLVol = wLTemp;
    }				/* endif */

  do
    {

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     pClient_Inst->
		     dwDspDataClientArea + CDATA_LEFT_VOLUME, wCurLVol);

      kDelayNMicroseconds (10);
      wCurLVol -= 0x800;
    }
  while (wCurLVol > wLeftVolume);

  wCurRVol &= 0xF000;
  wCurRVol |= (wRightVolume & 0x0FFF);

  if (wCurRVol < wRightVolume)
    {
      wRightVolume = wCurRVol;
      wCurRVol = wRTemp;
    }				/* endif */

  do
    {

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     pClient_Inst->
		     dwDspDataClientArea + CDATA_RIGHT_VOLUME, wCurRVol);

      kDelayNMicroseconds (10);
      wCurRVol -= 0x800;
    }
  while (wCurRVol > wRightVolume);

  wLeftVolume = wLTemp;
  wRightVolume = wRTemp;
#endif
  /* */
  /* write left/right word of current stream volume */
  /* */

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA,
		 pClient_Inst->
		 dwDspDataClientArea + CDATA_LEFT_VOLUME, -wLeftVolume);

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA,
		 pClient_Inst->
		 dwDspDataClientArea + CDATA_RIGHT_VOLUME, -wRightVolume);

  /* */
  /* tell the client whether this is broader range  */
  /* of volume setting or not */
  /* */

  if (wBoosterMode)
    kDspWriteWord (devc, phwi->dwBaseIO,
		   MEMTYPE_INTERNAL_DATA,
		   pClient_Inst->
		   dwDspDataClientArea + CDATA_HEADER_LEN + 23, TRUE);
  else
    kDspWriteWord (devc, phwi->dwBaseIO,
		   MEMTYPE_INTERNAL_DATA,
		   pClient_Inst->
		   dwDspDataClientArea + CDATA_HEADER_LEN + 23, FALSE);


  return KRETURN_SUCCESS;

}				/* kSetVolume() */

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kSetRearVolume */
/* */
/*  Description: */
/*      Set a client's current stream rear left/rear right Volume. */
/*      Maximum is 0x7FFF and Minimum is 0x0000. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      PCLIENT_INST pClient_Inst */
/*         Pointer to client instance */
/* */
/*      WORD wLeftRearVolume */
/*         16 bit constant for rear left channel volume */
/* */
/*      WORD wRightRearVolume */
/*         16 bit constant for rear right channel volume */
/* */
/*		 */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */

KRETURN kSetRearVolume
  (allegro_devc * devc, PHWI phwi,
   PCLIENT_INST pClient_Inst, WORD wLeftRearVolume, WORD wRightRearVolume)
{
  DWORD dwClient = pClient_Inst->dwClient;


#ifdef K_DBG
  _Debug_Printf_Service ("kSetVolume %X %X %X %X\n",
			 phwi,
			 pClient_Inst, wLeftRearVolume, wRightRearVolume);
#endif
  /* */
  /* Fail if the DSP kernel has been unloaded */
  /* */

  if (phwi->dwFlags & HWI_FLAG_UNLOADED)
    return KRETURN_ERROR_UNLOADED;

  /* */
  /* Make sure client and an instance exist... */
  /* */

  if (dwClient >= NUMBER_OF_CLIENTS)
    return KRETURN_ERROR_GENERIC;

  if (!phwi->asClientTable[dwClient].dwReferenceCount)
    return KRETURN_ERROR_GENERIC;

  /* */
  /* write rear left/rear right word of current stream volume */
  /* */

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA,
		 pClient_Inst->
		 dwDspDataClientArea + CDATA_LEFT_SUR_VOL, wLeftRearVolume);

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA,
		 pClient_Inst->
		 dwDspDataClientArea + CDATA_RIGHT_SUR_VOL, wRightRearVolume);

  return KRETURN_SUCCESS;

}				/* kSetRearVolume() */

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kSetPassThruVolume */
/* */
/*  Description: */
/*      Set a passthrough instance volume. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      PPASSTHRU *ppPassThru */
/*         Pointer to PPASSTHRU that will contain the pass through pointer */
/* */
/*      WORD wLeftVolume */
/*         16 bit constant for left channel volume */
/* */
/*      WORD wRightVolume */
/*         16 bit constant for right channel volume */
/* */
/*	 Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */
KRETURN kSetPassThruVolume
  (allegro_devc * devc, PHWI phwi, PPASSTHRU pPassThru, WORD wLeftVolume,
   WORD wRightVolume)
{
  /* */
  /* Fail if the DSP kernel has been unloaded */
  /* */

  if (phwi->dwFlags & HWI_FLAG_UNLOADED)
    return KRETURN_ERROR_UNLOADED;

  /*  pPassThru -> wLeftVolume = wLeftVolume ; */
  /*  pPassThru -> wRightVolume = wRightVolume ; */

  switch (pPassThru->dwDSPInConnection)
    {
    case KCONNECT_ADC1:
      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     KDATA_ADC1_LEFT_VOLUME, wLeftVolume);

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     KDATA_ADC1_RIGHT_VOLUME, wRightVolume);

      break;

    case KCONNECT_ADC2:
      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     KDATA_ADC2_LEFT_VOLUME, wLeftVolume);

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     KDATA_ADC2_RIGHT_VOLUME, wRightVolume);

      break;

    case KCONNECT_CD:
      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     KDATA_CD_LEFT_VOLUME, wLeftVolume);

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     KDATA_CD_RIGHT_VOLUME, wRightVolume);
      break;

    case KCONNECT_MIC:
      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA, KDATA_MIC_VOLUME, wLeftVolume);

      break;

    default:
      return KRETURN_ERROR_GENERIC;
    }				/* endswitch */


  return KRETURN_SUCCESS;

}				/* kSetPassThruVolume */

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kSetPassThruRearVolume */
/* */
/*  Description: */
/*      Set a passthrough instance's rear volume. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      PPASSTHRU *ppPassThru */
/*         Pointer to PPASSTHRU that will contain the pass through pointer */
/* */
/*      WORD wLeftRearVolume */
/*         16 bit constant for left rear channel volume */
/* */
/*      WORD wRightRearVolume */
/*         16 bit constant for right rear channel volume */
/* */
/*	 Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */
KRETURN kSetPassThruRearVolume
  (allegro_devc * devc, PHWI phwi,
   PPASSTHRU pPassThru, WORD wLeftRearVolume, WORD wRightRearVolume)
{
  /* */
  /* Fail if the DSP kernel has been unloaded */
  /* */
  if (phwi->dwFlags & HWI_FLAG_UNLOADED)
    return KRETURN_ERROR_UNLOADED;

  switch (pPassThru->dwDSPInConnection)
    {
    case KCONNECT_ADC1:
      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     KDATA_ADC1_LEFT_SUR_VOL, wLeftRearVolume);

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     KDATA_ADC1_RIGHT_SUR_VOL, wRightRearVolume);

      break;


    case KCONNECT_ADC2:
      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     KDATA_ADC2_LEFT_SUR_VOL, wLeftRearVolume);

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     KDATA_ADC2_RIGHT_SUR_VOL, wRightRearVolume);

      break;

    case KCONNECT_CD:
      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     KDATA_CD_LEFT_SUR_VOL, wLeftRearVolume);

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     KDATA_CD_RIGHT_SUR_VOL, wRightRearVolume);
      break;

    case KCONNECT_MIC:
      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     KDATA_MIC_SUR_VOL, wLeftRearVolume);

      break;

    default:
      return KRETURN_ERROR_GENERIC;
    }				/* endswitch */


  return KRETURN_SUCCESS;

}				/* kSetPassThruRearVolume */


/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kSetMasterVolume */
/* */
/*  Description: */
/*      Set master volume. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      WORD wLeftVolume */
/*         16 bit constant for left channel volume */
/* */
/*      WORD wRightVolume */
/*         16 bit constant for right channel volume */
/* */
/*	 Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */
KRETURN
kSetMasterVolume (allegro_devc * devc, PHWI phwi, WORD wLeftVolume,
		  WORD wRightVolume)
{
  /* */
  /* Fail if the DSP kernel has been unloaded */
  /* */
  if (phwi->dwFlags & HWI_FLAG_UNLOADED)
    return KRETURN_ERROR_UNLOADED;

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA, KDATA_DAC_LEFT_VOLUME, wLeftVolume);

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA, KDATA_DAC_RIGHT_VOLUME, wRightVolume);

  return KRETURN_SUCCESS;

}				/* kSetMasterVolume */

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kSetFrequency */
/* */
/*  Description: */
/*      Set a client's current stream sampling frequency. */
/*      48khz is corresponding to 0x7FFF */
/*      0 hz is corresponding to 0x0000 */
/*      The other frequencies are linear mapping between those two */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      PCLIENT_INST pClient_Inst */
/*         Pointer to client instance */
/* */
/*      WORD wFrequency */
/*         16 bit constant for sampling frequency */
/* */
/* */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */

KRETURN
kSetFrequency (allegro_devc * devc, PHWI phwi, PCLIENT_INST pClient_Inst,
	       WORD wFrequency)
{
  DWORD dwClient = pClient_Inst->dwClient;
#ifdef K_DBG
  _Debug_Printf_Service ("kSetVolume %X %X %X\n", phwi,
			 pClient_Inst, wFrequency);
#endif


  /* */
  /* Fail if the DSP kernel has been unloaded */
  /* */

  if (phwi->dwFlags & HWI_FLAG_UNLOADED)
    return KRETURN_ERROR_UNLOADED;

  /* */
  /* Make sure client and an instance exist... */
  /* */

  if (dwClient >= NUMBER_OF_CLIENTS)
    return KRETURN_ERROR_GENERIC;

  if (!phwi->asClientTable[dwClient].dwReferenceCount)
    return KRETURN_ERROR_GENERIC;

  /* */
  /* write left/right word of current stream sampling frequency */
  /* */

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA,
		 pClient_Inst->
		 dwDspDataClientArea + CDATA_FREQUENCY, wFrequency);

  return KRETURN_SUCCESS;

}				/* kSetFrequency */


/**************************************************************************** */
/* The following is unique to M2/M2E only */
/**************************************************************************** */
#ifdef PIOREC
/*-------------------------------------------------------------------------- */
/* */
/*  DWORD kCopyLinearToCircular */
/* */
/*  Description: */
/*      Copy data from a linear DSP output buffer to a potentially */
/*      circular host destination buffer. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      DWORD dwMemAddr */
/*         Memory address to read from. */
/* */
/*      DWORD dwMemLen */
/*         Number of WORDs to read from specified address. */
/* */
/*      DWORD dwHostAddr */
/*         Host memory address to write to. */
/* */
/*      DWORD dwHostBase */
/*         Base address of host buffer. */
/* */
/*      DWORD dwHostLen */
/*         Length in bytes of host buffer. */
/* */
/*  Return (DWORD): */
/* */
/*-------------------------------------------------------------------------- */

DWORD kCopyLinearToCircular
  (allegro_devc * devc, PHWI phwi,
   DWORD dwMemAddr,
   DWORD dwMemLen, DWORD dwHostAddr, DWORD dwHostBase, DWORD dwHostLen)
{
  DWORD dwPreWrapLen;

  /* */
  /* copy DSP buffer data to host buffer */
  /* */

  if ((dwHostAddr + (dwMemLen * sizeof (WORD))) <= (dwHostBase + dwHostLen))
    {
      /* */
      /* host buffer won't wrap, do straight copy */
      /* */

      kDspReadWords (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     dwMemAddr, dwMemLen, (PWORD) dwHostAddr);
    }
  else
    {
      dwPreWrapLen = (dwHostBase + dwHostLen - dwHostAddr) / sizeof (WORD);

      /* */
      /* host buffer will wrap, do pre-wrap copy */
      /* */

      kDspReadWords (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     dwMemAddr, dwPreWrapLen, (PWORD) dwHostAddr);

      /* */
      /* do post-wrap copy */
      /* */

      kDspReadWords (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA,
		     dwMemAddr + dwPreWrapLen,
		     dwMemLen - dwPreWrapLen, (PWORD) dwHostBase);
    }

  /* */
  /* update current host pointer and check for wrap */
  /* */

  dwHostAddr += (dwMemLen * sizeof (WORD));

  if (dwHostAddr >= (dwHostBase + dwHostLen))
    {
      dwHostAddr = dwHostBase + (dwHostAddr - (dwHostBase + dwHostLen));
    }

  return dwHostAddr;

}				/* kCopyLinearToCircular() */

/*-------------------------------------------------------------------------- */
/* */
/*  VOID kPIOInterruptHandler */
/* */
/*  Description: */
/*      PIO data available interrupt handler. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      PCLIENT_INST pClient_Inst */
/*         Pointer to client instance */
/* */
/*  Return (VOID): */
/* */
/*-------------------------------------------------------------------------- */

VOID
kPIOInterruptHandler (allegro_devc * devc, PHWI phwi,
		      PCLIENT_INST pClient_Inst)
{
  DWORD dwHostDstBufferAddr = pClient_Inst->dwHostDstBufferAddr;
  DWORD dwHostDstBufferLen = pClient_Inst->dwHostDstBufferLen;
  DWORD dwHostDstCurrent = pClient_Inst->dwHostDstCurrent;
  DWORD dwDSPOutBufferAddr = pClient_Inst->dwDSPOutBufferAddr;
  DWORD dwDSPOutBufferLen = pClient_Inst->dwDSPOutBufferLen;
  DWORD dwOutBufHead;
  DWORD dwOutBufTail;

  /* */
  /* return if PIO recording not active */
  /* */

  if (!phwi->awVirtualPIOList[0])
    return;

  /* */
  /* read buffer state from DSP */
  /* */

  dwOutBufHead = kDspReadWord (devc, phwi->dwBaseIO,
			       MEMTYPE_INTERNAL_DATA,
			       pClient_Inst->dwDspDataClientArea +
			       CDATA_OUT_BUF_HEAD);

  dwOutBufTail = kDspReadWord (devc, phwi->dwBaseIO,
			       MEMTYPE_INTERNAL_DATA,
			       pClient_Inst->dwDspDataClientArea +
			       CDATA_OUT_BUF_TAIL);

  /* */
  /* copy DSP buffer data to host buffer */
  /* */

  if (dwOutBufHead > dwOutBufTail)
    {
      /* */
      /* DSP buffer data doesn't wrap, do straight copy */
      /* */

      dwHostDstCurrent =
	kCopyLinearToCircular (devc, phwi,
			       dwOutBufTail,
			       dwOutBufHead - dwOutBufTail,
			       dwHostDstCurrent,
			       dwHostDstBufferAddr, dwHostDstBufferLen);
    }
  else
    {
      /* */
      /* DSP buffer data wraps, do pre-wrap copy */
      /* */

      dwHostDstCurrent =
	kCopyLinearToCircular (devc, phwi,
			       dwOutBufTail,
			       dwDSPOutBufferAddr + (dwDSPOutBufferLen / 2)
			       - dwOutBufTail,
			       dwHostDstCurrent,
			       dwHostDstBufferAddr, dwHostDstBufferLen);

      /* */
      /* do post-wrap copy */
      /* */

      dwHostDstCurrent =
	kCopyLinearToCircular (devc, phwi,
			       dwDSPOutBufferAddr,
			       dwOutBufHead - dwDSPOutBufferAddr,
			       dwHostDstCurrent,
			       dwHostDstBufferAddr, dwHostDstBufferLen);
    }

  /* */
  /* write updated buffer state to DSP */
  /* */

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA,
		 pClient_Inst->dwDspDataClientArea + CDATA_OUT_BUF_TAIL,
		 (WORD) dwOutBufHead);

  pClient_Inst->dwHostDstCurrent = dwHostDstCurrent;

}				/* kPIOInterruptHandler() */
#endif

#ifdef HOSTI2SFREQCALC		/* I2S was calculated in the host */
#ifdef DOS_MODEL
#pragma message("----kI2SInterruptHandler omitted (causes internal compiler error)")
#else
/*-------------------------------------------------------------------------- */
/* */
/*  VOID kI2SInterruptHandler */
/* */
/*  Description: */
/*      I2S data flow impeded interrupt handler. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      PDWORD pdwI2SRate */
/*         Pointer to DWORD that will contain the I2S sample rate or */
/*         -1 if the sample rate should not be changed */
/* */
/*  Return (VOID): */
/* */
/*-------------------------------------------------------------------------- */

VOID
kI2SInterruptHandler (allegro_devc * devc, PHWI phwi, PDWORD pdwI2SRate)
{
  WORD wI2SSampleCount;
  WORD wI2STimerCount;
  WORD wI2SSampleRate;

  /* */
  /* Get current sample count and timer count */
  /* */

  wI2SSampleCount = kDspReadWord (devc, phwi->dwBaseIO,
				  MEMTYPE_INTERNAL_DATA,
				  KDATA_I2S_SAMPLE_COUNT);

  wI2STimerCount = kInW (devc, phwi->dwBaseIO + DSP_PORT_TIMER_COUNT);

  /* */
  /* I2S sample rate determination is a two pass operation. */
  /* On the first pass we zero the activity flag and return -1. */
  /* On the second pass we check if the activity flag is still */
  /* zero.  If so, we know the I2S data stream has stopped */
  /* and we return 0 for the sample rate.  Otherwise, we use */
  /* the sample count data and do our best to determine the */
  /* sample rate. */
  /* */

  if (phwi->dwFlags & HWI_FLAG_I2S_SECONDPASS)
    {
      if (!kDspReadWord (devc, phwi->dwBaseIO,
			 MEMTYPE_INTERNAL_DATA, KDATA_I2S_ACTIVE))
	{
	  /* */
	  /* the I2S stream has stopped */
	  /* */

	  *pdwI2SRate = 0;
	}
      else
	{
	  /* */
	  /* the I2S stream is moving */
	  /* */

	  wI2SSampleCount -= phwi->wI2SSampleCount;
	  wI2STimerCount -= phwi->wI2STimerCount;

	  /* */
	  /* determine sample rate */
	  /* */
	  /* The timer rate is 48,000 ticks/second, so */
	  /* */
	  /* samples   N samples   48,000 ticks      1 */
	  /* ------- = --------- X ------------ X ------- */
	  /*  second       1         1 second     M ticks */
	  /* */

	  wI2SSampleRate = (wI2SSampleCount * 48000) / wI2STimerCount;

	  if (wI2SSampleRate > (48000 + 44100) / 2)
	    *pdwI2SRate = 48000;
	  else if (wI2SSampleRate > (44100 + 32000) / 2)
	    *pdwI2SRate = 44100;
	  else if (wI2SSampleRate > (32000 + 22050) / 2)
	    *pdwI2SRate = 32000;
	  else
	    *pdwI2SRate = 22050;
	}

      phwi->dwFlags &= ~HWI_FLAG_I2S_SECONDPASS;
    }
  else
    {
      /* */
      /* zero the activity flag and advance to the next state */
      /* */

      kDspWriteWord (devc, phwi->dwBaseIO,
		     MEMTYPE_INTERNAL_DATA, KDATA_I2S_ACTIVE, 0);

      phwi->wI2SSampleCount = wI2SSampleCount;
      phwi->wI2STimerCount = wI2STimerCount;

      *pdwI2SRate = (DWORD) - 1;

      phwi->dwFlags |= HWI_FLAG_I2S_SECONDPASS;
    }

}				/* kI2SInterruptHandler() */
#endif
#endif

#ifdef APUBLOCKCOUNT
/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kResetApuBlockCount */
/* */
/*  Description: */
/*      Reset a client's APU block count field. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      PCLIENT_INST pClient_Inst */
/*         Pointer to client instance */
/* */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */

KRETURN
kResetApuBlockCount (allegro_devc * devc, PHWI phwi,
		     PCLIENT_INST pClient_Inst)
{
  DWORD dwClient = pClient_Inst->dwClient;

  /* */
  /* Fail if the DSP kernel has been unloaded */
  /* */

  if (phwi->dwFlags & HWI_FLAG_UNLOADED)
    return KRETURN_ERROR_UNLOADED;

  /* */
  /* Make sure client and an instance exist... */
  /* */

  if (dwClient >= NUMBER_OF_CLIENTS)
    return KRETURN_ERROR_GENERIC;

  if (!phwi->asClientTable[dwClient].dwReferenceCount)
    return KRETURN_ERROR_GENERIC;

  /* */
  /* Reset the block count */
  /* */

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA,
		 pClient_Inst->
		 dwDspDataClientArea + CDATA_APU_BLOCK_COUNTL, 0);

  kDspWriteWord (devc, phwi->dwBaseIO,
		 MEMTYPE_INTERNAL_DATA,
		 pClient_Inst->
		 dwDspDataClientArea + CDATA_APU_BLOCK_COUNTH, 0);

  return KRETURN_SUCCESS;

}				/* kResetApuBlockCount() */

/*-------------------------------------------------------------------------- */
/* */
/*  KRETURN kGetApuBlockCount */
/* */
/*  Description: */
/*      Return a client's current stream position in terms of the number */
/*      of APU blocks transfered. */
/* */
/*  Parameters: */
/*      PHWI phwi */
/*         Pointer to hardware instance. */
/* */
/*      PCLIENT_INST pClient_Inst */
/*         Pointer to client instance */
/* */
/*      PDWORD pdwBlockCount */
/*         Pointer to DWORD that will contain the number of APU blocks */
/* */
/*  Return (KRETURN): */
/*      KRETURN_SUCCESS if successful, KRETURN_ERROR_XXX otherwise. */
/* */
/*-------------------------------------------------------------------------- */

KRETURN kGetApuBlockCount
  (allegro_devc * devc, PHWI phwi, PCLIENT_INST pClient_Inst,
   PDWORD pdwBlockCount)
{
  DWORD dwClient = pClient_Inst->dwClient;
  WORD wBlockCount;
  WORD wRetry = 10;
  WORD wBlockCountL;
  WORD wBlockCountH;

  /* */
  /* Fail if the DSP kernel has been unloaded */
  /* */

  if (phwi->dwFlags & HWI_FLAG_UNLOADED)
    return KRETURN_ERROR_UNLOADED;

  /* */
  /* Make sure client and an instance exist... */
  /* */

  if (dwClient >= NUMBER_OF_CLIENTS)
    return KRETURN_ERROR_GENERIC;

  if (!phwi->asClientTable[dwClient].dwReferenceCount)
    return KRETURN_ERROR_GENERIC;

  /* */
  /* Get the block count */
  /* */

  while (wRetry--)
    {
      /* read high/low word of current block count */

      wBlockCountH = kDspReadWord (devc, phwi->dwBaseIO,
				   MEMTYPE_INTERNAL_DATA,
				   pClient_Inst->
				   dwDspDataClientArea +
				   CDATA_APU_BLOCK_COUNTH);

      wBlockCountL = kDspReadWord (devc, phwi->dwBaseIO,
				   MEMTYPE_INTERNAL_DATA,
				   pClient_Inst->
				   dwDspDataClientArea +
				   CDATA_APU_BLOCK_COUNTL);

      /* if the high word hasn't changed, we've got a meaningful */
      /* block count value */

      wBlockCount = kDspReadWord (devc, phwi->dwBaseIO,
				  MEMTYPE_INTERNAL_DATA,
				  pClient_Inst->
				  dwDspDataClientArea +
				  CDATA_APU_BLOCK_COUNTH);

      if (wBlockCount == wBlockCountH)
	break;
    }

  /* fail if we couldn't get a meaningful block count */

  if (wBlockCount != wBlockCountH)
    return KRETURN_ERROR_GENERIC;

  *pdwBlockCount = MAKELONG (wBlockCountL, wBlockCountH);

  return KRETURN_SUCCESS;

}				/* kGetApuBlockCount() */

#endif

/*--------------------------------------------------------------------------- */
/*  End of File: kernel.c */
/*--------------------------------------------------------------------------- */

/******************************************************************************
 *                                                                            *
 *                       (C) 1997-1999 ESS Technology, Inc.                   *
 *                                                                            *
 ******************************************************************************/
