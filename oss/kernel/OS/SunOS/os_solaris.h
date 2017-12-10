#ifndef _OS_H_
#define _OS_H_

/*
 * Purpose: OS specific definitions for Solaris
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

#define OS_VERSION "10"
#define SUNDDI
#define __EXTENDED__
#define Solaris

#if (!defined(i386) && !defined(x86_64)) || defined(CONFIG_OSS_FIXDEPOINT)
// Floating point is not supported or it's disabled
#undef CONFIG_OSS_VMIX_FLOAT
#endif

#define VDEV_SUPPORT
#define USE_DEVICE_SUBDIRS

#define __inline__ inline
#define __inline inline
#define EXTERN_C extern "C"

/*
 * Debugging and misc settings
 */
#undef  MUTEX_CHECKS
#undef  MEMDEBUG

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

#include <stdarg.h>
#include <sys/types.h>
#include <sys/types32.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <oss_errno.h>
#include <sys/file.h>
#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/map.h>
#include <sys/debug.h>
#include <sys/modctl.h>
#include <sys/kmem.h>
#include <sys/cmn_err.h>
#include <sys/open.h>
#include <sys/stat.h>
#ifndef sparc
#include <sys/dditypes.h>
#include <sys/ddidmareq.h>
#include <sys/dma_engine.h>
#endif
#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/fcntl.h>
#include <sys/ddidmareq.h>
#include <sys/ksynch.h>
#include <sys/poll.h>
#ifdef SOL9
#include <sys/cred.h>
#else
#include <sys/cred_impl.h>
#endif

#ifndef SOL9
#include <sys/ddifm.h>
#include <sys/fm/protocol.h>
#include <sys/fm/util.h>
#include <sys/fm/io/ddi.h>
#endif

#undef HZ
#define OSS_HZ hz

#ifdef sparc
/*
 * It's not possible to use SB Live! under Sparc architecture.
 */
#define AUDIGY_ONLY
#endif

/*
 * Need to use slightly larger buffer because otherwise the interrupt
 * rate will become too high for Solaris.
 */
#undef  SMALL_DMABUF_SIZE
#define SMALL_DMABUF_SIZE (16*1024)

/* The soundcard.h could be in a nonstandard place so include it here. */
#include "soundcard.h"

typedef struct udi_usb_devc udi_usb_devc;

struct _oss_device_t
{
  int cardnum;
  int dev_type;
  int instance;
  int available;
  dev_info_t *dip;
  void *osid;
  void *devc;
  char *name;
  char nick[16];
  char modname[32];
  char handle[32];
  int num_audio_engines;
  int num_audioplay, num_audiorec, num_audioduplex;
  int num_mididevs;
  int num_mixerdevs;
  int num_loopdevs;
  int first_mixer;	/* This must be set to -1 by osdev_create() */
  char *hw_info;

  volatile int refcount;	/* Nonzero means that the device is needed by some other (virtual) driver. */

/* Interrupts */

  ddi_iblock_cookie_t iblock_cookie;
  int intr_is_hilevel;
  oss_tophalf_handler_t tophalf_handler;
  oss_bottomhalf_handler_t bottomhalf_handler;
  int intrcount, ackcount;

#ifdef _KERNEL
/* PCI related fields */
  ddi_acc_handle_t pci_config_handle;
#define OSS_MAX_ACC_HANDLE	10
  ddi_acc_handle_t acc_handle[OSS_MAX_ACC_HANDLE];
  int swap_mode;		/* 0=DDI_STRUCTURE_NEVERSWAP_ACC, 1=DDI_STRUCTURE_LE_ACC */

/* USB fields */
  udi_usb_devc *usbdev;

#ifndef SOL9
/* Fault management (FMA) */
  int fm_capabilities;
#endif
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
  kcondvar_t cv;
  short pollevents;
  struct pollhead ph;
};
#endif

/* Busy wait routine */
#define oss_udelay drv_usecwait
/* System wall timer access */
#define GET_JIFFIES()	ddi_get_lbolt()

/*
 * Mutexes
 */

#ifdef MUTEX_CHECKS
/* Debugging version */
struct _oss_mutex_t
{
  kmutex_t mu;
  char *file;
  int line;
  int busy_flags;
#define CNTX_INTR		1
#define CNTX_USER		2
};

typedef struct _oss_mutex_t oss_mutex_t;
extern void debug_mutex_init (oss_mutex_t * mutex, void *dummy, int typ,
			      void *arg, char *file, int line);
extern void debug_mutex_destroy (oss_mutex_t * mutex, char *file, int line);
extern void debug_mutex_enter (oss_mutex_t * mutex, char *file, int line);
extern void debug_mutex_exit (oss_mutex_t * mutex, char *file, int line);

#define MUTEX_INIT(osdev, mutex, hier)			debug_mutex_init(&mutex, NULL, MUTEX_DRIVER, osdev->iblock_cookie, __FILE__, __LINE__)
#define MUTEX_CLEANUP(mutex)			debug_mutex_destroy(&mutex, __FILE__, __LINE__)
#define MUTEX_ENTER_IRQDISABLE(mutex, flags)	flags=0;debug_mutex_enter(&mutex, __FILE__, __LINE__)
#define MUTEX_ENTER(mutex, flags)			flags=0;debug_mutex_enter(&mutex, __FILE__, __LINE__)
#define MUTEX_EXIT_IRQRESTORE(mutex, flags)	debug_mutex_exit(&mutex, __FILE__, __LINE__);(flags)++
#define MUTEX_EXIT(mutex, flags)			debug_mutex_exit(&mutex, __FILE__, __LINE__);(flags)++
#else
typedef kmutex_t oss_mutex_t;
#define MUTEX_INIT(osdev, mutex, hier)			mutex_init(&mutex, NULL, MUTEX_DRIVER, osdev->iblock_cookie)
#define MUTEX_CLEANUP(mutex)			mutex_destroy(&mutex)
#define MUTEX_ENTER_IRQDISABLE(mutex, flags)	flags=0;mutex_enter(&mutex)
#define MUTEX_ENTER(mutex, flags)			flags=0;mutex_enter(&mutex)
#define MUTEX_EXIT_IRQRESTORE(mutex, flags)	mutex_exit(&mutex);(flags)++
#define MUTEX_EXIT(mutex, flags)			mutex_exit(&mutex);(flags)++
#endif

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

#ifdef sparc
#ifdef _KERNEL
extern void oss_put16 (ddi_acc_handle_t handle, unsigned short *addr,
		       unsigned short value);
extern void oss_put32 (ddi_acc_handle_t handle, unsigned int *addr,
		       unsigned int value);
extern unsigned short oss_get16 (ddi_acc_handle_t handle,
				 unsigned short *addr);
extern unsigned int oss_get32 (ddi_acc_handle_t handle, unsigned int *addr);

#define INB(osdev,port) ddi_get8((osdev)->acc_handle[0], (uint8_t*)(port))
#define OUTB(osdev, d, port) ddi_put8((osdev)->acc_handle[0], (uint8_t*)(port), (d))
#define INW(osdev,port) oss_get16((osdev)->acc_handle[0], (uint16_t*)(port))
#define OUTW(osdev, d, port) oss_put16((osdev)->acc_handle[0], (uint16_t*)(port), (d))
#define INL(osdev,port) oss_get32((osdev)->acc_handle[0], (uint32_t*)(port))
#define OUTL(osdev,d, port) oss_put32((osdev)->acc_handle[0], (uint32_t*)(port), (d))

/* Memory Mapped devices */
extern uint16_t oss_mem_get16 (ddi_acc_handle_t handle, uint16_t * addr);
extern uint32_t oss_mem_get32 (ddi_acc_handle_t handle, uint32_t * addr);
extern void oss_mem_put16 (ddi_acc_handle_t handle, uint16_t * add,
			   uint16_t v);
extern void oss_mem_put32 (ddi_acc_handle_t handle, uint32_t * add,
			   uint32_t v);
#define PCI_READW(osdev, addr)		oss_mem_get16((osdev)->acc_handle[0], (uint16_t*)(addr))
#define PCI_READL(osdev, addr)		oss_mem_get32((osdev)->acc_handle[0], (uint32_t*)(addr))
#define PCI_WRITEW(osdev, addr, data)	oss_mem_put16((osdev)->acc_handle[0], (uint16_t*)(addr), data)
#define PCI_WRITEL(osdev, addr, data)	oss_mem_put32((osdev)->acc_handle[0], (uint32_t*)(addr), data)
#endif
#else /* x86/AMD64 */
/* I/O Mapped devices */
#define INB(o, p)	inb(p)
#define INW(o, p)	inw(p)
#define INL(o, p)	inl(p)

#define OUTB(o, v, p)	outb(p,v)
#define OUTW(o, v, p)	outw(p,v)
#define OUTL(o, v, p)	outl(p,v)

/* Memory Mapped devices */
#define PCI_READW(osdev, addr)		ddi_mem_get16((osdev)->acc_handle[0], (uint16_t*)(addr))
#define PCI_READL(osdev, addr)		ddi_mem_get32((osdev)->acc_handle[0], (uint32_t*)(addr))
#define PCI_WRITEW(osdev, addr, data)	ddi_mem_put16((osdev)->acc_handle[0], (uint16_t*)(addr), data)
#define PCI_WRITEL(osdev, addr, data)	ddi_mem_put32((osdev)->acc_handle[0], (uint32_t*)(addr), data)
#endif

#define PCI_READB(osdev, addr)		ddi_mem_get8((osdev)->acc_handle[0], addr)
#define PCI_WRITEB(osdev, addr, data)	ddi_mem_put8((osdev)->acc_handle[0], addr, data)

typedef ddi_dma_handle_t oss_dma_handle_t;

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
extern void oss_kmem_free (void *addr);
#define KERNEL_MALLOC(nbytes)	oss_kmem_alloc(nbytes, KM_SLEEP, __FILE__, __LINE__)
#define KERNEL_FREE(addr)	oss_kmem_free(addr)
extern void *oss_contig_malloc (oss_device_t * osdev, int sz,
				oss_uint64_t memlimit,
				oss_native_word *phaddr, 
				oss_dma_handle_t *dma_handle,
				char * file, int line);
extern void oss_contig_free (oss_device_t * osdev, void *p, int sz);
#define CONTIG_MALLOC(osdev, sz, memlimit, phaddr, handle)	oss_contig_malloc(osdev, sz, memlimit, phaddr, &handle, __FILE__, __LINE__)
#define CONTIG_FREE(osdev, p, sz, handle)	oss_contig_free(osdev, p, sz, handle)
#else
extern void *oss_kmem_alloc (size_t size, int flags);
extern void oss_kmem_free (void *addr);
#define KERNEL_MALLOC(nbytes)	oss_kmem_alloc(nbytes, KM_SLEEP)
#define KERNEL_FREE(addr)	oss_kmem_free(addr)
extern void *oss_contig_malloc (oss_device_t * osdev, int sz,
				oss_uint64_t memlimit,
				oss_native_word * phaddr,
				oss_dma_handle_t *dma_handle);
extern void oss_contig_free (oss_device_t * osdev, void *p, int sz);
#define CONTIG_MALLOC(osdev, sz, memlimit, phaddr, handle)	oss_contig_malloc(osdev, sz, memlimit, phaddr, &handle)
#define CONTIG_FREE(osdev, p, sz, handle)	oss_contig_free(osdev, p, sz)
#endif

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

#define OSS_DMA_SYNC_INBOUND	DDI_DMA_SYNC_FORCPU
#define OSS_DMA_SYNC_OUTBOUND	DDI_DMA_SYNC_FORDEV
#define OSS_DMA_SYNC(handle, offs, len, direc) (ddi_dma_sync(handle, offs, len, direc)==DDI_SUCCESS)

#ifdef _KERNEL
struct os_dma_params
{
  int state;			/* 0=unavail, 1=avail, 2=busy */
  oss_device_t *osdev;
  ddi_dma_handle_t handle;
  ddi_acc_handle_t dma_acc_handle;
  ddi_dma_cookie_t cookie;
  ddi_dma_win_t win;
  ddi_dma_seg_t seg;
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

#ifdef sparc
#define OSS_OS "Sparc"
#define OSS_OS_LONGNAME "Solaris (Sparc)"
#else
#define OSS_OS "Solaris"
#define OSS_OS_LONGNAME "Solaris (x86)"
#endif

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

/* 
 * There is apparently no documented way to map DMA buffers allocated
 * using ddi_dma_mem_alloc() to applications. For this reason we cannot
 * support mmap() under Solaris.
 */
#undef  ALLOW_BUFFER_MAPPING

#if 0
extern int detect_trace;
#define DDB(x) if (detect_trace) x
#else
#define DDB(x) {}
#endif

/*
 * PCI I/O and memory address mapping.
 *
 * Note that for compatibility with the other operating systems supported by
 *      OSS the region numbering is different. So 0 means BAR0 while under
 *      Solaris (ddi_regs_map_setup) BAR0 is 1. oss_map_pci_ioaddr() will
 *      add 1 to the region number.
 */
extern caddr_t oss_map_pci_ioaddr (oss_device_t * osdev, int nr, int io);
extern void oss_unmap_pci_ioaddr(oss_device_t * osdev, int nr);
#define MAP_PCI_IOADDR(osdev, nr, io) (oss_native_word)oss_map_pci_ioaddr(osdev, nr, io)
#define MAP_PCI_MEM(osdev, ix, phaddr, size) 	oss_map_pci_ioaddr(osdev, ix, phaddr)
#define UNMAP_PCI_MEM(osdev, ix, ph, virt, size)	oss_unmap_pci_ioaddr(osdev, ix)
#define UNMAP_PCI_IOADDR(osdev, ix)	oss_unmap_pci_ioaddr(osdev, ix)

#define GET_PROCESS_PID(x)  ddi_get_pid()

/*
 * Instead of returning UID check if the process has PRIV_SYS_DEVICES privilege.
 * Report such users as UID=0 (root) and others as UID=1.
 */
#define GET_PROCESS_UID()   ((drv_priv(ddi_get_cred())==0) ? 0 : 1)

#if 1
/* TODO: This works OK but may cause crashes with different kernel versions/builds */
extern char *oss_get_procname (void);
#define GET_PROCESS_NAME(x) oss_get_procname()
#endif

#define FP_SUPPORT

#ifdef FP_SUPPORT
typedef short fp_env_t[512];
typedef unsigned int fp_flags_t[5];
extern int oss_fp_check (void);
extern void oss_fp_save (short *envbuf, fp_flags_t flags);
extern void oss_fp_restore (short *envbuf, fp_flags_t flags);
#   define FP_SAVE(envbuf, flags)		oss_fp_save(envbuf, flags)
#   define FP_RESTORE(envbuf, flags)		oss_fp_restore(envbuf, flags)
#endif

#define abs(x)                  ((x) >= 0 ? (x) : -(x))

extern int oss_open (dev_t * dev_p, int open_flags, int otyp,
		     cred_t * cred_p);
extern int oss_close (dev_t dev, int flag, int otyp, cred_t * cred_p);
extern int oss_ioctl (dev_t dev, int cmd, intptr_t arg, int mode,
		      cred_t * cred_p, int *rval_p);
extern int oss_read (dev_t dev, struct uio *uiop, cred_t * credp);
extern int oss_write (dev_t dev, struct uio *uiop, cred_t * cred_p);
extern int oss_chpoll (dev_t dev, short events, int anyyet, short *reventsp,
		       struct pollhead **phpp);
#ifdef _KERNEL
extern int oss_devmap (dev_t dev, devmap_cookie_t handle, offset_t off,
		       size_t len, size_t * maplen, uint_t model);

extern void *oss_memset (void *s, int c, size_t n);
#define memset oss_memset

extern void *oss_memcpy (void *s1, const void *s2, size_t n);
#define memcpy oss_memcpy
#endif

#define DISABLE_FMA
#if !defined(SOL9) && defined(DDI_FM_DEVICE) && !defined(DISABLE_FMA)
/*
 * Fault management (FMA) support.
 */

#define FMA_EREPORT(osdev, detail, name, type, value) \
{ \
	uint64_t ena; \
	char buf[FM_MAX_CLASS]; \
	(void) snprintf (buf, FM_MAX_CLASS, "%s.%s", DDI_FM_DEVICE, detail); \
	ena = fm_ena_generate(0, FM_ENA_FMT1); \
	if (osdev->dip != NULL && osdev->fm_capabilities != 0) \
	   ddi_fm_ereport_post(osdev->dip, buf, ena, DDI_NOSLEEP, FM_VERSION, DATA_TYPE_UINT8, FM_EREPORT_VERS0, name, type, value, NULL); \
}

#define FMA_IMPACT(osdev, impact) \
	if (osdev->fm_capabilities != 0) \
	   ddi_fm_service_impact(osdev->dip, impact)
#endif
