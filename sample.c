/* COMPILE USING:  gcc -Wextra -o test `pkg-config --cflags --libs gtk+-3.0` test.c */
#include <gtk/gtk.h>
#include <gtk3curve.h>
//#include <gtk3curvetest.h>

#define WINDOW_WIDTH  400
#define WINDOW_HEIGHT 400

int main (int argc, char *argv[])
{
   GtkWidget *window;
   GtkWidget *da;

   gtk_init (&argc, &argv);

   window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
   g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

   da = gtk3_curve_new ();
   gtk_widget_set_name (da, "curve1");
   gtk3_curve_set_color_background (da, 0.0, 0.0, 0.0, 1.0);
   gtk3_curve_set_color_line (da, 1.0, 1.0, 1.0, 0.7);
   gtk3_curve_set_color_dot (da, 0.8, 0.5, 0.5, 0.9);
   gtk3_curve_set_grid_size (da, GTK3_CURVE_GRID_MEDIUM);
   gtk3_curve_set_range (da, 0, WINDOW_WIDTH, 0, WINDOW_HEIGHT);
   gtk_container_add (GTK_CONTAINER (window), da);
   gtk_widget_show (da);
   gtk_widget_show (window);

   gtk_main ();

   return 0;
}
