#include <glib.h>
#include "container.h"
#include "navit.h"
#include "gui.h"
#include "coord.h"
#include "transform.h"
#include "cursor.h"

static void
navit_draw(struct container *co)
{
	transform_setup_source_rect(co->trans);
	graphics_draw(co->gra, co->display_list, co->mapsets, co->trans, co->layouts);
	co->ready=1;
}

static void
navit_resize(struct container *co, int w, int h)
{
	transform_set_size(co->trans, w, h);
	navit_draw(co);
}

void
navit_zoom_in(struct container *co, int factor)
{
	navit_draw(co);
}

void
navit_zoom_out(struct container *co, int factor)
{
	navit_draw(co);
}

struct container *
navit_new(char *ui, char *graphics, struct coord_geo *center, double zoom)
{
	struct container *co=g_new0(struct container, 1);

	co->trans=g_new0(struct transformation, 1);	

	transform_setup(co->trans, 1300000,7000000,8192,0);
	co->flags=g_new0(struct map_flags, 1);
	co->display_list=g_hash_table_new(NULL,NULL);
	co->gui=gui_new(ui, 792, 547);
	if (! co->gui) 
		g_error("failed to create gui '%s'", ui);
	co->menubar=gui_menubar_new(co->gui, co);
	co->toolbar=gui_toolbar_new(co->gui, co);
	co->statusbar=gui_statusbar_new(co->gui, co);
	co->gra=graphics_new(graphics);

	graphics_register_resize_callback(co->gra, navit_resize, co);
	gui_set_graphics(co->gui, co->gra);
	graphics_init(co->gra);

	return co;
}

void
navit_vehicle_add(struct container *co, struct vehicle *v, struct color *c)
{
#if 0
	co->vehicle=v;
	co->cursor=cursor_new(co,co->vehicle,c);
#endif
}

