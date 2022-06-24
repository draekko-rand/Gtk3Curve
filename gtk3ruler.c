// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Customized ruler class for inkscape.  Note that this is a fork of
 * the GimpRuler widget from GIMP: libgimpwidgets/gimpruler.c.
 * The GIMP code was released under the LGPL3+ (which is GPL3 plus extra permissions that may be ignored).
 * The GIMP code itself is a fork of the now-obsolete GtkRuler widget from GTK+ 2.
 *
 * Major differences between implementations in Inkscape and GIMP are
 * as follows:
 *  - We use a 1,2,4,8... scale for inches and 1,2,5,10... for everything
 *    else.  GIMP uses 1,2,5,10... for everything.
 *
 *  - We use a default font size of PANGO_SCALE_X_SMALL for labels,
 *    GIMP uses PANGO_SCALE_SMALL (i.e., a bit larger than ours).
 *
 *  - We abbreviate large numbers in tick-labels (e.g., 10000 -> 10k)
 *
 *  - GtkStateFlags are read from GtkStyleContext objects where appropriate
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *   bulia byak <buliabyak@users.sf.net>
 *   Diederik van Lierop <mail@diedenrezi.nl>
 *   Jon A. Cruz <jon@joncruz.org>
 *   Alex Valavanis <valavanisalex@gmail.com>
 *
 * Copyright (C) 1999-2011 authors
 *
 * Released under GNU GPL v3+. Read the file 'COPYING' for more information.
 */

#include <string.h>
#include <math.h>
#include <stdio.h>

#include "gtk3ruler.h"

#define ROUND(x) ((int) round(x))

#define GTK_PARAM_READWRITE G_PARAM_READWRITE|G_PARAM_STATIC_NAME|G_PARAM_STATIC_NICK|G_PARAM_STATIC_BLURB

#define DEFAULT_RULER_FONT_SCALE    PANGO_SCALE_X_SMALL
#define MINIMUM_INCR                5
#define IMMEDIATE_REDRAW_THRESHOLD  20

enum {
  PROP_0,
  PROP_ORIENTATION,
  PROP_UNIT,
  PROP_LOWER,
  PROP_UPPER,
  PROP_POSITION,
  PROP_MAX_SIZE
};

typedef struct _Gtk3RulerMetric Gtk3RulerMetric;

/* All distances below are in 1/72nd's of an inch. (According to
 * Adobe, that's a point, but points are really 1/72.27 in.)
 */
struct _Gtk3RulerPrivate
{
  GtkOrientation   orientation;
  Gtk3RulerMetricUnit unit;
  gdouble          lower;
  gdouble          upper;
  gdouble          position;
  gdouble          max_size;

  GdkWindow       *input_window;
  cairo_surface_t *backing_store;
  gboolean         backing_store_valid;
  GdkRectangle     last_pos_rect;
  guint            pos_redraw_idle_id;
  PangoLayout     *layout;
  gdouble          font_scale;

  GList *track_widgets;
};

#define GTK3_RULER_GET_PRIVATE(ruler) \
  GTK3_RULER (ruler)->priv



struct _Gtk3RulerMetric
{
  gdouble ruler_scale[16];
  gint    subdivide[5];
};

Gtk3RulerMetric ruler_metric;

// Ruler metric for general use.
static const Gtk3RulerMetric ruler_metric_general = {
  { 1, 2, 5, 10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000, 25000, 50000, 100000 },
  { 1, 5, 10, 50, 100 }
};

// Ruler metric for inch scales.
static const Gtk3RulerMetric ruler_metric_inches = {
  { 1, 2, 4,  8, 16, 32,  64, 128, 256,  512, 1024, 2048, 4096, 8192, 16384, 32768 },
  { 1, 2,  4,  8,  16 }
};
#if 0
static const Gtk3RulerMetric ruler_metric_feet =
{
  /* 3 feet = 1 yard; 6 feet = 1 fathom */
  { 1, 3, 6, 12, 36, 72, 100, 250, 500, 1000, 2500, 5000, 10000, 25000, 50000, 100000 },

  /* 1 foot = 12 inches, so let's divide up to 12, */
  { 1, 3, 6, 12,
  /* then divide the inch by 2. */
    24 }
};

static const Gtk3RulerMetric ruler_metric_yards =
{
  /* 1 fathom = 2 yards. Should we go back to base-10 digits? */
  { 1, 2, 5, 10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000, 25000, 50000, 100000 },

  /* 1 yard = 3 feet = 36 inches. */
  { 1, 3, 6, 12, 36 }
};
#endif

static void          gtk3_ruler_dispose              (GObject        *object);
static void          gtk3_ruler_set_property         (GObject        *object,
                                                    guint           prop_id,
                                                    const GValue   *value,
                                                    GParamSpec     *pspec);
static void          gtk3_ruler_get_property         (GObject        *object,
                                                    guint           prop_id,
                                                    GValue         *value,
                                                    GParamSpec     *pspec);

static void          gtk3_ruler_realize              (GtkWidget      *widget);
static void          gtk3_ruler_unrealize            (GtkWidget      *widget);
static void          gtk3_ruler_map                  (GtkWidget      *widget);
static void          gtk3_ruler_unmap                (GtkWidget      *widget);
static void          gtk3_ruler_size_allocate        (GtkWidget      *widget,
                                                    GtkAllocation  *allocation);

static void          gtk3_ruler_get_preferred_width  (GtkWidget      *widget, 
                                                    gint           *minimum_width,
                                                    gint           *natural_width);

static void          gtk3_ruler_get_preferred_height (GtkWidget      *widget, 
                                                    gint           *minimum_height,
                                                    gint           *natural_height);
static void          gtk3_ruler_style_updated        (GtkWidget      *widget);

static gboolean      gtk3_ruler_motion_notify        (GtkWidget      *widget,
                                                    GdkEventMotion *event);
static gboolean      gtk3_ruler_draw                 (GtkWidget      *widget,
                                                    cairo_t        *cr);
static void          gtk3_ruler_draw_ticks           (Gtk3Ruler        *ruler);
static GdkRectangle  gtk3_ruler_get_pos_rect         (Gtk3Ruler        *ruler,
                                                    gdouble         position);
static gboolean      gtk3_ruler_idle_queue_pos_redraw(gpointer        data);
static void          gtk3_ruler_queue_pos_redraw     (Gtk3Ruler        *ruler);
static void          gtk3_ruler_draw_pos             (Gtk3Ruler        *ruler,
                                                    cairo_t        *cr);
static void          gtk3_ruler_make_pixmap          (Gtk3Ruler        *ruler);

static PangoLayout * gtk3_ruler_get_layout           (GtkWidget      *widget,
                                                    const gchar    *text);


G_DEFINE_TYPE_WITH_PRIVATE (Gtk3Ruler, gtk3_ruler, GTK_TYPE_WIDGET)

#define parent_class gtk3_ruler_parent_class


static void
gtk3_ruler_class_init (Gtk3RulerClass *klass)
{
  GObjectClass   *object_class  = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class  = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_css_name (widget_class, "ruler-widget");

  object_class->dispose              = gtk3_ruler_dispose;
  object_class->set_property         = gtk3_ruler_set_property;
  object_class->get_property         = gtk3_ruler_get_property;

  widget_class->realize              = gtk3_ruler_realize;
  widget_class->unrealize            = gtk3_ruler_unrealize;
  widget_class->map                  = gtk3_ruler_map;
  widget_class->unmap                = gtk3_ruler_unmap;
  widget_class->size_allocate        = gtk3_ruler_size_allocate;
  widget_class->get_preferred_width  = gtk3_ruler_get_preferred_width;
  widget_class->get_preferred_height = gtk3_ruler_get_preferred_height;
  widget_class->style_updated        = gtk3_ruler_style_updated;
  widget_class->draw                 = gtk3_ruler_draw;
  widget_class->motion_notify_event = gtk3_ruler_motion_notify;

  g_object_class_install_property (object_class,
                                   PROP_ORIENTATION,
				   g_param_spec_enum ("orientation",
					              ("Orientation"),
						      ("The orientation of the ruler"),
                                                      GTK_TYPE_ORIENTATION,
						      GTK_ORIENTATION_HORIZONTAL,
						      GTK_PARAM_READWRITE));

  /* FIXME: Should probably use g_param_spec_enum */
  g_object_class_install_property (object_class,
                                   PROP_UNIT,
                                   g_param_spec_string ("unit",
						      ("Unit"),
						      ("Unit of the ruler"),
						      "px",
						      GTK_PARAM_READWRITE));
  
  g_object_class_install_property (object_class,
                                   PROP_LOWER,
                                   g_param_spec_double ("lower",
							("Lower"),
							("Lower limit of ruler"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE,
							0.0,
							GTK_PARAM_READWRITE));  

  g_object_class_install_property (object_class,
                                   PROP_UPPER,
                                   g_param_spec_double ("upper",
							("Upper"),
							("Upper limit of ruler"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE,
							0.0,
							GTK_PARAM_READWRITE));  

  g_object_class_install_property (object_class,
                                   PROP_POSITION,
                                   g_param_spec_double ("position",
							("Position"),
							("Position of mark on the ruler"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE,
							0.0,
							GTK_PARAM_READWRITE));  

  g_object_class_install_property (object_class,
                                   PROP_MAX_SIZE,
                                   g_param_spec_double ("max-size",
							("Max Size"),
							("Maximum size of the ruler"),
							-G_MAXDOUBLE,
							G_MAXDOUBLE,
							0.0,
							GTK_PARAM_READWRITE));  

  gtk_widget_class_install_style_property (widget_class,
		                           g_param_spec_double ("font-scale",
						                NULL, NULL,
								0.0,
								G_MAXDOUBLE,
								DEFAULT_RULER_FONT_SCALE,
								G_PARAM_READABLE));
}

static void
gtk3_ruler_init (Gtk3Ruler *ruler)
{
  Gtk3RulerPrivate *priv = gtk3_ruler_get_instance_private (ruler);
  ruler->priv = priv;

  gtk_widget_set_has_window (GTK_WIDGET (ruler), FALSE);

  priv->orientation = GTK_ORIENTATION_HORIZONTAL;
  priv->unit                 = GTK3_RULER_METRIC_INCHES;
  priv->lower                = 0;
  priv->upper                = 0;
  priv->position             = 0;
  priv->max_size             = 0;

  priv->backing_store        = NULL;
  priv->backing_store_valid  = FALSE;

  priv->last_pos_rect.x      = 0;
  priv->last_pos_rect.y      = 0;
  priv->last_pos_rect.width  = 0;
  priv->last_pos_rect.height = 0;
  priv->pos_redraw_idle_id   = 0;

  priv->font_scale           = DEFAULT_RULER_FONT_SCALE;
  priv->track_widgets        = NULL;
}

static void
gtk3_ruler_dispose (GObject *object)
{
    Gtk3Ruler        *ruler = GTK3_RULER(object);
    Gtk3RulerPrivate *priv  = GTK3_RULER_GET_PRIVATE(ruler);

    if (ruler == NULL || priv == NULL || priv->track_widgets == NULL || g_list_length (priv->track_widgets) == 0) { return; }

    while (g_list_length (priv->track_widgets)) {
        gtk3_ruler_remove_track_widget(ruler, g_list_first (priv->track_widgets)->data);
    }

    if (priv->pos_redraw_idle_id) {
        g_source_remove(priv->pos_redraw_idle_id);
        priv->pos_redraw_idle_id = 0;
    }
    priv->track_widgets = NULL;

    G_OBJECT_CLASS (parent_class)->disp	ose(object);
}


/**
 * gtk3_ruler_set_range:
 * @ruler: the Gtk3Ruler
 * @lower: the lower limit of the ruler
 * @upper: the upper limit of the ruler
 * @max_size: the maximum size of the ruler used when calculating the space to
 * leave for the text
 *
 * This sets the range of the ruler. 
 */
void
gtk3_ruler_set_range (Gtk3Ruler *ruler,
		    gdouble   lower,
		    gdouble   upper,
		    gdouble   max_size)
{
  Gtk3RulerPrivate *priv;
  
  g_return_if_fail (GTK3_IS_RULER (ruler));

  priv = GTK3_RULER_GET_PRIVATE (ruler);

  g_object_freeze_notify (G_OBJECT (ruler));
  if (priv->lower != lower)
    {
      priv->lower = lower;
      g_object_notify (G_OBJECT (ruler), "lower");
    }
  if (priv->upper != upper)
    {
      priv->upper = upper;
      g_object_notify (G_OBJECT (ruler), "upper");
    }
  if (priv->max_size != max_size)
    {
      priv->max_size = max_size;
      g_object_notify (G_OBJECT (ruler), "max-size");
    }
  g_object_thaw_notify (G_OBJECT (ruler));

  priv->backing_store_valid = FALSE;
  gtk_widget_queue_draw (GTK_WIDGET (ruler));
}

/**
 * gtk3_ruler_get_range:
 * @ruler: a #Gtk3Ruler
 * @lower: (allow-none): location to store lower limit of the ruler, or %NULL
 * @upper: (allow-none): location to store upper limit of the ruler, or %NULL
 * @max_size: location to store the maximum size of the ruler used when calculating
 *            the space to leave for the text, or %NULL.
 *
 * Retrieves values indicating the range and current position of a #Gtk3Ruler.
 * See gtk3_ruler_set_range().
 **/
void
gtk3_ruler_get_range (Gtk3Ruler *ruler,
		     gdouble  *lower,
		     gdouble  *upper,
		     gdouble  *max_size)
{
  Gtk3RulerPrivate *priv;

  g_return_if_fail (GTK3_IS_RULER (ruler));
  
  priv = GTK3_RULER_GET_PRIVATE (ruler);

  if (lower)
    *lower = priv->lower;
  if (upper)
    *upper = priv->upper;
  if (max_size)
    *max_size = priv->max_size;
}

static void
gtk3_ruler_set_property (GObject      *object,
 			guint         prop_id,
			const GValue *value,
			GParamSpec   *pspec)
{
  Gtk3Ruler *ruler = GTK3_RULER (object);
  Gtk3RulerPrivate *priv = GTK3_RULER_GET_PRIVATE (ruler);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      priv->orientation = (GtkOrientation)g_value_get_enum (value);
      gtk_widget_queue_resize (GTK_WIDGET (ruler));
      break;
    
    case PROP_UNIT:
      gtk3_ruler_set_unit (ruler, (Gtk3RulerMetricUnit)g_value_get_int (value));
      break;

    case PROP_LOWER:
      gtk3_ruler_set_range (ruler,
                          g_value_get_double (value),
                          priv->upper,
			  priv->max_size);
      break;
    case PROP_UPPER:
      gtk3_ruler_set_range (ruler,
                          priv->lower,
                          g_value_get_int (value),
			  priv->max_size);
      break;

    case PROP_POSITION:
      gtk3_ruler_set_position (ruler, g_value_get_double (value));
      break;

    case PROP_MAX_SIZE:
      gtk3_ruler_set_range (ruler,
                          priv->lower,
                          priv->upper,
			  g_value_get_double (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk3_ruler_get_property (GObject      *object,
			guint         prop_id,
			GValue       *value,
			GParamSpec   *pspec)
{
  Gtk3Ruler *ruler = GTK3_RULER (object);
  Gtk3RulerPrivate *priv = GTK3_RULER_GET_PRIVATE (ruler);

  switch (prop_id)
    {
    case PROP_ORIENTATION:
      g_value_set_enum (value, priv->orientation);
      break;
    
    case PROP_UNIT:
      g_value_set_int (value, priv->unit);
      break;
    case PROP_LOWER:
      g_value_set_double (value, priv->lower);
      break;
    case PROP_UPPER:
      g_value_set_double (value, priv->upper);
      break;
    case PROP_POSITION:
      g_value_set_double (value, priv->position);
      break;
    case PROP_MAX_SIZE:
      g_value_set_double (value, priv->max_size);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
gtk3_ruler_realize (GtkWidget *widget)
{
  Gtk3Ruler        *ruler = GTK3_RULER (widget);
  Gtk3RulerPrivate *priv  = GTK3_RULER_GET_PRIVATE (ruler);
  GtkAllocation   allocation;
  GdkWindowAttr   attributes;
  gint            attributes_mask;

  GTK_WIDGET_CLASS (gtk3_ruler_parent_class)->realize (widget);
  
  gtk_widget_get_allocation (widget, &allocation);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x           = allocation.x;
  attributes.y           = allocation.y;
  attributes.width       = allocation.width;
  attributes.height      = allocation.height;
  attributes.wclass      = GDK_INPUT_ONLY;
  attributes.event_mask  = (gtk_widget_get_events (widget) |
                           GDK_EXPOSURE_MASK               |
			   GDK_POINTER_MOTION_MASK         |
			   GDK_POINTER_MOTION_HINT_MASK);

  attributes_mask = GDK_WA_X | GDK_WA_Y;

  priv->input_window = gdk_window_new (gtk_widget_get_parent_window (widget), 
                                       &attributes, attributes_mask);
  gdk_window_set_user_data (priv->input_window, ruler);

  gtk3_ruler_make_pixmap (ruler);
}

static void
gtk3_ruler_unrealize(GtkWidget *widget)
{
  Gtk3Ruler        *ruler = GTK3_RULER (widget);
  Gtk3RulerPrivate *priv  = GTK3_RULER_GET_PRIVATE (ruler);

  if (priv->backing_store)
    {
      cairo_surface_destroy (priv->backing_store);
      priv->backing_store = NULL;
    }

  priv->backing_store_valid = FALSE;

  if (priv->layout)
    {
      g_object_unref (priv->layout);
      priv->layout = NULL;
    }

  if (priv->input_window)
    {
      gdk_window_destroy (priv->input_window);
      priv->input_window = NULL;
    }

  GTK_WIDGET_CLASS (gtk3_ruler_parent_class)->unrealize (widget);
}

static void
gtk3_ruler_map (GtkWidget *widget)
{
  Gtk3Ruler        *ruler = GTK3_RULER (widget);
  Gtk3RulerPrivate *priv  = GTK3_RULER_GET_PRIVATE (ruler);

  GTK_WIDGET_CLASS (gtk3_ruler_parent_class)->map (widget);

  if (priv->input_window)
    gdk_window_show (priv->input_window);
}

static void
gtk3_ruler_unmap (GtkWidget *widget)
{
  Gtk3Ruler        *ruler = GTK3_RULER (widget);
  Gtk3RulerPrivate *priv  = GTK3_RULER_GET_PRIVATE (ruler);

  if (priv->input_window)
    gdk_window_hide (priv->input_window);
  
  GTK_WIDGET_CLASS (gtk3_ruler_parent_class)->unmap (widget);
}

static void
gtk3_ruler_size_allocate (GtkWidget     *widget,
                        GtkAllocation *allocation)
{
  Gtk3Ruler        *ruler = GTK3_RULER(widget);
  Gtk3RulerPrivate *priv  = GTK3_RULER_GET_PRIVATE (ruler);
  GtkAllocation   widget_allocation;
  gboolean        resized;

  gtk_widget_get_allocation (widget, &widget_allocation);
  
  resized = (widget_allocation.width  != allocation->width ||
             widget_allocation.height != allocation->height);

  GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

  if (gtk_widget_get_realized (widget))
    {
      gdk_window_move_resize (priv->input_window,
                              allocation->x, allocation->y,
                              allocation->width, allocation->height);
      
      if (resized)
        gtk3_ruler_make_pixmap (ruler);
    }
}

static void
gtk3_ruler_size_request (GtkWidget      *widget, 
                       GtkRequisition *requisition)
{
  Gtk3Ruler        *ruler = GTK3_RULER (widget);
  Gtk3RulerPrivate *priv  = GTK3_RULER_GET_PRIVATE (ruler);
  PangoLayout    *layout;
  PangoRectangle  ink_rect;
  gint            size;

  layout = gtk3_ruler_get_layout (widget, "0123456789");
  pango_layout_get_pixel_extents (layout, &ink_rect, NULL);

  size = 2 + ink_rect.height * 1.7;

  GtkStyleContext *context = gtk_widget_get_style_context (widget);
  GtkBorder        border;

  GtkStateFlags state_flags = gtk_style_context_get_state (context);

  gtk_style_context_get_border (context, state_flags, &border);
  
  requisition->width  = border.left + border.right;
  requisition->height = border.top  + border.bottom;

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      requisition->width  += 1;
      requisition->height += size;
    }
  else
    {
      requisition->width  += size;
      requisition->height += 1;
    }
}

static void
gtk3_ruler_style_updated (GtkWidget *widget)
{
  Gtk3Ruler        *ruler = GTK3_RULER (widget);
  Gtk3RulerPrivate *priv  = GTK3_RULER_GET_PRIVATE (ruler);

  GTK_WIDGET_CLASS (gtk3_ruler_parent_class)->style_updated (widget);

  gtk_widget_style_get (widget,
		        "font-scale", &priv->font_scale,
			NULL);

  if (priv->layout)
    {
     g_object_unref (priv->layout);
     priv->layout = NULL;
    }
}

static void
gtk3_ruler_get_preferred_width (GtkWidget *widget,
                              gint      *minimum_width,
                              gint      *natural_width)
{
  GtkRequisition requisition;

  gtk3_ruler_size_request (widget, &requisition);

  *minimum_width = *natural_width = requisition.width;
}

static void
gtk3_ruler_get_preferred_height (GtkWidget *widget,
                               gint      *minimum_height,
                               gint      *natural_height)
{
  GtkRequisition requisition;
  
  gtk3_ruler_size_request(widget, &requisition);
  
  *minimum_height = *natural_height = requisition.height;
}

static gboolean
gtk3_ruler_draw (GtkWidget *widget,
	       cairo_t *cr)
{
  Gtk3Ruler         *ruler   = GTK3_RULER (widget);
  Gtk3RulerPrivate  *priv    = GTK3_RULER_GET_PRIVATE (ruler);
  GtkStyleContext *context = gtk_widget_get_style_context (widget);
  GtkAllocation    allocation;

  gtk_widget_get_allocation (widget, &allocation);
  gtk_render_background (context, cr, 0, 0, allocation.width, allocation.height);
  gtk_render_frame (context, cr, 0, 0, allocation.width, allocation.height);

  gtk3_ruler_draw_ticks (ruler);

  cairo_set_source_surface(cr, priv->backing_store, 0, 0);
  cairo_paint(cr);

  gtk3_ruler_draw_pos (ruler, cr);

  return FALSE;
}

static void
gtk3_ruler_make_pixmap (Gtk3Ruler *ruler)
{
  GtkWidget      *widget = GTK_WIDGET (ruler);
  Gtk3RulerPrivate *priv   = GTK3_RULER_GET_PRIVATE (ruler);
  GtkAllocation   allocation;

  gtk_widget_get_allocation(widget, &allocation);

  if (priv->backing_store)
      cairo_surface_destroy (priv->backing_store);

  priv->backing_store = 
    gdk_window_create_similar_surface (gtk_widget_get_window (widget),
                                       CAIRO_CONTENT_COLOR_ALPHA,
                                       allocation.width,
                                       allocation.height);

  priv->backing_store_valid = FALSE;
}

static void
gtk3_ruler_draw_pos (Gtk3Ruler *ruler,
                   cairo_t *cr)
{
  GtkWidget       *widget  = GTK_WIDGET (ruler);

  GtkStyleContext *context = gtk_widget_get_style_context (widget);
  GdkRGBA          color;
  
  Gtk3RulerPrivate  *priv    = GTK3_RULER_GET_PRIVATE (ruler);
  GdkRectangle      pos_rect;

  if (! gtk_widget_is_drawable (widget))
      return;

  pos_rect = gtk3_ruler_get_pos_rect (ruler, gtk3_ruler_get_position (ruler));

  if ((pos_rect.width > 0) && (pos_rect.height > 0))
    {
      gtk_style_context_get_color (context, gtk_widget_get_state_flags (widget),
                                   &color);
      gdk_cairo_set_source_rgba (cr, &color);

      cairo_move_to (cr, pos_rect.x, pos_rect.y);

      if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
        {
          cairo_line_to (cr, pos_rect.x + pos_rect.width / 2.0,
                             pos_rect.y + pos_rect.height);
          cairo_line_to (cr, pos_rect.x + pos_rect.width,
                             pos_rect.y);
        }
      else
        {
          cairo_line_to (cr, pos_rect.x + pos_rect.width,

                             pos_rect.y + pos_rect.height / 2.0);
          cairo_line_to (cr, pos_rect.x,
                             pos_rect.y + pos_rect.height);
        }

      cairo_fill (cr);
    }

  if (priv->last_pos_rect.width  != 0 &&
      priv->last_pos_rect.height != 0)
    {
      gdk_rectangle_union (&priv->last_pos_rect,
                           &pos_rect,
                           &priv->last_pos_rect);
    }
  else
    {
      priv->last_pos_rect = pos_rect;
    }
}

/**
 * gtk3_ruler_new:
 * @orientation: the ruler's orientation
 *
 * Creates a new ruler.
 *
 * Return value: a new #Gtk3Ruler widget.
 */
GtkWidget *
gtk3_ruler_new (GtkOrientation orientation)
{
  return GTK_WIDGET (g_object_new (GTK3_TYPE_RULER, 
		                   "orientation", orientation,
                                   NULL));
}

static void
gtk3_ruler_update_position (Gtk3Ruler *ruler,
                          gdouble  x,
                          gdouble  y)
{
  Gtk3RulerPrivate *priv = GTK3_RULER_GET_PRIVATE (ruler);
  GtkAllocation   allocation;
  gdouble         lower;
  gdouble         upper;

  gtk_widget_get_allocation (GTK_WIDGET (ruler), &allocation);
  gtk3_ruler_get_range (ruler, &lower, &upper, NULL);

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
     gtk3_ruler_set_position (ruler,
                            lower +
                            (upper - lower) * x / allocation.width);
    }
  else
    {
     gtk3_ruler_set_position (ruler,
                            lower +
                            (upper - lower) * y / allocation.height);
    }
}

/* Returns TRUE if a translation should be done */
static gboolean
gtk_widget_get_translation_to_window (GtkWidget *widget,
		                      GdkWindow *window,
				      int       *x,
				      int       *y)
{
  GdkWindow *w, *widget_window;

  if (! gtk_widget_get_has_window (widget))
    {
      GtkAllocation allocation;

      gtk_widget_get_allocation (widget, &allocation);

      *x = -allocation.x;
      *y = -allocation.y;
    }
  else
    {
      *x = 0;
      *y = 0;
    }

  widget_window = gtk_widget_get_window (widget);

  for (w = window;
       w && w != widget_window;
       w = gdk_window_get_effective_parent (w))
    {
      gdouble px, py;

      gdk_window_coords_to_parent (w, *x, *y, &px, &py);

      *x += px;
      *y += px;
    }

  if (w == NULL)
    {
      *x = 0;
      *y = 0;
      return FALSE;
    }

  return TRUE;
}

static void
gtk3_ruler_event_to_widget_coords (GtkWidget *widget,
		                 GdkWindow *window,
				 gdouble    event_x,
				 gdouble    event_y,
				 gint      *widget_x,
				 gint      *widget_y)
{
  gint tx, ty;

  if (gtk_widget_get_translation_to_window (widget, window, &tx, &ty))
    {
      event_x += tx;
      event_y += ty;
    }

  *widget_x = event_x;
  *widget_y = event_y;
}

static gboolean
gtk3_ruler_track_widget_motion_notify (GtkWidget      *widget,
                                     GdkEventMotion *mevent,
                                     Gtk3Ruler        *ruler)
{
  gint widget_x;
  gint widget_y;
  gint ruler_x;
  gint ruler_y;

  widget = gtk_get_event_widget ((GdkEvent *)mevent);

  gtk3_ruler_event_to_widget_coords (widget, mevent->window,
		                   mevent->x,  mevent->y,
				   &widget_x, &widget_y);

  if (gtk_widget_translate_coordinates (widget, GTK_WIDGET (ruler),
			                widget_x, widget_y,
					&ruler_x, &ruler_y))
    {
      gtk3_ruler_update_position (ruler, ruler_x, ruler_y);
    }

  return FALSE;  
}

void
gtk3_ruler_add_track_widget (Gtk3Ruler   *ruler,
		           GtkWidget *widget)
{
  Gtk3RulerPrivate *priv;

  g_return_if_fail (GTK3_IS_RULER   (ruler));
  g_return_if_fail (GTK_IS_WIDGET (ruler));

  priv = GTK3_RULER_GET_PRIVATE (ruler);
  if (priv->track_widgets != NULL)
  {
       GList *lf = g_list_find (priv->track_widgets, widget);
       GList *fl = g_list_last (priv->track_widgets);
       if (lf != NULL)
       {
           GtkWidget *wlf = lf->data;
           GtkWidget *wfl = fl->data;
          g_return_if_fail (wfl == wlf);
	   }
  } 
  // priv->track_widgets->insert(widget);
  priv->track_widgets = g_list_append(priv->track_widgets, widget);

  g_signal_connect (widget, "motion-notify-event",
		    G_CALLBACK (gtk3_ruler_track_widget_motion_notify),
		    ruler);
  g_signal_connect (widget, "destroy",
		    G_CALLBACK (gtk3_ruler_remove_track_widget),
		    ruler);
}

/**
 * gtk3_ruler_remove_track_widget:
 * @ruler: an #Gtk3Ruler
 * @widget: the track widget to remove
 *
 * Removes a previously added track widget from the ruler. See
 * gtk3_ruler_add_track_widget().
 */
void
gtk3_ruler_remove_track_widget (Gtk3Ruler   *ruler,
                              GtkWidget *widget)
{
  g_return_if_fail (GTK3_IS_RULER   (ruler));
  g_return_if_fail (GTK_IS_WIDGET (ruler));

  Gtk3RulerPrivate *priv = GTK3_RULER_GET_PRIVATE (ruler);

  g_return_if_fail(g_list_find (priv->track_widgets, widget)->data!=g_list_last (priv->track_widgets)->data);
  
  g_signal_handlers_disconnect_by_func (widget,
                                        (gpointer) G_CALLBACK (gtk3_ruler_track_widget_motion_notify),
                                        ruler);
  g_signal_handlers_disconnect_by_func (widget,
                                        (gpointer) G_CALLBACK (gtk3_ruler_remove_track_widget),
                                        ruler);

  // priv->track_widgets->erase(widget);
  priv->track_widgets = g_list_remove (priv->track_widgets, widget);
  g_object_unref (widget);
}

/**
 * gtk3_ruler_set_unit:
 * @ruler: a #Gtk3Ruler
 * @unit: the #SPMetric to set the ruler to
 *
 * This sets the unit of the ruler.
 */
void
gtk3_ruler_set_unit (Gtk3Ruler  *ruler,
                   Gtk3RulerMetricUnit unit)
{
  Gtk3RulerPrivate *priv = GTK3_RULER_GET_PRIVATE (ruler);
  
  g_return_if_fail (GTK3_IS_RULER (ruler));

  if (priv->unit != unit)
    {
	  priv->unit = unit;
	  switch (unit)
	  {
		  case GTK3_RULER_METRIC_INCHES:
	         ruler_metric = ruler_metric_inches;
		     break;
#if 0
		  case GTK3_RULER_METRIC_FEET:
	         ruler_metric = ruler_metric_feet;
		     break;
		  case GTK3_RULER_METRIC_YARDS:
	         ruler_metric = ruler_metric_yards;
		     break;
#endif
		  default :
	         ruler_metric = ruler_metric_general;
		     break;
	  } 
      g_object_notify(G_OBJECT(ruler), "unit");

      priv->backing_store_valid = FALSE;
      gtk_widget_queue_draw (GTK_WIDGET (ruler));
    }
}

/**
 * gtk3_ruler_get_unit:
 * @ruler: a #Gtk3Ruler
 *
 * Return value: the unit currently used in the @ruler widget.
 **/
Gtk3RulerMetricUnit
gtk3_ruler_get_unit (Gtk3Ruler *ruler)
{
  return GTK3_RULER_GET_PRIVATE (ruler)->unit;
}

/**
 * gtk3_ruler_set_position:
 * @ruler: a #Gtk3Ruler
 * @position: the position to set the ruler to
 *
 * This sets the position of the ruler.
 */
void
gtk3_ruler_set_position (Gtk3Ruler *ruler,
                       gdouble  position)
{
    Gtk3RulerPrivate *priv;

    g_return_if_fail (GTK3_IS_RULER (ruler));
    
    priv = GTK3_RULER_GET_PRIVATE (ruler);

    if (priv->position != position)
    {
      GdkRectangle rect;
      gint xdiff, ydiff;

      priv->position = position;
      g_object_notify (G_OBJECT (ruler), "position");

      rect = gtk3_ruler_get_pos_rect (ruler, priv->position);

      xdiff = rect.x - priv->last_pos_rect.x;
      ydiff = rect.y - priv->last_pos_rect.y;

      /*
       * If the position has changed far enough, queue a redraw immediately.
       * Otherwise, we only queue a redraw in a low priority idle handler, to
       * allow for other things (like updating the canvas) to run.
       *
       * TODO: This might not be necessary any more in GTK3 with the frame
       *       clock. Investigate this more after the port to GTK3.
       */
      if (priv->last_pos_rect.width  != 0 &&
          priv->last_pos_rect.height != 0 &&
          (ABS (xdiff) > IMMEDIATE_REDRAW_THRESHOLD ||
           ABS (ydiff) > IMMEDIATE_REDRAW_THRESHOLD))
        {
          if (priv->pos_redraw_idle_id)
            {
              g_source_remove (priv->pos_redraw_idle_id);
              priv->pos_redraw_idle_id = 0;
            }

          gtk3_ruler_queue_pos_redraw (ruler);
        }
      else if (! priv->pos_redraw_idle_id)
        {
          priv->pos_redraw_idle_id =
            g_idle_add_full (G_PRIORITY_LOW,
                             gtk3_ruler_idle_queue_pos_redraw,
                             ruler, NULL);
        }
    }
}

/**
 * gtk3_ruler_get_position:
 * @ruler: a #Gtk3Ruler
 *
 * Return value: the current position of the @ruler widget.
 */
gdouble
gtk3_ruler_get_position (Gtk3Ruler *ruler)
{
    g_return_val_if_fail (GTK3_IS_RULER (ruler), 0.0);

    return GTK3_RULER_GET_PRIVATE (ruler)->position;
}

static gboolean
gtk3_ruler_motion_notify (GtkWidget      *widget,
                        GdkEventMotion *event)
{
  Gtk3Ruler *ruler = GTK3_RULER(widget);

  gtk3_ruler_update_position (ruler, event->x, event->y);

  return FALSE;
}

static void
gtk3_ruler_draw_ticks (Gtk3Ruler *ruler)
{
    GtkWidget       *widget  = GTK_WIDGET (ruler);

    GtkStyleContext *context = gtk_widget_get_style_context (widget);
    GtkBorder        border;
    GdkRGBA          color;

    Gtk3RulerPrivate  *priv    = GTK3_RULER_GET_PRIVATE (ruler);
    GtkAllocation    allocation;
    cairo_t         *cr;
    gint             i;
    gint             width, height;
    gint             length, ideal_length;
    gdouble          lower, upper; /* Upper and lower limits, in ruler units */
    gdouble          increment;    /* Number of pixels per unit */
    guint            scale;        /* Number of units per major unit */
    gdouble          start, end, cur;
    gchar            unit_str[32];
    gint             digit_height;
    gint             digit_offset;
    gchar            digit_str[2] = { '\0', '\0' };
    gint             text_size;
    gdouble          pos;
    gdouble          max_size;
    Gtk3RulerMetricUnit unit;
    Gtk3RulerMetric    ruler_metric = ruler_metric_general; /* The metric to use for this unit system */
    PangoLayout     *layout;
    PangoRectangle   logical_rect, ink_rect;
    
    if (! gtk_widget_is_drawable (widget))
      return;

    gtk_widget_get_allocation (widget, &allocation);

    GtkStateFlags state_flags = gtk_style_context_get_state (context);

    gtk_style_context_get_border (context, state_flags, &border);

    layout = gtk3_ruler_get_layout (widget, "0123456789");
    pango_layout_get_extents (layout, &ink_rect, &logical_rect);
    
    digit_height = PANGO_PIXELS (ink_rect.height) + 2;
    digit_offset = ink_rect.y;
    
    if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
     {
        width = allocation.width;
        height = allocation.height - (border.top + border.bottom);
     }
    else
     {
        width = allocation.height;
        height = allocation.width - (border.top + border.bottom);
     }

    cr = cairo_create (priv->backing_store);
    
    cairo_set_line_width(cr, 1.0);

    cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint (cr);
    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

    gtk_style_context_get_color (context, gtk_widget_get_state_flags (widget),
                                 &color);
    gdk_cairo_set_source_rgba (cr, &color);

    if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
      {
        cairo_rectangle (cr,
                         border.left,
                         height + border.top,
                         allocation.width - (border.left + border.right),
                         1);      
      }
    else
      {
        cairo_rectangle (cr,
                         height + border.left,
                         border.top,
                         1,
                         allocation.height - (border.top + border.bottom));
      }

    gtk3_ruler_get_range (ruler, &lower, &upper, &max_size);

    if ((upper - lower) == 0)
        goto out;

    increment = (gdouble) width / (upper - lower);

    /* determine the scale
    *    Use the maximum extents of the ruler to determine the largest
    *    possible number to be displayed.  Calculate the height in pixels
    *    of this displayed text. Use this height to find a scale which
    *    leaves sufficient room for drawing the ruler.
    *
    *    We calculate the text size as for the vruler instead of
    *    actually measuring the text width, so that the result for the
    *    scale looks consistent with an accompanying vruler
    */
    scale = ceil (priv->max_size);
    sprintf (unit_str, "%d", scale);
    text_size = strlen (unit_str) * digit_height + 1;

    /* Inkscape change to ruler: Use a 1,2,4,8... scale for inches
     * or a 1,2,5,10... scale for everything else */
    if (gtk3_ruler_get_unit (ruler) == GTK3_RULER_METRIC_INCHES)
      ruler_metric = ruler_metric_inches;

    for (scale = 0; scale < G_N_ELEMENTS (ruler_metric.ruler_scale); scale++)
        if (ruler_metric.ruler_scale[scale] * fabs (increment) > 2 * text_size)
            break;

    if (scale == G_N_ELEMENTS (ruler_metric.ruler_scale))
        scale = G_N_ELEMENTS (ruler_metric.ruler_scale) - 1;

    unit = gtk3_ruler_get_unit (ruler);

    /* drawing starts here */
    length = 0;
    for (i = G_N_ELEMENTS (ruler_metric.subdivide) - 1; i >= 0; i--)
      {
        gdouble subd_incr;
       
        /* hack to get proper subdivisions at full pixels */
        if (/*unit == *unit_table.getUnit("px") &&*/ scale == 1 && i == 1)
          subd_incr = 1.0;
        else
          subd_incr = ((gdouble) ruler_metric.ruler_scale[scale] / 
                       (gdouble) ruler_metric.subdivide[i]);

        if (subd_incr * fabs (increment) <= MINIMUM_INCR) 
            continue;

        /* Calculate the length of the tickmarks. Make sure that
        * this length increases for each set of ticks
        */
        ideal_length = height / (i + 1) - 1;
        if (ideal_length > ++length)
            length = ideal_length;

        if (lower < upper)
          {
            start = floor (lower / subd_incr) * subd_incr;
            end   = ceil  (upper / subd_incr) * subd_incr;
          }
        else
          {
            start = floor (upper / subd_incr) * subd_incr;
            end   = ceil  (lower / subd_incr) * subd_incr;
          }

        gint tick_index = 0;

        for (cur = start; cur <= end; cur += subd_incr)
          {
            pos = floor((cur - lower) * increment) + 0.5;

            if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
             {
               cairo_move_to(cr, pos, height + border.top - length);
               cairo_line_to(cr, pos, height + border.top);
             }
            else 
             {
               cairo_move_to(cr, height + border.left - length, pos);
               cairo_line_to(cr, height + border.left         , pos);
             }
             cairo_stroke(cr);

            /* draw label */
            double label_spacing_px = fabs(increment*(double)ruler_metric.ruler_scale[scale]/ruler_metric.subdivide[i]);
            if (i == 0 && 
                (label_spacing_px > 6*digit_height || tick_index%2 == 0 || cur == 0) && 
                (label_spacing_px > 3*digit_height || tick_index%4 == 0 || cur == 0))
              {
                if (abs((int)cur) >= 2000 && (((int) cur)/1000)*1000 == ((int) cur))
                    sprintf (unit_str, "%dk", ((int) cur)/1000);
                else
                    sprintf (unit_str, "%d", (int) cur);

                if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
                  {
                    pango_layout_set_text (layout, unit_str, -1);
                    pango_layout_get_extents (layout, &logical_rect, NULL);

                    cairo_move_to (cr,
                                   pos + 2,
                                   border.top + PANGO_PIXELS (logical_rect.y - digit_offset));

                    pango_cairo_show_layout(cr, layout);
                  }
                else
                  {
                    gint j;

                    for (j = 0; j < (int) strlen (unit_str); j++)
                      {
                        digit_str[0] = unit_str[j];
                        pango_layout_set_text (layout, digit_str, 1);
                        pango_layout_get_extents (layout, NULL, &logical_rect);

                        cairo_move_to (cr,
                                       border.left + 1,
                                       pos + digit_height * j + 2 + PANGO_PIXELS (logical_rect.y - digit_offset));
                        pango_cairo_show_layout (cr, layout);
                      }
                  }
              }
        
            ++tick_index;
          }
      }

    cairo_fill (cr);

  priv->backing_store_valid = TRUE;

out:
    cairo_destroy (cr);
}

static GdkRectangle
gtk3_ruler_get_pos_rect (Gtk3Ruler *ruler,
                       gdouble  position)
{
  GtkWidget        *widget = GTK_WIDGET (ruler);
  Gtk3RulerPrivate   *priv   = GTK3_RULER_GET_PRIVATE (ruler);
  GtkAllocation     allocation;
  gint              width, height;
  gint              xthickness;
  gint              ythickness;
  gdouble           upper, lower;
  gdouble           increment;
  GdkRectangle      rect = { 0, 0, 0, 0 };

  if (! gtk_widget_is_drawable (widget))
    return rect;

  gtk_widget_get_allocation (widget, &allocation);

  GtkStyleContext *context = gtk_widget_get_style_context (widget);
  GtkBorder padding;

  GtkStateFlags state_flags = gtk_style_context_get_state (context);

  gtk_style_context_get_border(context, state_flags, &padding);

  xthickness = padding.left + padding.right;
  ythickness = padding.top + padding.bottom;

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      width  = allocation.width;
      height = allocation.height - ythickness * 2;

      rect.width = height / 2 + 2;
      rect.width |= 1;  /* make sure it's odd */
      rect.height = rect.width / 2 + 1;
    }
  else
    {
      width  = allocation.width - xthickness * 2;
      height = allocation.height;

      rect.height = width / 2 + 2;
      rect.height |= 1;  /* make sure it's odd */
      rect.width = rect.height / 2 + 1;
    }

  gtk3_ruler_get_range (ruler, &lower, &upper, NULL);

  if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      increment = (gdouble) width / (upper - lower);

      rect.x = ROUND ((position - lower) * increment) + (xthickness - rect.width) / 2 - 1;
      rect.y = (height + rect.height) / 2 + ythickness;
    }
  else
    {
      increment = (gdouble) height / (upper - lower);

      rect.x = (width + rect.width) / 2 + xthickness;
      rect.y = ROUND ((position - lower) * increment) + (ythickness - rect.height) / 2 - 1;
    }

  return rect;
}

static gboolean
gtk3_ruler_idle_queue_pos_redraw (gpointer data)
{
  Gtk3Ruler        *ruler = (Gtk3Ruler *)data;
  Gtk3RulerPrivate *priv  = GTK3_RULER_GET_PRIVATE (ruler);

  gtk3_ruler_queue_pos_redraw (ruler);

  gboolean ret = g_source_remove(priv->pos_redraw_idle_id);
  priv->pos_redraw_idle_id = 0;

  return ret;
}

static void
gtk3_ruler_queue_pos_redraw (Gtk3Ruler *ruler)
{
  Gtk3RulerPrivate    *priv = GTK3_RULER_GET_PRIVATE (ruler);
  const GdkRectangle rect = gtk3_ruler_get_pos_rect (ruler, priv->position);
  GtkAllocation      allocation;

  gtk_widget_get_allocation (GTK_WIDGET(ruler), &allocation);

  gtk_widget_queue_draw_area (GTK_WIDGET (ruler),
                              rect.x + allocation.x,
                              rect.y + allocation.y,
                              rect.width,
                              rect.height);

  if (priv->last_pos_rect.width  != 0  &&
      priv->last_pos_rect.height != 0)
    {
      gtk_widget_queue_draw_area (GTK_WIDGET (ruler),
                                  priv->last_pos_rect.x + allocation.x,
                                  priv->last_pos_rect.y + allocation.y,
                                  priv->last_pos_rect.width,
                                  priv->last_pos_rect.height);

      priv->last_pos_rect.x      = 0;
      priv->last_pos_rect.y      = 0;
      priv->last_pos_rect.width  = 0;
      priv->last_pos_rect.height = 0;
    }
}

static PangoLayout*
gtk3_ruler_create_layout (GtkWidget   *widget,
                        const gchar *text)
{
  Gtk3Ruler        *ruler = GTK3_RULER (widget);
  Gtk3RulerPrivate *priv  = GTK3_RULER_GET_PRIVATE (ruler);
  PangoLayout    *layout;
  PangoAttrList  *attrs;
  PangoAttribute *attr;

  layout = gtk_widget_create_pango_layout (widget, text);

  attrs = pango_attr_list_new ();

  attr = pango_attr_scale_new (priv->font_scale);
  attr->start_index = 0;
  attr->end_index   = -1;
  pango_attr_list_insert (attrs, attr);

  pango_layout_set_attributes (layout, attrs);
  pango_attr_list_unref (attrs);

  return layout;
}

static PangoLayout *
gtk3_ruler_get_layout (GtkWidget   *widget,
                     const gchar *text)
{
  Gtk3Ruler        *ruler = GTK3_RULER (widget);
  Gtk3RulerPrivate *priv  = GTK3_RULER_GET_PRIVATE (ruler);

  if (priv->layout)
    {
      pango_layout_set_text (priv->layout, text, -1);
      return priv->layout;
    }

  priv->layout = gtk3_ruler_create_layout (widget, text);

  return priv->layout;
}
