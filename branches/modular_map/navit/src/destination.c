#include <gtk/gtk.h>
#include "attr.h"
#include "item.h"
#include "search.h"
#if 0 /* FIXME */
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "coord.h"
#include "transform.h"
#include "data_window.h"
#include "country.h"
#include "town.h"
#include "street.h"
#include "street_name.h"
#include "gui/gtk/gtkeyboard.h"
#include "cursor.h"
#include "route.h"
#include "statusbar.h"
#include "unistd.h"
#include "destination.h"
#include "coord.h"
#include "container.h"
#include "graphics.h"

extern gint track_focus(gpointer data);


#endif

GtkWidget *entry_country, *entry_postal, *entry_city, *entry_district;
GtkWidget *entry_street, *entry_number;
GtkWidget *listbox, *current=NULL;
GtkWidget *treeview;
GtkListStore *liststore;
GtkListStore *liststore2;
int row_count=8;

int selected;

struct search_param {
	struct mapset *ms;
#if 0
	const char *country;
	GHashTable *country_hash;
	const char *town;
	GHashTable *town_hash;
	GHashTable *district_hash;
	const char *street;
	GHashTable *street_hash;
	const char *number;
	int number_low, number_high;
	GtkWidget *clist;
	int count;
#endif
} search_param2;

#if 0
struct destination {
	struct town *town;
	struct street_name *street_name;
	struct coord *c;
};

struct country_list *country_list;

static void
select_row(GtkCList *clist, int row, int column, GdkEventButton *event, struct data_window *win)
{
	selected=row;
	gchar *text;
	printf("Selected row %d, column %d\n", row, column);
	
	if(current==entry_country) {
		gtk_clist_get_text(GTK_CLIST(clist),row,0,&text);
		gtk_entry_set_text(GTK_ENTRY(entry_country),g_strdup(text));
	}
	else if( current==entry_city) {
		gtk_clist_get_text(GTK_CLIST(clist),row,5,&text);
		gtk_entry_set_text(GTK_ENTRY(entry_city),g_strdup(text));
	}
	else if( current==entry_street) {
		gtk_clist_get_text(GTK_CLIST(clist),row,4,&text);
		gtk_entry_set_text(GTK_ENTRY(entry_street),g_strdup(text));
	}
	else if( current==entry_number) {
		gtk_clist_get_text(GTK_CLIST(clist),row,8,&text);
		gtk_entry_set_text(GTK_ENTRY(entry_number),g_strdup(text));
	}
}

int
destination_set(struct container *co, enum destination_type type, char *text, struct coord *pos)
{
	route_set_position(co->route, cursor_pos_get(co->cursor));
	route_set_destination(co->route, pos);
	graphics_redraw(co);
	if (co->statusbar && co->statusbar->statusbar_route_update)
		co->statusbar->statusbar_route_update(co->statusbar, co->route);
	return 0;
}

static int
get_position(struct search_param *search, struct coord *c)
{
	struct destination *dest;

	if (selected == -1)
		selected=0;
	dest=gtk_clist_get_row_data (GTK_CLIST(search->clist), selected);

	printf("row %d dest %p dest:0x%lx,0x%lx\n", selected, dest, dest->c->x, dest->c->y);
	*c=*dest->c;
	return 0;
}

static void button_map(GtkWidget *widget, struct container *co)
{
	struct coord c;

	if (!get_position(&search_param2, &c)) {
		graphics_set_view(co, &c.x, &c.y, NULL);
	}
}

static void button_destination(GtkWidget *widget, struct container *co)
{
	struct coord c;

	if (!get_position(&search_param2, &c)) {
		route_set_position(co->route, cursor_pos_get(co->cursor));
		route_set_destination(co->route, &c);
	        graphics_redraw(co);
	}
}

struct dest_town {
	int country;
	int assoc;
	char *name;
	char postal_code[16];
	struct town town;
};

static guint
destination_town_hash(gconstpointer key)
{
	const struct dest_town *hash=key;
	gconstpointer hashkey=(gconstpointer)(hash->country^hash->assoc);
	return g_direct_hash(hashkey);	
}

static gboolean
destination_town_equal(gconstpointer a, gconstpointer b)
{
	const struct dest_town *t_a=a;	
	const struct dest_town *t_b=b;	
	if (t_a->assoc == t_b->assoc && t_a->country == t_b->country) {
		if (t_a->name && t_b->name && strcmp(t_a->name, t_b->name))
			return FALSE;
		return TRUE;
	}
	return FALSE;
}

static GHashTable *
destination_town_new(void)
{
	return g_hash_table_new_full(destination_town_hash, destination_town_equal, NULL, g_free);
}

static void
destination_town_set(const struct dest_town *town, char **rows, int full)
{
	char country[32];
	struct country *cou;
	if ((cou=country_get_by_id(town->country))) {
		rows[1]=cou->car;
	} else {
		sprintf(country,"(%d)", town->country);
		rows[1]=country;
	} 
	if (full) {
		rows[4]=(char *)(town->town.postal_code2);
		rows[5]=g_convert(town->town.name,-1,"utf-8","iso8859-1",NULL,NULL,NULL);
		if (town->town.district[0]) 
			rows[6]=g_convert(town->town.district,-1,"utf-8","iso8859-1",NULL,NULL,NULL);
		else
			rows[6]=NULL;
	} else {
		rows[4]=(char *)(town->postal_code);
		rows[5]=g_convert(town->name,-1,"utf-8","iso8859-1",NULL,NULL,NULL);
	} 
}

static void
destination_town_show(gpointer key, gpointer value, gpointer user_data)
{
	struct dest_town *town=value;
	struct search_param *search=(struct search_param *)user_data;
	char *rows[9]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
	int row;

	if (search->count > 0) {
		struct destination *dest=g_new(struct destination, 1);
		dest->town=&town->town;
		dest->street_name=NULL;
		dest->c=town->town.c;
		destination_town_set(town, rows, 0);
		row=gtk_clist_append(GTK_CLIST(search->clist), rows);
		printf("town row %d %p dest:0x%lx,0x%lx\n", row, dest, dest->c->x, dest->c->y);
		gtk_clist_set_row_data(GTK_CLIST(search->clist), row, dest);
		search->count--;
	}
}

static GHashTable *
destination_country_new(void)
{
	return g_hash_table_new_full(NULL, NULL, NULL, g_free);
}

static int
destination_country_add(struct country *cou, void *data)
{
	struct search_param *search=data;
	struct country *cou2;

	void *first;
	first=g_hash_table_lookup(search->country_hash, (void *)(cou->id));
	if (! first) {
		cou2=g_new(struct country, 1);
		*cou2=*cou;
		g_hash_table_insert(search->country_hash, (void *)(cou->id), cou2);
	}
	return 0;
}

static void
destination_country_show(gpointer key, gpointer value, gpointer user_data)
{
	struct country *cou=value;
	struct search_param *search=(struct search_param *)user_data;
	char *rows[9]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
	if (search->count > 0) {
		rows[0]=cou->name;
		rows[1]=cou->car;
		rows[2]=cou->iso2;
		rows[3]=cou->iso3;
		gtk_clist_append(GTK_CLIST(search->clist), rows);
		search->count--;
	}
}

static int
destination_town_add(struct town *town, void *data)
{
	struct search_param *search=data;
	struct dest_town *first;

	struct dest_town cmp;
	char *zip1, *zip2;

	if (town->id == 0x1d546b7e) {
		printf("found\n");
	}
	cmp.country=town->country;
	cmp.assoc=town->street_assoc;
	cmp.name=town->name;
	first=g_hash_table_lookup(search->town_hash, &cmp);
	if (! first) {
		first=g_new(struct dest_town, 1);
		first->country=cmp.country;
		first->assoc=cmp.assoc;
		strcpy(first->postal_code, town->postal_code2);
		first->name=town->name;
		first->town=*town;
		g_hash_table_insert(search->town_hash, first, first);
	} else {
		zip1=town->postal_code2;
		zip2=first->postal_code;
		while (*zip1 && *zip2) {
			if (*zip1 != *zip2) {
				while (*zip2) {
					*zip2++='.';
				}
				break;
			}
			zip1++;
			zip2++;	
		}
	}
	cmp.name=NULL;
	cmp.assoc=town->id;
	first=g_hash_table_lookup(search->district_hash, &cmp);
	if (! first) {
		first=g_new(struct dest_town, 1);
		first->country=cmp.country;
		first->assoc=cmp.assoc;
		first->name=NULL;
		first->town=*town;
		g_hash_table_insert(search->district_hash, first, first);
	}
	return 0;
}

static void
destination_town_search(gpointer key, gpointer value, gpointer user_data)
{
	struct country *cou=value;
	struct search_param *search=(struct search_param *)user_data;
	town_search_by_name(search->map_data, cou->id, search->town, 1, destination_town_add, search);
	
}

static GHashTable *
destination_street_new(void)
{
	return g_hash_table_new_full(NULL, NULL, NULL, g_free);
}


static int
destination_street_add(struct street_name *name, void *data)
{
	struct search_param *search=data;
	struct street_name *name2;

	name2=g_new(struct street_name, 1);
	*name2=*name;
	g_hash_table_insert(search->street_hash, name2, name2);
	return 0;
}

static int
number_partial(int search, int ref, int ext)
{
	int max=1;

	printf("number_partial(%d,%d,%d)", search, ref, ext);
	if (ref >= 10)
		max=10;
	if (ref >= 100)
		max=100;
	if (ref >= 1000)
		max=1000;
	while (search < max) {
		search*=10;
		search+=ext;
	}
	printf("max=%d result=%d\n", max, search);
	return search;
}

static int
check_number(int low, int high, int s_low, int s_high)
{
	printf("check_number(%d,%d,%d,%d)\n", low, high, s_low, s_high);
	if (low <= s_high && high >= s_low)
		return 1;
	if (s_low == s_high) {
		if (low <= number_partial(s_high, high, 9) && high >= number_partial(s_low, low, 0)) 
			return 1;
	}
	printf("return 0\n");
	return 0;
}

static void
destination_street_show_common(gpointer key, gpointer value, gpointer user_data, int number)
{
	struct street_name *name=value;
	struct search_param *search=(struct search_param *)user_data;
	char *utf8;
	struct dest_town cmp;
	struct dest_town *town;
	int row;
	char buffer[32];
	char *rows[9]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
	struct street_name_info info;
	struct street_name_number_info num_info;

	name->tmp_len=name->aux_len;
	name->tmp_data=name->aux_data;
	while (street_name_get_info(&info, name) && search->count > 0) {
		struct destination *dest;
		cmp.country=info.country;
		cmp.assoc=info.dist;
		cmp.name=NULL;
		town=g_hash_table_lookup(search->district_hash, &cmp);
		printf("town=%p\n", town);
		if (town) {
			destination_town_set(town, rows, 1);
			utf8=g_convert(name->name2,-1,"utf-8","iso8859-1",NULL,NULL,NULL);
			rows[4]=utf8;
			if (number) {
				info.tmp_len=info.aux_len;
				info.tmp_data=info.aux_data;
				while (street_name_get_number_info(&num_info, &info) && search->count > 0) {
					dest=g_new(struct destination, 1);
					dest->town=&town->town;
					dest->street_name=name;
					dest->c=num_info.c;
					if (check_number(num_info.first, num_info.last, search->number_low, search->number_high)) {
						if (num_info.first == num_info.last)
							sprintf(buffer,"%d",num_info.first);
						else
							sprintf(buffer,"%d-%d",num_info.first,num_info.last);
						rows[8]=buffer;
						printf("'%s','%s','%s','%s','%s','%s'\n", rows[0],rows[1],rows[2],rows[3],rows[4],rows[5]);
						row=gtk_clist_append(GTK_CLIST(listbox), rows);
						gtk_clist_set_row_data(GTK_CLIST(listbox), row, dest);
						search->count--;
					}
				}
			} else {
				row=gtk_clist_append(GTK_CLIST(listbox), rows);
				dest=g_new(struct destination, 1);
				dest->town=&town->town;
				dest->street_name=name;
				dest->c=info.c;
				gtk_clist_set_row_data(GTK_CLIST(listbox), row, dest);
				search->count--;
			}
			g_free(utf8);
		} else {
			printf("Town for '%s' not found\n", name->name2);
		}
	}
}

static void
destination_street_show(gpointer key, gpointer value, gpointer user_data)
{
	destination_street_show_common(key, value, user_data, 0);
}

static void
destination_street_show_number(gpointer key, gpointer value, gpointer user_data)
{
	destination_street_show_common(key, value, user_data, 1);
}

static void
destination_street_search(gpointer key, gpointer value, gpointer user_data)
{
	const struct dest_town *town=value;
	struct search_param *search=(struct search_param *)user_data;
	street_name_search(search->map_data, town->country, town->assoc, search->street, 1, destination_street_add, search);
}

#endif


static void changed(GtkWidget *widget, struct search_param *search)
{
	const char *str;
	struct attr attr,result;
	struct item *item,*sitem;
	struct item temp;
	struct mapset_search *srch;
	GtkTreeIter iter;
	int i;

	attr.u.str=gtk_entry_get_text(GTK_ENTRY(widget));
	printf("changed %s\n", attr.u.str);
	if (widget == entry_country) {
		printf("country\n");
		sitem=NULL;
		attr.type=attr_country_all;
	}
	if (widget == entry_city) {
		printf("town\n");
		sitem=&temp;
		sitem->id_lo=0x31;
		attr.type=attr_town_name;
	}

	srch=mapset_search_new(search->ms,sitem,&attr,1);
	gtk_list_store_clear(liststore);
	while(item=mapset_search_get_item(srch)) {
#if 0
		enum attr_type type[]={attr_country_name, attr_country_car, attr_country_iso2, attr_country_iso3};
#endif
		enum attr_type type[]={attr_town_name, attr_district_name};
		gtk_list_store_append(liststore,&iter);
		for(i=0;i<sizeof(type)/sizeof(enum attr_type);i++) {
			if (item_attr_get(item, type[i], &result)) {
				char *str;
				str=g_convert(result.u.str,-1,"utf-8","iso8859-1",NULL,NULL,NULL);
#if 0
				printf("%d %s\n", i, result.u.str);
#endif
				gtk_list_store_set(liststore,&iter,i,str,-1);
				g_free(str);
			}
		}
	}
	mapset_search_destroy(srch);
#if 0
	char *empty[9]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
	char *dash;

	current=widget;
    
	gtk_clist_freeze(GTK_CLIST(listbox));
	gtk_clist_clear(GTK_CLIST(listbox));
	
	selected=-1;

	search->count=row_count;


	if (widget == entry_country) {
		if (search->country_hash) g_hash_table_destroy(search->country_hash);
		search->country_hash=NULL;
	}
	if (widget == entry_country || widget == entry_city) {
		if (search->town_hash) g_hash_table_destroy(search->town_hash);
		if (search->district_hash) g_hash_table_destroy(search->district_hash);
		search->town_hash=NULL;
		search->district_hash=NULL;
	}

	if (widget == entry_country || widget == entry_city || widget == entry_street) {
		if (search->street_hash) g_hash_table_destroy(search->street_hash);
		search->street_hash=NULL;
	}

	if (widget == entry_country) {
		search->country_hash=destination_country_new();
		search->country=str;
		country_search_by_name(str, 1, destination_country_add, search);
		country_search_by_car(str, 1, destination_country_add, search);
		country_search_by_iso2(str, 1, destination_country_add, search);
		country_search_by_iso3(str, 1, destination_country_add, search);
		g_hash_table_foreach(search->country_hash, destination_country_show, search);
	}
	if (widget == entry_city) {
		printf("Ort: '%s'\n", str);
		if (strlen(str) > 1) {
			search->town=str;
			search->town_hash=destination_town_new();
			search->district_hash=destination_town_new();
			g_hash_table_foreach(search->country_hash, destination_town_search, search);
			g_hash_table_foreach(search->town_hash, destination_town_show, search);
		}
	}
	if (widget == entry_street) {
		printf("Street: '%s'\n", str);
		search->street=str;
		search->street_hash=destination_street_new();
		g_hash_table_foreach(search->town_hash, destination_street_search, search);
		g_hash_table_foreach(search->street_hash, destination_street_show, search);
	}
	if (widget == entry_number) {
		char buffer[strlen(str)+1];
		strcpy(buffer, str);
		search->number=str;
		dash=index(buffer,'-'); 
		if (dash) {
			*dash++=0;
			search->number_low=atoi(buffer);
			if (strlen(str)) 
				search->number_high=atoi(dash);
			else
				search->number_high=10000;
		} else {
			if (!strlen(str)) {
				search->number_low=0;
				search->number_high=10000;
			} else {
				search->number_low=atoi(str);
				search->number_high=atoi(str);
			}
		}
		g_hash_table_foreach(search->street_hash, destination_street_show_number, search);
	}
	while (search->count-- > 0) {
		gtk_clist_append(GTK_CLIST(listbox), empty);
	}
        gtk_clist_columns_autosize (GTK_CLIST(listbox));
	gtk_clist_thaw(GTK_CLIST(listbox));
#endif
}

/* borrowed from gpe-login */

#include <fcntl.h>

#define MAX_ARGS 8

parse_xkbd_args (const char *cmd, char **argv)
{
	const char *p = cmd;
	char buf[strlen (cmd) + 1], *bufp = buf;
	int nargs = 0;
	int escape = 0, squote = 0, dquote = 0;

	while (*p)
	{
		if (escape)
		{
			*bufp++ = *p;
			 escape = 0;
		}
		else
		{
			switch (*p)
			{
			case '\\':
				escape = 1;
				break;
			case '"':
				if (squote)
					*bufp++ = *p;
				else
					dquote = !dquote;
				break;
			case '\'':
				if (dquote)
					*bufp++ = *p;
				else
					squote = !squote;
				break;
			case ' ':
				if (!squote && !dquote)
				{
					*bufp = 0;
					if (nargs < MAX_ARGS)
					argv[nargs++] = strdup (buf);
					bufp = buf;
					break;
				}
			default:
				*bufp++ = *p;
				break;
			}
		}
		p++;
	}

	if (bufp != buf)
	{
		*bufp = 0;
		if (nargs < MAX_ARGS)
			argv[nargs++] = strdup (buf);
	}
	argv[nargs] = NULL;
}

int kbd_pid;

static int
spawn_xkbd (char *xkbd_path, char *xkbd_str)
{
	char *xkbd_args[MAX_ARGS + 1];
	int fd[2];
	char buf[256];
	char c;
	int a = 0;
	size_t n;

	pipe (fd);
	kbd_pid = fork ();
	if (kbd_pid == 0)
	{
		close (fd[0]);
		if (dup2 (fd[1], 1) < 0)
			perror ("dup2");
		close (fd[1]);
		if (fcntl (1, F_SETFD, 0))
			perror ("fcntl");
		xkbd_args[0] = (char *)xkbd_path;
		xkbd_args[1] = "-xid";
		if (xkbd_str)
			parse_xkbd_args (xkbd_str, xkbd_args + 2);
		else
			xkbd_args[2] = NULL;
		execvp (xkbd_path, xkbd_args);
		perror (xkbd_path);
		_exit (1);
	}
	close (fd[1]);
	do {
		n = read (fd[0], &c, 1);
		if (n)
		{
			buf[a++] = c;
		}
	} while (n && (c != 10) && (a < (sizeof (buf) - 1)));

	if (a)
	{
		buf[a] = 0;
		return atoi (buf);
	}
	return 0;
}

int destination_address(struct navit *nav)
{

	GtkWidget *window2, *keyboard, *vbox, *table;
	GtkWidget *label_country;
	GtkWidget *label_postal, *label_city, *label_district;
	GtkWidget *label_street, *label_number;
	GtkWidget *hseparator1,*hseparator2;
	GtkWidget *button1,*button2;
	int handlerid;
	int i;
	gchar *text[9]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};


	search_param2.ms=navit_get_mapset(nav);
#if 0
	if (co->cursor) {
		struct coord *c;
		struct route_info *rt;
		struct street_str *st;
		struct block_info *blk;
		struct street_name name;
		struct town town;

		c=cursor_pos_get(co->cursor);
		rt=route_find_nearest_street(co->map_data, c);
		st=route_info_get_street(rt);	
		blk=route_info_get_block(rt);
		printf("segid 0x%lx nameid 0x%lx\n", st->segid, st->nameid);
		street_name_get_by_id(&name, blk->mdata, st->nameid);
		printf("'%s' '%s' %d\n", name.name1, name.name2, name.segment_count);
		for (i = 0 ; i < name.segment_count ; i++) {
			if (name.segments[i].segid == st->segid) {
				printf("found: 0x%x, 0x%x\n", name.segments[i].country, name.segments[i].segid);
				town_get_by_id(&town, co->map_data, name.segments[i].country, name.townassoc);
				printf("%s/%s\n", town.name, town.district);	
			}
		}
	}
#endif

	window2 = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	vbox = gtk_vbox_new(FALSE, 0);
	table = gtk_table_new(3, 8, FALSE);

	entry_country = gtk_entry_new();
	label_country = gtk_label_new("Land");
	entry_postal = gtk_entry_new();
	label_postal = gtk_label_new("PLZ");
	entry_city = gtk_entry_new();
	label_city = gtk_label_new("Ort");
	entry_district = gtk_entry_new();
	label_district = gtk_label_new("Ortsteil/Gemeinde");
	hseparator1 = gtk_vseparator_new();
	entry_street = gtk_entry_new();
	label_street = gtk_label_new("Strasse");
	entry_number = gtk_entry_new();
	label_number = gtk_label_new("Nummer");
 	treeview=gtk_tree_view_new();
#if 0
	listbox = gtk_clist_new(9);
	for (i=0 ; i < row_count ; i++) {
		gtk_clist_append(GTK_CLIST(listbox), text);
	}
	gtk_clist_thaw(GTK_CLIST(listbox));
        gtk_clist_columns_autosize (GTK_CLIST(listbox));
#endif
	listbox = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (listbox),
                        GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);

	gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), NULL);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(listbox),treeview);
#if 1
	for(i=0;i<row_count;i++) {
		GtkCellRenderer *cell=gtk_cell_renderer_text_new();
		gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),-1,NULL, cell,"text",i, NULL);
	}
#endif
	{ 
		GType types[row_count+1];
		for(i=0;i<row_count;i++)
			types[i]=G_TYPE_STRING;
		types[i]=G_TYPE_POINTER;
                liststore=gtk_list_store_newv(row_count+1,types);
                liststore2=gtk_tree_model_sort_new_with_model(liststore);
		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (liststore2), 0, GTK_SORT_ASCENDING);
                gtk_tree_view_set_model (GTK_TREE_VIEW (treeview), GTK_TREE_MODEL(liststore2));
	}




	hseparator2 = gtk_vseparator_new();
	button1 = gtk_button_new_with_label("Karte");
	button2 = gtk_button_new_with_label("Ziel");

	gtk_table_attach(GTK_TABLE(table), label_country,  0, 1,  0, 1,  0, GTK_FILL|GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(table), label_postal,   1, 2,  0, 1,  0, GTK_FILL|GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(table), label_city,     2, 3,  0, 1,  0, GTK_FILL|GTK_EXPAND, 0, 0);

	gtk_table_attach(GTK_TABLE(table), entry_country,  0, 1,  1, 2,  0, GTK_FILL|GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(table), entry_postal,   1, 2,  1, 2,  0, GTK_FILL|GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(table), entry_city,     2, 3,  1, 2,  0, GTK_FILL|GTK_EXPAND, 0, 0);

	gtk_table_attach(GTK_TABLE(table), label_district, 0, 1,  2, 3,  0, GTK_FILL|GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(table), label_street,   1, 2,  2, 3,  0, GTK_FILL|GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(table), label_number,   2, 3,  2, 3,  0, GTK_FILL|GTK_EXPAND, 0, 0);

	gtk_table_attach(GTK_TABLE(table), entry_district, 0, 1,  3, 4,  0, GTK_FILL|GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(table), entry_street,   1, 2,  3, 4,  0, GTK_FILL|GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(table), entry_number,   2, 3,  3, 4,  0, GTK_FILL|GTK_EXPAND, 0, 0);

	gtk_table_attach(GTK_TABLE(table), listbox,        0, 3,  4, 5,  GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);

	gtk_table_attach(GTK_TABLE(table), button1, 0, 1, 5, 6, GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
	gtk_table_attach(GTK_TABLE(table), button2, 2, 3, 5, 6, GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);

	g_signal_connect(G_OBJECT(entry_country), "changed", G_CALLBACK(changed), &search_param2);
	g_signal_connect(G_OBJECT(entry_postal), "changed", G_CALLBACK(changed), &search_param2);
	g_signal_connect(G_OBJECT(entry_city), "changed", G_CALLBACK(changed), &search_param2);
	g_signal_connect(G_OBJECT(entry_district), "changed", G_CALLBACK(changed), &search_param2);
	g_signal_connect(G_OBJECT(entry_street), "changed", G_CALLBACK(changed), &search_param2);
	g_signal_connect(G_OBJECT(entry_number), "changed", G_CALLBACK(changed), &search_param2);
#if 0
	g_signal_connect(G_OBJECT(button1), "clicked", G_CALLBACK(button_map), co);
	g_signal_connect(G_OBJECT(button2), "clicked", G_CALLBACK(button_destination), co);
#endif

	gtk_widget_grab_focus(entry_city);

	gtk_container_add(GTK_CONTAINER(vbox), table);
	keyboard=gtk_socket_new();
	gtk_container_add(GTK_CONTAINER(vbox), keyboard);
	gtk_container_add(GTK_CONTAINER(window2), vbox);
#if 0
	handlerid = gtk_timeout_add(256, (GtkFunction) track_focus, NULL);
#endif

#if 0
	g_signal_connect(G_OBJECT(listbox), "select-row", G_CALLBACK(select_row), NULL);
#endif
	
	gtk_widget_show_all(window2);
	gtk_socket_steal(keyboard, spawn_xkbd("xkbd","-geometry 200x100"));

	return 0;
}


int destination_address_tst(struct navit *nav)
{
	struct search *search1,*search2,*search3;
	struct mapset *ms;
	struct map *map;
	struct item *item;
	struct attr attr1,attr2,attr3;
	struct attr result,resulti;
	int i;

#if 1
	return;
#else
	ms=navit_get_mapset(nav);

#if 0
	debug_level_set("data_mg:tree_search_next", 1);
	debug_level_set("data_mg:town_search_compare", 1);
	debug_level_set("data_mg:tree_search_enter", 1);
#endif
	attr1.type=attr_country_all;
	attr1.u.str="D";
	attr2.type=attr_town_name;
	attr2.u.str="stuttga";
	attr3.type=attr_street_name;
	attr3.u.str="wall";
	search1=mapset_search_new(ms,NULL,&attr1,0);
	printf("searching country\n");
	while(item=mapset_search_get_item(search1)) {
		if (item_attr_get(item, attr_country_name, &result)) {
			printf("country %s 0x%x\n", result.u.str, item->id_lo);
		}
		printf("searching town\n");
		search2=mapset_search_new(ms,item,&attr2,1);
		i=0;
		while((item=mapset_search_get_item(search2))) {
			if (!item_attr_get(item, attr_district_name, &result)) {
				if (item_attr_get(item, attr_town_name, &result)) {
					printf("town %s\n", result.u.str);
					printf("searching street\n");
					if (item_attr_get(item, attr_town_streets_item, &resulti))
						search3=mapset_search_new(ms,resulti.u.item,&attr3,1);
					else
						search3=mapset_search_new(ms,item,&attr3,1);
#if 1
					while((item=mapset_search_get_item(search3))) {
						printf("item\n");
					}
					mapset_search_destroy(search3);
#endif
				}
			}
			i++;
		}
		mapset_search_destroy(search2);
		break;
	}
	mapset_search_destroy(search1);
	exit(0);
#endif
}
