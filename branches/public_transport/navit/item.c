/**
 * Navit, a modular navigation system.
 * Copyright (C) 2005-2008 Navit Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#include <string.h>
#include <glib.h>
#include "coord.h"
#include "debug.h"
#include "item.h"
#include "map.h"
#include "transform.h"

struct item_name {
        enum item_type item;
        char *name;
};

struct item_range item_range_all = { type_none, type_last };

#define AF_PBH (AF_PEDESTRIAN|AF_BIKE|AF_HORSE)
#define AF_MOTORIZED_FAST (AF_MOTORCYCLE|AF_CAR|AF_HIGH_OCCUPANCY_CAR|AF_TAXI|AF_PUBLIC_BUS|AF_DELIVERY_TRUCK|AF_TRANSPORT_TRUCK|AF_EMERGENCY_VEHICLES|AF_DANGEROUS_GOODS)
#define AF_ALL (AF_PBH|AF_MOPED|AF_MOTORIZED_FAST)


struct default_flags {
	enum item_type type;
	int flags;
};

struct default_flags default_flags2[]={
	{type_street_nopass, AF_PBH},
	{type_street_0, AF_ALL},
	{type_street_1_city, AF_ALL},
	{type_street_2_city, AF_ALL},
	{type_street_3_city, AF_ALL},
	{type_street_4_city, AF_ALL},
	{type_highway_city, AF_MOTORIZED_FAST}, 
	{type_street_1_land, AF_ALL},
	{type_street_2_land, AF_ALL},
	{type_street_3_land, AF_ALL},
	{type_street_4_land, AF_ALL},
	{type_street_n_lanes, AF_MOTORIZED_FAST},
	{type_highway_land, AF_MOTORIZED_FAST},
	{type_ramp, AF_MOTORIZED_FAST},
	{type_roundabout, AF_ALL},
	{type_ferry, AF_ALL},
	{type_cycleway, AF_PBH},
	{type_track_paved, AF_ALL},
	{type_track_gravelled, AF_ALL},
        {type_track_unpaved, AF_ALL},
        {type_track_ground, AF_ALL},
        {type_track_grass, AF_ALL},
	{type_footway, AF_PBH},
	{type_living_street, AF_ALL},
	{type_street_service, AF_ALL},
	{type_bridleway, AF_PBH},
	{type_path, AF_PBH},
};



struct item_name item_names[]={
#define ITEM2(x,y) ITEM(y)
#define ITEM(x) { type_##x, #x },
#include "item_def.h"
#undef ITEM2
#undef ITEM
};

static GHashTable *default_flags_hash;

int *
item_get_default_flags(enum item_type type)
{
	if (!default_flags_hash) {
		int i;
		default_flags_hash=g_hash_table_new(NULL, NULL);
		for (i = 0 ; i < sizeof(default_flags2)/sizeof(struct default_flags); i++) {
			g_hash_table_insert(default_flags_hash, (void *)(long)default_flags2[i].type, &default_flags2[i].flags);
		}
	}
	return g_hash_table_lookup(default_flags_hash, (void *)(long)type);
}

void
item_coord_rewind(struct item *it)
{
	it->meth->item_coord_rewind(it->priv_data);
}

int
item_coord_get(struct item *it, struct coord *c, int count)
{
	return it->meth->item_coord_get(it->priv_data, c, count);
}

int
item_coord_get_within_selection(struct item *it, struct coord *c, int count, struct map_selection *sel)
{
	int i,ret=it->meth->item_coord_get(it->priv_data, c, count);
	struct coord_rect r;
	struct map_selection *curr;
	if (ret <= 0 || !sel)
		return ret;
	r.lu=c[0];
	r.rl=c[0];
	for (i = 1 ; i < ret ; i++) {
		if (r.lu.x > c[i].x)
			r.lu.x=c[i].x;
		if (r.rl.x < c[i].x)
			r.rl.x=c[i].x;
		if (r.rl.y > c[i].y)
			r.rl.y=c[i].y;
		if (r.lu.y < c[i].y)
			r.lu.y=c[i].y;
	}
        curr=sel;
	while (curr) {
		struct coord_rect *sr=&curr->u.c_rect;
		if (r.lu.x <= sr->rl.x && r.rl.x >= sr->lu.x &&
		    r.lu.y >= sr->rl.y && r.rl.y <= sr->lu.y)
			return ret;
		curr=curr->next;
	}
        return 0;
}

int
item_coord_get_pro(struct item *it, struct coord *c, int count, enum projection to)
{
	int ret=item_coord_get(it, c, count);
	int i;
	enum projection from=map_projection(it->map);
	if (from != to) 
		for (i = 0 ; i < count ; i++) 
			transform_from_to(c+i, from, c+i, to);
	return ret;
}

int 
item_coord_is_node(struct item *it)
{
	if (it->meth->item_coord_is_node)
		return it->meth->item_coord_is_node(it->priv_data);
	return 0;
}

void
item_attr_rewind(struct item *it)
{
	it->meth->item_attr_rewind(it->priv_data);
}
int
item_attr_get(struct item *it, enum attr_type attr_type, struct attr *attr)
{
	return it->meth->item_attr_get(it->priv_data, attr_type, attr);
}

struct item * item_new(char *type, int zoom)
{
	struct item * it;

	it = g_new0(struct item, 1);

	/* FIXME evaluate arguments */

	return it;
}

enum item_type
item_from_name(const char *name)
{
	int i;

	for (i=0 ; i < sizeof(item_names)/sizeof(struct item_name) ; i++) {
		if (! strcmp(item_names[i].name, name))
			return item_names[i].item;
	}
	return type_none;
}

char *
item_to_name(enum item_type item)
{
	int i;

	for (i=0 ; i < sizeof(item_names)/sizeof(struct item_name) ; i++) {
		if (item_names[i].item == item)
			return item_names[i].name;
	}
	return NULL; 
}

struct item_hash {
	GHashTable *h;
};

static guint
item_hash_hash(gconstpointer key)
{
	const struct item *itm=key;
	gconstpointer hashkey=(gconstpointer)(itm->id_hi^itm->id_lo^((int) itm->map));
	return g_direct_hash(hashkey);
}

static gboolean
item_hash_equal(gconstpointer a, gconstpointer b)
{
	const struct item *itm_a=a;
	const struct item *itm_b=b;
	if (item_is_equal(*itm_a, *itm_b))
		return TRUE;
	return FALSE;
}



struct item_hash *
item_hash_new(void)
{
	struct item_hash *ret=g_new(struct item_hash, 1);

	ret->h=g_hash_table_new_full(item_hash_hash, item_hash_equal, g_free, NULL);
	return ret;
}

void
item_hash_insert(struct item_hash *h, struct item *item, void *val)
{
	struct item *hitem=g_new(struct item, 1);
        *hitem=*item;
	dbg(2,"inserting (0x%x,0x%x) into %p\n", item->id_hi, item->id_lo, h->h);
	g_hash_table_insert(h->h, hitem, val);
}

int
item_hash_remove(struct item_hash *h, struct item *item)
{
	int ret;

	dbg(2,"removing (0x%x,0x%x) from %p\n", item->id_hi, item->id_lo, h->h);
	ret=g_hash_table_remove(h->h, item);
	dbg(2,"ret=%d\n", ret);

	return ret;
}

void *
item_hash_lookup(struct item_hash *h, struct item *item)
{
	return g_hash_table_lookup(h->h, item);
}


void
item_hash_destroy(struct item_hash *h)
{
	g_hash_table_destroy(h->h);
	g_free(h);
}

int
item_range_intersects_range(struct item_range *range1, struct item_range *range2)
{
	if (range1->max < range2->min)
		return 0;
	if (range1->min > range2->max)
		return 0;
	return 1;
}
int
item_range_contains_item(struct item_range *range, enum item_type type)
{
	if (type >= range->min && type <= range->max)
		return 1;
	return 0;
}
