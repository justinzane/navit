enum menu_type {
	menu_type_submenu,
	menu_type_menu,
	menu_type_toggle,
};

struct menu_methods {
	struct menu_priv *(*add)(struct menu_priv *menu, struct menu_methods *meth, char *name, enum menu_type type, void (*callback)(void *data), void *data);
};

struct menu {
	struct menu_priv *priv;
	struct menu_methods meth;
};

struct container;

void menu_route_do_update(struct container *co);
void menu_route_update(struct container *co);
