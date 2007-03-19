struct color;
struct coord_geo;

struct container * navit_new(char *ui, char *graphics, struct coord_geo *center, double zoom);
void navit_vehicle_add(struct container *co, struct vehicle *v, struct color *c);
