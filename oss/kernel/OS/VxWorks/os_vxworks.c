/*
 * oss_vxworks.c: Common entry points for OSS under VxWorks.
 */

#include <oss_config.h>
#include <oss_pci.h>
#include <drv/pci/pciConfigLib.h>
#include <memLib.h>

#if 0
// TODO: Obsolete
typedef struct oss_device_handle
{
  DEV_HDR devHdr;
  int minor;
  int valid;			/* 1=valid, 0=undefined */
  struct fileinfo finfo;
}
oss_device_handle;
#endif
/*
 * Number of cards supported in the same system.
 */
#define MAX_CARDS 8

static oss_device_t *cards[MAX_CARDS];
int oss_num_cards = 0;

static int oss_driver_num = ERROR;
static int oss_expired = 0;
static oss_device_t *core_osdev = NULL;

int oss_hz = 100;

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
    a[i] = ( (sizeof(char *) == 32) ? ( *((char * **)(ap += ((sizeof(char * *)+sizeof(int)-1) & ~(sizeof(int)-1))))[-1] ) : ( ((char * *)(ap += ((sizeof(char *)+sizeof(int)-1) & ~(sizeof(int)-1))))[-1] ));
    //a[i] = va_arg (ap, char *); // This was supposed to be used instead of above. Unfortunately va_arg() seems to be buggy

  for (i = n; i < 6; i++)
    a[i] = NULL;

  if (level == CE_CONT)
    {
      sprintf (tmp, s, a[0], a[1], a[2], a[3], a[4], a[5], NULL,
	       NULL, NULL, NULL);
      printf ("%s", tmp);
    }
  else
    {
      strcpy (tmp, "osscore: ");
      sprintf (tmp + strlen (tmp), s, a[0], a[1], a[2], a[3], a[4], a[5],
	       NULL, NULL, NULL, NULL);

      printf ("%s", tmp);
    }

  va_end (ap);
}

int
oss_uiomove (void *addr, size_t nbytes, enum uio_rw rwflag, uio_t * uio)
{
/*
 * NOTE! Returns 0 upon success and EFAULT on failure (instead of -EFAULT
 * (for Solaris/BSD compatibility)).
 */

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
      memcpy(uio->ptr, addr, nbytes);
      break;

    case UIO_WRITE:
      memcpy(addr, uio->ptr, nbytes);
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

static int
grow_array(oss_device_t *osdev, oss_cdev_t ***arr, int *size, int increment)
{
	oss_cdev_t **old=*arr, **new = *arr;
	int old_size = *size;
	int new_size = *size;
		
	new_size += increment;

	if ((new=PMALLOC(osdev, new_size * sizeof (oss_cdev_t *)))==NULL)
	   return 0;

	memset(new, 0, new_size * sizeof(oss_cdev_t *));
	if (old != NULL)
	   memcpy(new, old, old_size * sizeof(oss_cdev_t *));

	*size = new_size;
	*arr = new;

	if (old != NULL)
	   PMFREE(osdev, old);

	return 1;
}

static void
register_chrdev(oss_cdev_t *cdev, char *name)
{
    if (iosDevAdd ((void *)cdev, name, oss_driver_num) == ERROR)
      {
	cmn_err (CE_WARN, "Failed to add device %s\n", name);
      }
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

  int num;
  oss_cdev_t *cdev = NULL;

  if (dev_class != OSS_DEV_STATUS)
    if (oss_expired && instance > 0)
      return;

      if (oss_num_cdevs >= OSS_MAX_CDEVS)
	{
	   if (!grow_array(osdev, &oss_cdevs, &oss_max_cdevs, 100))
	   {
	  	cmn_err (CE_WARN, "Out of minor numbers.\n");
	  	return;
	   }
	}

      if ((cdev = PMALLOC (NULL, sizeof (*cdev))) == NULL)
	{
	  cmn_err (CE_WARN, "Cannot allocate character device desc.\n");
	  return;
	}

      num = oss_num_cdevs++;

  memset (cdev, 0, sizeof (*cdev));
  cdev->dev_class = dev_class;
  cdev->instance = instance;
  cdev->d = drv;
  cdev->osdev = osdev;
  if (name != NULL)
    strncpy (cdev->name, name, sizeof (cdev->name));
  else
    strcpy (cdev->name, "NONE");
  cdev->name[sizeof (cdev->name) - 1] = 0;
  oss_cdevs[num] = cdev;

/*
 * Export the device only if name != NULL
 */
  if (name != NULL)
    {
      strcpy (cdev->name, name);
      register_chrdev (cdev, name);
    }
}

int
oss_find_minor (int dev_class, int instance)
{
  int i;

  for (i = 0; i < oss_num_cdevs; i++)
    {
      if (oss_cdevs[i]->d != NULL && oss_cdevs[i]->dev_class == dev_class
	  && oss_cdevs[i]->instance == instance)
	return i;
    }

  return OSS_ENXIO;
}

static inline int
cpy_file (int mode, struct fileinfo *fi)
{
  fi->mode = 0;
  fi->acc_flags = mode;

  if ((fi->acc_flags & O_ACCMODE) == O_RDWR)
    fi->mode = OPEN_READWRITE;
  if ((fi->acc_flags & O_ACCMODE) == O_RDONLY)
    fi->mode = OPEN_READ;
  if ((fi->acc_flags & O_ACCMODE) == O_WRONLY)
    fi->mode = OPEN_WRITE;

  return fi->mode;
}

static void *
ossOpen (oss_cdev_t *cdev, char *reminder, int mode)
{
  int tmpdev, retval;
  struct fileinfo fi;

  cpy_file (mode, &fi);

  DDB (cmn_err
       (CE_CONT, "ossOpen(%p): %s, class=%d, instance=%d\n", cdev,
	cdev->name, cdev->dev_class, cdev->instance));

  if (cdev->d->open == NULL)
    {
      errnoSet(ENODEV);
      return (void*)ERROR;
    }

  tmpdev = -1;
  retval =
    cdev->d->open (cdev->instance, cdev->dev_class, &fi, 0, 0, &tmpdev);

  if (retval < 0)
    {
      errnoSet(-retval);
      return (void*)ERROR;
    }

  if (tmpdev != -1)
  {
     if (tmpdev >= 0 && tmpdev < oss_num_cdevs)
        {
	     cdev = oss_cdevs[tmpdev];
        }
     else
        {
      		errnoSet(ENODEV);
      		return (void*)ERROR;
	}
  }

  errnoSet (0);
  memcpy(&cdev->file, &fi, sizeof(struct fileinfo));

  return cdev;
}

static int
ossClose (oss_cdev_t *cdev)
{
  if (cdev->d->close == NULL)
    {
      return OK;
    }

  cdev->d->close (cdev->instance, &cdev->file);

  return OK;
}

static int
ossRead (oss_cdev_t *cdev, char *buf, int count)
{
  int err, len;
  uio_t uio;

  if (cdev->d->read == NULL)
    {
      errnoSet (ENXIO);
      return ERROR;
    }

  if ((err = oss_create_uio (&uio, buf, count, UIO_READ, 0)) < 0)
    {
      errnoSet (-err);
      return ERROR;
    }

  len = cdev->d->read (cdev->instance, &cdev->file, &uio, count);

  if (len >= 0)
     return len;

  errnoSet (-len);
  return ERROR;
}

static int
ossWrite (oss_cdev_t *cdev, char *buf, int count)
{
  int err, len;
  uio_t uio;

  if (cdev->d->write == NULL)
    {
      errnoSet (ENXIO);
      return ERROR;
    }

  if ((err = oss_create_uio (&uio, buf, count, UIO_WRITE, 0)) < 0)
    {
      errnoSet (-err);
      return ERROR;
    }

  len = cdev->d->write (cdev->instance, &cdev->file, &uio, count);

  if (len >= 0)
     return len;

  errnoSet (-len);
  return ERROR;
}

static int
ossIoctl (oss_cdev_t *cdev, int cmd, int *arg)
{
  int err;

  if (cdev->d->ioctl == NULL)
  {
      errnoSet (ENXIO);
      return ERROR;
  }

  if ((err = cdev->d->ioctl (cdev->instance, &cdev->file, cmd, (ioctl_arg) arg)) < 0)
    {
      errnoSet (-err);
      return ERROR;
    }
  return OK;
}

oss_device_t *
osdev_create (dev_info_t * dip, int dev_type,
	      int instance, const char *nick, const char *handle)
{
  oss_device_t *osdev;

  osdev = PMALLOC (NULL, sizeof (*osdev));
  if (osdev == NULL)
    {
      cmn_err (CE_WARN, "osdev_create: Out of memory\n");
      return NULL;
    }

  memset (osdev, 0, sizeof (*osdev));

  sprintf (osdev->nick, "%s%d", nick, instance);
  osdev->instance = instance;
  osdev->dip = dip;
  osdev->available = 1;
  osdev->first_mixer = -1;

  strcpy (osdev->modname, nick);

  if (handle == NULL)
    handle = nick;

  if (oss_num_cards >= MAX_CARDS)
    cmn_err (CE_WARN, "Too many OSS devices. At most %d permitted.\n",
	     MAX_CARDS);
  else
    {
      osdev->cardnum = oss_num_cards;
      cards[oss_num_cards++] = osdev;
    }
/*
 * Create the device handle
 */
  switch (dev_type)
    {
    case DRV_PCI:
      {
#if 0
	// TODO
	unsigned int subvendor;
	char *devpath;
	devpath = oss_pci_read_devpath (osdev->dip);
	oss_pci_read_config_dword (osdev, 0x2c, &subvendor);

	sprintf (osdev->handle, "PCI%08x-%s", subvendor, devpath);
#else
	strcpy(osdev->handle, "PCICARD");
#endif
      }
      break;

    case DRV_USB:
      // TODO: Get the vendor information
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

  osdev->available = 0;
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
}

void *
oss_get_osid (oss_device_t * osdev)
{
  return NULL;			// TODO
}

int
oss_register_device (oss_device_t * osdev, const char *name)
{
  if (name == NULL)
    {
      cmn_err (CE_WARN, "oss_register_device: name==NULL\n");
      osdev->name = "Undefined name";
      return 0;
    }

  if ((osdev->name = PMALLOC (NULL, strlen (name) + 1)) == NULL)
    {
      cmn_err (CE_WARN, "Cannot allocate memory for device name\n");
      osdev->name = "Unknown device";
    }
  strcpy (osdev->name, name);
  return 0;
}

void
oss_unregister_device (oss_device_t * osdev)
{
/*
 * Notice! The driver calling this routine (the owner of the osdev parameter)
 * has already uninitialized itself. Do not do any actions that may call this
 * driver directly or indirectly.
 */

// TODO: Move this to some common OSS module (also under Solaris)
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
  ci->longname[127] = 0;

  if (cards[cardnum]->nick != NULL)
    strncpy (ci->shortname, cards[cardnum]->nick, 16);
  ci->shortname[15] = 0;

  if (cards[cardnum]->hw_info != NULL)
    strncpy (ci->hw_info, cards[cardnum]->hw_info, sizeof (ci->hw_info));
  ci->hw_info[sizeof (ci->hw_info) - 1] = 0;
  ci->intr_count = cards[cardnum]->intrcount;
  ci->ack_count = cards[cardnum]->ackcount;

  return 0;
}

int
ossDrv (void)
{
  oss_hz = sysClkRateGet();

  if (oss_driver_num != ERROR)
    {
      cmn_err (CE_WARN, "OSS is already running\n");
      return -1;
    }

#ifdef LICENSED_VERSION
  if (!oss_license_handle_time (oss_get_time ()))
    {
      cmn_err (CE_WARN, "This version of Open Sound System has expired\n");
      cmn_err (CE_CONT,
	       "Please download the latest version from www.opensound.com\n");
      oss_expired = 1;
      return -1;
    }
#endif

  oss_driver_num = iosDrvInstall ((FUNCPTR) NULL,	/* create */
				  (FUNCPTR) NULL,	/* delete */
				  (FUNCPTR) ossOpen, (FUNCPTR) ossClose, (FUNCPTR) ossRead, (FUNCPTR) ossWrite, (FUNCPTR) ossIoctl	/* ioctl */
    );
  if (oss_driver_num == ERROR)
    {
      cmn_err (CE_WARN, "Module osscore failed to install\n");
      return -1;
    }

  if ((core_osdev =
       osdev_create (NULL, DRV_UNKNOWN, 0, "osscore", NULL)) == NULL)
    {
      oss_cmn_err (CE_WARN, "Failed to allocate OSDEV structure\n");
      return -1;
    }
  oss_register_device (core_osdev, "OSS core services");

  oss_common_init (core_osdev);

  return oss_driver_num;
}

int
ossDrvRemove (void)
{
#if 1

  return ERROR;
#else
  int i;

  if (oss_driver_num == ERROR)
    return 0;

  for (i = 0; i < SND_NDEVS; i++)
    if (oss_files[i].valid)
      {
	iosDevDelete (&oss_files[i].devHdr);
	oss_files[i].valid = 0;
      }

  if (iosDrvRemove (oss_driver_num, FALSE) == ERROR)
    {
      cmn_err (CE_WARN, "Driver busy - cannot remove.\n");
      return ERROR;
    }

  // TODO
  oss_unload_drivers ();

  oss_driver_num = ERROR;	/* Mark it free */
  return OK;
#endif
}

#ifdef CONFIG_OSS_VMIX_FLOAT

#undef FP_SAVE
#undef FP_RESTORE
#define FP_SAVE(envbuf)		asm ("fnsave %0":"=m" (*envbuf));
#define FP_RESTORE(envbuf)		asm ("frstor %0":"=m" (*envbuf));

/* SSE/SSE2 compatible macros */
#define FX_SAVE(envbuf)		asm ("fxsave %0":"=m" (*envbuf));
#define FX_RESTORE(envbuf)		asm ("fxrstor %0":"=m" (*envbuf));

static int old_arch = 0;	/* No SSE/SSE2 instructions */

#if defined(__amd64__)
#define AMD64
#endif

static inline void
cpuid (int op, int *eax, int *ebx, int *ecx, int *edx)
{
__asm__ ("cpuid": "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx):"0" (op), "c"
	   (0));
}

#ifdef AMD64
#  define local_save_flags(x)     asm volatile("pushfq ; popq %0":"=g" (x):)
#  define local_restore_flags(x)  asm volatile("pushq %0 ; popfq"::"g" (x):"memory", "cc")
#else
#  define local_save_flags(x)     asm volatile("pushfl ; popl %0":"=g" (x):)
#  define local_restore_flags(x)  asm volatile("pushl %0 ; popfl"::"g" (x):"memory", "cc")
#endif

static inline unsigned long
read_cr0 (void)
{
  unsigned long cr0;
#ifdef AMD64
asm ("movq %%cr0,%0":"=r" (cr0));
#else
asm ("movl %%cr0,%0":"=r" (cr0));
#endif
  return cr0;
}

static inline void
write_cr0 (unsigned long val)
{
#ifdef AMD64
  asm ("movq %0,%%cr0"::"r" (val));
#else
  asm ("movl %0,%%cr0"::"r" (val));
#endif
}

static inline unsigned long
read_cr4 (void)
{
  unsigned long cr4;
#ifdef AMD64
asm ("movq %%cr4,%0":"=r" (cr4));
#else
asm ("movl %%cr4,%0":"=r" (cr4));
#endif
  return cr4;
}

static inline void
write_cr4 (unsigned long val)
{
#ifdef AMD64
  asm ("movq %0,%%cr4"::"r" (val));
#else
  asm ("movl %0,%%cr4"::"r" (val));
#endif
}
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
  local_restore_flags (flags_reg);

  local_save_flags (flags_reg);
  if (flags_reg & FLAGS_ID)
    return 0;

  flags_reg |= FLAGS_ID;
  local_restore_flags (flags_reg);

  local_save_flags (flags_reg);
  if (!(flags_reg & FLAGS_ID))
    return 0;

//#define CPUID_FXSR	(1<<24)
//#define CPUID_SSE	(1<<25)
//#define CPUID_SSE2	(1<<26)

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
      flags[1] = read_cr4 ();
      write_cr4 (flags[1] | 0x600);	/* Set OSFXSR & OSXMMEXCEPT */
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
      write_cr4 (flags[1]);	/* Restore cr4 */
    }
  write_cr0 (flags[0]);		/* Restore cr0 */
}
#endif

typedef struct tmout_desc
{
  volatile int active;
  int timestamp;
  void (*func) (void *);
  void *arg;

  WDOG_ID id;

} tmout_desc_t;

static volatile int next_id = 0;
#define MAX_TMOUTS 128

tmout_desc_t tmouts[MAX_TMOUTS] = { {0} };

int timeout_random = 0x12123400;

void
oss_timer_callback (int id)
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
  wdDelete(tmout->id);
}

timeout_id_t
oss_timeout (void (*func) (void *), void *arg,
				 unsigned long long ticks)
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

  if ((tmout->id=wdCreate()) == NULL)
     return 0;

  wdStart(tmout->id, ticks, (FUNCPTR)oss_timer_callback, (int)tmout->timestamp);
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
     {
	wdCancel(tmout->id);
	wdDelete(tmout->id);
     }

  tmout->active = 0;
  tmout->timestamp = 0;
}

void
oss_udelay(unsigned long ticks)
{
	// TODO
}

void *
oss_contig_malloc (oss_device_t * osdev, int buffsize, oss_uint64_t memlimit,
		   oss_native_word * phaddr)
{
  char *start_addr, *end_addr;

  *phaddr = 0;

  start_addr = NULL;

  // TODO: See if there is a previously freed buffer available

  start_addr = (char *) valloc (buffsize);

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
    }

  *phaddr = (oss_native_word)start_addr;
  return start_addr;
}

void
oss_contig_free (oss_device_t * osdev, void *p, int buffsize)
{
  if (p == NULL)
    return;

  // TODO: Put the freed memory block to available list
  cmn_err (CE_WARN, "Cannot free %d bytes of DMA buffer\n", buffsize);
}

int
__oss_alloc_dmabuf (int dev, dmap_p dmap, unsigned int alloc_flags,
		    oss_uint64_t maxaddr, int direction)
{
  void *buf;
  int err;
  oss_native_word phaddr;
  int size = 64 * 1024;
  extern int dma_buffsize;

  if (dma_buffsize > 16 && dma_buffsize <= 128)
    size = dma_buffsize * 1024;

  if (dmap->dmabuf != NULL)
    return 0;			/* Already done */

  if (dmap == NULL)
    {
      cmn_err (CE_WARN, "oss_alloc_dmabuf: dmap==NULL\n");
      return OSS_EIO;
    }

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

  dmap->dmabuf = NULL;
  dmap->buffsize = size;

  err = -1;

  while (err < 0 && dmap->dmabuf == NULL && dmap->buffsize >= 4 * 1024)
    {
      if ((buf =
	   oss_contig_malloc (dmap->osdev, dmap->buffsize, maxaddr,
			      &phaddr)) == NULL)
	{
	  if ((dmap->buffsize = (dmap->buffsize / 2)) < 8 * 1024)
	    return OSS_ENOMEM;
	  cmn_err (CE_CONT, "Dropping DMA buffer size to %d bytes.\n",
		   dmap->buffsize);
	  continue;
	}

      dmap->dmabuf = buf;
      dmap->dmabuf_phys = phaddr;

      return 0;
    }

  return OSS_ENOMEM;
}

void
oss_free_dmabuf (int dev, dmap_p dmap)
{
  void *buf = dmap->dmabuf;

  if (dmap->dmabuf == NULL)
    return;

  dmap->dmabuf = NULL;
  oss_contig_free (NULL, buf, dmap->buffsize);
  dmap->dmabuf_phys = 0;
}

/*
 * Sleep/wakeup
 */

struct oss_wait_queue
{
  volatile int flags;
  SEM_ID wq;
};

struct oss_wait_queue *
oss_create_wait_queue (oss_device_t * osdev, const char *name)
{
  struct oss_wait_queue *wq;

  if ((wq = malloc (sizeof (*wq))) == NULL)
    {
      oss_cmn_err (CE_WARN, "vmalloc(%d) failed (wq)\n", sizeof (*wq));
      return NULL;
    }
  wq->wq = semBCreate(SEM_Q_FIFO, SEM_EMPTY);

  return wq;
}

void
oss_reset_wait_queue (struct oss_wait_queue *wq)
{
	// TODO: ?
}

void
oss_remove_wait_queue (struct oss_wait_queue *wq)
{
  free (wq);
}

int
oss_sleep (struct oss_wait_queue *wq, oss_mutex_t * mutex, int ticks,
	   oss_native_word * flags, unsigned int *status)
{
  int result;

  *status = 0;

  if (wq == NULL)
    return 0;

  wq->flags = 0;

  if (ticks <= 0)
     ticks = WAIT_FOREVER;

    result =
	    semTake(wq->wq, ticks);

  if (result == ERROR)		/* Signal received */
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
  // TODO: ?
  return 0;
}

void
oss_wakeup (struct oss_wait_queue *wq, oss_mutex_t * mutex,
	    oss_native_word * flags, short events)
{
  if (wq == NULL)
    return;

  wq->flags |= WK_WAKEUP;
  semFlush(wq->wq);
}
