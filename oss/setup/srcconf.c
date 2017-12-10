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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <errno.h>

#define MAX_SUBDIRS	64
#define MAX_OBJECTS	64
#define MAX_INCLUDES	32
#define MAXOPTS		64

static char *vmix_mode="FIXEDPOINT";
static char *config_midi="ENABLED"; // Actually this depends on the configure script

static int exact_architectures=0; /* 1=Compile only drivers that have matching arch given in their .config file. */

static int config_phpmake=0;

typedef struct
{
  char project_name[64];
  char os_name[64];
  int mode;
#define MD_UNDEF	0
#define MD_USERLAND	1
#define MD_SBIN		2
#define MD_KERNEL	3
#define MD_KERNEL_	4
#define MD_MODULE_	5
#define MD_MODULE	6
#define MD_SHLIB	7
  char cflags[256];
  char ldflags[256];
  char OSflags[256];
  int license;
#define LIC_FREE	0
#define LIC_RESTRICTED	1
  char bus[16];
  char endianess[16];
  char ccomp[256];
  char cplusplus[256];
  char system[32], arch[32], platform[32];

  unsigned int flags;
#define F_USEARCH		0x00000001


  int check_os, os_ok, os_bad;
  int check_cpu, cpu_ok, cpu_bad;
  int check_endian, endian_ok;
  int check_platform, platform_ok;

  int power_manage;	/* Supports device power management (under Solaris) */
  int suspend_resume;	/* Supports suspend/resume (under Solaris) */

  char *purpose;
} conf_t;

#define DEFAULT_CC "cc"

static conf_t conf = {
  "Open Sound System",
  "Solaris",
  MD_USERLAND,
  "",				/* cflags */
  "",				/* ldflags */
  "",				/* OSflags */
  LIC_FREE,
  "PCI",			/* bus */
  "LITTLE",			/* Endianess */
  DEFAULT_CC,			/* c compiler */
  "c++"				/* cplusplus */
};

static char this_os[64] = "kernel/OS/SunOS";
static int kernelonly = 0;
static int useronly = 0;
static int do_warning_checks=1;

static char *shlib_cflags = "-shared -fPIC";
static char *shlib_ldflags = "-shared -fPIC";

static char *extra_libraries = "";

char *hostcc;
char *targetcc;

static int nincludes = 0;
static char *includes[MAX_INCLUDES];
static int do_cleanup = 0;

static char arch[32] = "";

static void
generate_driver (char *name, conf_t * conf, char *cfg_name, char *cfg_header,
		 char *dirname, char *topdir);

typedef void
(*generate_driver_t) (char *name, conf_t * conf, char *cfg_name, char *cfg_header,
                 char *dirname, char *topdir);

generate_driver_t driver_gen = generate_driver;

#ifdef linux
#include "srcconf_vxworks.inc"
#include "srcconf_linux.inc"
#endif

#ifdef __FreeBSD__
#include "srcconf_freebsd.inc"
#endif

#ifdef sun
#include "srcconf_vxworks.inc"
#include "srcconf_solaris.inc"
#endif

#if defined(__BEOS__) || defined(__HAIKU__)
#include "srcconf_beos.inc"
#endif

static int
parse_config (FILE * f, conf_t * conf, char *comment)
{
  char line[256], *p, *parms;

  while (fgets (line, sizeof (line) - 1, f) != NULL)
    {
      p = line + strlen (line) - 1;
      if (*p == '\n')
	*p = 0;

      if (*line == '#')		/* Comment line */
	continue;

      parms = p = line;
      while (*parms && *parms != '=')
	parms++;

      if (*parms == '=')
	*parms++ = 0;

#if defined(__BEOS__) || defined(__HAIKU__)
      if (strcmp (parms, "-lm") == 0)
	{
	  parms = "";
	}
#endif

      if (strcmp (parms, "$GTKCFLAGS") == 0)
	{
	  parms = "";
	  if (getenv ("GTK1") != NULL)
	    parms = "`gtk-config --cflags` -DGTK1_ONLY";
	  else
	  if (getenv ("GTK2") != NULL)
	    parms = "`pkg-config gtk+-2.0 --cflags`";
	}

      if (strcmp (parms, "$GTKLDFLAGS") == 0)
	{
	  parms = "";
	  if (getenv ("GTK1") != NULL)
	    parms = "`gtk-config --libs`";
	  else
	  if (getenv ("GTK2") != NULL)
	    parms = "`pkg-config gtk+-2.0 --libs`";
	}

      if (strcmp (parms, "$DLOPENLDFLAGS") == 0)
	{
	  parms = "";
#ifndef __FreeBSD__
	  if (getenv ("OGG_SUPPORT") != NULL)
	    parms = "-ldl";
#endif
	}

      if (strcmp (parms, "$OGGDEFINE") == 0)
	{
	  parms = "";
	  if (getenv ("OGG_SUPPORT") != NULL)
	    parms = "-DOGG_SUPPORT";
	}

      if (strcmp (line, "project") == 0)
	{
	  strcpy (conf->project_name, parms);
	  continue;
	}

      if (strcmp (line, "cflags") == 0)
	{
	  strcpy (conf->cflags, parms);
	  continue;
	}

      if (strcmp (line, "ldflags") == 0)
	{
	  strcpy (conf->ldflags, parms);
	  continue;
	}

      if (strcmp (line, "osflags") == 0)
	{
	  if (*conf->OSflags)
	    strcat (conf->OSflags, " ");
	  strcat (conf->OSflags, parms);
	  continue;
	}

      if (strcmp (line, "bus") == 0)
	{
	  strcpy (conf->bus, parms);
	  continue;
	}

      if (strcmp (line, "OS") == 0)
	{
	  strcpy (conf->os_name, parms);
	  continue;
	}

      if (strcmp (line, "mode") == 0)
	{
	  if (strcmp (parms, "user") == 0)
	    {
	      conf->mode = MD_USERLAND;
	      continue;
	    }

	  if (strcmp (parms, "sbin") == 0)
	    {
	      conf->mode = MD_SBIN;
	      continue;
	    }

	  if (strcmp (parms, "shlib") == 0)
	    {
	      conf->mode = MD_SHLIB;
	      continue;
	    }

	  if (strcmp (parms, "kernel") == 0)
	    {
	      conf->mode = MD_KERNEL;
	      continue;
	    }

	  if (strcmp (parms, "undefined") == 0)
	    {
	      conf->mode = MD_UNDEF;
	      continue;
	    }

	  if (strcmp (parms, "kernelmode") == 0)
	    {
	      conf->mode = MD_KERNEL_;
	      continue;
	    }

	  if (strcmp (parms, "module") == 0)
	    {
	      conf->mode = MD_MODULE_;
	      continue;
	    }

	  fprintf (stderr, "Bad mode %s\n", parms);
	  exit (-1);
	}

      if (strcmp (line, "license") == 0)
	{
	  if (strcmp (parms, "free") == 0)
	    conf->license = LIC_FREE;
	  if (strcmp (parms, "restricted") == 0)
	    conf->license = LIC_RESTRICTED;
	  continue;
	}

      if (strcmp (line, "depends") == 0)
	{
	  char tmp[64];
	  sprintf (tmp, "HAVE_%s", parms);
	  if (getenv (tmp) == NULL)
	    {
	      printf
		("Directory depends on the %s package which is not available\n",
		 parms);

	      return 0;
	    }
	  continue;
	}

      if (strcmp (line, "configcheck") == 0)
	{
	  if (strcmp (parms, "VMIX") == 0)
	    {
	      if (strcmp(vmix_mode, "DISABLED")==0)
	         {
		      printf
			("Directory depends on the VMIX subsystem which is not enabled\n");
	
		      return 0;
	         }
	      continue;
	    }

	  if (strcmp (parms, "MIDI") == 0) // Skip if MIDI is disabled 
	    {
	      if (strcmp(config_midi, "DISABLED")==0)
	         {
		      printf
			("Directory depends on the MIDI subsystem which is not enabled\n");
	
		      return 0;
	         }
	      continue;
	    }

	  if (strcmp (parms, "NO_MIDI") == 0) // Skip if MIDI is enabled
	    {
	      if (strcmp(config_midi, "DISABLED")!=0)
	         {
			// printf ("Directory not compatible with MIDI subsystem (which is enabled)\n");
	
		      return 0;
	         }
	      continue;
	    }

	  fprintf (stderr, "Unknown configcheck parameter '%s'\n", parms);
	  exit(-1);
	}

      if (strcmp (line, "targetos") == 0)
	{
	  conf->check_os = 1;
	  if (strcmp (conf->system, parms) == 0)
	    conf->os_ok = 1;
	  continue;
	}

      if (strcmp (line, "forgetos") == 0)
	{
	  if (strcmp (conf->system, parms) == 0)
	    conf->os_bad = 1;
	  continue;
	}

      if (strcmp (line, "targetcpu") == 0)
	{
	  conf->check_cpu = 1;
	  if (strcmp (parms, "any") == 0)
	     conf->cpu_ok = 1;
	  else
	     if (strcmp (conf->arch, parms) == 0)
	        conf->cpu_ok = 1;
	  continue;
	}

      if (strcmp (line, "forgetcpu") == 0)
	{
	  if (strcmp (conf->arch, parms) == 0)
	    conf->cpu_bad = 1;
	  continue;
	}

      if (strcmp (line, "endian") == 0)
	{
	  conf->check_endian = 1;
	  if (strcmp (conf->endianess, parms) == 0)
	    conf->endian_ok = 1;
	  continue;
	}

      if (strcmp (line, "platform") == 0)
	{
	  conf->check_platform = 1;
	  if (strcmp (conf->platform, parms) == 0)
	    conf->platform_ok = 1;
	  continue;
	}

      if (strcmp (line, "pm_support") == 0)
	{
	    conf->power_manage=1;
	}

      if (strcmp (line, "suspend_resume") == 0)
	{
	    conf->suspend_resume=1;
	}

      if (strcmp (line, "force_endian") == 0)
	 {
	      if (strcmp (parms, "BIG") == 0)
	         {
	      	    strcpy (conf->endianess, "BIG");
		 }
	      else
	      if (strcmp (parms, "LITTLE") == 0)
	         {
	      	    strcpy (conf->endianess, "LITTLE");
		 }
	      else
	      if (strcmp (parms, "UNKNOWN") == 0)
	         {
	      	    strcpy (conf->endianess, "UNKNOWN");
		 }
	 }

      printf ("\t     %s\n", line);
      printf ("\t     ^\n");
      printf ("\t*** Unknown parameter ***\n");
    }

  if (conf->os_bad)
    {
      return 0;
    }

  if (conf->check_os && !conf->os_ok)
    {
      return 0;
    }

  if (conf->cpu_bad)
    {
      return 0;
    }

  if (conf->check_cpu && !conf->cpu_ok)
    {
      return 0;
    }

  if (conf->check_endian && !conf->endian_ok)
    {
      return 0;
    }

  if (conf->check_platform && !conf->platform_ok)
    {
      return 0;
    }

/*
 * Under some CPU architectures we should compile only the driver modules
 * that have proper targetcpu line in their .config file. It doesn't make any
 * sense to compile PCI drivers for architectures that don't have any PCI bus.
 */
  if (conf->mode == MD_MODULE && exact_architectures && !conf->check_cpu)
     {
	printf ("Ignoring %s - No CPU specified\n", comment);
     	return 0;
     }

  return 1;
}

#if defined(linux)
#include "gen_driver_linux.inc"
#endif

#if defined(__FreeBSD__)
#include "gen_driver_freebsd.inc"
#endif

#if defined(sun)
#include "gen_driver_solaris.inc"
#endif

#if defined(__SCO_VERSION__)
#include "gen_driver_sco.inc"
#endif

#if defined(__BEOS__) || defined(__HAIKU__)
#include "gen_driver_beos.inc"
#endif

static int
is_cplusplus (char *fname)
{
  while (*fname && *fname != '.')
    fname++;

  if (strcmp (fname, ".cpp") == 0)
    return 1;
  if (strcmp (fname, ".C") == 0)
    return 1;

  return 0;
}

static int
cmpstringp (const void *p1, const void *p2)
{
  /* The arguments to this function are "pointers to
   * pointers to char", but strcmp() arguments are "pointers
   * to char", hence the following cast plus dereference
   */

  /*
   * Make sure "lib" directories get compiles before any other
   * subdirectories.
   */

   if (strcmp(*(char **) p1, "lib")==0)
      return -1;
   else
      if (strcmp(*(char **) p2, "lib")==0)
	 return 1;

  return strcmp (*(char **) p1, *(char **) p2);
}

static int
scan_dir (char *path, char *name, char *topdir, conf_t * cfg, int level)
{
  int i;
  DIR *dir;
  struct dirent *de;
  struct stat st;
  char tmp[256];
  FILE *f;
  FILE *cf;
  char cfg_name[64];
  char cfg_header[64];
  int cfg_seen = 0;

  conf_t conf;

  int ndirs = 0;
  char *subdirs[MAX_SUBDIRS];

  int nobjects = 0;
  char *objects[MAX_OBJECTS], *obj_src[MAX_OBJECTS];
  int nsources = 0;
  char *sources[MAX_OBJECTS];
  char obj[128], *p, *suffix;
  char *objdir = "OBJDIR";
  int include_local_makefile = 0;

#define MAX_FILENAME 128
  char *filenames[MAX_FILENAME];
  int n_filenames = 0;

  char tmp_endian[100]="";
  char autogen_sources[1024]="";

  memcpy (&conf, cfg, sizeof (conf));

  if (conf.mode == MD_MODULE_)
    conf.mode = MD_MODULE;

  if (conf.mode == MD_KERNEL_)
    conf.mode = MD_KERNEL;

  sprintf (tmp, "%s/.name", path);
  if ((cf = fopen (tmp, "r")) != NULL)
    {
      char *p;

      if (fgets(tmp, sizeof(tmp)-1, cf)==NULL)
	 strcpy(tmp, name);
      fclose (cf);

      p=tmp+strlen(tmp)-1;
      if (*p=='\n')*p=0;

      conf.purpose=strdup(tmp);
    }
  else
    {
	    conf.purpose=strdup(name);
    }

  sprintf (tmp, "%s/.config", path);
  if ((cf = fopen (tmp, "r")) != NULL)
    {
      if (!parse_config (cf, &conf, path))
	{
	  /* Not compatible with this environment */
	  fclose (cf);
	  return 0;
	}
      fclose (cf);
    }
  else
    if (conf.mode == MD_MODULE && exact_architectures) /* .config required for this arch */
     {
	printf ("Ignoring %s - No CPU specified\n", path);
        return 0;
     }

  sprintf (tmp, "%s/.nativemake", path);	/* Use the existing makefile */
  if (stat (tmp, &st) != -1)
    {
      return 1;
    }

  sprintf (cfg_name, "%s_cfg.c", name);
  sprintf (cfg_header, "%s_cfg.h", name);

  sprintf (tmp, "%s/Makefile.%s", path, conf.system);
  unlink (tmp);

  sprintf (tmp, "%s/Makefile", path);
  unlink (tmp);

  sprintf (tmp, "%s/.nomake", path);
  if (stat (tmp, &st) != -1)
    return 0;

  sprintf (tmp, "%s/.makefile", path);
  if (stat (tmp, &st) != -1)
    include_local_makefile = 1;

  if (kernelonly)
    if (conf.mode == MD_USERLAND || conf.mode == MD_SBIN)
      return 0;

  if (useronly)
    if (conf.mode == MD_KERNEL || conf.mode == MD_MODULE ||
	conf.mode == MD_KERNEL_ || conf.mode == MD_MODULE_)
      return 0;

  if (conf.mode == MD_MODULE)
    driver_gen (name, &conf, cfg_name, cfg_header, path, topdir);

  if ((dir = opendir (path)) == NULL)
    {
      perror (path);
      fprintf(stderr, "scan_dir(%s): Opendir failed\n", path);
      exit (-1);
    }

  while ((de = readdir (dir)) != NULL)
    {
      if (de->d_name[0] == '.')
	continue;

      if (n_filenames >= MAX_FILENAME)
	{
	  fprintf (stderr, "Too many files in directory %s\n", path);
	  exit (-1);
	}

      filenames[n_filenames++] = strdup (de->d_name);
    }

  qsort (filenames, n_filenames, sizeof (char *), cmpstringp);

  for (i = 0; i < n_filenames; i++)
    {
      sprintf (tmp, "%s/%s", path, filenames[i]);
      if (stat (tmp, &st) == -1)
	{
	  perror (tmp);
	  continue;
	}

      if (S_ISDIR (st.st_mode))
	{
	  char top[256];

	  if (topdir == NULL)
	    strcpy (top, "..");
	  else
	    sprintf (top, "../%s", topdir);

	  if (scan_dir (tmp, filenames[i], top, &conf, level + 1))
	    {
	      if (ndirs >= MAX_SUBDIRS)
		{
		  fprintf (stderr, "Too many subdirs in %s\n", path);
		  exit (-1);
		}

	      subdirs[ndirs++] = strdup (filenames[i]);
	    }
	  continue;
	}
      /*      printf("%s/%s\n", path, filenames[i]); */

      if (nobjects >= MAX_OBJECTS || nsources >= MAX_OBJECTS)
	{
	  fprintf (stderr, "Too many objects in %s\n", path);
	  exit (-1);
	}

      strcpy (obj, filenames[i]);
      p = obj;
      suffix = "";

      while (*p)
	{
	  if (*p == '.')
	    suffix = p;
	  p++;
	}

      if (strcmp (suffix, ".c") == 0)
	{
	  sources[nsources++] = strdup (obj);
	  if (strcmp (obj, cfg_name) == 0)
	    cfg_seen = 1;
	}

      if (config_phpmake)
         {
		 if (strcmp(suffix, ".PHc") == 0)
	            {
			    if (*autogen_sources != 0)
			       strcat(autogen_sources, " ");
			    *suffix=0;
			    strcat(autogen_sources, obj);
			    strcat(autogen_sources, ".c");
			    *suffix='.';
			    strcpy(suffix, ".c");
		    }
		 else
		 if (strcmp(suffix, ".PHh") == 0)
	            {
			    if (*autogen_sources != 0)
			       strcat(autogen_sources, " ");
			    *suffix=0;
			    strcat(autogen_sources, obj);
			    strcat(autogen_sources, ".h");
			    *suffix='.';
		    }
		 else
		 if (strcmp(suffix, ".PHinc") == 0)
	            {
			    if (*autogen_sources != 0)
			       strcat(autogen_sources, " ");
			    *suffix=0;
			    strcat(autogen_sources, obj);
			    strcat(autogen_sources, ".inc");
			    *suffix='.';
		    }
	 }
      else
	 {
		 char source[256], target[256];

		 if (strcmp(suffix, ".PHc") == 0)
	            {
			    *suffix=0;
			    sprintf (source, "%s.PHc", obj);
			    sprintf (target, "%s/%s.c", path, obj);
			    *suffix='.';
			    if (symlink (source, target) == -1)
			       {
				       perror(source);
				       exit(1);
			       }
		    }
		 else
		 if (strcmp(suffix, ".PHh") == 0)
	            {
			    *suffix=0;
			    sprintf (source, "%s.PHh", obj);
			    sprintf (target, "%s/%s.h", path, obj);
			    *suffix='.';
			    if (symlink (source, target) == -1)
			       {
				       perror(source);
				       exit(1);
			       }
		    }
		 else
		 if (strcmp(suffix, ".PHinc") == 0)
	            {
			    *suffix=0;
			    sprintf (source, "%s.PHinc", obj);
			    sprintf (target, "%s/%s.inc", path, obj);
			    *suffix='.';
printf("Symlink %s -> %s\n", source, target);
			    if (symlink (source, target) == -1)
			       {
				       perror(source);
				       exit(1);
			       }
		    }
	 }

      if (strcmp (suffix, ".c") == 0 ||
	  strcmp (suffix, ".C") == 0 || strcmp (suffix, ".cpp") == 0)
	{
	  obj_src[nobjects] = strdup (obj);
	  *suffix = 0;
	  strcat (obj, ".o");
	  objects[nobjects++] = strdup (obj);
	}
    }

  closedir (dir);
  sprintf (tmp, "%s/.depend", path);
  unlink (tmp);
  sprintf (tmp, "touch %s/.depend", path);
  system (tmp);

  if (level == 1 && *this_os && !useronly)
    {
      subdirs[ndirs++] = strdup (this_os);
    }

#if 0
  // This block is no longer necessary because the driver_gen () call was moved.
  // Now the _cfg.c file should get created so that it gets picked by the readdir() loop.
  // However keep it here for a while.
  if (!cfg_seen && conf.mode == MD_MODULE)
    {
# if !defined(linux) && !defined(__FreeBSD__)
      sprintf (tmp, "%s_cfg.c", name);
      sources[nsources++] = strdup (tmp);

      obj_src[nobjects] = strdup (tmp);
      sprintf (tmp, "%s_cfg.o", name);
      objects[nobjects++] = strdup (tmp);
# endif
    }
#endif

#if 0
  // This stuff has been moved above the readdir() loop.
  if (conf.mode == MD_MODULE)
    driver_gen (name, &conf, cfg_name, cfg_header, path, topdir);
#endif

  if (do_cleanup || (ndirs == 0 && nobjects == 0))
    {
      return 0;
    }

  if (config_phpmake)
     sprintf (tmp, "%s/Makefile.php", path);
  else
     sprintf (tmp, "%s/Makefile", path);

#if 0
  if ((f = fopen (tmp, "w")) == NULL)
    {
      perror (tmp);
      exit (-1);
    }

  if (include_local_makefile)
    {
      fprintf (f, "\n");
      fprintf (f, "include .makefile\n");
      fprintf (f, "\n");
    }

  fprintf (f, "all:\n");
  fprintf (f,
	   "\t$(MAKE) $(BUILDFLAGS) BUILDFLAGS=\"$(BUILDFLAGS)\" -f Makefile.`uname -s` all\n\n");

  fprintf (f, "config:\n");
  fprintf (f,
	   "\t$(MAKE) $(BUILDFLAGS) BUILDFLAGS=\"$(BUILDFLAGS)\" -f make.defs config\n\n");

  fprintf (f, "purge:\n");
  fprintf (f,
	   "\t$(MAKE) $(BUILDFLAGS) BUILDFLAGS=\"$(BUILDFLAGS)\" -f make.defs purge\n\n");

  fprintf (f, "dirs:\n");
  fprintf (f,
	   "\t$(MAKE) $(BUILDFLAGS) BUILDFLAGS=\"$(BUILDFLAGS)\" -f Makefile.`uname -s` dirs\n\n");

  fprintf (f, "clean:\n");
  fprintf (f,
	   "\t$(MAKE)  $(BUILDFLAGS) BUILDFLAGS=\"$(BUILDFLAGS)\" -f Makefile.`uname -s` clean\n\n");

  fprintf (f, "lint:\n");
  fprintf (f,
	   "\t$(MAKE)  $(BUILDFLAGS) BUILDFLAGS=\"$(BUILDFLAGS)\" -f Makefile.`uname -s` lint\n\n");

  fprintf (f, "dep:\n");
  fprintf (f,
	   "\t$(MAKE)  $(BUILDFLAGS) BUILDFLAGS=\"$(BUILDFLAGS)\" -f Makefile.`uname -s` dep\n\n");

  fclose (f);

  sprintf (tmp, "%s/Makefile.%s", path, conf.system);
#endif
  if ((f = fopen (tmp, "w")) == NULL)
    {
      perror (tmp);
      exit (-1);
    }

  fprintf (f, "# Makefile for %s module %s\n\n", conf.project_name, name);

  if (config_phpmake)
     fprintf (f, "<?php require getenv(\"PHPMAKE_LIBPATH\") . \"library.php\"; phpmake_makefile_top_rules(); ?>\n");

  fprintf (f, "CC=%s\n", conf.ccomp);
  // fprintf (f, "LD=ld\n");
  fprintf (f, "HOSTCC=%s\n", hostcc);
  fprintf (f, "CPLUSPLUS=%s\n", conf.cplusplus);

#ifdef VXWORKS
  vxworks_genheader (f, path);
#endif

#if defined(__SCO_VERSION__)
  if (*conf.cflags != 0)
    fprintf (f, "CFLAGS=%s\n", conf.cflags);
#endif
  if (*conf.ldflags != 0)
    fprintf (f, "LDFLAGS=%s\n", conf.ldflags);

  if (strcmp(conf.endianess, "UNKNOWN") != 0)
     sprintf (tmp_endian, " -DOSS_%s_ENDIAN", conf.endianess);

  fprintf (f, "OSFLAGS=%s%s\n", conf.OSflags, tmp_endian);

  fprintf (f, "OS=%s\n", conf.system);
  fprintf (f, "ARCH=%s\n", conf.arch);

  if (topdir == NULL)
    fprintf (f, "TOPDIR=.\n");
  else
    fprintf (f, "TOPDIR=%s\n", topdir);

  fprintf (f, "OBJDIR=$(TOPDIR)/target/objects\n");
  fprintf (f, "TMPDIR=.\n");
  fprintf (f, "MODDIR=$(TOPDIR)/target/modules\n");
  fprintf (f, "BINDIR=$(TOPDIR)/target/bin\n");
  fprintf (f, "LIBDIR=$(TOPDIR)/target/lib\n");
  fprintf (f, "SBINDIR=$(TOPDIR)/target/sbin\n");
  if ((p = getenv("OSSLIBDIR")) != NULL)
    fprintf (f, "OSSLIBDIR=\"%s\"\n", p);

  fprintf (f, "THISOS=%s\n", this_os);

  if (config_phpmake)
     fprintf (f, "CFLAGS+=-D__USE_PHPMAKE__\n");

  if (conf.mode == MD_KERNEL || conf.mode == MD_MODULE)
    {
#if defined(__SCO_VERSION__)
      fprintf (f, "CFLAGS=-O -D_KERNEL -D_DDI=8\n");
#else
      fprintf (f, "CFLAGS += -D_KERNEL\n");
#endif
#ifdef HAVE_KERNEL_FLAGS
      add_kernel_flags (f);
#endif
    }
#ifndef __SCO_VERSION__
  else
    {
      fprintf (f, "CFLAGS+=-O\n");
    }
#endif

#if !defined(__SCO_VERSION__)
  if (*conf.cflags != 0)
    fprintf (f, "CFLAGS += %s\n", conf.cflags);
  if (conf.mode == MD_SHLIB)
    fprintf (f, "CFLAGS += %s\n", shlib_cflags);
#endif
  if (conf.mode != MD_KERNEL)
    objdir = "TMPDIR";

  if (nincludes > 0)
    {
      int i;
      fprintf (f, "INCLUDES=");
      for (i = 0; i < nincludes; i++)
	{
	  if (i > 0)
	    fprintf (f, " ");
	  if (includes[i][0] == '/')
	    fprintf (f, "%s", includes[i]);
	  else
	    fprintf (f, "-I$(TOPDIR)/%s", includes[i]);
	}
      fprintf (f, "\n");
    }

  if (ndirs > 0)
    {
      int i;

      if (config_phpmake)
         {
	      fprintf (f, "<?php\n");
	      fprintf (f, "\t$subdirs=array(");
	      for (i = 0; i < ndirs; i++)
		{
		  if (i > 0)
		    fprintf (f, ", ");
		  fprintf (f, "\"%s\"", subdirs[i]);
		}
	      fprintf (f, ");\n");
	      fprintf (f, "phpmake_print_subdirs($subdirs);\n");
	      fprintf (f, "?>\n");
	 }
      else
         {
	      fprintf (f, "SUBDIRS=");
	      for (i = 0; i < ndirs; i++)
		{
		  if (i > 0)
		    fprintf (f, " ");
		  fprintf (f, "%s", subdirs[i]);
		}
	      fprintf (f, "\n");
	 }
    }

  if (nobjects > 0)
    {
      int i;

      fprintf (f, "OBJECTS=");

      for (i = 0; i < nobjects; i++)
	{
	  if (i > 0)
	    fprintf (f, " ");
	  fprintf (f, "$(%s)/%s", objdir, objects[i]);
	}

      fprintf (f, "\n");
    }

  if (conf.mode == MD_MODULE)
    {
      fprintf (f, "TARGETS=$(MODDIR)/%s $(MODDIR)/%s.o\n", name, name);
      fprintf (f, "DEPDIR=$(TMPDIR)\n");
    }
  else if ((conf.mode == MD_USERLAND) && nobjects > 0)
    {
      fprintf (f, "TARGETS=$(BINDIR)/%s\n", name);
      fprintf (f, "DEPDIR=$(BINDIR)/\n");
    }
  else if ((conf.mode == MD_SBIN) && nobjects > 0)
    {
      fprintf (f, "TARGETS=$(SBINDIR)/%s\n", name);
      fprintf (f, "DEPDIR=$(SBINDIR)/\n");
    }
  else
    {
      fprintf (f, "TARGETS=%s\n", name);
      fprintf (f, "DEPDIR=\n");
    }

  if (nsources > 0)
    {
      int i;

      fprintf (f, "CSOURCES=");

      for (i = 0; i < nsources; i++)
	{
	  if (i > 0)
	    fprintf (f, " ");
	  fprintf (f, "%s", sources[i]);
	}

      fprintf (f, "\n");
    }

  if (*autogen_sources != 0)
     fprintf (f, "AUTOGEN_SOURCES=%s\n", autogen_sources);

  fprintf (f, "\n");

  if (include_local_makefile)
    {
      fprintf (f, "include .makefile\n");
      fprintf (f, "\n");
    }

  if (config_phpmake)
     fprintf (f, "<?php phpmake_makefile_rules(); ?>\n");
  /*
   * Create the default target
   */
  fprintf (f, "all: ");

  if (conf.mode == MD_USERLAND && nsources > 0)
    {
      fprintf (f, "$(TARGETS) ");
    }
  else if (conf.mode == MD_MODULE)
    {
      if (nobjects > 0)
	fprintf (f, "$(MODDIR)/%s.o ", name);
    }
  else if (conf.mode == MD_SHLIB)
    {
	fprintf (f, "$(LIBDIR)/%s.so ", name);
    }
  else if (conf.mode != MD_KERNEL)
    {
      if (nobjects > 0)
	{
	  if (conf.mode == MD_SBIN)
	    fprintf (f, "$(SBINDIR)/%s ", name);
	  else
	    fprintf (f, "$(BINDIR)/%s ", name);
	}
    }
  else
    {
      if (nobjects > 0)
	fprintf (f, "$(AUTOGEN_SOURCES) objects ");
    }

  if (ndirs > 0)
    fprintf (f, "subdirs ");
  fprintf (f, "\n");
#if 0
  if (level == 1)
    fprintf (f,
	     "\t-sh $(THISOS)/build.sh \"$(ARCH)\" \"$(INCLUDES)\" \"$(CFLAGS)\"\n");
  fprintf (f, "\n");
#endif

  /*
   * Create the lint target
   */
  fprintf (f, "lint: ");
  if (nobjects > 0)
    fprintf (f, "lint_sources ");
  if (ndirs > 0)
    fprintf (f, "lint_subdirs ");
  fprintf (f, "\n\n");

  /*
   * Create the dep target
   */
  fprintf (f, "dep: ");
  if (nobjects > 0)
    fprintf (f, "$(AUTOGEN_SOURCES) dep_local ");
  if (ndirs > 0)
    fprintf (f, "dep_subdirs ");
  fprintf (f, "\n\n");

  fprintf (f, "include $(TOPDIR)/make.defs\n");
  fprintf (f, "\n");

  if (conf.mode == MD_USERLAND)
    {
      fprintf (f, "%s:\t$(BINDIR)/%s\n\n", name, name);

      fprintf (f, "$(BINDIR)/%s:\t$(OBJECTS)\n", name);
      fprintf (f,
	       "\t$(CC) $(CFLAGS) -s -o $(BINDIR)/%s $(OBJECTS) $(LIBRARIES) $(LDFLAGS) %s\n",
	       name, extra_libraries);
      fprintf (f, "\n\n");
    }

  if (conf.mode == MD_SHLIB)
    {
      fprintf (f, "%s.so:\t$(LIBDIR)/%s.so\n\n", name, name);

      fprintf (f, "$(LIBDIR)/%s.so:\t$(OBJECTS)\n", name);
#if defined(linux)
      /* gcc -shared works much better than ld on Linux */
      fprintf (f,
	       "\t$(CC) $(LDFLAGS) %s -o $(LIBDIR)/%s.so $(OBJECTS)\n",
	       shlib_cflags, name);
#else
      fprintf (f,
	       "\t$(LD) $(LDFLAGS) %s -o $(LIBDIR)/%s.so $(OBJECTS)\n",
	       shlib_ldflags, name);
#endif
      fprintf (f, "\n\n");
    }

  if (conf.mode == MD_SBIN)
    {
      fprintf (f, "%s:\t$(SBINDIR)/%s\n\n", name, name);

      fprintf (f, "$(SBINDIR)/%s:\t$(OBJECTS)\n", name);
      fprintf (f,
	       "\t$(CC) $(CFLAGS) -s -o $(SBINDIR)/%s $(OBJECTS) $(LIBRARIES) $(LDFLAGS) %s\n",
	       name, extra_libraries);
      fprintf (f, "\n\n");
    }

  if (conf.mode == MD_MODULE)
    {
      fprintf (f, "$(MODDIR)/%s.o:\t$(OBJECTS)\n", name);
      fprintf (f, "\t$(LD) $(LDARCH) -r -o $(MODDIR)/%s.o $(OBJECTS)\n",
	       name);
      fprintf (f, "\n\n");
    }

  if (nobjects > 0)
    {
      int i;

      for (i = 0; i < nobjects; i++)
	{
	  fprintf (f, "$(%s)/%s:\t%s\n", objdir, objects[i], obj_src[i]);

	  if (is_cplusplus (obj_src[i]))
	    fprintf (f,
		     "\t$(CPLUSPLUS) -c $(CFLAGS) $(OSFLAGS) $(INCLUDES) %s -o $(%s)/%s\n",
		     obj_src[i], objdir, objects[i]);
	  else
	    fprintf (f,
		     "\t$(CC) -c $(CFLAGS) $(OSFLAGS) $(LIBRARIES) $(INCLUDES) %s -o $(%s)/%s\n",
		     obj_src[i], objdir, objects[i]);
	  fprintf (f, "\n");
	}
    }

  fprintf (f, "clean: clean_local");
  if (ndirs > 0)
    fprintf (f, " clean_subdirs");
  fprintf (f, "\n\n");

  fclose (f);
  return 1;
}

static int already_configured = 0;

static void
produce_output (conf_t * conf)
{
  if (already_configured)
    return;

  scan_dir (".", "main", NULL, conf, 1);
  scan_dir (this_os, "main", "../../..", conf, 3);

  symlink (this_os, "targetos");

  already_configured = 1;
}

static void
check_endianess (conf_t * conf)
{
  unsigned char probe[4] = { 1, 2, 3, 4 };

  if ((*(unsigned int *) &probe) == 0x01020304)
    {
      strcpy (conf->endianess, "BIG");
    }
  else
    {
      strcpy (conf->endianess, "LITTLE");
    }
}

static void
produce_local_config_h (conf_t * conf)
{
/*
 * Produce local_config.h
 */
	int grc_min=3, grc_max=3; // GRC3 min/max quality settings

	FILE *f;
	char *p;
	int q;

	if ((f=fopen ("kernel/framework/include/local_config.h", "w"))==NULL)
	   {
		   perror ("kernel/framework/include/local_config.h");
		   exit(-1);
	   }

	fprintf (f, "/*\n");
	fprintf (f, " * Automatically generated by the configure script (srcconf.c) - Do not edit.\n");
	fprintf (f, "*/\n");

/*
 * GRC3 sample rate converter min/max quality settings
 */
	if ((p=getenv("GRC_MIN_QUALITY"))!= NULL)
	{
	   if (sscanf(p, "%d", &q) != 1)
	      {
		      fprintf (stderr, "Bad GRC_MIN_QUALITY '%s'\n", p);
		      exit (EXIT_FAILURE);
	      }

	   if (q >= 0 && q <= 6)
	      grc_min = q;
	}

	if ((p=getenv("GRC_MAX_QUALITY"))!= NULL)
	{
	   if (sscanf(p, "%d", &q) != 1)
	      {
		      fprintf (stderr, "Bad GRC_MAX_QUALITY '%s'\n", p);
		      exit (EXIT_FAILURE);
	      }

	   if (q >= 0 && q <= 6)
	      grc_max = q;
	}

	if (grc_max < grc_min)
	   grc_max = grc_min = 3;

	fprintf (f, "#define CONFIG_OSS_GRC_MIN_QUALITY %d\n", grc_min);
	fprintf (f, "#define CONFIG_OSS_GRC_MAX_QUALITY %d\n", grc_max);

/*
 * Generate VMIX configuration
 */

	if (strcmp (vmix_mode, "DISABLED") == 0)
	   {
		   fprintf (f, "#undef  CONFIG_OSS_VMIX\n");
	   }
	else
	   {
		   fprintf (f, "#define CONFIG_OSS_VMIX\n");

		   if (strcmp (vmix_mode, "FLOAT") == 0)
		      fprintf (f, "#define CONFIG_OSS_VMIX_FLOAT\n");
		   else
		      fprintf (f, "#undef  CONFIG_OSS_VMIX_FLOAT\n");
	   }

/*
 * Generate MIDI configuration
 */

	if (strcmp (config_midi, "DISABLED") == 0)
	   fprintf (f, "#undef  CONFIG_OSS_MIDI\n");
	else
	   fprintf (f, "#define CONFIG_OSS_MIDI\n");

/*
 * Enable DO_TIMINGS code
 */

	if (getenv("DO_TIMINGS") != NULL)
	   fprintf (f, "#define DO_TIMINGS\n");

	fclose (f);
}

static void
produce_errno_h(void)
{
	FILE *f;

/*
 * Generate oss_errno.h that contains all the errno.h codes used by OSS
 * but defined as negative numbers.
 */

	if ((f=fopen ("kernel/framework/include/oss_errno.h", "w"))==NULL)
	   {
		   perror ("kernel/framework/include/oss_errno.h");
		   exit(-1);
	   }
#define GEN_ERRNO(e) \
	fprintf (f, "#define OSS_"#e"\t\t%d\n", (e<=0) ? e : -(e));

	fprintf (f, "#ifndef OSS_ERRNO_H\n");
	fprintf (f, "#define OSS_ERRNO_H\n");
	fprintf (f, "\n");
	fprintf (f, "/*\n");
	fprintf (f, " * Error (errno) codes used by OSS.\n");
	fprintf (f, " * \n");
	fprintf (f, " * This file is automatically generated by srcconf.c (produce_errno_h()) - do not edit.\n");
	fprintf (f, " * \n");
	fprintf (f, " * The following codes are derived from the system dependent\n");
	fprintf (f, " * error numbers defined in <errno.h>\n");
	fprintf (f, " */\n");
	fprintf (f, "\n");

#ifndef EBADE /* Not in FreeBSD, Haiku */
#define EBADE EINVAL
#endif

#ifndef EIDRM /* Not in POSIX, but is in SuS */
#define EIDRM EFAULT
#endif

#ifndef ENOTSUP /* Not in Haiku */
#define ENOTSUP ENOSYS
#endif

#ifndef EFAULT
#define EFAULT ENOTSUP
#endif

	GEN_ERRNO(E2BIG);
	GEN_ERRNO(EACCES);
	GEN_ERRNO(EAGAIN);
	GEN_ERRNO(EBADE);
	GEN_ERRNO(EBUSY);
	GEN_ERRNO(ECONNRESET);
	GEN_ERRNO(EDOM);
	GEN_ERRNO(EFAULT);
	GEN_ERRNO(EIDRM);
	GEN_ERRNO(EINTR);
	GEN_ERRNO(EINVAL);
	GEN_ERRNO(EIO);
	GEN_ERRNO(ELOOP);
	GEN_ERRNO(ENODEV);
	GEN_ERRNO(ENOENT);
	GEN_ERRNO(ENOMEM);
	GEN_ERRNO(ENOSPC);
	GEN_ERRNO(ENOTSUP);
	GEN_ERRNO(ENXIO);
	GEN_ERRNO(EPERM);
	GEN_ERRNO(EPIPE);
	GEN_ERRNO(ERANGE);
	GEN_ERRNO(EWOULDBLOCK);

	fprintf (f, "\n");
	fprintf (f, "#endif\n");
	fclose(f);
}

static void
check_system (conf_t * conf)
{
  struct utsname un;
  char *p;

  if (uname (&un) == -1)
    {
      perror ("uname");
      exit (-1);
    }

  if (strcmp (un.sysname, "UnixWare") == 0)
    strcpy (un.sysname, "SCO_SV");
  if (strcmp (un.sysname, "Haiku") == 0)
    strcpy (un.sysname, "BeOS");
  printf ("System: %s\n", un.sysname);
  strcpy (conf->system, un.sysname);
  sprintf (this_os, "kernel/OS/%s", un.sysname);
  printf ("Release: %s\n", un.release);
  printf ("Machine: %s\n", un.machine);
  strcpy (conf->arch, un.machine);

#ifdef HAVE_SYSDEP
  check_sysdep (conf, &un);
#else

# if defined(__SCO_VERSION__)
	shlib_ldflags = "-G -lsocket -lnsl";
# endif

  if (strcmp (un.machine, "i386") == 0 ||
      strcmp (un.machine, "i486") == 0 ||
      strcmp (un.machine, "i586") == 0 || strcmp (un.machine, "i686") == 0)
    {
      strcpy (conf->platform, "i86pc");
    }
  else
    {
      fprintf (stderr, "Cannot determine the platform for %s\n", un.machine);
      exit (-1);
    }
#endif

  if (*conf->platform == 0)
    {
      fprintf (stderr, "Panic: No platform\n");
      exit (-1);
    }

  check_endianess (conf);

/*
 * Check virtual mixer configuration (as set by the configure script).
 */

  if ((p=getenv("VMIX_MODE"))!=NULL)
     {
	     vmix_mode = strdup(p);
     }

/*
 * Check MIDI enabled/disabled status
 */

  if ((p=getenv("CONFIG_MIDI"))!=NULL)
     {
	     config_midi = strdup(p);
     }

  produce_local_config_h (conf);
}

int
main (int argc, char *argv[])
{
  int i;
  char tmp[256], *env;

  struct stat st;

  if (getenv("USE_PHPMAKE") != NULL)
  if (stat("phpmake/library.php", &st) != -1)
     config_phpmake=1;

  for (i = 1; i < argc; i++)
    if (argv[i][0] == '-')
      switch (argv[i][1])
	{
	case 'A':
	  strcpy (arch, &argv[i][2]);
	  printf ("Arch=%s\n", arch);
	  break;
	case 'K':
	  kernelonly = 1;
	  break;		/* Compile only the kernel mode parts */
	case 'U':
	  useronly = 1;
	  break;		/* Compile only the user land utilities */
	}

  if (getenv("NO_WARNING_CHECKS")!=NULL)
     do_warning_checks = 0;

  hostcc = getenv ("HOSTCC");
  targetcc = getenv ("CC");
  if (hostcc == NULL) hostcc = DEFAULT_CC;
  if (targetcc == NULL) targetcc = DEFAULT_CC;

#if defined(linux) || defined(__FreeBSD__) || defined(__SCO_VERSION__)
  mkdir ("target", 0755);
  mkdir ("target/build", 0755);
  system ("touch target/build/.nomake");
#endif

  if (getenv ("SOL9") != NULL)
    system ("touch kernel/drv/oss_usb/.nomake");

  check_system (&conf);

/*
 * Check if setup/$CROSSCOMPILE.conf exists and load the settings in it.
 */
  if ((env=getenv("CROSSCOMPILE"))!=NULL)
     {
	     FILE *cf;

	     sprintf (tmp, "setup/%s.conf", env);
  	     if ((cf = fopen (tmp, "r")) != NULL)
    		{
      	     		parse_config (cf, &conf, tmp);
			fclose (cf);
     		}
     }
  

  produce_output (&conf);

  produce_errno_h();
  exit (0);
}
