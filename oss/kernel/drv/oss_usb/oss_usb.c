/*
 * Purpose: Top level USB audio class initialization and mixer functions
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

#define KBUILD_MODNAME oss_usb
#include "oss_config.h"
#include "ossusb.h"


static const udi_usb_devinfo known_devices[] = {
  {0x763, 0x2002, "M Audio USB AudioSport Duo",
   {
    {"OFF 24bit_hispeed 16bit_hispeed 24bit_lowspeed 16bit_lowspeed", 2,
     0x0014},
    {"OFF 24bit_hispeed 16bit_hispeed 24bit_lowspeed 16bit_lowspeed", 2,
     0x0014}
    }
   },
  {0x763, 0x2001, "M Audio USB AudioSport Quatro"},
  {0x763, 0x2805, "M Audio Sonica"},
  {0x763, 0x2007, "M Audio Sonica Theatre",
   {
    {"OFF 24bit_hispeed 16bit_hispeed 16bit_hispeed 24bit_lowspeed 24bit24bit 16bit 16bit OFF OFF", 3},
    {"OFF 24bit_hispeed 16bit_hispeed 24bit 16bit_lowspeed", 2}
    }
   },
  {0x763, 0x200d, "M Audio OmniStudio USB"},
  {0x41e, 0x3000, "Creative Sound Blaster Extigy"},
  {0x41e, 0x3010, "Creative Sound Blaster MP3+"},
  {0x41e, 0x3020, "Creative Audigy2 NX USB",
   {
    {"OFF 16bit22kHz 16bit44kHz "	/* 0-2 */
     "24bit44kHz 16bit48kHz 24bit48kHz "	/* 3-5 */
     "16bit96kHz 24bit96kHz 16bit22kHz "	/* 6-8 */
     "16bit48kHz 24bit48kHz 16bit5.1ch22kHz "	/* 9-11 */
     "16bit5.1ch48kHz 24bit5.1ch48kHz 16bit7.1ch22kHz "	/* 12-14 */
     "16bit7.1ch48kHz" /* 15 */ , 4},

    {"OFF 16bit32kHz 24bit32kHz "	/* 0-2 */
     "16bit44kHz 24bit44kHz 16bit48kHz "	/* 3-5 */
     "24bit48kHzin 24bit96kHz 24bit96kHz ",	/* 6-8 */
     5}
    }
   },
  {0x46d, 0x8b2, "Logitec Quickcam Pro 4000 (mic)"},
  {0x46d, 0xa01, "Logitec USB Headset"},
  {0x0471, 0x0311, "Philips ToUcam Pro (mic)"},
  {0x672, 0x1041, "Labtec LCS1040 Speaker System"},
  {0x6f8, 0xc000, "Hercules Gamesurround MUSE Pocket"},
  {0x0d8c, 0x000c, "C-Media USB audio"},
  {0x0d8c, 0x0102, "C-Media 2/4/6/8ch USB audio",
   {
    {"OFF 7.1 2.0 3.1 5.1 digital", 2},
    {"OFF stereo"}
    }
   },
  {0x0d8c, 0x0103, "C-Media USB audio"},
  {-1, -1, "USB sound device"}
};

typedef struct
{
  unsigned int devid;
  special_driver_t driver;
}
special_table_t;

extern int usb_mixerstyle;

#define MAX_DEVC	10
static ossusb_devc *devc_list[MAX_DEVC];
static int ndevs = 0;
int usb_quiet = 0;


void
ossusb_dump_desc (unsigned char *desc, int cfg_len)
{
  int i;

  for (i = 0; i < cfg_len; i++)
    {
      if (!(i % 16))
	cmn_err (CE_CONT, "\n%04x: ", i);
      cmn_err (CE_CONT, "%02x ", desc[i]);
    }
  cmn_err (CE_CONT, "\n");
}

unsigned int
ossusb_get_int (unsigned char *p, int nbytes)
{
  unsigned int v = 0;
  int i;

  for (i = 0; i < nbytes; i++)
    {
      v |= (*p++) << (i * 8);
    }

  return v;
}

/*
 * Mixer stuff
 */

static int
decode_vol (usb_control_t * c, unsigned char *buf, int l)
{
  int value, range;

  if (l == 1)
    value = (signed char) buf[0];
  else
    value = (signed short) (buf[0] | (buf[1] << 8));

  range = c->max - c->min;

  value -= c->min;
  value = (value * c->scale + range / 2) / range;

  if (value < 0)
    value = 0;
  if (value > 255)
    value = 255;
  return value;
}

static void
encode_vol (usb_control_t * c, unsigned char *buf, int l, int value)
{
  int range;

  range = c->max - c->min;

  value = (value * range + c->scale / 2) / c->scale;

  value += c->min;

  if (l == 1)
    buf[0] = value;
  else
    {
      buf[0] = value & 0xff;
      buf[1] = (value >> 8) & 0xff;
    }

}

static int
read_feature_value (ossusb_devc * devc, usb_control_t * c, int rq, int *v)
{
  int len, value = 0, l;
  int nch, ch;
  unsigned char buf[2];

/*
 * Volume controls use two message bytes while the others use just one.
 */
  l = (c->index == 1) ? 2 : 1;

  ch = c->min_ch + 1;
  if (c->global)
    ch = 0;

  if (rq != GET_CUR)
    {
      buf[0] = buf[1] = 0;
      len = udi_usb_rcv_control_msg (devc->mixer_usbdev, 0,	// endpoint
				     rq, USB_RECIP_INTERFACE | USB_TYPE_CLASS,	// rqtype
				     ((c->index + 1) << 8) | (ch),	// value
				     (c->unit->num << 8),	// index
				     buf,	// buffer
				     l,	// buflen
				     OSS_HZ * 2);
      if (len < 0)
	return 0;

      value =
	(len ==
	 1) ? (signed char) buf[0] : (signed short) (buf[0] | (buf[1] << 8));
      *v = value;
      return 1;
    }

  nch = c->max_ch - c->min_ch + 1;

  buf[0] = buf[1] = 0;
  len = udi_usb_rcv_control_msg (devc->mixer_usbdev, 0,	// endpoint
				 GET_CUR, USB_RECIP_INTERFACE | USB_TYPE_CLASS,	// rqtype
				 ((c->index + 1) << 8) | (ch),	// value
				 (c->unit->num << 8),	// index
				 buf,	// buffer
				 l,	// buflen
				 OSS_HZ * 2);
  if (len < 0)
    {
      // cmn_err(CE_CONT, "feature (%s/%d) read error %d\n", c->unit->name, c->index, len);
      *v = 0;
      return 0;
    }

  value = decode_vol (c, buf, len);

  if (!c->global)
    if (nch == 2)		/* Read the right channel */
      {
	len = udi_usb_rcv_control_msg (devc->mixer_usbdev, 0,	// endpoint
				       GET_CUR, USB_RECIP_INTERFACE | USB_TYPE_CLASS,	// rqtype
				       ((c->index + 1) << 8) | (ch + 1),	// value
				       (c->unit->num << 8),	// index
				       buf,	// buffer
				       l,	// buflen
				       OSS_HZ * 2);
	if (len < 0)
	  value |= (value & 0xff) << 8;
	else
	  value |= (decode_vol (c, buf, len)) << 8;
      }

  *v = value;
  return 1;
}

static int
write_feature_value (ossusb_devc * devc, usb_control_t * c, int value)
{
  int len, l;
  int left, right;
  int ch, nch;
  unsigned char buf[2];

  ch = c->min_ch + 1;
  if (c->global)
    ch = 0;

  left = value & 0xff;
  right = (value >> 8) & 0xff;

  if (left > (c->max - c->min))
    left = c->max - c->min;
  if (right > (c->max - c->min))
    right = c->max - c->min;

  l = (c->index == 1) ? 2 : 1;

  nch = c->max_ch - c->min_ch + 1;
  buf[0] = buf[1] = 0;
  encode_vol (c, buf, l, left);

  len = udi_usb_snd_control_msg (devc->mixer_usbdev, 0,	// endpoint
				 SET_CUR, USB_RECIP_INTERFACE | USB_TYPE_CLASS,	// rqtype
				 ((c->index + 1) << 8) | ch,	// value
				 (c->unit->num << 8),	// index
				 buf,	// buffer
				 l,	// buflen
				 OSS_HZ * 2);
  if (len < 0)
    {
      cmn_err (CE_CONT, "feature write error %d\n", len);
      return len;
    }

  if (nch == 2)			/* Write the right channel */
    {
      buf[0] = buf[1] = 0;
      encode_vol (c, buf, l, right);

      len = udi_usb_snd_control_msg (devc->mixer_usbdev, 0,	// endpoint
				     SET_CUR, USB_RECIP_INTERFACE | USB_TYPE_CLASS,	// rqtype
				     ((c->index + 1) << 8) | (ch + 1),	// value
				     (c->unit->num << 8),	// index
				     buf,	// buffer
				     l,	// buflen
				     OSS_HZ * 2);
    }
  else
    right = left;

  value = left | (right << 8);
  return value;
}

static int
read_mixer_value (ossusb_devc * devc, usb_control_t * c, int rq, int *v)
{
  int len, value = 0;
  unsigned char buf[2];

  if (rq != GET_CUR)
    {
      len = udi_usb_rcv_control_msg (devc->mixer_usbdev, 0,	// endpoint
				     rq, USB_RECIP_INTERFACE | USB_TYPE_CLASS,	// rqtype
				     (c->index << 8) | c->min_ch,	// value
				     (c->unit->num << 8) | 1,	// index
				     buf,	// buffer
				     2,	// buflen
				     OSS_HZ * 2);
      if (len < 0)
	return 0;

      *v = (signed char) buf[1];
      return 1;
    }

  len = udi_usb_rcv_control_msg (devc->mixer_usbdev, 0,	// endpoint
				 rq, USB_RECIP_INTERFACE | USB_TYPE_CLASS,	// rqtype
				 (c->index << 8) | c->min_ch,	// value
				 (c->unit->num << 8) | 1,	// index
				 buf,	// buffer
				 2,	// buflen
				 OSS_HZ * 2);
  if (len < 0)
    {
      cmn_err (CE_CONT, "mixer read error %d\n", len);
      return 0;
    }

  value = buf[1] - c->min;

  *v = value;
  return 1;
}

static int
write_mixer_value (ossusb_devc * devc, usb_control_t * c, int value)
{
  int len;
  unsigned char buf[2];

  value &= 0xff;

  buf[0] = 0;
  buf[1] = value + c->min;
  len = udi_usb_snd_control_msg (devc->mixer_usbdev, 0,	// endpoint
				 SET_CUR, USB_RECIP_INTERFACE | USB_TYPE_CLASS,	// rqtype
				 (c->index << 8) | c->min_ch,	// value
				 (c->unit->num << 8) | 1,	// index
				 buf,	// buffer
				 2,	// buflen
				 OSS_HZ * 2);
  if (len < 0)
    {
      cmn_err (CE_CONT, "mixer write error %d\n", len);
      return 0;
    }

  return value;
}

static int
read_unit (ossusb_devc * devc, usb_control_t * c, int rq, int *v)
{
  int err, value;
  unsigned char buf[2];

  *v = value = 0;

  switch (c->unit->typ)
    {
    case TY_FEAT:
      return read_feature_value (devc, c, rq, v);
      break;

    case TY_MIXER:
      return read_mixer_value (devc, c, rq, v);
      break;

    case TY_SELECT:
      err = udi_usb_rcv_control_msg (devc->mixer_usbdev, 0,	// endpoint
				     rq, USB_RECIP_INTERFACE | USB_TYPE_CLASS,	// rqtype
				     0,	// value
				     (c->unit->num << 8),	// index
				     buf,	// buffer
				     1,	// buflen
				     OSS_HZ * 2);
      if (err < 0)
	{
	  cmn_err (CE_CONT, "USB mixer unit read error %d\n", err);
	  return 0;
	}
      value = buf[0] - 1;
      break;

    }				/* switch */

  *v = value;
  return 1;
}

static int
read_current_value (ossusb_devc * devc, usb_control_t * c)
{
  int v;

  if (!read_unit (devc, c, GET_CUR, &v))
    return 0;

  c->value = v;

  return 1;
}

static int
write_current_value (ossusb_devc * devc, usb_control_t * c, int value)
{
  unsigned char buf[2];
  int err;

  switch (c->unit->typ)
    {
    case TY_SELECT:
      if (value > c->max)
	value = c->max;
      buf[0] = value + 1;
      err = udi_usb_snd_control_msg (devc->mixer_usbdev, 0,	// endpoint
				     SET_CUR, USB_RECIP_INTERFACE | USB_TYPE_CLASS,	// rqtype
				     0,	// value
				     (c->unit->num << 8),	// index
				     buf,	// buffer
				     1,	// buflen
				     OSS_HZ * 2);
      if (err < 0)
	{
	  cmn_err (CE_CONT, "Selector write error %d\n", err);
	  return 0;
	}
      break;

    case TY_FEAT:
      value = write_feature_value (devc, c, value);
      break;

    case TY_MIXER:
      value = write_mixer_value (devc, c, value);
      break;

    default:
      return OSS_EIO;
    }

  return value;
}

static int
new_ctl (ossusb_devc * devc, usb_audio_unit_t * un, int index, int exttype,
	 int chmask)
{
  int n, min, max;
  usb_control_t *c;
  int i, min_ch = 0, max_ch = 0;

  if (devc->ncontrols >= MAX_CONTROLS)
    {
      cmn_err (CE_CONT, "Too many mixer features.\n");
      return OSS_EIO;
    }

  n = devc->ncontrols++;

  c = &devc->controls[n];

  if (chmask)
    {
      min_ch = -1;

      for (i = 0; i < 32; i++)
	if (chmask & (1 << i))
	  {
	    if (min_ch == -1)
	      min_ch = i;
	    max_ch = i;
	  }
    }

  c->unit = un;
  c->index = index;
  c->exttype = exttype;
  c->min_ch = min_ch;
  c->global = (chmask == 0) ? 1 : 0;
  c->max_ch = max_ch;
  c->min = 0;
  c->max = 255;
  c->chmask = chmask;
  c->value = 0;

  if (index != 1)		/* Not volume control */
    {
      c->max = 1;
    }
  else
    {
      if (read_unit (devc, c, GET_MAX, &max))
	{
	  c->max = max;
	  if (read_unit (devc, c, GET_MIN, &min))
	    {
	      c->min = min;
	    }
	}
    }

  if (c->max <= c->min)
    {
/*
 * The device reported bad limits. Try to fix the situation.
 */
      if (c->min < 0)
	c->max = 0;
      else
	c->min = 0;
      if (c->max <= c->min)
	{
	  c->min = 0;
	  c->max = 255;
	}
    }

  c->scale = 255;
  if (c->max - c->min < 255)
    c->scale = c->max - c->min;


  read_current_value (devc, c);

  if (usb_trace)
    {
      cmn_err (CE_CONT, "Ctl %2d: %d/%s %d ", n, c->unit->num, c->unit->name,
	       c->index);
      cmn_err (CE_CONT, "Min %d, max %d, scale %d\n", c->min, c->max,
	       c->scale);
      cmn_err (CE_CONT, "ch=%x ", c->chmask);
      cmn_err (CE_CONT, "Value %04x, (%d - %d, %d)\n", c->value, c->min,
	       c->max, c->scale);
    }

  return n;
}

static int
mixer_func (int dev, int ctrl, unsigned int cmd, int value)
{
  ossusb_devc *devc = mixer_devs[dev]->devc;
  usb_control_t *c;

  if (devc->disabled)
    return OSS_EPIPE;

  if (ctrl < 0 || ctrl >= devc->ncontrols)
    return OSS_EIO;

  c = &devc->controls[ctrl];

  if (cmd == SNDCTL_MIX_READ)
    {
      return c->value;
    }

  if (cmd != SNDCTL_MIX_WRITE)
    return OSS_EINVAL;

  value = c->value = write_current_value (devc, c, value);

  return value;
}

static char *
get_feature_name (int n)
{
  switch (n)
    {
    case 0:
      return "mute";
    case 1:
      return "vol";
    case 2:
      return "bass";
    case 3:
      return "mid";
    case 4:
      return "treble";
    case 5:
      return "eq";
    case 6:
      return "agc";
    case 7:
      return "delay";
    case 8:
      return "boost";
    case 9:
      return "loud";
    case 10:
      return "igain";
    case 11:
      return "igainpad";
    case 12:
      return "phaseinv";
    case 13:
      return "underflow";
    case 14:
      return "overflow";
    default:
      return "misc";
    }
}

static int
get_feature_type (int n)
{
  switch (n)
    {
    case 0:
      return MIXT_ONOFF;
    case 1:
      return MIXT_SLIDER;
    case 2:
      return MIXT_SLIDER;
    case 3:
      return MIXT_SLIDER;
    case 4:
      return MIXT_SLIDER;
    case 5:
      return MIXT_SLIDER;
    case 6:
      return MIXT_ONOFF;
    case 7:
      return MIXT_SLIDER;
    case 8:
      return MIXT_ONOFF;
    case 9:
      return MIXT_ONOFF;
    default:
      return MIXT_ONOFF;
    }
}

struct chmasks
{
  unsigned int mask;
  char *name;
  char type;
  char channels;
};

static const struct chmasks chmasks[] = {
  {0x00000003, "front", MIXT_STEREOSLIDER, 2},
  {0x00000001, "L", MIXT_MONOSLIDER, 1},
  {0x00000002, "R", MIXT_MONOSLIDER, 1},
  {0x00000030, "surr", MIXT_STEREOSLIDER, 2},
  {0x00000010, "LS", MIXT_MONOSLIDER, 1},
  {0x00000020, "RS", MIXT_MONOSLIDER, 1},
  {0x0000000c, "C/L", MIXT_STEREOSLIDER, 2},
  {0x00000004, "C", MIXT_MONOSLIDER, 1},
  {0x00000008, "LFE", MIXT_MONOSLIDER, 1},
  {0x000000c0, "center", MIXT_STEREOSLIDER, 2},
  {0x00000040, "LC", MIXT_MONOSLIDER, 1},
  {0x00000080, "RC", MIXT_MONOSLIDER, 1},
  {0x00000100, "surr", MIXT_STEREOSLIDER, 2},
  {0x00000600, "side", MIXT_STEREOSLIDER, 2},
  {0x00000200, "SL", MIXT_MONOSLIDER, 1},
  {0x00000400, "SR", MIXT_MONOSLIDER, 1},
  {0x00000800, "TC", MIXT_MONOSLIDER, 1},
  {0x00001000, "TFL", MIXT_MONOSLIDER, 1},
  {0x00002000, "TFC", MIXT_MONOSLIDER, 1},
  {0x00004000, "TFR", MIXT_MONOSLIDER, 1},
  {0x00008000, "TBL", MIXT_MONOSLIDER, 1},
  {0x00010000, "TBC", MIXT_MONOSLIDER, 1},
  {0x00020000, "TBR", MIXT_MONOSLIDER, 1},
  {0x00040000, "TFLC", MIXT_MONOSLIDER, 1},
  {0x00080000, "TFRC", MIXT_MONOSLIDER, 1},
  {0x00100000, "LLFE", MIXT_MONOSLIDER, 1},
  {0x00200000, "RLFE", MIXT_MONOSLIDER, 1},
  {0x00400000, "TSL", MIXT_MONOSLIDER, 1},
  {0x00800000, "TSR", MIXT_MONOSLIDER, 1},
  {0x01000000, "BC", MIXT_MONOSLIDER, 1},
  {0x02000000, "BLC", MIXT_MONOSLIDER, 1},
  {0x04000000, "BRC", MIXT_MONOSLIDER, 1},
  {0x80000000, "RD", MIXT_MONOSLIDER, 1},
  {0, NULL}
};

static int
count_source_controls (ossusb_devc * devc, int unit)
{
  int n = 0, nn, i;
  usb_audio_unit_t *un;
  unsigned char *d;

  if (unit <= 0 || unit >= devc->nunits)
    return 0;

  un = &devc->units[unit];
  d = un->desc;

  if (un == NULL)
    return 0;

  switch (un->typ)
    {
    case TY_MIXER:
    case TY_SELECT:
      nn = d[4];
      d += 5;
      for (i = 0; i < nn; i++)
	{
	  n += count_source_controls (devc, *d);
	  d++;
	}
      break;

    case TY_PROC:
    case TY_EXT:
      nn = d[6];
      d += 7;
      for (i = 0; i < nn; i++)
	{
	  n += count_source_controls (devc, *d);
	  d++;
	}
      break;

    default:
      if (un->source <= 0 && un->source < devc->nunits)
	{
	  n = count_source_controls (devc, un->source);
	}
    }

  if (!un->ctl_avail)
    return n;

  return n + un->control_count;
}

static int
count_target_controls (ossusb_devc * devc, int unit)
{
  int n = 0;
  usb_audio_unit_t *un;

  if (unit <= 0 || unit >= devc->nunits)
    return 0;

  un = &devc->units[unit];

  if (un == NULL)
    return 0;

  if (un->typ == TY_SELECT || un->typ == TY_MIXER)
    {
      n = 0;
    }
  else if (un->target <= 0 && un->target < devc->nunits)
    {
      n = count_target_controls (devc, un->target);
    }

  if (!un->ctl_avail)
    return n;

  return n + un->control_count;
}

static int
follow_source_links (ossusb_devc * devc, usb_audio_unit_t * un)
{
  while (un->source > 0 && un->source < devc->nunits)
    un = &devc->units[un->source];

  return un->num;
}

static int
follow_target_links (ossusb_devc * devc, usb_audio_unit_t * un)
{
  while (un->target > 0 && un->target < devc->nunits)
    un = &devc->units[un->target];

  return un->num;
}

static void
add_controls_for_mixer (ossusb_devc * devc, usb_audio_unit_t * un, int group)
{
  int i, n;
  unsigned char *d = un->desc;

  if (!un->ctl_avail)
    return;

  n = d[4];
  d += 5;

  for (i = 0; i < n; i++)
    if (*d > 0 && *d < devc->nunits)
      {
	int ref = follow_source_links (devc, &devc->units[*d]);

	{

	  int ctl = new_ctl (devc, un, i, MIXT_MONOSLIDER, un->chmask);
	  UDB (cmn_err
	       (CE_CONT, "Add mixer control %d/%s\n", un->num,
		devc->units[ref].name));
	  if (mixer_ext_create_control (devc->mixer_dev, group, ctl,
					mixer_func, MIXT_MONOSLIDER,
					devc->units[ref].name, 255,
					MIXF_READABLE | MIXF_WRITEABLE) < 0)
	    return;
	}
	d++;
      }

  un->ctl_avail = 0;
}

/*ARGSUSED*/
static void
add_controls_for_proc (ossusb_devc * devc, usb_audio_unit_t * un, int group)
{
  int i, n;
  unsigned char *d = un->desc;

  if (!un->ctl_avail)
    return;

  n = d[6];
  d += 7;

  for (i = 0; i < n; i++)
    if (*d > 0 && *d < devc->nunits)
      {
	d++;
      }

  un->ctl_avail = 0;
}

static void
add_controls_for_selector (ossusb_devc * devc, usb_audio_unit_t * un,
			   int group)
{
  static oss_mixer_enuminfo ent;
  char *s;
  int i, n, err;
  unsigned char *d = un->desc;
  int ctl = new_ctl (devc, un, 0, MIXT_ENUM, 0);

  if (!un->ctl_avail)
    return;

  n = d[4];
  d += 5;
  UDB (cmn_err (CE_CONT, "Add selector control %d/%s\n", un->num, un->name));
  if ((err = mixer_ext_create_control (devc->mixer_dev, group,
				       ctl, mixer_func,
				       MIXT_ENUM,
				       un->name, n,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return;

  memset (&ent, 0, sizeof (ent));

  s = ent.strings;

  for (i = 0; i < n; i++)
    if (*d > 0 && *d < devc->nunits)
      {
	int ref = follow_source_links (devc, &devc->units[*d]);
	d++;

	if (s > ent.strings)
	  *s++ = '|';
	strcpy (s, devc->units[ref].name);
	s += strlen (s);
      }

  s = ent.strings;
  for (i = 0; i < strlen (s); i++)
    {
      if (s[i] == ' ')
	s[i] = '_';
      if (s[i] == '|')
	s[i] = ' ';
    }

  ent.dev = devc->mixer_dev;
  ent.ctrl = err;
  ent.nvalues = 0;

  mixer_ext_set_enum (&ent);

  un->ctl_avail = 0;
}

/*ARGSUSED*/
static void
add_multich_volumes (ossusb_devc * devc, usb_audio_unit_t * un, int group,
		     int fea, int mask, unsigned int *feature_mask)
{
  int i;

  for (i = 0; mask != 0 && chmasks[i].mask != 0; i++)
    {
      int m = chmasks[i].mask;
      int ctl;
      usb_control_t *c;

      if (!(mask & m))
	continue;

      ctl = new_ctl (devc, un, fea, chmasks[i].type, chmasks[i].mask);
      c = &devc->controls[ctl];

      UDB (cmn_err
	   (CE_CONT, "Add multich feature control %d/%s\n", un->num,
	    chmasks[i].name));
      if (mixer_ext_create_control (devc->mixer_dev, group, ctl, mixer_func,
				    chmasks[i].type, chmasks[i].name,
				    c->scale,
				    MIXF_READABLE | MIXF_WRITEABLE) < 0)
	return;

      mask &= ~m;
    }

  if (mask)
    cmn_err (CE_CONT, "Warning! Unsupported channels (%02x)\n", mask);
}

static void
translate_feature_mask_usb2 (ossusb_devc *devc, usb_audio_unit_t * un, unsigned int *feature_mask)
{
  int i, n, c;
  unsigned char *d = un->desc;

  n = 4;
  d = d + 5 + n;		// Skip the global mask

/*
 * USB 2.0 uses 2 bits for each control
 * 	01b means that the control is read only and 11b means it's RW
 */
  for (c = 0; c < un->channels; c++)
    {
      unsigned int mask;
      unsigned char *p = d + c * n;

      mask = p[3] |
	     (p[2] << 8) |
	     (p[1] << 16) |
	     (p[0] << 24);

      for (i = 0; i < n * 4; i++)
	if ((mask & (3 << 2*i)) && (un->ctl_avail & (1 << i)))
	  {
	    feature_mask[i] |= 1 << c;
	  }
    }
}

static void
translate_feature_mask (ossusb_devc *devc, usb_audio_unit_t * un, unsigned int *feature_mask)
{
  int i, n, c;
  unsigned char *d = un->desc;

  if (devc->usb_version > 1)
  {
	translate_feature_mask_usb2(devc, un, feature_mask);
	return;
  }

  n = d[5];
  if (n < 1 || n > 2)
    {
      	cmn_err (CE_CONT, "Bad feature mask size %d\n", n);
       	return;
    }

    d = d + 6 + n;		// Skip the global mask

  for (c = 0; c < un->channels; c++)
    {
      int mask = d[c * n];
      if (n > 1)
	mask |= d[c * n + 1];

      for (i = 0; i < n * 8; i++)
	if ((mask & (1 << i)) && (un->ctl_avail & (1 << i)))
	  {
	    feature_mask[i] |= 1 << c;
	  }
    }
}

static void
add_controls_for_feature (ossusb_devc * devc, usb_audio_unit_t * un,
			  int group)
{
  int i;
  unsigned int feature_mask[16], global_mask;

  if (!un->ctl_avail)
    return;

// Handle the global controls first

  global_mask = un->desc[6];
  if (un->desc[5] > 0)
    global_mask |= un->desc[7] << 8;

  for (i = 0; i < 11; i++)
    if (un->ctl_avail & (1 << i) && (global_mask & (1 << i)))
      {
	char *name = get_feature_name (i);
	int type = get_feature_type (i);
	int max;
	char tmp[16];
	int ctl;

	ctl = new_ctl (devc, un, i, type, 0);
	strcpy (tmp, name);
	if (type == MIXT_SLIDER)
	  max = devc->controls[ctl].max - devc->controls[ctl].min;
	else
	  max = 1;

	if (max > 255)
	  max = 255;

	UDB (cmn_err
	     (CE_CONT, "Add (global) feature control %d:%s, max=%d\n",
	      un->num, name, max));
	if (mixer_ext_create_control (devc->mixer_dev, group, ctl,
				      mixer_func, type, tmp, max,
				      MIXF_READABLE | MIXF_WRITEABLE) < 0)
	  return;
	//un->ctl_avail &= ~(1 << i);
      }

// Handle the channelized controls

  if (un->ctl_avail == 0)	/* Nothing left */
    return;

  // Translate the channel/feature availability matrix

  memset (feature_mask, 0, sizeof (feature_mask));
  translate_feature_mask (devc, un, feature_mask);

  for (i = 0; i < 16; i++)
    if (feature_mask[i])
      {
	char *name = get_feature_name (i);
	int type = get_feature_type (i);
	int max, instances = 0;

	int nc = 0, j;

	for (j = 0; j < 16; j++)
	  if (feature_mask[i] & (1 << j))
	    nc = j + 1;

	if (type == MIXT_SLIDER)
	  {
	    if (nc == 1)
	      {
		type = MIXT_MONOSLIDER;
		instances = 1;
	      }
	    else if (nc == 2)
	      {
		type = MIXT_STEREOSLIDER;
		instances = 1;
	      }
	    else
	      {
		type = MIXT_MONOSLIDER;
		if (i == 1)	/* "volume" */
		  instances = nc;
		else
		  {
		    instances = 0;
		    type = MIXT_MONOSLIDER;
		  }
	      }
	    max = 255;
	  }
	else
	  max = 2;

	if (instances && (instances > 1 || feature_mask[i] > 0x0003))
	  {
	    int g;
	    char tmp[16];
	    strcpy (tmp, name);
	    UDB (cmn_err (CE_CONT, "Add feature group %s\n", tmp));
	    if ((g =
		 mixer_ext_create_group (devc->mixer_dev, group, tmp)) < 0)
	      return;

	    add_multich_volumes (devc, un, g, i, feature_mask[i],
				 feature_mask);
	  }
	else
	  {
	    char tmp[16];
	    int ctl = new_ctl (devc, un, i, type, un->chmask);
	    strcpy (tmp, name);
	    max = devc->controls[ctl].max - devc->controls[ctl].min;
	    if (max > 255)
	      max = 255;

	    UDB (cmn_err
		 (CE_CONT, "Add feature control %d:%s\n", un->num, name));
	    if (mixer_ext_create_control (devc->mixer_dev, group, ctl,
					  mixer_func, type, tmp, max,
					  MIXF_READABLE | MIXF_WRITEABLE) < 0)
	      return;
	  }
      }

  un->ctl_avail = 0;		// All done (hope so)
}

static void
traverse_source_controls (ossusb_devc * devc, usb_audio_unit_t * un,
			  int group)
{
  unsigned char *d;

  d = un->desc;

  if (un->typ == TY_MIXER)
    {
      add_controls_for_mixer (devc, un, group);
      return;
    }

  if (un->typ == TY_PROC)
    {
      add_controls_for_proc (devc, un, group);

      if (d[6] > 1)		/* More than 1 sources */
	return;
    }

  if (un->typ == TY_SELECT)
    {
      add_controls_for_selector (devc, un, group);
      return;
    }

  if (un->source > 0 && un->source < devc->nunits)
    {
      traverse_source_controls (devc, &devc->units[un->source], group);
    }

  if (un->typ == TY_FEAT)
    {
      add_controls_for_feature (devc, un, group);
    }
}

static void
traverse_target_controls (ossusb_devc * devc, usb_audio_unit_t * un,
			  int group)
{
  unsigned char *d;


  d = un->desc;

  if (un->typ == TY_SELECT)
    {
      add_controls_for_selector (devc, un, group);
    }

  if (un->typ == TY_PROC || un->typ == TY_EXT)
    if (d[6] > 1)		// More than 1 input pins
      return;

  if (un->typ == TY_MIXER)
    {
#if 1
      add_controls_for_mixer (devc, un, group);
#endif
    }

  if (un->target > 0 && un->target < devc->nunits)
    traverse_target_controls (devc, &devc->units[un->target], group);

  if (un->typ == TY_FEAT)
    {
      add_controls_for_feature (devc, un, group);
    }
}

void
ossusb_create_altset_control (int dev, int portc_num, int nalt, char *name)
{
  int ctl;
  ossusb_devc *devc = mixer_devs[dev]->devc;
  char *as = NULL;
  int default_altsetting;
  unsigned int altsetting_mask;

  if (nalt < 3)			/* Only one alternative setting (plus "OFF") available */
    return;
  if ((as =
       udi_usbdev_get_altsetting_labels (devc->mixer_usbdev, portc_num,
					 &default_altsetting,
					 &altsetting_mask)) != NULL)
    {
      if (altsetting_mask != 0)
	{
	  /*
	   * Check if more than one of them are enabled.
	   */

	  int i, n = 0;

	  for (i = 1; i < nalt; i++)
	    if (altsetting_mask & (1 << i))
	      n++;

	  if (n < 2)		/* No */
	    return;
	}
    }

  if ((ctl = mixer_ext_create_control (dev, 0,
				       portc_num, ossusb_change_altsetting,
				       MIXT_ENUM,
				       name, nalt,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return;

  if (as != NULL)		/* Use custom labels */
    {
      mixer_ext_set_strings (dev, ctl, as, 0);
      if (altsetting_mask != 0)
	{
	  oss_mixext *ext;
	  int i;

	  ext = mixer_find_ext (dev, ctl);

	  memset (ext->enum_present, 0, sizeof (ext->enum_present));
	  for (i = 0; i < nalt; i++)
	    if (altsetting_mask & (1 << i))
	      ext->enum_present[i / 8] |= (1 << (i % 8));
	}

      if (default_altsetting > 0)
	ossusb_change_altsetting (dev, portc_num, SNDCTL_MIX_WRITE,
				  default_altsetting);
    }
  else
    mixer_ext_set_strings (dev, ctl,
			   "OFF 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19",
			   0);

}

static int
usb_mix_init (int dev)
{
  ossusb_devc *devc;
  int i, group;

  devc = mixer_devs[dev]->devc;

  for (i = 0; i < devc->nunits; i++)
    {
      usb_audio_unit_t *un = &devc->units[i];

      if (un->typ == TY_OUTPUT)
	{
	  if (count_source_controls (devc, un->source) == 0)
	    {
	      continue;
	    }

	  if ((group = mixer_ext_create_group (dev, 0, un->name)) < 0)
	    return group;

	  traverse_source_controls (devc, un, group);
	  continue;
	}
    }

  for (i = 0; i < devc->nunits; i++)
    {
      usb_audio_unit_t *un = &devc->units[i];

      if (un->typ == TY_INPUT)
	{
	  if (count_target_controls (devc, un->target) == 0)
	    {
	      continue;
	    }

	  if ((group = mixer_ext_create_group (dev, 0, un->name)) < 0)
	    return group;

	  traverse_target_controls (devc, un, group);
	  un->ctl_avail = 0;
	  continue;
	}
    }

  return 0;
}

static char *
check_feature (usb_audio_unit_t * un, unsigned char *mask, int n)
{
  if (mask[n / 8] & (1 << (n % 8)))
    {
      un->control_count++;

      return get_feature_name (n);
    }

  return NULL;
}

/*ARGSUSED*/
static int
usb_mixer_ioctl (int dev, int audiodev, unsigned int cmd, ioctl_arg arg)
{
  ossusb_devc *devc = mixer_devs[dev]->hw_devc;

  if (devc->disabled)
    return OSS_EPIPE;

  switch (cmd)
    {
    case SOUND_MIXER_READ_DEVMASK:
      return *arg = devc->devmask | devc->recmask;
    case SOUND_MIXER_READ_RECMASK:
      return *arg = devc->recmask;
    case SOUND_MIXER_READ_RECSRC:
      return *arg = devc->recsrc;
    case SOUND_MIXER_READ_STEREODEVS:
      return *arg = devc->stereodevs;
    }

  return *arg = 0;
}

static mixer_driver_t usb_mixer_driver = {
  usb_mixer_ioctl
};

static int
init_mixer (ossusb_devc * devc)
{
  if ((devc->mixer_dev = oss_install_mixer (OSS_MIXER_DRIVER_VERSION,
					    devc->osdev,
					    devc->osdev,
					    devc->dev_name,
					    &usb_mixer_driver,
					    sizeof (mixer_driver_t),
					    devc)) >= 0)
    {
      char mxname[20];

      sprintf (mxname, "USB_%d", ndevs);

      devc->levels = load_mixer_volumes (mxname, NULL, 1);
      mixer_devs[devc->mixer_dev]->hw_devc = devc;
      mixer_ext_set_init_fn (devc->mixer_dev, usb_mix_init, MAX_CONTROLS);
    }

  return 1;
}

static usb_audio_unit_t *
setup_unit (ossusb_devc * devc, int unit, char *name, unsigned char *desc,
	    int desclen, int typ)
{
  usb_audio_unit_t *un;

  if (unit < 0 || unit >= MAX_UNIT)
    {
      cmn_err (CE_WARN, "Bad unit number %d\n", unit);
      return NULL;
    }

  if (typ >= sizeof (devc->unit_counters))
    {
      cmn_err (CE_WARN, "Bad unit type %d\n", typ);
      return NULL;
    }

  devc->unit_counters[typ]++;

  if (unit >= devc->nunits)
    devc->nunits = unit + 1;

  un = &devc->units[unit];

  un->num = unit;
  un->typ = typ;
  un->desc = desc;
  un->desclen = desclen;
  un->mixnum = -1;
  un->channels = 2;
  un->chmask = 0x03;
  strcpy (un->name, name);

  return un;
}

static char *
get_terminal_id (int type, char *def, int typ, int *mn)
{
  int dummy, *mixnum;

  if (mn != NULL)
    mixnum = mn;
  else
    mixnum = &dummy;
  switch (type)
    {
    case 0x0101:
      if (typ == TY_INPUT)
	{
	  *mixnum = SOUND_MIXER_PCM;
	  return "play";
	}
      if (typ == TY_OUTPUT)
	{
	  *mixnum = SOUND_MIXER_RECLEV;
	  return "rec";
	}
      break;
    case 0x0301:
      *mixnum = SOUND_MIXER_VOLUME;
      return "output";
    case 0x0302:
      *mixnum = SOUND_MIXER_VOLUME;
      return "headph";
    case 0x0307:
      return "LFE";

    case 0x0401:
      return "handset";
    case 0x0402:
      return "headset";

    case 0x0403:
    case 0x0404:
    case 0x0405:
    case 0x0500:
    case 0x0501:
    case 0x0502:
    case 0x0504:
      return "phone";
      break;

    case 0x0600:
      *mixnum = SOUND_MIXER_LINE1;
      return "aux";
    case 0x0601:
      *mixnum = SOUND_MIXER_LINE2;
      return "aux";
    case 0x0602:
      return "digital";
    case 0x0603:
      *mixnum = SOUND_MIXER_LINE;
      return "line";
    case 0x0604:
      *mixnum = SOUND_MIXER_SPEAKER;
      return "pc_in";
    case 0x0605:
      *mixnum = SOUND_MIXER_DIGITAL1;
      if (typ == TY_INPUT)
	return "spdin";
      else
	return "spdout";
    case 0x0606:
      return "da_stream";
    case 0x0607:
      return "DV_audio";

    case 0x0701:
      return "noise";
    case 0x0702:
      return "eq_noise";
    case 0x0703:
      *mixnum = SOUND_MIXER_CD;
      return "cd";
    case 0x0704:
      return "dat";
    case 0x0706:
      return "md";
    case 0x0707:
      return "tape";
    case 0x0708:
      return "phono";
    case 0x0709:
      *mixnum = SOUND_MIXER_VIDEO;
      return "vcr";
    case 0x070b:
      return "dvd";
    case 0x070c:
      return "tv";
    case 0x070d:
      return "sat";
    case 0x070e:
      return "cable";
    case 0x0710:
      *mixnum = SOUND_MIXER_RADIO;
      return "radio";
    case 0x0711:
      *mixnum = SOUND_MIXER_RADIO;
      return "xmitter";
    case 0x0712:
      return "mtrack";
    case 0x0713:
      *mixnum = SOUND_MIXER_SYNTH;
      return "synth";
    }

  if (type < 0x201)		// Unknown type
    return def;

  if (type < 0x300)
    {
      *mixnum = SOUND_MIXER_MIC;
      return "mic";
    }

  return def;
}

static void
setup_legacy_mixer (ossusb_devc * devc)
{
  int x;

// Set up the recording selector

  for (x = 1; x < devc->nunits; x++)
    {
      usb_audio_unit_t *un = &devc->units[x];
      int i, n;
      unsigned char *d = un->desc;

      if (un->typ != TY_SELECT || strcmp (un->name, "rec.src") != 0)
	continue;

      if (!un->ctl_avail)
	continue;

      devc->rec_unit = x;
      un->ctl_avail = 0;

      devc->num_recdevs = n = d[4];
      d += 5;
      for (i = 0; i < n; i++)
	{
	  int ref = follow_source_links (devc, &devc->units[*d]);
	  int mask;

	  d++;

	  if (ref < 1 || ref >= devc->nunits || devc->units[ref].mixnum == -1)
	    continue;

	  mask = (1 << devc->units[ref].mixnum);
	  if (devc->recmask & mask)	// Duplicate
	    continue;
	  UDB (cmn_err
	       (CE_CONT, "Recsrc %d, num=%d, mask %x\n", i, ref, mask));

	  devc->recdevs[i] = mask;

	  devc->recmask |= mask;
	  if (devc->recsrc == 0)
	    devc->recsrc = mask;
	}

      break;
    }

// Set up the legacy mixer controls

  for (x = 1; x < devc->nunits; x++)
    {
      usb_audio_unit_t *un = &devc->units[x];
      int mixnum;
      unsigned char *d = un->desc;

      if (!(un->ctl_avail & 0x02))
	continue;

      if (un->typ == TY_FEAT && d[6] & 0x02)	// Volume control
	{
	  int t;
	  mixnum = un->mixnum;

	  if (mixnum == -1)
	    {
	      t = follow_target_links (devc, un);
	      if (t && devc->units[t].mixnum != -1)
		mixnum = devc->units[t].mixnum;
	      else
		{
		  t = follow_source_links (devc, un);
		  if (t && devc->units[t].mixnum != -1)
		    mixnum = devc->units[t].mixnum;
		}
	    }

	  if (mixnum == -1 || un->channels > 2)
	    continue;
	  UDB (cmn_err (CE_CONT, "Unit %d is volume %d\n", x, mixnum));
	  un->ctl_avail &= ~0x02;	// Reserve the volume slider

	  devc->devmask |= (1 << mixnum);
	  if (un->channels == 2)
	    devc->stereodevs |= (1 << mixnum);

	  continue;
	}
    }

}

static char *
get_processor_type (int t)
{
  switch (t)
    {
    case 0x01:
      return "upmix";
    case 0x02:
      return "prologic";
    case 0x03:
      return "3D";
    case 0x04:
      return "reverb";
    case 0x05:
      return "chorus";
    case 0x06:
      return "compress";
    default:
      return "proc";
    }
}

static void
parse_processing_unit (ossusb_devc * devc, unsigned char *d, int l)
{
  usb_audio_unit_t *un;
  char *name;

  name = get_processor_type (d[4]);

  if ((un = setup_unit (devc, d[3], name, d, l, TY_PROC)) == NULL)
    return;
  un->subtyp = d[4];
  un->ctl_avail = 1;

  if (un->subtyp == 1)		// Upmix/downmix unit
    {
      un->channels = d[8];
      un->chmask = ossusb_get_int (&d[9], 2);
    }
}

static int
get_feature_mask (unsigned char *d, int channels)
{
  int i, n, mask = 0, v;
  n = d[5];

  for (i = 0; i <= channels; i++)
    {
      v = d[6 + i * n];
      if (n > 1)
	v |= d[6 + i * n + 1] << 8;

      mask |= v;
    }

  return mask;
}

#if 1
static void
mixer_dump (ossusb_devc * devc)
{
  int i, j, c;
  usb_audio_unit_t *un;

  for (i = 1; i < devc->nunits; i++)
    {
      un = &devc->units[i];

      if (un->typ == TY_FEAT)
	{
	  cmn_err (CE_CONT, "Unit %d\n", un->num);

	  for (j = 0; j < 8; j++)
	    {
	      for (c = 0; c < 7; c++)
		{
		  unsigned char buf[2];
		  int len;

		  buf[0] = buf[1] = 0;
		  len = udi_usb_rcv_control_msg (devc->mixer_usbdev, 0,	// endpoint
						 GET_CUR, USB_RECIP_INTERFACE | USB_TYPE_CLASS,	// rqtype
						 ((j) << 8) | (c),	// value
						 (un->num << 8),	// index
						 buf,	// buffer
						 2,	// buflen
						 OSS_HZ * 2);

		  if (len < 0)
		    continue;
		  cmn_err (CE_CONT, "  Feature %d/%d, ch %d: ", un->num, j,
			   c);
		  cmn_err (CE_CONT, "%02x%02x ", buf[0], buf[1]);

		  if (len == 1)
		    cmn_err (CE_CONT, "(%d) ", (signed char) buf[0]);
		  else
		    cmn_err (CE_CONT, "(%d) ",
			     (signed short) (buf[0] | (buf[1] << 8)));

		  buf[0] = buf[1] = 0;
		  len = udi_usb_rcv_control_msg (devc->mixer_usbdev, 0,	// endpoint
						 GET_MIN, USB_RECIP_INTERFACE | USB_TYPE_CLASS,	// rqtype
						 ((j) << 8) | (c),	// value
						 (un->num << 8),	// index
						 buf,	// buffer
						 2,	// buflen
						 OSS_HZ * 2);

		  if (len >= 0)
		    cmn_err (CE_CONT, "Min=%02x%02x ", buf[0], buf[1]);
		  if (len == 1)
		    cmn_err (CE_CONT, "(%d) ", (signed char) buf[0]);
		  else if (len == 2)
		    cmn_err (CE_CONT, "(%d) ",
			     (signed short) (buf[0] | (buf[1] << 8)));

		  buf[0] = buf[1] = 0;
		  len = udi_usb_rcv_control_msg (devc->mixer_usbdev, 0,	// endpoint
						 GET_MAX, USB_RECIP_INTERFACE | USB_TYPE_CLASS,	// rqtype
						 ((j) << 8) | (c),	// value
						 (un->num << 8),	// index
						 buf,	// buffer
						 2,	// buflen
						 OSS_HZ * 2);

		  if (len >= 0)
		    cmn_err (CE_CONT, "max=%02x%02x ", buf[0], buf[1]);
		  if (len == 1)
		    cmn_err (CE_CONT, "(%d) ", (signed char) buf[0]);
		  else if (len == 2)
		    cmn_err (CE_CONT, "(%d) ",
			     (signed short) (buf[0] | (buf[1] << 8)));

		  cmn_err (CE_CONT, "\n");
		}
	    }
	}
    }
}
#endif

/*ARGSUSED*/
static ossusb_devc *
ossusb_init_audioctl (ossusb_devc * devc, udi_usb_devc * usbdev, int inum,
		      int reinit)
{
  int desc_len;
  unsigned char *desc, *d;
  int i, l, n = 0, p, x, mixnum = -1;
  char *name;
  usb_audio_unit_t *un;

  /* nsettings = udi_usbdev_get_num_altsettings (usbdev); */
  devc->mixer_usbdev = usbdev;

  if (!init_mixer (devc))
    return NULL;

  desc = udi_usbdev_get_altsetting (usbdev, 0, &desc_len);

  if (desc == NULL)
    {
      cmn_err (CE_CONT, "Can't read interface descriptors\n");
      return NULL;
    }

  p = 0;
  while (p < desc_len)
    {
      d = desc + p;

      l = *d;
      if (usb_trace > 1)
	{
	  char str[256], *s = str;
	  sprintf (s, "Control desc(%d): ", p);
	  s += strlen (s);
	  for (i = 0; i < l; i++)
	    {
	      sprintf (s, "%02x ", d[i]);
	      s += strlen (s);
	    }
	  cmn_err (CE_CONT, "%s\n", str);
	}

#define CASE(x) case x:cmn_err(CE_CONT, #x "\n"); break;

      if (d[1] == CS_INTERFACE)
	switch (d[2])
	  {
	  case AC_HEADER:
	    if (usb_trace)
	      {
		cmn_err (CE_CONT, "	Audio control interface header\n");
		n = d[7];
		cmn_err (CE_CONT, "	%d related streaming interfaces: ",
			 n);
		for (i = 0; i < n; i++)
		  {
		    cmn_err (CE_CONT, "%d ", d[8 + i]);
		  }
		cmn_err (CE_CONT, "\n");
	      }
	    break;

	  case AC_INPUT_TERMINAL:
	    name =
	      get_terminal_id (ossusb_get_int (&d[4], 2), "in", TY_INPUT,
			       &mixnum);
	    if ((un = setup_unit (devc, d[3], name, d, l, TY_INPUT)) == NULL)
	      return NULL;
	    un->mixnum = mixnum;
	    un->channels = d[7];
	    un->chmask = ossusb_get_int (&d[8], 2);
	    break;

	  case AC_OUTPUT_TERMINAL:
	    name =
	      get_terminal_id (ossusb_get_int (&d[4], 2), "out", TY_OUTPUT,
			       &mixnum);
	    if ((un = setup_unit (devc, d[3], name, d, l, TY_OUTPUT)) == NULL)
	      return NULL;
	    un->mixnum = mixnum;
	    break;

	  case AC_MIXER_UNIT:
	    if ((un = setup_unit (devc, d[3], "mix", d, l, TY_MIXER)) == NULL)
	      return NULL;
	    {
	      // Check if there are any visible controls

	      int mask = 0, nn;

	      n = d[4];		// # of input pins
	      d += 5 + n;
	      nn = *d;		// # of channels
	      d += 4;		// Seek to bmControls

	      n = (n * nn + 7) / 8;

	      for (i = 0; i < n; i++)
		mask |= d[i];

	      un->ctl_avail = mask;
	    }
	    break;

	  case AC_SELECTOR_UNIT:
	    if ((un =
		 setup_unit (devc, d[3], "src", d, l, TY_SELECT)) == NULL)
	      return NULL;
	    un->ctl_avail = 1;
	    break;

	  case AC_FEATURE_UNIT:
	    if ((un = setup_unit (devc, d[3], "fea", d, l, TY_FEAT)) == NULL)
	      return NULL;
	    un->ctl_avail = get_feature_mask (d, 2);	/* For now */
	    break;

	  case AC_PROCESSING_UNIT:
	    parse_processing_unit (devc, d, l);
	    break;

	  case AC_EXTENSION_UNIT:
	    if ((un = setup_unit (devc, d[3], "ext", d, l, TY_EXT)) == NULL)
	      return NULL;
	    un->ctl_avail = 1;

	    break;

	  }

      p += l;
    }

// Make sure the unit names are unique */

  for (x = 1; x < devc->nunits; x++)
    {
      usb_audio_unit_t *un = &devc->units[x];
      int n = 0;

      if (un->typ != TY_SELECT)
	for (i = x + 1; i < devc->nunits; i++)
	  if (strcmp (devc->units[i].name, un->name) == 0)
	    n++;

      if (n > 0)
	{
	  char tmpname[16];
	  strcpy (tmpname, un->name);
	  n = 1;

	  for (i = x; i < devc->nunits; i++)
	    if (strcmp (devc->units[i].name, tmpname) == 0)
	      {
		if (n > 1)
		  sprintf (devc->units[i].name, "%s%d", tmpname, n);
		n++;
	      }
	}

// Make sure the mixer control numbers are unique too

      n = 0;
      if (un->mixnum != -1)
	for (i = x + 1; i < devc->nunits; i++)
	  if (devc->units[i].mixnum == un->mixnum)
	    n++;

      if (n > 0)
	for (i = x + 1; i < devc->nunits; i++)
	  if (devc->units[i].mixnum == un->mixnum)
	    {
	      usb_audio_unit_t *uu = &devc->units[i];

	      switch (uu->mixnum)
		{
		case SOUND_MIXER_PCM:
		  uu->mixnum = SOUND_MIXER_ALTPCM;
		  break;
		case SOUND_MIXER_LINE:
		  uu->mixnum = SOUND_MIXER_LINE1;
		  break;
		case SOUND_MIXER_LINE1:
		  uu->mixnum = SOUND_MIXER_LINE2;
		  break;
		case SOUND_MIXER_LINE2:
		  uu->mixnum = SOUND_MIXER_LINE3;
		  break;
		case SOUND_MIXER_DIGITAL1:
		  uu->mixnum = SOUND_MIXER_DIGITAL2;
		  break;
		case SOUND_MIXER_DIGITAL2:
		  uu->mixnum = SOUND_MIXER_DIGITAL3;
		  break;
		default:
		  uu->mixnum = -1;
		}
	    }

    }

// Handle output selector names


  for (x = 1; x < devc->nunits; x++)
    {
      usb_audio_unit_t *un = &devc->units[x];
      int n;

      if (un->typ != TY_OUTPUT)
	continue;

      n = un->desc[7];		// Source ID
      if (n < 0 || n >= devc->nunits)
	continue;

      if (devc->units[n].target == 0)
	devc->units[n].target = x;

      if (devc->units[n].typ == TY_SELECT && usb_mixerstyle == 0)
	sprintf (devc->units[n].name, "%s.src", un->name);
    }

// Find out the sources
  for (x = 1; x < devc->nunits; x++)
    {
      usb_audio_unit_t *un = &devc->units[x];

      d = un->desc;
      l = un->desclen;

      switch (un->typ)
	{
	case TY_INPUT:
	  break;

	case TY_OUTPUT:
	  un->source = d[7];
	  break;

	case TY_SELECT:
	  un->source = d[5];
	  n = d[4];
	  for (i = 0; i < n; i++)
	    if (d[i + 5] > 0 && d[i + 5] <= devc->nunits)
	      if (devc->units[d[i + 5]].target == 0)
		devc->units[d[i + 5]].target = x;
	  break;

	case TY_MIXER:
	  n = d[4];
	  un->control_count = n;
	  un->source = d[5];
	  for (i = 0; i < n; i++)
	    if (d[i + 5] > 0 && d[i + 5] <= devc->nunits)
	      if (devc->units[d[i + 5]].target == 0)
		devc->units[d[i + 5]].target = x;

	  d += n + 5;
	  un->channels = *d++;
	  un->chmask = ossusb_get_int (d, 2);
	  break;

	case TY_FEAT:
	  un->source = d[4];
	  if (d[4] > 0 && d[4] <= devc->nunits)
	    if (devc->units[d[4]].target == 0)
	      devc->units[d[4]].target = x;
	  break;

	  //case TY_EXT: 
	case TY_PROC:
	  n = d[6];
	  un->source = d[7];
	  for (i = 0; i < n; i++)
	    if (d[i + 7] > 0 && d[i + 7] <= devc->nunits)
	      if (devc->units[d[i + 7]].target == 0)
		devc->units[d[i + 7]].target = x;
	  break;
	}
    }

// Trace the channel config

  for (x = 1; x < devc->nunits; x++)
    {
      usb_audio_unit_t *un = &devc->units[x], *uu;
      int ref;

      if (un->typ == TY_INPUT || un->typ == TY_MIXER)
	continue;

      if (un->typ == TY_PROC && un->subtyp == 1)	// Upmix/downmix unit
	continue;

      ref = follow_source_links (devc, un);
      uu = &devc->units[ref];

      un->channels = uu->channels;
      un->chmask = uu->chmask;
    }

// Handle feature channels
  for (x = 1; x < devc->nunits; x++)
    {
      usb_audio_unit_t *un = &devc->units[x];

      if (un->num == 0 || un->typ != TY_FEAT)
	continue;

      d = un->desc;
      l = un->desclen;

      un->ctl_avail = get_feature_mask (d, un->channels);
    }

// Final checks
  for (x = 1; x < devc->nunits; x++)
    {
      usb_audio_unit_t *un = &devc->units[x];
      int j;

      if (un->num == 0)
	{
	  //cmn_err(CE_CONT, "Skipped undefined control %d\n", x);
	  continue;
	}
      d = un->desc;
      l = un->desclen;

      switch (un->typ)
	{
	case TY_SELECT:
	  n = d[4];
	  un->control_count = n;
	  d += 5;
	  break;

	case TY_MIXER:
	  n = d[4];
	  un->control_count = n;
	  d += 5;
	  break;

	case TY_FEAT:
	  n = d[5];
	  d += 6;
	  for (i = 0; i < n * 8; i++)
	    name = check_feature (un, d, i);
	  for (j = 1; j <= un->channels; j++)
	    {
	      for (i = 0; i < n * 8; i++)
		name = check_feature (un, d + j * n, i);
	    }
	  break;

	}
    }

#if 0
// Debugging
  if (usb_trace)
    for (x = 1; x < devc->nunits; x++)
      {
	usb_audio_unit_t *un = &devc->units[x];
	int j;
	int ref = 0;

	if (un->num == 0)
	  {
	    //cmn_err(CE_CONT, "Skipped undefined control %d\n", x);
	    continue;
	  }
	d = un->desc;
	l = un->desclen;
	// ossusb_dump_desc (d, l);
	cmn_err (CE_CONT, "%2d: %s  ", un->num, un->name);
	if (un->mixnum != -1)
	  cmn_err (CE_CONT, "mix=%d ", un->mixnum);
	if (un->source)
	  cmn_err (CE_CONT, "source=%d ", un->source);
	if (un->target)
	  cmn_err (CE_CONT, "target=%d ", un->target);
	cmn_err (CE_CONT, "ch=%d/%x ", un->channels, un->chmask);

	switch (un->typ)
	  {
	  case TY_INPUT:
	    cmn_err (CE_CONT, "Input terminal type: %04x ",
		     ossusb_get_int (&d[4], 2));
	    cmn_err (CE_CONT, "Associated output 0x%02x ", d[6]);
	    cmn_err (CE_CONT, "#chn %d ", d[7]);
	    cmn_err (CE_CONT, "chconf %04x ", ossusb_get_int (&d[8], 2));
	    if (d[10])
	      cmn_err (CE_CONT, "chname# %d ", d[10]);
	    if (d[11])
	      cmn_err (CE_CONT, "terminalname# %d (%s) ", d[11],
		       udi_usbdev_get_string (usbdev, d[11]));
	    break;

	  case TY_OUTPUT:
	    cmn_err (CE_CONT, "Output terminal type: %04x ",
		     ossusb_get_int (&d[4], 2));
	    cmn_err (CE_CONT, "Associated input 0x%02x ", d[6]);
	    cmn_err (CE_CONT, "sourceid %d ", d[7]);
	    if (d[8])
	      cmn_err (CE_CONT, "terminalname# %d ", d[8]);
	    break;

	  case TY_SELECT:
	    n = d[4];
	    d += 5;
	    cmn_err (CE_CONT, "%d input pins (", n);
	    for (i = 0; i < n; i++)
	      {
		ref = follow_source_links (devc, &devc->units[*d]);
		cmn_err (CE_CONT, "%d/%s ", *d, devc->units[ref].name);
		d++;
	      }
	    cmn_err (CE_CONT, ") ");
	    break;

	  case TY_MIXER:
	    n = d[4];
	    d += 5;
	    cmn_err (CE_CONT, "%d inputs (", n);
	    for (i = 0; i < n; i++)
	      {
		ref = follow_source_links (devc, &devc->units[*d]);
		cmn_err (CE_CONT, "%d/%s ", *d, devc->units[ref].name);
		d++;
	      }
	    cmn_err (CE_CONT, ") ");
	    break;

	  case TY_FEAT:
	    //ossusb_dump_desc(d, l);
	    cmn_err (CE_CONT, "Source %d:%s ", d[4], devc->units[d[4]].name);
	    n = d[5];
	    d += 6;
	    cmn_err (CE_CONT, "main (", n);
	    for (i = 0; i < n * 8; i++)
	      if ((name = check_feature (un, d, i)) != NULL)
		cmn_err (CE_CONT, "%s ", name);
	    cmn_err (CE_CONT, ") ");
	    for (j = 1; j <= un->channels; j++)
	      {
		cmn_err (CE_CONT, "ch %d (", j);
		for (i = 0; i < n * 8; i++)
		  if ((name = check_feature (un, d + j * n, i)) != NULL)
		    cmn_err (CE_CONT, "%s ", name);
		cmn_err (CE_CONT, ") ");
	      }
	    break;

	  case TY_PROC:
	    cmn_err (CE_CONT, "subtype %x Sources/%d) (", un->subtyp, n);
	    n = d[6];
	    d += 7;
	    cmn_err (CE_CONT, "%d ", *d);
	    d++;
	    cmn_err (CE_CONT, ") ");
	    break;

	  case TY_EXT:
	    cmn_err (CE_CONT, "Extension unit %02x%02x ", d[5], d[4]);
	    break;

	  }

	cmn_err (CE_CONT, "\n");
      }
#endif

  if (usb_mixerstyle == 0)
    setup_legacy_mixer (devc);
  touch_mixer (devc->mixer_dev);

  if (usb_trace)
    mixer_dump (devc);

  return devc;
}

static ossusb_devc *
find_devc (char *devpath, int vendor, int product)
{
  int i;

  for (i = 0; i < ndevs; i++)
    {
      if (devc_list[i]->vendor == vendor && devc_list[i]->product == product)
	if (strcmp (devc_list[i]->devpath, devpath) == 0)
	  {
	    UDB (cmn_err (CE_CONT, "Another instance of '%s'\n", devpath));
	    return devc_list[i];
	  }
    }

  return NULL;
}

static void
ossusb_device_disconnect (void *d)
{
  ossusb_devc *devc = d;
  int i;

  if (devc == NULL)
    {
      cmn_err (CE_WARN, "oss_usb_device_disconnect: devc==NULL\n");
      return;
    }

  if (devc->unload_func)
    {
      devc->unload_func (devc);
      return;
    }

  devc->disabled = 1;

  for (i = 0; i < devc->num_audio_engines; i++)
    {
      int dev;

      dev = devc->portc[i].audio_dev;

      if (dev < 0 || dev >= num_audio_engines)
	continue;

      audio_engines[dev]->enabled = 0;

      if (devc->mixer_dev >= 0)
	mixer_devs[devc->mixer_dev]->enabled = 0;
    }

}

static void *
ossusb_device_attach (udi_usb_devc * usbdev, oss_device_t * osdev)
{
  ossusb_devc *devc;
  char *devpath;
  int inum;
  int old = 1;
  int i;
  int class, subclass;
  int vendor, product, version;

  devpath = udi_usbdev_get_devpath (usbdev);
  inum = udi_usbdev_get_inum (usbdev);
  class = udi_usbdev_get_class (usbdev);
  subclass = udi_usbdev_get_subclass (usbdev);
  vendor = udi_usbdev_get_vendor (usbdev);
  product = udi_usbdev_get_product (usbdev);
  version = udi_usbdev_get_usb_version (usbdev);

  if ((devc = find_devc (devpath, vendor, product)) == NULL)
    {
      old = 0;

      if (ndevs >= MAX_DEVC)
	{
	  cmn_err (CE_CONT, "Too many USB audio devices\n");
	  return NULL;
	}

      if ((devc = PMALLOC (osdev, sizeof (*devc))) == NULL)
	{
	  cmn_err (CE_CONT, "Out of memory\n");
	  return NULL;
	}
      memset (devc, 0, sizeof (*devc));
      devc->mixer_dev = -1;

      devc->osdev = osdev;
      osdev->devc = devc;

      MUTEX_INIT (devc->osdev, devc->mutex, MH_DRV);

      devc->vendor = vendor;
      devc->product = product;
      devc->usb_version = version;
      devc->dev_name = udi_usbdev_get_name (usbdev);

      strcpy (devc->devpath, udi_usbdev_get_devpath (usbdev));

      devc_list[ndevs++] = devc;
    }
  else
    {
      devc->osdev = osdev;
      osdev->devc = devc;
    }

  if (devc->dev_name == NULL)
    devc->dev_name = "Generic USB device";
  oss_register_device (osdev, devc->dev_name);

  devc->disabled = 0;

  if (old)
    {
      /* Check if this interface number is already seen */
      old = 0;
      for (i = 0; !old && i < devc->num_interfaces; i++)
	if (devc->inum[i] == inum)
	  old = 1;
    }

  if (!old)
    {
      if (devc->num_interfaces >= MAX_IFACE)
	{
	  cmn_err (CE_CONT, "The device has too many interfaces\n");
	  return NULL;
	}

      devc->usbdev[devc->num_interfaces] = usbdev;
      devc->last_usbdev = usbdev;
      devc->inum[devc->num_interfaces] = inum;
      devc->num_interfaces++;
    }
  else
    {
      devc->last_usbdev = usbdev;
    }

  switch (class)
    {
    case USBCLASS_AUDIO:
      switch (subclass)
	{
	case 1:		/* Audiocontrol subclass */
	  devc->main_osdev = osdev;
	  if (!usb_quiet)
	    cmn_err (CE_CONT, "%s audioctl device %s/%d - %s\n",
		     old ? "Reinsert of an" : "New",
		     devc->devpath, inum, devc->dev_name);
	  return ossusb_init_audioctl (devc, usbdev, inum, old);
	  break;

	case 2:		/* Audio streaming subclass */
	  if (!usb_quiet)
	    cmn_err (CE_CONT, "%s audio streaming device %s/%d - %s\n",
		     old ? "Reinsert of an" : "New",
		     devc->devpath, inum, devc->dev_name);
	  devc->osdev->first_mixer = devc->main_osdev->first_mixer;
	  return ossusb_init_audiostream (devc, usbdev, inum, old);
	  break;

	case 3:		/* MIDI streaming subclass */
	  return NULL;
	  break;

	default:
	  cmn_err (CE_CONT,
		   "Unknown USB audio device subclass %x:%d, device=%s\n",
		   class, subclass, devc->dev_name);
	  return NULL;
	}
      break;

#if 0
    case USBCLASS_HID:
      cmn_err (CE_CONT, "HID interface class %x:%d, device=%s\n",
	       class, subclass, devc->dev_name);
      return NULL;
      break;
#endif

    default:
      cmn_err (CE_CONT, "Unknown USB device class %x:%d, device=%s\n",
	       class, subclass, devc->dev_name);
    }

  return devc;
}

static udi_usb_driver ossusb_driver = {
  ossusb_device_attach,
  ossusb_device_disconnect
};

int
oss_usb_attach (oss_device_t * osdev)
{
  if (usb_trace < 0)
    {
      udi_usb_trace = 0;
      usb_trace = 0;
      usb_quiet = 1;
    }
  else if (usb_trace > 0)
    udi_usb_trace = usb_trace;

  return udi_attach_usbdriver (osdev, known_devices, &ossusb_driver);
}

int
oss_usb_detach (oss_device_t * osdev)
{
  if (oss_disable_device (osdev) < 0)
    return 0;

  udi_unload_usbdriver (osdev);
  oss_unregister_device (osdev);

  return 1;
}
