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
struct displaylist;
struct item;
struct map_selection;
struct mapset;
struct route;
struct route_info;
struct route_info_handle;
struct route_path_handle;
struct route_path_segment;
struct street_data;
struct track;
struct transformation;
struct route *route_new(struct mapset *ms);
struct mapset *route_get_mapset(struct route *this);
struct route_info *route_get_pos(struct route *this);
struct route_info *route_get_dst(struct route *this);
int route_contains(struct route *this, struct item *item);
void route_set_position(struct route *this, struct coord *pos);
void route_set_position_from_track(struct route *this, struct track *track);
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
int route_time(struct item *item, int len);
int route_info_length(struct route_info *pos, struct route_info *dst, int dir);
struct street_data *street_get_data(struct item *item);
struct street_data *street_data_dup(struct street_data *orig);
void street_data_free(struct street_data *sd);
void route_info_free(struct route_info *inf);
struct street_data *route_info_street(struct route_info *rinf);
struct coord *route_info_point(struct route_info *rinf, int point);
struct route_info_handle *route_info_open(struct route_info *start, struct route_info *end, int dir);
struct coord *route_info_get(struct route_info_handle *h);
void route_info_close(struct route_info_handle *h);
void route_draw(struct route *this, struct transformation *t, struct displaylist *dsp);
