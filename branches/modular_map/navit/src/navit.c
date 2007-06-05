#include <glib.h>
#include "navit.h"
#include "gui.h"
#include "map.h"
#include "mapset.h"
#include "coord.h"
#include "transform.h"
#include "projection.h"
#include "menu.h"
#include "graphics.h"
#include "cursor.h"
#include "popup.h"

struct navit {
	GList *mapsets;
	GList *layouts;
	struct gui *gui;
	struct layout *layout_current;
	struct graphics *gra;
	struct action *action;
	struct transformation *trans;
	struct compass *compass;
	struct map_data *map_data;
	struct menu *menu;
	struct menu *toolbar;
	struct statusbar *statusbar;
	struct menu *menubar;
	struct route *route;
	struct cursor *cursor;
	struct speech *speech;
	struct vehicle *vehicle;
	struct track *track;
	struct map_flags *flags;
	int ready;
	struct window *win;
	GHashTable *display_list;
};

void
navit_add_mapset(struct navit *this, struct mapset *ms)
{
	this->mapsets = g_list_append(this->mapsets, ms);
}

void
navit_add_layout(struct navit *this, struct layout *lay)
{
	this->layouts = g_list_append(this->layouts, lay);
}

void
navit_draw(struct navit *this)
{
	transform_setup_source_rect(this->trans);
	graphics_draw(this->gra, this->display_list, this->mapsets, this->trans, this->layouts, this->route);
	this->ready=1;
}

static void
navit_resize(void *data, int w, int h)
{
	struct navit *this=data;
	transform_set_size(this->trans, w, h);
	navit_draw(this);
}

static void
navit_button(void *data, int pressed, int button, struct point *p)
{
	struct navit *this=data;
	if (pressed && button == 1) {
		int border=16;
		struct transformation *t=this->trans;
		struct coord *c;
		if (! transform_within_border(t, p, border)) {
			c=transform_center(t);
			transform_reverse(t, p, c);
			navit_draw(this);
		}
	}
	if (pressed && button == 3)
		popup(this, button, p);
}

void
navit_zoom_in(struct navit *this, int factor)
{
	long scale=transform_get_scale(this->trans)/factor;
	if (scale < 1)
		scale=1;
	transform_set_scale(this->trans, scale);
	navit_draw(this);
}

void
navit_zoom_out(struct navit *this, int factor)
{
	long scale=transform_get_scale(this->trans)*factor;
	transform_set_scale(this->trans,scale);
	navit_draw(this);
}

struct navit *
navit_new(char *ui, char *graphics, struct coord *center, enum projection pro, int zoom)
{
	struct navit *this=g_new0(struct navit, 1);

	this->trans=transform_new();
	transform_set_projection(this->trans, pro);

	transform_setup(this->trans, center, zoom, 0);
	/* this->flags=g_new0(struct map_flags, 1); */
	this->display_list=g_hash_table_new(NULL,NULL);
	this->gui=gui_new(ui, 792, 547);
	if (! this->gui) {
		g_warning("failed to create gui '%s'", ui);
		navit_destroy(this);
		return NULL;
	}
	this->menubar=gui_menubar_new(this->gui, this);
	this->toolbar=gui_toolbar_new(this->gui, this);
	this->statusbar=gui_statusbar_new(this->gui, this);
	this->gra=graphics_new(graphics);
	if (! this->gra) {
		g_warning("failed to create graphics '%s'", graphics);
		navit_destroy(this);
		return NULL;
	}
	graphics_register_resize_callback(this->gra, navit_resize, this);
	graphics_register_button_callback(this->gra, navit_button, this);
	if (gui_set_graphics(this->gui, this->gra)) {
		g_warning("failed to connect graphics '%s' to gui '%s'\n", graphics, ui);
		navit_destroy(this);
		return NULL;
	}
	graphics_init(this->gra);
	return this;
}

static void
navit_map_toggle(struct menu *menu, void *this_p, void *map_p)
{
	if ((menu_get_toggle(menu) != 0) != (map_get_active(map_p) != 0)) {
		map_set_active(map_p, (menu_get_toggle(menu) != 0));
		navit_draw(this_p);
	}
}

static void
navit_projection_set(struct menu *menu, void *this_p, void *pro_p)
{
	struct navit *this=this_p;
	enum projection pro=(enum projection) pro_p;
	struct coord_geo g;
	struct coord *c;

	c=transform_center(this->trans);
	transform_to_geo(transform_get_projection(this->trans), c, &g);
	transform_set_projection(this->trans, pro);
	transform_from_geo(pro, &g, c);
	navit_draw(this);
}

void
navit_init(struct navit *this)
{
	struct menu *men,*men2;
	struct map *map;
	struct mapset_handle *handle;
	struct mapset *ms=this->mapsets->data;
	men=menu_add(this->menubar, "Map", menu_type_submenu, NULL, NULL, NULL);
	handle=mapset_open(ms);
	while ((map=mapset_next(handle,0))) {
		char *s=g_strdup_printf("%s:%s", map_get_type(map), map_get_filename(map));
		men2=menu_add(men, s, menu_type_toggle, navit_map_toggle, this, map);
		menu_set_toggle(men2, map_get_active(map));
		g_free(s);
	}
	mapset_close(handle);
	// menu_add(map, "Test", menu_type_menu, NULL, NULL);
	men=menu_add(this->menubar, "Layout", menu_type_submenu, NULL, NULL, NULL);
	menu_add(men, "Test", menu_type_menu, NULL, NULL, NULL);
	men=menu_add(this->menubar, "Projection", menu_type_submenu, NULL, NULL, NULL);
	menu_add(men, "M&G", menu_type_menu, navit_projection_set, this, (void *)projection_mg);
	menu_add(men, "Garmin", menu_type_menu, navit_projection_set, this, (void *)projection_garmin);
	this->route=route_new(ms);
}

static void
navit_cursor_offscreen(struct cursor *cursor, void *this_p)
{
	struct navit *this=this_p;
	struct coord *c=transform_center(this->trans);
	struct coord *cursor_c=cursor_pos_get(cursor);
	*c=*cursor_c;
	if (graphics_ready(this->gra))
		navit_draw(this);
}

void
navit_vehicle_add(struct navit *this, struct vehicle *v, struct color *c)
{
	this->vehicle=v;
	this->cursor=cursor_new(this->gra, v, c, this->trans);
	cursor_register_offscreen_callback(this->cursor, navit_cursor_offscreen, this);
}


struct gui *
navit_get_gui(struct navit *this)
{
	return this->gui;
}

struct transformation *
navit_get_trans(struct navit *this)
{
	return this->trans;
}

struct route *
navit_get_route(struct navit *this)
{
	return this->route;
}

GHashTable *
navit_get_display_list(struct navit *this)
{
	return this->display_list;
}

void
navit_destroy(struct navit *this)
{
	g_free(this);
}

