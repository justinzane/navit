#ifndef NAVIT_TRANSFORM_H
#define NAVIT_TRANSFORM_H

#ifdef __cplusplus
extern "C" {
#endif
/* prototypes */
enum projection;
struct coord;
struct coord_geo;
struct map_selection;
struct pcoord;
struct point;
struct transformation;
struct transformation *transform_new(void);
void transform_to_geo(enum projection pro, struct coord *c, struct coord_geo *g);
void transform_from_geo(enum projection pro, struct coord_geo *g, struct coord *c);
int transform(struct transformation *t, enum projection pro, struct coord *c, struct point *p, int count, int flags);
void transform_reverse(struct transformation *t, struct point *p, struct coord *c);
enum projection transform_get_projection(struct transformation *this_);
void transform_set_projection(struct transformation *this_, enum projection pro);
struct map_selection *transform_get_selection(struct transformation *this_, enum projection pro, int order);
struct coord *transform_center(struct transformation *this_);
void transform_set_angle(struct transformation *t, int angle);
int transform_get_angle(struct transformation *this_, int angle);
void transform_set_screen_selection(struct transformation *t, struct map_selection *sel);
void transform_get_size(struct transformation *t, int *width, int *height);
void transform_setup(struct transformation *t, struct pcoord *c, int scale, int angle);
void transform_setup_source_rect(struct transformation *t);
long transform_get_scale(struct transformation *t);
void transform_set_scale(struct transformation *t, long scale);
int transform_get_order(struct transformation *t);
void transform_geo_text(struct coord_geo *g, char *buffer);
double transform_scale(int y);
double transform_distance(enum projection pro, struct coord *c1, struct coord *c2);
int transform_distance_sq(struct coord *c1, struct coord *c2);
int transform_distance_line_sq(struct coord *l0, struct coord *l1, struct coord *ref, struct coord *lpnt);
int transform_distance_polyline_sq(struct coord *c, int count, struct coord *ref, struct coord *lpnt, int *pos);
void transform_print_deg(double deg);
int transform_get_angle_delta(struct coord *c1, struct coord *c2, int dir);
int transform_within_border(struct transformation *this_, struct point *p, int border);
/* end of prototypes */
#ifdef __cplusplus
}
#endif

#endif
