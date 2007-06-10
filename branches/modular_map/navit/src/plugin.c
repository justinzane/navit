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
plugin_get_type(enum plugin_type type, const char *name)
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
	plugin_load("gui/gtk/.libs/libgui_gtk.so.0");
	plugin_load("graphics/gtk_drawing_area/.libs/libgraphics_gtk_drawing_area.so.0");
	plugin_load("graphics/null/.libs/libgraphics_null.so.0");
	plugin_load("data/mg/.libs/libdata_mg.so.0");
	plugin_load("data/textfile/.libs/libdata_textfile.so.0");
	plugin_load("data/garmin_img/.libs/libdata_garmin_img.so.0");
}


