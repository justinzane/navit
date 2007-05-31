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

#if 0
static void mapset_maps_free(struct mapset *ms)
{
	/* todo */
}
#endif

void mapset_destroy(struct mapset *ms)
{
	g_free(ms);
}

struct mapset_handle {
	GList *l;
};

struct mapset_handle *
mapset_open(struct mapset *ms)
{
	struct mapset_handle *ret;

	ret=g_new(struct mapset_handle, 1);
	ret->l=ms->maps;

	return ret;
}

struct map * mapset_next(struct mapset_handle *msh)
{
	struct map *ret;

	if (!msh->l)
		return NULL;
	ret=msh->l->data;
	msh->l=g_list_next(msh->l);

	return ret;
}

void 
mapset_close(struct mapset_handle *msh)
{
	g_free(msh);
}
