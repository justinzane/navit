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
	GError *error;

	setenv("LC_NUMERIC","C",1);
	setlocale(LC_ALL,"");
	setlocale(LC_NUMERIC,"C");
	gtk_set_locale();
	setlocale(LC_NUMERIC,"C");
	debug_init();
	gtk_init(&argc, &argv);
	gdk_rgb_init();

	python_init();
	plugin_init();
#if 0
	map_data_default=load_maps(NULL);
#endif
#if 1
	co=gui_gtk_window(0x137c79,0x5f2679,1024);
#else
	co=gui_gtk_window(0x11e8a1,0x632815,1024);
#endif
	config_load("navit.xml", co, &error);

#if 0	/* FIXME */
	co->route=route_new();
	route_mapdata_set(co->route, co->map_data); 
#endif
	gps=getenv("GPSDATA");
	if (gps) {
		co->vehicle=vehicle_new(gps);
		if (co->vehicle) {
			co->cursor=cursor_new(co,co->vehicle);
		}
	} else {
		g_warning("Environment-Variable GPSDATA not set - No gps tracking. Set it to file:filename or gpsd://host[:port]");
	}
	co->speech=speech_new();
	speech_handle=co->speech;
	if (co->vehicle)
		co->compass=compass_new(co);
#if 0 /* FIXME */
	if (co->vehicle)
		co->track=track_new(co->map_data);
#endif

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

	gtk_main();
	return 0;
}


