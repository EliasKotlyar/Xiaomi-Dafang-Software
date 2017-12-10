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

/*#define USE_GCC*/
#define HAVE_SYSDEP
/* #define HAVE_KERNEL_FLAGS */
#include <sys/systeminfo.h>
static void
check_sysdep (conf_t * conf, struct utsname *un)
{
  char tmp[64], *p;

  conf->flags |= F_USEARCH;
#ifdef USE_GCC
  strcpy (conf->ccomp, "gcc -Wall -O");
#else

# if 0
// Using -fast seems to cause crash if OSS is compiled with certain version of Sun Studio. 
// The produced binary uses some instructions that are not permitted in kernel mode. For this reason
// -fast cannot be used at this moment.

  if (getenv("PORTABLE_BUILD") != NULL)
     strcpy (conf->ccomp, "cc -xO5");	/* Produce code for generic target */
  else
     strcpy (conf->ccomp, "cc -fast");	/* Optimize for the current system/arch */
# else
  strcpy (conf->ccomp, "cc -xO5");	/* Produce code for generic target */
# endif 

  shlib_cflags = "-Bdynamic -Kpic";
  shlib_ldflags = "-G -z text";
#endif

  extra_libraries = "-lnsl -lsocket -lresolv";
  strcpy (conf->cplusplus,
	  "CC -DNO_BUILTINS -I. -features=no%except -xvector=no");

  if (sysinfo (SI_ARCHITECTURE, tmp, sizeof (tmp)) == -1)
    {
      perror ("sysinfo SI_ARCHITECTURE");
      exit (-1);
    }
  printf ("Arch: %s\n", tmp);

  if (getenv ("SOL9") != NULL)
    strcat (conf->OSflags, "-DSOL9 ");

  if (*arch)
    strcpy (tmp, arch);
  printf ("Arch K: %s\n", tmp);
  strcpy (conf->arch, tmp);

#ifdef USE_GCC
  if (strcmp (tmp, "amd64") == 0)
    {
      strcpy (conf->OSflags, "-m64 -fno-common  -mcmodel=kernel -mno-red-zone");
      strcpy (conf->platform, "i86pc");
    }
  else if (strcmp (tmp, "sparcv9") == 0)
    {
      strcpy (conf->OSflags, "-m64");
      strcpy (conf->platform, "sparc");
    }
  else if (strcmp (tmp, "sparc") == 0)
    {
      strcpy (conf->OSflags, "-Dsparc32");
      strcpy (conf->platform, "sparc");
    }
  else if (strcmp (tmp, "i386") == 0)
    {
      strcpy (conf->OSflags, "-m32");
      strcpy (conf->platform, "i86pc");
    }
  else
    {
      strcpy (conf->OSflags, "-xarch=$(ARCH)");
      fprintf (stderr, "Cannot determine platform for '%s'\n", tmp);
      exit (-1);
    }
#else
  if (strcmp (tmp, "amd64") == 0)
    {
#if (__SUNPRO_C >= 0x590)
       strcpy (conf->OSflags, "-m64 -xmodel=kernel");
#else
       strcpy (conf->OSflags, "-xarch=$(ARCH) -xmodel=kernel");
#endif
         strcpy (conf->platform, "i86pc");
      strcpy (conf->platform, "i86pc");
    }
  else if (strcmp (tmp, "sparcv9") == 0)
    {
      strcpy (conf->OSflags, "-xarch=v9 -dalign");
      strcpy (conf->platform, "sparc");
    }
  else if (strcmp (tmp, "sparc") == 0)
    {
      strcpy (conf->OSflags, "-Dsparc32");
      strcpy (conf->platform, "sparc");
    }
  else if (strcmp (tmp, "i386") == 0)
    {
#if (__SUNPRO_C >= 0x590)
      strcpy (conf->OSflags, "-m32");
#else
      strcpy (conf->OSflags, "-xarch=386");
#endif
      strcpy (conf->platform, "i86pc");
    }
  else
    {
      strcpy (conf->OSflags, "-xarch=$(ARCH)");
      fprintf (stderr, "Cannot determine platform for '%s'\n", tmp);
      exit (-1);
    }
#endif

  if (getenv ("SOL9") != NULL)
    strcat (conf->OSflags, " -DSOL9");

  if ((p=getenv("CROSSCOMPILE"))!=NULL)
     {
	if (strcmp(p, "vxworks-x86")==0)
	   {
		vxworks_setup(conf, "386");
	   }
     }
}

static void
add_kernel_flags (FILE * f)
{
#if 0
  /* fprintf(f, "CFLAGS += -xmodel=kernel\n"); */
#endif
}
