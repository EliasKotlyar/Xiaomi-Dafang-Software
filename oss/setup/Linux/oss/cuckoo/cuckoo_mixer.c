/*
 * This software module makes it possible to use Open Sound System for Linux
 * (the _professional_ version) as a low level driver source for ALSA.
 *
 * Copyright (C) 2004 Hannu Savolainen (hannu@voimakentta.net).
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

#include "cuckoo.h"

MODULE_AUTHOR ("Hannu Savolainen <hannu@opensound.com>");
MODULE_LICENSE ("GPL v2");
MODULE_DESCRIPTION ("OSS mixer low level driver interface for ALSA");

typedef struct
{
  char *name, *data;
} enum_entry_t;

#if 0
static void
downshift (char *s)
{
  while (*s)
    {
      if (*s >= 'A' && *s <= 'Z')
	*s += 32;
      s++;
    }
}
#endif

static int
get_mixer_info (snd_kcontrol_t * kcontrol, snd_ctl_elem_info_t * uinfo)
{
  oss_mixext ext;
  int dev, ix;

  dev = ext.dev = kcontrol->private_value >> 16;
  ix = ext.ctrl = kcontrol->private_value & 0xffff;;

  oss_mixer_ext (dev, OSS_DEV_MIXER, SNDCTL_MIX_EXTINFO, (caddr_t) & ext);

  switch (ext.type)
    {
    case MIXT_STEREOSLIDER:
    case MIXT_STEREOVU:
      uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
      uinfo->count = 2;
      uinfo->value.integer.min = ext.minvalue;
      uinfo->value.integer.max = ext.maxvalue;
      break;

    case MIXT_MONOSLIDER:
    case MIXT_MONOVU:
    case MIXT_SLIDER:
      uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
      uinfo->count = 1;
      uinfo->value.integer.min = ext.minvalue;
      uinfo->value.integer.max = ext.maxvalue;
      break;

    case MIXT_ONOFF:
    case MIXT_MUTE:
      uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
      uinfo->count = 1;
      uinfo->value.integer.min = 0;
      uinfo->value.integer.max = 1;
      break;

    case MIXT_ENUM:
      {
	static const char *texts[] = {
	  "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
	  "10", "11", "12", "13", "14", "15", "16", "17", "18", "19",
	  "20", "21", "22", "23", "24", "25", "26", "27", "28", "29",
	  "30", "31", "32"
	};
	oss_mixer_enuminfo enumdef;
	uinfo->value.enumerated.items = ext.maxvalue;

	if (uinfo->value.enumerated.item < 0)
	  uinfo->value.enumerated.item = 0;
	if (uinfo->value.enumerated.item >= ext.maxvalue)
	  uinfo->value.enumerated.item = ext.maxvalue - 1;
	if (uinfo->value.enumerated.item > 31)
	  uinfo->value.enumerated.item = 31;
	strcpy (uinfo->value.enumerated.name,
		texts[uinfo->value.enumerated.item]);

	enumdef.dev = ext.dev;
	enumdef.ctrl = ext.ctrl;
	if (oss_mixer_ext
	    (dev, OSS_DEV_MIXER, SNDCTL_MIX_ENUMINFO,
	     (caddr_t) & enumdef) >= 0)
	  {
	    char *text;

	    text =
	      &enumdef.strings[enumdef.
			       strindex[uinfo->value.enumerated.item]];
	    strcpy (uinfo->value.enumerated.name, text);
	  }

	uinfo->type = SNDRV_CTL_ELEM_TYPE_ENUMERATED;
	uinfo->count = 1;
	uinfo->value.enumerated.items = ext.maxvalue;
      }
      break;

    default:
      printk ("cuckoo: mixer_info(%d/%d) - unknown type %d\n", dev, ix,
	      ext.type);
      uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
      uinfo->count = 1;
      uinfo->value.integer.min = ext.minvalue;
      uinfo->value.integer.max = ext.maxvalue;
      return 0;
    }

  return 0;
}

static int
mixer_get (snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol)
{
  oss_mixext ext;
  oss_mixer_value val;
  int dev, ix, err;

  dev = ext.dev = kcontrol->private_value >> 16;
  ix = ext.ctrl = kcontrol->private_value & 0xffff;;
  if ((err =
       oss_mixer_ext (dev, OSS_DEV_MIXER, SNDCTL_MIX_EXTINFO,
		      (caddr_t) & ext)) < 0)
    return err;

  val.dev = dev;
  val.ctrl = ix;
  val.timestamp = ext.timestamp;
  if ((err =
       oss_mixer_ext (dev, OSS_DEV_MIXER, SNDCTL_MIX_READ,
		      (caddr_t) & val)) < 0)
    return err;

  switch (ext.type)
    {
    case MIXT_STEREOVU:
    case MIXT_STEREOSLIDER:
      ucontrol->value.integer.value[0] = val.value & 0xff;	// Left
      ucontrol->value.integer.value[1] = (val.value >> 8) & 0xff;	// Right
      break;

    case MIXT_MONOSLIDER:
    case MIXT_MONOVU:
    case MIXT_SLIDER:
      ucontrol->value.integer.value[0] = val.value & 0xff;
      break;

    case MIXT_ONOFF:
    case MIXT_MUTE:
      ucontrol->value.integer.value[0] = !!val.value;
      break;

    case MIXT_ENUM:
      ucontrol->value.integer.value[0] = val.value;
      break;

    default:
      printk ("cuckoo: mixer_get(%d/%d) - unknown type %d\n", dev, ix,
	      ext.type);
      ucontrol->value.integer.value[0] = val.value & 0xff;
      return 0;
    }

  return 0;
}

static int
mixer_put (snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol)
{
  oss_mixext ext;
  oss_mixer_value val;
  int dev, ix, err;

  dev = ext.dev = kcontrol->private_value >> 16;
  ix = ext.ctrl = kcontrol->private_value & 0xffff;;
  if ((err =
       oss_mixer_ext (dev, OSS_DEV_MIXER, SNDCTL_MIX_EXTINFO,
		      (caddr_t) & ext)) < 0)
    return err;

  val.dev = dev;
  val.ctrl = ix;
  val.timestamp = ext.timestamp;

  switch (ext.type)
    {
    case MIXT_STEREOSLIDER:
      val.value = ucontrol->value.integer.value[0] |	// Left
	ucontrol->value.integer.value[1] << 8;	// Right
      if ((err =
	   oss_mixer_ext (dev, OSS_DEV_MIXER, SNDCTL_MIX_WRITE,
			  (caddr_t) & val)) < 0)
	return err;
      break;

    case MIXT_MONOSLIDER:
    case MIXT_SLIDER:
      val.value = ucontrol->value.integer.value[0];
      if ((err =
	   oss_mixer_ext (dev, OSS_DEV_MIXER, SNDCTL_MIX_WRITE,
			  (caddr_t) & val)) < 0)
	return err;
      break;

    case MIXT_ONOFF:
    case MIXT_MUTE:
      val.value = !!ucontrol->value.integer.value[0];
      if ((err =
	   oss_mixer_ext (dev, OSS_DEV_MIXER, SNDCTL_MIX_WRITE,
			  (caddr_t) & val)) < 0)
	return err;
      break;

    case MIXT_ENUM:
      val.value = ucontrol->value.integer.value[0];
      if ((err =
	   oss_mixer_ext (dev, OSS_DEV_MIXER, SNDCTL_MIX_WRITE,
			  (caddr_t) & val)) < 0)
	return err;
      break;

    case MIXT_MONOVU:
    case MIXT_STEREOVU:
      return -EPERM;

    default:
      printk ("cuckoo: mixer_put(%d/%d) - unknown type %d\n", dev, ix,
	      ext.type);
      val.value = ucontrol->value.integer.value[0];
      if ((err =
	   oss_mixer_ext (dev, OSS_DEV_MIXER, SNDCTL_MIX_WRITE,
			  (caddr_t) & val)) < 0)
	return err;
    }

  return 0;
}

static void
add_control (cuckoo_t * chip, int dev, int ix, oss_mixext * ext, char *name)
{
  int i, ok, err = 0;
  snd_kcontrol_new_t my_control;

// Upshift the name if it's an single part one

  ok = 0;
  for (i = 0; i < strlen (name); i++)
    if (name[i] == '.')
      ok = 1;
  if (!ok)
    for (i = 0; i < strlen (name); i++)
      if (name[i] >= 'a' && name[i] <= 'z')
	name[i] -= 32;

// Add the control

  memset (&my_control, 0, sizeof (my_control));

  my_control.iface = SNDRV_CTL_ELEM_IFACE_MIXER;
  my_control.name = name;
  my_control.index = 0;
  my_control.access = 0;

  if (ext->flags & MIXF_READABLE)
    my_control.access |= SNDRV_CTL_ELEM_ACCESS_READ;
  if (ext->flags & MIXF_WRITEABLE)
    my_control.access |= SNDRV_CTL_ELEM_ACCESS_WRITE;
  if ((ext->flags & 0x3) == MIXF_READABLE)	/* Read only */
    my_control.access |= SNDRV_CTL_ELEM_ACCESS_VOLATILE;

  my_control.private_value = (dev << 16) | ix;
  my_control.info = get_mixer_info;
  my_control.get = mixer_get;
  my_control.put = mixer_put;

  switch (ext->type)
    {
    case MIXT_ENUM:
    case MIXT_ONOFF:
    case MIXT_MUTE:
    case MIXT_STEREOSLIDER:
    case MIXT_SLIDER:
    case MIXT_MONOSLIDER:
    case MIXT_MONOVU:
    case MIXT_STEREOVU:
      if ((err =
	   snd_ctl_add (chip->card, snd_ctl_new1 (&my_control, chip))) < 0)
	{
	  printk ("cuckoo: snd_ctl_add(%s) failed, err=%d\n", ext->extname,
		  err);
	  return;
	}
      break;
    }
}

int
install_mixer_instances (cuckoo_t * chip, int cardno)
{
  int dev;
  mixer_operations_t **cuckoo_mixer_devs = mixer_devs_p;

  for (dev = 0; dev < num_mixers; dev++)
    if (cuckoo_mixer_devs[dev]->card_number == cardno)
      {
	int nrext, i, sz;

	touch_mixer (dev);

	nrext = dev;
	oss_mixer_ext (dev, OSS_DEV_MIXER, SNDCTL_MIX_NREXT,
		       (ioctl_arg) & nrext);

	if (nrext == 0)
	  continue;

	sz = nrext * (sizeof (char *) + 32);	// 32 characters / name (average)

	for (i = 0; i < nrext; i++)
	  {
	    oss_mixext ext;
	    int parent = 0;
	    oss_mixext_root *root = NULL;

	    ext.dev = dev;
	    ext.ctrl = i;
	    oss_mixer_ext (dev, OSS_DEV_MIXER, SNDCTL_MIX_EXTINFO,
			   (caddr_t) & ext);

	    switch (ext.type)
	      {
	      case MIXT_DEVROOT:
		root = (oss_mixext_root *) & ext.data;
		break;

	      case MIXT_GROUP:
		parent = ext.parent;
		break;

	      case MIXT_MARKER:
		break;

	      default:
		add_control (chip, dev, i, &ext, ext.extname);
		break;
	      }			// Switch

	  }			// i


      }				// dev

  return 0;
}

EXPORT_SYMBOL (install_mixer_instances);
