/*
 * Purpose: OSS device autodetection utility for SCO OpenServer
 *
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mkdev.h>
#include <sys/stat.h>
#include <sys/param.h>

dev_t default_dev = 0;

#define PCI_PASS	0
#define USB_PASS	1
#define PSEUDO_PASS	2
#define MAX_PASS	3

#define OSSLIBDIRLEN 512
static char *osslibdir = NULL;

static int verbose = 0;

typedef struct
{
  char *key, *driver, *name;
  int is_3rdparty;
  int detected;
  int pass;
} driver_def_t;

typedef struct drv_slist
{
  const char * drv_name;
  struct drv_slist * next;
} drvlist_t;
static drvlist_t * drvl = NULL;

#define MAX_DRIVERS	1000
static driver_def_t drivers[MAX_DRIVERS];
static int ndrivers = 0;

static int add_drv (const char *, int);
static void create_devlinks (mode_t);
static void create_node (char *, char *, int, mode_t);
static drvlist_t * prepend_drvlist (const char *);
static char * get_mapname (void);
static void load_license (const char *);
static void load_devlist (const char *, int);
static void pci_detect (void);

static char *
get_mapname (void)
{
  FILE *f;
  static char name[OSSLIBDIRLEN], tmp[OSSLIBDIRLEN+11];
  struct stat st;

  if ((f = fopen ("/etc/oss.conf", "r")) == NULL)
    {
      perror ("/etc/oss.conf");
      goto oexit2;
    }

  while (fgets (tmp, sizeof (tmp), f) != NULL)
    {
      int l = strlen (tmp);
      if (l > 0 && tmp[l - 1] == '\n')
	tmp[l - 1] = '\0';

      if (strncmp (tmp, "OSSLIBDIR=", 10) == 0)
	{
	  l = snprintf (name, sizeof (name), "%s", &tmp[10]);
	  if ((l >= OSSLIBDIRLEN) || (l < 0))
	    {
	      fprintf (stderr, "String in /etc/oss.conf is too long!\n");
	      goto oexit;
	    }
	  if ((stat (name, &st) == -1) || !S_ISDIR(st.st_mode))
	    {
	      fprintf (stderr, "Directory %s from /etc/oss.conf cannot "
			       "be used!\n", name);
	      goto oexit;
	    }
	  fclose (f);
	  return name;
	}
    }

  fprintf (stderr, "OSSLIBDIR not set in /etc/oss.conf, using default "
		   "/usr/lib/oss\n");
oexit:
  fclose (f);
oexit2:
  snprintf (name, sizeof (name), "/usr/lib/oss");
  return name;
}

static void
load_license (const char *fname)
{
  struct stat st;
  char cmd[2*OSSLIBDIRLEN];
  int n;

  if (stat (fname, &st) == -1)
    return;			/* Doesn't exist */

  if (stat ("/usr/sbin/osslic", &st) == -1)
    return;			/* No osslic utility in the system. No need to install license. */

  n = snprintf (cmd, sizeof (cmd), "/usr/sbin/osslic -q %s/%s", osslibdir,
                fname);
  if (n >= sizeof (cmd) || n < 0) return;
  system (cmd);
}

static void
load_devlist (const char *fname, int is_3rdparty)
{
  FILE *f;
  char line[256], *p, rfname[2*OSSLIBDIRLEN];
  char *driver, *key, *name;

  snprintf (rfname, sizeof (rfname), "%s/%s", osslibdir, fname);

  if ((f = fopen (rfname, "r")) == NULL)
    {
      perror (rfname);
      exit (-1);
    }

  while (fgets (line, sizeof (line), f) != NULL)
    {
      p = line;
      while (*p)
	{
	  if (*p == '#' || *p == '\n')
	    *p = 0;
	  p++;
	}

      /* Drivers with upper case names are unsupported ones */
      if ((*line >= 'A' && *line <= 'Z') || (*line == '\0'))
	continue;

      driver = line;
      p = line;

      while (*p && *p != '\t' && *p != ' ')
	p++;
      if (*p)
	*p++ = 0;
      key = p;

      while (*p && *p != '\t')
	p++;
      if (*p)
	*p++ = 0;
      name = p;

      if (verbose > 1)
	printf ("device=%s, name=%s, driver=%s\n", key, name, driver);

      if (ndrivers >= MAX_DRIVERS)
	{
	  printf ("Too many drivers defined in drivers.list\n");
	  exit (-1);
	}

      drivers[ndrivers].key = strdup (key);
      drivers[ndrivers].driver = strdup (driver);
      drivers[ndrivers].name = strdup (name);
      drivers[ndrivers].is_3rdparty = is_3rdparty;
      drivers[ndrivers].detected = 0;

      ndrivers++;
    }

  fclose (f);
}

static int
add_drv (const char * id, int pass)
{
  int i;

  for (i = 0; i < ndrivers; i++)
    {
      if (strcmp (id, drivers[i].key) == 0)
	{
	  if (verbose > 0)
	    printf ("Detected %s\n", drivers[i].name);
	  drivers[i].detected = 1;
	  drivers[i].pass = pass;
	  return 1;
	}
    }

  return 0;
}

static void
create_node (char *drvname, char *name, int devno, mode_t node_m)
{
  struct stat st;
  char tmp[64], *s, *p;
  char cmd[128];
  dev_t dev;

  sprintf (tmp, "/dev/%s", drvname);

  if (stat (tmp, &st) == -1)
    dev = default_dev;
  else
    {
      if (!S_ISCHR (st.st_mode))
	return;
      dev = st.st_rdev;
    }

  if (dev == 0)
    return;

/*
 * Check if the device is located in a subdirectory (say /dev/oss/sblive0/pcm0).
 */
  sprintf (tmp, "/dev/%s", name);

  s = tmp + 5;
  p = s;
  while (*s)
    {
      if (*s == '/')
	p = s;
      s++;
    }

  if (*p == '/')
    {
      *p = 0;			/* Extract the directory name part */
      mkdir (tmp, 0755);
    }

  sprintf (tmp, "/dev/%s", name);
  dev += devno;
  if (verbose)
    printf ("mknod %s c %d %d -m %o\n", tmp, major, minor, node_m);
  unlink (tmp);

  if (mknod (tmp, node_m, dev) == -1)
    perror (tmp);
}

static void
create_devlinks (mode_t node_m)
{
  FILE *drvf, *f;
  struct stat st;
  char drvname[32], name[32], line[64], *s, tmp[256], instfname[2*OSSLIBDIRLEN];
  mode_t perm;

  snprintf (instfname, sizeof (instfname), "%s/%s", osslibdir,
 	    "etc/installed_drivers");

  if ((drvf = fopen (instfname, "r")) == NULL)
    {
      perror (instfname);
      return;
    }

  perm = umask (0);
  mkdir ("/dev/oss", 0755);

  while (fgets (drvname, sizeof (drvname), drvf) != NULL)
    {
      s = strchr (drvname, '\n');
      if (s) *s = '\0';			/* Remove the LF character */

      /* Remove the device full name (comment) field from the line */
      s = drvname;
      while (*s && *s != ' ' && *s != '#')
	s++;
      *s = '\0';
      if (*drvname == '\0') continue;

      sprintf (name, "/dev/%s0", drvname);

      if (stat (name, &st) == -1)
	continue;
      default_dev = st.st_rdev;

      if ((f = fopen (name, "r")) == NULL)
	{
	  perror (name);
	  continue;
	}

      while (fgets (line, sizeof (line), f) != NULL)
	{
	  int minor;

	  if (sscanf (line, "%s %s %d", name, drvname, &minor) != 3)
	    {
	      fprintf (stderr,
		       "ossdetect: Unexpected line in the drvinfo file %s\n",
		       name);
	      fprintf (stderr, "'%s'\n", line);
	      exit (-1);
	    }

	  create_node (drvname, name, minor, node_m);
	}

      fclose (f);
      break;
    }

  umask (perm);
  fclose (drvf);
}

static void
pci_detect (void)
{
  FILE *f;
  char line[256];

  if ((f = popen ("echo pcishort|/usr/sbin/ndcfg -q", "r")) == NULL)
    {
      perror ("pcishort|/usr/sbin/ndcfg -q");
      fprintf (stderr, "Scanning PCI devices failed\n");
      exit (-1);
    }

  while (fgets (line, sizeof (line), f) != NULL)
    {
      int dummy;
      int vendor, device;
      char name[32];

      if (sscanf (line, "%d %x %x %d %d %x %x",
		  &dummy, &dummy, &dummy, &dummy, &dummy,
		  &vendor, &device) != 7)
	{
	  fprintf (stderr, "Bad line returned by ndcfg\n");
	  fprintf (stderr, "%s", line);
	  exit (-1);
	}

      sprintf (name, "pci%x,%x", vendor, device);

      add_drv (name, PCI_PASS);
    }

  pclose (f);
}

static drvlist_t *
prepend_drvlist (const char * name)
{
  drvlist_t * dp;

  dp = malloc (sizeof (drvlist_t));
  if (dp == NULL)
    {
      fprintf (stderr, "Can't allocate memory!\n");
      exit (-1);
    }

  dp->drv_name = name;
  dp->next = drvl;
  return dp;
}

int
main (int argc, char *argv[])
{
  char instfname[2*OSSLIBDIRLEN], *p;
  int i, pass, do_license = 0, make_devs = 0;
  mode_t node_m = S_IFCHR | 0666;
  struct stat st;
  FILE *f;

  while ((i = getopt(argc, argv, "L:a:dilm:uv")) != EOF)
    switch (i)
      {
	case 'v':
	  verbose++;
	  break;

	case 'd':
	  make_devs = 1;
	  break;

	case 'i':
	  drvl = prepend_drvlist ("oss_imux");
	  break;

	case 'u':
	  drvl = prepend_drvlist ("oss_userdev");
	  break;

	case 'a':
	  drvl = prepend_drvlist (optarg);
	  break;

	case 'l':
	  do_license = 1;
	  break;

	case 'L':
	  osslibdir = optarg;
	  break;

	case 'm':
	  p = optarg;
	  node_m = 0;
	  while ((*p >= '0') && (*p <= '7')) node_m = node_m * 8 + *p++ - '0';
	  if ((*p) || (node_m & ~(S_IRWXU|S_IRWXG|S_IRWXO)))
	    {
	      fprintf (stderr, "Invalid permissions: %s\n", optarg);
	      exit(1);
	    }
	  node_m |= S_IFCHR;
	  break;

	default:
	  fprintf (stderr, "%s: bad usage\n", argv[0]);
	  exit (-1);
      }

  if (osslibdir == NULL) osslibdir = get_mapname ();

  if (make_devs == 1)
    {
      create_devlinks (node_m);
      exit (0);
    }

  if (do_license == 1)
    {
      load_license ("etc/license.asc");
      exit (0);
    }

  load_devlist ("etc/devices.list", 0);

  if (stat ("/etc/oss_3rdparty", &st) != -1)
    load_devlist ("/etc/oss_3rdparty", 1);

  pci_detect ();

  while (drvl != NULL)
    {
      drvlist_t * d = drvl;
      add_drv (drvl->drv_name, PSEUDO_PASS);
      drvl = drvl->next;
      free (d);
    }

  snprintf (instfname, sizeof (instfname), "%s/%s", osslibdir,
	    "etc/installed_drivers");

  if ((f = fopen (instfname, "w")) == NULL)
    {
      perror (instfname);
      exit (-1);
    }

  for (pass = 0; pass < MAX_PASS; pass++)
    for (i = 0; i < ndrivers; i++)
      if (drivers[i].pass == pass && drivers[i].detected)
	{
	  /* fprintf (f, "%s #%s\n", drivers[i].driver, drivers[i].name); */
	  fprintf (f, "%s\n", drivers[i].driver);
	}

  fclose (f);

  exit (0);
}
