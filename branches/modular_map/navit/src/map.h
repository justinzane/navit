struct map_priv;
#include "coord.h"
#include "layer.h"

struct map_selection {
	struct map_selection *next;
	struct coord_rect rect;
	int order[layer_end];		
};

struct map_methods {
	void 			(*map_destroy)(struct map_priv *priv);
	char *			(*map_charset)(struct map_priv *priv);
	enum projection		(*map_projection)(struct map_priv *priv);
	struct map_rect_priv *  (*map_rect_new)(struct map_priv *map, struct map_selection *sel);
	void			(*map_rect_destroy)(struct map_rect_priv *mr);
	struct item *		(*map_rect_get_item)(struct map_rect_priv *mr);
	struct item *		(*map_rect_get_item_byid)(struct map_rect_priv *mr, int id_hi, int id_lo);
};

/* prototypes */
enum projection;
struct item;
struct map;
struct map_rect;
struct map_selection;
struct map *map_new(const char *type, const char *filename);
char *map_get_filename(struct map *this);
char *map_get_type(struct map *this);
int map_get_active(struct map *this);
void map_set_active(struct map *this, int active);
enum projection map_projection(struct map *this);
void map_destroy(struct map *m);
struct map_rect *map_rect_new(struct map *m, struct map_selection *sel);
struct item *map_rect_get_item(struct map_rect *mr);
struct item *map_rect_get_item_byid(struct map_rect *mr, int id_hi, int id_lo);
void map_rect_destroy(struct map_rect *mr);
