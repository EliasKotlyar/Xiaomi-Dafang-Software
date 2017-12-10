#ifndef _OS_H_
#define _OS_H_

/*
 * Purpose: OS specific definitions for VxWorks
 *
 */
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

#define OS_VERSION "5.5"

#if (!defined(i386) && !defined(x86_64)) || defined(CONFIG_OSS_FIXDEPOINT)
// Floating point is not supported or it's disabled
#undef CONFIG_OSS_VMIX_FLOAT
#endif

#define VDEV_SUPPORT
#undef  USE_DEVICE_SUBDIRS

#define __inline__ inline
#define __inline inline
#define EXTERN_C extern "C"

/*
 * Disable support for per-application features such as /dev/dsp device
 * selection based on command name. Requires working GET_PROCESS_NAME
 * macro implementation.
 */
#undef  APPLIST_SUPPORT

#undef  ALLOW_BUFFER_MAPPING

/*
 * Some integer types
 */
#if defined(amd64) || defined(sparc)
typedef unsigned long long oss_native_word;	/* Same as the address and status register size */
#else
typedef unsigned long oss_native_word;	/* Same as the address and status register size */
#endif

typedef long long oss_int64_t;			/* Signed 64 bit integer */
typedef unsigned long long oss_uint64_t;	/* Unsigned 64 bit integer */

#include <stdarg.h>
#include <sys/types.h>
#include "vxWorks.h"
#include "iv.h"
#include "ioLib.h"
#include "fioLib.h"
#include "iosLib.h"
#include "wdLib.h"
#include "intLib.h"
#include "tickLib.h"
#include "errnoLib.h"
#include "sysLib.h"
#include "objLib.h"
#include "vmLib.h"
#include "semLib.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <oss_errno.h>
/*
 * Mutexes
 */
typedef int oss_mutex_t;
#define MUTEX_INIT(osdev, mutex, hier)
#define MUTEX_CLEANUP(mutex)
#define MUTEX_ENTER_IRQDISABLE(mutex, flags)	{flags=0;mutex=0;}
#define MUTEX_ENTER(mutex, flags)	{flags=0;mutex=0;}
#define MUTEX_EXIT_IRQRESTORE(mutex, flags)	(flags)++
#define MUTEX_EXIT(mutex, flags)	(flags)++

/*
 * Fileinfo structure
 */
struct fileinfo
{
  int mode;			/* Open mode */
  int acc_flags;
};
#define ISSET_FILE_FLAG(fileinfo, flag)  (fileinfo->acc_flags & (flag) ? 1:0)

/*
 * Misc types
 */
typedef int oss_dma_handle_t;
typedef int oss_poll_event_t;
typedef int offset_t;

/*
 * uio_/uiomove()
 */
typedef enum uio_rw uio_rw_t;
typedef struct oss_uio
{
  char *ptr;
  int resid;
  int kernel_space;		/* Set if this uio points to a kernel space buffer */
  uio_rw_t rw;
} uio_t;
extern int oss_uiomove (void *address, size_t nbytes, enum uio_rw rwflag,
			uio_t * uio_p);
extern int oss_create_uio (uio_t * uiop, char *buf, size_t count, uio_rw_t rw,
			   int is_kernel);
#define uiomove oss_uiomove

/*
 * Error handling
 */
#define cmn_err oss_cmn_err
extern int detect_trace;
#define DDB(x) if (detect_trace) x
extern void oss_cmn_err (int level, const char *format, ...);
#define CE_CONT		0
#define CE_NOTE		1
#define CE_WARN		2
#define CE_PANIC	3

/* Busy wait routine */
extern void oss_udelay(unsigned long ticks);
/* System wall timer access */
#define GET_JIFFIES()	tickGet()
extern inline unsigned int
__inb (unsigned short port)
{
  unsigned int _v;
  __asm__ __volatile__ ("in" "b" " %" "w" "1,%" "b" "0":"=a" (_v):"d" (port),
			"0" (0));
  return _v;
}
extern inline unsigned int
__inw (unsigned short port)
{
  unsigned int _v;
  __asm__ __volatile__ ("in" "w" " %" "w" "1,%" "w" "0":"=a" (_v):"d" (port),
			"0" (0));
  return _v;
}
extern inline unsigned int
__inl (unsigned short port)
{
  unsigned int _v;
  __asm__ __volatile__ ("in" "l" " %" "w" "1,%" "" "0":"=a" (_v):"d" (port));
  return _v;
}

extern inline void
__outb (unsigned char value, unsigned short port)
{
  __asm__ __volatile__ ("out" "b" " %" "b" "0,%" "w" "1"::"a" (value),
			"d" (port));
}
extern inline void
__outw (unsigned short value, unsigned short port)
{
  __asm__ __volatile__ ("out" "w" " %" "w" "0,%" "w" "1"::"a" (value),
			"d" (port));
}
extern inline void
__outl (unsigned int value, unsigned short port)
{
  __asm__ __volatile__ ("out" "l" " %" "0,%" "w" "1"::"a" (value),
			"d" (port));
}

#define INB(osdev,a)	__inb(a)
#define INW(osdev,a)	__inw(a)
#define INL(osdev,a)	__inl(a)

#define OUTB(osdev, d, a) __outb(d, a)

#define OUTW(osdev, d, a)	__outw(d, a)
#define OUTL(osdev, d, a)	__outl(d, a)

#define PCI_READL(osdev, p)  (*(volatile unsigned int *) (p))
#define PCI_WRITEL(osdev, addr, data) (*(volatile unsigned int *) (addr) = (data))
#define PCI_READW(osdev, p)  (*(volatile unsigned short *) (p))
#define PCI_WRITEW(osdev, addr, data) (*(volatile unsigned short *) (addr) = (data))
#define PCI_READB(osdev, p)  (*(volatile unsigned char *) (p))
#define PCI_WRITEB(osdev, addr, data) (*(volatile unsigned char *) (addr) = (data))

#define MAP_PCI_IOADDR(osdev, nr, io) (oss_native_word)io
#define MAP_PCI_MEM(osdev, ix, phaddr, size) 	(oss_native_word)phaddr
#define UNMAP_PCI_MEM(osdev, ix, ph, virt, size)	{}
#define UNMAP_PCI_IOADDR(osdev, ix)	{}
/*
 * Memory allocation and mapping
 */
extern void *oss_contig_malloc (oss_device_t * osdev, int sz,
				oss_uint64_t memlimit,
				oss_native_word * phaddr);
extern void oss_contig_free (oss_device_t * osdev, void *p, int sz);
extern void oss_reserve_pages (oss_native_word start_addr,
			       oss_native_word end_addr);
extern void oss_unreserve_pages (oss_native_word start_addr,
				 oss_native_word end_addr);

#define KERNEL_MALLOC(nbytes)	malloc(nbytes)
#define KERNEL_FREE(addr)	free(addr)
#define CONTIG_MALLOC(osdev, sz, memlimit, phaddr, handle)	oss_contig_malloc(osdev, sz, memlimit, phaddr)
#define CONTIG_FREE(osdev, p, sz, handle)	oss_contig_free(osdev, p, sz)

typedef void dev_info_t;

struct _oss_device_t
{
  int cardnum;
  int dev_type;
  int available;
  int instance;
  dev_info_t *dip;
  void *devc;
  char *name;
  char nick[16];
  char handle[32];
  int num_audio_engines;
  int num_audioplay, num_audiorec, num_audioduplex;
  int num_mididevs;
  int num_mixerdevs;
  int num_loopdevs;
  int first_mixer;	/* This must be set to -1 by osdev_create() */
  int major;
  struct module *owner;		/* Pointer to THISMODULE (needed by osscore.c) */
  char modname[32];
  char *hw_info;

  volatile int refcount;	/* Nonzero means that the device is needed by some other (virtual) driver. */

/* Interrupts */

  int iblock_cookie;	/* Dummy field under Linux */
  void *irqparms;
  int intrcount, ackcount;
};

typedef struct
{
	int bus, dev, func;
} oss_pci_device_t;

typedef int timeout_id_t;
extern timeout_id_t oss_timeout (void (*func) (void *), void *arg,
				 unsigned long long ticks);
extern void oss_untimeout (timeout_id_t id);
#define timeout oss_timeout
#define untimeout oss_untimeout

extern int oss_hz;
#define HZ	oss_hz
#define OSS_HZ	HZ

/*
 * Dummy defines for poll()
 */
#define POLLIN		0
#define POLLOUT		0
#define POLLRDNORM	0
#define POLLWRNORM	0

/*
 * Process info macros
 */
#define GET_PROCESS_PID(x)  -1
#define GET_PROCESS_UID(x)  0
#define GET_PROCESS_NAME(x) NULL

/*
 * Floating point save/restore support for vmix
 */
#define FP_SUPPORT

#ifdef FP_SUPPORT
typedef short fp_env_t[512];
typedef unsigned int fp_flags_t[4];
extern int oss_fp_check (void);
extern void oss_fp_save (short *envbuf, fp_flags_t flags);
extern void oss_fp_restore (short *envbuf, fp_flags_t flags);
#undef FP_SAVE
#undef FP_RESTORE
#   define FP_SAVE(envbuf, flags)		oss_fp_save(envbuf, flags)
#   define FP_RESTORE(envbuf, flags)		oss_fp_restore(envbuf, flags)
#endif

#endif
