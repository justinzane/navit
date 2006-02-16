#include "layout.h"


struct layout * layout_new(char *name)
{
	struct layout *l;

	l = g_new0(struct layout, 1);
	l->name = g_strdup(name);
	return l;
}


struct layer * layer_new(char *name, int details)
{
	struct layer *l;

	l = g_new0(struct layer, 1);
	l->name = g_strdup(name);
	l->details = details;
	return l;
}

void layout_add_layer(struct layout *layout, struct layer *layer)
{
	layout->layers = g_list_append(layout->layers, layer);
}

void layer_add_item(struct layer *layer, struct item * item)
{
	layer->levels = g_list_append(layer->levels, item);
}

