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

#include <stdio.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>

#include "gtkvu.h"

extern int width_adjust;

#define SCROLL_DELAY_LENGTH  300
#define VU_DEFAULT_SIZE 100
#define VU_MARGIN	(widget->allocation.width/3)

/* Forward declarations */

static void gtk_vu_class_init (GtkVUClass * klass);
static void gtk_vu_init (GtkVU * vu);
static void gtk_vu_destroy (GtkObject * object);
static void gtk_vu_realize (GtkWidget * widget);
static void gtk_vu_unrealize (GtkWidget * widget);
static void gtk_vu_size_request (GtkWidget * widget,
				 GtkRequisition * requisition);
static void gtk_vu_size_allocate (GtkWidget * widget,
				  GtkAllocation * allocation);
static gint gtk_vu_expose (GtkWidget * widget, GdkEventExpose * event);

/* Local data */

static GtkWidgetClass *parent_class = NULL;

GtkType
gtk_vu_get_type (void)
{
  static GtkType vu_type = 0;

  if (!vu_type)
    {
      GtkTypeInfo vu_info = {
	"GtkVU",
	sizeof (GtkVU),
	sizeof (GtkVUClass),
	(GtkClassInitFunc) gtk_vu_class_init,
	(GtkObjectInitFunc) gtk_vu_init,
	NULL,
	NULL,
      };

      vu_type = gtk_type_unique (gtk_widget_get_type (), &vu_info);
    }

  return vu_type;
}

static void
gtk_vu_class_init (GtkVUClass * gvclass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass *) gvclass;
  widget_class = (GtkWidgetClass *) gvclass;

  parent_class = GTK_WIDGET_CLASS (gtk_type_class (gtk_widget_get_type ()));

  object_class->destroy = gtk_vu_destroy;

  widget_class->realize = gtk_vu_realize;
  widget_class->unrealize = gtk_vu_unrealize;
  widget_class->expose_event = gtk_vu_expose;
  widget_class->size_request = gtk_vu_size_request;
  widget_class->size_allocate = gtk_vu_size_allocate;
}

static void
gtk_vu_init (GtkVU * vu)
{
  vu->level = 0;
}

GtkWidget *
gtk_vu_new (void)
{
  GtkVU *vu;

  vu = GTK_VU (gtk_type_new (gtk_vu_get_type ()));

  return GTK_WIDGET (vu);
}

static void
gtk_vu_destroy (GtkObject * object)
{
  /* GtkVU *vu; */

  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK_IS_VU (object));

  /* vu = GTK_VU (object); */

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
gtk_vu_realize (GtkWidget * widget)
{
  GtkVU *vu;
  GdkWindowAttr attributes;
  gint attributes_mask;
  gboolean alloc_success[7];

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_VU (widget));

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
  vu = GTK_VU (widget);

  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window =
    gdk_window_new (widget->parent->window, &attributes, attributes_mask);

  widget->style = gtk_style_attach (widget->style, widget->window);

  gdk_window_set_user_data (widget->window, widget);

  /* Dark green */

  vu->colors[0].red = 0x0000;
  vu->colors[0].green = 0x30FF;
  vu->colors[0].blue = 0x0000;

  /* Green */

  vu->colors[1].red = 0x0000;
  vu->colors[1].green = 0xBFFF;
  vu->colors[1].blue = 0x0000;

  /* Dark Orange */

  vu->colors[2].red = 0x30FF;
  vu->colors[2].green = 0x30FF;
  vu->colors[2].blue = 0x0000;

  /* Orange */

  vu->colors[3].red = 0xBFFF;
  vu->colors[3].green = 0xBFFF;
  vu->colors[3].blue = 0x0000;

  /* Dark Red */

  vu->colors[4].red = 0x30FF;
  vu->colors[4].green = 0x0000;
  vu->colors[4].blue = 0x0000;

  /* Red */

  vu->colors[5].red = 0xBFFF;
  vu->colors[5].green = 0x0000;
  vu->colors[5].blue = 0x0000;

  /* Black */

  vu->colors[6].red = 0x0000;
  vu->colors[6].green = 0x0000;
  vu->colors[6].blue = 0x0000;

  gdk_colormap_alloc_colors (gtk_widget_get_colormap (widget), vu->colors, 7,
			     FALSE, TRUE, alloc_success);
  vu->gc = gdk_gc_new (widget->window);
  vu->pixmap =
    gdk_pixmap_new (widget->window, widget->allocation.width,
		    widget->allocation.height, -1);

  /*  gtk_style_set_background (widget->style, widget->window, GTK_STATE_ACTIVE); */
}

static void
gtk_vu_unrealize (GtkWidget * widget)
{
  GtkVU *vu;
  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_VU (widget));

  GTK_WIDGET_UNSET_FLAGS (widget, GTK_REALIZED);
  vu = GTK_VU (widget);

  gdk_colormap_free_colors (gtk_widget_get_colormap (widget), vu->colors, 7);
  gdk_pixmap_unref (vu->pixmap);
  gdk_gc_unref (vu->gc);
  gdk_window_unref (widget->window);
}

/*ARGSUSED*/
static void
gtk_vu_size_request (GtkWidget * widget, GtkRequisition * requisition)
{
  if (width_adjust <= 0)
    requisition->width = 20;
  else
    requisition->width = 28;
  requisition->height = 85;
}

static void
gtk_vu_size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{
  GtkVU *vu;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_VU (widget));
  g_return_if_fail (allocation != NULL);

  widget->allocation = *allocation;
  vu = GTK_VU (widget);

  if (GTK_WIDGET_REALIZED (widget))
    {

      gdk_window_move_resize (widget->window,
			      allocation->x, allocation->y,
			      allocation->width, allocation->height);
      gdk_pixmap_unref (vu->pixmap);
      vu->pixmap =
	gdk_pixmap_new (widget->window, widget->allocation.width,
			widget->allocation.height,
			gdk_window_get_visual (widget->window)->depth);
    }
}

static gint
gtk_vu_expose (GtkWidget * widget, GdkEventExpose * event)
{
  GtkVU *vu;
  guint i, y_size;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_VU (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (event->count > 0)
    return FALSE;

  vu = GTK_VU (widget);

  gdk_gc_set_foreground (vu->gc, &vu->colors[6]);
  gdk_draw_rectangle (vu->pixmap, vu->gc, TRUE, 0, 0,
		      widget->allocation.width, widget->allocation.height);
  /*  gdk_window_clear_area (vu->pixmap,
     0, 0,
     widget->allocation.width,
     widget->allocation.height); */

  y_size = (widget->allocation.height - 50) / 8;

  for (i = 0; i < 8; i++)
    {
      gdk_gc_set_foreground (vu->gc,
			     &vu->colors[((7 - i) / 3) * 2 +
					 (((8 - vu->level) > i) ? 0 : 1)]);
      gdk_draw_rectangle (vu->pixmap, vu->gc, TRUE, VU_MARGIN,
			  (i * (y_size + 5)) + 10,
			  widget->allocation.width - VU_MARGIN * 2, y_size);
    }

  gdk_draw_pixmap (widget->window, vu->gc, vu->pixmap, 0, 0, 0, 0,
		   widget->allocation.width, widget->allocation.height);
  return FALSE;
}

void
gtk_vu_set_level (GtkVU * vu, guint new_level)
{
  if (new_level != vu->level)
    {
      vu->level = new_level;
      gtk_widget_queue_draw (GTK_WIDGET (vu));
    }
}
