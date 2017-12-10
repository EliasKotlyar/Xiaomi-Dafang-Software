/*
 * Purpose: Default mixer/control panel driver for HDA codecs
 *
 * This generic driver is used to create mixer/control panels for HDaudio
 * codec chips that don't have any dedicated driver available.
 *
 * This driver will obtain the widget definitions from the codec and then
 * try to guess a mixer layout that makes some sense. However this approach
 * works properly only with a small set of codecs.
 *
 * Most codecs are unbearably complex and provide loads of redundant
 * functionality. The generic driver approach will not properly work with them
 * because the mixer (GUI) layout will become too large to fit on any screen.
 * In addition such automatically generated mixer controls will not make any
 * sense to the users. So in the future the only possible approach will be
 * creating dedicated mixer drivers for all possible codecs in the market.
 * Unfortunately in some cases the driver may even need to be motherboard
 * specific. Apparently this is going to be enormous task.
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

extern int hdaudio_snoopy;
extern int hdaudio_jacksense;
extern int hdaudio_noskip;

static int
count_linked_controls (hdaudio_mixer_t * mixer, codec_t * codec,
		       widget_t * widget, int recursive)
{
/*
 * This function counts the number of mixer control elements this
 * widget has.
 * If recursive==1 then control counts of the previous widgets in the
 * processing chain will be counted if number of inputs is exactly 1.
 * Input sources are not checked if number of connections is larger than 1
 * because separate mixer group is required for such widgets.
 *
 * Note! The policies used by this function must match exactly the policies 
 *       used by follow_widget_chain()
 */
  int count = 0;

  if (widget->skip)
    return 0;

  /*
   * Output amp?
   */
  if (widget->widget_caps & WCAP_OUTPUT_AMP_PRESENT)
    count += 1;

  /*
   * Input amp(s)?
   */
  if (widget->widget_caps & WCAP_INPUT_AMP_PRESENT)
    {
      if (widget->wid_type == NT_MIXER)
	count += widget->nconn;
      else
	count++;
    }

  /*
   * Input selector?
   */
  if (widget->wid_type == NT_SELECT && widget->nconn > 1)
    count += 1;

  if (recursive)
    if (widget->nconn == 1)	/* Exactly one input wource */
      count +=
	count_linked_controls (mixer, codec,
			       &codec->widgets[widget->connections[0]],
			       recursive);

  return count;
}

/*ARGSUSED*/
static int
attach_amplifiers (int dev, hdaudio_mixer_t * mixer, codec_t * codec,
		   widget_t * widget, int group, int group_mode)
{
  int i, cnum, ninputs;
  int g = group;
  int use_mutegroup = 0;
  oss_mixext *ent;

/*
 * Control for input amplifier(s)
 */
  if (widget->widget_caps & WCAP_INPUT_AMP_PRESENT)
    {
      if (widget->wid_type == NT_MIXER)
	ninputs = widget->nconn;
      else
	ninputs = 1;

      /* 
       * Check if it's possible to save horizontal space by creating a separate 
       * mute group. In this way the names of mute selectors become shorter.
       */
      if (!(widget->inamp_caps & ~AMPCAP_MUTE)
	  && (widget->inamp_caps & AMPCAP_MUTE) && ninputs > 2)
	{
	  use_mutegroup = 1;
	  if ((g =
	       mixer_ext_create_group (mixer->mixer_dev, group, "mute")) < 0)
	    return g;
	}

      for (i = 0; i < ninputs; i++)	/* All inputs */
	{
	  char tmpname[32], tmp[40];
	  char *name = codec->widgets[widget->connections[i]].name;

	  if (ninputs == 1)	/* Hide name */
	    name = "-";

	  if (codec->widgets[widget->connections[i]].skip)
	    continue;

	  if (widget->inamp_caps & ~AMPCAP_MUTE)	/* Supports gain control */
	    {
	      int typ, num, maxval, val, range, step;
	      range =
		((widget->
		  outamp_caps >> AMPCAP_NUMSTEPS_SHIFT) &
		 AMPCAP_NUMSTEPS_MASK) + 1;
	      step =
		((widget->
		  outamp_caps >> AMPCAP_STEPSIZE_SHIFT) &
		 AMPCAP_STEPSIZE_MASK) + 1;

	      if (step > 20 /* 5dB */  && range < 5)
		{
		  create_ingain_selector (mixer, codec, widget, group, i,
					  name);
		  continue;
		}
	      maxval = hdaudio_amp_maxval (widget->inamp_caps);

	      if (widget->widget_caps & WCAP_STEREO)
		{
		  typ = MIXT_STEREOSLIDER16;
		  num = MIXNUM (widget, CT_INSTEREO, i);
		}
	      else
		{
		  typ = MIXT_MONOSLIDER16;
		  num = MIXNUM (widget, CT_INMONO, i);
		}

	      if (hdaudio_snoopy > 0)
		{
		  sprintf (tmp, "%s:R%x", name, widget->wid);
		  name = tmp;
		}

	      if ((cnum = mixer_ext_create_control (mixer->mixer_dev,
						   group,
						   num,
						   hdaudio_set_control,
						   typ,
						   name, maxval,
						   MIXF_READABLE |
						   MIXF_WRITEABLE |
						   MIXF_CENTIBEL)) < 0)
		return cnum;

	      /* Copy RGB color */
	      if (widget->rgbcolor != 0)
	      if ((ent = mixer_find_ext (dev, cnum)) != NULL)
		 ent->rgbcolor = widget->rgbcolor;

	      /* Setup initial volume */
	      val = (maxval * 8) / 10;	/* 80% of the maximum */
	      val = val | (val << 16);

	      hdaudio_set_control (mixer->mixer_dev, num, SNDCTL_MIX_WRITE,
				   val);
	      continue;		/* Skip to the next input */
	    }

	  if (widget->inamp_caps & AMPCAP_MUTE)	/* Supports only mute */
	    {
	      if (use_mutegroup)
		strcpy (tmpname, name);
	      else
		sprintf (tmpname, "%s-mute", name);
	      name = tmpname;

	      if (hdaudio_snoopy > 0)
		{
		  sprintf (tmp, "%s:Q%x", name, widget->wid);
		  name = tmp;
		}

	      if ((cnum = mixer_ext_create_control (mixer->mixer_dev,
						   g,
						   MIXNUM (widget,
							   CT_INMUTE, i),
						   hdaudio_set_control,
						   MIXT_MUTE, name, 2,
						   MIXF_READABLE |
						   MIXF_WRITEABLE)) < 0)
		return cnum;
	      /* Copy RGB color */
	      if (widget->rgbcolor != 0)
	      if ((ent = mixer_find_ext (dev, cnum)) != NULL)
		 ent->rgbcolor = widget->rgbcolor;

	      hdaudio_set_control (mixer->mixer_dev,
				   MIXNUM (widget, CT_INMUTE, i),
				   SNDCTL_MIX_WRITE, 0);
	    }

	}
    }

/*
 * Output amplifier control
 */

  if (widget->widget_caps & WCAP_OUTPUT_AMP_PRESENT)
    {
      char tmp[32];
      char *name = "-";

      if (hdaudio_snoopy)
	name = "outamp";

      if (widget->outamp_caps & ~AMPCAP_MUTE)	/* Has gain control */
	{
	  int range, step, typ, num, maxval, val;
	  range =
	    ((widget->
	      outamp_caps >> AMPCAP_NUMSTEPS_SHIFT) & AMPCAP_NUMSTEPS_MASK) +
	    1;
	  step =
	    ((widget->
	      outamp_caps >> AMPCAP_STEPSIZE_SHIFT) & AMPCAP_STEPSIZE_MASK) +
	    1;

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

	      if (hdaudio_snoopy > 0)
		{
		  sprintf (tmp, "%s:V%x", name, widget->wid);
		  name = tmp;
		}
	      else
		{
		  sprintf (tmp, "%s", widget->name);
		  name = tmp;
		}

	      if ((cnum = mixer_ext_create_control (mixer->mixer_dev,
						   group,
						   num, hdaudio_set_control,
						   typ,
						   name, maxval,
						   MIXF_READABLE |
						   MIXF_WRITEABLE |
						   MIXF_CENTIBEL)) < 0)
		return cnum;

	      /* Copy RGB color */
	      if (widget->rgbcolor != 0)
	      if ((ent = mixer_find_ext (dev, cnum)) != NULL)
		 ent->rgbcolor = widget->rgbcolor;

	      /* setup volume */
	      val = (maxval * 8) / 10;	/* 80% of the maximum */
	      val = val | (val << 16);
	      hdaudio_set_control (mixer->mixer_dev, num, SNDCTL_MIX_WRITE,
				   val);
	    }
	}
      else if (widget->outamp_caps & AMPCAP_MUTE)	/* Only mute control */
	{
	  char tmpname[32];
	  name = "mute";
	  if (hdaudio_snoopy > 0)
	    {
	      sprintf (tmpname, "%s:U%x", name, widget->wid);
	      name = tmpname;
	    }

	  if ((cnum = mixer_ext_create_control (mixer->mixer_dev,
					       group,
					       MIXNUM (widget,
						       CT_OUTMUTE, 0),
					       hdaudio_set_control,
					       MIXT_MUTE, name, 2,
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return cnum;

	      /* Copy RGB color */
	      if (widget->rgbcolor != 0)
	      if ((ent = mixer_find_ext (dev, cnum)) != NULL)
		 ent->rgbcolor = widget->rgbcolor;

	  hdaudio_set_control (mixer->mixer_dev,
			       MIXNUM (widget, CT_OUTMUTE, 0),
			       SNDCTL_MIX_WRITE, 0);
	}
    }

  return 0;
}

/*ARGSUSED*/
static int
attach_selector (int dev, hdaudio_mixer_t * mixer, codec_t * codec,
		 widget_t * widget, int group, int group_mode)
{
  unsigned int c, b;
  char *name = "src";

  int i, ctl;
  char tmp[256], *t = tmp;
  oss_mixext *ext;
  int count = 0;


  /* 
   * first check to see if there are more that 2 valid options to create a
   * selector for.
   */
  for (i = 0; i < widget->nconn; i++)
    if (!codec->widgets[widget->connections[i]].skip
	&& codec->widgets[widget->connections[i]].sensed_pin != PIN_OUT)
      count++;

  if (count < 2)
    return 0;


  name = widget->name;

  if (corb_read (mixer, widget->cad, widget->wid, 0, GET_SELECTOR, 0, &c, &b))
    widget->current_selector = c;

  if (hdaudio_snoopy > 0)
    {
      sprintf (tmp, "%s:%x", name, widget->wid);
      name = tmp;
    }


  if ((ctl = mixer_ext_create_control (mixer->mixer_dev,
				       group,
				       MIXNUM (widget, CT_SELECT, 0),
				       hdaudio_set_control,
				       MIXT_ENUM,
				       name,
				       widget->nconn,
				       MIXF_READABLE | MIXF_WRITEABLE)) < 0)
    return ctl;

  *tmp = 0;
  ext = mixer_find_ext (mixer->mixer_dev, ctl);

  if (ext == NULL)
    {
      cmn_err (CE_WARN, "Cannot locate the mixer extension (a)\n");
      return OSS_EIO;
    }

  /* Copy RGB color */
  ext->rgbcolor = widget->rgbcolor;

  memset (ext->enum_present, 0, sizeof (ext->enum_present));

  for (i = 0; i < widget->nconn; i++)
    {
      char *s;

      /*
       * ensure that the connection list has a valid widget id - some
       * devices have bogus connection lists 
       */
      if (codec->widgets[widget->connections[i]].wid < codec->first_node)
	continue;

      s = codec->widgets[widget->connections[i]].name;
      if (strlen (tmp) + strlen (s) + 1 < sizeof (tmp) - 1)
	{
	  if (*tmp != 0)
	    *t++ = ' ';
	  strcpy (t, s);
	  if (hdaudio_snoopy > 0)
	    sprintf (t, "A%s:%x", s,
		     mixer->codecs[widget->cad]->widgets[widget->
							 connections[i]].wid);
	  t += strlen (t);

	  /*
	   * Show only widgets that are not marked to be ignored.
	   * Also hide I/O pins that are known to be outputs.
	   */
	  if (!codec->widgets[widget->connections[i]].skip
	      && codec->widgets[widget->connections[i]].sensed_pin != PIN_OUT)
	    ext->enum_present[i / 8] |= (1 << (i % 8));
	  else
	    {
	      if (widget->current_selector == i)
		widget->current_selector++;
	    }
	}
    }
  mixer_ext_set_strings (mixer->mixer_dev, ctl, tmp, 0);

  if (widget->current_selector >= widget->nconn)
    widget->current_selector = 0;
  corb_write (mixer, widget->cad, widget->wid, 0, SET_SELECTOR,
	      widget->current_selector);

  return 0;
}

static int
follow_widget_chain (int dev, hdaudio_mixer_t * mixer, codec_t * codec,
		     widget_t * widget, int group)
{
  int err;

  if (widget->used)		/* Already handled */
    return 0;

  widget->used = 1;

  if (widget->nconn >= 1)
    if ((err =
	 follow_widget_chain (dev, mixer, codec,
			      &codec->widgets[widget->connections[0]],
			      group)) < 0)
      return err;

  if ((err = attach_amplifiers (dev, mixer, codec, widget, group, 1)) < 0)
    return err;

  if (widget->wid_type == NT_SELECT)
    if ((err = attach_selector (dev, mixer, codec, widget, group, 1)) < 0)
      return err;

  return 0;
}

static int
attach_pin_widget (int dev, hdaudio_mixer_t * mixer, codec_t * codec,
		   widget_t * widget, int parent_group)
{
  int group = parent_group, g;
  unsigned int b, c, conf;
  int i, ctl, err;
  int inselects = 0, outselects = 0, linked_controls = 0;
  int num_amps = 0;
  char tmp[256], *t = tmp;
  oss_mixext *ext;

  if (widget->pincaps & PINCAP_OUTPUT_CAPABLE)
    {
      outselects = widget->nconn;

      if (widget->nconn == 1)	/* Exactly one connection */
	{
	  linked_controls =
	    count_linked_controls (mixer, codec,
				   &codec->widgets[widget->connections[0]],
				   1);
	}
    }

  if (widget->pincaps & PINCAP_INPUT_CAPABLE)
    {
      if (!(widget->widget_caps & WCAP_DIGITAL))	/* Analog pin */
	{
	  inselects = 1;
	}
    }

  if (widget->widget_caps & WCAP_INPUT_AMP_PRESENT)
    {
      num_amps++;
    }

  if (widget->widget_caps & WCAP_OUTPUT_AMP_PRESENT)
    {
      num_amps++;
    }

  if ((inselects + outselects > 1) || num_amps > 0 || linked_controls > 0)	/* Have something to control */
    {
      if (widget->color[0] == 0)	/* Empty name */
	sprintf (widget->color, "jack%02x", widget->wid);
      if ((g =
	   mixer_ext_create_group (mixer->mixer_dev, group,
				   widget->color)) < 0)
	return g;

      if (corb_read
	  (mixer, widget->cad, widget->wid, 0, GET_SELECTOR, 0, &c, &b))
	widget->current_selector = c;

      if (inselects + outselects > 1)
	{
	  if ((ctl = mixer_ext_create_control (mixer->mixer_dev,
					       g,
					       MIXNUM (widget, CT_SELECT, 0),
					       hdaudio_set_control,
					       MIXT_ENUM,
					       "mode",
					       inselects + outselects,
					       MIXF_READABLE |
					       MIXF_WRITEABLE)) < 0)
	    return ctl;

	  *tmp = 0;
	  ext = mixer_find_ext (mixer->mixer_dev, ctl);

	  if (ext == NULL)
	    {
	      cmn_err (CE_WARN, "Cannot locate the mixer extension (b)\n");
	      return OSS_EIO;
	    }
  	  /* Copy RGB color */
  	  ext->rgbcolor = widget->rgbcolor;

	  memset (ext->enum_present, 0, sizeof (ext->enum_present));

	  for (i = 0; i < widget->nconn; i++)
	    {
	      char *s;

	      s = codec->widgets[widget->connections[i]].name;
	      if (strlen (tmp) + strlen (s) + 1 < sizeof (tmp) - 1)
		{
		  if (*tmp != 0)
		    *t++ = ' ';
		  strcpy (t, s);
		  if (hdaudio_snoopy > 0)
		    sprintf (t, "A%s:%x", s,
			     mixer->codecs[widget->cad]->widgets[widget->
								 connections
								 [i]].wid);
		  t += strlen (t);

		  /*
		   * Show only widgets that are not marked to be ignored.
		   */
		  if (!codec->widgets[widget->connections[i]].skip)
		    ext->enum_present[i / 8] |= (1 << (i % 8));
		  else
		    {
		      if (widget->current_selector == i)
			widget->current_selector++;
		    }
		}
	    }

/*
 * Use the default sequence as an index to the output source selectors.
 */
	  if (widget->sensed_pin == PIN_OUT)
	    if (corb_read
		(mixer, widget->cad, widget->wid, 0, GET_CONFIG_DEFAULT, 0,
		 &conf, &b))
	      {
		int association, sequence;

		association = (conf >> 4) & 0x0f;
		sequence = conf & 0x0f;

		if (association != 0)
		  {
		    widget->current_selector = sequence;
		  }

	      }

	  if (widget->current_selector >= widget->nconn)
	    widget->current_selector = 0;

	  if (inselects > 0)	/* Input capable */
	    {
	      char *s;

	      i = widget->nconn;
	      s = widget->name;

	      if (*tmp != 0)
		*t++ = ' ';

	      strcpy (t, "input");

	      t += strlen (t);
	      ext->enum_present[i / 8] |= (1 << (i % 8));
	      i++;

	      if (widget->pin_type == PIN_IN)
		widget->current_selector = widget->nconn;
	    }

	  mixer_ext_set_strings (mixer->mixer_dev, ctl, tmp, 0);
	}

      hdaudio_set_control (mixer->mixer_dev,
			   MIXNUM (widget, CT_SELECT, 0),
			   SNDCTL_MIX_WRITE, widget->current_selector);

      if ((err = attach_amplifiers (dev, mixer, codec, widget, g, 0)) < 0)
	return err;

      if (widget->nconn == 1)
	if ((err =
	     follow_widget_chain (dev, mixer, codec,
				  &codec->widgets[widget->connections[0]],
				  g)) < 0)
	  return err;
    }

  return 0;
}

static int
attach_record_widget (int dev, hdaudio_mixer_t * mixer, codec_t * codec,
		      widget_t * widget, int parent_group)
{
  int group = parent_group, g;
  int err;
  int linked_controls = 0;
  int num_amps = 0;

  if (widget->widget_caps & WCAP_INPUT_AMP_PRESENT)
    {
      num_amps++;
    }

  if (widget->widget_caps & WCAP_OUTPUT_AMP_PRESENT)
    {
      num_amps++;
    }

  if (widget->nconn == 1)	/* Exactly one connection */
    {
      linked_controls =
	count_linked_controls (mixer, codec,
			       &codec->widgets[widget->connections[0]], 1);
    }

  if (num_amps > 0 || linked_controls > 1)	/* Have something to control */
    {
      if ((g =
	   mixer_ext_create_group (mixer->mixer_dev, group,
				   widget->name)) < 0)
	return g;

      if (widget->nconn == 1)
	if ((err =
	     follow_widget_chain (dev, mixer, codec,
				  &codec->widgets[widget->connections[0]],
				  g)) < 0)
	  return err;

      if ((err = attach_amplifiers (dev, mixer, codec, widget, g, 0)) < 0)
	return err;
      if ((err = attach_selector (dev, mixer, codec, widget, g, 0)) < 0)
	return err;
    }

  return 0;
}

static int
attach_misc_widget (int dev, hdaudio_mixer_t * mixer, codec_t * codec,
		    widget_t * widget, int parent_group)
{
  int err;
  int nselect = 0;
  int num_amps = 0;

  if (widget->widget_caps & WCAP_INPUT_AMP_PRESENT)
    {
      num_amps++;
    }

  if (widget->widget_caps & WCAP_OUTPUT_AMP_PRESENT)
    {
      num_amps++;
    }

  if ((widget->wid_type == NT_SELECT || widget->wid_type == NT_MIXER)
      && widget->nconn > 0)
    nselect = widget->nconn;

  if (num_amps > 0 || nselect > 1)	/* Have something to control */
    {
#if 0
      if ((g =
	   mixer_ext_create_group (mixer->mixer_dev, group,
				   widget->name)) < 0)
	return g;
#endif
      if ((err =
	   attach_amplifiers (dev, mixer, codec, widget, parent_group,
			      0)) < 0)
	return err;

      if (nselect > 1)
	if ((err =
	     attach_selector (dev, mixer, codec, widget, parent_group,
			      0)) < 0)
	  return err;
    }

  return 0;
}

int
hdaudio_generic_mixer_init (int dev, hdaudio_mixer_t * mixer, int cad,
			    int parent_group)
{
  unsigned int vendorid, b;
  int err;
  int wid, n;
  codec_t *codec;
  widget_t *widget;
  int group = parent_group;

  if (mixer->codecs[cad] == NULL)
    {
      cmn_err (CE_WARN, "Bad codec %d\n", cad);
      return OSS_EIO;
    }
  codec = mixer->codecs[cad];

  if (!corb_read (mixer, cad, 0, 0, GET_PARAMETER, HDA_VENDOR, &vendorid, &b))
    {
      cmn_err (CE_WARN, "Cannot get codec ID\n");
      return OSS_EIO;
    }

/*
 * First handle all the PIN widgets
 */
  n = 0;

  for (wid = 0; wid < codec->nwidgets; wid++)
    {
      widget = &codec->widgets[wid];

      if (widget->wid_type != NT_PIN)	/* Not a pin widget */
	continue;

      widget->used = 1;

      if (widget->skip)		/* Unused/unconnected PIN widget */
	{
	  continue;
	}

      if ((n++ % 3) == 0)
	{
	  if ((group =
	       mixer_ext_create_group (mixer->mixer_dev, parent_group,
				       "jack")) < 0)
	    return group;
	}


      if ((err = attach_pin_widget (dev, mixer, codec, widget, group)) < 0)
	return err;
    }

/*
 * Next handle all the ADC widgets
 */
  n = 0;
  for (wid = 0; wid < codec->nwidgets; wid++)
    {
      widget = &codec->widgets[wid];

      if (widget->wid_type != NT_ADC)	/* Not a pin widget */
	continue;

      if (widget->skip)
	continue;

      widget->used = 1;

      if ((n++ % 3) == 0)
	{
	  if ((group =
	       mixer_ext_create_group (mixer->mixer_dev, parent_group,
				       "record")) < 0)
	    return group;
	}

      if ((err = attach_record_widget (dev, mixer, codec, widget, group)) < 0)
	return err;
    }

/*
 * Finally handle all the widgets that have not been attached yet
 */

  n = 0;
  for (wid = 0; wid < codec->nwidgets; wid++)
    {
      widget = &codec->widgets[wid];

      if (widget->skip)
	continue;

      if (widget->used)		/* Already handled */
	continue;

      widget->used = 1;

      if (count_linked_controls (mixer, codec, widget, 0) > 0)
	if ((n++ % 4) == 0)
	  {
	    if ((group =
		 mixer_ext_create_group (mixer->mixer_dev, parent_group,
					 "misc")) < 0)
	      return group;
	  }

      if ((err = attach_misc_widget (dev, mixer, codec, widget, group)) < 0)
	return err;
    }
  return 0;
}
