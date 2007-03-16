struct map_priv;
struct layer;
struct coord_rect;

struct map_methods {
	void 			(*map_destroy)(struct map_priv *priv);
	char *			(*map_charset)(struct map_priv *priv);
	enum projection		(*map_projection)(struct map_priv *priv);
	struct map_rect_priv *  (*map_rect_new)(struct map_priv *map, struct coord_rect *r, struct layer *layers, int limit);
	void			(*map_rect_destroy)(struct map_rect_priv *mr);
	struct item *		(*map_rect_get_item)(struct map_rect_priv *mr);
	struct item *		(*map_rect_get_item_byid)(struct map_rect_priv *mr, int id_hi, int id_lo);
};

struct map * map_new(char *type, char *filename);
void map_destroy(struct map *m);
struct map_rect * map_rect_new(struct map *m, struct coord_rect *r, struct layer *layers, int limit);
void map_rect_destroy(struct map_rect *mr);
struct item * map_rect_get_item(struct map_rect *mr);
struct item * map_rect_get_item_byid(struct map_rect *mr, int id_hi, int id_lo);

