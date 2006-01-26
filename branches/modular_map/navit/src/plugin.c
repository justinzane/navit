#include <stdio.h>
#include <dlfcn.h>
#include "plugin.h"
#define PLUGIN_C
#include "plugin.h"

void
plugin_load(char *plugin)
{
	void *h=dlopen(plugin,RTLD_LAZY);
	void (*init)(void);

	if (! h)
		printf("can't load '%s', Error '%s'\n", plugin, dlerror());
	else {
		init=dlsym(h,"plugin_init");
		(*init)();
	}
}

void
plugin_init(void)
{
	plugin_load("data/mg/mg.so");
}
