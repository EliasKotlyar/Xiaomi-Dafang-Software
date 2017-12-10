/*
 * Purpose: Operating system abstraction functions for FreeBSD
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
#include <sys/conf.h>
#include <sys/module.h>
#include <sys/proc.h>
#include <sys/sx.h>
#include <sys/mman.h>
#include <sys/lockmgr.h>
#include <fs/devfs/devfs.h>
#include <sys/poll.h>
#include <sys/param.h>
#include <sys/filio.h>

/* Function prototypes */
static d_open_t oss_open;
static d_close_t oss_close;
static d_read_t oss_read;
static d_write_t oss_write;
static d_ioctl_t oss_ioctl;
static d_poll_t oss_poll;
static d_mmap_t oss_mmap;

/* Character device entry points */

static struct cdevsw oss_cdevsw = {
  .d_version = D_VERSION,
  .d_flags = D_TRACKCLOSE,
  .d_open = oss_open,
  .d_close = oss_close,
  .d_read = oss_read,
  .d_write = oss_write,
  .d_ioctl = oss_ioctl,
  .d_poll = oss_poll,
  .d_mmap = oss_mmap,
  .d_name = "oss"
};

static int oss_major = 30;
oss_device_t *core_osdev = NULL;
static int open_devices = 0;
static int refcount = 0;

#define MAX_CARDS	32
int oss_num_cards = 0;
static oss_device_t *cards[MAX_CARDS];
static int oss_expired = 0;


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

oss_wait_queue_t *
oss_create_wait_queue (oss_device_t * osdev, const char *name)
{
  oss_wait_queue_t *wq;

  if ((wq = PMALLOC (NULL, sizeof (*wq))) == NULL)
    return NULL;

  MUTEX_INIT (osdev, wq->mutex, MH_TOP);

  return wq;
}

void
oss_reset_wait_queue (oss_wait_queue_t * wq)
{
  wq->flags = 0;
}

void
oss_remove_wait_queue (oss_wait_queue_t * wq)
{
  MUTEX_CLEANUP (wq->mutex);
}

int
oss_sleep (oss_wait_queue_t * wq, oss_mutex_t * mutex, int ticks,
	   oss_native_word * flags, unsigned int *status)
{
  int flag;
  *status = 0;

  if (wq == NULL)
    return 0;

  wq->flags = 0;
#ifdef USE_SX_LOCK
  flag = sx_sleep (wq, *mutex, PRIBIO | PCATCH, "oss", ticks);
#else
#if __FreeBSD_version >= 602000
  flag = msleep_spin (wq, *mutex, "oss", ticks);
#else
  flag = msleep (wq, *mutex, PRIBIO | PCATCH, "oss", ticks);
#endif
#endif
  if (flag == EWOULDBLOCK)	/* Timeout */
    {
      return 0;
    }

  if (flag != 0)
    {
      *status |= WK_SIGNAL;
    }

  return 1;
}

int
oss_register_poll (oss_wait_queue_t * wq, oss_mutex_t * mutex,
		   oss_native_word * flags, oss_poll_event_t * ev)
{
  MUTEX_EXIT_IRQRESTORE (*mutex, *flags);
  selrecord (ev->p, &wq->poll_info);
  MUTEX_ENTER_IRQDISABLE (*mutex, *flags);
  return 0;
}

void
oss_wakeup (oss_wait_queue_t * wq, oss_mutex_t * mutex,
	    oss_native_word * flags, short events)
{
  MUTEX_EXIT_IRQRESTORE (*mutex, *flags);
  wakeup (wq);
  selwakeup (&wq->poll_info);
  MUTEX_ENTER_IRQDISABLE (*mutex, *flags);
}

void
oss_register_module (char *name)
{
  refcount++;
}

void
oss_unregister_module (char *name)
{
  refcount--;
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

int
oss_register_device (oss_device_t * osdev, const char *name)
{
  if ((osdev->name = PMALLOC (NULL, strlen (name) + 1)) == NULL)
    {
      cmn_err (CE_WARN, "Cannot allocate memory for device name\n");
      osdev->name = "Unknown device";
    }
  strcpy (osdev->name, name);
  if (osdev->dip != NULL)
    device_set_desc (osdev->dip, name);
  return OSS_ENXIO;
}

void
oss_unregister_device (oss_device_t * osdev)
{
  // NOP
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

void *
oss_find_minor_info (int dev_class, int instance)
{
  int i;

  for (i = 0; i < oss_num_cdevs; i++)
    {
      if (oss_cdevs[i]->d != NULL && oss_cdevs[i]->dev_class == dev_class
	  && oss_cdevs[i]->instance == instance)
	return oss_cdevs[i]->info;
    }

  return NULL;
}

int
oss_disable_device (oss_device_t * osdev)
{
  int i;

  if (osdev->refcount > 0)
    return OSS_EBUSY;

  if (open_devices > 0)
    return OSS_EBUSY;

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
  struct cdev *bsd_cdev;
  oss_cdev_t *cdev = NULL;
  int i, num;

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

  if (!(flags & CHDEV_VIRTUAL) && (name != NULL))
    {
#if __FreeBSD_version >= 900023
      bsd_cdev =
	make_dev_credf (MAKEDEV_CHECKNAME, &oss_cdevsw, num, NULL,
			UID_ROOT, GID_WHEEL, 0666, name, 0);
      if (bsd_cdev == NULL)
	{
	  cmn_err (CE_WARN, "Cannot allocate device node /dev/%s\n", name);
	  return;
	}
#else
      bsd_cdev =
	make_dev (&oss_cdevsw, num, UID_ROOT, GID_WHEEL, 0666, name, 0);
#endif
      cdev->info = bsd_cdev;
    }
}

#define MAX_MEMBLOCKS	4096
static void *memblocks[MAX_MEMBLOCKS];
static int nmemblocks = 0;

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
oss_uiomove (void *address, size_t nbytes, enum uio_rw rwflag, uio_t * uio_p)
{
  int err;

#undef uiomove
  err = uiomove (address, nbytes, uio_p);

  return err;
}

#undef timeout
#undef untimeout

typedef struct tmout_desc
{
  volatile int active;
  int timestamp;
  void (*func) (void *);
  void *arg;

  struct callout_handle timer;
} tmout_desc_t;

static volatile int next_id = 0;
#define MAX_TMOUTS 128

tmout_desc_t tmouts[MAX_TMOUTS] = { {0} };

int timeout_random = 0x12123400;

void
oss_timer_callback (void *arg)
{
  int ix;
  tmout_desc_t *tmout = arg;

  timeout_random++;

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
      cmn_err (CE_CONT, "Timeout table full\n");
      return 0;
    }

  tmout->func = func;
  tmout->arg = arg;
  tmout->timestamp = id | (timeout_random & ~0xff);

  tmout->timer = timeout (oss_timer_callback, tmout, ticks);

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
    untimeout (oss_timer_callback, tmout, tmout->timer);
  tmout->active = 0;
  tmout->timestamp = 0;
}

int
oss_get_procinfo (int what)
{
  switch (what)
    {
      case OSS_GET_PROCINFO_UID:
        return oss_get_uid();
    }

   return EINVAL; 
}

unsigned long
oss_get_time (void)
{
  struct timeval timecopy;

  getmicrotime (&timecopy);
  return timecopy.tv_usec / (1000000 / hz) + (u_long) timecopy.tv_sec * hz;
}

caddr_t
oss_map_pci_mem (oss_device_t * osdev, int nr, int paddr, int psize)
{
  void *vaddr = 0;
  u_int32_t poffs;

  poffs = paddr - trunc_page (paddr);
  vaddr = (caddr_t) pmap_mapdev (paddr - poffs, psize + poffs) + poffs;

  return vaddr;
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

static time_t
oss_get_walltime (void)
{
  struct timeval timecopy;

  getmicrotime (&timecopy);
  return timecopy.tv_sec;
}

int
soundcard_attach (void)
{
  oss_device_t *osdev;

  if (module_lookupbyname("sound") != NULL)
    {
      cmn_err (CE_WARN, "Open Sound System conflicts with FreeBSD driver\n");
      cmn_err (CE_CONT, "Please remove sound(4) from kernel or unload it\n");
      return EBUSY;
    }
  if ((osdev = PMALLOC (NULL, sizeof (*osdev))) == NULL)
    {
      return ENOSPC;
    }

  memset (osdev, 0, sizeof (*osdev));
  core_osdev = osdev;

#ifdef LICENSED_VERSION
  if (!oss_license_handle_time (oss_get_walltime ()))
    {
      cmn_err (CE_WARN, "This version of Open Sound System has expired\n");
      cmn_err (CE_CONT,
	       "Please download the latest version from www.opensound.com\n");
      oss_expired = 1;
    }
#endif

  osdev->major = oss_major;
  oss_register_device (osdev, "OSS core services");

  oss_common_init (osdev);

  return 0;
}

int
soundcard_detach (void)
{
  int i;

  if (refcount > 0 || open_devices > 0)
    return EBUSY;

  oss_unload_drivers ();

  osdev_delete (core_osdev);

  for (i = 0; i < nmemblocks; i++)
    KERNEL_FREE (memblocks[i]);
  nmemblocks = 0;

  return 0;
}

static void
init_fileinfo (struct fileinfo *fi, int flags)
{
  memset (fi, 0, sizeof (*fi));
  if ((flags & FREAD) && (flags & FWRITE))
    fi->mode = OPEN_READWRITE;
  else if (flags & FREAD)
    fi->mode = OPEN_READ;
  else if (flags & FWRITE)
    fi->mode = OPEN_WRITE;
  fi->acc_flags = flags;
  fi->pid = -1;
  fi->dev = -1;
  fi->cmd = NULL;
}

static int
oss_read (struct cdev *bsd_dev, struct uio *buf, int flags)
{
  int retval;
  int dev;
  oss_cdev_t *cdev;
#ifndef VDEV_SUPPORT
  struct fileinfo _fi, * fi = &_fi;
  dev = MINOR (bsd_dev);
  init_fileinfo (fi, flags);
#else
  struct fileinfo * fi;
  if (oss_file_get_private ((void **)&fi)) return ENXIO;
  dev = fi->dev;
#endif

  if (dev >= oss_num_cdevs)
    return ENXIO;

  if ((cdev = oss_cdevs[dev]) == NULL || cdev->d == NULL)
    return ENXIO;

  if (cdev->d->read == NULL)
    {
      return ENODEV;
    }

  retval = cdev->d->read (cdev->instance, fi, buf, buf->uio_resid);
  if (retval < 0)
    return -retval;
  return 0;

}

static int
oss_write (struct cdev *bsd_dev, struct uio *buf, int flags)
{
  int retval;
  int dev;
  oss_cdev_t *cdev;
#ifndef VDEV_SUPPORT
  struct fileinfo _fi, * fi = &_fi;
  dev = MINOR (bsd_dev);
  init_fileinfo (fi, flags);
#else
  struct fileinfo * fi;
  if (oss_file_get_private ((void **)&fi)) return ENXIO;
  dev = fi->dev;
#endif

  if (dev >= oss_num_cdevs)
    return ENXIO;

  if ((cdev = oss_cdevs[dev]) == NULL || cdev->d == NULL)
    return ENXIO;

  if (cdev->d->write == NULL)
    {
      return ENODEV;
    }

  retval = cdev->d->write (cdev->instance, fi, buf, buf->uio_resid);
  if (retval < 0)
    return -retval;
  return 0;
}

static int
oss_open (struct cdev *bsd_dev, int flags, int mode, struct thread *p)
{
  int dev = MINOR (bsd_dev);
  oss_cdev_t *cdev;
  struct fileinfo fi;
  int tmpdev, retval;

  if (dev >= oss_num_cdevs)
    return ENXIO;

  if ((cdev = oss_cdevs[dev]) == NULL || cdev->d == NULL)
    return ENXIO;

  if (cdev->d->open == NULL)
    {
      return ENODEV;
    }

  init_fileinfo (&fi, flags);
  fi.pid = p->td_proc->p_pid;
  fi.cmd = p->td_proc->p_comm;
  tmpdev = dev;

  retval =
    cdev->d->open (cdev->instance, cdev->dev_class, &fi, 0, 0, &tmpdev);

  if (tmpdev != -1) fi.dev = tmpdev;
  else fi.dev = dev;
  if (retval < 0)
    return -retval;

#ifdef VDEV_SUPPORT
  if (oss_file_set_private (p, (void *)&fi, sizeof (struct fileinfo)))
    return ENXIO;
#endif

  open_devices++;
  return 0;
}

static int
oss_close (struct cdev *bsd_dev, int flags, int mode, struct thread *p)
{
  int dev;
  oss_cdev_t *cdev;
#ifndef VDEV_SUPPORT
  struct fileinfo _fi, * fi = &_fi;
  dev = MINOR (bsd_dev);
  init_fileinfo (fi, flags);
#else
  struct fileinfo * fi;
  if (oss_file_get_private ((void **)&fi)) return ENXIO;
  dev = fi->dev;
#endif

  if (dev >= oss_num_cdevs)
    return ENXIO;

  if ((cdev = oss_cdevs[dev]) == NULL || cdev->d == NULL)
    return ENXIO;

  if (cdev->d->close == NULL)
    {
      return ENODEV;
    }

  cdev->d->close (cdev->instance, fi);
  open_devices--;
  return 0;
}

static int
oss_ioctl (struct cdev *bsd_dev, u_long cmd, caddr_t arg, int mode,
	   struct thread *p)
{
  int retval;
  int dev;
  oss_cdev_t *cdev;
#ifndef VDEV_SUPPORT
  struct fileinfo _fi, * fi = &_fi;
  dev = MINOR (bsd_dev);
  init_fileinfo (fi, mode);
#else
  struct fileinfo * fi;
  if (oss_file_get_private ((void **)&fi)) return ENXIO;
  dev = fi->dev;
#endif

  if (dev >= oss_num_cdevs)
    return ENXIO;

  if ((cdev = oss_cdevs[dev]) == NULL || cdev->d == NULL)
    return ENXIO;

  if (cdev->d->ioctl == NULL)
    {
      return ENODEV;
    }

  switch (cmd)
    {
     /*
      * FreeBSD uses these ioctls to (un)set nonblocking I/O for devices. e.g.
      * in case of fcntl (fd, F_SETFL, O_RDONLY|O_NONBLOCK) it will fire
      * both ioctls.
      * We deal with them here, and not in oss_audio_core because struct file
      * isn't available in oss_audio_ioctl and we want the flags to remain
      * accurate. oss_audio_core checks for O_NONBLOCK, and will pick
      * up the change next read/write.
      */
      case FIONBIO:
        if (arg != NULL)
          {
            if (*arg) fi->acc_flags |= O_NONBLOCK;
            else fi->acc_flags &= ~O_NONBLOCK;
          }
      case FIOASYNC:
        return 0;
      default: break;
    }

  retval = cdev->d->ioctl (cdev->instance, fi, cmd, (ioctl_arg) arg);
  if (retval < 0)
    return -retval;
  return 0;
}

static int
oss_poll (struct cdev *bsd_dev, int events, struct thread *p)
{
  int retval;
  int dev;
  oss_cdev_t *cdev;
  oss_poll_event_t ev;
  int err;
#ifndef VDEV_SUPPORT
  struct fileinfo _fi, * fi = &_fi;
  dev = MINOR (bsd_dev);
  init_fileinfo (fi, 0);
#else
  struct fileinfo * fi;
  if (oss_file_get_private ((void **)&fi)) return ENXIO;
  dev = fi->dev;
#endif

  if (dev >= oss_num_cdevs)
    return ENXIO;

  if ((cdev = oss_cdevs[dev]) == NULL || cdev->d == NULL)
    return ENXIO;

  if (cdev->d->chpoll == NULL)
    {
      return ENODEV;
    }

  ev.events = events;
  ev.revents = 0;
  ev.p = p;
  ev.bsd_dev = bsd_dev;

  err = cdev->d->chpoll (cdev->instance, fi, &ev);
  if (err < 0)
    {
      return -err;
    }
  return ev.revents;
}

#if defined(D_VERSION_03) && (D_VERSION == D_VERSION_03)
static int
oss_mmap (struct cdev *bsd_dev, vm_ooffset_t offset, vm_paddr_t * paddr,
	  int nprot, vm_memattr_t *memattr)
#else
static int
oss_mmap (struct cdev *bsd_dev, vm_offset_t offset, vm_paddr_t * paddr,
	  int nprot)
#endif
{
  int retval;
  int dev;
  oss_cdev_t *cdev;
  oss_poll_event_t ev;
  dmap_p dmap = NULL;
  int err;
#ifndef VDEV_SUPPORT
  dev = MINOR (bsd_dev);
#else
  struct fileinfo * fi;
  if (oss_file_get_private ((void **)&fi)) return ENXIO;
  dev = fi->dev;
#endif

  if (dev >= oss_num_cdevs)
    return ENXIO;

  if ((cdev = oss_cdevs[dev]) == NULL || cdev->d == NULL)
    return ENXIO;

  if (nprot & PROT_EXEC)
    return EACCES;

  if ((cdev->dev_class != OSS_DEV_DSP) &&
      (cdev->dev_class != OSS_DEV_DSP_ENGINE))	/* Only mmapable devices */
    {
      cmn_err (CE_NOTE, "mmap() is only possible with DSP devices (%d)\n",
	       cdev->dev_class);
      return EINVAL;
    }

  dev = cdev->instance;

  if (dev < 0 || dev >= num_audio_engines)
    return ENODEV;

  if (nprot & PROT_WRITE)
    dmap = audio_engines[dev]->dmap_out;
  else
    dmap = audio_engines[dev]->dmap_in;

  if (dmap == NULL)
    return EIO;

  if (dmap->dmabuf_phys == 0)
    return EIO;

  if (dmap->flags & DMAP_COOKED)
    {
      cmn_err (CE_WARN,
	       "mmap() not possible with currently selected sample format.\n");
      return EIO;
    }

  dmap->mapping_flags |= DMA_MAP_MAPPED;
  *paddr = dmap->dmabuf_phys + offset;

  return 0;
}

oss_device_t *
osdev_create (dev_info_t * dip, int dev_type, int instance, const char *nick,
	      const char *handle)
{
  oss_device_t *osdev = NULL;
  int i, err;
  caddr_t addr;
  off_t region_size;

  if (handle == NULL)
    handle = nick;

  /*
   * Don't accept any more drivers if expired
   */
  if (oss_expired && oss_num_cards > 0)
    return NULL;

  for (i = 0; i < oss_num_cards; i++)
    {
      if (cards[i]->dip == dip)
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

  osdev->dip = dip;
  osdev->osid = dip;
  osdev->available = 1;
  osdev->instance = instance;
  osdev->dev_type = dev_type;
  osdev->devc = NULL;
  osdev->first_mixer = -1;
  sprintf (osdev->nick, "%s%d", nick, instance);
  strcpy (osdev->modname, nick);

/*
 * Create the device handle
 */
  switch (dev_type)
    {
    case DRV_PCI:
      {
	sprintf (osdev->handle, "OSS-PCI");
      }
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
	if (oss_cdevs[i]->info != NULL)
	  destroy_dev (oss_cdevs[i]->info);
	oss_cdevs[i]->d = NULL;
	oss_cdevs[i]->info = NULL;
	oss_cdevs[i]->osdev = NULL;
	strcpy (oss_cdevs[i]->name, "Removed device");
      }
}

void *
oss_get_osid (oss_device_t * osdev)
{
  return osdev->osid;
}

void
oss_inc_intrcount (oss_device_t * osdev, int claimed)
{
  osdev->intrcount++;

  if (claimed)
    osdev->ackcount++;
}
