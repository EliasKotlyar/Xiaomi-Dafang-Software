static char * fnsp = " -fno-stack-protector";

#define HAVE_SYSDEP
#define HAVE_KERNEL_FLAGS
static void
check_sysdep (conf_t * conf, struct utsname *un)
{
  char *p;

  char cmd[512];

#ifndef __arm__
  vmix_mode = "FLOAT";
#else
  exact_architectures = 1; /* Compile only the drivers with targetcpu=arm */
#endif

  if ((p=getenv("CROSSCOMPILE"))!=NULL)
     {
	if (strcmp(p, "uclinux-blackfin")==0)
	   {
		hostcc="cc";
		targetcc="uclinux-blackfin-gcc";
	   }
#ifdef VXWORKS
	else if (strcmp(p, "vxworks-x86")==0)
	   {
		vxworks_setup(conf, "386");
	   }
#endif
     }

  /*
   * Check if cc supports -fno-stack-protector
   */
  sprintf (cmd, "%s -c -o srcconf.o -fno-stack-protector setup/srcconf.c >/dev/null 2>&1", targetcc);
  if (system(cmd))
    {
      fnsp = "";
    }

  if (do_warning_checks)
    strcpy(conf->OSflags, "-Wall");
  strcpy (conf->ccomp, targetcc);
  strcpy (conf->cplusplus, "g++ -fno-rtti -fno-exceptions -I.");
  strcpy (conf->platform, "i86pc");
}

static void
add_kernel_flags (FILE * f)
{
# if defined(__x86_64__)
  fprintf (f,
	     "CFLAGS += -O3 -fno-common -mcmodel=kernel  -mno-red-zone -fno-pie  -fno-asynchronous-unwind-tables -ffreestanding%s\n", fnsp);
# else
#   ifndef __arm__
  if (getenv ("NO_REGPARM") == NULL)
	{
	  fprintf (f,
		   "CFLAGS += -O3 -fno-common -ffreestanding -mregparm=3 -DUSE_REGPARM%s\n", fnsp);
	}
  else
	{
	  fprintf (f, "CFLAGS += -O3 -fno-common -ffreestanding -DNO_REGPARM%s\n", fnsp);
	}
#   else
  fprintf (f, "CFLAGS += -O3 -fno-common -ffreestanding%s\n", fnsp);
#   endif
# endif
}
