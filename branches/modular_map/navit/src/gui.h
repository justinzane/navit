struct navit;
struct gui_priv;
struct menu_methods;
struct statusbar_methods;
struct graphics;

struct gui_methods {
	struct menu_priv *(*menubar_new)(struct gui_priv *priv, struct menu_methods *meth, struct navit *nav);
	struct menu_priv *(*toolbar_new)(struct gui_priv *priv, struct menu_methods *meth, struct navit *nav);
	struct statusbar_priv *(*statusbar_new)(struct gui_priv *priv, struct statusbar_methods *meth, struct navit *nav);
	struct menu_priv *(*popup_new)(struct gui_priv *priv, struct menu_methods *meth, struct navit *nav);
	int (*set_graphics)(struct gui_priv *priv, struct graphics *gra);
};


struct gui {
	struct gui_methods meth;
	struct gui_priv *priv;		
};

/* prototypes */
struct graphics;
struct gui;
struct menu;
struct navit;
struct statusbar;
struct gui *gui_new(char *type, int w, int h);
struct statusbar *gui_statusbar_new(struct gui *gui, struct navit *nav);
struct menu *gui_menubar_new(struct gui *gui, struct navit *nav);
struct menu *gui_toolbar_new(struct gui *gui, struct navit *nav);
struct menu *gui_popup_new(struct gui *gui, struct navit *nav);
int gui_set_graphics(struct gui *this, struct graphics *gra);
