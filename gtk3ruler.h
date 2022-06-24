// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef __GTK3_RULER_H__
#define __GTK3_RULER_H__

/*
 * Customized ruler class for inkscape
 *
 * Authors:
 *   Lauris Kaplinski <lauris@kaplinski.com>
 *   Frank Felfe <innerspace@iname.com>
 *
 * Copyright (C) 1999-2002 authors
 *
 * Released under GNU GPL v2+, read the file 'COPYING' for more information.
 */

#include <gtk/gtk.h>
#include <glib.h>

#define GTK3_TYPE_RULER            (gtk3_ruler_get_type ())
#define GTK3_RULER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK3_TYPE_RULER, Gtk3Ruler))
#define GTK3_RULER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK3_TYPE_RULER, Gtk3RulerClass))
#define GTK3_IS_RULER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK3_TYPE_RULER))
#define GTK3_IS_RULER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK3_TYPE_RULER))
#define GTK3_RULER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK3_TYPE_RULER, Gtk3RulerClass))

typedef struct _Gtk3Ruler         Gtk3Ruler;
typedef struct _Gtk3RulerClass    Gtk3RulerClass;
typedef struct _Gtk3RulerPrivate Gtk3RulerPrivate;

struct _Gtk3Ruler
{
  GtkWidget parent_instance;
  Gtk3RulerPrivate *priv;
};

struct _Gtk3RulerClass
{
  GtkWidgetClass parent_class;
};

typedef enum
{
  GTK3_RULER_METRIC_DECIMAL = 0,
  GTK3_RULER_METRIC_INCHES,
#if 0
  GTK3_RULER_METRIC_FEET,
  GTK3_RULER_METRIC_YARDS,
#endif
} Gtk3RulerMetricUnit;

GType           gtk3_ruler_get_type            (void) G_GNUC_CONST;

GtkWidget*      gtk3_ruler_new                 (GtkOrientation  orientation);

void            gtk3_ruler_add_track_widget    (Gtk3Ruler        *ruler,
		                              GtkWidget      *widget);
void            gtk3_ruler_remove_track_widget (Gtk3Ruler        *ruler,
		                              GtkWidget      *widget);

void            gtk3_ruler_set_unit            (Gtk3Ruler        *ruler,
                                              Gtk3RulerMetricUnit unit);
Gtk3RulerMetricUnit gtk3_ruler_get_unit            (Gtk3Ruler        *ruler);
void            gtk3_ruler_set_position        (Gtk3Ruler        *ruler,
                                              gdouble         set_position);
gdouble         gtk3_ruler_get_position        (Gtk3Ruler        *ruler);
void            gtk3_ruler_set_range           (Gtk3Ruler        *ruler,
                                              gdouble         lower,
                                              gdouble         upper,
                                              gdouble         max_size);
void            gtk3_ruler_get_range           (Gtk3Ruler        *ruler,
                                              gdouble        *lower,
                                              gdouble        *upper,
                                              gdouble        *max_size);

G_END_DECLS

#endif /* __GTK3_RULER_H__ */
