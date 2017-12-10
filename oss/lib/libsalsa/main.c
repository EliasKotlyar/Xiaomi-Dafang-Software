/*
 *  Copyright (c) 2004 by Hannu Savolainen < hannu@opensound.com>
 *
 *   Parts of the code is derived from the alsa-lib package that is
 *   copyrighted by Jaroslav Kysela and the other ALSA team members.
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License version 2.1 as
 *   published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */
#include "local.h"

int mixer_fd = -1;
int alib_initialized = 0;

oss_sysinfo sysinfo = { 0 };
struct _snd_ctl
{
  oss_card_info info;
  oss_audioinfo ainfo;
};

int alib_verbose = 0;

int
alib_init (void)
{
  char *p;
  char *devmixer;

  if ((devmixer=getenv("OSS_MIXERDEV"))==NULL)
     devmixer = "/dev/mixer";

  if ((p = getenv ("ALIB_DEBUG")) != NULL)
    alib_verbose = atoi (p);

  if (mixer_fd == -1)
    {
      if ((mixer_fd = open (devmixer, O_RDONLY, 0)) == -1)
	return -errno;
    }

  if (ioctl (mixer_fd, SNDCTL_SYSINFO, &sysinfo) == -1)
    {
      perror ("SNDCTL_SYSINFO");
      return -errno;
    }

  return 0;
}

char alib_appname[64] = "salsa";

int
alib_appcheck (void)
{
  /*
   * Some programs are known to support OSS at the same time with ALSA.
   * Prevent them from using this library accidently.
   */

  static int done = 0;

  if (done)
    return 1;

  done = 1;

  typedef struct
  {
    char *name, *msg, *action;
  } prog_t;

  static const prog_t banned_programs[] = {
    {"artsd", "Please use 'artsd -a oss' instead.", NULL},
    {"kcontrol", NULL, NULL},
    {"kmid", NULL, NULL},
    {"krec", NULL, NULL},
    {"kplay", NULL, NULL},
    {"kamix", "Please use ossxmix instead", NULL},
    {"qamix", "Please use ossxmix instead", NULL},
    NULL
  };

  static const char *whitelist[] = {
    "esd",
    "kmix",
    "gnome-volume-control",
    "artsd",
    "xmms",
    "alsaplayer",
    "aplay",
    "alsamixer",
    "vkeybd",
    NULL
  };

  FILE *f;
  char tmp[64], *p, *cmd = tmp;
  int i;
  static int warned = 0;

  if ((f = fopen ("/proc/self/cmdline", "r")) == NULL)
    return 1;

  if (fgets (tmp, sizeof (tmp) - 1, f) == NULL)
    {
      fclose (f);
      return 1;
    }

  fclose (f);

  p = cmd;

  while (*p && (*p != ' ' && *p != '\n'))
    p++;
  *p = 0;

  p = cmd = tmp;
  while (*p)
    {
      if (*p == '/')
	cmd = p + 1;
      p++;
    }

  strcpy (alib_appname, cmd);

  for (i = 0; i < strlen (alib_appname); i++)
    if (alib_appname[i] < 'a' || alib_appname[i] > 'z')
      if (alib_appname[i] < 'A' || alib_appname[i] > 'Z')
	if (alib_appname[i] < '0' || alib_appname[i] > '9')
	  alib_appname[i] = '_';

  for (i = 0; banned_programs[i].name != NULL; i++)
    if (strcmp (banned_programs[i].name, cmd) == 0)
      {
	if (alib_verbose != 0)
	  {
	    return 1;
	  }
	fprintf (stderr,
		 "\n\n**************   WARNING   ***********************\n");
	fprintf (stderr, "This program (%s) should not use ALSA emulation\n",
		 cmd);
	if (banned_programs[i].msg != NULL)
	  fprintf (stderr, "%s\n", banned_programs[i].msg);
	fprintf (stderr,
		 "**************************************************\n\n");

	if (banned_programs[i].action != NULL)
	  {
	    if (fork () == 0)
	      {
		exit (system (banned_programs[i].action));
	      }
	    while (wait () != -1);
	    exit (1);
	  }
	return 0;
      }

  if (alib_verbose == 0)
    {
      int ok = 0;

      for (i = 0; !ok && whitelist[i] != NULL; i++)
	if (strcmp (cmd, whitelist[i]) == 0)
	  ok = 1;


      if (!ok)
	return 0;
    }

  if (!warned)
    {
      fprintf (stderr,
	       "\n\n********************   WARNING   *******************************\n");
      fprintf (stderr,
	       "Warning! %s uses ALSA emulation instead of the native OSS API\n",
	       cmd);
      fprintf (stderr,
	       "****************************************************************\n\n");
    }
  warned = 1;
  return 1;
}

typedef struct _snd_ctl_card_info
{
  oss_card_info *info;
} snd_ctl_card_info_t;

/**
 * \brief Try to determine the next card.
 * \param rcard pointer to card number
 * \result zero if success, otherwise a negative error code
 *
 * Tries to determine the next card from given card number.
 * If card number is -1, then the first available card is
 * returned. If the result card number is -1, no more cards
 * are available.
 */
int
snd_card_next (int *rcard)
{
  ALIB_INIT ();
  if (!alib_appcheck ())
    return -ENODEV;

  if (*rcard == -1)
    *rcard = 0;
  else
    *rcard = *rcard + 1;

  if (*rcard >= sysinfo.numcards)
    {
      *rcard = -1;
      return 0;
    }

  return 0;
}

/**
 * \brief Convert card string to an integer value.
 * \param string String containing card identifier
 * \return zero if success, otherwise a negative error code
 *
 * The accepted format is an integer value in ASCII representation
 * or the card identifier (the id parameter for sound-card drivers).
 */
int
snd_card_get_index (const char *string)
{
  dbg_printf ("snd_card_get_index(%s)\n", string);
  return -EINVAL;
}

/**
 * \brief Get card name from a CTL card info
 * \param obj CTL card info
 * \return card name
 */
const char *
snd_ctl_card_info_get_name (const snd_ctl_card_info_t * obj)
{
  return obj->info->longname;
}

/**
 * \brief get size of #snd_ctl_card_info_t
 * \return size in bytes
 */
size_t
snd_ctl_card_info_sizeof ()
{
  return sizeof (snd_ctl_card_info_t);
}

/**
 * \brief Opens a CTL
 * \param ctlp Returned CTL handle
 * \param name ASCII identifier of the CTL handle
 * \param mode Open mode (see #SND_CTL_NONBLOCK, #SND_CTL_ASYNC)
 * \return 0 on success otherwise a negative error code
 */
int
snd_ctl_open (snd_ctl_t ** ctlp, const char *name, int mode)
{
  int num;
  snd_ctl_t *ctl;

  ALIB_INIT ();
  if (!alib_appcheck ())
    return -ENODEV;

  *ctlp = NULL;

  if (strcmp (name, "default") == 0)
    num = 0;
  else
    {
      if (name[0] != 'h' && name[1] != 'w' && name[2] != ':')
	return -ENOENT;

      if (sscanf (name + 3, "%d", &num) != 1)
	return -ENOENT;
    }

  if (num < 0 || num >= sysinfo.numcards)
    return -ENXIO;


  if ((ctl = malloc (sizeof (*ctl))) == NULL)
    return -ENOMEM;

  memset (ctl, 0, sizeof (*ctl));
  ctl->info.card = num;
  if (ioctl (mixer_fd, SNDCTL_CARDINFO, &ctl->info) == -1)
    {
      perror ("SNDCTL_CARDINFO");
      fprintf (stderr, "Mixer fd was %d\n", mixer_fd);
      return -errno;
    }

  *ctlp = ctl;
  return 0;
}

/**
 * \brief close CTL handle
 * \param ctl CTL handle
 * \return 0 on success otherwise a negative error code
 *
 * Closes the specified CTL handle and frees all associated
 * resources.
 */
int
snd_ctl_close (snd_ctl_t * ctl)
{
  free (ctl);
  return 0;
}

/**
 * \brief Get card related information
 * \param ctl CTL handle
 * \param info Card info pointer
 * \return 0 on success otherwise a negative error code
 */
int
snd_ctl_card_info (snd_ctl_t * ctl, snd_ctl_card_info_t * info)
{

  memset (info, 0, sizeof (*info));
  info->info = &ctl->info;

  return 0;
}

/**
 * \brief Obtain the card name.
 * \param card Card number
 * \param name Result - card name corresponding to card number
 * \result zero if success, otherwise a negative error code
 */
int
snd_card_get_name (int card, char **name)
{
  char tmp[256];

  sprintf (tmp, "OSS%d", card);

  *name = strdup (tmp);
  return 0;
}

int
snd_card_get_longname (int card, char **name)
{
  oss_card_info ci;

  ci.card = card;
  if (ioctl (mixer_fd, SNDCTL_CARDINFO, &ci) == -1)
    return -errno;

  *name = strdup (ci.longname);
  return 0;
}

/**
 * \brief Get next PCM device number
 * \param ctl CTL handle
 * \param device current device on entry and next device on return
 * \return 0 on success otherwise a negative error code
 */
int
snd_ctl_pcm_next_device (snd_ctl_t * ctl, int *device)
{
  ALIB_INIT ();

  dbg_printf ("snd_ctl_pcm_next_device(%d)\n", *device);

  if (*device < 0)
    *device = 0;
  else
    {
      *device = *device + 1;
    }

  while (1)
    {
      if (*device < 0 || *device >= sysinfo.numaudios)
	{
	  *device = -1;
	  return 0;
	}

      ctl->ainfo.dev = *device;
      if (ioctl (mixer_fd, SNDCTL_AUDIOINFO, &ctl->ainfo) < 0)
	return -errno;

      if (ctl->ainfo.card_number == ctl->info.card)
	{
	  return 0;
	}

      *device = *device + 1;
    }

  *device = -1;
  return 0;
}

/**
 * \brief Get info about a PCM device
 * \param ctl CTL handle
 * \param info PCM device id/info pointer
 * \return 0 on success otherwise a negative error code
 */
int
snd_ctl_pcm_info (snd_ctl_t * ctl, snd_pcm_info_t * info)
{
  dbg_printf ("snd_ctl_pcm_info()\n");
  memset (info, 0, sizeof (*info));
  info->ainfo = &ctl->ainfo;
  return 0;
}

/**
 * \brief Get card mixer name from a CTL card info
 * \param obj CTL card info
 * \return card mixer name
 */
const char *
snd_ctl_card_info_get_mixername (const snd_ctl_card_info_t * obj)
{
  return obj->info->longname;
}

/**
 * \brief Get info about a RawMidi device
 * \param ctl CTL handle
 * \param info RawMidi device id/info pointer
 * \return 0 on success otherwise a negative error code
 */
int
snd_ctl_rawmidi_info (snd_ctl_t * ctl, snd_rawmidi_info_t * info)
{
  dbg_printf ("snd_ctl_rawmidi_info()\n");

  return 0;
}

/**
 * \brief Get next RawMidi device number
 * \param ctl CTL handle
 * \param device current device on entry and next device on return
 * \return 0 on success otherwise a negative error code
 */
int
snd_ctl_rawmidi_next_device (snd_ctl_t * ctl, int *device)
{
  dbg_printf ("snd_ctl_rawmidi_next_device()\n");

  if (*device < 0)
    *device = 0;
  else
    *device = -1;

  return 0;
}

/**
 * \brief Get card identifier from a CTL card info
 * \param obj CTL card info
 * \return card identifier
 */
const char *
snd_ctl_card_info_get_id (const snd_ctl_card_info_t * obj)
{
  dbg_printf ("snd_ctl_card_info_get_id()\n");

  return "snd_ctl_card_info_get_id";
}
