#include <locale.h>
#include <stdlib.h>
#include <glib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <gtk/gtk.h>
#include "file.h"
#include "debug.h"
#ifdef HAVE_PYTHON
#include "python.h"
#endif
#include "plugin.h"
#include "xmlconfig.h"

struct map_data *map_data_default;

static void sigchld(int sig)
{
	int status;
	while (waitpid(-1, &status, WNOHANG) > 0);
}

int main(int argc, char **argv)
{
	GError *error = NULL;
#if 0
	GMainLoop *loop;
#endif

	signal(SIGCHLD, sigchld);

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
#if 1
	gtk_main();
#else
	loop = g_main_loop_new (NULL, TRUE);
	if (g_main_loop_is_running (loop))
	{
		g_main_loop_run (loop);
	}
#endif

	return 0;
}
