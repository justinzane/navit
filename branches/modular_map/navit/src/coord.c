#include <glib.h>
#include "coord.h"
/**
 * @defgroup coord Coordinate handling functions
 * @{
 */

/**
 * Get a coordinate
 *
 * @param p Pointer to the coordinate
 * @returns the coordinate
 */

struct coord *
coord_get(unsigned char **p)
{
	struct coord *ret=(struct coord *)(*p);
	*p += sizeof(*ret);
	return ret;
}

struct coord *
coord_new(int x, int y)
{
	struct coord *c=g_new(struct coord, 1);

	c->x=x;
	c->y=y;

	return c;
}

void
coord_destroy(struct coord *c)
{
	g_free(c);
}

struct coord_rect *
coord_rect_new(struct coord *lu, struct coord *rl)
{
	struct coord_rect *r=g_new(struct coord_rect, 1);

	g_assert(lu->x <= rl->x);
	g_assert(lu->y >= rl->y);

	r->lu=*lu;
	r->rl=*rl;

	return r;
	
}

void
coord_rect_destroy(struct coord_rect *r)
{
	g_free(r);
}

int 
coord_rect_overlap(struct coord_rect *r1, struct coord_rect *r2)
{
	g_assert(r1->lu.x <= r1->rl.x);
	g_assert(r1->lu.y >= r1->rl.y);
	g_assert(r2->lu.x <= r2->rl.x);
	g_assert(r2->lu.y >= r2->rl.y);
	if (r1->lu.x > r2->rl.x)
		return 0;
	if (r1->rl.x < r2->lu.x)
		return 0;
	if (r1->lu.y < r2->rl.y)
		return 0;
	if (r1->rl.y > r2->lu.y)
		return 0;
	return 1;
}

int
coord_rect_contains(struct coord_rect *r, struct coord *c)
{
	g_assert(r->lu.x <= r->rl.x);
	g_assert(r->lu.y >= r->rl.y);
	if (c->x < r->lu.x)
		return 0;
	if (c->x > r->rl.x)
		return 0;
	if (c->y < r->rl.y)
		return 0;
	if (c->y > r->lu.y)
		return 0;
	return 1;
}

void
coord_rect_extend(struct coord_rect *r, struct coord *c)
{
	if (c->x < r->lu.x)
		r->lu.x=c->x;
	if (c->x > r->rl.x)
		r->rl.x=c->x;
	if (c->y < r->rl.y)
		r->rl.y=c->y;
	if (c->y > r->lu.y)
		r->lu.y=c->y;
}

/** @} */
