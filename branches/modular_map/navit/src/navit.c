#include <glib.h>
#include "container.h"
#include "navit.h"
#include "gui.h"
#include "coord.h"
#include "transform.h"
#include "menu.h"
#include "cursor.h"

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
	struct toolbar *toolbar;
	struct statusbar *statusbar;
	struct menu *menubar;
	struct route *route;
	struct cursor *cursor;
	struct speech *speech;
	struct vehicle *vehicle;
	struct track *track;
        struct data_window *data_window[data_window_type_end];
	struct map_flags *flags;
	int ready;
	struct window *win;
	GHashTable *display_list;
};

#define nav xyz[3]

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
	graphics_draw(this->gra, this->display_list, this->mapsets, this->trans, this->layouts);
	this->ready=1;
}

static void
navit_resize(struct navit *this, int w, int h)
{
	transform_set_size(this->trans, w, h);
	navit_draw(this);
}

static void
navit_button(struct navit *this, int pressed, int button, struct point *p)
{
	if (pressed && button == 1) {
		int border=16;
		struct transformation *t=this->trans;
		if (p->x < border || p->x > t->width-border || p->y < border || p->y > t->height-border) {
			transform_reverse(t, p, &t->center);
			navit_draw(this);
		}
	}
}

void
navit_zoom_in(struct navit *this, int factor)
{
	this->trans->scale=this->trans->scale/factor;
	if (this->trans->scale < 1)
		this->trans->scale=1;
	navit_draw(this);
}

void
navit_zoom_out(struct navit *this, int factor)
{
	this->trans->scale=this->trans->scale*factor;
	navit_draw(this);
}

struct navit *
navit_new(char *ui, char *graphics, struct coord_geo *center, double zoom)
{
	struct navit *this=g_new0(struct navit, 1);

	this->trans=g_new0(struct transformation, 1);	

	transform_setup(this->trans, 1300000,7000000,8192,0);
	this->flags=g_new0(struct map_flags, 1);
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

void
navit_init(struct navit *this)
{
	struct menu *map, *layout;
	map=menu_add(this->menubar, "Map", menu_type_submenu, NULL, NULL);
	menu_add(map, "Test", menu_type_menu, NULL, NULL);
	layout=menu_add(this->menubar, "Layout", menu_type_submenu, NULL, NULL);
	menu_add(layout, "Test", menu_type_menu, NULL, NULL);
}

void
navit_vehicle_add(struct navit *this, struct vehicle *v, struct color *c)
{
#if 0
	co->vehicle=v;
	co->cursor=cursor_new(co,co->vehicle,c);
#endif
}


void
navit_destroy(struct navit *this)
{
	g_free(this);
}
