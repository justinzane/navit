#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if 0
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>
#endif
#include <glib.h>
#include "config.h"
#include "debug.h"
#include "profile.h"
#include "coord.h"
#include "projection.h"
#include "map.h"
#include "mapset.h"
#include "item.h"
#include "route.h"
#include "track.h"
#include "point.h"
#include "graphics.h"
#include "transform.h"
#include "plugin.h"
#include "fib.h"


struct map_priv {
	struct route *route;
};

int debug_route=0;

struct route_graph_point {
	struct route_graph_point *next;
	struct route_graph_point *hash_next;
	struct route_graph_segment *start;
	struct route_graph_segment *end;
	struct route_graph_segment *seg;
	struct fibheap_el *el;
	int value;
	struct coord c;
};

struct route_graph_segment {
	struct route_graph_segment *next;
	struct route_graph_segment *start_next;
	struct route_graph_segment *end_next;
	struct route_graph_point *start;
	struct route_graph_point *end;
	struct item item;
	int flags;
	int len;
	int offset;
};

struct route_path_segment {
	struct route_path_segment *next;
	struct item item;
	unsigned int offset;
#if 0
	int time;
#endif
	int length;
	int dir;
	unsigned ncoords;
	struct coord c[0];
};

struct route_info {
	struct coord c;
	struct coord lp;
	int pos;

	int dist;
	int dir;

	struct street_data *street;
};

struct route_path {
	struct route_path_segment *path;
	struct route_path_segment *path_last;
	/* XXX: path_hash is not necessery now */
	struct item_hash *path_hash;
};

#define RF_FASTEST	(1<<0)
#define RF_SHORTEST	(1<<1)
#define RF_AVOIDHW	(1<<2)
#define RF_AVOIDPAID	(1<<3)
#define RF_LOCKONROAD	(1<<4)
#define RF_SHOWGRAPH	(1<<5)

struct route {
	int version;
	struct mapset *ms;
	unsigned flags;
	struct route_info *pos;
	struct route_info *dst;

	struct route_graph *graph;
	struct route_path *path2;
	struct map *map;
	struct map *graph_map;
	int speedlist[route_item_last-route_item_first+1];
};

struct route_graph {
	struct route_graph_point *route_points;
	struct route_graph_segment *route_segments;
#define HASH_SIZE 8192
	struct route_graph_point *hash[HASH_SIZE];
};

#define HASHCOORD(c) ((((c)->x +(c)->y) * 2654435761UL) & (HASH_SIZE-1))

static struct route_info * route_find_nearest_street(struct mapset *ms, struct pcoord *c);
static struct route_graph_point *route_graph_get_point(struct route_graph *this, struct coord *c);
static void route_graph_update(struct route *this);
static struct route_path *route_path_new(struct route_graph *this, struct route_path *oldpath, struct route_info *pos, struct route_info *dst, int *speedlist);
static void route_process_street_graph(struct route_graph *this, struct item *item);
static void route_graph_destroy(struct route_graph *this);
static void route_path_update(struct route *this);

static enum projection route_projection(struct route *route)
{
	struct street_data *street;
	street = route->pos ? route->pos->street : route->dst->street;
	return map_projection(street->item.map);
}

static void
route_path_destroy(struct route_path *this)
{
	struct route_path_segment *c,*n;
	if (! this)
		return;
	if (this->path_hash) {
		item_hash_destroy(this->path_hash);
		this->path_hash=NULL;
	}
	c=this->path;
	while (c) {
		n=c->next;
		g_free(c);
		c=n;
	}
	g_free(this);
}

struct route *
route_new(struct mapset *ms)
{
	struct route *this=g_new0(struct route, 1);
	if (!this) {
		printf("%s:Out of memory\n", __FUNCTION__);
		return NULL;
	}
	this->ms=ms;
	return this;
}

void
route_set_mapset(struct route *this, struct mapset *ms)
{
	this->ms=ms;
}

struct mapset *
route_get_mapset(struct route *this)
{
	return this->ms;
}

struct route_info *
route_get_pos(struct route *this)
{
	return this->pos;
}

struct route_info *
route_get_dst(struct route *this)
{
	return this->dst;
}

int *
route_get_speedlist(struct route *this)
{
	return this->speedlist;
}

int
route_get_path_set(struct route *this)
{
	return this->path2 != NULL;
}

int
route_set_speed(struct route *this, enum item_type type, int value)
{
	if (type < route_item_first || type > route_item_last) {
		dbg(0,"street type %d out of range [%d,%d]", type, route_item_first, route_item_last);
		return 0;
	}
	this->speedlist[type-route_item_first]=value;
	return 1;
}

int
route_contains(struct route *this, struct item *item)
{
	if (! this->path2 || !this->path2->path_hash)
		return 0;
	return  (int)item_hash_lookup(this->path2->path_hash, item);
}

static void
route_path_update(struct route *this)
{
	struct route_path *oldpath = NULL;
	if (! this->pos || ! this->dst) {
		route_path_destroy(this->path2);
		this->path2 = NULL;
		return;
	}
	/* the graph is destroyed when setting the destination */
	if (this->graph && this->pos && this->dst && this->path2) {
		// we can try to update
		oldpath = this->path2;
		this->path2 = NULL;
	}
	if (! this->graph || !(this->path2=route_path_new(this->graph, oldpath, this->pos, this->dst, this->speedlist))) {
		profile(0,NULL);
		route_graph_update(this);
		this->path2=route_path_new(this->graph, oldpath, this->pos, this->dst, this->speedlist);
		profile(1,"route_path_new");
		profile(0,"end");
	}
	if (oldpath) {
		/* Destroy what's left */
		route_path_destroy(oldpath);
	}
}

void
route_set_position(struct route *this, struct pcoord *pos)
{
	if (this->pos)
		route_info_free(this->pos);
	this->pos=NULL;
	this->pos=route_find_nearest_street(this->ms, pos);
	dbg(1,"this->pos=%p\n", this->pos);
	if (! this->pos)
		return;
	if (this->dst) 
		route_path_update(this);
}

void
route_set_position_from_tracking(struct route *this, struct tracking *tracking)
{
	struct coord *c;
	struct route_info *ret;

	dbg(2,"enter\n");
	c=tracking_get_pos(tracking);
	ret=g_new0(struct route_info, 1);
	if (!ret) {
		printf("%s:Out of memory\n", __FUNCTION__);
		return;
	}
	if (this->pos)
		route_info_free(this->pos);
	this->pos=NULL;
	ret->c=*c;
	ret->lp=*c;
	ret->pos=tracking_get_segment_pos(tracking);
	ret->dist=0;
	ret->dir=0;
	ret->street=street_data_dup(tracking_get_street_data(tracking));
	dbg(3,"c->x=0x%x, c->y=0x%x pos=%d item=(0x%x,0x%x)\n", c->x, c->y, ret->pos, ret->street->item.id_hi, ret->street->item.id_lo);
	dbg(3,"street 0=(0x%x,0x%x) %d=(0x%x,0x%x)\n", ret->street->c[0].x, ret->street->c[0].y, ret->street->count-1, ret->street->c[ret->street->count-1].x, ret->street->c[ret->street->count-1].y);
	this->pos=ret;
	if (this->dst) 
		route_path_update(this);
	dbg(2,"ret\n");
}

/* Used for debuging of route_rect, what routing sees */
struct map_selection *route_selection;

struct map_selection *
route_rect(int order, struct coord *c1, struct coord *c2, int rel, int abs)
{
	int dx,dy,sx=1,sy=1,d,m;
	struct map_selection *sel=g_new(struct map_selection, 1);
	if (!sel) {
		printf("%s:Out of memory\n", __FUNCTION__);
		return sel;
	}
	sel->order[layer_town]=0;
	sel->order[layer_poly]=0;
	sel->order[layer_street]=order;
	dbg(1,"%p %p\n", c1, c2);
	dx=c1->x-c2->x;
	dy=c1->y-c2->y;
	if (dx < 0) {
		sx=-1;
		sel->u.c_rect.lu.x=c1->x;
		sel->u.c_rect.rl.x=c2->x;
	} else {
		sel->u.c_rect.lu.x=c2->x;
		sel->u.c_rect.rl.x=c1->x;
	}
	if (dy < 0) {
		sy=-1;
		sel->u.c_rect.lu.y=c2->y;
		sel->u.c_rect.rl.y=c1->y;
	} else {
		sel->u.c_rect.lu.y=c1->y;
		sel->u.c_rect.rl.y=c2->y;
	}
	if (dx*sx > dy*sy) 
		d=dx*sx;
	else
		d=dy*sy;
	m=d*rel/100+abs;	
	sel->u.c_rect.lu.x-=m;
	sel->u.c_rect.rl.x+=m;
	sel->u.c_rect.lu.y+=m;
	sel->u.c_rect.rl.y-=m;
	sel->next=NULL;
	return sel;
}

static struct map_selection *
route_calc_selection(struct coord *c1, struct coord *c2)
{
	struct map_selection *ret,*sel;
	sel=route_rect(4, c1, c2, 25, 0);
	ret=sel;
	sel->next=route_rect(8, c1, c1, 0, 40000);
	sel=sel->next;
	sel->next=route_rect(18, c1, c1, 0, 10000);
	sel=sel->next;
	sel->next=route_rect(8, c2, c2, 0, 40000);
	sel=sel->next;
	sel->next=route_rect(18, c2, c2, 0, 10000);
	/* route_selection=ret; */
	return ret;
}

static void
route_free_selection(struct map_selection *sel)
{
	struct map_selection *next;
	while (sel) {
		next=sel->next;
		g_free(sel);
		sel=next;
	}
}



void
route_set_destination(struct route *this, struct pcoord *dst)
{
	profile(0,NULL);
	if (this->dst)
		route_info_free(this->dst);
	this->dst=NULL;
	if (dst)
		this->dst=route_find_nearest_street(this->ms, dst);
	profile(1,"find_nearest_street");
	route_graph_destroy(this->graph);
	this->graph=NULL;
	route_path_update(this);
	profile(0,"end");
}

static struct route_graph_point *
route_graph_get_point(struct route_graph *this, struct coord *c)
{
	struct route_graph_point *p;
	int hashval=HASHCOORD(c);
	p=this->hash[hashval];
	while (p) {
		if (p->c.x == c->x && p->c.y == c->y) 
			return p;
		p=p->hash_next;
	}
	return NULL;
}


static struct route_graph_point *
route_graph_add_point(struct route_graph *this, struct coord *f)
{
	int hashval;
	struct route_graph_point *p;

	p=route_graph_get_point(this,f);
	if (!p) {
		hashval=HASHCOORD(f);
		if (debug_route)
			printf("p (0x%x,0x%x)\n", f->x, f->y);
		p=g_new(struct route_graph_point,1);
		if (!p) {
			printf("%s:Out of memory\n", __FUNCTION__);
			return p;
		}
		p->hash_next=this->hash[hashval];
		this->hash[hashval]=p;
		p->next=this->route_points;
		p->el=NULL;
		p->start=NULL;
		p->end=NULL;
		p->seg=NULL;
		p->value=INT_MAX;
		p->c=*f;
		this->route_points=p;
	}
	return p;
}


static void
route_graph_free_points(struct route_graph *this)
{
	struct route_graph_point *curr,*next;
	curr=this->route_points;
	while (curr) {
		next=curr->next;
		g_free(curr);
		curr=next;
	}
	this->route_points=NULL;
	memset(this->hash, 0, sizeof(this->hash));
}

static void
route_graph_add_segment(struct route_graph *this, struct route_graph_point *start,
			struct route_graph_point *end, int len, struct item *item,
			int flags, int offset)
{
	struct route_graph_segment *s;
	s = g_new0(struct route_graph_segment, 1);
	if (!s) {
		printf("%s:Out of memory\n", __FUNCTION__);
		return;
	}
	s->start=start;
	s->start_next=start->start;
	start->start=s;
	s->end=end;
	s->end_next=end->end;
	end->end=s;
	g_assert(len >= 0);
	s->len=len;
	s->item=*item;
	s->flags=flags;
	s->offset = offset;
	s->next=this->route_segments;
	this->route_segments=s;
	if (debug_route)
		printf("l (0x%x,0x%x)-(0x%x,0x%x)\n", start->c.x, start->c.y, end->c.x, end->c.y);
}

static int get_item_seg_coords(struct item *i, struct coord *c, int max,
		struct route_graph_segment *rgs)
{
	struct map_rect *mr;
	struct item *item;
	int rc = 0, fs = 0, p = 0;
	struct coord c1;
	mr=map_rect_new(i->map, NULL);
	if (!mr)
		return 0;
	item = map_rect_get_item_byid(mr, i->id_hi, i->id_lo);
	if (item) {
		do {
			rc = item_coord_get(item, &c1, 1);
			if (rc) {
				if (!fs) {
					if (c1.x == rgs->start->c.x &&
						c1.y == rgs->start->c.y)
						fs = 1;
				}
				if (fs) {
					c[p++] = c1;
					if (c1.x == rgs->end->c.x &&
						c1.y == rgs->end->c.y)
						break;
				}
			}
		} while (rc);
	}
	map_rect_destroy(mr);
	return p;
}

static struct route_path_segment *
route_extract_segment_from_path(struct route_path *path, struct item *item,
						 int offset)
{
	struct route_path_segment *sp = NULL, *s;
	s = path->path;
	while (s) {
		if (s->offset == offset && item_is_equal(s->item,*item)) {
			if (sp) {
				sp->next = s->next;
				break;
			} else {
				path->path = s->next;
				break;
			}
		}
		sp = s;
		s = s->next;
	}
	if (s)
		item_hash_remove(path->path_hash, item);
	return s;
}

static void
route_path_add_item(struct route_path *this, struct route_path *oldpath,
		struct route_graph_segment *rgs, int len, int offset, int dir)
{
	struct route_path_segment *segment;
	int ccnt = 0;
	struct coord ca[2048];

	if (oldpath) {
		ccnt = (int)item_hash_lookup(oldpath->path_hash, &rgs->item);
		if (ccnt) {
			segment = route_extract_segment_from_path(oldpath,
							 &rgs->item, offset);
			if (segment)
				goto linkold;
		}
	}

	ccnt = get_item_seg_coords(&rgs->item, ca, 2047, rgs);
	segment= g_malloc0(sizeof(*segment) + sizeof(struct coord) * ccnt);
	if (!segment) {
		printf("%s:Out of memory\n", __FUNCTION__);
		return;
	}
	memcpy(segment->c, ca, ccnt * sizeof(struct coord));
	segment->ncoords = ccnt;
	segment->item=rgs->item;
	segment->offset = offset;
linkold:
	segment->length=len;
	segment->dir=dir;
	segment->next=NULL;
	item_hash_insert(this->path_hash,  &rgs->item, (void *)offset);
	if (!this->path)
		this->path=segment;
	if (this->path_last)
		this->path_last->next=segment;
	this->path_last=segment;
}

struct route_path_handle {
	struct route_path_segment *s;
};

struct route_path_handle *
route_path_open(struct route *this)
{
	struct route_path_handle *ret;

	if (! this->path2 || ! this->path2->path) 
		return NULL;

	ret=g_new(struct route_path_handle, 1);
	if (!ret) {
		printf("%s:Out of memory\n", __FUNCTION__);
		return ret;
	}
	ret->s=this->path2->path;
	return ret;
}

struct route_path_segment *
route_path_get_segment(struct route_path_handle *h)
{
	struct route_path_segment *ret=h->s;

	if (ret)
		h->s=ret->next;

	return ret;
}

static struct coord *
route_path_segment_get_helper(struct route_path_segment *s, int dir)
{
	if (s->dir == dir)
		return &s->c[0];
	else
		return &s->c[s->ncoords-1];
}

struct coord *
route_path_segment_get_start(struct route_path_segment *s)
{
	return route_path_segment_get_helper(s, 1);
}

struct coord *
route_path_segment_get_end(struct route_path_segment *s)
{
	return route_path_segment_get_helper(s, -1);
}

struct item *
route_path_segment_get_item(struct route_path_segment *s)
{
	return &s->item;
}

void
route_path_close(struct route_path_handle *h)
{
	g_free(h);
}

static void
route_graph_free_segments(struct route_graph *this)
{
	struct route_graph_segment *curr,*next;
	curr=this->route_segments;
	while (curr) {
		next=curr->next;
		g_free(curr);
		curr=next;
	}
	this->route_segments=NULL;
}

static void
route_graph_destroy(struct route_graph *this)
{
	if (this) {
		route_graph_free_points(this);
		route_graph_free_segments(this);
		g_free(this);
	}
}

int
route_time(int *speedlist, struct item *item, int len)
{
	if (item->type < route_item_first || item->type > route_item_last) {
		dbg(0,"street type %d out of range [%d,%d]", item->type, route_item_first, route_item_last);
		return len*36;
	}
	return len*36/speedlist[item->type-route_item_first];
}


static int
route_value(int *speedlist, struct item *item, int len)
{
	int ret;
	if (len < 0) {
		printf("len=%d\n", len);
	}
	g_assert(len >= 0);
	ret=route_time(speedlist, item, len);
	dbg(1, "route_value(0x%x, %d)=%d\n", item->type, len, ret);
	return ret;
}

static void
route_process_street_graph(struct route_graph *this, struct item *item)
{
#ifdef AVOID_FLOAT
	int len=0;
#else
	double len=0;
#endif
	struct route_graph_point *s_pnt,*e_pnt;
	struct coord c,l;
	struct attr attr;
	int flags = 0;
	int segmented = 0;
	int offset = 1;

	if (item_coord_get(item, &l, 1)) {
		if (item_attr_get(item, attr_flags, &attr)) {
			flags = attr.u.num;
			if (flags & AF_SEGMENTED)
				segmented = 1;
		}
		s_pnt=route_graph_add_point(this,&l);
		if (!segmented) {
			while (item_coord_get(item, &c, 1)) {
				len+=transform_distance(map_projection(item->map), &l, &c);
				l=c;
			}
			e_pnt=route_graph_add_point(this,&l);
			g_assert(len >= 0);
			route_graph_add_segment(this, s_pnt, e_pnt, len, item, flags, offset);
		} else {
			int isseg,rc;
			int sc = 0;
			do {
				isseg = item_coord_is_segment(item);
				rc = item_coord_get(item, &c, 1);
				if (rc) {
					len+=transform_distance(map_projection(item->map), &l, &c);
					l=c;
					if (isseg) {
						e_pnt=route_graph_add_point(this,&l);
						route_graph_add_segment(this, s_pnt, e_pnt, len, item, flags, offset);
						offset++;
						s_pnt=route_graph_add_point(this,&l);
						len = 0;
					}
				}
			} while(rc);
			e_pnt=route_graph_add_point(this,&l);
			g_assert(len >= 0);
			sc++;
			route_graph_add_segment(this, s_pnt, e_pnt, len, item, flags, offset);
		}
	}
}

static int
compare(void *v1, void *v2)
{
	struct route_graph_point *p1=v1;
	struct route_graph_point *p2=v2;
#if 0
	if (debug_route)
		printf("compare %d (%p) vs %d (%p)\n", p1->value,p1,p2->value,p2);
#endif
	return p1->value-p2->value;
}

int
route_info_length(struct route_info *pos, struct route_info *dst, int dir)
{
	struct route_info_handle *h;
	struct coord *c,*l;
	int ret=0;
	struct street_data *street;

	dbg(2,"enter pos=%p dst=%p dir=%d\n", pos, dst, dir);
	h=route_info_open(pos, dst, dir);
	if (! h) {
		dbg(2,"ret=-1\n");
		return -1;
	}
	street = pos ? pos->street : dst->street;
	l=route_info_get(h);
	while ((c=route_info_get(h))) {
		dbg(3,"c=%p\n", c);
		ret+=transform_distance(map_projection(street->item.map), c, l);
		l=c;
	}
	dbg(2,"ret=%d\n", ret);
	return ret;
}

static void
route_graph_flood(struct route_graph *this, struct route_info *dst, int *speedlist)
{
	struct route_graph_point *p_min,*end=NULL;
	struct route_graph_segment *s;
	int min,new,old,val;
	struct fibheap *heap;
	struct street_data *sd=dst->street;

	heap = fh_makeheap();   
	fh_setcmp(heap, compare);

	if (! (sd->flags & AF_ONEWAYREV)) {
		end=route_graph_get_point(this, &sd->c[0]);
		g_assert(end != 0);
		end->value=route_value(speedlist, &sd->item, route_info_length(NULL, dst, -1));
		end->el=fh_insert(heap, end);
	}

	if (! (sd->flags & AF_ONEWAY)) {
		end=route_graph_get_point(this, &sd->c[sd->count-1]);
		g_assert(end != 0);
		end->value=route_value(speedlist, &sd->item, route_info_length(NULL, dst, 1));
		end->el=fh_insert(heap, end);
	}

	dbg(1,"0x%x,0x%x\n", end->c.x, end->c.y);
	for (;;) {
		p_min=fh_extractmin(heap);
		if (! p_min)
			break;
		min=p_min->value;
		if (debug_route)
			printf("extract p=%p free el=%p min=%d, 0x%x, 0x%x\n", p_min, p_min->el, min, p_min->c.x, p_min->c.y);
		p_min->el=NULL;
		s=p_min->start;
		while (s) {
			val=route_value(speedlist, &s->item, s->len);
#if 0
			val+=val*2*street_route_contained(s->str->segid);
#endif
			new=min+val;
			if (debug_route)
				printf("begin %d len %d vs %d (0x%x,0x%x)\n",new,val,s->end->value, s->end->c.x, s->end->c.y);
			if (new < s->end->value && !(s->flags & AF_ONEWAY)) {
				s->end->value=new;
				s->end->seg=s;
				if (! s->end->el) {
					if (debug_route)
						printf("insert_end p=%p el=%p val=%d ", s->end, s->end->el, s->end->value);
					s->end->el=fh_insert(heap, s->end);
					if (debug_route)
						printf("el new=%p\n", s->end->el);
				}
				else {
					if (debug_route)
						printf("replace_end p=%p el=%p val=%d\n", s->end, s->end->el, s->end->value);
					fh_replacedata(heap, s->end->el, s->end);
				}
			}
			if (debug_route)
				printf("\n");
			s=s->start_next;
		}
		s=p_min->end;
		while (s) {
			val=route_value(speedlist, &s->item, s->len);
			new=min+val;
			if (debug_route)
				printf("end %d len %d vs %d (0x%x,0x%x)\n",new,val,s->start->value,s->start->c.x, s->start->c.y);
			if (new < s->start->value && !(s->flags & AF_ONEWAYREV)) {
				old=s->start->value;
				s->start->value=new;
				s->start->seg=s;
				if (! s->start->el) {
					if (debug_route)
						printf("insert_start p=%p el=%p val=%d ", s->start, s->start->el, s->start->value);
					s->start->el=fh_insert(heap, s->start);
					if (debug_route)
						printf("el new=%p\n", s->start->el);
				}
				else {
					if (debug_route)
						printf("replace_start p=%p el=%p val=%d\n", s->start, s->start->el, s->start->value);
					fh_replacedata(heap, s->start->el, s->start);
				}
			}
			if (debug_route)
				printf("\n");
			s=s->end_next;
		}
	}
	fh_deleteheap(heap);
}

static struct route_path *
route_path_new(struct route_graph *this, struct route_path *oldpath, struct route_info *pos, struct route_info *dst, int *speedlist)
{
	struct route_graph_point *start1=NULL,*start2=NULL,*start;
	struct route_graph_segment *s=NULL;
	int len=0,segs=0;
	int ilen,seg_len;
#if 0
	int time=0,hr,min,sec
#endif
	unsigned int val1=0xffffffff,val2=0xffffffff;
	struct street_data *sd=pos->street;
	struct route_path *ret;

	if (! (sd->flags & AF_ONEWAY)) {
		start1=route_graph_get_point(this, &sd->c[0]);
		if (! start1)
			return NULL;
		val1=start1->value+route_value(speedlist, &sd->item, route_info_length(pos, NULL, -1));
		dbg(1,"start1: %d(route)+%d=%d\n", start1->value, val1-start1->value, val1);
	}
	if (! (sd->flags & AF_ONEWAYREV)) {
		start2=route_graph_get_point(this, &sd->c[sd->count-1]);
		if (! start2)
			return NULL;
		val2=start2->value+route_value(speedlist, &sd->item, route_info_length(pos, NULL, 1));
		dbg(1,"start2: %d(route)+%d=%d\n", start2->value, val2-start2->value, val2);
	}
	dbg(1,"val1=%d val2=%d\n", val1, val2);
	if (val1 == val2) {
		val1=start1->start->start->value;
		val2=start2->end->end->value;
	}
	if (start1 && (val1 < val2)) {
		start=start1;
		pos->dir=-1;
	} else {
		if (start2) {
			start=start2;
			pos->dir=1;
		} else {
			printf("no route found, pos blocked\n");
			return NULL;
		}
	}
	ret=g_new0(struct route_path, 1);
	if (!ret) {
		printf("%s:Out of memory\n", __FUNCTION__);
		return ret;
	}
	ret->path_hash=item_hash_new();
	dbg(1,"dir=%d\n", pos->dir);
	while ((s=start->seg)) {
		segs++;
#if 0
		printf("start->value=%d 0x%x,0x%x\n", start->value, start->c.x, start->c.y);
#endif
		seg_len=s->len;
		len+=seg_len;
		if (s->start == start) {
			route_path_add_item(ret, oldpath, s, seg_len, s->offset, 1);
			start=s->end;
		} else {
			route_path_add_item(ret, oldpath, s, seg_len, s->offset, -1);
			start=s->start;
		}
	}
	sd=dst->street;
	dbg(1,"start->value=%d 0x%x,0x%x\n", start->value, start->c.x, start->c.y);
	dbg(1,"dst sd->flags=%d sd->c[0]=0x%x,0x%x sd->c[sd->count-1]=0x%x,0x%x\n", sd->flags, sd->c[0].x,sd->c[0].y, sd->c[sd->count-1].x, sd->c[sd->count-1].y);
	if (start->c.x == sd->c[0].x && start->c.y == sd->c[0].y)
		dst->dir=-1;
	else if (start->c.x == sd->c[sd->count-1].x && start->c.y == sd->c[sd->count-1].y)
		dst->dir=1;
	else {
		printf("no route found\n");
		route_path_destroy(ret);
		return NULL;
	}
	ilen=route_info_length(pos, NULL, 0);
	len+=ilen;

	ilen=route_info_length(NULL, dst, 0);
	len+=ilen;

	dbg(1, "%d segments\n", segs);
	return ret;
}

static struct route_graph *
route_graph_build(struct mapset *ms, struct coord *c1, struct coord *c2)
{
	struct route_graph *ret=g_new0(struct route_graph, 1);
	struct map_selection *sel;
	struct mapset_handle *h;
	struct map_rect *mr;
	struct map *m;
	struct item *item;

	if (!ret) {
		printf("%s:Out of memory\n", __FUNCTION__);
		return ret;
	}
	sel=route_calc_selection(c1, c2);
	h=mapset_open(ms);
	while ((m=mapset_next(h,1))) {
		mr=map_rect_new(m, sel);
		if (! mr)
			continue;
		while ((item=map_rect_get_item(mr))) {
			if (item->type >= type_street_0 && item->type <= type_ferry) {
				route_process_street_graph(ret, item);
			}
		}
		map_rect_destroy(mr);
        }
        mapset_close(h);
	route_free_selection(sel);

	return ret;
}

static void
route_graph_update(struct route *this)
{
	route_graph_destroy(this->graph);
	profile(1,"graph_free");
	this->graph=route_graph_build(this->ms, &this->pos->c, &this->dst->c);
	profile(1,"route_graph_build");
	route_graph_flood(this->graph, this->dst, this->speedlist);
	profile(1,"route_graph_flood");
	this->version++;
}

struct street_data *
street_get_data (struct item *item)
{
	int maxcount=2000;
	struct coord c[maxcount];
	int count=0;
	struct street_data *ret;
	struct attr attr;

	while (count < maxcount) {
		if (!item_coord_get(item, &c[count], 1))
			break;
		count++;
	}
	g_assert(count < maxcount);
	ret=g_malloc(sizeof(struct street_data)+count*sizeof(struct coord));
	ret->item=*item;
	ret->count=count;
	if (item_attr_get(item, attr_flags, &attr)) 
		ret->flags=attr.u.num;
	else
		ret->flags=0;
	memcpy(ret->c, c, count*sizeof(struct coord));

	return ret;
}

struct street_data *
street_data_dup(struct street_data *orig)
{
	struct street_data *ret;
	int size=sizeof(struct street_data)+orig->count*sizeof(struct coord);

	ret=g_malloc(size);
	memcpy(ret, orig, size);

	return ret;
}

void
street_data_free(struct street_data *sd)
{
	g_free(sd);
}

static struct route_info *
route_find_nearest_street(struct mapset *ms, struct pcoord *pc)
{
	struct route_info *ret=NULL;
	int max_dist=1000;
	struct map_selection *sel;
	int dist,pos;
	struct mapset_handle *h;
	struct map *m;
	struct map_rect *mr;
	struct item *item;
	struct coord lp;
	struct street_data *sd;
	struct coord c;
	struct coord_geo g;

	c.x = pc->x;
	c.y = pc->y;
	/*
	 * This is not correct for two reasons:
	 * - You may need to go back first
	 * - Currently we allow mixing of mapsets
	 */
	sel = route_rect(18, &c, &c, 0, max_dist);
	h=mapset_open(ms);
	while ((m=mapset_next(h,1))) {
		c.x = pc->x;
		c.y = pc->y;
		if (map_projection(m) != pc->pro) {
			transform_to_geo(pc->pro, &c, &g);
			transform_from_geo(map_projection(m), &g, &c);
		}
		mr=map_rect_new(m, sel);
		if (! mr)
			continue;
		while ((item=map_rect_get_item(mr))) {
			if (item->type >= type_street_0 && item->type <= type_ferry) {
				sd=street_get_data(item);
				dist=transform_distance_polyline_sq(sd->c, sd->count, &c, &lp, &pos);
				if (!ret || dist < ret->dist) {
					if (ret) {
						street_data_free(ret->street);
						g_free(ret);
					}
					ret=g_new(struct route_info, 1);
					if (!ret) {
						printf("%s:Out of memory\n", __FUNCTION__);
						return ret;
					}
					ret->c=c;
					ret->lp=lp;
					ret->pos=pos;
					ret->dist=dist;
					ret->dir=0;
					ret->street=sd;
					dbg(1,"dist=%d id 0x%x 0x%x pos=%d\n", dist, item->id_hi, item->id_lo, pos);
				} else 
					street_data_free(sd);
			}
		}  
		map_rect_destroy(mr);
	}
	mapset_close(h);
	map_selection_destroy(sel);

	return ret;
}

void
route_info_free(struct route_info *inf)
{
	if (inf->street)
		street_data_free(inf->street);
	g_free(inf);
}


#include "point.h"

struct street_data *
route_info_street(struct route_info *rinf)
{
	return rinf->street;
}

struct coord *
route_info_point(struct route_info *rinf, int point)
{
	struct street_data *sd=rinf->street;
	int dir;

	switch(point) {
	case -1:
	case 2:
		dir=(point == 2) ? rinf->dir : -rinf->dir;
		if (dir > 0)
			return &sd->c[sd->count-1];
		else
			return &sd->c[0];
	case 0:
		return &rinf->c;
	case 1:
		return &rinf->lp;
	}
	return NULL;

}

struct route_info_handle {
	struct route_info *start;
	struct route_info *curr;
	struct route_info *end;
	struct coord *last;
	int count;
	int iter;
	int pos;
	int endpos;
	int dir;
};

struct route_info_handle *
route_info_open(struct route_info *start, struct route_info *end, int dir)
{
	struct route_info_handle *ret=g_new0(struct route_info_handle, 1);
	struct route_info *curr;
	dbg(2,"enter start=%p end=%p dir=%d\n", start, end, dir);
	if (!ret) {
		printf("%s:Out of memory\n", __FUNCTION__);
		return ret;
	}
	ret->start=start;
	ret->end=end;
	if (start) 
		curr=start;
	else
		curr=end;
	ret->endpos=-2;
	if (start && end) {
		if (start->street->item.map != end->street->item.map || start->street->item.id_hi != end->street->item.id_hi || start->street->item.id_lo != end->street->item.id_lo) {
			dbg(1,"return NULL\n");
			return NULL;
		}
		printf("trivial start=%d end=%d start dir %d end dir %d\n", start->pos, end->pos, start->dir, end->dir);
		if (start->pos == end->pos) {
			printf("fixme\n");
			start->dir=0;
			end->dir=0;
		}
		if (start->pos > end->pos) {
			printf("fixme\n");
			start->dir=-1;
			end->dir=1;
		}
		if (start->pos < end->pos) {
			printf("fixme\n");
			start->dir=1;
			end->dir=-1;
		}
		printf("trivial now start=%d end=%d start dir %d end dir %d\n", start->pos, end->pos, start->dir, end->dir);
		ret->endpos=end->pos;
	}

	if (!dir)
		dir=curr->dir;
	dbg(2,"dir=%d\n", dir);
	ret->dir=dir;
	ret->curr=curr;
	ret->pos=curr->pos;
	if (dir > 0) {
		ret->pos++;
		ret->endpos++;
	}
	dbg(2,"ret=%p\n",ret);
	return ret;
}

struct coord *
route_info_get(struct route_info_handle *h)
{
	struct coord *new;
	for (;;) {
		new=NULL;
		dbg(1,"iter=%d\n", h->iter);
		switch(h->iter) {
		case 0:
			if (h->start) {
				new=&h->start->c;
				h->iter++;
				break;
			} else {
				h->iter=2;
				continue;
			}
		case 1:
			new=&h->start->lp;
			h->iter++;
			break;
		case 2:
			dbg(1,"h->pos=%d\n", h->pos);
			if (h->dir && h->pos >= 0 && h->pos < h->curr->street->count && (h->end == NULL || h->endpos!=h->pos)) {
				new=&h->curr->street->c[h->pos];
				h->pos+=h->dir;
			} else {
				h->iter++;
				continue;
			}
			break;	
		case 3:
			if (h->end) {
				new=&h->end->lp;
				h->iter++;
				break;
			}
			break;
		case 4:
			new=&h->end->c;
			h->iter++;
			break;
			
		}
		if (new) {
			dbg(1,"new=%p (0x%x,0x%x) last=%p\n", new, new->x, new->y, h->last);
			if (h->last && new->x == h->last->x && new->y == h->last->y)
				continue;
			h->last=new;
			return new;	
		}
		return NULL;
	}
}

void
route_info_close(struct route_info_handle *h)
{
	g_free(h);
}


#if 0
struct route_crossings *
route_crossings_get(struct route *this, struct coord *c)
{
      struct route_point *pnt;
      struct route_segment *seg;
      int crossings=0;
      struct route_crossings *ret;

       pnt=route_graph_get_point(this, c);
       seg=pnt->start;
       while (seg) {
		printf("start: 0x%x 0x%x\n", seg->item.id_hi, seg->item.id_lo);
               crossings++;
               seg=seg->start_next;
       }
       seg=pnt->end;
       while (seg) {
		printf("end: 0x%x 0x%x\n", seg->item.id_hi, seg->item.id_lo);
               crossings++;
               seg=seg->end_next;
       }
       ret=g_malloc(sizeof(struct route_crossings)+crossings*sizeof(struct route_crossing));
       ret->count=crossings;
       return ret;
}
#endif


struct map_rect_priv {
	struct route_info_handle *ri;
	enum attr_type attr_next;
	int pos_next;
	int pos;
	struct map_priv *mpriv;
	struct item item;
	struct item *sitem;
	int length;
	unsigned int last_coord;
	struct route_path_segment *seg;
	struct route_graph_point *point;
	struct route_graph_segment *rseg;
	char *str;
};

static void
rm_coord_rewind(void *priv_data)
{
	struct map_rect_priv *mr = priv_data;
	mr->last_coord = 0;
}

static void
rm_attr_rewind(void *priv_data)
{
	struct map_rect_priv *mr = priv_data;
	mr->attr_next = attr_street_item;
}

static int
rm_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{
	struct map_rect_priv *mr = priv_data;
	struct route_path_segment *seg=mr->seg;
	struct route *route=mr->mpriv->route;
	attr->type=attr_type;
	switch (attr_type) {
		case attr_any:
			while (mr->attr_next != attr_none) {
				if (rm_attr_get(priv_data, mr->attr_next, attr))
					return 1;
			}
			return 0;
		case attr_street_item:
			if (seg)
				attr->u.item=&seg->item;
			else
				attr->u.item=mr->sitem;
			mr->attr_next=attr_length;
			return 1;
		case attr_length:
			if (seg)
				attr->u.num=seg->length;
			else
				attr->u.num=mr->length;
			mr->attr_next=attr_time;
			return 1;
		case attr_time:
			if (seg)
				attr->u.num=route_time(route->speedlist, &seg->item, seg->length);
			else
				attr->u.num=route_time(route->speedlist, mr->sitem, mr->length);
			mr->attr_next=attr_none;
			return 1;
		default:
			attr->type=attr_none;
			return 0;
	}
	return 0;
}

static int
rm_coord_get(void *priv_data, struct coord *c, int count)
{
	struct map_rect_priv *mr = priv_data;
	struct route_path_segment *seg = mr->seg;
	struct coord *c1;
	int rc = 0,i;
	dbg(1,"pos=%d seg=%p\n", mr->pos, seg);
	switch (mr->pos) {
	case -1:
		for (i=0; i < count; i++) {
			if ((c1=route_info_get(mr->ri))) {
				c[i]=*c1;
				rc++;
			}
		}
		break;
	case 0:
		for (i=0; i < count; i++) {
			if (mr->last_coord >= seg->ncoords)
				break;
			if (i >= seg->ncoords)
				break;
			if (seg->dir < 0)
				c[i] = seg->c[seg->ncoords-++mr->last_coord];
			else
				c[i] = seg->c[mr->last_coord++];
			rc++;
		}
		break;
	case 1:
		for (i=0; i < count; i++) {
			if ((c1=route_info_get(mr->ri))) {
				c[i]=*c1;
				rc++;
			}
		}
		break;
	default:
		rc=0;
	}
	dbg(1,"return %d\n",rc);
	return rc;
}

static struct item_methods methods_route_item = {
	rm_coord_rewind,
	rm_coord_get,
	rm_attr_rewind,
	rm_attr_get,
};

static void
rp_attr_rewind(void *priv_data)
{
	struct map_rect_priv *mr = priv_data;
	mr->attr_next = attr_label;
}

static int
rp_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{
	struct map_rect_priv *mr = priv_data;
	struct route_graph_point *p = mr->point;
	if (mr->item.type != type_rg_point) 
		return 0;
	switch (attr_type) {
	case attr_any:
		while (mr->attr_next != attr_none) {
			if (rm_attr_get(priv_data, mr->attr_next, attr))
				return 1;
		}
	case attr_label:
		attr->type = attr_label;
		if (mr->str)
			g_free(mr->str);
		if (p->value != INT_MAX)
			mr->str=g_strdup_printf("%d", p->value);
		else
			mr->str=g_strdup("-");
		attr->u.str = mr->str;
		mr->attr_next=attr_none;
		return 1;
	case attr_debug:
		attr->type = attr_debug;
		if (mr->str)
			g_free(mr->str);
		mr->str=g_strdup_printf("x=%d y=%d", p->c.x, p->c.y);
		attr->u.str = mr->str;
		mr->attr_next=attr_none;
		return 1;
	default:
		return 0;
	}
}

static int
rp_coord_get(void *priv_data, struct coord *c, int count)
{
	struct map_rect_priv *mr = priv_data;
	struct route_graph_point *p = mr->point;
	struct route_graph_segment *seg = mr->rseg;
	int rc = 0,i;
	for (i=0; i < count; i++) {
		if (mr->item.type == type_rg_point) {
			if (mr->last_coord >= 1)
				break;
			c[i] = p->c;
		} else {
			if (mr->last_coord >= 2)
				break;
			if (mr->last_coord)
				c[i] = seg->end->c;
			else
				c[i] = seg->start->c;
		}
		mr->last_coord++;
		rc++;
	}
	return rc;
}

static struct item_methods methods_point_item = {
	rm_coord_rewind,
	rp_coord_get,
	rp_attr_rewind,
	rp_attr_get,
};

static void
rm_destroy(struct map_priv *priv)
{
	g_free(priv);
}

static struct map_rect_priv * 
rm_rect_new(struct map_priv *priv, struct map_selection *sel)
{
	struct map_rect_priv * mr;
	dbg(1,"enter\n");
	if (! route_get_pos(priv->route))
		return NULL;
	if (! route_get_dst(priv->route))
		return NULL;
	if (! priv->route->path2)
		return NULL;
	mr=g_new0(struct map_rect_priv, 1);
	mr->mpriv = priv;
	mr->ri=route_info_open(route_get_pos(priv->route), route_get_dst(priv->route), 0);
	mr->pos_next=1;
	mr->sitem=&(route_get_pos(priv->route)->street->item);
	mr->item.priv_data = mr;
	mr->item.type = type_street_route;
	mr->item.meth = &methods_route_item;
	if (mr->ri) {
		mr->length=route_info_length(route_get_pos(priv->route), route_get_dst(priv->route), 0);
	} else {
		mr->ri=route_info_open(route_get_pos(priv->route), NULL, 0);
		mr->length=route_info_length(route_get_pos(priv->route), NULL, 0);
		mr->pos_next=-1;
	}
	return mr;
}

static struct map_rect_priv * 
rp_rect_new(struct map_priv *priv, struct map_selection *sel)
{
	struct map_rect_priv * mr;
	dbg(1,"enter\n");
	if (! priv->route->graph || ! priv->route->graph->route_points)
		return NULL;
	mr=g_new0(struct map_rect_priv, 1);
	mr->mpriv = priv;
	mr->item.priv_data = mr;
	mr->item.type = type_rg_point;
	mr->item.meth = &methods_point_item;
	return mr;
}

static void
rm_rect_destroy(struct map_rect_priv *mr)
{
	if (mr->ri)
		route_info_close(mr->ri);
	if (mr->str)
		g_free(mr->str);
	g_free(mr);
}

static struct item *
rp_get_item(struct map_rect_priv *mr)
{
	struct route *r = mr->mpriv->route;
	struct route_graph_point *p = mr->point;
	struct route_graph_segment *seg = mr->rseg;

	if (mr->item.type == type_rg_point) {
		if (!p)
			p = r->graph->route_points;
		else
			p = p->next;
		if (p) {
			mr->point = p;
			mr->item.id_lo++;
			rm_coord_rewind(mr);
			rp_attr_rewind(mr);
			return &mr->item;
		} else
			mr->item.type = type_rg_segment;
	}
	if (!seg)
		seg=r->graph->route_segments;
	else
		seg=seg->next;
	if (seg) {
		mr->rseg = seg;
		mr->item.id_lo++;
		rm_coord_rewind(mr);
		rp_attr_rewind(mr);
		return &mr->item;
	}
	return NULL;
	
}

static struct item *
rp_get_item_byid(struct map_rect_priv *mr, int id_hi, int id_lo)
{
	struct item *ret=NULL;
	while (id_lo-- > 0) 
		ret=rp_get_item(mr);
	return ret;
}


static struct item *
rm_get_item(struct map_rect_priv *mr)
{
	struct route *r = mr->mpriv->route;
	struct route_path_segment *seg = mr->seg;
	dbg(1,"enter\n", mr->pos);

	mr->pos=mr->pos_next;
	switch(mr->pos) {
	case -1:
		mr->pos_next=0;
		break;
	case 0:
		if (!seg)
			seg = r->path2->path;
		else
			seg = seg->next;
		if (seg) {
			mr->sitem = &seg->item;
			break;
		}
		mr->pos=1;
		route_info_close(mr->ri);
		mr->ri=route_info_open(NULL, route_get_dst(r), 0);
		mr->sitem=&(route_get_dst(r)->street->item);
		mr->length=route_info_length(NULL, route_get_dst(r), 0);
	case 1:
		mr->pos_next=2;
		break;
	case 2:
		return NULL;
	}
	mr->seg = seg;
	mr->last_coord = 0;
	mr->item.id_lo++;
	rm_attr_rewind(mr);
	return &mr->item;
}

static struct item *
rm_get_item_byid(struct map_rect_priv *mr, int id_hi, int id_lo)
{
	struct item *ret=NULL;
	while (id_lo-- > 0) 
		ret=rm_get_item(mr);
	return ret;
}

static struct map_methods route_meth = {
	projection_mg,
	NULL,
	rm_destroy,
	rm_rect_new,
	rm_rect_destroy,
	rm_get_item,
	rm_get_item_byid,
	NULL,
	NULL,
	NULL,
};

static struct map_methods route_graph_meth = {
	projection_mg,
	NULL,
	rm_destroy,
	rp_rect_new,
	rm_rect_destroy,
	rp_get_item,
	rp_get_item_byid,
	NULL,
	NULL,
	NULL,
};

void 
route_toggle_routegraph_display(struct route *route)
{
	if (route->flags & RF_SHOWGRAPH) {
		route->flags &= ~RF_SHOWGRAPH;
	} else {
		route->flags |= RF_SHOWGRAPH;
	}
}

static struct map_priv *
route_map_new_helper(struct map_methods *meth, struct attr **attrs, int graph)
{
	struct map_priv *ret;
	struct attr *route_attr;

	route_attr=attr_search(attrs, NULL, attr_route);
	if (! route_attr)
		return NULL;
	ret=g_new0(struct map_priv, 1);
	if (graph)
		*meth=route_graph_meth;
	else
		*meth=route_meth;
	ret->route=route_attr->u.route;

	return ret;
}

static struct map_priv *
route_map_new(struct map_methods *meth, struct attr **attrs)
{
	return route_map_new_helper(meth, attrs, 0);
}

static struct map_priv *
route_graph_map_new(struct map_methods *meth, struct attr **attrs)
{
	return route_map_new_helper(meth, attrs, 1);
}

static struct map *
route_get_map_helper(struct route *this_, struct map **map, char *type)
{
	struct attr route_attr={.type=attr_route,.u.route=this_};
	struct attr data_attr={.type=attr_data,.u.str=""};
	struct attr *attrs_route[]={&route_attr, &data_attr, NULL};

	if (! *map) 
		*map=map_new(type,attrs_route);
	return *map;
}

struct map *
route_get_map(struct route *this_)
{
	return route_get_map_helper(this_, &this_->map, "route");
}


struct map *
route_get_graph_map(struct route *this_)
{
	return route_get_map_helper(this_, &this_->graph_map, "route_graph");
}

void
route_set_projection(struct route *this_, enum projection pro)
{
}

void
route_init(void)
{
	plugin_register_map_type("route", route_map_new);
	plugin_register_map_type("route_graph", route_graph_map_new);
}
