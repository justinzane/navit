#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include "coord.h"
#include "vehicle.h"
#include "cursor.h"
#include "speech.h"
#include "route.h"
#include "map.h"
#include "map_data.h"
#if 0
#include "map-share.h"
#endif
#include "transform.h"
#include "popup.h"
#include "plugin.h"
#include "compass.h"
#include "track.h"
#include "container.h"
#include "debug.h"

#include "gui/sdl/sdl.c"
#include "SDL/SDL_gfxPrimitives.h"

// #define DEBUG 1

void *speech_handle;

struct container *co;

struct map_data *map_data_default;

struct container *gui_gtk_window(int x, int y, int scale);

extern void test(struct map_data *mdat);

int main(int argc, char **argv)
{
#if 0
        CORBA_Environment ev;
        CORBA_ORB orb;
	Map map_client = CORBA_OBJECT_NIL;
#endif
	char *gps;

	setenv("LC_NUMERIC","C",1);
	setlocale(LC_ALL,"");
	setlocale(LC_NUMERIC,"C");
	gtk_set_locale();
	setlocale(LC_NUMERIC,"C");
	debug_init();
	gtk_init(&argc, &argv);
	gdk_rgb_init();

// 	i18n basic support

	bindtextdomain( "navit", "./locale" );
	textdomain( "navit" );

	map_data_default=load_maps(NULL);
	plugin_load();
	co=gui_gtk_window(1300000,7000000,32);
	
	co->route=route_new();
	route_mapdata_set(co->route, co->map_data); 
	gps=getenv("GPSDATA");
	if (gps) {
		co->vehicle=vehicle_new(gps);
		if (co->vehicle) {
			co->cursor=cursor_new(co,co->vehicle);
			sdl_gui_new(co->vehicle);
		}
	} else {
		g_warning(gettext("Environment-Variable GPSDATA not set - No gps tracking. Set it to file:filename or gpsd://host[:port]"));
	}
	co->speech=speech_new();
	if (! co->speech) 
		g_warning(gettext("Can't connect to speechd, no speech output available"));
	speech_handle=co->speech;
	if (co->vehicle)
		co->compass=compass_new(co);
	if (co->vehicle)
		co->track=track_new(co->map_data);


#if 0
        CORBA_exception_init(&ev);
        orb = CORBA_ORB_init(&argc, argv, "orbit-local-orb", &ev);
        g_assert(ev._major == CORBA_NO_EXCEPTION);

        map_srv_start_poa(orb, &ev);
        g_assert(ev._major == CORBA_NO_EXCEPTION);
        map_client = map_srv_start_object(&ev, map);
        retval = CORBA_ORB_object_to_string(orb, map_client, &ev);
        g_assert(ev._major == CORBA_NO_EXCEPTION);
	ior=fopen("map.ior","w");
	if (ior) {
  		fprintf(ior, "%s\n", retval);
		fclose(ior);
	}
        CORBA_free(retval);
#endif

	initSDL();
	gtk_main();
	return 0;
}


