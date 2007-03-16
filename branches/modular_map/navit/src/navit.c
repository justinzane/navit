#include <glib.h>
#include "container.h"

struct container *
navit_new(char *ui, struct coord_geo *center, double zoom)
{
	struct container *co;
	co=gui_gtk_window(1300000,7000000,8192);

	return co;
}

void
navit_vehicle_add(struct container *co, struct vehicle *v, struct color *c)
{
	co->vehicle=v;
	co->cursor=cursor_new(co,co->vehicle,c);
}


