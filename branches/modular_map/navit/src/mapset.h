struct map;
struct mapset;
struct mapset_handle;

struct mapset *mapset_new(void);
void mapset_add(struct mapset *ms, struct map *map);
struct mapset_handle *mapset_handle_new(struct mapset *ms);
int mapset_handle_read(struct mapset_handle *msh, struct map **map, int count);
struct mapset_handle *mapset_handle_destroy(struct mapset_handle *msh);
void mapset_destroy(struct mapset *ms);
void * mapset_open(struct mapset *ms);
struct map * mapset_get(void *h);
void mapset_close(void *h);

