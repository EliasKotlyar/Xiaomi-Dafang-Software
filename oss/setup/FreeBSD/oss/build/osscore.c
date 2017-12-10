/*
 * Purpose: OSS core functions that need to be compiled in the target system
 *
 * Some parts of the FreeBSD operating system interface of OSS are sensitive
 * to changes in internal structures of FreeBSD. For this reason these
 * files have to be compiled in the target system when OSS is installed.
 * In this way the same OSS binary package can be used with several FreeBSD
 * versions.
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
#include <sys/types.h>
#include <sys/errno.h>
#include <sys/param.h>		/* defines used in kernel.h */
#include <dev/pci/pcivar.h>	/* For pci_get macros! */
#include <dev/pci/pcireg.h>
#include <machine/intr_machdep.h>

#include <vm/vm.h>
#include <vm/pmap.h>
#include <sys/proc.h>

typedef struct _oss_device_t oss_device_t;
#include "bsddefs.h"

/* The PCIBIOS_* defines must match oss_pci.h */
#define PCIBIOS_SUCCESSFUL		0x00
#define PCIBIOS_FAILED			-1

extern int soundcard_attach (void);
extern int soundcard_detach (void);

void *
memset (void *t, int val, int l)
{
  char *c = t;
  while (l-- > 0)
    *c++ = val;

  return t;
}

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

  strcpy (tmp, "osscore: ");
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

void
oss_udelay (unsigned long t)
{
  DELAY (t);
}

typedef struct
{
  int irq;
  oss_device_t *osdev;
  oss_tophalf_handler_t top;
  oss_bottomhalf_handler_t bottom;
  struct resource *irqres;
  int irqid;
  void *cookie;
} osscore_intr_t;

#define MAX_INTRS	32

static osscore_intr_t intrs[MAX_INTRS] = { {0} };
static int nintrs = 0;

static void
ossintr (void *arg)
{
  osscore_intr_t *intr = arg;
  int serviced = 0;

  if (intr->top)
    serviced = intr->top (intr->osdev);
  if (intr->bottom)
    intr->bottom (intr->osdev);
  oss_inc_intrcount (intr->osdev, serviced);
}

int
oss_register_interrupts (oss_device_t * osdev, int intrnum,
			 oss_tophalf_handler_t top,
			 oss_bottomhalf_handler_t bottom)
{

  osscore_intr_t *intr;
  char name[32];

  if (nintrs >= MAX_INTRS)
    {
      cmn_err (CE_CONT,
	       "oss_register_interrupts: Too many interrupt handlers\n");
      return -ENOMEM;
    }

  intr = &intrs[nintrs];

  intr->irq = 0;
  intr->osdev = osdev;
  intr->top = top;
  intr->bottom = bottom;

  sprintf (name, "%s%d", osdev->nick, osdev->instance);

  intr->irqid = 0;
  intr->irqres = bus_alloc_resource (osdev->dip, SYS_RES_IRQ, &(intr->irqid),
				     0, ~0, 1, RF_SHAREABLE | RF_ACTIVE);
  if (intr->irqres == NULL)
    {
      cmn_err (CE_CONT,
	       "oss_register_interrupts: bus_alloc_resource failed.\n");
      return -EIO;
    }

  intr->irq = bus_setup_intr (osdev->dip, intr->irqres,
		              INTR_TYPE_AV | INTR_MPSAFE,
#if __FreeBSD_version >= 700031
			      NULL,
#endif
			      ossintr, intr, &(intr->cookie));

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
	osscore_intr_t *intr;

	intr = &intrs[i];
	bus_teardown_intr (osdev->dip, intr->irqres, intr->cookie);
	bus_release_resource (osdev->dip, SYS_RES_IRQ, intr->irqid,
			      intr->irqres);
      }
}

/*
 * PCI config space access
 */
char *
oss_pci_read_devpath (dev_info_t * dip)
{
  return "Unknown PCI path";	// TODO
}

int
pci_read_config_byte (oss_device_t * osdev, offset_t where,
		      unsigned char *val)
{
  *val = pci_read_config (osdev->dip, where, 1);
  return PCIBIOS_SUCCESSFUL;
}

int
pci_read_config_irq (oss_device_t * osdev, offset_t where, unsigned char *val)
{
  *val = pci_read_config (osdev->dip, where, 1);
  return PCIBIOS_SUCCESSFUL;
}

int
pci_read_config_word (oss_device_t * osdev, offset_t where,
		      unsigned short *val)
{
  *val = pci_read_config (osdev->dip, where, 2);
  return PCIBIOS_SUCCESSFUL;
}

int
pci_read_config_dword (oss_device_t * osdev, offset_t where,
		       unsigned int *val)
{
  *val = pci_read_config (osdev->dip, where, 4);
  return PCIBIOS_SUCCESSFUL;
}

int
pci_write_config_byte (oss_device_t * osdev, offset_t where,
		       unsigned char val)
{
  pci_write_config (osdev->dip, where, val, 1);
  return PCIBIOS_FAILED;
}

int
pci_write_config_word (oss_device_t * osdev, offset_t where,
		       unsigned short val)
{
  pci_write_config (osdev->dip, where, val, 2);
  return PCIBIOS_FAILED;
}

int
pci_write_config_dword (oss_device_t * osdev, offset_t where,
			unsigned int val)
{
  pci_write_config (osdev->dip, where, val, 4);
  return PCIBIOS_FAILED;
}

void *
oss_contig_malloc (unsigned long buffsize, unsigned long memlimit,
		   oss_native_word * phaddr)
{
  char *tmpbuf;
  *phaddr = 0;

  tmpbuf =
    (char *) contigmalloc (buffsize, M_DEVBUF, M_WAITOK, 0ul, memlimit,
			   PAGE_SIZE, 0ul);
  if (tmpbuf == NULL)
    {
      cmn_err (CE_CONT, "OSS: Unable to allocate %lu bytes for a DMA buffer\n",
	       buffsize);
      cmn_err (CE_CONT, "run soundoff and run soundon again.\n");
      return NULL;
    }
  *phaddr = vtophys (tmpbuf);
  return tmpbuf;
}

void
oss_contig_free (void *p, unsigned long sz)
{
  if (p)
    contigfree (p, sz, M_DEVBUF);
}

/* 
 * Load handler that deals with the loading and unloading of a KLD.
 */

static int
osscore_loader (struct module *m, int what, void *arg)
{
  int err = 0;

  switch (what)
    {
    case MOD_LOAD:		/* kldload */
      return soundcard_attach ();
      break;
    case MOD_UNLOAD:
      return soundcard_detach ();
      break;
    default:
      err = EINVAL;
      break;
    }
  return (err);
}

/* Declare this module to the rest of the kernel */

static moduledata_t osscore_mod = {
  "osscore",
  osscore_loader,
  NULL
};

#define _FP_SAVE(envbuf)		asm ("fnsave %0":"=m" (*envbuf));
#define _FP_RESTORE(envbuf)		asm ("frstor %0":"=m" (*envbuf));

/* SSE/SSE2 compatible macros */
#define FX_SAVE(envbuf)		asm ("fxsave %0":"=m" (*envbuf));
#define FX_RESTORE(envbuf)		asm ("fxrstor %0":"=m" (*envbuf));

static int old_arch = 0;	/* No SSE/SSE2 instructions */

#define asm __asm__

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
asm ("stmxcsr %0":"=m" (mxcsr));
  return mxcsr;
}

static inline void
write_mxcsr (unsigned long long val)
{
  asm ("ldmxcsr %0"::"m" (val));
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

#define OSS_CPUID_FXSR	(1<<24)
#define OSS_CPUID_SSE	(1<<25)
#define OSS_CPUID_SSE2	(1<<26)

  cpuid (1, &eax, &ebx, &ecx, &edx);

  if (!(edx & OSS_CPUID_FXSR))
    return 0;

  /*
   * Older machines require different FP handling than the latest ones. Use the SSE
   * instruction set as an indicator.
   */
  if (!(edx & OSS_CPUID_SSE))
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
      _FP_SAVE (envbuf);
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
      _FP_RESTORE (envbuf);
    }
  else
    {
      FX_RESTORE (envbuf);
      write_cr4 (flags[1]);	/* Restore cr4 */
    }
  write_cr0 (flags[0]);		/* Restore cr0 */
}

#ifdef VDEV_SUPPORT
static void
oss_file_free_private (void *v)
{
  free (v, M_DEVBUF);
}

int
oss_file_get_private (void **v)
{
  int error;

  error = devfs_get_cdevpriv (v);
  if (error)
    {
      cmn_err (CE_CONT, "Couldn't retrieve private data from file handle!\n");
      return error;
    }
  return 0;
}

int
oss_file_set_private (struct thread *t, void *v, size_t l)
{
  int error;
  void * p;

  p = malloc (l, M_DEVBUF, M_WAITOK);
  if (p == NULL)
    {
      cmn_err (CE_CONT, "Couldn't allocate memory!\n");
      return -1;
    }
  memcpy (p, v, l);
  error = devfs_set_cdevpriv (p, oss_file_free_private);
  if (error)
    {
      cmn_err (CE_CONT, "Couldn't attach private data to file handle!\n");
      oss_file_free_private (p);
      return error;
    }
  return 0;
}
#endif

int
oss_get_uid(void)
{
  return curthread->td_ucred->cr_uid;
}

extern int max_intrate;
extern int detect_trace;
extern int src_quality;
extern int flat_device_model;
extern int vmix_disabled;
extern int vmix_loopdevs;
extern int vmix_no_autoattach;
extern int ac97_amplifier;
extern int ac97_recselect;
extern int cooked_enable;
extern int dma_buffsize;
extern int excl_policy;
extern int mixer_muted;
TUNABLE_INT("osscore.max_intrate", &max_intrate);
TUNABLE_INT("osscore.detect_trace", &detect_trace);
TUNABLE_INT("osscore.src_quality", &src_quality);
TUNABLE_INT("osscore.flat_device_model", &flat_device_model);
TUNABLE_INT("osscore.vmix_disabled", &vmix_disabled);
TUNABLE_INT("osscore.vmix_loopdevs", &vmix_loopdevs);
TUNABLE_INT("osscore.vmix_no_autoattach", &vmix_no_autoattach);
TUNABLE_INT("osscore.ac97_amplifier", &ac97_amplifier);
TUNABLE_INT("osscore.ac97_recselect", &ac97_recselect);
TUNABLE_INT("osscore.cooked_enable", &cooked_enable);
TUNABLE_INT("osscore.dma_buffsize", &dma_buffsize);
TUNABLE_INT("osscore.excl_policy", &excl_policy);
TUNABLE_INT("osscore.mixer_muted", &mixer_muted);

DECLARE_MODULE (osscore, osscore_mod, SI_SUB_KLD, SI_ORDER_ANY);
MODULE_VERSION (osscore, 4);
