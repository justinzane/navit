struct statusbar_methods;
struct menu_methods;

struct gui_priv {
        GtkWindow *win;
        GtkVBox *vbox;
	GtkActionGroup *base_group;
	GtkActionGroup *debug_group;
	GtkActionGroup *dyn_group;
	GtkUIManager *menu_manager;
        void *statusbar;
};

struct menu_priv *gui_gtk_menubar_new(struct gui_priv *gui, struct menu_methods *meth, struct container *co);
struct menu_priv *gui_gtk_toolbar_new(struct gui_priv *gui, struct menu_methods *meth, struct container *co);
struct statusbar_priv *gui_gtk_statusbar_new(struct gui_priv *gui, struct statusbar_methods *meth, struct container *co);
struct action *gui_gtk_actions_new(struct container *co, GtkWidget *vbox);
struct container * gui_gtk_window(int x, int y, int scale);

