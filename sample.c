#include <gtk/gtk.h>
#include <gtk3curve.h>

#define WINDOW_WIDTH  400
#define WINDOW_HEIGHT 400

int main (int argc, char *argv[])
{
  Gtk3CurveColor background;
  Gtk3CurveColor cpoint;
  Gtk3CurveColor grid;
  Gtk3CurveColor curve;
  GtkWidget *window;
  GtkWidget *da;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  da = gtk3_curve_new ();
  gtk_widget_set_name (da, "curve1");

  background.red = 0.1;
  background.green = 0.1;
  background.blue = 0.1;
  background.alpha = 1.0;

  curve.red = 1.0;
  curve.green = 1.0;
  curve.blue = 1.0;
  curve.alpha = 1.0;

  grid.red = 1.0;
  grid.green = 1.0;
  grid.blue = 1.0;
  grid.alpha = 1.0;

  cpoint.red = 0.8;
  cpoint.green = 0.3;
  cpoint.blue = 0.3;
  cpoint.alpha = 1.0;

  gtk3_curve_set_color_background (da, background);
  gtk3_curve_set_color_grid (da, grid);
  gtk3_curve_set_color_curve (da, curve);
  gtk3_curve_set_color_cpoint (da, cpoint);

  gtk3_curve_set_use_theme_background(da, FALSE);
  gtk3_curve_set_grid_size (da, GTK3_CURVE_GRID_LARGE);
  gtk3_curve_set_range (da, 0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);

  gtk_container_add (GTK_CONTAINER (window), da);
  gtk_widget_show (da);
  gtk_widget_show (window);

  gtk_main ();

  return 0;
}
