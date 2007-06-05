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
	1, /* street_0 */
	10, /* street_1_city */
	30, /* street_2_city */
	50, /* street_3_city */
	60, /* street_4_city */
	80, /* highway_city */
	60, /* street_1_land */
	80, /* street_2_land */
	80, /* street_3_land */
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
#if 0
	int conn;
	int id;
#endif
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
	int offset;
};

#if 0
struct street_info {
	struct street_header *hdr;
	struct street_type *typ;
	struct street_str *str;
	unsigned char *p;
	int bytes;
	int include;
};

#endif

struct route_info {
	int dist;
	struct item item;
	struct coord c;
	int pos;
	struct coord lp;
	int count;
	struct coord sc[0];
#if 0
	int mode;
	struct coord3d seg1,seg2,line1,line2,pos,click;
	int seg1_len,seg2_len;
	int offset;
	int dist;
	struct block_info blk_inf;
	struct street_info str_inf;
#endif
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
	if (this->path_hash) {
		g_hash_table_destroy(this->path_hash);
		this->path_hash=NULL;
	}
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

#if 0

void
route_mapdata_set(struct route *this, struct map_data *mdata)
{
	this->map_data=mdata;
}

struct map_data *
route_mapdata_get(struct route *this)
{
	return this->map_data;
}

static void
route_add_path_segment(struct route *this, int segid, int offset, struct coord *start, struct coord *end, int dir, int len, int time)
{
        struct route_path_segment *segment=g_new0(struct route_path_segment,1);
        segment->next=NULL;
        segment->segid=segid;
        segment->offset=offset;
	segment->dir=dir;
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
#endif

static void
route_add_path_item(struct route *this, struct item *itm)
{
	struct item *hitm=g_new(struct item, 1);
	*hitm=*itm;
	g_hash_table_insert(this->path_hash, hitm, (void *)1);
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
		if (!route_find(this, this->pos)) {
			route_graph_free(this);
			profile(0,NULL);
			route_build_graph(this, &this->pos->c, &this->dst->c);
			profile(1,"map query");
			route_flood(this, this->dst);
			profile(1,"flood");
			route_find(this, this->pos);
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

#if 0

struct route_path_segment *
route_path_get_all(struct route *this)
{
	return this->path;
}

struct route_path_segment *
route_path_get(struct route *this, int segid)
{
	struct route_path_segment *curr=this->path;

	while (curr) {
                if (curr->segid == segid)
			return curr;
		curr=curr->next;
	}
        return NULL;

}

#endif

struct map_selection *route_selection;

static struct map_selection *
route_rect(int order, struct coord *c1, struct coord *c2, int rel, int abs)
{
	int dx,dy,sx=1,sy=1,d,m;
	struct map_selection *sel=g_new(struct map_selection, 1);
	sel->order[layer_town]=0;
	sel->order[layer_poly]=0;
	sel->order[layer_street]=order;
	dbg(0,"%p %p\n", c1, c2);
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
	if (! this->dst)
		return;
	profile(1,"find_nearest_street");

	route_graph_free(this);
	route_build_graph(this, &this->pos->c, &this->dst->c);
	profile(1,"map query");

	route_flood(this, this->dst);
	profile(1,"flood");

	route_path_free(this);
	route_path_init(this);
	route_find(this, this->pos);
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

#if 0
struct coord *
route_get_destination(struct route *this)
{
	if (! this->dst)
		return NULL;
	return &this->dst->click.xy;
}

static void
route_street_foreach(struct block_info *blk_inf, unsigned char *p, unsigned char *end, void *data,
			void(*func)(struct block_info *, struct street_info *, unsigned char **, unsigned char *, void *))
{
	struct street_info str_inf;
	struct street_str *str,*str_tmp;

	if (blk_inf->block_number == 0x10c6) {
		printf("route_street_foreach p=%p\n", p);
	}
	str_inf.hdr=(struct street_header *)p;
	p+=sizeof(struct street_header);
	assert(str_inf.hdr->count == blk_inf->block->count);

	str_inf.bytes=street_get_bytes(blk_inf->block);	
	
	str_inf.typ=(struct street_type *)p;
	p+=blk_inf->block->count*sizeof(struct street_type);  

	str=(struct street_str *)p;
	str_tmp=str;
	while (str_tmp->segid)
		str_tmp++;

	p=(unsigned char *)str_tmp;	
	p+=4;
	
	while (str->segid) {
		str_inf.include=(str[1].segid > 0);
		str_inf.str=str;
		str_inf.p=p;
		func(blk_inf, &str_inf, &p, end-4, data);
		if (str_inf.include)
			str_inf.typ++;
		str++;
	}
}


#endif
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
route_point_add(struct route *this, struct coord *f, int conn)
{
	int hashval;
	struct route_point *p;

	p=route_get_point(this,f);
	if (p) {
#if 0
		p->conn+=conn;
#endif
	} else {
		hashval=(f->x +  f->y) & (HASH_SIZE-1);
		if (debug_route)
			printf("p (0x%x,0x%x)\n", f->x, f->y);
		p=g_new(struct route_point,1);
		p->hash_next=this->hash[hashval];
		this->hash[hashval]=p;
		p->next=this->route_points;
#if 0
		p->conn=conn;
		p->id=++id;
#endif
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
route_segment_add(struct route *this, struct route_point *start, struct route_point *end, int len, struct item *item, int offset, int limit)
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
	s->offset=offset;
	s->limit=limit;
	s->next=this->route_segments;
	this->route_segments=s;
	if (debug_route)
		printf("l (0x%x,0x%x)-(0x%x,0x%x)\n", start->c.x, start->c.y, end->c.x, end->c.y);
	
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

#if 0
void
route_display_points(struct route *this, struct container *co)
{
#if 0
	GtkMap *map=co->map;
	struct route_point *p=this->route_points;
	int r=5;
	struct point pnt; 
	char text[256];

	while (p) {
		if (transform(co->trans, &p->c.xy, &pnt)) {
			gdk_draw_arc(GTK_WIDGET(map)->window, map->gc[GC_BLACK], FALSE, pnt.x-r/2, pnt.y-r/2, r, r, 0, 64*360);
			if (p->value != -1) {
				sprintf(text,"%d", p->value);
#if 0
				display_text(GTK_WIDGET(map)->window, map->gc[GC_TEXT_FG], map->gc[GC_TEXT_BG], map->face[0], text, pnt.x+6, pnt.y+4, 0x10000, 0);
#endif
			}
		}
		p=p->next;
	}
#endif
}
#endif

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
	if (len < 0) {
		printf("len=%d\n", len);
	}
	g_assert(len >= 0);
	return route_time(item, len);
}

#if 0

static int
route_get_height(int segid, struct coord *c)
{
	if (c->x == 0x141b53 && c->y == 0x5f2065 && (segid == 0x4fad2fa || segid == 0x4fad155)) 
		return 1;
	if (c->x == 0x1477a7 && c->y == 0x5fac38 && (segid == 0x32adac2 || segid == 0x40725c6)) 
		return 1;
	if (c->x == 0x147a4c && c->y == 0x5fb194 && (segid == 0x32adb17 || segid == 0x32adb16)) 
		return 1;
	return 0;
}

#endif

static void
route_process_street_graph(struct route *this, struct item *item)
{
	double len=0;
	struct route_point *s_pnt,*e_pnt;
	struct coord c,l;


	if (item_coord_get(item, &l, 1)) {
		s_pnt=route_point_add(this,&l, 1);
		while (item_coord_get(item, &c, 1)) {
			len+=transform_distance(&l, &c);
			l=c;
		}
		e_pnt=route_point_add(this,&l, 1);
		g_assert(len >= 0);
		route_segment_add(this, s_pnt, e_pnt, len, item, 0, 0);
	}
	
#if 0
	limit=str_inf->str->limit;
	if (str_inf->str->limit == 0x30 && (str_inf->str->type & 0x40))
		limit=0x03;
	if (str_inf->str->limit == 0x03 && (str_inf->str->type & 0x40))
		limit=0x30;

	if (str_inf->str->limit != 0x33)
#endif
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


static void
route_flood(struct route *this, struct route_info *dst)
{
	struct route_point *p_min;
	struct route_segment *s;
	int min,new,old,val;
	struct fibheap *heap;
	struct route_point *end;
 
	end=route_get_point(this, &dst->sc[0]);
	g_assert(end != 0);
	heap = fh_makeheap();   
	fh_setcmp(heap, compare);
	end->value=0;
	end->el=fh_insert(heap, end);
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
			if (new < s->end->value && !(s->limit & 0x30)) {
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
			if (new < s->start->value && !(s->limit & 0x03)) {
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
route_find(struct route *this, struct route_info *pos)
{
	struct route_point *start;
	struct route_segment *s=NULL;
	double len=0,slen;
	int ret=0,hr,min,time=0,seg_time,dir,type;
	unsigned int val1=0xffffffff,val2=0xffffffff;

	start=route_get_point(this, &pos->sc[0]);

	if (! start)
		return 0;
#if 0
	start1=route_get_point(this, &rt_start->seg1);	
	start2=route_get_point(this, &rt_start->seg2);	
	assert(start1 != 0);
	assert(start2 != 0);
	if (start1->value != -1) 
		val1=start1->value+route_value(rt_start->str_inf.str->type, rt_start->seg1_len);
	if (start2->value != -1) 
		val2=start2->value+route_value(rt_start->str_inf.str->type, rt_start->seg2_len);

	route_add_path_segment(this, 0, 0, &rt_start->click.xy, &rt_start->pos.xy, 1, 0, 0);
	type=rt_start->str_inf.str->type;
	if (val1 < val2) {
		ret=1;
		start=start1;
		slen=transform_distance(&rt_start->pos.xy, &rt_start->line1.xy);
		route_add_path_segment(this, 0, 0, &rt_start->pos.xy, &rt_start->line1.xy, 1, slen, route_time(type, slen));
		route_add_path_segment(this, rt_start->str_inf.str->segid, rt_start->offset, NULL, NULL, -1, rt_start->seg1_len, route_time(type, rt_start->seg1_len));
	}
	else {
		ret=2;
		start=start2;
		slen=transform_distance(&rt_start->pos.xy, &rt_start->line2.xy);
		route_add_path_segment(this, 0, 0, &rt_start->pos.xy, &rt_start->line2.xy, 1, slen, route_time(type, slen));
		route_add_path_segment(this, rt_start->str_inf.str->segid, -rt_start->offset, NULL, NULL, 1, rt_start->seg2_len, route_time(type, rt_start->seg2_len));
	}
#endif

	while (start->value) {
#if 0
		printf("start->value=%d 0x%x,0x%x\n", start->value, start->c.x, start->c.y);
#endif
		s=start->seg;
		if (! s) {
			printf("No Route found\n");
			return 0;
		}
		if (s->start == start) {
			start=s->end;
			dir=1;
		}
		else {
			start=s->start;
			dir=-1;
		}
		len+=s->len;
		seg_time=route_time(&s->item, s->len);
		time+=seg_time;
		route_add_path_item(this, &s->item);
	}
#if 0
	printf("start->value=%d 0x%x,0x%x\n", start->value, start->c.x, start->c.y);
#endif
#if 0
	if (s->start->c.xy.x == rt_end->seg1.xy.x && s->start->c.xy.y == rt_end->seg1.xy.y) 
		route_add_path_segment(this, 0, 0, &rt_end->pos.xy, &rt_end->line1.xy, -1, 0, 0);
	else
		route_add_path_segment(this, 0, 0, &rt_end->pos.xy, &rt_end->line2.xy, -1, 0, 0);
	route_add_path_segment(this, 0, 0, &rt_end->click.xy, &rt_end->pos.xy, -1, 0, 0);
#endif
	printf("len %5.3f\n", len/1000);
	this->route_time_val=time/10;
	time/=10;
	this->route_len_val=len;
	min=time/60;
	time-=min*60;
	hr=min/60;
	min-=hr*60;
	printf("time %02d:%02d:%02d\n", hr, min, time);
#if 0
	navigation_path_description(this);
#endif
	return 1;
}

#if 0

struct block_list {
	struct block_info blk_inf;
	unsigned char *p;
	unsigned char *end;
	struct block_list *next;
};

static void
route_process_street_block_graph(struct block_info *blk_inf, unsigned char *p, unsigned char *end, void *data)
{
	struct route *this=data;
	struct block_list *blk_lst=this->blk_lst;
	
	while (blk_lst) {
		if (blk_lst->blk_inf.block_number == blk_inf->block_number && blk_lst->blk_inf.file == blk_inf->file)
			return;
		blk_lst=blk_lst->next;
	}
	blk_lst=g_new(struct block_list,1);
	blk_lst->blk_inf=*blk_inf;
	blk_lst->p=p;
	blk_lst->end=end;
	blk_lst->next=this->blk_lst;
	this->blk_lst=blk_lst;
#if 0
	route_street_foreach(blk_inf, p, end, data, route_process_street_graph);
#endif
}

static void
route_blocklist_free(struct route *this)
{
	struct block_list *curr,*next;
	curr=this->blk_lst;
	while (curr) {
		next=curr->next;
		g_free(curr);
		curr=next;
	}
}

#endif

static void
route_build_graph(struct route *this, struct coord *c1, struct coord *c2)
{
	struct route_info *rt;
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

#if 0
static void
route_process_street3(struct block_info *blk_inf, struct street_info *str_inf, unsigned char **p, unsigned char *end, void *data)
{
	int flags=0;
	int i,ldist;
	struct coord3d first,f,o,l;
	struct coord3d cret;
	int match=0;
	double len=0,len_p=0;
	struct route_info *rt_inf=(struct route_info *)data;

	street_get_coord(p, str_inf->bytes, blk_inf->block->c, &f.xy);
	f.h=route_get_height(str_inf->str->segid, &f.xy);

	l=f;
	o=f;
	first=f;
	i=0;

	while (*p < end) {
		flags=street_get_coord(p, str_inf->bytes, blk_inf->block->c, &f.xy);
		f.h=route_get_height(str_inf->str->segid, &f.xy);
		if (flags && !str_inf->include)
			break;
		
		if (i++) {
			ldist=transform_distance_line_sq(&l.xy, &o.xy, &rt_inf->click.xy, &cret.xy);
			if (ldist < rt_inf->dist) {
				rt_inf->dist=ldist;
				rt_inf->seg1=first;
				rt_inf->line1=l;
				rt_inf->pos=cret;
				rt_inf->blk_inf=*blk_inf;
				rt_inf->str_inf=*str_inf;
				rt_inf->line2=o;
				rt_inf->offset=i-1;
				len_p=len;
				match=1;
			}
			if (rt_inf->mode == 1)
				len+=transform_distance(&l.xy, &o.xy);
		}
		l=o;
		o=f;
		if (flags)
			break;
        }
	ldist=transform_distance_line_sq(&l.xy, &o.xy, &rt_inf->click.xy, &cret.xy);
	if (ldist < rt_inf->dist) {
		rt_inf->dist=ldist;
		rt_inf->seg1=first;
		rt_inf->line1=l;
		rt_inf->pos=cret;
		rt_inf->blk_inf=*blk_inf;
		rt_inf->str_inf=*str_inf;
		rt_inf->line2=o;
		rt_inf->offset=i;
		len_p=len;
		match=1;
	}
	if (match) {
		rt_inf->seg2=o;
		if (rt_inf->mode == 1) {
			len+=transform_distance(&l.xy, &o.xy);
			len_p+=transform_distance(&rt_inf->pos.xy, &rt_inf->line1.xy);
			rt_inf->seg1_len=len_p;
			rt_inf->seg2_len=len-len_p;
		}
	}
	*p-=2*str_inf->bytes;
}


static void
route_process_street_block(struct block_info *blk_inf, unsigned char *p, unsigned char *end, void *data)
{
	route_street_foreach(blk_inf, p, end, data, route_process_street3);
}

struct street_str *
route_info_get_street(struct route_info *rt)
{	
	return rt->str_inf.str;
}

struct block_info *
route_info_get_block(struct route_info *rt)
{
	return &rt->blk_inf;
}

#endif

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

#if 0
void
route_find_point_on_street(struct route_info *rt_inf)
{
	unsigned char *p,*end;
	
	rt_inf->dist=INT_MAX;	
	rt_inf->mode=1;

	p=rt_inf->str_inf.p;
	end=(unsigned char *)rt_inf->blk_inf.block;
	end+=rt_inf->blk_inf.block->size;

	route_process_street3(&rt_inf->blk_inf, &rt_inf->str_inf, &p, end, rt_inf);
}


struct route_info *start,*end;
int count;

void
route_click(struct route *this, struct container *co, int x, int y)
{
#if 0
	GtkMap *map=co->map;
	struct point pnt;
	GdkBitmap *flag_mask;
	GdkPixmap *flag;
	struct coord c;
	struct route_info *rt_inf;
	GdkGC *gc;
	

	pnt.x=x;
	pnt.y=y;
	transform_reverse(co->trans, &pnt, &c);
	transform(co->trans, &c, &pnt);
	rt_inf=route_find_nearest_street(co->map_data, &c);


	route_find_point_on_street(rt_inf);

	flag=gdk_pixmap_create_from_xpm_d(GTK_WIDGET(map)->window, &flag_mask, NULL, flag_xpm);
	gc=gdk_gc_new(map->DrawingBuffer);      

	gdk_gc_set_clip_origin(gc,pnt.x, pnt.y-15);
	gdk_gc_set_clip_mask(gc,flag_mask);
	gdk_draw_pixmap(GTK_WIDGET(map)->window,
                        gc,
                        flag,
                        0, 0, pnt.x, pnt.y-15, 16, 16);            	
	printf("Segment ID 0x%lx\n", rt_inf->str_inf.str->segid);
#if 0
	printf("Segment Begin 0x%lx, 0x%lx, 0x%x\n", route_info.seg1.xy.x, route_info.seg1.xy.y, route_info.seg1.h);
	printf("Segment End 0x%lx, 0x%lx, 0x%x\n", route_info.seg2.xy.x, route_info.seg2.xy.y, route_info.seg2.h);
#endif

#if 0
	transform(map, &route_info.seg1.xy, &pnt); gdk_draw_arc(GTK_WIDGET(map)->window, map->gc[GC_BLACK], TRUE, pnt.x-r/2, pnt.y-r/2, r, r, 0, 64*360);
	transform(map, &route_info.line1.xy, &pnt); gdk_draw_arc(GTK_WIDGET(map)->window, map->gc[GC_BLACK], TRUE, pnt.x-r/2, pnt.y-r/2, r, r, 0, 64*360);
	transform(map, &route_info.seg2.xy, &pnt); gdk_draw_arc(GTK_WIDGET(map)->window, map->gc[GC_BLACK], TRUE, pnt.x-r/2, pnt.y-r/2, r, r, 0, 64*360);
	transform(map, &route_info.line2.xy, &pnt); gdk_draw_arc(GTK_WIDGET(map)->window, map->gc[GC_BLACK], TRUE, pnt.x-r/2, pnt.y-r/2, r, r, 0, 64*360);
	transform(map, &route_info.pos.xy, &pnt); gdk_draw_arc(GTK_WIDGET(map)->window, map->gc[GC_BLACK], TRUE, pnt.x-r/2, pnt.y-r/2, r, r, 0, 64*360);
#endif
	printf("offset=%d\n", rt_inf->offset);
	printf("seg1_len=%d\n", rt_inf->seg1_len);
	printf("seg2_len=%d\n", rt_inf->seg2_len);

	if (trace) {
		start=rt_inf;
		count=0;
		route_path_free(this);
		route_find(this, start, end);
		map_redraw(map);
	} else {
		if (! count) {
			start=rt_inf;
			count=1;
		}
		else {
			end=rt_inf;
			count=0;
		}
	}
#endif
}

void
route_start(struct route *this, struct container *co)
{
	route_do_start(this, end, start);
}

void
route_trace(struct container *co)
{
	trace=1-trace;
}

static void
route_data_free(void *t)
{
	route_blocklist_free(t);
	route_path_free(t);
	route_points_free(t);
	route_segments_free(t);
}

void
route_do_start(struct route *this, struct route_info *rt_start, struct route_info *rt_end)
{
	int res;
	struct route_point *seg1,*seg2,*pos;
	struct coord c[2];
	struct timeval tv[4];

	phrase_route_calc(speech_handle);
	route_data_free(this);
	gettimeofday(&tv[0], NULL);
	c[0]=rt_start->pos.xy;
	c[1]=rt_end->pos.xy;
	route_build_graph(this,this->map_data,c,2);
	gettimeofday(&tv[1], NULL);
	seg1=route_point_add(this, &rt_end->seg1, 1);
	pos=route_point_add(this, &rt_end->pos, 2);
	seg2=route_point_add(this ,&rt_end->seg2, 1);
	route_segment_add(this, seg1, pos, rt_end->seg1_len, rt_end->str_inf.str, rt_end->offset, 0);
	route_segment_add(this, seg2, pos, rt_end->seg2_len, rt_end->str_inf.str, -rt_end->offset, 0);

	printf("flood\n");
	route_flood(this, rt_end);
	gettimeofday(&tv[2], NULL);
	printf("find\n");
	res=route_find(this, rt_start, rt_end);
	printf("ok\n");
	gettimeofday(&tv[3], NULL);

	printf("graph time %ld\n", (tv[1].tv_sec-tv[0].tv_sec)*1000+(tv[1].tv_usec-tv[0].tv_usec)/1000);
	printf("flood time %ld\n", (tv[2].tv_sec-tv[1].tv_sec)*1000+(tv[2].tv_usec-tv[1].tv_usec)/1000);
	printf("find time %ld\n", (tv[3].tv_sec-tv[2].tv_sec)*1000+(tv[3].tv_usec-tv[2].tv_usec)/1000);
	phrase_route_calculated(speech_handle, this);
	
}


int
route_destroy(void *t)
{
	struct route *this=t;

	route_data_free(t);
	if (this->pos)
		g_free(this->pos);
	if (this->dst)
		g_free(this->dst);
	g_free(this);
	return 0;
}


struct tm *
route_get_eta(struct route *this)
{
	time_t eta;

        eta=time(NULL)+this->route_time_val;

        return localtime(&eta);
}

double
route_get_len(struct route *this)
{
	return this->route_len_val;
}

struct route_crossings *
route_crossings_get(struct route *this, struct coord *c)
{
	struct coord3d c3;
	struct route_point *pnt;
	struct route_segment *seg;
	int crossings=0;
	struct route_crossings *ret;

	c3.xy=*c;
	c3.h=0;	
	pnt=route_get_point(this, &c3);
	seg=pnt->start;
	while (seg) {
		crossings++;
		seg=seg->start_next;
	}
	seg=pnt->end;
	while (seg) {
		crossings++;
		seg=seg->end_next;
	}
	ret=g_malloc(sizeof(struct route_crossings)+crossings*sizeof(struct route_crossing));
	ret->count=crossings;
	return ret;
}
#endif
