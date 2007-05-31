#include <string.h>
#include <glib.h>
#include "attr.h"
#include "coord.h"
#include "item.h"

struct item_name {
        enum item_type item;
        char *name;
};


struct item_name item_names[]={
#define ITEM2(x,y) ITEM(y)
#define ITEM(x) { type_##x, #x },
#include "item_def.h"
#undef ITEM2
#undef ITEM
};

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

enum item_type
item_from_name(char *name)
{
	int i;

	for (i=0 ; i < sizeof(item_names)/sizeof(struct item_name) ; i++) {
		if (! strcmp(item_names[i].name, name))
			return item_names[i].item;
	}
	return type_none;
}

char *
item_to_name(enum item_type item)
{
	int i;

	for (i=0 ; i < sizeof(item_names)/sizeof(struct item_name) ; i++) {
		if (item_names[i].item == item)
			return item_names[i].name;
	}
	return NULL; 
}
