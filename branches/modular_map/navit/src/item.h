#include "attr.h"

enum item_type {
	/* Point */
	type_town_label=1,
	type_district_label,
	type_highway_exit_label,
	/* Line */
	type_border_country,
	type_border_state,
	type_rail,
	type_water_line,
	type_street_0,
	type_street_1_city,
	type_street_2_city,
	type_street_3_city,
	type_street_4_city,
	type_highway_city,
	type_street_1_land,
	type_street_2_land,
	type_street_3_land,
	type_street_4_land,
	type_street_n_lanes,
	type_highway_land,
	type_ramp,
	type_ferry,
	type_street_unkn,
	/* Area */
	type_wood,
	type_water_poly,
	type_town_poly,
	type_industry_poly,
	type_airport_poly,
	type_hospital_poly,
	type_park_poly,
	type_sport_poly,
};
struct item_methods {
	void (*item_coord_rewind)(void *priv_data);
	int (*item_coord_get)(void *priv_data, struct coord *c, int count);
	void (*item_attr_rewind)(void *priv_data);
	int (*item_attr_get)(void *priv_data, enum attr_type attr_type, struct attr *attr);
};

struct item {
	enum item_type type;
	int id_hi;
	int id_lo;
	struct map *map;
	struct item_methods *meth;	
	void *priv_data;
};


void item_coord_rewind(struct item *it);
int item_coord_get(struct item *it, struct coord *c, int count);
void item_attr_rewind(struct item *it);
int item_attr_get(struct item *it, enum attr_type attr_type, struct attr *attr);
