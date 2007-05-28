#include <stdio.h>
#include <glib.h>
#include "popup.h"
#include "navit.h"
#include "coord.h"
#include "gui.h"
#include "menu.h"
#include "point.h"
#include "transform.h"
#include "projection.h"

#if 0
static void
popup_item_destroy_text(struct popup_item *item)
{
	g_free(item->text);
	g_free(item);
}

struct popup_item *
popup_item_new_text(struct popup_item **last, char *text, int priority)
{
	struct popup_item *curr;
	curr=g_new0(struct popup_item,1);
	curr->text=g_strdup(text);
	curr->priority=priority;
	curr->destroy=popup_item_destroy_text;
	if (last) {
		curr->next=*last;
		*last=curr;
	}
	return curr;
}

struct popup_item *
popup_item_new_func(struct popup_item **last, char *text, int priority, void (*func)(struct popup_item *, void *), void *param)
{
	struct popup_item *curr=popup_item_new_text(last, text, priority);
	curr->func=func;
	curr->param=param;
	return curr;
}

static struct popup_item *
param_to_menu_new(char *name,struct param_list *plist, int c, int iso)
{
	struct popup_item *last, *curr, *ret;
	int i;

	ret=popup_item_new_text(NULL,name,1);
	last=NULL;
	for (i = 0 ; i < c ; i++) {
		char *name_buffer = g_strjoin(":",plist[i].name, plist[i].value,NULL);
		char *text = name_buffer;

		if (iso) {
			text=g_convert(name_buffer,-1,"utf-8","iso8859-1",NULL,NULL,NULL);
			if (! text) {
				printf("problem converting '%s'\n", name_buffer);
			}
		}
		curr=popup_item_new_text(&last,text,i);
		if (iso)
			g_free(text);
		g_free(name_buffer);
									
	}
	ret->submenu=last;
	return ret;
}

static void
popup_set_no_passing(struct popup_item *item, void *param)
{
#if 0
	struct display_list *l=param;
	struct segment *seg=(struct segment *)(l->data);
	struct street_str *str=(struct street_str *)(seg->data[0]);
	char log[256];
	int segid=str->segid;
	if (segid < 0)
		segid=-segid;

	sprintf(log,"Attributes Street 0x%x updated: limit=0x%x(0x%x)", segid, 0x33, str->limit);
	str->limit=0x33;
	log_write(log, seg->blk_inf.file, str, sizeof(*str));
#endif
}

static void
popup_set_destination(struct popup_item *item, void *param)
{
	struct popup_item *ref=param;
	struct popup *popup=ref->param;
	struct container *co=popup->co;
	printf("Destination %s\n", ref->text);
#if 0 /* FIXME */
	route_set_position(co->route, cursor_pos_get(co->cursor));
	route_set_destination(co->route, &popup->c);
#endif
	graphics_redraw(popup->co);
	if (co->statusbar && co->statusbar->statusbar_route_update)
		co->statusbar->statusbar_route_update(co->statusbar, co->route);
}

extern void *vehicle;

static void
popup_set_position(struct popup_item *item, void *param)
{
	struct popup_item *ref=param;
	struct popup *popup=ref->param;
	printf("Position %s\n", ref->text);
	g_assert(popup->co->vehicle != NULL);
	vehicle_set_position(popup->co->vehicle, &popup->c);	
}

#if 0
static void
popup_break_crossing(struct display_list *l)
{
	struct segment *seg=(struct segment *)(l->data);
	struct street_str *str=(struct street_str *)(seg->data[0]);
	char log[256];
	int segid=str->segid;
	if (segid < 0)
		segid=-segid;

	sprintf(log,"Coordinates Street 0x%x updated: limit=0x%x(0x%x)", segid, 0x33, str->limit);
	str->limit=0x33;
	log_write(log, seg->blk_inf.file, str, sizeof(*str));
}
#endif

static void
popup_call_func(GtkObject *obj, void *parm)
{
	struct popup_item *curr=parm;
	curr->func(curr, curr->param);
}

static GtkWidget *
popup_menu(struct popup_item *list)
{
	int min_prio,curr_prio;
	struct popup_item *curr;
	GtkWidget *item,*menu,*submenu;

	curr_prio=0;
	menu=gtk_menu_new();
	do {
		min_prio=INT_MAX;
		curr=list;
		while (curr) {
			if (curr->priority == curr_prio) {
				item=gtk_menu_item_new_with_label(curr->text);
				gtk_menu_append(GTK_MENU(menu), item);
				if (curr->submenu) {
					submenu=popup_menu(curr->submenu);
					gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), submenu);
				} else if (curr->func) {
					g_signal_connect(G_OBJECT(item), "activate",
					 	G_CALLBACK (popup_call_func), curr);
				}
			}
			if (curr->priority > curr_prio && curr->priority < min_prio)
				min_prio=curr->priority;
			curr=curr->next;
		}
		curr_prio=min_prio;
	} while (min_prio != INT_MAX);
	return menu;
}

static void
popup_display_list_default(struct display_list *d, struct popup_item **popup_list)
{
#if 0
	struct segment *seg;
	char *desc,*text,*item_text;
	struct popup_item *curr_item,*submenu;
	struct param_list plist[100];    

	desc=NULL;
	if (d->type == 0) desc="Polygon";	
	if (d->type == 1) desc="Polyline";
	if (d->type == 2) desc="Street";
	if (d->type == 3) desc="Label";
	if (d->type == 4) desc="Point";
	seg=(struct segment *)(d->data);
	if (seg) {
		if (d->label && strlen(d->label))
			item_text=g_strjoin(" ",desc,d->label,NULL);
		else
			item_text=g_strdup(desc);
		text=g_convert(item_text,-1,"utf-8","iso8859-1",NULL,NULL,NULL);
		curr_item=popup_item_new_text(popup_list,text,1);
		g_free(text);
		g_free(item_text);

		curr_item->submenu=param_to_menu_new("File", plist, file_get_param(seg->blk_inf.file, plist, 100), 1);
		submenu=curr_item->submenu;
		submenu->next=param_to_menu_new("Block", plist, block_get_param(&seg->blk_inf, plist, 100), 1);
		submenu=submenu->next;
		
		if (d->type == 0 || d->type == 1) {
			submenu->next=param_to_menu_new(desc, plist, poly_get_param(seg, plist, 100), 1);
		}
		if (d->type == 2) {
			submenu->next=param_to_menu_new(desc, plist, street_get_param(seg, plist, 100, 1), 1);
			popup_item_new_func(&submenu->next,"Set no passing", 1000, popup_set_no_passing, d);
		}
		if (d->type == 3) {
			submenu->next=param_to_menu_new(desc, plist, town_get_param(seg, plist, 100), 1);
		}
		if (d->type == 4) {
			submenu->next=param_to_menu_new(desc, plist, street_bti_get_param(seg, plist, 100), 1);
		}
	}
#endif
}

static void
popup_display_list(struct container *co, struct popup *popup, struct popup_item **popup_list)
{
	GtkWidget *menu, *item;
	struct display_list *list[100],**p=list;

	menu=gtk_menu_new();
	item=gtk_menu_item_new_with_label("Selection");
	gtk_menu_append (GTK_MENU(menu), item);	
	display_find(&popup->pnt, co->disp, display_end, 3, list, 100);
	while (*p) {
		if (! (*p)->info)
			popup_display_list_default(*p, popup_list);
		else
			(*(*p)->info)(*p, popup_list);
		p++;
	}	
}

static void
popup_destroy_items(struct popup_item *item)
{
	struct popup_item *next;
	while (item) {
		if (item->active && item->func)
			item->func(item, item->param);
		if (item->submenu)
			popup_destroy_items(item->submenu);
		next=item->next;
		assert(item->destroy != NULL);
		item->destroy(item);
		item=next;
	}
}

static void
popup_destroy(GtkObject *obj, void *parm)
{
	struct popup *popup=parm;

	popup_destroy_items(popup->items);
	g_free(popup);	
}
#endif

#include "item.h"
struct display_item {
        struct item item;
        char *label;
        int displayed;
        int count;
        struct point pnt[0];
};

struct point popup_pnt;

static void
popup_show_item(void *popup, struct display_item *di)
{
	char *str;
	if (di->label) 
		str=g_strdup_printf("%s '%s'", item_to_name(di->item.type), di->label);
	else
		str=g_strdup(item_to_name(di->item.type));
	menu_add(popup, str, menu_type_menu, NULL, NULL, NULL);
	g_free(str);
}

static int
within_dist_point(struct point *p0, struct point *p1, int dist)
{
	if (p0->x == 32767 || p0->y == 32767 || p1->x == 32767 || p1->y == 32767)
		return 0;
	if (p0->x == -32768 || p0->y == -32768 || p1->x == -32768 || p1->y == -32768)
		return 0;
        if ((p0->x-p1->x)*(p0->x-p1->x) + (p0->y-p1->y)*(p0->y-p1->y) <= dist*dist) {
                return 1;
        }
        return 0;
}

static int
within_dist_line(struct point *p, struct point *line_p0, struct point *line_p1, int dist)
{
	int vx,vy,wx,wy;
	int c1,c2;
	struct point line_p;

	vx=line_p1->x-line_p0->x;
	vy=line_p1->y-line_p0->y;
	wx=p->x-line_p0->x;
	wy=p->y-line_p0->y;

	c1=vx*wx+vy*wy;
	if ( c1 <= 0 )
		return within_dist_point(p, line_p0, dist);
	c2=vx*vx+vy*vy;
	if ( c2 <= c1 )
		return within_dist_point(p, line_p1, dist);

	line_p.x=line_p0->x+vx*c1/c2;
	line_p.y=line_p0->y+vy*c1/c2;
	return within_dist_point(p, &line_p, dist);
}

static int
within_dist_polyline(struct point *p, struct point *line_pnt, int count, int dist, int close)
{
	int i;
	for (i = 0 ; i < count-1 ; i++) {
		if (within_dist_line(p,line_pnt+i,line_pnt+i+1,dist)) {
			return 1;
		}
	}
#if 0
	if (close)
		return (within_dist_line(p,line_pnt,line_pnt+count-1,dist));
#endif
	return 0;
}

static int
within_dist(struct display_item *di, struct point *p, int dist)
{
	if (di->item.type < type_line) {
		return within_dist_point(p, &di->pnt[0], dist);
	}
	if (di->item.type < type_area) {
		return within_dist_polyline(p, di->pnt, di->count, dist, 0);
	}
	return 0;
}

static void
popup_display_list(gpointer data, gpointer user_data)
{
	struct display_item *di=data;
	if (within_dist(di, &popup_pnt, 5)) {
		popup_show_item(user_data, di);
	}
}

static void
popup_display_hash(gpointer key, gpointer value, gpointer user_data)
{
	g_list_foreach(value, popup_display_list, user_data);
}

static void
popup_display(struct navit *nav, void *popup, struct point *p)
{
	GHashTable *display;
	display=navit_get_display_list(nav);
        g_hash_table_foreach(display, popup_display_hash, popup);
}

void
popup(struct navit *nav, int button, struct point *p)
{
	void *popup,*men;
	char *str;
	char buffer[1024];
	struct coord c;
	struct coord_geo g;

	popup_pnt=*p;
	popup=gui_popup_new(navit_get_gui(nav), nav);
	transform_reverse(navit_get_trans(nav), p, &c);
	str=g_strdup_printf("Point 0x%x 0x%x", c.x, c.y);
	men=menu_add(popup, str, menu_type_submenu, NULL, NULL, NULL);
	g_free(str);
	str=g_strdup_printf("Screen %d %d", p->x, p->y);
	menu_add(men, str, menu_type_menu, NULL, NULL, NULL);
	g_free(str);
	transform_to_geo(transform_get_projection(navit_get_trans(nav)), &c, &g);
	transform_geo_text(&g, buffer);	
	menu_add(men, buffer, menu_type_menu, NULL, NULL, NULL);
	sprintf(buffer, "%f %f", g.lat, g.lng);
	menu_add(men, buffer, menu_type_menu, NULL, NULL, NULL);
	popup_display(nav, popup, p);
}
