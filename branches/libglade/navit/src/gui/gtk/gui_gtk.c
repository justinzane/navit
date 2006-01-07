#include <stdio.h>
#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <glade/glade.h>
#include "coord.h"
#include "transform.h"
#include "container.h"
#include "gui_gtk.h"


extern void container_init_gra(struct container *co);

void on_Zoom_in_clicked(GtkWidget *widget, gpointer user_data);
void on_Zoom_out_clicked(GtkWidget *widget, gpointer user_data);
void on_Refresh_clicked(GtkWidget *widget, gpointer user_data);
void on_Cursor_toggled(GtkWidget *widget, gpointer user_data);
void  on_Orientation_toggled(GtkWidget *widget, gpointer user_data);
void on_Destination_clicked(GtkWidget *widget, gpointer user_data);
void on_Quit_clicked(GtkWidget *widget, gpointer user_data);
void on_mainwindow_delete_event(GtkWidget *widget, gpointer user_data);


static struct container *
get_container(GtkWidget *widget)
{
	GtkWidget *map_widget;
	GladeXML *xml=glade_get_widget_tree(widget);
	map_widget = glade_xml_get_widget(xml,"map");
	return g_object_get_data(G_OBJECT(map_widget), "navit_container");
}

void on_Zoom_in_clicked(GtkWidget *widget, gpointer user_data)
{
	unsigned long scale;
	struct container *co;

	g_print("Zoom in\n");

	co=get_container(widget);
        graphics_get_view(co, NULL, NULL, &scale);
        scale/=2;
        if (scale < 1)
                scale=1;
        graphics_set_view(co, NULL, NULL, &scale);
}

void on_Zoom_out_clicked(GtkWidget *widget, gpointer user_data)
{
	g_print("Zoom out\n");
}

void on_Refresh_clicked(GtkWidget *widget, gpointer user_data)
{
	g_print("Refresh\n");
}

void on_Cursor_toggled(GtkWidget *widget, gpointer user_data)
{
	g_print("Cursor\n");
}

void  on_Orientation_toggled(GtkWidget *widget, gpointer user_data)
{
	g_print("Orientation\n");
}

void on_Destination_clicked(GtkWidget *widget, gpointer user_data)
{
	g_print("Destination\n");
}

void on_Quit_clicked(GtkWidget *widget, gpointer user_data)
{
	gtk_main_quit ();
}

void on_mainwindow_delete_event(GtkWidget *widget, gpointer user_data)
{
	gtk_main_quit ();
}


static struct container *
container_new(GtkWidget **widget)
{
	struct container *co=g_new0(struct container, 1);
	extern struct map_data *map_data_default;
	struct transformation *t=g_new0(struct transformation, 1);
	struct map_flags *flags=g_new0(struct map_flags, 1);
	struct graphics *gra;

	co->map_data=map_data_default;	
#if 1
	extern struct graphics *graphics_gtk_drawing_area_new(struct container *co, GtkWidget **widget);
	gra=graphics_gtk_drawing_area_new(co, widget);
#else
	extern struct graphics *graphics_gtk_gl_area_new(struct container *co, GtkWidget **widget);
	gra=graphics_gtk_gl_area_new(co, widget);
#endif
	co->gra=gra;
	co->trans=t;	
	co->flags=flags;

	return co;
}

static void glade_signal_connect (const gchar *handler_name, GObject *object, const gchar *signal_name, const gchar *signal_data,
		GObject *connect_object, gboolean after, gpointer user_data)
{
	g_printf("signal connect handler_name: %s signal_name: %s signal_data: %s\n",
			handler_name?handler_name:"NULL",
			signal_name?signal_name:"NULL",
			signal_data?signal_data:"NULL");
#if 0
	if(after)
		g_signal_connect_after(G_OBJECT(object),signal_name,G_CALLBACK(),user_data);
	else
		g_signal_connect(G_OBJECT(object),signal_name,G_CALLBACK()user_data);
#endif
}

struct container *
gui_gtk_window(int x, int y, int scale)
{
	GladeXML *window;
	GtkWidget *map_widget=NULL;
	struct container *co;


	window = glade_xml_new("navit.glade", NULL, NULL);


	map_widget = glade_xml_get_widget(window,"map");
	co=container_new(&map_widget);
	g_object_set_data(G_OBJECT(map_widget), "navit_container", co);
	transform_setup(co->trans, x, y, scale, 0);

	co->win=(struct window *) window;
	co->statusbar=gui_gtk_statusbar_new(window);


	/* connect all signals */
	glade_xml_signal_autoconnect(window);

	container_init_gra(co);
	return co;
#if 0
	GtkWidget *window,*map_widget;
	GtkWidget *vbox;
	GtkWidget *statusbar;
	struct container *co;

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), 792, 547);
	gtk_window_set_title(GTK_WINDOW(window), "Map");
	gtk_widget_realize(window);
	vbox = gtk_vbox_new(FALSE, 0);
	co=container_new(&map_widget);
	
	transform_setup(co->trans, x, y, scale, 0);

	co->win=(struct window *) window;
	co->statusbar=gui_gtk_statusbar_new(&statusbar);
	gui_gtk_actions_new(co,&vbox);
	
/*
	gtk_box_pack_start(GTK_BOX(vbox), menu, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);
*/
	gtk_box_pack_end(GTK_BOX(vbox), statusbar, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), map_widget, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(window), vbox);
	
	gtk_widget_show_all(window);
	container_init_gra(co);
	return co;
#endif
}

