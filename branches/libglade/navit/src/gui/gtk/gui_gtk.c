#include <stdio.h>
#include <gtk/gtk.h>
#include <glib/gprintf.h>
#include <glade/glade.h>
#include "coord.h"
#include "graphics.h"
#include "transform.h"
#include "container.h"
#include "gui_gtk.h"
#include "menu.h"
#include "destination.h"


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

	co=get_container(widget);
        graphics_get_view(co, NULL, NULL, &scale);
        scale/=2;
        if (scale < 1)
                scale=1;
        graphics_set_view(co, NULL, NULL, &scale);
}

void on_Zoom_out_clicked(GtkWidget *widget, gpointer user_data)
{
	unsigned long scale;
	struct container *co;

	co=get_container(widget);
        graphics_get_view(co, NULL, NULL, &scale);
        scale*=2;
        graphics_set_view(co, NULL, NULL, &scale);
}

void on_Refresh_clicked(GtkWidget *widget, gpointer user_data)
{
	struct container *co;

	co=get_container(widget);
	menu_route_update(co);
}

void on_Cursor_toggled(GtkWidget *widget, gpointer user_data)
{
	struct container *co;

	co=get_container(widget);
	co->flags->track=gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget));
}

void  on_Orientation_toggled(GtkWidget *widget, gpointer user_data)
{
	struct container *co;

	co=get_container(widget);
	co->flags->orient_north=gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget));
}

void on_Destination_clicked(GtkWidget *widget, gpointer user_data)
{
	struct container *co;

	co=get_container(widget);
	destination_address(co);
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
}

