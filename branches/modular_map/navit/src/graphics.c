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

GHashTable *ht;

void
container_init_gra(struct container *co)
{
	struct graphics *gra=co->gra;

	gra->font=g_new0(struct graphics_font *,3);
	gra->font[0]=gra->font_new(gra,140);
	gra->font[1]=gra->font_new(gra,200);
	gra->font[2]=gra->font_new(gra,300);
	gra->gc=g_new0(struct graphics_gc *, 3);
	gra->gc[0]=gra->gc_new(gra);
	gra->gc_set_background(gra->gc[0], 0xffff, 0xefef, 0xb7b7);
	gra->gc_set_foreground(gra->gc[0], 0xffff, 0xefef, 0xb7b7);
	gra->gc[1]=gra->gc_new(gra);
	gra->gc_set_background(gra->gc[1], 0x0000, 0x0000, 0x0000);
	gra->gc_set_foreground(gra->gc[1], 0xffff, 0xffff, 0xffff);
	gra->gc[2]=gra->gc_new(gra);
	gra->gc_set_background(gra->gc[2], 0xffff, 0x0000, 0x0000);
	gra->gc_set_foreground(gra->gc[2], 0xffff, 0x0000, 0x0000);
	ht=g_hash_table_new(NULL,NULL);
	
}

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
		struct display_item *di=l->data;
#if 0
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
xdisplay_free(void)
{
	g_hash_table_foreach_remove(ht, xdisplay_free_list, NULL);
}

static void
xdisplay_add(struct item *item, int count, struct point *pnt, char *label)
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

	l=g_hash_table_lookup(ht, GINT_TO_POINTER(item->type));
	l=g_list_prepend(l, di);
	g_hash_table_insert(ht, GINT_TO_POINTER(item->type), l);
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
				gc=gra->gc_new(gra);
				gra->gc_set_foreground(gc, e->color.r,e->color.g, e->color.b);
			}
			switch (e->type) {
			case element_polygon:
				gra->draw_polygon(gra, gc, di->pnt, di->count);
				break;
			case element_polyline:
				if (e->u.polyline.width > 1) 
					gra->gc_set_linewidth(gc, e->u.polyline.width);
				gra->draw_lines(gra, gc, di->pnt, di->count);
				break;
			case element_circle:
				if (e->u.circle.width > 1) 
					gra->gc_set_linewidth(gc, e->u.polyline.width);
				gra->draw_circle(gra, gc, &di->pnt[0], e->u.circle.radius);
				p.x=di->pnt[0].x+3;
				p.y=di->pnt[0].y+10;
				gra->draw_text(gra, gra->gc[2], gra->gc[1], gra->font[e->label_size], di->label, &p, 0x10000, 0);
				break;
			case element_icon:
				if (!img) {
					img=gra->image_new(gra, e->u.icon.src);
				}
				p.x=di->pnt[0].x - img->width/2;
				p.y=di->pnt[0].y - img->height/2;
				gra->draw_image(gra, gra->gc[0], &p, img);
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
xdisplay_draw_layer(struct graphics *gra, struct layer *lay, int order)
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
				xdisplay_draw_elements(gra, itm->elements, g_hash_table_lookup(ht, GINT_TO_POINTER(type)));
				types=g_list_next(types);
			}
		}
		itms=g_list_next(itms);
	}
}

static void
xdisplay_draw_layout(struct graphics *gra, struct layout *l, int order)
{
	GList *lays;
	struct layer *lay;
	
	lays=l->layers;
	while (lays) {
		lay=lays->data;
		xdisplay_draw_layer(gra, lay, order);
		lays=g_list_next(lays);
	}
}

static void
xdisplay_draw(struct graphics *gra, GList *layouts, int order)
{
	struct layout *l;

	while (layouts) {
		l=layouts->data;
		xdisplay_draw_layout(gra, l, order);
		return;
		layouts=g_list_next(layouts);
	}
}

static void
do_draw_poly(struct transformation *t, enum projection pro, struct item *item)
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
		xdisplay_add(item, count, pnt, attr.u.str);
	}
}

static void
do_draw_point(struct transformation *t, enum projection pro, struct item *item)
{
	struct point pnt;
	struct coord c;
	struct attr attr;

	item_coord_get(item, &c, 1);
	if (transform(t, pro, &c, &pnt)) {
		attr.u.str=NULL;
		item_attr_get(item, attr_label, &attr); 
		xdisplay_add(item, 1, &pnt, attr.u.str);
	}
}

static void
do_draw_item(struct transformation *t, enum projection pro, struct item *item)
{
	if (item->type < type_line) {
		do_draw_point(t, pro, item);
	} else {
		do_draw_poly(t, pro, item);
	}
}

static void
do_draw(struct container *co, int order)
{
	struct coord_rect r;
	struct map_rect *mr;
	struct item *item;
	struct mapset *ms;
	struct map *m;
	struct transformation *t=co->trans;
	enum projection pro;
	void *h;

	printf("co=%p\n", co);	
	r=co->trans->r;
	ms=co->mapsets->data;
	h=mapset_open(ms);
	while ((m=mapset_get(h))) {
		pro=map_projection(m);
		mr=map_rect_new(m, &r, NULL, order);
		while ((item=map_rect_get_item(mr))) {
			do_draw_item(t, pro, item);
		}
		map_rect_destroy(mr);
	}
	mapset_close(h);
}

void
graphics_redraw(struct container *co)
{
	int scale=transform_get_scale(co->trans);
	int order=transform_get_order(co->trans);
	struct graphics *gra=co->gra;
	int i;

#if 1
	printf("scale=%d center=0x%x,0x%x mercator scale=%f\n", scale, co->trans->center.x, co->trans->center.y, transform_scale(co->trans->center.y));
#endif
	
	xdisplay_free();

	transform_setup_source_rect(co->trans);

	gra->draw_mode(gra, draw_mode_begin);
	for (i = 0 ; i < data_window_type_end; i++) {
		data_window_begin(co->data_window[i]);	
	}
	do_draw(co, order);
	xdisplay_draw(co->gra, co->layouts, order);
  
	gra->draw_mode(gra, draw_mode_end);
	for (i = 0 ; i < data_window_type_end; i++) {
		data_window_end(co->data_window[i]);	
	}
	co->ready=1;
}

void
graphics_resize(struct container *co, int w, int h)
{
	co->trans->width=w;
        co->trans->height=h;
	graphics_redraw(co);
}
