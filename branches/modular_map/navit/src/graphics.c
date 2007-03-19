#include <glib.h>
#include <stdio.h>
#include "string.h"
#include "draw_info.h"
#include "graphics.h"
#include "map_data.h"
#include "coord.h"
#include "param.h"	/* FIXME */
#include "transform.h"
#include "container.h"
#include "plugin.h"
#include "data_window.h"
#include "profile.h"
#include "mapset.h"


#include "layout.h"

struct graphics *
graphics_new(char *type)
{
	struct graphics *this;
	struct graphics_priv * (*new)(struct graphics_methods *meth);

	new=plugin_get_graphics_type(type);
	if (! new)
		return NULL;	
	this=g_new0(struct graphics, 1);
	this->priv=(*new)(&this->meth);
	printf("this=%p\n", this);
	return this;
}


void
graphics_init(struct graphics *this)
{
	this->font[0]=graphics_font_new(this, 140);
	this->font[1]=graphics_font_new(this, 200);
	this->font[2]=graphics_font_new(this, 300);
	this->gc[0]=graphics_gc_new(this);
	graphics_gc_set_background(this->gc[0], &(struct color) { 0xffff, 0xefef, 0xb7b7 });
	graphics_gc_set_foreground(this->gc[0], &(struct color) { 0xffff, 0xefef, 0xb7b7 });
	this->gc[1]=graphics_gc_new(this);
	graphics_gc_set_background(this->gc[1], &(struct color) { 0x0000, 0x0000, 0x0000 });
	graphics_gc_set_foreground(this->gc[1], &(struct color) { 0xffff, 0xffff, 0xffff });
	this->gc[2]=graphics_gc_new(this);
	graphics_gc_set_background(this->gc[2], &(struct color) { 0xffff, 0xffff, 0xffff });
	graphics_gc_set_foreground(this->gc[2], &(struct color) { 0xffff, 0xffff, 0xffff });
	this->meth.background_gc(this->priv, this->gc[0]->priv);
}

void *
graphics_get_data(struct graphics *this, char *type)
{
	printf("this=%p\n", this);
	printf("graphics_get_data %p\n", this->meth.get_data);
	return (this->meth.get_data(this->priv, type));
}

void
graphics_register_resize_callback(struct graphics *this, void (*callback)(void *data, int w, int h), void *data)
{
	this->meth.register_resize_callback(this->priv, callback, data);
}

struct graphics_font *
graphics_font_new(struct graphics *gra, int size)
{
	struct graphics_font *this;

	this=g_new0(struct graphics_font,1);
	this->priv=gra->meth.font_new(gra->priv, &this->meth, size);
	return this;
}

struct graphics_gc *
graphics_gc_new(struct graphics *gra)
{
	struct graphics_gc *this;

	this=g_new0(struct graphics_gc,1);
	this->priv=gra->meth.gc_new(gra->priv, &this->meth);
	return this;
}

void
graphics_gc_set_foreground(struct graphics_gc *gc, struct color *c)
{
	gc->meth.gc_set_foreground(gc->priv, c);
}

void
graphics_gc_set_background(struct graphics_gc *gc, struct color *c)
{
	gc->meth.gc_set_background(gc->priv, c);
}

struct graphics_image *
graphics_image_new(struct graphics *gra, char *path)
{
	struct graphics_image *this;

	this=g_new0(struct graphics_image,1);
	this->priv=gra->meth.image_new(gra->priv, &this->meth, path, &this->width, &this->height);
	return this;
}

#if 0

void
graphics_get_view(struct container *co, long *x, long *y, unsigned long *scale)
{
	struct transformation *t=co->trans;
	if (x) *x=t->center.x;
	if (y) *y=t->center.y;
	if (scale) *scale=t->scale;
}

void
graphics_set_view(struct container *co, long *x, long *y, unsigned long *scale)
{
	struct transformation *t=co->trans;
	if (x) t->center.x=*x;
	if (y) t->center.y=*y;
	if (scale) t->scale=*scale;
	graphics_redraw(co);
}

#endif

#include "attr.h"
#include "popup.h"
#include <stdio.h>

#if 0
static void
popup_view_html(struct popup_item *item, char *file)
{
	char command[1024];
	sprintf(command,"firefox %s", file);
	system(command);
}

static void
graphics_popup(struct display_list *list, struct popup_item **popup)
{
	struct item *item;
	struct attr attr;
	struct map_rect *mr;
	struct coord c;
	struct popup_item *curr_item,*last=NULL;
	item=list->data;
	mr=map_rect_new(item->map, NULL, NULL, 0);
	printf("id hi=0x%x lo=0x%x\n", item->id_hi, item->id_lo);
	item=map_rect_get_item_byid(mr, item->id_hi, item->id_lo);
	if (item) {
		if (item_attr_get(item, attr_name, &attr)) {
			curr_item=popup_item_new_text(popup,attr.u.str,1);
			if (item_attr_get(item, attr_info_html, &attr)) {
				popup_item_new_func(&last,"HTML Info",1, popup_view_html, g_strdup(attr.u.str));
			}
			if (item_attr_get(item, attr_price_html, &attr)) {
				popup_item_new_func(&last,"HTML Preis",2, popup_view_html, g_strdup(attr.u.str));
			}
			curr_item->submenu=last;
		}
	}
	map_rect_destroy(mr);
}
#endif

struct display_item {
	struct item item;
	char *label;
	int count;
	int displayed;
	struct point pnt[0];
};

static int
xdisplay_free_list(gpointer key, gpointer value, gpointer user_data)
{
	GList *h, *l;
	h=value;
	l=h;
	while (l) {
#if 0
		struct display_item *di=l->data;
		if (! di->displayed) 
			printf("warning: item '%s' not displayed\n", item_to_name(di->item.type));
#endif
		g_free(l->data);
		l=g_list_next(l);
	}
	g_list_free(h);
	return TRUE;
}

static void
xdisplay_free(GHashTable *display_list)
{
	g_hash_table_foreach_remove(display_list, xdisplay_free_list, NULL);
}

static void
xdisplay_add(GHashTable *display_list, struct item *item, int count, struct point *pnt, char *label)
{
	struct display_item *di;
	int len;
	GList *l;
	char *p;


	len=sizeof(*di)+count*sizeof(*pnt);
	if (label)
		len+=strlen(label)+1;

	p=g_malloc(len);

	di=(struct display_item *)p;
	p+=sizeof(*di)+count*sizeof(*pnt);
	di->item=*item;
	if (label) {
		di->label=p;
		strcpy(di->label, label);
	} else 
		di->label=NULL;
	di->count=count;
	memcpy(di->pnt, pnt, count*sizeof(*pnt));

	l=g_hash_table_lookup(display_list, GINT_TO_POINTER(item->type));
	l=g_list_prepend(l, di);
	g_hash_table_insert(display_list, GINT_TO_POINTER(item->type), l);
}

static void
xdisplay_draw_elements(struct graphics *gra, GList *es, GList *ls)
{
	struct element *e;
	GList *l;
	struct graphics_gc *gc;
	struct graphics_image *img;
	struct point p;

	while (es) {
		e=es->data;
		l=ls;
		gc=NULL;
		img=NULL;
		while (l) {
			struct display_item *di;
			di=l->data;
			di->displayed=1;
			if (! gc) {
				gc=graphics_gc_new(gra);
				gc->meth.gc_set_foreground(gc->priv, &e->color);
			}
			switch (e->type) {
			case element_polygon:
				gra->meth.draw_polygon(gra->priv, gc->priv, di->pnt, di->count);
				break;
			case element_polyline:
				if (e->u.polyline.width > 1) 
					gc->meth.gc_set_linewidth(gc->priv, e->u.polyline.width);
				gra->meth.draw_lines(gra->priv, gc->priv, di->pnt, di->count);
				break;
			case element_circle:
				if (e->u.circle.width > 1) 
					gc->meth.gc_set_linewidth(gc->priv, e->u.polyline.width);
				gra->meth.draw_circle(gra->priv, gc->priv, &di->pnt[0], e->u.circle.radius);
				p.x=di->pnt[0].x+3;
				p.y=di->pnt[0].y+10;
				gra->meth.draw_text(gra->priv, gra->gc[2]->priv, gra->gc[1]->priv, gra->font[e->label_size]->priv, di->label, &p, 0x10000, 0);
				break;
			case element_icon:
				if (!img) {
					img=graphics_image_new(gra, e->u.icon.src);
				}
				p.x=di->pnt[0].x - img->width/2;
				p.y=di->pnt[0].y - img->height/2;
				gra->meth.draw_image(gra->priv, gra->gc[0]->priv, &p, img->priv);
				break;
			default:
				printf("Unhandled element type %d\n", e->type);
			
			}
			l=g_list_next(l);
		}
		es=g_list_next(es);
	}	
}

static void
xdisplay_draw_layer(GHashTable *display_list, struct graphics *gra, struct layer *lay, int order)
{
	GList *itms;
	GList *types;
	struct itemtype *itm;
	enum item_type type;

	itms=lay->itemtypes;
	while (itms) {
		itm=itms->data;
		if (order >= itm->zoom_min && order <= itm->zoom_max) {
			types=itm->type;
			while (types) {
				type=GPOINTER_TO_INT(types->data);
				xdisplay_draw_elements(gra, itm->elements, g_hash_table_lookup(display_list, GINT_TO_POINTER(type)));
				types=g_list_next(types);
			}
		}
		itms=g_list_next(itms);
	}
}

static void
xdisplay_draw_layout(GHashTable *display_list, struct graphics *gra, struct layout *l, int order)
{
	GList *lays;
	struct layer *lay;
	
	lays=l->layers;
	while (lays) {
		lay=lays->data;
		xdisplay_draw_layer(display_list, gra, lay, order);
		lays=g_list_next(lays);
	}
}

static void
xdisplay_draw(GHashTable *display_list, struct graphics *gra, GList *layouts, int order)
{
	struct layout *l;

	while (layouts) {
		l=layouts->data;
		xdisplay_draw_layout(display_list, gra, l, order);
		return;
		layouts=g_list_next(layouts);
	}
}

static void
do_draw_poly(GHashTable *display_list, struct transformation *t, enum projection pro, struct item *item)
{
	struct coord c;
	int max=16384;
	struct point pnt[max];
	struct attr attr;
	struct coord_rect r;
	int count=0;

	while (count < max) {
		if (!item_coord_get(item, &c, 1))
			break;
		if (! count) {
			r.lu=c;
			r.rl=c;
		} else
			coord_rect_extend(&r, &c);
		transform(t, pro, &c, &pnt[count]);
		count++;
			
	}
	g_assert(count < max);
	if (1 || coord_rect_overlap(&t->r, &r)) {
		attr.u.str=NULL;
		item_attr_get(item, attr_label, &attr);
		xdisplay_add(display_list, item, count, pnt, attr.u.str);
	}
}

static void
do_draw_point(GHashTable *display_list, struct transformation *t, enum projection pro, struct item *item)
{
	struct point pnt;
	struct coord c;
	struct attr attr;

	item_coord_get(item, &c, 1);
	if (transform(t, pro, &c, &pnt)) {
		attr.u.str=NULL;
		item_attr_get(item, attr_label, &attr); 
		xdisplay_add(display_list, item, 1, &pnt, attr.u.str);
	}
}

static void
do_draw_item(GHashTable *display_list, struct transformation *t, enum projection pro, struct item *item)
{
	if (item->type < type_line) {
		do_draw_point(display_list, t, pro, item);
	} else {
		do_draw_poly(display_list, t, pro, item);
	}
}

static void
do_draw(GHashTable *display_list, struct transformation *t, GList *mapsets, int order)
{
	struct coord_rect r;
	struct map_rect *mr;
	struct item *item;
	struct mapset *ms;
	struct map *m;
	enum projection pro;
	void *h;

	r=t->r;
	ms=mapsets->data;
	h=mapset_open(ms);
	while ((m=mapset_get(h))) {
		pro=map_projection(m);
		mr=map_rect_new(m, &r, NULL, order);
		while ((item=map_rect_get_item(mr))) {
			do_draw_item(display_list, t, pro, item);
		}
		map_rect_destroy(mr);
	}
	mapset_close(h);
}

void
graphics_draw(struct graphics *gra, GHashTable *display_list, GList *mapsets, struct transformation *trans, GList *layouts)
{
	int scale=transform_get_scale(trans);
	int order=transform_get_order(trans);
	int i;

#if 0
	printf("scale=%d center=0x%x,0x%x mercator scale=%f\n", scale, co->trans->center.x, co->trans->center.y, transform_scale(co->trans->center.y));
#endif
	
	xdisplay_free(display_list);


	gra->meth.draw_mode(gra->priv, draw_mode_begin);
#if 0
	for (i = 0 ; i < data_window_type_end; i++) {
		data_window_begin(co->data_window[i]);	
	}
#endif
	do_draw(display_list, trans, mapsets, order);
	xdisplay_draw(display_list, gra, layouts, order);
  
	gra->meth.draw_mode(gra->priv, draw_mode_end);
#if 0
	for (i = 0 ; i < data_window_type_end; i++) {
		data_window_end(co->data_window[i]);	
	}
#endif
	/* co->ready=1; */
}
