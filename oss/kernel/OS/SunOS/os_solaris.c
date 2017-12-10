/*
 * Purpose: Operating system abstraction functions for Solaris
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
#if !defined(SOL9) && !defined(SOL8)
#include <sys/sunldi.h>
#endif
#include <sys/mman.h>

#if 1
/*
 * Some older DDI routines are used by OSS instead of the latest ones.
 * In this way the same OSS binary can be run both under Sol10 and Sol11.
 */
uint16_t ddi_mem_get16 (ddi_acc_handle_t handle, uint16_t * host_addr);
uint32_t ddi_mem_get32 (ddi_acc_handle_t handle, uint32_t * host_addr);
void ddi_mem_put16 (ddi_acc_handle_t handle, uint16_t * dev_addr,
		    uint16_t value);
void ddi_mem_put32 (ddi_acc_handle_t handle, uint32_t * dev_addr,
		    uint32_t value);
int ddi_mem_alloc (dev_info_t * dip, ddi_dma_lim_t * limits, uint_t length,
		   uint_t flags, caddr_t * kaddrp, uint_t * real_length);
void ddi_mem_free (caddr_t kaddr);
#endif

#if 0
/* TODO: Disable this debugging stuff */
unsigned char tmp_status = 0;
# define UP_STATUS(v) OUTB(NULL, (tmp_status=tmp_status|(v)), 0x378)
# define DOWN_STATUS(v) OUTB(NULL, (tmp_status=tmp_status&~(v)), 0x378)
#else
# define UP_STATUS(v)
# define DOWN_STATUS(v)
#endif

static int oss_expired = 0;

static oss_mutex_t osscore_mutex;

#define TRC(x)
#define TRC2(x)

#define FIX_RET_VALUE(ret) (((ret)<0) ? -(ret) : 0)

static volatile int open_devices = 0;
static volatile int do_forceload = 0;
static volatile int forceload_in_progress = 0;

/*
 * MAX_CARDS must be larger than life. The system will panic if there are
 * more sound devices (cards os sound chips) than MAX_CARDS.
 */
#define MAX_CARDS 32

static oss_device_t *cards[MAX_CARDS];
int oss_num_cards = 0;

int oss_detach_enabled = 0;

#ifdef MUTEX_CHECKS
static volatile int inside_intr = 0;	/* For mutex debugging only */
#endif

/*
 * These are the entry points into our driver that are called when the
 * driver is loaded, during a system call, or in response to an interrupt.
 */
static int oss_getinfo (dev_info_t * dip, ddi_info_cmd_t infocmd, void *arg,
			void **result);
static int oss_attach (dev_info_t * dip, ddi_attach_cmd_t cmd);
static int oss_detach (dev_info_t * dip, ddi_detach_cmd_t cmd);
static void free_all_contig_memory (void);

/*
 * DMA memory management
 */

typedef struct contig_desc
{
  int is_special;
  struct contig_desc *next;
  unsigned char *first_addr, *last_addr;
  unsigned long physaddr;
  void *orig_buf;

  oss_device_t *osdev;
  ddi_dma_handle_t handle;
  ddi_dma_handle_t dhandle;
  ddi_acc_handle_t dma_acc_handle;
  ddi_dma_cookie_t cookie;
  ddi_dma_win_t win;
  ddi_dma_seg_t seg;
  size_t size;
}
contig_desc;

static contig_desc *contig_list = NULL;

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

#define CDEV_NUMHASH	79 // Prime number
static oss_cdev_t *cdev_hash[CDEV_NUMHASH] = {NULL};
#define compute_cdev_hash(dev_class, instance) (dev_class*13+instance) % CDEV_NUMHASH

static void
unlink_cdev_hash(oss_cdev_t *this_cdev)
{
	oss_cdev_t *cdev = cdev_hash[compute_cdev_hash(this_cdev->dev_class, this_cdev->instance)];

	if (cdev == this_cdev) // First in the hash chain
	   {
		cdev_hash[compute_cdev_hash(this_cdev->dev_class, this_cdev->instance)] = cdev->hl;
		return;
	   }

	while (cdev != NULL)
	   {
		if (cdev->hl == this_cdev)
		   {
			cdev->hl = this_cdev->hl;
			return;
		   }

		cdev = cdev->hl;
	   }

	cmn_err(CE_WARN, "unlink_cdev_hash: Cannot find cdev %p\n", this_cdev);
}

#if 1
/*
 * Unfortunately this handy function is not safe (incompatibilities
 * between kenel versions/builds). So let's hope it gets moved to
 * the kernel.
 */
char *
oss_get_procname (void)		/* Return the command name of the current thread */
{
  return ttoproc (curthread)->p_user.u_comm;
}
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

  if (ptr == NULL)
    return NULL;

  len = (uint64_t *) ptr;

  ptr += sizeof (uint64_t);
  *len = size + sizeof (uint64_t);

#ifdef MEMDEBUG
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
oss_kmem_free (void *addr)
{
  uint64_t *len;

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
	  break;
	}
  }
#endif
}

static const ddi_dma_attr_t dma_attr_pci = {
  DMA_ATTR_V0,          // Version
  0x00000000,        // Address low
  0xffffffff,        // Address high
  0xfffffffe,        // Counter max
  4,                 // Default byte align
  0x3c,                 // Burst size
  4,                  // Minimum xfer size
  0xffffffff,        // Maximum xfer size
  0xffffffff,        // Max segment size
  1,                    // S/G list length
  4,                    // Granularity
  0                     // Flag
};

#if !defined(SOL9) && !defined(SOL8)
static void
forceload_drivers (dev_t dev, cred_t * credp)
{
/*
 * This routine will be called whenever the first application tries
 * to access OSS devices. It's purpose is to load all the drivers
 * just in case they have not been loaded yet. In this way it's possible to
 * guarantee that the audio device numbering doesn't change between 
 * reboots.
 */
  char path[20];

  int i, err;
  ldi_handle_t lh;
  ldi_ident_t li;
  DDB (cmn_err (CE_NOTE, "Forceloading OSS devices\n"));

  if (ldi_ident_from_dev (dev, &li) != 0)
    {
      cmn_err (CE_NOTE, "ldi_ident_from_dev failed\n");
      return;
    }

  if ((err = ldi_open_by_name ("/dev/sndstat", FWRITE, credp, &lh, li)) != 0)
    {
      if (err != ENODEV)
	cmn_err (CE_NOTE, "Forceload error %d (/dev/sndstat)\n", err);
    }
  else
    err = ldi_close (lh, FWRITE, credp);

  for (i = 0; i < MAX_MIXER_DEV; i++)
    {
      sprintf (path, "/dev/mixer%d", i);

      if ((err = ldi_open_by_name (path, FWRITE, credp, &lh, li)) != 0)
	{
	  if (err != ENODEV)
	    cmn_err (CE_NOTE, "Forceload error %d\n", err);
	}
      else
	err = ldi_close (lh, FWRITE, credp);
    }

  for (i = 0; i < MAX_AUDIO_DEVFILES; i++)
    {
      sprintf (path, "/dev/dsp%d", i);

      if ((err = ldi_open_by_name (path, FWRITE, credp, &lh, li)) != 0)
	{
	  if (err != ENODEV)
	    cmn_err (CE_NOTE, "Forceload error %d\n", err);
	}
      else
	err = ldi_close (lh, FWRITE, credp);
    }

  ldi_ident_release (li);
  DDB (cmn_err (CE_NOTE, "Forceloading OSS devices done\n"));
}

void
oss_forceload_drivers (int dev, cred_t * cred_p)
{

  if (do_forceload)
    {
      do_forceload = 0;
      forceload_in_progress = 1;
      forceload_drivers (dev, cred_p);
      forceload_in_progress = 0;
    }
}
#endif

/*ARGSUSED*/
int
oss_open (dev_t * dev_p, int open_flags, int otyp, cred_t * cred_p)
{
  int retval;
  dev_t dev = *dev_p;
  oss_cdev_t *cdev;
  int tmpdev, maj;
  oss_native_word flags;

#ifdef DO_TIMINGS
  oss_timing_printf ("**** oss_open(%x) ****", getminor (dev));
#endif
//cmn_err(CE_CONT, "**** oss_open(%x) ****\n", getminor (dev));
//cmn_err(CE_CONT, "  PID %d, cmd %s\n", GET_PROCESS_PID(x), GET_PROCESS_NAME(x));
  maj = getmajor (dev);
  dev = getminor (dev);

#if !defined(SOL9) && !defined(SOL8)
/*
 * Handle driver forceload
 */

  if (forceload_in_progress)
    return ENODEV;

  oss_forceload_drivers (dev, cred_p);
#endif

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
  *dev_p = makedevice (maj, tmpdev);
  dev = tmpdev;

  if (retval < 0)
    {
      return -retval;
    }

  MUTEX_ENTER_IRQDISABLE (osscore_mutex, flags);
  open_devices++;
  cdev = oss_cdevs[dev];	/* Switch to the cdev that was actually opened */

  cdev->open_count++;
  if (open_flags & FREAD && open_flags & FWRITE)
    cdev->file.mode = OPEN_READWRITE;
  else if (open_flags & FREAD)
    cdev->file.mode = OPEN_READ;
  else if (open_flags & FWRITE)
    cdev->file.mode = OPEN_WRITE;

  oss_reserve_device (cdev->osdev);
//cmn_err(CE_CONT, "Increment open_devices=%d, refcount=%d\n", open_devices, cdev->osdev->refcount);
  MUTEX_EXIT_IRQRESTORE (osscore_mutex, flags);

  return 0;
}

/*ARGSUSED*/
int
oss_close (dev_t dev, int flag, int otyp, cred_t * cred_p)
{
  oss_cdev_t *cdev;
  oss_native_word flags;
#ifdef DO_TIMINGS
  oss_timing_printf ("***** oss_close(%x) ****", getminor (dev));
#endif
//cmn_err(CE_CONT, "***** oss_close(%x) ****\n", getminor (dev));
//cmn_err(CE_CONT, "    PID %d, cmd %s\n", GET_PROCESS_PID(x), GET_PROCESS_NAME(x));

  if (getminor (dev) >= OSS_MAX_CDEVS)
    {
      cmn_err (CE_NOTE, "Closing bad minor %d\n", getminor (dev));
      return ENXIO;
    }

  dev = getminor (dev);
  if ((cdev = oss_cdevs[dev]) == NULL || cdev->d == NULL)
    {
      cmn_err (CE_NOTE, "Closing undefined minor %d\n", getminor (dev));
      return ENXIO;
    }


  if (cdev->open_count == 0)	/* Not opened */
    {
      cmn_err (CE_NOTE, "Closing minor %d that is not open\n", dev);
      return 0;
    }

  cdev->d->close (cdev->instance, &cdev->file);

  MUTEX_ENTER_IRQDISABLE (osscore_mutex, flags);
  open_devices -= cdev->open_count;
//cmn_err(CE_CONT, "Decrement open_devices=%d\n", open_devices);
  oss_unreserve_device (cdev->osdev, cdev->open_count);
  cdev->open_count = 0;
  MUTEX_EXIT_IRQRESTORE (osscore_mutex, flags);
  return 0;
}

/*ARGSUSED*/
int
oss_ioctl (dev_t dev, int cmd, intptr_t arg, int mode, cred_t * cred_p,
	   int *rval_p)
{
  int retval;
  int len = 0;
  char b[4096], *buf = b;
  oss_cdev_t *cdev;
#ifdef DO_TIMINGS
  oss_timing_printf ("OSS ioctl(%x, %x, %x)", getminor (dev), cmd, arg);
#endif

  *rval_p = 0;

  dev = getminor (dev);
  if ((cdev = oss_cdevs[dev]) == NULL || cdev->d->ioctl == NULL)
    return ENXIO;

  if (cdev->open_count == 0)	/* Not opened */
    {
      cmn_err (CE_NOTE, "Call to ioctl on  minor %d that is not open\n", dev);
      return ENXIO;
    }

  if (oss_expired)
    return ENODEV;

  if (mode & FKIOCTL)		/* Called from kernel space */
    buf = (char *) arg;
  else if (cmd & (SIOC_OUT | SIOC_IN))
    {

      len = (cmd >> 16) & SIOCPARM_MASK;
      if (len < 0)
	len = 0;
      if (len > sizeof (b))
	{
	  cmn_err (CE_WARN, "Bad ioctl buffer size %d\n", len);
	  return EFAULT;
	}

      if ((cmd & SIOC_IN) && len > 0)
	{
	  if (copyin ((char *) arg, buf, len) == -1)
	    return EFAULT;
	}

    }

  retval = cdev->d->ioctl (cdev->instance, &cdev->file, cmd, (ioctl_arg) buf);

  if (!(mode & FKIOCTL))	/* Not called from kernel space */
    if ((cmd & SIOC_OUT) && len > 0)
      {
	if (copyout (buf, (char *) arg, len) == -1)
	  return EFAULT;
      }

  return FIX_RET_VALUE (retval);
}

/*ARGSUSED*/
int
oss_read (dev_t dev, struct uio *uiop, cred_t * credp)
{
  int count = uiop->uio_resid;
  int retval;
  oss_cdev_t *cdev;

  dev = getminor (dev);
  if ((cdev = oss_cdevs[dev]) == NULL || cdev->d->read == NULL)
    return ENXIO;

  if (cdev->open_count == 0)	/* Not opened */
    {
      cmn_err (CE_NOTE, "Call to read on  minor %d that is not open\n", dev);
      return ENXIO;
    }

  cdev->file.acc_flags = uiop->uio_fmode;

  retval = cdev->d->read (cdev->instance, &cdev->file, uiop, count);

  return FIX_RET_VALUE (retval);
}

/*ARGSUSED*/
int
oss_write (dev_t dev, struct uio *uiop, cred_t * cred_p)
{
  int count = uiop->uio_resid;
  int retval;
  oss_cdev_t *cdev;

  dev = getminor (dev);
  if ((cdev = oss_cdevs[dev]) == NULL || cdev->d->write == NULL)
    return ENXIO;

  if (cdev->open_count == 0)	/* Not opened */
    {
      cmn_err (CE_NOTE, "Call to write on minor %d that is not open\n", dev);
      return ENXIO;
    }
  cdev->file.acc_flags = uiop->uio_fmode;

  retval = cdev->d->write (cdev->instance, &cdev->file, uiop, count);
  return FIX_RET_VALUE (retval);
}

int
oss_chpoll (dev_t dev, short events, int anyyet, short *reventsp,
	    struct pollhead **phpp)
{
  oss_cdev_t *cdev;
  oss_poll_event_t ev;
  int ret;

#ifdef DO_TIMINGS
  oss_timing_printf ("***** oss_chpoll(%x) ****", getminor (dev));
#endif

  if (getminor (dev) >= OSS_MAX_CDEVS)
    return ENXIO;

  dev = getminor (dev);
  if ((cdev = oss_cdevs[dev]) == NULL || cdev->d->chpoll == NULL)
    return ENXIO;

  if (cdev->open_count == 0)	/* Not opened */
    {
      cmn_err (CE_NOTE, "Call to chpoll on minor %d that is not open\n", dev);
      return ENXIO;
    }

  *reventsp = 0;
  ev.events = events;
  ev.anyyet = anyyet;
  ev.revents = 0;
  ev.php = NULL;

  ret = cdev->d->chpoll (cdev->instance, &cdev->file, &ev);
  if (ret < 0)
    {
      return -ret;
    }

  *reventsp |= ev.revents;
  if (ev.php != NULL)
    *phpp = ev.php;

  return 0;
}

#ifdef ALLOW_BUFFER_MAPPING
int
oss_devmap (dev_t dev, devmap_cookie_t handle, offset_t off, size_t len,
	    size_t * maplen, uint_t model)
{
  oss_cdev_t *cdev;
  oss_poll_event_t ev;
  int ret;

#ifdef DO_TIMINGS
  oss_timing_printf ("***** oss_devmap(%x) ****", getminor (dev));
#endif

  if (getminor (dev) >= OSS_MAX_CDEVS)
    {
      return ENXIO;
    }

  dev = getminor (dev);
  if ((cdev = oss_cdevs[dev]) == NULL || cdev->d == NULL)
    {
      return ENXIO;
    }

  if (cdev->open_count == 0)	/* Not opened */
    {
      cmn_err (CE_NOTE, "Call to devmap on minor %d that is not open\n", dev);
      return EPERM;
    }

  if (cdev->dev_class != OSS_DEV_DSP && cdev->dev_class != OSS_DEV_DSP_ENGINE)	/* Only audio devices can be mapped */
    {
      return ENXIO;
    }

  dev = cdev->instance;
  if (dev < 0 || dev >= num_audio_engines)
    return ENXIO;

  return ENOTSUP;
}
#endif

extern struct mod_ops mod_miscops;
static struct modldrv modldrv = {
  &mod_miscops,
  "Open Sound System " OSS_VERSION_STRING " framework"
    // &oss_ops,
};

static struct modlinkage modlinkage = {
  MODREV_1,			/* MODREV_1 indicated by manual */
  {(void *) &modldrv,
   NULL}			/* termination of list of linkage structures */
};

static ddi_device_acc_attr_t acc_attr_neverswap = {
  DDI_DEVICE_ATTR_V0,
  DDI_NEVERSWAP_ACC,
  DDI_STRICTORDER_ACC
};

#ifdef OSS_BIG_ENDIAN
static ddi_device_acc_attr_t acc_attr_le_swap = {
  DDI_DEVICE_ATTR_V0,
  DDI_STRUCTURE_LE_ACC,
  DDI_STRICTORDER_ACC
};
#endif


/*ARGSUSED*/
static ddi_device_acc_attr_t *
get_acc_attr (oss_device_t * osdev)
{
#ifdef OSS_BIG_ENDIAN
  if (osdev->swap_mode == 1)
    {
      return &acc_attr_le_swap;
    }
  else
#endif
    {
      return &acc_attr_neverswap;
    }
}

void
oss_load_options (oss_device_t * osdev, oss_option_map_t map[])
{
  int i, val;

  for (i = 0; map[i].name != NULL; i++)
    {
      if ((val =
	   ddi_prop_get_int (DDI_DEV_T_ANY, osdev->dip,
			     DDI_PROP_NOTPROM,
			     map[i].name, -12345)) != -12345)
	{
	  *map[i].ptr = val;
	}
    }
}

static int
oss_attach (dev_info_t * dip, ddi_attach_cmd_t cmd)
{
  oss_device_t *osdev;

  if (cmd != DDI_ATTACH)
    {
      cmn_err (CE_WARN, "oss_attach: Command %x\n", cmd);
      return (DDI_FAILURE);
    }

  if ((osdev = osdev_create (dip, DRV_VIRTUAL, 0, "osscore", NULL)) == NULL)
    {
      cmn_err (CE_WARN, "Creating osdev failed\n");
      return DDI_FAILURE;
    }

  MUTEX_INIT (osdev, osscore_mutex, 0);

  oss_load_options (osdev, oss_global_options);

  oss_common_init (osdev);

  ddi_report_dev (dip);
  oss_register_device (osdev, "OSS core services");

  return (DDI_SUCCESS);
}

/*
 * This is a pretty generic getinfo routine as describe in the manual.
 */

/*ARGSUSED*/
static int
oss_getinfo (dev_info_t * dip, ddi_info_cmd_t infocmd, void *arg,
	     void **result)
{
  dev_t dev;
  register int error;
  int instance;

  dev = (dev_t) arg;
  instance = getminor (dev) >> 4;

  switch (infocmd)
    {
    case DDI_INFO_DEVT2DEVINFO:
      *result = dip;
      error = DDI_SUCCESS;
      break;
    case DDI_INFO_DEVT2INSTANCE:
      *result = (void *) (uintptr_t) instance;
      error = DDI_SUCCESS;
      break;
    default:
      *result = NULL;
      error = DDI_FAILURE;
    }

#if 0
  DDB (cmn_err (CE_CONT,
		"oss_getinfo: returns %d, result=%x minor=%d instance=%d dev=%x\n",
		error, *result, minor_num, instance, dev));
#endif
  return (error);
}

/*
 * _init, _info, and _fini support loading and unloading the driver.
 */
int
_init (void)
{
  register int error = 0;
  error = mod_install (&modlinkage);
  return error;
}

int
_info (struct modinfo *modinfop)
{
  return (mod_info (&modlinkage, modinfop));
}

int
_fini (void)
{
  int status;

#ifdef MEMDEBUG
  int i;
#endif

  if ((status = mod_remove (&modlinkage)) != 0)
    return (status);

  free_all_contig_memory ();

#ifdef MEMDEBUG
  if (num_memblocks >= MAX_MEMBLOCKS)
    cmn_err (CE_NOTE, "All memory allocations were not checked\n");

  for (i = 0; i < num_memblocks; i++)
    {
      if (memblocks[i].addr != NULL)
	{
	  cmn_err (CE_NOTE, "Memory leak in %s:%d\n",
		   memblocks[i].file, memblocks[i].line);
	}
    }
#endif

  return status;
}

/*
 * When the osscore module is unloaded, oss_detach cleans up and frees the
 * resources we allocated in oss_attach.
 */
static int
oss_detach (dev_info_t * dip, ddi_detach_cmd_t cmd)
{
  static int already_unloaded = 0;

  if (cmd != DDI_DETACH)
    return DDI_FAILURE;

  if (already_unloaded)
    return DDI_SUCCESS;
  already_unloaded = 1;
//cmn_err(CE_CONT, "Oss detach\n");

  /* instance = ddi_get_instance (dip); */

  /*
   * Remove the minor nodes created in attach
   */
  ddi_remove_minor_node (dip, NULL);

  oss_unload_drivers ();

  MUTEX_CLEANUP (osscore_mutex);

  return (DDI_SUCCESS);
}

/*
 *	Some support routines
 */
void *
memset (void *t, int c, size_t l)
{
  int i;

  for (i = 0; i < l; i++)
    ((char *) t)[i] = c;

  return t;
}

#ifdef sparc
/*
 * I/O functions that do byte swapping (for Sparc)
 */
void
oss_put16 (ddi_acc_handle_t handle, unsigned short *addr, unsigned short val)
{
  val = ((val >> 8) & 0xff) | ((val & 0xff) << 8);
  ddi_put16 (handle, addr, val);
}

void
oss_put32 (ddi_acc_handle_t handle, unsigned int *addr, unsigned int val)
{
#ifdef OSS_BIG_ENDIAN
  val = ((val & 0x000000ff) << 24) |
    ((val & 0x0000ff00) << 8) |
    ((val & 0x00ff0000) >> 8) | ((val & 0xff000000) >> 24);
#endif
  ddi_put32 (handle, addr, val);
}

unsigned short
oss_get16 (ddi_acc_handle_t handle, unsigned short *addr)
{
  unsigned short val;

  val = ddi_get16 (handle, addr);
#ifdef OSS_BIG_ENDIAN
  val = ((val >> 8) & 0xff) | ((val & 0xff) << 8);
#endif
  return val;
}

unsigned int
oss_get32 (ddi_acc_handle_t handle, unsigned int *addr)
{
  unsigned int val;

  val = ddi_get32 (handle, addr);
#ifdef OSS_BIG_ENDIAN
  val = ((val & 0x000000ff) << 24) |
    ((val & 0x0000ff00) << 8) |
    ((val & 0x00ff0000) >> 8) | ((val & 0xff000000) >> 24);
#endif
  return val;
}

uint16_t
oss_mem_get16 (ddi_acc_handle_t handle, uint16_t * addr)
{
  uint16_t val;

  val = ddi_mem_get16 (handle, addr);
#ifdef OSS_BIG_ENDIAN
  val = ((val >> 8) & 0xff) | ((val & 0xff) << 8);
#endif
  return val;
}

uint32_t
oss_mem_get32 (ddi_acc_handle_t handle, uint32_t * addr)
{

  uint32_t val;

  val = ddi_mem_get32 (handle, addr);
#ifdef OSS_BIG_ENDIAN
  val = ((val & 0x000000ff) << 24) |
    ((val & 0x0000ff00) << 8) |
    ((val & 0x00ff0000) >> 8) | ((val & 0xff000000) >> 24);
#endif
  return val;
}

void
oss_mem_put16 (ddi_acc_handle_t handle, uint16_t * addr, uint16_t val)
{
#ifdef OSS_BIG_ENDIAN
  val = ((val >> 8) & 0xff) | ((val & 0xff) << 8);
#endif

  ddi_mem_put16 (handle, addr, val);
}

void
oss_mem_put32 (ddi_acc_handle_t handle, uint32_t * addr, uint32_t val)
{
#ifdef OSS_BIG_ENDIAN
  val = ((val & 0x000000ff) << 24) |
    ((val & 0x0000ff00) << 8) |
    ((val & 0x00ff0000) >> 8) | ((val & 0xff000000) >> 24);
#endif
  ddi_mem_put32 (handle, addr, val);
}
#endif

void
oss_pci_byteswap (oss_device_t * osdev, int mode)
{
  osdev->swap_mode = mode;
}

void
oss_pcie_init (oss_device_t * osdev, int flags)
{
	/* TODO: Should we do something? */
}

/*ARGSUSED*/
caddr_t
oss_map_pci_ioaddr (oss_device_t * osdev, int nr, int io)
{
  caddr_t addr;
  off_t region_size;
  int err;
  ddi_device_acc_attr_t *acc_attr;

  if (nr >= OSS_MAX_ACC_HANDLE)
  {
	  cmn_err(CE_WARN, "Too large I/O region number %d\n", nr);

	  return 0;
  }

  acc_attr = get_acc_attr (osdev);

  if ((err =
       ddi_dev_regsize (osdev->dip, nr + 1, &region_size)) != DDI_SUCCESS)
    {
      cmn_err (CE_WARN, "Getting device regsize failed (%d)\n", err);
      return 0;
    }

  if ((err = ddi_regs_map_setup
       (osdev->dip, nr + 1, &addr, 0, region_size, acc_attr,
	&osdev->acc_handle[nr])) != DDI_SUCCESS)
    {
      cmn_err (CE_WARN, "Register setup failed (%d)\n", err);
      return 0;
    }

  return addr;
}

void
oss_unmap_pci_ioaddr(oss_device_t * osdev, int nr)
{
  if (nr >= OSS_MAX_ACC_HANDLE)
  {
	  cmn_err(CE_WARN, "Too large I/O region number %d\n", nr);
	  return;
  }

  ddi_regs_map_free(&osdev->acc_handle[nr]);
}

int
pci_read_config_byte (oss_device_t * osdev, offset_t where,
		      unsigned char *val)
{
  if (osdev->dev_type != DRV_PCI)
    return PCIBIOS_FAILED;
#if defined (sparc)
  if (where == PCI_INTERRUPT_LINE)
    *val = 7;			/* PC emulation hack */
  else
#endif
    *val = pci_config_get8 (osdev->pci_config_handle, where);
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
  if (osdev->dev_type != DRV_PCI)
    return PCIBIOS_FAILED;
  *val = pci_config_get16 (osdev->pci_config_handle, where);
  return PCIBIOS_SUCCESSFUL;
}

int
pci_read_config_dword (oss_device_t * osdev, offset_t where,
		       unsigned int *val)
{
  if (osdev->dev_type != DRV_PCI)
    return PCIBIOS_FAILED;
  *val = pci_config_get32 (osdev->pci_config_handle, where);
  return PCIBIOS_SUCCESSFUL;
}

int
pci_write_config_byte (oss_device_t * osdev, offset_t where,
		       unsigned char val)
{
  if (osdev->dev_type != DRV_PCI)
    return PCIBIOS_FAILED;
  pci_config_put8 (osdev->pci_config_handle, where, val);
  return PCIBIOS_SUCCESSFUL;
}

int
pci_write_config_word (oss_device_t * osdev, offset_t where,
		       unsigned short val)
{
  if (osdev->dev_type != DRV_PCI)
    return PCIBIOS_FAILED;
  pci_config_put16 (osdev->pci_config_handle, where, val);
  return PCIBIOS_SUCCESSFUL;
}

int
pci_write_config_dword (oss_device_t * osdev, offset_t where,
			unsigned int val)
{
  if (osdev->dev_type != DRV_PCI)
    return PCIBIOS_FAILED;
  pci_config_put32 (osdev->pci_config_handle, where, val);
  return PCIBIOS_SUCCESSFUL;
}

void *
memcpy (void *t, const void *s, size_t l)
{
  bcopy (s, t, l);
  return t;
}

#ifdef MEMDEBUG
void *
oss_contig_malloc (oss_device_t * osdev, int size, oss_uint64_t memlimit,
		   oss_native_word * phaddr, oss_dma_handle_t *dma_handle, char *file, int line)
#else
void *
oss_contig_malloc (oss_device_t * osdev, int size, oss_uint64_t memlimit,
		   oss_native_word * phaddr, oss_dma_handle_t *dma_handle)
#endif
{
/*
 * Allocate physically contiguous memory (suitable for DMA).
 *
 * The memlimit parameter is equal to oss_alloc_dmabuf().
 */
  int err;
  size_t len;
  uint_t count;
  contig_desc *desc;
  ddi_dma_attr_t dma_attr;
  ddi_device_acc_attr_t *acc_attr;

  int flags =
    DDI_DMA_CONSISTENT | DDI_DMA_READ | DDI_DMA_WRITE;

  if (osdev == NULL)
    {
      cmn_err (CE_WARN, "oss_contig_malloc: osdev==NULL\n");
      return NULL;
    }

  acc_attr = get_acc_attr (osdev);

  memcpy (&dma_attr, &dma_attr_pci, sizeof (ddi_dma_attr_t));
  dma_attr.dma_attr_addr_hi = memlimit;

  desc = KERNEL_MALLOC (sizeof (contig_desc));
  if (desc == NULL)
    {
      cmn_err (CE_WARN, "Failed to allocate contig buffer descriptor\n");
      return NULL;
    }

  desc->osdev = osdev;
  desc->next = NULL;
  desc->is_special = 0;

  if ((err = ddi_dma_alloc_handle (osdev->dip,
				   &dma_attr,
				   DDI_DMA_SLEEP,
				   NULL, &desc->dhandle)) != DDI_SUCCESS)
    {
      cmn_err (CE_WARN, "Failed to allocate pci DMA handle (%d)\n", err);
      return NULL;
    }

  if (dma_handle != NULL)
     *dma_handle = desc->dhandle;

#ifndef IOMEM_DATA_UNCACHED
#define IOMEM_DATA_UNCACHED 0	// Fix for Solaris 10
#endif

  if ((err = ddi_dma_mem_alloc (desc->dhandle,
				size, /* + 4096,*/
				acc_attr,
				flags,
				DDI_DMA_SLEEP,
				NULL,
				(caddr_t *) & desc->first_addr,
				(size_t *) & len,
				&desc->dma_acc_handle)) != DDI_SUCCESS)
    {
      cmn_err (CE_WARN, "Failed to allocate %d bytes of contig memory (%d)\n",
	       size, err);
      return NULL;

    }

  desc->size = len;
  desc->orig_buf = desc->first_addr;

  if (desc->first_addr == NULL)
    {
      cmn_err (CE_WARN, "Can't allocate a contig buffer\n");
      return NULL;
    }

  DDB (cmn_err
       (CE_CONT, "Allocated contig memory, addr=%x, len=%d\n",
	(unsigned int) desc->first_addr, desc->size));

  if ((err = ddi_dma_addr_bind_handle (desc->dhandle,
				       NULL,
				       (char *) desc->first_addr,
				       desc->size,
				       flags | DDI_DMA_STREAMING,
				       DDI_DMA_DONTWAIT,
				       NULL,
				       &desc->cookie, &count)) != DDI_SUCCESS)
    {
      cmn_err (CE_WARN, "Contig address setup failed (%d)\n", err);
      return NULL;
    }

  desc->physaddr = desc->cookie.dmac_address;

  desc->last_addr = desc->first_addr + desc->size - 1;
  *phaddr = desc->physaddr;

  desc->next = contig_list;
  contig_list = desc;
/* HW_PRINTF(("Alloc contig: %x-%x, ph=%x\n", desc->first_addr, desc->last_addr, desc->physaddr)); */

  return desc->first_addr;
}

/*ARGSUSED*/
void
oss_contig_free (oss_device_t * osdev, void *p, int sz)
{
  int err;
  contig_desc *d, *desc = NULL, *prev = NULL;

  if (p == NULL)
    return;

  d = contig_list;

  while (d && desc == NULL)
    {
      if (d->is_special)
	{
	  prev = d;
	  d = d->next;
	  continue;
	}

      if (d->first_addr == p)
	{
	  desc = d;
	  break;
	}

      prev = d;
      d = d->next;
    }

  if (desc == NULL)
    {
      cmn_err (CE_WARN, "OSS: Can't free memory\n");
      return;
    }

  if ((err = ddi_dma_unbind_handle (desc->dhandle)) != DDI_SUCCESS)
    cmn_err (CE_WARN, "Failed to free DMA handle (%d)\n", err);

  if (desc->dma_acc_handle == NULL)
    cmn_err (CE_WARN, "desc->dma_acc_handle==NULL\n");
  else
    ddi_dma_mem_free (&desc->dma_acc_handle);

  ddi_dma_free_handle (&desc->dhandle);


  if (desc == contig_list)
    contig_list = desc->next;
  else
    {
      prev->next = desc->next;
    }
  KERNEL_FREE (desc);

}

static void
free_all_contig_memory (void)
{
  contig_desc *p, *desc = contig_list;

  while (desc != NULL)
    {
      p = desc;
      desc = desc->next;

      if (p->is_special)
	continue;

      oss_contig_free (p->osdev, p->orig_buf, 0);
    }

  contig_list = NULL;
}

#if !defined(SOL9) && !defined(DISABLE_FMA)
/*ARGSUSED*/
static int
oss_fm_error_cb(dev_info_t *dip, ddi_fm_error_t *err, const void *osdev_)
{
	pci_ereport_post(dip, err, NULL);
	return err->fme_status;
}
#endif

oss_device_t *
osdev_create (dev_info_t * dip, int dev_type, int instance, const char *nick,
	      const char *handle)
{
  oss_device_t *osdev = NULL;
  int i;
  ddi_iblock_cookie_t iblk;
  static int license_checked=0;

#ifdef LICENSED_VERSION
  if (!license_checked)
     {
	timestruc_t ts;
	
	license_checked = 1;
	gethrestime (&ts);
	if (!oss_license_handle_time (ts.tv_sec))
	  {
	    cmn_err (CE_WARN, "This version of Open Sound System has expired\n");
	    cmn_err (CE_CONT,
	      "Please download the latest version from www.opensound.com\n");
	    oss_expired = 1;
	  }
     }
#endif

  if (handle == NULL)
    handle = nick;

  /*
   * Don't accept any more drivers if expired
   */
  if (oss_expired && oss_num_cards > 0)
    return NULL;

  /*
   * Check if the same device is being reinserted.
   */
  for (i = 0; i < oss_num_cards; i++)
    {
      if (cards[i]->available)	/* Not deleted */
	continue;

      if (cards[i]->dip == dip)
	{
	  osdev = cards[i];
	  break;
	}
    }

#if 1
  if (osdev == NULL)
     {
	  /*
	   * Check if there are some deleted devices.
	   */
	  for (i = 0; i < oss_num_cards; i++)
	    {
	      if (cards[i]->available) /* Not deleted */
		continue;
	
	      osdev = cards[i];
	      break;
	    }
      }
#endif

  if (osdev == NULL)
    {
	    for (i=0;i<oss_num_cards;i++)
      	    if (!cards[i]->available)
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
      memset (osdev, 0, sizeof (*osdev));

      osdev->cardnum = oss_num_cards;
      cards[oss_num_cards++] = osdev;
    }

  osdev->dip = dip;
  osdev->osid = dip;
  osdev->available = 1;
  osdev->instance = instance;
  osdev->dev_type = dev_type;
  osdev->devc = NULL;
  sprintf (osdev->nick, "%s%d", nick, instance);
  strcpy (osdev->modname, nick);
  osdev->num_audio_engines = 0;
  osdev->num_audioplay = 0;
  osdev->num_audiorec = 0;
  osdev->num_audioduplex = 0;
  osdev->num_mididevs = 0;
  osdev->num_mixerdevs = 0;
  osdev->first_mixer = -1;

  switch (dev_type)
    {
    case DRV_PCI:
      if (pci_config_setup (dip, &osdev->pci_config_handle) != DDI_SUCCESS)
	cmn_err (CE_NOTE, "pci_config_setup() failed\n");
#if !defined(SOL9) && !defined(DISABLE_FMA)
      osdev->fm_capabilities =	DDI_FM_EREPORT_CAPABLE | DDI_FM_DMACHK_CAPABLE |
	      			DDI_FM_ERRCB_CAPABLE;
      ddi_fm_init(dip, &osdev->fm_capabilities, &iblk);
      ddi_fm_handler_register(dip, oss_fm_error_cb, (void*)osdev);
      pci_ereport_setup(dip);
#endif
      break;

    case DRV_VIRTUAL:
    case DRV_VMIX:
    case DRV_STREAMS:
#if !defined(SOL9) && !defined(DISABLE_FMA)
      osdev->fm_capabilities=DDI_FM_EREPORT_CAPABLE;
      ddi_fm_init(dip, &osdev->fm_capabilities, &iblk);
#endif
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

  if (!osdev->available) /* Already deleted */
     return;

  osdev->available = 0;

  switch (osdev->dev_type)
    {
    case DRV_PCI:
#if !defined(SOL9) && !defined(DISABLE_FMA)
      ddi_fm_handler_unregister(osdev->dip);
      pci_ereport_teardown(osdev->dip);
#endif
      pci_config_teardown (&osdev->pci_config_handle);
#if !defined(SOL9) && !defined(DISABLE_FMA)
      ddi_fm_fini(osdev->dip);
#endif
      osdev->pci_config_handle = NULL;
      break;

    case DRV_VIRTUAL:
    case DRV_VMIX:
    case DRV_STREAMS:
#if !defined(SOL9) && !defined(DISABLE_FMA)
      ddi_fm_fini(osdev->dip);
#endif
      break;
    }

  ddi_remove_minor_node (osdev->dip, NULL);

/*
 * Mark all minor nodes for this module as invalid.
 */
  for (i = 0; i < oss_num_cdevs; i++)
    if (oss_cdevs[i] != NULL)
    if (oss_cdevs[i]->osdev == osdev)
      {
	unlink_cdev_hash(oss_cdevs[i]);
	oss_cdevs[i]->d = NULL;
	oss_cdevs[i]->osdev = NULL;
	oss_cdevs[i]->active = 0;	/* Device removed */
      }
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
  ci->intr_count = cards[cardnum]->intrcount;
  ci->ack_count = cards[cardnum]->ackcount;

  return 0;
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

/*ARGSUSED*/
void
oss_install_chrdev (oss_device_t * osdev, char *name, int dev_class,
		    int instance, oss_cdev_drv_t * drv, int flags)
{
/*
 * oss_install_chrdev creates a character device (minor). However if
 * name==NULL the device will not be exported (made visible to userland
 * clients).
 */

  int num;
  int hash_link;
  oss_cdev_t *cdev = NULL;

  if (dev_class != OSS_DEV_STATUS)
    if (oss_expired && instance > 0)
      return;
/*
 * Find if this dev_class&instance already exists (after previous module
 * detach).
 */

  if (flags & CHDEV_REPLUG)
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
  cdev->active = 1;
  cdev->osdev = osdev;
  if (name != NULL)
    strncpy (cdev->name, name, sizeof (cdev->name) - 1);
  else
    strcpy (cdev->name, "NONE");
  cdev->name[sizeof (cdev->name) - 1] = 0;
  oss_cdevs[num] = cdev;

  cdev->minor = num;

  /*
   * Add to the cdev_hash list.
   */
  hash_link = compute_cdev_hash (dev_class, instance);
  cdev->hl = cdev_hash[hash_link];
  cdev_hash[hash_link] = cdev;

/*
 * Export the device only if name != NULL
 */
  if (name != NULL)
    {
      char tmp[64], *s;
      char *dev_type = "oss_sysdev";

/*
 * Convert "oss/device/node" style names to the
 * "device,node" style naming required by Solaris.
 */
      if (name[0] == 'o' && name[3] == '/')	/* oss/ prefix */
	{
	  strcpy (tmp, name + 4);
	  name = tmp;

	  s = tmp;
	  while (*s)
	    {
	      if (*s == '/')
		*s = ',';
	      s++;
	    }
	  dev_type = "oss_audio";
	}

      if (ddi_create_minor_node (osdev->dip, name, S_IFCHR, num,
				 dev_type, 0) == DDI_FAILURE)
	{
	  cmn_err (CE_WARN, "ddi_create_minor_node failed\n");
	}
    }
}

int
oss_find_minor (int dev_class, int instance)
{
  oss_cdev_t *cdev;

  cdev = cdev_hash[compute_cdev_hash(dev_class, instance)];

  while (cdev != NULL)
  {
	   if (cdev->d != NULL && cdev->dev_class == dev_class
			           && cdev->instance == instance)
		   return cdev->minor;
	   cdev = cdev->hl; /* Next in the hash chain */
  }

  return OSS_EIO;
}

int
__oss_alloc_dmabuf (int dev, dmap_p dmap, unsigned int alloc_flags,
		    oss_uint64_t maxaddr, int direction)
{
  int err;
  size_t len;
  uint_t ncookies;
  contig_desc *desc;
  oss_device_t *osdev = dmap->osdev;
  ddi_dma_attr_t dma_attr;
  int size = 32 * 1024;
  extern int dma_buffsize;
  ddi_device_acc_attr_t *acc_attr;

  int flags = DDI_DMA_CONSISTENT |
    (direction == OPEN_READ) ? DDI_DMA_READ : DDI_DMA_WRITE;

  if (osdev == NULL)
    {
      cmn_err (CE_WARN, "oss_alloc_dmabuf: osdev==NULL\n");
      return OSS_EIO;
    }

  acc_attr = get_acc_attr (osdev);

  if (dma_buffsize > 16 && dma_buffsize <= 128)
    size = dma_buffsize * 1024;

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

  if (alloc_flags & DMABUF_LARGE)
    size = 256 * 1024;

  if ((alloc_flags & DMABUF_SIZE_16BITS) && size > 32 * 1024)
    size = 32 * 1024;

  memcpy (&dma_attr, &dma_attr_pci, sizeof (ddi_dma_attr_t));
  dma_attr.dma_attr_addr_hi = maxaddr;

#ifndef SOL9
  if (osdev->dev_type == DRV_PCI)
     dma_attr.dma_attr_flags |= DDI_DMA_FLAGERR;
#endif

  if (dmap->dmabuf != NULL)
    return 0;			/* Already done */

  dmap->dma_parms.state = 0;
  dmap->dma_parms.enabled = 1;
  dmap->dma_parms.ignore = 0;
  
  if (osdev->dip == NULL)
    {
      cmn_err (CE_WARN, "oss_alloc_dmabuf: osdev->dip==NULL\n");
      return OSS_EIO;
    }

  if (dmap == NULL)
    {
      cmn_err (CE_WARN, "oss_alloc_dmabuf: dmap==NULL\n");
      return OSS_EIO;
    }

  if ((err = ddi_dma_alloc_handle (osdev->dip,
				   &dma_attr,
				   DDI_DMA_SLEEP,
				   NULL,
				   &dmap->dmabuf_dma_handle)) != DDI_SUCCESS)
    {
      cmn_err (CE_WARN, "Failed to allocate DMA handle (error %d)\n", err);
      return OSS_ENOMEM;
    }


  dmap->dmabuf = NULL;
  dmap->buffsize = size;

  err = -1;

  while (err != DDI_SUCCESS && dmap->dmabuf == NULL && dmap->buffsize >= 4096)
    {
      if ((err = ddi_dma_mem_alloc (dmap->dmabuf_dma_handle,
				    dmap->buffsize,
				    acc_attr,
				    flags,
				    DDI_DMA_SLEEP,
				    NULL,
				    (caddr_t *) & dmap->dmabuf,
				    (size_t *) & len,
				    &dmap->dma_parms.dma_acc_handle)) !=
	  DDI_SUCCESS)
	{
	  if (!(alloc_flags & DMABUF_QUIET))
	    DDB (cmn_err (CE_WARN,
			  "failed to allocate %d bytes of DMA memory (%d)\n",
			  dmap->buffsize, err));
	  dmap->dmabuf = NULL;
	  dmap->buffsize /= 2;
	}
    }

  if (dmap->dmabuf == NULL)
    {
      cmn_err (CE_WARN, "Can't allocate a DMA buffer for device %d\n", dev);
      ddi_dma_free_handle (&dmap->dmabuf_dma_handle);
      return OSS_ENOMEM;
    }

  DDB (cmn_err (CE_CONT, "Allocated DMA memory, addr=%x, len=%d\n",
		(int) dmap->dmabuf, (int) dmap->buffsize));

  dmap->dma_parms.orig_buf = (caddr_t) dmap->dmabuf;

  if ((err = ddi_dma_addr_bind_handle (dmap->dmabuf_dma_handle,
				       NULL,
				       (char *) dmap->dmabuf,
				       dmap->buffsize,
				       flags | DDI_DMA_STREAMING,
				       DDI_DMA_DONTWAIT,
				       NULL,
				       &dmap->dma_parms.cookie,
				       &ncookies)) != DDI_SUCCESS)
    {
      cmn_err (CE_WARN, "DMA address setup failed (%d)\n", err);

      return OSS_EIO;
    }
  dmap->dmabuf_phys = dmap->dma_parms.cookie.dmac_address;

  desc = PMALLOC (osdev, sizeof (contig_desc));
  if (desc == NULL)
    return OSS_ENOMEM;

  desc->osdev = osdev;
  desc->next = NULL;
  desc->is_special = 1;
  desc->first_addr = dmap->dmabuf;
  desc->last_addr = dmap->dmabuf + dmap->buffsize - 1;
  desc->physaddr = dmap->dmabuf_phys;
  desc->orig_buf = dmap->dma_parms.orig_buf;

  if (contig_list != NULL)
    desc->next = contig_list;
/* HW_PRINTF(("Alloc DMA: %x-%x, ph=%x\n", desc->first_addr, desc->last_addr, desc->physaddr)); */
  contig_list = desc;

  return 0;
}

/*ARGSUSED*/
void
oss_free_dmabuf (int dev, dmap_p dmap)
{
  int err;

  if (dmap->dmabuf == NULL)
    return;

  if ((err = ddi_dma_unbind_handle (dmap->dmabuf_dma_handle)) != DDI_SUCCESS)
    cmn_err (CE_WARN, "Failed to free DMA handle (%d)\n", err);
  ddi_dma_mem_free (&dmap->dma_parms.dma_acc_handle);
  ddi_dma_free_handle (&dmap->dmabuf_dma_handle);

  dmap->dmabuf = NULL;
  dmap->dmabuf_phys = 0;
}

/*
 * Interrupt management
 */
static u_int
oss_intr (caddr_t arg)		/* Global interrupt handler */
{
  oss_device_t *osdev = (oss_device_t *) arg;
  int serviced;
#ifdef MUTEX_CHECKS
  int x = inside_intr;
  inside_intr = 1;		/* For mutex debugging only */
#endif

  if (osdev == NULL)
    {
#ifdef MUTEX_CHECKS
      inside_intr = x;
#endif
      return DDI_INTR_UNCLAIMED;
    }
  osdev->intrcount++;

  UP_STATUS (0x01);
  serviced = osdev->tophalf_handler (osdev);
  DOWN_STATUS (0x01);

  if (serviced == 0)
    return DDI_INTR_UNCLAIMED;
  osdev->ackcount++;

  if (osdev->bottomhalf_handler == NULL)
    return DDI_INTR_CLAIMED;

   osdev->bottomhalf_handler (osdev);

#ifdef MUTEX_CHECKS
  inside_intr = x;
#endif
  return DDI_INTR_CLAIMED;
}

int
oss_register_interrupts (oss_device_t * osdev, int intrnum,
			 oss_tophalf_handler_t top,
			 oss_bottomhalf_handler_t bottom)
{
  int err;
  ddi_idevice_cookie_t ic;


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
  osdev->intr_is_hilevel = ddi_intr_hilevel (osdev->dip, intrnum);
  if (osdev->intr_is_hilevel)
    {
      if (bottom == NULL)
	{
	  cmn_err (CE_WARN,
		   "The driver for %s doesn't support hilevel interrupts\n",
		   osdev->name);
	  return OSS_EINVAL;
	}

      DDB (cmn_err (CE_NOTE, "Using hilevel intr for %s\n", osdev->name));

      /* TODO: Fix hilevel intr handling */
      cmn_err (CE_WARN, "Hilevel interrupts are not supported yet.\n");
      return OSS_EINVAL;
    }

  ddi_get_iblock_cookie (osdev->dip, intrnum, &osdev->iblock_cookie);

  if ((err = ddi_add_intr (osdev->dip, intrnum, NULL, &ic,
			   oss_intr, (caddr_t) osdev)) != DDI_SUCCESS)
    {
      cmn_err (CE_WARN, "ddi_add_intr() failed, error=%d\n", err);
      return OSS_EIO;
    }

  return 0;
}

void
oss_unregister_interrupts (oss_device_t * osdev)
{
  ddi_remove_intr (osdev->dip, 0, osdev->iblock_cookie);
  osdev->tophalf_handler = NULL;
  osdev->bottomhalf_handler = NULL;
}

/*ARGSUSED*/
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
  if (osdev->refcount > 0 || open_devices > 0)
    {
      cmn_err (CE_CONT, "Refcount %d (%s) , open_devices %d\n",
	       osdev->refcount, osdev->nick, open_devices);
      if (open_devices > 0)
	{
	  int i;

	  for (i = 0; i < oss_num_cdevs; i++)
	    if (oss_cdevs[i]->open_count)
	      {
		cmn_err (CE_CONT, "%s is opened\n", oss_cdevs[i]->name);
	      }
	}
      return OSS_EBUSY;
    }
//cmn_err(CE_CONT, "oss_disable_device %s\n", osdev->nick);

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

/*ARGSUSED*/
void
oss_unregister_device (oss_device_t * osdev)
{
/*
 * Notice! The driver calling this routine (the owner of the osdev parameter)
 * has already uninitialized itself. Do not do any actions that may call this
 * driver directly or indirectly.
 */

/*
 * Force reload of all drivers if any application tries to open any
 * of the devices.
 */
  do_forceload = 1;
}

void
oss_reserve_device (oss_device_t * osdev)
{
  osdev->refcount++;
}

void
oss_unreserve_device (oss_device_t * osdev, int decrement)
{
  osdev->refcount -= decrement;
}

/*
 * Wait queue support
 */
 /*ARGSUSED*/
  oss_wait_queue_t *
oss_create_wait_queue (oss_device_t * osdev, const char *name)
{
  oss_wait_queue_t *wq;

  if ((wq = PMALLOC (NULL, sizeof (*wq))) == NULL)
    return NULL;

  cv_init (&wq->cv, NULL, CV_DRIVER, NULL);

  return wq;
}

void *
oss_get_osid (oss_device_t * osdev)
{
  return osdev->osid;
}

/*ARGSUSED*/
void
oss_reset_wait_queue (oss_wait_queue_t * wq)
{
  /* NOP */
}

void
oss_remove_wait_queue (oss_wait_queue_t * wq)
{
  cv_destroy (&wq->cv);
}

/*ARGSUSED*/
int
oss_sleep (oss_wait_queue_t * wq, oss_mutex_t * mutex, int ticks,
	   oss_native_word * flags, unsigned int *status)
{
/*
 * oss_sleep will return 0 if timeout occurred and 1 otherwise. The WK_SIGNAL bit will be reported on
 * status if a signal was caught.
 */
  *status = 0;

  if (ticks > 0)
    {
      int res;

      if ((res =
	   cv_timedwait_sig (&wq->cv, mutex, ddi_get_lbolt () + ticks)) == 0)
	*status |= WK_SIGNAL;	/* Got signal */
      if (res < 0)		/* Timeout */
	return 0;
    }
  else
    {
      if (cv_wait_sig (&wq->cv, mutex) == 0)
	*status |= WK_SIGNAL;	/* Got signal */
    }
  return 1;
}

/*ARGSUSED*/
int
oss_register_poll (oss_wait_queue_t * wq, oss_mutex_t * mutex,
		   oss_native_word * flags, oss_poll_event_t * ev)
{
  ev->php = &wq->ph;
  wq->pollevents |= ev->events;
  return 0;
}

/*ARGSUSED*/
void
oss_wakeup (oss_wait_queue_t * wq, oss_mutex_t * mutex,
	    oss_native_word * flags, short events)
{
  cv_broadcast (&wq->cv);

  if (wq->pollevents & events)
    {
      wq->pollevents &= ~events;
      MUTEX_EXIT_IRQRESTORE (*mutex, *flags);
      pollwakeup (&wq->ph, events);
      MUTEX_ENTER_IRQDISABLE (*mutex, *flags);
    }
}

#ifdef MUTEX_CHECKS
void
debug_mutex_init (oss_mutex_t * mutex, void *dummy, int typ, void *arg,
		  char *file, int line)
{
  memset (mutex, 0, sizeof (mutex));
  mutex_init (&mutex->mu, dummy, typ, arg);
}

void
debug_mutex_destroy (oss_mutex_t * mutex, char *file, int line)
{
  mutex_destroy (&mutex->mu);
}

void
debug_mutex_enter (oss_mutex_t * mutex, char *file, int line)
{
  if (mutex_owned (&mutex->mu))
    {
      cmn_err (CE_NOTE, "%s:%d: Re-entrant mutex (%s:%d %d)\n", file, line,
	       mutex->file, mutex->line, mutex->busy_flags);
      return;
    }

  mutex->file = file;
  mutex->line = line;
  mutex_enter (&mutex->mu);
}

void
debug_mutex_exit (oss_mutex_t * mutex, char *file, int line)
{
  if (!mutex_owned (&mutex->mu))
    {
      cmn_err (CE_NOTE, "Mutex not owned %s:%d\n", file, line);
    }
  else
    mutex_exit (&mutex->mu);

  mutex->file = NULL;
  mutex->line = 0;
}
#endif

int
oss_get_procinfo(int what)
{

	switch (what)
	{
	case OSS_GET_PROCINFO_UID:
		return ddi_get_cred()->cr_uid;
		break;

	case OSS_GET_PROCINFO_GID:
		return ddi_get_cred()->cr_gid;
		break;

	}
	return OSS_EINVAL;
}
