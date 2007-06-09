struct route_crossing {
	long segid;
	int dir;
};

struct route_crossings {
	int count;
	struct route_crossing crossing[0];
};

/* prototypes */
struct coord;
struct item;
struct map_selection;
struct mapset;
struct route;
struct route_info;
struct route_info_handle;
struct route_path_handle;
struct route_path_segment;
struct transformation;
struct route *route_new(struct mapset *ms);
int route_contains(struct route *this, struct item *itm);
void route_set_position(struct route *this, struct coord *pos);
struct map_selection *route_rect(int order, struct coord *c1, struct coord *c2, int rel, int abs);
void route_set_destination(struct route *this, struct coord *dst);
struct route_path_handle *route_path_open(struct route *this);
struct route_path_segment *route_path_get_segment(struct route_path_handle *h);
struct coord *route_path_segment_get_start(struct route_path_segment *s);
struct coord *route_path_segment_get_end(struct route_path_segment *s);
struct item *route_path_segment_get_item(struct route_path_segment *s);
int route_path_segment_get_length(struct route_path_segment *s);
int route_path_segment_get_time(struct route_path_segment *s);
void route_path_close(struct route_path_handle *h);
int route_find(struct route *this, struct route_info *pos, struct route_info *dst);
struct route_info *route_find_nearest_street(struct route *this, struct coord *c);
struct route_info_handle *route_info_open(struct route_info *start, struct route_info *end);
struct coord *route_info_get(struct route_info_handle *h);
void route_info_close(struct route_info_handle *h);
void route_draw(struct route *this, struct transformation *t, GHashTable *dsp);
