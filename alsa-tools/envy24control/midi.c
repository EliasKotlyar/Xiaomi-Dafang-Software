/*****************************************************************************
   envy24control.c - Env24 chipset (ICE1712) control utility
   midi controller code
   (c) 2004, 2005 by Dirk Jagdmann <doj@cubic.org>

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

#include <string.h>
#include <alsa/asoundlib.h>
#include "midi.h"
#include <gtk/gtk.h>
#include <stdint.h>

static const int midi2slider_lin[128] = {
  96, 96, 95, 94, 93, 93, 92, 91,
  90, 90, 89, 88, 87, 87, 86, 85,
  84, 84, 83, 82, 81, 81, 80, 79,
  78, 78, 77, 76, 75, 75, 74, 73,
  72, 72, 71, 70, 69, 69, 68, 67,
  66, 66, 65, 64, 63, 62, 62, 61,
  60, 59, 59, 58, 57, 56, 56, 55,
  54, 53, 53, 52, 51, 50, 50, 49,
  48, 47, 47, 46, 45, 44, 44, 43,
  42, 41, 41, 40, 39, 38, 38, 37,
  36, 35, 35, 34, 33, 32, 31, 31,
  30, 29, 28, 28, 27, 26, 25, 25,
  24, 23, 22, 22, 21, 20, 19, 19,
  18, 17, 16, 16, 15, 14, 13, 13,
  12, 11, 10, 10, 9, 8, 7, 7,
  6, 5, 4, 4, 3, 2, 1, 0,
};
static const int slider2midi_lin[97] = {
  0, 1, 2, 3, 5, 6, 7, 9,
  10, 11, 13, 14, 15, 17, 18, 19,
  21, 22, 23, 25, 26, 27, 29, 30,
  31, 33, 34, 35, 37, 38, 39, 41,
  42, 43, 44, 46, 47, 48, 50, 51,
  52, 54, 55, 56, 58, 59, 60, 62,
  63, 64, 66, 67, 68, 70, 71, 72,
  74, 75, 76, 78, 79, 80, 82, 83,
  84, 85, 87, 88, 89, 91, 92, 93,
  95, 96, 97, 99, 100, 101, 103, 104,
  105, 107, 108, 109, 111, 112, 113, 115,
  116, 117, 119, 120, 121, 123, 124, 125,
  127,
};

static const int midi2slider_enh[128] = {
  96, 95, 94, 93, 91, 90, 89, 87,
  86, 85, 83, 82, 81, 79, 78, 77,
  75, 74, 73, 72, 70, 69, 68, 66,
  65, 64, 62, 61, 60, 58, 57, 56,
  54, 53, 52, 51, 49, 48, 47, 45,
  44, 43, 41, 40, 39, 37, 36, 35,
  33, 32, 31, 30, 28, 27, 26, 24,
  23, 22, 20, 19, 18, 16, 15, 14,
  12, 12, 12, 12, 12, 11, 11, 11,
  11, 11, 10, 10, 10, 10, 10, 9,
  9, 9, 9, 9, 8, 8, 8, 8,
  8, 7, 7, 7, 7, 7, 6, 6,
  6, 6, 6, 5, 5, 5, 5, 5,
  4, 4, 4, 4, 4, 3, 3, 3,
  3, 3, 2, 2, 2, 2, 2, 1,
  1, 1, 1, 1, 0, 0, 0, 0,
};
static const int slider2midi_enh[97] = {
  0, 1, 2, 3, 4, 4, 5, 6,
  7, 7, 8, 9, 10, 10, 11, 12,
  13, 13, 14, 15, 16, 16, 17, 18,
  19, 20, 20, 21, 22, 23, 23, 24,
  25, 26, 26, 27, 28, 29, 29, 30,
  31, 32, 32, 33, 34, 35, 36, 36,
  37, 38, 39, 39, 40, 41, 42, 42,
  43, 44, 45, 45, 46, 47, 48, 48,
  49, 50, 51, 52, 52, 53, 54, 55,
  55, 56, 57, 58, 58, 59, 60, 61,
  61, 62, 63, 68, 68, 73, 78, 83,
  88, 93, 98, 103, 108, 113, 118, 123,
  127,
};

static const int *midi2slider, *slider2midi;
static snd_seq_t *seq=0;
static int client, clientId, port, ch;
static char *portname=0, *appname=0;
static int maxstreams=0;
static int currentvalue[128];

void midi_maxstreams(int m)
{
  maxstreams=m*2;
}

int midi_close()
{
  int i=0;
  if(seq)
    i=snd_seq_close(seq);

  seq=0;
  client=port=0;
  if(portname)
    free(portname), portname=0;
  if(appname)
    free(appname), appname=0;

  return i;
}

static void do_controller(int c, int v)
{
  snd_seq_event_t ev;
  if(!seq) return;
  if(currentvalue[c]==v) return;
#if 0
  fprintf(stderr, "do_controller(%i,%i)\n",c,v);
#endif
  snd_seq_ev_clear(&ev);
  snd_seq_ev_set_source(&ev, port);
  snd_seq_ev_set_subs(&ev);
  snd_seq_ev_set_direct(&ev);
  snd_seq_ev_set_controller(&ev,ch,c,v);
  snd_seq_event_output(seq, &ev);
  snd_seq_drain_output(seq);

  currentvalue[c]=v;
}

int midi_controller(int c, int v)
{
  int v2;

  if (! seq)
    return 0;

  if(c<0 || c>127) return 0;

  if(v<0) v=0;
  else if(v>96) v=96;
  v2=slider2midi[v];
#if 0
  fprintf(stderr, "midi_controller(%i,%i)->%i\n",c,v,v2);
#endif
  do_controller(c,v2);
  return 0;
}

int midi_button(int b, int v)
{
  if(b<0) return 0;
  b+=maxstreams;
  if(b>127) return 0;
  do_controller(b, v?127:0);
  return 0;
}

int midi_init(char *appname, int channel, int midi_enhanced)
{
  snd_seq_client_info_t *clientinfo;
  int npfd;
  struct pollfd *pfd;

  if(seq)
    return 0;

  for(npfd=0; npfd!=128; ++npfd)
    currentvalue[npfd]=-1;

  ch=channel;
  if(midi_enhanced)
    {
      midi2slider=midi2slider_enh;
      slider2midi=slider2midi_enh;
    }
  else
    {
      midi2slider=midi2slider_lin;
      slider2midi=slider2midi_lin;
    }

  if(snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK) < 0)
    {
      g_warning("could not init ALSA sequencer\n");
      seq=0;
      return -1;
    }

  snd_seq_set_client_name(seq, appname);
  snd_seq_client_info_alloca(&clientinfo);
  snd_seq_get_client_info (seq, clientinfo);
  client=snd_seq_client_info_get_client(clientinfo);
  clientId = snd_seq_client_id(seq);

  portname=g_strdup_printf("%s Mixer Control", appname);
  port=snd_seq_create_simple_port(seq, portname,
				  SND_SEQ_PORT_CAP_WRITE|SND_SEQ_PORT_CAP_SUBS_WRITE|SND_SEQ_PORT_CAP_READ|SND_SEQ_PORT_CAP_SUBS_READ,
				  SND_SEQ_PORT_TYPE_APPLICATION);
  if(port < 0)
    {
      g_warning("could not create ALSA sequencer port\n");
      midi_close();
      return -1;
    }

  npfd=snd_seq_poll_descriptors_count(seq, POLLIN);
  if(npfd<=0)
    {
      g_warning("could not get number of ALSA sequencer poll descriptors\n");
      midi_close();
      return -1;
    }

  pfd=(struct pollfd*)alloca(npfd * sizeof(struct pollfd));
  if(pfd==0)
    {
      g_warning("could not alloc memory for ALSA sequencer poll descriptors\n");
      midi_close();
      return -1;
    }
  if(snd_seq_poll_descriptors(seq, pfd, npfd, POLLIN) != npfd)
    {
      g_warning("number of returned poll desc is not equal of request poll desc\n");
      midi_close();
      return -1;
    }

  return pfd[0].fd;
}

void mixer_adjust(GtkAdjustment *adj, gpointer data);
void mixer_set_mute(int stream, int left, int right);

void midi_process(gpointer data, gint source, GdkInputCondition condition)
{
  snd_seq_event_t *ev;
  static GtkAdjustment *adj=0;
  if(!adj)
    adj=(GtkAdjustment*) gtk_adjustment_new(0, 0, 96, 1, 1, 10);

  do
    {
      snd_seq_event_input(seq, &ev);
      if(!ev) continue;
      switch(ev->type)
	{
	case SND_SEQ_EVENT_CONTROLLER:
#if 0
	  fprintf(stderr, "Channel %02d: Controller %03d: Value:%d\n",
		  ev->data.control.channel, ev->data.control.param, ev->data.control.value);
#endif
	  if(ev->data.control.channel == ch)
	    {
	      currentvalue[ev->data.control.param]=ev->data.control.value;
	      if(ev->data.control.param < maxstreams)
		{
		  int stream=ev->data.control.param;
		  long data=((stream/2+1)<<16)|(stream&1);
		  gtk_adjustment_set_value(adj, midi2slider[ev->data.control.value]);
		  mixer_adjust(adj, (gpointer)data);
		}
	      else if(ev->data.control.param < maxstreams*2)
		{
		  int b=ev->data.control.param-maxstreams;
		  int left=-1, right=-1;
		  if(b&1)
		    right=ev->data.control.value;
		  else
		    left=ev->data.control.value;
		  mixer_set_mute(b/2+1, left, right);
		}
	    }
	  break;

	case SND_SEQ_EVENT_PORT_SUBSCRIBED:
#if 0
	  fprintf(stderr, "event subscribed send.client:%i dest.client:%i clientId:%i\n",
		  (int)ev->data.connect.sender.client, (int)ev->data.connect.dest.client, clientId);
#endif
	  if(ev->data.connect.dest.client!=clientId)
	    {
	      int i;
	      for(i=0; i!=128; ++i)
		if(currentvalue[i] >= 0)
		  {
		    /* set currentvalue[i] to a fake value, so the check in do_controller does not trigger */
		    int v=currentvalue[i];
		    currentvalue[i]=-1;
		    do_controller(i, v);
		  }
	    }
	  break;
	}
      snd_seq_free_event(ev);
    }
  while (snd_seq_event_input_pending(seq, 0) > 0);
}

/* ************************************************* */
/* C++ code to help calculating midi<->slider tables */
#if 0
#include <iostream>
#include <map>
#include <vector>
using namespace std;
int main()
{
  int i;
  int midi2slider[128];
  ///// your midi to slider conversion should be calculated here
  for(i=0; i<64; ++i)
    midi2slider[i]=(i*84)/64;
  for(i=0; i<64; ++i)
    midi2slider[i+64]=(i*13)/64+84;
  ///// end of your calculation

  // print map
  map<int,int> m;
  int z=-1;
  cout << "static const int midi2slider_enh[128] = {" << endl;
  for(i=0; i<128; ++i)
    {
      int v=96-midi2slider[i];
      cout << v << ", ";
      if(((++z)%8)==7)
	cout << endl;
      m[v]=z;
    }
  cout << "};" << endl;

  // now generate the reverse map
  vector<int> rm;
  cout << "static const int slider2midi_enh[97] = {" << endl;
  int last=0;
  for(i=0; i<97; ++i)
    {
      int v=m[i];
      if(v==0) v=last;
      last=v;
      rm.push_back(v);
    }
  z=-1;
  for(i=96; i>=0; --i)
    {
      cout << rm[i] << ", ";
      if(((++z)%8)==7)
	cout << endl;
    }
  cout << "};" << endl;
  return 0;
}
#endif
