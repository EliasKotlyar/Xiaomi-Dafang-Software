/*
 * This software module makes it possible to use Open Sound System for Linux
 * (the _professional_ version) as a low level driver source for ALSA.
 *
 * Copyright (C) 2004-2009 Hannu Savolainen (hannu@opensound.com).
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

/*
 * !!!!!!!!!!!!!!!!!!!! Important  !!!!!!!!!!!!!!!!!!
 *
 * If this file doesn't compile, you must not try to resolve the problem
 * without perfect understanding of internals of Linux kernel, ALSA and
 * Open Sound System.
 *
 * Instead you need to check that you are using the version of this file
 * that matches the versions of ALSA, OSS and Linux you are currently using.
 */

#ifndef KBUILD_MODNAME
#define KBUILD_MODNAME cuckoo
#endif

#include "cuckoo.h"

#include "./checksum.h"

#ifdef VERMAGIC_STRING
static const char vermagic[] = VERMAGIC_STRING;
#endif

MODULE_AUTHOR ("Hannu Savolainen <hannu@opensound.com>");
MODULE_LICENSE ("GPL v2");
//MODULE_CLASSES("{sound}");
MODULE_DESCRIPTION ("OSS low level driver interface for ALSA");

#define CUCKOO_MAXCARD	SNDRV_CARDS
static int index[CUCKOO_MAXCARD] = SNDRV_DEFAULT_IDX;
static char *id[CUCKOO_MAXCARD] = SNDRV_DEFAULT_STR;
static int enable[CUCKOO_MAXCARD] = SNDRV_DEFAULT_ENABLE_PNP;

static int
snd_cuckoo_free (cuckoo_t * chip)
{
  // TODO
  return 0;
}

static int
snd_cuckoo_dev_free (snd_device_t * device)
{
  cuckoo_t *cuckoo = (cuckoo_t *) device->device_data;
  return snd_cuckoo_free (cuckoo);
}

static int
snd_cuckoo_create (snd_card_t * card, int osscard, cuckoo_t ** rchip)
{
  cuckoo_t *chip;
  int err;

  static snd_device_ops_t ops = {
    .dev_free = snd_cuckoo_dev_free
  };

  *rchip = NULL;

  if ((chip = (cuckoo_t *) kmalloc (sizeof (cuckoo_t), GFP_KERNEL)) == NULL)
    return -ENOMEM;

  chip->card = card;
  chip->osscard = osscard;
  chip->ncapture = chip->nplay = chip->npcm = 0;

  if ((err = snd_device_new (card, SNDRV_DEV_LOWLEVEL, chip, &ops)) < 0)
    {
      snd_cuckoo_free (chip);
      return err;
    }

  *rchip = chip;
  return 0;
}

static snd_card_t *cards[SNDRV_CARDS];
static int ncards = 0;

int
init_module (void)
{
  int err;
  int dev, cardno;
  char tmp[100];
  int pass;

#if 0
  // TODO
  if ((err = udi_connect (WRAPPER_VERSION)) < 0)
    return err;

  if (strcmp (oss_checksum, cuckoo_checksum) != 0)
    {
      printk
	("cuckoo: Error OSS incompatibility problem. Please recompile.\n");
      return -EIO;
    }
#endif

  for (pass = 0; pass < 2; pass++)
    {
      cardno = -1;

      for (dev = 0; dev < num_audio_engines; dev++)
	{
	  adev_p adev = audio_engines[dev];
	  cuckoo_t *chip;
	  snd_card_t *card = NULL;

	  if (pass == 0)
	    {
	      // Ignore non-virtual devices
	      if (!(adev->flags & ADEV_VIRTUAL))
		continue;
	    }
	  else
	    {
	      // Ignore virtual devices
	      if ((adev->flags & ADEV_VIRTUAL))
		continue;
	    }

	  if (adev->card_number < 0)
	    {
	      printk ("cuckoo: Ignored audio device %d - %s\n", dev,
		      adev->name);
	      continue;
	    }

	  if (adev->card_number != cardno)
	    {
	      oss_card_info cd;

	      cardno = adev->card_number;

	      if (oss_get_cardinfo (cardno, &cd) < 0)
		{
		  printk ("oss_get_cardinfo(%d) failed\n", cardno);
		  continue;
		}

	      // printk("Card %d: %s/%s\n", cardno, cd.shortname, cd.longname);
	      printk ("Card %d: %s/%s\n", cardno, cd.shortname, cd.longname);

	      if (ncards >= CUCKOO_MAXCARD)
		{
		  printk
		    ("Cuckoo: Too many audio devices (%d), only %d supported. by ALSA.\n",
		     num_audio_engines, CUCKOO_MAXCARD);
		  return -EIO;
		}

	      if (!enable[ncards])
		{
		  printk ("cuckoo: Device was not enabled (yet)\n");
		  return -EIO;
		}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31)
	      if ((card =
		   snd_card_new (index[ncards], id[ncards], THIS_MODULE,
				 0)) == NULL)
#else
	      if (
		   snd_card_create (index[ncards], id[ncards], THIS_MODULE,
				 0, &card) != 0)
#endif
		{
		  printk ("cuckoo: Can't create a card instance\n");
		  return -EIO;
		}

	      if ((err = snd_cuckoo_create (card, cardno, &chip)) < 0)
		{
		  printk ("cuckoo: Couldn't create a chip instance (%d)\n",
			  err);
		  snd_card_free (card);
		  return err;
		}

#define oss_version_string "v4.x"	// TODO
	      sprintf (tmp, "OSS %s", oss_version_string);
	      strlcpy (card->driver, tmp);
	      strlcpy (card->shortname, cd.shortname);
	      strlcpy (card->longname, cd.longname);

	      if ((err = install_pcm_instances (chip, cardno)) < 0)
		return err;

	      if ((err = install_mixer_instances (chip, cardno)) < 0)
		return err;

	      //              if ((err=install_midiport_instances(chip, cardno))<0)
	      //                 return err;

	      if ((err = snd_card_register (card)) < 0)
		{
		  printk ("cuckoo: Couldn't register card(%s) err=%d\n",
			  card->shortname, err);
		  continue;	// TODO: Should handle this in more intelligent way

		  snd_card_free (card);
		  return err;
		}

	      cards[ncards++] = card;
	    }
	}
    }

  return 0;
}

void
cleanup_module (void)
{
  int i;

  for (i = 0; i < ncards; i++)
    snd_card_free (cards[i]);
}

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,5))
#undef unix
struct module __this_module
  __attribute__ ((section (".gnu.linkonce.this_module"))) =
{
  .name = __stringify (KBUILD_MODNAME),.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
    .exit = cleanup_module
#endif
};
#endif
