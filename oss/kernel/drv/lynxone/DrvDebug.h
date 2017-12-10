/****************************************************************************
 DrvDebug.h

 Created: David A. Hoatson, March 1998
	
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
#ifndef _DRVDEBUG_H
#define _DRVDEBUG_H

#define COLOR_UNDERLINE	1
#define COLOR_NORMAL	7
#define COLOR_BOLD_U	9
#define COLOR_BOLD		15
#define COLOR_REVERSE	120

#ifdef DEBUG

VOID DbgInitialize (VOID);
VOID DbgPutCh (UCHAR cChar, UCHAR cColor);
VOID DbgPutStr (char *szStr, UCHAR cColor);
VOID DbgPutX8 (UCHAR uc8, UCHAR cColor);
VOID DbgPutX16 (USHORT w16, UCHAR cColor);
VOID DbgPutX32 (ULONG dw32, UCHAR cColor);
VOID DbgPrintMono (PUCHAR szFormat, ...);
VOID DbgClose (VOID);
VOID DbgPutTextXY (char *szStr, UCHAR cColor, UCHAR X, UCHAR Y);
VOID DbgPrintF (const char *format, ...);

#ifndef DOS
#define DC( a )		DbgPutCh( a, COLOR_NORMAL )
#define DB( a, b )	DbgPutCh( a, b )
#define DS_( a, b )	DbgPutStr( a, b )
#define DX8( a, b )	DbgPutX8( a, b )
#define DX16( a, b )	DbgPutX16( a, b )
#define DX32( a, b )	DbgPutX32( a, b )
#define DSXY( a, b, c, d )	DbgPutTextXY( a, b, c, d )
#else
#define DbgInitialize()
#define DbgClose()

#define DC( a )
#define DB( a, b )
#define DS( a, b )
#define DX8( a, b )
#define DX16( a, b )
#define DX32( a, b )
#define DSXY( a, b, c, d )
#endif

#ifdef ALPHA
#define DPF(_X_)	DbgPrint _X_
#endif

#ifdef DOS
#define DPF( _SZ_ )	printf _SZ_
#endif

#ifdef MACINTOSH
#define DPF( _SZ_ )	DbgPrintF _SZ_
#endif

#ifdef NT
typedef struct _MONO_INFO
{
  //KSPIN_LOCK    DeviceSpinLock;
  KMUTEX DeviceMutex;
}
MONO_INFO, *PMONO_INFO;

#define DPF(_X_)	DbgPrintMono _X_
#endif

#ifndef DPF
#define DPF(_X_)	DbgPrintMono _X_
#endif

#else // non-debug

#define DbgInitialize()
#define DbgClose()

#define DC( a )
#define DB( a, b )
	// #define DS( a, b )
#define DX8( a, b )
#define DX16( a, b )
#define DX32( a, b )
#ifndef DPF
#define DPF(_X_)
#endif
#define DSXY( a, b, c, d )
#endif

#endif
