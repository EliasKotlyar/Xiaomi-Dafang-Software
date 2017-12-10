/*
 * Purpose: OSS mixer core.
 * Copyright (C) 4Front Technologies, 2002-2004. All rights reserved.
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
#include <stdarg.h>
oss_mutex_t oss_timing_mutex;

char *oss_license_string = OSS_LICENSE;

#ifdef DO_TIMINGS
static int timing_is_active = 0; /* 1=The readtimings utility has been active */
#endif

void *mixer_devs_p = NULL;
/*
 * Mixer device list
 */
mixer_operations_t **mixer_devs = NULL;
int num_mixers = 0;
int mixer_port_number = 0;
int current_mixer_card = -1;

/*
 * mix_cvt is used for scaling mixer levels by various drivers.
 */
char mix_cvt[101] = {
  0, 0, 3, 7, 10, 13, 16, 19, 21, 23, 26, 28, 30, 32, 34, 35, 37, 39, 40, 42,
  43, 45, 46, 47, 49, 50, 51, 52, 53, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
  65,
  65, 66, 67, 68, 69, 70, 70, 71, 72, 73, 73, 74, 75, 75, 76, 77, 77, 78, 79,
  79,
  80, 81, 81, 82, 82, 83, 84, 84, 85, 85, 86, 86, 87, 87, 88, 88, 89, 89, 90,
  90,
  91, 91, 92, 92, 93, 93, 94, 94, 95, 95, 96, 96, 96, 97, 97, 98, 98, 98, 99,
  99,
  100
};

static int
get_mixer_info (int dev, ioctl_arg arg)
{
  mixer_info *info = (mixer_info *) arg;

  memset(info, 0, sizeof(*info));

  if (dev < 0 || dev >= num_mixers)
    return OSS_ENXIO;

  strcpy (info->id, mixer_devs[dev]->id);
  strncpy (info->name, mixer_devs[dev]->name, 32);
  info->name[31] = '\0';
  info->modify_counter = mixer_devs[dev]->modify_counter;

  return 0;
}

/*
 * Legacy mixer handling
 */

int
oss_legacy_mixer_ioctl (int mixdev, int audiodev, unsigned int cmd,
			ioctl_arg arg)
{
  int ret;

#ifdef DO_TIMINGS
    oss_timing_printf ("oss_legacy_mixer_ioctl(%d/%d, %08x) entered", mixdev,
	     audiodev, cmd);
#endif

  if (!(cmd & SIOC_OUT) && !(cmd & SIOC_IN))
    return OSS_EINVAL;

  if (arg == 0)
    return OSS_EINVAL;

  if (((cmd >> 8) & 0xff) != 'M')	/* Not a mixer ioctl */
    return OSS_EINVAL;

  if (mixdev < 0 || mixdev >= num_mixers)
    {
      cmn_err (CE_WARN, "Bad mixer device %d\n", mixdev);
      return OSS_EIO;
    }

  if (cmd == SOUND_MIXER_INFO)
    return get_mixer_info (mixdev, arg);

  if (!mixer_devs[mixdev]->enabled || mixer_devs[mixdev]->unloaded)
    return OSS_ENXIO;

  if (mixer_devs[mixdev]->d->ioctl == NULL)
    return OSS_EINVAL;

  if (IOC_IS_OUTPUT (cmd))
    mixer_devs[mixdev]->modify_counter++;

  ret = mixer_devs[mixdev]->d->ioctl (mixdev, audiodev, cmd, arg);

  return ret;
}

/*
 * Handling of initial mixer volumes
 */

static int muted_mixer_levels[32] = {0};

static int default_mixer_levels[32] = {
  0x3232,			/* Master Volume */
  0x3232,			/* Bass */
  0x3232,			/* Treble */
  0x4b4b,			/* FM */
  0x3232,			/* PCM */
  0x1515,			/* PC Speaker */
  0x2020,			/* Ext Line */
  0x2020,			/* Mic */
  0x4b4b,			/* CD */
  0x0000,			/* Recording monitor */
  0x4b4b,			/* Second PCM */
  0x4b4b,			/* Recording level */
  0x4b4b,			/* Input gain */
  0x4b4b,			/* Output gain */
  0x2020,			/* Line1 */
  0x2020,			/* Line2 */
  0x1515			/* Line3 (usually line in) */
};

/*
 * Table for configurable mixer volume handling
 */
static mixer_vol_table mixer_vols[MAX_MIXER_DEV];
static int num_mixer_volumes = 0;

int *
load_mixer_volumes (char *name, int *levels, int present)
{
  int i, n;
  extern int mixer_muted;

  if (mixer_muted)		/* Config setting from osscore.conf */
     return muted_mixer_levels;

  if (levels == NULL)
    levels = default_mixer_levels;

  for (i = 0; i < num_mixer_volumes; i++)
    if (strcmp (name, mixer_vols[i].name) == 0)
      {
	if (present)
	  mixer_vols[i].num = i;
	return mixer_vols[i].levels;
      }

  if (num_mixer_volumes >= MAX_MIXER_DEV)
    {
      cmn_err (CE_WARN, "Too many mixers (%s/%d/%d)\n",
	       name, num_mixer_volumes, MAX_MIXER_DEV);
      return levels;
    }

  n = num_mixer_volumes++;

  strcpy (mixer_vols[n].name, name);

  if (present)
    mixer_vols[n].num = n;
  else
    mixer_vols[n].num = -1;

  for (i = 0; i < 32; i++)
    mixer_vols[n].levels[i] = levels[i];
  return mixer_vols[n].levels;
}

/*
 * Mixer "extension" handling
 */

static int
oss_mixer_ext_info (oss_mixext * ent)
{

  int dev, ctrl;
  int extnr;

  if (ent == NULL)
    return OSS_EFAULT;

  if (ent->dev < 0 || ent->dev >= num_mixers)
    return OSS_ENXIO;

  dev = ent->dev;
  if (!mixer_devs[dev]->enabled || mixer_devs[dev]->unloaded)
    return OSS_ENXIO;
  touch_mixer (dev);

  ctrl = ent->ctrl;
  if (ent->ctrl < 0 || ent->ctrl >= mixer_devs[dev]->nr_ext)
    return OSS_EIDRM;
  extnr = ent->ctrl;

  memcpy ((char *) ent, (char *) &mixer_devs[dev]->extensions[extnr].ext,
	  sizeof (*ent));

  switch (ent->type)
    {
    case MIXT_MONOPEAK:
    case MIXT_STEREOPEAK:
    case MIXT_MONOVU:
    case MIXT_STEREOVU:
      /* Peak meters will need to be polled */
      ent->flags |= MIXF_POLL;
      break;
    }

/*
 * Read-only controls are likely to change their value spontaneously so
 * they should be polled.
 */
  if (!(ent->flags & MIXF_WRITEABLE))
     ent->flags |= MIXF_POLL;

  ent->ctrl = ctrl;
  return 0;
}

static int
mixer_ext_get_enuminfo (oss_mixer_enuminfo * ent)
{

  int dev, ctrl;

  if (ent == NULL)
    return OSS_EFAULT;

  dev = ent->dev;
  ctrl = ent->ctrl;
  memset (ent, 0, sizeof (*ent));

  if (dev < 0 || dev >= num_mixers)
    {
      return OSS_ENXIO;
    }

  touch_mixer (dev);

  if (ctrl < 0 || ctrl >= mixer_devs[dev]->nr_ext)
    {
      return OSS_EIDRM;
    }

  if (mixer_devs[dev]->extensions[ctrl].enum_info == NULL)
    {
      return OSS_EIO;
    }

  memcpy ((char *) ent,
	  (char *) mixer_devs[dev]->extensions[ctrl].enum_info,
	  sizeof (*ent));
  ent->ctrl = ctrl;
  return 0;
}

static int
mixer_ext_get_description (oss_mixer_enuminfo * ent)
{

  int dev, ctrl;
  char *s;

  if (ent == NULL)
    return OSS_EFAULT;

  dev = ent->dev;
  ctrl = ent->ctrl;
  memset (ent, 0, sizeof (*ent));

  if (dev < 0 || dev >= num_mixers)
    {
      return OSS_ENXIO;
    }

  touch_mixer (dev);

  if (ctrl < 0 || ctrl >= mixer_devs[dev]->nr_ext)
    {
      return OSS_EIDRM;
    }

  if (mixer_devs[dev]->extensions[ctrl].description == NULL)
    {
      return OSS_EIO;
    }

  s = mixer_devs[dev]->extensions[ctrl].description;

  strncpy (ent->strings, s, sizeof (ent->strings));
  ent->strings [sizeof (ent->strings) - 1] = '\0';
  ent->ctrl = ctrl;
  return 0;
}

int
mixer_ext_set_strings (int dev, int ctl, const char *s, int version)
{
/*
 * Note! The version parameter should usually be set to 0. However if the
 * 	 value set can change dynamically then it must be initially set to 1
 * 	 and later incremented every time the value set changes.
 * 	 This tells the applications to poll for updated values.
 */
  static oss_mixer_enuminfo ent;

  memset (&ent, 0, sizeof (ent));
  ent.dev = dev;
  ent.ctrl = ctl;
  ent.version = version;
  ent.nvalues = 0;
  strcpy (ent.strings, s);
  return mixer_ext_set_enum (&ent);
}

static int
rebuild_list (oss_mixer_enuminfo * ent)
{
  int i, n, l;

  n = 1;
  ent->nvalues = 0;
  ent->strindex[0] = 0;
  l = strlen (ent->strings);

  for (i = 0; i < l; i++)
    if (n < OSS_ENUM_MAXVALUE)
      {
	if (ent->strings[i] == ' ')
	  {
	    ent->strindex[n++] = i + 1;
	    ent->strings[i] = 0;
	  }
      }

  ent->nvalues = n;
  return 0;
}

int
mixer_ext_set_enum (oss_mixer_enuminfo * ent)
{
  int dev;
  int extnr;

  if (ent == NULL)
    return OSS_EFAULT;

  if (ent->dev < 0 || ent->dev >= num_mixers)
    return OSS_ENXIO;

  dev = ent->dev;
  touch_mixer (dev);

  if (ent->ctrl < 0 || ent->ctrl >= mixer_devs[dev]->nr_ext)
    return OSS_EIDRM;
  extnr = ent->ctrl;

  if (mixer_devs[dev]->extensions[extnr].enum_info == NULL)
    mixer_devs[dev]->extensions[extnr].enum_info =
      PMALLOC (mixer_devs[dev]->osdev, sizeof (*ent));

  if (mixer_devs[dev]->extensions[extnr].enum_info == NULL)
    return OSS_EIO;

  memcpy ((char *) mixer_devs[dev]->extensions[extnr].enum_info,
	  (char *) ent, sizeof (*ent));

  ent = mixer_devs[dev]->extensions[extnr].enum_info;

  if (ent->nvalues <= 0)
    return rebuild_list (ent);

  if (ent->nvalues >= OSS_ENUM_MAXVALUE)
    {
      mixer_devs[dev]->extensions[extnr].enum_info = NULL;
      return OSS_EIO;
    }

  return 0;
}

int
mixer_ext_set_description (int dev, int ctrl, const char *desc)
{
  int l = strlen(desc);

  if (dev < 0 || dev >= num_mixers)
    return OSS_ENXIO;

  touch_mixer (dev);

  if (ctrl < 0 || ctrl >= mixer_devs[dev]->nr_ext)
    return OSS_EIDRM;

  if (l > OSS_ENUM_STRINGSIZE) l = OSS_ENUM_STRINGSIZE;

    mixer_devs[dev]->extensions[ctrl].description =
      PMALLOC (mixer_devs[dev]->osdev, l);

  if (mixer_devs[dev]->extensions[ctrl].description == NULL)
    return OSS_EIO;

  strncpy (mixer_devs[dev]->extensions[ctrl].description, desc, l);
  mixer_devs[dev]->extensions[ctrl].description[l-1] = '\0';

  mixer_devs[dev]->extensions[ctrl].ext.flags |= MIXF_DESCR;

  return 0;
}

static int
mixer_ext_read (oss_mixer_value * val)
{

  int dev;
  int extnr;
  oss_mixext *ext;
  oss_mixext_desc *ext_desc;
  mixer_ext_fn func;

  if (val == NULL)
    return OSS_EFAULT;

  if (val->dev < 0 || val->dev >= num_mixers)
    return OSS_ENXIO;

  dev = val->dev;

  if (!mixer_devs[dev]->enabled || mixer_devs[dev]->unloaded)
    return OSS_ENXIO;
  touch_mixer (dev);

  if (val->ctrl < 0 || val->ctrl >= mixer_devs[dev]->nr_ext)
    return OSS_EIDRM;
  extnr = val->ctrl;

  ext_desc = &mixer_devs[dev]->extensions[extnr];
  ext = &ext_desc->ext;

  if (val->timestamp != ext->timestamp)
    {
      return OSS_EIDRM;
    }

  if (ext_desc->handler == NULL || !(ext->flags & MIXF_READABLE))
    return OSS_EFAULT;

  func = (mixer_ext_fn) ext_desc->handler;
  return (val->value = func (dev, ext->ctrl, SNDCTL_MIX_READ, val->value));
}

static int
mixer_ext_write (oss_mixer_value * val)
{

  int dev;
  int extnr;
  int err;
  oss_mixext *ext;
  oss_mixext_desc *ext_desc;
  mixer_ext_fn func;

  if (val == NULL)
    {
      cmn_err (CE_WARN, "NULL argument in mixer call\n");
      return OSS_EFAULT;
    }

  if (val->dev < 0 || val->dev >= num_mixers)
    return OSS_ENXIO;

  dev = val->dev;
  if (!mixer_devs[dev]->enabled || mixer_devs[dev]->unloaded)
    return OSS_ENXIO;
  touch_mixer (dev);

  if (val->ctrl < 0 || val->ctrl >= mixer_devs[dev]->nr_ext)
    return OSS_EIDRM;
  extnr = val->ctrl;

  ext_desc = &mixer_devs[dev]->extensions[extnr];
  ext = &ext_desc->ext;

  if (ext_desc->handler == NULL || !(ext->flags & MIXF_WRITEABLE))
    {
      cmn_err (CE_WARN, "NULL handler or control not writeable\n");
      return OSS_EFAULT;
    }

  if (val->timestamp != ext->timestamp)
    {
      return OSS_EIDRM;
    }

  func = (mixer_ext_fn) ext_desc->handler;
  mixer_devs[dev]->modify_counter++;
  err = val->value = func (dev, ext->ctrl, SNDCTL_MIX_WRITE, val->value);

  if (err >= 0)
    ext->update_counter++;
  return err;
}

int
mixer_ext_create_device (int dev, int maxentries)
{
  oss_mixext_root *mixroot;
  oss_mixext *mixext;
  oss_mixext_desc *mixext_desc;
  char *name = mixer_devs[dev]->name;
  char *id = mixer_devs[dev]->id;
  int i;

  maxentries++;			/* Needs space for the device root node */

  if (mixer_devs[dev]->max_ext == 0)
     {
        mixext_desc =
          PMALLOC (mixer_devs[dev]->osdev, sizeof (*mixext_desc) * maxentries);
        mixer_devs[dev]->max_ext = maxentries;
  	mixer_devs[dev]->extensions = mixext_desc;
     }
  else
     {
  	mixext_desc = mixer_devs[dev]->extensions;
     }

  if (mixext_desc == NULL)
    {
      cmn_err (CE_CONT, "Not enough memory for mixer%d (ext)\n", dev);
      return OSS_EIO;
    }

  mixer_devs[dev]->nr_ext = 1;
  mixer_devs[dev]->timestamp = GET_JIFFIES ();

  mixext = &mixext_desc->ext;
  mixext->dev = dev;
  mixext->ctrl = -1;		/* Undefined */
  mixext->type = MIXT_DEVROOT;
  mixext->maxvalue = 0;
  mixext->minvalue = 0;
  mixext->flags = 0;
  strcpy (mixext->id, "DEVROOT");
  mixext->parent = 0;		/* Link to itself */
  mixext->timestamp = mixer_devs[dev]->timestamp;
  mixext_desc->handler = NULL;
  memset (mixext->data, 0, sizeof (mixext->data));

  mixroot = (oss_mixext_root *) & mixext->data;

  for (i = 0; i < 15 && id[i]; i++)
    mixroot->id[i] = id[i];
  mixroot->id[15] = 0;

  for (i = 0; i < 47 && name[i]; i++)
    mixroot->name[i] = name[i];
  mixroot->name[47] = 0;

  return 0;
}

int
mixer_ext_create_group_flags (int dev, int parent, const char *id,
			      unsigned int flags)
{
  oss_mixext *mixext;
  oss_mixext_desc *mixext_desc, *parent_desc;
  int enumber;

  flags &= ~MIXF_DESCR;

  if (mixer_devs[dev]->extensions == NULL)
    {
      cmn_err (CE_WARN, "Mixer extensions not initialized for device %d\n",
	       dev);
      return OSS_EFAULT;
    }

  /*
   * Ensure that the parent node number is valid.
   */
  if (parent < 0 || parent >= mixer_devs[dev]->nr_ext)
     parent = 0;

  parent_desc =
    &mixer_devs[dev]->extensions[parent];
  mixext = &parent_desc->ext;

  if (mixext->type != MIXT_DEVROOT && mixext->type != MIXT_GROUP)
     parent = 0; /* Point to the root group */

  if (mixer_devs[dev]->nr_ext >= mixer_devs[dev]->max_ext)
    {
      cmn_err (CE_WARN, "Out of mixer controls for device %d/%s (%d)\n", dev,
	       mixer_devs[dev]->name, mixer_devs[dev]->max_ext);
      return OSS_ENOSPC;
    }

  mixext_desc =
    &mixer_devs[dev]->extensions[(enumber = mixer_devs[dev]->nr_ext++)];
  mixext = &mixext_desc->ext;
  mixext->dev = dev;
  mixext->ctrl = -1;		/* Undefined */
  mixext->type = MIXT_GROUP;
  mixext->maxvalue = 0;
  mixext->minvalue = 0;
  mixext->flags = flags | MIXF_FLAT;	/* Will be unflattened later if required */
  strcpy (mixext->id, id);
  mixext->parent = parent;
  mixext_desc->handler = NULL;
  mixext_desc->enum_info = NULL;
  memset (mixext->enum_present, 0xff, sizeof (mixext->enum_present));
  mixext->timestamp = mixer_devs[dev]->timestamp;
  mixext->control_no = -1;
  mixext->desc = 0;
  memset (mixext->data, 0, sizeof (mixext->data));

  return enumber;
}

int
mixer_ext_create_group (int dev, int parent, const char *id)
{
  return mixer_ext_create_group_flags (dev, parent, id, 0);
}

int
mixer_ext_truncate (int dev, int index)
{
  if (index < mixer_devs[dev]->nr_ext)
    {
      mixer_devs[dev]->nr_ext = index;
      mixer_devs[dev]->modify_counter++;
      mixer_devs[dev]->timestamp++;
    }
  return 0;
}

static void expand_names (int dev);
static void unflatten_group (int dev, int group);
static void touch_parents (int dev, int group);

int
mixer_ext_create_control (int dev, int parent, int ctrl, mixer_ext_fn func,
			  int type, const char *id, int maxvalue, int flags)
{
  oss_mixext *mixext;
  oss_mixext_desc *mixext_desc, *parent_desc;
  int enumber;

  flags &= ~MIXF_DESCR;

  if (mixer_devs[dev]->extensions == NULL)
    {
      cmn_err (CE_WARN, "Mixer extensions not initialized for device %d\n",
	       dev);
      return OSS_EFAULT;
    }

  if (mixer_devs[dev]->nr_ext >= mixer_devs[dev]->max_ext)
    {
      cmn_err (CE_WARN, "Out of mixer controls for device %d/%s (%d)\n", dev,
	       mixer_devs[dev]->name, mixer_devs[dev]->max_ext);
      return OSS_ENOSPC;
    }

  if (func == NULL)		/* No access function */
    flags &= ~(MIXF_READABLE | MIXF_WRITEABLE);

  /*
   * Ensure that the parent node number is valid.
   */
  if (parent < 0 || parent >= mixer_devs[dev]->nr_ext)
     parent = 0;

  parent_desc =
    &mixer_devs[dev]->extensions[parent];
  mixext = &parent_desc->ext;

  if (mixext->type != MIXT_DEVROOT && mixext->type != MIXT_GROUP)
     parent = 0; /* Point to the root group */


  mixext_desc =
    &mixer_devs[dev]->extensions[(enumber = mixer_devs[dev]->nr_ext++)];
  mixext = &mixext_desc->ext;
  mixext->dev = dev;
  mixext->ctrl = ctrl;
  mixext->type = type;
  mixext->maxvalue = maxvalue;
  mixext->minvalue = 0;
  mixext->flags = flags;
  strncpy (mixext->id, id, sizeof (mixext->id));
  mixext->id[sizeof (mixext->id) - 1] = 0;
  mixext->parent = parent;
  mixext->timestamp = mixer_devs[dev]->timestamp;
  mixext_desc->handler = (mixer_ext_fn) func;
  mixext_desc->enum_info = NULL;
  memset (mixext->data, 0, sizeof (mixext->data));
  memset (mixext->enum_present, 0xff, sizeof (mixext->enum_present));
  mixext->control_no = -1;
  mixext->desc = 0;

/*
 * Perform name expansion too if it has already been done for the
 * earlier controls. Note that this only gets done with rare devices
 * that add/remove controls on fly.
 *
 * TODO: Optimize this to expand only the current control. Scanning through
 * all the controls may be bit time consuming with future devices having 100s 
 * of controls.
 */
  if (mixer_devs[dev]->names_checked)
    expand_names (dev);
  else
    strcpy(mixext->extname, mixext->id);

/*
 * Mark groups with tall controls such as peak meters and sliders as
 * non-flat
 */

  switch (type)
    {
    case MIXT_SLIDER:
    case MIXT_MONOSLIDER:
    case MIXT_STEREOSLIDER:
    case MIXT_MONOSLIDER16:
    case MIXT_STEREOSLIDER16:
    case MIXT_MONOVU:
    case MIXT_STEREOVU:
    case MIXT_MONOPEAK:
    case MIXT_STEREOPEAK:
    case MIXT_MONODB:
    case MIXT_STEREODB:
    case MIXT_3D:
      unflatten_group (dev, parent);
      break;
    }

  touch_parents(dev, parent);

  return enumber;
}


oss_mixext *
mixer_find_ext (int dev, int enumber)
{
  oss_mixext_desc *mixext;

  if (dev < 0 || dev >= num_mixers)
    {
      return NULL;
    }
  touch_mixer (dev);

  if (enumber < 0 || enumber >= mixer_devs[dev]->nr_ext)
    {
      return NULL;
    }

  mixext = &mixer_devs[dev]->extensions[enumber];

  return &mixext->ext;
}

/*
 * Default read/write access functions
 */
int
mixer_ext_rw (int dev, int ctrl, unsigned int cmd, int value)
{
  int err;

  if (cmd == SNDCTL_MIX_READ)
    {
      if ((err =
	   oss_legacy_mixer_ioctl (dev, -1, MIXER_READ (ctrl),
				   (ioctl_arg) & value)) < 0)
	return err;
      return value;
    }

  if (cmd == SNDCTL_MIX_WRITE)
    {
      if ((err =
	   oss_legacy_mixer_ioctl (dev, -1, MIXER_WRITE (ctrl),
				   (ioctl_arg) & value)) < 0)
	return err;
      return value;
    }

  return OSS_EINVAL;
}

int
mixer_ext_recrw (int dev, int ctrl, unsigned int cmd, int value)
{
  int caps, recmask, err;

  if ((err =
       oss_legacy_mixer_ioctl (dev, -1, SOUND_MIXER_READ_CAPS,
			       (ioctl_arg) & caps)) < 0)
    caps = SOUND_CAP_EXCL_INPUT;	/* Default */

  if ((err =
       oss_legacy_mixer_ioctl (dev, -1, SOUND_MIXER_READ_RECSRC,
			       (ioctl_arg) & recmask)) < 0)
    return err;

  if (cmd == SNDCTL_MIX_READ)
    return (recmask & (1 << ctrl)) ? 1 : 0;

  if (caps & SOUND_CAP_EXCL_INPUT)	/* Single recording source */
    recmask = 0;

  if (value)
    recmask |= (1 << ctrl);
  else
    recmask &= ~(1 << ctrl);

  if (recmask == 0)
    return 1;			/* Can't remove the only recording source */

  if ((err =
       oss_legacy_mixer_ioctl (dev, -1, SOUND_MIXER_WRITE_RECSRC,
			       (ioctl_arg) & recmask)) < 0)
    return err;


  return (recmask & (1 << ctrl)) ? 1 : 0;
}

/*
 * Mixer extension initialization
 */

static void
store_name (oss_mixext * thisrec, char *name)
{
  int i;

  while (*name == '.')
    name++;
  strncpy (thisrec->extname, name, 32);
  thisrec->extname[31] = '\0';

  name = thisrec->extname;
  for (i = 0; i < strlen (name); i++)
    if (name[i] >= 'A' && name[i] <= 'Z')
      name[i] += 32;
}

static char *
cut_name (char *name)
{
  char *s = name;
  while (*s)
    if (*s++ == '_')
      return s;

  if (name[0] == '@')
    return &name[1];

  return name;
}

#include "mixerdefs.h"

static void
find_enum_defs (oss_mixext * thisrec, int dev, int ctl)
{
  int i;

  for (i = 0; mixer_defs[i].name != NULL; i++)
    if (strcmp (thisrec->extname, mixer_defs[i].name) == 0)
      {
	mixer_ext_set_strings (dev, ctl, mixer_defs[i].strings, 0);
	return;
      }
}

static void
expand_names (int dev)
{
  int i, n;
  oss_mixext_desc *mixext_desc;

  n = mixer_devs[dev]->nr_ext;
  mixer_devs[dev]->names_checked = 1;

  if (n < 1)
    return;

  for (i = 0; i < n; i++)
    {
      char tmp[100], *name;
      int parent = 0;
      oss_mixext *thisrec = NULL, *parentrec = NULL;

      mixext_desc = &mixer_devs[dev]->extensions[i];
      thisrec = &mixext_desc->ext;

      switch (thisrec->type)
	{
	case MIXT_DEVROOT:
	  thisrec->extname[0] = 0;
	  break;

	case MIXT_GROUP:
	  parent = thisrec->parent;
	  mixext_desc = &mixer_devs[dev]->extensions[parent];
	  parentrec = &mixext_desc->ext;
	  name = cut_name (thisrec->id);
	  if (parentrec->extname[0] == 0)
	    strcpy (tmp, name);
	  else
	    sprintf (tmp, "%s.%s", parentrec->extname, name);
	  store_name (thisrec, tmp);
	  break;

	case MIXT_STEREOSLIDER:
	case MIXT_STEREOSLIDER16:
	case MIXT_STEREODB:
	case MIXT_STEREOVU:
	case MIXT_MONODB:
	case MIXT_MONOSLIDER:
	case MIXT_MONOSLIDER16:
	case MIXT_SLIDER:
	case MIXT_MONOVU:
	case MIXT_MONOPEAK:
	case MIXT_STEREOPEAK:
	case MIXT_ONOFF:
	case MIXT_MUTE:
	case MIXT_ENUM:
	case MIXT_VALUE:
	case MIXT_HEXVALUE:
	case MIXT_3D:
	  parent = thisrec->parent;
	  mixext_desc = &mixer_devs[dev]->extensions[parent];
	  parentrec = &mixext_desc->ext;
	  name = cut_name (thisrec->id);
	  if (*thisrec->id == 0 || *thisrec->id == '-')	/* Special (hidden) names */
	    strcpy (thisrec->extname, parentrec->extname);
	  else
	    {
	      sprintf (tmp, "%s.%s", parentrec->extname, name);
	      store_name (thisrec, tmp);

	      if (thisrec->type == MIXT_ENUM)
		find_enum_defs (thisrec, dev, i);
	    }
	  break;

	case MIXT_MARKER:
	  break;

	default:;
	}
    }
/*
 * Fix duplicate names.
 */

  for (i = 0; i < n; i++)
    {
      char tmp[100];
      int j, dupes = 0;
      oss_mixext *thisrec = NULL;

      mixext_desc = &mixer_devs[dev]->extensions[i];
      thisrec = &mixext_desc->ext;

      if (thisrec->type == MIXT_GROUP)
	continue;

      strcpy (tmp, thisrec->extname);

      for (j = i + 1; j < n; j++)
	{
	  oss_mixext_desc *mixext_desc2;
	  oss_mixext *thisrec2 = NULL;
	  mixext_desc2 = &mixer_devs[dev]->extensions[j];
	  thisrec2 = &mixext_desc2->ext;

	  if (thisrec2->type == MIXT_GROUP)
	    continue;

	  if (strcmp (thisrec2->extname, tmp) == 0)
	    dupes++;
	}

      if (dupes > 0)		/* Need to fix duplicates */
	{
	  int count = 1, len;
	  char tmp2[32];

	  for (j = i; j < n; j++)
	    {
	      oss_mixext_desc *mixext_desc2;
	      oss_mixext *thisrec2 = NULL;
	      mixext_desc2 = &mixer_devs[dev]->extensions[j];
	      thisrec2 = &mixext_desc2->ext;

	      if (thisrec2->type != MIXT_GROUP)
		if (strcmp (thisrec2->extname, tmp) == 0)
		  {
		    sprintf (tmp2, "%d", count++);
		    tmp2[31] = '\0';
		    len = strlen (thisrec2->extname);
		    if (len >= sizeof (thisrec2->extname) - strlen (tmp2))
		      len = sizeof (thisrec2->extname) - strlen (tmp2) - 1;
		    strcpy (thisrec2->extname + len, tmp2);
		  }
	    }
	}
    }
}

static void
unflatten_group (int dev, int group)
{
/*
 * Clear the MIXF_FLAT flags from all parent groups (recursively):
 */
  int n;
  oss_mixext_desc *mixext_desc;
  oss_mixext *thisrec = NULL;

  n = mixer_devs[dev]->nr_ext;

  if (n < 1)
    return;

  if (group <= 0 || group >= n)
    return;

  mixext_desc = &mixer_devs[dev]->extensions[group];
  thisrec = &mixext_desc->ext;

  if (thisrec->type != MIXT_GROUP)	/* Not a group */
    return;

  if (!(thisrec->flags & MIXF_FLAT))	/* Already unflattened */
    return;

  thisrec->flags &= ~MIXF_FLAT;

  if (thisrec->parent >= group)	/* Broken link */
    return;

  unflatten_group (dev, thisrec->parent);	/* Unflatten the parent */
}

static void
touch_parents (int dev, int group)
{
  int n;
  oss_mixext_desc *mixext_desc;
  oss_mixext *thisrec = NULL;

  n = mixer_devs[dev]->nr_ext;

  if (n < 1)
    return;

  if (group <= 0 || group >= n)
    return;

  mixext_desc = &mixer_devs[dev]->extensions[group];
  thisrec = &mixext_desc->ext;

  while (thisrec->type != MIXT_DEVROOT)
    {
      if (thisrec->type != MIXT_GROUP)	/* Not a group */
        return;

      thisrec->update_counter++;

      if (thisrec->parent >= group)	/* Broken link */
        return;

      unflatten_group (dev, thisrec->parent);	/* Unflatten the parent */

      mixext_desc = &mixer_devs[dev]->extensions[thisrec->parent];
      thisrec = &mixext_desc->ext;
    }

  thisrec->update_counter++;
}

#define INPUT_MASK (SOUND_MASK_RECLEV|SOUND_MASK_IGAIN)
#define OUTPUT_MASK (SOUND_MASK_PCM|SOUND_MASK_ALTPCM|SOUND_MASK_VOLUME| \
		SOUND_MASK_BASS|SOUND_MASK_TREBLE|SOUND_MASK_SYNTH| \
		SOUND_MASK_IMIX|SOUND_MASK_REARVOL|SOUND_MASK_CENTERVOL| \
		SOUND_MASK_SIDEVOL)
#define MONITOR_MASK (SOUND_MASK_SPEAKER|SOUND_MASK_LINE|SOUND_MASK_LINE| \
		SOUND_MASK_MIC|SOUND_MASK_CD|SOUND_MASK_LINE1|SOUND_MASK_LINE2| \
		SOUND_MASK_LINE3|SOUND_MASK_DIGITAL1|SOUND_MASK_DIGITAL2| \
		SOUND_MASK_DIGITAL3|SOUND_MASK_MONO|SOUND_MASK_PHONE| \
		SOUND_MASK_RADIO|SOUND_MASK_VIDEO)

void
touch_mixer (int dev)
{
  int i, n, devmask, recmask, stereomask, grp, root = 0, caps = 0;
  static char *id[] = SOUND_DEVICE_NAMES;
  int created = 0;

  if (mixer_devs[dev] == NULL || mixer_devs[dev]->unloaded
      || !mixer_devs[dev]->enabled)
    {
      return;
    }

/*
 * Create default mixer extension records if required.
 */
  if (mixer_devs[dev]->nr_ext > 0)	/* Already initialized */
    return;

/*
 * Compute number of required mixer extension entries
 */

  n = mixer_devs[dev]->nr_extra_ext;	/* Reserve space for the actual driver */
  if (n < 20)
    n = 20;

  if (oss_legacy_mixer_ioctl
      (dev, -1, SOUND_MIXER_READ_CAPS, (ioctl_arg) & caps) < 0)
    caps = 0;			/* Error */

  if (oss_legacy_mixer_ioctl
      (dev, -1, SOUND_MIXER_READ_DEVMASK, (ioctl_arg) & devmask) < 0)
    goto skip;			/* Error */

  /* Remove devices that are handled otherwise */
  devmask &= ~mixer_devs[dev]->ignore_mask;

  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
    if (devmask & (1 << i))	/* This control is supported */
      n++;

  n = n * 2;

  if (oss_legacy_mixer_ioctl
      (dev, -1, SOUND_MIXER_READ_RECMASK, (ioctl_arg) & recmask) < 0)
    goto skip;			/* Error */

  for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
    if (recmask & (1 << i))	/* This control is also recording device */
      n++;

  root = 0;

#ifdef CONFIG_OSSD
  n += 20;			/* Space for OSSD use */
#endif

  n = n + 5;			/* The marker entry and some spare space */
  if ((root = mixer_ext_create_device (dev, n)) < 0)
    return;			/* Error */
  created = 1;

  if (oss_legacy_mixer_ioctl (dev, -1, SOUND_MIXER_READ_STEREODEVS,
			      (ioctl_arg) & stereomask) < 0)
    stereomask = -1;		/* Assume all stereo */

  if (!(caps & SOUND_CAP_NOLEGACY))	/* Don't (re)export the legacy mixer */
    for (i = 0; i < SOUND_MIXER_NRDEVICES; i++)
      if (devmask & (1 << i))
	{
	  if ((grp =
	       mixer_ext_create_group_flags (dev, root, id[i],
					     MIXF_LEGACY)) > 0)
	    {
	      int cnum;
	      oss_mixext *ent;
	      int flags = 0;

	      /*
	       * Set the type hints for main and PCM volume controls
	       */

	      switch (i)
		{
		case SOUND_MIXER_VOLUME:
		case SOUND_MIXER_MONO:
		case SOUND_MIXER_REARVOL:
		case SOUND_MIXER_CENTERVOL:
		case SOUND_MIXER_SIDEVOL:
		  flags |= MIXF_MAINVOL;
		  break;

		case SOUND_MIXER_PCM:
		case SOUND_MIXER_ALTPCM:
		  flags |= MIXF_PCMVOL;
		  break;

		case SOUND_MIXER_RECLEV:
		case SOUND_MIXER_IGAIN:
		  flags |= MIXF_RECVOL;
		  break;

		case SOUND_MIXER_SYNTH:
		case SOUND_MIXER_SPEAKER:
		case SOUND_MIXER_LINE:
		case SOUND_MIXER_LINE1:
		case SOUND_MIXER_LINE2:
		case SOUND_MIXER_LINE3:
		case SOUND_MIXER_MIC:
		case SOUND_MIXER_CD:
		case SOUND_MIXER_DIGITAL1:
		case SOUND_MIXER_DIGITAL2:
		case SOUND_MIXER_DIGITAL3:
		case SOUND_MIXER_PHONE:
		case SOUND_MIXER_VIDEO:
		case SOUND_MIXER_RADIO:
		  flags |= MIXF_MONVOL;
		  break;
		}

	      if (stereomask & (1 << i))
		cnum = mixer_ext_create_control (dev, grp, i, mixer_ext_rw,
						 MIXT_STEREOSLIDER,
						 "", 100,
						 flags | MIXF_READABLE |
						 MIXF_WRITEABLE);
	      else
		cnum = mixer_ext_create_control (dev, grp, i, mixer_ext_rw,
						 MIXT_MONOSLIDER,
						 "", 100,
						 flags | MIXF_READABLE |
						 MIXF_WRITEABLE);

	      if ((ent = mixer_find_ext (dev, cnum)) != NULL)
		{
		  ent->control_no = i;

		  if ((1 << i) & INPUT_MASK)
		    ent->desc &= MIXEXT_SCOPE_INPUT;
		  if ((1 << i) & OUTPUT_MASK)
		    ent->desc &= MIXEXT_SCOPE_OUTPUT;
		  if ((1 << i) & MONITOR_MASK)
		    ent->desc &= MIXEXT_SCOPE_MONITOR;

		      /*
		       * Set the RGB color for some of the controls
		       * to match the usual jack color.
		       */

		      switch (i)
		      {
		      case SOUND_MIXER_MIC: ent->rgbcolor=OSS_RGB_PINK; break;
		      case SOUND_MIXER_LINE: ent->rgbcolor=OSS_RGB_BLUE; break;
		      case SOUND_MIXER_VOLUME: ent->rgbcolor=OSS_RGB_GREEN; break;
		      case SOUND_MIXER_REARVOL: ent->rgbcolor=OSS_RGB_BLACK; break;
		      case SOUND_MIXER_SIDEVOL: ent->rgbcolor=OSS_RGB_GRAY; break;
		      case SOUND_MIXER_CENTERVOL: ent->rgbcolor=OSS_RGB_ORANGE; break;
		      }
		 }

	      if (recmask & (1 << i))
		{
		  cnum =
		    mixer_ext_create_control (dev, grp, i, mixer_ext_recrw,
					      MIXT_ONOFF, "REC", 1,
					      MIXF_READABLE | MIXF_WRITEABLE | MIXF_RECVOL);
		  if ((ent = mixer_find_ext (dev, cnum)) != NULL)
		    {
		      ent->desc &= MIXEXT_SCOPE_RECSWITCH;
		    }
		}
	    }
	}

skip:
  if (!created)
    if ((root = mixer_ext_create_device (dev, n)) < 0)
      return;			/* Error */
  mixer_ext_create_control (dev, root, 0, NULL, MIXT_MARKER, "", 0, 0);

  if (mixer_devs[dev]->create_controls != NULL)
    mixer_devs[dev]->create_controls (dev);
  expand_names (dev);
}

int
mixer_ext_set_init_fn (int dev, mixer_create_controls_t func, int nextra)
{
/*
 * Set device dependent mixer extension initialization function and
 * reserve some extension entries for device dependent use.
 *
 * This initialization function will be called later when/if the
 * extended mixer is actually used.
 */
  if (dev < 0 || dev >= num_mixers)
    return OSS_ENXIO;

  mixer_devs[dev]->nr_extra_ext = nextra;
  mixer_devs[dev]->create_controls = func;
  return 0;
}

int
mixer_ext_rebuild_all (int dev, mixer_create_controls_t func, int nextra)
{
/*
 * Throw away all existing mixer controls and recreate the mixer.
 */
  if (dev < 0 || dev >= num_mixers)
    return OSS_ENXIO;
  mixer_devs[dev]->nr_ext = 0;

  mixer_devs[dev]->nr_extra_ext = nextra;
  mixer_devs[dev]->create_controls = func;

  touch_mixer (dev);
  if (mixer_devs[dev]->create_vmix_controls != NULL)
     {
        mixer_devs[dev]->create_vmix_controls(dev);
     }

  return 0;
}

int
mixer_ext_set_vmix_init_fn (int dev, mixer_create_controls_t func, int nextra,
			    void *devc)
{
/*
 * Set device dependent mixer extension initialization function and
 * reserve some extension entries for device dependent use.
 *
 * This initialization function will be called later when/if the
 * extended mixer is actually used.
 */
  if (dev < 0 || dev >= num_mixers)
    return OSS_ENXIO;

  mixer_devs[dev]->nr_extra_ext += nextra;
  mixer_devs[dev]->create_vmix_controls = func;
  mixer_devs[dev]->vmix_devc = devc;
  touch_mixer (dev);
  func (dev);
  expand_names (dev);
  return 0;
}

#ifdef VDEV_SUPPORT
static void
ainfo_combine_caps (oss_audioinfo * ainfo, adev_p adev)
{
  if (!(adev->flags & ADEV_NOOUTPUT))
    ainfo->caps |= DSP_CAP_OUTPUT;
  else
    ainfo->caps &= ~DSP_CAP_OUTPUT;

  if (!(adev->flags & ADEV_NOINPUT))
    ainfo->caps |= DSP_CAP_INPUT;
  else
    ainfo->caps &= ~DSP_CAP_INPUT;

  if (adev->flags & ADEV_DUPLEX)
    ainfo->caps |= DSP_CAP_DUPLEX;
  else
    ainfo->caps &= ~DSP_CAP_DUPLEX;

#ifdef ALLOW_BUFFER_MAPPING
  if (!(adev->flags & ADEV_NOMMAP))
    ainfo->caps |= DSP_CAP_MMAP;
#endif
}
#endif

#if 0
/*
 * Device list support is currently not used
 */
static int
check_list (oss_devlist_t * oldlist, oss_devlist_t * newlist)
{
/*
 * Check that the same devices are present in both lists. Any difference
 * indicates that the device configuration has changed (invalidates the list).
 */
#if MAX_AUDIO_DEVFILES > 64
#error Too many audio devices - fix this algorithm
#endif
  unsigned long long mask1, mask2;
  int i;

  if (newlist->ndevs != oldlist->ndevs)
    return 0;

  mask1 = 0LL;
  mask2 = 0LL;

  for (i = 0; i < oldlist->ndevs; i++)
    mask1 |= 1LL << oldlist->devices[i];
  for (i = 0; i < newlist->ndevs; i++)
    mask1 |= 1LL << newlist->devices[i];

  if (mask1 != mask2)
    return 0;

  return 1;
}
#endif

static int
get_engineinfo (int dev, oss_audioinfo * info, int combine_slaves)
{
  int dev_present = 0;
  int i;
  oss_native_word flags;

  adev_p adev, next;

  flags = 0;
  memset ((char *) info, 0, sizeof (*info));

  if (dev < 0 || dev >= num_audio_engines)
    return OSS_ENXIO;

  adev = audio_engines[dev];
  if (adev == NULL)
    {
      cmn_err (CE_WARN, "Internal error - adev==NULL (%d)\n", dev);
      return OSS_ENXIO;
    }

  if (!adev->unloaded && adev->enabled)
    dev_present = 1;

  if (dev_present)
    {
      MUTEX_ENTER_IRQDISABLE (adev->mutex, flags);
    }
  info->dev = dev;
  strcpy (info->name, adev->name);
  strcpy (info->handle, adev->handle);
  info->busy = adev->open_mode;
  info->caps = adev->caps;
  if (!(adev->flags & ADEV_NOINPUT))
    info->caps |= PCM_CAP_INPUT;
  if (!(adev->flags & ADEV_NOOUTPUT))
    info->caps |= PCM_CAP_OUTPUT;
  if (adev->flags & ADEV_SPECIAL)
    info->caps |= PCM_CAP_SPECIAL;
  if (adev->flags & ADEV_VIRTUAL)
    info->caps |= PCM_CAP_VIRTUAL;

  if (adev->flags & (ADEV_HIDDEN | ADEV_SHADOW))
    {
      info->caps |= PCM_CAP_HIDDEN;
    }

  if (adev->flags & ADEV_DUPLEX)
    {
      info->caps |= PCM_CAP_DUPLEX;
    }
  if (adev->d->adrv_trigger)	/* Supports SETTRIGGER */
    info->caps |= PCM_CAP_TRIGGER;
#ifdef ALLOW_BUFFER_MAPPING
  if (!(adev->flags & ADEV_NOMMAP))
    info->caps |= PCM_CAP_MMAP;
#endif

  info->oformats = adev->oformat_mask;
  info->iformats = adev->iformat_mask;
  info->pid = adev->pid;
  info->latency = adev->latency;
  *info->cmd = 0;
  strncpy (info->cmd, adev->cmd, sizeof (info->cmd));
  info->cmd[sizeof (info->cmd) - 1] = 0;

  strcpy (info->devnode, adev->devnode);

  if (!adev->unloaded && adev->enabled)
    {
      if (audio_engines[dev]->d->adrv_ioctl (dev, SNDCTL_GETSONG,
					     (ioctl_arg) info->song_name) ==
	  OSS_EINVAL)
	strcpy (info->song_name, adev->song_name);
      if (audio_engines[dev]->d->adrv_ioctl (dev, SNDCTL_GETLABEL,
					     (ioctl_arg) info->label) ==
	  OSS_EINVAL)
	strcpy (info->label, adev->label);
    }

  if (*info->label == 0)
    {
      strncpy (info->label, info->cmd, sizeof (info->label));
      info->label[sizeof (info->label) - 1] = 0;
    }

  info->magic = adev->magic;
  info->card_number = adev->card_number;
  info->port_number = adev->port_number;
  info->mixer_dev = adev->mixer_dev;
  info->legacy_device = adev->real_dev;
  info->rate_source = adev->rate_source;
  info->enabled = (adev->enabled && !adev->unloaded);
  info->flags = adev->flags;
  info->min_rate = adev->min_rate;
  info->max_rate = adev->max_rate;
  info->min_channels = adev->min_channels;
  info->max_channels = adev->max_channels;
  info->binding = adev->binding;
  info->nrates = adev->nrates;
  for (i = 0; i < info->nrates; i++)
    info->rates[i] = adev->rates[i];

  if (adev->next_out == NULL || !dev_present)
    info->next_play_engine = 0;
  else
    {
      info->next_play_engine = adev->next_out->engine_num;
      next = adev->next_out;

#ifdef VDEV_SUPPORT
      i = 0;
      while (combine_slaves && next != NULL && i++ < num_audio_engines)
	{
	  ainfo_combine_caps (info, next);
	  next = next->next_out;
	}
#endif
    }

  if (adev->next_in == NULL || !dev_present)
    info->next_rec_engine = 0;
  else
    {
      info->next_rec_engine = adev->next_in->engine_num;
      next = adev->next_in;

#ifdef VDEV_SUPPORT
      i=0;
      while (combine_slaves && next != NULL && i++ < num_audio_engines)
	{
	  ainfo_combine_caps (info, next);
	  next = next->next_in;
	}
#endif
    }

  if (dev_present)
    {
      MUTEX_EXIT_IRQRESTORE (adev->mutex, flags);
    }
  return 0;
}

#ifdef CONFIG_OSS_VMIX
static int
vmixctl_attach(vmixctl_attach_t *att)
{
	int err;
	oss_device_t *osdev;

	if (att->masterdev<0 || att->masterdev >= num_audio_engines)
	   return OSS_ENXIO;

	if (att->inputdev != -1)
	if (att->inputdev<0 || att->inputdev >= num_audio_engines)
	   return OSS_ENXIO;

	osdev=audio_engines[att->masterdev]->master_osdev;

	if ((err=vmix_attach_audiodev(osdev, att->masterdev, att->inputdev, att->attach_flags))<0)
	   return err;

	return 0;
}

static int
vmixctl_detach(vmixctl_attach_t *att)
{
	int err;
	oss_device_t *osdev;

	if (att->masterdev<0 || att->masterdev >= num_audio_engines)
	   return OSS_ENXIO;

	osdev=audio_engines[att->masterdev]->master_osdev;

	if ((err=vmix_detach_audiodev(att->masterdev))<0)
	   return err;

	return 0;
}

static int
vmixctl_rate(vmixctl_rate_t *rate)
{
	int err;

	if (rate->masterdev<0 || rate->masterdev >= num_audio_engines)
	   return OSS_ENXIO;

	if ((err=vmix_set_master_rate(rate->masterdev, rate->rate))<0)
	   return err;

	return 0;
}

static int
vmixctl_map_channels(vmixctl_map_t *map)
{
	int err;

	if (map->masterdev < 0 || map->masterdev >= num_audio_engines)
	   return OSS_ENXIO;

	if ((err = vmix_set_channel_map (map->masterdev, &map->map)) < 0)
	  return err;

	return 0;
}
#endif

int
oss_mixer_ext (int orig_dev, int class, unsigned int cmd, ioctl_arg arg)
{
  int val;
  int combine_slaves = 0;
#ifdef MANAGE_DEV_DSP
#ifdef VDEV_SUPPORT
  extern void oss_combine_write_lists (void);
#endif
#endif

  switch (cmd)
    {
    case SNDCTL_SYSINFO:	/* Formerly OSS_SYSINFO */
      {
	oss_sysinfo *info = (oss_sysinfo *) arg;
	int i;

	memset (info, 0, sizeof (*info));
	strcpy (info->product, "OSS");
	strncpy (info->version, OSS_VERSION_STRING, sizeof (info->version));
	info->version [sizeof (info->version) - 1] = '\0';
	strcpy (info->license, oss_license_string);
	info->versionnum = OSS_VERSION;

#ifdef OSS_HG_INFO
	/* Detailed Mercurial version */
	strncpy (info->revision_info, OSS_HG_INFO, sizeof(info->revision_info));
	info->revision_info[sizeof(info->revision_info)-1]=0;
#endif

	memset (info->options, 0, sizeof (info->options));

	info->numaudios = num_audio_devfiles;
	info->numaudioengines = num_audio_engines;
	for (i = 0; i < 8; i++)
	  info->openedaudio[i] = 0;
	for (i = 0; i < num_audio_engines; i++)
	  if (audio_engines[i] != NULL)
	    {
	      if (audio_engines[i]->flags & ADEV_OPENED)
		if (audio_engines[i]->next_out != NULL)
		  {
		    int x = audio_engines[i]->real_dev;
		    info->openedaudio[x / 32] |= 1 << (x % 32);
		  }
	    }
	for (i = 0; i < 8; i++)
	  info->openedmidi[i] = 0;
	for (i = 0; i < num_mididevs; i++)
	  if (midi_devs[i]->open_mode != 0)
	    info->openedmidi[i / 32] |= 1 << (i % 32);

	info->numsynths = 0;
#ifdef CONFIG_OSS_MIDI
	info->nummidis = num_mididevs;
#endif
	info->numtimers = oss_num_timers;
	info->nummixers = num_mixers;
	info->numcards = oss_num_cards;

	return 0;
      }
      break;

    case SNDCTL_MIX_NRMIX:
      return *arg = num_mixers;
      break;

    case SNDCTL_MIX_NREXT:	/* Return # of mixer extensions for device */
      val = *arg;
      *arg = 0;
      if (val==-1)
	 val=orig_dev;
      if (val < 0 || val >= num_mixers)
	return OSS_ENXIO;
      if (mixer_devs[val] == NULL || mixer_devs[val]->unloaded
	  || !mixer_devs[val]->enabled)
	{
	  return OSS_ENXIO;
	}


      touch_mixer (val);
      return *arg = mixer_devs[val]->nr_ext;
      break;

    case SNDCTL_MIX_EXTINFO:
      return oss_mixer_ext_info ((oss_mixext *) arg);
      break;

    case SNDCTL_MIX_ENUMINFO:
      return mixer_ext_get_enuminfo ((oss_mixer_enuminfo *) arg);
      break;

    case SNDCTL_MIX_DESCRIPTION:
      return mixer_ext_get_description ((oss_mixer_enuminfo *) arg);
      break;

    case SNDCTL_MIX_READ:
      return mixer_ext_read ((oss_mixer_value *) arg);
      break;

    case SNDCTL_MIX_WRITE:
      return mixer_ext_write ((oss_mixer_value *) arg);
      break;

    case OSS_GETVERSION:
      return *arg = OSS_VERSION;
      break;

    case SNDCTL_AUDIOINFO:
    case SNDCTL_AUDIOINFO_EX:
      {
	int dev;
	oss_audioinfo *info = (oss_audioinfo *) arg;

	if (info == NULL)
	  return OSS_EFAULT;

	dev = info->dev;

	if (dev == -1)		/* Request for the current device */
	   {
	      oss_audio_set_error (orig_dev, E_PLAY,
				   OSSERR (1022,
					   "SNDCTL_AUDIOINFO called with dev=-1.."),
				   0);
	      /*
	       * Errordesc:
	       * Applications that try to obtain audio device information about
	       * the current device should call SNDCTL_ENGINEINFO instead of
	       * SNDCTL_AUDIOINFO.
	       *
	       * Audio file descriptors returned by open(2) are bound
	       * directly to specific audio engine instead of the
	       * device file.
	       */
	      return OSS_EINVAL;
	  }

	if (dev < 0 || dev >= num_audio_devfiles)
	  {
	    return OSS_EINVAL;
	  }

	if (audio_devfiles[dev] == NULL)
	  {
	    return OSS_EIO;
	  }

	if (dev >= 0)
	  {
	    dev = audio_devfiles[dev]->engine_num;	/* Get the engine number */
	    if (cmd == SNDCTL_AUDIOINFO && info->dev != -1)
	      combine_slaves = 1;
	  }
	return get_engineinfo (dev, info, combine_slaves);
      }
      break;

    case SNDCTL_ENGINEINFO:
      {
	int dev;
	oss_audioinfo *info = (oss_audioinfo *) arg;

	if (info == NULL)
	  return OSS_EFAULT;

	dev = info->dev;

	if (dev == -1)		/* Request for the current device */
	  switch (class)
	    {
	    case OSS_DEV_DSP:
	    case OSS_DEV_DSP_ENGINE:
	      dev = orig_dev;
	      break;

	    default:
	      cmn_err(CE_WARN, "Unrecognized device class %d for dev %d\n", class, orig_dev);
	      return OSS_EINVAL;
	    }

	if (dev < 0 || dev >= num_audio_engines)
	  {
	    return OSS_EINVAL;
	  }

	return get_engineinfo (dev, info, 0);
      }
      break;

#ifdef CONFIG_OSS_MIDI
    case SNDCTL_MIDIINFO:
      {
	int dev;
	oss_native_word flags;
	extern int oss_num_midi_clients;	/* midi.c */

	oss_midi_info *info = (oss_midi_info *) arg;
	mididev_t *mdev;

	dev = info->dev;

	if (dev == -1)		/* Request for the current device */
	  switch (class)
	    {
	    case OSS_DEV_MIDI:
	      /*
	       * Figure out the HW device connected to this client (orig_dev).
	       */
	      dev = orig_dev;
	      if (dev < 0 || dev >= oss_num_midi_clients)
		return OSS_ENXIO;
	      if (oss_midi_clients[dev]->mididev == NULL)
		return OSS_EBUSY;	/* No binding established (yet) */
	      dev = oss_midi_clients[dev]->mididev->dev;
	      break;

	    default:
	      return OSS_EINVAL;
	    }

	if (dev < 0 || dev >= num_mididevs)
	  {
	    return OSS_EINVAL;
	  }

	memset ((char *) info, 0, sizeof (*info));

	mdev = midi_devs[dev];
	MUTEX_ENTER_IRQDISABLE (mdev->mutex, flags);
	info->dev = dev;
	strcpy (info->name, mdev->name);
	strcpy (info->handle, mdev->handle);
	info->pid = mdev->pid;
	info->busy = mdev->open_mode;
	*info->cmd = 0;
	strncpy (info->cmd, mdev->cmd, sizeof (info->cmd));
	info->cmd[sizeof (info->cmd) - 1] = 0;
	info->magic = mdev->magic;
	info->card_number = mdev->card_number;
	strcpy (info->devnode, mdev->devnode);
	info->legacy_device = mdev->real_dev;
	info->port_number = mdev->port_number;
	info->enabled = mdev->enabled;
	info->flags = mdev->flags;
	info->caps = mdev->caps;
	info->latency = mdev->latency;
	if (!(info->caps & MIDI_CAP_INOUT))
	  info->caps |= MIDI_CAP_INOUT;
	if (mdev->flags & MFLAG_VIRTUAL)
	  info->caps |= MIDI_CAP_VIRTUAL;
	if (mdev->flags & MFLAG_CLIENT)
	  info->caps |= MIDI_CAP_CLIENT;
	if (mdev->flags & MFLAG_SERVER)
	  info->caps |= MIDI_CAP_SERVER;
	if (mdev->flags & MFLAG_INTERNAL)
	  info->caps |= MIDI_CAP_INTERNAL;
	if (mdev->flags & MFLAG_EXTERNAL)
	  info->caps |= MIDI_CAP_EXTERNAL;
	if (mdev->flags & MFLAG_MTC)
	  info->caps |= MIDI_CAP_MTC;
	if (midi_devs[dev]->enabled && !midi_devs[dev]->unloaded)
	  if (midi_devs[dev]->d->ioctl)
	    {
	      midi_devs[dev]->d->ioctl (dev, SNDCTL_GETSONG,
					(ioctl_arg) info->song_name);
	      midi_devs[dev]->d->ioctl (dev, SNDCTL_GETLABEL,
					(ioctl_arg) info->label);
	    }
	if (*info->label == 0)
	  {
	    strncpy (info->label, info->cmd, sizeof (info->label));
	    info->label[sizeof (info->label) - 1] = 0;
	  }
	MUTEX_EXIT_IRQRESTORE (mdev->mutex, flags);
      }
      return 0;
      break;
#endif

    case SNDCTL_CARDINFO:
      {
	int card, err;

	oss_card_info *info = (oss_card_info *) arg;
	card = info->card;

	if (card < 0 || card >= oss_num_cards)
	  {
	    return OSS_ENXIO;
	  }

	memset ((char *) info, 0, sizeof (*info));
	if ((err = oss_get_cardinfo (card, info)) < 0)
	  return err;
	info->card = card;
      }
      return 0;
      break;

    case SNDCTL_MIXERINFO:
      {
	int dev;

	oss_mixerinfo *info = (oss_mixerinfo *) arg;
	mixer_operations_t *mdev;

	dev = info->dev;

	if (dev == -1)		/* Request for the current device */
	  switch (class)
	    {
	    case OSS_DEV_MIXER:
	      dev = orig_dev;
	      break;

	    default:
	      return OSS_EINVAL;
	    }

	if (dev < 0 || dev >= num_mixers)
	  {
	    return OSS_ENXIO;
	  }

	if (mixer_devs[dev] == NULL)
	  return OSS_ENXIO;

	memset ((char *) info, 0, sizeof (*info));
	touch_mixer (dev);

	mdev = mixer_devs[dev];
	info->dev = dev;
	strncpy (info->name, mdev->name, sizeof (info->name));
	info->name[sizeof (info->name) - 1] = '\0';
	strcpy (info->id, mdev->id);
	strcpy (info->handle, mdev->handle);
	info->card_number = mdev->card_number;
	info->port_number = mdev->port_number;
	info->enabled = (mdev->enabled && !mdev->unloaded);
	info->magic = mdev->magic;
	info->caps = mdev->caps;
	info->flags = mdev->flags;
	info->modify_counter = mdev->modify_counter;
	info->nrext = mdev->nr_ext;
	info->priority = mdev->priority;
	strcpy (info->devnode, mdev->devnode);
	info->legacy_device = mdev->real_dev;
      }
      return 0;
      break;

    case OSSCTL_RENUM_AUDIODEVS:
      {
	oss_renumber_t *r = (oss_renumber_t *) arg;
	int i;

#ifdef GET_PROCESS_UID
	if (GET_PROCESS_UID () != 0)	/* Not root */
	  return OSS_EINVAL;
#endif

	if (r->n != num_audio_devfiles)	/* Wrong map size? */
	  {
	    cmn_err (CE_NOTE, "Legacy audio map size mismatch %d/%d\n",
		     r->n, num_audio_devfiles);
	    return OSS_EINVAL;
	  }

	for (i = 0; i < r->n; i++)
	  {
	    adev_p adev = audio_devfiles[i];

	    if (r->map[i] >= HARD_MAX_AUDIO_DEVFILES)	/* May be unnecessary check */
	      return OSS_EINVAL;

	    if (r->map[i] < -1)
	      r->map[i] = -1;

	    adev->real_dev = r->map[i];
	  }
      }
      return 0;
      break;

    case OSSCTL_RENUM_MIXERDEVS:
      {
	oss_renumber_t *r = (oss_renumber_t *) arg;
	int i;

#ifdef GET_PROCESS_UID
	if (GET_PROCESS_UID () != 0)	/* Not root */
	  return OSS_EINVAL;
#endif

	if (r->n != num_mixers)	/* Wrong map size? */
	  return OSS_EINVAL;

	for (i = 0; i < r->n; i++)
	  {
	    mixdev_p mdev = mixer_devs[i];

	    if (r->map[i] >= HARD_MAX_AUDIO_DEVFILES)	/* May be unnecessary check */
	      return OSS_EINVAL;

	    mdev->real_dev = r->map[i];
	  }
      }
      return 0;
      break;

    case OSSCTL_RENUM_MIDIDEVS:
      {
	oss_renumber_t *r = (oss_renumber_t *) arg;
	int i;

#ifdef GET_PROCESS_UID
	if (GET_PROCESS_UID () != 0)	/* Not root */
	  return OSS_EINVAL;
#endif

	if (r->n != num_mididevs)	/* Wrong map size? */
	  return OSS_EINVAL;

	for (i = 0; i < r->n; i++)
	  {
	    mididev_p mdev = midi_devs[i];

	    if (r->map[i] >= HARD_MAX_AUDIO_DEVFILES)	/* May be unnecessary check */
	      return OSS_EINVAL;

	    mdev->real_dev = r->map[i];
	  }
      }
      return 0;
      break;

#ifdef CONFIG_OSS_VMIX
    case VMIXCTL_ATTACH:
#ifdef GET_PROCESS_UID
	if (GET_PROCESS_UID () != 0)	/* Not root */
	  return OSS_EINVAL;
#endif
      return vmixctl_attach((vmixctl_attach_t*)arg);
      break;

    case VMIXCTL_DETACH:
#ifdef GET_PROCESS_UID
	if (GET_PROCESS_UID () != 0)	/* Not root */
	  return OSS_EINVAL;
#endif
      return vmixctl_detach((vmixctl_attach_t*)arg);
      break;

    case VMIXCTL_RATE:
#ifdef GET_PROCESS_UID
	if (GET_PROCESS_UID () != 0)	/* Not root */
	  return OSS_EINVAL;
#endif
      return vmixctl_rate((vmixctl_rate_t*)arg);
      break;

    case VMIXCTL_REMAP:
#ifdef GET_PROCESS_UID
	if (GET_PROCESS_UID () != 0)	/* Not root */
	  return OSS_EINVAL;
#endif
      return vmixctl_map_channels((vmixctl_map_t *)arg);
      break;

#endif

#if 0
/*
 * These calls are obsolete and disabled in current OSS version.
 */
    case OSSCTL_GET_REROUTE:
      {
	oss_reroute_t *r = (oss_reroute_t *) arg;

#ifdef GET_PROCESS_UID
	if (GET_PROCESS_UID () != 0)	/* Not root */
	  {
	    return OSS_EINVAL;
	  }
#endif

	switch (r->mode)
	  {
	  case OPEN_READ:
	    memcpy (&r->devlist, &dspinlist, sizeof (oss_devlist_t));
	    break;

	  case OPEN_WRITE:
#ifdef MANAGE_DEV_DSP
#ifdef VDEV_SUPPORT
	    oss_combine_write_lists ();
#endif
#endif
	    memcpy (&r->devlist, &dspoutlist, sizeof (oss_devlist_t));
	    break;

	  case OPEN_WRITE | OPEN_READ:
	    memcpy (&r->devlist, &dspinoutlist, sizeof (oss_devlist_t));
	    break;

	  default:
	    return OSS_EINVAL;
	  }
      }
      return 0;
      break;

#if 0
    case OSSCTL_SET_REROUTE:
      {
	oss_reroute_t *r = (oss_reroute_t *) arg;
	int i, d;

#ifdef GET_PROCESS_UID
	if (GET_PROCESS_UID () != 0)	/* Not root */
	  return OSS_EINVAL;
#endif

	for (i = 0; i < r->devlist.ndevs; i++)
	  {
	    if ((d = r->devlist.devices[i]) < 0 || d >= num_audio_devfiles)
	      return OSS_EINVAL;
	  }

	switch (r->mode)
	  {
	  case OPEN_READ:
	    /* Refuse if number of devices has changed */
	    if (!check_list (&r->devlist, &dspinlist))
	      {
		return OSS_EINVAL;
	      }
	    memcpy (&dspinlist, &r->devlist, sizeof (oss_devlist_t));
	    break;

	  case OPEN_WRITE:
#ifdef MANAGE_DEV_DSP
#ifdef VDEV_SUPPORT
	    oss_combine_write_lists ();
#endif
#endif
	    /* Refuse if number of devices has changed */
	    if (!check_list (&r->devlist, &dspoutlist))
	      {
		return OSS_EINVAL;
	      }
	    memcpy (&dspoutlist, &r->devlist, sizeof (oss_devlist_t));
	    dspoutlist2.ndevs = 0;
	    break;

	  case OPEN_WRITE | OPEN_READ:
	    /* Refuse if number of devices has changed */
	    if (!check_list (&r->devlist, &dspinoutlist))
	      {
		return OSS_EINVAL;
	      }
	    memcpy (&dspinoutlist, &r->devlist, sizeof (oss_devlist_t));
	    break;

	  default:
	    return OSS_EINVAL;
	  }
      }
      return 0;
      break;
#endif

#ifdef APPLIST_SUPPORT
    case OSSCTL_RESET_APPLIST:
#ifdef GET_PROCESS_UID
      if (GET_PROCESS_UID () != 0)	/* Not root */
	return OSS_EINVAL;
#endif

      oss_applist_size = 0;
      return 0;
      break;

    case OSSCTL_ADD_APPLIST:
      {
	app_routing_t *def, *parm = (app_routing_t *) arg;

#ifdef GET_PROCESS_UID
	if (GET_PROCESS_UID () != 0)	/* Not root */
	  return OSS_EINVAL;
#endif

	if (oss_applist_size >= APPLIST_SIZE)
	  return OSS_ENOSPC;

	if (parm->dev < -1 || parm->dev >= num_audio_devfiles)
	  return OSS_ENXIO;

	def = &oss_applist[oss_applist_size];

	memset (def, 0, sizeof (*def));
	strcpy (def->name, parm->name);
	def->mode = parm->mode & (OPEN_READ | OPEN_WRITE);
	def->dev = parm->dev;
	def->open_flags = parm->open_flags;
	oss_applist_size++;
	return 0;
      }
      break;

#endif
#endif
    default:
#if 0
      if (mixer_devs[orig_dev]->d->ioctl != NULL)
         return mixer_devs[orig_dev]->d->ioctl (orig_dev, -1, cmd, arg);
#endif

      return OSS_EINVAL;
    }
}

/*ARGSUSED*/
static int
oss_mixer_open (int dev, int dev_type, struct fileinfo *file, int recursive,
		int open_flags, int *newdev)
{
/*
 * Permit opening nonexistent mixer so that certain mixer ioctl calls
 * can be called. Other code must check that the devices really exist
 * before permitting the calls.
 */
  if (dev >= 0 && dev < num_mixers)
    return 0;

  if (mixer_devs == NULL)
    return 0;

  if (mixer_devs[dev]->unloaded)
    return OSS_ENODEV;

  if (!mixer_devs[dev]->enabled)
    return OSS_ENXIO;

  return 0;
}

/*ARGSUSED*/
static void
oss_mixer_release (int dev, struct fileinfo *file)
{
}

/*ARGSUSED*/
int
oss_mixer_ioctl (int dev, struct fileinfo *bogus,
		 unsigned int cmd, ioctl_arg arg)
{
  int ret;

  if (cmd == OSS_GETVERSION)
    return *arg = OSS_VERSION;

/*
 * Handle SNDCTL_SYSINFO/CARDINFO/etc even if there are no mixer devices in the
 * system.
 */
  switch (cmd)
  {
      case OSS_GETVERSION:
      case SNDCTL_SYSINFO:
      case SNDCTL_CARDINFO:
      case SNDCTL_MIXERINFO:
      case SNDCTL_MIDIINFO:
      case SNDCTL_AUDIOINFO:
      case SNDCTL_AUDIOINFO_EX:
      case SNDCTL_ENGINEINFO:
      case OSSCTL_RENUM_AUDIODEVS:
      case OSSCTL_RENUM_MIXERDEVS:
      case OSSCTL_RENUM_MIDIDEVS:
      case SNDCTL_MIX_EXTINFO:
      case SNDCTL_MIX_ENUMINFO:
      case SNDCTL_MIX_READ:
      case SNDCTL_MIX_WRITE:
      case SNDCTL_MIX_NRMIX:
      case SNDCTL_MIX_MATRIX_WRITE:
      case SNDCTL_MIX_MATRIX_READ:
      case VMIXCTL_ATTACH:
      case VMIXCTL_DETACH:
      case VMIXCTL_RATE:
         return oss_mixer_ext (dev, OSS_DEV_MIXER, cmd, arg);
         break;
  }

  if (dev < 0 || dev >= num_mixers)
    {
      return OSS_ENXIO;
    }

  if (mixer_devs == NULL)
    {
      return OSS_ENXIO;
    }

  if (!mixer_devs[dev]->enabled || mixer_devs[dev]->unloaded)
    {
      return OSS_ENODEV;
    }

  if ((ret = oss_legacy_mixer_ioctl (dev, -1, cmd, arg)) != OSS_EINVAL)
    {
      return ret;
    }

  return oss_mixer_ext (dev, OSS_DEV_MIXER, cmd, arg);
}

#ifdef DO_TIMINGS

static char timing_buf[256 * 1024] = { 0 }, *timing_ptr = timing_buf;
static int timing_prev_time = 0;
int timing_flags = 0x7fffffff;

#define TM_SCALE 256
static oss_timing_timer_func tmfunc = NULL;
static void *tmfunc_arg = NULL;

void
timing_install_timer (oss_timing_timer_func f, void *x)
{
  tmfunc = f;
  tmfunc_arg = x;
}

static void
oss_do_timing_ (char *txt)
{
  int l = strlen (txt) + 20;
  oss_native_word flags;
  oss_native_word this_time;

  if (!timing_is_active) /* Nobody is listening */
     return;

  MUTEX_ENTER_IRQDISABLE (oss_timing_mutex, flags);
  if ((long) (&timing_buf[sizeof (timing_buf)] - timing_ptr - 8) <= l)
    {
      MUTEX_EXIT_IRQRESTORE (oss_timing_mutex, flags);
      return;
    }

  if (tmfunc != NULL)
    {
      this_time = tmfunc (tmfunc_arg);
    }
  else
    {
      this_time = 0;		/* TODO: Get the actual audio pointer */

      if (this_time == 0)
	this_time = GET_JIFFIES ();
    }

  sprintf (timing_ptr, "%ld/%d: %s\n", this_time,
	   this_time - timing_prev_time, txt);
  l = strlen (timing_ptr);
  timing_ptr += l;
  timing_prev_time = this_time;
  MUTEX_EXIT_IRQRESTORE (oss_timing_mutex, flags);
}

typedef struct
{
  oss_native_word sum;
  oss_native_word open_time;
}
timing_entry;

static timing_entry timing_bins[DF_NRBINS];
static oss_native_word timing_start = 0;

static char *bin_names[DF_NRBINS] = {
  "Iddle",
  "Write",
  "Read",
  "Interrupt",
  "Write sleep",
  "Read sleep",
  "Write SRC",
  "Read SRC"
};

void
timing_open (void)
{
  int i;

  if (tmfunc == NULL)
    return;
  timing_start = tmfunc (tmfunc_arg) / TM_SCALE;

  for (i = 0; i < DF_NRBINS; i++)
    {
      timing_bins[i].sum = 0;
      timing_bins[i].open_time = 0xffffffff;
    }

}

void
timing_close (void)
{
  char tmp[64];
  int i;
  oss_native_word t, sum, pc;
  if (tmfunc == NULL)
    return;

  t = tmfunc (tmfunc_arg) / TM_SCALE - timing_start;

  sprintf (tmp, "Timing close, elapsed=%d", t);
  oss_do_timing2 (DFLAG_PROFILE, tmp);

  sum = 0;

  for (i = 0; i < DF_NRBINS; i++)
    {
      sum += timing_bins[i].sum;
      pc = (timing_bins[i].sum * 1000) / t;
      sprintf (tmp, "Bin %s: %d/%d (%d.%d%%)", bin_names[i],
	       timing_bins[i].sum, pc, pc / 10, pc % 10);
      oss_do_timing2 (DFLAG_PROFILE, tmp);
    }

  /* sum = sum-timing_bins[DF_SLEEPWRITE].sum-timing_bins[DF_SLEEPREAD].sum; */
  pc = (sum * 10000) / t;

  sprintf (tmp, "OSS Total: %d (%d.%d%%)", sum, pc / 100, pc % 100);
  oss_do_timing2 (DFLAG_PROFILE, tmp);
}

void
oss_timing_enter (int bin)
{
  if (tmfunc == NULL)
    return;

  timing_bins[bin].open_time = tmfunc (tmfunc_arg) / TM_SCALE;
}

void
oss_timing_leave (int bin)
{
  oss_native_word t;

  if (tmfunc == NULL)
    return;

  if (timing_bins[bin].open_time >= 0xfffffffe)
    return;

  t = tmfunc (tmfunc_arg) / TM_SCALE - timing_bins[bin].open_time;
  timing_bins[bin].sum += t;
  timing_bins[bin].open_time = 0xfffffffe;
}

void
oss_do_timing (char *txt)
{
  if (!timing_is_active) /* Nobody is listening */
     return;

  if (timing_flags & DFLAG_ALL)
    oss_do_timing_ (txt);
}

void
oss_do_timing2 (int mask, char *txt)
{
  if (!timing_is_active) /* Nobody is listening */
     return;

  if ((timing_flags & DFLAG_ALL) || (timing_flags & mask))
    oss_do_timing_ (txt);
}

void
oss_timing_printf (char *s, ...)
{
  char tmp[1024], *a[6];
  va_list ap;
  int i, n = 0;

  if (!timing_is_active) /* Nobody is listening */
     return;

  va_start (ap, s);

  for (i = 0; i < strlen (s); i++)
    if (s[i] == '%')
      n++;

  for (i = 0; i < n && i < 6; i++)
    a[i] = va_arg (ap, char *);

  for (i = n; i < 6; i++)
    a[i] = NULL;

      sprintf (tmp, s, a[0], a[1], a[2], a[3], a[4], a[5], NULL,
	       NULL, NULL, NULL);
      oss_do_timing(tmp);

  va_end (ap);
}

static int
timing_read (int dev, struct fileinfo *file, uio_t * buf, int count)
{
  /*
   * Return at most 'count' bytes from the status_buf.
   */
  int l;
  oss_native_word flags;

  timing_is_active = 1;

  MUTEX_ENTER_IRQDISABLE (oss_timing_mutex, flags);

  l = timing_ptr - timing_buf;
  if (l <= 0)
    {
      MUTEX_EXIT_IRQRESTORE (oss_timing_mutex, flags);
      return 0;
    }

  if (l > count)
    l = count;

  timing_ptr = timing_buf;

  MUTEX_EXIT_IRQRESTORE (oss_timing_mutex, flags);

  if (uiomove (timing_buf, l, UIO_READ, buf) != 0)
    cmn_err (CE_WARN, "audio: uiomove(UIO_READ) failed\n");

  return l;
}
#else
/*
 * Dummy wrappers
 */

/*ARGSUSED*/
void
oss_timing_enter (int bin)
{
}

/*ARGSUSED*/
void
oss_timing_leave (int bin)
{
}

/*ARGSUSED*/
void
oss_do_timing (char *txt)
{
}

/*ARGSUSED*/
void
oss_do_timing2 (int mask, char *txt)
{
}

/*ARGSUSED*/
void
oss_timing_printf (char *s, ...)
{
}
#endif

static oss_cdev_drv_t mixer_cdev_drv = {
  oss_mixer_open,
  oss_mixer_release,
#ifdef DO_TIMINGS
  timing_read,
#else
  NULL,				/* read */
#endif
  NULL,				/* write */
  oss_mixer_ioctl
};

int
oss_install_mixer (int vers,
		   oss_device_t * osdev,
		   oss_device_t * master_osdev,
		   const char *name,
		   mixer_driver_t * driver, int driver_size, void *devc)
{
  mixer_operations_t *op = NULL;
  mixer_driver_t *d;

  int i, num;
  char handle[32];

  if (master_osdev == NULL)
    master_osdev = osdev;

  if (mixer_devs == NULL)
    {
      mixer_devs = PMALLOC (osdev, sizeof (mixdev_p) * MAX_MIXER_DEV);
      memset (mixer_devs, 0, sizeof (mixdev_p) * MAX_MIXER_DEV);
      mixer_devs_p = mixer_devs;
    }

  if (num_mixers >= MAX_MIXER_DEV - 1)
    {
      static int nnn = 0;
      cmn_err (CE_WARN, "Too many mixer devices %d/%d (%s)\n",
	       num_mixers, MAX_MIXER_DEV, name);
      /*
       * In some special situations a driver may keep trying to install a mixer
       * in infinite loop if the request fails. Stop this by panicking after
       * this has continued for more than 50 times. In this case we can get an
       * error message instead of having the system to lock up foreever.
       */
      if (nnn++ > 50)
	cmn_err (CE_PANIC, "Killing runaway system.\n");
      return OSS_EIO;
    }

  if (vers != OSS_MIXER_DRIVER_VERSION)
    {
      cmn_err (CE_WARN, "Incompatible mixer driver for %s\n", name);
      return OSS_EIO;
    }

  if (driver_size > sizeof (mixer_driver_t))
    driver_size = sizeof (mixer_driver_t);

/*
 * Check if this device was earlier unloaded and now returning back.
 */
  num = -1;
  for (i = 0; i < num_mixers; i++)
    {
      if (mixer_devs[i]->unloaded
	  && mixer_devs[i]->os_id == oss_get_osid (osdev))
	{
	  op = mixer_devs[i];
	  num = i;
	  break;
	}
    }

  if ((d = PMALLOC (osdev, sizeof (*d))) == NULL)
    {
      cmn_err (CE_WARN, "Can't allocate mixer driver for (%s)\n", name);
      return OSS_ENOSPC;
    }

  if (num == -1)
    {
      op = PMALLOC (osdev, sizeof (mixer_operations_t));
      if (op == NULL)
	{
	  cmn_err (CE_WARN, "Can't allocate mixer driver for (%s)\n", name);
	  return OSS_ENOSPC;
	}

      memset ((char *) op, 0, sizeof (mixer_operations_t));
      num = num_mixers++;
      sprintf (handle, "%s-mx%02d", osdev->handle, osdev->num_mixerdevs+1);
      op->port_number = osdev->num_mixerdevs++;
    }
  else
    {
      strcpy (handle, op->handle);	/* Preserve the previous handle */
    }

  memset ((char *) d, 0, sizeof (mixer_driver_t));
  memcpy ((char *) d, (char *) driver, driver_size);
  strcpy (op->handle, handle);
  strcpy (op->id, osdev->nick);
  op->d = d;

  strncpy (op->name, name, sizeof (op->name));
  op->name[sizeof (op->name) - 1] = 0;
  op->devc = devc;
  op->osdev = osdev;
  op->os_id = oss_get_osid (osdev);
  op->master_osdev = master_osdev;
  op->hw_devc = NULL;
  op->max_ext = op->nr_ext = 0;
  op->names_checked = 0;
  op->extensions = NULL;
  op->timestamp = GET_JIFFIES ();
  op->ignore_mask = 0;
  op->card_number = osdev->cardnum;
  op->enabled = 1;
  op->unloaded = 0;
  op->flags = 0;
  op->caps = 0;
  op->priority = 0;		/* Normal (low) priority */
  op->real_dev = num;

  if (osdev->first_mixer == -1) /* Not defined yet */
     osdev->first_mixer = num;

  mixer_devs[num] = op;
/*
 * Create the device node
 */

  {
    oss_devnode_t name;

#ifdef NEW_DEVICE_NAMING
# ifdef USE_DEVICE_SUBDIRS
    sprintf (name, "oss/%s/mix%d", osdev->nick, osdev->num_mixerdevs - 1);
# else
    sprintf (name, "%s_mix%d", osdev->nick, osdev->num_mixerdevs - 1);
# endif
#else
    sprintf (name, "mixer%d", num);
#endif
    oss_install_chrdev (osdev, name, OSS_DEV_MIXER, num, &mixer_cdev_drv, 0);
    sprintf (op->devnode, "/dev/%s", name);

#if 0
    /*
     * Moved to install_dev_mixer()
     */
    if (num == 0)
      {
	oss_install_chrdev (osdev, "mixer", OSS_DEV_MIXER, num,
			    &mixer_cdev_drv, 0);
      }
#endif
  }

  return num;
}

void
install_dev_mixer (oss_device_t * osdev)
{
/*
 * Install the default mixer node if necessary
 */
  oss_install_chrdev (osdev, "mixer", OSS_DEV_MIXER, 0, &mixer_cdev_drv, 0);
}
