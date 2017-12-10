#ifndef _OS_H_
#define _OS_H_

/*
 * Purpose: OS specific definitions for BeOS/Haiku
 *
 */
/*
 *
 * This file is part of Open Sound System.
 *
 * Copyright (C) 4Front Technologies 1996-2007.
 *
 * This this source file is released under GPL v2 license (no other versions).
 * See the COPYING file included in the main directory of this source
 * distribution. Please contact sales@opensound.com for further info.
 *
 */
#define OS_VERSION "5"
#define __inline__ inline
#define __inline inline
#define EXTERN_C extern "C"

/*
 * Debugging and misc settings
 */
#undef DO_TIMINGS
#undef  MUTEX_CHECKS
#undef  MEMDEBUG
/* very verbose */
//#define DO_DEBUG_FUNC_CALLS
/* XXX */
#define DEBUG_KDLCMD 1
//#define ENABLE_IMUX 1
//#define ENABLE_SOFTOSS 1

/*
 * Disable support for per-application features such as /dev/dsp device
 * selection based on command name. Requires working GET_PROCESS_NAME
 * macro implementation.
 */
//Disable??
//#define APPLIST_SUPPORT
#define USE_DEVICE_SUBDIRS
/* no VDEV */
#undef VDEV_SUPPORT 

//XXX: try to avoid crashing
#define MIDI_DISABLED 1

#include <OS.h>
#include <stdarg.h>
//#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/param.h>
#include <oss_errno.h>
#include <sys/uio.h>
#include <fcntl.h>
#ifdef __HAIKU__
#include <poll.h>
#endif
//#include <sys/malloc.h>
#include <malloc.h>


#include <KernelExport.h>
#include <Drivers.h>
#include <PCI.h>

#ifndef ENOTSUP
#define ENOTSUP ENOSYS
#endif

#define OSS_CONTIG_AREA_NAME "OSS CONTIG MEM"
#define OSS_PCI_AREA_NAME "OSS PCI MEM"
#define OSS_WQ_SEM_NAME "OSS WAITQ: "

//#define DEVICE_PREFIX "audio/oss/"
#define DEVICE_PREFIX ""
#define DEVICE_NAME_LEN (32+sizeof(DEVICE_PREFIX))

#define OSS_CONFIG_FILE_PREFIX "oss_"

#ifdef _KERNEL_MODE

/* references to modules */
extern pci_module_info *gPCI;

#endif /* _KERNEL_MODE */

/*
 * Some integer types
 */
#if defined(amd64) || defined(sparc) // 64bit ? not yet
typedef uint64 oss_native_word;	/* Same as the address and status register size */
#else
typedef uint32 oss_native_word;	/* Same as the address and status register size */
#endif

typedef int64 oss_int64_t;		/* Signed 64 bit integer */
typedef uint64 oss_uint64_t;	/* Unsigned 64 bit integer */
typedef unsigned long offset_t;


extern void oss_cmn_err (int level, char *format, ...);
#define CE_CONT		0
#define CE_NOTE		1
#define CE_WARN		2
#define CE_PANIC	3
#define cmn_err oss_cmn_err

//snooze(1000000); \

#ifdef DO_DEBUG_FUNC_CALLS
/* verbose debugging */
#define FENTRY() \
dprintf("oss>%s()\n", __FUNCTION__)
#define FENTRYA(f,a...) \
dprintf("oss>%s(" f ")\n", __FUNCTION__, a)
#define FEXIT() \
dprintf("oss<%s\n", __FUNCTION__)
#define FEXITR(e) \
dprintf("oss<%s returned 0x%08lx\n", __FUNCTION__, (uint32)e)
#else
#define FENTRY() {}
#define FENTRYA(f,a...) {}
#define FEXIT() {}
#define FEXITR(e) {}
#endif

struct _dev_info_t
{
  //struct pci_dev *pcidev;
  struct pci_info pciinfo; //XXX: ptr or not ???
};
typedef struct _dev_info_t dev_info_t;


/*
 * timeout wrappers
 */
#undef timeout
#define timeout oss_timeout
#undef untimeout
#define untimeout oss_untimeout
typedef int timeout_id_t;
extern timeout_id_t oss_timeout (void (*func) (void *), void *arg,
				 unsigned long long ticks);
extern void oss_untimeout (timeout_id_t id);

#define uiomove oss_uiomove
typedef enum uio_rw
{ UIO_READ, UIO_WRITE } uio_rw_t;
struct uio
{
  char *ptr;
  int resid;
  int kernel_space;		/* Set if this uio points to a kernel space buffer */
  uio_rw_t rw;
};
typedef struct uio uio_t;
extern int oss_uiomove (void *address, size_t nbytes, enum uio_rw rwflag,
			uio_t * uio);
extern int oss_create_uio (uio_t * uiop, char *buf, size_t count, uio_rw_t rw,
			   int is_kernel);



/*
 * Mutexes
 */

#ifdef MUTEX_CHECKS
/* Debugging version */
struct _oss_mutex_t
{
  spinlock lock;
  thread_id owner;
  char *file;
  int line;
  int busy_flags;
#define CNTX_INTR		1
#define CNTX_USER		2
};

typedef struct _oss_mutex_t oss_mutex_t;
extern void debug_mutex_init (oss_mutex_t * mutex, char *file, int line);
extern void debug_mutex_destroy (oss_mutex_t * mutex, char *file, int line);
extern void debug_mutex_enter (oss_mutex_t * mutex, char *file, int line, oss_native_word *flags);
extern void debug_mutex_exit (oss_mutex_t * mutex, char *file, int line, oss_native_word *flags);

#define MUTEX_INIT(osdev, mutex, hier)			debug_mutex_init(&mutex, NULL, __FILE__, __LINE__)
#define MUTEX_CLEANUP(mutex)			debug_mutex_destroy(&mutex, __FILE__, __LINE__)
#define MUTEX_ENTER_IRQDISABLE(mutex, flags)	debug_mutex_enter(&mutex, __FILE__, __LINE__, &flags)
#define MUTEX_ENTER(mutex, flags)			debug_mutex_enter(&mutex, __FILE__, __LINE__, NULL)
#define MUTEX_EXIT_IRQRESTORE(mutex, flags)	debug_mutex_exit(&mutex, __FILE__, __LINE__, &flags)
#define MUTEX_EXIT(mutex, flags)			debug_mutex_exit(&mutex, __FILE__, __LINE__, NULL)
#else
typedef spinlock oss_mutex_t;
#define MUTEX_INIT(osdev, mutex, hier)			{ mutex = 0; }
#define MUTEX_CLEANUP(mutex)			
#define MUTEX_ENTER_IRQDISABLE(mutex, flags)	\
{ \
	flags = (oss_native_word) disable_interrupts(); \
	acquire_spinlock(&(mutex)); \
}
#define MUTEX_ENTER(mutex, flags)			acquire_spinlock(&(mutex))
#define MUTEX_EXIT_IRQRESTORE(mutex, flags)	\
{ \
	release_spinlock(&(mutex)); \
	restore_interrupts((cpu_status)(flags)); \
}
#define MUTEX_EXIT(mutex, flags)			release_spinlock(&(mutex))
#endif

/* The soundcard.h could be in a nonstandard place so include it here. */
#include "soundcard.h"

typedef struct udi_usb_devc udi_usb_devc;
struct _oss_device_t
{
#ifdef NEEDED_FOR_DRIVERS
  int instance;
  void *devc;
  char *name;
#endif

  int cardnum;
  int dev_type;
  int instance;
  int available;
  dev_info_t *dip;
  int unloaded;
  void *osid;
  void *devc;
  char *name;
  char *hw_info;
  char nick[16];
  char modname[32];
  char handle[32];
  int num_audio_engines;
  int num_audioplay, num_audiorec, num_audioduplex;
  int num_mididevs;
  int num_mixerdevs;
  int num_loopdevs;
  int first_mixer;	/* This must be set to -1 by osdev_create() */
  
  volatile int refcount;	/* Nonzero means that the device is needed by some other (virtual) driver. */

/* Interrupts */

  long irq; //XXX:init =-1
  oss_tophalf_handler_t tophalf_handler;
  oss_bottomhalf_handler_t bottomhalf_handler;
  oss_mutex_t mutex;

};

/* XXX we'll deal with select() later */
#undef ALLOW_SELECT
/* BeOS doesn't support mmap. Haiku does, so FIXME someday */
#undef ALLOW_BUFFER_MAPPING

/*
 * Sleep/wakeup
 */

#ifdef _KERNEL
struct oss_wait_queue
{
//XXX:
//  oss_mutex_t mutex;
  char name[32];
  sem_id sem;
  unsigned long flags;
//  struct selinfo poll_info;
};
#endif

/* Busy wait routine */
#define oss_udelay(d) spin((bigtime_t)d)


/* usec time base */
#undef HZ
/* XXX: might need to be lower to handle casts to long */
#define OSS_HZ 1000000

/* System wall timer access */
extern unsigned long oss_get_time (void);
#define GET_JIFFIES()	oss_get_time()


/*
 * INB() and OUTB() should be obvious. NOTE! The order of
 * paratemeters of OUTB() is different than on some other
 * operating systems.
 */

/* I/O Mapped devices */
#define INB(o, p)	gPCI->read_io_8(p)
#define INW(o, p)	gPCI->read_io_16(p)
#define INL(o, p)	gPCI->read_io_32(p)

#define OUTB(o, v, p)	gPCI->write_io_8(p,v)
#define OUTW(o, v, p)	gPCI->write_io_16(p,v)
#define OUTL(o, v, p)	gPCI->write_io_32(p,v)

/* Memory Mapped devices */
#define PCI_READB(osdev, addr)		*(volatile unsigned char *)(addr)
#define PCI_READW(osdev, addr)		*(volatile unsigned short *)(addr)
#define PCI_READL(osdev, addr)		*(volatile unsigned int *)(addr)

#define PCI_WRITEB(osdev, addr, data)	*(volatile unsigned char *)(addr)=data
#define PCI_WRITEW(osdev, addr, data)	*(volatile unsigned short *)(addr)=data
#define PCI_WRITEL(osdev, addr, data)	*(volatile unsigned int *)(addr)=data

typedef void *oss_dma_handle_t;

/*
 * When a error (such as EINVAL) is returned by a function,
 * the following macro is used. The driver assumes that a
 * error is signalled by returning a negative value.
 */

/* 
   KERNEL_MALLOC() allocates requested number of memory  and 
   KERNEL_FREE is used to free it. 
   These macros are never called from interrupt, in addition the
   nbytes will never be more than 4096 bytes. Generally the driver
   will allocate memory in blocks of 4k. If the kernel has just a
   page level memory allocation, 4K can be safely used as the size
   (the nbytes parameter can be ignored).
*/
#define	KERNEL_MALLOC(nbytes)	calloc(1,nbytes)
#define	KERNEL_FREE(addr)	{if (addr)free(addr);addr=NULL;}

extern void *oss_contig_malloc (oss_device_t * osdev, int sz,
				oss_uint64_t memlimit,
				oss_native_word * phaddr);
extern void oss_contig_free (oss_device_t * osdev, void *p, int sz);
extern oss_native_word oss_virt_to_bus (void *addr);
#define CONTIG_MALLOC(osdev, sz, memlimit, phaddr, handle)	oss_contig_malloc(osdev, sz, memlimit, phaddr)
#define CONTIG_FREE(osdev, p, sz, handle)	oss_contig_free(osdev, p, sz)

/*
 * PMALLOC is used to allocate memory that will get automatically freed when
 * OSS unloads. Usable for per-instance structures allocated when OSS modules
 * are being loaded.
 */
extern void *oss_pmalloc (size_t sz);

//#define PMALLOC(osdev, sz) oss_pmalloc(sz)

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
  //int flags;
  int acc_flags;
};
#define ISSET_FILE_FLAG(fileinfo, flag)  (fileinfo->acc_flags & (flag) ? 1:0)

#define OSS_OS "BeOS"
#define OSS_OS_LONGNAME "BeOS R" OS_VERSION

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
#ifndef POLLIN
/* events & revents - compatible with the B_SELECT_xxx definitions in Drivers.h */
#define POLLIN          0x0001          /* any readable data available */
#define POLLOUT         0x0002          /* file descriptor is writeable */
#define POLLRDNORM      POLLIN
#define POLLWRNORM      POLLOUT
#define POLLRDBAND      0x0008          /* priority readable data */
#define POLLWRBAND      0x0010          /* priority data can be written */
#define POLLPRI         0x0020          /* high priority readable data */
#endif

extern int detect_trace;
#define DDB(x) if (detect_trace) x

extern caddr_t oss_map_pci_mem (oss_device_t * osdev, int nr, int phaddr,
				int size);
extern void oss_unmap_pci_mem (void *addr);
#define MAP_PCI_IOADDR(osdev, nr, io) (oss_native_word)io
#define MAP_PCI_MEM(osdev, ix, phaddr, size) 	oss_map_pci_mem(osdev, ix, phaddr, size)
#define UNMAP_PCI_MEM(osdev, ix, ph, virt, size)	oss_unmap_pci_mem(virt)
#define UNMAP_PCI_IOADDR(osdev, ix)	{}

#define GET_PROCESS_PID(f)  find_thread(NULL)
#define GET_PROCESS_UID()	getuid()
#define GET_PROCESS_NAME(f) NULL

#define abs(x)                  ((x) >= 0 ? (x) : -(x))

/*
 * Interface with the front-end driver
 */
extern status_t init_osscore (void);
extern status_t uninit_osscore (void);
extern status_t oss_probe_pci (void);
extern status_t oss_load_drivers (void);
extern status_t oss_unload_all_drivers (void);

/*
 * osscore module for use by low-level driver
 */
#define OSS_CORE_MODULE_NAME "media/oss/oss_core/v1"
#define OSS_MODULES_PREFIX "media/oss/drivers/"
#define OSS_MODULES_SUFFIX "/v1"
#define OSS_MAKE_DRV_MOD_NAME(nick) OSS_MODULES_PREFIX nick OSS_MODULES_SUFFIX
typedef struct oss_drv_module_info {
  module_info minfo;
  int (*driver_probe)(void);
  int (*driver_attach)(oss_device_t *osdev);
  int (*driver_detach)(oss_device_t *osdev);
} oss_drv_module_info;

typedef struct oss_core_module_info {
  module_info minfo;
  status_t (*init_osscore) (void);
  status_t (*uninit_osscore) (void);
  const char **(*oss_publish_devices) (void);
  device_hooks *(*oss_get_driver_hooks) (void);
  status_t (*oss_probe_pci) (void);
  status_t (*oss_load_drivers) (void);
  status_t (*oss_unload_all_drivers) (void);
} oss_core_module_info;
extern oss_core_module_info *gOSSCore;

//XXX: not yet
#ifdef ASMODULES

struct oss_core_module {
  module_info minfo;
  void (*oss_foo)(void);
};
extern struct oss_core_info *gOSSCore;
#ifdef BUILDING_DRIVER
  #define oss_foo gOSSCore->oss_foo
#endif
#endif /* ASMODULES */
