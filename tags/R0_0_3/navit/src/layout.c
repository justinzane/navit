#include <glib.h>
#include <string.h>
#include "layout.h"

struct layout * layout_new(const char *name)
{
	struct layout *l;

	l = g_new0(struct layout, 1);
	l->name = g_strdup(name);
	return l;
}


struct layer * layer_new(const char *name, int details)
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

struct itemtype * itemtype_new(int order_min, int order_max)
{
	struct itemtype *itm;

	itm = g_new0(struct itemtype, 1);
	itm->order_min=order_min;
	itm->order_max=order_max;
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
polygon_new(struct color *color)
{
	struct element *e;
	e = g_new0(struct element, 1);
	e->type=element_polygon;
	e->color=*color;

	return e;
}

struct element *
polyline_new(struct color *color, int width)
{
	struct element *e;
	
	e = g_new0(struct element, 1);
	e->type=element_polyline;
	e->color=*color;
	e->u.polyline.width=width;

	return e;
}

struct element *
circle_new(struct color *color, int radius, int width, int label_size)
{
	struct element *e;
	
	e = g_new0(struct element, 1);
	e->type=element_circle;
	e->color=*color;
	e->label_size=label_size;
	e->u.circle.width=width;
	e->u.circle.radius=radius;

	return e;
}

struct element *
label_new(int label_size)
{
	struct element *e;
	
	e = g_new0(struct element, 1);
	e->type=element_label;
	e->label_size=label_size;

	return e;
}

struct element *
icon_new(const char *src)
{
	struct element *e;

	e = g_malloc0(sizeof(*e)+strlen(src)+1);
	e->type=element_icon;
	e->u.icon.src=(char *)(e+1);
	strcpy(e->u.icon.src,src);

	return e;	
}

struct element *
image_new(void)
{
	struct element *e;

	e = g_malloc0(sizeof(*e));
	e->type=element_image;

	return e;	
}

