/*
 * Purpose: Operating system abstraction functions for Linux
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

#include <oss_config.h>
#include <midi_core.h>

/*
 * OSS has traditionally used fixed character device number (14). However
 * current OSS uses fully dynamic major number allocation. The legacy
 * character device 14 is left for ALSA.
 */
static int osscore_major = 0;
static int oss_expired = 0;

/*
 * Number of cards supported in the same system. This should be largish
 * because unplugging/replugging USB cards in wrong way may create
 * large number of card instances.
 */
#define MAX_CARDS 32

static oss_device_t *cards[MAX_CARDS];
int oss_num_cards = 0;

void
oss_pci_byteswap (oss_device_t * osdev, int mode)
{
  // NOP
}

int
oss_pci_read_config_byte (oss_device_t * osdev, offset_t where,
			  unsigned char *val)
{
  return osscore_pci_read_config_byte (osdev->dip, where, val);
}

int
oss_pci_read_config_irq (oss_device_t * osdev, offset_t where,
			 unsigned char *val)
{
  return osscore_pci_read_config_irq (osdev->dip, where, val);
}

int
oss_pci_read_config_word (oss_device_t * osdev, offset_t where,
			  unsigned short *val)
{
  if (osdev == NULL)
    {
      cmn_err (CE_CONT, "oss_pci_read_config_word: osdev==NULL\n");
      return PCIBIOS_FAILED;
    }

  return osscore_pci_read_config_word (osdev->dip, where, val);
}

int
oss_pci_read_config_dword (oss_device_t * osdev, offset_t where,
			   unsigned int *val)
{
  return osscore_pci_read_config_dword (osdev->dip, where, val);
}

int
oss_pci_write_config_byte (oss_device_t * osdev, offset_t where,
			   unsigned char val)
{
  return osscore_pci_write_config_byte (osdev->dip, where, val);
}

int
oss_pci_write_config_word (oss_device_t * osdev, offset_t where,
			   unsigned short val)
{
  return osscore_pci_write_config_word (osdev->dip, where, val);
}

int
oss_pci_write_config_dword (oss_device_t * osdev, offset_t where,
			    unsigned int val)
{
  return osscore_pci_write_config_dword (osdev->dip, where, val);
}

int
oss_pci_enable_msi (oss_device_t * osdev)
{
  return osscore_pci_enable_msi (osdev->dip);
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
	unsigned int subvendor;
	char *devpath;
	devpath = oss_pci_read_devpath (osdev->dip);
	oss_pci_read_config_dword (osdev, 0x2c, &subvendor);

	sprintf (osdev->handle, "PCI%08x-%s", subvendor, devpath);
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

void
osdev_set_owner (oss_device_t * osdev, struct module *owner)
{
  osdev->owner = owner;
}

void
osdev_set_major (oss_device_t * osdev, int major)
{
  osdev->major = major;
}

void
osdev_set_irqparms (oss_device_t * osdev, void *irqparms)
{
  osdev->irqparms = irqparms;
}

void *
osdev_get_irqparms (oss_device_t * osdev)
{
  return osdev->irqparms;
}

char *
osdev_get_nick (oss_device_t * osdev)
{
  return osdev->nick;
}

int
osdev_get_instance (oss_device_t * osdev)
{
  return osdev->instance;
}

void
oss_inc_intrcount (oss_device_t * osdev, int claimed)
{
  osdev->intrcount++;

  if (claimed)
    osdev->ackcount++;
}

struct module *
osdev_get_owner (oss_device_t * osdev)
{
  return osdev->owner;
}

void *
oss_get_osid (oss_device_t * osdev)
{
  return NULL;			// TODO
}

int
oss_get_procinfo(int what)
{
	switch (what)
	{
	case OSS_GET_PROCINFO_UID:
		return oss_get_uid();
		break;
	}

	return OSS_EINVAL;
}

int
oss_disable_device (oss_device_t * osdev)
{
  int i;

  if (osdev->major > 0)
    oss_unregister_chrdev (osdev->major, osdev->nick);
  osdev->major = 0;

/*
 * Now mark all devices unavailable (for the time being)
 * TODO: Mobe this stuff to some common OSS module (also in Solaris)
 */
  if (osdev->refcount > 0)
    {
      return OSS_EBUSY;
    }

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

static inline int
cpy_file (oss_file_handle_t * f, struct fileinfo *fi)
{
  // TODO: Handle acc_flags properly
  fi->acc_flags = 0;
  fi->mode = 0;
  fi->acc_flags = oss_file_get_flags (f);

  if ((fi->acc_flags & O_ACCMODE) == O_RDWR)
    fi->mode = OPEN_READWRITE;
  if ((fi->acc_flags & O_ACCMODE) == O_RDONLY)
    fi->mode = OPEN_READ;
  if ((fi->acc_flags & O_ACCMODE) == O_WRONLY)
    fi->mode = OPEN_WRITE;

  return fi->mode;
}

static int
oss_cdev_open (oss_inode_handle_t * inode, oss_file_handle_t * file)
{
  int dev = oss_inode_get_minor (inode);
  oss_native_word d;
  int tmpdev, dev_class, retval;
  struct fileinfo fi;
  oss_cdev_t *cdev;

  cpy_file (file, &fi);

  if (dev > oss_num_cdevs)
    return OSS_ENXIO;
  if (oss_cdevs == NULL)
     return OSS_ENXIO;
  if ((cdev = oss_cdevs[dev]) == NULL || cdev->d == NULL)
    return OSS_ENXIO;

  DDB (cmn_err
       (CE_CONT, "oss_cdev_open(%d): %s, class=%d, instance=%d\n", dev,
	cdev->name, cdev->dev_class, cdev->instance));

  if (cdev->d->open == NULL)
    {
      return OSS_ENODEV;
    }

  dev_class = cdev->dev_class;

  tmpdev = -1;
  oss_inc_refcounts ();
  retval =
    cdev->d->open (cdev->instance, cdev->dev_class, &fi, 0, 0, &tmpdev);

  if (retval < 0)
    {
      oss_dec_refcounts ();
      return retval;
    }

  if (tmpdev != -1)
    dev = tmpdev;

  //open_devices++;
  //open_count[dev]++;

  d = dev;
  oss_file_set_private (file, (void *) d);

  return 0;
}

static int
oss_cdev_release (oss_inode_handle_t * inode, oss_file_handle_t * file)
{
  oss_native_word d = (oss_native_word) oss_file_get_private (file);
  int dev = d;
  struct fileinfo fi;
  oss_cdev_t *cdev;

  cpy_file (file, &fi);

  if (dev > oss_num_cdevs)
    return 0;
  if ((cdev = oss_cdevs[dev]) == NULL || cdev->d->close == NULL)
    {
      return 0;
    }

  cdev->d->close (cdev->instance, &fi);
  oss_dec_refcounts ();

  return 0;
}

static ssize_t
oss_cdev_read (oss_file_handle_t * file, char *buf, size_t count,
	       loff_t * offs)
{
  oss_native_word d = (oss_native_word) oss_file_get_private (file);
  int dev = d;
  oss_cdev_t *cdev;
  int err;
  uio_t uio;

  struct fileinfo fi;
  cpy_file (file, &fi);

  if (dev > oss_num_cdevs)
    return OSS_ENXIO;
  if ((cdev = oss_cdevs[dev]) == NULL || cdev->d->read == NULL)
    {
      return OSS_ENXIO;
    }

  if ((err = oss_create_uio (&uio, buf, count, UIO_READ, 0)) < 0)
    {
      return err;
    }

  err = cdev->d->read (cdev->instance, &fi, &uio, count);

  return err;
}

static ssize_t
oss_cdev_write (oss_file_handle_t * file, char *buf, size_t count,
		loff_t * offs)
{
  oss_native_word d = (oss_native_word) oss_file_get_private (file);
  int dev = d;
  oss_cdev_t *cdev;
  int err;
  uio_t uio;

  struct fileinfo fi;
  cpy_file (file, &fi);

  if (dev > oss_num_cdevs)
    return OSS_ENXIO;
  if ((cdev = oss_cdevs[dev]) == NULL || cdev->d->write == NULL)
    {
      return OSS_ENXIO;
    }

  if ((err = oss_create_uio (&uio, buf, count, UIO_WRITE, 0)) < 0)
    {
      return err;
    }

  err = cdev->d->write (cdev->instance, &fi, &uio, count);

  return err;
}

static int
oss_cdev_ioctl (oss_inode_handle_t * inode, oss_file_handle_t * file,
		unsigned int cmd, unsigned long arg)
{
  oss_native_word d = (oss_native_word) oss_file_get_private (file);
  int dev = d;
  oss_cdev_t *cdev;
  int err;
  /* Remove localbuf (workaround for CONFIG_HARDENED_USERCOPY from kernel 4.8) */
  /* int localbuf[64]; */		/* 256 bytes is the largest frequently used ioctl size */

  int len = 0;
  int alloced = 0;
  int *ptr = (int *) arg;

  struct fileinfo fi;
  cpy_file (file, &fi);

  if (dev > oss_num_cdevs)
    return OSS_ENXIO;

  if ((cdev = oss_cdevs[dev]) == NULL || cdev->d->ioctl == NULL)
    {
      return OSS_ENXIO;
    }

  if (__SIOC_DIR (cmd) != __SIOC_NONE && __SIOC_DIR (cmd) != 0)
    {
      len = __SIOC_SIZE (cmd);
      if (len < 1 || len > 65536 || arg == 0)
	{
	  cmn_err (CE_WARN, "Bad ioctl command %x, %d, %x\n", cmd, len, arg);
	  return OSS_EFAULT;
	}

      /* Always use dynamic kernel memory allocation (instead of static localbuf)
        (workaround for CONFIG_HARDENED_USERCOPY from kernel 4.8) */
      ptr = KERNEL_MALLOC (len);
      alloced = 1;
      /* Use statically allocated buffer for short arguments */
      /*if (len > sizeof (localbuf))
	{
	  ptr = KERNEL_MALLOC (len);
	  alloced = 1;
	}
      else
	ptr = localbuf;*/

      if (ptr == NULL || arg == 0)
	{
	  return OSS_EFAULT;
	}

      if (__SIOC_DIR (cmd) & __SIOC_WRITE)
	{
	  if (oss_copy_from_user (ptr, (char *) arg, len))
	    {
	      if (alloced)
		KERNEL_FREE (ptr);
	      return OSS_EFAULT;
	    }
	}
    }

  if ((err = cdev->d->ioctl (cdev->instance, &fi, cmd, (ioctl_arg) ptr)) < 0)
    {
      if (alloced)
	KERNEL_FREE (ptr);
      return err;
    }

  if (__SIOC_DIR (cmd) & __SIOC_READ)
    {
      if (oss_copy_to_user ((char *) arg, ptr, len))
	{
	  if (alloced)
	    KERNEL_FREE (ptr);
	  return OSS_EFAULT;
	}
    }

  /* Free the local buffer unless it was statically allocated */
  if (ptr != NULL && alloced)
//    if (len > sizeof (localbuf))
    KERNEL_FREE (ptr);

  return ((err < 0) ? err : 0);

}

/* No BKL if this is used */
static long
oss_cdev_unlocked_ioctl (oss_file_handle_t * file, unsigned int cmd,
		         unsigned long arg)
{
  return oss_cdev_ioctl (NULL, file, cmd, arg);
}

/* Used for 32 bit clients on a 64 bit kernel. No BKL here either */
static long
oss_cdev_compat_ioctl (oss_file_handle_t * file, unsigned int cmd,
	               unsigned long arg)
{
  return oss_cdev_ioctl (NULL, file, cmd, arg);
}

static unsigned int
oss_cdev_poll (oss_file_handle_t * file, oss_poll_table_handle_t * wait)
{
  oss_poll_event_t ev;
  oss_native_word d = (oss_native_word) oss_file_get_private (file);
  int dev = d;
  oss_cdev_t *cdev;
  int err;

  struct fileinfo fi;
  cpy_file (file, &fi);

  if (dev > oss_num_cdevs)
    return OSS_ENXIO;
  if ((cdev = oss_cdevs[dev]) == NULL || cdev->d->chpoll == NULL)
    {
      return OSS_ENXIO;
    }

  ev.wait = wait;
  ev.file = file;
  ev.events = POLLOUT | POLLWRNORM | POLLIN | POLLRDNORM;
  ev.revents = 0;
  err = cdev->d->chpoll (cdev->instance, &fi, &ev);
  if (err < 0)
    {
      return err;
    }

  return ev.revents;
}

static int
oss_cdev_mmap (oss_file_handle_t * file, oss_vm_area_handle_t * vma)
{
  oss_native_word d = (oss_native_word) oss_file_get_private (file);
  int dev = d;
  oss_cdev_t *cdev;
  dmap_p dmap = NULL;
  int err;

  if (dev > oss_num_cdevs)
    return OSS_ENXIO;

  if ((cdev = oss_cdevs[dev]) == NULL)
    {
      return OSS_ENXIO;
    }

  if (cdev->dev_class != OSS_DEV_DSP && cdev->dev_class != OSS_DEV_DSP_ENGINE)	/* Only mmap audio devices */
    {
      return OSS_ENXIO;
    }

  dev = cdev->instance;
  if (dev < 0 || dev >= num_audio_engines)
    return OSS_ENXIO;

  if (audio_engines[dev]->flags & ADEV_NOMMAP)
     return OSS_EIO;

  if (oss_vma_get_flags (vma) & VM_WRITE)	/* Map write and read/write to the output buf */
    {
      dmap = audio_engines[dev]->dmap_out;
    }
  else if (oss_vma_get_flags (vma) & VM_READ)
    {
      dmap = audio_engines[dev]->dmap_in;
    }
  else
    {
      cmn_err (CE_WARN, "Undefined mmap() access\n");
      return OSS_EINVAL;
    }

  if (dmap == NULL)
    {
      cmn_err (CE_WARN, "mmap() error. dmap == NULL\n");
      return OSS_EIO;
    }

  if (dmap->dmabuf == NULL)
    {
      cmn_err (CE_WARN, "mmap() called when raw_buf == NULL\n");
      return OSS_EIO;
    }

  if (dmap->dmabuf_phys == 0)
    {
      cmn_err (CE_WARN, "mmap() not supported by device /dev/dsp%d.\n", dev);
      return OSS_EIO;
    }

  if (dmap->mapping_flags)
    {
      cmn_err (CE_WARN, "mmap() called twice for the same DMA buffer\n");
      return OSS_EIO;
    }

  if (dmap->flags & DMAP_COOKED)
    {
      cmn_err (CE_WARN,
	       "mmap() not possible with currently selected sample format.\n");
      return OSS_EIO;
    }

  if ((err = oss_do_mmap (vma, dmap->dmabuf_phys, dmap->bytes_in_use)) < 0)
    return err;

  dmap->mapping_flags |= DMA_MAP_MAPPED;

  memset (dmap->dmabuf, dmap->neutral_byte, dmap->bytes_in_use);
  return 0;
}

oss_file_operation_handle_t oss_fops = {
  oss_cdev_read,
  oss_cdev_write,
  NULL,				/* oss_readdir */
  oss_cdev_poll,
  oss_cdev_ioctl,
  oss_cdev_mmap,
  oss_cdev_open,
  oss_cdev_release,
  oss_cdev_compat_ioctl,
  oss_cdev_unlocked_ioctl
};

static int
hookup_cdevsw (oss_device_t * osdev)
{
  return oss_register_chrdev (osdev, osscore_major, "osscore", &oss_fops);
}

int
oss_request_major (oss_device_t * osdev, int major, char *module)
{
  int err;

  err = oss_register_chrdev (osdev, major, module, &oss_fops);

  return err;
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

  if (osdev->major == 0)
    {
      cmn_err (CE_WARN, "Module %s major=0\n", osdev->nick);
      return;
    }

  if (osdev->major < 0)
    {
      cmn_err (CE_WARN, "Failed to allocate major device for %s\n",
	       osdev->nick);
    }

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
    }

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
      oss_register_minor (osdev->major, num, name);
    }
}

int
oss_init_osscore (oss_device_t * osdev)
{

#ifdef LICENSED_VERSION
  if (!oss_license_handle_time (oss_get_time ()))
    {
      cmn_err (CE_WARN, "This version of Open Sound System has expired\n");
      cmn_err (CE_CONT,
	       "Please download the latest version from www.opensound.com\n");
      oss_expired = 1;
    }
#endif

  if ((osscore_major = hookup_cdevsw (osdev)) < 0)
    {
      cmn_err (CE_WARN, "Failed to allocate character major number %d\n",
	       osscore_major);
      return OSS_EBUSY;
    }

  osdev->major = osscore_major;
  oss_register_device (osdev, "OSS core services");

  oss_common_init (osdev);

  return 0;
}

void
oss_uninit_osscore (oss_device_t * osdev)
{
  oss_unload_drivers ();

  // free_all_irqs ();          /* If something was left allocated by accident */
  oss_unregister_chrdev (osscore_major, "osscore");
}

/*
 * oss_pmalloc() is only used by usb_wraper.inc.
 */
void *
oss_pmalloc (size_t sz)
{
	return oss_memblk_malloc(&oss_global_memblk, sz);
}
