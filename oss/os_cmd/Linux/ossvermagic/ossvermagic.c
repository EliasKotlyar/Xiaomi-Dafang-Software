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
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <elf.h>
#define CONFIGURE_C
#include "oss_config.h"

int elf_verbose = 0;

#ifdef __x86_64__
#define ELF64
#define ELF_LOAD_SYMTAB	elf_load_symtab
#else
#define ELF32
#define ELF_LOAD_SYMTAB	elf_load_symtab
#endif
#include "../../../setup/elflib.inc"

char *fname;
int ok;
int quiet = 0;
int verbose = 0;
int once = 0;
int exit_status = -1;
int check_compile_vermagic = 0;

static void
sym_callback (char *buffer, int blen, Elf_Sym * sym, char *name, int addr)
{

  char tmp[256], *vermagic = tmp;

  ok = 1;
  exit_status = 0;

  if (verbose)
    printf ("Vermagic buffer at %x\n", addr);

  elf_read_datasym (buffer, blen, sym, addr, tmp, sizeof (tmp));

  if (strncmp (vermagic, "vermagic=", 9) == 0)
    vermagic += 9;

  if (quiet)
    printf ("%s\n", vermagic);
  else
    printf ("%s: '%s'\n", fname, vermagic);

  if (once)
    exit (0);
}

static void
find_vermagic (char *fname)
{
  ok = 0;

  if (verbose)
    printf ("ELF scan %s\n", fname);

  ok = ELF_LOAD_SYMTAB (fname, "vermagic", sym_callback);
  if (!ok)
    ok = ELF_LOAD_SYMTAB (fname, "__mod_vermagic", sym_callback);
  if (!ok)
    ELF_LOAD_SYMTAB (fname, "__oss_compile_vermagic", sym_callback);
}

static void
find_link_vermagic (char *fname)
{
  if (verbose)
    printf ("ELF scan %s\n", fname);

  ELF_LOAD_SYMTAB (fname, "__oss_compile_vermagic", sym_callback);
}

static void
check_gzipped_module (char *fname)
{
  char tmp[1024];

  sprintf (tmp, "gunzip -c %s > /tmp/oss.tmpmodule", fname);
  unlink ("/tmp/oss.tmpmodule");

  if (system (tmp) != 0)
    {
      unlink ("/tmp/oss.tmpmodule");
      return;
    }

  find_vermagic ("/tmp/oss.tmpmodule");
  unlink ("/tmp/oss.tmpmodule");
}

static void
check_bzipped_module (char *fname)
{
  char tmp[1024];

  sprintf (tmp, "bunzip2 -c %s > /tmp/oss.tmpmodule", fname);
  unlink ("/tmp/oss.tmpmodule");

  if (system (tmp) != 0)
    {
      unlink ("/tmp/oss.tmpmodule");
      return;
    }

  find_vermagic ("/tmp/oss.tmpmodule");
  unlink ("/tmp/oss.tmpmodule");
}

static void
scan_dir (char *dirname)
{
  char tmp[1024];
  DIR *dir;
  struct dirent *de;

  if ((dir = opendir (dirname)) == NULL)
    return;

  while ((de = readdir (dir)))
    {
      struct stat st;

      if (de->d_name[0] == '.')
	continue;

      sprintf (tmp, "%s/%s", dirname, de->d_name);

      if (stat (tmp, &st) == -1)
	{
	  perror (tmp);
	  return;
	}

      if (S_ISDIR (st.st_mode))
	scan_dir (tmp);
      else
	{
	  char *p;

	  if (verbose)
	    printf ("Checking %s\n", tmp);
	  p = tmp + strlen (tmp) - 3;	// Seek the .gz suffix
	  if (strcmp (p, ".gz") == 0)
	    {
	      fname = tmp;
	      check_gzipped_module (tmp);
	      continue;
	    }

	  p = tmp + strlen (tmp) - 4;	// Seek the .bz2 suffix
	  if (strcmp (p, ".bz2") == 0)
	    {
	      fname = tmp;
	      check_bzipped_module (tmp);
	      continue;
	    }

	  fname = tmp;
	  find_vermagic (tmp);
	}
    }

  closedir (dir);
}

static void
find_system_vermagic (void)
{
  struct utsname un;
  char dirname[1024];

  struct stat st;

  if (uname (&un) == -1)
    {
      perror ("uname");
      exit (-1);
    }

  sprintf (dirname, "/lib/modules/%s/kernel", un.release);

  if (stat (dirname, &st) == -1)
    {
      perror (dirname);
      fprintf (stderr, "Kernel modules not available\n");
      exit (-1);
    }

  quiet = 1;
  once = 1;
  scan_dir (dirname);
}

int
main (int argc, char *argv[])
{
  int i, ok;
  int do_sys = 0;
  int valid = 0;

  if (argc < 2)
    exit (-1);

  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "-r") == 0)
	{
	  /*
	   * Check if the system is 2.6.20 or later which is
	   * always compiled with CONFIG_REGPARM.
	   */

	  struct utsname un;
	  int a, b, c;

	  if (uname (&un) == -1)
	    {
	      perror ("uname");
	      exit (-1);
	    }

	  if (sscanf (un.release, "%d.%d.%d", &a, &b, &c) != 3)
	    {
	      fprintf (stderr, "Unrecognized kernel release '%s'\n",
		       un.release);
	      exit (0);		/* Assume it's some future kernel */
	    }

	  if (a > 2)
	    exit (0);		/* Always REGPARM */
	  if (b > 6)
	    exit (0);		/* Always REGPARM */
	  if (c >= 20)
	    exit (0);		/* Always REGPARM */

	  exit (1);		/* Don't know */
	}

      if (strcmp (argv[i], "-s") == 0)
	do_sys = 1;
      if (strcmp (argv[i], "-v") == 0)
	verbose++;
      if (strcmp (argv[i], "-z") == 0)
	valid = 1;
      if (strcmp (argv[i], "-u") == 0)
	check_compile_vermagic = 1;
    }

  if (!valid)
    {
      fprintf (stderr,
	       "%s: This program is reserved for use by Open Sound System\n",
	       argv[0]);
      exit (-1);
    }

  elf_verbose = verbose;

  if (do_sys)
    {
      ok = 0;
      if (fork () == 0)
	find_system_vermagic ();
      else
	{
	  int status;
	  wait (&status);
	  unlink ("/tmp/oss.tmpmodule");
	  exit (WEXITSTATUS (status));
	}

      if (!ok)
	exit (-1);
      exit (0);
    }

  for (i = 1; i < argc; i++)
    {
      if (strcmp (argv[i], "-q") == 0)
	{
	  quiet = 1;
	  continue;
	}

      if (strcmp (argv[i], "-1") == 0)
	{
	  once = 1;
	  continue;
	}

      fname = argv[i];
      ok = 0;

      if (*fname != '-')
	{
          if (check_compile_vermagic == 0) find_vermagic (fname);
          else find_link_vermagic (fname);
	}
    }

  exit (exit_status);
}
