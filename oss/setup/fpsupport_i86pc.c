#ifndef lint
/*
 * Floating point support routines for x86 and x86_64
 *
 * Copyright (C) 4Front Technologies 2006. Released under GPLv2/CDDL.
 *
 * Must be compiled to .asm with gcc -S -O6.
 */
#define BLOCK_INTERRUPTS

#define FP_SAVE(envbuf)		asm ("fnsave %0":"=m" (*envbuf));
#define FP_RESTORE(envbuf)		asm ("frstor %0":"=m" (*envbuf));

/* SSE/SSE2 compatible macros */
#define FX_SAVE(envbuf)		asm ("fxsave %0":"=m" (*envbuf));
#define FX_RESTORE(envbuf)		asm ("fxrstor %0":"=m" (*envbuf));

#if defined(amd64) || defined(__x86_64__)
#define ARCH64
#endif

static int old_arch = 0;	/* No SSE/SSE2 instructions */

/*
 * Generic CPUID function
 * clear %ecx since some cpus (Cyrix MII) do not set or clear %ecx
 * resulting in stale register contents being returned.
 */
static inline void
cpuid (int op, int *eax, int *ebx, int *ecx, int *edx)
{
__asm__ ("cpuid": "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx):"0" (op), "c"
	   (0));
}

#ifdef ARCH64
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
#ifdef ARCH64
  asm volatile ("movq %%cr0,%0":"=r" (cr0));
#else
  asm volatile ("movl %%cr0,%0":"=r" (cr0));
#endif
  return cr0;
}

static inline void
write_cr0 (unsigned long val)
{
#ifdef ARCH64
  asm volatile ("movq %0,%%cr0"::"r" (val));
#else
  asm volatile ("movl %0,%%cr0"::"r" (val));
#endif
}

static inline unsigned long
read_cr4 (void)
{
  unsigned long cr4;
#ifdef ARCH64
  asm volatile ("movq %%cr4,%0":"=r" (cr4));
#else
  asm volatile ("movl %%cr4,%0":"=r" (cr4));
#endif
  return cr4;
}

static inline void
write_cr4 (unsigned long val)
{
#ifdef ARCH64
  asm volatile ("movq %0,%%cr4"::"r" (val));
#else
  asm volatile ("movl %0,%%cr4"::"r" (val));
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
/*
 * oss_fp_check returns 1 if the CPU architecture supports floating point
 * in kernel space. Otherwise 0 will be returned.
 */
  int eax, ebx, ecx, edx;
#define FLAGS_ID (1<<21)

#ifdef ARCH64
  unsigned long long flags_reg;
#else
  unsigned long flags_reg;
#endif

/*
 * First probe if the CPU supports CPUID by checking if the ID bit
 * can be changed.
 */

  local_save_flags (flags_reg);
  flags_reg &= ~FLAGS_ID;	/* Clear */
  local_restore_flags (flags_reg);

  local_save_flags (flags_reg);
  if (flags_reg & FLAGS_ID)
    return -1;

  flags_reg |= FLAGS_ID;	/* Set */
  local_restore_flags (flags_reg);

  local_save_flags (flags_reg);
  if (!(flags_reg & FLAGS_ID))
    return -2;

/*
 * Now we know that the CPU supports CPUID. Ensure that FXSAVE and FXRSTOR
 * are supported.
 */
#define CPUID_FXSR	(1<<24)
#define CPUID_SSE	(1<<25)
#define CPUID_SSE2	(1<<26)

  cpuid (1, &eax, &ebx, &ecx, &edx);

  if (!(edx & CPUID_FXSR))
    return -3;			/* No */

  /*
   * Older machines require different FP handling. Use the SSE
   * instruction set as an indicator.
   */
  if (!(edx & CPUID_SSE))
    old_arch = 1;		/* No */

  return 1;
}

#define local_irq_disable() 	asm volatile("cli": : :"memory")

void
oss_fp_save (short *envbuf, unsigned int flags[])
{
/*
 * oss_fp_save saves the floating point environment (registers) and
 * enables floating point operations if necessary.
 */

#ifdef BLOCK_INTERRUPTS
#  ifdef ARCH64
  unsigned long long flags_reg;
#  else
  unsigned long flags_reg;
#  endif
  local_save_flags (flags_reg);
  flags[3] = flags_reg;
  local_irq_disable ();
#endif

  /*
   * Prepare the FP related control register bits to disable all kind of
   * FP related exceptions.
   */
  flags[0] = read_cr0 ();
  write_cr0 (flags[0] & ~0x0e);	/* Clear CR0.TS/EM/MP */

  /*
   * Save FPU/SSE/XMM registers and init the hardware.
   */
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
/*
 * oss_fp_restore reverses any changes made by oss_fp_save and restores
 * the floating point environment (registers) back to the original state.
 */
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
#ifdef BLOCK_INTERRUPTS
  local_restore_flags (flags[3]);
#endif
}
#else
int
oss_fp_check (void)
{
  return 0;
}
#endif
