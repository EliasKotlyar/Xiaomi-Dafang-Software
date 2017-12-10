/*
 * Purpose: OSS device autodetection utility for Linux
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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/dir.h>

#define PCI_PASS	0
#define USB_PASS	1
#define PSEUDO_PASS	2
#define MAX_PASS	3

#define OSSLIBDIRLEN 512
static char *osslibdir = NULL;

static int usb_ok = 0;

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
static int decode_descriptor (unsigned char *, int);
static void create_devlinks (mode_t);
static char * get_mapname (void);
static unsigned short get_uint16 (unsigned char *);
static int is_audio (unsigned char *, int);
static void load_license (const char *);
static void load_devlist (const char *, int);
static void pci_checkdevice (char *);
static void pci_detect (char *);
static drvlist_t * prepend_drvlist (const char *);
static int remove_devlinks (const char *);
static void usb_checkdevice (char *);
static void usb_scandir (char *);
static void usb_detect (void);

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
  if (((n = system (cmd)) == -1))
    fprintf (stderr, "Cannot run osslic!\n");
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
	    *p = '\0';
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

static unsigned short
get_uint16 (unsigned char *p)
{
  return *p + (p[1] << 8);
}

static int
decode_descriptor (unsigned char *d, int desclen)
{
  switch (d[1])
    {
    case 0x04:
      if (d[5] == 1)		// Audio
	return 1;
      break;
    }

  return 0;
}

static int
is_audio (unsigned char *desc, int datalen)
{
  int l, pos, desclen;

  if (desc[0] == 9 && desc[1] == 2)	/* Config descriptor */
    {
      l = get_uint16 (desc + 2);

      if (datalen > l)
	datalen = l;
    }

  pos = 0;

  while (pos < datalen)
    {
      unsigned char *d;
      desclen = desc[pos];

      if (desclen < 2 || desclen > (datalen - pos))
	{
	  return 0;
	}

      d = &desc[pos];
      if (decode_descriptor (d, desclen))
	return 1;
      pos += desclen;
    }

  return 0;
}

static void
usb_checkdevice (char *fname)
{
  int fd, l;
  unsigned char buf[4096];
  char tmp[64];
  int vendor, device;

  if ((fd = open (fname, O_RDONLY, 0)) == -1)
    {
      perror (fname);
      return;
    }

  if ((l = read (fd, buf, sizeof (buf))) == -1)
    {
      perror (fname);
      return;
    }

  close (fd);

  if (l < 12 || buf[1] != 1)
    return;

  vendor = buf[8] | (buf[9] << 8);
  device = buf[10] | (buf[11] << 8);

  sprintf (tmp, "usb%x,%x", vendor, device);
  if (add_drv (tmp, USB_PASS))
    return;

  sprintf (tmp, "usbif%x,%x", vendor, device);
  if (add_drv (tmp, USB_PASS))
    return;

  if (is_audio (buf, l))	/* USB audio class */
    {
      if (add_drv ("usbif,class1", USB_PASS))
	return;
    }
}

static void
usb_scandir (char *dirname)
{
  char path[PATH_MAX];
  DIR *dr;
  struct dirent *de;
  struct stat st;

  if ((dr = opendir (dirname)) == NULL)
    {
      return;
    }

  while ((de = readdir (dr)) != NULL)
    {
      if (de->d_name[0] < '0' || de->d_name[0] > '9')	/* Ignore non-numeric names */
	continue;

      snprintf (path, sizeof (path), "%s/%s", dirname, de->d_name);

      if (stat (path, &st) == -1)
	continue;

      if (S_ISDIR (st.st_mode))
	{
	  usb_scandir (path);
	  continue;
	}

      usb_checkdevice (path);
    }

  closedir (dr);
}

static void
usb_detect (void)
{
  struct stat st;
#if 0
  char path[OSSLIBDIRLEN + 25];

  sprintf (path, "%s/modules/oss_usb.o", osslibdir);

  if (stat (path, &st) == -1)	/* USB module not available */
    {
      fprintf (stderr, "USB module not available, aborting USB detect\n");
      return;
    }
#endif

  if (stat ("/dev/bus/usb", &st) != -1)
    {
      usb_ok = 1;
      usb_scandir ("/dev/bus/usb");
    }
  else if (stat ("/proc/bus/usb", &st) != -1)
    {
      usb_ok = 1;
      usb_scandir ("/proc/bus/usb");
    }
}

static void
pci_checkdevice (char *path)
{
  unsigned char buf[256];
  char id[32];
  int fd;
  int vendor, device;
  int subvendor, subdevice;

  if ((fd = open (path, O_RDONLY, 0)) == -1)
    {
      perror (path);
      return;
    }

  if (read (fd, buf, sizeof (buf)) != sizeof (buf))
    {
      perror (path);
      close (fd);
      return;
    }
  close (fd);
  vendor = buf[0] | (buf[1] << 8);
  device = buf[2] | (buf[3] << 8);
  subvendor = buf[0x2c] | (buf[0x2d] << 8);
  subdevice = buf[0x2e] | (buf[0x2f] << 8);
  sprintf (id, "pcs%x,%x", subvendor, subdevice);
  if (add_drv (id, PCI_PASS))
    return;
  sprintf (id, "pci%x,%x", vendor, device);
  add_drv (id, PCI_PASS);
}

static void
pci_detect (char *dirname)
{
  char path[PATH_MAX];
  DIR *dr;
  struct dirent *de;
  struct stat st;

  if (dirname == NULL)
    {
      dirname = "/proc/bus/pci";
    }

  if ((dr = opendir (dirname)) == NULL)
    {
      return;
    }

  while ((de = readdir (dr)) != NULL)
    {
      if (de->d_name[0] < '0' || de->d_name[0] > '9')	/* Ignore non-numeric names */
	continue;

      snprintf (path, sizeof (path), "%s/%s", dirname, de->d_name);

      if (stat (path, &st) == -1)
	continue;

      if (S_ISDIR (st.st_mode))
	{
	  pci_detect (path);
	  continue;
	}

      pci_checkdevice (path);
    }

  closedir (dr);
}

static int
remove_devlinks (const char * dirname)
{
  char path[PATH_MAX];
  DIR * dr;
  struct dirent * de;
  struct stat st;

  if ((dr = opendir (dirname)) == NULL)
    {
      if (errno == ENONET) return 0;
      perror ("opendir");
      return -1;
    }

  while ((de = readdir (dr)) != NULL)
    {
      if ((!strcmp (de->d_name, ".")) || (!strcmp (de->d_name, ".."))) continue;

      snprintf (path, sizeof (path), "%s/%s", dirname, de->d_name);

      if ((stat (path, &st) != -1) && (S_ISDIR (st.st_mode))) remove_devlinks (path);
      else
	{
	  if (verbose > 2) fprintf (stderr, "Removing %s\n", path);
	  unlink (path);
	}
    }

  closedir (dr);
  if (verbose > 2) fprintf (stderr, "Removing %s\n", path);
  if (rmdir (dirname) == -1)
    {
      fprintf (stderr, "Couldn't remove %s\n", path);
      return -1;
    }
  return 0;
}

static void
create_devlinks (mode_t node_m)
{
  FILE *f;
  char line[256], tmp[300], *p, *s;
  mode_t perm;
  int minor, major;

  if ((f = fopen ("/proc/opensound/devfiles", "r")) == NULL)
    {
      perror ("/proc/opensound/devfiles");
      fprintf (stderr, "Cannot connect to the OSS kernel module.\n");
      fprintf (stderr, "Perhaps you need to execute 'soundon' to load OSS\n");
      exit (-1);
    }

  remove_devlinks ("/dev/oss");
  perm = umask (0);
  mkdir ("/dev/oss", 0755);

  while (fgets (line, sizeof (line), f) != NULL)
    {
      char dev[64] = "/dev/";

      s = strchr (line, '\n');
      if (s) *s = '\0';

      if (sscanf (line, "%s %d %d", dev + 5, &major, &minor) != 3)
	{
	  fprintf (stderr, "Syntax error in /proc/opensound/devfiles\n");
	  fprintf (stderr, "%s\n", line);
	  exit (-1);
	}

/*
 * Check if the device is located in a subdirectory (say /dev/oss/sblive0/pcm0).
 */
      strcpy (tmp, dev);

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
	  *p = 0;		/* Extract the directory name part */
	  mkdir (tmp, 0755);
	}

      unlink (dev);
      if (verbose)
	printf ("mknod %s c %d %d -m %o\n", dev, major, minor, node_m);
      if (mknod (dev, node_m, makedev (major, minor)) == -1)
	perror (dev);
    }

  umask (perm);
  fclose (f);
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
  int i, do_license = 0, make_devs = 0, pass;
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

  pci_detect (NULL);
  usb_detect ();

  if (usb_ok)
    {
      if (verbose)
        printf ("USB support available in the system, adding USB driver\n");
      add_drv ("usbif,class1", USB_PASS);
    }
  else if (verbose)
    printf ("No USB support detected in the system - skipping USB\n");

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
	  fprintf (f, "%s #%s\n", drivers[i].driver, drivers[i].name);
	}

  fclose (f);

  exit (0);
}
