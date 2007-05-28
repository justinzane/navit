#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <glib.h>
#include "config.h"
#include "coord.h"
#include "transform.h"
#include "projection.h"
#include "point.h"

struct transformation {
        int width;		/* Height of destination rectangle */
        int height;		/* Width of destination rectangle */
        long scale;		/* Scale factor */
	int angle;		/* Rotation angle */
	double cos_val,sin_val;	/* cos and sin of rotation angle */
	enum projection pro;
	struct coord_rect r;	/* Source rectangle */
	struct coord center;	/* Center of source rectangle */
};

struct transformation *
transform_new(void)
{
	struct transformation *this;

	this=g_new0(struct transformation, 1);

	return this;
}

void
transform_to_geo(enum projection pro, struct coord *c, struct coord_geo *g)
{
	double f;

	switch (pro) {
	case projection_mg:
		g->lng=c->x/6371000.0/M_PI*180;
		g->lat=atan(exp(c->y/6371000.0))/M_PI*360-90;
		break;
	case projection_garmin:
		f=360.0/(1<<24);
		g->lng=c->x*f;
		g->lat=c->y*f;	
		break;
	default:
		break;
	}
}

void
transform_from_geo(enum projection pro, struct coord_geo *g, struct coord *c)
{
	double f;

	switch (pro) {
	case projection_mg:
		c->x=g->lng*6371000.0*M_PI/180;
		c->y=log(tan(M_PI_4+g->lat*M_PI/360))*6371000.0;
		break;
	case projection_garmin:
		f=360.0/(1<<24);
		c->x=g->lng/f;
		c->y=g->lat/f;	
		break;
	default:
		break;
	}
}

int
transform(struct transformation *t, enum projection pro, struct coord *c, struct point *p)
{
	struct coord c1;
#ifdef AVOID_FLOAT
	int xc,yc;
#else
        double xc,yc;
#endif
	int ret;
	if (pro != t->pro) {
		struct coord_geo g;
		transform_to_geo(pro, c, &g);
		transform_from_geo(t->pro, &g, &c1);
		c=&c1;
	}
        xc=c->x;
        yc=c->y;
	ret=coord_rect_contains(&t->r, c);
        xc-=t->center.x;
        yc-=t->center.y;
	yc=-yc;
	if (t->angle) {
	  	int xcn, ycn; 
	  	xcn=xc*t->cos_val+yc*t->sin_val;
	  	ycn=-xc*t->sin_val+yc*t->cos_val;
	  	xc=xcn;
	  	yc=ycn;
	}
#ifdef AVOID_FLOAT
        xc=xc*16.0/(double)(t->scale);
        yc=yc*16.0/(double)(t->scale);
#else
        xc=xc*16/t->scale;
        yc=yc*16/t->scale;
#endif
#if 0
	{
		double zc=yc;
		if (zc  < 10 && zc > 10)
			zc=10;
			return 0;
		yc=300;
		xc/=(-zc+1000.0)/1000.0;
		yc/=(-zc+1000.0)/1000.0;
		xc+=t->width/2;
	}
#else
        yc+=t->height/2;
        xc+=t->width/2;
#endif
	if (xc < -0x8000)
		xc=-0x8000;
	if (xc > 0x7fff) {
		xc=0x7fff;
	}
	if (yc < -0x8000)
		yc=-0x8000;
	if (yc > 0x7fff)
		yc=0x7fff;
        p->x=xc;
        p->y=yc;
        return ret;
}

void
transform_reverse(struct transformation *t, struct point *p, struct coord *c)
{
        int xc,yc;
	xc=p->x;
	yc=p->y;
	xc-=t->width/2;
	yc-=t->height/2;
	xc=xc*t->scale/16;
	yc=-yc*t->scale/16;
	if (t->angle) {
	  	int xcn, ycn; 
	  	xcn=xc*t->cos_val+yc*t->sin_val;
	  	ycn=-xc*t->sin_val+yc*t->cos_val;
	  	xc=xcn;
	  	yc=ycn;
	}
	c->x=t->center.x+xc;
	c->y=t->center.y+yc;
}

enum projection
transform_get_projection(struct transformation *this)
{
	return this->pro;
}

void
transform_set_projection(struct transformation *this, enum projection pro)
{
	this->pro=pro;
}

static int
min4(int v1,int v2, int v3, int v4)
{
	int res=v1;
	if (v2 < res)
		res=v2;
	if (v3 < res)
		res=v3;
	if (v4 < res)
		res=v4;
	return res;
}

static int
max4(int v1,int v2, int v3, int v4)
{
	int res=v1;
	if (v2 > res)
		res=v2;
	if (v3 > res)
		res=v3;
	if (v4 > res)
		res=v4;
	return res;
}

void
transform_rect(struct transformation *this, enum projection pro, struct coord_rect *r)
{
	struct coord_geo g;
	if (0 && this->pro == pro) {
		*r=this->r;
	} else {
		transform_to_geo(this->pro, &this->r.lu, &g);
		transform_from_geo(pro, &g, &r->lu);
		printf("%f,%f", g.lat, g.lng);
		transform_to_geo(this->pro, &this->r.rl, &g);
		printf(" - %f,%f\n", g.lat, g.lng);
		transform_from_geo(pro, &g, &r->rl);
	}
	printf("transform rect for %d is %d,%d - %d,%d\n", pro, r->lu.x, r->lu.y, r->rl.x, r->rl.y);
}

struct coord *
transform_center(struct transformation *this)
{
	return &this->center;
}

int
transform_contains(struct transformation *this, enum projection pro, struct coord_rect *r)
{
	return 1;
	struct coord_geo g;
	struct coord_rect r1;
	if (this->pro != pro) {
		transform_to_geo(pro, &r->lu, &g);
		transform_from_geo(this->pro, &g, &r1.lu);
		transform_to_geo(pro, &r->rl, &g);
		transform_from_geo(this->pro, &g, &r1.rl);
		r=&r1;
	}
	return coord_rect_overlap(&this->r, r);
	
}

void
transform_set_angle(struct transformation *t,int angle)
{
        t->angle=angle;
        t->cos_val=cos(M_PI*t->angle/180);
        t->sin_val=sin(M_PI*t->angle/180);
}

int
transform_get_angle(struct transformation *this,int angle)
{
	return this->angle;
}

void
transform_set_size(struct transformation *t, int width, int height)
{
	t->width=width;
	t->height=height;
}

void
transform_setup(struct transformation *t, struct coord *c, int scale, int angle)
{
        t->center=*c;
        t->scale=scale;
	transform_set_angle(t, angle);
}

void
transform_setup_source_rect_limit(struct transformation *t, struct coord *center, int limit)
{
	t->center=*center;
	t->scale=1;
	t->angle=0;
	t->r.lu.x=center->x-limit;
	t->r.rl.x=center->x+limit;
	t->r.rl.y=center->y-limit;
	t->r.lu.y=center->y+limit;
}

void
transform_setup_source_rect(struct transformation *t)
{
	int i;
	struct coord screen[4];
	struct point screen_pnt[4];

	screen_pnt[0].x=0;
	screen_pnt[0].y=0;
	screen_pnt[1].x=t->width;
	screen_pnt[1].y=0;
	screen_pnt[2].x=0;
	screen_pnt[2].y=t->height;
	screen_pnt[3].x=t->width;
	screen_pnt[3].y=t->height;
	for (i = 0 ; i < 4 ; i++) {
		transform_reverse(t, &screen_pnt[i], &screen[i]);
	}
	t->r.lu.x=min4(screen[0].x,screen[1].x,screen[2].x,screen[3].x);
	t->r.rl.x=max4(screen[0].x,screen[1].x,screen[2].x,screen[3].x);
	t->r.rl.y=min4(screen[0].y,screen[1].y,screen[2].y,screen[3].y);
	t->r.lu.y=max4(screen[0].y,screen[1].y,screen[2].y,screen[3].y);
}

long
transform_get_scale(struct transformation *t)
{
	return t->scale;
}

void
transform_set_scale(struct transformation *t, long scale)
{
	t->scale=scale;
}


int
transform_get_order(struct transformation *t)
{
	int scale=t->scale/16;
	int order=0;
        while (scale > 1) {
                order++;
                scale>>=1;
        }
        order=14-order;
        if (order < 0)
                order=0;
	return order;
}


void
transform_geo_text(struct coord_geo *g, char *buffer)
{
	double lng=g->lng;
	double lat=g->lat;
	char lng_c='E';
	char lat_c='N';

	if (lng < 0) {
		lng=-lng;
		lng_c='W';
	}
	if (lat < 0) {
		lat=-lat;
		lat_c='S';
	}

	sprintf(buffer,"%02.0f%07.4f%c %03.0f%07.4f%c", floor(lat), fmod(lat*60,60), lat_c, floor(lng), fmod(lng*60,60), lng_c);

}

double
transform_scale(int y)
{
	struct coord c;
	struct coord_geo g;
	c.x=0;
	c.y=y;
	transform_to_geo(projection_mg, &c, &g);
	return 1/cos(g.lat/180*M_PI);
}

double
transform_distance(struct coord *c1, struct coord *c2)
{
	double dx,dy,scale=transform_scale((c1->y+c2->y)/2);
	dx=c1->x-c2->x;
	dy=c1->y-c2->y;
	return sqrt(dx*dx+dy*dy)/scale;
}

int
transform_distance_sq(struct coord *c1, struct coord *c2)
{
	int dx=c1->x-c2->x;
	int dy=c1->y-c2->y;

	if (dx > 32767 || dy > 32767 || dx < -32767 || dy < -32767)
		return INT_MAX;
	else
		return dx*dx+dy*dy;
}

int
transform_distance_line_sq(struct coord *l0, struct coord *l1, struct coord *ref, struct coord *lpnt)
{
	int vx,vy,wx,wy;
	int c1,c2;
	int climit=1000000;
	struct coord l;

	vx=l1->x-l0->x;
	vy=l1->y-l0->y;
	wx=ref->x-l0->x;
	wy=ref->y-l0->y;

	c1=vx*wx+vy*wy;
	if ( c1 <= 0 ) {
		if (lpnt)
			*lpnt=*l0;
		return transform_distance_sq(l0, ref);
	}
	c2=vx*vx+vy*vy;
	if ( c2 <= c1 ) {
		if (lpnt)
			*lpnt=*l1;
		return transform_distance_sq(l1, ref);
	}
	while (c1 > climit || c2 > climit) {
		c1/=256;
		c2/=256;
	}
	l.x=l0->x+vx*c1/c2;
	l.y=l0->y+vy*c1/c2;
	if (lpnt)
		*lpnt=l;
	return transform_distance_sq(&l, ref);
}


void
transform_print_deg(double deg)
{
	printf("%2.0f:%2.0f:%2.4f", floor(deg), fmod(deg*60,60), fmod(deg*3600,60));
}

int 
is_visible(struct transformation *t, struct coord *c)
{
	struct coord_rect *r=&t->r;

	assert(c[0].x <= c[1].x);
	assert(c[0].y >= c[1].y);
	assert(r->lu.x <= r->rl.x);
	assert(r->lu.y >= r->rl.y);
	if (c[0].x > r->rl.x)
		return 0;
	if (c[1].x < r->lu.x)
		return 0;
	if (c[0].y < r->rl.y)
		return 0;
	if (c[1].y > r->lu.y)
		return 0;
	return 1;
}

int
is_line_visible(struct transformation *t, struct coord *c)
{
	struct coord_rect *r=&t->r;

	assert(r->lu.x <= r->rl.x);
	assert(r->lu.y >= r->rl.y);
	if (MIN(c[0].x,c[1].x) > r->rl.x)
		return 0;
	if (MAX(c[0].x,c[1].x) < r->lu.x)
		return 0;
	if (MAX(c[0].y,c[1].y) < r->rl.y)
		return 0;
	if (MIN(c[0].y,c[1].y) > r->lu.y)
		return 0;
	return 1;
}

int 
is_point_visible(struct transformation *t, struct coord *c)
{
	struct coord_rect *r=&t->r;

	assert(r->lu.x <= r->rl.x);
	assert(r->lu.y >= r->rl.y);
	if (c->x > r->rl.x)
		return 0;
	if (c->x < r->lu.x)
		return 0;
	if (c->y < r->rl.y)
		return 0;
	if (c->y > r->lu.y)
		return 0;
	return 1;
}


int
is_too_small(struct transformation *t, struct coord *c, int limit)
{
	return 0;
	if ((c[1].x-c[0].x) < limit*t->scale/16) {
		return 1;
	}
	if ((c[0].y-c[1].y) < limit*t->scale/16) {
		return 1;
	}
	return 0;	
}


void transform_limit_extend(struct coord *rect, struct coord *c)
{
	if (c->x < rect[0].x) rect[0].x=c->x;
	if (c->x > rect[1].x) rect[1].x=c->x;
	if (c->y < rect[1].y) rect[1].y=c->y;
	if (c->y > rect[0].y) rect[0].y=c->y;
}



int
transform_get_angle_delta(struct coord *c, int dir)
{
	double angle;
	int dx=c[1].x-c[0].x;
	int dy=c[1].y-c[0].y;
	angle=atan2(dx,dy);
	angle*=180/M_PI;
	if (dir == -1)
		angle=angle-180;
	if (angle < 0)
		angle+=360;
	return angle;
}

int
transform_within_border(struct transformation *this, struct point *p, int border)
{
	if (p->x < border || p->x > this->width-border || p->y < border || p->y > this->height-border)
		return 0;
	return 1;
}

/*
Note: there are many mathematically equivalent ways to express these formulas. As usual, not all of them are computationally equivalent.

L = latitude in radians (positive north)
Lo = longitude in radians (positive east)
E = easting (meters)
N = northing (meters)

For the sphere

E = r Lo
N = r ln [ tan (pi/4 + L/2) ]

where 

r = radius of the sphere (meters)
ln() is the natural logarithm

For the ellipsoid

E = a Lo
N = a * ln ( tan (pi/4 + L/2) * ( (1 - e * sin (L)) / (1 + e * sin (L))) ** (e/2)  )


                                               e
                                               -
                   pi     L     1 - e sin(L)   2
    =  a ln( tan( ---- + ---) (--------------)   )
                   4      2     1 + e sin(L) 


where

a = the length of the semi-major axis of the ellipsoid (meters)
e = the first eccentricity of the ellipsoid


*/


