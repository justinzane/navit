struct menu_methods {
};

struct menu {
	struct menu_priv *priv;
	struct menu_methods meth;
};

struct container;

void menu_route_do_update(struct container *co);
void menu_route_update(struct container *co);
