/*
 * Purpose: Operating system abstraction functions for SCO OpenServer/UnixWare
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

#include "oss_config.h"
#include "midi_core.h"
#include <oss_pci.h>
#include <sys/exec.h>
#include <sys/user.h>
#include <errno.h>

/*
 * MAX_CARDS must be larger than life. The system will panic if there are
 * more sound devices (cards os sound chips) than MAX_CARDS.
 */
#define MAX_CARDS 32

static oss_device_t *cards[MAX_CARDS];
int oss_num_cards = 0;
static int oss_expired = 0;

volatile int oss_open_devices = 0;

static bcb_t *oss_bcb;

/*
 * Driver information structures, for drv_attach().
 */
static int oss_config (cfg_func_t func, void *idata, rm_key_t key);

static const drvops_t oss_ops = {
  oss_config,
  oss_open,
  oss_close,
  oss_devinfo,
  oss_biostart,
  oss_ioctl,
  NULL,				/* drvctl */
  NULL				/* mmap */
};

static const drvinfo_t oss_drvinfo = {
  &oss_ops,
  "osscore",
  D_MP,				/* MP-safe */
  NULL,				/* Not a STREAMS driver */
  10				/* Must match $maxchan in Node file */
};

static physreq_t *physreq_isa = NULL;	/* 24 bits */
static physreq_t *physreq_28bit = NULL;
static physreq_t *physreq_30bit = NULL;
static physreq_t *physreq_31bit = NULL;
static physreq_t *physreq_32bit = NULL;

#ifdef MEMDEBUG
typedef struct
{
  void *addr;
  int size;
  char file[40];
  int line;
} mem_block_t;

#define MAX_MEMBLOCKS 1024

static mem_block_t memblocks[MAX_MEMBLOCKS];
static int num_memblocks = 0;
#endif

#ifdef MEMDEBUG
void *
oss_kmem_alloc (size_t size, int flags, char *file, int line)
#else
void *
oss_kmem_alloc (size_t size, int flags)
#endif
{
/*
 * This routine allocates a memory block and stores length of it in
 * the beginning. This length information can be used when later unallocating
 * the memory.
 */

  char *ptr;
  uint64_t *len;

  ptr = kmem_zalloc (size + sizeof (uint64_t), flags);
#ifdef MEMDEBUG
  cmn_err (CE_CONT, "kmalloc(%d, %s:%d)=%x\n", size, file, line, ptr);
#endif

  if (ptr == NULL)
    return NULL;

  len = (uint64_t *) ptr;

  ptr += sizeof (uint64_t);
  *len = size + sizeof (uint64_t);

#ifdef MEMDEBUG
#if 1
  {
    int i;
    for (i = 0; i < num_memblocks; i++)
      if (memblocks[i].addr == NULL)
	{
	  memblocks[i].addr = ptr;
	  memblocks[i].size = size;
	  strncpy (memblocks[i].file, file, 39);
	  memblocks[i].line = line;
	  return ptr;
	}
  }
#endif

  if (num_memblocks < MAX_MEMBLOCKS)
    {
      memblocks[num_memblocks].addr = ptr;
      memblocks[num_memblocks].size = size;
      strncpy (memblocks[num_memblocks].file, file, 39);
      memblocks[num_memblocks].line = line;
      num_memblocks++;
    }
#endif

  return ptr;
}

void
#ifdef MEMDEBUG
oss_kmem_free (void *addr, char *file, int line)
#else
oss_kmem_free (void *addr)
#endif
{
  uint64_t *len;
  int i;

  char *ptr = addr;

  if (addr == NULL)
    return;

  ptr -= sizeof (uint64_t);

  len = (uint64_t *) ptr;

  kmem_free (ptr, *len);
#ifdef MEMDEBUG
  {
    int i;

    for (i = 0; i < num_memblocks; i++)
      if (addr == memblocks[i].addr)
	{
	  memblocks[i].addr = NULL;
	  return;
	}
  }
  cmn_err (CE_WARN, "Bad kfree(%x, %s:%d)\n", addr, file, line);
#endif
}

/*
 * Table for permanently allocated memory (to be freed by _fini)
 */
#define OSS_MAX_MEM 	1024
void *oss_mem_blocks[OSS_MAX_MEM];
int oss_nblocks = 0;

void *
#ifdef MEMDEBUG
oss_pmalloc (size_t size, char *file, int line)
#else
oss_pmalloc (size_t size)
#endif
{
  void *mem_ptr;
#ifdef MEMDEBUG
  mem_ptr = (oss_mem_blocks[oss_nblocks] =
	     oss_kmem_alloc (size, KM_SLEEP, file, line));
#else
  mem_ptr = (oss_mem_blocks[oss_nblocks] = KERNEL_MALLOC (size));
#endif

  if (oss_nblocks <= OSS_MAX_MEM)
    oss_nblocks++;
  else
    cmn_err (CE_NOTE, "Out of mem blocks\n");
  return mem_ptr;
}

int
__oss_alloc_dmabuf (int dev, dmap_p dmap, unsigned int alloc_flags,
		    oss_uint64_t maxaddr, int direction)
{
  void *p;
  oss_native_word phaddr;
  int size = 64 * 1024;
  extern int dma_buffsize;

  if (dma_buffsize > 16 && dma_buffsize <= 128)
    size = dma_buffsize * 1024;

  if (dmap->dmabuf != NULL)
    return 0;

/*
 * Some applications and virtual drivers need shorter buffer.
 */
  if (dmap->flags & DMAP_SMALLBUF)
    {
      size = SMALL_DMABUF_SIZE;
    }
  else if (dmap->flags & DMAP_MEDIUMBUF)
    {
      size = MEDIUM_DMABUF_SIZE;
    }

  if ((alloc_flags & DMABUF_SIZE_16BITS) && size > 32 * 1024)
    size = 32 * 1024;

  if ((p =
       oss_contig_malloc (audio_engines[dev]->osdev, size, maxaddr,
			  &phaddr)) == NULL)
    return OSS_ENOMEM;

  dmap->dmabuf = p;
  dmap->dmabuf_phys = phaddr;
  dmap->buffsize = size;

  return 0;
}

void
oss_free_dmabuf (int dev, dmap_p dmap)
{
  if (dmap->dmabuf == NULL)
    return;

  oss_contig_free (audio_engines[dev]->osdev, dmap->dmabuf, dmap->buffsize);
  dmap->dmabuf = NULL;
  dmap->dmabuf_phys = 0;
  dmap->buffsize = 0;
}

void *
oss_contig_malloc (oss_device_t * osdev, int size, oss_uint64_t memlimit,
		   oss_native_word * phaddr)
{
  void *p = NULL;
  physreq_t *preqp = physreq_32bit;
  paddr32_t pa;

  *phaddr = 0;

  switch (memlimit)
    {
    case MEMLIMIT_ISA:
      preqp = physreq_isa;
      break;
    case MEMLIMIT_28BITS:
      preqp = physreq_28bit;
      break;
    case MEMLIMIT_30BITS:
      preqp = physreq_30bit;
      break;
    case MEMLIMIT_31BITS:
      preqp = physreq_31bit;
      break;
    case MEMLIMIT_32BITS:
      preqp = physreq_32bit;
      break;

    default:
      cmn_err (CE_WARN, "osscore: Bad DMA memlimit for %s\n", osdev->nick);
    }

  if ((p = kmem_alloc_phys (size, preqp, &pa, 0)) == NULL)
    {
      cmn_err (CE_WARN, "osscore: kmem_alloc_phys() failed\n");
      return NULL;
    }

  *phaddr = pa;
  return p;
}

void
oss_contig_free (oss_device_t * osdev, void *p, int sz)
{
  if (p)
    kmem_free (p, sz);
}

/*
 * Wait queue support
 */
oss_wait_queue_t *
oss_create_wait_queue (oss_device_t * osdev, const char *name)
{
  oss_wait_queue_t *q;

  if ((q = KERNEL_MALLOC (sizeof (*q))) == NULL)
    {
      cmn_err (CE_WARN, "osscore: Cannot allocate memory for a wait queue\n");
      return NULL;
    }

  memset (q, 0, sizeof (*q));

  if ((q->sv = SV_ALLOC (KM_SLEEP)) == NULL)
    {
      cmn_err (CE_WARN,
	       "osscore: Cannot allocate synchronization variable\n");
      return NULL;
    }

  return q;
}

void
oss_reset_wait_queue (oss_wait_queue_t * wq)
{
  // NOP
}

void
oss_remove_wait_queue (oss_wait_queue_t * wq)
{
  SV_DEALLOC (wq->sv);
  KERNEL_FREE (wq);
}

static void
sleep_timeout (caddr_t arg)
{
  oss_wait_queue_t *wq = (oss_wait_queue_t *) arg;

  SV_BROADCAST (wq->sv, 0);
}

int
oss_sleep (oss_wait_queue_t * wq, oss_mutex_t * mutex, int ticks,
	   oss_native_word * flags, unsigned int *status)
{
  timeout_id_t tid;

  *status = 0;
  wq->flags = 0;

  if (wq == NULL)
    {
      cmn_err (CE_WARN, "osscore: Unallocated wait queue\n");
      *status |= WK_SIGNAL;
      return 1;
    }
  if (ticks > 0)
    tid = timeout (sleep_timeout, wq, ticks);

  /*
   * Note SV_WAIT_SIG() will release the mutex/lock so we must re-acquire
   * it before returning.
   */
#ifdef MUTEX_CHECKS
  // Pop mutex because it will be released by SV_WAIT_SIG() 
  pop_mutex (*mutex, __FILE__, __LINE__);
#endif
  if (!SV_WAIT_SIG (wq->sv, prihi, *mutex))	/* Signal */
    {
      *status |= WK_SIGNAL;
      wq->flags |= WK_WAKEUP;	/* Needed to prevent false timeout messages */
    }

  MUTEX_ENTER_IRQDISABLE (*mutex, *flags);
  if (ticks > 0 && (wq->flags & WK_WAKEUP))
    untimeout (tid);

  return (wq->flags & WK_WAKEUP);
}

int
oss_register_poll (oss_wait_queue_t * wq, oss_mutex_t * mutex,
		   oss_native_word * flags, oss_poll_event_t * ev)
{
  // NOP: DDI8 doesn't support chpoll
}

void
oss_wakeup (oss_wait_queue_t * wq, oss_mutex_t * mutex,
	    oss_native_word * flags, short events)
{
  wq->flags |= WK_WAKEUP;
  SV_BROADCAST (wq->sv, 0);
}

void *
oss_get_osid (oss_device_t * osdev)
{
  return osdev->osid;
}

static int
grow_array(oss_device_t *osdev, oss_cdev_t ***arr, int *size, int element_size, int increment)
{
	oss_cdev_t **old=*arr, **new = *arr;
	int old_size = *size;
	int new_size = *size;
		
	new_size += increment;

	if ((new=PMALLOC(osdev, new_size * element_size))==NULL)
	   return 0;

	memset(new, 0, new_size * element_size);
	if (old != NULL)
	   memcpy(new, old, old_size * element_size);

	*size = new_size;
	*arr = new;

	if (old != NULL)
	   PMFREE(osdev, old);

	return 1;
}

void
oss_install_chrdev (oss_device_t * osdev, char *name, int dev_class,
		    int instance, oss_cdev_drv_t * drv, int flags)
{
/*
 * oss_install_chrdev creates a character device (minor). However if
 * name==NULL the device will not be exported (made visible to userland
 * clients).
 */

  int i, num;
  oss_cdev_t *cdev = NULL;

  if (dev_class != OSS_DEV_STATUS)
    if (oss_expired && instance > 0)
      return;
/*
 * Find if this dev_class&instance already exists (after previous module
 * detach).
 */

  for (num = 0; num < oss_num_cdevs; num++)
    if (oss_cdevs[num]->d == NULL)	/* Unloaded driver */
      if (oss_cdevs[num]->dev_class == dev_class
	  && oss_cdevs[num]->instance == instance)
	{
	  cdev = oss_cdevs[num];
	  break;
	}

  if (cdev == NULL)
    {
      if (oss_num_cdevs >= OSS_MAX_CDEVS)
	{
	   if (!grow_array(osdev, &oss_cdevs, &oss_max_cdevs, sizeof(oss_cdev_t*), 100))
	   {
	  	cmn_err (CE_WARN, "Cannot allocate new minor numbers.\n");
	  	return;
	   }
	}

      if ((cdev = PMALLOC (NULL, sizeof (*cdev))) == NULL)
	{
	  cmn_err (CE_WARN, "Cannot allocate character device desc.\n");
	  return;
	}
      num = oss_num_cdevs++;
    }

  memset (cdev, 0, sizeof (*cdev));
  cdev->dev_class = dev_class;
  cdev->instance = instance;
  cdev->d = drv;
  cdev->osdev = osdev;
  if (name != NULL)
    strncpy (cdev->name, name, sizeof (cdev->name) - 1);
  else
    strcpy (cdev->name, "NONE");
  cdev->name[sizeof (cdev->name) - 1] = 0;
  oss_cdevs[num] = cdev;
}

int
oss_find_minor (int dev_class, int instance)
{
  int i;

  for (i = 0; i < oss_num_cdevs; i++)
    if (oss_cdevs[i]->d != NULL && oss_cdevs[i]->dev_class == dev_class
	&& oss_cdevs[i]->instance == instance)
      return i;

  return OSS_EIO;
}

int
oss_get_cardinfo (int cardnum, oss_card_info * ci)
{
/*
 * Print information about a 'card' in a format suitable for /dev/sndstat
 */

  if (cardnum < 0 || cardnum >= oss_num_cards)
    return OSS_ENXIO;

  if (cards[cardnum]->name != NULL)
    strncpy (ci->longname, cards[cardnum]->name, 128);
  ci->shortname[127] = 0;

  if (cards[cardnum]->nick != NULL)
    strncpy (ci->shortname, cards[cardnum]->nick, 16);
  ci->shortname[15] = 0;

  if (cards[cardnum]->hw_info != NULL)
    strncpy (ci->hw_info, cards[cardnum]->hw_info, sizeof (ci->hw_info) - 1);
  ci->hw_info[sizeof (ci->hw_info) - 1] = 0;

  return 0;
}

void
oss_reserve_device (oss_device_t * osdev)
{
  osdev->refcount++;
}

void
oss_unreserve_device (oss_device_t * osdev, int decrement)
{
  osdev->refcount--;
  if (osdev->refcount < 0)
    osdev->refcount = 0;
}

/*
 *	Some standard C library routines
 */
void *
memset (void *t, int c, size_t l)
{
  int i;

  if (t == NULL)
    return NULL;

  for (i = 0; i < l; i++)
    ((char *) t)[i] = c;

  return t;
}

void *
memcpy (void *t, const void *s, size_t l)
{
  bcopy (s, t, l);
  return t;
}

oss_device_t *
osdev_create (dev_info_t * dip, int dev_type, int instance, const char *nick,
	      const char *handle)
{
  oss_device_t *osdev = NULL;
  int i, err;
  caddr_t addr;
  dev_info_t ldip = 0;

  if (dip == NULL)
    dip = &ldip;

  if (handle == NULL)
    handle = nick;

  /*
   * Don't accept any more drivers if expired
   */
  if (oss_expired && oss_num_cards > 0)
    return NULL;

/*
 * Check if a driver/device was reinserted
 */
  if (dip != &ldip)		/* Not a virtual driver */
    for (i = 0; i < oss_num_cards; i++)
      {
	if (!cards[i]->available && cards[i]->key == *dip)
	  {
	    osdev = cards[i];
	    break;
	  }
      }

  if (osdev == NULL)
    {
      if (oss_num_cards >= MAX_CARDS)
	{
	  cmn_err (CE_WARN, "Too many OSS devices. At most %d permitted.\n",
		   MAX_CARDS);
	  return NULL;
	}

      if ((osdev = PMALLOC (NULL, sizeof (*osdev))) == NULL)
	{
	  cmn_err (CE_WARN, "osdev_create: Out of memory\n");
	  return NULL;
	}

      osdev->cardnum = oss_num_cards;
      cards[oss_num_cards++] = osdev;
    }
  osdev->key = *dip;
  osdev->osid = dip;
  osdev->available = 1;
  osdev->first_mixer = -1;
  osdev->instance = instance;
  osdev->dev_type = dev_type;
  osdev->devc = NULL;
  MUTEX_INIT (osdev, osdev->mutex, MH_GLOBAL);
  sprintf (osdev->nick, "%s%d", nick, instance);
  strcpy (osdev->modname, nick);

  switch (dev_type)
    {
    case DRV_PCI:
      //pci_config_setup (dip, &osdev->pci_config_handle);
      break;

    case DRV_VIRTUAL:
    case DRV_VMIX:
    case DRV_STREAMS:
      /* NOP */
      break;

    case DRV_USB:
      /* NOP */
      break;

    default:
      cmn_err (CE_WARN, "Bad device type\n");
      return NULL;
    }

/*
 * Create the device handle
 */
  switch (dev_type)
    {
    case DRV_PCI:
      {
	unsigned int subvendor = 0;
	pci_read_config_dword (osdev, 0x2c, &subvendor);

	sprintf (osdev->handle, "PCI%08x-%d", subvendor, instance);
      }
      break;

#if 0
    case DRV_USB:
      sprintf (osdev->handle, "USB-%s%d", handle, instance);
      break;
#endif

    default:
      sprintf (osdev->handle, "%s%d", handle, instance);
    }

  return osdev;
}

oss_device_t *
osdev_clone (oss_device_t * orig_osdev, int new_instance)
{
  oss_device_t *osdev;

  osdev = PMALLOC (NULL, sizeof (*osdev));
  if (osdev == NULL)
    {
      cmn_err (CE_WARN, "osdev_create: Out of memory\n");
      return NULL;
    }
  memcpy (osdev, orig_osdev, sizeof (*osdev));
  osdev->dev_type = DRV_CLONE;
  osdev->instance = new_instance;
  sprintf (osdev->nick, "%s%d", orig_osdev->modname, new_instance);
  sprintf (osdev->handle, "%s%d", orig_osdev->modname, new_instance);

  return osdev;
}

void
osdev_delete (oss_device_t * osdev)
{
  int i;

  if (osdev == NULL)
    return;

  if (!osdev->available)
    {
      cmn_err (CE_WARN, "device %s, osdev already deleted\n", osdev->nick);
      return;
    }

  osdev->available = 0;

  switch (osdev->dev_type)
    {
    case DRV_PCI:
      break;
    }

/*
 * Mark all minor nodes for this module as invalid.
 */
  for (i = 0; i < oss_num_cdevs; i++)
    if (oss_cdevs[i]->osdev == osdev)
      {
	oss_cdevs[i]->d = NULL;
	oss_cdevs[i]->osdev = NULL;
	strcpy (oss_cdevs[i]->name, "Removed device");
      }
  MUTEX_CLEANUP (osdev->mutex);
}

int
oss_register_device (oss_device_t * osdev, const char *name)
{
  if ((osdev->name = PMALLOC (NULL, strlen (name) + 1)) == NULL)
    {
      cmn_err (CE_WARN, "Cannot allocate memory for device name\n");
      osdev->name = "Unknown device";
    }
  strcpy (osdev->name, name);
  return 0;
}

int
oss_disable_device (oss_device_t * osdev)
{
  int i;
/*
 * This routine should check if the device is ready to be unloaded (no devices are in use).
 * If the device cannot be unloaded this routine must return OSS_EBUSY.
 *
 * If the device can be unloaded then disable any timers or other features that may cause the
 * device to be called. Also mark the audio/midi/mixer/etc devices of this device to be disabled.
 * However the interrupt handler should still stay enabled. The low level driver will call
 * oss_unregister_interrupts() after it has cleared the interrupt enable register.
 */
  if (osdev->refcount > 0)
    {
      return OSS_EBUSY;
    }

/*
 * Now mark all devices unavailable (for the time being)
 */

  for (i = 0; i < num_mixers; i++)
    if (mixer_devs[i]->osdev == osdev)
      {
	mixer_devs[i]->unloaded = 1;
      }

  for (i = 0; i < num_mididevs; i++)
    {
      if (midi_devs[i]->osdev == osdev)
	{
	  midi_devs[i]->unloaded = 1;
	}
    }

  for (i = 0; i < num_audio_engines; i++)
    if (audio_engines[i]->osdev == osdev)
      {
	audio_uninit_device (i);
      }

  return 0;
}

void
oss_unregister_device (oss_device_t * osdev)
{
}

#ifdef MUTEX_CHECKS
static int oss_context = 0;	/* 0=user context, 1=interrupt context */
#endif

static int
ossintr (void *idata)
{
  oss_device_t *osdev = idata;
  oss_native_word flags;
#ifdef MUTEX_CHECKS
  int saved_context;
  saved_context = oss_context;
  if (oss_context == 1)
    cmn_err (CE_WARN, "Recursive interrupt\n");
  oss_context = 1;
#endif

  MUTEX_ENTER_IRQDISABLE (osdev->mutex, flags);

  if (!osdev->tophalf_handler (osdev))
    {
      MUTEX_EXIT_IRQRESTORE (osdev->mutex, flags);
#ifdef MUTEX_CHECKS
      oss_context = saved_context;
#endif
      return ISTAT_NONE;
    }

  if (osdev->bottomhalf_handler != NULL)
    osdev->bottomhalf_handler (osdev);

  MUTEX_EXIT_IRQRESTORE (osdev->mutex, flags);
#ifdef MUTEX_CHECKS
  oss_context = saved_context;
#endif

  return ISTAT_ASSERTED;
}

int
oss_register_interrupts (oss_device_t * osdev, int intrnum,
			 oss_tophalf_handler_t top,
			 oss_bottomhalf_handler_t bottom)
{
  int err;

  if (intrnum != 0)
    {
      cmn_err (CE_WARN, "Bad interrupt index (%d) for %s\n", intrnum,
	       osdev->name);
      return OSS_EINVAL;
    }

  if (osdev == NULL)
    {
      cmn_err (CE_WARN, "oss_register_interrupts: Bad osdev\n");
      return OSS_EINVAL;
    }

  if (osdev->tophalf_handler != NULL || osdev->bottomhalf_handler != NULL)
    {
      cmn_err (CE_WARN, "Interrupts already registered for %s\n",
	       osdev->name);
      return OSS_EINVAL;
    }

  if (top == NULL)
    {
      cmn_err (CE_WARN, "Bad interrupt handler for %s\n", osdev->name);
      return OSS_EINVAL;
    }

  osdev->tophalf_handler = top;
  osdev->bottomhalf_handler = bottom;
  if (cm_intr_attach
      (osdev->key, ossintr, osdev, osdev->drvinfo, &osdev->intr_cookie) == 0)
    {
      cmn_err (CE_WARN, "cm_intr_attach failed for %s\n", osdev->nick);
    }

  return 0;
}

void
oss_unregister_interrupts (oss_device_t * osdev)
{
  if (osdev->intr_cookie != NULL)
    cm_intr_detach (osdev->intr_cookie);
  osdev->intr_cookie = NULL;
}

static char *
ksprintn (ul, base, lenp)
     register u_long ul;
     register int base, *lenp;
{				/* A long in base 8, plus NULL. */
  static char buf[sizeof (long) * NBBY / 3 + 2];
  register char *p;

  p = buf;
  do
    {
      *++p = "0123456789abcdef"[ul % base];
    }
  while (ul /= base);
  if (lenp)
    *lenp = p - buf;
  return (p);
}

int
oss_sprintf (char *buf, char *cfmt, ...)
{
  const char *fmt = cfmt;
  register char *p, *bp;
  register int ch, base;
  unsigned long ul;
  int lflag;
  int count = 10;
  va_list ap;

  va_start (ap, fmt);
  for (bp = buf;;)
    {
      while ((ch = *(unsigned char *) fmt++) != '%')
	if ((*bp++ = ch) == '\0')
	  {
	    va_end (ap);
	    return ((bp - buf) - 1);
	  }

      lflag = 0;
    reswitch:
      switch (ch = *(unsigned char *) fmt++)
	{
	case 'l':
	  lflag = 1;
	  goto reswitch;

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	  goto reswitch;

	case 'c':
	  *bp++ = va_arg (ap, int);
	  break;

	case 's':
	  p = va_arg (ap, char *);
	  while (*bp++ = *p++)
	    ;
	  --bp;
	  break;

	case 'd':
	  ul = lflag ? va_arg (ap, long) : va_arg (ap, int);
	  if ((long) ul < 0)
	    {
	      *bp++ = '-';
	      ul = -(long) ul;
	    }
	  base = 10;
	  goto number;
	  break;

	case 'o':
	  ul = lflag ? va_arg (ap, unsigned long) : va_arg (ap, unsigned int);
	  base = 8;
	  goto number;
	  break;

	case 'u':
	  ul = lflag ? va_arg (ap, unsigned long) : va_arg (ap, unsigned int);
	  base = 10;
	  goto number;
	  break;

	case 'x':
	  ul = lflag ? va_arg (ap, unsigned long) : va_arg (ap, unsigned int);
	  base = 16;
	number:
	  for (p = (char *) ksprintn (ul, base, NULL); ch = *p--;)
	    *bp++ = ch;
	  break;

	default:
	  *bp++ = '%';
	  if (lflag)
	    *bp++ = 'l';
	  /* FALLTHROUGH */

	case '%':
	  *bp++ = ch;
	}
    }

  /* va_end(ap); */
}

static physreq_t *
build_physreq (int bits)
{
  physreq_t *pr;
  long long tmp;

  if ((pr = physreq_alloc (KM_SLEEP)) == NULL)
    return NULL;

  pr->phys_align = 4096;
  pr->phys_boundary = 0;
  pr->phys_dmasize = bits;
  pr->phys_max_scgth = 0;
  pr->phys_flags = PREQ_PHYSCONTIG;

  if (physreq_prep (pr, KM_SLEEP) != B_TRUE)
    {
      cmn_err (CE_WARN, "osscore: physreq_prep failed\n");
    }

  return pr;
}

/*
 * Driver info device file
 */
static int drvinfo_busy = 0;
static int drvinfo_len = 0, drvinfo_ptr = 0;
static char *drvinfo_buf = NULL;
#define DRVINFO_SIZE 4096

static int
drvinfo_open (int dev, int dev_class, struct fileinfo *file,
	      int recursive, int open_flags, int *redirect)
{
  int i;

  if (drvinfo_busy)
    return OSS_EBUSY;
  drvinfo_busy = 1;

  if ((drvinfo_buf = KERNEL_MALLOC (DRVINFO_SIZE)) == NULL)
    {
      cmn_err (CE_WARN, "Cannot allocate drvinfo buffer\n");
      return OSS_ENOMEM;
    }

  drvinfo_len = 0;
  drvinfo_ptr = 0;

  for (i = 1; i < oss_num_cdevs; i++)
    if (oss_cdevs[i]->name[0] != 'N' || oss_cdevs[i]->name[1] != 'O' || oss_cdevs[i]->name[2] != 'N' || oss_cdevs[i]->name[3] != 'E')	/* Not a dummy placeholder device */
      if (DRVINFO_SIZE - drvinfo_len > 32)
	{
	  char *s = drvinfo_buf + drvinfo_len;

	  drvinfo_len +=
	    oss_sprintf (s, "%s %s %d\n", oss_cdevs[i]->name,
			 oss_cdevs[i]->osdev->nick, i);

	}

  return 0;
}

static void
drvinfo_close (int dev, struct fileinfo *file)
{
  KERNEL_FREE (drvinfo_buf);
  drvinfo_buf = NULL;
  drvinfo_len = 0;
  drvinfo_ptr = 0;
  drvinfo_busy = 0;
}

static int
drvinfo_read (int dev, struct fileinfo *file, uio_t * buf, int count)
{
  /*
   * Return at most 'count' bytes from the drvinfo_buf.
   */
  int l, c;

  l = count;
  c = drvinfo_len - drvinfo_ptr;

  if (l > c)
    l = c;
  if (l <= 0)
    return 0;

  if (uiomove (&drvinfo_buf[drvinfo_ptr], l, UIO_READ, buf) != 0)
    return OSS_EFAULT;
  drvinfo_ptr += l;

  return l;
}

static oss_cdev_drv_t drvinfo_cdev_drv = {
  drvinfo_open,
  drvinfo_close,
  drvinfo_read
};

static void
install_drvinfo (oss_device_t * osdev)
{
  oss_install_chrdev (osdev, "ossinfo", OSS_DEV_MISC, 0, &drvinfo_cdev_drv,
		      0);
}

/*
 * Driver entry point routines
 */

int
_load ()
{
  int err = 0;
  oss_device_t *osdev;
  time_t t;

  if ((err = drv_attach (&oss_drvinfo)) != 0)
    {
      cmn_err (CE_WARN, "drv_attach failed %d\n", err);
      return err;
    }

#ifdef LICENSED_VERSION
  if (drv_getparm (TIME, &t) != 0)
    {
      cmn_err (CE_WARN, "drv_getparm(TIME) failed\n");
      return EBUSY;
    }

  if (!oss_license_handle_time (t))
    {
      cmn_err (CE_WARN, "This version of Open Sound System has expired\n");
      cmn_err (CE_CONT,
	       "Please download the latest version from www.opensound.com\n");
      oss_expired = 1;
    }
#endif

  if ((osdev = osdev_create (NULL, DRV_VIRTUAL, 0, "oss", NULL)) == NULL)
    {
      cmn_err (CE_WARN, "Creating osdev failed\n");
      return ENOMEM;
    }

/*
 * Allocate physrec structures for various memory ranges. Remember to free them in the _load
 * entry point.
 */
  physreq_isa = build_physreq (24);
  physreq_28bit = build_physreq (28);
  physreq_30bit = build_physreq (30);
  physreq_31bit = build_physreq (31);
  physreq_32bit = build_physreq (32);

/*
 * Create the BCB structure
 */
  oss_bcb = bcb_alloc (KM_SLEEP);
  oss_bcb->bcb_addrtypes = BA_UIO;
  oss_bcb->bcb_flags = 0;
  oss_bcb->bcb_max_xfer = 0;
  oss_bcb->bcb_granularity = 1;
  oss_bcb->bcb_physreqp = physreq_32bit;
  bcb_prep (oss_bcb, KM_SLEEP);

  install_drvinfo (osdev);
  oss_common_init (osdev);

  oss_register_device (osdev, "OSS core services");

  return 0;
}

int
_unload ()
{
  int i;
  static int already_unloaded = 0;

  if (oss_open_devices > 0)
    return EBUSY;
  drv_detach (&oss_drvinfo);

  if (already_unloaded)
    return 0;
  already_unloaded = 1;

  oss_unload_drivers ();

  physreq_free (physreq_isa);
  physreq_free (physreq_28bit);
  physreq_free (physreq_30bit);
  physreq_free (physreq_31bit);
  physreq_free (physreq_32bit);

  bcb_free (oss_bcb);

  for (i = 0; i < oss_nblocks; i++)
    KERNEL_FREE (oss_mem_blocks[i]);
  oss_nblocks = 0;

  return 0;
}

void
oss_pci_byteswap (oss_device_t * osdev, int mode)
{
  // NOP
}

void
oss_pcie_init (oss_device_t * osdev, int flags)
{
	/* TODO: Should we do something? */
}

int
pci_read_config_byte (oss_device_t * osdev, offset_t where,
		      unsigned char *val)
{
  *val = 0;
  if (cm_read_devconfig (osdev->key, where, val, sizeof (*val)) ==
      sizeof (*val))
    return PCIBIOS_SUCCESSFUL;

  return PCIBIOS_FAILED;
}

int
pci_read_config_irq (oss_device_t * osdev, offset_t where, unsigned char *val)
{
  *val = 0;
  if (cm_read_devconfig (osdev->key, where, val, sizeof (*val)) ==
      sizeof (*val))
    return PCIBIOS_SUCCESSFUL;

  return PCIBIOS_FAILED;
}


int
pci_read_config_word (oss_device_t * osdev, offset_t where,
		      unsigned short *val)
{
  *val = 0;
#if 1
  if (cm_read_devconfig (osdev->key, where, val, sizeof (*val)) ==
      sizeof (*val))
    return PCIBIOS_SUCCESSFUL;
#else
  switch (where)
    {
    case PCI_VENDOR_ID:
      *val = osdev->vendor;
      return PCIBIOS_SUCCESSFUL;
      break;

    case PCI_DEVICE_ID:
      *val = osdev->product;
      return PCIBIOS_SUCCESSFUL;
      break;

    }

#endif
  return PCIBIOS_FAILED;
}

int
pci_read_config_dword (oss_device_t * osdev, offset_t where,
		       unsigned int *val)
{
  *val = 0;
  if (cm_read_devconfig (osdev->key, where, val, sizeof (*val)) ==
      sizeof (*val))
    return PCIBIOS_SUCCESSFUL;

  return PCIBIOS_FAILED;
}

int
pci_write_config_byte (oss_device_t * osdev, offset_t where,
		       unsigned char val)
{
  if (cm_write_devconfig (osdev->key, where, &val, sizeof (val)) ==
      sizeof (val))
    return PCIBIOS_SUCCESSFUL;

  return PCIBIOS_FAILED;
}

int
pci_write_config_word (oss_device_t * osdev, offset_t where,
		       unsigned short val)
{
  if (cm_write_devconfig (osdev->key, where, &val, sizeof (val)) ==
      sizeof (val))
    return PCIBIOS_SUCCESSFUL;

  return PCIBIOS_FAILED;
}

int
pci_write_config_dword (oss_device_t * osdev, offset_t where,
			unsigned int val)
{
  if (cm_write_devconfig (osdev->key, where, &val, sizeof (val)) ==
      sizeof (val))
    return PCIBIOS_SUCCESSFUL;

  return PCIBIOS_FAILED;
}

/*
 * Entry point routines
 */
static int
oss_config (cfg_func_t func, void *idata, rm_key_t key)
{
  return EOPNOTSUPP;
}

int
oss_devinfo (void *idata, channel_t channel, di_parm_t parm, void *valp)
{
  switch (parm)
    {
    case DI_MEDIA:
      break;

    case DI_SIZE:
      break;

    case DI_RBCBP:
      *(bcb_t **) valp = oss_bcb;
      return 0;
      break;

    case DI_WBCBP:
      *(bcb_t **) valp = oss_bcb;
      return 0;
      break;

    case DI_PHYS_HINT:
      break;

    }
  return EOPNOTSUPP;
}

void
oss_biostart (void *idata, channel_t channel, buf_t * bp)
{
  int dev = channel;
  oss_cdev_t *cdev;
  int count = bp->b_un.b_uio->uio_resid;
  int retval;

  if ((cdev = oss_cdevs[dev]) == NULL)
    {
      bioerror (bp, ENXIO);
      biodone (bp);
      return;
    }

  cdev->file.acc_flags = 0;

  if (bp->b_flags & B_READ)
    {
      /* Read operation */
      if (cdev->d->read == NULL)
	{
	  bioerror (bp, ENXIO);
	  biodone (bp);
	  return;
	}
      retval =
	cdev->d->read (cdev->instance, &cdev->file, bp->b_un.b_uio, count);
      if (retval < 0)
	bioerror (bp, -retval);
      else if (retval < count)
	bp->b_resid = count - retval;

      biodone (bp);
      return;
    }

  /* Write operation */
  if (cdev->d->write == NULL)
    {
      bioerror (bp, ENXIO);
      biodone (bp);
      return;
    }
  retval =
    cdev->d->write (cdev->instance, &cdev->file, bp->b_un.b_uio, count);
  if (retval < 0)
    bioerror (bp, -retval);
  else if (retval < count)
    bp->b_resid = count - retval;

  biodone (bp);
}

int
oss_open (void *idata, channel_t * channelp,
	  int open_flags, cred_t * crp, queue_t * q)
{
  oss_device_t *osdev;
  int dev = *channelp, tmpdev;
  oss_cdev_t *cdev;
  int retval;

  osdev = idata;

  if (dev >= OSS_MAX_CDEVS)
    return ENXIO;

  if (dev >= oss_num_cdevs)
    {
      return ENODEV;
    }

  if ((cdev = oss_cdevs[dev]) == NULL || cdev->d == NULL)
    return ENODEV;

  if (cdev->d->open == NULL)
    {
      return ENODEV;
    }

  memset (&cdev->file, 0, sizeof (cdev->file));
  cdev->file.mode = 0;
  cdev->file.acc_flags = open_flags;

  if (open_flags & FREAD && open_flags & FWRITE)
    cdev->file.mode = OPEN_READWRITE;
  else if (open_flags & FREAD)
    cdev->file.mode = OPEN_READ;
  else if (open_flags & FWRITE)
    cdev->file.mode = OPEN_WRITE;

  tmpdev = dev;
  retval =
    cdev->d->open (cdev->instance, cdev->dev_class, &cdev->file, 0, 0, &tmpdev);
  *channelp = tmpdev;
  dev = tmpdev;

  if (retval < 0)
    {
      return -retval;
    }

  oss_open_devices++;
  cdev->open_count++;

  return 0;
}

int
oss_close (void *idata, channel_t channel,
	   int oflags, cred_t * crp, queue_t * q)
{
  oss_cdev_t *cdev;
  int dev;

  dev = channel;

  if (dev >= OSS_MAX_CDEVS)
    return ENXIO;

  if ((cdev = oss_cdevs[dev]) == NULL)
    return ENXIO;

  if (cdev->open_count == 0)    /* Not opened */
    return 0;

  cdev->d->close (cdev->instance, &cdev->file);

  oss_open_devices--;
  cdev->open_count--;
  return 0;
}

int
oss_ioctl (void *idata, channel_t channel, int cmd, void *arg,
	   int oflags, cred_t * crp, int *rvalp)
{
  int dev = channel;
  int retval;
  int len = 0;
  char localbuf[256];		/* All frequently used ioctl calls fit in 256 bytes */
  char *buf = localbuf, *auxbuf = NULL;
  oss_cdev_t *cdev;

  if ((cdev = oss_cdevs[dev]) == NULL || cdev->d->ioctl == NULL)
    return *rvalp = ENXIO;

  if (oss_expired)
    return *rvalp = ENODEV;

  if (cmd & (SIOC_OUT | SIOC_IN))
    {

      len = __SIOC_SIZE (cmd);
      if (len < 0)
	len = 0;
      if (len > sizeof (localbuf))
	{
	  /*
	   * For the few largest ioctl calls we need to allocate an off-stack
	   * buffer because otherwise the kernel stack will overflow.
	   * This approach is slower but fortunately "sane" applications don't
	   * use these ioctl calls too frequently.
	   */
	  if ((auxbuf = KERNEL_MALLOC (len)) == NULL)
	    {
	      cmn_err (CE_WARN,
		       "Failed to allocate an ioctl buffer of %d bytes\n",
		       len);
	      return *rvalp = ENOMEM;
	    }
	  buf = auxbuf;
	}

      if ((cmd & SIOC_IN) && len > 0)
	{
	  if (copyin ((char *) arg, buf, len) == -1)
	    {
	      if (auxbuf != NULL)
		KERNEL_FREE (auxbuf);
	      return *rvalp = EFAULT;
	    }
	}

    }

  retval = cdev->d->ioctl (cdev->instance, &cdev->file, cmd, (ioctl_arg) buf);

  if ((cmd & SIOC_OUT) && len > 0)
    {
      if (copyout (buf, (char *) arg, len) == -1)
	{
	  if (auxbuf != NULL)
	    KERNEL_FREE (auxbuf);
	  return *rvalp = EFAULT;
	}
    }

  *rvalp = (((retval) < 0) ? -(retval) : 0);

  if (auxbuf != NULL)
    KERNEL_FREE (auxbuf);

  return *rvalp;
}

#ifdef MUTEX_CHECKS

typedef struct
{
  int active;
  void *mutex;
  const char *filename;
  int line;
  int context;
} mutex_debug_t;

static mutex_debug_t mutex_tab[1024];
static int n_mutexes = 0;

void
push_mutex (void *mutex, const char *file, int line)
{
  int i, n = -1;
//cmn_err(CE_CONT, "Push mutex %08x, %s:%d, context=%d\n", mutex, file, line, oss_context);

  for (i = 0; n == -1 && i < n_mutexes; i++)
    if (!mutex_tab[i].active)
      n = i;
    else
      {
	if (mutex_tab[i].mutex == mutex
	    && mutex_tab[i].context == oss_context)
	  {
	    cmn_err (CE_NOTE, "Mutex %08x already held\n", mutex);
	    cmn_err (CE_CONT, "  Locked by %s:%d\n", mutex_tab[i].filename,
		     mutex_tab[i].line);
	    cmn_err (CE_CONT, "  Acquire by %s:%d\n", file, line);
	  }
      }

  if (n == -1)
    {
      if (n_mutexes >= 1024)
	{
	  cmn_err (CE_NOTE, "Mutex debug table full\n");
	  return;
	}
      n = n_mutexes++;
    }

  mutex_tab[n].active = 1;
  mutex_tab[n].mutex = mutex;
  mutex_tab[n].filename = file;
  mutex_tab[n].line = line;
  mutex_tab[n].context = oss_context;
}

void
pop_mutex (void *mutex, const char *file, int line)
{
  int i;

//cmn_err(CE_CONT, "Pop mutex %08x, %s:%d, context=%d\n", mutex, file, line, oss_context);
  for (i = 0; i < n_mutexes; i++)
    if (mutex_tab[i].active && mutex_tab[i].mutex == mutex
	&& mutex_tab[i].context == oss_context)
      {
	mutex_tab[i].active = 0;
	mutex_tab[i].filename = file;
	mutex_tab[i].line = line;
	return;
      }

  cmn_err (CE_NOTE, "Mutex %08x not locked (%s:%d), context=%d\n",
	   mutex, file, line, oss_context);
  for (i = 0; i < n_mutexes; i++)
    if (mutex_tab[i].active == 0 && mutex_tab[i].mutex == mutex)
      {
	cmn_err (CE_CONT, "  Previous unlock at %s:%d, context=%d\n",
		 mutex_tab[i].filename, mutex_tab[i].line,
		 mutex_tab[i].context);
      }
}

void
print_mutexes (void)
{
  int i, n = 0;

  for (i = 0; i < n_mutexes; i++)
    if (mutex_tab[i].active)
      n++;

  cmn_err (CE_CONT, "%d mutexes held\n", n);

  for (i = 0; i < n_mutexes; i++)
    if (mutex_tab[i].active)
      cmn_err (CE_CONT, "   %08x %s:%d\n", mutex_tab[i].mutex,
	       mutex_tab[i].filename, mutex_tab[i].line);
}
#endif

int
oss_get_procinfo(int what)
{
	// TODO

	return OSS_EINVAL;
}
