/****************************************************************************
 Environ.h
	
 Created: David A. Hoatson

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

 Environment: DOS / Windows 95 User Mode / Windows 95 VXD / Windows NT Kernel

 Revision History

 When      Who  Description
 --------- ---  ---------------------------------------------------------------
****************************************************************************/
#ifndef _ENVIRON_H
#define _ENVIRON_H

/////////////////////////////////////////////////////////////////////////////
// Figure out which compiler we are using
/////////////////////////////////////////////////////////////////////////////

// Is the Microsoft key #define'd
#ifdef _MSC_VER
#ifndef MICROSOFT
#define MICROSOFT
#endif
#endif

// Is the Borland key #define'd
#ifdef __BORLANDC__
#ifndef BORLAND
#define BORLAND
#define DOS			// MUST be in DOS mode as well
#endif
#endif

// Make sure we didn't define both
#if defined(MICROSOFT) && defined(BORLAND)
#error Cannot have MICROSOFT and BORLAND both defined!
#endif

/////////////////////////////////////////////////////////////////////////////
// Figure out which operating system we are using
/////////////////////////////////////////////////////////////////////////////

// Make sure we defined some environment
#if defined(WIN95USER)
#define ENVIRON
#endif
#if defined(WIN95VXD)
#define ENVIRON
#endif
#if defined(NT)
#define ENVIRON
#endif
#if defined(WIN32USER)
#define ENVIRON
#endif
#if defined(DOS)
#define ENVIRON
#endif
#if defined(MACINTOSH)
#define ENVIRON
#endif
#if !defined(ENVIRON)
	//#error You must define an environment!
#endif

/////////////////////////////////////////////////////////////////////////////
// Everybody Gets These
/////////////////////////////////////////////////////////////////////////////
#ifndef MAKEULONG
#define MAKEULONG(low, high)	((ULONG)(((USHORT)(low)) | (((ULONG)((USHORT)(high))) << 16)))
#endif
//#ifndef MAKELONG
//  #define MAKELONG(a, b)      ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
//#endif
#ifndef MAKEUSHORT
#define MAKEUSHORT(low, high)	((USHORT)(((BYTE)(low)) | (((USHORT)((BYTE)(high))) << 8)))
#endif
#ifndef MAKEWORD
#define MAKEWORD(a, b)      ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#endif

/////////////////////////////////////////////////////////////////////////////
// Microsoft
/////////////////////////////////////////////////////////////////////////////
#ifdef MICROSOFT
	// Nothing at the moment
#endif // MICROSOFT

/////////////////////////////////////////////////////////////////////////////
// Borland DOS
/////////////////////////////////////////////////////////////////////////////
#ifdef BORLAND
#define register
#endif

/////////////////////////////////////////////////////////////////////////////
// Windows NT Kernel Mode
/////////////////////////////////////////////////////////////////////////////
#ifdef NT
#include <ntddk.h>
#include <basetsd.h>		// DAH Added May 30, 2000 to be compatible with new MSSDK include files
#include <windef.h>
#include <mmsystem.h>
#include <wchar.h>
#endif // NT

/////////////////////////////////////////////////////////////////////////////
// Windows 95 VXD
/////////////////////////////////////////////////////////////////////////////
#ifdef WIN95VXD
#define WANTVXDWRAPS
#include <windows.h>
#include <mmsystem.h>

#undef CDECL
#undef PASCAL
#undef PSZ

#include <vmm.h>
#include <vmmreg.h>

#pragma warning (disable:4142)	// turn off "benign redefinition of type"
#define _NTDEF_			// make sure _LARGE_INTEGER doesn't get defined
#include <basedef.h>
#pragma warning (default:4142)	// turn on "benign redefinition of type"

#include <vxdldr.h>		// must go before vxdwraps.h
#include <vwin32.h>
#include <vpicd.h>		// must go before vxdwraps.h and after basedef.h
#include <vxdwraps.h>

#ifdef CURSEG
#undef CURSEG
#endif
#include <configmg.h>
#include <regstr.h>
#include <winerror.h>

#undef PASCAL
#define PASCAL      __stdcall

#include <dsound.h>
#include <dsdriver.h>
#include <dscert.h>

#include <debug.h>

#pragma intrinsic(strcpy, strlen)

#include <string.h>		// for memXXX functions

#pragma VxD_LOCKED_DATA_SEG
#pragma VxD_LOCKED_CODE_SEG

typedef unsigned char BOOLEAN;
typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef short SHORT;
typedef unsigned long ULONG;
typedef unsigned char *PUCHAR;
typedef unsigned long *PULONG;
typedef unsigned long *LPULONG;
typedef unsigned short *LPUSHORT;

#define VOID	void

	//typedef unsigned long     LONGLONG;   // BUGBUG

	//#define LOBYTE(w)           ((BYTE)(w))
	//#define HIBYTE(w)           ((BYTE)(((WORD)(w) >> 8) & 0xFF))

	//typedef struct waveformat_tag {
	//  WORD    wFormatTag;        /* format type */
	//  WORD    nChannels;         /* number of channels (i.e. mono, stereo, etc.) */
	//  DWORD   nSamplesPerSec;    /* sample rate */
	//  DWORD   nAvgBytesPerSec;   /* for buffer estimation */
	//  WORD    nBlockAlign;       /* block size of data */
	//} WAVEFORMAT, *PWAVEFORMAT, NEAR *NPWAVEFORMAT, FAR *LPWAVEFORMAT;

	/* flags for wFormatTag field of WAVEFORMAT */
	//#define WAVE_FORMAT_PCM     1

	/* specific waveform format structure for PCM data */
	//typedef struct pcmwaveformat_tag {
	//  WAVEFORMAT  wf;
	//  WORD        wBitsPerSample;
	//} PCMWAVEFORMAT, *PPCMWAVEFORMAT, NEAR *NPPCMWAVEFORMAT, FAR *LPPCMWAVEFORMAT;

	//typedef struct tWAVEFORMATEX
	//{
	//  WORD        wFormatTag;         /* format type */
	//  WORD        nChannels;          /* number of channels (i.e. mono, stereo...) */
	//  DWORD       nSamplesPerSec;     /* sample rate */
	//  DWORD       nAvgBytesPerSec;    /* for buffer estimation */
	//  WORD        nBlockAlign;        /* block size of data */
	//  WORD        wBitsPerSample;     /* number of bits per sample of mono data */
	//  WORD        cbSize;             /* the count in bytes of the size of */
	//} WAVEFORMATEX, *PWAVEFORMATEX, NEAR *NPWAVEFORMATEX, FAR *LPWAVEFORMATEX;

#define RtlMoveMemory(Destination,Source,Length)	memmove((Destination),(Source),(Length))
#define RtlCopyMemory(Destination,Source,Length)	memcpy((Destination),(Source),(Length))
#define RtlFillMemory(Destination,Length,Fill)		memset((Destination),(Fill),(Length))
#define RtlZeroMemory(Destination,Length)			memset((Destination),0,(Length))

	// Assumes pAddr is in segment:offset form.
	// 386 instructions must be turned on for this to work
#define READ_REGISTER_ULONG( pAddr )				*(DWORD FAR *)(pAddr)
#define WRITE_REGISTER_ULONG( pAddr, ulValue )		*(DWORD*)(pAddr) = (ulValue)

#define READ_REGISTER_BUFFER_ULONG( pAddr, pBuffer, ulSize )
#define WRITE_REGISTER_BUFFER_ULONG( pAddr, pBuffer, ulSize )

#ifdef DEBUG
int _inp (unsigned port);
int _outp (unsigned port, int databyte);
#pragma intrinsic( _inp, _outp )

#define WRITE_PORT_UCHAR(a,x)						_outp( (unsigned)(a), (int)(x) )
#define READ_PORT_UCHAR(a)							_inp( (unsigned)(a) )
#endif

#define PCI_TYPE0_ADDRESSES		5
#endif // VXD

/////////////////////////////////////////////////////////////////////////////
// Windows 95 User Mode (16 Bit)
/////////////////////////////////////////////////////////////////////////////
#ifdef WIN95USER

#ifdef WIN32			// Compiling within the IDE
#undef MAKEWORD
#endif

#include <windows.h>
#include <mmsystem.h>

#define MMNOAUXDEV
#define MMNOJOYDEV
#define MMNOMCIDEV
#define MMNOTASKDEV
#include <mmddk.h>
#include <mmreg.h>

#ifdef WIN32			// Compiling within the IDE
#undef FAR
#undef NEAR
#define FAR
#define NEAR
#define EXPORT
#define LOADDS
#define MAKELP(sel, off)    ((void FAR*)MAKELONG((off), (sel)))
#define DRVM_EXIT		0x65
#define	DRVM_DISABLE	0x66
#define	DRVM_ENABLE		0x67
#else
#define EXPORT	_export
#define LOADDS	_loadds
#endif

#ifndef MM_LYNX
#define	MM_LYNX		212
#endif

	// generate intrinsic code instead of function calls
void _enable (void);
void _disable (void);
#pragma intrinsic( _enable, _disable )

typedef unsigned int UINT;
typedef unsigned char BOOLEAN;
typedef unsigned char UCHAR;
typedef unsigned short USHORT;
typedef short SHORT;
typedef unsigned long ULONG;
typedef unsigned short FAR *LPUSHORT;
typedef unsigned char FAR *LPBYTE;
typedef unsigned char FAR *LPUCHAR;
typedef unsigned long FAR *LPULONG;
typedef char FAR *LPSTR;
#define VOID	void

#ifndef WIN32
#define RtlMoveMemory(Destination,Source,Length)	_fmemmove((Destination),(Source),(Length))
#define RtlCopyMemory(Destination,Source,Length)	_fmemcpy((Destination),(Source),(Length))
#define RtlFillMemory(Destination,Length,Fill)		_fmemset((Destination),(Fill),(Length))
#define RtlZeroMemory(Destination,Length)			_fmemset((Destination),0,(Length))
#endif

#define labs(a)         (a) < 0 ? (-a) : (a)
#define abs(a)          (a) < 0 ? (-a) : (a)
#endif

#ifdef WIN32USER
#define PCI_TYPE0_ADDRESSES	5	// never used
#endif

/////////////////////////////////////////////////////////////////////////////
// DOS BorlandC
/////////////////////////////////////////////////////////////////////////////
#ifdef DOS
#include <memory.h>

#ifndef MAKELONG
#define MAKELONG(a, b)      ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#endif

#ifdef DEBUG
#define DPF( _SZ_ )	printf _SZ_
#else
#define DPF( _SZ_ )
#endif

#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif

#define VOID                void
#ifdef WIN32
#define FAR
#define NEAR
#else
#define FAR                 _far
#define NEAR                _near
#endif
#define PASCAL              _pascal
#define CDECL               _cdecl

typedef unsigned char BOOLEAN;
typedef unsigned char BOOL;
typedef unsigned char UCHAR;

typedef short SHORT;
typedef long LONG;
typedef unsigned short USHORT;
typedef unsigned long ULONG;

typedef short FAR *PSHORT;
typedef long FAR *PLONG;
typedef unsigned short FAR *PUSHORT;
typedef unsigned long FAR *PULONG;

typedef unsigned char FAR *PBYTE;
typedef unsigned char FAR *PUCHAR;
typedef void *PVOID;
typedef unsigned char FAR *LPUCHAR;
typedef unsigned long FAR *LPULONG;
typedef void *LPVOID;

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned int UINT;
typedef unsigned long ULONG;

typedef unsigned long LONGLONG;	// BUGBUG

#define LOBYTE(w)           ((BYTE)(w))
#define HIBYTE(w)           ((BYTE)(((WORD)(w) >> 8) & 0xFF))
#define LOWORD(l)           ((WORD)(DWORD)(l))
#define HIWORD(l)           ((WORD)((((DWORD)(l)) >> 16) & 0xFFFF))

typedef struct waveformat_tag
{
  WORD wFormatTag;		/* format type */
  WORD nChannels;		/* number of channels (i.e. mono, stereo, etc.) */
  DWORD nSamplesPerSec;		/* sample rate */
  DWORD nAvgBytesPerSec;	/* for buffer estimation */
  WORD nBlockAlign;		/* block size of data */
}
WAVEFORMAT, *PWAVEFORMAT, NEAR * NPWAVEFORMAT, FAR * LPWAVEFORMAT;

	/* flags for wFormatTag field of WAVEFORMAT */
#define WAVE_FORMAT_PCM     1

	/* specific waveform format structure for PCM data */
typedef struct pcmwaveformat_tag
{
  WAVEFORMAT wf;
  WORD wBitsPerSample;
}
PCMWAVEFORMAT, *PPCMWAVEFORMAT, NEAR * NPPCMWAVEFORMAT, FAR * LPPCMWAVEFORMAT;

typedef struct tWAVEFORMATEX
{
  WORD wFormatTag;		/* format type */
  WORD nChannels;		/* number of channels (i.e. mono, stereo...) */
  DWORD nSamplesPerSec;		/* sample rate */
  DWORD nAvgBytesPerSec;	/* for buffer estimation */
  WORD nBlockAlign;		/* block size of data */
  WORD wBitsPerSample;		/* number of bits per sample of mono data */
  WORD cbSize;			/* the count in bytes of the size of */
}
WAVEFORMATEX, *PWAVEFORMATEX, NEAR * NPWAVEFORMATEX, FAR * LPWAVEFORMATEX;

#define RtlMoveMemory(Destination,Source,Length)	memmove((Destination),(Source),(Length))
#define RtlCopyMemory(Destination,Source,Length)	memcpy((Destination),(Source),(Length))
#define RtlFillMemory(Destination,Length,Fill)		memset((Destination),(Fill),(Length))
#define RtlZeroMemory(Destination,Length)			memset((Destination),0,(Length))

	// Assumes pAddr is in segment:offset form.
	// 386 instructions must be turned on for this to work
#define READ_REGISTER_ULONG( pAddr )				*(DWORD FAR *)(pAddr)
#define WRITE_REGISTER_ULONG( pAddr, ulValue )		*(DWORD FAR *)(pAddr) = (ulValue)

#define READ_REGISTER_BUFFER_ULONG(x, y, z) \
   { \
       PULONG registerBuffer = x; \
       PULONG readBuffer = y; \
       ULONG readCount; \
       for (readCount = z; readCount--; readBuffer++, registerBuffer++) {  \
           *readBuffer = *(volatile ULONG * const)(registerBuffer); \
       } \
   }

#define WRITE_REGISTER_BUFFER_ULONG(x, y, z) \
  { \
       PULONG registerBuffer = x; \
       PULONG writeBuffer = y; \
       ULONG writeCount; \
       for (writeCount = z; writeCount--; writeBuffer++, registerBuffer ++) { \
           *(volatile ULONG * const)(registerBuffer) = *writeBuffer; \
       } \
  }

#define PCI_TYPE0_ADDRESSES		5
#endif

#define OSS
#ifdef OSS
	//#define DEBUG

#ifndef TRUE
#define TRUE    1
#endif
#ifndef FALSE
#define FALSE   0
#endif

#define FAR
#define NEAR

#define VOID                void

typedef unsigned char BOOLEAN;
typedef unsigned char BOOL;
typedef unsigned char UCHAR;
typedef char TCHAR;

typedef short SHORT;
typedef int LONG;
typedef unsigned short USHORT;
typedef unsigned int ULONG;

typedef short FAR *PSHORT;
typedef int FAR *PLONG;
typedef unsigned short FAR *PUSHORT;
typedef unsigned int FAR *PULONG;

typedef unsigned char FAR *PBYTE;
typedef unsigned char FAR *PUCHAR;
typedef void *PVOID;
typedef unsigned char FAR *LPUCHAR;
typedef unsigned int FAR *LPULONG;
typedef void *LPVOID;

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;
typedef unsigned int UINT;
	//typedef unsigned long     ULONG;
	//typedef unsigned int      UINT;

typedef long long __int64;
typedef long long LONGLONG;
#define MAKELONG(a, b)      ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#define MAKEWORD(a, b)      ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))

//  #define i64

#define	TEXT( a )	a

#define PCI_TYPE0_ADDRESSES	5

#define LOBYTE(w)           ((BYTE)(w))
#define HIBYTE(w)           ((BYTE)(((WORD)(w) >> 8) & 0xFF))
#define LOWORD(l)           ((WORD)(DWORD)(l))
#define HIWORD(l)           ((WORD)((((DWORD)(l)) >> 16) & 0xFFFF))

typedef struct waveformat_tag
{
  WORD wFormatTag;		/* format type */
  WORD nChannels;		/* number of channels (i.e. mono, stereo, etc.) */
  DWORD nSamplesPerSec;		/* sample rate */
  DWORD nAvgBytesPerSec;	/* for buffer estimation */
  WORD nBlockAlign;		/* block size of data */
}
WAVEFORMAT, *PWAVEFORMAT, NEAR * NPWAVEFORMAT, FAR * LPWAVEFORMAT;

	/* flags for wFormatTag field of WAVEFORMAT */
#define WAVE_FORMAT_PCM     1

	/* specific waveform format structure for PCM data */
typedef struct pcmwaveformat_tag
{
  WAVEFORMAT wf;
  WORD wBitsPerSample;
}
PCMWAVEFORMAT, *PPCMWAVEFORMAT, NEAR * NPPCMWAVEFORMAT, FAR * LPPCMWAVEFORMAT;

typedef struct tWAVEFORMATEX
{
  WORD wFormatTag;		/* format type */
  WORD nChannels;		/* number of channels (i.e. mono, stereo...) */
  DWORD nSamplesPerSec;		/* sample rate */
  DWORD nAvgBytesPerSec;	/* for buffer estimation */
  WORD nBlockAlign;		/* block size of data */
  WORD wBitsPerSample;		/* number of bits per sample of mono data */
  WORD cbSize;			/* the count in bytes of the size of */
}
WAVEFORMATEX, *PWAVEFORMATEX, NEAR * NPWAVEFORMATEX, FAR * LPWAVEFORMATEX;

#define RtlMoveMemory(Destination,Source,Length)	memmove((Destination),(Source),(Length))
#define RtlCopyMemory(Destination,Source,Length)	memcpy((Destination),(Source),(Length))
#define RtlFillMemory(Destination,Length,Fill)		memset((Destination),(Fill),(Length))
#define RtlZeroMemory(Destination,Length)		memset((Destination),0,(Length))
#define READ_REGISTER_ULONG( pAddr )                    *(volatile ULONG * const)(pAddr)
#define WRITE_REGISTER_ULONG( pAddr, ulValue )          *(volatile ULONG * const)(pAddr) = (ulValue)

#define READ_REGISTER_BUFFER_ULONG( pAddr, pBuffer, ulSize )
#define WRITE_REGISTER_BUFFER_ULONG( pAddr, pBuffer, ulSize )

	//#include <DrvDebug.h>
#endif

#endif // _ENVIRON_H
