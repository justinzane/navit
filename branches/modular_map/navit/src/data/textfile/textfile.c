#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "plugin.h"
#include "map.h"
#include "maptype.h"
#include "item.h"
#include "attr.h"
#include "coord.h"
#include "transform.h"

#include "textfile.h"

static int map_id;

static int
contains_coord(char *line)
{
	return g_ascii_isdigit(line[0]);
}

static int debug=0;

static int
get_tag(char *line, char *name, int *pos, char *ret)
{
	int len,quoted;
	char *p,*e,*n;
	/* printf("get_tag %s from %s\n", name, line); */
	if (! name)
		return 0;
	len=strlen(name);
	if (pos) 
		p=line+*pos;
	else
		p=line;
	for(;;) {
		while (*p == ' ') {
			p++;
		}
		if (! *p)
			return 0;
		n=p;
		e=index(p,'=');
		if (! e)
			return 0;
		p=e+1;
		quoted=0;
		while (*p) {
			if (*p == ' ' && !quoted)
				break;
			if (*p == '"')
				quoted=1-quoted;
			p++;
		}
		if (e-n == len && !strncmp(n, name, len)) {
			e++;
			len=p-e;
			if (e[0] == '"') {
				e++;
				len-=2;
			}
			strncpy(ret, e, len);
			ret[len]='\0';
			if (pos)
				*pos=p-line;
			return 1;
		}
	}	
	return 0;
}

static void
get_line(struct map_rect_priv *mr)
{
	fgets(mr->line, 256, mr->f);
}

static void
map_destroy_textfile(struct map_priv *m)
{
	if (debug)
		printf("map_destroy_textfile\n");
	g_free(m);
}

static char *
map_charset_textfile(struct map_priv *m)
{
	return "iso8859-1";
}

static void
textfile_coord_rewind(void *priv_data)
{
}

static int
textfile_coord_get(void *priv_data, struct coord *c, int count)
{
	double lat,lng,lat_deg,lng_deg;
	char lat_c,lng_c;
	struct map_rect_priv *mr=priv_data;
	int pos,ret=0;
	if (debug)
		printf("textfile_coord_get %d\n",count);
	while (count--) {
		if (contains_coord(mr->line) && !feof(mr->f) && (!mr->item.id_hi || !mr->eoc)) {
			pos=0;
			sscanf(mr->line,"%lf %c %lf %c %n",&lat,&lat_c,&lng,&lng_c,&pos);
			if (mr->item.id_hi) {
				if (pos)
					strcpy(mr->attrs, mr->line+pos);
				else
					mr->attrs[0]='\0';
			}
			lat_deg=floor(lat/100);
			lat-=lat_deg*100;
			lat_deg+=lat/60;

			lng_deg=floor(lng/100);
			lng-=lng_deg*100;
			lng_deg+=lng/60;

			transform_mercator(&lng_deg, &lat_deg, c);
			c++;
			ret++;		
			get_line(mr);
			if (mr->item.id_hi)
				mr->eoc=1;
		} else {
			break;
		}
	}
	return ret;
}

static void
textfile_attr_rewind(void *priv_data)
{
}

static int
textfile_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{
	struct map_rect_priv *mr=priv_data;
	char *str=NULL;
	if (debug)
		printf("textfile_attr_get attrs=%s\n", mr->attrs);
	if (attr_type != mr->attr_last) {
		mr->attr_pos=0;
	}
	switch (attr_type) {
	case attr_name:
		str="name";
		break;
	case attr_icon:
		str="icon";
		break;
	default:
		break;
	}
	if (get_tag(mr->attrs,str,&mr->attr_pos,mr->attr)) {
		attr->u.str=mr->attr;
		return 1;
	}
	return 0;
}

static struct item_methods methods_textfile = {
        textfile_coord_rewind,
        textfile_coord_get,
        textfile_attr_rewind,
        textfile_attr_get,
};

static struct map_rect_priv *
map_rect_new_textfile(struct map_priv *map, struct coord_rect *r, struct layer *layers, int limit)
{
	struct map_rect_priv *mr;

	mr=g_new0(struct map_rect_priv, 1);
	mr->m=map;
	mr->r=*r;
	mr->limit=limit;
	mr->item.id_hi=0;
	mr->item.id_lo=0;
	mr->item.meth=&methods_textfile;
	mr->item.priv_data=mr;
	mr->f=fopen(map->filename, "r");
	get_line(mr);
	return mr;
}


static void
map_rect_destroy_textfile(struct map_rect_priv *mr)
{
	fclose(mr->f);
        g_free(mr);
}

static struct item *
map_rect_get_item_textfile(struct map_rect_priv *mr)
{
	char *p,type[256];
	if (debug)
		printf("map_rect_get_item_textfile\n");
	for(;;) {
		if (feof(mr->f)) {
			if (mr->item.id_hi) {
				return NULL;
			}
			mr->item.id_hi++;
			fseek(mr->f, 0, SEEK_SET);
			get_line(mr);
		}
		if (mr->item.id_hi) {
			if ((p=index(mr->line,'\n'))) 
				*p='\0';
			if (!contains_coord(mr->line)) {
				get_line(mr);
				continue;
			}
			mr->eoc=0;
		} else {
			strcpy(mr->attrs, mr->line);
			get_line(mr);
			if ((p=index(mr->attrs,'\n'))) {
				*p='\0';
			}
			if (! mr->attrs[0]) {
				for(;;) {
					if (feof(mr->f))
						break;
					get_line(mr);
					if (!contains_coord(mr->line))
						break;
				}
				continue;
			}
			if (debug)
				printf("attrs=%s\n", mr->attrs);
		}
		if (get_tag(mr->attrs,"type",NULL,type)) {
			if (debug)
				printf("type='%s'\n", type);
			if (!strcmp(type,"roadbook")) {
				mr->item.type=type_roadbook;
			} else if (!strcmp(type,"waypoint")) {
				mr->item.type=type_waypoint;
			} else if (!strcmp(type,"poi")) {
				mr->item.type=type_poi;
			}
		}
		mr->item.id_lo++;
		mr->attr_last=attr_none;
		return &mr->item;
	}
}

static struct map_methods map_methods_textfile = {
	map_destroy_textfile,
	map_charset_textfile,
	map_rect_new_textfile,
	map_rect_destroy_textfile,
	map_rect_get_item_textfile,
};

static struct map_priv *
map_new_textfile(struct map_methods *meth, char *filename)
{
	struct map_priv *m;
	if (debug)
		printf("map_new_textfile %s\n",filename);	
	*meth=map_methods_textfile;
	m=g_new(struct map_priv, 1);
	m->id=++map_id;
	m->filename=g_strdup(filename);
	return m;
}

void
plugin_init(void)
{
	if (debug)
		printf("textfile: plugin_init\n");
	maptype_register("textfile", map_new_textfile);
}

