#ifdef __cplusplus
extern "C" {
#endif
extern struct gui *main_loop_gui;
/* prototypes */
enum projection;
enum item_type;
struct callback;
struct color;
struct coord;
struct displaylist;
struct gui;
struct layout;
struct mapset;
struct menu;
struct navigation;
struct navit;
struct navit_vehicle;
struct point;
struct route;
struct speech;
struct tracking;
struct transformation;
struct vehicle;
void navit_add_mapset(struct navit *this_, struct mapset *ms);
struct mapset *navit_get_mapset(struct navit *this_);
void navit_add_layout(struct navit *this_, struct layout *lay);
void navit_draw(struct navit *this_);
void navit_draw_displaylist(struct navit *this_);
void navit_zoom_in(struct navit *this_, int factor);
void navit_zoom_out(struct navit *this_, int factor);
struct navit *navit_new(const char *ui, const char *graphics, struct coord *center, enum projection pro, int zoom);
void navit_set_destination(struct navit *this_, struct coord *c, char *description);
void navit_add_bookmark(struct navit *this_, struct coord *c, char *description);
void navit_add_menu_layouts(struct navit *this_, struct menu *men);
void navit_add_menu_layout(struct navit *this_, struct menu *men);
void navit_add_menu_projections(struct navit *this_, struct menu *men);
void navit_add_menu_projection(struct navit *this_, struct menu *men);
void navit_add_menu_maps(struct navit *this_, struct mapset *ms, struct menu *men);
void navit_add_menu_former_destinations(struct navit *this_, struct menu *men, struct route *route);
void navit_add_menu_bookmarks(struct navit *this_, struct menu *men);
void navit_add_menu_vehicles(struct navit *this_, struct menu *men);
void navit_add_menu_vehicle(struct navit *this_, struct menu *men);
void navit_speak(struct navit *this_);
void navit_window_roadbook_new(struct navit *this_);
void navit_window_roadbook_destroy(struct navit *this_);
struct navit_window_items * navit_window_items_new(char *name, int distance);
void navit_window_items_add_item(struct navit_window_items *nwi, enum item_type type);
void navit_add_window_items(struct navit *this_, struct navit_window_items *nwi);
void navit_add_menu_windows_items(struct navit *this_, struct menu *men);
void navit_init(struct navit *this_);
void navit_set_center(struct navit *this_, struct coord *center);
void navit_set_center_screen(struct navit *this_, struct point *p);
void navit_toggle_cursor(struct navit *this_);
void navit_toggle_tracking(struct navit *this_);
void navit_toggle_orient_north(struct navit *this_);
void navit_set_position(struct navit *this_, struct coord *c);
struct navit_vehicle *navit_add_vehicle(struct navit *this_, struct vehicle *v, const char *name, struct color *c, int update, int follow);
void navit_add_vehicle_cb(struct navit *this_, struct callback *cb);
void navit_remove_vehicle_cb(struct navit *this_, struct callback *cb);
void navit_set_vehicle(struct navit *this_, struct navit_vehicle *nv);
void navit_tracking_add(struct navit *this_, struct tracking *tracking);
void navit_route_add(struct navit *this_, struct route *route);
void navit_navigation_add(struct navit *this_, struct navigation *navigation);
void navit_set_speech(struct navit *this_, struct speech *speech);
struct gui *navit_get_gui(struct navit *this_);
struct transformation *navit_get_trans(struct navit *this_);
struct route *navit_get_route(struct navit *this_);
struct navigation *navit_get_navigation(struct navit *this_);
struct displaylist *navit_get_displaylist(struct navit *this_);
void navit_destroy(struct navit *this_);
/* end of prototypes */
#ifdef __cplusplus
}
#endif
