#include <stdio.h>
#include <gtk/gtk.h>
#include "gui.h"
#include "coord.h"
#include "transform.h"
#include "container.h"
#include "plugin.h"
#include "gui_gtk.h"


extern void container_init_gra(struct container *co);

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

#if 0
struct container *
gui_gtk_window(int x, int y, int scale)
{
	GtkWidget *window,*map_widget;
	GtkWidget *vbox;
	GtkWidget *toolbar, *menubar, *statusbar;
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
#if 0
	co->action=gui_gtk_actions_new(co,vbox);
#endif
	
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
}
#endif

void
gui_gtk_set_graphics(struct gui_priv *this, struct graphics *gra)
{
	GtkWidget *graphics;

	graphics=graphics_get_data(gra, "gtk_widget");
	gtk_box_pack_end(this->vbox, graphics, TRUE, TRUE, 0);
	gtk_widget_show_all(graphics);
}

struct gui_methods gui_gtk_methods = {
	gui_gtk_menubar_new,
	gui_gtk_toolbar_new,
	gui_gtk_statusbar_new,
	gui_gtk_set_graphics,
};

struct gui_priv *
gui_gtk_new(struct gui_methods *meth, int w, int h) 
{
	struct gui_priv *this;
	GtkWidget *toolbar, *menubar, *statusbar;

	*meth=gui_gtk_methods;

	this=g_new0(struct gui_priv, 1);
	this->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(this->win, w, h);
	gtk_window_set_title(this->win, "Navit");
	gtk_widget_realize(this->win);
#if 0
	gui_gtk_menubar_new();
#endif
#if 0
	this->statusbar=gui_gtk_statusbar_new(&statusbar);
#endif
	this->vbox = gtk_vbox_new(FALSE, 0);
#if 0
	gtk_box_pack_end(this->vbox, statusbar, FALSE, FALSE, 0);
#endif

	gtk_container_add(this->win, this->vbox);
	gtk_widget_show_all(this->win);

	return this;
}

void
plugin_init(void)
{
	plugin_register_gui_type("gtk", gui_gtk_new);
}
