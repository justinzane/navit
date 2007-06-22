/* prototypes */
struct coord;
struct item;
struct mapset;
struct navigation;
struct navigation_item;
struct route;
void navigation_item_get_data(struct item *item, struct coord *start, struct navigation_item *nitem);
void navigation_path_description(struct route *route, int dir);
struct navigation *navigation_new(struct mapset *ms);
void navigation_update(struct navigation *this, struct route *route);
void navigation_destroy(struct navigation *this);
