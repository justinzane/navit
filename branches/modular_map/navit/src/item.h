#include "attr.h"


enum item_type {
#define ITEM2(x,y) type_##y=x,
#define ITEM(x) type_##x,
#include "item_def.h"
#undef ITEM2
#undef ITEM
};

struct coord;

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
struct item * item_new(char *type, int zoom);

enum item_type item_from_name(char *name);
char * item_to_name(enum item_type item);
