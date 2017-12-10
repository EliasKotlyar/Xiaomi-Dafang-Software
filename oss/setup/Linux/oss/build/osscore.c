/*
 * Purpose: Linux kernel version specific wrapper routines.
 *
 * This file will be shipped in source format and compiled in the target
 * (customer) system. In this way minor changes between Linux versions
 * can be fixed by the customer.
 */

/*
 * Copyright (C) 4Front Technologies 2005-2014. Released under GPL2 license.
 */
//#include <linux/config.h>
typedef int *ioctl_arg;
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <stdarg.h>
#include <linux/vmalloc.h>
#include "timestamp.h"
#include "local_config.h"
#include "oss_exports.h"
#include "wrap.h"
#include "ossdip.h"
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/time.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/pci.h>
#include <linux/irq.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,10,0)
#include <linux/uidgid.h>
#endif
#undef strlen
#undef strcpy
#define strlen oss_strlen
#define strcpy oss_strcpy

typedef struct _smap_t dmap_t;

#include "soundcard.h"
#include "audio_core.h"
#include "mixer_core.h"
#include "ubuntu_version_hack.inc"

MODULE_LICENSE ("GPL v2");
MODULE_DESCRIPTION ("Open Sound System core services");
MODULE_AUTHOR ("4Front Technologies (support@opensound.com)");

struct _oss_mutex_t
{
  /* Caution! This definition must match cuckoo.h */
  spinlock_t lock;
  int filler;			/* Make sure this structure doesn't become zero length */
};

static oss_device_t *core_osdev = NULL;
/*
 * Minor device list
 */
#define MAX_MINOR 256
typedef struct
{
  int major, minor;
  char name[32];
} oss_minor_t;

static oss_minor_t minors[MAX_MINOR];
static int nminors = 0;

/* 
 * Sleep flags. Make sure these definitions match oss_config.h.
 */
#define WK_NONE		0x00
#define WK_WAKEUP	0x01
#define WK_TIMEOUT	0x02
#define WK_SIGNAL	0x04
#define WK_SLEEP	0x08
#define WK_SELECT	0x10

time_t
oss_get_time (void)
{
#if 1
  return get_seconds ();
#else
  return xtime.tv_sec;
#endif
}

void *oss_memset (void *t, int val, size_t l);

void *
oss_kmem_alloc (size_t size)
{
  void *ptr;
  if ((ptr = vmalloc (size)) == NULL)
    {
      oss_cmn_err (CE_WARN, "vmalloc(%d) failed\n", size);
      return NULL;
    }
  memset (ptr, 0, size);
  return ptr;
}

void
oss_kmem_free (void *addr)
{
  vfree (addr);
}

/* oss_pmalloc() moved to os_linux.c */

extern oss_native_word
oss_virt_to_bus (void *addr)
{
  return virt_to_bus (addr);
}

void *
oss_memcpy (void *t_, const void *f_, size_t l)
{
  unsigned char *t = t_;
  unsigned const char *f = f_;
  int i;

  for (i = 0; i < l; i++)
    *t++ = *f++;

  return t;
}

void *memmove(void *dest, const void *src, size_t n)
{
	return oss_memcpy(dest, src, n);
}

void *
oss_memset (void *t, int val, size_t l)
{
  char *c = t;
  while (l-- > 0)
    *c++ = val;

  return t;
}

int
oss_strcmp (const char *s1, const char *s2)
{
  while (*s1 && *s2)
    {
      if (*s1 != *s2)
	return *s1 - *s2;
      s1++;
      s2++;
    }

  return *s1 - *s2;
}

int
oss_strncmp (const char *s1, const char *s2, size_t len)
{
  while (*s1 && *s2 && len--)
    {
      if (*s1 != *s2)
	return *s1 - *s2;
      s1++;
      s2++;
    }

  return *s1 - *s2;
}

char *
oss_strcpy (char *s1, const char *s2)
{
  char *s = s1;

  while (*s2)
    *s1++ = *s2++;
  *s1 = '\0';
  return s;
}

size_t
oss_strlen (const char *s)
{
  int i;

  for (i = 0; s[i]; i++);

  return i;
}

char *
oss_strncpy (char *s1, const char *s2, size_t l)
{
  char *s = s1;
  int n = 0;

  while (*s2)
    {
      if (n++ >= l)
	return s;

      *s1++ = *s2++;
    }
  *s1 = '\0';
  return s;
}

int oss_hz = HZ;
extern int max_intrate;
extern int detect_trace;
extern int src_quality;
extern int flat_device_model;
extern int vmix_disabled;
extern int vmix_no_autoattach;
extern int vmix_loopdevs;
extern int ac97_amplifier;
extern int ac97_recselect;
extern int cooked_enable;
extern int dma_buffsize;
extern int excl_policy;
extern int mixer_muted;

module_param (oss_hz, int, S_IRUGO);
module_param (max_intrate, int, S_IRUGO);
module_param (detect_trace, int, S_IRUGO);
module_param (src_quality, int, S_IRUGO);
module_param (flat_device_model, int, S_IRUGO);
module_param (vmix_disabled, int, S_IRUGO);
module_param (vmix_no_autoattach, int, S_IRUGO);
module_param (vmix_loopdevs, int, S_IRUGO);
module_param (ac97_amplifier, int, S_IRUGO);
module_param (ac97_recselect, int, S_IRUGO);
module_param (cooked_enable, int, S_IRUGO);
module_param (dma_buffsize, int, S_IRUGO);
module_param (excl_policy, int, S_IRUGO);
module_param (mixer_muted, int, S_IRUGO);

static struct proc_dir_entry *oss_proc_root = NULL;
static struct proc_dir_entry *oss_proc_devfiles = NULL;

static ssize_t
oss_read_devfiles (struct file *file, char __user * buf, size_t count,
		   loff_t * ppos)
{
  static char tmp[8192];
  char *s;
  static int eof = 0;
  int i;

  if (eof)
    {
      eof = 0;
      return 0;
    }

  *tmp = 0;
  s = tmp;

  for (i = 0; i < nminors; i++)
    {
      if (strlen (tmp) > sizeof (tmp) - 20)
	{
	  printk (KERN_ALERT "osscore: Procfs buffer too small\n");
	  return -ENOMEM;
	}

      s += sprintf (s, "%s %d %d\n",
		    minors[i].name, minors[i].major, minors[i].minor);
    }

  if (copy_to_user (buf, (void *) tmp, strlen (tmp)))
    return -EFAULT;

  eof = 1;
  return strlen (tmp);
}


#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
static struct file_operations oss_proc_operations = {
  .read = oss_read_devfiles,
};
#else
static struct file_operations fops = { 
 .owner = THIS_MODULE,
 .read = oss_read_devfiles,
};
#endif


static void
init_proc_fs (void)
{

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)

  if ((oss_proc_root =
         create_proc_entry ("opensound", 0700 | S_IFDIR, NULL)) == NULL)
     {
         oss_cmn_err (CE_CONT, "Cannot create /proc/opensound\n");
         return;
     }

  if ((oss_proc_devfiles =
        create_proc_entry ("devfiles", 0600, oss_proc_root)) == NULL)
     {
         oss_cmn_err (CE_CONT, "Cannot create /proc/opensound/devfiles\n");
         return;
     }

  oss_proc_devfiles->proc_fops = &oss_proc_operations;

#else

  if ((oss_proc_root = 
         proc_mkdir ("opensound", NULL)) == NULL )
    {
   oss_cmn_err (CE_CONT, "Cannot create /proc/opensound\n");
   return;
    }

  if ((oss_proc_devfiles =
   proc_create ("devfiles", 0600, oss_proc_root, &fops)) == NULL)
    {
   oss_cmn_err (CE_CONT, "Cannot create /proc/opensound/devfiles\n");
         return;
    }

#endif
 
}

static void
uninit_proc_fs (void)
{
  if (oss_proc_root)
    {
      if (oss_proc_devfiles)
	remove_proc_entry ("devfiles", oss_proc_root);
      remove_proc_entry ("opensound", NULL);
    }
}

static int
osscore_init (void)
{
  if ((core_osdev =
       osdev_create (NULL, DRV_UNKNOWN, 0, "osscore", NULL)) == NULL)
    {
      oss_cmn_err (CE_WARN, "Failed to allocate OSDEV structure\n");
      return -ENOMEM;
    }

  osdev_set_owner (core_osdev, THIS_MODULE);

  init_proc_fs ();

  return oss_init_osscore (core_osdev);
}

static void
osscore_exit (void)
{
  uninit_proc_fs ();
  oss_uninit_osscore (core_osdev);
}

void
oss_udelay (unsigned long d)
{
  udelay (d);
}

oss_mutex_t
oss_mutex_init (void)
{
  oss_mutex_t mutex;

  if ((mutex = vmalloc (sizeof (*mutex))) == NULL)
    {
      oss_cmn_err (CE_WARN, "vmalloc(%d) failed (mutex)\n", sizeof (*mutex));
      return NULL;
    }

  spin_lock_init (&(mutex->lock));

  return mutex;
}

void
oss_mutex_cleanup (oss_mutex_t mutex)
{
  vfree (mutex);
}

void
oss_spin_lock_irqsave (oss_mutex_t mutex, oss_native_word * flags)
{
  unsigned long flag;
  if (mutex == NULL)
    return;
  spin_lock_irqsave (&mutex->lock, flag);
  *flags = flag;
}

void
oss_spin_unlock_irqrestore (oss_mutex_t mutex, oss_native_word flags)
{
  if (mutex == NULL)
    return;
  spin_unlock_irqrestore (&mutex->lock, flags);
}

void
oss_spin_lock (oss_mutex_t mutex)
{
  if (mutex == NULL)
    return;
  spin_lock (&mutex->lock);
}

void
oss_spin_unlock (oss_mutex_t mutex)
{
  if (mutex == NULL)
    return;
  spin_unlock (&mutex->lock);
}

void *
oss_map_pci_mem (oss_device_t * osdev, int size, unsigned long offset)
{
#ifdef __arm__
  return (void*)offset;
#else
  return ioremap (offset, size);
#endif
}

void
oss_unmap_pci_mem (void *addr)
{
#ifndef __arm__
  iounmap (addr);
#endif
}

unsigned long long
oss_get_jiffies (void)
{
  return jiffies_64;
}

char *
oss_get_procname (void)
{
  return current->comm;
}

int
oss_get_pid (void)
{
  return current->pid;
}

unsigned int
oss_get_uid (void)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)
  return current->cred->uid.val;
#else 
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0)
  return __kuid_val(current->cred->uid);
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
  return current->cred->uid;
#else
  return current->uid;
#endif
#endif
  return 0;
}

typedef struct tmout_desc
{
  volatile int active;
  int timestamp;
  void (*func) (void *);
  void *arg;

  struct timer_list timer;
} tmout_desc_t;

static volatile int next_id = 0;
#define MAX_TMOUTS 128

tmout_desc_t tmouts[MAX_TMOUTS] = { {0} };

int timeout_random = 0x12123400;

void
oss_timer_callback (unsigned long id)
{
  tmout_desc_t *tmout;
  int ix;
  void *arg;

  timeout_random++;

  ix = id & 0xff;
  if (ix < 0 || ix >= MAX_TMOUTS)
    return;
  tmout = &tmouts[ix];

  if (tmout->timestamp != id)	/* Expired timer */
    return;

  if (!tmout->active)
    return;

  arg = tmout->arg;
  tmout->active = 0;
  tmout->timestamp = 0;

  tmout->func (arg);
}

timeout_id_t
oss_timeout (void (*func) (void *), void *arg, unsigned long long ticks)
{
  tmout_desc_t *tmout = NULL;
  int id, n;

  timeout_random++;

  n = 0;
  id = -1;

  while (id == -1 && n < MAX_TMOUTS)
    {
      if (!tmouts[next_id].active)
	{
	  tmouts[next_id].active = 1;
	  id = next_id++;
	  tmout = &tmouts[id];
	  break;
	}

      next_id = (next_id + 1) % MAX_TMOUTS;
    }

  if (id == -1)			/* No timer slots available */
    {
      oss_cmn_err (CE_WARN, "Timeout table full\n");
      return 0;
    }

  tmout->func = func;
  tmout->arg = arg;
  tmout->timestamp = id | (timeout_random & ~0xff);

  init_timer (&tmout->timer);
  tmout->timer.expires = jiffies + ticks;
  tmout->timer.data = id | (timeout_random & ~0xff);
  tmout->timer.function = oss_timer_callback;
  add_timer (&tmout->timer);

  return id | (timeout_random & ~0xff);
}

void
oss_untimeout (timeout_id_t id)
{
  tmout_desc_t *tmout;
  int ix;

  ix = id & 0xff;
  if (ix < 0 || ix >= MAX_TMOUTS)
    return;

  timeout_random++;
  tmout = &tmouts[ix];

  if (tmout->timestamp != id)	/* Expired timer */
    return;
  if (tmout->active)
    del_timer (&tmout->timer);
  tmout->active = 0;
  tmout->timestamp = 0;
}

int
oss_uiomove (void *addr, size_t nbytes, enum uio_rw rwflag, uio_t * uio)
{
/*
 * NOTE! Returns 0 upon success and EFAULT on failure (instead of -EFAULT
 * (for Solaris/BSD compatibilityi)).
 */

  int c;
  char *address = addr;

  if (rwflag != uio->rw)
    {
      oss_cmn_err (CE_WARN, "uiomove: Bad direction\n");
      return EFAULT;
    }

  if (uio->resid < nbytes)
    {
      oss_cmn_err (CE_WARN, "uiomove: Bad count %d (%d)\n", nbytes,
		   uio->resid);
      return EFAULT;
    }

  if (uio->kernel_space)
    return EFAULT;

  switch (rwflag)
    {
    case UIO_READ:
      c = nbytes;
      if (c > 10)
	c = 0;

      if ((c = copy_to_user (uio->ptr, address, nbytes) != 0))
	{
	  uio->resid -= nbytes;
	  oss_cmn_err (CE_CONT, "copy_to_user(%d) failed (%d)\n", nbytes, c);
	  return EFAULT;
	}
      break;

    case UIO_WRITE:
      if (copy_from_user (address, uio->ptr, nbytes) != 0)
	{
	  oss_cmn_err (CE_CONT, "copy_from_user failed\n");
	  uio->resid -= nbytes;
	  return EFAULT;
	}
      break;
    }

  uio->resid -= nbytes;
  uio->ptr += nbytes;

  return 0;
}

int
oss_create_uio (uio_t * uio, char *buf, size_t count, uio_rw_t rw,
		int is_kernel)
{
  memset (uio, 0, sizeof (*uio));

  if (is_kernel)
    {
      oss_cmn_err (CE_CONT,
		   "oss_create_uio: Kernel space buffers not supported\n");
      return -EIO;
    }

  uio->ptr = buf;
  uio->resid = count;
  uio->kernel_space = is_kernel;
  uio->rw = rw;

  return 0;
}

void
oss_cmn_err (int level, const char *s, ...)
{
  char tmp[1024], *a[6];
  va_list ap;
  int i, n = 0;

  va_start (ap, s);

  for (i = 0; i < strlen (s); i++)
    if (s[i] == '%')
      n++;

  for (i = 0; i < n && i < 6; i++)
    a[i] = va_arg (ap, char *);

  for (i = n; i < 6; i++)
    a[i] = NULL;

  if (level == CE_CONT)
    {
      sprintf (tmp, s, a[0], a[1], a[2], a[3], a[4], a[5], NULL,
	       NULL, NULL, NULL);
      printk ("%s", tmp);
    }
  else
    {
      strcpy (tmp, "osscore: ");
      sprintf (tmp + strlen (tmp), s, a[0], a[1], a[2], a[3], a[4], a[5],
	       NULL, NULL, NULL, NULL);
      if (level == CE_PANIC)
	panic (tmp);

      printk (KERN_ALERT "%s", tmp);
    }
#if 0
  /* This may cause a crash under SMP */
  if (sound_started)
    store_msg (tmp);
#endif

  va_end (ap);
}

/*
 * Sleep/wakeup
 */

struct oss_wait_queue
{
  volatile int flags;
  wait_queue_head_t wq;
};

struct oss_wait_queue *
oss_create_wait_queue (oss_device_t * osdev, const char *name)
{
  struct oss_wait_queue *wq;

  if ((wq = vmalloc (sizeof (*wq))) == NULL)
    {
      oss_cmn_err (CE_WARN, "vmalloc(%d) failed (wq)\n", sizeof (*wq));
      return NULL;
    }
  init_waitqueue_head (&wq->wq);

  return wq;
}

void
oss_reset_wait_queue (struct oss_wait_queue *wq)
{
  wq->flags = 0;
}

void
oss_remove_wait_queue (struct oss_wait_queue *wq)
{
  vfree (wq);
}

int
oss_sleep (struct oss_wait_queue *wq, oss_mutex_t * mutex, int ticks,
	   oss_native_word * flags, unsigned int *status)
{
  int result = 0;
  *status = 0;

  if (wq == NULL)
    return 0;

  wq->flags = 0;
  oss_spin_unlock_irqrestore (*mutex, *flags);

  if (ticks <= 0)
    result = wait_event_interruptible (wq->wq, (wq->flags & WK_WAKEUP));
  else
    result =
      wait_event_interruptible_timeout (wq->wq, (wq->flags & WK_WAKEUP),
					ticks);

  oss_spin_lock_irqsave (*mutex, flags);

  if (result < 0)		/* Signal received */
    {
      *status |= WK_SIGNAL;
      return 1;
    }

  if (!(wq->flags & WK_WAKEUP))	/* Timeout */
    {
      return 0;
    }

  return 1;
}

int
oss_register_poll (struct oss_wait_queue *wq, oss_mutex_t * mutex,
		   oss_native_word * flags, oss_poll_event_t * ev)
{
  oss_spin_unlock_irqrestore (*mutex, *flags);
  poll_wait ((struct file *) ev->file, &wq->wq,
	     (struct poll_table_struct *) ev->wait);
  oss_spin_lock_irqsave (*mutex, flags);
  return 0;
}

void
oss_wakeup (struct oss_wait_queue *wq, oss_mutex_t * mutex,
	    oss_native_word * flags, short events)
{
  if (wq == NULL)
    return;

  wq->flags |= WK_WAKEUP;
  oss_spin_unlock_irqrestore (*mutex, *flags);
  wake_up (&wq->wq);
  oss_spin_lock_irqsave (*mutex, flags);
}

void
oss_reserve_pages (oss_native_word start_addr, oss_native_word end_addr)
{
  struct page *page, *lastpage;

  lastpage = virt_to_page (end_addr);

  for (page = virt_to_page (start_addr); page <= lastpage; page++)
    set_bit (PG_reserved, &page->flags);
}

void
oss_unreserve_pages (oss_native_word start_addr, oss_native_word end_addr)
{
  struct page *page, *lastpage;

  lastpage = virt_to_page (end_addr);

  for (page = virt_to_page (start_addr); page <= lastpage; page++)
    clear_bit (PG_reserved, &page->flags);
}

void *
oss_contig_malloc (oss_device_t * osdev, int buffsize, oss_uint64_t memlimit,
		   oss_native_word * phaddr)
{
  char *start_addr, *end_addr;
  int sz, size;
  int flags = 0;

  *phaddr = 0;

#ifdef GFP_DMA32
  if (memlimit < 0x0000000100000000LL)
    flags |= GFP_DMA32;
#endif

  if (memlimit < 0x00000000ffffffffLL)
    flags |= GFP_DMA;

  start_addr = NULL;

  for (sz = 0, size = PAGE_SIZE; size < buffsize; sz++, size <<= 1);

  if (buffsize != (PAGE_SIZE * (1 << sz)))
    {
#if 0
      printk
	("Contig_malloc: Invalid size %d != %ld\n", buffsize,
	 PAGE_SIZE * (1 << sz));
#endif
    }

  start_addr = (char *) __get_free_pages (GFP_KERNEL | flags, sz);

  if (start_addr == NULL)
    {
      cmn_err (CE_NOTE, "Failed to allocate memory buffer of %d bytes\n",
	       buffsize);
      return NULL;
    }
  else
    {
      /* make some checks */
      end_addr = start_addr + buffsize - 1;

      oss_reserve_pages ((oss_native_word) start_addr,
			 (oss_native_word) end_addr);
    }

  *phaddr = virt_to_bus (start_addr);
  return start_addr;
}

void
oss_contig_free (oss_device_t * osdev, void *p, int buffsize)
{
  int sz, size;
  caddr_t start_addr, end_addr;

  if (p == NULL)
    return;

  for (sz = 0, size = PAGE_SIZE; size < buffsize; sz++, size <<= 1);

  start_addr = p;
  end_addr = p + buffsize - 1;

  oss_unreserve_pages ((oss_native_word) start_addr,
		       (oss_native_word) end_addr);
  free_pages ((unsigned long) p, sz);
}

/*
 * These typedefs must match definition of struct file_operations.
 * (See notes in routine oss_register_chrdev).
 */
typedef ssize_t (*read_t) (struct file *, char *, size_t, loff_t *);
typedef ssize_t (*write_t) (struct file *, const char *, size_t, loff_t *);
typedef unsigned int (*poll_t) (struct file *, poll_table *);
typedef poll_table select_table;

typedef int (*readdir_t) (struct inode *, struct file *, void *, filldir_t);
typedef int (*ioctl_t) (struct inode *, struct file *, unsigned int,
			unsigned long);
typedef long (*new_ioctl_t) (struct file *, unsigned int, unsigned long);
typedef int (*mmap_t) (struct file *, struct vm_area_struct *);
typedef int (*open_t) (struct inode *, struct file *);

typedef int (*release_t) (struct inode *, struct file *);
typedef int (*fasync_t) (int, struct file *, int);
typedef int (*fsync_t) (struct inode *, struct file *);

static loff_t
oss_no_llseek (struct file *file, loff_t offset, int orig)
{
  return -EINVAL;
}

#if 0
static int
oss_no_fsync (struct file *f, struct dentry *d, int datasync)
{
  return -EINVAL;
}

static int
oss_no_fasync (int x, struct file *f, int m)
{
  return -EINVAL;
}
#endif

/*
 *	Wrappers for installing and uninstalling character and block devices.
 *
 *	The oss_file_operation_handle structure differs from kernel's
 *	file_operations structure in parameters of the driver entry points.
 *	Kernel inode, file, wait_struct, vm_are_struct etc. typed arguments
 *	are replaced by wrapper handles.
 */

static struct file_operations *
alloc_fop (oss_device_t * osdev, struct oss_file_operation_handle *op)
{
/*
 *	This routine performs initialization of kernel file_operations structure
 *	from oss_file_operation_handle structure. Each procedure pointer is copied
 *	to a temporary variable before doing the actual assignment. This 
 *	prevents unnecessary warnings while it still enables compatibility
 *	warnings.
 *
 *	Any warning given by this routine may indicate that kernel fs
 *	call interface has changed significantly (added parameters to the routines).
 *	In this case definition routine in oss_file_operation_handle must be updated
 *	and WRAPPER_VERSION must be incremented.
 */

  struct file_operations *fop;

  poll_t tmp_poll = (poll_t) op->poll;
  read_t tmp_read = (read_t) op->read;
  write_t tmp_write = (write_t) op->write;
  /* readdir_t tmp_readdir = (readdir_t)op->readdir; */
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,35)
  ioctl_t tmp_ioctl = (ioctl_t) op->ioctl;
#endif
  mmap_t tmp_mmap = (mmap_t) op->mmap;
  open_t tmp_open = (open_t) op->open;
  release_t tmp_release = (release_t) op->release;
  new_ioctl_t tmp_unlocked_ioctl = (new_ioctl_t) op->unlocked_ioctl;
  new_ioctl_t tmp_compat_ioctl = (new_ioctl_t) op->compat_ioctl;

  fop = (struct file_operations *)
    oss_kmem_alloc (sizeof (struct file_operations));

  memset ((char *) fop, 0, sizeof (struct file_operations));

/*
 *	Now the assignment
 */
  fop->llseek = oss_no_llseek;
  fop->read = tmp_read;
  fop->write = tmp_write;
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,11,0)
  fop->readdir = NULL;		/* tmp_readdir; */
#endif
  fop->poll = tmp_poll;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,35)
  fop->ioctl = tmp_ioctl;
#endif
  fop->mmap = tmp_mmap;
  fop->open = tmp_open;
  fop->release = tmp_release;
  fop->fsync = NULL; /* oss_no_fsync; */
  fop->fasync = NULL; /* oss_no_fasync; */
  fop->lock = NULL;
  fop->flush = NULL;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
  fop->owner = osdev_get_owner (osdev);
#endif
#ifdef HAVE_UNLOCKED_IOCTL
  fop->unlocked_ioctl = tmp_unlocked_ioctl;
#endif
#ifdef HAVE_COMPAT_IOCTL
  fop->compat_ioctl = tmp_compat_ioctl;
#endif

  return fop;
}

int
oss_register_chrdev (oss_device_t * osdev, unsigned int major,
		     const char *name, struct oss_file_operation_handle *op)
{
  int maj;
  maj = register_chrdev (major, name, alloc_fop (osdev, op));

  return maj;
}

void
oss_register_minor (int major, int minor, char *name)
{
  if (nminors >= MAX_MINOR)
    {
      oss_cmn_err (CE_WARN, "Too many device files\n");
      return;
    }

  minors[nminors].major = major;
  minors[nminors].minor = minor;
  strcpy (minors[nminors].name, name);
  nminors++;
}

int
oss_unregister_chrdev (unsigned int major, const char *name)
{
  unregister_chrdev (major, name);
  return 0;
}

int
oss_inode_get_minor (oss_inode_handle_t * inode)
{
  return MINOR (((struct inode *) inode)->i_rdev);
}

int
oss_file_get_flags (oss_file_handle_t * file)
{
  return ((struct file *) file)->f_flags;
}

void *
oss_file_get_private (oss_file_handle_t * file)
{
  return ((struct file *) file)->private_data;
}

void
oss_file_set_private (oss_file_handle_t * file, void *v)
{
  ((struct file *) file)->private_data = v;
}

int
oss_vma_get_flags (oss_vm_area_handle_t * vma)
{
  return ((struct vm_area_struct *) vma)->vm_flags;
}

int
oss_do_mmap (oss_vm_area_handle_t * v, oss_native_word dmabuf_phys,
	     int bytes_in_use)
{
  struct vm_area_struct *vma = (struct vm_area_struct *) v;
  int res, size;

  if (vma->vm_pgoff != 0)
    {
      oss_cmn_err (CE_WARN, "mmap() offset must be 0.\n");
      return -EINVAL;
    }

  size = vma->vm_end - vma->vm_start;

  if (size != bytes_in_use)
    {
      oss_cmn_err (CE_WARN, "mmap() size = %ld. Should be %d\n",
		   size, bytes_in_use);
      return -EBUSY;
    }

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,13)
  res = io_remap_page_range (vma, vma->vm_start,
			     dmabuf_phys, size, vma->vm_page_prot);

#else
  res = io_remap_pfn_range (vma, vma->vm_start,
			    dmabuf_phys >> PAGE_SHIFT,
			    size, vma->vm_page_prot);
#endif

  if (res)
    {
      oss_cmn_err (CE_WARN, "io_remap_pfn_range returned %d\n", res);
      return -EAGAIN;
    }

  return 0;
}

/* As a special exception, if you link this library with other files,
   some of which are compiled with GCC, to produce an executable,
   this library does not by itself cause the resulting executable
   to be covered by the GNU General Public License.
   This exception does not however invalidate any other reasons why
   the executable file might be covered by the GNU General Public License.  */

typedef unsigned int UQItype __attribute__ ((mode (QI)));
typedef int SItype __attribute__ ((mode (SI)));
typedef unsigned int USItype __attribute__ ((mode (SI)));
typedef int DItype __attribute__ ((mode (DI)));
typedef unsigned int UDItype __attribute__ ((mode (DI)));

typedef int word_type __attribute__ ((mode (__word__)));

/* DIstructs are pairs of SItype values in the order determined by
   little/big ENDIAN.  */

#if defined(__i386__) || defined(__x86_64__)
struct DIstruct
{
  SItype low, high;
};
#endif


#ifndef __arm__
/* We need this union to unpack/pack DImode values, since we don't have
   any arithmetic yet.  Incoming DImode parameters are stored into the
   `ll' field, and the unpacked result is read from the struct `s'.  */

typedef union
{
  struct DIstruct s;
  DItype ll;
} DIunion;


/*  From gcc/longlong.h  */

#ifndef SI_TYPE_SIZE
#define SI_TYPE_SIZE 32
#endif

#define __BITS4 (SI_TYPE_SIZE / 4)
#define __ll_B (1L << (SI_TYPE_SIZE / 2))
#define __ll_lowpart(t) ((USItype) (t) % __ll_B)
#define __ll_highpart(t) ((USItype) (t) / __ll_B)

#if defined(__i386__) || defined(__x86_64__)
#define sub_ddmmss(sh, sl, ah, al, bh, bl) \
  __asm__ ("subl %5,%1\n"						\
	"sbbl %3,%0"							\
	   : "=r" ((USItype) (sh)),					\
	     "=&r" ((USItype) (sl))					\
	   : "0" ((USItype) (ah)),					\
	     "g" ((USItype) (bh)),					\
	     "1" ((USItype) (al)),					\
	     "g" ((USItype) (bl)))
#define umul_ppmm(w1, w0, u, v) \
  __asm__ ("mull %3"							\
	   : "=a" ((USItype) (w0)),					\
	     "=d" ((USItype) (w1))					\
	   : "%0" ((USItype) (u)),					\
	     "rm" ((USItype) (v)))
#define udiv_qrnnd(q, r, n1, n0, d) \
  __asm__ ("divl %4"							\
	   : "=a" ((USItype) (q)),					\
	     "=d" ((USItype) (r))					\
	   : "0" ((USItype) (n0)),					\
	     "1" ((USItype) (n1)),					\
	     "rm" ((USItype) (d)))
#define count_leading_zeros(count, x) \
  do {									\
    USItype __cbtmp;							\
    __asm__ ("bsrl %1,%0"						\
	     : "=r" (__cbtmp) : "rm" ((USItype) (x)));			\
    (count) = __cbtmp ^ 31;						\
  } while (0)
#endif /* __i386__ */

/* If this machine has no inline assembler, use C macros.  */

/* Define this unconditionally, so it can be used for debugging.  */
#define __udiv_qrnnd_c(q, r, n1, n0, d)  \
  do { \
    USItype __d1, __d0, __q1, __q0;                                     \
    USItype __r1, __r0, __m;                                            \
    __d1 = __ll_highpart (d);                                           \
    __d0 = __ll_lowpart (d);                                            \
                                                                        \
    __r1 = (n1) % __d1;                                                 \
    __q1 = (n1) / __d1;                                                 \
    __m = (USItype) __q1 * __d0;                                        \
    __r1 = __r1 * __ll_B | __ll_highpart (n0);                          \
    if (__r1 < __m)                                                     \
      {                                                                 \
        __q1--, __r1 += (d);                                            \
        if (__r1 >= (d)) /* i.e. we didn't get carry when adding to __r1 */\
          if (__r1 < __m)                                               \
            __q1--, __r1 += (d);                                        \
      }                                                                 \
    __r1 -= __m;                                                        \
                                                                        \
    __r0 = __r1 % __d1;                                                 \
    __q0 = __r1 / __d1;                                                 \
    __m = (USItype) __q0 * __d0;                                        \
    __r0 = __r0 * __ll_B | __ll_lowpart (n0);                           \
    if (__r0 < __m)                                                     \
      {                                                                 \
        __q0--, __r0 += (d);                                            \
        if (__r0 >= (d))                                                \
          if (__r0 < __m)                                               \
            __q0--, __r0 += (d);                                        \
      }                                                                 \
    __r0 -= __m;                                                        \
                                                                        \
    (q) = (USItype) __q1 * __ll_B | __q0;                               \
    (r) = __r0;                                                         \
  } while (0)

/* If udiv_qrnnd was not defined for this processor, use __udiv_qrnnd_c.  */
#if !defined (udiv_qrnnd)
#define UDIV_NEEDS_NORMALIZATION 1
#define udiv_qrnnd __udiv_qrnnd_c
#endif

/*  End of gcc/longlong.h  */


static inline DItype
__negdi2 (DItype u)
{
  DIunion w;
  DIunion uu;

  uu.ll = u;

  w.s.low = -uu.s.low;
  w.s.high = -uu.s.high - ((USItype) w.s.low > 0);

  return w.ll;
}

static inline UDItype
__udivmoddi4 (UDItype n, UDItype d, UDItype * rp)
{
  DIunion ww;
  DIunion nn, dd;
  DIunion rr;
  USItype d0, d1, n0, n1, n2;
  USItype q0, q1;
  USItype b, bm;

  nn.ll = n;
  dd.ll = d;

  d0 = dd.s.low;
  d1 = dd.s.high;
  n0 = nn.s.low;
  n1 = nn.s.high;

#ifndef UDIV_NEEDS_NORMALIZATION
  if (d1 == 0)
    {
      if (d0 > n1)
	{
	  /* 0q = nn / 0D */

	  udiv_qrnnd (q0, n0, n1, n0, d0);
	  q1 = 0;

	  /* Remainder in n0.  */
	}
      else
	{
	  /* qq = NN / 0d */

	  if (d0 == 0)
	    d0 = 1 / d0;	/* Divide intentionally by zero.  */

	  udiv_qrnnd (q1, n1, 0, n1, d0);
	  udiv_qrnnd (q0, n0, n1, n0, d0);

	  /* Remainder in n0.  */
	}

      if (rp != 0)
	{
	  rr.s.low = n0;
	  rr.s.high = 0;
	  *rp = rr.ll;
	}
    }

#else /* UDIV_NEEDS_NORMALIZATION */

  if (d1 == 0)
    {
      if (d0 > n1)
	{
	  /* 0q = nn / 0D */

	  count_leading_zeros (bm, d0);

	  if (bm != 0)
	    {
	      /* Normalize, i.e. make the most significant bit of the
	         denominator set.  */

	      d0 = d0 << bm;
	      n1 = (n1 << bm) | (n0 >> (SI_TYPE_SIZE - bm));
	      n0 = n0 << bm;
	    }

	  udiv_qrnnd (q0, n0, n1, n0, d0);
	  q1 = 0;

	  /* Remainder in n0 >> bm.  */
	}
      else
	{
	  /* qq = NN / 0d */

	  if (d0 == 0)
	    d0 = 1 / d0;	/* Divide intentionally by zero.  */

	  count_leading_zeros (bm, d0);

	  if (bm == 0)
	    {
	      /* From (n1 >= d0) /\ (the most significant bit of d0 is set),
	         conclude (the most significant bit of n1 is set) /\ (the
	         leading quotient digit q1 = 1).

	         This special case is necessary, not an optimization.
	         (Shifts counts of SI_TYPE_SIZE are undefined.)  */

	      n1 -= d0;
	      q1 = 1;
	    }
	  else
	    {
	      /* Normalize.  */

	      b = SI_TYPE_SIZE - bm;

	      d0 = d0 << bm;
	      n2 = n1 >> b;
	      n1 = (n1 << bm) | (n0 >> b);
	      n0 = n0 << bm;

	      udiv_qrnnd (q1, n1, n2, n1, d0);
	    }

	  /* n1 != d0...  */

	  udiv_qrnnd (q0, n0, n1, n0, d0);

	  /* Remainder in n0 >> bm.  */
	}

      if (rp != 0)
	{
	  rr.s.low = n0 >> bm;
	  rr.s.high = 0;
	  *rp = rr.ll;
	}
    }
#endif /* UDIV_NEEDS_NORMALIZATION */

  else
    {
      if (d1 > n1)
	{
	  /* 00 = nn / DD */

	  q0 = 0;
	  q1 = 0;

	  /* Remainder in n1n0.  */
	  if (rp != 0)
	    {
	      rr.s.low = n0;
	      rr.s.high = n1;
	      *rp = rr.ll;
	    }
	}
      else
	{
	  /* 0q = NN / dd */

	  count_leading_zeros (bm, d1);
	  if (bm == 0)
	    {
	      /* From (n1 >= d1) /\ (the most significant bit of d1 is set),
	         conclude (the most significant bit of n1 is set) /\ (the
	         quotient digit q0 = 0 or 1).

	         This special case is necessary, not an optimization.  */

	      /* The condition on the next line takes advantage of that
	         n1 >= d1 (true due to program flow).  */
	      if (n1 > d1 || n0 >= d0)
		{
		  q0 = 1;
		  sub_ddmmss (n1, n0, n1, n0, d1, d0);
		}
	      else
		q0 = 0;

	      q1 = 0;

	      if (rp != 0)
		{
		  rr.s.low = n0;
		  rr.s.high = n1;
		  *rp = rr.ll;
		}
	    }
	  else
	    {
	      USItype m1, m0;
	      /* Normalize.  */

	      b = SI_TYPE_SIZE - bm;

	      d1 = (d1 << bm) | (d0 >> b);
	      d0 = d0 << bm;
	      n2 = n1 >> b;
	      n1 = (n1 << bm) | (n0 >> b);
	      n0 = n0 << bm;

	      udiv_qrnnd (q0, n1, n2, n1, d1);
	      umul_ppmm (m1, m0, q0, d0);

	      if (m1 > n1 || (m1 == n1 && m0 > n0))
		{
		  q0--;
		  sub_ddmmss (m1, m0, m1, m0, d1, d0);
		}

	      q1 = 0;

	      /* Remainder in (n1n0 - m1m0) >> bm.  */
	      if (rp != 0)
		{
		  sub_ddmmss (n1, n0, n1, n0, m1, m0);
		  rr.s.low = (n1 << b) | (n0 >> bm);
		  rr.s.high = n1 >> bm;
		  *rp = rr.ll;
		}
	    }
	}
    }

  ww.s.low = q0;
  ww.s.high = q1;
  return ww.ll;
}

DItype
__divdi3 (DItype u, DItype v)
{
  word_type c = 0;
  DIunion uu, vv;
  DItype w;

  uu.ll = u;
  vv.ll = v;

  if (uu.s.high < 0)
    c = ~c, uu.ll = __negdi2 (uu.ll);
  if (vv.s.high < 0)
    c = ~c, vv.ll = __negdi2 (vv.ll);

  w = __udivmoddi4 (uu.ll, vv.ll, (UDItype *) 0);
  if (c)
    w = __negdi2 (w);

  return w;
}


DItype
__moddi3 (DItype u, DItype v)
{
  word_type c = 0;
  DIunion uu, vv;
  DItype w;

  uu.ll = u;
  vv.ll = v;

  if (uu.s.high < 0)
    c = ~c, uu.ll = __negdi2 (uu.ll);
  if (vv.s.high < 0)
    vv.ll = __negdi2 (vv.ll);

  (void) __udivmoddi4 (uu.ll, vv.ll, (UDItype *) & w);
  if (c)
    w = __negdi2 (w);

  return w;
}


UDItype
__umoddi3 (UDItype u, UDItype v)
{
  UDItype w;

  (void) __udivmoddi4 (u, v, (UDItype *) & w);

  return w;
}


UDItype
__udivdi3 (UDItype n, UDItype d)
{
  return __udivmoddi4 (n, d, (UDItype *) 0);
}
#endif

#ifdef __arm__
void
raise(void)
{
	oss_cmn_err (CE_PANIC, "raise() got called\n");
}

#endif

dev_info_t *
oss_create_pcidip (struct pci_dev * pcidev)
{
  dev_info_t *dip;

  if ((dip = oss_pmalloc (sizeof (*dip))) == NULL)
    return NULL;

  memset (dip, 0, sizeof (*dip));
  dip->pcidev = pcidev;

  return dip;
}

int
osscore_pci_read_config_byte (dev_info_t * dip, unsigned int where,
			      unsigned char *val)
{
  return pci_read_config_byte (dip->pcidev, where, val);
}

int
osscore_pci_read_config_irq (dev_info_t * dip, unsigned int where,
			     unsigned char *val)
{
  *val = dip->pcidev->irq;
  return 0;			// PCIBIOS_SUCCESSFUL
}

int
osscore_pci_read_config_word (dev_info_t * dip, unsigned int where,
			      unsigned short *val)
{
  if (dip == NULL)
    {
      oss_cmn_err (CE_CONT, "osscore_pci_read_config_word: dip==NULL\n");
      return -1;
    }
  return pci_read_config_word (dip->pcidev, where, val);
}

int
osscore_pci_read_config_dword (dev_info_t * dip, unsigned int where,
			       unsigned int *val)
{
  return pci_read_config_dword (dip->pcidev, where, val);
}

int
osscore_pci_write_config_byte (dev_info_t * dip, unsigned int where,
			       unsigned char val)
{
  return pci_write_config_byte (dip->pcidev, where, val);
}

int
osscore_pci_write_config_word (dev_info_t * dip, unsigned int where,
			       unsigned short val)
{
  return pci_write_config_word (dip->pcidev, where, val);
}

int
osscore_pci_write_config_dword (dev_info_t * dip, unsigned int where,
				unsigned int val)
{
  return pci_write_config_dword (dip->pcidev, where, val);
}

int
osscore_pci_enable_msi (dev_info_t * dip)
{
  pci_enable_msi (dip->pcidev);
  return (dip->pcidev->irq);
}

/* These definitions must match with oss_config.h */
typedef int (*oss_tophalf_handler_t) (struct _oss_device_t * osdev);
typedef void (*oss_bottomhalf_handler_t) (struct _oss_device_t * osdev);

typedef struct
{
  int irq;
  oss_device_t *osdev;
  oss_tophalf_handler_t top;
  oss_bottomhalf_handler_t bottom;
} osscore_intr_t;

#define MAX_INTRS	32

static osscore_intr_t intrs[MAX_INTRS] = { {0} };
static int nintrs = 0;

static irqreturn_t
osscore_intr (int irq, void *arg)
{
  int serviced;
  osscore_intr_t *intr = arg;

  if (intr->osdev == NULL || intr->top == NULL)	/* Removed interrupt */
    return 0;

  serviced = intr->top (intr->osdev);
  oss_inc_intrcount (intr->osdev, serviced);
  if (serviced)
    {
      if (intr->bottom != NULL)
	intr->bottom (intr->osdev);
      return IRQ_HANDLED;
    }
  return IRQ_NONE;
}

extern int oss_pci_read_config_irq (oss_device_t * osdev, unsigned long where,
				    unsigned char *val);

const char *
oss_pci_read_devpath (dev_info_t * dip)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30)
  return dev_name(&dip->pcidev->dev);
#else
  return dip->pcidev->dev.bus_id;
#endif
}

int
oss_register_interrupts (oss_device_t * osdev, int irqnum,
			 oss_tophalf_handler_t top,
			 oss_bottomhalf_handler_t bottom)
{

  unsigned char pci_irq_line;
  osscore_intr_t *intr;
  char *name;
  int err;

  if (nintrs >= MAX_INTRS)
    {
      oss_cmn_err (CE_CONT,
		   "oss_register_interrupts: Too many interrupt handlers\n");
      return -ENOMEM;
    }

  intr = &intrs[nintrs];

  if (oss_pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line) > 0)
    return -EIO;

  intr->irq = pci_irq_line;
  intr->osdev = osdev;
  intr->top = top;
  intr->bottom = bottom;

  name = osdev_get_nick (osdev);
#ifndef IRQF_SHARED
#define IRQF_SHARED SA_SHIRQ
#endif

  if ((err =
       request_irq (pci_irq_line, osscore_intr, IRQF_SHARED, name, intr)) < 0)
    {
      oss_cmn_err (CE_CONT, "request_irq(%d) failed, err=%d\n", pci_irq_line,
		   err);
      return err;
    }

  nintrs++;

  return 0;
}

void
oss_unregister_interrupts (oss_device_t * osdev)
{
  int i;

  for (i = 0; i < nintrs; i++)
    if (intrs[i].osdev == osdev)
      {
	free_irq (intrs[i].irq, &intrs[i]);
	intrs[i].osdev = NULL;
	intrs[i].top = NULL;
	intrs[i].bottom = NULL;
      }
}

int
oss_copy_to_user (void *to, const void *from, unsigned long n)
{
  return copy_to_user (to, from, n);
}

int
oss_copy_from_user (void *to, const void *from, unsigned long n)
{
  return copy_from_user (to, from, n);
}

static int refcount = 0;

typedef struct
{
  struct module *module;
  int active;
} oss_module_t;

#define OSS_MAX_MODULES 50

static oss_module_t oss_modules[OSS_MAX_MODULES];
static int num_oss_modules = 0;

void
oss_inc_refcounts (void)
{
  int i;
  refcount++;
  for (i = 0; i < num_oss_modules; i++)
    if (oss_modules[i].active)
      {
	if (!try_module_get (oss_modules[i].module))
	  oss_cmn_err (CE_WARN, "try_module_get() failed\n");
      }
}

void
oss_dec_refcounts (void)
{
  int i;
  refcount--;
  for (i = 0; i < num_oss_modules; i++)
    if (oss_modules[i].active)
      {
	module_put (oss_modules[i].module);
      }
}

void
oss_register_module (struct module *mod)
{
  int i, n = -1;

  for (i = 0; i < num_oss_modules; i++)
    if (!oss_modules[i].active)
      {
	n = i;
	break;
      }

  if (n == -1)
    {
      if (num_oss_modules >= OSS_MAX_MODULES)
	{
	  printk (KERN_ALERT "Too many OSS modules\n");
	  return;
	}

      n = num_oss_modules++;
    }

  oss_modules[n].module = mod;
  oss_modules[n].active = 1;
}

void
oss_unregister_module (struct module *mod)
{
  int i;

  for (i = 0; i < num_oss_modules; i++)
    if (oss_modules[i].active && oss_modules[i].module == mod)
      {
	oss_modules[i].active = 0;
	oss_modules[i].module = NULL;
	return;
      }
}

module_init (osscore_init);
module_exit (osscore_exit);

#ifdef CONFIG_OSS_VMIX_FLOAT

#define FP_SAVE(envbuf)		asm ("fnsave %0":"=m" (*envbuf));
#define FP_RESTORE(envbuf)		asm ("frstor %0":"=m" (*envbuf));

/* SSE/SSE2 compatible macros */
#define FX_SAVE(envbuf)		asm ("fxsave %0":"=m" (*envbuf));
#define FX_RESTORE(envbuf)		asm ("fxrstor %0":"=m" (*envbuf));

static int old_arch = 0;	/* No SSE/SSE2 instructions */

#if 0
static inline unsigned long
read_cr0 (void)
{
  unsigned long cr0;
  asm volatile ("movq %%cr0,%0":"=r" (cr0));
  return cr0;
}

static inline void
write_cr0 (unsigned long val)
{
  asm volatile ("movq %0,%%cr0"::"r" (val));
}

static inline unsigned long
read_cr4 (void)
{
  unsigned long cr4;
  asm volatile ("movq %%cr4,%0":"=r" (cr4));
  return cr4;
}

static inline void
write_cr4 (unsigned long val)
{
  asm volatile ("movq %0,%%cr4"::"r" (val));
}
#endif

static inline unsigned long long
read_mxcsr (void)
{
  unsigned long long mxcsr;
  asm volatile ("stmxcsr %0":"=m" (mxcsr));
  return mxcsr;
}

static inline void
write_mxcsr (unsigned long long val)
{
  asm volatile ("ldmxcsr %0"::"m" (val));
}

int
oss_fp_check (void)
{
  int eax, ebx, ecx, edx;
#define FLAGS_ID (1<<21)

  oss_native_word flags_reg;

  local_save_flags (flags_reg);
  flags_reg &= ~FLAGS_ID;
  local_irq_restore (flags_reg);

  local_save_flags (flags_reg);
  if (flags_reg & FLAGS_ID)
    return 0;

  flags_reg |= FLAGS_ID;
  local_irq_restore (flags_reg);

  local_save_flags (flags_reg);
  if (!(flags_reg & FLAGS_ID))
    return 0;

#define CPUID_FXSR	(1<<24)
#define CPUID_SSE	(1<<25)
#define CPUID_SSE2	(1<<26)

  cpuid (1, &eax, &ebx, &ecx, &edx);

  if (!(edx & CPUID_FXSR))
    return -1;

  /*
   * Older machines require different FP handling than the latest ones. Use the SSE
   * instruction set as an indicator.
   */
  if (!(edx & CPUID_SSE))
    old_arch = 1;

  return 1;
}

void
oss_fp_save (short *envbuf, unsigned int flags[])
{
  flags[0] = read_cr0 ();
  write_cr0 (flags[0] & ~0x0e);	/* Clear CR0.TS/MP/EM */

  if (old_arch)
    {
      FP_SAVE (envbuf);
    }
  else
    {
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
      flags[1] = read_cr4 ();
      write_cr4 (flags[1] | 0x600);	/* Set OSFXSR & OSXMMEXCEPT */
#else
      flags[1] = __read_cr4 ();
      __write_cr4 (flags[1] | 0x600);	/* Set OSFXSR & OSXMMEXCEPT */
#endif
      FX_SAVE (envbuf);
      asm ("fninit");
      asm ("fwait");
      write_mxcsr (0x1f80);
    }
  flags[2] = read_cr0 ();
}

void
oss_fp_restore (short *envbuf, unsigned int flags[])
{
  asm ("fwait");
  if (old_arch)
    {
      FP_RESTORE (envbuf);
    }
  else
    {
      FX_RESTORE (envbuf);
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,0,0)
      write_cr4 (flags[1]);	/* Restore cr4 */
#else
      __write_cr4 (flags[1]);	/* Restore cr4 */
#endif
    }
  write_cr0 (flags[0]);		/* Restore cr0 */
}
#endif

#if 0
void
__stack_chk_fail (void)
{
  panic ("__stack_chk_fail\n");
}
#endif

/*
 * Exported symbols
 */

#define EXPORT_FUNC(name) extern void name(void);EXPORT_SYMBOL(name);
#define EXPORT_DATA(name) extern int name;EXPORT_SYMBOL(name);

/* EXPORT_SYMBOL (__stack_chk_fail); */

#ifdef CONFIG_OSS_VMIX_FLOAT
EXPORT_SYMBOL (oss_fp_check);
EXPORT_SYMBOL (oss_fp_save);
EXPORT_SYMBOL (oss_fp_restore);
#endif

EXPORT_SYMBOL (oss_register_chrdev);
EXPORT_SYMBOL (oss_unregister_chrdev);
EXPORT_SYMBOL (oss_reserve_pages);
EXPORT_SYMBOL (oss_unreserve_pages);
EXPORT_FUNC (ac97_install);
EXPORT_FUNC (ac97_install_full);
EXPORT_FUNC (ac97_playrate);
EXPORT_FUNC (ac97_recrate);
EXPORT_FUNC (ac97_varrate);
EXPORT_FUNC (ac97_override_control);
EXPORT_FUNC (ac97_init_ext);
EXPORT_FUNC (ac97_mixer_set);
EXPORT_FUNC (ac97_spdif_setup);
EXPORT_FUNC (ac97_spdifout_ctl);
EXPORT_FUNC (ac97_remove_control);
EXPORT_SYMBOL (ac97_amplifier);
EXPORT_FUNC (ac97_disable_spdif);
EXPORT_FUNC (ac97_enable_spdif);
EXPORT_FUNC (ac97_mixext_ctl);
EXPORT_FUNC (ac97_spdifin_ctl);
EXPORT_FUNC (oss_pci_byteswap);
EXPORT_SYMBOL (mixer_ext_truncate);
EXPORT_SYMBOL (mixer_ext_rebuild_all);
EXPORT_FUNC (remux_install);
EXPORT_SYMBOL (oss_strlen);
EXPORT_SYMBOL (oss_strcmp);
EXPORT_FUNC (oss_install_mididev);
EXPORT_DATA (detect_trace);
EXPORT_SYMBOL (dmap_get_qhead);
EXPORT_SYMBOL (dmap_get_qtail);
EXPORT_FUNC (oss_alloc_dmabuf);
EXPORT_SYMBOL (oss_contig_free);
EXPORT_SYMBOL (oss_contig_malloc);
EXPORT_FUNC (oss_disable_device);
EXPORT_FUNC (oss_free_dmabuf);
EXPORT_SYMBOL (oss_map_pci_mem);
EXPORT_SYMBOL (oss_install_audiodev);
EXPORT_SYMBOL (oss_install_audiodev_with_devname);
EXPORT_FUNC (oss_audio_set_error);
EXPORT_SYMBOL (load_mixer_volumes);
EXPORT_SYMBOL (oss_unmap_pci_mem);
EXPORT_SYMBOL (oss_memset);
EXPORT_SYMBOL (oss_mutex_cleanup);
EXPORT_SYMBOL (oss_mutex_init);
EXPORT_SYMBOL (oss_register_device);
EXPORT_SYMBOL (oss_register_interrupts);
EXPORT_SYMBOL (oss_inc_intrcount);
EXPORT_SYMBOL (oss_spin_lock);
EXPORT_SYMBOL (oss_spin_lock_irqsave);
EXPORT_SYMBOL (oss_spin_unlock);
EXPORT_SYMBOL (oss_spin_unlock_irqrestore);
EXPORT_SYMBOL (oss_udelay);
EXPORT_FUNC (oss_unregister_device);
EXPORT_SYMBOL (oss_unregister_interrupts);
EXPORT_SYMBOL (oss_virt_to_bus);
EXPORT_FUNC (oss_pci_read_config_byte);
EXPORT_FUNC (oss_pci_read_config_word);
EXPORT_FUNC (oss_pci_read_config_dword);
EXPORT_FUNC (oss_pci_write_config_byte);
EXPORT_FUNC (oss_pci_write_config_word);
EXPORT_FUNC (oss_pci_write_config_dword);
EXPORT_FUNC (oss_pci_enable_msi);
EXPORT_SYMBOL (oss_pci_read_config_irq);
EXPORT_SYMBOL (oss_pci_read_devpath);
EXPORT_SYMBOL (oss_get_jiffies);
EXPORT_SYMBOL (mixer_find_ext);
EXPORT_SYMBOL (oss_install_mixer);
EXPORT_SYMBOL (oss_strcpy);
EXPORT_SYMBOL (oss_kmem_free);
#ifndef __arm__
EXPORT_FUNC (uart401_init);
EXPORT_FUNC (uart401_disable);
EXPORT_FUNC (uart401_irq);
#endif
EXPORT_SYMBOL (mixer_ext_set_init_fn);
EXPORT_SYMBOL (mixer_ext_set_vmix_init_fn);
#ifdef CONFIG_OSS_VMIX
EXPORT_FUNC (vmix_attach_audiodev);
EXPORT_FUNC (vmix_detach_audiodev);
EXPORT_FUNC (vmix_change_devnames);
#endif
EXPORT_SYMBOL (mixer_ext_set_strings);
EXPORT_SYMBOL (mixer_ext_create_group);
EXPORT_SYMBOL (mixer_ext_create_group_flags);
EXPORT_SYMBOL (mixer_ext_create_control);
EXPORT_SYMBOL (oss_strncpy);
EXPORT_SYMBOL (oss_memcpy);
EXPORT_SYMBOL (oss_kmem_alloc);
EXPORT_DATA (oss_hz);
EXPORT_FUNC (oss_spdif_open);
EXPORT_FUNC (oss_spdif_ioctl);
EXPORT_FUNC (oss_spdif_install);
EXPORT_FUNC (oss_spdif_uninstall);
EXPORT_FUNC (oss_spdif_close);
EXPORT_FUNC (oss_spdif_mix_init);
EXPORT_FUNC (oss_spdif_setrate);
EXPORT_FUNC (create_new_card);
EXPORT_FUNC (oss_audio_ioctl);
EXPORT_FUNC (oss_audio_open_engine);
EXPORT_FUNC (oss_audio_release);
EXPORT_FUNC (oss_audio_set_rate);
EXPORT_SYMBOL (oss_uiomove);
EXPORT_SYMBOL (oss_get_pid);
EXPORT_SYMBOL (oss_get_uid);
EXPORT_SYMBOL (oss_get_procname);
EXPORT_SYMBOL (mix_cvt);
EXPORT_FUNC (oss_audio_set_format);
EXPORT_FUNC (oss_audio_set_channels);
EXPORT_FUNC (midiparser_create);
EXPORT_FUNC (midiparser_input);
EXPORT_FUNC (midiparser_unalloc);
EXPORT_FUNC (mixer_ext_create_device);
EXPORT_SYMBOL (mixer_ext_recrw);
EXPORT_SYMBOL (mixer_ext_rw);
EXPORT_SYMBOL (mixer_ext_set_enum);
EXPORT_SYMBOL (mixer_ext_set_description);
EXPORT_SYMBOL (osdev_create);
EXPORT_FUNC (osdev_clone);
EXPORT_SYMBOL (osdev_delete);
EXPORT_FUNC (oss_audio_chpoll);
EXPORT_FUNC (oss_audio_delayed_attach);
EXPORT_FUNC (oss_audio_read);
EXPORT_FUNC (oss_audio_write);
EXPORT_SYMBOL (oss_create_wait_queue);
EXPORT_SYMBOL (oss_remove_wait_queue);
EXPORT_SYMBOL (oss_reset_wait_queue);
EXPORT_SYMBOL (oss_sleep);
EXPORT_SYMBOL (oss_strncmp);
EXPORT_SYMBOL (oss_timeout);
EXPORT_SYMBOL (oss_untimeout);
EXPORT_SYMBOL (oss_wakeup);
#if 0
EXPORT_FUNC (ossddk_ac97_install);
EXPORT_FUNC (ossddk_ac97_is_varrate);
EXPORT_FUNC (ossddk_ac97_remove);
EXPORT_FUNC (ossddk_ac97_set_ext_init);
EXPORT_FUNC (ossddk_ac97_set_playrate);
EXPORT_FUNC (ossddk_ac97_set_recrate);
EXPORT_FUNC (ossddk_adev_get_devc);
EXPORT_FUNC (ossddk_adev_get_dmapin);
EXPORT_FUNC (ossddk_adev_get_dmapout);
EXPORT_FUNC (ossddk_adev_get_flags);
EXPORT_FUNC (ossddk_adev_get_label);
EXPORT_FUNC (ossddk_adev_get_portc);
EXPORT_FUNC (ossddk_adev_get_portc_play);
EXPORT_FUNC (ossddk_adev_get_portc_record);
EXPORT_FUNC (ossddk_adev_get_songname);
EXPORT_FUNC (ossddk_adev_set_buflimits);
EXPORT_FUNC (ossddk_adev_set_caps);
EXPORT_FUNC (ossddk_adev_set_channels);
EXPORT_FUNC (ossddk_adev_set_devc);
EXPORT_FUNC (ossddk_adev_set_enable_flag);
EXPORT_FUNC (ossddk_adev_set_flags);
EXPORT_FUNC (ossddk_adev_set_formats);
EXPORT_FUNC (ossddk_adev_set_label);
EXPORT_FUNC (ossddk_adev_set_magic);
EXPORT_FUNC (ossddk_adev_set_mixer);
EXPORT_FUNC (ossddk_adev_set_portc);
EXPORT_FUNC (ossddk_adev_set_portc_play);
EXPORT_FUNC (ossddk_adev_set_portc_record);
EXPORT_FUNC (ossddk_adev_set_portnum);
EXPORT_FUNC (ossddk_adev_set_rates);
EXPORT_FUNC (ossddk_adev_set_ratesource);
EXPORT_FUNC (ossddk_adev_set_songname);
EXPORT_FUNC (ossddk_adev_set_unloaded_flag);
EXPORT_FUNC (ossddk_audio_inputintr);
EXPORT_FUNC (ossddk_audio_outputintr);
EXPORT_FUNC (ossddk_disable_device);
EXPORT_FUNC (ossddk_dmap_get_buffsize);
EXPORT_FUNC (ossddk_dmap_get_buffused);
EXPORT_FUNC (ossddk_dmap_get_dmabuf);
EXPORT_FUNC (ossddk_dmap_get_fragsize);
EXPORT_FUNC (ossddk_dmap_get_numfrags);
EXPORT_FUNC (ossddk_dmap_get_phys);
EXPORT_FUNC (ossddk_dmap_get_private);
EXPORT_FUNC (ossddk_dmap_get_qhead);
EXPORT_FUNC (ossddk_dmap_get_qtail);
EXPORT_FUNC (ossddk_dmap_set_buffsize);
EXPORT_FUNC (ossddk_dmap_set_callback);
EXPORT_FUNC (ossddk_dmap_set_dmabuf);
EXPORT_FUNC (ossddk_dmap_set_fragsize);
EXPORT_FUNC (ossddk_dmap_set_numfrags);
EXPORT_FUNC (ossddk_dmap_set_phys);
EXPORT_FUNC (ossddk_dmap_set_playerror);
EXPORT_FUNC (ossddk_dmap_set_private);
EXPORT_FUNC (ossddk_dmap_set_recerror);
EXPORT_FUNC (ossddk_install_audiodev);
EXPORT_FUNC (ossddk_install_mixer);
EXPORT_FUNC (ossddk_mixer_create_control);
EXPORT_FUNC (ossddk_mixer_create_group);
EXPORT_FUNC (ossddk_mixer_get_devc);
EXPORT_FUNC (ossddk_mixer_set_strings);
EXPORT_FUNC (ossddk_mixer_touch);
EXPORT_FUNC (ossddk_mixer_truncate);
EXPORT_FUNC (ossddk_osdev_get_devc);
EXPORT_FUNC (ossddk_register_device);
EXPORT_FUNC (ossddk_unregister_device);
#endif
EXPORT_SYMBOL (osdev_set_major);
EXPORT_SYMBOL (osdev_set_owner);
EXPORT_SYMBOL (osdev_get_owner);
EXPORT_SYMBOL (oss_create_pcidip);
EXPORT_SYMBOL (touch_mixer);
EXPORT_SYMBOL (oss_mixer_ext);
EXPORT_SYMBOL (oss_request_major);
EXPORT_SYMBOL (audio_engines);
EXPORT_DATA (midi_devs);
EXPORT_SYMBOL (mixer_devs);
EXPORT_SYMBOL (mixer_devs_p);
EXPORT_DATA (num_audio_engines);
EXPORT_DATA (num_mididevs);
EXPORT_SYMBOL (num_mixers);
EXPORT_DATA (oss_timing_mutex);
EXPORT_DATA (oss_num_cards);
EXPORT_FUNC (oss_do_timing);
EXPORT_FUNC (oss_do_timing2);
EXPORT_FUNC (oss_timing_enter);
EXPORT_FUNC (oss_timing_leave);
#ifndef __arm__
EXPORT_SYMBOL (__udivdi3);
EXPORT_SYMBOL (__umoddi3);
EXPORT_SYMBOL (__divdi3);
#else
EXPORT_SYMBOL (raise);
#endif
EXPORT_SYMBOL (oss_copy_from_user);
EXPORT_SYMBOL (oss_copy_to_user);
EXPORT_SYMBOL (osdev_set_irqparms);
EXPORT_SYMBOL (osdev_get_irqparms);
EXPORT_SYMBOL (osdev_get_nick);
EXPORT_SYMBOL (osdev_get_instance);
EXPORT_SYMBOL (oss_inc_refcounts);
EXPORT_SYMBOL (oss_dec_refcounts);
EXPORT_SYMBOL (oss_register_module);
EXPORT_SYMBOL (oss_unregister_module);
EXPORT_SYMBOL (oss_audio_reset);
EXPORT_SYMBOL (oss_audio_start_syncgroup);
EXPORT_SYMBOL (oss_encode_enum);
EXPORT_SYMBOL (dmap_get_qlen);
EXPORT_SYMBOL (num_audio_devfiles);
EXPORT_SYMBOL (oss_audio_inc_byte_counter);
EXPORT_SYMBOL (oss_audio_register_client);
EXPORT_SYMBOL (audio_devfiles);
EXPORT_FUNC (oss_get_cardinfo);
EXPORT_SYMBOL (oss_pmalloc);
EXPORT_SYMBOL (oss_add_audio_devlist);
EXPORT_FUNC (oss_memblk_malloc);
EXPORT_FUNC (oss_memblk_free);
EXPORT_FUNC (oss_memblk_unalloc);
EXPORT_DATA (oss_global_memblk);
EXPORT_FUNC (oss_get_procinfo);
EXPORT_DATA (mixer_muted);

#ifdef CONFIG_OSS_MIDI
EXPORT_FUNC (oss_midi_ioctl);
EXPORT_FUNC (oss_midi_copy_timer);
#endif
