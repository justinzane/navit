/* prototypes */
struct coord;
struct item;
struct navigation_item;
struct route;
void navigation_item_get_data(struct item *item, struct coord *start, struct navigation_item *nitem);
void navigation_path_description(struct route *route, int dir);
