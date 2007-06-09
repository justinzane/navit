#include <stdio.h>
#include <string.h>
#if 0
#include <math.h>
#include <assert.h>
#include <unistd.h>
#include <sys/time.h>
#endif
#include <glib.h>
#include "debug.h"
#include "profile.h"
#include "coord.h"
#include "map.h"
#include "mapset.h"
#include "item.h"
#include "route.h"
#if 0
#include "param.h"
#include "map_data.h"
#include "block.h"
#include "street.h"
#include "street_data.h"
#include "display.h"
#endif
#include "transform.h"
#if 0
#include "route.h"
#include "phrase.h"
#include "navigation.h"
#endif
#include "fib-1.0/fib.h"
#if 0
#include "time.h"

/*	Node1:	4 */
/*	Node2:  4 */
/*	Segid:	4 */
/*	len:	4 */
/*	type:	1 */
/*	limit/order:	1 */
/*	18 Bytes  */

extern void *speech_handle;
#endif

static int speed_list[]={
	10, /* street_0 */
	10, /* street_1_city */
	30, /* street_2_city */
	40, /* street_3_city */
	50, /* street_4_city */
	80, /* highway_city */
	60, /* street_1_land */
	70, /* street_2_land */
	70, /* street_3_land */
	80, /* street_4_land */
	120, /* street_n_lanes */
	120, /* highway_land */
	40, /* ramp */
	30, /* ferry */
};

int debug_route=0;


struct route_point {
	struct route_point *next;
	struct route_point *hash_next;
	struct route_segment *start;
	struct route_segment *end;
	struct route_segment *seg;
	struct fibheap_el *el;	
	int value;
	struct coord c;
};


struct route_segment {
	struct route_segment *next;
	struct route_segment *start_next;
	struct route_segment *end_next;
	struct route_point *start;
	struct route_point *end;
	struct item item;
	int limit;
	int len;
};

struct route_path_segment {
	struct item item;
	int time;
	int length;
	struct coord c[2];
	struct route_path_segment *next;
};


struct route_info {
	int dist;
	struct item item;
	struct coord c;
	int pos;
	int dir;
	int limit;
	struct coord lp;
	int count;
	struct coord sc[0];
};

struct route {
	struct map_data *map_data;
	double route_time_val;
	double route_len_val;
	struct route_path_segment *path;
	struct route_path_segment *path_last;
	struct route_segment *route_segments;
	struct route_point *route_points;
	struct block_list *blk_lst;
#define HASH_SIZE 8192
	struct route_point *hash[HASH_SIZE];
	struct mapset *ms;
	struct route_info *pos;
	struct route_info *dst;
	GHashTable *path_hash;
};

struct route_info * route_find_nearest_street(struct route *this, struct coord *c);

static guint
route_path_hash(gconstpointer key)
{
        const struct item *itm=key;
        gconstpointer hashkey=(gconstpointer)(itm->id_hi^itm->id_lo^((int) itm->map));
        return g_direct_hash(hashkey);
}

static gboolean
route_path_equal(gconstpointer a, gconstpointer b)
{
        const struct item *itm_a=a;
        const struct item *itm_b=b;
        if (itm_a->id_hi == itm_b->id_hi && itm_a->id_lo == itm_b->id_lo && itm_a->map == itm_b->map) 
                return TRUE;
        return FALSE;
}



static void
route_path_free(struct route *this)
{
	struct route_path_segment *c,*n;
	if (this->path_hash) {
		g_hash_table_destroy(this->path_hash);
		this->path_hash=NULL;
	}
	c=this->path;	
	while (c) {
		n=c->next;
		g_free(c);
		c=n;
	}
	this->path=NULL;
	this->path_last=NULL;
}

static void
route_path_init(struct route *this)
{
	this->path_hash=g_hash_table_new_full(route_path_hash, route_path_equal, g_free, NULL);
}

struct route *
route_new(struct mapset *ms)
{
	struct route *this=g_new0(struct route, 1);
	this->ms=ms;
	route_path_init(this);
	return this;
}

int
route_contains(struct route *this, struct item *itm)
{
	return (int)(g_hash_table_lookup(this->path_hash, itm));
}


static struct route_point *route_get_point(struct route *this, struct coord *c);
static void route_flood(struct route *this, struct route_info *dst);
static void route_build_graph(struct route *this, struct coord *c1, struct coord *c2);
static void route_graph_free(struct route *this);

void
route_set_position(struct route *this, struct coord *pos)
{
	if (this->pos)
		g_free(this->pos);
	this->pos=route_find_nearest_street(this, pos);
	dbg(0,"this->pos=%p\n", this->pos);
	if (! this->pos)
		return;
	if (this->dst) {
		route_path_free(this);
		route_path_init(this);
		if (!route_find(this, this->pos, this->dst)) {
			route_graph_free(this);
			profile(0,NULL);
			route_build_graph(this, &this->pos->c, &this->dst->c);
			profile(1,"map query");
			route_flood(this, this->dst);
			profile(1,"flood");
			route_find(this, this->pos, this->dst);
			profile(1,"find");
			profile(0,"end");
		}
	}
#if 0
	struct route_info *rt;

	route_path_free(this);
	rt=route_find_nearest_street(this->map_data, pos);
	route_find_point_on_street(rt);
	if (this->pos)
		g_free(this->pos);
	this->pos=rt;
	if (this->dst) {
		route_find(this, this->pos, this->dst);
	}
#endif
	
}

struct map_selection *route_selection;

struct map_selection *
route_rect(int order, struct coord *c1, struct coord *c2, int rel, int abs)
{
	int dx,dy,sx=1,sy=1,d,m;
	struct map_selection *sel=g_new(struct map_selection, 1);
	sel->order[layer_town]=0;
	sel->order[layer_poly]=0;
	sel->order[layer_street]=order;
	dbg(1,"%p %p\n", c1, c2);
	dx=c1->x-c2->x;
	dy=c1->y-c2->y;
	if (dx < 0) {
		sx=-1;
		sel->rect.lu.x=c1->x;
		sel->rect.rl.x=c2->x;
	} else {
		sel->rect.lu.x=c2->x;
		sel->rect.rl.x=c1->x;
	}
	if (dy < 0) {
		sy=-1;
		sel->rect.lu.y=c2->y;
		sel->rect.rl.y=c1->y;
	} else {
		sel->rect.lu.y=c1->y;
		sel->rect.rl.y=c2->y;
	}
	if (dx*sx > dy*sy) 
		d=dx*sx;
	else
		d=dy*sy;
	m=d*rel/100+abs;	
	sel->rect.lu.x-=m;
	sel->rect.rl.x+=m;
	sel->rect.lu.y+=m;
	sel->rect.rl.y-=m;
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

static void route_process_street_graph(struct route *this, struct item *item);
static struct route_point *route_get_point(struct route *this, struct coord *c);

void
route_set_destination(struct route *this, struct coord *dst)
{
	profile(0,NULL);
	if (this->dst)
		g_free(this->dst);
	this->dst=route_find_nearest_street(this, dst);
	if (! this->dst || ! this->pos)
		return;
	profile(1,"find_nearest_street");

	route_graph_free(this);
	route_build_graph(this, &this->pos->c, &this->dst->c);
	profile(1,"map query");

	route_flood(this, this->dst);
	profile(1,"flood");

	route_path_free(this);
	route_path_init(this);
	route_find(this, this->pos, this->dst);
	profile(1,"find");
	profile(0,"end");
#if 0
	struct route_info *rt;

	route_find_point_on_street(rt);
	if (this->dst)
		g_free(this->dst);
	this->dst=rt;
	route_do_start(this, this->pos, this->dst);
#endif
}

static struct route_point *
route_get_point(struct route *this, struct coord *c)
{
	struct route_point *p=this->route_points;
	int hashval=(c->x +  c->y) & (HASH_SIZE-1);
	p=this->hash[hashval];
	while (p) {
		if (p->c.x == c->x && p->c.y == c->y) 
			return p;
		p=p->hash_next;
	}
	return NULL;
}


static struct route_point *
route_point_add(struct route *this, struct coord *f)
{
	int hashval;
	struct route_point *p;

	p=route_get_point(this,f);
	if (!p) {
		hashval=(f->x +  f->y) & (HASH_SIZE-1);
		if (debug_route)
			printf("p (0x%x,0x%x)\n", f->x, f->y);
		p=g_new(struct route_point,1);
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
route_points_free(struct route *this)
{
	struct route_point *curr,*next;
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
route_segment_add(struct route *this, struct route_point *start, struct route_point *end, int len, struct item *item, int limit)
{
	struct route_segment *s;
	s=g_new0(struct route_segment,1);
	s->start=start;
	s->start_next=start->start;
	start->start=s;
	s->end=end;
	s->end_next=end->end;
	end->end=s;
	g_assert(len >= 0);
	s->len=len;
	s->item=*item;
	s->limit=limit;
	s->next=this->route_segments;
	this->route_segments=s;
	if (debug_route)
		printf("l (0x%x,0x%x)-(0x%x,0x%x)\n", start->c.x, start->c.y, end->c.x, end->c.y);
	
}

static void
route_path_add_item(struct route *this, struct item *itm, struct coord *start, struct coord *end, int len, int time)
{
        struct route_path_segment *segment=g_new0(struct route_path_segment,1);
	struct item *hitm=g_new(struct item, 1);
	*hitm=*itm;
	g_hash_table_insert(this->path_hash, hitm, (void *)1);
	segment->item=*itm;
        segment->next=NULL;
	segment->length=len;
	segment->time=time;
        if (start)
                segment->c[0]=*start;
        if (end)
                segment->c[1]=*end;
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
	struct route_path_handle *ret=g_new(struct route_path_handle, 1);

	ret->s=this->path;	
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

struct coord *
route_path_segment_get_start(struct route_path_segment *s)
{
	return &s->c[0];
}

struct coord *
route_path_segment_get_end(struct route_path_segment *s)
{
	return &s->c[1];
}

struct item *
route_path_segment_get_item(struct route_path_segment *s)
{
	return &s->item;
}

int
route_path_segment_get_length(struct route_path_segment *s)
{
	return s->length;
}

int
route_path_segment_get_time(struct route_path_segment *s)
{
	return s->time;
}

void
route_path_close(struct route_path_handle *h)
{
	g_free(h);
}


static void
route_segments_free(struct route *this)
{
	struct route_segment *curr,*next;
	curr=this->route_segments;
	while (curr) {
		next=curr->next;
		g_free(curr);
		curr=next;
	}
	this->route_segments=NULL;
}

static void
route_graph_free(struct route *this)
{
	route_points_free(this);
	route_segments_free(this);
}

static int
route_time(struct item *item, int len)
{
	int idx=(item->type-type_street_0);
	if (idx >= sizeof(speed_list)/sizeof(int) || idx < 0) {
		dbg(0,"street idx(%d) out of range [0,%d[", sizeof(speed_list)/sizeof(int));
		return len*36;
	}
	return len*36/speed_list[idx];
}


static int
route_value(struct item *item, int len)
{
	int ret;
	if (len < 0) {
		printf("len=%d\n", len);
	}
	g_assert(len >= 0);
	ret=route_time(item, len);
	dbg(1, "route_value(0x%x, %d)=%d\n", item->type, len, ret);
	return ret;
}

static void
route_process_street_graph(struct route *this, struct item *item)
{
	double len=0;
	struct route_point *s_pnt,*e_pnt;
	struct coord c,l;
	struct attr attr;


	if (item_coord_get(item, &l, 1)) {
		s_pnt=route_point_add(this,&l);
		while (item_coord_get(item, &c, 1)) {
			len+=transform_distance(&l, &c);
			l=c;
		}
		e_pnt=route_point_add(this,&l);
		g_assert(len >= 0);
		if (item_attr_get(item, attr_limit, &attr)) 
			route_segment_add(this, s_pnt, e_pnt, len, item, attr.u.num);
		else
			route_segment_add(this, s_pnt, e_pnt, len, item, 0);
	}
}

static int
compare(void *v1, void *v2)
{
	struct route_point *p1=v1;
	struct route_point *p2=v2;
#if 0
	if (debug_route)
		printf("compare %d (%p) vs %d (%p)\n", p1->value,p1,p2->value,p2);
#endif
	return p1->value-p2->value;
}

static int
route_info_length(struct route_info *rinf, int dir)
{
	int ret;
	struct coord *l;
	int pos;

	ret=transform_distance(&rinf->c, &rinf->lp);
	l=&rinf->lp;
	pos=rinf->pos;
	if (dir > 0)
		pos++;
	while (pos >= 0 && pos < rinf->count) {
		ret+=transform_distance(&rinf->sc[pos], l);
		l=&rinf->sc[pos];
		pos+=dir;
	}
	return ret;
}

static void
route_flood(struct route *this, struct route_info *dst)
{
	struct route_point *p_min;
	struct route_segment *s;
	int min,new,old,val;
	struct fibheap *heap;
	struct route_point *end=NULL;

	heap = fh_makeheap();   
	fh_setcmp(heap, compare);

	if (! (dst->limit & 2)) {
		end=route_get_point(this, &dst->sc[0]);
		g_assert(end != 0);
		end->value=route_value(&dst->item, route_value(&dst->item, route_info_length(dst, -1)));
		end->el=fh_insert(heap, end);
	}

	if (! (dst->limit & 1)) {
		end=route_get_point(this, &dst->sc[dst->count-1]);
		g_assert(end != 0);
		end->value=route_value(&dst->item, route_value(&dst->item, route_info_length(dst, 1)));
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
			val=route_value(&s->item, s->len);
#if 0
			val+=val*2*street_route_contained(s->str->segid);
#endif
			new=min+val;
			if (debug_route)
				printf("begin %d len %d vs %d (0x%x,0x%x)\n",new,val,s->end->value, s->end->c.x, s->end->c.y);
			if (new < s->end->value && !(s->limit & 1)) {
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
			val=route_value(&s->item, s->len);
			new=min+val;
			if (debug_route)
				printf("end %d len %d vs %d (0x%x,0x%x)\n",new,val,s->start->value,s->start->c.x, s->start->c.y);
			if (new < s->start->value && !(s->limit & 2)) {
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

int
route_find(struct route *this, struct route_info *pos, struct route_info *dst)
{
	struct route_point *start1=NULL,*start2=NULL,*start;
	struct route_segment *s=NULL;
	double len=0;
	int ilen,hr,min,time=0,seg_time,seg_len;
	unsigned int val1=0xffffffff,val2=0xffffffff;

	if (! (pos->limit & 1)) {
		start1=route_get_point(this, &pos->sc[0]);
		if (! start1)
			return 0;
		val1=start1->value+route_info_length(pos, -1);
		printf("start1: %d(route)+%d=%d\n", start1->value, val1-start1->value, val1);
	}
	if (! (pos->limit & 2)) {
		start2=route_get_point(this, &pos->sc[pos->count-1]);
		if (! start2)
			return 0;
		val2=start2->value+route_info_length(pos, 1);
		printf("start2: %d(route)+%d=%d\n", start2->value, val2-start2->value, val2);
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
			return 0;
		}
	}
	dbg(0,"dir=%d\n", pos->dir);	
	while ((s=start->seg)) {
#if 0
		printf("start->value=%d 0x%x,0x%x\n", start->value, start->c.x, start->c.y);
#endif
		seg_len=s->len;
		seg_time=route_time(&s->item, seg_len);
		len+=seg_len;
		time+=seg_time;
		if (s->start == start) {
			route_path_add_item(this, &s->item, &s->start->c, &s->end->c, seg_len, seg_time);
			start=s->end;
		} else {
			route_path_add_item(this, &s->item, &s->end->c, &s->start->c, seg_len, seg_time);
			start=s->start;
		}
	}
	printf("start->value=%d 0x%x,0x%x\n", start->value, start->c.x, start->c.y);
	printf("dst->limit=%d dst->sc[0]=0x%x,0x%x dst->sc[dst->count-1]=0x%x,0x%x\n", dst->limit, dst->sc[0].x,dst->sc[0].y, dst->sc[dst->count-1].x, dst->sc[dst->count-1].y);
	if (start->c.x == dst->sc[0].x && start->c.y == dst->sc[0].y)
		dst->dir=-1;
	else if (start->c.x == dst->sc[dst->count-1].x && start->c.y == dst->sc[dst->count-1].y)
		dst->dir=1;
	else {
		printf("no route found\n");
		return 0;
	}
	ilen=route_info_length(pos, pos->dir);
	time+=route_time(&pos->item, ilen);
	len+=ilen;

	ilen=route_info_length(dst, dst->dir);
	time+=route_time(&dst->item, ilen);
	len+=ilen;

	dbg(0, "len %5.3f\n", len/1000);
	this->route_time_val=time/10;
	time/=10;
	this->route_len_val=len;
	min=time/60;
	time-=min*60;
	hr=min/60;
	min-=hr*60;
	dbg(0, "time %02d:%02d:%02d (%d sec)\n", hr, min, time, (int)this->route_time_val);
	dbg(0, "speed %f km/h\n", len/this->route_time_val*3.6);
	navigation_path_description(this, this->ms, this->pos, this->dst);
	return 1;
}

static void
route_build_graph(struct route *this, struct coord *c1, struct coord *c2)
{
	struct map_selection *sel;
	struct mapset_handle *h;
	struct map_rect *mr;
	struct map *m;
	struct item *item;
	struct coord e;

	sel=route_calc_selection(c1, c2);
        h=mapset_open(this->ms);
        while ((m=mapset_next(h,1))) {
		mr=map_rect_new(m, sel);
		while ((item=map_rect_get_item(mr))) {
			if (item->type >= type_street_0 && item->type <= type_ferry) {
				route_process_street_graph(this, item);
			} else 
				while (item_coord_get(item, &e, 1));
		}
		map_rect_destroy(mr);
        }
        mapset_close(h);

}

struct route_info *
route_find_nearest_street(struct route *this, struct coord *c)
{
	struct route_info *ret=NULL;
	int max_dist=1000;
	struct map_selection *sel=route_rect(18, c, c, 0, max_dist);
	int count,dist,pos;
	struct mapset_handle *h;
	struct map *m;
	struct map_rect *mr;
	struct item *item;
	struct coord lp, sc[1000];
	struct attr attr;

        h=mapset_open(this->ms);
        while ((m=mapset_next(h,1))) {
		mr=map_rect_new(m, sel);
               while ((item=map_rect_get_item(mr))) {
			if (item->type >= type_street_0 && item->type <= type_ferry) {
				count=0;
				while (count < 1000) {
					if (!item_coord_get(item, &sc[count], 1))
						break;
					count++;
				}
				g_assert(count < 1000);
				dist=transform_distance_polyline_sq(sc, count, c, &lp, &pos);
				if (!ret || dist < ret->dist) {
					g_free(ret);
					ret=g_malloc(sizeof(struct route_info)+count*sizeof(struct coord));
					if (item_attr_get(item, attr_limit, &attr)) {
						ret->limit=attr.u.num;
						dbg(1,"limit=%d\n", ret->limit);
					} else
						ret->limit=0;
					ret->dist=dist;
					ret->item=*item;
					ret->c=*c;
					ret->pos=pos;
					ret->lp=lp;
					ret->count=count;
					memcpy(ret->sc, sc, count*sizeof(struct coord));
					dbg(1,"dist=%d id 0x%x 0x%x\n", dist, item->id_hi, item->id_lo);
				}
			} else 
				while (item_coord_get(item, &sc[0], 1));
                }  
		map_rect_destroy(mr);
        }
        mapset_close(h);
	
	return ret;
}

#include "point.h"
#include "projection.h"

struct route_info_handle {
	struct route_info *start;
	struct route_info *end;
	struct coord *last;
	int count;
	int iter;
	int pos;
	int endpos;
};

struct route_info_handle *
route_info_open(struct route_info *start, struct route_info *end)
{
	struct route_info_handle *ret=g_new0(struct route_info_handle, 1);

	dbg(1,"enter\n");
	ret->start=start;
	ret->end=end;
	ret->pos=start->pos;
	if (end) {
		if (start->item.map != end->item.map || start->item.id_hi != end->item.id_hi || start->item.id_lo != end->item.id_lo) {
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

	if (start->dir > 0) {
		ret->pos++;
		ret->endpos++;
	}
	dbg(1,"return %p\n",ret);
	return ret;
}

struct coord *
route_info_get(struct route_info_handle *h)
{
	struct coord *new;
	for (;;) {
		new=NULL;
		switch(h->iter) {
		case 0:
			new=&h->start->c;
			h->iter++;
			break;
		case 1:
			new=&h->start->lp;
			h->iter++;
			break;
		case 2:
			if (h->start->dir && h->pos >= 0 && h->pos < h->start->count && (h->end == NULL || h->endpos!=h->pos)) {
				new=&h->start->sc[h->pos];
				h->pos+=h->start->dir;
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




static int
route_draw_route_info(struct route_info *start, struct route_info *end, struct transformation *t, GHashTable *dsp)
{
	struct route_info_handle *h;
	struct coord *c;
	struct coord_rect r;
	struct item item;
	struct point pnt[100];
	int count=0;

	item.id_lo=0;
	item.id_hi=0;
	item.map=NULL;
	item.type=type_street_route;

	h=route_info_open(start, end);
	dbg(1,"h=%p\n", h);
	if (! h)
		return 0;
	c=route_info_get(h);
	dbg(1,"c=%p\n", c);
	r.lu=*c;
	r.rl=*c;
	while (c && count < 100) {
		dbg(1,"0x%x,0x%x\n", c->x, c->y);
		transform(t, projection_mg, c, &pnt[count++]);
		coord_rect_extend(&r, c);
		c=route_info_get(h);
	}
	if (count && transform_contains(t, projection_mg, &r))
		display_add(dsp, &item, count, pnt, "Route");
	route_info_close(h);
	return 1;
}

void
route_draw(struct route *this, struct transformation *t, GHashTable *dsp)
{
	dbg(1,"enter\n");
	if (! this->pos || ! this->dst)
		return;
	if (! route_draw_route_info(this->pos, this->dst, t, dsp)) {
		route_draw_route_info(this->pos, NULL, t, dsp);
		route_draw_route_info(this->dst, NULL, t, dsp);
	}
	dbg(1,"exit\n");
}

#if 0
struct route_crossings *
route_crossings_get(struct route *this, struct coord *c)
{
      struct route_point *pnt;
      struct route_segment *seg;
      int crossings=0;
      struct route_crossings *ret;

       pnt=route_get_point(this, c);
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
