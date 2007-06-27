#include <glib.h>
#include "gui.h"
#include "statusbar.h"
#include "menu.h"
#include "plugin.h"

struct gui *
gui_new(struct navit *nav, const char *type, int w, int h)
{
	struct gui *this;
	struct gui_priv *(*guitype_new)(struct navit *nav, struct gui_methods *meth, int w, int h);

        guitype_new=plugin_get_gui_type(type);
        if (! guitype_new)
                return NULL;

	this=g_new0(struct gui, 1);
	this->priv=guitype_new(nav, &this->meth, w, h);
	return this;
}

struct statusbar *
gui_statusbar_new(struct gui *gui)
{
	struct statusbar *this;
	if (! gui->meth.statusbar_new)
		return NULL;
	this=g_new0(struct statusbar, 1);
	this->priv=gui->meth.statusbar_new(gui->priv, &this->meth);
	if (! this->priv) {
		g_free(this);
		return NULL;
	}
	return this;
}

struct menu *
gui_menubar_new(struct gui *gui)
{
	struct menu *this;
	if (! gui->meth.menubar_new)
		return NULL;
	this=g_new0(struct menu, 1);
	this->priv=gui->meth.menubar_new(gui->priv, &this->meth);
	if (! this->priv) {
		g_free(this);
		return NULL;
	}
	return this;
}


struct menu *
gui_toolbar_new(struct gui *gui)
{
	struct menu *this;
	if (! gui->meth.toolbar_new)
		return NULL;
	this=g_new0(struct menu, 1);
	this->priv=gui->meth.toolbar_new(gui->priv, &this->meth);
	if (! this->priv) {
		g_free(this);
		return NULL;
	}
	return this;
}

struct menu *
gui_popup_new(struct gui *gui)
{
	struct menu *this;
	if (! gui->meth.popup_new)
		return NULL;
	this=g_new0(struct menu, 1);
	this->priv=gui->meth.popup_new(gui->priv, &this->meth);
	if (! this->priv) {
		g_free(this);
		return NULL;
	}
	return this;
}

int
gui_set_graphics(struct gui *this, struct graphics *gra)
{
	if (! this->meth.set_graphics)
		return 1;
	return this->meth.set_graphics(this->priv, gra);
}

