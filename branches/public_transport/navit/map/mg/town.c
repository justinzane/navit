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

#include <stdio.h>
#include <string.h>
#include "debug.h"
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
	int len;

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
		twn->attr_next=attr_town_name;
		if (attr->u.str[0])
			return 1;
		attr->u.str=twn->name;
		return ((attr->u.str && attr->u.str[0]) ? 1:0);
	case attr_town_name:
		attr->u.str=twn->name;
		twn->attr_next=attr_town_postal;
		return ((attr->u.str && attr->u.str[0]) ? 1:0);
	case attr_town_postal:
		strncpy(twn->postal, twn->postal_code1, 32);
		attr->u.str=twn->postal;
		len=mg_country_postal_len(twn->country);
		if (!len)
			len=31;
		twn->postal[len]='\0';
		twn->attr_next=attr_district_name;
		return ((attr->u.str && attr->u.str[0]) ? 1:0);
	case attr_district_name:
		attr->u.str=twn->district;
		twn->attr_next=attr_debug;
		return ((attr->u.str && attr->u.str[0]) ? 1:0);
	case attr_town_streets_item:
		twn->town_attr_item.type=type_town_streets;
		twn->town_attr_item.id_hi=twn->country | (file_town_twn << 16) | 0x10000000;
		twn->town_attr_item.id_lo=twn->street_assoc;
		attr->u.item=&twn->town_attr_item;
		twn->attr_next=attr_debug;
		return 1;
	case attr_debug:
		sprintf(twn->debug, "order %d\nsize %d\nstreet_assoc 0x%x", twn->order, twn->size, twn->street_assoc);
		attr->u.str=twn->debug;
		twn->attr_next=attr_none;
		return 1;
	default:
		dbg_assert(1==0);
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
	twn->country=get_u16_unal(p);
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
                            /*0 1 2 3 4 5 6 7  8  9  10 11 12 13 14 15 16 17 18 */
static unsigned char limit[]={0,1,2,2,4,6,8,10,11,13,14,14,14,20,20,20,20,20,20};

static enum item_type town_item[]={type_town_label_5e1, type_town_label_1e2, type_town_label_2e2, type_town_label_5e2, type_town_label_1e3, type_town_label_1e3, type_town_label_2e3, type_town_label_5e3, type_town_label_1e4, type_town_label_2e4, type_town_label_5e4, type_town_label_1e5, type_town_label_1e5, type_town_label_2e5, type_town_label_5e5, type_town_label_1e6, type_town_label_2e6};
static enum item_type district_item[]={type_district_label_5e1, type_district_label_1e2, type_district_label_2e2, type_district_label_5e2, type_district_label_1e3, type_district_label_1e3, type_district_label_2e3, type_district_label_5e3, type_district_label_1e4, type_district_label_2e4, type_district_label_5e4, type_district_label_1e5, type_district_label_1e5, type_district_label_2e5, type_district_label_5e5, type_district_label_1e6, type_district_label_2e6};
int
town_get(struct map_rect_priv *mr, struct town_priv *twn, struct item *item)
{
	int size;
	for (;;) {
		if (mr->b.p >= mr->b.end)
			return 0;
		town_get_data(twn, &mr->b.p);
		twn->cidx=0;
		twn->aidx=0;
		twn->attr_next=attr_label;
		if (! mr->cur_sel || (twn->order <= limit[mr->cur_sel->order] && coord_rect_contains(&mr->cur_sel->u.c_rect,&twn->c))) {
			switch(twn->type) {
			case 1:
				size=twn->size;
				if (size >= sizeof(town_item)/sizeof(enum item_type)) 
					size=sizeof(town_item)/sizeof(enum item_type)-1;
				item->type=town_item[size];
				break;
			case 3:
				size=twn->size;
				if (size == 6 && twn->order < 14)
					size++;
				if (size == 5 && twn->order < 14)
					size+=2;
				if (size >= sizeof(district_item)/sizeof(enum item_type)) 
					size=sizeof(district_item)/sizeof(enum item_type)-1;
				item->type=district_item[size];
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
			if (map_selection_contains_item(mr->cur_sel, 0, item->type)) {
				item->id_hi=twn->country | (mr->current_file << 16);
				item->id_lo=twn->id;
				item->priv_data=twn;
				item->meth=&town_meth;
				return 1;
			}
		}
	}
}

int
town_get_byid(struct map_rect_priv *mr, struct town_priv *twn, int id_hi, int id_lo, struct item *item)
{
	int country=id_hi & 0xffff;
	int res;
	if (!tree_search_hv(mr->m->dirname, "town", (id_lo >> 8) | (country << 24), id_lo & 0xff, &res))
		return 0;
	block_get_byindex(mr->m->file[mr->current_file], res >> 16, &mr->b);
	mr->b.p=mr->b.block_start+(res & 0xffff);
	return town_get(mr, twn, item);
}

static int
town_search_compare(unsigned char **p, struct map_rect_priv *mr)
{
        int country, d;
        char *name;

	if (mr->search_type == attr_town_postal) {
		mr->search_blk_count=1;
		mr->search_blk_off=(struct block_offset *)(*p);
		*p+=4;
		name=get_string(p);
		d=0;
	} else {
		country=get_u16_unal(p);
		dbg(1,"country 0x%x ", country);
		name=get_string(p);
		dbg(1,"name '%s' ",name);
		mr->search_blk_count=get_u32_unal(p);
		mr->search_blk_off=(struct block_offset *)(*p);
		dbg(1,"len %d ", mr->search_blk_count);
		(*p)+=mr->search_blk_count*4;
		d=mr->search_country-country;
	}
	if (!d) {
		if (mr->search_partial)
			d=strncasecmp(mr->search_str, name, strlen(mr->search_str));
		else
			d=strcasecmp(mr->search_str, name);
	}
	dbg(1,"%d \n",d);
	return d;

}



struct item *
town_search_get_item(struct map_rect_priv *mr)
{
	int dir=1,leaf;

	if (! mr->search_blk_count) {
		dbg(1,"partial %d 0x%x '%s' ***\n", mr->search_partial, mr->search_country, mr->search_str);
		if (! mr->search_linear) {
			while ((leaf=tree_search_next(&mr->ts, &mr->search_p, dir)) != -1) {
				dir=town_search_compare(&mr->search_p, mr);
				if (! dir && leaf) {
					mr->search_linear=1;
					mr->search_p=NULL;
					break;
				}
			}
			if (! mr->search_linear) {
				dbg(1,"not found\n");
				return NULL;
			}
		}
		if (! tree_search_next_lin(&mr->ts, &mr->search_p)) {
			dbg(1,"linear not found\n");
			return NULL;
		}
		if (town_search_compare(&mr->search_p, mr)) {
			dbg(1,"no match\n");
			return NULL;
		}
		dbg(1,"found %d blocks\n",mr->search_blk_count);
	}
	if (! mr->search_blk_count)
		return NULL;
	dbg(1,"block 0x%x offset 0x%x\n", block_offset_get_block(mr->search_blk_off), block_offset_get_offset(mr->search_blk_off));
	block_get_byindex(mr->m->file[mr->current_file], block_offset_get_block(mr->search_blk_off), &mr->b);
	mr->b.p=mr->b.block_start+block_offset_get_offset(mr->search_blk_off);
	town_get(mr, &mr->town, &mr->item);
	mr->search_blk_off++;
	mr->search_blk_count--;
	return &mr->item;
}
