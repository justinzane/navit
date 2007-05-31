#include <stdio.h>
#include <glib.h>
#include <stdarg.h>
#include "popup.h"
#include "navit.h"
#include "coord.h"
#include "gui.h"
#include "menu.h"
#include "point.h"
#include "transform.h"
#include "projection.h"
#include "map.h"
#include "graphics.h"

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

struct point popup_pnt;

static void *
popup_printf(void *menu, enum menu_type type, const char *fmt, ...)
{
	gchar *str;
	va_list ap;
	void *ret;

	va_start(ap, fmt);
	str=g_strdup_vprintf(fmt, ap);
	ret=menu_add(menu, str, type, NULL, NULL, NULL);
	va_end(ap);
	g_free(str);
	return ret;
}

static void
popup_show_attr_val(void *menu, struct attr *attr)
{
	char *attr_name=attr_to_name(attr->type);
	
	printf("%s: %s", attr_name, attr->u.str);
	popup_printf(menu, menu_type_menu, "%s: %s", attr_name, attr->u.str);
}

static void
popup_show_attr(void *menu, struct item *item, enum attr_type attr_type)
{
	struct attr attr;
	memset(&attr, 0, sizeof(attr));
	attr.type=attr_type;
	if (item_attr_get(item, attr_type, &attr)) 
		popup_show_attr_val(menu, &attr);
}

static void
popup_show_attrs(void *menu, struct item *item)
{
#if 0
	popup_show_attr(menu, item, attr_debug);
	popup_show_attr(menu, item, attr_address);
	popup_show_attr(menu, item, attr_phone);
	popup_show_attr(menu, item, attr_phone);
	popup_show_attr(menu, item, attr_entry_fee);
	popup_show_attr(menu, item, attr_open_hours);
#else
	struct attr attr;
	for (;;) {
		memset(&attr, 0, sizeof(attr));
		if (item_attr_get(item, attr_any, &attr)) 
			popup_show_attr_val(menu, &attr);
		else
			break;
	}
	
#endif
}

static void
popup_show_item(void *popup, struct displayitem *di)
{
	struct map_rect *mr;
	void *menu, *menu_map, *menu_item;
	char *label;
	struct item *item;

	label=graphics_displayitem_get_label(di);
	item=graphics_displayitem_get_item(di);

	if (label) 
		menu=popup_printf(popup, menu_type_submenu, "%s '%s'", item_to_name(item->type), label);
	else
		menu=popup_printf(popup, menu_type_submenu, "%s", item_to_name(item->type));
	menu_item=popup_printf(menu, menu_type_submenu, "Item");
	popup_printf(menu_item, menu_type_menu, "type: 0x%x", item->type);
	popup_printf(menu_item, menu_type_menu, "id: 0x%x 0x%x", item->id_hi, item->id_lo);
	mr=map_rect_new(item->map,NULL,NULL,0);
	item=map_rect_get_item_byid(mr, item->id_hi, item->id_lo);
	if (item) {
		popup_show_attrs(menu_item, item);
	}
	map_rect_destroy(mr);
	menu_map=popup_printf(menu, menu_type_submenu, "Map");
}

static void
popup_display(struct navit *nav, void *popup, struct point *p)
{
	struct displaylist_handle *dlh;
	GHashTable *display;
	struct displayitem *di;

	display=navit_get_display_list(nav);
	dlh=graphics_displaylist_open(display);
	while ((di=graphics_displaylist_next(dlh))) {
		if (graphics_displayitem_within_dist(di, &popup_pnt, 5)) {
			popup_show_item(popup, di);
		}
	}
	graphics_displaylist_close(dlh);
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
