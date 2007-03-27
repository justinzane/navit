/* prototypes */
struct color;
struct coord_geo;
struct layout;
struct mapset;
struct navit;
struct vehicle;
void navit_add_mapset(struct navit *this, struct mapset *ms);
void navit_add_layout(struct navit *this, struct layout *lay);
void navit_draw(struct navit *this);
void navit_zoom_in(struct navit *this, int factor);
void navit_zoom_out(struct navit *this, int factor);
struct navit *navit_new(char *ui, char *graphics, struct coord_geo *center, double zoom);
void navit_init(struct navit *this);
void navit_vehicle_add(struct navit *this, struct vehicle *v, struct color *c);
void navit_destroy(struct navit *this);
