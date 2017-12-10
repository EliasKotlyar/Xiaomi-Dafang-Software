#ifndef _OS_H_
#define _OS_H_

/*
 * Purpose: OS specific definitions for FreeBSD
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
#define OS_VERSION "6"
#define __inline__ inline
#define __inline inline
#define EXTERN_C extern "C"

/*
 * Debugging and misc settings
 */
#undef  MUTEX_CHECKS
#undef  MEMDEBUG

#if (!defined(__i386__) && !defined(__x86_64__)) || defined(CONFIG_OSS_FIXDEPOINT)
// Floating point is not supported or it's disabled
#undef CONFIG_OSS_VMIX_FLOAT
#endif

/*
 * Disable support for per-application features such as /dev/dsp device
 * selection based on command name. Requires working GET_PROCESS_NAME
 * macro implementation.
 */
#undef  APPLIST_SUPPORT
#define USE_DEVICE_SUBDIRS

#include <stdarg.h>
#include <sys/types.h>
#ifdef _KERNEL
#include <sys/systm.h>
#endif
#include <sys/param.h>
#include <sys/uio.h>
#include <sys/fcntl.h>
#include <sys/poll.h>
#include <sys/malloc.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/kernel.h>
#include <machine/cpufunc.h>
#include <vm/vm.h>
#include <vm/pmap.h>
#include <sys/selinfo.h>
#include <oss_errno.h>

#if __FreeBSD_version >= 800062
#define MINOR(x) dev2unit(x)
#else
#define MINOR(x) minor(x)
#endif

#undef timeout
#define timeout oss_timeout
#undef untimeout
#define untimeout oss_untimeout
typedef int timeout_id_t;
extern timeout_id_t oss_timeout (void (*func) (void *), void *arg,
				 unsigned long long ticks);
extern void oss_untimeout (timeout_id_t id);

#include "kernel/OS/FreeBSD/wrapper/bsddefs.h"

#ifdef USE_SX_LOCK
#include <sys/proc.h>	/* XXX for curthread */
#include <sys/sx.h>
#else
#include <sys/mutex.h>
#endif


#define uiomove oss_uiomove
typedef struct uio uio_t;
extern int oss_uiomove (void *address, size_t nbytes, enum uio_rw rwflag,
			uio_t * uio_p);

#undef HZ
#define OSS_HZ hz

/* The soundcard.h could be in a nonstandard place so include it here. */
#include "soundcard.h"

typedef struct udi_usb_devc udi_usb_devc;

#define ALLOW_SELECT
#define ALLOW_BUFFER_MAPPING

/*
 * Sleep/wakeup
 */

#ifdef _KERNEL
struct oss_wait_queue
{
  oss_mutex_t mutex;
  unsigned long flags;
  struct selinfo poll_info;
};
#endif

/* Busy wait routine */

/* System wall timer access */
extern unsigned long oss_get_time (void);
#define GET_JIFFIES()	oss_get_time()

/*
 * Mutexes
 */

#ifdef USE_SX_LOCK
struct sx;
#define MUTEX_INIT(osdev, mutex, hier) \
do { \
	mutex = malloc(sizeof(*mutex), M_DEVBUF, M_WAITOK | M_ZERO); \
	sx_init(mutex, "oss"); \
} while (0)
#define MUTEX_CLEANUP(mutex) \
do { \
	sx_destroy(mutex); \
	free(mutex, M_DEVBUF); \
} while (0)
#define MUTEX_ENTER_IRQDISABLE(mutex, flags)	sx_xlock(mutex)
#define MUTEX_ENTER(mutex, flags)		sx_slock(mutex)
#define MUTEX_EXIT_IRQRESTORE(mutex, flags)	sx_xunlock(mutex)
#define MUTEX_EXIT(mutex, flags)		sx_sunlock(mutex)
#else	/* !USE_SX_LOCK */
struct mtx;
#define MUTEX_INIT(osdev, mutex, hier) \
do { \
	mutex = malloc(sizeof(*mutex), M_DEVBUF, M_WAITOK | M_ZERO); \
	mtx_init(mutex, "oss", NULL, MTX_RECURSE | MTX_SPIN); \
} while (0)
#define MUTEX_CLEANUP(mutex) \
do { \
	mtx_destroy(mutex); \
	free(mutex, M_DEVBUF); \
} while (0)
#define MUTEX_ENTER_IRQDISABLE(mutex, flags)	mtx_lock_spin_flags(mutex, flags)
#define MUTEX_ENTER(mutex, flags)		mtx_lock_spin(mutex)
#define MUTEX_EXIT_IRQRESTORE(mutex, flags)	mtx_unlock_spin_flags(mutex, flags)
#define MUTEX_EXIT(mutex, flags)		mtx_unlock_spin(mutex)
#endif	/* USE_SX_LOCK */


/*
 * INB() and OUTB() should be obvious. NOTE! The order of
 * paratemeters of OUTB() is different than on some other
 * operating systems.
 */

/* I/O Mapped devices */
#define INB(o, p)	inb(p)
#define INW(o, p)	inw(p)
#define INL(o, p)	inl(p)

#define OUTB(o, v, p)	outb(p,v)
#define OUTW(o, v, p)	outw(p,v)
#define OUTL(o, v, p)	outl(p,v)

/* Memory Mapped devices */
#define PCI_READL(osp, p)               readl(p)
#define PCI_READW(osp, p)               readw(p)
#define PCI_READB(osp, p)               readb(p)
#define PCI_WRITEL(osp, addr, data)     writel(addr, data)
#define PCI_WRITEW(osp, addr, data)     writew(addr, data)
#define PCI_WRITEB(osp, addr, data)     writeb(addr, data)

typedef void *oss_dma_handle_t;

/* 
   KERNEL_MALLOC() allocates requested number of memory  and 
   KERNEL_FREE is used to free it. 
   These macros are never called from interrupt, in addition the
   nbytes will never be more than 4096 bytes. Generally the driver
   will allocate memory in blocks of 4k. If the kernel has just a
   page level memory allocation, 4K can be safely used as the size
   (the nbytes parameter can be ignored).
*/
#define	KERNEL_MALLOC(nbytes)	malloc(nbytes, M_DEVBUF, M_NOWAIT|M_ZERO)
#define	KERNEL_FREE(addr)	{if (addr)free(addr, M_DEVBUF);addr=NULL;}

#define CONTIG_MALLOC(osdev, sz, memlimit, phaddr, handle)	oss_contig_malloc(sz, memlimit, phaddr)
#define CONTIG_FREE(osdev, p, sz, handle)	oss_contig_free(p, sz)

/*
 * Timer macros
 *
 * These macros are obsolete and should not be used in any new code.
 * Use the timeout mechanism (see the timeout(9F) Solaris man page).
 */
#define DEFINE_TIMER(name, proc)	static timeout_id_t name = 0
#define REMOVE_TIMER(name, proc)	{if (name != 0) untimeout(name);}
#define INIT_TIMER(name,proc)
typedef void (*timeout_func_t) (void *);
#define ACTIVATE_TIMER(name, proc, time) \
	name=timeout((timeout_func_t)proc, (void*)&name, time)
#endif

struct fileinfo
{
  int mode;			/* Open mode */
  int acc_flags;
  int pid;
  int dev;
  char *cmd;
};
#define ISSET_FILE_FLAG(fileinfo, flag)  (fileinfo->acc_flags & (flag) ? 1:0)

#define OSS_OS "FreeBSD"
#define OSS_OS_LONGNAME "FreeBSD " OS_VERSION

typedef void (*softintr_func_t) (int);

struct oss_softintr
{
  int id;
  softintr_func_t func;
  volatile int armed, running;
};

struct _oss_poll_event_t
{
  short events, revents;
  struct thread *p;
  struct cdev *bsd_dev;
};
typedef struct _oss_poll_event_t oss_poll_event_t;

extern int detect_trace;
#define DDB(x) if (detect_trace) x

extern caddr_t oss_map_pci_mem (oss_device_t * osdev, int nr, int phaddr,
				int size);
#define MAP_PCI_IOADDR(osdev, nr, io) (oss_native_word)(io)
#define MAP_PCI_MEM(osdev, ix, phaddr, size) 	oss_map_pci_mem(osdev, ix, phaddr, size)
#define UNMAP_PCI_MEM(osdev, ix, ph, virt, size)	{}
#define UNMAP_PCI_IOADDR(osdev, ix)	{}

#define GET_PROCESS_PID(f)  f->pid
#define GET_PROCESS_NAME(f) f->cmd

#define abs(x)                  ((x) >= 0 ? (x) : -(x))

/*
 * PCI config space access (in os.c)
 */
extern char *oss_pci_read_devpath (dev_info_t * dip);
