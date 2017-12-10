/*
 * Purpose: This is the ossxmix (GTK++ GUI) program shipped with OSS
 *
 * Description:
 * The {!xlink ossxmix} program is the primary mixer and control panel utility
 * available for OSS. It shows how the new mixer API of OSS can be
 * used in GUI type of programs See the "{!link mixer}" section of the
 * OSS Developer's manual for more info.
 *
 * This program is fully dynamic as required by the mixer interface. It doesn't
 * contain anything that is specific to certain device. All the mixer structure
 * information is loaded in the beginning of the program by using the
 * {!nlink SNDCTL_MIX_EXTINFO} ioctl (and the related calls).
 *
 * Note that this program was written before the final mixer API
 * was ready. For this reason handling of some special situations is missing
 * or incompletely implemented. For example handling of the
 * {!nlink EIDRM} is "emulated" simply by closing and re-execing the
 * program. This is bit iritating but works.
 *
 * What might be interesting in this program is how to create the GUI layout
 * based on the control tree obtained using the SNDCTL_MIX_EXTINFO routine.
 * However unfortunately this part of the program is not particularily easy
 * understand.
 *
 * {!notice Please read the mixer programming documentation very carefully
 * before studying this program.
 *
 * The {!nlink ossmix.c} program is a command line version of this one.
 *
 * The {!nlink mixext.c} program is a very simple program that shows how
 * "non-mixer" applications can do certain mixer changes.
 *
 * This program uses a "LED" bar widget contained in gtkvu.c.
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

#ifdef __hpux
#define G_INLINE_FUNC
#endif
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <soundcard.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "gtkvu.h"
#include "ossxmix.xpm"

#undef TEST_JOY
#undef DEBUG

#ifndef GTK1_ONLY
#include <gtk/gtkversion.h>
#if GTK_CHECK_VERSION(2,10,0) && !defined(GDK_WINDOWING_DIRECTFB)
#define STATUSICON
#endif /* GTK_CHECK_VERSION(2,10,0) && !GDK_WINDOWING_DIRECTFB */
#else
#include <gdk/gdkx.h>
#endif /* !GTK1_ONLY */

#ifdef TEST_JOY
#include "gtkjoy.h"
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

static int boomer_workaround = 0;

#define MAX_DEVS 16
static int set_counter[MAX_DEVS] = { 0 };
static int prev_update_counter[MAX_DEVS] = { 0 };
static oss_mixext_root * root[MAX_DEVS] = { NULL };
static oss_mixext * extrec[MAX_DEVS] = { NULL };
static int global_fd = -1;		/* Global /dev/mixer fd for SNDCTL_SYSINFO/MIXERINFO/etc */
static int local_fd[MAX_DEVS];		/* Mixer specific fd(s) for actual mixer access */
static int dev = -1;
static int show_all = 1;
static int fully_started = 0;
static int load_all_devs = 1;
static int background = 0;
static int show_status_icon = 1;

int width_adjust = 0;

#define LEFT 	1
#define RIGHT	2
#define MONO	3
#define BOTH	4

static guint poll_tag_list[4] = { 0 };
#define PEAK_DECAY		 6
#define PEAK_POLL_INTERVAL	50
#define VALUE_POLL_INTERVAL	5000
#define MIXER_POLL_INTERVAL	100
#define MIXNUM_POLL_INTERVAL	5000
static int mixer_num;

#ifndef EIDRM
#define EIDRM EFAULT
#endif

typedef enum uflag {
  WHAT_LABEL,
  WHAT_UPDATE,
  WHAT_VMIX
}
uflag_t;

typedef struct ctlrec
{
  struct ctlrec *next;
  oss_mixext *mixext;
  GtkObject *left, *right;
  GtkWidget *gang, *frame;
#define FRAME_NAME_LENGTH 8
  char frame_name[FRAME_NAME_LENGTH+1];
  int last_left, last_right;
  int full_scale;
  uflag_t what_to_do;
  int parm;
}
ctlrec_t;

static ctlrec_t * control_list[MAX_DEVS] = { NULL };
static ctlrec_t * peak_list[MAX_DEVS] = { NULL };
static ctlrec_t * value_poll_list[MAX_DEVS] = { NULL };
static ctlrec_t * check_list[MAX_DEVS] = { NULL };

static GtkWidget * window, * scrolledwin;

static gint add_timeout (gpointer);
static void change_enum (GtkToggleButton *, gpointer);
static void change_on_off (GtkToggleButton *, gpointer);
static void check_tooltip (oss_mixext *, GtkWidget *);
static void cleanup (void);
static gint close_request (GtkWidget *, gpointer);
static void connect_enum (oss_mixext *, GtkObject *);
static void connect_onoff (oss_mixext *, GtkObject *);
static void connect_peak (oss_mixext *, GtkWidget *, GtkWidget *);
static void connect_scrollers (oss_mixext *, GtkObject *,
                               GtkObject *, GtkWidget *);
static void connect_value_poll (oss_mixext *, GtkWidget *);
static void create_update (GtkWidget *, GtkObject *, GtkObject *, GtkWidget *,
                           oss_mixext *, uflag_t, int);
static GtkRequisition create_widgets (void);
static char * cut_name (char *);
static void do_update (ctlrec_t *);
static int findenum (oss_mixext *, const char *);
static int find_default_mixer (void);
static void gang_change (GtkToggleButton *, gpointer);
static int get_fd (int);
static int get_value (oss_mixext *);
static GtkWidget * load_devinfo (int);
static GList * load_enum_values (char *, oss_mixext *);
static GtkWidget * load_multiple_devs (void);
static void manage_label (GtkWidget *, oss_mixext *);
#ifndef GTK1_ONLY
static gint manage_timeouts (GtkWidget *, GdkEventWindowState *, gpointer);
#endif /* !GTK1_ONLY */
static void parse_dimarg (const char *, GtkRequisition *);
static gint poll_all (gpointer);
static gint poll_peaks (gpointer);
static gint poll_values (gpointer);
static gint poll_mixnum (gpointer);
static void reload_gui (void);
static gint remove_timeout (gpointer);
static void Scrolled (GtkAdjustment *, gpointer);
static int set_value (oss_mixext *, int);
static char * showenum (oss_mixext *, int);
static void store_name (int, int, char *, char **);
static void switch_page (GtkNotebook *, GtkNotebookPage *, guint, gpointer);
static void update_label (oss_mixext *, GtkWidget *, int);
#ifdef STATUSICON
static void activate_mainwindow (GtkStatusIcon *, guint, guint, gpointer);
static void popup_mainwindow (GtkWidget *, gpointer);
static void trayicon_popupmenu (GtkStatusIcon *, guint, guint, gpointer);

static GtkStatusIcon *status_icon = NULL;
#endif /* STATUSICON */

static int
get_fd (int dev)
{
	int fd;
	oss_mixerinfo mi;

	if (dev < 0 || dev >= MAX_DEVS)
	   {
		   fprintf (stderr, "Bad mixer device number %d\n", dev);
		   exit (EXIT_FAILURE);
	   }

	if (local_fd[dev] != -1)
	   return local_fd[dev];

	mi.dev = dev;
	if (ioctl (global_fd, SNDCTL_MIXERINFO, &mi) == -1)
	   {
		perror ("SNDCTL_MIXERINFO");
		exit (EXIT_FAILURE);
	   }

	if ((fd = open (mi.devnode, O_RDWR, 0)) == -1)
	   {
		perror (mi.devnode);
	   }

	return local_fd[dev] = fd;
}

static void
check_tooltip (oss_mixext * rec, GtkWidget * wid)
{
  oss_mixer_enuminfo ei;
  char *p;

  if (!(rec->flags & MIXF_DESCR)) /* No description available */
     return;

  ei.dev = rec->dev;
  ei.ctrl = rec->ctrl;

  if (ioctl (get_fd(rec->dev), SNDCTL_MIX_DESCRIPTION, &ei) == -1)
     return;

/*
 * Separate the first line which contains the tooltip from the subsequent lines
 * which contain the optional help text.
 */
  p=ei.strings;

  while (*p && *p != '\n') p++; /* Find a line feed */

  if (*p=='\n')
     *p++=0;
  if (*p==0)
     p=NULL;

#if GTK_CHECK_VERSION(2,12,0)
  gtk_widget_set_tooltip_text(wid, ei.strings);
#else
  {
	  GtkTooltips *tip;

	  tip = gtk_tooltips_new();
	  gtk_tooltips_set_tip(tip, wid, ei.strings, p);
  }
#endif
}

static void
store_name (int dev, int n, char *name, char **extnames)
{
  char *src = name;
  size_t i, l;

  l = strlen (name);
  for (i = 0; i < l; i++)
    {
      if (name[i] >= 'A' && name[i] <= 'Z')
        name[i] += 32;
      if (name[i] == '.')  src = name + i + 1;
    }

  extnames[n] = g_strdup (src);
#ifdef DEBUG
  fprintf (stderr, "Control = %s\n", name);
#endif
}

static char *
cut_name (char * name)
{
  char *s = name;
  while (*s)
    if (*s++ == '_')
      return s;

  if (name[0] == '@')
    return &name[1];

  return name;
}

static char *
showenum (oss_mixext * rec, int val)
{
  static char tmp[100];
  oss_mixer_enuminfo ei;

  if (val > rec->maxvalue)
    {
      snprintf (tmp, sizeof(tmp), "%d(too large (%d)?)", val, rec->maxvalue);
      return tmp;
    }

  ei.dev = rec->dev;
  ei.ctrl = rec->ctrl;

  if (ioctl (get_fd(rec->dev), SNDCTL_MIX_ENUMINFO, &ei) != -1)
    {
      char *p;

      if (val >= ei.nvalues)
	{
	  snprintf (tmp, sizeof(tmp), "%d(too large2 (%d)?)", val, ei.nvalues);
	  return tmp;
	}

      p = ei.strings + ei.strindex[val];
      strncpy (tmp, p, sizeof(tmp));
      return tmp;
    }

  snprintf (tmp, sizeof(tmp), "%d", val);
  return tmp;
}

static GList *
load_enum_values (char *extname, oss_mixext * rec)
{
  int i;
  GList *list = NULL;
  oss_mixer_enuminfo ei;

  ei.dev = rec->dev;
  ei.ctrl = rec->ctrl;

  if (ioctl (get_fd(rec->dev), SNDCTL_MIX_ENUMINFO, &ei) != -1)
    {
      int n = ei.nvalues;
      char *p;

      if (n > rec->maxvalue)
	n = rec->maxvalue;

      for (i = 0; i < rec->maxvalue; i++)
	if (rec->enum_present[i / 8] & (1 << (i % 8)))
	  {
	    p = ei.strings + ei.strindex[i];
	    list = g_list_append (list, g_strdup (p));
	  }

      return list;
    }

  if (*extname == '.')
    extname++;

  for (i = 0; i < rec->maxvalue; i++)
    if (rec->enum_present[i / 8] & (1 << (i % 8)))
      {
	list = g_list_append (list, g_strdup (showenum (rec, i)));
      }

  return list;
}

static int
findenum (oss_mixext * rec, const char * arg)
{
  int i, v;
  oss_mixer_enuminfo ei;

  ei.dev = rec->dev;
  ei.ctrl = rec->ctrl;

  if (ioctl (get_fd(rec->dev), SNDCTL_MIX_ENUMINFO, &ei) != -1)
    {
      int n = ei.nvalues;
      char *p;

      if (n > rec->maxvalue)
	n = rec->maxvalue;

      for (i = 0; i < rec->maxvalue; i++)
	if (rec->enum_present[i / 8] & (1 << (i % 8)))
	  {
	    p = ei.strings + ei.strindex[i];
	    if (strcmp (p, arg) == 0)
	      return i;
	  }
    }

  if (sscanf (arg, "%d", &v) == 1)
    return v;

  fprintf (stderr, "Invalid enumerated value '%s'\n", arg);
  return 0;
}

static int
get_value (oss_mixext * thisrec)
{
  oss_mixer_value val;

  val.dev = thisrec->dev;
  val.ctrl = thisrec->ctrl;
  val.timestamp = thisrec->timestamp;

  if (ioctl (get_fd(thisrec->dev), SNDCTL_MIX_READ, &val) == -1)
    {
      if (errno == EPIPE)
	{
#if 0
	  fprintf (stderr,
		   "ossxmix: Mixer device disconnected from the system\n");
#endif
	  return 0;
	}

      if (errno == EIDRM)
      {
	if (fully_started)
	  {
/*
 * The mixer structure got changed for some reason. This program
 * is not designed to handle this event properly so all we can do
 * is to recreate the entire GUI.
 *
 * Well written applications should just dispose the changed GUI elements 
 * (by comparing the {!code timestamp} fields. Then the new fields can be
 * created just like we did when starting the program.
 */
	    fprintf (stderr,
		     "ossxmix: Mixer structure changed - restarting.\n");
	    reload_gui ();
	    return -1;
	  }
	else
	  {
	    fprintf (stderr,
		     "ossxmix: Mixer structure changed - aborting.\n");
	    exit (-1);
	  }
      }
      fprintf (stderr, "%s\n", thisrec->id);
      perror ("SNDCTL_MIX_READ");
      return -1;
    }

  return val.value;
}

static int
set_value (oss_mixext * thisrec, int value)
{
  oss_mixer_value val;

  if (!(thisrec->flags & MIXF_WRITEABLE))
    return -1;
  val.dev = thisrec->dev;
  val.ctrl = thisrec->ctrl;
  val.value = value;
  val.timestamp = thisrec->timestamp;
  set_counter[thisrec->dev]++;

  if (ioctl (get_fd(thisrec->dev), SNDCTL_MIX_WRITE, &val) == -1)
    {
      if (errno == EIDRM)
      {
	if (fully_started)
	  {
	    fprintf (stderr,
		     "ossxmix: Mixer structure changed - restarting.\n");
	    reload_gui ();
	    return -1;
	  }
	else
	  {
	    fprintf (stderr,
		     "ossxmix: Mixer structure changed - aborting.\n");
	    exit (-1);
	  }
      }
      fprintf (stderr, "%s\n", thisrec->id);
      perror ("SNDCTL_MIX_WRITE");
      return -1;
    }
  return val.value;
}

static void
create_update (GtkWidget * frame, GtkObject * left, GtkObject * right,
	       GtkWidget * gang, oss_mixext * thisrec, uflag_t what, int parm)
{
  ctlrec_t *srec;

  srec = g_new (ctlrec_t, 1);
  srec->mixext = thisrec;
  srec->parm = parm;
  srec->what_to_do = what;
  srec->frame = frame;
  srec->left = left;
  srec->right = right;
  srec->gang = gang;
  srec->frame_name[0] = '\0';

  srec->next = check_list[thisrec->dev];
  check_list[thisrec->dev] = srec;
}

static void
manage_label (GtkWidget * frame, oss_mixext * thisrec)
{
  char new_label[FRAME_NAME_LENGTH+1], tmp[16];

  if (thisrec->id[0] != '@')
    return;

  *new_label = 0;

  strncpy (tmp, &thisrec->id[1], sizeof(tmp));
  tmp[15] = '\0';

  if ((tmp[0] == 'd' && tmp[1] == 's' && tmp[2] == 'p') ||
      (tmp[0] == 'p' && tmp[1] == 'c' && tmp[2] == 'm'))
    {
      int dspnum;
      oss_audioinfo ainfo;

      if (sscanf (&tmp[3], "%d", &dspnum) != 1)
	return;

      ainfo.dev = dspnum;
      if (ioctl (global_fd, SNDCTL_ENGINEINFO, &ainfo) == -1)
	{
	  perror ("SNDCTL_ENGINEINFO");
	  return;
	}
      create_update (frame, NULL, NULL, NULL, thisrec, WHAT_LABEL, dspnum);
      if (*ainfo.label != 0)
	{
	  strncpy (new_label, ainfo.label, FRAME_NAME_LENGTH);
	  new_label[FRAME_NAME_LENGTH] = 0;
	}
    }


  if (*new_label != 0)
    gtk_frame_set_label (GTK_FRAME (frame), new_label);

}

static void
Scrolled (GtkAdjustment * adjust, gpointer data)
{
  int val, origval, lval, rval;
  int side, gang_on;
  ctlrec_t *srec = (ctlrec_t *) data;
  int shift = 8;

  val = srec->mixext->maxvalue - (int) adjust->value;
  origval = (int) adjust->value;

  if (srec->mixext->type == MIXT_MONOSLIDER16
      || srec->mixext->type == MIXT_STEREOSLIDER16)
    shift = 16;

  if (srec->full_scale)
    side = BOTH;
  else if (srec->right == NULL)
    side = MONO;
  else if (GTK_OBJECT (adjust) == srec->left)
    side = LEFT;
  else
    side = RIGHT;

  if (srec->mixext->type == MIXT_3D)
    {
#ifdef TEST_JOY
#else
      lval = 100 - (int) GTK_ADJUSTMENT (srec->left)->value;
      rval = 360 - (int) GTK_ADJUSTMENT (srec->right)->value;
      val = (50 << 8) | (lval & 0xff) | (rval << 16);
      set_value (srec->mixext, val);
#endif
      return;
    }

  if (side == BOTH)
    {
      set_value (srec->mixext, val);
      return;
    }

  if (side == MONO)
    {
      val = val | (val << shift);
      set_value (srec->mixext, val);
      return;
    }

  gang_on = 0;

  if (srec->gang != NULL)
    {
      gang_on = GTK_TOGGLE_BUTTON (srec->gang)->active;
    }

  if (gang_on)
    {
      gtk_adjustment_set_value (GTK_ADJUSTMENT (srec->left), origval);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (srec->right), origval);

      val = val | (val << shift);
      set_value (srec->mixext, val);

      return;
    }

  lval = srec->mixext->maxvalue - (int) GTK_ADJUSTMENT (srec->left)->value;
  rval = srec->mixext->maxvalue - (int) GTK_ADJUSTMENT (srec->right)->value;
  val = lval | (rval << shift);
  set_value (srec->mixext, val);
}

static void
gang_change (GtkToggleButton * but, gpointer data)
{
  ctlrec_t *srec = (ctlrec_t *) data;
  int val, aval, lval, rval;
  int mask = 0xff, shift = 8;

  if (!but->active)
    return;

  lval = srec->mixext->maxvalue - (int) GTK_ADJUSTMENT (srec->left)->value;
  rval = srec->mixext->maxvalue - (int) GTK_ADJUSTMENT (srec->right)->value;
  if (lval == rval)
    return;

  if (lval < rval)
     lval = rval;

  if (srec->mixext->type == MIXT_STEREOSLIDER16)
    {
      shift = 16;
      mask = 0xffff;
    }

  val = lval | (lval << shift);
  aval = srec->mixext->maxvalue - (val & mask);

  gtk_adjustment_set_value (GTK_ADJUSTMENT (srec->left), aval);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (srec->right), aval);
  set_value (srec->mixext, val);
}

/*ARGSUSED*/
static void
change_enum (GtkToggleButton * but, gpointer data)
{
  ctlrec_t *srec = (ctlrec_t *) data;
  int val;
  const char *entry;

  entry = gtk_entry_get_text (GTK_ENTRY (srec->left));
  if (*entry == 0)		/* Empty - Why? */
    return;

  val = findenum (srec->mixext, entry);

  set_value (srec->mixext, val);
}


static void
change_on_off (GtkToggleButton * but, gpointer data)
{
  ctlrec_t *srec = (ctlrec_t *) data;
  int val;

  val = but->active;

 /*
  * old OSSv4 builds had a bug where SNDCTL_MIX_WRITE would always return
  * 1 when changing a rec button.
  */
#if 0
  val = set_value (srec->mixext, val);
#else
  set_value (srec->mixext, val);
  val = get_value (srec->mixext);
#endif
  if (val != -1) but->active = val;
}

static void
store_ctl (ctlrec_t * rec, int dev)
{
  rec->next = control_list[dev];
  control_list[dev] = rec;
}

static void
connect_scrollers (oss_mixext * thisrec, GtkObject * left, GtkObject * right,
		   GtkWidget * gang)
{
  ctlrec_t *srec;

  srec = g_new (ctlrec_t, 1);
  srec->mixext = thisrec;
  srec->left = left;
  srec->right = right;
  srec->full_scale = (thisrec->type == MIXT_SLIDER);
  srec->gang = gang;
  gtk_signal_connect (GTK_OBJECT (left), "value_changed",
		      GTK_SIGNAL_FUNC (Scrolled), srec);
  if (right != NULL)
    gtk_signal_connect (GTK_OBJECT (right), "value_changed",
			GTK_SIGNAL_FUNC (Scrolled), srec);
  if (gang != NULL)
    gtk_signal_connect (GTK_OBJECT (gang), "toggled",
			GTK_SIGNAL_FUNC (gang_change), srec);

  store_ctl (srec, thisrec->dev);

}

static void
connect_peak (oss_mixext * thisrec, GtkWidget * left, GtkWidget * right)
{
  ctlrec_t *srec;

  srec = g_new (ctlrec_t, 1);
  srec->mixext = thisrec;
  srec->left = GTK_OBJECT (left);
  if (right == NULL)
    srec->right = NULL;
  else
    srec->right = GTK_OBJECT (right);
  srec->gang = NULL;
  srec->last_left = 0;
  srec->last_right = 0;

  srec->next = peak_list[thisrec->dev];
  peak_list[thisrec->dev] = srec;
}

static void
connect_value_poll (oss_mixext * thisrec, GtkWidget * wid)
{
  ctlrec_t *srec;

  srec = g_new (ctlrec_t, 1);
  srec->mixext = thisrec;
  srec->left = GTK_OBJECT (wid);
  srec->right = NULL;
  srec->gang = NULL;
  srec->last_left = 0;
  srec->last_right = 0;

  srec->next = value_poll_list[thisrec->dev];
  value_poll_list[thisrec->dev] = srec;
}

static void
connect_enum (oss_mixext * thisrec, GtkObject * entry)
{
  ctlrec_t *srec;

  srec = g_new (ctlrec_t, 1);
  srec->mixext = thisrec;
  srec->left = entry;
  srec->right = NULL;
  srec->gang = NULL;
  gtk_signal_connect (entry, "changed", GTK_SIGNAL_FUNC (change_enum), srec);
  store_ctl (srec, thisrec->dev);

}

static void
connect_onoff (oss_mixext * thisrec, GtkObject * entry)
{
  ctlrec_t *srec;

  srec = g_new (ctlrec_t, 1);
  srec->mixext = thisrec;
  srec->left = entry;
  srec->right = NULL;
  srec->gang = NULL;
  gtk_signal_connect (entry, "toggled", GTK_SIGNAL_FUNC (change_on_off), srec);
  store_ctl (srec, thisrec->dev);

}

/*
 * Create notebook and populate it with multiple mixer tabs. Returns notebook.
 */
static GtkWidget *
load_multiple_devs (void)
{
  int i, first_page = -1;
  GtkWidget *notebook, *mixer_page, *label, *vbox, *hbox;

  if (ioctl (global_fd, SNDCTL_MIX_NRMIX, &mixer_num) == -1)
    {
      perror ("SNDCTL_MIX_NRMIX");
      exit (-1);
    }

  if (mixer_num > MAX_DEVS) mixer_num = MAX_DEVS;

  /* This can happen when ossxmix is restarted by {get/set}_value */
  if (dev > mixer_num - 1) dev = find_default_mixer ();

  notebook = gtk_notebook_new ();
  for (i = 0; i < mixer_num; i++)
    {
      if (get_fd(i) == -1) continue;
      mixer_page = load_devinfo (i);
      if (mixer_page == NULL) continue;
      if (first_page == -1) first_page = i;
      vbox = gtk_vbox_new (FALSE, 0);
      hbox = gtk_hbox_new (FALSE, 0);
      gtk_box_pack_start (GTK_BOX (vbox), mixer_page, FALSE, TRUE, 0);
      gtk_box_pack_end (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);
      gtk_widget_show (vbox);
      if (root[i] == NULL)
        {
          fprintf (stderr, "No device root node for mixer %d\n", i);
          exit (-1);
        }
      label = gtk_label_new (root[i]->name);
      gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);
    }

  if (root[dev] != NULL) first_page = dev;
  else if (first_page != -1) dev = first_page;
  else
    {
      fprintf (stderr, "No mixers could be opened\n");
      exit (EXIT_FAILURE);
    }

#ifndef GTK1_ONLY
  gtk_notebook_set_current_page (GTK_NOTEBOOK (notebook), dev);
#else
  gtk_notebook_set_page (GTK_NOTEBOOK (notebook), dev);
#endif /* !GTK1_ONLY */
  gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
  gtk_signal_connect (GTK_OBJECT (notebook), "switch-page",
                      GTK_SIGNAL_FUNC (switch_page), (gpointer)poll_tag_list);
  gtk_widget_show (notebook);

  return notebook;
}

/*
 * The load_devinfo() routine loads the mixer definitions and creates the
 * GUI structure based on it.
 *
 * In short the algorithm is to create GTK vbox or hbox widgets for
 * each group. A vbox is created for the root group. Then the orientation is
 * changed in each level of sub-groups. However there are some exceptions to
 * this rule (will be described in the documentation.
 *
 * The individual controls are just placed inside the hbox/vbx widgets of
 * the parent groups. However the "legacy" mixer controls (before
 * MIXT_MARKER) will be handled in slightly different way (please consult
 * the documentation).
 */
static GtkWidget *
load_devinfo (int dev)
{
  char tmp[1024], *name = '\0';
  char ** extnames;
  int i, n, val, left, right, mx, g, mask, shift;
  int angle, vol;
  int width;
  int ngroups = 0;
  int parent = 0;
  int change_color;
  oss_mixext *thisrec;
  oss_mixerinfo mi;
  GdkColor color;
  GtkWidget *wid, *wid2, *gang, *rootwid = NULL, *pw = NULL, *frame, *box;
  GtkWidget **widgets;
  GtkObject *adjust, *adjust2;
  gboolean change_orient = TRUE, ori, * orient;
  gboolean expand, use_layout_b = FALSE;

  mi.dev = dev;
  if (ioctl (global_fd, SNDCTL_MIXERINFO, &mi) == -1)
    {
      perror ("SNDCTL_MIXERINFO");
      exit (-1);
    }

  if (!mi.enabled) return NULL;
  /* e.g. disconnected USB device */

  if (mi.caps & MIXER_CAP_LAYOUT_B)
    use_layout_b = TRUE;

  if ((mi.caps & MIXER_CAP_NARROW) && (width_adjust >= 0))
    width_adjust = -1;

  n = mi.nrext;
  if (n < 1)
    {
      fprintf (stderr, "Error: illogical number of extension info records\n");
      return NULL;
    }
  extrec[dev] = g_new0 (oss_mixext, n+1);
  extnames = g_new (char *, n+1);
  widgets = g_new0 (GtkWidget *, n);
  orient = g_new0 (gboolean, n);

  for (i = 0; i < n; i++)
    {
      change_color = 0;
      mask = 0xff;
      shift = 8;
      expand = TRUE;

      thisrec = &extrec[dev][i];
      thisrec->dev = dev;
      thisrec->ctrl = i;

      if (ioctl (get_fd(dev), SNDCTL_MIX_EXTINFO, thisrec) == -1)
	{
	  if (errno == EINVAL)
	      printf ("Incompatible OSS version\n");
	  else
	      perror ("SNDCTL_MIX_EXTINFO");
	  exit (-1);
	}

      if (thisrec->id[0] == '-')	/* Hidden one */
	thisrec->id[0] = '\0';

      if (thisrec->type == MIXT_STEREOSLIDER16
	  || thisrec->type == MIXT_MONOSLIDER16)
	{
	  mask = 0xffff;
	  shift = 16;
	}

      if ((thisrec->type != MIXT_DEVROOT) && (thisrec->type != MIXT_MARKER))
        {
          parent = thisrec->parent;
          name = cut_name (thisrec->id);
          if ((thisrec->type == MIXT_GROUP) && !change_orient && (parent == 0))
            pw = rootwid;
          else
            pw = widgets[parent];
          if ((pw == NULL) && (show_all))
	    fprintf (stderr, "Control %d/%s: Parent(%d)==NULL\n", i,
                     thisrec->extname, parent);
        }

#if OSS_VERSION >= 0x040004
      if (thisrec->rgbcolor != 0)
      {
	/*
	 * Pick the 8 bit RGB component colors and expand them to 16 bits
	 */
      	color.red =	(thisrec->rgbcolor & 0xff0000) >> 8;
      	color.green =	(thisrec->rgbcolor & 0x00ff00);
      	color.blue =	(thisrec->rgbcolor & 0x0000ff) << 8;
	change_color=1;

      }
#endif

      switch (thisrec->type)
	{
	case MIXT_DEVROOT:
	  root[dev] = (oss_mixext_root *) & thisrec->data;
	  extnames[i] = g_strdup("");
	  rootwid = gtk_vbox_new (FALSE, 2);
	  gtk_box_set_child_packing (GTK_BOX (rootwid), rootwid, TRUE, TRUE,
                                    100, GTK_PACK_START);
	  wid = gtk_hbox_new (FALSE, 1);
	  gtk_box_pack_start (GTK_BOX (rootwid), wid, TRUE, TRUE, 1);
	  gtk_widget_show_all (rootwid);
	  widgets[i] = wid;
	  break;

	case MIXT_GROUP:
	  if (!show_all)
	    break;
#if OSS_VERSION >= 0x040090
	  if (!boomer_workaround) /* Boomer 4.0 doesn't provide update_counters */
	  if (thisrec->update_counter == 0)
	    break;
#endif

	  if (*extnames[parent] == '\0')
	    strcpy (tmp, name);
	  else
            snprintf (tmp, sizeof(tmp), "%s.%s", extnames[parent], name);
	  store_name (dev, i, tmp, extnames);
	  if (thisrec->flags & MIXF_FLAT)	/* Group contains only ENUM controls */
	    expand = FALSE;
	  ori = !orient[parent];
	  if (change_orient)
	    ori = !ori;
	  orient[i] = ori;

	  switch (ori)
	    {
	    case 0:
	      wid = gtk_vbox_new (FALSE, 1);
	      break;

	    default:
	      ngroups++;
	      if (!use_layout_b)
		ori = !ori;
	      orient[i] = ori;
	      wid = gtk_hbox_new (FALSE, 1);
	    }

	  frame = gtk_frame_new (extnames[i]);
	  manage_label (frame, thisrec);
	  gtk_box_pack_start (GTK_BOX (pw), frame, expand, TRUE, 1);
	  gtk_container_add (GTK_CONTAINER (frame), wid);
	  gtk_widget_set_name (wid, extnames[i]);
	  gtk_widget_show_all (frame);
	  widgets[i] = wid;
	  {
	    int tmp = -1;

            if ((sscanf (extnames[i], "vmix%d-out", &tmp) == 1) &&
		(tmp >= 0))
	      {
		create_update (NULL, NULL, NULL, wid, thisrec, WHAT_VMIX, n);
	      }
	  }
	  break;

	case MIXT_HEXVALUE:
	case MIXT_VALUE:
	  if (!show_all)
	    break;
	  if (*thisrec->id == 0)
	    extnames[i] = extnames[parent];
	  else
	    {
	      snprintf (tmp, sizeof(tmp), "%s.%s", extnames[parent], name);
	      store_name (dev, i, tmp, extnames);
	    }
	  val = get_value (thisrec);

	  wid = gtk_label_new ("????");
	  gtk_box_pack_start (GTK_BOX (pw), wid, FALSE, TRUE, 0);
	  if (thisrec->flags & MIXF_POLL)
	    connect_value_poll (thisrec, wid);
	  else
	    create_update (NULL, NULL, NULL, wid, thisrec, WHAT_UPDATE, i);
	  gtk_widget_set_name (wid, extnames[i]);
	  update_label (thisrec, wid, val);
	  check_tooltip(thisrec, wid);
	  gtk_widget_show (wid);
	  break;

	case MIXT_STEREODB:
	case MIXT_MONODB:
	  if (!show_all)
	    break;
	  if (*thisrec->id == 0)
	    extnames[i] = extnames[parent];
	  else
	    {
	      snprintf (tmp, sizeof(tmp), "%s.%s", extnames[parent], name);
	      store_name (dev, i, tmp, extnames);
	    }
	  wid = gtk_button_new_with_label (extnames[i]);
	  gtk_box_pack_start (GTK_BOX (pw), wid, FALSE, TRUE, 0);
	  gtk_widget_set_name (wid, extnames[i]);
	  check_tooltip(thisrec, wid);
	  gtk_widget_show (wid);
	  break;

	case MIXT_ONOFF:
#ifdef MIXT_MUTE
	case MIXT_MUTE: /* TODO: Mute could have custom widget */
#endif /* MIXT_MUTE */
	  if (!show_all)
	    break;
	  val = get_value (thisrec) & 0x01;
	  if (*thisrec->id == 0)
	    extnames[i] = extnames[parent];
	  else
	    {
	      snprintf (tmp, sizeof(tmp), "%s.%s", extnames[parent], name);
	      store_name (dev, i, tmp, extnames);
	    }
          wid = gtk_check_button_new_with_label (extnames[i]);
#ifndef GTK1_ONLY
	  if (change_color)
	     gtk_widget_modify_bg (wid, GTK_STATE_NORMAL, &color);
#endif /* !GTK1_ONLY */
	  connect_onoff (thisrec, GTK_OBJECT (wid));
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wid), val);
	  create_update (NULL, NULL, NULL, wid, thisrec, WHAT_UPDATE, 0);
	  gtk_box_pack_start (GTK_BOX (pw), wid, FALSE, TRUE, 0);
	  gtk_widget_set_name (wid, extnames[i]);
	  check_tooltip(thisrec, wid);
	  gtk_widget_show (wid);
	  break;

	case MIXT_STEREOVU:
	case MIXT_STEREOPEAK:
	  if (!show_all)
	    break;
	  val = get_value (thisrec);
	  mx = thisrec->maxvalue;
	  left = mx - (val & 0xff);
	  right = mx - ((val >> 8) & 0xff);
	  if (*thisrec->id == 0)
	    extnames[i] = extnames[parent];
	  else
	    {
	      snprintf (tmp, sizeof(tmp), "%s.%s", extnames[parent], name);
	      store_name (dev, i, tmp, extnames);
	    }
	  wid = gtk_vu_new ();
	  wid2 = gtk_vu_new ();
	  check_tooltip(thisrec, wid);

	  connect_peak (thisrec, wid, wid2);
	  gtk_widget_set_name (wid, extnames[i]);
	  gtk_widget_set_name (wid2, extnames[i]);
	  if (strcmp (extnames[parent], extnames[i]) != 0)
	    {
	      frame = gtk_frame_new (extnames[i]);
	      manage_label (frame, thisrec);
	      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	      gtk_box_pack_start (GTK_BOX (pw), frame, FALSE, TRUE, 0);
	      box = gtk_hbox_new (FALSE, 1);
	      gtk_container_add (GTK_CONTAINER (frame), box);
	      gtk_box_pack_start (GTK_BOX (box), wid, TRUE, TRUE, 1);
	      gtk_box_pack_start (GTK_BOX (box), wid2, TRUE, TRUE, 1);
	      gtk_widget_show_all (frame);
	    }
	  else
	    {
	      box = gtk_hbox_new (FALSE, 1);
	      gtk_box_pack_start (GTK_BOX (pw), box, FALSE, TRUE, 1);
	      gtk_box_pack_start (GTK_BOX (box), wid, TRUE, TRUE, 1);
	      gtk_box_pack_start (GTK_BOX (box), wid2, TRUE, TRUE, 1);
	      gtk_widget_show_all (box);
	    }
	  break;

	case MIXT_MONOVU:
	case MIXT_MONOPEAK:
	  if (!show_all)
	    break;
	  val = get_value (thisrec);
	  mx = thisrec->maxvalue;
	  left = mx - (val & 0xff);
	  if (*thisrec->id == 0)
	    extnames[i] = extnames[parent];
	  else
	    {
	      snprintf (tmp, sizeof(tmp), "%s.%s", extnames[parent], name);
	      store_name (dev, i, tmp, extnames);
	    }
	  wid = gtk_vu_new ();
	  check_tooltip(thisrec, wid);

	  connect_peak (thisrec, wid, NULL);
	  gtk_widget_set_name (wid, extnames[i]);
	  if (strcmp (extnames[parent], extnames[i]) != 0)
	    {
	      frame = gtk_frame_new (extnames[i]);
	      manage_label (frame, thisrec);
	      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	      gtk_box_pack_start (GTK_BOX (pw), frame, FALSE, TRUE, 0);
	      box = gtk_hbox_new (FALSE, 1);
	      gtk_container_add (GTK_CONTAINER (frame), box);
	      gtk_box_pack_start (GTK_BOX (box), wid, TRUE, TRUE, 1);
	      gtk_widget_show_all (frame);
	    }
	  else
	    {
	      box = gtk_hbox_new (FALSE, 1);
	      gtk_box_pack_start (GTK_BOX (pw), box, FALSE, TRUE, 1);
	      gtk_box_pack_start (GTK_BOX (box), wid, TRUE, TRUE, 1);
	      gtk_widget_show_all (box);
	    }
	  break;

	case MIXT_STEREOSLIDER:
	case MIXT_STEREOSLIDER16:
	  if (!show_all)
	    break;
	  width = -1;

	  if (width_adjust < 0)
	    width = 12;
	  val = get_value (thisrec);
	  mx = thisrec->maxvalue;
	  left = mx - (val & mask);
	  right = mx - ((val >> shift) & mask);
	  if (*thisrec->id == 0)
	    extnames[i] = extnames[parent];
	  else
	    {
	      snprintf (tmp, sizeof(tmp), "%s.%s", extnames[parent], name);
	      store_name (dev, i, tmp, extnames);
	    }
	  adjust = gtk_adjustment_new (left, 0, mx, 1, 5, 0);
	  adjust2 = gtk_adjustment_new (right, 0, mx, 1, 5, 0);
	  gang = gtk_check_button_new ();
	  g = (left == right);
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gang), g);

	  connect_scrollers (thisrec, adjust, adjust2, gang);
	  create_update (NULL, adjust, adjust2, gang, thisrec, WHAT_UPDATE,
			 0);

	  wid = gtk_vscale_new (GTK_ADJUSTMENT (adjust));
	  check_tooltip(thisrec, wid);
#ifndef GTK1_ONLY
	  if (change_color)
	     gtk_widget_modify_bg (wid, GTK_STATE_NORMAL, &color);
	  gtk_widget_set_size_request (wid, width, 80);
#endif /* !GTK1_ONLY */
	  gtk_scale_set_digits (GTK_SCALE (wid), 0);
	  gtk_scale_set_draw_value (GTK_SCALE (wid), FALSE);
	  gtk_widget_set_name (wid, extnames[i]);

	  wid2 = gtk_vscale_new (GTK_ADJUSTMENT (adjust2));
#ifndef GTK1_ONLY
	  if (change_color)
	     gtk_widget_modify_bg (wid2, GTK_STATE_NORMAL, &color);
	  gtk_widget_set_size_request (wid2, width, 80);
#endif /* !GTK1_ONLY */
	  gtk_scale_set_digits (GTK_SCALE (wid2), 0);
	  gtk_scale_set_draw_value (GTK_SCALE (wid2), FALSE);
	  gtk_widget_set_name (wid2, extnames[i]);

	  if (strcmp (extnames[parent], extnames[i]) != 0)
	    {
	      frame = gtk_frame_new (extnames[i]);
	      manage_label (frame, thisrec);
	      /* gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN); */
	      gtk_box_pack_start (GTK_BOX (pw), frame, FALSE, FALSE, 1);
	      box = gtk_hbox_new (FALSE, 1);
	      gtk_container_add (GTK_CONTAINER (frame), box);
	      gtk_box_pack_start (GTK_BOX (box), wid, FALSE, TRUE, 0);
	      gtk_box_pack_start (GTK_BOX (box), wid2, FALSE, TRUE, 0);
	      gtk_box_pack_start (GTK_BOX (box), gang, FALSE, TRUE, 0);
	      gtk_widget_show_all (frame);
	    }
	  else
	    {
	      box = gtk_hbox_new (FALSE, 1);
#if 1
	      gtk_box_pack_start (GTK_BOX (pw), box, TRUE, TRUE, 1);
#else
	      gtk_box_pack_start (GTK_BOX (pw), box, FALSE, FALSE, 1);
#endif
	      gtk_box_pack_start (GTK_BOX (box), wid, FALSE, TRUE, 0);
	      gtk_box_pack_start (GTK_BOX (box), wid2, FALSE, TRUE, 0);
	      gtk_box_pack_start (GTK_BOX (box), gang, FALSE, TRUE, 0);
	      gtk_widget_show_all (box);
	    }
	  break;

	case MIXT_3D:
#ifdef TEST_JOY
	  if (!show_all)
	    break;
	  val = get_value (thisrec);
	  if (*thisrec->id == 0)
	    extnames[i] = extnames[parent];
	  else
	    {
	      snprintf (tmp, sizeof(tmp), "%s.%s", extnames[parent], name);
	      store_name (dev, i, tmp, extnames);
	    }
	  wid = gtk_joy_new ();
	  check_tooltip(thisrec, wid);
	  create_update (NULL, NULL, NULL, wid, thisrec, WHAT_UPDATE, 0);

	  if (strcmp (extnames[parent], extnames[i]) != 0)
	    {
	      frame = gtk_frame_new (extnames[i]);
	      manage_label (frame, thisrec);
	      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	      gtk_box_pack_start (GTK_BOX (pw), frame, FALSE, TRUE, 0);
	      box = gtk_hbox_new (FALSE, 1);
	      gtk_container_add (GTK_CONTAINER (frame), box);
	      gtk_box_pack_start (GTK_BOX (box), wid, TRUE, TRUE, 1);
	      gtk_widget_show_all (frame);
	    }
	  else
	    {
	      box = gtk_hbox_new (FALSE, 1);
	      gtk_box_pack_start (GTK_BOX (pw), box, FALSE, TRUE, 1);
	      gtk_box_pack_start (GTK_BOX (box), wid, TRUE, TRUE, 1);
	      gtk_widget_show_all (box);
	    }
	  break;
#else
	  if (!show_all)
	    break;
	  val = get_value (thisrec);
	  mx = thisrec->maxvalue;
	  vol = 100 - (val & 0x00ff);
	  angle = 360 - ((val >> 16) & 0xffff);
	  if (*thisrec->id == 0)
	    extnames[i] = extnames[parent];
	  else
	    {
	      snprintf (tmp, sizeof(tmp), "%s.%s", extnames[parent], name);
	      store_name (dev, i, tmp, extnames);
	    }
	  adjust = gtk_adjustment_new (vol, 0, 100, 1, 5, 0);
	  adjust2 = gtk_adjustment_new (angle, 0, 360, 1, 5, 0);
	  connect_scrollers (thisrec, adjust, adjust2, NULL);
	  create_update (NULL, adjust, adjust2, NULL, thisrec, WHAT_UPDATE,
			 0);
	  wid = gtk_vscale_new (GTK_ADJUSTMENT (adjust));
	  gtk_scale_set_digits (GTK_SCALE (wid), 0);
	  gtk_scale_set_draw_value (GTK_SCALE (wid), FALSE);
	  gtk_widget_set_name (wid, extnames[i]);
	  wid2 = gtk_vscale_new (GTK_ADJUSTMENT (adjust2));
#ifndef GTK1_ONLY
	  if (change_color)
            {
	      gtk_widget_modify_bg (wid, GTK_STATE_NORMAL, &color);
	      gtk_widget_modify_bg (wid2, GTK_STATE_NORMAL, &color);
            }
#endif /* !GTK1_ONLY */
	  gtk_scale_set_digits (GTK_SCALE (wid2), 0);
	  gtk_scale_set_draw_value (GTK_SCALE (wid2), FALSE);
	  gtk_widget_set_name (wid2, extnames[i]);

	  frame = gtk_frame_new (extnames[i]);
	  manage_label (frame, thisrec);
	  /* gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN); */
	  gtk_box_pack_start (GTK_BOX (pw), frame, FALSE, FALSE, 1);
	  box = gtk_hbox_new (FALSE, 1);
	  gtk_container_add (GTK_CONTAINER (frame), box);
	  gtk_box_pack_start (GTK_BOX (box), wid, FALSE, TRUE, 0);
	  gtk_box_pack_start (GTK_BOX (box), wid2, TRUE, TRUE, 0);
	  gtk_widget_show_all (frame);
	  break;
#endif

	case MIXT_MONOSLIDER:
	case MIXT_MONOSLIDER16:
	case MIXT_SLIDER:
	  if (!show_all)
	    break;
	  val = get_value (thisrec);
	  mx = thisrec->maxvalue;

	  if (thisrec->type == MIXT_MONOSLIDER)
	    val &= 0xff;
	  else if (thisrec->type == MIXT_MONOSLIDER16)
	    val &= 0xffff;

	  val = mx - val;
	  if (*thisrec->id == 0)
	    extnames[i] = extnames[parent];
	  else
	    {
	      snprintf (tmp, sizeof(tmp), "%s.%s", extnames[parent], name);
	      store_name (dev, i, tmp, extnames);
	    }
	  adjust = gtk_adjustment_new (val, 0, mx, 1, 5, 0);
	  connect_scrollers (thisrec, adjust, NULL, NULL);
	  create_update (NULL, adjust, NULL, NULL, thisrec, WHAT_UPDATE, 0);
	  wid = gtk_vscale_new (GTK_ADJUSTMENT (adjust));
	  check_tooltip(thisrec, wid);
#ifndef GTK1_ONLY
	  if (change_color)
	     gtk_widget_modify_bg (wid, GTK_STATE_NORMAL, &color);
	  gtk_widget_set_size_request (wid, -1, 80);
#endif /* !GTK1_ONLY */
	  gtk_scale_set_digits (GTK_SCALE (wid), 0);
	  gtk_scale_set_draw_value (GTK_SCALE (wid), FALSE);
	  gtk_widget_set_name (wid, extnames[i]);

	  if (strcmp (extnames[parent], extnames[i]) != 0)
	    {
	      frame = gtk_frame_new (extnames[i]);
	      manage_label (frame, thisrec);
	      /* gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN); */
	      gtk_box_pack_start (GTK_BOX (pw), frame, FALSE, FALSE, 1);
	      gtk_container_add (GTK_CONTAINER (frame), wid);
	      gtk_widget_show_all (frame);
	    }
	  else
	    {
	      gtk_box_pack_start (GTK_BOX (pw), wid, FALSE, FALSE, 1);
	      gtk_widget_show (wid);
	    }
	  break;

	case MIXT_ENUM:

	  if (!show_all)
	    break;
	  if (*thisrec->id == 0)
	    extnames[i] = extnames[parent];
	  else
	    {
	      snprintf (tmp, sizeof(tmp), "%s.%s", extnames[parent], name);
	      store_name (dev, i, tmp, extnames);
	    }
	  val = get_value (thisrec) & 0xff;

	  wid = gtk_combo_new ();
	  check_tooltip(thisrec, wid);
#ifndef GTK1_ONLY
	  if (change_color)
	     gtk_widget_modify_fg (wid, GTK_STATE_NORMAL, &color);
#endif /* !GTK1_ONLY */
	  {
	    GList *opt = NULL;

	    if (!(thisrec->flags & MIXF_WIDE))
	      gtk_widget_set_usize (wid, 100 + 20 * width_adjust, -1);
	    opt = load_enum_values (extnames[i], thisrec);
	    gtk_combo_set_popdown_strings (GTK_COMBO (wid), opt);
	    g_list_free (opt);

	    gtk_combo_set_use_arrows_always (GTK_COMBO (wid), 1);
	    gtk_entry_set_editable (GTK_ENTRY (GTK_COMBO (wid)->entry),
				    FALSE);
	  }
	  connect_enum (thisrec, GTK_OBJECT (GTK_COMBO (wid)->entry));
	  create_update (NULL, NULL, NULL, wid, thisrec, WHAT_UPDATE, i);
	  gtk_widget_set_name (wid, extnames[i]);
	  frame = gtk_frame_new (extnames[i]);
#ifndef GTK1_ONLY
	  if (change_color)
	     gtk_widget_modify_bg (wid, GTK_STATE_NORMAL, &color);
#endif /* !GTK1_ONLY */
	  manage_label (frame, thisrec);
	  gtk_box_pack_start (GTK_BOX (pw), frame, TRUE, FALSE, 0);
	  gtk_container_add (GTK_CONTAINER (frame), wid);
	  gtk_widget_show_all (frame);
	  break;

	case MIXT_MARKER:
	  show_all = 1;
	  change_orient = FALSE;
	  break;

	default:
	  fprintf (stderr, "Unknown type for control %s\n", thisrec->extname);
	}

    }

  g_free (extnames);
  g_free (widgets);
  g_free (orient);
  return rootwid;
}

/*
 * Creates the widget tree. Returns dimensions of scrolledwin.
 */
static GtkRequisition
create_widgets (void)
{
  GtkRequisition Dimensions;
  char tmp[100];

  scrolledwin = gtk_scrolled_window_new (NULL, NULL);
  /*
   * A GtkScrolledWindow placed inside a GtkWindow is not considered to have
   * a demand for space on its parent if it's policy is GTK_POLICY_AUTOMATIC.
   * So if the window is realized, if will be reduced to the
   * GtkScrolledWindow's minimum size which is quite small.
   * 
   * To get around this, we setup scrolledwin with GTK_POLICY_NEVER, get the
   * size GTK would have used for the window in that case, and use it as a
   * default size for the window after we've reset the policy to
   * GTK_POLICY_AUTOMATIC.
   */
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwin),
                                  GTK_POLICY_NEVER, GTK_POLICY_NEVER);
  gtk_container_add (GTK_CONTAINER (window), scrolledwin);
  if (!load_all_devs)
    {
      gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolledwin),
                                             load_devinfo (dev));
      if (root[dev] == NULL)
        {
          fprintf (stderr, "No device root node\n");
          exit (-1);
        }
      snprintf (tmp, sizeof(tmp), "ossxmix - device %d / %s",
                dev, root[dev]->name);
      gtk_window_set_title (GTK_WINDOW (window), tmp);
      gtk_widget_size_request (scrolledwin, &Dimensions);
    }
  else
    {
      GtkWidget * notebook, * tab;
      GtkRequisition tDimensions;

      notebook = load_multiple_devs ();
      gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolledwin),
                                             notebook);
      gtk_window_set_title (GTK_WINDOW (window), "ossxmix");
      gtk_widget_size_request (scrolledwin, &Dimensions);

      /*
       * Dimensions.height doesn't include the tab for some reason.
       * I hate GTK.
       */  
      tab = gtk_notebook_get_tab_label (GTK_NOTEBOOK (notebook),
              gtk_notebook_get_nth_page (GTK_NOTEBOOK (notebook),
              gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook))));
      gtk_widget_size_request (tab, &tDimensions);
      Dimensions.height += tDimensions.height;
    }

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwin),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_show (scrolledwin);
  return Dimensions;
}

/*
 * Reload the entire GUI
 */
static void
reload_gui (void)
{
#define FREECTL(x) do { \
                     for (i=0; i < MAX_DEVS; i++) \
                       { \
                         for (p = x[i]; p != NULL;) \
                           { \
                             nextp = p->next; \
                             g_free (p); \
                             p = nextp; \
                           } \
                         x[i] = NULL; \
                       } \
                   } while (0)

  ctlrec_t * p, * nextp;
  int i;

  remove_timeout ((gpointer)poll_tag_list);
  fully_started = 0;
  FREECTL (control_list); FREECTL (peak_list);
  FREECTL (check_list); FREECTL (value_poll_list);
  for (i=0; i < MAX_DEVS; i++)
    {
      root[i] = NULL;
      g_free (extrec[i]);
      extrec[i] = NULL;
      if (local_fd[i] != -1)
        {
          close (local_fd[i]);
          local_fd[i] = -1;
        }
    }

  gtk_widget_destroy (scrolledwin);
  create_widgets ();
  fully_started = 1;
  add_timeout ((gpointer)poll_tag_list);
#undef FREECTL
}

/*
 * The update_label() routine is used to update the values of certain
 * read only controls.
 */

static void
update_label (oss_mixext * mixext, GtkWidget * wid, int val)
{
  char tmp[100];

  if (mixext->type == MIXT_HEXVALUE)
    snprintf (tmp, sizeof(tmp), "[%s: 0x%x] ",
              gtk_widget_get_name (wid), val);
  else
    snprintf (tmp, sizeof(tmp), "[%s: %d] ",
              gtk_widget_get_name (wid), val);

  if (mixext->flags & MIXF_HZ)
    {
      if (val > 1000000)
	{
	  snprintf (tmp, sizeof(tmp), "[%s: %d.%03d MHz] ",
		    gtk_widget_get_name (wid), val / 1000000,
		    (val / 1000) % 1000);
	}
      else if (val > 1000)
	{
	  snprintf (tmp, sizeof(tmp), "[%s: %d.%03d kHz] ",
		    gtk_widget_get_name (wid), val / 1000, val % 1000);
	}
      else
	snprintf (tmp, sizeof(tmp), "[%s: %d Hz] ",
		  gtk_widget_get_name (wid), val);
    }
  else if (mixext->flags & MIXF_OKFAIL)
    {
      if (val != 0)
	snprintf (tmp, sizeof(tmp), "[%s: Ok] ",
		  gtk_widget_get_name (wid));
      else
	snprintf (tmp, sizeof(tmp), "[%s: Fail] ",
		  gtk_widget_get_name (wid));
    }
  gtk_label_set (GTK_LABEL (wid), tmp);
}

/*
 * The do_update() routine reads a value of certain mixer control
 * and updates the on-screen value depending on the type of the control.
 */

static void
do_update (ctlrec_t * srec)
{
  int val, mx, left, right, vol, angle;
  int mask = 0xff, shift = 8;

  val = get_value (srec->mixext);
  if (val == -1) return;

  if (srec->mixext->type == MIXT_MONOSLIDER16
      || srec->mixext->type == MIXT_STEREOSLIDER16)
    {
      mask = 0xffff;
      shift = 16;
    }

  switch (srec->mixext->type)
    {
    case MIXT_ONOFF:
#ifdef MIXT_MUTE
    case MIXT_MUTE:
#endif /* MIXT_MUTE */
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (srec->gang), val);
      break;

    case MIXT_ENUM:
      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO (srec->gang)->entry),
			  showenum (srec->mixext, val));
      break;

    case MIXT_VALUE:
    case MIXT_HEXVALUE:
      update_label (srec->mixext, (srec->gang), val);
      break;

    case MIXT_SLIDER:
      mx = srec->mixext->maxvalue;
      val = mx - val;
      gtk_adjustment_set_value (GTK_ADJUSTMENT (srec->left), val);
      break;

    case MIXT_MONOSLIDER:
    case MIXT_MONOSLIDER16:
      mx = srec->mixext->maxvalue;
      val = mx - (val & mask);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (srec->left), val);
      break;

    case MIXT_STEREOSLIDER:
    case MIXT_STEREOSLIDER16:
      mx = srec->mixext->maxvalue;
      left = mx - (val & mask);
      right = mx - ((val >> shift) & mask);
      if (srec->gang != NULL)
	if (left != right)
	  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (srec->gang), 0);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (srec->left), left);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (srec->right), right);
      break;

    case MIXT_3D:
#ifdef TEST_JOY
      if (srec->gang != NULL)
	gtk_joy_set_level (GTK_JOY (srec->gang), val);
#else
      vol = 100 - (val & 0x00ff);
      angle = 360 - ((val >> 16) & 0xffff);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (srec->left), vol);
      gtk_adjustment_set_value (GTK_ADJUSTMENT (srec->right), angle);
#endif
      break;
    }
}

/*
 * The poll_all() routine get's called reqularily. It checks the 
 * modify counter for the mixer by calling {!nlink SNDCTL_MIXERINFO}.
 * It checks if some other mixer program has made changes to the settings
 * by comparing the modify counter against the "expected" value.
 *
 * If the mixer was changed then all the controls will be reloaded and updated.
 */

/*ARGSUSED*/
static gint
poll_all (gpointer data)
{
  ctlrec_t *srec;
  oss_audioinfo ainfo;
  char new_label[FRAME_NAME_LENGTH+1] = "";
  int status_changed = 0;
  oss_mixerinfo inf;

  inf.dev = dev;
  if (ioctl (global_fd, SNDCTL_MIXERINFO, &inf) == -1)
    {
      perror ("SNDCTL_MIXERINFO");
      exit (-1);
    }
  if (!inf.enabled)
    {
      reload_gui ();
      return TRUE;
    }

/*
 * Compare the modify counter.
 */
  if ((inf.modify_counter - prev_update_counter[dev]) > set_counter[dev])
    status_changed = 1;
  prev_update_counter[dev] = inf.modify_counter;
  set_counter[dev] = 0;

  srec = check_list[dev];

  while (srec != NULL)
    {
      switch (srec->what_to_do)
	{
	case WHAT_LABEL:
/*
 * Names of certain mixer controls depend on the application that is using
 * the associated audio device. Handling for this is here
 */
	  ainfo.dev = srec->parm;
	  if (ioctl (global_fd, SNDCTL_ENGINEINFO, &ainfo) == -1)
	    {
	      perror ("SNDCTL_ENGINEINFO");
	      continue;
	    }
	  if (*ainfo.label != '\0')
	    {
	      strncpy (new_label, ainfo.label, FRAME_NAME_LENGTH);
	      new_label[FRAME_NAME_LENGTH] = '\0';
	    }
	  else
	    {
	      snprintf (new_label, FRAME_NAME_LENGTH, "pcm%d", srec->parm);
	    }
	  if ((srec->frame != NULL) &&
	      (strncmp (srec->frame_name, new_label, FRAME_NAME_LENGTH)))
	    {
	      strcpy (srec->frame_name, new_label);
     	      gtk_frame_set_label (GTK_FRAME (srec->frame), new_label);
	    }
	  break;
	case WHAT_VMIX:
/*
 * The aforementioned mixer controls can be create dynamically, so ossxmix
 * needs to poll for this. Handling for this is here
 */
	  if (inf.nrext != srec->parm)
	    {
	      srec->parm = inf.nrext;
/*
 * Since we know the added controls are vmix controls, we should be able to do
 * something more graceful here, like reloading only the current device, or
 * even adding the controls directly. This will do for now.
 */
	      reload_gui ();
	      return TRUE;
	    }
	  break;
	case WHAT_UPDATE:
	  if (status_changed)
	    do_update (srec);
	  break;
	}
      srec = srec->next;
    }
  return TRUE;
}

/*
 * The poll_peaks() routine gets called several times per second to update the
 * VU/peak meter LED bar widgets.
 */

/*ARGSUSED*/
static gint
poll_peaks (gpointer data)
{
  ctlrec_t *srec;
  int val, left, right;

  srec = peak_list[dev];

  while (srec != NULL)
    {
      val = get_value (srec->mixext);
      if (val == -1) return TRUE;

      left = val & 0xff;
      right = (val >> 8) & 0xff;

      if (left > srec->last_left)
	srec->last_left = left;

      if (right > srec->last_right)
	srec->last_right = right;

      left = srec->last_left;
      right = srec->last_right;

      /*      gtk_adjustment_set_value(GTK_ADJUSTMENT(srec->left), left);
         gtk_adjustment_set_value(GTK_ADJUSTMENT(srec->right), right); */
      gtk_vu_set_level (GTK_VU (srec->left),
			(left * 8) / srec->mixext->maxvalue);

      if (srec->right != NULL)
	gtk_vu_set_level (GTK_VU (srec->right),
			  (right * 8) / srec->mixext->maxvalue);


      if (srec->last_left > 0)
	srec->last_left -= PEAK_DECAY;
      if (srec->last_right > 0)
	srec->last_right -= PEAK_DECAY;

      srec = srec->next;
    }

  return TRUE;
}

/*ARGSUSED*/
static gint
poll_values (gpointer data)
{
  ctlrec_t *srec;
  int val;

  srec = value_poll_list[dev];

  while (srec != NULL)
    {
      val = get_value (srec->mixext);
      if (val == -1) return TRUE;

      update_label (srec->mixext, GTK_WIDGET (srec->left), val);

      srec = srec->next;
    }

  return TRUE;
}

/*ARGSUSED*/
static gint
poll_mixnum (gpointer data)
{
  int c_mixer_num;

  if (ioctl (global_fd, SNDCTL_MIX_NRMIX, &c_mixer_num) == -1)
    {
      perror ("SNDCTL_MIX_NRMIX");
      exit (-1);
    }

  if (c_mixer_num > MAX_DEVS) c_mixer_num = MAX_DEVS;
  if (c_mixer_num != mixer_num) reload_gui ();

  return TRUE;
}

static int
find_default_mixer (void)
{
  oss_mixerinfo mi;
  int i, best = -1, bestpri = 0, mix_num;

  if (ioctl (global_fd, SNDCTL_MIX_NRMIX, &mix_num) == -1)
    {
      perror ("SNDCTL_MIX_NRMIX");
      if (errno == EINVAL)
	fprintf (stderr, "Error: OSS version 4.0 or later is required\n");
      exit (-1);
    }

  if (mix_num == 0)
    {
      fprintf (stderr, "No mixers are available\n");
      exit (-1);
    }

  for (i = 0; i < mix_num; i++)
    {
      mi.dev = i;

      if (ioctl (global_fd, SNDCTL_MIXERINFO, &mi) == -1)
	continue;		/* Ignore errors */

      if (mi.enabled)
        {
          if (best == -1) best = i;

          if (mi.priority > bestpri)
            {
              best = i;
              bestpri = mi.priority;
            }
        }
    }

  if (best == -1)
    {
      fprintf (stderr, "No mixers are available for use as a default mixer\n");
      exit (-1);
    }

  return best;
}

static void
parse_dimarg (const char * dimarg, GtkRequisition * Dimensions)
{
  long height = 0, width = 0;
  char * p;

  errno = 0;
  width = strtol (dimarg, &p, 10);
  if (errno || (width <= 0)) return;
  if (width > Dimensions->width) width = Dimensions->width;
  height = width;
  if (*p != '\0')
    {
      errno = 0;
      height = strtol (p+1, NULL, 10);
      if (errno || (height <= 0)) height = width;
    }

  Dimensions->width = width;
  if (height < Dimensions->height) Dimensions->height = height;
  return;
}

int
main (int argc, char **argv)
{
  extern char * optarg;
  char * dimarg = NULL;
  int i, v, c;
  GtkRequisition Dimensions;
#ifndef GTK1_ONLY
  GdkPixbuf *icon_pix;
#else
  GdkPixmap *icon_pix;
  GdkBitmap *icon_mask;
#endif /* !GTK1_ONLY */

  const char *devmixer;
  oss_sysinfo si;

  for (i=0; i< MAX_DEVS; i++)
      local_fd[i] = -1; /* Not opened */

  if ((devmixer=getenv("OSS_MIXERDEV"))==NULL)
     devmixer = "/dev/mixer";

#if !defined(GTK1_ONLY) && defined(DEBUG)
  g_mem_set_vtable (glib_mem_profiler_table);
#endif
  /* Get Gtk to process the startup arguments */
  gtk_init (&argc, &argv);

  while ((c = getopt (argc, argv, "Sbd:g:hn:w:x")) != EOF)
      switch (c)
	{
	case 'd':
	  dev = atoi (optarg);
	  load_all_devs = 0;
	  break;

	case 'w':
	  v = 0;
	  v = atoi (optarg);
	  if (v <= 0)
	    v = 1;
	  width_adjust += v;
	  break;

	case 'n':
	  v = 0;
	  v = atoi (optarg);
	  if (v <= 0)
	    v = 1;
	  width_adjust -= v;
	  break;

	case 'x':
	  show_all = 0;
	  break;

	case 'b':
	  background = 1;
	  break;

	case 'S':
	  show_status_icon = 0;
	  break;

	case 'g':
	  dimarg = optarg;
	  break;

	case 'h':
	  printf ("Usage: %s [options...]\n", argv[0]);
	  printf ("       -h          Prints help (this screen)\n");
	  printf ("       -d<dev#>    Selects the mixer device\n");
	  printf ("       -x          Hides the \"legacy\" mixer controls\n");
	  printf ("       -w[val]     Make mixer bit wider on screen\n");
	  printf ("       -n[val]     Make mixer bit narrower on screen\n");
	  printf ("       -b          Start mixer in background\n");
	  printf ("       -g[w:h]     Start mixer window with w:h size\n");
#ifdef STATUSICON
	  printf ("       -S          Don't place an icon in system tray\n");
#endif /* STATUSICON */
	  exit (0);
	  break;
	}

  if (width_adjust < -2)
    width_adjust = -2;
  if (width_adjust > 4)
    width_adjust = 4;

  if ((global_fd = open (devmixer, O_RDWR, 0)) == -1)
    {
      perror (devmixer);
      exit (-1);
    }

  atexit (cleanup);

  if (ioctl(global_fd, SNDCTL_SYSINFO, &si) != -1)
     if (si.versionnum == 0x040003) /* Boomer 4.0 */
	boomer_workaround = 1;

  if (dev == -1)
    dev = find_default_mixer ();

  v = chdir ("/"); /* We don't really care if this fails */

  /* Create the app's main window */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  Dimensions = create_widgets ();
  if (dimarg != NULL) parse_dimarg (dimarg, &Dimensions);
  gtk_window_set_default_size (GTK_WINDOW (window),
                               Dimensions.width, Dimensions.height);

  fully_started = 1;

  /* Connect a window's signal to a signal function */
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC (close_request), NULL);

#ifndef GTK1_ONLY
  icon_pix = gdk_pixbuf_new_from_xpm_data ((const char **)ossxmix);
#ifdef STATUSICON
  if (show_status_icon)
    {
      char tmp[100];

      status_icon = gtk_status_icon_new_from_pixbuf (icon_pix);
      snprintf (tmp, sizeof(tmp), "ossxmix - device %d / %s",
                dev, root[dev]->name);
      gtk_status_icon_set_tooltip (status_icon, tmp);
      g_signal_connect (G_OBJECT (status_icon), "popup-menu",
                        G_CALLBACK (trayicon_popupmenu), NULL);
      g_signal_connect (G_OBJECT (status_icon), "activate",
                        G_CALLBACK (activate_mainwindow), NULL);
    }
#endif /* STATUSICON */
  gtk_window_set_icon (GTK_WINDOW (window), icon_pix);

  g_signal_connect (G_OBJECT (window), "window-state-event",
                    G_CALLBACK (manage_timeouts), (gpointer)poll_tag_list);
  if ((!background) || (!show_status_icon))
    {
      gtk_widget_show (window);
      if (background) gtk_window_iconify (GTK_WINDOW (window));
    }
#if GTK_CHECK_VERSION(2,2,0)
  else gdk_notify_startup_complete ();
#endif
#else
  add_timeout ((gpointer)poll_tag_list);
  gtk_widget_show (window);
  if (background) XIconifyWindow (GDK_WINDOW_XDISPLAY (window->window),
				  GDK_WINDOW_XWINDOW (window->window),
				  DefaultScreen (GDK_DISPLAY ()));
  icon_pix = gdk_pixmap_create_from_xpm_d (window->window, &icon_mask,
                                          &window->style->bg[GTK_STATE_NORMAL],
                                          (gchar **)ossxmix);
  gdk_window_set_icon (window->window, NULL, icon_pix, icon_mask);
#endif /* !GTK1_ONLY */

  gtk_main ();

  return 0;
}

/*
 * Function to run when program is shutting down
 */
static void
cleanup (void)
{
  int i;

  close (global_fd);

  for (i=0;i<MAX_DEVS;i++)
      if (local_fd[i] != -1)
	 close (local_fd[i]);

#if !defined(GTK1_ONLY) && GTK_CHECK_VERSION(2,2,0)
  gdk_notify_startup_complete ();
#endif /* !GTK1_ONLY */

#ifdef DEBUG
  g_mem_profile ();
#endif
}

/*
 * Function to handle a close signal on the window
 */
/*ARGSUSED*/
static gint
close_request (GtkWidget * theWindow, gpointer data)
{
#ifdef STATUSICON
  if (show_status_icon && (gtk_status_icon_is_embedded (status_icon) == TRUE))
    {
      gtk_widget_hide (window);
      return TRUE;
    }
#endif
  gtk_main_quit ();
  return FALSE;
}

/*
 * Function to make sure only the currently shown mixer is polled
 */
/*ARGSUSED*/
static void
switch_page (GtkNotebook * notebook, GtkNotebookPage * page,
             guint page_num, gpointer data)
{
#ifdef STATUSICON
  char tmp[100];

  if ((show_status_icon) &&
      (gtk_status_icon_is_embedded (status_icon) == TRUE))
    {
      snprintf (tmp, sizeof(tmp), "ossxmix - device %d / %s",
                page_num, root[page_num]->name);
      gtk_status_icon_set_tooltip (status_icon, tmp);
    }
#endif /* STATUSICON */

  /*
   * GTK1 calls switch_page when scrolledwin is destroyed in reload_gui.
   * This is merely annoying, but this check prevents it nonetheless.
   */
  if (fully_started == 0) return;
  remove_timeout (data);
  dev = page_num;
  add_timeout (data);
}

/*
 * Function to start polling mixer 'dev'
 */
/*ARGSUSED*/
static gint
add_timeout (gpointer data)
{
  guint *poll_tag_list = (guint *) data;

  if ((peak_list[dev] != NULL) && (poll_tag_list[0] == 0))
    poll_tag_list[0] = g_timeout_add (PEAK_POLL_INTERVAL, poll_peaks, NULL);
  if ((value_poll_list[dev] != NULL) && (poll_tag_list[1] == 0))
    poll_tag_list[1] = g_timeout_add (VALUE_POLL_INTERVAL, poll_values, NULL);
  if (poll_tag_list[2] == 0)
    poll_tag_list[2] = g_timeout_add (MIXER_POLL_INTERVAL, poll_all, NULL);
  if ((poll_tag_list[3] == 0) && (load_all_devs))
    poll_tag_list[3] = g_timeout_add (MIXNUM_POLL_INTERVAL, poll_mixnum, NULL);
  return FALSE;
}

/*
 * Function to stop polling mixer 'dev'
 */
/*ARGSUSED*/
static gint
remove_timeout (gpointer data)
{
  guint *poll_tag_list = (guint *) data;
 
  if (poll_tag_list[0] != 0)
    {
      g_source_remove (poll_tag_list[0]);
      poll_tag_list[0] = 0;
    }
  if (poll_tag_list[1] != 0)
    {
      g_source_remove (poll_tag_list[1]);
      poll_tag_list[1] = 0;
    }
  if (poll_tag_list[2] != 0)
    {
      g_source_remove (poll_tag_list[2]);
      poll_tag_list[2] = 0;
    }
  if (poll_tag_list[3] != 0)
    {
      g_source_remove (poll_tag_list[3]);
      poll_tag_list[3] = 0;
    }
  return FALSE;
}

#ifndef GTK1_ONLY
/*
 * Function to make sure polling isn't done when window is minimized or hidden
 */
/*ARGSUSED*/
static gint
manage_timeouts (GtkWidget * w, GdkEventWindowState * e, gpointer data)
{
  if (e->new_window_state &
      (GDK_WINDOW_STATE_ICONIFIED | GDK_WINDOW_STATE_WITHDRAWN))
    {
      remove_timeout (data);
      return FALSE;
    }
  add_timeout (data);
  return FALSE;
}
#endif /* !GTK1_ONLY */

#ifdef STATUSICON
/*ARGSUSED*/
static void
activate_mainwindow (GtkStatusIcon * icon, guint button, guint etime,
		     gpointer data)
{
  if (GTK_WIDGET_VISIBLE (window)) gtk_widget_hide (window);
  else popup_mainwindow (NULL, NULL);
}

/*ARGSUSED*/
static void
popup_mainwindow (GtkWidget * w, gpointer data)
{
  gtk_widget_show (window);
  gtk_window_present (GTK_WINDOW (window));
}

/*
 * Popup menu when clicking on status icon
 */
/*ARGSUSED*/
static void
trayicon_popupmenu (GtkStatusIcon * icon, guint button, guint etime,
		    gpointer data)
{
  static GtkWidget *tray_menu = NULL;

  if (tray_menu == NULL)
    {
      GtkWidget *item;
      tray_menu = gtk_menu_new ();

      item = gtk_menu_item_new_with_label ("Restore");
      g_signal_connect (G_OBJECT (item), "activate",
                        G_CALLBACK (popup_mainwindow), NULL);
      gtk_menu_append (tray_menu, item);
      item = gtk_menu_item_new_with_label ("Quit");
      gtk_menu_append (tray_menu, item);
      g_signal_connect (G_OBJECT (item), "activate",
                        G_CALLBACK (gtk_main_quit), NULL);
    }

  gtk_widget_show_all (tray_menu);

  gtk_menu_popup (GTK_MENU (tray_menu), NULL, NULL,
                  gtk_status_icon_position_menu, status_icon,
                  button, etime);
}
#endif /* STATUSICON */

