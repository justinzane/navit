#include <glib.h>
#include <glib/gprintf.h>
#include "mapset.h"

struct mapset {
	GList *maps;
};

struct mapset *mapset_new(void)
{
	struct mapset *ms;

	ms=g_new0(struct mapset, 1);

	return ms;
}

void mapset_add(struct mapset *ms, struct map *m)
{
	ms->maps=g_list_append(ms->maps, m);
}

static void mapset_maps_free(struct mapset *ms)
{
	/* todo */
}

void mapset_destroy(struct mapset *ms)
{
	g_free(ms);
}

void 
mapset_open(struct mapset *ms, void **handle)
{
	*handle=ms->maps;
}

struct map * mapset_next(struct mapset *ms, void **handle)
{
	GList *l=*handle;
	if (!l)
		return NULL;
	*handle=g_list_next(l);
	return l->data;
}
