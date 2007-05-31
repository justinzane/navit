/* prototypes */
struct map;
struct mapset;
struct mapset_handle;
struct mapset *mapset_new(void);
void mapset_add(struct mapset *ms, struct map *m);
void mapset_destroy(struct mapset *ms);
struct mapset_handle *mapset_open(struct mapset *ms);
struct map *mapset_next(struct mapset_handle *msh);
void mapset_close(struct mapset_handle *msh);
