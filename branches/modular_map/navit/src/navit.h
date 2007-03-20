struct navit;
struct coord_geo;
struct vehicle;
struct color;

struct navit;
struct navit * navit_new(char *ui, char *graphics, struct coord_geo *center, double zoom);
void navit_vehicle_add(struct navit *nav, struct vehicle *v, struct color *c);
