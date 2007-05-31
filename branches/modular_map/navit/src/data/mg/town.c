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
	*c=twn->c;
	return 1;
}

static void
town_attr_rewind(void *priv_data)
{
	struct town_priv *twn=priv_data;

	twn->aidx=0;
	twn->attr_next=attr_label;
}

static int
town_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{
	struct town_priv *twn=priv_data;

	attr->type=attr_type;
	switch (attr_type) {
	case attr_any:
		while (twn->attr_next != attr_none) {
			if (town_attr_get(twn, twn->attr_next, attr))
				return 1;
		}
		return 0;
	case attr_label:
		attr->u.str=twn->district;
		twn->attr_next=attr_name;
		if (attr->u.str[0])
			return 1;
		attr->u.str=twn->name;
		return ((attr->u.str && attr->u.str[0]) ? 1:0);
	case attr_name:
		attr->u.str=twn->name;
		twn->attr_next=attr_district;
		return ((attr->u.str && attr->u.str[0]) ? 1:0);
	case attr_district:
		attr->u.str=twn->district;
		twn->attr_next=attr_debug;
		return ((attr->u.str && attr->u.str[0]) ? 1:0);
	case attr_debug:
		sprintf(twn->debug, "order %d\nsize %d", twn->order, twn->size);
		attr->u.str=twn->debug;
		twn->attr_next=attr_none;
		return 1;
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
	twn->id=get_u32_unal(p);
	twn->c.x=get_u32_unal(p);
	twn->c.y=get_u32_unal(p);
	twn->name=get_string(p);
	twn->district=get_string(p);
	twn->postal_code1=get_string(p);
	twn->order=get_u8(p);			/* 1-15 (19) */
	twn->country=get_u16(p);
	twn->type=get_u8(p);
	twn->unknown2=get_u32_unal(p);
	twn->size=get_u8(p);
	twn->street_assoc=get_u32_unal(p);
	twn->unknown3=get_u8(p);
	twn->postal_code2=get_string(p);
	twn->unknown4=get_u32_unal(p);
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
		twn->attr_next=attr_label;
#if 0
		if (twn->order <= limit[mr->limit] && coord_rect_contains(&mr->r,&twn->c)) {
#endif
			switch(twn->type) {
			case 1:
				item->type=type_town_label;
				break;
			case 3:
				item->type=type_district_label;
				break;
			case 4:
				item->type=type_port_label;
				break;
			case 9:
				item->type=type_highway_exit_label;
				break;
			default:
				printf("unknown town type 0x%x '%s' '%s' 0x%x,0x%x\n", twn->type, twn->name, twn->district, twn->c.x, twn->c.y);
				item->type=type_town_label;
			}
			item->id_hi=twn->country | (mr->current_file << 16);
			item->id_lo=twn->id;
			item->priv_data=twn;
			item->meth=&town_meth;
			return 1;
#if 0
		}
#endif
	}
}

int
town_get_byid(struct map_rect_priv *mr, struct town_priv *twn, int id_hi, int id_lo, struct item *item)
{
	int country=id_hi & 0xffff;
	int res;
	tree_search_hv(mr->m->dirname, "town", (id_lo >> 8) | (country << 24), id_lo & 0xff, &res);
	block_get_byindex(mr->m->file[mr->current_file], res >> 16, &mr->b);
	mr->b.p=mr->b.block_start+(res & 0xffff);
	return town_get(mr, twn, item);
}
