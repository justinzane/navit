#include <gtk/gtk.h>
#include "graphics.h"
#include "gui_gtk.h"
#include "menu.h"
#include "coord.h"
#include "destination.h"
#include "container.h"

struct toolbar_gui {
	struct container *co;
};

#include "toolbar.h"

static void
zoom_in(GtkWidget *w, struct toolbar *tb)
{
	unsigned long scale;
	graphics_get_view(tb->gui->co, NULL, NULL, &scale);
	scale/=2;
	if (scale < 1)
		scale=1;
	graphics_set_view(tb->gui->co, NULL, NULL, &scale);
}

static void
zoom_out(GtkWidget *w, struct toolbar *tb)
{
	unsigned long scale;
	graphics_get_view(tb->gui->co, NULL, NULL, &scale);
	scale*=2;
	graphics_set_view(tb->gui->co, NULL, NULL, &scale);
}

static void
refresh_route(GtkWidget *w, struct toolbar *tb)
{
	menu_route_update(tb->gui->co);
}

static void
track(GtkWidget *w, struct toolbar *tb)
{
	if (tb->gui->co->flags->track) {
		tb->gui->co->flags->track=0;
		gtk_widget_set_state(w, GTK_STATE_ACTIVE);
	} else {
		tb->gui->co->flags->track=1;
		gtk_widget_set_state(w, GTK_STATE_ACTIVE);
	}
}

static void
orient_north(GtkWidget *w, struct toolbar *tb)
{
	if (tb->gui->co->flags->orient_north) {
		tb->gui->co->flags->orient_north=0;
		gtk_widget_set_state(w, GTK_STATE_ACTIVE);
	} else {
		tb->gui->co->flags->orient_north=1;
		gtk_widget_set_state(w, GTK_STATE_ACTIVE);
	}
}

static void
destination(GtkWidget *w, struct toolbar *tb)
{
	destination_address(tb->gui->co);
}


/* XPM */
/* Drawn  by Mark Donohoe for the K Desktop Environment */
/* See http://www.kde.org */
static char*viewmag_plus_xpm[]={
"22 22 5 1",
"# c #000000",
"c c #a0a0a4",
"a c #dcdcdc",
"b c #ffffff",
". c None",
"......................",
"......................",
"......................",
"......................",
"......................",
".......####...........",
"......#abba#..........",
".....#abcbba#.........",
".....#bcbbbb#.........",
".....#bbbbbb#.........",
".....#abbbba#.........",
"......#abba##.........",
".......#######........",
"............###.......",
".....#.......###......",
".....#........###.....",
"...#####.......##.....",
".....#................",
".....#................",
"......................",
"......................",
"......................"};


/* XPM */
/* Drawn  by Mark Donohoe for the K Desktop Environment */
/* See http://www.kde.org */
static char*viewmag_minus_xpm[]={
"22 22 5 1",
"# c #000000",
"c c #a0a0a4",
"a c #dcdcdc",
"b c #ffffff",
". c None",
"......................",
"......................",
"......................",
"......................",
"......................",
".......####...........",
"......#abba#..........",
".....#abcbba#.........",
".....#bcbbbb#.........",
".....#bbbbbb#.........",
".....#abbbba#.........",
"......#abba##.........",
".......#######........",
"............###.......",
".............###......",
"..............###.....",
"...#####.......##.....",
"......................",
"......................",
"......................",
"......................",
"......................"};


/* XPM */
/* Drawn  by Mark Donohoe for the K Desktop Environment */
/* See http://www.kde.org */
static char*reload_xpm[]={
"22 22 3 1",
"# c #808080",
"a c #000000",
". c None",
"......................",
"......................",
"......................",
"......................",
"........##aaa#........",
".......#aaaaaaa.......",
"......#aa#....#a......",
"......aa#.............",
".....aaa.......a......",
"...aaaaaaa....aaa.....",
"....aaaaa....aaaaa....",
".....aaa....aaaaaaa...",
"......a.......aaa.....",
".............#aa......",
"......a#....#aa#......",
".......aaaaaaa#.......",
"........#aaa##........",
"......................",
"......................",
"......................",
"......................",
"......................"};


/* XPM */
static char * cursor_xpm[] = {
"22 22 2 1",
" 	c None",
".	c #0000FF",
"                      ",
"                      ",
"                      ",
"          ..          ",
"        ..  ..        ",
"      ..      ..      ",
"     .          .     ",
"     .          .     ",
"    .        ... .    ",
"    .     ... .  .    ",
"   .   ...   .    .   ",
"   . ..     .     .   ",
"    .      .     .    ",
"    .     .      .    ",
"     .   .      .     ",
"     .  .       .     ",
"      ..      ..      ",
"        ..  ..        ",
"          ..          ",
"                      ",
"                      ",
"                      "};


/* XPM */
static char * north_xpm[] = {
"22 22 2 1",
" 	c None",
".	c #000000",
"                      ",
"                      ",
"           .          ",
"          ...         ",
"         . . .        ",
"        .  .  .       ",
"           .          ",
"     ....  .  ....    ",
"     ....  .  ....    ",
"      .... .   ..     ",
"      .. ..    ..     ",
"      ..  ..   ..     ",
"      ..   ..  ..     ",
"      ..    .. ..     ",
"      ..   . ....     ",
"     ....  .  ....    ",
"     ....  .  ....    ",
"           .          ",
"           .          ",
"           .          ",
"                      ",
"                      "};


/* XPM */
static char * flag_xpm[] = {
"22 22 2 1",
" 	c None",
"+	c #000000",
"+++++++               ",
"+   +++++++++         ",
"+   +++   +++++++++   ",
"+   +++   +++   +++   ",
"++++      +++   +++   ",
"++++   +++      +++   ",
"++++   +++   +++  +   ",
"+   ++++++   +++  +   ",
"+   +++   ++++++  +   ",
"+   +++   +++   +++   ",
"++++      +++   +++   ",
"++++   +++      +++   ",
"++++++++++   +++  +   ",
"+      +++++++++  +   ",
"+            ++++++   ",
"+                     ",
"+                     ",
"+                     ",
"+                     ",
"+                     ",
"+                     ",
"+                     "};

static GtkWidget *
xpm_to_widget(GtkWidget *draw, gchar **xpm_data)
{
	/* GtkWidget is the storage type for widgets */
	GtkWidget *imagewid;
	GdkPixmap *pixmap;
	GdkBitmap *mask;
	GtkStyle *style;

	style = gtk_widget_get_style(draw);

	/* In order for this to not create a warning, window has to be a
	* gtk_realize_widget (realized) widget
	*/
	pixmap = gdk_pixmap_create_from_xpm_d(draw->window,  &mask,
				   &style->bg[GTK_STATE_NORMAL],
				   (gchar **)xpm_data );

	/* a pixmap widget to contain the pixmap */
	imagewid = gtk_image_new_from_pixmap(pixmap, mask);
	gtk_widget_show(imagewid);

	return(imagewid);
}

int tst_stat=1;

static void
toolbar_button(GtkWidget *window, GtkWidget *toolbar, char **icon_data, char *text, void (*func)(GtkWidget *w, struct toolbar *tb), void *data)
{
	GtkWidget *icon;
	GtkToolItem *toolitem;
	
	icon=xpm_to_widget(window, icon_data);
	toolitem=gtk_tool_button_new(icon,text);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),toolitem,-1);
	g_signal_connect(G_OBJECT(toolitem), "clicked", G_CALLBACK(func), data);
}

static void
toolbar_button_toggle(GtkWidget *window, GtkWidget *toolbar, char **icon_data, char *text, void (*func)(GtkWidget *w, struct toolbar *tb), void *data, int *flag)
{
	GtkWidget *icon;
	GtkToolItem *toggleitem;
	
	icon=xpm_to_widget(window, icon_data);
	toggleitem=gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(toggleitem),icon);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(toggleitem),text);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar),toggleitem,-1);
	if(*flag)
	    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(toggleitem),TRUE);
	    
	g_signal_connect(G_OBJECT(toggleitem), "clicked", G_CALLBACK(func), data);
}


struct toolbar *
gui_gtk_toolbar_new(struct container *co, GtkWidget **widget)
{
	GtkWidget *toolbar,*window;
	struct toolbar *this=g_new0(struct toolbar, 1);

	this->gui=g_new0(struct toolbar_gui, 1);	
	this->gui->co=co;

	toolbar=gtk_toolbar_new();
	window=(GtkWidget *)(co->win);

	co->flags->track=1;
	co->flags->orient_north=1;

	toolbar_button(window, toolbar, viewmag_plus_xpm, "Zoom In", zoom_in, this);
	toolbar_button(window, toolbar, viewmag_minus_xpm, "Zoom Out", zoom_out, this);
	toolbar_button(window, toolbar, reload_xpm, "Refresh Route", refresh_route, this);
	toolbar_button_toggle(window, toolbar, cursor_xpm, "Cursor on/off", track, this, &co->flags->track);
	toolbar_button_toggle(window, toolbar, north_xpm, "Orientate North on/off", orient_north, this, &co->flags->orient_north);
	toolbar_button(window, toolbar, flag_xpm, "Destination", destination, this);

	*widget=toolbar;

	return this;
}
