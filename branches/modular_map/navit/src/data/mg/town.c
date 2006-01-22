#include <glib.h>
#include <stdio.h>
#include "mg.h"



static void
town_coord_rewind(void *priv_data)
{
	struct town_priv *twn=priv_data;

	twn->cidx=0;
}

static int
town_coord_get(void *priv_data, struct coord *c, int count)
{
	struct town_priv *twn=priv_data;

	if (twn->cidx || count <= 0)
		return 0;
	twn->cidx=1;
	*c=*twn->c;
	return 1;
}

static void
town_attr_rewind(void *priv_data)
{
	struct town_priv *twn=priv_data;

	twn->aidx=0;
}

static int
town_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{
	struct town_priv *twn=priv_data;

	switch (attr_type) {
	case attr_name:
		attr->type=attr_name;
		attr->u.str=twn->name;
		break;
	default:
		g_assert(1==0);
		return 0;
	}
	return 1;
}

static struct item_methods town_meth = {
	town_coord_rewind,
	town_coord_get,
	town_attr_rewind,
	town_attr_get,
};
static void
town_get_data(struct town_priv *twn, unsigned char **p)
{
	twn->id=get_long(p);
	twn->c=coord_get(p);
	twn->name=get_string(p);
	twn->district=get_string(p);
	twn->postal_code1=get_string(p);
	twn->order=get_char(p);			/* 1-15 (19) */
	twn->country=get_short(p);
	twn->type=get_char(p);
	twn->unknown2=get_long(p);
	twn->size=get_char(p);
	twn->street_assoc=get_long(p);
	twn->unknown3=get_char(p);
	twn->postal_code2=get_string(p);
	twn->unknown4=get_long(p);
#if 0
		printf("%s\t%s\t%s\t%d\t%d\t%d\n",twn->name,twn->district,twn->postal_code1,twn->order, twn->country, twn->type);
#endif
}
                     /*0 1 2 3 4 5 6 7  8  9  10 11 12 13 14 */
unsigned char limit[]={0,1,2,2,4,6,8,10,11,12,12,13,14,20,20};

int
town_get(struct map_rect_priv *mr, struct town_priv *twn, struct item *item)
{
	for (;;) {
		if (mr->b.p >= mr->b.end)
			return 0;
		town_get_data(twn, &mr->b.p);
		twn->cidx=0;
		twn->aidx=0;
		if (twn->order == 19)
			printf("%s %s\n", twn->name, twn->district);		
		if (twn->order <= limit[mr->limit] && coord_rect_contains(&mr->r,twn->c)) {
			switch(twn->type) {
			case 1:
				item->type=type_town_label;
				break;
			case 3:
				item->type=type_district_label;
				break;
			case 9:
				item->type=type_highway_exit_label;
				break;
			default:
				printf("unknown town type %d\n", twn->type);
				item->type=type_town_label;
			}
			item->id_hi=twn->country;
			item->id_lo=twn->id;
			item->priv_data=twn;
			item->meth=&town_meth;
			return 1;
		}
	}
}

