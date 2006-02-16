#include <glib.h>
#include "attr.h"
#include "coord.h"
#include "item.h"

void
item_coord_rewind(struct item *it)
{
	it->meth->item_coord_rewind(it->priv_data);
}

int
item_coord_get(struct item *it, struct coord *c, int count)
{
	return it->meth->item_coord_get(it->priv_data, c, count);
}

void
item_attr_rewind(struct item *it)
{
	it->meth->item_attr_rewind(it->priv_data);
}
int
item_attr_get(struct item *it, enum attr_type attr_type, struct attr *attr)
{
	return it->meth->item_attr_get(it->priv_data, attr_type, attr);
}

struct item * item_new(char *type, int zoom)
{
	struct item * it;

	it = g_new0(struct item, 1);

	/* FIXME evaluate arguments */

	return it;
}
