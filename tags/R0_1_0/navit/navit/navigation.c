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
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include "debug.h"
#include "profile.h"
#include "navigation.h"
#include "coord.h"
#include "item.h"
#include "route.h"
#include "transform.h"
#include "mapset.h"
#include "projection.h"
#include "map.h"
#include "navit.h"
#include "callback.h"
#include "plugin.h"
#include "navit_nls.h"

struct suffix {
	char *fullname;
	char *abbrev;
	int sex;
} suffixes[]= {
	{"weg",NULL,1},
	{"platz","pl.",1},
	{"ring",NULL,1},
	{"allee",NULL,2},
	{"gasse",NULL,2},
	{"straße","str.",2},
	{"strasse",NULL,2},
};

struct navigation {
	struct map *map;
	struct item_hash *hash;
	struct navigation_itm *first;
	struct navigation_itm *last;
	struct navigation_command *cmd_first;
	struct navigation_command *cmd_last;
	struct callback_list *callback_speech;
	struct callback_list *callback;
	int level_last;
	struct item item_last;
	int turn_around;
	int turn_around_limit;
	int distance_turn;
	int distance_last;
	int announce[route_item_last-route_item_first+1][3];
};


struct navigation_command {
	struct navigation_itm *itm;
	struct navigation_command *next;
	int delta;
};

struct navigation *
navigation_new(struct attr **attrs)
{
	int i,j;
	struct navigation *ret=g_new0(struct navigation, 1);
	ret->hash=item_hash_new();
	ret->callback=callback_list_new();
	ret->callback_speech=callback_list_new();
	ret->level_last=-2;
	ret->distance_last=-2;
	ret->distance_turn=50;
	ret->turn_around_limit=3;

	for (j = 0 ; j <= route_item_last-route_item_first ; j++) {
		for (i = 0 ; i < 3 ; i++) {
			ret->announce[j][i]=-1;
		}
	}

	return ret;	
}

int
navigation_set_announce(struct navigation *this_, enum item_type type, int *level)
{
	int i;
	if (type < route_item_first || type > route_item_last) {
		dbg(0,"street type %d out of range [%d,%d]", type, route_item_first, route_item_last);
		return 0;
	}
	for (i = 0 ; i < 3 ; i++) 
		this_->announce[type-route_item_first][i]=level[i];
	return 1;
}

static int
navigation_get_announce_level(struct navigation *this_, enum item_type type, int dist)
{
	int i;

	if (type < route_item_first || type > route_item_last)
		return -1;
	for (i = 0 ; i < 3 ; i++) {
		if (dist <= this_->announce[type-route_item_first][i])
			return i;
	}
	return i;
}

struct navigation_itm {
	char *name1;
	char *name2;
	struct item item;
	int direction;
	int straight;
	int angle_start;
	int angle_end;
	struct coord start,end;
	int time;
	int length;
	int dest_time;
	int dest_length;
	int told;
	int dest_count;
	struct navigation_itm *next;
	struct navigation_itm *prev;
};

/* 0=N,90=E */
static int
road_angle(struct coord *c1, struct coord *c2, int dir)
{
	int ret=transform_get_angle_delta(c1, c2, dir);
	dbg(1, "road_angle(0x%x,0x%x - 0x%x,0x%x)=%d\n", c1->x, c1->y, c2->x, c2->y, ret);
	return ret;
}

static int
round_distance(int dist)
{
	if (dist < 100) {
		dist=(dist+5)/10;
		return dist*10;
	}
	if (dist < 250) {
		dist=(dist+13)/25;
		return dist*25;
	}
	if (dist < 500) {
		dist=(dist+25)/50;
		return dist*50;
	}
	if (dist < 1000) {
		dist=(dist+50)/100;
		return dist*100;
	}
	if (dist < 5000) {
		dist=(dist+50)/100;
		return dist*100;
	}
	if (dist < 100000) {
		dist=(dist+500)/1000;
		return dist*1000;
	}
	dist=(dist+5000)/10000;
	return dist*10000;
}

static char *
get_distance(int dist, enum attr_type type, int is_length)
{
	if (type == attr_navigation_long) {
		if (is_length)
			return g_strdup_printf(_("%d m"), dist);
		else
			return g_strdup_printf(_("in %d m"), dist);
	}
	if (dist < 1000) {
		if (is_length)
			return g_strdup_printf(_("%d meters"), dist);
		else
			return g_strdup_printf(_("in %d meters"), dist);
	}
	if (dist < 5000) {
		int rem=(dist/100)%10;
		if (rem) {
			if (is_length)
				return g_strdup_printf(_("%d.%d kilometer"), dist/1000, rem);
			else
				return g_strdup_printf(_("in %d.%d kilometers"), dist/1000, rem);
		}
	}
	if (is_length) 
		return g_strdup_printf(ngettext("one kilometer","%d kilometers", dist/1000), dist/1000);
	else
		return g_strdup_printf(ngettext("in one kilometer","in %d kilometers", dist/1000), dist/1000);
}

static void
navigation_destroy_itms_cmds(struct navigation *this_, struct navigation_itm *end)
{
	struct navigation_itm *itm;
	struct navigation_command *cmd;
	dbg(2,"enter this_=%p this_->first=%p this_->cmd_first=%p end=%p\n", this_, this_->first, this_->cmd_first, end);
	if (this_->cmd_first)
		dbg(2,"this_->cmd_first->itm=%p\n", this_->cmd_first->itm);
	while (this_->first && this_->first != end) {
		itm=this_->first;
		dbg(3,"destroying %p\n", itm);
		item_hash_remove(this_->hash, &itm->item);
		this_->first=itm->next;
		if (this_->first)
			this_->first->prev=NULL;
		if (this_->cmd_first && this_->cmd_first->itm == itm->next) {
			cmd=this_->cmd_first;
			this_->cmd_first=cmd->next;
			g_free(cmd);
		}
		map_convert_free(itm->name1);
		map_convert_free(itm->name2);
		g_free(itm);
	}
	if (! this_->first)
		this_->last=NULL;
	if (! this_->first && end) 
		dbg(0,"end wrong\n");
	dbg(2,"ret this_->first=%p this_->cmd_first=%p\n",this_->first, this_->cmd_first);
}

static void
navigation_itm_update(struct navigation_itm *itm, struct item *ritem)
{
	struct attr length, time, straight;
	if (! item_attr_get(ritem, attr_length, &length)) {
		dbg(0,"no length\n");
		return;
	}
	if (! item_attr_get(ritem, attr_time, &time)) {
		dbg(0,"no time\n");
		return;
	}

	if (item_attr_get(ritem, attr_route_follow_straight, &straight)) {
		itm->straight = straight.u.num;
	} else {
		itm->straight = 1;
	}

	dbg(1,"length=%d time=%d\n", length.u.num, time.u.num);
	itm->length=length.u.num;
	itm->time=time.u.num;
}

static struct navigation_itm *
navigation_itm_new(struct navigation *this_, struct item *ritem)
{
	struct navigation_itm *ret=g_new0(struct navigation_itm, 1);
	int i=0;
	struct item *sitem;
	struct attr street_item,direction;
	struct map_rect *mr;
	struct attr attr;
	struct coord c[5];

	if (ritem) {
		ret->told=0;
		if (! item_attr_get(ritem, attr_street_item, &street_item)) {
			dbg(0,"no street item\n");
			return NULL;
		}
		if (item_attr_get(ritem, attr_direction, &direction))
			ret->direction=direction.u.num;
		else
			ret->direction=0;
		sitem=street_item.u.item;
		ret->item=*sitem;
		item_hash_insert(this_->hash, sitem, ret);
		mr=map_rect_new(sitem->map, NULL);
		sitem=map_rect_get_item_byid(mr, sitem->id_hi, sitem->id_lo);
		if (item_attr_get(sitem, attr_street_name, &attr))
			ret->name1=map_convert_string(sitem->map,attr.u.str);
		if (item_attr_get(sitem, attr_street_name_systematic, &attr))
			ret->name2=map_convert_string(sitem->map,attr.u.str);
		navigation_itm_update(ret, ritem);

		while (item_coord_get(ritem, &c[i], 1)) {
			dbg(1, "coord %d 0x%x 0x%x\n", i, c[i].x ,c[i].y);

			if (i < 4) 
				i++;
			else {
				c[2]=c[3];
				c[3]=c[4];
			}
		}
		dbg(1,"count=%d\n", i);
		i--;

		ret->angle_start=road_angle(&c[0], &c[1], 0);
		ret->angle_end=road_angle(&c[i-1], &c[i], 0);

		ret->start=c[0];
		ret->end=c[i];
		dbg(1,"i=%d start %d end %d '%s' '%s'\n", i, ret->angle_start, ret->angle_end, ret->name1, ret->name2);
		map_rect_destroy(mr);
	} else {
		if (this_->last)
			ret->start=ret->end=this_->last->end;
	}
	if (! this_->first)
		this_->first=ret;
	if (this_->last) {
		this_->last->next=ret;
		ret->prev=this_->last;
	}
	dbg(1,"ret=%p\n", ret);
	this_->last=ret;
	return ret;
}

/**
 * @brief Calculates distance and time to the destination
 *
 * This function calculates the distance and the time to the destination of a
 * navigation. If incr is set, this is only calculated for the first navigation
 * item, which is a lot faster than re-calculation the whole destination, but works
 * only if the rest of the navigation already has been calculated.
 *
 * @param this_ The navigation whose destination / time should be calculated
 * @param incr Set this to true to only calculate the first item. See description.
 */
static void
calculate_dest_distance(struct navigation *this_, int incr)
{
	int len=0, time=0, count=0;
	struct navigation_itm *next,*itm=this_->last;
	dbg(1, "enter this_=%p, incr=%d\n", this_, incr);
	if (incr) {
		if (itm)
			dbg(2, "old values: (%p) time=%d lenght=%d\n", itm, itm->dest_length, itm->dest_time);
		else
			dbg(2, "old values: itm is null\n");
		itm=this_->first;
		next=itm->next;
		dbg(2, "itm values: time=%d lenght=%d\n", itm->length, itm->time);
		dbg(2, "next values: (%p) time=%d lenght=%d\n", next, next->dest_length, next->dest_time);
		itm->dest_length=next->dest_length+itm->length;
		itm->dest_count=next->dest_count+1;
		itm->dest_time=next->dest_time+itm->time;
		dbg(2, "new values: time=%d lenght=%d\n", itm->dest_length, itm->dest_time);
		return;
	}
	while (itm) {
		len+=itm->length;
		time+=itm->time;
		itm->dest_length=len;
		itm->dest_time=time;
		itm->dest_count=count++;
		itm=itm->prev;
	}
	dbg(1,"len %d time %d\n", len, time);
}

/**
 * @brief Checks if two navigation items are on the same street
 *
 * This function checks if two navigation items are on the same street. It returns
 * true if either their name or their "systematic name" (e.g. "A6" or "B256") are the
 * same.
 *
 * @param old The first item to be checked
 * @param new The second item to be checked
 * @return True if both old and new are on the same street
 */
static int
is_same_street2(struct navigation_itm *old, struct navigation_itm *new)
{
	if (old->name1 && new->name1 && !strcmp(old->name1, new->name1)) {
		dbg(1,"is_same_street: '%s' '%s' vs '%s' '%s' yes (1.)\n", old->name2, new->name2, old->name1, new->name1);
		return 1;
	}
	if (old->name2 && new->name2 && !strcmp(old->name2, new->name2)) {
		dbg(1,"is_same_street: '%s' '%s' vs '%s' '%s' yes (2.)\n", old->name2, new->name2, old->name1, new->name1);
		return 1;
	}
	dbg(1,"is_same_street: '%s' '%s' vs '%s' '%s' no\n", old->name2, new->name2, old->name1, new->name1);
	return 0;
}

/**
 * @brief Checks if two navigation items are on the same street
 *
 * This function checks if two navigation items are on the same street. It returns
 * true if the first part of their "systematic name" is equal. If the "systematic name" is
 * for example "A352/E3" (a german highway which at the same time is part of the international
 * E-road network), it would only search for "A352" in the second item's systematic name.
 *
 * @param old The first item to be checked
 * @param new The second item to be checked
 * @return True if the "systematic name" of both items matches. See description.
 */
static int
is_same_street_systematic(struct navigation_itm *old, struct navigation_itm *new)
{
	int slashold,slashnew;
	if (!old->name2 || !new->name2)
		return 1;
	slashold=strcspn(old->name2, "/");
	slashnew=strcspn(new->name2, "/");
	if (slashold != slashnew || strncmp(old->name2, new->name2, slashold))
		return 0;
	return 1;
}

/**
 * @brief Checks if navit has to create a maneuver to drive from old to new
 *
 * This function checks if it has to create a "maneuver" - i.e. guide the user - to drive 
 * from "old" to "new".
 *
 * @param old The old navigation item, where we're coming from
 * @param new The new navigation item, where we're going to
 * @param delta The angle the user has to steer to navigate from old to new
 * @return True if navit should guide the user, false otherwise
 */
static int
maneuver_required2(struct navigation_itm *old, struct navigation_itm *new, int *delta)
{
	dbg(1,"enter %p %p %p\n",old, new, delta);
	if (new->item.type == old->item.type || (new->item.type != type_ramp && old->item.type != type_ramp)) {
		if (is_same_street2(old, new)) {
			return 0;
		}
	} else
		dbg(1, "maneuver_required: old or new is ramp\n");
	if (old->item.type == type_ramp && (new->item.type == type_highway_city || new->item.type == type_highway_land)) {
		dbg(1, "no_maneuver_required: old is ramp new is highway\n");
		return 0;
	}
#if 0
	if (old->crossings_end == 2) {
		dbg(1, "maneuver_required: only 2 connections: no\n");
		return 0;
	}
#endif
	*delta=new->angle_start-old->angle_end;
	if (*delta < -180)
		*delta+=360;
	if (*delta > 180)
		*delta-=360;
	dbg(1,"delta=%d-%d=%d\n", new->angle_start, old->angle_end, *delta);
	if ((new->item.type == type_highway_land || new->item.type == type_highway_city || old->item.type == type_highway_land || old->item.type == type_highway_city) && (!is_same_street_systematic(old, new) || (old->name2 != NULL && new->name2 == NULL))) {
		dbg(1, "maneuver_required: highway changed name\n");
		return 1;
	}
	if (*delta < 20 && *delta >-20) {
		if (! new->straight) { /* We're not entering this item straight, so have a maneuver */
			return 1;
		}

		dbg(1, "maneuver_required: delta(%d) < 20: no\n", *delta);		
		return 0;
	}
	dbg(1, "maneuver_required: delta=%d: yes\n", *delta);
	return 1;
}

static struct navigation_command *
command_new(struct navigation *this_, struct navigation_itm *itm, int delta)
{
	struct navigation_command *ret=g_new0(struct navigation_command, 1);
	dbg(1,"enter this_=%p itm=%p delta=%d\n", this_, itm, delta);
	ret->delta=delta;
	ret->itm=itm;
	if (this_->cmd_last)
		this_->cmd_last->next=ret;
	this_->cmd_last=ret;

	if (!this_->cmd_first)
		this_->cmd_first=ret;
	return ret;
}

static void
make_maneuvers(struct navigation *this_)
{
	struct navigation_itm *itm, *last=NULL, *last_itm=NULL;
	int delta;
	itm=this_->first;
	this_->cmd_last=NULL;
	this_->cmd_first=NULL;
	while (itm) {
		if (last) {
			if (maneuver_required2(last_itm, itm, &delta)) {
				command_new(this_, itm, delta);
			}
		} else
			last=itm;
		last_itm=itm;
		itm=itm->next;
	}
}

static int
contains_suffix(char *name, char *suffix)
{
	if (!suffix)
		return 0;
	if (strlen(name) < strlen(suffix))
		return 0;
	return !strcasecmp(name+strlen(name)-strlen(suffix), suffix);
}

static char *
replace_suffix(char *name, char *search, char *replace)
{
	int len=strlen(name)-strlen(search);
	char *ret=g_malloc(len+strlen(replace)+1);
	strncpy(ret, name, len);
	strcpy(ret+len, replace);

	return ret;
}

static char *
navigation_item_destination(struct navigation_itm *itm, struct navigation_itm *next, char *prefix)
{
	char *ret=NULL,*name1,*sep,*name2;
	int i,sex;
	if (! prefix)
		prefix="";
	if(!itm->name1 && !itm->name2 && itm->item.type == type_ramp) {
		dbg(1,">> Next is ramp %lx current is %lx \n", itm->item.type, next->item.type);
			 
		if(next->item.type == type_ramp)
			return NULL;
		if(itm->item.type == type_highway_city || itm->item.type == type_highway_land )
			return g_strdup_printf("%s%s",prefix,_("exit"));	/* %FIXME Can this even be reached? */			 
		else
			return g_strdup_printf("%s%s",prefix,_("ramp"));
		
	}
	if (!itm->name1 && !itm->name2)
		return NULL;
	if (itm->name1) {
		sex=-1;
		name1=NULL;
		for (i = 0 ; i < sizeof(suffixes)/sizeof(suffixes[0]) ; i++) {
			if (contains_suffix(itm->name1,suffixes[i].fullname)) {
				sex=suffixes[i].sex;
				name1=g_strdup(itm->name1);
				break;
			}
			if (contains_suffix(itm->name1,suffixes[i].abbrev)) {
				sex=suffixes[i].sex;
				name1=replace_suffix(itm->name1, suffixes[i].abbrev, suffixes[i].fullname);
				break;
			}
		}
		if (itm->name2) {
			name2=itm->name2;
			sep=" ";
		} else {
			name2="";
			sep="";
		}
		switch (sex) {
		case -1:
			/* TRANSLATORS: Arguments: 1: Prefix (Space if required) 2: Street Name 3: Separator (Space if required), 4: Systematic Street Name */
			ret=g_strdup_printf(_("%sinto the street %s%s%s"),prefix,itm->name1, sep, name2);
			break;
		case 1:
			/* TRANSLATORS: Arguments: 1: Prefix (Space if required) 2: Street Name 3: Separator (Space if required), 4: Systematic Street Name. Male form. The stuff after | doesn't have to be included */
			ret=g_strdup_printf(_("%sinto the %s%s%s|male form"),prefix,name1, sep, name2);
			break;
		case 2:
			/* TRANSLATORS: Arguments: 1: Prefix (Space if required) 2: Street Name 3: Separator (Space if required), 4: Systematic Street Name. Female form. The stuff after | doesn't have to be included */
			ret=g_strdup_printf(_("%sinto the %s%s%s|female form"),prefix,name1, sep, name2);
			break;
		case 3:
			/* TRANSLATORS: Arguments: 1: Prefix (Space if required) 2: Street Name 3: Separator (Space if required), 4: Systematic Street Name. Neutral form. The stuff after | doesn't have to be included */
			ret=g_strdup_printf(_("%sinto the %s%s%s|neutral form"),prefix,name1, sep, name2);
			break;
		}
		g_free(name1);
			
	} else
		/* TRANSLATORS: gives the name of the next road to turn into (into the E17) */
		ret=g_strdup_printf(_("into the %s"),itm->name2);
	name1=ret;
	while (*name1) {
		switch (*name1) {
		case '|':
			*name1='\0';
			break;
		case '/':
			*name1++=' ';
			break;
		default:
			name1++;
		}
	}
	return ret;
}


static char *
show_maneuver(struct navigation *nav, struct navigation_itm *itm, struct navigation_command *cmd, enum attr_type type)
{
	/* TRANSLATORS: right, as in 'Turn right' */
	char *dir=_("right"),*strength="";
	int distance=itm->dest_length-cmd->itm->dest_length;
	char *d,*ret;
	int delta=cmd->delta;
	int level;
	level=1;
	if (delta < 0) {
		/* TRANSLATORS: left, as in 'Turn left' */
		dir=_("left");
		delta=-delta;
	}
	if (delta < 45) {
		/* TRANSLATORS: Don't forget the ending space */
		strength=_("easily ");
	} else if (delta < 105) {
		strength="";
	} else if (delta < 165) {
		/* TRANSLATORS: Don't forget the ending space */
		strength=_("strongly ");
	} else {
		dbg(1,"delta=%d\n", delta);
		/* TRANSLATORS: Don't forget the ending space */
		strength=_("unknown ");
	}
	if (type != attr_navigation_long_exact) 
		distance=round_distance(distance);
	if (type == attr_navigation_speech) {
		if (nav->turn_around && nav->turn_around == nav->turn_around_limit) 
			return g_strdup(_("When possible, please turn around"));
		level=navigation_get_announce_level(nav, itm->item.type, distance);
		dbg(1,"distance=%d level=%d type=0x%x\n", distance, level, itm->item.type);
	}
	switch(level) {
	case 3:
		d=get_distance(distance, type, 1);
		ret=g_strdup_printf(_("Follow the road for the next %s"), d);
		g_free(d);
		return ret;
	case 2:
		d=g_strdup(_("soon"));
		break;
	case 1:
		d=get_distance(distance, type, 0);
		break;
	case 0:
		d=g_strdup(_("now"));
		break;
	default:
		d=g_strdup(_("error"));
	}
	if (cmd->itm->next) {
		int tellstreetname = 0;
		char *destination = NULL; 
 
		if(type == attr_navigation_speech) { // In voice mode
			// In Voice Mode only tell the street name in level 1 or in level 0 if level 1
			// was skipped

			if (level == 1) { // we are close to the intersection
				cmd->itm->told = 1; // remeber to be checked when we turn
				tellstreetname = 1; // Ok so we tell the name of the street 
			}

			if (level == 0) {
				if(cmd->itm->told == 0) // we are write at the intersection
					tellstreetname = 1; 
				else
					cmd->itm->told = 0;  // reset just in case we come to the same street again
			}

		}
		else
		     tellstreetname = 1;

		if(tellstreetname) 
			destination=navigation_item_destination(cmd->itm, itm, " ");
		/* TRANSLATORS: The first argument is strength, the second direction, the third distance and the fourth destination Example: 'Turn 'slightly' 'left' in '100 m' 'onto baker street' */
		ret=g_strdup_printf(_("Turn %1$s%2$s %3$s%4$s"), strength, dir, d, destination ? destination:"");
		g_free(destination);
	} else
		ret=g_strdup_printf(_("You have reached your destination %s"), d);
	g_free(d);
	return ret;
}

static void
navigation_call_callbacks(struct navigation *this_, int force_speech)
{
	int distance, level = 0;
	void *p=this_;
	if (!this_->cmd_first)
		return;
	callback_list_call(this_->callback, 1, &p);
	dbg(1,"force_speech=%d turn_around=%d turn_around_limit=%d\n", force_speech, this_->turn_around, this_->turn_around_limit);
	distance=round_distance(this_->first->dest_length-this_->cmd_first->itm->dest_length);
	if (this_->turn_around_limit && this_->turn_around == this_->turn_around_limit) {
		dbg(1,"distance=%d distance_turn=%d\n", distance, this_->distance_turn);
		while (distance > this_->distance_turn) {
			this_->level_last=4;
			level=4;
			force_speech=1;
			if (this_->distance_turn >= 500)
				this_->distance_turn*=2;
			else
				this_->distance_turn=500;
		}
	} else if (!this_->turn_around_limit || this_->turn_around == -this_->turn_around_limit+1) {
		this_->distance_turn=50;
		level=navigation_get_announce_level(this_, this_->first->item.type, distance);
		if (level < this_->level_last) {
			dbg(1,"level %d < %d\n", level, this_->level_last);
			this_->level_last=level;
			force_speech=1;
		}
		if (!item_is_equal(this_->cmd_first->itm->item, this_->item_last)) {
			dbg(1,"item different\n");
			this_->item_last=this_->cmd_first->itm->item;
			force_speech=1;
		}
	}
	if (force_speech) {
		this_->level_last=level;
		dbg(1,"distance=%d level=%d type=0x%x\n", distance, level, this_->first->item.type);
		callback_list_call(this_->callback_speech, 1, &p);
	}
}

void
navigation_update(struct navigation *this_, struct route *route)
{
	struct map *map;
	struct map_rect *mr;
	struct item *ritem;			/* Holds an item from the route map */
	struct item *sitem;			/* Holds the corresponding item from the actual map */
	struct attr street_item,street_direction;
	struct navigation_itm *itm;
	int incr=0,first=1;

	if (! route)
		return;
	map=route_get_map(route);
	if (! map)
		return;
	mr=map_rect_new(map, NULL);
	if (! mr)
		return;
	dbg(1,"enter\n");
	while ((ritem=map_rect_get_item(mr))) {
		if (first && item_attr_get(ritem, attr_street_item, &street_item)) {
			first=0;
			if (!item_attr_get(ritem, attr_direction, &street_direction))
				street_direction.u.num=0;
			sitem=street_item.u.item;
			dbg(1,"sitem=%p\n", sitem);
			itm=item_hash_lookup(this_->hash, sitem);
			dbg(2,"itm for item with id (0x%x,0x%x) is %p\n", sitem->id_hi, sitem->id_lo, itm);
			if (itm && itm->direction != street_direction.u.num) {
				dbg(2,"wrong direction\n");
				itm=NULL;
			}
			navigation_destroy_itms_cmds(this_, itm);
			if (itm) {
				navigation_itm_update(itm, ritem);
				break;
			}
			dbg(1,"not on track\n");
		}
		navigation_itm_new(this_, ritem);
	}
	if (first) 
		navigation_destroy_itms_cmds(this_, NULL);
	else {
		if (! ritem) {
			navigation_itm_new(this_, NULL);
			make_maneuvers(this_);
		}
		calculate_dest_distance(this_, incr);
		dbg(2,"destination distance old=%d new=%d\n", this_->distance_last, this_->first->dest_length);
		if (this_->first->dest_length > this_->distance_last && this_->distance_last >= 0) 
			this_->turn_around++;
		else
			this_->turn_around--;
		if (this_->turn_around > this_->turn_around_limit)
			this_->turn_around=this_->turn_around_limit;
		else if (this_->turn_around < -this_->turn_around_limit+1)
			this_->turn_around=-this_->turn_around_limit+1;
		dbg(2,"turn_around=%d\n", this_->turn_around);
		this_->distance_last=this_->first->dest_length;
		profile(0,"end");
		navigation_call_callbacks(this_, FALSE);
	}
	map_rect_destroy(mr);
}

void
navigation_flush(struct navigation *this_)
{
	navigation_destroy_itms_cmds(this_, NULL);
}


void
navigation_destroy(struct navigation *this_)
{
	navigation_flush(this_);
	item_hash_destroy(this_->hash);
	callback_list_destroy(this_->callback);
	callback_list_destroy(this_->callback_speech);
	g_free(this_);
}

int
navigation_register_callback(struct navigation *this_, enum attr_type type, struct callback *cb)
{
	if (type == attr_navigation_speech)
		callback_list_add(this_->callback_speech, cb);
	else
		callback_list_add(this_->callback, cb);
	return 1;
}

void
navigation_unregister_callback(struct navigation *this_, enum attr_type type, struct callback *cb)
{
	if (type == attr_navigation_speech)
		callback_list_remove_destroy(this_->callback_speech, cb);
	else
		callback_list_remove_destroy(this_->callback, cb);
}

struct map *
navigation_get_map(struct navigation *this_)
{
	if (! this_->map)
		this_->map=map_new(NULL, (struct attr*[]){
			&(struct attr){attr_type,{"navigation"}},
			&(struct attr){attr_navigation,.u.navigation=this_},
			&(struct attr){attr_data,{""}},
			&(struct attr){attr_description,{"Navigation"}},
			NULL});
        return this_->map;
}

struct map_priv {
	struct navigation *navigation;
};

struct map_rect_priv {
	struct navigation *nav;
	struct navigation_command *cmd;
	struct navigation_command *cmd_next;
	struct navigation_itm *itm;
	struct navigation_itm *itm_next;
	struct navigation_itm *cmd_itm;
	struct navigation_itm *cmd_itm_next;
	struct item item;
	enum attr_type attr_next;
	int ccount;
	int debug_idx;
	int show_all;
	char *str;
};

static int
navigation_map_item_coord_get(void *priv_data, struct coord *c, int count)
{
	struct map_rect_priv *this=priv_data;
	if (this->ccount || ! count)
		return 0;
	*c=this->itm->start;
	this->ccount=1;
	return 1;
}

static int
navigation_map_item_attr_get(void *priv_data, enum attr_type attr_type, struct attr *attr)
{
	struct map_rect_priv *this_=priv_data;
	attr->type=attr_type;
	struct navigation_command *cmd=this_->cmd;
	struct navigation_itm *itm=this_->itm;
	struct navigation_itm *prev=itm->prev;

	if (this_->str) {
		g_free(this_->str);
		this_->str=NULL;
	}

	if (cmd) {
		if (cmd->itm != itm)
			cmd=NULL;	
	}
	switch(attr_type) {
	case attr_navigation_short:
		this_->attr_next=attr_navigation_long;
		if (cmd) {
			this_->str=attr->u.str=show_maneuver(this_->nav, this_->cmd_itm, cmd, attr_type);
			return 1;
		}
		return 0;
	case attr_navigation_long:
		this_->attr_next=attr_navigation_long_exact;
		if (cmd) {
			this_->str=attr->u.str=show_maneuver(this_->nav, this_->cmd_itm, cmd, attr_type);
			return 1;
		}
		return 0;
	case attr_navigation_long_exact:
		this_->attr_next=attr_navigation_speech;
		if (cmd) {
			this_->str=attr->u.str=show_maneuver(this_->nav, this_->cmd_itm, cmd, attr_type);
			return 1;
		}
		return 0;
	case attr_navigation_speech:
		this_->attr_next=attr_length;
		if (cmd) {
			this_->str=attr->u.str=show_maneuver(this_->nav, this_->cmd_itm, this_->cmd, attr_type);
			return 1;
		}
		return 0;
	case attr_length:
		this_->attr_next=attr_time;
		if (cmd) {
			attr->u.num=this_->cmd_itm->dest_length-cmd->itm->dest_length;
			return 1;
		}
		return 0;
	case attr_time:
		this_->attr_next=attr_destination_length;
		if (cmd) {
			attr->u.num=this_->cmd_itm->dest_time-cmd->itm->dest_time;
			return 1;
		}
		return 0;
	case attr_destination_length:
		attr->u.num=itm->dest_length;
		this_->attr_next=attr_destination_time;
		return 1;
	case attr_destination_time:
		attr->u.num=itm->dest_time;
		this_->attr_next=attr_street_name;
		return 1;
	case attr_street_name:
		attr->u.str=itm->name1;
		this_->attr_next=attr_street_name_systematic;
		if (attr->u.str)
			return 1;
	case attr_street_name_systematic:
		attr->u.str=itm->name2;
		this_->attr_next=attr_debug;
		if (attr->u.str)
			return 1;
	case attr_debug:
		switch(this_->debug_idx) {
		case 0:
			this_->debug_idx++;
			this_->str=attr->u.str=g_strdup_printf("angle:%d - %d", itm->angle_start, itm->angle_end);
			return 1;
		case 1:
			this_->debug_idx++;
			this_->str=attr->u.str=g_strdup_printf("item type:%s", item_to_name(itm->item.type));
			return 1;
		case 2:
			this_->debug_idx++;
			if (cmd) {
				this_->str=attr->u.str=g_strdup_printf("delta:%d", cmd->delta);
				return 1;
			}
		case 3:
			this_->debug_idx++;
			if (prev) {
				this_->str=attr->u.str=g_strdup_printf("prev street_name:%s", prev->name1);
				return 1;
			}
		case 4:
			this_->debug_idx++;
			if (prev) {
				this_->str=attr->u.str=g_strdup_printf("prev street_name_systematic:%s", prev->name2);
				return 1;
			}
		case 5:
			this_->debug_idx++;
			if (prev) {
				this_->str=attr->u.str=g_strdup_printf("prev angle:%d - %d", prev->angle_start, prev->angle_end);
				return 1;
			}
		case 6:
			this_->debug_idx++;
			if (prev) {
				this_->str=attr->u.str=g_strdup_printf("prev item type:%s", item_to_name(prev->item.type));
				return 1;
			}
		default:
			this_->attr_next=attr_none;
			return 0;
		}
	case attr_any:
		while (this_->attr_next != attr_none) {
			if (navigation_map_item_attr_get(priv_data, this_->attr_next, attr))
				return 1;
		}
		return 0;
	default:
		attr->type=attr_none;
		return 0;
	}
}

static struct item_methods navigation_map_item_methods = {
	NULL,
	navigation_map_item_coord_get,
	NULL,
	navigation_map_item_attr_get,
};


static void
navigation_map_destroy(struct map_priv *priv)
{
	g_free(priv);
}

static void
navigation_map_rect_init(struct map_rect_priv *priv)
{
	priv->cmd_next=priv->nav->cmd_first;
	priv->cmd_itm_next=priv->itm_next=priv->nav->first;
}

static struct map_rect_priv *
navigation_map_rect_new(struct map_priv *priv, struct map_selection *sel)
{
	struct navigation *nav=priv->navigation;
	struct map_rect_priv *ret=g_new0(struct map_rect_priv, 1);
	ret->nav=nav;
	navigation_map_rect_init(ret);
	ret->item.meth=&navigation_map_item_methods;
	ret->item.priv_data=ret;
	return ret;
}

static void
navigation_map_rect_destroy(struct map_rect_priv *priv)
{
	g_free(priv);
}

static struct item *
navigation_map_get_item(struct map_rect_priv *priv)
{
	struct item *ret=&priv->item;
	int delta;
	if (!priv->itm_next)
		return NULL;
	priv->itm=priv->itm_next;
	priv->cmd=priv->cmd_next;
	priv->cmd_itm=priv->cmd_itm_next;
	if (!priv->show_all && priv->itm->prev != NULL) {
		if (!priv->cmd)
			return NULL;
		priv->itm=priv->cmd->itm;
	}
	priv->itm_next=priv->itm->next;
	if (priv->itm->prev)
		ret->type=type_nav_none;
	else
		ret->type=type_nav_position;
	if (priv->cmd->itm == priv->itm) {
		priv->cmd_itm_next=priv->cmd->itm;
		priv->cmd_next=priv->cmd->next;
		if (priv->cmd_itm_next && !priv->cmd_itm_next->next)
			ret->type=type_nav_destination;
		else {
			delta=priv->cmd->delta;	
			if (delta < 0) {
				delta=-delta;
				if (delta < 45)
					ret->type=type_nav_left_1;
				else if (delta < 105)
					ret->type=type_nav_left_2;
				else if (delta < 165) 
					ret->type=type_nav_left_3;
				else
					ret->type=type_none;
			} else {
				if (delta < 45)
					ret->type=type_nav_right_1;
				else if (delta < 105)
					ret->type=type_nav_right_2;
				else if (delta < 165) 
					ret->type=type_nav_right_3;
				else
					ret->type=type_none;
			}
		}
	}
	priv->ccount=0;
	priv->debug_idx=0;
	priv->attr_next=attr_navigation_short;

	ret->id_lo=priv->itm->dest_count;
	dbg(1,"type=%d\n", ret->type);
	return ret;
}

static struct item *
navigation_map_get_item_byid(struct map_rect_priv *priv, int id_hi, int id_lo)
{
	struct item *ret;
	navigation_map_rect_init(priv);
	while ((ret=navigation_map_get_item(priv))) {
		if (ret->id_hi == id_hi && ret->id_lo == id_lo) 
			return ret;
	}
	return NULL;
}

static struct map_methods navigation_map_meth = {
	projection_mg,
	"utf-8",
	navigation_map_destroy,
	navigation_map_rect_new,
	navigation_map_rect_destroy,
	navigation_map_get_item,
	navigation_map_get_item_byid,
	NULL,
	NULL,
	NULL,
};

static struct map_priv *
navigation_map_new(struct map_methods *meth, struct attr **attrs)
{
	struct map_priv *ret;
	struct attr *navigation_attr;

	navigation_attr=attr_search(attrs, NULL, attr_navigation);
	if (! navigation_attr)
		return NULL;
	ret=g_new0(struct map_priv, 1);
	*meth=navigation_map_meth;
	ret->navigation=navigation_attr->u.navigation;

	return ret;
}


void
navigation_init(void)
{
	plugin_register_map_type("navigation", navigation_map_new);
}
