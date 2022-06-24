#include <gtk/gtk.h>
#include <gtk3curve.h>
#include <gtk3gamma.h>
#include <gtk3ruler.h>

#define WINDOW_WIDTH  400
#define WINDOW_HEIGHT 400

int main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *da;
  GtkWidget *gamma;
  GtkWidget *grid;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);
  grid = gtk_grid_new();
  if (strstr(argv[0], "gtk3ruler-sample"))
  {
	gamma = gtk_drawing_area_new();
	GtkWidget*hruler = gtk3_ruler_new (GTK_ORIENTATION_HORIZONTAL);
	gtk3_ruler_add_track_widget (GTK3_RULER (hruler), gamma);
	gtk_grid_attach(GTK_GRID(grid), hruler, 1, 0, 1, 1);
	gtk3_ruler_set_range (GTK3_RULER (hruler),
	      	  0,
	      	  WINDOW_WIDTH,
	      	  // f * min_x,
	      	  WINDOW_WIDTH); // max_size

	GtkWidget*vruler = gtk3_ruler_new (GTK_ORIENTATION_VERTICAL);
	gtk_grid_attach(GTK_GRID(grid), vruler, 0, 1, 1, 1);
	gtk3_ruler_add_track_widget (GTK3_RULER (vruler), gamma);
	gtk3_ruler_set_range (GTK3_RULER (vruler),
			      0,
			      WINDOW_HEIGHT,
			      // f * min_x,
			      WINDOW_HEIGHT); // max_size
        gtk_widget_show (vruler);
        gtk_widget_show (hruler);
  }
  else
  {
	if (strstr(argv[0], "gtk3curve-sample"))
	{
	        da = gtk3_curve_new ();
	        gtk_widget_set_name (da, "curve1");
	        gamma = da;
	}
	else
	{
	        gamma = gtk3_gamma_curve_new ();
	        da = GTK3_GAMMA_CURVE (gamma)->curve;
                gtk_widget_show (da);
	        gtk_widget_set_name (gamma, "GammaCurve");
	        // gtk_widget_set_size_request (da, WINDOW_WIDTH - 18, WINDOW_HEIGHT - 18);
	}

	gtk3_curve_set_color_background_rgba (da, 0.8, 0.8, 0.8, 0.5);
	gtk3_curve_set_color_grid_rgba (da, 0.0, 0.0, 0.0, 0.5);
	gtk3_curve_set_color_curve_rgba (da, 1.0, 1.0, 1.0, 1.0);
	gtk3_curve_set_color_cpoint_rgba (da, 0.8, 0.3, 0.3, 1.0);

	gtk3_curve_set_use_theme_background(da, FALSE);
	gtk_widget_set_size_request (da, WINDOW_WIDTH, WINDOW_HEIGHT);
	gtk3_curve_set_grid_size (da, GTK3_CURVE_GRID_SMALL);
	gtk3_curve_set_range (da, 0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
  }
  gtk_widget_set_size_request (gamma, WINDOW_WIDTH, WINDOW_HEIGHT);
  gtk_grid_attach(GTK_GRID(grid), gamma, 1, 1, 1, 1);
  gtk_container_add (GTK_CONTAINER (window), grid);

  gtk_widget_show (gamma);
  gtk_widget_show (grid);
  gtk_widget_show (window);

  gtk_main ();

  return 0;
}
