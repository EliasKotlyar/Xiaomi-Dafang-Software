#ifndef _OS_H_
#define _OS_H_

/*
 * Purpose: OS specific definitions for SCO OpenServer and SCO UnixWare (DDI8)
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
#define OpenServer

#if (!defined(i386) && !defined(x86_64)) || defined(CONFIG_OSS_FIXDEPOINT)
// Floating point is not supported or it's disabled
#undef CONFIG_OSS_VMIX_FLOAT
#endif

#if 0
// Enable kernel level debugging
#define DEBUG
/*
 * NOTE! Virtual devices (such as vmix, imux and midiloop) cannot be used
 *       when OSS is compiled with _LOCKTEST. Such drivers will not pass
 *       deadlock detection because virtual drivers make calls to the audio
 *       core and other drivers that use lower hierarchy levels. For the
 *       reason the system will crash as soon as the virtual devices get used.
 */
#define _LOCKTEST
#define MUTEX_CHECKS
#undef  MEMDEBUG
#endif

// Timing/tracing analysis stuff. This info can be read with the readtimings utility.

#ifdef DEBUG
#include <sys/debug.h>
#endif

#define VDEV_SUPPORT
#define USE_DEVICE_SUBDIRS

#define __inline__ inline
#define __inline inline
#define EXTERN_C extern "C"

/*
 * Do not export global parameters in oss_core_options.c since they will be
 * handled in Space.c for osscore
 */
#define NO_GLOBAL_OPTIONS

/*
 * Disable support for per-application features such as /dev/dsp device
 * selection based on command name. Requires working GET_PROCESS_NAME
 * macro implementation.
 */
#undef  APPLIST_SUPPORT

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
typedef unsigned long offset_t;

#include <stdarg.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <oss_errno.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/map.h>
#include <sys/debug.h>
#include <sys/kmem.h>
#include <sys/cmn_err.h>
#include <sys/open.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/ksynch.h>
#include <sys/confmgr.h>
#include <sys/cm_i386at.h>
#include <sys/poll.h>

/*
 * ddi.h must be the last include
 */
#include <sys/ddi.h>

#ifndef HZ
extern int hz;
#define HZ hz
#endif
#define OSS_HZ HZ

/*
 * Mutexes
 */

#ifdef _KERNEL
typedef lock_t *oss_mutex_t;
#else
typedef int oss_mutex_t;
#endif

/* The soundcard.h could be in a nonstandard place so include it here. */
#include "soundcard.h"

typedef struct udi_usb_devc udi_usb_devc;
typedef rm_key_t dev_info_t;

struct _oss_device_t
{
#ifdef _KERNEL
  int cardnum;
  int dev_type;
  int available;
  int instance;
  rm_key_t key;
  drvinfo_t *drvinfo;
  void *osid;
  void *devc;
  char *name;
  char nick[16];
  char modname[16];
  char handle[32];
  char *hw_info;
  int num_audio_engines;
  int num_audioplay, num_audiorec, num_audioduplex;
  int num_mididevs;
  int num_mixerdevs;
  int num_loopdevs;
  int first_mixer;	/* This must be set to -1 by osdev_create() */

  volatile int refcount;	/* Nonzero means that the device is needed by some other (virtual) driver. */

/* Interrupts */

  void *intr_cookie;		/* Returned by cm_intr_attach */
  oss_tophalf_handler_t tophalf_handler;
  oss_bottomhalf_handler_t bottomhalf_handler;
  oss_mutex_t mutex;

/* PCI configuration */
  int vendor, product;
#else
  int dummy;
#endif
};

/*
 * Support for select()
 */

struct _oss_poll_event_t
{
  short events, revents;
  struct pollhead *php;
  int anyyet;
};
typedef struct _oss_poll_event_t oss_poll_event_t;

#define ALLOW_SELECT
#define SEL_IN		0
#define SEL_OUT		1
#define SEL_EX		0xffffffff

/*
 * Sleep/wakeup
 */

#ifdef _KERNEL
struct oss_wait_queue
{
  sv_t *sv;
  short pollevents;
  volatile unsigned int flags;
};
#endif

#define FP_SUPPORT

#ifdef FP_SUPPORT
typedef short fp_env_t[512];
typedef unsigned int fp_flags_t[4];
extern int oss_fp_check (void);
extern void oss_fp_save (short *envbuf, fp_flags_t flags);
extern void oss_fp_restore (short *envbuf, fp_flags_t flags);
#   define FP_SAVE(envbuf, flags)		oss_fp_save(envbuf, flags)
#   define FP_RESTORE(envbuf, flags)		oss_fp_restore(envbuf, flags)
#endif

/* Busy wait routine */
#define oss_udelay drv_usecwait
/* System wall timer access */
#define GET_JIFFIES()	TICKS()

#ifdef MUTEX_CHECKS
extern void push_mutex (void *mutex, const char *file, int line);
extern void pop_mutex (void *mutex, const char *file, int line);
extern void print_mutexes (void);
#else
#define push_mutex(a, b, c)
#define pop_mutex(a, b, c)
#endif

#ifdef _LOCKTEST
#	define MUTEX_INIT(osdev, mutex, hier)		\
	{\
		static LKINFO_DECL(mtx_name, __FILE__ ":" #mutex, 0);\
		mutex=LOCK_ALLOC((uchar_t)(hier), pldisk, &mtx_name, KM_SLEEP);\
	}
#else
#	define MUTEX_INIT(osdev, mutex, hier)		\
	{\
		mutex=LOCK_ALLOC((uchar_t)(hier), pldisk, NULL, KM_SLEEP);\
	}
#endif

#define MUTEX_CLEANUP(mutex)			LOCK_DEALLOC(mutex)
#define MUTEX_ENTER_IRQDISABLE(mutex, flags)	{push_mutex(mutex, __FILE__, __LINE__);flags=LOCK(mutex, pldisk);}
#define MUTEX_ENTER(mutex, flags)		{push_mutex(mutex, __FILE__, __LINE__);flags=LOCK(mutex, pldisk);}
#define MUTEX_EXIT_IRQRESTORE(mutex, flags)	{pop_mutex(mutex, __FILE__, __LINE__);UNLOCK(mutex, flags);}
#define MUTEX_EXIT(mutex, flags)			{pop_mutex(mutex, __FILE__, __LINE__);UNLOCK(mutex, flags);}

/*
 * Move bytes from the buffer which the application given in a
 * write() call.
 * offs is position relative to the beginning of the buffer in
 * user space. The count is number of bytes to be moved.
 */
#define COPY_FROM_USER(target, source, offs, count) \
        if (uiomove((target), count, UIO_WRITE, source)) { \
                cmn_err(CE_WARN, "Bad copyin()!\n"); \
        }
/* Like COPY_FOM_USER but for writes. */
#define COPY_TO_USER(target, offs, source, count) \
        if (uiomove((source), count, UIO_READ, target)) { \
                cmn_err(CE_WARN, "Bad copyout()!\n"); \
        }

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
#define PCI_READB(osdev, addr)		*(volatile unsigned char *)(addr)
#define PCI_READW(osdev, addr)		*(volatile unsigned short *)(addr)
#define PCI_READL(osdev, addr)		*(volatile unsigned int *)(addr)

#define PCI_WRITEB(osdev, addr, data)	*(volatile unsigned char *)(addr)=data
#define PCI_WRITEW(osdev, addr, data)	*(volatile unsigned short *)(addr)=data
#define PCI_WRITEL(osdev, addr, data)	*(volatile unsigned int *)(addr)=data

#ifndef TRUE
#define TRUE  (1)
#define FALSE (0)
#endif

/* 
   KERNEL_MALLOC() allocates requested number of memory  and 
   KERNEL_FREE is used to free it. 
   These macros are never called from interrupt, in addition the
   nbytes will never be more than 4096 bytes. Generally the driver
   will allocate memory in blocks of 4k. If the kernel has just a
   page level memory allocation, 4K can be safely used as the size
   (the nbytes parameter can be ignored).
*/
#ifdef MEMDEBUG
extern void *oss_kmem_alloc (size_t size, int flags, char *file, int line);
extern void oss_kmem_free (void *addr, char *file, int line);
# define KERNEL_MALLOC(nbytes)	oss_kmem_alloc(nbytes, KM_SLEEP, __FILE__, __LINE__)
# define KERNEL_FREE(addr)	oss_kmem_free(addr, __FILE__, __LINE__)
#else
extern void *oss_kmem_alloc (size_t size, int flags);
extern void oss_kmem_free (void *addr);
# define KERNEL_MALLOC(nbytes)	oss_kmem_alloc(nbytes, KM_SLEEP)
# define KERNEL_FREE(addr)	oss_kmem_free(addr)
#endif

typedef void *oss_dma_handle_t;

extern void *oss_contig_malloc (oss_device_t * osdev, int sz,
				oss_uint64_t memlimit,
				oss_native_word * phaddr);
extern void oss_contig_free (oss_device_t * osdev, void *p, int sz);
extern oss_native_word oss_virt_to_bus (void *addr);
#define CONTIG_MALLOC(osdev, sz, memlimit, phaddr, handle)	oss_contig_malloc(osdev, sz, memlimit, phaddr)
#define CONTIG_FREE(osdev, p, sz, handle)	oss_contig_free(osdev, p, sz)

/*
 * Timer macros
 *
 * These macros are obsolete and should not be used in any new code.
 * Use the timeout mechanism (see the timeout(9F) Solaris man page).
 */
#define timeout(fn, arg, ticks) itimeout(fn, arg, ticks, pltimeout)
typedef int timeout_id_t;
#define DEFINE_TIMER(name, proc)	static timeout_id_t name = 0
#define REMOVE_TIMER(name, proc)	{if (name != 0) untimeout(name);}
#define INIT_TIMER(name,proc)
typedef void (*timeout_func_t) (void *);
#define ACTIVATE_TIMER(name, proc, time) \
	name=timeout((timeout_func_t)proc, (void*)&name, time)

#ifdef _KERNEL
struct os_dma_params
{
  int state;			/* 0=unavail, 1=avail, 2=busy */
  oss_device_t *osdev;
  void *orig_buf;

  volatile int enabled, ignore;
};
#define OS_DMA_PARMS \
	struct os_dma_params dma_parms;
#endif
#endif
struct fileinfo
{
  int mode;			/* Open mode */
  int acc_flags;
};
#define ISSET_FILE_FLAG(fileinfo, flag)  (fileinfo->acc_flags & (flag) ? 1:0)

#define OSS_OS "SCO"
#define OSS_OS_LONGNAME "SCO OpenServer/UnixWare"

#undef DMA_TRY_PSEUDOINIT

int get_dma_residue (int chn);
void disable_dma (int chn);
void enable_dma (int chn);

typedef void (*softintr_func_t) (int);

struct oss_softintr
{
  int id;
  softintr_func_t func;
  volatile int armed, running;
};

#undef  ALLOW_BUFFER_MAPPING

#undef  SMALL_DMABUF_SIZE
#define SMALL_DMABUF_SIZE (16*1024)

extern int detect_trace;
#define DDB(x) if (detect_trace) x

#define MAP_PCI_IOADDR(osdev, nr, io) 		io
#define MAP_PCI_MEM(osdev, ix, phaddr, size) 	devmem_mapin(osdev->key, ix, 0, size)
#define UNMAP_PCI_MEM(osdev, ix, ph, virt, size)	devmem_mapout(virt, size)
#define UNMAP_PCI_IOADDR(osdev, ix)	{}
#define GET_PROCESS_PID(x)  -1
#define GET_PROCESS_NAME(x) NULL

#define abs(x)                  ((x) >= 0 ? (x) : -(x))

#ifdef _KERNEL
extern void *oss_memset (void *s, int c, size_t n);
#define memset oss_memset

extern void *oss_memcpy (void *s1, const void *s2, size_t n);
#define memcpy oss_memcpy
int oss_sprintf (char *buf, char *cfmt, ...);
#define sprintf oss_sprintf

extern int oss_open (void *idata, channel_t * channelp,
		     int oflags, cred_t * crp, queue_t * q);
extern int oss_close (void *idata, channel_t channel,
		      int oflags, cred_t * crp, queue_t * q);
extern int oss_ioctl (void *idata, channel_t channel, int cmd, void *arg,
		      int oflags, cred_t * crp, int *rvalp);
extern int oss_devinfo (void *idata, channel_t channel, di_parm_t parm,
			void *valp);
extern void oss_biostart (void *idata, channel_t channel, buf_t * bp);

#endif
