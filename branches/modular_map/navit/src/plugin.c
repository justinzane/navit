#include <gmodule.h>
#include "plugin.h"
#define PLUGIN_C
#include "plugin.h"

struct plugin *
plugin_load(char *plugin)
{
	void (*init)(void);
	gpointer plugin_init;

	GModule *mod;

	mod=g_module_open(plugin, G_MODULE_BIND_LAZY);
	if (! mod) {
		g_warning("can't load '%s', Error '%s'\n", plugin, g_module_error());
		return NULL;
	}
	if (!g_module_symbol(mod, "plugin_init", &plugin_init)) {
		g_warning("can't load '%s', plugin_init not found\n", plugin);
		g_module_close(mod);
		return NULL;
	} else {
		init=plugin_init;
		(*init)();
	}
	return (struct plugin *)mod;
}

void *
plugin_get_type(enum plugin_type type, char *name)
{
	GList *l;
	struct name_val *nv;
	l=plugin_types[type];
	while (l) {
		nv=l->data;
	 	if (!g_ascii_strcasecmp(nv->name, name))
			return nv->val;
		l=g_list_next(l);
	}
	return NULL;
}


void
plugin_init(void)
{
	if (! g_module_supported()) {
		g_error("plugins not supported");
	}
	plugin_load("gui/gtk/gtk.so");
	plugin_load("graphics/gtk_drawing_area/gtk_drawing_area.so");
	plugin_load("data/mg/mg.so");
	plugin_load("data/textfile/textfile.so");
	plugin_load("data/garmin_img/garmin_img.so");
}


