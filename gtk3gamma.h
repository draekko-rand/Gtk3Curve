/* GTK - The GIMP Toolkit
 * Copyright (C) 1997 David Mosberger
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/.
 */

#ifndef __GTK3_GAMMA_CURVE_H__
#define __GTK3_GAMMA_CURVE_H__

#include "gtk3curve.h"


#define GTK3_TYPE_GAMMA_CURVE            (gtk3_gamma_curve_get_type ())
#define GTK3_GAMMA_CURVE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK3_TYPE_GAMMA_CURVE, Gtk3GammaCurve))
#define GTK3_GAMMA_CURVE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK3_TYPE_GAMMA_CURVE, Gtk3GammaCurveClass))
#define GTK3_IS_GAMMA_CURVE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK3_TYPE_GAMMA_CURVE))
#define GTK3_IS_GAMMA_CURVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK3_TYPE_GAMMA_CURVE))
#define GTK3_GAMMA_CURVE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK3_TYPE_GAMMA_CURVE, Gtk3GammaCurveClass))

typedef struct _Gtk3GammaCurve		Gtk3GammaCurve;
typedef struct _Gtk3GammaCurveClass	Gtk3GammaCurveClass;


struct _Gtk3GammaCurve
{
  GtkBox vbox;

  GtkWidget *table;
  GtkWidget *curve;
  GtkWidget *button[5];	/* spline, linear, free, gamma, reset */

  gfloat gamma;
  GtkWidget *gamma_dialog;
  GtkWidget *gamma_text;
};

struct _Gtk3GammaCurveClass
{
  GtkBoxClass parent_class;

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};


GType      gtk3_gamma_curve_get_type (void) G_GNUC_CONST;
GtkWidget* gtk3_gamma_curve_new      (void);


#endif /* __GTK3_GAMMA_CURVE_H__ */

