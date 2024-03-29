#include <locale.h>
#include <stdlib.h>
#include <glib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <gtk/gtk.h>
#include "file.h"
#include "debug.h"
#include "navit.h"
#include "gui.h"
#ifdef HAVE_PYTHON
#include "python.h"
#endif
#include "plugin.h"
#include "xmlconfig.h"

#define _(STRING)    gettext(STRING)

struct map_data *map_data_default;

static void sigchld(int sig)
{
	int status;
	while (waitpid(-1, &status, WNOHANG) > 0);
}


static gchar *get_home_directory(void)
{
	static gchar *homedir = NULL;

	if (homedir) return homedir;
	homedir = getenv("HOME");
	if (!homedir)
	{
		struct passwd *p;

// 		p = getpwuid(getuid());
// 		if (p) homedir = p->pw_dir;
	}
	if (!homedir)
	{
		g_warning(_("Could not find home directory. Using current directory as home directory."));
		homedir = ".";
	}
	return homedir;
}


int main(int argc, char **argv)
{
	GError *error = NULL;
	char *config_file = NULL;
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

	bindtextdomain( "navit", "./locale" );
	textdomain( "navit" );

#ifdef HAVE_PYTHON
	python_init();
#endif
	if (argc > 1) 
		config_file=argv[1];
	else {
		config_file=g_strjoin(NULL,get_home_directory(), "/.navit/navit.xml" , NULL);
		if (!file_exists(config_file)) {
			if (file_exists("navit.xml.local"))
				config_file="navit.xml.local";
			else
				config_file="navit.xml";
		}
	}
	if (!config_load(config_file, &error)) {
		g_error(_("Error parsing '%s': %s\n"), config_file, error->message);
	} else {
		printf(_("Using '%s'\n"), config_file);
	}
	if (main_loop_gui) {
		gui_run_main_loop(main_loop_gui);
	} else {
#if 1
		gtk_main();
#else
		loop = g_main_loop_new (NULL, TRUE);
		if (g_main_loop_is_running (loop))
		{
			g_main_loop_run (loop);
		}
#endif
	}

	return 0;
}
