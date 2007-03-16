#include <glib.h>
#include <string.h>
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

struct itemtype * itemtype_new(int zoom_min, int zoom_max)
{
	struct itemtype *itm;

	itm = g_new0(struct itemtype, 1);
	itm->zoom_min=zoom_min;
	itm->zoom_max=zoom_max;
	return itm;
}

void itemtype_add_type(struct itemtype *this, enum item_type type)
{
	this->type = g_list_append(this->type, GINT_TO_POINTER(type));
}


void layer_add_itemtype(struct layer *layer, struct itemtype * itemtype)
{
	layer->itemtypes = g_list_append(layer->itemtypes, itemtype);

}

void itemtype_add_element(struct itemtype *itemtype, struct element *element)
{
	itemtype->elements = g_list_append(itemtype->elements, element);
}

struct element *
polygon_new(int *color)
{
	struct element *e;
	e = g_new0(struct element, 1);
	e->type=element_polygon;
	e->color[0]=color[0];
	e->color[1]=color[1];
	e->color[2]=color[2];

	return e;
}

struct element *
polyline_new(int *color, int width)
{
	struct element *e;
	
	e = g_new0(struct element, 1);
	e->type=element_polyline;
	e->color[0]=color[0];
	e->color[1]=color[1];
	e->color[2]=color[2];
	e->u.polyline.width=width;

	return e;
}

struct element *
circle_new(int *color, int radius, int width, int label_size)
{
	struct element *e;
	
	e = g_new0(struct element, 1);
	e->type=element_circle;
	e->color[0]=color[0];
	e->color[1]=color[1];
	e->color[2]=color[2];
	e->label_size=label_size;
	e->u.circle.width=width;
	e->u.circle.radius=radius;

	return e;
}

struct element *
icon_new(char *src)
{
	struct element *e;

	e = g_malloc0(sizeof(*e)+strlen(src)+1);
	e->type=element_icon;
	e->u.icon.src=(char *)(e+1);
	strcpy(e->u.icon.src,src);

	return e;	
}

