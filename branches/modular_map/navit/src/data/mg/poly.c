#include <stdio.h>
#include "mg.h"

static void
poly_coord_rewind(void *priv_data)
{
	struct poly_priv *poly=priv_data;

	poly->p=poly->subpoly_start;	

}

static int
poly_coord_get(void *priv_data, struct coord *c, int count)
{
	struct poly_priv *poly=priv_data;
	int ret=0;

	while (count--) {
		if (poly->p >= poly->subpoly_next)
			break;
		*c++=*(coord_get(&poly->p));
		ret++;
	}
	return ret;
}

static void 
poly_attr_rewind(void *priv_data)
{
	struct poly_priv *poly=priv_data;

	poly->aidx=0;
}

static int
poly_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{
	/* struct poly_priv *poly=priv_data; */

	switch (attr_type) {
	default:
		return 0;
	}
	return 1;
}

static struct item_methods poly_meth = {
	poly_coord_rewind,
	poly_coord_get,
	poly_attr_rewind,
	poly_attr_get,
};

static void
poly_get_data(struct poly_priv *poly, unsigned char **p)
{
	poly->c=(struct coord *) (*p);
	*p+=3*sizeof(struct coord);
	poly->name=(char *)(*p);
	while (**p) {
		(*p)++;
	}
	(*p)++;
	poly->order=*(*p)++;
	poly->type=*(*p)++;
	poly->polys=*(unsigned int *)(*p); (*p)+=sizeof(unsigned int);
	poly->count=(unsigned int *)(*p); (*p)+=poly->polys*sizeof(unsigned int);
	poly->count_sum=*(unsigned long *)(*p); (*p)+=sizeof(unsigned long);
}

int
poly_get(struct map_rect_priv *mr, struct poly_priv *poly, struct item *item)
{
	struct coord_rect r;

        for (;;) {
                if (mr->b.p >= mr->b.end)
                        return 0;
		if (mr->b.p == mr->b.p_start) {
			poly->poly_num=0;
			poly->subpoly_num=0;
			poly->poly_next=mr->b.p;
			item->type=2;
			item->meth=&poly_meth;
		}
		if (poly->poly_num >= mr->b.b->count)
			return 0;
		if (!poly->subpoly_num) {
			mr->b.p=poly->poly_next;
			poly_get_data(poly, &mr->b.p);
			poly->poly_next=mr->b.p+poly->count_sum*sizeof(struct coord);
			poly->poly_num++;
			if (poly->order >= mr->limit*3) {
				mr->b.p=poly->poly_next;
				continue;
			}
			r.lu=poly->c[0];
			r.rl=poly->c[1];
			switch(poly->type) {
			case 0x13:
				item->type=type_wood;
				break;
			case 0x14:
				item->type=type_town_poly;
				break;
			case 0x1e:
				item->type=type_industry_poly;
				break;
			case 0x28:
				item->type=type_airport_poly;
				break;
			case 0x2d:
				item->type=type_hospital_poly;
				break;
			case 0x32:
				item->type=type_park_poly;
				break;
			case 0x34:
				item->type=type_sport_poly;
				break;
			case 0x3c:
				item->type=type_water_poly;
				break;
			case 0xbc:
				item->type=type_water_line;
				break;
			case 0xc6:
				item->type=type_border_country;
				break;
			case 0xc7:
				item->type=type_border_state;
				break;
			case 0xd0:
				item->type=type_rail;
				break;
			default:
				printf("Unknown type 0x%x '%s' 0x%x,0x%x\n", poly->type,poly->name,r.lu.x,r.lu.y);
				item->type=type_rail;
			}
			if (!coord_rect_overlap(&mr->r, &r)) {
				mr->b.p=poly->poly_next;
				continue;
			}
		} else 
			mr->b.p=poly->subpoly_next;
		poly->subpoly_next=mr->b.p+poly->count[poly->subpoly_num]*sizeof(struct coord);
		poly->subpoly_num++;
		if (poly->subpoly_num >= poly->polys) 
			poly->subpoly_num=0;
		poly->subpoly_start=poly->p=mr->b.p;
		item->priv_data=poly;
		return 1;
        }
}

