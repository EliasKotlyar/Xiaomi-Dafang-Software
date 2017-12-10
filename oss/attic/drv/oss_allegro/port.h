/*
 *      ESS Technology allegro audio driver.
 *
 *      Copyright (C) 1992-2000  Don Kim (don.kim@esstech.com)
 *
 */
#define VOID void
typedef void *PVOID;
typedef char CHAR;
typedef short SHORT;
typedef long LONG;
//typedef int BOOL;
//typedef unsigned char UCHAR;
typedef unsigned char *PBYTE;
typedef unsigned short *PUSHORT;
#define BYTE UCHAR
#define BOOLEAN UCHAR
typedef unsigned long DWORD;
#define USHORT	WORD
typedef unsigned short WORD;
#define ULONG	DWORD
#define IN
#define OUT
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
typedef unsigned short *PWORD;
typedef unsigned long *PDWORD;
typedef unsigned long *PULONG;

#define inp(o,a)	INB(o, a)
#define inpw(o,a)	INW(o, a)
#define outp(o,a,d) 	OUTB(o, d, a)
#define outpw(o,a,d) 	OUTW(o, d, a)

#define CRITENTER

#define CRITLEAVE

#define MAKEWORD(a, b)      ((WORD)(((BYTE)(a)) | ((WORD)((BYTE)(b))) << 8))
#define MAKELONG(a, b)      ((LONG)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))
#define LOWORD(l)           ((WORD)(l))
#define HIWORD(l)           ((WORD)(((DWORD)(l) >> 16) & 0xFFFF))
#define LOBYTE(w)           ((BYTE)(w))
#define HIBYTE(w)           ((BYTE)(((WORD)(w) >> 8) & 0xFF))


#define KeStallExecutionProcessor	oss_udelay
#define SoundDelay	mdelay

#define KeAcquireSpinLock(a, b)

#define KeReleaseSpinLock(a, b)


#define READ_PORT_UCHAR( o, a )  INB(o, a)
#define READ_PORT_USHORT( o, a )  INW(o, a)
#define WRITE_PORT_UCHAR( o, a, d )  OUTB(o, d, a)
#define WRITE_PORT_USHORT( o, a, d )  OUTW(o, d, a)

#define __cdecl

typedef struct _WAVE_INFO
{
  ULONG SamplesPerSec;
  UCHAR BitsPerSample;
  UCHAR Channels;
}
WAVE_INFO, *PWAVE_INFO;

#define KIRQL
#define OldIrql

#ifdef MDEBUG
extern void dDbgOut (char *sz, ...);
#define dprintf1( _x_ ) if (debug >= 1) dDbgOut _x_
#define dprintf3( _x_ ) if (debug >= 3) dDbgOut _x_
#else
#define dprintf1( _x_ )
#define dprintf3( _x_ )
#endif
