/*
 * Purpose: Wrapper routines for Linux kernel services
 *
 * The functions and structures declared here are part of the osscore.c
 * file that is compiled in the target system. This file must not be
 * modified in the target system because the precompiled binaries included
 * in the OSS installation package depend on it too.
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

/*
 * Some integer types
 */
#if defined(__x86_64__)
typedef unsigned long oss_native_word;	/* Same as the address and status register size */
#else
typedef unsigned long oss_native_word;	/* Same as the address and status register size */
#endif
typedef long long oss_int64_t;			/* Signed 64 bit integer */
typedef unsigned long long oss_uint64_t;	/* Unsigned 64 bit integer */

extern char *oss_strcpy (char *s1, const char *s2);
extern void *oss_memcpy (void *t_, const void *f_, size_t l);
extern void *oss_memset (void *t, int val, size_t l);
extern int oss_strcmp (const char *s1, const char *s2);
extern size_t oss_strlen (const char *s);
extern char *oss_strncpy (char *s1, const char *s2, size_t l);
extern void oss_udelay (unsigned long d);

typedef struct _oss_mutex_t *oss_mutex_t;
typedef struct _poll_table_handle oss_poll_table_handle_t;
typedef struct _file_handle_t oss_file_handle_t;

struct _oss_poll_event_t
{
  short events, revents;
  oss_poll_table_handle_t *wait;
  oss_file_handle_t *file;
};
typedef struct _oss_poll_event_t oss_poll_event_t;

extern oss_mutex_t oss_mutex_init (void);
extern void oss_mutex_cleanup (oss_mutex_t mutex);
extern void oss_spin_lock_irqsave (oss_mutex_t mutex,
				   oss_native_word * flags);
extern void oss_spin_unlock_irqrestore (oss_mutex_t mutex,
					oss_native_word flags);
extern void oss_spin_lock (oss_mutex_t mutex);
extern void oss_spin_unlock (oss_mutex_t mutex);
extern unsigned long long oss_get_jiffies (void);
extern char *oss_get_procname (void);
extern int oss_get_pid (void);
extern unsigned int oss_get_uid (void);


struct oss_wait_queue;
struct module;
struct _oss_device_t;
struct pci_dev;

typedef void *oss_dma_handle_t; /* Unused type */

/*
 * Sleep/wakeup/poll support. These definitions are duplicates from
 * oss_config.h which is the official place. Both definitions must match.
 */

extern struct oss_wait_queue *oss_create_wait_queue (oss_device_t * osdev,
						     const char *name);
extern void oss_reset_wait_queue (struct oss_wait_queue *wq);
extern void oss_remove_wait_queue (struct oss_wait_queue *wq);
extern int oss_sleep (struct oss_wait_queue *wq, oss_mutex_t * mutex,
		      int ticks, oss_native_word * flags,
		      unsigned int *status);
extern int oss_register_poll (struct oss_wait_queue *wq, oss_mutex_t * mutex,
			      oss_native_word * flags, oss_poll_event_t * ev);
extern void oss_wakeup (struct oss_wait_queue *wq, oss_mutex_t * mutex,
			oss_native_word * flags, short events);

extern void oss_cmn_err (int level, const char *format, ...);
#define CE_CONT		0
#define CE_NOTE		1
#define CE_WARN		2
#define CE_PANIC	3

typedef int timeout_id_t;
extern timeout_id_t oss_timeout (void (*func) (void *), void *arg,
				 unsigned long long ticks);
extern void oss_untimeout (timeout_id_t id);

extern int sprintf (char *buf, const char *s, ...);

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
			uio_t * uio_p);
extern int oss_create_uio (uio_t * uiop, char *buf, size_t count, uio_rw_t rw,
			   int is_kernel);
extern void *oss_kmem_alloc (size_t size);
extern void oss_kmem_free (void *addr);
extern void *oss_pmalloc (size_t sz);
extern oss_native_word oss_virt_to_bus (void *addr);
extern void oss_reserve_pages (oss_native_word start_addr,
			       oss_native_word end_addr);
extern void oss_unreserve_pages (oss_native_word start_addr,
				 oss_native_word end_addr);
extern void *oss_contig_malloc (oss_device_t * osdev, int sz,
				oss_uint64_t memlimit,
				oss_native_word * phaddr);
extern void oss_contig_free (oss_device_t * osdev, void *p, int sz);

extern time_t oss_get_time (void);

typedef struct _inode_handle_t oss_inode_handle_t;
typedef struct _vm_aread_handle oss_vm_area_handle_t;

extern int oss_vma_get_flags (oss_vm_area_handle_t *);

typedef struct oss_file_operation_handle
{
  ssize_t (*read) (oss_file_handle_t *, char *, size_t, loff_t *);
  ssize_t (*write) (oss_file_handle_t *, char *, size_t, loff_t *);
  int (*readdir) (oss_inode_handle_t *, oss_file_handle_t *, void *, int);
  unsigned int (*poll) (oss_file_handle_t *, oss_poll_table_handle_t *);
  int (*ioctl) (oss_inode_handle_t *, oss_file_handle_t *, unsigned int,
		unsigned long);
  int (*mmap) (oss_file_handle_t *, oss_vm_area_handle_t *);
  int (*open) (oss_inode_handle_t *, oss_file_handle_t *);
  int (*release) (oss_inode_handle_t *, oss_file_handle_t *);
  long (*compat_ioctl) (oss_file_handle_t *, unsigned int, unsigned long);
  long (*unlocked_ioctl) (oss_file_handle_t *, unsigned int, unsigned long);
}
oss_file_operation_handle_t;

extern int oss_do_mmap (oss_vm_area_handle_t * vma,
			oss_native_word dmabuf_phys, int bytes_in_use);
extern int oss_register_chrdev (oss_device_t * osdev, unsigned int major,
				const char *name,
				oss_file_operation_handle_t * op);
extern void oss_register_minor (int major, int minor, char *name);
extern int oss_unregister_chrdev (unsigned int major, const char *name);

extern int oss_inode_get_minor (oss_inode_handle_t * inode);
extern int oss_file_get_flags (oss_file_handle_t * file);
extern void *oss_file_get_private (oss_file_handle_t * file);
extern void oss_file_set_private (oss_file_handle_t * file, void *v);

extern void oss_inc_refcounts (void);
extern void oss_dec_refcounts (void);

/*
 * Redefinitions of some routines defined in oss_config.h
 * just to ensure they are defined in the same way in both places. The
 * osscore/wrapper modules only include wrap.h so they can't see the "official"
 * declarations in oss_config.h.
 */

typedef struct _dev_info_t dev_info_t;
extern dev_info_t *oss_create_pcidip (struct pci_dev *pcidev);
extern oss_device_t *osdev_create (dev_info_t * dip, int dev_type,
				   int instance, const char *nick,
				   const char *handle);

extern void osdev_delete (oss_device_t * osdev);

/*
 * PCI config space access (in osscore.c)
 */
extern const char *oss_pci_read_devpath (dev_info_t * dip);
extern int osscore_pci_read_config_byte (dev_info_t * dip, unsigned int where,
					 unsigned char *val);
extern int osscore_pci_read_config_irq (dev_info_t * dip, unsigned int where,
					unsigned char *val);
extern int osscore_pci_read_config_word (dev_info_t * dip, unsigned int where,
					 unsigned short *val);
extern int osscore_pci_read_config_dword (dev_info_t * dip,
					  unsigned int where,
					  unsigned int *val);
extern int osscore_pci_write_config_byte (dev_info_t * dip,
					  unsigned int where,
					  unsigned char val);
extern int osscore_pci_write_config_word (dev_info_t * dip,
					  unsigned int where,
					  unsigned short val);
extern int osscore_pci_write_config_dword (dev_info_t * dip,
					   unsigned int where,
					   unsigned int val);

extern int osscore_pci_enable_msi (dev_info_t * dip);

extern void *oss_map_pci_mem (oss_device_t * osdev, int size,
			      unsigned long offset);

extern void oss_unmap_pci_mem (void *addr);

extern int oss_copy_to_user (void *to, const void *from, unsigned long n);
extern int oss_copy_from_user (void *to, const void *from, unsigned long n);

extern void oss_register_module (struct module *mod);
extern void oss_unregister_module (struct module *mod);

#ifdef _KERNEL
extern char *oss_strcpy (char *s1, const char *s2);
#undef strcpy
#define strcpy oss_strcpy

extern void *oss_memcpy (void *t_, const void *f_, size_t l);
#undef memcpy
#define memcpy oss_memcpy

extern void *oss_memset (void *t, int val, size_t l);
#undef memset
#define memset oss_memset

extern int oss_strcmp (const char *s1, const char *s2);
#undef strcmp
#define strcmp oss_strcmp
extern int oss_strncmp (const char *s1, const char *s2, size_t len);
#undef strncmp
#define strncmp oss_strncmp

#undef strlen
#define strlen oss_strlen

#undef strncpy
#define strncpy oss_strncpy
#endif // KERNEL

#undef timeout
#define timeout oss_timeout

#undef untimeout
#define untimeout oss_untimeout

#define drv_usecwait oss_udelay

#define uiomove oss_uiomove

#define cmn_err oss_cmn_err

struct fileinfo
{
  int mode;			/* Open mode */
  int acc_flags;
};
#define ISSET_FILE_FLAG(fileinfo, flag)  (fileinfo->acc_flags & (flag) ? 1:0)

/*
 * USB related definitions
 */
typedef struct udi_usb_devc udi_usb_devc;

/*
 * Functions exported by os.c
 */
extern int oss_init_osscore (oss_device_t * osdev);
extern void oss_uninit_osscore (oss_device_t * osdev);
extern void osdev_set_owner (oss_device_t * osdev, struct module *owner);
extern void osdev_set_major (oss_device_t * osdev, int major);
extern void osdev_set_irqparms (oss_device_t * osdev, void *irqparms);
extern void *osdev_get_irqparms (oss_device_t * osdev);
extern void oss_inc_intrcount(oss_device_t *osdev, int claimed);
extern struct module *osdev_get_owner (oss_device_t * osdev);
extern char *osdev_get_nick (oss_device_t * osdev);
extern int osdev_get_instance (oss_device_t * osdev);
extern int oss_request_major (oss_device_t * osdev, int major, char *module);
extern int oss_register_device (oss_device_t * osdev, const char *name);	/* from oss_config.h */

