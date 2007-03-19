#include <string.h>
#include <gtk/gtk.h>
#include "graphics.h"
#include "gui_gtk.h"
#include "container.h"
#include "menu.h"
#include "data_window.h"
#include "coord.h"
#include "destination.h"

struct menu_priv {
	
};

struct action_gui {
	GtkUIManager        *menu_manager;
	struct container *co;
};

#include "action.h"


/* Create callbacks that implement our Actions */

static void
zoom_in_action(GtkWidget *w, struct container *co)
{
	navit_zoom_in(co, 2);
}

static void
zoom_out_action(GtkWidget *w, struct container *co)
{
	navit_zoom_out(co, 2);
}

static void
refresh_action(GtkWidget *w, struct action *ac)
{
	menu_route_update(ac->gui->co);
}

static void
cursor_action(GtkWidget *w, struct action *ac)
{
	ac->gui->co->flags->track=gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(w));
}

static void
orient_north_action(GtkWidget *w, struct action *ac)
{
	ac->gui->co->flags->orient_north=gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(w));
}

static void
destination_action(GtkWidget *w, struct action *ac)
{
#if 0 /* FIXME */
	destination_address(ac->gui->co);
#endif
}


static void     
quit_action (GtkWidget *w, struct action *ac)
{
	gtk_main_quit();
}

static void
visible_blocks_action(GtkWidget *w, struct container *co)
{
#if 0
	co->data_window[data_window_type_block]=data_window("Visible Blocks",co->win,NULL);
	graphics_redraw(co);
#endif
}

static void
visible_towns_action(GtkWidget *w, struct container *co)
{
#if 0
	co->data_window[data_window_type_town]=data_window("Visible Towns",co->win,NULL);
	graphics_redraw(co);
#endif
}

static void
visible_polys_action(GtkWidget *w, struct container *co)
{
#if 0
	co->data_window[data_window_type_street]=data_window("Visible Polys",co->win,NULL);
	graphics_redraw(co);
#endif
}

static void
visible_streets_action(GtkWidget *w, struct container *co)
{
#if 0
	co->data_window[data_window_type_street]=data_window("Visible Streets",co->win,NULL);
	graphics_redraw(co);
#endif
}

static void
visible_points_action(GtkWidget *w, struct container *co)
{
#if 0
	co->data_window[data_window_type_point]=data_window("Visible Points",co->win,NULL);
	graphics_redraw(co);
#endif
}


static GtkActionEntry entries[] = 
{
	{ "DisplayMenuAction", NULL, "Display" },
	{ "RouteMenuAction", NULL, "Route" },
	{ "MapMenuAction", NULL, "Map" },
	{ "LayoutMenuAction", NULL, "Layout" },
	{ "ZoomOutAction", GTK_STOCK_ZOOM_OUT, "ZoomOut", NULL, NULL, G_CALLBACK(zoom_out_action) },
	{ "ZoomInAction", GTK_STOCK_ZOOM_IN, "ZoomIn", NULL, NULL, G_CALLBACK(zoom_in_action) },
	{ "RefreshAction", GTK_STOCK_REFRESH, "Refresh", NULL, NULL, G_CALLBACK(refresh_action) },
	{ "DestinationAction", "flag_icon", "Destination", NULL, NULL, G_CALLBACK(destination_action) },
	{ "Test", NULL, "Test", NULL, NULL, G_CALLBACK(destination_action) },
	{ "QuitAction", GTK_STOCK_QUIT, "_Quit", "<control>Q",NULL, G_CALLBACK (quit_action) }
};

static guint n_entries = G_N_ELEMENTS (entries);

static GtkToggleActionEntry toggleentries[] = 
{
	{ "CursorAction", "cursor_icon","Cursor", NULL, NULL, G_CALLBACK(cursor_action),TRUE },
	{ "OrientationAction", "orientation_icon", "Orientation", NULL, NULL, G_CALLBACK(orient_north_action),FALSE }
};

static guint n_toggleentries = G_N_ELEMENTS (toggleentries);

static GtkActionEntry debug_entries[] = 
{
	{ "DataMenuAction", NULL, "Data" },
	{ "VisibleBlocksAction", NULL, "VisibleBlocks", NULL, NULL, G_CALLBACK(visible_blocks_action) },
	{ "VisibleTownsAction", NULL, "VisibleTowns", NULL, NULL, G_CALLBACK(visible_towns_action) },
	{ "VisiblePolysAction", NULL, "VisiblePolys", NULL, NULL, G_CALLBACK(visible_polys_action) },
	{ "VisibleStreetsAction", NULL, "VisibleStreets", NULL, NULL, G_CALLBACK(visible_streets_action) },
	{ "VisiblePointsAction", NULL, "VisiblePoints", NULL, NULL, G_CALLBACK(visible_points_action) }
};

static guint n_debug_entries = G_N_ELEMENTS (debug_entries);


static const char * cursor_xpm[] = {
"22 22 2 1",
" 	c None",
".	c #0000FF",
"                      ",
"                      ",
"                      ",
"          ..          ",
"        ..  ..        ",
"      ..      ..      ",
"     .          .     ",
"     .          .     ",
"    .        ... .    ",
"    .     ... .  .    ",
"   .   ...   .    .   ",
"   . ..     .     .   ",
"    .      .     .    ",
"    .     .      .    ",
"     .   .      .     ",
"     .  .       .     ",
"      ..      ..      ",
"        ..  ..        ",
"          ..          ",
"                      ",
"                      ",
"                      "};


static const char * north_xpm[] = {
"22 22 2 1",
" 	c None",
".	c #000000",
"                      ",
"                      ",
"           .          ",
"          ...         ",
"         . . .        ",
"        .  .  .       ",
"           .          ",
"     ....  .  ....    ",
"     ....  .  ....    ",
"      .... .   ..     ",
"      .. ..    ..     ",
"      ..  ..   ..     ",
"      ..   ..  ..     ",
"      ..    .. ..     ",
"      ..   . ....     ",
"     ....  .  ....    ",
"     ....  .  ....    ",
"           .          ",
"           .          ",
"           .          ",
"                      ",
"                      "};


static const char * flag_xpm[] = {
"22 22 2 1",
" 	c None",
"+	c #000000",
"+++++++               ",
"+   +++++++++         ",
"+   +++   +++++++++   ",
"+   +++   +++   +++   ",
"++++      +++   +++   ",
"++++   +++      +++   ",
"++++   +++   +++  +   ",
"+   ++++++   +++  +   ",
"+   +++   ++++++  +   ",
"+   +++   +++   +++   ",
"++++      +++   +++   ",
"++++   +++      +++   ",
"++++++++++   +++  +   ",
"+      +++++++++  +   ",
"+            ++++++   ",
"+                     ",
"+                     ",
"+                     ",
"+                     ",
"+                     ",
"+                     ",
"+                     "};



static struct {
	gchar *stockid;
	const char **icon_xpm;
} stock_icons[] = {
	{"cursor_icon", cursor_xpm },
	{"orientation_icon", north_xpm },
	{"flag_icon", flag_xpm }
};


static gint n_stock_icons = G_N_ELEMENTS (stock_icons);


static void
register_my_stock_icons (void)
{
	GtkIconFactory *icon_factory;
	GtkIconSet *icon_set; 
	GdkPixbuf *pixbuf;
	gint i;

	icon_factory = gtk_icon_factory_new ();

	for (i = 0; i < n_stock_icons; i++) 
	{
		pixbuf = gdk_pixbuf_new_from_xpm_data(stock_icons[i].icon_xpm);
		icon_set = gtk_icon_set_new_from_pixbuf (pixbuf);
		g_object_unref(pixbuf);
		gtk_icon_factory_add (icon_factory, stock_icons[i].stockid, icon_set);
		gtk_icon_set_unref (icon_set);
	}

	gtk_icon_factory_add_default(icon_factory); 

	g_object_unref(icon_factory);
}


static char layout[] =
	"<ui>\
		<menubar name=\"MenuBar\">\
			<menu name=\"DisplayMenu\" action=\"DisplayMenuAction\">\
				<menuitem name=\"Zoom in\" action=\"ZoomInAction\" />\
				<menuitem name=\"Zoom out\" action=\"ZoomOutAction\" />\
				<menuitem name=\"Cursor\" action=\"CursorAction\"/>\
				<menuitem name=\"Orientation\" action=\"OrientationAction\"/>\
				<menuitem name=\"Quit\" action=\"QuitAction\" />\
				<placeholder name=\"RouteMenuAdditions\" />\
			</menu>\
			<menu name=\"DataMenu\" action=\"DataMenuAction\">\
				<menuitem name=\"Visible Blocks\" action=\"VisibleBlocksAction\" />\
				<menuitem name=\"Visible Towns\" action=\"VisibleTownsAction\" />\
				<menuitem name=\"Visible Polys\" action=\"VisiblePolysAction\" />\
				<menuitem name=\"Visible Streets\" action=\"VisibleStreetsAction\" />\
				<menuitem name=\"Visible Points\" action=\"VisiblePointsAction\" />\
				<placeholder name=\"DataMenuAdditions\" />\
			</menu>\
			<menu name=\"RouteMenu\" action=\"RouteMenuAction\">\
				<menuitem name=\"Refresh\" action=\"RefreshAction\" />\
				<menuitem name=\"Destination\" action=\"DestinationAction\" />\
				<placeholder name=\"RouteMenuAdditions\" />\
			</menu>\
			<menu name=\"MapMenu\" action=\"MapMenuAction\">\
			</menu>\
			<menu name=\"LayoutMenu\" action=\"LayoutMenuAction\">\
			</menu>\
		</menubar>\
	 	<toolbar name=\"ToolBar\" action=\"BaseToolbar\" action=\"BaseToolbarAction\">\
			<placeholder name=\"ToolItems\">\
				<separator/>\
				<toolitem name=\"Zoom in\" action=\"ZoomInAction\"/>\
				<toolitem name=\"Zoom out\" action=\"ZoomOutAction\"/>\
				<toolitem name=\"Refresh\" action=\"RefreshAction\"/>\
				<toolitem name=\"Cursor\" action=\"CursorAction\"/>\
				<toolitem name=\"Orientation\" action=\"OrientationAction\"/>\
				<toolitem name=\"Destination\" action=\"DestinationAction\"/>\
				<toolitem name=\"Quit\" action=\"QuitAction\"/>\
				<separator/>\
			</placeholder>\
		</toolbar>\
	</ui>";
			
void *
gui_add_menu(GtkUIManager *menu_manager, char *name)
{
#if 0
	GtkAction *action;
#if 0
	GtkWidget *item;
	GtkWidget *mi;
	GtkWidget *me=gtk_menu_new();

	item = gtk_ui_manager_get_widget( ac->gui->menu_manager, "/ui/MenuBar" );
	mi=gtk_menu_item_new_with_label(name);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(mi), me);
	gtk_menu_bar_append(item, mi);

	return me;
#endif
	action=gtk_action_new(name, name, NULL, NULL);
	gtk_action_group_add_action(base_group, action);
	gtk_ui_manager_add_ui( menu_manager, gtk_ui_manager_new_merge_id(menu_manager), "/ui/MenuBar/MapMenu", name, name, GTK_UI_MANAGER_MENUITEM, FALSE);
#endif
		
}

static void
box_add_widget (struct gui_priv *this, char *path, struct container *co)
{
	GError *error;
	GtkWidget *widget;

	if (! this->menu_manager) {
		this->base_group = gtk_action_group_new ("BaseActions");
		this->debug_group = gtk_action_group_new ("DebugActions");
		this->dyn_group = gtk_action_group_new ("DynamicActions");
		register_my_stock_icons();
		this->menu_manager = gtk_ui_manager_new ();
		gtk_action_group_add_actions (this->base_group, entries, n_entries, co);
		gtk_action_group_add_toggle_actions (this->base_group, toggleentries, n_toggleentries, co);
		gtk_ui_manager_insert_action_group (this->menu_manager, this->base_group, 0);
		gtk_action_group_add_actions (this->debug_group, debug_entries, n_debug_entries, co);
		gtk_ui_manager_insert_action_group (this->menu_manager, this->debug_group, 0);
		gtk_ui_manager_add_ui_from_string (this->menu_manager, layout, strlen(layout), &error);
		error=NULL;
		if (error) {
			g_message ("building menus failed: %s", error->message);
			g_error_free (error);
		}
	}
	widget=gtk_ui_manager_get_widget(this->menu_manager, path);
	gtk_box_pack_start (this->vbox, widget, FALSE, FALSE, 0);
	gtk_widget_show (widget);
}

struct menu_priv *
gui_gtk_toolbar_new(struct gui_priv *this, struct menu_methods *meth, struct container *co)
{
	box_add_widget(this, "/ui/ToolBar", co);
#if 0
	strcpy(buffer, "Test");
	gui_add_menu(menu_manager, buffer);
	strcpy(buffer, "Test1");
	gui_add_menu(menu_manager, buffer);
	strcpy(buffer, "Test2");
	gui_add_menu(menu_manager, buffer);
#endif
}

struct menu_priv *
gui_gtk_menubar_new(struct gui_priv *this, struct menu_methods *meth, struct container *co)
{
	box_add_widget(this, "/ui/MenuBar", co);
}

