#include <glib.h>
#include <glib/gprintf.h>
#include "mapset.h"

struct mapset_item {
	struct mapset_item *next;
	struct map *map;
};
struct mapset {
	
	struct mapset_item *first;
};

struct mapset *mapset_new(void)
{
	struct mapset *ms;

	ms=g_new0(struct mapset, 1);

	return ms;
}

void mapset_add(struct mapset *ms, struct map *m)
{
	struct mapset_item *msi;
	msi=g_new0(struct mapset_item, 1);
	msi->next=ms->first;
	msi->map=m;
	ms->first=msi;
}

static void mapset_maps_free(struct mapset *ms)
{
}

void mapset_destroy(struct mapset *ms)
{
	g_free(ms);
}

void *
mapset_open(struct mapset *ms)
{
	struct mapset_item **iter;
	iter=g_new(struct mapset_item *,1);
	g_printf("ms=%p ms->first=%p\n", ms, ms->first);
	*iter=ms->first;
	return iter;	
}

struct map * mapset_get(void *h)
{
	struct mapset_item **iter=h;
	struct mapset_item *i=*iter;
	struct map *ret;

	if (! i)
		return NULL;
	ret=i->map;
	*iter=i->next;
	return ret;
}

void
mapset_close(void *h)
{
	g_free(h);
}
