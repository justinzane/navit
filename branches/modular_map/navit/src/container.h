enum data_window_type {
	data_window_type_block=0,
	data_window_type_town,
	data_window_type_poly,
	data_window_type_street,
	data_window_type_point,
	data_window_type_end
};

struct map_flags {
	int orient_north;
	int track;
};

struct layout;

struct container {
	GList *mapsets;
	GList *layouts;
	struct layout *layout_current;
	struct window *win;
	struct action *action;
	struct transformation *trans;
	struct graphics *gra;
	struct compass *compass;
	struct map_data *map_data;
	struct menu *menu;
	struct toolbar *toolbar;
	struct statusbar *statusbar;
	struct route *route;
	struct cursor *cursor;
	struct speech *speech;
	struct vehicle *vehicle;
	struct track *track;
        struct data_window *data_window[data_window_type_end];
	struct map_flags *flags;
	int ready;
};
