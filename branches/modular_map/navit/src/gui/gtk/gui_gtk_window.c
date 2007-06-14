#include <stdio.h>
#include <gtk/gtk.h>
#include "navit.h"
#include "gui.h"
#include "coord.h"
#include "plugin.h"
#include "graphics.h"
#include "gui_gtk.h"


static int
gui_gtk_set_graphics(struct gui_priv *this, struct graphics *gra)
{
	GtkWidget *graphics;

	graphics=graphics_get_data(gra, "gtk_widget");
	if (! graphics)
		return 1;
	gtk_box_pack_end(GTK_BOX(this->vbox), graphics, TRUE, TRUE, 0);
	gtk_widget_show_all(graphics);

	return 0;
}

struct gui_methods gui_gtk_methods = {
	gui_gtk_menubar_new,
	gui_gtk_toolbar_new,
	gui_gtk_statusbar_new,
	gui_gtk_popup_new,
	gui_gtk_set_graphics,
};

static struct gui_priv *
gui_gtk_new(struct gui_methods *meth, int w, int h) 
{
	struct gui_priv *this;

	*meth=gui_gtk_methods;

	this=g_new0(struct gui_priv, 1);
	this->win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	this->vbox = gtk_vbox_new(FALSE, 0);
	gtk_window_set_default_size(GTK_WINDOW(this->win), w, h);
	gtk_window_set_title(GTK_WINDOW(this->win), "Navit");
	gtk_widget_realize(this->win);
	gtk_container_add(GTK_CONTAINER(this->win), this->vbox);
	gtk_widget_show_all(this->win);

	return this;
}

void
plugin_init(void)
{
	plugin_register_gui_type("gtk", gui_gtk_new);
}
