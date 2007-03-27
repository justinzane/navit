enum projection;
/* prototypes */
struct coord;
struct coord_geo;
struct coord_rect;
struct point;
struct transformation;
struct transformation *transform_new(void);
void transform_to_geo(enum projection pro, struct coord *c, struct coord_geo *g);
void transform_from_geo(enum projection pro, struct coord_geo *g, struct coord *c);
int transform(struct transformation *t, enum projection pro, struct coord *c, struct point *p);
void transform_reverse(struct transformation *t, struct point *p, struct coord *c);
void transform_rect(struct transformation *this, enum projection pro, struct coord_rect *r);
struct coord *transform_center(struct transformation *this);
int transform_contains(struct transformation *this, enum projection pro, struct coord_rect *r);
void transform_set_angle(struct transformation *t, int angle);
int transform_get_angle(struct transformation *this, int angle);
void transform_set_size(struct transformation *t, int width, int height);
void transform_setup(struct transformation *t, int x, int y, int scale, int angle);
void transform_setup_source_rect_limit(struct transformation *t, struct coord *center, int limit);
void transform_setup_source_rect(struct transformation *t);
long transform_get_scale(struct transformation *t);
void transform_set_scale(struct transformation *t, long scale);
int transform_get_order(struct transformation *t);
void transform_geo_text(struct coord_geo *g, char *buffer);
double transform_scale(int y);
double transform_distance(struct coord *c1, struct coord *c2);
int transform_distance_sq(struct coord *c1, struct coord *c2);
int transform_distance_line_sq(struct coord *l0, struct coord *l1, struct coord *ref, struct coord *lpnt);
void transform_print_deg(double deg);
int is_visible(struct transformation *t, struct coord *c);
int is_line_visible(struct transformation *t, struct coord *c);
int is_point_visible(struct transformation *t, struct coord *c);
int is_too_small(struct transformation *t, struct coord *c, int limit);
void transform_limit_extend(struct coord *rect, struct coord *c);
int transform_get_angle_delta(struct coord *c, int dir);
int transform_within_border(struct transformation *this, struct point *p, int border);
