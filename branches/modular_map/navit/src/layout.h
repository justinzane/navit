#include <glib.h>
#include "item.h"

struct element_line;
struct element_text;

struct element {
	int type;
	union {
		struct element_line {} line;
		struct element_text {} text;
	};
};


struct itemtype { enum item_type type; GList *elements; };

struct level { int zoom; GList *itemtypes; };

struct layer { char *name; int details; GList *levels; };

struct layout { char *name; GList *layers; };



struct layout * layout_new(char *name);
struct layer * layer_new(char *name, int datails);
void layout_add_layer(struct layout *layout, struct layer *layer);
void layer_add_item(struct layer *layer, struct item * item);

