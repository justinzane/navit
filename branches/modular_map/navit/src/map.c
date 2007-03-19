#include <glib.h>
#include <string.h>
#include "coord.h"
#include "map.h"
#include "maptype.h"
#include "transform.h"
#include "item.h"
#include "plugin.h"

struct map {
	struct map_methods meth;
	struct map_priv *priv;
	char *charset;
	enum projection projection;
};

struct map_rect {
	struct map *m;
	struct map_rect_priv *priv;
};

struct map *
map_new(char *type, char *filename)
{
	struct map *m;
	struct map_priv *(*maptype_new)(struct map_methods *meth, char *name);

	maptype_new=plugin_get_map_type(type);
	if (! maptype_new)
		return NULL;

	m=g_new0(struct map, 1);

	m->priv=maptype_new(&m->meth, filename);
	m->charset=m->meth.map_charset(m->priv);
	m->projection=m->meth.map_projection(m->priv);
	return m;
}

enum projection
map_projection(struct map *this)
{
	return this->projection;
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

struct item *
map_rect_get_item_byid(struct map_rect *mr, int id_hi, int id_lo)
{
	struct item *ret=NULL;
	g_assert(mr != NULL);
	g_assert(mr->m != NULL);
	if (mr->m->meth.map_rect_get_item_byid)
		ret=mr->m->meth.map_rect_get_item_byid(mr->priv, id_hi, id_lo);
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
