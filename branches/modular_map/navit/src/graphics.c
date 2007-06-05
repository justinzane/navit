#include <glib.h>
#include <stdio.h>
#include "debug.h"
#include "string.h"
#include "draw_info.h"
#include "graphics.h"
#include "map.h"
#include "coord.h"
#include "param.h"	/* FIXME */
#include "transform.h"
#include "projection.h"
#include "point.h"
#include "plugin.h"
#include "profile.h"
#include "mapset.h"


#include "layout.h"

struct graphics
{
	struct graphics_priv *priv;
	struct graphics_methods meth;
	struct graphics_font *font[16];
	struct graphics_gc *gc[3];
	int ready;
};

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
	return this;
}


void
graphics_init(struct graphics *this)
{
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
	return (this->meth.get_data(this->priv, type));
}

void
graphics_register_resize_callback(struct graphics *this, void (*callback)(void *data, int w, int h), void *data)
{
	this->meth.register_resize_callback(this->priv, callback, data);
}

void
graphics_register_button_callback(struct graphics *this, void (*callback)(void *data, int pressed, int button, struct point *p), void *data)
{
	this->meth.register_button_callback(this->priv, callback, data);
}

void
graphics_register_motion_callback(struct graphics *this, void (*callback)(void *data, struct point *p), void *data)
{
	this->meth.register_motion_callback(this->priv, callback, data);
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

void
graphics_gc_set_linewidth(struct graphics_gc *gc, int width)
{
	gc->meth.gc_set_linewidth(gc->priv, width);
}

struct graphics_image *
graphics_image_new(struct graphics *gra, char *path)
{
	struct graphics_image *this;

	this=g_new0(struct graphics_image,1);
	this->priv=gra->meth.image_new(gra->priv, &this->meth, path, &this->width, &this->height);
	return this;
}

void
graphics_draw_restore(struct graphics *this, struct point *p, int w, int h)
{
	this->meth.draw_restore(this->priv, p, w, h);
}

void
graphics_draw_mode(struct graphics *this, enum draw_mode_num mode)
{
	this->meth.draw_mode(this->priv, mode);
}

void
graphics_draw_lines(struct graphics *this, struct graphics_gc *gc, struct point *p, int count)
{
	this->meth.draw_lines(this->priv, gc->priv, p, count);
}

void
graphics_draw_circle(struct graphics *this, struct graphics_gc *gc, struct point *p, int r)
{
	this->meth.draw_circle(this->priv, gc->priv, p, r);
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

struct displayitem {
	struct item item;
	char *label;
	int displayed;
	int count;
	struct point pnt[0];
};

static int
xdisplay_free_list(gpointer key, gpointer value, gpointer user_data)
{
	GList *h, *l;
	h=value;
	l=h;
	while (l) {
#if 1
		struct displayitem *di=l->data;
		if (! di->displayed && di->item.type < type_line) 
			dbg(0,"warning: item '%s' not displayed\n", item_to_name(di->item.type));
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
	struct displayitem *di;
	int len;
	GList *l;
	char *p;


	len=sizeof(*di)+count*sizeof(*pnt);
	if (label)
		len+=strlen(label)+1;

	p=g_malloc(len);

	di=(struct displayitem *)p;
	di->displayed=0;
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
label_line(struct graphics *gra, struct graphics_gc *fg, struct graphics_gc *bg, struct graphics_font *font, struct point *p, int count, char *label)
{
	int i,x,y,tl;
	double dx,dy,l;
	struct point p_t;

	tl=strlen(label)*400;
	for (i = 0 ; i < count-1 ; i++) {
		dx=p[i+1].x-p[i].x;
		dx*=100;
		dy=p[i+1].y-p[i].y;
		dy*=100;
		l=(int)sqrt((float)(dx*dx+dy*dy));
		if (l > tl) {
			x=p[i].x;
			y=p[i].y;
			if (dx < 0) {
				dx=-dx;
				dy=-dy;
				x=p[i+1].x;
				y=p[i+1].y;
			}
			x+=(l-tl)*dx/l/200;
			y+=(l-tl)*dy/l/200;
			x-=dy*45/l/10;
			y+=dx*45/l/10;
			p_t.x=x;
			p_t.y=y;
	#if 0
			printf("display_text: '%s', %d, %d, %d, %d %d\n", label, x, y, dx*0x10000/l, dy*0x10000/l, l);
	#endif
			gra->meth.draw_text(gra->priv, fg->priv, bg->priv, font->priv, label, &p_t, dx*0x10000/l, dy*0x10000/l);
		}
	}
}


static void
xdisplay_draw_elements(struct graphics *gra, GHashTable *display_list, struct itemtype *itm)
{
	struct element *e;
	GList *l,*ls,*es,*types;
	enum item_type type;
	struct graphics_gc *gc;
	struct graphics_image *img;
	struct point p;

	es=itm->elements;	
	while (es) {
		e=es->data;
		types=itm->type;
		while (types) {
			type=GPOINTER_TO_INT(types->data);
			ls=g_hash_table_lookup(display_list, GINT_TO_POINTER(type));
			l=ls;
			gc=NULL;
			img=NULL;
			while (l) {
				struct displayitem *di;
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
					if (! gra->font[e->label_size])
						gra->font[e->label_size]=graphics_font_new(gra, e->label_size*20);
					gra->meth.draw_text(gra->priv, gra->gc[2]->priv, gra->gc[1]->priv, gra->font[e->label_size]->priv, di->label, &p, 0x10000, 0);
					break;
				case element_label:
					if (di->label) {
						if (! gra->font[e->label_size])
							gra->font[e->label_size]=graphics_font_new(gra, e->label_size*20);
						label_line(gra, gra->gc[2], gra->gc[1], gra->font[e->label_size], di->pnt, di->count, di->label);
					}
					break;
				case element_icon:
					if (!img) {
						img=graphics_image_new(gra, e->u.icon.src);
						if (! img)
							g_warning("failed to load icon '%s'\n", e->u.icon.src);
					}
					p.x=di->pnt[0].x - img->width/2;
					p.y=di->pnt[0].y - img->height/2;
					gra->meth.draw_image(gra->priv, gra->gc[0]->priv, &p, img->priv);
					break;
				case element_image:
					printf("image: '%s'\n", di->label);
					gra->meth.draw_image_warp(gra->priv, gra->gc[0]->priv, di->pnt, di->count, di->label);
					break;
				default:
					printf("Unhandled element type %d\n", e->type);
				
				}
				l=g_list_next(l);
			}
			types=g_list_next(types);	
		}
		es=g_list_next(es);
	}	
}

static void
xdisplay_draw_layer(GHashTable *display_list, struct graphics *gra, struct layer *lay, int order)
{
	GList *itms;
	struct itemtype *itm;

	itms=lay->itemtypes;
	while (itms) {
		itm=itms->data;
		if (order >= itm->zoom_min && order <= itm->zoom_max) 
			xdisplay_draw_elements(gra, display_list, itm);
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
do_draw_poly(GHashTable *display_list, struct transformation *t, enum projection pro, struct item *item, struct route *route)
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
	if (transform_contains(t, pro, &r)) {	
		attr.u.str=NULL;
		char *utf8=NULL;
		item_attr_get(item, attr_label, &attr);
		if (attr.u.str && attr.u.str[0]) 
			utf8=g_convert(attr.u.str, -1,"utf-8","iso8859-1",NULL,NULL,NULL);
		xdisplay_add(display_list, item, count, pnt, utf8);
		g_free(utf8);
		if (route && route_contains(route, item)) {
			struct item ritem;
			ritem=*item;
			ritem.type=type_street_route;
			xdisplay_add(display_list, &ritem, count, pnt, NULL);
		}
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
		char *utf8=NULL;
		item_attr_get(item, attr_label, &attr); 
		if (attr.u.str && attr.u.str[0]) 
			utf8=g_convert(attr.u.str, -1,"utf-8","iso8859-1",NULL,NULL,NULL);
		xdisplay_add(display_list, item, 1, &pnt, utf8);
		g_free(utf8);
	}
}

extern void *route_selection;

static void
do_draw(GHashTable *display_list, struct transformation *t, GList *mapsets, int order, struct route *route)
{
	struct map_selection sel;
	struct map_rect *mr;
	struct item *item;
	struct mapset *ms;
	struct map *m;
	enum projection pro;
	struct mapset_handle *h;

	sel.next=NULL;
	sel.order[layer_town]=1*order;
	sel.order[layer_street]=order;
	sel.order[layer_poly]=1*order;
	ms=mapsets->data;
	h=mapset_open(ms);
	while ((m=mapset_next(h, 1))) {
		pro=map_projection(m);
		transform_rect(t, pro, &sel.rect);
		if (route_selection)
			mr=map_rect_new(m, route_selection);
		else
			mr=map_rect_new(m, &sel);
		while ((item=map_rect_get_item(mr))) {
			if (item->type < type_line) {
				do_draw_point(display_list, t, pro, item);
			} else {
				do_draw_poly(display_list, t, pro, item, route);
			}
		}
		map_rect_destroy(mr);
	}
	mapset_close(h);
}

int
graphics_ready(struct graphics *this)
{
	return this->ready;
}

void
graphics_draw(struct graphics *gra, GHashTable *display_list, GList *mapsets, struct transformation *trans, GList *layouts, struct route *route)
{
	int order=transform_get_order(trans);

	dbg(1,"enter");

#if 0
	printf("scale=%d center=0x%x,0x%x mercator scale=%f\n", scale, co->trans->center.x, co->trans->center.y, transform_scale(co->trans->center.y));
#endif
	
	xdisplay_free(display_list);
	dbg(0,"order=%d\n", order);


	gra->meth.draw_mode(gra->priv, draw_mode_begin);
#if 0
	for (i = 0 ; i < data_window_type_end; i++) {
		data_window_begin(co->data_window[i]);	
	}
#endif
	do_draw(display_list, trans, mapsets, order, route);
	xdisplay_draw(display_list, gra, layouts, order);
  
	gra->meth.draw_mode(gra->priv, draw_mode_end);
#if 0
	for (i = 0 ; i < data_window_type_end; i++) {
		data_window_end(co->data_window[i]);	
	}
#endif
	gra->ready=1;
}

struct displaylist_handle {
	GHashTable *dl;
	GList *l;
	gpointer hashkey;	
	gpointer lastkey;
};

static void
graphics_displaylist_hash_next(gpointer key, gpointer value, gpointer user_data)
{
	struct displaylist_handle *dlh=user_data;
	if (!dlh->l && (dlh->lastkey == dlh->hashkey || dlh->hashkey == NULL)) {
		dlh->hashkey=key;
		dlh->l=value;
	}
	dlh->lastkey=key;
}

struct displaylist_handle *
graphics_displaylist_open(GHashTable *display_list)
{
	struct displaylist_handle *ret;

	ret=g_new0(struct displaylist_handle, 1);
	ret->dl=display_list;
	

	return ret;
}

struct displayitem *
graphics_displaylist_next(struct displaylist_handle *dlh)
{
	struct displayitem *ret;
	if (! dlh->l) {
		dlh->lastkey=NULL;
		g_hash_table_foreach(dlh->dl, graphics_displaylist_hash_next, dlh);
		if (!dlh->l)
			return NULL;
	}
	ret=dlh->l->data;
	dlh->l=g_list_next(dlh->l);
	return ret;
}

void
graphics_displaylist_close(struct displaylist_handle *dlh)
{
	g_free(dlh);
}

struct item *
graphics_displayitem_get_item(struct displayitem *di)
{
	return &di->item;	
}

char *
graphics_displayitem_get_label(struct displayitem *di)
{
	return di->label;
}

static int
within_dist_point(struct point *p0, struct point *p1, int dist)
{
	if (p0->x == 32767 || p0->y == 32767 || p1->x == 32767 || p1->y == 32767)
		return 0;
	if (p0->x == -32768 || p0->y == -32768 || p1->x == -32768 || p1->y == -32768)
		return 0;
        if ((p0->x-p1->x)*(p0->x-p1->x) + (p0->y-p1->y)*(p0->y-p1->y) <= dist*dist) {
                return 1;
        }
        return 0;
}

static int
within_dist_line(struct point *p, struct point *line_p0, struct point *line_p1, int dist)
{
	int vx,vy,wx,wy;
	int c1,c2;
	struct point line_p;

	vx=line_p1->x-line_p0->x;
	vy=line_p1->y-line_p0->y;
	wx=p->x-line_p0->x;
	wy=p->y-line_p0->y;

	c1=vx*wx+vy*wy;
	if ( c1 <= 0 )
		return within_dist_point(p, line_p0, dist);
	c2=vx*vx+vy*vy;
	if ( c2 <= c1 )
		return within_dist_point(p, line_p1, dist);

	line_p.x=line_p0->x+vx*c1/c2;
	line_p.y=line_p0->y+vy*c1/c2;
	return within_dist_point(p, &line_p, dist);
}

static int
within_dist_polyline(struct point *p, struct point *line_pnt, int count, int dist, int close)
{
	int i;
	for (i = 0 ; i < count-1 ; i++) {
		if (within_dist_line(p,line_pnt+i,line_pnt+i+1,dist)) {
			return 1;
		}
	}
	if (close)
		return (within_dist_line(p,line_pnt,line_pnt+count-1,dist));
	return 0;
}

static int
within_dist_polygon(struct point *p, struct point *poly_pnt, int count, int dist)
{
	int i, j, c = 0;
        for (i = 0, j = count-1; i < count; j = i++) {
		if ((((poly_pnt[i].y <= p->y) && ( p->y < poly_pnt[j].y )) ||
		((poly_pnt[j].y <= p->y) && ( p->y < poly_pnt[i].y))) &&
		(p->x < (poly_pnt[j].x - poly_pnt[i].x) * (p->y - poly_pnt[i].y) / (poly_pnt[j].y - poly_pnt[i].y) + poly_pnt[i].x)) 
                        c = !c;
        }
	if (! c)
		return within_dist_polyline(p, poly_pnt, count, dist, 1);
        return c;
}

int
graphics_displayitem_within_dist(struct displayitem *di, struct point *p, int dist)
{
	if (di->item.type < type_line) {
		return within_dist_point(p, &di->pnt[0], dist);
	}
	if (di->item.type < type_area) {
		return within_dist_polyline(p, di->pnt, di->count, dist, 0);
	}
	return within_dist_polygon(p, di->pnt, di->count, dist);
}
