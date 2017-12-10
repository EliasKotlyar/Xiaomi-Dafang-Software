/*****************************************************************************
   rmedigicontrol.c
   Copyright (C) 2003 by Robert Vetter <postmaster@robertvetter.com>
   
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
*****************************************************************************/

#include "rmedigicontrol.h"

snd_ctl_t *ctl;
ctl_elem_info_val_t input_iv;
ctl_elem_info_val_t clock_iv;
ctl_elem_info_val_t monitor_iv;
ctl_elem_info_val_t att_iv;

void destroy(GtkWidget *widget,gpointer data)
{
	snd_ctl_close(ctl);	
    gtk_main_quit();
}
int main(int argc, char *argv[])
{
	int card;
	char name[8],*err;
	snd_ctl_card_info_t *hw_info;
	card_type_t type;
	
	GtkWidget *window,*main_box,*input_box,*loopback_box,*clock_box,*monitor_box,*att_box,*level_box;
	GtkWidget *col1_box,*col2_box,*err_lbl;
	snd_ctl_card_info_alloca(&hw_info);

	card = -1;
	type = NO_CARD;
	err=NULL;
	if(snd_card_next(&card)<0 || card<0)
		err="no soundcards found...";
	while(card>=0)
	{
		sprintf(name,"hw:%d",card);
		if(snd_ctl_open(&ctl, name, 0)<0)
		{
			err="snd_ctl_open Error";
			break;
			exit(EXIT_FAILURE);
		}
		if(snd_ctl_card_info(ctl, hw_info)< 0)
		{
			err="snd_ctl_card_info Error";
			break;
			exit(EXIT_FAILURE);
		}
		if(strcmp(snd_ctl_card_info_get_driver(hw_info),"Digi32")==0)
		{
			type=DIGI32;
			break;
		}
		if(strcmp(snd_ctl_card_info_get_driver(hw_info),"Digi96")==0)
		{
			if(strcmp(snd_ctl_card_info_get_name(hw_info),"RME Digi96")==0)
				type=DIGI96;
			else if(strcmp(snd_ctl_card_info_get_name(hw_info),"RME Digi96/8")==0)
				type=DIGI96_8;
			else
				type=DIGI96_8_OTHER;
			break;
		}
		snd_card_next(&card);
	}
	if(card<0)
		err="No RME Digi Soundcard found...";
	gtk_init(&argc, &argv);	
	window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_signal_connect(GTK_OBJECT(window),"destroy",GTK_SIGNAL_FUNC(destroy),NULL);
	if(err)
	{
		err_lbl=gtk_label_new(err);
		gtk_container_add(GTK_CONTAINER(window),err_lbl);
		gtk_widget_show_all(window);
		gtk_main ();	
		exit(EXIT_FAILURE);	
	}

	gtk_window_set_title(GTK_WINDOW(window),snd_ctl_card_info_get_name(hw_info));
	
	main_box=gtk_hbox_new(FALSE,0);
	col1_box=gtk_vbox_new(FALSE,0);	
	col2_box=gtk_vbox_new(FALSE,0);	
	
	input_box=create_enum_elem_radio("Input Connector",&input_iv);
	gtk_box_pack_start(GTK_BOX(col1_box),input_box,TRUE,FALSE,0);
	
	loopback_box=create_loopback_toggle();
	gtk_box_pack_start(GTK_BOX(col1_box),loopback_box,TRUE,FALSE,0);

	clock_box=create_enum_elem_radio("Sample Clock Source",&clock_iv);
	gtk_box_pack_start(GTK_BOX(col1_box),clock_box,TRUE,FALSE,0);

	gtk_box_pack_start(GTK_BOX(main_box),col1_box,TRUE,FALSE,8);

	if(type==DIGI96_8_OTHER)
	{
		monitor_box=create_enum_elem_radio("Monitor Tracks",&monitor_iv);
		gtk_box_pack_start(GTK_BOX(col2_box),monitor_box,TRUE,FALSE,0);
	
		att_box=create_enum_elem_radio("Attenuation",&att_iv);
		gtk_box_pack_start(GTK_BOX(col2_box),att_box,TRUE,FALSE,0);

		gtk_box_pack_start(GTK_BOX(main_box),col2_box,TRUE,TRUE,8);

		level_box=create_level_box();
		gtk_box_pack_start(GTK_BOX(main_box),level_box,TRUE,TRUE,8);
	}
	gtk_container_add(GTK_CONTAINER(window),main_box);		
	gtk_widget_show_all(window);
	gtk_main ();
	return EXIT_SUCCESS;
}
void elem_radio_toggled(GtkRadioButton *r,gpointer p)
{
	int i;
	GSList *l;
	ctl_elem_info_val_t *iv;
	
	iv=(ctl_elem_info_val_t *)p;
	l=gtk_radio_button_group(r);
	i=snd_ctl_elem_info_get_items(iv->info);
	while(l)
	{
		i--;
		if(l->data==r && i>=0)
		{
			snd_ctl_elem_value_set_enumerated(iv->val,0,i);
			snd_ctl_elem_write(ctl, iv->val);
		}
		l=l->next;
	}
}
GtkWidget *create_enum_elem_radio(char *elem_name,ctl_elem_info_val_t *iv)
{
	GtkWidget *r,*box,*active,*frame;
	int i;
	GSList *group;

	snd_ctl_elem_info_malloc(&iv->info);
	snd_ctl_elem_value_malloc(&iv->val);
	
	group=NULL;
	active=NULL;
	box=gtk_vbox_new(TRUE,0);
	
	snd_ctl_elem_info_set_interface(iv->info, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_info_set_name(iv->info,elem_name);
	snd_ctl_elem_info_set_numid(iv->info,0);
	snd_ctl_elem_info(ctl,iv->info);
	
	snd_ctl_elem_value_set_interface(iv->val, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_value_set_name(iv->val,elem_name);
	snd_ctl_elem_read(ctl,iv->val);
	
	for(i=0;i<snd_ctl_elem_info_get_items(iv->info);i++)
	{
		snd_ctl_elem_info_set_item(iv->info, i);
		snd_ctl_elem_info(ctl,iv->info);
		r=gtk_radio_button_new_with_label(group,snd_ctl_elem_info_get_item_name(iv->info));
		group=gtk_radio_button_group(GTK_RADIO_BUTTON(r));
		gtk_signal_connect(GTK_OBJECT(r),"toggled",GTK_SIGNAL_FUNC(elem_radio_toggled),(gpointer)iv);
		if(i==snd_ctl_elem_value_get_integer(iv->val,0))
			active=r;
		gtk_box_pack_start(GTK_BOX(box),r,TRUE,FALSE,0);
	}
	if(active)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(active),TRUE);
	frame=gtk_frame_new(elem_name);
	gtk_frame_set_shadow_type(GTK_FRAME(frame),GTK_SHADOW_IN);
	gtk_container_add(GTK_CONTAINER(frame),box);
	return frame;
}

