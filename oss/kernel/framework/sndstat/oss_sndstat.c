/*
 * Purpose: /dev/sndstat driver
 *
 * Description:
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

#include <oss_config.h>
#include <midi_core.h>

static char *sndstat_buf = NULL;
static int sndstat_len, sndstat_ptr;
static volatile int sndstat_busy = 0;
int riptide_notice = 0;		/* The Riptide driver will set this to 1 */

/*
 * All kind of status messages
 */
#define MAX_MESSAGE 50
static char *messages[MAX_MESSAGE];
static int nmessages;		/* # of display messages (/dev/sndstat) */

static int
put_status (const char *s)
{
  int l = strlen (s);

  if (sndstat_len + l >= 4000)
    return 0;

  memcpy (&sndstat_buf[sndstat_len], s, l);
  sndstat_len += l;

  return 1;
}

static int
put_status_int (unsigned int val, int radix)
{
  char buf[11], *rx = "%d";

  if (!val)
    return put_status ("0");

  if (radix == 16)
    rx = "%x";
  sprintf (buf, rx, val);

  return put_status (buf);
}

static void
init_status (void)
{
  /*
   * Write the status information to the sndstat_buf and update sndstat_len.
   * There is a limit of 4000 bytes for the data.
   */

  int i, p, missing_devs = 0;
  int notify = 0;
  extern char *oss_license_string;

  sndstat_ptr = 0;

  put_status ("OSS " OSS_VERSION_STRING);
  put_status (oss_license_string);
  put_status (" (C) 4Front Technologies 1996-2011\n");

  if (riptide_notice)
    put_status ("RipTide Driver (C) 2000, Conexant Systems, Inc.\n");

#ifdef LICENSED_VERSION
  oss_print_license (put_status, put_status_int);
#endif

#ifdef OSS_CONFIG_OPTIONS
  put_status ("\nSource configration options: ");
  put_status (OSS_CONFIG_OPTIONS);
  put_status ("\n");
#endif

#ifdef OSS_HG_INFO
  put_status ("\nHg revision: ");
  put_status (OSS_HG_INFO);
  put_status ("\n");
#endif

  if (nmessages > 0)
    {
      put_status ("\n");

      for (i = 0; i < nmessages; i++)
	{
	  if (!put_status (messages[i]))
	    return;
	}

      put_status ("\n");
    }

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
  {
#if defined(__FreeBSD__)
    extern char version[];
#endif

    put_status ("Kernel: ");
    put_status (version);
    put_status ("\n");
  }
#endif

#if 0
/*
 * This code is obsolete and not functional at this moment.
 */
  if (!put_status ("\nDevice objects:\n"))
    return;

  for (i = 0; i < oss_num_cards; i++)
    {
      char tmp[256];

      oss_get_cardinfo (i, tmp, sizeof (tmp) - 1);
      if (!put_status (tmp))
	return;
      if (!put_status ("\n"))
	return;
    }

#endif

  if (!put_status ("\nAudio devices:\n"))
    return;

  missing_devs = 0;

  if (audio_devfiles != NULL)
    for (i = 0; i < MAX_AUDIO_DEVFILES +100 && missing_devs < 40; i++)
      {
	int j, d;
#if 0
	if (i < num_audio_devfiles)
	if (audio_devfiles[i]->card_number != cardno)
	  {
	    put_status (" \n");
	    cardno = audio_devfiles[i]->card_number;
	  }
#endif
/*
 * New device numbering scheme may have /dev/dsp# devices in different
 * order than the order of devices in audio_devices[]. Find the right device
 * based on the adev->real_dev number. Note that device numbering may have
 * holes if devices have been removed after running ossdevlinks -f last time.
 */
	d = -1;
	for (j = 0; j < num_audio_devfiles; j++)
	  {
	    if (audio_devfiles[j]->real_dev == i)
	      {
		if (j != i)
		  notify = 1;
		d = audio_devfiles[j]->audio_devfile;
		break;
	      }
	  }

	if (d == -1)
	  {
	    missing_devs++;
	    continue;
	  }

	if (missing_devs > 0)	/* There is a hole in numbering */
	  for (j = i - missing_devs; j < i; j++)
	    {
	      notify = 1;
	      if (!put_status_int (j, 10))
		return;
	      if (!put_status (": (Undefined or removed device)\n"))
		return;
	    }
	missing_devs = 0;

	if (!put_status_int (i, 10))
	  return;
	if (!put_status (": "))
	  return;
	if (!audio_devfiles[d]->enabled || audio_devfiles[d]->unloaded)
	  put_status ("(");

	if (!put_status (audio_devfiles[d]->name))
	  return;
	if (!audio_devfiles[d]->enabled || audio_devfiles[d]->unloaded)
	  put_status (")");

	if ((audio_devfiles[d]->flags & ADEV_DUPLEX)
	    || (audio_devfiles[d]->flags & ADEV_NOINPUT)
	    || (audio_devfiles[d]->flags & ADEV_NOOUTPUT))
	  {
	    int nn = 0;

	    if (!put_status (" ("))
	      return;

	    if (audio_devfiles[d]->flags & ADEV_NOINPUT)
	      {
		if (nn++)
		  put_status (",");
		if (!put_status ("OUTPUT"))
		  return;
	      }

	    if (audio_devfiles[d]->flags & ADEV_NOOUTPUT)
	      {
		if (nn++)
		  put_status (",");
		if (!put_status ("INPUT"))
		  return;
	      }

	    if (audio_devfiles[d]->flags & ADEV_DUPLEX)
	      {
		if (nn++)
		  put_status (",");
		if (!put_status ("DUPLEX"))
		  return;
	      }
	    if (!put_status (")"))
	      return;
	  }

	if (!put_status ("\n"))
	  return;

	{
	  adev_t *adev = audio_devfiles[d];
	  int n = 0, single = 0;
	  if (adev->next_out == NULL)
	    single = 1;

	  while (adev != NULL)
	    {
	      if (adev->open_mode != 0)
		{
		  if (i < 10)
		    {
		      if (!put_status ("   "))
			return;
		    }
		  else
		    {
		      if (!put_status ("    "))
			return;
		    }

		  if (single)
		    {
		      put_status ("Opened ");
		      if (adev->open_mode & OPEN_READ)
			put_status ("IN");
		      if (adev->open_mode & OPEN_WRITE)
			put_status ("OUT");
		      put_status (" by ");
		    }
		  else
		    {
		      put_status ("Engine ");
		      put_status_int (n + 1, 10);
		      put_status (" opened ");
		      if (adev->open_mode & OPEN_READ)
			put_status ("IN");
		      if (adev->open_mode & OPEN_WRITE)
			put_status ("OUT");
		      put_status (" by ");
		    }

		  if (adev->pid != -1 || *adev->label != 0)
		    {
		      if (*adev->label != 0)
			{
			  if (!put_status (adev->label))
			    return;
			  if (!put_status ("/"))
			    return;
			}

		      put_status_int (adev->pid, 10);
		    }
		  else
		    {
		      if (!put_status ("unknown application"))
			return;
		    }

		  if (!put_status (" @ "))
		    return;
		  if (!put_status_int (adev->user_parms.rate, 10))
		    return;
		  if (!put_status ("/"))
		    return;
		  if (!put_status_int (adev->hw_parms.rate, 10))
		    return;
		  if (!put_status (" Hz"))
		    return;

#if 1
		  if (!put_status (" Fragment: "))
		    return;

		  if (!put_status (audio_show_latency (adev->engine_num)))
		    return;
#endif

		  if (!put_status ("\n"))
		    return;

		  if (*adev->song_name != 0)
		    {
		      if (i < 10)
			{
			  if (!put_status ("   "))
			    return;
			}
		      else
			{
			  if (!put_status ("    "))
			    return;
			}

		      if (!put_status ("Song name: "))
			return;
		      if (!put_status (adev->song_name))
			return;
		      if (!put_status ("\n"))
			return;
		    }
		}

	      adev = adev->next_out;
	      n++;
	    }
	}
      }

#ifdef CONFIG_OSS_MIDI
  if (!put_status ("\nMIDI devices:\n"))
    return;

  missing_devs = 0;
  for (i = 0; i < MAX_MIDI_DEV * 2 && missing_devs < 16; i++)
    {
      int j, d = -1;

      for (j = 0; j < num_mididevs; j++)
	if (midi_devs[j]->real_dev == i)
	  {
	    d = j;
	    if (j != i)
	      notify = 1;
	  }

      if (d == -1)
	{
	  missing_devs++;
	  continue;
	}

      for (j = i - missing_devs; j < i; j++)
	{
	  notify = 1;
	  if (!put_status_int (j, 10))
	    return;
	  if (!put_status (": (Unknown or removed device)\n"))
	    return;
	}
      missing_devs = 0;

      if (!put_status_int (i, 10))
	return;
      if (!put_status (": "))
	return;
      if (!midi_devs[d]->enabled || midi_devs[d]->unloaded)
	put_status ("(");
      if (!put_status (midi_devs[d]->name))
	return;
      if (!midi_devs[d]->enabled || midi_devs[d]->unloaded)
	put_status (")");
      if (!put_status ("\n"))
	return;

      if (midi_devs[d]->pid != -1)
	{
	  if (i < 10)
	    {
	      if (!put_status ("   "))
		return;
	    }
	  else
	    {
	      if (!put_status ("    "))
		return;
	    }
	  if (!put_status ("Open by "))
	    return;
	  if (!put_status_int (midi_devs[d]->pid, 10))
	    return;
	  if (*midi_devs[d]->cmd != 0)
	    {
	      if (!put_status ("/"))
		return;
	      if (!put_status (midi_devs[d]->cmd))
		return;
	    }
	  if (!put_status ("\n"))
	    return;
	  if (midi_devs[d]->d->ioctl)
	    {
	      oss_longname_t song_name;

	      if (midi_devs[d]->d->ioctl (d, SNDCTL_GETSONG,
					  (ioctl_arg) song_name) >= 0
		  && *song_name != 0)
		{
		  if (!put_status ("   Song name: "))
		    return;
		  if (!put_status (song_name))
		    return;
		  if (!put_status ("\n"))
		    return;
		}
	    }
	}
    }
#endif

#if 0
  /* TODO: No timer available at this moment */
  if (!put_status ("\nTimers:\n"))
    return;

  for (i = 0; i < oss_num_timers; i++)
    {
      if (!put_status_int (i, 10))
	return;
      if (!put_status (": "))
	return;
      if (!put_status (oss_timer_devs[i]->info.name))
	return;
      if (!put_status ("\n"))
	return;
    }
#endif

  if (!put_status ("\nMixers:\n"))
    return;

  missing_devs = 0;

  for (i = 0; i < MAX_MIXER_DEV * 2 && missing_devs < 10; i++)
    {
      int j, d = -1;

      for (j = 0; j < num_mixers; j++)
	if (mixer_devs[j]->real_dev == i)
	  {
	    if (j != i)
	      notify = 1;
	    d = j;
	  }

      if (d == -1)
	{
	  missing_devs++;
	  continue;
	}

      for (j = i - missing_devs; j < i; j++)
	{
	  notify = 1;
	  if (!put_status_int (j, 10))
	    return;
	  if (!put_status (": (Uninstalled or removed device\n"))
	    return;
	}
      missing_devs = 0;

      if (!put_status_int (i, 10))
	return;
      if (!put_status (": "))
	return;

      if (!mixer_devs[d]->enabled || mixer_devs[d]->unloaded)
	if (!put_status ("("))
	  return;

      if (!put_status (mixer_devs[d]->name))
	return;

      if (!mixer_devs[d]->enabled || mixer_devs[d]->unloaded)
	if (!put_status (")"))
	  return;

      if (!put_status ("\n"))
	return;
    }

#if 1
  p = 0;
  for (i = 0; i < OSS_HISTORY_SIZE; i++)
    if (*oss_history[i] != 0)
      p++;

  if (p > 0)
#ifdef GET_PROCESS_UID
    if (GET_PROCESS_UID () == 0)
#endif
      {
	if (!put_status ("\nHistory:\n"))
	  return;

	p = oss_history_p;

	for (i = 0; i < OSS_HISTORY_SIZE; i++)
	  {
	    int ix = (p + i) % OSS_HISTORY_SIZE;

	    if (*oss_history[ix] == 0)
	      continue;

	    if (!put_status (oss_history[ix]))
	      return;
	    if (!put_status ("\n"))
	      return;
	  }
      }
#endif
#if 0
  if (!put_status
      ("\n\nNOTICE! This /dev/sndstat file is obsolete - use the ossinfo command instead\n"))
    return;
#endif
  if (notify)
    {
      if (!put_status
	  ("\n\nWARNING! Legacy device numbering in /dev/sndstat is different from actual device numbering\n"))
	return;
    }

  put_status ("\n\nNOTICE! Device numbers shown above may be wrong.\n");
  put_status ("        Use the ossinfo command to find out the correct device names.\n");

  sndstat_buf[sndstat_len] = 0;
}

void
store_msg (char *msg)
{
  char *s;

  if (strlen (msg) > 100 || nmessages >= MAX_MESSAGE)
    return;

  s = PMALLOC (NULL, strlen (msg) + 1);
  if (s == NULL)
    {
      return;
    }
  strcpy (s, msg);
  messages[nmessages++] = s;
}

static int
read_status (uio_t * buf, int count)
{
  /*
   * Return at most 'count' bytes from the sndstat_buf.
   */
  int l, c;

  l = count;
  c = sndstat_len - sndstat_ptr;

  if (l > c)
    l = c;
  if (l <= 0)
    return 0;

  if (uiomove (&sndstat_buf[sndstat_ptr], l, UIO_READ, buf) != 0)
    return OSS_EFAULT;
  sndstat_ptr += l;

  return l;
}

/*ARGSUSED*/
static int
sndstat_open (int dev, int dev_class, struct fileinfo *file,
	      int recursive, int open_flags, int *redirect)
{
  /* TODO: Concurrency control */
  if (sndstat_busy)
    return OSS_EBUSY;
  sndstat_busy = 1;

  if ((sndstat_buf = KERNEL_MALLOC (4096)) == NULL)
    {
      sndstat_busy = 0;
      return OSS_ENOMEM;
    }

  sndstat_len = 0;
  init_status ();
  sndstat_ptr = 0;

  return 0;
}

/*ARGSUSED*/
static void
sndstat_close (int dev, struct fileinfo *file)
{
  KERNEL_FREE (sndstat_buf);
  sndstat_buf = NULL;
  sndstat_busy = 0;
}

/*ARGSUSED*/
static int
sndstat_read (int dev, struct fileinfo *file, uio_t * buf, int count)
{
  int l;
  l = read_status (buf, count);
  return l;
}

/*ARGSUSED*/
static int
sndstat_write (int dev, struct fileinfo *file, uio_t * buf, int count)
{
/*
 * This dummy write routine will be used for some internal management purposes
 * in the future. At this moment it just tells the osscore module that it 
 * should permit detaching itself.
 */
#ifdef sun
  extern int oss_detach_enabled;
  oss_detach_enabled = 1;
  return count;
#else
  return OSS_EIO;
#endif
}

/*ARGSUSED*/
static int
sndstat_ioctl (int dev, struct fileinfo *bogus,
	       unsigned int cmd, ioctl_arg arg)
{
  if (cmd == OSS_GETVERSION)
    return *arg = OSS_VERSION;

  return OSS_EINVAL;
}

static oss_cdev_drv_t sndstat_cdev_drv = {
  sndstat_open,
  sndstat_close,
  sndstat_read,
  sndstat_write,
  sndstat_ioctl
};

void
install_sndstat (oss_device_t * osdev)
{
  //static int already_installed=0;

  // if (!already_installed++) // TODO: Is it necessaary to prevent loading sndstat multiple times?
  oss_install_chrdev (osdev, "sndstat", OSS_DEV_STATUS, 0, &sndstat_cdev_drv,
		      0);
}
