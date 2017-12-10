/*
 * Purpose: Operating system abstraction functions for BeOS/Haiku
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

#include "oss_config.h"
#include "midi_core.h"
#include <oss_pci.h>
#include <Drivers.h>
#include <KernelExport.h>
#include <driver_settings.h>

/*
 * The BeOS and Haiku kernels are preemptible, 
 * therefore we must ensure safe access to global variables.
 * Such variables are marked by GM.
 * Other protected variables:
 * - oss_cdevs
 * - mixer_devs
 * XXX: use a benaphore ??
 */
#if 0
// spinlock
static oss_mutex_t osscore_mutex;
#define CORE_LOCK_VAR \
  oss_native_word osscore_mutex_flags
#define CORE_LOCK_INIT() \
  MUTEX_INIT (osdev, osscore_mutex, MH_TOP)
#define CORE_LOCK_CLEANUP() \
  MUTEX_CLEANUP (osscore_mutex)
#define LOCK_CORE() \
  MUTEX_ENTER_IRQDISABLE (osscore_mutex, osscore_mutex_flags)
#define UNLOCK_CORE() \
  MUTEX_EXIT_IRQRESTORE (osscore_mutex, osscore_mutex_flags)
#else
// benaphore
typedef struct benaphore {
        sem_id  sem;
        int32   count;
} benaphore;
benaphore osscore_benaphore;
#define CORE_LOCK_VAR \
  int dummy_ocl
#define CORE_LOCK_INIT() \
  osscore_benaphore.count = 1; \
  osscore_benaphore.sem = create_sem(0, "OSS CORE LOCK")
#define CORE_LOCK_CLEANUP() \
  delete_sem(osscore_benaphore.sem)
#define LOCK_CORE() \
  { \
    if (atomic_add(&osscore_benaphore.count, -1) <= 0) \
      acquire_sem(osscore_benaphore.sem); \
  }
#define UNLOCK_CORE() \
  { \
    if (atomic_add(&osscore_benaphore.count, 1) < 0) \
      release_sem(osscore_benaphore.sem); \
  }
#endif

#define DEBUG_IRQ 1
#if DEBUG_IRQ
vint32 irq_count = 0;
#endif

volatile int oss_open_devices = 0;
#define MAX_CARDS	16
int oss_num_cards = 0; /* GM */
static oss_device_t *cards[MAX_CARDS]; /* GM */
static int oss_expired = 0;
extern int vmix_disabled;

//static struct fileinfo files[OSS_MAX_CDEVS];
//static volatile int open_count[OSS_MAX_CDEVS] = { 0 };
//static volatile int open_devices = 0; /* GM */

pci_module_info *gPCI = NULL;

static char **gDeviceNames = NULL; // buffer for all names

device_hooks oss_driver_hooks;

/*
 * Table for permanently allocated memory (to be freed by std_op(UNLOAD))
 */
#define MAX_MEMBLOCKS	4096
static void *memblocks[MAX_MEMBLOCKS]; /* GM */
static int nmemblocks = 0; /* GM */

void *
oss_contig_malloc (oss_device_t * osdev, int size, oss_uint64_t memlimit,
		   oss_native_word * phaddr)
{
  status_t err;
  area_id id;
  void *p = NULL;
  uint32 lock = B_CONTIGUOUS;
  physical_entry pent[1];

  *phaddr = 0;

  switch (memlimit)
    {
    case MEMLIMIT_ISA:
    case MEMLIMIT_28BITS:
    case MEMLIMIT_30BITS:
    case MEMLIMIT_31BITS:
      /* no known way to force a physical address limit other than <16M */
      lock = B_LOMEM;
      break;
    case MEMLIMIT_32BITS:
      lock = B_CONTIGUOUS;
      break;

    default:
      cmn_err (CE_WARN, "Bad DMA memlimit for %s\n", osdev->nick);
    }

  /* round up to page size */
  size += B_PAGE_SIZE - 1;
  size &= ~(B_PAGE_SIZE - 1);

  if ((err = id = create_area(OSS_CONTIG_AREA_NAME, &p, B_ANY_KERNEL_ADDRESS,
                         size, lock, 0)) < B_OK)
    {
      cmn_err (CE_WARN, "create_area() failed\n");
      return NULL;
    }

  if ((err = get_memory_map(p, size, pent, 1)) < B_OK)
    {
      cmn_err (CE_WARN, "get_memory_map() failed\n");
      delete_area(id);
      return NULL;
    }
  //XXX:DEBUG
  *phaddr = (oss_native_word)pent[0].address;
  dprintf("oss_contig_malloc: area %d @ va %p, pa %p, sz %d\n", id, p, (void *)(*phaddr), size);
  return p;
}

void
oss_contig_free (oss_device_t * osdev, void *p, int sz)
{
  area_id id;
  if (p == NULL)
    return;
  id = area_for(p);
  if (id < B_OK)
    return;
#ifdef MEMDEBUG
  {
    area_info ai;
    if ((get_area_info(id, &ai) < B_OK) || strncmp(ai.name, OSS_CONTIG_AREA_NAME))
      {
        cmn_err (CE_NOTE, "oss_contig_free: bad area (%ld)!\n", id);
        return;
      }
  }
#endif
  delete_area(id);
}

int
__oss_alloc_dmabuf (int dev, dmap_p dmap, unsigned int alloc_flags,
		    oss_uint64_t maxaddr, int direction)
{
  void *tmpbuf;
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

  tmpbuf = CONTIG_MALLOC (dmap->osdev, size, maxaddr, &phaddr, NULL);
  if (tmpbuf == NULL)
    return OSS_ENOMEM;
  dmap->dmabuf = tmpbuf;
  dmap->buffsize = size;
  dmap->dmabuf_phys = phaddr;

  return 0;
}

void
oss_free_dmabuf (int dev, dmap_p dmap)
{
  if (dmap->dmabuf == NULL)
    return;

  CONTIG_FREE (dmap->osdev, dmap->dmabuf, dmap->buffsize, NULL);
  dmap->dmabuf = NULL;
  dmap->buffsize = 0;
  dmap->dmabuf_phys = 0;
}


oss_native_word
oss_virt_to_bus (void *addr)
{
  physical_entry pent[2];
  status_t err;

  if (addr == NULL)
    return 0;

  /* XXX: ROUNDUP(B_PAGE_SIZE) ? */
  if ((err = get_memory_map(addr, 1, pent, 2)) < 1)
    {
      cmn_err (CE_WARN, "Virtual address %x not mapped\n", (int) addr);
      return 0;
    }
  //XXX:which???
  //return (oss_native_word)pent[0].address;
  return (oss_native_word)(gPCI->ram_address(pent[0].address));
}


void *
oss_pmalloc (size_t sz)
{
  void *tmp;

  tmp = KERNEL_MALLOC (sz);

  if (nmemblocks < MAX_MEMBLOCKS)
    memblocks[nmemblocks++] = tmp;

  return tmp;
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
      return OSS_EIO;
    }

  uio->ptr = buf;
  uio->resid = count;
  uio->kernel_space = is_kernel;
  uio->rw = rw;

  return 0;
}

int
oss_uiomove (void *address, size_t nbytes, enum uio_rw rwflag, uio_t * uio)
{
  int err = EFAULT;
  FENTRY();

  if (rwflag != uio->rw)
    {
      oss_cmn_err (CE_WARN, "uiomove: Bad direction\n");
      goto err;
    }

  if (uio->resid < nbytes)
    {
      oss_cmn_err (CE_WARN, "uiomove: Bad count %d (%d)\n", nbytes,
		   uio->resid);
      goto err;
    }

  if (uio->kernel_space)
    goto err;

  switch (rwflag)
    {
    case UIO_READ:
      //XXX:user_memcpy...
      memcpy (uio->ptr, address, nbytes);
      break;

    case UIO_WRITE:
      //XXX:user_memcpy...
      memcpy (address, uio->ptr, nbytes);
      break;
    }

  uio->resid -= nbytes;
  uio->ptr += nbytes;

  err = B_OK;
err:
  FEXITR(err);
  return err;
}


void
oss_cmn_err (int level, char *s, ...)
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
      dprintf ("%s", tmp);
    }
  else
    {
      strcpy (tmp, "osscore: ");
      sprintf (tmp + strlen (tmp), s, a[0], a[1], a[2], a[3], a[4], a[5],
	       NULL, NULL, NULL, NULL);
      if (level == CE_PANIC)
	panic (tmp);

      dprintf ("%s", tmp);
    }
  va_end (ap);
}


/*
 * Sleep/wakeup
 */

struct oss_wait_queue *
oss_create_wait_queue (oss_device_t * osdev, const char *name)
{
  struct oss_wait_queue *wq;
  status_t err;
  FENTRYA(", %s", name);

  if ((wq = malloc (sizeof (*wq))) == NULL)
    {
      oss_cmn_err (CE_WARN, "malloc(%d) failed (wq)\n", sizeof (*wq));
      return NULL;
    }
  sprintf(wq->name, OSS_WQ_SEM_NAME "%-20s", name);
  err = wq->sem = create_sem(0, wq->name);
  if (err < B_OK)
    {
      free(wq);
      oss_cmn_err (CE_WARN, "create_sem() failed (wq)\n");
      return NULL;
    }

  return wq;
}

void
oss_reset_wait_queue (struct oss_wait_queue *wq)
{
  sem_info si;
  status_t err;
  FENTRY();
  
  wq->flags = 0;
  err = create_sem(0, wq->name);
  if (err >= 0) {
    /* replace with the new one */
    delete_sem(wq->sem);
    wq->sem = err;
  }
  
  FEXIT();
}

void
oss_remove_wait_queue (struct oss_wait_queue *wq)
{
  FENTRY();
  delete_sem(wq->sem);
  free (wq);
}

int
oss_sleep (struct oss_wait_queue *wq, oss_mutex_t * mutex, int ticks,
	   oss_native_word * flags, unsigned int *status)
{
  bigtime_t timeout = B_INFINITE_TIMEOUT;
  uint32 semflags = B_CAN_INTERRUPT | B_RELATIVE_TIMEOUT;
  int result = 0;
  FENTRYA("(%s), , %d, , ", wq->name, ticks);
  *status = 0;

  if (wq == NULL)
    return 0;

#ifdef B_WAKE_ON_TIMEOUT
  // Dano only; sure it's what we want ?
  if (wq->flags & WK_WAKEUP)
    semflags |= B_WAKE_ON_TIMEOUT;
#endif

  wq->flags = 0;
  MUTEX_EXIT_IRQRESTORE(*mutex, *flags);

  if (ticks > 0)
    timeout = ticks * 1000000LL / OSS_HZ;
  result = acquire_sem_etc (wq->sem, 1, semflags, timeout);
  //dprintf("oss_sleep:acquire_sem(s:%ld, 1, %x, %Ld): 0x%08lx\n", wq->sem, semflags, timeout, result);

  MUTEX_ENTER_IRQDISABLE (*mutex, *flags);

  if (result == EINTR)		/* Signal received */
    {
      *status |= WK_SIGNAL;
      return 1;
    }

  if (result == B_TIMED_OUT)
  //if (!(wq->flags & WK_WAKEUP))	/* Timeout */
    {
      return 0;
    }

  return 1;
}

int
oss_register_poll (struct oss_wait_queue *wq, oss_mutex_t * mutex,
		   oss_native_word * flags, oss_poll_event_t * ev)
{
  FENTRYA("(%s), , , , ", wq->name);
  dprintf("oss:UNIMPLEMENTED:%s\n", __FUNCTION__);
  MUTEX_EXIT_IRQRESTORE(*mutex, *flags);
  //poll_wait ((struct file *) ev->file, &wq->wq, (struct wait *) ev->wait);
  MUTEX_ENTER_IRQDISABLE (*mutex, *flags);
  return 0;
}

void
oss_wakeup (struct oss_wait_queue *wq, oss_mutex_t * mutex,
	    oss_native_word * flags, short events)
{
  FENTRYA("(%s), , %x, , ", wq->name, events);
  if (wq == NULL)
    return;

  wq->flags |= WK_WAKEUP;
  MUTEX_EXIT_IRQRESTORE(*mutex, *flags);
  
  //dprintf("oss_wakeup:release_sem(s:%ld)\n", wq->sem);
  release_sem_etc (wq->sem, 1, B_DO_NOT_RESCHEDULE);
  //XXX:handle select here
  
  MUTEX_ENTER_IRQDISABLE (*mutex, *flags);
}

unsigned long
oss_get_time (void)
{
  return (unsigned long) (system_time() / (1000000 / OSS_HZ));
}

typedef struct tmout_desc
{
  struct timer timer; /* MUST be first */

  volatile int active;
  int timestamp;
  void (*func) (void *);
  void *arg;
} tmout_desc_t;

static volatile int next_id = 0;
#define MAX_TMOUTS 128

tmout_desc_t tmouts[MAX_TMOUTS] = { {0} };

int timeout_random = 0x12123400;

int32
oss_timer_callback (struct timer *timer)
{
  tmout_desc_t *tmout = (tmout_desc_t *)timer;
  int ix;
  void *arg;

  timeout_random++;

  if (!tmout->active)
    return;

  arg = tmout->arg;
  tmout->active = 0;
  tmout->timestamp = 0;

  tmout->func (arg);
  return B_HANDLED_INTERRUPT;//B_INVOKE_SCHEDULER ?;
}

timeout_id_t
oss_timeout (void (*func) (void *), void *arg, unsigned long long ticks)
{
  tmout_desc_t *tmout = NULL;
  bigtime_t period;
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

  period = ticks * 1000000LL / OSS_HZ;
  add_timer (&tmout->timer, oss_timer_callback, period, B_ONE_SHOT_RELATIVE_TIMER);

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
    cancel_timer (&tmout->timer);
  tmout->active = 0;
  tmout->timestamp = 0;
}


caddr_t
oss_map_pci_mem (oss_device_t * osdev, int nr, int phaddr, int size)
{
  status_t err;
  void *va = NULL;
  FENTRYA("%p,%d,%u,%d", osdev, nr, phaddr, size);
  //XXX:align phaddr ?
  /* round up to page size */
  size += B_PAGE_SIZE - 1;
  size &= ~(B_PAGE_SIZE - 1);
  
  err = map_physical_memory(OSS_PCI_AREA_NAME, (void *)phaddr, size, 
                            B_ANY_KERNEL_BLOCK_ADDRESS, 0, &va);
  if (err < B_OK)
    va = NULL;
  FEXITR((uint32)va);
  return (caddr_t)va;
}

void
oss_unmap_pci_mem (void *addr)
{
  area_id id;
  if (addr == NULL)
    return;
  id = area_for(addr);
  if (id < B_OK)
    return;
#ifdef MEMDEBUG
  {
    area_info ai;
    if ((get_area_info(id, &ai) < B_OK) || strncmp(ai.name, OSS_PCI_AREA_NAME))
      {
        cmn_err (CE_NOTE, "oss_unmap_pci_mem: bad area (%ld)!\n", id);
        return;
      }
  }
#endif
  delete_area(id);
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
  if (osdev->dev_type != DRV_PCI || osdev->dip == NULL)
    return PCIBIOS_FAILED;
  *val = (unsigned char)gPCI->read_pci_config (osdev->dip->pciinfo.bus,
                                               osdev->dip->pciinfo.device,
                                               osdev->dip->pciinfo.function,
                                               (uchar)where, 1);
  return PCIBIOS_SUCCESSFUL;
}

int
pci_read_config_irq (oss_device_t * osdev, offset_t where, unsigned char *val)
{
  int ret;

  if (osdev->dev_type != DRV_PCI)
    return PCIBIOS_FAILED;
  ret = pci_read_config_byte (osdev, where, val);
  return ret;
}


int
pci_read_config_word (oss_device_t * osdev, offset_t where,
		      unsigned short *val)
{
  if (osdev->dev_type != DRV_PCI || osdev->dip == NULL)
    return PCIBIOS_FAILED;
  *val = (unsigned short)gPCI->read_pci_config (osdev->dip->pciinfo.bus,
                                                osdev->dip->pciinfo.device,
                                                osdev->dip->pciinfo.function,
                                                (uchar)where, 2);
  return PCIBIOS_SUCCESSFUL;
}

int
pci_read_config_dword (oss_device_t * osdev, offset_t where,
		       unsigned int *val)
{
  if (osdev->dev_type != DRV_PCI || osdev->dip == NULL)
    return PCIBIOS_FAILED;
  *val = (unsigned int)gPCI->read_pci_config (osdev->dip->pciinfo.bus,
                                              osdev->dip->pciinfo.device,
                                              osdev->dip->pciinfo.function,
                                              (uchar)where, 4);
  return PCIBIOS_SUCCESSFUL;
}

int
pci_write_config_byte (oss_device_t * osdev, offset_t where,
		       unsigned char val)
{
  if (osdev->dev_type != DRV_PCI || osdev->dip == NULL)
    return PCIBIOS_FAILED;
  gPCI->write_pci_config (osdev->dip->pciinfo.bus,
                          osdev->dip->pciinfo.device,
                          osdev->dip->pciinfo.function,
                          (uchar)where, 1, val);
  return PCIBIOS_SUCCESSFUL;
}

int
pci_write_config_word (oss_device_t * osdev, offset_t where,
		       unsigned short val)
{
  if (osdev->dev_type != DRV_PCI || osdev->dip == NULL)
    return PCIBIOS_FAILED;
  gPCI->write_pci_config (osdev->dip->pciinfo.bus,
                          osdev->dip->pciinfo.device,
                          osdev->dip->pciinfo.function,
                          (uchar)where, 2, val);
  return PCIBIOS_SUCCESSFUL;
}

int
pci_write_config_dword (oss_device_t * osdev, offset_t where,
			unsigned int val)
{
  if (osdev->dev_type != DRV_PCI || osdev->dip == NULL)
    return PCIBIOS_FAILED;
  gPCI->write_pci_config (osdev->dip->pciinfo.bus,
                          osdev->dip->pciinfo.device,
                          osdev->dip->pciinfo.function,
                          (uchar)where, 4, val);
  return PCIBIOS_SUCCESSFUL;
}



#ifdef MUTEX_CHECKS
static int oss_context = 0;	/* 0=user context, 1=interrupt context */
#endif

static int32
ossintr (void *idata)
{
  oss_device_t *osdev = idata;
  oss_native_word flags;
  //dprintf("oss:intr(%ld)!\n", osdev->irq);
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
      return B_UNHANDLED_INTERRUPT;
    }

  if (osdev->bottomhalf_handler != NULL)
    osdev->bottomhalf_handler (osdev);

  MUTEX_EXIT_IRQRESTORE (osdev->mutex, flags);
#ifdef MUTEX_CHECKS
  oss_context = saved_context;
#endif

  return B_HANDLED_INTERRUPT;
}

int
oss_register_interrupts (oss_device_t * osdev, int intrnum,
			 oss_tophalf_handler_t top,
			 oss_bottomhalf_handler_t bottom)
{
  unsigned char pci_irq_line;
  int err;
  FENTRYA(", %d, , ", intrnum);

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

  // could probably use osdev->dip->pciinfo...
  if (pci_read_config_irq (osdev, PCI_INTERRUPT_LINE, &pci_irq_line) > 0)
    return OSS_EIO;

  osdev->irq = pci_irq_line;
  osdev->tophalf_handler = top;
  osdev->bottomhalf_handler = bottom;
  err = install_io_interrupt_handler (pci_irq_line, ossintr, osdev, 0);
  dprintf("install_io_interrupt_handler (%d, %p, %p, 0) = 0x%08lx\n", pci_irq_line, ossintr, osdev, err);
  if (err < B_OK)
    {
      cmn_err (CE_WARN, "install_io_interrupt_handler failed for %s\n", osdev->nick);
      osdev->irq = -1;
      osdev->tophalf_handler = NULL;
      osdev->bottomhalf_handler = NULL;
      return err;
    }

#if DEBUG_IRQ
  atomic_add(&irq_count, 1);
#endif

  return 0;
}

void
oss_unregister_interrupts (oss_device_t * osdev)
{
  status_t err = B_OK;
  FENTRY();
  dprintf("remove_io_interrupt_handler (%d, %p, %p)\n", osdev->irq, ossintr, osdev);
  if (osdev->irq >= 0)
    err = remove_io_interrupt_handler (osdev->irq, ossintr, osdev);
  if (err < B_OK)
    cmn_err (CE_WARN, "Error removing interrupt index (%d) for %s: %s\n", 
             osdev->irq, osdev->name, strerror(err));
#if DEBUG_IRQ
  atomic_add(&irq_count, -1);
#endif
  osdev->irq = -1;
}

int
oss_register_device (oss_device_t * osdev, const char *name)
{
  static int dev_instance = 0;
  FENTRYA(", %s", name);

  DDB (cmn_err (CE_CONT, "OSS device %d is %s\n", dev_instance++, name));

  if ((osdev->name = PMALLOC (NULL, strlen (name) + 1)) == NULL)
    {
      cmn_err (CE_WARN, "Cannot allocate memory for device name\n");
      osdev->name = "Unknown device";
    }
  strcpy (osdev->name, name);
  FEXITR(0);
  return 0;
}

int
oss_disable_device (oss_device_t * osdev)
{
  int i;
  CORE_LOCK_VAR;
  FENTRY();
/*
 * This routine should check if the device is ready to be unloaded (no devices are in use).
 * If the device cannot be unloaded this routine must return OSS_EBUSY.
 *
 * If the device can be unloaded then disable any timers or other features that may cause the
 * device to be called. Also mark the audio/midi/mixer/etc devices of this device to be disabled.
 * However the interrupt handler should still stay enabled. The low level driver will call
 * oss_unregister_interrupts() after it has cleared the interrupt enable register.
 */
  LOCK_CORE();
  if (osdev->refcount > 0 || oss_open_devices > 0)
    {
      UNLOCK_CORE();
      cmn_err (CE_CONT, "Refcount %d, open_devices %d\n", osdev->refcount,
	       oss_open_devices);
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
	UNLOCK_CORE(); //needed until a benaphore is used.
	audio_uninit_device (i);
	LOCK_CORE();
      }

  UNLOCK_CORE();

  FEXIT();
  return 0;
}

void
oss_unregister_device (oss_device_t * osdev)
{
  FENTRY();
/*
 * Notice! The driver calling this routine (the owner of the osdev parameter)
 * has already uninitialized itself. Do not do any actions that may call this
 * driver directly or indirectly.
 */

/*
 * Force reload of all drivers if any application tries to open any
 * of the devices.
 */
  //do_forceload = 1;
  FEXIT();
}

void
oss_reserve_device (oss_device_t * osdev)
{
  CORE_LOCK_VAR;

  //XXX:use atomic_add() ?
  LOCK_CORE();
  osdev->refcount++;
  UNLOCK_CORE();
}

void
oss_unreserve_device (oss_device_t * osdev, int decrement)
{
  CORE_LOCK_VAR;

  //XXX:use atomic_add() ?
  LOCK_CORE();
  osdev->refcount--;
  if (osdev->refcount < 0)
    osdev->refcount = 0;
  UNLOCK_CORE();
}

void *
oss_get_osid (oss_device_t * osdev)
{
//  return osdev->osid;
  return osdev->dip;
  return NULL; // XXX:TODO
}

int
oss_get_procinfo(int what)
{
	switch (what)
	{
	case OSS_GET_PROCINFO_UID:
		return getuid();
		break;
	}

	return OSS_EINVAL;
}

oss_device_t *
osdev_create (dev_info_t * dip, int dev_type, int instance, const char *nick,
	      const char *handle)
{
  oss_device_t *osdev = NULL;
  int i, err;
  caddr_t addr;
  off_t region_size;
  CORE_LOCK_VAR;
  FENTRYA(", %d, %d, %s, %s", dev_type, instance, nick, handle);

  if (handle == NULL)
    handle = nick;

  /*
   * Don't accept any more drivers if expired
   */
  if (oss_expired && oss_num_cards > 0)
    return NULL;

  LOCK_CORE();
  for (i = 0; dip && (i < oss_num_cards); i++)
    {
      if (cards[i] == NULL)
        continue;
      if (cards[i]->available)
        continue;
      if (cards[i]->dip == dip)
	{
	  osdev = cards[i];
	  break;
	}
    }
  UNLOCK_CORE();

  if (osdev == NULL)
    {
      if ((osdev = PMALLOC (NULL, sizeof (*osdev))) == NULL)
	{
	  cmn_err (CE_WARN, "osdev_create: Out of memory\n");
	  return NULL;
	}

      LOCK_CORE();
      if (oss_num_cards >= MAX_CARDS)
	{
	  UNLOCK_CORE();
	  cmn_err (CE_PANIC, "Too many OSS devices. At most %d permitted.\n",
		   MAX_CARDS);
	  return NULL;
	}
      memset (osdev, 0, sizeof (*osdev));

      osdev->cardnum = oss_num_cards;
      cards[oss_num_cards++] = osdev;
      UNLOCK_CORE();
    }

  osdev->dip = dip;
  //osdev->osid = dip;
  osdev->unloaded = 0;
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
      /* NOP */
#ifdef __HAIKU__
      if (gPCI->reserve_device(osdev->dip->pciinfo.bus,
      				   osdev->dip->pciinfo.device,
				   osdev->dip->pciinfo.function,
				   "oss", osdev) != B_OK) {
      	cmn_err (CE_WARN, "Could not reserve PCI device\n");
      	/* XXX: CLEANUP! */
        return NULL;
      }
#endif
      break;

    case DRV_VIRTUAL:
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
	unsigned int subvendor;
	pci_read_config_dword (osdev, 0x2c, &subvendor);

	sprintf (osdev->handle, "PCI%08x-%d", subvendor, instance);
      }
      break;

    case DRV_USB:
      /* TODO: Get the vendor information */
      sprintf (osdev->handle, "USB-%s%d", handle, instance);
      break;

    default:
      sprintf (osdev->handle, "%s%d", handle, instance);
    }

  FEXIT();
  return osdev;
}

oss_device_t *
osdev_clone (oss_device_t * orig_osdev, int new_instance)
{
  oss_device_t *osdev;
  FENTRYA(", %d", new_instance);

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

  FEXIT();
  return osdev;
}

void
osdev_delete (oss_device_t * osdev)
{
  int i;
  CORE_LOCK_VAR;

  FENTRY();
  if (osdev == NULL)
    return;
  osdev->available=0;

  switch (osdev->dev_type)
    {
    case DRV_PCI:
      /* NOP */
      //pci_config_teardown (&osdev->pci_config_handle);
      //osdev->pci_config_handle = NULL;
#ifdef __HAIKU__
      gPCI->unreserve_device(osdev->dip->pciinfo.bus,
      				 osdev->dip->pciinfo.device,
				 osdev->dip->pciinfo.function,
				 "oss", osdev);
#endif
      break;
    }

/*
 * Mark all minor nodes for this module as invalid.
 */
  LOCK_CORE();
  for (i = 0; i < oss_num_cdevs; i++)
    if (oss_cdevs[i]->osdev == osdev)
      {
	oss_cdevs[i]->d = NULL;
	oss_cdevs[i]->osdev = NULL;
	strcpy (oss_cdevs[i]->name, "Removed device");
      }
  UNLOCK_CORE();

  MUTEX_CLEANUP (osdev->mutex);
  osdev->unloaded = 1;
  FEXIT();
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
  CORE_LOCK_VAR;
  FENTRYA(", %s, %d, %d, , %d", name, dev_class, instance, flags);

  if (dev_class != OSS_DEV_STATUS)
    if (oss_expired && instance > 0)
      return;
/*
 * Find if this dev_class&instance already exists (after previous module
 * detach).
 */

  LOCK_CORE();
  for (num = 0; num < oss_num_cdevs; num++)
    if (oss_cdevs[num]->d == NULL)	/* Unloaded driver */
      if (oss_cdevs[num]->dev_class == dev_class
	  && oss_cdevs[num]->instance == instance)
	{
	  cdev = oss_cdevs[num];
	  //dprintf("oss:reusing cdev[%d]\n", num);
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
	  break;
	}
  UNLOCK_CORE();

  if (cdev == NULL)
    {
      /* must alloc before locking, the rest must be atomic */
      if ((cdev = PMALLOC (NULL, sizeof (*cdev))) == NULL)
	{
	  cmn_err (CE_WARN, "Cannot allocate character device desc.\n");
	  return;
	}

      LOCK_CORE();
      if (oss_num_cdevs >= OSS_MAX_CDEVS)
	{
	   if (!grow_array(osdev, &oss_cdevs, &oss_max_cdevs, sizeof(oss_cdev_t*), 100))
	   {
	  	cmn_err (CE_WARN, "Cannot allocate new minor numbers.\n");
	    UNLOCK_CORE();
	  	return;
	   }
	}

      num = oss_num_cdevs++;
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
	  //dprintf("oss:reusing cdev[%d]: @%p\n", num, cdev);
      UNLOCK_CORE();
    }

/*
 * Export the device only if name != NULL
 */
#if 0
  if (name != NULL)
    {
      char tmp[64], *s;
      char *dev_type = "oss_sysdev";

//XXX: maybe do something ??
    }
#endif
  FEXIT();
}

int
oss_get_cardinfo (int cardnum, oss_card_info * ci)
{
  CORE_LOCK_VAR;
/*
 * Print information about a 'card' in a format suitable for /dev/sndstat
 */

  LOCK_CORE();
  if (cardnum < 0 || cardnum >= oss_num_cards)
    {
      UNLOCK_CORE();
      return OSS_ENXIO;
    }

  if (cards[cardnum]->name != NULL)
    strncpy (ci->longname, cards[cardnum]->name, 128);
  ci->shortname[127] = 0;

  if (cards[cardnum]->nick != NULL)
    strncpy (ci->shortname, cards[cardnum]->nick, 16);
  ci->shortname[15] = 0;

  if (cards[cardnum]->hw_info != NULL)
     strncpy (ci->hw_info, cards[cardnum]->hw_info, sizeof (ci->hw_info)-1);
  ci->hw_info[sizeof(ci->hw_info)-1]=0;

  UNLOCK_CORE();

  return 0;
}

/* XXX: major/minors don't exist in BeOS, WTF */
int
oss_find_minor (int dev_class, int instance)
{
  int i, minor = -1;
  CORE_LOCK_VAR;

  LOCK_CORE();
  for (i = 0; i < oss_num_cdevs; i++)
    if (oss_cdevs[i]->d != NULL && oss_cdevs[i]->dev_class == dev_class
	&& oss_cdevs[i]->instance == instance)
	  {
        minor = i;
        break;
      }
  UNLOCK_CORE();

  return minor;
}


#ifdef MUTEX_CHECKS
void
debug_mutex_init (oss_mutex_t * mutex, char *file, int line)
{
  memset (mutex, 0, sizeof (mutex));
  mutex->lock = 0;
  mutex->owner = -1;
}

void
debug_mutex_destroy (oss_mutex_t * mutex, char *file, int line)
{
  if (find_thread(NULL) == mutex->owner)
    {
      cmn_err (CE_NOTE, "%s:%d: mutex still owned (%d)\n", file, line, mutex->owner);
    }
  if (mutex->lock)
    {
      cmn_err (CE_NOTE, "%s:%d: mutex still locked (%d)\n", file, line, mutex->lock);
    }
}

void
debug_mutex_enter (oss_mutex_t * mutex, char *file, int line, oss_native_word *flags)
{
  if (find_thread(NULL) == mutex->owner)
    {
      cmn_err (CE_NOTE, "%s:%d: Re-entrant mutex (%s:%d %d)\n", file, line,
	       mutex->file, mutex->line, mutex->busy_flags);
      return;
    }

  mutex->file = file;
  mutex->line = line;
  mutex->busy_flags = flags ? CNTX_INTR : CNTX_USER;
  if (flags)
    *flags = (oss_native_word) disable_interrupts();
  acquire_spinlock (&mutex->lock);
  
}

void
debug_mutex_exit (oss_mutex_t * mutex, char *file, int line, oss_native_word *flags)
{
  if (find_thread(NULL) != mutex->owner)
    {
      cmn_err (CE_NOTE, "Mutex not owned %s:%d\n", file, line);
    }
  else
    {
      release_spinlock(&mutex->lock);
      if (flags)
        restore_interrupts((cpu_status)*flags);
    }

  mutex->owner = -1;
  mutex->file = NULL;
  mutex->line = 0;
  mutex->busy_flags = 0;
}
#endif


void
oss_load_options (oss_device_t * osdev, oss_option_map_t map[])
{
  char name[32] = OSS_CONFIG_FILE_PREFIX "core";
  void *handle;
  const char *valstr;
  int i;

  if (osdev == NULL)
    return;
  
  /* if not core module, take the module name */
  if (strcmp(osdev->modname, "oss"))
    strncpy (name, osdev->modname, 16);

  dprintf("oss_load_options(): %s\n", name);
  handle = load_driver_settings (name);
  /* not there */
  if (handle == NULL)
    return;
  
  for (i = 0; map[i].name != NULL; i++)
    {
      /* discard given without value */
      if ((valstr =
	   get_driver_parameter (handle, 
			     map[i].name, NULL, NULL)) != NULL)
	{
	  int base = 10;
	  if (!strncmp (valstr, "0x", 2))
	    {
	      base = 16;
	      valstr += 2;
	    }
	  *map[i].ptr = (int)strtol (valstr, NULL, base);
	}
    }
  unload_driver_settings (handle);
}

#ifdef DEBUG_KDLCMD
static int
kdl_oss_dump_cards(int argc, char **argv)
{
  int i;
  kprintf("oss_num_cards = %d\n", oss_num_cards);
  for (i = 0; i < oss_num_cards; i++)
    {
      if (cards[i] == NULL)
        continue;
      kprintf("oss_cards[%d] = {\n", i);
      kprintf("  cardnum= %d\n", cards[i]->cardnum);
      kprintf("  dev_type= %d\n", cards[i]->dev_type);
      kprintf("  instance= %d\n", cards[i]->instance);
      
      kprintf("  unloaded= %d\n", cards[i]->unloaded);
      
      kprintf("  name= %s\n", cards[i]->name);
      kprintf("  nick= %-16s\n", cards[i]->nick);
      kprintf("  modname= %-32s\n", cards[i]->modname);
      kprintf("  handle= %-32s\n", cards[i]->handle);
      kprintf("  num_audio_engines= %d\n", cards[i]->num_audio_engines);
      kprintf("  num_audioplay= %d\n", cards[i]->num_audioplay);
      kprintf("  num_audiorec= %d\n", cards[i]->num_audiorec);
      kprintf("  num_audioduplex= %d\n", cards[i]->num_audioduplex);
      kprintf("  num_mididevs= %d\n", cards[i]->num_mididevs);
      kprintf("  num_mixerdevs= %d\n", cards[i]->num_mixerdevs);

      kprintf("  refcount= %d\n", cards[i]->refcount);

      kprintf("  irq= %d\n", cards[i]->irq);
      kprintf("}\n");
    }
  return 0;
}

static int
kdl_oss_dump_cdevs(int argc, char **argv)
{
  int i;
  kprintf("oss_num_cdevs = %d\n", oss_num_cdevs);
  for (i = 0; i < oss_num_cdevs; i++)
    {
      if (oss_cdevs[i] == NULL)
        continue;
      kprintf("oss_cdevs[%d] = {\n", i);
      kprintf("  dev_class= %d\n", oss_cdevs[i]->dev_class);
      kprintf("  instance= %d\n", oss_cdevs[i]->instance);
      kprintf("  name= %-32s\n", oss_cdevs[i]->name);
      kprintf("  osdev= %p\n", oss_cdevs[i]->osdev);
      //kprintf("  opencount= %d\n", open_count[i]);
      kprintf("}\n");
    }
  return 0;
}
#endif

/*
 * Driver entry point routines
 */

status_t
init_osscore (void)
{
  int err = 0;
  oss_device_t *osdev;
  FENTRY();

#ifdef LICENSED_VERSION
/*WRITEME*/
#endif

  add_debugger_command("oss_dump_cards", &kdl_oss_dump_cards, "Dump the OSS cards[] array.");
  add_debugger_command("oss_dump_cdevs", &kdl_oss_dump_cdevs, "Dump the OSS cdevs[] array.");

  //MUTEX_INIT (osdev, osscore_mutex, MH_TOP);
  CORE_LOCK_INIT();
  
  if ((osdev = osdev_create (NULL, DRV_VIRTUAL, 0, "oss", NULL)) == NULL)
    {
      cmn_err (CE_WARN, "Creating osdev failed\n");
      FEXITR(ENOMEM);
      return OSS_ENOMEM;
    }

  
  oss_load_options (osdev, oss_global_options);
  
  oss_common_init (osdev);

  oss_register_device (osdev, "OSS core services");

  FEXIT();
  return 0;
}

status_t
uninit_osscore (void)
{
  int i;
  static int already_unloaded = 0;
  FENTRY();

  if (oss_open_devices > 0)
    return EBUSY;

  if (already_unloaded)
    return 0;
  already_unloaded = 1;

  oss_unload_drivers ();

  //LOCK_CORE();
  for (i = 0; i < nmemblocks; i++)
    KERNEL_FREE (memblocks[i]);
  nmemblocks = 0;

  CORE_LOCK_CLEANUP();
  //MUTEX_CLEANUP (osscore_mutex);

  remove_debugger_command("oss_dump_cdevs", &kdl_oss_dump_cdevs);
  remove_debugger_command("oss_dump_cards", &kdl_oss_dump_cards);

#if DEBUG_IRQ
  dprintf("oss: %ld irq handlers left\n", irq_count);
#endif


  return 0;
}

const char **
oss_publish_devices(void)
{
  int i, j;
  char *name;
  CORE_LOCK_VAR;

  FENTRY();

  LOCK_CORE();
  gDeviceNames = realloc(gDeviceNames, (oss_num_cdevs + 1) * sizeof(char *) 
    + (oss_num_cdevs * DEVICE_NAME_LEN));
  if (gDeviceNames == NULL) {
    UNLOCK_CORE();
    FEXIT();
  	return NULL;
  }
  name = (char *)(&gDeviceNames[oss_num_cdevs + 1]);
  for (i = 0; i < oss_num_cdevs+1; i++)
    gDeviceNames[i] = NULL;
  //dprintf("oss_num_cdevs = %d\n", oss_num_cdevs);
  for (i = 0, j = 0; i < oss_num_cdevs; i++)
    {
      oss_cdev_t *cdev = oss_cdevs[i];
      if (cdev && cdev->d)
        {
          strcpy(name, DEVICE_PREFIX);
          strncat(name, cdev->name, 32-1);
          //dprintf("oss: publishing %s\n", name);
          gDeviceNames[j++] = name;
          name += DEVICE_NAME_LEN;
        }
    }
  UNLOCK_CORE();

	FEXIT();
  return (const char**)gDeviceNames;
}

device_hooks *oss_get_driver_hooks (void)
{
  return &oss_driver_hooks;
}

status_t
oss_load_drivers (void)
{
  oss_drv_module_info *drv;
  char module[256];
  size_t modulesz = sizeof(module);
  void *cookie;
  status_t err = ENOENT;
  FENTRY();
  cookie = open_module_list(OSS_MODULES_PREFIX);
  if (cookie == NULL)
    goto err1;
  while (read_next_module_name(cookie, module, &modulesz) >= B_OK)
    {
      err = get_module(module, (module_info **)&drv);
      if (err >= B_OK)
        {
          if (drv->driver_probe() < B_OK)
            put_module(module);
        }
      modulesz = sizeof(module);
    }
  err = B_OK;
err2:
  close_module_list(cookie);
err1:
  FEXITR(err);
  return B_OK;
}

status_t
oss_probe_pci (void)
{
  FENTRY();
  //XXX:TODO:remove me
  //ali5455_probe();
  //atiaudio_probe();
  FEXIT();
  return B_OK;
}


static status_t
unload_driver (const char *nick)
{
  oss_drv_module_info *drv;
  char module[256];
  status_t err = B_OK;
  int i;
  CORE_LOCK_VAR;
  FENTRYA("%s", nick);

  /* skip ourselves */
  if (!strcmp(nick, "oss"))
    goto err1;

  sprintf(module, "%s%s%s", OSS_MODULES_PREFIX, nick, OSS_MODULES_SUFFIX);
  err = get_module(module, (module_info **)&drv);
  if (err < B_OK)
    goto err1;

  err = EBUSY;
  
  LOCK_CORE();
  /* detach all osdevs for this driver */
  for (i = 0; i < oss_num_cards; i++)
    {
      oss_device_t *osdev = cards[i];
      if (!osdev)
        continue;
      if (strncmp(osdev->modname, nick, 32))
        continue;
      UNLOCK_CORE();
      err = drv->driver_detach(osdev);
      if (err < B_OK)
        goto err2;
      LOCK_CORE();
      //cards[i] = NULL; // XXX: not sure...
    }
  UNLOCK_CORE();
  err = B_OK;
  put_module(module);
  /* twice */
err2:
  put_module(module);
err1:
  FEXITR(err);
  return err;
}

status_t
oss_unload_all_drivers(void)
{
  char module[256];
  status_t err = B_OK;
  int i;
  CORE_LOCK_VAR;
  FENTRY();

  /* for each osdev still around, unload the module it came from,
   * which should delete them.
   */
  LOCK_CORE();
  for (i = 0; i < oss_num_cards; i++)
    {
      if (!cards[i])
        continue;
      if (cards[i]->unloaded)
        continue;
      UNLOCK_CORE();
      err = unload_driver(cards[i]->modname);
      if (err < B_OK)
        break;
      LOCK_CORE();
    }
  UNLOCK_CORE();
  
  FEXITR(err);
  return err;
}


static status_t
oss_stdops(int32 op, ...)
{
  status_t err;
  switch (op)
    {
    case B_MODULE_INIT:
      //vmix_disabled = 1; /* disable vmix */
      err = get_module(B_PCI_MODULE_NAME, (module_info **)&gPCI);
      if (err >= B_OK)
        {
          //XXX:call init_osscore() here
          return err;
          put_module(B_PCI_MODULE_NAME);
        }
      return err;
    case B_MODULE_UNINIT:
      free(gDeviceNames);
      put_module(B_PCI_MODULE_NAME);
      return B_OK;

    }
  return B_ERROR;
}

oss_core_module_info gOSSCoreModule = {
  {
    OSS_CORE_MODULE_NAME,
    /*B_KEEP_LOADED*/0,
    oss_stdops,
  },
  init_osscore,
  uninit_osscore,
  oss_publish_devices,
  oss_get_driver_hooks,
  oss_probe_pci,
  oss_load_drivers,
  oss_unload_all_drivers,
};

// for f in kernel/drv/*; do echo "extern oss_drv_module_info gModule_$(basename $f);"; done
extern oss_drv_module_info gModule_oss_ali5455;
extern oss_drv_module_info gModule_oss_atiaudio;
extern oss_drv_module_info gModule_oss_audigyls;
//extern oss_drv_module_info gModule_oss_audiocs;
extern oss_drv_module_info gModule_oss_audioloop;
extern oss_drv_module_info gModule_oss_audiopci;
extern oss_drv_module_info gModule_oss_cmi878x;
extern oss_drv_module_info gModule_oss_cmpci;
extern oss_drv_module_info gModule_oss_cs4281;
extern oss_drv_module_info gModule_oss_cs461x;
extern oss_drv_module_info gModule_oss_digi96;
extern oss_drv_module_info gModule_oss_emu10k1x;
extern oss_drv_module_info gModule_oss_envy24;
extern oss_drv_module_info gModule_oss_envy24ht;
extern oss_drv_module_info gModule_oss_fmedia;
extern oss_drv_module_info gModule_oss_geode;
extern oss_drv_module_info gModule_oss_hdaudio;
extern oss_drv_module_info gModule_oss_ich;
//extern oss_drv_module_info gModule_oss_imux;
//extern oss_drv_module_info gModule_oss_midiloop;
//extern oss_drv_module_info gModule_oss_midimix;
//extern oss_drv_module_info gModule_oss_sadasupport;
extern oss_drv_module_info gModule_oss_sblive;
extern oss_drv_module_info gModule_oss_sbpci;
extern oss_drv_module_info gModule_oss_sbxfi;
extern oss_drv_module_info gModule_oss_solo;
extern oss_drv_module_info gModule_oss_trident;
//extern oss_drv_module_info gModule_oss_usb;
//extern oss_drv_module_info gModule_oss_userdev;
extern oss_drv_module_info gModule_oss_via823x;
extern oss_drv_module_info gModule_oss_via97;
extern oss_drv_module_info gModule_oss_ymf7xx;
//extern oss_drv_module_info gModule_osscore;

module_info *modules[] = {
  (module_info *)&gOSSCoreModule,
//for f in kernel/drv/*; do echo "  (module_info *)&gModule_$(basename $f),"; done >> kernel/OS/BeOS/os_beos.c  (module_info *)&gModule_oss_ali5455,
  (module_info *)&gModule_oss_atiaudio,
  (module_info *)&gModule_oss_audigyls,
  //(module_info *)&gModule_oss_audiocs,
  //(module_info *)&gModule_oss_audioloop,
  (module_info *)&gModule_oss_audiopci,
  (module_info *)&gModule_oss_cmi878x,
  (module_info *)&gModule_oss_cmpci,
  (module_info *)&gModule_oss_cs4281,
  (module_info *)&gModule_oss_cs461x,
  (module_info *)&gModule_oss_digi96,
  (module_info *)&gModule_oss_emu10k1x,
  (module_info *)&gModule_oss_envy24,
  (module_info *)&gModule_oss_envy24ht,
  (module_info *)&gModule_oss_fmedia,
  (module_info *)&gModule_oss_geode,
  (module_info *)&gModule_oss_hdaudio,
  (module_info *)&gModule_oss_ich,
  //(module_info *)&gModule_oss_imux,
  //(module_info *)&gModule_oss_midiloop,
  //(module_info *)&gModule_oss_midimix,
  //(module_info *)&gModule_oss_sadasupport,
  (module_info *)&gModule_oss_sblive,
  (module_info *)&gModule_oss_sbpci,
  (module_info *)&gModule_oss_sbxfi,
  (module_info *)&gModule_oss_solo,
  (module_info *)&gModule_oss_trident,
  //(module_info *)&gModule_oss_usb,
  //(module_info *)&gModule_oss_userdev,
  (module_info *)&gModule_oss_via823x,
  (module_info *)&gModule_oss_via97,
  (module_info *)&gModule_oss_ymf7xx,
//  (module_info *)&gModule_osscore,

#if 0 /* OLD */


  (module_info *)&gModule_oss_ali5455,
  
  (module_info *)&gModule_oss_allegro,
  (module_info *)&gModule_oss_als300,
  (module_info *)&gModule_oss_als4000,
  (module_info *)&gModule_oss_apci97,
  
  (module_info *)&gModule_oss_atiaudio,
  
  (module_info *)&gModule_oss_audigyls,
  //(module_info *)&gModule_oss_audioloop,
  (module_info *)&gModule_oss_audiopci,
  (module_info *)&gModule_oss_cmi8788,
  (module_info *)&gModule_oss_cmpci,
  (module_info *)&gModule_oss_cs4280,
  (module_info *)&gModule_oss_cs4281,
  (module_info *)&gModule_oss_digi32,
  (module_info *)&gModule_oss_digi96,
  (module_info *)&gModule_oss_emu10k1x,
  (module_info *)&gModule_oss_envy24,
  (module_info *)&gModule_oss_envy24ht,
  (module_info *)&gModule_oss_fm801,
  (module_info *)&gModule_oss_geode,
  (module_info *)&gModule_oss_hdaudio,
  
  (module_info *)&gModule_oss_ich,
  
#ifdef ENABLE_IMUX
  (module_info *)&gModule_oss_imux,
#endif
  (module_info *)&gModule_oss_maestro,
  (module_info *)&gModule_oss_neomagic,
  (module_info *)&gModule_oss_s3vibes,
  (module_info *)&gModule_oss_sblive,
#ifdef ENABLE_SOFTOSS
  //(module_info *)&gModule_oss_softoss,
#endif
  (module_info *)&gModule_oss_solo,
  (module_info *)&gModule_oss_trident,
  (module_info *)&gModule_oss_via8233,
  (module_info *)&gModule_oss_via97,
  (module_info *)&gModule_oss_vortex,
  (module_info *)&gModule_oss_ymf7xx,
#endif
  
  NULL
};

/*
 * Driver hooks
 */


typedef struct ossdev_cookie {
  int minor; /* index into cdevs[] */
  oss_cdev_t *cdev;
  struct fileinfo file;
} ossdev_cookie_t;

static int find_cdev(const char *name)
{
  int i, which = -1;
  CORE_LOCK_VAR;
//  if (strlen(name) < strlen(DEVICE_PREFIX))
//    return -1;
  name += strlen(DEVICE_PREFIX);
  
  LOCK_CORE();
  for (i = 0; i < oss_num_cdevs; i++)
    {
      oss_cdev_t *cdev = oss_cdevs[i];
      //dprintf("oss:find_cdev: cdev %p, ->d %p, %s <> %s\n", cdev, cdev?cdev->d:NULL, cdev?cdev->name:"-", name);
      if (cdev && cdev->d && !strncmp(cdev->name, name, 32))
        {
          which = i;
          break;
        }
    }
  UNLOCK_CORE();
  
  return which;
}

static status_t
ossdrv_open(const char *name, uint32 oflags, void **cookie)
{
  ossdev_cookie_t *c;
  status_t err;
  int dev, tmpdev;
  oss_cdev_t *cdev;
  CORE_LOCK_VAR;
  FENTRYA("%s, %ld, ", name, oflags);

  dev = find_cdev(name);
  err = ENOENT;
  if (dev < 0)
    goto err1;
  err = ENXIO;
  if ((cdev = oss_cdevs[dev]) == NULL || cdev->d == NULL)
    goto err1;

  DDB (cmn_err
       (CE_CONT, "oss_cdev_open(%d): %s, class=%d, instance=%d\n", dev,
	cdev->name, cdev->dev_class, cdev->instance));

  err = ENODEV;
  if (cdev->d->open == NULL)
    goto err1;

  err = ENOMEM;
  c = malloc(sizeof(ossdev_cookie_t));
  if (!c)
    goto err1;
  
  switch (oflags & O_RWMASK)
    {
    case O_RDONLY:
      c->file.mode = OPEN_READ;
      break;
    case O_WRONLY:
      c->file.mode = OPEN_WRITE;
      break;
    case O_RDWR:
      c->file.mode = OPEN_READWRITE;
      break;
    default:
      err = EINVAL;
      goto err2;
    }
  //c->file.flags = oflags;
  c->file.acc_flags = oflags;

  tmpdev = -1;
  err =
    cdev->d->open (cdev->instance, cdev->dev_class, &c->file, 0, 0, &tmpdev);
  if (err < 0)
    goto err3;
  if (tmpdev > -1)
    dev = tmpdev;

  c->minor = dev;
  c->cdev = cdev = oss_cdevs[dev];
  
  //XXX:locking
  LOCK_CORE();
  oss_open_devices++;
  //open_count[dev]++;
  UNLOCK_CORE();

  *cookie = c;
  FEXITR(B_OK);
  return B_OK;

err3:
err2:
  free(c);
err1:
  FEXITR(err);
  return err;
}


static status_t
ossdrv_close(void *cookie)
{
  FENTRY();
  FEXIT();
  return B_OK;
}


static status_t
ossdrv_freecookie(ossdev_cookie_t *cookie)
{
  oss_cdev_t *cdev = cookie->cdev;
  int dev = cookie->minor;
  CORE_LOCK_VAR;
  status_t err;
  FENTRY();

  err = ENXIO;
  if (dev >= OSS_MAX_CDEVS)
    goto err;

#if 0 // not safe (not locked)
  err = B_OK;
  if (open_count[dev] == 0)	/* Not opened */
    goto err;
#endif

  err = ENXIO;
  if (cdev != oss_cdevs[dev])
    goto err;

  //in close or free ??
  cdev->d->close (cdev->instance, &cookie->file);

  LOCK_CORE();
  oss_open_devices--;
  //open_count[dev]--;
  UNLOCK_CORE();
  
  free(cookie);
  err = B_OK;
err:
  FEXITR(err);
  return B_OK;
}

/* note: IOC stuff seems to not collide with the base ioctls from Drivers.h
 * as long as the type field is printable ascii (>32). Lucky are we.
 */
static status_t
ossdrv_ioctl(ossdev_cookie_t *cookie, uint32 op, void *buffer, size_t length)
{
  oss_cdev_t *cdev = cookie->cdev;
  status_t err = ENXIO;
  uint32 cmd = op;
  int len = 0;
  char buf[4096];
  FENTRYA(", %lx, , %ld", op, length);
  
  if ((cdev != oss_cdevs[cookie->minor]) || cdev->d->ioctl == NULL)
    goto err1;

  if (cmd & (SIOC_OUT | SIOC_IN))
    {

      len = (cmd >> 16) & SIOCPARM_MASK;
      if (len < 0)
	len = 0;
      if (len > sizeof (buf))
	{
	  cmn_err (CE_WARN, "Bad ioctl buffer size %d\n", len);
	  err = EFAULT;
	  goto err1;
	}

      if ((cmd & SIOC_IN) && len > 0)
	{
	  memcpy (buf, buffer, len);
	    //return EFAULT;
	}

    }

  err = cdev->d->ioctl (cdev->instance, &cookie->file, cmd, (ioctl_arg) buf);

  if ((cmd & SIOC_OUT) && len > 0)
    {
      memcpy (buffer, buf, len);
	//return EFAULT;
    }


err1:
  FEXITR(err);
  return err;
}


static status_t
ossdrv_read(ossdev_cookie_t *cookie, off_t pos, void *buffer, size_t *_length)
{
  oss_cdev_t *cdev = cookie->cdev;
  uio_t uio;
  int count = *_length;
  int err;
  FENTRYA(", %Ld, , %ld", pos, *_length);

  err = ENXIO;
  if ((cdev != oss_cdevs[cookie->minor]) || cdev->d->read == NULL)
    goto err;
  //files[dev].acc_flags = uiop->uio_fmode;
  if ((err = oss_create_uio (&uio, buffer, count, UIO_READ, 0)) < 0)
    goto err;

  err = cdev->d->read (cdev->instance, &cookie->file, &uio, count);
  //*_length = (err > 0) ? err : 0;
  if (err < 0)
    goto err;
  
  *_length = err;
  err = B_OK;
  FEXITR(err);
  return err;

err:
  *_length = 0;
  FEXITR(err);
  return err;
}


static status_t
ossdrv_write(ossdev_cookie_t *cookie, off_t pos, const void *buffer, size_t *_length)
{
  oss_cdev_t *cdev = cookie->cdev;
  uio_t uio;
  int count = *_length;
  int err;
  FENTRYA(", %Ld, , %ld", pos, *_length);

  err = ENXIO;
  if ((cdev != oss_cdevs[cookie->minor]) || cdev->d->write == NULL)
    goto err;
  //files[dev].acc_flags = uiop->uio_fmode;
  if ((err = oss_create_uio (&uio, (void *)buffer, count, UIO_WRITE, 0)) < 0)
    goto err;

  err = cdev->d->write (cdev->instance, &cookie->file, &uio, count);
  //*_length = (err > 0) ? err : 0;
  if (err < 0)
    goto err;
  
  *_length = err;
  err = B_OK;
  FEXITR(err);
  return err;

err:
  *_length = 0;
  FEXITR(err);
  return err;
}


device_hooks oss_driver_hooks = {
  &ossdrv_open,
  &ossdrv_close,
  (device_free_hook)&ossdrv_freecookie,
  (device_control_hook)&ossdrv_ioctl,
  (device_read_hook)&ossdrv_read,
  (device_write_hook)&ossdrv_write,
  /* Leave select/deselect/readv/writev undefined. The kernel will
   * use its own default implementation. The basic hooks above this
   * line MUST be defined, however. */
  NULL,
  NULL,
  NULL,
  NULL
};
