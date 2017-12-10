/*****************************************************************************
   loopback.c
   Copyright (C) 2003 by Robert Vetter <postmaster@robertvetter.com>
   
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
******************************************************************************/

#include "rmedigicontrol.h" 

static snd_ctl_elem_value_t *val;

static void loopback_toggled(GtkToggleButton *c,gpointer p)
{
	snd_ctl_elem_value_set_integer(val,0,gtk_toggle_button_get_active(c)?1:0);
	snd_ctl_elem_write(ctl, val);
}

GtkWidget *create_loopback_toggle()
{
	GtkWidget *t;
	GtkWidget *box;
	char *elem_name="Loopback Input";
	
	box=gtk_hbox_new(FALSE,0);
	
	snd_ctl_elem_value_malloc(&val);
	
	snd_ctl_elem_value_set_interface(val, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_value_set_name(val,elem_name);
	snd_ctl_elem_read(ctl, val);
	
	t=gtk_check_button_new_with_label(elem_name);

	gtk_signal_connect(GTK_OBJECT(t),"toggled",GTK_SIGNAL_FUNC(loopback_toggled),NULL);
	if(snd_ctl_elem_value_get_integer(val,0))
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(t),TRUE);

	gtk_box_pack_start(GTK_BOX(box),t,TRUE,TRUE,0);

	return box;
}
