/*
 *  ALSA mixer console for Echoaudio soundcards.
 *  Copyright (C) 2003 Giuliano Pochini <pochini@shiny.it>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#define EM_VERSION "%s Echomixer v" VERSION


/*******
  Remove the "//" if you want to compile Echomixer in reverse mode.
*******/

//#define REVERSE


/*******
 Constants marked with *M* can be modified to customize the interface.
*******/


#define BORDER		6			// *M* Inner border of GTK containers
#define SPACING		8			// *M* Spacing of control sections

// Graphic mixer constants
#define GM_BARWIDTH	5			// *M* Width of meters bars
#define XCELLBORDER	2			// *M* Space between the grid lines and the content of the cell
#define YCELLBORDER	2			// *M*
#define XCELLDIM	20			// *M* Width of the cell
#define YCELLDIM	32			// Height of the cell
#define XCELLTOT (1+XCELLBORDER*2+XCELLDIM)	// line + left border + cell + right border
#define YCELLTOT (1+YCELLBORDER*2+YCELLDIM)
#define XVOLUME (1+XCELLBORDER+3)		// Position of the volume slider
#define XMETER (1+XCELLBORDER-GM_BARWIDTH/2+13)	// Position of the VU bar

// VU-meter constants
#define VU_XGRAF	30			// Left margin of the graphic
#define VU_YGRAF	20			// Top margin
#define VU_BARWIDTH	6			// *M* Width of VU-meters bars
#define VU_BARSEP	2			// *M* Space between bars

#define SHORTSTEP	1			// *M* 1dB (when the users moves a slider with cursor keys)
#define LONGSTEP	6			// *M* 6dB (with Page up/down or clicking the background)
#define DIGITAL_MODES	16			// Max number of digital modes
#define ECHO_CLOCKS	8			// Max number of clock sources

#define INPUT 0
#define OUTPUT 1
#define ECHO_MAXAUDIO_IOS	32		// The maximum number of inputs + outputs
#define ECHO_MAXAUDIOINPUTS	32		// Max audio input channels
#define ECHO_MAXAUDIOOUTPUTS	32		// Max audio output channels
#define ECHOGAIN_MUTED		(-128)		// Minimum possible gain
#define ECHOGAIN_MINOUT		(-128)		// Min output gain (unit is 1dB)
#define ECHOGAIN_MAXOUT		6		// Max output gain (unit is 1dB)
#define ECHOGAIN_MININP		(-50)		// Min input gain (unit is 0.5dB)
#define ECHOGAIN_MAXINP		50		// Max input gain (unit is 0.5dB)

// GTK+ adjustment widgets have the mininum value at top and maximum at bottom,
// position, but we need the opposite. This function puts the scale upside-down.
#define INVERT(x)	(ECHOGAIN_MINOUT+ECHOGAIN_MAXOUT-(x))
#define IN_INVERT(x)	(ECHOGAIN_MININP+ECHOGAIN_MAXINP-(x))

// REAL is for debugging only.
#define REAL

//#define CTLID_DEBUG(x) printf x
#define CTLID_DEBUG(x)

#define UI_DEBUG(x)
//#define UI_DEBUG(x) printf x


#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <alsa/asoundlib.h>


char card[64], cardId[16];
char dmodeName[DIGITAL_MODES][64], clocksrcName[DIGITAL_MODES][64], spdifmodeName[DIGITAL_MODES][64];
int nLOut, nIn, fdIn, fdOut, nPOut, ClockMask;
int ndmodes, nclocksrc, nspdifmodes;
int GMixerRow, GMixerColumn, Gang, AutoClock;
int lineinId, pcmoutId, lineoutId, mixerId, vmixerId, p4InId, p4OutId, dmodeId;
int clocksrcId, spdifmodeId, vuswitchId, vumetersId, channelsId, phantomId, automuteId;
int metersStreams, metersNumber, metersTypes;
int outvolCount;
int mouseY, mouseButton;
int dmodeVal, clocksrcVal, spdifmodeVal;
int VUwidth, VUheight, Mixwidth, Mixheight;

#define DONT_DRAW (ECHOGAIN_MUTED-1)
#define DONT_CHANGE (1<<31)
#define NOPOS 999999
struct geometry {
  int st;		// window status: 0 = hidden ; 1 = visible ; NOPOS = no stored setting
  GtkWidget *toggler;	// The toggle button that controls this window
  int x, y;
  int w, h;
} Mainw_geom, Miscw_geom, PVw_geom, LVw_geom, Mixerw_geom, Vmixerw_geom, VUw_geom, GMw_geom;

// This structure contains the first and the last row of each section of the graphic mixer window
struct {
  int VmixerFirst, VmixerLast;	// Rows
  int LineOut;			// There is only one row
  int Inputs;			// Rows
  int Outputs;			// Columns
} GMixerSection;

struct mixel {
  int id;
  int Gain;
};

snd_ctl_t *ctlhandle;


struct mixerControl_s {
  int input, inputs;
  int output, outputs;
  int id;
  GtkWidget *window;
  GtkWidget *volume[ECHO_MAXAUDIOOUTPUTS];
  GtkWidget *label[ECHO_MAXAUDIOOUTPUTS];
  GtkObject *adj[ECHO_MAXAUDIOOUTPUTS];
  GtkWidget *outsel[ECHO_MAXAUDIOOUTPUTS];
  GtkWidget *inpsel[ECHO_MAXAUDIOINPUTS];
  GtkWidget *vchsel[ECHO_MAXAUDIOOUTPUTS];
  struct mixel mixer[ECHO_MAXAUDIOOUTPUTS][ECHO_MAXAUDIOOUTPUTS];
} mixerControl, vmixerControl;

struct VolumeControl_s {
  int input, output;				// Currently selected channels
  int inputs, outputs;
  int id;
  GtkWidget *window;
  GtkWidget *volume[ECHO_MAXAUDIOOUTPUTS];
  GtkWidget *label[ECHO_MAXAUDIOOUTPUTS];
  GtkObject *adj[ECHO_MAXAUDIOOUTPUTS];
  int Gain[ECHO_MAXAUDIOOUTPUTS];
} lineinControl, lineoutControl, pcmoutControl;

struct NominalLevelControl_s {
  int id;
  int Channels;
  char Level[ECHO_MAXAUDIOINPUTS];
  GtkWidget *Button[ECHO_MAXAUDIOINPUTS];
} NominalIn, NominalOut;

struct SwitchControl_s {
  int id;
  int value;
  GtkWidget *Button;
} PhantomPower, Automute;

GtkWidget *clocksrc_menuitem[ECHO_CLOCKS];
GtkWidget *dmodeOpt, *clocksrcOpt, *spdifmodeOpt, *phantomChkbutton, *autoclockChkbutton;
GtkWidget *window, *Mainwindow, *Miscwindow, *LVwindow, *VUwindow, *GMwindow;
GtkWidget *VUdarea, *Mixdarea;
gint VUtimer, Mixtimer, clocksrctimer;

GdkGC *gc=0;
static GdkPixmap *VUpixmap = NULL;
static GdkPixmap *Mixpixmap = NULL;
GdkFont *fnt;

void Clock_source_activate(GtkWidget *widget, gpointer clk);



int CountBits(int n) {
  int c;

  c=0;
  while (n) {
    c++;
    n&=(n-1);
  }
  return(c);
}



void ClampOutputVolume(int *v) {

  if (*v>ECHOGAIN_MAXOUT)
    *v=ECHOGAIN_MAXOUT;
  else if (*v<ECHOGAIN_MINOUT)
    *v=ECHOGAIN_MINOUT;
}



void ClampInputVolume(int *v) {

  if (*v>ECHOGAIN_MAXINP)
    *v=ECHOGAIN_MAXINP;
  else if (*v<ECHOGAIN_MININP)
    *v=ECHOGAIN_MININP;
}



// -128 dB means muted, that is -infinite dB
int Add_dB (int a, int b) {

  if (a==ECHOGAIN_MINOUT || b==ECHOGAIN_MINOUT)
    return(ECHOGAIN_MINOUT);
  a+=b;
  if (a<ECHOGAIN_MINOUT)
    return(ECHOGAIN_MINOUT);
  return(a);
}



char *strOutGain(char *s, int g) {

  if (g==ECHOGAIN_MINOUT)
    strcpy(s, "mute");
  else
    sprintf(s, "%+d", g);
  return(s);
}



// Write an enumerated ALSA control
int SetEnum(int numid, int val) {
  int err;
  snd_ctl_elem_id_t *id;
  snd_ctl_elem_value_t *control;

  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_value_alloca(&control);
  snd_ctl_elem_id_set_interface(id, numid==clocksrcId ? SND_CTL_ELEM_IFACE_PCM : SND_CTL_ELEM_IFACE_CARD);
  snd_ctl_elem_id_set_numid(id, numid);
  snd_ctl_elem_value_set_id(control, id);
  snd_ctl_elem_value_set_enumerated(control, 0, val);
  if ((err=snd_ctl_elem_write(ctlhandle, control)) < 0)
    printf("Control %s element write error: %s\n", card, snd_strerror(err));
  return(err);
}



// Read an enumerated ALSA control
int GetEnum(int numid) {
  int err, val;
  snd_ctl_elem_id_t *id;
  snd_ctl_elem_value_t *control;

  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_value_alloca(&control);
  snd_ctl_elem_id_set_interface(id, numid==clocksrcId ? SND_CTL_ELEM_IFACE_PCM : SND_CTL_ELEM_IFACE_CARD);
  snd_ctl_elem_id_set_numid(id, numid);
  snd_ctl_elem_value_set_id(control, id);
  if ((err=snd_ctl_elem_read(ctlhandle, control)) < 0)
    printf("Control %s element read error: %s\n", card, snd_strerror(err));
  val=snd_ctl_elem_value_get_enumerated(control, 0);
  return(val);
}



// Turn VU-meters on/off
void SetVUmeters(int onoff) {
  static signed char oncount=0;
  int err;
  snd_ctl_elem_id_t *id;
  snd_ctl_elem_value_t *control;

  if (onoff)
    oncount++;
  else
    if (--oncount<0)
      oncount=0;
  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_value_alloca(&control);
  snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_CARD);
  snd_ctl_elem_id_set_numid(id, vuswitchId);
  snd_ctl_elem_value_set_id(control, id);
  snd_ctl_elem_value_set_integer(control, 0, !!oncount);
  if ((err=snd_ctl_elem_write(ctlhandle, control)) < 0) {
    printf("Control %s element write error: %s\n", card, snd_strerror(err));
  }
}



void GetVUmeters(int *InLevel, int *InPeak, int *OutLevel, int *OutPeak, int *VirLevel, int *VirPeak) {
  int err, i, m;
  snd_ctl_elem_id_t *id;
  snd_ctl_elem_value_t *control;

  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_value_alloca(&control);
  snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
  snd_ctl_elem_id_set_numid(id, vumetersId);
  snd_ctl_elem_value_set_id(control, id);
  if ((err = snd_ctl_elem_read(ctlhandle, control)) < 0) {
    printf("Control %s element read error: %s\n", card, snd_strerror(err));
    return;
  }

  m=0;
  for (i=0; i<nLOut; i++) {
    OutLevel[i]=snd_ctl_elem_value_get_integer(control, m++);
    OutPeak[i]=snd_ctl_elem_value_get_integer(control, m++);
  }

  m=1*metersNumber*metersTypes;
  for (i=0; i<nIn; i++) {
    InLevel[i]=snd_ctl_elem_value_get_integer(control, m++);
    InPeak[i]=snd_ctl_elem_value_get_integer(control, m++);
  }
  
  if (metersStreams==3) {	// Has PCM levels (Mia only) ?
    m=2*metersNumber*metersTypes;
#ifdef REAL
    for (i=0; i<nPOut; i++) {
      VirLevel[i]=snd_ctl_elem_value_get_integer(control, m++);
      VirPeak[i]=snd_ctl_elem_value_get_integer(control, m++);
    }
#else
    for (i=0; i<nPOut; i++) {
      VirLevel[i]=i*5-100;
      VirPeak[i]=i*5-90;
    }
#endif
  }
}



#ifdef REVERSE

// Enable/disable widgets that control ADAT digital channels
void SetSensitivity(int enable) {
  int i;

  for (i=fdOut+2; i<nLOut; i++) {
    if (!enable && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mixerControl.outsel[i])))
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mixerControl.outsel[0]), TRUE);
    if (mixerId)
      gtk_widget_set_sensitive(mixerControl.outsel[i], enable);
    if (vmixerId) {
      if (!enable && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(vmixerControl.outsel[i])))
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(vmixerControl.outsel[0]), TRUE);
      gtk_widget_set_sensitive(vmixerControl.outsel[i], enable);
    }
    if (pcmoutId) {
      gtk_widget_set_sensitive(pcmoutControl.label[i], enable);
      gtk_widget_set_sensitive(pcmoutControl.volume[i], enable);
    }
    // Line-out control is always present
    gtk_widget_set_sensitive(lineoutControl.label[i], enable);
    gtk_widget_set_sensitive(lineoutControl.volume[i], enable);
  }
  for (i=fdIn+2; i<nIn; i++) {
    gtk_widget_set_sensitive(mixerControl.label[i], enable);
    gtk_widget_set_sensitive(mixerControl.volume[i], enable);
  }
  if (!enable && mixerControl.input>=fdIn+2)
    mixerControl.input=0;
}

#else // REVERSE

// Enable/disable widgets that control ADAT digital channels
void SetSensitivity(int enable) {
  int i;

  for (i=fdIn+2; i<nIn; i++) {
    if (!enable && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(mixerControl.inpsel[i])))
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mixerControl.inpsel[0]), TRUE);
    gtk_widget_set_sensitive(mixerControl.inpsel[i], enable);
  }
  for (i=fdOut+2; i<nLOut; i++) {
    if (mixerId) {
      gtk_widget_set_sensitive(mixerControl.label[i], enable);
      gtk_widget_set_sensitive(mixerControl.volume[i], enable);
    }
    if (vmixerId) {
      gtk_widget_set_sensitive(vmixerControl.label[i], enable);
      gtk_widget_set_sensitive(vmixerControl.volume[i], enable);
    }
    if (pcmoutId) {
      gtk_widget_set_sensitive(pcmoutControl.label[i], enable);
      gtk_widget_set_sensitive(pcmoutControl.volume[i], enable);
    }
    // Line-out control is always present
    gtk_widget_set_sensitive(lineoutControl.label[i], enable);
    gtk_widget_set_sensitive(lineoutControl.volume[i], enable);
  }
  if (!enable && mixerControl.output>=fdOut+2)
    mixerControl.output=0;
  if (vmixerId && !enable && vmixerControl.output>=fdOut+2)
    vmixerControl.output=0;
}

#endif // REVERSE



// Read current control settings.
int ReadControl(int *vol, int channels, int numid, int iface) {
  snd_ctl_elem_id_t *id;
  snd_ctl_elem_value_t *control;
  int err, ch;

  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_value_alloca(&control);
  snd_ctl_elem_id_set_interface(id, iface);
  snd_ctl_elem_id_set_numid(id, numid);
  snd_ctl_elem_value_set_id(control, id);
  if ((err=snd_ctl_elem_read(ctlhandle, control))<0) {
    printf("Control %s element read error: %s\n", card, snd_strerror(err));
    return(err);
  }

  for (ch=0; ch<channels; ch++)
    vol[ch]=snd_ctl_elem_value_get_integer(control, ch);
  return(0);
}



void ReadNominalLevels(struct NominalLevelControl_s *NominalLevel) {
  snd_ctl_elem_id_t *id;
  snd_ctl_elem_value_t *control;
  int err, i;

  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_value_alloca(&control);
  snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
  snd_ctl_elem_id_set_numid(id, NominalLevel->id);
  snd_ctl_elem_value_set_id(control, id);
  if ((err=snd_ctl_elem_read(ctlhandle, control))<0)
    printf("Control %s element read error: %s\n", card, snd_strerror(err));
  for (i=0; i<NominalLevel->Channels; i++)
    NominalLevel->Level[i]=snd_ctl_elem_value_get_integer(control, i);
}



int SetMixerGain(struct mixel *mxl, int Gain) {
  snd_ctl_elem_id_t *id;
  snd_ctl_elem_value_t *control;
  int err;

  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_value_alloca(&control);
  snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
  snd_ctl_elem_id_set_numid(id, mxl->id);
  snd_ctl_elem_value_set_id(control, id);
  snd_ctl_elem_value_set_integer(control, 0, Gain);
  if ((err = snd_ctl_elem_write(ctlhandle, control)) < 0) {
    printf("Control %s element write error: %s\n", card, snd_strerror(err));
    return(err);
  }
  return(0);
}



// Read current (v)mixer settings.
void ReadMixer(struct mixerControl_s *mixer) {
  int err, in, out;
  snd_ctl_elem_id_t *id;
  snd_ctl_elem_value_t *control;

#ifndef REAL
    return;
#endif

  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_value_alloca(&control);
  snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);

  for (out=0; out<mixer->outputs; out++) {
    for (in=0; in<mixer->inputs; in++) {
      snd_ctl_elem_id_set_numid(id, mixer->mixer[out][in].id);
      snd_ctl_elem_value_set_id(control, id);
      if ((err=snd_ctl_elem_read(ctlhandle, control))<0)
        printf("InitMixer - Control %s element read error: %s\n", card, snd_strerror(err));
      mixer->mixer[out][in].Gain=snd_ctl_elem_value_get_integer(control, 0);
    }
  }
}



void GetChannels(void) {
  snd_ctl_elem_id_t *id;
  snd_ctl_elem_value_t *control;
  int err;

  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_value_alloca(&control);
  snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_HWDEP);
  snd_ctl_elem_id_set_numid(id, channelsId);
  snd_ctl_elem_value_set_id(control, id);
  if ((err = snd_ctl_elem_read(ctlhandle, control)) < 0) {
    printf("GetChannels() read error: %s\n", snd_strerror(err));
    exit(1);
  }
  if (!nIn) {	// Only read the first time (mainly for debugging, see #define REAL)
    nIn=snd_ctl_elem_value_get_integer(control, 0);	// Number of input channels
    fdIn=snd_ctl_elem_value_get_integer(control, 1);	// First digital in (= number of analog input channels)
    nLOut=snd_ctl_elem_value_get_integer(control, 2);	// Number of output channels
    fdOut=snd_ctl_elem_value_get_integer(control, 3);	// First digital out
    nPOut=snd_ctl_elem_value_get_integer(control, 4);	// Number of virtual output channels (==nLOut on non-vmixer cards)

    mixerControl.outputs = nLOut;
    mixerControl.inputs = nIn;

    if (vmixerId) {
      vmixerControl.outputs = nLOut;
      vmixerControl.inputs = nPOut;

      /* For outputs and inputs. */
      metersStreams = 2;
    } else {
      /* For outputs, inputs and system outputs. */
      metersStreams = 3;
    }

    /* For the number of channels. */
    metersNumber = 16;
    /* For each of levels and peaks. */
    metersTypes = 2;
  }
  ClockMask=snd_ctl_elem_value_get_integer(control, 5);	// Bitmask of available input clocks
}



// Read what input clocks are valid and (de)activate the pop-down menu items accordingly
gint CheckInputs(gpointer unused) {
  int clk, valid, source;

  GetChannels();
  source=-1;

  // Switch to internal if the source is not available
  if (AutoClock>=0 && !(ClockMask & (1<<clocksrcVal)))
    source=0;

  for (clk=0; clk<nclocksrc; clk++) {
    valid=!!(ClockMask & (1<<clk));
    gtk_widget_set_sensitive(clocksrc_menuitem[clk], valid);
    if (clk==AutoClock && valid)
      source=AutoClock;
  }

  if (source>=0 && source!=clocksrcVal) {
    // Set the clock source, but do not change the value of AutoClock
    Clock_source_activate(clocksrc_menuitem[source], (gpointer)(long)(source|DONT_CHANGE));
    gtk_option_menu_set_history(GTK_OPTION_MENU(clocksrcOpt), clocksrcVal);
  }
  return(TRUE);
}



void DrawBar(int x, int y, int level, int peak, int gain) {
  GdkColor Bars={0x00FF00, 0, 0, 0};
  GdkColor Bars1={0x000000, 0, 0, 0};
  GdkColor Peak={0x1BABFF, 0, 0, 0};
  GdkColor Level={0xC0B000, 0, 0, 0};
  int db;

  x=XMETER+XCELLTOT*x;
  y=YCELLTOT*y+YCELLBORDER;

  if (level>ECHOGAIN_MUTED) {
    // Draw the "integer" part of the bar
    db=level>>2;
    gdk_gc_set_foreground(gc, &Bars);
    gdk_draw_rectangle(Mixpixmap, gc, TRUE, x, y-db, GM_BARWIDTH, YCELLDIM+db);

    // Draw the antialiased part
    Bars1.pixel=(level&3) << (6 + 8);	// 4 levels (256/4==64==2^6) of green (2^8)
    if (Bars1.pixel) {
      gdk_gc_set_foreground(gc, &Bars1);
      gdk_draw_rectangle(Mixpixmap, gc, TRUE, x, y-db-1, GM_BARWIDTH, 1);
    }
  }

  // Draw the peak
  if (peak>ECHOGAIN_MUTED) {
    db=peak>>2;
    gdk_gc_set_foreground(gc, &Peak);
    gdk_draw_rectangle(Mixpixmap, gc, TRUE, x, y-db, GM_BARWIDTH, 1);
  }

  // Draw the mixer gain
  if (gain>=ECHOGAIN_MUTED) {
    db=gain>>2;
    gdk_gc_set_foreground(gc, &Level);
    gdk_draw_rectangle(Mixpixmap, gc, TRUE, x-XMETER+XVOLUME, y, 1, YCELLDIM);
    gdk_draw_rectangle(Mixpixmap, gc, TRUE, x-XMETER+XVOLUME-2, y-db, 5, 1);
  }
}



// Draw the matrix mixer
gint DrawMixer(gpointer unused) {
  GdkRectangle update_rect;
  int InLevel[ECHO_MAXAUDIOINPUTS];
  int InPeak[ECHO_MAXAUDIOINPUTS];
  int OutLevel[ECHO_MAXAUDIOOUTPUTS];
  int OutPeak[ECHO_MAXAUDIOOUTPUTS];
  int VirLevel[ECHO_MAXAUDIOOUTPUTS];
  int VirPeak[ECHO_MAXAUDIOOUTPUTS];
  static int InClip[ECHO_MAXAUDIOINPUTS];
  static int OutClip[ECHO_MAXAUDIOOUTPUTS];
  char str[8];
  int i, o, dB;
  GdkColor Grid={0x787878, 0, 0, 0};
  GdkColor Labels={0x9694C4, 0, 0, 0};
  GdkColor Hilight={0x000078, 0, 0, 0};
  GdkColor Hilight2={0x600000, 0, 0, 0};

  if (!Mixpixmap)
    return(TRUE);

  update_rect.x = 0;
  update_rect.y = 0;
  update_rect.width = Mixwidth;
  update_rect.height = Mixheight;
  GetVUmeters(InLevel, InPeak, OutLevel, OutPeak, VirLevel, VirPeak);

  if (!gc) {
    gc=gdk_gc_new(gtk_widget_get_parent_window(Mixdarea));
    for (i=0; i<nIn; i++)
      InClip[i]=0;
    for (i=0; i<nLOut; i++)
      OutClip[i]=0;
  }

  gdk_draw_rectangle(Mixpixmap, Mixdarea->style->black_gc, TRUE, 0, 0, Mixwidth, Mixheight);

  // Highlight
  gdk_gc_set_foreground(gc, &Hilight);
  gdk_draw_rectangle(Mixpixmap, gc, TRUE, 0, YCELLTOT*mixerControl.input, XCELLTOT*(mixerControl.output+1), YCELLTOT);
  gdk_draw_rectangle(Mixpixmap, gc, TRUE, XCELLTOT*(mixerControl.output+1), YCELLTOT*mixerControl.input, XCELLTOT, Mixheight);
  if (vmixerId) {
    gdk_gc_set_foreground(gc, &Hilight2);
    gdk_draw_rectangle(Mixpixmap, gc, TRUE, 0, YCELLTOT*(GMixerSection.VmixerFirst+vmixerControl.input), XCELLTOT*(vmixerControl.output+1), YCELLTOT);
    gdk_draw_rectangle(Mixpixmap, gc, TRUE, XCELLTOT*(vmixerControl.output+1), YCELLTOT*(GMixerSection.VmixerFirst+vmixerControl.input), XCELLTOT, Mixheight);
  }

  // Draw the grid

  gdk_gc_set_font(gc, fnt);
  // Horizontal lines and input channel labels
  for (i=0; i<GMixerSection.LineOut; i++) {
    gdk_gc_set_foreground(gc, &Grid);
    gdk_draw_rectangle(Mixpixmap, gc, TRUE, 0, YCELLTOT*(i+1)-1, Mixwidth, 1);
    if (i<fdIn)
      sprintf(str, "A%d", i);		// Analog
    else if (i<nIn)
      sprintf(str, "D%d", i-fdIn);	// Digital
    else
      sprintf(str, "V%d", i-nIn);	// Virtual
    gdk_gc_set_foreground(gc, &Labels);
    gdk_draw_string(Mixpixmap, fnt, gc, 1, YCELLTOT*i+(YCELLTOT/2)+4, str);
  }

  // Vertical lines and output channel labels
  for (o=0; o<nLOut; o++) {
    gdk_gc_set_foreground(gc, &Grid);
    gdk_draw_rectangle(Mixpixmap, gc, TRUE, XCELLTOT*(o+1), 0, 1, Mixheight);
    if (o<fdOut)
      sprintf(str, "A%d", o);
    else
      sprintf(str, "D%d", o-fdOut);
    gdk_gc_set_foreground(gc, &Labels);
    gdk_draw_string(Mixpixmap, fnt, gc, XCELLTOT*(o+1)+(XCELLTOT/2)-6, YCELLTOT*GMixerSection.LineOut+YCELLTOT+8, str);
  }
  gdk_draw_string(Mixpixmap, fnt, gc, 1, 8, "In");
  gdk_draw_string(Mixpixmap, fnt, gc, 1, YCELLTOT*GMixerSection.LineOut+YCELLTOT+8, "Out");
  gdk_gc_set_foreground(gc, &Grid);
  gdk_draw_rectangle(Mixpixmap, gc, TRUE, 0, YCELLTOT*(GMixerSection.LineOut+1)-1, Mixwidth, 1);

  // Draw input levels and peaks
  for (i=0; i<GMixerSection.Inputs; i++)
    DrawBar(0, i, InLevel[i], InPeak[i], DONT_DRAW);

  // Draw vchannels levels and peaks (Vmixer cards only)
  if (vmixerId) {
    for (i=0; i<vmixerControl.inputs; i++)
      DrawBar(0, i+GMixerSection.VmixerFirst, VirLevel[i], VirPeak[i], DONT_DRAW);
  }

  // Draw output levels, peaks and volumes
  for (o=0; o<GMixerSection.Outputs; o++)
    DrawBar(o+1, GMixerSection.LineOut, OutLevel[o], OutPeak[o], lineoutControl.Gain[o]);

  // Draw monitor mixer elements
  for (o=0; o<GMixerSection.Outputs; o++) {
    for (i=0; i<GMixerSection.Inputs; i++) {
      dB=Add_dB(mixerControl.mixer[o][i].Gain, InLevel[i]);
      DrawBar(o+1, i, dB, DONT_DRAW, mixerControl.mixer[o][i].Gain);
    }
  }

  // Draw vmixer elements (Vmixer cards only)
  if (vmixerId) {
    for (o=0; o<GMixerSection.Outputs; o++)
      for (i=0; i<vmixerControl.inputs; i++) {
        dB=Add_dB(vmixerControl.mixer[o][i].Gain, VirLevel[i]);
        DrawBar(o+1, i+GMixerSection.VmixerFirst, dB, DONT_DRAW, vmixerControl.mixer[o][i].Gain);
      }
  }

  gtk_widget_draw(Mixdarea, &update_rect);
  return(TRUE);
}



// Draw the VU-meter
gint DrawVUmeters(gpointer unused) {
  GdkRectangle update_rect;
  int InLevel[ECHO_MAXAUDIOINPUTS];
  int InPeak[ECHO_MAXAUDIOINPUTS];
  int OutLevel[ECHO_MAXAUDIOOUTPUTS];
  int OutPeak[ECHO_MAXAUDIOOUTPUTS];
  int VirLevel[ECHO_MAXAUDIOOUTPUTS];
  int VirPeak[ECHO_MAXAUDIOOUTPUTS];
  static int InClip[ECHO_MAXAUDIOINPUTS];
  static int OutClip[ECHO_MAXAUDIOOUTPUTS];
  int i, x, dB;
  char str[16];
  GdkColor Selected={0xC86060, 0, 0, 0};
  GdkColor Grid={0x9694C4, 0, 0, 0};
  GdkColor Grid2={0x646383, 0, 0, 0};
  GdkColor dBValues={0x00B000, 0, 0, 0};
  GdkColor AnBars={0x00E0B8, 0, 0, 0};
  GdkColor DiBars={0x98E000, 0, 0, 0};
  GdkColor ClipPeak={0, 0, 0, 0};
  GdkColor Peak={0x00FF00, 0, 0, 0};

  if (!VUpixmap)
    return(TRUE);

  update_rect.x = 0;
  update_rect.y = 0;
  update_rect.width = VUwidth;
  update_rect.height = VUheight;
  GetVUmeters(InLevel, InPeak, OutLevel, OutPeak, VirLevel, VirPeak);

  if (!gc) {
    gc=gdk_gc_new(gtk_widget_get_parent_window(VUdarea));
    for (i=0; i<nIn; i++)
      InClip[i]=0;
    for (i=0; i<nLOut; i++)
      OutClip[i]=0;
  }

  // Clear the image
  gdk_draw_rectangle(VUpixmap, VUdarea->style->black_gc, TRUE, 0, 0, VUwidth, VUheight);

  // Draw the dB scale and the grid
  gdk_gc_set_font(gc, fnt);
  gdk_gc_set_foreground(gc, &Peak);
  gdk_draw_string(VUpixmap, fnt, gc, 2, VU_YGRAF-12+4, "  dB");
  for (i=0; i<=120; i+=12) {
    sprintf(str, "%4d", -i);
    gdk_gc_set_foreground(gc, &dBValues);
    gdk_draw_string(VUpixmap, fnt, gc, 2, VU_YGRAF+i+4, str);
    gdk_gc_set_foreground(gc, &Grid);
    gdk_draw_rectangle(VUpixmap, gc, TRUE, VU_XGRAF, VU_YGRAF+i, VUwidth-VU_XGRAF, 1);
  }
  gdk_gc_set_foreground(gc, &Grid2);
  gdk_draw_rectangle(VUpixmap, gc, TRUE, VU_XGRAF, VU_YGRAF+128, VUwidth-VU_XGRAF, 1);

  x=VU_XGRAF+VU_BARSEP;

  // Draw inputs
  for (i=0; i<nIn; i++) {
    if (i<fdIn)
      gdk_gc_set_foreground(gc, &AnBars);
    else
      gdk_gc_set_foreground(gc, &DiBars);
    dB=InLevel[i];
    gdk_draw_rectangle(VUpixmap, gc, TRUE, x, VU_YGRAF-dB, VU_BARWIDTH, 129+VU_YGRAF-(VU_YGRAF-dB));

    dB=InPeak[i];
    if (dB==0)
      InClip[i]=64;
    if (InClip[i]) {
      InClip[i]--;
      ClipPeak.pixel=(InClip[i]<<18)+((255-(InClip[i]*3))<<8);
      gdk_gc_set_foreground(gc, &ClipPeak);
    } else {
      gdk_gc_set_foreground(gc, &Peak);
    }
    gdk_draw_rectangle(VUpixmap, gc, TRUE, x, VU_YGRAF-dB, VU_BARWIDTH, 1);
    if (mixerControl.input==i) {
      gdk_gc_set_foreground(gc, &Selected);
      gdk_draw_rectangle(VUpixmap, gc, TRUE, x+1, VU_YGRAF+128+3, VU_BARWIDTH-2, 1);
      gdk_draw_rectangle(VUpixmap, gc, TRUE, x, VU_YGRAF+128+4, VU_BARWIDTH, 1);
    }
    x+=VU_BARWIDTH+VU_BARSEP;
  }

  // Draw outputs
  x+=VU_BARWIDTH+VU_BARSEP;
  for (i=0; i<nLOut; i++) {
    if (i<fdOut)
      gdk_gc_set_foreground(gc, &AnBars);
    else
      gdk_gc_set_foreground(gc, &DiBars);
    dB=OutLevel[i];
    gdk_draw_rectangle(VUpixmap, gc, TRUE, x, VU_YGRAF-dB, VU_BARWIDTH, 129+VU_YGRAF-(VU_YGRAF-dB));

    dB=OutPeak[i];
    if (dB==0)
      OutClip[i]=64;
    if (OutClip[i]) {
      OutClip[i]--;
      ClipPeak.pixel=(OutClip[i]<<18)+((255-(OutClip[i]*3))<<8);
      gdk_gc_set_foreground(gc, &ClipPeak);
    } else {
      gdk_gc_set_foreground(gc, &Peak);
    }
    gdk_draw_rectangle(VUpixmap, gc, TRUE, x, VU_YGRAF-dB, VU_BARWIDTH, 1);
    if (mixerControl.output==i) {
      gdk_gc_set_foreground(gc, &Selected);
      gdk_draw_rectangle(VUpixmap, gc, TRUE, x+1, VU_YGRAF+128+3, VU_BARWIDTH-2, 1);
      gdk_draw_rectangle(VUpixmap, gc, TRUE, x, VU_YGRAF+128+4, VU_BARWIDTH, 1);
    }
    x+=VU_BARWIDTH+VU_BARSEP;
  }

  gtk_widget_draw(VUdarea, &update_rect);

  return(TRUE);
}



///////////////////// GUI events


#ifdef REVERSE

void Mixer_Output_selector_clicked(GtkWidget *widget, gpointer och) {
  int ich, val;
  snd_ctl_elem_id_t *id;
  snd_ctl_elem_value_t *control;

  if (mixerControl.output==(int)och)
    return;

  mixerControl.output=(int)och;
  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_value_alloca(&control);

  snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
  for (ich=0; ich<nIn; ich++) {
    val=INVERT(mixerControl.mixer[mixerControl.output][ich].Gain);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(mixerControl.adj[ich]), (gfloat)val);
  }
}

#else // REVERSE

void Mixer_Input_selector_clicked(GtkWidget *widget, gpointer ich) {
  int och, val;
  snd_ctl_elem_id_t *id;
  snd_ctl_elem_value_t *control;

  if (mixerControl.input==(int)(long)ich)
    return;

  mixerControl.input=(int)(long)ich;
  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_value_alloca(&control);

  snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
  for (och=0; och<nLOut; och++) {
    val=INVERT(mixerControl.mixer[och][mixerControl.input].Gain);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(mixerControl.adj[och]), (gfloat)val);
  }
}

#endif // REVERSE


#ifdef REVERSE

static gint Gmixer_button_press(GtkWidget *widget, GdkEventButton *event) {

  GMixerRow=(int)event->y/YCELLTOT;
  GMixerColumn=(int)event->x/XCELLTOT-1;

  if (GMixerColumn<0 || GMixerColumn>=GMixerSection.Outputs)
    return TRUE;

  /* grab_focus must follow set_active because the latter causes
     Vmixer_*_selector_clicked() to be called and, in turn,
     Vmixer_volume_changed() which changes mixerControl.input
     (or .output in non-reverse mode). grab_focus then causes
     Vmixer_volume_clicked() to be called and that event handler
     finally sets the correct value of mixerControl.input */
  if (GMixerRow<GMixerSection.Inputs) {
    if (GMixerColumn!=mixerControl.output)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mixerControl.outsel[GMixerColumn]), TRUE);
    if (GMixerRow!=mixerControl.input)
      gtk_widget_grab_focus(GTK_WIDGET(mixerControl.volume[GMixerRow]));
  } else if (GMixerRow>=GMixerSection.VmixerFirst && GMixerRow<=GMixerSection.VmixerLast) {
    if (GMixerColumn!=vmixerControl.output)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(vmixerControl.outsel[GMixerColumn]), TRUE);
    if (GMixerRow!=vmixerControl.input)
      gtk_widget_grab_focus(GTK_WIDGET(vmixerControl.volume[GMixerRow-GMixerSection.VmixerFirst]));
  }

  if (event->button==1) {
    mouseY=event->y;
    mouseButton=1;
  }
  return(TRUE);
}

#else //REVERSE

static gint Gmixer_button_press(GtkWidget *widget, GdkEventButton *event) {

  GMixerRow=(int)event->y/YCELLTOT;
  GMixerColumn=(int)event->x/XCELLTOT-1;

  if (GMixerColumn<0 || GMixerColumn>=GMixerSection.Outputs)
    return TRUE;

  // See the note above
  if (GMixerRow<GMixerSection.VmixerFirst) {
    if (GMixerRow!=mixerControl.input)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(mixerControl.inpsel[GMixerRow]), TRUE);
    if (GMixerColumn!=mixerControl.output)
      gtk_widget_grab_focus(GTK_WIDGET(mixerControl.volume[GMixerColumn]));
  } else if (GMixerRow>=GMixerSection.VmixerFirst && GMixerRow<=GMixerSection.VmixerLast) {
    if (GMixerRow!=vmixerControl.input+GMixerSection.VmixerFirst)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(vmixerControl.vchsel[GMixerRow-GMixerSection.VmixerFirst]), TRUE);
    if (GMixerColumn!=vmixerControl.output)
      gtk_widget_grab_focus(GTK_WIDGET(vmixerControl.volume[GMixerColumn]));
  }

  if (event->button==1) {
    mouseY=event->y;
    mouseButton=1;
  }
  return(TRUE);
}

#endif //REVERSE



static gint Gmixer_button_release(GtkWidget *widget, GdkEventButton *event) {

  if (event->state & GDK_BUTTON1_MASK)
    mouseButton=0;
  return TRUE;
}



// Gets how many pixels the mouse pointer was moved of and updates
// the currently active mixer/vmixer/line_out control.
static gint Gmixer_motion_notify(GtkWidget *widget, GdkEventMotion *event) {
  int x, y;
  GdkModifierType state;
  float val;

  if (event->is_hint)
    gdk_window_get_pointer(event->window, &x, &y, &state);
  else {
    x=event->x;
    y=event->y;
    state=event->state;
  }

  // Check if the button is still pressed because the release event can
  // fall in another window, so we may miss it. Ignore the event if there
  // isn't a backing pixmap or the user clicked an invalid cell. We also
  // have to check mouseButton because the button_press event may arrive
  // after the respective motion_notify event.
  if (!(state & GDK_BUTTON1_MASK) || !mouseButton || !Mixpixmap || GMixerColumn<0 || GMixerColumn>=GMixerSection.Outputs) {
    mouseButton=0;
    return(TRUE);
  }

  if (GMixerRow<GMixerSection.Inputs) {
    val=INVERT(mixerControl.mixer[mixerControl.output][mixerControl.input].Gain);
    val+=y-mouseY;
    mouseY=y;
    // Gtk already limits the range of "val"
#ifdef REVERSE
    gtk_adjustment_set_value(GTK_ADJUSTMENT(mixerControl.adj[mixerControl.input]), (gfloat)val);
#else
    gtk_adjustment_set_value(GTK_ADJUSTMENT(mixerControl.adj[mixerControl.output]), (gfloat)val);
#endif
  } else if (GMixerRow>=GMixerSection.VmixerFirst && GMixerRow<=GMixerSection.VmixerLast) {
    val=INVERT(vmixerControl.mixer[vmixerControl.output][vmixerControl.input].Gain);
    val+=y-mouseY;
    mouseY=y;
#ifdef REVERSE
    gtk_adjustment_set_value(GTK_ADJUSTMENT(vmixerControl.adj[vmixerControl.input]), (gfloat)val);
#else
    gtk_adjustment_set_value(GTK_ADJUSTMENT(vmixerControl.adj[vmixerControl.output]), (gfloat)val);
#endif
  } else if (GMixerRow==GMixerSection.LineOut) {
    val=INVERT(lineoutControl.Gain[GMixerColumn]);
    val+=y-mouseY;
    mouseY=y;
    gtk_adjustment_set_value(GTK_ADJUSTMENT(lineoutControl.adj[GMixerColumn]), (gfloat)val);
  }

  return(TRUE);
}



void Monitor_volume_changed(GtkWidget *widget, gpointer cnl) {
  int val, rval, ch;
  int i, o;
  char str[16];

  UI_DEBUG(("Monitor_volume_changed()  %d %d\n",mixerControl.input,mixerControl.output));
  val=rval=INVERT((int)GTK_ADJUSTMENT(widget)->value);

  ch=(int)(long)cnl;

#ifdef REVERSE
  i=ch;
  o=mixerControl.output;
#else
  i=mixerControl.input;
  o=ch;
#endif

  // Emulate the line-out volume if this card can't do it in hw.
  if (!lineoutId) {
    rval=Add_dB(val, lineoutControl.Gain[o]);
    ClampOutputVolume(&rval);
  }

  SetMixerGain(&mixerControl.mixer[o][i], rval);	//@ we should restore the old adj position on error
  mixerControl.mixer[o][i].Gain=val;

  if (Gang) {
    SetMixerGain(&mixerControl.mixer[o^1][i^1], rval);
    mixerControl.mixer[o^1][i^1].Gain=val;
  }

  gtk_label_set_text(GTK_LABEL(mixerControl.label[ch]), strOutGain(str, val));
}



void Monitor_volume_clicked(GtkWidget *widget, gpointer ch) {

#ifdef REVERSE
  mixerControl.input=(int)(long)ch;
#else
  mixerControl.output=(int)(long)ch;
#endif
}



void Gang_button_toggled(GtkWidget *widget, gpointer unused) {

  Gang=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}



void PCM_volume_changed(GtkWidget *widget, gpointer ch) {
  snd_ctl_elem_id_t *id;
  snd_ctl_elem_value_t *control;
  char str[16];
  int err, channel, val, rval;
  struct VolumeControl_s *vol;

  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_value_alloca(&control);

  if ((int)(long)ch<ECHO_MAXAUDIOINPUTS) {
    // Input
    channel=(int)(long)ch;
    vol=&lineinControl;
    rval=val=IN_INVERT((int)GTK_ADJUSTMENT(widget)->value);
    sprintf(str, "%+4.1f", 0.5*val);
  } else {
    // Output
    channel=(int)(long)ch-ECHO_MAXAUDIOINPUTS;
    vol=&pcmoutControl;
    val=rval=INVERT((int)GTK_ADJUSTMENT(widget)->value);
    pcmoutControl.Gain[channel]=val;
    // Emulate the line-out volume if this card can't do it in hw.
    if (!lineoutId) {
      rval=Add_dB(val, lineoutControl.Gain[channel]);
      ClampOutputVolume(&rval);
    }
    strOutGain(str, val);
  }

  gtk_label_set_text(GTK_LABEL(vol->label[channel]), str);

  snd_ctl_elem_id_set_numid(id, vol->id);
  snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
  snd_ctl_elem_value_set_id(control, id);

  if ((err=snd_ctl_elem_read(ctlhandle, control))<0) {
    printf("Control %s element read error: %s\n", card, snd_strerror(err));
    return;
  }

  snd_ctl_elem_value_set_integer(control, channel, rval);
  if ((err=snd_ctl_elem_write(ctlhandle, control))<0) {
    printf("Control %s element write error: %s\n", card, snd_strerror(err));
  } else {
    vol->Gain[channel]=val;
  }
  if (Gang)
    gtk_adjustment_set_value(GTK_ADJUSTMENT(vol->adj[channel^1]), (gfloat)GTK_ADJUSTMENT(widget)->value);
}



// Changes the PCM volume according to the current Line-out volume for non-vmixer cards
void UpdatePCMVolume(int outchannel) {
  snd_ctl_elem_id_t *id;
  snd_ctl_elem_value_t *control;
  int err, val;

  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_value_alloca(&control);
  snd_ctl_elem_id_set_numid(id, pcmoutId);
  snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
  snd_ctl_elem_value_set_id(control, id);

  if ((err=snd_ctl_elem_read(ctlhandle, control))<0)
    printf("Control %s element read error: %s\n", card, snd_strerror(err));

  val=Add_dB(pcmoutControl.Gain[outchannel], lineoutControl.Gain[outchannel]);
  ClampOutputVolume(&val);

  snd_ctl_elem_value_set_integer(control, outchannel, val);
  if ((err=snd_ctl_elem_write(ctlhandle, control))<0)
    printf("Control %s element write error: %s\n", card, snd_strerror(err));
}



// Changes the monitor mixer volume according to the current Line-out volume for non-vmixer cards.
void UpdateMixerVolume(int outchannel) {
  snd_ctl_elem_id_t *id;
  snd_ctl_elem_value_t *control;
  int err, val, ch;

  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_value_alloca(&control);
  snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);

  for (ch=0; ch<nIn; ch++) {
    val=Add_dB(mixerControl.mixer[outchannel][ch].Gain, lineoutControl.Gain[outchannel]);
    ClampOutputVolume(&val);
    snd_ctl_elem_id_set_numid(id, mixerControl.mixer[outchannel][ch].id);
    snd_ctl_elem_value_set_id(control, id);
    snd_ctl_elem_value_set_integer(control, 0, val);
    if ((err = snd_ctl_elem_write(ctlhandle, control)) < 0)
      printf("Control %s element write error: %s\n", card, snd_strerror(err));
  }
}



// Changes the vmixer volume according to the current Line-out volume for vmixer cards.
void UpdateVMixerVolume(int outchannel) {
  snd_ctl_elem_id_t *id;
  snd_ctl_elem_value_t *control;
  int err, val, ch;

  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_value_alloca(&control);
  snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);

  for (ch=0; ch<nPOut; ch++) {
    val=Add_dB(vmixerControl.mixer[outchannel][ch].Gain, lineoutControl.Gain[outchannel]);
    ClampOutputVolume(&val);
    snd_ctl_elem_id_set_numid(id, vmixerControl.mixer[outchannel][ch].id);
    snd_ctl_elem_value_set_id(control, id);
    snd_ctl_elem_value_set_integer(control, 0, val);
    if ((err = snd_ctl_elem_write(ctlhandle, control)) < 0)
      printf("Control %s element write error: %s\n", card, snd_strerror(err));
  }
}



void LineOut_volume_changed(GtkWidget *widget, gpointer ch) {
  char str[16];
  int err, channel, val;

  channel=(int)(long)ch;

  val=INVERT((int)GTK_ADJUSTMENT(widget)->value);
  lineoutControl.Gain[channel]=val;

  gtk_label_set_text(GTK_LABEL(lineoutControl.label[channel]), strOutGain(str, val));

  if (lineoutId) {	// If this card has the line-out control, use it
    snd_ctl_elem_id_t *id;
    snd_ctl_elem_value_t *control;

    snd_ctl_elem_id_alloca(&id);
    snd_ctl_elem_value_alloca(&control);
    snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
    snd_ctl_elem_id_set_numid(id, lineoutId);
    snd_ctl_elem_value_set_id(control, id);

    if ((err=snd_ctl_elem_read(ctlhandle, control))<0)
      printf("Control %s element read error: %s\n", card, snd_strerror(err));
     snd_ctl_elem_value_set_integer(control, channel, val);
     if ((err=snd_ctl_elem_write(ctlhandle, control))<0)
       printf("Control %s element write error: %s\n", card, snd_strerror(err));
  } else if (vmixerId) {
    UpdateVMixerVolume(channel);
    UpdateMixerVolume(channel);
  } else {		// Otherwise we have to emulate it.
    UpdatePCMVolume(channel);
    UpdateMixerVolume(channel);
  }

  if (Gang)
    gtk_adjustment_set_value(GTK_ADJUSTMENT(lineoutControl.adj[channel^1]), (gfloat)GTK_ADJUSTMENT(widget)->value);
}



void Vmixer_volume_changed(GtkWidget *widget, gpointer ch) {
  char str[16];
  int val, rval, channel;
  int o, v;

  channel=(int)(long)ch;
  val=rval=INVERT((int)GTK_ADJUSTMENT(widget)->value);

#ifdef REVERSE
  v=channel;
  o=vmixerControl.output;
#else
  v=vmixerControl.input;
  o=channel;
#endif

  // Emulate the line-out volume if this card can't do it in hw.
  if (!lineoutId) {
    rval=Add_dB(val, lineoutControl.Gain[o]);
    ClampOutputVolume(&rval);
  }

  SetMixerGain(&vmixerControl.mixer[o][v], rval);
  vmixerControl.mixer[o][v].Gain=val;

  if (Gang) {
    SetMixerGain(&vmixerControl.mixer[o^1][v^1], rval);
    vmixerControl.mixer[o^1][v^1].Gain=val;
  }
  
  gtk_label_set_text(GTK_LABEL(vmixerControl.label[channel]), strOutGain(str, val));
}



void Vmixer_volume_clicked(GtkWidget *widget, gpointer ch) {

#ifdef REVERSE
  vmixerControl.input=(int)(long)ch;
  UI_DEBUG(("Vmixer_volume_clicked vch=%d\n",vmixerControl.input));
#else
  vmixerControl.output=(int)(long)ch;
  UI_DEBUG(("Vmixer_volume_clicked out=%d\n",vmixerControl.output));
#endif
}



#ifdef REVERSE

void Vmixer_output_selector_clicked(GtkWidget *widget, gpointer ch) {
  int c, val;
  snd_ctl_elem_id_t *id;
  snd_ctl_elem_value_t *control;

  if (vmixerControl.output==(int)ch)
    return;
  vmixerControl.output=(int)ch;

  UI_DEBUG(("Vmixer_selector_clicked out=%d\n",vmixerControl.output));
  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_value_alloca(&control);
  snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
  for (c=vmixerControl.inputs-1; c>=0; c--) {
    val=INVERT(vmixerControl.mixer[vmixerControl.output][c].Gain);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(vmixerControl.adj[c]), (gfloat)val);
  }
}

#else

void Vmixer_vchannel_selector_clicked(GtkWidget *widget, gpointer ch) {
  int c, val;
  snd_ctl_elem_id_t *id;
  snd_ctl_elem_value_t *control;

  if (vmixerControl.input==(int)(long)ch)
    return;
  vmixerControl.input=(int)(long)ch;

  UI_DEBUG(("Vmixer_selector_clicked vch=%d\n",vmixerControl.input));
  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_value_alloca(&control);
  snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
  for (c=vmixerControl.outputs-1; c>=0; c--) {
    val=INVERT(vmixerControl.mixer[c][vmixerControl.input].Gain);
    gtk_adjustment_set_value(GTK_ADJUSTMENT(vmixerControl.adj[c]), (gfloat)val);
  }
}

#endif



void Nominal_level_toggled(GtkWidget *widget, gpointer ch) {
  snd_ctl_elem_id_t *id;
  snd_ctl_elem_value_t *control;
  int err, val, channel;
  struct NominalLevelControl_s *NominalLevel;

  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_value_alloca(&control);
  snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);

  val=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  if ((int)(long)ch<ECHO_MAXAUDIOINPUTS) {
    channel=(int)(long)ch;
    NominalLevel=&NominalIn;
  } else {
    channel=(int)(long)ch-ECHO_MAXAUDIOINPUTS;
    NominalLevel=&NominalOut;
  }
  NominalLevel->Level[channel]=!val;

  snd_ctl_elem_id_set_numid(id, NominalLevel->id);
  snd_ctl_elem_value_set_id(control, id);
  if ((err=snd_ctl_elem_read(ctlhandle, control))<0)
    printf("Control %s element read error: %s\n", card, snd_strerror(err));
  snd_ctl_elem_value_set_integer(control, channel, !val);	// FALSE is +4
  if ((err=snd_ctl_elem_write(ctlhandle, control))<0)
    printf("Control %s element write error: %s\n", card, snd_strerror(err));
  if (Gang)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(NominalLevel->Button[channel^1]), val);
}



void Switch_toggled(GtkWidget *widget, gpointer Ctl) {
  snd_ctl_elem_id_t *id;
  snd_ctl_elem_value_t *control;
  int err;
  struct SwitchControl_s *Switch=(struct SwitchControl_s *)Ctl;

  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_value_alloca(&control);

  Switch->value=gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
  snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_CARD);
  snd_ctl_elem_id_set_numid(id, Switch->id);
  snd_ctl_elem_value_set_id(control, id);
  snd_ctl_elem_value_set_integer(control, 0, Switch->value);
  if ((err=snd_ctl_elem_write(ctlhandle, control))<0)
    printf("Control %s element write error: %s\n", card, snd_strerror(err));
}



void AutoClock_toggled(GtkWidget *widget, gpointer unused) {
  char str[32];

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
    AutoClock=clocksrcVal;
    snprintf(str, 31, "Autoclock [%s]", clocksrcName[AutoClock]);
    str[31]=0;
    gtk_label_set_text(GTK_LABEL(GTK_BIN(widget)->child), str);
  } else {
    AutoClock=-1;
    gtk_label_set_text(GTK_LABEL(GTK_BIN(widget)->child), "Autoclock");
  }

}



void Digital_mode_activate(GtkWidget *widget, gpointer mode) {
  int adat;

  if (SetEnum(dmodeId, (int)(long)mode)<0) {
    // Restore old value if it failed
    gtk_option_menu_set_history(GTK_OPTION_MENU(dmodeOpt), dmodeVal);
    return;
  }

  dmodeVal=(int)(long)mode;
  // When I change the digital mode, the clock source can change too
  clocksrcVal=GetEnum(clocksrcId);
  gtk_option_menu_set_history(GTK_OPTION_MENU(clocksrcOpt), clocksrcVal);

  adat=!memcmp(dmodeName[dmodeVal], "ADAT", 4);
  SetSensitivity(adat);
  if (adat) {
    GMixerSection.Inputs=nIn;
    GMixerSection.Outputs=nLOut;
  } else {
    GMixerSection.Inputs=fdIn+2;	// S/PDIF has only 2 channels
    GMixerSection.Outputs=fdOut+2;
  }
}



void Clock_source_activate(GtkWidget *widget, gpointer clk) {
  unsigned int source;

  source=(unsigned int)(long)clk & 0xff;
  if (SetEnum(clocksrcId, source)<0) {
    gtk_option_menu_set_history(GTK_OPTION_MENU(clocksrcOpt), clocksrcVal);
  } else {
    clocksrcVal=(int)(long)clk & 0xff;
    // Change only when the user triggers it
    if (((int)(long)clk & DONT_CHANGE)==0 && AutoClock>=0) {
      AutoClock=clocksrcVal;
      AutoClock_toggled(autoclockChkbutton, NULL);
    }
  }
}



void SPDIF_mode_activate(GtkWidget *widget, gpointer mode) {

  SetEnum(spdifmodeId, (int)(long)mode);	// This one should never fail
  spdifmodeVal=(int)(long)mode;
}



// Create a new backing pixmap of the appropriate size
static gint VU_configure_event(GtkWidget *widget, GdkEventConfigure *event) {

  if (VUpixmap)
    gdk_pixmap_unref(VUpixmap);
  VUpixmap=gdk_pixmap_new(widget->window, widget->allocation.width, widget->allocation.height, -1);
  gdk_draw_rectangle(VUpixmap, widget->style->black_gc, TRUE, 0, 0, widget->allocation.width, widget->allocation.height);
  return(TRUE);
}



// Redraw the screen from the backing pixmap
static gint VU_expose(GtkWidget *widget, GdkEventExpose *event) {

  if (VUpixmap)
    gdk_draw_pixmap(widget->window, widget->style->fg_gc[GTK_WIDGET_STATE(widget)], VUpixmap,
                    event->area.x, event->area.y,
                    event->area.x, event->area.y,
                    event->area.width, event->area.height);
  return(FALSE);
}



// Create a new backing pixmap of the appropriate size
static gint Gmixer_configure_event(GtkWidget *widget, GdkEventConfigure *event) {

  if (Mixpixmap)
    gdk_pixmap_unref(Mixpixmap);
  Mixpixmap=gdk_pixmap_new(widget->window, widget->allocation.width, widget->allocation.height, -1);
  gdk_draw_rectangle(Mixpixmap, widget->style->black_gc, TRUE, 0, 0, widget->allocation.width, widget->allocation.height);
  return(TRUE);
}



// Redraw the screen from the backing pixmap
static gint Gmixer_expose(GtkWidget *widget, GdkEventExpose *event) {

  if (Mixpixmap)
    gdk_draw_pixmap(widget->window, widget->style->fg_gc[GTK_WIDGET_STATE(widget)], Mixpixmap,
                    event->area.x, event->area.y,
                    event->area.x, event->area.y,
                    event->area.width, event->area.height);
  return(FALSE);
}



gint CloseWindow(GtkWidget *widget, GdkEvent *event, gpointer geom) {
  struct geometry *g=geom;

  gdk_window_get_root_origin(widget->window, &g->x, &g->y);
  gdk_window_get_size(widget->window, &g->w, &g->h);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g->toggler), FALSE);	// This hides the window
  //gtk_widget_set_uposition(widget, g->x, g->y);
  return(TRUE);		// Do not destroy it
}



gint Mainwindow_delete(GtkWidget *widget, GdkEvent *event, gpointer geom) {
  struct geometry *g=geom;

  if (VUwindow) {
    gdk_window_get_root_origin(VUwindow->window, &VUw_geom.x, &VUw_geom.y);
    gtk_widget_destroy(VUwindow);
  }
  if (GMwindow) {
    gdk_window_get_root_origin(GMwindow->window, &GMw_geom.x, &GMw_geom.y);
    gtk_widget_destroy(GMwindow);
  }
  gdk_window_get_root_origin(Mainwindow->window, &g->x, &g->y);
  gtk_main_quit();
  return(FALSE);
}



gint VUwindow_delete(GtkWidget *widget, GdkEvent *event, gpointer geom) {
  struct geometry *g=geom;

  gdk_window_get_root_origin(widget->window, &g->x, &g->y);
  g->st=0;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g->toggler), FALSE);
  return(FALSE);
}



gint VUwindow_destroy(GtkWidget *widget, gpointer unused) {

  SetVUmeters(0);
  gtk_timeout_remove(VUtimer);
  //@@@del gc and fnt
  VUwindow=0;
  return(TRUE);
}



gint GMwindow_delete(GtkWidget *widget, GdkEvent *event, gpointer geom) {
  struct geometry *g=geom;

  gdk_window_get_root_origin(widget->window, &g->x, &g->y);
  g->st=0;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g->toggler), FALSE);
  return(FALSE);
}



gint GMwindow_destroy(GtkWidget *widget, gpointer unused) {

  SetVUmeters(0);
  gtk_timeout_remove(Mixtimer);
  //@@@del gc and fnt
  GMwindow=0;
  return(TRUE);
}



void VUmeters_button_click(GtkWidget *widget, gpointer unused) {
  char str[64];

  if (VUwindow && !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
    VUw_geom.st=0;
    gtk_widget_destroy(VUwindow);
  } else if (!VUwindow && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
    // Create VU-meter window
    VUwidth=VU_XGRAF+(VU_BARWIDTH+VU_BARSEP)*(nIn+nLOut+1)+VU_BARSEP;
    VUheight=160;
    SetVUmeters(1);
    VUwindow=gtk_window_new(GTK_WINDOW_TOPLEVEL);
    sprintf(str, "%s VU-meters", cardId);
    gtk_window_set_title (GTK_WINDOW (VUwindow), str);
    gtk_window_set_wmclass(GTK_WINDOW(VUwindow), "vumeters", "Emixer");
    gtk_signal_connect(GTK_OBJECT(VUwindow), "destroy", GTK_SIGNAL_FUNC(VUwindow_destroy), NULL);
    gtk_signal_connect(GTK_OBJECT(VUwindow), "delete_event", GTK_SIGNAL_FUNC(VUwindow_delete), (gpointer)&VUw_geom);
    gtk_window_set_policy(GTK_WINDOW(VUwindow), FALSE, FALSE, TRUE);
    if (VUw_geom.st!=NOPOS)
      gtk_widget_set_uposition(VUwindow, VUw_geom.x, VUw_geom.y);
    gtk_widget_show(VUwindow);

    VUdarea=gtk_drawing_area_new();
    gtk_widget_set_events(VUdarea, GDK_EXPOSURE_MASK);
    gtk_drawing_area_size(GTK_DRAWING_AREA(VUdarea), VUwidth, VUheight);
    gtk_container_add(GTK_CONTAINER(VUwindow), VUdarea);

    gtk_widget_show(VUdarea);
    gtk_signal_connect(GTK_OBJECT(VUdarea), "expose_event", (GtkSignalFunc)VU_expose, NULL);
    gtk_signal_connect(GTK_OBJECT(VUdarea), "configure_event", (GtkSignalFunc)VU_configure_event, NULL);
    VUtimer=gtk_timeout_add(30, DrawVUmeters, 0);	// The hw updates the meters about 30 times/s
    gdk_window_clear_area(VUdarea->window, 0, 0, VUwidth, VUheight);
    VUw_geom.st=1;
  }
}



void GMixer_button_click(GtkWidget *widget, gpointer unused) {
  char str[64];

  if (GMwindow && !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
    GMw_geom.st=0;
    gtk_widget_destroy(GMwindow);
  } else if (!GMwindow && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget))) {
    // Create graphic mixer window
    Mixwidth=XCELLTOT*(nLOut+1);
    Mixheight=YCELLTOT*(GMixerSection.LineOut+1)+9;
    SetVUmeters(1);
    GMwindow=gtk_window_new(GTK_WINDOW_TOPLEVEL);
    sprintf(str, "%s Mixer", cardId);
    gtk_window_set_title (GTK_WINDOW (GMwindow), str);
    gtk_window_set_wmclass(GTK_WINDOW(GMwindow), "gridmixer", "Emixer");
    gtk_signal_connect(GTK_OBJECT(GMwindow), "destroy", GTK_SIGNAL_FUNC(GMwindow_destroy), NULL);
    gtk_signal_connect(GTK_OBJECT(GMwindow), "delete_event", GTK_SIGNAL_FUNC(GMwindow_delete), (gpointer)&GMw_geom);
    gtk_window_set_policy(GTK_WINDOW(GMwindow), FALSE, FALSE, TRUE);
    if (GMw_geom.st!=NOPOS)
      gtk_widget_set_uposition(GMwindow, GMw_geom.x, GMw_geom.y);
    gtk_widget_show(GMwindow);

    Mixdarea=gtk_drawing_area_new();
    gtk_widget_set_events(Mixdarea, GDK_EXPOSURE_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);
    gtk_drawing_area_size(GTK_DRAWING_AREA(Mixdarea), Mixwidth, Mixheight);
    gtk_container_add(GTK_CONTAINER(GMwindow), Mixdarea);

    gtk_widget_show(Mixdarea);
    gtk_signal_connect(GTK_OBJECT(Mixdarea), "expose_event", (GtkSignalFunc)Gmixer_expose, NULL);
    gtk_signal_connect(GTK_OBJECT(Mixdarea), "configure_event", (GtkSignalFunc)Gmixer_configure_event, NULL);
    gtk_signal_connect(GTK_OBJECT(Mixdarea), "motion_notify_event", (GtkSignalFunc)Gmixer_motion_notify, NULL);
    gtk_signal_connect(GTK_OBJECT(Mixdarea), "button_press_event", (GtkSignalFunc)Gmixer_button_press, NULL);
    gtk_signal_connect(GTK_OBJECT(Mixdarea), "button_release_event", (GtkSignalFunc)Gmixer_button_release, NULL);
    Mixtimer=gtk_timeout_add(30, DrawMixer, 0);		// The hw updates the meters about 30 times/s
    gdk_window_clear_area(Mixdarea->window, 0, 0, Mixwidth, Mixheight);
    GMw_geom.st=1;
  }
}



void ToggleWindow(GtkWidget *widget, gpointer window) {

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
    gtk_widget_show(GTK_WIDGET(window));
  else
    gtk_widget_hide(GTK_WIDGET(window));
}



// Scan all controls and sets up the structures needed to access them.
int OpenControls(const char *card, const char *cardname) {
  int err, i, o;
  int numid, count, items, item;
  snd_hctl_t *handle;
  snd_hctl_elem_t *elem;
  snd_ctl_elem_id_t *id;
  snd_ctl_elem_info_t *info;

  pcmoutId=lineoutId=vmixerId=p4InId=p4OutId=dmodeId=clocksrcId=spdifmodeId=vuswitchId=vumetersId=mixerId=0;
  memset(&vmixerControl, 0, sizeof(vmixerControl));
  memset(&mixerControl, 0, sizeof(mixerControl));
  memset(&lineoutControl, 0, sizeof(struct VolumeControl_s));
  memset(&lineinControl, 0, sizeof(struct VolumeControl_s));
  memset(&pcmoutControl, 0, sizeof(struct VolumeControl_s));
  ndmodes=nclocksrc=nspdifmodes=0;
  snd_ctl_elem_id_alloca(&id);
  snd_ctl_elem_info_alloca(&info);

  if ((err=snd_hctl_open(&handle, card, 0))<0) {
    printf("Control %s open error: %s", card, snd_strerror(err));
    return err;
  }
  if ((err=snd_hctl_load(handle))<0) {
    printf("Control %s local error: %s\n", card, snd_strerror(err));
    return err;
  }
  for (elem=snd_hctl_first_elem(handle); elem; elem=snd_hctl_elem_next(elem)) {
    if ((err=snd_hctl_elem_info(elem, info))<0) {
      printf("Control %s snd_hctl_elem_info error: %s\n", card, snd_strerror(err));
      return err;
    }
    if (snd_ctl_elem_info_is_inactive(info))
      continue;
    snd_hctl_elem_get_id(elem, id);
    numid=snd_ctl_elem_id_get_numid(id);
    count=snd_ctl_elem_info_get_count(info);
    if (!strcmp("Monitor Mixer Volume", snd_ctl_elem_id_get_name(id))) {
      if (!mixerId) {
        mixerId=numid;
        CTLID_DEBUG(("First Mixer id=%d\n", mixerId));
      }
    } else if (!strcmp("VMixer Volume", snd_ctl_elem_id_get_name(id))) {
      if (!vmixerId) {
        vmixerId=vmixerControl.id=numid;
        CTLID_DEBUG(("First Vmixer id=%d\n", vmixerId));
      }
    } else if (!strcmp("PCM Playback Volume", snd_ctl_elem_id_get_name(id))) {
      pcmoutId=pcmoutControl.id=numid;
      CTLID_DEBUG(("PCM Playback Volume id=%d [%d]\n", pcmoutId, count));
    } else if (!strcmp("Line Playback Volume", snd_ctl_elem_id_get_name(id))) {
      lineoutId=numid;
      CTLID_DEBUG(("Line Volume id=%d\n", lineoutId));
    } else if (!strcmp("Line Capture Volume", snd_ctl_elem_id_get_name(id))) {
      lineinId=lineinControl.id=numid;
      CTLID_DEBUG(("Capture Volume id=%d [%d]\n", lineinId, count));
    } else if (!strcmp("Line Playback Switch (-10dBV)", snd_ctl_elem_id_get_name(id))) {
      p4OutId=NominalOut.id=numid;
      CTLID_DEBUG(("Playback nominal id=%d [%d]\n", p4OutId, count));
    } else if (!strcmp("Line Capture Switch (-10dBV)", snd_ctl_elem_id_get_name(id))) {
      p4InId=NominalIn.id=numid;
      CTLID_DEBUG(("Capture nominal id=%d [%d]\n", p4InId, count));
    } else if (!strcmp("Digital mode Switch", snd_ctl_elem_id_get_name(id))) {
      dmodeId=numid;
      items=snd_ctl_elem_info_get_items(info);
      ndmodes=items;
      for (item=0; item<items; item++) {
        snd_ctl_elem_info_set_item(info, item);
        if ((err=snd_hctl_elem_info(elem, info)) < 0) {
          printf("Control %s element info error: %s\n", card, snd_strerror(err));
          exit(err);
        }
        strncpy(dmodeName[item], snd_ctl_elem_info_get_item_name(info), 63);
        dmodeName[item][63]=0;
        CTLID_DEBUG(("Digital Mode id=%d item #%u '%s'\n", numid, item, snd_ctl_elem_info_get_item_name(info)));
      }
    } else if (!strcmp("Sample Clock Source", snd_ctl_elem_id_get_name(id))) {
      clocksrcId=numid;
      items=snd_ctl_elem_info_get_items(info);
      nclocksrc=items;
      for (item=0; item<items; item++) {
        snd_ctl_elem_info_set_item(info, item);
        if ((err=snd_hctl_elem_info(elem, info))<0) {
          printf("Control %s element info error: %s\n", card, snd_strerror(err));
          exit(err);
        }
        strncpy(clocksrcName[item], snd_ctl_elem_info_get_item_name(info), 63);
        clocksrcName[item][63]=0;
        CTLID_DEBUG(("Clock source id=%d item #%u '%s'\n", numid, item, snd_ctl_elem_info_get_item_name(info)));
      }
    } else if (!strcmp("S/PDIF mode Switch", snd_ctl_elem_id_get_name(id))) {
      spdifmodeId=numid;
      items=snd_ctl_elem_info_get_items(info);
      nspdifmodes=items;
      for (item=0; item<items; item++) {
        snd_ctl_elem_info_set_item(info, item);
        if ((err=snd_hctl_elem_info(elem, info)) < 0) {
          printf("Control %s element info error: %s\n", card, snd_strerror(err));
        }
        strncpy(spdifmodeName[item], snd_ctl_elem_info_get_item_name(info), 63);
        spdifmodeName[item][63]=0;
        CTLID_DEBUG(("S/PDIF Mode id=%d item #%u '%s'\n", numid, item, snd_ctl_elem_info_get_item_name(info)));
      }
    } else if (!strcmp("Phantom power Switch", snd_ctl_elem_id_get_name(id))) {
      phantomId=PhantomPower.id=numid;
      CTLID_DEBUG(("Phantom power Switch id=%d\n", numid));
    } else if (!strcmp("Digital Capture Switch (automute)", snd_ctl_elem_id_get_name(id))) {
      automuteId=Automute.id=numid;
      CTLID_DEBUG(("Automute Switch id=%d\n", numid));
    } else if (!strcmp("VU-meters Switch", snd_ctl_elem_id_get_name(id))) {
      vuswitchId=numid;
      CTLID_DEBUG(("VU-meter switch id=%d\n", numid));
    } else if (!strcmp("VU-meters", snd_ctl_elem_id_get_name(id))) {
      vumetersId=numid;
      CTLID_DEBUG(("VU-meters id=%d\n", numid));
    } else if (!strcmp("Channels info", snd_ctl_elem_id_get_name(id))) {
      channelsId=numid;
      CTLID_DEBUG(("Channels info id=%d\n", numid));
    }
  }

  GetChannels();
  CTLID_DEBUG(("Input channels = %d (analog=%d digital=%d)\n", nIn, fdIn, nIn-fdIn));
  CTLID_DEBUG(("Output channels = %d (analog=%d digital=%d)\n", nLOut, fdOut, nLOut-fdOut));
  CTLID_DEBUG(("PCM channels out = %d\n", nPOut));

#ifndef REAL
vmixerId=1000;
vmixerControl.inputs=12;
vmixerControl.outputs=mixerControl.outputs=nLOut=10;
metersStreams=3;
metersNumber=16;
metersTypes=2;
nPOut=12;
fdOut=2;
nIn=10;
fdIn=2;
printf("nIn=%d fdIn=%d nLOut=%d nPOut=%d fdOut=%d\n", nIn,fdIn,nLOut,nPOut, fdOut);
#endif

  if (mixerId && (mixerControl.inputs!=nIn || mixerControl.outputs!=nLOut)) {
    printf("** Error - Mixer/channels mismatch !!  nIn=%d mnIn=%d    nLOut=%d mnLOut=%d\n", nIn, mixerControl.inputs, nLOut, mixerControl.outputs);
    return(1);
  }
  if (lineoutId && !vmixerId)
    printf("** Warning - Vmixer cards without LineOut volume control are not supported !\n");

  if (vmixerId) {
    if (vmixerControl.inputs!=nPOut || vmixerControl.outputs!=nLOut) {
      printf("** Error - vmixer/channels mismatch:  vmp=%d npo=%d    vmo=%d nlo=%d !!\n", vmixerControl.inputs, nPOut, vmixerControl.outputs, nLOut);
      return(1);
    }
  }

  if (p4InId)
    NominalIn.Channels=fdIn;

  if (p4OutId)
    NominalOut.Channels=fdOut;

  //@ Assumes all mixer and vmixer controls are contiguous
  if (mixerId)
    for (o=0, numid=mixerId; o<nLOut; o++) {
      for (i=0; i<nIn; i++) {
        mixerControl.mixer[o][i].id=numid++;
      }
    }

  if (vmixerId)
    for (o=0, numid=vmixerId; o<vmixerControl.outputs; o++) {
      for (i=0; i<vmixerControl.inputs; i++) {
        vmixerControl.mixer[o][i].id=numid++;
      }
    }

  snd_hctl_close(handle);
  return(0);
}



int main(int argc, char *argv[]) {
  gchar str[256];
  GtkWidget *hbox, *vbox;
  GtkWidget *mainbox;
  GtkWidget *vbsel, *frame, *button;
  GtkWidget *label, *menu, *menuitem;
  GSList *bgroup;
  int err, i, o, n, cardnum, value;
  char hwname[8], cardname[32], load, save;
  snd_ctl_card_info_t *hw_info;

  load=save=1;

  // Scans all installed cards
  snd_ctl_card_info_alloca(&hw_info);
  cardnum=-1;
  ctlhandle=0;

  if (argc>1)
    cardnum=atoi(argv[1])-1;

  while (snd_card_next(&cardnum)>=0 && cardnum>=0) {
    sprintf(hwname, "hw:%d", cardnum);
    if ((err=snd_ctl_open(&ctlhandle, hwname, 0))<0) {
      printf("snd_ctl_open(%s) Error: %s\n", hwname, snd_strerror(err));
      continue;
    }
    if ((err=snd_ctl_card_info(ctlhandle, hw_info))>=0) {
      if (!strncmp(snd_ctl_card_info_get_driver(hw_info), "Echo_", 5)) {
        strncpy(card, hwname, 7);
        hwname[7]=0;
        strncpy(cardname, snd_ctl_card_info_get_name(hw_info), 31);
        cardname[31]=0;
        strncpy(cardId, snd_ctl_card_info_get_name(hw_info), 15);
        cardId[15]=0;
        CTLID_DEBUG(("Card found: %s  (%s)\n", snd_ctl_card_info_get_longname(hw_info), hwname));
/*printf("card       = %d\n", snd_ctl_card_info_get_card(hw_info));
printf("id         = %s\n", snd_ctl_card_info_get_id(hw_info));
printf("driver     = %s\n", snd_ctl_card_info_get_driver(hw_info));
printf("name       = %s\n", snd_ctl_card_info_get_name(hw_info));
printf("longname   = %s\n", snd_ctl_card_info_get_longname(hw_info));
printf("mixername  = %s\n", snd_ctl_card_info_get_mixername(hw_info));
printf("components = %s\n", snd_ctl_card_info_get_components(hw_info));*/
        break;
      }
    } else {
      printf("snd_ctl_card_info(%s) Error: %s\n", hwname, snd_strerror(err));
    }
    snd_ctl_close(ctlhandle);
    ctlhandle=0;
  }

  if (!ctlhandle) {
    printf("No Echoaudio cards found, sorry.\n");
    return(0);
  }

  // Reads available controls
  if (OpenControls(card, cardname))
    exit(1);

  mouseButton=0;
  Gang=0;		// Set the gang button off, because has annoying side effects during initialization
  Mainw_geom.st=NOPOS;
  PVw_geom.st=NOPOS;
  LVw_geom.st=NOPOS;
  VUw_geom.st=NOPOS;
  Mixerw_geom.st=NOPOS;
  Vmixerw_geom.st=NOPOS;
  VUwindow=GMwindow=0;
  GMixerSection.Inputs=nIn;	// The correct value is set by Digital_mode_activate()
  GMixerSection.Outputs=nLOut;
  GMixerSection.VmixerFirst=nIn;
  GMixerSection.VmixerLast=nIn+vmixerControl.inputs-1;
  GMixerSection.LineOut=GMixerSection.VmixerLast+1;

  // Read current mixer setting.
  if (mixerId)
    ReadMixer(&mixerControl);
  if (vmixerId)
    ReadMixer(&vmixerControl);
  if (pcmoutId)
    ReadControl(pcmoutControl.Gain, nPOut, pcmoutControl.id, SND_CTL_ELEM_IFACE_MIXER);
  if (lineinId)
    ReadControl(lineinControl.Gain, nIn, lineinControl.id, SND_CTL_ELEM_IFACE_MIXER);
  if (lineoutId)
    ReadControl(lineoutControl.Gain, nLOut, lineoutId, SND_CTL_ELEM_IFACE_MIXER);
  if (p4InId)
    ReadNominalLevels(&NominalIn);
  if (p4OutId)
    ReadNominalLevels(&NominalOut);

  //@@ check the values
  if (load) {
    FILE *f;
    snprintf(str, 255, "%s/.Emixer_%s", getenv("HOME"), cardId);
    str[255]=0;
    if ((f=fopen(str, "r"))) {
      str[255]=0;
      while (fgets(str, 255, f)) {
        if (!strncmp("LineOut ", str, 8)) {
          sscanf(str+8, "%d %d", &o, &n);
          if (o>=0 && o<nLOut)
            lineoutControl.Gain[o]=n;
        } else if (!strncmp("LineIn ", str, 7)) {
          sscanf(str+7, "%d %d", &i, &n);
          if (i>=0 && i<nIn)
            lineinControl.Gain[i]=n;
        } else if (!strncmp("PcmOut ", str, 7)) {
          sscanf(str+7, "%d %d", &o, &n);
          if (o>=0 && o<nPOut)
            pcmoutControl.Gain[o]=n;
        } else if (!strncmp("NominalOut ", str, 11)) {
          sscanf(str+11, "%d %d", &o, &n);
          if (o>=0 && o<fdOut)
            NominalOut.Level[o]=!!n;
        } else if (!strncmp("NominalIn ", str, 10)) {
          sscanf(str+10, "%d %d", &i, &n);
          if (i>=0 && i<fdIn)
            NominalIn.Level[i]=!!n;
        } else if (!strncmp("Mixer ", str, 6)) {
          sscanf(str+6, "%d %d %d", &o, &i, &n);
          if (o>=0 && o<nLOut && i>=0 && i<nIn)
            mixerControl.mixer[o][i].Gain=n;
        } else if (!strncmp("Vmixer ", str, 7)) {
          sscanf(str+7, "%d %d %d", &o, &i, &n);
          if (o>=0 && o<nLOut && i>=0 && i<nPOut)
            vmixerControl.mixer[o][i].Gain=n;
        } else if (!strncmp("MainWindow ", str, 11)) {
          sscanf(str+11, "%d %d %d %d", &Mainw_geom.x, &Mainw_geom.y, &Mainw_geom.w, &Mainw_geom.h);
        } else if (!strncmp("VUmetersWindow ", str, 15)) {
          sscanf(str+15, "%d %d %d", &VUw_geom.x, &VUw_geom.y, &VUw_geom.st);
        } else if (!strncmp("GfxMixerWindow ", str, 15)) {
          sscanf(str+15, "%d %d %d", &GMw_geom.x, &GMw_geom.y, &GMw_geom.st);
        } else if (!strncmp("PcmVolumeWindow ", str, 16)) {
          sscanf(str+16, "%d %d %d %d %d", &PVw_geom.x, &PVw_geom.y, &PVw_geom.w, &PVw_geom.h, &PVw_geom.st);
        } else if (!strncmp("LineVolumeWindow ", str, 17)) {
          sscanf(str+17, "%d %d %d %d %d", &LVw_geom.x, &LVw_geom.y, &LVw_geom.w, &LVw_geom.h, &LVw_geom.st);
        } else if (!strncmp("MixerWindow ", str, 12)) {
          sscanf(str+12, "%d %d %d %d %d", &Mixerw_geom.x, &Mixerw_geom.y, &Mixerw_geom.w, &Mixerw_geom.h, &Mixerw_geom.st);
        } else if (!strncmp("VmixerWindow ", str, 13)) {
          sscanf(str+13, "%d %d %d %d %d", &Vmixerw_geom.x, &Vmixerw_geom.y, &Vmixerw_geom.w, &Vmixerw_geom.h, &Vmixerw_geom.st);
        } else if (!strncmp("MiscControlsWindow ", str, 19)) {
          sscanf(str+19, "%d %d %d %d %d", &Miscw_geom.x, &Miscw_geom.y, &Miscw_geom.w, &Miscw_geom.h, &Miscw_geom.st);
        }
      }
    }
  }
  gtk_init(&argc, &argv);
  fnt=gdk_font_load("-misc-fixed-medium-r-*-*-10-*-*-*-*-*-*-*");
  if (!fnt) {
    printf("Cannot find the font\n");
    exit(1);
  }

  /* Now assemble the control windows */


  /* ********** Misc controls window ********** */

  Miscwindow=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  sprintf(str, "%s Misc controls", cardId);
  gtk_window_set_title(GTK_WINDOW(Miscwindow), str);
  gtk_window_set_wmclass(GTK_WINDOW(Miscwindow), "misc", "Emixer");
  gtk_signal_connect(GTK_OBJECT(Miscwindow), "delete_event", GTK_SIGNAL_FUNC(CloseWindow), (gpointer)&Miscw_geom);
  gtk_container_set_border_width(GTK_CONTAINER(Miscwindow), BORDER);
  if (Miscw_geom.st!=NOPOS) {
    gtk_widget_set_uposition(Miscwindow, Miscw_geom.x, Miscw_geom.y);
    gtk_window_set_default_size(GTK_WINDOW(Miscwindow), Miscw_geom.w, Miscw_geom.h);
  }

  mainbox=gtk_vbox_new(FALSE, SPACING);
  gtk_widget_show(mainbox);
  gtk_container_add(GTK_CONTAINER(Miscwindow), mainbox);


  if (p4InId) {
    // Consumer/professional analog input switches
    frame=gtk_frame_new("Input +4dBu");
    gtk_widget_show(frame);
    gtk_box_pack_start(GTK_BOX(mainbox), frame, TRUE, FALSE, 0);
    hbox=gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    for (i=0; i<fdIn; i++) {
      sprintf(str, "%d", i);
      button=NominalIn.Button[i]=gtk_toggle_button_new_with_label(str);
      gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 1);
      gtk_widget_show(button);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), NominalIn.Level[i]);	// Forces handler call
      gtk_signal_connect(GTK_OBJECT(button), "toggled", GTK_SIGNAL_FUNC(Nominal_level_toggled), (gpointer)(long)i);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), !NominalIn.Level[i]);
    }
  }


  if (p4OutId) {
    // Consumer/professional analog output switches
    frame=gtk_frame_new("Output +4dBu");
    gtk_widget_show(frame);
    gtk_box_pack_start(GTK_BOX(mainbox), frame, TRUE, FALSE, 0);
    hbox=gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    for (i=0; i<fdOut; i++) {
      sprintf(str, "%d", i);
      button=NominalOut.Button[i]=gtk_toggle_button_new_with_label(str);
      gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 1);
      gtk_widget_show(button);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), NominalOut.Level[i]);
      gtk_signal_connect(GTK_OBJECT(button), "toggled", GTK_SIGNAL_FUNC(Nominal_level_toggled), (gpointer)(long)(i+ECHO_MAXAUDIOINPUTS));
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), !NominalOut.Level[i]);
    }
  }


  if (dmodeId && ndmodes>1) {
    // Digital mode switch
    frame=gtk_frame_new("Digital mode");
    gtk_widget_show(frame);
    gtk_box_pack_start(GTK_BOX(mainbox), frame, TRUE, FALSE, 0);
    hbox=gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    dmodeOpt=gtk_option_menu_new();
    gtk_widget_show(dmodeOpt);
    menu=gtk_menu_new();
    gtk_widget_show(menu);
    for (i=0; i<ndmodes; i++) {
      menuitem=gtk_menu_item_new_with_label(dmodeName[i]);
      gtk_widget_show(menuitem);
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate", Digital_mode_activate, (gpointer)(long)i);
      gtk_menu_append(GTK_MENU(menu), menuitem);
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(dmodeOpt), menu);
    gtk_box_pack_start(GTK_BOX(hbox), dmodeOpt, TRUE, TRUE, 0);
    gtk_option_menu_set_history(GTK_OPTION_MENU(dmodeOpt), dmodeVal=GetEnum(dmodeId));
  }


  if (clocksrcId && nclocksrc>1) {
    // Clock source switch
    frame=gtk_frame_new("Clock source");
    gtk_widget_show(frame);
    gtk_box_pack_start(GTK_BOX(mainbox), frame, TRUE, FALSE, 0);
    hbox=gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    clocksrcOpt=gtk_option_menu_new();
    gtk_widget_show(clocksrcOpt);
    menu=gtk_menu_new();
    gtk_widget_show(menu);
    for (i=0; i<nclocksrc; i++) {
      clocksrc_menuitem[i]=gtk_menu_item_new_with_label(clocksrcName[i]);
      gtk_widget_show(clocksrc_menuitem[i]);
      gtk_widget_set_sensitive(clocksrc_menuitem[i], FALSE);
      gtk_signal_connect(GTK_OBJECT(clocksrc_menuitem[i]), "activate", Clock_source_activate, (gpointer)(long)i);
      gtk_menu_append(GTK_MENU(menu), clocksrc_menuitem[i]);
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(clocksrcOpt), menu);
    gtk_box_pack_start(GTK_BOX(hbox), clocksrcOpt, TRUE, TRUE, 0);
    gtk_option_menu_set_history(GTK_OPTION_MENU(clocksrcOpt), clocksrcVal=GetEnum(clocksrcId));
    clocksrctimer=gtk_timeout_add(2000, CheckInputs, 0);
  }


  if (spdifmodeId && nspdifmodes>1) {
    // S/PDIF mode switch
    frame=gtk_frame_new("S/PDIF mode");
    gtk_widget_show(frame);
    gtk_box_pack_start(GTK_BOX(mainbox), frame, TRUE, FALSE, 0);
    hbox=gtk_hbox_new(FALSE, 0);
    gtk_widget_show(hbox);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    spdifmodeOpt=gtk_option_menu_new();
    gtk_widget_show(spdifmodeOpt);
    menu=gtk_menu_new();
    gtk_widget_show(menu);
    for (i=0; i<nspdifmodes; i++) {
      menuitem=gtk_menu_item_new_with_label(spdifmodeName[i]);
      gtk_widget_show(menuitem);
      gtk_signal_connect(GTK_OBJECT(menuitem), "activate", SPDIF_mode_activate, (gpointer)(long)i);
      gtk_menu_append(GTK_MENU(menu), menuitem);
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(spdifmodeOpt), menu);
    gtk_box_pack_start(GTK_BOX(hbox), spdifmodeOpt, TRUE, TRUE, 0);
    gtk_option_menu_set_history(GTK_OPTION_MENU(spdifmodeOpt), spdifmodeVal=GetEnum(spdifmodeId));
  }


  // Switches
  if (phantomId || clocksrcId) {
    frame=gtk_frame_new("Switches");
    gtk_widget_show(frame);
    gtk_box_pack_start(GTK_BOX(mainbox), frame, TRUE, FALSE, 0);
    hbox=gtk_vbox_new(FALSE, 0);
    gtk_widget_show(hbox);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    if (phantomId) {
      // Phantom power switch
      button=gtk_check_button_new_with_label("Phantom power");
      gtk_widget_show(button);
      gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 0);
      ReadControl(&i, 1, PhantomPower.id, SND_CTL_ELEM_IFACE_MIXER);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), i);
      gtk_signal_connect(GTK_OBJECT(button), "toggled", Switch_toggled, (gpointer)&PhantomPower);
      PhantomPower.Button=button;
    }

    if (automuteId) {
      // Digital input automute switch
      button=gtk_check_button_new_with_label("Automute");
      gtk_widget_show(button);
      gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 0);
      ReadControl(&i, 1, Automute.id, SND_CTL_ELEM_IFACE_CARD);
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), i);
      gtk_signal_connect(GTK_OBJECT(button), "toggled", Switch_toggled, (gpointer)&Automute);
      Automute.Button=button;
    }

    // Auto clock switch
    if (clocksrcId) {
      autoclockChkbutton=gtk_check_button_new_with_label("Autoclock");
      gtk_widget_show(autoclockChkbutton);
      gtk_box_pack_start(GTK_BOX(hbox), autoclockChkbutton, TRUE, FALSE, 0);
      gtk_signal_connect(GTK_OBJECT(autoclockChkbutton), "toggled", AutoClock_toggled, NULL);
      AutoClock=-1;
    }
  }


/* ********** PCM volume window ********** */

  pcmoutControl.window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  sprintf(str, "%s PCM volume", cardId);
  gtk_window_set_title(GTK_WINDOW (pcmoutControl.window), str);
  gtk_window_set_wmclass(GTK_WINDOW(pcmoutControl.window), "pcm", "Emixer");
  gtk_signal_connect(GTK_OBJECT(pcmoutControl.window), "delete_event", GTK_SIGNAL_FUNC(CloseWindow), (gpointer)&PVw_geom);
  gtk_container_set_border_width(GTK_CONTAINER(pcmoutControl.window), BORDER);
  if (PVw_geom.st!=NOPOS) {
    gtk_widget_set_uposition(pcmoutControl.window, PVw_geom.x, PVw_geom.y);
    gtk_window_set_default_size(GTK_WINDOW(pcmoutControl.window), PVw_geom.w, PVw_geom.h);
  }

  mainbox=gtk_hbox_new(FALSE, SPACING);
  gtk_widget_show(mainbox);
  gtk_container_add(GTK_CONTAINER(pcmoutControl.window), mainbox);


  if (pcmoutId) {
    // PCM Output volume widgets
    frame=gtk_frame_new("PCM Output volume");
    gtk_widget_show(frame);
    gtk_box_pack_start(GTK_BOX(mainbox), frame, TRUE, TRUE, 0);
    hbox=gtk_hbox_new(TRUE, 1);
    gtk_widget_show(hbox);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    for (i=0; i<nPOut; i++) {
      vbox=gtk_vbox_new(FALSE, 0);
      gtk_widget_show(vbox);
      gtk_container_add(GTK_CONTAINER(hbox), vbox);
      // Channel label
      if (i<fdOut)
        sprintf(str, "A%d", i);
      else
        sprintf(str, "D%d", i-fdOut);
      label=gtk_label_new(str);
      gtk_widget_show(label);
      gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
      // Volume
      value = INVERT(pcmoutControl.Gain[i]);
      pcmoutControl.adj[i]=gtk_adjustment_new(!value, ECHOGAIN_MINOUT, ECHOGAIN_MAXOUT, SHORTSTEP, LONGSTEP, 0);
      pcmoutControl.volume[i]=gtk_vscale_new(GTK_ADJUSTMENT(pcmoutControl.adj[i]));
      gtk_widget_show(pcmoutControl.volume[i]);
      gtk_box_pack_start(GTK_BOX(vbox), pcmoutControl.volume[i], TRUE, TRUE, 0);
      gtk_scale_set_draw_value(GTK_SCALE(pcmoutControl.volume[i]), 0);
      gtk_signal_connect(GTK_OBJECT(pcmoutControl.adj[i]), "value_changed", GTK_SIGNAL_FUNC(PCM_volume_changed), (gpointer)(long)(i+ECHO_MAXAUDIOINPUTS));
      // Value label
      pcmoutControl.label[i]=gtk_label_new("xxx");
      gtk_widget_show(pcmoutControl.label[i]);
      gtk_box_pack_start(GTK_BOX(vbox), pcmoutControl.label[i], FALSE, FALSE, 0);
      gtk_adjustment_set_value(GTK_ADJUSTMENT(pcmoutControl.adj[i]), value);
    }
    gtk_widget_set_usize(GTK_WIDGET(pcmoutControl.volume[0]), 0, 170);		// Set minimum y size
  }


/* ********** Line volume window ********** */

  LVwindow=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  sprintf(str, "%s Line volume", cardId);
  gtk_window_set_title(GTK_WINDOW (LVwindow), str);
  gtk_window_set_wmclass(GTK_WINDOW(LVwindow), "line", "Emixer");
  gtk_signal_connect(GTK_OBJECT(LVwindow), "delete_event", GTK_SIGNAL_FUNC(CloseWindow), (gpointer)&LVw_geom);
  gtk_container_set_border_width(GTK_CONTAINER(LVwindow), BORDER);
  if (LVw_geom.st!=NOPOS) {
    gtk_widget_set_uposition(LVwindow, LVw_geom.x, LVw_geom.y);
    gtk_window_set_default_size(GTK_WINDOW(LVwindow), LVw_geom.w, LVw_geom.h);
  }

  mainbox=gtk_hbox_new(FALSE, SPACING);
  gtk_widget_show(mainbox);
  gtk_container_add(GTK_CONTAINER(LVwindow), mainbox);

  // Line input volume widgets
  if (lineinId) {
    frame=gtk_frame_new("Analog input volume");
    gtk_widget_show(frame);
    gtk_box_pack_start(GTK_BOX(mainbox), frame, TRUE, TRUE, 0);
    hbox=gtk_hbox_new(TRUE, 1);
    gtk_widget_show(hbox);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    for (i=0; i<fdIn; i++) {
      vbox=gtk_vbox_new(FALSE, 0);
      gtk_widget_show(vbox);
      gtk_container_add(GTK_CONTAINER(hbox), vbox);
      // Channel label
      sprintf(str, "%d", i);
      label=gtk_label_new(str);
      gtk_widget_show(label);
      gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
      // Volume (resolution is 0.5 dB)
      value = IN_INVERT(lineinControl.Gain[i]);
      lineinControl.adj[i]=gtk_adjustment_new(!value, ECHOGAIN_MININP, ECHOGAIN_MAXINP, SHORTSTEP, LONGSTEP*2, 0);
      lineinControl.volume[i]=gtk_vscale_new(GTK_ADJUSTMENT(lineinControl.adj[i]));
      gtk_widget_show(lineinControl.volume[i]);
      gtk_box_pack_start(GTK_BOX(vbox), lineinControl.volume[i], TRUE, TRUE, 0);
      gtk_scale_set_draw_value(GTK_SCALE(lineinControl.volume[i]), 0);
      gtk_signal_connect(GTK_OBJECT(lineinControl.adj[i]), "value_changed", GTK_SIGNAL_FUNC(PCM_volume_changed), (gpointer)(long)i);
      // Value label
      lineinControl.label[i]=gtk_label_new("xxx");
      gtk_widget_show(lineinControl.label[i]);
      gtk_box_pack_start(GTK_BOX(vbox), lineinControl.label[i], FALSE, FALSE, 0);
      gtk_adjustment_set_value(GTK_ADJUSTMENT(lineinControl.adj[i]), value);
    }
    gtk_widget_set_usize(GTK_WIDGET(lineinControl.volume[0]), 0, 170);	// Set minimum y size
  }


  // Line output volume widgets
  if (1) {
    frame=gtk_frame_new("Line output volume");
    gtk_widget_show(frame);
    gtk_box_pack_start(GTK_BOX(mainbox), frame, TRUE, TRUE, 0);
    hbox=gtk_hbox_new(TRUE, 1);
    gtk_widget_show(hbox);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    for (i=0; i<nLOut; i++) {
      vbox=gtk_vbox_new(FALSE, 0);
      gtk_widget_show(vbox);
      gtk_container_add(GTK_CONTAINER(hbox), vbox);
      // Channel label
      if (i<fdOut)
        sprintf(str, "A%d", i);
      else
        sprintf(str, "D%d", i-fdOut);
      label=gtk_label_new(str);
      gtk_widget_show(label);
      gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
      // Volume
      value = INVERT(lineoutControl.Gain[i]);
      lineoutControl.adj[i]=gtk_adjustment_new(!value, ECHOGAIN_MINOUT, ECHOGAIN_MAXOUT, SHORTSTEP, LONGSTEP, 0);
      lineoutControl.volume[i]=gtk_vscale_new(GTK_ADJUSTMENT(lineoutControl.adj[i]));
      gtk_widget_show(lineoutControl.volume[i]);
      gtk_box_pack_start(GTK_BOX(vbox), lineoutControl.volume[i], TRUE, TRUE, 0);
      gtk_scale_set_draw_value(GTK_SCALE(lineoutControl.volume[i]), 0);
      gtk_signal_connect(GTK_OBJECT(lineoutControl.adj[i]), "value_changed", GTK_SIGNAL_FUNC(LineOut_volume_changed), (gpointer)(long)i);
      // Value label
      lineoutControl.label[i]=gtk_label_new("xxx");
      gtk_widget_show(lineoutControl.label[i]);
      gtk_box_pack_start(GTK_BOX(vbox), lineoutControl.label[i], FALSE, FALSE, 0);
      gtk_adjustment_set_value(GTK_ADJUSTMENT(lineoutControl.adj[i]), value);
    }
    gtk_widget_set_usize(GTK_WIDGET(lineoutControl.volume[0]), 0, 170);		// Set minimum y size
  }



/* ********** Mixer window ********** */

  if (mixerId) {
    mixerControl.window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
    sprintf(str, "%s Monitor mixer", cardId);
    gtk_window_set_title(GTK_WINDOW(mixerControl.window), str);
    gtk_window_set_wmclass(GTK_WINDOW(mixerControl.window), "mixer", "Emixer");
    gtk_signal_connect(GTK_OBJECT(mixerControl.window), "delete_event", GTK_SIGNAL_FUNC(CloseWindow), (gpointer)&Mixerw_geom);
    gtk_container_set_border_width(GTK_CONTAINER(mixerControl.window), BORDER);
    if (Mixerw_geom.st!=NOPOS) {
      gtk_widget_set_uposition(mixerControl.window, Mixerw_geom.x, Mixerw_geom.y);
      gtk_window_set_default_size(GTK_WINDOW(mixerControl.window), Mixerw_geom.w, Mixerw_geom.h);
//      gdk_window_move_resize(mixerControl.window->window, Mixerw_geom.x, Mixerw_geom.y, Mixerw_geom.w, Mixerw_geom.h);
/*      gtk_widget_set_usize(mixerControl.window, Mixerw_geom.w, Mixerw_geom.h);
      gtk_widget_set_usize(mixerControl.window, -1, -1);*/
    }

    mainbox=gtk_hbox_new(FALSE, SPACING);
    gtk_widget_show(mainbox);
    gtk_container_add(GTK_CONTAINER(mixerControl.window), mainbox);

#ifdef REVERSE
    // Mixer volume widgets
    frame=gtk_frame_new("Mixer input levels");
    gtk_widget_show(frame);
    gtk_box_pack_start(GTK_BOX(mainbox), frame, TRUE, TRUE, 0);
    hbox=gtk_hbox_new(TRUE, 1);
    gtk_widget_show(hbox);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    for (i=0; i<nIn; i++) {
      vbox=gtk_vbox_new(FALSE, 0);
      gtk_widget_show(vbox);
      gtk_container_add(GTK_CONTAINER(hbox), vbox);
      // Channel label
      if (i<fdIn)
        sprintf(str, "A%d", i);
      else
        sprintf(str, "D%d", i-fdIn);
      label=gtk_label_new(str);
      gtk_widget_show(label);
      gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
      // Volume
      mixerControl.adj[i]=gtk_adjustment_new(0, ECHOGAIN_MINOUT, ECHOGAIN_MAXOUT, SHORTSTEP, LONGSTEP, 0);
      mixerControl.volume[i]=gtk_vscale_new(GTK_ADJUSTMENT(mixerControl.adj[i]));
      gtk_widget_show(mixerControl.volume[i]);
      gtk_box_pack_start(GTK_BOX(vbox), mixerControl.volume[i], TRUE, TRUE, 0);
      gtk_scale_set_draw_value(GTK_SCALE(mixerControl.volume[i]), 0);
      gtk_signal_connect(GTK_OBJECT(mixerControl.volume[i]), "grab_focus", GTK_SIGNAL_FUNC(Monitor_volume_clicked), (gpointer)i);
      gtk_signal_connect(GTK_OBJECT(mixerControl.adj[i]), "value_changed", GTK_SIGNAL_FUNC(Monitor_volume_changed), (gpointer)i);
      // Value label
      mixerControl.label[i]=gtk_label_new("xxx");
      gtk_widget_show(mixerControl.label[i]);
      gtk_box_pack_start(GTK_BOX(vbox), mixerControl.label[i], FALSE, FALSE, 0);
    }
    gtk_widget_set_usize(GTK_WIDGET(mixerControl.volume[0]), 0, 170);		// Set minimum y size

    // Output channel selectors
    frame=gtk_frame_new("Mixer output");
    gtk_widget_show(frame);
    vbsel=gtk_vbox_new(FALSE, 2);
    gtk_box_pack_start(GTK_BOX(mainbox), frame, FALSE, FALSE, 0);
    gtk_widget_show(vbsel);
    gtk_container_add(GTK_CONTAINER(frame), vbsel);

    bgroup=0;
    for (i=n=0; i<nLOut; i++) {
      if (i<fdOut)
        sprintf(str, "An-%d", i);
      else
        sprintf(str, "Di-%d", i-fdOut);
      if (i)
        bgroup=gtk_radio_button_group(GTK_RADIO_BUTTON(mixerControl.outsel[i-1]));
      mixerControl.outsel[i]=gtk_radio_button_new_with_label(bgroup, str);
      gtk_widget_show(mixerControl.outsel[i]);
      gtk_box_pack_start(GTK_BOX(vbsel), mixerControl.outsel[i], FALSE, FALSE, 0);
      gtk_signal_connect(GTK_OBJECT(mixerControl.outsel[i]), "toggled", GTK_SIGNAL_FUNC(Mixer_Output_selector_clicked), (gpointer)i);
    }
    mixerControl.input=0;
    mixerControl.output=-1;
    Mixer_Output_selector_clicked(0, 0);

#else // REVERSE

    // Input channel selectors
    frame=gtk_frame_new("Mixer input");
    gtk_widget_show(frame);
    gtk_box_pack_start(GTK_BOX(mainbox), frame, FALSE, FALSE, 0);
    vbsel=gtk_vbox_new(FALSE, 2);
    gtk_widget_show(vbsel);
    gtk_container_add(GTK_CONTAINER(frame), vbsel);

    bgroup=0;
    for (i=n=0; i<nIn; i++) {
      if (i<fdIn)
        sprintf(str, "An-%d", i);
      else
        sprintf(str, "Di-%d", i-fdIn);
      if (i)
        bgroup=gtk_radio_button_group(GTK_RADIO_BUTTON(mixerControl.inpsel[i-1]));
      mixerControl.inpsel[i]=gtk_radio_button_new_with_label(bgroup, str);
      gtk_widget_show(mixerControl.inpsel[i]);
      gtk_box_pack_start(GTK_BOX(vbsel), mixerControl.inpsel[i], FALSE, FALSE, 0);
      gtk_signal_connect(GTK_OBJECT(mixerControl.inpsel[i]), "toggled", GTK_SIGNAL_FUNC(Mixer_Input_selector_clicked), (gpointer)(long)i);
    }

    // Mixer volume widgets
    frame=gtk_frame_new("Mixer output levels");
    gtk_widget_show(frame);
    gtk_box_pack_start(GTK_BOX(mainbox), frame, TRUE, TRUE, 0);
    hbox=gtk_hbox_new(TRUE, 1);
    gtk_widget_show(hbox);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    for (i=0; i<nLOut; i++) {
      vbox=gtk_vbox_new(FALSE, 0);
      gtk_widget_show(vbox);
      gtk_container_add(GTK_CONTAINER(hbox), vbox);
      // Channel label
      if (i<fdOut)
        sprintf(str, "A%d", i);
      else
        sprintf(str, "D%d", i-fdOut);
      label=gtk_label_new(str);
      gtk_widget_show(label);
      gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
      // Volume
      mixerControl.adj[i]=gtk_adjustment_new(0, ECHOGAIN_MINOUT, ECHOGAIN_MAXOUT, SHORTSTEP, LONGSTEP, 0);
      mixerControl.volume[i]=gtk_vscale_new(GTK_ADJUSTMENT(mixerControl.adj[i]));
      gtk_widget_show(mixerControl.volume[i]);
      gtk_box_pack_start(GTK_BOX(vbox), mixerControl.volume[i], TRUE, TRUE, 0);
      gtk_scale_set_draw_value(GTK_SCALE(mixerControl.volume[i]), 0);
      gtk_signal_connect(GTK_OBJECT(mixerControl.volume[i]), "grab_focus", GTK_SIGNAL_FUNC(Monitor_volume_clicked), (gpointer)(long)i);
      gtk_signal_connect(GTK_OBJECT(mixerControl.adj[i]), "value_changed", GTK_SIGNAL_FUNC(Monitor_volume_changed), (gpointer)(long)i);
      // Value label
      mixerControl.label[i]=gtk_label_new("xxx");
      gtk_widget_show(mixerControl.label[i]);
      gtk_box_pack_start(GTK_BOX(vbox), mixerControl.label[i], FALSE, FALSE, 0);
    }
    gtk_widget_set_usize(GTK_WIDGET(mixerControl.volume[0]), 0, 170);		// Set minimum y size
    mixerControl.input=-1;
    mixerControl.output=0;
    Mixer_Input_selector_clicked(0, 0);
#endif
  }


/* ********** Vmixer window ********** */

  if (vmixerId) {
    vmixerControl.window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
    sprintf(str, "%s Vmixer", cardId);
    gtk_window_set_title(GTK_WINDOW(vmixerControl.window), str);
    gtk_window_set_wmclass(GTK_WINDOW(vmixerControl.window), "vmixer", "Emixer");
    gtk_signal_connect(GTK_OBJECT(vmixerControl.window), "delete_event", GTK_SIGNAL_FUNC(CloseWindow), (gpointer)&Vmixerw_geom);
    gtk_container_set_border_width(GTK_CONTAINER(vmixerControl.window), BORDER);
    if (Vmixerw_geom.st!=NOPOS) {
      gtk_widget_set_uposition(vmixerControl.window, Vmixerw_geom.x, Vmixerw_geom.y);
      gtk_window_set_default_size(GTK_WINDOW(vmixerControl.window), Vmixerw_geom.w, Vmixerw_geom.h);
    }

    mainbox=gtk_hbox_new(FALSE, SPACING);
    gtk_widget_show(mainbox);
    gtk_container_add(GTK_CONTAINER(vmixerControl.window), mainbox);

#ifdef REVERSE

    // Vmixer volume widgets
    frame=gtk_frame_new("Vmixer vchannels levels");
    gtk_widget_show(frame);
    gtk_box_pack_start(GTK_BOX(mainbox), frame, TRUE, TRUE, 0);
    hbox=gtk_hbox_new(TRUE, 1);
    gtk_widget_show(hbox);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    for (i=0; i<vmixerControl.inputs; i++) {
      vbox=gtk_vbox_new(FALSE, 0);
      gtk_widget_show(vbox);
      gtk_container_add(GTK_CONTAINER(hbox), vbox);
      // Channel label
      sprintf(str, "V%d", i);
      label=gtk_label_new(str);
      gtk_widget_show(label);
      gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
      // Volume
      vmixerControl.adj[i]=gtk_adjustment_new(0, ECHOGAIN_MINOUT, ECHOGAIN_MAXOUT, SHORTSTEP, LONGSTEP, 0);
      vmixerControl.volume[i]=gtk_vscale_new(GTK_ADJUSTMENT(vmixerControl.adj[i]));
      gtk_widget_show(vmixerControl.volume[i]);
      gtk_box_pack_start(GTK_BOX(vbox), vmixerControl.volume[i], TRUE, TRUE, 0);
      gtk_scale_set_draw_value(GTK_SCALE(vmixerControl.volume[i]), 0);
      gtk_signal_connect(GTK_OBJECT(vmixerControl.volume[i]), "grab_focus", GTK_SIGNAL_FUNC(Vmixer_volume_clicked), (gpointer)i);
      gtk_signal_connect(GTK_OBJECT(vmixerControl.adj[i]), "value_changed", GTK_SIGNAL_FUNC(Vmixer_volume_changed), (gpointer)i);
      // Value label
      vmixerControl.label[i]=gtk_label_new("xxx");
      gtk_widget_show(vmixerControl.label[i]);
      gtk_box_pack_start(GTK_BOX(vbox), vmixerControl.label[i], FALSE, FALSE, 0);
    }
    gtk_widget_set_usize(GTK_WIDGET(vmixerControl.volume[0]), 0, 170);		// Set minimum y size

    // Input channel selectors
    frame=gtk_frame_new("Output");
    gtk_widget_show(frame);
    gtk_box_pack_start(GTK_BOX(mainbox), frame, FALSE, FALSE, 0);
    vbsel=gtk_vbox_new(FALSE, 2);
    gtk_widget_show(vbsel);
    gtk_container_add(GTK_CONTAINER(frame), vbsel);

    bgroup=0;
    for (i=0; i<vmixerControl.outputs; i++) {
      if (i<fdOut)
        sprintf(str, "A%d", i);
      else
        sprintf(str, "D%d", i);
      if (i)
        bgroup=gtk_radio_button_group(GTK_RADIO_BUTTON(vmixerControl.outsel[i-1]));
      vmixerControl.outsel[i]=gtk_radio_button_new_with_label(bgroup, str);
      gtk_widget_show(vmixerControl.outsel[i]);
      gtk_box_pack_start(GTK_BOX(vbsel), vmixerControl.outsel[i], FALSE, FALSE, 0);
      gtk_signal_connect(GTK_OBJECT(vmixerControl.outsel[i]), "toggled", GTK_SIGNAL_FUNC(Vmixer_output_selector_clicked), (gpointer)i);
    }
    vmixerControl.output=-1;
    Vmixer_output_selector_clicked(0, 0);

#else // REVERSE

    // Input channel selectors
    frame=gtk_frame_new("Vchannel");
    gtk_widget_show(frame);
    gtk_box_pack_start(GTK_BOX(mainbox), frame, FALSE, FALSE, 0);
    vbsel=gtk_vbox_new(FALSE, 2);
    gtk_widget_show(vbsel);
    gtk_container_add(GTK_CONTAINER(frame), vbsel);

    bgroup=0;
    for (i=0; i<vmixerControl.inputs; i++) {
      sprintf(str, "V%d", i);
      if (i)
        bgroup=gtk_radio_button_group(GTK_RADIO_BUTTON(vmixerControl.vchsel[i-1]));
      vmixerControl.vchsel[i]=gtk_radio_button_new_with_label(bgroup, str);
      gtk_widget_show(vmixerControl.vchsel[i]);
      gtk_box_pack_start(GTK_BOX(vbsel), vmixerControl.vchsel[i], FALSE, FALSE, 0);
      gtk_signal_connect(GTK_OBJECT(vmixerControl.vchsel[i]), "toggled", GTK_SIGNAL_FUNC(Vmixer_vchannel_selector_clicked), (gpointer)(long)i);
    }

    // Vmixer volume widgets
    frame=gtk_frame_new("Vmixer output levels");
    gtk_widget_show(frame);
    gtk_box_pack_start(GTK_BOX(mainbox), frame, TRUE, TRUE, 0);
    hbox=gtk_hbox_new(TRUE, 1);
    gtk_widget_show(hbox);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    for (i=0; i<vmixerControl.outputs; i++) {
      vbox=gtk_vbox_new(FALSE, 0);
      gtk_widget_show(vbox);
      gtk_container_add(GTK_CONTAINER(hbox), vbox);
      // Channel label
      if (i<fdOut)
        sprintf(str, "A%d", i);
      else
        sprintf(str, "D%d", i-fdOut);
      label=gtk_label_new(str);
      gtk_widget_show(label);
      gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
      // Volume
      vmixerControl.adj[i]=gtk_adjustment_new(0, ECHOGAIN_MINOUT, ECHOGAIN_MAXOUT, SHORTSTEP, LONGSTEP, 0);
      vmixerControl.volume[i]=gtk_vscale_new(GTK_ADJUSTMENT(vmixerControl.adj[i]));
      gtk_widget_show(vmixerControl.volume[i]);
      gtk_box_pack_start(GTK_BOX(vbox), vmixerControl.volume[i], TRUE, TRUE, 0);
      gtk_scale_set_draw_value(GTK_SCALE(vmixerControl.volume[i]), 0);
      gtk_signal_connect(GTK_OBJECT(vmixerControl.volume[i]), "grab_focus", GTK_SIGNAL_FUNC(Vmixer_volume_clicked), (gpointer)(long)i);
      gtk_signal_connect(GTK_OBJECT(vmixerControl.adj[i]), "value_changed", GTK_SIGNAL_FUNC(Vmixer_volume_changed), (gpointer)(long)i);
      // Value label
      vmixerControl.label[i]=gtk_label_new("xxx");
      gtk_widget_show(vmixerControl.label[i]);
      gtk_box_pack_start(GTK_BOX(vbox), vmixerControl.label[i], FALSE, FALSE, 0);
    }
    gtk_widget_set_usize(GTK_WIDGET(vmixerControl.volume[0]), 0, 170);		// Set minimum y size
    vmixerControl.input=-1;
    Vmixer_vchannel_selector_clicked(0, 0);
#endif
  }


/* ********** Main window ********** */

  Mainwindow=gtk_window_new(GTK_WINDOW_TOPLEVEL);
  sprintf(str, EM_VERSION, cardId);
  gtk_window_set_title(GTK_WINDOW(Mainwindow), str);
  gtk_window_set_wmclass(GTK_WINDOW(Mainwindow), "emixer", "Emixer");
  gtk_signal_connect(GTK_OBJECT(Mainwindow), "delete_event", GTK_SIGNAL_FUNC(Mainwindow_delete), (gpointer)&Mainw_geom);
  gtk_container_set_border_width(GTK_CONTAINER(Mainwindow), BORDER);
  gtk_widget_show(Mainwindow);
  if (Mainw_geom.x!=NOPOS) {
    gtk_widget_set_uposition(Mainwindow, Mainw_geom.x, Mainw_geom.y);
    gtk_window_set_default_size(GTK_WINDOW(Mainwindow), Mainw_geom.w, Mainw_geom.h);
  }

  mainbox=gtk_hbox_new(FALSE, SPACING);
  gtk_widget_show(mainbox);
  gtk_container_add(GTK_CONTAINER(Mainwindow), mainbox);


  // Gang button and its frame
  frame=gtk_frame_new("Gang");
  gtk_widget_show(frame);
  gtk_box_pack_start(GTK_BOX(mainbox), frame, FALSE, FALSE, 0);
  hbox=gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox);
  gtk_container_add(GTK_CONTAINER(frame), hbox);
  button=gtk_toggle_button_new_with_label("On");
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), 1);
  gtk_widget_show(button);
  gtk_signal_connect(GTK_OBJECT(button), "toggled", Gang_button_toggled, 0);

  // Controls frame
  frame=gtk_frame_new("Controls");
  gtk_widget_show(frame);
  gtk_box_pack_end(GTK_BOX(mainbox), frame, FALSE, FALSE, 0);
  hbox=gtk_hbox_new(FALSE, 0);
  gtk_widget_show(hbox);
  gtk_container_add(GTK_CONTAINER(frame), hbox);

  // VUmeters button
  if (vumetersId && vuswitchId) {
    button=gtk_toggle_button_new_with_label("VU");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 1);
    gtk_signal_connect(GTK_OBJECT(button), "toggled", VUmeters_button_click, 0);
    VUw_geom.toggler=button;
    if (VUw_geom.st==1)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
  }

  // Line volume button
  button=gtk_toggle_button_new_with_label("Line");
  gtk_widget_show(button);
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 1);
  gtk_signal_connect(GTK_OBJECT(button), "toggled", ToggleWindow, (gpointer)LVwindow);
  LVw_geom.toggler=button;
  if (LVw_geom.st==1)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);

  // Misc controls button
  if (p4InId || p4OutId || phantomId || (dmodeId && ndmodes>1) || (clocksrcId && nclocksrc>1) || (spdifmodeId && nspdifmodes>1)) {
    button=gtk_toggle_button_new_with_label("Misc");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 1);
    gtk_signal_connect(GTK_OBJECT(button), "toggled", ToggleWindow, (gpointer)Miscwindow);
    Miscw_geom.toggler=button;
    if (Miscw_geom.st==1)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
  }

  if (mixerId) {
    // Graphical mixer button
    button=gtk_toggle_button_new_with_label("GrMix");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 1);
    gtk_signal_connect(GTK_OBJECT(button), "toggled", GMixer_button_click, 0);
    GMw_geom.toggler=button;
    if (GMw_geom.st==1)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);

    // Mixer button
    button=gtk_toggle_button_new_with_label("Mixer");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 1);
    gtk_signal_connect(GTK_OBJECT(button), "toggled", ToggleWindow, (gpointer)mixerControl.window);
    Mixerw_geom.toggler=button;
    if (Mixerw_geom.st==1)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
  }

  if (vmixerId) {
    // Vmixer button
    button=gtk_toggle_button_new_with_label("Vmixer");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 1);
    gtk_signal_connect(GTK_OBJECT(button), "toggled", ToggleWindow, (gpointer)vmixerControl.window);
    Vmixerw_geom.toggler=button;
    if (Vmixerw_geom.st==1)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
  }

  if (pcmoutId) {
    // PCM volume button
    button=gtk_toggle_button_new_with_label("PCM");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 1);
    gtk_signal_connect(GTK_OBJECT(button), "toggled", ToggleWindow, (gpointer)pcmoutControl.window);
    PVw_geom.toggler=button;
    if (PVw_geom.st==1)
      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
  }




/* ********** GTK-main ********** */

  Gang=1;
  if (dmodeId)
    Digital_mode_activate(dmodeOpt, (gpointer)(long)dmodeVal);	// Also calls SetSensitivity()
  gtk_widget_show(Mainwindow);
  gtk_main();

  if (save) {
    FILE *f;
    if (snprintf(str, 255, "%s/.Emixer_%s", getenv("HOME"), cardId)>0) {
      str[255]=0;
      if ((f=fopen(str, "w"))) {
        fprintf(f, "-- LineOut <channel> <gain>\n");
        for (i=0; i<nLOut; i++)
          fprintf(f, "LineOut %2d %d\n", i, lineoutControl.Gain[i]);
        fprintf(f, "-- LineIn <channel> <gain>\n");
        for (i=0; i<nIn; i++)
          fprintf(f, "LineIn %2d %d\n", i, lineinControl.Gain[i]);
        fprintf(f, "-- PcmOut <channel> <gain>\n");
        for (i=0; i<nPOut; i++)
          fprintf(f, "PcmOut %2d %d\n", i, pcmoutControl.Gain[i]);
        if (p4InId) {
          fprintf(f, "-- NominalIn <channel> <consumer level enabled>\n");
          for (i=0; i<fdIn; i++)
            fprintf(f, "NominalIn %2d %d\n", i, NominalIn.Level[i]);
        }
        if (p4OutId) {
          fprintf(f, "-- NominalOut <channel> <consumer level enabled>\n");
          for (o=0; o<fdOut; o++)
            fprintf(f, "NominalOut %2d %d\n", o, NominalOut.Level[o]);
        }
        if (mixerId) {
          fprintf(f, "-- Mixer <output> <input> <gain>\n");
          for (o=0; o<nLOut; o++)
            for (i=0; i<nIn; i++)
              fprintf(f, "Mixer %2d %2d %d\n", o, i, mixerControl.mixer[o][i].Gain);
        }
        if (vmixerId) {
          fprintf(f, "-- Vmixer <output> <vchannel> <gain>\n");
          for (o=0; o<nLOut; o++)
            for (i=0; i<nPOut; i++)
              fprintf(f, "Vmixer %2d %2d %d\n", o, i, vmixerControl.mixer[o][i].Gain);
        }
        fprintf(f, "-- xxWindow <x> <y> <width> <height> <visible>\n");
        fprintf(f, "MainWindow %d %d %d %d\n", Mainw_geom.x, Mainw_geom.y, Mainw_geom.w, Mainw_geom.h);
        if (VUwindow)
          gdk_window_get_root_origin(VUwindow->window, &VUw_geom.x, &VUw_geom.y);
        fprintf(f, "VUmetersWindow %d %d %d\n", VUw_geom.x, VUw_geom.y, VUw_geom.st);
        if (GMwindow)
          gdk_window_get_root_origin(GMwindow->window, &VUw_geom.x, &VUw_geom.y);
        fprintf(f, "GfxMixerWindow %d %d %d\n", GMw_geom.x, GMw_geom.y, GMw_geom.st);
        if (pcmoutId) {
          if (pcmoutControl.window->window) {
            gdk_window_get_root_origin(pcmoutControl.window->window, &PVw_geom.x, &PVw_geom.y);
            gdk_window_get_size(pcmoutControl.window->window, &PVw_geom.w, &PVw_geom.h);
          }
          fprintf(f, "PcmVolumeWindow %d %d %d %d %d\n", PVw_geom.x, PVw_geom.y, PVw_geom.w, PVw_geom.h, !!GTK_WIDGET_VISIBLE(pcmoutControl.window));
        }
        if (LVwindow->window) {
          gdk_window_get_root_origin(LVwindow->window, &LVw_geom.x, &LVw_geom.y);
          gdk_window_get_size(LVwindow->window, &LVw_geom.w, &LVw_geom.h);
        }
        fprintf(f, "LineVolumeWindow %d %d %d %d %d\n", LVw_geom.x, LVw_geom.y, LVw_geom.w, LVw_geom.h, !!GTK_WIDGET_VISIBLE(LVwindow));
        if (Miscwindow->window) {
          gdk_window_get_root_origin(Miscwindow->window, &Miscw_geom.x, &Miscw_geom.y);
          gdk_window_get_size(Miscwindow->window, &Miscw_geom.w, &Miscw_geom.h);
        }
        fprintf(f, "MiscControlsWindow %d %d %d %d %d\n", Miscw_geom.x, Miscw_geom.y, Miscw_geom.w, Miscw_geom.h, !!GTK_WIDGET_VISIBLE(Miscwindow));
        if (mixerId) {
          if (mixerControl.window->window) {
            gdk_window_get_root_origin(mixerControl.window->window, &Mixerw_geom.x, &Mixerw_geom.y);
            gdk_window_get_size(mixerControl.window->window, &Mixerw_geom.w, &Mixerw_geom.h);
          }
          fprintf(f, "MixerWindow %d %d %d %d %d\n", Mixerw_geom.x, Mixerw_geom.y, Mixerw_geom.w, Mixerw_geom.h, !!GTK_WIDGET_VISIBLE(mixerControl.window));
        }
        if (vmixerId) {
          if (vmixerControl.window->window) {
            gdk_window_get_root_origin(vmixerControl.window->window, &Vmixerw_geom.x, &Vmixerw_geom.y);
            gdk_window_get_size(vmixerControl.window->window, &Vmixerw_geom.w, &Vmixerw_geom.h);
          }
          fprintf(f, "VmixerWindow %d %d %d %d %d\n", Vmixerw_geom.x, Vmixerw_geom.y, Vmixerw_geom.w, Vmixerw_geom.h, !!GTK_WIDGET_VISIBLE(vmixerControl.window));
        }
        fprintf(f, "\n");
        fclose(f);
      }
    }
  }

  if (VUwindow) {
    SetVUmeters(0);
    gtk_timeout_remove(VUtimer);
  }
  if (GMwindow) {
    SetVUmeters(0);
    gtk_timeout_remove(Mixtimer);
  }
  snd_ctl_close(ctlhandle);
  return(0);
}

