/*****************************************************************************
   driverevents.c - Events from the driver processing
   Copyright (C) 2000 by Jaroslav Kysela <perex@perex.cz>
   
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
******************************************************************************/

#include "envy24control.h"

void control_input_callback(gpointer data, gint source, GdkInputCondition condition)
{
	snd_ctl_t *ctl = (snd_ctl_t *)data;
	snd_ctl_event_t *ev;
	const char *name;
	int index;
	unsigned int mask;

	snd_ctl_event_alloca(&ev);
	if (snd_ctl_read(ctl, ev) < 0)
		return;
	name = snd_ctl_event_elem_get_name(ev);
	index = snd_ctl_event_elem_get_index(ev);
	mask = snd_ctl_event_elem_get_mask(ev);
	if (! (mask & (SND_CTL_EVENT_MASK_VALUE | SND_CTL_EVENT_MASK_INFO)))
		return;

	switch (snd_ctl_event_elem_get_interface(ev)) {
	case SND_CTL_ELEM_IFACE_MIXER:
		if (!strcmp(name, "Word Clock Sync"))
			master_clock_update();
		else if (!strcmp(name, "Multi Track Volume Rate"))
			volume_change_rate_update();
		else if (!strcmp(name, "IEC958 Input Optical"))
			spdif_input_update();
		else if (!strcmp(name, "Delta IEC958 Output Defaults"))
			spdif_output_update();
		else if (!strcmp(name, "Multi Track Internal Clock"))
			master_clock_update();
		else if (!strcmp(name, "Multi Track Internal Clock Default"))
			master_clock_update();
		else if (!strcmp(name, "Multi Track Rate Locking"))
			rate_locking_update();
		else if (!strcmp(name, "Multi Track Rate Reset"))
			rate_reset_update();
		else if (!strcmp(name, "Multi Playback Volume"))
			mixer_update_stream(index + 1, 1, 0);
		else if (!strcmp(name, "H/W Multi Capture Volume"))
			mixer_update_stream(index + 11, 1, 0);
		else if (!strcmp(name, "IEC958 Multi Capture Volume"))
			mixer_update_stream(index + 19, 1, 0);
		else if (!strcmp(name, "Multi Playback Switch"))
			mixer_update_stream(index + 1, 0, 1);
		else if (!strcmp(name, "H/W Multi Capture Switch"))
			mixer_update_stream(index + 11, 0, 1);
		else if (!strcmp(name, "IEC958 Multi Capture Switch"))
			mixer_update_stream(index + 19, 0, 1);
		else if (!strcmp(name, "H/W Playback Route"))
			patchbay_update();
		else if (!strcmp(name, "IEC958 Playback Route"))
			patchbay_update();
		else if (!strcmp(name, "DAC Volume"))
			dac_volume_update(index);
		else if (!strcmp(name, "ADC Volume"))
			adc_volume_update(index);
		else if (!strcmp(name, "IPGA Analog Capture Volume"))
			ipga_volume_update(index);
		else if (!strcmp(name, "Output Sensitivity Switch"))
			dac_sense_update(index);
		else if (!strcmp(name, "Input Sensitivity Switch"))
			adc_sense_update(index);
		break;
	default:
		break;
	}
}

