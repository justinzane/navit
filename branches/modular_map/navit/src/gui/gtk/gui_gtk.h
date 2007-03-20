struct statusbar_methods;
struct menu_methods;
struct navit;

struct gui_priv {
        GtkWindow *win;
        GtkVBox *vbox;
	GtkActionGroup *base_group;
	GtkActionGroup *debug_group;
	GtkActionGroup *dyn_group;
	GtkUIManager *menu_manager;
        void *statusbar;
	int dyn_counter;
};

struct menu_priv *gui_gtk_menubar_new(struct gui_priv *gui, struct menu_methods *meth, struct navit *nav);
struct menu_priv *gui_gtk_toolbar_new(struct gui_priv *gui, struct menu_methods *meth, struct navit *nav);
struct statusbar_priv *gui_gtk_statusbar_new(struct gui_priv *gui, struct statusbar_methods *meth, struct navit *nav);

