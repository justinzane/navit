#include <stdio.h>
#include "debug.h"
#include "mg.h"

int coord_debug;

static void
street_name_get(struct street_name *name, unsigned char **p)
{
	unsigned char *start=*p;
	name->len=get_u16_unal(p);
	name->country=get_u16_unal(p);
	name->townassoc=get_u32_unal(p);
	name->name1=get_string(p);
	name->name2=get_string(p);
	name->segment_count=get_u32_unal(p);
	name->segments=(struct street_name_segment *)(*p);
	(*p)+=(sizeof (struct street_name_segment))*name->segment_count;
	name->aux_len=name->len-(*p-start);
	name->aux_data=*p;
	name->tmp_len=name->aux_len;
	name->tmp_data=name->aux_data;
	(*p)+=name->aux_len;
}

static void
street_name_get_by_id(struct street_name *name, struct file *file, unsigned long id)
{
	unsigned char *p;
	if (id) {
		p=file->begin+id+0x2000;
		street_name_get(name, &p);
	}
}

static int street_get_bytes(struct coord_rect *r)
{
	int bytes,dx,dy;
        bytes=2;
        dx=r->rl.x-r->lu.x;
        dy=r->lu.y-r->rl.y;
	g_assert(dx > 0);
	g_assert(dy > 0); 
        if (dx > 32767 || dy > 32767)
                bytes=3;
        if (dx > 8388608 || dy > 8388608)
                bytes=4;                  
	
	return bytes;
}

static int street_get_coord(unsigned char **pos, int bytes, struct coord *ref, struct coord *f)
{
	unsigned char *p;
	int x,y,flags=0;
		
	p=*pos;
        x=*p++;
        x|=(*p++) << 8;
        if (bytes == 2) {
		if (   x > 0x7fff) {
			x=0x10000-x;
			flags=1;
		}
	}
	else if (bytes == 3) {
		x|=(*p++) << 16;
		if (   x > 0x7fffff) {
			x=0x1000000-x;
			flags=1;
		}
	} else {
		x|=(*p++) << 16;
		x|=(*p++) << 24;
		if (x < 0) {
			x=-x;
			flags=1;
		}
	}
	y=*p++;
	y|=(*p++) << 8;
	if (bytes == 3) {
		y|=(*p++) << 16;
	} else if (bytes == 4) {
		y|=(*p++) << 16;
		y|=(*p++) << 24;
	}
	if (f) {
		f->x=ref[0].x+x;
		f->y=ref[1].y+y;
	}
	dbg(1,"0x%x,0x%x + 0x%x,0x%x = 0x%x,0x%x\n", x, y, ref[0].x, ref[1].y, f->x, f->y);
	*pos=p;
	return flags;
}

static void
street_coord_get_begin(unsigned char **p)
{
	struct street_str *str;

	str=(struct street_str *)(*p);
	while (L(str->segid)) {
		str++;
	}
	(*p)=(unsigned char *)str;
	(*p)+=4;
}


static void
street_coord_rewind(void *priv_data)
{
	/* struct street_priv *street=priv_data; */

}

static int
street_coord_get(void *priv_data, struct coord *c, int count)
{
	struct street_priv *street=priv_data;
	unsigned char *n;
	int ret=0;

	while (count > 0) {
		if (street->p+street->bytes*2 >= street->end) 
			return ret;
		if (street->status >= 4)
			return ret;
		n=street->p;
		if (street_get_coord(&street->p, street->bytes, street->ref, c)) {
			if (street->status)
				street->next=n;
			street->status+=2;
			if (street->status == 5)
				return ret;
		}
		c++;
		ret++;
		count--;
	}
	return ret;
}

static void
street_attr_rewind(void *priv_data)
{
	/* struct street_priv *street=priv_data; */

}

static int
street_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{
	struct street_priv *street=priv_data;

	dbg(1,"segid 0x%x\n", street->str->segid);
	attr->type=attr_type;
	switch (attr_type) {
	case attr_any:
		while (street->attr_next != attr_none) {
			if (street_attr_get(street, street->attr_next, attr))
				return 1;
		}
		return 0;
	case attr_label:
		if (! street->name.len)
			street_name_get_by_id(&street->name,street->name_file,L(street->str->nameid));
		street->attr_next=attr_name;
		attr->u.str=street->name.name2;
		if (attr->u.str && attr->u.str[0])
			return 1;
		attr->u.str=street->name.name1;
		if (attr->u.str && attr->u.str[0])
			return 1;
		return 0;
	case attr_name:
		if (! street->name.len)
			street_name_get_by_id(&street->name,street->name_file,L(street->str->nameid));
		attr->u.str=street->name.name2;
		street->attr_next=attr_name_systematic;
		return ((attr->u.str && attr->u.str[0]) ? 1:0);
	case attr_name_systematic:
		if (! street->name.len)
			street_name_get_by_id(&street->name,street->name_file,L(street->str->nameid));
		attr->u.str=street->name.name1;
		street->attr_next=attr_debug;
		return ((attr->u.str && attr->u.str[0]) ? 1:0);
	case attr_debug:
		street->attr_next=attr_none;
		{
		struct street_str *str=street->str;
		sprintf(street->debug,"order:0x%x\nsegid:0x%x\nlimit:0x%x\nunknown2:0x%x\nunknown3:0x%x\ntype:0x%x\nnameid:0x%x",street->header->order,str->segid,str->limit,str->unknown2,str->unknown3,str->type,str->nameid);
		attr->u.str=street->debug;
		}
		return 1;
	default:
		return 0;
	}
	return 1;
}

static struct item_methods street_meth = {
	street_coord_rewind,
	street_coord_get,
	street_attr_rewind,
	street_attr_get,
};

static void
street_get_data(struct street_priv *street, unsigned char **p)
{
	street->header=(struct street_header *)(*p);
	(*p)+=sizeof(struct street_header);
	street->type_count=street->header->count;
	street->type=(struct street_type *)(*p);	
	(*p)+=street->type_count*sizeof(struct street_type);
}


                            /*0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 */
static unsigned char limit[]={0,0,1,1,1,2,2,4,6,6,12,13,14,20,20,20,20,20,20};

int
street_get(struct map_rect_priv *mr, struct street_priv *street, struct item *item)
{
	if (mr->b.p == mr->b.p_start) {
		street_get_data(street, &mr->b.p);
		street->name_file=mr->m->file[file_strname_stn];
		if (mr->cur_sel && street->header->order > limit[mr->cur_sel->order[layer_street]])
			return 0;
		street->end=mr->b.end;
		street->ref=&mr->b.b->r.lu;
		street->bytes=street_get_bytes(&mr->b.b->r);
		street->str=(struct street_str *)mr->b.p;
		street->coord_begin=mr->b.p;
		street_coord_get_begin(&street->coord_begin);
		street->p=street->coord_begin;
		street->type--; 
		item->meth=&street_meth;
		item->priv_data=street;
	} else {
		street->str++;
		street->p=street->next;
	}
	if (! L(street->str->segid))
		return 0;
	if (L(street->str->segid) < 0)
		street->type++;
#if 0
	g_assert(street->p != NULL);
#endif
	street->next=NULL;
	street->status_rewind=street->status=L(street->str[1].segid) >= 0 ? 0:1;
#if 0
	if (street->type->country != 0x31) {
		printf("country=0x%x\n", street->type->country);
	}
#endif
	item->id_hi=street->type->country | (mr->current_file << 16);
	item->id_lo=L(street->str->segid) > 0 ? L(street->str->segid) : -L(street->str->segid);
	switch(street->str->type & 0x1f) {
	case 0xf: /* very small street */
		if (street->str->limit == 0x33) 
			item->type=type_street_nopass;
		else
			item->type=type_street_0;
		break;
	case 0xd:
		item->type=type_ferry;
		break;
	case 0xc: /* small street */
		item->type=type_street_1_city;
		break;
	case 0xb:
		item->type=type_street_2_city;
		break;
	case 0xa:
		if ((street->str->limit == 0x03 || street->str->limit == 0x30) && street->header->order < 4)
			item->type=type_street_4_city;
		else	
			item->type=type_street_3_city;
		break;
	case 0x9:
		if (street->header->order < 5)
			item->type=type_street_4_city;
		else if (street->header->order < 7)
			item->type=type_street_2_city;
		else
			item->type=type_street_1_city;
		break;
	case 0x8:
		item->type=type_street_2_land;
		break;
	case 0x7:
		if ((street->str->limit == 0x03 || street->str->limit == 0x30) && street->header->order < 4)
			item->type=type_street_4_land;
		else
			item->type=type_street_3_land;
		break;
	case 0x6:
		item->type=type_ramp;
		break;
	case 0x5:
		item->type=type_street_4_land;
		break;
	case 0x4:
		item->type=type_street_4_land;
		break;
	case 0x3:
		item->type=type_street_n_lanes;
		break;
	case 0x2:
		item->type=type_highway_city;
		break;
	case 0x1:
		item->type=type_highway_land;
		break;
	default:
		item->type=type_street_unkn;
		dbg(0,"unknown type 0x%x\n",street->str->type);
	}
#if 0
	coord_debug=(street->str->unknown2 != 0x40 || street->str->unknown3 != 0x40);
	if (coord_debug) {
		item->type=type_street_unkn;
		printf("%d %02x %02x %02x %02x\n", street->str->segid, street->str->type, street->str->limit, street->str->unknown2, street->str->unknown3);
	}
#endif
	street->p_rewind=street->p;
	street->name.len=0;
	street->attr_next=attr_label;
	return 1;
}

int
street_get_byid(struct map_rect_priv *mr, struct street_priv *street, int id_hi, int id_lo, struct item *item)
{
        int country=id_hi & 0xffff;
        int res;
	dbg(0,"enter(%p,%p,0x%x,0x%x,%p)\n", mr, street, id_hi, id_lo, item);
	if (! country)
		return 0;
        tree_search_hv(mr->m->dirname, "street", (id_lo >> 8) | (country << 24), id_lo & 0xff, &res);
	dbg(0,"res=0x%x (blk=0x%x)\n", res, res >> 12);
        block_get_byindex(mr->m->file[mr->current_file], res >> 12, &mr->b);
	street_get_data(street, &mr->b.p);
	street->name_file=mr->m->file[file_strname_stn];
	street->end=mr->b.end;
	street->ref=&mr->b.b->r.lu;
	street->bytes=street_get_bytes(&mr->b.b->r);
	street->str=(struct street_str *)mr->b.p;
	street->coord_begin=mr->b.p;
	street_coord_get_begin(&street->coord_begin);
	street->p=street->coord_begin;
	street->type--;
	item->meth=&street_meth;
	item->priv_data=street;
	street->str+=(res & 0xfff)-1;
	dbg(1,"segid 0x%x\n", street->str[1].segid);
	return street_get(mr, street, item);
#if 0
        mr->b.p=mr->b.block_start+(res & 0xffff);
        return town_get(mr, twn, item);
#endif

	return 0;
}
      
