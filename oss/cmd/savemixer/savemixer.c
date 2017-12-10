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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <oss_config.h>
#include <sys/ioctl.h>

static void change_mixer (const char *, char *);
static int find_mixerdev (const char *);
static char * get_mapname (void);
#ifdef APPLIST_SUPPORT
static void load_applist (void);
#endif
static void load_config (const char *);
static void open_device (const char *, int);
#ifdef MANAGE_DEV_DSP
static void reorder_dspdevs (void);
#endif

#define ETCDIRLEN 512
#define SLINELEN 256

static char ossetcdir[ETCDIRLEN] = "/usr/lib/oss/etc";
	/* This is the usual place */
static oss_mixerinfo mixerinfo;
static oss_mixext *mixerdefs = NULL;
static int fd, load_settings = 0, nummixers, verbose = 0;

#ifdef MANAGE_DEV_DSP
static void
reorder_dspdevs (void)
{
  char line[1024], *p, *s;
  oss_reroute_t reroute[3] = { {0} };
  int i, j, n, m;
  FILE *f;

  snprintf (line, sizeof (line), "%s/dspdevs.map", ossetcdir);

  if ((f = fopen (line, "r")) == NULL)
    return;

  n = 0;
  while (n < 3 && (fgets (line, sizeof (line), f) != NULL))
    {
      s = strchr (line, '\n');
      if (s != NULL) *s = '\0';

      m = 0;
      s = line;
      while (*s)
	{
	  int v;

	  p = s;

	  while (*p && *p != ' ')
	    p++;
	  if (*p)
	    *p++ = '\0';

	  if (m > MAX_AUDIO_DEVFILES || sscanf (s, "%d", &v) != 1)
	    {
	      fprintf (stderr, "Bad info in dspdevs.map\n");
	      fclose (f);
	      return;
	    }
	  while (*p == ' ')
	    p++;

	  s = p;

	  reroute[n].devlist.devices[m++] = v;
	  reroute[n].devlist.ndevs = m;
	}
      n++;
    }
  fclose (f);

  for (i = 0; i < n; i++)
    {
      reroute[i].mode = i + 1;

      if (ioctl (fd, OSSCTL_SET_REROUTE, &reroute[i]) == -1)
	{
	  if (errno == EINVAL)
	    {
	      fprintf (stderr,
		       "Device configuration changed - use ossdevlinks "
		       "to update device lists\n");
	      return;
	    }

	  perror ("reroute");
	  return;
	}

      if (verbose)
	{
	  switch (i + 1)
	    {
	    case OSS_OPEN_READ:
	      fprintf (stdout, "/dev/dsp input assignment: ");
	      break;
	    case OSS_OPEN_WRITE:
	      fprintf (stdout, "/dev/dsp output assignment: ");
	      break;
	    case OSS_OPEN_READ | OSS_OPEN_WRITE:
	      fprintf (stdout, "/dev/dsp output assignment: ");
	      break;
	    }

	  for (j = 0; j < reroute[i].devlist.ndevs; j++)
	    fprintf (stdout, "%d ", reroute[i].devlist.devices[j]);
	  fprintf (stdout, "\n");
	}
    }

}
#endif

#ifdef APPLIST_SUPPORT
static void
load_applist (void)
{
  char line[1024];

  FILE *f;

  snprintf (line, sizeof (line), "%s/applist.conf", ossetcdir);

  if ((f = fopen (line, "r")) == NULL)
    return;

  if (ioctl (fd, OSSCTL_RESET_APPLIST, NULL) == -1)
    {
      perror ("OSSCTL_RESET_APPLIST");
      fclose (f);
      return;
    }

  while (fgets (line, sizeof (line), f) != NULL)
    {
      int i, j;
      char *s, *name, *mode, *dev, *flag;
      app_routing_t rout;

      if ((*line == '#') || (*line == '\0'))
	continue;

      memset (&rout, 0, sizeof (rout));

      j = strlen (line);
      for (i = 0; i < j; i++)
	if (line[i] == '\n' || line[i] == '#')
	  {
  	    line[i] = '\0';
	    break;
	  }

      s = name = line;

      /* Find the field separator (LWSP) */
      while (*s && (*s != ' ' && *s != '\t'))
	s++;
      while (*s == ' ' || *s == '\t')
	*s++ = '\0';

      strncpy (rout.name, name, 32);
      rout.name[32] = '\0';

      mode = s;

      /* Find the field separator (LWSP) */
      while (*s && (*s != ' ' && *s != '\t'))
	s++;
      while (*s == ' ' || *s == '\t')
	*s++ = '\0';

      j = strlen (mode);
      for (i = 0; i < j; i++)
	switch (mode[i])
	  {
	  case 'r':
	    rout.mode |= OSS_OPEN_READ;
	    break;
	  case 'w':
	    rout.mode |= OSS_OPEN_WRITE;
	    break;

	  default:
	    fprintf (stderr, "Bad open mode flag '%c' in applist.conf\n",
		     mode[i]);
	  }

      dev = s;

      /* Find the field separator (LWSP) */
      while (*s && (*s != ' ' && *s != '\t'))
	s++;
      while (*s == ' ' || *s == '\t')
	*s++ = '\0';

      if (sscanf (dev, "%d", &rout.dev) != 1)
	{
	  fprintf (stderr, "bad audio device number '%s' in applist.conf\n",
		   dev);
	  continue;
	}

      while (*s)
	{
	  flag = s;

	  while (*s && *s != '|')
	    s++;
	  while (*s == '|')
	    *s++ = '\0';

	  if (strcmp (flag, "MMAP") == 0)
	    {
	      rout.open_flags |= OF_MMAP;
	      continue;
	    }

	  if (strcmp (flag, "BLOCK") == 0)
	    {
	      rout.open_flags |= OF_BLOCK;
	      continue;
	    }

	  if (strcmp (flag, "NOCONV") == 0)
	    {
	      rout.open_flags |= OF_NOCONV;
	      continue;
	    }

	  fprintf (stderr, "Bad option '%s' in applist.conf\n", flag);
	}

      if (ioctl (fd, OSSCTL_ADD_APPLIST, &rout) == -1)
	{
	  if (errno != ENXIO)
	    perror ("OSSCTL_ADD_APPLIST");
	}
    }

  fclose (f);
}
#endif

static char *
get_mapname (void)
{
  FILE *f;
  char tmp[ETCDIRLEN+11]; /* Adding 'OSSLIBDIR=' */
  static char name[ETCDIRLEN+15]; /* Adding '/etc/mixer.save' */
  struct stat st;

  if (stat ("/etc/oss", &st) != -1)	/* Use /etc/oss/mixer.save */
    {
      strcpy (name, "/etc/oss/mixer.save");
      strcpy (ossetcdir, "/etc/oss");
      return name;
    }

  if ((f = fopen ("/etc/oss.conf", "r")) == NULL)
    {
      // perror ("/etc/oss.conf");
      goto dexit;
    }

  while (fgets (tmp, sizeof (tmp), f) != NULL)
    {
      size_t l = strlen (tmp);
      if (l > 0 && tmp[l - 1] == '\n')
	tmp[l - 1] = '\0';

      if (strncmp (tmp, "OSSLIBDIR=", 10) == 0)
	{
	  l = snprintf (name, sizeof (name), "%s/etc/mixer.save", &tmp[10]);
	  if ((l >= sizeof (name)) || (l < 0))
	    {
	      fprintf (stderr, "String in /etc/oss.conf is too long!\n");
	      goto oexit;
	    }
	  snprintf (ossetcdir, sizeof (ossetcdir), "%s/etc", &tmp[10]);
	  if ((l >= sizeof (ossetcdir)) || (l < 0))
	    {
	      fprintf (stderr, "String in /etc/oss.conf is too long!\n");
	      goto oexit;
	    }
	  fclose (f);
	  return name;
	}
    }

  fclose (f);
  fprintf (stderr, "Error: OSSLIBDIR not set in /etc/oss.conf\n");

dexit:
  snprintf (name, sizeof (name), "%s/mixer.save", ossetcdir);
  return name;

oexit:
  fclose (f);
  exit (-1);
}

static int
find_mixerdev (const char *handle)
{
/*
 * Find the mixer device (number) which matches the given handle.
 */

  int i;

  if (mixerdefs != NULL)
    free (mixerdefs);
  mixerdefs = NULL;

  for (i = 0; i < nummixers; i++)
    {
      int j;

      mixerinfo.dev = i;

      if (ioctl (fd, SNDCTL_MIXERINFO, &mixerinfo) == -1)
	{
	  perror ("SNDCTL_MIXERINFO");
	  exit (-1);
	}

      if (strcmp (mixerinfo.handle, handle) == 0)	/* Match */
	{
	  mixerdefs =
            (oss_mixext *)malloc (sizeof (*mixerdefs) * mixerinfo.nrext);
	  if (mixerdefs == NULL)
	    {
	      fprintf (stderr, "Out of memory\n");
	      exit (-1);
	    }

	  for (j = 0; j < mixerinfo.nrext; j++)
	    {
	      oss_mixext *ext = mixerdefs + j;

	      ext->dev = i;
	      ext->ctrl = j;

	      if (ioctl (fd, SNDCTL_MIX_EXTINFO, ext) == -1)
		{
		  perror ("SNDCTL_MIX_EXTINFO");
		  exit (-1);
		}
	    }

	  return i;
	}
    }

  return -1;
}

static void
change_mixer (const char *fname, char *line)
{
  unsigned int value, i;
  char name[SLINELEN];

  if (sscanf (line, "%s %x", name, &value) != 2)
    {
      fprintf (stderr, "Bad line in %s\n", fname);
      fprintf (stderr, "%s\n", line);
    }

  for (i = 0; i < mixerinfo.nrext; i++)
    {
      oss_mixext *ext = mixerdefs + i;
      oss_mixer_value val;

      if (strcmp (ext->extname, name) == 0)
	{

	  if (!(ext->flags & MIXF_WRITEABLE))
	    continue;

	  if (ext->type == MIXT_GROUP)
	    continue;
	  if (ext->type == MIXT_DEVROOT)
	    continue;
	  if (ext->type == MIXT_MARKER)
	    continue;

	  val.dev = ext->dev;
	  val.ctrl = ext->ctrl;
	  val.timestamp = ext->timestamp;
	  val.value = value;

	  if (ioctl (fd, SNDCTL_MIX_WRITE, &val) == -1)
	    {
	      perror ("SNDCTL_MIX_WRITE");
	      fprintf (stderr, "%s (%d)=%04x\n", name, val.ctrl, value);
	      continue;
	    }
	  return;
	}
    }
}

static void
load_config (const char *name)
{
  FILE *f;
  char line[SLINELEN], *s;
  int dev = -1;

  if (verbose) fprintf (stdout, "Loading mixer settings from %s\n", name);
#ifdef MANAGE_DEV_DSP
  reorder_dspdevs ();
#endif
#ifdef APPLIST_SUPPORT
  load_applist ();
#endif

  if ((f = fopen (name, "r")) == NULL)
    {
      /* Nothing to do */
      exit (0);
    }

  /* Remove the EOL character */
  while (fgets (line, sizeof (line), f) != NULL)
    {

      if ((s = strchr (line, '\n')) != NULL)
	*s = '\0';

      if ((*line == '\0') || (*line == '#'))
	continue;

      if (*line == '$')
	{
	  if (strcmp (line, "$endmix") == 0)
	    continue;		/* Ignore this */

	  s = line;

	  while (*s && *s != ' ')
	    s++;
	  if (*s == ' ')
	    *s++ = 0;

	  if (strcmp (line, "$startmix") != 0)
	    continue;

	  dev = find_mixerdev (s);

	  continue;
	}

      if (dev < 0)		/* Unknown mixer device? */
	continue;

      change_mixer (name, line);
    }

  fclose (f);
}

static void
open_device (const char * dev_name, int mode)
{
  if ((fd = open (dev_name, mode, 0)) == -1)
    {
      if (errno != ENODEV)
	perror (dev_name);
      exit (-1);
    }

  if (ioctl (fd, SNDCTL_MIX_NRMIX, &nummixers) == -1)
    {
      perror ("SNDCTL_MIX_NRMIX");
      fprintf (stderr, "Possibly incompatible OSS version\n");
      exit (-1);
    }

  if (nummixers < 1)
    {
      fprintf (stderr, "No mixers in the system\n");
      exit (-1);
    }

}

int
main (int argc, char *argv[])
{
  int dev, i;
  char * mapname = NULL;
  extern char * optarg;

  FILE *f;

  while ((i = getopt (argc, argv, "LVf:v")) != EOF)
    {
      switch (i)
	{
	case 'v':
	  verbose++;
	  break;

	case 'L':
	  load_settings = 1;
	  break;

	case 'V':
	  fprintf (stdout, "OSS " OSS_VERSION_STRING " savemixer utility\n");
	  exit (0);
	  break;

	case 'f':
	  mapname = optarg;
	  break;

	default:
	  fprintf (stdout, "Usage: %s [option(s)]\n", argv[0]);
	  fprintf (stdout, "  Options:  -L           Restore mixer settings\n");
	  fprintf (stdout, "            -V           Display version\n");
	  fprintf (stdout, "            -f<fname>    Use fname as settings "
		   "file\n");
	  fprintf (stdout, "            -v           Verbose output\n");
	  exit (-1);
	}
    }

  if (mapname == NULL) mapname = get_mapname ();

  if (load_settings)
    {
      open_device ("/dev/mixer", O_WRONLY);
      load_config (mapname);
      exit (0);
    }

  open_device ("/dev/mixer", O_RDONLY);

  if ((f = fopen (mapname, "w")) == NULL)
    {
      perror (mapname);
      exit (-1);
    }
  fprintf (f, "# Automatically generated by OSS savemixer - do not edit\n");

  if (verbose) fprintf (stdout, "Saving mixer settings to %s\n", mapname);

  for (dev = 0; dev < nummixers; dev++)
    {
      mixerinfo.dev = dev;

      if (ioctl (fd, SNDCTL_MIXERINFO, &mixerinfo) == -1)
	{
	  perror ("SNDCTL_MIXERINFO");
	  exit (-1);
	}

      fprintf (f, "\n# %s\n", mixerinfo.name);
      fprintf (f, "$startmix %s\n", mixerinfo.handle);

      for (i = 0; i < mixerinfo.nrext; i++)
	{
	  oss_mixext ext;
	  oss_mixer_value val;

	  ext.dev = dev;
	  ext.ctrl = i;

	  if (ioctl (fd, SNDCTL_MIX_EXTINFO, &ext) == -1)
	    {
	      perror ("SNDCTL_MIX_EXTINFO");
	      exit (-1);
	    }

	  if (!(ext.flags & MIXF_WRITEABLE))
	    continue;

	  if (ext.type == MIXT_GROUP)
	    continue;
	  if (ext.type == MIXT_DEVROOT)
	    continue;
	  if (ext.type == MIXT_MARKER)
	    continue;

	  val.dev = dev;
	  val.ctrl = i;
	  val.timestamp = ext.timestamp;

	  if (ioctl (fd, SNDCTL_MIX_READ, &val) == -1)
	    {
	      perror ("SNDCTL_MIX_READ");
	      continue;
	    }

	  fprintf (f, "%s %04x\n", ext.extname, val.value);
	}

      fprintf (f, "$endmix\n");
    }
  fclose (f);

  return 0;
}
