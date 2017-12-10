/*
 * Purpose: Definitions for routines and variables exported by osscore.c
 *
 * Do not make any modifications to these settings because OSS core modules
 * have been compiled against them. Full rebuild of OSS will be required if 
 * this file is changed.
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

#include <sys/types.h>
#if 0 /* __FreeBSD_version >= 700031 */
/* Some crashes have been reported with SX on 7-STABLE/8-CURRENT:
 * http://4front-tech.com/forum/viewtopic.php?t=2718
 * http://4front-tech.com/forum/viewtopic.php?t=2563
 */
#define USE_SX_LOCK	1
#endif
#undef VDEV_SUPPORT
#if __FreeBSD_version >= 700111
#define VDEV_SUPPORT
extern int oss_file_set_private (struct thread *p, void *v, size_t l);
extern int oss_file_get_private (void **v);
#endif
extern int oss_get_uid (void);

typedef struct device dev_info_t;
typedef long long oss_int64_t;			/* Signed 64 bit integer */
typedef unsigned long long oss_uint64_t;	/* Unsigned 64 bit integer */
typedef unsigned long offset_t;

/*
 * Some integer types
 */
#if defined(__amd64__)
typedef unsigned long long oss_native_word;	/* Same as the address and status register size */
#else
typedef unsigned long oss_native_word;	/* Same as the address and status register size */
#endif

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
  char *hw_info;
  int major;
  char nick[16];
  char modname[16];
  char handle[32];
  int num_audio_engines;
  int num_audioplay, num_audiorec, num_audioduplex;
  int num_mididevs;
  int num_mixerdevs;
  int num_loopdevs;
  int first_mixer;	/* This must be set to -1 by osdev_create() */

  int intrcount;
  int ackcount;
  volatile int refcount;	/* Nonzero means that the device is needed by some other (virtual) driver. */

};

extern void cmn_err (int level, char *format, ...);
#define CE_CONT		0
#define CE_NOTE		1
#define CE_WARN		2
#define CE_PANIC	3

#ifdef USE_SX_LOCK
typedef struct sx *oss_mutex_t;
#else
typedef struct mtx *oss_mutex_t;
#endif

typedef int ddi_iblock_cookie_t;

extern void oss_udelay (unsigned long t);

#ifdef _KERNEL
#define memset oss_memset
extern void *oss_memset (void *t, int val, int l);
#endif

extern oss_device_t *osdev_create (dev_info_t * dip, int dev_type,
				   int instance, const char *nick,
				   const char *handle);
extern void osdev_delete (oss_device_t * osdev);

extern char *oss_pci_read_devpath (dev_info_t * dip);
extern int pci_read_config_byte (oss_device_t * osdev, offset_t where,
				 unsigned char *val);
extern int pci_read_config_irq (oss_device_t * osdev, offset_t where,
				unsigned char *val);
extern int pci_read_config_word (oss_device_t * osdev, offset_t where,
				 unsigned short *val);
extern int pci_read_config_dword (oss_device_t * osdev, offset_t where,
				  unsigned int *val);
extern int pci_write_config_byte (oss_device_t * osdev, offset_t where,
				  unsigned char val);
extern int pci_write_config_word (oss_device_t * osdev, offset_t where,
				  unsigned short val);
extern int pci_write_config_dword (oss_device_t * osdev, offset_t where,
				   unsigned int val);
#ifndef OSS_CONFIG_H
/* These definitions must match with oss_config.h */
typedef int (*oss_tophalf_handler_t) (struct _oss_device_t * osdev);
typedef void (*oss_bottomhalf_handler_t) (struct _oss_device_t * osdev);
#endif

extern int oss_register_interrupts (oss_device_t * osdev, int intrnum,
				    oss_tophalf_handler_t top,
				    oss_bottomhalf_handler_t bottom);
extern void oss_unregister_interrupts (oss_device_t * osdev);

extern void *oss_contig_malloc (unsigned long sz, unsigned long memlimit,
				oss_native_word * phaddr);
extern void oss_contig_free (void *p, unsigned long sz);

extern void oss_register_module (char *name);
extern void oss_unregister_module (char *name);
extern void *oss_find_minor_info (int dev_class, int instance);
extern int oss_find_minor (int dev_class, int instance);
extern void oss_inc_intrcount (oss_device_t * osdev, int claimed);

#define FP_SUPPORT

#ifdef FP_SUPPORT
typedef short fp_env_t[512];
typedef unsigned int fp_flags_t[4];
extern int oss_fp_check (void);
extern void oss_fp_save (short *envbuf, fp_flags_t flags);
extern void oss_fp_restore (short *envbuf, fp_flags_t flags);
#   define FP_SAVE(envbuf, flags)		oss_fp_save(envbuf, flags)
#   define FP_RESTORE(envbuf, flags)		oss_fp_restore(envbuf, flags)
#endif
