#include <stdio.h>
#include "attr.h"
#include "coord.h"
struct map_priv {
	int id;
	char *filename;
};

struct map_rect_priv {
	struct coord_rect r;
	int limit;

	FILE *f;
	long pos;
	char line[256];
	int attr_pos;
	enum attr_type attr_last;
	char attrs[256];
	char attr[256];
	double lat,lng;
	char lat_c,lng_c;
	int eoc;
	struct map_priv *m;
	struct item item;
};

