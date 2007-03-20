struct navit;
struct gui_priv;
struct menu_methods;
struct statusbar_methods;
struct graphics;

struct gui_methods {
	struct menubar_priv *(*menubar_new)(struct gui_priv *priv, struct menu_methods *meth, struct navit *nav);
	struct toolbar_priv *(*toolbar_new)(struct gui_priv *priv, struct menu_methods *meth, struct navit *nav);
	struct statusbar_priv *(*statusbar_new)(struct gui_priv *priv, struct statusbar_methods *meth, struct navit *nav);
	int (*set_graphics)(struct gui_priv *priv, struct graphics *gra);
};


struct gui {
	struct gui_methods meth;
	struct gui_priv *priv;		
};

void gui_register(char *name, struct gui_priv *(*gui_new)(struct gui_methods *meth));
struct gui * gui_new(char *name, int w, int h);
