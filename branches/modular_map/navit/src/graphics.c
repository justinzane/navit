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
#include "display.h"
#include "data_window.h"
#include "profile.h"


#define GC_BACKGROUND 0
#define GC_WOOD 1
#define GC_TOWN_FILL 2
#define GC_TOWN_LINE 3
#define GC_WATER_FILL 4
#define GC_WATER_LINE 5
#define GC_RAIL 6
#define GC_TEXT_FG 7
#define GC_TEXT_BG 8
#define GC_BLACK 9
#define GC_STREET_SMALL 10
#define GC_STREET_SMALL_B 11
#define GC_PARK 12
#define GC_BUILDING 13
#define GC_BUILDING_2 14
#define GC_STREET_MID 15
#define GC_STREET_MID_B 16
#define GC_STREET_BIG 17
#define GC_STREET_BIG_B 18
#define GC_STREET_BIG2 19
#define GC_STREET_BIG2_B 20
#define GC_STREET_BIG2_L 21
#define GC_STREET_NO_PASS 22
#define GC_STREET_ROUTE 23
#define GC_RAIL1 24
#define GC_BORDER 25
#define GC_LAST	26


int color[][3]={
	{0xffff, 0xefef, 0xb7b7},
	{0x8e8e, 0xc7c7, 0x8d8d},
	{0xffff, 0xc8c8, 0x9595},
	{0xebeb, 0xb4b4, 0x8181},
	{0x8282, 0xc8c8, 0xeaea},
	{0x5050, 0x9696, 0xb8b8},
	{0x8080, 0x8080, 0x8080},
	{0x0, 0x0, 0x0},
	{0xffff, 0xffff, 0xffff},
	{0x0, 0x0, 0x0},
	{0xffff, 0xffff, 0xffff},
	{0xe0e0, 0xe0e0, 0xe0e0},
	{0x7c7c, 0xc3c3, 0x3434},
	{0xe6e6, 0xe6e6, 0xe6e6},
	{0xffff, 0x6666, 0x6666},
	{0xffff, 0xffff, 0x0a0a},
	{0xe0e0, 0xe0e0, 0xe0e0},
	{0xffff, 0x0000, 0x0000},
	{0x0000, 0x0000, 0x0000},
	{0xffff, 0xffff, 0x0a0a},
	{0xffff, 0x0000, 0x0000},
	{0xffff, 0x0000, 0x0000},
	{0xe0e0, 0xe0e0, 0xffff},
	{0x0000, 0x0000, 0xa0a0},
	{0xffff, 0xffff, 0xffff},
	{0x8080, 0x8080, 0x8080},
};

void
graphics_parse(void)
{
	FILE *f;
	char line[1024];
	char mode[1024];
	char item[1024];
	char type[1024];
	int limit;
	int len;

	printf("graphics_parse\n");
	f=fopen("graphics.txt","r");
	while (fgets(line, 1024, f)) {
		len=strlen(line);
		if (len && line[len-1] == '\n') 
			line[len-1]='\0';
		printf("line '%s'\n", line);
		sscanf(line, "%s\t%s\t%d\t%s", mode, item, &limit, type);
		printf("mode='%s' item='%s' limit=%d type='%s'\n", mode, item, limit, type);
		if (! strcasecmp(type,"l")) {
		}
	}
	fclose(f);
}


void
container_init_gra(struct container *co)
{
	struct graphics *gra=co->gra;
	int i;

	gra->font=g_new0(struct graphics_font *,3);
	gra->font[0]=gra->font_new(gra,140);
	gra->font[1]=gra->font_new(gra,200);
	gra->font[2]=gra->font_new(gra,300);
	gra->gc=g_new0(struct graphics_gc *, GC_LAST);
	for (i = 0 ; i < GC_LAST ; i++) {
		gra->gc[i]=gra->gc_new(gra);
		gra->gc_set_background(gra->gc[i], color[0][0], color[0][1], color[0][2]);
		gra->gc_set_foreground(gra->gc[i], color[i][0], color[i][1], color[i][2]);
	}
	gra->gc_set_background(gra->gc[GC_TEXT_BG], color[7][0], color[7][1], color[7][2]);
	graphics_parse();
	
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

static void
graphics_draw(struct map_data *mdata, int file, struct container *co, int display, int limit, int limit2,
		  void(*func)(struct block_info *, unsigned char *, unsigned char *, void *))
{
	struct draw_info info;
	info.co=co;
	info.display=display;
	info.limit=limit;
	map_data_foreach(mdata, file, co->trans, limit2, func, &info);
}

#include "attr.h"
#include "item.h"
#include <stdio.h>

static void
do_draw_poly(struct display_list *d, int line, struct transformation *t,struct item *item)
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
		transform(t, &c, &pnt[count]);
		count++;
			
	}
	g_assert(count < max);
	if (coord_rect_overlap(&t->r, &r)) {
		if (!item_attr_get(item, attr_name, &attr))
			item_attr_get(item, attr_name_systematic, &attr);
		display_add(d, line, 0, attr.u.str, count, pnt, NULL, NULL, NULL);	
	}
}

static void
do_draw_label(struct display_list *d, struct transformation *t, struct item *item)
{
	struct point pnt;
	struct coord c;
	struct attr attr;
	item_coord_get(item, &c, 1);
	if (transform(t, &c, &pnt)) {
		if (!item_attr_get(item, attr_district, &attr))
			item_attr_get(item, attr_name, &attr); 
		display_add(d, 3, 0, attr.u.str, 1, &pnt, NULL, NULL, 0);
	}
}

static void
do_draw_item(struct container *co, struct item *item)
{
	switch (item->type) {
	case type_town_label:
		do_draw_label(&co->disp[display_town+8], co->trans, item);
		break;
	case type_district_label:
		do_draw_label(&co->disp[display_town+8], co->trans, item);
		break;
	case type_highway_exit_label:
		do_draw_label(&co->disp[display_town+8], co->trans, item);
		break;
	case type_street_0:
		do_draw_poly(&co->disp[display_street], 1, co->trans, item);
		break;
	case type_street_1_city:
		do_draw_poly(&co->disp[display_street], 1, co->trans, item);
		break;
	case type_street_2_city:
		do_draw_poly(&co->disp[display_street1], 1, co->trans, item);
		break;
	case type_street_3_city:
		do_draw_poly(&co->disp[display_street2], 1, co->trans, item);
		break;
	case type_street_4_city:
		do_draw_poly(&co->disp[display_street2], 1, co->trans, item);
		break;
	case type_highway_city:
		do_draw_poly(&co->disp[display_street3], 1, co->trans, item);
		break;
	case type_street_1_land:
		do_draw_poly(&co->disp[display_street], 1, co->trans, item);
		break;
	case type_street_2_land:
		do_draw_poly(&co->disp[display_street1], 1, co->trans, item);
		break;
	case type_street_3_land:
		do_draw_poly(&co->disp[display_street2], 1, co->trans, item);
		break;
	case type_street_4_land:
		do_draw_poly(&co->disp[display_street2], 1, co->trans, item);
		break;
	case type_street_n_lanes:
		do_draw_poly(&co->disp[display_street2], 1, co->trans, item);
		break;
	case type_highway_land:
		do_draw_poly(&co->disp[display_street3], 1, co->trans, item);
		break;
	case type_ramp:
		do_draw_poly(&co->disp[display_street1], 1, co->trans, item);
		break;
	case type_ferry:
		do_draw_poly(&co->disp[display_rail], 1, co->trans, item);
		break;
	case type_street_unkn:
		do_draw_poly(&co->disp[display_street_route], 1, co->trans, item);
		break;
	case type_rail:
		do_draw_poly(&co->disp[display_border], 1, co->trans, item);
		break;
	case type_border_country:
		do_draw_poly(&co->disp[display_border], 1, co->trans, item);
		break;
	case type_border_state:
		do_draw_poly(&co->disp[display_border], 1, co->trans, item);
		break;
	case type_water_line:
		do_draw_poly(&co->disp[display_water], 1, co->trans, item);
		break;
	case type_wood:
		do_draw_poly(&co->disp[display_wood], 0, co->trans, item);
		break;
	case type_water_poly:
		do_draw_poly(&co->disp[display_water], 0, co->trans, item);
		break;
	case type_town_poly:
	case type_airport_poly:
		do_draw_poly(&co->disp[display_other], 0, co->trans, item);
		break;
	case type_industry_poly:
		do_draw_poly(&co->disp[display_other1], 0, co->trans, item);
		break;
	case type_hospital_poly:
		do_draw_poly(&co->disp[display_other2], 0, co->trans, item);
		break;
	case type_park_poly:
		do_draw_poly(&co->disp[display_other3], 0, co->trans, item);
		break;
	case type_sport_poly:
		do_draw_poly(&co->disp[display_other1], 0, co->trans, item);
		break;
	default:
		printf("Unsupported item\n");
		exit(1);
	}
}

static void
do_draw(struct container *co)
{
	struct coord_rect r;
	struct map_rect *mr;
	struct item *item;
	int scale,order,rem;

	extern struct map *map_default1,*map_default2;
	extern struct map *map_default3,*map_default4;
	extern struct map *map_default5,*map_default6;
	scale=transform_get_scale(co->trans);
	printf("scale=%d\n", scale);
	order=0;
	while (scale > 1) {
		order++;
		scale>>=1;
	}
	order=14-order;
	if (order < 0)
		order=0;
	printf("order=%d\n", order);
	
	r=co->trans->r;
	mr=map_rect_new(map_default1, &r, NULL, order);
	while (item=map_rect_get_item(mr)) {
		do_draw_item(co, item);
	}
	map_rect_destroy(mr);
	mr=map_rect_new(map_default2, &r, NULL, order);
	while (item=map_rect_get_item(mr)) {
		do_draw_item(co, item);
	}
	map_rect_destroy(mr);
	mr=map_rect_new(map_default3, &r, NULL, order);
	while (item=map_rect_get_item(mr)) {
		do_draw_item(co, item);
	}
	map_rect_destroy(mr);
	mr=map_rect_new(map_default4, &r, NULL, order);
	while (item=map_rect_get_item(mr)) {
		do_draw_item(co, item);
	}
	map_rect_destroy(mr);
	mr=map_rect_new(map_default5, &r, NULL, order);
	while (item=map_rect_get_item(mr)) {
		do_draw_item(co, item);
	}
	map_rect_destroy(mr);
	mr=map_rect_new(map_default6, &r, NULL, order);
	while (item=map_rect_get_item(mr)) {
		do_draw_item(co, item);
	}
	map_rect_destroy(mr);
}

void
graphics_redraw(struct container *co)
{
	int scale=transform_get_scale(co->trans);
	int i,slimit=255,tlimit=255,plimit=255;
	int bw[4],w[4],t[4];
	struct display_list **disp=co->disp;
	struct graphics *gra=co->gra;
	unsigned char dashes[]={4,4};

#if 0
	printf("scale=%d center=0x%lx,0x%lx mercator scale=%f\n", scale, co->trans->center.x, co->trans->center.y, transform_scale(co->trans->center.y));
#endif
	
	display_free(co->disp, display_end);

	transform_setup_source_rect(co->trans);

	gra->draw_mode(gra, draw_mode_begin);
	for (i = 0 ; i < data_window_type_end; i++) {
		data_window_begin(co->data_window[i]);	
	}
	gra->gc_set_linewidth(gra->gc[GC_RAIL], 3);
	gra->gc_set_linewidth(gra->gc[GC_BORDER], 3);
	gra->gc_set_dashes(gra->gc[GC_RAIL1], dashes, 2);

	bw[0]=0;
	bw[1]=0;
	bw[2]=0;
	bw[3]=0;
	w[0]=1;
	w[1]=1;
	w[2]=1;
	w[3]=1;
	t[0]=0xf;
	t[1]=0xf;
	t[2]=0xf;
	t[3]=0xf;
	if (scale < 2) {
		tlimit=0xff;
		slimit=0xff;
		bw[0]=17;
		w[0]=15;
		bw[1]=19;
		w[1]=17;
		bw[2]=19;
		w[2]=17;
		bw[3]=21;
		w[3]=17;
	} else if (scale < 4) {
		tlimit=0xff;
		slimit=0xff;
		bw[0]=11;
		w[0]=9;
		bw[1]=13;
		w[1]=11;
		bw[2]=13;
		w[2]=11;
		bw[3]=15;
		w[3]=11;
	} else if (scale < 8) {
		tlimit=0xff;
		slimit=0xff;
		bw[0]=5;
		w[0]=3;
		bw[1]=11;
		w[1]=9;
		bw[2]=11;
		w[2]=9;
		bw[3]=13;
		w[3]=9;
		t[0]=0xa;
		t[1]=0xf;
	} else if (scale < 16) {
		tlimit=0xff;
		slimit=0xff;
		bw[1]=9;
		w[1]=7;
		bw[2]=9;
		w[2]=7;
		bw[3]=11;
		w[3]=7;
		t[0]=0x9;
		t[1]=0xe;
	} else if (scale < 32) {
		tlimit=0xff;
		slimit=0xff;
		bw[1]=5;
		w[1]=3;
		bw[2]=5;
		w[2]=3;
		bw[3]=5;
		w[3]=3;
		t[0]=0x8;
		t[1]=0xb;
	} else if (scale < 64) {
		tlimit=0xf;
		slimit=0x6;
		bw[1]=5;
		w[1]=3;
		bw[2]=5;
		w[2]=3;
		bw[3]=5;
		w[3]=3;
		t[0]=0x8;
		t[1]=0xa;
	} else if (scale < 128) {
		tlimit=0xc;
		slimit=0x6;
		plimit=0x1e;
		w[1]=3;
		w[2]=3;
		bw[3]=5;
		w[3]=3;
		t[0]=0x7;
		t[1]=0xa;
	} else if (scale < 256) {
		tlimit=0xb;
		slimit=0x5;
		plimit=0x1a;
		w[2]=3;
		bw[3]=5;
		w[3]=3;
		t[0]=0x7;
		t[1]=0x8;
	} else if (scale < 512) {
		tlimit=0x9;
		slimit=0x5;
		plimit=0x14;
		w[1]=0;
		w[2]=1;
		bw[3]=3;
		w[3]=1;
		t[0]=0x4;
		t[1]=0x7;
	} else if (scale < 1024) {
		tlimit=0x8;
		slimit=0x4;
		slimit=0x4;
		plimit=0x11;
		w[1]=0;
		w[2]=1;
		bw[3]=3;
		w[3]=1;
		t[0]=0x3;
		t[1]=0x5;
	} else if (scale < 2048) {
		tlimit=0x5;
		slimit=0x3;
		plimit=0x10;
		bw[3]=3;
		w[3]=1;
		t[0]=0x2;
		t[1]=0x4;
	} else if (scale < 4096) {
		bw[3]=3;
		w[3]=1;
		tlimit=0x4;
		slimit=0x2;
		plimit=0xf;
		t[0]=0x2;
		t[1]=0x3;
	} else if (scale < 8192) {
		bw[3]=3;
		w[3]=1;
		tlimit=0x3;
		slimit=0x2;
		plimit=0xf;
		t[0]=0x1;
		t[1]=0x2;
	} else {
		bw[3]=3;
		w[3]=1;
		tlimit=0x2;
		slimit=0x2;
		plimit=0xf;
		t[0]=0x1;
		t[1]=0x4;
	}
	gra->gc_set_linewidth(gra->gc[GC_STREET_SMALL], w[0]);
	gra->gc_set_linewidth(gra->gc[GC_STREET_NO_PASS], w[0]);
	gra->gc_set_linewidth(gra->gc[GC_STREET_SMALL_B], bw[0]);
	gra->gc_set_linewidth(gra->gc[GC_STREET_MID], w[1]);
	gra->gc_set_linewidth(gra->gc[GC_STREET_MID_B], bw[1]);
	gra->gc_set_linewidth(gra->gc[GC_STREET_BIG], w[2]);
	gra->gc_set_linewidth(gra->gc[GC_STREET_BIG_B], bw[2]);
	gra->gc_set_linewidth(gra->gc[GC_STREET_BIG2], w[3]);
	gra->gc_set_linewidth(gra->gc[GC_STREET_BIG2_B], bw[3]);
	gra->gc_set_linewidth(gra->gc[GC_STREET_ROUTE], w[3]+7+w[3]/2);

#if 0
	profile_timer(NULL);
	graphics_draw(co->map_data, file_border_ply, co, display_rail, plimit, 48, poly_draw_block);
	graphics_draw(co->map_data, file_woodland_ply, co, display_wood, plimit, 48, poly_draw_block);
	graphics_draw(co->map_data, file_other_ply, co, display_other, plimit, 48, poly_draw_block);
	graphics_draw(co->map_data, file_town_twn, co, display_town, tlimit, 48, town_draw_block);
	graphics_draw(co->map_data, file_water_ply, co, display_water, plimit, 48, poly_draw_block);
	graphics_draw(co->map_data, file_sea_ply, co, display_sea, plimit, 48, poly_draw_block);
	/* todo height, tunnel, bridge, street_bti ??? */
#if 0
	graphics_draw(co->map_data, file_height_ply, co, display_other1, plimit, 48, poly_draw_block);
#endif
	if (scale < 256) {
		graphics_draw(co->map_data, file_rail_ply, co, display_rail, plimit, 48, poly_draw_block);
	}
	profile_timer("map_draw");
	plugin_call_draw(co);
	profile_timer("plugin");
#if 0
	draw_poly(map, &co->d_tunnel_ply, "Tunnel", 0, 11, plimit);
#endif
	graphics_draw(co->map_data, file_street_str, co, display_street, slimit, 7, street_draw_block);
#endif
	do_draw(co);

  
	display_draw(disp[display_sea], gra, gra->gc[GC_WATER_FILL], NULL); 
	display_draw(disp[display_wood], gra, gra->gc[GC_WOOD], NULL); 
	display_draw(disp[display_other], gra, gra->gc[GC_TOWN_FILL], gra->gc[GC_TOWN_LINE]); 
	display_draw(disp[display_other1], gra, gra->gc[GC_BUILDING], NULL); 
	display_draw(disp[display_other2], gra, gra->gc[GC_BUILDING_2], NULL); 
	display_draw(disp[display_other3], gra, gra->gc[GC_PARK], NULL); 
	display_draw(disp[display_water], gra, gra->gc[GC_WATER_FILL], gra->gc[GC_WATER_LINE]); 
	display_draw(disp[display_rail], gra, gra->gc[GC_RAIL], NULL); 
	display_draw(disp[display_rail], gra, gra->gc[GC_RAIL1], NULL); 
	display_draw(disp[display_border], gra, gra->gc[GC_BORDER], NULL); 
#if 0 /* FIXME */
	street_route_draw(co);
#endif
	display_draw(disp[display_street_route], gra, gra->gc[GC_STREET_ROUTE], NULL); 
	if (bw[0]) {
		display_draw(disp[display_street_no_pass], gra, gra->gc[GC_STREET_SMALL_B], NULL); 
		display_draw(disp[display_street], gra, gra->gc[GC_STREET_SMALL_B], NULL); 
	}
	if (bw[1]) 
		display_draw(disp[display_street1], gra, gra->gc[GC_STREET_MID_B], NULL); 
	if (bw[2])
		display_draw(disp[display_street2], gra, gra->gc[GC_STREET_BIG_B], NULL); 
	if (bw[3])
		display_draw(disp[display_street3], gra, gra->gc[GC_STREET_BIG2_B], NULL); 
	if (w[0]) {
		display_draw(disp[display_street_no_pass], gra, gra->gc[GC_STREET_NO_PASS], NULL); 
		display_draw(disp[display_street], gra, gra->gc[GC_STREET_SMALL], NULL); 
	}
	if (w[1]) 
		display_draw(disp[display_street1], gra, gra->gc[GC_STREET_MID], gra->gc[GC_BLACK]); 
	display_draw(disp[display_street2], gra, gra->gc[GC_STREET_BIG], gra->gc[GC_BLACK]); 
	display_draw(disp[display_street3], gra, gra->gc[GC_STREET_BIG2], gra->gc[GC_BLACK]); 
	if (w[3] > 1) 
		display_draw(disp[display_street3], gra, gra->gc[GC_STREET_BIG2_L], NULL); 

	display_draw(disp[display_poi], gra, gra->gc[GC_BLACK], NULL); 


	profile_timer("display_draw");

	if (scale < 2) {
		display_labels(disp[display_street], gra, gra->gc[GC_TEXT_FG], gra->gc[GC_TEXT_BG], gra->font[1]);
		display_labels(disp[display_street1], gra, gra->gc[GC_TEXT_FG], gra->gc[GC_TEXT_BG], gra->font[1]);
	}
	else {
		display_labels(disp[display_street], gra, gra->gc[GC_TEXT_FG], gra->gc[GC_TEXT_BG], gra->font[0]);
		display_labels(disp[display_street1], gra, gra->gc[GC_TEXT_FG], gra->gc[GC_TEXT_BG], gra->font[0]);
	}
	display_labels(disp[display_street2], gra, gra->gc[GC_TEXT_FG], gra->gc[GC_TEXT_BG], gra->font[0]);
	display_labels(disp[display_street3], gra, gra->gc[GC_TEXT_FG], gra->gc[GC_TEXT_BG], gra->font[0]);

	for (i = display_town+t[1] ; i < display_town+0x10 ; i++) 
		display_labels(disp[i], gra, gra->gc[GC_TEXT_FG], gra->gc[GC_TEXT_BG], gra->font[0]);
	for (i = display_town+t[0] ; i < display_town+t[1] ; i++) 
		display_labels(disp[i], gra, gra->gc[GC_TEXT_FG], gra->gc[GC_TEXT_BG], gra->font[1]);
	for (i = display_town ; i < display_town+t[0] ; i++) 
		display_labels(disp[i], gra, gra->gc[GC_TEXT_FG], gra->gc[GC_TEXT_BG], gra->font[2]);

	for (i = display_town ; i < display_town+0x10 ; i++) 
		display_draw(disp[i], gra, gra->gc[GC_BLACK], NULL); 
	display_draw(disp[display_bti], gra, gra->gc[GC_BLACK], NULL); 
	profile_timer("labels");
	gra->draw_mode(gra, draw_mode_end);
	for (i = 0 ; i < data_window_type_end; i++) {
		data_window_end(co->data_window[i]);	
	}
#if 0
	map_scrollbars_update(map);
#endif
}

void
graphics_resize(struct container *co, int w, int h)
{
	co->trans->width=w;
        co->trans->height=h;
	graphics_redraw(co);
}
