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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "gtkgamma.h"


/* forward declarations: */
static void gtk3_gamma_curve_destroy (GtkWidget *object);

static void curve_type_changed_callback (GtkWidget *w, gpointer data);
static void button_realize_callback     (GtkWidget *w);
static void button_toggled_callback     (GtkWidget *w, gpointer data);
static void button_clicked_callback     (GtkWidget *w, gpointer data);

static const gchar * button_images[] = {
	"/images/spline.png" ,
	"/images/linear.png" ,
	"/images/free.png" ,
	"/images/gamma.png" ,
	"/images/reset.png"
};

enum
  {
    LINEAR = 0,
    SPLINE,
    FREE,
    GAMMA,
    RESET,
    NUM_XPMS
  };

G_DEFINE_TYPE (Gtk3GammaCurve, gtk3_gamma_curve, GTK_TYPE_BOX)

static void
gtk3_gamma_curve_class_init (Gtk3GammaCurveClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  
 //  object_class->destroy = gtk3_gamma_curve_destroy;
}

static void
gtk3_gamma_curve_init (Gtk3GammaCurve *curve)
{
  GtkWidget *image = NULL;
  GtkWidget *vbox;
  int i;
  
  gtk_orientable_set_orientation (GTK_ORIENTABLE (curve), GTK_ORIENTATION_VERTICAL);
  
  curve->gamma = 1.0;

  // curve->table = gtk_table_new (1, 2, FALSE);
  curve->table = gtk_grid_new ();
  gtk_grid_set_column_homogeneous (GTK_GRID (curve->table), FALSE);
  gtk_grid_set_row_homogeneous (GTK_GRID (curve->table), FALSE);
  
  //gtk_table_set_col_spacings (GTK_TABLE (curve->table), 3);
  gtk_container_add (GTK_CONTAINER (curve), curve->table);
  // GTK4 => gtk_box_append (GTK_BOX (curve), curve->table);
  
  curve->curve = gtk3_curve_new ();
  g_signal_connect (curve->curve, "curve-type-changed",
		    G_CALLBACK (curve_type_changed_callback), curve);
  gtk_grid_attach (GTK_GRID (curve->table), curve->curve,  0, 0 , 1, 1);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, /* spacing */ 3);
  gtk_box_set_homogeneous (GTK_BOX (vbox), FALSE);
  
  gtk_grid_attach (GTK_GRID (curve->table), vbox,  1, 0 , 1, 1);

  /* toggle buttons: */
  for (i = 0; i < 3; ++i)
    {
	  curve->button[i] = gtk_toggle_button_new ();

      image = gtk_image_new_from_resource (button_images[i]);
      gtk_button_set_image (GTK_BUTTON (curve->button[i]), image);
     // GTK 4 => gtk_button_set_child (GTK_BUTTON (curve->button[i]), image);
      
      g_object_set_data (G_OBJECT (curve->button[i]), "_Gtk3GammaCurveIndex",
			 GINT_TO_POINTER (i));
	  gtk_container_add (GTK_CONTAINER (vbox), curve->button[i]);
      // GTK4 => gtk_box_append (GTK_BOX (vbox), curve->button[i]);
      g_signal_connect (G_OBJECT (curve->button[i]), "toggled",
			G_CALLBACK (button_toggled_callback), curve);
      gtk_widget_show (curve->button[i]);
    }

  /* push buttons: */
  for (i = 3; i < 5; ++i)
    {
      curve->button[i] = gtk_button_new ();

      image = gtk_image_new_from_resource (button_images[i]);
      gtk_button_set_image (GTK_BUTTON (curve->button[i]), image);
     // GTK 4 => gtk_button_set_child (GTK_BUTTON (curve->button[i]), image);
      
      g_object_set_data (G_OBJECT (curve->button[i]), "_Gtk3GammaCurveIndex",
			 GINT_TO_POINTER (i));
	  gtk_container_add (GTK_CONTAINER (vbox), curve->button[i]);
      // GTK4 => gtk_box_append (GTK_BOX (vbox), curve->button[i]);
      g_signal_connect (G_OBJECT (curve->button[i]), "clicked",
			G_CALLBACK (button_clicked_callback), curve);
      gtk_widget_show (curve->button[i]);
    }

  gtk_widget_show (vbox);
  gtk_widget_show (curve->table);
  gtk_widget_show (curve->curve);
}

static void
button_toggled_callback (GtkWidget *w, gpointer data)
{
  Gtk3GammaCurve *c = data;
  Gtk3CurveType type;
  int active, i;

  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)))
    return;

  active = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (w), "_Gtk3GammaCurveIndex"));

  for (i = 0; i < 3; ++i)
    if ((i != active) && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (c->button[i])))
      break;

  if (i < 3)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (c->button[i]), FALSE);

  switch (active)
    {
    case 0:  type = GTK3_CURVE_TYPE_SPLINE; break;
    case 1:  type = GTK3_CURVE_TYPE_LINEAR; break;
    default: type = GTK3_CURVE_TYPE_FREE; break;
    }
  gtk3_curve_set_curve_type (c->curve, type);
}

static void
gamma_cancel_callback (GtkWidget *w, gpointer data)
{
  Gtk3GammaCurve *c = data;

  gtk_widget_destroy (c->gamma_dialog);
  c->gamma_dialog = NULL;
}

static void
gamma_response_callback (GtkWidget *w,
                                               gint response_id,
                                               gpointer data)
{
	if (response_id == GTK_RESPONSE_OK)
	   {
           Gtk3GammaCurve *c = data;
           const gchar *start;
           gchar *end;
           gfloat v;

           start = gtk_entry_get_text (GTK_ENTRY (c->gamma_text));
           // GTK 4 => start = gtk_editable_get_text (GTK_EDITABLE (c->gamma_text));
           if (start)
             {
                  v = g_strtod (start, &end);
                  if (end > start && v > 0.0)
	                   c->gamma = v;
             }
          gtk3_curve_set_gamma (c->curve, c->gamma);
	  }
   gamma_cancel_callback (w, data);
}

static void
button_clicked_callback (GtkWidget *w, gpointer data)
{
  Gtk3GammaCurve *c = data;
  int active;

  active = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (w), "_Gtk3GammaCurveIndex"));
  if (active == 3)
    {
      /* set gamma */
      if (c->gamma_dialog)
	return;
      else
	{
	  GtkWidget *vbox, *hbox, *label, *button;
	  gchar buf[64];
	  
	  c->gamma_dialog = gtk_dialog_new_with_buttons ("Gamma",
	                                                                                       GTK_WINDOW (w),
	                                                                                       GTK_DIALOG_DESTROY_WITH_PARENT,
	                                                                                       "Ok", GTK_RESPONSE_OK,
	                                                                                       "Cancel", GTK_RESPONSE_CANCEL,
	                                                                                       NULL);

      g_object_add_weak_pointer (G_OBJECT (c->gamma_dialog),
				                                      (gpointer *)&c->gamma_dialog);
	  
	  vbox =gtk_dialog_get_content_area (GTK_DIALOG (c->gamma_dialog));

	  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, /* spacing */ 0);
      gtk_box_set_homogeneous (GTK_BOX (hbox), FALSE);
  
	  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 2);
	  // GTK4 => gtk_box_append (GTK_BOX (vbox), hbox);
	  gtk_widget_show (hbox);
	  
	  label = gtk_label_new_with_mnemonic ("_Gamma value");
	  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
	  // GTK4 => gtk_box_append (GTK_BOX (hbox), label);
	  gtk_widget_show (label);
	  
	  sprintf (buf, "%g", c->gamma);
	  c->gamma_text = gtk_entry_new ();
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), c->gamma_text);
      gtk_entry_set_text (GTK_ENTRY (c->gamma_text), buf);
	   // GTK4 => gtk_editable_set_text (GTK_EDITABLE (c->gamma_text), buf);
	  gtk_box_pack_start (GTK_BOX (hbox), c->gamma_text, TRUE, TRUE, 2);
	  // GTK4 => gtk_box_append (GTK_BOX (hbox), c->gamma_text);
	  gtk_widget_show (c->gamma_text);

	  /* fill in action area: */
	   g_signal_connect (c->gamma_dialog,
	                                 "response", 
	                                 G_CALLBACK (gamma_response_callback),
	                                 c);	  
	  gtk_widget_show (c->gamma_dialog);
	}
    }
  else
    {
      /* reset */
      gtk3_curve_reset (c->curve);
    }
}

static void
curve_type_changed_callback (GtkWidget *w, gpointer data)
{
  Gtk3GammaCurve *c = data;
  Gtk3CurveType new_type;
  int active;

  new_type = gtk3_curve_get_curve_type (w);
  switch (new_type)
    {
    case GTK3_CURVE_TYPE_SPLINE: active = 0; break;
    case GTK3_CURVE_TYPE_LINEAR: active = 1; break;
    default:		        active = 2; break;
    }
  if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (c->button[active])))
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (c->button[active]), TRUE);
}

GtkWidget*
gtk3_gamma_curve_new (void)
{
  return g_object_new (GTK3_TYPE_GAMMA_CURVE, NULL);
  
}

static void
gtk3_gamma_curve_destroy (GtkWidget *object)
{
  Gtk3GammaCurve *c = GTK3_GAMMA_CURVE (object);

  if (c->gamma_dialog)
    gtk_widget_destroy (c->gamma_dialog);

  // G_OBJECT_CLASS (gtk3_gamma_curve_parent_class)->destroy (object);
}

