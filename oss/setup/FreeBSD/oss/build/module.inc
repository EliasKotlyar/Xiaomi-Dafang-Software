/*
 * Purpose: Generic OSS driver module interface for FreeBSD
 *
 * This file is included by the driver modules when they are compiled
 * in the target system. In this way this code can be changed for non-srandard
 * kernels. Compiling this file in the target file makes it also possible
 * to distribute single OSS binary package that works under as many
 * FreeBSD versions as possible.
 */
/*
 * Copyright (C) 4Front Technologies 2005-2007. Released under BSD license.
 */

#include <machine/stdarg.h>
#include <sys/param.h>		/* defines used in kernel.h */
#include <sys/module.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/kernel.h>		/* types used in module initialization */
#include <sys/conf.h>		/* cdevsw struct */
#include <sys/uio.h>		/* uio struct */
#include <sys/malloc.h>

#include <sys/bus.h>		/* structs, prototypes for pci bus stuff */
#include <machine/bus.h>
#include <sys/rman.h>
#include <machine/resource.h>

#include <timestamp.h>
#include <oss_exports.h>
#include "bsddefs.h"

void
cmn_err (int level, char *s, ...)
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

  strcpy (tmp, DRIVER_NICK ": ");
  sprintf (tmp + strlen (tmp), s, a[0], a[1], a[2], a[3], a[4], a[5], NULL,
	   NULL, NULL, NULL);
  if (level == CE_PANIC)
    panic ("%s", tmp);
  printf ("%s", tmp);
#if 0
  /* This may cause a crash under SMP */
  if (sound_started)
    store_msg (tmp);
#endif

  va_end (ap);
}

extern int DRIVER_ATTACH (oss_device_t * osdev);
extern int DRIVER_DETACH (oss_device_t * osdev);

#ifdef DEVTYPE_VMIX
#define TYPE_OK
#include "bsdvirtual.inc"
#endif

#ifdef DEVTYPE_PCI
#define TYPE_OK
#include "bsdpci.inc"
#endif

#ifdef DEVTYPE_VIRTUAL
#define TYPE_OK
#include "bsdvirtual.inc"
#endif

#ifndef TYPE_OK
#error Unrecognized driver type
#endif

MODULE_DEPEND (DRIVER_NAME, osscore, 4, 4, 4);
