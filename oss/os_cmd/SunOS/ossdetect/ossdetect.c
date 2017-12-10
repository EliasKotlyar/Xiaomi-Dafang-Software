/*
 * Purpose: OSS device autodetection utility for Solaris
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
#include <string.h>
#include <libdevinfo.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <ctype.h>
#include <sys/systeminfo.h>

static int use_force = 0;
static char arch[32] = "";
static int install_imux = 0;
static int install_userdev = 0;
static char *safe_mode="";

/* List of all modules installed in the system (OSS and non-oss) */
#define MAX_MODS 512
static char *installed_modules[MAX_MODS];
static int nmods = 0;


FILE *drivers_f = NULL;

static const char *ossdevs[] = {
  "sndstat",
  "mixer",
  "dsp",
/*      "audio", */
  "midi",
  "sequencer",
  "music",
  NULL
};

static void
scan_devdir (char *path)
{
  int i, ok;
  DIR *dir;
  struct dirent *de;
  struct stat st;

  char fullname[256], *devpart, devclass[256], *s;
  char devname[256];

  if ((dir = opendir (path)) == NULL)
    {
      perror (path);
      exit (-1);
    }

  while ((de = readdir (dir)) != NULL)
    {
      if (de->d_name[0] == '.')
	continue;

      sprintf (fullname, "%s/%s", path, de->d_name);

      if (stat (fullname, &st) == -1)
	{
	  perror (fullname);
	  continue;
	}

      if (S_ISDIR (st.st_mode))
	{
	  scan_devdir (fullname);
	  continue;
	}

#if 1
      if (!S_ISCHR (st.st_mode))
	continue;
#endif

/*
 * Find the device name part (after ":").
 */
      devpart = fullname;
      while (*devpart)
	{
	  if (*devpart == ':')
	    {
	      devpart++;
	      break;
	    }

	  devpart++;
	}

      strcpy (devclass, devpart);
      s = devclass;
      while (*s)
	{
	  if (isdigit (*s))
	    *s = 0;
	  else
	    s++;
	}

/*
 * Check if this kind of devices are known by OSS
 */
      ok = 0;

      for (i = 0; !ok && ossdevs[i] != NULL; i++)
	if (strcmp (devclass, ossdevs[i]) == 0)
	  ok = 1;

      if (!ok)
	continue;

      sprintf (devname, "/dev/%s", devpart);
      unlink (devname);

      if (symlink (fullname, devname) == -1)
	{
	  perror (devname);
	}
    }

  closedir (dir);
}

typedef struct
{
  char *driver;
  char *name;
  char idlist[128];
  int nids;
  char *ids[32];
  int pass;
} module_def_t;

typedef struct
{
  char *key, *driver, *name;
  int is_3rdparty, reload;
} driver_def_t;

#define MAX_DRIVERS	1000
static driver_def_t drivers[MAX_DRIVERS];
static int ndrivers = 0;

#define MAX_MODULES	32
static module_def_t modules[MAX_MODULES];
static int nmodules = 0;

static int verbose = 0;

static void
install_driver (driver_def_t * drv, char *drv_id)
{
  int i, j;

  if (verbose > 0)
    printf ("Install module '%s' (%s)\n", drv->driver, drv_id);

  for (i = 0; i < nmodules; i++)
    if (strcmp (drv->driver, modules[i].driver) == 0)
      {
	for (j = 0; j < modules[i].nids; j++)
	  {
	    if (strcmp (drv_id, modules[i].ids[j]) == 0)
	      return;
	  }

	strcat (modules[i].idlist, " \"");
	strcat (modules[i].idlist, drv_id);
	strcat (modules[i].idlist, "\"");

	if (modules[i].nids < 32)
	  {
	    int n = modules[i].nids++;
	    modules[i].ids[n] = strdup (drv_id);
	  }
	return;
      }

  if (nmodules >= MAX_MODULES)
    {
      fprintf (stderr, "Too many OSS modules\n");
      exit (-1);
    }

  modules[nmodules].name = drv->name;
  modules[nmodules].driver = drv->driver;
  sprintf (modules[nmodules].idlist, "\"%s\"", drv_id);
  modules[nmodules].nids = 1;
  modules[nmodules].ids[0] = strdup (drv_id);
  modules[nmodules].pass = 0;

/*
 * Hot-pluggable modules should be loaded after other modules.
 */
  if (strcmp (drv->driver, "oss_usb") == 0)
    modules[nmodules].pass = 1;

  nmodules++;
}

static void
find_and_install_driver (char *key)
{

  int i;

  for (i = 0; i < ndrivers; i++)
    if (strcmp (drivers[i].key, key) == 0)
      {
	if (verbose > 0)
	  printf ("\nDetected %s: %s (driver=%s)\n",
		  key, drivers[i].name, drivers[i].driver);
	install_driver (&drivers[i], key);
	return;
      }
}

static int
check_name (char *key, char *realname)
{
  int i;

  if (realname == NULL)
    realname = key;

  for (i = 0; i < ndrivers; i++)
    if (strcmp (drivers[i].key, key) == 0)
      {
	if (verbose > 0)
	  printf ("\nDetected %s: %s (driver=%s)\n",
		  realname, drivers[i].name, drivers[i].driver);
	/*      install_driver(&drivers[i], realname); */
	install_driver (&drivers[i], key);
	return 1;
      }
  return 0;
}

static int
check_node (di_node_t node, int level, char *realname)
{
  int j, ok = 0;

  while (node != DI_NODE_NIL)
    {
      char *name = di_node_name (node), *p;
      char *cnames;
      int n, i;

      cnames = "?";

      if (verbose > 1)
	{
	  for (j = 1; j < level; j++)
	    printf ("\t");
	  printf ("%s ", name);
	}

      /* if (realname==NULL) */
      {
	realname = name;
      }

      if (check_name (name, realname))
	{
	  if (verbose > 1)
	    {
	      char *drv = di_driver_name (node);
	      char *bnd = di_binding_name (node);
	      if (drv == NULL)
		drv = "";
	      if (bnd == NULL)
		bnd = "";
	      printf ("*match (driver=%s, binding=%s, instance=%d)*", drv,
		      bnd, di_instance (node));
	    }
	  ok = 1;
	}

      if (verbose > 1)
	printf ("\n");

      n = di_compatible_names (node, &cnames);
      p = cnames;
      for (i = 0; i < n; i++)
	if (!ok)
	  {
	    if (verbose > 1)
	      {
		for (j = 1; j < level; j++)
		  printf ("\t");
		printf (" = %s ", p);
	      }

	    if (check_name (p, realname))
	      {
		if (verbose > 1)
		  {
		    char *drv = di_driver_name (node);
		    char *bnd = di_binding_name (node);
		    if (drv == NULL)
		      drv = "";
		    if (bnd == NULL)
		      bnd = "";
		    printf ("*match (driver=%s, binding=%s, instance=%d)*",
			    drv, bnd, di_instance (node));
		  }
		ok = 1;
	      }

	    if (verbose > 1)
	      printf ("\n");
	    p = p + strlen (p) + 1;
	  }

      if (ok)
	{			/* Handle properties */
	  di_prop_t prop = DI_PROP_NIL;

	  while ((prop = di_prop_next (node, prop)) != DI_PROP_NIL)
	    {
	      printf ("\tProp '%s'}n", di_prop_name (prop));
	    }
	}

      /* if (!ok) */
      check_node (di_child_node (node), level + 1, realname);

      node = di_sibling_node (node);
    }

  return ok;
}

static void
load_devlist (const char *fname, int is_3rdparty)
{
  FILE *f;
  char line[256], *p;
  char *driver, *key, *name;

  if ((f = fopen (fname, "r")) == NULL)
    {
      perror (fname);
      exit (-1);
    }

  while (fgets (line, sizeof (line) - 1, f) != NULL)
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

/*
 * PCI subvendor ID's are marked as pcsNN,MM in devices.list. Convert them
 * to pciNN,MM for Solaris
 */
      if (strncmp (key, "pcs", 3) == 0)
	key[2] = 'i';

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
      drivers[ndrivers].reload = 0;

      ndrivers++;
    }

  fclose (f);
}

static void
check_conf (char *modname, int is_virtual, char *options)
{
  char fname[256], drivername[256];
  struct stat st;
  FILE *f;

#ifdef sparc
  sprintf (fname, "/kernel/drv/%s.conf", modname);
  sprintf (drivername, "/kernel/drv/%s%s", arch, modname);
#else
  sprintf (fname, "/kernel/drv/%s.conf", modname);
  sprintf (drivername, "/kernel/drv/%s%s", arch, modname);
#endif

  /* fprintf(start_script, "/usr/sbin/modload %s\n", drivername); */

  if (!use_force)
    if (stat (fname, &st) != -1)	/* File already exists */
      return;
    else
      fprintf (stderr, "\n\nWarning! Config file %s was missing!\n\n", fname);

  if ((f = fopen (fname, "w")) == NULL)
    {
      perror (fname);
      exit (-1);
    }
  fprintf (f, "# Open Sound System configuration file\n");

  if (is_virtual)
    fprintf (f, "name=\"%s\" parent=\"pseudo\" instance=0%s;\n", modname,
	     options);
  else
    fprintf (f, "interrupt-priorities=9%s;\n", options);

  fclose (f);
}

static void
load_name_to_major (void)
{
  FILE *f;
  char line[256], *p;

  if ((f = fopen ("/etc/name_to_major", "r")) == NULL)
    return;

  while (fgets (line, sizeof (line), f) != NULL)
    {
      p = line;

      while (*p && *p != ' ')
	p++;
      *p = '\0';

      if (*line == '\0') continue;

      if (nmods < MAX_MODS)
	{
	  installed_modules[nmods++] = strdup (line);
	}

/*
 * Force reinstall of imux if it is currently installed in the system
 */
      if (strcmp (line, "oss_imux") == 0)
	install_imux = 1;
      if (strcmp (line, "oss_userdev") == 0)
	install_userdev = 1;
    }

  fclose (f);
}

static int
add_drv (char *name, char *drv, char *parms)
{
  char tmp[512];
  int ret;

  printf ("add_drv %s%s %s\n", safe_mode, parms, drv);
  sprintf (tmp, "add_drv %s%s %s", safe_mode, parms, drv);

  ret = system (tmp);

  if (ret != 0)
    fprintf (stderr, "%s\n", tmp);

  /* save the driver and name info in installed_drivers file except osscore */
  if (name != NULL)
    fprintf (drivers_f, "%s #%s\n", drv, name);

  return ret;
}

static void
load_license (char *fname)
{
  struct stat st;
  char cmd[256];

  if (stat (fname, &st) == -1)
    return;			/* Doesn't exist */

  if (stat ("/usr/sbin/osslic", &st) == -1)
    return;			/* No osslic utility in the system. No need to install license. */

  sprintf (cmd, "/usr/sbin/osslic -q %s", fname);
  system (cmd);
}

static void
forceload_drivers (char *fname)
{
  FILE *f;
  char line[256], *p;

  if ((f = fopen (fname, "r")) == NULL)
    return;

  while (fgets (line, sizeof (line), f) != NULL)
    {
      p = strchr (line, '\n');
      if (p) *p = '\0';
      find_and_install_driver (line);
    }
}

int
main (int argc, char *argv[])
{
  int i, pass, c;
  di_node_t root_node;
  char tmp[256];
  struct stat st;
  FILE *ptr;
  int pid;
  char *cmd = "/usr/sbin/modinfo | grep osscommon";

#if 0
  if (sysinfo (SI_ARCHITECTURE_K, tmp, sizeof (tmp)) == -1)
    {
      perror ("sysinfo SI_ARCHITECTURE_K");
      exit (-1);
    }
#else
  if (sysinfo (SI_ARCHITECTURE, tmp, sizeof (tmp)) == -1)
    {
      perror ("sysinfo SI_ARCHITECTURE");
      exit (-1);
    }
#endif

  load_name_to_major ();

#ifdef sparc
  if (strcmp (tmp, "sparc") == 0)
    sprintf (arch, "sparcv9/", tmp);
#else
  if (strcmp (tmp, "amd64") == 0)
    sprintf (arch, "%s/", tmp);
#endif

  while ((c = getopt (argc, argv, "vfiudlVS")) != EOF)
      switch (c)
	{
	case 'v':
	  verbose++;
	  break;
	case 'f':
	  use_force = 1;
	  break;
	case 'S': /* Safe mode */
	  safe_mode="-n ";
	  break;
	case 'i':
	  install_imux = 1;
	  break;
	case 'u':
	  install_userdev = 1;
	  break;
	case 'd': /* Obolete under Solaris. */
	  exit (0);
	  break;
	case 'l':
	  load_license ("/etc/oss/license.asc");
	  exit (0);
	  break;
	}

  load_license ("/etc/oss/license.asc");

  load_devlist ("/etc/oss/devices.list", 0);

  if ((drivers_f = fopen ("/etc/oss/installed_drivers", "w")) == NULL)
    {
      perror ("/etc/oss/installed_drivers");
      exit (-1);
    }

  if (stat ("/etc/oss_3rdparty", &st) != -1)
    load_devlist ("/etc/oss_3rdparty", 1);

  if ((root_node = di_init ("/", DINFOSUBTREE)) == DI_NODE_NIL)
    {
      printf ("di_init() failed\n");
    }

  check_node (root_node, 0, NULL);

  di_fini (root_node);

  /*
   * Force unconditional loading of the USB driver and few others to make
   * hotplugging possible.
   */
  forceload_drivers ("/etc/oss/forceload.conf");

  if (verbose > 3)
    {
      for (i = 0; i < nmodules; i++)
	{
	  printf ("Would add %s -i '%s'\n", modules[i].driver,
		  modules[i].idlist);
	}
      printf ("Skipping actual device installation\n");
      exit (0);
    }

  sync ();

/*
 * Unload drivers that appear to be loaded
 */
  for (i = 0; i < nmods; i++)
    {
      int j, ok = 0;

      for (j = 0; j < ndrivers && !ok; j++)
	{
	  if (strcmp (installed_modules[i], drivers[j].driver) == 0)
	    {
	      ok = 1;
	      if (strcmp (drivers[j].key, "$PSEUDO") == 0 ||
		  drivers[j].is_3rdparty)
		{
		  if (drivers[j].is_3rdparty)
		    printf ("Removed 3rd party driver %s\n",
			    drivers[j].driver);
		  drivers[j].reload = 1;
		}
	    }
	}

      if (!ok)
	continue;

      sprintf (tmp, "rem_drv %s\n", installed_modules[i]);
      printf ("rem_drv %s\n", installed_modules[i]);
      system (tmp);
      sync ();
    }

#if 0
/*
 * Tell the osscore module to go away by writing something to /dev/sndstat
 * (at this moment it doesn't matter what is written).
 */

  system ("rem_drv osscore");
  sync ();
#endif

  /* Find the osscommon module's PID and pass it to moduload to unload it */
  if ((ptr = popen (cmd, "r")) != NULL)
    {
      if (fscanf (ptr, "%d", &pid) == 1 && pid > 0)
	{
	  sprintf (tmp, "modunload -i %d", pid);
	  fprintf (stderr, "unloaded osscommon\n");
	  system (tmp);
	}
      pclose (ptr);
    }


/* Now start loading all the modules */
  check_conf ("osscore", 1, "");

  if (add_drv ("OSS Core Devices", "osscore", "-m '* 0666 root sys'") != 0)
    {
      fprintf (stderr, "Installing OSS (osscore) failed\n");
      exit (256);
    }

  for (pass = 0; pass < 2; pass++)
    for (i = 0; i < nmodules; i++)
      if (modules[i].pass == pass)
	{
	  check_conf (modules[i].driver, 0, "");
	  sprintf (tmp, "-m '* 0666 root sys' -i '%s'", modules[i].idlist);
	  if (add_drv (modules[i].name, modules[i].driver, tmp) != 0)
	    {
	      fprintf (stderr, "Installing OSS (%s) failed\n",
		       modules[i].driver);
	      exit (256);
	    }
	}

  if (install_imux)
    {
      check_conf ("oss_imux", 1, "");
      add_drv ("OSS Input Multiplexer", "oss_imux", "-m '* 0666 root sys'");
    }

  if (install_userdev)
    {
      check_conf ("oss_userdev", 1, "");
      add_drv ("OSS User space device driver", "oss_userdev", "-m '* 0666 root sys'");
    }

#ifdef sparc
  if (stat("/kernel/drv/sparcv9/oss_sadasupport", &st) != -1)
    if (stat("/kernel/misc/sparcv9/audiosup", &st) != -1)
#else
  if (stat("/kernel/drv/oss_sadasupport", &st) != -1)
    if (stat("/kernel/misc/audiosup", &st) != -1)
#endif
     {
  	check_conf ("oss_sadasupport", 1, "");
  	add_drv ("SADA emulation layer", "oss_sadasupport", "-m '* 0666 root sys'");
     }

  for (i = 0; i < ndrivers; i++)
    if (drivers[i].reload)
      if (!drivers[i].is_3rdparty)
	{
	  check_conf (drivers[i].driver, 1, "");
	  add_drv (drivers[i].name, drivers[i].driver,
		   "-m '* 0666 root sys'");
	}
      else
	{
	  char parms[1024];

	  if (strcmp (drivers[i].key, "$PSEUDO") == 0)
	    {
	      add_drv (drivers[i].name, drivers[i].driver,
		       "-m '* 0666 root sys'");
	      continue;
	    }

	  sprintf (parms, "-i '%s' -m '* 0666 root sys'", drivers[i].key);
	  printf ("Attempting to reload %s\n", drivers[i].driver);
	  add_drv (drivers[i].name, drivers[i].driver, parms);
	}

/*
 * Reload the previous default settings if they were ever saved.
 */
  if (stat ("/etc/oss/mixer.save", &st) != -1 ||
      stat ("/etc/oss/dspdevs.map", &st) != -1)
    system ("savemixer -L");

  fclose (drivers_f);
#if 0
  sync ();
  printf ("Restarting OSS - Please wait\r");
  fflush (stdout);
  system ("soundoff");
  system ("soundon");
  printf ("\n");
#endif
  return 0;
}
