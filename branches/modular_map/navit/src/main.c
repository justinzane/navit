#include <locale.h>
#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include "file.h"
#include "debug.h"
#ifdef HAVE_PYTHON
#include "python.h"
#endif
#include "plugin.h"
#include "xmlconfig.h"

struct map_data *map_data_default;

int main(int argc, char **argv)
{
	GError *error = NULL;

	setenv("LC_NUMERIC","C",1);
	setlocale(LC_ALL,"");
	setlocale(LC_NUMERIC,"C");
	gtk_set_locale();
	setlocale(LC_NUMERIC,"C");
	debug_init();
	gtk_init(&argc, &argv);
	gdk_rgb_init();

#ifdef HAVE_PYTHON
	python_init();
#endif
	plugin_init();
	if (argc > 1) 
		config_load(argv[1], &error);
	else
		if (file_exists("navit.xml.local"))
			config_load("navit.xml.local", &error);
		else	
			config_load("navit.xml", &error);
	gtk_main();
	return 0;
}
