struct search_destination {
	char *country_name;
	char *country_car;
	char *country_iso2;	
	char *country_iso3;
	char *town_postal;
	char *town_name;
	char *district;
	char *street_name;
	char *street_number;
	struct country *country;
	struct town *town;
	struct street_name *street;
	struct coord *c;
};

struct search_item {
	int attrs;
	struct attr attr[8];
};

/* prototypes */
struct attr;
struct item;
struct mapset;
struct search;
struct search *search_new(struct mapset *ms, struct item *item, struct attr *search_attr, int partial);
struct item *search_get_item(struct search *this);
void search_destroy(struct search *this);
