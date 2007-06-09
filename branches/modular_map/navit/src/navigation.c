#include <stdio.h>
#include <math.h>
#include <glib.h>
#include "debug.h"
#include "coord.h"
#include "item.h"
#include "route.h"
#if 0
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "coord.h"
#include "graphics.h"
#include "param.h"
#include "block.h"
#include "route.h"
#include "street.h"
#include "street_name.h"
#include "speech.h"
#include "navigation.h"

#endif

#include "data_window.h"

struct data_window *navigation_window;


struct navigation_item {
	char name1[128];
	char name2[128];
	int length;
	int time;
	int crossings_start;
	int crossings_end;
	int angle_start;
	int angle_end;
	int points;	
	struct coord c;
	struct item item;
};


/* 0=N,90=E */
static int
road_angle(struct coord *c1, struct coord *c2, int dir)
{
	double angle;
	int dx=c2->x-c1->x;
	int dy=c2->y-c1->y;
	angle=atan2(dx,dy);
	angle*=180/M_PI;
	if (dir == -1)
		angle=angle-180;
	if (angle < 0)
		angle+=360;
	dbg(1, "road_angle(0x%x,0x%x - 0x%x,0x%x)=%d\n", c1->x, c1->y, c2->x, c2->y, (int) angle);
	return angle;
}

static void
expand_str(char *str)
{
	int len=strlen(str);
	if (len > 4 && !strcmp(str+len-4,"str.")) 
		strcpy(str+len-4,"strasse");
	if (len > 4 && !strcmp(str+len-4,"Str.")) 
		strcpy(str+len-4,"Strasse");
}

extern struct navit *global_navit;
static void
navigation_goto(struct data_window *navigation_window, char **cols)
{
	struct coord c;

	printf("goto %s\n",cols[8]);
	sscanf(cols[8],"%lx,%lx",&c.x,&c.y);
	printf("0x%x,0x%x\n", c.x, c.y);
	navit_set_center(global_navit, &c);
}


static int
is_same_street(struct navigation_item *old, struct navigation_item *new)
{
	if (strlen(old->name2) && !strcmp(old->name2, new->name2)) {
		strcpy(old->name1, new->name1);
		dbg(1,"is_same_street: '%s' '%s' vs '%s' '%s' yes (1.)\n", old->name2, new->name2, old->name1, new->name1);
		return 1;
	}
	if (strlen(old->name1) && !strcmp(old->name1, new->name1)) {
		strcpy(old->name2, new->name2);
		dbg(1,"is_same_street: '%s' '%s' vs '%s' '%s' yes (2.)\n", old->name2, new->name2, old->name1, new->name1);
		return 1;
	}
	strcpy(old->name1, new->name1);
	strcpy(old->name2, new->name2);
	dbg(1,"is_same_street: '%s' '%s' vs '%s' '%s' no\n", old->name2, new->name2, old->name1, new->name1);
	return 0;
}

static int
maneuver_required(struct navigation_item *old, struct navigation_item *new, int *delta)
{
	if (new->item.type == type_ramp && (old->item.type == type_highway_land || old->item.type == type_highway_city)) {
		dbg(1, "maneuver_required: new is ramp from highway: yes\n");
		return 1;
	}
	if (is_same_street(old, new)) {
		dbg(1, "maneuver_required: is_same_street: no\n");
		return 0;
	}
	if (old->crossings_end == 2) {
		dbg(1, "maneuver_required: only 2 connections: no\n");
		return 0;
	}
	*delta=new->angle_start-old->angle_end;
	if (*delta < -180)
		*delta+=360;
	if (*delta > 180)
		*delta-=360;
	if (*delta < 20 && *delta >-20) {
		dbg(1, "maneuver_required: delta(%d) < 20: no\n", *delta);
		return 0;
	}
	dbg(1, "maneuver_required: delta=%d: yes\n", *delta);
	return 1;
}

int flag,old_level;
extern void *speech_handle;

static void
get_distance(char *dst, int dist)
{
	if (dist < 100) {
		dist=(dist+5)/10;
		dist*=10;
		sprintf(dst,"%d Meter", dist);
		return;
	}
	if (dist < 250) {
		dist=(dist+13)/25;
		dist*=25;
		sprintf(dst,"%d Meter", dist);
		return;
	}
	if (dist < 500) {
		dist=(dist+25)/50;
		dist*=50;
		sprintf(dst,"%d Meter", dist);
		return;
	}
	if (dist < 1000) {
		dist=(dist+50)/100;
		dist*=100;
		sprintf(dst,"%d Meter", dist);
		return;
	}
	if (dist < 5000) {
		dist=(dist+50)/100;
		if (dist % 10) 
			sprintf(dst,"%d,%d Kilometer", dist/10,dist%10);
		else
			sprintf(dst,"%d Kilometer", dist/10);
		return;
	}
	if (dist < 100000) {
		dist=(dist+500)/1000;
		sprintf(dst,"%d Kilometer", dist);
		return;
	}
	dist=(dist+5000)/10000;
	dist*=10;
	sprintf(dst,"%d kilometer", dist);
}

struct param_list {
	char *name;
	char *value;
};

static void
make_maneuver(struct navigation_item *old, struct navigation_item *new)
{
	
	int delta,navmode=1,add_dir,level;
	struct param_list param_list[20];

	char angle_old[30],angle_new[30],angle_delta[30],cross_roads[30],position[30],distance[30];
	char command[256],*p,*dir,*strength;
	char dist[256];
	struct navigation_item old_save=*old;

	dbg(1, "new->name1 '%s' new->name2 '%s'\n", new->name1, new->name2);
	param_list[0].name="Name1 Old";
	param_list[0].value=old_save.name1;
	param_list[1].name="Name2 Old";
	param_list[1].value=old_save.name2;
	param_list[2].name="Name1 New";
	param_list[2].value=new->name1;
	param_list[3].name="Name2 New";
	param_list[3].value=new->name2;
	param_list[4].name="Angle\nOld";
	param_list[5].name="Angle\nNew";
	param_list[6].name="Delta";
	param_list[7].name="X-\nRoads";
	param_list[8].name="Position";
	param_list[9].name="Dist";
	param_list[10].name="Command";
	if (old->points) {
		if (!maneuver_required(old, new, &delta)) {
			old->length+=new->length;
			old->time+=new->time;
			old->crossings_end=new->crossings_end;
			old->angle_end=new->angle_end;
			old->points+=new->points;
		} else {	
			sprintf(angle_old,"%d", old->angle_end);
			param_list[4].value=angle_old;
			sprintf(angle_new,"%d", new->angle_start);
			param_list[5].value=angle_new;
			sprintf(angle_delta,"%d", delta);
			param_list[6].value=angle_delta;
			sprintf(cross_roads,"%d", old->crossings_end);
			param_list[7].value=cross_roads;
			sprintf(position,"0x%lx,0x%lx", new->c.x, new->c.y);
			param_list[8].value=position;
			sprintf(distance,"%d", old->length);
			param_list[9].value=distance;
			add_dir=1;
			dir="rechts";
			if (delta < 0) {
				dir="links";
				delta=-delta;
			}
			if (delta < 45) {
				strength="leicht ";
			} else if (delta < 105) {
				strength="";
			} else if (delta < 165) {
				strength="scharf ";
			} else {
				printf("delta=%d\n", delta);
				strength="unbekannt ";
			}
			level=0;
			if (navmode) {
				if (old->length < 20) {
					level=1;
					sprintf(command,"Jetzt ");
				} else if (old->length <= 200) {
					level=2;
					get_distance(dist, old->length);
					sprintf(command,"In %sn ", dist);
				} else if (old->length <= 500) {
					level=3;
					sprintf(command,"In Kürze ");
				} else {
					level=4;
					get_distance(dist, old->length);
					sprintf(command,"Dem Strassenverlauf %s folgen", dist);
					add_dir=0;
				}
			} else {
				sprintf(command,"Dem Strassenverlauf %d Meter folgen, dann ", old->length);
				add_dir=1;
			}
			if (add_dir) {
				p=command+strlen(command);
				strcpy(p,strength);
				p+=strlen(p);
				strcpy(p,dir);
				p+=strlen(p);
				strcpy(p," abbiegen");
			}
			param_list[10].value=command;
			if (flag) {
				if (level != old_level) {
					char buffer[2048];
					printf("command='%s'\n", command);
#if 0
					speech_say(speech_handle, command);
#endif
					sprintf(buffer,"espeak -v german '%s' &", command);
					system(buffer);
					old_level=level;
				}
				flag=0;
			}
			printf("*** %s\n", param_list[10].value);
#if 1
			data_window_add(navigation_window, param_list, 11);
#endif
			*old=*new;
		}
	} else {
		*old=*new;
	}
}


int
navigation_crossings(struct mapset *ms, struct coord *cc)
{
	int max_dist=1000;
	struct map_selection *sel=route_rect(18, cc, cc, 0, max_dist);
	struct mapset_handle *h;
	struct map *m;
	struct map_rect *mr;
	struct item *item;
	struct coord c;
	int ret=0;

        h=mapset_open(ms);
        while ((m=mapset_next(h,1))) {
		mr=map_rect_new(m, sel);
		while ((item=map_rect_get_item(mr))) {
			if (item->type >= type_street_0 && item->type <= type_ferry) {
				while (item_coord_get(item, &c, 1)) {
					if (c.x == cc->x && c.y == cc->y)
						ret++;
				}
			} else 
				while (item_coord_get(item, &c, 1));
                }  
		map_rect_destroy(mr);
        }
        mapset_close(h);
	
	return ret;
}

void
navigation_item_get_data(struct item *item, struct coord *start, struct navigation_item *nitem)
{
	struct coord c[4];
	struct map_rect *mr;
	struct attr attr;
	int l,i=0,a1,a2,dir=0;
	mr=map_rect_new(item->map, NULL);
	item=map_rect_get_item_byid(mr, item->id_hi, item->id_lo);
	if (item_attr_get(item, attr_name, &attr)) 
		strcpy(nitem->name1, attr.u.str);
	else
		nitem->name1[0]='\0';
	if (item_attr_get(item, attr_name_systematic, &attr))
		strcpy(nitem->name2, attr.u.str);
	else
		nitem->name2[0]='\0';
	expand_str(nitem->name1);
	expand_str(nitem->name2);
	nitem->points=0;
	nitem->c=*start;
	nitem->item=*item;
	l=-1;
	while (item_coord_get(item, &c[i], 1)) {
		dbg(1, "coord %d 0x%x 0x%x\n", i, c[i].x ,c[i].y);
		l=i;
		if (i < 3) 
			i++;
		else
			c[2]=c[3];
		nitem->points++;
	}
	dbg(1,"count=%d\n", l);
	if (start->x != c[0].x || start->y != c[0].y)
		dir=-1;
	a1=road_angle(&c[0], &c[1], dir);
	a2=road_angle(&c[l-1], &c[l], dir);
	if (dir >= 0) {
		nitem->angle_start=a1;
		nitem->angle_end=a2;
	} else {
		nitem->angle_start=a2;
		nitem->angle_end=a1;
	}
	dbg(1,"i=%d a1 %d a2 %d %s %s\n", i, a1, a2, nitem->name1, nitem->name2);
	map_rect_destroy(mr);
}

void
navigation_path_description(void *route, void *ms, void *pos, void *dst)
{
	struct route_info_handle *h;
	struct coord *c, *p1=NULL, *p2=NULL, ci;
	struct route_segment_handle *rph;
	struct route_segment *s;
	struct item *item;
	h=route_info_open(pos, dst);
	int angle_start,i;
	struct navigation_item item_curr,item_last;

	if (!navigation_window)
		navigation_window=data_window("Navigation",NULL,navigation_goto);
	data_window_begin(navigation_window);
	memset(&item_last,0,sizeof(item_last));
	flag=1;

	if (h) {
		printf("destination nearly reached\n");
		route_info_close(h);
		return;
	}
	h=route_info_open(pos, NULL);
	while (c=route_info_get(h)) {
		p1=p2;
		p2=c;
	}
	dbg(1,"p1=%p p2=%p\n", p1, p2);
	if (!p1 || !p2)
		return;
	angle_start=road_angle(p1, p2, 0);
	printf("angle_start=%d\n", angle_start);
	printf("c=0x%x,0x%x\n", p2->x, p2->y);


	rph=route_path_open(route);

	i=0;
	while((s=route_path_get_segment(rph)) && i < 1000) {
		dbg(1,"s=%p\n", s);
		p1=route_path_segment_get_start(s);
		dbg(1,"c=0x%x,0x%x\n", p1->x, p1->y);
		p2=route_path_segment_get_end(s);
		dbg(1,"c=0x%x,0x%x\n", p2->x, p2->y);
		item=route_path_segment_get_item(s);
		navigation_item_get_data(item, p1, &item_curr);
		item_curr.time=route_path_segment_get_time(s);
		item_curr.length=route_path_segment_get_length(s);
#if 0
		item_curr.crossings_start=navigation_crossings(ms, p1);
		item_curr.crossings_end=navigation_crossings(ms, p2);
#endif
		item_curr.crossings_start=3;
		item_curr.crossings_end=3;
		dbg(1,"crossings %d %d\n", item_curr.crossings_start, item_curr.crossings_end);
		make_maneuver(&item_last,&item_curr);
		i++;
	}
	data_window_end(navigation_window);
#if 0
	exit(0);	
#endif
}
