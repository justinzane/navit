
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <glib.h>
#include "coord.h"
#include "transform.h"
#include "projection.h"
#include "point.h"
#include "graphics.h"
#include "statusbar.h"
#include "menu.h"
#include "vehicle.h"
#include "navit.h"
#include "color.h"
#include "cursor.h"
#include "compass.h"
/* #include "track.h" */

struct callback {
        void (*func)(struct cursor *, void *data);
        void *data;
};


struct cursor {
	struct graphics *gra;
	struct graphics_gc *cursor_gc;
	struct transformation *trans;
	struct point cursor_pnt;
	struct callback offscreen_callback;
	struct callback update_callback;
	struct vehicle *v;
	int dir;
	struct coord pos;
	void *vehicle_callback;
};

struct coord *
cursor_pos_get(struct cursor *this)
{
	return &this->pos;
#if 0
	return vehicle_pos_get(this->v);
#endif
}


struct coord *
cursor_pos_set(struct cursor *this, struct coord *pos)
{
	this->pos=*pos;
#if 0
	return vehicle_pos_get(this->v);
#endif
}


static void
cursor_draw(struct cursor *this, struct point *pnt, int dir, int draw_dir)
{
	int x=this->cursor_pnt.x;
	int y=this->cursor_pnt.y;
	int r=12,lw=2;
	double dx,dy;
	double fac1,fac2;
	struct point cpnt[3];
	struct graphics *gra=this->gra;

	if (pnt && x == pnt->x && y == pnt->y)
		return;
	if (!graphics_ready(gra))
		return;
	cpnt[0]=this->cursor_pnt;
	cpnt[0].x-=r+lw;
	cpnt[0].y-=r+lw;
	graphics_draw_restore(gra, &cpnt[0], (r+lw)*2, (r+lw)*2);
	if (pnt) {
		graphics_draw_mode(gra, draw_mode_cursor);
		this->cursor_pnt=*pnt;
		x=pnt->x;
		y=pnt->y;
		cpnt[0].x=x;
		cpnt[0].y=y;
		graphics_draw_circle(gra, this->cursor_gc, &cpnt[0], r*2);
		if (draw_dir) {
			dx=sin(M_PI*dir/180.0);
			dy=-cos(M_PI*dir/180.0);

			fac1=0.7*r;
			fac2=0.4*r;	
			cpnt[0].x=x-dx*fac1+dy*fac2;
			cpnt[0].y=y-dy*fac1-dx*fac2;
			cpnt[1].x=x+dx*r;
			cpnt[1].y=y+dy*r;
			cpnt[2].x=x-dx*fac1-dy*fac2;
			cpnt[2].y=y-dy*fac1+dx*fac2;
			graphics_draw_lines(gra, this->cursor_gc, cpnt, 3);
		} else {
			cpnt[1]=cpnt[0];
			graphics_draw_lines(gra, this->cursor_gc, cpnt, 2);
		}
		graphics_draw_mode(gra, draw_mode_end);
	}
}

#if 0
static void
cursor_map_reposition_screen(struct cursor *this, struct coord *c, double *dir, int x_new, int y_new)
{
	struct coord c_new;
	struct transformation tr;
	struct point pnt;
	unsigned long scale;
	long x,y;
	int dir_i;
	struct container *co=this->co;

	if (dir)
		dir_i=*dir;
	else
		dir_i=0;

	pnt.x=co->trans->width-x_new;
	pnt.y=co->trans->height-y_new;
	graphics_get_view(co, &x, &y, &scale);
	tr=*this->co->trans;
	transform_setup(&tr, c->x, c->y, scale, dir_i);
	transform_reverse(&tr, &pnt, &c_new);
	printf("%lx %lx vs %lx %lx\n", c->x, c->y, c_new.x, c_new.y);
	x=c_new.x;
	y=c_new.y;
	transform_set_angle(co->trans,dir_i);
	graphics_set_view(co, &x, &y, &scale);
}

static void
cursor_map_reposition(struct cursor *this, struct coord *c, double *dir)
{
	if (this->co->flags->orient_north) {
		graphics_set_view(this->co, &c->x, &c->y, NULL);
	} else {
		cursor_map_reposition_screen(this, c, dir, this->co->trans->width/2, this->co->trans->height*0.8);
	}
}

static int
cursor_map_reposition_boundary(struct cursor *this, struct coord *c, double *dir, struct point *pnt)
{
	struct point pnt_new;
	struct transformation *t=this->co->trans;

	pnt_new.x=-1;
	pnt_new.y=-1;
	if (pnt->x < 0.1*t->width) {
		pnt_new.x=0.8*t->width;
		pnt_new.y=t->height/2;
	}
	if (pnt->x > 0.9*t->width) {
		pnt_new.x=0.2*t->width;
		pnt_new.y=t->height/2;
	}
	if (pnt->y < (this->co->flags->orient_north ? 0.1 : 0.5)*t->height) {
		pnt_new.x=t->width/2;
		pnt_new.y=0.8*t->height;
	}
	if (pnt->y > 0.9*t->height) {
		pnt_new.x=t->width/2;
		pnt_new.y=0.2*t->height;
	}
	if (pnt_new.x != -1) {
		if (this->co->flags->orient_north) {
			cursor_map_reposition_screen(this, c, NULL, pnt_new.x, pnt_new.y);
		} else {
			cursor_map_reposition(this, c, dir);
		}
		return 1;
	}
	return 0;
}

#endif

int
cursor_get_dir(struct  cursor *this)
{
	return this->dir;
}

static void
cursor_update(struct vehicle *v, void *data)
{
	struct cursor *this=data;
	struct point pnt;
	struct coord *pos;
	double *dir;
	double *speed;
	enum projection pro;
	int border=10;

	if (v) {
		pos=vehicle_pos_get(v);	
		dir=vehicle_dir_get(v);
		speed=vehicle_speed_get(v);
		pro=vehicle_projection(v);
		this->dir=*dir;
		this->pos=*pos;
		if (this->update_callback.func) 
			(*this->update_callback.func)(this, this->update_callback.data);
		if (!transform(this->trans, pro, &this->pos, &pnt) || !transform_within_border(this->trans, &pnt, border)) {
			if (this->offscreen_callback.func) 
				(*this->offscreen_callback.func)(this, this->offscreen_callback.data);
			transform(this->trans, pro, &this->pos, &pnt);
		}
		cursor_draw(this, &pnt, *dir-transform_get_angle(this->trans, 0), *speed > 2.5);
	}
#if 0
	compass_draw(this->co->compass, this->co);
#endif
}

struct cursor *
cursor_new(struct graphics *gra, struct vehicle *v, struct color *c, struct transformation *t)
{
	printf("cursor_new v=%p\n", v);
	struct cursor *this=g_new(struct cursor,1);
	this->gra=gra;
	this->trans=t;
	this->cursor_gc=graphics_gc_new(gra);
	this->v=v;
	graphics_gc_set_foreground(this->cursor_gc, c);
	graphics_gc_set_linewidth(this->cursor_gc, 2);
	this->vehicle_callback=vehicle_callback_register(v, cursor_update, this);
	return this;
}

void
cursor_register_offscreen_callback(struct cursor *this, void (*func)(struct cursor *cursor, void *data), void *data)
{
	this->offscreen_callback.func=func;
	this->offscreen_callback.data=data;
}


void
cursor_register_update_callback(struct cursor *this, void (*func)(struct cursor *cursor, void *data), void *data)
{
	this->update_callback.func=func;
	this->update_callback.data=data;
}


