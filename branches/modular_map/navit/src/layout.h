#include "item.h"
#include "color.h"

struct element_line;
struct element_text;

struct element {
	enum { element_point, element_polyline, element_polygon, element_circle, element_icon, element_image } type;
	struct color color;
	int label_size;
	union {
		struct element_point {
		} point;
		struct element_polyline {
			int width;
		} polyline;
		struct element_polygon {
		} polygon;
		struct element_circle {
			int width;
			int radius;
		} circle;
		struct element_icon {
			char *src;
		} icon;
	} u;
};


struct itemtype { 
	int zoom_min, zoom_max;
	GList *type;
	GList *elements;
};

struct layer { char *name; int details; GList *itemtypes; };

struct layout { char *name; GList *layers; };

/* prototypes */
struct color;
struct element;
struct itemtype;
struct layer;
struct layout;
struct layout *layout_new(char *name);
struct layer *layer_new(char *name, int details);
void layout_add_layer(struct layout *layout, struct layer *layer);
struct itemtype *itemtype_new(int zoom_min, int zoom_max);
void itemtype_add_type(struct itemtype *this, enum item_type type);
void layer_add_itemtype(struct layer *layer, struct itemtype *itemtype);
void itemtype_add_element(struct itemtype *itemtype, struct element *element);
struct element *polygon_new(struct color *color);
struct element *polyline_new(struct color *color, int width);
struct element *circle_new(struct color *color, int radius, int width, int label_size);
struct element *icon_new(char *src);
struct element *image_new(void);
