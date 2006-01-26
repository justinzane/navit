#include <glib.h>
#include <string.h>
#include "coord.h"
#include "map.h"
#include "maptype.h"
#include "item.h"

struct map {
	struct map_methods meth;
	struct map_priv *priv;
	char *charset;
};

struct map_rect {
	struct map *m;
	struct map_rect_priv *priv;
};

struct map *
map_new(char *type, char *filename)
{
	struct map *m;
	struct maptype *mt;

	mt=maptype_get(type);
	if (! mt)
		return NULL;

	m=g_new0(struct map, 1);

	m->priv=mt->map_new(&m->meth, filename);
	m->charset=m->meth.map_charset(m->priv);
	return m;
}

void
map_destroy(struct map *m)
{
	m->meth.map_destroy(m->priv);
	g_free(m);
}

struct map_rect *
map_rect_new(struct map *m, struct coord_rect *r, struct layer *layers, int limit)
{
	struct map_rect *mr;

#if 0
	printf("map_rect_new 0x%x,0x%x-0x%x,0x%x\n", r->lu.x, r->lu.y, r->rl.x, r->rl.y);
#endif
	mr=g_new0(struct map_rect, 1);
	mr->m=m;
	mr->priv=m->meth.map_rect_new(m->priv, r, layers, limit);

	return mr;
}

struct item *
map_rect_get_item(struct map_rect *mr)
{
	struct item *ret;
	g_assert(mr != NULL);
	g_assert(mr->m != NULL);
	g_assert(mr->m->meth.map_rect_get_item != NULL);
	ret=mr->m->meth.map_rect_get_item(mr->priv);
	if (ret)
		ret->map=mr->m;
	return ret;
}

void
map_rect_destroy(struct map_rect *mr)
{
	mr->m->meth.map_rect_destroy(mr->priv);
	g_free(mr);
}
