/* prototypes */
struct coord;
struct item;
struct mapset;
struct navigation_item;
struct route;
struct route_info;
void navigation_item_get_data(struct item *item, struct coord *start, struct navigation_item *nitem);
void navigation_path_description(struct route *route, struct mapset *ms, struct route_info *pos, struct route_info *dst);
