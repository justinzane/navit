#include <glib.h>

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

}

static void mapset_maps_free(struct mapset *ms)
{
}

void mapset_destroy(struct mapset *ms)
{
	g_free(ms);
}

