/*
 * These routines are used to change the contects of various structures
 * stored inside the modules
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

#if !defined(ELF64) && !defined(ELF32)
#  if defined(sparc) || defined(__x86_64__)
#     define ELF64
#  else
#error here
#     define ELF32
#  endif
#endif

#undef Elf_Ehdr
#undef Elf_Shdr
#undef Elf_Sym

#ifdef ELF32
#define Elf_Ehdr Elf32_Ehdr
#define Elf_Shdr Elf32_Shdr
#define Elf_Sym Elf32_Sym

static int
valid_elf_file (Elf_Ehdr * hdr)
{
  if (hdr->e_ident[0] != 0x7f)
    return 0;
  if (hdr->e_ident[1] != 'E')
    return 0;
  if (hdr->e_ident[2] != 'L')
    return 0;
  if (hdr->e_ident[3] != 'F')
    return 0;
  if (hdr->e_ident[EI_CLASS] != ELFCLASS32)
    {
      fprintf (stderr, "EI_CLASS=%x\n", hdr->e_ident[EI_CLASS]);
      return 0;
    }
/* Check for x86 (LSB) and PPC (MSB) data format */
  if ((hdr->e_ident[EI_DATA] != ELFDATA2LSB) &&
      (hdr->e_ident[EI_DATA] != ELFDATA2MSB))
    {
      fprintf (stderr, "EI_DATA=%x\n", hdr->e_ident[EI_DATA]);
      return 0;
    }
  if (hdr->e_ident[EI_VERSION] != EV_CURRENT)
    {
      fprintf (stderr, "EI_VERSION=%x (%x)\n", hdr->e_ident[EI_VERSION],
	       EV_CURRENT);
      return 0;
    }
  if (hdr->e_type != ET_REL && hdr->e_type != ET_DYN)
    {
      fprintf (stderr, "e_type=%x\n", hdr->e_type);
      return 0;
    }
/* Check for x86 and PPC machine type */
  if ((hdr->e_machine != EM_386) && (hdr->e_machine != EM_PPC)
      && (hdr->e_machine != EM_SPARC))
    {
      fprintf (stderr, "e_machine=%x\n", hdr->e_machine);
      return 0;
    }

  if (hdr->e_version != EV_CURRENT)
    {
      fprintf (stderr, "e_version=%x (%x)\n", hdr->e_version, EV_CURRENT);
      return 0;
    }
  if (hdr->e_ehsize != sizeof (*hdr))
    {
      fprintf (stderr, "e_ehsize=%x (%x)\n", hdr->e_ehsize, sizeof (*hdr));
      return 0;
    }

  return 1;
}
#else
/* Elf64 */
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Shdr Elf64_Shdr
#define Elf_Sym Elf64_Sym
#define locate_symbol locate_symbol64

#define elf_read_datasym elf_read_datasym64
#define valid_elf_file valid_elf_file64

static int
valid_elf_file (Elf64_Ehdr * hdr)
{
  if (hdr->e_ident[0] != 0x7f)
    return 0;
  if (hdr->e_ident[1] != 'E')
    return 0;
  if (hdr->e_ident[2] != 'L')
    return 0;
  if (hdr->e_ident[3] != 'F')
    return 0;
  if (hdr->e_ident[EI_CLASS] != ELFCLASS64)
    return 0;
  if (hdr->e_ident[EI_VERSION] != EV_CURRENT)
    return 0;
  if (hdr->e_type != ET_REL)
    return 0;
#ifdef sparc
/* Check for Sparc machine type */
  if ((hdr->e_machine != EM_SPARCV9))
    {
      fprintf (stderr, "e_machine=%x\n", hdr->e_machine);
      return 0;
    }
#else
/* Check for AMD64 (LSB) data format */
  if (hdr->e_ident[EI_DATA] != ELFDATA2LSB)
    return 0;
/* Check for x86 and PPC machine type */
#ifndef EM_X86_64
#define EM_X86_64 0		/* Dummy */
#endif
  if ((hdr->e_machine != EM_ALPHA) && (hdr->e_machine != EM_X86_64))
    return 0;
#endif
  if (hdr->e_version != EV_CURRENT)
    return 0;
  if (hdr->e_ehsize != sizeof (*hdr))
    return 0;

  return 1;
}
#endif

#ifdef NEEDS_LOCATE_SYMBOL
static char *
locate_symbol (char *buffer, int len, char *symname)
{
  Elf_Ehdr *ehdr = (Elf_Ehdr *) buffer;
  Elf_Shdr *shdr;
  Elf_Sym *sym = NULL;
  char *strtab = NULL, *ptr;
  int symsize = 0;
  int i;

  shdr = (Elf_Shdr *) & buffer[ehdr->e_shoff];

  for (i = 0; i < ehdr->e_shnum; i++)
    {

      switch (shdr[i].sh_type)
	{
	case SHT_SYMTAB:
	  sym = (Elf_Sym *) & buffer[shdr[i].sh_offset];
	  symsize = shdr[i].sh_size / shdr[i].sh_entsize;
	  break;

	case SHT_STRTAB:
#ifndef SECOND_STRTAB
	  if (strtab != NULL)
	    break;
#endif
	  strtab = &buffer[shdr[i].sh_offset];
	  break;
	}
    }

  if (sym == NULL || strtab == NULL)
    {
      fprintf (stderr, "Missing ELF symbol information\n");
      return NULL;
    }

  for (i = 0; i < symsize; i++)
    {
      char *name = &strtab[sym[i].st_name];

      if (strcmp (name, symname) == 0)
	{
	  ptr = buffer + (sym[i].st_value + shdr[sym[i].st_shndx].sh_offset);
	  return ptr;
	}
    }
  return NULL;
}
#endif

int
elf_read_datasym (char *buffer, int blen, Elf_Sym * sym, int addr,
		  char *buf, int count)
{
  Elf_Ehdr *ehdr = (Elf_Ehdr *) buffer;
  Elf_Shdr *shdr;
  int i;

  shdr = (Elf_Shdr *) & buffer[ehdr->e_shoff];

  i = sym->st_shndx;
  memcpy (buf, &buffer[shdr[i].sh_offset + addr], count);

  return 1;
}

#ifdef PATCH_MODULE
static int
PATCH_MODULE (char *fname)
{
  int fd;
  int l, n;
  char *lic;

  oss_license_t *oss_license;

  char elf_file[1024 * 1024];

/*
 * Some security checks to prevent running osslic by
 * mistake. That would wipe out the license information.
 */

  if (!ok)
    {
      printf ("Incorrect osslic usage\n");
      return 0;
    }

  if (fname == NULL)
    return 0;

  if ((fd = open (fname, O_RDWR, 0)) == -1)
    {
      perror (fname);
      return 0;
    }

  if ((l = read (fd, elf_file, sizeof (elf_file))) < 1
      || l == sizeof (elf_file))
    {
      perror (fname);
      fprintf (stderr, "Bad ELF file %s\n", fname);
      return 0;
    }

  if (!valid_elf_file ((Elf_Ehdr *) elf_file))
    {
      fprintf (stderr, "OSS elflib: Invalid ELF file %s\n", fname);
      return 0;
    }

  if ((lic = locate_symbol (elf_file, l, "oss_license")) == NULL)
    {
      fprintf (stderr, "File error\n");
      close (fd);
      return 0;
    }

  oss_license = (oss_license_t *) lic;

  oss_license->license_type = license_type;
  strcpy (oss_license->person, person);
  strcpy (oss_license->organization, organization);
  strcpy (oss_license->options_string, options_string);
  strcpy (oss_license->serial, serial);
  oss_license->exp_year = exp_year;
  oss_license->exp_month = exp_mon;

  if (lseek (fd, 0L, SEEK_SET) == -1)
    {
      perror (fname);
      fprintf (stderr, "Bad seek\n");
      return 0;
    }

  if ((n = write (fd, elf_file, l)) != l)
    {
      perror (fname);
      fprintf (stderr, "Writing ELF file %s failed\n", fname);
      return 0;
    }

  close (fd);
  return 1;
}
#endif

#ifdef ELF_LOAD_SYMTAB

typedef struct
{
  char name[64];
  unsigned long addr;
} oss_symbol_t;

#define MAX_MODULES 100
static oss_symbol_t module_table[MAX_MODULES];
static int n_module_table = 0;

typedef void (*elf_callback_t) (char *buffer, int blen, Elf_Sym * sym,
				char *name, int addr);
static int
list_symbols (char *buffer, int len, char *prefix, elf_callback_t callback)
{
  Elf_Ehdr *ehdr = (Elf_Ehdr *) buffer;
  Elf_Shdr *shdr;
  Elf_Sym *sym = NULL;
  char *strtab = NULL;
  int symsize = 0;
  int i;
  int shstrndx = -1;
  int found = 0;

  shdr = (Elf_Shdr *) & buffer[ehdr->e_shoff];
  shstrndx = ehdr->e_shstrndx;

  if (elf_verbose > 1)
    printf ("e_shstrndx=%d\n", shstrndx);

  for (i = 0; i < ehdr->e_shnum; i++)
    {
      switch (shdr[i].sh_type)
	{
	case SHT_SYMTAB:
	  sym = (Elf_Sym *) & buffer[shdr[i].sh_offset];
	  symsize = shdr[i].sh_size / shdr[i].sh_entsize;
	  if (elf_verbose > 1)
#ifdef ELF32
	    printf ("ELF symtab at 0x%x\n", shdr[i].sh_offset);
#else
	    printf ("ELF symtab at 0x%lx\n", shdr[i].sh_offset);
#endif
	  break;

	case SHT_STRTAB:
	  if (i == shstrndx)	/* Ignore section header symbol table */
	    break;
	  strtab = &buffer[shdr[i].sh_offset];
	  if (elf_verbose > 1)
#ifdef ELF32
	    printf ("ELF strtab at 0x%x (%d) \n", shdr[i].sh_offset, i);
#else
	    printf ("ELF strtab at 0x%lx (%d) \n", shdr[i].sh_offset, i);
#endif
	  break;
	}
    }

  if (sym == NULL || strtab == NULL)
    {
      fprintf (stderr, "Missing ELF symbol information\n");
      return 0;
    }

  for (i = 0; i < symsize; i++)
    {
      char *name = &strtab[sym[i].st_name];

      if (elf_verbose > 2)
	printf ("Sym %d '%s'\n", sym[i].st_name, name);

      if (strncmp (name, prefix, strlen (prefix)) == 0)
	{
	  oss_symbol_t *m;

	  if (callback != NULL)
	    {
	      found = 1;
	      callback (buffer, len, &sym[i], name, sym[i].st_value);
	      continue;
	    }

	  m = &module_table[n_module_table++];
	  strcpy (m->name, name);
	  m->addr = sym[i].st_value;
	}
    }
  return found;
}

int
ELF_LOAD_SYMTAB (char *filename, char *prefix, elf_callback_t callback)
{
  char buffer[4 * 1024 * 1024];
  int fd, len;
  Elf_Ehdr *hdr = (Elf_Ehdr *) buffer;

  if ((fd = open (filename, O_RDONLY, 0)) == -1)
    {
      perror (filename);
      return 0;
    }

  if ((len = read (fd, buffer, sizeof (buffer))) <= 0)
    {
      perror (filename);
      close (fd);
      return 0;
    }

  if (len >= sizeof (buffer))
    {
      fprintf (stderr, "%s is too large\n", filename);
      close (fd);
      return 0;
    }

  if (len < sizeof (*hdr))
    {
      fprintf (stderr, "%s is too short (%d)\n", filename, len);
      close (fd);
      return 0;
    }

  if (!valid_elf_file (hdr))
    {
      fprintf (stderr, "%s is not a valid ELF object\n", filename);
      close (fd);
      return 0;
    }

  if (!(list_symbols (buffer, len, prefix, callback)))
    {
      if (elf_verbose > 1)
	fprintf(stderr, "%s does not contain %s symbol\n", filename, prefix);
      close (fd);
      return 0;
    }

  close (fd);
  return 1;
}
#endif
