/*
 * Purpose: Codec handling for Intel High Definition Audio (HDA/Azalia).
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

#include "oss_hdaudio_cfg.h"
#include "hdaudio.h"
#include "hdaudio_codec.h"
#include "hdaudio_codecids.h"

extern int hdaudio_snoopy;
extern int hdaudio_jacksense;
extern int hdaudio_noskip;

static codec_t NULL_codec = { 0 };	/* TODO: Temporary workaround - to be removed */


/* Si3055 functions (implemented in hdaudio_si3055.c) */
extern void hdaudio_si3055_endpoint_init(hdaudio_mixer_t *mixer, int cad);
extern void hdaudio_si3055_set_rate(hdaudio_mixer_t *mixer, int cad, int rate);
extern int hdaudio_si3055_set_offhook(hdaudio_mixer_t *mixer, int cad, int offhook);


static int attach_codec (hdaudio_mixer_t * mixer, int cad, char *hw_info,
		      unsigned int pci_subdevice, int group_type);
int
hdaudio_mixer_get_outendpoints (hdaudio_mixer_t * mixer,
				hdaudio_endpointinfo_t ** endpoints, int size)
{
  int i;

  if (size != sizeof (hdaudio_endpointinfo_t))
    {
      cmn_err (CE_WARN, "Bad endpoint size\n");
      return OSS_EIO;
    }

  *endpoints = (hdaudio_endpointinfo_t *) & mixer->outendpoints;

  for (i = 0; i < mixer->num_outendpoints; i++)
    {
      hdaudio_endpointinfo_t *ep = *endpoints + i;

      ep->skip = mixer->codecs[ep->cad]->widgets[ep->base_wid].skip;

      if (hdaudio_snoopy)
         {
	     char *s = ep->name + strlen(ep->name);
	     sprintf(s, ":%d:%d", ep->cad, ep->base_wid);
	 }
    }
  return mixer->num_outendpoints;
}

int
hdaudio_mixer_get_inendpoints (hdaudio_mixer_t * mixer,
			       hdaudio_endpointinfo_t ** endpoints, int size)
{
  int i;

  if (size != sizeof (hdaudio_endpointinfo_t))
    {
      cmn_err (CE_WARN, "Bad endpoint size\n");
      return OSS_EIO;
    }

  *endpoints = (hdaudio_endpointinfo_t *) & mixer->inendpoints;

  for (i = 0; i < mixer->num_inendpoints; i++)
    {
      hdaudio_endpointinfo_t *ep = *endpoints + i;

      ep->skip = mixer->codecs[ep->cad]->widgets[ep->base_wid].skip;

      if (hdaudio_snoopy)
         {
	     char *s = ep->name + strlen(ep->name);
	     sprintf(s, ":%d:%d", ep->cad, ep->base_wid);
	 }
    }
  return mixer->num_inendpoints;
}

/*ARGSUSED*/
static int
hda_mixer_ioctl (int dev, int audiodev, unsigned int cmd, ioctl_arg arg)
{
  if (cmd == SOUND_MIXER_READ_DEVMASK ||
      cmd == SOUND_MIXER_READ_RECMASK || cmd == SOUND_MIXER_READ_RECSRC ||
      cmd == SOUND_MIXER_READ_STEREODEVS)
    return *arg = 0;

  return OSS_EINVAL;
}

static mixer_driver_t hda_mixer_driver = {
  hda_mixer_ioctl
};


static void
propagate_names (hdaudio_mixer_t * mixer)
{
  int c, w;
  int i;
/*
 * Check if the same name can be used for all the widgets on an unique path.
 */

  for (i = 0; i < 20; i++)
    for (c = 0; c < mixer->ncodecs; c++)
      if (mixer->codecs[c] != &NULL_codec)
	{
	  for (w = 1; w < mixer->codecs[c]->nwidgets; w++)
	    {
	      widget_t *widget = &mixer->codecs[c]->widgets[w];
	      widget_t *src_widget =
		&mixer->codecs[c]->widgets[widget->connections[0]];

	      if (widget->nconn != 1)
		continue;

#if 0
	      if (src_widget->wid_type == NT_PIN
		  || src_widget->wid_type == NT_DAC)
		continue;

	      if (src_widget->wid_type != NT_MIXER && src_widget->wid_type != NT_VENDOR)	/* Mixer */
		continue;
#endif

	      strcpy (widget->name, src_widget->name);

	      /*
	       * Copy widget's RGB color to all widgets in the path
	       */
	      if (widget->rgbcolor == 0)
		 widget->rgbcolor = src_widget->rgbcolor;
	    }
	}
#if 0
  // Debugging code
    for (c = 0; c < mixer->ncodecs; c++)
    if (mixer->codecs[c] != &NULL_codec)
    for (w = 1; w < mixer->codecs[c]->nwidgets; w++)
    {
	      widget_t *widget = &mixer->codecs[c]->widgets[w];

	      cmn_err(CE_CONT, "w= %02x rgb=%06x: %s (%s)\n", w, widget->rgbcolor, widget->name, widget->color);
    }
#endif
}

static void
check_names (hdaudio_mixer_t * mixer)
{
  int c, c2, w, w2;
  int n, start;

/*
 * Make sure all widgets have unique names.
 */
  for (c = 0; c < mixer->ncodecs; c++)
    if (mixer->codecs[c] != &NULL_codec)
      {
	for (w = 1; w < mixer->codecs[c]->nwidgets; w++)
	  {
	    char tmp[16];
	    n = 0;
	    if (mixer->codecs[c]->widgets[w].skip)	/* Not available */
	      continue;

	    strcpy (tmp, mixer->codecs[c]->widgets[w].name);

	    start = w + 1;

	    for (c2 = c; c2 < mixer->ncodecs; c2++)
	      {
		for (w2 = start; w2 < mixer->codecs[c2]->nwidgets; w2++)
		  {
		    if (mixer->codecs[c2]->widgets[w2].skip)	/* Not available */
		      continue;

		    if (strcmp (tmp, mixer->codecs[c2]->widgets[w2].name) ==
			0)
		      n++;
		  }

		start = 1;
	      }

	    if (n > 0)		/* Duplicates found */
	      {
		n = 0;
		start = w;
		for (c2 = c; c2 < mixer->ncodecs; c2++)
		  {
		    for (w2 = start; w2 < mixer->codecs[c2]->nwidgets; w2++)
		      {
			if (mixer->codecs[c2]->widgets[w2].skip)	/* Not available */
			  continue;

			if (strcmp (tmp, mixer->codecs[c2]->widgets[w2].name)
			    == 0)
			  {
			    n++;
			    sprintf (mixer->codecs[c2]->widgets[w2].name,
				     "%s%d", tmp, n);
			  }
		      }

		    start = 1;
		  }
	      }
	  }
      }
}

int
hdaudio_amp_maxval (unsigned int ampcaps)
{
  int step, range, maxval;

  range = ((ampcaps >> AMPCAP_NUMSTEPS_SHIFT) & AMPCAP_NUMSTEPS_MASK) + 1;
  step = ((ampcaps >> AMPCAP_STEPSIZE_SHIFT) & AMPCAP_STEPSIZE_MASK) + 1;

  maxval = range * step * 25;	/* Now in 0.01 dB steps */
  maxval = (maxval / 10) - 1;	/* Truncate to centibel (0.1 dB) scaling */

  return maxval;
}

static int
scaleout_vol (int v, unsigned int ampcaps)
{
  int step, range, maxval;

  range = ((ampcaps >> AMPCAP_NUMSTEPS_SHIFT) & AMPCAP_NUMSTEPS_MASK) + 1;
  step = ((ampcaps >> AMPCAP_STEPSIZE_SHIFT) & AMPCAP_STEPSIZE_MASK) + 1;

  maxval = range * step * 25;	/* Now in 0.01 dB steps */
  maxval = (maxval / 10) - 1;	/* Truncate to centibel (0.1 dB) scaling */
  if (v > maxval)
    v = maxval;

  v *= 10;			/* centibels -> millibels */

  v = (v + (25 * step) / 2) / (25 * step);

  if (v > 0x7f || v >= range)
    {
      v = range - 1;
    }

  if (v < 0)
    v = 0;
  return v;
}

static int
scalein_vol (int v, unsigned int ampcaps)
{
  int step, range, maxval;

  range = ((ampcaps >> AMPCAP_NUMSTEPS_SHIFT) & AMPCAP_NUMSTEPS_MASK) + 1;
  step = ((ampcaps >> AMPCAP_STEPSIZE_SHIFT) & AMPCAP_STEPSIZE_MASK) + 1;

  maxval = range * step * 25;	/* Now in 0.01 dB steps */
  maxval = (maxval / 10) - 1;	/* Truncate to centibel (0.1 dB) scaling */

  v *= step * 25;		/* Convert to millibels */

  v = v / 10 - 1;		/* millibels -> centibels */

  if (v > maxval)
    {
      v = maxval;
    }
  if (v < 0)
    v = 0;

  return v;
}

static int
handle_insrcselect (hdaudio_mixer_t * mixer, widget_t * widget, int value)
{
/*
 * Emulated (recording) source selection based on input amp mute controls.
 *
 * Mute all inputs other than the selected one.
 */
  int i;

  widget->current_selector = value;

  for (i = 0; i < widget->nconn; i++)
    {
      int v = (i == value) ? 0 : 0x80;

      corb_write (mixer, widget->cad, widget->wid, 0,
		  SET_GAIN (0, 1, 1, 1, i), v);
    }

  return value;
}

int
hdaudio_set_control (int dev, int ctrl, unsigned int cmd, int value)
{
  hdaudio_mixer_t *mixer = mixer_devs[dev]->devc;

  unsigned int cad, wid, linked_wid, typ, ix, left, right, a, b, v;
  widget_t *widget;

  ix = ctrl & 0xff;
  typ = (ctrl >> 8) & 0xff;
  wid = (ctrl >> 16) & 0xff;
  cad = (ctrl >> 24) & 0xff;

  if (cad >= mixer->ncodecs)
    return OSS_EIO;

  if (wid >= mixer->codecs[cad]->nwidgets)
    return OSS_EIO;

  widget = &mixer->codecs[cad]->widgets[wid];

  if (mixer->codecs[cad]->vendor_flags & VF_VAIO_HACK)
    linked_wid = (wid == 0x02) ? 0x05 : ((wid == 0x05) ? 0x02 : 0);
  else
    linked_wid = 0;

  if (cmd == SNDCTL_MIX_READ)
    switch (typ)
      {
      case CT_INGAINSEL:
	if (!corb_read (mixer, cad, wid, 0, GET_GAIN (0, 0), ix, &a, &b))
	  return OSS_EIO;
	return a & 0x7f;	// TODO: Handle mute
	break;

      case CT_INMONO:
	if (!corb_read (mixer, cad, wid, 0, GET_GAIN (0, 0), ix, &a, &b))
	  return OSS_EIO;
	if (a & 0x80)
	  left = 0;
	else
	  left = scalein_vol (a, widget->inamp_caps);
	return left | (left << 16);
	break;

      case CT_INSTEREO:
	if (!corb_read (mixer, cad, wid, 0, GET_GAIN (0, 1), ix, &a, &b))
	  return OSS_EIO;
	if (a & 0x80)
	  left = 0;
	else
	  left = scalein_vol (a, widget->inamp_caps);
	if (!corb_read (mixer, cad, wid, 0, GET_GAIN (0, 0), ix, &a, &b))
	  return OSS_EIO;
	if (a & 0x80)
	  right = 0;
	else
	  right = scalein_vol (a, widget->inamp_caps);
	return left | (right << 16);
	break;

      case CT_INMUTE:
	if (!corb_read (mixer, cad, wid, 0, GET_GAIN (0, 0), ix, &a, &b))
	  return OSS_EIO;
	return (a >> 7) & 0x01;
	break;

      case CT_INSRC:		/* Inverse mute */
	if (!corb_read (mixer, cad, wid, 0, GET_GAIN (0, 0), ix, &a, &b))
	  return OSS_EIO;
	return !((a >> 7) & 0x01);
	break;

      case CT_SELECT:
	return widget->current_selector;
	break;

      case CT_INSRCSELECT:	/* Emulated selector based on input mute controls */
	return widget->current_selector;

      case CT_OUTGAINSEL:
	if (!corb_read (mixer, cad, wid, 0, GET_GAIN (1, 1), 0, &a, &b))
	  return OSS_EIO;
	return a;		// TODO: Handle mute
	break;

      case CT_OUTMONO:
	if (!corb_read (mixer, cad, wid, 0, GET_GAIN (1, 1), 0, &a, &b))
	  return OSS_EIO;
	left = a & 0x7f;
	if (a & 0x80)
	  left = 0;
	else
	  left = scalein_vol (a, widget->outamp_caps);
	return left | (left << 16);
	break;

      case CT_OUTSTEREO:
	if (!corb_read (mixer, cad, wid, 0, GET_GAIN (1, 1), 0, &a, &b))
	  return OSS_EIO;
	left = a & 0x7f;
	if (a & 0x80)
	  left = 0;
	else
	  left = scalein_vol (a, widget->outamp_caps);
	if (!corb_read (mixer, cad, wid, 0, GET_GAIN (1, 0), 0, &a, &b))
	  return OSS_EIO;
	right = a & 0x7f;
	if (a & 0x80)
	  right = 0;
	else
	  right = scalein_vol (a, widget->outamp_caps);
	return left | (right << 16);
	break;

      case CT_OUTMUTE:
	if (!corb_read (mixer, cad, wid, 0, GET_GAIN (1, 0), 0, &a, &b))
	  return OSS_EIO;
	return (a >> 7) & 0x01;
	break;

      default:
	return OSS_EINVAL;
      }

  if (cmd == SNDCTL_MIX_WRITE)
    {
      switch (typ)
	{
	case CT_INMONO:
	  v = (value & 0xffff);
	  if (v == 0)
	    v = 0x80;		/* Muted */
	  else
	    v = scaleout_vol (v, widget->inamp_caps);

	  corb_write (mixer, cad, wid, 0, SET_GAIN (0, 1, 1, 1, ix), v);
	  return value;
	  break;

	case CT_INSTEREO:
	  v = (value & 0xffff);
	  if (v == 0)
	    v = 0x80;		/* Muted */
	  else
	    v = scaleout_vol (v, widget->inamp_caps);

	  corb_write (mixer, cad, wid, 0, SET_GAIN (0, 1, 1, 0, ix), v);
	  v = ((value >> 16) & 0xffff);
	  if (v == 0)
	    v = 0x80;		/* Muted */
	  else
	    v = scaleout_vol (v, widget->inamp_caps);
	  corb_write (mixer, cad, wid, 0, SET_GAIN (0, 1, 0, 1, ix), v);
	  return value;
	  break;

	case CT_INGAINSEL:
	  v = (value & 0x7f);
	  // TODO: Handle mute
	  corb_write (mixer, cad, wid, 0, SET_GAIN (0, 1, 1, 0, ix), v);
	  corb_write (mixer, cad, wid, 0, SET_GAIN (0, 1, 0, 1, ix), v);
	  return value;
	  break;

	case CT_INMUTE:
	  v = 0;
	  if (value)
	    v = 0x80;
	  corb_write (mixer, cad, wid, 0, SET_GAIN (0, 1, 1, 1, ix), v);
	  return value;
	  break;

	case CT_INSRC:		/* Inverse mute */
	  v = 0;
	  if (!value)
	    v = 0x80;
	  corb_write (mixer, cad, wid, 0, SET_GAIN (0, 1, 1, 1, ix), v);
	  return value;
	  break;

	case CT_SELECT:
	  if (value < 0)
	    value = 0;
	  widget->current_selector = value;

	  if (value < widget->nconn)
	    {
	      /* Output source select */
	      corb_write (mixer, cad, wid, 0, SET_SELECTOR, value);
	      /* Enable output and HP amp. Set vref=Ground */
	      corb_write (mixer, cad, wid, 0, SET_PINCTL, 0xc0);
	    }
	  else
	    {
	      /* Input select
	       * Program the correct VRef Values
	       */

	      if (widget->pin_type == PIN_IN)	/* Line-in */
		{
		  corb_write (mixer, cad, wid, 0, SET_PINCTL, 0x20);	/*Ground*/
		}
	      else		/* Mic-in */
		{
		  corb_write (mixer, cad, wid, 0, SET_PINCTL, 0x24);	/*Vref=8
									   0% */
		}
	    }
	  return value;
	  break;

	case CT_INSRCSELECT:	/* Emulated selector based on input mute mask */
	  if (value < 0)
	    value = 0;
	  if (value >= widget->nconn)
	    value = widget->nconn;
	  return handle_insrcselect (mixer, widget, value);
	  break;

	case CT_OUTGAINSEL:
	  // TODO: Handle mute
	  corb_write (mixer, cad, wid, 0, SET_GAIN (1, 0, 1, 0, ix), value);
	  corb_write (mixer, cad, wid, 0, SET_GAIN (1, 0, 0, 1, ix), value);
	  return value;
	  break;

	case CT_OUTMONO:
	  v = (value & 0xffff);
	  if (v == 0)
	    v = 0x80;		/* Muted */
	  else
	    v = scaleout_vol (v, widget->outamp_caps);
	  corb_write (mixer, cad, wid, 0, SET_GAIN (1, 0, 1, 0, ix), v);
	  if (linked_wid)
	    corb_write (mixer, cad, wid, 0, SET_GAIN (1, 0, 1, 0, ix), v);
	  return value;
	  break;

	case CT_OUTSTEREO:
	  v = (value & 0xffff);
	  if (v == 0)
	    v = 0x80;		/* Muted */
	  else
	    v = scaleout_vol (v, widget->outamp_caps);
	  corb_write (mixer, cad, wid, 0, SET_GAIN (1, 0, 1, 0, ix), v);
	  if (linked_wid)
	    corb_write (mixer, cad, linked_wid, 0, SET_GAIN (1, 0, 1, 0, ix), v);
	  v = ((value >> 16) & 0xffff);
	  if (v == 0)
	    v = 0x80;		/* Muted */
	  else
	    v = scaleout_vol (v, widget->outamp_caps);
	  corb_write (mixer, cad, wid, 0, SET_GAIN (1, 0, 0, 1, ix), v);
	  if (linked_wid)
	    corb_write (mixer, cad, linked_wid, 0, SET_GAIN (1, 0, 0, 1, ix), v);
	  return value;
	  break;

	case CT_OUTMUTE:
	  v = 0;
	  if (value)
	    v = 0x80;
	  corb_write (mixer, cad, wid, 0, SET_GAIN (1, 0, 1, 1, ix), v);
	  if (linked_wid)
	    corb_write (mixer, cad, linked_wid, 0, SET_GAIN (1, 0, 1, 1, ix), v);
	  return value;
	  break;

	default:
	  return OSS_EINVAL;
	}
    }

  return OSS_EINVAL;
}

static int
perform_pin_sense (hdaudio_mixer_t * mixer)
{
  int cad, wid;
  unsigned int a, b;
  int plugged_in;
  codec_t *codec;

  for (cad = 0; cad < mixer->ncodecs; cad++)
    if (mixer->codecs[cad] != &NULL_codec)
      {
	codec = mixer->codecs[cad];

	for (wid = 1; wid < mixer->codecs[cad]->nwidgets; wid++)
	  if (mixer->codecs[cad]->widgets[wid].wid_type == NT_PIN)	/* Pin complex */
	    {
	      widget_t *widget = &mixer->codecs[cad]->widgets[wid];

	      widget->impsense = -1;
	      widget->sensed_pin = widget->pin_type;

	      plugged_in = 1;

	      /* Load the sense information */
	      if ((widget->pincaps & PINCAP_JACKSENSE_CAPABLE)
		  || (widget->pincaps & PINCAP_IMPSENSE_CAPABLE))
		{
		  int tmout = 0;
		  if (!corb_read
		      (mixer, cad, wid, 0, GET_PIN_SENSE, 0, &a, &b))
		    a = 0x7fffffff;	/* No jack present */

		  if (a & (1UL << 31))	/* Jack present */
		    if (widget->pincaps & PINCAP_TRIGGERR_RQRD)	/* Trigger required */
		      {
			corb_write (mixer, cad, wid, 0, TRIGGER_PIN_SENSE, 0);

			do
			  {
			    oss_udelay (10);
			    if (!corb_read
				(mixer, cad, wid, 0, GET_PIN_SENSE, 0, &a,
				 &b))
			      break;
			  }
			while (tmout++ < 10000
			       && ((a & 0x7fffffff) == 0x7fffffff));
		      }

		  if (!corb_read
		      (mixer, cad, wid, 0, GET_PIN_SENSE, 0, &a, &b))
		    continue;
		}
	      else
		continue;

	      /* Precence detect */
	      if (widget->pincaps & PINCAP_JACKSENSE_CAPABLE)
		{
#if 0
		  if (a & (1UL << 31))
		    cmn_err (CE_WARN, "%s jack is plugged in\n",
			     widget->name);
		  else
		    cmn_err (CE_WARN, "%s NOT plugged in\n", widget->name);
#endif
		  if (!(a & (1UL << 31)))
		    plugged_in = 0;
		}

	      widget->plugged_in = plugged_in;
	      if (plugged_in)
		codec->num_jacks_plugged++;

	      /* Impedance sensing */
	      widget->impsense = a & 0x7fffffff;
	      if (plugged_in && (widget->pincaps & PINCAP_IMPSENSE_CAPABLE))
		if (hdaudio_jacksense > 0)
		  if (widget->impsense != 0x7fffffff)	/* Sense operation finished */
		    {
		      /* cmn_err (CE_WARN, "%s sense %08x (%d)\n", widget->name, a, a & 0x7fffffff);  */

		      if (widget->impsense >= 1000000)	/* High impedance (line-in) pin */
			{
			  /* cmn_err(CE_CONT, "  --> Line level input\n"); */
			  widget->pin_type = widget->sensed_pin = PIN_IN;
			}
		      else if (widget->impsense <= 10000)	/* Low impedance (speaker/hp out) */
			{
			  /* cmn_err(CE_CONT, "  --> Output pin\n"); */
			  widget->pin_type = widget->sensed_pin = PIN_OUT;
			}
		      else	/* Something between low and high (mic?) */
			{
			  /* cmn_err(CE_CONT, "  --> Mic level input\n"); */
			  widget->pin_type = widget->sensed_pin = PIN_MIC;
			}
		    }
	    }
      }

/*
 * Set all pins to correct mode
 */

  for (cad = 0; cad < mixer->ncodecs; cad++)
    if (mixer->codecs[cad] != &NULL_codec)
      for (wid = 1; wid < mixer->codecs[cad]->nwidgets; wid++)
	{
	  if (mixer->codecs[cad]->widgets[wid].wid_type == NT_PIN)	/* Pin complex */
	    {
	      widget_t *widget = &mixer->codecs[cad]->widgets[wid];

	      if (widget->pin_type == PIN_IN
		  || widget->pin_type == PIN_UNKNOWN)
		{		/* Input PIN */
		  corb_write (mixer, cad, wid, 0, SET_PINCTL, 0x20);
		}
	      if (widget->pin_type == PIN_MIC)
		{		/* Input PIN (mic) */
		  /* TODO: Handle mic amp */
		  corb_write (mixer, cad, wid, 0, SET_PINCTL, 0x24);
		}
	      else
		{		/* Output PIN */
		  corb_write (mixer, cad, wid, 0, SET_PINCTL, 0xc0);
		}

	      if (widget->pincaps & PINCAP_EAPD)
		{
		  unsigned int eapd, dummy;
		  DDB (cmn_err
		       (CE_CONT, "Pin widget %d is EAPD capable.\n",
			widget->wid));
		  if (corb_read
		      (mixer, cad, wid, 0, GET_EAPD, 0, &eapd, &dummy))
		    {
		      eapd |= 0x02;	/* EAPD enable */
		      corb_write (mixer, cad, wid, 0, SET_EAPD, eapd);
		    }
		}
	    }
	}

  return 1;
}

static int
hdaudio_mix_init (int dev)
{
  hdaudio_mixer_t *mixer = mixer_devs[dev]->devc;
  char tmp[32];
  int err;
  int working_codecs=0;
  int cad;

/*
 * First pass. Count the number of active codecs.
 */

  for (cad = 0; cad < mixer->ncodecs; cad++)
    if (mixer->codecs[cad] != &NULL_codec)
      {
	codec_t *codec = mixer->codecs[cad];

	if (codec == &NULL_codec || codec->nwidgets == 0)	/* Codec not active */
	  continue;

	/*
	 * Enable the new generic mixer driver
	 */
	if (codec->mixer_init == NULL)
	{
	  codec->mixer_init = hdaudio_generic_mixer_init;
	}
	
	if (codec->active && codec->mixer_init != NULL_mixer_init)
	   working_codecs++;
      }

  /*
   * Second pass. Initialize the mixer interfaces for all active codecs.
   */

  for (cad = 0; cad < mixer->ncodecs; cad++)
    if (mixer->codecs[cad] != &NULL_codec)
      {
	codec_t *codec = mixer->codecs[cad];
	int group = 0;

	if (codec == &NULL_codec || codec->nwidgets == 0)	/* Codec not active */
	  continue;

	if (working_codecs > 1)
	  {
	    sprintf (tmp, "codec%d", cad + 1);
	    if ((group = mixer_ext_create_group (dev, 0, tmp)) < 0)
	      {
		return group;
	      }
	  }

	if (codec->active && codec->mixer_init != NULL_mixer_init)
	   {
	    if ((err = codec->mixer_init (dev, mixer, cad, group)) < 0)
	      {
		if (err != OSS_EAGAIN)
		  return err;
		/*
		 * We got EAGAIN whic means that we should fall
		 * to the old generic mixer.
		 */
		if ((err =
		     hdaudio_generic_mixer_init (dev, mixer, cad, group)) < 0)
		  {
		    return err;
		  }
	      }
	    else
	      continue;
	   }
      }

  if (mixer->client_mixer_init != 0)
    mixer->client_mixer_init (dev);

  return 0;
}

static void
copy_endpoints(hdaudio_mixer_t * mixer, codec_t *codec, int pass)
{
	int i;

/*
 * Install output endpoints from the codec to the global endpoint table.
 */

  for (i = 0; i < codec->num_outendpoints; i++)
    {
      hdaudio_endpointinfo_t *ep = &codec->outendpoints[i];
      ep->skip = mixer->codecs[ep->cad]->widgets[ep->base_wid].skip;
    }

	for (i=0;i<codec->num_outendpoints;i++)
	{
	  int ix = (codec->multich_map >> (i * 4)) & 0x0f;
          hdaudio_endpointinfo_t *ep = &codec->outendpoints[ix];

       	  ep->skip = mixer->codecs[ep->cad]->widgets[ep->base_wid].skip;

	  if (ep->skip || ep->already_used)
	     continue;

	  switch (pass)
	  {
	  case 0: /* Pick analog endpoints */
		if (ep->is_digital)
		   continue;
		break;

	  case 1: /* Pick digital endpoints */
		if (!ep->is_digital)
		   continue;
		break;
	  }

	  if (mixer->copied_outendpoints >= HDA_MAX_OUTSTREAMS)
	     {
	          cmn_err (CE_WARN,
			   "Too many output endpoints (%d)\n",
			   mixer->copied_outendpoints);
		  continue;
	     }

	  memcpy(&mixer->outendpoints[mixer->copied_outendpoints++], ep, sizeof(*ep));
	  ep->already_used=1;
	}
        mixer->num_outendpoints = mixer->copied_outendpoints;

/*
 * Install input endpoints from the codec to the global endpoint table.
 */

  for (i = 0; i < codec->num_inendpoints; i++)
    {
      hdaudio_endpointinfo_t *ep = &codec->inendpoints[i];
      ep->skip = mixer->codecs[ep->cad]->widgets[ep->base_wid].skip;
    }

	for (i=0;i<codec->num_inendpoints;i++)
	{
          hdaudio_endpointinfo_t *ep = &codec->inendpoints[i];

       	  ep->skip = mixer->codecs[ep->cad]->widgets[ep->base_wid].skip;

	  if (ep->skip || ep->already_used)
	     continue;

	  switch (pass)
	  {
	  case 0: /* Pick analog endpoints */
		if (ep->is_digital)
		   continue;
		break;

	  case 1: /* Pick digital endpoints */
		if (!ep->is_digital)
		   continue;
		break;
	  }

	  if (mixer->copied_inendpoints >= HDA_MAX_INSTREAMS)
	     {
	          cmn_err (CE_WARN,
			   "Too many output endpoints (%d)\n",
			   mixer->copied_inendpoints);
		  continue;
	     }

	  memcpy(&mixer->inendpoints[mixer->copied_inendpoints++], ep, sizeof(*ep));
	  ep->already_used=1;
	}
        mixer->num_inendpoints = mixer->copied_inendpoints;
}

 /*ARGSUSED*/
  hdaudio_mixer_t *
hdaudio_mixer_create (char *name, void *devc,
		      oss_device_t * osdev,
		      hdmixer_write_t writefunc,
		      hdmixer_read_t readfunc, unsigned int codecmask,
		      unsigned int vendor_id, unsigned int subvendor_id)
{
  hdaudio_mixer_t *mixer;
  int i, func;
  int ncodecs = 0;
  char tmp[128];
  int mixer_dev;

  char *hw_info = osdev->hw_info;	/* For printing hardware information */

  if ((mixer = PMALLOC (osdev, sizeof (*mixer))) == NULL)
    {
      cmn_err (CE_WARN, "hdaudio_mixer_create: Out of memory\n");
      return NULL;
    }

  memset (mixer, 0, sizeof (*mixer));

  mixer->devc = devc;
  mixer->osdev = osdev;
  mixer->mixer_dev = 0;
  strncpy (mixer->name, name, sizeof (mixer->name));
  mixer->name[sizeof (mixer->name) - 1] = 0;

  for (i = 0; i < MAX_CODECS; i++)
    mixer->codecs[i] = &NULL_codec;

  mixer->read = readfunc;
  mixer->write = writefunc;
  mixer->codecmask = codecmask;

  sprintf (hw_info, "HD Audio controller %s\n"
	   "Vendor ID    0x%08x\n"
	   "Subvendor ID 0x%08x\n", name, vendor_id, subvendor_id);
  hw_info += strlen (hw_info);

  if (hdaudio_snoopy)
     {
	sprintf(hw_info, "**** Warning: Diagnostic mode enabled (hdaudio_snoopy) ****\n");
  	hw_info += strlen (hw_info);
     }

/*
 * Search first all audio function groups for all codecs and then
 * handle modem function groups.
 */
  for (func=1;func<=2;func++)
  for (i = 0; i < 16; i++)
    if (mixer->codecmask & (1 << i))
      {
	if (attach_codec (mixer, i, hw_info, subvendor_id, func) >= 0)
	  ncodecs++;
	hw_info += strlen (hw_info);
      }

  if (ncodecs == 0)
    {
      cmn_err (CE_WARN, "No hdaudio codecs were detected\n");
      return NULL;
    }

/*
 * The attach_codec routine copied all analog endpoints to the global endpoint
 * table. Now pick possible digital endpoints from the active codecs.
 */

  for (i = 0; i < 16; i++)
    if (mixer->codecmask & (1 << i))
    if (mixer->codecs[i]->active)
      {
  	copy_endpoints(mixer, mixer->codecs[i], 1); /* Copy digital endpoints from codec to mixer */
      }

  if (!mixer->remap_avail)
     check_names (mixer);

  propagate_names (mixer);
  perform_pin_sense (mixer);

  if (mixer->chip_name == NULL)
    mixer->chip_name = "None";
  DDB (cmn_err (CE_CONT, "Mixer: %s %s\n", mixer->name, mixer->chip_name));
  //sprintf (tmp, "%s %s", mixer->name, mixer->chip_name);
  sprintf (tmp, "High Definition Audio %s", mixer->chip_name);

  if ((mixer_dev = oss_install_mixer (OSS_MIXER_DRIVER_VERSION,
				      osdev,
				      osdev,
				      tmp,
				      &hda_mixer_driver,
				      sizeof (mixer_driver_t), mixer)) < 0)
    {
      return NULL;
    }

  mixer_devs[mixer_dev]->hw_devc = devc;
  mixer_devs[mixer_dev]->priority = 10;	/* Known motherboard device */
  mixer->mixer_dev = mixer_dev;
  mixer_ext_set_init_fn (mixer->mixer_dev, hdaudio_mix_init,
			 mixer->ncontrols * 4);
  touch_mixer (mixer->mixer_dev);

  return mixer;
}

/*ARGSUSED*/
static int
find_playvol_widget (hdaudio_mixer_t * mixer, int cad, int wid)
{
  int this_wid = wid;

  return this_wid;
}

/*ARGSUSED*/
static int
find_recvol_widget (hdaudio_mixer_t * mixer, int cad, int wid)
{
  int this_wid = wid;

  return this_wid;
}

/*ARGSUSED*/
static int
find_recsrc_widget (hdaudio_mixer_t * mixer, int cad, int wid)
{
  int this_wid = wid;

  return this_wid;
}

static int
attach_node (hdaudio_mixer_t * mixer, int cad, int wid, int parent)
{
  static const char *widget_types[16] = {
    "Audio output",
    "Audio input",
    "Audio mixer",
    "Audio selector",
    "Pin complex",
    "Power widget",
    "Volume knob",
    "Beep generator",
    "Reserved8",
    "Reserved9",
    "ReservedA",
    "ReservedB",
    "ReservedC",
    "ReservedD",
    "ReservedE",
    "Vendor defined audio"
  };

  static const char *widget_id[16] = {
    "pcm",
    "rec",
    "mix",
    "select",
    "jack",
    "power",
    "vol",
    "beep",
    "unkn",
    "unkn",
    "unkn",
    "unkn",
    "unkn",
    "unkn",
    "unkn",
    "vendor"
  };

  static const int bit_sizes[] = {
    8,
    16,
    20,
    24,
    32
  };

  static const unsigned int bit_fmts[] = {
    AFMT_U8,
    AFMT_S16_LE,
    AFMT_S32_LE,
    AFMT_S32_LE,
    AFMT_S32_LE
  };

  static const int srates[] = {
    8000,
    11025,
    16000,
    22050,
    32000,
    44100,
    48000,
    88200,
    96000,
    176400,
    192000,
    384000
  };

  unsigned int widget_caps, b, pstate, group_type_in;
  unsigned int inamp_caps, outamp_caps;
  int group_type, wid_type;
  int i;

  codec_t *codec = mixer->codecs[cad];
  widget_t *widget;

  if (codec == &NULL_codec)
    return 0;

  if (!corb_read
      (mixer, cad, wid, 0, GET_PARAMETER, HDA_GROUP_TYPE, &group_type_in, &b))
    group_type_in = 0;

  if (wid >= MAX_WIDGETS)
    {
      cmn_err (CE_WARN, "Too many widgets for codec %d (%d/%d)\n", cad, wid, MAX_WIDGETS);
      return 0;
    }

  mixer->ncontrols++;

  codec->nwidgets = wid + 1;
  widget = &codec->widgets[wid];

  widget->cad = cad;
  widget->wid = wid;

  widget->rgbcolor = 0;

  group_type = group_type_in & 0xff;
  widget->group_type = group_type_in;

  DDB (cmn_err (CE_CONT, "Node %d, parent %d type %d, unsol capa %d\n",
		wid, parent, group_type, !!(group_type_in & 0x100)));

  if (!corb_read
      (mixer, cad, wid, 0, GET_PARAMETER, HDA_WIDGET_CAPS, &widget_caps, &b))
    {
      cmn_err (CE_WARN, "GET_PARAMETER HDA_WIDGET_CAPS failed\n");
      return 0;
    }

  if (widget_caps & WCAP_AMP_CAP_OVERRIDE)	/* Amp param override? */
    {
      if (!corb_read (mixer, widget->cad, widget->wid, 0,
		      GET_PARAMETER, HDA_OUTPUTAMP_CAPS, &outamp_caps, &b))
	{
	  cmn_err (CE_WARN, "GET_PARAMETER HDA_OUTPUTAMP_CAPS failed\n");
	  return OSS_EIO;
	}
      widget->outamp_caps = outamp_caps;

      if (!corb_read (mixer, widget->cad, widget->wid, 0,
		      GET_PARAMETER, HDA_INPUTAMP_CAPS, &inamp_caps, &b))
	{
	  cmn_err (CE_WARN, "GET_PARAMETER HDA_INPUTAMP_CAPS failed\n");
	  return OSS_EIO;
	}
      widget->inamp_caps = inamp_caps;
    }
  else
    {
      widget->outamp_caps = outamp_caps = codec->default_outamp_caps;
      widget->inamp_caps = inamp_caps = codec->default_inamp_caps;
    }

  if (!corb_read (mixer, cad, wid, 0, GET_POWER_STATE, 0, &pstate, &b))
    return 0;

  /* power up each of the widgets if there is a Power Capability (1<<10) */
  if (widget_caps & WCAP_POWER_CTL)
    corb_write (mixer, cad, wid, 0, SET_POWER_STATE, 0);

  widget->widget_caps = widget_caps;

  wid_type = (widget_caps >> 20) & 0x0f;
  DDB (cmn_err
       (CE_CONT, "\tWidget type %d (%s)(%s)\n", wid_type,
	widget_types[wid_type], widget_id[wid_type]));
  DDB (cmn_err (CE_CONT, "\tPower State %d\n", pstate));

  if (widget_caps & WCAP_CONN_LIST)
    {
      unsigned int clen;
      /* Handle connection list */

      if (!corb_read
	  (mixer, cad, wid, 0, GET_PARAMETER, HDA_CONNLIST_LEN, &clen, &b))
	{
	  cmn_err (CE_WARN, "GET_PARAMETER HDA_CONNLIST_LEN failed\n");
	  return 0;
	}

      if (clen & 0x80)
	{
	  cmn_err (CE_WARN, "Long form connection list not supported\n");
	  return 0;
	}

      if (clen > 0)
	{
	  if (clen > MAX_CONN)
	    {
	      cmn_err (CE_WARN, "Too many connections\n");
	      return 0;
	    }

	  DDB (cmn_err (CE_CONT, "\tConn list (%d): ", clen));

	  for (i = 0; i < clen; i += 4)
	    {
	      int j;
	      unsigned int a;

	      if (!corb_read
		  (mixer, cad, wid, 0, GET_CONNECTION_LIST_ENTRY, i, &a, &b))
		{
		  cmn_err (CE_WARN, "GET_CONNECTION_LIST_ENTRY failed\n");
		  return 0;
		}

	      for (j = 0; j < 4 && (i + j) < clen; j++)
		{
		  int v, is_range = 0;

		  if (widget->nconn >= MAX_CONN)
		    {
		      cmn_err (CE_WARN,
			       "Too many connections for widget %d (%d)\n",
			       widget->wid, widget->nconn);
		      break;
		    }

		  v = (a >> (j * 8)) & 0xff;
		  DDB (cmn_err (CE_CONT, "%d ", v));

		  if (v & 0x80)
		    {
		      is_range = 1;
		      v &= ~0x80;
		    }

		  if (v < 0 || v >= MAX_WIDGETS)
		    {
		      cmn_err (CE_NOTE,
			       "Connection %d for widget %d is out of range (%d) - Skipped\n",
			       j, widget->wid, v);
		      continue;
		    }

		  if (is_range)	/* Range: prev...v */
		    {
		      int x;
		      codec->widgets[v].references[codec->widgets[v].
						   refcount++] = wid;

		      if (widget->nconn < 1)
			{
			  cmn_err (CE_CONT,
				   "Bad range connection for widget %d\n",
				   widget->wid);
			  continue;
			}

		      for (x = widget->connections[widget->nconn - 1] + 1;
			   x <= v; x++)
			{
			  if (widget->nconn >= MAX_CONN)
			    {
			      cmn_err (CE_WARN,
				       "Too many connections(B) for widget %d (%d)\n",
				       widget->wid, widget->nconn);
			      break;
			    }

			  widget->connections[widget->nconn++] = x;
			  codec->widgets[x].references[codec->widgets[x].
						       refcount++] = wid;
			}
		    }
		  else
		    {
		      widget->connections[widget->nconn++] = v;
		      codec->widgets[v].references[codec->widgets[v].
						   refcount++] = wid;
		    }
		}
	    }

	  DDB (cmn_err (CE_CONT, "\n"));
	}
    }

  widget->wid_type = wid_type;

  strcpy (widget->name, widget_id[wid_type]);

  if (group_type == 0)		/* Not group but a widget */
    switch (wid_type)
      {
      case NT_DAC:		/* Audio output */
      case NT_ADC:		/* Audio input */
	{
	  unsigned int sizes;
	  int j;
	  hdaudio_endpointinfo_t *endpoint;

	  if (wid_type == 0)
	    {			/* Output endpoint */
	      if (mixer->num_outendpoints >= HDA_MAX_OUTSTREAMS)
		{
		  cmn_err (CE_WARN, "Too many output endpoints\n");
		  return 0;
		}

	      endpoint = &codec->outendpoints[codec->num_outendpoints++];

	      endpoint->stream_number = endpoint->default_stream_number =
		++mixer->num_outendpoints;
	      endpoint->ix = codec->num_outendpoints - 1;
	      endpoint->iddle_stream = 0;
	    }
	  else
	    {			/* Input endpoint */
	      if (mixer->num_inendpoints >= HDA_MAX_INSTREAMS)
		{
		  cmn_err (CE_WARN, "Too many input endpoints\n");
		  return 0;
		}

	      endpoint = &codec->inendpoints[codec->num_inendpoints++];

	      endpoint->stream_number = endpoint->default_stream_number =
		++mixer->num_inendpoints;
	      endpoint->ix = codec->num_inendpoints - 1;
	      endpoint->iddle_stream = 0;
	    }

	  endpoint->cad = cad;
	  endpoint->base_wid = wid;
	  endpoint->recsrc_wid = wid;
	  endpoint->volume_wid = wid;
	  endpoint->nrates=0;
	  endpoint->name = widget->name;

	  widget->endpoint = endpoint;

	  /*
	   * Find the widgets that manage rec/play volumes and recording 
	   * source selection.
	   */
	  switch (wid_type)
	    {
	    case NT_DAC:
	      endpoint->volume_wid = find_playvol_widget (mixer, cad, wid);
	      break;

	    case NT_ADC:
	      endpoint->volume_wid = find_recvol_widget (mixer, cad, wid);
	      endpoint->recsrc_wid = find_recsrc_widget (mixer, cad, wid);
	      break;
	    }

	  if (widget->widget_caps & WCAP_STEREO)
	    endpoint->channels = 2;
	  else
	    endpoint->channels = 1;

	  sizes = codec->sizes;

	  if (widget->widget_caps & WCAP_DIGITAL)	/* Digital */
	    {
	      endpoint->is_digital = 1;
	      if (wid_type == NT_ADC)
		strcpy (widget->name, "spdifin");
	      else
		{
		  strcpy (widget->name, "spdifout");
		  corb_write (mixer, cad, wid, 0, SET_SPDIF_CONTROL1, 1);	/* Digital output enable */
		  endpoint->iddle_stream = FRONT_STREAM;
		}

	      endpoint->fmt_mask |= AFMT_AC3;
	      if (sizes & (1 << 20))	/* 32 bits */
		{
		  endpoint->fmt_mask |= AFMT_SPDIF_RAW;
		}
	    }
	  else
	    {
	      endpoint->is_digital = 0;
	    }

	  if (widget->widget_caps & WCAP_FORMAT_OVERRIDE)	/* Override */
	    {
	      if (!corb_read (mixer, cad, wid, 0,
			      GET_PARAMETER, HDA_PCM_SIZES, &sizes, &b))
		{
		  cmn_err (CE_WARN, "GET_PARAMETER HDA_PCM_SIZES failed\n");
		  return 0;
		}
	    }

	  widget->sizes = sizes;
	  endpoint->sizemask = sizes;
	  if (sizes == 0)
	    {
	      corb_read (mixer, cad, codec->afg, 0, GET_PARAMETER,
			 HDA_PCM_SIZES, &sizes, &b);
	      widget->sizes = sizes;
	      endpoint->sizemask = sizes;
	    }

	  DDB (cmn_err (CE_CONT, "\tPCM Size/Rate %08x\n", sizes));

	  for (j = 0; j < 5; j++)
	    if (sizes & (1 << (j + 16)))
	      {
		DDB (cmn_err
		     (CE_CONT, "\t\tSupports %d bits\n", bit_sizes[j]));
		endpoint->fmt_mask |= bit_fmts[j];
	      }

	  for (j = 0; j < 12; j++)
	    if (sizes & (1 << j))
	      {
		DDB (cmn_err (CE_CONT, "\t\tSupports %d Hz\n", srates[j]));
		if (endpoint->nrates < 20)
		  {
		    endpoint->rates[endpoint->nrates++] = srates[j];
		  }
	      }

	  if ((widget->widget_caps & WCAP_DIGITAL) && wid_type == NT_DAC)	/* Digital output */
	    {
	      /*
	       * Select front output as the default stream number. In 
	       * this way copy of the analog front signal will automatically
	       * be delivered to the S/PDIF outpput when the S/PDIF device
	       * is not being used for some other purpose.
	       */
	      corb_write (mixer, cad, wid, 0, SET_CONVERTER,
			  FRONT_STREAM << 4);
	    }
	  else
	    {
	      /* Select the iddle stream (0) for analog outputs */
	      corb_write (mixer, cad, wid, 0, SET_CONVERTER,
			  IDDLE_STREAM << 4);
	    }
	  /* Select 48 kHz/16 bits/stereo */
	  corb_write (mixer, cad, wid, 0, SET_CONVERTER_FORMAT, 0x0009);

	}
	break;

      case NT_KNOB:		/* Volume knob */
	/* Clear the direct control bit */
	corb_write (mixer, cad, wid, 0, SET_VOLUME_KNOB_CONTROL, 0x00);
	break;

      case NT_PIN:		/* Pin complex */
	{
	  unsigned int conf;
	  unsigned int pincaps;

	  if (!corb_read
	      (mixer, cad, wid, 0, GET_CONFIG_DEFAULT, 0, &conf, &b))
	    return 0;
	  if (!corb_read (mixer, cad, wid, 0,
			  GET_PARAMETER, HDA_PIN_CAPS, &pincaps, &b))
	    {
	      cmn_err (CE_WARN, "GET_PARAMETER HDA_PIN_CAPS failed\n");
	      return 0;
	    }

	  widget->pincaps = pincaps;
#if 0
	  if (widget->widget_caps & WCAP_UNSOL_CAPABLE)
	    corb_write (mixer, cad, wid, 0, SET_UNSOLICITED, 0x80 | wid);
#endif

	  if (conf != 0)
	    {
	      int default_device = (conf >> 20) & 0x0f;
	      int default_loc = (conf >> 24) & 0x3f;
	      int conn = (conf >> 30) & 0x03;

	      int color;
	      int  no_color=0;
	      char *name = NULL, *loc = "", *color_name = NULL;

	      color = (conf >> 12) & 0x0f;

	      if (pincaps & (1 << 6))
		cmn_err (CE_WARN, "Balanced I/O not supported\n");
	      if (!(pincaps & (1 << 5)))	/* No input */
		widget->pin_type = PIN_OUT;
	      if (!(pincaps & (1 << 4)))
		widget->pin_type = PIN_IN;

	      DDB (cmn_err (CE_CONT, "\tConfig default %08x\n", conf));

	      if ((default_loc & 0x0f) == 0x1)	/* Rear panel - default */
		loc = "";
	      if ((default_loc & 0x0f) == 0x2)	/* Front panel */
		loc = "fp-";
	      if ((default_loc & 0xf0) == 0x10)	/* Internal func - eg cd/tad/spk */
		loc = "int-";

	      if (conn == 1 && !(hdaudio_noskip & 1))	/* Pin not connected to anything */
		{
		  widget->skip = 1;
		  widget->skip_output = 1;
		}

	      switch (default_device)
		{
		case 0x0:
		  name = "lineout";
		  widget->pin_type = PIN_OUT;
		  break;
		case 0x1:
		  name = "speaker";
		  no_color=1;
		  widget->pin_type = PIN_OUT;
		  break;
		case 0x2:
		  name = "headphone";
		  widget->pin_type = PIN_OUT;
		  break;
		case 0x3:
		  name = "cd";
		  no_color=1;
		  widget->pin_type = PIN_IN;
		  break;
		case 0x4:
		  name = "spdifout";
		  no_color=1;
		  widget->pin_type = PIN_OUT;
		  break;
		case 0x5:
		  name = "digout";
		  no_color=1;
		  widget->pin_type = PIN_OUT;
		  break;
		case 0x6:
		  name = "modem";
		  no_color=1;
		  break;
		case 0x7:
		  name = "phone";
		  no_color=1;
		  break;
		case 0x8:
		  name = "linein";
		  widget->pin_type = PIN_IN;
		  break;
		case 0x9:
		  name = "aux";
		  break;
		case 0xa:
		  name = "mic";
		  widget->pin_type = PIN_MIC;
		  break;
		case 0xb:
		  name = "telephony";
		  no_color=1;
		  break;
		case 0xc:
		  name = "spdifin";
		  no_color=1;
		  break;
		case 0xd:
		  name = "digin";
		  no_color=1;
		  break;
		case 0xe:
		  name = "reserved";
		  no_color=1;
		  break;
		case 0xf:	/* Unused pin widget */
		  if (hdaudio_noskip & 2) break;
		  widget->skip = 1;
		  widget->skip_output = 1;
		  break;
		}

/* process only colored jacks and skip fixed function jacks */
	      switch (color)
		{
		case 0x1:
		  color_name = "black";
		  widget->rgbcolor = OSS_RGB_BLACK;
		  break;
		case 0x2:
		  color_name = "gray";
		  widget->rgbcolor = OSS_RGB_GRAY;
		  break;
		case 0x3:
		  color_name = "blue";
		  widget->rgbcolor = OSS_RGB_BLUE;
		  break;
		case 0x4:
		  color_name = "green";
		  widget->rgbcolor = OSS_RGB_GREEN;
		  break;
		case 0x5:
		  color_name = "red";
		  widget->rgbcolor = OSS_RGB_RED;
		  break;
		case 0x6:
		  color_name = "orange";
		  widget->rgbcolor = OSS_RGB_ORANGE;
		  break;
		case 0x7:
		  color_name = "yellow";
		  widget->rgbcolor = OSS_RGB_YELLOW;
		  break;
		case 0x8:
		  color_name = "purple";
		  widget->rgbcolor = OSS_RGB_PURPLE;
		  break;
		case 0x9:
		  color_name = "pink";
		  widget->rgbcolor = OSS_RGB_PINK;
		  break;
		case 0xe:
		  color_name = "white";
		  widget->rgbcolor = OSS_RGB_WHITE;
		  break;

		default:
		  if (name != NULL)
		    color_name = name;
		  else
		    color_name = "internal";
		}

	      if (no_color)
		 widget->rgbcolor = 0;

	      if (default_device == 0xf) /* Not present */
	      {
		      widget->rgbcolor=0;
		      color_name = "internal";
	      }

	      sprintf (widget->color, "%s%s", loc, color_name);
/*
 * By Hannu 20080111
 * Use jack color as the widget name if no name was defined or if the default 
 * function is lineout. This fixes the problem of several jacks being named
 * as lineout.
 */
	      if (name == NULL || default_device == 0x00)
		name = color_name;
	      sprintf (widget->name, "%s%s", loc, name);
	    }
	}
	break;
      }

#if 0
  if (!corb_read
      (mixer, cad, wid, 0, GET_PARAMETER, HDA_NODE_COUNT, &node_count, &b))
    {
      cmn_err (CE_WARN, "GET_PARAMETER HDA_NODE_COUNT1 failed\n");
      return 0;
    }

  first_node = (node_count >> 16) & 0xff;
  num_nodes = node_count & 0xff;

  DDB (cmn_err
       (CE_CONT, "\tFirst node %d, num nodes %d\n", first_node, num_nodes));

  if (first_node > 0)
    for (i = first_node; i < first_node + num_nodes; i++)
      if (!attach_node (mixer, cad, i, wid))
	return 0;
#endif
/*
 * Handle hardcodded widget names
 */

  if (codec->remap != NULL)
    {
      int w;

      mixer->remap_avail=1;

      for (w = 0; codec->remap[w] != NULL; w++)
	if (w == wid)
	  {
	    char *s = codec->remap[w];

	    if (*s != 0)
	      {
		strcpy (widget->name, s);
		if (*widget->color == 0)
		  {
		    strcpy (widget->color, widget->name);
		  }
	      }
	    break;
	  }
    }

  return 1;
}

/*ARGSUSED*/
static int
attach_group (hdaudio_mixer_t * mixer, int cad, int wid, int parent, int group_type)
{
  unsigned int a, b, gt;
  int i, first_node, num_nodes;
  codec_t *codec = mixer->codecs[cad];

  if (codec == &NULL_codec)
    return 0;

  if (!corb_read (mixer, cad, wid, 0,
		  GET_PARAMETER, HDA_OUTPUTAMP_CAPS,
		  &codec->default_outamp_caps, &b))
    {
      cmn_err (CE_WARN, "GET_PARAMETER HDA_OUTPUTAMP_CAPS failed\n");
      return 0;
    }

  if (!corb_read (mixer, cad, wid, 0,
		  GET_PARAMETER, HDA_INPUTAMP_CAPS,
		  &codec->default_inamp_caps, &b))
    {
      cmn_err (CE_WARN, "GET_PARAMETER HDA_INPUTTAMP_CAPS failed\n");
      return 0;
    }

  if (!corb_read (mixer, cad, wid, 0, GET_PARAMETER, HDA_NODE_COUNT, &a, &b))
    {
      cmn_err (CE_WARN, "GET_PARAMETER HDA_NODE_COUNT2 failed\n");
      return 0;
    }

  first_node = (a >> 16) & 0xff;
  num_nodes = a & 0xff;

  corb_read (mixer, cad, wid, 0, GET_PARAMETER, HDA_GROUP_TYPE, &gt, &b);
  DDB (cmn_err (CE_CONT, "\tGroup %d First node %d, num nodes %d\n", wid,
		first_node, num_nodes));
/*
 * Ignore other than audio function groups. Codecs probably allocate
 * higher widget number for the modem group than the audio group. So in this
 * way we can have smaller MAX_WIDGETS which in turn conserves memory.
 */
  if ((gt & 0xff) != group_type)
    return 0;

  if (corb_read (mixer, cad, wid, 0, GET_PARAMETER, HDA_PCM_SIZES, &a, &b))
    {
      codec->sizes = a;
    }

  if (first_node > 0)
    for (i = first_node; i < first_node + num_nodes; i++)
      if (!attach_node (mixer, cad, i, wid))
	return 0;

  if (num_nodes >= 1)
     codec->active=1;
  return 1;
}

static void
polish_widget_list (hdaudio_mixer_t * mixer, int cad)
{
  widget_t *widget;
  codec_t *codec;
  int wid, conn, loop;
  int skip = 0;
  int do_jacksense = 0;

  if (hdaudio_noskip & 4) return;

  if (mixer->codecs[cad] == &NULL_codec)
    {
      cmn_err (CE_WARN, "Bad codec %d\n", cad);
    }
  codec = mixer->codecs[cad];

  for (wid = 0; wid < codec->nwidgets; wid++)
    {
      widget = &codec->widgets[wid];
    }

/*
 * Use jack sensing information to remove unconnected I/O pints from the mixer
 * interface.
 */

  do_jacksense = 1;
  if (codec->num_jacks_plugged < 1 || hdaudio_jacksense < 1)
    do_jacksense = 0;

  if (do_jacksense)
    for (wid = 0; wid < codec->nwidgets; wid++)
      {
	widget = &codec->widgets[wid];

	if (widget->wid_type != NT_PIN)	/* Not a pin widget */
	  continue;

	if (!widget->plugged_in)
	  widget->skip = 1;
      }

  /*
   * Check all widgets and mark them unusable (skip=1) if all of their input
   * connections are marked to be skipped.
   *
   * This needs to be done number of times so that the skip status propagates
   * to the end of the longest path.
   */

  for (loop = 0; loop < codec->nwidgets / 4; loop++)	/* nwidgets/4 iterations */
    for (wid = 0; wid < codec->nwidgets; wid++)
      {
	widget = &codec->widgets[wid];

	if (widget->skip || widget->used)
	  continue;

	skip = 1;		/* For now */
	for (conn = 0; conn < widget->nconn; conn++)
	  {
	    if (!codec->widgets[widget->connections[conn]].skip)
	      skip = 0;		/* Cannot be skipped */
	  }

	if (skip && widget->nconn > 0)
	  {
	    widget->skip = 1;
	    widget->used = 1;
	  }
      }

/*
 * Do the same backwards. Remove widgets that don't have connectivity to any
 * of the pin widgets.
 */
  for (loop = 0; loop < codec->nwidgets / 4; loop++)	/* nwidgets/4 iterations */
    for (wid = 0; wid < codec->nwidgets; wid++)
      {
	widget = &codec->widgets[wid];

	if (widget->skip_output || widget->used)
	  continue;

	skip = 1;		/* For now */
	for (conn = 0; conn < widget->refcount; conn++)
	  {
	    if (!codec->widgets[widget->references[conn]].skip_output)
	      skip = 0;		/* Cannot be skipped */
	  }

	if (skip && widget->refcount > 0)
	  {
	    widget->skip_output = 1;
	    widget->used = 1;
	  }
      }

/*
 * Final pass.
 */

  for (wid = 0; wid < codec->nwidgets; wid++)
    {
      widget = &codec->widgets[wid];

      if (widget->skip_output)
	{
	  widget->skip = 1;
	}
    }
}


/* ARGSUSED */
static int
attach_codec (hdaudio_mixer_t * mixer, int cad, char *hw_info,
		      unsigned int pci_subdevice, int group_type)
{
  unsigned int a, b, x;
  int subix, ix, i;
  int first_node, num_nodes;
  int has_audio_group = 0;
  codec_t *codec;

  if (cad >= MAX_CODECS)
    {
      cmn_err (CE_WARN, "attach_codec: Too many codecs %d\n", cad);
      return OSS_EIO;
    }

  mixer->ncodecs = cad + 1;

  if (mixer->codecs[cad] == &NULL_codec)
     {
	  if ((codec = PMALLOC (mixer->osdev, sizeof (*codec))) == NULL)
	    {
	      cmn_err (CE_CONT, "Cannot allocate codec descriptor\n");
	      return OSS_ENOMEM;
	    }
	
	  memset (codec, 0, sizeof (*codec));
	
	  mixer->codecs[cad] = codec;
     }
  else
     {
     	codec = mixer->codecs[cad];
     }

  corb_write (mixer, cad, 0, 0, SET_POWER_STATE, 0);	/* Power up everything */

  if (!corb_read (mixer, cad, 0, 0, GET_PARAMETER, HDA_VENDOR, &a, &b))
    {
      if (group_type == 1)
         {
		sprintf (hw_info, " Codec %2d: Not present\n", cad);
      		cmn_err (CE_NOTE,
	       		"attach_codec: Codec #%d is not physically present\n",
	       		cad);
	 }
      return OSS_EIO;
    }
  codec->vendor_id = a;

/*
 * Find out the primary group list
 */

  if (!corb_read (mixer, cad, 0, 0, GET_PARAMETER, HDA_NODE_COUNT, &x, &b))
    {
      cmn_err (CE_WARN, "GET_PARAMETER HDA_NODE_COUNT3 failed\n");
      return OSS_EIO;
    }

  codec->first_node = first_node = (x >> 16) & 0xff;
  num_nodes = x & 0xff;

/*
 * Check if this one is an audio codec (has an audio function group)
 */
  for (i = first_node; i < first_node + num_nodes; i++)
    {
      unsigned int gt;

      corb_read (mixer, cad, i, 0, GET_PARAMETER, HDA_GROUP_TYPE, &gt, &b);
/*
 * Handle only the function group type requested by the upper layer code.
 */
      if ((gt & 0xff) != 1)
	continue;

      has_audio_group = 1;
    }

/*
 * Find codec specific settings
 */
  for (ix = 0; codecs[ix].id != 0; ix++)
    if (codecs[ix].id == a)
      break;

  DDB (cmn_err
       (CE_CONT, "HD audio Codec ID: %08x (%s)\n", a, codecs[ix].name));

  if (codecs[ix].id == 0)	/* Unknown codec */
    {
      if (group_type == 1)
         sprintf (hw_info, " Codec %2d: Unknown (0x%08x", cad, a);
      cmn_err (CE_NOTE, "HDA codec 0x%08x not known yet\n", a);
      /* 
       * Create hexadecimal codec ID
       */
      if (has_audio_group && mixer->chip_name == NULL)
	if ((mixer->chip_name = PMALLOC (mixer->osdev, 32)) != NULL)
	  {
	    sprintf (mixer->chip_name, "0x%08x", a);
	  }
    }
  else
    {
      if (group_type == 1)
         sprintf (hw_info, " Codec %2d: %s (0x%08x", cad, codecs[ix].name, a);
    }

  if (has_audio_group && mixer->chip_name == NULL)
    {
      mixer->chip_name = codecs[ix].name;

    }
    
    if (codecs[ix].remap != NULL)
       codec->remap = codecs[ix].remap;

    if (codecs[ix].flags != 0)
       codec->vendor_flags = codecs[ix].flags;

  if (codecs[ix].mixer_init != NULL)
     codec->mixer_init = codecs[ix].mixer_init;

  codec->multich_map = codecs[ix].multich_map;

  if (corb_read (mixer, cad, 0, 0, GET_PARAMETER, HDA_REVISION, &a, &b))
    {
      DDB (cmn_err (CE_CONT, "HDA codec revision %d.%d (%d.%d) (0x%08x)\n",
		    (a >> 20) & 0xf,
		    (a >> 16) & 0xf, (a >> 8) & 0xff, a & 0xff, a));
    }
  else
    DDB (cmn_err (CE_CONT, "Can't get codec revision\n"));

  codec->codec_desc = &codecs[ix];

  DDB (cmn_err (CE_CONT, "**** Codec %d ****\n", cad));
  DDB (cmn_err
       (CE_CONT, "First node %d, num nodes %d\n", first_node, num_nodes));

  for (i = first_node; i < first_node + num_nodes; i++)
    {
      corb_read (mixer, cad, i, 0, GET_PARAMETER, HDA_GROUP_TYPE, &a, &b);
      if ((a & 0xff) == group_type)	/* Proper function group type */
	{
	  codec->afg = i;

	  if (corb_read (mixer, cad, i, 0, GET_SUBSYSTEM_ID, 0, &a, &b))
	    {
	      DDB (cmn_err (CE_CONT, "Subsystem ID = 0x%08x\n", a));

      	      if (group_type == 1)
	         {
	      		/* Append subvendor ID to hw_info */
	      		hw_info += strlen (hw_info);
	      		sprintf (hw_info, "/0x%08x", a);
	         }

	      codec->subvendor_id = a;

	      for (subix = 0; subdevices[subix].id != 0; subix++)
		if (subdevices[subix].id == a)
		  {
		    if (subdevices[subix].main_id != 0)
		      if (subdevices[subix].main_id != codec->vendor_id)
			continue;

		    if (subdevices[subix].pci_subdevice != 0)
		      if (subdevices[subix].pci_subdevice != pci_subdevice)
			continue;


		    DDB (cmn_err
			 (CE_CONT, "Subdevice known as %s\n",
			  subdevices[subix].name));
      		    if (group_type == 1)
		       {
		    	  hw_info += strlen (hw_info);
		    	  sprintf (hw_info, " %s", subdevices[subix].name);
		       }
		    if (subdevices[subix].remap != NULL)
		    {
		      codec->remap = subdevices[subix].remap;
		    }

		    if (subdevices[subix].multich_map != 0)
		      codec->multich_map = subdevices[subix].multich_map;
		    if (subdevices[subix].flags != 0)
		      codec->vendor_flags = subdevices[subix].flags;
		    if (subdevices[subix].mixer_init != NULL)
		    {
		      codec->mixer_init = subdevices[subix].mixer_init;
		    }
		  }
	    }
	  break;
	}
    }

  hw_info += strlen (hw_info);
  if (group_type == 1)
     strcpy (hw_info, ")\n");

  if (codec->multich_map == 0)
    {
      codec->multich_map = 0x76543210;	/* Sequential order */
    }

  for (i = first_node; i < first_node + num_nodes; i++)
    {
      if (!attach_group (mixer, cad, i, 0, group_type))
	continue;

      /* power up the AFG! */
      corb_write (mixer, cad, i, 0, SET_POWER_STATE, 0);
    }
    
  /* Initialize and setup manually endpoints for Si3055. */
  if ((mixer->codecs[cad]->vendor_flags & VF_SI3055_HACK) && (group_type == 2))
    {
      hdaudio_si3055_endpoint_init(mixer, cad);
    }

  if (has_audio_group)
    {
      polish_widget_list (mixer, cad);
    }

  copy_endpoints(mixer, codec, 0); /* Copy analog endpoints from codec to mixer */

  return (has_audio_group) ? 0 : OSS_EIO;
}

int
hdaudio_mixer_get_mixdev (hdaudio_mixer_t * mixer)
{
  return mixer->mixer_dev;
}

void
hdaudio_mixer_set_initfunc (hdaudio_mixer_t * mixer,
			    mixer_create_controls_t func)
{
  mixer->client_mixer_init = func;
}

#define BASE44k	(1<<14)

static const struct hdaudio_rate_def _hdaudio_rates[] = {
  /* 48 kHz based rates */
  {192000, (3 << 11)},		/* 4x */
  {96000, (1 << 11)},		/* 2x */
  {48000, 0},			/* 1x */
  {24000, (1 << 8)},		/* 1/2x */
  {16000, (2 << 8)},		/* 1/3x */
  {12000, (3 << 8)},		/* 1/4x */
  {9600, (4 << 8)},		/* 1/5x */
  {8000, (5 << 8)},		/* 1/6x */
  /* TODO: These rates didn't work for some reason. */
  /* 44.1 kHz based rates */
  {176400, BASE44k | (3 << 11)},	/* 4x */
  {88200, BASE44k | (1 << 11)},	/* 2x */
  {44100, BASE44k},		/* 1x */
  {22050, BASE44k | (1 << 8)},	/* 1/2x */
  {14700, BASE44k | (2 << 8)},	/* 1/3x */
  {11025, BASE44k | (3 << 8)},	/* 1/4x */
  {8820, BASE44k | (4 << 8)},	/* 1/5x */
  {7350, BASE44k | (5 << 8)},	/* 1/6x */
  {0}
};

const struct hdaudio_rate_def *hdaudio_rates = _hdaudio_rates;

int
hdaudio_codec_setup_endpoint (hdaudio_mixer_t * mixer,
			      hdaudio_endpointinfo_t * endpoint,
			      int rate, int channels, int fmt,
			      int stream_number, unsigned int *setupbits)
{

  unsigned int tmp, spdif, dummy;
  int i;

  endpoint->auto_muted = 0;

  if (!corb_read
      (mixer, endpoint->cad, endpoint->base_wid, 0, GET_SPDIF_CONTROL, 0,
       &spdif, &dummy))
    spdif = 0;

  spdif &= ~(1 << 5);		/* Audio */

  tmp = 0;

  if (fmt == AFMT_AC3)
    channels = 2;

  tmp |= channels - 1;

  switch (fmt)
    {
    case AFMT_U8:
      break;

    case AFMT_S16_LE:
      tmp |= 0x00000010;
      break;

    case AFMT_S32_LE:
      if (endpoint->sizemask & (1 << 20))	/* 32 bits */
	tmp |= 0x00000040;
      else if (endpoint->sizemask & (1 << 19))	/* 24 bits */
	tmp |= 0x00000030;
      else if (endpoint->sizemask & (1 << 18))	/* 20 bits */
	tmp |= 0x00000020;
      else
	{
	  cmn_err (CE_WARN, "Bad bit size\n");
	  return OSS_EIO;
	}
      break;

    case AFMT_AC3:
      tmp &= 0xff;
      tmp |= 0x11;		/* 16 bits stereo */
      spdif |= (1 << 5);	/* Data */
      break;

    case AFMT_SPDIF_RAW:
      tmp &= 0xff;
      tmp |= 0x81;		/* 32 bits stereo */
      break;

    default:
      cmn_err (CE_WARN, "Bad format %x\n", fmt);
      return OSS_EIO;
    }

  corb_write (mixer, endpoint->cad, endpoint->base_wid, 0, SET_SPDIF_CONTROL1,
	      spdif & 0xff);
/*
 * Finally the sample rate setup
 */

  for (i = 0; hdaudio_rates[i].rate != 0; i++)
    if (hdaudio_rates[i].rate == rate)
      {
	tmp |= hdaudio_rates[i].mask;
	break;
      }

  *setupbits = tmp;
  
  if (mixer->codecs[endpoint->cad]->vendor_flags & VF_SI3055_HACK)
    {
      hdaudio_si3055_set_rate(mixer, endpoint->cad, rate);
    }

  corb_write (mixer, endpoint->cad, endpoint->base_wid, 0,
	      SET_CONVERTER_FORMAT, tmp);
  corb_write (mixer, endpoint->cad, endpoint->base_wid, 0, SET_CONVERTER,
	      stream_number << 4);

  if (channels > 2)
    {
      /*
       * Set up the converters for the other stereo pairs
       */
#if 1
      // TODO: Test this

      int n = (channels + 1) / 2;

      for (i = 1; i < n; i++)
	{
	  hdaudio_endpointinfo_t *ep;
	  ep = &endpoint[i];

	  corb_write (mixer, ep->cad, ep->base_wid, 0, SET_CONVERTER_FORMAT,
		      tmp);
	  corb_write (mixer, ep->cad, ep->base_wid, 0, SET_CONVERTER,
		      (stream_number << 4) | (i * 2));
	}
#endif
    }

  if (mixer->codecs[endpoint->cad]->vendor_flags & VF_VAIO_HACK)
    {
      /*
       * STAC9872 specific hack. In Sony VAIO configurations, the DAC widget
       * used for the headphone jack needs to duplicate the stream playing on
       * the DAC widget for the speaker when not in multichannel mode.
       */
      if (channels <= 2 && endpoint->base_wid == 0x05)
	{
	  corb_write (mixer, endpoint->cad, 0x02, 0, SET_CONVERTER_FORMAT,
		      tmp);
	  corb_write (mixer, endpoint->cad, 0x02, 0, SET_CONVERTER,
		      stream_number << 4);
	}
    }

#if 1
  if (mixer->codecs[endpoint->cad]->vendor_flags & VF_ALC88X_HACK)
    {
      /*
       * ALC88x specfic hack. These codecs cannot play S/PDIF unless the front
       * DAC widget is playing the same stream.
       *
       * Analog front output (widget 0x14) will be automatically muted.
       */
      if (endpoint->is_digital)
	{
	  unsigned int v, b;

	  hdaudio_codec_setup_endpoint (mixer, &mixer->outendpoints[0], rate,
					channels, fmt, stream_number, &tmp);

	  if (fmt == AFMT_AC3)
	    if (corb_read
		(mixer, endpoint->cad, 0x14, 0, GET_GAIN (1, 0), 0, &v, &b))
	      {
		endpoint->auto_muted = !(v & 0x80);

		v |= 0x80;	/* Mute */
		corb_write (mixer, endpoint->cad, 0x14, 0,
			    SET_GAIN (1, 0, 1, 1, 0), v);
	      }
	}
    }
#endif

  return 0;
}

int
hdaudio_codec_reset_endpoint (hdaudio_mixer_t * mixer,
			      hdaudio_endpointinfo_t * endpoint, int channels)
{

  int i;
  unsigned int v, b;
  int n = (channels + 1) / 2;

  /*
   * Set all converters to play stream iddle stream (usually 0=silence).
   */

  corb_write (mixer, endpoint->cad, endpoint->base_wid, 0, SET_CONVERTER,
	      endpoint->iddle_stream << 4);

#if 1
  // TODO: Test this
  if (channels > 2)
    for (i = 1; i < n; i++)
      {
	hdaudio_endpointinfo_t *ep;

	ep = &endpoint[i];

	corb_write (mixer, ep->cad, ep->base_wid, 0, SET_CONVERTER,
		    ep->iddle_stream << 4);
      }
#endif

  if (mixer->codecs[endpoint->cad]->vendor_flags & VF_VAIO_HACK)
    /* Also set headphone DAC to play the iddle stream */
    if (channels <= 2 && endpoint->base_wid == 0x05)
      {
	corb_write (mixer, endpoint->cad, 0x02, 0, SET_CONVERTER,
		    endpoint->iddle_stream << 4);
      }

  if (mixer->codecs[endpoint->cad]->vendor_flags & VF_ALC88X_HACK)
    if (endpoint->is_digital && endpoint->auto_muted)	/* Restore automatic analog mute back to normal */
      {
	if (corb_read
	    (mixer, endpoint->cad, 0x14, 0, GET_GAIN (1, 0), 0, &v, &b))
	  {
	    v &= ~0x80;		/* Unmute */
	    corb_write (mixer, endpoint->cad, 0x14, 0,
			SET_GAIN (1, 0, 1, 1, 0), v);
	    endpoint->auto_muted = 0;
	  }
      }

  return 0;
}

/*ARGSUSED*/
void
hda_codec_unsol (hdaudio_mixer_t * mixer, unsigned int upper,
		 unsigned int lower)
{
  DDB (cmn_err (CE_CONT, "Unsol event %08x %08x\n", upper, lower));
}

int
hda_codec_getname (hdaudio_mixer_t * mixer, hda_name_t * name)
{
  widget_t *widget;

  if (name->cad >= mixer->ncodecs)
    return OSS_EIO;
  if (mixer->codecs[name->cad] == &NULL_codec)
    return OSS_EIO;
#if 1
  if (name->wid >= mixer->codecs[name->cad]->nwidgets)
    return OSS_EIO;
#endif

  widget = &mixer->codecs[name->cad]->widgets[name->wid];
  strcpy (name->name, widget->name);

  return 0;
}

int
hda_codec_getwidget (hdaudio_mixer_t * mixer, hda_widget_info_t * info)
{
  widget_t *widget;

  if (info->cad >= mixer->ncodecs)
    return OSS_EIO;
  if (mixer->codecs[info->cad] == &NULL_codec)
    return OSS_EIO;

  widget = &mixer->codecs[info->cad]->widgets[info->wid];
  if (info->wid >= mixer->codecs[info->cad]->nwidgets)
     return OSS_EIO;
  if (widget == NULL)
     return OSS_EIO;
  memcpy (info->info, widget, sizeof (*widget));

  return 0;
}

int
hdaudio_codec_audio_ioctl (hdaudio_mixer_t * mixer,
			   hdaudio_endpointinfo_t * endpoint,
			   unsigned int cmd, ioctl_arg arg)
{
  //widget_t *base_widget = &mixer->codecs[endpoint->cad]->widgets[endpoint->base_wid];
  widget_t *recsrc_widget =
    &mixer->codecs[endpoint->cad]->widgets[endpoint->recsrc_wid];
  widget_t *volume_widget =
    &mixer->codecs[endpoint->cad]->widgets[endpoint->volume_wid];
  char tmp[128], *t;
  unsigned int linked_wid, a, b;
  int i, v, left, right;
  int nsteps;

  if (mixer->codecs[endpoint->cad]->vendor_flags & VF_VAIO_HACK)
    linked_wid = (endpoint->volume_wid == 0x02) ? 0x05 :
		  ((endpoint->volume_wid == 0x05) ? 0x02 : 0);
  else 
    linked_wid = 0;

  switch (cmd)
    {
    case SNDCTL_DSP_GET_RECSRC_NAMES:
      *tmp = 0;
      t = tmp;

      for (i = 0; i < recsrc_widget->nconn; i++)
	{
	  if (*tmp)		/* Not empty */
	    *t++ = ' ';
	  strcpy (t,
		  mixer->codecs[recsrc_widget->cad]->widgets[recsrc_widget->
							     connections[i]].
		  name);
	  t += strlen (t);
	}
      return oss_encode_enum ((oss_mixer_enuminfo *) arg, tmp, 0);
      break;

    case SNDCTL_DSP_GET_RECSRC:
      if (!corb_read
	  (mixer, recsrc_widget->cad, recsrc_widget->wid, 0, GET_SELECTOR, 0,
	   &a, &b))
	return OSS_EIO;
      return *arg = a;
      break;

    case SNDCTL_DSP_SET_RECSRC:
      a = *arg;
      if (a > recsrc_widget->nconn)
	return OSS_EIO;

      corb_write (mixer, recsrc_widget->cad, recsrc_widget->wid, 0,
		  SET_SELECTOR, a);
      recsrc_widget->current_selector = a;
      mixer_devs[mixer->mixer_dev]->modify_counter++;
      return *arg = a;
      break;

    case SNDCTL_DSP_GETRECVOL:
      nsteps = (volume_widget->inamp_caps >> 8) & 0x3f;
      if (nsteps < 1)
	nsteps = 1;
      if (!corb_read
	  (mixer, volume_widget->cad, volume_widget->wid, 0, GET_GAIN (0, 1),
	   0, &a, &b))
	return OSS_EIO;
      if (a & 0x80)		/* Muted */
	left = 0;
      else
	left = ((a & 0x7f) * 100) / nsteps;
      if (!corb_read
	  (mixer, volume_widget->cad, volume_widget->wid, 0, GET_GAIN (0, 0),
	   0, &a, &b))
	return OSS_EIO;
      if (a & 0x80)		/* Muted */
	right = 0;
      else
	right = ((a & 0x7f) * 100) / nsteps;
      v = left | (right << 8);
      return *arg = v;
      break;

    case SNDCTL_DSP_SETRECVOL:
      v = *arg;

      left = v & 0xff;
      right = (v >> 8) & 0xff;

      if (left < 0)
	left = 0;
      if (left > 100)
	left = 100;
      if (right < 0)
	right = 0;
      if (right > 100)
	right = 100;
      v = left | (right << 8);

      nsteps = (volume_widget->inamp_caps >> 8) & 0x3f;
      if (nsteps < 1)
	nsteps = 1;

      a = (left * nsteps) / 100;
      if (left == 0)
	a |= 0x80;		/* Mute */
      corb_write (mixer, volume_widget->cad, volume_widget->wid, 0,
		  SET_GAIN (0, 1, 1, 0, 0), a);
      a = (right * nsteps) / 100;
      if (right == 0)
	a |= 0x80;		/* Mute */
      corb_write (mixer, volume_widget->cad, volume_widget->wid, 0,
		  SET_GAIN (0, 1, 0, 1, 0), a);

      mixer_devs[mixer->mixer_dev]->modify_counter++;
      return *arg = v;
      break;

    case SNDCTL_DSP_GETPLAYVOL:
      nsteps = (volume_widget->outamp_caps >> 8) & 0x3f;
      if (nsteps < 1)
	nsteps = 1;
      if (!corb_read
	  (mixer, volume_widget->cad, volume_widget->wid, 0, GET_GAIN (1, 1),
	   0, &a, &b))
	return OSS_EIO;
      if (a & 0x80)		/* Muted */
	left = 0;
      else
	left = ((a & 0x7f) * 100) / nsteps;
      if (!corb_read
	  (mixer, volume_widget->cad, volume_widget->wid, 0, GET_GAIN (1, 0),
	   0, &a, &b))
	return OSS_EIO;
      if (a & 0x80)		/* Muted */
	right = 0;
      else
	right = ((a & 0x7f) * 100) / nsteps;
      v = left | (right << 8);
      return *arg = v;
      break;

    case SNDCTL_DSP_SETPLAYVOL:
      v = *arg;

      left = v & 0xff;
      right = (v >> 8) & 0xff;

      if (left < 0)
	left = 0;
      if (left > 100)
	left = 100;
      if (right < 0)
	right = 0;
      if (right > 100)
	right = 100;
      v = left | (right << 8);

      nsteps = (volume_widget->outamp_caps >> 8) & 0x3f;
      if (nsteps < 1)
	nsteps = 1;

      a = (left * nsteps) / 100;
      if (left == 0)
	a |= 0x80;		/* Mute */
      corb_write (mixer, volume_widget->cad, volume_widget->wid, 0,
		  SET_GAIN (1, 0, 1, 0, 0), a);
      if (linked_wid)
	 corb_write (mixer, volume_widget->cad, linked_wid, 0,
		     SET_GAIN (1, 0, 1, 0, 0), a);
      a = (right * nsteps) / 100;
      if (right == 0)
	a |= 0x80;		/* Mute */
      corb_write (mixer, volume_widget->cad, volume_widget->wid, 0,
		  SET_GAIN (1, 0, 0, 1, 1), a);
      if (linked_wid)
	 corb_write (mixer, volume_widget->cad, linked_wid, 0,
		     SET_GAIN (1, 0, 0, 1, 1), a);

      mixer_devs[mixer->mixer_dev]->modify_counter++;
      return *arg = v;
      break;

    case SNDCTL_DSP_MODEM_OFFHOOK:
      if (!endpoint->is_modem)
        {
          return OSS_EINVAL;
        }
      v = *arg;
      if (mixer->codecs[endpoint->cad]->vendor_flags & VF_SI3055_HACK)
        {
          v = hdaudio_si3055_set_offhook(mixer, endpoint->cad, v);
        }
      else
        {
          return OSS_ENOTSUP;
        }
      return *arg = v;
      break;
    }

  return OSS_EINVAL;
}

/*
 * Support routines for dedicated mixer drivers
 */

void
hda_codec_add_group (int dev, hdaudio_mixer_t * mixer, int cad, int *group,
		     int parent_group, const char *name)
{
  int grp;

  if ((grp = mixer_ext_create_group (dev, parent_group, name)) > 0)
    *group = grp;
}

int
hda_codec_add_pingroup (int dev, hdaudio_mixer_t * mixer, int cad, int wid,
			int *group, int top_group, int *parent_group,
			const char *name, int *n, const char *parent_name,
			int group_size)
{
  int grp;
  widget_t *widget;
  codec_t *codec;

  if (cad < 0 || cad >= mixer->ncodecs)
    return OSS_ENXIO;
  codec = mixer->codecs[cad];

  if (wid < 0 || wid >= codec->nwidgets)
    return OSS_ENXIO;

  widget = &codec->widgets[wid];
  if (widget->used || widget->skip)
    return 0;

  if (group_size <= 1 || ((*n % group_size) == 0 && *n > 0))
    {
      if ((grp = mixer_ext_create_group (dev, top_group, parent_name)) > 0)
	*parent_group = grp;
      *n = 0;
    }

  if ((grp = mixer_ext_create_group (dev, *parent_group, name)) > 0)
    *group = grp;
  (*n)++;

  return 1;
}

int
hda_codec_add_adcgroup (int dev, hdaudio_mixer_t * mixer, int cad, int wid,
			int *group, int top_group, int *parent_group,
			const char *name, int *n, const char *parent_name,
			int group_size)
{
  int grp;
  widget_t *widget;
  codec_t *codec;

  if (cad < 0 || cad >= mixer->ncodecs)
    return OSS_ENXIO;
  codec = mixer->codecs[cad];

  if (wid < 0 || wid >= codec->nwidgets)
    return OSS_ENXIO;

  widget = &codec->widgets[wid];
  if (widget->used || widget->skip)
    return 0;

  if (group_size <= 1 || ((*n % group_size) == 0 && *n > 0))
    {
      if ((grp = mixer_ext_create_group (dev, top_group, parent_name)) > 0)
	*parent_group = grp;
      *n = 0;
    }

  if ((grp = mixer_ext_create_group (dev, *parent_group, name)) > 0)
    *group = grp;
  (*n)++;

  return 1;
}

int
hda_codec_add_miscgroup (int dev, hdaudio_mixer_t * mixer, int cad, int wid,
			 int *group, int top_group, int *parent_group,
			 const char *name, int *n, const char *parent_name,
			 int group_size)
{
  int grp;
  widget_t *widget;
  codec_t *codec;

  if (cad < 0 || cad >= mixer->ncodecs)
    return OSS_ENXIO;
  codec = mixer->codecs[cad];

  if (wid < 0 || wid >= codec->nwidgets)
    return OSS_ENXIO;

  widget = &codec->widgets[wid];
  if (widget->used || widget->skip)
    return 0;

  if (group_size <= 1 || ((*n % group_size) == 0 && *n > 0))
    {
      if ((grp = mixer_ext_create_group (dev, top_group, parent_name)) > 0)
	*parent_group = grp;
      *n = 0;
    }

  if ((grp = mixer_ext_create_group (dev, *parent_group, name)) > 0)
    *group = grp;
  (*n)++;

  return 1;
}

int
create_outgain_selector (hdaudio_mixer_t * mixer, widget_t * widget,
			 int group, const char *name)
{
  int num = MIXNUM (widget, CT_OUTGAINSEL, 0);
  int ctl, i;
  int maxval;
  int offs, step, range;
  oss_mixext *ent;

  char tmp[128], *t;

  range =
    ((widget->outamp_caps >> AMPCAP_NUMSTEPS_SHIFT) & AMPCAP_NUMSTEPS_MASK) +
    1;
  step =
    ((widget->outamp_caps >> AMPCAP_STEPSIZE_SHIFT) & AMPCAP_STEPSIZE_MASK) +
    1;
  offs = (widget->outamp_caps >> AMPCAP_OFFSET_SHIFT) & AMPCAP_OFFSET_MASK;

  maxval = range;

  if (widget->outamp_caps & AMPCAP_MUTE)
    maxval += 1;

  if ((ctl = mixer_ext_create_control (mixer->mixer_dev,
				       group,
				       num, hdaudio_set_control,
				       MIXT_ENUM,
				       name, maxval,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  t = tmp;
  *t = 0;

  for (i = 0; i < range; i++)
    {
      int v;

      v = (i - offs) * step * 4;

      if (*tmp != 0)
	*t++ = ' ';

      sprintf (t, "%d.%ddB", v / 10, v % 10);
      t += strlen (t);
    }

  if (widget->outamp_caps & AMPCAP_MUTE)
    {
      if (*tmp != 0)
	*t++ = ' ';
      strcpy (t, "mute");
      t += strlen (t);
    }

  mixer_ext_set_strings (mixer->mixer_dev, ctl, tmp, 0);
  /* Copy RGB color */
  if (widget->rgbcolor != 0)
      if ((ent = mixer_find_ext (mixer->mixer_dev, ctl)) != NULL)
	 ent->rgbcolor = widget->rgbcolor;

  return 0;
}

int
create_ingain_selector (hdaudio_mixer_t * mixer, codec_t * codec,
			widget_t * widget, int group, int ix,
			const char *name)
{
  int num = MIXNUM (widget, CT_INGAINSEL, ix);

  int ctl, i;
  int maxval;
  int offs, step, range;
  oss_mixext *ent;

  char tmp[128], *t;

  range =
    ((widget->inamp_caps >> AMPCAP_NUMSTEPS_SHIFT) & AMPCAP_NUMSTEPS_MASK) +
    1;
  step =
    ((widget->inamp_caps >> AMPCAP_STEPSIZE_SHIFT) & AMPCAP_STEPSIZE_MASK) +
    1;
  offs = (widget->inamp_caps >> AMPCAP_OFFSET_SHIFT) & AMPCAP_OFFSET_MASK;

  maxval = range;

  if (widget->inamp_caps & AMPCAP_MUTE)
    maxval += 1;

  if ((ctl = mixer_ext_create_control (mixer->mixer_dev,
				       group,
				       num, hdaudio_set_control,
				       MIXT_ENUM,
				       name, maxval,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  t = tmp;
  *t = 0;

  for (i = 0; i < range; i++)
    {
      int v;

      v = (i - offs) * step * 4;

      if (*tmp != 0)
	*t++ = ' ';

      sprintf (t, "%d.%ddB", v / 10, v % 10);
      t += strlen (t);
    }

  if (widget->inamp_caps & AMPCAP_MUTE)
    {
      if (*tmp != 0)
	*t++ = ' ';
      strcpy (t, "mute");
      t += strlen (t);
    }

  mixer_ext_set_strings (mixer->mixer_dev, ctl, tmp, 0);
  /* Copy RGB color */
  if (widget->color != 0)
     if ((ent = mixer_find_ext (mixer->mixer_dev, ctl)) != NULL)
	 ent->rgbcolor = widget->rgbcolor;
  return 0;
}

int
hda_codec_add_outamp (int dev, hdaudio_mixer_t * mixer, int cad, int wid,
		      int group, const char *name, int percent,
		      unsigned int flags)
{
  widget_t *widget;
  codec_t *codec;
  int typ, num, maxval, val, ctl = 0;
  int range, step;
  oss_mixext *ent;
  extern int mixer_muted;

  if (cad < 0 || cad >= mixer->ncodecs)
    return OSS_ENXIO;
  codec = mixer->codecs[cad];

  if (wid < 0 || wid >= codec->nwidgets)
    return OSS_ENXIO;

  widget = &codec->widgets[wid];

  range =
    ((widget->
      outamp_caps >> AMPCAP_NUMSTEPS_SHIFT) & AMPCAP_NUMSTEPS_MASK) + 1;
  step =
    ((widget->
      outamp_caps >> AMPCAP_STEPSIZE_SHIFT) & AMPCAP_STEPSIZE_MASK) + 1;

  if (step > 20 /* 5dB */  && range < 5)
    {
      create_outgain_selector (mixer, widget, group, name);
    }
  else
    {

      maxval = hdaudio_amp_maxval (widget->outamp_caps);

      if (widget->widget_caps & WCAP_STEREO)
	{
	  typ = MIXT_STEREOSLIDER16;
	  num = MIXNUM (widget, CT_OUTSTEREO, 0);
	}
      else
	{
	  typ = MIXT_MONOSLIDER16;
	  num = MIXNUM (widget, CT_OUTMONO, 0);
	}

      if ((ctl = mixer_ext_create_control (mixer->mixer_dev,
					   group,
					   num, hdaudio_set_control,
					   typ,
					   name, maxval,
					   flags | MIXF_READABLE |
					   MIXF_WRITEABLE |
					   MIXF_CENTIBEL)) < 0)
	return ctl;

      /* setup volume */
      val = (maxval * percent) / 100;
      val = val | (val << 16);
      if (mixer_muted)
	 val = 0;
      /* Copy RGB color */
      if (widget->rgbcolor != 0)
      if ((ent = mixer_find_ext (dev, ctl)) != NULL)
	 ent->rgbcolor = widget->rgbcolor;
      hdaudio_set_control (mixer->mixer_dev, num, SNDCTL_MIX_WRITE, val);
    }

  return ctl;
}

int
hda_codec_add_outmute (int dev, hdaudio_mixer_t * mixer, int cad, int wid,
		       int group, const char *name, int muted)
{
  widget_t *widget;
  codec_t *codec;
  int ctl = 0;
  oss_mixext *ent;

  if (cad < 0 || cad >= mixer->ncodecs)
    return OSS_ENXIO;
  codec = mixer->codecs[cad];

  if (wid < 0 || wid >= codec->nwidgets)
    return OSS_ENXIO;

  widget = &codec->widgets[wid];

  if (widget->outamp_caps & AMPCAP_MUTE)	/* Only mute control */
    {
      // name = "mute";

      if ((ctl = mixer_ext_create_control (mixer->mixer_dev,
					   group,
					   MIXNUM (widget,
						   CT_OUTMUTE, 0),
					   hdaudio_set_control,
					   MIXT_MUTE, name, 2,
					   MIXF_READABLE |
					   MIXF_WRITEABLE)) < 0)
	return ctl;
      /* Copy RGB color */
      if (widget->rgbcolor != 0)
      if ((ent = mixer_find_ext (dev, ctl)) != NULL)
	 ent->rgbcolor = widget->rgbcolor;

      hdaudio_set_control (mixer->mixer_dev,
			   MIXNUM (widget, CT_OUTMUTE, 0),
			   SNDCTL_MIX_WRITE, muted);
      return ctl;
    }
  return 0;
}

int
hda_codec_add_inamp (int dev, hdaudio_mixer_t * mixer, int cad, int wid,
		     int ix, int group, const char *name, int percent, int flags)
{
  widget_t *widget;
  widget_t *src_widget;
  codec_t *codec;
  int typ, num, maxval, val, ctl = 0, range, step;
  oss_mixext *ent;
  extern int mixer_muted;

  if (cad < 0 || cad >= mixer->ncodecs)
    return OSS_ENXIO;
  codec = mixer->codecs[cad];

  if (wid < 0 || wid >= codec->nwidgets)
    return OSS_ENXIO;

  widget = &codec->widgets[wid];

  range =
    ((widget->
      outamp_caps >> AMPCAP_NUMSTEPS_SHIFT) & AMPCAP_NUMSTEPS_MASK) + 1;
  step =
    ((widget->
      outamp_caps >> AMPCAP_STEPSIZE_SHIFT) & AMPCAP_STEPSIZE_MASK) + 1;

  if (step > 20 /* 5dB */  && range < 5)
    {
      return create_ingain_selector (mixer, codec, widget, group, ix, name);
    }
  maxval = hdaudio_amp_maxval (widget->inamp_caps);

  if (widget->widget_caps & WCAP_STEREO)
    {
      typ = MIXT_STEREOSLIDER16;
      num = MIXNUM (widget, CT_INSTEREO, ix);
    }
  else
    {
      typ = MIXT_MONOSLIDER16;
      num = MIXNUM (widget, CT_INMONO, ix);
    }

  if (codec->widgets[widget->connections[ix]].skip)
    {
      hdaudio_set_control (mixer->mixer_dev, num, SNDCTL_MIX_WRITE, 0);
      return 0;
    }

  if ((ctl = mixer_ext_create_control (mixer->mixer_dev,
				       group,
				       num,
				       hdaudio_set_control,
				       typ,
				       name, maxval,
				       MIXF_READABLE |
				       MIXF_WRITEABLE | MIXF_CENTIBEL | flags)) < 0)
    return ctl;

  /* Setup initial volume */
  val = (maxval * percent) / 100;
  val = val | (val << 16);
  if (mixer_muted)
     val = 0;

  hdaudio_set_control (mixer->mixer_dev, num, SNDCTL_MIX_WRITE, val);

  /* Copy RGB color */
  src_widget = &codec->widgets[widget->connections[ix]];
  if (src_widget->rgbcolor != 0)
      if ((ent = mixer_find_ext (dev, ctl)) != NULL)
	 ent->rgbcolor = src_widget->rgbcolor;

  return ctl;
}

int
hda_codec_add_inmute (int dev, hdaudio_mixer_t * mixer, int cad, int wid,
		      int ix, int group, const char *name, int muted, unsigned int flags)
{
  widget_t *widget;
  codec_t *codec;
  oss_mixext *ent;
  int ctl = 0;

  if (cad < 0 || cad >= mixer->ncodecs)
    return OSS_ENXIO;
  codec = mixer->codecs[cad];

  if (wid < 0 || wid >= codec->nwidgets)
    return OSS_ENXIO;

  widget = &codec->widgets[wid];

  if (codec->widgets[widget->connections[ix]].skip)
    {
      int num = MIXNUM (widget, CT_INMUTE, ix);
      hdaudio_set_control (mixer->mixer_dev, num, SNDCTL_MIX_WRITE, 1);
      return 0;
    }

  if ((ctl = mixer_ext_create_control (mixer->mixer_dev,
				       group,
				       MIXNUM (widget,
					       CT_INMUTE, ix),
				       hdaudio_set_control,
				       MIXT_MUTE, name, 2,
				       flags | MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;
  /* Copy RGB color */
  if (widget->rgbcolor != 0)
      if ((ent = mixer_find_ext (dev, ctl)) != NULL)
	 ent->rgbcolor = widget->rgbcolor;

  hdaudio_set_control (mixer->mixer_dev,
		       MIXNUM (widget, CT_INMUTE, ix),
		       SNDCTL_MIX_WRITE, muted);
  return ctl;
}

int
hda_codec_set_inmute (int dev, hdaudio_mixer_t * mixer, int cad, int wid,
		      int ix, int group, const char *name, int muted)
{
  widget_t *widget;
  codec_t *codec;

  if (cad < 0 || cad >= mixer->ncodecs)
    return OSS_ENXIO;
  codec = mixer->codecs[cad];

  if (wid < 0 || wid >= codec->nwidgets)
    return OSS_ENXIO;

  widget = &codec->widgets[wid];

// cmn_err(CE_CONT, "Set inmute 0x%02x:%d=%d\n", wid, ix, muted);
  return hdaudio_set_control (mixer->mixer_dev,
			      MIXNUM (widget, CT_INMUTE, ix),
			      SNDCTL_MIX_WRITE, muted);
}

int
hda_codec_add_insrc (int dev, hdaudio_mixer_t * mixer, int cad, int wid,
		     int ix, int group, const char *name, int unselected)
{
  widget_t *widget;
  codec_t *codec;
  oss_mixext *ent;
  int ctl = 0;

  if (cad < 0 || cad >= mixer->ncodecs)
    return OSS_ENXIO;
  codec = mixer->codecs[cad];

  if (wid < 0 || wid >= codec->nwidgets)
    return OSS_ENXIO;

  widget = &codec->widgets[wid];

  if (codec->widgets[widget->connections[ix]].skip)
    {
      int num = MIXNUM (widget, CT_INMUTE, ix);

      hdaudio_set_control (mixer->mixer_dev, num, SNDCTL_MIX_WRITE, 1);
      return 0;
    }

  if ((ctl = mixer_ext_create_control (mixer->mixer_dev,
				       group,
				       MIXNUM (widget,
					       CT_INSRC, ix),
				       hdaudio_set_control,
				       MIXT_ONOFF, name, 2,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;
  /* Copy RGB color */
  if (widget->rgbcolor != 0)
     if ((ent = mixer_find_ext (dev, ctl)) != NULL)
	 ent->rgbcolor = widget->rgbcolor;

  hdaudio_set_control (mixer->mixer_dev,
		       MIXNUM (widget, CT_INSRC, ix),
		       SNDCTL_MIX_WRITE, unselected);
  return ctl;
}

int
hda_codec_add_insrcselect (int dev, hdaudio_mixer_t * mixer, int cad, int wid,
			   int group, int *ctl, const char *name,
			   int initial_selection)
{
  widget_t *widget;
  codec_t *codec;
  int i;
  oss_mixext *ext;

  *ctl = 0;

  if (cad < 0 || cad >= mixer->ncodecs)
    return OSS_ENXIO;
  codec = mixer->codecs[cad];

  if (wid < 0 || wid >= codec->nwidgets)
    return OSS_ENXIO;

  widget = &codec->widgets[wid];

  if ((*ctl = mixer_ext_create_control (mixer->mixer_dev,
					group,
					MIXNUM (widget,
						CT_INSRCSELECT, 0),
					hdaudio_set_control,
					MIXT_ENUM, name, widget->nconn,
					MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return 0;

  ext = mixer_find_ext (mixer->mixer_dev, *ctl);
  if (ext == NULL)
    {
      cmn_err (CE_WARN, "Cannot locate the mixer extension (x)\n");
      return OSS_EIO;
    }

  /* Copy RGB color */
  if (widget->color != 0)
     ext->rgbcolor = widget->rgbcolor;

  memset (ext->enum_present, 0, sizeof (ext->enum_present));

  for (i = 0; i < widget->nconn; i++)
    {

      /*
       * ensure that the connection list has a valid widget id - some
       * devices have bogus connection lists 
       */
      if (codec->widgets[widget->connections[i]].wid < codec->first_node)
	continue;

      /*
       * Show only widgets that are not marked to be ignored.
       * Also hide I/O pins that are known to be outputs.
       */
      if (!codec->widgets[widget->connections[i]].skip
	  && codec->widgets[widget->connections[i]].sensed_pin != PIN_OUT)
	ext->enum_present[i / 8] |= (1 << (i % 8));
    }

  hdaudio_set_control (mixer->mixer_dev,
		       MIXNUM (widget, CT_INSRCSELECT, 0),
		       SNDCTL_MIX_WRITE, initial_selection);
  return 1;
}

int
hda_codec_add_select (int dev, hdaudio_mixer_t * mixer, int cad, int wid,
		      int group, const char *name, int *ctl,
		      int initial_select)
{
  widget_t *widget;
  codec_t *codec;
  oss_mixext *ext;
  int i;

  *ctl = 0;

  if (cad < 0 || cad >= mixer->ncodecs)
    return OSS_ENXIO;
  codec = mixer->codecs[cad];

  if (wid < 0 || wid >= codec->nwidgets)
    return OSS_ENXIO;

  widget = &codec->widgets[wid];

  if ((*ctl = mixer_ext_create_control (mixer->mixer_dev,
					group,
					MIXNUM (widget, CT_SELECT, 0),
					hdaudio_set_control,
					MIXT_ENUM,
					name,
					widget->nconn,
					MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return *ctl;

  ext = mixer_find_ext (mixer->mixer_dev, *ctl);

  if (ext == NULL)
    {
      cmn_err (CE_WARN, "Cannot locate the mixer extension (x)\n");
      return OSS_EIO;
    }
  /* Copy RGB color */
  if (widget->color != 0)
     ext->rgbcolor = widget->rgbcolor;

  memset (ext->enum_present, 0, sizeof (ext->enum_present));

  for (i = 0; i < widget->nconn; i++)
    {

      /*
       * ensure that the connection list has a valid widget id - some
       * devices have bogus connection lists 
       */
      if (codec->widgets[widget->connections[i]].wid < codec->first_node)
	continue;

      /*
       * Show only widgets that are not marked to be ignored.
       * Also hide I/O pins that are known to be outputs.
       */
      if (!codec->widgets[widget->connections[i]].skip
	  && codec->widgets[widget->connections[i]].sensed_pin != PIN_OUT)
	ext->enum_present[i / 8] |= (1 << (i % 8));
    }

  if (initial_select > -1)
    widget->current_selector = initial_select;

  if (widget->current_selector >= widget->nconn)
    widget->current_selector = 0;
  corb_write (mixer, widget->cad, widget->wid, 0, SET_SELECTOR,
	      widget->current_selector);

  return *ctl;
}

int
hda_codec_add_pinselect (int dev, hdaudio_mixer_t * mixer, int cad, int wid,
			 int group, const char *name, int *ctl,
			 int initial_select)
{
  widget_t *widget;
  codec_t *codec;
  unsigned int conf, b;
  oss_mixext *ext;
  int i;

  *ctl = 0;

  if (cad < 0 || cad >= mixer->ncodecs)
    return OSS_ENXIO;
  codec = mixer->codecs[cad];

  if (wid < 0 || wid >= codec->nwidgets)
    return OSS_ENXIO;

  widget = &codec->widgets[wid];

  if ((*ctl = mixer_ext_create_control (mixer->mixer_dev,
					group,
					MIXNUM (widget, CT_SELECT, 0),
					hdaudio_set_control,
					MIXT_ENUM,
					name,
					widget->nconn + 1,
					MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return *ctl;

  ext = mixer_find_ext (mixer->mixer_dev, *ctl);

  if (ext == NULL)
    {
      cmn_err (CE_WARN, "Cannot locate the mixer extension (x)\n");
      return OSS_EIO;
    }

  /* Copy RGB color */
  if (widget->color != 0)
     ext->rgbcolor = widget->rgbcolor;
  memset (ext->enum_present, 0, sizeof (ext->enum_present));

  for (i = 0; i < widget->nconn; i++)
    {

      /*
       * ensure that the connection list has a valid widget id - some
       * devices have bogus connection lists 
       */
      if (codec->widgets[widget->connections[i]].wid < codec->first_node)
	continue;

      /*
       * Show only widgets that are not marked to be ignored.
       * Also hide I/O pins that are known to be outputs.
       */
      if (!codec->widgets[widget->connections[i]].skip
	  && codec->widgets[widget->connections[i]].sensed_pin != PIN_OUT)
	ext->enum_present[i / 8] |= (1 << (i % 8));
    }

  /*
   * Enable the input selection (if available)
   */
  i = widget->nconn;
  if (widget->pincaps & PINCAP_INPUT_CAPABLE)
    ext->enum_present[i / 8] |= (1 << (i % 8));

/*
 * Set the initial value.
 *
 * Use the default sequence as an index to the output source selectors.
 */
  if (widget->sensed_pin == PIN_OUT)
    {
      if (corb_read
	  (mixer, widget->cad, widget->wid, 0, GET_CONFIG_DEFAULT, 0, &conf,
	   &b))
	{
	  int association, sequence;

	  association = (conf >> 4) & 0x0f;
	  sequence = conf & 0x0f;

	  if (association != 0)
	    {
	      widget->current_selector = sequence;
	    }

	}
    }
  else if (widget->sensed_pin == PIN_IN || widget->sensed_pin == PIN_MIC)
    widget->current_selector = widget->nconn;	/* Turn on input mode */

  if (initial_select > -1)
    widget->current_selector = initial_select;

  if (widget->current_selector < 0
      || widget->current_selector >= widget->nconn + 1)
    widget->current_selector = 0;

  if (widget->current_selector < widget->nconn)
    {
      /* Output source select */
      corb_write (mixer, cad, wid, 0, SET_SELECTOR, widget->current_selector);
      /* Enable output and HP amp. Set vref=Ground */
      corb_write (mixer, cad, wid, 0, SET_PINCTL, 0xc0);
    }
  else
    {
      /* Input select
       * Program the correct VRef Values
       */

      if (widget->pin_type == PIN_IN)	/* Line-in */
	{
	  corb_write (mixer, cad, wid, 0, SET_PINCTL, 0x20);	/*Ground*/
	}
      else		/* Mic-in */
	{
	  corb_write (mixer, cad, wid, 0, SET_PINCTL, 0x24);	/*Vref=8
								   0% */
	}
    }

  return *ctl;
}

void
hda_codec_set_select (int dev, hdaudio_mixer_t * mixer, int cad, int wid,
		      int value)
{
  codec_t *codec;
  widget_t *widget;

  if (cad < 0 || cad >= mixer->ncodecs)
    return;
  codec = mixer->codecs[cad];

  if (wid < 0 || wid >= codec->nwidgets)
    return;

  widget = &codec->widgets[wid];

  widget->current_selector = value;

  corb_write (mixer, cad, wid, 0, SET_SELECTOR, value);
}

void
hda_codec_set_pinselect (int dev, hdaudio_mixer_t * mixer, int cad, int wid,
		      int value)
{
  codec_t *codec;
  widget_t *widget;

  if (cad < 0 || cad >= mixer->ncodecs)
    return;
  codec = mixer->codecs[cad];

  if (wid < 0 || wid >= codec->nwidgets)
    return;

  widget = &codec->widgets[wid];

  widget->current_selector = value;

  if (widget->current_selector < widget->nconn)
    {
      /* Output source select */
      corb_write (mixer, cad, wid, 0, SET_SELECTOR, widget->current_selector);
      /* Enable output and HP amp. Set vref=Ground */
      corb_write (mixer, cad, wid, 0, SET_PINCTL, 0xc0);
    }
  else
    {
      /* Input select
       * Program the correct VRef Values
       */

      if (widget->pin_type == PIN_IN)	/* Line-in */
	{
	  corb_write (mixer, cad, wid, 0, SET_PINCTL, 0x20);	/*Ground*/
	}
      else		/* Mic-in */
	{
	  corb_write (mixer, cad, wid, 0, SET_PINCTL, 0x24);	/*Vref=8
								   0% */
	}
    }
}

int
hda_codec_add_choices (int dev, hdaudio_mixer_t * mixer, int ctl,
		       const char *choiselist)
{
  mixer_ext_set_strings (dev, ctl, choiselist, 0);

  return 0;
}

void
hda_codec_set_color(int dev, hdaudio_mixer_t *mixer, int ctl, int color)
{
  oss_mixext *ext;

  if ((ext = mixer_find_ext (mixer->mixer_dev, ctl)) != NULL)
  {
     ext->rgbcolor = color;
//cmn_err(CE_CONT, "Mixer %s rgb->%06x\n", ext->extname, ext->rgbcolor);
  }
}
